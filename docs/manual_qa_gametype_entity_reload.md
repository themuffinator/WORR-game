# Manual QA: Gametype transitions reuse cached entity string

## Goal
Verify that changing `g_gametype` mid-map reloads the world from the cached entity string via `Match_Reset` instead of forcing a full `gamemap` restart.

## Steps
1. Start a local server on any deathmatch map (e.g., `map q2dm1`) and wait for the map to finish loading.
2. Note the current gametype and a few world features that would reset on a full map reload (e.g., watch item or monster positions).
3. From the console, issue `set g_gametype 4` (or another valid value) to trigger a gametype change while staying on the same map.
4. Observe the server console:
   - It should print the updated gametype banner.
   - No `gamemap <map>` command should fire unless the entity reload fails.
5. Confirm players remain connected and the level state resets in place:
   - Items and spawns match a fresh load for the new gametype.
   - The reset occurs immediately without the usual loading pause from a `gamemap` restart.
6. If the console reports a fallback to `gamemap`, verify the map still reloads cleanly; report the missing cached entity string as a separate issue.

## Expected Result
Gametype transitions reuse the cached entity string through `Match_Reset`, avoiding full map restarts while still refreshing map entities for the new ruleset.
