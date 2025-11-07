# WORR Client Manual

## Persistent Profiles and Preferences

### Profile storage
- Each player gets an individual JSON profile under `<gameversion>/pcfg/<socialID>.json`, ensuring stats and settings follow them across sessions.【F:src/server/gameplay/g_client_cfg.cpp†L37-L49】
- Profiles are created automatically when a new social ID joins, seeding identity, permissions, timestamps, HUD/audio defaults, and empty weapon preference arrays.【F:src/server/gameplay/g_client_cfg.cpp†L94-L141】

### Default client configuration
- New profiles enable crosshair IDs, frag messages, match timer, EyeCam, and the kill beep while disabling automatic follow behaviors until the player opts in.【F:src/server/gameplay/g_client_cfg.cpp†L105-L116】
- Follow preferences for killer, leader, and power-up carriers mirror the player’s current session flags when the file is created, so spectators can keep their preferred automation once they set it.【F:src/server/gameplay/g_client_cfg.cpp†L105-L116】

### Keeping configs current
- When a returning player connects, the loader tracks alias changes, rebuilds missing config/stat blocks, and backfills follow toggles and weapon preference arrays if older files lack them.【F:src/server/gameplay/g_client_cfg.cpp†L203-L257】
- The session copies persisted toggles (crosshair IDs, timers, frag messages, EyeCam, kill beep, and follow preferences) back onto the client so the HUD immediately reflects saved choices.【F:src/server/gameplay/g_client_cfg.cpp†L378-L396】

### Stored progression stats
- Profile stats record totals for matches, wins, losses, time played, and best skill rating, updating the last skill change after every save.【F:src/server/gameplay/g_client_cfg.cpp†L462-L507】
- Ghost participants increment an abandon counter while regular players log match results, keeping campaign history intact even if someone disconnects.【F:src/server/gameplay/g_client_cfg.cpp†L483-L499】

### When data is written
- Real players save their profile (including HUD toggles and weapon order) at the end of each match, and ghosts do the same using their recorded session data.【F:src/server/gameplay/g_client_cfg.cpp†L569-L609】
- Saves stamp the profile with the latest rating and configuration, guaranteeing subsequent sessions inherit the same preferences.【F:src/server/gameplay/g_client_cfg.cpp†L500-L559】

## HUD and Campaign Information

### End-of-Unit summary table
- Before broadcasting the summary, the game copies the current campaign tallies into the level entry and sorts the unit’s level list so rows appear in visit order.【F:src/server/player/p_hud_main.cpp†L102-L128】
- `EndOfUnitMessage` walks each level entry, rendering rows that show long names, kill/secret ratios, and per-map completion times while accumulating totals.【F:src/server/player/p_hud_main.cpp†L199-L225】【F:src/server/player/p_hud_main.cpp†L137-L155】
- If more than one visited level exists, a totals row is appended to underline aggregate progress before the layout is sent.【F:src/server/player/p_hud_main.cpp†L227-L229】【F:src/server/player/p_hud_main.cpp†L162-L167】

### Broadcast and player prompts
- The broadcast step packages the table, schedules a “press button” prompt five seconds after intermission starts, and flags every active client to show the End-of-Unit screen.【F:src/server/player/p_hud_main.cpp†L174-L191】
- The prompt text explicitly instructs everyone to press a button once the delay expires, making it clear how to advance after reviewing the summary.【F:src/server/player/p_hud_main.cpp†L174-L187】

### Campaign objective displays
- Interacting with the in-level help computer draws a HUD overlay with level name, localized objective headers, and counts for goals, kills, and secrets alongside the current skill level.【F:src/server/player/p_hud_main.cpp†L334-L385】
- Nintendo 64-specific levels collapse the layout to a simplified message block, but objective tracking still reports the same campaign counters.【F:src/server/player/p_hud_main.cpp†L352-L370】

### Cooperative and limited-life overlays
- Cooperative HUD stats show remaining lives whenever limited lives are active, zeroing the display otherwise.【F:src/server/player/p_hud_main.cpp†L398-L405】
- Horde and other campaign modes reuse the same stat slots to highlight remaining monsters and the current round number while a match is in progress.【F:src/server/player/p_hud_main.cpp†L408-L422】
- Respawn prompts display as localized string indices when a player is waiting to re-enter, helping squads coordinate.【F:src/server/player/p_hud_main.cpp†L425-L433】

## Console and Userinfo Options

### Identity and appearance
- Updating the `name` userinfo key changes the on-screen nickname, while `skin` selects a model/skin pair and updates the associated HUD portrait automatically.【F:src/server/player/p_client.cpp†L3284-L3301】
- The client packs the name and skin into the global configstring so teammates and chasers see the correct combination in scoreboards and chase cams.【F:src/server/player/p_client.cpp†L3302-L3315】

### Field of view and handedness
- The `fov` userinfo setting clamps the player’s view between 1° and 160°, giving wide-angle flexibility without breaking the HUD.【F:src/server/player/p_client.cpp†L3317-L3319】
- `hand` selects weapon handedness (right, left, or center), defaulting to right-handed if unset.【F:src/server/player/p_client.cpp†L3321-L3327】

### Weapon and defense automation (WORR-specific)
- `autoswitch` maps to WORR’s expanded auto-weapon behavior, letting players pick from “Smart” through “Never” while preserving sensible defaults.【F:src/server/player/p_client.cpp†L3329-L3335】
- `autoshield` enables WORR’s defensive trigger, with `-1` meaning “use game defaults” and other values turning the feature on explicitly.【F:src/server/player/p_client.cpp†L3337-L3342】

### Camera motion options (WORR-specific)
- The `bobskip` toggle removes weapon bobbing for players who prefer a steadier camera; any non-zero value enables the skip.【F:src/server/player/p_client.cpp†L3344-L3350】

### Follow behavior and spectator controls
- Spectators can tap attack to attach to a target or detach if already following someone, and use jump to cycle to the next player without leaving chase cam.【F:src/server/player/p_client.cpp†L4441-L4478】
- Automated follow preferences—follow last killer, current leader, or the active power-up carrier—are persisted in the profile config so these behaviors survive reconnects once toggled.【F:src/server/gameplay/g_client_cfg.cpp†L105-L116】【F:src/server/gameplay/g_client_cfg.cpp†L378-L396】
