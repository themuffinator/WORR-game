#pragma once

#include "client_session_results.hpp"
#include "client_session_service.hpp"

namespace worr::server::client {

class ClientSessionServiceImpl : public ClientSessionService {
public:
ClientSessionServiceImpl(game_import_t& gi, GameLocals& game, LevelLocals& level);

	bool ClientConnect(game_import_t& gi, GameLocals& game, LevelLocals& level,
		gentity_t* ent, char* userInfo, const char* socialID, bool isBot) override;
	void ClientBegin(game_import_t& gi, GameLocals& game, LevelLocals& level,
		gentity_t* ent) override;
	void ClientUserinfoChanged(game_import_t& gi, GameLocals& game, LevelLocals& level,
		gentity_t* ent, const char* userInfo) override;
	DisconnectResult ClientDisconnect(game_import_t& gi, GameLocals& game, LevelLocals& level,
		gentity_t* ent) override;
	void ClientThink(game_import_t& gi, GameLocals& game, LevelLocals& level,
		gentity_t* ent, usercmd_t* cmd) override;
	void ClientBeginServerFrame(game_import_t& gi, GameLocals& game, LevelLocals& level,
		gentity_t* ent) override;
	ReadyResult OnReadyToggled(gentity_t* ent, bool state, bool toggle);
	void ApplySpawnFlags(gentity_t* ent) const;
	void PrepareSpawnPoint(gentity_t* ent, bool allowElevatorDrop = false,
		void (*dropThink)(gentity_t*) = nullptr) const;

	private:
	game_import_t& gi_;
	GameLocals& game_;
	LevelLocals& level_;
void OnDisconnect(gentity_t* ent);
};

void InitializeClientSessionService(game_import_t& gi, GameLocals& game, LevelLocals& level);
ClientSessionServiceImpl& GetClientSessionService();

} // namespace worr::server::client
