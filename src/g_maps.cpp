// g_maps.cpp (Game Maps)
// This file manages the map loading, rotation, and voting systems for
// multiplayer matches. It is responsible for parsing map lists, selecting the
// next map to be played, and handling the end-of-match map voting screen.
//
// Key Responsibilities:
// - Map Database: `LoadMapPool` reads a JSON file (`mapdb.json`) to create an
//   internal database of all available maps and their properties (e.g., name,
//   supported gametypes, player count).
// - Map Cycle: `LoadMapCycle` reads a text file (`mapcycle.txt`) to determine
//   which maps from the pool are part of the regular rotation.
// - Next Map Selection: `AutoSelectNextMap` contains the logic for automatically
//   choosing the next map, considering factors like player count, map popularity,
//   and avoiding recent repeats.
// - Map Voting: Implements the `MapSelector` system, which presents players with
//   a choice of maps at the end of a match and transitions to the winning map.
// - "MyMap" Queue: Manages a player-driven queue where users can vote to play
//   a specific map next.

#include "g_local.hpp"
#include <json/json.h>
#include <fstream>
#include <regex>

/*
=============
MapSelectorFinalize
=============
*/
void CloseActiveMenu(gentity_t *ent);
void MapSelectorFinalize() {
	auto &ms = level.mapSelector;

	if (ms.voteStartTime == 0_sec)
		return;

	// Close menus for all players
	for (auto ec : active_players()) {
		CloseActiveMenu(ec);
		ec->client->showScores = false;
		ec->client->showInventory = false;
	}

	// Tally votes
	std::fill(ms.voteCounts.begin(), ms.voteCounts.end(), 0);
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		int vote = ms.votes[i];
		if (vote >= 0 && vote < static_cast<int>(ms.candidates.size()) && ms.candidates[vote]) {
			ms.voteCounts[vote]++;
		}
	}

	// Find max vote count
	int maxVotes = *std::max_element(ms.voteCounts.begin(), ms.voteCounts.end());

	// Find all tied candidates
	std::vector<int> tiedIndices;
	for (size_t i = 0; i < ms.candidates.size(); ++i) {
		if (ms.candidates[i] && ms.voteCounts[i] == maxVotes)
			tiedIndices.push_back(static_cast<int>(i));
	}

	int selectedIndex = -1;

	if (!tiedIndices.empty() && maxVotes > 0) {
		selectedIndex = tiedIndices[rand() % tiedIndices.size()];
	} else {
		// No votes or all invalid - fallback
		std::vector<int> available;
		for (size_t i = 0; i < ms.candidates.size(); ++i) {
			if (ms.candidates[i])
				available.push_back(static_cast<int>(i));
		}
		if (!available.empty())
			selectedIndex = available[rand() % available.size()];
	}

	if (selectedIndex >= 0 && ms.candidates[selectedIndex]) {
		const MapEntry *selected = ms.candidates[selectedIndex];
		level.changeMap = selected->filename.c_str();

		gi.LocBroadcast_Print(PRINT_CENTER, ".Map vote complete!\nNext map: {} ({})\n",
			selected->filename.c_str(),
			selected->longName.empty() ? selected->filename.c_str() : selected->longName.c_str());

		AnnouncerSound(world, "vote_passed");
	} else {
		auto fallback = AutoSelectNextMap();
		if (fallback) {
			level.changeMap = fallback->filename.c_str();
			gi.LocBroadcast_Print(PRINT_CENTER, ".Map vote failed.\nRandomly selected: {} ({})\n",
				fallback->filename.c_str(),
				fallback->longName.empty() ? fallback->filename.c_str() : fallback->longName.c_str());
		} else {
			gi.Broadcast_Print(PRINT_CENTER, ".Map vote failed.\nNo maps available for next match.\n");
		}
		AnnouncerSound(world, "vote_failed");
	}
	
        ms.voteStartTime = 0_sec;
        level.intermission.exit = true;
}

