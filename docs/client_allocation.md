# Client allocation helpers

The game module allocates `gclient_t` arrays with `TagMalloc` so it cannot rely
on C++'s array new to run constructors and destructors automatically.  The
helpers declared in [`src/server/gameplay/g_clients.hpp`](../src/server/gameplay/g_clients.hpp)
provide a single place to construct and destroy those objects correctly.

The server currently allocates or replaces the client array in two situations:

1. **Initial startup** – `InitGame` allocates `game.clients` after reading the
   `maxclients` cvar.
2. **Savegame loading** – the save system allocates a replacement array when
   restoring a save file.

Both call sites now include the helper header to ensure they stay in sync.  Any
future code that resizes or swaps the client array should include the header and
use `ConstructClients`/`DestroyClients` (or `ClientArrayLifetime`) so that the
client constructors run exactly once per allocation cycle.
