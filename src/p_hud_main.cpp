// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"
#include "g_statusbar.h"

/*
======================================================================

INTERMISSION

======================================================================
*/

/*
===============
MoveClientToIntermission
===============
*/
void MoveClientToIntermission(gentity_t *ent) {
	if (ent->svFlags & SVF_NOCLIENT) {
		ent->s.event = EV_OTHER_TELEPORT;
	}

	// Set client view and movement
	ent->s.origin = level.intermission.origin;
	//ent->client->ps.pmove.origin = level.intermission.origin;
	ent->s.angles = level.intermission.angles;
	//gi.Com_PrintFmt("Moving client {} to intermission at origin {} with angles {}\n",
	//	ent->client->sess.netName, level.intermission.origin, level.intermission.angles);
	ent->client->ps.viewAngles = ent->s.angles;
	ent->client->vAngle = ent->s.angles;
	ent->client->ps.pmove.delta_angles[PITCH] = ent->s.angles[PITCH];
	ent->client->ps.pmove.pm_type = PM_FREEZE;
	ent->client->ps.gunIndex = 0;
	ent->client->ps.gunSkin = 0;
	ent->client->ps.damageBlend[3] = 0;
	ent->client->ps.screenBlend[3] = 0;
	ent->client->ps.rdFlags = RDF_NONE;

	// Reset powerup timers
	ent->client->powerupTime = {};

	// Reset grenade and timers
	ent->client->grenadeBlewUp = false;
	ent->client->grenadeTime = 0_ms;
	ent->client->powerupTime.irGoggles = 0_ms;
	ent->client->nukeTime = 0_ms;
	ent->client->trackerPainTime = 0_ms;

	// Reset HUD flags
	ent->client->showHelp = false;
	ent->client->showScores = false;
	ent->client->showInventory = false;

	// Clear slow time flag
	globals.server_flags &= ~SERVER_FLAG_SLOW_TIME;

	// Intermission model state
	ent->viewHeight = 0;
	ent->s.modelindex = 0;
	ent->s.modelindex2 = 0;
	ent->s.modelindex3 = 0;
	ent->s.effects = EF_NONE;
	ent->s.sound = 0;
	ent->solid = SOLID_NOT;
	ent->moveType = MOVETYPE_FREECAM;

	gi.linkentity(ent);

	if (deathmatch->integer) {
		if (!g_autoScreenshotTool->integer) {
			MultiplayerScoreboard(ent);
			ent->client->showScores = true;
		}
		ent->client->ps.stats[STAT_SHOW_STATUSBAR] = 0;
	}
}

/*
===============
UpdateLevelEntry
===============
*/
void UpdateLevelEntry() {
	if (!level.entry)
		return;

	level.entry->foundSecrets = level.campaign.foundSecrets;
	level.entry->totalSecrets = level.campaign.totalSecrets;
	level.entry->killedMonsters = level.campaign.killedMonsters;
	level.entry->totalMonsters = level.campaign.totalMonsters;
}

/*
===============
SortLevelEntries
===============
*/
static void SortLevelEntries() {
	std::sort(game.level_entries.begin(), game.level_entries.end(),
		[](const level_entry_t &a, const level_entry_t &b) {
			int32_t a_order = a.visit_order
				? a.visit_order
				: (*a.pretty_name ? MAX_LEVELS_PER_UNIT + 1 : MAX_LEVELS_PER_UNIT + 2);
			int32_t b_order = b.visit_order
				? b.visit_order
				: (*b.pretty_name ? MAX_LEVELS_PER_UNIT + 1 : MAX_LEVELS_PER_UNIT + 2);
			return a_order < b_order;
		});
}

/*
===============
BuildEOUTableRow

Appends a single End-of-Unit stats row to the layout.
If isTotalsRow is true, it renders the total row.
===============
*/
static void BuildEOUTableRow(std::stringstream &layout, int y, const level_entry_t &entry, bool isTotalsRow = false) {
	layout << G_Fmt("yv {} ", y).data();

	if (!isTotalsRow && !*entry.pretty_name) {
		layout << "table_row 1 ??? ";
		return;
	}

	const char *name = isTotalsRow ? "Totals" : entry.pretty_name;

	layout << G_Fmt("table_row 4 \"{}\" ", name).data()
		<< G_Fmt("{}/{} ", entry.killedMonsters, entry.totalMonsters).data()
		<< G_Fmt("{}/{} ", entry.foundSecrets, entry.totalSecrets).data();

	int32_t ms = entry.time.milliseconds();
	int32_t minutes = ms / 60000;
	int32_t seconds = (ms / 1000) % 60;
	int32_t milliseconds = ms % 1000;

	layout << G_Fmt("{:02}:{:02}:{:03} ", minutes, seconds, milliseconds).data();
}

/*
===============
AddEOUTotalsRow
===============
*/
static void AddEOUTotalsRow(std::stringstream &layout, int y, const level_entry_t &totals) {
	y += 8;
	layout << "table_row 0 ";  // Spacer row
	BuildEOUTableRow(layout, y, totals, true);
}

/*
===============
BroadcastEOULayout
===============
*/
static void BroadcastEOULayout(const std::stringstream &layout) {
	// Finalize table rendering
	std::string out = layout.str();
	out += "xv 160 yt 0 draw_table ";

	// Add intermission press button prompt (after 5 seconds)
	int frameGate = level.intermission.serverFrame + (5_sec).frames();
	fmt::format_to(std::back_inserter(out),
		"ifgef {} yb -48 xv 0 loc_cstring2 0 \"$m_eou_press_button\" endif ",
		frameGate);

	gi.WriteByte(svc_layout);
	gi.WriteString(out.c_str());
	gi.multicast(vec3_origin, MULTICAST_ALL, true);

	for (gentity_t *player : active_clients()) {
		player->client->showEOU = true;
	}
}

/*
===============
EndOfUnitMessage
===============
*/
void EndOfUnitMessage() {
	UpdateLevelEntry();
	SortLevelEntries();

	std::stringstream layout;
	layout << "start_table 4 $m_eou_level $m_eou_kills $m_eou_secrets $m_eou_time ";

	int y = 16;
	level_entry_t totals{};
	int32_t numRows = 0;

	for (auto &entry : game.level_entries) {
		if (!*entry.map_name)
			break;

		BuildEOUTableRow(layout, y, entry);
		y += 8;

		totals.killedMonsters += entry.killedMonsters;
		totals.totalMonsters += entry.totalMonsters;
		totals.foundSecrets += entry.foundSecrets;
		totals.totalSecrets += entry.totalSecrets;
		totals.time += entry.time;

		if (entry.visit_order)
			numRows++;
	}

	if (numRows > 1)
		AddEOUTotalsRow(layout, y, totals);

	BroadcastEOULayout(layout);
}

