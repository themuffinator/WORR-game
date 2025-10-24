// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

// g_svcmds.cpp (Game Server Commands)
// This file implements the server-side logic for commands that are executed
// from the server console or via RCON (Remote Console). These commands
// typically begin with the "sv" prefix.
//
// Key Responsibilities:
// - `ServerCommand()`: The main entry point that the engine calls when an "sv"
//   command is issued. It acts as a dispatcher, matching the command name
//   to the appropriate handler function.
// - IP Filtering: Implements commands for managing server access based on IP
//   addresses (`addip`, `removeip`, `listip`, `writeip`). This allows server
//   administrators to create ban lists or allow lists.
// - Packet Filtering: Contains the logic for `G_FilterPacket`, which is called
//   by the engine to determine if an incoming connection from a specific IP
//   address should be allowed or denied based on the configured filter list
//   and mode (ban vs. allow).

#include "g_local.hpp"

static void Svcmd_Test_f() {
	gi.LocClient_Print(nullptr, PRINT_HIGH, "Svcmd_Test_f()\n");
}

/*
==============================================================================

PACKET FILTERING


You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and any unspecified digits will match any value, so you can specify an entire
class C network with "addip 192.246.40".

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single
host.

listip
Prints the current list of filters.

writeip
Dumps "addip <ip>" commands to listip.cfg so it can be execed at a later date.  The filter lists are not saved and
restored by default, because I beleive it would cause too much confusion.

filterBan <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the
default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that
only allows players from your local network.


==============================================================================
*/

struct ipFilter_t {
	unsigned mask;
	unsigned compare;
};

constexpr size_t MAX_IPFILTERS = 1024;

ipFilter_t ipfilters[MAX_IPFILTERS];
int		   numipfilters;

/*
===============
StringToFilter

Parses an IP string into filter structure.
===============
*/
static bool StringToFilter(const char *s, ipFilter_t *f) {
	if (!s || !f)
		return false;

	std::array<uint8_t, 4> b{};
	std::array<uint8_t, 4> m{};

	for (int i = 0; i < 4; ++i) {
		if (*s < '0' || *s > '9') {
			gi.LocClient_Print(nullptr, PRINT_HIGH, "Bad filter address: {}\n", s);
			return false;
		}

		char num[4] = {};
		int j = 0;

		while (*s >= '0' && *s <= '9' && j < 3) {
			num[j++] = *s++;
		}
		num[j] = '\0';

		const unsigned val = std::strtoul(num, nullptr, 10);
		b[i] = static_cast<uint8_t>(val);
		m[i] = (val != 0) ? 255 : 0;

		if (!*s)
			break;
		if (*s != '.')
			return false; // malformed IP segment separator
		++s;
	}

	std::memcpy(&f->compare, b.data(), sizeof(uint32_t));
	std::memcpy(&f->mask, m.data(), sizeof(uint32_t));
	return true;
}

/*
===============
G_FilterPacket

Determines whether a given IP address should be blocked.
===============
*/
bool G_FilterPacket(const char *from) {
	if (!from)
		return false;

	std::array<uint8_t, 4> m{};
	int segment = 0;
	const char *p = from;

	while (*p && segment < 4) {
		if (*p < '0' || *p > '9') {
			if (*p == ':' || *p == '\0')
				break;
			++p;
			continue;
		}

		uint8_t value = 0;
		while (*p >= '0' && *p <= '9') {
			value = value * 10 + static_cast<uint8_t>(*p - '0');
			++p;
		}

		m[segment++] = value;

		if (*p == '.')
			++p;
		else
			break;
	}

	uint32_t in = 0;
	std::memcpy(&in, m.data(), sizeof(uint32_t));

	for (int i = 0; i < numipfilters; ++i) {
		if ((in & ipfilters[i].mask) == ipfilters[i].compare)
			return filterBan->integer != 0;
	}

	return filterBan->integer == 0;
}

/*
=================
SVCmd_AddIP_f
=================
*/
static void SVCmd_AddIP_f() {
	int i;

	if (gi.argc() < 3) {
		gi.LocClient_Print(nullptr, PRINT_HIGH, "Usage: sv {} <ip-mask>\n", gi.argv(1));
		return;
	}

	for (i = 0; i < numipfilters; i++)
		if (ipfilters[i].compare == 0xffffffff)
			break; // free spot
	if (i == numipfilters) {
		if (numipfilters == MAX_IPFILTERS) {
			gi.LocClient_Print(nullptr, PRINT_HIGH, "IP filter list is full\n");
			return;
		}
		numipfilters++;
	}

	if (!StringToFilter(gi.argv(2), &ipfilters[i]))
		ipfilters[i].compare = 0xffffffff;
}

