# Server Host Operations Guide

This guide walks server operators through provisioning WORR, managing rotations, governing votes, and exporting analytics. WORR’s multiplayer stack layers on match presets, tournament flows, and persistent player state so administrators can deliver curated experiences beyond the base rerelease.【F:README.md†L20-L45】

## Release Channels & Upgrades
1. Pick the release stream that matches your risk tolerance: **Stable**, **Release Candidate**, **Beta**, or **Nightly** snapshots.【F:README.md†L50-L55】
2. Review the detailed [Versioning & Release Channels](docs/versioning.md) policy before promoting builds to production.
3. Stage upgrades on a private server first, mirroring live configuration and map pools.
4. Announce disruptive cvar or command changes to players, linking the relevant wiki page.

## Baseline Configuration
- Set your banner via `hostname` (`set hostname "Example WORR Hub"`).【F:src/server/gameplay/g_main.cpp†L709-L910】
- Define administrative passwords with `admin_password` and spectator locks through `spectator_password` when needed.【F:src/server/gameplay/g_main.cpp†L790-L904】
- Tune lobby sizing using `maxplayers`, `minplayers`, and `match_start_no_humans` to control auto-start behavior.【F:src/server/gameplay/g_main.cpp†L540-L849】
- Enable or disable ready-up (`warmup_enabled`, `warmup_do_ready_up`) and forced respawns (`match_do_force_respawn`, `match_force_respawn_time`) to shape match pacing.【F:src/server/gameplay/g_main.cpp†L828-L847】
- Publish match telemetry by setting `g_matchstats 1` so JSON payloads are emitted after each session.【F:src/server/gameplay/g_main.cpp†L875-L878】【F:docs/match_stats_json.md†L1-L31】

## Map System Management
WORR replaces `g_map_list` with a JSON-driven database, cycle, and per-player queue system.【F:docs/mapsystem.html†L17-L104】

### Map Pool (`g_maps_pool_file`)
- Author a JSON file under `baseq2/` matching the schema below and point `g_maps_pool_file` at it.【F:docs/mapsystem.html†L24-L47】【F:src/server/gameplay/g_main.cpp†L909-L914】
- Required key: `bsp`. Optional keys gate eligibility:
  - `dm`, `tdm`, `ctf`, `duel` — mark supported gametypes.
  - `min`/`max` — player count gating.
  - `ruleset`, `scorelimit`, `timelimit` — overrides applied on load.

### Map Cycle (`g_maps_cycle_file`)
- Maintain a plain-text rotation listing BSP names separated by commas, spaces, or line breaks; comments are allowed.【F:docs/mapsystem.html†L50-L55】
- Only maps flagged `dm: true` in the pool will enter the cycle, preventing typos from breaking rotations.【F:docs/mapsystem.html†L55-L56】

### MyMap Queue & Selector
- Toggle `g_maps_mymap` to let players queue personal picks with override flags such as `+pu` (powerups) or `+sd` (self damage).【F:docs/mapsystem.html†L58-L72】【F:src/server/gameplay/g_main.cpp†L912-L914】
- Enable `g_maps_selector` to surface up to three suitable candidates at intermission, filtered by player counts, history, and custom map policies.【F:docs/mapsystem.html†L75-L84】【F:src/server/gameplay/g_main.cpp†L910-L914】
- Reload JSON and cycle files live with the `load_mappool` and `load_mapcycle` admin commands after edits.【F:docs/mapsystem.html†L86-L97】【F:src/server/commands/command_admin.cpp†L494-L496】

## Vote Governance
- `g_vote_flags` exposes twelve discrete vote types. Use bitmasks to curate menus (e.g., disable `ruleset` changes on competitive servers).【F:docs/g_vote_flags.md†L1-L24】
- Limit repeated votes by lowering `g_vote_limit`, and gate mid-match calls with `g_allow_vote_mid_game` or spectator-only ballots via `g_allow_spec_vote`.【F:src/server/gameplay/g_main.cpp†L817-L904】
- Encourage balanced outcomes: pair `match_powerup_min_player_lock` with `match_powerup_drops` to prevent low-population exploit votes.【F:src/server/gameplay/g_main.cpp†L843-L846】

## Administrative Commands
Administrative verbs are restricted to authenticated operators (`g_allow_admin 1` and `admin_password`).【F:src/server/gameplay/g_main.cpp†L813-L821】

| Purpose | Commands |
| --- | --- |
| Access control | `add_admin`, `remove_admin`, `add_ban`, `remove_ban`, `load_admins`, `load_bans`. |
| Match flow | `start_match`, `end_match`, `reset_match`, `map_restart`, `next_map`, `force_vote`, `ready_all`, `unready_all`. |
| Gametype & arena | `gametype`, `ruleset`, `set_map`, `arena`, `shuffle`, `balance`. |
| Team management | `set_team`, `lock_team`, `unlock_team`. |
| Content refresh | `load_mappool`, `load_mapcycle`, `load_motd`. |

Refer to the [Command Reference](Commands.md) for syntax and examples derived from the registration tables.【F:src/server/commands/command_admin.cpp†L484-L510】

## Analytics & Match Archives
- Activate `g_matchstats` and ensure the server has write permissions to its stats directory so JSON payloads land reliably.【F:src/server/gameplay/g_main.cpp†L875-L878】【F:docs/match_stats_json.md†L1-L31】
- Pull top-level context (`matchID`, `gameType`, `ruleSet`, etc.) from each payload to populate dashboards or historical match browsers.【F:docs/match_stats_json.md†L6-L29】
- Rotate or compress logs periodically; tie rotations to the same cadence you use for `g_maps_pool_file` updates so analytics stay consistent.

## Checklist for Launching a New Server
1. Clone the latest Stable or RC tag and verify checksums.
2. Populate `baseq2/mapdb.json` and `mapcycle.txt` with curated rotations.
3. Configure passwords, vote policies, and match pacing cvars.
4. Upload MOTD files and ensure `g_motd_filename` points to the correct asset.【F:src/server/gameplay/g_main.cpp†L876-L878】
5. Restart the server, watch the console for JSON reload confirmations, and run a smoke test lobby covering ready-up, voting, and analytics export.

Document any bespoke overrides (ruleset scripts, entity remaps) alongside your configs so future maintainers can reconstruct the environment quickly.
