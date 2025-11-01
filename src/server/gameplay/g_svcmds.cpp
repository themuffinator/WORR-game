// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

// g_svcmds.cpp (Game Server Commands) - modernized C++
//
// Responsibilities:
// - ServerCommand(): dispatch "sv" console/RCON commands
// - IP filtering: addip/removeip/listip/writeip
// - G_FilterPacket(): packet gate using configured filters

#include "../g_local.hpp"

#include <array>
#include <vector>
#include <string_view>
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <charconv>

// External cvars/engine globals expected from g_local.hpp
// extern cvar_t* filterBan; // 1 = matching IPs are banned, 0 = only matching IPs are allowed

namespace
{

	/*
	===============
	IPFilter
	===============
	*/
	struct IPFilter
	{
		std::array<uint8_t, 4> compare{}; // value to compare
		std::array<uint8_t, 4> mask{};    // mask; 255 means must match, 0 means wildcard
	};

	constexpr size_t MAX_IPFILTERS = 1024;

	static std::vector<IPFilter> g_filters; // active filters

	/*
	===============
	IsDigit
	===============
	*/
	inline bool IsDigit(char c) noexcept
	{
		return c >= '0' && c <= '9';
	}

	/*
	===============
	ParseOctet

	Parses a decimal octet [0..255] from the head of s.
	Advances sv to the first non-digit.
	Returns std::optional style via bool success + out value.
	===============
	*/
	static bool ParseOctet(std::string_view& sv, uint8_t& out) noexcept
	{
		if (sv.empty() || !IsDigit(sv.front()))
			return false;

		unsigned value = 0;
		size_t i = 0;
		while (i < sv.size() && IsDigit(sv[i]) && value <= 255u) {
			value = (value * 10u) + static_cast<unsigned>(sv[i] - '0');
			++i;
		}

		if (value > 255u)
			return false;

		out = static_cast<uint8_t>(value);
		sv.remove_prefix(i);
		return true;
	}

	/*
	===============
	StringToFilter

	Parses an IP mask string into an IPFilter.
	Behavior matches classic Quake II semantics:
	- Dotted quad with optional trailing segments.
	- Any segment set to 0 acts as a wildcard (mask 0).
	Examples:
	  "192.168.1.15"  => exact host
	  "192.168.0.0"   => wildcard last two (class C style)
	  "10"            => wildcard last three
	===============
	*/
	static bool StringToFilter(std::string_view s, IPFilter& out)
	{
		std::array<uint8_t, 4> b{ 0, 0, 0, 0 };
		std::array<uint8_t, 4> m{ 0, 0, 0, 0 };

		for (int seg = 0; seg < 4; ++seg) {
			// empty segment list: stop early (remaining stay as 0/wildcards)
			if (s.empty())
				break;

			// malformed: non-digit at start (except we allow immediate '.' to mean empty which we reject)
			if (!IsDigit(s.front())) {
				gi.LocClient_Print(nullptr, PRINT_HIGH, "Bad filter address: {}\n", std::string(s).c_str());
				return false;
			}

			uint8_t oct = 0;
			if (!ParseOctet(s, oct)) {
				gi.LocClient_Print(nullptr, PRINT_HIGH, "Bad filter address: {}\n", std::string(s).c_str());
				return false;
			}

			b[seg] = oct;
			m[seg] = (oct != 0) ? 255 : 0;

			// expect '.' between segments, otherwise we stop
			if (!s.empty()) {
				if (s.front() == '.') {
					s.remove_prefix(1);
					continue;
				}
				// allow stopping if not '.' (e.g. end or port ':' etc.)
				break;
			}
		}

		out.compare = b;
		out.mask = m;
		return true;
	}

	/*
	===============
	ParseFromAddress

	Extract dotted IPv4 left side of "a.b.c.d[:port]" into 4 octets.
	Silently ignores trailing ":port". Non-digit leading chars are skipped.
	Returns true on success.
	===============
	*/
	static bool ParseFromAddress(std::string_view s, std::array<uint8_t, 4>& out)
	{
		// Skip up to the first digit
		while (!s.empty() && !IsDigit(s.front()))
			s.remove_prefix(1);

		std::array<uint8_t, 4> b{ 0, 0, 0, 0 };
		int seg = 0;
		while (!s.empty() && seg < 4) {
			if (!IsDigit(s.front()))
				break;

			uint8_t oct = 0;
			if (!ParseOctet(s, oct))
				return false;

			b[seg++] = oct;

			if (!s.empty() && s.front() == '.') {
				s.remove_prefix(1);
				continue;
			}
			break;
		}

		if (seg == 0)
			return false;

		out = b;
		return true;
	}

	/*
	===============
	Matches
	===============
	*/
	static bool Matches(const IPFilter& f, const std::array<uint8_t, 4>& in) noexcept
	{
		for (int i = 0; i < 4; ++i) {
			if ((in[i] & f.mask[i]) != f.compare[i])
				return false;
		}
		return true;
	}

	/*
	===============
	PrintIP
	===============
	*/
	static void PrintIP(const std::array<uint8_t, 4>& b)
	{
		gi.LocClient_Print(nullptr, PRINT_HIGH, "{}.{}.{}.{}\n",
			static_cast<int>(b[0]),
			static_cast<int>(b[1]),
			static_cast<int>(b[2]),
			static_cast<int>(b[3]));
	}

