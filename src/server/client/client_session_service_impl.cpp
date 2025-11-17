#include "client_session_service_impl.hpp"

#include "../g_local.hpp"
#include "../gameplay/g_proball.hpp"
#include "../gameplay/client_config.hpp"
#include "../commands/commands.hpp"
#include "../gameplay/g_headhunters.hpp"
#include "../monsters/m_player.hpp"
#include "../bots/bot_includes.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

bool ClientConnect(gentity_t* ent, char* userInfo, const char* socialID, bool isBot);
void ClientBegin(gentity_t* ent);
void ClientUserinfoChanged(gentity_t* ent, const char* userInfo);
void ClientDisconnect(gentity_t* ent);
void ClientThink(gentity_t* ent, usercmd_t* cmd);
void ClientBeginServerFrame(gentity_t* ent);

namespace worr::server::client {

/*
=============
ClientSessionServiceImpl::ClientSessionServiceImpl

Stores references to the game state objects that were previously accessed via
globals so the service can eventually operate without that implicit coupling.
=============
*/
ClientSessionServiceImpl::ClientSessionServiceImpl(game_import_t& gi, GameLocals& game, LevelLocals& level,
		ClientConfigStore& configStore)
		: gi_(gi)
		, game_(game)
		, level_(level)
		, configStore_(configStore) {}

/*
=============
ClientSessionServiceImpl::ClientConnect

Entry point that forwards to the existing procedural ClientConnect logic.
=============
*/
bool ClientSessionServiceImpl::ClientConnect(game_import_t& gi, GameLocals& game, LevelLocals& level,
		gentity_t* ent, char* userInfo, const char* socialID, bool isBot) {
		(void)gi;
		(void)game;
		(void)level;
		return ::ClientConnect(ent, userInfo, socialID, isBot);
}

/*
=============
ClientSessionServiceImpl::ClientBegin

Delegates to the legacy ClientBegin implementation until the logic migrates.
=============
*/
void ClientSessionServiceImpl::ClientBegin(game_import_t& gi, GameLocals& game, LevelLocals& level, gentity_t* ent) {
		(void)gi;
		(void)game;
		(void)level;
		::ClientBegin(ent);
}

/*
=============
ClientSessionServiceImpl::ClientUserinfoChanged

Routes userinfo updates to the existing ClientUserinfoChanged handler.
=============
*/
void ClientSessionServiceImpl::ClientUserinfoChanged(game_import_t& gi, GameLocals& game, LevelLocals& level,
		gentity_t* ent, const char* userInfo) {
		(void)gi;
		(void)game;
		(void)level;
		::ClientUserinfoChanged(ent, userInfo);
}

/*
=============
ClientSessionServiceImpl::ClientDisconnect

Forwards disconnect handling to the procedural ClientDisconnect implementation.
=============
*/
void ClientSessionServiceImpl::ClientDisconnect(game_import_t& gi, GameLocals& game, LevelLocals& level, gentity_t* ent) {
		(void)gi;
		(void)game;
		(void)level;
		::ClientDisconnect(ent);
}

/*
=============
ClientSessionServiceImpl::ClientThink

Passes the per-frame ClientThink logic through to the legacy code path.
=============
*/
void ClientSessionServiceImpl::ClientThink(game_import_t& gi, GameLocals& game, LevelLocals& level,
		gentity_t* ent, usercmd_t* cmd) {
		(void)gi;
		(void)game;
		(void)level;
		::ClientThink(ent, cmd);
}

/*
=============
ClientSessionServiceImpl::ClientBeginServerFrame

Defers to the existing ClientBeginServerFrame function until it is migrated.
=============
*/
void ClientSessionServiceImpl::ClientBeginServerFrame(game_import_t& gi, GameLocals& game, LevelLocals& level,
		gentity_t* ent) {
		(void)gi;
		(void)game;
		(void)level;
		::ClientBeginServerFrame(ent);
}

} // namespace worr::server::client
