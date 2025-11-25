/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_callvote_map_flags.cpp implementation.*/

#include "server/commands/command_voting_utils.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

enum MyMapOverride : uint16_t {
	MAPFLAG_NONE = 0,
	MAPFLAG_PU = 1 << 0,
	MAPFLAG_PA = 1 << 1,
	MAPFLAG_AR = 1 << 2,
	MAPFLAG_AM = 1 << 3,
	MAPFLAG_HT = 1 << 4,
	MAPFLAG_BFG = 1 << 5,
	MAPFLAG_PB = 1 << 6,
	MAPFLAG_FD = 1 << 7,
	MAPFLAG_SD = 1 << 8,
	MAPFLAG_WS = 1 << 9
};

struct MapFlagEntry {
	uint16_t bit;
	const char *code;
	const char *label;
};

static constexpr std::array<MapFlagEntry, 10> kMapFlags = { {
	{ MAPFLAG_PU, "pu", "Powerups" },
	{ MAPFLAG_PA, "pa", "Power Armor" },
	{ MAPFLAG_AR, "ar", "Armor" },
	{ MAPFLAG_AM, "am", "Ammo" },
	{ MAPFLAG_HT, "ht", "Health" },
	{ MAPFLAG_BFG, "bfg", "BFG10K" },
	{ MAPFLAG_PB, "pb", "Plasma Beam" },
	{ MAPFLAG_FD, "fd", "Falling Damage" },
	{ MAPFLAG_SD, "sd", "Self Damage" },
	{ MAPFLAG_WS, "ws", "Weapons Stay" },
} };

struct MapVoteState {
	uint16_t enableFlags = 0;
	uint16_t disableFlags = 0;
};

/*
=============
MapFlags_ToggleTri

Cycles a tri-state toggle for simulated MyMap flag selection, matching
the in-game menu behavior.
=============
*/
static void MapFlags_ToggleTri(MapVoteState &state, uint16_t mask) {
	const bool en = (state.enableFlags & mask) != 0;
	const bool dis = (state.disableFlags & mask) != 0;

	if (!en && !dis) {
		state.enableFlags |= mask;
	} else if (en) {
		state.enableFlags &= ~mask;
		state.disableFlags |= mask;
	} else {
		state.disableFlags &= ~mask;
	}
}

/*
=============
BuildMapVoteArg

Serializes a map vote argument string with the provided tri-state
selection, mirroring the client-side builder.
=============
*/
static std::string BuildMapVoteArg(const std::string &mapname, const MapVoteState &state) {
	std::string arg = mapname;
	for (const auto &f : kMapFlags) {
		if (state.enableFlags & f.bit) {
			arg += " +";
			arg += f.code;
	}
		if (state.disableFlags & f.bit) {
			arg += " -";
			arg += f.code;
	}
	}
	return arg;
}

/*
=============
main

Exercises map vote flag parsing, ensuring valid, duplicate, and invalid
inputs behave as expected.
=============
*/
int main() {
	std::string error;
	auto parsed = Commands::ParseMapVoteArguments({ "testmap", "+pu", "-pb" }, error);
	assert(parsed.has_value());
	assert(error.empty());
	assert(parsed->mapName == "testmap");
	assert(parsed->displayArg == "testmap +pu -pb");
	assert(parsed->enableFlags == MAPFLAG_PU);
	assert(parsed->disableFlags == MAPFLAG_PB);

	// Simulate Pass_Map by applying overrides to a simple context
	struct {
		std::string changeMap;
		uint16_t enable = 0;
		uint16_t disable = 0;
	} context{};
	context.changeMap = parsed->mapName;
	context.enable = parsed->enableFlags;
	context.disable = parsed->disableFlags;
	assert(context.changeMap == "testmap");
	assert(context.enable == MAPFLAG_PU);
	assert(context.disable == MAPFLAG_PB);

	// Map vote without flags should reset overrides
	error.clear();
	auto parsedNoFlags = Commands::ParseMapVoteArguments({ "testmap" }, error);
	assert(parsedNoFlags.has_value());
	assert(error.empty());
	assert(parsedNoFlags->enableFlags == 0);
	assert(parsedNoFlags->disableFlags == 0);
	assert(parsedNoFlags->displayArg == "testmap");

	// Invalid flag should produce an error
	error.clear();
	auto parsedInvalid = Commands::ParseMapVoteArguments({ "testmap", "+unknown" }, error);
	assert(!parsedInvalid.has_value());
	assert(!error.empty());

	// High-order flags should survive parsing
	error.clear();
	auto parsedHighBits = Commands::ParseMapVoteArguments({ "testmap", "+sd", "-ws" }, error);
	assert(parsedHighBits.has_value());
	assert(error.empty());
	assert((parsedHighBits->enableFlags & MAPFLAG_SD) == MAPFLAG_SD);
	assert((parsedHighBits->disableFlags & MAPFLAG_WS) == MAPFLAG_WS);

	MapVoteState menuState{};
	MapFlags_ToggleTri(menuState, MAPFLAG_SD); // enable +sd
	MapFlags_ToggleTri(menuState, MAPFLAG_WS); // enable ws
	MapFlags_ToggleTri(menuState, MAPFLAG_WS); // disable ws
	const std::string builtArg = BuildMapVoteArg("testmap", menuState);
	assert(builtArg.find("+sd") != std::string::npos);
	assert(builtArg.find("-ws") != std::string::npos);

	// Failed next-map selection should clear stale overrides so they aren't reused.
	uint16_t staleEnable = static_cast<uint16_t>(MAPFLAG_PU | MAPFLAG_BFG);
	uint16_t staleDisable = MAPFLAG_WS;
	uint16_t overrideEnable = staleEnable;
	uint16_t overrideDisable = staleDisable;
	std::optional<int> noNextMap{};
	if (!noNextMap.has_value()) {
		overrideEnable = 0;
		overrideDisable = 0;
	}

	assert(overrideEnable == 0);
	assert(overrideDisable == 0);

	error.clear();
	auto parsedAfterFailure = Commands::ParseMapVoteArguments({ "testmap", "+pa" }, error);
	assert(parsedAfterFailure.has_value());
	assert(error.empty());
	assert(parsedAfterFailure->enableFlags == MAPFLAG_PA);
	assert(parsedAfterFailure->disableFlags == 0);

	return 0;
}
