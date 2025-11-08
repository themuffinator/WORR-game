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
