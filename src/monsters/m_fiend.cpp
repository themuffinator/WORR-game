// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

FIEND (Demon) - WOR port

- Melee claws at close range
- Mid/long pounce (leap) that deals on-impact damage
- Two pain sets, two death sets
- Sight/search/idle barks

==============================================================================
*/

#include "../g_local.hpp"
#include "m_fiend.hpp"

/*
===============
Spawnflags
===============
*/
constexpr SpawnFlags SPAWNFLAG_FIEND_NOJUMP = 8_spawnflag;

/*
===============
Sounds
===============
*/
static cached_soundIndex snd_claw;
static cached_soundIndex snd_pounce;
static cached_soundIndex snd_pain1;
static cached_soundIndex snd_pain2;
static cached_soundIndex snd_death;
static cached_soundIndex snd_idle1;
static cached_soundIndex snd_idle2;
static cached_soundIndex snd_search;
static cached_soundIndex snd_sight1;
static cached_soundIndex snd_sight2;
static cached_soundIndex snd_land;

/*
===============
fiend_sight
===============
*/
MONSTERINFO_SIGHT(fiend_sight) (gentity_t* self, gentity_t* other) -> void {
	if (frandom() > 0.7f)
		gi.sound(self, CHAN_VOICE, snd_sight1, 1, ATTN_IDLE, 0);
	else
		gi.sound(self, CHAN_VOICE, snd_sight2, 1, ATTN_IDLE, 0);
}

/*
===============
fiend_search
===============
*/
MONSTERINFO_SEARCH(fiend_search) (gentity_t* self) -> void {
	gi.sound(self, CHAN_VOICE, snd_search, 1, ATTN_IDLE, 0);
}

/*
===============
fiend_idle
===============
*/
MONSTERINFO_IDLE(fiend_idle) (gentity_t* self) -> void {
	if (frandom() > 0.7f)
		gi.sound(self, CHAN_VOICE, snd_idle1, 1, ATTN_IDLE, 0);
	else
		gi.sound(self, CHAN_VOICE, snd_idle2, 1, ATTN_IDLE, 0);
}

/*
===============
fiend_stand
===============
*/
static MonsterFrame fiend_frames_stand[] = {
	{ ai_stand },{ ai_stand },{ ai_stand },{ ai_stand },{ ai_stand },
	{ ai_stand },{ ai_stand },{ ai_stand },{ ai_stand },{ ai_stand }
};
MMOVE_T(fiend_move_stand) = { FRAME_stand01, FRAME_stand10, fiend_frames_stand, nullptr };

MONSTERINFO_STAND(fiend_stand) (gentity_t* self) -> void {
	M_SetAnimation(self, &fiend_move_stand);
}

/*
===============
fiend_walk
===============
*/
static MonsterFrame fiend_frames_walk[] = {
	{ ai_walk, 6 },{ ai_walk, 6 },{ ai_walk, 6 },{ ai_walk, 6 },
	{ ai_walk, 6 },{ ai_walk, 6 },{ ai_walk, 6 },{ ai_walk, 6 }
};
MMOVE_T(fiend_move_walk) = { FRAME_walk01, FRAME_walk08, fiend_frames_walk, nullptr };

MONSTERINFO_WALK(fiend_walk) (gentity_t* self) -> void {
	M_SetAnimation(self, &fiend_move_walk);
}

/*
===============
fiend_run
===============
*/
static MonsterFrame fiend_frames_run[] = {
	{ ai_run, 18 },{ ai_run, 24 },{ ai_run, 26 },{ ai_run, 22 },{ ai_run, 20 },
	{ ai_run, 26 },{ ai_run, 24 },{ ai_run, 22 },{ ai_run, 26 },{ ai_run, 24 }
};
MMOVE_T(fiend_move_run) = { FRAME_run01, FRAME_run10, fiend_frames_run, nullptr };

MONSTERINFO_RUN(fiend_run) (gentity_t* self) -> void {
	if (self->monsterInfo.aiFlags & AI_STAND_GROUND)
		M_SetAnimation(self, &fiend_move_stand);
	else
		M_SetAnimation(self, &fiend_move_run);
}

/*
===============
fiend_claw
===============
*/
static void fiend_claw(gentity_t* self) {
	if (!self->enemy || self->enemy->health <= 0)
		return;

	Vector3 aim = { MELEE_DISTANCE, self->mins[0], 8.0f };
	const int dmg = irandom(20, 35);

	if (fire_hit(self, aim, dmg, 150))
		gi.sound(self, CHAN_WEAPON, snd_claw, 1, ATTN_NORM, 0);
}

