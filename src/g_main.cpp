// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#pragma once
#include "g_local.h"
#include "bots/bot_includes.h"

//#include <unordered_set>
#include <fstream>
#include <sstream>
//#include <cctype>

CHECK_GCLIENT_INTEGRITY;
CHECK_ENTITY_INTEGRITY;

constexpr int32_t DEFAULT_GRAPPLE_SPEED = 750; // speed of grapple in flight
constexpr float	  DEFAULT_GRAPPLE_PULL_SPEED = 750; // speed player is pulled at

std::mt19937 mt_rand;

game_locals_t  game;
level_locals_t level;

local_game_import_t  gi;

/*static*/ char local_game_import_t::print_buffer[0x10000];

/*static*/ std::array<char[MAX_INFO_STRING], MAX_LOCALIZATION_ARGS> local_game_import_t::buffers;
/*static*/ std::array<const char *, MAX_LOCALIZATION_ARGS> local_game_import_t::buffer_ptrs;

game_export_t  globals;
spawn_temp_t   st;

cached_modelindex		sm_meat_index;
cached_soundindex		snd_fry;

gentity_t *g_entities;

cvar_t *hostname;

cvar_t *deathmatch;
cvar_t *ctf;
cvar_t *teamplay;
cvar_t *g_gametype;

cvar_t *coop;

cvar_t *skill;
cvar_t *fraglimit;
cvar_t *capturelimit;
cvar_t *timelimit;
cvar_t *roundlimit;
cvar_t *roundtimelimit;
cvar_t *mercylimit;
cvar_t *noplayerstime;

cvar_t *g_ruleset;

cvar_t *password;
cvar_t *spectator_password;
cvar_t *admin_password;
cvar_t *needpass;
cvar_t *filterban;

static cvar_t *maxclients;
static cvar_t *maxentities;
cvar_t *maxplayers;
cvar_t *minplayers;

cvar_t *ai_allow_dm_spawn;
cvar_t *ai_damage_scale;
cvar_t *ai_model_scale;
cvar_t *ai_movement_disabled;
cvar_t *bob_pitch;
cvar_t *bob_roll;
cvar_t *bob_up;
cvar_t *bot_debug_follow_actor;
cvar_t *bot_debug_move_to_point;
cvar_t *flood_msgs;
cvar_t *flood_persecond;
cvar_t *flood_waitdelay;
cvar_t *gun_x, *gun_y, *gun_z;
cvar_t *run_pitch;
cvar_t *run_roll;

cvar_t *g_airAccelerate;
cvar_t *g_allowAdmin;
cvar_t *g_allowCustomSkins;
cvar_t *g_allowForfeit;
cvar_t *g_allow_grapple;
cvar_t *g_allow_kill;
cvar_t *g_allowMymap;
cvar_t *g_allowSpecVote;
cvar_t *g_allowTechs;
cvar_t *g_allowVoteMidGame;
cvar_t *g_allowVoting;
cvar_t *g_arenaSelfDmgArmor;
cvar_t *g_arenaStartingArmor;
cvar_t *g_arenaStartingHealth;
cvar_t *g_cheats;
cvar_t *g_coop_enable_lives;
cvar_t *g_coop_health_scaling;
cvar_t *g_coop_instanced_items;
cvar_t *g_coop_num_lives;
cvar_t *g_coop_player_collision;
cvar_t *g_coop_squad_respawn;
cvar_t *g_damage_scale;
cvar_t *g_debug_monster_kills;
cvar_t *g_debug_monster_paths;
cvar_t *g_dedicated;
cvar_t *g_disable_player_collision;
cvar_t *match_startNoHumans;
cvar_t *match_autoJoin;
cvar_t *match_crosshairIDs;
cvar_t *warmup_doReadyUp;
cvar_t *warmup_enabled;
cvar_t *g_dm_exec_level_cfg;
cvar_t *match_forceJoin;
cvar_t *match_doForceRespawn;
cvar_t *match_forceRespawnTime;
cvar_t *match_holdableAdrenaline;
cvar_t *match_instantItems;
cvar_t *owner_intermissionShots;
cvar_t *match_itemsRespawnRate;
cvar_t *g_fallingDamage;
cvar_t *g_selfDamage;
cvar_t *match_doOvertime;
cvar_t *match_powerupDrops;
cvar_t *match_powerupMinPlayerLock;
cvar_t *g_dm_random_items;
cvar_t *match_playerRespawnMinDelay;
cvar_t *match_playerRespawnMinDistance;
cvar_t *match_playerRespawnMinDistanceDebug;
cvar_t *match_map_sameLevel;
cvar_t *match_allowSpawnPads;
cvar_t *g_dm_strong_mines;
cvar_t *match_allowTeleporterPads;
cvar_t *match_timeoutLength;
cvar_t *match_weaponsStay;
cvar_t *match_dropCmdFlags;
cvar_t *g_entityOverrideDir;
cvar_t *g_entityOverrideLoad;
cvar_t *g_entityOverrideSave;
cvar_t *g_eyecam;
cvar_t *g_fastDoors;
cvar_t *g_frag_messages;
cvar_t *g_frenzy;
cvar_t *g_friendlyFireScale;
cvar_t *g_frozen_time;
cvar_t *g_grapple_damage;
cvar_t *g_grapple_fly_speed;
cvar_t *g_grapple_offhand;
cvar_t *g_grapple_pull_speed;
cvar_t *g_gravity;
cvar_t *g_horde_starting_wave;
cvar_t *g_huntercam;
cvar_t *g_inactivity;
cvar_t *g_infiniteAmmo;
cvar_t *g_instagib;
cvar_t *g_instagib_splash;
cvar_t *g_instantWeaponSwitch;
cvar_t *g_itemBobbing;
cvar_t *g_knockbackScale;
cvar_t *g_ladderSteps;
cvar_t *g_lagCompensation;
cvar_t *match_levelRulesets;
cvar_t *match_maps_list;
cvar_t *match_maps_listShuffle;
cvar_t *match_lock;
cvar_t *g_matchstats;
cvar_t *g_maxvelocity;
cvar_t *g_motd_filename;
cvar_t *g_mover_debug;
cvar_t *g_mover_speed_scale;
cvar_t *g_nadefest;
cvar_t *g_no_armor;
cvar_t *g_mapspawn_no_bfg;
cvar_t *g_mapspawn_no_plasmabeam;
cvar_t *g_no_health;
cvar_t *g_no_items;
cvar_t *g_no_mines;
cvar_t *g_no_nukes;
cvar_t *g_no_powerups;
cvar_t *g_no_spheres;
cvar_t *g_owner_auto_join;
cvar_t *g_owner_push_scores;
cvar_t *g_quadhog;
cvar_t *g_quick_weapon_switch;
cvar_t *g_rollangle;
cvar_t *g_rollspeed;
cvar_t *g_select_empty;
cvar_t *g_showhelp;
cvar_t *g_showmotd;
cvar_t *g_skip_view_modifiers;
cvar_t *g_start_items;
cvar_t *g_starting_health;
cvar_t *g_starting_health_bonus;
cvar_t *g_starting_armor;
cvar_t *g_stopspeed;
cvar_t *g_strict_saves;
cvar_t *g_teamplay_allow_team_pick;
cvar_t *g_teamplay_armor_protect;
cvar_t *g_teamplay_auto_balance;
cvar_t *g_teamplay_force_balance;
cvar_t *g_teamplay_item_drop_notice;
cvar_t *g_vampiric_damage;
cvar_t *g_vampiric_exp_min;
cvar_t *g_vampiric_health_max;
cvar_t *g_vampiric_percentile;
cvar_t *g_verbose;
cvar_t *g_vote_flags;
cvar_t *g_vote_limit;
cvar_t *g_warmup_countdown;
cvar_t *g_warmup_ready_percentage;
cvar_t *g_weapon_projection;
cvar_t *g_weapon_respawn_time;

cvar_t *g_maps_pool_file;
cvar_t *g_maps_cycle_file;
cvar_t *g_maps_selector;
cvar_t *g_maps_mymap;
cvar_t *g_maps_allow_custom_textures;
cvar_t *g_maps_allow_custom_sounds;

cvar_t *g_statex_enabled;
cvar_t *g_statex_humans_present;

cvar_t *g_blueTeamName;
cvar_t *g_redTeamName;

cvar_t *bot_name_prefix;

static cvar_t *g_framesPerFrame;

int ii_duel_header;
int ii_highlight;
int ii_ctf_red_dropped;
int ii_ctf_blue_dropped;
int ii_ctf_red_taken;
int ii_ctf_blue_taken;
int ii_teams_red_default;
int ii_teams_blue_default;
int ii_teams_red_tiny;
int ii_teams_blue_tiny;
int ii_teams_header_red;
int ii_teams_header_blue;
int mi_ctf_red_flag, mi_ctf_blue_flag; // [Paril-KEX]

