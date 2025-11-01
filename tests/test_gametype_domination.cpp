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

int main() {
        const auto& info = GAME_MODES[static_cast<size_t>(GameType::Domination)];
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

        return 0;
}
