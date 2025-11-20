#include <algorithm>
#include <cassert>
#include <cmath>

#include "server/g_local.hpp"

// Minimal globals required by included headers
local_game_import_t gi{};
game_export_t globals{};
gentity_t* g_entities = nullptr;
GameLocals game{};
LevelLocals level{};
std::mt19937 mt_rand{};

static gentity_t worldEntity{};

struct PlaneTraceEnvironment {
	Vector3 normal{};
	float offset = 0.0f;
};

static PlaneTraceEnvironment g_env{};

/*
=============
TestPointContents

Always reports empty space for spawn validation tests.
=============
*/
static int TestPointContents(const Vector3&) {
	return 0;
}

/*
=============
GravityTrace

Simulates a trace against a configurable plane, accounting for the supplied
bounding box when determining the point of contact.
=============
*/
static trace_t GravityTrace(const Vector3& start, const Vector3& mins, const Vector3& maxs, const Vector3& end, const gentity_t*, contents_t) {
	trace_t tr{};

	tr.ent = world;
	tr.plane.normal = g_env.normal;

	Vector3 dir = end - start;

	Vector3 corner{
		g_env.normal.x >= 0.0f ? mins.x : maxs.x,
		g_env.normal.y >= 0.0f ? mins.y : maxs.y,
		g_env.normal.z >= 0.0f ? mins.z : maxs.z
	};

	Vector3 startCorner = start + corner;
	Vector3 endCorner = end + corner;

	float startDist = g_env.normal.dot(startCorner) - g_env.offset;
	float endDist = g_env.normal.dot(endCorner) - g_env.offset;
	float fraction = 1.0f;

	if (startDist == 0.0f) {
		fraction = 0.0f;
	}
	else if ((startDist > 0.0f && endDist < 0.0f) || (startDist < 0.0f && endDist > 0.0f)) {
		float total = startDist - endDist;

		if (total != 0.0f)
			fraction = std::clamp(startDist / total, 0.0f, 1.0f);
	}

	tr.fraction = fraction;
	tr.startSolid = startDist < 0.0f;
	tr.endPos = start + (dir * fraction);

	return tr;
}

/*
=============
main

Validates spawn logic for standard, inverted, and custom-axis gravity setups.
=============
*/
int main() {
	g_entities = &worldEntity;
	gi.trace = GravityTrace;
	gi.pointContents = TestPointContents;

	Vector3 spawnpoint{};
	const Vector3 mins{ -28, -28, -18 };
	const Vector3 maxs{ 28, 28, 18 };

	// Standard gravity, downward onto a floor at z = 0
	worldEntity.gravityVector = { 0, 0, -1 };
	g_env = { { 0, 0, 1 }, 0.0f };
	assert(FindSpawnPoint({ 0, 0, 64 }, mins, maxs, spawnpoint, 64.0f, true, false));
	assert(std::fabs(spawnpoint[2] - 18.0f) < 0.001f);
	assert(CheckGroundSpawnPoint(spawnpoint, mins, maxs, 128.0f, false));

	// Inverted gravity, tracing toward a ceiling at z = 128
	worldEntity.gravityVector = { 0, 0, 1 };
	g_env = { { 0, 0, -1 }, -128.0f };
	assert(FindSpawnPoint({ 0, 0, 32 }, mins, maxs, spawnpoint, 160.0f, true, true));
	assert(std::fabs(spawnpoint[2] - 110.0f) < 0.001f);
	assert(CheckGroundSpawnPoint(spawnpoint, mins, maxs, 256.0f, true));

	// Custom horizontal gravity, pulling along +X toward a wall at x = 0
	worldEntity.gravityVector = { 1, 0, 0 };
	g_env = { { -1, 0, 0 }, 0.0f };
	assert(FindSpawnPoint({ -64, 0, 0 }, mins, maxs, spawnpoint, 96.0f, true, false));
	assert(std::fabs(spawnpoint[0] + maxs[0]) < 0.001f);
	assert(CheckGroundSpawnPoint(spawnpoint, mins, maxs, 192.0f, false));

	return 0;
}
