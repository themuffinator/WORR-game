# WORR Cvar Reference

This reference consolidates the gameplay-facing console variables (cvars) that WORR registers on startup. Each table lists the default value supplied by the server module, the scope implied by its cvar flags, and practical usage notes derived from the systems that read the setting. Flag meanings follow the engine definitions (`CVAR_SERVERINFO` replicates to clients, `CVAR_LATCH` defers changes until the next map load, etc.).【F:src/server/gameplay/g_main.cpp†L709-L917】【F:src/server/match/match_state.cpp†L2295-L2351】【F:src/shared/game.hpp†L181-L203】

## Reading the tables

| Scope tag | Meaning |
| --- | --- |
| **Live** | `CVAR_NOFLAGS` – may be changed during a match. |
| **Serverinfo** | `CVAR_SERVERINFO` – published to clients via `serverinfo`. |
| **Userinfo** | `CVAR_USERINFO` – part of client authentication flow. |
| **Archive** | `CVAR_ARCHIVE` – persisted to config files. |
| **Latch** | `CVAR_LATCH` – applies after a map or server restart. |
| **NoSet** | `CVAR_NOSET` – locked at runtime; change from command line or config only. |

### Match flow & pacing

| Cvar | Default | Scope | Usage |
| --- | --- | --- | --- |
| `timelimit` | `0` | Serverinfo | Minutes before regulation ends; overtime adds on top when enabled.【F:src/server/gameplay/g_main.cpp†L777-L907】【F:src/server/match/match_state.cpp†L1534-L1540】【F:src/server/match/match_state.cpp†L2094-L2100】 |
| `fraglimit` | `0` | Serverinfo | Frag-based win condition for supported gametypes.【F:src/server/gameplay/g_main.cpp†L777-L907】【F:src/server/match/match_state.cpp†L2088-L2094】 |
| `capturelimit` | `8` | Serverinfo | Flag capture win condition for CTF-style modes.【F:src/server/gameplay/g_main.cpp†L777-L907】 |
| `roundlimit` | `8` | Serverinfo | Maximum number of rounds in elimination playlists.【F:src/server/gameplay/g_main.cpp†L777-L907】 |
| `roundtimelimit` | `2` | Serverinfo | Minutes allotted per round before sudden-death logic kicks in.【F:src/server/gameplay/g_main.cpp†L777-L907】 |
| `mercylimit` | `0` | Live | Ends lopsided matches early when a team exceeds the margin.【F:src/server/gameplay/g_main.cpp†L777-L907】 |
| `noplayerstime` | `10` | Live | Seconds the map stays active without human players before resetting.【F:src/server/gameplay/g_main.cpp†L777-L907】 |
| `marathon` | `0` | Serverinfo | Enables Marathon leg chaining; consult timers below.【F:src/server/gameplay/g_main.cpp†L777-L907】 |
| `g_marathon_timelimit` | `0` | Live | Overrides leg duration when Marathon mode is active.【F:src/server/gameplay/g_main.cpp†L777-L907】 |
| `g_marathon_scorelimit` | `0` | Live | Score cap per leg in Marathon runs.【F:src/server/gameplay/g_main.cpp†L777-L907】 |
| `match_start_no_humans` | `1` | Live | Defers countdown until a human joins the lobby.【F:src/server/gameplay/g_main.cpp†L820-L907】【F:src/server/match/match_state.cpp†L1296-L1321】 |
| `match_auto_join` | `1` | Live | Auto-slots players onto teams as they connect.【F:src/server/gameplay/g_main.cpp†L820-L907】 |
| `match_force_join` | `0` | Live | Forces ready players into teams when the match starts.【F:src/server/gameplay/g_main.cpp†L820-L907】 |
| `match_do_force_respawn` | `1` | Live | Re-spawns eliminated players after `match_force_respawn_time`.【F:src/server/gameplay/g_main.cpp†L820-L907】 |
| `match_force_respawn_time` | `2.4` | Live | Seconds before a forced respawn occurs.【F:src/server/gameplay/g_main.cpp†L820-L907】 |
| `match_do_overtime` | `120` | Live | Adds the listed seconds of overtime to 1v1 matches that tie at regulation.【F:src/server/gameplay/g_main.cpp†L820-L907】【F:src/server/match/match_state.cpp†L2098-L2100】 |
| `match_timeout_length` | `120` | Live | Duration (seconds) of player-called timeouts; `0` disables the feature.【F:src/server/gameplay/g_main.cpp†L820-L907】【F:src/server/commands/command_client.cpp†L937-L956】 |
| `warmup_enabled` | `1` | Live | Enables the warmup lobby cycle after map load.【F:src/server/gameplay/g_main.cpp†L820-L907】【F:src/server/match/match_state.cpp†L1296-L1331】 |
| `warmup_do_ready_up` | `0` | Live | Requires players to ready-up before the countdown starts.【F:src/server/gameplay/g_main.cpp†L820-L907】【F:src/server/match/match_state.cpp†L1296-L1331】 |
| `g_warmup_countdown` | `10` | Live | Seconds the start timer runs once ready-up conditions are satisfied.【F:src/server/gameplay/g_main.cpp†L888-L907】【F:src/server/match/match_state.cpp†L1674-L1676】 |
| `g_warmup_ready_percentage` | `0.51f` | Live | Fraction of ready players required when ready-up is enabled.【F:src/server/gameplay/g_main.cpp†L888-L907】【F:src/server/match/match_state.cpp†L1591-L1596】 |

