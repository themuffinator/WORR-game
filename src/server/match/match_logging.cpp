// g_match_logging.cpp (Game Match Logging)
// This file implements a detailed match statistics logging system. At the end
// of each match, it gathers comprehensive data about players and teams (kills,
// deaths, damage, accuracy, weapon usage, awards, etc.) and writes it out to
// structured files for later analysis and display.
//
// Key Responsibilities:
// - Data Aggregation: The `MatchStats_End` function iterates through all players
//   at the end of a match and compiles their performance data into `PlayerStats`
//   and `TeamStats` structures.
// - JSON Output: Serializes the collected match data into a well-structured
//   JSON file. This format is ideal for data parsing by external tools or websites.
// - HTML Report Generation: Creates a user-friendly HTML report of the match
//   results, including overall scores, team comparisons, top player lists, and
//   detailed individual performance breakdowns with progress bars and weapon stats.
// - Match Initialization: `MatchStats_Init` is called at the start of a match
//   to generate a unique match ID and reset all statistical counters.

#include "../g_local.hpp"
#include "../gameplay/client_config.hpp"
#include "../../shared/char_array_utils.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <json/json.h>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <cerrno>

using json = Json::Value;

/*
=============
HtmlEscape

Escapes special characters for safe HTML output.
=============
*/
static std::string HtmlEscape(std::string_view input) {
	std::string output;
	output.reserve(input.size());
	for (char c : input) {
		switch (c) {
		case '&':
			output.append("&amp;");
			break;
		case '<':
			output.append("&lt;");
			break;
		case '>':
			output.append("&gt;");
			break;
		case '\"':
			output.append("&quot;");
			break;
		case '\'':
			output.append("&apos;");
			break;
		default:
			output.push_back(c);
			break;
		}
	}
	return output;
}

const std::string MATCH_STATS_PATH = GAMEVERSION + "/matches";

// Precomputed map for fast abbreviation-to-index lookup
const std::unordered_map<std::string, Weapon> weaponAbbreviationMap = []() {
	std::unordered_map<std::string, Weapon> map;
	for (int i = 0; i < static_cast<uint8_t>(Weapon::Total); i++) {
		map[weaponAbbreviations[i]] = static_cast<Weapon>(i);
	}
	return map;
	}();

// Function to get the weapon index from an abbreviation
static Weapon getWeaponIndex(const std::string& abbreviation) {
	auto it = weaponAbbreviationMap.find(abbreviation);
	if (it != weaponAbbreviationMap.end()) {
		return it->second; // Found abbreviation, return the Weapon
	}
	return Weapon::None; // Default for unknown abbreviations
}

static inline double GetAveragePickupDelay(uint32_t pickupCount, double totalDelay) {
	if (pickupCount == 0)
		return 0.0;
	return totalDelay / pickupCount;
}

static inline ModID getModIdByName(const std::string& modName) {
	for (const auto& m : modr) {
		if (m.name == modName)
			return m.mod;
	}
	return ModID::Unknown; // fallback
}

const std::array<std::string, 2> booleanStrings = { "false", "true" };
const std::array<std::string, 2> winLossStrings = { "loss", "win" };

struct PlayerStats {
	std::string socialID;
	std::string playerName;
	int totalKills = 0;
	int totalSpawnKills = 0;
	int totalTeamKills = 0;
	int totalDeaths = 0;
	int totalSuicides = 0;
	double totalKDR = 0.0;
        int totalScore = 0;
        int proBallGoals = 0;
        int proBallAssists = 0;
        int totalShots = 0;
        int totalHits = 0;
	double totalAccuracy = 0.0;
	int totalDmgDealt = 0;
	int totalDmgReceived = 0;
	int ratingChange = 0;

	double killsPerMinute = 0;
	int64_t playTimeMsec = 0;
	int skillRating = 0;
	int skillRatingChange = 0;

	std::array <uint32_t, static_cast<size_t>(HighValueItems::Total)> pickupCounts = {};
	std::array <double, static_cast<size_t>(HighValueItems::Total)> pickupDelays = {};

	// Weapon-based stats
	std::map<std::string, int> totalShotsPerWeapon;
	std::map<std::string, int> totalHitsPerWeapon;
	std::map<std::string, double> accuracyPerWeapon;

	// MOD-based stats
        std::map<ModID, int> modTotalKills;
        std::map<ModID, int> modTotalDeaths;
        std::map<ModID, double> modTotalKDR;
	std::map<ModID, int> modTotalDmgD;
	std::map<ModID, int> modTotalDmgR;

	std::array<uint32_t, static_cast<size_t>(PlayerMedal::Total)> awards = {};

	PlayerStats() {
		for (const auto& weapon : weaponAbbreviations) {
			totalShotsPerWeapon[weapon] = 0;
			totalHitsPerWeapon[weapon] = 0;
			accuracyPerWeapon[weapon] = 0.0;
		}
		for (const auto& mod : modr) {
                        modTotalKills[mod.mod] = 0;
                        modTotalDeaths[mod.mod] = 0;
                        modTotalKDR[mod.mod] = 0.0;
                        modTotalDmgD[mod.mod] = 0;
                        modTotalDmgR[mod.mod] = 0;
		}
	}

	// Method to calculate weapon accuracy
	void calculateWeaponAccuracy() {
		for (const auto& weapon : weaponAbbreviations) {
			if (totalShotsPerWeapon[weapon] > 0) {
				accuracyPerWeapon[weapon] =
					(static_cast<double>(totalHitsPerWeapon[weapon]) / totalShotsPerWeapon[weapon]) * 100.0;
			}
			else {
				accuracyPerWeapon[weapon] = 0.0;
			}
		}
	}

	// Function to calculate the Kill-Death Ratio (KDR)
	void calculateKDR() {
		if (totalDeaths > 0) {
			totalKDR = static_cast<double>(totalKills) / totalDeaths;
		}
		else if (totalKills > 0) {
			totalKDR = static_cast<double>(totalKills);  // Infinite KDR represented as kills
		}
		else {
			totalKDR = 0.0;  // No kills, no deaths
		}
	}

	json toJson() const {
		json result;
		result["socialID"] = socialID;
                result["playerName"] = playerName;
                result["totalScore"] = totalScore;
                if (proBallGoals > 0)    result["proBallGoals"] = proBallGoals;
                if (proBallAssists > 0) result["proBallAssists"] = proBallAssists;

                if (totalKills > 0)        result["totalKills"] = totalKills;
		if (totalSpawnKills > 0)   result["totalSpawnKills"] = totalSpawnKills;
		if (totalTeamKills > 0)    result["totalTeamKills"] = totalTeamKills; // fixed
		if (totalDeaths > 0)       result["totalDeaths"] = totalDeaths;
		if (totalSuicides > 0)     result["totalSuicides"] = totalSuicides;
		if (totalKDR > 0.0)        result["totalKDR"] = totalKDR;
		if (totalHits > 0)         result["totalHits"] = totalHits;
		if (totalShots > 0)        result["totalShots"] = totalShots;
		if (totalAccuracy > 0.0)   result["totalAccuracy"] = totalAccuracy;
		if (totalDmgDealt > 0)     result["totalDmgDealt"] = totalDmgDealt;
		if (totalDmgReceived > 0)  result["totalDmgReceived"] = totalDmgReceived;
		if (ratingChange != 0)     result["ratingChange"] = ratingChange;
		if (playTimeMsec > 0)      result["playTime"] = playTimeMsec;
		if (killsPerMinute > 0)    result["killsPerMinute"] = killsPerMinute;
		if (skillRating > 0)       result["skillRating"] = skillRating;
		if (skillRatingChange != 0) result["skillRatingChange"] = skillRatingChange;

		json shotsJson, hitsJson, accuracyJson;
		for (const auto& weaponName : weaponAbbreviations) {
			auto shotsIt = totalShotsPerWeapon.find(weaponName);
			if (shotsIt != totalShotsPerWeapon.end() && shotsIt->second > 0)
				shotsJson[weaponName] = shotsIt->second;

			auto hitsIt = totalHitsPerWeapon.find(weaponName);
			if (hitsIt != totalHitsPerWeapon.end() && hitsIt->second > 0)
				hitsJson[weaponName] = hitsIt->second;

			auto accIt = accuracyPerWeapon.find(weaponName);
			if (accIt != accuracyPerWeapon.end() && accIt->second > 0.0)
				accuracyJson[weaponName] = accIt->second;
		}
		if (!shotsJson.empty())    result["totalShotsPerWeapon"] = shotsJson;
		if (!hitsJson.empty())     result["totalHitsPerWeapon"] = hitsJson;
		if (!accuracyJson.empty()) result["accuracyPerWeapon"] = accuracyJson;

		json modKillsJson, modDeathsJson, modKDRJson, modDmgDJson, modDmgRJson;
		for (const auto& mod : modr) {
                        if (modTotalKills.at(mod.mod) > 0) modKillsJson[mod.name] = modTotalKills.at(mod.mod);
                        if (modTotalDeaths.at(mod.mod) > 0) modDeathsJson[mod.name] = modTotalDeaths.at(mod.mod);
                        if (modTotalKDR.at(mod.mod) > 0.0) modKDRJson[mod.name] = modTotalKDR.at(mod.mod);
                        if (modTotalDmgD.at(mod.mod) > 0)  modDmgDJson[mod.name] = modTotalDmgD.at(mod.mod);
                        if (modTotalDmgR.at(mod.mod) > 0)  modDmgRJson[mod.name] = modTotalDmgR.at(mod.mod);
		}
		if (!modKillsJson.empty()) result["totalKillsByMOD"] = modKillsJson;
		if (!modDeathsJson.empty()) result["totalDeathsByMOD"] = modDeathsJson;
		if (!modKDRJson.empty())    result["totalKDRByMOD"] = modKDRJson;
		if (!modDmgDJson.empty())   result["totalDmgDByMOD"] = modDmgDJson;
		if (!modDmgRJson.empty())   result["totalDmgRByMOD"] = modDmgRJson;

		if (!level.match.deathLog.empty()) {
			json dlog = json(Json::arrayValue);
			for (auto& e : level.match.deathLog) {
				json entry;
				entry["time"] = e.time.seconds();
				entry["victim"]["name"] = e.victim.name;
				entry["victim"]["id"] = e.victim.id;
				entry["attacker"]["name"] = e.attacker.name;
				entry["attacker"]["id"] = e.attacker.id;
				entry["mod"] = modr[static_cast<int>(e.mod.id)].name;
				dlog.append(entry);
			}
			result["deathLog"] = std::move(dlog);
		}

		json pickupsJson;
		json pickupDelayJson;
		for (int i = static_cast<size_t>(HighValueItems::None) + 1; i < static_cast<size_t>(HighValueItems::Total); ++i) {
			auto item = i;
			if (pickupCounts[item] > 0)
				pickupsJson[HighValueItemNames[item]] = pickupCounts[item];
			if (pickupDelays[item] > 0.0)
				pickupDelayJson[HighValueItemNames[item]] = pickupDelays[item];
		}
		if (!pickupsJson.empty())
			result["pickupCounts"] = std::move(pickupsJson);
		if (!pickupDelayJson.empty())
			result["pickupDelays"] = std::move(pickupDelayJson);

		return result;
	}
};

