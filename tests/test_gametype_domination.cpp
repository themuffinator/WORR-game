#include <cmath>
#include <memory>
#include <random>
#include <type_traits>

namespace std {
	using ::sinf;

	template<typename E>
	constexpr auto to_underlying(E e) noexcept {
	return static_cast<std::underlying_type_t<E>>(e);
}
}

#include "server/g_local.hpp"

#include <cassert>

GameLocals game{};
LevelLocals level{};
game_export_t globals{};
local_game_import_t gi{};
std::mt19937 mt_rand{};
gentity_t* g_entities = nullptr;

static cvar_t g_gametype_storage{};
cvar_t* g_gametype = &g_gametype_storage;

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

	return 0;
}

