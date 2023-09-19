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
#include <atomic>
#include <algorithm>

// use CSF_IMPLEMENTATION define prior including
// to add actual implementation to this project
//
// prior implementation include, supports following options
//
// CSF_SUPPORT_ZLIB         0/1 (uses zlib)
// CSF_SUPPORT_GLTF2        0/1 (uses cgltf)
//
// CSF_PARALLEL_WORK         0/1 
//    meant for other cadscenefile processing files
//    that want minimal dependencies (as in just this header).
//    It is meant to remove all of the `pragma omp parallel for` code.
//
// CSF_DISABLE_FILEMAPPING_SUPPORT 0/1
//    If 1 the implementation will not use the OS-level functionality
//    to support memory mapped files, but rather use traditional file
//    api to load or save using a full in memory copy.


#if defined(__cplusplus)
#include <utility>
#if CSF_PARALLEL_WORK || defined(CSF_IMPLEMENTATION)
#include <mutex>
#include <thread>
#include <functional>
#endif
#endif

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

enum CadSceneFileVersion : int
{
  // original version
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

  // binary compatibility
  CADSCENEFILE_VERSION_COMPAT = 2,
  // current stable version
  CADSCENEFILE_VERSION = CADSCENEFILE_VERSION_GEOMETRYCHANNELS,
  // special compatibility
  CADSCENEFILE_VERSION_SUPPORTED = CADSCENEFILE_VERSION_GEOMETRYCHANNELS,
};

enum CadSceneFileStatus : int
{
  // return codes
  CADSCENEFILE_NOERROR         = 0,  // Success.
  CADSCENEFILE_ERROR_NOFILE    = 1,  // The file did not exist or an I/O operation failed.
  CADSCENEFILE_ERROR_VERSION   = 2,  // The file had an invalid header.
  CADSCENEFILE_ERROR_OPERATION = 3,  // Called an operation that cannot be called on this object.
  CADSCENEFILE_ERROR_INVALID   = 4,  // The file contains invalid data.
};

enum CadSceneFileFlag : uint32_t
{
  // the loader was fixed so various flags, CADSCENEFILE_FLAG_META_FILE etc.
  // were deprecated and will be recycled in future versions

  // each node is visited only once during traversal.
  // always set, likely only commandline tools support multiple
  // referencing of nodes
  CADSCENEFILE_FLAG_UNIQUENODES = 1 << 0,
  // all triangles/lines are using strips instead of index lists
  // never set, only special purpose file/application made use of this
  CADSCENEFILE_FLAG_STRIPS = 1 << 1,
  // parts get their own vertex buffer sub-range
  // requires CSFGEOMETRY_PARTCHANNEL_VERTEXRANGE to be available
  CADSCENEFILE_FLAG_PERPARTVERTICES = 1 << 5,
  // degenerate primitives were removed
  CADSCENEFILE_FLAG_NODEGENERATES = 1 << 7,
};

enum CadSceneFileLengths : int
{
  CADSCENEFILE_LENGTH_STRING = 128,
  CADSCENEFILE_LENGTH_AUXS   = 32,
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
  When performance matters, the parts should be flattened as much as possible.

  CSFNodes  (hierarchy of nodes)

  A node can also reference a geometry, this way the same geometry
  data can be instanced multiple times.
  If the node references geometry, then it must have an
  array of "CSFNodePart" matching referenced CSFGeometryParts.
  The CSFNodePart encodes the materials/matrix as well as its
  active state.


  CSFoffset - nameOFFSET variables
  --------------------------------

  CSFoffset is only indirectly used during save and load operations.
  As end user you can ignore the various "nameOFFSET" variables
  in all the unions, as well as the pointers array.


  CSFBytePacket - meta information
  --------------------------------

  Additional meta information can be stored in a linear chunk of memory that is 
  organized in CSFBytePackets.

  meta/material.bytes = { [CSFBytePacket A] [Payload A..] [CSFBytePacket B] [Payload B...]...}

  Tools operating on CSF files are not expected to interpret the meta information.
  They may, however, remove/clone the entire entry on duplication/removal of 
  the appropriate material, geometry or node. 