struct TeamStats {
	std::string teamName;          // Team name or identifier
	int score = 0;                 // Team score
	std::string outcome;           // "win", "loss", or "draw"
	std::vector<PlayerStats> players; // Players on the team

	// Generate JSON object for this team's stats
	json toJson() const {
		json teamJson;
		teamJson["teamName"] = teamName;
		teamJson["score"] = score;
		teamJson["outcome"] = outcome;
		teamJson["players"] = json(Json::arrayValue);
		for (const auto& player : players) {
			teamJson["players"].append(player.toJson());
		}
		return teamJson;
	}
};

struct MatchStats {
	std::string matchID;           // Unique match identifier
        std::string serverName;          // Server name
        std::string serverHostName;    // Name of the server host
        std::string gameType;          // Game type (e.g., "FFA", "TDM")
        std::string ruleSet;
        std::string mapName;           // Name of the map
        bool ranked = false;
	int totalKills = 0;            // Total kills in the match
        int totalSpawnKills = 0;        // Total spawn kills in the match
        int totalTeamKills = 0;        // Total team kills in the match
        int totalDeaths = 0;            // Total deaths in the match
        int totalSuicides = 0;            // Total suicides in the match
        int proBall_totalGoals = 0;
        int proBall_totalAssists = 0;
        double avKillsPerMinute = 0;
	int ctf_totalFlagsCaptured = 0;    // Flags captured (for CTF)
	int ctf_totalFlagAssists = 0;    // Flag assists (for CTF)
	int ctf_totalFlagDefends = 0;    // Flag defends (for CTF)
	std::map<std::string, int>    totalKillsByMOD;
	std::map<std::string, int>    totalDeathsByMOD;
	std::map<std::string, double> totalKDRByMOD;
	int64_t durationMS = 0;         // Match duration in msec
	std::vector<PlayerStats> players; // Individual player stats (for FFA/Duel)
	std::vector<TeamStats> teams;  // Team stats (for TDM/CTF)
#if 0
	// Convert time_t to ISO 8601 string
	std::string formatTime(std::time_t time) const {
		std::ostringstream oss;
		oss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");
		return oss.str();
	}
#endif
	std::string formatTime(int64_t msec) const {
		time_t t = static_cast<time_t>(msec / 1000);
		if (!t || t <= 0) return "n/a";

		std::tm* tm = std::gmtime(&t);
		if (!tm) return "invalid";

		char buf[64];
		if (std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm)) {
			return buf;
		}
		return "error";
	}


	// Calculate duration based on start and end times
	void calculateDuration() {
		// level.matchStartRealTime and matchEndRealTime are in milliseconds
		durationMS = level.matchEndRealTime - level.matchStartRealTime;
	}

	// Generate JSON object for the match stats
	json toJson() const {
                json matchJson;
                matchJson["matchID"] = matchID;
                matchJson["serverName"] = serverName;
                if (!serverHostName.empty()) {
                        matchJson["serverHostName"] = serverHostName;
                }
                matchJson["gameType"] = gameType;
                matchJson["ruleSet"] = ruleSet;
                matchJson["mapName"] = mapName;
                matchJson["matchRanked"] = ranked;
                matchJson["totalKills"] = totalKills;
                matchJson["totalSpawnKills"] = totalSpawnKills;
                matchJson["totalTeamKills"] = totalTeamKills;
                matchJson["totalDeaths"] = totalDeaths;
                matchJson["totalSuicides"] = totalSuicides;
                if (proBall_totalGoals > 0)
                        matchJson["totalGoals"] = proBall_totalGoals;
                if (proBall_totalAssists > 0)
                        matchJson["totalGoalAssists"] = proBall_totalAssists;
                matchJson["avKillsPerMinute"] = avKillsPerMinute;
		matchJson["totalFlagsCaptured"] = ctf_totalFlagsCaptured;
		matchJson["totalFlagAssists"] = ctf_totalFlagAssists;
		matchJson["totalFlagDefends"] = ctf_totalFlagDefends;
		matchJson["matchTimeStart"] = level.matchStartRealTime;
		matchJson["matchTimeEnd"] = level.matchEndRealTime;
		matchJson["matchTimeDuration"] = durationMS;
		matchJson["players"] = json(Json::arrayValue);
		matchJson["teams"] = json(Json::arrayValue);

		// Add player stats for FFA or Duel
		for (const auto& player : players) {
			matchJson["players"].append(player.toJson());
		}

		if (Teams()) {
			// Add team stats for team-based modes
			for (const auto& team : teams) {
				matchJson["teams"].append(team.toJson());
			}
		}

		return matchJson;
	}
};
MatchStats matchStats;

static bool MatchStats_WriteJson(const MatchStats& matchStats, const std::string& fileName) {
	try {
		std::ofstream file(fileName);
		if (file.is_open()) {
			Json::StreamWriterBuilder writer;
			writer["indentation"] = "    "; // match original .dump(4)
			std::string output = Json::writeString(writer, matchStats.toJson());
			file << output;
			file.close();
			gi.Com_PrintFmt("Match JSON written to {}\n", fileName.c_str());
			return true;
		}

		const std::error_code ec(errno, std::system_category());
		gi.Com_PrintFmt("Failed to open JSON file: {} ({})\n", fileName.c_str(), ec.message().c_str());
	}
	catch (const std::exception& e) {
		gi.Com_PrintFmt("Exception while writing JSON ({}): {}\n", fileName.c_str(), e.what());
	}

	return false;
}

