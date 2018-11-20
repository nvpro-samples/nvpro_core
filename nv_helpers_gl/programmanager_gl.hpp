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

#ifndef NV_PROGRAMMANAGER_INCLUDED
#define NV_PROGRAMMANAGER_INCLUDED

#include "extensions_gl.hpp"
#include <stdio.h>
#include <vector>
#include <string>

#include <nv_helpers/shaderfilemanager.hpp>
#include <nv_helpers/assetsloader.hpp>
#include <nv_helpers/nvprint.hpp>

namespace nv_helpers_gl
{

  class ProgramManager : public nv_helpers::ShaderFileManager  {
  public:
    static const uint32_t PREPROCESS_ONLY_PROGRAM = ~0;
    static const size_t   INVALID_ID = ~0;

    class ProgramID {
    public:
      size_t  m_value;

      ProgramID() : m_value(INVALID_ID) {}

      ProgramID( size_t b) : m_value(b) {}
      operator size_t() const { return m_value; }
      ProgramID& operator=( size_t b) { m_value = b; return *this; }

      bool isValid() const 
      {
        return m_value != INVALID_ID;
      }
    };

    struct Program {
      Program() : program(0) {}

      uint32_t                  program;
      std::vector<Definition>   definitions;
    };

    ProgramID createProgram(size_t num, const Definition* definitions);
    ProgramID createProgram(const std::vector<Definition>& definitions);
    ProgramID createProgram(const Definition& def0, const Definition& def1 = Definition(), const Definition& def2 = Definition(), const Definition& def3 = Definition(), const Definition& def4 = Definition());

    void destroyProgram( ProgramID idx );
    void reloadProgram( ProgramID idx );

    void reloadPrograms();
    void deletePrograms();
    bool areProgramsValid();


    bool isValid( ProgramID idx ) const;
    unsigned int get(ProgramID idx) const;
    Program& getProgram(ProgramID idx);
    const Program& getProgram(ProgramID idx) const;

    std::string m_useCacheFile;
    bool        m_preferCache;
    bool        m_preprocessOnly;
    bool        m_rawOnly;

    ProgramManager() 
      : m_preprocessOnly(false)
      , m_preferCache(false)
      , m_rawOnly(false)
    {
      m_filetype = FILETYPE_GLSL;
    }

  private:
    bool setupProgram(Program& prog);

    bool loadBinary( GLuint program, const std::string& combinedPrepend, const std::string& combinedFilenames );
    void saveBinary( GLuint program, const std::string& combinedPrepend, const std::string& combinedFilenames );
    std::string binaryName(const std::string& combinedPrepend, const std::string& combinedFilenames);

    std::vector<Program>      m_programs;
  };

}


#endif
