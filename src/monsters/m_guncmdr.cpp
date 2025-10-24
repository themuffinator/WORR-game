// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

GUNNER

==============================================================================
*/

#include "../g_local.hpp"
#include "m_gunner.hpp"
#include "m_flash.hpp"

constexpr SpawnFlags SPAWNFLAG_GUNCMDR_NOJUMPING = 8_spawnflag;

static cached_soundIndex sound_pain;
static cached_soundIndex sound_pain2;
static cached_soundIndex sound_death;
static cached_soundIndex sound_idle;
static cached_soundIndex sound_open;
static cached_soundIndex sound_search;
static cached_soundIndex sound_sight;

static void guncmdr_idlesound(gentity_t *self) {
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

MONSTERINFO_SIGHT(guncmdr_sight) (gentity_t *self, gentity_t *other) -> void {
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

MONSTERINFO_SEARCH(guncmdr_search) (gentity_t *self) -> void {
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void GunnerGrenade(gentity_t *self);
void GunnerFire(gentity_t *self);
void guncmdr_fire_chain(gentity_t *self);
void guncmdr_refire_chain(gentity_t *self);

void guncmdr_stand(gentity_t *self);

MonsterFrame guncmdr_frames_fidget[] = {
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand, 0, guncmdr_idlesound },
	{ ai_stand },
	{ ai_stand },

	{ ai_stand },
	{ ai_stand, 0, guncmdr_idlesound },
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
MMOVE_T(guncmdr_move_fidget) = { FRAME_c_stand201, FRAME_c_stand254, guncmdr_frames_fidget, guncmdr_stand };

static void guncmdr_fidget(gentity_t *self) {
	if (self->monsterInfo.aiFlags & AI_STAND_GROUND)
		return;
	else if (self->enemy)
		return;
	if (frandom() <= 0.05f)
		M_SetAnimation(self, &guncmdr_move_fidget);
}

MonsterFrame guncmdr_frames_stand[] = {
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand, 0, guncmdr_fidget },

	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand, 0, guncmdr_fidget },

	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand, 0, guncmdr_fidget },

	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand, 0, guncmdr_fidget }
};
MMOVE_T(guncmdr_move_stand) = { FRAME_c_stand101, FRAME_c_stand140, guncmdr_frames_stand, nullptr };

MONSTERINFO_STAND(guncmdr_stand) (gentity_t *self) -> void {
	M_SetAnimation(self, &guncmdr_move_stand);
}

MonsterFrame guncmdr_frames_walk[] = {
	{ ai_walk, 1.5f, monster_footstep },
	{ ai_walk, 2.5f },
	{ ai_walk, 3.0f },
	{ ai_walk, 2.5f },
	{ ai_walk, 2.3f },
	{ ai_walk, 3.0f },
	{ ai_walk, 2.8f, monster_footstep },
	{ ai_walk, 3.6f },
	{ ai_walk, 2.8f },
	{ ai_walk, 2.5f },

	{ ai_walk, 2.3f },
	{ ai_walk, 4.3f },
	{ ai_walk, 3.0f, monster_footstep },
	{ ai_walk, 1.5f },
	{ ai_walk, 2.5f },
	{ ai_walk, 3.3f },
	{ ai_walk, 2.8f },
	{ ai_walk, 3.0f },
	{ ai_walk, 2.0f, monster_footstep },
	{ ai_walk, 2.0f },

	{ ai_walk, 3.3f },
	{ ai_walk, 3.6f },
	{ ai_walk, 3.4f },
	{ ai_walk, 2.8f },
};
MMOVE_T(guncmdr_move_walk) = { FRAME_c_walk101, FRAME_c_walk124, guncmdr_frames_walk, nullptr };

MONSTERINFO_WALK(guncmdr_walk) (gentity_t *self) -> void {
	M_SetAnimation(self, &guncmdr_move_walk);
}

MonsterFrame guncmdr_frames_run[] = {
	{ ai_run, 15.f, monster_done_dodge },
	{ ai_run, 16.f, monster_footstep },
	{ ai_run, 20.f },
	{ ai_run, 18.f },
	{ ai_run, 24.f, monster_footstep },
	{ ai_run, 13.5f }
};

MMOVE_T(guncmdr_move_run) = { FRAME_c_run101, FRAME_c_run106, guncmdr_frames_run, nullptr };

MONSTERINFO_RUN(guncmdr_run) (gentity_t *self) -> void {
	monster_done_dodge(self);
	if (self->monsterInfo.aiFlags & AI_STAND_GROUND)
		M_SetAnimation(self, &guncmdr_move_stand);
	else
		M_SetAnimation(self, &guncmdr_move_run);
}

// standing pains

MonsterFrame guncmdr_frames_pain1[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
};
MMOVE_T(guncmdr_move_pain1) = { FRAME_c_pain101, FRAME_c_pain104, guncmdr_frames_pain1, guncmdr_run };

