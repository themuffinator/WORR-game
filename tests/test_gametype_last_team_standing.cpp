/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_gametype_last_team_standing.cpp implementation.*/

#include <cmath>
#include <memory>
#include <string>

namespace std {
        using ::sinf;
}

#include "server/g_local.hpp"

#include <cassert>

// Provide the import table required by g_local.hpp consumers.
local_game_import_t gi{};

int main() {
	const auto &info = Game::GetInfo(GameType::LastTeamStanding);
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

	// High-bit characters should not trigger UB in case-insensitive compares.
	std::string highBitName = "lt";
	highBitName.push_back(static_cast<char>(0xE1));
	assert(Game::AreStringsEqualIgnoreCase(highBitName, highBitName));
	const auto highBitLookup = Game::FromString(highBitName);
	assert(!highBitLookup.has_value());

        return 0;
}
