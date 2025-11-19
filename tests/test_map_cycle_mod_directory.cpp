#include <cmath>
#include <type_traits>

namespace std {
	using ::sinf;
}

#if __cplusplus < 202302L
template<typename Enum>
constexpr std::underlying_type_t<Enum> to_underlying(Enum e) noexcept
{
	return static_cast<std::underlying_type_t<Enum>>(e);
}

namespace std {
	using ::to_underlying;
}
#endif

#include "server/g_local.hpp"

#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
char local_game_import_t::print_buffer[0x10000];
std::array<char[MAX_INFO_STRING], MAX_LOCALIZATION_ARGS> local_game_import_t::buffers{};
std::array<const char*, MAX_LOCALIZATION_ARGS> local_game_import_t::buffer_ptrs{};
cvar_t* g_maps_pool_file;
cvar_t* g_maps_cycle_file;

namespace {
	static cvar_t g_gameCvar{};
	static cvar_t g_cycleFileCvar{};
	static std::string g_activeGameDir;
	static std::string g_cycleFileName = "mapcycle.txt";

	static cvar_t* StubCvar(const char* name, const char*, cvar_flags_t)
	{
		if (std::strcmp(name, "game") == 0)
			return &g_gameCvar;
		return nullptr;
	}

	static void SetGameDirectory(const std::string& dir)
	{
		g_activeGameDir = dir;
		g_gameCvar.string = g_activeGameDir.data();
	}

	static std::filesystem::path WriteCycleFile(const std::filesystem::path& dir, const std::string& contents)
	{
		std::filesystem::create_directories(dir);
		const std::filesystem::path path = dir / g_cycleFileName;
		std::ofstream file(path);
		file << contents;
		return path;
	}
}

int main()
{
	gi.cvar = &StubCvar;
	g_cycleFileCvar.string = g_cycleFileName.data();
	g_maps_cycle_file = &g_cycleFileCvar;

	const std::filesystem::path originalWorkingDir = std::filesystem::current_path();
	const std::filesystem::path tempRoot = std::filesystem::temp_directory_path() / "worr_map_cycle_mod";
	const std::filesystem::path baseDir = tempRoot / GAMEVERSION;
	const std::filesystem::path modDir = tempRoot / "custom_mod";

	std::error_code ec;
	std::filesystem::remove_all(tempRoot, ec);
	std::filesystem::create_directories(baseDir);
	std::filesystem::create_directories(modDir);
	std::filesystem::current_path(tempRoot);

	WriteCycleFile(baseDir, "basecycle\n");
	const std::filesystem::path modPath = WriteCycleFile(modDir, "// mod cycle file\nmodcycle // preferred map\n/* basecycle */\n");

	game.mapSystem.mapPool.clear();
	MapEntry modMap{};
	modMap.filename = "modcycle";
	MapEntry baseMap{};
	baseMap.filename = "basecycle";
	game.mapSystem.mapPool.push_back(modMap);
	game.mapSystem.mapPool.push_back(baseMap);

	std::ifstream file;
	std::string resolvedPath;

	SetGameDirectory("custom_mod");

	assert(G_OpenMapFile(g_maps_cycle_file->string, file, resolvedPath));
	assert(std::filesystem::path(resolvedPath) == modPath);

	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	int matched = 0;
	int unmatched = 0;

	for (auto& map : game.mapSystem.mapPool)
		map.isCycleable = false;

	G_ParseMapCycleContent(content, game.mapSystem.mapPool, matched, unmatched);

	assert(matched == 1);
	assert(unmatched == 0);
	assert(game.mapSystem.mapPool[0].isCycleable);
	assert(!game.mapSystem.mapPool[1].isCycleable);

	std::filesystem::current_path(originalWorkingDir);
	std::filesystem::remove_all(tempRoot);

	return 0;
}
