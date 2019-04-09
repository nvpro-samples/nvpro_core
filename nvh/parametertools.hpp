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

#ifndef __NVPARMETERTOOLS_H__
#define __NVPARMETERTOOLS_H__


#include "platform.h"

#include <string>
#include <vector>
#include <functional>

namespace nvh {

  class ParameterList {
  public:

    typedef std::function<void(uint32_t)> Callback;

    enum Type {
      TYPE_FLOAT,
      TYPE_INT,
      TYPE_UINT,
      TYPE_BOOL,
      TYPE_BOOL_VALUE,
      TYPE_STRING,
      TYPE_FILENAME,
      TYPE_TRIGGER,
    };

    ParameterList();

    uint32_t append(const ParameterList& list);

    // Add a parameter. The name can be given in format: name[|help text], for example: "winsize|Set window size"
    uint32_t add(const char* name, float* destination, Callback callback = nullptr, uint32_t length = 1, float min = -FLT_MAX, float max = FLT_MAX);
    uint32_t add(const char* name, int32_t* destination, Callback callback = nullptr, uint32_t length = 1, int32_t min = -INT_MAX, int32_t max = +INT_MAX);
    uint32_t add(const char* name, uint32_t* destination, Callback callback = nullptr, uint32_t length = 1, uint32_t min = 0, uint32_t max = 0xFFFFFFFF);
    uint32_t add(const char* name, bool* destination, Callback callback = nullptr, uint32_t length = 1 );
    uint32_t add(const char* name, bool* destination, bool value, Callback callback = nullptr, uint32_t length = 1);
    uint32_t add(const char* name, std::string* destination, Callback callback = nullptr, uint32_t length = 1);
    uint32_t add(const char* name, Callback callback, uint32_t length = 0);

    // if the parameter "name" starts with "." then we test the variable against this file-ending rather than
    // treating name as commandline option.  So an argument that ends with ".blah" will trigger this parameter
    uint32_t addFilename(const char* name, std::string* destination, Callback callback = nullptr);
    

    // Set help of a parameter, returns the parameterIndex
    uint32_t setHelp(uint32_t parameterIndex, const char* helptext);

    // returns number of tokens found
    // relative filenames get the defaultFilePath prepended
    uint32_t applyTokens(uint32_t argCount, const char** argv, const char* paramPrefix = nullptr, const char* defaultFilePath = nullptr) const;
    // tests only single argument, increases arg by appropriate length on success (returns true)
    bool applyParameters(uint32_t argCount, const char** argv, uint32_t &arg, const char* paramPrefix = nullptr, const char* defaultFilePath = nullptr) const;

    void print() const;

    // separators are all space characters 
    // preserves quotes based on "", converts backslashes, uses # as line comment
    // modifies content string by setting 0 at separators
    static void tokenizeString(std::string& content, std::vector<const char*>& args);
    static const char* toString(Type typ);

  private:
    struct Parameter {
      Type          type;
      std::string   name;
      uint32_t      readLength;
      uint32_t      writeLength;
      union {
        uint32_t    u32;
        int32_t     s32;
        float       f32;
        bool        b;
      } minmax[2];
      union {
        uint32_t*     u32;
        int32_t*      s32;
        float*        f32;
        bool*         b;
        std::string*  str;
        void*         ptr;
      } destination;
      Callback        callback = nullptr;
      std::string     helptext;

      Parameter() {}
      Parameter(Type type, const char* name, Callback callback, void* destination, uint32_t readLength, uint32_t writeLength);
    };

    std::vector<Parameter>  m_parameters;

    Parameter makeParam(Type type, const char* name, Callback callback, void* destination, uint32_t readLength, uint32_t writeLength);
  };


  class ParameterSequence {
  public:
    ParameterSequence()
      : m_list(nullptr), m_index(0), m_iteration(0)
    {}

    void init(const ParameterList* list, const std::vector<const char*>& tokens)
    {
      m_tokens = tokens;
      m_list = list;
    }

    // returns true if finished with all tokens, otherwise processes until next separator token is found
    bool advanceIteration(const char* separator, uint32_t separatorArgLength, uint32_t &argBegin, uint32_t &argCount);
    // also applies parameterlist
    bool applyIteration(const char* separator, uint32_t separatorArgLength=0, const char* paramPrefix = nullptr, const char* defaultFilePath = nullptr);
    // sets iteration to beginning
    void resetIteration();

    bool isActive() const
    {
      return m_list && m_index && m_iteration;
    }

    uint32_t getIteration() const
    {
      return m_iteration;
    }

    const char* getSeparatorArg(uint32_t offset) const
    {
      return m_separator != ~0 ? m_tokens[m_separator + offset + 1] : ""; 
    }

  private:
    const ParameterList*      m_list;
    std::vector<const char*>  m_tokens;
    size_t                    m_index;
    size_t                    m_separator;
    uint32_t                  m_iteration;
  };

}


#endif


