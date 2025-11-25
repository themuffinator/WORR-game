/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

menu_page_welcome.cpp (Menu Page - Welcome/Join) This file implements the main menu that players
see when they are spectators or have just joined the server. It is the primary navigation hub
for joining the game, spectating, or accessing other informational menus. Key Responsibilities:
- Main Menu Hub: `OpenJoinMenu` is the function called to display the main menu. - Dynamic Join
Options: The `onUpdate` function (`AddJoinOptions`) dynamically creates the "Join" options based
on the current gametype (e.g., "Join Red", "Join Blue" for TDM; "Join Match" or "Join Queue" for
FFA/Duel). - Player Counts: Displays the current number of players in the match or on each team.
- Navigation: Provides the entry points to all other major menus, such as "Host Info", "Match
Info", and "Call a Vote".*/

#include "../g_local.hpp"

extern bool SetTeam(gentity_t* ent, Team desired_team, bool inactive, bool force, bool silent);
void GetFollowTarget(gentity_t* ent);
void FreeFollower(gentity_t* ent);
bool Vote_Menu_Active(gentity_t* ent);

extern void OpenHostInfoMenu(gentity_t* ent);
extern void OpenMatchInfoMenu(gentity_t* ent);
extern void OpenPlayerMatchStatsMenu(gentity_t* ent);
extern void OpenAdminSettingsMenu(gentity_t* ent);
extern void OpenVoteMenu(gentity_t* ent);

static void AddJoinOptions(MenuBuilder& builder, gentity_t* ent, int maxPlayers) {
	uint8_t redCount = 0, blueCount = 0, freeCount = 0, queueCount = 0;
	for (auto ec : active_clients()) {
		if (Game::Has(GameFlags::OneVOne) && ec->client->sess.team == Team::Spectator && ec->client->sess.matchQueued) {
			queueCount++;
		}
		else {
			switch (ec->client->sess.team) {
			case Team::Free:  freeCount++; break;
			case Team::Red:   redCount++;  break;
			case Team::Blue:  blueCount++; break;
			}
		}
	}

	if (Teams()) {
		builder.add(fmt::format("Join Red ({}/{})", redCount, maxPlayers / 2), MenuAlign::Left, [](gentity_t* e, Menu&) {
			SetTeam(e, Team::Red, false, false, false);
			});
		builder.add(fmt::format("Join Blue ({}/{})", blueCount, maxPlayers / 2), MenuAlign::Left, [](gentity_t* e, Menu&) {
			SetTeam(e, Team::Blue, false, false, false);
			});
	}
	else {
		std::string joinText;
		if (Game::Has(GameFlags::OneVOne) && level.pop.num_playing_clients == 2)
			joinText = fmt::format("Join Queue ({}/{})", queueCount, maxPlayers - 2);
		else
			joinText = fmt::format("Join Match ({}/{})", freeCount, Game::Has(GameFlags::OneVOne) ? 2 : maxPlayers);

		builder.add(joinText, MenuAlign::Left, [](gentity_t* e, Menu&) {
			SetTeam(e, Team::Free, false, false, false);
			});
	}
}

void OpenJoinMenu(gentity_t* ent) {
	if (!ent || !ent->client) return;

	if (Vote_Menu_Active(ent)) {
		OpenVoteMenu(ent);
		return;
	}

	int maxPlayers = maxplayers->integer;
	if (maxPlayers < 1) maxPlayers = 1;

	MenuBuilder builder;
	builder.add(G_Fmt("{} v{}", worr::version::kGameTitle, worr::version::kGameVersion).data(), MenuAlign::Center).spacer();
	builder.add("---", MenuAlign::Center).spacer().spacer();

	AddJoinOptions(builder, ent, maxPlayers);

	builder.add("Spectate", MenuAlign::Left, [](gentity_t* e, Menu&) {
		SetTeam(e, Team::Spectator, false, false, false);
		});

	if (g_allowVoting->integer && (ClientIsPlaying(ent->client) || (!ClientIsPlaying(ent->client) && g_allowSpecVote->integer))) {
		builder.add("Call a Vote", MenuAlign::Left, [](gentity_t* e, Menu&) {
			OpenCallvoteMenu(e);
			});
	}

	builder.add("Host Info", MenuAlign::Left, [](gentity_t* e, Menu&) {
		OpenHostInfoMenu(e);
		});

	builder.add("Match Info", MenuAlign::Left, [](gentity_t* e, Menu&) {
		OpenMatchInfoMenu(e);
		});

	if (g_matchstats->integer) {
		builder.add("Player Stats", MenuAlign::Left, [](gentity_t* e, Menu&) {
			OpenPlayerMatchStatsMenu(e);
			});
	}

	if (ent->client->sess.admin) {
		builder.add("Admin", MenuAlign::Left, [](gentity_t* e, Menu&) {
			OpenAdminSettingsMenu(e);
			});
	}

	builder.spacer().spacer().spacer().spacer();
	builder.add("visit darkmatter-quake.com", MenuAlign::Center);
	builder.add(":: community :: matches ::", MenuAlign::Center);
	builder.add(":: content :: news ::", MenuAlign::Center);
	MenuSystem::Open(ent, builder.build());
}
