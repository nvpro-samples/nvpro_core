#
#  Copyright (c) 2018-2021, NVIDIA CORPORATION.  All rights reserved.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
#  SPDX-FileCopyrightText: Copyright (c) 2018-2021 NVIDIA CORPORATION
#  SPDX-License-Identifier: Apache-2.0
#

#
# The following script originate from Volk (https://github.com/zeux/volk) and adapted to the need
# of nvpro-core samples.
#


# Copyright (c) 2018-2023 Arseny Kapoulkine
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:


#!/usr/bin/python3
# This will generate the entry point for all Vulkan extensions
# Code blocks are created and will be replace between
# 'NVVK_GENERATE_'<BLOCK_NAME>
import argparse
import os.path
import urllib
import urllib.request
import xml.etree.ElementTree as etree
import re
from collections import OrderedDict

# Ignoring those extensions because they are part of vulkan-1.lib
ExcludeList = [
    "defined(VK_KHR_surface)",
    "defined(VK_KHR_win32_surface)",
    "defined(VK_KHR_xlib_surface)",
    "defined(VK_KHR_wayland_surface)",
    "defined(VK_KHR_xcb_surface)",
    "defined(VK_KHR_display)",
    "defined(VK_KHR_swapchain)",
    "defined(VK_KHR_get_surface_capabilities2)",
    "defined(VK_KHR_get_display_properties2)",
    "defined(VK_KHR_display_swapchain)",
    "VK_VERSION_1_0",
    "VK_VERSION_1_1",
    "VK_VERSION_1_2",
    "VK_VERSION_1_3",
]

# Debugging - To be sure that the exclude list is excluding commands
# exported in vulkan-1, populate the list here. If there is a duplicate
# the name of the command and the extension name will be printed out.
ExportedCommands = []  # dumpbin /EXPORTS vulkan-1.lib


# Commands that were added in newer extension revisions.
# Extensions such as VK_EXT_discard_rectangles have had specification revisions
# that added new commands. Since these commands should only be used if the
# extension's `VkExtensionProperties::specVersion` is high enough, this table
# tracks the first `specVersion` in which each newer command was introduced
# (as this information is not currently contained in vk.xml).
cmdversions = {
    "vkCmdSetDiscardRectangleEnableEXT": 2,
    "vkCmdSetDiscardRectangleModeEXT": 2,
    "vkCmdSetExclusiveScissorEnableNV": 2,
}


def parse_xml(path):
    # Parsing the Vulkan 'vk.xml' document
    file = urllib.request.urlopen(path) if path.startswith("http") else open(path, "r")
    with file:
        tree = etree.parse(file)
        return tree


def patch_file(fileName, blocks):
    # Find each section of NVVK_GENERATE_ and replace with block of text
    result = []
    block = None

    scriptDir = os.path.dirname(os.path.realpath(__file__))
    path = os.path.join(scriptDir, fileName)

    with open(path, "r") as file:
        for line in file.readlines():
            if block:
                if line == block:
                    result.append(line)
                    block = None
            else:
                result.append(line)
                # C comment marker
                if line.strip().startswith("/* NVVK_GENERATE_"):
                    block = line
                    result.append(blocks[line.strip()[17:-3]])

    with open(path, "w", newline="\n") as file:
        for line in result:
            file.write(line)


def is_descendant_type(types, name, base):
    # Finding the base type of each type:
    # <type category="handle" parent="VkPhysicalDevice"
    # objtypeenum="VK_OBJECT_TYPE_DEVICE"><type>VK_DEFINE_HANDLE</type>(<name>VkDevice</name>)</type>
    # <type category="handle" parent="VkDevice"
    # objtypeenum="VK_OBJECT_TYPE_QUEUE"><type>VK_DEFINE_HANDLE</type>(<name>VkQueue</name>)</type>
    if name == base:
        return True
    type = types.get(name)
    if not type:
        return False
    parents = type.get("parent")
    if not parents:
        return False
    return any(
        [is_descendant_type(types, parent, base) for parent in parents.split(",")]
    )


def defined(key):
    return "defined(" + key + ")"


def cdepends(key):
    return (
        re.sub(r"[a-zA-Z0-9_]+", lambda m: defined(m.group(0)), key)
        .replace(",", " || ")
        .replace("+", " && ")
    )


# Remove "defined(..)"
def remove_defined(input_string):
    return re.sub(r"defined\((.*?)\)", r"\1", input_string)

def toStr(txt):
    # Return the string if it exist or '' if None
    if txt:
        return txt
    return ""


