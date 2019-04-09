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

/*
 * This file contains code derived from glf by Christophe Riccio, www.g-truc.net
 * Copyright (c) 2005 - 2015 G-Truc Creation (www.g-truc.net)
 * https://github.com/g-truc/ogl-samples/blob/master/framework/compiler.cpp
 */

#include "shaderfilemanager.hpp"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iostream>

#include "nvh/assetsloader.hpp"
#include <nvh/misc.hpp>


namespace nvh
{

  std::string ShaderFileManager::format(const char* msg, ...)
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

  std::string parseInclude(std::string const & Line, std::size_t const & Offset)
  {
    std::string Result;

    std::string::size_type IncludeFirstQuote = Line.find("\"", Offset);
    std::string::size_type IncludeSecondQuote = Line.find("\"", IncludeFirstQuote + 1);

    return Line.substr(IncludeFirstQuote + 1, IncludeSecondQuote - IncludeFirstQuote - 1);
  }

  inline std::string fixFilename(std::string const & filename)
  {
#if defined(_WIN32) && 1
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

  inline std::string ShaderFileManager::markerString(int line, std::string const & filename, int fileid)
  {
    if (m_supportsExtendedInclude || m_forceLineFilenames)
    {
      return ShaderFileManager::format("#line %d \"",line) + fixFilename(filename) + std::string("\"\n");
    }
    else{
      return ShaderFileManager::format("#line %d %d\n",line,fileid);
    }
  }

  std::string ShaderFileManager::getIncludeContent(IncludeID idx, std::string &filename)
  {
    IncludeEntry& entry = getInclude(idx);

    filename = entry.filename;

    if (m_forceIncludeContent) {
      return entry.content;
    }

    if (!entry.content.empty() && !AssetLoaderFileExists(entry.filename)) {
      return entry.content;
    }

    std::string content = AssetLoadTextFile(entry.filename, filename);
    return content.empty() ? entry.content : content;
  }

  std::string ShaderFileManager::getContent(std::string const & name, std::string& filename)
  {
    if (name.empty()) {
      return std::string();
    }

    IncludeID idx = findInclude(name);

    if (idx.isValid()) {
      return getIncludeContent(idx, filename);
    }

    // fall back
    filename = name;
    return AssetLoadTextFile(name, filename);
  }

  std::string ShaderFileManager::manualInclude (
    std::string const & filenameorig,
    std::string & filename,
    std::string const & prepend,
    bool foundVersion)
  {
    std::string source = getContent(filenameorig, filename);

    if (source.empty()){
      return std::string();
    }

    std::stringstream stream;
    stream << source;
    std::string line, text;

    // Handle command line defines
    text += prepend;
    if (m_lineMarkers){
      text +=  markerString(1, filename, 0);
    }

    int lineCount  = 0;
    while(std::getline(stream, line))
    {
      std::size_t offset = 0;
      lineCount++;

      // Version
      offset = line.find("#version");
      if(offset != std::string::npos)
      {
        std::size_t commentOffset = line.find("//");
        if(commentOffset != std::string::npos && commentOffset < offset)
          continue;

        if (foundVersion) {
          // someone else already set the version, so just comment out
          text += std::string("//") + line + std::string("\n");
        }
        else {
          // Reorder so that the #version line is always the first of a shader text
          text = line + std::string("\n") + text + std::string("//") + line + std::string("\n");
          foundVersion = true;
        }
        continue;
      }

      // Include
      offset = line.find("#include");
      if(offset != std::string::npos)
      {
        std::size_t commentOffset = line.find("//");
        if(commentOffset != std::string::npos && commentOffset < offset)
          continue;

        std::string Include = parseInclude(line, offset);

        for(std::size_t i = 0; i < m_includes.size(); ++i)
        {
          if (m_includes[i].name != Include) continue;

          std::string includedFilename;
          std::string sourceIncluded = manualInclude(m_includes[i].filename, includedFilename, std::string(), foundVersion);

          if(!sourceIncluded.empty())
          {
            //if (m_lineMarkers){
            //  text += markerString(1, includedFilename, 1);
            //}
              text += sourceIncluded;
            if (m_lineMarkers){
              text += std::string("\n") + markerString(lineCount + 1, filename, 0);
            }
            break;
          }
        }

        continue;
      }

      text += line + "\n";
    }

    return text;
  }

  

  ShaderFileManager::IncludeID  ShaderFileManager::registerInclude(std::string const & name, std::string const & filename, std::string const & content)
  {
    // find if already registered
    for (size_t i = 0; i < m_includes.size(); i++){
      if (m_includes[i].name == name){
        return i;
      }
    }

    IncludeEntry entry;
    entry.name = name;
    entry.filename = filename;
    entry.content = content;

    m_includes.push_back(entry);

    return m_includes.size()-1;
  }


  ShaderFileManager::IncludeID ShaderFileManager::findInclude(std::string const & name) const
  {
    // check registered includes first
    for (std::size_t i = 0; i < m_includes.size(); ++i)
    {
      if (m_includes[i].name == name) {
        return IncludeID(i);
      }
    }

    return IncludeID(INVALID_ID);
  }

  bool ShaderFileManager::loadIncludeContent(IncludeID idx)
  {
    std::string filename;
    getInclude(idx).content = getIncludeContent(idx, filename);
    return !getInclude(idx).content.empty();
  }

  ShaderFileManager::IncludeEntry& ShaderFileManager::getInclude(IncludeID idx)
  {
    return m_includes[idx];
  }

  const ShaderFileManager::IncludeEntry& ShaderFileManager::getInclude( IncludeID idx ) const
  {
    return m_includes[idx];

  }

}


