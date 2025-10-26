#pragma once

#include <cstdint>

enum class TeamJoinCapacityAction {
	Allow,
	QueueForDuel,
	Deny
};

inline TeamJoinCapacityAction EvaluateTeamJoinCapacity(
	bool joinPlaying,
	bool requestQueue,
	bool force,
	bool wasPlaying,
	bool duel,
	bool isHuman,
	int playingHumans,
	int maxPlayers) {
	if (!joinPlaying || requestQueue || force || wasPlaying || !isHuman)
		return TeamJoinCapacityAction::Allow;

	if (maxPlayers <= 0)
		return TeamJoinCapacityAction::Allow;

	if (playingHumans < maxPlayers)
		return TeamJoinCapacityAction::Allow;

	if (duel)
		return TeamJoinCapacityAction::QueueForDuel;

	return TeamJoinCapacityAction::Deny;
}
