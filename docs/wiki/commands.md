# WORR Command Reference

This guide consolidates the console commands exported by WORR's server module, grouped by the jobs they solve for hosts, admins, and players. Each entry calls out syntax, permission requirements, and a practical console example.

## Permission flags at a glance

Commands expose access rules through `CommandFlag` bitmasks:

- **`AllowDead`** – usable while dead or in limbo.
- **`AllowIntermission`** – callable during the post-match screen.
- **`AllowSpectator`** – callable from spectator slots.
- **`MatchOnly`** – restricted to live match states.
- **`AdminOnly`** – requires an authenticated admin session.
- **`CheatProtect`** – requires cheats enabled (single-player or developer servers).

These flags appear in each `RegisterCommand` call alongside the handler function.【F:src/server/commands/command_system.hpp†L21-L32】【F:src/server/commands/command_admin.cpp†L484-L510】【F:src/server/commands/command_client.cpp†L1256-L1313】【F:src/server/commands/command_voting.cpp†L662-L668】【F:src/server/commands/command_cheats.cpp†L207-L223】 Flood-exempt commands are marked in the source with a trailing `true` argument.

---

## Admin & host workflows

### Moderation and policy

- **`add_admin`** — Adds a player's social ID to `admin.txt`, reloads the list, and announces the promotion.【F:src/server/commands/command_admin.cpp†L47-L71】  
  - *Syntax:* `add_admin <client#|name|social_id>`  
  - *Permissions:* Admin-only; allowed while spectating or during intermission.【F:src/server/commands/command_admin.cpp†L484-L510】  
  - *Example:* `\add_admin EOS:ABCDEF0123456789ABCDEF0123456789`
- **`remove_admin`** — Removes a social ID from `admin.txt` (except the lobby host) and refreshes the roster.【F:src/server/commands/command_admin.cpp†L296-L327】  
  - *Syntax:* `remove_admin <client#|name|social_id>`  
  - *Permissions:* Admin-only; usable while spectating or during intermission.【F:src/server/commands/command_admin.cpp†L484-L510】  
  - *Example:* `\remove_admin EOS:...`
- **`add_ban`** — Resolves the target to a social ID, blocks banning listed admins or the host, and appends the ID to `ban.txt`.【F:src/server/commands/command_admin.cpp†L75-L107】  
  - *Syntax:* `add_ban <client#|name|social_id>`  
  - *Permissions:* Admin-only; usable while spectating or during intermission.【F:src/server/commands/command_admin.cpp†L484-L510】  
  - *Example:* `\add_ban NX:12345678901234567`
- **`remove_ban`** — Removes a social ID from `ban.txt` and reloads the ban cache.【F:src/server/commands/command_admin.cpp†L329-L335】  
  - *Syntax:* `remove_ban <social_id>`  
  - *Permissions:* Admin-only; usable while spectating or during intermission.【F:src/server/commands/command_admin.cpp†L484-L510】  
  - *Example:* `\remove_ban Steamworks:7656119...`
- **`load_admins`** / **`load_bans`** — Reload the cached admin or ban list after manual file edits.【F:src/server/commands/command_admin.cpp†L229-L238】  
  - *Syntax:* `load_admins` or `load_bans`  
  - *Permissions:* Admin-only; usable while spectating or during intermission.【F:src/server/commands/command_admin.cpp†L484-L510】  
  - *Example:* `\load_admins`
- **`boot`** — Kicks a player unless they are the lobby host or an admin, using the engine's `kick` command behind the scenes.【F:src/server/commands/command_admin.cpp†L149-L170】  
  - *Syntax:* `boot <client name|number>`  
  - *Permissions:* Admin-only; usable while spectating or during intermission.【F:src/server/commands/command_admin.cpp†L484-L510】  
  - *Example:* `\boot troublemaker`

### Match control and rosters

- **`start_match`** — Forces the match out of warmup into live play when it has not already started.【F:src/server/commands/command_admin.cpp†L441-L447】  
  - *Syntax:* `start_match`  
  - *Permissions:* Admin-only; usable while spectating or during intermission.【F:src/server/commands/command_admin.cpp†L484-L510】  
  - *Example:* `\start_match`
