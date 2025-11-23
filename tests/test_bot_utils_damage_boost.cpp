#include "server/bots/bot_utils.hpp"

#include <cassert>
#include <random>

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
std::mt19937 mt_rand{};

/*
=============
MakeTestPlayer

Creates a minimal player entity for bot state updates.
=============
*/
static gentity_t MakeTestPlayer(gclient_t &client) {
	gentity_t player{};
	player.client = &client;
	player.solid = SOLID_BBOX;
	player.sv.init = true;
	player.takeDamage = false;
	player.flags = 0;
	return player;
}

/*
=============
main

Verifies damage boost flags follow the damage boost timer list.
=============
*/
int main() {
	level.time = 0_ms;

	gclient_t client{};
	gentity_t player = MakeTestPlayer(client);

	for (const PowerupTimer timer : DamageBoostTimers) {
		client.ResetPowerups();
		player.sv.entFlags = SVFL_NONE;

		client.PowerupTimer(timer) = level.time + 1_sec;
		Entity_UpdateState(&player);
		assert((player.sv.entFlags & SVFL_HAS_DMG_BOOST) != 0);

		client.PowerupTimer(timer) = level.time - 1_sec;
		Entity_UpdateState(&player);
		assert((player.sv.entFlags & SVFL_HAS_DMG_BOOST) == 0);
	}

	return 0;
}
