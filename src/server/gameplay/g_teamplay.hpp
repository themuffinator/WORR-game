#pragma once

#include "../g_local.hpp"

enum class FlagStatus : int {
	Invalid = -1,
	AtBase = 0,
	Taken = 1,
	TakenRed = 2,
	TakenBlue = 3,
	Dropped = 4
};

void Team_CaptureFlagSound(Team team);
bool SetFlagStatus(Team team, FlagStatus status);
void AwardFlagCapture(gentity_t* flagEntity, gentity_t* scorer, Team scoringTeam, GameTime pickupTime);
