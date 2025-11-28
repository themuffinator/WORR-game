#include "../src/server/gameplay/server_limits.hpp"

#include <cassert>
#include <string>

local_game_import_t gi{};
GameLocals game{};
namespace {
	static bool g_errorTriggered = false;
	static std::string g_lastError;

	/*
	=============
	TestComError

	Captures fatal initialization errors emitted by the guard under test.
	=============
	*/
	void TestComError(const char* message) {
		g_errorTriggered = true;
		g_lastError = message ? message : "";
	}
}


/*
	=============
main

Verifies that ValidateEntityCapacityOrError mirrors InitGame's guard.
=============
*/
int main() {
	gi.Com_Error = &TestComError;

	g_errorTriggered = false;
	ValidateEntityCapacityOrError(static_cast<int>(BODY_QUEUE_SIZE) + 1, 0);
	assert(!g_errorTriggered);

	g_errorTriggered = false;
	ValidateEntityCapacityOrError(static_cast<int>(BODY_QUEUE_SIZE) + 4, 3);
	assert(!g_errorTriggered);

	g_errorTriggered = false;
	const int configuredClients = 6;
	const int insufficientEntities = configuredClients + static_cast<int>(BODY_QUEUE_SIZE);
	ValidateEntityCapacityOrError(insufficientEntities, configuredClients);
	assert(g_errorTriggered);
	assert(g_lastError.find("maxentities") != std::string::npos);
	assert(g_lastError.find(std::to_string(configuredClients + static_cast<int>(BODY_QUEUE_SIZE) + 1)) != std::string::npos);

	return 0;
}
