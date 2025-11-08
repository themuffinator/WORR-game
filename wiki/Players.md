# Player Handbook

WORR modernizes Quake II combat with tuned weapon physics, refreshed HUD cues, and a broad gametype roster. This handbook summarizes the essentials so you can jump into any lobby prepared.【F:README.md†L20-L45】

## Movement & Combat Changes
- **Weapon tuning:** Rockets fly faster and deal consistent damage, Plasma Beam range and knockback are capped, and machinegun families adopt Quake III spread math for predictability.【F:docs/gameplay.html†L21-L51】
- **Knockback:** Splash impulses mirror Quake III, enabling reliable rocket jumps and air control adjustments.【F:docs/gameplay.html†L53-L56】
- **Ammunition:** Clip sizes and pickups are rebalanced; Bandolier and Ammo Pack respawn faster, so watch timers for resource denial.【F:docs/gameplay.html†L58-L84】
- **Health & Tech:** Spawn with +25 temporary health, adrenaline becomes a holdable, and tech regen alternates between health and armor for pacing.【F:docs/gameplay.html†L86-L118】
- **Powerups:** Haste, Battle Suit, and Regeneration deliver Quake III-inspired effects, and every powerup can drop on death—protect your carrier.【F:docs/gameplay.html†L101-L111】

## UI & Feedback
- WORR’s HUD is adaptive, layering mini-score displays, frag messages, and announcer cues to track match flow without opening the scoreboard.【F:README.md†L42-L45】
- Crosshair IDs (`match_crosshair_ids`) are enabled by default so you can confirm team status at a glance.【F:src/server/gameplay/g_main.cpp†L829-L831】
- Use `kb` and `fm` console commands to customize kill beeps and frag message feeds mid-match.【F:src/server/commands/command_client.cpp†L1284-L1290】

## Gametype Cheat Sheet
Each gametype exposes distinct objectives and spawn logic. The short codes below appear in the HUD, server browser, and vote menus.【F:src/server/g_local.hpp†L222-L305】

| Code | Name | Core Rules |
| --- | --- | --- |
| `FFA` | Free For All | Classic deathmatch; first to frag limit or highest score at time limit wins. |
| `DUEL` | Duel | 1v1 arena with Marathon queues and no spectators mid-round. |
| `TDM` | Team Deathmatch | Team frags toward shared limit; expect autobalance on public servers. |
| `DOM` | Domination | Capture and hold control points; flips grant bonus points per tick.【F:docs/domination.md†L6-L25】 |
| `CTF` | Capture the Flag | Two-base flag runs with assist/defend scoring. |
| `CA` | Clan Arena | Round-based, no item pickups; loadouts refresh each round. |
| `ONEFLAG/HAR/OVLD` | Objective CTF variants | Neutral flag runs, skull collection, or base core destruction depending on mode. |
| `FT` | Freeze Tag | Freeze enemies, thaw teammates; elimination per round. |
| `STRIKE` | CaptureStrike | Alternating offense/defense rounds with flag plants. |
| `RR` | Red Rover | Eliminated players swap teams; survive the cycle. |
| `LMS/LTS` | Last (Team) Standing | Finite lives; pace your engagements. |
| `HORDE` | Horde Mode | PvE waves with scaling difficulty—coordinate weapon drops. |
| `BALL` | ProBall | Ball carries score by running goals; focus on interceptions. |
| `GAUNTLET` | Gauntlet | Winner stays, loser to the queue; duel fundamentals matter. |

## Voting & Match Etiquette
- Use `callvote` (or shorthand `cv`) to request map, ruleset, or shuffle changes. Available options depend on `g_vote_flags`—check the Call Vote menu before spamming chat.【F:docs/g_vote_flags.md†L1-L24】【F:src/server/commands/command_voting.cpp†L665-L667】
- Servers often disable mid-match switches via `g_allow_vote_mid_game 0`; wait for intermission before proposing disruptive changes.【F:src/server/gameplay/g_main.cpp†L819-L904】
- Respect spectator ballots: some hosts allow `g_allow_spec_vote` so observers can fix imbalances without rejoining.【F:src/server/gameplay/g_main.cpp†L817-L818】

## Reconnects & Ghosts
- WORR preserves your spawn state when you disconnect unexpectedly. On reconnect, the server attempts to respawn you at the stored ghost origin/angles if the spot is safe; otherwise it falls back to a standard spawn point and alerts you to blockers.【F:src/server/gameplay/g_spawn_points.cpp†L1377-L1405】
- Use this to your advantage: if you crash mid-series, rejoin quickly so your team retains your inventory and stats.

## Quick Console Tips
- `ready`/`notready` toggle your warmup state; countdowns will not start until enough players are marked ready.【F:src/server/commands/command_client.cpp†L1291-L1295】
- `timeout`/`timein` consume the team’s pause quota; check with teammates before stopping play.【F:src/server/commands/command_client.cpp†L1301-L1302】
- `mymap <bsp>` queues your preferred map after the current match when the host enables MyMap.【F:docs/mapsystem.html†L58-L72】【F:src/server/commands/command_client.cpp†L1288-L1290】

Study the [Cvar Reference](Cvars.md) and [Command Reference](Commands.md) for deeper configuration tweaks that may affect gameplay on specific servers.
