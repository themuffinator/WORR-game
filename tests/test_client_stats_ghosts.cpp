/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_client_stats_ghosts.cpp implementation.*/

#include "server/g_local.hpp"
#include "server/client/client_stats_service.hpp"
#include "server/gameplay/client_config.hpp"

#include <cassert>
#include <cstring>
#include <string>
#include <vector>

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
std::mt19937 mt_rand{};
cvar_t g_verbose_storage{};
cvar_t* g_verbose = &g_verbose_storage;

struct GhostSaveRecord {
	std::string id;
	int skillRating;
	int skillRatingChange;
	bool won;
};

static std::vector<GhostSaveRecord> g_savedGhosts;
static int g_defaultSkillRating = 1500;

/*
=============
ClientConfigStore::ClientConfigStore

Test stub constructor.
=============
*/
ClientConfigStore::ClientConfigStore(local_game_import_t&, std::string) {}

/*
=============
ClientConfigStore::LoadProfile

Unused stub.
=============
*/
bool ClientConfigStore::LoadProfile(gclient_t*, const std::string&, const std::string&, const std::string&) {
	return false;
}

/*
=============
ClientConfigStore::SaveStats

Unused stub.
=============
*/
void ClientConfigStore::SaveStats(gclient_t*, bool) {}

/*
=============
ClientConfigStore::SaveStatsForGhost

Captures ghost results for verification.
=============
*/
void ClientConfigStore::SaveStatsForGhost(const Ghosts& ghost, bool wonMatch) {
	GhostSaveRecord record{};
	record.id = ghost.socialID;
	record.skillRating = ghost.skillRating;
	record.skillRatingChange = ghost.skillRatingChange;
	record.won = wonMatch;
	g_savedGhosts.push_back(record);
}

/*
=============
ClientConfigStore::SaveWeaponPreferences

Unused stub.
=============
*/
void ClientConfigStore::SaveWeaponPreferences(gclient_t*) {}

/*
=============
ClientConfigStore::DefaultSkillRating

Returns the configured default for testing.
=============
*/
int ClientConfigStore::DefaultSkillRating() const {
	return g_defaultSkillRating;
}

/*
=============
ClientConfigStore::PlayerNameForSocialID

Unused stub.
=============
*/
std::string ClientConfigStore::PlayerNameForSocialID(const std::string&) const {
	return {};
}

/*
=============
GetClientConfigStore

Provides access to the test stub.
=============
*/
ClientConfigStore& GetClientConfigStore() {
	static ClientConfigStore store(gi, "");
	return store;
}

#include "server/client/client_stats_service.cpp"

/*
=============
main

Ensures ghost stats persist when no participants are present.
=============
*/
int main() {
	Ghosts ghostA{};
	std::strncpy(ghostA.socialID, "ghostA", sizeof(ghostA.socialID));
	ghostA.socialID[sizeof(ghostA.socialID) - 1] = '\0';
	ghostA.score = 15;
	ghostA.skillRating = 1550;
	ghostA.team = Team::Red;

	Ghosts ghostB{};
	std::strncpy(ghostB.socialID, "ghostB", sizeof(ghostB.socialID));
	ghostB.socialID[sizeof(ghostB.socialID) - 1] = '\0';
	ghostB.score = 10;
	ghostB.skillRating = 1450;
	ghostB.team = Team::Blue;

	g_savedGhosts.clear();

	worr::server::client::MatchStatsContext context{};
	context.mode = GameType::FreeForAll;
	context.isTeamMode = false;
	context.allowSkillAdjustments = true;
	context.ghosts.push_back(&ghostA);
	context.ghosts.push_back(&ghostB);

	worr::server::client::GetClientStatsService().PersistMatchResults(context);

	assert(g_savedGhosts.size() == 2);
	assert(g_savedGhosts[0].won);
	assert(!g_savedGhosts[1].won);
	assert(ghostA.skillRatingChange > 0);
	assert(ghostB.skillRatingChange < 0);

	return 0;
}
