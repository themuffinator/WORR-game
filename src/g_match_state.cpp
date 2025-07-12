
#include "g_local.h"
//#include "monsters/m_player.h"	// match starts for death anim

/*
=================
str_split
=================
*/
static inline std::vector<std::string> str_split(const std::string_view &str, char by) {
	std::vector<std::string> out;
	size_t start, end = 0;

	while ((start = str.find_first_not_of(by, end)) != std::string_view::npos) {
		end = str.find(by, start);
		out.push_back(std::string{ str.substr(start, end - start) });
	}

	return out;
}

constexpr struct gt_rules_t {
	int						flags = GTF_NONE;
	uint8_t					weaponRespawnDelay = 8;	// in seconds. if 0, weapon stay is on
	bool					holdables = true;			// can hold items such as adrenaline and personal teleporter
	bool					powerupsEnabled = true;	// yes to powerups?
	uint8_t					scoreLimit = 40;
	uint8_t					timeLimit = 10;
	bool					startingHealthBonus = true;
	float					readyUpPercentile = 0.51f;
} gt_rules_t[GT_NUM_GAMETYPES] = {
	/* GT_FFA */ {GTF_FRAGS},
	/* GT_DUEL */ {GTF_FRAGS, 30, false, false, 0},
	/* GT_TDM */ {GTF_TEAMS | GTF_FRAGS, 30, true, true, 100, 20},
	/* GT_CTF */ {GTF_TEAMS | GTF_CTF, 30},
	/* GT_CA */ { },
	/* GT_ONEFLAG */ { },
	/* GT_HARVESTER */ { },
	/* GT_OVERLOAD */ { },
	/* GT_FREEZE */ { },
	/* GT_STRIKE */ { },
	/* GT_RR */ { },
	/* GT_LMS */ { },
	/* GT_HORDE */ { },
	/* GT_BALL */ { },
	/* GT_GAUNTLET */ { }
};

static void Monsters_KillAll() {
	for (size_t i = 0; i < globals.max_entities; i++) {
		if (!g_entities[i].inUse)
			continue;
		if (!(g_entities[i].svFlags & SVF_MONSTER))
			continue;
		FreeEntity(&g_entities[i]);
	}
	level.totalMonsters = 0;
	level.killedMonsters = 0;
}

static void Entities_ItemTeams_Reset() {
	gentity_t	*ent;
	size_t		i;

	gentity_t	*master;
	int			count, choice;

	for (ent = g_entities + 1, i = 1; i < globals.num_entities; i++, ent++) {
		if (!ent->inUse)
			continue;

		if (!ent->item)
			continue;

		if (!ent->team)
			continue;

		if (!ent->teamMaster)
			continue;

		master = ent->teamMaster;

		ent->svFlags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
		gi.linkentity(ent);

		for (count = 0, ent = master; ent; ent = ent->chain, count++)
			;

		choice = irandom(count);
		for (count = 0, ent = master; count < choice; ent = ent->chain, count++)
			;
	}
}

/*
============
Entities_Reset

Reset clients and items
============
*/
static void Entities_Reset(bool reset_players, bool reset_ghost, bool reset_score) {
	gentity_t *ent;
	size_t	i;

	// reset the players
	if (reset_players) {
		for (auto ec : active_clients()) {
			ec->client->resp.ctf_state = 0;
			if (reset_score)
				ec->client->resp.score = 0;
			if (reset_ghost) {

			}
			if (ClientIsPlaying(ec->client)) {
				if (reset_ghost) {

				}
				Weapon_Grapple_DoReset(ec->client);
				ec->client->eliminated = false;
				ec->client->pers.readyStatus = false;
				ec->moveType = MOVETYPE_NOCLIP;
				ec->client->respawnMaxTime = level.time + FRAME_TIME_MS;
				ClientSpawn(ec);
				memset(&ec->client->pers.match, 0, sizeof(ec->client->pers.match));

				gi.linkentity(ec);
			}
		}

		CalculateRanks();
	}

	// reset the level items
	Tech_Reset();
	CTF_ResetFlags();

	Monsters_KillAll();

	Entities_ItemTeams_Reset();

	// reset item spawns and gibs/corpses, remove dropped items and projectiles
	for (ent = g_entities + 1, i = 1; i < globals.num_entities; i++, ent++) {
		if (!ent->inUse)
			continue;

		if (Q_strcasecmp(ent->className, "bodyque") == 0 || Q_strcasecmp(ent->className, "gib") == 0) {
			ent->svFlags = SVF_NOCLIENT;
			ent->takeDamage = false;
			ent->solid = SOLID_NOT;
			gi.unlinkentity(ent);
			FreeEntity(ent);
		} else if ((ent->svFlags & SVF_PROJECTILE) || (ent->clipMask & CONTENTS_PROJECTILECLIP)) {
			FreeEntity(ent);
		} else if (ent->item) {
			// already processed in CTF_ResetFlags()
			if (ent->item->id == IT_FLAG_RED || ent->item->id == IT_FLAG_BLUE)
				continue;

			if (ent->spawnflags.has(SPAWNFLAG_ITEM_DROPPED | SPAWNFLAG_ITEM_DROPPED_PLAYER)) {
				//FreeEntity(ent);
				ent->nextThink = level.time;
			} else {
				// powerups don't spawn in for a while
				if (ent->item->flags & IF_POWERUP) {
					if (g_quadhog->integer && ent->item->id == IT_POWERUP_QUAD) {
						FreeEntity(ent);
						QuadHog_SetupSpawn(5_sec);
					} else {
						ent->svFlags |= SVF_NOCLIENT;
						ent->solid = SOLID_NOT;

						ent->nextThink = level.time + gtime_t::from_sec(irandom(30, 60));
						//if (!ent->think)
						ent->think = RespawnItem;
					}
					continue;
				} else {
					if (ent->svFlags & (SVF_NOCLIENT | SVF_RESPAWNING) || ent->solid == SOLID_NOT) {
						gtime_t t = 0_sec;
						if (ent->random) {
							t += gtime_t::from_ms((crandom() * ent->random) * 1000);
							if (t < FRAME_TIME_MS) {
								t = FRAME_TIME_MS;
							}
						}
						//if (ent->item->id == IT_HEALTH_MEGA)
						ent->think = RespawnItem;
						ent->nextThink = level.time + t;
					}
				}
			}
		}
	}
}

