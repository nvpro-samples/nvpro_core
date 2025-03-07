/*
 * Copyright (c) 2025, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stacktrace.hpp"

#if defined(_WIN32)  // Windows
#include <Windows.h>
#include <DbgHelp.h>  // For symbols

#include <array>
#include <cassert>
#include <limits>
#include <sstream>
#include <stdint.h>
#include <tuple>  // For std::ignore
#include <type_traits>

namespace nvh {
std::string getStacktrace(unsigned int numFramesToSkip) noexcept
{
  const HANDLE process = GetCurrentProcess();
  // Initialize the handler so we can collect symbol information.
  // If symbol initialization fails, it's not an error because
  // we can continue on without symbols.
  std::ignore = SymInitialize(process, nullptr, TRUE);

  // Windows allows up to 0xffff frames here; 1024 should be enough.
  std::array<void*, 1024> stackframes;
  // Skip 1 frame here for `getStacktrace()` itself.
  const USHORT numCapturedFrames = CaptureStackBackTrace(numFramesToSkip + 1,                     // FramesToSkip
                                                         static_cast<DWORD>(stackframes.size()),  // FramesToCapture
                                                         stackframes.data(),                      // Backtrace
                                                         nullptr);                                // Backtrace hash

  // Allocate a SYMBOL_INFO buffer. This starts with a SYMBOL_INFO header
  // (which includes the first character of the function name at the end),
  // followed by the rest of the function name.
  constexpr ULONG k_maxNameLength = 1024;
  // Windows uses TCHARs, but if these are different, later code will
  // probably break.
  static_assert(std::is_same_v<char, TCHAR>);
  std::array<char, sizeof(SYMBOL_INFO) + (k_maxNameLength - 1)> symbolBuffer{};
  SYMBOL_INFO* pSymbol  = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer.data());
  pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
  pSymbol->MaxNameLen   = k_maxNameLength;

  try
  {
    std::stringstream result;
    for(USHORT frame = 0; frame < numCapturedFrames; frame++)
    {
      // Try to get the symbol and line at this address.
      const uintptr_t memoryAddress = reinterpret_cast<uintptr_t>(stackframes[frame]);
      DWORD64         displacement64{};
      IMAGEHLP_LINE64 line{.SizeOfStruct = sizeof(IMAGEHLP_LINE64)};
      BOOL            lineInfoOk = SymFromAddr(process, memoryAddress, &displacement64, pSymbol);
      if(lineInfoOk)
      {
        // This also writes the displacement, but of a different bit length.
        DWORD displacement32{};
        lineInfoOk = SymGetLineFromAddr64(process, memoryAddress, &displacement32, &line);
      }

      // Try to determine which module this symbol's contained within.
      // If the module base is 0, this won't give us much information.
      std::array<char, 256> moduleName;
      BOOL                  moduleOk = (pSymbol->ModBase != 0);
      if(moduleOk)
      {
        moduleOk = GetModuleFileName(reinterpret_cast<HMODULE>(pSymbol->ModBase), moduleName.data(),
                                     static_cast<DWORD>(moduleName.size()));
      }

      // Format the data we have.
      result << frame << ": " << pSymbol->Name;
      if(lineInfoOk)
      {
        result << " in " << line.FileName << ", line " << line.LineNumber;
      }
      result << ", address 0x" << std::hex << pSymbol->Address;
      if(moduleOk)
      {
        result << " (" << moduleName.data() << " + 0x" << pSymbol->Address - pSymbol->ModBase << ")";
      }
      result << std::dec;
      result << "\n";
      // We also have access to symbol flags, but we're not currently
      // printing them -- that could be useful if in the future we need to
      // know if a function is a thunk, for instance.
    }

    return result.str();
  }
  catch(const std::exception& /* unused */)
  {
    assert(!"Stacktrace failed; likely out of memory.");
    return "";
  }
}
}  // namespace nvh

#elif __has_include(<execinfo.h>)  // Linux libcs that provide `backtrace()`

#include <array>
#include <cassert>
#include <execinfo.h>
#include <memory>
#include <sstream>
#include <stdint.h>

namespace nvh {
std::string getStacktrace(unsigned int numFramesToSkip) noexcept
{
  std::array<void*, 1024> stackframes;
  const int               numFrames = backtrace(stackframes.data(), static_cast<int>(stackframes.size()));
  if(numFrames < 0 || numFrames > stackframes.size())
  {
    assert(!"Stacktrace failed; number of frames out of bounds");
    return "";
  }

  // Load symbols; wrap the result in a unique_ptr so it'll certainly be freed.
  char** strings = backtrace_symbols(stackframes.data(), numFrames);
  // This makes sure `strings` is freed even if control flow is abnormal
  std::unique_ptr<char*> pStrings(strings);

  try
  {
    std::stringstream result;

    // Add 1 to skip `getStacktrace()` itself
    unsigned int trueFramesToSkip = numFramesToSkip + 1;
    for(unsigned int frame = trueFramesToSkip; frame < static_cast<unsigned int>(numFrames); frame++)
    {
      result << (frame - trueFramesToSkip) << ": ";
      if(strings)
      {
        result << strings[frame];
      }
      else
      {
        result << "address 0x" << std::hex << reinterpret_cast<uintptr_t>(stackframes[frame]) << std::dec;
      }
      result << "\n";
    }

    return result.str();
  }
  catch(const std::exception& /* unused */)
  {
    assert(!"Stacktrace failed; likely out of memory.");
    return "";
  }
}

}  // namespace nvh

#else  // No backend available

namespace nvh {

std::string getStacktrace(unsigned int /* numFramesToSkip */) noexcept
{
  try
  {
    return "<stacktrace not supported on this system>";
  }
  catch(const std::exception& /* unused */)
  {
    assert(!"Stacktrace failed attempting to allocate a short string; something is very wrong.");
    return "";
  }
}

}  // namespace nvh

#endif
