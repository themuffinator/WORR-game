# WORR Level Design Overview

WORR dramatically broadens Quake II Rerelease's entity vocabulary and map logic, enabling classic layouts to coexist with ambitious, multi-mode battlegrounds. This guide summarizes the most relevant systems for designers and highlights how to author rotation-ready maps for the expanded rulesets.

## Expanded Entity & Logic Support
- WORR integrates a much wider range of entities, monsters, and scripted behaviors than stock Quake II, pulling from other Quake titles and popular community mods.【F:README.md†L13-L46】【F:src/server/gameplay/g_spawn.cpp†L300-L620】 Designers can rely on richer encounter scripting, more interactive set-pieces, and broader multiplayer logic without custom DLL work.
- Multiplayer infrastructure layers modern match management on top of legacy triggers, so legacy BSPs remain compatible while new content can opt into advanced announcers, scoreboard cues, arena selectors, and voting hooks.【F:README.md†L31-L46】【F:src/server/gameplay/g_spawn.cpp†L2320-L2389】

## Entity Catalogue
Each classname below has a corresponding `SP_*` spawn routine in `g_spawn.cpp`. Groupings match the major editor categories so you can audit coverage quickly.

### Player & World Setup Entities
| Classname | Purpose | Notable Keys & Flags |
| --- | --- | --- |
| `worldspawn` | Defines global lighting, gravity, ruleset overrides, and arena counts for the level. | Supports `sky`, `music`, `gravity`, `ruleset`, `start_items`, `arena`, and other `worldspawn` keys parsed via `spawn_temp_t` to seed `LevelLocals`.【F:src/server/gameplay/g_spawn.cpp†L2320-L2389】【F:src/server/g_local.hpp†L2564-L2656】 |
| `info_player_start`, `info_player_coop`, `info_player_coop_lava` | Campaign/coop spawn anchors. | Honor standard Quake II spawnflag filters (`NOT_EASY`…); coop-only spawns automatically inherit `SPAWNFLAG_COOP_ONLY`.【F:src/server/gameplay/g_spawn.cpp†L324-L340】【F:src/server/g_local.hpp†L724-L735】 |
| `info_player_deathmatch` | Free-for-all spawn pads. | Eligible for arena filtering; `level.arenaActive` hides pads whose `arena` key does not match the active arena.【F:src/server/gameplay/g_spawn.cpp†L324-L348】【F:src/server/gameplay/g_spawn.cpp†L2320-L2389】 |
| `info_player_team_red`, `info_player_team_blue` | Team-specific spawns for TDM/CTF and Domination fallback spawns. | Respect `arena` key; combine with `info_nav_lock` volumes to fence warmup staging areas.【F:src/server/gameplay/g_spawn.cpp†L324-L342】 |
| `info_player_intermission`, `info_intermission` | Intermission camera nodes. | Pair with `target` chains for cinematic fly-throughs; supports message strings for scoreboard overlays.【F:src/server/gameplay/g_spawn.cpp†L332-L344】 |
| `info_teleport_destination`, `info_ctf_teleport_destination` | Teleport exit anchors. | Used by `trigger_teleport`, `trigger_ctf_teleport`, and scripted targets; honor `angles` and `targetname`.【F:src/server/gameplay/g_spawn.cpp†L332-L360】 |
| `info_null`, `info_notnull` | Editor helpers for rotating movers or target positions. | `info_notnull` keeps its origin for targeting; `info_null` collapses to the world origin.【F:src/server/gameplay/g_spawn.cpp†L332-L366】 |
| `info_landmark` | Reference origin for changelevel transitions. | Combine with `SPAWNFLAG_LANDMARK_KEEP_Z` to preserve vertical offset between linked maps.【F:src/server/gameplay/g_spawn.cpp†L332-L372】【F:src/server/g_local.hpp†L4258-L4260】 |
| `info_world_text` | Places localized instructional text in the world. | Configure `message`, `image`, and fade distances through `spawn_temp_t` keys.【F:src/server/gameplay/g_spawn.cpp†L334-L376】【F:src/server/g_local.hpp†L2564-L2605】 |
| `info_nav_lock` | Blocks bot navigation through a brush volume. | Useful for sealing navgraph around cinematic-only spaces.【F:src/server/gameplay/g_spawn.cpp†L334-L342】 |
| `domination_point` | Domination control volume. | Optional spawnflags pre-assign ownership (`SPAWNFLAG_DOMINATION_START_RED/BLUE`); `message` or `targetname` seeds capture callouts.【F:src/server/gameplay/g_spawn.cpp†L340-L342】【F:src/server/gameplay/g_domination.cpp†L1-L116】 |

