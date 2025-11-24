#include <cassert>

#include "server/gameplay/g_harvester.hpp"
#include "server/g_local.hpp"

std::mt19937 mt_rand{};
GameLocals game{};
LevelLocals level{};
game_export_t globals{};
local_game_import_t gi{};
cvar_t* g_gametype = nullptr;
cvar_t* teamplay = nullptr;
cvar_t* deathmatch = nullptr;
cvar_t* ctf = nullptr;

static int g_spawned = 0;

/*
=============
FakeHarvesterSpawn

Counts simulated skull spawns for stress validation.
=============
*/
static gentity_t* FakeHarvesterSpawn(Team, const Vector3&, bool) {
	++g_spawned;
	return nullptr;
}

/*
=============
main

Stress test deferred skull spawning and clamping behavior.
=============
*/
int main() {
	cvar_t gametype{};
	g_gametype = &gametype;
	g_gametype->integer = static_cast<int>(GameType::Harvester);

	cvar_t teamplayCvar{};
	teamplay = &teamplayCvar;
	teamplay->integer = 1;

	cvar_t deathmatchCvar{};
	deathmatch = &deathmatchCvar;
	deathmatch->integer = 1;

	cvar_t ctfCvar{};
	ctf = &ctfCvar;
	ctf->integer = 1;

	level.time = 0_ms;
	Harvester_SetSpawnCallback(FakeHarvesterSpawn);

	const Vector3 origin{};
	const int total = 100;
	Harvester_DropSkulls(Team::Red, total, origin, false);

	const int maxDrop = Harvester_GetMaxSkullsPerDrop();
	assert(g_spawned == maxDrop);

	int iterations = 0;
	while (g_spawned < total && iterations < 32) {
		level.time += Harvester_GetDeferredDropInterval();
		Harvester_ProcessDeferredDrops();
		++iterations;
	}

	assert(g_spawned == total);
	return 0;
}
