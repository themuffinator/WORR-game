#include <cmath>
#include <memory>
#include <random>
#include <cstring>
#include <type_traits>
#include <array>
#include <string>

namespace std {
	using ::sinf;

	template<typename E>
	constexpr auto to_underlying(E e) noexcept {
	return static_cast<std::underlying_type_t<E>>(e);
}
}

#include "server/g_local.hpp"
#include "server/player/p_hud_domination.hpp"

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

static std::array<std::string, MAX_CONFIGSTRINGS> g_configStringStorage{};

static void TestConfigString(int num, const char* value) {
	if (num < 0 || num >= MAX_CONFIGSTRINGS)
		return;
	if (value)
		g_configStringStorage[static_cast<size_t>(num)] = value;
	else
		g_configStringStorage[static_cast<size_t>(num)].clear();
}

static const char* TestGetConfigString(int num) {
	if (num < 0 || num >= MAX_CONFIGSTRINGS)
		return "";
	return g_configStringStorage[static_cast<size_t>(num)].c_str();
}

game_export_t globals{};
gentity_t* g_entities = nullptr;
GameLocals game{};
LevelLocals level{};
game_export_t globals{};
local_game_import_t gi{};
std::mt19937 mt_rand{};
gentity_t* g_entities = nullptr;

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

static cvar_t g_domination_tick_interval_storage{};
cvar_t* g_domination_tick_interval = &g_domination_tick_interval_storage;

static cvar_t g_domination_points_per_tick_storage{};
cvar_t* g_domination_points_per_tick = &g_domination_points_per_tick_storage;

static cvar_t deathmatch_storage{};
cvar_t* deathmatch = &deathmatch_storage;

bool ScoringIsDisabled() {
	return level.matchState != MatchState::In_Progress;
}

void G_AdjustTeamScore(Team team, int32_t offset) {
	if (team == Team::Red || team == Team::Blue)
				level.teamScores[static_cast<int>(team)] += offset;
}

bool ClientIsPlaying(gclient_t*) {
	return true;
}

gentity_t* Spawn() {
	return nullptr;
}

void FreeEntity(gentity_t*) {}

bool Teams() {
	return Game::Has(GameFlags::Teams);
}

const char* Teams_TeamName(Team team) {
	switch (team) {
	case Team::Red:
		return "Red";
	case Team::Blue:
		return "Blue";
	default:
		return "None";
	}
}

const spawn_temp_t& ED_GetSpawnTemp() {
	static spawn_temp_t temp{};
	return temp;
}

#include "server/gameplay/g_domination.cpp"

