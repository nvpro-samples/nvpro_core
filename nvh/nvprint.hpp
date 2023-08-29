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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2023 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef __NVPRINT_H__
#define __NVPRINT_H__

#include <cstdarg>
#include <fmt/format.h>
#include <functional>
#include <stdint.h>
#include <string>

/**
  Multiple functions and macros that should be used for logging purposes,
  rather than printf. These can print to multiple places at once
  \fn nvprintf etc
  
  Configuration:
  - nvprintSetLevel : sets default loglevel
  - nvprintGetLevel : gets default loglevel
  - nvprintSetLogFileName : sets log filename
  - nvprintSetLogging : sets file logging state
  - nvprintSetCallback : sets custom callback

  Printf-style functions and macros.
  These take printf-style specifiers.
  - nvprintf : prints at default loglevel
  - nvprintfLevel : nvprintfLevel print at a certain loglevel
  - LOGI : macro that does nvprintfLevel(LOGLEVEL_INFO)
  - LOGW : macro that does nvprintfLevel(LOGLEVEL_WARNING)
  - LOGE : macro that does nvprintfLevel(LOGLEVEL_ERROR)
  - LOGE_FILELINE : macro that does nvprintfLevel(LOGLEVEL_ERROR) combined with filename/line
  - LOGD : macro that does nvprintfLevel(LOGLEVEL_DEBUG) (only in debug builds)
  - LOGOK : macro that does nvprintfLevel(LOGLEVEL_OK)
  - LOGSTATS : macro that does nvprintfLevel(LOGLEVEL_STATS)

  std::print-style functions and macros.
  These take std::format-style specifiers
  (https://en.cppreference.com/w/cpp/utility/format/formatter#Standard_format_specification).
  - nvprintLevel : print at a certain loglevel
  - PRINTI : macro that does nvprintLevel(LOGLEVEL_INFO)
  - PRINTW : macro that does nvprintLevel(LOGLEVEL_WARNING)
  - PRINTE : macro that does nvprintLevel(LOGLEVEL_ERROR)
  - PRINTE_FILELINE : macro that does nvprintLevel(LOGLEVEL_ERROR) combined with filename/line
  - PRINTD : macro that does nvprintLevel(LOGLEVEL_DEBUG) (only in debug builds)
  - PRINTOK : macro that does nvprintLevel(LOGLEVEL_OK)
  - PRINTSTATS : macro that does nvprintLevel(LOGLEVEL_STATS)

  Safety:
  On error, all functions print an error message.
  All functions are thread-safe.
  Printf-style functions have annotations that should produce warnings at
  compile-time or when performing static analysis. Their format strings may be
  dynamic - but this can be bad if an adversary can choose the content of the
  format string.
  std::print-style functions are safer: they produce compile-time errors, and
  their format strings must be compile-time constants. Dynamic formatting
  should be performed outside of printing, like this:
  ```
  ImGui::InputText("Enter a format string: ", userFormat, sizeof(userFormat));
  try
  {
    std::string formatted = fmt::vformat(userFormat, ...);
  }
  catch (const std::exception& e)
  {
    (error handling...)
  }
  PRINTI("{}", formatted);
  ```

  Text encoding:
  Printing to the Windows debug console is the only operation that assumes a
  text encoding, which is ANSI. In all other cases, strings are copied into
  the output.
*/


// trick for pragma message so we can write:
// #pragma message(__FILE__"("S__LINE__"): blah")
#define S__(x) #x
#define S_(x) S__(x)
#define S__LINE__ S_(__LINE__)

#ifndef LOGLEVEL_INFO
#define LOGLEVEL_INFO 0
#define LOGLEVEL_WARNING 1
#define LOGLEVEL_ERROR 2
#define LOGLEVEL_DEBUG 3
#define LOGLEVEL_STATS 4
#define LOGLEVEL_OK 7
#define LOGBIT_INFO (1 << LOGLEVEL_INFO)
#define LOGBIT_WARNING (1 << LOGLEVEL_WARNING)
#define LOGBIT_ERROR (1 << LOGLEVEL_ERROR)
#define LOGBIT_DEBUG (1 << LOGLEVEL_DEBUG)
#define LOGBIT_STATS (1 << LOGLEVEL_STATS)
#define LOGBIT_OK (1 << LOGLEVEL_OK)
#define LOGBITS_ERRORS LOGBIT_ERROR
#define LOGBITS_WARNINGS (LOGBITS_ERRORS | LOGBIT_WARNING)
#define LOGBITS_INFO (LOGBITS_WARNINGS | LOGBIT_INFO)
#define LOGBITS_DEBUG (LOGBITS_INFO | LOGBIT_DEBUG)
#define LOGBITS_STATS (LOGBITS_DEBUG | LOGBIT_STATS)
#define LOGBITS_OK (LOGBITS_WARNINGS | LOGBIT_OK)
#define LOGBITS_ALL 0xffffffffu
#endif

// Set/get the default level for calls to nvprintf(). Use LOGLEVEL_*.
void nvprintSetLevel(int l);
int  nvprintGetLevel();

void nvprintSetLogFileName(const char* name) noexcept;

// Globally enable/disable all nvprint output and logging
void nvprintSetLogging(bool b);

