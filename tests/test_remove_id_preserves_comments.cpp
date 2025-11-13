#include "../src/server/g_local.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

/*
=============
ReadFileContents

Loads the full contents of a text file into a single string for assertions.
=============
*/
static std::string ReadFileContents(const std::filesystem::path& path) {
	std::ifstream stream(path);
	std::ostringstream buffer;
	buffer << stream.rdbuf();
	return buffer.str();
}

/*
=============
main

Verifies RemoveIDFromFile keeps comment and blank lines while removing only the target ID.
=============
*/
int main() {
	const std::filesystem::path originalWorkingDir = std::filesystem::current_path();
	const std::filesystem::path tempRoot = std::filesystem::temp_directory_path() / "worr_remove_id_preservation";

	std::error_code ec;
	std::filesystem::remove_all(tempRoot, ec);
	std::filesystem::create_directories(tempRoot);
	std::filesystem::current_path(tempRoot);

	const std::string adminFixture =
		"# Admin list\n"
		"// This is a comment\n"
		"\n"
		"STEAM_1:1:111\n"
		"STEAM_1:1:222\n";

	const std::string banFixture =
		"/* Banned players */\n"
		"// Secondary comment\n"
		"\n"
		"QUAKE2-123\n"
		"QUAKE2-456\n";

	{
		std::ofstream("admin.txt") << adminFixture;
		std::ofstream("ban.txt") << banFixture;
	}

	assert(RemoveIDFromFile("admin.txt", "STEAM_1:1:111"));
	assert(RemoveIDFromFile("ban.txt", "QUAKE2-123"));

	const std::string expectedAdmin =
		"# Admin list\n"
		"// This is a comment\n"
		"\n"
		"STEAM_1:1:222\n";

	const std::string expectedBan =
		"/* Banned players */\n"
		"// Secondary comment\n"
		"\n"
		"QUAKE2-456\n";

	assert(ReadFileContents("admin.txt") == expectedAdmin);
	assert(ReadFileContents("ban.txt") == expectedBan);

	std::filesystem::current_path(originalWorkingDir);
	std::filesystem::remove_all(tempRoot, ec);

	return 0;
}