void ClientThink(gentity_t *ent, usercmd_t *cmd);
gentity_t *ClientChooseSlot(const char *userInfo, const char *socialID, bool isBot, gentity_t **ignore, size_t num_ignore, bool cinematic);
bool ClientConnect(gentity_t *ent, char *userInfo, const char *socialID, bool isBot);
char *WriteGameJson(bool autosave, size_t *out_size);
void ReadGameJson(const char *jsonString);
char *WriteLevelJson(bool transition, size_t *out_size);
void ReadLevelJson(const char *jsonString);
bool CanSave();
void ClientDisconnect(gentity_t *ent);
void ClientBegin(gentity_t *ent);
void ClientCommand(gentity_t *ent);
void G_RunFrame(bool main_loop);
void G_PrepFrame();
void G_InitSave();

#include <chrono>

// =================================================

void LoadMotd() {
	// load up ent override
	const char *name = G_Fmt("baseq2/{}", g_motd_filename->string[0] ? g_motd_filename->string : "motd.txt").data();
	FILE *f = fopen(name, "rb");
	bool valid = true;
	if (f != NULL) {
		char *buffer = nullptr;
		size_t length;
		size_t read_length;

		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);

		if (length > 0x40000) {
			gi.Com_PrintFmt("{}: MoTD file length exceeds maximum: \"{}\"\n", __FUNCTION__, name);
			valid = false;
		}
		if (valid) {
			buffer = (char *)gi.TagMalloc(length + 1, '\0');
			if (length) {
				read_length = fread(buffer, 1, length, f);

				if (length != read_length) {
					gi.Com_PrintFmt("{}: MoTD file read error: \"{}\"\n", __FUNCTION__, name);
					valid = false;
				}
			}
		}
		fclose(f);
		
		if (valid) {
			game.motd = (const char *)buffer;
			game.motdModificationCount++;
			if (g_verbose->integer)
				gi.Com_PrintFmt("{}: MotD file verified and loaded: \"{}\"\n", __FUNCTION__, name);
		} else {
			gi.Com_PrintFmt("{}: MotD file load error for \"{}\", discarding.\n", __FUNCTION__, name);
		}
	}
}

int check_ruleset = -1;
static void CheckRuleset() {
	if (game.ruleset && check_ruleset == g_ruleset->modified_count)
		return;

	game.ruleset = (ruleset_t)clamp(g_ruleset->integer, (int)RS_NONE + 1, (int)RS_NUM_RULESETS - 1);

	if ((int)game.ruleset != g_ruleset->integer)
		gi.cvar_forceset("g_ruleset", G_Fmt("{}", (int)game.ruleset).data());

	check_ruleset = g_ruleset->modified_count;

	gi.LocBroadcast_Print(PRINT_HIGH, "Ruleset: {}\n", rs_long_name[(int)game.ruleset]);
}

int gt_teamplay = 0;
int gt_ctf = 0;
int gt_g_gametype = 0;
bool gt_teams_on = false;
gametype_t gt_check = GT_NONE;
void GT_Changes() {
	if (!deathmatch->integer)
		return;

	// do these checks only once level has initialised
	if (!level.init)
		return;

	bool changed = false, team_reset = false;
	gametype_t gt = gametype_t::GT_NONE;

	if (gt_g_gametype != g_gametype->modified_count) {
		gt = (gametype_t)clamp(g_gametype->integer, (int)GT_FIRST, (int)GT_LAST);

		if (gt != gt_check) {
			switch (gt) {
			case gametype_t::GT_TDM:
				if (!teamplay->integer)
					gi.cvar_forceset("teamplay", "1");
				break;
			case gametype_t::GT_CTF:
				if (!ctf->integer)
					gi.cvar_forceset("ctf", "1");
				break;
			default:
				if (teamplay->integer)
					gi.cvar_forceset("teamplay", "0");
				if (ctf->integer)
					gi.cvar_forceset("ctf", "0");
				break;
			}
			gt_teamplay = teamplay->modified_count;
			gt_ctf = ctf->modified_count;
			changed = true;
		}
	}

	if (!changed) {
		if (gt_teamplay != teamplay->modified_count) {
			if (teamplay->integer) {
				gt = gametype_t::GT_TDM;
				if (!teamplay->integer)
					gi.cvar_forceset("teamplay", "1");
				if (ctf->integer)
					gi.cvar_forceset("ctf", "0");
			} else {
				gt = gametype_t::GT_FFA;
				if (teamplay->integer)
					gi.cvar_forceset("teamplay", "0");
				if (ctf->integer)
					gi.cvar_forceset("ctf", "0");
			}
			changed = true;
			gt_teamplay = teamplay->modified_count;
			gt_ctf = ctf->modified_count;
		}
		if (gt_ctf != ctf->modified_count) {
			if (ctf->integer) {
				gt = gametype_t::GT_CTF;
				if (teamplay->integer)
					gi.cvar_forceset("teamplay", "0");
				if (!ctf->integer)
					gi.cvar_forceset("ctf", "1");
			} else {
				gt = gametype_t::GT_TDM;
				if (!teamplay->integer)
					gi.cvar_forceset("teamplay", "1");
				if (ctf->integer)
					gi.cvar_forceset("ctf", "0");
			}
			changed = true;
			gt_teamplay = teamplay->modified_count;
			gt_ctf = ctf->modified_count;
		}
	}

	if (!changed || gt == gametype_t::GT_NONE)
		return;

	//gi.Com_PrintFmt("GAMETYPE = {}\n", (int)gt);
	
	if (gt_teams_on != Teams()) {
		team_reset = true;
		gt_teams_on = Teams();
	}

	if (team_reset) {
		// move all to spectator first
		for (auto ec : active_clients()) {
			//SetIntermissionPoint();
			FindIntermissionPoint();

			ec->s.origin = level.intermission.origin;
			ec->client->ps.pmove.origin = level.intermission.origin;
			ec->client->ps.viewangles = level.intermission.angles;

			ec->client->awaiting_respawn = true;
			ec->client->ps.pmove.pm_type = PM_FREEZE;
			ec->client->ps.rdflags = RDF_NONE;
			ec->deadFlag = false;
			ec->solid = SOLID_NOT;
			ec->moveType = MOVETYPE_FREECAM;
			ec->s.modelindex = 0;
			ec->svFlags |= SVF_NOCLIENT;
			gi.linkentity(ec);
		}

		// set to team and reset match
		for (auto ec : active_clients()) {
			if (!ClientIsPlaying(ec->client))
				continue;
			SetTeam(ec, PickTeam(-1), false, false, true);
		}
	}

	if ((int)gt != gt_check) {
		gi.cvar_forceset("g_gametype", G_Fmt("{}", (int)gt).data());
		gt_g_gametype = g_gametype->modified_count;
		gt_check = (gametype_t)g_gametype->integer;
	} else return;

	//TODO: save ent string so we can simply reload it and Match_Reset
	//gi.AddCommandString("map_restart");

	gi.AddCommandString(G_Fmt("gamemap {}\n", level.mapname).data());

	GT_PrecacheAssets();
	GT_SetLongName();
	gi.LocBroadcast_Print(PRINT_CENTER, "{}", level.gametype_name.data());
}

/*
============
PreInitGame

This will be called when the dll is first loaded, which
only happens when a new game is started or a save game
is loaded.
============
*/
extern void GT_Init();
static void PreInitGame() {
	maxclients = gi.cvar("maxclients", G_Fmt("{}", MAX_SPLIT_PLAYERS).data(), CVAR_SERVERINFO | CVAR_LATCH);
	minplayers = gi.cvar("minplayers", "2", CVAR_NOFLAGS);
	maxplayers = gi.cvar("maxplayers", "16", CVAR_NOFLAGS);

	GT_Init();
}

/*
============
InitMapSystem
============
*/
static void InitMapSystem(gentity_t *ent) {
	if (game.mapSystem.mapPool.empty())
		LoadMapPool(ent);

	bool hasCycleable = std::any_of(game.mapSystem.mapPool.begin(), game.mapSystem.mapPool.end(),
		[](const MapEntry &m) { return m.isCycleable; });

	if (!hasCycleable)
		LoadMapCycle(ent);
}

// ================================================

/*
================
ParseIDListFile
================
*/
static std::unordered_set<std::string> ParseIDListFile(const char *filename) {
	std::unordered_set<std::string> ids;

	std::ifstream file(filename);
	if (!file.is_open())
		return ids;

	std::string line;
	bool inCommentBlock = false;

	while (std::getline(file, line)) {
		// Remove leading/trailing whitespace
		line.erase(0, line.find_first_not_of(" \t\r\n"));
		line.erase(line.find_last_not_of(" \t\r\n") + 1);

		if (line.empty())
			continue;

		// Handle block comments
		if (inCommentBlock) {
			if (line.find("*/") != std::string::npos)
				inCommentBlock = false;
			continue;
		}
		if (line.find("/*") != std::string::npos) {
			inCommentBlock = true;
			continue;
		}

		// Skip single-line comments
		if (line.starts_with("#") || line.starts_with("//"))
			continue;

		// Replace commas with spaces
		for (char &ch : line) {
			if (ch == ',')
				ch = ' ';
		}

		std::istringstream iss(line);
		std::string id;
		while (iss >> id) {
			if (!id.empty())
				ids.insert(id);
		}
	}

	return ids;
}

