# WORR Level Design Overview

WORR dramatically broadens Quake II Rerelease's entity vocabulary and map logic, enabling classic layouts to coexist with ambitious, multi-mode battlegrounds. This guide summarizes the most relevant systems for designers and highlights how to author rotation-ready maps for the expanded rulesets.

## Expanded Entity & Logic Support
- WORR integrates a much wider range of entities, monsters, and scripted behaviors than stock Quake II, pulling from other Quake titles and popular community mods.【F:README.md†L13-L26】 Designers can rely on richer encounter scripting, more interactive set-pieces, and broader multiplayer logic without custom DLL work.
- Multiplayer infrastructure layers modern match management on top of legacy triggers, so legacy BSPs remain compatible while new content can opt into advanced announcers, scoreboard cues, and voting hooks.【F:README.md†L31-L46】

## Gametype Metadata & Required Targets
Each gametype advertises its short name, rotation flags, and gameplay expectations through the `GAME_MODES` table compiled in the server DLL. Map authors can match spawn targets and supporting entities to the metadata:

| Gametype | Short Name | Key Flags | Required Entities & Targets |
| --- | --- | --- | --- |
| Campaign | `cmp` | None | Standard campaign spawn flow and mission scripting. |
| Free For All | `ffa` | Frags | Deathmatch player starts and weapon/item spawns tuned for fast turnover. |
| Duel | `duel` | 1v1, Frags | Tight weapon placement, mirrored item control, two initial player starts. |
| Team Deathmatch | `tdm` | Teams, Frags | Team-specific spawn targets and ammo/armor clustering to support four or more players. |
| Domination | `dom` | Teams, Frags | Three or more control points with clear sightlines and circulation routes. |
| Capture The Flag | `ctf` | Teams, CTF | Flag bases, return zones, and symmetrical resources. |
| Clan Arena | `ca` | Teams, Arena, Rounds, Elimination | Arena start pads, no respawns, weapon loadouts delivered at round start. |
| One Flag | `oneflag` | Teams, CTF | Central neutral flag target with offense/defense staging. |
| Harvester | `har` | Teams, CTF | Skull generator, obelisks, and team scoring pads. |
| Overload | `overload` | Teams, CTF | Defendable core entity with approach lanes. |
| Freeze Tag | `ft` | Teams, Rounds, Elimination | Clustered respawn statues or thaw zones to encourage team rescues. |
| CaptureStrike | `strike` | Teams, Arena, Rounds, CTF, Elimination | Spawn pods for attackers/defenders, capture pad, and round-based reset logic. |
| Red Rover | `rr` | Teams, Rounds, Arena | Shared arena spawn bank that supports team swaps mid-round. |
| Last Man Standing | `lms` | Elimination | Single-life spawn pads with immediate access to critical gear. |
| Last Team Standing | `lts` | Teams, Elimination | Team spawn groupings and cover for opening engagements. |
| Horde Mode | `horde` | Rounds | Monster spawn volumes, defense staging, and escalation triggers. |
| ProBall | `ball` | Teams | Goal volumes, ball spawn, and scoring indicators. |
| Gauntlet | `gauntlet` | 1v1, Rounds, Frags | Sequential duel arenas with audience-safe staging. |

Refer to `src/server/g_local.hpp` for the authoritative metadata, which drives rotation filters and server logic for each mode.【F:src/server/g_local.hpp†L222-L240】

## Mode-Specific Layout Guidance
Leverage existing WORR documentation when blocking out mode-focused revisions:
- **Domination:** Control points should land on high-visibility landmarks and form a circuit that minimizes stalemates. Keep travel time between points roughly equal so teams trade captures instead of turtling. Encourage multi-level fights with vertical flanks for comeback plays.【F:docs/domination.md†L1-L19】
- **Map Selector Expectations:** The voting system favors maps whose `min`/`max` player counts match the active population, have not been played recently, and advertise compatibility with the current gametype and ruleset. Distribute spawn groups and objectives so that low-pop and high-pop brackets both feel supported.【F:docs/mapsystem.html†L20-L71】【F:docs/mapsystem.html†L74-L103】

## Preparing Maps for Rotation
1. Annotate each BSP entry in the map pool JSON with accurate `min`, `max`, `gametype`, and `ruleset` metadata. These fields gate automated rotation, MyMap eligibility, and selector votes.【F:docs/mapsystem.html†L20-L71】
2. Ensure spawn targets and objective entities exist for every gametype you flag as supported. Avoid advertising unsupported modes—rotation filters will skip incomplete entries, and players will receive confusing errors.
3. When authoring new geometry, budget warmup staging areas, callvote cameras, and spectator sightlines. WORR’s match modifiers and announcer cues assume clear sightlines to objectives and spawn pads.

## Validation Checklist & Tooling
- Use JSON-aware editors or `jq` to validate map pool files before reloading them in-game.
- Reload configuration in a development server with `loadmappool` and `loadmapcycle` to catch syntax errors or missing BSPs immediately.【F:docs/mapsystem.html†L105-L126】
- Run `mappool` and `mapcycle` commands after reload to confirm that each entry appears under the intended gametype and player count brackets.
- Test MyMap compatibility by queueing the map with representative override flags (e.g., `mymap myarena +pu -ht`) and confirming the server accepts or rejects it according to your metadata.【F:docs/mapsystem.html†L48-L71】
- Maintain a staging rotation that covers each advertised gametype so QA can spawn, capture, and score through at least one full loop before publishing updates.
