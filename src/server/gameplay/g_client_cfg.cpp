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

#include "client_config.hpp"
#include "../g_local.hpp"
#include "../../shared/weapon_pref_utils.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <array>
#include <sstream>
#include <vector>
#include <filesystem>
#include <system_error>
#include <algorithm>
#include <limits>
#include <json/json.h>

#include <optional>
#include <set>
#include <string_view>

using json = Json::Value;

const int DEFAULT_RATING = 1500;
const std::string PLAYER_CONFIG_PATH = GAMEVERSION + "/pcfg";

/*
=============
PlayerConfigPathFromID

Sanitizes the supplied player ID and returns the filesystem path for the
associated JSON config file. Returns std::nullopt if the sanitized value is
empty.
=============
*/
static std::optional<std::string> PlayerConfigPathFromID(const std::string& playerID, const char* functionName)
{
	const std::string sanitized = SanitizeSocialID(playerID);
	if (sanitized.empty()) {
		if (!playerID.empty() && gi.Com_Print) {
			gi.Com_PrintFmt("WARNING: {}: refusing to use invalid social ID '{}' for config filename\n", functionName, playerID.c_str());
		}
		return std::nullopt;
	}

	if (sanitized != playerID && gi.Com_Print) {
		gi.Com_PrintFmt("WARNING: {}: sanitized social ID '{}' to '{}' for config filename\n", functionName, playerID.c_str(), sanitized.c_str());
	}

	return std::optional<std::string>(G_Fmt("{}/{}.json", PLAYER_CONFIG_PATH, sanitized).data());
}

/*
================
GetPlayerNameForSocialID
================
*/
std::string GetPlayerNameForSocialID(const std::string& socialID) {
	if (socialID.empty())
		return {};

	const auto pathOpt = PlayerConfigPathFromID(socialID, __FUNCTION__);
	if (!pathOpt)
		return {};

	const std::string path = *pathOpt;

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

static bool EnsurePlayerConfigDirectory() {
        std::error_code ec;
        std::filesystem::create_directories(PLAYER_CONFIG_PATH, ec);
        if (ec) {
                gi.Com_PrintFmt("WARNING: failed to create player config directory {}: {}\n",
                        PLAYER_CONFIG_PATH.c_str(), ec.message().c_str());
        }

        if (!std::filesystem::exists(PLAYER_CONFIG_PATH)) {
                gi.Com_PrintFmt("WARNING: player config directory missing: {}\n", PLAYER_CONFIG_PATH.c_str());
                return false;
        }

        return true;
}

int ClientConfig_DefaultSkillRating() {
	return DEFAULT_RATING;
}

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
        config["followKiller"] = cl ? cl->sess.pc.follow_killer : false;
        config["followLeader"] = cl ? cl->sess.pc.follow_leader : false;
        config["followPowerup"] = cl ? cl->sess.pc.follow_powerup : false;
        config["weaponPrefs"] = Json::Value(Json::arrayValue);
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
	stats["totalTimePlayed"] = Json::Value::Int64(0);
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
		const auto pathOpt = PlayerConfigPathFromID(playerID, __FUNCTION__);
		if (!pathOpt)
                        return;

		if (!EnsurePlayerConfigDirectory())
                        return;

		const std::string path = *pathOpt;

		std::ofstream file(path);
                if (file.is_open()) {
			Json::StreamWriterBuilder builder;
			builder["indentation"] = "    "; // 4 spaces
			std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
			writer->write(newFile, &file);
			file.close();
			gi.Com_PrintFmt("Created new client config file: {}\n", path);
		}
		else {
			gi.Com_PrintFmt("Failed to create client config file: {}\n", path);
		}
	}
	catch (const std::exception& e) {
		gi.Com_PrintFmt("{}: exception while creating client config: {}\n", __FUNCTION__, e.what());
	}
}


