/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_command_args_consistency.cpp implementation.*/

#include "server/commands/command_system.hpp"

#include <cassert>
#include <string>
#include <vector>

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
std::mt19937 mt_rand{};

static std::vector<std::string> g_args;

/*
=============
TestArgc

Returns the simulated engine argument count.
=============
*/
static int TestArgc() {
	return static_cast<int>(g_args.size());
}

/*
=============
TestArgv

Returns the simulated engine argument at the requested index.
=============
*/
static const char* TestArgv(int index) {
	return g_args.at(static_cast<size_t>(index)).c_str();
}

/*
=============
main

Ensures CommandArgs snapshots engine arguments and remains stable.
=============
*/
int main() {
	g_args = { "cmd", "alpha", "beta" };
	gi.argc = &TestArgc;
	gi.argv = &TestArgv;

	CommandArgs args;
	assert(args.count() == 3);
	assert(args.getString(0) == "cmd");
	assert(args.getString(2) == "beta");
	assert(args.joinFrom(0) == "cmd alpha beta");

	g_args = { "cmd", "gamma" };
	assert(args.count() == 3);
	assert(args.getString(1) == "alpha");
	assert(args.joinFrom(1) == "alpha beta");

	CommandArgs manualArgs(std::vector<std::string>{ "manual", "delta", "epsilon" });
	g_args = { "cmd" };
	assert(manualArgs.count() == 3);
	assert(manualArgs.getString(2) == "epsilon");
	assert(manualArgs.joinFrom(0) == "manual delta epsilon");

	return 0;
}
