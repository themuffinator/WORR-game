import os
import re

# File extensions to include
SOURCE_EXTENSIONS = ('.h', '.hpp', '.cpp')

# Regex to match valid #include lines
include_pattern = re.compile(r'^\s*#\s*include\s+[<"].+[>"]')

# Only allow scanning these subdirectories (relative to cwd)
ALLOWED_DIRS = {'.', './bot', './menu', './monsters'}

# Normalize directory paths to match os.walk format
ALLOWED_DIRS = {os.path.normpath(d) for d in ALLOWED_DIRS}

# Store results
includes_by_file = {}

for root, dirs, files in os.walk('.'):
    # Normalize root path
    norm_root = os.path.normpath(root)
    if norm_root not in ALLOWED_DIRS:
        # Don't descend into subdirs of disallowed dirs
        dirs[:] = []
        continue

    for file in files:
        if file.endswith(SOURCE_EXTENSIONS):
            filepath = os.path.join(root, file)
            includes = []

            try:
                with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                    for line in f:
                        stripped = line.strip()
                        if stripped.startswith('//') or stripped.startswith('/*') or not stripped:
                            continue
                        if include_pattern.match(stripped):
                            includes.append(stripped)
            except OSError as e:
                includes.append(f"// Error reading file (check permissions or path): {e}")

            includes_by_file[filepath] = includes

# Write results
with open('includes2.txt', 'w', encoding='utf-8') as out:
    for path, includes in sorted(includes_by_file.items()):
        out.write(f"{path}:\n")
        for inc in includes:
            out.write(f"  {inc}\n")
        out.write("\n")
