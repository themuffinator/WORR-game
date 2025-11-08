#!/usr/bin/env python3
"""Create release bundles for WORR.

This script collects binaries, configuration files, and documentation into
versioned ZIP archives following the ``WORR-vMAJOR.MINOR.PATCH[-pre].zip``
naming convention. It is intentionally lightweight so it can run on Linux,
macOS, and Windows without additional dependencies.
"""

from __future__ import annotations

import argparse
import re
import shutil
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable
from zipfile import ZIP_DEFLATED, ZipFile


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_VERSION_FILE = REPO_ROOT / "VERSION"

DEFAULT_BINARY_DIR = REPO_ROOT / "build" / "bin"
DEFAULT_CONFIG_DIR = REPO_ROOT / "tools" / "pack" / "rerelease"
DEFAULT_DOCS_DIR = REPO_ROOT / "docs"
DEFAULT_EXTRAS = [
    REPO_ROOT / "README.md",
    REPO_ROOT / "LICENSE",
    REPO_ROOT / "VERSION",
]


SEMVER_RE = re.compile(
    r"^(?P<major>0|[1-9]\d*)\.(?P<minor>0|[1-9]\d*)\.(?P<patch>0|[1-9]\d*)"
    r"(?:-(?P<prerelease>[0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*))?"
    r"(?:\+(?P<build>[0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*))?$"
)


class BundleError(Exception):
    """Raised when bundling cannot proceed."""


@dataclass(frozen=True)
class Version:
    """Simple semantic version container."""

    major: int
    minor: int
    patch: int
    prerelease: str | None = None
    build: str | None = None

    @classmethod
    def parse(cls, text: str) -> "Version":
        match = SEMVER_RE.match(text.strip())
        if not match:
            raise BundleError(
                "VERSION must follow MAJOR.MINOR.PATCH[-pre][+build]; "
                f"got {text!r}"
            )
        return cls(
            major=int(match.group("major")),
            minor=int(match.group("minor")),
            patch=int(match.group("patch")),
            prerelease=match.group("prerelease"),
            build=match.group("build"),
        )

    @property
    def tag(self) -> str:
        base = f"v{self.major}.{self.minor}.{self.patch}"
        if self.prerelease:
            base += f"-{self.prerelease}"
        return base

    def __str__(self) -> str:  # pragma: no cover - simple wrapper
        result = f"{self.major}.{self.minor}.{self.patch}"
        if self.prerelease:
            result += f"-{self.prerelease}"
        if self.build:
            result += f"+{self.build}"
        return result


def existing_paths(paths: Iterable[Path], description: str) -> list[Path]:
    resolved: list[Path] = []
    missing: list[Path] = []
    for raw in paths:
        path = raw.resolve()
        if path.exists():
            resolved.append(path)
        else:
            missing.append(path)
    if missing:
        missing_text = ", ".join(str(p) for p in missing)
        raise BundleError(f"Missing {description}: {missing_text}")
    return resolved


def copy_into(sources: Iterable[Path], destination: Path) -> None:
    destination.mkdir(parents=True, exist_ok=True)
    for source in sources:
        if source.is_dir():
            for entry in source.iterdir():
                target = destination / entry.name
                if entry.is_dir():
                    shutil.copytree(entry, target, dirs_exist_ok=True)
                else:
                    target.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(entry, target)
        else:
            target = destination / source.name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, target)


def write_version_file(version: Version, bundle_root: Path) -> None:
    version_path = bundle_root / "VERSION"
    version_path.write_text(str(version) + "\n", encoding="utf-8")


def make_bundle(args: argparse.Namespace) -> Path:
    version_text = args.version or args.version_file.read_text(encoding="utf-8").strip()
    version = Version.parse(version_text)

    output_dir = args.output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    bundle_name = f"WORR-{version.tag}"
    zip_path = output_dir / f"{bundle_name}.zip"

    binary_sources = existing_paths(args.binaries, "binary paths")
    config_sources = existing_paths(args.configs, "configuration paths")
    doc_sources = existing_paths(args.docs, "documentation paths")
    extra_sources = existing_paths(args.extras, "extra paths") if args.extras else []

    with tempfile.TemporaryDirectory() as temp_dir:
        staging_root = Path(temp_dir)
        bundle_root = staging_root / bundle_name
        bundle_root.mkdir(parents=True, exist_ok=True)

        copy_into(binary_sources, bundle_root / "bin")
        copy_into(config_sources, bundle_root / "config")
        copy_into(doc_sources, bundle_root / "docs")
        if extra_sources:
            copy_into(extra_sources, bundle_root)

        write_version_file(version, bundle_root)

        with ZipFile(zip_path, "w", compression=ZIP_DEFLATED) as zip_file:
            for path in bundle_root.rglob("*"):
                archive_name = path.relative_to(staging_root)
                zip_file.write(path, archive_name)

    return zip_path


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--version",
        help="Override the version string instead of reading from the VERSION file.",
    )
    parser.add_argument(
        "--version-file",
        type=Path,
        default=DEFAULT_VERSION_FILE,
        help="Path to the VERSION file (default: %(default)s)",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=REPO_ROOT / "tools" / "pack" / "dist",
        help="Directory where the bundle should be created (default: %(default)s)",
    )
    parser.add_argument(
        "--binary",
        dest="binaries",
        type=Path,
        action="append",
        default=None,
        help=(
            "File or directory containing compiled binaries. May be passed multiple "
            f"times (default: {DEFAULT_BINARY_DIR})."
        ),
    )
    parser.add_argument(
        "--config",
        dest="configs",
        type=Path,
        action="append",
        default=None,
        help=(
            "Configuration source path. May be passed multiple times "
            f"(default: {DEFAULT_CONFIG_DIR})."
        ),
    )
    parser.add_argument(
        "--doc",
        dest="docs",
        type=Path,
        action="append",
        default=None,
        help=(
            "Documentation source path. May be passed multiple times "
            f"(default: {DEFAULT_DOCS_DIR})."
        ),
    )
    parser.add_argument(
        "--extra",
        dest="extras",
        type=Path,
        action="append",
        default=None,
        help=(
            "Additional files or directories to copy to the bundle root "
            f"(default: {', '.join(str(p) for p in DEFAULT_EXTRAS)})."
        ),
    )
    parser.add_argument(
        "--no-extras",
        action="store_true",
        help="Do not include any extra files (overrides --extra defaults).",
    )
    args = parser.parse_args(argv)
    if args.binaries is None:
        args.binaries = [DEFAULT_BINARY_DIR]
    if args.configs is None:
        args.configs = [DEFAULT_CONFIG_DIR]
    if args.docs is None:
        args.docs = [DEFAULT_DOCS_DIR]
    if args.extras is None:
        args.extras = list(DEFAULT_EXTRAS)
    return args


def main(argv: list[str] | None = None) -> int:
    if argv is None:
        argv = sys.argv[1:]
    args = parse_args(argv)
    if args.no_extras:
        args.extras = []

    try:
        zip_path = make_bundle(args)
    except BundleError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    print(f"Created bundle: {zip_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