- **`end_match`** — Triggers intermission immediately if a match is in progress and not already finished.【F:src/server/commands/command_admin.cpp†L173-L183】  
  - *Syntax:* `end_match`  
  - *Permissions:* Admin-only; usable while spectating or during intermission.【F:src/server/commands/command_admin.cpp†L484-L510】  
  - *Example:* `\end_match`
- **`reset_match`** — Restarts the live match after confirming it has begun and is not already at intermission.【F:src/server/commands/command_admin.cpp†L346-L356】  
  - *Syntax:* `reset_match`  
  - *Permissions:* Admin-only; usable while spectating or during intermission.【F:src/server/commands/command_admin.cpp†L484-L510】  
  - *Example:* `\reset_match`
- **`map_restart`** — Issues a `gamemap` reload for the current BSP, effectively soft-restarting the server session.【F:src/server/commands/command_admin.cpp†L277-L282】  
  - *Syntax:* `map_restart`  
  - *Permissions:* Admin-only; usable while spectating or during intermission.【F:src/server/commands/command_admin.cpp†L484-L510】  
  - *Example:* `\map_restart`
- **`ready_all`** / **`unready_all`** — Force every player into or out of ready state after validating warmup requirements.【F:src/server/commands/command_admin.cpp†L288-L295】【F:src/server/commands/command_admin.cpp†L472-L478】  
  - *Syntax:* `ready_all` / `unready_all`  
  - *Permissions:* Admin-only; usable while spectating or during intermission.【F:src/server/commands/command_admin.cpp†L484-L510】  
  - *Example:* `\ready_all`
- **`force_vote`** — Resolves the current vote to yes or no, applying the action or cancelling it immediately.【F:src/server/commands/command_admin.cpp†L185-L207】  
  - *Syntax:* `force_vote <yes|no>`  
  - *Permissions:* Admin-only; usable while spectating or during intermission.【F:src/server/commands/command_admin.cpp†L484-L510】  
  - *Example:* `\force_vote yes`
- **`balance`** — Calls the team balancer to redistribute players by score/skill.【F:src/server/commands/command_admin.cpp†L144-L147】  
  - *Syntax:* `balance`  
  - *Permissions:* Admin-only; usable while spectating or during intermission.【F:src/server/commands/command_admin.cpp†L487-L510】  
  - *Example:* `\balance`
- **`shuffle`** — Shuffles teams using the skill-based algorithm.【F:src/server/commands/command_admin.cpp†L436-L439】  
  - *Syntax:* `shuffle`  
  - *Permissions:* Admin-only; usable while spectating or during intermission.【F:src/server/commands/command_admin.cpp†L507-L510】  
  - *Example:* `\shuffle`
- **`set_team`** — Moves a specific client to a named team, validating gametype restrictions and skipping redundant moves.【F:src/server/commands/command_admin.cpp†L404-L435】  
  - *Syntax:* `set_team <client> <red|blue|spectator|free>`  
  - *Permissions:* Admin-only; usable while spectating or during intermission.【F:src/server/commands/command_admin.cpp†L505-L510】  
  - *Example:* `\set_team 7 blue`
- **`lock_team`** / **`unlock_team`** — Toggle whether Red or Blue can accept joins, with feedback if the requested state already matches.【F:src/server/commands/command_admin.cpp†L255-L274】【F:src/server/commands/command_admin.cpp†L450-L470】  
  - *Syntax:* `lock_team <red|blue>` / `unlock_team <red|blue>`  
  - *Permissions:* Admin-only; usable while spectating or during intermission.【F:src/server/commands/command_admin.cpp†L497-L510】  
  - *Example:* `\lock_team red`

### Map, rules, and arena tools

- **`set_map`** — Validates the requested map against the pool, announces the change, and queues a level transition.【F:src/server/commands/command_admin.cpp†L377-L432】  
  - *Syntax:* `set_map <mapname>`  
  - *Permissions:* Admin-only; usable while spectating or during intermission.【F:src/server/commands/command_admin.cpp†L505-L510】  
  - *Example:* `\set_map q2dm1`
