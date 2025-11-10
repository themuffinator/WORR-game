// g_match_state.cpp (Game Match State)
// This file manages the high-level state and flow of a multiplayer match.
// It controls the transitions between different phases of a game, such as
// warmup, countdown, active gameplay, and post-game intermission. It is the
// central authority for enforcing game rules and round-based logic.
//
// Key Responsibilities:
// - Match Lifecycle: Implements the state machine for the match, progressing
//   from `MatchState::Warmup` to `MatchState::Countdown` to `MatchState::In_Progress`.
// - Rule Enforcement: `CheckDMExitRules` is called every frame to check for
//   end-of-match conditions like timelimit, scorelimit, or mercylimit.
// - Round-Based Logic: Manages the start and end of rounds for gametypes like
//   Clan Arena and Horde mode (`Round_StartNew`, `Round_End`).
// - Warmup and Ready-Up: Handles the "ready-up" system, where the match will
//   not start until a certain percentage of players have indicated they are ready.
// - Gametype Switching: Contains the logic to cleanly switch between different
//   gametypes (`ChangeGametype`) by reloading the map and resetting state.

#include "../g_local.hpp"
#include "../gameplay/g_headhunters.hpp"
#include "../gameplay/client_config.hpp"
#include "../commands/command_registration.hpp"
#include "../match/match.hpp"
#include "../commands/commands.hpp"
#include "match_state_utils.hpp"
#include "match_state_helper.hpp"

#include <array>
#include <string>

using LevelMatchTransition = MatchStateTransition<LevelLocals>;

static void SetMatchState(LevelMatchTransition transition) {
	ApplyMatchState(level, transition);
}

namespace {
	[[nodiscard]] bool MarathonEnabledForMatch() {
		return deathmatch->integer && marathon && marathon->integer;
	}

	bool MarathonShouldCarryScores() {
		return game.marathon.active && game.marathon.legIndex > 0;
	}

	void MarathonResetState() {
		game.marathon = {};
	}

	void MarathonEnsureStateForMatch() {
		if (!MarathonEnabledForMatch()) {
			MarathonResetState();
			return;
		}

		if (!game.marathon.active) {
			MarathonResetState();
			game.marathon.active = true;
		}
	}

	void MarathonRecordMapStart() {
		if (!game.marathon.active)
			return;

		game.marathon.transitionPending = false;
		game.marathon.mapStartTime = level.time;
		game.marathon.mapStartTeamScores = level.teamScores;

		if (!level.matchID.empty())
			game.marathon.matchID = level.matchID;
		game.marathon.mapStartScoreValid.fill(false);

		for (size_t i = 0; i < game.maxClients; ++i) {
			if (!game.clients[i].pers.connected)
				continue;

			game.marathon.mapStartPlayerScores[i] = game.clients[i].resp.score;
			game.marathon.mapStartScoreValid[i] = true;
		}

		if (Teams() && Game::IsNot(GameType::RedRover))
			level.teamOldScores = level.teamScores;
	}

	void MarathonAccumulateElapsed() {
		if (!game.marathon.active)
			return;

		GameTime elapsed = level.time - game.marathon.mapStartTime;
		if (elapsed < 0_ms)
			elapsed = 0_ms;

		game.marathon.totalElapsedBeforeCurrentMap += elapsed;
	}

	bool MarathonCheckTimeLimit(std::string& message) {
		if (!game.marathon.active || !g_marathon_timelimit || g_marathon_timelimit->value <= 0.0f)
			return false;

		const GameTime limit = GameTime::from_min(g_marathon_timelimit->value);
		if (!limit)
			return false;

		GameTime elapsed = level.time - game.marathon.mapStartTime;
		if (elapsed < limit)
			return false;

		message = G_Fmt("Marathon: Time limit ({:.2g} min) reached.", g_marathon_timelimit->value);
		return true;
	}

	bool MarathonCheckScoreLimit(std::string& message) {
		if (!game.marathon.active || !g_marathon_scorelimit)
			return false;

		const int limit = g_marathon_scorelimit->integer;
		if (limit <= 0)
			return false;

		if (Teams() && Game::IsNot(GameType::RedRover)) {
			for (Team team : { Team::Red, Team::Blue }) {
				const size_t index = static_cast<size_t>(team);
				const int start = game.marathon.mapStartTeamScores[index];
				const int current = level.teamScores[index];

				if (current - start >= limit) {
					message = G_Fmt("Marathon: {} gained {} points this map.", Teams_TeamName(team), limit);
					return true;
				}
			}
		}
		else {
			for (auto ec : active_clients()) {
				auto* cl = ec->client;
				if (!ClientIsPlaying(cl))
					continue;

				const size_t index = static_cast<size_t>(cl - game.clients);
				if (index >= game.maxClients)
					continue;

				if (!game.marathon.mapStartScoreValid[index]) {
					game.marathon.mapStartPlayerScores[index] = cl->resp.score;
					game.marathon.mapStartScoreValid[index] = true;
				}

				if (cl->resp.score - game.marathon.mapStartPlayerScores[index] >= limit) {
					message = G_Fmt("Marathon: {} hit {} points this map.", cl->sess.netName, limit);
					return true;
				}
			}
		}

		return false;
	}

	void MarathonTriggerAdvance(const std::string& message) {
		if (!game.marathon.active || game.marathon.transitionPending)
			return;

		MarathonAccumulateElapsed();
		game.marathon.cumulativeTeamScores = level.teamScores;
		game.marathon.matchID = level.matchID;
		game.marathon.transitionPending = true;
		game.marathon.legIndex++;
		QueueIntermission(message.c_str(), false, false);
	}

} // namespace

void Marathon_RegisterClientBaseline(gclient_t* cl) {
	if (!cl || !game.marathon.active)
		return;

	const ptrdiff_t index = cl - game.clients;
	if (index < 0 || static_cast<size_t>(index) >= game.maxClients)
		return;

	game.marathon.mapStartPlayerScores[static_cast<size_t>(index)] = cl->resp.score;
	game.marathon.mapStartScoreValid[static_cast<size_t>(index)] = true;
}

// Utility scope guard used by CheckDMExitRules to track whether any
// grace-based endmatch condition fired during the current frame. When the
// scope ends without a condition being marked active, it automatically resets
// the grace timer so future violations receive a full grace window.
template<typename TimeT>
class EndmatchGraceScope {
public:
	EndmatchGraceScope(TimeT& timer, TimeT zeroValue)
		: timer(timer)
		, zeroValue(zeroValue) {
	}

	void MarkConditionActive() {
		active = true;
	}

	[[nodiscard]] bool ConditionWasActive() const {
		return active;
	}

	~EndmatchGraceScope() {
		if (!active && timer)
			timer = zeroValue;
	}

private:
	TimeT& timer;
	TimeT zeroValue;
	bool active = false;
};

/*
=================
str_split
=================
*/
static inline std::vector<std::string> str_split(const std::string_view& str, char by) {
	std::vector<std::string> out;

	size_t start = 0;
	while (start < str.size()) {
		start = str.find_first_not_of(by, start);
		if (start == std::string_view::npos)
			break;

		const size_t end = str.find(by, start);
		if (end == std::string_view::npos) {
			out.emplace_back(str.substr(start));
			break;
		}

		out.emplace_back(str.substr(start, end - start));
		start = end + 1;
	}

	return out;
}

constexpr struct GameTypeRules {
	GameFlags				flags = GameFlags::None;
	uint8_t					weaponRespawnDelay = 8;	// in seconds. if 0, weapon stay is on
	bool					holdables = true;			// can hold items such as adrenaline and personal teleporter
	bool					powerupsEnabled = true;	// yes to powerups?
	uint8_t					scoreLimit = 40;
	uint8_t					timeLimit = 10;
	bool					startingHealthBonus = true;
	float					readyUpPercentile = 0.51f;
} gt_rules_t[static_cast<int>(GameType::Total)] = {
	/* GameType::FreeForAll */ {GameFlags::Frags},
	/* GameType::Duel */ {GameFlags::Frags, 30, false, false, 0},
	/* GameType::TeamDeathmatch */ {GameFlags::Teams | GameFlags::Frags, 30, true, true, 100, 20},
	/* GameType::Domination */ {GameFlags::Teams | GameFlags::Frags, 30, true, true, 100, 20},
	/* GameType::CaptureTheFlag */ {GameFlags::Teams | GameFlags::CTF, 30},
	/* GameType::ClanArena */ { },
	/* GameType::OneFlag */ { },
	/* GameType::Harvester */ {GameFlags::Teams | GameFlags::CTF, 30},
	/* GameType::Overload */ { },
	/* GameType::FreezeTag */ { },
	/* GameType::CaptureStrike */ { },
	/* GameType::RedRover */ { },
	/* GameType::LastManStanding */ { },
	/* GameType::LastTeamStanding */ { },
	/* GameType::Horde */ { },
	/* GameType::HeadHunters */ {GameFlags::None, 8, true, true, 25, 15},
	/* GameType::ProBall */ {GameFlags::Teams, 0, false, false, 10, 15, false, 0.6f},
	/* GameType::Gauntlet */ { }
};

static void Monsters_KillAll() {
	for (size_t i = 0; i < globals.maxEntities; i++) {
		if (!g_entities[i].inUse)
			continue;
		if (!(g_entities[i].svFlags & SVF_MONSTER))
			continue;
		FreeEntity(&g_entities[i]);
	}
	level.campaign.totalMonsters = 0;
	level.campaign.killedMonsters = 0;
}

/*
============
Match reset helpers
============
*/
enum class LimitedLivesResetMode {
	Auto,
	Force,
};

static bool ShouldResetLimitedLives(LimitedLivesResetMode mode) {
	if (!G_LimitedLivesActive())
		return false;

	if (G_LimitedLivesInCoop())
		return true;

	return mode == LimitedLivesResetMode::Force;
}

