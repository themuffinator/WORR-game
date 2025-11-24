#include <cassert>

#include "server/g_local.hpp"

local_game_import_t gi{};
game_export_t globals{};
gentity_t* g_entities = nullptr;
GameLocals game{};
LevelLocals level{};
std::mt19937 mt_rand{};
cvar_t g_gametype_storage{};
cvar_t* g_gametype = &g_gametype_storage;
cvar_t deathmatch_storage{};
cvar_t* deathmatch = &deathmatch_storage;

static gclient_t clients[2]{};
static gentity_t entities[4]{};

/*
=============
BlockedTrace

Returns a fully obstructed trace to force LocCanSee failures.
=============
*/
static trace_t BlockedTrace(const Vector3&, const Vector3&, const Vector3&, const Vector3&, const gentity_t*, contents_t) {
	trace_t tr{};

	tr.fraction = 0.0f;

	return tr;
}

/*
=============
TestSoundIndex
=============
*/
static int TestSoundIndex(const char*) {
	return 0;
}

/*
=============
main

Validates base defense awards when line of sight checks fail near the flag.
=============
*/
int main() {
	g_entities = entities;
	gi.trace = BlockedTrace;
	gi.soundIndex = TestSoundIndex;
	game.clients = clients;
	game.maxClients = 2;
	game.maxEntities = 4;
	globals.numEntities = 4;
	deathmatch_storage.integer = 0;
	deathmatch_storage.value = 0.0f;
	g_gametype_storage.integer = static_cast<int>(GameType::CaptureTheFlag);
	level.matchState = MatchState::In_Progress;
	level.roundState = RoundState::In_Progress;
	level.intermission = {};
	level.restarted = false;
	level.time = 0_ms;
	level.timeoutActive = false;
	level.teamScores.fill(0);
	level.sortedClients.fill(-1);

	entities[0].inUse = true;
	entities[0].s.number = 0;

	entities[3].inUse = true;
	entities[3].className = ITEM_CTF_FLAG_RED;
	entities[3].s.origin = {};
	entities[3].spawnFlags = {};
	entities[3].s.number = 3;

	gentity_t* attacker = &entities[1];
	gentity_t* target = &entities[2];

	attacker->inUse = true;
	attacker->client = &clients[0];
	attacker->s.number = 1;
	attacker->s.origin = { 128.0f, 0.0f, 0.0f };

	attacker->client->sess.team = Team::Red;
	attacker->client->pers.connected = true;
	attacker->client->resp.score = 0;
	attacker->client->sess.teamJoinTime = 0;

	target->inUse = true;
	target->client = &clients[1];
	target->s.number = 2;
	target->s.origin = { 0.0f, 128.0f, 0.0f };

	target->client->sess.team = Team::Blue;
	target->client->pers.connected = true;
	target->client->resp.score = 0;
	target->client->sess.teamJoinTime = 0;

	CTF_ScoreBonuses(target, nullptr, attacker);
	assert(attacker->client->resp.score == CTF::FLAG_DEFENSE_BONUS);

	attacker->client->resp.score = 0;
	attacker->s.origin = { CTF::TARGET_PROTECT_RADIUS + 10.0f, 0.0f, 0.0f };
	target->s.origin = { 0.0f, -(CTF::TARGET_PROTECT_RADIUS + 10.0f), 0.0f };

	CTF_ScoreBonuses(target, nullptr, attacker);
	assert(attacker->client->resp.score == 0);

	return 0;
}
