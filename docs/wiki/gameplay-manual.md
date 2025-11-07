# WORR Gameplay Manual

## Gametype reference
The server enumerates every multiplayer mode in a single `GAME_MODES` table. Each entry lists the console short name, the long display name, the spawn alias used by map scripts, and the structural flags that describe how the mode behaves (teams, rounds, elimination, capture mechanics, and so on).【F:src/server/g_local.hpp†L210-L239】 The flag meanings are:

- `Teams` – requires team assignments before players can join the action.
- `CTF` – uses flag- or objective-capture scoring rules.
- `Arena` – loads self-contained arenas instead of the base map flow.
- `Rounds` – restarts play in discrete rounds.
- `Elimination` – removes players until a round ends.
- `Frags` – traditional frag-based scoring.
- `OneVOne` – enforces a duel-ready 1v1 roster.【F:src/server/g_local.hpp†L185-L193】

| Long name | Console aliases | Format flags | Notes |
|-----------|-----------------|--------------|-------|
| Campaign | `cmp`, spawn alias `campaign` | — | Story-driven co-op baseline used for testing. |
| Free For All | `ffa`, spawn alias `ffa` | Frags | Every player scores independently. |
| Duel | `duel`, spawn alias `tournament` | OneVOne, Frags | 1v1 ladder with frag scoring. |
| Team Deathmatch | `tdm`, spawn alias `team` | Teams, Frags | Classic team frag battle. |
| Domination | `dom`, spawn alias `domination` | Teams, Frags | Tick-based territory control; points accumulate like frags. |
| Capture The Flag | `ctf`, spawn alias `ctf` | Teams, CTF | Traditional two-flag CTF. |
| Clan Arena | `ca`, spawn alias `ca` | Teams, Arena, Rounds, Elimination | Round-reset arena fights with no pickups mid-round. |
| One Flag | `oneflag`, spawn alias `1f` | Teams, CTF | Teams fight over a shared neutral flag. |
| Harvester | `har`, spawn alias `har` | Teams, CTF | Skulls drop on frags and must be run home. |
| Overload | `overload`, spawn alias `obelisk` | Teams, CTF | Attack/defend the enemy obelisk. |
| Freeze Tag | `ft`, spawn alias `freeze` | Teams, Rounds, Elimination | Frozen teammates must be thawed between rounds. |
| CaptureStrike | `strike`, spawn alias `strike` | Teams, Arena, Rounds, CTF, Elimination | Alternating attack/defend rounds with objective captures. |
| Red Rover | `rr`, spawn alias `rr` | Teams, Arena, Rounds | Swapping lineups each round keeps both teams active. |
| Last Man Standing | `lms`, spawn alias `lms` | Elimination | Solo lives are limited per round. |
| Last Team Standing | `lts`, spawn alias `lts` | Teams, Elimination | Team-based elimination race. |
| Horde Mode | `horde`, spawn alias `horde` | Rounds | Cooperative monster waves. |
| ProBall | `ball`, spawn alias `ball` | Teams | Score by carrying and throwing the ball. |
| Gauntlet | `gauntlet`, spawn alias `gauntlet` | OneVOne, Rounds, Frags | Duel ladder that restarts each round. |

## Baseline rule presets and warmup flow
Core defaults live in `gt_rules_t`. Each mode inherits an 8-second weapon respawn timer (0 means weapon stay), holdables and powerups enabled, a 40-point score limit, 10-minute timelimit, spawn bonus health, and a ready-up threshold of 51% of humans unless the row overrides those values.【F:src/server/match/match_state.cpp†L239-L266】 Specific adjustments include:

- **Duel** – 30-second weapon respawns, holdables and powerups disabled, no score limit cap so matches end purely on timelimit or admin control.【F:src/server/match/match_state.cpp†L249-L254】
- **Team Deathmatch & Domination** – same 30-second respawns but raise the score limit to 100 and timelimit to 20 minutes for extended team play.【F:src/server/match/match_state.cpp†L251-L253】
- **Capture the Flag** – retains 30-second weapon respawns while using the default 10-minute timelimit and capture-based scoring.【F:src/server/match/match_state.cpp†L253-L254】
- **ProBall** – enables weapon stay (`0`), disables holdables/powerups, sets a 10-goal limit, 15-minute clock, removes the spawn bonus, and requires 60% of humans to ready up.【F:src/server/match/match_state.cpp†L264-L265】

