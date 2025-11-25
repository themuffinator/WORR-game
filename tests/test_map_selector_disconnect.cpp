/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_map_selector_disconnect.cpp implementation.*/

#include "server/g_local.hpp"

#include <cassert>
#include <random>

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
game_export_t globals{};
gentity_t* g_entities = nullptr;
std::mt19937 mt_rand{};

/*
=============
main

Simulates a player casting a map vote, disconnecting, and ensures stale
votes don't produce a majority tally.
=============
*/
int main() {
	auto& ms = level.mapSelector;

	ms.votes.fill(-1);
	ms.voteCounts.fill(0);

	ms.votes[0] = 0;
	ms.voteCounts[0] = 1;

	MapSelector_ClearVote(level, 0);

	const int totalVoters = 1;
	bool majorityDetected = false;
	for (int count : ms.voteCounts) {
		if (count > totalVoters / 2) {
			majorityDetected = true;
			break;
		}
	}

	assert(!majorityDetected);
	return 0;
}