/*
=============
MapSelectorBegin
=============
*/
void MapSelectorBegin() {
	auto &ms = level.mapSelector;

	if (ms.voteStartTime != 0_sec)
		return; // already started

	// Defensive reset
	ms.votes.fill(-1);
	ms.voteCounts.fill(0);
	ms.candidates.fill(nullptr);

	auto candidates = MapSelectorVoteCandidates();
	if (candidates.empty())
		return;

	for (size_t i = 0; i < std::min(candidates.size(), ms.candidates.size()); ++i)
		ms.candidates[i] = candidates[i];

	// Set voteStartTime here to lock vote as active
	ms.voteStartTime = level.time;

	for (auto ec : active_players()) {
		OpenMapSelectorMenu(ec);
	}

	gi.LocBroadcast_Print(PRINT_HIGH, "Voting has started for the next map!\nYou have {} seconds to vote.\n", MAP_SELECTOR_DURATION.seconds());
	AnnouncerSound(world, "vote_now");
}

/*
====================
MapSelector_CastVote
====================
*/
void MapSelector_CastVote(gentity_t *ent, int voteIndex) {
	if (!ent || !ent->client || voteIndex < 0 || voteIndex >= 3)
		return;

	auto &ms = level.mapSelector;

	auto *candidate = ms.candidates[voteIndex];
	if (!candidate)
		return;

	const int clientNum = ent->s.number;

	// Ignore if already voted for same candidate
	if (ms.votes[clientNum] == voteIndex)
		return;

	// Remove previous vote if any
	if (int prevVote = ms.votes[clientNum]; prevVote >= 0 && prevVote < 3) {
		ms.voteCounts[prevVote] = std::max(0, ms.voteCounts[prevVote] - 1);
	}

	// Store new vote
	ms.votes[clientNum] = voteIndex;
	ms.voteCounts[voteIndex]++;

	// Feedback
	const char *mapName = candidate->longName.empty()
		? candidate->filename.c_str()
		: candidate->longName.c_str();

	gi.LocBroadcast_Print(PRINT_HIGH, "{} voted for map {}\n",
		ent->client->sess.netName, mapName);

	// Mark menu dirty to update HUD/bar
	ent->client->menu.doUpdate = true;

	// === Early vote finalization check ===

	// Count number of active players
	int totalVoters = 0;
	for (auto ec : active_players()) {
		if (ec && ec->client && !ec->client->sess.is_a_bot)
			++totalVoters;
	}

	// If a map has more than half the votes, finalize early
	for (int i = 0; i < 3; ++i) {
		if (ms.candidates[i] && ms.voteCounts[i] > totalVoters / 2) {
			gi.Broadcast_Print(PRINT_HIGH, "Majority vote detected - finalizing early...\n");
			MapSelectorFinalize();
			level.intermission.postIntermissionTime = level.time;  // allow countdown to continue cleanly
			break;
		}
	}
}

// ==========================

/*
=========================
PrintMapList
=========================
*/
int PrintMapList(gentity_t *ent, bool cycleOnly) {
	if (!ent || !ent->client)
		return 0;

	const int maxMsgLen = 1024;
	const int maxLineLen = 120;
	int longestName = 0;

	// Determine longest map name for formatting
	for (const auto &map : game.mapSystem.mapPool) {
		if (cycleOnly && !map.isCycleable)
			continue;
		if ((int)map.filename.length() > longestName)
			longestName = (int)map.filename.length();
	}

	int colWidth = longestName + 1;
	int cols = maxLineLen / colWidth;
	if (cols == 0) cols = 1;

	std::string message;
	int colCount = 0;
	int printedCount = 0;

	for (const auto &map : game.mapSystem.mapPool) {
		if (cycleOnly && !map.isCycleable)
			continue;

		message += map.filename;
		message.append(colWidth - map.filename.length(), ' ');
		printedCount++;

		if (++colCount >= cols) {
			message += '\n';
			colCount = 0;
		}
	}

	// Ensure proper message segmentation
	size_t pos = 0;
	while (pos < message.length()) {
		std::string part = message.substr(pos, maxMsgLen);
		size_t lastNewline = part.find_last_of('\n');
		if (lastNewline != std::string::npos && (pos + lastNewline) < message.length())
			part = message.substr(pos, lastNewline + 1);

		gi.LocClient_Print(ent, PRINT_HIGH, "{}", part.c_str());
		pos += part.length();
	}

	if (printedCount > 0)
		gi.LocClient_Print(ent, PRINT_HIGH, "\n");

	return printedCount;
}

