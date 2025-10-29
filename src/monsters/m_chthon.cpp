/*
==============================================================================

CHTHON (Quake 1 Boss) - WOR Port

Behavior overview
- Immobile boss that lobs lava balls at enemies on a timer.
- Normally invulnerable; a target_chthon_lightning can strike Chthon, dealing
  a big hit and briefly making him vulnerable.
- Can only be killed while vulnerable, or by telefrag.

==============================================================================
*/

#include "../g_local.hpp"
#include "m_chthon.hpp"
#include "m_flash.hpp"
#include "q1_support.hpp"

// -----------------------------------------------------------------------------
// Tunables
// -----------------------------------------------------------------------------
static constexpr Vector3  CHTHON_MINS = { -64.0f, -64.0f, -24.0f };
static constexpr Vector3  CHTHON_MAXS = { 64.0f,  64.0f, 128.0f };
static constexpr int      CHTHON_HEALTH = 1200;
static constexpr int      CHTHON_GIBHEALTH = -150;
static constexpr int      CHTHON_MASS = 1000;
static constexpr GameTime CHTHON_ATTACK_PERIOD = 2_sec;
static constexpr Vector3  CHTHON_MUZZLE_OFFSET = { 32.0f, 0.0f, 48.0f };

// Sounds
static cached_soundIndex s_idle;
static cached_soundIndex s_sight;
static cached_soundIndex s_pain;
static cached_soundIndex s_death;
static cached_soundIndex s_attack;

// -----------------------------------------------------------------------------
// Fwd decls
// -----------------------------------------------------------------------------
static void chthon_precache();
static void chthon_start(gentity_t* self);

/*
===============
chthon_idle
===============
*/
static void chthon_idle(gentity_t* self) {
	if (frandom() < 0.15f) {
		gi.sound(self, CHAN_VOICE, s_idle, 1, ATTN_IDLE, 0);
	}
}

/*
===============
chthon_sight
===============
*/
MONSTERINFO_SIGHT(chthon_sight) (gentity_t* self, gentity_t* other) -> void {
	gi.sound(self, CHAN_VOICE, s_sight, 1, ATTN_NORM, 0);
}

/*
===============
chthon_setskin
===============
*/
MONSTERINFO_SETSKIN(chthon_setskin) (gentity_t* self) -> void {
	// Optional cosmetic feedback: swap skin while vulnerable
	// 0 = normal, 1 = vulnerable (adjust to match your model skins)
	const bool vuln = (self->monsterInfo.aiFlags & AI_CHTHON_VULNERABLE) != 0;
	self->s.skinNum = vuln ? 1 : 0;
}

/*
===============
chthon_fireball
===============
*/
static void chthon_fireball(gentity_t* self) {
	if (!self->enemy || !self->enemy->inUse)
		return;

	Vector3 forward, right;
	AngleVectors(self->s.angles, forward, right, nullptr);
	Vector3 start = M_ProjectFlashSource(self, CHTHON_MUZZLE_OFFSET, forward, right);

	// Aim roughly center-mass
	Vector3 end = self->enemy->s.origin;
	end[2] += self->enemy->viewHeight * 0.5f;

	Vector3 dir = end - start;
	dir.normalize();

	gi.sound(self, CHAN_WEAPON, s_attack, 1, ATTN_NORM, 0);

	// Heavy, slow "lava ball" matching Quake 1 behaviour.
	fire_lavaball(self, start, dir, 40, 400, 40.0f, 40);
}

/*
===============
chthon_attack_anim
===============
*/
static void chthon_attack_anim(gentity_t* self);

static MonsterFrame chthon_frames_attack[] = {
	{ ai_stand }, { ai_stand }, { ai_stand, 0, chthon_fireball },
	{ ai_stand }, { ai_stand, 0, chthon_fireball }, { ai_stand }
};
MMOVE_T(chthon_move_attack) = { FRAME_attack01, FRAME_attack06, chthon_frames_attack, chthon_attack_anim };

/*
===============
chthon_attack_anim
===============
*/
static void chthon_attack_anim(gentity_t* self) {
	self->monsterInfo.attackFinished = level.time + CHTHON_ATTACK_PERIOD;
	M_SetAnimation(self, nullptr); // return to stand
}

/*
===============
chthon_stand
===============
*/
static void chthon_stand(gentity_t* self);

