/*Copyright (c) 2024 ZeniMax Media Inc.
Licensed under the GNU General Public License 2.0.

p_hud_scoreboard.cpp (Player HUD Scoreboard) This file contains the server-side logic for generating the layout strings for the multiplayer scoreboards. It sorts players, gathers their scores and other relevant data, and constructs a formatted string that the client-side game module can parse to render the scoreboard. Key Responsibilities: - `MultiplayerScoreboard`: The main entry point for generating the scoreboard layout. It dispatches to different functions based on the current gametype. - Player Sorting: Implements the logic to sort players based on score for Free-for-All modes and by team and score for team-based modes. - Layout String Construction: Builds the complex string of HUD commands that defines the position, content, and appearance of each element on the scoreboard (player names, scores, pings, icons, etc.). - Gametype-Specific Scoreboards: Contains specialized functions for rendering scoreboards for different modes like FFA, Duel, and Team Deathmatch.*/

#include <utility>

#include "../g_local.hpp"

/*
===============
SortClientsByTeamAndScore
===============
*/
static void SortClientsByTeamAndScore(
	uint8_t sorted[2][MAX_CLIENTS],
	int8_t sortedScores[2][MAX_CLIENTS],
	uint8_t total[2],
	uint8_t totalLiving[2],
	int totalScore[2]
) {
	memset(total, 0, sizeof(uint8_t) * 2);
	memset(totalLiving, 0, sizeof(uint8_t) * 2);
	memset(totalScore, 0, sizeof(int) * 2);

	for (uint32_t i = 0; i < game.maxClients; ++i) {
		gentity_t* cl_ent = g_entities + 1 + i;
		if (!cl_ent->inUse)
			continue;

		int team = -1;
		if (game.clients[i].sess.team == Team::Red)
			team = 0;
		else if (game.clients[i].sess.team == Team::Blue)
			team = 1;

		if (team < 0)
			continue;

		int score = game.clients[i].resp.score;
		int j;
		for (j = 0; j < total[team]; ++j) {
			if (score > sortedScores[team][j])
				break;
		}
		for (int k = total[team]; k > j; --k) {
			sorted[team][k] = sorted[team][k - 1];
			sortedScores[team][k] = sortedScores[team][k - 1];
		}
		sorted[team][j] = i;
		sortedScores[team][j] = score;
		totalScore[team] += score;
		total[team]++;

		if (!game.clients[i].eliminated)
			totalLiving[team]++;
	}
}

// ===========================================================================
// SCOREBOARD MESSAGE HANDLING
// ===========================================================================

enum class SpectatorListMode {
	QueuedOnly,
	PassiveOnly,
	Both
};

enum class PlayerEntryMode {
	FFA,
	Duel,
	Team
};

/*
===============
AppendFormat

Attempts to append a formatted string to the layout. Returns false when
the append would exceed MAX_STRING_CHARS, leaving the layout untouched.
===============
*/
template <typename... Args>
static bool AppendFormat(std::string& layout, fmt::format_string<Args...> fmtStr, Args&&... args)
{
	std::string buffer = fmt::format(fmtStr, std::forward<Args>(args)...);
	if (layout.size() + buffer.size() > MAX_STRING_CHARS)
		return false;

	layout += buffer;
	return true;
}