/*
=========================
ParseMyMapFlags
=========================
*/
bool ParseMyMapFlags(const std::vector<std::string> &args, uint16_t &enableFlags, uint16_t &disableFlags) {
	enableFlags = 0;
	disableFlags = 0;

	for (const std::string &arg : args) {
		if (arg.length() < 2 || (arg[0] != '+' && arg[0] != '-'))
			return false;

		bool enable = (arg[0] == '+');
		const char *flag = arg.c_str() + 1;

		uint16_t bit = 0;
		if (_stricmp(flag, "pu") == 0) bit = MAPFLAG_PU;
		else if (_stricmp(flag, "pa") == 0) bit = MAPFLAG_PA;
		else if (_stricmp(flag, "ar") == 0) bit = MAPFLAG_AR;
		else if (_stricmp(flag, "am") == 0) bit = MAPFLAG_AM;
		else if (_stricmp(flag, "ht") == 0) bit = MAPFLAG_HT;
		else if (_stricmp(flag, "bfg") == 0) bit = MAPFLAG_BFG;
		else if (_stricmp(flag, "pb") == 0) bit = MAPFLAG_PB;
		else if (_stricmp(flag, "fd") == 0) bit = MAPFLAG_FD;
		else if (_stricmp(flag, "sd") == 0) bit = MAPFLAG_SD;
		else if (_stricmp(flag, "ws") == 0) bit = MAPFLAG_WS;
		else return false;

		if (enable) enableFlags |= bit;
		else disableFlags |= bit;
	}

	return true;
}

/*
===============
MapSystem::GetMapEntry
===============
*/
const MapEntry *MapSystem::GetMapEntry(const std::string &mapName) const {
	for (const auto &map : mapPool) {
		if (_stricmp(map.filename.c_str(), mapName.c_str()) == 0)
			return &map;
	}
	return nullptr;
}

/*
===============
MapSystem::IsClientInQueue
===============
*/
bool MapSystem::IsClientInQueue(const std::string &socialID) const {
	for (const auto &q : playQueue) {
		if (_stricmp(q.socialID.c_str(), socialID.c_str()) == 0)
			return true;
	}
	return false;
}

/*
===============
MapSystem::MapExists

Checks whether a map BSP file exists under "baseq2/maps/<mapname>.bsp".
Returns true if the file can be opened.
===============
*/
bool MapSystem::MapExists(std::string_view mapName) const {
	if (mapName.empty())
		return false;

	std::string path = "baseq2/maps/";
	path += mapName;
	path += ".bsp";

	std::ifstream file(path);
	return file.is_open();
}

/*
===============
MapSystem::IsMapInQueue
===============
*/
bool MapSystem::IsMapInQueue(const std::string &mapName) const {
	for (const auto &q : playQueue) {
		if (_stricmp(q.filename.c_str(), mapName.c_str()) == 0)
			return true;
	}
	return false;
}

