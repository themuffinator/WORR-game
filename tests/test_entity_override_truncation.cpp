#include <cassert>
#include <string>

#include "server/q_std.cpp"

local_game_import_t gi{};

static bool g_errorCalled = false;
static bool g_printCalled = false;

/*
=============
TestComPrint

Records when the print callback is invoked for validation.
=============
*/
static void TestComPrint(const char* message) {
	g_printCalled = true;
	(void)message;
}

/*
=============
TestComError

Records when the error callback is invoked for validation.
=============
*/
static void TestComError(const char* message) {
	g_errorCalled = true;
	(void)message;
}

/*
=============
main

Verifies that VerifyEntityString rejects truncated override data while
accepting a well-formed entity block.
=============
*/
int main() {
	gi.Com_Print = TestComPrint;
	gi.Com_Error = TestComError;

	const char* valid = "{\n\"classname\" \"worldspawn\"\n}";
	const char* truncated = "{\n\"classname\" \"worldspawn\"";
	const char* empty = "";

	g_errorCalled = false;
	g_printCalled = false;
	assert(VerifyEntityString(valid));
	assert(!g_errorCalled);

	g_errorCalled = false;
	assert(!VerifyEntityString(truncated));
	assert(g_errorCalled);

	g_errorCalled = false;
	assert(!VerifyEntityString(empty));
	assert(g_errorCalled);
	return 0;
}