/*
===============
ReportMatchDetails

data is binary now.
u8 num_teams
u8 num_players
[ repeat num_teams:
	string team_name
]
[ repeat num_players:
	u8 client_index
	s32 score
	u8 ranking
	(if num_teams > 0)
		u8 team
]
===============
*/
void ReportMatchDetails(bool is_end) {
	static std::array<uint32_t, MAX_CLIENTS> player_ranks;

	player_ranks = {};
	bool teams = Teams() && notGT(GT_RR);

	// teamplay is simple
	if (teams) {
		Teams_CalcRankings(player_ranks);

		gi.WriteByte(2);
		gi.WriteString(g_redTeamName->string[0] ? g_redTeamName->string : "RED TEAM");
		gi.WriteString(g_blueTeamName->string[0] ? g_blueTeamName->string : "BLUE TEAM");
	} else {
		// sort players by score, then match everybody to
		// the current highest score downwards until we run out of players.
		static std::array<gentity_t *, MAX_CLIENTS> sorted_players;
		size_t num_active_players = 0;

		for (auto player : active_clients())
			sorted_players[num_active_players++] = player;

		std::sort(sorted_players.begin(), sorted_players.begin() + num_active_players, [](const gentity_t *a, const gentity_t *b) { return b->client->resp.score < a->client->resp.score; });

		int32_t current_score = INT_MIN;
		int32_t current_rank = 0;

		for (size_t i = 0; i < num_active_players; i++) {
			if (!current_rank || sorted_players[i]->client->resp.score != current_score) {
				current_rank++;
				current_score = sorted_players[i]->client->resp.score;
			}
			int index = static_cast<int>(sorted_players[i]->s.number) - 1;
			if (index >= 0 && index < player_ranks.size())
				player_ranks[index] = current_rank;
		}

		gi.WriteByte(0);
	}

	uint8_t num_players = 0;

	for (auto player : active_players()) {
		// leave spectators out of this data, they don't need to be seen.
		if (player->client->pers.spawned) {
			// just in case...
			if (teams && !ClientIsPlaying(player->client))
				continue;

			num_players++;
		}
	}

	gi.WriteByte(num_players);

	for (auto player : active_players()) {
		// leave spectators out of this data, they don't need to be seen.
		if (player->client->pers.spawned) {
			// just in case...
			if (teams && !ClientIsPlaying(player->client))
				continue;

			gi.WriteByte(player->s.number - 1);
			gi.WriteLong(player->client->resp.score);
			int index = static_cast<int>(player->s.number) - 1;
			if (index >= 0 && index < player_ranks.size())
				gi.WriteByte(player_ranks[index]);

			if (teams)
				gi.WriteByte(player->client->sess.team == TEAM_RED ? 0 : 1);
		}
	}

	gi.ReportMatchDetails_Multicast(is_end);
}

/*
===============
DrawHelpComputer

Displays the help computer screen with objectives and stats.
===============
*/
static void DrawHelpComputer(gentity_t *ent) {
	const char *skillName = nullptr;

	switch (skill->integer) {
	case 0: skillName = "$m_easy"; break;
	case 1: skillName = "$m_medium"; break;
	case 2: skillName = "$m_hard"; break;
	case 3: skillName = "$m_nightmare"; break;
	default: skillName = "nightmare+"; break;
	}

	std::string layout;

	// Background and level name
	layout += G_Fmt("xv 32 yv 8 picn help "
		"xv 0 yv 25 cstring2 \"{}\" ", level.longName).data();

	if (level.isN64) {
		// N64 layout
		layout += G_Fmt("xv 0 yv 54 loc_cstring 1 \"{{}}\" \"{}\" ", game.helpmessage1).data();
	} else {
		int y = 54;

		if (*game.helpmessage1) {
			layout += G_Fmt("xv 0 yv {} loc_cstring2 0 \"$g_pc_primary_objective\" ", y).data();
			y += 11;
			layout += G_Fmt("xv 0 yv {} loc_cstring 0 \"{}\" ", y, game.helpmessage1).data();
			y += 47;
		}

		if (*game.helpmessage2) {
			layout += G_Fmt("xv 0 yv {} loc_cstring2 0 \"$g_pc_secondary_objective\" ", y).data();
			y += 11;
			layout += G_Fmt("xv 0 yv {} loc_cstring 0 \"{}\" ", y, game.helpmessage2).data();
		}
	}

	// Bottom stats: skill and performance
	layout += G_Fmt("xv 55 yv 164 loc_string2 0 \"{}\" "
		"xv 265 yv 164 loc_rstring2 1 \"{{}}: {}/{}\" \"$g_pc_goals\" "
		"xv 55 yv 172 loc_string2 1 \"{{}}: {}/{}\" \"$g_pc_kills\" "
		"xv 265 yv 172 loc_rstring2 1 \"{{}}: {}/{}\" \"$g_pc_secrets\" ",
		skillName,
		level.campaign.foundGoals, level.campaign.totalGoals,
		level.campaign.killedMonsters, level.campaign.totalMonsters,
		level.campaign.foundSecrets, level.campaign.totalSecrets).data();

	// Send to client
	gi.WriteByte(svc_layout);
	gi.WriteString(layout.c_str());
	gi.unicast(ent, true);
}

/*
===============
Cmd_Help_f

Toggles the help screen (objectives or stats).
In deathmatch, this shows the scoreboard instead.
===============
*/
void Cmd_Help_f(gentity_t *ent) {
	// In deathmatch, "help" key shows the scoreboard
	if (deathmatch->integer) {
		Cmd_Score_f(ent);
		return;
	}

	if (level.intermissionTime)
		return;

	ent->client->showInventory = false;
	ent->client->showScores = false;

	// Toggle off if help hasn't changed
	if (ent->client->showHelp &&
		ent->client->pers.game_help1changed == game.help1changed &&
		ent->client->pers.game_help2changed == game.help2changed) {
		ent->client->showHelp = false;
		globals.server_flags &= ~SERVER_FLAG_SLOW_TIME;
		return;
	}

	ent->client->showHelp = true;
	ent->client->pers.helpchanged = 0;
	globals.server_flags |= SERVER_FLAG_SLOW_TIME;

	DrawHelpComputer(ent);
}

//=======================================================================