MonsterFrame guncmdr_frames_pain2[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(guncmdr_move_pain2) = { FRAME_c_pain201, FRAME_c_pain204, guncmdr_frames_pain2, guncmdr_run };

MonsterFrame guncmdr_frames_pain3[] = {
	{ ai_move, -3.0f },
	{ ai_move },
	{ ai_move },
	{ ai_move },
};
MMOVE_T(guncmdr_move_pain3) = { FRAME_c_pain301, FRAME_c_pain304, guncmdr_frames_pain3, guncmdr_run };

MonsterFrame guncmdr_frames_pain4[] = {
	{ ai_move, -17.1f },
	{ ai_move, -3.2f },
	{ ai_move, 0.9f },
	{ ai_move, 3.6f },
	{ ai_move, -2.6f },
	{ ai_move, 1.0f },
	{ ai_move, -5.1f },
	{ ai_move, -6.7f },
	{ ai_move, -8.8f },
	{ ai_move },

	{ ai_move },
	{ ai_move, -2.1f },
	{ ai_move, -2.3f },
	{ ai_move, -2.5f },
	{ ai_move }
};
MMOVE_T(guncmdr_move_pain4) = { FRAME_c_pain401, FRAME_c_pain415, guncmdr_frames_pain4, guncmdr_run };

void guncmdr_dead(gentity_t *);

MonsterFrame guncmdr_frames_death1[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move, 4.0f }, // scoot
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
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
MMOVE_T(guncmdr_move_death1) = { FRAME_c_death101, FRAME_c_death118, guncmdr_frames_death1, guncmdr_dead };

static void guncmdr_pain5_to_death1(gentity_t *self) {
	if (self->health < 0)
		M_SetAnimation(self, &guncmdr_move_death1, false);
}

MonsterFrame guncmdr_frames_death2[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(guncmdr_move_death2) = { FRAME_c_death201, FRAME_c_death204, guncmdr_frames_death2, guncmdr_dead };

static void guncmdr_pain5_to_death2(gentity_t *self) {
	if (self->health < 0 && brandom())
		M_SetAnimation(self, &guncmdr_move_death2, false);
}

MonsterFrame guncmdr_frames_pain5[] = {
	{ ai_move, -29.f },
	{ ai_move, -5.f },
	{ ai_move, -5.f },
	{ ai_move, -3.f },
	{ ai_move },
	{ ai_move, 0, guncmdr_pain5_to_death2 },
	{ ai_move, 9.f },
	{ ai_move, 3.f },
	{ ai_move, 0, guncmdr_pain5_to_death1 },
	{ ai_move },

	{ ai_move },
	{ ai_move, -4.6f },
	{ ai_move, -4.8f },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 9.5f },
	{ ai_move, 3.4f },
	{ ai_move },
	{ ai_move },

	{ ai_move, -2.4f },
	{ ai_move, -9.0f },
	{ ai_move, -5.0f },
	{ ai_move, -3.6f },
};
MMOVE_T(guncmdr_move_pain5) = { FRAME_c_pain501, FRAME_c_pain524, guncmdr_frames_pain5, guncmdr_run };

void guncmdr_dead(gentity_t *self) {
	self->mins = Vector3{ -16, -16, -24 } *self->s.scale;
	self->maxs = Vector3{ 16, 16, -8 } *self->s.scale;
	monster_dead(self);
}

static void guncmdr_shrink(gentity_t *self) {
	self->maxs[2] = -4 * self->s.scale;
	self->svFlags |= SVF_DEADMONSTER;
	gi.linkEntity(self);
}

MonsterFrame guncmdr_frames_death6[] = {
	{ ai_move, 0, guncmdr_shrink },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
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
MMOVE_T(guncmdr_move_death6) = { FRAME_c_death601, FRAME_c_death614, guncmdr_frames_death6, guncmdr_dead };

static void guncmdr_pain6_to_death6(gentity_t *self) {
	if (self->health < 0)
		M_SetAnimation(self, &guncmdr_move_death6, false);
}

MonsterFrame guncmdr_frames_pain6[] = {
	{ ai_move, 16.f },
	{ ai_move, 16.f },
	{ ai_move, 12.f },
	{ ai_move, 5.5f, monster_duck_down },
	{ ai_move, 3.0f },
	{ ai_move, -4.7f },
	{ ai_move, -6.0f, guncmdr_pain6_to_death6 },
	{ ai_move },
	{ ai_move, 1.8f },
	{ ai_move, 0.7f },

	{ ai_move },
	{ ai_move, -2.1f },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move, -6.1f },
	{ ai_move, 10.5f },
	{ ai_move, 4.3f },
	{ ai_move, 4.7f, monster_duck_up },
	{ ai_move, 1.4f },
	{ ai_move },
	{ ai_move, -3.2f },
	{ ai_move, 2.3f },
	{ ai_move, -4.4f },

	{ ai_move, -4.4f },
	{ ai_move, -2.4f }
};
MMOVE_T(guncmdr_move_pain6) = { FRAME_c_pain601, FRAME_c_pain632, guncmdr_frames_pain6, guncmdr_run };

MonsterFrame guncmdr_frames_pain7[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
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
MMOVE_T(guncmdr_move_pain7) = { FRAME_c_pain701, FRAME_c_pain714, guncmdr_frames_pain7, guncmdr_run };

extern const MonsterMove guncmdr_move_jump;
extern const MonsterMove guncmdr_move_jump2;
extern const MonsterMove guncmdr_move_duck_attack;

bool guncmdr_sidestep(gentity_t *self);

static PAIN(guncmdr_pain) (gentity_t *self, gentity_t *other, float kick, int damage, const MeansOfDeath &mod) -> void {
	monster_done_dodge(self);

	if (self->monsterInfo.active_move == &guncmdr_move_jump ||
		self->monsterInfo.active_move == &guncmdr_move_jump2 ||
		self->monsterInfo.active_move == &guncmdr_move_duck_attack)
		return;

	if (level.time < self->pain_debounce_time) {
		if (frandom() < 0.3)
			self->monsterInfo.dodge(self, other, FRAME_TIME_S, nullptr, false);

		return;
	}

	self->pain_debounce_time = level.time + 3_sec;

	if (brandom())
		gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

	if (!M_ShouldReactToPain(self, mod)) {
		if (frandom() < 0.3)
			self->monsterInfo.dodge(self, other, FRAME_TIME_S, nullptr, false);

		return; // no pain anims in nightmare
	}

	Vector3 forward;
	AngleVectors(self->s.angles, forward, nullptr, nullptr);

	Vector3 dif = (other->s.origin - self->s.origin);
	dif.z = 0;
	dif.normalize();

	// small pain
	if (damage < 35) {
		int r = irandom(0, 4);

		if (r == 0)
			M_SetAnimation(self, &guncmdr_move_pain3);
		else if (r == 1)
			M_SetAnimation(self, &guncmdr_move_pain2);
		else if (r == 2)
			M_SetAnimation(self, &guncmdr_move_pain1);
		else
			M_SetAnimation(self, &guncmdr_move_pain7);
	}
	// large pain from behind (aka Paril)
	else if (dif.dot(forward) < -0.40f) {
		M_SetAnimation(self, &guncmdr_move_pain6);

		self->pain_debounce_time += 1.5_sec;
	} else {
		if (brandom())
			M_SetAnimation(self, &guncmdr_move_pain4);
		else
			M_SetAnimation(self, &guncmdr_move_pain5);

		self->pain_debounce_time += 1.5_sec;
	}

	self->monsterInfo.aiFlags &= ~AI_MANUAL_STEERING;

	// PMM - clear duck flag
	if (self->monsterInfo.aiFlags & AI_DUCKED)
		monster_duck_up(self);
}

MONSTERINFO_SETSKIN(guncmdr_setskin) (gentity_t *self) -> void {
	if (self->health < (self->maxHealth / 2))
		self->s.skinNum |= 1;
	else
		self->s.skinNum &= ~1;
}

MonsterFrame guncmdr_frames_death3[] = {
	{ ai_move, 20.f },
	{ ai_move, 10.f },
	{ ai_move, 10.f, [](gentity_t *self) { monster_footstep(self); guncmdr_shrink(self); } },
	{ ai_move, 0.f, monster_footstep },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
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
MMOVE_T(guncmdr_move_death3) = { FRAME_c_death301, FRAME_c_death321, guncmdr_frames_death3, guncmdr_dead };

MonsterFrame guncmdr_frames_death7[] = {
	{ ai_move, 30.f },
	{ ai_move, 20.f },
	{ ai_move, 16.f, [](gentity_t *self) { monster_footstep(self); guncmdr_shrink(self); } },
	{ ai_move, 5.f, monster_footstep },
	{ ai_move, -6.f },
	{ ai_move, -7.f, monster_footstep },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0.f, monster_footstep },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0.f, monster_footstep },
	{ ai_move },
	{ ai_move },
};
MMOVE_T(guncmdr_move_death7) = { FRAME_c_death701, FRAME_c_death730, guncmdr_frames_death7, guncmdr_dead };

MonsterFrame guncmdr_frames_death4[] = {
	{ ai_move, -20.f },
	{ ai_move, -16.f },
	{ ai_move, -26.f, [](gentity_t *self) { monster_footstep(self); guncmdr_shrink(self); } },
	{ ai_move, 0.f, monster_footstep },
	{ ai_move, -12.f },
	{ ai_move, 16.f },
	{ ai_move, 9.2f },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
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
MMOVE_T(guncmdr_move_death4) = { FRAME_c_death401, FRAME_c_death436, guncmdr_frames_death4, guncmdr_dead };

MonsterFrame guncmdr_frames_death5[] = {
	{ ai_move, -14.f },
	{ ai_move, -2.7f },
	{ ai_move, -2.5f },
	{ ai_move, -4.6f, monster_footstep },
	{ ai_move, -4.0f, monster_footstep },
	{ ai_move, -1.5f },
	{ ai_move, 2.3f },
	{ ai_move, 2.5f },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 3.5f },
	{ ai_move, 12.9f, monster_footstep },
	{ ai_move, 3.8f },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move, -2.1f },
	{ ai_move, -1.3f },
	{ ai_move },
	{ ai_move },
	{ ai_move, 3.4f },
	{ ai_move, 5.7f },
	{ ai_move, 11.2f },
	{ ai_move, 0, monster_footstep }
};
MMOVE_T(guncmdr_move_death5) = { FRAME_c_death501, FRAME_c_death528, guncmdr_frames_death5, guncmdr_dead };

