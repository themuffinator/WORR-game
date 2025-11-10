#include "server/g_local.hpp"

#include <cassert>

namespace std {
	using ::sinf;
}

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
