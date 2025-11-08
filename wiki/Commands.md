# WORR Console Command Reference

Commands are grouped by responsibility so hosts, admins, and players can locate the verbs they need quickly. Access restrictions derive from the registration flags in the command modules—commands marked with `AdminOnly` require successful `/admin <password>` login, while `CheatProtect` follows the `cheats` cvar.【F:src/server/commands/command_client.cpp†L1256-L1314】【F:src/server/commands/command_admin.cpp†L482-L510】【F:src/server/commands/command_cheats.cpp†L206-L224】

## Player & Spectator Commands
| Command | Access | Summary |
| --- | --- | --- |
| `admin` | Spectators & intermission | Opens the admin login prompt when you know the `admin_password`.【F:src/server/commands/command_client.cpp†L1260-L1260】 |
| `clientlist` | All players | Prints connection slots and ping/state info for troubleshooting.【F:src/server/commands/command_client.cpp†L1261-L1261】 |
| `drop` / `dropindex` | Live players | Drop current weapon or a specific inventory index.【F:src/server/commands/command_client.cpp†L1262-L1264】 |
| `eyecam` | Spectators | Toggles chase camera to orbit points of interest.【F:src/server/commands/command_client.cpp†L1264-L1264】 |
| `follow`, `followkiller`, `followleader`, `followpowerup` | Dead players & spectators | Cycle chase targets during spectating.【F:src/server/commands/command_client.cpp†L1266-L1269】 |
| `forfeit` | Live players | Requests a team forfeit in elimination modes (subject to server policy).【F:src/server/commands/command_client.cpp†L1270-L1270】 |
| `help` / `inven` / `score` | All | Display usage help, inventory browser, or scoreboard without leaving the arena.【F:src/server/commands/command_client.cpp†L1271-L1297】 |
| `hook` / `unhook` | All | Engage or release the grappling hook when enabled.【F:src/server/commands/command_client.cpp†L1272-L1304】 |
| `id` | All | Toggles crosshair IDs when `match_crosshair_ids` is active.【F:src/server/commands/command_client.cpp†L1273-L1273】 |
| `invnext*` / `invprev*` / `invuse` | All | Cycle through and use inventory slots (weapons, powerups).【F:src/server/commands/command_client.cpp†L1276-L1283】 |
| `kb` | All | Toggle kill beep feedback locally.【F:src/server/commands/command_client.cpp†L1284-L1284】 |
| `kill` | Live players | Suicide command (respecting `g_allow_kill`).【F:src/server/commands/command_client.cpp†L1285-L1285】 |
| `mapcycle` / `mappool` / `mapinfo` | All | Inspect rotation contents and per-map metadata pulled from the JSON pool.【F:src/server/commands/command_client.cpp†L1286-L1289】 |
| `motd` | Spectators & intermission | Re-read the server MOTD file.【F:src/server/commands/command_client.cpp†L1289-L1289】 |
| `mymap` | All | Queue a MyMap pick when hosts enable the feature.【F:src/server/commands/command_client.cpp†L1290-L1290】 |
| `ready` / `readyup` / `ready_up` / `notready` | All | Control warmup ready state to start matches.【F:src/server/commands/command_client.cpp†L1291-L1295】 |
| `setweaponpref` | All | Persist preferred weapon ordering for spawns.【F:src/server/commands/command_client.cpp†L1296-L1297】 |
| `sr` | All | Display personal skill rating estimate used for balance.【F:src/server/commands/command_client.cpp†L1298-L1298】 |
| `stats` | Spectators & intermission | Show detailed match stats overlay.【F:src/server/commands/command_client.cpp†L1299-L1299】 |
| `team` | Dead players & spectators | Join or switch teams when allowed.【F:src/server/commands/command_client.cpp†L1300-L1300】 |
| `timeout` / `timein` / `timer` | All | Consume timeouts, resume play, or show personal timers.【F:src/server/commands/command_client.cpp†L1301-L1303】 |
| `use*` | All | Activate items by name or index (weapons, techs).【F:src/server/commands/command_client.cpp†L1305-L1308】 |
| `weapnext` / `weapprev` / `weaplast` | All | Cycle weapon inventory in either direction.【F:src/server/commands/command_client.cpp†L1309-L1312】 |
| `where` | Spectators | Print current coordinates for callouts or mapping.【F:src/server/commands/command_client.cpp†L1313-L1313】 |

