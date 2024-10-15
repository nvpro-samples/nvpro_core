"""
This script generates a README.md file for each folder containing header files,
with a table of contents and documentation extracted from the header files.

How to use:
1. Customize the excluded_folders list as per your project's needs.
2. Run the script in the root directory of your project.

Note: To include documentation in the README.md file, enclose documentation
within "@DOC_START" and "@DOC_END" tags in the header files. Anything inside
will be treated as Markdown documentation. Files can contain multiple @DOC_START
and @DOC_END tags.
If a header file contains @DOC_SKIP, then no documentation will be generated
for it.

Note: Any title (`#`) will be demoted by two levels to fit the rest of the
      documentation. This is because the outer documentation reserves level-1
      titles, and uses level-2 titles for filenames.
      Ex. "# MyClass"  -> "### MyClass"

For example, a header file might have a documentation block after the license
like this:

```cpp
/** @DOC_START
# functions in nvvk

- makeAccessMaskPipelineStageFlags : depending on accessMask returns appropriate VkPipelineStageFlagBits
- cmdBegin : wraps vkBeginCommandBuffer with VkCommandBufferUsageFlags and implicitly handles VkCommandBufferBeginInfo setup
- makeSubmitInfo : VkSubmitInfo struct setup using provided arrays of signals and commandbuffers, leaving rest zeroed
@DOC_END */
```

and then more documentation blocks above classes or functions like this:

```cpp
/** @DOC_START
  # class nvvk::RingFences

  nvvk::RingFences recycles a fixed number of fences, provides information in which cycle
  we are currently at, and prevents accidental access to a cycle in-flight.

  A typical frame would start by "setCycleAndWait", which waits for the
  requested cycle to be available.
@DOC_END */

class RingFences
...
```

"""

import os

# Define excluded folders
excluded_folders = [
    ".git",
    ".vscode",
    "cmake",
    "doxygen",
    "resources",
    "third_party",
    "KHR",
    "GL",
    "OV",
    "PACKAGE-LICENSES",
    "_autogen"
]

# Function to generate table of contents
def generate_table_of_contents(header_files):
    return "## Table of Contents\n" + "\n".join(f"- [{file}](#{file.replace('.', '')})" for file in header_files)

# Function to extract documentation from header files
def extract_documentation(file_path):
    documentation = ""
    with open(file_path, 'r', encoding="utf-8") as file:
        in_doc_block = False
        for line in file:
            if "@DOC_START" in line:
                in_doc_block = True
                # For each doc block, we keep track of the minimum indentation
                # so that we can uniformly un-indent it.
                block_indent = 999
                block_lines = []
            elif "@DOC_END" in line:
                # Un-indent block
                for block_line in block_lines:
                    documentation += block_line[block_indent:] + "\n"
                # Reset for next block
                block_lines = []
                in_doc_block = False
            elif in_doc_block:
                # Remove whitespace and newline characters at the end
                line = line.rstrip()
                # Count number of whitespace characters
                lstripped = line.lstrip()                
                if len(line) > 0:
                    block_indent = min(block_indent, len(line) - len(lstripped))
                # If this is a title...
                if lstripped.startswith("# ") or lstripped.startswith("##"):
                    # Add two levels to it
                    line = line.replace('#', '###', 1)
                block_lines.append(line)
    if not documentation:
        documentation = "\n> Todo: Add documentation\n"
    return documentation


# Traverse through folders
for root, dirs, files in os.walk("."):
    print("Parsing sub-folder:", root)  # Print the sub-folder being parsed

    # Exclude specified folders
    dirs[:] = [d for d in dirs if d not in excluded_folders]

    # Filter header files
    header_files = []
    for file in files:
        if file.endswith((".h", ".hpp")):
            with open(os.path.join(root, file), encoding="utf-8") as f:
                # Check for "@DOC_SKIP" only if file extension matches
                if not any("@DOC_SKIP" in line for line in f):
                    header_files.append(file)
                else:
                    # Optional: Inform about skipping files (for clarity)
                    print(f" - Skipping file: {file}")  # Informative, optional

    # Proceed if there are header files
    if header_files:
        # Generate table of contents
        table_of_contents = generate_table_of_contents(header_files)

        # Create or append to README.md
        with open(os.path.join(root, "README.md"), "w") as readme:
            readme.write(table_of_contents + "\n")

            # Process each header file
            for header_file in header_files:
                readme.write(f"\n## {header_file}\n")
                header_path = os.path.join(root, header_file)

                # Extract documentation from header file
                documentation = extract_documentation(header_path)

                # Append documentation to README.md
                readme.write(documentation)
