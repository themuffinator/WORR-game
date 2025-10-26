// g_client_cfg.cpp (Game Client Configuration)
// This file manages the persistence of player data by reading and writing
// individual JSON configuration files for each unique player (identified by
// their social ID). This allows for tracking stats and settings across
// multiple game sessions.
//
// Key Responsibilities:
// - Configuration Loading: `ClientConfig_Init` loads a player's JSON file upon
//   connection, creating a new one with default values if it doesn't exist.
// - Data Persistence: `ClientConfig_SaveStats` and related functions write
//   updated information back to the JSON file at the end of a match, including
//   skill rating changes, match history, and total playtime.
// - Data Integrity: Ensures that the JSON files have the necessary structure,
//   adding missing fields (like "stats" or "ratings") if they were created
//   with an older version of the mod.
// - Player Identity: Manages player name changes by tracking aliases.

#include "g_local.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <sstream>
#include <json/json.h>

#include <set>

using json = Json::Value;

/*
================
GetPlayerNameForSocialID
================
*/
std::string GetPlayerNameForSocialID(const std::string& socialID) {
        if (socialID.empty())
                return {};

        const std::string path = "pcfg/" + socialID + ".json";

	std::ifstream file(path);
	if (!file.is_open())
		return {};

	Json::Value root;
	Json::CharReaderBuilder builder;
	std::string errs;

	if (!Json::parseFromStream(builder, file, &root, &errs)) {
		return {};
	}

	if (!root.isMember("playerName") || !root["playerName"].isString())
		return {};

	return root["playerName"].asString();
}

const int DEFAULT_RATING = 1500;
const std::string PLAYER_CONFIG_PATH = GAMEVERSION + "/pcfg";

/*
===============
ClientConfig_Create
===============
*/
static void ClientConfig_Create(gclient_t* cl, const std::string& playerID, const std::string& playerName, const std::string& gameType) {
        if (playerID.empty())
                return;

        Json::Value newFile(Json::objectValue);

	newFile["socialID"] = playerID;
	newFile["playerName"] = playerName;
	newFile["originalPlayerName"] = playerName;
	newFile["playerAliases"] = Json::Value(Json::arrayValue);

	// Visual & audio settings
	Json::Value config(Json::objectValue);
	config["drawCrosshairID"] = 1;
	config["drawFragMessages"] = 1;
	config["drawTimer"] = 1;
	config["eyeCam"] = 1;
	config["killBeep"] = 1;
	newFile["config"] = config;

	// Per-game-type ratings
	Json::Value ratings(Json::objectValue);
	ratings[gameType] = DEFAULT_RATING;
	newFile["ratings"] = ratings;

	// Match-level stats
	Json::Value stats(Json::objectValue);
	stats["totalMatches"] = 0;
	stats["totalWins"] = 0;
	stats["totalLosses"] = 0;
	stats["totalTimePlayed"] = 0;
	stats["bestSkillRating"] = 0;
	stats["lastSkillRating"] = DEFAULT_RATING;
	stats["lastSkillChange"] = 0;
	newFile["stats"] = stats;

	// Permissions
	newFile["admin"] = false;
	newFile["banned"] = false;

	// Tracking
	newFile["lastUpdated"] = TimeStamp();
	newFile["lastSeen"] = TimeStamp();
	newFile["firstSeen"] = TimeStamp();

	try {
		const std::string path = G_Fmt("{}/{}.json", PLAYER_CONFIG_PATH, playerID).data();
		std::ofstream file(path);
		if (file.is_open()) {
			Json::StreamWriterBuilder builder;
			builder["indentation"] = "    "; // 4 spaces
			std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
			writer->write(newFile, &file);
			file.close();
			gi.Com_PrintFmt("Created new client config file: {}\n", path.c_str());
		}
		else {
			gi.Com_PrintFmt("Failed to create client config file: {}\n", path.c_str());
		}
	}
	catch (const std::exception& e) {
		gi.Com_PrintFmt("__FUNCTION__: exception: {}\n", __FUNCTION__, e.what());
	}
}


