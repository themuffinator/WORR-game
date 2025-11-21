#include <cassert>

#include "server/client/client_session_service_impl.hpp"
#include "server/client/client_stats_service.hpp"
#include "server/gameplay/client_config.hpp"
#include "server/g_local.hpp"

cvar_t* g_gametype;
static bool freezeHookCalled = false;

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

Verifies latched buttons are cleared before early exits during intermission,
freeze-tag handling, and respawn waiting.
=============
*/
int main() {
	local_game_import_t gi{};
	gi.ServerFrame = []() -> uint32_t { return 0U; };
	gi.Com_Error = +[](const char*) {};
	gi.Loc_Print = +[](gentity_t*, print_type_t, const char*, const char**, size_t) {};
	
	GameLocals game{};
	LevelLocals level{};
	ClientConfigStore configStore(gi, "");
	worr::server::client::StubClientStatsService statsService{};
	worr::server::client::ClientSessionServiceImpl service(gi, game, level, configStore, statsService);
	
	gentity_t ent{};
	gclient_t client{};
	ent.client = &client;
	
	client.latchedButtons = BUTTON_ATTACK;
	level.intermission.time = 1_sec;
	service.ClientBeginServerFrame(gi, game, level, &ent);
	assert(client.latchedButtons == BUTTON_NONE);
	
	cvar_t freezeType{};
	freezeType.integer = static_cast<int>(GameType::FreezeTag);
	g_gametype = &freezeType;
	level.intermission.time = 0_ms;
	level.time = 1_ms;
	client.freeze.thawTime = 1_ms;
	client.eliminated = true;
	client.latchedButtons = BUTTON_USE;
	freezeHookCalled = false;
	service.SetClientBeginServerFrameFreezeHookForTests(+[](gentity_t*) -> bool {
	freezeHookCalled = true;
	return true;
	});
	service.ClientBeginServerFrame(gi, game, level, &ent);
	assert(freezeHookCalled);
	assert(client.latchedButtons == BUTTON_NONE);
	service.SetClientBeginServerFrameFreezeHookForTests(nullptr);
	
	client.awaitingRespawn = true;
	client.latchedButtons = BUTTON_ATTACK;
	level.time = 1_ms;
	service.ClientBeginServerFrame(gi, game, level, &ent);
	assert(client.latchedButtons == BUTTON_NONE);
	
	return 0;
}