Regardless of preset, the match layer resets world state before warmup: techs and flags are cleared, monsters and leftover projectiles are removed, dropped items immediately think, powerups either despawn into Quad Hog or reappear after a randomized 30–60 second delay, and any hidden spawn logic resumes.【F:src/server/match/match_state.cpp†L300-L359】 Players are respawned with grapples reset, elimination flags cleared, scores optionally wiped, limited lives restored, and everyone set back to “not ready.”【F:src/server/match/match_state.cpp†L364-L402】

Warmup begins unless `warmup_enabled` is off, in which case the match jumps straight to `In_Progress`. Otherwise the reset broadcasts a fresh lobby state and waits for ready-up triggers.【F:src/server/match/match_state.cpp†L1303-L1341】 Ready-up enforcement checks a stack of modifiers: it ignores the process when `warmup_doReadyUp` is disabled, blocks start until the minimum player count is satisfied, lets bots auto-start when `match_startNoHumans` allows it, and starts once the percentage of ready humans meets `g_warmup_ready_percentage`.【F:src/server/match/match_state.cpp†L1427-L1467】 Admin shortcuts (`ready_all`/`unready_all`) simply flip those flags for every active client.【F:src/server/commands/command_admin.cpp†L288-L294】【F:src/server/commands/command_admin.cpp†L472-L477】

Score and time caps pull from gametype-aware cvars. Domination falls back to `fraglimit`, round-based modes use `roundlimit`, Capture the Flag and ProBall consult `capturelimit`, and everything else uses the frag limit. The user-facing label (“frag”, “round”, “goal”, etc.) adapts with the same logic.【F:src/server/match/match_state.cpp†L1946-L1967】 During live play `CheckDMExitRules` layers in additional modifiers: it enforces timelimit and optional overtime (`match_doOvertime`), triggers sudden death, respects mercylimit gaps, watches for Horde-specific win/lose rules, and can auto-balance or even terminate matches when player-count or team-balance cvars demand it.【F:src/server/match/match_state.cpp†L1981-L2160】 Marathon mode extends these checks across multiple maps, carrying over match IDs, elapsed time, and per-player or per-team score deltas while honoring optional marathon-specific score/time limits.【F:src/server/match/match_state.cpp†L35-L165】

## Game modifiers
The match layer exposes a handful of latched “modifier” cvars that reskin core gameplay. Because most are marked `CVAR_LATCH`, change them during warmup and follow up with `reset_match` so the new preset can rebuild the arena.【F:src/server/match/match_state.cpp†L2303-L2314】【F:src/server/commands/command_admin.cpp†L346-L356】 Key options include:

- **InstaGib (`g_instaGib`, optional `g_instagib_splash`)** – Forces every spawn to railgun-only combat with infinite slugs, hides most HUD panels, strips map weapon precaches, and can add a harmless splash burst to rails for movement tricks.【F:src/server/match/match_state.cpp†L2303-L2305】【F:src/server/player/p_client.cpp†L2058-L2061】【F:src/server/gameplay/g_spawn.cpp†L2509-L2534】【F:src/server/player/p_hud_main.cpp†L1333-L1339】【F:src/server/gameplay/g_weapon.cpp†L1024-L1040】
- **NadeFest (`g_nadeFest`)** – Swaps the universal loadout to endless grenades and trims the HUD for the throw-fest just like InstaGib does.【F:src/server/match/match_state.cpp†L2308-L2310】【F:src/server/player/p_client.cpp†L2061-L2064】【F:src/server/player/p_hud_main.cpp†L1333-L1339】
- **Weapons Frenzy (`g_frenzy`)** – Lets players holster instantly and continuously regenerates ammo in two-second pulses across the entire weapon table, keeping firefights relentless without touching infinite ammo.【F:src/server/match/match_state.cpp†L2309-L2312】【F:src/server/player/p_weapon.cpp†L345-L349】【F:src/server/player/p_view.cpp†L1276-L1322】
- **Vampiric Damage (`g_vampiric_damage` and tuning cvars)** – Award self-healing on every damaging hit up to a configurable cap, then bleed off excess health each second once players climb above the expiration threshold so “blood is fuel” but not permanent.【F:src/server/match/match_state.cpp†L2310-L2314】【F:src/server/gameplay/g_combat.cpp†L930-L938】【F:src/server/player/p_view.cpp†L1389-L1401】
- **Quad Hog (`g_quadhog`)** – Turns Quad Damage into a rotating objective: only one can exist, it respawns at random DM pads, and anyone holding it counts as hostile for friendly-fire checks until the timer resets.【F:src/server/match/match_state.cpp†L2306-L2309】【F:src/server/gameplay/g_items.cpp†L1037-L1144】【F:src/server/gameplay/g_combat.cpp†L437-L451】
- **Marathon (`marathon`, `g_marathon_timelimit`, `g_marathon_scorelimit`)** – Chains several maps into one extended leg, carrying scores and elapsed time forward and auto-advancing once the per-leg limit triggers.【F:src/server/gameplay/g_main.cpp†L779-L789】【F:src/server/match/match_state.cpp†L35-L165】
- **Gravity shuffles (`g_gravity` and `target_gravity`)** – Adjust gravity mid-season through admin cvar changes or scripted triggers; the server watches the `g_gravity` cvar every frame and applies any updates broadcast by map entities such as `target_gravity`, enabling “Gravity Lotto” rotations without a restart.【F:src/server/gameplay/g_main.cpp†L1710-L1718】【F:src/server/gameplay/g_target.cpp†L1487-L1501】

