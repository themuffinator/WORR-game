#include <cassert>

#include "server/gameplay/g_capture.hpp"
#include "server/gameplay/g_teamplay.hpp"

GameLocals game{};
LevelLocals level{};
game_export_t globals{};
local_game_import_t gi{};
cvar_t* g_gametype = nullptr;

/*
=============
main

Validates that hurt carrier tracking ignores non-primary teams.
=============
*/
int main() {
	cvar_t gametype{};
	g_gametype = &gametype;
	g_gametype->integer = static_cast<int>(GameType::CaptureTheFlag);

	gentity_t carrier{};
	gclient_t carrierClient{};
	carrier.client = &carrierClient;
	carrier.client->sess.team = Team::Blue;
	carrier.client->pers.inventory[IT_FLAG_RED] = 1;

	gentity_t attacker{};
	gclient_t attackerClient{};
	attacker.client = &attackerClient;
	attacker.client->sess.team = Team::Red;
	attacker.client->resp.ctf_lasthurtcarrier = 0_ms;

	level.time = 5_sec;
	CTF_CheckHurtCarrier(&carrier, &attacker);
	assert(attacker.client->resp.ctf_lasthurtcarrier == level.time);

	attacker.client->resp.ctf_lasthurtcarrier = 0_ms;
	attacker.client->sess.team = Team::Spectator;
	level.time = 10_sec;
	CTF_CheckHurtCarrier(&carrier, &attacker);
	assert(attacker.client->resp.ctf_lasthurtcarrier == 0_ms);

	gentity_t neutralCarrier{};
	gclient_t neutralCarrierClient{};
	neutralCarrier.client = &neutralCarrierClient;
	neutralCarrier.client->sess.team = Team::Free;
	neutralCarrier.client->pers.inventory[IT_FLAG_RED] = 1;

	attacker.client->sess.team = Team::Red;
	attacker.client->resp.ctf_lasthurtcarrier = 15_sec;
	level.time = 20_sec;
	CTF_CheckHurtCarrier(&neutralCarrier, &attacker);
	assert(attacker.client->resp.ctf_lasthurtcarrier == 15_sec);

	return 0;
}