### Player lifecycle & loadouts

| Cvar | Default | Scope | Usage |
| --- | --- | --- | --- |
| `match_crosshair_ids` | `1` | Live | Shows friendly names when aiming at teammates.【F:src/server/gameplay/g_main.cpp†L820-L907】 |
| `match_holdable_adrenaline` | `1` | Live | Allows adrenaline holdable pickup outside duel presets.【F:src/server/gameplay/g_main.cpp†L820-L907】 |
| `match_instant_items` | `1` | Live | Grants items immediately on pickup (no use delay).【F:src/server/gameplay/g_main.cpp†L820-L907】 |
| `match_items_respawn_rate` | `1.0` | Live | Global multiplier on item respawn timers.【F:src/server/gameplay/g_main.cpp†L820-L907】 |
| `match_player_respawn_min_delay` | `1` | Live | Minimum delay (seconds) before manual respawn becomes available.【F:src/server/gameplay/g_main.cpp†L820-L907】 |
| `match_player_respawn_min_distance` | `256` | Live | Minimum distance from enemies for preferred spawn picks.【F:src/server/gameplay/g_main.cpp†L820-L907】 |
| `match_player_respawn_min_distance_debug` | `0` | Live | Enables debug overlays for the spawn distance checks.【F:src/server/gameplay/g_main.cpp†L820-L907】 |
| `match_allow_spawn_pads` | `1` | Live | Permits spawn-pad entities in rotation.【F:src/server/gameplay/g_main.cpp†L820-L907】 |
| `match_allow_teleporter_pads` | `1` | Live | Keeps teleporter pads available for spawn logic.【F:src/server/gameplay/g_main.cpp†L820-L907】 |
| `match_weapons_stay` | `0` | Live | When set, weapon pickups persist after being claimed.【F:src/server/gameplay/g_main.cpp†L820-L907】 |
| `match_drop_cmd_flags` | `7` | Live | Bitmask of inventory types dropped with the `drop` command.【F:src/server/gameplay/g_main.cpp†L820-L907】 |
| `g_start_items` | `` | Live | Seed inventory items granted on spawn.【F:src/server/gameplay/g_main.cpp†L876-L907】 |
| `g_starting_health` | `100` | Live | Base health on spawn before bonuses apply.【F:src/server/gameplay/g_main.cpp†L876-L907】 |
| `g_starting_health_bonus` | `25` | Live | Temporary bonus health that decays to the base value.【F:src/server/gameplay/g_main.cpp†L876-L907】 |
| `g_starting_armor` | `0` | Live | Armor granted on spawn in addition to pickups.【F:src/server/gameplay/g_main.cpp†L876-L907】 |
| `g_arena_starting_health` | `200` | Live | Clan Arena spawn health for round-based loadouts.【F:src/server/gameplay/g_main.cpp†L820-L907】 |
| `g_arena_starting_armor` | `200` | Live | Clan Arena spawn armor.【F:src/server/gameplay/g_main.cpp†L820-L907】 |
| `g_arena_self_dmg_armor` | `0` | Live | Enables armor loss from self-damage in Arena variants.【F:src/server/gameplay/g_main.cpp†L820-L907】 |
| `g_weapon_respawn_time` | `30` | Live | Seconds between weapon respawns (unless `match_weapons_stay`).【F:src/server/gameplay/g_main.cpp†L888-L907】 |
| `g_instant_weapon_switch` | `0` | Latch | Makes weapon swaps immediate after restart.【F:src/server/gameplay/g_main.cpp†L852-L907】 |
| `g_quick_weapon_switch` | `1` | Latch | Enables legacy quick-swap handling post-restart.【F:src/server/gameplay/g_main.cpp†L876-L907】 |

