/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_supertank_n64_tuning.cpp implementation.*/

#include <cassert>
#include <limits>

#include "server/g_local.hpp"

void SupertankApplyN64Tuning(gentity_t *self);

/*
=============
main

Verifies that N64 tuning enables the long-death loop, disables gibs, and leaves
manual defaults unchanged when not invoked.
=============
*/
int main() {
	gentity_t ent{};
	ent.gibHealth = -500;

	SupertankApplyN64Tuning(&ent);

	assert(ent.spawnFlags & SPAWNFLAG_SUPERTANK_LONG_DEATH);
	assert(ent.count == 10);
	assert(ent.gibHealth == std::numeric_limits<int32_t>::min());

	gentity_t untouched{};
	untouched.gibHealth = -500;

	assert(!(untouched.spawnFlags & SPAWNFLAG_SUPERTANK_LONG_DEATH));
	assert(untouched.count == 0);
	assert(untouched.gibHealth == -500);

	return 0;
}