/*
===============
AddScoreboardHeaderAndFooter

Displays standard header and footer for all scoreboard types.
Includes map name, gametype, score limit, match time, victor string, and optional footer tip.
Sections are omitted when insufficient room remains in the layout buffer.
===============
*/
static void AddScoreboardHeaderAndFooter(std::string& layout, gentity_t* viewer, bool includeFooter = true) {
	const char* limitLabel = "Score Limit";
	if (Game::Has(GameFlags::Rounds) || Game::Has(GameFlags::Elimination)) {
		limitLabel = "Round Limit";
	}
	else if (Game::Has(GameFlags::CTF)) {
		limitLabel = "Capture Limit";
	}
	
	// Header: map and gametype
	if (!AppendFormat(layout,
			"xv 0 yv -40 cstring2 \"{} on '{}'\" "
			"xv 0 yv -30 cstring2 \"{}: {}\" ",
			level.gametype_name.data(), level.longName.data(), limitLabel, GT_ScoreLimit()))
		return;

	if (hostname->string[0] && *hostname->string)
		AppendFormat(layout, "xv 0 yv -50 cstring2 \"{}\" ", hostname->string);

	// During intermission
	if (level.intermission.time) {
		// Match duration
		if (level.levelStartTime && (level.time - level.levelStartTime).seconds() > 0) {
			int duration = (level.intermission.time - level.levelStartTime - 1_sec).milliseconds();
			AppendFormat(layout,
				"xv 0 yv -50 cstring2 \"Total Match Time: {}\" ",
				TimeString(duration, true, false));
		}

		// Victor message
		std::string_view msg(level.intermission.victorMessage.data());
		if (!msg.empty()) {
			AppendFormat(layout,
				"xv 0 yv -10 cstring2 \"{}\" ",
				msg);
		}

		// Press button prompt (5s gate)
		int frameGate = level.intermission.serverFrame + (5_sec).frames();
		AppendFormat(layout,
			"ifgef {} yb -48 xv 0 loc_cstring2 0 \"$m_eou_press_button\" endif ",
			frameGate);
	}
	// During live match
	else if (level.matchState == MatchState::In_Progress && viewer->client && ClientIsPlaying(viewer->client)) {
		if (viewer->client->resp.score > 0 && level.pop.num_playing_clients > 1) {
			AppendFormat(layout,
				"xv 0 yv -10 cstring2 \"{} place with a score of {}\" ",
				PlaceString(viewer->client->pers.currentRank + 1), viewer->client->resp.score);
		}
		if (includeFooter) {
			AppendFormat(layout,
				"xv 0 yb -48 cstring2 \"{}\" ", "Show inventory to toggle menu.");
		}
	}
}



/*
===============
AddSpectatorList

Draws queued players, passive spectators, or both.
Used by all scoreboard modes.
===============
*/
static void AddSpectatorList(std::string& layout, int startY, SpectatorListMode mode) {
	uint32_t y = startY;
	uint8_t lineIndex = 0;
	bool wroteQueued = false;
	bool wroteSpecs = false;

	for (uint32_t i = 0; i < game.maxClients && layout.size() < MAX_STRING_CHARS - 50; ++i) {
		gentity_t* cl_ent = &g_entities[i + 1];
		gclient_t* cl = &game.clients[i];

		if (!cl_ent->inUse || !cl->pers.connected || cl_ent->solid != SOLID_NOT)
			continue;

		const bool isPlaying = ClientIsPlaying(cl);
		const bool isQueued = cl->sess.matchQueued;

		if (mode == SpectatorListMode::QueuedOnly && !isQueued)
			continue;
		if (mode == SpectatorListMode::PassiveOnly && (isQueued || isPlaying))
			continue;
		if (mode == SpectatorListMode::Both && isPlaying)
			continue;

		// Queued header
		if (isQueued && !wroteQueued) {
			if (!AppendFormat(layout,
				"xv 0 yv {} loc_string2 0 \"Queued Contenders:\" "
				"xv -40 yv {} loc_string2 0 \"w  l  name\" ",
				y, y + 8))
				return; // Queued section omitted when the layout buffer is full
			y += 16;
			wroteQueued = true;
		}

		// Spectator header
		if (!isQueued && !wroteSpecs) {
			if (!AppendFormat(layout,
				"xv 0 yv {} loc_string2 0 \"Spectators:\" ", y))
				return; // Spectator section omitted when the layout buffer is full
			y += 8;
			wroteSpecs = true;
		}

		// Draw entry
		std::string_view entry = isQueued
			? G_Fmt("ctf {} {} {} {} {} \"\" ",
				(lineIndex++ & 1) ? 200 : -40, y, i, cl->sess.matchWins, cl->sess.matchLosses).data()
			: G_Fmt("ctf {} {} {} 0 0 \"\" ",
				(lineIndex++ & 1) ? 200 : -40, y, i).data();

		if (layout.size() + entry.size() < MAX_STRING_CHARS) {
			layout += entry;
			if ((lineIndex & 1) == 0)
				y += 8;
		}
	}
}
static int AddDuelistSummary(std::string& layout, int startY) {
	if (!Game::Has(GameFlags::OneVOne))
		return startY;

	std::array<int, 2> duelists{};
	uint8_t found = 0;

	for (int clientIndex : level.sortedClients) {
		if (clientIndex < 0 || clientIndex >= static_cast<int>(game.maxClients))
			continue;

		gclient_t* cl = &game.clients[clientIndex];
		if (!cl->pers.connected || !ClientIsPlaying(cl))
			continue;

		duelists[found++] = clientIndex;

		if (found == duelists.size())
			break;
	}

	if (!found)
		return startY;

	uint32_t y = startY;
	if (!AppendFormat(layout,
		"xv 0 yv {} loc_string2 0 \"Current Duelists:\" "
		"xv -40 yv {} loc_string2 0 \"w  l  name\" ",
		y, y + 8))
		return static_cast<int>(y); // Duelist header omitted when the layout buffer is full
	y += 16;

	uint8_t lineIndex = 0;
	for (uint8_t i = 0; i < found; ++i) {
		int x = (lineIndex & 1) ? 200 : -40;
		gclient_t* cl = &game.clients[duelists[i]];

		std::string_view entry = G_Fmt("ctf {} {} {} {} {} \"\" ",
			x, y, duelists[i], cl->sess.matchWins, cl->sess.matchLosses).data();

		if (layout.size() + entry.size() >= MAX_STRING_CHARS)
			break;

		layout += entry;

		lineIndex++;

		if ((lineIndex & 1) == 0)
			y += 8;
	}

	if (lineIndex & 1)
		y += 8;

	return static_cast<int>(y + 8);
}