### Voting, admin, and moderation

| Cvar | Default | Scope | Usage |
| --- | --- | --- | --- |
| `g_allow_admin` | `1` | Live | Allows persistent admin roster to take effect.【F:src/server/gameplay/g_main.cpp†L811-L907】【F:docs/wiki/server-hosts.md†L73-L96】 |
| `admin_password` | `` | Live | Optional shared secret for temporary admin elevation.【F:src/server/gameplay/g_main.cpp†L788-L794】 |
| `g_allow_voting` | `1` | Live | Enables player vote commands globally.【F:src/server/gameplay/g_main.cpp†L818-L907】 |
| `g_allow_vote_mid_game` | `0` | Live | Blocks new votes once a match is underway when `0`.【F:src/server/gameplay/g_main.cpp†L818-L907】 |
| `g_allow_spec_vote` | `0` | Live | Lets spectators initiate votes when enabled.【F:src/server/gameplay/g_main.cpp†L816-L907】 |
| `g_allow_forfeit` | `1` | Live | Unlocks the `forfeit` vote/command path.【F:src/server/gameplay/g_main.cpp†L813-L907】 |
| `g_vote_flags` | `8191` | Live | Bitmask of vote commands available to players; see [Voting Flags](../g_vote_flags.md).【F:src/server/gameplay/g_main.cpp†L901-L903】【F:src/server/commands/command_voting.hpp†L10-L37】【F:docs/g_vote_flags.md†L1-L74】 |
| `g_vote_limit` | `3` | Live | Number of failed votes before a player is throttled.【F:src/server/gameplay/g_main.cpp†L901-L904】 |
| `g_allow_mymap` | `1` | Live | Enables per-player MyMap queue submissions.【F:src/server/gameplay/g_main.cpp†L813-L907】【F:src/server/match/match_state.cpp†L1200-L1263】 |
| `g_allow_custom_skins` | `1` | Live | Lets clients stream custom skin assets when permitted by policy.【F:src/server/gameplay/g_main.cpp†L811-L907】 |
| `g_allow_admin` | `1` | Live | Keeps admin command set active for authenticated users.【F:src/server/gameplay/g_main.cpp†L811-L907】【F:docs/wiki/server-hosts.md†L97-L136】 |
| `g_owner_auto_join` | `0` | Live | Lets lobby owners auto-swap into open teams.【F:src/server/match/match_state.cpp†L2306-L2310】 |
| `g_owner_push_scores` | `1` | Live | Determines if lobby owners broadcast scoreboard updates.【F:src/server/match/match_state.cpp†L2306-L2310】 |

