// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

floater

==============================================================================
*/

#include "../g_local.hpp"
#include "m_float.hpp"
#include "m_flash.hpp"

static cached_soundIndex sound_attack2;
static cached_soundIndex sound_attack3;
static cached_soundIndex sound_death1;
static cached_soundIndex sound_idle;
static cached_soundIndex sound_pain1;
static cached_soundIndex sound_pain2;
static cached_soundIndex sound_sight;

MONSTERINFO_SIGHT(floater_sight) (gentity_t *self, gentity_t *other) -> void {
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

MONSTERINFO_IDLE(floater_idle) (gentity_t *self) -> void {
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

void floater_dead(gentity_t *self);
void floater_run(gentity_t *self);
void floater_wham(gentity_t *self);
void floater_zap(gentity_t *self);

static void floater_fire_blaster(gentity_t *self) {
	Vector3	  start;
	Vector3	  forward, right;
	Vector3	  end;
	Vector3	  dir;

	if (!self->enemy || !self->enemy->inUse)
		return;

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_FLOAT_BLASTER_1], forward, right);

	end = self->enemy->s.origin;
	end[2] += self->enemy->viewHeight;
	dir = end - start;
	dir.normalize();

	monster_fire_blaster(self, start, dir, 1, 1000, MZ2_FLOAT_BLASTER_1, (self->s.frame % 4) ? EF_NONE : EF_HYPERBLASTER);
}

MonsterFrame floater_frames_stand1[] = {
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
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }
};
MMOVE_T(floater_move_stand1) = { FRAME_stand101, FRAME_stand152, floater_frames_stand1, nullptr };

MonsterFrame floater_frames_stand2[] = {
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
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }
};
MMOVE_T(floater_move_stand2) = { FRAME_stand201, FRAME_stand252, floater_frames_stand2, nullptr };

MonsterFrame floater_frames_pop[] = {
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{},
	{}
};
MMOVE_T(floater_move_pop) = { FRAME_actvat05, FRAME_actvat31, floater_frames_pop, floater_run };

MonsterFrame floater_frames_disguise[] = {
	{ ai_stand }
};
MMOVE_T(floater_move_disguise) = { FRAME_actvat01, FRAME_actvat01, floater_frames_disguise, nullptr };

MONSTERINFO_STAND(floater_stand) (gentity_t *self) -> void {
	if (self->monsterInfo.active_move == &floater_move_disguise)
		M_SetAnimation(self, &floater_move_disguise);
	else if (frandom() <= 0.5f)
		M_SetAnimation(self, &floater_move_stand1);
	else
		M_SetAnimation(self, &floater_move_stand2);
}

MonsterFrame floater_frames_attack1[] = {
	{ ai_charge }, // Blaster attack
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, floater_fire_blaster }, // BOOM (0, -25.8, 32.5)	-- LOOP Starts
	{ ai_charge, 0, floater_fire_blaster },
	{ ai_charge, 0, floater_fire_blaster },
	{ ai_charge, 0, floater_fire_blaster },
	{ ai_charge, 0, floater_fire_blaster },
	{ ai_charge, 0, floater_fire_blaster },
	{ ai_charge, 0, floater_fire_blaster },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge } //							-- LOOP Ends
};
MMOVE_T(floater_move_attack1) = { FRAME_attack101, FRAME_attack114, floater_frames_attack1, floater_run };

// circle strafe frames
MonsterFrame floater_frames_attack1a[] = {
	{ ai_charge, 10 }, // Blaster attack
	{ ai_charge, 10 },
	{ ai_charge, 10 },
	{ ai_charge, 10, floater_fire_blaster }, // BOOM (0, -25.8, 32.5)	-- LOOP Starts
	{ ai_charge, 10, floater_fire_blaster },
	{ ai_charge, 10, floater_fire_blaster },
	{ ai_charge, 10, floater_fire_blaster },
	{ ai_charge, 10, floater_fire_blaster },
	{ ai_charge, 10, floater_fire_blaster },
	{ ai_charge, 10, floater_fire_blaster },
	{ ai_charge, 10 },
	{ ai_charge, 10 },
	{ ai_charge, 10 },
	{ ai_charge, 10 } //							-- LOOP Ends
};
MMOVE_T(floater_move_attack1a) = { FRAME_attack101, FRAME_attack114, floater_frames_attack1a, floater_run };

MonsterFrame floater_frames_attack2[] = {
	{ ai_charge }, // Claws
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, floater_wham }, // WHAM (0, -45, 29.6)		-- LOOP Starts
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }, //							-- LOOP Ends
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }
};
MMOVE_T(floater_move_attack2) = { FRAME_attack201, FRAME_attack225, floater_frames_attack2, floater_run };

