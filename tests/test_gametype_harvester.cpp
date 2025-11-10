#include <cassert>
#include <functional>
#include <cmath>
#include <memory>

namespace std { using ::sinf; }
#include "server/g_local.hpp"

std::mt19937 mt_rand{};
GameLocals game{};
LevelLocals level{};
game_export_t globals{};
local_game_import_t gi{};
cvar_t* g_gametype = nullptr;


int main() {
	cvar_t gametype{};
	g_gametype = &gametype;

	const auto& info = Game::GetInfo(GameType::Harvester);
	assert(info.type == GameType::Harvester);
	assert(HasFlag(info.flags, GameFlags::Teams));
	assert(HasFlag(info.flags, GameFlags::CTF));

	g_gametype->integer = static_cast<int>(GameType::Harvester);
	assert(Game::Is(GameType::Harvester));
	assert(Game::Has(GameFlags::CTF));

	g_gametype->integer = static_cast<int>(GameType::TeamDeathmatch);
	assert(Game::IsNot(GameType::Harvester));
	assert(Game::DoesNotHave(GameFlags::CTF));

	return 0;
}
