import io
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path
from unittest import mock

import tools.ci.run_tests as run_tests
import tools.fix_encoding as fix_encoding


class TerminationLoggingTests(unittest.TestCase):
    def setUp(self) -> None:
        run_tests.ACTIVE_PROCESSES.clear()

    def test_terminate_logs_actionable_messages(self):
        class _BrokenProcess:
            def __init__(self):
                self.pid = 999

            def terminate(self):
                raise ProcessLookupError("process missing")

            def kill(self):
                raise PermissionError("cannot kill")

        proc = _BrokenProcess()
        run_tests.ACTIVE_PROCESSES.add(proc)

        buffer = io.StringIO()
        with redirect_stderr(buffer):
            run_tests._terminate_processes()

        log = buffer.getvalue()
        self.assertIn("unable to terminate process 999", log)
        self.assertIn("Manually clean up stuck test processes", log)
        self.assertEqual(run_tests.ACTIVE_PROCESSES, set())


class JUnitWriteFailureTests(unittest.TestCase):
    def test_write_junit_surfaces_artifact_error(self):
        dummy_result = run_tests.TestResult(
            name="sample",
            source=Path("test_sample.cpp"),
            executable=Path("./a.out"),
            compiled=True,
            compile_returncode=0,
            run_returncode=0,
            compile_stdout="",
            compile_stderr="",
            run_stdout="",
            run_stderr="",
        )

        with mock.patch("xml.etree.ElementTree.ElementTree.write", side_effect=OSError("read-only")):
            with self.assertRaises(OSError) as ctx:
                run_tests.write_junit([dummy_result])

        self.assertIn("Verify disk space and permissions", str(ctx.exception))


class FixEncodingFailureTests(unittest.TestCase):
    def test_fix_encoding_reports_lookup_errors(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            input_path = Path(tmpdir) / "input.bin"
            output_path = Path(tmpdir) / "output.txt"
            input_path.write_bytes(b"dummy")

            with mock.patch.object(fix_encoding, "detect_encoding", return_value=("unknown-charset", b"abc")):
                buffer = io.StringIO()
                with redirect_stdout(buffer):
                    fix_encoding.fix_encoding(input_path, output_path)

        message = buffer.getvalue()
        self.assertIn("Specify a valid encoding", message)
        self.assertFalse(output_path.exists())


if __name__ == "__main__":
    unittest.main()