/*
===============
fiend_melee
===============
*/
static MonsterFrame fiend_frames_melee[] = {
	{ ai_charge, 10 },
	{ ai_charge,  6, fiend_claw },
	{ ai_charge,  0 },
	{ ai_charge,  6, fiend_claw },
	{ ai_charge,  0 },
	{ ai_charge,  8 }
};
MMOVE_T(fiend_move_melee) = { FRAME_melee01, FRAME_melee06, fiend_frames_melee, fiend_run };

MONSTERINFO_MELEE(fiend_melee) (gentity_t* self) -> void {
	M_SetAnimation(self, &fiend_move_melee);
}

/*
===============
fiend_jump_touch
Deals damage during a pounce when impacting with speed.
===============
*/
static TOUCH(fiend_jump_touch) (gentity_t* self, gentity_t* other, const trace_t& tr, bool /*otherTouchingSelf*/) -> void {
	if (self->health <= 0) {
		self->touch = nullptr;
		return;
	}

	if (self->style == 1 && other->takeDamage) {
		if (self->velocity.length() > 80) {
			const Vector3 dir = self->velocity.normalized();
			const Vector3 point = self->s.origin + (dir * self->maxs[0]);
			const int dmg = irandom(25, 40);
			Damage(other, self, self, self->velocity, point, dir, dmg, dmg, DamageFlags::Normal, ModID::Unknown);
			self->style = 0;

			self->monsterInfo.melee_debounce_time = level.time + 0.4_sec;

			// In fiend_check_landing, before calling melee:
			if (self->monsterInfo.melee_debounce_time <= level.time && self->enemy && range_to(self, self->enemy) <= RANGE_MELEE * 1.75f)
				self->monsterInfo.melee(self);
		}
	}

	if (self->groundEntity)
		self->touch = nullptr;
}

/*
===============
fiend_jump_takeoff
===============
*/
static void fiend_jump_takeoff(gentity_t* self) {
	Vector3 fwd;
	AngleVectors(self->s.angles, fwd, nullptr, nullptr);

	self->s.origin[Z] += 1.0f;
	self->velocity = fwd * 400.0f;
	self->velocity[2] = 250.0f;
	self->groundEntity = nullptr;

	self->monsterInfo.aiFlags |= AI_DUCKED;
	self->monsterInfo.attackFinished = level.time + 2.8_sec;

	self->style = 1; // damaging leap phase
	self->touch = fiend_jump_touch;

	gi.sound(self, CHAN_WEAPON, snd_pounce, 1, ATTN_NORM, 0);
}

/*
===============
fiend_check_landing
===============
*/
static void fiend_check_landing(gentity_t* self) {
        monster_jump_finished(self);

        self->owner = nullptr;

        if (self->groundEntity) {
                gi.sound(self, CHAN_WEAPON, snd_land, 1, ATTN_NORM, 0);
                self->monsterInfo.attackFinished = level.time + random_time(500_ms, 1.5_sec);

                if (self->monsterInfo.unDuck)
                        self->monsterInfo.unDuck(self);

                if (self->enemy && range_to(self, self->enemy) <= RANGE_MELEE * 2.0f)
                        self->monsterInfo.melee(self);

                return;
        }

	if (level.time > self->monsterInfo.attackFinished)
		self->monsterInfo.nextFrame = FRAME_leap03;
	else
		self->monsterInfo.nextFrame = FRAME_leap06;
}

/*
===============
fiend_jump (missile attack)
===============
*/
static MonsterFrame fiend_frames_leap[] = {
	{ ai_charge },                       // leap01 face/aim
	{ ai_charge, 0, fiend_jump_takeoff },// leap02 takeoff
	{ ai_charge },                       // leap03 flight
	{ ai_charge },                       // leap04
	{ ai_charge },                       // leap05
	{ ai_charge, 0, fiend_check_landing }, // leap06 poll landing
	{ ai_charge },                       // leap07
	{ ai_charge },                       // leap08
	{ ai_charge },                       // leap09
	{ ai_charge }                        // leap10
};
MMOVE_T(fiend_move_leap) = { FRAME_leap01, FRAME_leap10, fiend_frames_leap, fiend_run };

MONSTERINFO_ATTACK(fiend_jump) (gentity_t* self) -> void {
	M_SetAnimation(self, &fiend_move_leap);
}

static void fiend_jump_down(gentity_t* self) {
        Vector3 forward, up;

        AngleVectors(self->s.angles, forward, nullptr, up);
        self->velocity += forward * 100.0f;
        self->velocity += up * 300.0f;
}

static void fiend_jump_up(gentity_t* self) {
        Vector3 forward, up;

        AngleVectors(self->s.angles, forward, nullptr, up);
        self->velocity += forward * 200.0f;
        self->velocity += up * 450.0f;
}

