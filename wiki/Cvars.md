# WORR Cvar Reference

This guide catalogs WORR’s server and gameplay cvars by responsibility. Defaults and flags originate from the registration logic in `g_main.cpp`; adjust settings via `set`/`seta` on the server console or config files. Update this page whenever new cvars land so operators and players can track behavior changes.【F:src/server/gameplay/g_main.cpp†L709-L918】

## Server Identity & Access Control
| Cvar | Default | Notes |
| --- | --- | --- |
| `hostname` | `"Welcome to WORR!"` | Advertised server name in browsers.【F:src/server/gameplay/g_main.cpp†L709-L710】 |
| `password` | `""` | Player password; set to gate private lobbies.【F:src/server/gameplay/g_main.cpp†L790-L794】 |
| `spectator_password` | `""` | Optional spectator-only password.【F:src/server/gameplay/g_main.cpp†L790-L794】 |
| `admin_password` | `""` | Required for admin command authentication.【F:src/server/gameplay/g_main.cpp†L790-L794】 |
| `needpass` | `0` | Auto-updated flag exposing whether passwords are active.【F:src/server/gameplay/g_main.cpp†L790-L794】 |
| `filterban` | `1` | Ban list behavior (1=ban matches, 0=allow list).【F:src/server/gameplay/g_main.cpp†L794-L804】 |

## Lobby Size & Warmup Flow
| Cvar | Default | Notes |
| --- | --- | --- |
| `maxclients` | `8` per split player constant | Latched engine limit; set at init.【F:src/server/gameplay/g_main.cpp†L540-L543】 |
| `maxplayers` | `16` | Caps active player slots per match.【F:src/server/gameplay/g_main.cpp†L540-L547】 |
| `minplayers` | `2` | Minimum participants before the match can start.【F:src/server/gameplay/g_main.cpp†L540-L547】 |
| `match_start_no_humans` | `1` | Auto-start matches without humans after warmup grace.【F:src/server/gameplay/g_main.cpp†L828-L835】 |
| `match_auto_join` | `1` | Auto-assigns connecting players to a team.【F:src/server/gameplay/g_main.cpp†L828-L835】 |
| `match_crosshair_ids` | `1` | Displays player names under crosshair for team modes.【F:src/server/gameplay/g_main.cpp†L829-L833】 |
| `warmup_enabled` | `1` | Enables warmup pre-game phase.【F:src/server/gameplay/g_main.cpp†L831-L833】 |
| `warmup_do_ready_up` | `0` | Require players to `/ready` before countdown.【F:src/server/gameplay/g_main.cpp†L831-L833】 |
| `g_warmup_countdown` | `10` | Seconds to count down once ready conditions met.【F:src/server/gameplay/g_main.cpp†L904-L905】 |
| `g_warmup_ready_percentage` | `0.51` | Portion of players required to be ready.【F:src/server/gameplay/g_main.cpp†L904-L905】 |

## Match Timing & Respawn Rules
| Cvar | Default | Notes |
| --- | --- | --- |
| `fraglimit` | `0` | Frags needed to end non-round matches.【F:src/server/gameplay/g_main.cpp†L777-L783】 |
| `timelimit` | `0` | Minutes before auto-ending the match.【F:src/server/gameplay/g_main.cpp†L778-L782】 |
| `roundlimit` | `8` | Rounds per match for elimination modes.【F:src/server/gameplay/g_main.cpp†L779-L780】 |
| `roundtimelimit` | `2` | Minutes per round in elimination modes.【F:src/server/gameplay/g_main.cpp†L779-L781】 |
| `capturelimit` | `8` | Flag captures to win CTF variants.【F:src/server/gameplay/g_main.cpp†L781-L782】 |
| `mercylimit` | `0` | Score delta that triggers mercy end.【F:src/server/gameplay/g_main.cpp†L781-L783】 |
| `match_do_force_respawn` | `1` | Force players to respawn automatically after death.【F:src/server/gameplay/g_main.cpp†L834-L838】 |
| `match_force_respawn_time` | `2.4` | Seconds before auto respawn triggers.【F:src/server/gameplay/g_main.cpp†L834-L838】 |
| `match_holdable_adrenaline` | `1` | Allows adrenaline to persist between deaths.【F:src/server/gameplay/g_main.cpp†L837-L838】 |
| `match_instant_items` | `1` | Enables instant-use items from inventory.【F:src/server/gameplay/g_main.cpp†L837-L838】 |
| `match_items_respawn_rate` | `1.0` | Multiplier for pickup respawn timers.【F:src/server/gameplay/g_main.cpp†L839-L841】 |
| `match_do_overtime` | `120` | Seconds of overtime if tied at regulation end.【F:src/server/gameplay/g_main.cpp†L843-L844】 |
| `match_powerup_drops` | `1` | Drop carried powerups on death.【F:src/server/gameplay/g_main.cpp†L843-L844】 |
| `match_powerup_min_player_lock` | `0` | Minimum human players before powerups spawn.【F:src/server/gameplay/g_main.cpp†L843-L846】 |
| `match_player_respawn_min_delay` | `1` | Minimum seconds before respawning players.【F:src/server/gameplay/g_main.cpp†L846-L847】 |
| `match_player_respawn_min_distance` | `256` | Minimum units from enemies for respawns.【F:src/server/gameplay/g_main.cpp†L846-L848】 |
| `match_timeout_length` | `120` | Duration of tactical pauses in seconds.【F:src/server/gameplay/g_main.cpp†L852-L853】 |
| `match_weapons_stay` | `0` | Toggle Quake III style weapons-stay logic.【F:src/server/gameplay/g_main.cpp†L852-L853】 |
| `match_drop_cmd_flags` | `7` | Bitmask controlling `/drop` command options.【F:src/server/gameplay/g_main.cpp†L853-L854】 |

