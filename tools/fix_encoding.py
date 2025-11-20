import chardet
import argparse

def detect_encoding(path):
    with open(path, 'rb') as f:
        raw = f.read()
    result = chardet.detect(raw)
    return result['encoding'], raw

def fix_encoding(input_file, output_file):
    encoding, raw_bytes = detect_encoding(input_file)

    if not encoding:
        print(
            f"Failed to detect encoding for {input_file}; ensure the file is not empty "
            "or specify the expected encoding manually."
        )
        return

    try:
        text = raw_bytes.decode(encoding, errors='replace')  # Replace illegal chars
    except (UnicodeDecodeError, LookupError) as e:
        print(
            f"Failed to decode {input_file} using detected encoding '{encoding}': {e}. "
            "Try converting the file to UTF-8 manually before re-running."
        )
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
