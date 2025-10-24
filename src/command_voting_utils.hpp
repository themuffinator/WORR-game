#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct gentity_t;

bool ParseMyMapFlags(const std::vector<std::string> &args, uint8_t &enableFlags, uint8_t &disableFlags);

namespace Commands {

struct MapVoteParseResult {
        std::string mapName;
        std::string displayArg;
        uint8_t enableFlags = 0;
        uint8_t disableFlags = 0;
};

inline std::optional<MapVoteParseResult> ParseMapVoteArguments(
        const std::vector<std::string> &args,
        std::string &errorMessage)
{
        MapVoteParseResult result;

        if (args.empty()) {
                errorMessage = "Map name is required.";
                return std::nullopt;
        }

        result.mapName = args.front();
        result.displayArg = result.mapName;

        if (args.size() > 1) {
                std::vector<std::string> flagArgs(args.begin() + 1, args.end());
                if (!ParseMyMapFlags(flagArgs, result.enableFlags, result.disableFlags)) {
                        errorMessage = "Invalid map flag syntax. Use +flag/-flag (e.g. +pu -fd).";
                        return std::nullopt;
                }

                for (const auto &flag : flagArgs) {
                        result.displayArg.push_back(' ');
                        result.displayArg.append(flag);
                }
        }

        errorMessage.clear();
        return result;
}

} // namespace Commands