/*
==================
LoadMapPool
==================
*/
void LoadMapPool(gentity_t* ent) {
	bool entClient = ent && ent->client;
	game.mapSystem.mapPool.clear();

	std::string path = "baseq2/";
	path += g_maps_pool_file->string;

	std::ifstream file(path, std::ifstream::binary);
	if (!file.is_open()) {
		if (entClient)
			gi.LocClient_Print(ent, PRINT_HIGH, "[MapPool] Failed to open file: {}\n", path.c_str());
		return;
	}

	Json::Value root;
	Json::CharReaderBuilder builder;
	std::string errs;

	if (!Json::parseFromStream(builder, file, &root, &errs)) {
		if (entClient)
			gi.LocClient_Print(ent, PRINT_HIGH, "[MapPool] JSON parsing failed: {}\n", errs.c_str());
		return;
	}

	if (!root.isMember("maps") || !root["maps"].isArray()) {
		if (entClient)
			gi.Client_Print(ent, PRINT_HIGH, "[MapPool] JSON must contain a 'maps' array.\n");
		return;
	}

	int loaded = 0, skipped = 0;
	for (const auto& entry : root["maps"]) {
		if (!entry.isMember("bsp") || !entry["bsp"].isString() ||
			!entry.isMember("dm") || !entry["dm"].asBool()) {
			skipped++;
			continue;
		}

		MapEntry map;
		map.filename = entry["bsp"].asString();

		if (entry.isMember("title"))          map.longName = entry["title"].asString();
		if (entry.isMember("min"))            map.minPlayers = entry["min"].asInt();
		if (entry.isMember("max"))            map.maxPlayers = entry["max"].asInt();
		if (entry.isMember("gametype"))       map.suggestedGametype = static_cast<GameType>(entry["gametype"].asInt());
		if (entry.isMember("ruleset"))        map.suggestedRuleset = static_cast<ruleset_t>(entry["ruleset"].asInt());
		if (entry.isMember("scorelimit"))     map.scoreLimit = entry["scorelimit"].asInt();
		if (entry.isMember("timeLimit"))      map.timeLimit = entry["timeLimit"].asInt();
		if (entry.isMember("popular"))        map.isPopular = entry["popular"].asBool();
		if (entry.isMember("custom"))         map.isCustom = entry["custom"].asBool();
		if (entry.isMember("custom_textures")) map.isCustom = entry["custom_textures"].asBool();
		if (entry.isMember("custom_sounds"))   map.isCustom = entry["custom_sounds"].asBool();

		map.mapTypeFlags |= MAP_DM;
		if (entry.get("sp", false).asBool())   map.mapTypeFlags |= MAP_SP;
		if (entry.get("coop", false).asBool()) map.mapTypeFlags |= MAP_COOP;
		if (entry.get("tdm", false).asBool())  map.preferredTDM = true;
		if (entry.get("ctf", false).asBool())  map.preferredCTF = true;
		if (entry.get("duel", false).asBool()) map.preferredDuel = true;

		map.isCycleable = false;
		map.lastPlayed = 0;

		game.mapSystem.mapPool.push_back(std::move(map));
		loaded++;
	}

	if (entClient) {
		gi.LocClient_Print(ent, PRINT_HIGH,
			"[MapPool] Loaded {} map{} from '{}'. Skipped {} non-DM or invalid entr{}.\n",
			loaded, (loaded == 1 ? "" : "s"), path.c_str(),
			skipped, (skipped == 1 ? "y" : "ies"));
	}
}

/*
==================
LoadMapCycle
==================
*/
void LoadMapCycle(gentity_t *ent) {
	bool entClient = ent && ent->client;

	std::string path = "baseq2/";
	path += g_maps_cycle_file->string;

	std::ifstream file(path);
	if (!file.is_open()) {
		if (ent && ent->client)
			gi.LocClient_Print(ent, PRINT_HIGH, "[MapPool] Failed to open file: {}\n", path.c_str());
		return;
	}


	// Reset cycleable flags
	for (auto &m : game.mapSystem.mapPool)
		m.isCycleable = false;

	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	// Remove single-line comments
	content = std::regex_replace(content, std::regex("//.*"), "");

	// Remove multi-line comments (match across lines safely)
	content = std::regex_replace(content, std::regex("/\\*[^*]*\\*+(?:[^/*][^*]*\\*+)*/"), "");

	std::istringstream stream(content);
	std::string token;
	int matched = 0, unmatched = 0;

	while (stream >> token) {
		for (auto &m : game.mapSystem.mapPool) {
			if (_stricmp(token.c_str(), m.filename.c_str()) == 0) {
				m.isCycleable = true;
				matched++;
				goto next_token;
			}
		}
		unmatched++;
	next_token:;
	}

	if (entClient)
		gi.LocClient_Print(ent, PRINT_HIGH, "[MapCycle] Marked {} maps cycleable, ignored {} unknown entries.\n", matched, unmatched);
}