int main() {
	const auto& info = Game::GetInfo(GameType::Domination);
	assert(info.type == GameType::Domination);
	assert(info.short_name == "dom");
	assert(info.short_name_upper == "DOM");
	assert(info.long_name == "Domination");
	assert(info.spawn_name == "domination");
	assert(HasFlag(info.flags, GameFlags::Teams));
	assert(HasFlag(info.flags, GameFlags::Frags));

	const auto invalidType = static_cast<GameType>(-1);
	const auto& fallback = Game::GetInfo(invalidType);
	assert(fallback.type == GameType::FreeForAll);

	g_gametype_storage.integer = static_cast<int>(GameType::Domination);
	assert(Game::Has(GameFlags::Teams));
	assert(Teams());

	deathmatch->integer = 0;
	game.maxClients = 0;
	globals.numEntities = 0;

	level.matchState = MatchState::In_Progress;
	level.time = 0_ms;
	level.teamScores.fill(0);
	level.domination = {};

	g_domination_tick_interval_storage.value = 2.0f;
	g_domination_tick_interval_storage.integer = 2;
	g_domination_points_per_tick_storage.value = 3.0f;
	g_domination_points_per_tick_storage.integer = 3;

	LevelLocals::DominationState& dom = level.domination;
	dom.count = 2;
	std::aligned_storage_t<sizeof(gentity_t), alignof(gentity_t)> pointStorage[2];
	dom.points[0].ent = reinterpret_cast<gentity_t*>(&pointStorage[0]);
	dom.points[1].ent = reinterpret_cast<gentity_t*>(&pointStorage[1]);
	dom.points[0].owner = Team::Red;
	dom.points[1].owner = Team::Blue;
	dom.nextScoreTime = 0_ms;
	dom.points[0].ent->inUse = true;
	dom.points[0].ent->spawn_count = 1;
	dom.points[0].spawnCount = dom.points[0].ent->spawn_count;
	dom.points[0].ent->message = const_cast<char*>("Alpha");
	dom.points[1].ent->inUse = true;
	dom.points[1].ent->spawn_count = 2;
	dom.points[1].spawnCount = dom.points[1].ent->spawn_count;
	dom.points[1].ent->targetName = const_cast<char*>("Bravo");

	g_configStringStorage.fill("");
	std::array<int16_t, MAX_STATS> domStats{};
	Domination_SetHudStats(domStats);
	const uint16_t packedOwners = static_cast<uint16_t>(domStats[STAT_DOMINATION_POINTS]);
	assert(DominationPointOwnerIndex(packedOwners, 0) == static_cast<uint16_t>(dom.points[0].owner));
	assert(DominationPointOwnerIndex(packedOwners, 1) == static_cast<uint16_t>(dom.points[1].owner));
	assert(g_configStringStorage[CONFIG_DOMINATION_POINT_LABEL_START + 0] == "Alpha");
	assert(g_configStringStorage[CONFIG_DOMINATION_POINT_LABEL_START + 1] == "Bravo");

	Domination_RunFrame();
	assert(dom.nextScoreTime == GameTime::from_sec(2.0f));
	assert(level.teamScores[static_cast<int>(Team::Red)] == 0);
	assert(level.teamScores[static_cast<int>(Team::Blue)] == 0);

	level.time = GameTime::from_sec(2.0f);
	Domination_RunFrame();
	assert(level.teamScores[static_cast<int>(Team::Red)] == 3);
	assert(level.teamScores[static_cast<int>(Team::Blue)] == 3);
	assert(dom.nextScoreTime == level.time + GameTime::from_sec(2.0f));

	g_domination_tick_interval_storage.value = 0.0f;
	g_domination_tick_interval_storage.integer = 0;
	g_domination_points_per_tick_storage.value = 0.0f;
	g_domination_points_per_tick_storage.integer = 0;
	dom.points[1].owner = Team::Red;
	Domination_SetHudStats(domStats);
	assert(DominationPointOwnerIndex(static_cast<uint16_t>(domStats[STAT_DOMINATION_POINTS]), 1) == static_cast<uint16_t>(Team::Red));
	level.time = GameTime::from_sec(4.0f);
	Domination_RunFrame();
	assert(level.teamScores[static_cast<int>(Team::Red)] == 5);
	assert(level.teamScores[static_cast<int>(Team::Blue)] == 3);
	assert(dom.nextScoreTime == level.time + 1_sec);

	g_domination_tick_interval_storage.value = 0.001f;
	g_domination_tick_interval_storage.integer = 0;
	g_domination_points_per_tick_storage.value = 2.0f;
	g_domination_points_per_tick_storage.integer = 2;
	dom.points[1].owner = Team::Blue;
	Domination_SetHudStats(domStats);
	assert(DominationPointOwnerIndex(static_cast<uint16_t>(domStats[STAT_DOMINATION_POINTS]), 1) == static_cast<uint16_t>(Team::Blue));
	level.time = GameTime::from_sec(5.0f);
	Domination_RunFrame();
	assert(level.teamScores[static_cast<int>(Team::Red)] == 7);
	assert(level.teamScores[static_cast<int>(Team::Blue)] == 5);
	assert(dom.nextScoreTime == level.time + 100_ms);
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
	gi.configString = TestConfigString;
	gi.get_configString = TestGetConfigString;

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

	Domination_SetHudStats(domStats);
	const uint16_t finalPacked = static_cast<uint16_t>(domStats[STAT_DOMINATION_POINTS]);
	assert(finalPacked == 0);
	assert(g_configStringStorage[CONFIG_DOMINATION_POINT_LABEL_START + 0].empty());

	return 0;
}