### Ambient Audio Entities
| Classname | Loop Type |
| --- | --- |
| `ambient_suck_wind`, `ambient_drone`, `ambient_flouro_buzz`, `ambient_drip`, `ambient_comp_hum`, `ambient_thunder`, `ambient_light_buzz`, `ambient_swamp1`, `ambient_swamp2`, `ambient_generic` | Ambient sound emitters that precache looping samples and attach to the entity origin; use `noise` and `volume` keys as in stock Q2.【F:src/server/gameplay/g_spawn.cpp†L304-L332】 |

### Movers & Mechanical Brushes
| Classname | Purpose | Notes |
| --- | --- | --- |
| `func_plat`, `func_plat2`, `func_button`, `func_door`, `func_door_secret`, `func_door_secret2`, `func_door_rotating` | Core movers with Quake II spawnflags (toggle, crusher, axis selection). | WORR preserves legacy spawnflags and adds smart water doors via `SPAWNFLAG_WATER_SMART` and `SPAWNFLAG_DOOR_REVERSE` helpers.【F:src/server/gameplay/g_spawn.cpp†L344-L402】【F:src/server/g_local.hpp†L3719-L3722】 |
| `func_rotating`, `func_rotating_ext`, `func_spinning` | Continuous rotating brushes; `_ext` retains Oblivion compatibility fields. | Use `speed`, `dmg`, and axis keys; `_ext` honors extended rotate vectors in `spawn_temp_t`.【F:src/server/gameplay/g_spawn.cpp†L344-L410】【F:src/server/g_local.hpp†L2613-L2623】 |
| `func_train`, `func_rotate_train` | Path-based movers (linear or rotating). | Combine with `path_corner` and `SPAWNFLAG_TRAIN_START_ON` / `SPAWNFLAG_TRAIN_MOVE_TEAMCHAIN` for advanced sequences.【F:src/server/gameplay/g_spawn.cpp†L352-L412】【F:src/server/g_local.hpp†L3719-L3721】 |
| `func_water`, `func_bobbingwater`, `func_conveyor` | Volume movers for liquids or scrolling brushes. | `SPAWNFLAG_WATER_SMART` enables dynamic toggles (e.g., sluice gates).【F:src/server/gameplay/g_spawn.cpp†L352-L412】【F:src/server/g_local.hpp†L3719-L3721】 |
| `func_areaportal`, `func_clock`, `func_timer`, `func_killbox`, `func_explosive`, `func_object`, `func_object_repair`, `func_eye`, `func_animation` | Utility brush logic for optimization, timed events, kill volumes, breakables, and animated props. | Timer entities respect `wait`, `random`, and `delay` keys; repair/object combos add scripted set pieces for campaign extras.【F:src/server/gameplay/g_spawn.cpp†L352-L410】【F:src/server/gameplay/g_spawn.cpp†L412-L438】 |
| `func_wall`, `func_static`, `func_force_wall`, `func_illusionary` | Decorative or solid brushes (static BSP). | `func_force_wall` supports toggled forcefields; `func_static` aliases to `func_wall` for compatibility.【F:src/server/gameplay/g_spawn.cpp†L352-L410】 |

### Trigger Volumes
| Classname | Purpose |
| --- | --- |
| `trigger_always`, `trigger_once`, `trigger_multiple`, `trigger_relay` | Core trigger logic for scripted sequences and logic splitting.【F:src/server/gameplay/g_spawn.cpp†L412-L444】 |
| `trigger_push`, `trigger_gravity`, `trigger_safe_fall` | Physics modifiers (jump pads, gravity volumes, fall-damage toggles).【F:src/server/gameplay/g_spawn.cpp†L418-L444】 |
| `trigger_hurt`, `trigger_monsterjump`, `trigger_key`, `trigger_counter`, `trigger_elevator` | Damage, AI, key-gated, counter, and elevator flows.【F:src/server/gameplay/g_spawn.cpp†L418-L444】 |
| `trigger_flashlight`, `trigger_fog`, `trigger_coop_relay`, `trigger_health_relay`, `trigger_disguise` | Specialty volumes for lighting, fog, cooperative gating, health requirements, or disguise toggles from mission pack logic.【F:src/server/gameplay/g_spawn.cpp†L420-L444】 |
| `trigger_teleport`, `trigger_ctf_teleport`, `trigger_misc_camera` | Teleport and camera triggers, including mission-pack rail cameras.【F:src/server/gameplay/g_spawn.cpp†L424-L446】 |
| `trigger_proball_goal`, `trigger_proball_oob` | ProBall scoring/out-of-bounds volumes; assign spawnflags to lock to red/blue goals.【F:src/server/gameplay/g_spawn.cpp†L430-L446】【F:src/server/gameplay/g_proball.cpp†L380-L533】 |
| `trigger_secret` | Legacy secret counter trigger (targets `target_secret`).【F:src/server/gameplay/g_spawn.cpp†L446-L448】 |

