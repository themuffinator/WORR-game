# WORR Map System Manual

The WORR map system combines server-side configuration with player-facing tools so you can keep rotations fresh without constant babysitting. This guide splits responsibilities for server hosts and players while highlighting where their workflows intersect.

## For Server Hosts

### Required files and cvars
- **Map pool JSON** – Set `g_maps_pool_file` to the JSON that enumerates every BSP the server will accept. The file must live under `baseq2/` and each entry needs at least a `bsp` name and `dm` flag, while optional metadata (player counts, gametype hints, score/time overrides, popularity, custom flag) refine eligibility and presentation.【F:docs/mapsystem.html†L22-L47】
- **Map cycle list** – Point `g_maps_cycle_file` at a plain-text list of BSP names (whitespace- or comma-separated). Line and block comments are ignored so you can annotate the list. Only maps in the active pool with `dm: true` will make it into rotation, preventing typos from breaking the loop.【F:docs/mapsystem.html†L49-L56】
- **Legacy fallback** – If the pool cannot be parsed, the server quietly falls back to `g_map_list`, so keep it populated as an emergency net for public servers.【F:docs/mapsystem.html†L99-L101】

### Runtime management commands
- `mappool` and `mapcycle` dump the currently eligible pool and rotation, making it easy to validate new files after an upload.【F:docs/mapsystem.html†L86-L95】
- `loadmappool` / `loadmapcycle` hot-reload their respective files without kicking players, perfect for mid-evening tweaks.【F:docs/mapsystem.html†L86-L95】
- `callvote map <map> [+flag]` and `callvote nextmap` remain available even when you curate the cycle, letting players react to imbalanced lobbies while staying inside your guardrails.【F:docs/mapsystem.html†L86-L96】

### Optional systems to enable
- **MyMap queue (`g_maps_mymap`)** – Allow each connected human to queue one preferred map (up to eight queued). The queue rejects maps played in the last ~30 minutes and respects per-match modifiers supplied with `+`/`-` flags (powerups, armor, ammo, health, BFG, fall damage, self-damage).【F:docs/mapsystem.html†L58-L75】
- **Post-match selector (`g_maps_selector`)** – Present up to three filtered options after each match. Filters account for cycle eligibility, min/max player counts, recent history, and whether console clients are connected (to avoid custom content). On ties or blank ballots, the selector reverts to the rotation or a random pick, so the server never stalls.【F:docs/mapsystem.html†L75-L84】

### Host best practices
- Pair the map pool metadata (min/max, popularity, custom) with your automated voting tools so high-traffic hours surface big maps while off-hours default to duels.【F:docs/mapsystem.html†L22-L47】【F:docs/mapsystem.html†L75-L84】
- Encourage responsible use of override flags by documenting which combinations your community approves, since the queue honors toggles for powerups, armor, ammo, health, BFG, fall damage, and self-damage.【F:docs/mapsystem.html†L58-L75】
- When testing new BSPs, temporarily tag them with `custom: true` so the selector avoids exposing them to console-only clients until you are ready for a full release.【F:docs/mapsystem.html†L22-L47】【F:docs/mapsystem.html†L75-L84】

## For Players

### Reading the rotation
- Use `mappool` and `mapcycle` in the console to preview what the server might load next. Pool entries show what is theoretically allowed; the cycle is what the server will actually choose from after each match.【F:docs/mapsystem.html†L86-L95】

### Claiming a MyMap slot
1. Make sure the server host has enabled the feature (`g_maps_mymap`). If disabled, the command politely fails.【F:docs/mapsystem.html†L58-L72】
2. Enter `mymap <bsp> [+flag] [-flag]` in the console. You can add toggles such as `+pu` for powerups or `-sd` to disable self-damage; only one map per player is allowed and the server caps the queue at eight entries.【F:docs/mapsystem.html†L58-L72】
3. If your choice was played within the last 30 minutes, pick a different map or wait until it leaves the cooldown window.【F:docs/mapsystem.html†L58-L72】

### Voting at match end
- When the post-match selector appears, pick the option that fits the current lobby size. The selector respects player counts and recent history, so if you want to swing the vote toward a large objective map, do it when the lobby is full.【F:docs/mapsystem.html†L75-L84】
- In the event of a tie or unanimous skip, the server will fall back to the scheduled cycle or a random eligible map—no need to worry about getting stuck in limbo.【F:docs/mapsystem.html†L75-L84】

### On-demand map changes
- `callvote map <map>` lets the lobby pivot instantly, but remember that your vote still has to pass whatever restrictions the host configured. Respect the community’s expectations when using override flags like `+bfg` or `-ht`.【F:docs/mapsystem.html†L58-L75】【F:docs/mapsystem.html†L86-L96】
- `callvote nextmap` is a lower-friction option that simply advances to the next cycle entry—ideal when the current map is winding down but no consensus exists on the replacement.【F:docs/mapsystem.html†L86-L96】

With these tools, hosts can curate a healthy rotation and players can influence the flow without derailing the experience. Coordinate across both perspectives to keep WORR sessions lively.
