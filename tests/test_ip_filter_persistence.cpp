#include "server/g_local.hpp"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <vector>
#include <utility>

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
std::mt19937 mt_rand{};
cvar_t* filterBan = nullptr;

namespace {
	static std::vector<std::string> g_locLogs;
	static std::vector<std::string> g_commandQueue;
	static std::vector<std::string> g_args;
	static cvar_t g_gameCvar{};
	static cvar_t g_dummyCvar{};
	static cvar_t g_filterBanCvar{};
	static char g_gameBuffer[1] = { '\0' };
	static char g_dummyBuffer[1] = { '\0' };
	static char g_filterBanBuffer[2] = "0";

	/*
	=============
	TestLocPrint

	Captures localized prints emitted during testing.
	=============
	*/
	static void TestLocPrint(gentity_t*, print_type_t, const char* base, const char** args, size_t count)
	{
		std::string message = base ? base : "";
		for (size_t i = 0; i < count; ++i) {
			const char* arg = args[i] ? args[i] : "";
			const size_t pos = message.find("{}");
			if (pos != std::string::npos)
			message.replace(pos, 2, arg);
		}
		g_locLogs.emplace_back(std::move(message));
	}

	/*
	=============
	TestAddCommandString

	Records commands queued for execution.
	=============
	*/
	static void TestAddCommandString(const char* text)
	{
		g_commandQueue.emplace_back(text ? text : "");
	}

	/*
	=============
	TestArgc
	=============
	*/
	static int TestArgc()
	{
		return static_cast<int>(g_args.size());
	}

	/*
	=============
	TestArgv
	=============
	*/
	static const char* TestArgv(int n)
	{
		return g_args.at(static_cast<size_t>(n)).c_str();
	}

	/*
	=============
	TestCvar

	Provides stub cvar lookups for the test harness.
	=============
	*/
	static cvar_t* TestCvar(const char* name, const char*, cvar_flags_t)
	{
		if (std::strcmp(name, "game") == 0)
		return &g_gameCvar;
		return &g_dummyCvar;
	}

	/*
	=============
	TokenizeCommand

	Splits a whitespace-delimited command string into argv tokens.
	=============
	*/
	static std::vector<std::string> TokenizeCommand(const std::string& command)
	{
		std::istringstream stream(command);
		std::vector<std::string> tokens;
		std::string token;
		while (stream >> token)
		tokens.push_back(token);
		return tokens;
	}

} // namespace

/*
=============
main

Verifies that IP filters persist to disk and restore across restarts.
=============
*/
int main()
{
	const std::filesystem::path originalCwd = std::filesystem::current_path();
	const std::filesystem::path tempRoot = std::filesystem::temp_directory_path() / "worr_ip_filter_persistence";
	const std::filesystem::path baseDir = tempRoot / "baseq2";

	std::error_code ec;
	std::filesystem::remove_all(tempRoot, ec);
	std::filesystem::create_directories(baseDir);
	std::filesystem::current_path(tempRoot);

	g_gameCvar.string = g_gameBuffer;
	g_gameCvar.integer = 0;
	g_dummyCvar.string = g_dummyBuffer;
	g_dummyCvar.integer = 0;
	g_filterBanCvar.string = g_filterBanBuffer;
	g_filterBanCvar.integer = 0;
	filterBan = &g_filterBanCvar;

	gi.Loc_Print = &TestLocPrint;
	gi.AddCommandString = &TestAddCommandString;
	gi.argc = &TestArgc;
	gi.argv = &TestArgv;
	gi.cvar = &TestCvar;

	g_locLogs.clear();
	g_commandQueue.clear();

	g_args = { "sv", "addip", "192.168.1.1" };
	ServerCommand();

	g_args = { "sv", "addip", "10.0.0.0" };
	ServerCommand();

	G_SaveIPFilters();

	const std::filesystem::path cfgPath = baseDir / "listip.cfg";
	std::ifstream outFile(cfgPath);
	assert(outFile.is_open());

	std::vector<std::string> fileLines;
	std::string fileLine;
	while (std::getline(outFile, fileLine))
	fileLines.emplace_back(fileLine);
	outFile.close();

	assert(fileLines.size() == 3);
	assert(fileLines[0] == "set filterban 0");
	assert(fileLines[1] == "sv addip 192.168.1.1");
	assert(fileLines[2] == "sv addip 10.0.0.0");

	g_args = { "sv", "removeip", "192.168.1.1" };
	ServerCommand();
	g_args = { "sv", "removeip", "10.0.0.0" };
	ServerCommand();

	g_locLogs.clear();
	g_args = { "sv", "listip" };
	ServerCommand();

	size_t remainingEntries = 0;
	for (const std::string& message : g_locLogs) {
		if (message.find('.') != std::string::npos)
		++remainingEntries;
	}
	assert(remainingEntries == 0);

	g_filterBanCvar.integer = 1;
	std::snprintf(g_filterBanBuffer, sizeof(g_filterBanBuffer), "%d", g_filterBanCvar.integer);

	g_commandQueue.clear();
	G_LoadIPFilters();

	for (std::string command : g_commandQueue) {
		if (!command.empty() && command.back() == '\n')
		command.pop_back();
		if (command.empty())
		continue;

		if (command.rfind("sv ", 0) == 0) {
			g_args = TokenizeCommand(command);
			ServerCommand();
		}
		else if (command.rfind("set filterban ", 0) == 0) {
			const int value = std::stoi(command.substr(14));
			g_filterBanCvar.integer = value;
			std::snprintf(g_filterBanBuffer, sizeof(g_filterBanBuffer), "%d", value);
		}
	}

	g_locLogs.clear();
	g_args = { "sv", "listip" };
	ServerCommand();

	size_t restoredEntries = 0;
	for (const std::string& message : g_locLogs) {
		if (message.find('.') != std::string::npos)
		++restoredEntries;
	}
	assert(restoredEntries == 2);
	assert(g_filterBanCvar.integer == 0);

	std::filesystem::current_path(originalCwd);
	std::filesystem::remove_all(tempRoot, ec);

	return 0;
}
