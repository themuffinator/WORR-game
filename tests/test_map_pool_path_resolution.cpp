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
cvar_t* g_maps_pool_file = nullptr;
char local_game_import_t::print_buffer[0x10000];
std::array<char[MAX_INFO_STRING], MAX_LOCALIZATION_ARGS> local_game_import_t::buffers{};
std::array<const char*, MAX_LOCALIZATION_ARGS> local_game_import_t::buffer_ptrs{};

namespace {
	static std::string g_gameDir;
	static std::string g_poolFileName = "mapdb.json";
	static cvar_t g_gameCvar{};
	static cvar_t g_poolFileCvar{};

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
	ConfigurePoolFile
	=============
	*/
	static void ConfigurePoolFile()
	{
		g_poolFileCvar.string = g_poolFileName.c_str();
		g_maps_pool_file = &g_poolFileCvar;
	}

	/*
	=============
	WritePoolFile
	=============
	*/
	static void WritePoolFile(const std::filesystem::path& path)
	{
		std::filesystem::create_directories(path.parent_path());
		std::ofstream file(path);
		file << "{}";
	}
}

/*
=============
main

Validates map pool path resolution across base and mod gamedirs.
=============
*/
int main()
{
	gi.cvar = &TestCvar;
	ConfigurePoolFile();

	const std::filesystem::path originalWorkingDir = std::filesystem::current_path();
	const std::filesystem::path tempRoot = std::filesystem::temp_directory_path() / "worr_map_pool_path";
	const std::filesystem::path baseDir = tempRoot / GAMEVERSION;
	const std::filesystem::path modDir = tempRoot / "custom_mod";

	std::error_code ec;
	std::filesystem::remove_all(tempRoot, ec);
	std::filesystem::create_directories(baseDir);
	std::filesystem::create_directories(modDir);
	std::filesystem::current_path(tempRoot);

	const std::filesystem::path basePool = baseDir / g_poolFileName;
	const std::filesystem::path modPool = modDir / g_poolFileName;

	WritePoolFile(basePool);

	SetGameDirectory("");
	const MapPoolLocation baseLocation = G_ResolveMapPoolPath();
	assert(baseLocation.path == basePool.string());
	assert(baseLocation.exists);
	assert(!baseLocation.loadedFromMod);

	SetGameDirectory("custom_mod");
	WritePoolFile(modPool);
	const MapPoolLocation modLocation = G_ResolveMapPoolPath();
	assert(modLocation.path == modPool.string());
	assert(modLocation.exists);
	assert(modLocation.loadedFromMod);

	std::filesystem::remove(modPool);
	const MapPoolLocation fallbackLocation = G_ResolveMapPoolPath();
	assert(fallbackLocation.path == basePool.string());
	assert(fallbackLocation.exists);
	assert(!fallbackLocation.loadedFromMod);

	std::filesystem::remove(basePool);
	const MapPoolLocation missingLocation = G_ResolveMapPoolPath();
	assert(!missingLocation.exists);
	assert(missingLocation.path == basePool.string());
	assert(!missingLocation.loadedFromMod);

	std::filesystem::current_path(originalWorkingDir);
	std::filesystem::remove_all(tempRoot);

	return 0;
}
