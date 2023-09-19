/*
 * Copyright (c) 2011-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2011-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if CSF_SUPPORT_ZLIB
#include <zlib.h>
#endif

#include <algorithm>
#include <map>
#include <mutex>
#include <thread>
#include <vector>


#define CADSCENEFILE_MAGIC 1567262451

#ifdef WIN32
#define xfreads(buf, bufsz, sz, count, f) fread_s(buf, bufsz, sz, count, f)
#else
#define xfreads(buf, bufsz, sz, count, f) fread(buf, sz, count, f)
#endif

#if defined(WIN32) && (defined(__amd64__) || defined(__x86_64__) || defined(_M_X64) || defined(__AMD64__))
#define xftell(f) _ftelli64(f)
#define xfseek(f, pos, encoded) _fseeki64(f, pos, encoded)
#else
#define xftell(f) ftell(f)
#define xfseek(f, pos, encoded) fseek(f, (long)pos, encoded)
#endif

#if defined(LINUX)
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

//////////////////////////////////////////////////////////////////////////
// FILEMAPPING

namespace csfutils {

class FileMapping
{
public:
  FileMapping(FileMapping&& other) noexcept { this->operator=(std::move(other)); };

  FileMapping& operator=(FileMapping&& other)
  {
    m_isValid     = other.m_isValid;
    m_fileSize    = other.m_fileSize;
    m_mappingType = other.m_mappingType;
    m_mappingPtr  = other.m_mappingPtr;
    m_mappingSize = other.m_mappingSize;
#ifdef _WIN32
    m_win32.file              = other.m_win32.file;
    m_win32.fileMapping       = other.m_win32.fileMapping;
    other.m_win32.file        = nullptr;
    other.m_win32.fileMapping = nullptr;
#else
    m_unix.file       = other.m_unix.file;
    other.m_unix.file = -1;
#endif
    other.m_isValid    = false;
    other.m_mappingPtr = nullptr;

    return *this;
  }

  FileMapping(const FileMapping&)                  = delete;
  FileMapping& operator=(const FileMapping& other) = delete;
  FileMapping() {}

  ~FileMapping() { close(); }

  enum MappingType
  {
    MAPPING_READONLY,       // opens existing file for read-only access
    MAPPING_READOVERWRITE,  // creates new file with read/write access, overwriting existing files
  };

  // fileSize only for write access
  bool open(const char* filename, MappingType mappingType, size_t fileSize = 0);
  void close();

  const void* data() const { return m_mappingPtr; }
  void*       data() { return m_mappingPtr; }
  size_t      size() const { return m_mappingSize; }
  bool        valid() const { return m_isValid; }

protected:
  static size_t g_pageSize;

#ifdef _WIN32
  struct
  {
    void* file        = nullptr;
    void* fileMapping = nullptr;
  } m_win32;
#else
  struct
  {
    int file = -1;
  } m_unix;
#endif

  bool        m_isValid  = false;
  size_t      m_fileSize = 0;
  MappingType m_mappingType;
  void*       m_mappingPtr  = nullptr;
  size_t      m_mappingSize = 0;
};

// convenience types
class FileReadMapping : private FileMapping
{
public:
  bool        open(const char* filename) { return FileMapping::open(filename, MAPPING_READONLY, 0); }
  void        close() { FileMapping::close(); }
  const void* data() const { return m_mappingPtr; }
  size_t      size() const { return m_fileSize; }
  bool        valid() const { return m_isValid; }
};

class FileReadOverWriteMapping : private FileMapping
{
public:
  bool open(const char* filename, size_t fileSize)
  {
    return FileMapping::open(filename, MAPPING_READOVERWRITE, fileSize);
  }
  void   close() { FileMapping::close(); }
  void*  data() { return m_mappingPtr; }
  size_t size() const { return m_fileSize; }
  bool   valid() const { return m_isValid; }
};
}  // namespace csfutils

//////////////////////////////////////////////////////////////////////////
// GENERICS

static inline void csfMatrix44Copy(float* __restrict dst, const float* __restrict a)
{
  memcpy(dst, a, sizeof(float) * 16);
}

static inline void csfMatrix44MultiplyFull(float* __restrict clip, const float* __restrict proj, const float* __restrict modl)
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

static void csfTraverseHierarchy(CSFile* csf, CSFNode* __restrict node, CSFNode* __restrict parent)
{
  if(parent)
  {
    csfMatrix44MultiplyFull(node->worldTM, parent->worldTM, node->objectTM);
  }
  else
  {
    csfMatrix44Copy(node->worldTM, node->objectTM);
  }

  for(int i = 0; i < node->numChildren; i++)
  {
    CSFNode* __restrict child = csf->nodes + node->children[i];
    csfTraverseHierarchy(csf, child, node);
  }
}

static inline size_t csfGeometryPartChannelSize(CSFGeometryPartChannel channel)
{
  switch(channel)
  {
    case CSFGEOMETRY_PARTCHANNEL_BBOX:
      return sizeof(CSFGeometryPartBbox);
    case CSFGEOMETRY_PARTCHANNEL_VERTEXRANGE:
      return sizeof(CSFGeometryPartVertexRange);
    default:
      return 0;
  }
}

static inline size_t csfGeometryPerPartSize(int numPartChannels, CSFGeometryPartChannel* perpartStorageOrder, int numParts)
{
  size_t size = 0;
  for(int i = 0; i < numPartChannels; i++)
  {
    size += CSFGeometryPartChannel_getSize(perpartStorageOrder[i]) * numParts;
  }
  return size;
}

CSFAPI int CSFile_transform(CSFile* csf)
{
  if(csf->rootIDX < 0)
    return CADSCENEFILE_NOERROR;

  if(!(csf->fileFlags & CADSCENEFILE_FLAG_UNIQUENODES))
    return CADSCENEFILE_ERROR_OPERATION;

  csfTraverseHierarchy(csf, csf->nodes + csf->rootIDX, nullptr);
  return CADSCENEFILE_NOERROR;
}

static void CSFile_versionFixHeader(CSFile* csf)
{
  if(csf->version < CADSCENEFILE_VERSION_FILEFLAGS)
  {
    csf->fileFlags = csf->fileFlags ? CADSCENEFILE_FLAG_UNIQUENODES : 0;
  }
}

static const CSFMeta* CSFile_getNodeMetas(const CSFile* csf)
{
  if(csf->version >= CADSCENEFILE_VERSION_META)
  {
    return csf->nodeMetas;
  }

  return nullptr;
}

static const CSFMeta* CSFile_getGeometryMetas(const CSFile* csf)
{
  if(csf->version >= CADSCENEFILE_VERSION_META)
  {
    return csf->geometryMetas;
  }

  return nullptr;
}

static const CSFMeta* CSFile_getFileMeta(const CSFile* csf)
{
  if(csf->version >= CADSCENEFILE_VERSION_META)
  {
    return csf->fileMeta;
  }

  return nullptr;
}

static inline const CSFBytePacket* csfGetBytePacket(const unsigned char* bytes, CSFoffset numBytes, const CSFGuid guid)
{
  if(numBytes < sizeof(CSFBytePacket))
    return nullptr;

  do
  {
    const CSFBytePacket* packet = (const CSFBytePacket*)bytes;
    if(memcmp(&guid, &packet->guid, sizeof(CSFGuid)) == 0)
    {
      return packet;
    }
    numBytes -= packet->numBytes;
    bytes += packet->numBytes;

  } while(numBytes >= sizeof(CSFBytePacket));

  return nullptr;
}

CSFAPI const CSFBytePacket* CSFMeta_getBytePacket(const CSFMeta* meta, const CSFGuid guid)
{
  return meta ? csfGetBytePacket(meta->bytes, meta->numBytes, guid) : nullptr;
}


CSFAPI const CSFBytePacket* CSFMaterial_getBytePacket(const CSFMaterial* material, const CSFGuid guid)
{
  return csfGetBytePacket(material->bytes, material->numBytes, guid);
}


CSFAPI const CSFBytePacket* CSFile_getMaterialBytePacket(const CSFile* csf, int materialIDX, const CSFGuid guid)
{
  if(materialIDX < 0 || materialIDX >= csf->numMaterials)
  {
    return nullptr;
  }

  return csfGetBytePacket(csf->materials[materialIDX].bytes, csf->materials[materialIDX].numBytes, guid);
}

CSFAPI const CSFBytePacket* CSFile_getFileBytePacket(const CSFile* csf, const CSFGuid guid)
{
  return CSFMeta_getBytePacket(csf->fileMeta, guid);
}

CSFAPI void CSFMatrix_identity(float* matrix)
{
  memset(matrix, 0, sizeof(float) * 16);
  matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1.0f;
}

CSFAPI void CSFGeometry_clearDeprecated(CSFGeometry* geo)
{
  memset(geo->_deprecated, 0, sizeof(geo->_deprecated));
}

CSFAPI void CSFile_clearDeprecated(CSFile* csf)
{
  for(int g = 0; g < csf->numGeometries; g++)
  {
    CSFGeometry_clearDeprecated(csf->geometries + g);
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

CSFAPI void CSFGeometry_removeAllPartChannels(CSFGeometry* geo, CSFileMemoryPTR mem)
{
  geo->numPartChannels     = 0;
  geo->perpart             = nullptr;
  geo->perpartStorageOrder = nullptr;
}

CSFAPI void CSFMeta_setOrAddBytePacket(CSFMeta** metaPtr, CSFileMemoryPTR csfmem, size_t size, const void* data)
{
  assert(metaPtr && data);

  CSFMeta* meta = *metaPtr;

  if(!meta)
  {
    *metaPtr = meta = (CSFMeta*)CSFileMemory_alloc(csfmem, sizeof(CSFMeta), nullptr);
    memset(meta, 0, sizeof(CSFMeta));
  }

  const CSFBytePacket* packetIn = (const CSFBytePacket*)data;
  assert(packetIn->numBytes == uint32_t(size));

  CSFBytePacket* packetFile = (CSFBytePacket*)CSFMeta_getBytePacket(meta, packetIn->guid);
  if(packetFile)
  {
    assert(packetFile->numBytes == uint32_t(size));
    memcpy(packetFile, data, size);
  }
  else
  {
    meta->bytes = (unsigned char*)CSFileMemory_allocPartial(csfmem, meta->numBytes + size, meta->numBytes, meta->bytes);
    memcpy(meta->bytes + meta->numBytes, data, size);
    meta->numBytes += size;
  }
}

CSFAPI void CSFGeometry_removePartChannels(CSFGeometry* geo, CSFileMemoryPTR mem, uint32_t numChannels, const CSFGeometryPartChannel* channels)
{
  uint32_t numNew = 0;
  for(int p = 0; p < geo->numPartChannels; p++)
  {
    CSFGeometryPartChannel channel = geo->perpartStorageOrder[p];
    bool                   remove  = false;
    for(uint32_t i = 0; i < numChannels; i++)
    {
      if(channel == channels[i])
      {
        remove = true;
        break;
      }
    }

    if(!remove)
    {
      const uint8_t* oldContent          = (const uint8_t*)CSFGeometry_getPartChannel(geo, channel);
      geo->perpartStorageOrder[numNew++] = channel;
      uint8_t* newContent                = (uint8_t*)CSFGeometry_getPartChannel(geo, channel);
      if(newContent != oldContent)
      {
        size_t size = CSFGeometryPartChannel_getSize(channel) * geo->numParts;
        if(newContent + size >= oldContent)
        {
          memmove(newContent, oldContent, size);
        }
        else
        {
          memcpy(newContent, oldContent, size);
        }
      }
    }
  }

  geo->numPartChannels = numNew;
}

CSFAPI void CSFGeometry_requirePartChannels(CSFGeometry* geo, CSFileMemoryPTR mem, uint32_t numChannels, const CSFGeometryPartChannel* channels)
{
  uint32_t numNew = 0;
  for(uint32_t i = 0; i < numChannels; i++)
  {
    bool exists = CSFGeometry_getPartChannel(geo, channels[i]) != nullptr;
    numNew += exists ? 0 : 1;
  }

  if(!numNew)
    return;

  size_t oldSize = CSFGeometry_getPerPartSize(geo);

  // realloc storage
  geo->perpartStorageOrder =
      CSFileMemory_allocPartialT(mem, geo->numPartChannels + numNew, geo->numPartChannels, geo->perpartStorageOrder);

  numNew = 0;
  for(uint32_t i = 0; i < numChannels; i++)
  {
    bool exists = CSFGeometry_getPartChannel(geo, channels[i]) != nullptr;
    if(!exists)
    {
      geo->perpartStorageOrder[geo->numPartChannels + (numNew++)] = channels[i];
    }
  }

  geo->numPartChannels += numNew;

  size_t newSize = CSFGeometry_getPerPartSize(geo);

  // realloc content
  geo->perpart = CSFileMemory_allocPartialT(mem, newSize, oldSize, geo->perpart);
}

CSFAPI void CSFGeometry_requirePartChannel(CSFGeometry* geo, CSFileMemoryPTR mem, CSFGeometryPartChannel channel)
{
  CSFGeometry_requirePartChannels(geo, mem, 1, &channel);
}

CSFAPI void CSFGeometry_requireAuxChannel(CSFGeometry* geo, CSFileMemoryPTR mem, CSFGeometryAuxChannel channel)
{
  if(CSFGeometry_getAuxChannel(geo, channel))
    return;

  size_t oldSize = sizeof(float) * 4 * geo->numVertices * geo->numAuxChannels;

  // realloc storage
  geo->auxStorageOrder = CSFileMemory_allocPartialT(mem, geo->numAuxChannels + 1, geo->numAuxChannels, geo->auxStorageOrder);
  geo->auxStorageOrder[geo->numAuxChannels++] = channel;

  size_t newSize = sizeof(float) * 4 * geo->numVertices * geo->numAuxChannels;

  // realloc content
  geo->aux = CSFileMemory_allocPartialT(mem, newSize, oldSize, geo->aux);
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
  return int(channel) < geo->numNormalChannels ? geo->normal + size_t(geo->numVertices * 3 * channel) : nullptr;
}

CSFAPI const float* CSFGeometry_getTexChannel(const CSFGeometry* geo, CSFGeometryTexChannel channel)
{
  return int(channel) < geo->numTexChannels ? geo->tex + size_t(geo->numVertices * 2 * channel) : nullptr;
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
  return csfGeometryPartChannelSize(channel);
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

CSFAPI size_t CSFGeometry_getPerPartSize(const CSFGeometry* geo)
{
  return csfGeometryPerPartSize(geo->numPartChannels, geo->perpartStorageOrder, geo->numParts);
}

CSFAPI size_t CSFGeometry_getPerPartRequiredSize(const CSFGeometry* geo, int numParts)
{
  return csfGeometryPerPartSize(geo->numPartChannels, geo->perpartStorageOrder, numParts);
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
    offset += csfGeometryPartChannelSize(geo->perpartStorageOrder[i]) * numParts;
  }
  return ~0ull;
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

// Adds two unsigned size_t numbers and returns the result. If the sum would
// overflow, sets the provided flag to true and returns 0.
// This is used for range checking; for instance, imagine a file where
// pointersOFFSET = 0x100 and numPointers = 0x20000000`00000001. Then
// pointersOFFSET + numPointers * sizeof(CSFoffset) = 0x108 - so if we didn't
// check for this, both the start and end of the pointer array could appear to
// be in-bounds, even though the header would be invalid.
static size_t CSFile_checkedAdd(const size_t a, const size_t b, bool& overflow)
{
  if(a > SIZE_MAX - b)  // Safe way of checking a+b > SIZE_MAX without overflow
  {
    overflow = true;
    return 0;
  }
  return a + b;
}

// Multiplies two unsigned size_t numbers and returns the result. If the sum
// would overflow, sets the provided flag to true and returns 0.
static size_t CSFile_checkedMul(const size_t a, const size_t b, bool& overflow)
{
  if(b != 0 && (a > SIZE_MAX / b))
  {
    overflow = true;
    return 0;
  }
  return a * b;
}

static size_t CSFile_max(const size_t a, const size_t b)
{
  if(a > b)
  {
    return a;
  }
  return b;
}

// Returns the minimum required size of the file based on the header, or 0 if
// the file fails validation checks. This assumes that csf points to at least
// CSFile_getHeaderSize(csf) bytes of data.
static size_t CSFile_getRawSize(const CSFile* csf)
{
  // The header must have a valid version number.
  if(CSFile_invalidVersion(csf))
  {
    return 0;
  }

  // All length-type fields in the header must not be negative.
  if((csf->numPointers < 0)       //
     || (csf->numGeometries < 0)  //
     || (csf->numMaterials < 0)   //
     || (csf->numNodes < 0))
  {
    return 0;
  }

  // rootIDX must be in-bounds.
  if(csf->rootIDX >= csf->numNodes)
  {
    return 0;
  }

  // Determine the minimum file length from the different ranges specified
  // in the header.
  bool   overflow            = false;
  size_t minimum_file_length = CSFile_getHeaderSize(csf);
  minimum_file_length =
      CSFile_max(minimum_file_length, CSFile_checkedAdd(csf->pointersOFFSET, csf->numPointers * sizeof(CSFoffset), overflow));
  minimum_file_length =
      CSFile_max(minimum_file_length, CSFile_checkedAdd(csf->geometriesOFFSET, csf->numGeometries * sizeof(CSFGeometry), overflow));
  minimum_file_length =
      CSFile_max(minimum_file_length, CSFile_checkedAdd(csf->materialsOFFSET, csf->numMaterials * sizeof(CSFMaterial), overflow));
  minimum_file_length =
      CSFile_max(minimum_file_length, CSFile_checkedAdd(csf->nodesOFFSET, csf->numNodes * sizeof(CSFNode), overflow));

  if(csf->version >= CADSCENEFILE_VERSION_META)
  {
    if(csf->nodeMetas)
    {
      minimum_file_length =
          CSFile_max(minimum_file_length, CSFile_checkedAdd(csf->nodeMetasOFFSET, csf->numNodes * sizeof(CSFMeta), overflow));
    }

    if(csf->fileMeta)
    {
      minimum_file_length = CSFile_max(minimum_file_length, CSFile_checkedAdd(csf->fileMetaOFFSET, sizeof(CSFMeta), overflow));
    }

    if(csf->geometryMetas)
    {
      minimum_file_length =
          CSFile_max(minimum_file_length,
                     CSFile_checkedAdd(csf->geometryMetasOFFSET, csf->numGeometries * sizeof(CSFMeta), overflow));
    }
  }

  if(overflow)
  {
    return 0;
  }

  return minimum_file_length;
}

//////////////////////////////////////////////////////////////////
// MEMORY

struct CSFileMemory_s
{
  CSFLoaderConfig m_config;

  std::vector<void*> m_allocations;
  std::mutex         m_mutex;

  std::vector<csfutils::FileReadMapping> m_readMappings;

  void* alloc(size_t size, const void* indata = nullptr, size_t indataSize = 0)
  {
    if(size == 0)
      return nullptr;

    void* data = malloc(size);
    if(indata == CSF_MEMORY_ZEROED_FILL)
    {
      memset(data, 0, size);
    }
    else if(indata)
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

  template <typename T>
  T* allocT(size_t size, const T* indata, size_t indataSize = 0)
  {
    return (T*)alloc(size, indata, indataSize);
  }

  CSFileMemory_s()
  {
    m_config.secondariesReadOnly      = 0;
    m_config.validate                 = 1;
    m_config.gltfFindUniqueGeometries = 1;
  }

  ~CSFileMemory_s()
  {
    size_t numThreads = (std::thread::hardware_concurrency() + 1) / 2;

    if(m_allocations.size() > 2048)
    {
      std::vector<std::thread> threads(numThreads);

      size_t range = (m_allocations.size() + numThreads - 1) / numThreads;

      for(size_t t = 0; t < numThreads; t++)
      {
        threads[t] = std::thread(
            [&](size_t offset, size_t range) {
              for(size_t i = offset; i < std::min(offset + range, m_allocations.size()); i++)
              {
                free(m_allocations[i]);
              }
            },
            t * range, range);
      }
      for(size_t t = 0; t < numThreads; t++)
      {
        threads[t].join();
      }
    }
    else
    {
      for(size_t i = 0; i < m_allocations.size(); i++)
      {
        free(m_allocations[i]);
      }
    }


    m_readMappings.clear();
  }
};

CSFAPI CSFileMemoryPTR CSFileMemory_new()
{
  return new CSFileMemory_s;
}
CSFAPI CSFileMemoryPTR CSFileMemory_newCfg(const CSFLoaderConfig* config)
{
  CSFileMemoryPTR mem = new CSFileMemory_s;
  mem->m_config       = *config;
  return mem;
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
  return mem->alloc(sz, szPartial == 0 ? nullptr : fillPartial, szPartial);
}

CSFAPI void CSFileMemory_getCfg(CSFileMemoryPTR mem, CSFLoaderConfig* config)
{
  memcpy(config, &mem->m_config, sizeof(CSFLoaderConfig));
}

CSFAPI int CSFileMemory_areSecondariesReadOnly(CSFileMemoryPTR mem)
{
  return mem->m_config.secondariesReadOnly;
}

//////////////////////////////////////////////////////////////////////////
// LOADING


struct CSFileHandle_s
{
  FILE*  m_file;
  CSFile m_header;
  size_t m_fileSize;

  void close()
  {
    if(m_file)
    {
      fclose(m_file);
      m_file     = nullptr;
      m_fileSize = 0;
    }
  }
  void      seek(CSFoffset offset, int pos) { xfseek(m_file, offset, pos); }
  CSFoffset tell() { return xftell(m_file); }

  // returns number of bytes read (either matches dataSize or is 0)
  size_t read(void* data, size_t dataSize) { return xfreads(data, dataSize, dataSize, 1, m_file) * dataSize; }

  int open(const char* filename)
  {

    int result = 0;
#ifdef WIN32
    result = !fopen_s(&m_file, filename, "rb");
#else
    m_file = fopen(filename, "rb");
    result = (m_file) != nullptr;
#endif

    if(!result)
    {
      return CADSCENEFILE_ERROR_NOFILE;
    }

    size_t sizeshould = 0;
    if(read(&m_header, sizeof(m_header)) != sizeof(m_header))
    {
      return CADSCENEFILE_ERROR_VERSION;
    }

    // zero things as expected

    size_t headerSize = CSFile_getHeaderSize(&m_header);
    size_t delta      = sizeof(m_header) - headerSize;
    if(delta)
    {
      memset(((char*)&m_header) + (sizeof(m_header) - delta), 0, delta);
    }

    CSFile_versionFixHeader(&m_header);

    sizeshould = CSFile_getRawSize(&m_header);
    if(sizeshould == 0)
    {
      return CADSCENEFILE_ERROR_VERSION;
    }

    seek(0, SEEK_END);
    m_fileSize = tell();
    seek(0, SEEK_SET);

    if(m_fileSize < sizeshould)
    {
      return CADSCENEFILE_ERROR_VERSION;
    }

    return CADSCENEFILE_NOERROR;
  }

  size_t read(CSFoffset offset, void* data, size_t dataSize)
  {
    seek(offset, SEEK_SET);
    size_t readSize = read(data, dataSize);
    return readSize;
  }

  void* allocAndRead(CSFoffset offset, size_t size, CSFileMemoryPTR mem)
  {
    if(offset == 0 || size == 0)
    {
      return nullptr;
    }

    void* data = mem->alloc(size);

    seek(offset, SEEK_SET);
    size_t readSize = read(data, size);
    if(size != readSize)
    {
      return nullptr;
    }

    return data;
  }

  template <class T>
  T* allocAndReadT(CSFoffset offset, uint32_t num, CSFileMemoryPTR mem, int valid = 1)
  {
    if(!valid)
    {
      return nullptr;
    }

    T* data = (T*)allocAndRead(offset, sizeof(T) * num, mem);

    return data;
  }

  CSFileHandle_s() { memset(this, 0, sizeof(CSFileHandle_s)); }

  ~CSFileHandle_s() { close(); }
};


template <typename T>
static void fixPointer(T*& ptr, CSFoffset offset, void* base)
{
  if(offset)
  {
    ptr = (T*)(((uint8_t*)base) + offset);
  }
}

static void CSFile_fixSecondaryPointers(CSFile* csf, void* base)
{
  // setup pointers
  for(int m = 0; m < csf->numMaterials; m++)
  {
    CSFMaterial& material = csf->materials[m];
    fixPointer(material.bytes, material.bytesOFFSET, base);
  }
  for(int g = 0; g < csf->numGeometries; g++)
  {
    CSFGeometry& geo = csf->geometries[g];
    fixPointer(geo.vertex, geo.vertexOFFSET, base);
    fixPointer(geo.normal, geo.normalOFFSET, base);
    fixPointer(geo.tex, geo.texOFFSET, base);
    fixPointer(geo.indexSolid, geo.indexSolidOFFSET, base);
    fixPointer(geo.indexWire, geo.indexWireOFFSET, base);
    fixPointer(geo.parts, geo.partsOFFSET, base);
    fixPointer(geo.auxStorageOrder, geo.auxStorageOrderOFFSET, base);
    fixPointer(geo.aux, geo.auxOFFSET, base);
    fixPointer(geo.perpart, geo.perpartOFFSET, base);
    fixPointer(geo.perpartStorageOrder, geo.perpartStorageOrderOFFSET, base);
  }

  csfutils::parallel_ranges(csf->numNodes, [&](uint64_t idxBegin, uint64_t idxEnd, uint32_t threadIndex, void* userData) {
    for(int i = int(idxBegin); i < int(idxEnd); i++)
    {
      CSFNode& node = csf->nodes[i];
      fixPointer(node.children, node.childrenOFFSET, base);
      fixPointer(node.parts, node.partsOFFSET, base);
    }
  });

  if(CSFile_getGeometryMetas(csf))
  {
    for(int g = 0; g < csf->numGeometries; g++)
    {
      CSFMeta& meta = csf->geometryMetas[g];
      fixPointer(meta.bytes, meta.bytesOFFSET, base);
    }
  }
  if(CSFile_getNodeMetas(csf))
  {
    csfutils::parallel_ranges(csf->numNodes, [&](uint64_t idxBegin, uint64_t idxEnd, uint32_t threadIndex, void* userData) {
      for(int i = int(idxBegin); i < int(idxEnd); i++)
      {
        CSFMeta& meta = csf->nodeMetas[i];
        fixPointer(meta.bytes, meta.bytesOFFSET, base);
      }
    });
  }
  if(CSFile_getFileMeta(csf))
  {
    CSFMeta& meta = csf->fileMeta[0];
    fixPointer(meta.bytes, meta.bytesOFFSET, base);
  }
}

//////////////////////////////////////////////////////////////////////////

// hasContent means the pointers within the struct are loaded as well

static inline void csfPostLoadFile(CSFile* csf, bool hasContent = true)
{
  if(csf->version < CADSCENEFILE_VERSION_FILEFLAGS)
  {
    csf->fileFlags = csf->fileFlags ? CADSCENEFILE_FLAG_UNIQUENODES : 0;
  }

  csf->numPointers = 0;
  csf->pointers    = nullptr;
}

static inline void csfPostLoadMaterial(const CSFile* csf, CSFMaterial* material, bool hasContent = true) {}

static inline void csfPostLoadGeometry(const CSFile* csf, CSFGeometry* geo, bool hasContent = true)
{
  CSFGeometry_clearDeprecated(geo);

  if(csf->version < CADSCENEFILE_VERSION_GEOMETRYCHANNELS)
  {
    CSFGeometry_setupDefaultChannels(geo);
  }
}

static inline void csfPostLoadNode(const CSFile* csf, CSFNode* node, bool hasContent = true)
{
  if(csf->version >= CADSCENEFILE_VERSION_PARTNODEIDX || !hasContent)
    return;

  // node->parts is only guaranteed to be valid if geometryIDX >= 0:
  if(node->geometryIDX >= 0)
  {
    for(int p = 0; p < node->numParts; p++)
    {
      node->parts[p].nodeIDX = -1;
    }
  }
}

// Returns whether an array defined by `ptr` and `numElements` is contained
// within the range of bytes specified by `csf` and `csf_size`, not including
// the header.
template <class T, class CountType>
static bool CSFile_validateRange(const T* ptr, CountType numElements, const CSFile* csf, size_t csf_size)
{
  if(numElements < 0)
    return false;

  // Always allow a range of length 0, even if ptr is not valid. This occurs on
  // worldcar.csf.gz, for instance.
  if(numElements == 0)
    return true;

  // Two casts here to ensure cast from void* to uintptr_t is valid, since only
  // T* -> void* and void* -> uintptr_t are defined.
  const uintptr_t ptr_as_number = reinterpret_cast<uintptr_t>(static_cast<const void*>(ptr));
  const uintptr_t csf_as_number = reinterpret_cast<uintptr_t>(static_cast<const void*>(csf));

  // We say that csf->pointers is the only array which can point to other values
  // in the header. Otherwise, it is very easy for objects to contain pointers
  // into the header - and when functions like CSFile_setupDefaultChannels
  // modify these objects, they could make the header invalid!
  if(ptr_as_number < (csf_as_number + CSFile_getHeaderSize(csf)))
    return false;

  if(ptr_as_number % alignof(T) != 0)
  {
    return false;
  }

  static_assert(std::is_same<size_t, uintptr_t>::value,
                "Behavior of CSFile_validatePointer requires size_t and uintptr_t to be equivalent!");

  bool         overflow    = false;
  const size_t address_end = CSFile_checkedAdd(csf_as_number, csf_size, overflow);
  const size_t array_size  = CSFile_checkedMul(sizeof(T), static_cast<size_t>(numElements), overflow);
  const size_t pointer_end = CSFile_checkedAdd(ptr_as_number, array_size, overflow);
  if(overflow || (pointer_end > address_end))
    return false;

  return true;
}

// Returns whether a CSFMeta array is within bounds.
static bool CSFile_validateMetaArray(const CSFMeta* meta_ptr, int numElements, const CSFile* csf, size_t csf_size)
{
  if(!CSFile_validateRange(meta_ptr, numElements, csf, csf_size))
    return false;

  int hadError = 0;

  csfutils::parallel_ranges(
      numElements,
      [&](uint64_t idxBegin, uint64_t idxEnd, uint32_t threadIdx, void* userData) {
        for(uint64_t m = idxBegin; m < idxEnd; m++)
        {
          const CSFMeta& meta = meta_ptr[m];
          if(!CSFile_validateRange(meta.bytes, meta.numBytes, csf, csf_size))
          {
            hadError = 1;
            break;
          }
        }
      },
      nullptr, 1024 * 256);

  return hadError == 0;
}

// Returns whether all ranges are within bounds. This will catch if
// csf->pointers didn't include some pointers.
static bool CSFile_validateAllRanges(const CSFile* csf, size_t csf_size)
{
  if(!CSFile_validateRange(csf->geometries, csf->numGeometries, csf, csf_size))
    return false;

  if(csf->version >= CADSCENEFILE_VERSION_GEOMETRYCHANNELS)
  {
    for(int g = 0; g < csf->numGeometries; g++)
    {
      const CSFGeometry& geo = csf->geometries[g];
      if(geo.numNormalChannels < 0 || geo.numTexChannels < 0 || geo.numAuxChannels < 0 || geo.numPartChannels < 0
         || geo.numParts < 0 || geo.numVertices < 0 || geo.numIndexSolid < 0 || geo.numIndexWire < 0)
        return false;

      if(!CSFile_validateRange(geo.auxStorageOrder, geo.numAuxChannels, csf, csf_size))
        return false;

      bool overflow = false;
      const size_t aux_count = CSFile_checkedMul(CSFile_checkedMul(4, geo.numVertices, overflow), geo.numAuxChannels, overflow);
      if(overflow)
        return false;
      if(!CSFile_validateRange(geo.aux, aux_count, csf, csf_size))
        return false;

      if(!CSFile_validateRange(geo.perpartStorageOrder, geo.numPartChannels, csf, csf_size))
        return false;

      // For simplicity here, we require that
      // geo.numPartChannels * sizeof(CSFGeometryPartBbox) * geo.numParts <= SIZE_MAX -
      // that ensures that these functions called here don't overflow.
      CSFile_checkedMul(sizeof(CSFGeometryPartBbox), CSFile_checkedMul(geo.numPartChannels, geo.numParts, overflow), overflow);
      if(overflow)
        return false;

      const size_t perpart_size = CSFGeometry_getPerPartSize(&geo);
      if(!CSFile_validateRange(geo.perpart, perpart_size, csf, csf_size))
        return false;

      const size_t vertex_num_elements = CSFile_checkedMul(3, geo.numVertices, overflow);
      if(overflow || !CSFile_validateRange(geo.vertex, vertex_num_elements, csf, csf_size))
        return false;

      const size_t normal_num_elements = CSFile_checkedMul(vertex_num_elements, geo.numNormalChannels, overflow);
      if(overflow || !CSFile_validateRange(geo.normal, normal_num_elements, csf, csf_size))
        return false;

      const size_t tex_num_elements =
          CSFile_checkedMul(2, CSFile_checkedMul(geo.numVertices, geo.numTexChannels, overflow), overflow);
      if(!CSFile_validateRange(geo.tex, tex_num_elements, csf, csf_size))
        return false;

      if(!CSFile_validateRange(geo.indexSolid, geo.numIndexSolid, csf, csf_size))
        return false;
      if(!CSFile_validateRange(geo.indexWire, geo.numIndexWire, csf, csf_size))
        return false;
      if(!CSFile_validateRange(geo.parts, geo.numParts, csf, csf_size))
        return false;
    }
  }

  if(!CSFile_validateRange(csf->materials, csf->numMaterials, csf, csf_size))
    return false;

  for(int m = 0; m < csf->numMaterials; m++)
  {
    if(!CSFile_validateRange(csf->materials[m].bytes, csf->materials[m].numBytes, csf, csf_size))
      return false;
  }

  if(!CSFile_validateRange(csf->nodes, csf->numNodes, csf, csf_size))
    return false;

  int hadError = 0;

  csfutils::parallel_ranges(csf->numNodes, [&](uint64_t idxBegin, uint64_t idxEnd, uint32_t threadIdx, void* userData) {
    for(uint64_t n = idxBegin; n < idxEnd; n++)
    {
      CSFNode& node = csf->nodes[n];
      if(node.geometryIDX >= 0)
      {
        if(node.geometryIDX >= csf->numGeometries)
        {
          hadError = 1;
          break;
        }

        CSFGeometry& geo = csf->geometries[node.geometryIDX];
        if(node.numParts != geo.numParts)
        {
          hadError = 1;
          break;
        }
        if(!CSFile_validateRange(node.parts, node.numParts, csf, csf_size))
        {
          hadError = 1;
          break;
        }
      }

      if(!CSFile_validateRange(node.children, node.numChildren, csf, csf_size))
      {
        hadError = 1;
        break;
      }
    }
  });

  if(hadError)
  {
    return false;
  }

  if(csf->version >= CADSCENEFILE_VERSION_META)
  {
    if(csf->nodeMetas)
    {
      if(!CSFile_validateMetaArray(csf->nodeMetas, csf->numNodes, csf, csf_size))
        return false;
    }

    if(csf->geometryMetas)
    {
      if(!CSFile_validateMetaArray(csf->geometryMetas, csf->numGeometries, csf, csf_size))
        return false;
    }

    if(csf->fileMeta)
    {
      if(!CSFile_validateMetaArray(csf->fileMeta, 1, csf, csf_size))
        return false;
    }
  }

  return true;
}

CSFAPI int CSFile_loadRaw(CSFile* outcsf, size_t size, void* dataraw, int validate)
{
  char*   data = (char*)dataraw;
  CSFile* csf  = (CSFile*)data;

  if(size_t(outcsf) >= size_t(dataraw) && size_t(outcsf) < (size_t(dataraw) + size))
  {
    return CADSCENEFILE_ERROR_OPERATION;
  }

  if(size < sizeof(CSFile) || CSFile_invalidVersion(csf))
  {
    return CADSCENEFILE_ERROR_VERSION;
  }

  CSFile_versionFixHeader(csf);

  {
    const size_t required_minimum_size = CSFile_getRawSize(csf);
    if((required_minimum_size == 0) || (size < required_minimum_size))
    {
      // The file was truncated, or the header pointed to data that was out of bounds.
      return CADSCENEFILE_ERROR_VERSION;
    }
  }

  // Pointers are saved relative to the start of the file. Modify them so that
  // they point to locations in memory. Also check that alignments are OK to
  // avoid undefined behavior. Finally, check that each element of csf->pointers
  // points to csf->geometries or later - otherwise, this shift could change the
  // header, or even the location of the pointers array itself!
  if(validate && csf->pointersOFFSET % alignof(CSFoffset) != 0)
  {
    return CADSCENEFILE_ERROR_INVALID;
  }

  csf->pointersOFFSET += (CSFoffset)csf;

  int hadError = 0;

  csfutils::parallel_ranges(
      csf->numPointers,
      [&](uint64_t idxBegin, uint64_t idxEnd, uint32_t threadIdx, void* userData) {
        for(uint64_t i = idxBegin; i < idxEnd; i++)
        {
          CSFoffset offsetFromStart;

          // support unaligned loads
          const uint8_t* pointerData = (const uint8_t*)csf->pointers;
          memcpy(&offsetFromStart, pointerData + sizeof(CSFoffset) * i, sizeof(CSFoffset));

          if(validate
             && ((offsetFromStart > static_cast<CSFoffset>(size) - sizeof(CSFoffset*))  //
                 || (offsetFromStart < offsetof(CSFile, geometries))                    //
                 || (offsetFromStart % alignof(CSFoffset) != 0)))
          {
            hadError = 1;
            break;
          }
          else
          {
#if 0
            CSFoffset* ptr = (CSFoffset*)(data + offsetFromStart);
            *(ptr) += (CSFoffset)csf;
#else
            // support unaligned loads
            CSFoffset pointer;
            memcpy(&pointer, data + offsetFromStart, sizeof(CSFoffset));
            pointer += (CSFoffset)csf;
            memcpy(data + offsetFromStart, &pointer, sizeof(CSFoffset));
#endif
          }
        }
      },
      nullptr, 1024 * 256);

  // remove pointer information
  csf->pointers    = 0;
  csf->numPointers = 0;

  if(hadError)
  {
    return CADSCENEFILE_ERROR_INVALID;
  }

  // Check that pointers really contained all pointers in the file.
  if(validate && !CSFile_validateAllRanges(csf, size))
  {
    return CADSCENEFILE_ERROR_INVALID;
  }

  int version = csf->version;

  int numMaterials = csf->numMaterials;
  for(int i = 0; i < numMaterials; i++)
  {
    csfPostLoadMaterial(csf, csf->materials + i);
  }

  int numGeometries = csf->numGeometries;
  for(int i = 0; i < numGeometries; i++)
  {
    csfPostLoadGeometry(csf, csf->geometries + i);
  }

  int numNodes = csf->numNodes;
  for(int i = 0; i < numNodes; i++)
  {
    csfPostLoadNode(csf, csf->nodes + i);
  }

  csfPostLoadFile(csf);

  memset(outcsf, 0, sizeof(CSFile));
  memcpy(outcsf, csf, CSFile_getHeaderSize(csf));

  outcsf->fileMeta      = (CSFMeta*)CSFile_getFileMeta(csf);
  outcsf->geometryMetas = (CSFMeta*)CSFile_getGeometryMetas(csf);
  outcsf->nodeMetas     = (CSFMeta*)CSFile_getNodeMetas(csf);

  return CADSCENEFILE_NOERROR;
}

CSFAPI int CSFile_loadReadOnly(CSFile** outcsf, const char* filename, CSFileMemoryPTR mem)
{
#if CSF_DISABLE_FILEMAPPING_SUPPORT
  return CSFile_load(outcsf, filename, mem);
#else
  csfutils::FileReadMapping file;
  if(!file.open(filename))
  {
    return CADSCENEFILE_ERROR_NOFILE;
  }

  const uint8_t* base = (const uint8_t*)file.data();

  // allocate the primary arrays
  CSFile* csf = mem->allocT(sizeof(CSFile), (const CSFile*)nullptr);
  memset(csf, 0, sizeof(CSFile));
  memcpy(csf, base, CSFile_getHeaderSize((const CSFile*)base));

  CSFile_versionFixHeader(csf);
  csf->materials = mem->allocT(sizeof(CSFMaterial) * csf->numMaterials, (const CSFMaterial*)(base + csf->materialsOFFSET));
  csf->geometries = mem->allocT(sizeof(CSFGeometry) * csf->numGeometries, (const CSFGeometry*)(base + csf->geometriesOFFSET));
  csf->nodes = mem->allocT(sizeof(CSFNode) * csf->numNodes, (const CSFNode*)(base + csf->nodesOFFSET));
  csf->pointers = 0;
  csf->numPointers = 0;
  if(CSFile_getGeometryMetas(csf))
  {
    csf->geometryMetas = mem->allocT(sizeof(CSFMeta) * csf->numGeometries, (const CSFMeta*)(base + csf->geometryMetasOFFSET));
  }
  else
  {
    csf->geometryMetas = nullptr;
  }

  if(CSFile_getNodeMetas(csf))
  {
    csf->nodeMetas = mem->allocT(sizeof(CSFMeta) * csf->numNodes, (const CSFMeta*)(base + csf->nodeMetasOFFSET));
  }
  else
  {
    csf->nodeMetas = nullptr;
  }

  if(CSFile_getFileMeta(csf))
  {
    csf->fileMeta = mem->allocT(sizeof(CSFMeta), (const CSFMeta*)(base + csf->fileMetaOFFSET));
  }
  else
  {
    csf->fileMeta = nullptr;
  }

  if(csf->version < CADSCENEFILE_VERSION_GEOMETRYCHANNELS)
  {
    CSFile_setupDefaultChannels(csf);
  }

  CSFile_fixSecondaryPointers(csf, const_cast<void*>((const void*)base));

  mem->m_readMappings.push_back(std::move(file));

  *outcsf = csf;
  return CADSCENEFILE_NOERROR;
#endif
}

CSFAPI int CSFile_load(CSFile** outcsf, const char* filename, CSFileMemoryPTR mem)
{
  CSFileHandle_s fileHandle;

  int result = fileHandle.open(filename);

  if(result != CADSCENEFILE_NOERROR)
  {
    *outcsf = 0;
    return result;
  }

  if(mem->m_config.secondariesReadOnly)
  {
    fileHandle.close();
    return CSFile_loadReadOnly(outcsf, filename, mem);
  }
  else
  {
    size_t dataSize = fileHandle.m_fileSize;
    char*  data     = (char*)CSFileMemory_alloc(mem, dataSize, nullptr);
    size_t readSize = fileHandle.read(data, dataSize);
    if(readSize != dataSize)
    {
      return CADSCENEFILE_ERROR_INVALID;
    }
    fileHandle.close();

    CSFile* newHeader = (CSFile*)CSFileMemory_alloc(mem, sizeof(CSFile), nullptr);

    int err = CSFile_loadRaw(newHeader, dataSize, data, mem->m_config.validate);
    if(err != CADSCENEFILE_NOERROR)
    {
      return err;
    }

    *outcsf = newHeader;

    return CADSCENEFILE_NOERROR;
  }
}

#if CSF_SUPPORT_GLTF2
CSFAPI int CSFile_loadGTLF(CSFile** outcsf, const char* filename, CSFileMemoryPTR mem);
#endif

CSFAPI int CSFile_loadExt(CSFile** outcsf, const char* filename, CSFileMemoryPTR mem)
{
  if(!filename)
  {
    return CADSCENEFILE_ERROR_NOFILE;
  }
  size_t len = strlen(filename);
#if CSF_SUPPORT_ZLIB
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

    CSFile* csf    = (CSFile*)CSFileMemory_alloc(mem, sizeof(CSFile), 0);
    int     result = CSFile_loadRaw(csf, sizeshould, data, mem->m_config.validate);
    *outcsf        = result == CADSCENEFILE_NOERROR ? csf : 0;
    return result;
  }
  else
#endif
#if CSF_SUPPORT_GLTF2
      if(len > 5 && strcmp(filename + len - 5, ".gltf") == 0)
  {
    return CSFile_loadGTLF(outcsf, filename, mem);
  }
#endif
  {
    return CSFile_load(outcsf, filename, mem);
  }
}

//////////////////////////////////////////////////////////////////////////
// SAVING

struct OutputFILE
{
#if CSF_DISABLE_FILEMAPPING_SUPPORT
  FILE* m_file;

  int open(const char* filename, size_t filesize)
  {
#ifdef WIN32
    return fopen_s(&m_file, filename, "wb");
#else
    m_file = fopen(filename, "wb");
    return (m_file) != nullptr;
#endif
  }

  void close() { fclose(m_file); }
  void write(size_t offset, const void* data, size_t dataSize)
  {
    xfseek(m_file, offset, SEEK_SET);
    fwrite(data, dataSize, 1, m_file);
  }

#else
  csfutils::FileReadOverWriteMapping m_mapping;
#endif

  int  open(const char* filename, size_t filesize) { return m_mapping.open(filename, filesize) != true; }
  void close() { m_mapping.close(); }
  void write(size_t offset, const void* data, size_t dataSize)
  {
    memcpy(((uint8_t*)m_mapping.data()) + offset, data, dataSize);
  }
};


struct OutputBuf
{
  char*  m_data;
  size_t m_allocated;
  size_t m_used;
  size_t m_cur;

  int open(const char* filename, size_t filesize)
  {
    m_allocated = filesize;
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

  void write(size_t offset, const void* data, size_t dataSize) { memcpy(m_data + offset, data, dataSize); }
};


#if CSF_SUPPORT_ZLIB
struct OutputGZ
{
  gzFile    m_file;
  OutputBuf m_buf;

  int open(const char* filename, size_t filesize)
  {
    m_buf.open(filename, filesize);
    m_file = gzopen(filename, "wb");
    return m_file == 0;
  }
  void close()
  {
    gzwrite(m_file, m_buf.m_data, (z_off_t)m_buf.m_used);
    gzclose(m_file);
    m_buf.close();
  }
  void write(size_t offset, const void* data, size_t dataSize) { m_buf.write(offset, data, dataSize); }
};
#endif

struct CSFSerializerInfo
{
  int         objIndex       = -1;
  size_t      objSize        = 0;
  const void* obj            = nullptr;
  size_t      objStoreOffset = 0;
  uint32_t    threadIndex    = 0;
};

template <class T>
struct CSFOffsetMgr
{
  T& m_file;

  int m_version = CADSCENEFILE_VERSION;
  int m_pass    = -1;

  // serial file length, start after header
  size_t m_current      = sizeof(CSFile);
  size_t m_numLocations = 0;

  // patch list for pointers
  struct PatchEntry
  {
    CSFoffset offset;
    CSFoffset location;
  };
  std::vector<PatchEntry> m_patches;
  // output pointer locations
  std::vector<CSFoffset> m_locations;

  // in-place fixup of objects' pointers needs temporary data
  // we provide one per thread
  size_t               m_tempObjSizeMax = 0;
  std::vector<uint8_t> m_tempObjData;

  // current pass's object offsets list
  std::vector<size_t>* m_objectOffsets = nullptr;


  CSFOffsetMgr(T& file)
      : m_file(file)
  {
    size_t objSize   = 0;
    objSize          = std::max(objSize, sizeof(CSFNode));
    objSize          = std::max(objSize, sizeof(CSFGeometry));
    objSize          = std::max(objSize, sizeof(CSFMaterial));
    objSize          = std::max(objSize, sizeof(CSFMeta));
    m_tempObjSizeMax = objSize;

    m_tempObjData.resize(m_tempObjSizeMax * CSF_DEFAULT_NUM_THREADS);
  }

  void setPass(int pass) { m_pass = pass; }

  void setObjects(std::vector<size_t>* objectOffsets) { m_objectOffsets = objectOffsets; }

  size_t writePadding(size_t start, size_t padding)
  {
    static const uint32_t padBufferSize            = 16;
    static const uint8_t  padBuffer[padBufferSize] = {0};

    if(!padding)
      return 0;

    size_t paddingLeft = padding;

    while(paddingLeft)
    {
      size_t padCurrent = paddingLeft > padBufferSize ? padBufferSize : paddingLeft;
      m_file.write(start, padBuffer, padCurrent);

      paddingLeft -= padCurrent;
      start += padCurrent;
    }

    return padding;
  }

  size_t handleAlignment(size_t current, size_t alignment)
  {
    // always align to 4 bytes at least
    alignment      = alignment < 4 ? 4 : alignment;
    size_t rest    = current % alignment;
    size_t padding = rest ? alignment - rest : 0;
    current += padding;

    return current;
  }

  void storeHeader(const void* data, size_t dataSize)
  {
    assert(dataSize == sizeof(CSFile));
    m_file.write(0, data, dataSize);
  }

  void beginObjectReference(const CSFSerializerInfo& info)
  {
    assert(info.objIndex >= 0 && info.obj && info.objSize);

    if(m_pass == 0)
    {
      // record content offset for this object
      // we need this to later re-do the various stores relative
      assert(info.objIndex <= m_objectOffsets->size());
      m_objectOffsets->at(info.objIndex) = m_current;
    }
    else
    {
      // object pass, copy in the object to temp
      memcpy(m_tempObjData.data() + m_tempObjSizeMax * info.threadIndex, info.obj, info.objSize);
    }
  }

  void endObjectReference(const CSFSerializerInfo& info)
  {
    assert(info.objIndex >= 0 && info.obj && info.objSize);

    if(m_pass == 0)
      return;

    // finish old object
    m_file.write(info.objStoreOffset, m_tempObjData.data() + m_tempObjSizeMax * info.threadIndex, info.objSize);
  }

  size_t storeArray(size_t location, size_t dataSize, size_t alignment)
  {
    if(m_pass == 0)
    {
      size_t old      = m_current;
      size_t newBegin = handleAlignment(old, alignment);
      m_current       = newBegin;
      m_current += dataSize;
      // arrays stored in last element
      m_objectOffsets->at(m_objectOffsets->size() - 1) = old;

      // without object we also record the patch
      m_patches.push_back({newBegin, location});
      // and the final pointer location
      m_locations.push_back(location);

      return newBegin;
    }
    else
    {
      size_t old      = m_objectOffsets->at(m_objectOffsets->size() - 1);
      size_t newBegin = handleAlignment(old, alignment);
      writePadding(old, newBegin - old);

      return newBegin;
    }
  }

  size_t storeObjectData(const CSFSerializerInfo& info, size_t location, const void* data, size_t dataSize, size_t alignment)
  {
    // in content pass or when no object is used
    // we just dump the data to file

    if(m_pass == 0)
    {
      size_t old      = m_current;
      size_t newBegin = handleAlignment(old, alignment);
      m_current       = newBegin;
      m_current += dataSize;

      // final pointer location is relative to m_storeOffset of current object
      m_locations.push_back(location + info.objStoreOffset);

      return newBegin;
    }
    else
    {
      // modify object's content offset in-place
      size_t& current  = m_objectOffsets->at(info.objIndex);
      size_t  newBegin = handleAlignment(current, alignment);
      writePadding(current, newBegin - current);
      current = newBegin;

      m_file.write(current, data, dataSize);
      current += dataSize;

      // modify temp object
      CSFoffset* localOffset = (CSFoffset*)(m_tempObjData.data() + m_tempObjSizeMax * info.threadIndex + location);
      *localOffset           = newBegin;

      return newBegin;
    }
  }

  size_t getFileSize()
  {
    // add pointer table to end
    size_t current = handleAlignment(m_current, alignof(CSFoffset));
    current += sizeof(CSFoffset) * m_locations.size();

    return current;
  }

  bool finalize(size_t fileSize, size_t tableCountLocation, size_t tableLocation)
  {
    int num = int(m_locations.size());

    if(size_t(num) != m_locations.size())
      return false;

    m_file.write(tableCountLocation, &num, sizeof(int));

    size_t old        = m_current;
    size_t tableBegin = handleAlignment(m_current, alignof(CSFoffset));
    m_current += writePadding(m_current, tableBegin - old);

    m_file.write(tableLocation, &tableBegin, sizeof(CSFoffset));

    // dump table
    m_file.write(tableBegin, m_locations.data(), sizeof(CSFoffset) * m_locations.size());
    m_current += sizeof(CSFoffset) * m_locations.size();

    // patch a few pointers retroactively
    for(ptrdiff_t i = 0; i < ptrdiff_t(m_patches.size()); i++)
    {
      m_file.write(m_patches[i].location, &m_patches[i].offset, sizeof(CSFoffset));
    }

    return m_current == fileSize;
  }
};

namespace CSFSerializer {

// Class is used to handle both load and save depending on template.
// MUST not use functions or values that depend on array content.

template <typename T>
static void processGeometry(T& ser, uint32_t threadIndex, int index, const CSFGeometry* geo, CSFoffset geomOFFSET, size_t perPartSize)
{
  CSFSerializerInfo info;
  info.obj            = geo;
  info.objSize        = sizeof(CSFGeometry);
  info.objIndex       = index;
  info.objStoreOffset = geomOFFSET;
  info.threadIndex    = threadIndex;

  ser.beginObjectReference(info);

  if(geo->vertex && geo->numVertices)
  {
    ser.storeObjectData(info, offsetof(CSFGeometry, vertexOFFSET), geo->vertex, sizeof(float) * 3 * geo->numVertices,
                        alignof(float));
  }
  if(geo->normal && geo->numVertices)
  {
    ser.storeObjectData(info, offsetof(CSFGeometry, normalOFFSET), geo->normal,
                        sizeof(float) * 3 * geo->numVertices * geo->numNormalChannels, alignof(float));
  }
  if(geo->tex && geo->numVertices)
  {
    ser.storeObjectData(info, offsetof(CSFGeometry, texOFFSET), geo->tex,
                        sizeof(float) * 2 * geo->numVertices * geo->numTexChannels, alignof(float));
  }

  if(geo->indexSolid && geo->numIndexSolid)
  {
    ser.storeObjectData(info, offsetof(CSFGeometry, indexSolidOFFSET), geo->indexSolid,
                        sizeof(uint32_t) * geo->numIndexSolid, alignof(uint32_t));
  }
  if(geo->indexWire && geo->numIndexWire)
  {
    ser.storeObjectData(info, offsetof(CSFGeometry, indexWireOFFSET), geo->indexWire,
                        sizeof(uint32_t) * geo->numIndexWire, alignof(uint32_t));
  }

  if(geo->parts && geo->numParts)
  {
    ser.storeObjectData(info, offsetof(CSFGeometry, partsOFFSET), geo->parts, sizeof(CSFGeometryPart) * geo->numParts,
                        alignof(CSFGeometryPart));
  }

  if(ser.m_version >= CADSCENEFILE_VERSION_GEOMETRYCHANNELS)
  {
    if(geo->aux && geo->numVertices)
    {
      ser.storeObjectData(info, offsetof(CSFGeometry, auxOFFSET), geo->aux,
                          sizeof(float) * 4 * geo->numVertices * geo->numAuxChannels, alignof(float));
    }
    if(geo->auxStorageOrder && geo->numAuxChannels)
    {
      ser.storeObjectData(info, offsetof(CSFGeometry, auxStorageOrderOFFSET), geo->auxStorageOrder,
                          sizeof(CSFGeometryAuxChannel) * geo->numAuxChannels, alignof(CSFGeometryAuxChannel));
    }

    if(geo->perpartStorageOrder && geo->numPartChannels)
    {
      ser.storeObjectData(info, offsetof(CSFGeometry, perpartStorageOrder), geo->perpartStorageOrder,
                          sizeof(CSFGeometryPartChannel) * geo->numPartChannels, alignof(CSFGeometryPartChannel));
    }
    if(geo->perpart && geo->numPartChannels && perPartSize)
    {
      ser.storeObjectData(info, offsetof(CSFGeometry, perpart), geo->perpart, perPartSize, 16);
    }
  }

  ser.endObjectReference(info);
}

template <typename T>
static void processMaterial(T& ser, uint32_t threadIndex, int index, const CSFMaterial* mat, CSFoffset matOFFSET)
{
  CSFSerializerInfo info;
  info.obj            = mat;
  info.objSize        = sizeof(CSFMaterial);
  info.objIndex       = index;
  info.objStoreOffset = matOFFSET;
  info.threadIndex    = threadIndex;

  ser.beginObjectReference(info);

  if(mat->bytes && mat->numBytes)
  {
    ser.storeObjectData(info, offsetof(CSFMaterial, bytesOFFSET), mat->bytes, sizeof(unsigned char) * mat->numBytes,
                        alignof(unsigned char));
  }

  ser.endObjectReference(info);
}

template <typename T>
static void processNode(T& ser, uint32_t threadIndex, int index, const CSFNode* node, CSFoffset nodeOFFSET)
{
  CSFSerializerInfo info;
  info.obj            = node;
  info.objSize        = sizeof(CSFNode);
  info.objIndex       = index;
  info.objStoreOffset = nodeOFFSET;
  info.threadIndex    = threadIndex;

  ser.beginObjectReference(info);

  if(node->parts && node->numParts)
  {
    ser.storeObjectData(info, offsetof(CSFNode, partsOFFSET), node->parts, sizeof(CSFNodePart) * node->numParts, alignof(CSFNodePart));
  }
  if(node->children && node->numChildren)
  {
    ser.storeObjectData(info, offsetof(CSFNode, childrenOFFSET), node->children, sizeof(int) * node->numChildren, alignof(int));
  }

  ser.endObjectReference(info);
}

template <typename T>
static inline void processMeta(T& ser, uint32_t threadIndex, int index, const CSFMeta* meta, CSFoffset metaOFFSET)
{
  CSFSerializerInfo info;
  info.obj            = meta;
  info.objSize        = sizeof(CSFMeta);
  info.objIndex       = index;
  info.objStoreOffset = metaOFFSET;
  info.threadIndex    = threadIndex;

  ser.beginObjectReference(info);

  if(meta->bytes && meta->numBytes)
  {
    ser.storeObjectData(info, offsetof(CSFMeta, bytesOFFSET), meta->bytes, sizeof(unsigned char) * meta->numBytes, 16);
  }

  ser.endObjectReference(info);
}
};  // namespace CSFSerializer

template <class T>
static inline void processMetaArray(CSFOffsetMgr<T>& mgr, const CSFile* csf, int numMetas, const CSFMeta* metas, size_t location, uint32_t numThreads)
{
  CSFSerializerInfo dummy;

  size_t metaOFFSET = mgr.storeArray(location, sizeof(CSFMeta) * numMetas, alignof(CSFMeta));

  csfutils::parallel_ranges(
      numMetas,
      [&](uint64_t idxBegin, uint64_t idxEnd, uint32_t threadIndex, void* userData) {
        for(int i = int(idxBegin); i < int(idxEnd); i++)
        {
          const CSFMeta* meta = metas + i;
          CSFSerializer::processMeta(mgr, threadIndex, i, meta, metaOFFSET + i * sizeof(CSFMeta));
        }
      },
      nullptr, 1024, numThreads);
}


template <class T>
static int CSFile_saveInternal(const CSFile* csf, const char* filename)
{
  T               file;
  CSFOffsetMgr<T> mgr(file);

  CSFile dump = {0};
  memcpy(&dump, csf, CSFile_getHeaderSize(csf));

  dump.version = std::max(csf->version, (int)CADSCENEFILE_VERSION);
  dump.magic   = CADSCENEFILE_MAGIC;

  // initialize these to zero, their actual values will be handled later if they exist
  dump.fileMeta      = nullptr;
  dump.geometryMetas = nullptr;
  dump.nodeMetas     = nullptr;

  std::vector<size_t> materialOffsets(csf->numMaterials + 1);
  std::vector<size_t> geometryOffsets(csf->numGeometries + 1);
  std::vector<size_t> nodeOffsets(csf->numNodes + 1);
  std::vector<size_t> geometryMeshletOffsets;
  std::vector<size_t> geometryLodOffsets;
  std::vector<size_t> fileMetaOffsets;
  std::vector<size_t> geoemtryMetaOffsets;
  std::vector<size_t> nodeMetaOffsets;

  // two passes:
  // p 0: compute all offsets and filesize
  // p 1: store to file

  size_t fileSize = 0;

  for(int p = 0; p < 2; p++)
  {

#if CSF_DISABLE_FILEMAPPING_SUPPORT
    // must be serial always
    uint32_t numThreads = 1;
#else
    uint32_t numThreads = CSF_DEFAULT_NUM_THREADS;
#endif
    // computation pass is serial
    if(p == 0)
    {
      numThreads = 1;
    }
    mgr.setPass(p);

    {
      mgr.setObjects(&geometryOffsets);
      size_t geomOFFSET =
          mgr.storeArray(offsetof(CSFile, geometriesOFFSET), sizeof(CSFGeometry) * csf->numGeometries, alignof(CSFGeometry));

      csfutils::parallel_ranges(
          csf->numGeometries,
          [&](uint64_t idxBegin, uint64_t idxEnd, uint32_t threadIndex, void* userData) {
            for(int i = int(idxBegin); i < int(idxEnd); i++)
            {
              const CSFGeometry* geo = csf->geometries + i;
              size_t perPartSize = csfGeometryPerPartSize(geo->numPartChannels, geo->perpartStorageOrder, geo->numParts);
              CSFSerializer::processGeometry(mgr, threadIndex, i, geo, geomOFFSET + i * sizeof(CSFGeometry), perPartSize);
            }
          },
          nullptr, 1024, numThreads);

      // typically not worth to do threading for materials

      mgr.setObjects(&materialOffsets);
      size_t matOFFSET =
          mgr.storeArray(offsetof(CSFile, materialsOFFSET), sizeof(CSFMaterial) * csf->numMaterials, alignof(CSFMaterial));

      for(int i = 0; i < csf->numMaterials; i++)
      {
        const CSFMaterial* mat = csf->materials + i;
        CSFSerializer::processMaterial(mgr, 0, i, mat, matOFFSET + i * sizeof(CSFMaterial));
      }

      mgr.setObjects(&nodeOffsets);
      size_t nodeOFFSET = mgr.storeArray(offsetof(CSFile, nodesOFFSET), sizeof(CSFNode) * csf->numNodes, alignof(CSFNode));

      csfutils::parallel_ranges(
          csf->numNodes,
          [&](uint64_t idxBegin, uint64_t idxEnd, uint32_t threadIndex, void* userData) {
            for(int i = int(idxBegin); i < int(idxEnd); i++)
            {
              const CSFNode* node = csf->nodes + i;
              CSFSerializer::processNode(mgr, threadIndex, i, node, nodeOFFSET + i * sizeof(CSFNode));
            }
          },
          nullptr, 1024, numThreads);
    }
    if(dump.version >= CADSCENEFILE_VERSION_META)
    {
      if(csf->nodeMetas)
      {
        nodeMetaOffsets.resize(csf->numNodes + 1);

        mgr.setObjects(&nodeMetaOffsets);
        processMetaArray(mgr, csf, csf->numNodes, csf->nodeMetas, offsetof(CSFile, nodeMetasOFFSET), numThreads);
      }

      if(csf->geometryMetas)
      {
        geoemtryMetaOffsets.resize(csf->numGeometries + 1);

        mgr.setObjects(&geoemtryMetaOffsets);
        processMetaArray(mgr, csf, csf->numGeometries, csf->geometryMetas, offsetof(CSFile, geometryMetasOFFSET), numThreads);
      }

      if(csf->fileMeta)
      {
        fileMetaOffsets.resize(1 + 1);

        mgr.setObjects(&fileMetaOffsets);
        processMetaArray(mgr, csf, 1, csf->fileMeta, offsetof(CSFile, fileMetaOFFSET), 1);
      }
    }

    // open file at end of computation pass
    if(p == 0)
    {
      fileSize = mgr.getFileSize();
      if(file.open(filename, fileSize))
      {
        return CADSCENEFILE_ERROR_NOFILE;
      }

      // dump main part as is
      mgr.storeHeader(&dump, sizeof(CSFile));
    }
  }

  bool hadError = !mgr.finalize(fileSize, offsetof(CSFile, numPointers), offsetof(CSFile, pointersOFFSET));

  file.close();

  return hadError ? CADSCENEFILE_ERROR_OPERATION : CADSCENEFILE_NOERROR;
}

CSFAPI int CSFile_save(const CSFile* csf, const char* filename)
{
  return CSFile_saveInternal<OutputFILE>(csf, filename);
}

CSFAPI int CSFile_saveExt(CSFile* csf, const char* filename)
{
#if CSF_SUPPORT_ZLIB
  size_t len = strlen(filename);
  if(strcmp(filename + len - 3, ".gz") == 0)
  {
    return CSFile_saveInternal<OutputGZ>(csf, filename);
  }
  else
#endif
  {
    return CSFile_save(csf, filename);
  }
}

//////////////////////////////////////////////////////////////////////////
struct CSFReadMapping_s
{
  csfutils::FileReadMapping mapping;
};

CSFAPI CSFReadMappingPTR CSFReadMapping_new(const char* filename)
{
  CSFReadMappingPTR mapping = new CSFReadMapping_s();
  if(mapping->mapping.open(filename))
  {
    return mapping;
  }
  else
  {
    delete mapping;
    return nullptr;
  }
}
CSFAPI void CSFReadMapping_delete(CSFReadMappingPTR readMapping)
{
  if(readMapping)
  {
    delete readMapping;
  }
}

CSFAPI size_t CSFReadMapping_getSize(CSFReadMappingPTR readMapping)
{
  return readMapping->mapping.size();
}

CSFAPI const void* CSFReadMapping_getData(CSFReadMappingPTR readMapping)
{
  return readMapping->mapping.data();
}

//////////////////////////////////////////////////////////////////////////
// FILEHANDLE API

CSFAPI int CSFileHandle_open(CSFileHandlePTR* pHandle, const char* filename)
{
  CSFileHandlePTR handle = new CSFileHandle_s;
  int             result = handle->open(filename);
  if(result != CADSCENEFILE_NOERROR)
  {
    delete handle;
  }
  else
  {
    *pHandle = handle;
  }

  return result;
}

CSFAPI void CSFileHandle_close(CSFileHandlePTR handle)
{
  delete handle;
}


CSFAPI CSFile* CSFileHandle_rawHeader(CSFileHandlePTR handle, CSFile* header)
{
  memcpy(header, &handle->m_header, sizeof(CSFile));

  return header;
}


CSFAPI CSFile* CSFileHandle_loadHeader(CSFileHandlePTR handle, CSFile* header)
{
  memcpy(header, &handle->m_header, sizeof(CSFile));

  header->materialsOFFSET     = 0;
  header->geometriesOFFSET    = 0;
  header->nodesOFFSET         = 0;
  header->geometryMetasOFFSET = 0;
  header->nodeMetasOFFSET     = 0;
  header->fileMetaOFFSET      = 0;

  return header;
}


struct CSFOffsetNullify
{

  // nukes all pointers to zero

  CSFOffsetNullify(int version, int fileFlags)
      : m_version(version)
  {
  }

  int            m_version;
  int            m_fileFlags;
  unsigned char* m_refpointer;

  void beginObjectReference(const CSFSerializerInfo& info)
  {
    // original the serializer was meant to read from the csf
    // this operation here actually modifies the objects
    m_refpointer = (unsigned char*)info.obj;
  }

  void endObjectReference(const CSFSerializerInfo& info) { m_refpointer = nullptr; }

  size_t storeObjectData(const CSFSerializerInfo& info, size_t location, const void* data, size_t dataSize, size_t alignment)
  {
    void** writePointer = (void**)(m_refpointer + location);
    *writePointer       = nullptr;

    return 0;
  }
};


CSFAPI CSFile* CSFileHandle_loadBasics(CSFileHandlePTR handle, uint32_t typebitsflag, CSFileMemoryPTR mem)
{
  // allocate CSFile
  CSFile* csf = (CSFile*)mem->alloc(sizeof(CSFile), &handle->m_header);

  int version = csf->version;

  CSFOffsetNullify mgr(version, csf->fileFlags);

  csf->materials = handle->allocAndReadT<CSFMaterial>(csf->materialsOFFSET, csf->numMaterials, mem,
                                                      typebitsflag & CSFILEHANDLE_CONTENT_MATERIAL);
  if(csf->materials)
  {
    for(int i = 0; i < csf->numMaterials; i++)
    {
      CSFSerializer::processMaterial(mgr, 0, i, csf->materials + i, 0);
      csfPostLoadMaterial(csf, csf->materials + i, false);
    }
  }

  csf->geometries = handle->allocAndReadT<CSFGeometry>(csf->geometriesOFFSET, csf->numGeometries, mem,
                                                       typebitsflag & CSFILEHANDLE_CONTENT_GEOMETRY);
  if(csf->geometries)
  {
    for(int i = 0; i < csf->numGeometries; i++)
    {
      CSFSerializer::processGeometry(mgr, 0, i, csf->geometries + i, 0, 0);
      csfPostLoadGeometry(csf, csf->geometries + i, false);
    }
  }

  csf->nodes = handle->allocAndReadT<CSFNode>(csf->nodesOFFSET, csf->numNodes, mem, typebitsflag & CSFILEHANDLE_CONTENT_NODE);
  if(csf->nodes)
  {
    for(int i = 0; i < csf->numNodes; i++)
    {
      CSFSerializer::processNode(mgr, 0, i, csf->nodes + i, 0);
      csfPostLoadNode(csf, csf->nodes + i, false);
    }
  }

  csf->geometryMetas =
      handle->allocAndReadT<CSFMeta>(csf->geometryMetasOFFSET, csf->numGeometries, mem,
                                     CSFile_getGeometryMetas(csf) && typebitsflag & CSFILEHANDLE_CONTENT_GEOMETRYMETA);
  if(csf->geometryMetas)
  {
    for(int i = 0; i < csf->numGeometries; i++)
    {
      CSFSerializer::processMeta(mgr, 0, i, csf->geometryMetas + i, 0);
    }
  }

  csf->nodeMetas = handle->allocAndReadT<CSFMeta>(csf->nodeMetasOFFSET, csf->numNodes, mem,
                                                  CSFile_getNodeMetas(csf) && typebitsflag & CSFILEHANDLE_CONTENT_NODEMETA);
  if(csf->nodeMetas)
  {
    for(int i = 0; i < csf->numNodes; i++)
    {
      CSFSerializer::processMeta(mgr, 0, i, csf->nodeMetas + i, 0);
    }
  }

  csf->fileMeta = handle->allocAndReadT<CSFMeta>(csf->fileMetaOFFSET, 1, mem,
                                                 CSFile_getFileMeta(csf) && typebitsflag & CSFILEHANDLE_CONTENT_FILEMETA);
  if(csf->fileMeta)
  {
    for(int i = 0; i < 1; i++)
    {
      CSFSerializer::processMeta(mgr, 0, i, csf->fileMeta + i, 0);
    }
  }

  csfPostLoadFile(csf, false);

  return csf;
}


struct CSFOffsetReader
{
  int             m_version;
  int             m_fileFlags;
  CSFileHandlePTR m_handle;
  CSFileMemoryPTR m_mem;
  bool            m_calculate;

  unsigned char* m_allocation;

  CSFoffset m_minOffset;
  CSFoffset m_maxOffset;

  unsigned char* m_refpointer = nullptr;

  CSFOffsetReader(CSFileHandlePTR handle, CSFileMemoryPTR mem)
      : m_handle(handle)
      , m_mem(mem)
      , m_calculate(true)
  {
    m_version   = handle->m_header.version;
    m_fileFlags = handle->m_header.fileFlags;
  }

  size_t geometrySetup(CSFGeometry* geo)
  {
    if(m_version >= CADSCENEFILE_VERSION_GEOMETRYCHANNELS && geo->numPartChannels)
    {
      // special case data structures
      CSFGeometryPartChannel* channels = new CSFGeometryPartChannel[geo->numPartChannels];

      m_handle->read(geo->perpartStorageOrderOFFSET, channels, sizeof(CSFGeometryPartChannel) * geo->numPartChannels);

      size_t size = csfGeometryPerPartSize(geo->numPartChannels, channels, geo->numParts);
      delete[] channels;

      return size;
    }

    return 0;
  }

  void beginObjectReference(const CSFSerializerInfo& info)
  {
    // original the serializer was meant to read from the csf
    // this operation here actually modifies the objects
    m_refpointer = (unsigned char*)info.obj;
  }

  void endObjectReference(const CSFSerializerInfo& info) { m_refpointer = nullptr; }

  size_t storeObjectData(const CSFSerializerInfo& info, size_t location, const void* data, size_t dataSize, size_t alignment)
  {
    // m_refpointer is the begin of the struct
    // apply the location offset which gives us the place where the pointer/offset is stored
    // offset within file
    CSFoffset readOffset = *(CSFoffset*)(m_refpointer + location);

    if(m_calculate)
    {
      // extract the file memory location of all pointers within an object
      m_minOffset = std::min(m_minOffset, readOffset);
      m_maxOffset = std::max(m_maxOffset, readOffset + dataSize);
    }
    else
    {
      // location of pointer, assign current allocation memory
      void** writePointer = (void**)(m_refpointer + location);
      // reabase the original offset relative to allocation
      *writePointer = m_allocation + (readOffset - m_minOffset);
    }

    return 0;
  }

  void calculatePass()
  {
    m_calculate = true;

    m_minOffset = ~0ull;
    m_maxOffset = 0;
  }

  void readPass()
  {
    m_calculate = false;

    // all data relevant to the same reference object
    // is stored in a contiguous chunk of memory
    size_t allocationSize = m_maxOffset - m_minOffset;
    m_allocation          = (unsigned char*)m_mem->alloc(allocationSize);
    m_handle->read(m_minOffset, m_allocation, allocationSize);
  }
};

CSFAPI void* CSFileHandle_loadElementsInto(CSFileHandlePTR        handle,
                                           CSFileHandleContentBit contentbit,
                                           int                    begin,
                                           int                    num,
                                           CSFileMemoryPTR        mem,
                                           size_t                 pimarySize,
                                           void*                  primaryArray)
{
  const CSFile* csf = &handle->m_header;

  int version = csf->version;

  CSFOffsetReader reader(handle, mem);

  switch(contentbit)
  {
    case CSFILEHANDLE_CONTENT_MATERIAL: {
      assert(begin + num <= csf->numMaterials);

      CSFoffset    offset       = csf->materialsOFFSET + sizeof(CSFMaterial) * begin;
      CSFMaterial* arrayContent = (CSFMaterial*)primaryArray;
      handle->read(offset, arrayContent, pimarySize);
      for(int i = 0; i < num; i++)
      {
        reader.calculatePass();
        CSFSerializer::processMaterial(reader, 0, i, arrayContent + i, 0);
        reader.readPass();
        CSFSerializer::processMaterial(reader, 0, i, arrayContent + i, 0);

        csfPostLoadMaterial(csf, arrayContent + i);
      }
      return arrayContent;
    }
    case CSFILEHANDLE_CONTENT_GEOMETRY: {
      assert(begin + num <= csf->numGeometries);

      CSFoffset    offset       = csf->geometriesOFFSET + sizeof(CSFGeometry) * begin;
      CSFGeometry* arrayContent = (CSFGeometry*)primaryArray;
      handle->read(offset, arrayContent, pimarySize);
      for(int i = 0; i < num; i++)
      {

        size_t perpartSize = reader.geometrySetup(arrayContent + i);

        reader.calculatePass();
        CSFSerializer::processGeometry(reader, 0, i, arrayContent + i, 0, perpartSize);
        reader.readPass();
        CSFSerializer::processGeometry(reader, 0, i, arrayContent + i, 0, perpartSize);

        csfPostLoadGeometry(csf, arrayContent + i);
      }
      return arrayContent;
    }
    case CSFILEHANDLE_CONTENT_NODE: {
      assert(begin + num <= csf->numNodes);

      CSFoffset offset       = csf->nodesOFFSET + sizeof(CSFNode) * begin;
      CSFNode*  arrayContent = (CSFNode*)primaryArray;
      handle->read(offset, arrayContent, pimarySize);
      for(int i = 0; i < num; i++)
      {
        reader.calculatePass();
        CSFSerializer::processNode(reader, 0, i, arrayContent + i, 0);
        reader.readPass();
        CSFSerializer::processNode(reader, 0, i, arrayContent + i, 0);

        csfPostLoadNode(csf, arrayContent + i);
      }
      return arrayContent;
    }
    case CSFILEHANDLE_CONTENT_GEOMETRYMETA: {
      if(!CSFile_getGeometryMetas(csf))
        return nullptr;

      assert(begin + num <= csf->numGeometries);

      CSFoffset offset       = csf->geometryMetasOFFSET + sizeof(CSFMeta) * begin;
      CSFMeta*  arrayContent = (CSFMeta*)primaryArray;
      handle->read(offset, arrayContent, pimarySize);
      for(int i = 0; i < num; i++)
      {
        reader.calculatePass();
        CSFSerializer::processMeta(reader, 0, i, arrayContent + i, 0);
        reader.readPass();
        CSFSerializer::processMeta(reader, 0, i, arrayContent + i, 0);
      }
      return arrayContent;
    }
    case CSFILEHANDLE_CONTENT_NODEMETA: {
      if(!CSFile_getNodeMetas(csf))
        return nullptr;

      assert(begin + num <= csf->numNodes);

      CSFoffset offset       = csf->nodeMetasOFFSET + sizeof(CSFMeta) * begin;
      CSFMeta*  arrayContent = (CSFMeta*)primaryArray;
      handle->read(offset, arrayContent, pimarySize);
      for(int i = 0; i < num; i++)
      {
        reader.calculatePass();
        CSFSerializer::processMeta(reader, 0, i, arrayContent + i, 0);
        reader.readPass();
        CSFSerializer::processMeta(reader, 0, i, arrayContent + i, 0);
      }
      return arrayContent;
    }
    case CSFILEHANDLE_CONTENT_FILEMETA: {
      if(!CSFile_getFileMeta(csf))
        return nullptr;

      assert(begin + num <= 1);

      CSFoffset offset       = csf->fileMetaOFFSET + sizeof(CSFMeta) * begin;
      CSFMeta*  arrayContent = (CSFMeta*)primaryArray;
      handle->read(offset, arrayContent, pimarySize);
      for(int i = 0; i < num; i++)
      {
        reader.calculatePass();
        CSFSerializer::processMeta(reader, 0, i, arrayContent + i, 0);
        reader.readPass();
        CSFSerializer::processMeta(reader, 0, i, arrayContent + i, 0);
      }
      return arrayContent;
    }
  }

  return nullptr;
}

CSFAPI void* CSFileHandle_loadElements(CSFileHandlePTR handle, CSFileHandleContentBit contentbit, int begin, int num, CSFileMemoryPTR mem)
{
  size_t arraySize = 0;

  switch(contentbit)
  {
    case CSFILEHANDLE_CONTENT_MATERIAL:
      arraySize = sizeof(CSFMaterial) * num;
      break;
    case CSFILEHANDLE_CONTENT_GEOMETRY:
      arraySize = sizeof(CSFGeometry) * num;
      break;
    case CSFILEHANDLE_CONTENT_NODE:
      arraySize = sizeof(CSFNode) * num;
      break;
    case CSFILEHANDLE_CONTENT_GEOMETRYMETA:
      arraySize = sizeof(CSFMeta) * num;
      break;
    case CSFILEHANDLE_CONTENT_NODEMETA:
      arraySize = sizeof(CSFMeta) * num;
      break;
    case CSFILEHANDLE_CONTENT_FILEMETA:
      arraySize = sizeof(CSFMeta) * num;
      break;
    default:
      return nullptr;
  }

  void* arrayContent = mem->alloc(arraySize);

  return CSFileHandle_loadElementsInto(handle, contentbit, begin, num, mem, arraySize, arrayContent);
}

//////////////////////////////////////////////////////////////////////////

#if !CSF_DISABLE_FILEMAPPING_SUPPORT

#if defined(LINUX)
#include <errno.h>
#include <unistd.h>
#endif

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

inline DWORD HIDWORD(size_t x)
{
  return (DWORD)(x >> 32);
}
inline DWORD LODWORD(size_t x)
{
  return (DWORD)x;
}
#endif


namespace csfutils {

bool FileMapping::open(const char* fileName, MappingType mappingType, size_t fileSize)
{
  if(!g_pageSize)
  {
#if defined(_WIN32)
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    g_pageSize = (size_t)si.dwAllocationGranularity;
#elif defined(LINUX)
    g_pageSize = (size_t)getpagesize();
#endif
  }

  m_mappingType = mappingType;

  if(mappingType == MAPPING_READOVERWRITE)
  {
    assert(fileSize);
    m_fileSize    = fileSize;
    m_mappingSize = ((fileSize + g_pageSize - 1) / g_pageSize) * g_pageSize;

    // check if the current process is allowed to save a file of that size
#if defined(_WIN32)
    TCHAR          dir[MAX_PATH + 1];
    BOOL           success = FALSE;
    ULARGE_INTEGER numFreeBytes;

    DWORD length = GetVolumePathName(fileName, dir, MAX_PATH + 1);

    if(length > 0)
    {
      success = GetDiskFreeSpaceEx(dir, NULL, NULL, &numFreeBytes);
    }

    m_isValid = (!!success) && (m_mappingSize <= numFreeBytes.QuadPart);
#elif defined(LINUX)
    struct rlimit rlim;
    getrlimit(RLIMIT_FSIZE, &rlim);
    m_isValid = (m_mappingSize <= rlim.rlim_cur);
#endif
    if(!m_isValid)
    {
      return false;
    }
  }

#if defined(_WIN32)
  m_win32.file = mappingType == MAPPING_READONLY ?
                     CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL) :
                     CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

  m_isValid = (m_win32.file != INVALID_HANDLE_VALUE);
  if(m_isValid)
  {
    if(mappingType == MAPPING_READONLY)
    {
      DWORD sizeHi  = 0;
      DWORD sizeLo  = GetFileSize(m_win32.file, &sizeHi);
      m_mappingSize = (static_cast<size_t>(sizeHi) << 32) | sizeLo;
      m_fileSize    = m_mappingSize;
    }

    m_win32.fileMapping = CreateFileMapping(m_win32.file, NULL, mappingType == MAPPING_READONLY ? PAGE_READONLY : PAGE_READWRITE,
                                            HIDWORD(m_mappingSize), LODWORD(m_mappingSize), NULL);

    m_isValid = (m_win32.fileMapping != NULL);
    if(m_isValid)
    {
      m_mappingPtr = MapViewOfFile(m_win32.fileMapping, mappingType == MAPPING_READONLY ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS,
                                   HIDWORD(0), LODWORD(0), (SIZE_T)m_mappingSize);
      if(!m_mappingPtr)
      {
#if 0
          DWORD err = GetLastError();
#endif
        CloseHandle(m_win32.file);
        m_isValid = false;
      }
    }
    else
    {
      CloseHandle(m_win32.file);
    }
  }
#elif defined(LINUX)
  m_unix.file = mappingType == MAPPING_READONLY ? ::open(fileName, O_RDONLY) : ::open(fileName, O_RDWR | O_CREAT | O_TRUNC, 0666);

  m_isValid = (m_unix.file != -1);
  if(m_isValid)
  {
    if(mappingType == MAPPING_READONLY)
    {
      struct stat s;
      fstat(m_unix.file, &s);
      m_mappingSize = s.st_size;
    }
    else
    {
      // make file large enough to hold the complete scene
      lseek(m_unix.file, m_mappingSize - 1, SEEK_SET);
      write(m_unix.file, "", 1);
      lseek(m_unix.file, 0, SEEK_SET);
    }
    m_fileSize = m_mappingSize;
    m_mappingPtr = mmap(0, m_mappingSize, mappingType == MAPPING_READONLY ? PROT_READ : (PROT_READ | PROT_WRITE),
                        MAP_SHARED, m_unix.file, 0);
    if(m_mappingPtr == MAP_FAILED)
    {
      ::close(m_unix.file);
      m_unix.file = -1;
      m_isValid = false;
    }
  }
#endif
  return m_isValid;
}

void FileMapping::close()
{
  if(m_isValid)
  {
#if defined(_WIN32)
    assert((m_win32.file != INVALID_HANDLE_VALUE) && (m_win32.fileMapping != NULL));

    UnmapViewOfFile(m_mappingPtr);
    CloseHandle(m_win32.fileMapping);

    if(m_mappingType == MAPPING_READOVERWRITE)
    {
      // truncate file to minimum size
      // To work with 64-bit file pointers, you can declare a LONG, treat it as the upper half
      // of the 64-bit file pointer, and pass its address in lpDistanceToMoveHigh. This means
      // you have to treat two different variables as a logical unit, which is error-prone.
      // The problems can be ameliorated by using the LARGE_INTEGER structure to create a 64-bit
      // value and passing the two 32-bit values by means of the appropriate elements of the union.
      // (see msdn documentation on SetFilePointer)
      LARGE_INTEGER li;
      li.QuadPart = (__int64)m_fileSize;
      SetFilePointer(m_win32.file, li.LowPart, &li.HighPart, FILE_BEGIN);

      SetEndOfFile(m_win32.file);
    }
    CloseHandle(m_win32.file);

    m_mappingPtr        = nullptr;
    m_win32.fileMapping = nullptr;
    m_win32.file        = nullptr;

#elif defined(LINUX)
    assert(m_unix.file != -1);

    munmap(m_mappingPtr, m_mappingSize);
    if(m_mappingType == MAPPING_READOVERWRITE)
    {
      ::ftruncate(m_unix.file, m_fileSize);
    }
    ::close(m_unix.file);

    m_mappingPtr = nullptr;
    m_unix.file = -1;
#endif

    m_isValid = false;
  }
}

size_t FileMapping::g_pageSize = 0;

}  // namespace csfutils

#else
namespace csfutils {

bool FileMapping::open(const char* fileName, MappingType mappingType, size_t fileSize)
{
  return false;
}
void FileMapping::close() {}
};  // namespace csfutils
#endif


#if CSF_SUPPORT_GLTF2

#include "cgltf.h"
#include <nvmath/nvmath.h>
#include <unordered_map>

void CSFile_countGLTFNodes(CSFile* csf, const cgltf_data* gltfModel, const cgltf_node* node)
{
  csf->numNodes++;
  for(cgltf_size i = 0; i < node->children_count; i++)
  {
    CSFile_countGLTFNodes(csf, gltfModel, node->children[i]);
  }
}

int CSFile_addGLTFNode(CSFile* csf, const cgltf_data* gltfModel, const uint32_t* meshGeometries, CSFileMemoryPTR mem, const cgltf_node* node)
{
  int      idx     = csf->numNodes++;
  CSFNode& csfnode = csf->nodes[idx];

  CSFMatrix_identity(csfnode.worldTM);
  CSFMatrix_identity(csfnode.objectTM);

  if(node->has_matrix)
  {
    for(int i = 0; i < 16; i++)
    {
      csfnode.objectTM[i] = (float)node->matrix[i];
    }
  }
  else
  {
    nvmath::vec3f translation = {0, 0, 0};
    nvmath::quatf rotation    = {0, 0, 0, 0};
    nvmath::vec3f scale       = {1, 1, 1};

    if(node->has_translation)
    {
      translation.x = static_cast<float>(node->translation[0]);
      translation.y = static_cast<float>(node->translation[1]);
      translation.z = static_cast<float>(node->translation[2]);
    }

    if(node->has_rotation)
    {
      rotation.x = static_cast<float>(node->rotation[0]);
      rotation.y = static_cast<float>(node->rotation[1]);
      rotation.z = static_cast<float>(node->rotation[2]);
      rotation.w = static_cast<float>(node->rotation[3]);
    }

    if(node->has_scale)
    {
      scale.x = static_cast<float>(node->scale[0]);
      scale.y = static_cast<float>(node->scale[1]);
      scale.z = static_cast<float>(node->scale[2]);
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
  if(node->mesh)
  {
    size_t meshIndex = node->mesh - gltfModel->meshes;

    csfnode.geometryIDX    = meshGeometries[meshIndex];
    const cgltf_mesh& mesh = gltfModel->meshes[meshIndex];

    csfnode.numParts = csf->geometries[csfnode.geometryIDX].numParts;
    csfnode.parts    = (CSFNodePart*)CSFileMemory_alloc(mem, sizeof(CSFNodePart) * csfnode.numParts, nullptr);

    uint32_t p = 0;
    for(cgltf_size i = 0; i < mesh.primitives_count; i++)
    {
      const cgltf_primitive& primitive = mesh.primitives[i];

      if(primitive.type != cgltf_primitive_type_triangles)
        continue;

      CSFNodePart& csfpart = csfnode.parts[p++];

      csfpart.active      = 1;
      csfpart.materialIDX = primitive.material ? int(primitive.material - gltfModel->materials) : 0;
      csfpart.nodeIDX     = -1;
    }
  }
  else
  {
    csfnode.geometryIDX = -1;
  }

  csfnode.numChildren = (int)node->children_count;
  csfnode.children    = (int*)CSFileMemory_alloc(mem, sizeof(int) * csfnode.numChildren, nullptr);

  for(cgltf_size i = 0; i < node->children_count; i++)
  {
    csfnode.children[i] = CSFile_addGLTFNode(csf, gltfModel, meshGeometries, mem, node->children[i]);
  }

  return idx;
}

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
  uint32_t numVertices  = 0;
  uint32_t numNormals   = 0;
  uint32_t numTexcoords = 0;
  uint32_t numIndices   = 0;
  uint32_t numParts     = 0;

  uint32_t hashIndex    = 0;
  uint32_t hashVertex   = 0;
  uint32_t hashNormal   = 0;
  uint32_t hashTexcoord = 0;

  uint32_t hashLightVertex   = 0;
  uint32_t hashLightNormal   = 0;
  uint32_t hashLightTexcoord = 0;

  bool isEqualHash(const GLTFGeometryInfo& other)
  {
    return hashIndex == other.hashIndex && hashVertex == other.hashVertex && hashNormal == other.hashNormal
           && hashTexcoord == other.hashTexcoord;
  }

  bool isEqualLight(const GLTFGeometryInfo& other)
  {
    return numVertices == other.numVertices && numNormals == other.numNormals && numIndices == other.numIndices
           && numParts == other.numParts && hashLightVertex == other.hashLightVertex
           && hashLightNormal == other.hashLightNormal && hashLightTexcoord == other.hashLightTexcoord;
  }

  void setup(const cgltf_data* gltfModel, const cgltf_mesh& mesh)
  {
    hashVertex   = 0;
    hashNormal   = 0;
    hashTexcoord = 0;
    hashIndex    = 0;

    hashLightVertex   = 0;
    hashLightNormal   = 0;
    hashLightTexcoord = 0;

    for(cgltf_size i = 0; i < mesh.primitives_count; i++)
    {
      const cgltf_primitive& primitive = mesh.primitives[i];

      if(primitive.type != cgltf_primitive_type_triangles)
        continue;

      for(cgltf_size a = 0; a < primitive.attributes_count; a++)
      {
        const cgltf_accessor*    accessor = primitive.attributes[a].data;
        const cgltf_buffer_view* view     = accessor->buffer_view;
        const uint8_t*           data     = reinterpret_cast<const uint8_t*>(view->buffer->data);
        data += accessor->offset + view->offset;

        switch(primitive.attributes[a].type)
        {
          case cgltf_attribute_type_position:
            numVertices += static_cast<uint32_t>(accessor->count);
            hashLightVertex = strMurmurHash2A(data, accessor->stride, hashLightVertex);
            break;
          case cgltf_attribute_type_normal:
            numNormals += static_cast<uint32_t>(accessor->count);
            hashLightNormal = strMurmurHash2A(data, accessor->stride, hashLightNormal);
            break;
          case cgltf_attribute_type_texcoord:
            if(primitive.attributes[a].index == 0)
            {
              numTexcoords += static_cast<uint32_t>(accessor->count);
              hashLightTexcoord = strMurmurHash2A(data, accessor->stride, hashLightTexcoord);
            }
            break;
          default:
            break;  // No matching csf attribute
        }
      }

      numIndices += static_cast<uint32_t>(primitive.indices->count);
      numParts++;
    }
  }

  bool hasHash() const { return hashIndex != 0 || hashVertex != 0 || hashNormal != 0; }

  void setupHash(const cgltf_data* gltfModel, const cgltf_mesh& mesh)
  {
    for(cgltf_size i = 0; i < mesh.primitives_count; i++)
    {
      const cgltf_primitive& primitive = mesh.primitives[i];

      if(primitive.type != cgltf_primitive_type_triangles)
        continue;

      for(cgltf_size a = 0; a < primitive.attributes_count; a++)
      {
        const cgltf_accessor*    accessor = primitive.attributes[a].data;
        const cgltf_buffer_view* view     = accessor->buffer_view;
        const uint8_t*           data     = reinterpret_cast<const uint8_t*>(view->buffer->data);
        data += accessor->offset + view->offset;

        switch(primitive.attributes[a].type)
        {
          case cgltf_attribute_type_position:
            hashVertex = strMurmurHash2A(data, accessor->stride * accessor->count, hashVertex);
            break;
          case cgltf_attribute_type_normal:
            hashNormal = strMurmurHash2A(data, accessor->stride * accessor->count, hashNormal);
            break;
          case cgltf_attribute_type_texcoord:
            if(primitive.attributes[a].index == 0)
            {
              hashTexcoord = strMurmurHash2A(data, accessor->stride * accessor->count, hashTexcoord);
            }
            break;
          default:
            break;  // No matching csf attribute
        }
      }

      {
        const cgltf_accessor*    accessor = primitive.indices;
        const cgltf_buffer_view* view     = accessor->buffer_view;
        const uint8_t*           data     = reinterpret_cast<const uint8_t*>(view->buffer->data);
        data += accessor->offset + view->offset;

        hashIndex = strMurmurHash2A(data, accessor->stride * accessor->count, hashIndex);
      }
    }
  }
};

static inline void setupCSFMaterialTexture(CSFMaterialGLTF2Texture& csftex, const cgltf_texture_view& tex)
{
  if(!tex.texture)
    return;

  const char* uri = tex.texture->image->uri;
  if(uri)
  {
    strncpy(csftex.name, uri, sizeof(csftex.name));
  }
}

struct MappingList
{
  std::unordered_map<std::string, CSFReadMapping> maps;
};

cgltf_result csf_cgltf_read(const struct cgltf_memory_options* memory_options,
                            const struct cgltf_file_options*   file_options,
                            const char*                        path,
                            cgltf_size*                        size,
                            void**                             data)
{
  MappingList* mappings = (MappingList*)file_options->user_data;
  std::string  pathStr(path);

  auto it = mappings->maps.find(pathStr);
  if(it != mappings->maps.end())
  {
    *data = const_cast<void*>(CSFReadMapping_getData(it->second.mapping));
    *size = CSFReadMapping_getSize(it->second.mapping);
    return cgltf_result_success;
  }

  CSFReadMapping entry;
  entry.mapping = CSFReadMapping_new(path);

  if(entry.mapping)
  {
    *data = const_cast<void*>(CSFReadMapping_getData(entry.mapping));
    *size = CSFReadMapping_getSize(entry.mapping);
    mappings->maps.insert({pathStr, std::move(entry)});
    return cgltf_result_success;
  }

  return cgltf_result_io_error;
}

void csf_cgltf_release(const struct cgltf_memory_options* memory_options, const struct cgltf_file_options* file_options, void* data)
{
  // let MappingList destructor handle it
}

CSFAPI int CSFile_loadGTLF(CSFile** outcsf, const char* filename, CSFileMemoryPTR mem)
{
  if(!filename)
  {
    return CADSCENEFILE_ERROR_NOFILE;
  }

  int findUniqueGeometries = mem->m_config.gltfFindUniqueGeometries;

  cgltf_options gltfOptions = {};
  cgltf_data*   gltfModel;

  MappingList mappings;

  gltfOptions.file.read      = csf_cgltf_read;
  gltfOptions.file.release   = csf_cgltf_release;
  gltfOptions.file.user_data = &mappings;

  *outcsf = NULL;

  cgltf_result result = cgltf_parse_file(&gltfOptions, filename, &gltfModel);
  if(result != cgltf_result_success)
  {
    printf("ERR: cgltf_parse_file: %d\n", result);
    return CADSCENEFILE_ERROR_OPERATION;
  }

  result = cgltf_load_buffers(&gltfOptions, gltfModel, filename);
  if(result != cgltf_result_success)
  {
    printf("ERR: cgltf_load_buffers: %d\n", result);
    cgltf_free(gltfModel);
    return CADSCENEFILE_ERROR_OPERATION;
  }

  const cgltf_scene* scene = gltfModel->scene ? gltfModel->scene : &gltfModel->scenes[0];
  if(!scene)
  {
    printf("ERR: cgltf: no scene\n");
    cgltf_free(gltfModel);
    return CADSCENEFILE_ERROR_OPERATION;
  }

  CSFile* csf = (CSFile*)CSFileMemory_alloc(mem, sizeof(CSFile), NULL);
  memset(csf, 0, sizeof(CSFile));
  csf->version   = CADSCENEFILE_VERSION;
  csf->fileFlags = 0;
  csf->fileFlags |= CADSCENEFILE_FLAG_UNIQUENODES;
  csf->numMaterials = (int)gltfModel->materials_count;
  csf->numNodes     = (int)gltfModel->nodes_count;

  csf->materials = (CSFMaterial*)CSFileMemory_alloc(mem, sizeof(CSFMaterial) * csf->numMaterials, NULL);
  memset(csf->materials, 0, sizeof(CSFMaterial) * csf->numMaterials);

  // create materials
  for(cgltf_size materialIdx = 0; materialIdx < gltfModel->materials_count; materialIdx++)
  {
    const cgltf_material& mat    = gltfModel->materials[materialIdx];
    CSFMaterial&          csfmat = csf->materials[materialIdx];

    if(mat.has_pbr_metallic_roughness)
    {
      csfmat.color[0] = mat.pbr_metallic_roughness.base_color_factor[0];
      csfmat.color[1] = mat.pbr_metallic_roughness.base_color_factor[1];
      csfmat.color[2] = mat.pbr_metallic_roughness.base_color_factor[2];
      csfmat.color[3] = mat.pbr_metallic_roughness.base_color_factor[3];
    }
    else if(mat.has_pbr_specular_glossiness)
    {
      csfmat.color[0] = mat.pbr_specular_glossiness.diffuse_factor[0];
      csfmat.color[1] = mat.pbr_specular_glossiness.diffuse_factor[1];
      csfmat.color[2] = mat.pbr_specular_glossiness.diffuse_factor[2];
      csfmat.color[3] = mat.pbr_specular_glossiness.diffuse_factor[3];
    }
    else
    {
      csfmat.color[0] = 1.0f;
      csfmat.color[1] = 1.0f;
      csfmat.color[2] = 1.0f;
      csfmat.color[3] = 1.0f;
    }

    if(mat.name)
    {
      strncpy(csfmat.name, mat.name, sizeof(csfmat.name));
    }
    else
    {
      strncpy(csfmat.name, "undefined", sizeof(csfmat.name));
    }
    csfmat.bytes    = nullptr;
    csfmat.numBytes = 0;
    csfmat.type     = 0;

    CSFMaterialGLTF2Meta csfmatgltf = {{CSFGUID_MATERIAL_GLTF2, sizeof(CSFMaterialGLTF2Meta)}};

    csfmatgltf.shadingModel      = -1;
    csfmatgltf.emissiveFactor[0] = mat.emissive_factor[0];
    csfmatgltf.emissiveFactor[1] = mat.emissive_factor[1];
    csfmatgltf.emissiveFactor[2] = mat.emissive_factor[2];
    csfmatgltf.doubleSided       = mat.double_sided ? 1 : 0;
    csfmatgltf.alphaCutoff       = mat.alpha_cutoff;
    csfmatgltf.alphaMode         = mat.alpha_mode;

    setupCSFMaterialTexture(csfmatgltf.emissiveTexture, mat.emissive_texture);
    setupCSFMaterialTexture(csfmatgltf.normalTexture, mat.normal_texture);
    setupCSFMaterialTexture(csfmatgltf.occlusionTexture, mat.occlusion_texture);

    if(mat.has_pbr_metallic_roughness)
    {
      csfmatgltf.shadingModel       = mat.unlit ? -1 : 0;
      csfmatgltf.baseColorFactor[0] = mat.pbr_metallic_roughness.base_color_factor[0];
      csfmatgltf.baseColorFactor[1] = mat.pbr_metallic_roughness.base_color_factor[1];
      csfmatgltf.baseColorFactor[2] = mat.pbr_metallic_roughness.base_color_factor[2];
      csfmatgltf.baseColorFactor[3] = mat.pbr_metallic_roughness.base_color_factor[3];
      csfmatgltf.roughnessFactor    = mat.pbr_metallic_roughness.roughness_factor;
      csfmatgltf.metallicFactor     = mat.pbr_metallic_roughness.metallic_factor;
      setupCSFMaterialTexture(csfmatgltf.baseColorTexture, mat.pbr_metallic_roughness.base_color_texture);
      setupCSFMaterialTexture(csfmatgltf.metallicRoughnessTexture, mat.pbr_metallic_roughness.metallic_roughness_texture);
    }
    else if(mat.has_pbr_specular_glossiness)
    {
      csfmatgltf.shadingModel      = 1;
      csfmatgltf.diffuseFactor[0]  = mat.pbr_specular_glossiness.diffuse_factor[0];
      csfmatgltf.diffuseFactor[1]  = mat.pbr_specular_glossiness.diffuse_factor[1];
      csfmatgltf.diffuseFactor[2]  = mat.pbr_specular_glossiness.diffuse_factor[2];
      csfmatgltf.diffuseFactor[3]  = mat.pbr_specular_glossiness.diffuse_factor[3];
      csfmatgltf.glossinessFactor  = mat.pbr_specular_glossiness.glossiness_factor;
      csfmatgltf.specularFactor[0] = mat.pbr_specular_glossiness.specular_factor[0];
      csfmatgltf.specularFactor[1] = mat.pbr_specular_glossiness.specular_factor[1];
      csfmatgltf.specularFactor[2] = mat.pbr_specular_glossiness.specular_factor[2];
      setupCSFMaterialTexture(csfmatgltf.diffuseTexture, mat.pbr_specular_glossiness.diffuse_texture);
      setupCSFMaterialTexture(csfmatgltf.specularGlossinessTexture, mat.pbr_specular_glossiness.specular_glossiness_texture);
    }

    csfmat.numBytes = sizeof(csfmatgltf);
    csfmat.bytes    = (unsigned char*)CSFileMemory_alloc(mem, sizeof(csfmatgltf), &csfmatgltf);
  }

  // find unique geometries
  // many gltf files make improper use of geometry instancing

  std::vector<uint32_t> meshGeometries;
  std::vector<uint32_t> geometryMeshes;

  meshGeometries.reserve(gltfModel->meshes_count);
  geometryMeshes.reserve(gltfModel->meshes_count);


  if(findUniqueGeometries)
  {
    // use some hashing based comparisons to avoid deep comparisons

    std::vector<GLTFGeometryInfo> geometryInfos;
    geometryInfos.reserve(gltfModel->meshes_count);

    uint32_t meshIdx = 0;
    for(cgltf_size m = 0; m < gltfModel->meshes_count; m++)
    {
      const cgltf_mesh& mesh = gltfModel->meshes[m];
      GLTFGeometryInfo  geoInfo;

      geoInfo.setup(gltfModel, mesh);

      // compare against existing hashes
      uint32_t found = ~0;
      for(uint32_t i = 0; i < (uint32_t)geometryInfos.size(); i++)
      {
        if(geoInfo.isEqualLight(geometryInfos[i]))
        {
          if(!geometryInfos[i].hasHash())
          {
            geometryInfos[i].setupHash(gltfModel, gltfModel->meshes[geometryMeshes[i]]);
          }

          geoInfo.setupHash(gltfModel, mesh);

          if(geoInfo.isEqualHash(geometryInfos[i]))
          {
            found = i;
            break;
          }
        }
      }
      if(found != ~uint32_t(0))
      {
        meshGeometries.push_back(found);
      }
      else
      {
        meshGeometries.push_back((uint32_t)geometryInfos.size());
        geometryInfos.push_back(geoInfo);
        geometryMeshes.push_back(uint32_t(meshIdx));
      }
      meshIdx++;
    }
  }
  else
  {
    // 1:1 Mesh to CSFGeometry
    for(cgltf_size meshIdx = 0; meshIdx < gltfModel->meshes_count; meshIdx++)
    {
      meshGeometries.push_back(uint32_t(meshIdx));
      geometryMeshes.push_back(uint32_t(meshIdx));
    }
  }

  csf->numGeometries = (int)geometryMeshes.size();
  csf->geometries    = (CSFGeometry*)CSFileMemory_alloc(mem, sizeof(CSFGeometry) * csf->numGeometries, NULL);
  memset(csf->geometries, 0, sizeof(CSFGeometry) * csf->numGeometries);


  // create geometries
#pragma omp parallel for
  for(int outIdx = 0; outIdx < csf->numGeometries; outIdx++)
  {
    const cgltf_mesh& mesh    = gltfModel->meshes[geometryMeshes[outIdx]];
    CSFGeometry&      csfgeom = csf->geometries[outIdx];

    // count pass
    uint32_t vertexTotCount = 0;
    uint32_t indexTotCount  = 0;
    uint32_t partsTotCount  = 0;

    bool hasNormals   = false;
    bool hasTexcoords = false;
    for(cgltf_size p = 0; p < mesh.primitives_count; p++)
    {
      const cgltf_primitive& primitive = mesh.primitives[p];

      if(primitive.type != cgltf_primitive_type_triangles)
        continue;

      for(cgltf_size a = 0; a < primitive.attributes_count; a++)
      {
        const cgltf_accessor* accessor = primitive.attributes[a].data;

        switch(primitive.attributes[a].type)
        {
          case cgltf_attribute_type_position:
            vertexTotCount += uint32_t(accessor->count);
            break;
          case cgltf_attribute_type_normal:
            hasNormals = true;
            break;
          case cgltf_attribute_type_texcoord:
            if(primitive.attributes[a].index == 0)
            {
              hasTexcoords = true;
            }
            break;
          default:
            break;  // No matching csf attribute
        }
      }
      indexTotCount += uint32_t(primitive.indices->count);
      partsTotCount++;
    }

    // allocate all data
    csfgeom.numVertices = vertexTotCount;
    csfgeom.numParts    = partsTotCount;

    csfgeom.vertex = (float*)CSFileMemory_alloc(mem, sizeof(float) * 3 * vertexTotCount, nullptr);
    if(hasNormals)
    {
      csfgeom.normal = (float*)CSFileMemory_alloc(mem, sizeof(float) * 3 * vertexTotCount, nullptr);
    }
    if(hasTexcoords)
    {
      csfgeom.tex = (float*)CSFileMemory_alloc(mem, sizeof(float) * 2 * vertexTotCount, nullptr);
    }
    csfgeom.indexSolid = (uint32_t*)CSFileMemory_alloc(mem, sizeof(uint32_t) * indexTotCount, nullptr);
    csfgeom.parts      = (CSFGeometryPart*)CSFileMemory_alloc(mem, sizeof(CSFGeometryPart) * partsTotCount, nullptr);

    // fill pass
    indexTotCount  = 0;
    vertexTotCount = 0;
    partsTotCount  = 0;

    for(cgltf_size p = 0; p < mesh.primitives_count; p++)
    {
      const cgltf_primitive& primitive = mesh.primitives[p];

      if(primitive.type != cgltf_primitive_type_triangles)
        continue;

      CSFGeometryPart& csfpart = csfgeom.parts[partsTotCount++];

      uint32_t vertexCount = 0;

      for(cgltf_size a = 0; a < primitive.attributes_count; a++)
      {
        const cgltf_accessor*    accessor = primitive.attributes[a].data;
        const cgltf_buffer_view* view     = accessor->buffer_view;
        const uint8_t*           data     = reinterpret_cast<const uint8_t*>(view->buffer->data);
        data += accessor->offset + view->offset;

        switch(primitive.attributes[a].type)
        {
          case cgltf_attribute_type_position:
            vertexCount += uint32_t(accessor->count);

            for(cgltf_size i = 0; i < accessor->count; i++)
            {
              const float* vec                             = (const float*)(data + i * accessor->stride);
              csfgeom.vertex[(vertexTotCount + i) * 3 + 0] = vec[0];
              csfgeom.vertex[(vertexTotCount + i) * 3 + 1] = vec[1];
              csfgeom.vertex[(vertexTotCount + i) * 3 + 2] = vec[2];
            }
            break;
          case cgltf_attribute_type_normal:
            for(cgltf_size i = 0; i < accessor->count; i++)
            {
              const float* vec                             = (const float*)(data + i * accessor->stride);
              csfgeom.normal[(vertexTotCount + i) * 3 + 0] = vec[0];
              csfgeom.normal[(vertexTotCount + i) * 3 + 1] = vec[1];
              csfgeom.normal[(vertexTotCount + i) * 3 + 2] = vec[2];
            }
            hasNormals = true;
            break;
          case cgltf_attribute_type_texcoord:
            if(primitive.attributes[a].index == 0)
            {
              for(cgltf_size i = 0; i < accessor->count; i++)
              {
                cgltf_accessor_read_float(accessor, i, csfgeom.tex + (i + vertexTotCount) * 2, 2);
              }
            }
            break;
          default:
            break;  // No matching csf attribute
        }
      }

      {
        const cgltf_accessor*    accessor = primitive.indices;
        const cgltf_buffer_view* view     = accessor->buffer_view;
        const uint8_t*           data     = reinterpret_cast<const uint8_t*>(view->buffer->data);
        data += accessor->offset + view->offset;

        uint32_t indexBegin = indexTotCount;
        switch(accessor->component_type)
        {
          case cgltf_component_type_r_16:
            for(cgltf_size i = 0; i < accessor->count; i++)
            {
              const uint8_t* in                   = data + (i * accessor->stride);
              csfgeom.indexSolid[indexTotCount++] = *((const int16_t*)in) + vertexTotCount;
            }
            break;
          case cgltf_component_type_r_16u:
            for(cgltf_size i = 0; i < accessor->count; i++)
            {
              const uint8_t* in                   = data + (i * accessor->stride);
              csfgeom.indexSolid[indexTotCount++] = *((const uint16_t*)in) + vertexTotCount;
            }
            break;
          case cgltf_component_type_r_32u:
            for(cgltf_size i = 0; i < accessor->count; i++)
            {
              const uint8_t* in                   = data + (i * accessor->stride);
              csfgeom.indexSolid[indexTotCount++] = *((const uint32_t*)in) + vertexTotCount;
            }
            break;
          case cgltf_component_type_r_8:
            for(cgltf_size i = 0; i < accessor->count; i++)
            {
              const uint8_t* in                   = data + (i * accessor->stride);
              csfgeom.indexSolid[indexTotCount++] = *((const int8_t*)in) + vertexTotCount;
            }
            break;
          case cgltf_component_type_r_8u:
            for(cgltf_size i = 0; i < accessor->count; i++)
            {
              const uint8_t* in                   = data + (i * accessor->stride);
              csfgeom.indexSolid[indexTotCount++] = *((const uint8_t*)in) + vertexTotCount;
            }
            break;
          default:
            assert(0);
            break;
        }

        csfpart.numIndexSolid = indexTotCount - indexBegin;
      }

      vertexTotCount += vertexCount;

      csfpart.numIndexWire = 0;
      csfpart._deprecated  = 0;
    }

    csfgeom.numIndexSolid = (int)indexTotCount;

    CSFGeometry_setupDefaultChannels(&csfgeom);
  }

  // create flattened nodes
  csf->numNodes = 1;  // reserve for root
  csf->rootIDX  = 0;


  for(size_t i = 0; i < scene->nodes_count; i++)
  {
    CSFile_countGLTFNodes(csf, gltfModel, scene->nodes[i]);
  }

  csf->nodes = (CSFNode*)CSFileMemory_alloc(mem, sizeof(CSFNode) * csf->numNodes, nullptr);
  memset(csf->nodes, 0, sizeof(CSFNode) * csf->numNodes);

  csf->numNodes = 1;
  // root setup
  csf->nodes[0].geometryIDX = -1;
  csf->nodes[0].numChildren = (int)scene->nodes_count;
  csf->nodes[0].children    = (int*)CSFileMemory_alloc(mem, sizeof(int) * scene->nodes_count, nullptr);
  CSFMatrix_identity(csf->nodes[0].worldTM);
  CSFMatrix_identity(csf->nodes[0].objectTM);

  for(size_t i = 0; i < scene->nodes_count; i++)
  {
    csf->nodes[0].children[i] = CSFile_addGLTFNode(csf, gltfModel, meshGeometries.data(), mem, scene->nodes[i]);
  }

  CSFile_transform(csf);
  cgltf_free(gltfModel);

  *outcsf = csf;
  return CADSCENEFILE_NOERROR;
}

#endif