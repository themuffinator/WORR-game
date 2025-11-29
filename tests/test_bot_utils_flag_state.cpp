/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_bot_utils_flag_state.cpp implementation.*/

#include "server/g_local.hpp"
#include "server/bots/bot_utils.hpp"
#include "server/gameplay/g_capture.hpp"

#include <cassert>
#include <unordered_map>

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};

static std::unordered_map<Team, FlagStatus> flagStatuses{
	{ Team::Red, FlagStatus::AtBase },
	{ Team::Blue, FlagStatus::AtBase },
};

/*
=============
SetFlagStatus

Stores the flag status per team for the lightweight capture test harness.
=============
*/
bool SetFlagStatus(Team team, FlagStatus status) {
	flagStatuses[team] = status;
	return true;
}

/*
=============
Entity_UpdateState

Updates the flag entity's server flags based on the cached flag status.
=============
*/
void Entity_UpdateState(gentity_t* ent) {
if (!ent || !ent->item) {
return;
}

	FlagStatus status = FlagStatus::AtBase;
	switch (ent->item->id) {
	case IT_FLAG_RED:
		status = flagStatuses[Team::Red];
		break;
	case IT_FLAG_BLUE:
		status = flagStatuses[Team::Blue];
		break;
	default:
		break;
	}

	ent->sv.entFlags &= ~(SVFL_OBJECTIVE_AT_BASE | SVFL_OBJECTIVE_TAKEN | SVFL_OBJECTIVE_DROPPED);
	if (status == FlagStatus::AtBase) {
		ent->sv.entFlags |= SVFL_OBJECTIVE_AT_BASE;
	}
	else if (status == FlagStatus::Dropped) {
		ent->sv.entFlags |= SVFL_OBJECTIVE_DROPPED;
	}
	else if (status == FlagStatus::Taken) {
		ent->sv.entFlags |= SVFL_OBJECTIVE_TAKEN;
}
}

/*
=============
main

Validates that flag entities update their status flags for each tracked state.
=============
*/
int main() {
	gitem_t redFlag{};
	redFlag.id = IT_FLAG_RED;
	gentity_t redEntity{};
	redEntity.item = &redFlag;

	SetFlagStatus(Team::Red, FlagStatus::AtBase);
redEntity.sv.entFlags = SVFL_NONE;
Entity_UpdateState(&redEntity);
assert(redEntity.sv.entFlags & SVFL_OBJECTIVE_AT_BASE);
assert(!(redEntity.sv.entFlags & SVFL_OBJECTIVE_TAKEN));
assert(!(redEntity.sv.entFlags & SVFL_OBJECTIVE_DROPPED));

SetFlagStatus(Team::Red, FlagStatus::Dropped);
redEntity.sv.entFlags = SVFL_NONE;
Entity_UpdateState(&redEntity);
assert(redEntity.sv.entFlags & SVFL_OBJECTIVE_DROPPED);
assert(!(redEntity.sv.entFlags & SVFL_OBJECTIVE_TAKEN));

SetFlagStatus(Team::Red, FlagStatus::Taken);
redEntity.sv.entFlags = SVFL_NONE;
Entity_UpdateState(&redEntity);
assert(redEntity.sv.entFlags & SVFL_OBJECTIVE_TAKEN);
assert(!(redEntity.sv.entFlags & SVFL_OBJECTIVE_AT_BASE));

	gitem_t blueFlag{};
	blueFlag.id = IT_FLAG_BLUE;
	gentity_t blueEntity{};
	blueEntity.item = &blueFlag;

	SetFlagStatus(Team::Blue, FlagStatus::AtBase);
blueEntity.sv.entFlags = SVFL_NONE;
Entity_UpdateState(&blueEntity);
assert(blueEntity.sv.entFlags & SVFL_OBJECTIVE_AT_BASE);

	return 0;
}

