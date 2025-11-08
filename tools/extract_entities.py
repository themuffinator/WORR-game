import os
import re

# Output file
output_file = "entities.def"

# Regex pattern to match QUAKED comment blocks
quaked_block_pattern = re.compile(r'/\*QUAKED[\s\S]*?\*/', re.MULTILINE)

entity_blocks = []

# Walk through current and subdirectories
for root, dirs, files in os.walk("."):
    for file in files:
        if file.endswith(".cpp"):
            file_path = os.path.join(root, file)
            try:
                with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
                    content = f.read()
                    matches = quaked_block_pattern.findall(content)
                    if matches:
                        entity_blocks.extend(matches)
            except Exception as e:
                print(f"Error reading {file_path}: {e}")

# Write all found blocks to the output file
with open(output_file, "w", encoding="utf-8") as out:
    for block in entity_blocks:
        out.write(block.strip())
        out.write("\n\n")

print(f"{len(entity_blocks)} entity definitions written to {output_file}.")
