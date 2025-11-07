# Level Design Manual

## Domination Control Points
- Domination maps place multiple capturable locations. Standing uncontested inside a control point charges a short progress meter, flips the point to your team when full, and broadcasts the capture change before it starts generating score.【F:docs/domination.md†L1-L6】
- Each server tick awards one point per control point owned, so simultaneous pressure on multiple sites is the fastest path to victory.【F:docs/domination.md†L8-L9】
- Capturing a point also grants an immediate bonus payout, encouraging aggressive swings even when defenders may retake the position moments later.【F:docs/domination.md†L9-L10】
- Domination ends when a team reaches `fraglimit` or the configured `timelimit`; standard overtime rules extend tied games. WORR defaults recommend `fraglimit 100` and a 20 minute `timelimit` for classic pacing.【F:docs/domination.md†L10-L17】
- The scoreboard’s “PT” column tracks total team points while the ticker shows live scoring, giving playtesters quick visual feedback when verifying control point behavior.【F:docs/domination.md†L21-L22】

## Worldspawn Configuration Keys
Use the following `worldspawn` keys to tune level-wide behavior. All values are optional; omit a key to accept the server defaults.

| Key | Purpose |
| --- | --- |
| `sky`, `skyAxis`, `skyRotate` | Configure the skybox texture and optional rotation axis/speed for map-wide backdrops.【F:src/server/gameplay/g_spawn.cpp†L2338-L2341】【F:src/server/gameplay/g_spawn.cpp†L2413-L2420】
| `sounds` / `music` | Select a CD track or explicit audio file for ambient music. The `music` key overrides `sounds` when present.【F:src/server/gameplay/g_spawn.cpp†L2341-L2343】【F:src/server/gameplay/g_spawn.cpp†L2422-L2431】
| `loop_count` | Explicitly control soundtrack looping; otherwise single-player maps default to loopless playback and multiplayer to standard looping.【F:src/server/gameplay/g_spawn.cpp†L2431-L2438】
| `gravity` | Override default 800 u/s² gravity. The engine also updates `g_gravity` so designers can quickly sanity-check physics in dev builds.【F:src/server/gameplay/g_spawn.cpp†L2448-L2456】
| `message` | Supplies the long-form level name shown to players, sanitized to remove unsafe characters.【F:src/server/gameplay/g_spawn.cpp†L2256-L2317】
| `author`, `author2` | Credit one or two creators in the loading screen metadata.【F:src/server/gameplay/g_spawn.cpp†L2331-L2335】
| `hub_map` | Marks the level as a campaign hub and resets coop help prompts when loaded.【F:src/server/gameplay/g_spawn.cpp†L2330-L2335】【F:src/server/gameplay/g_spawn.cpp†L2393-L2406】
| `start_items` | Grant a predefined item string to every player as they spawn.【F:src/server/gameplay/g_spawn.cpp†L2335-L2337】【F:src/server/gameplay/g_spawn.cpp†L2442-L2446】
| `no_grapple` | Disables the grappling hook for all players.【F:src/server/gameplay/g_spawn.cpp†L2337-L2338】【F:src/server/gameplay/g_spawn.cpp†L2446-L2448】
| `no_dm_spawnpads` | Prevents deathmatch spawn pads from appearing, useful when the layout already provides bespoke respawn markers.【F:src/server/gameplay/g_spawn.cpp†L2338-L2339】【F:src/server/gameplay/g_spawn.cpp†L2448-L2452】
| `no_dm_telepads` | Suppresses teleporter pads in deathmatch variants when the designer prefers raw trigger volumes.【F:src/server/gameplay/g_spawn.cpp†L2339-L2340】【F:src/server/gameplay/g_spawn.cpp†L2452-L2454】
| `ruleset` | Force Quake 1/2/3A balance rules, overriding the server’s default ruleset selection.【F:src/server/gameplay/g_spawn.cpp†L2339-L2340】【F:src/server/gameplay/g_spawn.cpp†L2383-L2393】
| `instantItems` | Flag levels that should instantly equip pickups (N64-styled encounters).【F:src/server/gameplay/g_spawn.cpp†L2325-L2327】【F:src/server/gameplay/g_spawn.cpp†L2438-L2442】

