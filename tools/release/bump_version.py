#!/usr/bin/env python3
"""Utility to bump the WORR project version.

This script updates the VERSION file and regenerates src/shared/version_autogen.hpp.
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

REPO_ROOT = Path(__file__).resolve().parents[2]
VERSION_FILE = REPO_ROOT / "VERSION"
HEADER_FILE = REPO_ROOT / "src" / "shared" / "version_autogen.hpp"

SEMVER_RE = re.compile(
    r"^(?P<major>0|[1-9]\d*)\.(?P<minor>0|[1-9]\d*)\.(?P<patch>0|[1-9]\d*)"
    r"(?:-(?P<prerelease>[0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*))?"
    r"(?:\+(?P<build>[0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*))?$"
)


@dataclass
class SemVer:
    major: int
    minor: int
    patch: int
    prerelease: Optional[str] = None
    build: Optional[str] = None

    @classmethod
    def parse(cls, text: str) -> "SemVer":
        match = SEMVER_RE.match(text.strip())
        if not match:
            raise ValueError(
                "VERSION file does not contain a valid semantic version: "
                f"{text!r}"
            )
        return cls(
            major=int(match.group("major")),
            minor=int(match.group("minor")),
            patch=int(match.group("patch")),
            prerelease=match.group("prerelease"),
            build=match.group("build"),
        )

    def bump(self, part: str) -> None:
        if part == "major":
            self.major += 1
            self.minor = 0
            self.patch = 0
        elif part == "minor":
            self.minor += 1
            self.patch = 0
        elif part == "patch":
            self.patch += 1
        else:
            raise ValueError(f"Unsupported bump part: {part}")
        self.prerelease = None
        self.build = None

    def with_prerelease(self, value: Optional[str]) -> None:
        self.prerelease = value or None

    def with_build(self, value: Optional[str]) -> None:
        self.build = value or None

    def __str__(self) -> str:
        result = f"{self.major}.{self.minor}.{self.patch}"
        if self.prerelease:
            result += f"-{self.prerelease}"
        if self.build:
            result += f"+{self.build}"
        return result


HEADER_TEMPLATE = """// This file is generated. Do not edit directly.
#pragma once

#define WORR_VERSION_MAJOR {major}
#define WORR_VERSION_MINOR {minor}
#define WORR_VERSION_PATCH {patch}
#define WORR_VERSION_PRERELEASE "{prerelease}"
#define WORR_BUILD_METADATA "{build}"
#define WORR_SEMVER "{version}"
"""


class GitError(Exception):
    pass


def stage_files(*paths: Path) -> None:
    try:
        subprocess.run(["git", "add", *map(str, paths)], check=True)
    except FileNotFoundError as exc:
        raise GitError("git command not found. Stage files manually.") from exc
    except subprocess.CalledProcessError as exc:
        raise GitError("Failed to stage files with git. Stage them manually.") from exc


def escape(string: Optional[str]) -> str:
    if not string:
        return ""
    return string.replace("\\", "\\\\").replace('"', '\\"')


def write_header(version: SemVer) -> None:
    header_contents = HEADER_TEMPLATE.format(
        major=version.major,
        minor=version.minor,
        patch=version.patch,
        prerelease=escape(version.prerelease),
        build=escape(version.build),
        version=escape(str(version)),
    )
    HEADER_FILE.parent.mkdir(parents=True, exist_ok=True)
    HEADER_FILE.write_text(header_contents + "\n")


def write_version_file(version: SemVer) -> None:
    VERSION_FILE.write_text(str(version) + "\n")


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Bump the WORR project version.")
    group = parser.add_mutually_exclusive_group(required=False)
    group.add_argument("--major", action="store_true", help="Increment the major version")
    group.add_argument("--minor", action="store_true", help="Increment the minor version")
    group.add_argument("--patch", action="store_true", help="Increment the patch version")
    parser.add_argument(
        "--prerelease",
        help="Set the pre-release identifier (empty string clears it)",
        default=None,
    )
    parser.add_argument(
        "--build",
        help="Set the build metadata (empty string clears it)",
        default=None,
    )
    parser.add_argument(
        "--no-stage",
        action="store_true",
        help="Do not attempt to stage files automatically",
    )
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    if not VERSION_FILE.exists():
        print(f"VERSION file not found at {VERSION_FILE}.", file=sys.stderr)
        return 1

    args = parse_args(argv)

    try:
        version = SemVer.parse(VERSION_FILE.read_text())
    except ValueError as exc:
        print(exc, file=sys.stderr)
        return 1

    if args.major:
        version.bump("major")
    elif args.minor:
        version.bump("minor")
    elif args.patch:
        version.bump("patch")

    if args.prerelease is not None:
        version.with_prerelease(args.prerelease)
    if args.build is not None:
        version.with_build(args.build)

    write_version_file(version)
    write_header(version)

    if args.no_stage:
        staged = False
    else:
        try:
            stage_files(VERSION_FILE, HEADER_FILE)
            staged = True
        except GitError as exc:
            print(exc, file=sys.stderr)
            staged = False

    print(f"Updated version to {version}.")
    if staged:
        print("Files staged: VERSION, src/shared/version_autogen.hpp")
    else:
        print("Run 'git add VERSION src/shared/version_autogen.hpp' to stage the changes.")

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
