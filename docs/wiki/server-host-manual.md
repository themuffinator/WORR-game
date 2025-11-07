# WORR Server Host Manual

## Gametype reference

### Flag legend
- **Teams** – match expects red/blue team assignments and team scoring.【F:src/server/g_local.hpp†L185-L193】
- **CTF** – enables capture-the-flag mechanics such as flag entities and captures.【F:src/server/g_local.hpp†L185-L193】
- **Arena** – round-based arena logic such as weapon stripping and shared spawns.【F:src/server/g_local.hpp†L185-L193】【F:src/server/g_local.hpp†L221-L240】
- **Rounds** – match progresses through distinct rounds or waves rather than a single continuous timer.【F:src/server/g_local.hpp†L185-L193】【F:src/server/g_local.hpp†L221-L240】
- **Elimination** – players are removed from play for the rest of the round when fragged.【F:src/server/g_local.hpp†L185-L193】【F:src/server/g_local.hpp†L221-L240】
- **Frags** – score is tracked by kills (either individual or team totals).【F:src/server/g_local.hpp†L185-L193】【F:src/server/g_local.hpp†L221-L240】
- **OneVOne** – gametype is limited to duels and enforces 1v1 matchmaking rules.【F:src/server/g_local.hpp†L185-L193】【F:src/server/g_local.hpp†L221-L240】

### Default round presets
Unless overridden, gametypes inherit the default preset of 8 second weapon respawns, holdables and powerups enabled, score limit 40, 10 minute time limit, starting health bonus enabled, and a 51% ready-up threshold.【F:src/server/match/match_state.cpp†L239-L265】

### Gametype quick reference
| Gametype | Primary purpose | Flags | Weapon respawn | Holdables | Powerups | Score limit | Time limit | Ready-up |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Free For All | Classic deathmatch with individual scoring.【F:src/server/g_local.hpp†L223-L224】 | Frags | 8s (default) | On | On | 40 | 10 | 51%【F:src/server/match/match_state.cpp†L249-L250】 |
| Duel | Strict 1v1 arena; disables item advantages.【F:src/server/g_local.hpp†L223-L225】 | OneVOne, Frags | 30s | Off | Off | Unlimited | 10 | 51%【F:src/server/match/match_state.cpp†L249-L251】 |
| Team Deathmatch | Team frag race with higher limits.【F:src/server/g_local.hpp†L225-L226】 | Teams, Frags | 30s | On | On | 100 | 20 | 51%【F:src/server/match/match_state.cpp†L251-L252】 |
| Domination | Control-point TDM variant using frag scoring.【F:src/server/g_local.hpp†L226-L227】 | Teams, Frags | 30s | On | On | 100 | 20 | 51%【F:src/server/match/match_state.cpp†L252-L253】 |
| Capture The Flag | Two-base flag runs.【F:src/server/g_local.hpp†L227-L228】 | Teams, CTF | 30s | On | On | 40 | 10 | 51%【F:src/server/match/match_state.cpp†L253-L254】 |
| Clan Arena | Round-based elimination starts with full loads.【F:src/server/g_local.hpp†L228-L229】 | Teams, Arena, Rounds, Elimination | 8s (default) | On | On | 40 | 10 | 51%【F:src/server/match/match_state.cpp†L239-L265】 |
| One Flag | Shared flag offense/defense rotation.【F:src/server/g_local.hpp†L229-L230】 | Teams, CTF | 8s (default) | On | On | 40 | 10 | 51%【F:src/server/match/match_state.cpp†L239-L265】 |
| Harvester | Collect skulls and score at base.【F:src/server/g_local.hpp†L230-L231】 | Teams, CTF | 8s (default) | On | On | 40 | 10 | 51%【F:src/server/match/match_state.cpp†L239-L265】 |
| Overload | Attack/defend power cores.【F:src/server/g_local.hpp†L231-L232】 | Teams, CTF | 8s (default) | On | On | 40 | 10 | 51%【F:src/server/match/match_state.cpp†L239-L265】 |
| Freeze Tag | Freeze/defrost rounds with elimination.【F:src/server/g_local.hpp†L232-L233】 | Teams, Rounds, Elimination | 8s (default) | On | On | 40 | 10 | 51%【F:src/server/match/match_state.cpp†L239-L265】 |
| CaptureStrike | Alternating offense/defense rounds with flag play.【F:src/server/g_local.hpp†L233-L234】 | Teams, Arena, Rounds, CTF, Elimination | 8s (default) | On | On | 40 | 10 | 51%【F:src/server/match/match_state.cpp†L239-L265】 |
| Red Rover | Team-based arena with player swapping per round.【F:src/server/g_local.hpp†L234-L235】 | Teams, Rounds, Arena | 8s (default) | On | On | 40 | 10 | 51%【F:src/server/match/match_state.cpp†L239-L265】 |
| Last Man Standing | Solo elimination over multiple lives.【F:src/server/g_local.hpp†L235-L236】 | Elimination | 8s (default) | On | On | 40 | 10 | 51%【F:src/server/match/match_state.cpp†L239-L265】 |
| Last Team Standing | Team-based elimination race.【F:src/server/g_local.hpp†L236-L237】 | Teams, Elimination | 8s (default) | On | On | 40 | 10 | 51%【F:src/server/match/match_state.cpp†L239-L265】 |
| Horde | PvE wave survival.【F:src/server/g_local.hpp†L237-L238】 | Rounds | 8s (default) | On | On | 40 | 10 | 51%【F:src/server/match/match_state.cpp†L239-L265】 |
| ProBall | Team sport variant with item lockdowns.【F:src/server/g_local.hpp†L238-L239】 | Teams | Stay (0s) | Off | Off | 10 | 15 | 60% (rounded)【F:src/server/match/match_state.cpp†L239-L265】 |
| Gauntlet | Round-based duel ladder.【F:src/server/g_local.hpp†L239-L240】 | OneVOne, Rounds, Frags | 8s (default) | On | On | 40 | 10 | 51%【F:src/server/match/match_state.cpp†L239-L265】 |

