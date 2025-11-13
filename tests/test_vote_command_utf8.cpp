#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

enum print_type_t { PRINT_HIGH };

struct gclient_pers_t {
	int voted = 0;
};

struct gclient_t {
	gclient_pers_t pers;
};

struct gentity_t {
	gclient_t* client = nullptr;
};

struct GameTime {
	bool value = false;

	static constexpr GameTime from_ms(int) {
		return GameTime{ true };
	}

	constexpr explicit operator bool() const {
		return value;
	}
};

struct VotingState {
	GameTime time{};
	int8_t countYes = 0;
	int8_t countNo = 0;
};

struct LevelLocals {
	VotingState vote{};
} level;

struct local_game_import_t {
	void (*Client_Print)(gentity_t*, print_type_t, const char*) = nullptr;
} gi;

class CommandArgs {
public:
	explicit CommandArgs(std::vector<std::string> args)
		: _args(std::move(args)) {
	}

	int count() const {
		return static_cast<int>(_args.size());
	}

	std::string_view getString(int index) const {
		if (index < 0 || index >= static_cast<int>(_args.size())) {
			return {};
		}
		return _args[static_cast<size_t>(index)];
	}

private:
	std::vector<std::string> _args;
};

namespace {
	static std::vector<std::string> g_printMessages;
	static int g_usageCount = 0;
}

/*
=============
StubClientPrint

Captures calls to the mocked Client_Print function for verification.
=============
*/
static void StubClientPrint(gentity_t*, print_type_t, const char* message) {
	if (message) {
		g_printMessages.emplace_back(message);
	}
}

/*
=============
PrintUsage

Stubbed usage helper to track invocation counts during tests.
=============
*/
void PrintUsage(gentity_t*, const CommandArgs&, std::string_view, std::string_view, std::string_view) {
	++g_usageCount;
}

namespace Commands {
#include "server/commands/command_voting_vote.inl"
}

/*
=============
main

Verifies that vote input normalization safely handles ASCII extensions and valid uppercase strings.
=============
*/
int main() {
	gi.Client_Print = &StubClientPrint;
	level.vote.time = GameTime::from_ms(1);
	level.vote.countYes = 0;
	level.vote.countNo = 0;

	gclient_t client{};
	gentity_t entity{};
	entity.client = &client;

	CommandArgs yesArgs(std::vector<std::string>{ "vote", "YES" });
	Commands::Vote(&entity, yesArgs);
	assert(level.vote.countYes == 1);
	assert(level.vote.countNo == 0);
	assert(entity.client->pers.voted == 1);
	assert(!g_printMessages.empty());
	assert(g_printMessages.back() == "Vote cast.\n");

	entity.client->pers.voted = 0;
	level.vote.countYes = 0;
	level.vote.countNo = 0;
	g_printMessages.clear();
	g_usageCount = 0;

	std::string utf8Yes = "Y\xC3\x89S";
	CommandArgs utf8Args(std::vector<std::string>{ "vote", utf8Yes });
	Commands::Vote(&entity, utf8Args);
	assert(level.vote.countYes == 0);
	assert(level.vote.countNo == 0);
	assert(entity.client->pers.voted == 0);
	assert(g_usageCount == 1);
	assert(g_printMessages.empty());

	return 0;
}
