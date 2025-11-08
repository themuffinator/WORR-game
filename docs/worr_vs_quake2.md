# WORR vs. Quake II Rerelease — Reference Journal

## 1. Vision and Scope
- WORR positions itself as an "advanced gameplay expansion and server mod" that deliberately extends both single-player and multiplayer systems, citing vast entity support, competitive tooling, and UI polish beyond the base rerelease.【F:README.md†L1-L46】
- The original rerelease source focuses on shipping the combined game DLL, API updates, and tooling guidance for modders rather than gameplay-layer overhauls, underscoring a much narrower remit.【F:refs/quake2-rerelease-dll-main/README.md†L1-L120】

## 2. Gameplay and Balance Extensions
- WORR ships curated balance passes such as faster rockets, toned-down Plasma Beam range, and nerfed Hunter/Vengeance spheres, all documented for server operators. The stock rerelease retains original weapon values and lacks this balance changelog.【F:docs/gameplay.html†L20-L36】
- Manual QA checklists codify behaviors unique to WORR—e.g., warmup deaths consuming lives in elimination modes and strict validation of vote arguments—highlighting systems not present or documented in the base game.【F:docs/manual_qa_limited_lives.md†L1-L16】【F:docs/manual_qa_vote_timelimit.md†L1-L18】

## 3. Multiplayer Modes and Match Flow
- WORR enumerates 17 gametypes ranging from Domination and Clan Arena to ProBall and Gauntlet, tagging each with structured metadata (flags, spawn identifiers) for runtime logic. The rerelease DLL does not expose a comparable expanded roster.【F:src/server/g_local.hpp†L162-L241】
- The match engine promotes a full state machine (warmup, ready-up, countdown, live, overtime) with Marathon meta-progression that tracks leg scores across maps. This systemic layer is absent from the vanilla rerelease, which relies on simpler deathmatch loops.【F:src/server/match/match_state.cpp†L1-L168】【F:src/server/g_local.hpp†L2022-L2062】
- WORR persists ghosts for disconnected players, storing spawn origin/angles so returning clients respawn seamlessly—capabilities beyond the original game's session handling.【F:src/server/g_local.hpp†L2484-L2505】【F:src/server/gameplay/g_spawn_points.cpp†L1350-L1396】

## 4. Map Rotation, Voting, and Match Configuration
- WORR replaces the rerelease's `g_map_list`/`g_map_list_shuffle` cvars with a JSON-driven map pool, MyMap queues, and post-match selectors governed by player counts, history, and map tags. The stock DLL's rotation is limited to parsing the `g_map_list` string at runtime.【F:docs/mapsystem.html†L17-L102】【F:refs/quake2-rerelease-dll-main/rerelease/g_main.cpp†L321-L381】
- `g_vote_flags` exposes granular toggles for twelve vote types (maps, gametype, shuffle, arena changes, etc.), enabling curated call-vote menus that have no equivalent control surface upstream.【F:docs/g_vote_flags.md†L1-L24】

## 5. Administration and Server Operations
- WORR modularizes command handling into dedicated admin, client, cheat, system, and voting sources. Admin tooling covers ban/allow lists, arena forcing, ready-state mass toggles, and match control workflows absent from the stock command set.【F:src/server/commands/command_admin.cpp†L1-L200】
- The rerelease retains the monolithic `g_cmds.cpp` with traditional Quake II console commands, underscoring how WORR expands administrative breadth rather than modifying the legacy command hub directly.【F:refs/quake2-rerelease-dll-main/rerelease/g_cmds.cpp†L1-L160】

## 6. Player Experience and UI
- WORR augments the HUD renderer to respect spectator states, draw mini-score portraits, and format additional scoreboard data within the layout parser. The baseline rerelease renderer lacks these conditional paths and mini-score presentation tweaks.【F:src/client/cg_screen.cpp†L823-L940】【F:refs/quake2-rerelease-dll-main/rerelease/cg_screen.cpp†L781-L900】
- README highlights an adaptive HUD, mini-score displays, frag messaging, and announcer integration—features beyond the rerelease's default presentation layer.【F:README.md†L42-L46】

## 7. Data, Persistence, and Ecosystem Support
- WORR writes structured match summaries (IDs, totals, per-player stats) to JSON to feed external analytics—export surfaces not implemented in the base DLL.【F:docs/match_stats_json.md†L1-L31】
- Server-side client configs and ghost stats recording persist preferences and outcomes across reconnects, enabling continuity features the upstream project does not advertise.【F:src/server/g_local.hpp†L2484-L2505】【F:src/server/gameplay/g_spawn_points.cpp†L1350-L1396】

## 8. Release Management and Repository Structure
- WORR formalizes release cadence into Stable/RC/Beta/Nightly channels, guiding operators through upgrade paths and support expectations. The base rerelease repository instead documents build requirements and API notes for mod developers.【F:README.md†L48-L56】【F:refs/quake2-rerelease-dll-main/README.md†L1-L120】
- The repository keeps curated references (`refs/`), tooling (`tools/`), and documentation that expand on gameplay systems—material not bundled with the original source drop, underscoring WORR's focus on long-term maintainability and knowledge sharing.【F:README.md†L74-L82】