/*
===============
SetCoopStats

Sets HUD stats used in cooperative gameplay.
This runs even if the player is spectating.
===============
*/
void SetCoopStats(gentity_t *ent) {
	// Show lives (if enabled)
	if (CooperativeModeOn() && g_coop_enable_lives->integer) {
		ent->client->ps.stats[STAT_LIVES] = ent->client->pers.lives + 1;
	} else {
		ent->client->ps.stats[STAT_LIVES] = 0;
	}

	// Monster count (if horde)
	if (level.matchState == MatchState::MATCH_IN_PROGRESS) {
		if (GT(GT_HORDE)) {
			ent->client->ps.stats[STAT_MONSTER_COUNT] = level.campaign.totalMonsters - level.campaign.killedMonsters;
		} else {
			ent->client->ps.stats[STAT_MONSTER_COUNT] = 0;
		}

		// Round number (if rounds mode)
		if (GTF(GTF_ROUNDS)) {
			ent->client->ps.stats[STAT_ROUND_NUMBER] = level.roundNumber;
		} else {
			ent->client->ps.stats[STAT_ROUND_NUMBER] = 0;
		}
	}

	// Respawn status string index
	if (ent->client->coop_respawn_state) {
		ent->client->ps.stats[STAT_COOP_RESPAWN] =
			CONFIG_COOP_RESPAWN_STRING + (ent->client->coop_respawn_state - COOP_RESPAWN_IN_COMBAT);
	} else {
		ent->client->ps.stats[STAT_COOP_RESPAWN] = 0;
	}
}

/*
=================
powerup_info_t

Describes a powerup's timer and optional counter field accessor.
Used to drive time-based or count-based powerups like quad, invis, silencer, etc.
=================
*/
struct powerup_info_t {
	item_id_t item;
	std::function<gtime_t &(gclient_t &)> time_accessor;
	std::function<int32_t &(gclient_t &)> count_accessor;
};

static const powerup_info_t powerup_table[] = {
	{ IT_POWERUP_QUAD,        [](gclient_t &c) -> gtime_t & { return c.powerupTime.quadDamage; },      nullptr },
	{ IT_POWERUP_DOUBLE,      [](gclient_t &c) -> gtime_t & { return c.powerupTime.doubleDamage; },     nullptr },
	{ IT_POWERUP_BATTLESUIT,  [](gclient_t &c) -> gtime_t & { return c.powerupTime.battleSuit; },       nullptr },
	{ IT_POWERUP_HASTE,       [](gclient_t &c) -> gtime_t & { return c.powerupTime.haste; },            nullptr },
	{ IT_POWERUP_INVISIBILITY,[](gclient_t &c) -> gtime_t & { return c.powerupTime.invisibility; },     nullptr },
	{ IT_POWERUP_REGEN,       [](gclient_t &c) -> gtime_t & { return c.powerupTime.regeneration; },     nullptr },
	{ IT_POWERUP_ENVIROSUIT,  [](gclient_t &c) -> gtime_t & { return c.powerupTime.enviroSuit; },       nullptr },
	{ IT_POWERUP_REBREATHER,  [](gclient_t &c) -> gtime_t & { return c.powerupTime.rebreather; },       nullptr },
	{ IT_IR_GOGGLES,          [](gclient_t &c) -> gtime_t & { return c.powerupTime.irGoggles; },        nullptr },
	{ IT_POWERUP_SILENCER,    nullptr,                         [](gclient_t &c) -> int32_t & { return reinterpret_cast<int32_t &>(c.powerupTime.silencerShots); } }
};

/*
===============
SetCrosshairIDView

Sets crosshair target ID and team color for the HUD.
Only runs periodically and respects invisibility and team rules.
===============
*/
static void SetCrosshairIDView(gentity_t *ent) {
	if (level.time - ent->client->resp.lastIDTime < 250_ms)
		return;

	ent->client->resp.lastIDTime = level.time;

	ent->client->ps.stats[STAT_CROSSHAIR_ID_VIEW] = 0;
	ent->client->ps.stats[STAT_CROSSHAIR_ID_VIEW_COLOR] = 0;

	if (!match_crosshairIDs->integer)
		return;

	vec3_t forward;
	AngleVectors(ent->client->vAngle, forward, nullptr, nullptr);
	forward *= 1024;
	vec3_t target = ent->s.origin + forward;

	trace_t tr = gi.traceline(ent->s.origin, target, ent, CONTENTS_MIST | MASK_WATER | MASK_SOLID);

	if (tr.fraction < 1.0f && tr.ent && tr.ent->client && tr.ent->health > 0) {
		if (!ClientIsPlaying(tr.ent->client) || tr.ent->client->eliminated)
			return;

		if (tr.ent->client->powerupTime.invisibility > level.time)
			return;

		ent->client->ps.stats[STAT_CROSSHAIR_ID_VIEW] = (tr.ent - g_entities);

		switch (tr.ent->client->sess.team) {
		case TEAM_RED:
			ent->client->ps.stats[STAT_CROSSHAIR_ID_VIEW_COLOR] = ii_teams_red_tiny;
			break;
		case TEAM_BLUE:
			ent->client->ps.stats[STAT_CROSSHAIR_ID_VIEW_COLOR] = ii_teams_blue_tiny;
			break;
		}
		return;
	}

	// Fallback: use FOV and visibility
	vec3_t dir;
	vec3_t fwd;
	AngleVectors(ent->client->vAngle, fwd, nullptr, nullptr);

	gentity_t *best = nullptr;
	float bestDot = 0.0f;

	for (uint32_t i = 1; i <= game.maxclients; ++i) {
		gentity_t *who = &g_entities[i];
		if (!who->inUse || who->solid == SOLID_NOT)
			continue;

		dir = (who->s.origin - ent->s.origin).normalized();
		float dot = fwd.dot(dir);

		if (Teams() && ent->client->sess.team == who->client->sess.team)
			continue;

		if (dot > bestDot && LocCanSee(ent, who)) {
			bestDot = dot;
			best = who;
		}
	}

	if (bestDot > 0.90f && best) {
		ent->client->ps.stats[STAT_CROSSHAIR_ID_VIEW] = (best - g_entities);

		switch (best->client->sess.team) {
		case TEAM_RED:
			ent->client->ps.stats[STAT_CROSSHAIR_ID_VIEW_COLOR] = ii_teams_red_tiny;
			break;
		case TEAM_BLUE:
			ent->client->ps.stats[STAT_CROSSHAIR_ID_VIEW_COLOR] = ii_teams_blue_tiny;
			break;
		}
	}
}