- **`next_map`** — Ends the match to advance into the rotation-defined next map.【F:src/server/commands/command_admin.cpp†L283-L286】  
  - *Syntax:* `next_map`  
  - *Permissions:* Admin-only; usable while spectating or during intermission.【F:src/server/commands/command_admin.cpp†L499-L510】  
  - *Example:* `\next_map`
- **`arena`** — Reports the current arena when called without arguments or forces a new arena after validation in multi-arena modes.【F:src/server/commands/command_admin.cpp†L109-L142】  
  - *Syntax:* `arena [number]`  
  - *Permissions:* Admin-only; callable while spectating (intermission restricted).【F:src/server/commands/command_admin.cpp†L486-L510】  
  - *Example:* `\arena 3`
- **`gametype`** — Lists available gametypes on `?` and switches modes when supplied a valid identifier.【F:src/server/commands/command_admin.cpp†L209-L227】  
  - *Syntax:* `gametype <name>`  
  - *Permissions:* Admin-only; usable while spectating or during intermission.【F:src/server/commands/command_admin.cpp†L491-L510】  
  - *Example:* `\gametype tdm`
- **`ruleset`** — Shows the active ruleset and applies a new one by identifier, updating the `g_ruleset` cvar.【F:src/server/commands/command_admin.cpp†L359-L373】  
  - *Syntax:* `ruleset <q1|q2|q3a>`  
  - *Permissions:* Admin-only; usable while spectating or during intermission.【F:src/server/commands/command_admin.cpp†L504-L510】  
  - *Example:* `\ruleset q3a`
- **`load_mappool`** / **`load_mapcycle`** — Reload map pool and rotation definitions from disk, optionally chaining both reloads for the pool command.【F:src/server/commands/command_admin.cpp†L244-L254】  
  - *Syntax:* `load_mappool` / `load_mapcycle`  
  - *Permissions:* Admin-only; usable while spectating or during intermission.【F:src/server/commands/command_admin.cpp†L495-L510】  
  - *Example:* `\load_mappool`
- **`load_motd`** — Reloads the Message of the Day text into memory.【F:src/server/commands/command_admin.cpp†L239-L243】  
  - *Syntax:* `load_motd`  
  - *Permissions:* Admin-only; usable while spectating or during intermission.【F:src/server/commands/command_admin.cpp†L494-L510】  
  - *Example:* `\load_motd`

---

## Player & spectator utilities

### Access, readiness, and team flow

- **`admin`** — Displays admin status or consumes the configured password to grant admin rights when allowed.【F:src/server/commands/command_client.cpp†L74-L98】  
  - *Syntax:* `admin [password]`  
  - *Permissions:* Available to all players and spectators, including during intermission.【F:src/server/commands/command_client.cpp†L1256-L1313】  
  - *Example:* `\admin mySecret`
- **`team`** — Shows current team when used without arguments or switches to the requested team (spectator/free validated).【F:src/server/commands/command_client.cpp†L902-L921】  
  - *Syntax:* `team [red|blue|spectator|free]`  
  - *Permissions:* Usable while dead or spectating.【F:src/server/commands/command_client.cpp†L1299-L1301】  
  - *Example:* `\team red`
- **`ready`**, **`ready_up`/`readyup`**, **`notready`** — Toggles a player's ready state or toggles the “ready up” countdown flag after verifying warmup conditions.【F:src/server/commands/command_client.cpp†L789-L813】  
  - *Syntax:* `ready` | `ready_up` | `readyup` | `notready`  
  - *Permissions:* Usable while dead; flood-exempt for quick warmup coordination.【F:src/server/commands/command_client.cpp†L1291-L1295】  
  - *Example:* `\ready`
- **`forfeit`** — Allows the losing Duel/Gauntlet player to concede mid-match when forfeits are enabled.【F:src/server/commands/command_client.cpp†L424-L443】  
  - *Syntax:* `forfeit`  
  - *Permissions:* Usable while dead; flood-exempt.【F:src/server/commands/command_client.cpp†L1270-L1271】  
  - *Example:* `\forfeit`