### Target Logic Entities
| Classname | Purpose |
| --- | --- |
| `target_temp_entity`, `target_speaker`, `target_explosion`, `target_splash`, `target_blaster`, `target_railgun`, `target_push` | Fire-and-forget effects, ambient audio, and scripted weapon fire.【F:src/server/gameplay/g_spawn.cpp†L448-L520】 |
| `target_changelevel`, `target_crosslevel_trigger`, `target_crosslevel_target`, `target_crossunit_trigger`, `target_crossunit_target` | Inter-level progression and persistent secret tally hooks; support changelevel spawnflags listed later.【F:src/server/gameplay/g_spawn.cpp†L452-L474】【F:src/server/g_local.hpp†L3955-L3958】 |
| `target_secret`, `target_goal`, `target_help`, `target_character`, `target_string`, `target_story`, `target_achievement` | Campaign scoring, helper VO, character subtitles, and Steam-style achievements.【F:src/server/gameplay/g_spawn.cpp†L452-L520】 |
| `target_camera`, `target_lightramp`, `target_light`, `target_music`, `target_sky`, `target_steam`, `target_anger`, `target_mal_laser` | Presentation controls for cameras, lighting transitions, ambient layers, sky swaps, steam vents, anger triggers, and boss-specific lasers.【F:src/server/gameplay/g_spawn.cpp†L460-L520】 |
| `target_poi`, `target_healthbar`, `target_autosave`, `target_killplayers`, `target_remove_powerups`, `target_remove_weapons` | Multiplayer HUD cues, boss healthbars, autosave triggers, fail-safe wipes, and inventory scripts.【F:src/server/gameplay/g_spawn.cpp†L468-L520】【F:src/server/g_local.hpp†L3598-L3604】 |
| `target_give`, `target_delay`, `target_print`, `target_cvar`, `target_setskill`, `target_score` | Utility logic from Quake III/Live for giving items, delayed actions, messaging, cvar manipulation, skill changes, and score awards.【F:src/server/gameplay/g_spawn.cpp†L470-L520】 |
| `target_shooter_*`, `trap_shooter`, `trap_spikeshooter` | Projectile turrets configurable via spawnflags and `speed`, `count`, and `wait` keys.【F:src/server/gameplay/g_spawn.cpp†L486-L520】 |

### Lighting, Pathing & Combat Setup
| Classname | Purpose |
| --- | --- |
| `dynamic_light`, `rotating_light`, `light`, `light_mine1`, `light_mine2` | Dynamic or static light sources; `dynamic_light` and `rotating_light` expose RGB/rotation keys, while mine lights mimic mission-pack props.【F:src/server/gameplay/g_spawn.cpp†L520-L548】 |
| `func_group` | Editor helper for grouping brushes; stripped at runtime.【F:src/server/gameplay/g_spawn.cpp†L520-L548】 |
| `path_corner`, `point_combat`, `hint_path` | AI navigation and combat staging markers. `SPAWNFLAG_PATH_CORNER_TELEPORT` and `SPAWNFLAG_POINT_COMBAT_HOLD` adjust behaviors.【F:src/server/gameplay/g_spawn.cpp†L520-L564】【F:src/server/g_local.hpp†L3841-L3844】 |

