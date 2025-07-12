#include "../g_local.h"
#include <string>

extern void OpenJoinMenu(gentity_t *ent);

static inline void AddReturnToCallvoteMenu(MenuBuilder &builder) {
	builder.spacer().add("Return", MenuAlign::Left, [](gentity_t *e, Menu &) {
		OpenCallvoteMenu(e);
		});
}

static void OpenCallvoteMap(gentity_t *ent) {
	MenuBuilder builder;
	builder.add("Callvote: Map", MenuAlign::Center).spacer();

	for (const auto &entry : game.mapSystem.mapPool) {
		const std::string &displayName = entry.longName.empty() ? entry.filename : entry.longName;

		builder.add(displayName, MenuAlign::Left, [mapname = entry.filename](gentity_t *e, Menu &) {
			if (TryStartVote(e, "map", mapname, true))
				MenuSystem::Close(e);
			});
	}

	AddReturnToCallvoteMenu(builder);

	MenuSystem::Open(ent, builder.build());
}

static void OpenCallvoteGametype(gentity_t *ent) {
	MenuBuilder builder;
	builder.add("Callvote: Gametype", MenuAlign::Center).spacer();

	for (size_t i = 0; i < gt_short_name.size(); ++i) {
		const std::string &shortName = gt_short_name[i];
		const char *longName = gt_long_name[i];

		builder.add(longName, MenuAlign::Left, [shortName](gentity_t *e, Menu &) {
			if (TryStartVote(e, "gametype", shortName, true))
				MenuSystem::Close(e);
			});
	}

	AddReturnToCallvoteMenu(builder);
	MenuSystem::Open(ent, builder.build());
}

static void OpenCallvoteRuleset(gentity_t *ent) {
	MenuBuilder builder;
	builder.add("Callvote: Ruleset", MenuAlign::Center).spacer();

	for (size_t i = 1; i < rs_short_name.size(); ++i) {
		const char *shortName = rs_short_name[i];
		const char *longName = rs_long_name[i];

		builder.add(longName, MenuAlign::Left, [shortName](gentity_t *e, Menu &) {
			if (TryStartVote(e, "ruleset", shortName, true))
				MenuSystem::Close(e);
			});
	}

	AddReturnToCallvoteMenu(builder);

	MenuSystem::Open(ent, builder.build());
}

static void OpenCallvoteUnlagged(gentity_t *ent) {
	MenuBuilder builder;
	builder.add("Callvote: Unlagged", MenuAlign::Center).spacer();

	builder.add("Enable", MenuAlign::Left, [](gentity_t *e, Menu &) {
		if (TryStartVote(e, "unlagged", "1", true))
			MenuSystem::Close(e);
		});

	builder.add("Disable", MenuAlign::Left, [](gentity_t *e, Menu &) {
		if (TryStartVote(e, "unlagged", "0", true))
			MenuSystem::Close(e);
		});

	AddReturnToCallvoteMenu(builder);
	MenuSystem::Open(ent, builder.build());
}

static void OpenCallvoteRandom(gentity_t *ent) {
	MenuBuilder builder;
	builder.add("Callvote: Random", MenuAlign::Center).spacer();

	constexpr int kMin = 2;
	constexpr int kMax = 100;

	for (int i = kMin; i <= kMax; i += 5) {
		builder.add(fmt::format("1-{}", i), MenuAlign::Left, [i](gentity_t *e, Menu &) {
			if (TryStartVote(e, "random", std::to_string(i), true))
				MenuSystem::Close(e);
			});
	}

	AddReturnToCallvoteMenu(builder);
	MenuSystem::Open(ent, builder.build());
}

static void OpenCallvoteArena(gentity_t *ent) {
	MenuBuilder builder;
	builder.add("Callvote: Arena", MenuAlign::Center).spacer();

	int optionsAdded = 0;
	for (int i = 0; i < level.arenaTotal; ++i) {
		int arenaNum = i + 1;
		if (arenaNum == level.arenaActive)
			continue;

		builder.add(fmt::format("Arena {}", arenaNum), MenuAlign::Left, [arenaNum](gentity_t *e, Menu &) {
			if (TryStartVote(e, "arena", std::to_string(arenaNum), true))
				MenuSystem::Close(e);
			});
		++optionsAdded;
	}

	if (optionsAdded == 0)
		builder.add("No other arenas available", MenuAlign::Left);

	AddReturnToCallvoteMenu(builder);
	MenuSystem::Open(ent, builder.build());
}

static void OpenSimpleCallvote(const std::string &voteName, gentity_t *ent) {
	if (TryStartVote(ent, voteName, "", true))
		MenuSystem::Close(ent);
}

static void AddCallvoteOption(MenuBuilder &builder, const std::string &label, const std::string &argHint, const std::string &desc, std::function<void(gentity_t *)> callback) {
	builder.add(label, MenuAlign::Left, [callback](gentity_t *e, Menu &) {
		callback(e);
		});
}

void OpenCallvoteMenu(gentity_t *ent) {
	MenuBuilder builder;
	builder.add("Call a Vote", MenuAlign::Center).spacer();

	builder.add("Map", MenuAlign::Left, [](gentity_t *e, Menu &) {
		OpenCallvoteMap(e);
		});

	builder.add("Next Map", MenuAlign::Left, [](gentity_t *e, Menu &) {
		OpenSimpleCallvote("nextMap", e);
		});

	builder.add("Restart Match", MenuAlign::Left, [](gentity_t *e, Menu &) {
		OpenSimpleCallvote("restart", e);
		});

	builder.add("Gametype", MenuAlign::Left, [](gentity_t *e, Menu &) {
		OpenCallvoteGametype(e);
		});

	builder.add("Ruleset", MenuAlign::Left, [](gentity_t *e, Menu &) {
		OpenCallvoteRuleset(e);
		});

	if (Teams()) {
		builder.add("Shuffle Teams", MenuAlign::Left, [](gentity_t *e, Menu &) {
			OpenSimpleCallvote("shuffle", e);
			});

		builder.add("Balance Teams", MenuAlign::Left, [](gentity_t *e, Menu &) {
			OpenSimpleCallvote("balance", e);
			});
	}

	builder.add("Unlagged", MenuAlign::Left, [](gentity_t *e, Menu &) {
		OpenCallvoteUnlagged(e);
		});

	builder.add("Cointoss", MenuAlign::Left, [](gentity_t *e, Menu &) {
		OpenSimpleCallvote("cointoss", e);
		});

	builder.add("Random Number", MenuAlign::Left, [](gentity_t *e, Menu &) {
		OpenCallvoteRandom(e);
		});

	if (level.arenaTotal) {
		builder.add("Arena", MenuAlign::Left, [](gentity_t *e, Menu &) {
			OpenCallvoteArena(e);
			});
	}

	builder.spacer().add("Return", MenuAlign::Left, [](gentity_t *e, Menu &) {
		OpenJoinMenu(e);
		});

	MenuSystem::Open(ent, builder.build());
}
