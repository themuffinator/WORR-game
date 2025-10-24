// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

SCRAG / WIZARD (Quake 1) - WOR Port
Floating caster that fires poison spikes while hovering.

Revised to mirror the Hover monster's structure and use supported AI
callbacks from g_ai.cpp (ai_stand, ai_walk, ai_run, ai_charge, ai_turn).
No behavior or stats changed.

==============================================================================
*/
#include "../g_local.hpp"
#include "m_scrag.hpp"
#include "m_flash.hpp"

// -----------------------------------------------------------------------------
// Tunables (unchanged to preserve behavior)
// -----------------------------------------------------------------------------
static constexpr float SCRAG_MODEL_SCALE = 1.0f;
static constexpr int   SCRAG_HEALTH = 80;
static constexpr int   SCRAG_GIB_HEALTH = -40;
static constexpr int   SCRAG_DAMAGE = 12;    // wizspike-ish
static constexpr int   SCRAG_SPEED = 700;   // projectile speed
static const Vector3    SCRAG_MUZZLE_OFFSET = { 24.0f, 0.0f, 16.0f };

// -----------------------------------------------------------------------------
// Sounds
// -----------------------------------------------------------------------------
static cached_soundIndex s_idle;
static cached_soundIndex s_sight;
static cached_soundIndex s_pain;
static cached_soundIndex s_death;
static cached_soundIndex s_fire;

/*
===============
scrag_idle
===============
*/
static void scrag_idle(gentity_t* self) {
	if (frandom() < 0.15f) {
		gi.sound(self, CHAN_VOICE, s_idle, 1, ATTN_IDLE, 0);
	}
}

/*
===============
scrag_sight
===============
*/
MONSTERINFO_SIGHT(scrag_sight) (gentity_t* self, gentity_t* other) -> void {
	gi.sound(self, CHAN_VOICE, s_sight, 1, ATTN_NORM, 0);
}

/*
===============
scrag_setskin
===============
*/
MONSTERINFO_SETSKIN(scrag_setskin) (gentity_t* self) -> void {
	// Keep base skin; no special pain skin by default.
}

/*
===============
scrag_fire
===============
*/
static void scrag_fire(gentity_t* self) {
	if (!self->enemy || !self->enemy->inUse)
		return;

	Vector3 forward, right;
	AngleVectors(self->s.angles, forward, right, nullptr);

	Vector3 start = M_ProjectFlashSource(self, SCRAG_MUZZLE_OFFSET, forward, right);

	Vector3 end = self->enemy->s.origin;
	end[2] += self->enemy->viewHeight;

	Vector3 dir = end - start;
	dir.normalize();

	gi.sound(self, CHAN_WEAPON, s_fire, 1, ATTN_NORM, 0);

	// Reuse a flier muzzleflash so visuals stay consistent
	const MonsterMuzzleFlashID flash = MZ2_FLYER_BLASTER_1;
	monster_fire_blaster(self, start, dir, SCRAG_DAMAGE, SCRAG_SPEED, flash, EF_BLASTER);
}

/*
===============
scrag_stand
===============
*/
void scrag_stand(gentity_t* self);

static MonsterFrame scrag_frames_stand[] = {
	{ ai_stand, 0, scrag_idle },
	{ ai_stand }, { ai_stand }, { ai_stand },
	{ ai_stand }, { ai_stand }
};
MMOVE_T(scrag_move_stand) = { FRAME_idle01, FRAME_idle06, scrag_frames_stand, scrag_stand };

/*
===============
scrag_stand
===============
*/
MONSTERINFO_STAND(scrag_stand) (gentity_t* self) -> void {
	M_SetAnimation(self, &scrag_move_stand);
}

/*
===============
scrag_fly
Use run-style motion like Hover to simulate flight.
===============
*/
void scrag_fly(gentity_t* self);

static MonsterFrame scrag_frames_fly[] = {
	{ ai_run, 4 }, { ai_run, 6 }, { ai_run, 5 }, { ai_run, 6 },
	{ ai_run, 4 }, { ai_run, 6 }, { ai_run, 5 }, { ai_run, 6 }
};
MMOVE_T(scrag_move_fly) = { FRAME_fly01, FRAME_fly08, scrag_frames_fly, nullptr };

/*
===============
scrag_fly
===============
*/
MONSTERINFO_RUN(scrag_fly) (gentity_t* self) -> void {
	if (self->monsterInfo.aiFlags & AI_STAND_GROUND) {
		M_SetAnimation(self, &scrag_move_stand);
		return;
	}
	M_SetAnimation(self, &scrag_move_fly);
}

/*
===============
scrag_walk
For completeness; uses a gentler motion than run.
===============
*/
MONSTERINFO_WALK(scrag_walk) (gentity_t* self) -> void {
	// Reuse run frames; distances come from the frame list and active_move.
	M_SetAnimation(self, &scrag_move_fly);
}

/*
===============
scrag_attack
===============
*/
void scrag_attack(gentity_t* self);