/*
=============
ClientConfig_Init
=============
*/
void ClientConfig_Init(gclient_t* cl, const std::string& playerID, const std::string& playerName, const std::string& gameType) {
        cl->sess.skillRating = 0;
        cl->sess.skillRatingChange = 0;

        if (playerID.empty()) {
                cl->sess.skillRating = DEFAULT_RATING;
                cl->sess.skillRatingChange = 0;
                cl->sess.admin = false;
                cl->sess.banned = false;
                return;
        }

        const std::string path = G_Fmt("{}/{}.json", PLAYER_CONFIG_PATH, playerID).data();
        std::ifstream file(path);
        Json::Value playerData;
        bool modified = false;

	// If file doesn't exist, create a new default config
	if (!file.is_open()) {
		ClientConfig_Create(cl, playerID, playerName, gameType);
		return;
	}

	// Attempt to parse existing config
	Json::CharReaderBuilder builder;
	std::string errs;
	if (!Json::parseFromStream(builder, file, &playerData, &errs)) {
		gi.Com_PrintFmt("Failed to parse client config for {}: {}\n", playerName.c_str(), path.c_str());
		return;
	}
	file.close();

	// Handle player name changes
	if (playerData.isMember("playerName") && playerData["playerName"].asString() != playerName) {
		if (!playerData.isMember("originalPlayerName"))
			playerData["originalPlayerName"] = playerData["playerName"];

		if (!playerData.isMember("playerAliases") || !playerData["playerAliases"].isArray())
			playerData["playerAliases"] = Json::Value(Json::arrayValue);

		bool alreadyPresent = false;
		for (const auto& alias : playerData["playerAliases"]) {
			if (alias.asString() == playerName) {
				alreadyPresent = true;
				break;
			}
		}
		if (!alreadyPresent) {
			playerData["playerAliases"].append(playerName);
		}

		playerData["playerName"] = playerName;
		modified = true;
	}

	// Ensure config block exists
	if (!playerData.isMember("config") || !playerData["config"].isObject()) {
		Json::Value config(Json::objectValue);
		config["drawCrosshairID"] = 1;
		config["drawFragMessages"] = 1;
		config["drawTimer"] = 1;
		config["eyeCam"] = 1;
		config["killBeep"] = 1;
		playerData["config"] = config;
		modified = true;
	}

	// Ensure stats block exists
	if (!playerData.isMember("stats") || !playerData["stats"].isObject()) {
		Json::Value stats(Json::objectValue);
		stats["totalMatches"] = 0;
		stats["totalWins"] = 0;
		stats["totalLosses"] = 0;
		stats["totalTimePlayed"] = 0;
		stats["bestSkillRating"] = 0;
		stats["lastSkillRating"] = DEFAULT_RATING;
		stats["lastSkillChange"] = 0;
		playerData["stats"] = stats;
		modified = true;
	}

	// Ensure ratings block exists
	if (!playerData.isMember("ratings") || !playerData["ratings"].isObject()) {
		Json::Value ratings(Json::objectValue);
		ratings[gameType] = DEFAULT_RATING;
		playerData["ratings"] = ratings;
		playerData["stats"]["lastSkillRating"] = DEFAULT_RATING;
		modified = true;
	}
	else if (!playerData["ratings"].isMember(gameType)) {
		int maxRating = DEFAULT_RATING;
		const Json::Value& ratings = playerData["ratings"];
		for (const auto& key : ratings.getMemberNames()) {
			maxRating = std::max(maxRating, ratings[key].asInt());
		}
		playerData["ratings"][gameType] = maxRating;
		playerData["stats"]["lastSkillRating"] = maxRating;
		modified = true;
	}

	// Update timestamps
	const std::string now = TimeStamp();
	if (!playerData.isMember("firstSeen")) {
		playerData["firstSeen"] = now;
		modified = true;
	}
	playerData["lastSeen"] = now;
	playerData["lastUpdated"] = now;

	// Save file if changes occurred
	if (modified) {
		try {
			std::ofstream outFile(path);
			if (outFile.is_open()) {
				Json::StreamWriterBuilder writerBuilder;
				writerBuilder["indentation"] = "    ";
				std::unique_ptr<Json::StreamWriter> writer(writerBuilder.newStreamWriter());
				writer->write(playerData, &outFile);
				outFile.close();
			}
			else {
				gi.Com_PrintFmt("Failed to write updated config for {}: {}\n", playerName.c_str(), path.c_str());
			}
		}
		catch (const std::exception& e) {
			gi.Com_PrintFmt("{}: exception writing {}: {}\n", __FUNCTION__, playerName.c_str(), e.what());
		}
	}

	// Apply weapon prefs if present
	if (playerData.isMember("config") && playerData["config"].isMember("weaponPrefs")) {
		const auto& prefs = playerData["config"]["weaponPrefs"];
		if (prefs.isArray()) {
			for (const auto& p : prefs) {
				if (p.isString())
					cl->sess.weaponPrefs.push_back(p.asString());
			}
		}
	}

	// Apply visual/audio config settings
	if (playerData.isMember("config")) {
		const auto& cfg = playerData["config"];

		auto get_bool = [&](const std::string& key, bool def) {
			return cfg.isMember(key) && cfg[key].isBool() ? cfg[key].asBool() : def;
			};
		auto get_int = [&](const std::string& key, int def) {
			return cfg.isMember(key) && cfg[key].isInt() ? cfg[key].asInt() : def;
			};

		cl->sess.pc.show_id = get_bool("drawCrosshairID", true);
		cl->sess.pc.show_timer = get_bool("drawTimer", true);
		cl->sess.pc.show_fragmessages = get_bool("drawFragMessages", true);
		cl->sess.pc.use_eyecam = get_bool("eyeCam", true);
		cl->sess.pc.killbeep_num = get_int("killBeep", 1);
	}

	// Apply to session
	cl->sess.skillRating = playerData["ratings"][gameType].asInt();
	cl->sess.skillRatingChange = playerData["stats"].get("lastSkillChange", 0).asInt();

	cl->sess.admin = playerData.isMember("admin") && playerData["admin"].isBool() ? playerData["admin"].asBool() : false;
	cl->sess.banned = playerData.isMember("banned") && playerData["banned"].isBool() ? playerData["banned"].asBool() : false;
}