- **`timeout`** / **`timein`** — Starts a timeout (respecting per-player limits) or ends one if called by the owner or an admin.【F:src/server/commands/command_client.cpp†L937-L959】【F:src/server/commands/command_client.cpp†L924-L935】  
  - *Syntax:* `timeout` / `timein`  
  - *Permissions:* Usable while dead or spectating.【F:src/server/commands/command_client.cpp†L1301-L1303】  
  - *Example:* `\timeout`
- **`timer`** — Toggles the HUD match timer widget for the caller.【F:src/server/commands/command_client.cpp†L961-L964】  
  - *Syntax:* `timer`  
  - *Permissions:* Usable while dead or spectating.【F:src/server/commands/command_client.cpp†L1302-L1304】  
  - *Example:* `\timer`
- **`putaway`** — Closes active menus, scoreboards, and help screens, restoring the regular HUD.【F:src/server/commands/command_client.cpp†L794-L803】  
  - *Syntax:* `putaway`  
  - *Permissions:* Usable while spectating.【F:src/server/commands/command_client.cpp†L1291-L1293】  
  - *Example:* `\putaway`

### Spectating and camera tools

- **`follow`** — As a spectator, lock the camera onto a named or numbered player after validating they are active.【F:src/server/commands/command_client.cpp†L388-L407】  
  - *Syntax:* `follow <client name|number>`  
  - *Permissions:* Usable by spectators or dead players; flood-exempt for menu bindings.【F:src/server/commands/command_client.cpp†L1266-L1269】  
  - *Example:* `\follow 3`
- **`followkiller`**, **`followleader`**, **`followpowerup`** — Toggle auto-follow helpers that snap to your killer, top fragger, or current powerup carrier.【F:src/server/commands/command_client.cpp†L409-L422】  
  - *Syntax:* `followkiller` | `followleader` | `followpowerup`  
  - *Permissions:* Usable by spectators or dead players; flood-exempt.【F:src/server/commands/command_client.cpp†L1267-L1269】  
  - *Example:* `\followpowerup`
- **`eyecam`** — Toggles EyeCam first-person spectating mode.【F:src/server/commands/command_client.cpp†L378-L382】  
  - *Syntax:* `eyecam`  
  - *Permissions:* Spectator-only.【F:src/server/commands/command_client.cpp†L1264-L1265】  
  - *Example:* `\eyecam`
- **`id`** — Toggles crosshair nameplates on hovered players.【F:src/server/commands/command_client.cpp†L474-L477】  
  - *Syntax:* `id`  
  - *Permissions:* Usable while dead or spectating.【F:src/server/commands/command_client.cpp†L1273-L1274】  
  - *Example:* `\id`
- **`where`** — Prints and copies your current coordinates and view angles for callouts or bug reports.【F:src/server/commands/command_client.cpp†L1246-L1254】  
  - *Syntax:* `where`  
  - *Permissions:* Spectator-only.【F:src/server/commands/command_client.cpp†L1312-L1313】  
  - *Example:* `\where`

### Inventory, weapons, and mobility

- **`drop`** / **`dropindex`** — Drops an item by name, special keyword (`tech`/`weapon`), or inventory index, respecting server drop restrictions and notifying teammates when enabled.【F:src/server/commands/command_client.cpp†L250-L376】  
  - *Syntax:* `drop <item|tech|weapon>` or `dropindex <index>`  
  - *Permissions:* Available to active players (no special flags).【F:src/server/commands/command_client.cpp†L1262-L1264】  
  - *Example:* `\drop quad`
- **`invdrop`** — Drops the currently selected inventory item if it supports dropping.【F:src/server/commands/command_client.cpp†L608-L621】  
  - *Syntax:* `invdrop`  
  - *Permissions:* Available to active players.【F:src/server/commands/command_client.cpp†L1275-L1276】  
  - *Example:* `\invdrop`
- **`inven`** — Toggles the join/inventory menu in multiplayer or the inventory display in other modes.【F:src/server/commands/command_client.cpp†L623-L652】  
  - *Syntax:* `inven`  
  - *Permissions:* Usable while dead or spectating; flood-exempt for menu binding.【F:src/server/commands/command_client.cpp†L1276-L1277】  
  - *Example:* `\inven`
