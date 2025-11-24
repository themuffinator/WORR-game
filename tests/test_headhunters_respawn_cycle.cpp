#include <array>
#include <cassert>
#include <cstring>
#include <random>

namespace std {
	using ::sinf;
}

#include "server/g_local.hpp"

static int freedEntities = 0;

static cvar_t g_gametype_storage{};

std::mt19937 mt_rand{};
game_export_t globals{};
gentity_t* g_entities = nullptr;
GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
cvar_t* g_gametype = &g_gametype_storage;

/*
=============
TestModelIndex

Returns a dummy model index for tests.
=============
*/
static int TestModelIndex(const char*)
{
	return 1;
}

/*
=============
TestSetModel

No-op model setter for tests.
=============
*/
static void TestSetModel(gentity_t* ent, const char*)
{
	if (!ent)
		return;
	++ent->s.modelIndex;
}

/*
=============
TestLinkEntity

Marks an entity as linked for tests.
=============
*/
static void TestLinkEntity(gentity_t* ent)
{
	if (!ent)
		return;
	ent->linked = true;
}

/*
=============
TestComPrintFmt

No-op print stub for tests.
=============
*/
static void TestComPrintFmt(const char*, ...)
{
}

/*
=============
TestLocBroadcastPrint

No-op localized broadcast stub for tests.
=============
*/
static void TestLocBroadcastPrint(print_type_t, const char*, ...)
{
}

/*
=============
TestSound

No-op sound stub for tests.
=============
*/
static void TestSound(gentity_t*, soundchan_t, int, float, float, float)
{
}

/*
=============
TestSoundIndex

Provides a dummy sound index for tests.
=============
*/
static int TestSoundIndex(const char*)
{
	return 1;
}

/*
=============
Spawn

Provides a cleared entity for tests.
=============
*/
gentity_t* Spawn()
{
	static std::aligned_storage_t<sizeof(gentity_t), alignof(gentity_t)> storage;
	auto* ent = reinterpret_cast<gentity_t*>(&storage);
	std::memset(ent, 0, sizeof(gentity_t));
	ent->inUse = true;
	return ent;
}

/*
=============
FreeEntity

Tracks freed entities for tests.
=============
*/
void FreeEntity(gentity_t* ent)
{
	++freedEntities;
	if (ent)
		ent->inUse = false;
}

/*
=============
crandom

Returns a deterministic random value for tests.
=============
*/
float crandom()
{
	return 0.25f;
}

/*
=============
frandom

Returns a deterministic random value for tests.
=============
*/
float frandom()
{
	return 0.5f;
}

/*
=============
ScoringIsDisabled

Indicates that scoring is enabled in tests.
=============
*/
bool ScoringIsDisabled()
{
	return false;
}

/*
=============
G_AdjustPlayerScore

No-op score adjustment for tests.
=============
*/
void G_AdjustPlayerScore(gclient_t*, int, bool, int)
{
}

/*
=============
ClientIsPlaying

Treats clients with an active session as playing.
=============
*/
bool ClientIsPlaying(gclient_t* cl)
{
	return cl && cl->pers.connected && cl->pers.spawned;
}

/*
=============
ED_GetSpawnTemp

Returns an empty spawn temp for tests.
=============
*/
const spawn_temp_t& ED_GetSpawnTemp()
{
	static spawn_temp_t temp{};
	return temp;
}

#include "server/gameplay/g_headhunters.cpp"

/*
=============
main

Verifies head attachments reset and stay cleared across elimination and respawn.
=============
*/
int main()
{
	g_gametype_storage.integer = static_cast<int>(GameType::HeadHunters);
	globals.numEntities = 4;
	std::array<gentity_t, 4> entityStorage{};
	g_entities = entityStorage.data();
	std::memset(g_entities, 0, sizeof(gentity_t) * entityStorage.size());

	std::array<gclient_t, 1> clients{};
	game.clients = clients.data();
	game.maxClients = clients.size();
	clients[0].pers.connected = true;
	clients[0].pers.spawned = true;
	g_entities[1].inUse = true;
	g_entities[1].client = &clients[0];
	g_entities[1].viewHeight = 12.0f;

	gi.modelIndex = TestModelIndex;
	gi.setModel = TestSetModel;
	gi.linkEntity = TestLinkEntity;
	gi.Com_PrintFmt = TestComPrintFmt;
	gi.LocBroadcast_Print = TestLocBroadcastPrint;
	gi.sound = TestSound;
	gi.soundIndex = TestSoundIndex;

	HeadHunters::InitLevel();

	gentity_t attachmentA{};
	gentity_t attachmentB{};
	attachmentA.inUse = true;
	attachmentB.inUse = true;
	clients[0].headhunter.carried = 2;
	clients[0].headhunter.attachments[0] = &attachmentA;
	clients[0].headhunter.attachments[1] = &attachmentB;
	clients[0].ps.stats[STAT_GAMEPLAY_CARRIED] = clients[0].headhunter.carried;
	clients[0].eliminated = true;

	HeadHunters::RunFrame();

	assert(clients[0].headhunter.carried == 0);
	assert(clients[0].ps.stats[STAT_GAMEPLAY_CARRIED] == 0);
	for (auto* attachment : clients[0].headhunter.attachments)
		assert(attachment == nullptr || !attachment->inUse);
	assert(freedEntities >= 2);

	clients[0].eliminated = false;

	HeadHunters::RunFrame();

	for (auto* attachment : clients[0].headhunter.attachments)
		assert(attachment == nullptr);
	assert(clients[0].headhunter.carried == 0);
	assert(clients[0].ps.stats[STAT_GAMEPLAY_CARRIED] == 0);

	return 0;
}