static DIE(guncmdr_die) (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, const Vector3 &point, const MeansOfDeath &mod) -> void {
	// check for gib
	if (M_CheckGib(self, mod)) {
		gi.sound(self, CHAN_VOICE, gi.soundIndex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		const char *head_gib = (self->monsterInfo.active_move != &guncmdr_move_death5) ? "models/objects/gibs/sm_meat/tris.md2" : "models/monsters/gunner/gibs/head.md2";

		self->s.skinNum /= 2;

		ThrowGibs(self, damage, {
			{ 2, "models/objects/gibs/bone/tris.md2" },
			{ 2, "models/objects/gibs/sm_meat/tris.md2" },
			{ 1, "models/objects/gibs/gear/tris.md2" },
			{ "models/monsters/gunner/gibs/chest.md2", GIB_SKINNED },
			{ "models/monsters/gunner/gibs/garm.md2", GIB_SKINNED | GIB_UPRIGHT },
			{ "models/monsters/gunner/gibs/gun.md2", GIB_SKINNED | GIB_UPRIGHT },
			{ "models/monsters/gunner/gibs/foot.md2", GIB_SKINNED },
			{ head_gib, GIB_SKINNED | GIB_HEAD }
			});
		self->deadFlag = true;
		return;
	}

	if (self->deadFlag)
		return;

	// regular death
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->deadFlag = true;
	self->takeDamage = true;

	// these animations cleanly transitions to death, so just keep going
	if (self->monsterInfo.active_move == &guncmdr_move_pain5 &&
		self->s.frame < FRAME_c_pain508)
		return;
	else if (self->monsterInfo.active_move == &guncmdr_move_pain6 &&
		self->s.frame < FRAME_c_pain607)
		return;

	Vector3 forward;
	AngleVectors(self->s.angles, forward, nullptr, nullptr);

	Vector3 dif = (inflictor->s.origin - self->s.origin);
	dif.z = 0;
	dif.normalize();

	// off with da head
	if (std::fabs((self->s.origin[Z] + self->viewHeight) - point[2]) <= 4 &&
		self->velocity.z < 65.f) {
		M_SetAnimation(self, &guncmdr_move_death5);

		gentity_t *head = ThrowGib(self, "models/monsters/gunner/gibs/head.md2", damage, GIB_NONE, self->s.scale);

		if (head) {
			head->s.angles = self->s.angles;
			head->s.origin = self->s.origin + Vector3{ 0, 0, 24.f };
			Vector3 headDir = (self->s.origin - inflictor->s.origin);
			head->velocity = headDir / headDir.length() * 100.0f;
			head->velocity[2] = 200.0f;
			head->aVelocity *= 0.15f;
			gi.linkEntity(head);
		}
	}
	// damage came from behind; use backwards death
	else if (dif.dot(forward) < -0.40f) {
		int r = irandom(0, self->monsterInfo.active_move == &guncmdr_move_pain6 ? 2 : 3);

		if (r == 0)
			M_SetAnimation(self, &guncmdr_move_death3);
		else if (r == 1)
			M_SetAnimation(self, &guncmdr_move_death7);
		else if (r == 2)
			M_SetAnimation(self, &guncmdr_move_pain6);
	} else {
		int r = irandom(0, self->monsterInfo.active_move == &guncmdr_move_pain5 ? 1 : 2);

		if (r == 0)
			M_SetAnimation(self, &guncmdr_move_death4);
		else
			M_SetAnimation(self, &guncmdr_move_pain5);
	}
}

static void guncmdr_opengun(gentity_t *self) {
	gi.sound(self, CHAN_VOICE, sound_open, 1, ATTN_IDLE, 0);
}

static void GunnerCmdrFire(gentity_t *self) {
	Vector3					 start;
	Vector3					 forward, right;
	Vector3					 aim;
	MonsterMuzzleFlashID flash_number;

	if (!self->enemy || !self->enemy->inUse)
		return;

	if (self->s.frame >= FRAME_c_attack401 && self->s.frame <= FRAME_c_attack505)
		flash_number = MZ2_GUNCMDR_CHAINGUN_2;
	else
		flash_number = MZ2_GUNCMDR_CHAINGUN_1;

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, monster_flash_offset[flash_number], forward, right);
	PredictAim(self, self->enemy, start, 800, false, frandom() * 0.3f, &aim, nullptr);
	for (int i = 0; i < 3; i++)
		aim[i] += crandom_open() * 0.025f;
	monster_fire_flechette(self, start, aim, 4, 800, flash_number);
}

