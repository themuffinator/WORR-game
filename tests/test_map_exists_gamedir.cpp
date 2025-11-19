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

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
char local_game_import_t::print_buffer[0x10000];
std::array<char[MAX_INFO_STRING], MAX_LOCALIZATION_ARGS> local_game_import_t::buffers{};
std::array<const char*, MAX_LOCALIZATION_ARGS> local_game_import_t::buffer_ptrs{};

namespace {
	static cvar_t g_gameCvar{};
	static std::string g_activeGameDir;

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

	static void WriteBsp(const std::filesystem::path& path)
	{
		std::filesystem::create_directories(path.parent_path());
		std::ofstream file(path, std::ios::binary);
		file << "IBSP";
	}
} // namespace

int main()
{
	gi.cvar = &StubCvar;

	const std::filesystem::path originalWorkingDir = std::filesystem::current_path();
	const std::filesystem::path tempRoot = std::filesystem::temp_directory_path() / "worr_map_exists_validation";
	const std::filesystem::path baseDir = tempRoot / GAMEVERSION;
	const std::filesystem::path customDir = tempRoot / "custom_mod";

	std::error_code ec;
	std::filesystem::remove_all(tempRoot, ec);
	std::filesystem::create_directories(baseDir / "maps");
	std::filesystem::create_directories(customDir / "maps");
	std::filesystem::current_path(tempRoot);

	WriteBsp(baseDir / "maps" / "stockmap.bsp");
	WriteBsp(customDir / "maps" / "custommap.bsp");

	MapSystem system{};

	SetGameDirectory("");
	assert(system.MapExists("stockmap"));

	SetGameDirectory("custom_mod");
	assert(system.MapExists("custommap"));
	assert(system.MapExists("stockmap"));

	std::filesystem::current_path(originalWorkingDir);
	std::filesystem::remove_all(tempRoot);

	return 0;
}
