// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

flyer

==============================================================================
*/

#include "../g_local.hpp"
#include "m_flyer.hpp"
#include "m_flash.hpp"

static cached_soundIndex sound_sight;
static cached_soundIndex sound_idle;
static cached_soundIndex sound_pain1;
static cached_soundIndex sound_pain2;
static cached_soundIndex sound_slash;
static cached_soundIndex sound_sproing;
static cached_soundIndex sound_die;

void flyer_check_melee(gentity_t *self);
void flyer_loop_melee(gentity_t *self);

void flyer_kamikaze(gentity_t *self);
void flyer_kamikaze_check(gentity_t *self);
void flyer_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, const Vector3 &point, const MeansOfDeath &mod);

MONSTERINFO_SIGHT(flyer_sight) (gentity_t *self, gentity_t *other) -> void {
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

MONSTERINFO_IDLE(flyer_idle) (gentity_t *self) -> void {
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

static void flyer_pop_blades(gentity_t *self) {
	gi.sound(self, CHAN_VOICE, sound_sproing, 1, ATTN_NORM, 0);
}

MonsterFrame flyer_frames_stand[] = {
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }
};
MMOVE_T(flyer_move_stand) = { FRAME_stand01, FRAME_stand45, flyer_frames_stand, nullptr };

MonsterFrame flyer_frames_walk[] = {
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 }
};
MMOVE_T(flyer_move_walk) = { FRAME_stand01, FRAME_stand45, flyer_frames_walk, nullptr };

MonsterFrame flyer_frames_run[] = {
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 }
};
MMOVE_T(flyer_move_run) = { FRAME_stand01, FRAME_stand45, flyer_frames_run, nullptr };

MonsterFrame flyer_frames_kamizake[] = {
	{ ai_charge, 40, flyer_kamikaze_check },
	{ ai_charge, 40, flyer_kamikaze_check },
	{ ai_charge, 40, flyer_kamikaze_check },
	{ ai_charge, 40, flyer_kamikaze_check },
	{ ai_charge, 40, flyer_kamikaze_check }
};
MMOVE_T(flyer_move_kamikaze) = { FRAME_rollr02, FRAME_rollr06, flyer_frames_kamizake, flyer_kamikaze };

MONSTERINFO_RUN(flyer_run) (gentity_t *self) -> void {
	if (self->mass > 50)
		M_SetAnimation(self, &flyer_move_kamikaze);
	else if (self->monsterInfo.aiFlags & AI_STAND_GROUND)
		M_SetAnimation(self, &flyer_move_stand);
	else
		M_SetAnimation(self, &flyer_move_run);
}

MONSTERINFO_WALK(flyer_walk) (gentity_t *self) -> void {
	if (self->mass > 50)
		flyer_run(self);
	else
		M_SetAnimation(self, &flyer_move_walk);
}

MONSTERINFO_STAND(flyer_stand) (gentity_t *self) -> void {
	if (self->mass > 50)
		flyer_run(self);
	else
		M_SetAnimation(self, &flyer_move_stand);
}

// kamikaze stuff
static void flyer_kamikaze_explode(gentity_t *self) {
	Vector3 dir;

	if (self->monsterInfo.commander && self->monsterInfo.commander->inUse &&
		!strcmp(self->monsterInfo.commander->className, "monster_carrier"))
		self->monsterInfo.commander->monsterInfo.monster_slots++;

	if (self->enemy) {
		dir = self->enemy->s.origin - self->s.origin;
		Damage(self->enemy, self, self, dir, self->s.origin, vec3_origin, (int)50, (int)50, DamageFlags::Radius, ModID::Unknown);
	}

	flyer_die(self, nullptr, nullptr, 0, dir, ModID::Explosives);
}

void flyer_kamikaze(gentity_t *self) {
	M_SetAnimation(self, &flyer_move_kamikaze);
}

void flyer_kamikaze_check(gentity_t *self) {
	float dist;

	// PMM - this needed because we could have gone away before we get here (blocked code)
	if (!self->inUse)
		return;

	if ((!self->enemy) || (!self->enemy->inUse)) {
		flyer_kamikaze_explode(self);
		return;
	}

	self->s.angles[PITCH] = VectorToAngles(self->enemy->s.origin - self->s.origin).x;

	self->goalEntity = self->enemy;

	dist = realrange(self, self->enemy);

	if (dist < 90)
		flyer_kamikaze_explode(self);
}

