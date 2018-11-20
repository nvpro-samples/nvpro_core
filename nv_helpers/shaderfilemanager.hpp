/* Copyright (c) 2014-2018, NVIDIA CORPORATION. All rights reserved.
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

#ifndef NV_SHADERFILEMANAGER_INCLUDED
#define NV_SHADERFILEMANAGER_INCLUDED


#include <stdio.h>
#include <vector>
#include <string>

#include <nv_helpers/assetsloader.hpp>

namespace nv_helpers
{

  class ShaderFileManager {
  public:
    enum FileType {
      FILETYPE_DEFAULT,
      FILETYPE_GLSL,
      FILETYPE_HLSL,
      FILETYPE_SPIRV,
    };

    struct IncludeEntry {
      std::string   name;
      std::string   filename;
      std::string   content;
    };

    typedef std::vector<IncludeEntry> IncludeRegistry;

    static std::string format(const char* msg, ...);

  public:
    static const unsigned int PREPROCESS_ONLY_PROGRAM = ~0;
    static const size_t       INVALID_ID = ~0;

    class IncludeID {
    public:
      size_t  m_value;

      IncludeID() : m_value(INVALID_ID) {}

      IncludeID( size_t b) : m_value(b) {}
      operator size_t() const { return m_value; }
      IncludeID& operator=( size_t b) { m_value = b; return *this; }

      bool isValid() const 
      {
        return m_value != INVALID_ID;
      }
    };

    struct Definition {

      Definition() : type(0) , prepend(""), filename(""), filetype(FILETYPE_DEFAULT), entry("main") {}
      Definition(uint32_t type, std::string const & prepend, std::string const & filename) : type(type), prepend(prepend), filename(filename), filetype(FILETYPE_DEFAULT), entry("main") {}
      Definition(uint32_t type, std::string const & filename) : type(type), prepend(""), filename(filename), filetype(FILETYPE_DEFAULT), entry("main") {}

      uint32_t    type;
      FileType    filetype;
      std::string prepend;
      std::string filename;
      std::string filenameFound;
      std::string entry;
      std::string content;
    };

    struct Program {
      Program() : program(0) {}

      unsigned int              program;
      std::vector<Definition>   definitions;
    };

    IncludeID registerInclude(std::string const & name, std::string const & filename, std::string const & content= std::string() );
    IncludeID findInclude(std::string const & name) const;

    bool  loadIncludeContent(IncludeID);

    IncludeEntry& getInclude(IncludeID idx);
    const IncludeEntry& getInclude(IncludeID idx) const;

    std::string m_prepend;
    FileType    m_filetype;
    bool        m_lineMarkers;
    bool        m_forceLineFilenames;
    bool        m_forceIncludeContent;
    bool        m_supportsExtendedInclude;
    
    // Note: we might want to avoid and directly rely on AssetLoaderAddSearchPath in the samples... ?
    void addDirectory(const std::string& dir){
        AssetLoaderAddSearchPath(dir.c_str());
        //m_directories.push_back(dir);
    }

    ShaderFileManager() 
      : m_forceLineFilenames(false)
      , m_lineMarkers(true)
      , m_forceIncludeContent(false)
      , m_supportsExtendedInclude(false)
      , m_filetype(FILETYPE_GLSL)
    {
      AssetLoaderAddSearchPath(".");
      //m_directories.push_back(".");
    }

  protected:
    std::string markerString(int line, std::string const & filename, int fileid);
    std::string getIncludeContent(IncludeID idx, std::string& filenameFound);
    std::string getContent(std::string const & filename, std::string & filenameFound);
    std::string manualInclude(std::string const & filename, std::string & filenameFound, std::string const & prepend, bool foundVersion);

    //std::vector<std::string>  m_directories;
    IncludeRegistry           m_includes;
  };

}


#endif//NV_PROGRAM_INCLUDED
