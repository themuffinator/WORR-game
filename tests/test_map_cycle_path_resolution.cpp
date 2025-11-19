#include "server/g_local.hpp"

#include <array>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
std::mt19937 mt_rand{};
cvar_t* g_maps_cycle_file = nullptr;
cvar_t* g_maps_pool_file = nullptr;
char local_game_import_t::print_buffer[0x10000];
std::array<char[MAX_INFO_STRING], MAX_LOCALIZATION_ARGS> local_game_import_t::buffers{};
std::array<const char*, MAX_LOCALIZATION_ARGS> local_game_import_t::buffer_ptrs{};

namespace {
	static std::string g_gameDir;
	static std::string g_cycleFileName = "mapcycle.txt";
	static cvar_t g_gameCvar{};
	static cvar_t g_cycleFileCvar{};

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
		return nullptr;
	}

	/*
	=============
	NoopPrint
	=============
	*/
	static void NoopPrint(const char*)
	{
	}

	/*
	=============
	ConfigureCycleFile
	=============
	*/
	static void ConfigureCycleFile()
	{
		g_cycleFileCvar.string = g_cycleFileName.c_str();
		g_maps_cycle_file = &g_cycleFileCvar;
	}

	/*
	=============
	SetGameDirectory
	=============
	*/
	static void SetGameDirectory(const std::string& dir)
	{
		g_gameDir = dir;
		g_gameCvar.string = g_gameDir.c_str();
	}

	/*
	=============
	WriteCycleFile
	=============
	*/
	static void WriteCycleFile(const std::filesystem::path& path, const std::string& contents)
	{
		std::filesystem::create_directories(path.parent_path());
		std::ofstream file(path);
		file << contents;
	}
}

/*
=============
main

Verifies map cycle files respect active gamedirs before falling back to
GAMEVERSION.
=============
*/
int main()
{
	gi.cvar = &TestCvar;
	gi.Com_Print = &NoopPrint;
	ConfigureCycleFile();

	const std::filesystem::path originalWorkingDir = std::filesystem::current_path();
	const std::filesystem::path tempRoot = std::filesystem::temp_directory_path() / "worr_map_cycle_path";
	const std::filesystem::path baseDir = tempRoot / GAMEVERSION;
	const std::filesystem::path modDir = tempRoot / "custom_mod";

	std::error_code ec;
	std::filesystem::remove_all(tempRoot, ec);
	std::filesystem::create_directories(baseDir);
	std::filesystem::create_directories(modDir);
	std::filesystem::current_path(tempRoot);

	game.mapSystem.mapPool.clear();

	MapEntry baseEntry{};
	baseEntry.filename = "basemap";
	MapEntry modEntry{};
	modEntry.filename = "modmap";
	game.mapSystem.mapPool.push_back(baseEntry);
	game.mapSystem.mapPool.push_back(modEntry);

	const std::filesystem::path baseCycle = baseDir / g_cycleFileName;
	const std::filesystem::path modCycle = modDir / g_cycleFileName;

	WriteCycleFile(baseCycle, "basemap\n");
	SetGameDirectory("");
	LoadMapCycle(nullptr);
	assert(game.mapSystem.mapPool[0].isCycleable);
	assert(!game.mapSystem.mapPool[1].isCycleable);

	for (auto& map : game.mapSystem.mapPool)
		map.isCycleable = false;

	WriteCycleFile(modCycle, "// custom mapcycle\nmodmap /* preferred */\n");
	SetGameDirectory("custom_mod");
	LoadMapCycle(nullptr);
	assert(!game.mapSystem.mapPool[0].isCycleable);
	assert(game.mapSystem.mapPool[1].isCycleable);

	std::filesystem::current_path(originalWorkingDir);
	std::filesystem::remove_all(tempRoot);

	return 0;
}
