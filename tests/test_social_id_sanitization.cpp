#include "server/g_local.hpp"
#include "server/gameplay/client_config.hpp"

#include <cassert>
#include <cstring>
#include <filesystem>
#include <random>
#include <string>

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
std::mt19937 mt_rand{};

/*
============
NoopPrint
============
*/
static void NoopPrint(const char* /*msg*/)
{
}

/*
============
NoopError
============
*/
static void NoopError(const char* /*msg*/)
{
}

/*
============
main

Validates that social IDs are sanitized before creating client configuration files.
============
*/
int main()
{
	gi.Com_Print = NoopPrint;
	gi.Com_Error = NoopError;

	const std::filesystem::path configDir = "baseq2/pcfg";
	std::filesystem::create_directories(configDir);

	const std::string validID = "Valid_ID-123";
	assert(SanitizeSocialID(validID) == validID);

	const std::string invalidID = "Bad../ID\\!@#";
	const std::string sanitized = SanitizeSocialID(invalidID);
	assert(sanitized == "BadID");

	const std::filesystem::path sanitizedPath = configDir / (sanitized + ".json");
	std::error_code ec;
	std::filesystem::remove(sanitizedPath, ec);

	gclient_t client{};
	std::strncpy(client.sess.socialID, invalidID.c_str(), sizeof(client.sess.socialID));
	client.sess.socialID[sizeof(client.sess.socialID) - 1] = '\0';

	ClientConfig_Init(&client, invalidID, "SanitizedPlayer", "FFA");
	assert(std::filesystem::exists(sanitizedPath));
	assert(client.sess.skillRating == ClientConfig_DefaultSkillRating());

	for (char ch : sanitized) {
		const bool allowed = (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '-' || ch == '_';
		assert(allowed);
	}

	std::filesystem::remove(sanitizedPath, ec);

	return 0;
}
