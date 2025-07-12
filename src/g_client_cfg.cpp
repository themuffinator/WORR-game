#include "g_local.h"

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <sstream>
#include "nlohmann/json.hpp"

#include <set>

using json = nlohmann::json;

/*
================
GetPlayerNameForSocialID
================
*/
std::string GetPlayerNameForSocialID(const std::string &socialID) {
	const std::string path = "pcfg/" + socialID + ".json";

	std::ifstream file(path);
	if (!file.is_open())
		return {};

	nlohmann::json json;
	try {
		file >> json;
	} catch (...) {
		return {};
	}

	if (!json.contains("playerName") || !json["playerName"].is_string())
		return {};

	return json["playerName"].get<std::string>();
}

const int DEFAULT_RATING = 1500;
const std::string PLAYER_CONFIG_PATH = GAMEVERSION + "/pcfg";

// Function to create a new client config file with default ratings
static void ClientConfig_Create(gclient_t *cl, const std::string &playerID, const std::string &playerName, const std::string &gameType) {
	json newFile = {
		{"socialID", playerID},
		{"playerName", playerName},
		{"originalPlayerName", playerName},
		{"playerAliases", json::array()},  // known past names

		// Visual & audio settings
		{"config", {
			{"drawCrosshairID", 1},
			{"drawFragMessages", 1},
			{"drawTimer", 1},
			{"eyeCam", 1},
			{"killBeep", 1}
		}},

			// Per-game-type ratings (can be float or int depending on your system)
		{"ratings", {
			{gameType, DEFAULT_RATING}
		}},

		// Match-level stats
		{"stats", {
			{"totalMatches", 0},
			{"totalWins",    0},
			{"totalLosses",  0},
			{"totalTimePlayed", 0},   // in seconds
			{"bestSkillRating", 0},
			{"lastSkillRating", DEFAULT_RATING},
			{"lastSkillChange", 0}
		}},

		// Permissions & moderation
		{"admin", false},
		{"banned", false},

		// Tracking
		{"lastUpdated", TimeStamp()},
		{"lastSeen",    TimeStamp()},
		{"firstSeen",   TimeStamp()}
	};


	try {
		std::ofstream file(G_Fmt("{}/{}.json", PLAYER_CONFIG_PATH, playerID).data());
		if (file.is_open()) {
			file << newFile.dump(4); // Pretty print with 4 spaces
			file.close();
			gi.Com_PrintFmt("Created new client config file: {}/{}.json\n", PLAYER_CONFIG_PATH, playerID);
		} else {
			gi.Com_PrintFmt("Failed to create client config file: {}/{}.json\n", PLAYER_CONFIG_PATH, playerID);
		}
	} catch (const std::exception &e) {
		gi.Com_PrintFmt("__FUNCTION__: exception: {}\n", __FUNCTION__, e.what());
	}
}

