/*
 * Copyright (c) 2014-2023, NVIDIA CORPORATION.  All rights reserved.
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

#include <limits.h>
#include <mutex>
#include <vector>

#ifdef WIN32
#include <io.h>
#include <windows.h>
#else
#include <signal.h>
#include <unistd.h>
#endif

enum class TriState
{
  eUnknown,
  eFalse,
  eTrue
};

static std::string         s_logFileName = "log_nvprosample.txt";
static std::vector<char>   s_strBuffer;  // Persistent allocation for formatted text.
static FILE*               s_fd                   = nullptr;
static bool                s_bLogReady            = false;
static bool                s_bPrintLogging        = true;
static uint32_t            s_bPrintFileLogging    = LOGBITS_ALL;
static uint32_t            s_bPrintConsoleLogging = LOGBITS_ALL;
static uint32_t            s_bPrintBreakpoints    = 0;
static int                 s_printLevel           = -1;  // <0 mean no level prefix
static PFN_NVPRINTCALLBACK s_printCallback        = nullptr;
static TriState            s_consoleSupportsColor = TriState::eUnknown;
// Lock this when modifying any static variables.
// Because it is a recursive mutex, its owner can lock it multiple times.
static std::recursive_mutex s_mutex;

void nvprintSetLogFileName(const char* name) noexcept
{
  std::lock_guard<std::recursive_mutex> lockGuard(s_mutex);

  if(name == NULL || s_logFileName == name)
    return;

  try
  {
    s_logFileName = name;
  }
  catch(const std::exception& e)
  {
    nvprintLevel(LOGLEVEL_ERROR, "nvprintfSetLogFileName could not allocate space for new file name. Additional info below:");
    nvprintLevel(LOGLEVEL_ERROR, e.what());
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
  std::lock_guard<std::recursive_mutex> lockGuard(s_mutex);

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
  std::lock_guard<std::recursive_mutex> lockGuard(s_mutex);

  if(state)
  {
    s_bPrintConsoleLogging |= mask;
  }
  else
  {
    s_bPrintConsoleLogging &= ~mask;
  }
}

void nvprintSetBreakpoints(bool state, uint32_t mask)
{
  std::lock_guard<std::recursive_mutex> lockGuard(s_mutex);

  if(state)
  {
    s_bPrintBreakpoints |= mask;
  }
  else
  {
    s_bPrintBreakpoints &= ~mask;
  }
}

void nvprintfV(va_list& vlist, const char* fmt, int level) noexcept
{
  if(s_bPrintLogging == false)
  {
    return;
  }

  // Format the inputs into s_strBuffer.
  std::lock_guard<std::recursive_mutex> lockGuard(s_mutex);
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
      nvprintLevel(LOGLEVEL_ERROR, "nvprintfV: Internal message formatting error.");
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
        nvprintLevel(LOGLEVEL_ERROR, "nvprintfV: Error resizing buffer to hold message. Additional info below:");
        nvprintLevel(LOGLEVEL_ERROR, e.what());
        return;
      }

      // Now format it; we know this will succeed.
      (void)vsnprintf(s_strBuffer.data(), s_strBuffer.size(), fmt, vlist);
    }
  }

  nvprintLevel(level, s_strBuffer.data());
}

void nvprintLevel(int level, const std::string& msg) noexcept
{
  nvprintLevel(level, msg.c_str());
}

void nvprintLevel(int level, const char* msg) noexcept
{
  std::lock_guard<std::recursive_mutex> lockGuard(s_mutex);

#ifdef WIN32
  // Note: Maybe we could consider changing to a text encoding of UTF-8 in
  // the future, bring in calls to Windows' MultiByteToWideChar, and call
  // OutputDebugStringW.
  OutputDebugStringA(msg);
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
      fputs(msg, s_fd);
    }
  }

  if(s_printCallback)
  {
    s_printCallback(level, msg);
  }

  if(s_bPrintConsoleLogging & (1 << level))
  {
    // Determine if the output supports ANSI color sequences only once to avoid
    // many calls to isatty.
    if(TriState::eUnknown == s_consoleSupportsColor)
    {
      // Determining this perfectly is difficult; terminfo does it by storing
      // a large table of all consoles it knows about. For now, we assume
      // all consoles support colors, and all pipes do not.
#ifdef WIN32
      const bool supportsColor = _isatty(_fileno(stderr)) && _isatty(_fileno(stdout));
#else
      const bool supportsColor = isatty(fileno(stderr)) && isatty(fileno(stdout));
#endif
      s_consoleSupportsColor = (supportsColor ? TriState::eTrue : TriState::eFalse);
    }

    FILE* outStream = (((1 << level) & LOGBITS_ERRORS) ? stderr : stdout);

    if(TriState::eTrue == s_consoleSupportsColor)
    {
      // Set the foreground color depending on level:
      // https://en.wikipedia.org/wiki/ANSI_escape_code#SGR_(Select_Graphic_Rendition)_parameters
      if(level == LOGLEVEL_OK)
      {
        fputs("\033[32m", outStream);  // Green
      }
      else if(level == LOGLEVEL_ERROR)
      {
        fputs("\033[31m", outStream);  // Red
      }
      else if(level == LOGLEVEL_WARNING)
      {
        fputs("\033[33m", outStream);  // Yellow
      }
      else if(level == LOGLEVEL_DEBUG)
      {
        fputs("\033[36m", outStream);  // Cyan
      }
    }

    fputs(msg, outStream);

    if(TriState::eTrue == s_consoleSupportsColor)
    {
      // Reset all attributes
      fputs("\033[0m", outStream);
    }
  }

  if(s_bPrintBreakpoints & (1 << level))
  {
#ifdef WIN32
    DebugBreak();
#else
    raise(SIGTRAP);
#endif
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
  nvprintfV(vlist, fmt, s_printLevel);
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
  nvprintfV(vlist, fmt, level);
  va_end(vlist);
}
