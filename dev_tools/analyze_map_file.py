import re
import sys
from collections import defaultdict

def parse_map_file(filepath):
    # Matches lines like: 0001:00001000  symbol_name  object.obj
    symbol_regex = re.compile(r'^\s*[0-9A-F]+:[0-9A-F]+\s+[^\s]+\s+(.+\.obj)', re.IGNORECASE)
    obj_size_map = defaultdict(int)

    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
        for line in f:
            match = symbol_regex.match(line)
            if match:
                obj_file = match.group(1)
                obj_size_map[obj_file] += 1  # We use 1 "unit" per symbol as approximation

    sorted_objs = sorted(obj_size_map.items(), key=lambda x: x[1], reverse=True)
    total_units = sum(size for _, size in sorted_objs)

    with open("map_summary.txt", "w", encoding='utf-8') as out:
        out.write("=== Object File Contribution Summary ===\n\n")
        for obj, size in sorted_objs:
            percent = (size / total_units) * 100 if total_units > 0 else 0
            out.write(f"{size:>6} entries  ({percent:5.2f}%)  {obj}\n")
        out.write(f"\nTotal symbol entries: {total_units}\n")

    print("Summary written to map_summary.txt")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python analyze_map_file.py <your_map_file.map>")
        sys.exit(1)

    parse_map_file(sys.argv[1])