static void ResetMatchWorldState(bool reloadWorldEntities) {
	bool reloadedEntities = false;
	if (reloadWorldEntities && deathmatch->integer) {
		reloadedEntities = G_ResetWorldEntitiesFromSavedString();
	}

	Tech_Reset();
	CTF_ResetFlags();
	Harvester_Reset();

	if (!reloadedEntities) {
		Monsters_KillAll();
	}

	gentity_t* ent;
	size_t i;

	for (ent = g_entities + 1, i = 1; i < globals.numEntities; i++, ent++) {
		if (!ent->inUse)
			continue;

		if (Q_strcasecmp(ent->className, "gib") == 0) {
			ent->svFlags = SVF_NOCLIENT;
			ent->takeDamage = false;
			ent->solid = SOLID_NOT;
			gi.unlinkEntity(ent);
			FreeEntity(ent);
		}
		else if ((ent->svFlags & SVF_PROJECTILE) || (ent->clipMask & CONTENTS_PROJECTILECLIP)) {
			FreeEntity(ent);
		}
		else if (ent->item) {
			if (ent->item->id == IT_FLAG_RED || ent->item->id == IT_FLAG_BLUE)
				continue;

			if (ent->spawnFlags.has(SPAWNFLAG_ITEM_DROPPED | SPAWNFLAG_ITEM_DROPPED_PLAYER)) {
				ent->nextThink = level.time;
			}
			else if (ent->item->flags & IF_POWERUP) {
				if (g_quadhog->integer && ent->item->id == IT_POWERUP_QUAD) {
					FreeEntity(ent);
					QuadHog_SetupSpawn(5_sec);
				}
				else {
					ent->svFlags |= SVF_NOCLIENT;
					ent->solid = SOLID_NOT;
					ent->nextThink = level.time + GameTime::from_sec(irandom(30, 60));
					ent->think = RespawnItem;
				}
			}
			else if (ent->svFlags & (SVF_NOCLIENT | SVF_RESPAWNING) || ent->solid == SOLID_NOT) {
				GameTime t = 0_sec;
				if (ent->random) {
					t += GameTime::from_ms((crandom() * ent->random) * 1000);
					if (t < FRAME_TIME_MS) {
						t = FRAME_TIME_MS;
					}
				}
				ent->think = RespawnItem;
				ent->nextThink = level.time + t;
			}
		}
	}
}

static void ResetMatchPlayers(bool resetGhost, bool resetScore, LimitedLivesResetMode limitedLivesResetMode = LimitedLivesResetMode::Auto) {
	for (auto ec : active_clients()) {
		ec->client->resp.ctf_state = 0;
		if (ShouldResetLimitedLives(limitedLivesResetMode)) {
			ec->client->pers.lives = G_LimitedLivesMax();
			ec->client->pers.limitedLivesStash = ec->client->pers.lives;
			ec->client->pers.limitedLivesPersist = false;
			if (G_LimitedLivesInCoop())
				ec->client->resp.coopRespawn.lives = ec->client->pers.lives;
		}

		if (resetScore)
			ec->client->resp.score = 0;

		if (resetGhost) {
			// Reserved for ghost handling
		}

		if (ClientIsPlaying(ec->client)) {
			if (resetGhost) {
				// Reserved for ghost handling
			}

			Weapon_Grapple_DoReset(ec->client);
			ec->client->eliminated = false;
			ec->client->pers.readyStatus = false;
			ec->moveType = MoveType::NoClip;
			ec->client->respawnMaxTime = level.time + FRAME_TIME_MS;
			ec->svFlags &= ~SVF_NOCLIENT;
			ClientSpawn(ec);
			G_PostRespawn(ec);
			memset(&ec->client->pers.match, 0, sizeof(ec->client->pers.match));

			gi.linkEntity(ec);
		}
	}

	CalculateRanks();
}
// =================================================

static void RoundAnnounceWin(Team team, const char* reason) {
	G_AdjustTeamScore(team, 1);
	gi.LocBroadcast_Print(PRINT_CENTER, "{} wins the round!\n({})\n", Teams_TeamName(team), reason);
	AnnouncerSound(world, team == Team::Red ? "red_wins_round" : "blue_wins_round");
}

static void RoundAnnounceDraw() {
	gi.Broadcast_Print(PRINT_CENTER, "Round draw!\n");
	AnnouncerSound(world, "round_draw");
}

static bool IsFreezeTagPlayerFrozen(const gentity_t* ent) {
	if (!ent || !ent->client)
		return false;

	if (!ClientIsPlaying(ent->client))
		return false;

	switch (ent->client->sess.team) {
	case Team::Red:
	case Team::Blue:
		break;
	default:
		return false;
	}

	return ent->client->eliminated || ent->client->ps.pmove.pmType == PM_DEAD;
}

static void CheckRoundFreezeTag() {
	bool redHasPlayers = false;
	bool blueHasPlayers = false;
	bool redAllFrozen = true;
	bool blueAllFrozen = true;

	for (auto ec : active_players()) {
		switch (ec->client->sess.team) {
		case Team::Red:
			redHasPlayers = true;
			if (!IsFreezeTagPlayerFrozen(ec))
				redAllFrozen = false;
			break;
		case Team::Blue:
			blueHasPlayers = true;
			if (!IsFreezeTagPlayerFrozen(ec))
				blueAllFrozen = false;
			break;
		default:
			break;
		}
	}

	if (redHasPlayers && blueHasPlayers && redAllFrozen) {
		RoundAnnounceWin(Team::Blue, "froze the enemy team");
		Round_End();
		return;
	}

	if (redHasPlayers && blueHasPlayers && blueAllFrozen) {
		RoundAnnounceWin(Team::Red, "froze the enemy team");
		Round_End();
	}
}

static void CheckRoundEliminationCA() {
	int redAlive = 0, blueAlive = 0;
	for (auto ec : active_players()) {
		if (ec->health <= 0)
			continue;
		switch (ec->client->sess.team) {
		case Team::Red: redAlive++; break;
		case Team::Blue: blueAlive++; break;
		}
	}

	if (redAlive && !blueAlive) {
		RoundAnnounceWin(Team::Red, "eliminated blue team");
		Round_End();
	}
	else if (blueAlive && !redAlive) {
		RoundAnnounceWin(Team::Blue, "eliminated red team");
		Round_End();
	}
}

static void CheckRoundTimeLimitCA() {
	if (level.pop.num_living_red > level.pop.num_living_blue) {
		RoundAnnounceWin(Team::Red, "players remaining");
	}
	else if (level.pop.num_living_blue > level.pop.num_living_red) {
		RoundAnnounceWin(Team::Blue, "players remaining");
	}
	else {
		int healthRed = 0, healthBlue = 0;
		for (auto ec : active_players()) {
			if (ec->health <= 0) continue;
			switch (ec->client->sess.team) {
			case Team::Red: healthRed += ec->health; break;
			case Team::Blue: healthBlue += ec->health; break;
			}
		}
		if (healthRed > healthBlue) {
			RoundAnnounceWin(Team::Red, "total health");
		}
		else if (healthBlue > healthRed) {
			RoundAnnounceWin(Team::Blue, "total health");
		}
		else {
			RoundAnnounceDraw();
		}
	}
	Round_End();
}

static void CheckRoundHorde() {
	Horde_RunSpawning();
	if (level.horde_all_spawned && !(level.campaign.totalMonsters - level.campaign.killedMonsters)) {
		gi.Broadcast_Print(PRINT_CENTER, "Monsters eliminated!\n");
		gi.positionedSound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundIndex("ctf/flagcap.wav"), 1, ATTN_NONE, 0);
		Round_End();
	}
}

static void CheckRoundRR() {
	if (!level.pop.num_playing_red || !level.pop.num_playing_blue) {
		gi.Broadcast_Print(PRINT_CENTER, "Round Ends!\n");
		gi.positionedSound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundIndex("ctf/flagcap.wav"), 1, ATTN_NONE, 0);
		if (level.roundNumber + 1 >= roundLimit->integer) {
			QueueIntermission("MATCH ENDED", false, false);
		}
		else {
			Round_End();
		}
	}
}

static void CheckRoundStrikeTimeLimit() {
	if (level.strike_flag_touch) {
		RoundAnnounceWin(level.strike_red_attacks ? Team::Red : Team::Blue, "scored a point");
	}
	else {
		const Team defendingTeam = level.strike_red_attacks ? Team::Blue : Team::Red;
		RoundAnnounceWin(defendingTeam, "successfully defended");
		gi.LocBroadcast_Print(PRINT_CENTER, "Turn has ended.\n{} successfully defended!", Teams_TeamName(defendingTeam));
	}
	Round_End();
}

static void CheckRoundStrikeStartTurn() {
	if (!level.strike_turn_red && level.strike_red_attacks) {
		level.strike_turn_red = true;
	}
	else if (!level.strike_turn_blue && !level.strike_red_attacks) {
		level.strike_turn_blue = true;
	}
	else {
		level.strike_turn_red = level.strike_red_attacks;
		level.strike_turn_blue = !level.strike_red_attacks;
	}
}

static gclient_t* GetNextQueuedPlayer() {
	gclient_t* next = nullptr;
	for (auto ec : active_clients()) {
		if (ec->client->sess.matchQueued && !ClientIsPlaying(ec->client)) {
			gclient_t* candidate = ec->client;
			if (!next) {
				next = candidate;
				continue;
			}
			const uint64_t candidateTicket = candidate->sess.duelQueueTicket;
			const uint64_t nextTicket = next->sess.duelQueueTicket;
			if (candidateTicket && nextTicket) {
				if (candidateTicket < nextTicket)
					next = candidate;
			}
			else if (candidateTicket && !nextTicket) {
				next = candidate;
			}
			else if (!candidateTicket && !nextTicket) {
				if (candidate->sess.teamJoinTime < next->sess.teamJoinTime)
					next = candidate;
			}
		}
	}
	return next;
}

