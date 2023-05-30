/*
 * Copyright (c) 2014-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#include "nvprint.hpp"

#include <cstdarg>
#include <limits.h>
#include <mutex>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

static std::string         s_logFileName = "log_nvprosample.txt";
static std::vector<char>   s_strBuffer;  // Persistent allocation for formatted text.
static FILE*               s_fd                   = nullptr;
static bool                s_bLogReady            = false;
static bool                s_bPrintLogging        = true;
static uint32_t            s_bPrintFileLogging    = LOGBITS_ALL;
static uint32_t            s_bPrintConsoleLogging = LOGBITS_ALL;
static int                 s_printLevel           = -1;  // <0 mean no level prefix
static PFN_NVPRINTCALLBACK s_printCallback        = nullptr;
static std::mutex          s_mutex;

void nvprintSetLogFileName(const char* name) noexcept
{
  std::lock_guard<std::mutex> lockGuard(s_mutex);

  if(name == NULL || s_logFileName == name)
    return;

  try
  {
    s_logFileName = name;
  }
  catch(const std::exception& e)
  {
    fputs("nvprintfSetLogFileName could not allocate space for new file name. Additional info below:", stderr);
    fputs(e.what(), stderr);
  }

  if(s_fd)
  {
    fclose(s_fd);
    s_fd        = nullptr;
    s_bLogReady = false;
  }
}
void nvprintSetCallback(PFN_NVPRINTCALLBACK callback)
{
  s_printCallback = callback;
}
void nvprintSetLevel(int l)
{
  s_printLevel = l;
}
int nvprintGetLevel()
{
  return s_printLevel;
}
void nvprintSetLogging(bool b)
{
  s_bPrintLogging = b;
}

void nvprintSetFileLogging(bool state, uint32_t mask)
{
  std::lock_guard<std::mutex> lockGuard(s_mutex);

  if(state)
  {
    s_bPrintFileLogging |= mask;
  }
  else
  {
    s_bPrintFileLogging &= ~mask;
  }
}

void nvprintSetConsoleLogging(bool state, uint32_t mask)
{
  std::lock_guard<std::mutex> lockGuard(s_mutex);

  if(state)
  {
    s_bPrintConsoleLogging |= mask;
  }
  else
  {
    s_bPrintConsoleLogging &= ~mask;
  }
}

void nvprintf2(va_list& vlist, const char* fmt, int level) noexcept
{
  if(s_bPrintLogging == false)
  {
    return;
  }

  std::lock_guard<std::mutex> lockGuard(s_mutex);

  // Format the inputs into an std::string.
  {
    // Copy vlist as it may be modified by vsnprintf.
    va_list vlistCopy;
    va_copy(vlistCopy, vlist);
    const int charactersNeeded = vsnprintf(s_strBuffer.data(), s_strBuffer.size(), fmt, vlistCopy);
    va_end(vlistCopy);

    // Check that:
    // * vsnprintf did not return an error;
    // * The string (plus null terminator) could fit in a vector.
    if((charactersNeeded < 0) || (size_t(charactersNeeded) > s_strBuffer.max_size() - 1))
    {
      // Formatting error
      fputs("nvprintf2: Internal message formatting error.", stderr);
      return;
    }

    // Increase the size of s_strBuffer as needed if there wasn't enough space.
    if(size_t(charactersNeeded) >= s_strBuffer.size())
    {
      try
      {
        // Make sure to add 1, because vsnprintf doesn't count the terminating
        // null character. This can potentially throw an exception.
        s_strBuffer.resize(size_t(charactersNeeded) + 1, '\0');
      }
      catch(const std::exception& e)
      {
        fputs("nvprintf2: Error resizing buffer to hold message. Additional info below:", stderr);
        fputs(e.what(), stderr);
        return;
      }

      // Now format it; we know this will succeed.
      (void)vsnprintf(s_strBuffer.data(), s_strBuffer.size(), fmt, vlist);
    }
  }

#ifdef WIN32
  OutputDebugStringA(s_strBuffer.data());
#endif

  if(s_bPrintFileLogging & (1 << level))
  {
    if(s_bLogReady == false)
    {
      s_fd        = fopen(s_logFileName.c_str(), "wt");
      s_bLogReady = true;
    }
    if(s_fd)
    {
      fputs(s_strBuffer.data(), s_fd);
    }
  }

  if(s_printCallback)
  {
    s_printCallback(level, s_strBuffer.data());
  }

  if(s_bPrintConsoleLogging & (1 << level))
  {
    fputs(s_strBuffer.data(), stderr);
  }
}
void nvprintf(
#ifdef _MSC_VER
    _Printf_format_string_
#endif
    const char* fmt,
    ...) noexcept
{
  //    int r = 0;
  va_list vlist;
  va_start(vlist, fmt);
  nvprintf2(vlist, fmt, s_printLevel);
  va_end(vlist);
}
void nvprintfLevel(int level,
#ifdef _MSC_VER
                   _Printf_format_string_
#endif
                   const char* fmt,
                   ...) noexcept
{
  va_list vlist;
  va_start(vlist, fmt);
  nvprintf2(vlist, fmt, level);
  va_end(vlist);
}