MonsterFrame guncmdr_frames_attack_chain[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, guncmdr_opengun },
	{ ai_charge }
};
MMOVE_T(guncmdr_move_attack_chain) = { FRAME_c_attack101, FRAME_c_attack106, guncmdr_frames_attack_chain, guncmdr_fire_chain };

MonsterFrame guncmdr_frames_fire_chain[] = {
	{ ai_charge, 0, GunnerCmdrFire },
	{ ai_charge, 0, GunnerCmdrFire },
	{ ai_charge, 0, GunnerCmdrFire },
	{ ai_charge, 0, GunnerCmdrFire },
	{ ai_charge, 0, GunnerCmdrFire },
	{ ai_charge, 0, GunnerCmdrFire }
};
MMOVE_T(guncmdr_move_fire_chain) = { FRAME_c_attack107, FRAME_c_attack112, guncmdr_frames_fire_chain, guncmdr_refire_chain };

MonsterFrame guncmdr_frames_fire_chain_run[] = {
	{ ai_charge, 15.f, GunnerCmdrFire },
	{ ai_charge, 16.f, GunnerCmdrFire },
	{ ai_charge, 20.f, GunnerCmdrFire },
	{ ai_charge, 18.f, GunnerCmdrFire },
	{ ai_charge, 24.f, GunnerCmdrFire },
	{ ai_charge, 13.5f, GunnerCmdrFire }
};
MMOVE_T(guncmdr_move_fire_chain_run) = { FRAME_c_run201, FRAME_c_run206, guncmdr_frames_fire_chain_run, guncmdr_refire_chain };

MonsterFrame guncmdr_frames_fire_chain_dodge_right[] = {
	{ ai_charge, 5.1f * 2.0f, GunnerCmdrFire },
	{ ai_charge, 9.0f * 2.0f, GunnerCmdrFire },
	{ ai_charge, 3.5f * 2.0f, GunnerCmdrFire },
	{ ai_charge, 3.6f * 2.0f, GunnerCmdrFire },
	{ ai_charge, -1.0f * 2.0f, GunnerCmdrFire }
};
MMOVE_T(guncmdr_move_fire_chain_dodge_right) = { FRAME_c_attack401, FRAME_c_attack405, guncmdr_frames_fire_chain_dodge_right, guncmdr_refire_chain };

MonsterFrame guncmdr_frames_fire_chain_dodge_left[] = {
	{ ai_charge, 5.1f * 2.0f, GunnerCmdrFire },
	{ ai_charge, 9.0f * 2.0f, GunnerCmdrFire },
	{ ai_charge, 3.5f * 2.0f, GunnerCmdrFire },
	{ ai_charge, 3.6f * 2.0f, GunnerCmdrFire },
	{ ai_charge, -1.0f * 2.0f, GunnerCmdrFire }
};
MMOVE_T(guncmdr_move_fire_chain_dodge_left) = { FRAME_c_attack501, FRAME_c_attack505, guncmdr_frames_fire_chain_dodge_left, guncmdr_refire_chain };

MonsterFrame guncmdr_frames_endfire_chain[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, guncmdr_opengun },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }
};
MMOVE_T(guncmdr_move_endfire_chain) = { FRAME_c_attack118, FRAME_c_attack124, guncmdr_frames_endfire_chain, guncmdr_run };

constexpr float MORTAR_SPEED = 850.f;
constexpr float GRENADE_SPEED = 600.f;

