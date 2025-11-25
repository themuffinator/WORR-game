/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_client_connect_null_userinfo.cpp implementation.*/

#include "server/client/client_session_service_impl.hpp"
#include "server/gameplay/client_config.hpp"
#include "server/g_local.hpp"
#include "client_session_service_impl_stubs.hpp"

#include <array>
#include <cassert>
#include <cstring>
#include <string>

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
game_export_t globals{};
gentity_t* g_entities = nullptr;
spawn_temp_t st{};
std::mt19937 mt_rand{};

static cvar_t deathmatch_storage{};
cvar_t* deathmatch = &deathmatch_storage;
static cvar_t bot_name_prefix_storage{};
cvar_t* bot_name_prefix = &bot_name_prefix_storage;
static cvar_t g_allowCustomSkins_storage{};
cvar_t* g_allowCustomSkins = &g_allowCustomSkins_storage;
static cvar_t g_gametype_storage{};
cvar_t* g_gametype = &g_gametype_storage;
static cvar_t teamplay_storage{};
cvar_t* teamplay = &teamplay_storage;
static cvar_t ctf_storage{};
cvar_t* ctf = &ctf_storage;
static cvar_t coop_storage{};
cvar_t* coop = &coop_storage;
static cvar_t skill_storage{};
cvar_t* skill = &skill_storage;

/*
=============
StubComPrint

Silences formatted log output during testing.
=============
*/
static void StubComPrint(const char* /*msg*/) {}

/*
=============
StubComError

Prevents error calls from aborting the test run.
=============
*/
static void StubComError(const char* /*msg*/) {}

/*
=============
StubLocPrint

No-op localization print handler for client/broadcast messages.
=============
*/
static void StubLocPrint(gentity_t*, print_type_t, const char*, const char**, size_t) {}

/*
=============
StubInfoValueForKey

Returns an empty string for requested info keys.
=============
*/
static size_t StubInfoValueForKey(const char*, const char*, char* buffer, size_t buffer_len)
{
	if (buffer_len > 0)
		buffer[0] = '\0';
	return 0;
}

/*
=============
StubInfoSetValueForKey

Accepts updates to info strings without mutation.
=============
*/
static bool StubInfoSetValueForKey(char*, const char*, const char*)
{
	return true;
}

/*
=============
StubInfoRemoveKey

Accepts removal requests without mutation.
=============
*/
static bool StubInfoRemoveKey(char*, const char*)
{
	return true;
}

/*
=============
StubAddCommandString

Ignores server command enqueue requests.
=============
*/
static void StubAddCommandString(const char*) {}

/*
=============
StubImageIndex

Avoids touching the renderer while resolving images.
=============
*/
static int StubImageIndex(const char*)
{
	return 0;
}

/*
=============
StubConfigString

Suppresses configstring updates for the test harness.
=============
*/
static void StubConfigString(int, const char*) {}

/*
=============
main

Verifies that ClientConnect safely handles a null userInfo pointer.
=============
*/
int main()
{
	gi.Com_Print = &StubComPrint;
	gi.Com_Error = &StubComError;
	gi.Loc_Print = &StubLocPrint;
	gi.Info_ValueForKey = &StubInfoValueForKey;
	gi.Info_SetValueForKey = &StubInfoSetValueForKey;
	gi.Info_RemoveKey = &StubInfoRemoveKey;
	gi.AddCommandString = &StubAddCommandString;
	gi.imageIndex = &StubImageIndex;
	gi.configString = &StubConfigString;

	deathmatch_storage.integer = 1;
	g_allowCustomSkins_storage.integer = 1;

	std::array<gentity_t, 2> entities{};
	std::array<gclient_t, 1> clients{};

	g_entities = entities.data();
	globals.numEntities = static_cast<int32_t>(entities.size());
	game.maxClients = static_cast<int32_t>(clients.size());
	game.clients = clients.data();

	g_entities[1].client = &clients[0];

ClientConfigStore configStore(gi, ".");

	class StubClientStatsService final : public worr::server::client::ClientStatsService {
		public:
		/*
		=============
		PersistMatchResults

		No-op implementation for test coverage.
		=============
		*/
		void PersistMatchResults(const worr::server::client::MatchStatsContext&) override {}

		/*
		=============
		SaveStatsForDisconnect

		No-op implementation for test coverage.
		=============
		*/
		void SaveStatsForDisconnect(const worr::server::client::MatchStatsContext&, gentity_t*) override {}
	} statsService;

	worr::server::client::ClientSessionServiceImpl service(gi, game, level, configStore, statsService);

	assert(service.ClientConnect(gi, game, level, &entities[1], nullptr, "", true));
	assert(std::strcmp(clients[0].sess.netName, "badinfo") == 0);

	return 0;
}