/*
===============
AddPlayerEntry

Draws a single player entry row in the scoreboard.
Can be used by all scoreboard types.
===============
*/
static void AddPlayerEntry(
	std::string& layout,
	gentity_t* cl_ent,
	int x, int y,
	PlayerEntryMode mode,
	gentity_t* viewer,
	gentity_t* killer,
	bool isReady,
	const char* flagIcon
) {
	if (!cl_ent || !cl_ent->inUse || !cl_ent->client)
		return;

	gclient_t* cl = cl_ent->client;
	int clientNum = cl_ent->s.number - 1;

	std::string entry;

	// === Tag icon ===
	if (mode == PlayerEntryMode::FFA || mode == PlayerEntryMode::Duel) {
		if (cl_ent == viewer || Game::Is(GameType::RedRover)) {
			const char* tag = (cl->sess.team == Team::Red) ? "/tags/ctf_red" :
				(cl->sess.team == Team::Blue) ? "/tags/ctf_blue" :
				"/tags/default";
			fmt::format_to(std::back_inserter(entry), "xv {} yv {} picn {} ", x, y, tag);
		}
		else if (cl_ent == killer) {
			fmt::format_to(std::back_inserter(entry), "xv {} yv {} picn /tags/bloody ", x, y);
		}
	}
	else if (mode == PlayerEntryMode::Team && flagIcon) {
		fmt::format_to(std::back_inserter(entry), "xv {} yv {} picn {} ", x, y, flagIcon);
	}

	// === Skin icon ===
	if (cl->sess.skinIconIndex > 0) {
		std::string skinPath = G_Fmt("/players/{}_i", cl->sess.skinName).data();
		fmt::format_to(std::back_inserter(entry), "xv {} yv {} picn {} ", x, y, skinPath);
	}

	// === Ready or eliminated marker ===
	if (isReady) {
		fmt::format_to(std::back_inserter(entry), "xv {} yv {} picn wheel/p_compass_selected ", x + 16, y + 16);
	}
	else if (Game::Has(GameFlags::Rounds) && mode == PlayerEntryMode::Team && !cl->eliminated && level.matchState == MatchState::In_Progress) {
		const char* teamIcon = (cl->sess.team == Team::Red) ? "sbfctf1" : "sbfctf2";
		fmt::format_to(std::back_inserter(entry), "xv {} yv {} picn {} ", x + 16, y, teamIcon);
	}

	// === Append entry if it fits ===
	if (layout.size() + entry.size() >= MAX_STRING_CHARS)
		return;
	layout += entry;
	entry.clear();

	// === Score/ping line ===
	fmt::format_to(std::back_inserter(entry),
		"client {} {} {} {} {} {} ",
		x, y, clientNum, cl->resp.score, std::min(cl->ping, 999), 0);

	if (layout.size() + entry.size() >= MAX_STRING_CHARS)
		return;
	layout += entry;

	if (Game::Is(GameType::FreezeTag)) {
		std::string extra;

		if (cl->eliminated) {
			const bool thawing = cl->resp.thawer && cl->freeze.holdDeadline && cl->freeze.holdDeadline > level.time;
			const char* status = thawing ? "THAWING" : "FROZEN";

			fmt::format_to(std::back_inserter(extra),
				"xv {} yv {} string \"{}\" ",
				x + 96, y, status);
		}

		if (cl->resp.thawed > 0) {
			fmt::format_to(std::back_inserter(extra),
				"xv {} yv {} string \"TH:{}\" ",
				x + 96, y + 8, cl->resp.thawed);
		}

		if (!extra.empty()) {
			if (layout.size() + extra.size() >= MAX_STRING_CHARS)
				return;
			layout += extra;
		}
	}
}

