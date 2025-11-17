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

void ClientBegin(gentity_t* ent);
void ClientUserinfoChanged(gentity_t* ent, const char* userInfo);
void ClientThink(gentity_t* ent, usercmd_t* cmd);
void ClientBeginServerFrame(gentity_t* ent);

namespace {

/*
=============
CheckBanned

Determines whether the connecting player should be rejected based on a
hard-coded ban list. When tripped, the function plays local feedback and
requests that the server kick the player immediately.
=============
*/
static bool CheckBanned(game_import_t& gi, LevelLocals& level, gentity_t* ent, char* userInfo,
		const char* socialID) {
	if (!socialID || !*socialID)
		return false;

	// currently all bans are in Steamworks and Epic, don't bother if not from there
	if (socialID[0] != 'S' && socialID[0] != 'E')
		return false;

	// Israel
	if (!Q_strcasecmp(socialID, "Steamworks-76561198026297488")) {
		gi.Info_SetValueForKey(userInfo, "rejmsg", "Antisemite detected!\n");

		if (host && host->client) {
			if (level.time > host->client->lastBannedMessageTime + 10_sec) {
				char name[MAX_INFO_VALUE] = { 0 };
				gi.Info_ValueForKey(userInfo, "name", name, sizeof(name));

				gi.LocClient_Print(host, PRINT_TTS, "ANTISEMITE DETECTED ({})!\n", name);
				host->client->lastBannedMessageTime = level.time;
				gi.LocBroadcast_Print(PRINT_CHAT, "{}: God Bless Palestine\n", name);
			}
		}

		gi.localSound(ent, CHAN_AUTO, gi.soundIndex("world/klaxon3.wav"), 1, ATTN_NONE, 0);
		gi.AddCommandString(G_Fmt("kick {}\n", ent - g_entities - 1).data());
		return true;
	}

	// Kirlomax
	if (!Q_strcasecmp(socialID, "Steamworks-76561198001774610")) {
		gi.Info_SetValueForKey(userInfo, "rejmsg", "WARNING! KNOWN CHEATER DETECTED\n");

		if (host && host->client) {
			if (level.time > host->client->lastBannedMessageTime + 10_sec) {
				char name[MAX_INFO_VALUE] = { 0 };
				gi.Info_ValueForKey(userInfo, "name", name, sizeof(name));

				gi.LocClient_Print(host, PRINT_TTS, "WARNING! KNOWN CHEATER DETECTED ({})!\n", name);
				host->client->lastBannedMessageTime = level.time;
				gi.LocBroadcast_Print(PRINT_CHAT, "{}: I am a known cheater, banned from all servers.\n", name);
			}
		}

		gi.localSound(ent, CHAN_AUTO, gi.soundIndex("world/klaxon3.wav"), 1, ATTN_NONE, 0);
		gi.AddCommandString(G_Fmt("kick {}\n", ent - g_entities - 1).data());
		return true;
	}

	// Model192
	if (!Q_strcasecmp(socialID, "Steamworks-76561197972296343")) {
		gi.Info_SetValueForKey(userInfo, "rejmsg", "WARNING! MOANERTONE DETECTED\n");

		if (host && host->client) {
			if (level.time > host->client->lastBannedMessageTime + 10_sec) {
				char name[MAX_INFO_VALUE] = { 0 };
				gi.Info_ValueForKey(userInfo, "name", name, sizeof(name));

				gi.LocClient_Print(host, PRINT_TTS, "WARNING! MOANERTONE DETECTED ({})!\n", name);
				host->client->lastBannedMessageTime = level.time;
				gi.LocBroadcast_Print(PRINT_CHAT, "{}: Listen up, I have something to moan about.\n", name);
			}
		}

		gi.localSound(ent, CHAN_AUTO, gi.soundIndex("world/klaxon3.wav"), 1, ATTN_NONE, 0);
		gi.AddCommandString(G_Fmt("kick {}\n", ent - g_entities - 1).data());
		return true;
	}

	// Dalude
	if (!Q_strcasecmp(socialID, "Steamworks-76561199001991246") ||
		!Q_strcasecmp(socialID, "EOS-07e230c273be4248bbf26c89033923c1")) {
		ent->client->sess.is_888 = true;
		gi.Info_SetValueForKey(userInfo, "rejmsg", "Fake 888 Agent detected!\n");
		gi.Info_SetValueForKey(userInfo, "name", "Fake 888 Agent");

		if (host && host->client) {
			if (level.time > host->client->lastBannedMessageTime + 10_sec) {
				char name[MAX_INFO_VALUE] = { 0 };
				gi.Info_ValueForKey(userInfo, "name", name, sizeof(name));

				gi.LocClient_Print(host, PRINT_TTS, "FAKE 888 AGENT DETECTED ({})!\n", name);
				host->client->lastBannedMessageTime = level.time;
				gi.LocBroadcast_Print(PRINT_CHAT, "{}: bejesus, what a lovely lobby! certainly better than 888's!\n", name);
			}
		}
		gi.localSound(ent, CHAN_AUTO, gi.soundIndex("world/klaxon3.wav"), 1, ATTN_NONE, 0);
		gi.AddCommandString(G_Fmt("kick {}\n", ent - g_entities - 1).data());
		return true;
	}

	return false;
}

/*
================
ClientCheckPermissions

Updates the client's admin/banned flags based on the configured social ID
lists.
================
*/
static void ClientCheckPermissions(GameLocals& game, gentity_t* ent, const char* socialID) {
	if (!socialID || !*socialID)
		return;

	std::string id(socialID);

	ent->client->sess.banned = game.bannedIDs.contains(id);
	ent->client->sess.admin = game.adminIDs.contains(id);
}

} // namespace

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