## Gametype & Modifier Controls
| Cvar | Default | Notes |
| --- | --- | --- |
| `g_gametype` | `0` (`ffa`) | Active gametype index/short name.【F:src/server/match/match_state.cpp†L2299-L2316】 |
| `deathmatch` | `1` | Enables deathmatch logic when true.【F:src/server/match/match_state.cpp†L2299-L2339】 |
| `teamplay` | `0` | Legacy toggle forced on for team modes.【F:src/server/match/match_state.cpp†L2299-L2373】 |
| `ctf` | `0` | Legacy toggle forced on for CTF variants.【F:src/server/match/match_state.cpp†L2299-L2373】 |
| `marathon` | `0` | Enables multi-map Marathon meta progression.【F:src/server/gameplay/g_main.cpp†L784-L786】【F:src/server/match/match_state.cpp†L1-L168】 |
| `g_marathon_timelimit` | `0` | Minutes per leg before forced advance.【F:src/server/gameplay/g_main.cpp†L785-L786】 |
| `g_marathon_scorelimit` | `0` | Score delta per leg before advance.【F:src/server/gameplay/g_main.cpp†L785-L786】 |
| `g_ruleset` | `RS_Q2` | Active ruleset (Q2, Q3A, custom).【F:src/server/gameplay/g_main.cpp†L788-L789】 |
| `g_level_rulesets` | `0` | Allow BSP metadata to override rulesets.【F:src/server/gameplay/g_main.cpp†L869-L870】 |
| `g_instaGib` | `0` | Instagib modifier (latched).【F:src/server/match/match_state.cpp†L2306-L2316】 |
| `g_quadhog` / `g_nadeFest` / `g_frenzy` | `0` | Additional match modifiers toggled at load.【F:src/server/match/match_state.cpp†L2310-L2316】 |
| `g_vampiric_damage` | `0` | Enables vampiric life steal; see supporting cvars for scaling.【F:src/server/match/match_state.cpp†L2313-L2316】 |
| `g_allow_techs` | `auto` | Governs tech pickups across modes.【F:src/server/gameplay/g_main.cpp†L817-L819】 |
| `g_allow_grapple` | `auto` | Controls offhand grappling availability.【F:src/server/gameplay/g_main.cpp†L745-L750】 |
| `g_allow_forfeit` | `1` | Allows teams to `/forfeit`.【F:src/server/gameplay/g_main.cpp†L813-L816】 |
| `g_allow_kill` | `1` | Permit `/kill` command in lobbies.【F:src/server/gameplay/g_main.cpp†L745-L747】 |
| `g_allow_mymap` | `1` | Enables player map queue feature.【F:src/server/gameplay/g_main.cpp†L815-L817】 |
| `g_allow_voting` | `1` | Master toggle for callvote system.【F:src/server/gameplay/g_main.cpp†L820-L820】 |
| `g_allow_vote_mid_game` | `0` | Permit votes while a match is live.【F:src/server/gameplay/g_main.cpp†L819-L820】 |
| `g_allow_spec_vote` | `0` | Allow spectators to vote.【F:src/server/gameplay/g_main.cpp†L817-L818】 |

