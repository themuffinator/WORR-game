# WORR Versioning

## Semantic versioning
- WORR follows [SemVer](https://semver.org/) in the form `MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]`.
- `MAJOR` increments for breaking network protocol or gameplay scripting changes that require coordinated updates.
- `MINOR` increments for backwards-compatible feature releases; older clients remain compatible with newer servers when the major version matches.
- `PATCH` increments for backwards-compatible bug fixes and documentation-only updates.
- Build metadata (`+build`) may appear in developer snapshots but is ignored by compatibility checks.

## Pre-release identifiers
- Pre-release tags use lowercase identifiers joined by dots (for example `1.4.0-rc.2`).
- Supported identifiers are `rc`, `beta`, and `nightly`, each followed by an incrementing integer.
- Pre-release builds sort before the corresponding final release; upgrade prompts warn that the build is not yet final.
- Development forks can append custom identifiers after the official tag (for example `1.5.0-beta.1.community.3`).

## Release channels
- **Stable** – tagged `MAJOR.MINOR.PATCH` releases. Expect long-lived compatibility and server-required updates only when majors change.
- **Release Candidate (RC)** – tagged `-rc.N`. Intended for final testing; only critical fixes land before promotion to stable.
- **Beta** – tagged `-beta.N`. Feature-complete for the cycle but open to balance tweaks and translation updates.
- **Nightly** – tagged `-nightly.N`. Automatically generated snapshots that may include incomplete work or temporary instrumentation.

## Upgrade expectations
- **Players** – stable builds install automatically; upgrading to RC/Beta/Nightly is opt-in via the launcher and may require manual rollback.
- **Server admins** – upgrade to the newest stable within two weeks of release to stay in sync with public clients; RCs are recommended for staging servers; nightly builds are unsupported in production.
- **Contributors** – base feature work on the latest nightly or main branch; verify fixes against the current RC when one exists and note any protocol-impacting changes in the changelog draft.

## Updating the project version
Use `tools/release/bump_version.py` to update the canonical `VERSION` file and regenerate the C++ header that surfaces version information to the game code.

```
python3 tools/release/bump_version.py --minor
```

The script understands the following flags:

- `--major`, `--minor`, `--patch` – increment the corresponding part of the semantic version. Selecting a larger component resets the smaller numbers and clears any pre-release or build metadata.
- `--prerelease <value>` – set or clear (`""`) the pre-release identifier.
- `--build <value>` – set or clear (`""`) the build metadata identifier.
- `--no-stage` – skip the automatic `git add VERSION src/shared/version_autogen.hpp` step.

When run without `--no-stage`, the script stages the modified files. If staging fails (for example because Git is unavailable), it prints the `git add` command contributors should run manually.

## Release notes template
The automated release workflow seeds draft GitHub releases from [`.github/release_template.md`](../.github/release_template.md). Keep the compatibility notes, upgrade steps, and deprecation warning sections up to date so maintainers can ship consistent guidance with each tag.
