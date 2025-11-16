#include "server/g_local.hpp"
#include "server/player/p_client_pcfg.hpp"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <array>
#include <filesystem>
#include <fstream>
#include <string>

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
std::mt19937 mt_rand{};
g_fmt_data_t g_fmt_data{};
char local_game_import_t::print_buffer[0x10000];
std::array<char[MAX_INFO_STRING], MAX_LOCALIZATION_ARGS> local_game_import_t::buffers{};
std::array<const char*, MAX_LOCALIZATION_ARGS> local_game_import_t::buffer_ptrs{};

/*
=============
TestTagMalloc

Allocates memory using the CRT allocator for the test harness.
=============
*/
static void* TestTagMalloc(size_t size, int /*tag*/) {
	return std::malloc(size);
}

/*
=============
TestTagFree

Releases memory allocated by TestTagMalloc.
=============
*/
static void TestTagFree(void* block) {
	std::free(block);
}

/*
=============
TestComPrint

Swallows engine print calls during the test run.
=============
*/
static void TestComPrint(const char* /*message*/) {}

/*
=============
main

Creates a legacy .cfg file, loads it through PCfg_ClientInitPConfig, and
verifies the session settings are applied.
=============
*/
int main() {
	const std::string socialID = "legacy_cfg_test";
	const std::filesystem::path configDir = "baseq2/pcfg";
	const std::filesystem::path cfgPath = configDir / (socialID + ".cfg");

	std::filesystem::create_directories(configDir);
	std::error_code ec;
	std::filesystem::remove(cfgPath, ec);

	{
		std::ofstream out(cfgPath);
		assert(out.is_open());
		out << "// Legacy config\n";
		out << "show_id 1\n";
		out << "show_fragmessages 0\n";
		out << "show_timer off\n";
		out << "killbeep_num 3\n";
	}

	gi.TagMalloc = TestTagMalloc;
	gi.TagFree = TestTagFree;
	gi.Com_Print = TestComPrint;

	gclient_t client{};
	std::strncpy(client.sess.socialID, socialID.c_str(), sizeof(client.sess.socialID));
	client.sess.socialID[sizeof(client.sess.socialID) - 1] = '\0';
	std::strncpy(client.sess.netName, "LegacyTester", sizeof(client.sess.netName));
	client.sess.netName[sizeof(client.sess.netName) - 1] = '\0';

	client.sess.pc.show_id = false;
	client.sess.pc.show_fragmessages = true;
	client.sess.pc.show_timer = true;
	client.sess.pc.killbeep_num = 0;

PCfg_ClientInitPConfigForSession(&client, svflags_t{});

	assert(client.sess.pc.show_id);
	assert(!client.sess.pc.show_fragmessages);
	assert(!client.sess.pc.show_timer);
	assert(client.sess.pc.killbeep_num == 3);

	std::filesystem::remove(cfgPath, ec);

	return 0;
}
