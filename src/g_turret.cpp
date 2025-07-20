// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_turret.c

#include "g_local.h"

constexpr spawnflags_t SPAWNFLAG_TURRET_BREACH_FIRE = 65536_spawnflag;

static void AnglesNormalize(vec3_t &vec) {
	while (vec[0] > 360)
		vec[0] -= 360;
	while (vec[0] < 0)
		vec[0] += 360;
	while (vec[1] > 360)
		vec[1] -= 360;
	while (vec[1] < 0)
		vec[1] += 360;
}

MOVEINFO_BLOCKED(turret_blocked) (gentity_t *self, gentity_t *other) -> void {
	gentity_t *attacker;

	if (other->takeDamage) {
		if (self->teamMaster->owner)
			attacker = self->teamMaster->owner;
		else
			attacker = self->teamMaster;
		Damage(other, self, attacker, vec3_origin, other->s.origin, vec3_origin, self->teamMaster->dmg, 10, DAMAGE_NONE, MOD_CRUSH);
	}
}

/*QUAKED turret_breach (0 0 0) ? x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
This portion of the turret can change both pitch and yaw.
The model  should be made with a flat pitch.
It (and the associated base) need to be oriented towards 0.
Use "angle" to set the starting angle.

"speed"		default 50
"dmg"		default 10
"angle"		point this forward
"target"	point this at an info_notnull at the muzzle tip
"minPitch"	min acceptable pitch angle : default -30
"maxPitch"	max acceptable pitch angle : default 30
"minYaw"	min acceptable yaw angle   : default 0
"maxYaw"	max acceptable yaw angle   : default 360
*/

static void turret_breach_fire(gentity_t *self) {
	vec3_t f, r, u;
	vec3_t start;
	int	   damage;
	int	   speed;

	AngleVectors(self->s.angles, f, r, u);
	start = self->s.origin + (f * self->move_origin[0]);
	start += (r * self->move_origin[1]);
	start += (u * self->move_origin[2]);

	if (self->count)
		damage = self->count;
	else
		damage = (int)frandom(100, 150);
	speed = 550 + 50 * skill->integer;
	gentity_t *rocket = fire_rocket(self->teamMaster->owner->activator ? self->teamMaster->owner->activator : self->teamMaster->owner, start, f, damage, speed, 150, damage);
	rocket->s.scale = self->teamMaster->splashRadius;

	gi.positioned_sound(start, self, CHAN_WEAPON, gi.soundindex("weapons/rocklf1a.wav"), 1, ATTN_NORM, 0);
}

