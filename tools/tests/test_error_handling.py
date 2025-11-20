import io
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path
from unittest.mock import patch

from tools import extract_entities, fix_encoding, header


class FixEncodingTests(unittest.TestCase):
    def test_reports_missing_encoding_for_empty_file(self):
        with tempfile.TemporaryDirectory() as tmp_dir:
            input_path = Path(tmp_dir) / "empty.txt"
            input_path.write_bytes(b"")
            output_path = Path(tmp_dir) / "out.txt"

            buffer = io.StringIO()
            with redirect_stdout(buffer):
                fix_encoding.fix_encoding(str(input_path), str(output_path))

            self.assertIn("Failed to detect encoding", buffer.getvalue())
            self.assertFalse(output_path.exists(), "No output should be written when encoding is missing")

    def test_reports_decode_errors(self):
        with tempfile.TemporaryDirectory() as tmp_dir:
            input_path = Path(tmp_dir) / "data.txt"
            output_path = Path(tmp_dir) / "out.txt"

            with patch("tools.fix_encoding.detect_encoding", return_value=("invalid-encoding", b"abc")):
                buffer = io.StringIO()
                with redirect_stdout(buffer):
                    fix_encoding.fix_encoding(str(input_path), str(output_path))

            self.assertIn("Failed to decode", buffer.getvalue())
            self.assertFalse(output_path.exists(), "No output should be written when decoding fails")


class ExtractEntitiesTests(unittest.TestCase):
    def test_handles_missing_cpp_file(self):
        with tempfile.TemporaryDirectory() as tmp_dir:
            missing_file = Path(tmp_dir) / "missing.cpp"

            buffer = io.StringIO()
            with redirect_stdout(buffer):
                matches = extract_entities.read_cpp_file_for_entities(missing_file)

            self.assertEqual([], matches)
            self.assertIn("file not found", buffer.getvalue())

    def test_handles_permission_error(self):
        with tempfile.TemporaryDirectory() as tmp_dir:
            file_path = Path(tmp_dir) / "protected.cpp"
            file_path.write_text("// content")

            with patch("builtins.open", side_effect=PermissionError("permission denied")):
                buffer = io.StringIO()
                with redirect_stdout(buffer):
                    matches = extract_entities.read_cpp_file_for_entities(file_path)

            self.assertEqual([], matches)
            self.assertIn("permission denied", buffer.getvalue())


class HeaderTests(unittest.TestCase):
    def test_reports_missing_include_file(self):
        includes = header.read_includes_from_file("./nonexistent.cpp")

        self.assertEqual(1, len(includes))
        self.assertIn("not found", includes[0])

    def test_reports_unreadable_include_file(self):
        with tempfile.TemporaryDirectory() as tmp_dir:
            file_path = Path(tmp_dir) / "data.cpp"
            file_path.write_text("#include <iostream>\n")

            with patch("builtins.open", side_effect=PermissionError("no access")):
                includes = header.read_includes_from_file(str(file_path))

        self.assertEqual(1, len(includes))
        self.assertIn("permission denied", includes[0])


if __name__ == "__main__":
    unittest.main()
