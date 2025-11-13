#include "../src/server/g_local.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {
static std::vector<std::string> g_logs;

/*
=============
TestComPrint

Records messages emitted by gi.Com_Print during testing.
=============
*/
static void TestComPrint(const char* msg) {
	g_logs.emplace_back(msg ? msg : "");
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

	std::error_code ec;
	std::filesystem::remove_all(tempRoot, ec);
	std::filesystem::create_directories(baseDir);
	std::filesystem::current_path(tempRoot);

	{
		std::ofstream(baseDir / "motd.txt") << "Default Message";
		std::ofstream(baseDir / "motd_custom.txt") << "Custom Message";
	}

	static cvar_t motdCvar{};
	static cvar_t verboseCvar{};
	std::string motdValue = "motd_custom.txt";
	std::string verboseValue = "0";

	motdCvar.string = motdValue.data();
	verboseCvar.string = verboseValue.data();
	verboseCvar.integer = 0;

	g_motd_filename = &motdCvar;
	g_verbose = &verboseCvar;

	gi.Com_Print = &TestComPrint;
	g_logs.clear();
	game.motd.clear();
	game.motdModificationCount = 0;

	LoadMotd();

	assert(game.motd == "Custom Message");
	assert(game.motdModificationCount == 1);
	assert(g_logs.empty());

	g_logs.clear();
	game.motd.clear();
	game.motdModificationCount = 0;

	motdValue = "../motd_evil.txt";
	motdCvar.string = motdValue.data();

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
