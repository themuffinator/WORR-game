#include "g_local.h"
/*----------------------------------------------------------------*/

bool TeamSkillShuffle();

static bool Vote_Val_None(gentity_t *ent) {
	return true;
}

void Vote_Pass_Map() {
	const MapEntry *map = game.mapSystem.GetMapEntry(level.vote.arg);
	if (!map) {
		gi.Com_Print("Error: Map not found in pool at vote pass stage.\n");
		return;
	}

	level.changeMap = map->filename.c_str();
	game.overrideEnableFlags = level.vote_flags_enable;
	game.overrideDisableFlags = level.vote_flags_disable;

	ExitLevel();
}

static bool Vote_Val_Map(gentity_t *ent) {
	if (gi.argc() < 3 || !Q_strcasecmp(gi.argv(1), "?")) {
		PrintMapList(ent, false);
		return false;
	}

	const char *mapName = gi.argv(2);
	const MapEntry *map = game.mapSystem.GetMapEntry(mapName);
	if (!map) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Map '{}' not found in map pool.\n", mapName);
		PrintMapList(ent, false);
		return false;
	}

	if (map->lastPlayed) {
		int64_t timeSince = GetCurrentRealTimeMillis() - map->lastPlayed;
		if (timeSince < 1800000) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Map '{}' was played recently, please wait {}.\n", mapName, FormatDuration(1800000 - timeSince).c_str());
			return false;
		}
	}

	// Store map name and flags in level vars
	level.vote.arg = map->filename;

	// Parse override flags
	uint8_t enableFlags = 0, disableFlags = 0;
	std::vector<std::string> flags;
	for (int i = 3; i < gi.argc(); ++i)
		flags.emplace_back(gi.argv(i));

	if (!ParseMyMapFlags(flags, enableFlags, disableFlags)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid flag(s) used. Use +pu -fd etc.\n");
		return false;
	}

	level.vote_flags_enable = enableFlags;
	level.vote_flags_disable = disableFlags;

	return true;
}

void Vote_Pass_RestartMatch() {
	Match_Reset();
}

void Vote_Pass_Gametype() {
	gametype_t gt = GametypeStringToIndex(level.vote.arg.data());
	if (gt == GT_NONE)
		return;
	
	ChangeGametype(gt);
}

static bool Vote_Val_Gametype(gentity_t *ent) {
	if (GametypeStringToIndex(gi.argv(2)) == gametype_t::GT_NONE) {
		gi.Client_Print(ent, PRINT_HIGH, "Invalid gametype.\n");
		return false;
	}

	return true;
}

static void Vote_Pass_Ruleset() {
	ruleset_t rs = RS_IndexFromString(level.vote.arg.data());
	if (rs == ruleset_t::RS_NONE)
		return;

	gi.cvar_forceset("g_ruleset", G_Fmt("{}", (int)rs).data());
}

static bool Vote_Val_Ruleset(gentity_t *ent) {
	ruleset_t desired_rs = RS_IndexFromString(gi.argv(2));
	if (desired_rs == ruleset_t::RS_NONE) {
		gi.Client_Print(ent, PRINT_HIGH, "Invalid ruleset.\n");
		return false;
	}
	if ((int)desired_rs == game.ruleset) {
		gi.Client_Print(ent, PRINT_HIGH, "Ruleset currently active.\n");
		return false;
	}

	return true;
}

void Vote_Pass_NextMap() {
	// Play queue overrides auto-select
	if (!game.mapSystem.playQueue.empty()) {
		const auto &queued = game.mapSystem.playQueue.front();
		level.changeMap = queued.filename.c_str();
		game.overrideEnableFlags = queued.settings.to_ulong();
		game.overrideDisableFlags = 0;
		ExitLevel();
		return;
	}

	auto result = AutoSelectNextMap();
	if (result.has_value()) {
		level.changeMap = result->filename.c_str();
		game.overrideEnableFlags = 0;
		game.overrideDisableFlags = 0;
		ExitLevel();
	} else {
		gi.Broadcast_Print(PRINT_HIGH, "No eligible maps available.\n");
	}
}

void Vote_Pass_ShuffleTeams() {
	TeamSkillShuffle();
	//Match_Reset();
	gi.Broadcast_Print(PRINT_HIGH, "Teams have been shuffled.\n");
}