### Gameplay Override Flags
`ApplyMapSettingOverrides` merges server cvars and map-authored overrides into a single decision about which item classes spawn. Level designers can rely on the following priority:

1. Server cvars such as `g_no_powerups`, `g_no_health`, `g_mapspawn_no_bfg`, and `match_weaponsStay` set the baseline for every map.【F:src/server/gameplay/g_spawn.cpp†L2168-L2175】
2. Map-specific enable/disable bits (`MAPFLAG_*`) then force categories on or off, ensuring critical pickups like armor, powerups, BFGs, or plasma beams appear exactly when the layout demands.【F:src/server/gameplay/g_spawn.cpp†L2179-L2197】【F:src/server/gameplay/g_spawn.cpp†L2201-L2215】
3. Entity spawn logic checks the resolved flags. For example, entities tagged `powerups_on` or `spawnpad` types automatically inhibit themselves if the combined rules disable their resource.【F:src/server/gameplay/g_spawn.cpp†L1362-L1410】

## Optional Systems and Server Interaction

### Entity Override Files
- The loader automatically looks for `<overrideDir>/<mapname>.ent` files when `g_entityOverrideLoad` is enabled, defaulting to `baseq2/maps`. Valid overrides replace the compiled entity string after passing syntax verification and a 256 KB safety cap.【F:src/server/gameplay/g_spawn.cpp†L1724-L1769】
- Use `g_entityOverrideSave` to write the current entity lump to disk the first time a map without an override loads, making it easy to bootstrap iterative edits from a running server.【F:src/server/gameplay/g_spawn.cpp†L1779-L1789】
- Designers should coordinate with server ops: if overrides are disabled the baked BSP entities will always win, so gameplay-critical adjustments (item swaps, scripted fixes) must live in the published map instead.【F:src/server/gameplay/g_spawn.cpp†L1724-L1789】

### Spawn Pads
- `CreateSpawnPad` only materializes the pad geometry when the match allows them, the map is not an arena or N64 variant, and `match_allowSpawnPads` remains enabled server-side. Expect pads to vanish in competitive playlists that forbid them.【F:src/server/gameplay/g_utilities.cpp†L1429-L1450】
- Maps can request conditional pads by setting the `spawnpad` key on spawn entities. Values like `pu`, `ar`, or `ht` ensure the pad is inhibited if powerups, armor, or health are disabled through cvars or map overrides (including `g_vampiric_damage`).【F:src/server/gameplay/g_spawn.cpp†L1394-L1410】
- The `worldspawn` `no_dm_spawnpads` flag forces pads off in deathmatch, even if the server normally allows them, so only enable it when the layout already guarantees safe respawn footprints.【F:src/server/gameplay/g_spawn.cpp†L2338-L2339】【F:src/server/gameplay/g_spawn.cpp†L2448-L2452】

### Teleporter Pads
- Teleporters (`misc_teleporter`) spawn with a disc pad by default; setting custom `mins`/`maxs` removes the pad so designers can supply bespoke touch volumes instead.【F:src/server/gameplay/g_misc.cpp†L1590-L1637】
- Destination markers (`misc_teleporter_dest`) call `CreateSpawnPad`, so the same server checks (`match_allowSpawnPads`) and map-level toggles (`no_dm_spawnpads`) determine whether arrival pads appear.【F:src/server/gameplay/g_misc.cpp†L1656-L1665】【F:src/server/gameplay/g_utilities.cpp†L1429-L1450】
- Use the `worldspawn` `no_dm_telepads` flag when deathmatch flow should omit teleporter pads entirely; coordinate with admins if playlists rely on the global teleporter pad cvar before shipping the map.【F:src/server/gameplay/g_spawn.cpp†L2339-L2340】【F:src/server/gameplay/g_spawn.cpp†L2452-L2454】

## Entity Reference (New & Updated)
The following entities extend the classic Quake II set. Use them to communicate rules, tune ambience, or script encounters while respecting server overrides noted below.