static bool Versus_AddPlayer() {
	if (Game::Has(GameFlags::OneVOne) && level.pop.num_playing_clients >= 2)
		return false;
	if (level.matchState > MatchState::Warmup_Default || level.intermission.time || level.intermission.queued)
		return false;

	gclient_t* next = GetNextQueuedPlayer();
	if (!next)
		return false;

	SetTeam(&g_entities[next - game.clients + 1], Team::Free, false, true, false);

	return true;
}

void Duel_RemoveLoser() {
	if (Game::IsNot(GameType::Duel) || level.pop.num_playing_clients != 2)
		return;

	gentity_t* loser = &g_entities[level.sortedClients[1] + 1];
	if (!loser || !loser->client || !loser->client->pers.connected)
		return;
	if (!ClientIsPlaying(loser->client))
		return;

	if (g_verbose->integer)
            gi.Com_PrintFmt("Duel: Moving the loser, {} to spectator queue.\n", loser->client->sess.netName);

	SetTeam(loser, Team::None, false, true, false);

	Versus_AddPlayer();
}

void Gauntlet_RemoveLoser() {
	if (Game::IsNot(GameType::Gauntlet) || level.pop.num_playing_clients != 2)
		return;

	gentity_t* loser = &g_entities[level.sortedClients[1] + 1];
	if (!loser || !loser->client || !loser->client->pers.connected)
		return;
	if (loser->client->sess.team != Team::Free)
		return;

	if (g_verbose->integer)
            gi.Com_PrintFmt("Gauntlet: Moving the loser, {} to end of queue.\n", loser->client->sess.netName);

	SetTeam(loser, Team::None, false, true, false);
}

void Gauntlet_MatchEnd_AdjustScores() {
	if (Game::IsNot(GameType::Gauntlet))
		return;
	if (level.pop.num_playing_clients < 2)
		return;

	int winnerNum = level.sortedClients[0];
	if (game.clients[winnerNum].pers.connected) {
		game.clients[winnerNum].sess.matchWins++;
	}
}

static size_t CollectActiveDuelists(std::array<gclient_t*, 2>& duelists) {
	size_t found = 0;

	for (int clientIndex : level.sortedClients) {
		if (clientIndex < 0 || clientIndex >= static_cast<int>(game.maxClients))
			continue;

		gclient_t* cl = &game.clients[clientIndex];
		if (!cl->pers.connected || !ClientIsPlaying(cl))
			continue;

		duelists[found++] = cl;
		if (found == duelists.size())
			break;
	}

	return found;
}

void Match_UpdateDuelRecords() {
	if (!Game::Has(GameFlags::OneVOne))
		return;
	if (level.intermission.duelWinLossApplied)
		return;

	CalculateRanks();

	std::array<gclient_t*, 2> duelists{};
	if (CollectActiveDuelists(duelists) != duelists.size())
		return;

	gclient_t* first = duelists[0];
	gclient_t* second = duelists[1];
	if (!first || !second)
		return;

	gclient_t* winner = first;
	gclient_t* loser = second;

	if (second->resp.score > first->resp.score)
		std::swap(winner, loser);

	if (!winner || !loser)
		return;

	if (winner->resp.score == loser->resp.score)
		return;

	winner->sess.matchWins++;
	loser->sess.matchLosses++;
	level.intermission.duelWinLossApplied = true;
}

static void EnforceDuelRules() {
	if (Game::IsNot(GameType::Duel))
		return;

	if (level.pop.num_playing_clients > 2) {
		// Kick or move spectators if too many players
		for (auto ec : active_clients()) {
			if (ClientIsPlaying(ec->client)) {
				// Allow the first two
				continue;
			}
			if (ec->client->sess.team != Team::Spectator) {
				SetTeam(ec, Team::Spectator, false, true, false);
				gi.LocClient_Print(ec, PRINT_HIGH, "This is a Duel match (1v1 only).\nYou have been moved to spectator.");
			}
		}
	}
}

/*
=============
Round_StartNew
=============
*/
static bool Round_StartNew() {
	if (!Game::Has(GameFlags::Rounds)) {
		level.roundState = RoundState::None;
		level.roundStateTimer = 0_sec;
		return false;
	}

	bool horde = Game::Is(GameType::Horde);

	level.roundState = RoundState::Countdown;
	level.roundStateTimer = level.time + 10_sec;
	level.countdownTimerCheck = 0_sec;

	if (!horde) {
		ResetMatchWorldState(true);
		ResetMatchPlayers(false, false);
	}

	if (Game::Is(GameType::FreezeTag)) {
		for (auto ec : active_clients()) {
			gclient_t* cl = ec->client;
			if (!cl)
				continue;

			cl->resp.thawer = nullptr;
			cl->resp.help = 0;
			cl->resp.thawed = 0;
			cl->freeze.thawTime = 0_ms;
			cl->freeze.frozenTime = 0_ms;
			cl->eliminated = false;
		}
	}

	if (Game::Is(GameType::CaptureStrike)) {
		level.strike_red_attacks ^= true;
		level.strike_flag_touch = false;

		int round_num;
		if (level.roundNumber && (!level.strike_turn_red && level.strike_turn_blue ||
			level.strike_turn_red && !level.strike_turn_blue))
			round_num = level.roundNumber;
		else {
			round_num = level.roundNumber + 1;
		}
		BroadcastTeamMessage(Team::Red, PRINT_CENTER, G_Fmt("Your team is on {}!\nRound {} - Begins in...", level.strike_red_attacks ? "OFFENSE" : "DEFENSE", round_num).data());
		BroadcastTeamMessage(Team::Blue, PRINT_CENTER, G_Fmt("Your team is on {}!\nRound {} - Begins in...", !level.strike_red_attacks ? "OFFENSE" : "DEFENSE", round_num).data());
	}
	else {
		int round_num;

		if (horde && !level.roundNumber && g_horde_starting_wave->integer > 0)
			round_num = g_horde_starting_wave->integer;
		else
			round_num = level.roundNumber + 1;

		if (Game::Is(GameType::RedRover) && roundLimit->integer) {
			gi.LocBroadcast_Print(PRINT_CENTER, "{} {} of {}\nBegins in...", horde ? "Wave" : "Round", round_num, roundLimit->integer);
		}
		else
			gi.LocBroadcast_Print(PRINT_CENTER, "{} {}\nBegins in...", horde ? "Wave" : "Round", round_num);
	}

	AnnouncerSound(world, "round_begins_in");

	return true;
}

/*
=============
Round_End
=============
*/
void Round_End() {
	// reset if not round based
	if (!Game::Has(GameFlags::Rounds)) {
		level.roundState = RoundState::None;
		level.roundStateTimer = 0_sec;
		return;
	}

	// there must be a round to end
	if (level.roundState != RoundState::In_Progress)
		return;

	level.roundState = RoundState::Ended;
	level.roundStateTimer = level.time + 3_sec;
	level.horde_all_spawned = false;
}

/*
============
Match_Start

Starts a match
============
*/

extern void MatchStats_Init();
void Match_Start() {
	if (!deathmatch->integer)
		return;

	MarathonEnsureStateForMatch();
	const bool carryScores = MarathonShouldCarryScores();

	time_t now = GetCurrentRealTimeMillis();

	if (!carryScores)
		level.matchStartRealTime = now;

	level.matchEndRealTime = 0;
	level.levelStartTime = carryScores ? level.time - game.marathon.totalElapsedBeforeCurrentMap : level.time;
	level.overtime = 0_sec;

	const char* s = TimeString(timeLimit->value ? timeLimit->value * 1000 : 0, false, true);
	gi.configString(CONFIG_MATCH_STATE, s);

	level.matchState = MatchState::In_Progress;
	level.matchStateTimer = level.time;
	level.warmupState = WarmupState::Default;
	level.warmupNoticeTime = 0_sec;

	if (carryScores) {
		level.teamScores[static_cast<int>(Team::Red)] = game.marathon.cumulativeTeamScores[static_cast<int>(Team::Red)];
		level.teamScores[static_cast<int>(Team::Blue)] = game.marathon.cumulativeTeamScores[static_cast<int>(Team::Blue)];
	}
	else {
		level.teamScores[static_cast<int>(Team::Red)] = level.teamScores[static_cast<int>(Team::Blue)] = 0;
		game.marathon.cumulativeTeamScores = level.teamScores;
		level.match = {};
	}

	if (carryScores && !game.marathon.matchID.empty())
		level.matchID = game.marathon.matchID;

	Monsters_KillAll();
	ResetMatchWorldState(true);
	ResetMatchPlayers(true, !carryScores);
	UnReadyAll();

	if (!carryScores) {
		for (auto ec : active_players())
			ec->client->sess.playStartRealTime = now;
	}

	if (!carryScores)
		MatchStats_Init();

	if (Game::Is(GameType::CaptureStrike))
		level.strike_red_attacks = brandom();

	MarathonRecordMapStart();

	if (Round_StartNew())
		return;

	gi.LocBroadcast_Print(PRINT_CENTER, ".FIGHT!");
	AnnouncerSound(world, "fight");
}

/*
===============================
SetMapLastPlayedTime
===============================
*/
static void SetMapLastPlayedTime(const char* mapname) {
	if (!mapname || !*mapname || game.serverStartTime == 0)
		return;

	time_t now = time(nullptr);
	int secondsSinceStart = static_cast<int>(now - game.serverStartTime);

	for (auto& map : game.mapSystem.mapPool) {
		if (_stricmp(map.filename.c_str(), mapname) == 0) {
			map.lastPlayed = secondsSinceStart;
			break;
		}
	}
}

