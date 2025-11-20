#include "server/g_local.hpp"

#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
std::mt19937 mt_rand{};

/*
=============
NoopPrint
=============
*/
static void NoopPrint(const char* /*msg*/)
{
}

/*
=============
NoopError
=============
*/
static void NoopError(const char* /*msg*/)
{
}

/*
=============
CountOpenFileDescriptors

Counts the visible file descriptors in /proc/self/fd for leak detection.
=============
*/
	static size_t CountOpenFileDescriptors()
	{
		size_t count = 0;
		for (const auto& entry : std::filesystem::directory_iterator("/proc/self/fd"))
		{
			(void)entry;
			++count;
		}
		return count;
	}

/*
=============
main

Validates that legacy player config exports contain all expected key/value pairs.
=============
*/
int main()
	{
	gi.Com_Print = NoopPrint;
	gi.Com_Error = NoopError;

	const std::string socialID = "Export../Tester ID";
	const std::string sanitized = SanitizeSocialID(socialID);
	const std::filesystem::path configDir = "baseq2/pcfg";
	const std::filesystem::path cfgPath = configDir / (sanitized + ".cfg");

	std::filesystem::create_directories(configDir);
	std::error_code ec;
	std::filesystem::remove(cfgPath, ec);

	const std::size_t fdCountBefore = CountOpenFileDescriptors();

	gclient_t client{};
	std::strncpy(client.sess.socialID, socialID.c_str(), sizeof(client.sess.socialID));
	client.sess.socialID[sizeof(client.sess.socialID) - 1] = '\0';
	std::strncpy(client.sess.netName, "CfgExporter", sizeof(client.sess.netName));
	client.sess.netName[sizeof(client.sess.netName) - 1] = '\0';
	client.sess.pc.show_id = false;
	client.sess.pc.show_fragmessages = true;
	client.sess.pc.show_timer = false;
	client.sess.pc.use_eyecam = false;
	client.sess.pc.killbeep_num = 3;
	client.sess.pc.follow_killer = true;
	client.sess.pc.follow_leader = false;
	client.sess.pc.follow_powerup = true;

	gentity_t entity{};
	entity.client = &client;

	PCfg_WriteConfig(&entity);

	std::string contents;
	{
		std::ifstream in(cfgPath);
		assert(in.is_open());
		contents.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

	assert(contents.find("show_id 0") != std::string::npos);
	assert(contents.find("show_fragmessages 1") != std::string::npos);
	assert(contents.find("show_timer 0") != std::string::npos);
	assert(contents.find("use_eyecam 0") != std::string::npos);
	assert(contents.find("killbeep_num 3") != std::string::npos);
	assert(contents.find("follow_killer 1") != std::string::npos);
	assert(contents.find("follow_leader 0") != std::string::npos);
	assert(contents.find("follow_powerup 1") != std::string::npos);

	std::filesystem::remove(cfgPath, ec);

	const std::size_t fdCountAfter = CountOpenFileDescriptors();
	assert(fdCountBefore == fdCountAfter);

	return 0;
}