## Map Rotation & Overrides
| Cvar | Default | Notes |
| --- | --- | --- |
| `g_maps_pool_file` | `"mapdb.json"` | JSON pool enumerating map metadata.【F:src/server/gameplay/g_main.cpp†L909-L914】【F:docs/mapsystem.html†L24-L47】 |
| `g_maps_cycle_file` | `"mapcycle.txt"` | Text rotation consumed by `nextmap`.【F:src/server/gameplay/g_main.cpp†L909-L911】【F:docs/mapsystem.html†L50-L56】 |
| `g_maps_selector` | `1` | Enable end-of-match selector vote.【F:src/server/gameplay/g_main.cpp†L910-L912】【F:docs/mapsystem.html†L75-L84】 |
| `g_maps_mymap` | `1` | Allow MyMap queue submissions.【F:src/server/gameplay/g_main.cpp†L912-L914】【F:docs/mapsystem.html†L58-L72】 |
| `g_maps_allow_custom_textures` | `1` | Permit custom texture assets for pool entries.【F:src/server/gameplay/g_main.cpp†L913-L914】 |
| `g_maps_allow_custom_sounds` | `1` | Permit custom sound assets for pool entries.【F:src/server/gameplay/g_main.cpp†L913-L914】 |
| `g_entity_override_dir` | `"maps"` | Directory containing entity override files.【F:src/server/gameplay/g_main.cpp†L855-L857】 |
| `g_entity_override_load` | `1` | Load `.ent` overrides at runtime.【F:src/server/gameplay/g_main.cpp†L855-L857】 |
| `g_entity_override_save` | `0` | Save `.ent` overrides produced in-editor.【F:src/server/gameplay/g_main.cpp†L855-L857】 |
| `match_map_same_level` | `0` | Force Marathon legs to stay on current BSP when set.【F:src/server/gameplay/g_main.cpp†L849-L850】 |

## Physics & Movement
| Cvar | Default | Notes |
| --- | --- | --- |
| `g_gravity` | `800` | Global gravity acceleration.【F:src/server/gameplay/g_main.cpp†L717-L719】 |
| `g_air_accelerate` | `0` | Air-control acceleration scale.【F:src/server/gameplay/g_main.cpp†L812-L813】 |
| `g_max_velocity` | `2000` | Max player speed clamp.【F:src/server/gameplay/g_main.cpp†L717-L718】 |
| `g_roll_speed` / `g_roll_angle` | `200` / `2` | View roll response.【F:src/server/gameplay/g_main.cpp†L715-L716】 |
| `g_stop_speed` | `100` | Friction stop speed threshold.【F:src/server/gameplay/g_main.cpp†L722-L723】 |
| `g_knockback_scale` | `1.0` | Global knockback multiplier.【F:src/server/gameplay/g_main.cpp†L866-L867】 |
| `g_lag_compensation` | `1` | Enables unlagged hitscan compensation.【F:src/server/gameplay/g_main.cpp†L868-L869】 |
| `g_instant_weapon_switch` | `0` | Latched instant weapon swapping.【F:src/server/gameplay/g_main.cpp†L864-L865】 |
| `g_quick_weapon_switch` | `1` | Quick switch toggle (latched).【F:src/server/gameplay/g_main.cpp†L886-L887】 |
| `g_weapon_respawn_time` | `30` | Weapon respawn seconds for non-arena modes.【F:src/server/gameplay/g_main.cpp†L906-L907】 |
| `g_weapon_projection` | `0` | Enables weapon model projection offset.【F:src/server/gameplay/g_main.cpp†L905-L907】 |

