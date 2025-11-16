#pragma once

#include "../g_local.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>

/*
=============
PCfg_TrimView

Removes leading and trailing whitespace from a string view.
=============
*/
inline std::string_view PCfg_TrimView(std::string_view text) {
	while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
		text.remove_prefix(1);
	}

	while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
		text.remove_suffix(1);
	}

	return text;
}

/*
=============
PCfg_ParseInt

Attempts to parse an integer from the supplied string view.
=============
*/
inline bool PCfg_ParseInt(std::string_view text, int& outValue) {
	text = PCfg_TrimView(text);
	if (text.empty()) {
		return false;
	}

	const std::string temp(text);
	char* end = nullptr;
	const long parsed = std::strtol(temp.c_str(), &end, 10);
	if (end == temp.c_str() || *end != '\0') {
		return false;
	}

	outValue = static_cast<int>(parsed);
	return true;
}

/*
=============
PCfg_ParseBool

Attempts to parse a boolean from the supplied string view.
=============
*/
inline bool PCfg_ParseBool(std::string_view text, bool& outValue) {
	text = PCfg_TrimView(text);
	if (text.empty()) {
		return false;
	}

	std::string lowered(text);
	std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});

	if (lowered == "1" || lowered == "true" || lowered == "yes" || lowered == "on") {
		outValue = true;
		return true;
	}

	if (lowered == "0" || lowered == "false" || lowered == "no" || lowered == "off") {
		outValue = false;
		return true;
	}

	return false;
}

/*
=============
PCfg_ApplyConfigLine

Parses and applies a single key/value pair from the legacy player config.
=============
*/
inline void PCfg_ApplyConfigLine(gclient_t* client, std::string_view line) {
line = PCfg_TrimView(line);
if (line.empty()) {
		return;
	}

	if (line.size() >= 2 && line[0] == '/' && line[1] == '/') {
		return;
	}

	if (line.front() == '#') {
		return;
	}

	const size_t separator = line.find_first_of("	 ");
	if (separator == std::string_view::npos) {
		return;
	}

	const std::string_view key = line.substr(0, separator);
	const std::string_view value = PCfg_TrimView(line.substr(separator + 1));

	if (key == "show_id") {
		bool parsed = false;
if (PCfg_ParseBool(value, parsed)) {
client->sess.pc.show_id = parsed;
}
return;
	}

	if (key == "show_fragmessages") {
		bool parsed = false;
if (PCfg_ParseBool(value, parsed)) {
client->sess.pc.show_fragmessages = parsed;
}
return;
	}

	if (key == "show_timer") {
		bool parsed = false;
if (PCfg_ParseBool(value, parsed)) {
client->sess.pc.show_timer = parsed;
}
return;
	}

	if (key == "killbeep_num") {
		int parsed = 0;
if (PCfg_ParseInt(value, parsed)) {
if (parsed < 0) {
parsed = 0;
}
if (parsed > 4) {
parsed = 4;
}
client->sess.pc.killbeep_num = parsed;
}
return;
	}
}

/*
=============
PCfg_ParseConfigBuffer

Parses the legacy player configuration buffer and applies known settings.
=============
*/
inline void PCfg_ParseConfigBuffer(gclient_t* client, const char* buffer) {
if (!buffer || !*buffer) {
return;
}

	const char* cursor = buffer;
	while (*cursor) {
		const char* line_start = cursor;
		while (*cursor && *cursor != '\n' && *cursor != '\r') {
			++cursor;
		}

const std::string_view line(line_start, static_cast<size_t>(cursor - line_start));
PCfg_ApplyConfigLine(client, line);

		while (*cursor == '\n' || *cursor == '\r') {
			++cursor;
		}
	}
}

