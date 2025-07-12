#include "../g_local.h"

extern void OpenJoinMenu(gentity_t *ent);

void OpenHostInfoMenu(gentity_t *ent) {
	MenuBuilder builder;
	builder.add("Host Info", MenuAlign::Center)
		.spacer()
		.add("Server Name:", MenuAlign::Left)
		.add(hostname->string, MenuAlign::Left)
		.spacer();

	if (g_entities[1].client) {
		char value[MAX_INFO_VALUE] = { 0 };
		gi.Info_ValueForKey(g_entities[1].client->pers.userInfo, "name", value, sizeof(value));
		if (value[0]) {
			builder.add("Host:", MenuAlign::Left);
			builder.add(value, MenuAlign::Left);
		}
	}

	if (!game.motd.empty()) {
		builder.spacer().add("Message of the Day:", MenuAlign::Left);
		builder.add(game.motd, MenuAlign::Left);
	}

	builder.spacer().add("Return", MenuAlign::Left, [](gentity_t *ent, Menu &) {
		OpenJoinMenu(ent);
		});

	MenuSystem::Open(ent, builder.build());
}