- **`invnext` / `invprev`** (and `invnextp`, `invprevp`, `invnextw`, `invprevw`) — Cycle inventory selection across items, powerups, or weapons only.【F:src/server/commands/command_client.cpp†L654-L659】  
  - *Syntax:* `invnext`, `invprev`, `invnextp`, `invprevp`, `invnextw`, `invprevw`  
  - *Permissions:* Some variants require spectator/intermission access for menu navigation.【F:src/server/commands/command_client.cpp†L1277-L1282】  
  - *Example:* `\invnextw`
- **`invuse`** — Uses the currently selected item or activates the highlighted menu entry when a menu is open.【F:src/server/commands/command_client.cpp†L661-L682】  
  - *Syntax:* `invuse`  
  - *Permissions:* Usable while spectating or during intermission; flood-exempt.【F:src/server/commands/command_client.cpp†L1283-L1284】  
  - *Example:* `\invuse`
- **`use`**, **`use_only`**, **`use_index`**, **`use_index_only`** — Activates an item by name (including holdable shortcuts) or by inventory index, optionally suppressing weapon switches.【F:src/server/commands/command_client.cpp†L970-L1007】  
  - *Syntax:* `use <item_name>` or `use_index <index>`; `_only` variants suppress weapon chains  
  - *Permissions:* Available to active players; flood-exempt for binds.【F:src/server/commands/command_client.cpp†L1304-L1308】  
  - *Example:* `\use holdable`
- **`weapnext` / `weapprev` / `weaplast`** — Cycle to the next, previous, or last-used weapon, honouring owned inventory and weapon usability.【F:src/server/commands/command_client.cpp†L1197-L1240】  
  - *Syntax:* `weapnext`, `weapprev`, `weaplast`  
  - *Permissions:* Available to active players; flood-exempt for smooth weapon binds.【F:src/server/commands/command_client.cpp†L1310-L1312】  
  - *Example:* `\weapprev`
- **`impulse`** — Provides Quake 1-style impulse shortcuts for weapon selection, cheats, and special actions (e.g., `9` gives all weapons when cheats are allowed).【F:src/server/commands/command_client.cpp†L492-L606】  
  - *Syntax:* `impulse <0..255>`  
  - *Permissions:* Available to active players (cheat-protected actions still respect server cheat gating).【F:src/server/commands/command_client.cpp†L1274-L1275】  
  - *Example:* `\impulse 10`
- **`hook`** / **`unhook`** — Fires or resets the grapple when grapple support is enabled.【F:src/server/commands/command_client.cpp†L469-L472】【F:src/server/commands/command_client.cpp†L966-L968】  
  - *Syntax:* `hook` / `unhook`  
  - *Permissions:* Available to active players; flood-exempt for quick grappling binds.【F:src/server/commands/command_client.cpp†L1272-L1305】  
  - *Example:* `\hook`
- **`kill`** — Self-destructs the player after spawn protection and cheat checks clear, counting as a suicide.【F:src/server/commands/command_client.cpp†L697-L705】  
  - *Syntax:* `kill`  
  - *Permissions:* Available to active players.【F:src/server/commands/command_client.cpp†L1285-L1286】  
  - *Example:* `\kill`
- **`wave`** — Performs gestures (point, wave, salute, taunt, flip-off) and pings teammates when pointing at map items or players.【F:src/server/commands/command_client.cpp†L1009-L1194】  
  - *Syntax:* `wave [0-4]` (defaults to flip-off; `1` point, `2` wave, `3` salute, `4` taunt)  
  - *Permissions:* Available to active players.【F:src/server/commands/command_client.cpp†L1309-L1310】  
  - *Example:* `\wave 1`

### Information, HUD, and analytics

- **`clientlist`** — Prints a sortable table of connected clients with IDs, skill rating, ping, and team data.【F:src/server/commands/command_client.cpp†L100-L248】  
  - *Syntax:* `clientlist [score|time|name]`  
  - *Permissions:* Usable while dead, spectating, or during intermission.【F:src/server/commands/command_client.cpp†L1261-L1262】  
  - *Example:* `\clientlist name`