static void GunnerCmdrGrenade(gentity_t *self) {
	Vector3					 start;
	Vector3					 forward, right, up;
	Vector3					 aim;
	MonsterMuzzleFlashID flash_number = MZ2_GUNCMDR_GRENADE_FRONT_1;
	float					 spread = 1.0f;
	float					 pitch = 0;
	Vector3					target;
	bool					blindFire = false;

	if (!self->enemy || !self->enemy->inUse)
		return;

	if (self->monsterInfo.aiFlags & AI_MANUAL_STEERING)
		blindFire = true;

	if (self->s.frame == FRAME_c_attack205) {
		spread = -0.1f;
		flash_number = MZ2_GUNCMDR_GRENADE_MORTAR_1;
	} else if (self->s.frame == FRAME_c_attack208) {
		spread = 0.f;
		flash_number = MZ2_GUNCMDR_GRENADE_MORTAR_2;
	} else if (self->s.frame == FRAME_c_attack211) {
		spread = 0.1f;
		flash_number = MZ2_GUNCMDR_GRENADE_MORTAR_3;
	} else if (self->s.frame == FRAME_c_attack304) {
		spread = -0.1f;
		flash_number = MZ2_GUNCMDR_GRENADE_FRONT_1;
	} else if (self->s.frame == FRAME_c_attack307) {
		spread = 0.f;
		flash_number = MZ2_GUNCMDR_GRENADE_FRONT_2;
	} else if (self->s.frame == FRAME_c_attack310) {
		spread = 0.1f;
		flash_number = MZ2_GUNCMDR_GRENADE_FRONT_3;
	} else if (self->s.frame == FRAME_c_attack911) {
		spread = 0.25f;
		flash_number = MZ2_GUNCMDR_GRENADE_CROUCH_1;
	} else if (self->s.frame == FRAME_c_attack912) {
		spread = 0.f;
		flash_number = MZ2_GUNCMDR_GRENADE_CROUCH_2;
	} else if (self->s.frame == FRAME_c_attack913) {
		spread = -0.25f;
		flash_number = MZ2_GUNCMDR_GRENADE_CROUCH_3;
	}

	// if we're shooting blind and we still can't see our enemy
	if ((blindFire) && (!visible(self, self->enemy))) {
		// and we have a valid blind_fire_target
		if (!self->monsterInfo.blind_fire_target)
			return;

		target = self->monsterInfo.blind_fire_target;
	} else
		target = self->enemy->s.origin;

	AngleVectors(self->s.angles, forward, right, up);
	start = M_ProjectFlashSource(self, monster_flash_offset[flash_number], forward, right);

	if (self->enemy && !(flash_number >= MZ2_GUNCMDR_GRENADE_CROUCH_1 && flash_number <= MZ2_GUNCMDR_GRENADE_CROUCH_3)) {
		float dist;

		aim = target - self->s.origin;
		dist = aim.length();

		// aim up if they're on the same level as me and far away.
		if ((dist > 512) && (aim[2] < 64) && (aim[2] > -64)) {
			aim[2] += (dist - 512);
		}

		aim.normalize();
		pitch = aim[2];
		if (pitch > 0.4f)
			pitch = 0.4f;
		else if (pitch < -0.5f)
			pitch = -0.5f;

		if ((self->enemy->absMin.z - self->absMax.z) > 16.f && flash_number >= MZ2_GUNCMDR_GRENADE_MORTAR_1 && flash_number <= MZ2_GUNCMDR_GRENADE_MORTAR_3)
			pitch += 0.5f;
	}

	if (flash_number >= MZ2_GUNCMDR_GRENADE_FRONT_1 && flash_number <= MZ2_GUNCMDR_GRENADE_FRONT_3)
		pitch -= 0.05f;

	if (!(flash_number >= MZ2_GUNCMDR_GRENADE_CROUCH_1 && flash_number <= MZ2_GUNCMDR_GRENADE_CROUCH_3)) {
		aim = forward + (right * spread);
		aim += (up * pitch);
		aim.normalize();
	} else {
		PredictAim(self, self->enemy, start, 800, false, 0.f, &aim, nullptr);
		aim += right * spread;
		aim.normalize();
	}

	if (flash_number >= MZ2_GUNCMDR_GRENADE_CROUCH_1 && flash_number <= MZ2_GUNCMDR_GRENADE_CROUCH_3) {
		constexpr float inner_spread = 0.125f;

		for (size_t i = 0; i < 3; i++)
			fire_ionripper(self, start, aim + (right * (-(inner_spread * 2) + (inner_spread * (i + 1)))), 15, 800, EF_IONRIPPER);

		monster_muzzleflash(self, start, flash_number);
	} else {
		// mortar fires farther
		float speed;

		if (flash_number >= MZ2_GUNCMDR_GRENADE_MORTAR_1 && flash_number <= MZ2_GUNCMDR_GRENADE_MORTAR_3)
			speed = MORTAR_SPEED;
		else
			speed = GRENADE_SPEED;

		// try search for best pitch
		if (M_CalculatePitchToFire(self, target, start, aim, speed, 2.5f, (flash_number >= MZ2_GUNCMDR_GRENADE_MORTAR_1 && flash_number <= MZ2_GUNCMDR_GRENADE_MORTAR_3)))
			monster_fire_grenade(self, start, aim, 50, speed, flash_number, (crandom_open() * 10.0f), frandom() * 10.f);
		else
			// normal shot
			monster_fire_grenade(self, start, aim, 50, speed, flash_number, (crandom_open() * 10.0f), 200.f + (crandom_open() * 10.0f));
	}
}

MonsterFrame guncmdr_frames_attack_mortar[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, GunnerCmdrGrenade },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, GunnerCmdrGrenade },
	{ ai_charge },
	{ ai_charge },

	{ ai_charge, 0, GunnerCmdrGrenade },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, monster_duck_up },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }
};
MMOVE_T(guncmdr_move_attack_mortar) = { FRAME_c_attack201, FRAME_c_attack221, guncmdr_frames_attack_mortar, guncmdr_run };

static void guncmdr_grenade_mortar_resume(gentity_t *self) {
	M_SetAnimation(self, &guncmdr_move_attack_mortar);
	self->monsterInfo.attackState = MonsterAttackState::Straight;
	self->s.frame = self->count;
}