static MonsterFrame scrag_frames_attack[] = {
	// Use ai_charge with distance 0 to "face" target (ai_stand equivalent).
	{ ai_charge, 0 },               // FRAME_attack01
	{ ai_charge, 0 },               // FRAME_attack02
	{ ai_charge, 0, scrag_fire },   // FRAME_attack03 (fire)
	{ ai_charge, 0 },               // FRAME_attack04
	{ ai_charge, 0 },               // FRAME_attack05
	{ ai_charge, 0 }                // FRAME_attack06
};
MMOVE_T(scrag_move_attack) = { FRAME_attack01, FRAME_attack06, scrag_frames_attack, scrag_fly };

/*
===============
scrag_attack
===============
*/
MONSTERINFO_ATTACK(scrag_attack) (gentity_t* self) -> void {
	self->monsterInfo.attackFinished = level.time + 900_ms;
	M_SetAnimation(self, &scrag_move_attack);
}

static MonsterFrame pain_frames[] = {
	{ ai_move }, { ai_move }, { ai_move }, { ai_move }
};
MMOVE_T(scrag_move_pain) = { FRAME_pain01, FRAME_pain04, pain_frames, scrag_fly };

/*
===============
scrag_pain
===============
*/
static PAIN(scrag_pain) (gentity_t* self, gentity_t* other, float kick, int damage, const MeansOfDeath& mod) -> void {
	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 300_ms;
	gi.sound(self, CHAN_VOICE, s_pain, 1, ATTN_NORM, 0);

	M_SetAnimation(self, &scrag_move_pain);
}

/*
===============
scrag_dead
===============
*/
static void scrag_dead(gentity_t* self) {
	self->mins = { -16, -16, -24 };
	self->maxs = { 16,  16,  -8 };
	self->moveType = MoveType::Toss;
	self->nextThink = level.time + FRAME_TIME_S;
	gi.linkEntity(self);
}

static MonsterFrame death_frames[] = {
	{ ai_move }, { ai_move }, { ai_move }, { ai_move }, { ai_move }, { ai_move }, { ai_move }
};

MMOVE_T(scrag_move_death) = { FRAME_death01, FRAME_death07, death_frames, scrag_dead };

/*
===============
scrag_die
===============
*/
static DIE(scrag_die) (gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, const Vector3& point, const MeansOfDeath& mod) -> void {
	if (M_CheckGib(self, mod)) {
		ThrowGibs(self, 120, {
			{ "models/objects/gibs/sm_meat/tris.md2" }
			});
		return;
	}
	if (self->deadFlag)
		return;

	self->deadFlag = true;
	self->takeDamage = true;

	gi.sound(self, CHAN_VOICE, s_death, 1, ATTN_NORM, 0);
	M_SetAnimation(self, &scrag_move_death);
}

/*
===============
scrag_precache
===============
*/
static void scrag_precache() {
	gi.modelIndex("models/monsters/wizard/tris.md2");
	s_idle.assign("wizard/widle.wav");
	s_sight.assign("wizard/wsight.wav");
	s_pain.assign("wizard/wpain.wav");
	s_death.assign("wizard/wdeath.wav");
	s_fire.assign("wizard/wattack.wav");
}

/*
===============
scrag_start
===============
*/
static void scrag_start(gentity_t* self) {
	// Hook table (mirrors hover layout)
	self->monsterInfo.stand = scrag_stand;
	self->monsterInfo.walk = scrag_walk;
	self->monsterInfo.run = scrag_fly;
	self->monsterInfo.attack = scrag_attack;
	self->monsterInfo.sight = scrag_sight;
	self->monsterInfo.setSkin = scrag_setskin;

	self->pain = scrag_pain;
	self->die = scrag_die;

	// Spawn setup (kept identical where it matters)
	self->moveType = MoveType::Step;
	self->solid = SOLID_BBOX;

	self->mins = { -16, -16, -24 };
	self->maxs = { 16,  16,  16 };
	self->s.scale = SCRAG_MODEL_SCALE;
	self->yawSpeed = 20;

	self->health = self->maxHealth = SCRAG_HEALTH * st.health_multiplier;
	self->gibHealth = SCRAG_GIB_HEALTH;
	self->mass = 100;

	// Floating
	self->flags |= FL_FLY;

	gi.linkEntity(self);

	M_SetAnimation(self, &scrag_move_stand);
	self->monsterInfo.scale = SCRAG_MODEL_SCALE;

	flymonster_start(self);
}

/*QUAKED monster_wizard (1 .5 0) (-16 -16 -24) (16 16 16) AMBUSH TRIGGER_SPAWN SIGHT NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Quake 1 Scrag (Wizard). Floating monster that fires poison spikes.
*/
void SP_monster_wizard(gentity_t* self) {
	if (!M_AllowSpawn(self)) {
		FreeEntity(self);
		return;
	}

	scrag_precache();

	self->className = "monster_wizard";
	self->s.modelIndex = gi.modelIndex("models/monsters/wizard/tris.md2");

	scrag_start(self);
}