/*
================
LoadBanList
================
*/
void LoadBanList() {
	game.bannedIDs = ParseIDListFile("ban.txt");
}

/*
================
LoadAdminList
================
*/
void LoadAdminList() {
	game.adminIDs = ParseIDListFile("admin.txt");
}

/*
================
AppendIDToFile
================
*/
bool AppendIDToFile(const char *filename, const std::string &id) {
	std::ofstream file(filename, std::ios::app);
	if (!file.is_open())
		return false;

	file << id << "\n";
	return true;
}

/*
================
RemoveIDFromFile
================
*/
bool RemoveIDFromFile(const char *filename, const std::string &id) {
	std::ifstream infile(filename);
	if (!infile.is_open())
		return false;

	std::vector<std::string> lines;
	std::string line;

	while (std::getline(infile, line)) {
		std::string trimmed = line;
		trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
		trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);

		// Skip match or comment lines
		if (trimmed.empty() || trimmed == id || trimmed.starts_with("#") || trimmed.starts_with("//") || trimmed.starts_with("/*"))
			continue;

		lines.push_back(line); // preserve original line formatting
	}

	std::ofstream outfile(filename, std::ios::trunc);
	if (!outfile.is_open())
		return false;

	for (const auto &out : lines)
		outfile << out << "\n";

	return true;
}

// ================================================

/*
============
InitGame

Called after PreInitGame when the game has set up cvars.
============
*/
extern void Horde_Init();