- **`help`** — Shows the help computer in non-deathmatch games or falls back to the scoreboard in competitive modes.【F:src/server/commands/command_client.cpp†L445-L467】  
  - *Syntax:* `help`  
  - *Permissions:* Usable while dead or spectating; flood-exempt.【F:src/server/commands/command_client.cpp†L1271-L1272】  
  - *Example:* `\help`
- **`score`** — Toggles the multiplayer scoreboard, closing menus first when necessary.【F:src/server/commands/command_client.cpp†L815-L837】  
  - *Syntax:* `score`  
  - *Permissions:* Usable while dead, spectating, or during intermission; flood-exempt.【F:src/server/commands/command_client.cpp†L1296-L1297】  
  - *Example:* `\score`
- **`fm`** / **`kb`** — Toggle frag message popups or switch the kill beep sound set, cycling presets when no value is supplied.【F:src/server/commands/command_client.cpp†L383-L386】【F:src/server/commands/command_client.cpp†L684-L695】  
  - *Syntax:* `fm` | `kb [0-4]`  
  - *Permissions:* Usable while dead or spectating.【F:src/server/commands/command_client.cpp†L1265-L1285】  
  - *Example:* `\kb 3`
- **`mapinfo`** — Prints the current map’s filename, long name, and authorship metadata.【F:src/server/commands/command_client.cpp†L707-L725】  
  - *Syntax:* `mapinfo`  
  - *Permissions:* Usable while dead or spectating.【F:src/server/commands/command_client.cpp†L1287-L1288】  
  - *Example:* `\mapinfo`
- **`mappool`** / **`mapcycle`** — Lists maps from the pool or cycle, with optional string filtering, and reports totals.【F:src/server/commands/command_client.cpp†L727-L735】  
  - *Syntax:* `mappool [filter]` / `mapcycle [filter]`  
  - *Permissions:* Usable while dead or spectating.【F:src/server/commands/command_client.cpp†L1287-L1288】  
  - *Example:* `\mappool q2dm`
- **`motd`** — Displays the server Message of the Day if one is loaded.【F:src/server/commands/command_client.cpp†L737-L744】  
  - *Syntax:* `motd`  
  - *Permissions:* Usable while spectating or during intermission.【F:src/server/commands/command_client.cpp†L1289-L1290】  
  - *Example:* `\motd`
- **`mymap`** — Queues a personal map pick (with optional rule modifiers) when `g_maps_mymap` is enabled and the player is authenticated.【F:src/server/commands/command_client.cpp†L746-L777】  
  - *Syntax:* `mymap <mapname> [+flag] [-flag] ...`  
  - *Permissions:* Usable while dead or spectating.【F:src/server/commands/command_client.cpp†L1289-L1290】  
  - *Example:* `\mymap q2dm1 +pu`
- **`sr`** — Reports the player’s skill rating alongside the server average for the current gametype.【F:src/server/commands/command_client.cpp†L779-L787】  
  - *Syntax:* `sr`  
  - *Permissions:* Usable while dead or spectating.【F:src/server/commands/command_client.cpp†L1298-L1299】  
  - *Example:* `\sr`
- **`stats`** — Stubbed CTF stats dump (announces availability when CTF flags are active).【F:src/server/commands/command_client.cpp†L893-L900】  
  - *Syntax:* `stats`  
  - *Permissions:* Usable during intermission or as a spectator.【F:src/server/commands/command_client.cpp†L1299-L1300】  
  - *Example:* `\stats`

---

## Voting commands

- **`callvote`** / **`cv`** — Launches a vote if voting is enabled, enforcing warmup rules, per-player limits, and `g_vote_flags`. Lists available vote types when called with no arguments.【F:src/server/commands/command_voting.cpp†L487-L617】【F:src/server/commands/command_voting.cpp†L320-L479】  
  - *Syntax:* `callvote <command> [params]` (alias `cv`)  
  - *Permissions:* Usable while dead or spectating when the server allows spectator votes.【F:src/server/commands/command_voting.cpp†L665-L666】【F:src/server/commands/command_voting.cpp†L400-L423】  
  - *Example:* `\callvote map q2dm2`