static bool Vote_Val_ShuffleTeams(gentity_t *ent) {
	if (!Teams())
		return false;

	return true;
}

static void Vote_Pass_Unlagged() {
	int argi = strtoul(level.vote.arg.data(), nullptr, 10);
	
	gi.LocBroadcast_Print(PRINT_HIGH, "Lag compensation has been {}.\n", argi ? "ENABLED" : "DISABLED");

	gi.cvar_forceset("g_lagCompensation", argi ? "1" : "0");
}

static bool Vote_Val_Unlagged(gentity_t *ent) {
	int arg = strtoul(gi.argv(2), nullptr, 10);
	
	if ((g_lagCompensation->integer && arg)
			|| (!g_lagCompensation->integer && !arg)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Lag compensation is already {}.\n", arg ? "ENABLED" : "DISABLED");
		return false;
	}

	return true;
}

static bool Vote_Val_Random(gentity_t *ent) {
	int arg = strtoul(gi.argv(2), nullptr, 10);

	if (arg > 100 || arg < 2)
		return false;

	return true;
}

void Vote_Pass_Cointoss() {
	gi.LocBroadcast_Print(PRINT_HIGH, "The coin is: {}\n", brandom() ? "HEADS" : "TAILS");
}

void Vote_Pass_Random() {
	gi.LocBroadcast_Print(PRINT_HIGH, "The random number is: {}\n", irandom(2, atoi(level.vote.arg.data())));
}

void Vote_Pass_Timelimit() {
	const char *s = level.vote.arg.data();
	int argi = strtoul(s, nullptr, 10);
	
	if (!argi)
		gi.Broadcast_Print(PRINT_HIGH, "Time limit has been DISABLED.\n");
	else
		gi.LocBroadcast_Print(PRINT_HIGH, "Time limit has been set to {}.\n", TimeString(argi * 60000, false, false));

	gi.cvar_forceset("timelimit", s);
}

static bool Vote_Val_Timelimit(gentity_t *ent) {
	int argi = strtoul(gi.argv(2), nullptr, 10);
	
	if (argi < 0 || argi > 1440) {
		gi.Client_Print(ent, PRINT_HIGH, "Invalid time limit value.\n");
		return false;
	}
	
	if (argi == timelimit->integer) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Time limit is already set to {}.\n", TimeString(argi * 60000, false, false));
		return false;
	}
	return true;
}

void Vote_Pass_Scorelimit() {
	int argi = strtoul(level.vote.arg.data(), nullptr, 10);
	
	if (argi)
		gi.LocBroadcast_Print(PRINT_HIGH, "Score limit has been set to {}.\n", argi);
	else
		gi.Broadcast_Print(PRINT_HIGH, "Score limit has been DISABLED.\n");

	gi.cvar_forceset(G_Fmt("{}limit", GT_ScoreLimitString()).data(), level.vote.arg.data());
}

static bool Vote_Val_Scorelimit(gentity_t *ent) {
	int argi = strtoul(gi.argv(2), nullptr, 10);
	
	if (argi < 0) {
		gi.Client_Print(ent, PRINT_HIGH, "Invalid score limit value.\n");
		return false;
	}

	if (argi == GT_ScoreLimit()) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Score limit is already set to {}.\n", argi);
		return false;
	}

	return true;
}

/*
=================
Vote_Pass_Arena
=================
*/
static void Vote_Pass_Arena() {
	int argi = strtoul(level.vote.arg.data(), nullptr, 10);

	if (!ChangeArena(argi))
		return;

	gi.LocBroadcast_Print(PRINT_HIGH, "Active arena changed to {}.\n", level.arenaActive);
}

/*
=================
Vote_Val_Arena
=================
*/
static bool Vote_Val_Arena(gentity_t *ent) {

	if (!level.arenaTotal) {
		gi.Client_Print(ent, PRINT_HIGH, "No arenas present in current map.\n");
		return false;
	}

	if (gi.argc() < 2 || !Q_strcasecmp(gi.argv(1), "?")) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Active arena is: {}\nTotal arenas: {}\n", level.arenaActive, level.arenaTotal);
		return false;
	}

	int argi = strtoul(gi.argv(2), nullptr, 10);

	if (argi == level.arenaActive) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Arena {} is already active.\n", argi);
		return false;
	}

	if (!CheckArenaValid(argi)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid arena number: {}\n", argi);
		return false;
	}
	return true;
}

