/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_widow_roof_spawn.cpp implementation.*/

#include <cassert>
#include <cmath>
#include <cstddef>
#include <random>

#include "server/g_local.hpp"

// Minimal globals required by included headers
local_game_import_t gi{};
game_export_t globals{};
gentity_t* g_entities = nullptr;
GameLocals game{};
LevelLocals level{};
std::mt19937 mt_rand{};

static constexpr float kRoofHeight = 128.0f;
static gentity_t worldEntity{};

/*
=============
TestPointContents

Returns solid only when sampling the ceiling plane used by the test traces.
=============
*/
static int TestPointContents(const Vector3& point) {
	return (point[2] >= kRoofHeight) ? CONTENTS_SOLID : 0;
}

/*
=============
TestTrace

Simulates upward traces that stop at the roof plane while keeping other
traces unobstructed. The returned trace always reports the world entity as
the collider so spawn validation succeeds.
=============
*/
static trace_t TestTrace(const Vector3& start, const Vector3& mins, const Vector3& maxs, const Vector3& end, const gentity_t*, contents_t) {
	trace_t tr{};

	tr.ent = world;

	if (start == end) {
		tr.endPos = start;
		return tr;
	}

	const bool movingUp = end[2] > start[2];

	if (movingUp) {
		const float hitZ = kRoofHeight - maxs[2];
		const float clampedEnd = std::min(end[2], hitZ);
		const float total = end[2] - start[2];
		const float moved = std::max(0.0f, clampedEnd - start[2]);

		tr.endPos = start;
		tr.endPos[2] = clampedEnd;
		tr.fraction = total != 0.0f ? moved / total : 0.0f;
	}
	else {
		tr.endPos = end;
		tr.fraction = 1.0f;
	}

	return tr;
}

/*
=============
main

Validates that roof-based spawn discovery produces a ceiling-aligned spawn
position and that ground checks accept the ceiling contact without clipping.
=============
*/
int main() {
	g_entities = &worldEntity;
	gi.trace = TestTrace;
	gi.pointContents = TestPointContents;

	Vector3 spawnpoint{};
	const Vector3 mins{ -28, -28, -18 };
	const Vector3 maxs{ 28, 28, 18 };
	const Vector3 startpoint{ 0, 0, 64 };

	const bool found = FindSpawnPoint(startpoint, mins, maxs, spawnpoint, 64.0f, true, true);
	assert(found);
	assert(std::fabs(spawnpoint[2] - (kRoofHeight - maxs[2])) < 0.001f);

	const bool grounded = CheckGroundSpawnPoint(spawnpoint, mins, maxs, 256.0f, true);
	assert(grounded);

	return 0;
}
