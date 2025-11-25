/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_client_userinfo_fov_default.cpp implementation.*/

#include "server/client/client_session_service_impl.hpp"
#include "server/client/client_stats_service.hpp"
#include "server/gameplay/client_config.hpp"
#include "server/g_local.hpp"
#include "client_session_service_impl_stubs.hpp"

#include <array>
#include <cassert>
#include <cstring>
#include <string>

cvar_t* g_allowCustomSkins = nullptr;
gentity_t* g_entities = nullptr;
local_game_import_t gi{};
game_export_t globals{};

namespace {

class StubClientStatsService : public worr::server::client::ClientStatsService {
	public:
	/*
	=============
	StubClientStatsService::PersistMatchResults

	No-op for testing.
	=============
	*/
	void PersistMatchResults(const worr::server::client::MatchStatsContext&) override {}

	/*
	=============
	StubClientStatsService::SaveStatsForDisconnect

	No-op for testing.
	=============
	*/
	void SaveStatsForDisconnect(const worr::server::client::MatchStatsContext&, gentity_t*) override {}
};

/*
=============
StubInfoValueForKey

Provides deterministic userinfo fields for test coverage.
=============
*/
static size_t StubInfoValueForKey(const char* /*unused*/, const char* key, char* buffer, size_t buffer_len)
{
	if (std::strcmp(key, "name") == 0) {
		Q_strlcpy(buffer, "FovTester", buffer_len);
		return std::strlen(buffer);
	}

	if (std::strcmp(key, "skin") == 0) {
		Q_strlcpy(buffer, "male/grunt", buffer_len);
		return std::strlen(buffer);
	}

	return 0;
}

/*
=============
StubConfigString

Discards configstring writes during testing.
=============
*/
static void StubConfigString(int, const char*) {}

/*
=============
StubImageIndex

Avoids touching the renderer while testing.
=============
*/
static int StubImageIndex(const char*)
{
	return 0;
}

} // namespace

/*
=============
main

Ensures missing userinfo fov values leave the existing default unchanged.
=============
*/
int main()
{
	local_game_import_t gi{};
	gi.Info_ValueForKey = StubInfoValueForKey;
	gi.configString = StubConfigString;
	gi.imageIndex = StubImageIndex;

	GameLocals game{};
	LevelLocals level{};
	worr::server::client::ClientConfigStore configStore(gi, "");
	StubClientStatsService statsService{};
	worr::server::client::ClientSessionServiceImpl service(gi, game, level, configStore, statsService);

	std::array<gentity_t, 2> entityStorage{};
	g_entities = entityStorage.data();

	gclient_t client{};
	client.ps.fov = 110.f;
	entityStorage[1].client = &client;

	cvar_t allowCustomSkins{};
	allowCustomSkins.integer = 1;
	g_allowCustomSkins = &allowCustomSkins;

	service.ClientUserinfoChanged(gi, game, level, &entityStorage[1], "\\name\\FovTester\\skin\\male/grunt");
	assert(client.ps.fov == 110.f);

	return 0;
}
