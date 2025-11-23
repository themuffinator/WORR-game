#include "server/g_local.hpp"

#include <cassert>
#include <string>

extern bool VerifyEntityString(const char* entities);

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
char local_game_import_t::print_buffer[0x10000];
std::array<char[MAX_INFO_STRING], MAX_LOCALIZATION_ARGS> local_game_import_t::buffers{};
std::array<const char*, MAX_LOCALIZATION_ARGS> local_game_import_t::buffer_ptrs{};

namespace {
	static bool g_errorRaised = false;

	/*
	=============
	StubComPrint
	=============
	*/
	static void StubComPrint(const char* message)
	{
		(void)message;
	}

	/*
	=============
	StubComError
	=============
	*/
	static void StubComError(const char* message)
	{
		g_errorRaised = true;
		(void)message;
	}
}

/*
=============
main
=============
*/
int main()
{
	gi.Com_Print = &StubComPrint;
	gi.Com_Error = &StubComError;

	const std::string truncatedOverride = "{ \"classname\" \"worldspawn\"";

	g_errorRaised = false;
	assert(!VerifyEntityString(truncatedOverride.c_str()));
	assert(g_errorRaised);

	return 0;
}
