# This script generates a README.md file for each folder containing header files, 
# with a table of contents and documentation extracted from the header files.

# How to use:
# 1. Customize the excluded_folders list as per your project's needs.
# 2. Run the script in the root directory of your project.

# Note: To include documentation in the README.md file, enclose the documentation 
# within the "@DOC_START" and "@DOC_END" tags in the header files. Anything inside
# will be threated as Markdown documentation. 
# If the header contains @DOC_SKIP, the header will not try to generate documentation

# Note: Any title (`#`) will be demoted by two level to fit the documentation.
#       Level-1 (title): reserve, level-2 (sub-title): filename
#       Ex. "# MyClass"  -> "### MyClass"

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
            if "@DOC_START" in line.strip():
                in_doc_block = True
            elif "@DOC_END" in line.strip():
                in_doc_block = False
            elif in_doc_block:
                if line.lstrip().startswith("# "):
                    documentation += "##" + line.lstrip() + "\n"
                else:
                    documentation += line.strip() + "\n"
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