  */

typedef struct _CSFLoaderConfig
{
  // read only access to loaded csf data
  // means we can use filemappings to avoid allocations
  // for content within the main structs (nodes, geometries...)
  // The primary structs are still allocated for write access,
  // but all pointers within them are mapped.
  // default = 0
  int secondariesReadOnly;

  // detailed pointer alignment validation (some older files need resaving to pass)
  // default = 1
  int validate;

  // only relevant for CSF_GLTF2_SUPPORT
  // uses hashes of geometry data to figure out what gltf mesh
  // data is re-used under different materials, and can
  // therefore be mapped to a single CSF geometry.
  // default = 1
  int gltfFindUniqueGeometries;
} CSFLoaderConfig;

typedef uint64_t CSFoffset;
typedef struct _CSFGuid
{
  uint32_t value0;
  uint32_t value1;
  uint32_t value2;
  uint32_t value3;
} CSFGuid;


// optional, if one wants to pack
// additional meta information into the bytes arrays
typedef struct _CSFBytePacket
{
  CSFGuid  guid;
  uint32_t numBytes;  // size of the payload + size of this header
} CSFBytePacket;

// list of standard GUIDS
// create your own via random 128-bit value
// where the first 96-bit are not 0


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

      CSFMaterialGLTF2Texture baseColorTexture;
      CSFMaterialGLTF2Texture metallicRoughnessTexture;
    };

    struct
    {
      float diffuseFactor[4];
      float specularFactor[3];
      float glossinessFactor;

      CSFMaterialGLTF2Texture diffuseTexture;
      CSFMaterialGLTF2Texture specularGlossinessTexture;
    };
  };

  CSFMaterialGLTF2Texture occlusionTexture;
  CSFMaterialGLTF2Texture normalTexture;
  CSFMaterialGLTF2Texture emissiveTexture;
} CSFMaterialGLTF2Meta;

typedef struct _CSFMeta
{
  char name[CADSCENEFILE_LENGTH_STRING];
  int  flags;

  // access via CSFMeta_getBytePacket
  CSFoffset numBytes;
  union
  {
    CSFoffset      bytesOFFSET;
    unsigned char* bytes;
  };
} CSFMeta;

//////////////////////////////////////////////////////////////////////////

typedef struct _CSFMaterial
{
  char  name[CADSCENEFILE_LENGTH_STRING];
  float color[4];
  int   type;  // arbitrary data


  // access via CSFMaterial_getBytePacket
  //         or CSFile_getMaterialBytePacket
  uint32_t numBytes;
  union
  {
    CSFoffset      bytesOFFSET;
    unsigned char* bytes;
  };
} CSFMaterial;

//////////////////////////////////////////////////////////////////////////

typedef enum _CSFGeometryPartChannel
{
  // CSFGeometryPartChannelBbox
  CSFGEOMETRY_PARTCHANNEL_BBOX,
  // CSFGeometryPartChannelVertices
  CSFGEOMETRY_PARTCHANNEL_VERTEXRANGE,
  CSFGEOMETRY_PARTCHANNELS,
} CSFGeometryPartChannel;

typedef struct _CSFGeometryPartBbox
{
  float min[3];
  float max[3];
} CSFGeometryPartBbox;

typedef struct _CSFGeometryPartVertexRange
{
  uint32_t vertexBegin;
  uint32_t numVertices;
} CSFGeometryPartVertexRange;

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
  CSFGEOMETRY_AUXCHANNEL_TANGENT,
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

  // CADSCENEFILE_VERSION_GEOMETRYCHANNELS
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

  // CADSCENEFILE_VERSION_BASE
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
    CSFoffset indexSolidOFFSET;
    uint32_t* indexSolid;
  };

  union
  {
    CSFoffset indexWireOFFSET;
    uint32_t* indexWire;
  };

  union
  {
    CSFoffset        partsOFFSET;
    CSFGeometryPart* parts;
  };
} CSFGeometry;