static void fiend_jump_wait_land(gentity_t* self) {
        if (!monster_jump_finished(self) && self->groundEntity == nullptr) {
                self->monsterInfo.nextFrame = self->s.frame;
                return;
        }

        if (self->groundEntity) {
                gi.sound(self, CHAN_WEAPON, snd_land, 1, ATTN_NORM, 0);
                self->monsterInfo.attackFinished = level.time + random_time(500_ms, 1.5_sec);

                if (self->monsterInfo.unDuck)
                        self->monsterInfo.unDuck(self);

                if (self->enemy && range_to(self, self->enemy) <= RANGE_MELEE * 2.0f)
                        self->monsterInfo.melee(self);
        }

        self->monsterInfo.nextFrame = self->s.frame + 1;
}

static MonsterFrame fiend_frames_jump_up[] = {
        { ai_move, -8 },
        { ai_move },
        { ai_move, -8, fiend_jump_up },
        { ai_move },
        { ai_move },
        { ai_move, 0, fiend_jump_wait_land },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move }
};
MMOVE_T(fiend_move_jump_up) = { FRAME_leap01, FRAME_leap10, fiend_frames_jump_up, fiend_run };

static MonsterFrame fiend_frames_jump_down[] = {
        { ai_move },
        { ai_move },
        { ai_move, 0, fiend_jump_down },
        { ai_move },
        { ai_move },
        { ai_move, 0, fiend_jump_wait_land },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move }
};
MMOVE_T(fiend_move_jump_down) = { FRAME_leap01, FRAME_leap10, fiend_frames_jump_down, fiend_run };

static void fiend_jump_updown(gentity_t* self, BlockedJumpResult result) {
        if (!self->enemy)
                return;

        if (result == BlockedJumpResult::Jump_Turn_Up)
                M_SetAnimation(self, &fiend_move_jump_up);
        else
                M_SetAnimation(self, &fiend_move_jump_down);
}

/*
===
Blocked
===
*/
MONSTERINFO_BLOCKED(fiend_blocked) (gentity_t* self, float dist) -> bool {
        if (auto result = blocked_checkjump(self, dist); result != BlockedJumpResult::No_Jump) {
                if (result != BlockedJumpResult::Jump_Turn)
                        fiend_jump_updown(self, result);
                return true;
        }

        if (blocked_checkplat(self, dist))
                return true;

        return false;
}

/*
===============
fiend_check_melee
===============
*/
static bool fiend_check_melee(gentity_t* self) {
	return self->enemy && (range_to(self, self->enemy) <= RANGE_MELEE) && (self->monsterInfo.melee_debounce_time <= level.time);
}

/*
===============
fiend_check_jump
Prefer mid range; avoid huge vertical deltas.
===============
*/
static bool fiend_check_jump(gentity_t* self) {
	if (!self->enemy)
		return false;

	if ((self->monsterInfo.attackFinished >= level.time) || self->spawnFlags.has(SPAWNFLAG_FIEND_NOJUMP))
		return false;

	if (self->absMin[2] + 96 < self->enemy->absMin[2])
		return false;

	Vector3 flat = self->enemy->s.origin - self->s.origin;
	flat[2] = 0;
	const float d = flat.length();

	// fiend is more aggressive than dog: longer preferred band
	if (d < 120.0f || d > 300.0f)
		return false;

	// a bit of randomness to vary rhythm
	return frandom() > 0.35f;
}

/*
===============
fiend_checkattack
===============
*/
MONSTERINFO_CHECKATTACK(fiend_checkattack) (gentity_t* self) -> bool {
	if (!self->enemy || self->enemy->health <= 0)
		return false;

	if (fiend_check_melee(self)) {
		self->monsterInfo.attackState = MonsterAttackState::Melee;
		return true;
	}

	if (fiend_check_jump(self)) {
		self->monsterInfo.attackState = MonsterAttackState::Missile;
		return true;
	}

	return false;
}

/*
===============
fiend_pain
===============
*/
static MonsterFrame fiend_frames_painA[] = {
	{ ai_move },{ ai_move },{ ai_move },{ ai_move },{ ai_move }
};
MMOVE_T(fiend_move_painA) = { FRAME_pain01, FRAME_pain05, fiend_frames_painA, fiend_run };

static MonsterFrame fiend_frames_painB[] = {
	{ ai_move },{ ai_move },{ ai_move },{ ai_move },{ ai_move },{ ai_move },{ ai_move }
};
MMOVE_T(fiend_move_painB) = { FRAME_painb01, FRAME_painb07, fiend_frames_painB, fiend_run };

static PAIN(fiend_pain) (gentity_t* self, gentity_t* other, float kick, int damage, const MeansOfDeath& mod) -> void {
	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 1.8_sec;

	if (brandom())
		gi.sound(self, CHAN_VOICE, snd_pain1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, snd_pain2, 1, ATTN_NORM, 0);

	if (!M_ShouldReactToPain(self, mod))
		return;

	if (damage <= 20)
		M_SetAnimation(self, &fiend_move_painA);
	else
		M_SetAnimation(self, &fiend_move_painB);
}

