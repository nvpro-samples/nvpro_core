# -------------------------------------------------------------------------------------------------------------
#
# -------------------------------------------------------------------------------------------------------------

import platform
import os
import argparse
import logging
import time
from enum import Enum
import re
import time

verbose = False

# https://docs.python.org/3/library/enum.html
class Category(Enum):
    NONE = 0
    ARTICLE = 1
    BG = 2
    BP = 3
    SB = 4
currentCategory = Category.NONE

parser = argparse.ArgumentParser()
parser.add_argument("--verbose", help="Add more info", action="store_true")
parser.add_argument("--revision", nargs=1, default='none', help="filter with revision number")
parser.add_argument("--root", nargs=1, default=['.'], help="Root directory where to start the script")
args = parser.parse_args()

verbose = args.verbose

#
# Color text
#
GREY, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE = range(30, 38)
BOLD, DARK, _, UNDERLINE, BLINK, _, REVERSE, CONCEALED = range(1, 9)
RESET = '\033[0m'


def colored(text, color=None, attr=None):
    fmt_str = '\033[%dm%s'
    if color is not None:
        text = fmt_str % (color, text)
    if attr is not None:
        text = fmt_str % (attr, text)
    text += RESET
    return text


def set_windows_console_mode():
    # For Windows os to work and get the escape sequences correctly,
    # we'll need to enable the variable ENABLE_VIRTUAL_TERMINAL_PROCESSING
    if platform.system() == 'Windows':
        try:
            from ctypes import windll
            k = windll.kernel32
            k.SetConsoleMode(k.GetStdHandle(-11), 7)
            return True
        except ImportError:
            return False
    return False


set_windows_console_mode()

###########################################################################
#
def read_date_stamp(root, file):
    # https://docs.python.org/3/library/time.html
    created_time = os.path.getctime(root + "/" + file)
    modified_time = os.path.getmtime(root + "/" + file)
    delta = modified_time - created_time
    deltatime = time.gmtime(delta)
    #print(time.ctime(delta))
    print(time.ctime(created_time))
    #print(deltatime.)
    return

def parse_header_comments2(m, f_readme, root, file, bFound):
    # We found a block of comments
    str_comment = m.group(1)
    # remove '. ' on the title for the anchor
    str_anchor = re.sub(r"\.", "", file)
    # Now look for the previous way using MARKDOWN
    # # class <name>
    # <body of the doc>
    it2 = re.finditer('( *)#\s*(class|functions)\s+((?:.)+)\n((?:.|[\n\r])*)', str_comment)
    # Nornally we only have one it
    for n in it2:
        if bFound == False: 
            # put an anchor for the first found pattern
            f_readme.write("<a name=\"" + str_anchor + "\"></a>\n")
            print("    WARNING : " + root + "/" + file + " When Using Markdown doc instead of Doxygen, please use \code \endcode")
        bFound = True
        str_indent = n.group(1)
        str_type = n.group(2)
        str_title = n.group(3)
        str_body = n.group(4)
        # remove spaces on the left of text
        str_body = re.sub(r"\n"+ str_indent, "\n", str_body)
        # remove '/// ' on the left of text
        str_body = re.sub(r"/// +", "", str_body)
        # Add more '#' to the MD titles
        str_body = re.sub(r"# +", "## ", str_body)
        # change code encapsulation
        str_body = re.sub(r"\\code\{\.cpp\}", "``` c++", str_body)
        str_body = re.sub(r"\\code", "``` c++", str_body)
        str_body = re.sub(r"\\endcode", "```", str_body)
        # gather the name of the class and outline it in the body
        # TODO: do not perform this inside a code-block...
        #str_class = re.sub(r"\w+::", "", str_title)
        #str_body = re.sub(str_class, "**" + str_class + "**", str_body)
        f_readme.write("## "+ str_type +" " + str_title + "\n")
        f_readme.write(str_body + "\n")
        f_readme.write("\n\n")
    # Now look for the new way : Doxygen headers
    #/// \struct MyStruct
    #/// \brief ...Blah...
    #/// ...Blah...
    # let's detect only for few key commands : class, struct, fn
    # see https://www.doxygen.nl/manual/commands.html for more
    it2 = re.finditer('( *)\\\s*(class|struct|fn|namespace)\s+((?:.)+)\n((?:.|[\n\r])*)', str_comment)
    # Nornally we only have one it
    for n in it2:
        if bFound == False: # put an anchor for the first found pattern
            f_readme.write("<a name=\"" + str_anchor + "\"></a>\n")
        bFound = True
        str_indent = n.group(1)
        str_type = n.group(2)
        str_title = n.group(3)
        str_body = n.group(4)
        # remove '/// ' on the left of text
        str_body = re.sub(r"/// *", "", str_body)
        # Add more '#' to the MD titles
        str_body = re.sub(r"# +", "## ", str_body)
        # remove space on the left
        str_body = re.sub(r"\n"+ str_indent, "\n", str_body)
        # remove \brief
        str_body = re.sub(r"\\brief +", "> ", str_body)
        # change code encapsulation
        str_body = re.sub(r"\\code\{\.cpp\}", "``` c++", str_body)
        str_body = re.sub(r"\\code", "``` c++", str_body)
        str_body = re.sub(r"\\endcode", "```", str_body)
        # gather the name of the class and outline it in the body
        # TODO: do not perform this inside a code-block...
        #str_class = re.sub(r"\w+::", "", str_title)
        #str_body = re.sub(str_class, "**" + str_class + "**", str_body)
        if str_type == "fn": str_type = "function"
        f_readme.write("## "+ str_type +" " + str_title + "\n")
        f_readme.write(str_body + "\n")
        f_readme.write("\n\n")
    return bFound

