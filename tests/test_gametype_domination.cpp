#include <cmath>
#include <array>
#include <memory>
#include <random>
#include <cstring>
#include <type_traits>
#include <string>
#include <vector>

namespace std {
	using ::sinf;

	template<typename E>
	constexpr auto to_underlying(E e) noexcept {
	return static_cast<std::underlying_type_t<E>>(e);
}
}

#include "server/g_local.hpp"

#include <cassert>

static std::vector<std::string> g_comPrintMessages{};

/*
=============
TestComPrint

Captures server print output for assertions.
=============
*/
static void TestComPrint(const char* message) {
	g_comPrintMessages.emplace_back(message ? message : "");
}

/*
=============
TestBroadcast
=============
*/
static void TestBroadcast(print_type_t, const char*) {}

/*
=============
TestClientPrint
=============
*/
static void TestClientPrint(gentity_t*, print_type_t, const char*) {}

/*
=============
TestCenterPrint
=============
*/
static void TestCenterPrint(gentity_t*, const char*) {}

/*
=============
TestSound
=============
*/
static void TestSound(gentity_t*, soundchan_t, int, float, float, float) {}

/*
=============
TestPositionedSound
=============
*/
static void TestPositionedSound(const Vector3&, gentity_t*, soundchan_t, int, float, float, float) {}

/*
=============
TestSoundIndex
=============
*/
static int TestSoundIndex(const char*) { return 0; }

/*
=============
TestLink
=============
*/
static void TestLink(gentity_t*) {}

/*
=============
TestUnlink
=============
*/
static void TestUnlink(gentity_t*) {}

/*
=============
TestBotUnregister
=============
*/
static void TestBotUnregister(const gentity_t*) {}

/*
=============
TestComError
=============
*/
static void TestComError(const char*) { assert(false && "Com_Error called"); }

/*
=============
TestLocPrint
=============
*/
static void TestLocPrint(gentity_t*, print_type_t, const char*, const char**, size_t) {}

/*
=============
TestTrace
=============
*/
static trace_t TestTrace(const Vector3&, const Vector3&, const Vector3&, const Vector3&, const gentity_t*, contents_t) {
	trace_t tr{};
	return tr;
}

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

static cvar_t g_domination_tick_interval_storage{};
cvar_t* g_domination_tick_interval = &g_domination_tick_interval_storage;

static cvar_t g_domination_points_per_tick_storage{};
cvar_t* g_domination_points_per_tick = &g_domination_points_per_tick_storage;

static cvar_t g_domination_capture_time_storage{};
cvar_t* g_domination_capture_time = &g_domination_capture_time_storage;

/*
=============
ScoringIsDisabled
=============
*/
bool ScoringIsDisabled() {
	return level.matchState != MatchState::In_Progress;
}

/*
=============
G_AdjustTeamScore
=============
*/
void G_AdjustTeamScore(Team team, int32_t offset) {
	if (team == Team::Red || team == Team::Blue)
		level.teamScores[static_cast<int>(team)] += offset;
}

/*
=============
ClientIsPlaying
=============
*/
bool ClientIsPlaying(gclient_t*) {
	return true;
}

/*
=============
Spawn
=============
*/
gentity_t* Spawn() {
	static std::aligned_storage_t<sizeof(gentity_t), alignof(gentity_t)> storage;
	auto* ent = reinterpret_cast<gentity_t*>(&storage);
	std::memset(ent, 0, sizeof(gentity_t));
	return ent;
}

/*
=============
FreeEntity
=============
*/
void FreeEntity(gentity_t*) {}

/*
=============
Teams
=============
*/
bool Teams() {
	return Game::Has(GameFlags::Teams);
}

/*
=============
Teams_TeamName
=============
*/
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

/*
=============
ED_GetSpawnTemp
=============
*/
const spawn_temp_t& ED_GetSpawnTemp() {
	static spawn_temp_t temp{};
	return temp;
}

#include "server/gameplay/g_domination.cpp"