static void Vote_Pass_BalanceTeams() {
	TeamBalance(true);
}

static bool Vote_Val_BalanceTeams(gentity_t *ent) {
	if (!Teams())
		return false;

	return true;
}

std::vector<VoteCommand> vote_cmds = {
	{ "map",        Vote_Val_Map,          Vote_Pass_Map,          1,     2, "<mapname>",                         "Changes to the specified map" },
	{ "nextMap",    Vote_Val_None,         Vote_Pass_NextMap,      2,     1, "",                                 "Moves to the next map in the rotation" },
	{ "restart",    Vote_Val_None,         Vote_Pass_RestartMatch, 4,     1, "",                                 "Restarts the current match" },
	{ "gametype",   Vote_Val_Gametype,     Vote_Pass_Gametype,     8,     2, "<ffa|duel|tdm|ctf|ca|ft|horde|gauntlet>", "Changes the current gametype" },
	{ "timelimit",  Vote_Val_Timelimit,    Vote_Pass_Timelimit,    16,    2, "<0..$>",                            "Alters the match time limit, 0 for no time limit" },
	{ "scorelimit", Vote_Val_Scorelimit,   Vote_Pass_Scorelimit,   32,    2, "<0..$>",                            "Alters the match score limit, 0 for no score limit" },
	{ "shuffle",    Vote_Val_ShuffleTeams, Vote_Pass_ShuffleTeams, 64,    2, "",                                 "Shuffles teams" },
	{ "unlagged",   Vote_Val_Unlagged,     Vote_Pass_Unlagged,     128,   2, "<0/1>",                             "Enables or disables lag compensation" },
	{ "cointoss",   Vote_Val_None,         Vote_Pass_Cointoss,     256,   1, "",                                 "Invokes a HEADS or TAILS cointoss" },
	{ "random",     Vote_Val_Random,       Vote_Pass_Random,       512,   1, "<2-100>",                           "Randomly selects a number from 2 to specified value" },
	{ "balance",    Vote_Val_BalanceTeams, Vote_Pass_BalanceTeams, 1024,  1, "",                                 "Balances teams without shuffling" },
	{ "ruleset",    Vote_Val_Ruleset,      Vote_Pass_Ruleset,      2048,  2, "<q1/q2/q3a>",                       "Changes the current ruleset" },
	{ "arena",      Vote_Val_Arena,        Vote_Pass_Arena,        2096,  2, "<num>",                             "Changes the active arena in RA2 maps" }
};

/*
===============
FindVoteCmdByName
===============
*/
static VoteCommand *FindVoteCmdByName(std::string_view name) {
	for (auto &cmd : vote_cmds) {
		if (cmd.name == name)
			return &cmd;
	}
	return nullptr;
}

/*
==================
Vote_Passed
==================
*/
void Vote_Passed() {
	if (level.vote.cmd && level.vote.cmd->execute)
		level.vote.cmd->execute();

	level.vote.cmd = nullptr;
	level.vote.arg.clear();
	level.vote.executeTime = 0_sec;
}

/*
=================
ValidVoteCommand
=================
*/
static bool ValidVoteCommand(gentity_t *ent) {
	if (!ent || !ent->client)
		return false;

	level.vote.cmd = nullptr;

	const std::string cmdName = gi.argv(1);
	const VoteCommand *cmd = FindVoteCmdByName(cmdName);
	if (!cmd) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid vote command: {}\n", cmdName.c_str());
		return false;
	}

	// Check for minimum argument count
	if (cmd->minArgs > 0 && gi.argc() < 1 + cmd->minArgs) {
		gi.LocClient_Print(ent, PRINT_HIGH,
			"{}: {}\nUsage: {} {}\n",
			cmd->name.data(), cmd->helpText.data(), cmd->name.data(), cmd->argsUsage.data());
		return false;
	}

	// Grab the first argument if present
	std::string arg = (gi.argc() > 2) ? gi.argv(2) : "";

	if (!cmd->validate(ent))
		return false;

	level.vote.cmd = cmd;
	level.vote.arg = arg;
	return true;
}