static void InitGame() {
	gi.Com_Print("==== InitGame ====\n");

	G_InitSave();

	std::random_device rd;
	game.mapRNG.seed(rd());

	// seed RNG
	mt_rand.seed((uint32_t)std::chrono::system_clock::now().time_since_epoch().count());

	hostname = gi.cvar("hostname", "Welcome to WOR!", CVAR_NOFLAGS);

	gun_x = gi.cvar("gun_x", "0", CVAR_NOFLAGS);
	gun_y = gi.cvar("gun_y", "0", CVAR_NOFLAGS);
	gun_z = gi.cvar("gun_z", "0", CVAR_NOFLAGS);

	g_rollspeed = gi.cvar("g_rollspeed", "200", CVAR_NOFLAGS);
	g_rollangle = gi.cvar("g_rollangle", "2", CVAR_NOFLAGS);
	g_maxvelocity = gi.cvar("g_maxvelocity", "2000", CVAR_NOFLAGS);
	g_gravity = gi.cvar("g_gravity", "800", CVAR_NOFLAGS);

	g_skip_view_modifiers = gi.cvar("g_skip_view_modifiers", "0", CVAR_NOSET);

	g_stopspeed = gi.cvar("g_stopspeed", "100", CVAR_NOFLAGS);

	g_horde_starting_wave = gi.cvar("g_horde_starting_wave", "1", CVAR_SERVERINFO | CVAR_LATCH);

	g_huntercam = gi.cvar("g_huntercam", "1", CVAR_SERVERINFO | CVAR_LATCH);
	g_dm_strong_mines = gi.cvar("g_dm_strong_mines", "0", CVAR_NOFLAGS);
	g_dm_random_items = gi.cvar("g_dm_random_items", "0", CVAR_NOFLAGS);

	// freeze tag
	g_frozen_time = gi.cvar("g_frozen_time", "180", CVAR_NOFLAGS);

	// [Paril-KEX]
	g_coop_player_collision = gi.cvar("g_coop_player_collision", "0", CVAR_LATCH);
	g_coop_squad_respawn = gi.cvar("g_coop_squad_respawn", "1", CVAR_LATCH);
	g_coop_enable_lives = gi.cvar("g_coop_enable_lives", "0", CVAR_LATCH);
	g_coop_num_lives = gi.cvar("g_coop_num_lives", "2", CVAR_LATCH);
	g_coop_instanced_items = gi.cvar("g_coop_instanced_items", "1", CVAR_LATCH);
	g_allow_grapple = gi.cvar("g_allow_grapple", "auto", CVAR_NOFLAGS);
	g_allow_kill = gi.cvar("g_allow_kill", "1", CVAR_NOFLAGS);
	g_grapple_offhand = gi.cvar("g_grapple_offhand", "0", CVAR_NOFLAGS);
	g_grapple_fly_speed = gi.cvar("g_grapple_fly_speed", G_Fmt("{}", DEFAULT_GRAPPLE_SPEED).data(), CVAR_NOFLAGS);
	g_grapple_pull_speed = gi.cvar("g_grapple_pull_speed", G_Fmt("{}", DEFAULT_GRAPPLE_PULL_SPEED).data(), CVAR_NOFLAGS);
	g_grapple_damage = gi.cvar("g_grapple_damage", "10", CVAR_NOFLAGS);

	g_frag_messages = gi.cvar("g_frag_messages", "1", CVAR_NOFLAGS);

	g_debug_monster_paths = gi.cvar("g_debug_monster_paths", "0", CVAR_NOFLAGS);
	g_debug_monster_kills = gi.cvar("g_debug_monster_kills", "0", CVAR_LATCH);

	bot_debug_follow_actor = gi.cvar("bot_debug_follow_actor", "0", CVAR_NOFLAGS);
	bot_debug_move_to_point = gi.cvar("bot_debug_move_to_point", "0", CVAR_NOFLAGS);

	// noset vars
	g_dedicated = gi.cvar("dedicated", "0", CVAR_NOSET);

	// latched vars
	g_cheats = gi.cvar("cheats",
#if defined(_DEBUG)
		"1"
#else
		"0"
#endif
		, CVAR_SERVERINFO | CVAR_LATCH);
	gi.cvar("gamename", GAMEVERSION.c_str(), CVAR_SERVERINFO | CVAR_LATCH);

	skill = gi.cvar("skill", "3", CVAR_LATCH);
	maxentities = gi.cvar("maxentities", G_Fmt("{}", MAX_ENTITIES).data(), CVAR_LATCH);

	// change anytime vars
	fraglimit = gi.cvar("fraglimit", "0", CVAR_SERVERINFO);
	timelimit = gi.cvar("timelimit", "0", CVAR_SERVERINFO);
	roundlimit = gi.cvar("roundlimit", "8", CVAR_SERVERINFO);
	roundtimelimit = gi.cvar("roundtimelimit", "2", CVAR_SERVERINFO);
	capturelimit = gi.cvar("capturelimit", "8", CVAR_SERVERINFO);
	mercylimit = gi.cvar("mercylimit", "0", CVAR_NOFLAGS);
	noplayerstime = gi.cvar("noplayerstime", "10", CVAR_NOFLAGS);

	g_ruleset = gi.cvar("g_ruleset", std::to_string(RS_Q2).c_str(), CVAR_SERVERINFO);
	
	password = gi.cvar("password", "", CVAR_USERINFO);
	spectator_password = gi.cvar("spectator_password", "", CVAR_USERINFO);
	admin_password = gi.cvar("admin_password", "", CVAR_NOFLAGS);
	needpass = gi.cvar("needpass", "0", CVAR_SERVERINFO);
	filterban = gi.cvar("filterban", "1", CVAR_NOFLAGS);

	run_pitch = gi.cvar("run_pitch", "0.002", CVAR_NOFLAGS);
	run_roll = gi.cvar("run_roll", "0.005", CVAR_NOFLAGS);
	bob_up = gi.cvar("bob_up", "0.005", CVAR_NOFLAGS);
	bob_pitch = gi.cvar("bob_pitch", "0.002", CVAR_NOFLAGS);
	bob_roll = gi.cvar("bob_roll", "0.002", CVAR_NOFLAGS);

	flood_msgs = gi.cvar("flood_msgs", "4", CVAR_NOFLAGS);
	flood_persecond = gi.cvar("flood_persecond", "4", CVAR_NOFLAGS);
	flood_waitdelay = gi.cvar("flood_waitdelay", "10", CVAR_NOFLAGS);

	ai_allow_dm_spawn = gi.cvar("ai_allow_dm_spawn", "0", CVAR_NOFLAGS);
	ai_damage_scale = gi.cvar("ai_damage_scale", "1", CVAR_NOFLAGS);
	ai_model_scale = gi.cvar("ai_model_scale", "0", CVAR_NOFLAGS);
	ai_movement_disabled = gi.cvar("ai_movement_disabled", "0", CVAR_NOFLAGS);

	bot_name_prefix = gi.cvar("bot_name_prefix", "B|", CVAR_NOFLAGS);

	g_airAccelerate = gi.cvar("g_airAccelerate", "0", CVAR_NOFLAGS);
	g_allowAdmin = gi.cvar("g_allowAdmin", "1", CVAR_NOFLAGS);
	g_allowCustomSkins = gi.cvar("g_allowCustomSkins", "1", CVAR_NOFLAGS);
	g_allowForfeit = gi.cvar("g_allowForfeit", "1", CVAR_NOFLAGS);
	g_allowMymap = gi.cvar("g_allowMymap", "1", CVAR_NOFLAGS);
	g_allowSpecVote = gi.cvar("g_allowSpecVote", "0", CVAR_NOFLAGS);
	g_allowTechs = gi.cvar("g_allowTechs", "auto", CVAR_NOFLAGS);
	g_allowVoteMidGame = gi.cvar("g_allowVoteMidGame", "0", CVAR_NOFLAGS);
	g_allowVoting = gi.cvar("g_allowVoting", "1", CVAR_NOFLAGS);
	g_arenaSelfDmgArmor = gi.cvar("g_arenaSelfDmgArmor", "0", CVAR_NOFLAGS);
	g_arenaStartingArmor = gi.cvar("g_arenaStartingArmor", "200", CVAR_NOFLAGS);
	g_arenaStartingHealth = gi.cvar("g_arenaStartingHealth", "200", CVAR_NOFLAGS);
	g_coop_health_scaling = gi.cvar("g_coop_health_scaling", "0", CVAR_LATCH);
	g_damage_scale = gi.cvar("g_damage_scale", "1", CVAR_NOFLAGS);
	g_disable_player_collision = gi.cvar("g_disable_player_collision", "0", CVAR_NOFLAGS);
	match_startNoHumans = gi.cvar("match_startNoHumans", "1", CVAR_NOFLAGS);
	match_autoJoin = gi.cvar("match_autoJoin", "1", CVAR_NOFLAGS);
	match_crosshairIDs = gi.cvar("match_crosshairIDs", "1", CVAR_NOFLAGS);
	warmup_doReadyUp = gi.cvar("warmup_doReadyUp", "0", CVAR_NOFLAGS);
	warmup_enabled = gi.cvar("warmup_enabled", "1", CVAR_NOFLAGS);
	g_dm_exec_level_cfg = gi.cvar("g_dm_exec_level_cfg", "0", CVAR_NOFLAGS);
	match_forceJoin = gi.cvar("match_forceJoin", "0", CVAR_NOFLAGS);
	match_doForceRespawn = gi.cvar("match_doForceRespawn", "1", CVAR_NOFLAGS);
	match_forceRespawnTime = gi.cvar("match_forceRespawnTime", "2.4", CVAR_NOFLAGS);
	match_holdableAdrenaline = gi.cvar("match_holdableAdrenaline", "1", CVAR_NOFLAGS);
	match_instantItems = gi.cvar("match_instantItems", "1", CVAR_NOFLAGS);
	owner_intermissionShots = gi.cvar("owner_intermissionShots", "0", CVAR_NOFLAGS);
	match_itemsRespawnRate = gi.cvar("match_itemsRespawnRate", "1.0", CVAR_NOFLAGS);
	g_fallingDamage = gi.cvar("g_fallingDamage", "1", CVAR_NOFLAGS);
	g_selfDamage = gi.cvar("g_selfDamage", "1", CVAR_NOFLAGS);
	match_doOvertime = gi.cvar("match_doOvertime", "120", CVAR_NOFLAGS);
	match_powerupDrops = gi.cvar("match_powerupDrops", "1", CVAR_NOFLAGS);
	match_powerupMinPlayerLock = gi.cvar("match_powerupMinPlayerLock", "0", CVAR_NOFLAGS);
	match_playerRespawnMinDelay = gi.cvar("match_playerRespawnMinDelay", "1", CVAR_NOFLAGS);
	match_playerRespawnMinDistance = gi.cvar("match_playerRespawnMinDistance", "256", CVAR_NOFLAGS);
	match_playerRespawnMinDistanceDebug = gi.cvar("match_playerRespawnMinDistanceDebug", "0", CVAR_NOFLAGS);
	match_map_sameLevel = gi.cvar("match_map_sameLevel", "0", CVAR_NOFLAGS);
	match_allowSpawnPads = gi.cvar("match_allowSpawnPads", "1", CVAR_NOFLAGS);
	match_allowTeleporterPads = gi.cvar("match_allowTeleporterPads", "1", CVAR_NOFLAGS);
	match_timeoutLength = gi.cvar("match_timeoutLength", "120", CVAR_NOFLAGS);
	match_weaponsStay = gi.cvar("match_weaponsStay", "0", CVAR_NOFLAGS);
	match_dropCmdFlags = gi.cvar("match_dropCmdFlags", "7", CVAR_NOFLAGS);
	g_entityOverrideDir = gi.cvar("g_entityOverrideDir", "maps", CVAR_NOFLAGS);
	g_entityOverrideLoad = gi.cvar("g_entityOverrideLoad", "1", CVAR_NOFLAGS);
	g_entityOverrideSave = gi.cvar("g_entityOverrideSave", "0", CVAR_NOFLAGS);
	g_eyecam = gi.cvar("g_eyecam", "1", CVAR_NOFLAGS);
	g_fastDoors = gi.cvar("g_fastDoors", "1", CVAR_NOFLAGS);
	g_framesPerFrame = gi.cvar("g_framesPerFrame", "1", CVAR_NOFLAGS);
	g_friendlyFireScale = gi.cvar("g_friendlyFireScale", "1.0", CVAR_NOFLAGS);
	g_inactivity = gi.cvar("g_inactivity", "120", CVAR_NOFLAGS);
	g_infiniteAmmo = gi.cvar("g_infiniteAmmo", "0", CVAR_LATCH);
	g_instantWeaponSwitch = gi.cvar("g_instantWeaponSwitch", "0", CVAR_LATCH);
	g_itemBobbing = gi.cvar("g_itemBobbing", "1", CVAR_NOFLAGS);
	g_knockbackScale = gi.cvar("g_knockbackScale", "1.0", CVAR_NOFLAGS);
	g_ladderSteps = gi.cvar("g_ladderSteps", "1", CVAR_NOFLAGS);
	g_lagCompensation = gi.cvar("g_lagCompensation", "1", CVAR_NOFLAGS);
	match_levelRulesets = gi.cvar("match_levelRulesets", "0", CVAR_NOFLAGS);
	match_maps_list = gi.cvar("match_maps_list", "", CVAR_NOFLAGS);
	match_maps_listShuffle = gi.cvar("match_maps_listShuffle", "1", CVAR_NOFLAGS);
	g_mapspawn_no_bfg = gi.cvar("g_mapspawn_no_bfg", "0", CVAR_NOFLAGS);
	g_mapspawn_no_plasmabeam = gi.cvar("g_mapspawn_no_plasmabeam", "0", CVAR_NOFLAGS);
	match_lock = gi.cvar("match_lock", "0", CVAR_SERVERINFO);
	g_matchstats = gi.cvar("g_matchstats", "0", CVAR_NOFLAGS);
	g_motd_filename = gi.cvar("g_motd_filename", "motd.txt", CVAR_NOFLAGS);
	g_mover_debug = gi.cvar("g_mover_debug", "0", CVAR_NOFLAGS);
	g_mover_speed_scale = gi.cvar("g_mover_speed_scale", "1.0f", CVAR_NOFLAGS);
	g_no_armor = gi.cvar("g_no_armor", "0", CVAR_NOFLAGS);
	g_no_health = gi.cvar("g_no_health", "0", CVAR_NOFLAGS);
	g_no_items = gi.cvar("g_no_items", "0", CVAR_NOFLAGS);
	g_no_mines = gi.cvar("g_no_mines", "0", CVAR_NOFLAGS);
	g_no_nukes = gi.cvar("g_no_nukes", "0", CVAR_NOFLAGS);
	g_no_powerups = gi.cvar("g_no_powerups", "0", CVAR_NOFLAGS);
	g_no_spheres = gi.cvar("g_no_spheres", "0", CVAR_NOFLAGS);
	g_quick_weapon_switch = gi.cvar("g_quick_weapon_switch", "1", CVAR_LATCH);
	g_select_empty = gi.cvar("g_select_empty", "0", CVAR_ARCHIVE);
	g_showhelp = gi.cvar("g_showhelp", "1", CVAR_NOFLAGS);
	g_showmotd = gi.cvar("g_showmotd", "1", CVAR_NOFLAGS);
	g_start_items = gi.cvar("g_start_items", "", CVAR_NOFLAGS);
	g_starting_health = gi.cvar("g_starting_health", "100", CVAR_NOFLAGS);
	g_starting_health_bonus = gi.cvar("g_starting_health_bonus", "25", CVAR_NOFLAGS);
	g_starting_armor = gi.cvar("g_starting_armor", "0", CVAR_NOFLAGS);
	g_strict_saves = gi.cvar("g_strict_saves", "1", CVAR_NOFLAGS);
	g_teamplay_allow_team_pick = gi.cvar("g_teamplay_allow_team_pick", "0", CVAR_NOFLAGS);
	g_teamplay_armor_protect = gi.cvar("g_teamplay_armor_protect", "0", CVAR_NOFLAGS);
	g_teamplay_auto_balance = gi.cvar("g_teamplay_auto_balance", "1", CVAR_NOFLAGS);
	g_teamplay_force_balance = gi.cvar("g_teamplay_force_balance", "0", CVAR_NOFLAGS);
	g_teamplay_item_drop_notice = gi.cvar("g_teamplay_item_drop_notice", "1", CVAR_NOFLAGS);
	g_verbose = gi.cvar("g_verbose", "0", CVAR_NOFLAGS);
	g_vote_flags = gi.cvar("g_vote_flags", "0", CVAR_NOFLAGS);
	g_vote_limit = gi.cvar("g_vote_limit", "3", CVAR_NOFLAGS);
	g_warmup_countdown = gi.cvar("g_warmup_countdown", "10", CVAR_NOFLAGS);
	g_warmup_ready_percentage = gi.cvar("g_warmup_ready_percentage", "0.51f", CVAR_NOFLAGS);
	g_weapon_projection = gi.cvar("g_weapon_projection", "0", CVAR_NOFLAGS);
	g_weapon_respawn_time = gi.cvar("g_weapon_respawn_time", "30", CVAR_NOFLAGS);

	g_maps_pool_file = gi.cvar("g_maps_pool_file", "mapdb.json", CVAR_NOFLAGS);
	g_maps_cycle_file = gi.cvar("g_maps_cycle_file", "mapcycle.txt", CVAR_NOFLAGS);
	g_maps_selector = gi.cvar("g_maps_selector", "1", CVAR_NOFLAGS);
	g_maps_mymap = gi.cvar("g_maps_mymap", "1", CVAR_NOFLAGS);
	g_maps_allow_custom_textures = gi.cvar("g_maps_allow_custom_textures", "1", CVAR_NOFLAGS);
	g_maps_allow_custom_sounds = gi.cvar("g_maps_allow_custom_sounds", "1", CVAR_NOFLAGS);

	g_statex_enabled = gi.cvar("g_statex_enabled", "1", CVAR_NOFLAGS);
	g_statex_humans_present = gi.cvar("g_statex_humans_present", "1", CVAR_NOFLAGS);

	g_blueTeamName = gi.cvar("g_blueTeamName", "Team BLUE", CVAR_NOFLAGS);
	g_redTeamName = gi.cvar("g_redTeamName", "Team RED", CVAR_NOFLAGS);

	// items
	InitItems();

	// ruleset
	CheckRuleset();

	game = {};

	// initialize all entities for this game
	game.maxentities = maxentities->integer;
	g_entities = (gentity_t *)gi.TagMalloc(game.maxentities * sizeof(g_entities[0]), TAG_GAME);
	globals.gentities = g_entities;
	globals.max_entities = game.maxentities;
	
	// initialize all clients for this game
	game.maxclients = maxclients->integer > MAX_CLIENTS_KEX ? MAX_CLIENTS_KEX : maxclients->integer;
	game.clients = (gclient_t *)gi.TagMalloc(game.maxclients * sizeof(game.clients[0]), TAG_GAME);
	globals.num_entities = game.maxclients + 1;

	// how far back we should support lag origins for
	game.max_lag_origins = 20 * (0.1f / gi.frame_time_s);
	game.lag_origins = (vec3_t *)gi.TagMalloc(game.maxclients * sizeof(vec3_t) * game.max_lag_origins, TAG_GAME);

	level.levelStartTime = level.time;
	game.serverStartTime = time(nullptr);

	level.readyToExit = false;

	level.matchState = MatchState::MATCH_WARMUP_DELAYED;
	level.matchStateTimer = 0_sec;
	level.matchStartRealTime = GetCurrentRealTimeMillis();
	level.warmupNoticeTime = level.time;

	memset(level.locked, false, sizeof(level.locked));

	*level.weaponCount = { 0 };

	level.vote.cmd = nullptr;
	level.vote.arg = '\n';

	level.match.totalDeaths = 0;

	gt_teamplay = teamplay->modified_count;
	gt_ctf = ctf->modified_count;
	gt_g_gametype = g_gametype->modified_count;
	gt_teams_on = Teams();

	Horde_Init();

	LoadMotd();

	InitMapSystem(host);

	LoadBanList();
	LoadAdminList();

	if (g_dm_exec_level_cfg->integer)
		gi.AddCommandString(G_Fmt("exec {}\n", level.mapname).data());
}