### Miscellaneous World Objects
| Classname | Purpose |
| --- | --- |
| `misc_explobox`, `misc_banner`, `misc_ctf_banner`, `misc_ctf_small_banner`, `misc_satellite_dish`, `misc_actor`, `misc_player_mannequin`, `misc_model` | Decorative props and scripted actors; combine with `target`/`pathtarget` for set pieces.【F:src/server/gameplay/g_spawn.cpp†L548-L604】 |
| `misc_gib_arm`, `misc_gib_leg`, `misc_gib_head`, `misc_insane`, `misc_deadsoldier` | Gory ambience and idle NPCs from the campaign.【F:src/server/gameplay/g_spawn.cpp†L548-L604】 |
| `misc_viper`, `misc_viper_bomb`, `misc_bigviper`, `misc_strogg_ship`, `misc_transport`, `misc_crashviper`, `misc_viper_missile`, `misc_amb4` | Animated vehicles and scripted sequences from the base game and mission packs.【F:src/server/gameplay/g_spawn.cpp†L556-L612】 |
| `misc_teleporter`, `misc_teleporter_dest`, `misc_blackhole`, `misc_flare`, `misc_hologram`, `misc_lavaball`, `misc_nuke`, `misc_nuke_core` | Utility set pieces for teleporters, FX, and nuke sequences.【F:src/server/gameplay/g_spawn.cpp†L556-L612】 |
| `misc_eastertank`, `misc_easterchick`, `misc_easterchick2` | Secret easter eggs preserved for compatibility.【F:src/server/gameplay/g_spawn.cpp†L560-L600】 |
| `misc_camera`, `misc_camera_target` | Mission-pack surveillance cameras with scripted movement targets.【F:src/server/gameplay/g_spawn.cpp†L600-L604】 |

### Monsters, Turrets & Boss Logic
| Classname | Faction/Notes |
| --- | --- |
| `monster_berserk`, `monster_gladiator`, `monster_gunner`, `monster_infantry`, `monster_soldier_light`, `monster_soldier`, `monster_soldier_ss`, `monster_tank`, `monster_tank_commander`, `monster_medic`, `monster_flipper`, `monster_eel`, `monster_chick`, `monster_parasite`, `monster_flyer`, `monster_brain`, `monster_floater`, `monster_hover`, `monster_mutant`, `monster_supertank`, `monster_boss2`, `monster_boss3_stand`, `monster_jorg`, `monster_makron`, `monster_tank_stand`, `monster_guardian`, `monster_arachnid`, `monster_guncmdr` | Core Quake II roster including bosses and guard variants; honor `SPAWNFLAG_MONSTER_*` flags for ambush, trigger-spawn, corpse, super step, no-drop, and scenic behaviors.【F:src/server/gameplay/g_spawn.cpp†L511-L538】【F:src/server/g_local.hpp†L3814-L3820】 |
| `monster_commander_body` | Static commander body used in intro set pieces.【F:src/server/gameplay/g_spawn.cpp†L540-L544】 |
| `turret_breach`, `turret_base`, `turret_driver`, `monster_turret`, `turret_invisible_brain` | Automated turrets and invisible brain controllers; spawnflags select weapon loadouts (machinegun, rocket, blaster).【F:src/server/gameplay/g_spawn.cpp†L542-L566】【F:src/server/g_local.hpp†L8652-L8739】 |
| `monster_soldier_hypergun`, `monster_soldier_lasergun`, `monster_soldier_ripper`, `monster_fixbot`, `monster_gekk`, `monster_chick_heat`, `monster_gladb`, `monster_boss5`, `monster_stalker`, `monster_daedalus`, `monster_carrier`, `monster_widow`, `monster_widow2`, `monster_medic_commander`, `monster_kamikaze` | Mission-pack additions and scripted allies (FixBot). FixBot spawnflags (`FIXIT`, `TAKEOFF`, `LANDING`, `WORKING`) gate AI state transitions.【F:src/server/gameplay/g_spawn.cpp†L546-L564】【F:src/server/gameplay/g_spawn.cpp†L555-L564】【F:src/server/g_local.hpp†L3823-L3827】【F:src/server/monsters/m_fixbot.cpp†L198-L213】 |
| `monster_shambler`, `monster_dog`, `monster_ogre`, `monster_ogre_marksman`, `monster_ogre_multigrenade`, `monster_fish`, `monster_army`, `monster_centroid`, `monster_demon1`, `monster_zombie`, `monster_tarbaby`, `monster_tarbaby_hell`, `monster_spike`, `monster_spike_hell`, `monster_mine`, `monster_mine_hell`, `monster_shalrath`, `monster_enforcer`, `monster_knight`, `monster_sword`, `monster_hell_knight`, `monster_wizard`, `monster_oldone`, `monster_chthon`, `monster_dragon`, `monster_lavaman`, `monster_boss`, `monster_wyvern` | Quake/mission-pack crossover roster. Pair with `target_chthon_lightning` for boss scripted strikes.【F:src/server/gameplay/g_spawn.cpp†L566-L595】【F:src/server/gameplay/g_spawn.cpp†L595-L596】 |
| `hint_path` | Guides AI navigation decisions (especially Widow Carriers). Combine with `targetname` chains. 【F:src/server/gameplay/g_spawn.cpp†L640-L668】 |