/*
=========================
AutoSelectNextMap
=========================
*/
std::optional<MapEntry> AutoSelectNextMap() {
	const auto &pool = game.mapSystem.mapPool;

	// Screenshot tool override - select next in map list (looping, based on current map)
	if (g_autoScreenshotTool && g_autoScreenshotTool->integer > 0 && !pool.empty()) {
		const std::string current = level.mapName.data();
		auto it = std::find_if(pool.begin(), pool.end(), [&](const MapEntry &m) {
			return !_stricmp(m.filename.c_str(), current.c_str());
			});

		if (it != pool.end()) {
			auto next = std::next(it);
			if (next == pool.end())
				next = pool.begin();
			return *next;
		}

		// If current not found, fallback to first map
		return pool.front();
	}

	const int playerCount = level.pop.num_playing_human_clients;
	const bool avoidCustom = (level.pop.num_console_clients > 0);
	const bool avoidCustomTextures = g_maps_allow_custom_textures && !g_maps_allow_custom_textures->integer;
	const bool avoidCustomSounds = g_maps_allow_custom_sounds && !g_maps_allow_custom_sounds->integer;

	const int cooldown = 1800;
	int secondsSinceStart = static_cast<int>(time(nullptr) - game.serverStartTime);

	auto mapValid = [&](const MapEntry &map) -> bool {
		int lastPlayed = map.lastPlayed / 1000;

		if (lastPlayed > 0) {
			int delta = secondsSinceStart - lastPlayed;
			if (delta < 0 || delta < cooldown) {
				gi.Com_PrintFmt("Map {} skipped: played {} ago (cooldown: {})\n",
					map.filename.c_str(),
					FormatDuration(delta).c_str(),
					FormatDuration(cooldown - delta).c_str()
				);
				return false;
			}
		}

		if ((map.minPlayers > 0 && playerCount < map.minPlayers) ||
			(map.maxPlayers > 0 && playerCount > map.maxPlayers))
			return false;

		if (avoidCustom && map.isCustom)
			return false;

		if (avoidCustomTextures && map.hasCustomTextures)
			return false;

		if (avoidCustomSounds && map.hasCustomSounds)
			return false;

		return true;
		};

	std::vector<const MapEntry *> eligible;

	for (const auto &map : pool) {
		if (!map.isCycleable)
			continue;
		if (mapValid(map))
			eligible.push_back(&map);
	}

	if (eligible.empty()) {
		for (const auto &map : pool) {
			if (mapValid(map))
				eligible.push_back(&map);
		}
	}

	if (eligible.empty()) {
		for (const auto &map : pool) {
			if (avoidCustom && map.isCustom)
				continue;
			if (avoidCustomTextures && map.hasCustomTextures)
				continue;
			if (avoidCustomSounds && map.hasCustomSounds)
				continue;
			eligible.push_back(&map);
		}
	}

	if (eligible.empty())
		return std::nullopt;

	std::vector<const MapEntry *> weighted;
	for (const auto *map : eligible) {
		weighted.push_back(map);
		if (map->isPopular)
			weighted.push_back(map); // 2x chance
	}

	std::shuffle(eligible.begin(), eligible.end(), game.mapRNG);
	const MapEntry *chosen = eligible[0];

	return *chosen;
}

