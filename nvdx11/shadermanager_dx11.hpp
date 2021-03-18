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

#ifndef NV_PROGRAMMANAGER_INCLUDED
#define NV_PROGRAMMANAGER_INCLUDED

#include <stdio.h>
#include <vector>
#include <string>
#include <d3d11.h>
#include <d3dcompiler.h>

#include <nvh/shaderfilemanager.hpp>
#pragma message("TODO TODO TODO TODO TODO TODO TODO : assetsloader DELETED... Replace this by another helper if required. ")
 //#include <nvh/assetsloader.hpp>


namespace nvdx11
{

  enum ShaderType {
    SHADER_UNDEFINED,
    SHADER_VERTEX,
    SHADER_HULL,
    SHADER_DOMAIN,
    SHADER_GEOMETRY,
    SHADER_PIXEL,
    SHADER_COMPUTE,
  };

  class ShaderManager : public ID3DInclude, public nvh::ShaderFileManager {
  public:
    static const unsigned int PREPROCESS_ONLY_PROGRAM = ~0;
    static const size_t       INVALID_ID = ~0;

    class ShaderID {
    public:
      size_t  m_value;

      ShaderID() : m_value(INVALID_ID) {}

      ShaderID( size_t b) : m_value(b) {}
      operator size_t() const { return m_value; }
      ShaderID& operator=( size_t b) { m_value = b; return *this; }

      bool isValid() const 
      {
        return m_value != INVALID_ID;
      }
    };

    struct Shader {
      Shader() 
      {
        type = SHADER_UNDEFINED;
        handle.compute = NULL;
        binary = NULL;
      }

      union {
        ID3D11VertexShader* vertex;
        ID3D11PixelShader* pixel;
        ID3D11ComputeShader* compute;
        ID3D11GeometryShader* geometry;
        ID3D11HullShader* hull;
        ID3D11DomainShader* domain;
        void* any;
      } handle;
      
      ShaderType                type;
      ID3DBlob*                 binary; // only preserved for vertex shaders
      Definition                definition;
    };

    ShaderID createShader(const Definition& definition);

    void deleteShader( ShaderID idx );
    void reloadShader( ShaderID idx );

    void reloadShaders();
    void deleteShaders();
    bool areShadersValid();

    bool isValid( ShaderID idx ) const;
    Shader& getShader(ShaderID idx);
    const Shader& getShader(ShaderID idx) const;

    ID3D11Device* m_device;
    std::string   m_useCacheFile;
    bool          m_preferCache;

    ShaderManager() 
      : m_preferCache(false)
      , m_device(NULL)
    {
    }

  private:
    bool setupShader(Shader& prog);
    
    ID3DBlob* loadBinary(Shader& prog, const std::string& combinedPrepend, const std::string& combinedFilenames );
    void saveBinary(Shader& prog, ID3DBlob* binary, const std::string& combinedPrepend, const std::string& combinedFilenames );
    std::string binaryName( const std::string& combinedPrepend, const std::string& combinedFilenames );

    //IncludeRegistry          m_includes;
    std::vector<Shader>      m_shaders;

    virtual HRESULT Open(THIS_ D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes);
    virtual HRESULT Close(THIS_ LPCVOID pData);
  };

}//namespace nvglf


#endif//NV_PROGRAM_INCLUDED
