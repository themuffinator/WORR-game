/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_apply_spawn_flags.cpp implementation.*/

#include <cassert>
#include <string>

#include "server/client/client_session_service_impl.hpp"
#include "server/client/client_stats_service.hpp"
#include "server/gameplay/client_config.hpp"
#include "server/g_local.hpp"
#include "client_session_service_impl_stubs.hpp"

local_game_import_t gi{};
game_export_t globals{};
gentity_t* g_entities = nullptr;

spawn_temp_t st{};

namespace worr::server::client {

class StubClientStatsService : public ClientStatsService {
public:
	/*
	=============
	StubClientStatsService::PersistMatchResults

	No-op for testing.
	=============
	*/
	void PersistMatchResults(const MatchStatsContext&) override {}

	/*
	=============
	StubClientStatsService::SaveStatsForDisconnect

	No-op for testing.
	=============
	*/
	void SaveStatsForDisconnect(const MatchStatsContext&, gentity_t*) override {}
};

} // namespace worr::server::client

/*
=============
main

Verifies that ApplySpawnFlags preserves existing flags, allows coexistence of
bot/human restrictions, and leaves unrelated bits untouched.
=============
*/
int main() {
	local_game_import_t gi{};
	GameLocals game{};
	LevelLocals level{};
	ClientConfigStore configStore(gi, "");
	worr::server::client::StubClientStatsService statsService{};
	worr::server::client::ClientSessionServiceImpl service(gi, game, level, configStore, statsService);

	gentity_t ent{};
	ent.flags = FL_FLASHLIGHT | FL_NO_BOTS;
	ent.arena = 7;

	st = {};
	service.ApplySpawnFlags(&ent);
	assert(ent.flags & FL_NO_BOTS);
	assert(ent.flags & FL_FLASHLIGHT);
	assert((ent.flags & FL_NO_HUMANS) == 0);
	assert(ent.arena == 0);

	st = {};
	st.keys_specified.insert("noBots");
	st.keys_specified.insert("noHumans");
	st.noBots = true;
	st.noHumans = true;
	st.keys_specified.insert("arena");
	st.arena = 3;
	service.ApplySpawnFlags(&ent);
	assert(ent.flags & FL_NO_BOTS);
	assert(ent.flags & FL_NO_HUMANS);
	assert(ent.flags & FL_FLASHLIGHT);
	assert(ent.arena == 3);

	st = {};
	st.keys_specified.insert("noBots");
	st.noBots = false;
	service.ApplySpawnFlags(&ent);
	assert(!(ent.flags & FL_NO_BOTS));
	assert(ent.flags & FL_NO_HUMANS);
	assert(ent.flags & FL_FLASHLIGHT);
	assert(ent.arena == 0);

	return 0;
}