static THINK(turret_breach_think) (gentity_t *self) -> void {
	gentity_t *ent;
	vec3_t	 current_angles;
	vec3_t	 delta;

	current_angles = self->s.angles;
	AnglesNormalize(current_angles);

	AnglesNormalize(self->move_angles);
	if (self->move_angles[PITCH] > 180)
		self->move_angles[PITCH] -= 360;

	// clamp angles to mins & maxs
	if (self->move_angles[PITCH] > self->pos1[PITCH])
		self->move_angles[PITCH] = self->pos1[PITCH];
	else if (self->move_angles[PITCH] < self->pos2[PITCH])
		self->move_angles[PITCH] = self->pos2[PITCH];

	if ((self->move_angles[YAW] < self->pos1[YAW]) || (self->move_angles[YAW] > self->pos2[YAW])) {
		float dmin, dmax;

		dmin = fabsf(self->pos1[YAW] - self->move_angles[YAW]);
		if (dmin < -180)
			dmin += 360;
		else if (dmin > 180)
			dmin -= 360;
		dmax = fabsf(self->pos2[YAW] - self->move_angles[YAW]);
		if (dmax < -180)
			dmax += 360;
		else if (dmax > 180)
			dmax -= 360;
		if (fabsf(dmin) < fabsf(dmax))
			self->move_angles[YAW] = self->pos1[YAW];
		else
			self->move_angles[YAW] = self->pos2[YAW];
	}

	delta = self->move_angles - current_angles;
	if (delta[0] < -180)
		delta[0] += 360;
	else if (delta[0] > 180)
		delta[0] -= 360;
	if (delta[1] < -180)
		delta[1] += 360;
	else if (delta[1] > 180)
		delta[1] -= 360;
	delta[2] = 0;

	if (delta[0] > self->speed * gi.frame_time_s)
		delta[0] = self->speed * gi.frame_time_s;
	if (delta[0] < -1 * self->speed * gi.frame_time_s)
		delta[0] = -1 * self->speed * gi.frame_time_s;
	if (delta[1] > self->speed * gi.frame_time_s)
		delta[1] = self->speed * gi.frame_time_s;
	if (delta[1] < -1 * self->speed * gi.frame_time_s)
		delta[1] = -1 * self->speed * gi.frame_time_s;

	for (ent = self->teamMaster; ent; ent = ent->teamChain) {
		if (ent->noise_index) {
			if (delta[0] || delta[1]) {
				ent->s.sound = ent->noise_index;
				ent->s.loop_attenuation = ATTN_NORM;
			} else
				ent->s.sound = 0;
		}
	}

	self->aVelocity = delta * (1.0f / gi.frame_time_s);

	self->nextThink = level.time + FRAME_TIME_S;

	for (ent = self->teamMaster; ent; ent = ent->teamChain)
		ent->aVelocity[1] = self->aVelocity[1];

	// if we have a driver, adjust his velocities
	if (self->owner) {
		float  angle;
		float  target_z;
		float  diff;
		vec3_t target{};
		vec3_t dir;

		// angular is easy, just copy ours
		self->owner->aVelocity[0] = self->aVelocity[0];
		self->owner->aVelocity[1] = self->aVelocity[1];

		// x & y
		angle = self->s.angles[YAW] + self->owner->move_origin[1];
		angle *= (float)(PI * 2 / 360);
		target[0] = self->s.origin[0] + cosf(angle) * self->owner->move_origin[0];
		target[1] = self->s.origin[1] + sinf(angle) * self->owner->move_origin[0];
		target[2] = self->owner->s.origin[2];

		dir = target - self->owner->s.origin;
		self->owner->velocity[0] = dir[0] * 1.0f / gi.frame_time_s;
		self->owner->velocity[1] = dir[1] * 1.0f / gi.frame_time_s;

		// z
		angle = self->s.angles[PITCH] * (float)(PI * 2 / 360);
		target_z = self->s.origin[2] + self->owner->move_origin[0] * tan(angle) + self->owner->move_origin[2];

		diff = target_z - self->owner->s.origin[2];
		self->owner->velocity[2] = diff * 1.0f / gi.frame_time_s;

		if (self->spawnflags.has(SPAWNFLAG_TURRET_BREACH_FIRE)) {
			turret_breach_fire(self);
			self->spawnflags &= ~SPAWNFLAG_TURRET_BREACH_FIRE;
		}
	}
}

static THINK(turret_breach_finish_init) (gentity_t *self) -> void {
	// get and save info for muzzle location
	if (!self->target) {
		gi.Com_PrintFmt("{}: needs a target\n", *self);
	} else {
		self->target_ent = PickTarget(self->target);
		if (self->target_ent) {
			self->move_origin = self->target_ent->s.origin - self->s.origin;
			FreeEntity(self->target_ent);
		} else
			gi.Com_PrintFmt("{}: could not find target entity \"{}\"\n", *self, self->target);
	}

	self->teamMaster->dmg = self->dmg;
	self->teamMaster->splashRadius = self->splashRadius; // scale
	self->think = turret_breach_think;
	self->think(self);
}

void SP_turret_breach(gentity_t *self) {
	self->solid = SOLID_BSP;
	self->moveType = MOVETYPE_PUSH;

	if (st.noise)
		self->noise_index = gi.soundindex(st.noise);

	gi.setmodel(self, self->model);

	if (!self->speed)
		self->speed = 50;
	if (!self->dmg)
		self->dmg = 10;

	if (!st.minPitch)
		st.minPitch = -30;
	if (!st.maxPitch)
		st.maxPitch = 30;
	if (!st.maxYaw)
		st.maxYaw = 360;

	self->pos1[PITCH] = -1 * st.minPitch;
	self->pos1[YAW] = st.minYaw;
	self->pos2[PITCH] = -1 * st.maxPitch;
	self->pos2[YAW] = st.maxYaw;

	// scale used for rocket scale
	self->splashRadius = self->s.scale;
	self->s.scale = 0;

	self->ideal_yaw = self->s.angles[YAW];
	self->move_angles[YAW] = self->ideal_yaw;

	self->moveinfo.blocked = turret_blocked;

	self->think = turret_breach_finish_init;
	self->nextThink = level.time + FRAME_TIME_S;
	gi.linkentity(self);
}