/*
============================
ClientConfig_SaveInternal
Internal helper to mutate and save client config JSON.
============================
*/
/*
==========================
ClientConfig_SaveInternal
==========================
*/
static void ClientConfig_SaveInternal(
        const std::string& playerID,
        int skillRating,
        int skillChange,
        int timePlayedSeconds,
        bool won,
        bool isGhost,
        bool updateStats,
        const client_config_t* pc = nullptr
) {
        if (playerID.empty())
                return;

        const std::string path = G_Fmt("{}/{}.json", PLAYER_CONFIG_PATH, playerID).data();
	std::ifstream file(path);
	if (!file.is_open()) {
		gi.Com_PrintFmt("{}: could not open file for {}\n", __FUNCTION__, playerID);
		return;
	}

	Json::Value playerData;
	Json::CharReaderBuilder builder;
	std::string errs;
	if (!Json::parseFromStream(builder, file, &playerData, &errs)) {
		gi.Com_PrintFmt("{}: parse error in {}\n", __FUNCTION__, path);
		return;
	}
	file.close();

	bool modified = false;
	const std::string now = TimeStamp();

	// Normalize old fields
	if (playerData.isMember("admin") && playerData["admin"].isNull()) {
		playerData["admin"] = false;
		modified = true;
	}
	if (playerData.isMember("banned") && playerData["banned"].isNull()) {
		playerData["banned"] = false;
		modified = true;
	}

	// Ensure stats block
	if (!playerData.isMember("stats") || !playerData["stats"].isObject()) {
		playerData["stats"] = Json::Value(Json::objectValue);
		modified = true;
	}
	Json::Value& stats = playerData["stats"];

	auto ensureInt = [&](const char* key, int def) -> int {
		if (!stats.isMember(key) || !stats[key].isInt()) {
			stats[key] = def;
			modified = true;
		}
		return stats[key].asInt();
		};

	int totalMatches = ensureInt("totalMatches", 0);
	int totalWins = ensureInt("totalWins", 0);
	int totalLosses = ensureInt("totalLosses", 0);
	int totalTimePlayed = ensureInt("totalTimePlayed", 0);
	int bestSkillRating = std::max(skillRating, ensureInt("bestSkillRating", skillRating));

	if (updateStats) {
		stats["totalMatches"] = totalMatches + 1;
		stats["totalWins"] = won ? totalWins + 1 : totalWins;
		stats["totalLosses"] = !won ? totalLosses + 1 : totalLosses;

		if (timePlayedSeconds > 0)
			stats["totalTimePlayed"] = totalTimePlayed + timePlayedSeconds;

		if (isGhost) {
			int abandons = ensureInt("totalAbandons", 0);
			stats["totalAbandons"] = abandons + 1;
		}
	}

	stats["bestSkillRating"] = bestSkillRating;
	stats["lastSkillRating"] = skillRating;
	stats["lastSkillChange"] = skillChange;

	// Ensure ratings block
	if (!playerData.isMember("ratings") || !playerData["ratings"].isObject()) {
		playerData["ratings"] = Json::Value(Json::objectValue);
		modified = true;
	}
	playerData["ratings"][Game::GetCurrentInfo().short_name_upper.data()] = skillRating;

	// Save visual/audio config from session
	if (pc) {
		playerData["config"]["drawCrosshairID"] = pc->show_id;
		playerData["config"]["drawTimer"] = pc->show_timer;
		playerData["config"]["drawFragMessages"] = pc->show_fragmessages;
		playerData["config"]["eyeCam"] = pc->use_eyecam;
		playerData["config"]["killBeep"] = pc->killbeep_num;
	}

	playerData["lastUpdated"] = now;

	// Write updated file
	try {
		std::ofstream outFile(path);
		if (outFile.is_open()) {
			Json::StreamWriterBuilder writerBuilder;
			writerBuilder["indentation"] = "    ";
			std::unique_ptr<Json::StreamWriter> writer(writerBuilder.newStreamWriter());
			writer->write(playerData, &outFile);
			outFile.close();
		}
		else {
			gi.Com_PrintFmt("{}: failed to write file for {}\n", __FUNCTION__, playerID);
		}
	}
	catch (const std::exception& e) {
		gi.Com_PrintFmt("{}: exception writing {}: {}\n", __FUNCTION__, playerID, e.what());
	}
}


