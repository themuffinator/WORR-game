#include <array>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

static constexpr int CS_PLAYERSKINS = 0;
static constexpr int CONFIG_CHASE_PLAYER_NAME = 1;
static constexpr std::size_t MAX_NETNAME = 64;

struct session_t {
	char netName[MAX_NETNAME] = { '\\0' };
};

struct persistent_t {
	char netName[MAX_NETNAME] = { '\\0' };
};

struct gclient_t {
	session_t sess{};
	persistent_t pers{};
};

struct gentity_s {
	gclient_t* client = nullptr;
	int s_number = 0;
};

using gentity_t = gentity_s;

struct local_game_import_t {
	bool Info_ValueForKey(const char* userInfo, const char* key, char* value, std::size_t len) const;
	void Info_SetValueForKey(std::string& userInfo, const char* key, const char* value) const;
	void configString(int index, const char* value);

	std::vector<std::string> configstrings{ 2 };
};

/*
=============
ParseUserinfo

Splits a Quake-style userinfo string into a key/value map for easier access.
=============
*/
static std::unordered_map<std::string, std::string> ParseUserinfo(const char* userInfo) {
	std::unordered_map<std::string, std::string> values{};
	if (!userInfo)
		return values;

	const char* cursor = userInfo;
	while (*cursor) {
		if (*cursor != '\\') {
			++cursor;
			continue;
		}

		++cursor;
		const char* keyStart = cursor;
		while (*cursor && *cursor != '\\')
			++cursor;
		std::string key(keyStart, cursor);

		if (*cursor == '\\')
			++cursor;
		const char* valueStart = cursor;
		while (*cursor && *cursor != '\\')
			++cursor;
		std::string value(valueStart, cursor);

		if (!key.empty())
			values[key] = value;
	}

	return values;
}

/*
=============
BuildUserinfoString

Serializes key/value pairs back into a Quake-style userinfo string.
=============
*/
static std::string BuildUserinfoString(const std::unordered_map<std::string, std::string>& entries) {
	std::string result;
	for (const auto& [key, value] : entries) {
		result.push_back('\\');
		result.append(key);
		result.push_back('\\');
		result.append(value);
	}
	return result;
}

/*
=============
local_game_import_t::Info_ValueForKey

Retrieves a value from the userinfo string into the provided buffer, returning
true on success.
=============
*/
bool local_game_import_t::Info_ValueForKey(const char* userInfo, const char* key, char* value, std::size_t len) const {
	if (!userInfo || !key || !value || len == 0)
		return false;

	auto entries = ParseUserinfo(userInfo);
	auto it = entries.find(key);
	if (it == entries.end())
		return false;

	std::strncpy(value, it->second.c_str(), len);
	value[len - 1] = '\\0';
	return true;
}

/*
=============
local_game_import_t::Info_SetValueForKey

Updates a key/value pair within the userinfo string and rewrites it in-place.
=============
*/
void local_game_import_t::Info_SetValueForKey(std::string& userInfo, const char* key, const char* value) const {
	if (!key || !value)
		return;

	auto entries = ParseUserinfo(userInfo.c_str());
	entries[key] = value;
	userInfo = BuildUserinfoString(entries);
}

/*
=============
local_game_import_t::configString

Stores the provided configstring value in the capture buffer for verification.
=============
*/
void local_game_import_t::configString(int index, const char* value) {
	if (index < 0)
		return;

	if (static_cast<std::size_t>(index) >= configstrings.size())
		configstrings.resize(static_cast<std::size_t>(index) + 1);

	configstrings[static_cast<std::size_t>(index)] = value ? value : "";
}

/*
=============
ClientUserinfoChanged

Minimal stand-in that mirrors the production function's handling of names and
configstrings for this regression test.
=============
*/
static void ClientUserinfoChanged(local_game_import_t& gi, gentity_t* ent, const std::string& userInfo) {
	std::array<char, MAX_NETNAME> name{};
	if (!gi.Info_ValueForKey(userInfo.c_str(), "name", name.data(), name.size())) {
		std::strncpy(name.data(), "badinfo", name.size());
		name[name.size() - 1] = '\\0';
	}

	std::strncpy(ent->client->sess.netName, name.data(), MAX_NETNAME);
	ent->client->sess.netName[MAX_NETNAME - 1] = '\\0';

	const std::string skin = "male/grunt";
	const std::string composite = std::string(ent->client->sess.netName) + "\\\\" + skin;
	gi.configString(CS_PLAYERSKINS + ent->s_number, composite.c_str());
	gi.configString(CONFIG_CHASE_PLAYER_NAME + ent->s_number, ent->client->sess.netName);

	std::strncpy(ent->client->pers.netName, ent->client->sess.netName, MAX_NETNAME);
	ent->client->pers.netName[MAX_NETNAME - 1] = '\\0';
}

class ClientSessionHarness {
public:
	explicit ClientSessionHarness(local_game_import_t& import)
	: gi(import) {}

	/*
	=============
	ClientSessionHarness::ClientConnect

	Applies the bot name prefix before delegating to ClientUserinfoChanged,
	then records the name used for simulated stats persistence.
	=============
	*/
	void ClientConnect(gentity_t* ent, std::string& userInfo, bool isBot) {
		if (!ent || !ent->client)
			return;

		if (isBot) {
			const char* botPrefix = "B|";
			std::array<char, MAX_NETNAME> originalName{};
			gi.Info_ValueForKey(userInfo.c_str(), "name", originalName.data(), originalName.size());
			const std::string prefixedName = std::string(botPrefix) + originalName.data();
			gi.Info_SetValueForKey(userInfo, "name", prefixedName.c_str());
		}

		ClientUserinfoChanged(gi, ent, userInfo);
		statsNetName.assign(ent->client->pers.netName);
	}

	/*
	=============
	ClientSessionHarness::StatsName

	Returns the name captured for persistence validation.
	=============
	*/
	std::string StatsName() const {
		return statsNetName;
	}

private:
	local_game_import_t& gi;
	std::string statsNetName{};
};

/*
=============
main

Confirms that bot name prefixing occurs before userinfo processing so every
consumer sees the prefixed name.
=============
*/
int main() {
	local_game_import_t gi{};
	ClientSessionHarness harness(gi);

	gclient_t client{};
	gentity_t ent{};
	ent.client = &client;
	ent.s_number = 1;

	std::string userInfo = "\\name\\Crash\\skin\\male/grunt";
	harness.ClientConnect(&ent, userInfo, true);

	assert(std::string(ent.client->sess.netName) == "B|Crash");
	assert(std::string(ent.client->pers.netName) == "B|Crash");
	assert(gi.configstrings[CS_PLAYERSKINS + ent.s_number] == "B|Crash\\\\male/grunt");
	assert(gi.configstrings[CONFIG_CHASE_PLAYER_NAME + ent.s_number] == "B|Crash");
	assert(harness.StatsName() == "B|Crash");

	return 0;
}
