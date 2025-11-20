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

def read_includes_from_file(filepath):
    includes = []
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            for line in f:
                stripped = line.strip()
                if stripped.startswith('//') or stripped.startswith('/*') or not stripped:
                    continue
                if include_pattern.match(stripped):
                    includes.append(stripped)
    except FileNotFoundError:
        includes.append(f"// Error reading file: {filepath} not found. Confirm the path is valid.")
    except PermissionError as exc:
        includes.append(
            f"// Error reading file: permission denied ({exc}). Adjust permissions or run with access."
        )
    except OSError as exc:
        includes.append(f"// Error reading file: {exc}. Ensure the file is readable.")
    return includes


def collect_includes(allowed_dirs=ALLOWED_DIRS):
    includes_by_file = {}
    for root, dirs, files in os.walk('.'):
        # Normalize root path
        norm_root = os.path.normpath(root)
        if norm_root not in allowed_dirs:
            # Don't descend into subdirs of disallowed dirs
            dirs[:] = []
            continue

        for file in files:
            if file.endswith(SOURCE_EXTENSIONS):
                filepath = os.path.join(root, file)
                includes_by_file[filepath] = read_includes_from_file(filepath)
    return includes_by_file


def main():
    # Store results
    includes_by_file = collect_includes()

    # Write results
    with open('includes2.txt', 'w', encoding='utf-8') as out:
        for path, includes in sorted(includes_by_file.items()):
            out.write(f"{path}:\n")
            for inc in includes:
                out.write(f"  {inc}\n")
            out.write("\n")


if __name__ == '__main__':
    main()
