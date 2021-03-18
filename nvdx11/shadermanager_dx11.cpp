/* Copyright (c) 2017-2018, NVIDIA CORPORATION. All rights reserved.
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


#include "shadermanager_dx11.hpp"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iostream>

#pragma message("TODO TODO TODO TODO TODO TODO TODO : assetsloader DELETED... Replace this by another helper if required. ")
 //#include "nvh/assetsloader.hpp"
#include "main.h"

#include <nvh/misc.hpp>


namespace nvdx11
{

  HRESULT ShaderManager::Open( THIS_ D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes )
  {
    std::string filename;
    IncludeID id = findInclude(pFileName);
    if (id.isValid() && loadIncludeContent(id)){
      std::string& content = m_includes[id].content;

      *ppData = content.data();
      *pBytes = UINT(content.size());
      return S_OK;
    }
    else{
      return E_FAIL;
    }
  }


  HRESULT ShaderManager::Close( THIS_ LPCVOID pData )
  {
    return S_OK;
  }

  bool  ShaderManager::setupShader( Shader& prog )
  {
    Definition& definition = prog.definition;

    if (definition.type == SHADER_UNDEFINED) return false;

    std::string combinedPrepend = m_prepend + definition.prepend;
    std::string combinedFilenames = definition.filename;

    std::string filename;
    std::string source = getContent(definition.filename, filename);
    
    bool found = !source.empty();

    ID3DBlob* binary = NULL;
    bool loadedCache = false;
    if (!m_useCacheFile.empty() && (!found || m_preferCache)){
      // try cache
      binary = loadBinary(prog, combinedPrepend,combinedFilenames);
      loadedCache = binary != NULL;
    }

    if (found && !loadedCache){
      const char* target = NULL;
      switch (definition.type){
      case SHADER_VERTEX:
      target = "vs_5_0";
      break;
      case SHADER_PIXEL:
      target = "ps_5_0";
      break;
      case SHADER_COMPUTE:
      target = "cs_5_0";
      break;
      case SHADER_GEOMETRY:
      target = "gs_5_0";
      break;
      case SHADER_HULL:
      target = "hs_5_0";
      break;
      case SHADER_DOMAIN:
      target = "ds_5_0";
      break;
      }


      std::vector<D3D_SHADER_MACRO> macros;

      // chop define string into sequence of macros
      std::string macrostring = combinedPrepend;
      const char* current = macrostring.data();
      D3D_SHADER_MACRO  macro;
      bool name = true;
      size_t length = macrostring.size();
      for (size_t i = 0; i < length; i++){
        bool last = i == length-1;
        if (macrostring[i] == ';' || last){
          if (!last){
            macrostring[i] = 0;
          }
          if (name){
            macro.Name = current;
          }
          else{
            macro.Definition = current;
            macros.push_back(macro);
          }
          name = !name;
          current = &macrostring[i+1];
        }
      }
      D3D_SHADER_MACRO term = {0,0};
      macros.push_back(term);

      ID3DBlob* errors = NULL;

      D3DCompile(source.data(), source.size(), definition.filename.c_str(), macros.data(),
        this, definition.entry.c_str(), target, 0, 0, &binary, &errors );

      if (errors){
        fprintf(stdout,"error on shader compilation of file: %s\n", filename.c_str());
        fwrite(errors->GetBufferPointer(), errors->GetBufferSize(), 1, stdout);
        errors->Release();
        return false;
      }
    }
    else{
      fprintf(stdout,"neither file, nor cache found\n");
      return false;
    }

    HRESULT hr;
    switch (definition.type){
    case SHADER_VERTEX:
    hr = m_device->CreateVertexShader( binary->GetBufferPointer(), binary->GetBufferSize(), NULL, &prog.handle.vertex );
    break;
    case SHADER_PIXEL:
    hr = m_device->CreatePixelShader( binary->GetBufferPointer(), binary->GetBufferSize(), NULL, &prog.handle.pixel );
    break;
    case SHADER_COMPUTE:
    hr = m_device->CreateComputeShader( binary->GetBufferPointer(), binary->GetBufferSize(), NULL, &prog.handle.compute );
    break;
    case SHADER_GEOMETRY:
    hr = m_device->CreateGeometryShader( binary->GetBufferPointer(), binary->GetBufferSize(), NULL, &prog.handle.geometry );
    break;
    case SHADER_HULL:
    hr = m_device->CreateHullShader( binary->GetBufferPointer(), binary->GetBufferSize(), NULL, &prog.handle.hull );
    break;
    case SHADER_DOMAIN:
    hr = m_device->CreateDomainShader( binary->GetBufferPointer(), binary->GetBufferSize(), NULL, &prog.handle.domain );
    break;
    }

    if (FAILED( hr )){
      binary->Release();
      fprintf( stdout, "error on shader creation\n" );
      return false;
    }

    if (!m_useCacheFile.empty() && !loadedCache){
      saveBinary(prog, binary, combinedPrepend, combinedFilenames);
    }

    prog.type = (ShaderType)definition.type;

    if (prog.type == SHADER_VERTEX){
      prog.binary = binary;
    }
    else{
      binary->Release();
    }

    return true;
  }


  ShaderManager::ShaderID ShaderManager::createShader( const ShaderManager::Definition& definition )
  {
    Shader prog;
    prog.definition = definition;
    setupShader(prog);

    for (size_t i = 0; i < m_shaders.size(); i++){
      if (m_shaders[i].definition.type == 0){
        m_shaders[i] = prog;
        return i;
      }
    }
    
    m_shaders.push_back(prog);
    return m_shaders.size()-1;
  }

  bool ShaderManager::areShadersValid()
  {
    bool valid = true;
    for (size_t i = 0; i < m_shaders.size(); i++){
      valid = valid && isValid( (ShaderID)i );
    }
    return valid;
  }

  void ShaderManager::deleteShader( ShaderID idx )
  {
    if (!isValid( idx )) return;

    Shader& shader = getShader( idx );

    if (shader.handle.any){
      switch(shader.type){
      case SHADER_VERTEX:
      shader.handle.vertex->Release();
      break;
      case SHADER_PIXEL:
      shader.handle.pixel->Release();
      break;
      case SHADER_COMPUTE:
      shader.handle.compute->Release();
      break;
      case SHADER_GEOMETRY:
      shader.handle.geometry->Release();
      break;
      case SHADER_DOMAIN:
      shader.handle.domain->Release();
      break;
      case SHADER_HULL:
      shader.handle.hull->Release();
      break;
      }
    }

    if (shader.binary){
      shader.binary->Release();
    }

    shader.handle.any = NULL;
    shader.binary = NULL;
    shader.definition = Definition();
  }

  void ShaderManager::deleteShaders()
  {
    for (size_t i = 0; i < m_shaders.size(); i++){
      deleteShader((ShaderID)i);
    }
  }
  
  void ShaderManager::reloadShader( ShaderID i )
  {
    if (!isValid(i)) return;

    if (m_shaders[i].definition.type){
      Definition preserve = m_shaders[i].definition;
      deleteShader(i);
      m_shaders[i].definition = preserve;
      setupShader(m_shaders[i]);
    }
  }

  void ShaderManager::reloadShaders()
  {
    nvprintf("Reloading programs...\n");
    
    for (size_t i = 0; i < m_shaders.size(); i++){
      reloadShader((ShaderID)i);
    }
    
    nvprintf("done\n");
  }

  bool ShaderManager::isValid( ShaderID idx ) const
  {
    return  idx.isValid() && (m_shaders[idx].definition.type == 0 || m_shaders[idx].handle.any != 0);
  }

  ShaderManager::Shader& ShaderManager::getShader( ShaderID idx )
  {
    return m_shaders[idx];
  }

  const ShaderManager::Shader& ShaderManager::getShader( ShaderID idx ) const
  {
    return m_shaders[idx];
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

#define mmix(h,k) { k *= m; k ^= k >> r; k *= m; h *= m; h ^= k; }

  static unsigned int strMurmurHash2A ( const void * key, size_t len, unsigned int seed )
  {
    const unsigned int m = 0x5bd1e995;
    const int r = 24;
    unsigned int l = (unsigned int)len;

    const unsigned char * data = (const unsigned char *)key;

    unsigned int h = seed;
    unsigned int t = 0;

    while(len >= 4)
    {
      unsigned int k = *(unsigned int*)data;

      mmix(h,k);

      data += 4;
      len -= 4;
    }



    switch(len)
    {
    case 3: t ^= data[2] << 16;
    case 2: t ^= data[1] << 8;
    case 1: t ^= data[0];
    };

    mmix(h,t);
    mmix(h,l);

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
  }
#undef mmix


  static size_t strHexFromByte(char *buffer, size_t bufferlen, const void *data, size_t len)
  {
    const char tostr[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    const unsigned char *d = (const unsigned char *)data;
    char *out = buffer;
    size_t i = 0;
    for (; i < len && (i*2)+1 < bufferlen; i++,d++,out+=2){
      unsigned int val = *d;
      unsigned int hi = val/16;
      unsigned int lo = val%16;
      out[0] = tostr[hi];
      out[1] = tostr[lo];
    }

    return i*2;
  }

  std::string  ShaderManager::binaryName(const std::string& combinedPrepend, const std::string& combinedFilenames)
  {
    unsigned int hashCombine   = combinedPrepend.empty() ? 0 : strMurmurHash2A(&combinedPrepend[0],combinedPrepend.size(),127);
    unsigned int hashFilenames = strMurmurHash2A(&combinedFilenames[0],combinedFilenames.size(),129);

    std::string hexCombine;
    std::string hexFilenames;
    hexCombine.resize(8);
    hexFilenames.resize(8);
    strHexFromByte(&hexCombine[0], 8, &hashCombine, 4);
    strHexFromByte(&hexFilenames[0], 8, &hashFilenames, 4);

    return m_useCacheFile + "_" + hexCombine + "_" + hexFilenames + ".glp";
  }


  ID3DBlob* ShaderManager::loadBinary( Shader& prog, const std::string& combinedPrepend, const std::string& combinedFilenames )
  {
    std::string filename = binaryName( combinedPrepend, combinedFilenames );

    std::string binraw = nvh::loadFile( filename.c_str() );
    if (!binraw.empty()){
      ID3DBlob* binary;
      HRESULT hr;
      hr = D3DCreateBlob(binraw.size(), &binary);
      memcpy(binary->GetBufferPointer(), binraw.data(), binraw.size());
      return binary;
    }
    return NULL;
  }

  void ShaderManager::saveBinary( Shader& prog, ID3DBlob* binary, const std::string& combinedPrepend, const std::string& combinedFilenames )
  {
    std::string filename = binaryName( combinedPrepend, combinedFilenames );

    std::ofstream binfile;
    binfile.open( filename.c_str(), std::ios::binary | std::ios::out );
    if (binfile.is_open()){
      binfile.write( (const char*)binary->GetBufferPointer(), binary->GetBufferSize() );
    }
  }

}//namespace nvglf


