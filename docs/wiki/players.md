# Player Guide

## Movement and Combat Updates
- **Rocket Launcher:** Faster rockets (800 UPS vs. 650) with fixed 100 damage tighten timing for splash plays and air denial.
- **Plasma Beam:** The beam now stops at 768 units, deals 8 damage per tick (down from 15), and barely pushes targets (knockback 8). Expect closer duels and less juggling.
- **Hunter & Vengeance Spheres:** Slower travel (300 UPS) and shorter 10-second lifespan reduce map-wide pressure; plan ambushes carefully.
- **Bullet Weapons:** Machinegun and Chaingun scatter more vertically, and the shotgun family now mirrors Quake III’s cone spread—burst fire to stay accurate.
- **Knockback Model:** Radial damage now follows Quake III physics, making rocket and grenade jumps feel predictable; rehearse angles knowing the launch math stays consistent.
- **Ammo Economy:** Expect larger reserves on bullets, flechettes, and cells plus halved respawn times for Bandoliers (30s) and Ammo Packs (90s). Aggressive players can keep pressure with mindful pickup routes.
- **Health Flow:** +25 bonus spawn health decays to 100, Mega Health timers stack, and Stimpacks heal 3. Adrenaline is holdable outside duels and adds +5 max health—stash it before big pushes.
- **Power Armor & Powerups:** Power Armor always soaks 1 damage per cell. Haste grants +50% rate of fire and speed, Battle Suit halves direct hits and blocks splash, Regeneration heals over time, and all major powerups announce spawns/pickups globally to fuel fights.

## UI and Feedback
- **Adaptive HUD:** The default overlay surfaces only the essentials you need mid-fight while adapting to context so situational awareness stays high.
- **Score Awareness:** Mini-score displays and frag messages sit in the HUD, keeping streaks and team status visible without chat scrolling.
- **Match Announcer:** Key events—from powerup grabs to lead changes—play through a dynamic announcer layer, reinforcing callouts for players without voice comms.

## Gametype Cheat Sheet
| Mode | Console Short Name | Core Focus & Notes | Quick Reference |
| --- | --- | --- | --- |
| Campaign | `cmp` | Solo/co-op progression with standard objectives. | — |
| Free For All | `ffa` | Classic deathmatch; highest frag count wins. | — |
| Duel | `duel` | 1v1 spotlight with frag scoring—great for warm-ups. | — |
| Team Deathmatch | `tdm` | Teams race to frag limit; communication decides trades. | — |
| Domination | `dom` | Capture and hold control points for continuous scoring; flip bonuses reward aggression. | [Domination guide](../domination.md) |
| Capture The Flag | `ctf` | Flag runs and returns define the pace. | — |
| Clan Arena | `ca` | Round-based team elimination with arena loadouts. | — |
| One Flag | `oneflag` | Shared neutral flag—coordinate escorts both ways. | — |
| Harvester | `har` | Collect enemy skulls and score at your base. | — |
| Overload | `overload` | Destroy the enemy obelisk to score; defend your own. | — |
| Freeze Tag | `ft` | Freeze foes instead of fragging; thaw teammates to win rounds. | — |
| CaptureStrike | `strike` | Hybrid round play with capture objectives and elimination stakes. | — |
| Red Rover | `rr` | Swapping sides on death keeps pressure constant; manage spawns. | — |
| Last Man Standing | `lms` | Everyone gets limited lives—play cautiously. | — |
| Last Team Standing | `lts` | Team variant of LMS; protect allies’ life pools. | — |
| Horde Mode | `horde` | Survive PvE waves with periodic breaks. | — |
| ProBall | `ball` | Team sports twist—control the ball to score. | — |
| Gauntlet | `gauntlet` | Duel-centric rounds emphasizing precision frags. | — |

> **Tip:** Loadouts, scoring rules, and round flow come from the flags defined in `GameTypeInfo`. The console short name is the value to feed `g_gametype` or the Call Vote menu. Consult mode-specific docs (e.g., Domination above) when a lobby needs deeper strategy, and map authors can reference the [Entity Catalogue](level-design.md#entity-catalogue) to double-check spawn and objective coverage before sharing workshop builds.

## Voting and Match Etiquette
- Want the complete syntax for `callvote`, ready commands, or spectator tools? Check the [Command Reference](commands.md) for detailed usage notes to share with teammates before calling for action.
- **How votes unlock:** Server hosts mask vote commands with `g_vote_flags`. Each bit toggles an item (map, restart, gametype, limits, shuffle/balance, unlagged, arena, etc.). Default servers enable all flags (`8191`), showing every option.
- **Reading the menu:** If a choice is missing, the host likely cleared that bit—ask before spamming chat, and remember hosts may restrict disruptive calls like `shuffle` during events.
- **Etiquette:** Call votes with a clear reason (“Restart for overtime bugged scoreboard”) and allow the countdown to finish. Avoid chaining failed votes; wait a few minutes or until the match state changes. Hosts appreciate players rallying support in team chat before triggering global prompts.
- **Want to know the knobs?** The [Cvar Reference](cvars.md#voting-admin-and-moderation) breaks down which server toggles influence the vote menu, warmup readiness, and timeout privileges.

## Reconnect Safeguards and Ghost Tips
- **Pending Ghost Respawns:** WORR caches your last position/angles when the auto-ghost system is active. On rejoin, it first checks if your ghost spot is clear of players or geometry before spawning you there.
- **Blocked Ghosts:** If the spot is occupied (player or map brush), the server denies the ghost spawn, logs what blocked you, and reverts to regular spawn logic. Expect to appear at the next safe point instead.
- **Quick reconnect routine:** Rejoin promptly, wait a second for the ghost check, and if you spawn elsewhere, regroup—your inventory and stats still restore thanks to the ghost system’s state persistence.