//===================================================================

/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
void FindIntermissionPoint(void) {
	if (level.intermission.spot)
		return;

	gentity_t *ent = level.spawnSpots[SPAWN_SPOT_INTERMISSION];
	gentity_t *target = nullptr;
	vec3_t dir;
	bool is_landmark = false;

	if (!ent) {
		// fallback if no intermission spot set
		SelectSpawnPoint(nullptr, level.intermission.origin, level.intermission.angles, false, is_landmark);
	} else {
		level.intermission.origin = ent->s.origin;

		// map-specific hacks
		if (!Q_strncasecmp(level.mapname, "campgrounds", 11)) {
			const gvec3_t v = { -320, -96, 503 };
			if (ent->s.origin == v)
				level.intermission.angles[PITCH] = -30;
		} else if (!Q_strncasecmp(level.mapname, "rdm10", 5)) {
			const gvec3_t v = { -1256, -1672, -136 };
			if (ent->s.origin == v)
				level.intermission.angles = { 15, 135, 0 };
		} else {
			level.intermission.angles = ent->s.angles;
		}

		// face toward target if angle is still unset
		if (ent->target && level.intermission.angles == gvec3_t{ 0, 0, 0 }) {
			target = PickTarget(ent->target);
			if (target) {
				dir = (target->s.origin - ent->s.origin).normalized();
				AngleVectors(dir);
				level.intermission.angles = dir * 360.0f;
				gi.Com_PrintFmt("FindIntermissionPoint angles: {}\n", level.intermission.angles);
			}
		}
	}

	level.intermission.spot = true;
}

/*
==================
SetIntermissionPoint
==================
*/
void SetIntermissionPoint(void) {
	if (level.intermission.set)
		return;

	gentity_t *ent = nullptr;

	// Prefer intermission points
	std::vector<gentity_t *> candidates;
	for (auto *e = G_FindByString<&gentity_t::className>(nullptr, "info_player_intermission");
		e != nullptr;
		e = G_FindByString<&gentity_t::className>(e, "info_player_intermission")) {
		if (level.arenaActive == 0 || e->arena == level.arenaActive)
			candidates.push_back(e);
	}

	if (!candidates.empty()) {
		ent = candidates[irandom(candidates.size())];
	} else {
		// fallback: start or dm spawn (filtered if arena > 0)
		ent = G_FindByString<&gentity_t::className>(nullptr, "info_player_start");
		while (ent && level.arenaActive > 0 && ent->arena != level.arenaActive)
			ent = G_FindByString<&gentity_t::className>(ent, "info_player_start");

		if (!ent) {
			ent = G_FindByString<&gentity_t::className>(nullptr, "info_player_deathmatch");
			while (ent && level.arenaActive > 0 && ent->arena != level.arenaActive)
				ent = G_FindByString<&gentity_t::className>(ent, "info_player_deathmatch");
		}
	}

	if (!ent)
		return;

	level.intermission.origin = ent->s.origin;
	level.spawnSpots[SPAWN_SPOT_INTERMISSION] = ent;

	// map-specific hacks
	if (!Q_strncasecmp(level.mapname, "campgrounds", 11)) {
		const gvec3_t v = { -320, -96, 503 };
		if (ent->s.origin == v)
			level.intermission.angles[PITCH] = -30;
	} else if (!Q_strncasecmp(level.mapname, "rdm10", 5)) {
		const gvec3_t v = { -1256, -1672, -136 };
		if (ent->s.origin == v)
			level.intermission.angles = { 15, 135, 0 };
	} else {
		// look at target if present
		if (ent->target) {
			gentity_t *target = PickTarget(ent->target);
			if (target) {
				vec3_t dir = (target->s.origin - level.intermission.origin).normalized();
				AngleVectors(dir);
				level.intermission.angles = dir;
			}
		}
		if (level.intermission.angles == gvec3_t{ 0, 0, 0 })
			level.intermission.angles = ent->s.angles;
	}
}

//===================================================================

static void ShutdownGame() {
	gi.Com_Print("==== ShutdownGame ====\n");

	gi.FreeTags(TAG_LEVEL);
	gi.FreeTags(TAG_GAME);
}

static void *G_GetExtension(const char *name) {
	return nullptr;
}

const shadow_light_data_t *GetShadowLightData(int32_t entity_number);

gtime_t FRAME_TIME_S;
gtime_t FRAME_TIME_MS;