// =============================================================

#include <cmath>
#include <vector>
#include <algorithm>

constexpr float SKILL_K = 32.0f;

// helper to get all playing clients
static std::vector<gentity_t*> GetPlayers() {
	std::vector<gentity_t*> out;
	for (auto ent : active_clients()) {
		if (ClientIsPlaying(ent->client))
			out.push_back(ent);
	}
	return out;
}

// calculate Elo expectation
static float EloExpected(float ra, float rb) {
	return 1.0f / (1.0f + std::pow(10.0f, (rb - ra) / 400.0f));
}

/*
===============
DidPlayerWin
===============
*/
static bool DidPlayerWin(gentity_t* ent) {
	if (Game::Is(GameType::Duel)) {
		auto players = GetPlayers();
		if (players.size() == 2)
			return (ent->client->resp.score > players[0]->client->resp.score || ent == players[0]);
	}

	if (Teams() && Game::IsNot(GameType::RedRover)) {
		const int redScore = level.teamScores[static_cast<int>(Team::Red)];
		const int blueScore = level.teamScores[static_cast<int>(Team::Blue)];

		if (ent->client->sess.team == Team::Red)
			return redScore > blueScore;
		else if (ent->client->sess.team == Team::Blue)
			return blueScore > redScore;
	}

	// FFA
	auto players = GetPlayers();
	if (!players.empty())
		std::sort(players.begin(), players.end(),
			[](gentity_t* a, gentity_t* b) {
				return a->client->resp.score > b->client->resp.score;
			});
	return (ent == players.front());
}

/*
===============
AdjustSkillRatings
===============
*/
static void AdjustSkillRatings() {
	if (level.pop.num_playing_clients != level.pop.num_playing_human_clients) {
		// Not all players are human, so we can't adjust skill ratings
		if (g_verbose->integer)
			gi.Com_Print("AdjustSkillRatings: Not all players are human, skipping skill rating adjustment.\n");

		// Update all player config files regardless
		for (auto ec : active_players()) {
			// Save stats for all players
			ClientConfig_SaveStats(ec->client, false);
		}
		return;
	}

	// Sync sess.skillRating with config.skillRating
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		gentity_t* ent = &g_entities[i];
		if (!ent->inUse || !ent->client)
			continue;

		gclient_t* cl = ent->client;
		if (cl->sess.skillRating != cl->sess.skillRating)
			cl->sess.skillRating = cl->sess.skillRating;
	}

	auto players = GetPlayers();
	if (players.empty())
		return;

	const bool isTeamMatch = Teams() && Game::IsNot(GameType::RedRover);

	// === DUEL MODE ===
	if (Game::Is(GameType::Duel) && players.size() == 2) {
		auto* a = players[0], * b = players[1];
		float Ra = a->client->sess.skillRating;
		float Rb = b->client->sess.skillRating;
		bool aWon = a->client->resp.score > b->client->resp.score;
		float Ea = EloExpected(Ra, Rb);
		float Eb = 1.0f - Ea;

		float dA = SKILL_K * ((aWon ? 1.0f : 0.0f) - Ea);
		float dB = SKILL_K * ((aWon ? 0.0f : 1.0f) - Eb);

		a->client->sess.skillRating += dA;
		b->client->sess.skillRating += dB;

		a->client->sess.skillRatingChange = static_cast<int>(dA);
		b->client->sess.skillRatingChange = static_cast<int>(dB);

		ClientConfig_SaveStats(a->client, aWon);
		ClientConfig_SaveStats(b->client, !aWon);

		// Ghosts
		for (auto& g : level.ghosts) {
			if (!*g.socialID)
				continue;
			if (Q_strcasecmp(g.socialID, a->client->sess.socialID) == 0) {
				g.skillRating += dA;
				g.skillRatingChange = static_cast<int>(dA);
				ClientConfig_SaveStatsForGhost(g, aWon);
			}
			else if (Q_strcasecmp(g.socialID, b->client->sess.socialID) == 0) {
				g.skillRating += dB;
				g.skillRatingChange = static_cast<int>(dB);
				ClientConfig_SaveStatsForGhost(g, !aWon);
			}
		}
		return;
	}

	// === TEAM MODE ===
	if (isTeamMatch && players.size() >= 2) {
		std::vector<gentity_t*> red, blue;
		for (auto* ent : players) {
			if (ent->client->sess.team == Team::Red)
				red.push_back(ent);
			else if (ent->client->sess.team == Team::Blue)
				blue.push_back(ent);
		}
		if (red.empty() || blue.empty())
			return;

		auto avg = [](const std::vector<gentity_t*>& v) {
			float sum = 0;
			for (auto* e : v)
				sum += e->client->sess.skillRating;
			return sum / v.size();
			};

		float Rr = avg(red), Rb = avg(blue);
		float Er = EloExpected(Rr, Rb), Eb = 1.0f - Er;

		const int Sr = level.teamScores[static_cast<int>(Team::Red)];
		const int Sb = level.teamScores[static_cast<int>(Team::Blue)];

		bool redWin = Sr > Sb;

		for (auto* e : red) {
			float S = redWin ? 1.0f : 0.0f;
			float d = SKILL_K * (S - Er);
			e->client->sess.skillRating += d;
			e->client->sess.skillRatingChange = static_cast<int>(d);
			ClientConfig_SaveStats(e->client, redWin);
		}
		for (auto* e : blue) {
			float S = redWin ? 0.0f : 1.0f;
			float d = SKILL_K * (S - Eb);
			e->client->sess.skillRating += d;
			e->client->sess.skillRatingChange = static_cast<int>(d);
			ClientConfig_SaveStats(e->client, !redWin);
		}

		// Ghosts
		for (auto& g : level.ghosts) {
			if (!*g.socialID)
				continue;

			float S = (g.team == Team::Red) ? (redWin ? 1.0f : 0.0f)
				: (g.team == Team::Blue ? (redWin ? 0.0f : 1.0f) : 0.5f);
			float E = (g.team == Team::Red) ? Er : (g.team == Team::Blue ? Eb : 0.5f);
			float d = SKILL_K * (S - E);

			g.skillRating += d;
			g.skillRatingChange = static_cast<int>(d);
			ClientConfig_SaveStatsForGhost(g, (g.team == Team::Red) ? redWin : (g.team == Team::Blue ? !redWin : false));
		}
		return;
	}

	// === FFA MODE ===
	{
		int n = players.size();

		std::sort(players.begin(), players.end(),
			[](gentity_t* a, gentity_t* b) {
				return a->client->resp.score > b->client->resp.score;
			});

		std::vector<float> R(n), S(n), E(n, 0.0f);
		for (int i = 0; i < n; i++) {
			R[i] = players[i]->client->sess.skillRating;
			S[i] = 1.0f - float(i) / float(n - 1);
		}

		for (int i = 0; i < n; i++) {
			for (int j = 0; j < n; j++) {
				if (i == j) continue;
				E[i] += EloExpected(R[i], R[j]);
			}
			E[i] /= float(n - 1);
		}

		for (int i = 0; i < n; i++) {
			float delta = SKILL_K * (S[i] - E[i]);
			auto* cl = players[i]->client;
			cl->sess.skillRating += delta;
			cl->sess.skillRatingChange = static_cast<int>(delta);
			ClientConfig_SaveStats(cl, i == 0);
		}

		// Ghosts
		std::vector<Ghosts*> sortedGhosts;
		for (auto& g : level.ghosts) {
			if (*g.socialID)
				sortedGhosts.push_back(&g);
		}

		std::sort(sortedGhosts.begin(), sortedGhosts.end(), [](Ghosts* a, Ghosts* b) {
			return a->score > b->score;
			});

		int gn = sortedGhosts.size();
		if (gn > 0) {
			for (int i = 0; i < gn; i++) {
				float Ri = sortedGhosts[i]->skillRating;
				float Si = 1.0f - float(i) / float(gn - 1);
				float Ei = 0.0f;

				for (int j = 0; j < gn; j++) {
					if (i == j) continue;
					Ei += EloExpected(Ri, sortedGhosts[j]->skillRating);
				}
				Ei /= float(gn - 1);

				float delta = SKILL_K * (Si - Ei);
				sortedGhosts[i]->skillRating += delta;
				sortedGhosts[i]->skillRatingChange = static_cast<int>(delta);
				ClientConfig_SaveStatsForGhost(*sortedGhosts[i], i == 0);
			}
		}
	}
}