/*QUAKED turret_base (0 0 0) ? x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
This portion of the turret changes yaw only.
MUST be teamed with a turret_breach.
*/

void SP_turret_base(gentity_t *self) {
	self->solid = SOLID_BSP;
	self->moveType = MOVETYPE_PUSH;

	if (st.noise)
		self->noise_index = gi.soundindex(st.noise);

	gi.setmodel(self, self->model);
	self->moveinfo.blocked = turret_blocked;
	gi.linkentity(self);
}

/*QUAKED turret_driver (1 .5 0) (-16 -16 -24) (16 16 32) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Must NOT be on the team with the rest of the turret parts.
Instead it must target the turret_breach.
*/

void infantry_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, const vec3_t &point, const mod_t &mod);
void infantry_stand(gentity_t *self);
void infantry_pain(gentity_t *self, gentity_t *other, float kick, int damage, const mod_t &mod);
void infantry_setskin(gentity_t *self);

static DIE(turret_driver_die) (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void {
	if (!self->deadFlag) {
		gentity_t *ent;

		// level the gun
		self->target_ent->move_angles[0] = 0;

		// remove the driver from the end of them team chain
		for (ent = self->target_ent->teamMaster; ent->teamChain != self; ent = ent->teamChain)
			;
		ent->teamChain = nullptr;
		self->teamMaster = nullptr;
		self->flags &= ~FL_TEAMSLAVE;

		self->target_ent->owner = nullptr;
		self->target_ent->teamMaster->owner = nullptr;

		self->target_ent->moveinfo.blocked = nullptr;

		// clear pitch
		self->s.angles[PITCH] = 0;
		self->moveType = MOVETYPE_STEP;

		self->think = monster_think;
	}

	infantry_die(self, inflictor, attacker, damage, point, mod);

	G_FixStuckObject(self, self->s.origin);
	AngleVectors(self->s.angles, self->velocity, nullptr, nullptr);
	self->velocity *= -50;
	self->velocity.z += 110.f;
}

bool FindTarget(gentity_t *self);

static THINK(turret_driver_think) (gentity_t *self) -> void {
	vec3_t target;
	vec3_t dir;

	self->nextThink = level.time + FRAME_TIME_S;

	if (self->enemy && (!self->enemy->inUse || self->enemy->health <= 0))
		self->enemy = nullptr;

	if (!self->enemy) {
		if (!FindTarget(self))
			return;
		self->monsterInfo.trail_time = level.time;
		self->monsterInfo.aiFlags &= ~AI_LOST_SIGHT;
	} else {
		if (visible(self, self->enemy)) {
			if (self->monsterInfo.aiFlags & AI_LOST_SIGHT) {
				self->monsterInfo.trail_time = level.time;
				self->monsterInfo.aiFlags &= ~AI_LOST_SIGHT;
			}
		} else {
			self->monsterInfo.aiFlags |= AI_LOST_SIGHT;
			return;
		}
	}

	// let the turret know where we want it to aim
	target = self->enemy->s.origin;
	target[2] += self->enemy->viewHeight;
	dir = target - self->target_ent->s.origin;
	self->target_ent->move_angles = vectoangles(dir);

	// decide if we should shoot
	if (level.time < self->monsterInfo.attack_finished)
		return;

	gtime_t reaction_time = gtime_t::from_sec(3 - skill->integer);
	if ((level.time - self->monsterInfo.trail_time) < reaction_time)
		return;

	self->monsterInfo.attack_finished = level.time + reaction_time + 1_sec;
	// FIXME how do we really want to pass this along?
	self->target_ent->spawnflags |= SPAWNFLAG_TURRET_BREACH_FIRE;
}

static THINK(turret_driver_link) (gentity_t *self) -> void {
	vec3_t	 vec{};
	gentity_t *ent;

	self->think = turret_driver_think;
	self->nextThink = level.time + FRAME_TIME_S;

	self->target_ent = PickTarget(self->target);
	self->target_ent->owner = self;
	self->target_ent->teamMaster->owner = self;
	self->s.angles = self->target_ent->s.angles;

	vec[0] = self->target_ent->s.origin[0] - self->s.origin[0];
	vec[1] = self->target_ent->s.origin[1] - self->s.origin[1];
	vec[2] = 0;
	self->move_origin[0] = vec.length();

	vec = self->s.origin - self->target_ent->s.origin;
	vec = vectoangles(vec);
	AnglesNormalize(vec);
	self->move_origin[1] = vec[1];

	self->move_origin[2] = self->s.origin[2] - self->target_ent->s.origin[2];

	// add the driver to the end of them team chain
	for (ent = self->target_ent->teamMaster; ent->teamChain; ent = ent->teamChain)
		;
	ent->teamChain = self;
	self->teamMaster = self->target_ent->teamMaster;
	self->flags |= FL_TEAMSLAVE;
}

void InfantryPrecache();

void SP_turret_driver(gentity_t *self) {
	if (deathmatch->integer) {
		FreeEntity(self);
		return;
	}

	InfantryPrecache();

	self->moveType = MOVETYPE_PUSH;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/infantry/tris.md2");
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, 32 };

	self->health = self->max_health = 100;
	self->gibHealth = GIB_HEALTH;
	self->mass = 200;
	self->viewHeight = 24;

	self->pain = infantry_pain;
	self->die = turret_driver_die;
	self->monsterInfo.stand = infantry_stand;

	self->flags |= FL_NO_KNOCKBACK;

	if (g_debug_monster_kills->integer)
		level.campaign.monstersRegistered[level.campaign.totalMonsters] = self;
	level.campaign.totalMonsters++;

	self->svFlags |= SVF_MONSTER;
	self->takeDamage = true;
	self->use = monster_use;
	self->clipMask = MASK_MONSTERSOLID;
	self->s.old_origin = self->s.origin;
	self->monsterInfo.aiFlags |= AI_STAND_GROUND;
	self->monsterInfo.setskin = infantry_setskin;

	if (st.item) {
		self->item = FindItemByClassname(st.item);
		if (!self->item)
			gi.Com_PrintFmt("{}: bad item: {}\n", *self, st.item);
	}

	self->think = turret_driver_link;
	self->nextThink = level.time + FRAME_TIME_S;

	gi.linkentity(self);
}