/*
=================
GetGameAPI

Returns a pointer to the structure with all entry points
and global variables
=================
*/
Q2GAME_API game_export_t * GetGameAPI(game_import_t * import) {
	gi = *import;

	FRAME_TIME_S = FRAME_TIME_MS = gtime_t::from_ms(gi.frame_time_ms);

	globals.apiversion = GAME_API_VERSION;
	globals.PreInit = PreInitGame;
	globals.Init = InitGame;
	globals.Shutdown = ShutdownGame;
	globals.SpawnEntities = SpawnEntities;

	globals.WriteGameJson = WriteGameJson;
	globals.ReadGameJson = ReadGameJson;
	globals.WriteLevelJson = WriteLevelJson;
	globals.ReadLevelJson = ReadLevelJson;
	globals.CanSave = CanSave;

	globals.Pmove = Pmove;

	globals.GetExtension = G_GetExtension;

	globals.ClientChooseSlot = ClientChooseSlot;
	globals.ClientThink = ClientThink;
	globals.ClientConnect = ClientConnect;
	globals.ClientUserinfoChanged = ClientUserinfoChanged;
	globals.ClientDisconnect = ClientDisconnect;
	globals.ClientBegin = ClientBegin;
	globals.ClientCommand = ClientCommand;

	globals.RunFrame = G_RunFrame;
	globals.PrepFrame = G_PrepFrame;

	globals.ServerCommand = ServerCommand;
	globals.Bot_SetWeapon = Bot_SetWeapon;
	globals.Bot_TriggerEntity = Bot_TriggerEntity;
	globals.Bot_GetItemID = Bot_GetItemID;
	globals.Bot_UseItem = Bot_UseItem;
	globals.Entity_ForceLookAtPoint = Entity_ForceLookAtPoint;
	globals.Bot_PickedUpItem = Bot_PickedUpItem;

	globals.Entity_IsVisibleToPlayer = Entity_IsVisibleToPlayer;
	globals.GetShadowLightData = GetShadowLightData;

	globals.gentity_size = sizeof(gentity_t);

	return &globals;
}

//======================================================================

/*
=================
ClientEndServerFrames
=================
*/
static void ClientEndServerFrames() {
	// calc the player views now that all pushing
	// and damage has been added
	for (auto ec : active_clients())
		ClientEndServerFrame(ec);
}

/*
=================
CreateTargetChangeLevel

Returns the created target changelevel
=================
*/
gentity_t *CreateTargetChangeLevel(const char *map) {
	gentity_t *ent = Spawn();
	ent->className = "target_changelevel";
	Q_strlcpy(level.nextMap, map, sizeof(level.nextMap));
	ent->map = level.nextMap;
	return ent;
}

// =============================================================

/*
=================
CheckNeedPass
=================
*/
static void CheckNeedPass() {
	int need;
	static int32_t password_modified, spectator_password_modified;

	// if password or spectator_password has changed, update needpass
	// as needed
	if (Cvar_WasModified(password, password_modified) || Cvar_WasModified(spectator_password, spectator_password_modified)) {
		need = 0;

		if (*password->string && Q_strcasecmp(password->string, "none"))
			need |= 1;
		if (*spectator_password->string && Q_strcasecmp(spectator_password->string, "none"))
			need |= 2;

		gi.cvar_set("needpass", G_Fmt("{}", need).data());
	}
}

/*
====================
QueueIntermission
====================
*/
void QueueIntermission(const char *msg, bool boo, bool reset) {
	if (level.intermissionQueued || level.matchState < MatchState::MATCH_IN_PROGRESS)
		return;

	std::strncpy(level.intermission.victorMessage.data(), msg, level.intermission.victorMessage.size() - 1);
	level.intermission.victorMessage.back() = '\0'; // Ensure null-termination

	const char *reason = level.intermission.victorMessage[0] ? level.intermission.victorMessage.data() : "Unknown Reason";
	gi.Com_PrintFmt("MATCH END: {}\n", reason);

	const char *sound = boo ? "insane/insane4.wav" : "world/xian1.wav";
	gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex(sound), 1, ATTN_NONE, 0);

	if (reset) {
		Match_Reset();
		return;
	}

	int64_t now = GetCurrentRealTimeMillis();

	level.matchState = MatchState::MATCH_ENDED;
	level.matchStateTimer = 0_sec;
	level.matchEndRealTime = now;
	level.intermissionQueued = level.time;

	for (auto ec : active_players())
		ec->client->sess.playEndRealTime = now;

	gi.configstring(CS_CDTRACK, "0");
}

/*
============
Teams_CalcRankings

End game rankings
============
*/
void Teams_CalcRankings(std::array<uint32_t, MAX_CLIENTS> &playerRanks) {
	if (!Teams())
		return;

	// we're all winners.. or losers. whatever
	if (level.teamScores[TEAM_RED] == level.teamScores[TEAM_BLUE]) {
		playerRanks.fill(1);
		return;
	}

	team_t winningTeam = (level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE]) ? TEAM_RED : TEAM_BLUE;

	for (auto player : active_players())
		if (player->client->pers.spawned && ClientIsPlaying(player->client)) {
			int index = static_cast<int>(player->s.number) - 1;
			playerRanks[index] = player->client->sess.team == winningTeam ? 1 : 2;
		}
}

/*
=============
BeginIntermission
=============
*/
extern void Gauntlet_MatchEnd_AdjustScores();
void BeginIntermission(gentity_t *targ) {
	if (level.intermissionTime)
		return; // already activated

	// if in a duel, change the wins / losses
	Gauntlet_MatchEnd_AdjustScores();

	game.autosaved = false;

	level.intermissionTime = level.time;

	// respawn any dead clients
	for (auto ec : active_clients()) {
		if (ec->health <= 0 || ec->client->eliminated) {
			ec->health = 1;
			// give us our max health back since it will reset
			// to pers.health; in instanced items we'd lose the items
			// we touched so we always want to respawn with our max.
			if (P_UseCoopInstancedItems())
				ec->client->pers.health = ec->client->pers.max_health = ec->max_health;

			ClientRespawn(ec);
		}
	}

	level.intermission.serverFrame = gi.ServerFrame();
	level.changeMap = targ->map;
	level.intermission.clear = targ->spawnflags.has(SPAWNFLAG_CHANGELEVEL_CLEAR_INVENTORY);
	level.intermission.endOfUnit = false;
	level.intermission.fade = targ->spawnflags.has(SPAWNFLAG_CHANGELEVEL_FADE_OUT);

	// destroy all player trails
	PlayerTrail_Destroy(nullptr);

	// [Paril-KEX] update game level entry
	UpdateLevelEntry();

	if (strstr(level.changeMap, "*")) {
		if (coop->integer) {
			for (auto ec : active_clients()) {
				// strip players of all keys between units
				for (uint8_t n = 0; n < IT_TOTAL; n++)
					if (itemList[n].flags & IF_KEY)
						ec->client->pers.inventory[n] = 0;
			}
		}

		if (level.achievement && level.achievement[0]) {
			gi.WriteByte(svc_achievement);
			gi.WriteString(level.achievement);
			gi.multicast(vec3_origin, MULTICAST_ALL, true);
		}

		level.intermission.endOfUnit = true;

		// "no end of unit" maps handle intermission differently
		if (!targ->spawnflags.has(SPAWNFLAG_CHANGELEVEL_NO_END_OF_UNIT))
			EndOfUnitMessage();
		else if (targ->spawnflags.has(SPAWNFLAG_CHANGELEVEL_IMMEDIATE_LEAVE) && !deathmatch->integer) {
			// Need to call this now
			ReportMatchDetails(true);
			level.intermission.preExit = true; // go to the next level soon
			return;
		}
	} else {
		if (!deathmatch->integer) {
			level.intermission.preExit = true; // go to the next level soon
			return;
		}
	}

	// Call while intermission is running
	ReportMatchDetails(true);

	level.intermission.preExit = false;

	//SetIntermissionPoint();

	// move all clients to the intermission point
	for (auto ec : active_clients()) {
		MoveClientToIntermission(ec);
		if (Teams())
			AnnouncerSound(ec, level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE] ? "red_wins" : "blue_wins");
		else {
			if (ClientIsPlaying(ec->client))
				AnnouncerSound(ec, ec->client->pers.currentRank == 0 ? "you_win" : "you_lose");
		}
	}
}

/*
=============================
TakeIntermissionScreenshot
=============================
*/
static void TakeIntermissionScreenshot() {
	if (!deathmatch->integer || !owner_intermissionShots->integer || level.pop.num_playing_human_clients <= 0)
		return;

	time_t rawTime;
	tm lTime;
	time(&rawTime);
	localtime_s(&lTime, &rawTime);

	char timestamp[64];
	snprintf(timestamp, sizeof(timestamp), "%04d_%02d_%02d-%02d_%02d_%02d",
		1900 + lTime.tm_year, lTime.tm_mon + 1, lTime.tm_mday,
		lTime.tm_hour, lTime.tm_min, lTime.tm_sec);

	std::string filename;

	if (GT(GT_DUEL)) {
		gentity_t *e1 = &g_entities[level.sorted_clients[0] + 1];
		gentity_t *e2 = &g_entities[level.sorted_clients[1] + 1];
		const char *n1 = e1 ? e1->client->sess.netName : "player1";
		const char *n2 = e2 ? e2->client->sess.netName : "player2";

		filename = G_Fmt("screenshot {}-vs-{}-{}-{}\n", n1, n2, level.mapname, timestamp);
	} else {
		gentity_t *ent = &g_entities[1];
		const char *name = (ent->client->followTarget)
			? ent->client->followTarget->client->sess.netName
			: ent->client->sess.netName;

		filename = G_Fmt("screenshot {}-{}-{}-{}\n",
			GametypeIndexToString((gametype_t)g_gametype->integer).c_str(),
			name, level.mapname, timestamp);
	}
	
	gi.Com_PrintFmt("[INTERMISSION]: Taking screenshot '{}'", filename.c_str());
	gi.AddCommandString(filename.c_str());
}