/*
=================
VoteCommandStore
=================
*/
void VoteCommandStore(gentity_t *ent) {
	// start the voting, the caller automatically votes yes
	level.vote.client = ent->client;
	level.vote.time = level.time;
	level.vote.countYes = 1;
	level.vote.countNo = 0;
	
	gi.LocBroadcast_Print(PRINT_CENTER, "{} called a vote:\n{}{}\n", level.vote.client->sess.netName, level.vote.cmd->name.data(), level.vote.arg[0] ? G_Fmt(" {}", level.vote.arg).data() : "");

	for (auto ec : active_clients())
		ec->client->pers.voted = ec == ent ? 1 : 0;

	ent->client->pers.vote_count++;
	AnnouncerSound(world, "vote_now");

	for (auto ec : active_players()) {
		if (ec->svFlags & SVF_BOT)
			continue;

		//gi.local_sound(ec, CHAN_AUTO, gi.soundindex("misc/pc_up.wav"), 1, ATTN_NONE, 0);

		if (ec->client == level.vote.client)
			continue;

		if (!ClientIsPlaying(ec->client) && !g_allowSpecVote->integer)
			continue;

		ec->client->showInventory = false;
		ec->client->showHelp = false;
		ec->client->showScores = false;
		gentity_t *e = ec->client->followTarget ? ec->client->followTarget : ec;
		ec->client->ps.stats[STAT_SHOW_STATUSBAR] = !ClientIsPlaying(e->client) ? 0 : 1;
		CloseActiveMenu(ec);
		OpenVoteMenu(ec);
	}
}

bool TryStartVote(gentity_t *ent, const std::string &name, const std::string &arg, bool fromMenu) {
	if (!ent || !ent->client || !deathmatch->integer)
		return false;

	// Build valid vote command list (for optional usage message)
	std::string validVotes;
	for (const auto &vc : vote_cmds) {
		if (!vc.name.empty() && !(g_vote_flags->integer & vc.flag)) {
			validVotes += vc.name;
			validVotes += " ";
		}
	}

	if (!g_allowVoting->integer || validVotes.empty()) {
		if (!fromMenu)
			gi.Client_Print(ent, PRINT_HIGH, "Voting not allowed here.\n");
		return false;
	}

	if (!g_allowVoteMidGame->integer && level.matchState >= MatchState::MATCH_COUNTDOWN) {
		if (!fromMenu)
			gi.Client_Print(ent, PRINT_HIGH, "Voting is only allowed during the warm up period.\n");
		return false;
	}

	if (level.vote.time) {
		if (!fromMenu)
			gi.Client_Print(ent, PRINT_HIGH, "A vote is already in progress.\n");
		return false;
	}

	if (level.vote.executeTime || level.restarted) {
		if (!fromMenu)
			gi.Client_Print(ent, PRINT_HIGH, "Previous vote command is still awaiting execution.\n");
		return false;
	}

	if (!g_allowSpecVote->integer && !ClientIsPlaying(ent->client)) {
		if (!fromMenu)
			gi.Client_Print(ent, PRINT_HIGH, "You are not allowed to call a vote as a spectator.\n");
		return false;
	}

	if (g_vote_limit->integer && ent->client->pers.vote_count >= g_vote_limit->integer) {
		if (!fromMenu)
			gi.LocClient_Print(ent, PRINT_HIGH,
				"You have called the maximum number of votes ({}).\n",
				g_vote_limit->integer);
		return false;
	}

	// Find matching vote command
	for (const auto &vc : vote_cmds) {
		if (vc.name.empty() || vc.name == name)
			continue;

		if (vc.validate && !vc.validate(ent))
			return false;

		level.vote.cmd = &vc;
		level.vote.arg = arg;

		VoteCommandStore(ent);
		return true;
	}

	// Unknown vote command
	if (!fromMenu) {
		gi.LocClient_Print(ent, PRINT_HIGH,
			"Usage: callvote <command> <params>\nValid Voting Commands: {}\n",
			validVotes.c_str());
	}
	return false;
}

void G_RevertVote(gclient_t *client) {
	if (!level.vote.time)
		return;

	if (!level.vote.client)
		return;

	if (client->pers.voted == 1) {
		level.vote.countYes--;
		client->pers.voted = 0;
	} else if (client->pers.voted == -1) {
		level.vote.countNo--;
		client->pers.voted = 0;
	}
}
