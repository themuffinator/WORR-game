/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_reinforcement_selection.cpp implementation.*/

#include <array>
#include <cassert>
#include <cstdint>
#include <vector>

#include "src/server/monsters/reinforcement_selection.hpp"

/*
=============
main

Exercise deterministic reinforcement selection weighting and ordering.
=============
*/
int main() {
	uint32_t counts[3] = { 0, 0, 0 };
	uint32_t cursor = 0;
	std::vector<uint8_t> available = { 0, 1, 2 };

	assert(M_SelectReinforcementIndex(counts, 3, cursor, available) == 0);
	assert(cursor == 1);
	assert(counts[0] == reinforcement_selection_defaults.base_weight);

	assert(M_SelectReinforcementIndex(counts, 3, cursor, available) == 1);
	assert(cursor == 2);
	assert(counts[1] == reinforcement_selection_defaults.base_weight);

	assert(M_SelectReinforcementIndex(counts, 3, cursor, available) == 2);
	assert(cursor == 0);
	assert(counts[2] == reinforcement_selection_defaults.base_weight);

	counts[0] = 5;
	counts[1] = 2;
	counts[2] = 1;
	cursor = 0;

	assert(M_SelectReinforcementIndex(counts, 3, cursor, available) == 2);
	assert(cursor == 1);

	available = { 1, 2 };
	assert(M_SelectReinforcementIndex(counts, 3, cursor, available) == 1);
	assert(cursor == 2);

	return 0;
}