/*
=============
ClientConfig_Init
=============
*/
void ClientConfig_Init(gclient_t *cl, const std::string &playerID, const std::string &playerName, const std::string &gameType) {
	const std::string path = G_Fmt("{}/{}.json", PLAYER_CONFIG_PATH, playerID).data();
	std::ifstream file(path);
	json playerData;
	bool modified = false;

	cl->sess.skillRating = 0;
	cl->sess.skillRatingChange = 0;

	// If file doesn't exist, create a new default config
	if (!file.is_open()) {
		ClientConfig_Create(cl, playerID, playerName, gameType);
		return;
	}

	// Attempt to parse existing config
	try {
		file >> playerData;
	} catch (const json::parse_error &) {
		gi.Com_PrintFmt("Failed to parse client config for {}: {}\n", playerName.c_str(), path.c_str());
		return;
	}
	file.close();

	// Handle player name changes
	if (playerData.contains("playerName") && playerData["playerName"] != playerName) {
		if (!playerData.contains("originalPlayerName"))
			playerData["originalPlayerName"] = playerData["playerName"];

		if (!playerData.contains("playerAliases") || !playerData["playerAliases"].is_array())
			playerData["playerAliases"] = json::array();

		if (std::find(playerData["playerAliases"].begin(), playerData["playerAliases"].end(), playerName) == playerData["playerAliases"].end()) {
			playerData["playerAliases"].push_back(playerName);
		}

		playerData["playerName"] = playerName;
		modified = true;
	}

	// Ensure config block exists
	if (!playerData.contains("config") || !playerData["config"].is_object()) {
		playerData["config"] = {
			{"drawCrosshairID", 1},
			{"drawFragMessages", 1},
			{"drawTimer", 1},
			{"eyeCam", 1},
			{"killBeep", 1}
		};
		modified = true;
	}

	// Ensure stats block exists
	if (!playerData.contains("stats") || !playerData["stats"].is_object()) {
		playerData["stats"] = {
			{"totalMatches",     0},
			{"totalWins",        0},
			{"totalLosses",      0},
			{"totalTimePlayed",  0},
			{"bestSkillRating",  0},
			{"lastSkillRating",  DEFAULT_RATING},
			{"lastSkillChange",  0}
		};
		modified = true;
	}

	// Ensure ratings block exists
	if (!playerData.contains("ratings") || !playerData["ratings"].is_object()) {
		playerData["ratings"] = {
			{gameType, DEFAULT_RATING}
		};
		playerData["stats"]["lastSkillRating"] = DEFAULT_RATING;
		modified = true;
	} else if (!playerData["ratings"].contains(gameType)) {
		int maxRating = DEFAULT_RATING;
		for (auto &[type, rating] : playerData["ratings"].items()) {
			maxRating = std::max(maxRating, rating.get<int>());
		}
		playerData["ratings"][gameType] = maxRating;
		playerData["stats"]["lastSkillRating"] = maxRating;
		modified = true;
	}

	// Update timestamps
	const std::string now = TimeStamp();
	if (!playerData.contains("firstSeen")) {
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
				outFile << playerData.dump(4);
				outFile.close();
			} else {
				gi.Com_PrintFmt("Failed to write updated config for {}: {}\n", playerName.c_str(), path.c_str());
			}
		} catch (const std::exception &e) {
			gi.Com_PrintFmt("{}: exception writing {}: {}\n", __FUNCTION__, playerName.c_str(), e.what());
		}
	}

	if (playerData.contains("config") && playerData["config"].contains("weaponPrefs")) {
		const auto &prefs = playerData["config"]["weaponPrefs"];
		if (prefs.is_array()) {
			for (const auto &p : prefs) {
				if (p.is_string())
					cl->sess.weaponPrefs.push_back(p.get<std::string>());
			}
		}
	}

	// Apply to session
	cl->sess.skillRating = playerData["ratings"][gameType];
	cl->sess.skillRatingChange = playerData["stats"].value("lastSkillChange", 0);

	if (playerData.contains("admin") && playerData["admin"].is_boolean()) {
		cl->sess.admin = playerData["admin"];
	} else {
		cl->sess.admin = false;
	}
	if (playerData.contains("banned") && playerData["banned"].is_boolean()) {
		cl->sess.banned = playerData["banned"];
	} else {
		cl->sess.banned = false;
	}
}

