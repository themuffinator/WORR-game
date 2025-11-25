#pragma once

#include "server/client/client_session_service_impl.hpp"
#include "server/client/client_stats_service.hpp"
#include "server/gameplay/client_config.hpp"
#include "server/g_local.hpp"

namespace worr::server::client {

	/*
	=============
	ClientSessionServiceImpl::ClientSessionServiceImpl

	Constructs a lightweight service instance for unit tests without relying on
	the full game runtime.
	=============
	*/
inline ClientSessionServiceImpl::ClientSessionServiceImpl(local_game_import_t& gi, GameLocals& game, LevelLocals& level,
ClientConfigStore& configStore, ClientStatsService& statsService) :
gi_(gi),
game_(game),
level_(level),
configStore_(configStore),
statsService_(statsService) {}

	/*
	=============
	ClientSessionServiceImpl::ClientConnect

	Stubbed connection handler for tests.
	=============
	*/
inline bool ClientSessionServiceImpl::ClientConnect(local_game_import_t&, GameLocals&, LevelLocals&, gentity_t* ent, char* userInfo, const char*, bool isBot) {
if (!ent || !ent->client)
return false;

ent->client->sess.is_a_bot = isBot;
ent->client->sess.consolePlayer = false;
ent->client->sess.admin = false;
ent->client->sess.banned = false;
ent->client->sess.is_888 = false;

if (!userInfo) {
Q_strlcpy(ent->client->sess.netName, "badinfo", sizeof(ent->client->sess.netName));
return true;
}

return true;
}

	/*
	=============
	ClientSessionServiceImpl::ClientBegin

	Stubbed begin handler for tests.
	=============
	*/
	inline void ClientSessionServiceImpl::ClientBegin(local_game_import_t&, GameLocals&, LevelLocals&, gentity_t*) {}

	/*
	=============
	ClientSessionServiceImpl::ClientUserinfoChanged

	Stubbed userinfo handler for tests.
	=============
	*/
	inline void ClientSessionServiceImpl::ClientUserinfoChanged(local_game_import_t&, GameLocals&, LevelLocals&, gentity_t*, const char*) {}

	/*
	=============
	ClientSessionServiceImpl::ClientDisconnect

	Stubbed disconnect handler for tests.
	=============
	*/
	inline DisconnectResult ClientSessionServiceImpl::ClientDisconnect(local_game_import_t&, GameLocals&, LevelLocals&, gentity_t*) {
	return DisconnectResult::Success;
	}

	/*
	=============
	ClientSessionServiceImpl::ClientThink

	Stubbed think handler for tests.
	=============
	*/
	inline void ClientSessionServiceImpl::ClientThink(local_game_import_t&, GameLocals&, LevelLocals&, gentity_t*, usercmd_t*) {}

	/*
	=============
	ClientSessionServiceImpl::ClientBeginServerFrame

	Stubbed frame begin handler for tests.
	=============
	*/
	inline void ClientSessionServiceImpl::ClientBeginServerFrame(local_game_import_t&, GameLocals&, LevelLocals&, gentity_t*) {}

	/*
	=============
	ClientSessionServiceImpl::OnReadyToggled

	Returns a placeholder ready result for tests.
	=============
	*/
inline ReadyResult ClientSessionServiceImpl::OnReadyToggled(gentity_t*, bool, bool) {
return ReadyResult::Success;
}

	/*
	=============
	ClientSessionServiceImpl::ApplySpawnFlags

	Copies the spawn temp flags collected by the map parser onto the entity so
	that bots, humans, and arena assignments are honored consistently.
	=============
	*/
	inline void ClientSessionServiceImpl::ApplySpawnFlags(gentity_t* ent) const {
	if (!ent) {
	return;
	}

	if (st.was_key_specified("noBots")) {
	if (st.noBots) {
	ent->flags |= FL_NO_BOTS;
	}
	else {
	ent->flags &= ~FL_NO_BOTS;
	}
	}
	else {
	ent->flags &= ~FL_NO_BOTS;
	}

	if (st.was_key_specified("noHumans")) {
	if (st.noHumans) {
	ent->flags |= FL_NO_HUMANS;
	}
	else {
	ent->flags &= ~FL_NO_HUMANS;
	}
	}
	else {
	ent->flags &= ~FL_NO_HUMANS;
	}

	if (st.arena) {
	ent->arena = st.arena;
	}
	else if (!st.was_key_specified("arena")) {
	ent->arena = 0;
	}
	}

	/*
	=============
	ClientSessionServiceImpl::PrepareSpawnPoint

	Stubbed spawn point preparation for tests.
	=============
	*/
	inline void ClientSessionServiceImpl::PrepareSpawnPoint(gentity_t*, bool, void (*)(gentity_t*)) const {}

	/*
	=============
	ClientSessionServiceImpl::SetClientBeginServerFrameFreezeHookForTests

	Ignores the hook assignment for tests.
	=============
	*/
	inline void ClientSessionServiceImpl::SetClientBeginServerFrameFreezeHookForTests(ClientBeginServerFrameFreezeHook) {}

	/*
	=============
	ClientSessionServiceImpl::OnDisconnect

	Stubbed disconnect helper for tests.
	=============
	*/
	inline void ClientSessionServiceImpl::OnDisconnect(local_game_import_t&, gentity_t*) {}

	/*
	=============
	InitializeClientSessionService

	No-op initializer for tests.
	=============
	*/
inline void InitializeClientSessionService(local_game_import_t&, GameLocals&, LevelLocals&, ClientConfigStore&, ClientStatsService&) {}
inline void InitializeClientSessionService(local_game_import_t&, GameLocals&, LevelLocals&) {}

	/*
	=============
	GetClientSessionService

	Returns a dummy reference for tests that expect the accessor to exist.
	=============
	*/
inline ClientSessionServiceImpl& GetClientSessionService() {
static local_game_import_t dummyGi{};
static GameLocals dummyGame{};
static LevelLocals dummyLevel{};
static ClientConfigStore dummyStore(dummyGi, "");
class NullStats final : public ClientStatsService {
public:
void PersistMatchResults(const MatchStatsContext&) override {}
void SaveStatsForDisconnect(const MatchStatsContext&, gentity_t*) override {}
};
static NullStats dummyStats{};
static ClientSessionServiceImpl instance(dummyGi, dummyGame, dummyLevel, dummyStore, dummyStats);
return instance;
}

} // namespace worr::server::client

#include "server/gameplay/g_client_cfg.cpp"