## Map Voting & Governance
| Command | Access | Summary |
| --- | --- | --- |
| `callvote <topic>` / `cv` | Dead players & spectators | Start a vote for map, mode, shuffle, or ruleset as allowed by `g_vote_flags`. Syntax is topic-specific (e.g., `callvote map q2dm1`).【F:src/server/commands/command_voting.cpp†L661-L667】【F:docs/g_vote_flags.md†L1-L24】 |
| `vote <yes|no>` | Dead players | Cast a ballot on the active vote prompt.【F:src/server/commands/command_voting.cpp†L643-L667】 |

## Administrative Commands
These require a logged-in admin user (see `/admin`).

| Command | Purpose |
| --- | --- |
| `add_admin` / `remove_admin` | Manage the admin roster file on disk.【F:src/server/commands/command_admin.cpp†L484-L502】 |
| `add_ban` / `remove_ban` / `load_bans` | Maintain IP ban lists without restarting.【F:src/server/commands/command_admin.cpp†L485-L502】 |
| `arena` | Force a specific arena layout in Arena/RA2 playlists.【F:src/server/commands/command_admin.cpp†L486-L486】 |
| `balance` / `shuffle` | Trigger skill-based team balancing or random shuffles.【F:src/server/commands/command_admin.cpp†L487-L507】 |
| `boot` | Kick a player from the server (with messaging).【F:src/server/commands/command_admin.cpp†L488-L488】 |
| `end_match` / `reset_match` / `start_match` | Control match lifecycle manually (end now, wipe scores, start immediately).【F:src/server/commands/command_admin.cpp†L489-L508】 |
| `force_vote` | Override the current vote result (force pass/fail).【F:src/server/commands/command_admin.cpp†L490-L490】 |
| `gametype` / `ruleset` | Switch active gametype or ruleset instantly.【F:src/server/commands/command_admin.cpp†L491-L504】 |
| `load_mappool` / `load_mapcycle` / `load_motd` | Reload map or MOTD assets from disk after edits.【F:src/server/commands/command_admin.cpp†L494-L496】 |
| `lock_team` / `unlock_team` | Prevent or allow team joins for specific colors.【F:src/server/commands/command_admin.cpp†L497-L510】 |
| `map_restart` / `next_map` / `set_map` | Restart the current map, advance to cycle entry, or load a specific BSP.【F:src/server/commands/command_admin.cpp†L498-L505】 |
| `ready_all` / `unready_all` | Force lobby participants into (or out of) ready state.【F:src/server/commands/command_admin.cpp†L500-L510】 |
| `set_team` | Move a named player to a team slot manually.【F:src/server/commands/command_admin.cpp†L505-L506】 |

## Cheat & Development Utilities
These commands are guarded by the `cheats` cvar and primarily support offline testing.

| Command | Summary |
| --- | --- |
| `give`, `god`, `immortal`, `noclip`, `notarget`, `teleport` | Standard Quake-style cheat toggles for inventory, invulnerability, movement, and relocation.【F:src/server/commands/command_cheats.cpp†L209-L223】 |
| `alert_all` | Broadcast a center-print message to all players for announcements.【F:src/server/commands/command_cheats.cpp†L209-L209】 |
| `check_poi` / `set_poi` / `target` | Inspect and manipulate saved points-of-interest for scripted sequences.【F:src/server/commands/command_cheats.cpp†L210-L223】 |
| `clear_ai_enemy` / `kill_ai` | Reset or slay targeted AI actors while debugging encounters.【F:src/server/commands/command_cheats.cpp†L211-L215】 |
| `list_entities` / `list_monsters` | Dump active entity or monster tables to the console with optional filtering.【F:src/server/commands/command_cheats.cpp†L216-L217】 |
| `novisible` | Toggle visibility checks for testing stealth and fog volumes.【F:src/server/commands/command_cheats.cpp†L220-L220】 |

## Tips for Hosts
- Combine admin commands with matching cvar changes (e.g., `set g_vote_flags 1535`) to ensure your policy updates take effect immediately.
- Document any non-default command flows (like forcing `/forfeit` in tournaments) in match rules so players understand expectations.
- Pair this reference with the [Cvar Guide](Cvars.md) when adjusting rules mid-event to avoid conflicting settings.