/*
=============
ClientConfig_SaveStats
=============
*/
void ClientConfig_SaveStats(gclient_t *cl, bool wonMatch) {
	if (!cl || cl->sess.is_a_bot)
		return;

	const std::string path = G_Fmt("{}/{}.json", PLAYER_CONFIG_PATH, cl->sess.socialID).data();
	std::ifstream file(path);
	if (!file.is_open()) {
		gi.Com_PrintFmt("{}: could not open file for {}\n", __FUNCTION__, cl->sess.socialID);
		return;
	}

	json playerData;
	try {
		file >> playerData;
	} catch (const json::parse_error &) {
		gi.Com_PrintFmt("{}: parse error in {}\n", __FUNCTION__, path);
		return;
	}
	file.close();

	bool modified = false;
	const std::string now = TimeStamp();

	// Normalize any bad admin/banned fields (legacy fix)
	if (playerData.contains("admin") && playerData["admin"].is_null()) {
		playerData["admin"] = false;
		modified = true;
	}
	if (playerData.contains("banned") && playerData["banned"].is_null()) {
		playerData["banned"] = false;
		modified = true;
	}

	// Ensure stats block exists and is valid
	if (!playerData.contains("stats") || !playerData["stats"].is_object()) {
		playerData["stats"] = json::object();
		modified = true;
	}

	auto &stats = playerData["stats"];

	// Initialize missing stats fields with defaults
	stats["totalMatches"] = stats.value("totalMatches", 0);
	stats["totalWins"] = stats.value("totalWins", 0);
	stats["totalLosses"] = stats.value("totalLosses", 0);
	stats["totalTimePlayed"] = stats.value("totalTimePlayed", 0);
	stats["lastSkillRating"] = cl->sess.skillRating;
	stats["lastSkillChange"] = cl->sess.skillRatingChange;
	stats["bestSkillRating"] = std::max(cl->sess.skillRating, stats.value("bestSkillRating", cl->sess.skillRating));

	// Apply stat deltas
	stats["totalMatches"] = stats["totalMatches"].get<int>() + 1;
	stats["totalTimePlayed"] = stats["totalTimePlayed"].get<int>() + (cl->sess.playEndRealTime - cl->sess.playStartRealTime);
	if (wonMatch) {
		stats["totalWins"] = stats["totalWins"].get<int>() + 1;
	} else {
		stats["totalLosses"] = stats["totalLosses"].get<int>() + 1;
	}

	// Ensure ratings block exists
	if (!playerData.contains("ratings") || !playerData["ratings"].is_object()) {
		playerData["ratings"] = json::object();
		modified = true;
	}

	// Save updated rating for current game type
	playerData["ratings"][gt_short_name_upper[g_gametype->integer]] = cl->sess.skillRating;

	// Timestamp update
	playerData["lastUpdated"] = now;

	// Save file
	try {
		std::ofstream outFile(path);
		if (outFile.is_open()) {
			outFile << playerData.dump(4);
			outFile.close();
		} else {
			gi.Com_PrintFmt("{}: failed to write file for {}\n", __FUNCTION__, cl->sess.socialID);
		}
	} catch (const std::exception &e) {
		gi.Com_PrintFmt("{}: exception writing {}: {}\n", __FUNCTION__, cl->sess.socialID, e.what());
	}
}

