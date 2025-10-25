// Copyright (c) DarkMatter Projects 2023-2025
// Licensed under the GNU General Public License 2.0.
//
// commands_voting.cpp - Implements the voting system commands.
// This module contains all logic for calling votes, casting votes,
// and processing the results for various game actions.

#include "command_voting.hpp"
#include "g_local.hpp"
#include "command_registration.hpp"
#include "command_voting_utils.hpp"
#include "command_system.hpp"
#include <string>
#include <vector>
#include <format>
#include <unordered_map>
#include <sstream>
#include <string_view>

namespace Commands {

	// --- Forward Declarations ---
	void CallVote(gentity_t* ent, const CommandArgs& args);
	void Vote(gentity_t* ent, const CommandArgs& args);

	// --- Voting System Internals ---

	// Use a map for efficient O(1) lookup of vote commands.
        static std::unordered_map<std::string, VoteCommand, StringViewHash, std::equal_to<>> s_voteCommands;
        static std::vector<VoteDefinitionView> s_voteDefinitions;

        bool IsVoteCommandEnabled(std::string_view name) {
                if (!g_allowVoting || !g_allowVoting->integer) {
                        return false;
                }

                auto it = s_voteCommands.find(name);
                if (it == s_voteCommands.end()) {
                        return false;
                }

                const VoteCommand& cmd = it->second;
                return (g_vote_flags->integer & cmd.flag) != 0;
        }

	// Helper to make registering vote commands clean and consistent.
        static void RegisterVoteCommand(
                std::string_view name,
                bool (*validateFn)(gentity_t*, const CommandArgs&),
                void (*executeFn)(),
                int32_t flag,
                int8_t minArgs,
                std::string_view argsUsage,
                std::string_view helpText,
                bool visibleInMenu = true)
        {
                auto [iter, inserted] = s_voteCommands.emplace(std::string(name), VoteCommand{ name, validateFn, executeFn, flag, minArgs, argsUsage, helpText });
                if (!inserted) {
                        iter->second = { name, validateFn, executeFn, flag, minArgs, argsUsage, helpText };
                }
                s_voteDefinitions.push_back({ iter->first, flag, visibleInMenu });
        }


	// --- Vote Execution Functions ("Pass_*") ---

	static void Pass_Map() {
		const MapEntry* map = game.mapSystem.GetMapEntry(level.vote.arg);
		if (!map) {
			gi.Com_Print("Error: Map not found in pool at vote pass stage.\n");
			return;
		}
		level.changeMap = map->filename.c_str();
		game.map.overrideEnableFlags = level.vote_flags_enable;
		game.map.overrideDisableFlags = level.vote_flags_disable;
		ExitLevel();
	}

	static void Pass_NextMap() {
		if (!game.mapSystem.playQueue.empty()) {
			const auto& queued = game.mapSystem.playQueue.front();
			level.changeMap = queued.filename.c_str();
			game.map.overrideEnableFlags = queued.settings.to_ulong();
			game.map.overrideDisableFlags = 0;
			ExitLevel();
			return;
		}

		auto result = AutoSelectNextMap();
		if (result.has_value()) {
			level.changeMap = result->filename.c_str();
			game.map.overrideEnableFlags = 0;
			game.map.overrideDisableFlags = 0;
			ExitLevel();
		}
		else {
			gi.Broadcast_Print(PRINT_HIGH, "No eligible maps available.\n");
		}
	}

	static void Pass_RestartMatch() { Match_Reset(); }
	static void Pass_ShuffleTeams() { TeamSkillShuffle(); }
	static void Pass_BalanceTeams() { TeamBalance(true); }

	static void Pass_Gametype() {
		auto gt = Game::FromString(level.vote.arg);
		if (gt) {
			ChangeGametype(*gt);
		}
	}

	static void Pass_Ruleset() {
		ruleset_t rs = RS_IndexFromString(level.vote.arg.c_str());
		if (rs != RS_NONE) {
			std::string cvar_val = std::format("{}", static_cast<int>(rs));
			gi.cvarForceSet("g_ruleset", cvar_val.c_str());
		}
	}

	static void Pass_Timelimit() {
		if (auto val = CommandArgs::ParseInt(level.vote.arg)) {
			if (*val == 0) {
				gi.Broadcast_Print(PRINT_HIGH, "Time limit has been DISABLED.\n");
			}
			else {
				gi.LocBroadcast_Print(PRINT_HIGH, "Time limit has been set to {}.\n", TimeString(*val * 60000, false, false));
			}
			gi.cvarForceSet("timeLimit", level.vote.arg.c_str());
		}
	}

