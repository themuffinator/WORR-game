// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

ZOMBIE - WOR port

- Slow shamble, lurching run
- Melee swipe only
- Feign-death on kill unless gibbed; rises after a delay
- Idle/search/sight, pain, death/getup sequences

==============================================================================
*/
#include "../g_local.hpp"
#include "m_zombie.hpp"

void zombie_getup_done(gentity_t* self);

/*
===============
Sounds
===============
*/
static cached_soundIndex snd_idle;
static cached_soundIndex snd_sight;
static cached_soundIndex snd_search;
static cached_soundIndex snd_pain1;
static cached_soundIndex snd_pain2;
static cached_soundIndex snd_death;   // initial fall
static cached_soundIndex snd_getup;   // resurrection grunt
static cached_soundIndex snd_swipe;

/*
===============
Internal state flags
count: 0=normal, 1=feign-dead
===============
*/
constexpr int ZSTATE_NORMAL = 0;
constexpr int ZSTATE_FEIGNDEAD = 1;

/*
===============
zombie_idle
===============
*/
static void zombie_idle(gentity_t* self) {
	if (frandom() > 0.75f)
		gi.sound(self, CHAN_VOICE, snd_idle, 1, ATTN_IDLE, 0);
}

/*
===============
zombie_stand
===============
*/
static MonsterFrame zombie_frames_stand[] = {
	{ ai_stand, 0, zombie_idle },
	{ ai_stand },{ ai_stand },{ ai_stand },
	{ ai_stand },{ ai_stand },{ ai_stand },{ ai_stand }
};
MMOVE_T(zombie_move_stand) = { FRAME_stand01, FRAME_stand08, zombie_frames_stand, nullptr };

MONSTERINFO_STAND(zombie_stand) (gentity_t* self) -> void {
	M_SetAnimation(self, &zombie_move_stand);
}

/*
===============
zombie_walk
===============
*/
static MonsterFrame zombie_frames_walk[] = {
	{ ai_walk, 2 },{ ai_walk, 3 },{ ai_walk, 2 },{ ai_walk, 3 },
	{ ai_walk, 2 },{ ai_walk, 3 },{ ai_walk, 2 },{ ai_walk, 3 }
};
MMOVE_T(zombie_move_walk) = { FRAME_walk01, FRAME_walk08, zombie_frames_walk, nullptr };

MONSTERINFO_WALK(zombie_walk) (gentity_t* self) -> void {
	M_SetAnimation(self, &zombie_move_walk);
}

/*
===============
zombie_run
===============
*/
static MonsterFrame zombie_frames_run[] = {
	{ ai_run, 6 },{ ai_run, 8 },{ ai_run, 10 },
	{ ai_run, 6 },{ ai_run, 8 },{ ai_run, 10 }
};
MMOVE_T(zombie_move_run) = { FRAME_run01, FRAME_run06, zombie_frames_run, nullptr };

MONSTERINFO_RUN(zombie_run) (gentity_t* self) -> void {
	if (self->monsterInfo.aiFlags & AI_STAND_GROUND)
		M_SetAnimation(self, &zombie_move_stand);
	else
		M_SetAnimation(self, &zombie_move_run);
}

/*
===============
zombie_swipe
===============
*/
static void zombie_swipe(gentity_t* self) {
	if (!self->enemy || self->enemy->health <= 0)
		return;

	Vector3 aim = { MELEE_DISTANCE, self->maxs[0] * 0.25f, 8.0f };
	const int dmg = irandom(8, 16);

	if (fire_hit(self, aim, dmg, 60))
		gi.sound(self, CHAN_WEAPON, snd_swipe, 1, ATTN_NORM, 0);
}

/*
===============
zombie_melee
===============
*/
static MonsterFrame zombie_frames_melee[] = {
	{ ai_charge, 6 },
	{ ai_charge, 0, zombie_swipe },
	{ ai_charge, 4 },
	{ ai_charge, 0, zombie_swipe }
};
MMOVE_T(zombie_move_melee) = { FRAME_melee01, FRAME_melee04, zombie_frames_melee, zombie_run };

MONSTERINFO_MELEE(zombie_melee) (gentity_t* self) -> void {
	M_SetAnimation(self, &zombie_move_melee);
}

/*
===============
zombie_pain
===============
*/
static MonsterFrame zombie_frames_pain[] = {
	{ ai_move },{ ai_move },{ ai_move },{ ai_move }
};
MMOVE_T(zombie_move_pain) = { FRAME_pain01, FRAME_pain04, zombie_frames_pain, zombie_run };

static PAIN(zombie_pain) (gentity_t* self, gentity_t* other, float kick, int damage, const MeansOfDeath& mod) -> void {
	if (self->count == ZSTATE_FEIGNDEAD)
		return; // no flinch while feigning

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 2.0_sec;

	if (brandom())
		gi.sound(self, CHAN_VOICE, snd_pain1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, snd_pain2, 1, ATTN_NORM, 0);

	if (!M_ShouldReactToPain(self, mod))
		return;

	M_SetAnimation(self, &zombie_move_pain);
}

/*
===============
zombie_sight
===============
*/
MONSTERINFO_SIGHT(zombie_sight) (gentity_t* self, gentity_t* other) -> void {
	gi.sound(self, CHAN_VOICE, snd_sight, 1, ATTN_NORM, 0);
}

/*
===============
zombie_search
===============
*/
MONSTERINFO_SEARCH(zombie_search) (gentity_t* self) -> void {
	gi.sound(self, CHAN_VOICE, snd_search, 1, ATTN_IDLE, 0);
}

