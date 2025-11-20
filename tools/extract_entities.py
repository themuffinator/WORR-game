import os
import re
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent
REFS_DIR = PROJECT_ROOT / "refs"
OUTPUT_PATH = REFS_DIR / "entities.def"

# Ensure the refs directory exists so the export can succeed even if invoked
# before `refs/` has been created manually.
REFS_DIR.mkdir(exist_ok=True)

# Regex pattern to match QUAKED comment blocks
quaked_block_pattern = re.compile(r'/\*QUAKED[\s\S]*?\*/', re.MULTILINE)

entity_blocks = []

# Walk through the repository and search for QUAKED blocks in C++ sources.
for root, dirs, files in os.walk(PROJECT_ROOT):
    for file in files:
        if file.endswith(".cpp"):
            file_path = Path(root) / file
            try:
                with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
                    content = f.read()
                    matches = quaked_block_pattern.findall(content)
                    if matches:
                        entity_blocks.extend(matches)
            except OSError as e:
                print(f"Error reading {file_path}: {e}. Ensure the file exists and is readable.")

# Write all found blocks to the output file
with open(OUTPUT_PATH, "w", encoding="utf-8") as out:
    for block in entity_blocks:
        out.write(block.strip())
        out.write("\n\n")

print(f"{len(entity_blocks)} entity definitions written to {OUTPUT_PATH}.")
