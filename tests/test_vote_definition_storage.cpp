/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_vote_definition_storage.cpp implementation.*/

#include "server/commands/command_voting.hpp"
#include <array>
#include <cassert>
#include <cmath>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

/*
=============
main

Ensures vote definition names remain stable even if the command map rehashes repeatedly.
=============
*/
int main() {
	std::array<std::string_view, 12> voteNames = {
		"map",
		"nextmap",
		"restart",
		"gametype",
		"ruleset",
		"timelimit",
		"scorelimit",
		"shuffle",
		"balance",
		"unlagged",
		"cointoss",
		"random"
	};

	std::unordered_map<std::string, int> commandMap;
	std::vector<Commands::VoteDefinitionView> definitions;
	definitions.reserve(voteNames.size());

	for (std::string_view name : voteNames) {
		auto [iter, inserted] = commandMap.emplace(std::string(name), 0);
		assert(inserted);
		definitions.push_back({ iter->first, 0, true });
	}

	commandMap.rehash(static_cast<size_t>(commandMap.size() * 4 + 1));

	for (size_t i = 0; i < voteNames.size(); ++i) {
		assert(definitions[i].name == voteNames[i]);
		assert(!definitions[i].name.empty());
	}

	return 0;
}
