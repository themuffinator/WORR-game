#pragma once

#include <string>

struct game_import_t;
struct gclient_t;
struct Ghosts;

namespace worr::server::client {

/*
=================
ClientConfigStore

Interface abstraction for the persistence logic implemented in
src/server/gameplay/g_client_cfg.cpp. It exposes the operations required to
initialize, read, and persist a player's configuration and rating data.
=================
*/
class ClientConfigStore {
	public:
	virtual ~ClientConfigStore() = default;

	virtual void Initialize(game_import_t& gi, gclient_t* client, const std::string& playerID,
		const std::string& playerName, const std::string& gameType) = 0;
	virtual void SaveStats(game_import_t& gi, gclient_t* client, bool wonMatch) = 0;
	virtual void SaveStatsForGhost(game_import_t& gi, const Ghosts& ghost, bool wonMatch) = 0;
	virtual void SaveWeaponPreferences(game_import_t& gi, gclient_t* client) = 0;
	virtual int DefaultSkillRating(game_import_t& gi) const = 0;
	virtual std::string PlayerNameForSocialID(game_import_t& gi, const std::string& socialID) = 0;
};

} // namespace worr::server::client