/*
===============
CTF_SetStats

Sets red/blue flag status icons and scores for the HUD.
===============
*/
static void CTF_SetStats(gentity_t *ent, bool blink) {
	if (notGTF(GTF_CTF))
		return;

	int p1 = ii_teams_red_default;
	int p2 = ii_teams_blue_default;

	// RED FLAG
	gentity_t *redFlag = G_FindByString<&gentity_t::className>(nullptr, ITEM_CTF_FLAG_RED);
	if (redFlag) {
		if (redFlag->solid == SOLID_NOT) {
			p1 = ii_ctf_red_dropped;

			for (uint32_t i = 1; i <= game.maxclients; ++i) {
				if (g_entities[i].inUse &&
					g_entities[i].client->pers.inventory[IT_FLAG_RED]) {
					p1 = ii_ctf_red_taken;
					break;
				}
			}

			if (p1 == ii_ctf_red_dropped) {
				if (!G_FindByString<&gentity_t::className>(redFlag, ITEM_CTF_FLAG_RED)) {
					CTF_ResetTeamFlag(TEAM_RED);
					gi.LocBroadcast_Print(PRINT_HIGH, "$g_flag_returned", Teams_TeamName(TEAM_RED));
					gi.sound(ent, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX,
						gi.soundindex("ctf/flagret.wav"), 1, ATTN_NONE, 0);
				}
			}
		} else if (redFlag->spawnflags.has(SPAWNFLAG_ITEM_DROPPED)) {
			p1 = ii_ctf_red_dropped;
		}
	}

	// BLUE FLAG
	gentity_t *blueFlag = G_FindByString<&gentity_t::className>(nullptr, ITEM_CTF_FLAG_BLUE);
	if (blueFlag) {
		if (blueFlag->solid == SOLID_NOT) {
			p2 = ii_ctf_blue_dropped;

			for (uint32_t i = 1; i <= game.maxclients; ++i) {
				if (g_entities[i].inUse &&
					g_entities[i].client->pers.inventory[IT_FLAG_BLUE]) {
					p2 = ii_ctf_blue_taken;
					break;
				}
			}

			if (p2 == ii_ctf_blue_dropped) {
				if (!G_FindByString<&gentity_t::className>(blueFlag, ITEM_CTF_FLAG_BLUE)) {
					CTF_ResetTeamFlag(TEAM_BLUE);
					gi.LocBroadcast_Print(PRINT_HIGH, "$g_flag_returned", Teams_TeamName(TEAM_BLUE));
					gi.sound(ent, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX,
						gi.soundindex("ctf/flagret.wav"), 1, ATTN_NONE, 0);
				}
			}
		} else if (blueFlag->spawnflags.has(SPAWNFLAG_ITEM_DROPPED)) {
			p2 = ii_ctf_blue_dropped;
		}
	}

	ent->client->ps.stats[STAT_MINISCORE_FIRST_PIC] = p1;
	ent->client->ps.stats[STAT_MINISCORE_SECOND_PIC] = p2;

	// Blink last captured flag
	if (level.ctf_last_flag_capture && level.time - level.ctf_last_flag_capture < 5_sec) {
		if (level.ctf_last_capture_team == TEAM_RED) {
			ent->client->ps.stats[STAT_MINISCORE_FIRST_PIC] = blink ? p1 : 0;
		} else {
			ent->client->ps.stats[STAT_MINISCORE_SECOND_PIC] = blink ? p2 : 0;
		}
	}

	// Team scores
	if (level.matchState == MatchState::MATCH_IN_PROGRESS) {
		ent->client->ps.stats[STAT_MINISCORE_FIRST_SCORE] = level.teamScores[TEAM_RED];
		ent->client->ps.stats[STAT_MINISCORE_SECOND_SCORE] = level.teamScores[TEAM_BLUE];
	}

	// Flag carrier icon for player
	ent->client->ps.stats[STAT_CTF_FLAG_PIC] = 0;

	if (ent->client->sess.team == TEAM_RED &&
		ent->client->pers.inventory[IT_FLAG_BLUE] && blink) {
		ent->client->ps.stats[STAT_CTF_FLAG_PIC] = ii_teams_blue_default;
	} else if (ent->client->sess.team == TEAM_BLUE &&
		ent->client->pers.inventory[IT_FLAG_RED] && blink) {
		ent->client->ps.stats[STAT_CTF_FLAG_PIC] = ii_teams_red_default;
	}
}

