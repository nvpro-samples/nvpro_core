/*
 * Copyright (c) 2014-2025, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iomanip>
#include <sstream>

#include "commandlineparser.hpp"
#include "nvprint.hpp"
#include "fileoperations.hpp"

static constexpr int MAX_LINE_WIDTH = 60;

nvh::CommandLineParser::CommandLineParser(const std::string& description)
    : m_description(description)
{
  // Add default -h / --help argument
  addArgument({"-h", "--help"}, 0, "Print this help message and exit.", [this](std::vector<std::string> const&) {
    printHelp();
    std::exit(0);  // Exit the program after printing help
  });
}

void nvh::CommandLineParser::addArgument(std::vector<std::string> const& flags, Value const& value, std::string const& help, Callback callback)
{
  m_arguments.emplace_back(Argument{flags, value, help, {}, callback});
}

void nvh::CommandLineParser::addArgument(std::vector<std::string> const& flags, int numArgsToAdvance, std::string const& help, CallbackArgs callbackArgs)
{
  m_arguments.emplace_back(Argument{flags, {}, help, callbackArgs, {}, numArgsToAdvance});
}

void nvh::CommandLineParser::addFilename(const std::string& extension, std::string* filename, const std::string& help)
{
  m_arguments.emplace_back(Argument{{}, filename, help, nullptr, nullptr, 0, extension, true});
}

bool nvh::CommandLineParser::parse(int argc, char* argv[])
{
  bool result = true;

  // Create a map of flags to argument indices
  for(size_t i = 0; i < m_arguments.size(); ++i)
  {
    for(size_t j = 0; j < m_arguments[i].flags.size(); j++)
    {
      const std::string& flag = m_arguments[i].flags[j];
      m_flagMap[flag]         = i;
    }
  }

  int argIndex = 1;  // Start after program name
  while(argIndex < argc)
  {
    std::string flag(argv[argIndex]);

    // Lookup the flag in the map
    auto it = m_flagMap.find(flag);
    if(it != m_flagMap.end())
    {
      Argument& argument = m_arguments[it->second];

      if(argument.callbackArgs)
      {
        executeCallback(argument, argv, argIndex, argc);
        ++argIndex;  // Move to the next flag
        continue;
      }

      try
      {
        // Applies a visitor (callable) to the active variant (value) in a type-safe way, handling all possible types the variant might hold
        int argsProcessed =
            std::visit([&](auto&& arg) { return parseVariantValue(arg, argv, argIndex, argc); }, argument.value);
        if(argument.callback)
        {
          argument.callback();
        }
        argIndex += argsProcessed;  // Advance by the number of arguments processed
      }
      catch(const std::exception& e)
      {
        LOGE("Error parsing value for argument: %s\n", e.what());
        return false;  // Error
      }
    }
    // Handle filenames
    else
    {
      bool handled = false;
      for(auto& argument : m_arguments)
      {
        if(argument.isFilename && nvh::endsWith(argv[argIndex], argument.filenameExt))
        {
          // std::holds_alternative checks if a std::variant holds a specific type. Here, it checks if 'argument.value' holds a std::string*.
          if(std::holds_alternative<std::string*>(argument.value))
          {
            *std::get<std::string*>(argument.value) = argv[argIndex];
            handled                                 = true;
            break;
          }
        }
      }
      if(!handled && m_verbose)
      {
        LOGW("Ignoring unknown command line argument: %s \n", flag.c_str());
        result = false;
      }
    }

    ++argIndex;  // Move to the next flag
  }

  return result;
}

template <typename T>
int nvh::CommandLineParser::parseVariantValue(T* arg, char* argv[], int i, int argc)
{
  if constexpr(std::is_same_v<T, bool>)
  {
    // Handle bool: Check for "true"/"false" or default to true
    if(argc > i + 1 && (std::string(argv[i + 1]) == "true" || std::string(argv[i + 1]) == "false"))
    {
      *arg = (std::string(argv[i + 1]) == "true");
      return 1;  // One argument processed
    }
    *arg = true;  // Default to true if no explicit value
    return 0;     // No additional arguments processed
  }
  else if constexpr(std::is_same_v<T, std::string>)
  {
    // Handle std::string: Take the next argument
    if(i + 1 >= argc)
    {
      throw std::runtime_error("Missing value for std::string argument");
    }
    *arg = argv[i + 1];
    return 1;  // One argument processed
  }
  else if constexpr(std::is_same_v<T, glm::ivec2> || std::is_same_v<T, glm::uvec2>)
  {
    // Handle glm::ivec2/uvec2: Parse two values
    if(i + 2 >= argc)
    {
      throw std::runtime_error("Missing second value for glm::ivec2/uvec2");
    }
    int x, y;
    std::stringstream(argv[i + 1]) >> x;
    std::stringstream(argv[i + 2]) >> y;
    *arg = T(x, y);
    return 2;  // Two arguments processed
  }
  else
  {
    // Handle integral types via std::stringstream i.e. float, int, double, uint32_t, etc..
    if(i + 1 >= argc)
    {
      throw std::runtime_error("Missing value for argument");
    }
    if constexpr(std::is_integral_v<T> && sizeof(T) < sizeof(int))
    {
      // Handle small integer types through an int (i.e. int8_t, int16_t, uint8_t, uint16_t)
      int temp;
      std::stringstream(argv[i + 1]) >> temp;
      *arg = static_cast<T>(temp);
    }
    else
    {
      // Handle all other types directly
      std::stringstream(argv[i + 1]) >> *arg;
    }
    return 1;  // One argument processed
  }
}

void nvh::CommandLineParser::executeCallback(Argument& argument, char* argv[], int& i, int argc)
{
  std::vector<std::string> args;
  for(int j = 1; j <= argument.numArgsToAdvance && i + j < argc; ++j)
  {
    args.emplace_back(argv[i + j]);
  }
  argument.callbackArgs(args);
  i += argument.numArgsToAdvance;  // Advance index
}

void nvh::CommandLineParser::printHelp() const
{
  // Print the general description.
  LOGI("%s\n", m_description.c_str());

  // Find the argument with the longest combined flag length (in order to align the help messages).
  uint32_t maxFlagLength = 0;
  for(size_t i = 0; i < m_arguments.size(); ++i)
  {
    uint32_t flagLength = 0;
    for(size_t j = 0; j < m_arguments[i].flags.size(); ++j)
    {
      flagLength += static_cast<uint32_t>(m_arguments[i].flags[j].size()) + 2;  // +2 for ", "
    }
    maxFlagLength = std::max(maxFlagLength, flagLength);
  }


  // Now print each argument.
  for(size_t i = 0; i < m_arguments.size(); ++i)
  {
    const Argument& argument = m_arguments[i];

    std::string flags;
    if(argument.isFilename)
    {
      flags = "[" + argument.filenameExt + "]  ";
    }
    else
    {
      for(auto const& flag : argument.flags)
      {
        flags += flag + ", ";
      }
    }

    // Remove last comma and space and add padding according to the longest flags in order to align the help messages.
    std::stringstream sstr;
    sstr << std::left << std::setw(maxFlagLength) << flags.substr(0, flags.size() - 2);

    // Print the help for each argument. This is a bit more involved since we do line wrapping for long descriptions.
    size_t spacePos  = 0;
    size_t lineWidth = 0;
    while(spacePos != std::string::npos)
    {
      size_t nextspacePos = argument.help.find_first_of(' ', spacePos + 1);
      sstr << argument.help.substr(spacePos, nextspacePos - spacePos);
      lineWidth += nextspacePos - spacePos;
      spacePos = nextspacePos;

      if(lineWidth > MAX_LINE_WIDTH)
      {
        LOGI("%s\n", sstr.str().c_str());
        sstr = std::stringstream();
        sstr << std::left << std::setw(maxFlagLength - 1) << " ";
        lineWidth = 0;
      }
    }
  }
}