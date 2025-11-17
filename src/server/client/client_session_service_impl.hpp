#pragma once

#include "client_config_store.hpp"
#include "client_session_results.hpp"
#include "client_session_service.hpp"

namespace worr::server::client {

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
ClientConfigStore& configStore_;
void OnDisconnect(gentity_t* ent);
};

class LegacyClientConfigStore final : public ClientConfigStore {
public:
void Initialize(game_import_t& gi, gclient_t* client, const std::string& playerID,
const std::string& playerName, const std::string& gameType) override;
void SaveStats(game_import_t& gi, gclient_t* client, bool wonMatch) override;
void SaveStatsForGhost(game_import_t& gi, const Ghosts& ghost, bool wonMatch) override;
void SaveWeaponPreferences(game_import_t& gi, gclient_t* client) override;
int DefaultSkillRating(game_import_t& gi) const override;
std::string PlayerNameForSocialID(game_import_t& gi, const std::string& socialID) override;
};

ClientSessionServiceImpl& GetClientSessionService();

} // namespace worr::server::client