/*
===============
AddTeamScoreOverlay
===============
*/
static void AddTeamScoreOverlay(std::string& layout, const uint8_t total[2], const uint8_t totalLiving[2], int teamsize) {
	const bool domination = Game::Is(GameType::Domination);
	const bool proBall = Game::Is(GameType::ProBall);
	const char* scoreLabel = domination ? "PT/points" : (proBall ? "GO" : "SC");

	if (Game::Is(GameType::CaptureTheFlag)) {
		if (!AppendFormat(layout,
			"if 25 xv -32 yv 8 pic 25 endif "
			"xv 0 yv 28 string \"{}/{}\" "
			"xv 58 yv 12 num 2 19 "
			"xv -40 yv 42 string \"{}\" "
			"xv -12 yv 42 picn ping "
			"if 26 xv 208 yv 8 pic 26 endif "
			"xv 240 yv 28 string \"{}/{}\" "
			"xv 296 yv 12 num 2 21 "
			"xv 200 yv 42 string \"{}\" "
			"xv 228 yv 42 picn ping ",
			total[0], teamsize, scoreLabel,
			total[1], teamsize, scoreLabel))
			return; // Team overlay omitted when the layout buffer is full
	}
	else if (Game::Has(GameFlags::Rounds)) {
		if (!AppendFormat(layout,
			"if 25 xv -32 yv 8 pic 25 endif "
			"xv 0 yv 28 string \"{}/{}/{}\" "
			"xv 58 yv 12 num 2 19 "
			"xv -40 yv 42 string \"{}\" "
			"xv -12 yv 42 picn ping "
			"if 26 xv 208 yv 8 pic 26 endif "
			"xv 240 yv 28 string \"{}/{}/{}\" "
			"xv 296 yv 12 num 2 21 "
			"xv 200 yv 42 string \"{}\" "
			"xv 228 yv 42 picn ping ",
			totalLiving[0], total[0], teamsize, scoreLabel,
			totalLiving[1], total[1], teamsize, scoreLabel))
			return; // Team overlay omitted when the layout buffer is full
	}
	else {
		if (!AppendFormat(layout,
			"if 25 xv -32 yv 8 pic 25 endif "
			"xv -123 yv 28 cstring \"{}/{}\" "
			"xv 41 yv 12 num 3 19 "
			"xv -40 yv 42 string \"{}\" "
			"xv -12 yv 42 picn ping "
			"if 26 xv 208 yv 8 pic 26 endif "
			"xv 117 yv 28 cstring \"{}/{}\" "
			"xv 280 yv 12 num 3 21 "
			"xv 200 yv 42 string \"{}\" "
			"xv 228 yv 42 picn ping ",
			total[0], teamsize, scoreLabel,
			total[1], teamsize, scoreLabel))
			return; // Team overlay omitted when the layout buffer is full
	}
}