> **Persona tie-in:** Server operators should pair these settings with the [Server Host Guide](server-hosts.md) workflow advice, while players can review etiquette expectations in the [Player Guide](players.md#voting-and-match-etiquette).【F:docs/wiki/server-hosts.md†L65-L160】【F:docs/wiki/players.md†L37-L78】

### Map rotation, queues, and restrictions

| Cvar | Default | Scope | Usage |
| --- | --- | --- | --- |
| `g_maps_pool_file` | `mapdb.json` | Live | Source JSON for the curated map pool loader.【F:src/server/gameplay/g_main.cpp†L909-L914】【F:docs/wiki/server-hosts.md†L19-L44】 |
| `g_maps_cycle_file` | `mapcycle.txt` | Live | Plain-text rotation list consumed when the pool is empty.【F:src/server/gameplay/g_main.cpp†L909-L914】【F:docs/wiki/server-hosts.md†L45-L63】 |
| `g_maps_selector` | `1` | Live | Enables the end-of-match selector ballot.【F:src/server/gameplay/g_main.cpp†L909-L914】 |
| `g_maps_mymap` | `1` | Live | Allows personal queue submissions (see `g_allow_mymap`).【F:src/server/gameplay/g_main.cpp†L909-L914】【F:src/server/match/match_state.cpp†L1200-L1263】 |
| `g_maps_allow_custom_textures` | `1` | Live | Permits custom material references from map metadata.【F:src/server/gameplay/g_main.cpp†L909-L914】 |
| `g_maps_allow_custom_sounds` | `1` | Live | Same as above for audio packages.【F:src/server/gameplay/g_main.cpp†L909-L914】 |
| `match_maps_list` | `` | Live | Inline rotation fallback processed when the pool is empty.【F:src/server/gameplay/g_main.cpp†L870-L872】【F:src/server/match/match_state.cpp†L1216-L1263】 |
| `match_maps_list_shuffle` | `1` | Live | Shuffles the inline rotation once it loops, avoiding repeats.【F:src/server/gameplay/g_main.cpp†L870-L872】【F:src/server/match/match_state.cpp†L1236-L1263】 |
| `g_mapspawn_no_bfg` | `0` | Live | Strips BFG spawns from maps that include them.【F:src/server/gameplay/g_main.cpp†L870-L873】 |
| `g_mapspawn_no_plasmabeam` | `0` | Live | Removes Plasma Beam spawns when toggled.【F:src/server/gameplay/g_main.cpp†L870-L873】 |

### Gametype & ruleset management

| Cvar | Default | Scope | Usage |
| --- | --- | --- | --- |
| `g_gametype` | `0` (FFA) | Serverinfo | Primary gametype switch; forced into valid bounds at startup.【F:src/server/match/match_state.cpp†L2299-L2320】 |
| `g_ruleset` | `RS_Q2` | Serverinfo | Applies predefined gameplay rule bundles; auto-corrected via `CheckRuleset`.【F:src/server/gameplay/g_main.cpp†L788-L927】 |
| `g_level_rulesets` | `0` | Live | Lets maps override the active ruleset when set.【F:src/server/gameplay/g_main.cpp†L866-L873】 |
| `deathmatch` | `1` | Latch | Baseline deathmatch toggle; enforced when team modes require it.【F:src/server/match/match_state.cpp†L2299-L2339】 |
| `teamplay` | `0` | Serverinfo | Enables team scoring; locks max player count to even values.【F:src/server/match/match_state.cpp†L2299-L2348】 |
| `ctf` | `0` | Serverinfo | Activates Capture the Flag logic and disables coop/teamplay conflicts.【F:src/server/match/match_state.cpp†L2318-L2333】 |
| `coop` | `0` | Latch | Cooperative campaign toggle forced off when CTF or teamplay is active.【F:src/server/match/match_state.cpp†L2318-L2333】 |
| `g_instaGib` | `0` | Serverinfo + Latch | Enables instagib loadouts; requires restart.【F:src/server/match/match_state.cpp†L2305-L2310】 |
| `g_instagib_splash` | `0` | Live | Adds splash damage back into instagib variants.【F:src/server/match/match_state.cpp†L2305-L2310】 |
| `g_allow_techs` | `auto` | Live | Controls tech pickups; `auto` respects map metadata.【F:src/server/gameplay/g_main.cpp†L813-L907】 |
| `g_allow_grapple` | `auto` | Live | Allows the grapple weapon based on mode or explicit override.【F:src/server/gameplay/g_main.cpp†L739-L751】 |
| `g_grapple_offhand` | `0` | Live | Spawns grapple as an offhand item when enabled.【F:src/server/gameplay/g_main.cpp†L739-L751】 |
| `g_grapple_fly_speed` | `750` | Live | Projectile speed for grapple shots.【F:src/server/gameplay/g_main.cpp†L739-L751】 |
| `g_grapple_pull_speed` | `750` | Live | Pull speed once grappled.【F:src/server/gameplay/g_main.cpp†L739-L751】 |
| `g_grapple_damage` | `10` | Live | Optional grapple hit damage.【F:src/server/gameplay/g_main.cpp†L739-L751】 |
| `g_quadhog` | `0` | Serverinfo + Latch | Restricts Quad distribution to a single player at a time.【F:src/server/match/match_state.cpp†L2306-L2310】 |
| `g_nadeFest` | `0` | Serverinfo + Latch | Forces grenade-focused loadouts in supported modes.【F:src/server/match/match_state.cpp†L2306-L2312】 |
| `g_frenzy` | `0` | Serverinfo + Latch | Activates Frenzy rules for high-velocity matches.【F:src/server/match/match_state.cpp†L2306-L2312】 |
| `g_domination_*` | `see defaults` | Live | Tunes Domination tick cadence, capture speeds, and bonuses.【F:src/server/gameplay/g_main.cpp†L724-L733】【F:docs/domination.md†L13-L64】 |
| `g_coop_*` | `see defaults` | Latch | Coop campaign toggles for collision, lives, and instanced items.【F:src/server/gameplay/g_main.cpp†L738-L744】【F:docs/manual_qa_limited_lives.md†L1-L52】 |
| `g_lms_num_lives` | `4` | Latch | Starting lives for LMS/LTS variants.【F:src/server/gameplay/g_main.cpp†L738-L744】 |
| `g_vampiric_*` | `see defaults` | Live | Configures Vampiric Damage modifier thresholds.【F:src/server/match/match_state.cpp†L2313-L2316】 |
| `g_frozen_time` | `180` | Live | Freeze Tag thaw timer baseline.【F:src/server/gameplay/g_main.cpp†L735-L737】 |
| `g_horde_starting_wave` | `1` | Serverinfo + Latch | Entry wave for Horde mode sessions.【F:src/server/gameplay/g_main.cpp†L724-L727】 |

### Combat, movement, and physics

| Cvar | Default | Scope | Usage |
| --- | --- | --- | --- |
| `g_gravity` | `800` | Live | World gravity; affects jump arcs and projectile falloff.【F:src/server/gameplay/g_main.cpp†L709-L719】 |
| `g_air_accelerate` | `0` | Live | Air-control acceleration, useful for arena-style feel tweaks.【F:src/server/gameplay/g_main.cpp†L811-L814】 |
| `g_stop_speed` | `100` | Live | Base friction speed clamp for player movement.【F:src/server/gameplay/g_main.cpp†L721-L724】 |
| `g_max_velocity` | `2000` | Live | Caps player speed to prevent physics explosions.【F:src/server/gameplay/g_main.cpp†L715-L718】 |
| `g_falling_damage` | `1` | Live | Enables fall-damage calculations; disable for low-grav events.【F:src/server/gameplay/g_main.cpp†L820-L907】 |
| `g_self_damage` | `1` | Live | Governs self-inflicted damage from splash weapons.【F:src/server/gameplay/g_main.cpp†L820-L907】 |
| `g_knockback_scale` | `1.0` | Live | Global multiplier on knockback output.【F:src/server/gameplay/g_main.cpp†L852-L907】 |
| `g_lag_compensation` | `1` | Live | Toggles lag-compensated hit-scan rewinding.【F:src/server/gameplay/g_main.cpp†L852-L907】【F:src/server/commands/command_voting.cpp†L108-L175】 |
| `g_ladder_steps` | `1` | Live | Enables ladder step smoothing.【F:src/server/gameplay/g_main.cpp†L852-L907】 |
| `g_inactivity` | `120` | Live | Seconds of idle time before AFK handling kicks in.【F:src/server/gameplay/g_main.cpp†L852-L907】 |
| `g_friendly_fire_scale` | `1.0` | Live | Multiplier for team damage (0 disables friendly fire).【F:src/server/gameplay/g_main.cpp†L852-L907】 |
| `g_infinite_ammo` | `0` | Latch | Locks ammo counts after restart when enabled.【F:src/server/gameplay/g_main.cpp†L852-L907】 |

### Item & powerup availability

| Cvar | Default | Scope | Usage |
| --- | --- | --- | --- |
| `g_no_items` | `0` | Live | Removes all item pickups when set to `1`.【F:src/server/gameplay/g_main.cpp†L875-L885】 |
| `g_no_health` | `0` | Live | Disables health pickups.【F:src/server/gameplay/g_main.cpp†L875-L885】 |
| `g_no_armor` | `0` | Live | Disables armor pickups.【F:src/server/gameplay/g_main.cpp†L875-L885】 |
| `g_no_powerups` | `0` | Live | Strips major powerups.【F:src/server/gameplay/g_main.cpp†L875-L885】 |
| `g_no_spheres` | `0` | Live | Removes sphere powerups (Hunter, Vengeance, etc.).【F:src/server/gameplay/g_main.cpp†L875-L885】 |
| `g_no_mines` | `0` | Live | Disables mine pickups.【F:src/server/gameplay/g_main.cpp†L875-L885】 |
| `g_no_nukes` | `0` | Live | Disables nuke pickups.【F:src/server/gameplay/g_main.cpp†L875-L885】 |
| `g_mapspawn_no_bfg` | `0` | Live | Prevents BFG spawn placements.【F:src/server/gameplay/g_main.cpp†L870-L873】 |
| `g_mapspawn_no_plasmabeam` | `0` | Live | Prevents Plasma Beam spawns.【F:src/server/gameplay/g_main.cpp†L870-L873】 |
| `match_powerup_drops` | `1` | Live | Allows powerups to drop on death.【F:src/server/gameplay/g_main.cpp†L820-L907】 |
| `match_powerup_min_player_lock` | `0` | Live | Minimum players required before powerups spawn.【F:src/server/gameplay/g_main.cpp†L820-L907】 |

### Teamplay safeguards

| Cvar | Default | Scope | Usage |
| --- | --- | --- | --- |
| `g_teamplay_allow_team_pick` | `0` | Live | Allows players to choose teams manually.【F:src/server/gameplay/g_main.cpp†L895-L900】 |
| `g_teamplay_auto_balance` | `1` | Live | Auto-balances roster counts after deaths.【F:src/server/gameplay/g_main.cpp†L895-L900】 |
| `g_teamplay_force_balance` | `0` | Live | Forces immediate swaps to repair imbalance.【F:src/server/gameplay/g_main.cpp†L895-L900】 |
| `g_teamplay_item_drop_notice` | `1` | Live | Announces when teammates drop key items.【F:src/server/gameplay/g_main.cpp†L895-L900】 |
| `g_teamplay_armor_protect` | `0` | Live | Shares armor with teammates when enabled.【F:src/server/gameplay/g_main.cpp†L895-L900】 |
| `match_lock` | `0` | Serverinfo | Locks teams to stop late joins when `1`.【F:src/server/gameplay/g_main.cpp†L870-L876】 |
| `g_friendly_fire_scale` | `1.0` | Live | Adjusts friendly fire penalties.【F:src/server/gameplay/g_main.cpp†L852-L907】 |

### Analytics & telemetry

| Cvar | Default | Scope | Usage |
| --- | --- | --- | --- |
| `g_matchstats` | `0` | Live | When `1`, writes per-match JSON summaries for downstream analytics.【F:src/server/gameplay/g_main.cpp†L870-L876】【F:docs/match_stats_json.md†L1-L31】 |
| `g_statex_enabled` | `1` | Live | Enables STATEX match reporting at end-of-match.【F:src/server/gameplay/g_main.cpp†L909-L917】【F:src/server/match/match_logging.cpp†L1548-L1574】 |
| `g_statex_humans_present` | `1` | Live | Requires human players before analytics exports fire.【F:src/server/gameplay/g_main.cpp†L909-L917】【F:src/server/match/match_logging.cpp†L1555-L1564】 |
| `g_auto_screenshot_tool` | `0` | Live | Signals external tooling to capture end-game screenshots.【F:src/server/gameplay/g_main.cpp†L820-L907】 |

### Server identity & access control

| Cvar | Default | Scope | Usage |
| --- | --- | --- | --- |
| `hostname` | `Welcome to WORR!` | Live | Server name broadcast to browsers.【F:src/server/gameplay/g_main.cpp†L709-L717】 |
| `password` | `` | Userinfo | Gate for standard player connections.【F:src/server/gameplay/g_main.cpp†L788-L794】 |
| `spectator_password` | `` | Userinfo | Separates spectator-only access.【F:src/server/gameplay/g_main.cpp†L788-L794】 |
| `needpass` | `0` | Serverinfo | Flag advertised when a password is required.【F:src/server/gameplay/g_main.cpp†L788-L794】 |
| `filterban` | `1` | Live | Bans (1) or allows (0) addresses listed in `ban.txt`.【F:src/server/gameplay/g_main.cpp†L788-L795】 |
| `flood_msgs` | `4` | Live | Chat flood throttle limit (messages).【F:src/server/gameplay/g_main.cpp†L802-L804】 |
| `flood_persecond` | `4` | Live | Chat flood rate limit window.【F:src/server/gameplay/g_main.cpp†L802-L804】 |
| `flood_waitdelay` | `10` | Live | Cooldown seconds after flood is detected.【F:src/server/gameplay/g_main.cpp†L802-L804】 |

### Bots, AI, and debugging helpers

| Cvar | Default | Scope | Usage |
| --- | --- | --- | --- |
| `bot_name_prefix` | `B|` | Live | Prefix assigned to auto-generated bot names.【F:src/server/gameplay/g_main.cpp†L811-L814】 |
| `bot_debug_follow_actor` | `0` | Live | Prints debug for bot follow goals.【F:src/server/gameplay/g_main.cpp†L757-L758】 |
| `bot_debug_move_to_point` | `0` | Live | Logs navigation for pathing debug.【F:src/server/gameplay/g_main.cpp†L757-L758】 |
| `ai_allow_dm_spawn` | `0` | Live | Allows AI to spawn in DM for testing.【F:src/server/gameplay/g_main.cpp†L806-L809】 |
| `ai_damage_scale` | `1` | Live | Scales AI damage dealt.【F:src/server/gameplay/g_main.cpp†L806-L809】 |
| `ai_model_scale` | `0` | Live | Overrides AI model scale for prototyping.【F:src/server/gameplay/g_main.cpp†L806-L809】 |
| `ai_movement_disabled` | `0` | Live | Freezes AI movement when `1`.【F:src/server/gameplay/g_main.cpp†L806-L809】 |
| `g_debug_monster_paths` | `0` | Live | Enables path grid debug draws.【F:src/server/gameplay/g_main.cpp†L754-L755】 |
| `g_debug_monster_kills` | `0` | Latch | Tracks monster kill accounting post-restart.【F:src/server/gameplay/g_main.cpp†L754-L756】 |
| `g_mover_debug` | `0` | Live | Verbose mover logging for map debugging.【F:src/server/gameplay/g_main.cpp†L870-L878】 |
| `g_mover_speed_scale` | `1.0f` | Live | Adjusts mover speed globally for tests.【F:src/server/gameplay/g_main.cpp†L870-L878】 |
| `g_verbose` | `0` | Live | Turns on extra console logging in multiple systems.【F:src/server/gameplay/g_main.cpp†L895-L902】 |
| `g_frames_per_frame` | `1` | Live | Debug multiplier for running multiple simulation frames per tick.【F:src/server/gameplay/g_main.cpp†L852-L861】 |
| `g_skip_view_modifiers` | `0` | NoSet | Locks view modifier toggles for testing toolchains.【F:src/server/gameplay/g_main.cpp†L720-L722】 |
| `g_entity_override_dir` | `maps` | Live | Directory for entity override JSON files.【F:src/server/gameplay/g_main.cpp†L838-L857】 |
| `g_entity_override_load` | `1` | Live | Enables override loading.【F:src/server/gameplay/g_main.cpp†L838-L857】 |
| `g_entity_override_save` | `0` | Live | Dumps live entity overrides when enabled.【F:src/server/gameplay/g_main.cpp†L838-L857】 |

### Presentation & UI tweaks

| Cvar | Default | Scope | Usage |
| --- | --- | --- | --- |
| `g_showhelp` | `1` | Live | Enables contextual help popups.【F:src/server/gameplay/g_main.cpp†L886-L893】 |
| `g_showmotd` | `1` | Live | Shows the Message of the Day at connect.【F:src/server/gameplay/g_main.cpp†L886-L893】 |
| `g_motd_filename` | `motd.txt` | Live | File loaded for the MOTD banner.【F:src/server/gameplay/g_main.cpp†L870-L877】【F:src/server/gameplay/g_main.cpp†L780-L824】 |
| `run_pitch` | `0.002` | Live | View bob tuning (pitch).【F:src/server/gameplay/g_main.cpp†L796-L800】 |
| `run_roll` | `0.005` | Live | View bob tuning (roll).【F:src/server/gameplay/g_main.cpp†L796-L800】 |
| `bob_pitch` | `0.002` | Live | Weapon bob tuning (pitch).【F:src/server/gameplay/g_main.cpp†L796-L800】 |
| `bob_roll` | `0.002` | Live | Weapon bob tuning (roll).【F:src/server/gameplay/g_main.cpp†L796-L800】 |
| `bob_up` | `0.005` | Live | Weapon bob vertical component.【F:src/server/gameplay/g_main.cpp†L796-L800】 |
| `g_item_bobbing` | `1` | Live | Toggles idle bob animation on pickups.【F:src/server/gameplay/g_main.cpp†L852-L907】 |
| `g_eyecam` | `1` | Live | Enables cinematic camera sweeps during intermission.【F:src/server/gameplay/g_main.cpp†L838-L859】 |
| `owner_intermission_shots` | `0` | Live | Lets lobby owners pick intermission camera shots.【F:src/server/gameplay/g_main.cpp†L820-L907】 |

### Marathon, tournament, and misc. helpers

| Cvar | Default | Scope | Usage |
| --- | --- | --- | --- |
| `g_dm_exec_level_cfg` | `0` | Live | Executes `<mapname>.cfg` after load when set.【F:src/server/gameplay/g_main.cpp†L832-L907】【F:src/server/gameplay/g_main.cpp†L965-L976】 |
| `g_dm_random_items` | `0` | Live | Enables item randomizer for DM playlists.【F:src/server/gameplay/g_main.cpp†L724-L729】 |
| `g_dm_strong_mines` | `0` | Live | Buffs mines in DM playlists.【F:src/server/gameplay/g_main.cpp†L724-L729】 |
| `g_frag_messages` | `1` | Live | Broadcasts frag strings to clients.【F:src/server/gameplay/g_main.cpp†L751-L753】 |
| `g_owner_push_scores` | `1` | Live | Keeps lobby owners syncing scoreboard data.【F:src/server/match/match_state.cpp†L2306-L2310】 |
| `g_matchstats` | `0` | Live | Controls per-match stat dumps (see Analytics).【F:src/server/gameplay/g_main.cpp†L870-L876】 |
| `g_auto_screenshot_tool` | `0` | Live | Signals screenshot automation (see Analytics).【F:src/server/gameplay/g_main.cpp†L820-L907】 |

## Related persona guides

- [Server Host Guide](server-hosts.md) — Preset recommendations and rotation examples that build on these cvars.【F:docs/wiki/server-hosts.md†L19-L160】
- [Player Guide](players.md) — Explains how vote flags and warmup settings impact player etiquette.【F:docs/wiki/players.md†L37-L78】
- [Programmers Guide](programmers.md) — Pointers to the source modules that register and consume the cvars above.【F:docs/wiki/programmers.md†L1-L112】

Use these references together when planning configuration changes or validating gameplay behavior across environments.