## Switching modes and combining modifiers
Server operators change structure mid-session with the admin command suite. Use `gametype <name>` to switch to any short or long mode name from the table above (for example, `gametype strike`), and `ruleset <alias>` to swap between the SLIPGATE/ STROYENT/ VADRIGAR balance presets (`q1`, `q2`, or `q3`).【F:src/server/commands/command_admin.cpp†L209-L375】【F:src/server/g_local.hpp†L106-L118】 After altering either lever, follow up with `reset_match` if a round is already underway so the new configuration can rebuild world state using the fresh presets.【F:src/server/commands/command_admin.cpp†L346-L356】 When you need to coordinate players around warmup changes, `ready_all` and `unready_all` remain available overrides.【F:src/server/commands/command_admin.cpp†L288-L294】【F:src/server/commands/command_admin.cpp†L472-L477】 See the [Server Administration Manual](server-admin.md) for syntax examples and escalation tips.

The modifiers discussed above stack cleanly: gametype flags define which score limit cvar applies, rulesets change the underlying physics or item settings, and Marathon or warmup toggles layer on top. Always confirm the resulting state with the HUD (`GT_ScoreLimit`, timelimit, and match warnings) before starting competitive rounds so players know which victory condition is live.【F:src/server/match/match_state.cpp†L1946-L2160】

## Gameplay balance quick reference
While gametypes and modifiers define structure, WORR also layers a global balance package that modernizes Quake II’s sandbox. The highlights below match the dedicated balance changelog and help admins anticipate how rulesets feel in play.【F:docs/gameplay.html†L1-L117】

- **Weapons** – Rockets fly faster and hit for a flat 100 damage, plasma beams fade after 768 units with softened damage/knockback, Hunter/Vengeance spheres crawl slower for tighter counterplay, and bullet weapons share the Quake III cone-spread implementation (with heavier vertical kick on the machinegun/chaingun family).【F:docs/gameplay.html†L21-L41】
- **Knockback** – Radial push uses Quake III logic, creating predictable rocket jumps and splash displacements across all rulesets.【F:docs/gameplay.html†L43-L45】
- **Ammo economy** – Stockpile caps and pickup amounts differ by ammo type, with Bandolier/Ammo Pack capacities clearly documented; the two carry items also respawn twice as fast (30s/90s) to keep teams supplied.【F:docs/gameplay.html†L47-L71】
- **Health curve** – Players spawn with a decaying +25 stack, Megas keep their timer through multiple grabs, Stimpacks heal for 3, and Adrenaline becomes a reusable holdable (adding +5 max health outside Duel).【F:docs/gameplay.html†L75-L82】
- **Armor and techs** – Power armor now absorbs a fixed 1 damage per cell and Auto Doc alternates armor/health regeneration after a one-second delay.【F:docs/gameplay.html†L84-L94】
- **Powerups** – All major powerups respawn on a 120-second timer with a 30–60 second first-spawn delay, can be dropped on death, and feature refreshed behavior: Haste grants +50% fire rate/move speed, Battle Suit nullifies splash and halves direct damage, Regeneration is imported from Quake III, and Invisibility is stealthier with global spawn/pickup cues.【F:docs/gameplay.html†L96-L115】
- **Mode flavor** – Domination’s point logic mirrors the fraglimit/timelimit pairing described earlier, rewarding capture flips and ticking points while owned.【F:docs/gameplay.html†L117-L125】

Taken together, these adjustments reinforce the same pacing across modes: more reliable projectile physics, faster item loops, and a health economy that encourages map control without making snowballs unstoppable. Pair the section above with the SLIPGATE/STROYENT/VADRIGAR rulesets when communicating expectations to players joining mid-rotation.【F:docs/gameplay.html†L21-L125】
