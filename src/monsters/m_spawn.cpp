// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

SPAWN (Tarbaby) - WOR port

Behavior:
- Bounding kamikaze blob. Primary attack is a leap that detonates on contact.
- Small pain flinch; otherwise keeps pressure.
- Explodes on death or on touching an enemy while leaping.
- Slight splash immunity suggestion (optional): low knockback.

Notes:
- Frames are a scaffold; map ranges to your model export.
- Uses standard WOR helpers (ai_*, M_SetAnimation, monster_jump_finished, etc).

==============================================================================
*/
#include "../g_local.hpp"
#include "m_spawn.hpp"

/*
===============
Config
===============
*/
static const int   SPAWN_DAMAGE_MIN = 40;
static const int   SPAWN_DAMAGE_MAX = 60;
static const float SPAWN_SPLASH_RADIUS = 120.0f;
static const float SPAWN_LEAP_SPEED_F = 520.0f;
static const float SPAWN_LEAP_SPEED_UP = 320.0f;

/*
===============
Sounds
===============
*/
static cached_soundIndex snd_idle;
static cached_soundIndex snd_sight;
static cached_soundIndex snd_search;
static cached_soundIndex snd_pain;
static cached_soundIndex snd_jump;
static cached_soundIndex snd_explode;

/*
===============
Forward decls
===============
*/
static void spawn_run(gentity_t* self);

/*
===============
spawn_idle
===============
*/
static void spawn_idle(gentity_t* self) {
	if (frandom() > 0.7f)
		gi.sound(self, CHAN_VOICE, snd_idle, 1, ATTN_IDLE, 0);
}

/*
===============
spawn_touch_detonate
Called while leaping; blows up on touching a valid target or world at high speed.
===============
*/
static TOUCH(spawn_touch_detonate) (gentity_t* self, gentity_t* other, const trace_t& tr, bool /*otherTouchingSelf*/) -> void {
	// prevent chain reentry
	if (!self->inUse || self->deadFlag)
		return;

	// ignore touches with the owner or non-solid debris
	if (other && (other == self || other == self->owner))
		return;

	// detonate only once we left ground (during leap) or on solid impact
	if (self->groundEntity && (!other || !other->takeDamage))
		return;

	// detonate
	const int dmg = irandom(SPAWN_DAMAGE_MIN, SPAWN_DAMAGE_MAX);
	Vector3   origin = self->s.origin;

	gi.sound(self, CHAN_WEAPON, snd_explode, 1, ATTN_NORM, 0);

	// visual effect (use generic explosion if no tar splash effect exists)
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(origin);
	gi.multicast(origin, MULTICAST_PVS, false);

	// radius damage (self as inflictor and attacker)
	RadiusDamage(self, self, dmg, self, SPAWN_SPLASH_RADIUS, DamageFlags::Normal, ModID::Explosives);

	// kill self softly; mark dead and drop a tiny chunk
	self->health = 0;
	self->deadFlag = true;
	self->takeDamage = false;
	self->svFlags |= SVF_DEADMONSTER;

	// shrink bbox so actors do not snag
	self->mins = { -8, -8, -8 };
	self->maxs = { 8,  8,  0 };
	gi.linkEntity(self);

	// remove entity soon
	self->think = FreeEntity;
	self->nextThink = level.time + 0.1_sec;
}

/*
===============
spawn_stand
===============
*/
static MonsterFrame spawn_frames_stand[] = {
	{ ai_stand, 0, spawn_idle },
	{ ai_stand },{ ai_stand },{ ai_stand },{ ai_stand },
	{ ai_stand },{ ai_stand },{ ai_stand }
};
MMOVE_T(spawn_move_stand) = { FRAME_stand01, FRAME_stand08, spawn_frames_stand, nullptr };

MONSTERINFO_STAND(spawn_stand) (gentity_t* self) -> void {
	M_SetAnimation(self, &spawn_move_stand);
}

/*
===============
spawn_walk
===============
*/
static MonsterFrame spawn_frames_walk[] = {
	{ ai_walk, 6 },{ ai_walk, 4 },{ ai_walk, 6 },{ ai_walk, 4 },
	{ ai_walk, 6 },{ ai_walk, 4 },{ ai_walk, 6 },{ ai_walk, 4 }
};
MMOVE_T(spawn_move_walk) = { FRAME_walk01, FRAME_walk08, spawn_frames_walk, nullptr };

