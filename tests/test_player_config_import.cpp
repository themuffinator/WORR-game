#include "server/g_local.hpp"

#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <cstdlib>

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
TestTagMalloc
=============
*/
static void* TestTagMalloc(size_t size, int /*tag*/)
{
	return std::malloc(size);
}

/*
=============
TestTagFree
=============
*/
static void TestTagFree(void* ptr)
{
	std::free(ptr);
}

/*
=============
main

Loads a legacy player config and verifies the parsed settings populate the session config.
=============
*/
int main()
{
	gi.Com_Print = NoopPrint;
	gi.Com_Error = NoopError;
	gi.TagMalloc = TestTagMalloc;
	gi.TagFree = TestTagFree;

	const std::string socialID = "Import../Tester";
	const std::string sanitized = SanitizeSocialID(socialID);
	const std::filesystem::path configDir = "baseq2/pcfg";
	const std::filesystem::path cfgPath = configDir / (sanitized + ".cfg");

	std::filesystem::create_directories(configDir);
	std::error_code ec;
	std::filesystem::remove(cfgPath, ec);

	{
		std::ofstream out(cfgPath);
		assert(out.is_open());
		out << "show_id 1\n";
		out << "show_fragmessages 0\n";
		out << "show_timer 1\n";
		out << "use_eyecam 1\n";
		out << "killbeep_num 2\n";
		out << "follow_killer 0\n";
		out << "follow_leader 1\n";
		out << "follow_powerup 0\n";
	}

	gclient_t client{};
	std::strncpy(client.sess.socialID, socialID.c_str(), sizeof(client.sess.socialID));
	client.sess.socialID[sizeof(client.sess.socialID) - 1] = '\0';
	client.sess.pc.show_id = false;
	client.sess.pc.show_fragmessages = true;
	client.sess.pc.show_timer = false;
	client.sess.pc.use_eyecam = false;
	client.sess.pc.killbeep_num = 4;
	client.sess.pc.follow_killer = true;
	client.sess.pc.follow_leader = false;
	client.sess.pc.follow_powerup = true;

	gentity_t entity{};
	entity.client = &client;

	PCfg_ClientInitPConfig(&entity);

	assert(client.sess.pc.show_id);
	assert(!client.sess.pc.show_fragmessages);
	assert(client.sess.pc.show_timer);
	assert(client.sess.pc.use_eyecam);
	assert(client.sess.pc.killbeep_num == 2);
	assert(!client.sess.pc.follow_killer);
	assert(client.sess.pc.follow_leader);
	assert(!client.sess.pc.follow_powerup);

	std::filesystem::remove(cfgPath, ec);

	return 0;
}