/*
===============
SetMiniScoreStats

Populates the miniscore HUD: either 1v1 players or red/blue team stats.
===============
*/
static void SetMiniScoreStats(gentity_t *ent) {
	const bool isTeamGame = Teams() && notGT(GT_RR);
	const bool blink = (level.time.milliseconds() % 1000) < 500;

	int16_t pos1 = -1, pos2 = -1, own = -1;

	const PlayerMedal medalType = ent->client->pers.medalType;

	// Medal HUD display
	if (medalType != PlayerMedal::MEDAL_NONE) {
		const uint32_t count = ent->client->pers.match.medalCount[static_cast<size_t>(medalType)];

		// Only display if this isn't the first time (i.e., show count only if >= 2)
		if (count >= 2 && static_cast<size_t>(medalType) < awardNames.size()) {
			const std::string &medalName = awardNames[static_cast<size_t>(medalType)];
			const std::string medalText = fmt::format("{} (x{})", medalName, count);

			// Set HUD stat string
			ent->client->ps.stats[STAT_MEDAL] = 0;
		} else {
			// Clear HUD stat if count is 1 (first earn)
			ent->client->ps.stats[STAT_MEDAL] = 0;
		}
	} else {
		// No medal active, clear HUD
		ent->client->ps.stats[STAT_MEDAL] = 0;
	}

	// FFA logic
	if (!isTeamGame) {
		int16_t ownRank = -1;
		int16_t other = -1, other2 = -1;

		if (ent->client->sess.team == TEAM_FREE || ent->client->followTarget) {
			own = ent->client->followTarget
				? ent->client->followTarget->s.number - 1
				: ent->s.number - 1;

			ownRank = game.clients[own].pers.currentRank & ~RANK_TIED_FLAG;
		}

		// Find the top two opponents (excluding self)
		for (size_t i = 0; i < MAX_CLIENTS; ++i) {
			int num = level.sorted_clients[i];
			if (num < 0 || num == own || !game.clients[num].pers.connected || !ClientIsPlaying(&game.clients[num]))
				continue;

			if (other < 0) {
				other = num;
				if (ownRank >= 0)
					break;
				continue;
			}

			if (other2 < 0) {
				other2 = num;
				break;
			}
		}

		// Determine display positions
		if (ownRank >= 0) {
			if (ownRank == 0) {
				pos1 = own;
				pos2 = (other >= 0) ? other : other2;
			} else {
				pos1 = (other >= 0) ? other : other2;
				pos2 = own;
			}
		} else {
			pos1 = other;
			pos2 = other2;
		}

		if (GTF(GTF_1V1))
			ent->client->ps.stats[STAT_DUEL_HEADER] = ii_duel_header;

	} else {
		// Team headers
		ent->client->ps.stats[STAT_TEAM_RED_HEADER] = ii_teams_header_red;
		ent->client->ps.stats[STAT_TEAM_BLUE_HEADER] = ii_teams_header_blue;

		// Blink header of winner during intermission
		if (level.intermissionTime && blink) {
			int redScore = level.teamScores[TEAM_RED];
			int blueScore = level.teamScores[TEAM_BLUE];

			if (redScore > blueScore)
				ent->client->ps.stats[STAT_TEAM_RED_HEADER] = 0;
			else if (blueScore > redScore)
				ent->client->ps.stats[STAT_TEAM_BLUE_HEADER] = 0;
			else {
				ent->client->ps.stats[STAT_TEAM_RED_HEADER] = 0;
				ent->client->ps.stats[STAT_TEAM_BLUE_HEADER] = 0;
			}
		}
	}

	// Set scores and icons
	if (GTF(GTF_CTF)) {
		CTF_SetStats(ent, blink);
	} else if (isTeamGame) {
		if (level.matchState == MatchState::MATCH_IN_PROGRESS) {
			ent->client->ps.stats[STAT_MINISCORE_FIRST_PIC] = ii_teams_red_default;
			ent->client->ps.stats[STAT_MINISCORE_SECOND_PIC] = ii_teams_blue_default;
			ent->client->ps.stats[STAT_MINISCORE_FIRST_SCORE] = level.teamScores[TEAM_RED];
			ent->client->ps.stats[STAT_MINISCORE_SECOND_SCORE] = level.teamScores[TEAM_BLUE];
		}

		if (GTF(GTF_ROUNDS)) {
			ent->client->ps.stats[STAT_MINISCORE_FIRST_VAL] = 0;
			ent->client->ps.stats[STAT_MINISCORE_SECOND_VAL] = 0;
		} else {
			ent->client->ps.stats[STAT_MINISCORE_FIRST_VAL] = 0;
			ent->client->ps.stats[STAT_MINISCORE_SECOND_VAL] = 0;
		}
	} else {
		ent->client->ps.stats[STAT_MINISCORE_FIRST_SCORE] = -999;
		ent->client->ps.stats[STAT_MINISCORE_SECOND_SCORE] = -999;

		if (level.matchState == MatchState::MATCH_IN_PROGRESS) {
			if (pos1 >= 0) {
				ent->client->ps.stats[STAT_MINISCORE_FIRST_SCORE] = game.clients[pos1].resp.score;
				ent->client->ps.stats[STAT_MINISCORE_FIRST_PIC] = game.clients[pos1].sess.skinIconIndex;
			}
			if (pos2 >= 0) {
				ent->client->ps.stats[STAT_MINISCORE_SECOND_SCORE] = game.clients[pos2].resp.score;
				ent->client->ps.stats[STAT_MINISCORE_SECOND_PIC] = game.clients[pos2].sess.skinIconIndex;
			}
		} else {
			ent->client->ps.stats[STAT_MINISCORE_FIRST_PIC] = 0;
			ent->client->ps.stats[STAT_MINISCORE_SECOND_PIC] = 0;
		}
	}

	// Highlight own score or team
	ent->client->ps.stats[STAT_MINISCORE_FIRST_POS] = 0;
	ent->client->ps.stats[STAT_MINISCORE_SECOND_POS] = 0;

	if (level.matchState == MatchState::MATCH_IN_PROGRESS) {
		if (isTeamGame) {
			if (ent->client->sess.team == TEAM_RED)
				ent->client->ps.stats[STAT_MINISCORE_FIRST_POS] = ii_highlight;
			else if (ent->client->sess.team == TEAM_BLUE)
				ent->client->ps.stats[STAT_MINISCORE_SECOND_POS] = ii_highlight;
		} else if (own >= 0) {
			if (own == pos1)
				ent->client->ps.stats[STAT_MINISCORE_FIRST_POS] = ii_highlight;
			else if (own == pos2)
				ent->client->ps.stats[STAT_MINISCORE_SECOND_POS] = ii_highlight;
		}
	}
}

/*
===============
SetHealthStats
===============
*/
static void SetHealthStats(gentity_t *ent) {
	if (ent->s.renderfx & RF_USE_DISGUISE) {
		ent->client->ps.stats[STAT_HEALTH_ICON] = level.campaign.disguiseIcon;
	} else {
		switch (ent->client->sess.team) {
		case TEAM_RED:
			ent->client->ps.stats[STAT_HEALTH_ICON] = ii_teams_red_default;
			break;
		case TEAM_BLUE:
			ent->client->ps.stats[STAT_HEALTH_ICON] = ii_teams_blue_default;
			break;
		default:
			ent->client->ps.stats[STAT_HEALTH_ICON] = level.picHealth;
			break;
		}
	}

	ent->client->ps.stats[STAT_HEALTH] = ent->health;
}

/*
===============
SetWeaponStats
===============
*/
static void SetWeaponStats(gentity_t *ent) {
	uint32_t weaponBits = 0;

	for (int invIndex = IT_WEAPON_GRAPPLE; invIndex <= IT_WEAPON_DISRUPTOR; ++invIndex) {
		if (ent->client->pers.inventory[invIndex]) {
			weaponBits |= 1 << GetItemByIndex((item_id_t)invIndex)->weapon_wheel_index;
		}
	}

	ent->client->ps.stats[STAT_WEAPONS_OWNED_1] = weaponBits & 0xFFFF;
	ent->client->ps.stats[STAT_WEAPONS_OWNED_2] = (weaponBits >> 16);

	const Item *weapon = ent->client->newWeapon ? ent->client->newWeapon : ent->client->pers.weapon;
	ent->client->ps.stats[STAT_ACTIVE_WHEEL_WEAPON] = weapon ? weapon->weapon_wheel_index : -1;
	ent->client->ps.stats[STAT_ACTIVE_WEAPON] = ent->client->pers.weapon ? ent->client->pers.weapon->weapon_wheel_index : -1;
}