MonsterFrame floater_frames_attack3[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, floater_zap }, //								-- LOOP Starts
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }, //								-- LOOP Ends
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }
};
MMOVE_T(floater_move_attack3) = { FRAME_attack301, FRAME_attack334, floater_frames_attack3, floater_run };

MonsterFrame floater_frames_pain1[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(floater_move_pain1) = { FRAME_pain101, FRAME_pain107, floater_frames_pain1, floater_run };

MonsterFrame floater_frames_pain2[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(floater_move_pain2) = { FRAME_pain201, FRAME_pain208, floater_frames_pain2, floater_run };

MonsterFrame floater_frames_walk[] = {
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
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 5 }
};
MMOVE_T(floater_move_walk) = { FRAME_stand101, FRAME_stand152, floater_frames_walk, nullptr };

MonsterFrame floater_frames_run[] = {
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 },
	{ ai_run, 13 }
};
MMOVE_T(floater_move_run) = { FRAME_stand101, FRAME_stand152, floater_frames_run, nullptr };

MONSTERINFO_RUN(floater_run) (gentity_t *self) -> void {
	if (self->monsterInfo.active_move == &floater_move_disguise)
		M_SetAnimation(self, &floater_move_pop);
	else if (self->monsterInfo.aiFlags & AI_STAND_GROUND)
		M_SetAnimation(self, &floater_move_stand1);
	else
		M_SetAnimation(self, &floater_move_run);
}

MONSTERINFO_WALK(floater_walk) (gentity_t *self) -> void {
	M_SetAnimation(self, &floater_move_walk);
}

void floater_wham(gentity_t *self) {
	constexpr Vector3 aim = { MELEE_DISTANCE, 0, 0 };
	gi.sound(self, CHAN_WEAPON, sound_attack3, 1, ATTN_NORM, 0);

	if (!fire_hit(self, aim, irandom(5, 11), -50))
		self->monsterInfo.melee_debounce_time = level.time + 3_sec;
}

void floater_zap(gentity_t *self) {
	Vector3 forward, right;
	Vector3 origin;
	Vector3 dir;
	Vector3 offset;

	dir = self->enemy->s.origin - self->s.origin;

	AngleVectors(self->s.angles, forward, right, nullptr);
	// FIXME use a flash and replace these two lines with the commented one
	offset = { 18.5f, -0.9f, 10 };
	origin = M_ProjectFlashSource(self, offset, forward, right);

	gi.sound(self, CHAN_WEAPON, sound_attack2, 1, ATTN_NORM, 0);

	// FIXME use the flash, Luke
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_SPLASH);
	gi.WriteByte(32);
	gi.WritePosition(origin);
	gi.WriteDir(dir);
	gi.WriteByte(SPLASH_SPARKS);
	gi.multicast(origin, MULTICAST_PVS, false);

	Damage(self->enemy, self, self, dir, self->enemy->s.origin, vec3_origin, irandom(5, 11), -10, DamageFlags::Energy, ModID::Unknown);
}

MONSTERINFO_ATTACK(floater_attack) (gentity_t *self) -> void {
	float chance = 0.5f;

	if (frandom() > chance) {
		self->monsterInfo.attackState = MonsterAttackState::Straight;
		M_SetAnimation(self, &floater_move_attack1);
	} else // circle strafe
	{
		if (frandom() <= 0.5f) // switch directions
			self->monsterInfo.lefty = !self->monsterInfo.lefty;
		self->monsterInfo.attackState = MonsterAttackState::Sliding;
		M_SetAnimation(self, &floater_move_attack1a);
	}
}

MONSTERINFO_MELEE(floater_melee) (gentity_t *self) -> void {
	if (frandom() < 0.5f)
		M_SetAnimation(self, &floater_move_attack3);
	else
		M_SetAnimation(self, &floater_move_attack2);
}

static PAIN(floater_pain) (gentity_t *self, gentity_t *other, float kick, int damage, const MeansOfDeath &mod) -> void {
	int n;

	if (level.time < self->pain_debounce_time)
		return;

	// no pain anims if poppin'
	if (self->monsterInfo.active_move == &floater_move_disguise ||
		self->monsterInfo.active_move == &floater_move_pop)
		return;

	n = irandom(3);
	if (n == 0)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

	self->pain_debounce_time = level.time + 3_sec;

	if (!M_ShouldReactToPain(self, mod))
		return; // no pain anims in nightmare

	if (n == 0)
		M_SetAnimation(self, &floater_move_pain1);
	else
		M_SetAnimation(self, &floater_move_pain2);
}