static MonsterFrame chthon_frames_stand[] = {
	{ ai_stand, 0, chthon_idle },
	{ ai_stand }, { ai_stand }, { ai_stand }, { ai_stand },
	{ ai_stand }, { ai_stand }, { ai_stand }
};
MMOVE_T(chthon_move_stand) = { FRAME_idle01, FRAME_idle08, chthon_frames_stand, chthon_stand };

/*
===============
chthon_stand
===============
*/
MONSTERINFO_STAND(chthon_stand) (gentity_t* self) -> void {
	self->monsterInfo.aiFlags |= AI_STAND_GROUND;
	M_SetAnimation(self, &chthon_move_stand);
}

/*
===============
chthon_run
===============
*/
MONSTERINFO_RUN(chthon_run) (gentity_t* self) -> void {
	self->monsterInfo.aiFlags |= AI_STAND_GROUND;
	M_SetAnimation(self, &chthon_move_stand);
}

/*
===============
chthon_walk
===============
*/
MONSTERINFO_WALK(chthon_walk) (gentity_t* self) -> void {
	self->monsterInfo.aiFlags |= AI_STAND_GROUND;
	M_SetAnimation(self, &chthon_move_stand);
}

/*
===============
chthon_pain
===============
*/
static PAIN(chthon_pain) (gentity_t* self, gentity_t* other, float kick, int damage, const MeansOfDeath& mod) -> void {
	const bool vuln = (self->monsterInfo.aiFlags & AI_CHTHON_VULNERABLE) != 0;
	if (!vuln) {
		// Bark but do not flinch; cap minimum health so stray hits cannot kill
		if (level.time >= self->pain_debounce_time) {
			self->pain_debounce_time = level.time + 2_sec;
			gi.sound(self, CHAN_VOICE, s_pain, 1, ATTN_NORM, 0);
		}
		if (self->health < 50)
			self->health = 50;
		return;
	}

	if (level.time >= self->pain_debounce_time) {
		self->pain_debounce_time = level.time + 1_sec;
		gi.sound(self, CHAN_VOICE, s_pain, 1, ATTN_NORM, 0);
	}
}

/*
===============
chthon_dead
===============
*/
static void chthon_dead(gentity_t* self) {
	self->mins = { -64, -64, 0 };
	self->maxs = { 64, 64, 8 };
	monster_dead(self);
}

