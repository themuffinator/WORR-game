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

