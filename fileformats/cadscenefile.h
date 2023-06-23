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

/// \nodoc (keyword to exclude this file from automatic README.md generation)

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string>

// use CSF_IMPLEMENTATION define prior including
// to add actual implementation to this project
//
// prior implementation include, supports following options
//
// CSF_SUPPORT_ZLIB         0/1 (uses zlib)
// CSF_SUPPORT_GLTF2        0/1 (uses cgltf)
// CSF_SUPPORT_FILEMAPPING  0/1 (default uses nvh)
// specify CSF_FILEMAPPING_READTYPE, otherwise defaults to nvh::FileReadMapping

extern "C" {

#ifdef _WIN32
#if defined(CSFAPI_EXPORTS)
#define CSFAPI __declspec(dllexport)
#elif defined(CSFAPI_IMPORTS)
#define CSFAPI __declspec(dllimport)
#else
#define CSFAPI
#endif
#else
#define CSFAPI
#endif

enum
{
  CADSCENEFILE_VERSION = 6,
  // binary compatibility
  CADSCENEFILE_VERSION_COMPAT = 2,

  CADSCENEFILE_VERSION_BASE = 1,
  // add support for material meta information
  CADSCENEFILE_VERSION_MATERIAL = 2,
  // add support for fileflags
  CADSCENEFILE_VERSION_FILEFLAGS = 3,
  // changes CSFNodePart.lineWidth to CSFNodePart.nodeIDX
  CADSCENEFILE_VERSION_PARTNODEIDX = 4,
  // adds support for meta pointers
  CADSCENEFILE_VERSION_META = 5,
  // adds support for vertex and part channels
  CADSCENEFILE_VERSION_GEOMETRYCHANNELS = 6,

  CADSCENEFILE_NOERROR         = 0,  // Success.
  CADSCENEFILE_ERROR_NOFILE    = 1,  // The file did not exist or an I/O operation failed.
  CADSCENEFILE_ERROR_VERSION   = 2,  // The file had an invalid header.
  CADSCENEFILE_ERROR_OPERATION = 3,  // Called an operation that cannot be called on this object.
  CADSCENEFILE_ERROR_INVALID   = 4,  // The file contains invalid data.

  // node tree, no multiple references to same node
  // always set, as no application supports non-unique case
  CADSCENEFILE_FLAG_UNIQUENODES = 1,

  // all triangles/lines are using strips instead of index lists
  // never set, only special purpose file/application made use of this
  CADSCENEFILE_FLAG_STRIPS = 2,

  // file has meta node array
  CADSCENEFILE_FLAG_META_NODE = 4,
  // file has meta geometry array
  CADSCENEFILE_FLAG_META_GEOMETRY = 8,
  // file has meta file pointer
  CADSCENEFILE_FLAG_META_FILE = 16,

  // number of uint32_t per GUID
  CADSCENEFILE_LENGTH_GUID   = 4,
  CADSCENEFILE_LENGTH_STRING = 128,
};

#define CADSCENEFILE_RESTARTINDEX (~0)

/*

  Version History
  ---------------

  1 initial
  2 !binary break
    material allows custom payload
    deprecate geometry matrix
    deprecate geometry part vertex
  3 hasUniqueNodes became a bitflag
    added strip indices flag, file is either strip or non-strip
  4 lineWidth changed to nodeIDX, allows per-part sub-transforms. sub-transforms should be below
    object in hierarchy and not affect geometry bbox
  5 meta information handling
  6 vertex channels using deprecated geometry matrix space

  Example Structure
  -----------------
  
  CSFMaterials   
  0 Red          
  1 Green        
  2 Blue         
                
                 
  CSFGeometries (index,vertex & "parts")
  0 Box
  1 Cylinder
  e.g. parts  (CSFGeometryPart defines a region in the indexbuffers of CSFGeometry):
    0 mantle
    1 top cap
    2 bottom cap

  There is no need to have multiple parts, but for experimenting with
  rendering some raw CAD data, having each patch/surface feature individually
  can be useful. A typical CAD file with use one CSFGeometry per Solid (e.g. cube)
  and multiple CSFGeometryParts for each "feature" (face of a cube etc.).

  CSFNodes  (hierarchy of nodes)

  A node can also reference a geometry, this way the same geometry
  data can be instanced multiple times.
  If the node references geometry, then it must have an
  array of "CSFNodePart" matching referenced CSFGeometryParts.
  The CSFNodePart encodes the materials/matrix as well as its
  "visibility" (active) state.


  CSFoffset - nameOFFSET variables
  --------------------------------

  CSFoffset is only indirectly used during save and load operations.
  As end user you can ignore the various "nameOFFSET" variables
  in all the unions, as well as the pointers array.
  */

typedef struct _CSFLoaderConfig
{
  // read only access to loaded csf data
  // means we can use filemappings
  // default = false
  // the primary structs are still allocated for write access,
  // but all pointers within them are mapped.
  int secondariesReadOnly;

  // uses hashes of geometry data to figure out what gltf mesh
  // data is re-used under different materials, and can
  // therefore be mapped to a CSF geometry.
  // default = true
  // (only relevant if CSF_SUPPORT_GLTF2 was enabled, otherwise
  // ignored)
  int gltfFindUniqueGeometries;
} CSFLoaderConfig;

typedef unsigned long long CSFoffset;
typedef unsigned int       CSFGuid[CADSCENEFILE_LENGTH_GUID];

// optional, if one wants to pack
// additional meta information into the bytes arrays
typedef struct _CSFBytePacket
{
  CSFGuid guid;
  int     numBytes;  // includes size of this header
} CSFBytePacket;

#define CSFGUID_MATERIAL_GLTF2                                                                                         \
  {                                                                                                                    \
    0, 0, 0, 2                                                                                                         \
  }

typedef struct _CSFMaterialGLTF2Texture
{
  char     name[CADSCENEFILE_LENGTH_STRING];
  uint16_t minFilter;
  uint16_t magFilter;
  uint16_t wrapS;
  uint16_t wrapT;
  float    scale;
  int      coord;
  int      xformUsed;
  int      xformCoord;
  float    xformOffset[2];
  float    xformScale[2];
  float    xformRotation;
} CSFMaterialGLTF2Texture;

typedef struct _CSFMaterialGLTF2Meta
{
  CSFBytePacket packet;

  //-1: unlit
  // 0: metallicRoughness
  // 1: specularGlossiness
  int   shadingModel;
  int   doubleSided;
  int   alphaMode;
  float alphaCutoff;
  float emissiveFactor[3];

  union
  {
    struct
    {
      float baseColorFactor[4];
      float metallicFactor;
      float roughnessFactor;

      _CSFMaterialGLTF2Texture baseColorTexture;
      _CSFMaterialGLTF2Texture metallicRoughnessTexture;
    };

    struct
    {
      float diffuseFactor[4];
      float specularFactor[3];
      float glossinessFactor;

      _CSFMaterialGLTF2Texture diffuseTexture;
      _CSFMaterialGLTF2Texture specularGlossinessTexture;
    };
  };

  _CSFMaterialGLTF2Texture occlusionTexture;
  _CSFMaterialGLTF2Texture normalTexture;
  _CSFMaterialGLTF2Texture emissiveTexture;
} CSFMaterialGLTF2Meta;

typedef struct _CSFMeta
{
  char      name[CADSCENEFILE_LENGTH_STRING];
  int       flags;
  CSFoffset numBytes;
  union
  {
    CSFoffset      bytesOFFSET;
    unsigned char* bytes;
  };
} CSFMeta;

typedef struct _CSFMaterial
{
  char  name[CADSCENEFILE_LENGTH_STRING];
  float color[4];
  int   type;  // arbitrary data

  // FIXME should move meta outside material, but breaks binary
  // compatibility
  int numBytes;
  union
  {
    CSFoffset      bytesOFFSET;
    unsigned char* bytes;
  };
} CSFMaterial;

typedef enum _CSFGeometryPartChannel
{
  // CSFGeometryPartChannelBbox
  CSFGEOMETRY_PARTCHANNEL_BBOX,
  CSFGEOMETRY_PARTCHANNELS,
} CSFGeometryPartChannel;

typedef struct _CSFGeometryPartBbox
{
  float min[3];
  float max[3];
} CSFGeometryPartBbox;

typedef enum _CSFGeometryNormalChannel
{
  // float[3]
  // can extend but must not change order
  CSFGEOMETRY_NORMALCHANNEL_NORMAL,
  CSFGEOMETRY_NORMALCHANNELS,
} CSFGeometryNormalChannel;

typedef enum _CSFGeometryTexChannel
{
  // float[2]
  // can extend but must not change order
  CSFGEOMETRY_TEXCHANNEL_GENERIC,
  CSFGEOMETRY_TEXCHANNEL_LIGHTMAP,
  CSFGEOMETRY_TEXCHANNELS,
} CSFGeometryTexChannel;

typedef enum _CSFGeometryAuxChannel
{
  // float[4]
  // can extend but must not change order
  CSFGEOMETRY_AUXCHANNEL_RADIANCE,
  CSFGEOMETRY_AUXCHANNELS,
} CSFGeometryAuxChannel;

typedef struct _CSFGeometryPart
{
  int _deprecated;    // deprecated
  int numIndexSolid;  // number of triangle indices that the part uses
  int numIndexWire;   // number of line indices that the part uses
} CSFGeometryPart;

typedef struct _CSFGeometry
{
  /*
    Each Geometry stores:
    - optional index buffer triangles (solid)
    - optional index buffer for lines (wire)
    
      At least one of the index buffers must be
      present.

    - vertex buffer (mandatory)
    - optional vertex attribute (normal,tex,aux) buffers

      Each vertex channel is stored in full for all vertices before
      subsequent channels. Use the channel getter functions.
      Auxiliar data uses the auxStorageOrder array to encode
      what and in which order channels are stored.
    
    - parts array

      index buffer:  { part 0 ....,  part 1.., part 2......., ...}

      Each geometry part represents a range within the
      index buffers. The parts are stored ascending
      in the index buffer. To get the starting 
      offset, use the sum of the previous parts.

    - perpart array

      Allows storing auxiliar per-part channel data (CSFGeometryPartChannel)
      perpartStorageOrder array encodes what and in which
      order the channels are stored.
      Use the channel getter function and size functions.
    */

  // ordering of variable is a bit weird due to keeping binary
  // compatibility with past versions
  float _deprecated[4];

  // VERSION < CADSCENEFILE_VERSION
  /////////////////////////////////////////////////
  int numNormalChannels;
  int numTexChannels;
  int numAuxChannels;
  int numPartChannels;

  union
  {
    // numAuxChannels
    CSFoffset               auxStorageOrderOFFSET;
    _CSFGeometryAuxChannel* auxStorageOrder;
  };
  union
  {
    // 4 * numVertices * numAuxChannels
    CSFoffset auxOFFSET;
    float*    aux;
  };

  union
  {
    // numPartChannels
    CSFoffset               perpartStorageOrderOFFSET;
    CSFGeometryPartChannel* perpartStorageOrder;
  };
  union
  {
    // sized implicitly use CSFGeometry_getPerPartSize functions
    CSFoffset      perpartOFFSET;
    unsigned char* perpart;
  };

  // VERSION < CADSCENEFILE_VERSION_GEOMETRYCHANNELS
  /////////////////////////////////////////////////
  int numParts;
  int numVertices;
  int numIndexSolid;
  int numIndexWire;

  union
  {
    // 3 components * numVertices
    CSFoffset vertexOFFSET;
    float*    vertex;
  };

  union
  {
    // 3 components * numVertices * numNormalChannels
    // canonical order as defined by CSFGeometryNormalChannel
    CSFoffset normalOFFSET;
    float*    normal;
  };

  union
  {
    // 2 components * numVertices * numTexChannels
    // canonical order is defined in CSFGeometryTexChannel
    CSFoffset texOFFSET;
    float*    tex;
  };

  union
  {
    CSFoffset     indexSolidOFFSET;
    unsigned int* indexSolid;
  };

  union
  {
    CSFoffset     indexWireOFFSET;
    unsigned int* indexWire;
  };

  union
  {
    CSFoffset        partsOFFSET;
    CSFGeometryPart* parts;
  };
} CSFGeometry;

typedef struct _CSFNodePart
{
  // CSFNodePart defines the state for the corresponding CSFGeometryPart

  // allow setting visibility of a part, 0 or 1
  // should always be 1
  int active;

  // index into csf->materials
  // must alwaye be >= 0
  // ideally all parts of a node use the same material
  int materialIDX;

  // index into csf->nodes
  // if -1 it uses the matrix of the node it belongs to.
  // This is highly recommended to be used.
  // if >= 0 the nodeIDX should be a child of the
  // part's node.
  int nodeIDX;
} CSFNodePart;

typedef struct _CSFNode
{
  /*
    CSFNodes form a hierarchy, starting at
    csf->nodes[csf->rootIDX].

    Each node can have children. If CADSCENEFILE_FLAG_UNIQUENODES is set
    the hierarchy is a tree.

    Each Node stores:
    - the object transform (relative to parent)
    - the world transform (final transform to get from object to world space
      node.worldTM = node.parent.worldTM * node.objectTM;
    - optional geometry reference
    - optional array of node children
    - the parts array is mandatory if a geometry is referenced
      and must be sized to form a 1:1 correspondence to the geoemtry's parts.
    */

  float objectTM[16];
  float worldTM[16];

  // index into csf->geometries
  // can be -1 (no geometry used) or >= 0
  int geometryIDX;

  // if geometryIDX >= 0, must match geometry's numParts
  int numParts;
  int numChildren;
  union
  {
    // must exist if geometryIDX >= 0, null otherwise
    CSFoffset    partsOFFSET;
    CSFNodePart* parts;
  };
  union
  {
    // array of indices into csf->nodes
    // each must be >= 0
    // array must be != null if numChildren is > 0
    CSFoffset childrenOFFSET;
    int*      children;
  };
} CSFNode;


typedef struct _CSFile
{
  int magic;
  int version;

  // see CADSCENEFILE_FLAG_??
  unsigned int fileFlags;

  // used internally for load & save operations, can be ignored
  int numPointers;

  int numGeometries;
  int numMaterials;
  int numNodes;

  // index into csf->nodes where the root node is located
  // must be >= 0
  int rootIDX;

  union
  {
    // the pointers are used internally for load & save operations
    // no need to specify prior save
    // no need to access pos load

    CSFoffset  pointersOFFSET;
    CSFoffset* pointers;
  };

  union
  {
    CSFoffset    geometriesOFFSET;
    CSFGeometry* geometries;
  };

  union
  {
    CSFoffset    materialsOFFSET;
    CSFMaterial* materials;
  };

  union
  {
    CSFoffset nodesOFFSET;
    CSFNode*  nodes;
  };

  //----------------------------------
  // Only available for version >= CADSCENEFILE_VERSION_META and if flag is set.
  // Use the getter functions to access, they return null if the criteria aren't met.
  // Otherwise this memory will overlap with different content.

  union
  {
    // one per node if CADSCENEFILE_FLAG_META_NODE is set
    CSFoffset nodeMetasOFFSET;
    CSFMeta*  nodeMetas;
  };
  union
  {
    // one per geometry if CADSCENEFILE_FLAG_META_GEOMETRY is set
    CSFoffset geometryMetasOFFSET;
    CSFMeta*  geometryMetas;
  };
  union
  {
    // one per file if CADSCENEFILE_FLAG_META_FILE is set
    CSFoffset fileMetaOFFSET;
    CSFMeta*  fileMeta;
  };
  //----------------------------------
} CSFile;

typedef struct CSFileMemory_s* CSFileMemoryPTR;

// Internal allocation wrapper
// also handles details for loading operations
CSFAPI CSFileMemoryPTR CSFileMemory_new();
CSFAPI CSFileMemoryPTR CSFileMemory_newCfg(const CSFLoaderConfig* config);

// alloc functions are thread-safe
// fill if provided must provide sz bytes
CSFAPI void* CSFileMemory_alloc(CSFileMemoryPTR mem, size_t sz, const void* fill);
// fillPartial if provided must provide szPartial bytes and szPartial <= sz
CSFAPI void* CSFileMemory_allocPartial(CSFileMemoryPTR mem, size_t sz, size_t szPartial, const void* fillPartial);
// all allocations within will be freed
CSFAPI void CSFileMemory_delete(CSFileMemoryPTR mem);

// The data pointed to is modified, therefore the raw load operation can be executed only once.
// It must be preserved for as long as the csf and its internals are accessed
CSFAPI int CSFile_loadRaw(CSFile** outcsf, size_t sz, void* data);

// All allocations are done within the provided file memory.
// It must be preserved for as long as the csf and its internals are accessed
CSFAPI int CSFile_load(CSFile** outcsf, const char* filename, CSFileMemoryPTR mem);

CSFAPI int CSFile_save(const CSFile* csf, const char* filename);

// sets all content of _deprecated to zero, automatically done at load
// recommended to be done prior safe
CSFAPI void CSFile_clearDeprecated(CSFile* csf);

// sets up single normal/tex channel based on array existence
CSFAPI void CSFile_setupDefaultChannels(CSFile* csf);
CSFAPI void CSFGeometry_setupDefaultChannels(CSFGeometry* geo);


// returns vec3*numVertices
CSFAPI const float* CSFGeometry_getNormalChannel(const CSFGeometry* geo, CSFGeometryNormalChannel channel);
// returns vec2*numVertices
CSFAPI const float* CSFGeometry_getTexChannel(const CSFGeometry* geo, CSFGeometryTexChannel texChannel);
// returns vec4*numVertices
CSFAPI const float* CSFGeometry_getAuxChannel(const CSFGeometry* geo, CSFGeometryAuxChannel channel);
// returns arbitrary struct array * numParts
CSFAPI const void* CSFGeometry_getPartChannel(const CSFGeometry* geo, CSFGeometryPartChannel channel);

// accumulates partchannel sizes and multiplies with geo->numParts
CSFAPI size_t CSFGeometry_getPerPartSize(const CSFGeometry* geo);
// accumulates partchannel sizes and multiplies with provided numParts
CSFAPI size_t CSFGeometry_getPerPartRequiredSize(const CSFGeometry* geo, int numParts);
CSFAPI size_t CSFGeometry_getPerPartRequiredOffset(const CSFGeometry* geo, int numParts, CSFGeometryPartChannel channel);
// single element size
CSFAPI size_t CSFGeometryPartChannel_getSize(CSFGeometryPartChannel channel);


// safer to use these
CSFAPI const CSFMeta* CSFile_getNodeMetas(const CSFile* csf);
CSFAPI const CSFMeta* CSFile_getGeometryMetas(const CSFile* csf);
CSFAPI const CSFMeta* CSFile_getFileMeta(const CSFile* csf);

CSFAPI const CSFBytePacket* CSFile_getMetaBytePacket(const CSFMeta* meta, CSFGuid guid);
CSFAPI const CSFBytePacket* CSFile_getMaterialBytePacket(const CSFile* csf, int materialIDX, CSFGuid guid);

CSFAPI int CSFile_transform(CSFile* csf);  // requires unique nodes

// can support gltf/.gz if appropraite CSF_SUPPORT was set
CSFAPI int CSFile_loadExt(CSFile** outcsf, const char* filename, CSFileMemoryPTR mem);
CSFAPI int CSFile_saveExt(CSFile* csf, const char* filename);

CSFAPI void CSFMatrix_identity(float*);
};

