#include "server/bots/bot_utils.hpp"

#include <cassert>
#include <random>

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
std::mt19937 mt_rand{};

/*
=============
Entity_UpdateState

Sets the damage boost flag when any active boost timer extends beyond the
current level time.
=============
*/
void Entity_UpdateState(gentity_t* ent) {
	if (!ent || !ent->client) {
		return;
	}

	ent->sv.entFlags &= ~SVFL_HAS_DMG_BOOST;
	for (const PowerupTimer timer : DamageBoostTimers) {
		if (ent->client->PowerupTimer(timer) > level.time) {
			ent->sv.entFlags |= SVFL_HAS_DMG_BOOST;
			break;
		}
	}
}

static gentity_t MakeTestPlayer(gclient_t &client) {
	gentity_t player{};
player.client = &client;
player.solid = SOLID_BBOX;
player.sv.init = true;
player.takeDamage = false;
player.flags = FL_NONE;
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