// invisible turret drivers so we can have unmanned turrets.
// originally designed to shoot at func_trains and such, so they
// fire at the center of the bounding box, rather than the entity's
// origin.

constexpr spawnflags_t SPAWNFLAG_TURRET_BRAIN_IGNORE_SIGHT = 1_spawnflag;

static THINK(turret_brain_think) (gentity_t *self) -> void {
	vec3_t	target;
	vec3_t	dir;
	vec3_t	endpos;
	trace_t trace;

	self->nextThink = level.time + FRAME_TIME_S;

	if (self->enemy) {
		if (!self->enemy->inUse)
			self->enemy = nullptr;
		else if (self->enemy->takeDamage && self->enemy->health <= 0)
			self->enemy = nullptr;
	}

	if (!self->enemy) {
		if (!FindTarget(self))
			return;
		self->monsterInfo.trail_time = level.time;
		self->monsterInfo.aiFlags &= ~AI_LOST_SIGHT;
	}

	endpos = self->enemy->absMax + self->enemy->absMin;
	endpos *= 0.5f;

	if (!self->spawnflags.has(SPAWNFLAG_TURRET_BRAIN_IGNORE_SIGHT)) {
		trace = gi.traceline(self->target_ent->s.origin, endpos, self->target_ent, MASK_SHOT);
		if (trace.fraction == 1 || trace.ent == self->enemy) {
			if (self->monsterInfo.aiFlags & AI_LOST_SIGHT) {
				self->monsterInfo.trail_time = level.time;
				self->monsterInfo.aiFlags &= ~AI_LOST_SIGHT;
			}
		} else {
			self->monsterInfo.aiFlags |= AI_LOST_SIGHT;
			return;
		}
	}

	// let the turret know where we want it to aim
	target = endpos;
	dir = target - self->target_ent->s.origin;
	self->target_ent->move_angles = vectoangles(dir);

	// decide if we should shoot
	if (level.time < self->monsterInfo.attack_finished)
		return;

	gtime_t reaction_time;

	if (self->delay)
		reaction_time = gtime_t::from_sec(self->delay);
	else
		reaction_time = gtime_t::from_sec(3 - skill->integer);

	if ((level.time - self->monsterInfo.trail_time) < reaction_time)
		return;

	self->monsterInfo.attack_finished = level.time + reaction_time + 1_sec;
	// FIXME how do we really want to pass this along?
	self->target_ent->spawnflags |= SPAWNFLAG_TURRET_BREACH_FIRE;
}

