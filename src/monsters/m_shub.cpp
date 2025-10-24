// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

OLD ONE (Shub-Niggurath) - WOR Port (rewritten to follow Widow-style structure)
- Immobile boss entity.
- Generally invulnerable; can be made vulnerable briefly via target_oldone_vulnerable,
  or killed by telefrag.
- Periodically spawns minions.

==============================================================================
*/

#include "../g_local.hpp"
#include "m_shub.hpp"

// -----------------------------------------------------------------------------
// Tunables
// -----------------------------------------------------------------------------
static constexpr Vector3 OLDONE_MINS = { -96.0f, -96.0f, -24.0f };
static constexpr Vector3 OLDONE_MAXS = { 96.0f,  96.0f, 128.0f };
static constexpr int     OLDONE_BASE_HEALTH = 1500;
static constexpr int     OLDONE_GIBHEALTH = -200;
static constexpr int     OLDONE_MASS = 1000;
static constexpr GameTime OLDONE_SPAWN_PERIOD = 5_sec;
static constexpr GameTime OLDONE_IDLE_VOX_COOLDOWN = 5_sec;

// Sounds
static cached_soundIndex s_idle;
static cached_soundIndex s_sight;
static cached_soundIndex s_pain;
static cached_soundIndex s_death;
static cached_soundIndex s_spawn;

// -----------------------------------------------------------------------------
// Forward decls
// -----------------------------------------------------------------------------
static void oldone_frame_tick(gentity_t* self);
static void oldone_spawn_minion(gentity_t* self);

// -----------------------------------------------------------------------------
// Idle helpers
// -----------------------------------------------------------------------------
/*
===============
oldone_idle
===============
*/
static void oldone_idle(gentity_t* self) {
	// Light ambience while idling
	if (self->fly_sound_debounce_time <= level.time) {
		self->fly_sound_debounce_time = level.time + OLDONE_IDLE_VOX_COOLDOWN;
		if (frandom() < 0.25f)
			gi.sound(self, CHAN_VOICE, s_idle, 1, ATTN_IDLE, 0);
	}
}

/*
===============
oldone_sight
===============
*/
MONSTERINFO_SIGHT(oldone_sight) (gentity_t* self, gentity_t* other) -> void {
	(void)other;
	gi.sound(self, CHAN_VOICE, s_sight, 1, ATTN_NORM, 0);
}

/*
===============
oldone_setskin
===============
*/
MONSTERINFO_SETSKIN(oldone_setskin) (gentity_t* self) -> void {
	// Optional: if the model supports it, flash a skin when vulnerable
	// self->s.skinNum = (self->monsterInfo.aiFlags & AI_OLDONE_VULNERABLE) ? 1 : 0;
}

// -----------------------------------------------------------------------------
// Anim state
// -----------------------------------------------------------------------------
static void oldone_stand(gentity_t* self);

static MonsterFrame oldone_frames_stand[] = {
	{ ai_stand, 0, oldone_frame_tick },
	{ ai_stand }, { ai_stand }, { ai_stand }, { ai_stand },
	{ ai_stand }, { ai_stand }, { ai_stand },
};
MMOVE_T(oldone_move_stand) = { FRAME_idle01, FRAME_idle08, oldone_frames_stand, oldone_stand };

/*
===============
oldone_stand
===============
*/
MONSTERINFO_STAND(oldone_stand) (gentity_t* self) -> void {
	self->monsterInfo.aiFlags |= AI_STAND_GROUND;
	M_SetAnimation(self, &oldone_move_stand);
}

/*
===============
oldone_run
===============
*/
MONSTERINFO_RUN(oldone_run) (gentity_t* self) -> void {
	M_SetAnimation(self, &oldone_move_stand);
}

/*
===============
oldone_walk
===============
*/
MONSTERINFO_WALK(oldone_walk) (gentity_t* self) -> void {
	M_SetAnimation(self, &oldone_move_stand);
}