	static void Pass_Scorelimit() {
		if (auto val = CommandArgs::ParseInt(level.vote.arg)) {
			if (*val == 0) {
				gi.Broadcast_Print(PRINT_HIGH, "Score limit has been DISABLED.\n");
			}
			else {
				gi.LocBroadcast_Print(PRINT_HIGH, "Score limit has been set to {}.\n", *val);
			}
			std::string limitCvar = std::format("{}limit", GT_ScoreLimitString());
			gi.cvarForceSet(limitCvar.c_str(), level.vote.arg.c_str());
		}
	}

	// --- Vote Validation Functions ("Validate_*") ---

	static bool Validate_None(gentity_t* ent, const CommandArgs& args) { return true; }

	static bool Validate_Map(gentity_t* ent, const CommandArgs& args) {
		std::string_view mapName = args.getString(2);
		const MapEntry* map = game.mapSystem.GetMapEntry(std::string(mapName));

		if (!map) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Map '{}' not found in map pool.\n", mapName.data());
			return false;
		}
		if (map->lastPlayed) {
			int64_t timeSince = GetCurrentRealTimeMillis() - map->lastPlayed;
			if (timeSince < 1800000) {
				gi.LocClient_Print(ent, PRINT_HIGH, "Map '{}' was played recently, please wait {}.\n", mapName.data(), FormatDuration(1800000 - timeSince).c_str());
				return false;
			}
		}
		return true;
	}

	static bool Validate_Gametype(gentity_t* ent, const CommandArgs& args) {
		if (!Game::FromString(args.getString(2))) {
			gi.Client_Print(ent, PRINT_HIGH, "Invalid gametype.\n");
			return false;
		}
		return true;
	}

	static bool Validate_Ruleset(gentity_t* ent, const CommandArgs& args) {
		ruleset_t desired_rs = RS_IndexFromString(args.getString(2).data());
		if (desired_rs == RS_NONE) {
			gi.Client_Print(ent, PRINT_HIGH, "Invalid ruleset.\n");
			return false;
		}
		if (desired_rs == game.ruleset) {
			gi.Client_Print(ent, PRINT_HIGH, "That ruleset is already active.\n");
			return false;
		}
		return true;
	}

	static bool Validate_Timelimit(gentity_t* ent, const CommandArgs& args) {
		auto limit = args.getInt(2);
		if (!limit || *limit < 0 || *limit > 1440) {
			gi.Client_Print(ent, PRINT_HIGH, "Invalid time limit value.\n");
			return false;
		}
		if (*limit == timeLimit->integer) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Time limit is already set to {}.\n", *limit);
			return false;
		}
		return true;
	}

	static bool Validate_Scorelimit(gentity_t* ent, const CommandArgs& args) {
		auto limit = args.getInt(2);
		if (!limit || *limit < 0) {
			gi.Client_Print(ent, PRINT_HIGH, "Invalid score limit value.\n");
			return false;
		}
		if (*limit == GT_ScoreLimit()) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Score limit is already set to {}.\n", *limit);
			return false;
		}
		return true;
	}

	static bool Validate_TeamBased(gentity_t* ent, const CommandArgs& args) {
		if (!Teams()) {
			gi.Client_Print(ent, PRINT_HIGH, "This vote is only available in team-based gametypes.\n");
			return false;
		}
		return true;
	}


        static void RegisterAllVoteCommands() {
                s_voteCommands.clear();
                s_voteDefinitions.clear();
                RegisterVoteCommand("map", &Validate_Map, &Pass_Map, 1, 2, "<mapname> [flags]", "Changes to the specified map");
                RegisterVoteCommand("nextmap", &Validate_None, &Pass_NextMap, 2, 1, "", "Moves to the next map in the rotation");
                RegisterVoteCommand("restart", &Validate_None, &Pass_RestartMatch, 4, 1, "", "Restarts the current match");
                RegisterVoteCommand("gametype", &Validate_Gametype, &Pass_Gametype, 8, 2, "<gametype>", "Changes the current gametype");
		RegisterVoteCommand("timelimit", &Validate_Timelimit, &Pass_Timelimit, 16, 2, "<minutes>", "Alters the match time limit (0 for none)");
		RegisterVoteCommand("scorelimit", &Validate_Scorelimit, &Pass_Scorelimit, 32, 2, "<score>", "Alters the match score limit (0 for none)");
		RegisterVoteCommand("shuffle", &Validate_TeamBased, &Pass_ShuffleTeams, 64, 1, "", "Shuffles the teams based on skill");
		RegisterVoteCommand("balance", &Validate_TeamBased, &Pass_BalanceTeams, 1024, 1, "", "Balances teams without shuffling");
		RegisterVoteCommand("ruleset", &Validate_Ruleset, &Pass_Ruleset, 2048, 2, "<q1|q2|q3a>", "Changes the current ruleset");
	}

        static void VoteCommandStore(
                gentity_t* ent,
                const VoteCommand* vote_cmd,
                std::string_view arg,
                std::string_view displayArg = {})
        {
                level.vote.client = ent->client;
                level.vote.time = level.time;
                level.vote.countYes = 1;
                level.vote.countNo = 0;
                level.vote.cmd = vote_cmd;
                level.vote.arg = arg;

                std::string_view effectiveArg = displayArg.empty() ? arg : displayArg;

                gi.LocBroadcast_Print(PRINT_CENTER, "{} called a vote:\n{}{}\n",
                        level.vote.client->sess.netName,
                        vote_cmd->name.data(),
                        effectiveArg.empty() ? "" : std::format(" {}", effectiveArg).c_str());

		for (auto ec : active_clients()) {
			ec->client->pers.voted = (ec == ent) ? 1 : 0;
		}

		ent->client->pers.vote_count++;
		AnnouncerSound(world, "vote_now");

		for (auto ec : active_players()) {
			if (ec->svFlags & SVF_BOT) continue;
			if (ec == ent) continue;
			if (!ClientIsPlaying(ec->client) && !g_allowSpecVote->integer) continue;

			CloseActiveMenu(ec);
			OpenVoteMenu(ec);
		}
        }


        std::span<const VoteDefinitionView> GetRegisteredVoteDefinitions() {
                return s_voteDefinitions;
        }

        bool TryLaunchVote(gentity_t* ent, std::string_view voteName, std::string_view voteArg) {
                if (!g_allowVoting || !g_allowVoting->integer) {
                        return false;
                }
                if (level.vote.time || level.vote.executeTime || level.restarted) {
                        return false;
                }
                if (!g_allowVoteMidGame->integer && level.matchState >= MatchState::Countdown) {
                        return false;
                }
                if (g_vote_limit->integer && ent->client->pers.vote_count >= g_vote_limit->integer) {
                        return false;
                }
                if (!ClientIsPlaying(ent->client) && !g_allowSpecVote->integer) {
                        return false;
                }

                auto it = s_voteCommands.find(voteName);
                if (it == s_voteCommands.end()) {
                        return false;
                }

                const VoteCommand& found_cmd = it->second;
                if ((g_vote_flags->integer & found_cmd.flag) == 0) {
                        return false;
                }

                std::vector<std::string> args;
                args.reserve(voteArg.empty() ? 2 : 3);
                args.emplace_back("callvote");
                args.emplace_back(voteName);
                if (!voteArg.empty()) {
                        args.emplace_back(voteArg);
                }

                CommandArgs manualArgs(std::move(args));
                if (manualArgs.count() < (1 + found_cmd.minArgs)) {
                        return false;
                }

                if (!found_cmd.validate || !found_cmd.validate(ent, manualArgs)) {
                        return false;
                }

                std::string_view storedArg;
                if (manualArgs.count() >= 3) {
                        storedArg = manualArgs.getString(2);
                }

                VoteCommandStore(ent, &found_cmd, storedArg);
                return true;
        }


        // --- Main Command Functions ---

	void CallVote(gentity_t* ent, const CommandArgs& args) {
		if (!g_allowVoting->integer) {
			gi.Client_Print(ent, PRINT_HIGH, "Voting is disabled on this server.\n");
			return;
		}
		if (level.vote.time) {
			gi.Client_Print(ent, PRINT_HIGH, "A vote is already in progress.\n");
			return;
		}
		if (!g_allowVoteMidGame->integer && level.matchState >= MatchState::Countdown) {
			gi.Client_Print(ent, PRINT_HIGH, "Voting is only allowed during warmup.\n");
			return;
		}
		if (g_vote_limit->integer && ent->client->pers.vote_count >= g_vote_limit->integer) {
			gi.LocClient_Print(ent, PRINT_HIGH, "You have called the maximum number of votes ({}).\n", g_vote_limit->integer);
			return;
		}
		if (!ClientIsPlaying(ent->client) && !g_allowSpecVote->integer) {
			gi.Client_Print(ent, PRINT_HIGH, "Spectators cannot call a vote on this server.\n");
			return;
		}
		if (level.vote.time) {
			gi.Client_Print(ent, PRINT_HIGH, "A vote is already in progress.\n");
			return;
		}
		if (level.vote.executeTime || level.restarted) {
			gi.Client_Print(ent, PRINT_HIGH, "Cannot start a vote right now.\n");
			return;
		}

		if (args.count() < 2) {
			PrintUsage(ent, args, "<command>", "[params]", "Call a vote to change a server setting.");
			return;
		}

		std::string_view voteName = args.getString(1);
		auto it = s_voteCommands.find(voteName);

		if (it == s_voteCommands.end()) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Invalid vote command: '{}'.\n", voteName.data());
			return;
		}
		const VoteCommand& found_cmd = it->second;

		if (args.count() < (1 + found_cmd.minArgs)) {
			PrintUsage(ent, args, found_cmd.name, found_cmd.argsUsage, found_cmd.helpText);
			return;
		}

		if (found_cmd.validate(ent, args)) {

			if ((g_vote_flags->integer & found_cmd.flag) == 0) {
				gi.Client_Print(ent, PRINT_HIGH, "That vote type is disabled on this server.\n");
				return;
			}

                        level.vote_flags_enable = 0;
                        level.vote_flags_disable = 0;

                        std::string vote_arg_str;
                        std::string vote_display_str;

                        if (found_cmd.name == "map") {
                                std::vector<std::string> mapArgs;
                                for (int i = 2; i < args.count(); ++i) {
                                        mapArgs.emplace_back(args.getString(i));
                                }

                                std::string parseError;
                                auto parsed = ParseMapVoteArguments(mapArgs, parseError);
                                if (!parsed) {
                                        gi.LocClient_Print(ent, PRINT_HIGH, "{}\n", parseError.c_str());
                                        return;
                                }

                                vote_arg_str = parsed->mapName;
                                vote_display_str = parsed->displayArg;
                                level.vote_flags_enable = parsed->enableFlags;
                                level.vote_flags_disable = parsed->disableFlags;
                        }
                        else {
                                if (args.count() >= 3) {
                                        vote_arg_str = args.getString(2);
                                }
                        }

                        VoteCommandStore(ent, &found_cmd, vote_arg_str, vote_display_str);
                }
        }

	void Vote(gentity_t* ent, const CommandArgs& args) {
		if (!level.vote.time) {
			gi.Client_Print(ent, PRINT_HIGH, "No vote in progress.\n");
			return;
		}
		if (ent->client->pers.voted != 0) {
			gi.Client_Print(ent, PRINT_HIGH, "You have already voted.\n");
			return;
		}
		if (args.count() < 2) {
			PrintUsage(ent, args, "<yes|no>", "", "Casts your vote.");
			return;
		}

		// Accept "1"/"0" as well for convenience.
		std::string arg = (args.count() > 1) ? std::string(args.getString(1)) : "";
		std::ranges::transform(arg, arg.begin(), ::tolower);
		if (arg == "1") arg = "yes";
		if (arg == "0") arg = "no";
		if (arg != "yes" && arg != "y" && arg != "no" && arg != "n") {
			PrintUsage(ent, args, "<yes|no>", "", "Cast your vote.");
			return;
		}

		std::string_view vote = args.getString(1);
		if (vote == "yes" || vote == "y") {
			level.vote.countYes++;
			ent->client->pers.voted = 1;
		}
		else if (vote == "no" || vote == "n") {
			level.vote.countNo++;
			ent->client->pers.voted = -1;
		}
		else {
			PrintUsage(ent, args, "<yes|no>", "", "Casts your vote.");
			return;
		}

		gi.Client_Print(ent, PRINT_HIGH, "Vote cast.\n");
	}


	// --- Registration Function ---
	void RegisterVotingCommands() {
		RegisterAllVoteCommands();
		using enum CommandFlag;
		RegisterCommand("callvote", &CallVote, AllowDead | AllowSpectator);
		RegisterCommand("cv", &CallVote, AllowDead | AllowSpectator); // Alias
		RegisterCommand("vote", &Vote, AllowDead);
	}

} // namespace Commands