- **`vote`** — Casts a yes/no ballot on the active vote, accepting shorthand `1`/`0` and preventing double voting.【F:src/server/commands/command_voting.cpp†L619-L658】  
  - *Syntax:* `vote <yes|no>`  
  - *Permissions:* Usable while dead (live players only).【F:src/server/commands/command_voting.cpp†L667-L668】  
  - *Example:* `\vote yes`

Enabled vote types and their validation logic (map, restart, shuffle, unlagged, cointoss, random, arena, etc.) are registered through `RegisterVoteCommand`, which enforces argument counts, gametype eligibility, and cooldowns before the vote menu appears.【F:src/server/commands/command_voting.cpp†L320-L479】 Consult the server host guide for the `g_vote_flags` matrix.

---

## Cheat & debug commands (development servers only)

These commands are guarded by `CheatProtect` and require cheats to be enabled via server settings.【F:src/server/commands/command_cheats.cpp†L207-L223】 Use them on private or test servers.

- **`alert_all`** — Forces all monsters to target you immediately.【F:src/server/commands/command_cheats.cpp†L40-L49】  
  - *Syntax:* `alert_all`
  - *Example:* `\alert_all`
- **`check_poi`** — Reports whether the current Point of Interest is visible from your position using PVS tests.【F:src/server/commands/command_cheats.cpp†L51-L58】  
  - *Syntax:* `check_poi`
  - *Example:* `\check_poi`
- **`clear_ai_enemy`** — Sets an AI flag telling all monsters to forget their current enemy.【F:src/server/commands/command_cheats.cpp†L61-L69】  
  - *Syntax:* `clear_ai_enemy`
  - *Example:* `\clear_ai_enemy`
- **`give`** — Grants specific items, ammo, or the entire inventory when `all` is supplied; optional count supported for ammo-like items.【F:src/server/commands/command_cheats.cpp†L71-L104】  
  - *Syntax:* `give <item_name|all|health|...> [count]`
  - *Example:* `\give rocketlauncher`
- **`god`** / **`immortal`** — Toggles god mode or immortal mode flags, printing the resulting state.【F:src/server/commands/command_cheats.cpp†L106-L114】  
  - *Syntax:* `god` / `immortal`
  - *Example:* `\god`
- **`kill_ai`** — Removes all active monsters from the level.【F:src/server/commands/command_cheats.cpp†L116-L124】  
  - *Syntax:* `kill_ai`
  - *Example:* `\kill_ai`
- **`list_entities`** / **`list_monsters`** — Dump entity or monster tables (with positions for monsters) to the console.【F:src/server/commands/command_cheats.cpp†L126-L149】  
  - *Syntax:* `list_entities` / `list_monsters`
  - *Example:* `\list_monsters`
- **`noclip`**, **`notarget`**, **`novisible`** — Toggle collision, targeting, or visibility flags respectively, printing ON/OFF feedback.【F:src/server/commands/command_cheats.cpp†L151-L164】  
  - *Syntax:* `noclip` | `notarget` | `novisible`
  - *Example:* `\noclip`
- **`set_poi`** / **`check_poi`** — Sets or inspects the Point of Interest used by other tools.【F:src/server/commands/command_cheats.cpp†L166-L170】【F:src/server/commands/command_cheats.cpp†L51-L58】  
  - *Syntax:* `set_poi`
  - *Example:* `\set_poi`
- **`target`** — Fires all entities that match the supplied `targetname`, mimicking trigger relays.【F:src/server/commands/command_cheats.cpp†L172-L179】  
  - *Syntax:* `target <target_name>`
  - *Example:* `\target door_trigger`
- **`teleport`** — Warps the player to specified coordinates with optional pitch/yaw/roll overrides.【F:src/server/commands/command_cheats.cpp†L181-L204】  
  - *Syntax:* `teleport <x> <y> <z> [pitch] [yaw] [roll]`
  - *Example:* `\teleport 0 0 128`

---

## Quick cross-links

- Hosts: the **Admin Command Quick Reference** now links here for deeper coverage—pair it with the `g_vote_flags` presets in the [Server Host Guide](server-hosts.md).  
- Players: skim the [Voting and Match Etiquette](players.md#voting-and-match-etiquette) section after reviewing relevant player commands above.  
- Need more automation context? The programmers guide outlines how commands register with the dispatcher in `command_system.cpp`.

