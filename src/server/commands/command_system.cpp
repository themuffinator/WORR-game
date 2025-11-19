// Copyright (c) DarkMatter Projects 2023-2025
// Licensed under the GNU General Public License 2.0.
//
// command_system.cpp - Core implementation of the command dispatcher.
// This file contains the command map, the main dispatcher logic, and the
// top-level registration function that calls into each command module.

#include "command_system.hpp"
#include "command_registration.hpp"
#include <unordered_map>
#include <string>

// The central registry for all client commands.
static std::unordered_map<std::string, Command, StringViewHash, std::equal_to<>> s_clientCommands;

namespace {
	static inline char AsciiToLower(char c) {
		if (c >= 'A' && c <= 'Z') {
			return static_cast<char>(c - 'A' + 'a');
		}
		return c;
	}

	static std::string NormalizeCommandKey(std::string_view name) {
		std::string key(name);
		for (char& ch : key) {
			ch = AsciiToLower(ch);
		}
		return key;
	}
}

// Definition of the global registration function.
void RegisterCommand(std::string_view name, void (*function)(gentity_t*, const CommandArgs&), BitFlags<CommandFlag> flags, bool floodExempt) {
	s_clientCommands[NormalizeCommandKey(name)] = { function, flags, floodExempt };
}

// Forward declarations for the registration functions in other files.
namespace Commands {
	void RegisterAdminCommands();
	void RegisterClientCommands();
	void RegisterVotingCommands();
	void RegisterCheatCommands();

	void PrintUsage(gentity_t* ent, const CommandArgs& args, std::string_view required_params, std::string_view optional_params, std::string_view help_text) {
		std::string usage = std::format("Usage: {} {}", args.getString(0), required_params);
		if (!optional_params.empty()) {
			usage += std::format(" {}", optional_params);
		}
		if (!help_text.empty()) {
			usage += std::format("\n{}", help_text);
		}
		gi.Client_Print(ent, PRINT_HIGH, usage.c_str());
	}

	// --- Helper Function Definitions ---

	static bool ValidateSocialIDFormat(std::string_view id) {
		size_t sep = id.find(':');
		if (sep == std::string_view::npos || sep == 0 || sep + 1 >= id.length())
			return false;

		std::string_view prefix = id.substr(0, sep);
		std::string_view value = id.substr(sep + 1);

		if (prefix == "EOS") {
			return value.length() == 32 && std::all_of(value.begin(), value.end(), [](char c) {
				return std::isxdigit(static_cast<unsigned char>(c));
				});
		}
		if (prefix == "Galaxy" || prefix == "NX") {
			return (value.length() >= 17 && value.length() <= 20) && std::all_of(value.begin(), value.end(), ::isdigit);
		}
		if (prefix == "GDK") {
			return (value.length() >= 15 && value.length() <= 17) && std::all_of(value.begin(), value.end(), ::isdigit);
		}
		if (prefix == "PSN") {
			return !value.empty() && std::all_of(value.begin(), value.end(), ::isdigit);
		}
		if (prefix == "Steamworks") {
			return value.starts_with("7656119") && std::all_of(value.begin(), value.end(), ::isdigit);
		}
		return false;
	}

	const char* ResolveSocialID(const char* rawArg, gentity_t*& foundClient) {
		std::string_view arg(rawArg);
		foundClient = ClientEntFromString(rawArg);
		if (foundClient && foundClient->client) {
			return foundClient->client->sess.socialID;
		}

		if (ValidateSocialIDFormat(arg)) {
			return rawArg;
		}

		foundClient = nullptr;
		return nullptr;
	}

	bool TeamSkillShuffle() {
		if (!Teams()) return false;

		std::vector<int> playerIndices;
		int totalSkill = 0;

		for (auto p_ent : active_players()) {
			const int clientIndex = p_ent->s.number - 1;
			if (clientIndex < 0 || clientIndex >= static_cast<int>(game.maxClients)) {
				continue;
			}

			playerIndices.push_back(clientIndex);
			totalSkill += p_ent->client->sess.skillRating;
		}

		if (playerIndices.size() < 2) return false;

		// Sort players by skill rating, descending
		std::ranges::sort(playerIndices, std::greater{}, [](int clientNum) {
			return game.clients[clientNum].sess.skillRating;
			});

		// Distribute players into teams like picking for a schoolyard game
		Team currentTeam = Team::Red;
		for (int clientNum : playerIndices) {
			game.clients[clientNum].sess.team = currentTeam;
			currentTeam = (currentTeam == Team::Red) ? Team::Blue : Team::Red;
		}

		gi.Broadcast_Print(PRINT_HIGH, "Teams have been shuffled based on skill.\n");
		Match_Reset();
		return true;
	}
}

