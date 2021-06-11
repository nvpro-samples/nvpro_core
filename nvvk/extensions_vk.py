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

#!/usr/bin/python3
# This will generate the entry point for all Vulkan extensions
# Code blocks are created and will be replace between
# 'NVVK_GENERATE_'<BLOCK_NAME>
import os.path
import sys
import urllib
import urllib.request
import xml.etree.ElementTree as etree
from collections import OrderedDict

# Ignoring those extensions because they are part of vulkan-1.lib
ExcludeList = ['VK_KHR_surface', 
             'VK_KHR_win32_surface', 
             'VK_KHR_xlib_surface', 
             'VK_KHR_wayland_surface',
             'VK_KHR_xcb_surface',
             'VK_KHR_display', 
             'VK_KHR_swapchain', 
             'VK_KHR_get_surface_capabilities2', 
             'VK_KHR_get_display_properties2', 
             'VK_KHR_display_swapchain']

# Debuggin - To be sure that the black list is excluding commands 
# exported in vulkan-1, populate the list here. If there is a duplicate
# the name of the command and the extension name will be printed out.
ExportedCommands = [ # dumpbin /EXPORTS vulkan-1.lib
    ]



def parse_xml(path):
    # Parsing the Vulkan 'vk.xml' document
    file = urllib.request.urlopen(path) if path.startswith("http") else open(path, 'r')
    with file:
        tree = etree.parse(file)
        return tree


def patch_file(fileName, blocks):
    # Find each section of NVVK_GENERATE_ and replace with block of text
    result = []
    block = None

    scriptDir = os.path.dirname(os.path.realpath(__file__))
    path = os.path.join(scriptDir, fileName)

    with open(path, 'r') as file:
        for line in file.readlines():
            if block:
                if line == block:
                    result.append(line)
                    block = None
            else:
                result.append(line)
                # C comment marker
                if line.strip().startswith('/* NVVK_GENERATE_'):
                    block = line
                    result.append(blocks[line.strip()[17:-3]])

    with open(path, 'w', newline='\n') as file:
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
    parents = type.get('parent')
    if not parents:
        return False
    return any([is_descendant_type(types, parent, base) for parent in parents.split(',')])


def toStr(txt):
    # Return the string if it exist or '' if None
    if txt:
        return txt
    return ''


def get_function(rtype, name, params):
    # Returning the function declaration

    fct_args = []  # incoming argument
    call_args = []  # call arguments
    for p in params:
        ptype = p.find('type')
        pname = p.find('name')
        fct_args.append("".join([toStr(p.text), ptype.text,
                                 ptype.tail, pname.text, toStr(pname.tail)]))  # 'const', 'vkSome', '*', 'some', '[2]'
        call_args.append(pname.text)

    # Function signature
    fct = 'VKAPI_ATTR ' + rtype + ' VKAPI_CALL ' + name + '(\n'
    # Arguments of the function
    fct += '\t' + ', \n\t'.join(fct_args) + ') \n'
    fct += '{ \n  '
    # fct += ' assert(pfn_'+name+');\n'
    # Check if the function is returning a value
    if rtype != 'void':
        fct += 'return '
    fct += 'pfn_' + name + '(' + ', '.join(call_args) + '); \n'
    fct += '}\n'
    return fct