/*
===============
SetAmmoStats
===============
*/
static void SetAmmoStats(gentity_t *ent) {
	ent->client->ps.stats[STAT_AMMO_ICON] = 0;
	ent->client->ps.stats[STAT_AMMO] = 0;

	const Item *weapon = ent->client->pers.weapon;

	if (weapon && weapon->ammo) {
		Item *ammoItem = GetItemByIndex(weapon->ammo);
		if (!InfiniteAmmoOn(ammoItem)) {
			ent->client->ps.stats[STAT_AMMO_ICON] = gi.imageindex(ammoItem->icon);
			ent->client->ps.stats[STAT_AMMO] = ent->client->pers.inventory[weapon->ammo];
		}
	}

	memset(&ent->client->ps.stats[STAT_AMMO_INFO_START], 0, sizeof(uint16_t) * NUM_AMMO_STATS);

	for (unsigned int ammoIndex = AMMO_BULLETS; ammoIndex < AMMO_MAX; ++ammoIndex) {
		Item *ammo = GetItemByAmmo((ammo_t)ammoIndex);
		if (!ammo)
			continue;

		uint16_t val = InfiniteAmmoOn(ammo)
			? AMMO_VALUE_INFINITE
			: clamp(ent->client->pers.inventory[ammo->id], 0, AMMO_VALUE_INFINITE - 1);

		SetAmmoStat((uint16_t *)&ent->client->ps.stats[STAT_AMMO_INFO_START], ammo->ammo_wheel_index, val);
	}
}

/*
===============
SetArmorStats
===============
*/
static void SetArmorStats(gentity_t *ent) {
	int cells = 0;
	item_id_t armorIndex = ArmorIndex(ent);
	item_id_t powerArmorType = PowerArmorType(ent);

	if (powerArmorType)
		cells = ent->client->pers.inventory[IT_AMMO_CELLS];

	const bool flashPowerArmor = powerArmorType &&
		(!armorIndex || (level.time.milliseconds() % 3000) < 1500);

	if (flashPowerArmor) {
		const char *icon = (powerArmorType == IT_POWER_SHIELD) ? "i_powershield" : "i_powerscreen";
		ent->client->ps.stats[STAT_ARMOR_ICON] = gi.imageindex(icon);
		ent->client->ps.stats[STAT_ARMOR] = cells;
	} else if (armorIndex) {
		const Item *armor = GetItemByIndex(armorIndex);
		ent->client->ps.stats[STAT_ARMOR_ICON] = gi.imageindex(armor->icon);
		ent->client->ps.stats[STAT_ARMOR] = ent->client->pers.inventory[armorIndex];
	} else {
		ent->client->ps.stats[STAT_ARMOR_ICON] = 0;
		ent->client->ps.stats[STAT_ARMOR] = 0;
	}
}

/*
=================
SetPowerupStats

Updates the player's stat array with current powerup statuses.
This includes equipped powerups, active timed effects, and any owned sphere.
=================
*/
static void SetPowerupStats(gentity_t *ent) {
	auto &client = *ent->client;
	auto &stats = client.ps.stats;
	auto &inventory = client.pers.inventory;

	// Clear all powerup stats
	std::fill_n(&stats[STAT_POWERUP_INFO_START], NUM_POWERUP_STATS, 0);

	// Evaluate static or equipped powerups
	for (unsigned int powerupIndex = POWERUP_SCREEN; powerupIndex < POWERUP_MAX; ++powerupIndex) {
		const Item *item = GetItemByPowerup(static_cast<powerup_t>(powerupIndex));
		if (!item)
			continue;

		uint16_t val = 0;

		switch (item->id) {
		case IT_POWER_SCREEN:
		case IT_POWER_SHIELD:
			if (inventory[item->id]) {
				val = (ent->flags & FL_POWER_ARMOR) ? 2 : 1;
			}
			break;

		case IT_FLASHLIGHT:
			if (inventory[item->id]) {
				val = (ent->flags & FL_FLASHLIGHT) ? 2 : 1;
			}
			break;

		default:
			val = clamp(inventory[item->id], 0, 3);
			break;
		}

		SetPowerupStat(reinterpret_cast<uint16_t *>(&stats[STAT_POWERUP_INFO_START]), item->powerup_wheel_index, val);
	}

	// Reset icon and timer initially
	stats[STAT_POWERUP_ICON] = 0;
	stats[STAT_POWERUP_TIME] = 0;

	// If an owned sphere is active, override the HUD icon and timer
	if (client.ownedSphere) {
		int iconIndex = gi.imageindex("i_fixme");
		const auto flags = client.ownedSphere->spawnflags;

		if (flags.has(SF_SPHERE_DEFENDER)) {
			iconIndex = gi.imageindex("p_defender");
		} else if (flags.has(SF_SPHERE_HUNTER)) {
			iconIndex = gi.imageindex("p_hunter");
		} else if (flags.has(SF_SPHERE_VENGEANCE)) {
			iconIndex = gi.imageindex("p_vengeance");
		}

		stats[STAT_POWERUP_ICON] = iconIndex;
		stats[STAT_POWERUP_TIME] = static_cast<int>(std::ceil(client.ownedSphere->wait - level.time.seconds()));
		return;
	}

	// Otherwise, scan for the most relevant active powerup from powerup_table
	const powerup_info_t *best = nullptr;

	for (const auto &powerup : powerup_table) {
		const gtime_t *t = powerup.time_accessor ? &powerup.time_accessor(client) : nullptr;
		const int32_t *c = powerup.count_accessor ? &powerup.count_accessor(client) : nullptr;

		if (t && *t <= level.time)
			continue;
		if (c && *c == 0)
			continue;

		if (!best) {
			best = &powerup;
			continue;
		}

		// Prefer shortest remaining duration
		if (t && best->time_accessor && *t < best->time_accessor(client)) {
			best = &powerup;
		}
		// Prefer count-based powerups over empty entries
		else if (c && !best->time_accessor) {
			best = &powerup;
		}
	}

	if (best) {
		int16_t value = 0;

		if (best->count_accessor)
			value = best->count_accessor(client);
		else if (best->time_accessor)
			value = static_cast<int16_t>(std::ceil((best->time_accessor(client) - level.time).seconds()));

		stats[STAT_POWERUP_ICON] = gi.imageindex(GetItemByIndex(best->item)->icon);
		stats[STAT_POWERUP_TIME] = value;
	}
}