MonsterFrame flyer_frames_pain3[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(flyer_move_pain3) = { FRAME_pain301, FRAME_pain304, flyer_frames_pain3, flyer_run };

MonsterFrame flyer_frames_pain2[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(flyer_move_pain2) = { FRAME_pain201, FRAME_pain204, flyer_frames_pain2, flyer_run };

MonsterFrame flyer_frames_pain1[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(flyer_move_pain1) = { FRAME_pain101, FRAME_pain109, flyer_frames_pain1, flyer_run };

static void flyer_fire(gentity_t *self, MonsterMuzzleFlashID flash_number) {
	Vector3	  start;
	Vector3	  forward, right;
	Vector3	  end;
	Vector3	  dir;

	if (!self->enemy || !self->enemy->inUse)
		return;

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, monster_flash_offset[flash_number], forward, right);

	end = self->enemy->s.origin;
	end[2] += self->enemy->viewHeight;
	dir = end - start;
	dir.normalize();

	monster_fire_blaster(self, start, dir, 1, 1000, flash_number, (self->s.frame % 4) ? EF_NONE : EF_HYPERBLASTER);
}

static void flyer_fireleft(gentity_t *self) {
	flyer_fire(self, MZ2_FLYER_BLASTER_1);
}

static void flyer_fireright(gentity_t *self) {
	flyer_fire(self, MZ2_FLYER_BLASTER_2);
}

MonsterFrame flyer_frames_attack2[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, -10, flyer_fireleft },	 // left gun
	{ ai_charge, -10, flyer_fireright }, // right gun
	{ ai_charge, -10, flyer_fireleft },	 // left gun
	{ ai_charge, -10, flyer_fireright }, // right gun
	{ ai_charge, -10, flyer_fireleft },	 // left gun
	{ ai_charge, -10, flyer_fireright }, // right gun
	{ ai_charge, -10, flyer_fireleft },	 // left gun
	{ ai_charge, -10, flyer_fireright }, // right gun
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }
};
MMOVE_T(flyer_move_attack2) = { FRAME_attack201, FRAME_attack217, flyer_frames_attack2, flyer_run };

// circle strafe frames
MonsterFrame flyer_frames_attack3[] = {
	{ ai_charge, 10 },
	{ ai_charge, 10 },
	{ ai_charge, 10 },
	{ ai_charge, 10, flyer_fireleft },	// left gun
	{ ai_charge, 10, flyer_fireright }, // right gun
	{ ai_charge, 10, flyer_fireleft },	// left gun
	{ ai_charge, 10, flyer_fireright }, // right gun
	{ ai_charge, 10, flyer_fireleft },	// left gun
	{ ai_charge, 10, flyer_fireright }, // right gun
	{ ai_charge, 10, flyer_fireleft },	// left gun
	{ ai_charge, 10, flyer_fireright }, // right gun
	{ ai_charge, 10 },
	{ ai_charge, 10 },
	{ ai_charge, 10 },
	{ ai_charge, 10 },
	{ ai_charge, 10 },
	{ ai_charge, 10 }
};
MMOVE_T(flyer_move_attack3) = { FRAME_attack201, FRAME_attack217, flyer_frames_attack3, flyer_run };

static void flyer_slash_left(gentity_t *self) {
	Vector3 aim = { MELEE_DISTANCE, self->mins[0], 0 };
	if (!fire_hit(self, aim, 5, 0))
		self->monsterInfo.melee_debounce_time = level.time + 1.5_sec;
	gi.sound(self, CHAN_WEAPON, sound_slash, 1, ATTN_NORM, 0);
}

static void flyer_slash_right(gentity_t *self) {
	Vector3 aim = { MELEE_DISTANCE, self->maxs[0], 0 };
	if (!fire_hit(self, aim, 5, 0))
		self->monsterInfo.melee_debounce_time = level.time + 1.5_sec;
	gi.sound(self, CHAN_WEAPON, sound_slash, 1, ATTN_NORM, 0);
}

MonsterFrame flyer_frames_start_melee[] = {
	{ ai_charge, 0, flyer_pop_blades },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }
};
MMOVE_T(flyer_move_start_melee) = { FRAME_attack101, FRAME_attack106, flyer_frames_start_melee, flyer_loop_melee };

MonsterFrame flyer_frames_end_melee[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }
};
MMOVE_T(flyer_move_end_melee) = { FRAME_attack119, FRAME_attack121, flyer_frames_end_melee, flyer_run };

MonsterFrame flyer_frames_loop_melee[] = {
	{ ai_charge }, // Loop Start
	{ ai_charge },
	{ ai_charge, 0, flyer_slash_left }, // Left Wing Strike
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, flyer_slash_right }, // Right Wing Strike
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge } // Loop Ends

};
MMOVE_T(flyer_move_loop_melee) = { FRAME_attack107, FRAME_attack118, flyer_frames_loop_melee, flyer_check_melee };

void flyer_loop_melee(gentity_t *self) {
	M_SetAnimation(self, &flyer_move_loop_melee);
}

