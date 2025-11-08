#!/usr/bin/env python3
"""Generate the project's version header from the VERSION file."""

from __future__ import annotations

import argparse
import re
from pathlib import Path

SEMVER_RE = re.compile(
    r"^(?P<major>0|[1-9]\d*)\.(?P<minor>0|[1-9]\d*)\.(?P<patch>0|[1-9]\d*)"
    r"(?:-(?P<prerelease>[0-9A-Za-z.-]+))?"
    r"(?:\+(?P<build>[0-9A-Za-z.-]+))?$"
)


def parse_version(version_text: str) -> dict[str, str | None]:
    match = SEMVER_RE.match(version_text.strip())
    if not match:
        raise ValueError(
            "VERSION must follow MAJOR.MINOR.PATCH[-pre][+build] semantics; "
            f"got: {version_text!r}"
        )
    return match.groupdict()


def render_header(version_info: dict[str, str | None]) -> str:
    major = version_info["major"]
    minor = version_info["minor"]
    patch = version_info["patch"]
    prerelease = version_info.get("prerelease") or ""
    build = version_info.get("build") or ""

    semver = f"{major}.{minor}.{patch}"
    if prerelease:
        semver += f"-{prerelease}"
    if build:
        semver += f"+{build}"

    header_lines = [
        "// This file is generated. Do not edit directly.",
        "#pragma once",
        "",
        f"#define WORR_VERSION_MAJOR {major}",
        f"#define WORR_VERSION_MINOR {minor}",
        f"#define WORR_VERSION_PATCH {patch}",
        f"#define WORR_VERSION_PRERELEASE \"{prerelease}\"",
        f"#define WORR_BUILD_METADATA \"{build}\"",
        f"#define WORR_SEMVER \"{semver}\"",
    ]

    return "\n".join(header_lines) + "\n"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version-file", default="VERSION", type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    version_text = args.version_file.read_text(encoding="utf-8").strip()
    version_info = parse_version(version_text)

    header_content = render_header(version_info)
    args.output.parent.mkdir(parents=True, exist_ok=True)

    # Only rewrite when content changes to avoid unnecessary rebuilds.
    if args.output.exists() and args.output.read_text(encoding="utf-8") == header_content:
        return

    args.output.write_text(header_content, encoding="utf-8")


if __name__ == "__main__":
    main()