### Info & Utility Helpers
- **`info_world_text`** – Projects floating or billboarded copy in-world. You can drive colors through the `sounds` index, size via `radius`, and optionally fire additional targets or clean up after a single use; a leaderboard mode injects ready-up prompts or leader callouts during deathmatch.【F:src/server/gameplay/g_misc.cpp†L1820-L1943】
- **`info_nav_lock`** – Toggles the `FL_LOCKED` state on targeted doors. Requires both a `targetname` and `target`, then hides itself while exposing the `use` handler for scripted locks/unlocks.【F:src/server/bots/bot_utils.cpp†L382-L412】

### Visual Dressing & Debug Aids
- **`misc_flare`** – Spawns the N64-style flare sprite with optional RGB shell tint, custom image, and fade distances; if targeted it toggles visibility so gameplay scripts can accentuate events.【F:src/server/gameplay/g_misc.cpp†L1668-L1723】
- **`misc_hologram`** – Emits a rotating translucent dropship hologram that gently flickers to sell sci-fi POIs without collisions.【F:src/server/gameplay/g_misc.cpp†L1726-L1745】
- **`misc_player_mannequin`** – Plants an idle player model for scale or cinematic beats. Designers can choose the model family, weapon prop, gesture to play when triggered, and override scale (or let the `ai_model_scale` cvar force a global debug size).【F:src/server/gameplay/g_misc.cpp†L1947-L2112】

### Navigation & Flow Control
- **`target_poi`** – Declares single or teamed navigation beacons that campaign HUDs highlight. Use `count` to gate by stage, `style` to order teammates, and spawn flags such as `NEAREST` or `DYNAMIC` for context-sensitive routing; deathmatch auto-prunes the entity so arenas stay clean.【F:src/server/gameplay/g_target.cpp†L1695-L1919】
- **`target_music`**, **`target_story`**, and **`target_achievement`** – Swap CD tracks, toggle story HUD text, or broadcast achievements when triggered—handy for campaign beats that maps should not hardcode through cvars.【F:src/server/gameplay/g_target.cpp†L1921-L2171】
- **`target_crossunit_trigger` / `target_crossunit_target`** – Record persistent campaign progress within a unit. Designers can burn trigger bits once per playthrough and fire deferred targets after the next map loads.【F:src/server/gameplay/g_target.cpp†L2093-L2136】
- **`trigger_coop_relay`** – Forces cooperative players to gather before progression, optionally auto-firing on a timer. Customize localized prompts with `message`/`map`, and remember the relay ignores spectators or noclip testers.【F:src/server/gameplay/g_trigger.cpp†L1287-L1383】

### Campaign Quality-of-Life
- **`target_healthbar`** – Binds a HUD health bar to a monster when the encounter matters. Supply a readable name via `message`; the entity silently removes itself if spawned in deathmatch to avoid clutter.【F:src/server/gameplay/g_target.cpp†L1934-L2001】
- **`target_autosave`** – Enqueues an `autosave` command when enough time has elapsed since the previous save. Server operators can raise the minimum interval with `g_athena_auto_save_min_time`, so pair the trigger with pacing that tolerates skipped saves.【F:src/server/gameplay/g_target.cpp†L2003-L2023】
- **`target_sky`** – Live-adjusts skyboxes, rotation speed, and autorotation settings using the same keys as `worldspawn`. Great for weather shifts or teleporter vistas without respawning the level.【F:src/server/gameplay/g_target.cpp†L2025-L2089】
- **`target_delay`** and **`target_print`** – Queue downstream targets after a `wait ± random` window or broadcast localized centerprint text (team-filtered or activator-only). Pair them with cinematic triggers to smooth pacing.【F:src/server/gameplay/g_target.cpp†L2658-L2713】
- **`trigger_flashlight`** & **`trigger_fog`** – Manipulate presentation as players move. Flashlight triggers enforce on/off states based on travel direction or explicit styles, while fog triggers lerp color, density, and height fog via on/off banks, optionally blending by trigger depth.【F:src/server/gameplay/g_trigger.cpp†L1031-L1284】

