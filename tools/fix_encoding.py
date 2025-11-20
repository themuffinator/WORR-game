import argparse

def detect_encoding(path):
    try:
        import chardet
    except ModuleNotFoundError as exc:
        raise ModuleNotFoundError(
            "The 'chardet' package is required to detect file encodings. "
            "Install it with 'pip install chardet'.") from exc
    with open(path, 'rb') as f:
        raw = f.read()
    result = chardet.detect(raw)
    return result['encoding'], raw

def fix_encoding(input_file, output_file):
    encoding, raw_bytes = detect_encoding(input_file)

    try:
        text = raw_bytes.decode(encoding, errors='replace')  # Replace illegal chars
    except (UnicodeDecodeError, LookupError) as e:
        print(
            f"Failed to decode using detected encoding '{encoding}': {e}. "
            "Specify a valid encoding or regenerate the source with UTF-8.")
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
