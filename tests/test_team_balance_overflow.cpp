/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_team_balance_overflow.cpp implementation.*/

#include <array>
#include <cassert>
#include <vector>
#include <type_traits>

namespace std {
	using ::sinf;

	template<typename E>
	constexpr auto to_underlying(E e) noexcept {
		return static_cast<std::underlying_type_t<E>>(e);
	}
}

#include "server/gameplay/team_balance.hpp"

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
game_export_t globals{};
gentity_t* g_entities = nullptr;
std::mt19937 mt_rand{};
static cvar_t coop_storage{};
cvar_t* coop = &coop_storage;
static cvar_t minplayers_storage{};
cvar_t* minplayers = &minplayers_storage;
static cvar_t g_gametype_storage{};
cvar_t* g_gametype = &g_gametype_storage;
static cvar_t g_teamplay_force_balance_storage{};
cvar_t* g_teamplay_force_balance = &g_teamplay_force_balance_storage;
static cvar_t g_teamplay_auto_balance_storage{};
cvar_t* g_teamplay_auto_balance = &g_teamplay_auto_balance_storage;
static cvar_t deathmatch_storage{};
cvar_t* deathmatch = &deathmatch_storage;
static cvar_t captureLimit_storage{};
cvar_t* captureLimit = &captureLimit_storage;
static cvar_t fragLimit_storage{};
cvar_t* fragLimit = &fragLimit_storage;
static cvar_t roundLimit_storage{};
cvar_t* roundLimit = &roundLimit_storage;
static cvar_t timeLimit_storage{};
cvar_t* timeLimit = &timeLimit_storage;
static cvar_t mercyLimit_storage{};
cvar_t* mercyLimit = &mercyLimit_storage;
static cvar_t noPlayersTime_storage{};
cvar_t* noPlayersTime = &noPlayersTime_storage;
static cvar_t match_startNoHumans_storage{};
cvar_t* match_startNoHumans = &match_startNoHumans_storage;
static cvar_t match_doOvertime_storage{};
cvar_t* match_doOvertime = &match_doOvertime_storage;
static cvar_t hostname_storage{};
cvar_t* hostname = &hostname_storage;
static cvar_t skill_storage{};
cvar_t* skill = &skill_storage;
static cvar_t password_storage{};
cvar_t* password = &password_storage;
static cvar_t spectatorPassword_storage{};
cvar_t* spectatorPassword = &spectatorPassword_storage;
static cvar_t admin_password_storage{};
cvar_t* admin_password = &admin_password_storage;
static cvar_t needPass_storage{};
cvar_t* needPass = &needPass_storage;
static cvar_t filterBan_storage{};
cvar_t* filterBan = &filterBan_storage;
static cvar_t marathon_storage{};
cvar_t* marathon = &marathon_storage;
static cvar_t g_marathon_timelimit_storage{};
cvar_t* g_marathon_timelimit = &g_marathon_timelimit_storage;
static cvar_t g_marathon_scorelimit_storage{};
cvar_t* g_marathon_scorelimit = &g_marathon_scorelimit_storage;
static cvar_t g_allowSpecVote_storage{};
cvar_t* g_allowSpecVote = &g_allowSpecVote_storage;
static cvar_t g_teamAutoLock_storage{};
cvar_t* g_teamAutoLock = &g_teamAutoLock_storage;
static cvar_t g_teamMaxPlayers_storage{};
cvar_t* g_teamMaxPlayers = &g_teamMaxPlayers_storage;
static cvar_t g_teamForceJoin_storage{};
cvar_t* g_teamForceJoin = &g_teamForceJoin_storage;
static cvar_t g_teamplay_item_drop_notice_storage{};
cvar_t* g_teamplay_item_drop_notice = &g_teamplay_item_drop_notice_storage;
static cvar_t g_teamplay_item_drop_alert_storage{};
cvar_t* g_teamplay_item_drop_alert = &g_teamplay_item_drop_alert_storage;
static cvar_t g_allowPogo_storage{};
cvar_t* g_allowPogo = &g_allowPogo_storage;
static cvar_t g_allowGrapple_storage{};
cvar_t* g_allowGrapple = &g_allowGrapple_storage;
static cvar_t g_loadout_storage{};
cvar_t* g_loadout = &g_loadout_storage;
static cvar_t g_respawn_storage{};
cvar_t* g_respawn = &g_respawn_storage;
static cvar_t g_headshot_storage{};
cvar_t* g_headshot = &g_headshot_storage;
static cvar_t g_falling_damage_storage{};
cvar_t* g_falling_damage = &g_falling_damage_storage;
static cvar_t g_infinite_ammo_storage{};
cvar_t* g_infinite_ammo = &g_infinite_ammo_storage;
static cvar_t g_match_extended_warmup_storage{};
cvar_t* g_match_extended_warmup = &g_match_extended_warmup_storage;
static cvar_t g_teamForceBalance_storage{};
cvar_t* g_teamForceBalance = &g_teamForceBalance_storage;
static cvar_t g_teamAutobalanceImmunity_storage{};
cvar_t* g_teamAutobalanceImmunity = &g_teamAutobalanceImmunity_storage;
static cvar_t g_teamMaxImbalance_storage{};
cvar_t* g_teamMaxImbalance = &g_teamMaxImbalance_storage;
static cvar_t g_teamplay_item_drop_lockout_storage{};
cvar_t* g_teamplay_item_drop_lockout = &g_teamplay_item_drop_lockout_storage;
static cvar_t g_voteFlags_storage{};
cvar_t* g_voteFlags = &g_voteFlags_storage;
static cvar_t g_teamplay_switch_delay_storage{};
cvar_t* g_teamplay_switch_delay = &g_teamplay_switch_delay_storage;

/*
=============
main

Verify that CollectStackedTeamClients handles a 32-player stacked team without overwriting the buffer.
=============
*/
int main() {
	std::vector<gclient_t> clientStorage(MAX_CLIENTS_KEX);
	std::vector<gentity_t> entityStorage(MAX_CLIENTS_KEX + 1);

	game.clients = clientStorage.data();
	game.maxClients = MAX_CLIENTS_KEX;
	game.maxEntities = static_cast<uint32_t>(entityStorage.size());
	globals.numEntities = game.maxClients + 1;
	g_entities = entityStorage.data();

	constexpr int stackedPlayers = 32;

	for (int i = 0; i < stackedPlayers; ++i) {
		gentity_t& ent = g_entities[i + 1];
		ent.inUse = true;
		ent.client = &game.clients[i];
		ent.client->pers.connected = true;
		ent.client->sess.team = Team::Red;
	}

	std::array<int, MAX_CLIENTS_KEX> index{};
	index.fill(-1);

	size_t count = CollectStackedTeamClients(Team::Red, index);

	assert(count == static_cast<size_t>(stackedPlayers));
	for (int i = 0; i < stackedPlayers; ++i) {
		assert(index[i] == i);
	}
	for (size_t i = stackedPlayers; i < index.size(); ++i) {
		assert(index[i] == -1);
	}

	return 0;
}