MONSTERINFO_WALK(spawn_walk) (gentity_t* self) -> void {
	M_SetAnimation(self, &spawn_move_walk);
}

/*
===============
spawn_run
===============
*/
static MonsterFrame spawn_frames_run[] = {
	{ ai_run, 14 },{ ai_run, 16 },{ ai_run, 18 },
	{ ai_run, 16 },{ ai_run, 18 },{ ai_run, 20 }
};
MMOVE_T(spawn_move_run) = { FRAME_run01, FRAME_run06, spawn_frames_run, nullptr };

static void spawn_run(gentity_t* self) {
	if (self->monsterInfo.aiFlags & AI_STAND_GROUND)
		M_SetAnimation(self, &spawn_move_stand);
	else
		M_SetAnimation(self, &spawn_move_run);
}

/*
===============
spawn_takeoff
===============
*/
static void spawn_takeoff(gentity_t* self) {
	Vector3 fwd;
	AngleVectors(self->s.angles, fwd, nullptr, nullptr);

	self->s.origin[Z] += 1.0f;
	self->velocity = fwd * SPAWN_LEAP_SPEED_F;
	self->velocity[2] = SPAWN_LEAP_SPEED_UP;

	self->groundEntity = nullptr;

	self->touch = spawn_touch_detonate;
	gi.sound(self, CHAN_WEAPON, snd_jump, 1, ATTN_NORM, 0);
}

/*
===============
spawn_check_land
===============
*/
static void spawn_check_land(gentity_t* self) {
	monster_jump_finished(self);

	if (self->groundEntity) {
		// if we landed without hitting a target, keep chasing
		self->touch = nullptr;
		if (self->enemy && range_to(self, self->enemy) <= RANGE_NEAR && visible(self, self->enemy)) {
			// immediate re-leap rhythm
			self->monsterInfo.nextFrame = FRAME_leap02;
		}
		else {
			M_SetAnimation(self, &spawn_move_run);
		}
	}
}

/*
===============
spawn_leap
===============
*/
static MonsterFrame spawn_frames_leap[] = {
	{ ai_charge, 0 },                 // leap01 face
	{ ai_charge, 0, spawn_takeoff },  // leap02 takeoff
	{ ai_charge, 0 },                 // leap03 flight
	{ ai_charge, 0 },                 // leap04 flight
	{ ai_charge, 0, spawn_check_land }, // leap05 poll landing
	{ ai_charge, 0 }                  // leap06
};
MMOVE_T(spawn_move_leap) = { FRAME_leap01, FRAME_leap06, spawn_frames_leap, spawn_run };

MONSTERINFO_ATTACK(spawn_attack) (gentity_t* self) -> void {
	M_SetAnimation(self, &spawn_move_leap);
}

/*
===============
spawn_checkattack
===============
*/
MONSTERINFO_CHECKATTACK(spawn_checkattack) (gentity_t* self) -> bool {
	if (!self->enemy || self->enemy->health <= 0)
		return false;

	// prefer leap at mid range, avoid extreme height differences
	if (self->absMin[2] + 128 < self->enemy->absMin[2])
		return false;

	Vector3 diff = self->enemy->s.origin - self->s.origin;
	float  dist = diff.length();

	if (dist < 64.0f) {
		// close: quick hop body-check
		self->monsterInfo.attackState = MonsterAttackState::Missile;
		return true;
	}

	if (dist <= 320.0f && visible(self, self->enemy)) {
		self->monsterInfo.attackState = MonsterAttackState::Missile;
		return true;
	}

	return false;
}

/*
===============
spawn_pain
===============
*/
static MonsterFrame spawn_frames_pain[] = {
	{ ai_move },{ ai_move },{ ai_move },{ ai_move }
};
MMOVE_T(spawn_move_pain) = { FRAME_pain01, FRAME_pain04, spawn_frames_pain, spawn_run };

