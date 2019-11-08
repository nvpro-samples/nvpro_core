/* Copyright (c) 2011-2018, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


//////////////////////////////////////////////////////////////////////////

// tries to find unique geometries from original meshes, uses hashes
// for vertex/index data
#define CSF_GLTF_UNIQUE_GEOMETRIES  1

//////////////////////////////////////////////////////////////////////////


#include <assert.h>
#include <map>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#if CSF_ZIP_SUPPORT
#include <zlib.h>
#endif

#include <mutex>

#include <stddef.h>  // for memcpy
#include <string.h>  // for memcpy

#include "cadscenefile.h"
#include <NvFoundation.h>

#define CADSCENEFILE_MAGIC 1567262451

#ifdef WIN32
#define FREAD(a, b, c, d, e) fread_s(a, b, c, d, e)
#else
#define FREAD(a, b, c, d, e) fread(a, c, d, e)
#endif

#if defined(WIN32) && (defined(__amd64__) || defined(__x86_64__) || defined(_M_X64) || defined(__AMD64__))
#define xftell(f) _ftelli64(f)
#define xfseek(f, pos, encoded) _fseeki64(f, pos, encoded)
#else
#define xftell(f) ftell(f)
#define xfseek(f, pos, encoded) fseek(f, (long)pos, encoded)
#endif

struct CSFileMemory_s
{
  std::vector<void*> m_allocations;
  std::mutex         m_mutex;

  void* alloc(size_t size, const void* indata = nullptr, size_t indataSize = 0)
  {
    if(size == 0)
      return nullptr;

    void* data = malloc(size);
    if(indata)
    {
      indataSize = indataSize ? indataSize : size;
      memcpy(data, indata, indataSize);
    }

    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_allocations.push_back(data);
    }
    return data;
  }

  ~CSFileMemory_s()
  {
    for(size_t i = 0; i < m_allocations.size(); i++)
    {
      free(m_allocations[i]);
    }
  }
};

CSFAPI CSFileMemoryPTR CSFileMemory_new()
{
  return new CSFileMemory_s;
}
CSFAPI void CSFileMemory_delete(CSFileMemoryPTR mem)
{
  delete mem;
}

CSFAPI void* CSFileMemory_alloc(CSFileMemoryPTR mem, size_t sz, const void* fill)
{
  return mem->alloc(sz, fill);
}


CSFAPI void* CSFileMemory_allocPartial(CSFileMemoryPTR mem, size_t sz, size_t szPartial, const void* fillPartial)
{
  return mem->alloc(sz, fillPartial, szPartial);
}

static int CSFile_invalidVersion(const CSFile* csf)
{
  return csf->magic != CADSCENEFILE_MAGIC || csf->version < CADSCENEFILE_VERSION_COMPAT || csf->version > CADSCENEFILE_VERSION;
}

static size_t CSFile_getHeaderSize(const CSFile* csf)
{
  if(csf->version >= CADSCENEFILE_VERSION_META)
  {
    return sizeof(CSFile);
  }
  else
  {
    return offsetof(CSFile, nodeMetas);
  }
}

static size_t CSFile_getRawSize(const CSFile* csf)
{
  if(CSFile_invalidVersion(csf))
    return 0;

  return csf->pointersOFFSET + csf->numPointers * sizeof(CSFoffset);
}

CSFAPI int CSFile_loadRaw(CSFile** outcsf, size_t size, void* dataraw)
{
  char*   data = (char*)dataraw;
  CSFile* csf  = (CSFile*)data;

  if(size < sizeof(CSFile) || CSFile_invalidVersion(csf))
  {
    *outcsf = 0;
    return CADSCENEFILE_ERROR_VERSION;
  }

  if(size < CSFile_getRawSize((CSFile*)dataraw))
  {
    *outcsf = 0;
    return CADSCENEFILE_ERROR_VERSION;
  }

  if(csf->version < CADSCENEFILE_VERSION_FILEFLAGS)
  {
    csf->fileFlags = csf->fileFlags ? CADSCENEFILE_FLAG_UNIQUENODES : 0;
  }

  csf->pointersOFFSET += (CSFoffset)csf;
  for(int i = 0; i < csf->numPointers; i++)
  {
    CSFoffset* ptr = (CSFoffset*)(data + csf->pointers[i]);
    *(ptr) += (CSFoffset)csf;
  }

  if(csf->version < CADSCENEFILE_VERSION_PARTNODEIDX)
  {
    for(int i = 0; i < csf->numNodes; i++)
    {
      for(int p = 0; p < csf->nodes[i].numParts; p++)
      {
        csf->nodes[i].parts[p].nodeIDX = -1;
      }
    }
  }

  if(csf->version < CADSCENEFILE_VERSION_GEOMETRYCHANNELS)
  {
    CSFile_setupDefaultChannels(csf);
  }

  CSFile_clearDeprecated(csf);


  csf->numPointers = 0;
  csf->pointers    = nullptr;

  *outcsf = csf;

  return CADSCENEFILE_NOERROR;
}

CSFAPI int CSFile_load(CSFile** outcsf, const char* filename, CSFileMemoryPTR mem)
{
  if(!filename)
  {
    return CADSCENEFILE_ERROR_NOFILE;
  }

  FILE* file;
#ifdef WIN32
  if(fopen_s(&file, filename, "rb"))
#else
  if((file = fopen(filename, "rb")) == nullptr)
#endif
  {
    *outcsf = 0;
    return CADSCENEFILE_ERROR_NOFILE;
  }

  CSFile header     = {0};
  size_t sizeshould = 0;
  if(!FREAD(&header, sizeof(header), sizeof(header), 1, file) || (sizeshould = CSFile_getRawSize(&header)) == 0)
  {
    fclose(file);
    *outcsf = 0;
    return CADSCENEFILE_ERROR_VERSION;
  }

  // load the full file to memory
  xfseek(file, 0, SEEK_END);
  size_t size = (size_t)xftell(file);
  xfseek(file, 0, SEEK_SET);

  if(sizeshould != size)
  {
    fclose(file);
    *outcsf = 0;
    return CADSCENEFILE_ERROR_VERSION;
  }

  char* data = (char*)mem->alloc(size);
  FREAD(data, size, size, 1, file);
  fclose(file);

  return CSFile_loadRaw(outcsf, size, data);
}

#if CSF_GLTF_SUPPORT
CSFAPI int CSFile_loadGTLF(CSFile** outcsf, const char* filename, CSFileMemoryPTR mem);
#endif

#if CSF_ZIP_SUPPORT || CSF_GLTF_SUPPORT
CSFAPI int CSFile_loadExt(CSFile** outcsf, const char* filename, CSFileMemoryPTR mem)
{
  if(!filename)
  {
    return CADSCENEFILE_ERROR_NOFILE;
  }
  size_t len = strlen(filename);
#if CSF_ZIP_SUPPORT
  if(len > 3 && strcmp(filename + len - 3, ".gz") == 0)
  {
    gzFile filegz = gzopen(filename, "rb");
    if(!filegz)
    {
      *outcsf = 0;
      return CADSCENEFILE_ERROR_NOFILE;
    }

    CSFile header     = {0};
    size_t sizeshould = 0;
    if(!gzread(filegz, &header, (z_off_t)sizeof(header)) || (sizeshould = CSFile_getRawSize(&header)) == 0)
    {
      gzclose(filegz);
      *outcsf = 0;
      return CADSCENEFILE_ERROR_VERSION;
    }


    gzseek(filegz, 0, SEEK_SET);
    char* data = (char*)CSFileMemory_alloc(mem, sizeshould, 0);
    if(!gzread(filegz, data, (z_off_t)sizeshould))
    {
      gzclose(filegz);
      *outcsf = 0;
      return CADSCENEFILE_ERROR_VERSION;
    }
    gzclose(filegz);

    return CSFile_loadRaw(outcsf, sizeshould, data);
  }
  else
#endif
#if CSF_GLTF_SUPPORT
      if(len > 5 && strcmp(filename + len - 5, ".gltf") == 0)
  {
    return CSFile_loadGTLF(outcsf, filename, mem);
  }
#endif
  {
    return CSFile_load(outcsf, filename, mem);
  }
}
#endif

struct OutputFILE
{
  FILE* m_file;

  int open(const char* filename)
  {
#ifdef WIN32
    return fopen_s(&m_file, filename, "wb");
#else
    return (m_file = fopen(filename, "wb")) ? 1 : 0;
#endif
  }
  void close() { fclose(m_file); }
  void seek(size_t offset, int pos) { xfseek(m_file, offset, pos); }
  void write(const void* data, size_t dataSize) { fwrite(data, dataSize, 1, m_file); }
};


struct OutputBuf
{
  char*  m_data;
  size_t m_allocated;
  size_t m_used;
  size_t m_cur;

  int open(const char* filename)
  {
    m_allocated = 1024 * 1024;
    m_data      = (char*)malloc(m_allocated);
    m_used      = 0;
    m_cur       = 0;
    return 0;
  }
  void close()
  {
    if(m_data)
    {
      free(m_data);
    }
    m_data      = 0;
    m_allocated = 0;
    m_used      = 0;
    m_cur       = 0;
  }
  void seek(size_t offset, int pos)
  {
    switch(pos)
    {
      case SEEK_CUR:
        m_cur += offset;
        break;
      case SEEK_SET:
        m_cur = offset;
        break;
      case SEEK_END:
        m_cur = m_used;
        break;
    }
  }
  void write(const void* data, size_t dataSize)
  {
    if(m_cur + dataSize > m_used)
    {
      m_used = m_cur + dataSize;
    }

    if(m_cur + dataSize > m_allocated)
    {
      size_t add = m_allocated * 2;
      if(add < dataSize)
        add = dataSize;

      size_t chunk = 1024 * 1024 * 128;
      if(add > chunk && dataSize < chunk)
      {
        add = chunk;
      }
      m_data = (char*)realloc(m_data, m_allocated + add);
      m_allocated += add;
    }
    memcpy(m_data + m_cur, data, dataSize);
    m_cur += dataSize;
  }
};


#if CSF_ZIP_SUPPORT
struct OutputGZ
{
  gzFile    m_file;
  OutputBuf m_buf;

  int open(const char* filename)
  {
    m_buf.open(filename);
    m_file = gzopen(filename, "wb");
    return m_file == 0;
  }
  void close()
  {
    gzwrite(m_file, m_buf.m_data, (z_off_t)m_buf.m_used);
    gzclose(m_file);
    m_buf.close();
  }
  void seek(size_t offset, int pos) { m_buf.seek(offset, pos); }
  void write(const void* data, size_t dataSize) { m_buf.write(data, dataSize); }
};
#endif

template <class T>
struct CSFOffsetMgr
{
  struct Entry
  {
    CSFoffset offset;
    CSFoffset location;
  };
  T&                 m_file;
  std::vector<Entry> m_offsetLocations;
  size_t             m_current;


  CSFOffsetMgr(T& file)
      : m_current(0)
      , m_file(file)
  {
  }

  size_t store(const void* data, size_t dataSize)
  {
    size_t last = m_current;
    m_file.write(data, dataSize);

    m_current += dataSize;
    return last;
  }

  size_t store(size_t location, const void* data, size_t dataSize)
  {
    size_t last = m_current;
    m_file.write(data, dataSize);

    m_current += dataSize;

    Entry entry = {last, location};
    m_offsetLocations.push_back(entry);

    return last;
  }

  void finalize(size_t tableCountLocation, size_t tableLocation)
  {
    m_file.seek(tableCountLocation, SEEK_SET);
    int num = int(m_offsetLocations.size());
    m_file.write(&num, sizeof(int));

    CSFoffset offset = (CSFoffset)m_current;
    m_file.seek(tableLocation, SEEK_SET);
    m_file.write(&offset, sizeof(CSFoffset));

    for(size_t i = 0; i < m_offsetLocations.size(); i++)
    {
      m_file.seek(m_offsetLocations[i].location, SEEK_SET);
      m_file.write(&m_offsetLocations[i].offset, sizeof(CSFoffset));
    }

    // dump table
    m_file.seek(0, SEEK_END);
    for(size_t i = 0; i < m_offsetLocations.size(); i++)
    {
      m_file.write(&m_offsetLocations[i].location, sizeof(CSFoffset));
    }
  }
};

template <class T>
static int CSFile_saveInternal(const CSFile* csf, const char* filename)
{
  T file;
  if(file.open(filename))
  {
    return CADSCENEFILE_ERROR_NOFILE;
  }

  CSFOffsetMgr<T> mgr(file);

  CSFile dump = {0};
  memcpy(&dump, csf, CSFile_getHeaderSize(csf));

  dump.version = CADSCENEFILE_VERSION;
  dump.magic   = CADSCENEFILE_MAGIC;
  // dump main part as is
  mgr.store(&dump, sizeof(CSFile));

  // iterate the objects

  {
    size_t geomOFFSET = mgr.store(offsetof(CSFile, geometriesOFFSET), csf->geometries, sizeof(CSFGeometry) * csf->numGeometries);

    for(int i = 0; i < csf->numGeometries; i++, geomOFFSET += sizeof(CSFGeometry))
    {
      const CSFGeometry* geo = csf->geometries + i;

      if(geo->vertex && geo->numVertices)
      {
        mgr.store(geomOFFSET + offsetof(CSFGeometry, vertexOFFSET), geo->vertex, sizeof(float) * 3 * geo->numVertices);
      }
      if(geo->normal && geo->numVertices)
      {
        mgr.store(geomOFFSET + offsetof(CSFGeometry, normalOFFSET), geo->normal,
                  sizeof(float) * 3 * geo->numVertices * geo->numNormalChannels);
      }
      if(geo->tex && geo->numVertices)
      {
        mgr.store(geomOFFSET + offsetof(CSFGeometry, texOFFSET), geo->tex, sizeof(float) * 2 * geo->numVertices * geo->numTexChannels);
      }

      if(geo->aux && geo->numVertices)
      {
        mgr.store(geomOFFSET + offsetof(CSFGeometry, auxOFFSET), geo->aux, sizeof(float) * 4 * geo->numVertices * geo->numAuxChannels);
      }
      if(geo->auxStorageOrder && geo->numAuxChannels)
      {
        mgr.store(geomOFFSET + offsetof(CSFGeometry, auxStorageOrderOFFSET), geo->auxStorageOrder,
                  sizeof(CSFGeometryAuxChannel) * geo->numAuxChannels);
      }

      if(geo->indexSolid && geo->numIndexSolid)
      {
        mgr.store(geomOFFSET + offsetof(CSFGeometry, indexSolidOFFSET), geo->indexSolid, sizeof(int) * geo->numIndexSolid);
      }
      if(geo->indexWire && geo->numIndexWire)
      {
        mgr.store(geomOFFSET + offsetof(CSFGeometry, indexWireOFFSET), geo->indexWire, sizeof(int) * geo->numIndexWire);
      }

      if(geo->perpartStorageOrder && geo->numPartChannels)
      {
        mgr.store(geomOFFSET + offsetof(CSFGeometry, perpartStorageOrder), geo->perpartStorageOrder,
                  sizeof(CSFGeometryPartChannel) * geo->numPartChannels);
      }
      if(geo->perpart && geo->numPartChannels)
      {
        mgr.store(geomOFFSET + offsetof(CSFGeometry, perpart), geo->perpart, CSFGeometry_getPerPartSize(geo));
      }

      if(geo->parts && geo->numParts)
      {
        mgr.store(geomOFFSET + offsetof(CSFGeometry, partsOFFSET), geo->parts, sizeof(CSFGeometryPart) * geo->numParts);
      }
    }
  }


  {
    size_t matOFFSET = mgr.store(offsetof(CSFile, materialsOFFSET), csf->materials, sizeof(CSFMaterial) * csf->numMaterials);

    for(int i = 0; i < csf->numMaterials; i++, matOFFSET += sizeof(CSFMaterial))
    {
      const CSFMaterial* mat = csf->materials + i;
      if(mat->bytes && mat->numBytes)
      {
        mgr.store(matOFFSET + offsetof(CSFMaterial, bytesOFFSET), mat->bytes, sizeof(unsigned char) * mat->numBytes);
      }
    }
  }

  {
    size_t nodeOFFSET = mgr.store(offsetof(CSFile, nodesOFFSET), csf->nodes, sizeof(CSFNode) * csf->numNodes);

    for(int i = 0; i < csf->numNodes; i++, nodeOFFSET += sizeof(CSFNode))
    {
      const CSFNode* node = csf->nodes + i;
      if(node->parts && node->numParts)
      {
        mgr.store(nodeOFFSET + offsetof(CSFNode, partsOFFSET), node->parts, sizeof(CSFNodePart) * node->numParts);
      }
      if(node->children && node->numChildren)
      {
        mgr.store(nodeOFFSET + offsetof(CSFNode, childrenOFFSET), node->children, sizeof(int) * node->numChildren);
      }
    }
  }

  if(CSFile_getNodeMetas(csf))
  {
    size_t metaOFFSET = mgr.store(offsetof(CSFile, nodeMetasOFFSET), csf->nodeMetas, sizeof(CSFMeta) * csf->numNodes);

    for(int i = 0; i < csf->numNodes; i++, metaOFFSET += sizeof(CSFMeta))
    {
      const CSFMeta* meta = csf->nodeMetas + i;
      if(meta->bytes && meta->numBytes)
      {
        mgr.store(metaOFFSET + offsetof(CSFMeta, bytesOFFSET), meta->bytes, sizeof(unsigned char) * meta->numBytes);
      }
    }
  }

  if(CSFile_getGeometryMetas(csf))
  {
    size_t metaOFFSET = mgr.store(offsetof(CSFile, geometryMetasOFFSET), csf->geometryMetas, sizeof(CSFMeta) * csf->numGeometries);

    for(int i = 0; i < csf->numNodes; i++, metaOFFSET += sizeof(CSFMeta))
    {
      const CSFMeta* meta = csf->geometryMetas + i;
      if(meta->bytes && meta->numBytes)
      {
        mgr.store(metaOFFSET + offsetof(CSFMeta, bytesOFFSET), meta->bytes, sizeof(unsigned char) * meta->numBytes);
      }
    }
  }

  if(CSFile_getFileMeta(csf))
  {
    size_t metaOFFSET = mgr.store(offsetof(CSFile, fileMetaOFFSET), csf->fileMeta, sizeof(CSFMeta));

    {
      const CSFMeta* meta = csf->fileMeta;
      if(meta->bytes && meta->numBytes)
      {
        mgr.store(metaOFFSET + offsetof(CSFMeta, bytesOFFSET), meta->bytes, sizeof(unsigned char) * meta->numBytes);
      }
    }
  }

  mgr.finalize(offsetof(CSFile, numPointers), offsetof(CSFile, pointersOFFSET));

  file.close();

  return CADSCENEFILE_NOERROR;
}

CSFAPI int CSFile_save(const CSFile* csf, const char* filename)
{
  return CSFile_saveInternal<OutputFILE>(csf, filename);
}

#if CSF_ZIP_SUPPORT
CSFAPI int CSFile_saveExt(CSFile* csf, const char* filename)
{
  size_t len = strlen(filename);
  if(strcmp(filename + len - 3, ".gz") == 0)
  {
    return CSFile_saveInternal<OutputGZ>(csf, filename);
  }
  else
  {
    return CSFile_saveInternal<OutputFILE>(csf, filename);
  }
}

#endif

static NV_FORCE_INLINE void Matrix44Copy(float* NV_RESTRICT dst, const float* NV_RESTRICT a)
{
  memcpy(dst, a, sizeof(float) * 16);
}

static NV_FORCE_INLINE void Matrix44MultiplyFull(float* NV_RESTRICT clip, const float* NV_RESTRICT proj, const float* NV_RESTRICT modl)
{

  clip[0] = modl[0] * proj[0] + modl[1] * proj[4] + modl[2] * proj[8] + modl[3] * proj[12];
  clip[1] = modl[0] * proj[1] + modl[1] * proj[5] + modl[2] * proj[9] + modl[3] * proj[13];
  clip[2] = modl[0] * proj[2] + modl[1] * proj[6] + modl[2] * proj[10] + modl[3] * proj[14];
  clip[3] = modl[0] * proj[3] + modl[1] * proj[7] + modl[2] * proj[11] + modl[3] * proj[15];

  clip[4] = modl[4] * proj[0] + modl[5] * proj[4] + modl[6] * proj[8] + modl[7] * proj[12];
  clip[5] = modl[4] * proj[1] + modl[5] * proj[5] + modl[6] * proj[9] + modl[7] * proj[13];
  clip[6] = modl[4] * proj[2] + modl[5] * proj[6] + modl[6] * proj[10] + modl[7] * proj[14];
  clip[7] = modl[4] * proj[3] + modl[5] * proj[7] + modl[6] * proj[11] + modl[7] * proj[15];

  clip[8]  = modl[8] * proj[0] + modl[9] * proj[4] + modl[10] * proj[8] + modl[11] * proj[12];
  clip[9]  = modl[8] * proj[1] + modl[9] * proj[5] + modl[10] * proj[9] + modl[11] * proj[13];
  clip[10] = modl[8] * proj[2] + modl[9] * proj[6] + modl[10] * proj[10] + modl[11] * proj[14];
  clip[11] = modl[8] * proj[3] + modl[9] * proj[7] + modl[10] * proj[11] + modl[11] * proj[15];

  clip[12] = modl[12] * proj[0] + modl[13] * proj[4] + modl[14] * proj[8] + modl[15] * proj[12];
  clip[13] = modl[12] * proj[1] + modl[13] * proj[5] + modl[14] * proj[9] + modl[15] * proj[13];
  clip[14] = modl[12] * proj[2] + modl[13] * proj[6] + modl[14] * proj[10] + modl[15] * proj[14];
  clip[15] = modl[12] * proj[3] + modl[13] * proj[7] + modl[14] * proj[11] + modl[15] * proj[15];
}

static void CSFile_transformHierarchy(CSFile* csf, CSFNode* NV_RESTRICT node, CSFNode* NV_RESTRICT parent)
{
  if(parent)
  {
    Matrix44MultiplyFull(node->worldTM, parent->worldTM, node->objectTM);
  }
  else
  {
    Matrix44Copy(node->worldTM, node->objectTM);
  }

  for(int i = 0; i < node->numChildren; i++)
  {
    CSFNode* NV_RESTRICT child = csf->nodes + node->children[i];
    CSFile_transformHierarchy(csf, child, node);
  }
}

CSFAPI int CSFile_transform(CSFile* csf)
{
  if(!(csf->fileFlags & CADSCENEFILE_FLAG_UNIQUENODES))
    return CADSCENEFILE_ERROR_OPERATION;

  CSFile_transformHierarchy(csf, csf->nodes + csf->rootIDX, nullptr);
  return CADSCENEFILE_NOERROR;
}

CSFAPI const CSFMeta* CSFile_getNodeMetas(const CSFile* csf)
{
  if(csf->version >= CADSCENEFILE_VERSION_META && csf->fileFlags & CADSCENEFILE_FLAG_META_NODE)
  {
    return csf->nodeMetas;
  }

  return nullptr;
}

CSFAPI const CSFMeta* CSFile_getGeometryMetas(const CSFile* csf)
{
  if(csf->version >= CADSCENEFILE_VERSION_META && csf->fileFlags & CADSCENEFILE_FLAG_META_GEOMETRY)
  {
    return csf->geometryMetas;
  }

  return nullptr;
}


CSFAPI const CSFMeta* CSFile_getFileMeta(const CSFile* csf)
{
  if(csf->version >= CADSCENEFILE_VERSION_META && csf->fileFlags & CADSCENEFILE_FLAG_META_FILE)
  {
    return csf->fileMeta;
  }

  return nullptr;
}


CSFAPI const CSFBytePacket* CSFile_getBytePacket(const unsigned char* bytes, CSFoffset numBytes, CSFGuid guid)
{
  if(numBytes < sizeof(CSFBytePacket))
    return nullptr;

  do
  {
    const CSFBytePacket* packet = (const CSFBytePacket*)bytes;
    if(memcmp(guid, packet->guid, sizeof(CSFGuid)) == 0)
    {
      return packet;
    }
    numBytes -= packet->numBytes;
    bytes += packet->numBytes;

  } while(numBytes >= sizeof(CSFBytePacket));

  return nullptr;
}

CSFAPI const CSFBytePacket* CSFile_getMetaBytePacket(const CSFMeta* meta, CSFGuid guid)
{
  return CSFile_getBytePacket(meta->bytes, meta->numBytes, guid);
}

CSFAPI const CSFBytePacket* CSFile_getMaterialBytePacket(const CSFile* csf, int materialIDX, CSFGuid guid)
{
  if(materialIDX < 0 || materialIDX >= csf->numMaterials)
  {
    return nullptr;
  }

  return CSFile_getBytePacket(csf->materials[materialIDX].bytes, csf->materials[materialIDX].numBytes, guid);
}

CSFAPI void CSFMatrix_identity(float* matrix)
{
  memset(matrix, 0, sizeof(float) * 16);
  matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
}

CSFAPI void CSFile_clearDeprecated(CSFile* csf)
{
  for(int g = 0; g < csf->numGeometries; g++)
  {
    memset(csf->geometries[g]._deprecated, 0, sizeof(csf->geometries[g]._deprecated));
    for(int p = 0; p < csf->geometries[g].numParts; p++)
    {
      csf->geometries[g].parts[p]._deprecated = 0;
    }
  }
}

CSFAPI void CSFGeometry_setupDefaultChannels(CSFGeometry* geo)
{
  geo->numNormalChannels = geo->normal ? 1 : 0;
  geo->numTexChannels    = geo->tex ? 1 : 0;
  geo->numAuxChannels    = 0;
  geo->numPartChannels   = 0;
  geo->aux               = nullptr;
  geo->auxStorageOrder   = nullptr;
  geo->perpart           = nullptr;
}

CSFAPI void CSFile_setupDefaultChannels(CSFile* csf)
{
  for(int g = 0; g < csf->numGeometries; g++)
  {
    CSFGeometry_setupDefaultChannels(csf->geometries + g);
  }
}

CSFAPI const float* CSFGeometry_getNormalChannel(const CSFGeometry* geo, CSFGeometryNormalChannel channel)
{
  return channel < geo->numNormalChannels ? geo->normal + size_t(geo->numVertices * 3 * channel) : nullptr;
}

CSFAPI const float* CSFGeometry_getTexChannel(const CSFGeometry* geo, CSFGeometryTexChannel channel)
{
  return channel < geo->numTexChannels ? geo->tex + size_t(geo->numVertices * 2 * channel) : nullptr;
}

CSFAPI const float* CSFGeometry_getAuxChannel(const CSFGeometry* geo, CSFGeometryAuxChannel channel)
{
  for(int i = 0; i < geo->numAuxChannels; i++)
  {
    if(geo->auxStorageOrder[i] == channel)
    {
      return geo->aux + size_t(geo->numVertices * 4 * i);
    }
  }

  return nullptr;
}

CSFAPI size_t CSFGeometryPartChannel_getSize(CSFGeometryPartChannel channel)
{
  size_t sizes[CSFGEOMETRY_PARTCHANNELS];
  sizes[CSFGEOMETRY_PARTCHANNEL_BBOX] = sizeof(CSFGeometryPartBbox);

  return sizes[channel];
}

CSFAPI size_t CSFGeometry_getPerPartSize(const CSFGeometry* geo)
{
  size_t size = 0;
  for(int i = 0; i < geo->numPartChannels; i++)
  {
    size += CSFGeometryPartChannel_getSize(geo->perpartStorageOrder[i]) * geo->numParts;
  }
  return size;
}

CSFAPI size_t CSFGeometry_getPerPartRequiredSize(const CSFGeometry* geo, int numParts)
{
  size_t size = 0;
  for(int i = 0; i < geo->numPartChannels; i++)
  {
    size += CSFGeometryPartChannel_getSize(geo->perpartStorageOrder[i]) * numParts;
  }
  return size;
}

CSFAPI size_t CSFGeometry_getPerPartRequiredOffset(const CSFGeometry* geo, int numParts, CSFGeometryPartChannel channel)
{
  size_t offset = 0;
  for(int i = 0; i < geo->numPartChannels; i++)
  {
    if(geo->perpartStorageOrder[i] == channel)
    {
      return offset;
    }
    offset += CSFGeometryPartChannel_getSize(geo->perpartStorageOrder[i]) * numParts;
  }
  return ~0ull;
}

CSFAPI const void* CSFGeometry_getPartChannel(const CSFGeometry* geo, CSFGeometryPartChannel channel)
{
  size_t offset = CSFGeometry_getPerPartRequiredOffset(geo, geo->numParts, channel);
  if(offset != ~0ull)
  {
    return geo->perpart + offset;
  }

  return nullptr;
}


#if CSF_GLTF_SUPPORT

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE

#include "tiny_gltf.h"

#include <nvmath/nvmath.h>

void CSFile_countGLTFNode(CSFile* csf, const tinygltf::Model& gltfModel, int nodeIdx)
{
  const tinygltf::Node& node = gltfModel.nodes[nodeIdx];

  csf->numNodes++;
  for(int child : node.children)
  {
    CSFile_countGLTFNode(csf, gltfModel, child);
  }
}

int CSFile_addGLTFNode(CSFile* csf, const tinygltf::Model& gltfModel, const uint32_t* meshGeometries, CSFileMemoryPTR mem, int nodeIdx)
{
  const tinygltf::Node& node = gltfModel.nodes[nodeIdx];

  int idx = csf->numNodes++;

  CSFNode& csfnode = csf->nodes[idx];
  CSFMatrix_identity(csfnode.worldTM);
  CSFMatrix_identity(csfnode.objectTM);

  if(node.matrix.size() == 16)
  {
    for(int i = 0; i < 16; i++)
    {
      csfnode.objectTM[i] = (float)node.matrix[i];
    }
  }
  else
  {
    nvmath::vec3f translation = {0, 0, 0};
    nvmath::quatf rotation    = {0, 0, 0, 0};
    nvmath::vec3f scale       = {1, 1, 1};

    if(node.translation.size() == 3)
    {
      translation.x = static_cast<float>(node.translation[0]);
      translation.y = static_cast<float>(node.translation[1]);
      translation.z = static_cast<float>(node.translation[2]);
    }

    if(node.rotation.size() == 4)
    {
      rotation.x = static_cast<float>(node.rotation[0]);
      rotation.y = static_cast<float>(node.rotation[1]);
      rotation.z = static_cast<float>(node.rotation[2]);
      rotation.w = static_cast<float>(node.rotation[3]);
    }

    if(node.scale.size() == 3)
    {
      scale.x = static_cast<float>(node.scale[0]);
      scale.y = static_cast<float>(node.scale[1]);
      scale.z = static_cast<float>(node.scale[2]);
    }


    nvmath::mat4f mtranslation, mscale, mrot;
    nvmath::quatf mrotation;
    mtranslation.as_translation(translation);
    mscale.as_scale(scale);
    rotation.to_matrix(mrot);

    nvmath::mat4f matrix = mtranslation * mrot * mscale;
    for(int i = 0; i < 16; i++)
    {
      csfnode.objectTM[i] = matrix.mat_array[i];
    }
  }

  // setup geometry
  if(node.mesh > -1)
  {
    csfnode.geometryIDX        = meshGeometries[node.mesh];
    const tinygltf::Mesh& mesh = gltfModel.meshes[node.mesh];

    csfnode.numParts = csf->geometries[csfnode.geometryIDX].numParts;
    csfnode.parts    = (CSFNodePart*)CSFileMemory_alloc(mem, sizeof(CSFNodePart) * csfnode.numParts, nullptr);

    int i = 0;
    for(const tinygltf::Primitive& primitive : mesh.primitives)
    {
      if(primitive.mode != TINYGLTF_MODE_TRIANGLES)
        continue;

      CSFNodePart& csfpart = csfnode.parts[i++];

      csfpart.active      = 1;
      csfpart.materialIDX = primitive.material;
      csfpart.nodeIDX     = -1;
    }
  }
  else
  {
    csfnode.geometryIDX = -1;
  }

  csfnode.numChildren = (int)node.children.size();
  csfnode.children    = (int*)CSFileMemory_alloc(mem, sizeof(int) * csfnode.numChildren, nullptr);

  int childIdx = 0;
  for(int child : node.children)
  {
    csfnode.children[childIdx++] = CSFile_addGLTFNode(csf, gltfModel, meshGeometries, mem, child);
  }

  return idx;
}

#if CSF_GLTF_UNIQUE_GEOMETRIES
//-----------------------------------------------------------------------------
// MurmurHash2A, by Austin Appleby

// This is a variant of MurmurHash2 modified to use the Merkle-Damgard
// construction. Bulk speed should be identical to Murmur2, small-key speed
// will be 10%-20% slower due to the added overhead at the end of the hash.

// This variant fixes a minor issue where null keys were more likely to
// collide with each other than expected, and also makes the algorithm
// more amenable to incremental implementations. All other caveats from
// MurmurHash2 still apply.

#define mmix(h, k)                                                                                                     \
  {                                                                                                                    \
    k *= m;                                                                                                            \
    k ^= k >> r;                                                                                                       \
    k *= m;                                                                                                            \
    h *= m;                                                                                                            \
    h ^= k;                                                                                                            \
  }

static unsigned int strMurmurHash2A(const void* key, size_t len, unsigned int seed)
{
  const unsigned int m = 0x5bd1e995;
  const int          r = 24;
  unsigned int       l = (unsigned int)len;

  const unsigned char* data = (const unsigned char*)key;

  unsigned int h = seed;
  unsigned int t = 0;

  while(len >= 4)
  {
    unsigned int k = *(unsigned int*)data;

    mmix(h, k);

    data += 4;
    len -= 4;
  }


  switch(len)
  {
    case 3:
      t ^= data[2] << 16;
    case 2:
      t ^= data[1] << 8;
    case 1:
      t ^= data[0];
  };

  mmix(h, t);
  mmix(h, l);

  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;

  return h;
}
#undef mmix

struct GLTFGeometryInfo
{
  uint32_t numVertices = 0;
  uint32_t numNormals  = 0;
  uint32_t numIndices  = 0;
  uint32_t numParts    = 0;
  
  uint32_t hashIndex  = 0;
  uint32_t hashVertex = 0;
  uint32_t hashNormal = 0;

  bool isEqualLight(const GLTFGeometryInfo& other)
  {
    return numVertices == other.numVertices && numNormals == other.numNormals && numIndices == other.numIndices
           && numParts == other.numParts;
  }

  bool isEqualHash(const GLTFGeometryInfo& other)
  {
    return hashIndex == other.hashIndex && hashVertex == other.hashVertex && hashNormal == other.hashNormal;
  }

  void setup(const tinygltf::Model& gltfModel, const tinygltf::Mesh& mesh)
  {
    for(const tinygltf::Primitive& primitive : mesh.primitives)
    {
      if(primitive.mode != TINYGLTF_MODE_TRIANGLES)
        continue;

      const tinygltf::Accessor& posAccessor = gltfModel.accessors[primitive.attributes.find("POSITION")->second];
      numVertices += static_cast<uint32_t>(posAccessor.count);

      if(primitive.attributes.find("NORMAL") != primitive.attributes.end())
      {
        numNormals += static_cast<uint32_t>(posAccessor.count);
      }

      const tinygltf::Accessor& indexAccessor = gltfModel.accessors[primitive.indices];
      numIndices += static_cast<uint32_t>(indexAccessor.count);
      numParts++;
    }
  }

  bool hasHash() const { return hashIndex != 0 || hashVertex != 0 || hashNormal != 0; }

  void setupHash(const tinygltf::Model& gltfModel, const tinygltf::Mesh& mesh)
  {
    for(const tinygltf::Primitive& primitive : mesh.primitives)
    {
      if(primitive.mode != TINYGLTF_MODE_TRIANGLES)
        continue;

      const tinygltf::Accessor& posAccessor = gltfModel.accessors[primitive.attributes.find("POSITION")->second];
      {
        const tinygltf::Accessor&   accessor = posAccessor;
        const tinygltf::BufferView& view     = gltfModel.bufferViews[accessor.bufferView];
        const float*                buffer =
            reinterpret_cast<const float*>(&(gltfModel.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));

        hashVertex = strMurmurHash2A(buffer, sizeof(float) * 3 * accessor.count, hashVertex);
      }

      const auto normalit = primitive.attributes.find("NORMAL");
      if(normalit != primitive.attributes.end())
      {
        const tinygltf::Accessor&   accessor = gltfModel.accessors[normalit->second];
        const tinygltf::BufferView& view     = gltfModel.bufferViews[accessor.bufferView];
        const float*                buffer =
            reinterpret_cast<const float*>(&(gltfModel.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));

        hashNormal = strMurmurHash2A(buffer, sizeof(float) * 3 * accessor.count, hashNormal);
      }

      const tinygltf::Accessor& indexAccessor = gltfModel.accessors[primitive.indices];
      {
        const tinygltf::Accessor&   accessor = indexAccessor;
        const tinygltf::BufferView& view     = gltfModel.bufferViews[accessor.bufferView];
        const void*                 buffer =
            reinterpret_cast<const void*>(&(gltfModel.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));

        size_t indexSize = 0;
        switch(accessor.componentType)
        {
          case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
            indexSize = sizeof(uint32_t);
            break;
          case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
            indexSize = sizeof(uint16_t);
            break;
          case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
            indexSize = sizeof(uint8_t);
            break;
        }

        hashIndex = strMurmurHash2A(buffer, indexSize * accessor.count, hashIndex);
      }
    }
  }
};
#endif

CSFAPI int CSFile_loadGTLF(CSFile** outcsf, const char* filename, CSFileMemoryPTR mem)
{
  if(!filename)
  {
    return CADSCENEFILE_ERROR_NOFILE;
  }

  tinygltf::TinyGLTF loader;
  tinygltf::Model    gltfModel;
  std::string        err;
  std::string        warn;

  *outcsf = NULL;

  bool ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filename);
  if(!warn.empty())
  {
    printf("Warn: %s\n", warn.c_str());
  }

  if(!err.empty())
  {
    printf("ERR: %s\n", err.c_str());
  }
  if(!ret)
  {
    return CADSCENEFILE_ERROR_OPERATION;
  }

  CSFile* csf = (CSFile*)CSFileMemory_alloc(mem, sizeof(CSFile), NULL);
  memset(csf, 0, sizeof(CSFile));
  csf->version   = CADSCENEFILE_VERSION;
  csf->fileFlags = 0;
  csf->fileFlags |= CADSCENEFILE_FLAG_UNIQUENODES;
  csf->numMaterials = (int)gltfModel.materials.size();
  csf->numNodes     = (int)gltfModel.nodes.size();

  csf->materials = (CSFMaterial*)CSFileMemory_alloc(mem, sizeof(CSFMaterial) * csf->numMaterials, NULL);
  memset(csf->materials, 0, sizeof(CSFMaterial) * csf->numMaterials);

  // create materials
  size_t materialIdx = 0;

  for(tinygltf::Material& mat : gltfModel.materials)
  {
    CSFMaterial& csfmat = csf->materials[materialIdx++];

    if(mat.values.find("baseColorFactor") != mat.values.end())
    {
      tinygltf::ColorValue cv = mat.values["baseColorFactor"].ColorFactor();
      csfmat.color[0]         = static_cast<float>(cv[0]);
      csfmat.color[1]         = static_cast<float>(cv[1]);
      csfmat.color[2]         = static_cast<float>(cv[2]);
      csfmat.color[3]         = static_cast<float>(cv[3]);
    }
    else
    {
      csfmat.color[0] = 1.0f;
      csfmat.color[1] = 1.0f;
      csfmat.color[2] = 1.0f;
      csfmat.color[3] = 1.0f;
    }

    strncpy(csfmat.name, mat.name.c_str(), sizeof(csfmat.name));
  }

  // find unique geometries
  // many gltf files make improper use of geometry instancing

  std::vector<uint32_t>         meshGeometries;
  std::vector<uint32_t>         geometryMeshes;

  meshGeometries.reserve(gltfModel.meshes.size());
  geometryMeshes.reserve(gltfModel.meshes.size());
  

#if CSF_GLTF_UNIQUE_GEOMETRIES
  // use some hashing based comparisons to avoid deep comparisons

  std::vector<GLTFGeometryInfo> geometryInfos;
  geometryInfos.reserve(gltfModel.meshes.size());

  uint32_t meshIdx = 0;
  for(const tinygltf::Mesh& mesh : gltfModel.meshes)
  {
    GLTFGeometryInfo geoInfo;

    geoInfo.setup(gltfModel, mesh);

    // compare against existing hashes
    uint32_t found = ~0;
    for(uint32_t i = 0; i < (uint32_t)geometryInfos.size(); i++)
    {
      if(geoInfo.isEqualLight(geometryInfos[i]))
      {
        if(!geometryInfos[i].hasHash())
        {
          geometryInfos[i].setupHash(gltfModel, gltfModel.meshes[geometryMeshes[i]]);
        }

        geoInfo.setupHash(gltfModel, mesh);

        if(geoInfo.isEqualHash(geometryInfos[i]))
        {
          found = i;
          break;
        }
      }
    }
    if(found != ~0)
    {
      meshGeometries.push_back(found);
    }
    else
    {
      meshGeometries.push_back((uint32_t)geometryInfos.size());
      geometryInfos.push_back(geoInfo);
      geometryMeshes.push_back(meshIdx);
    }
    meshIdx++;
  }
#else 
  // 1:1 Mesh to CSFGeometry
  uint32_t meshIdx = 0;
  for (const tinygltf::Mesh& mesh : gltfModel.meshes)
  {
    meshGeometries.push_back(meshIdx);
    geometryMeshes.push_back(meshIdx);
  }
#endif

  csf->numGeometries = (int)geometryMeshes.size();
  csf->geometries    = (CSFGeometry*)CSFileMemory_alloc(mem, sizeof(CSFGeometry) * csf->numGeometries, NULL);
  memset(csf->geometries, 0, sizeof(CSFGeometry) * csf->numGeometries);


  // create geometries
  for(uint32_t outIdx = 0; outIdx < (uint32_t)csf->numGeometries; outIdx++)
  {
    const tinygltf::Mesh& mesh    = gltfModel.meshes[geometryMeshes[outIdx]];
    CSFGeometry&          csfgeom = csf->geometries[outIdx];

    // count pass
    uint32_t vertexTotCount = 0;
    uint32_t indexTotCount  = 0;
    uint32_t partsTotCount  = 0;

    bool hasNormals = false;
    for(const tinygltf::Primitive& primitive : mesh.primitives)
    {
      if(primitive.mode != TINYGLTF_MODE_TRIANGLES)
        continue;

      const tinygltf::Accessor& posAccessor = gltfModel.accessors[primitive.attributes.find("POSITION")->second];
      vertexTotCount += static_cast<uint32_t>(posAccessor.count);
      const tinygltf::Accessor& indexAccessor = gltfModel.accessors[primitive.indices];
      indexTotCount += static_cast<uint32_t>(indexAccessor.count);

      if(primitive.attributes.find("NORMAL") != primitive.attributes.end())
      {
        hasNormals = true;
      }

      partsTotCount++;
    }

    // allocate all data
    csfgeom.numIndexSolid = indexTotCount;
    csfgeom.numVertices   = vertexTotCount;
    csfgeom.numParts      = partsTotCount;

    csfgeom.vertex = (float*)CSFileMemory_alloc(mem, sizeof(float) * 3 * vertexTotCount, nullptr);
    if(hasNormals)
    {
      csfgeom.normal = (float*)CSFileMemory_alloc(mem, sizeof(float) * 3 * vertexTotCount, nullptr);
    }
    csfgeom.indexSolid = (uint32_t*)CSFileMemory_alloc(mem, sizeof(uint32_t) * indexTotCount, nullptr);
    csfgeom.parts      = (CSFGeometryPart*)CSFileMemory_alloc(mem, sizeof(CSFGeometryPart) * partsTotCount, nullptr);

    // fill pass
    indexTotCount  = 0;
    vertexTotCount = 0;
    partsTotCount  = 0;

    for(const tinygltf::Primitive& primitive : mesh.primitives)
    {
      if(primitive.mode != TINYGLTF_MODE_TRIANGLES)
        continue;

      CSFGeometryPart& csfpart = csfgeom.parts[partsTotCount++];

      const tinygltf::Accessor& posAccessor = gltfModel.accessors[primitive.attributes.find("POSITION")->second];

      {
        const tinygltf::Accessor&   accessor = posAccessor;
        const tinygltf::BufferView& view     = gltfModel.bufferViews[accessor.bufferView];
        const float*                buffer =
            reinterpret_cast<const float*>(&(gltfModel.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));

        memcpy(&csfgeom.vertex[vertexTotCount * 3], buffer, sizeof(float) * 3 * accessor.count);
      }

      const auto normalit = primitive.attributes.find("NORMAL");
      if(normalit != primitive.attributes.end())
      {
        const tinygltf::Accessor&   accessor = gltfModel.accessors[normalit->second];
        const tinygltf::BufferView& view     = gltfModel.bufferViews[accessor.bufferView];
        const float*                buffer =
            reinterpret_cast<const float*>(&(gltfModel.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));

        memcpy(&csfgeom.normal[vertexTotCount * 3], buffer, sizeof(float) * 3 * accessor.count);
      }

      {
        const tinygltf::Accessor&   accessor = gltfModel.accessors[primitive.indices];
        const tinygltf::BufferView& view     = gltfModel.bufferViews[accessor.bufferView];
        const void*                 data =
            reinterpret_cast<const void*>(&(gltfModel.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));

        uint32_t indexCount   = static_cast<uint32_t>(accessor.count);
        csfpart.numIndexSolid = indexCount;

        switch(accessor.componentType)
        {
          case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
          {
            const uint32_t* buffer = reinterpret_cast<const uint32_t*>(data);

            for(uint32_t i = 0; i < indexCount; i++)
            {
              csfgeom.indexSolid[indexTotCount + i] = buffer[i] + vertexTotCount;
            }
            break;
          }
          case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
          {
            const uint16_t* buffer = reinterpret_cast<const uint16_t*>(data);

            for(uint32_t i = 0; i < indexCount; i++)
            {
              csfgeom.indexSolid[indexTotCount + i] = (uint32_t)buffer[i] + vertexTotCount;
            }

            break;
          }
          case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
          {
            const uint8_t* buffer = reinterpret_cast<const uint8_t*>(data);
            for(uint32_t i = 0; i < indexCount; i++)
            {
              csfgeom.indexSolid[indexTotCount + i] = (uint32_t)buffer[i] + vertexTotCount;
            }
            break;
          }
          default:
            return CADSCENEFILE_ERROR_OPERATION;
        }

        indexTotCount += indexCount;
      }

      vertexTotCount += static_cast<uint32_t>(posAccessor.count);

      csfpart.numIndexWire = 0;
      csfpart._deprecated  = 0;
    }

    CSFGeometry_setupDefaultChannels(&csfgeom);
  }

  // create flattened nodes
  csf->numNodes = 1;  // reserve for root
  csf->rootIDX  = 0;


  tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];
  for(size_t i = 0; i < scene.nodes.size(); i++)
  {
    CSFile_countGLTFNode(csf, gltfModel, scene.nodes[i]);
  }

  csf->nodes = (CSFNode*)CSFileMemory_alloc(mem, sizeof(CSFNode) * csf->numNodes, nullptr);
  memset(csf->nodes, 0, sizeof(CSFNode) * csf->numNodes);

  csf->numNodes = 1;
  // root setup
  csf->nodes[0].geometryIDX = -1;
  csf->nodes[0].numChildren = (int)scene.nodes.size();
  csf->nodes[0].children    = (int*)CSFileMemory_alloc(mem, sizeof(int) * scene.nodes.size(), nullptr);
  CSFMatrix_identity(csf->nodes[0].worldTM);
  CSFMatrix_identity(csf->nodes[0].objectTM);

  for(size_t i = 0; i < scene.nodes.size(); i++)
  {
    csf->nodes[0].children[i] = CSFile_addGLTFNode(csf, gltfModel, meshGeometries.data(), mem, scene.nodes[i]);
  }

  CSFile_transform(csf);


  *outcsf = csf;
  return CADSCENEFILE_NOERROR;
}

#endif