Implements the legacy ClientConnect logic behind the service seam so future
callers can transition away from the procedural entry point.
=============
*/
bool ClientSessionServiceImpl::ClientConnect(game_import_t&, GameLocals&, LevelLocals&,
	gentity_t* ent, char* userInfo, const char* socialID, bool isBot) {
	game_import_t& gi = gi_;
	GameLocals& game = game_;
	LevelLocals& level = level_;
	const char* safeSocialID = (socialID && *socialID) ? socialID : "";

	if (!isBot) {
		if (CheckBanned(gi, level, ent, userInfo, safeSocialID))
			return false;

		ClientCheckPermissions(game, ent, safeSocialID);
	}

	ent->client->sess.team = deathmatch->integer ? Team::None : Team::Free;

	// they can connect
	ent->client = game.clients + (ent - g_entities - 1);

	// set up userInfo early
	ClientUserinfoChanged(ent, userInfo);

	// if there is already a body waiting for us (a loadgame), just
	// take it, otherwise spawn one from scratch
	if (ent->inUse == false) {
		// clear the respawning variables

		if (!ent->client->sess.initialised && ent->client->sess.team == Team::None) {
			ent->client->pers.introTime = 3_sec;

			// force team join
			ent->client->sess.team = deathmatch->integer ? Team::None : Team::Free;
			ent->client->sess.pc = {};

			InitClientResp(ent->client);

			ent->client->sess.playStartRealTime = GetCurrentRealTimeMillis();
		}

		if (!game.autoSaved || !ent->client->pers.weapon)
			InitClientPersistant(ent, ent->client);
	}

	// make sure we start with known default(s)
	ent->svFlags = SVF_PLAYER;

	if (isBot) {
		ent->svFlags |= SVF_BOT;
		ent->client->sess.is_a_bot = true;

		if (bot_name_prefix->string[0] && *bot_name_prefix->string) {
			std::array<char, MAX_NETNAME> oldName = {};
			std::array<char, MAX_NETNAME> newName = {};

			gi.Info_ValueForKey(userInfo, "name", oldName.data(), oldName.size());
			Q_strlcpy(newName.data(), bot_name_prefix->string, newName.size());
			Q_strlcat(newName.data(), oldName.data(), newName.size());
			gi.Info_SetValueForKey(userInfo, "name", newName.data());
		}
	}

	Q_strlcpy(ent->client->sess.socialID, safeSocialID, sizeof(ent->client->sess.socialID));

	std::array<char, MAX_INFO_VALUE> value = {};
	// [Paril-KEX] fetch name because now netName is kinda unsuitable
	gi.Info_ValueForKey(userInfo, "name", value.data(), value.size());
	Q_strlcpy(ent->client->sess.netName, value.data(), sizeof(ent->client->sess.netName));

	ent->client->sess.skillRating = 0;
	ent->client->sess.skillRatingChange = 0;

	if (!isBot) {
		if (ent->client->sess.socialID[0]) {
			ClientConfig_Init(ent->client, ent->client->sess.socialID, value.data(),
				Game::GetCurrentInfo().short_name_upper.data());
			PCfg_ClientInitPConfig(ent);
		}
		else {
			ent->client->sess.skillRating = ClientConfig_DefaultSkillRating();
		}

		if (ent->client->sess.banned) {
			gi.LocBroadcast_Print(PRINT_HIGH, "BANNED PLAYER {} connects.\n", value.data());
			gi.AddCommandString(G_Fmt("kick {}\n", ent - g_entities - 1).data());
			return false;
		}

		if (ent->client->sess.skillRating > 0) {
			gi.LocBroadcast_Print(PRINT_HIGH, "{} connects. (SR: {})\n", value.data(),
				ent->client->sess.skillRating);
		}
		else {
			gi.LocBroadcast_Print(PRINT_HIGH, "$g_player_connected", value.data());
		}

		// entity 1 is always server host, so make admin
		if (ent == &g_entities[1])
			ent->client->sess.admin = true;

		// Detect if client is on a console system
		if (*safeSocialID && (
				_strnicmp(safeSocialID, "PSN", 3) == 0 ||
				_strnicmp(safeSocialID, "NX", 2) == 0 ||
				_strnicmp(safeSocialID, "GDK", 3) == 0
			)) {
			ent->client->sess.consolePlayer = true;
		}
		else {
			ent->client->sess.consolePlayer = false;
		}
	}

	Client_RebuildWeaponPreferenceOrder(*ent->client);

	if (level.endmatch_grace)
		level.endmatch_grace = 0_ms;

	// set skin
	std::array<char, MAX_INFO_VALUE> val = {};
	if (!gi.Info_ValueForKey(userInfo, "skin", val.data(), sizeof(val)))
		Q_strlcpy(val.data(), "male/grunt", val.size());
	const char* sanitizedSkin = ClientSkinOverride(val.data());
	if (Q_strcasecmp(ent->client->sess.skinName.c_str(), sanitizedSkin)) {
		ent->client->sess.skinName = sanitizedSkin;
		ent->client->sess.skinIconIndex = gi.imageIndex(G_Fmt("/players/{}_i",
			ent->client->sess.skinName).data());
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	ent->client->pers.connected = true;

	ent->client->sess.inGame = true;

	// [Paril-KEX] force a state update
	ent->sv.init = false;

	return true;
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

Handles the disconnect workflow previously implemented procedurally, ensuring
the player's state is torn down and other systems are notified appropriately
while reporting status via DisconnectResult.
=============
*/
DisconnectResult ClientSessionServiceImpl::ClientDisconnect(game_import_t& gi, GameLocals& game, LevelLocals& level, gentity_t* ent) {
		(void)gi;
		(void)game;
		(void)level;

		if (!ent || !ent->client) {
			return DisconnectResult::InvalidEntity;
		}

		gclient_t* cl = ent->client;
		const int64_t now = GetCurrentRealTimeMillis();
		cl->sess.playEndRealTime = now;
		P_AccumulateMatchPlayTime(cl, now);

		OnDisconnect(ent);

		if (cl->trackerPainTime) {
			RemoveAttackingPainDaemons(ent);
		}

		if (cl->ownedSphere) {
			if (cl->ownedSphere->inUse) {
				FreeEntity(cl->ownedSphere);
			}
			cl->ownedSphere = nullptr;
		}

		PlayerTrail_Destroy(ent);

		ProBall::HandleCarrierDisconnect(ent);
		Harvester_HandlePlayerDisconnect(ent);

		HeadHunters::DropHeads(ent, nullptr);
		HeadHunters::ResetPlayerState(cl);

		if (!(ent->svFlags & SVF_NOCLIENT)) {
			TossClientItems(ent);

			gi_.WriteByte(svc_muzzleflash);
			gi_.WriteEntity(ent);
			gi_.WriteByte(MZ_LOGOUT);
			gi_.multicast(ent->s.origin, MULTICAST_PVS, false);
		}

		if (cl->pers.connected && cl->sess.initialised && !cl->sess.is_a_bot) {
			if (cl->sess.netName[0]) {
				gi_.LocBroadcast_Print(PRINT_HIGH, "{} disconnected.", cl->sess.netName);
			}
		}

		FreeClientFollowers(ent);

		G_RevertVote(cl);

		P_SaveGhostSlot(ent);

		gi_.unlinkEntity(ent);
		ent->s.modelIndex = 0;
		ent->solid = SOLID_NOT;
		ent->inUse = false;
		ent->sv.init = false;
		ent->className = "disconnected";
		cl->pers.connected = false;
		cl->sess.matchWins = 0;
		cl->sess.matchLosses = 0;
		cl->pers.limitedLivesPersist = false;
		cl->pers.limitedLivesStash = 0;
		const bool wasSpawned = cl->pers.spawned;
		cl->pers.spawned = false;
		ent->timeStamp = level_.time + 1_sec;

		if (wasSpawned) {
			ClientConfig_SaveStats(cl, false);
		}

		if (deathmatch->integer) {
			CalculateRanks();

			for (auto ec : active_clients()) {
				if (ec->client->showScores) {
					ec->client->menu.updateTime = level_.time;
				}
			}
		}

		return DisconnectResult::Success;
}


/*
=============
ClientSessionServiceImpl::OnDisconnect

Validates that the player's ready state can be cleared and, when appropriate,
broadcasts the change before the rest of the disconnect teardown executes.
=============
*/
void ClientSessionServiceImpl::OnDisconnect(gentity_t* ent) {
		if (!ent || !ent->client) {
			return;
		}

		gclient_t* cl = ent->client;

		if (!cl->pers.readyStatus) {
			return;
		}

		const bool canUpdateReady = ReadyConditions(ent, false);
		cl->pers.readyStatus = false;

		if (canUpdateReady && cl->sess.netName[0]) {
			gi_.LocBroadcast_Print(PRINT_CENTER,
			"%bind:+wheel2:Use Compass to toggle your ready status.%.MATCH IS IN WARMUP\n{} is NOT ready.",
			cl->sess.netName);
		}
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
