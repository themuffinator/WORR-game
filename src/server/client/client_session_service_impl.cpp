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

/*
=============
LegacyClientConfigStore::Initialize

Bridges the service's configuration initialization call to the existing
ClientConfig_Init helper.
=============
*/
void LegacyClientConfigStore::Initialize(game_import_t& gi, gclient_t* client, const std::string& playerID,
		const std::string& playerName, const std::string& gameType) {
		(void)gi;
		ClientConfig_Init(client, playerID, playerName, gameType);
}

/*
=============
LegacyClientConfigStore::SaveStats

Persists the player's stats using the legacy ClientConfig_SaveStats function.
=============
*/
void LegacyClientConfigStore::SaveStats(game_import_t& gi, gclient_t* client, bool wonMatch) {
		(void)gi;
		ClientConfig_SaveStats(client, wonMatch);
}

/*
=============
LegacyClientConfigStore::SaveStatsForGhost

Persists ghost player stats by deferring to ClientConfig_SaveStatsForGhost.
=============
*/
void LegacyClientConfigStore::SaveStatsForGhost(game_import_t& gi, const Ghosts& ghost, bool wonMatch) {
		(void)gi;
		ClientConfig_SaveStatsForGhost(ghost, wonMatch);
}

/*
=============
LegacyClientConfigStore::SaveWeaponPreferences

Writes the player's weapon preferences via ClientConfig_SaveWeaponPreferences.
=============
*/
void LegacyClientConfigStore::SaveWeaponPreferences(game_import_t& gi, gclient_t* client) {
		(void)gi;
		ClientConfig_SaveWeaponPreferences(client);
}

/*
=============
LegacyClientConfigStore::DefaultSkillRating

Retrieves the default skill rating through the ClientConfig_DefaultSkillRating helper.
=============
*/
int LegacyClientConfigStore::DefaultSkillRating(game_import_t& gi) const {
		(void)gi;
		return ClientConfig_DefaultSkillRating();
}

/*
=============
LegacyClientConfigStore::PlayerNameForSocialID

Resolves a player's name by delegating to GetPlayerNameForSocialID.
=============
*/
std::string LegacyClientConfigStore::PlayerNameForSocialID(game_import_t& gi, const std::string& socialID) {
		(void)gi;
		return GetPlayerNameForSocialID(socialID);
}

/*
=============
ClientSessionServiceImpl::OnReadyToggled

Manages the ready-state toggle workflow, including precondition checks,
messaging, and broadcasting.
=============
*/
ReadyResult ClientSessionServiceImpl::OnReadyToggled(gentity_t* ent, bool state, bool toggle) {
		if (!ReadyConditions(ent, false)) {
			return ReadyResult::NoConditions;
		}

		client_persistant_t* pers = &ent->client->pers;

		if (toggle) {
			pers->readyStatus = !pers->readyStatus;
		}
		else {
			if (pers->readyStatus == state) {
				gi_.LocClient_Print(ent, PRINT_HIGH, "You are already {}ready.\n", state ? "" : "NOT " );
				return ReadyResult::AlreadySet;
			}

			pers->readyStatus = state;
		}

		gi_.LocBroadcast_Print(PRINT_CENTER,
			"%bind:+wheel2:Use Compass to toggle your ready status.%.MATCH IS IN WARMUP\n{} is {}ready.",
			ent->client->sess.netName, ent->client->pers.readyStatus ? "" : "NOT " );

		return ReadyResult::Success;
}

/*
=============
GetClientSessionService

Provides access to the translation-unit singleton used to service client
session requests.
=============
*/
ClientSessionServiceImpl& GetClientSessionService() {
		static LegacyClientConfigStore configStore;
		static ClientSessionServiceImpl service(gi, game, level, configStore);
		return service;
}

} // namespace worr::server::client