/*
=================
Match_End

An end of match condition has been reached
=================
*/
extern void MatchStats_End();
void Match_End() {
	MarathonResetState();

	gentity_t* ent;

	time_t now = GetCurrentRealTimeMillis();

	//for (auto ec : active_players())
	//	ec->client->sess.playEndRealTime = now;
	MatchStats_End();
	SetMapLastPlayedTime(level.mapName.data());

	Match_UpdateDuelRecords();

	level.matchState = MatchState::Ended;
	level.matchStateTimer = 0_sec;

	AdjustSkillRatings();

	// stay on same level flag
	if (match_map_sameLevel->integer) {
		BeginIntermission(CreateTargetChangeLevel(level.mapName.data()));
		return;
	}

	if (level.forceMap[0]) {
		BeginIntermission(CreateTargetChangeLevel(level.forceMap.data()));
		return;
	}

	// pull next map from MyMap queue, if present
	if (!game.mapSystem.playQueue.empty()) {
		const auto& queued = game.mapSystem.playQueue.front();

		//level.changeMap = queued.filename.c_str(); // optional but keeps consistency

		game.map.overrideEnableFlags = queued.settings.to_ulong();
		game.map.overrideDisableFlags = ~queued.settings.to_ulong(); // correctly complement enabled flags

		BeginIntermission(CreateTargetChangeLevel(queued.filename.c_str()));

		game.mapSystem.playQueue.erase(game.mapSystem.playQueue.begin()); // remove after intermission trigger
		return;
	}

	// auto-select from cycleable map pool
	if (auto next = AutoSelectNextMap(); next) {
		BeginIntermission(CreateTargetChangeLevel(next->filename.c_str()));
		return;
	}

	// see if it's in the map list
	if (game.mapSystem.mapPool.empty() && *match_maps_list->string) {
		const char* str = match_maps_list->string;
		char first_map[MAX_QPATH]{ 0 };
		char* map;

		while (1) {
			map = COM_ParseEx(&str, " ");

			if (!*map)
				break;

			if (Q_strcasecmp(map, level.mapName.data()) == 0) {
				// it's in the list, go to the next one
				map = COM_ParseEx(&str, " ");
				if (!*map) {
					// end of list, go to first one
					if (!first_map[0]) // there isn't a first one, same level
					{
						BeginIntermission(CreateTargetChangeLevel(level.mapName.data()));
						return;
					}
					else {
						// [Paril-KEX] re-shuffle if necessary
						if (match_maps_listShuffle->integer) {
							auto values = str_split(match_maps_list->string, ' ');

							if (values.size() == 1) {
								// meh
								BeginIntermission(CreateTargetChangeLevel(level.mapName.data()));
								return;
							}

							std::shuffle(values.begin(), values.end(), mt_rand);

							// if the current map is the map at the front, push it to the end
							std::string_view mapView(level.mapName.data(), strnlen(level.mapName.data(), level.mapName.size()));
							if (values[0] == mapView)
								std::swap(values[0], values[values.size() - 1]);

							gi.cvarForceSet("match_maps_list", fmt::format("{}", join_strings(values, " ")).data());

							BeginIntermission(CreateTargetChangeLevel(values[0].c_str()));
							return;
						}

						BeginIntermission(CreateTargetChangeLevel(first_map));
						return;
					}
				}
				else {
					BeginIntermission(CreateTargetChangeLevel(map));
					return;
				}
			}
			if (!first_map[0])
				Q_strlcpy(first_map, map, sizeof(first_map));
		}
	}

	if (level.nextMap[0]) { // go to a specific map
		BeginIntermission(CreateTargetChangeLevel(level.nextMap.data()));
		return;
	}

	// search for a changelevel
	ent = G_FindByString<&gentity_t::className>(nullptr, "target_changelevel");

	if (!ent) { // the map designer didn't include a changelevel,
		// so create a fake ent that goes back to the same level
		BeginIntermission(CreateTargetChangeLevel(level.mapName.data()));
		return;
	}

	BeginIntermission(ent);
}

/*
============
Match_Reset
============
*/
void Match_Reset() {
	MarathonResetState();

	ResetMatchWorldState(true);
	ResetMatchPlayers(true, true, LimitedLivesResetMode::Force);
	UnReadyAll();

	level.intermission.queued = 0_sec;
	level.intermission.postIntermission = false;
	level.intermission.time = 0_sec;
	level.intermission.duelWinLossApplied = false;
	memset(&level.match, 0, sizeof(level.match));

	if (!warmup_enabled->integer) {
		time_t now = GetCurrentRealTimeMillis();
		level.matchStartRealTime = now;
		level.matchEndRealTime = 0;
		level.levelStartTime = level.time;
		level.overtime = 0_sec;
		const char* s = TimeString(timeLimit->value ? timeLimit->value * 1000 : 0, false, true);
		gi.configString(CONFIG_MATCH_STATE, s);
		SetMatchState(LevelMatchTransition{
				MatchState::In_Progress,
				level.time,
				std::optional<WarmupState>{WarmupState::Default},
				std::optional<GameTime>{0_sec},
				std::optional<bool>{false}
		});
		CalculateRanks();
		gi.Broadcast_Print(PRINT_CENTER, ".The match has been reset.\n");
		return;
	}

	level.matchStartRealTime = GetCurrentRealTimeMillis();
	level.matchEndRealTime = 0;
	level.levelStartTime = level.time;
	// Transition: reset -> default warmup lobby before players ready up.
	SetMatchState(LevelMatchTransition{
			MatchState::Warmup_Default,
			0_sec,
			std::optional<WarmupState>{WarmupState::Default},
			std::optional<GameTime>{0_sec},
			std::optional<bool>{false}
	});

	CalculateRanks();

	gi.Broadcast_Print(PRINT_CENTER, ".The match has been reset.\n");
}
/*
============
CheckDMRoundState
============
*/
static void CheckDMRoundState() {
	if (!Game::Has(GameFlags::Rounds) || level.matchState != MatchState::In_Progress)
		return;

	if (level.roundState == RoundState::None || level.roundState == RoundState::Ended) {
		if (level.roundStateTimer > level.time)
			return;
		if (Game::Is(GameType::RedRover) && level.roundState == RoundState::Ended)
			Commands::TeamSkillShuffle();
		Round_StartNew();
		return;
	}

	if (level.roundState == RoundState::Countdown && level.time >= level.roundStateTimer) {
		for (auto ec : active_clients())
			ec->client->latchedButtons = BUTTON_NONE;
		level.roundState = RoundState::In_Progress;
		level.roundStateTimer = level.time + GameTime::from_min(roundTimeLimit->value);
		level.roundNumber++;
		gi.Broadcast_Print(PRINT_CENTER, ".FIGHT!\n");
		AnnouncerSound(world, "fight");

		if (Game::Is(GameType::CaptureStrike)) {
			CheckRoundStrikeStartTurn();
		}
		return;
	}

	if (level.roundState == RoundState::In_Progress) {
		GameType gt = static_cast<GameType>(g_gametype->integer);
		using enum GameType;
		switch (gt) {
		case ClanArena:     CheckRoundEliminationCA(); break;
		case CaptureStrike: CheckRoundEliminationCA(); break;
		case FreezeTag:     CheckRoundFreezeTag(); break;
		case Horde:        CheckRoundHorde(); break;
		case RedRover:     CheckRoundRR(); break;
		}

		if (level.time >= level.roundStateTimer) {
			switch (gt) {
			case ClanArena:     CheckRoundTimeLimitCA(); break;
			case CaptureStrike: CheckRoundStrikeTimeLimit(); break;
				// Additional GTs can be added here
			}
		}
	}
}

/*
=============
ReadyAll
=============
*/
void ReadyAll() {
	for (auto ec : active_clients()) {
		if (!ClientIsPlaying(ec->client))
			continue;
		ec->client->pers.readyStatus = true;
	}
}

/*
=============
UnReadyAll
=============
*/
void UnReadyAll() {
	for (auto ec : active_clients()) {
		if (!ClientIsPlaying(ec->client))
			continue;
		ec->client->pers.readyStatus = false;
	}
}

/*
=============
CheckReady
=============
*/
static bool CheckReady() {
	if (!warmup_doReadyUp->integer)
		return true;

	uint8_t count_ready, count_humans, count_bots;

	count_ready = count_humans = count_bots = 0;
	for (auto ec : active_clients()) {
		if (!ClientIsPlaying(ec->client))
			continue;
		if (ec->svFlags & SVF_BOT || ec->client->sess.is_a_bot) {
			count_bots++;
			continue;
		}

		if (ec->client->pers.readyStatus)
			count_ready++;
		count_humans++;
	}

	// wait if no players at all
	if (!count_humans && !count_bots)
		return true;

	// wait if below minimum players
	if (minplayers->integer > 0 && (count_humans + count_bots) < minplayers->integer)
		return false;

	// start if only bots
	if (!count_humans && count_bots && match_startNoHumans->integer)
		return true;

	// wait if no ready humans
	if (!count_ready)
		return false;

	// start if over min ready percentile
	if (((float)count_ready / (float)count_humans) * 100.0f >= g_warmup_ready_percentage->value * 100.0f)
		return true;

	return false;
}

/*
=============
AnnounceCountdown
=============
*/
void AnnounceCountdown(int t, GameTime& checkRef) {
	const GameTime nextCheck = GameTime::from_sec(t);
	if (!checkRef || checkRef > nextCheck) {
		static constexpr std::array<std::string_view, 3> labels = {
			"one", "two", "three"
		};
		if (t >= 1 && t <= static_cast<int>(labels.size())) {
			AnnouncerSound(world, labels[t - 1].data());
		}
		checkRef = nextCheck;
	}
}

/*
=============
CheckDMCountdown
=============
*/
static void CheckDMCountdown() {
	// bail out if we're not in a true countdown
	if ((level.matchState != MatchState::Countdown
		&& level.roundState != RoundState::Countdown)
		|| level.intermission.time) {
		level.countdownTimerCheck = 0_sec;
		return;
	}

	// choose the correct base timer
	GameTime base = (level.roundState == RoundState::Countdown)
		? level.roundStateTimer
		: level.matchStateTimer;

	int t = (base + 1_sec - level.time).seconds<int>();

	// DEBUG: print current countdown info
	if (g_verbose->integer) {
		gi.Com_PrintFmt("[Countdown] matchState={}, roundState={}, base={}, now={}, countdown={}\n",
			(int)level.matchState,
			(int)level.roundState,
			base.milliseconds(),
			level.time.milliseconds(),
			t
		);
	}

	AnnounceCountdown(t, level.countdownTimerCheck);
}