/*
=========================
MapSelectorVoteCandidates
=========================
*/
std::vector<const MapEntry *> MapSelectorVoteCandidates(int maxCandidates) {
	std::vector<const MapEntry *> pool;
	const int playerCount = level.pop.num_playing_human_clients;
	const bool avoidCustom = (level.pop.num_console_clients > 0);
	const bool avoidCustomTextures = !g_maps_allow_custom_textures->integer;
	int64_t now = GetCurrentRealTimeMillis();
	bool isCTF = Game::Has(GameFlags::CTF);
	bool isDuel = Game::Has(GameFlags::OneVOne);
	bool isTDM = Teams();

	for (const auto &map : game.mapSystem.mapPool) {
		if (!map.isCycleable)
			continue;
		if (map.lastPlayed && (now - map.lastPlayed) < 1800000)
			continue;
		if ((map.minPlayers > 0 && playerCount < map.minPlayers) ||
			(map.maxPlayers > 0 && playerCount > map.maxPlayers))
			continue;
		if (avoidCustomTextures && map.hasCustomTextures)
			continue;
		if (!Q_strcasecmp(level.mapName.data(), map.filename.c_str()))
			continue;

		bool preferred = true;

		if (isCTF && !map.preferredCTF)
			preferred = false;
		else if (isDuel && !map.preferredDuel)
			preferred = false;
		else if (isTDM && !map.preferredTDM)
			preferred = false;

		if (!preferred)
			continue;

		pool.push_back(&map);
	}

	if (pool.size() < 2) {
		pool.clear();
		for (const auto &map : game.mapSystem.mapPool) {
			if (map.lastPlayed && (now - map.lastPlayed) < 1800000)
				continue;
			if (avoidCustomTextures && map.hasCustomTextures)
				continue;
			pool.push_back(&map);
		}
	}

	std::shuffle(pool.begin(), pool.end(), std::default_random_engine(level.time.milliseconds()));
	if (pool.size() > maxCandidates)
		pool.resize(maxCandidates);
	return pool;
}

// ====================================================================================

// -----------------------------
// Filtering System for mappool/mapcycle
// -----------------------------
#include <functional>
#include <cctype>

using MapFilter = std::function<bool(const MapEntry &)>;

/*
==============================
Case-insensitive substring check
==============================
*/
static bool str_contains_case(const std::string &haystack, const std::string &needle) {
	return std::search(
		haystack.begin(), haystack.end(),
		needle.begin(), needle.end(),
		[](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); }
	) != haystack.end();
}

/*
==============================
Quoted string and token parser
==============================
*/
static std::vector<std::string> TokenizeQuery(const std::string &input) {
	std::vector<std::string> tokens;
	bool inQuote = false;
	std::string current;
	for (char ch : input) {
		if (ch == '"') {
			inQuote = !inQuote;
			if (!inQuote && !current.empty()) {
				tokens.push_back(current);
				current.clear();
			}
		} else if (std::isspace(ch) && !inQuote) {
			if (!current.empty()) {
				tokens.push_back(current);
				current.clear();
			}
		} else {
			current += ch;
		}
	}
	if (!current.empty())
		tokens.push_back(current);
	return tokens;
}