/*
===============
SetSelectedItemStats
===============
*/
static void SetSelectedItemStats(gentity_t *ent) {
	const item_id_t selected = ent->client->pers.selected_item;
	ent->client->ps.stats[STAT_SELECTED_ITEM] = selected;

	if (selected == IT_NULL) {
		ent->client->ps.stats[STAT_SELECTED_ICON] = 0;
	} else {
		ent->client->ps.stats[STAT_SELECTED_ICON] = gi.imageindex(itemList[selected].icon);

		if (ent->client->pers.selected_item_time < level.time) {
			ent->client->ps.stats[STAT_SELECTED_ITEM_NAME] = 0;
		}
	}
}

/*
===============
SetLayoutStats
===============
*/
static void SetLayoutStats(gentity_t *ent) {
	ent->client->ps.stats[STAT_LAYOUTS] = 0;

	if (deathmatch->integer) {
		if (ent->client->pers.health <= 0 || level.intermissionTime || ent->client->showScores)
			ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_LAYOUT;

		if (ent->client->showInventory && ent->client->pers.health > 0)
			ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_INVENTORY;
	} else {
		if (ent->client->showScores || ent->client->showHelp || ent->client->showEOU)
			ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_LAYOUT;

		if (ent->client->showInventory && ent->client->pers.health > 0)
			ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_INVENTORY;

		if (ent->client->showHelp)
			ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_HELP;
	}

	if (level.intermissionTime || ent->client->awaitingRespawn) {
		if (ent->client->awaitingRespawn ||
			level.intermission.endOfUnit ||
			level.isN64 ||
			(deathmatch->integer && (ent->client->showScores || level.intermissionTime))) {
			ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_HIDE_HUD;
		}

		if (level.intermission.endOfUnit || level.isN64 || (deathmatch->integer && level.intermissionTime)) {
			ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_INTERMISSION;
		}
	}

	// Crosshair visibility
	if (deathmatch->integer) {
		if (ClientIsPlaying(ent->client) || !ent->client->followTarget)
			ent->client->ps.stats[STAT_LAYOUTS] &= ~LAYOUTS_HIDE_CROSSHAIR;
		else
			ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_HIDE_CROSSHAIR;
	} else {
		if (level.campaign.story_active)
			ent->client->ps.stats[STAT_LAYOUTS] |= LAYOUTS_HIDE_CROSSHAIR;
		else
			ent->client->ps.stats[STAT_LAYOUTS] &= ~LAYOUTS_HIDE_CROSSHAIR;
	}
}

/*
===============
SetKeyStats
===============
*/
static void SetKeyStats(gentity_t *ent) {
	ent->client->ps.stats[STAT_KEY_A] = 0;
	ent->client->ps.stats[STAT_KEY_B] = 0;
	ent->client->ps.stats[STAT_KEY_C] = 0;

	std::array<item_id_t, IT_TOTAL> keysHeld{};
	size_t numKeys = 0;

	for (const auto &item : itemList) {
		if ((item.flags & IF_KEY) && ent->client->pers.inventory[item.id]) {
			keysHeld[numKeys++] = item.id;
		}
	}

	int keyOffset = 0;
	if (numKeys > 3) {
		keyOffset = static_cast<int>(level.time.seconds() / 5);
	}

	player_stat_t stat = STAT_KEY_A;
	for (size_t i = 0; i < std::min(numKeys, size_t(3)); ++i, stat = static_cast<player_stat_t>(stat + 1)) {
		ent->client->ps.stats[stat] = gi.imageindex(GetItemByIndex(keysHeld[(i + keyOffset) % numKeys])->icon);
	}
}

/*
===============
SetHelpIconStats
===============
*/
static void SetHelpIconStats(gentity_t *ent, bool minhud) {
	if (ent->client->pers.helpchanged >= 1 &&
		ent->client->pers.helpchanged <= 2 &&
		(level.time.milliseconds() % 1000) < 500) {
		ent->client->ps.stats[STAT_HELPICON] = gi.imageindex("i_help");
	} else if ((ent->client->pers.hand == CENTER_HANDED) && ent->client->pers.weapon) {
		if (!minhud || (minhud && ent->client->pers.weapon->id == IT_WEAPON_GRAPPLE)) {
			ent->client->ps.stats[STAT_HELPICON] = gi.imageindex(ent->client->pers.weapon->icon);
		}
	} else {
		ent->client->ps.stats[STAT_HELPICON] = 0;
	}
}

/*
===============
SetHealthBarStats
===============
*/
static void SetHealthBarStats(gentity_t *ent) {
	for (size_t i = 0; i < MAX_HEALTH_BARS; ++i) {
		byte *hb = reinterpret_cast<byte *>(&ent->client->ps.stats[STAT_HEALTH_BARS]) + i;
		auto *e = level.campaign.health_bar_entities[i];

		if (!e) {
			*hb = 0;
			continue;
		}

		if (e->timeStamp) {
			if (e->timeStamp < level.time) {
				level.campaign.health_bar_entities[i] = nullptr;
				*hb = 0;
				continue;
			}

			*hb = 0b10000000; // blinking
			continue;
		}

		if (!e->enemy->inUse || e->enemy->health <= 0) {
			// Special case for Makron double-death hack
			if (e->enemy->monsterInfo.aiFlags & AI_DOUBLE_TROUBLE) {
				*hb = 0b10000000;
				continue;
			}

			if (e->delay) {
				e->timeStamp = level.time + gtime_t::from_sec(e->delay);
				*hb = 0b10000000;
			} else {
				level.campaign.health_bar_entities[i] = nullptr;
				*hb = 0;
			}
			continue;
		}

		if (e->spawnflags.has(SPAWNFLAG_HEALTHBAR_PVS_ONLY) &&
			!gi.inPVS(ent->s.origin, e->enemy->s.origin, true)) {
			*hb = 0;
			continue;
		}

		float healthFrac = (float)e->enemy->health / (float)e->enemy->max_health;
		*hb = ((byte)(healthFrac * 0x7F)) | 0x80;
	}
}

/*
===============
SetTechStats
===============
*/
static void SetTechStats(gentity_t *ent) {
	ent->client->ps.stats[STAT_TECH] = 0;

	for (size_t i = 0; i < q_countof(tech_ids); ++i) {
		if (ent->client->pers.inventory[tech_ids[i]]) {
			ent->client->ps.stats[STAT_TECH] = gi.imageindex(GetItemByIndex(tech_ids[i])->icon);
			break;
		}
	}
}