/*
=============
PCfg_ClientInitPConfigForSession

Initializes the player configuration for a specific client session by loading
an existing config file or generating a default when none is present.
=============
*/
inline void PCfg_ClientInitPConfigForSession(gclient_t* client, svflags_t svFlags) {
	bool	file_exists = false;
	bool	cfg_valid = true;
	bool	directory_ready = true;

	if (!client) return;
	if (svFlags & SVF_BOT) return;

	const std::string originalSocialID = client->sess.socialID;
	const std::string sanitizedSocialID = SanitizeSocialID(originalSocialID);
	if (sanitizedSocialID.empty()) {
		if (gi.Com_Print) {
			gi.Com_PrintFmt("WARNING: {}: refusing to read player config for invalid social ID '{}'\n", __FUNCTION__, originalSocialID.c_str());
		}
		return;
	}
	if (sanitizedSocialID != originalSocialID && gi.Com_Print) {
		gi.Com_PrintFmt("WARNING: {}: sanitized social ID '{}' to '{}' for player config filename\n", __FUNCTION__, originalSocialID.c_str(), sanitizedSocialID.c_str());
	}
	constexpr const char* player_config_directory = "baseq2/pcfg";
	const std::string path = G_Fmt("{}/{}.cfg", player_config_directory, sanitizedSocialID).data();

	std::error_code directory_error;
	if (!std::filesystem::create_directories(player_config_directory, directory_error) && directory_error) {
		directory_ready = false;
		if (gi.Com_Print) {
			gi.Com_PrintFmt("WARNING: {}: failed to create player config directory \"{}\": {}\n", __FUNCTION__, player_config_directory, directory_error.message().c_str());
		}
	}

	FILE* f = fopen(path.c_str(), "rb");
	char* buffer = nullptr;
	if (f != nullptr) {
		size_t length;
		size_t read_length = 0;

		fseek(f, 0, SEEK_END);
		length = static_cast<size_t>(ftell(f));
		fseek(f, 0, SEEK_SET);

		if (length > 0x40000) {
			cfg_valid = false;
		}
		if (cfg_valid) {
			buffer = static_cast<char*>(gi.TagMalloc ? gi.TagMalloc(length + 1, TAG_LEVEL) : std::malloc(length + 1));
			if (!buffer) {
				cfg_valid = false;
			}
			else {
				if (length > 0) {
					read_length = fread(buffer, 1, length, f);

					if (length != read_length) {
						cfg_valid = false;
					}
				}
				buffer[read_length] = '\0';
			}
		}
		file_exists = true;
		fclose(f);

		if (!cfg_valid || !buffer) {
			if (buffer) {
				if (gi.TagFree) {
					gi.TagFree(buffer);
				}
				else {
					std::free(buffer);
				}
			}
			gi.Com_PrintFmt("{}: Player config load error for \"{}\", discarding.\n", __FUNCTION__, path.c_str());
			return;
		}

PCfg_ParseConfigBuffer(client, buffer);
		if (gi.TagFree) {
			gi.TagFree(buffer);
		}
		else {
			std::free(buffer);
		}
	}

	if (!file_exists) {
		if (directory_ready) {
			f = fopen(path.c_str(), "w");
			if (f) {
				const char* str = G_Fmt("// {}'s Player Config\n// Generated by WOR\n", client->sess.netName).data();

				fwrite(str, 1, strlen(str), f);
				gi.Com_PrintFmt("{}: Player config written to: \"{}\"\n", __FUNCTION__, path.c_str());
				fclose(f);
			}
			else {
				gi.Com_PrintFmt("{}: Cannot save player config: {}\n", __FUNCTION__, path.c_str());
			}
		}
		else {
			gi.Com_PrintFmt("{}: Cannot save player config: {}\n", __FUNCTION__, path.c_str());
		}
	}
}

/*
=============
PCfg_ClientInitPConfig

Convenience wrapper that adapts gentity_t pointers to the session-level
initializer.
=============
*/
inline void PCfg_ClientInitPConfig(gentity_t* ent) {
	if (!ent)
		return;
	PCfg_ClientInitPConfigForSession(ent->client, ent->svFlags);
}