/*
=============
CheckDMMatchEndWarning
=============
*/
static void CheckDMMatchEndWarning(void) {
	if (Game::Has(GameFlags::Rounds))
		return;

	if (level.matchState != MatchState::In_Progress || !timeLimit->value) {
		if (level.matchEndWarnTimerCheck)
			level.matchEndWarnTimerCheck = 0_sec;
		return;
	}

	int t = (level.levelStartTime + GameTime::from_min(timeLimit->value) - level.time).seconds<int>();	// +1;

	if (!level.matchEndWarnTimerCheck || level.matchEndWarnTimerCheck.seconds<int>() > t) {
		if (t && (t == 30 || t == 20 || t <= 10)) {
			//AnnouncerSound(world, nullptr, G_Fmt("world/{}{}.wav", t, t >= 20 ? "sec" : "").data(), false);
			//gi.positionedSound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundIndex(G_Fmt("world/{}{}.wav", t, t >= 20 ? "sec" : "").data()), 1, ATTN_NONE, 0);
			if (t >= 10)
				gi.LocBroadcast_Print(PRINT_HIGH, "{} second warning!\n", t);
		}
		else if (t == 300 || t == 60) {
			AnnouncerSound(world, G_Fmt("{}_minute", t == 300 ? 5 : 1).data());
		}
		level.matchEndWarnTimerCheck = GameTime::from_sec(t);
	}
}

/*
=============
CheckDMWarmupState
=============
*/
static void CheckDMWarmupState() {
	const bool duel = Game::Has(GameFlags::OneVOne);
	const int min_players = duel ? 2 : minplayers->integer;

	// Handle no players
	if (!level.pop.num_playing_clients) {
		if (level.matchState != MatchState::None) {
			// Transition: all players left -> return to idle state.
			SetMatchState(LevelMatchTransition{
					MatchState::None,
					0_sec,
					std::optional<WarmupState>{WarmupState::Default},
					std::optional<GameTime>{0_sec},
					std::optional<bool>{false}
				});
		}

		// Pull in idle bots
		for (auto ec : active_clients())
			if (!ClientIsPlaying(ec->client) && (ec->client->sess.is_a_bot || ec->svFlags & SVF_BOT))
				SetTeam(ec, PickTeam(-1), false, false, false);
		return;
	}

	// Pull queued players (if needed) during 1v1
	if (Game::Has(GameFlags::OneVOne) && Versus_AddPlayer())
		return;

	// If warmup disabled and enough players, start match
	if (level.matchState < MatchState::Countdown &&
		!warmup_enabled->integer &&
		level.pop.num_playing_clients >= min_players) {
		Match_Start();
		return;
	}

	// Trigger initial delayed warmup on fresh map
	if (level.matchState == MatchState::None) {
		// Transition: idle -> initial warmup delay after map load.
		SetMatchState(LevelMatchTransition{
				MatchState::Initial_Delay,
				level.time + 5_sec,
				std::optional<WarmupState>{WarmupState::Default},
				std::optional<GameTime>{level.time},
				std::optional<bool>{false}
			});
		return;
	}

	// Wait for delayed warmup to trigger, then immediately promote into warmup
	if (level.matchState == MatchState::Initial_Delay) {
		const bool transitioned = MatchWarmup::PromoteInitialDelayToWarmup(
			level.matchState,
			level.matchStateTimer,
			level.time,
			level.warmupState,
			level.warmupNoticeTime,
			MatchState::Initial_Delay,
			MatchState::Warmup_Default,
			WarmupState::Default,
			0_sec);

		if (!transitioned)
			return;

		if (g_verbose->integer) {
			gi.Com_PrintFmt("Initial warmup delay expired; entering Warmup_Default with {} players.\n",
				level.pop.num_playing_clients);
		}
	}

	// Run spawning logic during warmup (e.g., Horde)
	if (level.matchState == MatchState::Warmup_Default ||
		level.matchState == MatchState::Warmup_ReadyUp) {
		Horde_RunSpawning();
	}

	// Check for imbalance or missing players
	const bool forceBalance = Teams() && g_teamplay_force_balance->integer;
	const bool teamsImbalanced = forceBalance &&
		std::abs(level.pop.num_playing_red - level.pop.num_playing_blue) > 1;
	const bool notEnoughPlayers =
		(Teams() && (level.pop.num_playing_red < 1 || level.pop.num_playing_blue < 1)) ||
		(duel && level.pop.num_playing_clients != 2) ||
		(!Teams() && !duel && level.pop.num_playing_clients < min_players) ||
		(!match_startNoHumans->integer && !level.pop.num_playing_human_clients);

	if (teamsImbalanced || notEnoughPlayers) {
		if (level.matchState <= MatchState::Countdown) {
			if (level.matchState == MatchState::Warmup_ReadyUp)
				UnReadyAll();

			if (level.matchState == MatchState::Countdown) {
				const char* reason = teamsImbalanced ? "teams are imbalanced" : "not enough players";
				gi.LocBroadcast_Print(PRINT_CENTER, ".Countdown cancelled: {}\n", reason);
			}

			if (level.matchState != MatchState::Warmup_Default) {
				// Transition: countdown cancelled -> communicate imbalance reason.
				SetMatchState(LevelMatchTransition{
						MatchState::Warmup_Default,
						0_sec,
						std::optional<WarmupState>{teamsImbalanced ? WarmupState::Teams_Imbalanced : WarmupState::Too_Few_Players},
						std::optional<GameTime>{level.time},
						std::optional<bool>{false}
					});
			}
		}
		return;
	}

	// If we're in default warmup and ready-up is required
	if (level.matchState == MatchState::Warmup_Default) {
		if (!warmup_enabled->integer && g_warmup_countdown->integer <= 0) {
			// Transition: warmup disabled but countdown allowed -> start countdown immediately.
			SetMatchState(LevelMatchTransition{
					MatchState::Countdown,
					0_sec
				});
		}
		else {
			// Transition to ready-up
			SetMatchState(LevelMatchTransition{
					MatchState::Warmup_ReadyUp,
					0_sec,
					std::optional<WarmupState>{WarmupState::Not_Ready},
					std::optional<GameTime>{level.time},
					std::optional<bool>{false}
				});

			if (!duel) {
				// Pull in bots
				for (auto ec : active_clients())
					if (!ClientIsPlaying(ec->client) && ec->client->sess.is_a_bot)
						SetTeam(ec, PickTeam(-1), false, false, false);
			}

			BroadcastReadyReminderMessage();
			return;
		}
	}

	// Cancel countdown if warmup settings changed
	if (level.matchState <= MatchState::Countdown &&
		g_warmup_countdown->modifiedCount != level.warmupModificationCount) {
		level.warmupModificationCount = g_warmup_countdown->modifiedCount;
		// Transition: configuration changed -> reset warmup messaging.
		SetMatchState(LevelMatchTransition{
				MatchState::Warmup_Default,
				0_sec,
				std::optional<WarmupState>{WarmupState::Default},
				std::optional<GameTime>{0_sec},
				std::optional<bool>{false}
			});
		return;
	}

	// Ready-up check
	if (level.matchState == MatchState::Warmup_ReadyUp) {
		if (!CheckReady())
			return;

		if (g_warmup_countdown->integer > 0) {
			// Transition: ready-up complete -> begin countdown.
			SetMatchState(LevelMatchTransition{
					MatchState::Countdown,
					level.time + GameTime::from_sec(g_warmup_countdown->integer),
					std::optional<WarmupState>{WarmupState::Default},
					std::optional<GameTime>{0_sec}
				});

			if ((duel || (level.pop.num_playing_clients == 2 && match_lock->integer)) &&
				game.clients[level.sortedClients[0]].pers.connected &&
				game.clients[level.sortedClients[1]].pers.connected) {
				gi.LocBroadcast_Print(PRINT_CENTER, "{} vs {}\nBegins in...",
					game.clients[level.sortedClients[0]].sess.netName,
					game.clients[level.sortedClients[1]].sess.netName);
			}
			else {
				gi.LocBroadcast_Print(PRINT_CENTER, "{}\nBegins in...", level.gametype_name.data());
			}

			if (!level.prepare_to_fight) {
				const char* sound = (Teams() && level.pop.num_playing_clients >= 4) ? "prepare_your_team" : "prepare_to_fight";
				AnnouncerSound(world, sound);
				level.prepare_to_fight = true;
			}
			return;
		}
		else {
			// No countdown, start immediately
			Match_Start();
			return;
		}
	}

	// Final check: countdown timer expired?
	if (level.matchState == MatchState::Countdown &&
		level.time.seconds() >= level.matchStateTimer.seconds()) {
		Match_Start();
	}
}

/*
================
CheckDMEndFrame
================
*/
void CheckDMEndFrame() {
	if (!deathmatch->integer)
		return;

	// see if it is time to do a match restart
	CheckDMWarmupState();     // Manages warmup -> countdown -> match start
	CheckDMCountdown();       // Handles audible/visual countdown
	CheckDMRoundState();      // Handles per-round progression
	Domination_RunFrame();    // Updates domination scoring during live play
	HeadHunters::RunFrame();  // Handles loose-head logic and scoring
	ProBall::RunFrame();      // Updates ProBall scoring and state
	CheckDMMatchEndWarning(); // Optional: match-ending warnings

	// see if it is time to end a deathmatch
	CheckDMExitRules();       // Handles intermission and map end

	if (g_verbose->integer) {
                static constexpr std::array<const char*, 7> MatchStateNames{ {
                        "None",
                        "Initial_Delay",
                        "Warmup_Default",
                        "Warmup_ReadyUp",
                        "Countdown",
                        "In_Progress",
                        "Ended"
                } };

                const char* stateName = (static_cast<size_t>(level.matchState) < MatchStateNames.size())
                        ? MatchStateNames[static_cast<size_t>(level.matchState)]
			: "UNKNOWN";

		gi.Com_PrintFmt("MatchState: {}, NumPlayers: {}\n", stateName, level.pop.num_playing_clients);
	}

}