/*
=============
ExitLevel
=============
*/
extern void Gauntlet_RemoveLoser();

void ExitLevel() {
	// [Paril-KEX] N64 fade
	if (level.intermission.fade) {
		level.intermission.fadeTime = level.time + 1.3_sec;
		level.intermission.fading = true;
		return;
	}

	ClientEndServerFrames();
	TakeIntermissionScreenshot();

	level.intermissionTime = 0_ms;

	// if we are running a gauntlet, kick the loser to queue,
	// which will automatically grab the next queued player and restart
	if (deathmatch->integer) {
		if (GT(GT_GAUNTLET))
			Gauntlet_RemoveLoser();
	} else {
		// [Paril-KEX] support for intermission completely wiping players
		// back to default stuff
		if (level.intermission.clear) {
			level.intermission.clear = false;

			for (auto ec : active_clients()) {
				// [Kex] Maintain user info to keep the player skin. 
				char userInfo[MAX_INFO_STRING];
				memcpy(userInfo, ec->client->pers.userInfo, sizeof(userInfo));

				ec->client->pers = ec->client->resp.coopRespawn = {};
				ec->health = 0; // this should trip the power armor, etc to reset as well

				memcpy(ec->client->pers.userInfo, userInfo, sizeof(userInfo));
				memcpy(ec->client->resp.coopRespawn.userInfo, userInfo, sizeof(userInfo));
			}
		}

		// [Paril-KEX] end of unit, so clear level trackers
		if (level.intermission.endOfUnit) {
			game.level_entries = {};

			// give all players their lives back
			if (g_coop_enable_lives->integer)
				for (auto player : active_clients())
					player->client->pers.lives = g_coop_num_lives->integer + 1;
		}
	}

	if (level.changeMap == nullptr) {
		gi.Com_Error("Got null changeMap when trying to exit level. Was a trigger_changelevel configured correctly?");
		return;
	}

	// for N64 mainly, but if we're directly changing to "victorXXX.pcx" then
	// end game
	size_t start_offset = (level.changeMap[0] == '*' ? 1 : 0);

	if (deathmatch->integer && GT(GT_RR) && level.pop.num_playing_clients > 1 && (!level.pop.num_playing_red || !level.pop.num_playing_blue))
		TeamShuffle();

	if (!deathmatch->integer && strlen(level.changeMap) > (6 + start_offset) &&
		!Q_strncasecmp(level.changeMap + start_offset, "victor", 6) &&
		!Q_strncasecmp(level.changeMap + strlen(level.changeMap) - 4, ".pcx", 4))
		gi.AddCommandString(G_Fmt("endgame \"{}\"\n", level.changeMap + start_offset).data());
	else
		gi.AddCommandString(G_Fmt("gamemap \"{}\"\n", level.changeMap).data());

	level.changeMap = nullptr;
}

/*
=============
MapSelectorFinalize
=============
*/
inline void CloseActiveMenu(gentity_t *ent);
static void MapSelectorFinalize() {
	if (level.mapSelectorVoteStartTime == 0_sec)
		return;

	int voteCounts[3] = { 0 };

	// close vote menu for all players
	for (auto ec : active_players()) {
		CloseActiveMenu(ec);
	}

	// Tally votes
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		int vote = level.mapSelectorVoteByClient[i];
		if (vote >= 0 && vote < 3 && level.mapSelectorVoteCandidates[vote]) {
			voteCounts[vote]++;
		}
	}

	// Find max votes
	int maxVotes = 0;
	for (int i = 0; i < 3; ++i) {
		if (voteCounts[i] > maxVotes)
			maxVotes = voteCounts[i];
	}

	// Collect all tied top-voted indices
	std::vector<int> tiedIndices;
	for (int i = 0; i < 3; ++i) {
		if (level.mapSelectorVoteCandidates[i] && voteCounts[i] == maxVotes) {
			tiedIndices.push_back(i);
		}
	}

	int selectedIndex = -1;

	if (!tiedIndices.empty() && maxVotes > 0) {
		selectedIndex = tiedIndices[rand() % tiedIndices.size()];
	} else {
		// No votes cast, randomly select from available candidates
		std::vector<int> available;
		for (int i = 0; i < 3; ++i) {
			if (level.mapSelectorVoteCandidates[i])
				available.push_back(i);
		}
		if (!available.empty())
			selectedIndex = available[rand() % available.size()];
	}

	if (selectedIndex >= 0 && level.mapSelectorVoteCandidates[selectedIndex]) {
		const MapEntry *selected = level.mapSelectorVoteCandidates[selectedIndex];
		level.changeMap = selected->filename.c_str();
		gi.LocBroadcast_Print(PRINT_HIGH, "Map vote complete! Next map: {} ({})\n",
			selected->filename.c_str(),
			selected->longName.empty() ? selected->filename.c_str() : selected->longName.c_str());
		AnnouncerSound(world, "vote_passed");
	} else {
		// Fallback: use auto-select
		auto fallback = AutoSelectNextMap();
		if (fallback) {
			level.changeMap = fallback->filename.c_str();
			gi.LocBroadcast_Print(PRINT_HIGH, "Map vote failed. Randomly selected: {} ({})\n",
				fallback->filename.c_str(),
				fallback->longName.empty() ? fallback->filename.c_str() : fallback->longName.c_str());
		} else {
			gi.Broadcast_Print(PRINT_HIGH, "No maps available for next match.\n");
		}
		AnnouncerSound(world, "vote_failed");
	}

	level.mapSelectorVoteStartTime = 0_sec;
	level.intermission.exit = true;
}

/*
===========================
MapSelectorBegin
===========================
*/
void MapSelectorBegin() {
	level.matchSelectorTried = true;

	// Skip if MyMap queue has entries
	if (!game.mapSystem.playQueue.empty())
		return;

	// Only proceed if map selector is enabled
	if (!g_maps_selector || !g_maps_selector->integer)
		return;

	// Select up to 3 candidate maps
	auto candidates = MapSelectorVoteCandidates();
	if (candidates.empty())
		return;

	// Initialize vote state
	memset(level.mapSelectorVoteCandidates, 0, sizeof(level.mapSelectorVoteCandidates));
	memset(level.mapSelectorVoteCounts, 0, sizeof(level.mapSelectorVoteCounts));
	std::fill_n(level.mapSelectorVoteByClient, MAX_CLIENTS, -1);

	for (size_t i = 0; i < candidates.size() && i < 3; ++i)
		level.mapSelectorVoteCandidates[i] = candidates[i];

	level.mapSelectorVoteStartTime = level.time;

	// Open vote menu for all players
	for (auto ec : active_players()) {
		ec->client->showInventory = false;
		ec->client->showHelp = false;
		ec->client->showScores = false;
		//G_Menu_MapSelector_Open(ec);
	}

	AnnouncerSound(world, "vote_now");
	gi.Broadcast_Print(PRINT_HIGH, "Voting has started for the next map!\nYou have 10 seconds to vote.\n");
}


/*
=============
PreExitLevel

Additional end of match goodness before actually changing level
=============
*/
static void PreExitLevel() {
	if (!game.mapSystem.playQueue.empty()) {
		ExitLevel();
		return;
	}
	if (!level.matchSelectorTried) {
		MapSelectorBegin();
		return;
	}

	//for (auto ec : active_players())
	//	UpdateMapSelectorProgressBar(ec);

	if (level.time < level.mapSelectorVoteStartTime + 10_sec)
		return;

	if (!level.preExitDelay) {
		MapSelectorFinalize();
		level.preExitDelay = level.time;
		return;
	}

	if (level.time < level.preExitDelay + 1_sec)
		return;

	ExitLevel();
}

/*
=============
CheckPowerupsDisabled
=============
*/
static int powerupMinplayersModificationCount = -1;
static int powerupNumPlayersCheck = -1;

static void CheckPowerupsDisabled() {
	bool docheck = false;

	if (powerupMinplayersModificationCount != match_powerupMinPlayerLock->integer) {
		powerupMinplayersModificationCount = match_powerupMinPlayerLock->integer;
		docheck = true;
	}

	if (powerupNumPlayersCheck != level.pop.num_playing_clients) {
		powerupNumPlayersCheck = level.pop.num_playing_clients;
		docheck = true;
	}

	if (!docheck)
		return;

	bool	disable = match_powerupMinPlayerLock->integer > 0 && (level.pop.num_playing_clients < match_powerupMinPlayerLock->integer);
	gentity_t	*ent = nullptr;
	size_t	i;
	for (ent = g_entities + 1, i = 1; i < globals.num_entities; i++, ent++) {
		if (!ent->inUse || !ent->item)
			continue;

		if (!(ent->item->flags & IF_POWERUP))
			continue;
		if (g_quadhog->integer && ent->item->id == IT_POWERUP_QUAD)
			return;

		if (disable) {
			ent->s.renderfx |= (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);
			ent->s.effects |= EF_COLOR_SHELL;
		} else {
			ent->s.renderfx &= ~(RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);
			ent->s.effects &= ~EF_COLOR_SHELL;
		}
	}
}

