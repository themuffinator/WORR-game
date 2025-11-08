// menu_page_matchinfo.cpp (Menu Page - Match Info)
// This file implements the "Match Info" menu page, which provides players with
// a summary of the current match's settings and rules.
//
// Key Responsibilities:
// - Information Display: `OpenMatchInfoMenu` builds a read-only menu that
//   displays key details about the ongoing match.
// - Data Fetching: It gathers information from various sources, including the
//   level locals (map name, author), game state (gametype, ruleset), and cvars
//   (timelimit, scorelimit) to populate the menu.
// - Rule Summary: It can be extended to show a detailed summary of active game
//   mutators and server settings (e.g., "InstaGib", "Weapons Stay").

#include "../g_local.hpp"

extern void OpenJoinMenu(gentity_t* ent);

void OpenMatchInfoMenu(gentity_t* ent) {
	MenuBuilder builder;
	builder.add("Match Info", MenuAlign::Center)
		.spacer()
		.add(level.gametype_name.data(), MenuAlign::Left)
		.add(fmt::format("map: {}", level.longName.data()), MenuAlign::Left)
		.add(fmt::format("mapname: {}", level.mapName.data()), MenuAlign::Left);

	if (level.author[0])
		builder.add(fmt::format("author: {}", level.author), MenuAlign::Left);
	if (level.author2[0])
		builder.add(fmt::format("      {}", level.author2), MenuAlign::Left);

	builder.add(fmt::format("ruleset: {}", rs_long_name[(int)game.ruleset]), MenuAlign::Left);

	int styleIndex = g_playstyle ? g_playstyle->integer : static_cast<int>(PlayStyle::Standard);
	if (styleIndex < 0 || styleIndex >= static_cast<int>(PlayStyle::Total))
		styleIndex = static_cast<int>(PlayStyle::Standard);
	builder.add(fmt::format("play style: {}", playstyle_long_name[styleIndex]), MenuAlign::Left);

	if (GT_ScoreLimit())
		builder.add(fmt::format("{} limit: {}", GT_ScoreLimitString(), GT_ScoreLimit()), MenuAlign::Left);

	if (timeLimit->value > 0)
		builder.add(fmt::format("time limit: {}", TimeString(timeLimit->value * 60000, false, false)), MenuAlign::Left);

	if (match_weaponsStay && match_weaponsStay->integer)
		builder.add("weapon availability: Weapons Stay", MenuAlign::Left);
	else if (g_weapon_respawn_time)
		builder.add(fmt::format("weapon respawn: {:.0f}s", g_weapon_respawn_time->value), MenuAlign::Left);

	if (g_no_powerups)
		builder.add(fmt::format("powerups: {}", g_no_powerups->integer ? "Disabled" : "Enabled"), MenuAlign::Left);
	if (g_mapspawn_no_bfg)
		builder.add(fmt::format("BFG: {}", g_mapspawn_no_bfg->integer ? "Disabled" : "Enabled"), MenuAlign::Left);
	if (g_allow_techs)
		builder.add(fmt::format("techs: {}", g_allow_techs->integer ? "Enabled" : "Disabled"), MenuAlign::Left);
	if (g_friendlyFireScale)
		builder.add(fmt::format("friendly fire scale: {:.2f}", g_friendlyFireScale->value), MenuAlign::Left);

	const float spawnProtection = g_spawnProtectionTime ? g_spawnProtectionTime->value : 0.0f;
	builder.add(fmt::format("spawn protection: {}", spawnProtection > 0.0f ? fmt::format("{:.0f}s", spawnProtection) : std::string("Off")), MenuAlign::Left);

	if (g_corpseSinkDelay)
		builder.add(fmt::format("corpse sink delay: {:.0f}s", g_corpseSinkDelay->value), MenuAlign::Left);

	if (g_matchstats)
		builder.add(fmt::format("match stats: {}", g_matchstats->integer ? "Enabled" : "Disabled"), MenuAlign::Left);

	builder.spacer().add("Return", MenuAlign::Left, [](gentity_t* ent, Menu&) {
		OpenJoinMenu(ent);
	});

	MenuSystem::Open(ent, builder.build());
}

