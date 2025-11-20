#include <algorithm>
#include <array>
#include <cstddef>
#include <cassert>
#include <cmath>
#include <random>

namespace std {
	using ::sinf;

	template<typename E>
	constexpr auto to_underlying(E e) noexcept {
		return static_cast<std::underlying_type_t<E>>(e);
	}
}

#include "server/g_local.hpp"

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
std::mt19937 mt_rand{};

alignas(gentity_t) static std::array<std::byte, sizeof(gentity_t)> entityStorage{};
gentity_t* g_entities = reinterpret_cast<gentity_t*>(entityStorage.data());
gentity_t* g_edicts = reinterpret_cast<gentity_t*>(entityStorage.data());

struct TraceEnvironment {
	Vector3 normal;
	float offset;
	contents_t contents;
};

static TraceEnvironment g_env{ { 0, 0, 1 }, 0.0f, MASK_MONSTERSOLID };

/*
=============
SetTraceEnvironment

Configures the trace plane used by the test harness.
=============
*/
static void SetTraceEnvironment(const Vector3& normal, float offset, contents_t contents) {
	g_env.normal = normal;
	g_env.offset = offset;
	g_env.contents = contents;
}

/*
=============
TestTrace

Simple planar trace used to emulate gravity-aware collisions.
=============
*/
static trace_t TestTrace(const Vector3& start, const Vector3* mins, const Vector3* maxs, const Vector3& end, const gentity_t* passthrough, contents_t contentMask) {
	trace_t tr{};

	const Vector3 dir = end - start;
	const float startDist = start.dot(g_env.normal) - g_env.offset;
	const float endDist = end.dot(g_env.normal) - g_env.offset;

	tr.ent = world;
	tr.contents = g_env.contents & contentMask;
	tr.fraction = 1.0f;
	tr.endPos = end;

	if (dir.lengthSquared() == 0.0f) {
		tr.startSolid = startDist <= 0.0f;
		tr.allSolid = tr.startSolid;
		return tr;
	}

	if ((startDist > 0.0f && endDist > 0.0f) || (startDist < 0.0f && endDist < 0.0f))
		return tr;

	const float denom = startDist - endDist;
	const float fraction = (denom != 0.0f) ? (startDist / denom) : 1.0f;

	tr.fraction = std::clamp(fraction, 0.0f, 1.0f);
	tr.endPos = start + (dir * tr.fraction);
	tr.plane.normal = g_env.normal;
	tr.startSolid = startDist <= 0.0f;
	tr.contents = g_env.contents & contentMask;

	return tr;
}

/*
=============
G_FixStuckObject_Generic

Test stub that always reports a valid position.
=============
*/
StuckResult G_FixStuckObject_Generic(Vector3& origin, const Vector3& ownMins, const Vector3& ownMaxs, std::function<stuck_object_trace_fn_t> trace) {
	return StuckResult::GoodPosition;
}

#define UNIT_TEST
#include "server/gameplay/g_monster_spawn.cpp"
#undef UNIT_TEST

/*
=============
main
=============
*/
int main() {
	gi.game_import_t::trace = TestTrace;

	const Vector3 mins{ -16, -16, -16 };
	const Vector3 maxs{ 16, 16, 16 };

	SetTraceEnvironment({ 0, 0, 1 }, 0.0f, MASK_MONSTERSOLID);
	assert(CheckGroundSpawnPoint({ 0, 0, 64 }, mins, maxs, 128.0f, { 0, 0, -1 }));

	SetTraceEnvironment({ 0, 0, -1 }, -128.0f, MASK_MONSTERSOLID);
	assert(CheckGroundSpawnPoint({ 0, 0, 0 }, mins, maxs, 256.0f, { 0, 0, 1 }));

	SetTraceEnvironment({ 1, 0, 0 }, 0.0f, MASK_MONSTERSOLID);
	Vector3 lateralSpawn{ 64, 0, 0 };
	assert(FindSpawnPoint(lateralSpawn, mins, maxs, lateralSpawn, 0.0f, true, { -1, 0, 0 }));
	assert(std::fabs(lateralSpawn.x) < 0.01f);
	assert(CheckGroundSpawnPoint({ 64, 0, 0 }, mins, maxs, 128.0f, { -1, 0, 0 }));

	const Vector3 diagonalNormal = Vector3{ 0.0f, 0.7071067f, 0.7071067f };
	SetTraceEnvironment(diagonalNormal, 0.0f, MASK_MONSTERSOLID);
	Vector3 diagonalSpawn{ 0, 128, 128 };
	assert(CheckSpawnPoint(diagonalSpawn, mins, maxs, { 0, -1, -1 }));
	assert(CheckGroundSpawnPoint(diagonalSpawn, mins, maxs, 256.0f, { 0, -1, -1 }));

	SetTraceEnvironment({ 0, 0, 1 }, 0.0f, MASK_WATER);
	assert(!CheckGroundSpawnPoint({ 0, 0, 64 }, mins, maxs, 128.0f, { 0, 0, -1 }));

	return 0;
}