//////////////////////////////////////////////////////////////////////////
#if defined(CSF_IMPLEMENTATION)
#include <assert.h>
#include <map>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#if CSF_SUPPORT_ZLIB
#include <zlib.h>
#endif

#include <mutex>

#include <stddef.h>  // for memcpy
#include <string.h>  // for memcpy

#if CSF_SUPPORT_FILEMAPPING
#ifndef CSF_FILEMAPPING_READTYPE
#include <nvh/filemapping.hpp>
#define CSF_FILEMAPPING_READTYPE nvh::FileReadMapping
#endif
#endif

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
  CSFLoaderConfig m_config;

  std::vector<void*> m_allocations;
  std::mutex         m_mutex;

#if CSF_SUPPORT_FILEMAPPING
  std::vector<CSF_FILEMAPPING_READTYPE> m_readMappings;
#endif

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

  template <typename T>
  T* allocT(size_t size, const T* indata, size_t indataSize = 0)
  {
    return (T*)alloc(size, indata, indataSize);
  }

  CSFileMemory_s()
  {
    m_config.secondariesReadOnly = 0;
#if CSF_SUPPORT_GLTF2
    m_config.gltfFindUniqueGeometries = 1;
#endif
  }

  ~CSFileMemory_s()
  {
    for(size_t i = 0; i < m_allocations.size(); i++)
    {
      free(m_allocations[i]);
    }
#if CSF_SUPPORT_FILEMAPPING
    m_readMappings.clear();
#endif
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
     || (csf->numNodes < 0)       //
     || (csf->rootIDX < 0))
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
    if((csf->fileFlags & CADSCENEFILE_FLAG_META_NODE) != 0)
    {
      minimum_file_length =
          CSFile_max(minimum_file_length, CSFile_checkedAdd(csf->nodeMetasOFFSET, csf->numNodes * sizeof(CSFMeta), overflow));
    }

    if((csf->fileFlags & CADSCENEFILE_FLAG_META_GEOMETRY) != 0)
    {
      minimum_file_length =
          CSFile_max(minimum_file_length, CSFile_checkedAdd(csf->fileMetaOFFSET, csf->numNodes * sizeof(CSFMeta), overflow));
    }

    if((csf->fileFlags & CADSCENEFILE_FLAG_META_FILE) != 0)
    {
      minimum_file_length =
          CSFile_max(minimum_file_length, CSFile_checkedAdd(csf->geometryMetasOFFSET, csf->numNodes * sizeof(CSFMeta), overflow));
    }
  }

  if(overflow)
  {
    return 0;
  }

  return minimum_file_length;
}


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
    fixPointer(geo.indexSolid, geo.indexSolidOFFSET, base);
    fixPointer(geo.indexWire, geo.indexWireOFFSET, base);
    fixPointer(geo.tex, geo.texOFFSET, base);
    fixPointer(geo.parts, geo.partsOFFSET, base);
    fixPointer(geo.auxStorageOrder, geo.auxStorageOrderOFFSET, base);
    fixPointer(geo.aux, geo.auxOFFSET, base);
    fixPointer(geo.perpart, geo.perpartOFFSET, base);
    fixPointer(geo.perpartStorageOrder, geo.perpartStorageOrderOFFSET, base);
  }
  for(int n = 0; n < csf->numNodes; n++)
  {
    CSFNode& node = csf->nodes[n];
    fixPointer(node.children, node.childrenOFFSET, base);
    fixPointer(node.parts, node.partsOFFSET, base);
  }
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
    for(int n = 0; n < csf->numNodes; n++)
    {
      CSFMeta& meta = csf->nodeMetas[n];
      fixPointer(meta.bytes, meta.bytesOFFSET, base);
    }
  }
  if(CSFile_getFileMeta(csf))
  {
    CSFMeta& meta = csf->fileMeta[0];
    fixPointer(meta.bytes, meta.bytesOFFSET, base);
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

  for(int m = 0; m < numElements; m++)
  {
    const CSFMeta& meta = meta_ptr[m];
    if(!CSFile_validateRange(meta.bytes, meta.numBytes, csf, csf_size))
      return false;
  }

  return true;
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

      bool         overflow = false;
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
  for(int n = 0; n < csf->numNodes; n++)
  {
    CSFNode& node = csf->nodes[n];
    if(node.geometryIDX >= 0)
    {
      if(node.geometryIDX >= csf->numGeometries)
        return false;
      CSFGeometry& geo = csf->geometries[node.geometryIDX];
      if(node.numParts != geo.numParts)
        return false;
      if(!CSFile_validateRange(node.parts, node.numParts, csf, csf_size))
        return false;
    }

    if(!CSFile_validateRange(node.children, node.numChildren, csf, csf_size))
      return false;
  }

  if(csf->version >= CADSCENEFILE_VERSION_META)
  {
    if((csf->fileFlags & CADSCENEFILE_FLAG_META_NODE) != 0)
    {
      if(!CSFile_validateMetaArray(csf->nodeMetas, csf->numNodes, csf, csf_size))
        return false;
    }

    if((csf->fileFlags & CADSCENEFILE_FLAG_META_GEOMETRY) != 0)
    {
      if(!CSFile_validateMetaArray(csf->geometryMetas, csf->numNodes, csf, csf_size))
        return false;
    }

    if((csf->fileFlags & CADSCENEFILE_FLAG_META_FILE) != 0)
    {
      if(!CSFile_validateMetaArray(csf->fileMeta, csf->numNodes, csf, csf_size))
        return false;
    }
  }

  return true;
}