/*
==================
CheckVote
==================
*/
void CheckVote(void) {
	if (!deathmatch->integer)
		return;

	// vote has passed, execute
	if (level.vote.executeTime) {
		if (level.time > level.vote.executeTime)
			Vote_Passed();
		return;
	}

	if (!level.vote.time)
		return;

	if (!level.vote.client)
		return;

	// give it a minimum duration
	if (level.time - level.vote.time < 1_sec)
		return;

	if (level.time - level.vote.time >= 30_sec) {
		gi.Broadcast_Print(PRINT_HIGH, "Vote timed out.\n");
		AnnouncerSound(world, "vote_failed");
	}
	else {
		int halfpoint = level.pop.num_voting_clients / 2;
		if (level.vote.countYes > halfpoint) {
			// execute the command, then remove the vote
			gi.Broadcast_Print(PRINT_HIGH, "Vote passed.\n");
			level.vote.executeTime = level.time + 3_sec;
			AnnouncerSound(world, "vote_passed");
		}
		else if (level.vote.countNo >= halfpoint) {
			// same behavior as a timeout
			gi.Broadcast_Print(PRINT_HIGH, "Vote failed.\n");
			AnnouncerSound(world, "vote_failed");
		}
		else {
			// still waiting for a majority
			return;
		}
	}

	level.vote.time = 0_sec;
}

/*
===========================
CheckDMIntermissionExit

The level will stay at intermission for a minimum of 5 seconds.
If all human players confirm readiness, the level exits immediately.
Otherwise, it waits up to 10 seconds after the first readiness.
===========================
*/
static void CheckDMIntermissionExit() {
	// if we're in post intermission, bail out
	if (level.intermission.postIntermission)
		return;

	// Never exit in less than five seconds unless already timed
	if (level.time < level.intermission.time + 5_sec && level.exitTime)
		return;

	int numReady = 0;
	int numNotReady = 0;
	int numHumans = 0;

	for (auto ec : active_clients()) {
		auto* cl = ec->client;

		if (!ClientIsPlaying(cl))
			continue;

		if (cl->sess.is_a_bot)
			continue;

		numHumans++;

		if (cl->readyToExit)
			numReady++;
		else
			numNotReady++;
	}

	// If humans are present
	if (numHumans > 0) {
		// If a vote is running or pending execution, defer exit
		if (level.vote.time || level.vote.executeTime) {
			numReady = 0;
			numNotReady = 1;
		}

		// No one wants to exit yet
		if (numReady == 0 && numNotReady > 0) {
			level.readyToExit = false;
			return;
		}

		// Everyone is ready
		if (numNotReady == 0) {
			//ExitLevel();
			level.intermission.postIntermission = true;
			return;
		}
	}

	// Start 10s timeout if someone is ready or there are no humans
	if ((numReady > 0 || numHumans == 0) && !level.readyToExit) {
		level.readyToExit = true;
		level.exitTime = level.time + 10_sec;
	}

	// If the timeout hasn't expired yet, wait
	if (level.time < level.exitTime)
		return;

	// Force exit
	//ExitLevel();
	level.intermission.postIntermission = true;
}

/*
=============
ScoreIsTied
=============
*/
static bool ScoreIsTied(void) {
	if (level.pop.num_playing_clients < 2)
		return false;

	if (Teams() && Game::IsNot(GameType::RedRover))
		return level.teamScores[static_cast<int>(Team::Red)] == level.teamScores[static_cast<int>(Team::Blue)];

	return game.clients[level.sortedClients[0]].resp.score == game.clients[level.sortedClients[1]].resp.score;
}


int GT_ScoreLimit() {
	if (Game::Is(GameType::Domination))
		return fragLimit->integer;
	if (Game::Has(GameFlags::Rounds))
		return roundLimit->integer;
	if (Game::Is(GameType::CaptureTheFlag))
		return captureLimit->integer;
	if (Game::Is(GameType::ProBall))
		return captureLimit->integer;
	if (Game::Is(GameType::HeadHunters))
		return fragLimit->integer;
	return fragLimit->integer;
}

const char* GT_ScoreLimitString() {
	if (Game::Is(GameType::Domination))
		return "point";
	if (Game::Is(GameType::CaptureTheFlag))
		return "capture";
	if (Game::Is(GameType::ProBall))
		return "goal";
	if (Game::Is(GameType::HeadHunters))
		return "head";
	if (Game::Has(GameFlags::Rounds))
		return "round";
	return "frag";
}

/*
=================
CheckDMExitRules

Evaluates end-of-match rules for deathmatch, including:
- Intermission flow
- Timelimit, score, mercy limit
- Player count
- Horde win/loss
=================
*/
void CheckDMExitRules() {
	constexpr auto GRACE_TIME = 200_ms;

	EndmatchGraceScope<GameTime> graceScope(level.endmatch_grace, 0_ms);

	if (level.intermission.time) {
		CheckDMIntermissionExit();
		return;
	}

	// --- No players for X minutes ---
	if (!level.pop.num_playing_clients && noPlayersTime->integer &&
		level.time > level.no_players_time + GameTime::from_min(noPlayersTime->integer)) {
		Match_End();
		return;
	}

	// --- Intermission was queued previously ---
	if (level.intermission.queued) {
		if (level.time - level.intermission.queued >= 1_sec) {
			level.intermission.queued = 0_ms;
			Match_End();
		}
		return;
	}

	if (level.matchState < MatchState::In_Progress)
		return;

	if (level.time - level.levelStartTime <= FRAME_TIME_MS)
		return;

	const bool teams = Teams() && Game::IsNot(GameType::RedRover);

	// --- HORDE mode defeat ---
	if (Game::Is(GameType::Horde)) {
		if ((level.campaign.totalMonsters - level.campaign.killedMonsters) >= 100) {
			gi.Broadcast_Print(PRINT_CENTER, "DEFEATED!");
			QueueIntermission("OVERRUN BY MONSTERS!", true, false);
			return;
		}
	}

	// --- Rounds: wait for round to end ---
	if (Game::Has(GameFlags::Rounds) && level.roundState != RoundState::Ended)
		return;

	// --- HORDE round limit victory ---
	if (Game::Is(GameType::Horde) && roundLimit->integer > 0 && level.roundNumber >= roundLimit->integer) {
		auto& winner = game.clients[level.sortedClients[0]];
		QueueIntermission(G_Fmt("{} WINS with a final score of {}.", winner.sess.netName, winner.resp.score).data(), false, false);
		return;
	}

	// --- No human players remaining ---
	if (!match_startNoHumans->integer && !level.pop.num_playing_human_clients) {
		graceScope.MarkConditionActive();
		if (!level.endmatch_grace) {
			level.endmatch_grace = level.time;
			return;
		}
		if (level.time > level.endmatch_grace + GRACE_TIME) {
			QueueIntermission("No human players remaining.", true, false);
		}
		return;
	}

	// --- Not enough players for match ---
	if (minplayers->integer > 0 && level.pop.num_playing_clients < minplayers->integer) {
		graceScope.MarkConditionActive();
		if (!level.endmatch_grace) {
			level.endmatch_grace = level.time;
			return;
		}
		if (level.time > level.endmatch_grace + GRACE_TIME) {
			QueueIntermission("Not enough players remaining.", true, false);
		}
		return;
	}

	// --- Team imbalance enforcement ---
	if (teams && g_teamplay_force_balance->integer) {
		int diff = abs(level.pop.num_playing_red - level.pop.num_playing_blue);
		if (diff > 1) {
			graceScope.MarkConditionActive();
			if (g_teamplay_auto_balance->integer) {
				TeamBalance(true);
			}
			else {
				if (!level.endmatch_grace) {
					level.endmatch_grace = level.time;
					return;
				}
				if (level.time > level.endmatch_grace + GRACE_TIME) {
					QueueIntermission("Teams are imbalanced.", true, true);
				}
			}
			return;
		}
	}
#if 0
	if (Game::Is(GameType::FreeForAll) && level.sortedClients[0] >= 0) {
		gi.Com_Print("== CheckDMExitRules (FFA) ==\n");
		gi.Com_PrintFmt("  matchState: {}\n", static_cast<int>(level.matchState));
		gi.Com_PrintFmt("  levelStartTime: {}  level.time: {}\n", level.levelStartTime.milliseconds(), level.time.milliseconds());
		gi.Com_PrintFmt("  timeLimit: {}  scorelimit: {}\n", timeLimit->value, GT_ScoreLimit());
		gi.Com_PrintFmt("  top player clientnum: {}\n", level.sortedClients[0]);
		gi.Com_PrintFmt("  top player score: {}\n", game.clients[level.sortedClients[0]].resp.score);
	}
#endif
	// --- Timelimit ---
	if (timeLimit->value) {
		bool isRoundOver = !Game::Has(GameFlags::Rounds) || level.roundState == RoundState::Ended;
		if (isRoundOver && level.time >= level.levelStartTime + GameTime::from_min(timeLimit->value) + level.overtime) {
			if (ScoreIsTied()) {
				if (Game::Has(GameFlags::OneVOne) && match_doOvertime->integer > 0) {
					level.overtime += GameTime::from_sec(match_doOvertime->integer);
					gi.LocBroadcast_Print(PRINT_CENTER, "Overtime!\n{} added", TimeString(match_doOvertime->integer * 1000, false, false));
					AnnouncerSound(world, "overtime");
				}
				else if (!level.suddenDeath) {
					level.suddenDeath = true;
					gi.Broadcast_Print(PRINT_CENTER, "Sudden Death!");
					AnnouncerSound(world, "sudden_death");
				}
				return;
			}

			// Determine winner
			if (teams) {
				int red = level.teamScores[static_cast<int>(Team::Red)];
				int blue = level.teamScores[static_cast<int>(Team::Blue)];

				if (red != blue) {
					Team winner = (red > blue) ? Team::Red : Team::Blue;
					Team loser = (red < blue) ? Team::Red : Team::Blue;
					QueueIntermission(G_Fmt("{} Team WINS with a final score of {} to {}.\n", Teams_TeamName(winner), level.teamScores[static_cast<int>(winner)], level.teamScores[static_cast<int>(loser)]).data(), false, false);
					return;
				}
			}
			else {
				const int winnerIndex = level.sortedClients[0];
				if (winnerIndex < 0) {
					QueueIntermission("Timelimit hit.", false, false);
					return;
				}

				auto& winner = game.clients[winnerIndex];
				QueueIntermission(G_Fmt("{} WINS with a final score of {}.", winner.sess.netName, winner.resp.score).data(), false, false);
				return;
			}

			QueueIntermission("Timelimit hit.", false, false);
			return;
		}
	}

	// --- Mercylimit ---
	if (mercyLimit->integer > 0) {
		if (teams) {
			if (abs(level.teamScores[static_cast<int>(Team::Red)] - level.teamScores[static_cast<int>(Team::Blue)]) >= mercyLimit->integer) {
				Team leader = level.teamScores[static_cast<int>(Team::Red)] > level.teamScores[static_cast<int>(Team::Blue)] ? Team::Red : Team::Blue;
				QueueIntermission(G_Fmt("{} hit the mercy limit ({}).", Teams_TeamName(leader), mercyLimit->integer).data(), true, false);
				return;
			}
		}
		else if (Game::IsNot(GameType::Horde)) {
			const int leaderIndex = level.sortedClients[0];
			const int runnerUpIndex = level.sortedClients[1];

			if (leaderIndex < 0 || runnerUpIndex < 0)
				return;

			auto& cl1 = game.clients[leaderIndex];
			auto& cl2 = game.clients[runnerUpIndex];
			if (cl1.resp.score >= cl2.resp.score + mercyLimit->integer) {
				QueueIntermission(G_Fmt("{} hit the mercy limit ({}).", cl1.sess.netName, mercyLimit->integer).data(), true, false);
				return;
			}
		}
	}

	// --- Final score check (not Horde) ---
	if (Game::Is(GameType::Horde))
		return;

	if (Game::Is(GameType::LastManStanding) || Game::Is(GameType::LastTeamStanding)) {
		if (Game::Is(GameType::LastTeamStanding)) {
			std::array<int, static_cast<size_t>(Team::Total)> teamPlayers{};
			std::array<int, static_cast<size_t>(Team::Total)> teamLives{};

			for (auto ec : active_clients()) {
				if (!ClientIsPlaying(ec->client))
					continue;

				const Team team = ec->client->sess.team;
				if (team != Team::Red && team != Team::Blue)
					continue;

				const auto teamIndex = static_cast<size_t>(team);
				teamPlayers[teamIndex]++;

				if (ec->client->pers.lives > 0)
					teamLives[teamIndex] += ec->client->pers.lives;
			}

			int participatingTeams = 0;
			int teamsWithLives = 0;
			Team potentialWinner = Team::None;

			for (Team team : { Team::Red, Team::Blue }) {
				const auto teamIndex = static_cast<size_t>(team);
				if (teamPlayers[teamIndex] == 0)
					continue;

				participatingTeams++;

				if (teamLives[teamIndex] > 0) {
					teamsWithLives++;
					potentialWinner = team;
				}
			}

			if (participatingTeams > 1 && teamsWithLives <= 1) {
				if (teamsWithLives == 1 && potentialWinner != Team::None) {
					QueueIntermission(G_Fmt("{} Team WINS! (last surviving team)", Teams_TeamName(potentialWinner)).data(), false, false);
				}
				else {
					QueueIntermission("All teams eliminated!", true, false);
				}
				return;
			}
		}
		else {
			int playingClients = 0;
			int playersWithLives = 0;
			gentity_t* potentialWinner = nullptr;

			for (auto ec : active_clients()) {
				if (!ClientIsPlaying(ec->client))
					continue;
				if (ec->client->sess.team != Team::Free)
					continue;

				playingClients++;

				if (ec->client->pers.lives > 0) {
					playersWithLives++;
					potentialWinner = ec;
				}
			}

			if (playingClients > 1 && playersWithLives <= 1) {
				if (playersWithLives == 1 && potentialWinner) {
					QueueIntermission(G_Fmt("{} WINS! (last survivor)", potentialWinner->client->sess.netName).data(), false, false);
				}
				else {
					QueueIntermission("All players eliminated!", true, false);
				}
				return;
			}
		}
	}

	if (ScoreIsTied())
		return;

	int scoreLimit = GT_ScoreLimit();
	if (scoreLimit <= 0)
		return;

	if (teams) {
		for (Team team : { Team::Red, Team::Blue }) {
			if (level.teamScores[static_cast<int>(team)] >= scoreLimit) {
				QueueIntermission(G_Fmt("{} WINS! (hit the {} limit)", Teams_TeamName(team), GT_ScoreLimitString()).data(), false, false);
				return;
			}
		}
	}
	else {
		for (auto ec : active_clients()) {
			if (ec->client->sess.team != Team::Free)
				continue;

			if (ec->client->resp.score >= scoreLimit) {
				QueueIntermission(G_Fmt("{} WINS! (hit the {} limit)", ec->client->sess.netName, GT_ScoreLimitString()).data(), false, false);
				return;
			}
		}
	}

	if (game.marathon.active) {
		std::string marathonMessage;
		if (MarathonCheckTimeLimit(marathonMessage) || MarathonCheckScoreLimit(marathonMessage)) {
			if (marathonMessage.empty())
				marathonMessage = "Marathon: Advancing to the next map.";
			MarathonTriggerAdvance(marathonMessage);
			return;
		}
	}
}