/*
===============
fiend_die
===============
*/
static void fiend_collapse(gentity_t* self) {
	self->svFlags |= SVF_DEADMONSTER;
	self->maxs[2] = 0;
	gi.linkEntity(self);
}

static MonsterFrame fiend_frames_death1[] = {
	{ ai_move },{ ai_move },{ ai_move, 0, fiend_collapse },{ ai_move },
	{ ai_move },{ ai_move },{ ai_move },{ ai_move },{ ai_move },{ ai_move }
};
MMOVE_T(fiend_move_death1) = { FRAME_death01, FRAME_death10, fiend_frames_death1, monster_dead };

static MonsterFrame fiend_frames_death2[] = {
	{ ai_move },{ ai_move },{ ai_move, 0, fiend_collapse },{ ai_move },
	{ ai_move },{ ai_move },{ ai_move },{ ai_move },{ ai_move },{ ai_move },
	{ ai_move },{ ai_move }
};
MMOVE_T(fiend_move_death2) = { FRAME_deathb01, FRAME_deathb12, fiend_frames_death2, monster_dead };

static DIE(fiend_die) (gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, const Vector3& point, const MeansOfDeath& mod) -> void {
	if (M_CheckGib(self, mod)) {
		gi.sound(self, CHAN_VOICE, gi.soundIndex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		ThrowGibs(self, damage, {
			{ 3, "models/objects/gibs/bone/tris.md2" },
			{ 4, "models/objects/gibs/sm_meat/tris.md2" },
			{ "models/objects/gibs/head2/tris.md2", GIB_HEAD }
			});
		self->deadFlag = true;
		return;
	}

	if (self->deadFlag)
		return;

	gi.sound(self, CHAN_VOICE, snd_death, 1, ATTN_NORM, 0);
	self->deadFlag = true;
	self->takeDamage = true;

	if (brandom())
		M_SetAnimation(self, &fiend_move_death1);
	else
		M_SetAnimation(self, &fiend_move_death2);
}

/*
===============
fiend_setskin
===============
*/
MONSTERINFO_SETSKIN(fiend_setskin) (gentity_t* self) -> void {
	self->s.skinNum = (self->health < (self->maxHealth / 2)) ? 1 : 0;
}

/*
===============
SP_monster_fiend
===============
*/
/*QUAKED monster_fiend (1 0 0) (-32 -32 -24) (32 32 48) AMBUSH TRIGGER_SPAWN SIGHT NOJUMP x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
model="models/monsters/demon/tris.md2"
*/
void SP_monster_fiend(gentity_t* self) {
	if (!M_AllowSpawn(self)) {
		FreeEntity(self);
		return;
	}

	// sounds
	snd_claw.assign("demon/dhit2.wav");
	snd_pounce.assign("demon/djump.wav");
	snd_pain1.assign("demon/dpain1.wav");
	snd_pain2.assign("demon/dpain1.wav");
	snd_death.assign("demon/ddeath.wav");
	snd_idle1.assign("demon/idle1.wav");
        snd_idle2.assign("demon/idle2.wav");
        snd_search.assign("demon/search.wav");
        snd_sight1.assign("demon/sight1.wav");
        snd_sight2.assign("demon/sight2.wav");
        snd_land.assign("fiend/dland2.wav");

	// model + bbox
	self->moveType = MoveType::Step;
	self->solid = SOLID_BBOX;
	self->s.modelIndex = gi.modelIndex("models/monsters/demon/tris.md2");

	self->mins = { -32, -32, -24 };
	self->maxs = { 32,  32,  48 };

	// stats
	self->health = self->maxHealth = 300 * st.health_multiplier;
	self->gibHealth = -80;
	self->mass = 300;

	// callbacks
	self->pain = fiend_pain;
	self->die = fiend_die;

	MonsterInfo* m = &self->monsterInfo;

	m->stand = fiend_stand;
	m->walk = fiend_walk;
	m->run = fiend_run;
	m->attack = fiend_jump;
	m->melee = fiend_melee;
	m->sight = fiend_sight;
        m->search = fiend_search;
        m->idle = fiend_idle;
        m->checkAttack = fiend_checkattack;
        m->blocked = fiend_blocked;
        m->setSkin = fiend_setskin;

	gi.linkEntity(self);

	M_SetAnimation(self, &fiend_move_stand);
	m->scale = MODEL_SCALE;

	m->combatStyle = CombatStyle::Melee;
	m->canJump = !(self->spawnFlags & SPAWNFLAG_FIEND_NOJUMP);
	m->dropHeight = 256;
	m->jumpHeight = 72;

	walkmonster_start(self);
}