/*
===============
AddTeamPlayerEntries

Returns the last shown index for the team.
===============
*/
static uint8_t AddTeamPlayerEntries(std::string& layout, int teamIndex, const uint8_t* sorted, uint8_t total, gentity_t* /* killer */) {
	uint8_t lastShown = 0;

	for (uint8_t i = 0; i < total; ++i) {
		uint32_t clientNum = sorted[i];
		if (clientNum < 0 || clientNum >= game.maxClients) continue;

		gentity_t* cl_ent = &g_entities[clientNum + 1];
		gclient_t* cl = &game.clients[clientNum];

		int y = 52 + i * 8;
		int x = (teamIndex == 0) ? -40 : 200;
		bool isReady = (level.matchState == MatchState::Warmup_ReadyUp &&
			(cl->pers.readyStatus || cl->sess.is_a_bot));

		const char* flagIcon = nullptr;
		if (teamIndex == 0 && cl->pers.inventory[IT_FLAG_BLUE])  flagIcon = "sbfctf2";
		if (teamIndex == 1 && cl->pers.inventory[IT_FLAG_RED])   flagIcon = "sbfctf1";

		size_t preSize = layout.size();
		AddPlayerEntry(layout, cl_ent, x, y, PlayerEntryMode::Team, nullptr, nullptr, isReady, flagIcon);

		if (layout.size() != preSize)
			lastShown = i;
	}

	return lastShown;
}

/*
===============
AddSpectatorEntries
===============
*/
static void AddSpectatorEntries(std::string& layout, uint8_t lastRed, uint8_t lastBlue) {
	uint32_t j = ((std::max(lastRed, lastBlue) + 3) * 8) + 42;
	uint32_t lineIndex = 0;
	bool wroteQueued = false, wroteSpecs = false;

	for (uint32_t i = 0; i < game.maxClients && layout.size() < MAX_STRING_CHARS - 50; ++i) {
		gentity_t* cl_ent = &g_entities[i + 1];
		gclient_t* cl = &game.clients[i];

		if (!cl_ent->inUse || cl_ent->solid != SOLID_NOT || ClientIsPlaying(cl))
			continue;

		if (cl->sess.matchQueued) {
			if (!wroteQueued) {
				if (!AppendFormat(layout,
					"xv 0 yv {} loc_string2 0 \"Queued Contenders:\" "
					"xv -40 yv {} loc_string2 0 \"w  l  name\" ",
					j, j + 8))
					return; // Queued spectators omitted when the layout buffer is full
				j += 16;
				wroteQueued = true;
			}

			std::string_view entry = G_Fmt("ctf {} {} {} {} {} \"\" ",
				(lineIndex++ & 1) ? 200 : -40, j, i, cl->sess.matchWins, cl->sess.matchLosses).data();

			if (layout.size() + entry.size() < MAX_STRING_CHARS) {
				layout += entry;
				if ((lineIndex & 1) == 0)
					j += 8;
			}
		}
	}

	// Reset state for non-queued spectators
	lineIndex = 0;
	if (wroteQueued)
		j += 8;

	for (uint32_t i = 0; i < game.maxClients && layout.size() < MAX_STRING_CHARS - 50; ++i) {
		gentity_t* cl_ent = &g_entities[i + 1];
		gclient_t* cl = &game.clients[i];

		if (!cl_ent->inUse || cl_ent->solid != SOLID_NOT || ClientIsPlaying(cl) || cl->sess.matchQueued)
			continue;

		if (!wroteSpecs) {
			if (!AppendFormat(layout,
				"xv 0 yv {} loc_string2 0 \"Spectators:\" ", j))
				return; // Passive spectator section omitted when the layout buffer is full
			j += 8;
			wroteSpecs = true;
		}

		std::string_view entry = G_Fmt("ctf {} {} {} 0 0 \"\" ",
			(lineIndex++ & 1) ? 200 : -40, j, i).data();

		if (layout.size() + entry.size() < MAX_STRING_CHARS) {
			layout += entry;
			if ((lineIndex & 1) == 0)
				j += 8;
		}
	}
}

