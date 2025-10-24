// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

VORE (Shalrath) - WOR port

- Ranged: launches a slow homing pod that tracks target and explodes
- No melee; prefers mid/long engagements with clear shot checks
- Idle/search/sight barks, two pain sets, standard death
- Uses WOR muzzle-flash system like Chick/Gunner/Ogre

==============================================================================
*/

#include "../g_local.hpp"
#include "m_vore.hpp"
#include "m_flash.hpp"

// -----------------------------------------------------------------------------
// Config
// -----------------------------------------------------------------------------
static const int   VORE_POD_DAMAGE = 35;
static const float VORE_POD_SPEED = 550.0f;
// static const float VORE_POD_TURNRATE = 0.045f; // handled in projectile think (g_weapon.cpp) if used
static const float VORE_MIN_RANGE = 160.0f;
static const float VORE_MAX_RANGE = 1024.0f;

// -----------------------------------------------------------------------------
// Sounds
// -----------------------------------------------------------------------------
static cached_soundIndex snd_attack;	// not sure what this is for
static cached_soundIndex snd_attack2; // launch
static cached_soundIndex snd_idle;
static cached_soundIndex snd_search;
static cached_soundIndex snd_sight;
static cached_soundIndex snd_pain;
static cached_soundIndex snd_death;

/*
===============
vore_idle
===============
*/
static void vore_idle(gentity_t* self) {
	if (frandom() < 0.5f)
		gi.sound(self, CHAN_VOICE, snd_idle, 1, ATTN_IDLE, 0);
}

/*
===============
vore_search
===============
*/
MONSTERINFO_SEARCH(vore_search) (gentity_t* self) -> void {
	gi.sound(self, CHAN_VOICE, snd_search, 1, ATTN_IDLE, 0);
}

/*
===============
vore_sight
===============
*/
MONSTERINFO_SIGHT(vore_sight) (gentity_t* self, gentity_t* other) -> void {
	gi.sound(self, CHAN_VOICE, snd_sight, 1, ATTN_NORM, 0);
}

/*
===============
vore_stand
===============
*/
static MonsterFrame vore_frames_stand[] = {
	{ ai_stand, 0, vore_idle },
	{ ai_stand },{ ai_stand },{ ai_stand },{ ai_stand },
	{ ai_stand },{ ai_stand },{ ai_stand },{ ai_stand },{ ai_stand }
};

MMOVE_T(vore_move_stand) = { FRAME_stand01, FRAME_stand10, vore_frames_stand, nullptr };
MONSTERINFO_STAND(vore_stand) (gentity_t* self) -> void {
	M_SetAnimation(self, &vore_move_stand);
}

/*
===============
vore_pain
===============
*/
PAIN(vore_pain) (gentity_t* self, gentity_t* other, float kick, int damage, const MeansOfDeath& mod) -> void {
	if (self->pain_debounce_time > level.time)
		return;

	// modest debounce; Vore is a mid-range turret-y monster
	self->pain_debounce_time = level.time + 300_ms;

	gi.sound(self, CHAN_VOICE, snd_pain, 1, ATTN_NORM, 0);
	M_SetAnimation(self, &vore_move_stand); // quick flinch not needed; keep pressure
}

/*
===============
vore_select_flash
Map current attack frame to a muzzle flash index and small lateral spread.
===============
*/
static void vore_select_flash(int frame, MonsterMuzzleFlashID& flashNumber, float& spread_rl) {
	switch (frame) {
	case FRAME_attack01:
	case FRAME_attack02:
		flashNumber = MZ2_VORE_POD_1; spread_rl = -0.08f; break;
	case FRAME_attack03:
	case FRAME_attack04:
		flashNumber = MZ2_VORE_POD_2; spread_rl = -0.04f; break;
	case FRAME_attack05:
	case FRAME_attack06:
		flashNumber = MZ2_VORE_POD_3; spread_rl = 0.04f; break;
	default:
		flashNumber = MZ2_VORE_POD_4; spread_rl = 0.08f; break;
	}
}

/*
===============
vore_walk
===============
*/
static MonsterFrame vore_frames_walk[] = {
	{ ai_walk, 4 },{ ai_walk, 4 },{ ai_walk, 4 },{ ai_walk, 4 },
	{ ai_walk, 4 },{ ai_walk, 4 },{ ai_walk, 4 },{ ai_walk, 4 }
};
MMOVE_T(vore_move_walk) = { FRAME_walk01, FRAME_walk08, vore_frames_walk, nullptr };

MONSTERINFO_WALK(vore_walk) (gentity_t* self) -> void {
	M_SetAnimation(self, &vore_move_walk);
}

/*
===============
vore_run
===============
*/
static MonsterFrame vore_frames_run[] = {
	{ ai_run, 10 },{ ai_run, 12 },{ ai_run, 12 },{ ai_run, 14 },
	{ ai_run, 10 },{ ai_run, 12 },{ ai_run, 12 },{ ai_run, 14 }
};
MMOVE_T(vore_move_run) = { FRAME_run01, FRAME_run08, vore_frames_run, nullptr };