### Mode-Specific Hooks
| Classname | Use |
| --- | --- |
| `domination_point` | Team scoring volume; optional spawnflags define initial owner and `spawn_temp_t.radius`/`height` auto-build the trigger hull.【F:src/server/gameplay/g_domination.cpp†L1-L116】 |
| `trigger_proball_goal`, `trigger_proball_oob`, `item_ball` | ProBall scoring and reset logic; goal triggers mark red/blue with dedicated spawnflags, while out-of-bounds resets the carrier and ball entity.【F:src/server/gameplay/g_spawn.cpp†L430-L446】【F:src/server/gameplay/g_proball.cpp†L380-L533】【F:src/server/gameplay/g_item_list.cpp†L1080-L1185】 |
| `target_chthon_lightning` | Boss helper that spawns scripted lightning strikes for Chthon arenas.【F:src/server/gameplay/g_spawn.cpp†L618-L620】 |

### Item Catalogue
Items are defined in `g_item_list.cpp`; categories below enumerate every spawnable classname so rotation audits remain complete.

#### Armor & Power Armor
`item_armor_body`, `item_armor_combat`, `item_armor_jacket`, `item_armor_shard`, `item_power_screen`, `item_power_shield`【F:src/server/gameplay/g_item_list.cpp†L64-L210】

#### Weapons & Offensive Deployables
`weapon_grapple`, `weapon_blaster`, `weapon_chainfist`, `weapon_shotgun`, `weapon_supershotgun`, `weapon_machinegun`, `weapon_etf_rifle`, `weapon_chaingun`, `ammo_grenades`, `ammo_trap`, `ammo_tesla`, `weapon_grenadelauncher`, `weapon_proxlauncher`, `weapon_rocketlauncher`, `weapon_hyperblaster`, `weapon_boomer`, `weapon_plasmabeam`, `weapon_lightning`, `weapon_railgun`, `weapon_phalanx`, `weapon_bfg`, `weapon_disintegrator`【F:src/server/gameplay/g_item_list.cpp†L210-L640】

#### Ammo & Charges
`ammo_shells`, `ammo_bullets`, `ammo_cells`, `ammo_rockets`, `ammo_slugs`, `ammo_magslug`, `ammo_flechettes`, `ammo_prox`, `ammo_nuke`, `ammo_disruptor`【F:src/server/gameplay/g_item_list.cpp†L642-L760】

#### Powerups & Utility Gear
`item_quad`, `item_quadfire`, `item_invulnerability`, `item_invisibility`, `item_silencer`, `item_breather`, `item_enviro`, `item_empathy`, `item_antigrav`, `item_ancient_head`, `item_legacy_head`, `item_adrenaline`, `item_bandolier`, `item_pack`, `item_ir_goggles`, `item_double`, `item_sphere_vengeance`, `item_sphere_hunter`, `item_sphere_defender`, `item_doppleganger`【F:src/server/gameplay/g_item_list.cpp†L760-L1080】

#### Keys & Objectives
`key_data_cd`, `key_power_cube`, `key_explosive_charges`, `key_yellow_key`, `key_power_core`, `key_pyramid`, `key_data_spinner`, `key_pass`, `key_blue_key`, `key_red_key`, `key_green_key`, `key_commander_head`, `key_airstrike_target`, `key_nuke_container`, `key_nuke`【F:src/server/gameplay/g_item_list.cpp†L1080-L1278】

#### Health, Flags, Tech & Miscellaneous Pickups
`item_health_small`, `item_health`, `item_health_large`, `item_health_mega`, `ITEM_CTF_FLAG_RED`, `ITEM_CTF_FLAG_BLUE`, `ITEM_CTF_FLAG_NEUTRAL`, `item_tech1`, `item_tech2`, `item_tech3`, `item_tech4`, `ammo_shells_large`, `ammo_shells_small`, `ammo_bullets_large`, `ammo_bullets_small`, `ammo_cells_large`, `ammo_cells_small`, `ammo_rockets_small`, `ammo_slugs_large`, `ammo_slugs_small`, `item_teleporter`, `item_regen`, `item_foodcube`, `item_ball`, `item_spawn_protect`, `item_flashlight`, `item_compass`【F:src/server/gameplay/g_item_list.cpp†L1278-L1540】