def get_function(rtype, name, params):
    # Returning the function declaration

    fct_args = []  # incoming argument
    call_args = []  # call arguments
    for p in params:
        ptype = p.find("type")
        pname = p.find("name")
        papi = p.attrib.get("api")
        # Avoid `vulkansc`
        if not papi or papi == "vulkan":
            fct_args.append(
                "".join(
                    [
                        toStr(p.text),
                        ptype.text,
                        ptype.tail,
                        pname.text,
                        toStr(pname.tail),
                    ]
                )
            )  # 'const', 'vkSome', '*', 'some', '[2]'
            call_args.append(pname.text)

    # Function signature
    fct = "VKAPI_ATTR " + rtype + " VKAPI_CALL " + name + "(\n"
    # Arguments of the function
    fct += "\t" + ", \n\t".join(fct_args) + ") \n"
    fct += "{ \n  "
    # fct += ' assert(pfn_'+name+');\n'
    # Check if the function is returning a value
    if rtype != "void":
        fct += "return "
    fct += "pfn_" + name + "(" + ", ".join(call_args) + "); \n"
    fct += "}\n"
    return fct


def get_vk_xml_path(spec_arg):
    """
    Find the Vulkan specification XML file by looking for (highest priority to
    lowest) an incoming `spec` argument, a local copy within the Vulkan SDK,
    or by downloading it from KhronosGroup/Vulkan-Docs.
    """
    if spec_arg is not None:
        return spec_arg

    # VULKAN_SDK is a newer version of VK_SDK_PATH. The Linux Tarball Vulkan SDK
    # instructions only say to set VULKAN_SDK - so VULKAN_SDK might exist while
    # VK_SDK_PATH might not.
    vulkan_sdk_env = os.getenv("VULKAN_SDK")
    if vulkan_sdk_env is not None:
        local_spec_path = os.path.normpath(
            vulkan_sdk_env + "/share/vulkan/registry/vk.xml"
        )
        if os.path.isfile(local_spec_path):
            return local_spec_path

    # Ubuntu installations might not have VULKAN_SDK set, but have vk.xml in /usr.
    if os.path.isfile("/usr/share/vulkan/registry/vk.xml"):
        return "/usr/share/vulkan/registry/vk.xml"

    print(
        "Warning: no `spec` parameter was provided, and vk.xml could not be "
        "found in the path given by the VULKAN_SDK environment variable or in "
        "system folders. This script will download the latest copy of vk.xml "
        "online, which may be incompatible with an installed Vulkan installation."
    )
    return "https://raw.githubusercontent.com/KhronosGroup/Vulkan-Docs/main/xml/vk.xml"