// -----------------------------------------------------------------------------
// Pain/Death
// -----------------------------------------------------------------------------
/*
===============
oldone_pain
===============
*/
static PAIN(oldone_pain) (gentity_t* self, gentity_t* other, float kick, int damage, const MeansOfDeath& mod) -> void {
	(void)other; (void)kick; (void)damage;

	// Respect global pain settings (nightmare, etc.) as Widow does
	if (!M_ShouldReactToPain(self, mod))
		return;

	// While invulnerable, just vocalize occasionally and prevent soft-deaths
	if (!(self->monsterInfo.aiFlags & AI_OLDONE_VULNERABLE)) {
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
oldone_dead
===============
*/
static void oldone_dead(gentity_t* self) {
	//M_FlyCheck(self);
}

/*
===============
oldone_die
===============
*/
static DIE(oldone_die) (gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, const Vector3& point, const MeansOfDeath& mod) -> void {
	(void)inflictor; (void)attacker; (void)damage; (void)point;

	const bool telefrag = (mod.id == ModID::Telefragged || mod.id == ModID::Telefrag_Spawn);
	const bool vulnerable = !!(self->monsterInfo.aiFlags & AI_OLDONE_VULNERABLE);
	if (!telefrag && !vulnerable) {
		// Deny death, restore some health, and play pain.
		self->health = 200;
		gi.sound(self, CHAN_VOICE, s_pain, 1, ATTN_NORM, 0);
		return;
	}

	gi.sound(self, CHAN_VOICE, s_death, 1, ATTN_NORM, 0);

	// Gibbing similar to Widow
	ThrowGibs(self, damage, {
			{ 2, "models/objects/gibs/sm_meat/tris.md2" },
			{ 3, "models/objects/gibs/sm_meat/tris.md2" },
			{ 3, "models/objects/gibs/sm_meat/tris.md2" },
			{ 1, "models/objects/gibs/head2/tris.md2" },
		});

	self->deadFlag = true;
	self->takeDamage = false;
	self->svFlags |= SVF_DEADMONSTER;
	self->solid = SOLID_NOT;
	self->s.effects = EF_NONE;
	self->s.sound = 0;

	//M_Remove(self, false, true);
}

// -----------------------------------------------------------------------------
// Think/tick and minion spawns
// -----------------------------------------------------------------------------
/*
===============
oldone_spawn_minion
===============
*/
static void oldone_spawn_minion(gentity_t* self) {
	// Audio cue
	gi.sound(self, CHAN_AUTO, s_spawn, 1, ATTN_NORM, 0);

	// Spawn slightly above to avoid clipping
	Vector3 org = self->s.origin;
	org[2] += 48.0f;

	// Try a tarbaby-like helper; adjust class if your mod uses a different name
	gentity_t* baby = CreateMonster(org, self->s.angles, "monster_spawn");
	if (baby) {
		Vector3 fwd; AngleVectors(self->s.angles, fwd, nullptr, nullptr);
		baby->velocity = fwd * 120.0f;
	}
}

/*
===============
oldone_frame_tick
===============
*/
static void oldone_frame_tick(gentity_t* self) {
	// Idle chatter
	oldone_idle(self);

	// Handle vulnerability window expiry without hijacking think
	if ((self->monsterInfo.aiFlags & AI_OLDONE_VULNERABLE) && (level.time >= self->teleportTime)) {
		self->monsterInfo.aiFlags &= ~AI_OLDONE_VULNERABLE;
	}

	// Periodic minion spawning (use fireWait like Widow patterns)
	if (self->monsterInfo.fireWait <= level.time) {
		oldone_spawn_minion(self);
		self->monsterInfo.fireWait = level.time + OLDONE_SPAWN_PERIOD;
	}
}

// -----------------------------------------------------------------------------
// Precache and spawn
// -----------------------------------------------------------------------------
/*
===============
oldone_precache
===============
*/
static void oldone_precache() {
	gi.modelIndex("models/monsters/oldone/tris.md2");
	s_idle.assign("oldone/idle1.wav");
	s_sight.assign("oldone/sight1.wav");
	s_pain.assign("oldone/pain1.wav");
	s_death.assign("oldone/death1.wav");
	s_spawn.assign("oldone/spawn.wav");
}

/*
===============
oldone_configure
===============
*/
static void oldone_configure(gentity_t* self) {
	self->monsterInfo.stand = oldone_stand;
	self->monsterInfo.walk = oldone_walk;
	self->monsterInfo.run = oldone_run;
	self->monsterInfo.sight = oldone_sight;
	self->monsterInfo.setSkin = oldone_setskin;

	self->pain = oldone_pain;
	self->die = oldone_die;

	self->mins = OLDONE_MINS;
	self->maxs = OLDONE_MAXS;
	self->yawSpeed = 10;
	self->mass = OLDONE_MASS;

	int base = OLDONE_BASE_HEALTH + 500 * skill->integer;
	self->health = self->maxHealth = static_cast<int>(base * st.health_multiplier);
	if (CooperativeModeOn())
		self->health += 250 * skill->integer;
	self->gibHealth = OLDONE_GIBHEALTH;

	self->svFlags |= SVF_MONSTER;
	self->moveType = MoveType::None;
	self->solid = SOLID_BBOX;
	self->takeDamage = true;

	self->monsterInfo.scale = MODEL_SCALE;

	// Start periodic spawn timer soon after spawn
	self->monsterInfo.fireWait = level.time + 2_sec;

	gi.linkEntity(self);

	M_SetAnimation(self, &oldone_move_stand);
}

/*QUAKED monster_oldone (1 .5 0) (-96 -96 -24) (96 96 128) AMBUSH TRIGGER_SPAWN SIGHT NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Shub-Niggurath (Old One). Immobile boss. Generally invulnerable except during
brief vulnerability windows (see target_oldone_vulnerable) or by telefrag.
*/
/*
===============
SP_monster_oldone
===============
*/
void SP_monster_oldone(gentity_t* self) {
	if (!M_AllowSpawn(self)) {
		FreeEntity(self);
		return;
	}

	oldone_precache();

	self->className = "monster_oldone";
	self->s.modelIndex = gi.modelIndex("models/monsters/oldone/tris.md2");

	oldone_configure(self);
	stationarymonster_start(self);
}

// -----------------------------------------------------------------------------
// target_oldone_vulnerable: toggles vulnerability for a short duration
// -----------------------------------------------------------------------------
/*QUAKED target_oldone_vulnerable (0 .5 .8) (-8 -8 -8) (8 8 8)
Keys:
- target: name of the Shub to affect. If omitted, applies to all Shubs.
- wait: vulnerability duration in seconds (default 2.0).
When triggered, sets AI_OLDONE_VULNERABLE for the chosen monster(s).
*/

/*
===============
Use_target_oldone_vulnerable
===============
*/
static USE(Use_target_oldone_vulnerable) (gentity_t* self, gentity_t* other, gentity_t* activator) -> void {
	(void)other; (void)activator;

	const float dur = (self->wait > 0.0f) ? self->wait : 2.0f;
	GameTime until = level.time + GameTime::from_sec(dur);

	for (gentity_t* e = g_entities; e < g_entities + globals.numEntities; ++e) {
		if (!e->inUse || !e->className)
			continue;
		if (strcmp(e->className, "monster_oldone") != 0)
			continue;
		if (self->target && e->targetName && strcmp(self->target, e->targetName) != 0)
			continue;

		e->monsterInfo.aiFlags |= AI_OLDONE_VULNERABLE;
		// store window expiry; cleared in oldone_frame_tick()
		e->teleportTime = until;
	}
}

/*
===============
SP_target_oldone_vulnerable
===============
*/
void SP_target_oldone_vulnerable(gentity_t* self) {
	self->className = "target_oldone_vulnerable";
	self->use = Use_target_oldone_vulnerable;
}