// =================================================

static void RoundAnnounceWin(team_t team, const char *reason) {
	G_AdjustTeamScore(team, 1);
	gi.LocBroadcast_Print(PRINT_CENTER, "{} wins the round!\n({})\n", Teams_TeamName(team), reason);
	AnnouncerSound(world, team == TEAM_RED ? "red_wins_round" : "blue_wins_round");
}

static void RoundAnnounceDraw() {
	gi.Broadcast_Print(PRINT_CENTER, "Round draw!\n");
	AnnouncerSound(world, "round_draw");
}

static void CheckRoundEliminationCA() {
	int redAlive = 0, blueAlive = 0;
	for (auto ec : active_players()) {
		if (ec->health <= 0)
			continue;
		switch (ec->client->sess.team) {
		case TEAM_RED: redAlive++; break;
		case TEAM_BLUE: blueAlive++; break;
		}
	}

	if (redAlive && !blueAlive) {
		RoundAnnounceWin(TEAM_RED, "eliminated blue team");
		Round_End();
	} else if (blueAlive && !redAlive) {
		RoundAnnounceWin(TEAM_BLUE, "eliminated red team");
		Round_End();
	}
}

static void CheckRoundTimeLimitCA() {
	if (level.pop.num_living_red > level.pop.num_living_blue) {
		RoundAnnounceWin(TEAM_RED, "players remaining");
	} else if (level.pop.num_living_blue > level.pop.num_living_red) {
		RoundAnnounceWin(TEAM_BLUE, "players remaining");
	} else {
		int healthRed = 0, healthBlue = 0;
		for (auto ec : active_players()) {
			if (ec->health <= 0) continue;
			switch (ec->client->sess.team) {
			case TEAM_RED: healthRed += ec->health; break;
			case TEAM_BLUE: healthBlue += ec->health; break;
			}
		}
		if (healthRed > healthBlue) {
			RoundAnnounceWin(TEAM_RED, "total health");
		} else if (healthBlue > healthRed) {
			RoundAnnounceWin(TEAM_BLUE, "total health");
		} else {
			RoundAnnounceDraw();
		}
	}
	Round_End();
}

static void CheckRoundHorde() {
	Horde_RunSpawning();
	if (level.horde_all_spawned && !(level.totalMonsters - level.killedMonsters)) {
		gi.Broadcast_Print(PRINT_CENTER, "Monsters eliminated!\n");
		gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("ctf/flagcap.wav"), 1, ATTN_NONE, 0);
		Round_End();
	}
}

static void CheckRoundRR() {
	if (!level.pop.num_playing_red || !level.pop.num_playing_blue) {
		gi.Broadcast_Print(PRINT_CENTER, "Round Ends!\n");
		gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("ctf/flagcap.wav"), 1, ATTN_NONE, 0);
		if (level.roundNumber + 1 >= roundlimit->integer) {
			QueueIntermission("MATCH ENDED", false, false);
		} else {
			Round_End();
		}
	}
}

static void CheckRoundStrikeTimeLimit() {
	if (level.strike_flag_touch) {
		RoundAnnounceWin(level.strike_red_attacks ? TEAM_RED : TEAM_BLUE, "scored a point");
	} else {
		gi.LocBroadcast_Print(PRINT_CENTER, "Turn has ended.\n{} successfully defended!", Teams_TeamName(!level.strike_red_attacks ? TEAM_RED : TEAM_BLUE));
	}
	Round_End();
}

static void CheckRoundStrikeStartTurn() {
	if (!level.strike_turn_red && level.strike_red_attacks) {
		level.strike_turn_red = true;
	} else if (!level.strike_turn_blue && !level.strike_red_attacks) {
		level.strike_turn_blue = true;
	} else {
		level.strike_turn_red = level.strike_red_attacks;
		level.strike_turn_blue = !level.strike_red_attacks;
	}
}

static gclient_t *GetNextQueuedPlayer() {
	gclient_t *next = nullptr;
	for (auto ec : active_clients()) {
		if (ec->client->sess.matchQueued && !ClientIsPlaying(ec->client)) {
			if (!next || ec->client->sess.teamJoinTime < next->sess.teamJoinTime)
				next = ec->client;
		}
	}
	return next;
}

static bool Versus_AddPlayer() {
	if (GTF(GTF_1V1) && level.pop.num_playing_clients >= 2)
		return false;
	if (level.matchState > MatchState::MATCH_WARMUP_DEFAULT || level.intermissionTime || level.intermissionQueued)
		return false;

	gclient_t *next = GetNextQueuedPlayer();
	if (!next)
		return false;

	SetTeam(&g_entities[next - game.clients + 1], TEAM_FREE, false, true, false);

	return true;
}

void Gauntlet_RemoveLoser() {
	if (notGT(GT_GAUNTLET) || level.pop.num_playing_clients != 2)
		return;

	gentity_t *loser = &g_entities[level.sorted_clients[1] + 1];
	if (!loser || !loser->client || !loser->client->pers.connected)
		return;
	if (loser->client->sess.team != TEAM_FREE)
		return;

	if (g_verbose->integer)
		gi.Com_PrintFmt("Gauntlet: Moving the loser, {} to end of queue.\n", loser->client->sess.netName);

	SetTeam(loser, TEAM_NONE, false, true, false);
}

void Gauntlet_MatchEnd_AdjustScores() {
	if (notGT(GT_GAUNTLET))
		return;
	if (level.pop.num_playing_clients < 2)
		return;

	int winnerNum = level.sorted_clients[0];
	if (game.clients[winnerNum].pers.connected) {
		game.clients[winnerNum].sess.matchWins++;
	}
}

