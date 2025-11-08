# Packaging assets

This directory contains helper material for building release bundles of the
WORR mod. The most important entry point is [`make_bundle.py`](make_bundle.py),
which assembles binaries, configuration files, and documentation into a
versioned ZIP archive that follows the ``WORR-vMAJOR.MINOR.PATCH[-pre].zip``
convention.

## Quick start

1. Build the project so that the compiled binaries land in `build/bin/` (or any
   directory of your choosing).
2. Run the bundler:

   ```sh
   python tools/pack/make_bundle.py \
       --binary build/bin \
       --config tools/pack/rerelease \
       --doc docs
   ```

   The command creates `tools/pack/dist/WORR-vX.Y.Z[-pre].zip` containing
   platform binaries in `bin/`, packaged configuration content in `config/`,
   the documentation set in `docs/`, and top-level metadata such as `LICENSE`,
   `README.md`, and `VERSION`.

## Options

`make_bundle.py` accepts multiple `--binary`, `--config`, `--doc`, and
`--extra` flags. Each flag may point to files or directories, allowing you to
add platform-specific builds or additional resources. `--no-extras` disables the
default inclusion of `README.md`, `LICENSE`, and `VERSION`. Use `--version` to
override the version string if you need to stage a release without touching the
repository's `VERSION` file.

All options are cross-platform and rely on standard Python libraries. The
resulting archives can be produced from Windows, macOS, or Linux without any
additional tooling.

## Legacy artifacts

Older Muff Mode packages are kept here for reference. They are not touched by
the bundler but can be inspected if historical assets are required. New
releases should always be generated through `make_bundle.py` to ensure the
expected naming scheme and directory layout.