#
# MAIN Entry
#
if __name__ == "__main__":
    # Using incoming argument, local XML or web version
    specpath = "https://raw.githubusercontent.com/KhronosGroup/Vulkan-Docs/main/xml/vk.xml"
    loc_spec_path = os.path.normpath(os.getenv('VK_SDK_PATH') + '/share/vulkan/registry/vk.xml')

    if len(sys.argv) > 1:
        specpath = sys.argv[1]
    elif os.path.isfile(loc_spec_path):
        specpath = loc_spec_path

    # Retriving the XML file
    spec = parse_xml(specpath)

    # CODE BLOCS
    blocks = {}

    # CODE BLOCS for generated code
    block_keys = ('STATIC_PFN', 'LOAD_PROC',  'DECLARE')

    for key in block_keys:
        blocks[key] = ''

    # Retrieving the version of the Vulkan specification
    version = spec.find('types/type[name="VK_HEADER_VERSION"]')
    blocks['VERSION_INFO'] = '// Generated using Vulkan ' + version.find('name').tail.strip() + '\n'

    # Patching the files
    patch_file('extensions_vk.cpp', blocks)

    # Ordered list of commands per extension group
    command_groups = OrderedDict()
    instance_commands = set()

    # Retrieve all extension and sorted alphabetically
    for ext in sorted(spec.findall('extensions/extension'), key=lambda ext: ext.get('name')):
        # Discard disabled extensions
        if ext.get('supported') == 'disabled':
            continue

        name = ext.get('name')
        type = ext.get('type')  # device or instance

        for req in ext.findall('require'):
            # Adding all commands for this extension
            key = name
            cmdrefs = req.findall('command')
            command_groups.setdefault(key, []).extend([cmdref.get('name') for cmdref in cmdrefs])
            # Adding commands that are 'instance' instead of 'device'
            if type == 'instance':
                for cmdref in cmdrefs:
                    instance_commands.add(cmdref.get('name'))

    # From a command, find which group it's belong
    commands_to_groups = OrderedDict()
    for (group, cmdnames) in command_groups.items():
        for name in cmdnames:
            commands_to_groups.setdefault(name, []).append(group)

    for (group, cmdnames) in command_groups.items():
        command_groups[group] = [
            name for name in cmdnames if len(commands_to_groups[name]) == 1]

    # Finding the alias name for a function: <command
    # name="vkGetPhysicalDeviceExternalBufferPropertiesKHR"
    # alias="vkGetPhysicalDeviceExternalBufferProperties"/>
    commands = {}
    for cmd in spec.findall('commands/command'):
        if not cmd.get('alias'):
            name = cmd.findtext('proto/name')
            commands[name] = cmd

    for cmd in spec.findall('commands/command'):
        if cmd.get('alias'):
            name = cmd.get('name')
            commands[name] = commands[cmd.get('alias')]

    # Finding all Vulkan types to be use by is_descendant_type
    types = {}
    for type in spec.findall('types/type'):
        name = type.findtext('name')
        if name:
            types[name] = type

    # For each group, get the list of all commands
    for (group, cmdnames) in command_groups.items():
        # Skiping some extensions
        if group in ExcludeList:
            continue

        # Each code groups will be surrounded by #ifdef/#endif
        ifdef = '#ifdef ' + group + '\n'
        for key in block_keys:
            blocks[key] += ifdef

        # Getting all commands within the group
        for name in sorted(cmdnames):
            # Finding the 'alias' command
            cmd = commands[name]

            if name in ExportedCommands:
                print("Command " + name + " from group " + group)



            # Get the first argument type, which defines if it is an instance
            # function
            type = cmd.findtext('param[1]/type')

            # Create the function declaration block
            params = cmd.findall('param')
            return_type = cmd.findtext('proto/type')
            blocks['DECLARE'] += get_function(return_type, name, params)

            # Loading proc address can be device or instance
            if is_descendant_type(types, type, 'VkDevice') and name not in instance_commands:
                blocks['LOAD_PROC'] += '  pfn_' + name + ' = (PFN_' + name + ')getDeviceProcAddr(device, "' + name + '");\n'
            elif is_descendant_type(types, type, 'VkInstance'):
                blocks['LOAD_PROC'] += '  pfn_' + name + ' = (PFN_' + name + ')getInstanceProcAddr(instance, "' + name + '");\n'

            # Creates the bloc for all static functions
            blocks['STATIC_PFN'] += 'static PFN_' + name + ' pfn_' + name + '= 0;\n'

        # Adding the #endif or removing empty blocks
        for key in block_keys:
            if blocks[key].endswith(ifdef):
                blocks[key] = blocks[key][:-len(ifdef)]
            else:
                blocks[key] += '#endif /* ' + group + ' */\n'

    # Patching the files
    patch_file('extensions_vk.hpp', blocks)
    patch_file('extensions_vk.cpp', blocks)
