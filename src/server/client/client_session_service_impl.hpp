#pragma once

#include "client_config_store.hpp"
#include "client_session_service.hpp"

namespace worr::server::client {

enum class ReadyResult {
	Success,
	NoConditions,
	AlreadySet,
};

class ClientSessionServiceImpl : public ClientSessionService {
	public:
	ClientSessionServiceImpl(game_import_t& gi, GameLocals& game, LevelLocals& level,
		ClientConfigStore& configStore);

	bool ClientConnect(game_import_t& gi, GameLocals& game, LevelLocals& level,
		gentity_t* ent, char* userInfo, const char* socialID, bool isBot) override;
	void ClientBegin(game_import_t& gi, GameLocals& game, LevelLocals& level,
		gentity_t* ent) override;
	void ClientUserinfoChanged(game_import_t& gi, GameLocals& game, LevelLocals& level,
		gentity_t* ent, const char* userInfo) override;
	bool ClientDisconnect(game_import_t& gi, GameLocals& game, LevelLocals& level,
		gentity_t* ent) override;
	void ClientThink(game_import_t& gi, GameLocals& game, LevelLocals& level,
		gentity_t* ent, usercmd_t* cmd) override;
	void ClientBeginServerFrame(game_import_t& gi, GameLocals& game, LevelLocals& level,
		gentity_t* ent) override;
	ReadyResult OnReadyToggled(gentity_t* ent, bool state, bool toggle);

	private:
	game_import_t& gi_;
	GameLocals& game_;
	LevelLocals& level_;
	ClientConfigStore& configStore_;
	void OnDisconnect(gentity_t* ent);
};

ClientSessionServiceImpl& GetClientSessionService();

} // namespace worr::server::client
