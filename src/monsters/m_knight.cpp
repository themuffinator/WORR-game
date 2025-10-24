#include "../g_local.hpp"
#include "m_knight.hpp"

/*
==============================================================================
Local sound handles
==============================================================================
*/
static cached_soundIndex s_idle;
static cached_soundIndex s_sight;
static cached_soundIndex s_pain;
static cached_soundIndex s_death;
static cached_soundIndex s_sword_hit;
static cached_soundIndex s_sword_miss;

/*
==============================================================================
Helpers
==============================================================================
*/
static void knight_idle(gentity_t* self) {
	if (frandom() < 0.2f)
		gi.sound(self, CHAN_VOICE, s_idle, 1, ATTN_IDLE, 0);
}

static void knight_step(gentity_t* self) {
	monster_footstep(self);
}

MONSTERINFO_SIGHT(knight_sight) (gentity_t* self, gentity_t* other) -> void {
	gi.sound(self, CHAN_VOICE, s_sight, 1, ATTN_NORM, 0);
}

/*
===============
knight_melee_hit
===============
*/
static void knight_melee_hit(gentity_t* self) {
	Vector3 aim = { MELEE_DISTANCE, self->mins[0], -4.0f };

	if (fire_hit(self, aim, irandom(15, 25), 200)) {
		// Successful strike
		gi.sound(self, CHAN_WEAPON, s_sword_hit, 1, ATTN_NORM, 0);
	}
	else {
		// Missed swing
		gi.sound(self, CHAN_WEAPON, s_sword_miss, 1, ATTN_NORM, 0);
		self->monsterInfo.melee_debounce_time = level.time + 1.5_sec;
	}
}

/*
==============================================================================
Attack sequence
==============================================================================
*/
void knight_attack(gentity_t* self);

static MonsterFrame frames_attack[] = {
	{ ai_charge, 0, [](gentity_t* self) { gi.sound(self, CHAN_WEAPON, s_sword_hit, 1, ATTN_NORM, 0); } },
	{ ai_charge, 7 },
	{ ai_charge, 4 },
	{ ai_charge, 0 },
	{ ai_charge, 3 },
	{ ai_charge, 4, knight_melee_hit },
	{ ai_charge, 1, knight_melee_hit },
	{ ai_charge, 3, knight_melee_hit },
	{ ai_charge, 1 },
	{ ai_charge, 5 }
};
void knight_run(gentity_t* self);
MMOVE_T(knight_move_attack) = { FRAME_attackb1, FRAME_attackb10, frames_attack, knight_run };

MONSTERINFO_ATTACK(knight_attack) (gentity_t* self) -> void {
	M_SetAnimation(self, &knight_move_attack);
}

/*
==============================================================================
Stand / Walk / Run
==============================================================================
*/
void knight_stand(gentity_t* self);
static MonsterFrame frames_stand[] = {
	{ ai_stand, 0, knight_idle }, { ai_stand }, { ai_stand }, { ai_stand },
	{ ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }
};
MMOVE_T(move_stand) = { FRAME_stand1, FRAME_stand9, frames_stand, knight_stand };
MONSTERINFO_STAND(knight_stand) (gentity_t* self) -> void {
	M_SetAnimation(self, &move_stand);
}

void knight_walk(gentity_t* self);
static MonsterFrame frames_walk[] = {
	{ ai_walk, 2 },{ ai_walk, 5 },{ ai_walk, 5 },{ ai_walk, 4 },
	{ ai_walk, 2 },{ ai_walk, 2 },{ ai_walk, 3 },{ ai_walk, 3 },
	{ ai_walk, 4 },{ ai_walk, 4 },{ ai_walk, 2 },{ ai_walk, 3 },
	{ ai_walk, 2 },{ ai_walk, 2 },{ ai_walk, 3 },{ ai_walk, 3 }
};
MMOVE_T(move_walk) = { FRAME_walk1, FRAME_walk16, frames_walk, nullptr };
MONSTERINFO_WALK(knight_walk) (gentity_t* self) -> void {
	M_SetAnimation(self, &move_walk);
}