/*
================
ClientConfig_SaveStats
Saves real player's config and updates stats.
================
*/
void ClientConfig_SaveStats(gclient_t *cl, bool wonMatch) {
        if (!cl || cl->sess.is_a_bot || !cl->sess.socialID[0])
                return;

	const int timePlayed = cl->sess.playEndRealTime - cl->sess.playStartRealTime;
	ClientConfig_SaveInternal(
		cl->sess.socialID,
		cl->sess.skillRating,
		cl->sess.skillRatingChange,
		timePlayed,
		wonMatch,
		false,  // isGhost
		true,   // updateStats
		&cl->sess.pc
	);
}

/*
===========================
ClientConfig_SaveStatsForGhost
Saves ghost player's config and updates stats.
===========================
*/
void ClientConfig_SaveStatsForGhost(const Ghosts &ghost, bool won) {
	if (!*ghost.socialID)
		return;

	const int timePlayed = ghost.totalMatchPlayRealTime;
	ClientConfig_SaveInternal(
		ghost.socialID,
		ghost.skillRating,
		ghost.skillRatingChange,
		timePlayed,
		won,
		true,   // isGhost
		true,   // updateStats
		nullptr // no pc settings
	);
}


/*
====================
ClientConfig_Update

Loads PLAYER_CONFIG_PATH/<playerID>.json, runs your updater,
and if anything changed, stamps lastUpdated and saves.
Returns true if modified and written, false otherwise.
====================
*/
static bool ClientConfig_Update(
        const std::string& playerID,
        const std::function<void(Json::Value&)>& updater
) {
        if (playerID.empty())
                return false;

        const std::string path = G_Fmt("{}/{}.json", PLAYER_CONFIG_PATH, playerID).data();

	// 1) load
	std::ifstream in(path);
	if (!in.is_open()) {
		gi.Com_PrintFmt("{}: failed to open {}\n", __FUNCTION__, path.c_str());
		return false;
	}

	Json::Value cfg;
	Json::CharReaderBuilder builder;
	std::string errs;
	if (!Json::parseFromStream(builder, in, &cfg, &errs)) {
		gi.Com_PrintFmt("{}: parse error in {}: {}\n", __FUNCTION__, path.c_str(), errs.c_str());
		return false;
	}
	in.close();

	// 2) snapshot before (deep copy)
	Json::Value before = cfg;

	// 3) let caller mutate
	updater(cfg);

	// 4) if nothing changed, do nothing
	if (cfg == before)
		return false;

	// 5) stamp update time
	cfg["lastUpdated"] = TimeStamp();

	// 6) write back
	try {
		std::ofstream out(path);
		if (!out.is_open()) {
			gi.Com_PrintFmt("{}: failed to write {}\n", __FUNCTION__, path.c_str());
			return false;
		}

		Json::StreamWriterBuilder writerBuilder;
		writerBuilder["indentation"] = "    ";
		std::unique_ptr<Json::StreamWriter> writer(writerBuilder.newStreamWriter());
		writer->write(cfg, &out);
		out.close();

		gi.Com_PrintFmt("{}: saved updates for {}\n", __FUNCTION__, playerID.c_str());
		return true;
	}
	catch (const std::exception& e) {
		gi.Com_PrintFmt("{}: exception: {}\n", __FUNCTION__, e.what());
		return false;
	}
}