### Encounter & Hazard Tools
- **`target_mal_laser`** – Drops a trap laser that can be color-coded, scaled fat or thin, and toggled at runtime. Respect `wait`/`delay` to tune burst cadence, and remember the trap flags mesh with `target_laser` logic for zaps.【F:src/server/gameplay/g_target.cpp†L2173-L2259】
- **`target_steam`** – Emits particle jets toward a `target`ed bounding box or along its move direction. Adjust `count`, `speed`, and color palette for steam, sparks, slime, or blood bursts.【F:src/server/gameplay/g_target.cpp†L2265-L2337】
- **`target_anger`** & **`target_killplayers`** – Force monsters to acquire new enemies or wipe the arena clean—useful for fail-safe events or scripted betrayals.【F:src/server/gameplay/g_target.cpp†L2425-L2487】
- **`target_blacklight`** & **`target_orb`** – Provide pulsing visual markers tied to tracker-style effects for supernatural set pieces; both auto-remove in competitive playlists.【F:src/server/gameplay/g_target.cpp†L2489-L2537】
- **`trigger_disguise`** & **`trigger_safe_fall`** – Flag players as disguised (or strip the disguise) and grant a single fall-damage exemption when they’re airborne—ideal for stealth and traversal tutorials.【F:src/server/gameplay/g_trigger.cpp†L1601-L1669】

### Inventory, Score, and Progression Controls
- **`target_remove_powerups`**, **`target_remove_weapons`**, and **`target_give`** – Script loadouts mid-level. Powerup removal respects server rules like `g_quadhog` when deciding whether to drop the Quad, weapon removal clears every gun/ammo but leaves the blaster equipped for safety, and `target_give` clones the targeted item’s pickup function.【F:src/server/gameplay/g_target.cpp†L2542-L2654】
- **`target_score`** & **`target_setskill`** – Award points (optionally team score) or clamp the global `skill` cvar during campaign sequences that expect a certain difficulty.【F:src/server/gameplay/g_target.cpp†L2821-L2839】【F:src/server/gameplay/g_target.cpp†L2792-L2817】
- **`target_cvar`**, **`target_autosave`**, and **`target_achievement`** – Offer mapper-driven hooks into server state without editing configs: flip arbitrary cvars, trigger safe checkpoints, or emit achievements for all clients.【F:src/server/gameplay/g_target.cpp†L2003-L2023】【F:src/server/gameplay/g_target.cpp†L2138-L2157】【F:src/server/gameplay/g_target.cpp†L2769-L2788】
- **`target_print`** & **`target_music`** – Layer UI feedback and soundtrack swaps without touching server scripts, ensuring consistent messaging between public servers and offline tests.【F:src/server/gameplay/g_target.cpp†L1921-L1932】【F:src/server/gameplay/g_target.cpp†L2684-L2713】

### Teleportation & Movement
- **`target_teleporter`** – Teleports the activator to a targeted destination (or a fallback spawn) and accepts grenades for trap scripting. Combine with `trigger_flashlight`/fog cues for readable transitions.【F:src/server/gameplay/g_target.cpp†L2718-L2747】
- **`trigger_safe_fall` / `trigger_coop_relay` pairing** – Use safe-fall buffers near teleports or jump puzzles while a nearby coop relay confirms every live player is present before moving platforms or lifts.【F:src/server/gameplay/g_trigger.cpp†L1287-L1383】【F:src/server/gameplay/g_trigger.cpp†L1651-L1669】

### Projectile Emitters & Legacy Compatibility
- **`target_shooter_*` family** – Fire configured grenades, rockets, BFGs, Prox mines, Ion Rippers, Phalanx rounds, or Flechettes along the entity’s `angles`. Each defaults to sensible damage/speed per weapon but can be tuned per encounter, emitting the proper firing SFX.【F:src/server/gameplay/g_target.cpp†L2844-L3030】
- **`trap_spikeshooter`** – Quake 1 compatibility helper that flings spikes or a blaster-like laser based on spawn flags; treat it as a lightweight shortcut when porting legacy traps.【F:src/server/gameplay/g_target.cpp†L3031-L3073】

## Additional Notes
- Domination-specific scripts run during `SpawnEntities`, so entity overrides that remove control-point actors can break the mode; keep overrides additive when supporting multiple gametypes per BSP.【F:src/server/gameplay/g_spawn.cpp†L1893-L1932】
- Reloading the saved entity string resets `worldspawn` toggles (`no_grapple`, spawn pads, telepads) to their defaults before re-parsing, ensuring hot-reloaded edits behave consistently between iterations.【F:src/server/gameplay/g_spawn.cpp†L1936-L1961】