MONSTERINFO_SETSKIN(floater_setskin) (gentity_t *self) -> void {
	if (self->health < (self->maxHealth / 2))
		self->s.skinNum = 1;
	else
		self->s.skinNum = 0;
}

void floater_dead(gentity_t *self) {
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, -8 };
	self->moveType = MoveType::Toss;
	self->svFlags |= SVF_DEADMONSTER;
	self->nextThink = 0_ms;
	gi.linkEntity(self);
}

static DIE(floater_die) (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, const Vector3 &point, const MeansOfDeath &mod) -> void {
	gi.sound(self, CHAN_VOICE, sound_death1, 1, ATTN_NORM, 0);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PHS, false);

	self->s.skinNum /= 2;

	ThrowGibs(self, 55, {
		{ 2, "models/objects/gibs/sm_metal/tris.md2" },
		{ 3, "models/objects/gibs/sm_meat/tris.md2" },
		{ "models/monsters/float/gibs/piece.md2", GIB_SKINNED },
		{ "models/monsters/float/gibs/gun.md2", GIB_SKINNED },
		{ "models/monsters/float/gibs/base.md2", GIB_SKINNED },
		{ "models/monsters/float/gibs/jar.md2", GIB_SKINNED | GIB_HEAD }
		});
}

static void float_set_fly_parameters(gentity_t *self) {
	self->monsterInfo.fly_thrusters = false;
	self->monsterInfo.fly_acceleration = 10.f;
	self->monsterInfo.fly_speed = 100.f;
	// Technician gets in closer because he has two melee attacks
	self->monsterInfo.fly_min_distance = 20.f;
	self->monsterInfo.fly_max_distance = 200.f;
}

constexpr SpawnFlags SPAWNFLAG_FLOATER_DISGUISE = 8_spawnflag;

/*QUAKED monster_floater (1 .5 0) (-16 -16 -24) (16 16 32) AMBUSH TRIGGER_SPAWN SIGHT DISGUISE x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
 */
void SP_monster_floater(gentity_t *self) {
	if (!M_AllowSpawn(self)) {
		FreeEntity(self);
		return;
	}

	sound_attack2.assign("floater/fltatck2.wav");
	sound_attack3.assign("floater/fltatck3.wav");
	sound_death1.assign("floater/fltdeth1.wav");
	sound_idle.assign("floater/fltidle1.wav");
	sound_pain1.assign("floater/fltpain1.wav");
	sound_pain2.assign("floater/fltpain2.wav");
	sound_sight.assign("floater/fltsght1.wav");

	gi.soundIndex("floater/fltatck1.wav");

	self->monsterInfo.engineSound = gi.soundIndex("floater/fltsrch1.wav");

	self->moveType = MoveType::Step;
	self->solid = SOLID_BBOX;
	self->s.modelIndex = gi.modelIndex("models/monsters/float/tris.md2");

	gi.modelIndex("models/monsters/float/gibs/base.md2");
	gi.modelIndex("models/monsters/float/gibs/gun.md2");
	gi.modelIndex("models/monsters/float/gibs/jar.md2");
	gi.modelIndex("models/monsters/float/gibs/piece.md2");

	self->mins = { -24, -24, -24 };
	self->maxs = { 24, 24, 48 };

	self->health = 200 * st.health_multiplier;
	self->gibHealth = -80;
	self->mass = 300;

	self->pain = floater_pain;
	self->die = floater_die;

	self->monsterInfo.stand = floater_stand;
	self->monsterInfo.walk = floater_walk;
	self->monsterInfo.run = floater_run;
	self->monsterInfo.attack = floater_attack;
	self->monsterInfo.melee = floater_melee;
	self->monsterInfo.sight = floater_sight;
	self->monsterInfo.idle = floater_idle;
	self->monsterInfo.setSkin = floater_setskin;

	gi.linkEntity(self);

	if (self->spawnFlags.has(SPAWNFLAG_FLOATER_DISGUISE))
		M_SetAnimation(self, &floater_move_disguise);
	else if (frandom() <= 0.5f)
		M_SetAnimation(self, &floater_move_stand1);
	else
		M_SetAnimation(self, &floater_move_stand2);

	self->monsterInfo.scale = MODEL_SCALE;

	self->monsterInfo.aiFlags |= AI_ALTERNATE_FLY;
	float_set_fly_parameters(self);

	flymonster_start(self);
}