/*
=============
Html_WriteHeader
=============
*/
static inline void Html_WriteHeader(std::ofstream& html, const MatchStats& matchStats) {
	const std::string escapedMatchId = HtmlEscape(matchStats.matchID);
	html << R"(<!DOCTYPE html>
<html lang="en"><head><meta charset="UTF-8">
<title>Match Summary - )" << escapedMatchId << R"(</title>
<style>
  body { font-family:Arial,sans-serif; background:#f4f4f4; margin:0; padding:20px; }
  .top-info {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
    gap: 10px;
    background:#fff;
    padding:20px;
    border-radius:8px;
    box-shadow:0 2px 4px rgba(0,0,0,0.1);
    margin-bottom:20px;
  }
  .top-info h1 {
    grid-column:1 / -1;
    font-size:1.8em;
    margin:0 0 10px;
  }
  .top-info p {
    margin:0;
    font-size:0.9em;
    color:#555;
  }
.progress-cell {
  position: relative;
  background: #eee;
//  border-radius: 4px;
  overflow: hidden;
}
.progress-cell .bar {
  position: absolute;
  top: 0; left: 0; bottom: 0;
  background: rgba(0,100,0,0.2);
}
.progress-cell.red .bar { background: rgba(200,0,0,0.3); }
.progress-cell.blue .bar { background: rgba(0,0,200,0.3); }
.progress-cell.green .bar { background: rgba(0,100,0,0.3); }
.player-cell {
  border-left: 6px solid transparent;
  padding-left: 6px;
}
.player-cell.red { border-color: #c00; }
.player-cell.blue { border-color: #00c; }
.player-cell.green { border-color: #060; }

.section.team-red {
  border: 2px solid #c00;
}
.section.team-blue {
  border: 2px solid #00c;
}
.team-score-header {
  font-size: 1.8em;
  font-weight: bold;
  text-align: center;
  margin: 20px 0;
}
.team-score-header span {
  padding: 4px 10px;
  border-radius: 8px;
  color: #fff;
}
.team-score-header .red {
  background: #c00;
}
.team-score-header .blue {
  background: #00c;
}
.player-name.red { color: #c00; font-weight: bold; }
.player-name.blue { color: #00c; font-weight: bold; }
.player-name.green { color: #060; font-weight: bold; }
.player-cell.red { border-left: 6px solid #c00; padding-left: 6px; }
.player-cell.blue { border-left: 6px solid #00c; padding-left: 6px; }
.player-cell.green { border-left: 6px solid #060; padding-left: 6px; }
.player-section.red { border-left: 6px solid #c00; padding-left: 8px; margin-bottom: 16px; }
.player-section.blue { border-left: 6px solid #00c; padding-left: 8px; margin-bottom: 16px; }
.player-section.green { border-left: 6px solid #060; padding-left: 8px; margin-bottom: 16px; }

.winner.red {
  color: #c00;
}
.winner.blue {
  color: #00c;
}
.progress-cell span {
  position: relative;
  padding: 0 4px;
  z-index: 1;
}
.flex-container {
	display: flex;
	flex-wrap: wrap;
	gap: 24px;
	margin-top: 12px;
}

.flex-item {
	flex: 1;
	min-width: 320px;
}
  .section { background:#fff; padding:15px; margin-bottom:20px; border-radius:5px; box-shadow:0 1px 3px rgba(0,0,0,0.1); }
  .overall { border:2px solid #006400; }
  table { width:100%; border-collapse:collapse; margin-top:10px; }
  th,td { border:1px solid #ccc; padding:8px; text-align:left; }
  th { background:#eee; }
  .winner { font-size:1.5em; font-weight:bold; color:#006400; text-align:center; margin-bottom:10px; }
  .footer { font-size:0.8em; color:#666; text-align:right; }
</style>
</head><body>
)";
}

/*
=============
Html_WriteTopInfo
=============
*/
static inline void Html_WriteTopInfo(std::ofstream& html, const MatchStats& matchStats) {
	const bool proBall = Q_strcasecmp(matchStats.gameType.c_str(), "PROBALL") == 0;
	const std::string escapedMatchId = HtmlEscape(matchStats.matchID);
	const std::string escapedServerName = HtmlEscape(matchStats.serverName);
	const std::string escapedGameType = HtmlEscape(matchStats.gameType);
	const std::string escapedMapName = HtmlEscape(matchStats.mapName);
	html << "<div class=\"top-info\">\n"
		<< "  <h1>Match Summary - " << escapedMatchId << "</h1>\n"
		<< "  <p><strong>Server:</strong> " << escapedServerName << "</p>\n"
		<< "  <p><strong>Type:</strong> " << escapedGameType << "</p>\n"
		<< "  <p><strong>Start:</strong> " << matchStats.formatTime(level.matchStartRealTime) << " UTC</p>\n"
		<< "  <p><strong>End:</strong>   " << matchStats.formatTime(level.matchEndRealTime) << " UTC</p>\n"
		<< "  <p><strong>Map:</strong>  " << escapedMapName << "</p>\n"
		<< "  <p><strong>Score Limit:</strong> " << GT_ScoreLimit() << "</p>\n";
	// Time Limit
	{
		int t_secs = timeLimit->integer * 60;
		int h = t_secs / 3600;
		int m = (t_secs % 3600) / 60;
		int s = t_secs % 60;
		html << "  <p><strong>Time Limit:</strong> ";
		if (h > 0)      html << h << "h " << m << "m " << s << "s";
		else if (m > 0) html << m << "m " << s << "s";
		else            html << s << "s";
		html << "</p>\n";
	}
	// Duration
	html << "  <p><strong>Duration:</strong> ";
	{
		int secs = matchStats.durationMS / 1000;
		int h = secs / 3600;
		int m = (secs % 3600) / 60;
		int s = secs % 60;
		if (h > 0)      html << h << "h " << m << "m " << s << "s";
		else if (m > 0) html << m << "m " << s << "s";
		else            html << s << "s";
	}
		html << "</p>\n";
	if (proBall) {
		html << "  <p><strong>Total Goals:</strong> " << matchStats.proBall_totalGoals << "</p>\n"
			<< "  <p><strong>Total Assists:</strong> " << matchStats.proBall_totalAssists << "</p>\n";
	}
	html << "</div>\n";
}


/*
=============
Html_WriteWinnerSummary
=============
*/
static inline void Html_WriteWinnerSummary(std::ofstream& html, const MatchStats& matchStats) {
	std::string winner;
	std::string winnerClass;

	if (!matchStats.teams.empty()) {
		auto& t0 = matchStats.teams[0];
		auto& t1 = matchStats.teams[1];
		if (t0.score > t1.score) {
			winner = t0.teamName;
			winnerClass = (winner == "Red") ? "red" : "blue";
		}
		else {
			winner = t1.teamName;
			winnerClass = (winner == "Red") ? "red" : "blue";
		}
	}
	else if (!matchStats.players.empty()) {
		const auto* best = &matchStats.players.front();
		for (auto& p : matchStats.players)
			if (p.totalScore > best->totalScore) best = &p;
		winner = best->playerName;
	}

	const std::string escapedWinner = HtmlEscape(winner);
	html << "<div class=\"winner";
	if (!winnerClass.empty()) {
		html << ' ' << winnerClass;
	}
	html << "\">Winner: " << escapedWinner << "</div>\\n";
}


/*
=============
Html_WriteOverallScores
=============
*/
static inline void Html_WriteOverallScores(std::ofstream& html, const MatchStats& matchStats, std::vector<const PlayerStats*> allPlayers) {
        const bool proBall = Q_strcasecmp(matchStats.gameType.c_str(), "PROBALL") == 0;
        html << "<div class=\"section overall\">\n"
                << "  <h2>Overall Scores</h2>\n"
                << "  <table>\n"
                << "    <tr>"
                "<th title=\"Player's in-game name (click to jump)\">Player</th>"
                "<th title=\"Percentage of match time played\">%TIME</th>"
                "<th title=\"Skill Rating (and change from match)\">SR</th>"
                "<th title=\"Kill-Death Ratio (Kills / Deaths)\">KDR</th>"
                "<th title=\"Kills Per Minute (Kills / Minutes Played)\">KPM</th>"
                "<th title=\"Damage Ratio (Damage Dealt / Damage Received)\">DMR</th>"
                "<th>Score</th>";
        if (proBall) {
                html << "<th title=\"Goals scored\">GO</th>"
                        << "<th title=\"Goal assists credited\">AS</th>";
        }
        html << "</tr>\n";

        int maxSR = 0, maxScore = 0;
        double maxKDR = 0.0, maxKPM = 0.0, maxDMR = 0.0;
        int maxGoals = 0, maxAssists = 0;

        for (auto* p : allPlayers) {
                maxSR = std::max(maxSR, p->skillRating);
                maxScore = std::max(maxScore, p->totalScore);

                const double kdr = p->totalDeaths ? double(p->totalKills) / p->totalDeaths : double(p->totalKills);
                const double kpm = (p->playTimeMsec > 0) ? (p->totalKills * 60.0) / (p->playTimeMsec / 1000.0) : 0.0;
                const double dmr = p->totalDmgReceived ? double(p->totalDmgDealt) / p->totalDmgReceived : double(p->totalDmgDealt);

                maxKDR = std::max(maxKDR, kdr);
                maxKPM = std::max(maxKPM, kpm);
                maxDMR = std::max(maxDMR, dmr);
                if (proBall) {
                        maxGoals = std::max(maxGoals, p->proBallGoals);
                        maxAssists = std::max(maxAssists, p->proBallAssists);
                }
        }

	const double durationMin = matchStats.durationMS / 60000.0;

	for (auto* p : allPlayers) {
		const double kdr = p->totalDeaths ? double(p->totalKills) / p->totalDeaths
			: (p->totalKills ? double(p->totalKills) : 0.0);
		const double kpm = (durationMin > 0.0) ? p->totalKills / durationMin : 0.0;
		const double dmr = p->totalDmgReceived ? double(p->totalDmgDealt) / p->totalDmgReceived
			: (p->totalDmgDealt ? double(p->totalDmgDealt) : 0.0);
		const int tp = (p->playTimeMsec > 0) ? p->playTimeMsec : matchStats.durationMS;

			const std::string escapedSocialId = HtmlEscape(p->socialID);
			const std::string escapedPlayerName = HtmlEscape(p->playerName);
			html << "    <tr><td title=\"" << escapedSocialId << "\">"
				<< "<a href=\"#player-" << escapedSocialId << "\">" << escapedPlayerName << "</a></td>";

		const double pctTime = (tp > 0) ? (tp / matchStats.durationMS) * 100.0 : 0.0;
		html << "<td class=\"progress-cell\" title=\"% of match time\">"
			<< "<div class=\"bar\" style=\"width:" << pctTime << "%\"></div>"
			<< "<span>" << std::fixed << std::setprecision(1) << pctTime << "%</span></td>";

		// Skill Rating
		const double pctSR = (maxSR > 0) ? (double(p->skillRating) / maxSR) * 100.0 : 0.0;
		html << "<td class=\"progress-cell\" title=\"Skill Rating change from match: "
			<< (p->skillRatingChange >= 0 ? "+" : "") << p->skillRatingChange << "\">"
			<< "<div class=\"bar\" style=\"width:" << pctSR << "%\"></div><span>"
			<< p->skillRating;
		if (p->skillRatingChange != 0) {
			html << " (" << (p->skillRatingChange >= 0 ? "+" : "") << p->skillRatingChange << ")";
		}
		html << "</span></td>";

		const double pctKDR = (maxKDR > 0) ? (kdr / maxKDR) * 100.0 : 0.0;
		html << "<td class=\"progress-cell\" title=\"Kills: " << p->totalKills << ", Deaths: " << p->totalDeaths << "\">"
			<< "<div class=\"bar\" style=\"width:" << pctKDR << "%\"></div>"
			<< "<span>" << std::fixed << std::setprecision(2) << kdr << "</span></td>";

		const double pctKPM = (maxKPM > 0) ? (kpm / maxKPM) * 100.0 : 0.0;
		html << "<td class=\"progress-cell\" title=\"Kills: " << p->totalKills << ", Min: " << int(tp / 60) << "\">"
			<< "<div class=\"bar\" style=\"width:" << pctKPM << "%\"></div>"
			<< "<span>" << std::fixed << std::setprecision(2) << kpm << "</span></td>";

		const double pctDMR = (maxDMR > 0) ? (dmr / maxDMR) * 100.0 : 0.0;
		html << "<td class=\"progress-cell\" title=\"DmgD: " << p->totalDmgDealt << ", DmgR: " << p->totalDmgReceived << "\">"
			<< "<div class=\"bar\" style=\"width:" << pctDMR << "%\"></div>"
			<< "<span>" << std::fixed << std::setprecision(2) << dmr << "</span></td>";

                const double pctScore = (maxScore > 0) ? (double(p->totalScore) / maxScore) * 100.0 : 0.0;
                html << "<td class=\"progress-cell\" title=\"Score relative to top (" << maxScore << ")\">"
                        << "<div class=\"bar\" style=\"width:" << pctScore << "%\"></div>"
                        << "<span>" << p->totalScore << "</span></td>";

                if (proBall) {
                        const double pctGoals = (maxGoals > 0) ? (double(p->proBallGoals) / maxGoals) * 100.0 : 0.0;
                        html << "<td class=\"progress-cell\" title=\"Goals scored\">"
                                << "<div class=\"bar\" style=\"width:" << pctGoals << "%\"></div>"
                                << "<span>" << p->proBallGoals << "</span></td>";

                        const double pctAssists = (maxAssists > 0) ? (double(p->proBallAssists) / maxAssists) * 100.0 : 0.0;
                        html << "<td class=\"progress-cell\" title=\"Goal assists credited\">"
                                << "<div class=\"bar\" style=\"width:" << pctAssists << "%\"></div>"
                                << "<span>" << p->proBallAssists << "</span></td>";
                }

                html << "</tr>\n";
        }

	html << "  </table>\n</div>\n";
}

/*
=============
Html_WriteTeamScores
=============
*/
static inline void Html_WriteTeamScores(std::ofstream& html,
	const std::vector<const PlayerStats*>& redPlayersOrig,
	const std::vector<const PlayerStats*>& bluePlayersOrig,
	int redScore, int blueScore,
	double matchDuration,
	int maxGlobalScore) {

	// Big header
	html << "<div class=\"team-score-header\">\n"
		<< "<span class=\"red\">" << redScore << "</span> | "
		<< "<span class=\"blue\">" << blueScore << "</span>\n"
		<< "</div>\n";

	std::vector<const PlayerStats*> redPlayers = redPlayersOrig;
	std::vector<const PlayerStats*> bluePlayers = bluePlayersOrig;

	std::sort(redPlayers.begin(), redPlayers.end(), [](const PlayerStats* a, const PlayerStats* b) {
		return a->totalScore > b->totalScore;
		});
	std::sort(bluePlayers.begin(), bluePlayers.end(), [](const PlayerStats* a, const PlayerStats* b) {
		return a->totalScore > b->totalScore;
		});

	auto writeOneTeam = [&](const std::vector<const PlayerStats*>& teamPlayers, const std::string& color, const std::string& teamName, bool isWinner) {
		const std::string escapedTeamName = HtmlEscape(teamName);
		html << "<div class=\"section team-" << color << "\">\n"
			<< "<h2>" << escapedTeamName;
		if (isWinner) html << " (Winner)";
		html << "</h2>\n";

		html << "<table>\n<tr>"
			<< "<th class=\"" << color << "\">Player</th>"
			<< "<th class=\"" << color << "\">%TIME</th>"
			<< "<th class=\"" << color << "\">SR</th>"
			<< "<th class=\"" << color << "\">KDR</th>"
			<< "<th class=\"" << color << "\">KPM</th>"
			<< "<th class=\"" << color << "\">DMR</th>"
			<< "<th class=\"" << color << "\">Score</th>"
			<< "</tr>\n";

		for (auto* p : teamPlayers) {
			const std::string escapedPlayerName = HtmlEscape(p->playerName);
			html << "<tr><td class=\"player-cell " << color << "\">" << escapedPlayerName << "</td>";

			double pctTime = (matchDuration > 0.0) ? (double(p->playTimeMsec) / matchDuration) * 100.0 : 0.0;
			if (pctTime < 1.0) pctTime = 1.0;

			double kdr = (p->totalDeaths > 0) ? (double(p->totalKills) / p->totalDeaths) : double(p->totalKills);
			double kpm = (matchDuration > 0.0) ? (p->totalKills / (matchDuration / 60000.0)) : 0.0;
			double dmr = (p->totalDmgReceived > 0) ? (double(p->totalDmgDealt) / p->totalDmgReceived) : double(p->totalDmgDealt);

			double pctScore = (maxGlobalScore > 0) ? (double(p->totalScore) / maxGlobalScore) * 100.0 : 0.0;
			if (pctScore < 1.0) pctScore = 1.0;

			html << "<td class=\"progress-cell " << color << "\"><div class=\"bar\" style=\"width:" << pctTime << "%\"></div><span>" << std::fixed << std::setprecision(1) << pctTime << "%</span></td>";

			// SR cell with delta
			html << "<td class=\"progress-cell " << color << "\" title=\"Skill Rating change from match: "
				<< (p->skillRatingChange >= 0 ? "+" : "") << p->skillRatingChange << "\">"
				<< "<div class=\"bar\" style=\"width:100%\"></div><span>"
				<< p->skillRating;
			if (p->skillRatingChange != 0) {
				html << " (" << (p->skillRatingChange >= 0 ? "+" : "") << p->skillRatingChange << ")";
			}
			html << "</span></td>";

			html << "<td class=\"progress-cell " << color << "\"><div class=\"bar\" style=\"width:" << std::max(kdr * 10.0, 1.0) << "%\"></div><span>" << std::fixed << std::setprecision(2) << kdr << "</span></td>"
				<< "<td class=\"progress-cell " << color << "\"><div class=\"bar\" style=\"width:" << std::max(kpm * 10.0, 1.0) << "%\"></div><span>" << std::fixed << std::setprecision(2) << kpm << "</span></td>"
				<< "<td class=\"progress-cell " << color << "\"><div class=\"bar\" style=\"width:" << std::max(dmr * 10.0, 1.0) << "%\"></div><span>" << std::fixed << std::setprecision(2) << dmr << "</span></td>"
				<< "<td class=\"progress-cell " << color << "\"><div class=\"bar\" style=\"width:" << pctScore << "%\"></div><span>" << p->totalScore << "</span></td>"
				<< "</tr>\n";
		}

		html << "</table>\n</div>\n";
		};

	bool redWins = redScore > blueScore;
	writeOneTeam(redPlayers, "red", "Red", redWins);
	writeOneTeam(bluePlayers, "blue", "Blue", !redWins);
}

/*
=============
Html_WriteTeamsComparison
=============
*/
static inline void Html_WriteTeamsComparison(std::ofstream& html,
        const std::vector<const PlayerStats*>& redPlayers,
        const std::vector<const PlayerStats*>& bluePlayers,
        double matchDurationMs) {
	html << "<div class=\"section\">\n<h2>Team Comparison</h2>\n<table>\n";

	html << "<tr>"
		<< "<th title=\"Comparison metric\">Metric</th>"
		<< "<th title=\"Red Team\">Red</th>"
		<< "<th title=\"Blue Team\">Blue</th>"
		<< "</tr>\n";

	auto calcTeamStats = [](const std::vector<const PlayerStats*>& players, double matchMinutes) -> std::tuple<double, double, double> {
		int kills = 0, deaths = 0, dmgDealt = 0, dmgTaken = 0;
		for (auto* p : players) {
			kills += p->totalKills;
			deaths += p->totalDeaths;
			dmgDealt += p->totalDmgDealt;
			dmgTaken += p->totalDmgReceived;
		}
		double kdr = deaths > 0 ? double(kills) / deaths : (kills > 0 ? double(kills) : 0.0);
		double kpm = matchMinutes > 0.0 ? double(kills) / matchMinutes : 0.0;
		double dmr = dmgTaken > 0 ? double(dmgDealt) / dmgTaken : (dmgDealt > 0 ? double(dmgDealt) : 0.0);
		return { kdr, kpm, dmr };
		};

	const double matchMinutes = matchDurationMs / 60000.0;

	auto [redKDR, redKPM, redDMR] = calcTeamStats(redPlayers, matchMinutes);
	auto [blueKDR, blueKPM, blueDMR] = calcTeamStats(bluePlayers, matchMinutes);

	auto writeRow = [&](const char* name, const char* tip, double redVal, double blueVal, const char* redTip, const char* blueTip) {
		html << "<tr><td title=\"" << tip << "\">" << name << "</td>"
			<< "<td title=\"" << redTip << "\">" << std::fixed << std::setprecision(2) << redVal << "</td>"
			<< "<td title=\"" << blueTip << "\">" << std::fixed << std::setprecision(2) << blueVal << "</td></tr>\n";
		};

	writeRow("KDR", "Kills divided by Deaths", redKDR, blueKDR, "Red Team KDR", "Blue Team KDR");
	writeRow("KPM", "Kills per Minute played", redKPM, blueKPM, "Red Team KPM", "Blue Team KPM");
	writeRow("DMR", "Damage dealt divided by Damage received", redDMR, blueDMR, "Red Team DMR", "Blue Team DMR");

	// Totals row
	double redAvg = (redKDR + redKPM + redDMR) / 3.0;
	double blueAvg = (blueKDR + blueKPM + blueDMR) / 3.0;

	html << "<tr><td><b>Average</b></td>"
		<< "<td>" << std::fixed << std::setprecision(2) << redAvg << "</td>"
		<< "<td>" << std::fixed << std::setprecision(2) << blueAvg << "</td></tr>\n";

	html << "</table>\n</div>\n";
}

/*
=============
Html_WriteTopPlayers
=============
*/
static inline void Html_WriteTopPlayers(std::ofstream& html, const MatchStats& matchStats, std::vector<const PlayerStats*> allPlayers) {
	html << "<div class=\"section\">\n<h2>Top Players</h2>\n";

	auto getPlayerColor = [&](const PlayerStats* p) -> const char* {
		if (!Teams()) return "green";
		for (const auto& tp : matchStats.teams[0].players)
			if (&tp == p) return "red";
		for (const auto& tp : matchStats.teams[1].players)
			if (&tp == p) return "blue";
		return "green"; // fallback
		};

	auto writeList = [&](const char* title, auto valueFn) {
		std::vector<std::pair<const PlayerStats*, double>> list;
		double maxVal = 0.0;

		for (auto* p : allPlayers) {
			double val = valueFn(p);
			if (val > 0.0) {
				list.push_back({ p, val });
				maxVal = std::max(maxVal, val);
			}
		}

		if (list.empty())
			return;

		std::sort(list.begin(), list.end(), [](auto& a, auto& b) { return a.second > b.second; });

		html << "<h3>" << title << "</h3>\n<table>\n<tr><th>Player</th><th>" << title << "</th></tr>\n";

		for (size_t i = 0; i < std::min<size_t>(10, list.size()); ++i) {
			const auto* p = list[i].first;
			double val = list[i].second;
			const char* color = getPlayerColor(p);

			const std::string escapedPlayerName = HtmlEscape(p->playerName);
			double pct = (maxVal > 0.0) ? (val / maxVal) * 100.0 : 0.0;
			if (pct < 1.0) pct = 1.0; // enforce minimum

			html << "<tr><td class=\"player-cell " << color << "\">" << escapedPlayerName << "</td>"
				<< "<td class=\"progress-cell " << color << "\">"
				<< "<div class=\"bar\" style=\"width:" << pct << "%\"></div>"
				<< "<span>" << std::fixed << std::setprecision(2) << val << "</span></td></tr>\n";
		}

		html << "</table>\n";
		};

	// Write 3 separate lists
	writeList("KDR", [](const PlayerStats* p) -> double {
		if (p->totalKills == 0 && p->totalDeaths == 0) return 0.0;
		return (p->totalDeaths > 0) ? (double(p->totalKills) / p->totalDeaths) : double(p->totalKills);
		});

	writeList("KPM", [](const PlayerStats* p) -> double {
		if (p->playTimeMsec <= 0) return 0.0;
		return (p->totalKills * 60.0) / (p->playTimeMsec / 1000.0f);
		});

	writeList("DMR", [](const PlayerStats* p) -> double {
		if (p->totalDmgDealt == 0 && p->totalDmgReceived == 0) return 0.0;
		return (p->totalDmgReceived > 0) ? (double(p->totalDmgDealt) / p->totalDmgReceived) : double(p->totalDmgDealt);
		});

	html << "</div>\n";
}

/*
=============
Html_WriteItemPickups
=============
*/
static inline void Html_WriteItemPickups(std::ofstream& html, const MatchStats& matchStats, const std::vector<const PlayerStats*>& allPlayers) {
	if (allPlayers.empty())
		return;

	std::unordered_map<std::string, uint32_t> itemTotals;
	std::unordered_map<std::string, double> itemDelays;

	auto getPickup = [](const PlayerStats* p, HighValueItems item) -> uint32_t {
		return (item > HighValueItems::None && item < HighValueItems::Total) ? p->pickupCounts[static_cast<int>(item)] : 0u;
		};

	auto getDelay = [](const PlayerStats* p, HighValueItems item) -> double {
		return (item > HighValueItems::None && item < HighValueItems::Total) ? p->pickupDelays[static_cast<int>(item)] : 0.0;
		};

	// Aggregate totals
	for (auto* p : allPlayers) {
		for (int i = static_cast<size_t>(HighValueItems::None) + 1; i < static_cast<size_t>(HighValueItems::Total); ++i) {
			HighValueItems item = static_cast<HighValueItems>(i);
			itemTotals[HighValueItemNames[i]] += getPickup(p, item);
			itemDelays[HighValueItemNames[i]] += getDelay(p, item);
		}
	}

	// Only items that were actually picked up
	std::vector<std::string> sortedItems;
	for (const auto& it : itemTotals) {
		if (it.second > 0)
			sortedItems.push_back(it.first);
	}
	std::sort(sortedItems.begin(), sortedItems.end(), [&](const std::string& a, const std::string& b) {
		return itemTotals[a] > itemTotals[b];
		});

	if (sortedItems.empty())
		return;

	html << "<div class=\"section\">\n<h2>Global High Value Item Pickups</h2>\n";

	html << "<div class=\"flex-container\">\n";

	// --- Players Table ---
	html << "<div class=\"flex-item\">\n";
	html << "<table>\n<tr><th>Player</th>";
	for (const auto& name : sortedItems) {
		html << "<th>" << name << "</th>";
	}
	html << "</tr>\n";

        bool wrotePlayerRow = false;

        for (auto* p : allPlayers) {
                bool hasPickup = false;
                for (const auto& name : sortedItems) {
                        for (int i = static_cast<int>(HighValueItems::None) + 1; i < static_cast<int>(HighValueItems::Total); ++i) {
                                if (HighValueItemNames[i] == name && getPickup(p, static_cast<HighValueItems>(i)) > 0) {
                                        hasPickup = true;
					break;
				}
			}
			if (hasPickup) break;
                }
                if (!hasPickup)
                        continue;

                wrotePlayerRow = true;

                std::string color = "green";
                if (Teams()) {
                        for (const auto& rp : matchStats.teams[0].players)
                                if (&rp == p) { color = "red"; break; }
                        for (const auto& bp : matchStats.teams[1].players)
				if (&bp == p) { color = "blue"; break; }
		}

		const std::string escapedPlayerName = HtmlEscape(p->playerName);
		html << "<tr><td class=\"player-cell " << color << "\">" << escapedPlayerName << "</td>";

		for (const auto& name : sortedItems) {
			int idx = -1;
			for (int i = static_cast<int>(HighValueItems::None) + 1; i < static_cast<int>(HighValueItems::Total); ++i) {
				if (HighValueItemNames[i] == name) {
					idx = i;
					break;
				}
			}
			if (idx == -1) {
				html << "<td>-</td>";
				continue;
			}

			uint32_t pickups = getPickup(p, static_cast<HighValueItems>(idx));
			double delay = getDelay(p, static_cast<HighValueItems>(idx));

			if (pickups > 0) {
				int avgSecs = static_cast<int>((delay / pickups) + 0.5);
				html << "<td>" << pickups << " (" << FormatDuration(avgSecs) << ")</td>";
			}
                        else {
                                html << "<td>-</td>";
                        }
                }

                html << "</tr>\n";
        }

        if (wrotePlayerRow) {
                html << "<tr><td><b>Totals</b></td>";

                for (const auto& name : sortedItems) {
                        auto total = itemTotals[name];
                        auto totalDelay = itemDelays[name];
                        if (total > 0) {
                                int avgSecs = static_cast<int>((totalDelay / total) + 0.5);
                                html << "<td>" << total << " (" << FormatDuration(avgSecs) << ")</td>";
                        }
                        else {
                                html << "<td>-</td>";
                        }
                }

                html << "</tr>\n";
        }

        html << "</table>\n</div>\n"; // flex-item (players)

	// --- Team Totals Table ---
	if (Teams()) {
		uint32_t redTotal = 0, blueTotal = 0;
		double redDelay = 0.0, blueDelay = 0.0;

		for (auto* p : allPlayers) {
			bool isRed = false;
			for (const auto& rp : matchStats.teams[0].players)
				if (&rp == p) { isRed = true; break; }
			for (const auto& bp : matchStats.teams[1].players)
				if (&bp == p) { isRed = false; break; }
			for (int i = static_cast<int>(HighValueItems::None) + 1; i < static_cast<int>(HighValueItems::Total); ++i) {
				if (isRed) {
					redTotal += getPickup(p, static_cast<HighValueItems>(i));
					redDelay += getDelay(p, static_cast<HighValueItems>(i));
				}
				else {
					blueTotal += getPickup(p, static_cast<HighValueItems>(i));
					blueDelay += getDelay(p, static_cast<HighValueItems>(i));
				}
			}
		}

		html << "<div class=\"flex-item\">\n";
		html << "<h3>Team Item Pickup Summary</h3>\n<table>\n<tr><th>Team</th><th>Total Pickups</th><th>Avg Delay</th></tr>\n";

		int redAvgSecs = (redTotal > 0) ? static_cast<int>((redDelay / redTotal) + 0.5) : 0;
		int blueAvgSecs = (blueTotal > 0) ? static_cast<int>((blueDelay / blueTotal) + 0.5) : 0;

		html << "<tr><td class=\"player-cell red\">Red</td><td>" << redTotal << "</td><td>" << FormatDuration(redAvgSecs) << "</td></tr>\n";
		html << "<tr><td class=\"player-cell blue\">Blue</td><td>" << blueTotal << "</td><td>" << FormatDuration(blueAvgSecs) << "</td></tr>\n";

		html << "</table>\n</div>\n"; // flex-item (teams)
	}

	html << "</div>\n"; // flex-container
	html << "</div>\n"; // section
}

/*
=============
Html_WriteTopMeansOfDeath
=============
*/
static inline void Html_WriteTopMeansOfDeath(std::ofstream& html, const MatchStats& matchStats, const std::vector<const PlayerStats*>& redPlayers, const std::vector<const PlayerStats*>& bluePlayers) {
	html << "<div class=\"section\">\n<h2>Deaths by Type</h2>\n<table>\n";

	if (Teams()) {
		html << "<tr><th>MOD</th><th>Red</th><th>Blue</th><th>Total</th></tr>\n";
	}
	else {
		html << "<tr><th>MOD</th><th>Total</th></tr>\n";
	}

	// Build the MOD list
	std::vector<std::string> mods;
	for (auto& kv : matchStats.totalDeathsByMOD) {
		if (kv.second > 0)
			mods.push_back(kv.first);
	}

	std::sort(mods.begin(), mods.end(), [&](const std::string& a, const std::string& b) {
		return matchStats.totalDeathsByMOD.at(a) > matchStats.totalDeathsByMOD.at(b);
		});

	for (auto& modName : mods) {
		int total = matchStats.totalDeathsByMOD.at(modName);
		const std::string escapedModName = HtmlEscape(modName);

		if (!Teams()) {
			// Solo mode
			html << "<tr><td>" << escapedModName << "</td><td>" << total << "</td></tr>\n";
		}
		else {
			// Team mode: split
			int redDeaths = 0, blueDeaths = 0;

			for (auto* p : redPlayers) {
				auto it = p->modTotalDeaths.find(getModIdByName(modName));
				if (it != p->modTotalDeaths.end())
					redDeaths += it->second;
			}
			for (auto* p : bluePlayers) {
				auto it = p->modTotalDeaths.find(getModIdByName(modName));
				if (it != p->modTotalDeaths.end())
					blueDeaths += it->second;
			}

			html << "<tr><td>" << escapedModName << "</td><td>" << redDeaths << "</td><td>" << blueDeaths << "</td><td>" << (redDeaths + blueDeaths) << "</td></tr>\n";
		}
	}

	html << "</table>\n</div>\n";
}

/*
=============
Html_WriteEventLog
=============
*/
static inline void Html_WriteEventLog(std::ofstream& html, const MatchStats& matchStats, const std::vector<const PlayerStats*>& allPlayers) {
	if (level.match.eventLog.empty())
		return;

	int matchDuration = matchStats.durationMS;

	// === Precompute name replacements ===
	std::unordered_map<std::string, std::string> nameToHtml;

	for (const auto* p : allPlayers) {
		std::string name = p->playerName;
		std::string color = "green";

		if (Teams()) {
			for (const auto& tp : matchStats.teams[0].players)
				if (&tp == p) { color = "red"; break; }
			for (const auto& tp : matchStats.teams[1].players)
				if (&tp == p) { color = "blue"; break; }
		}

		const std::string escapedName = HtmlEscape(name);
		if (Teams()) {
			nameToHtml[escapedName] = "<span class=\"player-name " + color + "\"><b>" + escapedName + "</b></span>";
		}
		else {
			nameToHtml[escapedName] = "<b>" + escapedName + "</b>";
		}
	}

	// === Render event log ===
	html << "<div class=\"section\">\n<h2>Event Log</h2>\n<table>\n<tr><th>Time</th><th>Event</th></tr>\n";

	for (auto& e : level.match.eventLog) {
		int secs = static_cast<int>(e.time.seconds());
		double pctTime = (matchDuration > 0.0) ? (double(secs) / matchDuration) * 100.0 : 0.0;
		if (pctTime < 1.0) pctTime = 1.0;

		// Start with original string
		std::string evStr = HtmlEscape(e.eventStr);

		// Replace player names
		for (auto& kv : nameToHtml) {
			size_t pos = evStr.find(kv.first);
			if (pos != std::string::npos) {
				evStr.replace(pos, kv.first.length(), kv.second);
			}
		}

		// --- Write the event row ---
		html << "<tr><td class=\"progress-cell green\" title=\"" << secs << " seconds\">"
			<< "<div class=\"bar\" style=\"width:" << pctTime << "%\"></div>"
			<< "<span>";

		int h = secs / 3600;
		int m = (secs % 3600) / 60;
		int s = secs % 60;
		if (h > 0)
			html << h << "h " << m << "m " << s << "s";
		else if (m > 0)
			html << m << "m " << s << "s";
		else
			html << s << "s";

		html << "</span></td><td>" << evStr << "</td></tr>\n";
	}

	html << "</table>\n</div>\n";
}

/*
=============
Html_WriteIndividualPlayerSections
=============
*/
static inline void Html_WriteIndividualPlayerSections(std::ofstream& html, const MatchStats& matchStats, std::vector<const PlayerStats*> allPlayers) {
	for (const PlayerStats* p : allPlayers) {
		html << "<div class=\"section\">";
		const std::string fullID = p->socialID;
		const std::string escapedFullId = HtmlEscape(fullID);
		const std::string escapedPlayerName = HtmlEscape(p->playerName);
		const std::string steamPref = "Steamworks-";
		const std::string gogPref = "Galaxy-";
		std::string profileURL;

		// Steam branch
		if (fullID.rfind(steamPref, 0) == 0) {
			// strip "Steamworks-"
			auto id = fullID.substr(steamPref.size());
			profileURL = "https://steamcommunity.com/profiles/" + id;

			// GOG branch
		}
		else if (fullID.rfind(gogPref, 0) == 0) {
			// strip "GOG-" -> leftover is the user's GOG slug
			auto slug = fullID.substr(gogPref.size());
			profileURL = "https://www.gog.com/u/" + slug;
		}

		// emit the header
		const std::string escapedProfileURL = HtmlEscape(profileURL);
		html << "  <h2 id=\"player-" << escapedFullId << "\">Player: " << escapedPlayerName << " (";
		if (!profileURL.empty()) {
			html << "<a href=\"" << escapedProfileURL << "\">" << escapedFullId << "</a>";
		}
		else {
			html << escapedFullId;
		}
		html << ")</h2>";

		// Top-line summary
		if (Teams()) {
			html << "  <p>"
				<< "Kills: " << p->totalKills
				<< " | SpawnKills: " << p->totalSpawnKills
				<< " | TeamKills: " << p->totalTeamKills
				<< " | Deaths: " << p->totalDeaths
				<< " | Suicides: " << p->totalSuicides
				<< " | Score: " << p->totalScore
				<< "</p>";
		}
		else {
			html << "  <p>"
				<< "Kills: " << p->totalKills
				<< " | SpawnKills: " << p->totalSpawnKills
				<< " | Deaths: " << p->totalDeaths
				<< " | Suicides: " << p->totalSuicides
				<< " | Score: " << p->totalScore
				<< "</p>";
		}

		// Top Victims by this player
		{
			std::unordered_map<std::string, int> victimCounts;
			for (const auto& e : level.match.deathLog) {
				if (e.attacker.id == p->socialID) {
					victimCounts[e.victim.name]++;
				}
			}
			std::vector<std::pair<std::string, int>> victims(victimCounts.begin(), victimCounts.end());
			std::sort(victims.begin(), victims.end(), [](auto& a, auto& b) { return a.second > b.second; });
			html << "  <h3>Top Victims by " << escapedPlayerName << "</h3>"
				<< "  <table><tr><th>Player</th><th>Kills</th></tr>";
			for (size_t i = 0; i < std::min<size_t>(10, victims.size()); ++i) {
				const std::string escapedVictim = HtmlEscape(victims[i].first);
				html << "    <tr><td>" << escapedVictim
					<< "</td><td>" << victims[i].second << "</td></tr>";
			}
			html << "  </table>";
		}

		// Top Killers of this player
		{
			std::unordered_map<std::string, int> killerCounts;
			for (const auto& e : level.match.deathLog) {
				if (e.victim.id == p->socialID) {
					killerCounts[e.attacker.name]++;
				}
			}
			std::vector<std::pair<std::string, int>> killers(killerCounts.begin(), killerCounts.end());
			std::sort(killers.begin(), killers.end(), [](auto& a, auto& b) { return a.second > b.second; });
			html << "  <h3>Top Killers of " << escapedPlayerName << "</h3>"
				<< "  <table><tr><th>Player</th><th>Deaths</th></tr>";
			for (size_t i = 0; i < std::min<size_t>(10, killers.size()); ++i) {
				const std::string escapedKiller = HtmlEscape(killers[i].first);
				html << "    <tr><td>" << escapedKiller
					<< "</td><td>" << killers[i].second << "</td></tr>";
			}
			html << "  </table>";
		}

		// Weapon Stats (only used weapons, sorted by accuracy desc)
		html << "  <h3>Weapon Stats</h3>"
			<< "  <table><tr><th>Weapon</th><th>Shots</th><th>Hits</th><th>Acc (%)</th></tr>";
		{
			std::vector<std::string> used;
			for (auto& kv : p->totalShotsPerWeapon) {
				if (kv.second > 0 || p->totalHitsPerWeapon.at(kv.first) > 0) {
					used.push_back(kv.first);
				}
			}
			std::sort(used.begin(), used.end(), [&](const std::string& a, const std::string& b) {
				return p->accuracyPerWeapon.at(a) > p->accuracyPerWeapon.at(b);
				});
			for (auto& w : used) {
				html << "    <tr>"
					<< "<td>" << w << "</td>"
					<< "<td>" << p->totalShotsPerWeapon.at(w) << "</td>"
					<< "<td>" << p->totalHitsPerWeapon.at(w) << "</td>"
					<< std::fixed << std::setprecision(1)
					<< "<td>" << p->accuracyPerWeapon.at(w) << "</td></tr>";
			}
		}
		html << "  </table>";

		// Means-of-Death Stats (MOD rows sorted by KDR)
		html << "  <h3>Means-of-Death Stats</h3>"
			<< "  <table><tr><th>MOD</th><th>Kills</th><th>Deaths</th><th>KDR</th><th>DmgD</th><th>DmgR</th></tr>";
		{
			struct Row { std::string mod; int k, d; double kdr; int dd, dr; };
			std::vector<Row> rows;
			for (auto& mr : modr) {
				int kills = p->modTotalKills.at(mr.mod);
				int deaths = p->modTotalDeaths.at(mr.mod);
				if (!kills && !deaths) continue;
				double ratio = deaths > 0 ? double(kills) / deaths : (kills > 0 ? double(kills) : 0.0);
				rows.push_back({ mr.name,kills,deaths,ratio,p->modTotalDmgD.at(mr.mod),p->modTotalDmgR.at(mr.mod) });
			}
			std::sort(rows.begin(), rows.end(), [](auto& a, auto& b) { return a.kdr > b.kdr; });
			for (auto& r : rows) {
				html << "    <tr><td>" << r.mod
					<< "</td><td>" << r.k
					<< "</td><td>" << r.d
					<< std::fixed << std::setprecision(2)
					<< "</td><td>" << r.kdr
					<< std::setprecision(0)
					<< "</td><td>" << r.dd
					<< "</td><td>" << r.dr
					<< "</td></tr>";
			}
		}
		html << "  </table>";

		// Awards (only if earned)
		{
			std::vector<std::pair<std::string, int>> aw;
			for (size_t i = 0; i < static_cast<uint8_t>(PlayerMedal::Total); i++) {
				if (p->awards[i] > 0) aw.emplace_back(awardNames[i], p->awards[i]);
			}
			if (!aw.empty()) {
				std::sort(aw.begin(), aw.end(), [](auto& a, auto& b) {return a.second > b.second; });
				html << "  <h3>Awards</h3>"
					<< "  <table><tr><th>Award</th><th>Count</th></tr>";
				for (auto& e : aw) {
					html << "    <tr><td>" << e.first
						<< "</td><td>" << e.second << "</td></tr>";
				}
				html << "  </table>";
			}
		}

		html << "</div>";
	}
}

/*
=============
Html_WriteFooter
=============
*/
static inline void Html_WriteFooter(std::ofstream& html, const std::string& htmlPath) {
        html << "<div class=\"footer\">Compiled by " << worr::version::kGameTitle << " "
             << worr::version::kGameVersion << "</div>\n";
	html << "</body></html>\n";
	html.close();
}

/*
=============
MatchStats_WriteHtml
=============
*/
static bool MatchStats_WriteHtml(const MatchStats& matchStats, const std::string& htmlPath) {
	std::ofstream html(htmlPath);
	if (!html.is_open()) {
		const std::error_code ec(errno, std::system_category());
		gi.Com_PrintFmt("Failed to open HTML file: {} ({})\n", htmlPath.c_str(), ec.message().c_str());
		return false;
	}

	// Gather players
	std::vector<const PlayerStats*> allPlayers;
	std::vector<const PlayerStats*> redPlayers;
	std::vector<const PlayerStats*> bluePlayers;

	int redScore = 0, blueScore = 0;
	int maxGlobalScore = 0;

	// solo players
	for (const auto& p : matchStats.players) {
		allPlayers.push_back(&p);
		maxGlobalScore = std::max(maxGlobalScore, p.totalScore);
	}

	// team players
	for (size_t i = 0; i < matchStats.teams.size(); ++i) {
		const auto& team = matchStats.teams[i];
		if (i == 0) redScore = team.score;
		if (i == 1) blueScore = team.score;

		for (const auto& p : team.players) {
			allPlayers.push_back(&p);
			maxGlobalScore = std::max(maxGlobalScore, p.totalScore);
			if (i == 0)
				redPlayers.push_back(&p);
			else if (i == 1)
				bluePlayers.push_back(&p);
		}
	}

	// Sort by totalScore descending
	std::sort(allPlayers.begin(), allPlayers.end(), [](auto a, auto b) {
		return a->totalScore > b->totalScore;
		});

	Html_WriteHeader(html, matchStats);
	Html_WriteTopInfo(html, matchStats);
	Html_WriteWinnerSummary(html, matchStats);

	if (Teams()) {
		Html_WriteTeamScores(html, redPlayers, bluePlayers, redScore, blueScore, matchStats.durationMS, maxGlobalScore);
		const double matchDurationMs = static_cast<double>(matchStats.durationMS);
		Html_WriteTeamsComparison(html, redPlayers, bluePlayers, matchDurationMs);
	}
	else {
		Html_WriteOverallScores(html, matchStats, allPlayers);
	}

	Html_WriteTopPlayers(html, matchStats, allPlayers);
	Html_WriteItemPickups(html, matchStats, allPlayers);
	Html_WriteTopMeansOfDeath(html, matchStats, redPlayers, bluePlayers);
	Html_WriteEventLog(html, matchStats, allPlayers);
	Html_WriteIndividualPlayerSections(html, matchStats, allPlayers);
	Html_WriteFooter(html, htmlPath);

	html.close();
	gi.Com_PrintFmt("Match HTML report written to {}\n", htmlPath.c_str());
	return true;
}

/*
=============
MatchStats_WriteAll
=============
*/
static void SendIndividualMiniStats(const MatchStats& matchStats) {
	for (auto ec : active_players()) {
		if (!ec || !ec->client)
			continue;

                const char* rawName = ec->client->sess.netName;

                if (!rawName[0]) {
                        gi.Com_PrintFmt("SendIndividualMiniStats: skipping client {} due to empty netName\n", ec->s.number);
                        continue;
                }

                std::string_view name(rawName);

		const PlayerStats* matchedStats = nullptr;

		auto matchesName = [&](const PlayerStats& stats) {
			if (stats.playerName.empty())
				return false;

			return _stricmp(stats.playerName.c_str(), name.data()) == 0;
			};

		for (const PlayerStats& p : matchStats.players) {
			if (!matchesName(p))
				continue;

			matchedStats = &p;
			break;
		}

		if (!matchedStats) {
			for (const TeamStats& team : matchStats.teams) {
				for (const PlayerStats& teamPlayer : team.players) {
					if (!matchesName(teamPlayer))
						continue;

					matchedStats = &teamPlayer;
					break;
				}

				if (matchedStats)
					break;
			}
		}

		if (!matchedStats)
			continue;

		const PlayerStats& p = *matchedStats;

		std::string msg;
		msg += ":: Match Summary ::\n";
		msg += G_Fmt("{} - ", rawName);
		msg += G_Fmt("Kills: {} | Deaths: {}", p.totalKills, p.totalDeaths);

		msg += G_Fmt(" | K/D Ratio: {:.2f}", p.totalKDR);
		/*
		double total = p.totalKills + p.totalAssists + p.totalDeaths;
		double eff = total > 0 ? (double)p.totalKills / total * 100.0 : 0.0;
		msg += G_Fmt(" | Eff: {:.1f}%%\n", eff);
		*/
		gi.LocClient_Print(ec, PRINT_HIGH, "{}\n", msg.c_str());
	}
}
/*
=============
MatchStats_WriteAll

Ensures the destination directory exists, then writes JSON and HTML exports for the
current match while reporting any errors that occur during the process.
=============
*/
static void MatchStats_WriteAll(MatchStats& matchStats, const std::string& baseFilePath) {
	const std::filesystem::path basePath(baseFilePath);
	const std::filesystem::path directory = basePath.parent_path();

	if (!directory.empty()) {
		std::error_code dirError;
		std::filesystem::create_directories(directory, dirError);
		if (dirError) {
			gi.Com_PrintFmt("{}: Failed to create directory '{}': {}\n", __FUNCTION__, directory.string().c_str(), dirError.message().c_str());
			return;
		}
	}

	const bool jsonWritten = MatchStats_WriteJson(matchStats, baseFilePath + ".json");
	const bool htmlWritten = MatchStats_WriteHtml(matchStats, baseFilePath + ".html");
	if (!jsonWritten || !htmlWritten) {
		gi.Com_PrintFmt("{}: Export completed with errors (JSON: {}, HTML: {})\n", __FUNCTION__, jsonWritten ? "ok" : "failed", htmlWritten ? "ok" : "failed");
	}

	SendIndividualMiniStats(matchStats);

	level.match.deathLog.clear();
	level.match.eventLog.clear();
	matchStats.players.clear();
	matchStats.teams.clear();
}

/*
=============
MatchStats_End
=============
*/
void MatchStats_End() {
	if (!deathmatch->integer)
		return;

	G_LogEvent("MATCH END");

	if (!g_statex_enabled->integer) {
		gi.Com_PrintFmt("{}: Reporting disabled.\n", __FUNCTION__);
		return;
	}

	if (g_statex_humans_present->integer && !level.pop.num_playing_human_clients) {
		gi.Com_PrintFmt("{}: No reporting without human players.\n", __FUNCTION__);
		return;
	}

	try {
		matchStats.matchID = level.matchID;
                matchStats.gameType = std::string(Game::GetCurrentInfo().short_name_upper);
                matchStats.ruleSet = rs_long_name[game.ruleset];
                matchStats.serverName = hostname->string ? hostname->string : "";
                matchStats.serverHostName.clear();
                if (host && host->client) {
                        char hostNameValue[MAX_INFO_VALUE] = { 0 };
                        gi.Info_ValueForKey(host->client->pers.userInfo, "name", hostNameValue, sizeof(hostNameValue));
                        if (hostNameValue[0] != '\0') {
                                matchStats.serverHostName = hostNameValue;
                        }
                }
                matchStats.mapName.assign(CharArrayToStringView(level.mapName));
                matchStats.ranked = false;
                matchStats.totalKills = level.match.totalKills;
                matchStats.totalSpawnKills = level.match.totalSpawnKills;
                matchStats.totalTeamKills = level.match.totalTeamKills;
                matchStats.totalDeaths = level.match.totalDeaths;
                matchStats.totalSuicides = level.match.totalSuicides;
                matchStats.proBall_totalGoals = level.match.proBallGoals;
                matchStats.proBall_totalAssists = level.match.proBallAssists;

                matchStats.calculateDuration();
		matchStats.avKillsPerMinute = matchStats.durationMS > 0
			? level.match.totalKills / (matchStats.durationMS / 60000.0f)
			: 0.0;

		auto process_player = [&](gentity_t* ec) {
			auto* cl = ec->client;
			PlayerStats p;

			p.socialID = cl->sess.socialID;
			p.playerName = cl->sess.netName;
			p.skillRating = cl->sess.skillRating;
			p.skillRatingChange = cl->sess.skillRatingChange;
			p.totalKills = cl->pers.match.totalKills;
			p.totalSpawnKills = cl->pers.match.totalSpawnKills;
			p.totalTeamKills = cl->pers.match.totalTeamKills;
			p.totalDeaths = cl->pers.match.totalDeaths;
                        p.totalSuicides = cl->pers.match.totalSuicides;
                        p.calculateKDR();
                        p.totalScore = cl->resp.score;
                        p.proBallGoals = cl->pers.match.proBallGoals;
                        p.proBallAssists = cl->pers.match.proBallAssists;
                        p.totalShots = cl->pers.match.totalShots;
			p.totalHits = cl->pers.match.totalHits;
			p.totalDmgDealt = cl->pers.match.totalDmgDealt;
			p.totalDmgReceived = cl->pers.match.totalDmgReceived;

			p.playTimeMsec = cl->sess.playEndRealTime - cl->sess.playStartRealTime;
			if (p.playTimeMsec > 0)
				p.killsPerMinute = (p.totalKills * 60.0) / (p.playTimeMsec / 1000.0f);

			// Weapon stats
			for (const auto& weapon : weaponAbbreviations) {
				Weapon index = getWeaponIndex(weapon);
				if (index < Weapon::None || index >= Weapon::Total)
					continue;

				int shots = cl->pers.match.totalShotsPerWeapon[static_cast<uint8_t>(index)];
				int hits = cl->pers.match.totalHitsPerWeapon[static_cast<uint8_t>(index)];
				if (shots > 0) {
					p.totalShotsPerWeapon[weapon] = shots;
					p.totalHitsPerWeapon[weapon] = hits;
					p.accuracyPerWeapon[weapon] = (double)hits / shots * 100.0;
				}
			}

			// Accuracy
			p.totalAccuracy = (p.totalShots > 0)
				? (double)p.totalHits / p.totalShots * 100.0
				: 0.0;

			// Pickup stats
			for (int i = static_cast<int>(HighValueItems::None) + 1; i < static_cast<int>(HighValueItems::Total); ++i) {
				p.pickupCounts[i] = cl->pers.match.pickupCounts[i];
				p.pickupDelays[i] = cl->pers.match.pickupDelay[i].seconds<double>();
			}

			// MOD stats
			for (const auto& mod : modr) {
				int kills = cl->pers.match.modTotalKills[static_cast<int>(mod.mod)];
				int deaths = cl->pers.match.modTotalDeaths[static_cast<int>(mod.mod)];
				int dmgDealt = cl->pers.match.modTotalDmgD[static_cast<int>(mod.mod)];
				int dmgReceived = cl->pers.match.modTotalDmgR[static_cast<int>(mod.mod)];

				if (dmgDealt > 0)
					p.modTotalDmgD[mod.mod] = dmgDealt;
				if (dmgReceived > 0)
					p.modTotalDmgR[mod.mod] = dmgReceived;

				if (kills > 0 || deaths > 0) {
					p.modTotalKills[mod.mod] = kills;
					p.modTotalDeaths[mod.mod] = deaths;
					double kdr = (deaths > 0)
						? (double)kills / deaths
						: (kills > 0 ? (double)kills : 0.0);
					p.modTotalKDR[mod.mod] = kdr;
				}
			}

			// Medals
			p.awards = cl->pers.match.medalCount;

			// Bot sanitization
			if (cl->sess.is_a_bot) {
				p.skillRating = 0;
				p.skillRatingChange = 0;
			}

			bool won = false;
			switch (cl->sess.team) {
			case Team::Red:
				if (level.teamScores[static_cast<int>(Team::Red)] > level.teamScores[static_cast<int>(Team::Blue)]) {
					won = true;
				}
				else {
					won = false;
				}
				break;
			case Team::Blue:
				if (level.teamScores[static_cast<int>(Team::Blue)] > level.teamScores[static_cast<int>(Team::Red)]) {
					won = true;
				}
				else {
					won = false;
				}
				break;
			default:
				won = (cl == &game.clients[level.sortedClients[0]]);
				break;
			}

			// Save persistent stats
			ClientConfig_SaveStats(cl, won);

			return p;
			};

		if (Teams()) {
			TeamStats redTeam = { "Red",  level.teamScores[static_cast<int>(Team::Red)],  level.teamScores[static_cast<int>(Team::Red)] > level.teamScores[static_cast<int>(Team::Blue)] ? "win" : "loss" };
			TeamStats blueTeam = { "Blue", level.teamScores[static_cast<int>(Team::Blue)], level.teamScores[static_cast<int>(Team::Blue)] > level.teamScores[static_cast<int>(Team::Red)] ? "win" : "loss" };

			for (auto ec : active_players()) {
				PlayerStats ps = process_player(ec);
				switch (ec->client->sess.team) {
				case Team::Red:  redTeam.players.push_back(ps); break;
				case Team::Blue: blueTeam.players.push_back(ps); break;
				}
			}

			matchStats.teams.push_back(redTeam);
			matchStats.teams.push_back(blueTeam);
		}
		else {
			for (auto ec : active_players()) {
				matchStats.players.push_back(process_player(ec));
			}
		}

		std::unordered_set<std::string> accountedPlayerIDs;
		auto accumulateModTotals = [&](const std::vector<PlayerStats>& playersVec) {
			for (const auto& p : playersVec) {
				accountedPlayerIDs.insert(p.socialID);

				for (const auto& [modId, kills] : p.modTotalKills) {
					if (kills <= 0)
						continue;

					const auto& modName = modr[static_cast<int>(modId)].name;
					matchStats.totalKillsByMOD[modName] += kills;
				}

				for (const auto& [modId, deaths] : p.modTotalDeaths) {
					if (deaths <= 0)
						continue;

					const auto& modName = modr[static_cast<int>(modId)].name;
					matchStats.totalDeathsByMOD[modName] += deaths;
				}
			}
			};

		accumulateModTotals(matchStats.players);
		for (const auto& team : matchStats.teams) {
			accumulateModTotals(team.players);
		}

		auto isAccounted = [&](const std::string& id) {
			return !id.empty() && accountedPlayerIDs.find(id) != accountedPlayerIDs.end();
			};

		for (const auto& e : level.match.deathLog) {
			const auto& modName = modr[static_cast<int>(e.mod.id)].name;
			const bool attackerAccounted = isAccounted(e.attacker.id);
			const bool victimAccounted = isAccounted(e.victim.id);
			const bool environmentKill = e.attacker.id.empty() || e.attacker.id == "0";
			const bool suicide = !environmentKill && !e.attacker.id.empty() && e.attacker.id == e.victim.id;

			if (!victimAccounted) {
				matchStats.totalDeathsByMOD[modName]++;
			}

			if (!attackerAccounted && !environmentKill && !suicide) {
				matchStats.totalKillsByMOD[modName]++;
			}
		}

		for (auto& [modName, kills] : matchStats.totalKillsByMOD) {
			int deaths = matchStats.totalDeathsByMOD[modName];
			matchStats.totalKDRByMOD[modName] = deaths > 0
				? (double)kills / deaths
				: (double)kills;
		}

		int aggregatedKillSum = 0;
		for (const auto& [modName, kills] : matchStats.totalKillsByMOD) {
			aggregatedKillSum += kills;
		}

		int aggregatedDeathSum = 0;
		for (const auto& [modName, deaths] : matchStats.totalDeathsByMOD) {
			aggregatedDeathSum += deaths;
		}

		if (aggregatedKillSum != level.match.totalKills) {
			gi.Com_PrintFmt("{}: totalKillsByMOD mismatch ({} != {})\n", __FUNCTION__, aggregatedKillSum, level.match.totalKills);
		}

		if (aggregatedDeathSum != level.match.totalDeaths) {
			gi.Com_PrintFmt("{}: totalDeathsByMOD mismatch ({} != {})\n", __FUNCTION__, aggregatedDeathSum, level.match.totalDeaths);
		}

		MatchStats_WriteAll(matchStats, MATCH_STATS_PATH + "/" + level.matchID);
	}
	catch (const std::exception& e) {
		gi.Com_PrintFmt("{}: exception: {}\n", __FUNCTION__, e.what());
	}
}

/*
=============
MatchStats_Init
=============
*/
void MatchStats_Init() {
	if (!deathmatch->integer)
		return;

	// Clear any previous data
	matchStats = MatchStats{};
	level.match.deathLog.clear();
	level.match.eventLog.clear();

	level.matchID = std::format("{}_{}",
		GametypeIndexToString(static_cast<GameType>(g_gametype->integer)),
		FileTimeStamp());

	matchStats.matchID = level.matchID;
	//matchStats.startTime = level.matchStartRealTime.seconds();	// std::time(nullptr);

	gi.LocBroadcast_Print(PRINT_TTS, "Match start for ID: {}\n", level.matchID.c_str());

	G_LogEvent("MATCH START");
}
