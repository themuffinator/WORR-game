#include <cmath>
#include <memory>

namespace std {
        using ::sinf;
}

#include "server/g_local.hpp"

#include <cassert>

// Provide the import table required by g_local.hpp consumers.
local_game_import_t gi{};

int main() {
        const auto &info = GAME_MODES[static_cast<size_t>(GameType::LastTeamStanding)];
        assert(info.type == GameType::LastTeamStanding);
        assert(info.short_name == "lts");
        assert(info.short_name_upper == "LTS");
        assert(info.long_name == "Last Team Standing");
        assert(info.spawn_name == "lts");
        assert(HasFlag(info.flags, GameFlags::Teams));
        assert(HasFlag(info.flags, GameFlags::Elimination));

        const auto shortLookup = Game::FromString("lts");
        assert(shortLookup.has_value() && *shortLookup == GameType::LastTeamStanding);

        const auto longLookup = Game::FromString("Last Team Standing");
        assert(longLookup.has_value() && *longLookup == GameType::LastTeamStanding);

        return 0;
}
