#pragma once

#include <algorithm>
#include <array>
#include <cstring>
#include <string>

#include "../gameplay/g_domination.hpp"

/*
=============
Domination_SetHudStats

Populates domination control point metadata stats and configstrings for the HUD.
=============
*/
inline void Domination_SetHudStats(std::array<int16_t, MAX_STATS>& stats) {
	stats[STAT_DOMINATION_POINTS] = 0;

	if (Game::IsNot(GameType::Domination)) {
		for (size_t i = 0; i < MAX_DOMINATION_POINTS; ++i) {
			const int configIndex = CONFIG_DOMINATION_POINT_LABEL_START + static_cast<int>(i);
			const char* current = gi.get_configString(configIndex);
			if (current && current[0]) {
				gi.configString(configIndex, "");
			}
		}
		return;
	}

	auto& dom = level.domination;
	const size_t activePoints = std::min(dom.count, MAX_DOMINATION_POINTS);
	uint16_t packedOwners = 0;

	for (size_t i = 0; i < MAX_DOMINATION_POINTS; ++i) {
		const int configIndex = CONFIG_DOMINATION_POINT_LABEL_START + static_cast<int>(i);

		if (i >= activePoints) {
			const char* current = gi.get_configString(configIndex);
			if (current && current[0]) {
				gi.configString(configIndex, "");
			}
			packedOwners = PackDominationPointOwner(packedOwners, i, static_cast<uint16_t>(Team::None));
			continue;
		}

		auto& point = dom.points[i];
		std::string label;

		if (point.ent && point.ent->inUse && point.ent->spawn_count == point.spawnCount) {
			label = Domination_PointLabel(point.ent, point.index);
		} else {
			label = G_Fmt("Point {}", i + 1).data();
		}

		const char* current = gi.get_configString(configIndex);
		if (!current || std::strcmp(current, label.c_str()) != 0) {
			gi.configString(configIndex, label.c_str());
		}

		Team owner = Team::None;
		if (point.ent && point.ent->inUse && point.ent->spawn_count == point.spawnCount) {
			if (point.owner == Team::Red || point.owner == Team::Blue) {
				owner = point.owner;
			}
		}

		packedOwners = PackDominationPointOwner(packedOwners, i, static_cast<uint16_t>(owner));
	}

	stats[STAT_DOMINATION_POINTS] = static_cast<int16_t>(packedOwners);
}
