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

def read_cpp_file_for_entities(file_path: Path) -> list[str]:
    try:
        with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
            content = f.read()
    except FileNotFoundError:
        print(
            f"Error reading {file_path}: file not found. Verify the path before re-running."
        )
        return []
    except PermissionError as exc:
        print(
            f"Error reading {file_path}: permission denied ({exc}). Adjust file permissions "
            "or run with appropriate access."
        )
        return []
    except OSError as exc:
        print(
            f"Error reading {file_path}: {exc}. Ensure the file is accessible and readable."
        )
        return []

    return quaked_block_pattern.findall(content)


def collect_entity_blocks(project_root: Path) -> list[str]:
    entity_blocks: list[str] = []

    # Walk through the repository and search for QUAKED blocks in C++ sources.
    for root, dirs, files in os.walk(project_root):
        for file in files:
            if file.endswith(".cpp"):
                file_path = Path(root) / file
                matches = read_cpp_file_for_entities(file_path)
                if matches:
                    entity_blocks.extend(matches)

    return entity_blocks


def main() -> None:
    entity_blocks = collect_entity_blocks(PROJECT_ROOT)

    # Write all found blocks to the output file
    with open(OUTPUT_PATH, "w", encoding="utf-8") as out:
        for block in entity_blocks:
            out.write(block.strip())
            out.write("\n\n")

    print(f"{len(entity_blocks)} entity definitions written to {OUTPUT_PATH}.")


if __name__ == "__main__":
    main()