/*
==============================
ParseMapFilters
==============================
*/
static std::vector<MapFilter> ParseMapFilters(const std::string &input) {
	auto tokens = TokenizeQuery(input);
	std::vector<std::vector<MapFilter>> orGroups;
	std::vector<MapFilter> currentGroup;

	for (const std::string &token : tokens) {
		if (token == "or" || token == "OR") {
			if (!currentGroup.empty()) {
				orGroups.push_back(std::move(currentGroup));
				currentGroup.clear();
			}
			continue;
		}

		bool isNegated = token[0] == '!';
		std::string raw = isNegated ? token.substr(1) : token;
		MapFilter filter;

		if (raw == "dm") {
			filter = [](const MapEntry &m) { return m.mapTypeFlags & MAP_DM; };
		} else if (raw == "ctf") {
			filter = [](const MapEntry &m) { return m.suggestedGametype == GameType::CaptureTheFlag; };
		} else if (raw == "sp") {
			filter = [](const MapEntry &m) { return m.mapTypeFlags & MAP_SP; };
		} else if (raw == "coop") {
			filter = [](const MapEntry &m) { return m.mapTypeFlags & MAP_COOP; };
		} else if (raw == "custom") {
			filter = [](const MapEntry &m) { return m.isCustom; };
		} else if (raw == "custom_textures") {
			filter = [](const MapEntry &m) { return m.hasCustomTextures; };
		} else if (raw == "custom_sounds") {
			filter = [](const MapEntry &m) { return m.hasCustomSounds; };
		} else if (!raw.empty() && raw[0] == '>') {
			int n = atoi(raw.c_str() + 1);
			filter = [n](const MapEntry &m) { return m.minPlayers > n; };
		} else if (!raw.empty() && raw[0] == '<') {
			int n = atoi(raw.c_str() + 1);
			filter = [n](const MapEntry &m) { return m.maxPlayers < n; };
		} else {
			filter = [raw](const MapEntry &m) {
				return str_contains_case(m.filename, raw) || str_contains_case(m.longName, raw);
				};
		}

		if (isNegated) {
			MapFilter base = filter;
			filter = [base](const MapEntry &m) { return !base(m); };
		}

		currentGroup.push_back(std::move(filter));
	}

	if (!currentGroup.empty())
		orGroups.push_back(std::move(currentGroup));

	// Return a single combined filter that ORs groups and ANDs within groups
	return {
		[orGroups](const MapEntry &m) -> bool {
			for (const auto &group : orGroups) {
				bool match = true;
				for (const auto &f : group) {
					if (!f(m)) {
						match = false;
						break;
					}
				}
				if (match) return true;
			}
			return false;
		}
	};
}

/*
==============================
MapMatchesFilters
==============================
*/
static bool MapMatchesFilters(const MapEntry &map, const std::vector<MapFilter> &filters) {
	for (const auto &f : filters) {
		if (!f(map)) return false;
	}
	return true;
}

/*
==============================
PrintMapListFiltered (caller of filters)
==============================
*/
int PrintMapListFiltered(gentity_t *ent, bool cycleOnly, const std::string &filterQuery) {
	if (!ent || !ent->client)
		return 0;

	auto filters = ParseMapFilters(filterQuery);

	const int maxMsgLen = 1024;
	const int maxLineLen = 120;
	int longestName = 0;

	for (const auto &map : game.mapSystem.mapPool) {
		if (cycleOnly && !map.isCycleable)
			continue;
		if (!filterQuery.empty() && !MapMatchesFilters(map, filters))
			continue;
		if ((int)map.filename.length() > longestName)
			longestName = (int)map.filename.length();
	}

	int colWidth = longestName + 1;
	int cols = maxLineLen / colWidth;
	if (cols == 0) cols = 1;

	std::string message;
	int colCount = 0;
	int printedCount = 0;

	for (const auto &map : game.mapSystem.mapPool) {
		if (cycleOnly && !map.isCycleable)
			continue;
		if (!filterQuery.empty() && !MapMatchesFilters(map, filters))
			continue;

		message += map.filename;
		message.append(colWidth - map.filename.length(), ' ');
		printedCount++;

		if (++colCount >= cols) {
			message += '\n';
			colCount = 0;
		}
	}

	size_t pos = 0;
	while (pos < message.length()) {
		std::string part = message.substr(pos, maxMsgLen);
		size_t lastNewline = part.find_last_of('\n');
		if (lastNewline != std::string::npos && (pos + lastNewline) < message.length())
			part = message.substr(pos, lastNewline + 1);

		gi.LocClient_Print(ent, PRINT_HIGH, "{}", part.c_str());
		pos += part.length();
	}

	if (!filterQuery.empty()) {
		gi.LocClient_Print(ent, PRINT_HIGH, "\n{} map{} matched filter: {}\n",
			printedCount, printedCount == 1 ? "" : "s", filterQuery.c_str());
	}

	return printedCount;
}
