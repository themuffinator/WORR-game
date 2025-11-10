#include <cmath>
#include <memory>
#include <random>

namespace std {
		using ::sinf;
}

#include "server/g_local.hpp"

#include <cassert>

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
std::mt19937 mt_rand{};
static cvar_t g_gametype_storage{};
cvar_t* g_gametype = &g_gametype_storage;
GameTime FRAME_TIME_S{};
GameTime FRAME_TIME_MS{};
static cvar_t deathmatch_storage{};
cvar_t* deathmatch = &deathmatch_storage;
static cvar_t g_domination_capture_time_storage{};
cvar_t* g_domination_capture_time = &g_domination_capture_time_storage;
static cvar_t g_domination_neutralize_time_storage{};
cvar_t* g_domination_neutralize_time = &g_domination_neutralize_time_storage;
static cvar_t g_domination_capture_bonus_storage{};
cvar_t* g_domination_capture_bonus = &g_domination_capture_bonus_storage;

static void StubLocPrint(gentity_t*, print_type_t, const char*, const char**, size_t) {}

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

		gi.Loc_Print = StubLocPrint;

		FRAME_TIME_MS = FRAME_TIME_S = GameTime::from_ms(100);
		deathmatch_storage.integer = 1;
		g_domination_capture_time_storage.value = 1.0f;
		g_domination_capture_time_storage.integer = 1;
		g_domination_neutralize_time_storage.value = 0.5f;
		g_domination_neutralize_time_storage.integer = 0;
		g_domination_capture_bonus_storage.value = 7.0f;
		g_domination_capture_bonus_storage.integer = 7;

		level.matchState = MatchState::In_Progress;
		level.roundState = RoundState::In_Progress;
		level.restarted = true;

		gentity_t pointEntity{};
		pointEntity.inUse = false;
		gentity_t player{};
		gclient_t client{};
		player.client = &client;
		client.pers.connected = true;
		client.sess.team = Team::Red;

		level.domination = {};
		level.domination.count = 1;
		auto& point = level.domination.points[0];
		point.ent = &pointEntity;
		point.index = 0;
		point.owner = Team::None;

		trace_t trace{};

		level.time = 0_ms;
		client.resp.score = 0;

		for (int i = 0; i < 9; ++i) {
				Domination_PointTouch(&pointEntity, &player, trace, false);
				assert(point.owner != Team::Red);
				level.time += FRAME_TIME_MS;
		}

		Domination_PointTouch(&pointEntity, &player, trace, false);
		assert(point.owner == Team::Red);
		assert(client.resp.score == g_domination_capture_bonus_storage.integer);

		client.resp.score = 0;
		point.owner = Team::Blue;
		point.captureProgress = {};
		point.neutralizeProgress = {};
		point.lastProgressUpdate = {};
		level.time = 0_ms;

		for (int i = 0; i < 4; ++i) {
				Domination_PointTouch(&pointEntity, &player, trace, false);
				assert(point.owner == Team::Blue);
				level.time += FRAME_TIME_MS;
		}

		Domination_PointTouch(&pointEntity, &player, trace, false);
		assert(point.owner == Team::None);
		level.time += FRAME_TIME_MS;

		for (int i = 0; i < 9; ++i) {
				Domination_PointTouch(&pointEntity, &player, trace, false);
				assert(point.owner != Team::Red);
				level.time += FRAME_TIME_MS;
		}

		Domination_PointTouch(&pointEntity, &player, trace, false);
		assert(point.owner == Team::Red);
		assert(client.resp.score == g_domination_capture_bonus_storage.integer);

		return 0;
}