## Spawn Flag Reference
WORR exposes dedicated spawnflag literals so editor setups remain deterministic.

### Global Difficulty & Availability Flags
- `SPAWNFLAG_NOT_EASY`, `SPAWNFLAG_NOT_MEDIUM`, `SPAWNFLAG_NOT_HARD`, `SPAWNFLAG_NOT_DEATHMATCH`, `SPAWNFLAG_NOT_COOP`, `SPAWNFLAG_COOP_ONLY` — inherited from Quake II and reserved for universal availability filters.【F:src/server/g_local.hpp†L724-L735】

### Mode & Objective Flags
- `SPAWNFLAG_DOMINATION_START_RED`, `SPAWNFLAG_DOMINATION_START_BLUE` — pre-assign Domination point ownership.【F:src/server/g_local.hpp†L755-L756】【F:src/server/gameplay/g_domination.cpp†L24-L55】
- `SPAWNFLAG_PROBALL_GOAL_RED`, `SPAWNFLAG_PROBALL_GOAL_BLUE` — lock ProBall goal volumes to a team.【F:src/server/g_local.hpp†L758-L759】【F:src/server/gameplay/g_proball.cpp†L404-L433】

### Item & Powerup Flags
- `SPAWNFLAG_ITEM_TRIGGER_SPAWN`, `SPAWNFLAG_ITEM_NO_TOUCH`, `SPAWNFLAG_ITEM_TOSS_SPAWN`, `SPAWNFLAG_ITEM_SUSPENDED`, `SPAWNFLAG_ITEM_MAX` — legacy Quake II item behavior.
- `SPAWNFLAG_ITEM_DROPPED`, `SPAWNFLAG_ITEM_DROPPED_PLAYER`, `SPAWNFLAG_ITEM_TARGETS_USED`, `SPAWNFLAG_ITEM_NO_DROP` — WORR-internal bookkeeping to suppress respawns or mark scripted drops.【F:src/server/g_local.hpp†L3384-L3396】【F:src/server/gameplay/g_items.cpp†L180-L312】

### Laser, Healthbar & Presentation Flags
- `SPAWNFLAG_LASER_ON`, `SPAWNFLAG_LASER_RED`, `SPAWNFLAG_LASER_GREEN`, `SPAWNFLAG_LASER_BLUE`, `SPAWNFLAG_LASER_YELLOW`, `SPAWNFLAG_LASER_ORANGE`, `SPAWNFLAG_LASER_FAT`, `SPAWNFLAG_LASER_LIGHTNING`, `SPAWNFLAG_LASER_ZAP` — customize `target_laser` beams.【F:src/server/g_local.hpp†L3588-L3596】
- `SPAWNFLAG_HEALTHBAR_PVS_ONLY` — shows boss healthbars only when the entity is in the player’s PVS.【F:src/server/g_local.hpp†L3598-L3601】

### Movers & Brush Entities
- `SPAWNFLAG_TRAIN_START_ON`, `SPAWNFLAG_TRAIN_MOVE_TEAMCHAIN`, `SPAWNFLAG_WATER_SMART`, `SPAWNFLAG_DOOR_REVERSE` — extend `func_train`, smart water doors, and inverse door directions.【F:src/server/g_local.hpp†L3719-L3722】

### Monster & Bot Helpers
- `SPAWNFLAG_MONSTER_AMBUSH`, `SPAWNFLAG_MONSTER_TRIGGER_SPAWN`, `SPAWNFLAG_MONSTER_CORPSE`, `SPAWNFLAG_MONSTER_SUPER_STEP`, `SPAWNFLAG_MONSTER_NO_DROP`, `SPAWNFLAG_MONSTER_SCENIC` — control AI awareness, spawn timing, corpse actors, and loot drops.【F:src/server/g_local.hpp†L3814-L3820】
- FixBot-specific flags (`SPAWNFLAG_FIXBOT_FIXIT`, `SPAWNFLAG_FIXBOT_TAKEOFF`, `SPAWNFLAG_FIXBOT_LANDING`, `SPAWNFLAG_FIXBOT_WORKING`) toggle repair drone behaviors.【F:src/server/g_local.hpp†L3823-L3826】【F:src/server/monsters/m_fixbot.cpp†L8985-L9022】

