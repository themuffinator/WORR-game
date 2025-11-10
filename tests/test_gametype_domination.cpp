#include <cmath>
#include <memory>
#include <random>
#include <cstring>
#include <type_traits>

namespace std {
	using ::sinf;
}

#include "server/g_local.hpp"

#include <cassert>

static void TestComPrint(const char*) {}
static void TestBroadcast(print_type_t, const char*) {}
static void TestClientPrint(gentity_t*, print_type_t, const char*) {}
static void TestCenterPrint(gentity_t*, const char*) {}
static void TestSound(gentity_t*, soundchan_t, int, float, float, float) {}
static void TestPositionedSound(const Vector3&, gentity_t*, soundchan_t, int, float, float, float) {}
static int TestSoundIndex(const char*) { return 0; }
static void TestUnlink(gentity_t*) {}
static void TestBotUnregister(const gentity_t*) {}
static void TestComError(const char*) { assert(false && "Com_Error called"); }

game_export_t globals{};
gentity_t* g_entities = nullptr;
GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
std::mt19937 mt_rand{};
static cvar_t g_gametype_storage{};
cvar_t* g_gametype = &g_gametype_storage;
static cvar_t deathmatchVar{};
cvar_t* deathmatch = &deathmatchVar;
static cvar_t fragLimitVar{};
cvar_t* fragLimit = &fragLimitVar;
static cvar_t captureLimitVar{};
cvar_t* captureLimit = &captureLimitVar;
static cvar_t roundLimitVar{};
cvar_t* roundLimit = &roundLimitVar;
static cvar_t noPlayersTimeVar{};
cvar_t* noPlayersTime = &noPlayersTimeVar;
static cvar_t timeLimitVar{};
cvar_t* timeLimit = &timeLimitVar;
static cvar_t mercyLimitVar{};
cvar_t* mercyLimit = &mercyLimitVar;
static cvar_t matchStartNoHumansVar{};
cvar_t* match_startNoHumans = &matchStartNoHumansVar;
static cvar_t matchDoOvertimeVar{};
cvar_t* match_doOvertime = &matchDoOvertimeVar;
static cvar_t minplayersVar{};
cvar_t* minplayers = &minplayersVar;
static cvar_t allowSpecVoteVar{};
cvar_t* g_allowSpecVote = &allowSpecVoteVar;
static cvar_t teamplayForceBalanceVar{};
cvar_t* g_teamplay_force_balance = &teamplayForceBalanceVar;
static cvar_t teamplayAutoBalanceVar{};
cvar_t* g_teamplay_auto_balance = &teamplayAutoBalanceVar;

int main() {
	const auto& info = Game::GetInfo(GameType::Domination);
	assert(info.type == GameType::Domination);
	assert(info.short_name == "dom");
	assert(info.short_name_upper == "DOM");
	assert(info.long_name == "Domination");
	assert(info.spawn_name == "domination");
	assert(HasFlag(info.flags, GameFlags::Teams));
	assert(HasFlag(info.flags, GameFlags::Frags));

	g_gametype_storage.integer = static_cast<int>(GameType::Domination);
	assert(Game::Has(GameFlags::Teams));
	assert(Teams());

	gi.Com_Print = TestComPrint;
	gi.Broadcast_Print = TestBroadcast;
	gi.Client_Print = TestClientPrint;
	gi.Center_Print = TestCenterPrint;
	gi.sound = TestSound;
	gi.positionedSound = TestPositionedSound;
	gi.soundIndex = TestSoundIndex;
	gi.unlinkEntity = TestUnlink;
	gi.Bot_UnRegisterEntity = TestBotUnregister;
	gi.Com_Error = TestComError;

	deathmatchVar.integer = 0;
	deathmatchVar.value = 0.0f;
	fragLimitVar.integer = 0;
	fragLimitVar.value = 0.0f;
	captureLimitVar.integer = 0;
	captureLimitVar.value = 0.0f;
	roundLimitVar.integer = 0;
	roundLimitVar.value = 0.0f;
	noPlayersTimeVar.integer = 0;
	noPlayersTimeVar.value = 0.0f;
	timeLimitVar.integer = 0;
	timeLimitVar.value = 0.0f;
	mercyLimitVar.integer = 0;
	mercyLimitVar.value = 0.0f;
	matchStartNoHumansVar.integer = 1;
	matchStartNoHumansVar.value = 1.0f;
	matchDoOvertimeVar.integer = 0;
	matchDoOvertimeVar.value = 0.0f;
	minplayersVar.integer = 0;
	minplayersVar.value = 0.0f;
	allowSpecVoteVar.integer = 0;
	allowSpecVoteVar.value = 0.0f;
	teamplayForceBalanceVar.integer = 0;
	teamplayForceBalanceVar.value = 0.0f;
	teamplayAutoBalanceVar.integer = 0;
	teamplayAutoBalanceVar.value = 0.0f;

	constexpr size_t kEntityCapacity = 4;
	using EntityStorage = std::aligned_storage_t<sizeof(gentity_t), alignof(gentity_t)>;
	static EntityStorage entityStorage[kEntityCapacity];
	g_entities = reinterpret_cast<gentity_t*>(entityStorage);
	std::memset(g_entities, 0, sizeof(gentity_t) * kEntityCapacity);
	for (size_t i = 0; i < kEntityCapacity; ++i) {
	        g_entities[i].s.number = static_cast<int>(i);
	        g_entities[i].spawn_count = 0;
	}

	using ClientStorage = std::aligned_storage_t<sizeof(gclient_t), alignof(gclient_t)>;
	static ClientStorage clientStorage[1];
	game.clients = reinterpret_cast<gclient_t*>(clientStorage);
	std::memset(game.clients, 0, sizeof(gclient_t) * 1);
	game.maxClients = 0;
	game.maxEntities = kEntityCapacity;

	globals.numEntities = static_cast<uint32_t>(game.maxClients + 1);
	globals.gentities = g_entities;

	level = {};
	level.matchState = MatchState::In_Progress;
	level.roundState = RoundState::In_Progress;
	level.levelStartTime = 0_ms;
	level.time = 10_sec;
	level.teamScores.fill(0);
	level.teamOldScores.fill(0);
	level.domination = {};
	level.domination.count = 1;
	level.domination.nextScoreTime = level.time;

	auto& point = level.domination.points[0];
	gentity_t* pointEnt = Spawn();
	assert(pointEnt);
	pointEnt->className = "domination_point";
	point.ent = pointEnt;
	point.owner = Team::Red;
	point.index = 0;
	point.spawnCount = pointEnt->spawn_count;

	const int initialRedScore = level.teamScores[static_cast<int>(Team::Red)];
	const int initialBlueScore = level.teamScores[static_cast<int>(Team::Blue)];

	Domination_RunFrame();

	const int firstRedScore = level.teamScores[static_cast<int>(Team::Red)];
	assert(firstRedScore == initialRedScore + 1);
	assert(level.teamScores[static_cast<int>(Team::Blue)] == initialBlueScore);

	const GameTime nextTick = level.domination.nextScoreTime;

	FreeEntity(pointEnt);
	level.time = nextTick;

	Domination_RunFrame();

	assert(level.teamScores[static_cast<int>(Team::Red)] == firstRedScore);
	assert(level.teamScores[static_cast<int>(Team::Blue)] == initialBlueScore);
	assert(level.domination.points[0].ent == nullptr);
	assert(level.domination.points[0].owner == Team::None);
	assert(level.domination.points[0].spawnCount == 0);

	return 0;
}