/*
=================
G_RemoveIP_f
=================
*/
static void SVCmd_RemoveIP_f() {
	ipFilter_t f;
	int		   i, j;

	if (gi.argc() < 3) {
		gi.LocClient_Print(nullptr, PRINT_HIGH, "Usage: sv {} <ip-mask>\n", gi.argv(1));
		return;
	}

	if (!StringToFilter(gi.argv(2), &f))
		return;

	for (i = 0; i < numipfilters; i++)
		if (ipfilters[i].mask == f.mask && ipfilters[i].compare == f.compare) {
			for (j = i + 1; j < numipfilters; j++)
				ipfilters[j - 1] = ipfilters[j];
			numipfilters--;
			gi.LocClient_Print(nullptr, PRINT_HIGH, "Removed.\n");
			return;
		}
	gi.LocClient_Print(nullptr, PRINT_HIGH, "Didn't find {}.\n", gi.argv(2));
}

/*
===============
SVCmd_ListIP_f

Prints all active IP filter entries.
===============
*/
static void SVCmd_ListIP_f() {
	gi.LocClient_Print(nullptr, PRINT_HIGH, "Filter list:\n");

	for (int i = 0; i < numipfilters; ++i) {
		const uint32_t ip = ipfilters[i].compare;
		const uint8_t b0 = static_cast<uint8_t>(ip & 0xFF);
		const uint8_t b1 = static_cast<uint8_t>((ip >> 8) & 0xFF);
		const uint8_t b2 = static_cast<uint8_t>((ip >> 16) & 0xFF);
		const uint8_t b3 = static_cast<uint8_t>((ip >> 24) & 0xFF);

		gi.LocClient_Print(nullptr, PRINT_HIGH, "{}.{}.{}.{}\n", b0, b1, b2, b3);
	}
}

// [Paril-KEX]
static void SVCmd_NextMap_f() {
	gi.LocBroadcast_Print(PRINT_HIGH, "$g_map_ended_by_server");
	Match_End();
}

/*
=================
G_WriteIP_f
=================
*/
static void SVCmd_WriteIP_f(void) {
	// KEX_FIXME: Sys_FOpen isn't available atm, just commenting this out since i don't think we even need this functionality - sponge
	/*
	FILE* f;

	byte	b[4];
	int		i;
	cvar_t* game;

	game = gi.cvar("game", "", 0);

	std::string name;
	if (!*game->string)
		name = GAMEVERSION + "/listip.cfg";
	else
		name = std::string(game->string) + "/listip.cfg";

	gi.LocClient_Print(nullptr, PRINT_HIGH, "Writing {}.\n", name.c_str());

	f = Sys_FOpen(name.c_str(), "wb");
	if (!f)
	{
		gi.LocClient_Print(nullptr, PRINT_HIGH, "Couldn't open {}\n", name.c_str());
		return;
	}

	fprintf(f, "set filterBan %d\n", filterBan->integer);

	for (i = 0; i < numipfilters; i++)
	{
		*(unsigned*)b = ipfilters[i].compare;
		fprintf(f, "sv addip %i.%i.%i.%i\n", b[0], b[1], b[2], b[3]);
	}

	fclose(f);
	*/
}

/*
=================
ServerCommand

ServerCommand will be called when an "sv" command is issued.
The game can issue gi.argc() / gi.argv() commands to get the rest
of the parameters
=================
*/
void ServerCommand() {
	const char *cmd = gi.argv(1);

	if (Q_strcasecmp(cmd, "test") == 0)
		Svcmd_Test_f();
	else if (Q_strcasecmp(cmd, "addip") == 0)
		SVCmd_AddIP_f();
	else if (Q_strcasecmp(cmd, "removeip") == 0)
		SVCmd_RemoveIP_f();
	else if (Q_strcasecmp(cmd, "listip") == 0)
		SVCmd_ListIP_f();
	else if (Q_strcasecmp(cmd, "writeip") == 0)
		SVCmd_WriteIP_f();
	else if (Q_strcasecmp(cmd, "nextMap") == 0)
		SVCmd_NextMap_f();
	else
		gi.LocClient_Print(nullptr, PRINT_HIGH, "Unknown server command \"{}\"\n", cmd);
}