CSFAPI int CSFile_loadRaw(CSFile** outcsf, size_t size, void* dataraw)
{
  char*   data = (char*)dataraw;
  CSFile* csf  = (CSFile*)data;
  *outcsf      = 0;

  if(size < sizeof(CSFile) || CSFile_invalidVersion(csf))
  {
    return CADSCENEFILE_ERROR_VERSION;
  }

  if(csf->version < CADSCENEFILE_VERSION_FILEFLAGS)
  {
    csf->fileFlags = csf->fileFlags ? CADSCENEFILE_FLAG_UNIQUENODES : 0;
  }

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
  if(csf->pointersOFFSET % alignof(CSFoffset) != 0)
  {
    return CADSCENEFILE_ERROR_INVALID;
  }
  csf->pointersOFFSET += (CSFoffset)csf;
  for(int i = 0; i < csf->numPointers; i++)
  {
    const CSFoffset location_of_ptr_from_start = csf->pointers[i];
    if((location_of_ptr_from_start > static_cast<CSFoffset>(size) - sizeof(CSFoffset*))  //
       || (location_of_ptr_from_start < offsetof(CSFile, geometries))                    //
       || (location_of_ptr_from_start % alignof(CSFoffset) != 0))
    {
      return CADSCENEFILE_ERROR_INVALID;
    }

    CSFoffset* ptr = (CSFoffset*)(data + location_of_ptr_from_start);
    *(ptr) += (CSFoffset)csf;
  }

  // Check that pointers really contained all pointers in the file.
  if(!CSFile_validateAllRanges(csf, size))
  {
    return CADSCENEFILE_ERROR_INVALID;
  }

  if(csf->version < CADSCENEFILE_VERSION_PARTNODEIDX)
  {
    for(int i = 0; i < csf->numNodes; i++)
    {
      // csf->nodes[i].parts is only guaranteed to be valid if geometryIDX >= 0:
      if(csf->nodes[i].geometryIDX >= 0)
      {
        for(int p = 0; p < csf->nodes[i].numParts; p++)
        {
          csf->nodes[i].parts[p].nodeIDX = -1;
        }
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

#if CSF_SUPPORT_FILEMAPPING
int CSFile_loadReadOnly(CSFile** outcsf, const char* filename, CSFileMemoryPTR mem)
{
  CSF_FILEMAPPING_READTYPE file;
  if(!file.open(filename))
  {
    return CADSCENEFILE_ERROR_NOFILE;
  }

  const uint8_t* base = (const uint8_t*)file.data();

  // allocate the primary arrays
  CSFile* csf = mem->allocT(sizeof(CSFile), (const CSFile*)base, sizeof(CSFile));

  csf->materials = mem->allocT(sizeof(CSFMaterial) * csf->numMaterials, (const CSFMaterial*)(base + csf->materialsOFFSET));
  csf->geometries = mem->allocT(sizeof(CSFGeometry) * csf->numGeometries, (const CSFGeometry*)(base + csf->geometriesOFFSET));
  csf->nodes       = mem->allocT(sizeof(CSFNode) * csf->numNodes, (const CSFNode*)(base + csf->nodesOFFSET));
  csf->pointers    = 0;
  csf->numPointers = 0;
  if(CSFile_getGeometryMetas(csf))
  {
    csf->geometryMetas = mem->allocT(sizeof(CSFMeta) * csf->numGeometries, (const CSFMeta*)(base + csf->geometryMetasOFFSET));
  }
  if(CSFile_getNodeMetas(csf))
  {
    csf->nodeMetas = mem->allocT(sizeof(CSFMeta) * csf->numNodes, (const CSFMeta*)(base + csf->nodeMetasOFFSET));
  }
  if(CSFile_getFileMeta(csf))
  {
    csf->fileMeta = mem->allocT(sizeof(CSFMeta), (const CSFMeta*)(base + csf->fileMetaOFFSET));
  }
  if(csf->version < CADSCENEFILE_VERSION_GEOMETRYCHANNELS)
  {
    CSFile_setupDefaultChannels(csf);
  }

  CSFile_fixSecondaryPointers(csf, const_cast<void*>((const void*)base));

  mem->m_readMappings.push_back(std::move(file));

  *outcsf = csf;
  return CADSCENEFILE_NOERROR;
}
#endif

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

#if CSF_SUPPORT_FILEMAPPING
  if(mem->m_config.secondariesReadOnly)
  {
    fclose(file);
    return CSFile_loadReadOnly(outcsf, filename, mem);
  }
#endif

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

  char* data           = (char*)mem->alloc(size);
  const size_t numRead = FREAD(data, size, size, 1, file);
  fclose(file);

  if(numRead != size)
  {
    return CADSCENEFILE_ERROR_NOFILE;
  }

  return CSFile_loadRaw(outcsf, size, data);
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

    return CSFile_loadRaw(outcsf, sizeshould, data);
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


#if CSF_SUPPORT_ZLIB
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
      : m_file(file)
      , m_current(0)
  {
  }

  size_t handleAlignment(size_t alignment)
  {
    // always align to 4 bytes at least
    alignment   = alignment < 4 ? 4 : alignment;
    size_t rest = m_current % alignment;
    if(rest)
    {
      static const uint32_t padBufferSize            = 16;
      static const uint8_t  padBuffer[padBufferSize] = {0};
      size_t                padding                  = alignment - rest;

      m_current += padding;

      while(padding)
      {
        size_t padCurrent = padding > padBufferSize ? padBufferSize : padding;
        m_file.write(padBuffer, padCurrent);

        padding -= padCurrent;
      }
    }

    return m_current;
  }

  size_t store(const void* data, size_t dataSize, size_t alignment)
  {
    size_t last = handleAlignment(alignment);
    m_file.write(data, dataSize);

    m_current += dataSize;
    return last;
  }

  size_t storeLocation(size_t location, const void* data, size_t dataSize, size_t alignment)
  {
    size_t last = handleAlignment(alignment);
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

    m_file.seek(0, SEEK_END);
    CSFoffset offset = (CSFoffset)handleAlignment(alignof(CSFoffset));
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
  mgr.store(&dump, sizeof(CSFile), alignof(CSFile));

  // iterate the objects
  {
    size_t geomOFFSET = mgr.storeLocation(offsetof(CSFile, geometriesOFFSET), csf->geometries,
                                          sizeof(CSFGeometry) * csf->numGeometries, alignof(CSFGeometry));

    for(int i = 0; i < csf->numGeometries; i++, geomOFFSET += sizeof(CSFGeometry))
    {
      const CSFGeometry* geo = csf->geometries + i;

      if(geo->vertex && geo->numVertices)
      {
        mgr.storeLocation(geomOFFSET + offsetof(CSFGeometry, vertexOFFSET), geo->vertex,
                          sizeof(float) * 3 * geo->numVertices, alignof(float));
      }
      if(geo->normal && geo->numVertices)
      {
        mgr.storeLocation(geomOFFSET + offsetof(CSFGeometry, normalOFFSET), geo->normal,
                          sizeof(float) * 3 * geo->numVertices * geo->numNormalChannels, alignof(float));
      }
      if(geo->tex && geo->numVertices)
      {
        mgr.storeLocation(geomOFFSET + offsetof(CSFGeometry, texOFFSET), geo->tex,
                          sizeof(float) * 2 * geo->numVertices * geo->numTexChannels, alignof(float));
      }

      if(geo->aux && geo->numVertices)
      {
        mgr.storeLocation(geomOFFSET + offsetof(CSFGeometry, auxOFFSET), geo->aux,
                          sizeof(float) * 4 * geo->numVertices * geo->numAuxChannels, alignof(float));
      }
      if(geo->auxStorageOrder && geo->numAuxChannels)
      {
        mgr.storeLocation(geomOFFSET + offsetof(CSFGeometry, auxStorageOrderOFFSET), geo->auxStorageOrder,
                          sizeof(CSFGeometryAuxChannel) * geo->numAuxChannels, alignof(CSFGeometryAuxChannel));
      }

      if(geo->indexSolid && geo->numIndexSolid)
      {
        mgr.storeLocation(geomOFFSET + offsetof(CSFGeometry, indexSolidOFFSET), geo->indexSolid,
                          sizeof(int) * geo->numIndexSolid, alignof(int));
      }
      if(geo->indexWire && geo->numIndexWire)
      {
        mgr.storeLocation(geomOFFSET + offsetof(CSFGeometry, indexWireOFFSET), geo->indexWire,
                          sizeof(int) * geo->numIndexWire, alignof(int));
      }

      if(geo->perpartStorageOrder && geo->numPartChannels)
      {
        mgr.storeLocation(geomOFFSET + offsetof(CSFGeometry, perpartStorageOrder), geo->perpartStorageOrder,
                          sizeof(CSFGeometryPartChannel) * geo->numPartChannels, alignof(CSFGeometryAuxChannel));
      }
      if(geo->perpart && geo->numPartChannels)
      {
        mgr.storeLocation(geomOFFSET + offsetof(CSFGeometry, perpart), geo->perpart, CSFGeometry_getPerPartSize(geo), 16);
      }

      if(geo->parts && geo->numParts)
      {
        mgr.storeLocation(geomOFFSET + offsetof(CSFGeometry, partsOFFSET), geo->parts,
                          sizeof(CSFGeometryPart) * geo->numParts, alignof(CSFGeometryPart));
      }
    }
  }


  {
    size_t matOFFSET = mgr.storeLocation(offsetof(CSFile, materialsOFFSET), csf->materials,
                                         sizeof(CSFMaterial) * csf->numMaterials, alignof(CSFMaterial));

    for(int i = 0; i < csf->numMaterials; i++, matOFFSET += sizeof(CSFMaterial))
    {
      const CSFMaterial* mat = csf->materials + i;
      if(mat->bytes && mat->numBytes)
      {
        mgr.storeLocation(matOFFSET + offsetof(CSFMaterial, bytesOFFSET), mat->bytes,
                          sizeof(unsigned char) * mat->numBytes, alignof(unsigned char));
      }
    }
  }

  {
    size_t nodeOFFSET =
        mgr.storeLocation(offsetof(CSFile, nodesOFFSET), csf->nodes, sizeof(CSFNode) * csf->numNodes, alignof(CSFNode));

    for(int i = 0; i < csf->numNodes; i++, nodeOFFSET += sizeof(CSFNode))
    {
      const CSFNode* node = csf->nodes + i;
      if(node->parts && node->numParts)
      {
        mgr.storeLocation(nodeOFFSET + offsetof(CSFNode, partsOFFSET), node->parts,
                          sizeof(CSFNodePart) * node->numParts, alignof(CSFNodePart));
      }
      if(node->children && node->numChildren)
      {
        mgr.storeLocation(nodeOFFSET + offsetof(CSFNode, childrenOFFSET), node->children,
                          sizeof(int) * node->numChildren, alignof(int));
      }
    }
  }

  if(CSFile_getNodeMetas(csf))
  {
    size_t metaOFFSET = mgr.storeLocation(offsetof(CSFile, nodeMetasOFFSET), csf->nodeMetas,
                                          sizeof(CSFMeta) * csf->numNodes, alignof(CSFMeta));

    for(int i = 0; i < csf->numNodes; i++, metaOFFSET += sizeof(CSFMeta))
    {
      const CSFMeta* meta = csf->nodeMetas + i;
      if(meta->bytes && meta->numBytes)
      {
        mgr.storeLocation(metaOFFSET + offsetof(CSFMeta, bytesOFFSET), meta->bytes,
                          sizeof(unsigned char) * meta->numBytes, alignof(unsigned char));
      }
    }
  }

  if(CSFile_getGeometryMetas(csf))
  {
    size_t metaOFFSET = mgr.storeLocation(offsetof(CSFile, geometryMetasOFFSET), csf->geometryMetas,
                                          sizeof(CSFMeta) * csf->numGeometries, alignof(CSFMeta));

    for(int i = 0; i < csf->numNodes; i++, metaOFFSET += sizeof(CSFMeta))
    {
      const CSFMeta* meta = csf->geometryMetas + i;
      if(meta->bytes && meta->numBytes)
      {
        mgr.storeLocation(metaOFFSET + offsetof(CSFMeta, bytesOFFSET), meta->bytes,
                          sizeof(unsigned char) * meta->numBytes, alignof(unsigned char));
      }
    }
  }

  if(CSFile_getFileMeta(csf))
  {
    size_t metaOFFSET = mgr.storeLocation(offsetof(CSFile, fileMetaOFFSET), csf->fileMeta, sizeof(CSFMeta), alignof(CSFMeta));

    {
      const CSFMeta* meta = csf->fileMeta;
      if(meta->bytes && meta->numBytes)
      {
        mgr.storeLocation(metaOFFSET + offsetof(CSFMeta, bytesOFFSET), meta->bytes,
                          sizeof(unsigned char) * meta->numBytes, alignof(unsigned char));
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
    return CSFile_saveInternal<OutputFILE>(csf, filename);
  }
}

static inline void Matrix44Copy(float* __restrict dst, const float* __restrict a)
{
  memcpy(dst, a, sizeof(float) * 16);
}

static inline void Matrix44MultiplyFull(float* __restrict clip, const float* __restrict proj, const float* __restrict modl)
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

static void CSFile_transformHierarchy(CSFile* csf, CSFNode* __restrict node, CSFNode* __restrict parent)
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
    CSFNode* __restrict child = csf->nodes + node->children[i];
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

#if CSF_SUPPORT_FILEMAPPING

struct MappingList
{
  std::unordered_map<std::string, CSF_FILEMAPPING_READTYPE> maps;
};

static cgltf_result csf_cgltf_read(const struct cgltf_memory_options* memory_options,
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
    *data = const_cast<void*>(it->second.data());
    *size = it->second.size();
    return cgltf_result_success;
  }

  CSF_FILEMAPPING_READTYPE map;
  if(map.open(path))
  {
    *data = const_cast<void*>(map.data());
    *size = map.size();
    mappings->maps.insert({pathStr, std::move(map)});
    return cgltf_result_success;
  }

  return cgltf_result_io_error;
}

static void csf_cgltf_release(const struct cgltf_memory_options* memory_options, const struct cgltf_file_options* file_options, void* data)
{
  // let MappingList destructor handle it
}

#endif

CSFAPI int CSFile_loadGTLF(CSFile** outcsf, const char* filename, CSFileMemoryPTR mem)
{
  if(!filename)
  {
    return CADSCENEFILE_ERROR_NOFILE;
  }

  int findUniqueGeometries = mem->m_config.gltfFindUniqueGeometries;

  cgltf_options gltfOptions = {};
  cgltf_data*   gltfModel;

#if CSF_SUPPORT_FILEMAPPING
  MappingList mappings;

  gltfOptions.file.read      = csf_cgltf_read;
  gltfOptions.file.release   = csf_cgltf_release;
  gltfOptions.file.user_data = &mappings;
#endif

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

#define checkDegenerate(index, count)                                                                                  \
  (index[count - 1] == index[count - 2] || index[count - 2] == index[count - 3] || index[count - 3] == index[count - 1])

        uint32_t indexBegin = indexTotCount;
        switch(accessor->component_type)
        {
          case cgltf_component_type_r_16:
            for(cgltf_size i = 0; i < accessor->count; i++)
            {
              const uint8_t* in                   = data + (i * accessor->stride);
              csfgeom.indexSolid[indexTotCount++] = *((const int16_t*)in) + vertexTotCount;
              if(i % 3 == 2 && checkDegenerate(csfgeom.indexSolid, indexTotCount))
              {
                indexTotCount -= 3;
              }
            }
            break;
          case cgltf_component_type_r_16u:
            for(cgltf_size i = 0; i < accessor->count; i++)
            {
              const uint8_t* in                   = data + (i * accessor->stride);
              csfgeom.indexSolid[indexTotCount++] = *((const uint16_t*)in) + vertexTotCount;
              if(i % 3 == 2 && checkDegenerate(csfgeom.indexSolid, indexTotCount))
              {
                indexTotCount -= 3;
              }
            }
            break;
          case cgltf_component_type_r_32u:
            for(cgltf_size i = 0; i < accessor->count; i++)
            {
              const uint8_t* in                   = data + (i * accessor->stride);
              csfgeom.indexSolid[indexTotCount++] = *((const uint32_t*)in) + vertexTotCount;
              if(i % 3 == 2 && checkDegenerate(csfgeom.indexSolid, indexTotCount))
              {
                indexTotCount -= 3;
              }
            }
            break;
          case cgltf_component_type_r_8:
            for(cgltf_size i = 0; i < accessor->count; i++)
            {
              const uint8_t* in                   = data + (i * accessor->stride);
              csfgeom.indexSolid[indexTotCount++] = *((const int8_t*)in) + vertexTotCount;
              if(i % 3 == 2 && checkDegenerate(csfgeom.indexSolid, indexTotCount))
              {
                indexTotCount -= 3;
              }
            }
            break;
          case cgltf_component_type_r_8u:
            for(cgltf_size i = 0; i < accessor->count; i++)
            {
              const uint8_t* in                   = data + (i * accessor->stride);
              csfgeom.indexSolid[indexTotCount++] = *((const uint8_t*)in) + vertexTotCount;
              if(i % 3 == 2 && checkDegenerate(csfgeom.indexSolid, indexTotCount))
              {
                indexTotCount -= 3;
              }
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
#endif