## Player Experience & Messaging
| Cvar | Default | Notes |
| --- | --- | --- |
| `g_frag_messages` | `1` | Toggles frag feed overlay.【F:src/server/gameplay/g_main.cpp†L752-L753】 |
| `g_showmotd` / `g_showhelp` | `1` | Display MOTD/help prompts on connect.【F:src/server/gameplay/g_main.cpp†L888-L889】 |
| `g_motd_filename` | `"motd.txt"` | MOTD file path relative to game dir.【F:src/server/gameplay/g_main.cpp†L876-L878】 |
| `g_auto_screenshot_tool` | `0` | Enables automated intermission screenshots.【F:src/server/gameplay/g_main.cpp†L824-L825】 |
| `g_owner_auto_join` | `0` | Auto-join lobby owner to teams.【F:src/server/match/match_state.cpp†L2308-L2310】 |
| `g_owner_push_scores` | `1` | Push match scores to lobby owner HUD.【F:src/server/match/match_state.cpp†L2308-L2310】 |
| `g_inactivity` | `120` | Seconds before AFK kick triggers.【F:src/server/gameplay/g_main.cpp†L861-L863】 |
| `g_verbose` | `0` | Verbose console logging (entity spawn diagnostics).【F:src/server/gameplay/g_main.cpp†L897-L902】 |

## Health, Armor, and Items
| Cvar | Default | Notes |
| --- | --- | --- |
| `g_starting_health` | `100` | Base health on spawn.【F:src/server/gameplay/g_main.cpp†L891-L893】 |
| `g_starting_health_bonus` | `25` | Temporary bonus health on spawn.【F:src/server/gameplay/g_main.cpp†L891-L893】 |
| `g_starting_armor` | `0` | Armor granted on spawn.【F:src/server/gameplay/g_main.cpp†L891-L893】 |
| `g_start_items` | `""` | CSV list of starting items.【F:src/server/gameplay/g_main.cpp†L890-L891】 |
| `g_powerups` toggles (`g_no_powerups`, `g_no_items`, `g_no_mines`, `g_no_spheres`, `g_mapspawn_no_bfg`, etc.) | `0` | Disable specific pickups globally.【F:src/server/gameplay/g_main.cpp†L872-L885】 |
| `g_arena_starting_health` / `g_arena_starting_armor` | `200` | Round-based loadout values for Clan Arena modes.【F:src/server/gameplay/g_main.cpp†L821-L823】 |
| `g_arena_self_dmg_armor` | `0` | Mitigate self-damage via armor in arenas.【F:src/server/gameplay/g_main.cpp†L821-L823】 |
| `g_vampiric_percentile` | `0.67` | Fraction of damage converted to healing when vampiric mode active.【F:src/server/match/match_state.cpp†L2313-L2316】 |

## Voting & Administration
| Cvar | Default | Notes |
| --- | --- | --- |
| `g_allow_admin` | `1` | Enables admin login flow for console commands.【F:src/server/gameplay/g_main.cpp†L813-L814】 |
| `g_vote_flags` | `8191` (all votes) | Bitmask enabling specific vote topics.【F:src/server/gameplay/g_main.cpp†L901-L903】【F:docs/g_vote_flags.md†L1-L24】 |
| `g_vote_limit` | `3` | Votes per player each map.【F:src/server/gameplay/g_main.cpp†L903-L904】 |
| `g_allow_spec_vote` | `0` | Permit spectators to initiate votes.【F:src/server/gameplay/g_main.cpp†L817-L818】 |
| `g_allow_vote_mid_game` | `0` | Allow votes during active matches.【F:src/server/gameplay/g_main.cpp†L819-L820】 |
| `g_allow_voting` | `1` | Master toggle; disable to remove callvote entirely.【F:src/server/gameplay/g_main.cpp†L820-L820】 |

## Cooperative, Horde, and PvE
| Cvar | Default | Notes |
| --- | --- | --- |
| `g_coop_enable_lives` | `0` | Enable shared life pool in coop.【F:src/server/gameplay/g_main.cpp†L738-L744】 |
| `g_coop_num_lives` | `2` | Lives per player when coop lives enabled.【F:src/server/gameplay/g_main.cpp†L741-L744】 |
| `g_coop_squad_respawn` | `1` | Enable squad respawn mechanic.【F:src/server/gameplay/g_main.cpp†L739-L742】 |
| `g_coop_player_collision` | `0` | Toggle friendly collision in coop.【F:src/server/gameplay/g_main.cpp†L739-L740】 |
| `g_coop_instanced_items` | `1` | Instanced item pickups per player.【F:src/server/gameplay/g_main.cpp†L742-L743】 |
| `g_horde_starting_wave` | `1` | Initial wave index for Horde mode.【F:src/server/gameplay/g_main.cpp†L724-L736】 |
| `g_frozen_time` | `180` | Seconds a player stays frozen in Freeze Tag.【F:src/server/gameplay/g_main.cpp†L735-L737】 |
| `g_lms_num_lives` | `4` | Lives in Last Man/Team Standing.【F:src/server/gameplay/g_main.cpp†L743-L745】 |