static bool Match_NextMap() {
	if (level.matchState == MatchState::Ended) {
		level.matchState = MatchState::Initial_Delay;
		level.warmupNoticeTime = level.time;
		Match_Reset();
		return true;
	}
	return false;
}

void GT_Init() {
	constexpr const char* COOP = "coop";
	bool force_dm = false;

	deathmatch = gi.cvar("deathmatch", "1", CVAR_LATCH);
	teamplay = gi.cvar("teamplay", "0", CVAR_SERVERINFO);
	ctf = gi.cvar("ctf", "0", CVAR_SERVERINFO);
	g_gametype = gi.cvar("g_gametype", G_Fmt("{}", (int)GameType::FreeForAll).data(), CVAR_SERVERINFO);
	coop = gi.cvar("coop", "0", CVAR_LATCH);

	// game modifications
	g_instaGib = gi.cvar("g_instaGib", "0", CVAR_SERVERINFO | CVAR_LATCH);
	g_instagib_splash = gi.cvar("g_instagib_splash", "0", CVAR_NOFLAGS);
	g_owner_auto_join = gi.cvar("g_owner_auto_join", "0", CVAR_NOFLAGS);
	g_owner_push_scores = gi.cvar("g_owner_push_scores", "1", CVAR_NOFLAGS);
	g_quadhog = gi.cvar("g_quadhog", "0", CVAR_SERVERINFO | CVAR_LATCH);
	g_nadeFest = gi.cvar("g_nadeFest", "0", CVAR_SERVERINFO | CVAR_LATCH);
	g_frenzy = gi.cvar("g_frenzy", "0", CVAR_SERVERINFO | CVAR_LATCH);
	g_vampiric_damage = gi.cvar("g_vampiric_damage", "0", CVAR_NOFLAGS);
	g_vampiric_exp_min = gi.cvar("g_vampiric_exp_min", "0", CVAR_NOFLAGS);
	g_vampiric_health_max = gi.cvar("g_vampiric_health_max", "9999", CVAR_NOFLAGS);
	g_vampiric_percentile = gi.cvar("g_vampiric_percentile", "0.67f", CVAR_NOFLAGS);

	if (!Game::IsCurrentTypeValid())
		gi.cvarForceSet("g_gametype", G_Fmt("{}", static_cast<int>(Game::NormalizeTypeValue(g_gametype->integer))).data());

	if (ctf->integer) {
		force_dm = true;
		// force coop off
		if (coop->integer)
			gi.cvarSet(COOP, "0");
		// force tdm off
		if (teamplay->integer)
			gi.cvarSet("teamplay", "0");
	}
	if (teamplay->integer) {
		force_dm = true;
		// force coop off
		if (coop->integer)
			gi.cvarSet(COOP, "0");
	}

	if (force_dm && !deathmatch->integer) {
		gi.Com_Print("Forcing deathmatch.\n");
		gi.cvarForceSet("deathmatch", "1");
	}

	// force even maxPlayers value during teamplay
	if (Teams()) {
		int pmax = maxplayers->integer;

		if (pmax != floor(pmax / 2))
			gi.cvarSet("maxPlayers", G_Fmt("{}", floor(pmax / 2) * 2).data());
	}

	GT_SetLongName();
}

void ChangeGametype(GameType gt) {
	switch (gt) {
	case GameType::CaptureTheFlag:
		if (!ctf->integer)
			gi.cvarForceSet("ctf", "1");
		break;
	case GameType::TeamDeathmatch:
		if (!teamplay->integer)
			gi.cvarForceSet("teamplay", "1");
		break;
	case GameType::Domination:
		if (!teamplay->integer)
			gi.cvarForceSet("teamplay", "1");
		if (ctf->integer)
			gi.cvarForceSet("ctf", "0");
		break;
	default:
		if (ctf->integer)
			gi.cvarForceSet("ctf", "0");
		if (teamplay->integer)
			gi.cvarForceSet("teamplay", "0");
		break;
	}

	if (!deathmatch->integer) {
		gi.Com_Print("Forcing deathmatch.\n");
		gi.cvarForceSet("deathmatch", "1");
	}

	if ((int)gt != g_gametype->integer)
		gi.cvarForceSet("g_gametype", G_Fmt("{}", (int)gt).data());
}