#
# MAIN Entry
#
if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generates entry points for Vulkan extensions in extensions_vk.cpp."
    )
    parser.add_argument(
        "--beta",
        action="store_true",
        help="Includes provisional Vulkan extensions; these extensions are not guaranteed to be consistent across Vulkan SDK versions.",
    )
    parser.add_argument(
        "spec",
        type=str,
        nargs="?",
        help="Optional path to a vk.xml file to use to generate extensions. Otherwise, uses the vk.xml in the Vulkan SDK distribution specified in the VULKAN_SDK environment variable.",
    )

    args = parser.parse_args()

    # Retrieving the XML file
    specpath = get_vk_xml_path(args.spec)
    spec = parse_xml(specpath)

    # CODE BLOCS
    blocks = {}

    # CODE BLOCS for generated code
    block_keys = ("STATIC_PFN", "LOAD_PROC", "DECLARE", "DEFINE")

    for key in block_keys:
        blocks[key] = ""

    # Retrieving the version of the Vulkan specification
    version = spec.find('types/type[name="VK_HEADER_VERSION"]')
    blocks["VERSION_INFO"] = (
        "// Generated using Vulkan " + version.find("name").tail.strip() + "\n"
    )

    # Patching the files
    patch_file("extensions_vk.cpp", blocks)

    # Ordered list of commands per extension group
    command_groups = OrderedDict()
    instance_commands = set()

    for feature in spec.findall("feature"):
        api = feature.get("api")
        if "vulkan" not in api.split(","):
            continue
        key = feature.get("name")
        cmdrefs = feature.findall("require/command")
        command_groups[key] = [cmdref.get("name") for cmdref in cmdrefs]

    # Retrieve all extension and sorted alphabetically
    for ext in sorted(
        spec.findall("extensions/extension"), key=lambda ext: ext.get("name")
    ):
        # Only add the extension if 'vulkan' is part of the support attribute
        supported = ext.get("supported")
        if "vulkan" not in supported.split(","):
            continue

        # Discard beta extensions
        if ext.get("provisional") == "true" and not args.beta:
            continue

        name = ext.get("name")
        type = ext.get("type")  # device or instance

        for req in ext.findall("require"):
            # Adding all commands for this extension
            key = defined(name)
            if req.get("feature"):  # old-style XML depends specification
                for i in req.get("feature").split(","):
                    key += " && " + defined(i)
            if req.get("extension"):  # old-style XML depends specification
                for i in req.get("extension").split(","):
                    key += " && " + defined(i)
            if req.get("depends"):  # new-style XML depends specification
                dep = cdepends(req.get("depends"))
                key += " && " + ("(" + dep + ")" if "||" in dep else dep)
            cmdrefs = req.findall("command")

            # Add ifdef section and split commands with high version
            for cmdref in cmdrefs:
                ver = cmdversions.get(cmdref.get("name"))
                if ver:
                    command_groups.setdefault(
                        key + " && " + name.upper() + "_SPEC_VERSION >= " + str(ver), []
                    ).append(cmdref.get("name"))
                else:
                    command_groups.setdefault(key, []).append(cmdref.get("name"))

            # Adding commands that are 'instance' instead of 'device'
            if type == "instance":
                for cmdref in cmdrefs:
                    instance_commands.add(cmdref.get("name"))

    # From a command, find which group it's belong
    commands_to_groups = OrderedDict()
    for group, cmdnames in command_groups.items():
        for name in cmdnames:
            commands_to_groups.setdefault(name, []).append(group)

    for group, cmdnames in command_groups.items():
        command_groups[group] = [
            name for name in cmdnames if len(commands_to_groups[name]) == 1
        ]

    for name, groups in commands_to_groups.items():
        if len(groups) == 1:
            continue
        key = " || ".join([g for g in groups])
        command_groups.setdefault(key, []).append(name)

    # Finding the alias name for a function: <command
    # name="vkGetPhysicalDeviceExternalBufferPropertiesKHR"
    # alias="vkGetPhysicalDeviceExternalBufferProperties"/>
    commands = {}
    for cmd in spec.findall("commands/command"):
        if not cmd.get("alias"):
            name = cmd.findtext("proto/name")
            commands[name] = cmd

    for cmd in spec.findall("commands/command"):
        if cmd.get("alias"):
            name = cmd.get("name")
            commands[name] = commands[cmd.get("alias")]

    # Finding all Vulkan types to be use by is_descendant_type
    types = {}
    for type in spec.findall("types/type"):
        name = type.findtext("name")
        if name:
            types[name] = type

    for key in block_keys:
        blocks[key] = ""

    # For each group, get the list of all commands
    for group, cmdnames in command_groups.items():
        # Skipping some extensions
        if group in ExcludeList:
            continue

        ifdef = "#if " + group + "\n"

        for key in block_keys:
            blocks[key] += ifdef

        # Name the NVVK_HAS with the first part of the group
        ext_name = group
        if "&&" in group:
            ext_name = group.split("&&")[0].strip()
        elif "||" in group:
            ext_name = group.split("||")[0].strip()
        if ext_name != None:
            blocks["DEFINE"] += "#define NVVK_HAS_" + remove_defined(ext_name) + "\n"

        # Getting all commands within the group
        for name in sorted(cmdnames):
            # Finding the 'alias' command
            cmd = commands[name]

            if name in ExportedCommands:
                print("Command " + name + " from group " + group)

            # Get the first argument type, which defines if it is an instance
            # function
            type = cmd.findtext("param[1]/type")

            # Create the function declaration block
            params = cmd.findall("param")
            return_type = cmd.findtext("proto/type")
            blocks["DECLARE"] += get_function(return_type, name, params)

            # Loading proc address can be device or instance
            if (
                is_descendant_type(types, type, "VkDevice")
                and name not in instance_commands
            ):
                blocks["LOAD_PROC"] += (
                    "  pfn_"
                    + name
                    + " = (PFN_"
                    + name
                    + ')getDeviceProcAddr(device, "'
                    + name
                    + '");\n'
                )
            elif is_descendant_type(types, type, "VkInstance"):
                blocks["LOAD_PROC"] += (
                    "  pfn_"
                    + name
                    + " = (PFN_"
                    + name
                    + ')getInstanceProcAddr(instance, "'
                    + name
                    + '");\n'
                )

            # Creates the bloc for all static functions
            blocks["STATIC_PFN"] += "static PFN_" + name + " pfn_" + name + "= 0;\n"

        # Adding the #endif or removing empty blocks
        for key in block_keys:
            if blocks[key].endswith(ifdef):
                blocks[key] = blocks[key][: -len(ifdef)]
            else:
                blocks[key] += "#endif /* " + remove_defined(group) + " */\n"

    # Patching the files
    patch_file("extensions_vk.hpp", blocks)
    patch_file("extensions_vk.cpp", blocks)