bool CheckFlood(gentity_t* ent) {
	if (!flood_msgs->integer)
		return false;
	gclient_t* cl = ent->client;
	if (level.time < cl->flood.lockUntil) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_flood_cant_talk",
			(cl->flood.lockUntil - level.time).seconds<int32_t>());
		return true;
	}
	const size_t maxMsgs = static_cast<size_t>(flood_msgs->integer);
	const size_t bufferSize = std::size(cl->flood.messageTimes);
	size_t i = (static_cast<size_t>(cl->flood.time) + bufferSize - maxMsgs + 1) % bufferSize;
	if (cl->flood.messageTimes[i] && (level.time - cl->flood.messageTimes[i] < GameTime::from_sec(flood_persecond->value))) {
		cl->flood.lockUntil = level.time + GameTime::from_sec(flood_waitdelay->value);
		gi.LocClient_Print(ent, PRINT_CHAT, "$g_flood_cant_talk", flood_waitdelay->integer);
		return true;
	}
	cl->flood.time = static_cast<int>(
		(static_cast<size_t>(cl->flood.time) + 1) % bufferSize
		);
	cl->flood.messageTimes[cl->flood.time] = level.time;
	return false;
}

// Main registration function that orchestrates all others.
void RegisterAllCommands() {
	s_clientCommands.clear();
	Commands::RegisterAdminCommands();
	Commands::RegisterClientCommands();
	Commands::RegisterVotingCommands();
	Commands::RegisterCheatCommands();
}

// --- Permission Check Helpers ---
inline bool CheatsOk(gentity_t* ent) {
	if (!deathmatch->integer && !coop->integer) return true;
	if (!g_cheats->integer) {
		gi.Client_Print(ent, PRINT_HIGH, "Cheats must be enabled to use this command.\n");
		return false;
	}
	return true;
}

static inline bool AdminOk(gentity_t* ent) {
	if (!g_allowAdmin->integer || !ent->client->sess.admin) {
		gi.Client_Print(ent, PRINT_HIGH, "Only admins can use this command.\n");
		return false;
	}
	return true;
}


/*
=================
HasCommandPermission

Verifies that the client is allowed to execute the command based on
the current game state and command flags.
=================
*/
static inline bool HasCommandPermission(gentity_t* ent, const Command& cmd) {
	using enum CommandFlag;
	if (cmd.flags.has(AdminOnly) && !AdminOk(ent)) return false;
	if (cmd.flags.has(CheatProtect) && !CheatsOk(ent)) return false;
	if (!cmd.flags.has(AllowDead) && (ent->health <= 0 || ent->deadFlag)) return false;
	if (!cmd.flags.has(AllowSpectator) && !ClientIsPlaying(ent->client)) return false;
	if (cmd.flags.has(MatchOnly) && !InAMatch()) return false;
	if (!cmd.flags.has(AllowIntermission) && (level.intermission.time || level.intermission.postIntermissionTime)) return false;
	return true;
}

/*
=================
ClientCommand

Central command entrypoint. Looks up the command in the
registry, applies flood/permission checks, and executes it.
Falls back to dynamic cvar force set for replace_* / disable_*.
=================
*/
void ClientCommand(gentity_t* ent) {
	if (!ent || !ent->client) {
		return; // not fully in game yet
	}

	CommandArgs args;
	std::string_view commandName = args.getString(0);
	if (commandName.empty()) {
		return;
	}

	// Fast lookup in the heterogeneous s_clientCommands map
	auto it = s_clientCommands.find(NormalizeCommandKey(commandName));
	if (it == s_clientCommands.end()) {
		// Dynamic cvar fallback (parity with legacy behavior)
		// Example: "replace_gun 0" or "disable_powerups 1"
		if (args.count() > 1) {
			const char* raw = gi.argv(0);
			if (raw && (strstr(raw, "replace_") || strstr(raw, "disable_"))) {
				gi.cvarForceSet(raw, gi.argv(1));
				return;
			}
		}
		gi.LocClient_Print(ent, PRINT_HIGH, "Unknown command: '{}'\n", commandName.data());
		return;
	}

	const Command& cmd = it->second;

	// Optional per-command flood check (keeps your existing behavior)
	// Uses your existing CheckFlood(ent) gate and the command's floodExempt flag.
	// If you later decide to use a separate command-flood window, wire it here.
	if (!cmd.floodExempt && CheckFlood(ent)) {
		return;
	}

	// Permission gates: admin/cheat/intermission/spectator/dead/match-only
	if (!HasCommandPermission(ent, cmd)) {
		return;
	}

	// Execute
	cmd.function(ent, args);
}
