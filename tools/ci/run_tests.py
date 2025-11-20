#!/usr/bin/env python3
"""
Compile and run standalone C++ tests.

This script discovers C++ sources under ``tests/`` prefixed with ``test_`` and
compiles each one as an executable using the platform toolchain.  The resulting
executables are executed and the aggregated results are written to
``artifacts/test-results``.
"""

from __future__ import annotations

import os
import platform
import shutil
import signal
import subprocess
import sys
import textwrap
import threading
from dataclasses import dataclass
from datetime import datetime, UTC
from pathlib import Path
from typing import Iterable, List, Sequence

REPO_ROOT = Path(__file__).resolve().parents[2]
TEST_ROOT = REPO_ROOT / "tests"
SRC_ROOT = REPO_ROOT / "src"
INCLUDE_DIRS = [SRC_ROOT, SRC_ROOT / "fmt", SRC_ROOT / "json"]
ARTIFACT_DIR = REPO_ROOT / "artifacts" / "test-results"
LOG_FILE = ARTIFACT_DIR / "test-log.txt"
JUNIT_FILE = ARTIFACT_DIR / "junit.xml"

ACTIVE_PROCESSES: "set[subprocess.Popen[str]]" = set()
SHUTDOWN_EVENT = threading.Event()


@dataclass
class TestResult:
    name: str
    source: Path
    executable: Path
    compiled: bool
    compile_returncode: int
    run_returncode: int | None
    compile_stdout: str
    compile_stderr: str
    run_stdout: str | None
    run_stderr: str | None

    @property
    def passed(self) -> bool:
        return self.compiled and (self.run_returncode == 0)


class ToolchainError(RuntimeError):
    pass


class ShutdownRequested(RuntimeError):
    pass


def shutdown_requested() -> bool:
    return SHUTDOWN_EVENT.is_set()


def _terminate_processes() -> None:
    for proc in list(ACTIVE_PROCESSES):
        try:
            proc.terminate()
        except OSError as exc:
            print(
                f"warning: unable to terminate process {getattr(proc, 'pid', 'unknown')}: {exc}. "
                "Attempting forced kill; check permissions or lingering test runs if this persists.",
                file=sys.stderr,
            )
            try:
                proc.kill()
            except OSError as kill_exc:
                print(
                    f"error: failed to kill process {getattr(proc, 'pid', 'unknown')}: {kill_exc}. "
                    "Manually clean up stuck test processes before re-running.",
                    file=sys.stderr,
                )
        finally:
            ACTIVE_PROCESSES.discard(proc)


def request_shutdown(signum: int | None = None, frame: object | None = None) -> None:  # pragma: no cover - invoked by signal
    if shutdown_requested():
        return
    SHUTDOWN_EVENT.set()
    _terminate_processes()


def register_signal_handlers() -> None:
    for sig in (signal.SIGINT, signal.SIGTERM):
        signal.signal(sig, request_shutdown)


def find_tests() -> List[Path]:
    if not TEST_ROOT.exists():
        return []
    return sorted(TEST_ROOT.glob("test_*.cpp"))


def detect_compiler() -> Sequence[str]:
    system = platform.system()
    if system == "Windows":
        compiler = shutil.which("cl")
        if not compiler:
            raise ToolchainError("MSVC 'cl' compiler not found in PATH")
        return [compiler]

    for candidate in ("clang++", "g++"):
        compiler = shutil.which(candidate)
        if compiler:
            return [compiler]

    raise ToolchainError("Unable to locate a C++ compiler (clang++ or g++) in PATH")


def build_command(compiler_cmd: Sequence[str], source: Path, output: Path) -> List[str]:
    system = platform.system()
    include_dirs = [str(path) for path in INCLUDE_DIRS if path.exists()]
    if not include_dirs:
        include_dirs = [str(SRC_ROOT)]

    if system == "Windows":
        # ``cl`` requires include flags prefixed with ``/I``.
        includes: list[str] = []
        seen: set[str] = set()
        for directory in include_dirs:
            flag = f"/I{directory}"
            if flag not in seen:
                includes.append(flag)
                seen.add(flag)
        return [
            *compiler_cmd,
            "/nologo",
            "/std:c++20",
            *includes,
            str(source),
            f"/Fe:{output}",
        ]

    include_flags: list[str] = []
    for directory in include_dirs:
        include_flags.extend(["-I", directory])

    return [
        *compiler_cmd,
        "-std=c++20",
        *include_flags,
        str(source),
        "-o",
        str(output),
    ]


def run_subprocess(command: Sequence[str], cwd: Path) -> subprocess.CompletedProcess[str]:
    if shutdown_requested():
        raise ShutdownRequested("shutdown requested before process start")

    proc = subprocess.Popen(
        command,
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        start_new_session=True,
    )
    ACTIVE_PROCESSES.add(proc)
    try:
        stdout, stderr = proc.communicate()
    finally:
        ACTIVE_PROCESSES.discard(proc)

    if shutdown_requested():
        raise ShutdownRequested("shutdown requested during process execution")

    return subprocess.CompletedProcess(command, proc.returncode, stdout, stderr)


def run_test(compiler_cmd: Sequence[str], source: Path, build_dir: Path) -> TestResult:
    build_dir.mkdir(parents=True, exist_ok=True)
    exe_suffix = ".exe" if platform.system() == "Windows" else ""
    executable = build_dir / (source.stem + exe_suffix)

    compile_proc = run_subprocess(
        build_command(compiler_cmd, source, executable),
        cwd=REPO_ROOT,
    )
    compiled = compile_proc.returncode == 0 and executable.exists()

    run_returncode: int | None = None
    run_stdout: str | None = None
    run_stderr: str | None = None

    if compiled:
        run_proc = run_subprocess([str(executable)], cwd=REPO_ROOT)
        run_returncode = run_proc.returncode
        run_stdout = run_proc.stdout
        run_stderr = run_proc.stderr

    return TestResult(
        name=source.stem,
        source=source,
        executable=executable,
        compiled=compiled,
        compile_returncode=compile_proc.returncode,
        run_returncode=run_returncode,
        compile_stdout=compile_proc.stdout,
        compile_stderr=compile_proc.stderr,
        run_stdout=run_stdout,
        run_stderr=run_stderr,
    )