	/*
	===============
	Svcmd_Test_f
	===============
	*/
	static void Svcmd_Test_f()
	{
		gi.LocClient_Print(nullptr, PRINT_HIGH, "Svcmd_Test_f()\n");
	}

	/*
	===============
	SVCmd_AddIP_f
	===============
	*/
	static void SVCmd_AddIP_f()
	{
		if (gi.argc() < 3) {
			gi.LocClient_Print(nullptr, PRINT_HIGH, "Usage: sv {} <ip-mask>\n", gi.argv(1));
			return;
		}

		if (g_filters.size() >= MAX_IPFILTERS) {
			gi.LocClient_Print(nullptr, PRINT_HIGH, "IP filter list is full\n");
			return;
		}

		IPFilter f{};
		if (!StringToFilter(gi.argv(2), f))
			return;

		// If identical exists, do not duplicate.
		auto it = std::find_if(g_filters.begin(), g_filters.end(),
			[&](const IPFilter& x)
			{
				return x.compare == f.compare && x.mask == f.mask;
			});
		if (it == g_filters.end()) {
			g_filters.emplace_back(f);
		}
	}

	/*
	===============
	SVCmd_RemoveIP_f
	===============
	*/
	static void SVCmd_RemoveIP_f()
	{
		if (gi.argc() < 3) {
			gi.LocClient_Print(nullptr, PRINT_HIGH, "Usage: sv {} <ip-mask>\n", gi.argv(1));
			return;
		}

		IPFilter f{};
		if (!StringToFilter(gi.argv(2), f))
			return;

		const auto oldSize = g_filters.size();
		g_filters.erase(std::remove_if(g_filters.begin(), g_filters.end(),
			[&](const IPFilter& x)
			{
				return x.mask == f.mask && x.compare == f.compare;
			}),
			g_filters.end());

		if (g_filters.size() != oldSize) {
			gi.LocClient_Print(nullptr, PRINT_HIGH, "Removed.\n");
		}
		else {
			gi.LocClient_Print(nullptr, PRINT_HIGH, "Did not find {}.\n", gi.argv(2));
		}
	}

	/*
	===============
	SVCmd_ListIP_f
	===============
	*/
	static void SVCmd_ListIP_f()
	{
		gi.LocClient_Print(nullptr, PRINT_HIGH, "Filter list:\n");
		for (const auto& f : g_filters) {
			PrintIP(f.compare);
		}
	}

	/*
	===============
	SVCmd_WriteIP_f

	Note: Original implementation wrote list to listip.cfg.
	Left as a no-op unless file I/O helpers are provided.
	===============
	*/
	static void SVCmd_WriteIP_f()
	{
		// Intentionally left inert until file IO helper is available.
		gi.LocClient_Print(nullptr, PRINT_HIGH, "writeip not implemented.\n");
	}

	/*
	===============
	SVCmd_NextMap_f
	===============
	*/
	static void SVCmd_NextMap_f()
	{
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_map_ended_by_server");
		Match_End();
	}

} // anonymous namespace

/*
===============
G_FilterPacket

Determines whether a given IP address should be blocked.
Respects filterBan:
- filterBan = 1 (default): matching IPs are rejected
- filterBan = 0: ONLY matching IPs are accepted
===============
*/
bool G_FilterPacket(const char* from)
{
	if (!from || !*from)
		return false;

	std::array<uint8_t, 4> in{};
	if (!ParseFromAddress(std::string_view{ from }, in))
		return false;

	const bool anyMatch = std::any_of(g_filters.begin(), g_filters.end(),
		[&](const IPFilter& f) { return Matches(f, in); });

	// If filterBan==1, a match means block; if ==0, a match means allow-only
	if (filterBan && filterBan->integer != 0)
		return anyMatch;      // match => blocked
	else
		return !anyMatch;     // no match => blocked
}

/*
===============
ServerCommand

Dispatch "sv" commands
===============
*/
void ServerCommand()
{
	const char* cmd = gi.argv(1);
	if (!cmd || !*cmd) {
		gi.LocClient_Print(nullptr, PRINT_HIGH, "No server command provided.\n");
		return;
	}

	if (Q_strcasecmp(cmd, "test") == 0) {
		Svcmd_Test_f();
	}
	else if (Q_strcasecmp(cmd, "addip") == 0) {
		SVCmd_AddIP_f();
	}
	else if (Q_strcasecmp(cmd, "removeip") == 0) {
		SVCmd_RemoveIP_f();
	}
	else if (Q_strcasecmp(cmd, "listip") == 0) {
		SVCmd_ListIP_f();
	}
	else if (Q_strcasecmp(cmd, "writeip") == 0) {
		SVCmd_WriteIP_f();
	}
	else if (Q_strcasecmp(cmd, "nextmap") == 0) {
		SVCmd_NextMap_f();
	}
	else {
		gi.LocClient_Print(nullptr, PRINT_HIGH, "Unknown server command \"{}\"\n", cmd);
	}
}

/*
===============
Notes

- Endian safety: we compare per-octet with masks, avoiding memcpy to uint32_t.
- Wildcard semantics: identical to legacy code; an octet value of 0 becomes mask 0.
- Capacity: up to MAX_IPFILTERS entries, de-duplicates identical filters.
- I/O: writeip is a stub until file helpers are available (Sys_FOpen etc.).
- Threading: expects main thread usage like original.
*/
