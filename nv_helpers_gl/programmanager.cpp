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

#include <main.h>


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

    std::string findFile( std::string const & infilename, std::vector<std::string> directories)
    {
      std::ifstream stream;

      for (size_t i = 0; i < directories.size(); i++ ){
        std::string filename = directories[i] + "/" + infilename;
        stream.open(filename.c_str());
        if (stream.is_open()) return filename;
      }
      return infilename;
    }

    std::string loadFile( std::string const & infilename)
  {
    std::string result;
    std::string filename = infilename;

    std::ifstream stream(filename.c_str());
    if(!stream.is_open()){
      nvprintfLevel(LOGLEVEL_WARNING,"file not found:%s\n",filename.c_str());
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

  inline std::string fixFilename(std::string const & filename)
  {
#ifdef _WIN32
    // workaround for older versions of nsight
    std::string fixedname;
    for (size_t i = 0; i < filename.size(); i++){
      char c = filename[i];
      if ( c == '/' || c == '\\'){
        fixedname.append("\\\\");
      }
      else{
        fixedname.append(1,c);
      }
    }
    return fixedname;
#else
    return filename; 
#endif
  }

  inline std::string markerString(int line, std::string const & filename, int fileid)
  {
    if (GLEW_ARB_shading_language_include){
      return ProgramManager::format("#line %d \"",line) + fixFilename(filename) + std::string("\"\n");
    }
    else{
      return ProgramManager::format("#line %d %d\n",line,fileid);
    }
  }

  std::string manualInclude (
    std::string const & filenameorig,
    std::string const & prepend,
    const std::vector<std::string>& directories,
    const ProgramManager::IncludeRegistry &includes)
  {
    std::string filename = findFile(filenameorig,directories);
    std::string source = loadFile(filename);

    if (source.empty()){
      return std::string();
    }

    std::stringstream stream;
    stream << source;
    std::string line, text;

    // Handle command line defines
    text += prepend;
#if NV_LINE_MARKERS
    text +=  markerString(1,filename,0);
#endif
    int lineCount  = 0;
    while(std::getline(stream, line))
    {
      std::size_t Offset = 0;
      lineCount++;

      // Version
      Offset = line.find("#version");
      if(Offset != std::string::npos)
      {
        std::size_t CommentOffset = line.find("//");
        if(CommentOffset != std::string::npos && CommentOffset < Offset)
          continue;

        // Reorder so that the #version line is always the first of a shader text
        text = line + std::string("\n") + text + std::string("//") + line + std::string("\n");
        continue;
      }

      // Include
      Offset = line.find("#include");
      if(Offset != std::string::npos)
      {
        std::size_t CommentOffset = line.find("//");
        if(CommentOffset != std::string::npos && CommentOffset < Offset)
          continue;

        std::string Include = parseInclude(line, Offset);

        for(std::size_t i = 0; i < includes.size(); ++i)
        {
          if (includes[i].name != Include) continue;

          std::string PathName = findFile(includes[i].filename,directories);
          std::string Source = loadFile(PathName);
          if(!Source.empty())
          {
#if NV_LINE_MARKERS
            text += markerString(1,PathName,1);
#endif
            text += Source;
#if NV_LINE_MARKERS
            text += std::string("\n") + markerString(lineCount + 1, filename, 0);
#endif
            break;
          }
        }

        continue;
      }

      text += line + "\n";
    }

    return text;
  }



  GLuint createShader
    (
    GLenum Type,
    std::string const & preprocessed
    )
  {
    GLuint name = 0;

    if (!preprocessed.empty()){
      char const * sourcePointer = preprocessed.c_str();
      name = glCreateShader(Type);
      glShaderSource(name, 1, &sourcePointer, NULL);
      glCompileShader(name);
    }

    return name;
  }

  std::string preprocess(
    std::string const & prepend, 
    std::string const & filename,
    const std::vector<std::string>& directories,
    const ProgramManager::IncludeRegistry &includes
    )
  {
    if(!filename.empty())
    {
      return manualInclude(filename,prepend,directories,includes);
    }
    else{
      return std::string();
    }
  }

  void ProgramManager::registerInclude(std::string const & name, std::string const & filename)
  {
    IncludeEntry entry;
    entry.name = name;
    entry.filename = filename;

    m_includes.push_back(entry);
  }

  bool  ProgramManager::createProgram(Program& prog, size_t num, Definition* definitions)
  {
    prog.program = 0;

    if (!num) return false;

    if (!m_preprocessOnly){
      prog.program = glCreateProgram();
    }
    else{
      prog.program = PREPROCESS;
    }
    
    for (size_t i = 0; i < num; i++) {
      definitions[i].preprocessed = preprocess(m_prepend + definitions[i].prepend, definitions[i].filename, m_directories, m_includes);
      if (m_preprocessOnly) continue;

      GLuint shader = createShader(definitions[i].type, definitions[i].preprocessed);
      if (!shader || !checkShader(shader,definitions[i].filename)){
        glDeleteShader(shader);
        glDeleteProgram(prog.program);
        prog.program = 0;
        return false;
      }
      glAttachShader(prog.program, shader);
      glDeleteShader(shader);
    }

    if (m_preprocessOnly){
      return true;
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
    prog.definitions.reserve(num);
    for (size_t i = 0; i < num; i++){
      prog.definitions.push_back(definitions[i]);
    }

    if (createProgram(prog,num,&prog.definitions[0])){

    }

    for (size_t i = 0; i < m_programs.size(); i++){
      if (m_programs[i].definitions.empty()){
        m_programs[i] = prog;
        return i;
      }
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
      if (m_programs[i].program && m_programs[i].program != PREPROCESS){
        glDeleteProgram(m_programs[i].program);
      }
      m_programs[i].program = 0;
    }
  }

  void ProgramManager::reloadPrograms()
  {
    nvprintf("Reloading programs...\n");

    bool old = m_preprocessOnly;

    for (size_t i = 0; i < m_programs.size(); i++){
      if (m_programs[i].program && m_programs[i].program != PREPROCESS){
        glDeleteProgram(m_programs[i].program);
      }
      m_preprocessOnly = m_programs[i].program == PREPROCESS;
      m_programs[i].program = 0;
      if (!m_programs[i].definitions.empty()){
        createProgram(m_programs[i],m_programs[i].definitions.size(), m_programs[i].definitions.size() ? &m_programs[i].definitions[0] : NULL);
      }

    }

    m_preprocessOnly = old;

    nvprintf("done\n");
  }

  bool ProgramManager::isValid( ProgramID idx ) const
  {
    return m_programs[idx].definitions.empty() || m_programs[idx].program != 0;
  }

  GLuint ProgramManager::get( ProgramID idx ) const
  {
    assert( m_programs[idx].program != PREPROCESS);
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

  void ProgramManager::destroyProgram( ProgramID idx )
  {
    if (m_programs[idx].program && m_programs[idx].program != PREPROCESS){
      glDeleteProgram(m_programs[idx].program);
    }
    m_programs[idx].program = 0;
    m_programs[idx].definitions.clear();
  }

}//namespace nvglf


