# Level Designer Toolkit

WORR dramatically expands Quake II’s entity vocabulary, borrowing movers, targets, and monsters from classic Quake titles and popular mods. Maps can opt into curated rulesets, gametype-specific spawn logic, and JSON metadata so rotations stay responsive to player counts.【F:README.md†L13-L47】【F:docs/mapsystem.html†L17-L104】

## Map Metadata & Pool Integration
- Declare map capabilities in the JSON map pool referenced by `g_maps_pool_file`. Required key: `bsp`; optional flags advertise supported gametypes (`dm`, `tdm`, `ctf`, `duel`), player counts, and overrides such as `ruleset`, `scorelimit`, or `timelimit`.【F:docs/mapsystem.html†L24-L47】
- Use metadata to gate selectors: maps marked `popular` or `custom` influence MyMap selection bias, while `min`/`max` prevent low- or high-pop lobbies from voting unsuitable layouts.【F:docs/mapsystem.html†L24-L84】
- Store the pool JSON under `baseq2/` so live servers can reload it with `load_mappool` without restarting.【F:docs/mapsystem.html†L24-L97】【F:src/server/commands/command_admin.cpp†L494-L496】

## Worldspawn & Ruleset Hooks
- WORR filters entities via `gametype` / `not_gametype` and `ruleset` keys during spawn. Match the values listed in `GAME_MODES` (e.g., `domination`, `gauntlet`) so objects only appear for the intended modes.【F:src/server/g_local.hpp†L222-L305】【F:src/server/gameplay/g_spawn.cpp†L1354-L1410】
- Use `spawnpad` tokens (`pu`, `ar`, `ht`) on entity overrides to tie items to map flags toggled through MyMap and selector settings.【F:src/server/gameplay/g_spawn.cpp†L1379-L1406】
- Designers can ship alternate entity sets in `maps/<bspname>/entities.ent`. Toggle loading or persistence with `g_entity_override_dir`, `g_entity_override_load`, and `g_entity_override_save`.【F:src/server/gameplay/g_main.cpp†L855-L857】

## Player Spawns & Nav Points
- Provide baseline deathmatch spawns (`info_player_deathmatch`) plus team spawns (`info_player_team_red`/`blue`) for team modes. Include `info_player_intermission` viewpoints for post-match cameras.【F:src/server/gameplay/g_spawn.cpp†L49-L60】
- `info_landmark` and `info_world_text` unlock cross-map transitions and world text overlays used in campaign conversions.【F:src/server/gameplay/g_spawn.cpp†L56-L60】
- Navigation helpers (`path_corner`, `point_combat`, `info_nav_lock`) assist AI and bot pathing in cooperative or Horde scenarios.【F:src/server/gameplay/g_spawn.cpp†L59-L193】

## Movers, Triggers, and Logic
- WORR keeps all Quake II movers—`func_door`, `func_plat`, `func_train`—and backports extras like `func_rotating_ext`, `func_rotate_train`, `func_bobbing`, and `func_pendulum` for dynamic set pieces.【F:src/server/gameplay/g_spawn.cpp†L62-L89】
- Trigger logic includes campaign staples (`trigger_once`, `trigger_multiple`, `trigger_hurt`) plus Quake III/Live style relays such as `trigger_fog`, `trigger_misc_camera`, and `trigger_safe_fall`.【F:src/server/gameplay/g_spawn.cpp†L91-L116】
- Target entities span explosions, scripted camera paths, achievements, autosaves, light ramps, music cues, and Quake III style `target_print` or `target_cvar` interactions for runtime configuration tweaks.【F:src/server/gameplay/g_spawn.cpp†L117-L166】
- Projectile launchers (`target_shooter_*`, `trap_shooter`, `trap_spikeshooter`) enable automated turrets or hazard scripting.【F:src/server/gameplay/g_spawn.cpp†L168-L178】

## Environmental Dressing & Storytelling
- Dynamic lighting (`dynamic_light`, `rotating_light`), holograms, banners, and cinematic transports (`misc_transport`, `misc_blackhole`) are available for atmosphere.【F:src/server/gameplay/g_spawn.cpp†L181-L224】
- Domination-specific points use the `domination_point` classname—place at least three in symmetric layouts so both teams have routes to contest.【F:src/server/gameplay/g_spawn.cpp†L195-L203】【F:docs/domination.md†L3-L21】
- Use `misc_world_text` and `target_story` to surface narrative beats in cooperative conversions without modifying BSP geometry.【F:src/server/gameplay/g_spawn.cpp†L59-L147】

## Monsters & AI Encounters
- WORR supports the full Quake II roster plus mission-pack additions and classic Quake bestiary (Shamblers, Fiends, Ogres). Place them using `monster_*` classnames shown in the spawn registry.【F:src/server/gameplay/g_spawn.cpp†L229-L310】
- Turrets (`turret_breach`, `turret_invisible_brain`) and boss actors (`monster_makron`, `monster_chthon`) can anchor set-piece fights. Validate health scaling with cooperative cvars like `g_coop_health_scaling` when testing difficulty.【F:src/server/gameplay/g_spawn.cpp†L229-L310】【F:src/server/gameplay/g_main.cpp†L738-L826】

## Mode-Specific Considerations
- **Domination:** Control points should sit in sight lines that encourage rotation; pair each point with at least two approach routes and ensure spawn groups are equidistant.【F:docs/domination.md†L3-L21】
- **Arena / Clan Arena:** Flag weapon and item spawns with `spawnpad` or `not_gametype` filters so loadout-based rounds stay clean.【F:src/server/gameplay/g_spawn.cpp†L1354-L1406】
- **Horde:** Combine `monster_*` waves with `trigger_spawn` and `target_spawner` logic to schedule escalating attacks while respecting `g_horde_starting_wave`.【F:src/server/gameplay/g_spawn.cpp†L117-L176】【F:src/server/gameplay/g_main.cpp†L724-L736】

## Custom Assets & Packaging
- Allow bespoke textures and sounds by keeping `g_maps_allow_custom_textures` and `g_maps_allow_custom_sounds` enabled. Bundle external resources alongside the BSP or in PKZ archives for distribution.【F:src/server/gameplay/g_main.cpp†L913-L914】
- Reference any asset dependencies in release notes so server hosts pre-load content before rotating your map.

## Validation Workflow
1. Compile the BSP and test with `developer 1` to surface console warnings.
2. Run through each intended gametype, verifying spawn filters (`gametype`, `not_gametype`, `ruleset`) behave as expected.
3. Execute `load_mappool` and `load_mapcycle` to confirm JSON metadata exposes your map to selectors and MyMap queues.【F:src/server/commands/command_admin.cpp†L494-L496】
4. Capture analytics-enabled sessions to ensure `domination_point` scoring or scripted encounters feed match stats cleanly.【F:docs/match_stats_json.md†L6-L29】

Document entity usage and override files alongside your source `.map` so future iterations understand intentional filters and scripting choices.