namespace {

void ResetActiveVoteState() {
        for (auto ent : active_clients()) {
                if (ent->client) {
                        ent->client->pers.voted = 0;
                }
        }

        level.vote.cmd = nullptr;
        level.vote.client = nullptr;
        level.vote.arg.clear();
        level.vote.countYes = 0;
        level.vote.countNo = 0;
        level.vote.time = 0_sec;
        level.vote.executeTime = 0_sec;
        level.vote_flags_enable = 0;
        level.vote_flags_disable = 0;
}

} // namespace


void G_RevertVote(gclient_t *client) {
        if (!client) {
                return;
        }

        if (client->pers.voted == 1 && level.vote.countYes > 0) {
                level.vote.countYes--;
        } else if (client->pers.voted == -1 && level.vote.countNo > 0) {
                level.vote.countNo--;
        }

        client->pers.voted = 0;

        if (level.vote.client == client) {
                gi.Broadcast_Print(PRINT_HIGH, "Vote cancelled because the caller disconnected.\n");
                AnnouncerSound(world, "vote_failed");
                ResetActiveVoteState();
                return;
        }
}


void Vote_Passed() {
        const VoteCommand *command = level.vote.cmd;
        if (!command) {
                gi.Com_Print("Vote_Passed called without an active command.\n");
        } else if (command->execute) {
                command->execute();
        }

        ResetActiveVoteState();
}