MonsterFrame guncmdr_frames_attack_mortar_dodge[] = {
	{ ai_charge, 11.f },
	{ ai_charge, 12.f },
	{ ai_charge, 16.f },
	{ ai_charge, 16.f },
	{ ai_charge, 12.f },
	{ ai_charge, 11.f }
};
MMOVE_T(guncmdr_move_attack_mortar_dodge) = { FRAME_c_duckstep01, FRAME_c_duckstep06, guncmdr_frames_attack_mortar_dodge, guncmdr_grenade_mortar_resume };

MonsterFrame guncmdr_frames_attack_back[] = {
	//{ ai_charge },
	{ ai_charge, -2.f },
	{ ai_charge, -1.5f },
	{ ai_charge, -0.5f, GunnerCmdrGrenade },
	{ ai_charge, -6.0f },
	{ ai_charge, -4.f },
	{ ai_charge, -2.5f, GunnerCmdrGrenade },
	{ ai_charge, -7.0f },
	{ ai_charge, -3.5f },
	{ ai_charge, -1.1f, GunnerCmdrGrenade },

	{ ai_charge, -4.6f },
	{ ai_charge, 1.9f },
	{ ai_charge, 1.0f },
	{ ai_charge, -4.5f },
	{ ai_charge, 3.2f },
	{ ai_charge, 4.4f },
	{ ai_charge, -6.5f },
	{ ai_charge, -6.1f },
	{ ai_charge, 3.0f },
	{ ai_charge, -0.7f },
	{ ai_charge, -1.0f }
};
MMOVE_T(guncmdr_move_attack_grenade_back) = { FRAME_c_attack302, FRAME_c_attack321, guncmdr_frames_attack_back, guncmdr_run };

static void guncmdr_grenade_back_dodge_resume(gentity_t *self) {
	M_SetAnimation(self, &guncmdr_move_attack_grenade_back);
	self->monsterInfo.attackState = MonsterAttackState::Straight;
	self->s.frame = self->count;
}

MonsterFrame guncmdr_frames_attack_grenade_back_dodge_right[] = {
	{ ai_charge, 5.1f * 2.0f },
	{ ai_charge, 9.0f * 2.0f },
	{ ai_charge, 3.5f * 2.0f },
	{ ai_charge, 3.6f * 2.0f },
	{ ai_charge, -1.0f * 2.0f }
};
MMOVE_T(guncmdr_move_attack_grenade_back_dodge_right) = { FRAME_c_attack601, FRAME_c_attack605, guncmdr_frames_attack_grenade_back_dodge_right, guncmdr_grenade_back_dodge_resume };

MonsterFrame guncmdr_frames_attack_grenade_back_dodge_left[] = {
	{ ai_charge, 5.1f * 2.0f },
	{ ai_charge, 9.0f * 2.0f },
	{ ai_charge, 3.5f * 2.0f },
	{ ai_charge, 3.6f * 2.0f },
	{ ai_charge, -1.0f * 2.0f }
};
MMOVE_T(guncmdr_move_attack_grenade_back_dodge_left) = { FRAME_c_attack701, FRAME_c_attack705, guncmdr_frames_attack_grenade_back_dodge_left, guncmdr_grenade_back_dodge_resume };

static void guncmdr_kick_finished(gentity_t *self) {
	self->monsterInfo.melee_debounce_time = level.time + 3_sec;
	self->monsterInfo.attack(self);
}

static void guncmdr_kick(gentity_t *self) {
	if (fire_hit(self, Vector3{ MELEE_DISTANCE, 0.f, -32.f }, 15.f, 400.f)) {
		if (self->enemy && self->enemy->client && self->enemy->velocity.z < 270.f)
			self->enemy->velocity.z = 270.f;
	}
}

MonsterFrame guncmdr_frames_attack_kick[] = {
	{ ai_charge, -7.7f },
	{ ai_charge, -4.9f },
	{ ai_charge, 12.6f, guncmdr_kick },
	{ ai_charge },
	{ ai_charge, -3.0f },
	{ ai_charge },
	{ ai_charge, -4.1f },
	{ ai_charge, 8.6f },
	//{ ai_charge, -3.5f }
};
MMOVE_T(guncmdr_move_attack_kick) = { FRAME_c_attack801, FRAME_c_attack808, guncmdr_frames_attack_kick, guncmdr_kick_finished };

// don't ever try grenades if we get this close
constexpr float RANGE_GRENADE = 100.f;

// always use mortar at this range
constexpr float RANGE_GRENADE_MORTAR = 525.f;

// at this range, run towards the enemy
constexpr float RANGE_CHAINGUN_RUN = 400.f;

MONSTERINFO_ATTACK(guncmdr_attack) (gentity_t *self) -> void {
	monster_done_dodge(self);

	float d = range_to(self, self->enemy);

	Vector3 forward, right, aim;
	AngleVectors(self->s.angles, forward, right, nullptr);

	// always use chaingun on tesla
	// kick close enemies
	if (!self->bad_area && d < RANGE_MELEE && self->monsterInfo.melee_debounce_time < level.time)
		M_SetAnimation(self, &guncmdr_move_attack_kick);
	else if (self->bad_area || ((d <= RANGE_GRENADE || brandom()) && M_CheckClearShot(self, monster_flash_offset[MZ2_GUNCMDR_CHAINGUN_1])))
		M_SetAnimation(self, &guncmdr_move_attack_chain);
	else if ((d >= RANGE_GRENADE_MORTAR ||
		fabs(self->absMin.z - self->enemy->absMax.z) > 64.f // enemy is far below or above us, always try mortar
		) && M_CheckClearShot(self, monster_flash_offset[MZ2_GUNCMDR_GRENADE_MORTAR_1]) &&
		M_CalculatePitchToFire(self, self->enemy->s.origin, M_ProjectFlashSource(self, monster_flash_offset[MZ2_GUNCMDR_GRENADE_MORTAR_1], forward, right),
			aim = (self->enemy->s.origin - self->s.origin).normalized(), MORTAR_SPEED, 2.5f, true)
		) {
		M_SetAnimation(self, &guncmdr_move_attack_mortar);
		monster_duck_down(self);
	} else if (M_CheckClearShot(self, monster_flash_offset[MZ2_GUNCMDR_GRENADE_FRONT_1]) && !(self->monsterInfo.aiFlags & AI_STAND_GROUND) &&
		M_CalculatePitchToFire(self, self->enemy->s.origin, M_ProjectFlashSource(self, monster_flash_offset[MZ2_GUNCMDR_GRENADE_FRONT_1], forward, right),
			aim = (self->enemy->s.origin - self->s.origin).normalized(), GRENADE_SPEED, 2.5f, false))
		M_SetAnimation(self, &guncmdr_move_attack_grenade_back);
	else if (self->monsterInfo.aiFlags & AI_STAND_GROUND)
		M_SetAnimation(self, &guncmdr_move_attack_chain);
}

