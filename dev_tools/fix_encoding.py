import chardet
import argparse

def detect_encoding(path):
    with open(path, 'rb') as f:
        raw = f.read()
    result = chardet.detect(raw)
    return result['encoding'], raw

def fix_encoding(input_file, output_file):
    encoding, raw_bytes = detect_encoding(input_file)

    try:
        text = raw_bytes.decode(encoding, errors='replace')  # Replace illegal chars
    except Exception as e:
        print(f"Failed to decode using {encoding}: {e}")
        return

    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(text)

    print(f"✅ Fixed encoding: {input_file} → {output_file} (from {encoding})")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Fix encoding to UTF-8")
    parser.add_argument("input", help="Input source file")
    parser.add_argument("output", help="Output cleaned file")
    args = parser.parse_args()

    fix_encoding(args.input, args.output)