def write_log(results: Iterable[TestResult]) -> None:
    ARTIFACT_DIR.mkdir(parents=True, exist_ok=True)
    timestamp = datetime.now(UTC).isoformat(timespec="seconds").replace("+00:00", "Z")
    lines = [f"C++ Test Run - {timestamp}", ""]
    for result in results:
        status = "PASS" if result.passed else "FAIL"
        lines.append(f"[{status}] {result.name} ({result.source.relative_to(REPO_ROOT)})")
        lines.append("  Compile return code: %s" % result.compile_returncode)
        if result.compile_stdout:
            lines.append("  Compile stdout:\n" + textwrap.indent(result.compile_stdout.rstrip(), "    "))
        if result.compile_stderr:
            lines.append("  Compile stderr:\n" + textwrap.indent(result.compile_stderr.rstrip(), "    "))
        if result.compiled:
            lines.append("  Run return code: %s" % result.run_returncode)
            if result.run_stdout:
                lines.append("  Run stdout:\n" + textwrap.indent(result.run_stdout.rstrip(), "    "))
            if result.run_stderr:
                lines.append("  Run stderr:\n" + textwrap.indent(result.run_stderr.rstrip(), "    "))
        lines.append("")
    LOG_FILE.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_junit(results: Iterable[TestResult]) -> None:
    import xml.etree.ElementTree as ET

    results = list(results)
    testsuite = ET.Element(
        "testsuite",
        attrib={
            "name": "cpp-tests",
            "tests": str(len(results)),
            "failures": str(sum(1 for r in results if not r.passed)),
            "errors": "0",
        },
    )

    for result in results:
        testcase = ET.SubElement(
            testsuite,
            "testcase",
            attrib={"name": result.name, "classname": "cpp"},
        )
        if not result.passed:
            message_lines = [
                f"Compile return code: {result.compile_returncode}",
            ]
            if result.compile_stdout:
                message_lines.append("Compile stdout:\n" + result.compile_stdout)
            if result.compile_stderr:
                message_lines.append("Compile stderr:\n" + result.compile_stderr)
            if result.compiled and result.run_stdout:
                message_lines.append("Run stdout:\n" + result.run_stdout)
            if result.compiled and result.run_stderr:
                message_lines.append("Run stderr:\n" + result.run_stderr)
            failure = ET.SubElement(
                testcase,
                "failure",
                attrib={"message": "; ".join(line.splitlines()[0] for line in message_lines if line)},
            )
            failure.text = "\n".join(message_lines)

    tree = ET.ElementTree(testsuite)
    try:
        ARTIFACT_DIR.mkdir(parents=True, exist_ok=True)
        tree.write(JUNIT_FILE, encoding="utf-8", xml_declaration=True)
    except OSError as exc:
        raise OSError(
            f"Failed to write JUnit report to {JUNIT_FILE}: {exc}. "
            "Verify disk space and permissions for the artifacts directory.") from exc


def write_summary(results: Iterable[TestResult]) -> None:
    summary_path = os.environ.get("GITHUB_STEP_SUMMARY")
    if not summary_path:
        return
    results = list(results)
    passed = sum(1 for r in results if r.passed)
    failed = len(results) - passed
    lines = [
        "## C++ Test Summary",
        "",
        f"* Total: {len(results)}", 
        f"* Passed: {passed}",
        f"* Failed: {failed}",
        "",
        "| Test | Status |",
        "| --- | --- |",
    ]
    for result in results:
        status = "✅ Pass" if result.passed else "❌ Fail"
        lines.append(f"| `{result.name}` | {status} |")
    lines.append("")
    with open(summary_path, "a", encoding="utf-8") as handle:
        handle.write("\n".join(lines))


def main() -> int:
    register_signal_handlers()

    try:
        compiler_cmd = detect_compiler()
    except ToolchainError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    tests = find_tests()
    if not tests:
        print("No tests found.")
        return 0

    build_dir = ARTIFACT_DIR / "build"
    results: List[TestResult] = []

    interrupted = False

    try:
        for test_source in tests:
            if shutdown_requested():
                interrupted = True
                print("Shutdown requested. Skipping remaining tests.")
                break

            print(f"Running {test_source.name}...")
            try:
                result = run_test(compiler_cmd, test_source, build_dir)
            except ShutdownRequested:
                interrupted = True
                print("Shutdown requested. Aborting current test run.")
                break
            status = "PASS" if result.passed else "FAIL"
            print(f"  {status}")
            results.append(result)
    finally:
        write_log(results)
        try:
            write_junit(results)
        except OSError as exc:  # pragma: no cover - best effort only
            print(
                f"warning: failed to write JUnit report at {JUNIT_FILE}: {exc}. "
                "Ensure the artifacts directory is writable.",
                file=sys.stderr,
            )
        write_summary(results)

    failures = sum(1 for result in results if not result.passed)
    if failures:
        print(f"{failures} test(s) failed. See {LOG_FILE.relative_to(REPO_ROOT)} for details.")
    elif interrupted:
        print("Test run interrupted before completion.")
    else:
        print(f"All {len(results)} test(s) passed.")

    if interrupted:
        return 1
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
