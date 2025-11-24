#include <cassert>
#include <string>

#include "server/g_local.hpp"
#include "server/gameplay/g_harvester.hpp"

local_game_import_t gi{};
game_export_t globals{};
GameLocals game{};
LevelLocals level{};
cvar_t* g_gametype = nullptr;

namespace {
	int reminderCount = 0;
	std::string lastMessage;

	void TestLocPrint(gentity_t* ent, print_type_t, const char* base, const char**, size_t) {
		if (!ent) {
			return;
		}
		++reminderCount;
		lastMessage = base ? base : "";
	}
}

/*
=============
main

Confirms Harvester and OneFlag base reminders fire once per cooldown when missing objectives.
=============
*/
int main() {
	gi.Loc_Print = TestLocPrint;

	gentity_t player{};
	gclient_t client{};
	player.client = &client;

	Harvester_SendMissingObjectiveReminder(&player, true, false);
	assert(reminderCount == 1);
	assert(lastMessage == "$g_harvester_need_skulls");

	Harvester_SendMissingObjectiveReminder(&player, true, false);
	assert(reminderCount == 1);

	level.time += 3_sec;
	Harvester_SendMissingObjectiveReminder(&player, false, true);
	assert(reminderCount == 2);
	assert(lastMessage == "$g_oneflag_need_flag");

	return 0;
}
