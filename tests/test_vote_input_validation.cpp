/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_vote_input_validation.cpp implementation.*/

#include "server/commands/command_validation.hpp"
#include <cassert>
#include <string>

/*
=============
main

Verifies printable ASCII validation for vote-related input helpers.
=============
*/
int main() {
	std::string error;

	assert(!Commands::ValidatePrintableASCII("", "Vote command", error));
	assert(error == "Vote command is required.");

	error.clear();
	assert(!Commands::ValidatePrintableASCII("bad\x01name", "Vote command", error));
	assert(error == "Vote command must use printable ASCII characters.");

	error.clear();
	assert(Commands::ValidatePrintableASCII("shuffle", "Vote command", error));
	assert(error.empty());

	return 0;
}
