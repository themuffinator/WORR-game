#include "../src/server/gameplay/g_turret.cpp"

#include <cassert>
#include <cstring>
#include <type_traits>

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
std::mt19937 mt_rand{};
spawn_temp_t st{};
cvar_t skill_storage{};
cvar_t* skill = &skill_storage;
cvar_t deathmatch_storage{};
cvar_t* deathmatch = &deathmatch_storage;
cvar_t g_debug_monster_kills_storage{};
cvar_t* g_debug_monster_kills = &g_debug_monster_kills_storage;
const Vector3 vec3_origin{};

/*
=============
Damage
=============
*/
void Damage(gentity_t*, gentity_t*, gentity_t*, const Vector3&, const Vector3&, const Vector3&, int, int, DamageFlags, MeansOfDeath) {}

/*
=============
fire_rocket
=============
*/
gentity_t* fire_rocket(gentity_t*, const Vector3&, const Vector3&, int, int, int, int) {
	using Storage = std::aligned_storage_t<sizeof(gentity_t), alignof(gentity_t)>;
	static Storage rocket_storage;
	std::memset(&rocket_storage, 0, sizeof(rocket_storage));
	return reinterpret_cast<gentity_t*>(&rocket_storage);
}

/*
=============
PickTarget
=============
*/
gentity_t* PickTarget(const char*) {
	return nullptr;
}

/*
=============
FreeEntity
=============
*/
void FreeEntity(gentity_t*) {}

/*
=============
InfantryPrecache
=============
*/
void InfantryPrecache() {}

/*
=============
monster_use
=============
*/
void monster_use(gentity_t*, gentity_t*, gentity_t*) {}

/*
=============
monster_think
=============
*/
void monster_think(gentity_t*) {}

/*
=============
infantry_die
=============
*/
void infantry_die(gentity_t*, gentity_t*, gentity_t*, int, const Vector3&, const MeansOfDeath&) {}

/*
=============
infantry_stand
=============
*/
void infantry_stand(gentity_t*) {}

/*
=============
infantry_pain
=============
*/
void infantry_pain(gentity_t*, gentity_t*, float, int, const MeansOfDeath&) {}

/*
=============
infantry_setskin
=============
*/
void infantry_setskin(gentity_t*) {}

/*
=============
FindTarget
=============
*/
bool FindTarget(gentity_t*) {
	return false;
}

/*
=============
visible
=============
*/
bool visible(gentity_t*, gentity_t*) {
	return true;
}

/*
=============
FindItemByClassname
=============
*/
item_t* FindItemByClassname(const char*) {
	return nullptr;
}

/*
=============
G_FixStuckObject
=============
*/
void G_FixStuckObject(gentity_t*, const Vector3&) {}

/*
=============
AllocateEntity

Provides a zero-initialized gentity_t instance for tests.
=============
*/
gentity_t* AllocateEntity() {
	using Storage = std::aligned_storage_t<sizeof(gentity_t), alignof(gentity_t)>;
	static Storage storage;
	std::memset(&storage, 0, sizeof(storage));
	return reinterpret_cast<gentity_t*>(&storage);
}

/*
=============
main
=============
*/
int main() {
	gi.frameTimeSec = 0.05f;

	gentity_t* driverBreach = AllocateEntity();
	TurretRequestFire(driverBreach);
	assert(driverBreach->turretFireRequested);
	assert(TurretConsumeFireRequest(driverBreach));
	assert(!driverBreach->turretFireRequested);
	assert(!TurretConsumeFireRequest(driverBreach));

	gentity_t* brainBreach = AllocateEntity();
	TurretRequestFire(brainBreach);
	TurretRequestFire(brainBreach);
	assert(TurretConsumeFireRequest(brainBreach));
	assert(!brainBreach->turretFireRequested);

	return 0;
}
