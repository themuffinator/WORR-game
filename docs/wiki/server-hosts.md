# WORR Server Host Guide

## Release Channels and Upgrade Cadence
- **Stable** builds are the baseline for production servers. They prioritize long-lived compatibility and only receive critical fixes between major releases. Plan to upgrade within two weeks of each stable tag so that public clients and servers stay synchronized.
- **Release Candidate (RC)** builds mirror the upcoming stable drop. Use them on staging environments that mirror production hardware and configs. Validate mods, map rotations, and third-party tooling against RCs before the stable tag lands.
- **Beta** builds are feature-complete for the cycle but continue to receive balance tweaks and localization updates. They suit enthusiast communities willing to triage regressions or to rehearse upgrade runbooks ahead of the next RC.
- **Nightly** builds expose the latest development work, including incomplete systems. Limit them to test servers used by contributors who need to validate fixes or protocol changes; nightly snapshots are not supported for live rotations.

**Upgrade rhythm.** Track releases via the `VERSION` file or repository tags, and maintain a staging server on the current RC (or latest stable when no RC exists). Promote staging builds after a smoke test that covers login, warmup, match flow, map voting, and stat export. For critical tournaments, freeze on the latest stable for at least one scrim cycle to validate player configs before inviting the public.

## Map System Configuration
WORR’s map framework combines a structured pool with rotation helpers. Keep the source files in `baseq2/` and reload them via admin commands after edits. Consult the [Cvar Reference](cvars.md) for flag defaults that govern map pools, selector logic, and MyMap queues before changing these files.

### Map Pool
Define the canonical list of playable maps in a JSON file referenced by `g_maps_pool_file`:
```json
{
  "maps": [
    {
      "bsp": "q2dm1",
      "title": "The Edge",
      "dm": true,
      "tdm": true,
      "ctf": false,
      "duel": true,
      "min": 2,
      "max": 8,
      "gametype": 0,
      "ruleset": 0,
      "scorelimit": 30,
      "timelimit": 15,
      "popular": true,
      "custom": false
    }
  ]
}
```
Required keys: `bsp`, at least one game-mode flag (`dm` for deathmatch). Optional fields tailor matchmaking constraints and defaults such as score limit overrides or preferred rulesets.