/*
=============
ClientConfig_Init
=============
*/
void ClientConfig_Init(gclient_t* cl, const std::string& playerID, const std::string& playerName, const std::string& gameType) {
	Json::Value playerData;
	bool modified = false;

	cl->sess.skillRating = 0;
	cl->sess.skillRatingChange = 0;

	if (playerID.empty()) {
		cl->sess.skillRating = DEFAULT_RATING;
		return;
	}

	const auto pathOpt = PlayerConfigPathFromID(playerID, __FUNCTION__);
	if (!pathOpt) {
		cl->sess.skillRating = DEFAULT_RATING;
		return;
	}

	const std::string path = *pathOpt;
	std::ifstream file(path);

	// If file doesn't exist, create a new default config
	if (!file.is_open()) {
		ClientConfig_Create(cl, playerID, playerName, gameType);
		cl->sess.skillRating = DEFAULT_RATING;
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
                config["followKiller"] = cl->sess.pc.follow_killer;
                config["followLeader"] = cl->sess.pc.follow_leader;
                config["followPowerup"] = cl->sess.pc.follow_powerup;
                config["weaponPrefs"] = Json::Value(Json::arrayValue);
                playerData["config"] = config;
                modified = true;
        }
        else {
                auto& cfg = playerData["config"];

                auto ensure_config_bool = [&](const char* key, bool value) {
                        if (!cfg.isMember(key) || !cfg[key].isBool()) {
                                cfg[key] = value;
                                modified = true;
                        }
                };

                ensure_config_bool("followKiller", cl->sess.pc.follow_killer);
                ensure_config_bool("followLeader", cl->sess.pc.follow_leader);
                ensure_config_bool("followPowerup", cl->sess.pc.follow_powerup);

                if (!cfg.isMember("weaponPrefs") || !cfg["weaponPrefs"].isArray()) {
                        cfg["weaponPrefs"] = Json::Value(Json::arrayValue);
                        modified = true;
                }
        }

	// Ensure stats block exists
	if (!playerData.isMember("stats") || !playerData["stats"].isObject()) {
		Json::Value stats(Json::objectValue);
		stats["totalMatches"] = 0;
		stats["totalWins"] = 0;
		stats["totalLosses"] = 0;
		stats["totalTimePlayed"] = Json::Value::Int64(0);
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

        cl->sess.weaponPrefs.clear();

        // Apply weapon prefs if present
        if (playerData.isMember("config") && playerData["config"].isMember("weaponPrefs")) {
                const auto& prefs = playerData["config"]["weaponPrefs"];
                if (prefs.isArray()) {
                        std::array<bool, static_cast<size_t>(Weapon::Total)> seen{};
                        std::vector<Weapon> parsed;
                        parsed.reserve(static_cast<size_t>(prefs.size()));
                        std::vector<std::string> invalidTokens;
                        bool capacityExceeded = false;

                        for (const auto& p : prefs) {
                                if (!p.isString())
                                        continue;

                                const std::string token = p.asString();
                                std::string normalized;
                                switch (TryAppendWeaponPreference(token, parsed, seen, &normalized)) {
                                case WeaponPrefAppendResult::Added:
                                        break;
                                case WeaponPrefAppendResult::Duplicate:
                                        break;
                                case WeaponPrefAppendResult::Invalid:
                                        if (!normalized.empty())
                                                invalidTokens.emplace_back(std::move(normalized));
                                        break;
                                case WeaponPrefAppendResult::CapacityExceeded:
                                        capacityExceeded = true;
                                        break;
                                }
                        }

                        cl->sess.weaponPrefs.swap(parsed);

                        if (!invalidTokens.empty()) {
                                std::ostringstream joined;
                                for (size_t i = 0; i < invalidTokens.size(); ++i) {
                                        if (i)
                                                joined << ", ";
                                        joined << invalidTokens[i];
                                }
                                gi.Com_PrintFmt("{}: ignored invalid weapon preference tokens for {}: {}\n",
                                        __FUNCTION__, playerName.c_str(), joined.str().c_str());
                        }

                        if (capacityExceeded) {
                                gi.Com_PrintFmt("{}: weapon preferences for {} truncated to {} entries\n",
                                        __FUNCTION__, playerName.c_str(), cl->sess.weaponPrefs.size());
                        }
                }
        }

        Client_RebuildWeaponPreferenceOrder(*cl);

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
                cl->sess.pc.follow_killer = get_bool("followKiller", cl->sess.pc.follow_killer);
                cl->sess.pc.follow_leader = get_bool("followLeader", cl->sess.pc.follow_leader);
                cl->sess.pc.follow_powerup = get_bool("followPowerup", cl->sess.pc.follow_powerup);
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
int64_t timePlayedSeconds,
bool won,
bool isGhost,
bool updateStats,
const client_config_t* pc = nullptr,
const std::vector<Weapon>* weaponPrefs = nullptr
) {
	if (playerID.empty())
		return;

	const auto pathOpt = PlayerConfigPathFromID(playerID, __FUNCTION__);
	if (!pathOpt)
		return;

	const std::string path = *pathOpt;
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
	auto ensureInt64 = [&](const char* key, Json::Value::Int64 def) -> Json::Value::Int64 {
		if (!stats.isMember(key) || !stats[key].isIntegral()) {
			stats[key] = Json::Value::Int64(def);
			modified = true;
		}
		return stats[key].asInt64();
		};

	int totalMatches = ensureInt("totalMatches", 0);
	int totalWins = ensureInt("totalWins", 0);
	int totalLosses = ensureInt("totalLosses", 0);
	Json::Value::Int64 totalTimePlayed = ensureInt64("totalTimePlayed", 0);
	int bestSkillRating = std::max(skillRating, ensureInt("bestSkillRating", skillRating));

	if (updateStats) {
		stats["totalMatches"] = totalMatches + 1;
		stats["totalWins"] = won ? totalWins + 1 : totalWins;
		stats["totalLosses"] = !won ? totalLosses + 1 : totalLosses;

		if (timePlayedSeconds > 0) {
			const Json::Value::Int64 maxTime = std::numeric_limits<Json::Value::Int64>::max();
			Json::Value::Int64 clampedInput = timePlayedSeconds > maxTime ? maxTime : static_cast<Json::Value::Int64>(timePlayedSeconds);
			Json::Value::Int64 cappedTotal = totalTimePlayed > maxTime ? maxTime : totalTimePlayed;
			if (cappedTotal < 0)
				cappedTotal = 0;
			if (cappedTotal > maxTime - clampedInput)
				cappedTotal = maxTime;
			else
				cappedTotal += clampedInput;
			stats["totalTimePlayed"] = Json::Value::Int64(cappedTotal);
		}

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
        if (pc || weaponPrefs) {
                if (!playerData.isMember("config") || !playerData["config"].isObject()) {
                        playerData["config"] = Json::Value(Json::objectValue);
                }
        }

        if (pc) {
                auto& config = playerData["config"];
                config["drawCrosshairID"] = pc->show_id;
                config["drawTimer"] = pc->show_timer;
                config["drawFragMessages"] = pc->show_fragmessages;
                config["eyeCam"] = pc->use_eyecam;
                config["killBeep"] = pc->killbeep_num;
                config["followKiller"] = pc->follow_killer;
                config["followLeader"] = pc->follow_leader;
                config["followPowerup"] = pc->follow_powerup;
        }

        if (weaponPrefs) {
                Json::Value prefs(Json::arrayValue);
                for (Weapon weapon : *weaponPrefs) {
                        const std::string_view abbreviation = WeaponToAbbreviation(weapon);
                        if (!abbreviation.empty()) {
                                prefs.append(std::string(abbreviation));
                        }
                }
                playerData["config"]["weaponPrefs"] = prefs;
        }

        playerData["lastUpdated"] = now;

        // Write updated file
        try {
		if (!EnsurePlayerConfigDirectory())
                        return;

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
void ClientConfig_SaveStats(gclient_t* cl, bool wonMatch) {
	if (!cl || cl->sess.is_a_bot || !cl->sess.socialID[0])
		return;

	const int64_t rawTimePlayed = cl->sess.playEndRealTime - cl->sess.playStartRealTime;
	const int64_t timePlayed = rawTimePlayed > 0 ? rawTimePlayed : 0;
	ClientConfig_SaveInternal(
		cl->sess.socialID,
		cl->sess.skillRating,
		cl->sess.skillRatingChange,
		timePlayed,
		wonMatch,
		false,  // isGhost
		true,   // updateStats
		&cl->sess.pc,
		&cl->sess.weaponPrefs
	);
}

/*
===========================
ClientConfig_SaveStatsForGhost
Saves ghost player's config and updates stats.
===========================
*/
void ClientConfig_SaveStatsForGhost(const Ghosts& ghost, bool won) {
	if (!*ghost.socialID)
		return;

	const int64_t timePlayed = ghost.totalMatchPlayRealTime > 0 ? ghost.totalMatchPlayRealTime : 0;
	ClientConfig_SaveInternal(
		ghost.socialID,
		ghost.skillRating,
		ghost.skillRatingChange,
		timePlayed,
		won,
		true,   // isGhost
		true,   // updateStats
		nullptr, // no pc settings
		nullptr
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

	const auto pathOpt = PlayerConfigPathFromID(playerID, __FUNCTION__);
	if (!pathOpt)
		return false;

	const std::string path = *pathOpt;

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
		if (!EnsurePlayerConfigDirectory())
                        return false;

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

void ClientConfig_SaveWeaponPreferences(gclient_t* cl) {
        if (!cl || cl->sess.is_a_bot || !cl->sess.socialID[0])
                return;

        Client_RebuildWeaponPreferenceOrder(*cl);
        std::vector<std::string> sanitized = GetSanitizedWeaponPrefStrings(*cl);

        ClientConfig_Update(cl->sess.socialID, [sanitized](Json::Value& cfg) {
                if (!cfg.isMember("config") || !cfg["config"].isObject())
                        cfg["config"] = Json::Value(Json::objectValue);

                Json::Value prefs(Json::arrayValue);
                for (const auto& pref : sanitized)
                        prefs.append(pref);

                cfg["config"]["weaponPrefs"] = prefs;
        });
}