static void EnforceDuelRules() {
	if (notGT(GT_DUEL))
		return;

	if (level.pop.num_playing_clients > 2) {
		// Kick or move spectators if too many players
		for (auto ec : active_clients()) {
			if (ClientIsPlaying(ec->client)) {
				// Allow the first two
				continue;
			}
			if (ec->client->sess.team != TEAM_SPECTATOR) {
				SetTeam(ec, TEAM_SPECTATOR, false, true, false);
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
	if (notGTF(GTF_ROUNDS)) {
		level.roundState = RoundState::ROUND_NONE;
		level.roundStateTimer = 0_sec;
		return false;
	}

	bool horde = GT(GT_HORDE);

	level.roundState = RoundState::ROUND_COUNTDOWN;
	level.roundStateTimer = level.time + 10_sec;
	level.countdownTimerCheck = 0_sec;

	if (!horde)
		Entities_Reset(!horde, false, false);

	if (GT(GT_STRIKE)) {
		level.strike_red_attacks ^= true;
		level.strike_flag_touch = false;

		int round_num;
		if (level.roundNumber && (!level.strike_turn_red && level.strike_turn_blue ||
			level.strike_turn_red && !level.strike_turn_blue))
			round_num = level.roundNumber;
		else {
			round_num = level.roundNumber + 1;
		}
		BroadcastTeamMessage(TEAM_RED, PRINT_CENTER, G_Fmt("Your team is on {}!\nRound {} - Begins in...", level.strike_red_attacks ? "OFFENSE" : "DEFENSE", round_num).data());
		BroadcastTeamMessage(TEAM_BLUE, PRINT_CENTER, G_Fmt("Your team is on {}!\nRound {} - Begins in...", !level.strike_red_attacks ? "OFFENSE" : "DEFENSE", round_num).data());
	} else {
		int round_num;

		if (horde && !level.roundNumber && g_horde_starting_wave->integer > 0)
			round_num = g_horde_starting_wave->integer;
		else
			round_num = level.roundNumber + 1;

		if (GT(GT_RR) && roundlimit->integer) {
			gi.LocBroadcast_Print(PRINT_CENTER, "{} {} of {}\nBegins in...", horde ? "Wave" : "Round", round_num, roundlimit->integer);
		} else
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
	if (notGTF(GTF_ROUNDS)) {
		level.roundState = RoundState::ROUND_NONE;
		level.roundStateTimer = 0_sec;
		return;
	}

	// there must be a round to end
	if (level.roundState != RoundState::ROUND_IN_PROGRESS)
		return;

	level.roundState = RoundState::ROUND_ENDED;
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

	time_t now = GetCurrentRealTimeMillis();

	level.matchStartRealTime = now;
	level.matchEndRealTime = 0;
	level.levelStartTime = level.time;
	level.overtime = 0_sec;

	const char *s = TimeString(timelimit->value ? timelimit->value * 1000 : 0, false, true);
	gi.configstring(CONFIG_MATCH_STATE, s);

	level.matchState = MatchState::MATCH_IN_PROGRESS;
	level.matchStateTimer = level.time;
	level.warmupState = WarmupState::WARMUP_REQ_NONE;
	level.warmupNoticeTime = 0_sec;

	level.teamScores[TEAM_RED] = level.teamScores[TEAM_BLUE] = 0;

	level.match = {};

	Monsters_KillAll();
	Entities_Reset(true, true, true);
	UnReadyAll();

	for (auto ec : active_players())
		ec->client->sess.playStartRealTime = now;

	MatchStats_Init();

	if (GT(GT_STRIKE))
		level.strike_red_attacks = brandom();

	if (Round_StartNew())
		return;

	gi.LocBroadcast_Print(PRINT_CENTER, ".FIGHT!");
	//gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("misc/tele_up.wav"), 1, ATTN_NONE, 0);
	AnnouncerSound(world, "fight");
}

/*
===============================
SetMapLastPlayedTime
===============================
*/
static void SetMapLastPlayedTime(const char *mapname) {
	if (!mapname || !*mapname || game.serverStartTime == 0)
		return;

	time_t now = time(nullptr);
	int secondsSinceStart = static_cast<int>(now - game.serverStartTime);

	for (auto &map : game.mapSystem.mapPool) {
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
static std::vector<gentity_t *> GetPlayers() {
	std::vector<gentity_t *> out;
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
static bool DidPlayerWin(gentity_t *ent) {
	if (GT(GT_DUEL)) {
		auto players = GetPlayers();
		if (players.size() == 2)
			return (ent->client->resp.score > players[0]->client->resp.score || ent == players[0]);
	}

	if (GT(GT_TDM) || GT(GT_CTF)) {
		int redScore = 0, blueScore = 0;
		for (auto *e : GetPlayers()) {
			if (e->client->sess.team == TEAM_RED) redScore += e->client->resp.score;
			else if (e->client->sess.team == TEAM_BLUE) blueScore += e->client->resp.score;
		}
		if (ent->client->sess.team == TEAM_RED)
			return redScore > blueScore;
		else if (ent->client->sess.team == TEAM_BLUE)
			return blueScore > redScore;
	}

	// FFA
	auto players = GetPlayers();
	if (!players.empty())
		std::sort(players.begin(), players.end(),
			[](gentity_t *a, gentity_t *b) {
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
	// Sync sess.skillRating with config.skillRating
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		gentity_t *ent = &g_entities[i];
		if (!ent->inUse || !ent->client)
			continue;

		gclient_t *cl = ent->client;
		if (cl->sess.skillRating != cl->sess.skillRating)
			cl->sess.skillRating = cl->sess.skillRating;
	}

	auto players = GetPlayers();
	if (players.empty())
		return;

	// === DUEL MODE ===
	if (GT(GT_DUEL) && players.size() == 2) {
		auto *a = players[0], *b = players[1];
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
		for (auto &g : level.ghosts) {
			if (!*g.socialID)
				continue;
			if (Q_strcasecmp(g.socialID, a->client->sess.socialID) == 0) {
				g.skillRating += dA;
				g.skillRatingChange = static_cast<int>(dA);
				ClientConfig_SaveStatsForGhost(g, aWon);
			} else if (Q_strcasecmp(g.socialID, b->client->sess.socialID) == 0) {
				g.skillRating += dB;
				g.skillRatingChange = static_cast<int>(dB);
				ClientConfig_SaveStatsForGhost(g, !aWon);
			}
		}
		return;
	}

	// === TEAM MODE ===
	if ((GT(GT_TDM) || GT(GT_CTF)) && players.size() >= 2) {
		std::vector<gentity_t *> red, blue;
		for (auto *ent : players) {
			if (ent->client->sess.team == TEAM_RED)
				red.push_back(ent);
			else if (ent->client->sess.team == TEAM_BLUE)
				blue.push_back(ent);
		}
		if (red.empty() || blue.empty())
			return;

		auto avg = [](const std::vector<gentity_t *> &v) {
			float sum = 0;
			for (auto *e : v)
				sum += e->client->sess.skillRating;
			return sum / v.size();
			};

		float Rr = avg(red), Rb = avg(blue);
		float Er = EloExpected(Rr, Rb), Eb = 1.0f - Er;

		int Sr = 0, Sb = 0;
		for (auto *e : red)  Sr += e->client->resp.score;
		for (auto *e : blue) Sb += e->client->resp.score;

		bool redWin = Sr > Sb;

		for (auto *e : red) {
			float S = redWin ? 1.0f : 0.0f;
			float d = SKILL_K * (S - Er);
			e->client->sess.skillRating += d;
			e->client->sess.skillRatingChange = static_cast<int>(d);
			ClientConfig_SaveStats(e->client, redWin);
		}
		for (auto *e : blue) {
			float S = redWin ? 0.0f : 1.0f;
			float d = SKILL_K * (S - Eb);
			e->client->sess.skillRating += d;
			e->client->sess.skillRatingChange = static_cast<int>(d);
			ClientConfig_SaveStats(e->client, !redWin);
		}

		// Ghosts
		for (auto &g : level.ghosts) {
			if (!*g.socialID)
				continue;

			float S = (g.team == TEAM_RED) ? (redWin ? 1.0f : 0.0f)
				: (g.team == TEAM_BLUE ? (redWin ? 0.0f : 1.0f) : 0.5f);
			float E = (g.team == TEAM_RED) ? Er : (g.team == TEAM_BLUE ? Eb : 0.5f);
			float d = SKILL_K * (S - E);

			g.skillRating += d;
			g.skillRatingChange = static_cast<int>(d);
			ClientConfig_SaveStatsForGhost(g, (g.team == TEAM_RED) ? redWin : (g.team == TEAM_BLUE ? !redWin : false));
		}
		return;
	}

	// === FFA MODE ===
	{
		int n = players.size();

		std::sort(players.begin(), players.end(),
			[](gentity_t *a, gentity_t *b) {
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
			auto *cl = players[i]->client;
			cl->sess.skillRating += delta;
			cl->sess.skillRatingChange = static_cast<int>(delta);
			ClientConfig_SaveStats(cl, i == 0);
		}

		// Ghosts
		std::vector<Ghosts *> sortedGhosts;
		for (auto &g : level.ghosts) {
			if (*g.socialID)
				sortedGhosts.push_back(&g);
		}

		std::sort(sortedGhosts.begin(), sortedGhosts.end(), [](Ghosts *a, Ghosts *b) {
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
	gentity_t *ent;

	time_t now = GetCurrentRealTimeMillis();

	//for (auto ec : active_players())
	//	ec->client->sess.playEndRealTime = now;
	MatchStats_End();
	SetMapLastPlayedTime(level.mapname);

	level.matchState = MatchState::MATCH_ENDED;
	level.matchStateTimer = 0_sec;

	AdjustSkillRatings();

	// stay on same level flag
	if (match_map_sameLevel->integer) {
		BeginIntermission(CreateTargetChangeLevel(level.mapname));
		return;
	}

	if (*level.forceMap) {
		BeginIntermission(CreateTargetChangeLevel(level.forceMap));
		return;
	}

	// pull next map from MyMap queue, if present
	if (!game.mapSystem.playQueue.empty()) {
		const auto &queued = game.mapSystem.playQueue.front();

		//level.changeMap = queued.filename.c_str(); // optional but keeps consistency

		game.overrideEnableFlags = queued.settings.to_ulong();
		game.overrideDisableFlags = ~queued.settings.to_ulong(); // correctly complement enabled flags

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
		const char *str = match_maps_list->string;
		char first_map[MAX_QPATH]{ 0 };
		char *map;

		while (1) {
			map = COM_ParseEx(&str, " ");

			if (!*map)
				break;

			if (Q_strcasecmp(map, level.mapname) == 0) {
				// it's in the list, go to the next one
				map = COM_ParseEx(&str, " ");
				if (!*map) {
					// end of list, go to first one
					if (!first_map[0]) // there isn't a first one, same level
					{
						BeginIntermission(CreateTargetChangeLevel(level.mapname));
						return;
					} else {
						// [Paril-KEX] re-shuffle if necessary
						if (match_maps_listShuffle->integer) {
							auto values = str_split(match_maps_list->string, ' ');

							if (values.size() == 1) {
								// meh
								BeginIntermission(CreateTargetChangeLevel(level.mapname));
								return;
							}

							std::shuffle(values.begin(), values.end(), mt_rand);

							// if the current map is the map at the front, push it to the end
							if (values[0] == level.mapname)
								std::swap(values[0], values[values.size() - 1]);

							gi.cvar_forceset("match_maps_list", fmt::format("{}", join_strings(values, " ")).data());

							BeginIntermission(CreateTargetChangeLevel(values[0].c_str()));
							return;
						}

						BeginIntermission(CreateTargetChangeLevel(first_map));
						return;
					}
				} else {
					BeginIntermission(CreateTargetChangeLevel(map));
					return;
				}
			}
			if (!first_map[0])
				Q_strlcpy(first_map, map, sizeof(first_map));
		}
	}

	if (level.nextMap[0]) { // go to a specific map
		BeginIntermission(CreateTargetChangeLevel(level.nextMap));
		return;
	}

	// search for a changelevel
	ent = G_FindByString<&gentity_t::className>(nullptr, "target_changelevel");

	if (!ent) { // the map designer didn't include a changelevel,
		// so create a fake ent that goes back to the same level
		BeginIntermission(CreateTargetChangeLevel(level.mapname));
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
	if (!warmup_enabled->integer) {
		level.levelStartTime = level.time;
		level.matchState = MatchState::MATCH_IN_PROGRESS;
		level.warmupState = WarmupState::WARMUP_REQ_NONE;
		level.warmupNoticeTime = 0_sec;
		level.matchStateTimer = 0_sec;
		return;
	}

	Entities_Reset(true, true, true);
	UnReadyAll();

	level.matchStartRealTime = GetCurrentRealTimeMillis();
	level.matchEndRealTime = 0;
	level.levelStartTime = level.time;
	level.matchState = MatchState::MATCH_WARMUP_DEFAULT;
	level.warmupState = WarmupState::WARMUP_REQ_NONE;
	level.warmupNoticeTime = 0_sec;
	level.matchStateTimer = 0_sec;
	level.intermissionQueued = 0_sec;
	level.intermission.preExit = false;
	level.intermissionTime = 0_sec;
	memset(&level.match, 0, sizeof(level.match));

	CalculateRanks();

	gi.Broadcast_Print(PRINT_CENTER, ".The match has been reset.\n");
}

/*
============
CheckDMRoundState
============
*/
static void CheckDMRoundState() {
	if (notGTF(GTF_ROUNDS) || level.matchState != MatchState::MATCH_IN_PROGRESS)
		return;

	if (level.roundState == RoundState::ROUND_NONE || level.roundState == RoundState::ROUND_ENDED) {
		if (level.roundStateTimer > level.time)
			return;
		if (GT(GT_RR) && level.roundState == RoundState::ROUND_ENDED)
			TeamShuffle();
		Round_StartNew();
		return;
	}

	if (level.roundState == RoundState::ROUND_COUNTDOWN && level.time >= level.roundStateTimer) {
		for (auto ec : active_clients())
			ec->client->latchedButtons = BUTTON_NONE;
		level.roundState = RoundState::ROUND_IN_PROGRESS;
		level.roundStateTimer = level.time + gtime_t::from_min(roundtimelimit->value);
		level.roundNumber++;
		gi.Broadcast_Print(PRINT_CENTER, ".FIGHT!\n");
		AnnouncerSound(world, "fight");

		if (GT(GT_STRIKE)) {
			CheckRoundStrikeStartTurn();
		}
		return;
	}

	if (level.roundState == RoundState::ROUND_IN_PROGRESS) {
		switch (g_gametype->integer) {
		case GT_CA:     CheckRoundEliminationCA(); break;
		case GT_HORDE:  CheckRoundHorde(); break;
		case GT_RR:     CheckRoundRR(); break;
		}

		if (level.time >= level.roundStateTimer) {
			switch (g_gametype->integer) {
			case GT_CA:     CheckRoundTimeLimitCA(); break;
			case GT_STRIKE: CheckRoundStrikeTimeLimit(); break;
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
void AnnounceCountdown(int t, gtime_t &checkRef) {
	const gtime_t nextCheck = gtime_t::from_sec(t);
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
	if ((level.matchState != MatchState::MATCH_COUNTDOWN
		&& level.roundState != RoundState::ROUND_COUNTDOWN)
		|| level.intermissionTime) {
		level.countdownTimerCheck = 0_sec;
		return;
	}

	// choose the correct base timer
	gtime_t base = (level.roundState == RoundState::ROUND_COUNTDOWN)
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
	if (GTF(GTF_ROUNDS))
		return;

	if (level.matchState != MatchState::MATCH_IN_PROGRESS || !timelimit->value) {
		if (level.matchEndWarnTimerCheck)
			level.matchEndWarnTimerCheck = 0_sec;
		return;
	}

	int t = (level.levelStartTime + gtime_t::from_min(timelimit->value) - level.time).seconds<int>();	// +1;

	if (!level.matchEndWarnTimerCheck || level.matchEndWarnTimerCheck.seconds<int>() > t) {
		if (t && (t == 30 || t == 20 || t <= 10)) {
			//AnnouncerSound(world, nullptr, G_Fmt("world/{}{}.wav", t, t >= 20 ? "sec" : "").data(), false);
			//gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex(G_Fmt("world/{}{}.wav", t, t >= 20 ? "sec" : "").data()), 1, ATTN_NONE, 0);
			if (t >= 10)
				gi.LocBroadcast_Print(PRINT_HIGH, "{} second warning!\n", t);
		} else if (t == 300 || t == 60) {
			AnnouncerSound(world, G_Fmt("{}_minute", t == 300 ? 5 : 1).data());
		}
		level.matchEndWarnTimerCheck = gtime_t::from_sec(t);
	}
}

/*
=============
CheckDMWarmupState
=============
*/
static void CheckDMWarmupState() {
	const bool duel = GTF(GTF_1V1);
	const int min_players = duel ? 2 : minplayers->integer;

	// Handle no players
	if (!level.pop.num_playing_clients) {
		if (level.matchState != MatchState::MATCH_NONE) {
			level.matchState = MatchState::MATCH_NONE;
			level.matchStateTimer = 0_sec;
			level.warmupState = WarmupState::WARMUP_REQ_NONE;
			level.warmupNoticeTime = 0_sec;
		}

		// Pull in idle bots
		for (auto ec : active_clients())
			if (!ClientIsPlaying(ec->client) && (ec->client->sess.is_a_bot || ec->svFlags & SVF_BOT))
				SetTeam(ec, PickTeam(-1), false, false, false);
		return;
	}

	// Pull queued players (if needed) during 1v1
	if (GTF(GTF_1V1) && Versus_AddPlayer())
		return;

	// If warmup disabled and enough players, start match
	if (level.matchState < MatchState::MATCH_COUNTDOWN &&
		!warmup_enabled->integer &&
		level.pop.num_playing_clients >= min_players) {
		Match_Start();
		return;
	}

	// Trigger initial delayed warmup on fresh map
	if (level.matchState == MatchState::MATCH_NONE) {
		level.matchState = MatchState::MATCH_WARMUP_DELAYED;
		level.matchStateTimer = level.time + 5_sec;
		level.warmupState = WarmupState::WARMUP_REQ_NONE;
		level.warmupNoticeTime = level.time;
		return;
	}

	// Wait for delayed warmup to trigger
	if (level.matchState == MatchState::MATCH_WARMUP_DELAYED &&
		level.matchStateTimer > level.time)
		return;

	// Run spawning logic during warmup (e.g., Horde)
	if (level.matchState == MatchState::MATCH_WARMUP_DEFAULT ||
		level.matchState == MatchState::MATCH_WARMUP_READYUP) {
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
		if (level.matchState <= MatchState::MATCH_COUNTDOWN) {
			if (level.matchState == MatchState::MATCH_WARMUP_READYUP)
				UnReadyAll();

			if (level.matchState == MatchState::MATCH_COUNTDOWN) {
				const char *reason = teamsImbalanced ? "teams are imbalanced" : "not enough players";
				gi.LocBroadcast_Print(PRINT_CENTER, ".Countdown cancelled: {}\n", reason);
			}

			if (level.matchState != MatchState::MATCH_WARMUP_DEFAULT) {
				level.matchState = MatchState::MATCH_WARMUP_DEFAULT;
				level.matchStateTimer = 0_sec;
				level.warmupState = teamsImbalanced ? WarmupState::WARMUP_REQ_BALANCE : WarmupState::WARMUP_REQ_MORE_PLAYERS;
				level.warmupNoticeTime = level.time;
			}
		}
		return;
	}

	// If we're in default warmup and ready-up is required
	if (level.matchState == MatchState::MATCH_WARMUP_DEFAULT) {
		if (!warmup_enabled->integer && g_warmup_countdown->integer <= 0) {
			level.matchState = MatchState::MATCH_COUNTDOWN;
			level.matchStateTimer = 0_sec;
		} else {
			// Transition to ready-up
			level.matchState = MatchState::MATCH_WARMUP_READYUP;
			level.matchStateTimer = 0_sec;
			level.warmupState = WarmupState::WARMUP_REQ_READYUP;
			level.warmupNoticeTime = level.time;

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
	if (level.matchState <= MatchState::MATCH_COUNTDOWN &&
		g_warmup_countdown->modified_count != level.warmupModificationCount) {
		level.warmupModificationCount = g_warmup_countdown->modified_count;
		level.matchState = MatchState::MATCH_WARMUP_DEFAULT;
		level.warmupState = WarmupState::WARMUP_REQ_NONE;
		level.matchStateTimer = 0_sec;
		level.warmupNoticeTime = 0_sec;
		level.prepare_to_fight = false;
		return;
	}

	// Ready-up check
	if (level.matchState == MatchState::MATCH_WARMUP_READYUP) {
		if (!CheckReady())
			return;

		if (g_warmup_countdown->integer > 0) {
			level.matchState = MatchState::MATCH_COUNTDOWN;
			level.warmupState = WarmupState::WARMUP_REQ_NONE;
			level.warmupNoticeTime = 0_sec;

			level.matchStateTimer = level.time + gtime_t::from_sec(g_warmup_countdown->integer);

			if ((duel || (level.pop.num_playing_clients == 2 && match_lock->integer)) &&
				game.clients[level.sorted_clients[0]].pers.connected &&
				game.clients[level.sorted_clients[1]].pers.connected) {
				gi.LocBroadcast_Print(PRINT_CENTER, "{} vs {}\nBegins in...",
					game.clients[level.sorted_clients[0]].sess.netName,
					game.clients[level.sorted_clients[1]].sess.netName);
			} else {
				gi.LocBroadcast_Print(PRINT_CENTER, "{}\nBegins in...", level.gametype_name.data());
			}

			if (!level.prepare_to_fight) {
				const char *sound = (Teams() && level.pop.num_playing_clients >= 4) ? "prepare_your_team" : "prepare_to_fight";
				AnnouncerSound(world, sound);
				level.prepare_to_fight = true;
			}
			return;
		} else {
			// No countdown, start immediately
			Match_Start();
			return;
		}
	}

	// Final check: countdown timer expired?
	if (level.matchState == MatchState::MATCH_COUNTDOWN &&
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
	CheckDMMatchEndWarning(); // Optional: match-ending warnings

	// see if it is time to end a deathmatch
	CheckDMExitRules();       // Handles intermission and map end

	if (g_verbose->integer) {
		static constexpr const char *MatchStateNames[] = {
			"MATCH_NONE",
			"MATCH_WARMUP_DELAYED",
			"MATCH_WARMUP_DEFAULT",
			"MATCH_WARMUP_READYUP",
			"MATCH_COUNTDOWN",
			"MATCH_IN_PROGRESS",
			"MATCH_ENDED"
		};

		const char *stateName = (static_cast<size_t>(level.matchState) < std::size(MatchStateNames))
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
	} else {
		int halfpoint = level.pop.num_voting_clients / 2;
		if (level.vote.countYes > halfpoint) {
			// execute the command, then remove the vote
			gi.Broadcast_Print(PRINT_HIGH, "Vote passed.\n");
			level.vote.executeTime = level.time + 3_sec;
			AnnouncerSound(world, "vote_passed");
		} else if (level.vote.countNo >= halfpoint) {
			// same behavior as a timeout
			gi.Broadcast_Print(PRINT_HIGH, "Vote failed.\n");
			AnnouncerSound(world, "vote_failed");
		} else {
			// still waiting for a majority
			return;
		}
	}

	level.vote.time = 0_sec;
}


/*
=================
CheckDMIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.

Adapted from Quake III
=================
*/

static void CheckDMIntermissionExit(void) {
	int ready, not_ready;

	// see which players are ready
	ready = not_ready = 0;
	for (auto ec : active_clients()) {
		if (!ClientIsPlaying(ec->client))
			continue;

		if (ec->client->sess.is_a_bot)
			ec->client->readyToExit = true;

		if (ec->client->readyToExit)
			ready++;
		else
			not_ready++;
	}

	// vote in progress
	if (level.vote.time || level.vote.executeTime) {
		ready = 0;
		not_ready = 1;
	}

	// never exit in less than five seconds
	if (level.time < level.intermissionTime + 5_sec && !level.exitTime)
		return;

	// if nobody wants to go, clear timer
	// skip this if no players present
	if (!ready && not_ready) {
		level.readyToExit = false;
		return;
	}

	// if everyone wants to go, go now
	if (!not_ready) {
		ExitLevel();
		return;
	}

	// the first person to ready starts the ten second timeout
	if (ready && !level.readyToExit) {
		level.readyToExit = true;
		level.exitTime = level.time + 10_sec;
	}

	// if we have waited ten seconds since at least one player
	// wanted to exit, go ahead
	if (level.time < level.exitTime)
		return;

	ExitLevel();
}

/*
=============
ScoreIsTied
=============
*/
static bool ScoreIsTied(void) {
	if (level.pop.num_playing_clients < 2)
		return false;

	if (Teams() && notGT(GT_RR))
		return level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE];

	return game.clients[level.sorted_clients[0]].resp.score == game.clients[level.sorted_clients[1]].resp.score;
}


int GT_ScoreLimit() {
	if (GTF(GTF_ROUNDS))
		return roundlimit->integer;
	if (GT(GT_CTF))
		return capturelimit->integer;
	return fraglimit->integer;
}

const char *GT_ScoreLimitString() {
	if (GT(GT_CTF))
		return "capture";
	if (GTF(GTF_ROUNDS))
		return "round";
	return "frag";
}

/*
=================
CheckDMExitRules

There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag/capture.
=================
*/
void CheckDMExitRules() {

	// if at the intermission, wait for all non-bots to
	// signal ready, then go to next level
	if (level.intermissionTime) {
		CheckDMIntermissionExit();
		return;
	}

	if (!level.pop.num_playing_clients && noplayerstime->integer && level.time > level.no_players_time + gtime_t::from_min(noplayerstime->integer)) {
		Match_End();
		return;
	}

	if (level.intermissionQueued) {
		if (level.time - level.intermissionQueued >= 1_sec) {
			level.intermissionQueued = 0_ms;
			Match_End();
		}
		return;
	}

	if (level.matchState < MatchState::MATCH_IN_PROGRESS)
		return;

	if (level.time - level.levelStartTime <= FRAME_TIME_MS)
		return;

	if (GT(GT_HORDE)) {
		if ((level.totalMonsters - level.killedMonsters) >= 100) {
			gi.Broadcast_Print(PRINT_CENTER, "DEFEATED!");
			QueueIntermission("OVERRUN BY MONSTERS!", true, false);
			return;
		}
	}

	if (GTF(GTF_ROUNDS) && level.roundState != RoundState::ROUND_ENDED)
		return;

	if (GT(GT_HORDE)) {
		if (roundlimit->integer > 0 && level.roundNumber >= roundlimit->integer) {
			QueueIntermission(G_Fmt("{} WINS with a final score of {}.", game.clients[level.sorted_clients[0]].sess.netName, game.clients[level.sorted_clients[0]].resp.score).data(), false, false);
			return;
		}
	}

	if (!match_startNoHumans->integer && !level.pop.num_playing_human_clients) {
		if (!level.endmatch_grace) {
			level.endmatch_grace = level.time;
			return;
		}
		if (level.time > level.endmatch_grace + 200_ms)
			QueueIntermission("No human players remaining.", true, false);
		return;
	}

	if (minplayers->integer > 0 && level.pop.num_playing_clients < minplayers->integer) {
		if (!level.endmatch_grace) {
			level.endmatch_grace = level.time;
			return;
		}
		if (level.time > level.endmatch_grace + 200_ms)
			QueueIntermission("Not enough players remaining.", true, false);
		return;
	}

	bool teams = Teams() && notGT(GT_RR);

	if (teams && g_teamplay_force_balance->integer) {
		if (abs(level.pop.num_playing_red - level.pop.num_playing_blue) > 1) {
			if (g_teamplay_auto_balance->integer) {
				TeamBalance(true);
			} else {
				if (!level.endmatch_grace) {
					level.endmatch_grace = level.time;
					return;
				}
				if (level.time > level.endmatch_grace + 200_ms)
					QueueIntermission("Teams are imbalanced.", true, true);
			}
			return;
		}
	}

	if (timelimit->value) {
		if (notGTF(GTF_ROUNDS) || level.roundState == RoundState::ROUND_ENDED) {
			if (level.time >= level.levelStartTime + gtime_t::from_min(timelimit->value) + level.overtime) {
				// check for overtime
				if (ScoreIsTied()) {
					bool play = false;

					if (GTF(GTF_1V1) && match_doOvertime->integer > 0) {
						level.overtime += gtime_t::from_sec(match_doOvertime->integer);
						gi.LocBroadcast_Print(PRINT_CENTER, "Overtime!\n{} added", TimeString(match_doOvertime->integer * 1000, false, false));
						AnnouncerSound(world, "overtime");
						play = true;
					} else if (!level.suddenDeath) {
						gi.Broadcast_Print(PRINT_CENTER, "Sudden Death!");
						AnnouncerSound(world, "sudden_death");
						level.suddenDeath = true;
						play = true;
					}

					//if (play)
						//gi.positioned_sound(world->s.origin, world, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundindex("world/klaxon2.wav"), 1, ATTN_NONE, 0);
					return;
				}

				// find the winner and broadcast it
				if (teams) {
					if (level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE]) {
						QueueIntermission(G_Fmt("{} Team WINS with a final score of {} to {}.\n", Teams_TeamName(TEAM_RED), level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE]).data(), false, false);
						return;
					}
					if (level.teamScores[TEAM_BLUE] > level.teamScores[TEAM_RED]) {
						QueueIntermission(G_Fmt("{} Team WINS with a final score of {} to {}.\n", Teams_TeamName(TEAM_BLUE), level.teamScores[TEAM_BLUE], level.teamScores[TEAM_RED]).data(), false, false);
						return;
					}
				} else {
					QueueIntermission(G_Fmt("{} WINS with a final score of {}.", game.clients[level.sorted_clients[0]].sess.netName, game.clients[level.sorted_clients[0]].resp.score).data(), false, false);
					return;
				}

				QueueIntermission("Timelimit hit.", false, false);
				return;
			}
		}
	}

	if (mercylimit->integer > 0) {
		if (teams) {
			if (level.teamScores[TEAM_RED] >= level.teamScores[TEAM_BLUE] + mercylimit->integer) {
				QueueIntermission(G_Fmt("{} hit the mercylimit ({}).", Teams_TeamName(TEAM_RED), mercylimit->integer).data(), true, false);
				return;
			}
			if (level.teamScores[TEAM_BLUE] >= level.teamScores[TEAM_RED] + mercylimit->integer) {
				QueueIntermission(G_Fmt("{} hit the mercylimit ({}).", Teams_TeamName(TEAM_BLUE), mercylimit->integer).data(), true, false);
				return;
			}
		} else {
			if (notGT(GT_HORDE)) {
				gclient_t *cl1, *cl2;

				cl1 = &game.clients[level.sorted_clients[0]];
				cl2 = &game.clients[level.sorted_clients[1]];
				if (cl1 && cl2) {
					if (cl1->resp.score >= cl2->resp.score + mercylimit->integer) {
						QueueIntermission(G_Fmt("{} hit the mercylimit ({}).", cl1->sess.netName, mercylimit->integer).data(), true, false);
						return;
					}
				}
			}
		}
	}

	// check for sudden death
	if (ScoreIsTied())
		return;

	// no score limit in horde
	if (GT(GT_HORDE))
		return;

	int	scorelimit = GT_ScoreLimit();
	if (!scorelimit) return;

	if (teams) {
		if (level.teamScores[TEAM_RED] >= scorelimit) {
			QueueIntermission(G_Fmt("{} WINS! (hit the {} limit)", Teams_TeamName(TEAM_RED), GT_ScoreLimitString()).data(), false, false);
			return;
		}
		if (level.teamScores[TEAM_BLUE] >= scorelimit) {
			QueueIntermission(G_Fmt("{} WINS! (hit the {} limit)", Teams_TeamName(TEAM_BLUE), GT_ScoreLimitString()).data(), false, false);
			return;
		}
	} else {
		for (auto ec : active_clients()) {
			if (ec->client->sess.team != TEAM_FREE)
				continue;

			if (ec->client->resp.score >= scorelimit) {
				QueueIntermission(G_Fmt("{} WINS! (hit the {} limit)", ec->client->sess.netName, GT_ScoreLimitString()).data(), false, false);
				return;
			}
		}
	}
}

static bool Match_NextMap() {
	if (level.matchState == MatchState::MATCH_ENDED) {
		level.matchState = MatchState::MATCH_WARMUP_DELAYED;
		level.warmupNoticeTime = level.time;
		Match_Reset();
		return true;
	}
	return false;
}

void GT_Init() {
	constexpr const char *COOP = "coop";
	bool force_dm = false;

	deathmatch = gi.cvar("deathmatch", "1", CVAR_LATCH);
	teamplay = gi.cvar("teamplay", "0", CVAR_SERVERINFO);
	ctf = gi.cvar("ctf", "0", CVAR_SERVERINFO);
	g_gametype = gi.cvar("g_gametype", G_Fmt("{}", (int)GT_FFA).data(), CVAR_SERVERINFO);
	coop = gi.cvar("coop", "0", CVAR_LATCH);

	// game modifications
	g_instagib = gi.cvar("g_instagib", "0", CVAR_SERVERINFO | CVAR_LATCH);
	g_instagib_splash = gi.cvar("g_instagib_splash", "0", CVAR_NOFLAGS);
	g_owner_auto_join = gi.cvar("g_owner_auto_join", "0", CVAR_NOFLAGS);
	g_owner_push_scores = gi.cvar("g_owner_push_scores", "1", CVAR_NOFLAGS);
	g_quadhog = gi.cvar("g_quadhog", "0", CVAR_SERVERINFO | CVAR_LATCH);
	g_nadefest = gi.cvar("g_nadefest", "0", CVAR_SERVERINFO | CVAR_LATCH);
	g_frenzy = gi.cvar("g_frenzy", "0", CVAR_SERVERINFO | CVAR_LATCH);
	g_vampiric_damage = gi.cvar("g_vampiric_damage", "0", CVAR_NOFLAGS);
	g_vampiric_exp_min = gi.cvar("g_vampiric_exp_min", "0", CVAR_NOFLAGS);
	g_vampiric_health_max = gi.cvar("g_vampiric_health_max", "9999", CVAR_NOFLAGS);
	g_vampiric_percentile = gi.cvar("g_vampiric_percentile", "0.67f", CVAR_NOFLAGS);

	if (g_gametype->integer < 0 || g_gametype->integer >= GT_NUM_GAMETYPES)
		gi.cvar_forceset("g_gametype", G_Fmt("{}", clamp(g_gametype->integer, (int)GT_FIRST, (int)GT_LAST)).data());

	if (ctf->integer) {
		force_dm = true;
		// force coop off
		if (coop->integer)
			gi.cvar_set(COOP, "0");
		// force tdm off
		if (teamplay->integer)
			gi.cvar_set("teamplay", "0");
	}
	if (teamplay->integer) {
		force_dm = true;
		// force coop off
		if (coop->integer)
			gi.cvar_set(COOP, "0");
	}

	if (force_dm && !deathmatch->integer) {
		gi.Com_Print("Forcing deathmatch.\n");
		gi.cvar_forceset("deathmatch", "1");
	}

	// force even maxplayers value during teamplay
	if (Teams()) {
		int pmax = maxplayers->integer;

		if (pmax != floor(pmax / 2))
			gi.cvar_set("maxplayers", G_Fmt("{}", floor(pmax / 2) * 2).data());
	}

	GT_SetLongName();
}

void ChangeGametype(gametype_t gt) {
	switch (gt) {
	case gametype_t::GT_CTF:
		if (!ctf->integer)
			gi.cvar_forceset("ctf", "1");
		break;
	case gametype_t::GT_TDM:
		if (!teamplay->integer)
			gi.cvar_forceset("teamplay", "1");
		break;
	default:
		if (ctf->integer)
			gi.cvar_forceset("ctf", "0");
		if (teamplay->integer)
			gi.cvar_forceset("teamplay", "0");
		break;
	}

	if (!deathmatch->integer) {
		gi.Com_Print("Forcing deathmatch.\n");
		gi.cvar_forceset("deathmatch", "1");
	}

	if ((int)gt != g_gametype->integer)
		gi.cvar_forceset("g_gametype", G_Fmt("{}", (int)gt).data());
}