// Update the bitmask of which levels receive file and stderr output, or
// trigger breakpoints. `state` controls whether to enable or disable the bits
// in `mask`. Use LOGBITS_*.
void nvprintSetFileLogging(bool state, uint32_t mask = ~0);
void nvprintSetConsoleLogging(bool state, uint32_t mask = ~0);
void nvprintSetBreakpoints(bool state, uint32_t mask = LOGBITS_ERRORS);

// Set a custom print handler. Called in addition to file and console logging.
using PFN_NVPRINTCALLBACK = std::function<void(int level, const char* msg)>;
void nvprintSetCallback(PFN_NVPRINTCALLBACK callback);

// Printf-style macros and functions.
#define LOGI(...)                                                                                                      \
  {                                                                                                                    \
    nvprintfLevel(LOGLEVEL_INFO, __VA_ARGS__);                                                                         \
  }
#define LOGW(...)                                                                                                      \
  {                                                                                                                    \
    nvprintfLevel(LOGLEVEL_WARNING, __VA_ARGS__);                                                                      \
  }
#define LOGE(...)                                                                                                      \
  {                                                                                                                    \
    nvprintfLevel(LOGLEVEL_ERROR, __VA_ARGS__);                                                                        \
  }
#define LOGE_FILELINE(...)                                                                                             \
  {                                                                                                                    \
    nvprintfLevel(LOGLEVEL_ERROR, __FILE__ "(" S__LINE__ "): **ERROR**:\n" __VA_ARGS__);                               \
  }
#ifndef NDEBUG
#define LOGD(...)                                                                                                      \
  {                                                                                                                    \
    nvprintfLevel(LOGLEVEL_DEBUG, __FILE__ "(" S__LINE__ "): Debug Info:\n" __VA_ARGS__);                              \
  }
#else
#define LOGD(...)
#endif
#define LOGOK(...)                                                                                                     \
  {                                                                                                                    \
    nvprintfLevel(LOGLEVEL_OK, __VA_ARGS__);                                                                           \
  }
#define LOGSTATS(...)                                                                                                  \
  {                                                                                                                    \
    nvprintfLevel(LOGLEVEL_STATS, __VA_ARGS__);                                                                        \
  }

void nvprintf(
#ifdef _MSC_VER
    _Printf_format_string_
#endif
    const char* fmt,
    ...) noexcept
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 1, 2)));
#endif
;

void nvprintfLevel(int level,
#ifdef _MSC_VER
                   _Printf_format_string_
#endif
                   const char* fmt,
                   ...) noexcept
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 2, 3)));
#endif
;

// std::print-style macros and functions.
// Use fmt::format's built-in checking if the compiler supports consteval,
// which cleans up how the macros appear in Intellisense. Otherwise, use
// FMT_STRING; this will be messier. In either case, the last line of the
// compiler error will point to the line with the incorrect print specifier.
#ifdef FMT_HAS_CONSTEVAL
#define PRINT_CHECK_FMT
#else
#define PRINT_CHECK_FMT FMT_STRING
#endif
// This macro catches exceptions from fmt::format. This gives us compile-time
// checking, while still making these functions have the same noexcept
// semantics as nvprintf.
#define PRINT_CATCH(lvl, fmtstr, ...)                                                                                  \
  {                                                                                                                    \
    try                                                                                                                \
    {                                                                                                                  \
      nvprintLevel(lvl, fmt::format(PRINT_CHECK_FMT(fmtstr), __VA_ARGS__));                                            \
    }                                                                                                                  \
    catch(const std::exception&)                                                                                       \
    {                                                                                                                  \
      nvprintLevel(LOGLEVEL_ERROR, "PRINT_CATCH: Could not format string.\n");                                         \
    }                                                                                                                  \
  }
#define PRINTI(fmtstr, ...) PRINT_CATCH(LOGLEVEL_INFO, fmtstr, __VA_ARGS__)
#define PRINTW(fmtstr, ...) PRINT_CATCH(LOGLEVEL_WARNING, fmtstr, __VA_ARGS__)
#define PRINTE(fmtstr, ...) PRINT_CATCH(LOGLEVEL_ERROR, fmtstr, __VA_ARGS__)
#define PRINTE_FILELINE(fmtstr, ...)                                                                                   \
  PRINT_CATCH(LOGLEVEL_ERROR, __FILE__ "(" S__LINE__ "): **ERROR**:\n" fmtstr, __VA_ARGS__)
#ifndef NDEBUG
#define PRINTD(fmtstr, ...) PRINT_CATCH(LOGLEVEL_DEBUG, __FILE__ "(" S__LINE__ "): Debug Info:\n" fmtstr, __VA_ARGS__)
#else
#define PRINTD(...)
#endif
#define PRINTOK(fmtstr, ...) PRINT_CATCH(LOGLEVEL_OK, fmtstr, __VA_ARGS__)
#define PRINTSTATS(fmtstr, ...) PRINT_CATCH(LOGLEVEL_STATS, fmtstr, __VA_ARGS__)

// Directly prints a message at the given level, without formatting.
void nvprintLevel(int level, const std::string& msg) noexcept;
void nvprintLevel(int level, const char* msg) noexcept;

#endif