/*
=============
CheckMinMaxPlayers
=============
*/
static int minplayers_mod_count = -1;
static int maxplayers_mod_count = -1;

static void CheckMinMaxPlayers() {

	if (!deathmatch->integer)
		return;

	if (minplayers_mod_count == minplayers->modified_count &&
			maxplayers_mod_count == maxplayers->modified_count)
		return;

	// set min/maxplayer limits
	if (minplayers->integer < 2) gi.cvar_set("minplayers", "2");
	else if (minplayers->integer > maxclients->integer) gi.cvar_set("minplayers", maxclients->string);
	if (maxplayers->integer < 0) gi.cvar_set("maxplayers", maxclients->string);
	if (maxplayers->integer > maxclients->integer) gi.cvar_set("maxplayers", maxclients->string);
	else if (maxplayers->integer < minplayers->integer) gi.cvar_set("maxplayers", minplayers->string);

	minplayers_mod_count = minplayers->modified_count;
	maxplayers_mod_count = maxplayers->modified_count;
}

static void CheckCvars() {
	if (Cvar_WasModified(g_airAccelerate, game.airacceleration_modified)) {
		// [Paril-KEX] air accel handled by game DLL now, and allow
		// it to be changed in sp/coop
		gi.configstring(CS_AIRACCEL, G_Fmt("{}", g_airAccelerate->integer).data());
		pm_config.airaccel = g_airAccelerate->integer;
	}

	if (Cvar_WasModified(g_gravity, game.gravity_modified))
		level.gravity = g_gravity->value;

	CheckMinMaxPlayers();
}

static bool G_AnyDeadPlayersWithoutLives() {
	for (auto player : active_clients())
		if (player->health <= 0 && (!player->client->pers.lives || player->client->eliminated))
			return true;

	return false;
}

/*
================
G_RunFrame

Advances the world by 0.1 seconds
================
*/
extern void AnnounceCountdown(int t, gtime_t &checkRef);
extern void CheckVote(void);
extern void CheckDMEndFrame();

static inline void G_RunFrame_(bool main_loop) {
	level.inFrame = true;

	if (level.timeoutActive > 0_ms && level.timeoutOwner) {
		int tick = level.timeoutActive.seconds<int>() + 1;
		AnnounceCountdown(tick, level.countdownTimerCheck);

		level.timeoutActive -= FRAME_TIME_MS;
		if (level.timeoutActive <= 0_ms) {
			TimeoutEnd();
		}

		ClientEndServerFrames();
		return;
	} else {
		// track gametype changes and update accordingly
		GT_Changes();

		// cancel vote if timed out
		CheckVote();

		// for tracking changes
		CheckCvars();

		CheckPowerupsDisabled();

		CheckRuleset();

		Bot_UpdateDebug();

		level.time += FRAME_TIME_MS;

		if (!deathmatch->integer && level.intermission.fading) {
			if (level.intermission.fadeTime > level.time) {
				float alpha = clamp(1.0f - (level.intermission.fadeTime - level.time - 300_ms).seconds(), 0.f, 1.f);

				for (auto player : active_clients())
					player->client->ps.screen_blend = { 0, 0, 0, alpha };
			} else {
				level.intermission.fade = level.intermission.fading = false;
				ExitLevel();
			}

			level.inFrame = false;

			return;
		}

		// exit intermissions

		if (level.intermission.preExit) {
			PreExitLevel();
			level.inFrame = false;
			return;
		}
	}

	if (!deathmatch->integer) {
		// reload the map start save if restart time is set (all players are dead)
		if (level.coopLevelRestartTime > 0_ms && level.time > level.coopLevelRestartTime) {
			ClientEndServerFrames();
			gi.AddCommandString("restart_level\n");
		}

		// clear client coop respawn states; this is done
		// early since it may be set multiple times for different
		// players
		if (CooperativeModeOn() && (g_coop_enable_lives->integer || g_coop_squad_respawn->integer)) {
			for (auto player : active_clients()) {
				if (player->client->respawnMaxTime >= level.time)
					player->client->coop_respawn_state = COOP_RESPAWN_WAITING;
				else if (g_coop_enable_lives->integer && player->health <= 0 && player->client->pers.lives == 0)
					player->client->coop_respawn_state = COOP_RESPAWN_NO_LIVES;
				else if (g_coop_enable_lives->integer && G_AnyDeadPlayersWithoutLives())
					player->client->coop_respawn_state = COOP_RESPAWN_NO_LIVES;
				else
					player->client->coop_respawn_state = COOP_RESPAWN_NONE;
			}
		}
	}

	//
	// treat each object in turn
	// even the world gets a chance to think
	//
	gentity_t *ent = &g_entities[0];
	for (size_t i = 0; i < globals.num_entities; i++, ent++) {
		if (!ent->inUse) {
			// defer removing client info so that disconnected, etc works
			if (i > 0 && i <= game.maxclients) {
				if (ent->timeStamp && level.time < ent->timeStamp) {
					int32_t playernum = ent - g_entities - 1;
					gi.configstring(CS_PLAYERSKINS + playernum, "");
					ent->timeStamp = 0_ms;
				}
			}
			continue;
		}

		level.currentEntity = ent;

		// Paril: RF_BEAM entities update their old_origin by hand.
		if (!(ent->s.renderfx & RF_BEAM))
			ent->s.old_origin = ent->s.origin;

		// if the ground entity moved, make sure we are still on it
		if ((ent->groundEntity) && (ent->groundEntity->linkCount != ent->groundEntity_linkCount)) {
			contents_t mask = G_GetClipMask(ent);

			if (!(ent->flags & (FL_SWIM | FL_FLY)) && (ent->svFlags & SVF_MONSTER)) {
				ent->groundEntity = nullptr;
				M_CheckGround(ent, mask);
			} else {
				// if it's still 1 point below us, we're good
				trace_t tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, ent->s.origin + ent->gravityVector, ent,
					mask);

				if (tr.startsolid || tr.allsolid || tr.ent != ent->groundEntity)
					ent->groundEntity = nullptr;
				else
					ent->groundEntity_linkCount = ent->groundEntity->linkCount;
			}
		}

		Entity_UpdateState(ent);

		if (i > 0 && i <= game.maxclients) {
			ClientBeginServerFrame(ent);
			continue;
		}

		G_RunEntity(ent);
	}

	CheckDMEndFrame();

	// see if needpass needs updated
	CheckNeedPass();

	if (CooperativeModeOn() && (g_coop_enable_lives->integer || g_coop_squad_respawn->integer)) {
		// rarely, we can see a flash of text if all players respawned
		// on some other player, so if everybody is now alive we'll reset
		// back to empty
		bool reset_coop_respawn = true;

		for (auto player : active_clients()) {
			if (player->health > 0) {	//muff: changed from >= to >
				reset_coop_respawn = false;
				break;
			}
		}

		if (reset_coop_respawn) {
			for (auto player : active_clients())
				player->client->coop_respawn_state = COOP_RESPAWN_NONE;
		}
	}

	// build the playerstate_t structures for all players
	ClientEndServerFrames();

	// [Paril-KEX] if not in intermission and player 1 is loaded in
	// the game as an entity, increase timer on current entry
	if (level.entry && !level.intermissionTime && g_entities[1].inUse && g_entities[1].client->pers.connected)
		level.entry->time += FRAME_TIME_S;

	// [Paril-KEX] run monster pains now
	for (size_t i = 0; i < static_cast<size_t>(globals.num_entities + 1u + game.maxclients) + BODY_QUEUE_SIZE; i++) {
		gentity_t *e = &g_entities[i];

		if (!e->inUse || !(e->svFlags & SVF_MONSTER))
			continue;

		M_ProcessPain(e);
	}

	level.inFrame = false;
}

static inline bool G_AnyClientsSpawned() {
	for (auto player : active_clients())
		if (player->client && player->client->pers.spawned)
			return true;

	return false;
}

void G_RunFrame(bool main_loop) {
	if (main_loop && !G_AnyClientsSpawned())
		return;

	for (size_t i = 0; i < g_framesPerFrame->integer; i++)
		G_RunFrame_(main_loop);

	// match details.. only bother if there's at least 1 player in-game
	// and not already end of game
	if (G_AnyClientsSpawned() && !level.intermissionTime) {
		constexpr gtime_t report_time = 45_sec;

		if (level.time - level.next_match_report > report_time) {
			level.next_match_report = level.time + report_time;
			ReportMatchDetails(false);
		}
	}
}

/*
================
G_PrepFrame

This has to be done before the world logic, because
player processing happens outside RunFrame
================
*/
void G_PrepFrame() {
	for (size_t i = 0; i < globals.num_entities; i++)
		g_entities[i].s.event = EV_NONE;

	for (auto player : active_clients())
		player->client->ps.stats[STAT_HIT_MARKER] = 0;

	globals.server_flags &= ~SERVER_FLAG_INTERMISSION;

	if (level.intermissionTime) {
		globals.server_flags |= SERVER_FLAG_INTERMISSION;
	}
}