void guncmdr_fire_chain(gentity_t *self) {
	if (!(self->monsterInfo.aiFlags & AI_STAND_GROUND) && self->enemy && range_to(self, self->enemy) > RANGE_CHAINGUN_RUN && ai_check_move(self, 8.0f))
		M_SetAnimation(self, &guncmdr_move_fire_chain_run);
	else
		M_SetAnimation(self, &guncmdr_move_fire_chain);
}

void guncmdr_refire_chain(gentity_t *self) {
	monster_done_dodge(self);
	self->monsterInfo.attackState = MonsterAttackState::Straight;

	if (self->enemy->health > 0)
		if (visible(self, self->enemy))
			if (frandom() <= 0.5f) {
				if (!(self->monsterInfo.aiFlags & AI_STAND_GROUND) && self->enemy && range_to(self, self->enemy) > RANGE_CHAINGUN_RUN && ai_check_move(self, 8.0f))
					M_SetAnimation(self, &guncmdr_move_fire_chain_run, false);
				else
					M_SetAnimation(self, &guncmdr_move_fire_chain, false);
				return;
			}
	M_SetAnimation(self, &guncmdr_move_endfire_chain, false);
}

static void guncmdr_jump_now(gentity_t *self) {
	Vector3 forward, up;

	AngleVectors(self->s.angles, forward, nullptr, up);
	self->velocity += (forward * 100);
	self->velocity += (up * 300);
}

static void guncmdr_jump2_now(gentity_t *self) {
	Vector3 forward, up;

	AngleVectors(self->s.angles, forward, nullptr, up);
	self->velocity += (forward * 150);
	self->velocity += (up * 400);
}

static void guncmdr_jump_wait_land(gentity_t *self) {
	if (self->groundEntity == nullptr) {
		self->monsterInfo.nextFrame = self->s.frame;

		if (monster_jump_finished(self))
			self->monsterInfo.nextFrame = self->s.frame + 1;
	} else
		self->monsterInfo.nextFrame = self->s.frame + 1;
}

MonsterFrame guncmdr_frames_jump[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, guncmdr_jump_now },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, guncmdr_jump_wait_land },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(guncmdr_move_jump) = { FRAME_c_jump01, FRAME_c_jump10, guncmdr_frames_jump, guncmdr_run };

MonsterFrame guncmdr_frames_jump2[] = {
	{ ai_move, -8 },
	{ ai_move, -4 },
	{ ai_move, -4 },
	{ ai_move, 0, guncmdr_jump2_now },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, guncmdr_jump_wait_land },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(guncmdr_move_jump2) = { FRAME_c_jump01, FRAME_c_jump10, guncmdr_frames_jump2, guncmdr_run };

static void guncmdr_jump(gentity_t *self, BlockedJumpResult result) {
	if (!self->enemy)
		return;

	monster_done_dodge(self);

	if (result == BlockedJumpResult::Jump_Turn_Up)
		M_SetAnimation(self, &guncmdr_move_jump2);
	else
		M_SetAnimation(self, &guncmdr_move_jump);
}

void T_SlamRadiusDamage(Vector3 point, gentity_t *inflictor, gentity_t *attacker, float damage, float kick, gentity_t *ignore, float radius, MeansOfDeath mod);

static void GunnerCmdrCounter(gentity_t *self) {
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_BERSERK_SLAM);
	Vector3 f, r, start;
	AngleVectors(self->s.angles, f, r, nullptr);
	start = M_ProjectFlashSource(self, { 20.f, 0.f, 14.f }, f, r);
	trace_t tr = gi.traceLine(self->s.origin, start, self, MASK_SOLID);
	gi.WritePosition(tr.endPos);
	gi.WriteDir(f);
	gi.multicast(tr.endPos, MULTICAST_PHS, false);

	T_SlamRadiusDamage(tr.endPos, self, self, 15, 250.f, self, 200.f, ModID::Unknown);
}

MonsterFrame guncmdr_frames_duck_attack[] = {
	{ ai_move, 3.6f },
	{ ai_move, 5.6f, monster_duck_down },
	{ ai_move, 8.4f },
	{ ai_move, 2.0f, monster_duck_hold },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },

	//{ ai_charge, 0, GunnerCmdrGrenade },
	//{ ai_charge, 9.5f, GunnerCmdrGrenade },
	//{ ai_charge, -1.5f, GunnerCmdrGrenade },

	{ ai_charge, 0 },
	{ ai_charge, 9.5f, GunnerCmdrCounter },
	{ ai_charge, -1.5f },
	{ ai_charge },
	{ ai_charge, 0, monster_duck_up },
	{ ai_charge },
	{ ai_charge, 11.f },
	{ ai_charge, 2.0f },
	{ ai_charge, 5.6f }
};
MMOVE_T(guncmdr_move_duck_attack) = { FRAME_c_attack901, FRAME_c_attack919, guncmdr_frames_duck_attack, guncmdr_run };