MONSTERINFO_RUN(vore_run) (gentity_t* self) -> void {
	M_SetAnimation(self, &vore_move_run);
}

/*
===============
vore_fire
Fire a tracking pod using gunner-style muzzle flash projection.
Note: Requires MZ2_VORE_POD_1..4 entries and offsets in m_flash.hpp.
===============
*/
static void vore_fire(gentity_t* self) {
	if (!self->enemy || !self->enemy->inUse)
		return;

	Vector3 forward, right, up;
	AngleVectors(self->s.angles, forward, right, up);

	MonsterMuzzleFlashID flashNumber;
	float spread_rl;
	vore_select_flash(self->s.frame, flashNumber, spread_rl);

	// project muzzle from flash offset like Chick/Gunner/Ogre
	const Vector3 start = M_ProjectFlashSource(self, monster_flash_offset[flashNumber], forward, right);

	// choose target: blind-fire target if set, otherwise enemy origin
	const Vector3 target = (self->monsterInfo.aiFlags & AI_MANUAL_STEERING && self->monsterInfo.blind_fire_target)
		? self->monsterInfo.blind_fire_target
		: self->enemy->s.origin;

	const Vector3 to = target - start;
	const float   dist = to.length();

	Vector3 aim = forward + (right * spread_rl);
	if (dist > 512.0f)
		aim[2] += 0.06f; // gentle loft at long range

	aim.normalize();

	gi.sound(self, CHAN_WEAPON, snd_attack2, 1, ATTN_NORM, 0);

	// follow Chick’s pattern: call wrapper that handles the muzzleflash
	monster_fire_homing_pod(self, start, aim, VORE_POD_DAMAGE, VORE_POD_SPEED, flashNumber);
}

/*
===============
vore_attack
===============
*/
static MonsterFrame vore_frames_attack[] = {
	{ ai_charge, 0 },
	{ ai_charge, 0 },
	{ ai_charge, 0, vore_fire }, // fire 1
	{ ai_charge, 0 },
	{ ai_charge, 0, vore_fire }, // optional second
	{ ai_charge, 0 },
	{ ai_charge, 0 },
	{ ai_charge, 0 }
};
MMOVE_T(vore_move_attack) = { FRAME_attack01, FRAME_attack08, vore_frames_attack, nullptr };

MONSTERINFO_ATTACK(vore_attack) (gentity_t* self) -> void {
	M_SetAnimation(self, &vore_move_attack);
}

/*
===============
vore_checkattack
===============
*/
MONSTERINFO_CHECKATTACK(vore_checkattack) (gentity_t* self) -> bool {
	if (!self->enemy || self->enemy->health <= 0)
		return false;

	// range gating
	const float d = (self->enemy->s.origin - self->s.origin).length();
	if (d < VORE_MIN_RANGE || d > VORE_MAX_RANGE)
		return false;

	// clear shot from a representative flash
	if (!M_CheckClearShot(self, monster_flash_offset[MZ2_VORE_POD_2]))
		return false;

	self->monsterInfo.attackState = MonsterAttackState::Missile;
	return true;
}

/*
===============
SP_monster_vore
===============
*/
void SP_monster_vore(gentity_t* self) {
	if (!M_AllowSpawn(self)) {
		FreeEntity(self);
		return;
	}

	// sounds
	snd_attack.assign("shalrath/attack.wav");	// probably to remove
	snd_attack2.assign("shalrath/attack2.wav");
	snd_idle.assign("shalrath/idle.wav");
	snd_search.assign("shalrath/search.wav");	// remove
	snd_sight.assign("shalrath/sight.wav");
	snd_pain.assign("shalrath/pain.wav");
	snd_death.assign("shalrath/death.wav");

	// model + bbox
	self->moveType = MoveType::Step;
	self->solid = SOLID_BBOX;
	self->s.modelIndex = gi.modelIndex("models/monsters/shalrath/tris.md2");
	self->mins = { -24, -24, -24 };
	self->maxs = { 24,  24,  40 };

	// stats
	self->health = self->maxHealth = 350 * st.health_multiplier;
	self->gibHealth = -90;
	self->mass = 400;

	// think/ai wiring
	self->yawSpeed = 20;

	self->monsterInfo.stand = vore_stand;
	self->monsterInfo.walk = vore_walk;
	self->monsterInfo.run = vore_run;
	self->monsterInfo.attack = vore_attack;
	self->monsterInfo.sight = vore_sight;
	self->monsterInfo.search = vore_search;
	self->pain = vore_pain;
	self->monsterInfo.checkAttack = vore_checkattack;

	// behavior prefs
	self->monsterInfo.combatStyle = CombatStyle::Ranged;
	self->monsterInfo.dropHeight = 192;
	self->monsterInfo.jumpHeight = 0;
	self->monsterInfo.blindFire = true;

	gi.linkEntity(self);

	M_SetAnimation(self, &vore_move_stand);
	self->monsterInfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}
