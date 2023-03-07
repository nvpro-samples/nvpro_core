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


#ifndef __NVPRINT_H__
#define __NVPRINT_H__


#include "platform.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


/**
  Multiple functions and macros that should be used for logging purposes,
  rather than printf
  \fn nvprintf etc
  
  - nvprintf : prints at default loglevel
  - nvprintfLevel : nvprintfLevel print at a certain loglevel
  - nvprintSetLevel : sets default loglevel
  - nvprintGetLevel : gets default loglevel
  - nvprintSetLogFileName : sets log filename
  - nvprintSetLogging : sets file logging state
  - nvprintSetCallback : sets custom callback
  - LOGI : macro that does nvprintfLevel(LOGLEVEL_INFO)
  - LOGW : macro that does nvprintfLevel(LOGLEVEL_WARNING)
  - LOGE : macro that does nvprintfLevel(LOGLEVEL_ERROR)
  - LOGE_FILELINE : macro that does nvprintfLevel(LOGLEVEL_ERROR) combined with filename/line
  - LOGD : macro that does nvprintfLevel(LOGLEVEL_DEBUG) (only in debug builds)
  - LOGOK : macro that does nvprintfLevel(LOGLEVEL_OK)
  - LOGSTATS : macro that does nvprintfLevel(LOGLEVEL_STATS)
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
#ifdef _DEBUG
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

typedef void (*PFN_NVPRINTCALLBACK)(int level, const char* fmt);

void nvprintf(
#ifdef _MSC_VER
    _Printf_format_string_
#endif
    const char* fmt,
    ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 1, 2)));
#endif
;

void nvprintfLevel(int level,
#ifdef _MSC_VER
                   _Printf_format_string_
#endif
                   const char* fmt,
                   ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 2, 3)));
#endif
;

// Set/get the level for calls to nvprintf(). Use LOGLEVEL_*.
void nvprintSetLevel(int l);
int  nvprintGetLevel();

void nvprintSetLogFileName(const char* name);

// Globally enable/disable all nvprint output and logging
void nvprintSetLogging(bool b);

// Update level bitmasks file and stderr output. 'state' controls whether to
// enable or disable the 'mask' bits. Use LOGBITS_*.
void nvprintSetFileLogging(bool state, uint32_t mask = ~0);
void nvprintSetConsoleLogging(bool state, uint32_t mask = ~0);

// Set a custom print handler. Called in addition to file and console logging.
void nvprintSetCallback(PFN_NVPRINTCALLBACK callback);


#endif