> **Note:** Entries that list the “default” preset use the shared round defaults described above; only Duel, Team Deathmatch, Domination, Capture the Flag, and ProBall change the stock timers or item allowances.【F:src/server/match/match_state.cpp†L249-L265】

## Map rotation management

### Map pool (JSON)
- Configure the JSON pool file path with `g_maps_pool_file`; it must live under `baseq2/` and defines every map the server is willing to load for automated selection.【F:docs/mapsystem.html†L22-L47】【F:src/server/gameplay/g_main.cpp†L912-L917】
- Each entry must include a `bsp` name and `dm` boolean, while optional metadata (player counts, titles, preferred gametypes, score/time overrides, popularity) can tailor rotation logic.【F:docs/mapsystem.html†L24-L46】

### Map cycle (rotation list)
- `g_maps_cycle_file` points to a plain-text list of BSP names separated by whitespace or commas; `//` and `/* */` comments are ignored for easy annotations.【F:docs/mapsystem.html†L49-L55】【F:src/server/gameplay/g_main.cpp†L912-L917】
- Only maps present in the active pool with `dm: true` are admitted into the rotation, ensuring cycles never reference disallowed maps.【F:docs/mapsystem.html†L49-L56】

### MyMap player queue
- Enable personalized picks with `g_maps_mymap`; each human can queue exactly one eligible map (maximum of eight queued) using `mymap <bsp> [+flag] [-flag]`.【F:docs/mapsystem.html†L58-L72】【F:src/server/gameplay/g_main.cpp†L912-L917】
- Recently played maps (within ~30 minutes) are rejected, and optional flags toggle per-match item restrictions such as powerups, armor, ammo, health, BFG, fall damage, or self-damage.【F:docs/mapsystem.html†L58-L72】

### Map selector vote
- Set `g_maps_selector` to enable the post-match selector: up to three suitable maps are offered based on cycle eligibility, player counts, recent history, and whether console clients are present (which suppresses custom content).【F:docs/mapsystem.html†L75-L84】【F:src/server/gameplay/g_main.cpp†L912-L917】
- When voting ties or no ballots are cast, the selector falls back to the rotation or a random pick, so the server never stalls.【F:docs/mapsystem.html†L75-L84】

### Operational commands
- `mappool` and `mapcycle` dump the current eligible lists; `loadmappool` / `loadmapcycle` refresh files at runtime without a restart.【F:docs/mapsystem.html†L86-L96】
- `mymap`, `callvote map`, and `callvote nextmap` integrate the queue and vote tools with the rotation system for on-demand map changes.【F:docs/mapsystem.html†L86-L96】
- If the JSON pool is missing or invalid, the legacy `g_map_list` cvar acts as an emergency fallback rotation.【F:docs/mapsystem.html†L99-L101】

## Vote administration & match controls

### Allowable vote types
- Toggle specific vote entries by setting `g_vote_flags` to the bitwise OR of the desired options; the default `8191` exposes all listed votes in the Call Vote menu.【F:docs/g_vote_flags.md†L1-L24】
- Use the provided bit table to enable or disable commands such as `map`, `nextmap`, `restart`, `gametype`, `timelimit`, `scorelimit`, `shuffle`, `unlagged`, `cointoss`, `random`, `balance`, `ruleset`, and `arena`. Combine decimal values as needed for your policy.【F:docs/g_vote_flags.md†L8-L23】
- Complement the flag mask with global toggles like `g_allow_voting`, `g_allow_spec_vote`, `g_allow_vote_mid_game`, and the per-player `g_vote_limit` counter to define how voting operates during play.【F:src/server/gameplay/g_main.cpp†L820-L907】

### Key match control cvars
- **Timeouts:** `match_timeout_length` sets the length (seconds) for manual timeouts, while readiness gating uses `g_warmup_countdown` and `g_warmup_ready_percentage` to control pre-match flow.【F:src/server/gameplay/g_main.cpp†L834-L909】【F:src/server/gameplay/g_main.cpp†L852-L907】
- **Resets & player flow:** Adjust `match_do_force_respawn`, `match_force_respawn_time`, and `match_force_join` to control how quickly players return to the arena; `match_start_no_humans` and `match_auto_join` automate match starts and team assignment when humans connect.【F:src/server/gameplay/g_main.cpp†L831-L909】
- **Overtime & pacing:** `match_do_overtime` (seconds), `match_items_respawn_rate`, and `g_weapon_respawn_time` tune how long matches extend and how quickly equipment reappears.【F:src/server/gameplay/g_main.cpp†L838-L910】
- **Damage & item modifiers:** Use `g_self_damage`, `g_falling_damage`, `g_damage_scale`, `g_no_powerups`, `g_no_items`, and `match_weapons_stay` / `g_mapspawn_no_bfg` to customize lethality and available gear per match.【F:src/server/gameplay/g_main.cpp†L829-L910】
- **Map/round behavior:** `match_map_same_level`, `match_allow_spawn_pads`, and `match_allow_teleporter_pads` shape how rounds reset the map, while `match_powerup_drops` and `match_powerup_min_player_lock` handle item persistence in team modes.【F:src/server/gameplay/g_main.cpp†L843-L917】

Use these controls in tandem to script league rule sets, casual rotations, or experimental modifiers without touching the game binaries.