//////////////////////////////////////////////////////////////////////////

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

    CSFNodes can also be a simple list of instances if csf->rootIDX <= 0
    in that case nodes must not have children.

    Each Node stores:
    - the object transform (relative to parent)
    - the world transform (final transform to get from object to world space
      node.worldTM = node.parent.worldTM * node.objectTM;
    - optional geometry reference
    - optional array of node children
    - the parts array is mandatory if a geometry is referenced
      and must be sized to form a 1:1 correspondence to the geometry's parts.
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


// All loaders will allocate this header separate from the 
// file data, therefore the header can be safely extended with
// new versions.
// New members will be nulled, if the file version
// was older than what this struct defines.

typedef struct _CSFile
{
  int magic;
  int version;

  // see CADSCENEFILE_FLAG_??
  uint32_t fileFlags;

  // used internally for load & save operations, can be ignored
  int numPointers;

  int numGeometries;
  int numMaterials;
  int numNodes;

  // index into csf->nodes where the root node is located
  // if negative it means no hierarchy exists and all
  // nodes have no children (flat instance list)
  int rootIDX;

  union
  {
    // the pointers are used internally for load & save operations
    // no need to specify prior save
    // no need to access post load

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
  // Only available for version >= CADSCENEFILE_VERSION_META

  union
  {
    CSFoffset nodeMetasOFFSET;
    CSFMeta*  nodeMetas;
  };
  union
  {
    CSFoffset geometryMetasOFFSET;
    CSFMeta*  geometryMetas;
  };
  union
  {
    CSFoffset fileMetaOFFSET;
    CSFMeta*  fileMeta;
  };
  //----------------------------------
} CSFile;


//////////////////////////////////////////////////////////////////////////

typedef struct CSFileMemory_s* CSFileMemoryPTR;

// Internal allocation wrapper
// also handles details for loading operations
CSFAPI CSFileMemoryPTR CSFileMemory_new();
CSFAPI CSFileMemoryPTR CSFileMemory_newCfg(const CSFLoaderConfig* config);

// special value for CSFileMemory_alloc
#define CSF_MEMORY_ZEROED_FILL ((const void*)1)

// alloc functions are thread-safe
// fill if provided must provide sz bytes, or must be CSF_MEMORY_ZEROED_FILL
// which clears the allocation to zero
CSFAPI void* CSFileMemory_alloc(CSFileMemoryPTR mem, size_t sz, const void* fill);
// fillPartial if provided must provide szPartial bytes and szPartial <= sz
CSFAPI void* CSFileMemory_allocPartial(CSFileMemoryPTR mem, size_t sz, size_t szPartial, const void* fillPartial);
// all allocations within will be freed
CSFAPI void CSFileMemory_delete(CSFileMemoryPTR mem);

CSFAPI void CSFileMemory_getCfg(CSFileMemoryPTR mem, CSFLoaderConfig* config);
CSFAPI int  CSFileMemory_areSecondariesReadOnly(CSFileMemoryPTR mem);

//////////////////////////////////////////////////////////////////////////

#if !CSF_DISABLE_FILEMAPPING_SUPPORT
typedef struct CSFReadMapping_s* CSFReadMappingPTR;
CSFAPI CSFReadMappingPTR         CSFReadMapping_new(const char* filename);
CSFAPI void                      CSFReadMapping_delete(CSFReadMappingPTR readMapping);
CSFAPI size_t                    CSFReadMapping_getSize(CSFReadMappingPTR readMapping);
CSFAPI const void*               CSFReadMapping_getData(CSFReadMappingPTR readMapping);

typedef struct CSFWriteMapping_s* CSFWriteMappingPTR;
CSFAPI CSFWriteMappingPTR         CSFWriteMapping_new(const char* filename, size_t fileSize);
CSFAPI void                       CSFWriteMapping_delete(CSFWriteMappingPTR writeMapping);
CSFAPI size_t                     CSFWriteMapping_getSize(CSFWriteMappingPTR writeMapping);
CSFAPI void*                      CSFWriteMapping_getData(CSFWriteMappingPTR writeMapping);
#endif

//////////////////////////////////////////////////////////////////////////

// The `data` is modified, therefore the raw load operation can be executed only once.
// It must be preserved for as long as the csf and its internals are accessed.
// `outcsf` must be separate and not overlap with `data`.
// If `validate` is non zero, additional checks on pointer and content alignment,
// ranges etc. is performed.
CSFAPI int CSFile_loadRaw(CSFile* outcsf, size_t sz, void* data, int validate);

// All allocations are done within the provided file memory.
// It must be preserved for as long as the csf and its internals are accessed
CSFAPI int CSFile_load(CSFile** outcsf, const char* filename, CSFileMemoryPTR mem);

CSFAPI int CSFile_save(const CSFile* csf, const char* filename);


// support for other extensions depends on
// CSF_ZIP_SUPPORT, CSF_GLTF2_SUPPORT

CSFAPI int CSFile_loadExt(CSFile** outcsf, const char* filename, CSFileMemoryPTR mem);

CSFAPI int CSFile_saveExt(CSFile* csf, const char* filename);


//////////////////////////////////////////////////////////////////////////

// sets all content of _deprecated to zero, automatically done at load
// recommended to be done prior save
CSFAPI void CSFile_clearDeprecated(CSFile* csf);
CSFAPI void CSFGeometry_clearDeprecated(CSFGeometry* geo);

// sets up single normal/tex channel based on array existence, automatically done at load
CSFAPI void CSFile_setupDefaultChannels(CSFile* csf);
CSFAPI void CSFGeometry_setupDefaultChannels(CSFGeometry* geo);

// removes part channels
CSFAPI void CSFGeometry_removeAllPartChannels(CSFGeometry* geo, CSFileMemoryPTR mem);
// if channel doesn't exist, allocates it
// memory wasteful if you do one by one channel at a time
CSFAPI void CSFGeometry_requirePartChannel(CSFGeometry* geo, CSFileMemoryPTR mem, CSFGeometryPartChannel channel);
CSFAPI void CSFGeometry_requirePartChannels(CSFGeometry* geo, CSFileMemoryPTR mem, uint32_t numChannels, const CSFGeometryPartChannel* channels);
CSFAPI void CSFGeometry_removePartChannels(CSFGeometry* geo, CSFileMemoryPTR mem, uint32_t numChannels, const CSFGeometryPartChannel* channels);
// creates new allocation and copies old
CSFAPI void CSFGeometry_requireAuxChannel(CSFGeometry* geo, CSFileMemoryPTR mem, CSFGeometryAuxChannel channel);

// add byte packet to meta if its guid doesn't already exist
CSFAPI void CSFMeta_setOrAddBytePacket(CSFMeta** metaPtr, CSFileMemoryPTR csfmem, size_t size, const void* data);

//////////////////////////////////////////////////////////////////////////

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

// meta can be nullptr
CSFAPI const CSFBytePacket* CSFMeta_getBytePacket(const CSFMeta* meta, const CSFGuid guid);

CSFAPI const CSFBytePacket* CSFMaterial_getBytePacket(const CSFMaterial* material, const CSFGuid guid);
CSFAPI const CSFBytePacket* CSFile_getMaterialBytePacket(const CSFile* csf, int materialIDX, const CSFGuid guid);

CSFAPI const CSFBytePacket* CSFile_getFileBytePacket(const CSFile* csf, const CSFGuid guid);

//////////////////////////////////////////////////////////////////////////

CSFAPI int  CSFile_transform(CSFile* csf);  // requires unique nodes
CSFAPI void CSFMatrix_identity(float*);

//////////////////////////////////////////////////////////////////////////

// The CSFileHandle allows to open a file without fully loading all its
// content. Only works on uncompressed files.
// The access to same file is currently NOT thread-safe.
//
// Not the most tested / commonly used api

typedef struct CSFileHandle_s* CSFileHandlePTR;

CSFAPI int  CSFileHandle_open(CSFileHandlePTR* pHandle, const char* filename);
CSFAPI void CSFileHandle_close(CSFileHandlePTR handle);

typedef enum CSFileHandleContentBit_e
{
  CSFILEHANDLE_CONTENT_MATERIAL        = 1,
  CSFILEHANDLE_CONTENT_GEOMETRY        = 2,
  CSFILEHANDLE_CONTENT_NODE            = 4,
  CSFILEHANDLE_CONTENT_GEOMETRYMETA    = 8,
  CSFILEHANDLE_CONTENT_NODEMETA        = 16,
  CSFILEHANDLE_CONTENT_FILEMETA        = 32,
} CSFileHandleContentBit;

// zeros pointer arrays to avoid accidental access
// fills and returns header passed in
CSFAPI CSFile* CSFileHandle_loadHeader(CSFileHandlePTR handle, CSFile* header);
// keeps file offsets
// fills and returns header passed in
CSFAPI CSFile* CSFileHandle_rawHeader(CSFileHandlePTR handle, CSFile* header);

// Loads the csfile with its primary arrays, but without the data pointed to by an array element.
// Which arrays should be loaded is provided by the bitflag.
// The CSFile struct and the arrays are allocated within mem.
CSFAPI CSFile* CSFileHandle_loadBasics(CSFileHandlePTR handle, uint32_t contentbitsflag, CSFileMemoryPTR mem);

// Loads the array elements along with their content, returns an array of the appropriate struct type for num elements.
// The array and content is allocated within mem
CSFAPI void* CSFileHandle_loadElements(CSFileHandlePTR handle, CSFileHandleContentBit contentbit, int begin, int num, CSFileMemoryPTR mem);

// similar as above but the primary struct array is provided by the pointer, rest of data is allocated into mem
CSFAPI void* CSFileHandle_loadElementsInto(CSFileHandlePTR        handle,
                                           CSFileHandleContentBit contentbit,
                                           int                    begin,
                                           int                    num,
                                           CSFileMemoryPTR        mem,
                                           size_t                 pimarySize,
                                           void*                  primaryArray);
};

#define CSF_logPrintf(outlog, ...)                                                                                     \
  if((outlog))                                                                                                         \
  {                                                                                                                    \
    fprintf((outlog), __VA_ARGS__);                                                                                    \
  }

inline size_t CSF_alignIndexAllocation(size_t size)
{
  return (size + 3) & ~3;
}

#ifdef __cplusplus

template <typename T>
inline T* CSFGeometry_getPartChannelT(CSFGeometry* geo, CSFGeometryPartChannel channel)
{
  return (T*)CSFGeometry_getPartChannel(geo, channel);
}

template <typename T>
inline const T* CSFGeometry_getPartChannelT(const CSFGeometry* geo, CSFGeometryPartChannel channel)
{
  return (const T*)CSFGeometry_getPartChannel(geo, channel);
}

template <typename T>
inline T* CSFileMemory_allocCopyT(CSFileMemoryPTR mem, const T* src, size_t num)
{
  if(num == 0)
    return nullptr;
  return (T*)CSFileMemory_alloc(mem, sizeof(T) * num, src);
}

template <typename T>
inline T* CSFileMemory_allocPartialT(CSFileMemoryPTR mem, size_t num, size_t numSrc, const T* src)
{
  if(num == 0)
    return nullptr;
  return (T*)CSFileMemory_allocPartial(mem, sizeof(T) * num, sizeof(T) * numSrc, src);
}

#if !CSF_DISABLE_FILEMAPPING_SUPPORT
struct CSFReadMapping
{
  CSFReadMappingPTR mapping;


  CSFReadMapping() {}
  CSFReadMapping(const char* filename) { mapping = CSFReadMapping_new(filename); }

  CSFReadMapping(CSFReadMapping&& other) noexcept { this->operator=(std::move(other)); };
  CSFReadMapping& operator=(CSFReadMapping&& other)
  {
    mapping       = other.mapping;
    other.mapping = nullptr;
    return *this;
  }
  CSFReadMapping(const CSFReadMapping&) = delete;
  CSFReadMapping& operator=(const CSFReadMapping& other) = delete;

  size_t getSize() const { return mapping ? CSFReadMapping_getSize(mapping) : 0; }

  const void* getData() const { return mapping ? CSFReadMapping_getData(mapping) : nullptr; }

  ~CSFReadMapping()
  {
    if(mapping)
    {
      CSFReadMapping_delete(mapping);
    }
  }
};

struct CSFWriteMapping
{
  CSFWriteMappingPTR mapping;

  CSFWriteMapping() {}
  CSFWriteMapping(const char* filename, size_t fileSize) { mapping = CSFWriteMapping_new(filename, fileSize); }

  CSFWriteMapping(CSFWriteMapping&& other) noexcept { this->operator=(std::move(other)); };
  CSFWriteMapping& operator=(CSFWriteMapping&& other)
  {
    mapping       = other.mapping;
    other.mapping = nullptr;
    return *this;
  }
  CSFWriteMapping(const CSFWriteMapping&) = delete;
  CSFWriteMapping& operator=(const CSFWriteMapping& other) = delete;

  size_t getSize() const { return mapping ? CSFWriteMapping_getSize(mapping) : 0; }

  void* getData() const { return mapping ? CSFWriteMapping_getData(mapping) : nullptr; }

  ~CSFWriteMapping()
  {
    if(mapping)
    {
      CSFWriteMapping_delete(mapping);
    }
  }
};
#endif

#if CSF_PARALLEL_WORK || defined(CSF_IMPLEMENTATION)
namespace csfutils {

#if !defined(NDEBUG) && 0
#define CSF_DEFAULT_NUM_THREADS 1
#else
#define CSF_DEFAULT_NUM_THREADS uint32_t(std::thread::hardware_concurrency())
#endif

inline uint32_t parallel_items(uint64_t                                                              numItems,
                               std::function<void(uint64_t idx, uint32_t threadIdx, void* userData)> fn,
                               void*                                                                 userData = nullptr,
                               uint32_t                                                              batchSize = 1,
                               uint32_t numThreads = CSF_DEFAULT_NUM_THREADS)
{
  if(numThreads <= 1 || numItems < numThreads || numItems < batchSize)
  {
    for(uint64_t idx = 0; idx < numItems; idx++)
    {
      fn(idx, 0, userData);
    }
    return 1;
  }
  else
  {
    std::atomic_uint64_t counter = 0;

    auto worker = [&](uint32_t threadIdx) {
      uint64_t idxBegin;
      while((idxBegin = counter.fetch_add(batchSize)) < numItems)
      {
        uint64_t idxLast = std::min(numItems, idxBegin + batchSize);
        for(uint64_t idx = idxBegin; idx < idxLast; idx++)
        {
          fn(idx, threadIdx, userData);
        }
      }
    };

    std::thread* threads = new std::thread[numThreads];
    for(uint32_t i = 0; i < numThreads; i++)
    {
      threads[i] = std::thread(worker, i);
    }

    for(uint32_t i = 0; i < numThreads; i++)
    {
      threads[i].join();
    }
    delete[] threads;

    return numThreads;
  }
}

inline uint32_t parallel_ranges(uint64_t numItems,
                                std::function<void(uint64_t idxBegin, uint64_t idxEnd, uint32_t threadIdx, void* userData)> fn,
                                void*    userData   = nullptr,
                                uint32_t batchSize  = 1024,
                                uint32_t numThreads = CSF_DEFAULT_NUM_THREADS)
{
  if(numThreads <= 1 || numItems < numThreads || numItems < batchSize)
  {
    fn(0, numItems, 0, userData);
    return 1;
  }
  else
  {
    std::atomic_uint64_t counter = 0;

    auto worker = [&](uint32_t threadIdx) {
      uint64_t idxBegin;
      while((idxBegin = counter.fetch_add(batchSize)) < numItems)
      {
        uint64_t idxLast = std::min(numItems, idxBegin + batchSize);
        fn(idxBegin, idxLast, threadIdx, userData);
      }
    };

    std::thread* threads = new std::thread[numThreads];
    for(uint32_t i = 0; i < numThreads; i++)
    {
      threads[i] = std::thread(worker, i);
    }

    for(uint32_t i = 0; i < numThreads; i++)
    {
      threads[i].join();
    }
    delete[] threads;

    return numThreads;
  }
}
}  // namespace csfutils
#endif
#endif

#ifdef CSF_IMPLEMENTATION
#include "cadscenefile.inl"
#endif
