#include "server/client/client_session_service_impl.hpp"
#include "server/g_local.hpp"
#include "client_session_service_impl_stubs.hpp"

#include <cassert>
#include <string>
#include <string_view>
#include <unordered_map>

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
game_export_t globals{};
gentity_t* g_entities = nullptr;
gentity_t* host = nullptr;
std::mt19937 mt_rand{};

static cvar_t deathmatch_storage{ .name = nullptr, .string = nullptr, .latchedString = nullptr, .flags = CVAR_NONE, .modifiedCount = 0, .value = 0.0f, .next = nullptr, .integer = 0 };
cvar_t* deathmatch = &deathmatch_storage;
static cvar_t allow_custom_skins_storage{ .name = nullptr, .string = nullptr, .latchedString = nullptr, .flags = CVAR_NONE, .modifiedCount = 0, .value = 0.0f, .next = nullptr, .integer = 0 };
cvar_t* g_allowCustomSkins = &allow_custom_skins_storage;
static cvar_t bot_name_prefix_storage{ .name = nullptr, .string = nullptr, .latchedString = nullptr, .flags = CVAR_NONE, .modifiedCount = 0, .value = 0.0f, .next = nullptr, .integer = 0 };
cvar_t* bot_name_prefix = &bot_name_prefix_storage;

namespace {

	/*
	=============
	InfoPairs

	Extracts key/value pairs from a Quake-style info string into a map for test
	helpers.
	=============
	*/
	static std::unordered_map<std::string, std::string> InfoPairs(const char* info) {
		std::unordered_map<std::string, std::string> pairs;

		if (!info || !*info)
		return pairs;

		std::string_view source{ info };
		size_t cursor = 0;
		while (cursor < source.size()) {
			if (source[cursor] == '\\')
			++cursor;

			size_t key_end = source.find('\\', cursor);
			if (key_end == std::string_view::npos)
			break;

			size_t value_start = key_end + 1;
			size_t value_end = source.find('\\', value_start);
			if (value_end == std::string_view::npos)
			value_end = source.size();

			pairs.emplace(source.substr(cursor, key_end - cursor), source.substr(value_start, value_end - value_start));
			cursor = value_end + 1;
		}

		return pairs;
	}

	/*
	=============
	TestInfo_ValueForKey

	Provides a minimal Info_ValueForKey replacement for exercising ClientConnect
	in tests.
	=============
	*/
	static size_t TestInfo_ValueForKey(const char* s, const char* key, char* buffer, size_t buffer_len) {
		const auto pairs = InfoPairs(s);
		const auto it = pairs.find(key);
		if (it == pairs.end() || buffer_len == 0)
		return 0;

		const size_t copy_len = std::min(buffer_len - 1, it->second.size());
		std::copy_n(it->second.data(), copy_len, buffer);
		buffer[copy_len] = '\0';
		return copy_len;
	}

	/*
	=============
	TestInfo_SetValueForKey

	Simplified Info_SetValueForKey that rewrites the provided buffer for testing.
	=============
	*/
	static bool TestInfo_SetValueForKey(char* s, const char* key, const char* value) {
		auto pairs = InfoPairs(s);
		pairs[std::string(key)] = value ? std::string(value) : std::string{};

		std::string rebuilt;
		for (const auto& [k, v] : pairs) {
			rebuilt.push_back('\\');
			rebuilt.append(k);
			rebuilt.push_back('\\');
			rebuilt.append(v);
		}

		Q_strlcpy(s, rebuilt.c_str(), MAX_INFO_STRING);
		return true;
	}

	/*
	=============
	TestConfigString
	=============
	*/
	static void TestConfigString(int, const char*) {}

	/*
	=============
	TestImageIndex
	=============
	*/
	static int TestImageIndex(const char*) {
		return 0;
	}

	/*
	=============
	TestComPrint
	=============
	*/
	static void TestComPrint(const char*) {}

	/*
	=============
	TestAddCommandString
	=============
	*/
	static void TestAddCommandString(const char*) {}

	class DummyStatsService final : public worr::server::client::ClientStatsService {
		public:
		/*
		=============
		PersistMatchResults

		No-op stub for the test environment.
		=============
		*/
		void PersistMatchResults(const worr::server::client::MatchStatsContext&) override {}

		/*
		=============
		SaveStatsForDisconnect

		No-op stub for the test environment.
		=============
		*/
		void SaveStatsForDisconnect(const worr::server::client::MatchStatsContext&, gentity_t*) override {}
	};

} // namespace

/*
=============
main

Exercises ClientConnect flag resetting when swapping bot and human clients in
the same slot.
=============
*/
int main() {
	deathmatch->integer = 1;
	deathmatch->value = 1.0f;

	allow_custom_skins_storage.string = const_cast<char*>("1");
	allow_custom_skins_storage.integer = 1;
	allow_custom_skins_storage.value = 1.0f;

	static char bot_prefix_value[] = "";
	bot_name_prefix_storage.string = bot_prefix_value;

	gi.Info_ValueForKey = TestInfo_ValueForKey;
	gi.Info_SetValueForKey = TestInfo_SetValueForKey;
	gi.configString = TestConfigString;
	gi.imageIndex = TestImageIndex;
	gi.Com_Print = TestComPrint;
	gi.AddCommandString = TestAddCommandString;

	gclient_t clients[1]{};
	game.clients = clients;
	game.maxClients = 1;

	gentity_t entities[2]{};
	entities[1].client = &clients[0];
	entities[1].inUse = true;
	entities[1].s.number = 1;
	g_entities = entities;
	globals.numEntities = 2;

	DummyStatsService statsService;
	ClientConfigStore configStore(gi, "./");
	worr::server::client::ClientSessionServiceImpl service(gi, game, level, configStore, statsService);

	char humanUserInfo[MAX_INFO_STRING] = "\\name\\Human\\skin\\male/grunt";
	char botUserInfo[MAX_INFO_STRING] = "\\name\\Bot\\skin\\male/grunt";

	assert(service.ClientConnect(gi, game, level, &entities[1], humanUserInfo, "", false));
	assert(!clients[0].sess.is_a_bot);
	assert(!clients[0].sess.consolePlayer);
	assert(!clients[0].sess.admin);
	assert(!clients[0].sess.banned);
	assert(!clients[0].sess.is_888);

	clients[0].sess.admin = true;
	clients[0].sess.banned = true;
	clients[0].sess.consolePlayer = true;
	clients[0].sess.is_888 = true;

	Q_strlcpy(botUserInfo, "\\name\\Bot\\skin\\male/grunt", sizeof(botUserInfo));
	assert(service.ClientConnect(gi, game, level, &entities[1], botUserInfo, "", true));
	assert(clients[0].sess.is_a_bot);
	assert(!clients[0].sess.consolePlayer);
	assert(!clients[0].sess.admin);
	assert(!clients[0].sess.banned);
	assert(!clients[0].sess.is_888);

	clients[0].sess.consolePlayer = true;
	clients[0].sess.admin = true;
	clients[0].sess.banned = true;
	clients[0].sess.is_a_bot = true;

	Q_strlcpy(humanUserInfo, "\\name\\Human\\skin\\male/grunt", sizeof(humanUserInfo));
	assert(service.ClientConnect(gi, game, level, &entities[1], humanUserInfo, "", false));
	assert(!clients[0].sess.is_a_bot);
	assert(!clients[0].sess.consolePlayer);
	assert(!clients[0].sess.admin);
	assert(!clients[0].sess.banned);
	assert(!clients[0].sess.is_888);

	return 0;
}
