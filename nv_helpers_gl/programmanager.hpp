/*
 * Copyright 1993-2014 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 *
 */

#ifndef _PROGRAMMANAGER_INCLUDED
#define _PROGRAMMANAGER_INCLUDED

#include "common.hpp"

#define LINE_MARKERS           1

namespace nv_helpers
{

  class ProgramManager {
  public:
    struct IncludeEntry {
      std::string   name;
      std::string   filename;
    };

    typedef std::vector<IncludeEntry> IncludeRegistry;

    static std::string format(const char* msg, ...);

  public:
    class ProgramID {
      static const size_t INVALID = ~0;
    public:
      size_t  m_value;

      ProgramID() : m_value(0) {}

      ProgramID( size_t b) : m_value(b) {}
      operator size_t() const { return m_value; }
      ProgramID& operator=( size_t b) { m_value = b; return *this; }

    };

    struct Definition {

      Definition() : type(0) , prepend(""), filename("") {}
      Definition(GLenum type, std::string const & prepend, std::string const & filename) : type(type) , prepend(prepend) , filename(filename) {}
      Definition(GLenum type, std::string const & filename) : type(type) , prepend("") , filename(filename) {}

      GLenum      type;
      std::string prepend;
      std::string filename;
    };

    struct Program {
      Program() : program(0) {}

      GLuint                    program;
      std::vector<Definition>   definitions;
    };

    void registerInclude(std::string const & name, std::string const & filename);

    ProgramID createProgram(size_t num, const Definition* definitions);
    ProgramID createProgram(const std::vector<Definition>& definitions);
    ProgramID createProgram(const Definition& def0, const Definition& def1 = Definition(), const Definition& def2 = Definition(), const Definition& def3 = Definition(), const Definition& def4 = Definition());

    void reloadPrograms();
    void deletePrograms();
    bool areProgramsValid();


    bool ProgramManager::isValid( ProgramID idx ) const;
    GLuint get(ProgramID idx) const;
    Program& getProgram(ProgramID idx);
    const Program& getProgram(ProgramID idx) const;

    std::string m_prepend;

  private:
    bool createProgram(Program& prog, size_t num, const Definition* definitions);

    IncludeRegistry       m_includes;
    std::vector<Program>  m_programs;
  };

}//namespace nv_helpers


#endif//_PROGRAM_INCLUDED
