#include "../src/server/player/team_join_capacity.hpp"

#include <cassert>

int main() {
	using Action = TeamJoinCapacityAction;

	// Allow when server has room for new human players.
	assert(EvaluateTeamJoinCapacity(true, false, false, false, false, true, 3, 4) == Action::Allow);

	// Ignore non-playing or already queued scenarios.
	assert(EvaluateTeamJoinCapacity(false, false, false, false, false, true, 5, 4) == Action::Allow);
	assert(EvaluateTeamJoinCapacity(true, true, false, false, false, true, 5, 4) == Action::Allow);

	// Humans that were already playing may always switch teams.
	assert(EvaluateTeamJoinCapacity(true, false, false, true, false, true, 5, 4) == Action::Allow);

	// Bots do not count toward human capacity checks.
	assert(EvaluateTeamJoinCapacity(true, false, false, false, false, false, 5, 4) == Action::Allow);

	// Full 1v1 games should redirect to the spectator queue.
	assert(EvaluateTeamJoinCapacity(true, false, false, false, true, true, 2, 2) == Action::QueueForDuel);

	// Standard matches deny joins beyond the cap.
	assert(EvaluateTeamJoinCapacity(true, false, false, false, false, true, 4, 4) == Action::Deny);

	return 0;
}
