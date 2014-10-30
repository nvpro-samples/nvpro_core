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
/*
 * This file contains code derived from glf by Christophe Riccio, www.g-truc.net
 */

#include "programmanager.hpp"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iostream>

#ifndef nvprintf
extern void nvprintf(const char * fmt, ...);
//#define nvprintf printf
#endif

#define NV_LINE_MARKERS           1

namespace nv_helpers_gl
{
  std::string ProgramManager::format(const char* msg, ...)
  {
    std::size_t const STRING_BUFFER(8192);
    char text[STRING_BUFFER];
    va_list list;

    if(msg == 0)
      return std::string();

    va_start(list, msg);
    vsprintf(text, msg, list);
    va_end(list);

    return std::string(text);
  }

    std::string loadFile(std::string const & infilename)
  {
    std::string result;
    std::string filename = infilename;

    std::replace(filename.begin(),filename.end(),'\\','/');

    std::ifstream stream(filename.c_str());
    if(!stream.is_open()){
      // search in last subdirectory
      const char* last = strrchr(filename.c_str(),'/');
      const char* sub  = strchr (filename.c_str(),'/');
      if (last && sub && sub < last){
        const char* cur = sub;
        do {
          sub = cur;
          cur = strchr(cur+1,'/');
        } while (cur && cur < last);

        stream.open(sub + 1);
      }
    }
    if (!stream.is_open()){
      // search file only
      if (strrchr(filename.c_str(),'/')){
        stream.open(strrchr(filename.c_str(),'/') + 1);
      }
    }
    if(!stream.is_open()){
      printf("file not found:%s\n",filename.c_str());
      return result;
    }

    stream.seekg(0, std::ios::end);
    result.reserve(stream.tellg());
    stream.seekg(0, std::ios::beg);

    result.assign(
      (std::istreambuf_iterator<char>(stream)),
      std::istreambuf_iterator<char>());

    return result;
  }

  bool validateProgram(GLuint program)
  {
    if(!program)
      return false;

    glValidateProgram(program);
    GLint result = GL_FALSE;
    glGetProgramiv(program, GL_VALIDATE_STATUS, &result);

    if(result == GL_FALSE)
    {
      nvprintf("Validate program\n");
      int infoLogLength;
      glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
      if(infoLogLength > 0)
      {
        std::vector<char> buffer(infoLogLength);
        glGetProgramInfoLog(program, infoLogLength, NULL, &buffer[0]);
        nvprintf( "%s\n", &buffer[0]);
      }
    }

    return result == GL_TRUE;
  }

  bool checkProgram(GLuint program)
  {
    if(!program)
      return false;

    GLint result = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &result);

    //nvprintf( "Linking program\n");
    int infoLogLength;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
    if(infoLogLength > 1)
    {
      std::vector<char> buffer(infoLogLength);
      glGetProgramInfoLog(program, infoLogLength, NULL, &buffer[0]);
      nvprintf( "%s\n", &buffer[0]);
    }