static void flyer_set_fly_parameters(gentity_t *self, bool melee) {
	if (melee) {
		// engage thrusters for a slice
		self->monsterInfo.fly_pinned = false;
		self->monsterInfo.fly_thrusters = true;
		self->monsterInfo.fly_position_time = 0_ms;
		self->monsterInfo.fly_acceleration = 20.f;
		self->monsterInfo.fly_speed = 210.f;
		self->monsterInfo.fly_min_distance = 0.f;
		self->monsterInfo.fly_max_distance = 10.f;
	} else {
		self->monsterInfo.fly_thrusters = false;
		self->monsterInfo.fly_acceleration = 15.f;
		self->monsterInfo.fly_speed = 165.f;
		self->monsterInfo.fly_min_distance = 45.f;
		self->monsterInfo.fly_max_distance = 200.f;
	}
}

MONSTERINFO_ATTACK(flyer_attack) (gentity_t *self) -> void {
	if (self->mass > 50) {
		flyer_run(self);
		return;
	}

	float range = range_to(self, self->enemy);

	if (self->enemy && visible(self, self->enemy) && range <= 225.f && frandom() > (range / 225.f) * 0.35f) {
		// fly-by slicing!
		self->monsterInfo.attackState = MonsterAttackState::Straight;
		M_SetAnimation(self, &flyer_move_start_melee);
		flyer_set_fly_parameters(self, true);
	} else {
		self->monsterInfo.attackState = MonsterAttackState::Straight;
		M_SetAnimation(self, &flyer_move_attack2);
	}

	// [Paril-KEX] for alternate fly mode, sometimes we'll pin us
	// down, kind of like a pseudo-stand ground
	if (!self->monsterInfo.fly_pinned && brandom() && self->enemy && visible(self, self->enemy)) {
		self->monsterInfo.fly_pinned = true;
		self->monsterInfo.fly_position_time = max(self->monsterInfo.fly_position_time, self->monsterInfo.fly_position_time + 1.7_sec); // make sure there's enough time for attack2/3

		if (brandom())
			self->monsterInfo.fly_ideal_position = self->s.origin + (self->velocity * frandom()); // pin to our current position
		else
			self->monsterInfo.fly_ideal_position += self->enemy->s.origin; // make un-relative
	}

	// if we're currently pinned, fly_position_time will unpin us eventually
}

MONSTERINFO_MELEE(flyer_melee) (gentity_t *self) -> void {
	if (self->mass > 50)
		flyer_run(self);
	else {
		M_SetAnimation(self, &flyer_move_start_melee);
		flyer_set_fly_parameters(self, true);
	}
}

void flyer_check_melee(gentity_t *self) {
	if (range_to(self, self->enemy) <= RANGE_MELEE) {
		if (self->monsterInfo.melee_debounce_time <= level.time) {
			M_SetAnimation(self, &flyer_move_loop_melee);
			return;
		}
	}

	M_SetAnimation(self, &flyer_move_end_melee);
	flyer_set_fly_parameters(self, false);
}

static PAIN(flyer_pain) (gentity_t *self, gentity_t *other, float kick, int damage, const MeansOfDeath &mod) -> void {
	int n;

	//	pmm	 - kamikaze's don't feel pain
	if (self->mass != 50)
		return;
	// pmm

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3_sec;

	n = irandom(3);
	if (n == 0)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else if (n == 1)
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);

	if (!M_ShouldReactToPain(self, mod))
		return; // no pain anims in nightmare

	flyer_set_fly_parameters(self, false);

	if (n == 0)
		M_SetAnimation(self, &flyer_move_pain1);
	else if (n == 1)
		M_SetAnimation(self, &flyer_move_pain2);
	else
		M_SetAnimation(self, &flyer_move_pain3);
}

MONSTERINFO_SETSKIN(flyer_setskin) (gentity_t *self) -> void {
	if (self->health < (self->maxHealth / 2))
		self->s.skinNum = 1;
	else
		self->s.skinNum = 0;
}

DIE(flyer_die) (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, const Vector3 &point, const MeansOfDeath &mod) -> void {
	gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PHS, false);

	self->s.skinNum /= 2;

	ThrowGibs(self, 55, {
		{ 2, "models/objects/gibs/sm_metal/tris.md2" },
		{ 2, "models/objects/gibs/sm_meat/tris.md2" },
		{ "models/monsters/flyer/gibs/base.md2", GIB_SKINNED },
		{ 2, "models/monsters/flyer/gibs/gun.md2", GIB_SKINNED },
		{ 2, "models/monsters/flyer/gibs/wing.md2", GIB_SKINNED },
		{ "models/monsters/flyer/gibs/head.md2", GIB_SKINNED | GIB_HEAD }
		});

	self->touch = nullptr;
}

