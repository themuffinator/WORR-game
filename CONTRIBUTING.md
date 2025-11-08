# Contributing Guidelines

Thank you for your interest in contributing to WORR-kex! This document captures the conventions and guardrails that help us keep contributions flowing smoothly.

## Getting Started

1. Fork the repository and create your working branch following the naming rules below.
2. Make focused commits with clear messages.
3. Add tests whenever you change behavior or add features.
4. Run the relevant test suites and linters before submitting your pull request.

## Branch Strategy

We follow a simplified Git Flow inspired model for branch names:

- `feature/<short-description>` for new features and general enhancements.
- `hotfix/<issue-or-bug-id>` for urgent patches that must reach production quickly.
- `release/x.y` for staging a release based on the `main` branch. The `x.y` portion should match the minor version being prepared (for example `release/1.4`).

Choose descriptive branch suffixes so reviewers can infer the intent from the branch name.

## Version Tagging

Releases are tagged using [Semantic Versioning](https://semver.org) with the pattern `vMAJOR.MINOR.PATCH[-pre]`.

- Increment the `MAJOR` version when making incompatible API changes.
- Increment the `MINOR` version when adding functionality in a backward-compatible manner.
- Increment the `PATCH` version when making backward-compatible bug fixes.
- Append a pre-release identifier such as `-alpha.1`, `-beta.1`, or `-rc.1` when publishing preview builds.

Tag release commits on the `main` branch so automation can detect and publish the new version.

## Support Window

Each major version receives active support (bug fixes and security updates) for 18 months from its initial release date. After the support window closes, only critical security fixes may be backported at the maintainers' discretion. Plan migrations to newer major versions before older releases age out of support.

## Changelog Expectations

Every pull request that changes user-visible behavior must include an entry in `CHANGELOG.md`. Use consistent formatting and group entries under the upcoming release heading following the [Keep a Changelog](https://keepachangelog.com) structure.

## Branch Protection and Required Status Checks

Our GitHub Actions coverage currently relies on the **CI** workflow (`.github/workflows/ci.yml`). It builds the game on Windows and Linux, regenerates the version header, runs the automated tests, and publishes build artifacts for review.

Branch protections require the `CI` check to finish successfully before merges are allowed:

| Branch pattern | Required checks |
| --- | --- |
| `main` | `CI` |
| `develop` | `CI` |
| `release/*` | `CI` |

Only repository administrators can override branch protections. Overrides should be rare, require two maintainer approvals on the pull request, and must document the rationale in the merge commit body. The release workflow (`.github/workflows/release.yml`) runs automatically for version tags and does not gate pull request merges.

### Hotfix workflow

Hotfixes start from the `main` branch using the `hotfix/<issue-or-bug-id>` naming rule. Open a pull request targeting `main` and allow the same set of required checks to complete before merging. After the hotfix lands on `main`, immediately merge `main` back into `develop` and any active `release/*` branch so those lines keep parity with production. If conflicts arise, resolve them on a short-lived branch and rerun the required workflows before completing the merges.

## Continuous Integration and Release Automation

- **CI** covers both Windows and Linux builds, executes the automated tests, and uploads artifacts for inspection. It accepts manual reruns from the GitHub Actions tab when transient failures occur.
- **Release** (`.github/workflows/release.yml`) packages tagged builds, produces checksums, and drafts the GitHub release notes using the repository template.
- Release managers coordinate any additional release-time automation (for example, scripts under `tools/release/`). Document bespoke steps in the release pull request so the audit trail reflects each manual action.

## Pull Request Checklist

Before opening a pull request, confirm that you have:

- [ ] Followed the branch naming conventions above.
- [ ] Updated documentation, samples, and changelog entries affected by your changes.
- [ ] Added or updated tests.
- [ ] Verified that all CI checks pass locally (or documented any known failures).
- [ ] Filled in a clear summary and testing plan in the PR description.

We appreciate your contributions and the time you invest in improving WORR-kex. If you have questions, open a discussion or draft pull request so maintainers can assist early in the process.