    return result == GL_TRUE;
  }

  bool checkShader
    (
    GLuint shader, 
    std::string const & filename
    )
  {
    if(!shader)
      return false;

    GLint result = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);

    nvprintf( "%s ...\n", filename.c_str());
    int infoLogLength;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
    if(infoLogLength > 1)
    {
      std::vector<char> buffer(infoLogLength);
      glGetShaderInfoLog(shader, infoLogLength, NULL, &buffer[0]);
      nvprintf( "%s\n", &buffer[0]);
    }

    return result == GL_TRUE;
  }

  std::string parseInclude(std::string const & Line, std::size_t const & Offset)
  {
    std::string Result;

    std::string::size_type IncludeFirstQuote = Line.find("\"", Offset);
    std::string::size_type IncludeSecondQuote = Line.find("\"", IncludeFirstQuote + 1);

    return Line.substr(IncludeFirstQuote + 1, IncludeSecondQuote - IncludeFirstQuote - 1);
  }

  inline std::string fixFilename(std::string const & Filename)
  {
#ifdef _WIN32
    // workaround for older versions of nsight
    std::string fixedname;
    for (size_t i = 0; i < Filename.size(); i++){
      char c = Filename[i];
      if ( c == '/' || c == '\\'){
        fixedname.append("\\\\");
      }
      else{
        fixedname.append(1,c);
      }
    }
    return fixedname;
#else
    return Filename; 
#endif
  }

  inline std::string markerString(int line, std::string const & Filename, int fileid)
  {
    if (GLEW_ARB_shading_language_include){
      return ProgramManager::format("#line %d \"",line) + fixFilename(Filename) + std::string("\"\n");
    }
    else{
      return ProgramManager::format("#line %d %d\n",line,fileid);
    }
  }

  std::string manualInclude (
    std::string const & Filename,
    std::string const & prepend,
    const ProgramManager::IncludeRegistry &includes)
  {
    std::string Source = loadFile(Filename);

    if (Source.empty()){
      return std::string();
    }

    std::stringstream Stream;
    Stream << Source;
    std::string Line, Text;

    // Handle command line defines
    Text += prepend;
#if NV_LINE_MARKERS
    Text +=  markerString(1,Filename,0);
#endif
    int lineCount  = 0;
    while(std::getline(Stream, Line))
    {
      std::size_t Offset = 0;
      lineCount++;

      // Version
      Offset = Line.find("#version");
      if(Offset != std::string::npos)
      {
        std::size_t CommentOffset = Line.find("//");
        if(CommentOffset != std::string::npos && CommentOffset < Offset)
          continue;

        // Reorder so that the #version line is always the first of a shader text
        Text = Line + std::string("\n") + Text + std::string("//") + Line + std::string("\n");
        continue;
      }

      // Include
      Offset = Line.find("#include");
      if(Offset != std::string::npos)
      {
        std::size_t CommentOffset = Line.find("//");
        if(CommentOffset != std::string::npos && CommentOffset < Offset)
          continue;

        std::string Include = parseInclude(Line, Offset);

        for(std::size_t i = 0; i < includes.size(); ++i)
        {
          if (includes[i].name != Include) continue;

          std::string PathName = includes[i].filename;
          std::string Source = loadFile(PathName);
          if(!Source.empty())
          {
#if NV_LINE_MARKERS
            Text += markerString(1,PathName,1);
#endif
            Text += Source;
#if NV_LINE_MARKERS
            Text += std::string("\n") + markerString(lineCount + 1, Filename, 0);
#endif
            break;
          }
        }

        continue;
      }

      Text += Line + "\n";
    }

    return Text;
  }



  GLuint createShader
    (
    GLenum Type,
    std::string const & prepend, 
    std::string const & filename,
    const ProgramManager::IncludeRegistry &includes
    )
  {
    GLuint name = 0;

    if(!filename.empty())
    {
      std::string sourceContent = manualInclude(filename,prepend,includes);
      if (!sourceContent.empty()){
        char const * sourcePointer = sourceContent.c_str();
        name = glCreateShader(Type);
        glShaderSource(name, 1, &sourcePointer, NULL);
        glCompileShader(name);
      }
    }

    return name;
  }

  void ProgramManager::registerInclude(std::string const & name, std::string const & filename)
  {
    IncludeEntry entry;
    entry.name = name;
    entry.filename = m_directory + filename;

    m_includes.push_back(entry);
  }

  bool  ProgramManager::createProgram(Program& prog, size_t num, const Definition* definitions)
  {
    prog.program = 0;

    if (!num) return false;

    prog.program = glCreateProgram();
    for (size_t i = 0; i < num; i++) {
      GLuint shader = createShader(definitions[i].type, m_prepend + definitions[i].prepend, m_directory + definitions[i].filename, m_includes);
      if (!shader || !checkShader(shader,definitions[i].filename)){
        glDeleteShader(shader);
        glDeleteProgram(prog.program);
        prog.program = 0;
        return false;
      }
      glAttachShader(prog.program, shader);
      glDeleteShader(shader);
    }

    glLinkProgram(prog.program);

    if (checkProgram(prog.program)){
      return true;
    }

    glDeleteProgram(prog.program);
    prog.program = 0;
    return false;
  }

  ProgramManager::ProgramID ProgramManager::createProgram( const std::vector<ProgramManager::Definition>& definitions )
  {
    return createProgram(definitions.size(), definitions.size() ? &definitions[0] : NULL);
  }

  ProgramManager::ProgramID ProgramManager::createProgram( const Definition& def0, const Definition& def1 /*= ShaderDefinition()*/, const Definition& def2 /*= ShaderDefinition()*/, const Definition& def3 /*= ShaderDefinition()*/, const Definition& def4 /*= ShaderDefinition()*/ )
  {
    std::vector<ProgramManager::Definition> defs;
    defs.push_back(def0);
    if (def1.type) defs.push_back(def1);
    if (def2.type) defs.push_back(def2);
    if (def3.type) defs.push_back(def3);
    if (def4.type) defs.push_back(def4);

    return createProgram(defs);
  }

  ProgramManager::ProgramID ProgramManager::createProgram( size_t num, const ProgramManager::Definition* definitions )
  {
    Program prog;
    if (createProgram(prog,num,definitions)){

    }
    // fixme handle this stuff better, needed here so we get invalid error
    prog.definitions.reserve(num);
    for (size_t i = 0; i < num; i++){
      prog.definitions.push_back(definitions[i]);
    }
    m_programs.push_back(prog);
    return m_programs.size()-1;
  }

  bool ProgramManager::areProgramsValid()
  {
    bool valid = true;
    for (size_t i = 0; i < m_programs.size(); i++){
      valid = valid && isValid( (ProgramID)i );
    }

    return valid;
  }

  void ProgramManager::deletePrograms()
  {
    for (size_t i = 0; i < m_programs.size(); i++){
      if (m_programs[i].program){
        glDeleteProgram(m_programs[i].program);
      }
      m_programs[i].program = 0;
    }
  }

  void ProgramManager::reloadPrograms()
  {
    nvprintf("Reloading programs...\n");
    for (size_t i = 0; i < m_programs.size(); i++){
      if (m_programs[i].program){
        glDeleteProgram(m_programs[i].program);
        m_programs[i].program = 0;
      }
      if (!m_programs[i].definitions.empty()){
        createProgram(m_programs[i],m_programs[i].definitions.size(), m_programs[i].definitions.size() ? &m_programs[i].definitions[0] : NULL);
      }

    }
    nvprintf("done\n");
  }

  bool ProgramManager::isValid( ProgramID idx ) const
  {
    return m_programs[idx].definitions.empty() || m_programs[idx].program != 0;
  }

  GLuint ProgramManager::get( ProgramID idx ) const
  {
    return m_programs[idx].program;
  }

  ProgramManager::Program& ProgramManager::getProgram( ProgramID idx )
  {
    return m_programs[idx];
  }

  const ProgramManager::Program& ProgramManager::getProgram( ProgramID idx ) const
  {
    return m_programs[idx];
  }

}//namespace nvglf


