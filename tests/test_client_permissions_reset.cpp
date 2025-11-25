/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_client_permissions_reset.cpp implementation.*/

#include <cassert>
#include <string>

#include "server/g_local.hpp"

extern void ClientCheckPermissionsForTesting(GameLocals& game, gentity_t* ent, const char* socialID);

/*
=============
main

Ensures that reconnecting without a social ID clears admin and banned flags
from a previous authenticated session.
=============
*/
int main() {
	GameLocals game{};
	game.adminIDs.insert("trusted-id");
	game.bannedIDs.insert("trusted-id");

	gclient_t client{};
	gentity_t ent{};
	ent.client = &client;

	ClientCheckPermissionsForTesting(game, &ent, "trusted-id");
	assert(ent.client->sess.admin);
	assert(ent.client->sess.banned);

	ClientCheckPermissionsForTesting(game, &ent, "");
	assert(!ent.client->sess.admin);
	assert(!ent.client->sess.banned);

	return 0;
}
