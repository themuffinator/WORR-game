/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_gametype_validation.cpp implementation.*/

#include <cmath>

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

int main() {
	// Valid values within [GT_FIRST, GT_LAST] should be accepted untouched.
	g_gametype_storage.integer = static_cast<int>(GameType::FreeForAll);
	assert(Game::IsCurrentTypeValid());
	assert(Game::NormalizeTypeValue(g_gametype_storage.integer) == GameType::FreeForAll);

	g_gametype_storage.integer = static_cast<int>(GT_LAST);
	assert(Game::IsCurrentTypeValid());
	assert(Game::NormalizeTypeValue(g_gametype_storage.integer) == GT_LAST);

	// Sentinel or engine-provided defaults (e.g., 0) must fall back to FreeForAll.
	g_gametype_storage.integer = static_cast<int>(GameType::None);
	assert(!Game::IsCurrentTypeValid());
	assert(Game::NormalizeTypeValue(g_gametype_storage.integer) == GameType::FreeForAll);

	// Out-of-range upper bounds should also snap back to FreeForAll.
	g_gametype_storage.integer = static_cast<int>(GameType::Total);
	assert(!Game::IsCurrentTypeValid());
	assert(Game::NormalizeTypeValue(g_gametype_storage.integer) == GameType::FreeForAll);

	g_gametype_storage.integer = 256;
	assert(!Game::IsCurrentTypeValid());
	assert(Game::NormalizeTypeValue(g_gametype_storage.integer) == GameType::FreeForAll);
	
	// Mid-match assignments should coerce back to FreeForAll before additional logic runs.
	g_gametype_storage.integer = static_cast<int>(GameType::TeamDeathmatch);
	assert(Game::NormalizeTypeValue(g_gametype_storage.integer) == GameType::TeamDeathmatch);
	
	g_gametype_storage.integer = 4096;
	const GameType mid_match = Game::NormalizeTypeValue(g_gametype_storage.integer);
	assert(mid_match == GameType::FreeForAll);
	g_gametype_storage.integer = static_cast<int>(mid_match);
	assert(g_gametype_storage.integer == static_cast<int>(GameType::FreeForAll));

	// Negative values should also fall back to FreeForAll and not crash.
	g_gametype_storage.integer = -5;
	assert(!Game::IsCurrentTypeValid());
	assert(Game::GetCurrentType() == GameType::FreeForAll);
	assert(Game::GetCurrentInfo().type == GameType::FreeForAll);

	// Oversized integers must resolve to the fallback gametype info.
	g_gametype_storage.integer = 1'000'000;
	assert(!Game::IsCurrentTypeValid());
	assert(Game::GetCurrentType() == GameType::FreeForAll);
	assert(Game::GetCurrentInfo().type == GameType::FreeForAll);

	// A null gametype pointer should behave identically to the fallback case.
	g_gametype = nullptr;
	assert(Game::GetCurrentType() == GameType::FreeForAll);
	assert(Game::GetCurrentInfo().type == GameType::FreeForAll);
	g_gametype = &g_gametype_storage;

	return 0;
}
