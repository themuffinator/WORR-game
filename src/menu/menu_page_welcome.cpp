#include "../g_local.h"

extern bool SetTeam(gentity_t *ent, team_t desired_team, bool inactive, bool force, bool silent);
void GetFollowTarget(gentity_t *ent);
void FreeFollower(gentity_t *ent);
bool Vote_Menu_Active(gentity_t *ent);

extern void OpenHostInfoMenu(gentity_t *ent);
extern void OpenMatchInfoMenu(gentity_t *ent);
extern void OpenPlayerMatchStatsMenu(gentity_t *ent);
extern void OpenAdminSettingsMenu(gentity_t *ent);
extern void OpenVoteMenu(gentity_t *ent);

static void AddJoinOptions(MenuBuilder &builder, gentity_t *ent, int maxPlayers) {
	uint8_t redCount = 0, blueCount = 0, freeCount = 0, queueCount = 0;
	for (auto ec : active_clients()) {
		if (GTF(GTF_1V1) && ec->client->sess.team == team_t::TEAM_SPECTATOR && ec->client->sess.matchQueued) {
			queueCount++;
		} else {
			switch (ec->client->sess.team) {
			case team_t::TEAM_FREE:  freeCount++; break;
			case team_t::TEAM_RED:   redCount++;  break;
			case team_t::TEAM_BLUE:  blueCount++; break;
			}
		}
	}

	if (Teams()) {
		builder.add(fmt::format("Join Red ({}/{})", redCount, maxPlayers / 2), MenuAlign::Left, [](gentity_t *e, Menu &) {
			SetTeam(e, team_t::TEAM_RED, false, false, false);
			});
		builder.add(fmt::format("Join Blue ({}/{})", blueCount, maxPlayers / 2), MenuAlign::Left, [](gentity_t *e, Menu &) {
			SetTeam(e, team_t::TEAM_BLUE, false, false, false);
			});
	} else {
		std::string joinText;
		if (GTF(GTF_1V1) && level.pop.num_playing_clients == 2)
			joinText = fmt::format("Join Queue ({}/{})", queueCount, maxPlayers - 2);
		else
			joinText = fmt::format("Join Match ({}/{})", freeCount, GTF(GTF_1V1) ? 2 : maxPlayers);

		builder.add(joinText, MenuAlign::Left, [](gentity_t *e, Menu &) {
			SetTeam(e, team_t::TEAM_FREE, false, false, false);
			});
	}
}

void OpenJoinMenu(gentity_t *ent) {
	if (!ent || !ent->client) return;

	if (Vote_Menu_Active(ent)) {
		OpenVoteMenu(ent);
		return;
	}

	int maxPlayers = maxplayers->integer;
	if (maxPlayers < 1) maxPlayers = 1;

	MenuBuilder builder;
	builder.add(G_Fmt("{} v{}", GAMEMOD_TITLE, GAMEMOD_VERSION).data(), MenuAlign::Center).spacer();
	builder.add("---", MenuAlign::Center).spacer().spacer();

	AddJoinOptions(builder, ent, maxPlayers);

	builder.add("Spectate", MenuAlign::Left, [](gentity_t *e, Menu &) {
		SetTeam(e, team_t::TEAM_SPECTATOR, false, false, false);
		});

	builder.add("Chase Camera", MenuAlign::Left, [](gentity_t *e, Menu &) {
		SetTeam(e, team_t::TEAM_SPECTATOR, false, false, false);
		if (e->client->followTarget) FreeFollower(e);
		else GetFollowTarget(e);
		MenuSystem::Close(e);
		});

	builder.add("Host Info", MenuAlign::Left, [](gentity_t *e, Menu &) {
		OpenHostInfoMenu(e);
		});

	builder.add("Match Info", MenuAlign::Left, [](gentity_t *e, Menu &) {
		OpenMatchInfoMenu(e);
		});

	if (g_matchstats->integer) {
		builder.add("Player Stats", MenuAlign::Left, [](gentity_t *e, Menu &) {
			OpenPlayerMatchStatsMenu(e);
			});
	}

	if (ent->client->sess.admin) {
		builder.add("Admin", MenuAlign::Left, [](gentity_t *e, Menu &) {
			OpenAdminSettingsMenu(e);
			});
	}

	builder.spacer().spacer().spacer().spacer();
	builder.add("visit darkmatter-quake.com", MenuAlign::Center);
	builder.add(":: community :: matches ::", MenuAlign::Center);
	builder.add(":: content :: news ::", MenuAlign::Center);
	MenuSystem::Open(ent, builder.build());
}