def parse_header_comments(f_readme, root, file, str):
    # Look for a relevant Block of comments made of '/// ...'
    it = re.finditer('((/// *.*\n)+)', str)
    for m in it:
        if "\\nodoc" in str:
            print("    skipping " + root + "/" + file + " on request")
            return
    if(verbose):
        print("    parsing " + root + "/" + file)
    f_readme.write("_____\n\n")
    f_readme.write("# " + file + "\n\n")
    bFound = False
    # Look for a relevant Block of comments made of '/// ...'
    it = re.finditer('((/// *.*\n)+)', str)
    for m in it:
        bFound = parse_header_comments2(m, f_readme, root, file, bFound)
    # Look for a relevant block of comments starting with /**
    it = re.finditer('/\*\*((.|[\n\r])*?)\*/', str)
    for m in it:
        bFound = parse_header_comments2(m, f_readme, root, file, bFound)
    if not bFound:
        f_readme.write("\\todo write some documentation for " + file + "\n")
    return
###########################################################################
# Walking through all folders recursively
excluded_folders = [
    ".git",
    ".vscode",
    "cmake",
    "doxygen",
    "resources",
    "third_party",
    "KHR",
    "GL"
]
for root, folders, files in os.walk(args.root[0]):
    root = root.replace('\\', '/')
    argroot = args.root[0].replace('\\', '/')
    localroot = root.replace(argroot, '.')
    skip = False
    for d in excluded_folders:
        if d in root:
            skip = True
            break
    if skip: continue
    if root == '.': # Do not overwrite the main README.md : it is hand-written !
        continue
    # create a readme for each folder
    f_readme = open(root + "/README.md", "w")
    if(verbose):
        print("generating " + root + "/README.md")
    rootn = os.path.normpath(root)
    f_readme.write("# Helpers " + rootn + "\n\n")
    f_readme.write("Table of Contents\n\n")
    for file in files:
        if(".h" in file):
            if args.revision != "none":
                read_date_stamp(root, file)
            ftag = file.replace(".", "")
            f_readme.write("- ["+ file + "](#"+ ftag +")\n")
    
    for file in files:
        if(".h" in file):
            if args.revision != "none":
                read_date_stamp(root, file)
            filename = root + "/" + file
            f_header = open(filename, "r", encoding='utf8')
            str = f_header.read()
            f_header.close()
            parse_header_comments(f_readme, localroot, file, str)
    f_readme.close()

print("Done")