static PAIN(spawn_pain) (gentity_t* self, gentity_t* other, float kick, int damage, const MeansOfDeath& mod) -> void {
	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 1.5_sec;

	gi.sound(self, CHAN_VOICE, snd_pain, 1, ATTN_NORM, 0);

	if (!M_ShouldReactToPain(self, mod))
		return;

	M_SetAnimation(self, &spawn_move_pain);
}

/*
===============
spawn_die
If killed before contact, explode anyway.
===============
*/
static MonsterFrame spawn_frames_death[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(spawn_move_death) = { FRAME_death01, FRAME_death04, spawn_frames_death, nullptr };

static void spawn_explode_now(gentity_t* self) {
	// force detonation path
	spawn_touch_detonate(self, self->enemy ? self->enemy : world, {}, false);
}

static DIE(spawn_die) (gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, const Vector3& point, const MeansOfDeath& mod) -> void {
	// tarbaby always explodes on death (unless already dead)
	if (self->deadFlag)
		return;

	// show quick detonate anim, then explode
	gi.sound(self, CHAN_WEAPON, snd_explode, 1, ATTN_NORM, 0);
	self->deadFlag = true;
	self->takeDamage = false;

	M_SetAnimation(self, &spawn_move_death);
	self->think = spawn_explode_now;
	self->nextThink = level.time + 0.05_sec;
}

/*
===============
spawn_sight
===============
*/
MONSTERINFO_SIGHT(spawn_sight) (gentity_t* self, gentity_t* other) -> void {
	gi.sound(self, CHAN_VOICE, snd_sight, 1, ATTN_NORM, 0);
}

/*
===============
spawn_search
===============
*/
MONSTERINFO_SEARCH(spawn_search) (gentity_t* self) -> void {
	gi.sound(self, CHAN_VOICE, snd_search, 1, ATTN_IDLE, 0);
}

/*
===============
spawn_setskin
===============
*/
MONSTERINFO_SETSKIN(spawn_setskin) (gentity_t* self) -> void {
	// tarbaby does not visibly wound; keep skin 0
	self->s.skinNum = 0;
}

/*
===============
SP_monster_spawn
===============
*/
/*QUAKED monster_spawn (1 0 0) (-16 -16 -16) (16 16 24) AMBUSH TRIGGER_SPAWN SIGHT x CORPSE x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
model="models/monsters/tarbaby/tris.md2"
*/
void SP_monster_spawn(gentity_t* self) {
	if (!M_AllowSpawn(self)) {
		FreeEntity(self);
		return;
	}

	// sounds
	snd_idle.assign("spawn/idle.wav");
	snd_sight.assign("spawn/sight.wav");
	snd_search.assign("spawn/search.wav");
	snd_pain.assign("spawn/pain.wav");
	snd_jump.assign("spawn/jump.wav");
	snd_explode.assign("spawn/explode.wav");

	// model and bbox
	self->moveType = MoveType::Step;
	self->solid = SOLID_BBOX;
	self->s.modelIndex = gi.modelIndex("models/monsters/tarbaby/tris.md2");
	self->mins = { -16, -16, -16 };
	self->maxs = { 16,  16,  24 };

	// stats
	self->health = self->maxHealth = 80 * st.health_multiplier;
	self->gibHealth = -60; // irrelevant; it explodes
	self->mass = 100;

	// callbacks
	self->pain = spawn_pain;
	self->die = spawn_die;

	self->monsterInfo.stand = spawn_stand;
	self->monsterInfo.walk = spawn_walk;
	self->monsterInfo.run = spawn_run;
	self->monsterInfo.melee = nullptr;
	self->monsterInfo.attack = spawn_attack;     // leap/detonate
	self->monsterInfo.checkAttack = spawn_checkattack;
	self->monsterInfo.sight = spawn_sight;
	self->monsterInfo.search = spawn_search;
	self->monsterInfo.setSkin = spawn_setskin;

	gi.linkEntity(self);

	M_SetAnimation(self, &spawn_move_stand);
	self->monsterInfo.scale = MODEL_SCALE;

	// aggressive melee style
	self->monsterInfo.combatStyle = CombatStyle::Melee;

	walkmonster_start(self);
}