/*
===============
SetMatchTimerStats
===============
*/
static void SetMatchTimerStats(gentity_t *ent) {
	static int lastTimeEncoded = 0;

	const gtime_t matchTime =
		timelimit->value
		? (level.levelStartTime + gtime_t::from_min(timelimit->value) + level.overtime - level.time)
		: (level.time - level.levelStartTime);

	const int milliseconds = matchTime.milliseconds();
	const int encoded = milliseconds * 1000;

	if (ent->client->last_match_timer_update == encoded)
		return;

	ent->client->last_match_timer_update = encoded;

	const char *s1 = "";
	const char *s2 = "";

	switch (level.matchState) {
	case MatchState::MATCH_WARMUP_DELAYED:
		if (level.warmupNoticeTime + 4_sec > level.time)
			s1 = G_Fmt("{} v{}", GAMEMOD_TITLE, GAMEMOD_VERSION).data();
		else if (level.warmupNoticeTime + 8_sec > level.time)
			s1 = G_Fmt("Ruleset: {}", rs_long_name[(int)game.ruleset]).data();
		break;

	case MatchState::MATCH_NONE:
		break;

	case MatchState::MATCH_WARMUP_DEFAULT:
	case MatchState::MATCH_WARMUP_READYUP:
		s1 = "WARMUP";
		break;

	case MatchState::MATCH_COUNTDOWN:
		s1 = "COUNTDOWN";
		break;

	default:
		if (level.timeoutActive > 0_ms) {
			int t2 = level.timeoutActive.milliseconds();
			s1 = G_Fmt("TIMEOUT! ({})", TimeString(t2, false, false)).data();
		} else if (milliseconds < 0 && milliseconds >= -4000) {
			s1 = "OVERTIME!";
		} else if (GTF(GTF_ROUNDS)) {
			if (level.roundState == RoundState::ROUND_COUNTDOWN) {
				s1 = "COUNTDOWN";
			} else if (level.roundState == RoundState::ROUND_IN_PROGRESS) {
				int t2 = (level.roundStateTimer - level.time).milliseconds();
				s1 = G_Fmt("{} ({})", TimeString(milliseconds, false, false), TimeString(t2, false, false)).data();
			}
		} else {
			if (!level.intermissionQueued && (milliseconds < -1000 || milliseconds > 1000)) {
				s1 = TimeString(milliseconds, false, false);
			}
		}
		break;
	}

	// Additional warmup reason display
	if ((level.matchState == MatchState::MATCH_WARMUP_DEFAULT || level.matchState == MatchState::MATCH_WARMUP_READYUP) &&
		static_cast<uint8_t>(level.warmupState) &&
		level.warmupNoticeTime + 3_sec > level.time) {
		switch (level.warmupState) {
		case WarmupState::WARMUP_REQ_MORE_PLAYERS:
			s2 = G_Fmt(": More players needed ({} players min.)", minplayers->integer).data();
			break;
		case WarmupState::WARMUP_REQ_BALANCE:
			s2 = ": Teams are imbalanced.";
			break;
		case WarmupState::WARMUP_REQ_READYUP:
			s2 = ": Players must ready up.";
			break;
		}
	}

	std::string finalStr = G_Fmt("{}{}", s1, s2).data();
	ent->client->ps.stats[STAT_MATCH_STATE] = CONFIG_MATCH_STATE;
	gi.configstring(CONFIG_MATCH_STATE, finalStr.c_str());
}

/*
===============
SetStats

Central function to set all client HUD stats.
===============
*/
void SetStats(gentity_t *ent) {
	if (!ent || !ent->client)
		return;

	const bool minhud = g_instagib->integer || g_nadefest->integer;

	SetHealthStats(ent);
	if (!minhud) {
		SetWeaponStats(ent);
		SetAmmoStats(ent);
		SetArmorStats(ent);
		SetPowerupStats(ent);
		SetSelectedItemStats(ent);
	}
	SetLayoutStats(ent);
	if (!deathmatch->integer) {
		SetKeyStats(ent);
	}
	SetHelpIconStats(ent, minhud);
	SetHealthBarStats(ent);
	SetTechStats(ent);
	SetMiniScoreStats(ent);

	// Update crosshair ID
	if (ent->client->sess.pc.show_id && !CooperativeModeOn()) {
		SetCrosshairIDView(ent);
	} else {
		ent->client->ps.stats[STAT_CROSSHAIR_ID_VIEW] = 0;
		ent->client->ps.stats[STAT_CROSSHAIR_ID_VIEW_COLOR] = 0;
	}

	// Match state HUD indicator
	ent->client->ps.stats[STAT_COUNTDOWN] = level.countdownTimerCheck.seconds<int>();

	if (ent->client->sess.pc.show_timer)
		SetMatchTimerStats(ent);

	// Medal time blocking FOLLOWING tag
	if (ent->client->pers.medalTime + 3_sec > level.time)
		ent->client->ps.stats[STAT_FOLLOWING] = 0;
	else
		ent->client->ps.stats[STAT_FOLLOWING] = 0;
}

/*
===============
CheckFollowStats

Ensures that any spectators chasing this player get updated HUD stats.
===============
*/
void CheckFollowStats(gentity_t *ent) {
	for (gentity_t *viewer : active_clients()) {
		if (viewer->client->followTarget != ent)
			continue;

		viewer->client->ps.stats = ent->client->ps.stats;
		SetSpectatorStats(viewer);
	}
}

/*
===============
SetSpectatorStats

Sets HUD stats for a spectator.
Includes chase mode and passive spectator support.
===============
*/
void SetSpectatorStats(gentity_t *ent) {
	gclient_t *cl = ent->client;

	if (!cl->followTarget) {
		SetStats(ent);
	}

	// Reset all layouts first
	cl->ps.stats[STAT_LAYOUTS] = 0;

	if (cl->pers.health <= 0 || level.intermissionTime || cl->showScores)
		cl->ps.stats[STAT_LAYOUTS] |= LAYOUTS_LAYOUT;

	if (cl->showInventory && cl->pers.health > 0)
		cl->ps.stats[STAT_LAYOUTS] |= LAYOUTS_INVENTORY;

	if (cl->followTarget && cl->followTarget->inUse) {
		cl->ps.stats[STAT_FOLLOWING] = CONFIG_CHASE_PLAYER_NAME + (cl->followTarget - g_entities) - 1;
		cl->ps.stats[STAT_SPECTATOR] = 0;
	} else {
		cl->ps.stats[STAT_FOLLOWING] = 0;
		cl->ps.stats[STAT_SPECTATOR] = 1;
	}
}
