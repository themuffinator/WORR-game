#include "../g_local.h"

extern void OpenJoinMenu(gentity_t *ent);

void OpenMatchInfoMenu(gentity_t *ent) {
	MenuBuilder builder;
	builder.add("Match Info", MenuAlign::Center)
		.spacer()
		.add(level.gametype_name.data(), MenuAlign::Left)
		.add(fmt::format("map: {}", level.longName), MenuAlign::Left)
		.add(fmt::format("mapname: {}", level.mapName), MenuAlign::Left);

	if (level.author[0])
		builder.add(fmt::format("author: {}", level.author), MenuAlign::Left);
	if (level.author2[0])
		builder.add(fmt::format("      {}", level.author2), MenuAlign::Left);

	builder.add(fmt::format("ruleset: {}", rs_long_name[(int)game.ruleset]), MenuAlign::Left);

	if (GT_ScoreLimit())
		builder.add(fmt::format("{} limit: {}", GT_ScoreLimitString(), GT_ScoreLimit()), MenuAlign::Left);

	if (timelimit->value > 0)
		builder.add(fmt::format("time limit: {}", TimeString(timelimit->value * 60000, false, false)), MenuAlign::Left);

	builder.spacer().add("Return", MenuAlign::Left, [](gentity_t *ent, Menu &) {
		OpenJoinMenu(ent);
		});

	MenuSystem::Open(ent, builder.build());
}
