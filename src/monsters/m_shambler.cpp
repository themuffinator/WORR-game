// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

SHAMBLER

==============================================================================
*/

#include "../g_local.hpp"
#include "m_shambler.hpp"
#include "m_flash.hpp"

static cached_soundIndex sound_pain;
static cached_soundIndex sound_idle;
static cached_soundIndex sound_die;
static cached_soundIndex sound_sight;
static cached_soundIndex sound_windup;
static cached_soundIndex sound_melee1;
static cached_soundIndex sound_melee2;
static cached_soundIndex sound_smack;
static cached_soundIndex sound_boom;

//
// misc
//

MONSTERINFO_SIGHT(shambler_sight) (gentity_t *self, gentity_t *other) -> void {
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

constexpr Vector3 lightning_left_hand[] = {
	{ 44, 36, 25 },
	{ 10, 44, 57 },
	{ -1, 40, 70 },
	{ -10, 34, 75 },
	{ 7.4f, 24, 89 }
};

constexpr Vector3 lightning_right_hand[] = {
	{ 28, -38, 25 },
	{ 31, -7, 70 },
	{ 20, 0, 80 },
	{ 16, 1.2f, 81 },
	{ 27, -11, 83 }
};

static void shambler_lightning_update(gentity_t *self) {
	gentity_t *lightning = self->beam;

	if (self->s.frame >= FRAME_magic01 + q_countof(lightning_left_hand)) {
		FreeEntity(lightning);
		self->beam = nullptr;
		return;
	}

	Vector3 f, r;
	AngleVectors(self->s.angles, f, r, nullptr);
	lightning->s.origin = M_ProjectFlashSource(self, lightning_left_hand[self->s.frame - FRAME_magic01], f, r);
	lightning->s.oldOrigin = M_ProjectFlashSource(self, lightning_right_hand[self->s.frame - FRAME_magic01], f, r);
	gi.linkEntity(lightning);
}

static void shambler_windup(gentity_t *self) {
	gi.sound(self, CHAN_WEAPON, sound_windup, 1, ATTN_NORM, 0);

	gentity_t *lightning = self->beam = Spawn();
	lightning->s.modelIndex = gi.modelIndex("models/proj/lightning/tris.md2");
	lightning->s.renderFX |= RF_BEAM;
	lightning->owner = self;
	shambler_lightning_update(self);
}

MONSTERINFO_IDLE(shambler_idle) (gentity_t *self) -> void {
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

static void shambler_maybe_idle(gentity_t *self) {
	if (frandom() > 0.8)
		gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

//
// stand
//

MonsterFrame shambler_frames_stand[] = {
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
MMOVE_T(shambler_move_stand) = { FRAME_stand01, FRAME_stand17, shambler_frames_stand, nullptr };

MONSTERINFO_STAND(shambler_stand) (gentity_t *self) -> void {
	M_SetAnimation(self, &shambler_move_stand);
}

//
// walk
//

void shambler_walk(gentity_t *self);

MonsterFrame shambler_frames_walk[] = {
	{ ai_walk, 10 }, // FIXME: add footsteps?
	{ ai_walk, 9 },
	{ ai_walk, 9 },
	{ ai_walk, 5 },
	{ ai_walk, 6 },
	{ ai_walk, 12 },
	{ ai_walk, 8 },
	{ ai_walk, 3 },
	{ ai_walk, 13 },
	{ ai_walk, 9 },
	{ ai_walk, 7, shambler_maybe_idle },
	{ ai_walk, 5 },
};
MMOVE_T(shambler_move_walk) = { FRAME_walk01, FRAME_walk12, shambler_frames_walk, nullptr };

MONSTERINFO_WALK(shambler_walk) (gentity_t *self) -> void {
	M_SetAnimation(self, &shambler_move_walk);
}

//
// run
//

void shambler_run(gentity_t *self);

MonsterFrame shambler_frames_run[] = {
	{ ai_run, 20 }, // FIXME: add footsteps?
	{ ai_run, 24 },
	{ ai_run, 20 },
	{ ai_run, 20 },
	{ ai_run, 24 },
	{ ai_run, 20, shambler_maybe_idle },
};
MMOVE_T(shambler_move_run) = { FRAME_run01, FRAME_run06, shambler_frames_run, nullptr };

MONSTERINFO_RUN(shambler_run) (gentity_t *self) -> void {
	if (self->enemy && self->enemy->client)
		self->monsterInfo.aiFlags |= AI_BRUTAL;
	else
		self->monsterInfo.aiFlags &= ~AI_BRUTAL;

	if (self->monsterInfo.aiFlags & AI_STAND_GROUND) {
		M_SetAnimation(self, &shambler_move_stand);
		return;
	}

	M_SetAnimation(self, &shambler_move_run);
}

//
// pain
//

// FIXME: needs halved explosion damage

MonsterFrame shambler_frames_pain[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
};
MMOVE_T(shambler_move_pain) = { FRAME_pain01, FRAME_pain06, shambler_frames_pain, shambler_run };

static PAIN(shambler_pain) (gentity_t *self, gentity_t *other, float kick, int damage, const MeansOfDeath &mod) -> void {
	if (level.time < self->timeStamp)
		return;

	self->timeStamp = level.time + 1_ms;
	gi.sound(self, CHAN_AUTO, sound_pain, 1, ATTN_NORM, 0);

	if (mod.id != ModID::Chainfist && damage <= 30 && frandom() > 0.2f)
		return;

	// If hard or nightmare, don't go into pain while attacking
	if (skill->integer >= 2) {
		if ((self->s.frame >= FRAME_smash01) && (self->s.frame <= FRAME_smash12))
			return;

		if ((self->s.frame >= FRAME_swingl01) && (self->s.frame <= FRAME_swingl09))
			return;

		if ((self->s.frame >= FRAME_swingr01) && (self->s.frame <= FRAME_swingr09))
			return;
	}

	if (!M_ShouldReactToPain(self, mod))
		return; // no pain anims in nightmare

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 2_sec;
	M_SetAnimation(self, &shambler_move_pain);
}

MONSTERINFO_SETSKIN(shambler_setskin) (gentity_t *self) -> void {
	// FIXME: create pain skin?
	//if (self->health < (self->maxHealth / 2))
	//	self->s.skinNum |= 1;
	//else
	//	self->s.skinNum &= ~1;
}

//
// attacks
//

static void ShamblerSaveLoc(gentity_t *self) {
	self->pos1 = self->enemy->s.origin; // save for aiming the shot
	self->pos1[2] += self->enemy->viewHeight;
	self->monsterInfo.nextFrame = FRAME_magic09;

	gi.sound(self, CHAN_WEAPON, sound_boom, 1, ATTN_NORM, 0);
	shambler_lightning_update(self);
}

constexpr SpawnFlags SPAWNFLAG_SHAMBLER_PRECISE = 1_spawnflag;

static Vector3 FindShamblerOffset(gentity_t *self) {
	Vector3 offset = { 0, 0, 48.f };

	for (int i = 0; i < 8; i++) {
		if (M_CheckClearShot(self, offset))
			return offset;

		offset.z -= 4.f;
	}

	return { 0, 0, 48.f };
}

static void ShamblerCastLightning(gentity_t *self) {
	if (!self->enemy)
		return;

	Vector3 start;
	Vector3 dir;
	Vector3 forward, right;
	Vector3 offset = FindShamblerOffset(self);

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, offset, forward, right);

	// calc direction to where we targted
	PredictAim(self, self->enemy, start, 0, false, self->spawnFlags.has(SPAWNFLAG_SHAMBLER_PRECISE) ? 0.f : 0.1f, &dir, nullptr);

	Vector3 end = start + (dir * 8192);
	trace_t tr = gi.traceLine(start, end, self, MASK_PROJECTILE | CONTENTS_SLIME | CONTENTS_LAVA);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_LIGHTNING);
	gi.WriteEntity(self);	// source entity
	gi.WriteEntity(world); // destination entity
	gi.WritePosition(start);
	gi.WritePosition(tr.endPos);
	gi.multicast(start, MULTICAST_PVS, false);

	fire_bullet(self, start, dir, irandom(8, 12), 15, 0, 0, ModID::TeslaMine);
}

MonsterFrame shambler_frames_magic[] = {
	{ ai_charge, 0, shambler_windup },
	{ ai_charge, 0, shambler_lightning_update },
	{ ai_charge, 0, shambler_lightning_update },
	{ ai_move, 0, shambler_lightning_update },
	{ ai_move, 0, shambler_lightning_update },
	{ ai_move, 0, ShamblerSaveLoc},
	{ ai_move },
	{ ai_charge },
	{ ai_move, 0, ShamblerCastLightning },
	{ ai_move, 0, ShamblerCastLightning },
	{ ai_move, 0, ShamblerCastLightning },
	{ ai_move },
};

MMOVE_T(shambler_attack_magic) = { FRAME_magic01, FRAME_magic12, shambler_frames_magic, shambler_run };

MONSTERINFO_ATTACK(shambler_attack) (gentity_t *self) -> void {
	M_SetAnimation(self, &shambler_attack_magic);
}

//
// melee
//

static void shambler_melee1(gentity_t *self) {
	gi.sound(self, CHAN_WEAPON, sound_melee1, 1, ATTN_NORM, 0);
}

static void shambler_melee2(gentity_t *self) {
	gi.sound(self, CHAN_WEAPON, sound_melee2, 1, ATTN_NORM, 0);
}

void sham_swingl9(gentity_t *self);
void sham_swingr9(gentity_t *self);

static void sham_smash10(gentity_t *self) {
	if (!self->enemy)
		return;

	ai_charge(self, 0);

	if (!CanDamage(self->enemy, self))
		return;

	Vector3 aim = { MELEE_DISTANCE, self->mins[0], -4 };
	bool hit = fire_hit(self, aim, irandom(110, 120), 120); // Slower attack

	if (hit)
		gi.sound(self, CHAN_WEAPON, sound_smack, 1, ATTN_NORM, 0);

	// SpawnMeatSpray(self.origin + vForward * 16, crandom() * 100 * v_right);
	// SpawnMeatSpray(self.origin + vForward * 16, crandom() * 100 * v_right);
};

static void ShamClaw(gentity_t *self) {
	if (!self->enemy)
		return;

	ai_charge(self, 10);

	if (!CanDamage(self->enemy, self))
		return;

	Vector3 aim = { MELEE_DISTANCE, self->mins[0], -4 };
	bool hit = fire_hit(self, aim, irandom(70, 80), 80); // Slower attack

	if (hit)
		gi.sound(self, CHAN_WEAPON, sound_smack, 1, ATTN_NORM, 0);
};

MonsterFrame shambler_frames_smash[] = {
	{ ai_charge, 2, shambler_melee1 },
	{ ai_charge, 6 },
	{ ai_charge, 6 },
	{ ai_charge, 5 },
	{ ai_charge, 4 },
	{ ai_charge, 1 },
	{ ai_charge, 0 },
	{ ai_charge, 0 },
	{ ai_charge, 0 },
	{ ai_charge, 0, sham_smash10 },
	{ ai_charge, 5 },
	{ ai_charge, 4 },
};

MMOVE_T(shambler_attack_smash) = { FRAME_smash01, FRAME_smash12, shambler_frames_smash, shambler_run };

MonsterFrame shambler_frames_swingl[] = {
	{ ai_charge, 5, shambler_melee1 },
	{ ai_charge, 3 },
	{ ai_charge, 7 },
	{ ai_charge, 3 },
	{ ai_charge, 7 },
	{ ai_charge, 9 },
	{ ai_charge, 5, ShamClaw },
	{ ai_charge, 4 },
	{ ai_charge, 8, sham_swingl9 },
};

MMOVE_T(shambler_attack_swingl) = { FRAME_swingl01, FRAME_swingl09, shambler_frames_swingl, shambler_run };

MonsterFrame shambler_frames_swingr[] = {
	{ ai_charge, 1, shambler_melee2 },
	{ ai_charge, 8 },
	{ ai_charge, 14 },
	{ ai_charge, 7 },
	{ ai_charge, 3 },
	{ ai_charge, 6 },
	{ ai_charge, 6, ShamClaw },
	{ ai_charge, 3 },
	{ ai_charge, 8, sham_swingr9 },
};

MMOVE_T(shambler_attack_swingr) = { FRAME_swingr01, FRAME_swingr09, shambler_frames_swingr, shambler_run };

void sham_swingl9(gentity_t *self) {
	ai_charge(self, 8);

	if (brandom() && self->enemy && range_to(self, self->enemy) < MELEE_DISTANCE)
		M_SetAnimation(self, &shambler_attack_swingr);
}

void sham_swingr9(gentity_t *self) {
	ai_charge(self, 1);
	ai_charge(self, 10);

	if (brandom() && self->enemy && range_to(self, self->enemy) < MELEE_DISTANCE)
		M_SetAnimation(self, &shambler_attack_swingl);
}

MONSTERINFO_MELEE(shambler_melee) (gentity_t *self) -> void {
	float chance = frandom();
	if (chance > 0.6 || self->health == 600)
		M_SetAnimation(self, &shambler_attack_smash);
	else if (chance > 0.3)
		M_SetAnimation(self, &shambler_attack_swingl);
	else
		M_SetAnimation(self, &shambler_attack_swingr);
}

//
// death
//

static void shambler_dead(gentity_t *self) {
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, -0 };
	monster_dead(self);
}

static void shambler_shrink(gentity_t *self) {
	self->maxs[2] = 0;
	self->svFlags |= SVF_DEADMONSTER;
	gi.linkEntity(self);
}

MonsterFrame shambler_frames_death[] = {
	{ ai_move, 0 },
	{ ai_move, 0 },
	{ ai_move, 0, shambler_shrink },
	{ ai_move, 0 },
	{ ai_move, 0 },
	{ ai_move, 0 },
	{ ai_move, 0 },
	{ ai_move, 0 },
	{ ai_move, 0 },
	{ ai_move, 0 },
	{ ai_move, 0 }, // FIXME: thud?
};
MMOVE_T(shambler_move_death) = { FRAME_death01, FRAME_death11, shambler_frames_death, shambler_dead };

static DIE(shambler_die) (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, const Vector3 &point, const MeansOfDeath &mod) -> void {
	if (self->beam) {
		FreeEntity(self->beam);
		self->beam = nullptr;
	}

	if (self->beam2) {
		FreeEntity(self->beam2);
		self->beam2 = nullptr;
	}

	// check for gib
	if (M_CheckGib(self, mod)) {
		gi.sound(self, CHAN_VOICE, gi.soundIndex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		// FIXME: better gibs for shambler, shambler head
		ThrowGibs(self, damage, {
			{ "models/objects/gibs/sm_meat/tris.md2" },
			{ "models/objects/gibs/chest/tris.md2" },
			{ "models/objects/gibs/head2/tris.md2", GIB_HEAD }
			});
		self->deadFlag = true;
		return;
	}

	if (self->deadFlag)
		return;

	// regular death
	gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	self->deadFlag = true;
	self->takeDamage = true;

	M_SetAnimation(self, &shambler_move_death);
}

void SP_monster_shambler(gentity_t *self) {
	if (!M_AllowSpawn(self)) {
		FreeEntity(self);
		return;
	}

	self->s.modelIndex = gi.modelIndex("models/monsters/shambler/tris.md2");
	self->mins = { -32, -32, -24 };
	self->maxs = { 32, 32, 64 };
	self->moveType = MoveType::Step;
	self->solid = SOLID_BBOX;

	gi.modelIndex("models/proj/lightning/tris.md2");
	sound_pain.assign("shambler/shurt2.wav");
	sound_idle.assign("shambler/sidle.wav");
	sound_die.assign("shambler/sdeath.wav");
	sound_windup.assign("shambler/sattck1.wav");
	sound_melee1.assign("shambler/melee1.wav");
	sound_melee2.assign("shambler/melee2.wav");
	sound_sight.assign("shambler/ssight.wav");
	sound_smack.assign("shambler/smack.wav");
	sound_boom.assign("shambler/sboom.wav");

	self->health = 600 * st.health_multiplier;
	self->gibHealth = -60;

	self->mass = 500;

	self->pain = shambler_pain;
	self->die = shambler_die;
	self->monsterInfo.stand = shambler_stand;
	self->monsterInfo.walk = shambler_walk;
	self->monsterInfo.run = shambler_run;
	self->monsterInfo.dodge = nullptr;
	self->monsterInfo.attack = shambler_attack;
	self->monsterInfo.melee = shambler_melee;
	self->monsterInfo.sight = shambler_sight;
	self->monsterInfo.idle = shambler_idle;
	self->monsterInfo.blocked = nullptr;
	self->monsterInfo.setSkin = shambler_setskin;

	gi.linkEntity(self);

	if (self->spawnFlags.has(SPAWNFLAG_SHAMBLER_PRECISE))
		self->monsterInfo.aiFlags |= AI_IGNORE_SHOTS;

	M_SetAnimation(self, &shambler_move_stand);
	self->monsterInfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}