MONSTERINFO_DUCK(guncmdr_duck) (gentity_t *self, GameTime eta) -> bool {
	if ((self->monsterInfo.active_move == &guncmdr_move_jump2) ||
		(self->monsterInfo.active_move == &guncmdr_move_jump)) {
		return false;
	}

	if ((self->monsterInfo.active_move == &guncmdr_move_fire_chain_dodge_left) ||
		(self->monsterInfo.active_move == &guncmdr_move_fire_chain_dodge_right) ||
		(self->monsterInfo.active_move == &guncmdr_move_attack_grenade_back_dodge_left) ||
		(self->monsterInfo.active_move == &guncmdr_move_attack_grenade_back_dodge_right) ||
		(self->monsterInfo.active_move == &guncmdr_move_attack_mortar_dodge)) {
		// if we're dodging, don't duck
		self->monsterInfo.unDuck(self);
		return false;
	}

	M_SetAnimation(self, &guncmdr_move_duck_attack);

	return true;
}

MONSTERINFO_SIDESTEP(guncmdr_sidestep) (gentity_t *self) -> bool {
	// use special dodge during the main firing anim
	if (self->monsterInfo.active_move == &guncmdr_move_fire_chain ||
		self->monsterInfo.active_move == &guncmdr_move_fire_chain_run) {
		M_SetAnimation(self, !self->monsterInfo.lefty ? &guncmdr_move_fire_chain_dodge_right : &guncmdr_move_fire_chain_dodge_left, false);
		return true;
	}

	// for backwards mortar, back up where we are in the animation and do a quick dodge
	if (self->monsterInfo.active_move == &guncmdr_move_attack_grenade_back) {
		self->count = self->s.frame;
		M_SetAnimation(self, !self->monsterInfo.lefty ? &guncmdr_move_attack_grenade_back_dodge_right : &guncmdr_move_attack_grenade_back_dodge_left, false);
		return true;
	}

	// use crouch-move for mortar dodge
	if (self->monsterInfo.active_move == &guncmdr_move_attack_mortar) {
		self->count = self->s.frame;
		M_SetAnimation(self, &guncmdr_move_attack_mortar_dodge, false);
		return true;
	}

	// regular sideStep during run
	if (self->monsterInfo.active_move == &guncmdr_move_run) {
		M_SetAnimation(self, &guncmdr_move_run, true);
		return true;
	}

	return false;
}

MONSTERINFO_BLOCKED(guncmdr_blocked) (gentity_t *self, float dist) -> bool {
	if (blocked_checkplat(self, dist))
		return true;

	if (auto result = blocked_checkjump(self, dist); result != BlockedJumpResult::No_Jump) {
		if (result != BlockedJumpResult::Jump_Turn)
			guncmdr_jump(self, result);

		return true;
	}

	return false;
}

/*QUAKED monster_guncmdr (1 .5 0) (-16 -16 -24) (16 16 32) AMBUSH TRIGGER_SPAWN SIGHT NOJUMPING x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
model="models/monsters/guncmdr/tris.md2"
*/
void SP_monster_guncmdr(gentity_t *self) {
	if (!M_AllowSpawn(self)) {
		FreeEntity(self);
		return;
	}

	sound_death.assign("guncmdr/gcdrdeath1.wav");
	sound_pain.assign("guncmdr/gcdrpain2.wav");
	sound_pain2.assign("guncmdr/gcdrpain1.wav");
	sound_idle.assign("guncmdr/gcdridle1.wav");
	sound_open.assign("guncmdr/gcdratck1.wav");
	sound_search.assign("guncmdr/gcdrsrch1.wav");
	sound_sight.assign("guncmdr/sight1.wav");

	gi.soundIndex("guncmdr/gcdratck2.wav");
	gi.soundIndex("guncmdr/gcdratck3.wav");

	self->moveType = MoveType::Step;
	self->solid = SOLID_BBOX;
	self->s.modelIndex = gi.modelIndex("models/monsters/gunner/tris.md2");

	gi.modelIndex("models/monsters/gunner/gibs/chest.md2");
	gi.modelIndex("models/monsters/gunner/gibs/foot.md2");
	gi.modelIndex("models/monsters/gunner/gibs/garm.md2");
	gi.modelIndex("models/monsters/gunner/gibs/gun.md2");
	gi.modelIndex("models/monsters/gunner/gibs/head.md2");

	self->s.scale = 1.25f;
	self->mins = Vector3{ -16, -16, -24 };
	self->maxs = Vector3{ 16, 16, 36 };
	self->s.skinNum = 2;

	self->health = 325 * st.health_multiplier;
	self->gibHealth = -175;
	self->mass = 255;

	self->pain = guncmdr_pain;
	self->die = guncmdr_die;

	self->monsterInfo.stand = guncmdr_stand;
	self->monsterInfo.walk = guncmdr_walk;
	self->monsterInfo.run = guncmdr_run;
	self->monsterInfo.dodge = M_MonsterDodge;
	self->monsterInfo.duck = guncmdr_duck;
	self->monsterInfo.unDuck = monster_duck_up;
	self->monsterInfo.sideStep = guncmdr_sidestep;
	self->monsterInfo.blocked = guncmdr_blocked;
	self->monsterInfo.attack = guncmdr_attack;
	self->monsterInfo.melee = nullptr;
	self->monsterInfo.sight = guncmdr_sight;
	self->monsterInfo.search = guncmdr_search;
	self->monsterInfo.setSkin = guncmdr_setskin;

	gi.linkEntity(self);

	M_SetAnimation(self, &guncmdr_move_stand);
	self->monsterInfo.scale = MODEL_SCALE;

	if (!st.was_key_specified("powerArmorPower"))
		self->monsterInfo.powerArmorPower = 200;
	if (!st.was_key_specified("powerArmorType"))
		self->monsterInfo.powerArmorType = IT_POWER_SHIELD;

	self->monsterInfo.canJump = !self->spawnFlags.has(SPAWNFLAG_GUNCMDR_NOJUMPING);
	self->monsterInfo.dropHeight = 192;
	self->monsterInfo.jumpHeight = 40;

	walkmonster_start(self);
}
