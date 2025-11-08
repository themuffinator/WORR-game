# Programmers Guide

This guide orients engineers to WORR's codebase layout, workflows, and core gameplay systems so you can extend the mod without breaking its competitive backbone.【F:README.md†L1-L32】

## Repository layout & primary subsystems

| Area | What lives here | Key references |
| --- | --- | --- |
| `src/server/` | Core gameplay module, including the consolidated gametype registry, match-state enums, and level-scoped data tracked during a session.【F:src/server/g_local.hpp†L209-L344】【F:src/server/g_local.hpp†L2323-L2505】 | Start with `src/server/g_local.hpp` for top-level enums/structs and `src/server/match/` for runtime controllers like `match_state.cpp`. |
| `src/server/match/` | State machine governing warmup, countdown, live play, and leg transitions. Handles gametype switches and Marathon carry-over logic.【F:src/server/match/match_state.cpp†L1-L168】 | `match_state.cpp` orchestrates lifecycle helpers; supporting utilities live in the same folder. |
| `src/server/gameplay/` | Gameplay helpers such as spawn logic and client config persistence, including ghost-aware respawn paths.【F:src/server/gameplay/g_spawn_points.cpp†L1377-L1419】【F:src/server/gameplay/g_client_cfg.cpp†L564-L610】 | Pair these with the `LevelLocals` ghost array in `g_local.hpp`. |
| `docs/` | System design notes, analytics schemas, mode rules, and host guidance. Essential when exposing data externally or aligning with persona-focused docs.【F:docs/match_stats_json.md†L1-L31】【F:docs/domination.md†L13-L22】【F:docs/wiki/index.md†L1-L47】 | Use these specs to keep documentation synchronized with feature behavior. |
| `tools/` | Automation for CI, asset prep, and release engineering, backing the workflows described below.【F:README.md†L80-L82】【F:tools/ci/run_tests.py†L1-L112】【F:tools/release/bump_version.py†L1-L127】 | See `tools/ci/run_tests.py` for local CI parity and `tools/release/` scripts for versioning chores. |
| `refs/` | Vendored upstream drops and historical notes that document deviations from vanilla Quake II rerelease sources.【F:README.md†L80-L82】 | Consult when reconciling behavior with upstream expectations. |

## Workflow expectations

- **Branches.** Follow the Git-flow-inspired names: `feature/<short-description>` for enhancements, `hotfix/<issue-or-bug-id>` for urgent fixes off `main`, and `release/x.y` to stage a new minor release.【F:CONTRIBUTING.md†L11-L24】【F:CONTRIBUTING.md†L44-L52】
- **CI contract.** Every PR must pass the `CI` GitHub Actions build that compiles on Windows/Linux, regenerates headers, runs tests, and uploads artifacts. Local smoke testing should mirror this using `tools/ci/run_tests.py` to compile and execute the standalone C++ test corpus.【F:CONTRIBUTING.md†L36-L43】【F:tools/ci/run_tests.py†L1-L112】
- **Release automation.** Tag releases on `main` using semantic versioning so `.github/workflows/release.yml` can package builds. Scripts like `tools/release/bump_version.py` and `tools/release/gen_version_header.py` keep `VERSION` and `version_autogen.hpp` aligned before tagging.【F:CONTRIBUTING.md†L26-L35】【F:CONTRIBUTING.md†L62-L71】【F:tools/release/bump_version.py†L1-L127】【F:tools/release/gen_version_header.py†L1-L72】
- **Hotfix cadence.** Ship hotfixes from `main`, merge them back into `develop` and any active `release/*` branch immediately, and rerun the required checks after resolving conflicts.【F:CONTRIBUTING.md†L44-L62】
- **PR readiness.** Keep changes focused, update docs and changelog entries, add tests for behavioral shifts, and ensure CI passes (or document the exception) before requesting review.【F:CONTRIBUTING.md†L5-L10】【F:CONTRIBUTING.md†L72-L88】

## Gameplay systems to know

- **Gametype registry.** `GAME_MODES` centralizes metadata for every supported mode (short names, flags, spawn identifiers), while inline helpers in `Game` expose type/flag checks for other subsystems.【F:src/server/g_local.hpp†L209-L306】 This replaces parallel arrays and ensures new modes surface uniformly across UI, spawn logic, and voting.
- **Match state tracking.** `MatchState`, `WarmupState`, and `RoundState` enumerations, along with timers and counters on `LevelLocals`, describe the lifecycle of a match, including warmup gating, round sequencing, and overtime bookkeeping.【F:src/server/g_local.hpp†L309-L344】【F:src/server/g_local.hpp†L2431-L2484】 Runtime transitions live in `src/server/match/match_state.cpp`, which also carries Marathon leg data forward between maps.【F:src/server/match/match_state.cpp†L1-L168】
- **Ghost & analytics infrastructure.** Disconnect ghosts persist spawn origins/angles and pending respawn flags, while save hooks write ghost stats back through the client config layer.【F:src/server/g_local.hpp†L2484-L2505】【F:src/server/gameplay/g_spawn_points.cpp†L1377-L1419】【F:src/server/gameplay/g_client_cfg.cpp†L590-L610】 Post-match analytics serialize structured JSON (match IDs, totals, per-player arrays) for downstream tooling, so keep payload changes compatible with `docs/match_stats_json.md`.【F:docs/match_stats_json.md†L1-L31】

## Client allocation helpers

The game module manually manages `gclient_t` storage; always use `AllocateClientArray`, `FreeClientArray`, or `ReplaceClientArray` from `src/server/gameplay/g_clients.hpp` so constructors, lag buffers, and global counters stay synchronized. The helpers are required during initial startup and when loading save games, and `ConstructClients`/`DestroyClients` are available for custom lifetimes.【F:docs/client_allocation.md†L1-L30】

## Further reading

- Side-by-side feature deltas versus the stock rerelease: [docs/worr_vs_quake2.md](../worr_vs_quake2.md).【F:docs/worr_vs_quake2.md†L1-L63】
- Server-facing configuration defaults: [Cvar Reference](cvars.md).【F:docs/wiki/cvars.md†L1-L266】
- Persona guides for cross-discipline context: server hosts, players, and level designers are indexed alongside this guide in the wiki landing page.【F:docs/wiki/index.md†L5-L28】