/*
===============
ClientConfig_SaveStatsForGhost
===============
*/
void ClientConfig_SaveStatsForGhost(const Ghosts &ghost, bool won) {
	if (!*ghost.socialID)
		return;

	const std::string path = G_Fmt("{}/{}.json", PLAYER_CONFIG_PATH, ghost.socialID).data();
	std::ifstream file(path);
	if (!file.is_open()) {
		gi.Com_PrintFmt("{}: could not open file for ghost {}\n", __FUNCTION__, ghost.socialID);
		return;
	}

	json playerData;
	try {
		file >> playerData;
	} catch (const json::parse_error &) {
		gi.Com_PrintFmt("{}: parse error in {}\n", __FUNCTION__, path);
		return;
	}
	file.close();

	bool modified = false;
	const std::string now = TimeStamp();

	// Normalize any bad admin/banned fields (legacy fix)
	if (playerData.contains("admin") && playerData["admin"].is_null()) {
		playerData["admin"] = false;
		modified = true;
	}
	if (playerData.contains("banned") && playerData["banned"].is_null()) {
		playerData["banned"] = false;
		modified = true;
	}

	// Ensure stats block exists
	if (!playerData.contains("stats") || !playerData["stats"].is_object()) {
		playerData["stats"] = json::object();
		modified = true;
	}

	auto &stats = playerData["stats"];

	// Initialize missing fields with defaults
	stats["totalMatches"] = stats.value("totalMatches", 0);
	stats["totalWins"] = stats.value("totalWins", 0);
	stats["totalLosses"] = stats.value("totalLosses", 0);
	stats["totalTimePlayed"] = stats.value("totalTimePlayed", 0);
	stats["lastSkillRating"] = ghost.skillRating;
	stats["lastSkillChange"] = ghost.skillRatingChange;
	stats["bestSkillRating"] = std::max(ghost.skillRating, stats.value("bestSkillRating", ghost.skillRating));

	// Apply stat deltas
	stats["totalMatches"] = stats["totalMatches"].get<int>() + 1;
	if (won) {
		stats["totalWins"] = stats["totalWins"].get<int>() + 1;
	} else {
		stats["totalLosses"] = stats["totalLosses"].get<int>() + 1;
	}

	// If you tracked time for ghosts, add to totalTimePlayed here
	// stats["totalTimePlayed"] += ghost.playTimeSeconds;

	// Ensure ratings block exists
	if (!playerData.contains("ratings") || !playerData["ratings"].is_object()) {
		playerData["ratings"] = json::object();
		modified = true;
	}

	// Save updated rating for current game type
	const std::string gtKey = gt_short_name_upper[g_gametype->integer];
	playerData["ratings"][gtKey] = ghost.skillRating;

	// Update timestamp
	playerData["lastUpdated"] = now;

	// Save file
	try {
		std::ofstream outFile(path);
		if (outFile.is_open()) {
			outFile << playerData.dump(4);
			outFile.close();
		} else {
			gi.Com_PrintFmt("{}: failed to write file for {}\n", __FUNCTION__, ghost.socialID);
		}
	} catch (const std::exception &e) {
		gi.Com_PrintFmt("{}: exception writing {}: {}\n", __FUNCTION__, ghost.socialID, e.what());
	}
}

// Loads PLAYER_CONFIG_PATH/<playerID>.json, runs your updater,
// and if anything actually changed, stamps lastUpdated and saves.
// Returns true if the file was modified & written, false otherwise.
static bool ClientConfig_Update(
	const std::string &playerID,
	const std::function<void(json &)> &updater
) {
	const std::string path = G_Fmt("{}/{}.json", PLAYER_CONFIG_PATH, playerID).data();

	// 1) load
	std::ifstream in(path);
	if (!in.is_open()) {
		gi.Com_PrintFmt("{}: failed to open {}\n", __FUNCTION__, path.c_str());
		return false;
	}

	json cfg;
	try {
		in >> cfg;
	} catch (const json::parse_error &e) {
		gi.Com_PrintFmt("{}: parse error in {}: {}\n", __FUNCTION__, path.c_str(), e.what());
		return false;
	}
	in.close();

	// 2) snapshot before
	json before = cfg;

	// 3) let caller mutate
	updater(cfg);

	// 4) if nothing changed, do nothing
	if (cfg == before) {
		return false;
	}

	// 5) stamp update time
	cfg["lastUpdated"] = TimeStamp();

	// 6) write back
	try {
		std::ofstream out(path);
		if (!out.is_open()) {
			gi.Com_PrintFmt("{}: failed to write {}\n", __FUNCTION__, path.c_str());
			return false;
		}
		out << cfg.dump(4);
		out.close();

		gi.Com_PrintFmt("{}: saved updates for {}\n", __FUNCTION__, playerID.c_str());
	}

	catch (const std::exception &e) {
		gi.Com_PrintFmt("__FUNCTION__: exception: {}\n", e.what());
		return false;
	}

	return true;
}

#if 0
// Bulk-update any top-level fields in the player's JSON.
// Example call at the bottom.
bool ClientConfig_BulkUpdate(
	const std::string &playerID,
	const std::initializer_list<std::pair<std::string, json>> &updates) {
	return ClientConfig_Update(playerID, [&](json &cfg) {
		for (auto &kv : updates) {
			cfg[kv.first] = kv.second;
		}
		});
}
#endif