## Bots & AI
| Cvar | Default | Notes |
| --- | --- | --- |
| `ai_allow_dm_spawn` | `0` | Allow AI to spawn in deathmatch.【F:src/server/gameplay/g_main.cpp†L806-L810】 |
| `ai_damage_scale` | `1` | Damage multiplier applied to AI.【F:src/server/gameplay/g_main.cpp†L806-L810】 |
| `ai_model_scale` | `0` | Model scale override for AI actors.【F:src/server/gameplay/g_main.cpp†L806-L810】 |
| `ai_movement_disabled` | `0` | Disable AI pathing (debug).【F:src/server/gameplay/g_main.cpp†L806-L810】 |
| `bot_name_prefix` | `"B|"` | Prefix applied to bot names.【F:src/server/gameplay/g_main.cpp†L811-L812】 |
| `bot_debug_follow_actor` / `bot_debug_move_to_point` | `0` | Debug draws for bot navigation.【F:src/server/gameplay/g_main.cpp†L757-L758】 |

## Analytics & Diagnostics
| Cvar | Default | Notes |
| --- | --- | --- |
| `g_matchstats` | `0` | Emit match summary JSON on completion.【F:src/server/gameplay/g_main.cpp†L875-L878】【F:docs/match_stats_json.md†L1-L31】 |
| `g_statex_enabled` | `1` | Enable stat export subsystem (StateX).【F:src/server/gameplay/g_main.cpp†L916-L917】 |
| `g_statex_humans_present` | `1` | Require human players before exporting stats.【F:src/server/gameplay/g_main.cpp†L916-L917】 |
| `g_owner_push_scores` | `1` | Broadcast scores to lobby host for integrations.【F:src/server/match/match_state.cpp†L2308-L2310】 |
| `g_auto_screenshot_tool` | `0` | Capture end-match screenshots for archives.【F:src/server/gameplay/g_main.cpp†L824-L825】 |

## Legacy Movement & View
| Cvar | Default | Notes |
| --- | --- | --- |
| `run_pitch` / `run_roll` | `0.002` / `0.005` | Player view bob controls.【F:src/server/gameplay/g_main.cpp†L796-L801】 |
| `bob_pitch` / `bob_roll` / `bob_up` | `0.002` / `0.002` / `0.005` | View bobbing intensities.【F:src/server/gameplay/g_main.cpp†L796-L801】 |
| `gun_x`, `gun_y`, `gun_z` | `0` | Weapon model offsets in view space.【F:src/server/gameplay/g_main.cpp†L711-L714】 |

## Miscellaneous
| Cvar | Default | Notes |
| --- | --- | --- |
| `g_disable_player_collision` | `0` | Disables player-player collision globally.【F:src/server/gameplay/g_main.cpp†L827-L828】 |
| `g_item_bobbing` | `1` | Toggle idle item bob animations.【F:src/server/gameplay/g_main.cpp†L865-L866】 |
| `g_fast_doors` | `1` | Speed multiplier for door movers.【F:src/server/gameplay/g_main.cpp†L858-L860】 |
| `g_frames_per_frame` | `1` | Internal simulation tick multiplier (debug).【F:src/server/gameplay/g_main.cpp†L858-L860】 |
| `g_friendly_fire_scale` | `1.0` | Scales friendly fire damage.【F:src/server/gameplay/g_main.cpp†L861-L862】 |
| `g_showmotd` | `1` | Show MOTD at spawn.【F:src/server/gameplay/g_main.cpp†L888-L889】 |
| `g_mapspawn_no_plasmabeam` / `g_mapspawn_no_bfg` | `0` | Disable specific weapons by map metadata.【F:src/server/gameplay/g_main.cpp†L872-L873】 |
| `g_mover_debug` | `0` | Extra logging for movers.【F:src/server/gameplay/g_main.cpp†L877-L878】 |
| `g_mover_speed_scale` | `1.0` | Multiply mover speeds for testing.【F:src/server/gameplay/g_main.cpp†L877-L878】 |

For legacy compatibility, WORR still honors engine-provided cvars like `dedicated`, `cheats`, and `skill`; confirm their values before relying on behavior in game logic.【F:src/server/gameplay/g_main.cpp†L760-L775】