### Pathing & Changelevel Flags
- `SPAWNFLAG_PATH_CORNER_TELEPORT` — teleport trains to the next `path_corner` instantly rather than moving along the path.【F:src/server/g_local.hpp†L3841-L3842】
- `SPAWNFLAG_POINT_COMBAT_HOLD` — orders AI to hold at a `point_combat` node.【F:src/server/g_local.hpp†L3843-L3844】
- `SPAWNFLAG_CHANGELEVEL_CLEAR_INVENTORY`, `SPAWNFLAG_CHANGELEVEL_NO_END_OF_UNIT`, `SPAWNFLAG_CHANGELEVEL_FADE_OUT`, `SPAWNFLAG_CHANGELEVEL_IMMEDIATE_LEAVE` — tweak intermission flow when using `target_changelevel`.【F:src/server/g_local.hpp†L3955-L3958】
- `SPAWNFLAG_LANDMARK_KEEP_Z` — preserve vertical offset across changelevels anchored to `info_landmark`.【F:src/server/g_local.hpp†L4258-L4260】

## Map Metadata & JSON Schema
Map metadata drives rotation, voting, and MyMap validation. The `MapEntry` structure shows the authoritative field list used by the server.【F:src/server/g_local.hpp†L1874-L1950】 Loading routines in `g_map_manager.cpp` map JSON keys to these members.【F:src/server/gameplay/g_map_manager.cpp†L400-L452】

| JSON Field | Type | Description |
| --- | --- | --- |
| `bsp` | string (required) | BSP filename without extension; primary key for lookups. |
| `title` | string | Friendly name displayed in votes/status. |
| `dm` | bool (required) | Marks the entry as multiplayer-capable. Non-DM entries are skipped for rotation. |
| `min`, `max` | integer | Recommended player bounds; selector enforces the active population band. |
| `gametype` | integer | Default `GameType` index when no override is supplied. |
| `ruleset` | integer | Preferred ruleset (Quake/Quake III style). |
| `scorelimit`, `timelimit` | integer | Per-map overrides applied on load. |
| `popular`, `custom` | bool | Selector hints to prefer or avoid entries, and mark custom packages. |
| `custom_textures`, `custom_sounds` | bool | Flag custom resources so hosts can filter them. |
| `sp`, `coop` | bool | Advertise campaign/coop compatibility for mixed playlists. |
| `tdm`, `ctf`, `duel` | bool | Preferred gametype badges used when filtering votes for teamplay or duel nights. |
| `control_points` | integer | Optional Domination metadata for designers; document expected capture volumes in rotations. |

Example map pool entry with optional fields:
```json
{
  "maps": [
    {
      "bsp": "worr_dom01",
      "title": "Orbital Relay",
      "dm": true,
      "dom": true,
      "tdm": true,
      "min": 4,
      "max": 12,
      "gametype": 3,
      "ruleset": 2,
      "scorelimit": 125,
      "timelimit": 20,
      "control_points": 3,
      "custom": false,
      "custom_textures": true,
      "popular": true
    }
  ]
}
```
The cycle file remains a whitespace- or comma-delimited list with comments removed prior to parsing:
```text
// Arena rotation
ra2_dm1 ra2_dm2, ra2_dm3
```
`LoadMapCycle` promotes matching BSP names by setting `MapEntry::isCycleable`; unmatched tokens are reported to admins.【F:docs/mapsystem.html†L20-L128】【F:src/server/gameplay/g_map_manager.cpp†L420-L520】

### MyMap Overrides
Personal map requests expose bitfield toggles parsed into `MyMapRequest` and `QueuedMap` structures.【F:src/server/g_local.hpp†L1890-L1962】 Supported flags:
- Enable bits: `+pu` (powerups), `+pa` (power armor), `+ar` (armor), `+am` (ammo), `+ht` (health), `+bfg`, `+pb` (plasma beam), `+fd` (fall damage), `+sd` (self damage), `+ws` (weapons stay)
- Disable bits: use the `-` prefix with the same tokens. Entries failing population or gametype checks are rejected before joining the queue.【F:docs/mapsystem.html†L48-L86】