> **Design hand-off:** When curating rotation updates, point level builders to the [Entity Catalogue](level-design.md#entity-catalogue) so spawnpads, arenas, and objective triggers match the metadata you advertise to players.

### Map Cycle
The `g_maps_cycle_file` points to a plain-text rotation list. Separate BSP names by spaces, commas, or newlines and annotate with comments:
```text
// This is a comment
/* block comment */
q2dm1 q2dm2, q2dm3
```
Only maps flagged `dm: true` in the active pool are eligible for the cycle.

### MyMap Queue
Enable personal map requests with `g_maps_mymap`. Each player may queue one map (max 8 across the lobby) using optional overrides:
```text
mymap q2dm1 +pu -ht
```
Flags let requesters toggle powerups, armor, ammo, health, the BFG, fall damage, and self damage for their pick. WORR blocks maps played in the last 30 minutes to keep rotations fresh.

### Selector Voting
`g_maps_selector` presents up to three vetted choices at match end. Candidates respect cycle eligibility, player-count bounds, recent-play cooldowns, and custom-map avoidance when necessary. Ties fall back to rotation defaults or a weighted random pick.

## `g_vote_flags` Matrix
`g_vote_flags` is a bit field that exposes vote commands to players. Combine values with bitwise OR.

| Bit (1 << n) | Value | Command | Description |
| --- | --- | --- | --- |
| 0 | 1 | `map` | Change to a specific map (accepts map flags). |
| 1 | 2 | `nextmap` | Advance to the next entry in the rotation. |
| 2 | 4 | `restart` | Restart the current match. |
| 3 | 8 | `gametype` | Switch to a different gametype. |
| 4 | 16 | `timelimit` | Adjust or clear the match time limit. |
| 5 | 32 | `scorelimit` | Adjust or clear the score limit. |
| 6 | 64 | `shuffle` | Shuffle teams based on skill. |
| 7 | 128 | `unlagged` | Toggle lag compensation. |
| 8 | 256 | `cointoss` | Flip a coin and announce the result. |
| 9 | 512 | `random` | Roll a random number. |
| 10 | 1024 | `balance` | Balance teams without a shuffle. |
| 11 | 2048 | `ruleset` | Change the active ruleset. |
| 12 | 4096 | `arena` | Change arenas in Arena/RA2 modes. |

**Recommended presets.** Tailor defaults to the match structure while preserving admin authority for disruptive actions.

| Mode Profile | Suggested Flags | Bit Sum | Rationale |
| --- | --- | --- | --- |
| Free-for-all / Duel | `map`, `nextmap`, `restart`, `timelimit`, `scorelimit`, `unlagged`, `cointoss`, `random` | 951 | Keeps rotation tweaks available without exposing team management tools; unlagged toggles assist latency-sensitive duels. |
| Team Objective (TDM, CTF, DOM) | `map`, `nextmap`, `restart`, `gametype`, `timelimit`, `scorelimit`, `shuffle`, `unlagged`, `balance`, `cointoss` | 1535 | Adds cooperative tools (shuffle/balance) so teams can correct lopsided rosters mid-match. |
| Arena / RA2 | `map`, `nextmap`, `restart`, `timelimit`, `scorelimit`, `unlagged`, `cointoss`, `arena` | 4535 | Allows players to rotate arenas while avoiding team-specific controls unnecessary in duel-style arenas. |

Start with the preset that matches your primary playlist and extend it when the community requests additional autonomy (for example, enabling `ruleset` during mixed rules nights).

## Admin Command Quick Reference
Group admin commands by the workflows they support. All commands require admin status and, unless noted, work for spectators and during intermission. See the [Command Reference](commands.md) for exhaustive syntax, permissions, and player-facing utilities.

### Access Control & Policy
- `add_admin <client#|name|social_id>` / `remove_admin <…>` – Manage the persistent admin roster stored in `admin.txt`. Refreshes the in-memory list and announces changes.
- `add_ban <client#|name|social_id>` / `remove_ban <social_id>` – Append to or remove from `ban.txt`. Blocks banning hosts or listed admins to prevent accidental lockouts.
- `load_admins`, `load_bans` – Reload lists from disk after manual edits.
- `boot <client>` – Kick a player (excluding the lobby owner and other admins).

### Match Flow & Rules
- `start_match`, `end_match`, `reset_match`, `map_restart` – Control the match lifecycle (force start, intermission, reset, or full session reload).
- `ready_all`, `unready_all` – Force all players into or out of ready state after verifying warmup conditions.
- `force_vote <yes|no>` – Resolve the active vote immediately.
- `gametype <id>`, `ruleset <id>` – Apply new game modes or rulesets using validated identifiers.
- `set_map <map>` / `next_map` – Jump to any map in the pool or advance the rotation; honors pool validation.
- `load_mappool`, `load_mapcycle`, `load_motd` – Refresh key server config files without a restart.

### Team & Arena Management
- `balance`, `shuffle`, `set_team <client> <team>`, `lock_team <team>`, `unlock_team <team>` – Adjust team compositions, lock rosters, or repair imbalance in cooperative modes.
- `arena <num>` – Inspect or switch arenas when multi-arena content is active.

## Match Analytics Export
Each completed match emits a JSON payload that summarizes the session. Archive these payloads to feed dashboards, replay digests, or stat bots.

- Capture the output by tailing the server log directory or configuring WORR to write JSON blobs to a dedicated analytics folder. Rotate daily and retain at least the past tournament cycle so retroactive audits remain possible.
- Store payloads with deterministic filenames such as `<epoch>-<matchID>.json` to simplify ingest pipelines. Compress older files (for example, using `xz` or `zstd`) once they age beyond active analysis windows.
- Downstream consumers should treat optional keys as absent when missing—avoid defaulting to empty strings or zeroes so schema changes remain forward-compatible.
- Key fields include match metadata (`matchID`, `mapName`, `gameType`), aggregate totals (`totalKills`, `totalTeamKills`), and arrays for per-player or per-team breakdowns. Use the `matchTimeStart`/`matchTimeEnd` timestamps to align with external telemetry such as stream markers or anti-cheat logs.

Before promoting a new build, replay recent JSON exports against your analytics importer to ensure format changes are handled gracefully. Keep retention policies documented alongside your backup plan so staff know when it is safe to prune historical data.
