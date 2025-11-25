/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_horde_life_evaluation.cpp implementation.*/

#include "server/g_local.hpp"

#include <cassert>

/*
=============
main

Verifies limited-life elimination detection when health is zero or less.
=============
*/
int main() {
	gclient_t client{};
	client.pers.health = 0;
	client.pers.lives = 0;
	assert(ClientIsEliminatedFromLimitedLives(&client));

	client.pers.health = -25;
	client.pers.lives = 0;
	assert(ClientIsEliminatedFromLimitedLives(&client));

	client.pers.health = 0;
	client.pers.lives = 1;
	assert(!ClientIsEliminatedFromLimitedLives(&client));

	client.pers.health = 25;
	client.pers.lives = 0;
	assert(!ClientIsEliminatedFromLimitedLives(&client));

	return 0;
}
