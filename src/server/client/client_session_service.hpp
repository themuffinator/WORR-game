#pragma once

#include <string>

struct game_import_t;
struct gentity_t;
struct gclient_t;
struct usercmd_t;
class LevelLocals;
struct GameLocals;

namespace worr::server::client {

/*
=================
ClientSessionService

Abstract interface describing the operations that manage a client's session
lifecycle. The eventual implementation will move the responsibilities that
live in src/server/player/p_client.cpp behind this seam.
=================
*/
class ClientSessionService {
	public:
	virtual ~ClientSessionService() = default;

	virtual bool ClientConnect(game_import_t& gi, GameLocals& game, LevelLocals& level,
		gentity_t* ent, char* userInfo, const char* socialID, bool isBot) = 0;
	virtual void ClientBegin(game_import_t& gi, GameLocals& game, LevelLocals& level,
		gentity_t* ent) = 0;
	virtual void ClientUserinfoChanged(game_import_t& gi, GameLocals& game, LevelLocals& level,
		gentity_t* ent, const char* userInfo) = 0;
	virtual bool ClientDisconnect(game_import_t& gi, GameLocals& game, LevelLocals& level,
		gentity_t* ent) = 0;
	virtual void ClientThink(game_import_t& gi, GameLocals& game, LevelLocals& level,
		gentity_t* ent, usercmd_t* cmd) = 0;
	virtual void ClientBeginServerFrame(game_import_t& gi, GameLocals& game, LevelLocals& level,
		gentity_t* ent) = 0;
};

} // namespace worr::server::client