/*
=============
main
=============
*/
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

	gi.Com_Print = TestComPrint;
	gi.Broadcast_Print = TestBroadcast;
	gi.Client_Print = TestClientPrint;
	gi.Center_Print = TestCenterPrint;
	gi.sound = TestSound;
	gi.positionedSound = TestPositionedSound;
	gi.soundIndex = TestSoundIndex;
	gi.linkEntity = TestLink;
	gi.unlinkEntity = TestUnlink;
	gi.Bot_UnRegisterEntity = TestBotUnregister;
	gi.Com_Error = TestComError;
	gi.Loc_Print = TestLocPrint;
	gi.trace = TestTrace;
	gi.frameTimeMs = 100;

	g_comPrintMessages.clear();
	g_domination_tick_interval_storage.value = -1.0f;
	g_domination_tick_interval_storage.integer = -1;
	const GameTime minimumInterval = DominationTickInterval();
	assert(minimumInterval == kDominationMinScoreInterval);
	assert(!g_comPrintMessages.empty());
	assert(g_comPrintMessages.back().find("minimum") != std::string::npos);

	g_comPrintMessages.clear();
	g_domination_tick_interval_storage.value = 50.0f;
	g_domination_tick_interval_storage.integer = 50;
	const GameTime maximumInterval = DominationTickInterval();
	assert(maximumInterval == kDominationMaxScoreInterval);
	assert(!g_comPrintMessages.empty());
	assert(g_comPrintMessages.back().find("maximum") != std::string::npos);

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

	g_domination_tick_interval_storage.value = 2.0f;
	g_domination_tick_interval_storage.integer = 2;
	g_domination_points_per_tick_storage.value = 3.0f;
	g_domination_points_per_tick_storage.integer = 3;
	g_domination_capture_time_storage.value = 1.0f;
	g_domination_capture_time_storage.integer = 1;

	level.matchState = MatchState::In_Progress;
	level.time = 0_ms;
	level.teamScores.fill(0);
	level.domination = {};

	static std::array<gclient_t, 2> clientStorage{};
	game.maxClients = clientStorage.size();
	game.clients = clientStorage.data();
	for (auto& client : clientStorage)
		client = {};

	static std::array<std::aligned_storage_t<sizeof(gentity_t), alignof(gentity_t)>, 4> entityStorage{};
	g_entities = reinterpret_cast<gentity_t*>(entityStorage.data());
	std::memset(g_entities, 0, sizeof(gentity_t) * entityStorage.size());

	g_entities[1].inUse = true;
	g_entities[1].client = &game.clients[0];
	g_entities[1].client->sess.team = Team::Red;
	g_entities[1].client->eliminated = false;

	g_entities[2].inUse = true;
	g_entities[2].client = &game.clients[1];
	g_entities[2].client->sess.team = Team::Blue;
	g_entities[2].client->eliminated = false;

	LevelLocals::DominationState& dom = level.domination;
	dom.count = 2;

	std::array<std::aligned_storage_t<sizeof(gentity_t), alignof(gentity_t)>, 2> pointStorage{};
	for (size_t i = 0; i < dom.count; ++i) {
		auto* ent = reinterpret_cast<gentity_t*>(&pointStorage[i]);
		std::memset(ent, 0, sizeof(gentity_t));
		ent->inUse = true;
		ent->spawn_count = 1;
		dom.points[i].ent = ent;
		dom.points[i].spawnCount = ent->spawn_count;
	}

	dom.points[0].owner = Team::Red;
	dom.points[1].owner = Team::Blue;
	dom.nextScoreTime = 0_ms;

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
	level.time = GameTime::from_sec(5.0f);
	Domination_RunFrame();
	assert(level.teamScores[static_cast<int>(Team::Red)] == 7);
	assert(level.teamScores[static_cast<int>(Team::Blue)] == 5);
	assert(dom.nextScoreTime == level.time + 100_ms);

	g_domination_tick_interval_storage.value = 1.0f;
	g_domination_tick_interval_storage.integer = 1;
	g_domination_points_per_tick_storage.value = 1.0f;
	g_domination_points_per_tick_storage.integer = 1;
	level.teamScores.fill(0);

	const GameTime frameStep = GameTime::from_ms(gi.frameTimeMs);
	trace_t tr{};

	level.domination = {};
	LevelLocals::DominationState& captureDom = level.domination;
	captureDom.count = 1;
	auto* captureEnt = reinterpret_cast<gentity_t*>(&pointStorage[0]);
	std::memset(captureEnt, 0, sizeof(gentity_t));
	captureEnt->inUse = true;
	captureEnt->spawn_count = 1;
	auto& capturePoint = captureDom.points[0];
	capturePoint.ent = captureEnt;
	capturePoint.spawnCount = captureEnt->spawn_count;
	capturePoint.owner = Team::Red;
	captureDom.nextScoreTime = 0_ms;
	level.time = 0_ms;

	Domination_InitLevel();

	for (int i = 0; i < 9; ++i) {
		level.time += frameStep;
		Domination_PointTouch(capturePoint.ent, &g_entities[2], tr, false);
		Domination_RunFrame();
		assert(capturePoint.owner == Team::Red);
	}

	level.time += frameStep;
	Domination_PointTouch(capturePoint.ent, &g_entities[2], tr, false);
	Domination_RunFrame();
	assert(capturePoint.owner == Team::Blue);
	assert(capturePoint.capturingTeam == Team::None);
	assert(capturePoint.captureProgress == 0.0f);

	GameTime scoreTime = captureDom.nextScoreTime;
	level.time = scoreTime;
	Domination_RunFrame();
	assert(level.teamScores[static_cast<int>(Team::Blue)] == 1);

	level.teamScores.fill(0);
	level.domination = {};
	LevelLocals::DominationState& contestDom = level.domination;
	contestDom.count = 1;
	auto* contestEnt = reinterpret_cast<gentity_t*>(&pointStorage[0]);
	std::memset(contestEnt, 0, sizeof(gentity_t));
	contestEnt->inUse = true;
	contestEnt->spawn_count = 1;
	auto& contestPoint = contestDom.points[0];
	contestPoint.ent = contestEnt;
	contestPoint.spawnCount = contestEnt->spawn_count;
	contestPoint.owner = Team::Red;
	contestDom.nextScoreTime = 0_ms;
	level.time = 0_ms;

	Domination_InitLevel();

	for (int i = 0; i < 5; ++i) {
		level.time += frameStep;
		Domination_PointTouch(contestPoint.ent, &g_entities[2], tr, false);
		Domination_RunFrame();
	}

	assert(contestPoint.capturingTeam == Team::Blue);
	assert(contestPoint.captureProgress > 0.0f);

	for (int i = 0; i < 5; ++i) {
		level.time += frameStep;
		Domination_PointTouch(contestPoint.ent, &g_entities[2], tr, false);
		Domination_PointTouch(contestPoint.ent, &g_entities[1], tr, false);
		Domination_RunFrame();
	}

	assert(contestPoint.captureProgress == 0.0f);
	assert(contestPoint.capturingTeam == Team::None);
	assert(contestPoint.owner == Team::Red);

	return 0;
}
