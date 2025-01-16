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

#pragma once

#include <glm/glm.hpp>
#include <variant>
#include <functional>
#include <unordered_map>
#include <string>
#include <vector>


namespace nvh {
/* @DOC_START
Command line parser.
```cpp
    std::string            inFilename{};
    bool                   printHelp = false;
    glm::ivec2             winSize   = {1280, 720};
    int8_t                 color[3];
    nvh::CommandLineParser cli("Test Parser");
    cli.addArgument({"-f", "--filename"}, &inFilename, "Input filename");
    cli.addArgument({"--winSize"}, &winSize, "Size of window",
                    [&]() { glfwSetWindowSize(nullptr, winSize[0], winSize[1]); });
    cli.addArgument({"--callback"}, 1, "Callback with one argument",
                    [&inFilename](std::vector<std::string> const& args) { inFilename = args[0]; });
    cli.addArgument({"--color"}, 3, "Clear color ", [&color](std::vector<std::string> const& args) {
      std::stringstream(args[0]) >> color[0];
      std::stringstream(args[1]) >> color[1];
      std::stringstream(args[2]) >> color[2];
    });
    cli.addFilename(".gltf", &inFilename, "Input filename with extension");
    bool result = cli.parse(argc, argv);
```
@DOC_END */
class CommandLineParser
{
public:
  using CallbackArgs = std::function<void(std::vector<std::string> const&)>;
  using Callback     = std::function<void(void)>;

  // These are the possible variables the options may point to. Bool and
  // std::string are handled in a special way, all other values are parsed
  // with a std::stringstream. This std::variant can be easily extended if
  // the stream operator>> is overloaded. If not, you have to add a special
  // case to the parse() method.
  using Value =
      std::variant<int8_t*, uint8_t*, int32_t*, uint32_t*, double*, float*, bool*, std::string*, glm::ivec2*, glm::uvec2*>;

  // The description is printed as part of the help message.
  CommandLineParser(const std::string& description);

  void addArgument(std::vector<std::string> const& flags, Value const& value, std::string const& help = "", Callback callback = nullptr);
  void addArgument(std::vector<std::string> const& flags, int numArgsToAdvance, std::string const& help, CallbackArgs callback);
  void addFilename(const std::string& extension, std::string* filename, const std::string& help);

  // Prints the description given to the constructor and the help for each option.
  void printHelp() const;

  void setVerbose(bool verbose) { m_verbose = verbose; }

  // The command line arguments are traversed from start to end. That means,
  // if an option is set multiple times, the last will be the one which is
  // finally used. This call will throw a std::runtime_error if a value is
  // missing for a given option. Unknown flags will cause a warning on
  // std::cerr.
  bool parse(int argc, char* argv[]);


private:
  struct Argument
  {
    std::vector<std::string> flags{};
    Value                    value{};
    std::string              help{};
    CallbackArgs             callbackArgs     = nullptr;
    Callback                 callback         = nullptr;
    int                      numArgsToAdvance = 0;
    std::string              filenameExt{};       // Extension for filename arguments (e.g., ".gltf")
    bool                     isFilename = false;  // True if this is a filename handler
  };

  template <typename T>
  int  parseVariantValue(T* arg, char* argv[], int i, int argc);
  void executeCallback(Argument& argument, char* argv[], int& i, int argc);

  std::string                             m_description;
  std::vector<Argument>                   m_arguments;
  std::unordered_map<std::string, size_t> m_flagMap;
  bool                                    m_verbose = false;
};

}  // namespace nvh