## Mode-Specific Layout Guidance
Leverage existing WORR documentation when blocking out mode-focused revisions:
- **Domination:** Place `domination_point` volumes on high-visibility landmarks that form a short circulation loop so teams can rotate quickly. Each point auto-builds a trigger using `radius`/`height` keys and supports up to eight active points (`LevelLocals::DominationState::MAX_POINTS`). Use red/blue spawnflags to script opening ownership and keep travel times roughly equal.【F:docs/domination.md†L1-L24】【F:src/server/gameplay/g_domination.cpp†L1-L138】【F:src/server/g_local.hpp†L2384-L2396】
- **Arena / Round Modes (Clan Arena, CaptureStrike, Red Rover, Gauntlet):** Author discrete arenas and expose the total count via the `worldspawn` `arena` key so WORR tracks `level.arenaTotal` and rotates pads correctly. Associate spawns with arena numbers to keep `level.arenaActive` filters functional when players vote for specific arenas.【F:src/server/gameplay/g_spawn.cpp†L2320-L2389】【F:src/server/gameplay/g_spawn.cpp†L2246-L2399】
- **CTF Family (CTF, One Flag, Harvester, Overload):** Ensure flag stands, obelisks, or cores are mirrored and accessible from balanced approaches. Use team spawnpoints, inventory clustering, and `target_help` callouts to reinforce objectives. Preferred gametype booleans (`tdm`, `ctf`, `duel`) in the map pool help selector logic surface these layouts when the playlist rotates.【F:src/server/gameplay/g_spawn.cpp†L324-L388】【F:src/server/gameplay/g_map_manager.cpp†L420-L452】
- **ProBall:** Build clearly signed goal trigger brushes with `SPAWNFLAG_PROBALL_GOAL_RED/BLUE` and surround aerial boundaries with out-of-bounds volumes so the ball resets cleanly. Keep the midfield open for chainfist steals and provide spectator perches for announcer clarity.【F:src/server/gameplay/g_proball.cpp†L380-L533】【F:docs/mapsystem.html†L90-L128】
- **Campaign/Horde Support:** Populate scripted encounters with mission-pack monsters listed above and use `point_combat`/`hint_path` nodes plus monster spawnflags for ambush pacing. `target_autosave`, `target_story`, and `target_achievement` entities help align with WORR’s analytics and progression tooling.【F:src/server/gameplay/g_spawn.cpp†L448-L604】【F:src/server/g_local.hpp†L2430-L2505】
- **Map Selector Expectations:** The voting system favors maps whose `min`/`max` player counts match the active population, have not been played recently, and advertise compatibility with the current gametype and ruleset. Distribute spawn groups and objectives so that low-pop and high-pop brackets both feel supported.【F:docs/mapsystem.html†L20-L103】

## Preparing Maps for Rotation
1. Annotate each BSP entry in the map pool JSON with accurate `min`, `max`, `gametype`, and `ruleset` metadata. These fields gate automated rotation, MyMap eligibility, and selector votes.【F:docs/mapsystem.html†L20-L86】【F:src/server/gameplay/g_map_manager.cpp†L400-L452】
2. Ensure spawn targets, arena assignments, and objective entities exist for every gametype you flag as supported. Avoid advertising unsupported modes—rotation filters skip incomplete entries and players receive confusing errors.【F:src/server/gameplay/g_spawn.cpp†L300-L620】【F:src/server/gameplay/g_spawn.cpp†L2320-L2399】
3. When authoring new geometry, budget warmup staging areas, callvote cameras, and spectator sightlines. WORR’s match modifiers and announcer cues assume clear sightlines to objectives and spawn pads.【F:src/server/gameplay/g_spawn.cpp†L412-L520】【F:src/server/g_local.hpp†L209-L344】

## Validation Checklist & Tooling
- Use JSON-aware editors or `jq` to validate map pool files before reloading them in-game.
- Reload configuration in a development server with `loadmappool` and `loadmapcycle` to catch syntax errors or missing BSPs immediately.【F:docs/mapsystem.html†L105-L126】
- Run `mappool` and `mapcycle` commands after reload to confirm that each entry appears under the intended gametype and player count brackets.【F:docs/mapsystem.html†L105-L126】
- Test MyMap compatibility by queueing the map with representative override flags (e.g., `mymap myarena +pu -ht`) and confirming the server accepts or rejects it according to your metadata.【F:docs/mapsystem.html†L48-L86】
- Maintain a staging rotation that covers each advertised gametype so QA can spawn, capture, and score through at least one full loop before publishing updates.