// =================
// =================
static THINK(turret_brain_link) (gentity_t *self) -> void {
	vec3_t	 vec{};
	gentity_t *ent;

	if (self->killtarget) {
		self->enemy = PickTarget(self->killtarget);
	}

	self->think = turret_brain_think;
	self->nextThink = level.time + FRAME_TIME_S;

	self->target_ent = PickTarget(self->target);
	self->target_ent->owner = self;
	self->target_ent->teamMaster->owner = self;
	self->s.angles = self->target_ent->s.angles;

	vec[0] = self->target_ent->s.origin[0] - self->s.origin[0];
	vec[1] = self->target_ent->s.origin[1] - self->s.origin[1];
	vec[2] = 0;
	self->move_origin[0] = vec.length();

	vec = self->s.origin - self->target_ent->s.origin;
	vec = vectoangles(vec);
	AnglesNormalize(vec);
	self->move_origin[1] = vec[1];

	self->move_origin[2] = self->s.origin[2] - self->target_ent->s.origin[2];

	// add the driver to the end of them team chain
	for (ent = self->target_ent->teamMaster; ent->teamChain; ent = ent->teamChain)
		ent->activator = self->activator; // pass along activator to breach, etc

	ent->teamChain = self;
	self->teamMaster = self->target_ent->teamMaster;
	self->flags |= FL_TEAMSLAVE;
}

// =================
// =================
static USE(turret_brain_deactivate) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	self->think = nullptr;
	self->nextThink = 0_ms;
}

// =================
// =================
static USE(turret_brain_activate) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	if (!self->enemy)
		self->enemy = activator;

	// wait at least 3 seconds to fire.
	if (self->wait)
		self->monsterInfo.attack_finished = level.time + gtime_t::from_sec(self->wait);
	else
		self->monsterInfo.attack_finished = level.time + 3_sec;
	self->use = turret_brain_deactivate;

	// Paril NOTE: rhangar1 has a turret_invisible_brain that breaks the
	// hangar ceiling; once the final rocket explodes the barrier,
	// it attempts to print "Barrier neutralized." to the rocket owner
	// who happens to be this brain rather than the player that activated
	// the turret. this resolves this by passing it along to fire_rocket.
	self->activator = activator;

	self->think = turret_brain_link;
	self->nextThink = level.time + FRAME_TIME_S;
}

/*QUAKED turret_invisible_brain (1 .5 0) (-16 -16 -16) (16 16 16) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Invisible brain to drive the turret.

Does not search for targets. If targeted, can only be turned on once
and then off once. After that they are completely disabled.

"delay" the delay between firing (default ramps for skill level)
"Target" the turret breach
"Killtarget" the item you want it to attack.
Target the brain if you want it activated later, instead of immediately. It will wait 3 seconds
before firing to acquire the target.
*/
void SP_turret_invisible_brain(gentity_t *self) {
	if (!self->killtarget) {
		gi.Com_Print("turret_invisible_brain with no killtarget!\n");
		FreeEntity(self);
		return;
	}
	if (!self->target) {
		gi.Com_Print("turret_invisible_brain with no target!\n");
		FreeEntity(self);
		return;
	}

	if (self->targetname) {
		self->use = turret_brain_activate;
	} else {
		self->think = turret_brain_link;
		self->nextThink = level.time + FRAME_TIME_S;
	}

	self->moveType = MOVETYPE_PUSH;
	gi.linkentity(self);
}