/*
===============
AddTeamSummaryLine
===============
*/
static void AddTeamSummaryLine(std::string& layout, const uint8_t total[2], const uint8_t lastShown[2]) {
	if (total[0] > lastShown[0] + 1) {
		int y = 42 + (lastShown[0] + 1) * 8;
		if (!AppendFormat(layout,
			"xv -32 yv {} loc_string 1 $g_ctf_and_more {} ", y, total[0] - lastShown[0] - 1))
			return; // Remaining red team members omitted when the layout buffer is full
	}
	if (total[1] > lastShown[1] + 1) {
		int y = 42 + (lastShown[1] + 1) * 8;
		if (!AppendFormat(layout,
			"xv 208 yv {} loc_string 1 $g_ctf_and_more {} ", y, total[1] - lastShown[1] - 1))
			return; // Remaining blue team members omitted when the layout buffer is full
	}
}

/*
===============
TeamsScoreboardMessage
===============
*/
void TeamsScoreboardMessage(gentity_t* ent, gentity_t* killer) {
	std::string layout;
	uint8_t sorted[2][MAX_CLIENTS] = {};
	int8_t sortedScores[2][MAX_CLIENTS] = {};
	uint8_t total[2] = {}, totalLiving[2] = {};
	int totalScore[2] = {};
	uint8_t lastShown[2] = {};
	int teamsize = floor(maxplayers->integer / 2);

	SortClientsByTeamAndScore(sorted, sortedScores, total, totalLiving, totalScore);

	AddScoreboardHeaderAndFooter(layout, ent);
	AddTeamScoreOverlay(layout, total, totalLiving, teamsize);

	uint8_t lastRed = AddTeamPlayerEntries(layout, 0, sorted[0], total[0], killer);
	uint8_t lastBlue = AddTeamPlayerEntries(layout, 1, sorted[1], total[1], killer);

	lastShown[0] = lastRed;
	lastShown[1] = lastBlue;

	AddTeamSummaryLine(layout, total, lastShown);
	int startY = ((std::max(lastRed, lastBlue) + 3) * 8) + 42;
	AddSpectatorList(layout, startY, SpectatorListMode::Both);

	gi.WriteByte(svc_layout);
	gi.WriteString(layout.c_str());
}

/*
===============
DuelScoreboardMessage
===============
*/
static void DuelScoreboardMessage(gentity_t* ent, gentity_t* killer) {
	std::string layout;
	AddScoreboardHeaderAndFooter(layout, ent);
	int spectatorStart = AddDuelistSummary(layout, 42);
	if (spectatorStart == 42)
		spectatorStart = 58;
	AddSpectatorList(layout, spectatorStart, SpectatorListMode::Both);

	gi.WriteByte(svc_layout);
	gi.WriteString(layout.c_str());
}

/*
===============
DeathmatchScoreboardMessage
===============
*/
void DeathmatchScoreboardMessage(gentity_t* ent, gentity_t* killer) {
	if (Teams() && Game::IsNot(GameType::RedRover)) {
		TeamsScoreboardMessage(ent, ent->enemy);
		return;
	}
	if (Game::Has(GameFlags::OneVOne)) {
		DuelScoreboardMessage(ent, ent->enemy);
		return;
	}

	uint8_t total = std::min<uint8_t>(level.pop.num_playing_clients, 16);
	std::string layout;

	for (size_t i = 0; i < total; ++i) {
		uint32_t clientNum = level.sortedClients[i];
		if (clientNum < 0 || clientNum >= game.maxClients)
			continue;

		gclient_t* cl = &game.clients[clientNum];
		gentity_t* cl_ent = &g_entities[clientNum + 1];

		if (!ClientIsPlaying(cl))
			continue;

		int x = (i >= 8) ? 130 : -72;
		int y = 32 * (i % 8);
		AddPlayerEntry(layout, cl_ent, x, y, PlayerEntryMode::FFA, ent, killer, cl->pers.readyStatus, nullptr);
	}
	AddScoreboardHeaderAndFooter(layout, ent);

	gi.WriteByte(svc_layout);
	gi.WriteString(layout.c_str());
}

/*
===============
MultiplayerScoreboard

Displays the scoreboard instead of the help screen.
Note that it isn't that hard to overflow the 1400 byte message limit!
===============
*/
void MultiplayerScoreboard(gentity_t* ent) {
	gentity_t* target = ent->client->follow.target ? ent->client->follow.target : ent;

	DeathmatchScoreboardMessage(target, target->enemy);

	gi.unicast(ent, true);
	ent->client->menu.updateTime = level.time + 3_sec;
}