/*
===============
zombie_setskin
===============
*/
MONSTERINFO_SETSKIN(zombie_setskin) (gentity_t* self) -> void {
	self->s.skinNum = (self->health < (self->maxHealth / 2)) ? 1 : 0;
}

/*
===============
zombie_deadbox
===============
*/
static void zombie_deadbox(gentity_t* self) {
	self->mins = { -16, -16, -8 };
	self->maxs = { 16,  16,  0 };
	self->svFlags |= SVF_DEADMONSTER;
	gi.linkEntity(self);
}

static MonsterFrame zombie_frames_getup[] = {
	{ ai_move },{ ai_move },{ ai_move },{ ai_move },
	{ ai_move },{ ai_move },{ ai_move },{ ai_move, 0, zombie_getup_done }
};
MMOVE_T(getup_move) = { FRAME_getup01, FRAME_getup08, zombie_frames_getup, zombie_getup_done };

/*
===============
zombie_try_getup_think
===============
*/
static THINK(zombie_try_getup_think) (gentity_t* self) -> void {
	// already removed or gibbed?
	if (!self->inUse || self->health <= self->gibHealth)
		return;

	// resurrect
	self->count = ZSTATE_NORMAL;
	self->deadFlag = false;
	self->svFlags &= ~SVF_DEADMONSTER;

	self->mins = { -16, -16, -24 };
	self->maxs = { 16,  16,  32 };
	self->health = self->maxHealth; // full restore on rise

	gi.linkEntity(self);

	gi.sound(self, CHAN_VOICE, snd_getup, 1, ATTN_NORM, 0);
	M_SetAnimation(self, &getup_move); // forward-declared alias below
}

/*
===============
zombie_getup end callback
===============
*/
void zombie_getup_done(gentity_t* self) {
	zombie_run(self);
}

/*
===============
zombie_die
Feign death unless gibbed
===============
*/
static MonsterFrame zombie_frames_death[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, zombie_deadbox },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(zombie_move_death) = { FRAME_death01, FRAME_death06, zombie_frames_death, nullptr };

static DIE(zombie_die) (gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, const Vector3& point, const MeansOfDeath& mod) -> void {
	// gib check first
	if (M_CheckGib(self, mod)) {
		gi.sound(self, CHAN_VOICE, gi.soundIndex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		ThrowGibs(self, damage, {
			{ 2, "models/objects/gibs/bone/tris.md2" },
			{ 3, "models/objects/gibs/sm_meat/tris.md2" },
			{ "models/objects/gibs/head2/tris.md2", GIB_HEAD }
			});
		self->deadFlag = true;
		return;
	}

	// if already feigning and hit again, shorten the getup delay
	if (self->count == ZSTATE_FEIGNDEAD) {
		self->think = zombie_try_getup_think;
		self->nextThink = level.time + random_time(2.0_sec, 4.0_sec);
		return;
	}

	// enter feign-death state
	self->deadFlag = true;
	self->takeDamage = true;
	self->count = ZSTATE_FEIGNDEAD;

	gi.sound(self, CHAN_VOICE, snd_death, 1, ATTN_NORM, 0);
	M_SetAnimation(self, &zombie_move_death);

	// schedule resurrection
	self->think = zombie_try_getup_think;
	self->nextThink = level.time + random_time(6.0_sec, 10.0_sec);
}

/*
===============
SP_monster_zombie
===============
*/
/*QUAKED monster_zombie (1 0 0) (-16 -16 -24) (16 16 32) AMBUSH TRIGGER_SPAWN SIGHT x CORPSE x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
model="models/monsters/zombie/tris.md2"
*/
void SP_monster_zombie(gentity_t* self) {
	if (!M_AllowSpawn(self)) {
		FreeEntity(self);
		return;
	}

	// sounds
	snd_idle.assign("zombie/z_idle1.wav");
	snd_sight.assign("zombie/idle_w2.wav");
	snd_search.assign("zombie/search.wav");	// doesn't exist
	snd_pain1.assign("zombie/z_pain.wav");
	snd_pain2.assign("zombie/z_pain1.wav");
	snd_death.assign("zombie/fall.wav");	// doesn't exist
	snd_getup.assign("zombie/getup.wav");	// doesn't exist
	snd_swipe.assign("zombie/z_shot1.wav");
	// todo: z_miss.wav
	//snd_miss.assign("zombie/z_miss.wav");

	// model and bbox
	self->moveType = MoveType::Step;
	self->solid = SOLID_BBOX;
	self->s.modelIndex = gi.modelIndex("models/monsters/zombie/tris.md2");
	self->mins = { -16, -16, -24 };
	self->maxs = { 16,  16,  32 };

	// stats
	self->health = self->maxHealth = 60 * st.health_multiplier;
	self->gibHealth = -35;
	self->mass = 140;

	// callbacks
	self->pain = zombie_pain;
	self->die = zombie_die;

	self->monsterInfo.stand = zombie_stand;
	self->monsterInfo.walk = zombie_walk;
	self->monsterInfo.run = zombie_run;
	self->monsterInfo.melee = zombie_melee;
	self->monsterInfo.sight = zombie_sight;
	self->monsterInfo.search = zombie_search;
	self->monsterInfo.setSkin = zombie_setskin;

	gi.linkEntity(self);

	M_SetAnimation(self, &zombie_move_stand);
	self->monsterInfo.scale = MODEL_SCALE;

	walkmonster_start(self);
}
