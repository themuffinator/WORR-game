#include "../src/server/g_local.hpp"

#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {
static std::vector<std::string> g_logs;
static cvar_t gameCvar{};

/*
=============
TestComPrint

Records messages emitted by gi.Com_Print during testing.
=============
*/
static void TestComPrint(const char* msg) {
	g_logs.emplace_back(msg ? msg : "");
}

/*
=============
TestCvar

Provides access to the stubbed "game" cvar during testing.
=============
*/
static cvar_t* TestCvar(const char* name, const char*, int) {
	if (!name) {
		return nullptr;
	}

	if (std::strcmp(name, "game") == 0) {
		return &gameCvar;
	}

	return nullptr;
}
}

/*
=============
main

Validates MotD filename sanitization and fallback behaviors.
=============
*/
int main() {
	const std::filesystem::path originalWorkingDir = std::filesystem::current_path();
	const std::filesystem::path tempRoot = std::filesystem::temp_directory_path() / "worr_motd_validation";
	const std::filesystem::path baseDir = tempRoot / "baseq2";
	const std::filesystem::path modDir = tempRoot / "custommod";

	std::error_code ec;
	std::filesystem::remove_all(tempRoot, ec);
	std::filesystem::create_directories(baseDir);
	std::filesystem::create_directories(modDir);
	std::filesystem::current_path(tempRoot);

	{
		std::ofstream(baseDir / "motd.txt") << "Default Message";
		std::ofstream(baseDir / "motd_custom.txt") << "Base Custom Message";
		std::ofstream(modDir / "motd_custom.txt") << "Mod Custom Message";
	}

	static cvar_t motdCvar{};
	static cvar_t verboseCvar{};
	std::string motdValue = "motd_custom.txt";
	std::string verboseValue = "1";
	std::string gameDirValue = "custommod";

	motdCvar.string = motdValue.data();
	verboseCvar.string = verboseValue.data();
	verboseCvar.integer = 1;
	gameCvar.string = gameDirValue.data();

	g_motd_filename = &motdCvar;
	g_verbose = &verboseCvar;

	gi.Com_Print = &TestComPrint;
	gi.cvar = &TestCvar;

	g_logs.clear();
	game.motd.clear();
	game.motdModificationCount = 0;

	LoadMotd();

	assert(game.motd == "Mod Custom Message");
	assert(game.motdModificationCount == 1);

	bool sawModLoad = false;
	for (const std::string& log : g_logs) {
		if (log.find("custommod/motd_custom.txt") != std::string::npos) {
			sawModLoad = true;
			break;
		}
	}
	assert(sawModLoad);

	std::filesystem::remove(modDir / "motd_custom.txt");
	g_logs.clear();
	game.motd.clear();
	game.motdModificationCount = 0;

	LoadMotd();

	assert(game.motd == "Base Custom Message");
	assert(game.motdModificationCount == 1);

	bool sawModMissing = false;
	bool sawBaseLoad = false;
	for (const std::string& log : g_logs) {
		if (log.find("MotD file not found: custommod/motd_custom.txt") != std::string::npos) {
			sawModMissing = true;
		}

		if (log.find("baseq2/motd_custom.txt") != std::string::npos) {
			sawBaseLoad = true;
		}
	}
	assert(sawModMissing);
	assert(sawBaseLoad);

	motdValue = "../motd_evil.txt";
	verboseValue = "0";
	motdCvar.string = motdValue.data();
	verboseCvar.string = verboseValue.data();
	verboseCvar.integer = 0;

	g_logs.clear();
	game.motd.clear();
	game.motdModificationCount = 0;

	LoadMotd();

	assert(game.motd == "Default Message");
	assert(game.motdModificationCount == 1);

	bool sawInvalidLog = false;
	for (const std::string& log : g_logs) {
		if (log.find("Invalid MotD filename") != std::string::npos) {
			sawInvalidLog = true;
			break;
		}
	}
	assert(sawInvalidLog);

	std::filesystem::current_path(originalWorkingDir);
	std::filesystem::remove_all(tempRoot);

	return 0;
}
