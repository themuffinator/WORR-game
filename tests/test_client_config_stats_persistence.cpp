/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_client_config_stats_persistence.cpp implementation.*/

#include "server/g_local.hpp"
#include "server/gameplay/client_config.hpp"

#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <system_error>

#include <json/json.h>

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
std::mt19937 mt_rand{};

/*
=============
LoadJson

Loads a JSON document from disk for validation.
=============
*/
static Json::Value LoadJson(const std::filesystem::path& path) {
	std::ifstream in(path);
	assert(in.is_open());
	Json::Value root;
	Json::CharReaderBuilder builder;
	std::string errs;
	const bool parsed = Json::parseFromStream(builder, in, &root, &errs);
	assert(parsed);
	return root;
}

/*
=============
WriteJson

Writes a JSON document back to disk with stable formatting.
=============
*/
static void WriteJson(const std::filesystem::path& path, const Json::Value& value) {
	std::ofstream out(path);
	assert(out.is_open());
	Json::StreamWriterBuilder builder;
	builder["indentation"] = "    ";
	std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
	writer->write(value, &out);
}

/*
=============
main

Validates long-duration stat persistence for real and ghost players.
=============
*/
int main() {
	const std::string playerID = "test_long_duration_real";
	const std::string ghostID = "test_long_duration_ghost";
	const std::string playerName = "LongSessionPlayer";
	const std::string ghostName = "GhostSessionPlayer";
	const std::string gameType = "FFA";
	const std::filesystem::path configDir = "baseq2/pcfg";
	const std::filesystem::path playerPath = configDir / (playerID + ".json");
	const std::filesystem::path ghostPath = configDir / (ghostID + ".json");

	std::filesystem::create_directories(configDir);
	std::error_code ec;
	std::filesystem::remove(playerPath, ec);
	std::filesystem::remove(ghostPath, ec);

	gclient_t client{};
	std::strncpy(client.sess.socialID, playerID.c_str(), sizeof(client.sess.socialID));
	client.sess.socialID[sizeof(client.sess.socialID) - 1] = '\0';

	GetClientConfigStore().LoadProfile(&client, playerID, playerName, gameType);

	Json::Value initial = LoadJson(playerPath);
	const Json::Value::Int64 nearMax = std::numeric_limits<Json::Value::Int64>::max() - 5;
	initial["stats"]["totalTimePlayed"] = Json::Value::Int64(nearMax);
	WriteJson(playerPath, initial);

	client.sess.skillRating = 1850;
	client.sess.skillRatingChange = 25;
	client.sess.playStartRealTime = 0;
	client.sess.playEndRealTime = 20;

	GetClientConfigStore().SaveStats(&client, true);

	Json::Value updated = LoadJson(playerPath);
	const Json::Value::Int64 cappedMax = std::numeric_limits<Json::Value::Int64>::max();
	assert(updated["stats"]["totalTimePlayed"].isInt64());
	assert(updated["stats"]["totalTimePlayed"].asInt64() == cappedMax);

	client.sess.playStartRealTime = 100;
	client.sess.playEndRealTime = 50;
	GetClientConfigStore().SaveStats(&client, true);

	Json::Value afterNegative = LoadJson(playerPath);
	assert(afterNegative["stats"]["totalTimePlayed"].asInt64() == cappedMax);

	gclient_t ghostInitializer{};
	std::strncpy(ghostInitializer.sess.socialID, ghostID.c_str(), sizeof(ghostInitializer.sess.socialID));
	ghostInitializer.sess.socialID[sizeof(ghostInitializer.sess.socialID) - 1] = '\0';
	GetClientConfigStore().LoadProfile(&ghostInitializer, ghostID, ghostName, gameType);

	Ghosts ghost{};
	std::strncpy(ghost.socialID, ghostID.c_str(), sizeof(ghost.socialID));
	ghost.socialID[sizeof(ghost.socialID) - 1] = '\0';
	ghost.skillRating = 1600;
	ghost.skillRatingChange = -10;
	ghost.totalMatchPlayRealTime = std::numeric_limits<int64_t>::max();

	GetClientConfigStore().SaveStatsForGhost(ghost, false);

	Json::Value ghostData = LoadJson(ghostPath);
	assert(ghostData["stats"]["totalTimePlayed"].isInt64());
	assert(ghostData["stats"]["totalTimePlayed"].asInt64() == cappedMax);
	assert(ghostData["stats"]["totalAbandons"].asInt() >= 1);

	{
		std::ofstream corrupt(playerPath);
		assert(corrupt.is_open());
		corrupt << "{ this is not valid json";
	}

	client.sess.skillRating = 0;
	client.sess.skillRatingChange = 42;
	GetClientConfigStore().LoadProfile(&client, playerID, playerName, gameType);
	assert(client.sess.skillRating == GetClientConfigStore().DefaultSkillRating());
	assert(client.sess.skillRatingChange == 0);

	std::filesystem::remove(playerPath, ec);
	std::filesystem::remove(ghostPath, ec);

	return 0;
}