static MonsterFrame frames_run[] = {
	{ ai_run, 12, knight_step },{ ai_run, 10 },
	{ ai_run, 14 },{ ai_run, 8, knight_step },
	{ ai_run, 10 },{ ai_run, 12 },
	{ ai_run, 7 }, { ai_run, 11 }
};
MMOVE_T(move_run) = { FRAME_run1, FRAME_run8, frames_run, nullptr };
MONSTERINFO_RUN(knight_run) (gentity_t* self) -> void {
	if (self->monsterInfo.aiFlags & AI_STAND_GROUND) {
		M_SetAnimation(self, &move_stand);
		return;
	}
	M_SetAnimation(self, &move_run);
}

static MonsterFrame pain_frames[] = {
	{ ai_move },{ ai_move },{ ai_move },{ ai_move }
};
MMOVE_T(knight_move_pain) = { FRAME_paina1, FRAME_paina4, pain_frames, knight_run };

/*
==============================================================================
Pain
==============================================================================
*/
static PAIN(knight_pain) (gentity_t* self, gentity_t* other, float kick, int damage,
	const MeansOfDeath& mod) -> void {
	if (level.time < self->pain_debounce_time)
		return;
	self->pain_debounce_time = level.time + 1_sec;

	gi.sound(self, CHAN_VOICE, s_pain, 1, ATTN_NORM, 0);

	if (!M_ShouldReactToPain(self, mod))
		return;

	M_SetAnimation(self, &knight_move_pain);
}

/*
==============================================================================
Death
==============================================================================
*/
static void knight_dead(gentity_t* self) {
	self->mins = { -16, -16, -24 };
	self->maxs = { 16,  16,  -8 };
	monster_dead(self);
}

static MonsterFrame frames_death[] = {
	{ ai_move },{ ai_move },{ ai_move },{ ai_move },
	{ ai_move },{ ai_move },{ ai_move },{ ai_move },
	{ ai_move },{ ai_move }
};
MMOVE_T(knight_move_death) = { FRAME_death1, FRAME_death10, frames_death, knight_dead };

static DIE(knight_die) (gentity_t* self, gentity_t* inflictor, gentity_t* attacker,
	int damage, const Vector3& point, const MeansOfDeath& mod) -> void {
	if (M_CheckGib(self, mod)) {
		gi.sound(self, CHAN_VOICE, gi.soundIndex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		ThrowGibs(self, damage, {
			{ 2, "models/objects/gibs/bone/tris.md2" },
			{ 2, "models/objects/gibs/sm_meat/tris.md2" },
			{ "models/monsters/knight/gibs/head.md2", GIB_HEAD }
			});
		self->deadFlag = true;
		return;
	}

	if (self->deadFlag)
		return;

	gi.sound(self, CHAN_VOICE, s_death, 1, ATTN_NORM, 0);

	self->deadFlag = true;
	self->takeDamage = true;
	M_SetAnimation(self, &knight_move_death);
}

/*
==============================================================================
Precache / Spawn
==============================================================================
*/
static void knight_precache() {
	gi.modelIndex("models/monsters/knight/tris.md2");
	gi.modelIndex("models/monsters/knight/gibs/head.md2");

	s_idle.assign("knight/idle.wav");
	s_sight.assign("knight/sight.wav");
	s_pain.assign("knight/pain.wav");
	s_death.assign("knight/death.wav");
	s_sword_hit.assign("knight/sword1.wav");
	s_sword_miss.assign("knight/swordmiss.wav");
}

void SP_monster_knight(gentity_t* self) {
	if (!M_AllowSpawn(self)) {
		FreeEntity(self);
		return;
	}

	knight_precache();

	self->className = "monster_knight";
	self->s.modelIndex = gi.modelIndex("models/monsters/knight/tris.md2");

	self->mins = { -16, -16, -24 };
	self->maxs = { 16,  16,  40 };
	self->yawSpeed = 15;

	self->health = self->maxHealth = 75 * st.health_multiplier;
	self->gibHealth = -40;
	self->mass = 200;

	self->monsterInfo.stand = knight_stand;
	self->monsterInfo.walk = knight_walk;
	self->monsterInfo.run = knight_run;
	self->monsterInfo.attack = knight_attack;
	self->monsterInfo.sight = knight_sight;
	self->pain = knight_pain;
	self->die = knight_die;

	M_SetAnimation(self, &move_stand);
	walkmonster_start(self);
}