/*
===============
chthon_die
===============
*/
static DIE(chthon_die) (gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, const Vector3& point, const MeansOfDeath& mod) -> void {
	// Chthon is only killable while vulnerable or by telefrag.
	const bool telefrag = (mod.id == ModID::Telefragged);
	const bool vulnerable = (self->monsterInfo.aiFlags & AI_CHTHON_VULNERABLE) != 0;

	if (!telefrag && !vulnerable) {
		// Refuse to die outside the vulnerability window.
		// Play a pain bark and std::clamp very low health so stray hits cannot finish him.
		if (level.time >= self->pain_debounce_time) {
			self->pain_debounce_time = level.time + 1_sec;
			gi.sound(self, CHAN_VOICE, s_pain, 1, ATTN_NORM, 0);
		}
		if (self->health < 50)
			self->health = 50;
		return;
	}

	// Normal monster die structure from here on.

	// check for gib
	if (M_CheckGib(self, mod)) {
		gi.sound(self, CHAN_VOICE, gi.soundIndex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		// Optionally alter skin on gib if your model supports it
		// self->s.skinNum /= 2;

		ThrowGibs(self, damage, {
			{ 3, "models/objects/gibs/bone/tris.md2" },
			{ 4, "models/objects/gibs/sm_meat/tris.md2" },
			{ "models/objects/gibs/head2/tris.md2", GIB_HEAD | GIB_SKINNED }
			});

		self->deadFlag = true;
		return;
	}

	if (self->deadFlag)
		return;

	// regular death
	self->deadFlag = true;
	self->takeDamage = true;

	// Chthon typically has a single death sound; if you have multiple, branch like chick_die.
	gi.sound(self, CHAN_VOICE, s_death, 1, ATTN_NORM, 0);

	// If you have death animations, trigger them here, e.g.:
	// M_SetAnimation(self, &chthon_move_death1);
	// Otherwise, remove the corpse in the standard way.
	Q1BossExplode(self);
	chthon_dead(self);
}

/*
===============
chthon_precache
===============
*/
static void chthon_precache() {
	gi.modelIndex("models/monsters/boss/tris.md2");
	s_idle.assign("boss1/idle1.wav");		// remove
	s_sight.assign("boss1/sight1.wav");
	s_pain.assign("boss1/pain.wav");
	s_death.assign("boss1/death.wav");
	s_attack.assign("boss1/throw.wav");
}

/*
===============
chthon_think
===============
*/
static THINK(chthon_think) (gentity_t* self) -> void {
	// Fire on a simple cadence when we have an enemy
	if (self->enemy && self->enemy->inUse && level.time >= self->monsterInfo.attackFinished) {
		M_SetAnimation(self, &chthon_move_attack);
	}
	self->nextThink = level.time + 250_ms;
}

/*
===============
chthon_start
===============
*/
static void chthon_start(gentity_t* self) {
	self->monsterInfo.stand = chthon_stand;
	self->monsterInfo.walk = chthon_walk;
	self->monsterInfo.run = chthon_run;
	self->monsterInfo.sight = chthon_sight;
	self->monsterInfo.setSkin = chthon_setskin;
	self->pain = chthon_pain;
	self->die = chthon_die;

	self->mins = CHTHON_MINS;
	self->maxs = CHTHON_MAXS;
	self->yawSpeed = 10;
	self->mass = CHTHON_MASS;
	self->health = self->maxHealth = CHTHON_HEALTH;
	self->gibHealth = CHTHON_GIBHEALTH;

	self->svFlags |= SVF_MONSTER;
	self->moveType = MoveType::None;   // truly stationary

	M_SetAnimation(self, &chthon_move_stand);

	// Proper stationary monster init (fixes the old walkmonster_start misuse)
	stationarymonster_start(self);

	self->think = chthon_think;
	self->nextThink = level.time + 500_ms;
}

/*QUAKED SP_monster_boss (1 .5 0) (-64 -64 -24) (64 64 128) AMBUSH TRIGGER_SPAWN SIGHT NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Chthon boss. Immobile, lobs lava balls. Vulnerable only during lightning windows.
*/
void SP_monster_boss(gentity_t* self) {
	if (!M_AllowSpawn(self)) {
		FreeEntity(self);
		return;
	}

	chthon_precache();

	self->className = "monster_chthon";
	self->s.modelIndex = gi.modelIndex("models/monsters/boss/tris.md2");

	chthon_start(self);
}

// -----------------------------------------------------------------------------
// target_chthon_lightning: applies a big damage hit and brief vulnerability
// -----------------------------------------------------------------------------

/*
===============
chthon_clear_vuln_think
===============
*/
static THINK(chthon_clear_vuln_think) (gentity_t* self) -> void {
	self->monsterInfo.aiFlags &= ~AI_CHTHON_VULNERABLE;
	self->think = chthon_think;
	self->nextThink = level.time + 250_ms;
}

/*
===============
Use_target_chthon_lightning
===============
*/
static USE(Use_target_chthon_lightning) (gentity_t* self, gentity_t* other, gentity_t* activator) -> void {
	(void)other;

	const int   lightning_damage = (self->dmg > 0) ? self->dmg : 200;
	const float vuln_seconds = (self->wait > 0.f) ? self->wait : 1.5f;

	for (gentity_t* e = g_entities; e < g_entities + globals.numEntities; ++e) {
		if (!e->inUse || !e->className) continue;
		if (strcmp(e->className, "monster_chthon") != 0) continue;
		if (self->target && e->targetName && strcmp(self->target, e->targetName) != 0) continue;

		// Mark vulnerable and deal the lightning strike
		e->monsterInfo.aiFlags |= AI_CHTHON_VULNERABLE;

		Damage(
			e,
			self,
			activator ? activator : self,
			Vector3{ 0, 0, 0 },
			e->s.origin,
			Vector3{ 0, 0, 0 },
			lightning_damage,
			0,
			DamageFlags::NoKnockback,
			ModID::Laser // use an existing MOD for lightning hit
		);

		// Optional: TE lightning effect could be added here.

		// Schedule the vulnerability to clear
		e->think = chthon_clear_vuln_think;
		e->nextThink = level.time + GameTime::from_sec(vuln_seconds);
	}

	// One-shot target
	FreeEntity(self);
}

/*
===============
SP_event_lightning
===============
*/
void SP_event_lightning(gentity_t* self) {
	self->className = "target_chthon_lightning";
	self->use = Use_target_chthon_lightning;
}