// kamikaze code .. blow up if blocked
MONSTERINFO_BLOCKED(flyer_blocked) (gentity_t *self, float dist) -> bool {
	// kamikaze = 100, normal = 50
	if (self->mass == 100) {
		flyer_kamikaze_check(self);

		// if the above didn't blow us up (i.e. I got blocked by the player)
		if (self->inUse)
			Damage(self, self, self, vec3_origin, self->s.origin, vec3_origin, 9999, 100, DamageFlags::Normal, ModID::Unknown);

		return true;
	}

	return false;
}

static TOUCH(kamikaze_touch) (gentity_t *ent, gentity_t *other, const trace_t &tr, bool otherTouchingSelf) -> void {
	Damage(ent, ent, ent, ent->velocity.normalized(), ent->s.origin, ent->velocity.normalized(), 9999, 100, DamageFlags::Normal, ModID::Unknown);
}

static TOUCH(flyer_touch) (gentity_t *ent, gentity_t *other, const trace_t &tr, bool otherTouchingSelf) -> void {
	if ((other->monsterInfo.aiFlags & AI_ALTERNATE_FLY) && (other->flags & FL_FLY) &&
		(ent->monsterInfo.duck_wait_time < level.time)) {
		ent->monsterInfo.duck_wait_time = level.time + 1_sec;
		ent->monsterInfo.fly_thrusters = false;

		Vector3 dir = (ent->s.origin - other->s.origin).normalized();
		ent->velocity = dir * 500.f;

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_SPLASH);
		gi.WriteByte(32);
		gi.WritePosition(tr.endPos);
		gi.WriteDir(dir);
		gi.WriteByte(SPLASH_SPARKS);
		gi.multicast(tr.endPos, MULTICAST_PVS, false);
	}
}

/*QUAKED monster_flyer (1 .5 0) (-16 -16 -24) (16 16 32) AMBUSH TRIGGER_SPAWN SIGHT x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
 */
void SP_monster_flyer(gentity_t *self) {
	if (!M_AllowSpawn(self)) {
		FreeEntity(self);
		return;
	}

	sound_sight.assign("flyer/flysght1.wav");
	sound_idle.assign("flyer/flysrch1.wav");
	sound_pain1.assign("flyer/flypain1.wav");
	sound_pain2.assign("flyer/flypain2.wav");
	sound_slash.assign("flyer/flyatck2.wav");
	sound_sproing.assign("flyer/flyatck1.wav");
	sound_die.assign("flyer/flydeth1.wav");

	gi.soundIndex("flyer/flyatck3.wav");

	self->s.modelIndex = gi.modelIndex("models/monsters/flyer/tris.md2");

	gi.modelIndex("models/monsters/flyer/gibs/base.md2");
	gi.modelIndex("models/monsters/flyer/gibs/wing.md2");
	gi.modelIndex("models/monsters/flyer/gibs/gun.md2");
	gi.modelIndex("models/monsters/flyer/gibs/head.md2");

	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, 16 };
	self->moveType = MoveType::Step;
	self->solid = SOLID_BBOX;

	self->viewHeight = 12;

	self->monsterInfo.engineSound = gi.soundIndex("flyer/flyidle1.wav");

	self->health = 50 * st.health_multiplier;
	self->mass = 50;

	self->pain = flyer_pain;
	self->die = flyer_die;

	self->monsterInfo.stand = flyer_stand;
	self->monsterInfo.walk = flyer_walk;
	self->monsterInfo.run = flyer_run;
	self->monsterInfo.attack = flyer_attack;
	self->monsterInfo.melee = flyer_melee;
	self->monsterInfo.sight = flyer_sight;
	self->monsterInfo.idle = flyer_idle;
	self->monsterInfo.blocked = flyer_blocked;
	self->monsterInfo.setSkin = flyer_setskin;

	gi.linkEntity(self);

	M_SetAnimation(self, &flyer_move_stand);
	self->monsterInfo.scale = MODEL_SCALE;

	if (self->s.effects & EF_ROCKET) {
		// PMM - normal flyer has mass of 50
		self->mass = 100;
		self->yawSpeed = 5;
		self->touch = kamikaze_touch;
	} else {
		self->monsterInfo.aiFlags |= AI_ALTERNATE_FLY;
		self->monsterInfo.fly_buzzard = true;
		flyer_set_fly_parameters(self, false);
		self->touch = flyer_touch;
	}

	flymonster_start(self);
}

// suicide fliers
void SP_monster_kamikaze(gentity_t *self) {
	if (!M_AllowSpawn(self)) {
		FreeEntity(self);
		return;
	}

	self->s.effects |= EF_ROCKET;

	SP_monster_flyer(self);
}
