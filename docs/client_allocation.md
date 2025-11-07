# Client allocation helpers

The game module allocates `gclient_t` arrays with `TagMalloc` so it cannot rely
on C++'s array new to run constructors and destructors automatically.  The
helpers declared in [`src/server/gameplay/g_clients.hpp`](../src/server/gameplay/g_clients.hpp)
provide a single place to construct and destroy those objects correctly.

## Runtime helpers

The following entry points wrap the allocation logic used by the main game
loop:

| Function | Purpose |
| --- | --- |
| `AllocateClientArray(int maxClients)` | Allocates `game.clients`, runs the constructors, and updates `game.maxClients`, `globals.numEntities`, and the lag-compensation buffer (`game.lagOrigins`). |
| `FreeClientArray()` | Tears down the client and lag arrays, running destructors before returning memory to the engine. |
| `ReplaceClientArray(int maxClients)` | Convenience helper used by save/load paths to reinitialize the arrays in one call. |

The server currently allocates or replaces the client array in two situations:

1. **Initial startup** – `InitGame` allocates `game.clients` after reading the
   `maxclients` cvar via `AllocateClientArray`.
2. **Savegame loading** – the save system uses `ReplaceClientArray` when
   restoring a save file.

Both call sites include the helper header to ensure they stay in sync. Any
future code that resizes or swaps the client array should include the header and
use the new helpers (or `ConstructClients`/`DestroyClients`/`ClientArrayLifetime`
for custom storage) so that the client constructors run exactly once per
allocation cycle while the related globals stay consistent.
