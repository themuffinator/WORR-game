// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_misc.c

#include "g_local.h"

//=====================================================

/*
===============
Respawn_Think
===============
*/
static THINK(Respawn_Think) (gentity_t *ent) -> void {
	if (!ent->saved)
		return;

	const vec3_t &spawnOrigin = ent->saved->origin;
	const vec3_t &origin = ent->saved->origin;
	const vec3_t &mins = ent->saved->mins;
	const vec3_t &maxs = ent->saved->maxs;

	// (a) Check player visibility (in PVS and in front)
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		gentity_t *cl = &g_entities[i];
		if (!cl->inUse || !cl->client)
			continue;

		if (LocCanSee(ent, cl)) {
			ent->nextThink = level.time + 1_sec;
			return;
		}

		vec3_t forward;
		AngleVectors(cl->client->ps.viewangles, forward, nullptr, nullptr);

		vec3_t dir = spawnOrigin - cl->s.origin;
		dir.normalize();

		const float dot = dir.dot(forward);
		if (dot > 0.15f) {
			ent->nextThink = level.time + 1_sec;
			return;
		}
	}

	// (b) Telefrag check
	vec3_t p = origin + vec3_t{ 0.f, 0.f, 9.f };
	trace_t tr = gi.trace(p, mins, maxs, p, ent, CONTENTS_PLAYER | CONTENTS_MONSTER);
	if (tr.startsolid) {
		ent->nextThink = level.time + 1_sec;
		return;
	}

	// (c) Proximity check: is any client inside a 128u radius of bbox?
	vec3_t rangeMins = origin - vec3_t{ 128.f, 128.f, 128.f };
	vec3_t rangeMaxs = origin + vec3_t{ 128.f, 128.f, 128.f };

	for (int i = 0; i < MAX_CLIENTS; ++i) {
		gentity_t *cl = &g_entities[i];
		if (!cl->inUse || !cl->client)
			continue;

		vec3_t clientMins = cl->s.origin + cl->mins;
		vec3_t clientMaxs = cl->s.origin + cl->maxs;

		if (!(clientMins.x > rangeMaxs.x || clientMaxs.x < rangeMins.x ||
			clientMins.y > rangeMaxs.y || clientMaxs.y < rangeMins.y ||
			clientMins.z > rangeMaxs.z || clientMaxs.z < rangeMins.z)) {
			ent->nextThink = level.time + 1_sec;
			return;
		}
	}

	// Spawn new entity
	gentity_t *newEnt = Spawn();
	newEnt->className = ent->saved->className;
	//Q_strlcpy((char *)newEnt->className, ent->saved->className, sizeof(newEnt->className));
	newEnt->s.origin = ent->saved->origin;
	newEnt->s.angles = ent->saved->angles;
	newEnt->health = ent->saved->health;
	newEnt->dmg = ent->saved->dmg;
	newEnt->s.scale = ent->saved->scale;
	newEnt->target = ent->saved->target;
	newEnt->targetname = ent->saved->targetname;
	newEnt->spawnflags = ent->saved->spawnflags;
	newEnt->mass = ent->saved->mass;
	newEnt->mins = ent->saved->mins;
	newEnt->maxs = ent->saved->maxs;
	newEnt->model = ent->saved->model;

	newEnt->saved = ent->saved;
	ent->saved = nullptr;

	newEnt->saved->spawnFunc(newEnt);
	FreeEntity(ent);
}

//=====================================================

/*QUAKED func_group (0 0 0) ? x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Used to group brushes together just for editor convenience.
*/

//=====================================================

static USE(Use_Areaportal) (gentity_t *ent, gentity_t *other, gentity_t *activator) -> void {
	ent->count ^= 1; // toggle state
	gi.SetAreaPortalState(ent->style, ent->count);
}

/*QUAKED func_areaportal (0 0 0) ? x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP

This is a non-visible object that divides the world into
areas that are seperated when this portal is not activated.
Usually enclosed in the middle of a door.
*/
void SP_func_areaportal(gentity_t *ent) {
	ent->use = Use_Areaportal;
	ent->count = 0; // always start closed;
}

//=====================================================

/*
=================
Misc functions
=================
*/
void VelocityForDamage(int damage, vec3_t &v) {
	v[0] = 100.0f * crandom();
	v[1] = 100.0f * crandom();
	v[2] = frandom(200.0f, 300.0f);

	if (damage < 50)
		v = v * 0.7f;
	else
		v = v * 1.2f;
}

void ClipGibVelocity(gentity_t *ent) {
	if (ent->velocity[0] < -300)
		ent->velocity[0] = -300;
	else if (ent->velocity[0] > 300)
		ent->velocity[0] = 300;
	if (ent->velocity[1] < -300)
		ent->velocity[1] = -300;
	else if (ent->velocity[1] > 300)
		ent->velocity[1] = 300;
	if (ent->velocity[2] < 200)
		ent->velocity[2] = 200; // always some upwards
	else if (ent->velocity[2] > 500)
		ent->velocity[2] = 500;
}

/*
=================
gibs
=================
*/
DIE(gib_die) (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void {
	if (mod.id == MOD_CRUSH)
		FreeEntity(self);
}

static TOUCH(gib_touch) (gentity_t *self, gentity_t *other, const trace_t &tr, bool otherTouchingSelf) -> void {
	if (tr.plane.normal[2] > 0.7f) {
		self->s.angles[PITCH] = clamp(self->s.angles[PITCH], -5.0f, 5.0f);
		self->s.angles[ROLL] = clamp(self->s.angles[ROLL], -5.0f, 5.0f);
	}
}

/*
=============
GibSink

After sitting around for x seconds, fall into the ground and disappear
=============
*/
static THINK(GibSink) (gentity_t *ent) -> void {
	if (!ent->timeStamp)
		ent->timeStamp = level.time + 1_sec;

	if (level.time > ent->timeStamp) {
		ent->svFlags = SVF_NOCLIENT;
		ent->takeDamage = false;
		ent->solid = SOLID_NOT;
		FreeEntity(ent);
		return;
	}
	//ent->s.renderfx = RF_FULLBRIGHT;
	ent->nextThink = level.time + FRAME_TIME_S;
	ent->s.origin[2] -= 0.5;
}

/*
=============
GibThink
=============
*/
static THINK(GibThink) (gentity_t *self) -> void {
	if (self->timeStamp && level.time >= self->timeStamp) {

		if (g_instagib->integer)
			self->nextThink = level.time + random_time(1_sec, 5_sec);
		else
			self->nextThink = level.time + random_time(10_sec, 20_sec);

		self->think = GibSink;
		self->timeStamp = 0_sec;
		return;
	}

	if (self->velocity) {
		float p = self->s.angles.x;
		float z = self->s.angles.z;
		float speed_frac = clamp(self->velocity.lengthSquared() / (self->speed * self->speed), 0.f, 1.f);
		self->s.angles = vectoangles(self->velocity);
		self->s.angles.x = LerpAngle(p, self->s.angles.x, speed_frac);
		self->s.angles.z = z + (gi.frame_time_s * 360 * speed_frac);
	}

	self->nextThink = level.time + FRAME_TIME_S;
}

/*
=============
GibTouch
=============
*/
static TOUCH(GibTouch) (gentity_t *ent, gentity_t *other, const trace_t &tr, bool otherTouchingSelf) -> void {
	if (other == ent->owner)
		return;

	if (level.time > ent->pain_debounce_time) {

		if (tr.surface && (tr.surface->flags & SURF_SKY)) {
			FreeEntity(ent);
			return;
		}

		// bounce sound variation
		static constexpr std::array<const char *, 3> gib_sounds = {
			"player/gibimp1.wav",
			"player/gibimp2.wav",
			"player/gibimp3.wav"
		};
		const char *sfx = random_element(gib_sounds);
		gi.sound(ent, CHAN_VOICE, gi.soundindex(sfx), 1.0f, ATTN_NORM, 0);

		ent->pain_debounce_time = level.time + 500_ms;
	}
}

/*
=============
ThrowGib
=============
*/
gentity_t *ThrowGib(gentity_t *self, std::string gibname, int damage, gib_type_t type, float scale) {
	gentity_t *gib;
	vec3_t	 vd;
	vec3_t	 origin;
	vec3_t	 size;
	float	 vscale;

	if (type & GIB_HEAD) {
		gib = self;
		gib->s.event = EV_OTHER_TELEPORT;
		// remove setskin so that it doesn't set the skin wrongly later
		self->monsterInfo.setskin = nullptr;
	} else
		gib = Spawn();

	size = self->size * 0.5f;
	// since absMin is bloated by 1, un-bloat it here
	origin = (self->absMin + vec3_t{ 1, 1, 1 }) + size;

	int32_t i;

	for (i = 0; i < 3; i++) {
		gib->s.origin = origin + vec3_t{ crandom(), crandom(), crandom() }.scaled(size);

		// try 3 times to get a good, non-solid position
		if (!(gi.pointcontents(gib->s.origin) & MASK_SOLID))
			break;
	}

	if (i == 3) {
		// only free us if we're not being turned into the gib, otherwise
		// just spawn inside a wall
		if (gib != self) {
			FreeEntity(gib);
			return nullptr;
		}
	}

	gib->s.modelindex = gi.modelindex(gibname.c_str());
	gib->s.modelindex2 = 0;
	gib->s.scale = scale;
	gib->solid = SOLID_NOT;
	gib->svFlags |= SVF_DEADMONSTER;
	gib->svFlags &= ~SVF_MONSTER;
	gib->clipMask = MASK_SOLID;
	gib->s.effects = type ? EF_NONE : EF_GIB;
	gib->s.renderfx = RF_NONE;	// RF_LOW_PRIORITY | RF_FULLBRIGHT;
	//gib->s.renderfx = RF_LOW_PRIORITY;
	gib->s.renderfx |= RF_NOSHADOW;
	
	if (!(type & GIB_DEBRIS)) {
		if (type & GIB_ACID)
			gib->s.effects |= EF_GREENGIB;
		else
			gib->s.effects |= EF_GIB;
		gib->s.renderfx |= RF_IR_VISIBLE;
	}
	gib->flags |= FL_NO_KNOCKBACK | FL_NO_DAMAGE_EFFECTS;
	gib->takeDamage = true;
	gib->die = gib_die;
	gib->className = "gib";
	if (type & GIB_SKINNED)
		gib->s.skinnum = self->s.skinnum;
	else
		gib->s.skinnum = 0;
	gib->s.frame = 0;
	gib->mins = gib->maxs = {};
	gib->s.sound = 0;
	gib->monsterInfo.engine_sound = 0;

	if (GT(GT_FREEZE)) {
		gib->s.renderfx |= (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);
		gib->s.effects |= EF_COLOR_SHELL;
	}

	if (!(type & GIB_METALLIC)) {
		gib->moveType = MOVETYPE_TOSS;
		vscale = (type & GIB_ACID) ? 3.0 : 0.5;
	} else {
		gib->moveType = MOVETYPE_BOUNCE;
		vscale = 1.0;
	}

	if (type & GIB_DEBRIS) {
		vec3_t v{};
		v[0] = 100 * crandom();
		v[1] = 100 * crandom();
		v[2] = 100 + 100 * crandom();
		gib->velocity = self->velocity + (v * damage);
	} else {
		/*
		VelocityForDamage(damage, vd);
		gib->velocity = self->velocity + (vd * vscale);
		ClipGibVelocity(gib);
		*/
		VelocityForDamage(damage, vd);

		// base velocity plus scaled damage vector
		gib->velocity = self->velocity + (vd * vscale);

		// add a little random 'kick' in all three axes
		vec3_t rnd = {frandom(200.0f), frandom(200.0f), frandom(200.0f)};
		gib->velocity += rnd;

		// clamp it so you don't exceed your clip speed
		ClipGibVelocity(gib);
	}

	if (type & GIB_UPRIGHT) {
		gib->touch = gib_touch;
		gib->flags |= FL_ALWAYS_TOUCH;
	} else {
		gib->touch = type ? nullptr : GibTouch;
		gib->flags |= FL_ALWAYS_TOUCH;
	}

	gib->aVelocity[0] = 200 + frandom(400);
	gib->aVelocity[1] = 200 + frandom(400);
	gib->aVelocity[2] = 200 + frandom(400);

	gib->s.angles[PITCH] = frandom(359);
	gib->s.angles[YAW] = frandom(359);
	gib->s.angles[ROLL] = frandom(359);

	gib->think = GibThink;

	gib->nextThink = level.time + FRAME_TIME_S;
	gib->timeStamp = gib->nextThink + gtime_t::from_sec(1.5);

	gi.linkentity(gib);

	gib->watertype = gi.pointcontents(gib->s.origin);
	
	if (gib->watertype & MASK_WATER)
		gib->waterlevel = WATER_FEET;
	else
		gib->waterlevel = WATER_NONE;

	gib->clipMask = MASK_PROJECTILE;
	gib->solid = SOLID_BBOX;
	gib->svFlags |= SVF_PROJECTILE;

	return gib;
}

void ThrowClientHead(gentity_t *self, int damage) {
	vec3_t		vd;
	const char *gibname;

	if (brandom()) {
		gibname = "models/objects/gibs/head2/tris.md2";
		self->s.skinnum = 1; // second skin is player
	} else {
		gibname = "models/objects/gibs/skull/tris.md2";
		self->s.skinnum = 0;
	}

	self->s.origin[2] += 16;
	self->s.frame = 0;
	gi.setmodel(self, gibname);
	self->mins = { -8, -8, 0 };	//{ -16, -16, 0 };
	self->maxs = { 8, 8, 8 };	//{ 16, 16, 16 };

	self->takeDamage = true; // [Paril-KEX] allow takeDamage so we get crushed
	self->solid = SOLID_TRIGGER; // [Paril-KEX] make 'trigger' so we still move but don't block shots/explode
	self->svFlags |= SVF_DEADMONSTER;
	self->s.effects = EF_GIB;
	self->s.renderfx = RF_LOW_PRIORITY | RF_FULLBRIGHT | RF_IR_VISIBLE;
	self->s.sound = 0;
	self->flags |= FL_NO_KNOCKBACK | FL_NO_DAMAGE_EFFECTS;

	self->moveType = MOVETYPE_BOUNCE;
	VelocityForDamage(damage, vd);
	self->velocity += vd;

	if (self->client) {	// bodies in the queue don't have a client anymore
		self->client->anim.priority = ANIM_DEATH;
		self->client->anim.end = self->s.frame;
	} else {
		self->think = nullptr;
		self->nextThink = 0_ms;
	}

	self->think = GibThink;

	self->touch = GibTouch;

	if (g_instagib->integer)
		self->nextThink = level.time + random_time(1_sec, 5_sec);
	else
		self->nextThink = level.time + random_time(10_sec, 20_sec);

	self->timeStamp = self->nextThink + gtime_t::from_sec(1.5);

	gi.linkentity(self);
}

void BecomeExplosion1(gentity_t *self) {
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PHS, false);

	FreeEntity(self);
}

static void BecomeExplosion2(gentity_t *self) {
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION2);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PHS, false);

	FreeEntity(self);
}

/*QUAKED path_corner (.5 .3 0) (-8 -8 -8) (8 8 8) TELEPORT x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Target: next path corner
Pathtarget: gets used when an entity that has
	this path_corner targeted touches it
*/

static TOUCH(path_corner_touch) (gentity_t *self, gentity_t *other, const trace_t &tr, bool otherTouchingSelf) -> void {
	vec3_t	 v;
	gentity_t *next;

	if (other->movetarget != self)
		return;

	if (other->enemy)
		return;

	if (self->pathtarget) {
		const char *savetarget;

		savetarget = self->target;
		self->target = self->pathtarget;
		UseTargets(self, other);
		self->target = savetarget;
	}

	// see m_move; this is just so we don't needlessly check it
	self->flags |= FL_PARTIALGROUND;

	if (self->target)
		next = PickTarget(self->target);
	else
		next = nullptr;

	// [Paril-KEX] don't teleport to a point_combat, it means HOLD for them.
	if ((next) && !strcmp(next->className, "path_corner") && next->spawnflags.has(SPAWNFLAG_PATH_CORNER_TELEPORT)) {
		v = next->s.origin;
		v[2] += next->mins[2];
		v[2] -= other->mins[2];
		other->s.origin = v;
		next = PickTarget(next->target);
		other->s.event = EV_OTHER_TELEPORT;
	}

	other->goalentity = other->movetarget = next;

	if (self->wait) {
		other->monsterInfo.pauseTime = level.time + gtime_t::from_sec(self->wait);
		other->monsterInfo.stand(other);
		return;
	}

	if (!other->movetarget) {
		// N64 cutscene behavior
		if (other->hackflags & HACKFLAG_END_CUTSCENE) {
			FreeEntity(other);
			return;
		}

		other->monsterInfo.pauseTime = HOLD_FOREVER;
		other->monsterInfo.stand(other);
	} else {
		v = other->goalentity->s.origin - other->s.origin;
		other->ideal_yaw = vectoyaw(v);
	}
}

void SP_path_corner(gentity_t *self) {
	if (!self->targetname) {
		gi.Com_PrintFmt("{} with no targetname\n", *self);
		FreeEntity(self);
		return;
	}

	self->solid = SOLID_TRIGGER;
	self->touch = path_corner_touch;
	self->mins = { -8, -8, -8 };
	self->maxs = { 8, 8, 8 };
	self->svFlags |= SVF_NOCLIENT;
	gi.linkentity(self);
}

/*QUAKED point_combat (0.5 0.3 0) (-8 -8 -8) (8 8 8) HOLD x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Makes this the target of a monster and it will head here
when first activated before going after the activator.  If
hold is selected, it will stay here.
*/
static TOUCH(point_combat_touch) (gentity_t *self, gentity_t *other, const trace_t &tr, bool otherTouchingSelf) -> void {
	gentity_t *activator;

	if (other->movetarget != self)
		return;

	if (self->target) {
		other->target = self->target;
		other->goalentity = other->movetarget = PickTarget(other->target);
		if (!other->goalentity) {
			gi.Com_PrintFmt("{} target {} does not exist\n", *self, self->target);
			other->movetarget = self;
		}
		// [Paril-KEX] allow them to be re-used
		//self->target = nullptr;
	} else if (self->spawnflags.has(SPAWNFLAG_POINT_COMBAT_HOLD) && !(other->flags & (FL_SWIM | FL_FLY))) {
		// already standing
		if (other->monsterInfo.aiFlags & AI_STAND_GROUND)
			return;

		other->monsterInfo.pauseTime = HOLD_FOREVER;
		other->monsterInfo.aiFlags |= AI_STAND_GROUND | AI_REACHED_HOLD_COMBAT | AI_THIRD_EYE;
		other->monsterInfo.stand(other);
	}

	if (other->movetarget == self) {
		// [Paril-KEX] if we're holding, keep movetarget set; we will
		// use this to make sure we haven't moved too far from where
		// we want to "guard".
		if (!self->spawnflags.has(SPAWNFLAG_POINT_COMBAT_HOLD)) {
			other->target = nullptr;
			other->movetarget = nullptr;
		}

		other->goalentity = other->enemy;
		other->monsterInfo.aiFlags &= ~AI_COMBAT_POINT;
	}

	if (self->pathtarget) {
		const char *savetarget;

		savetarget = self->target;
		self->target = self->pathtarget;
		if (other->enemy && other->enemy->client)
			activator = other->enemy;
		else if (other->oldEnemy && other->oldEnemy->client)
			activator = other->oldEnemy;
		else if (other->activator && other->activator->client)
			activator = other->activator;
		else
			activator = other;
		UseTargets(self, activator);
		self->target = savetarget;
	}
}

void SP_point_combat(gentity_t *self) {
	if (deathmatch->integer && !ai_allow_dm_spawn->integer) {
		FreeEntity(self);
		return;
	}
	self->solid = SOLID_TRIGGER;
	self->touch = point_combat_touch;
	self->mins = { -8, -8, -16 };
	self->maxs = { 8, 8, 16 };
	self->svFlags = SVF_NOCLIENT;
	gi.linkentity(self);
}

/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Used as a positional target for spotlights, etc.
*/
void SP_info_null(gentity_t *self) {
	FreeEntity(self);
}

/*QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Used as a positional target for entities.
*/
void SP_info_notnull(gentity_t *self) {
	self->absMin = self->s.origin;
	self->absMax = self->s.origin;
}

/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) START_OFF ALLOW_IN_DM x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Non-displayed light.
Default light value is 300.
Default style is 0.
If targeted, will toggle between on and off.
Default _cone value is 10 (used to set size of light for spotlights)
*/

constexpr spawnflags_t SPAWNFLAG_LIGHT_START_OFF = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAG_LIGHT_ALLOW_IN_DM = 2_spawnflag;

static USE(light_use) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	if (self->spawnflags.has(SPAWNFLAG_LIGHT_START_OFF)) {
		gi.configstring(CS_LIGHTS + self->style, self->style_on);
		self->spawnflags &= ~SPAWNFLAG_LIGHT_START_OFF;
	} else {
		gi.configstring(CS_LIGHTS + self->style, self->style_off);
		self->spawnflags |= SPAWNFLAG_LIGHT_START_OFF;
	}
}

// ---------------------------------------------------------------------------------
// [Sam-KEX] For keeping track of shadow light parameters and setting them up on
// the server side.

// TODO move to level_locals_t
struct shadow_light_info_t {
	int entity_number;
	shadow_light_data_t shadowlight;
};

static shadow_light_info_t shadowlightinfo[MAX_SHADOW_LIGHTS];

const shadow_light_data_t *GetShadowLightData(int32_t entity_number) {
	for (size_t i = 0; i < level.shadowLightCount; i++) {
		if (shadowlightinfo[i].entity_number == entity_number)
			return &shadowlightinfo[i].shadowlight;
	}

	return nullptr;
}

void setup_shadow_lights() {
	for (int i = 0; i < level.shadowLightCount; ++i) {
		gentity_t *self = &g_entities[shadowlightinfo[i].entity_number];

		shadowlightinfo[i].shadowlight.lighttype = shadow_light_type_t::point;
		shadowlightinfo[i].shadowlight.conedirection = {};

		if (self->target) {
			gentity_t *target = G_FindByString<&gentity_t::targetname>(nullptr, self->target);
			if (target) {
				shadowlightinfo[i].shadowlight.conedirection = (target->s.origin - self->s.origin).normalized();
				shadowlightinfo[i].shadowlight.lighttype = shadow_light_type_t::cone;
			}
		}

		if (self->itemtarget) {
			gentity_t *target = G_FindByString<&gentity_t::targetname>(nullptr, self->itemtarget);
			if (target)
				shadowlightinfo[i].shadowlight.lightstyle = target->style;
		}

		gi.configstring(CS_SHADOWLIGHTS + i, G_Fmt("{};{};{:1};{};{:1};{:1};{:1};{};{:1};{:1};{:1};{:1}",
			self->s.number,
			(int)shadowlightinfo[i].shadowlight.lighttype,
			shadowlightinfo[i].shadowlight.radius,
			shadowlightinfo[i].shadowlight.resolution,
			shadowlightinfo[i].shadowlight.intensity,
			shadowlightinfo[i].shadowlight.fade_start,
			shadowlightinfo[i].shadowlight.fade_end,
			shadowlightinfo[i].shadowlight.lightstyle,
			shadowlightinfo[i].shadowlight.coneangle,
			shadowlightinfo[i].shadowlight.conedirection[0],
			shadowlightinfo[i].shadowlight.conedirection[1],
			shadowlightinfo[i].shadowlight.conedirection[2]).data());
	}
}

// fix an oversight in shadow light code that causes
// lights to be ordered wrong on return levels
// if the spawn functions are changed.
// this will work without changing the save/load code.
void G_LoadShadowLights() {
	for (size_t i = 0; i < level.shadowLightCount; i++) {
		const char *cstr = gi.get_configstring(CS_SHADOWLIGHTS + i);
		const char *token = COM_ParseEx(&cstr, ";");

		if (token && *token) {
			shadowlightinfo[i].entity_number = atoi(token);

			token = COM_ParseEx(&cstr, ";");
			shadowlightinfo[i].shadowlight.lighttype = (shadow_light_type_t)atoi(token);

			token = COM_ParseEx(&cstr, ";");
			shadowlightinfo[i].shadowlight.radius = atof(token);

			token = COM_ParseEx(&cstr, ";");
			shadowlightinfo[i].shadowlight.resolution = atoi(token);

			token = COM_ParseEx(&cstr, ";");
			shadowlightinfo[i].shadowlight.intensity = atof(token);

			token = COM_ParseEx(&cstr, ";");
			shadowlightinfo[i].shadowlight.fade_start = atof(token);

			token = COM_ParseEx(&cstr, ";");
			shadowlightinfo[i].shadowlight.fade_end = atof(token);

			token = COM_ParseEx(&cstr, ";");
			shadowlightinfo[i].shadowlight.lightstyle = atoi(token);

			token = COM_ParseEx(&cstr, ";");
			shadowlightinfo[i].shadowlight.coneangle = atof(token);

			token = COM_ParseEx(&cstr, ";");
			shadowlightinfo[i].shadowlight.conedirection[0] = atof(token);

			token = COM_ParseEx(&cstr, ";");
			shadowlightinfo[i].shadowlight.conedirection[1] = atof(token);

			token = COM_ParseEx(&cstr, ";");
			shadowlightinfo[i].shadowlight.conedirection[2] = atof(token);
		}
	}
}
// ---------------------------------------------------------------------------------

static void setup_dynamic_light(gentity_t *self) {
	// [Sam-KEX] Shadow stuff
	if (st.sl.data.radius > 0) {
		self->s.renderfx = RF_CASTSHADOW;
		self->itemtarget = st.sl.lightStyleTarget;

		shadowlightinfo[level.shadowLightCount].entity_number = self->s.number;
		shadowlightinfo[level.shadowLightCount].shadowlight = st.sl.data;
		level.shadowLightCount++;

		self->mins[0] = self->mins[1] = self->mins[2] = 0;
		self->maxs[0] = self->maxs[1] = self->maxs[2] = 0;

		gi.linkentity(self);
	}
}

static USE(dynamic_light_use) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	self->svFlags ^= SVF_NOCLIENT;
}

void SP_dynamic_light(gentity_t *self) {
	setup_dynamic_light(self);

	if (self->targetname) {
		self->use = dynamic_light_use;
	}

	if (self->spawnflags.has(SPAWNFLAG_LIGHT_START_OFF))
		self->svFlags ^= SVF_NOCLIENT;
}

void SP_light(gentity_t *self) {
	// no targeted lights in deathmatch, because they cause global messages
	if ((!self->targetname || (deathmatch->integer && !(self->spawnflags.has(SPAWNFLAG_LIGHT_ALLOW_IN_DM)))) && st.sl.data.radius == 0) // [Sam-KEX]
	{
		FreeEntity(self);
		return;
	}

	if (self->style >= 32) {
		self->use = light_use;

		if (!self->style_on || !*self->style_on)
			self->style_on = "m";
		else if (*self->style_on >= '0' && *self->style_on <= '9')
			self->style_on = gi.get_configstring(CS_LIGHTS + atoi(self->style_on));
		if (!self->style_off || !*self->style_off)
			self->style_off = "a";
		else if (*self->style_off >= '0' && *self->style_off <= '9')
			self->style_off = gi.get_configstring(CS_LIGHTS + atoi(self->style_off));

		if (self->spawnflags.has(SPAWNFLAG_LIGHT_START_OFF))
			gi.configstring(CS_LIGHTS + self->style, self->style_off);
		else
			gi.configstring(CS_LIGHTS + self->style, self->style_on);
	}

	setup_dynamic_light(self);
}

/*QUAKED func_wall (0 .5 .8) ? TRIGGER_SPAWN TOGGLE START_ON ANIMATED ANIMATED_FAST x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
This is just a solid wall if not inhibited

TRIGGER_SPAWN	the wall will not be present until triggered
				it will then blink in to existance; it will
				kill anything that was in it's way

TOGGLE			only valid for TRIGGER_SPAWN walls
				this allows the wall to be turned on and off

START_ON		only valid for TRIGGER_SPAWN walls
				the wall will initially be present
*/

constexpr spawnflags_t SPAWNFLAG_WALL_TRIGGER_SPAWN = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAG_WALL_TOGGLE = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAG_WALL_START_ON = 4_spawnflag;
constexpr spawnflags_t SPAWNFLAG_WALL_ANIMATED = 8_spawnflag;
constexpr spawnflags_t SPAWNFLAG_WALL_ANIMATED_FAST = 16_spawnflag;

static USE(func_wall_use) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	if (self->solid == SOLID_NOT) {
		self->solid = SOLID_BSP;
		self->svFlags &= ~SVF_NOCLIENT;
		gi.linkentity(self);
		KillBox(self, false);
	} else {
		self->solid = SOLID_NOT;
		self->svFlags |= SVF_NOCLIENT;
		gi.linkentity(self);
	}

	if (!self->spawnflags.has(SPAWNFLAG_WALL_TOGGLE))
		self->use = nullptr;
}

void SP_func_wall(gentity_t *self) {
	self->moveType = MOVETYPE_PUSH;
	gi.setmodel(self, self->model);

	if (self->spawnflags.has(SPAWNFLAG_WALL_ANIMATED))
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags.has(SPAWNFLAG_WALL_ANIMATED_FAST))
		self->s.effects |= EF_ANIM_ALLFAST;

	// just a wall
	if (!self->spawnflags.has(SPAWNFLAG_WALL_TRIGGER_SPAWN | SPAWNFLAG_WALL_TOGGLE | SPAWNFLAG_WALL_START_ON)) {
		self->solid = SOLID_BSP;
		gi.linkentity(self);
		return;
	}

	// it must be TRIGGER_SPAWN
	if (!(self->spawnflags & SPAWNFLAG_WALL_TRIGGER_SPAWN))
		self->spawnflags |= SPAWNFLAG_WALL_TRIGGER_SPAWN;

	// yell if the spawnflags are odd
	if (self->spawnflags.has(SPAWNFLAG_WALL_START_ON)) {
		if (!self->spawnflags.has(SPAWNFLAG_WALL_TOGGLE)) {
			gi.Com_Print("func_wall START_ON without TOGGLE\n");
			self->spawnflags |= SPAWNFLAG_WALL_TOGGLE;
		}
	}

	self->use = func_wall_use;
	if (self->spawnflags.has(SPAWNFLAG_WALL_START_ON)) {
		self->solid = SOLID_BSP;
	} else {
		self->solid = SOLID_NOT;
		self->svFlags |= SVF_NOCLIENT;
	}
	gi.linkentity(self);
}

// [Paril-KEX]
/*QUAKED func_animation (0 .5 .8) ? START_ON x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Similar to func_wall, but triggering it will toggle animation
state rather than going on/off.

START_ON		will start in alterate animation
*/

constexpr spawnflags_t SPAWNFLAG_ANIMATION_START_ON = 1_spawnflag;

static USE(func_animation_use) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	self->bmodel_anim.alternate = !self->bmodel_anim.alternate;
}

void SP_func_animation(gentity_t *self) {
	if (!self->bmodel_anim.enabled) {
		gi.Com_PrintFmt("{} has no animation data\n", *self);
		FreeEntity(self);
		return;
	}

	self->moveType = MOVETYPE_PUSH;
	gi.setmodel(self, self->model);
	self->solid = SOLID_BSP;

	self->use = func_animation_use;
	self->bmodel_anim.alternate = self->spawnflags.has(SPAWNFLAG_ANIMATION_START_ON);

	if (self->bmodel_anim.alternate)
		self->s.frame = self->bmodel_anim.alt_start;
	else
		self->s.frame = self->bmodel_anim.start;

	gi.linkentity(self);
}

/*QUAKED func_object (0 .5 .8) ? TRIGGER_SPAWN ANIMATED ANIMATED_FAST x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
This is solid bmodel that will fall if it's support it removed.
*/

constexpr spawnflags_t SPAWNFLAGS_OBJECT_TRIGGER_SPAWN = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_OBJECT_ANIMATED = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_OBJECT_ANIMATED_FAST = 4_spawnflag;

static TOUCH(func_object_touch) (gentity_t *self, gentity_t *other, const trace_t &tr, bool otherTouchingSelf) -> void {
	// only squash thing we fall on top of
	if (otherTouchingSelf)
		return;
	if (tr.plane.normal[2] < 1.0f)
		return;
	if (other->takeDamage == false)
		return;
	if (other->damage_debounce_time > level.time)
		return;
	Damage(other, self, self, vec3_origin, closest_point_to_box(other->s.origin, self->absMin, self->absMax), tr.plane.normal, self->dmg, 1, DAMAGE_NONE, MOD_CRUSH);
	other->damage_debounce_time = level.time + 10_hz;
}

static THINK(func_object_release) (gentity_t *self) -> void {
	self->moveType = MOVETYPE_TOSS;
	self->touch = func_object_touch;
}

static USE(func_object_use) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	self->solid = SOLID_BSP;
	self->svFlags &= ~SVF_NOCLIENT;
	self->use = nullptr;
	func_object_release(self);
	KillBox(self, false);
}

void SP_func_object(gentity_t *self) {
	gi.setmodel(self, self->model);

	self->mins[0] += 1;
	self->mins[1] += 1;
	self->mins[2] += 1;
	self->maxs[0] -= 1;
	self->maxs[1] -= 1;
	self->maxs[2] -= 1;

	if (!self->dmg)
		self->dmg = 100;

	if (!(self->spawnflags & SPAWNFLAGS_OBJECT_TRIGGER_SPAWN)) {
		self->solid = SOLID_BSP;
		self->moveType = MOVETYPE_PUSH;
		self->think = func_object_release;
		self->nextThink = level.time + 20_hz;
	} else {
		self->solid = SOLID_NOT;
		self->moveType = MOVETYPE_PUSH;
		self->use = func_object_use;
		self->svFlags |= SVF_NOCLIENT;
	}

	if (self->spawnflags.has(SPAWNFLAGS_OBJECT_ANIMATED))
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags.has(SPAWNFLAGS_OBJECT_ANIMATED_FAST))
		self->s.effects |= EF_ANIM_ALLFAST;

	self->clipMask = MASK_MONSTERSOLID;
	self->flags |= FL_NO_STANDING;

	gi.linkentity(self);
}

/*QUAKED func_explosive (0 .5 .8) ? TRIGGER_SPAWN ANIMATED ANIMATED_FAST INACTIVE ALWAYS_SHOOTABLE x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Any brush that you want to explode or break apart.  If you want an
ex0plosion, set dmg and it will do a radius explosion of that amount
at the center of the bursh.

If targeted it will not be shootable.

INACTIVE - specifies that the entity is not explodable until triggered. If you use this you must
target the entity you want to trigger it. This is the only entity approved to activate it.

health defaults to 100.

mass defaults to 75.  This determines how much debris is emitted when
it explodes.  You get one large chunk per 100 of mass (up to 8) and
one small chunk per 25 of mass (up to 16).  So 800 gives the most.
*/

constexpr spawnflags_t SPAWNFLAGS_EXPLOSIVE_TRIGGER_SPAWN = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_EXPLOSIVE_ANIMATED = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_EXPLOSIVE_ANIMATED_FAST = 4_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_EXPLOSIVE_INACTIVE = 8_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_EXPLOSIVE_ALWAYS_SHOOTABLE = 16_spawnflag;

static DIE(func_explosive_explode) (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void {
	size_t   count;
	int		 mass;
	gentity_t *master;
	bool	 done = false;

	self->takeDamage = false;

	if (self->dmg)
		RadiusDamage(self, attacker, (float)self->dmg, nullptr, (float)(self->dmg + 40), DAMAGE_NONE, MOD_EXPLOSIVE);

	self->velocity = inflictor->s.origin - self->s.origin;
	self->velocity.normalize();
	self->velocity *= 150;

	mass = self->mass;
	if (!mass)
		mass = 75;

	// big chunks
	if (mass >= 100) {
		count = mass / 100;
		if (count > 8)
			count = 8;
		ThrowGibs(self, 1, {
			{ count, "models/objects/debris1/tris.md2", GIB_METALLIC | GIB_DEBRIS }
			});
	}

	// small chunks
	count = mass / 25;
	if (count > 16)
		count = 16;
	ThrowGibs(self, 2, {
		{ count, "models/objects/debris2/tris.md2", GIB_METALLIC | GIB_DEBRIS }
		});

	// PMM - if we're part of a train, clean ourselves out of it
	if (self->flags & FL_TEAMSLAVE) {
		if (self->teamMaster) {
			master = self->teamMaster;
			if (master && master->inUse) // because mappers (other than jim (usually)) are stupid....
			{
				while (!done) {
					if (master->teamChain == self) {
						master->teamChain = self->teamChain;
						done = true;
					}
					master = master->teamChain;
				}
			}
		}
	}

	UseTargets(self, attacker);

	self->s.origin = (self->absMin + self->absMax) * 0.5f;

	if (self->noise_index)
		gi.positioned_sound(self->s.origin, self, CHAN_AUTO, self->noise_index, 1, ATTN_NORM, 0);

	if (deathmatch->integer && self->saved) {
		gentity_t *respawner = Spawn();
		respawner->think = Respawn_Think;
		respawner->nextThink = level.time + 1_min;
		respawner->saved = self->saved;
		self->saved = nullptr;
	}

	if (self->dmg)
		BecomeExplosion1(self);
	else
		FreeEntity(self);
}

static USE(func_explosive_use) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	// Paril: pass activator to explode as attacker. this fixes
	// "strike" trying to centerprint to the relay. Should be
	// a safe change.
	func_explosive_explode(self, self, activator, self->health, vec3_origin, MOD_EXPLOSIVE);
}

static USE(func_explosive_activate) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	int approved;

	approved = 0;
	// PMM - looked like target and targetname were flipped here
	if (other != nullptr && other->target) {
		if (!strcmp(other->target, self->targetname))
			approved = 1;
	}
	if (!approved && activator != nullptr && activator->target) {
		if (!strcmp(activator->target, self->targetname))
			approved = 1;
	}

	if (!approved)
		return;

	self->use = func_explosive_use;
	if (!self->health)
		self->health = 100;
	self->die = func_explosive_explode;
	self->takeDamage = true;
}
// PGM

static USE(func_explosive_spawn) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	self->solid = SOLID_BSP;
	self->svFlags &= ~SVF_NOCLIENT;
	self->use = nullptr;
	gi.linkentity(self);
	KillBox(self, false);
}

void SP_func_explosive(gentity_t *self) {
	/*
	if (deathmatch->integer)
	{ // auto-remove for deathmatch
		FreeEntity(self);
		return;
	}
	*/
	self->moveType = MOVETYPE_PUSH;

	gi.modelindex("models/objects/debris1/tris.md2");
	gi.modelindex("models/objects/debris2/tris.md2");

	gi.setmodel(self, self->model);

	if (self->spawnflags.has(SPAWNFLAGS_EXPLOSIVE_TRIGGER_SPAWN)) {
		self->svFlags |= SVF_NOCLIENT;
		self->solid = SOLID_NOT;
		self->use = func_explosive_spawn;
	} else if (self->spawnflags.has(SPAWNFLAGS_EXPLOSIVE_INACTIVE)) {
		self->solid = SOLID_BSP;
		if (self->targetname)
			self->use = func_explosive_activate;
	} else {
		self->solid = SOLID_BSP;
		if (self->targetname)
			self->use = func_explosive_use;
	}

	if (self->spawnflags.has(SPAWNFLAGS_EXPLOSIVE_ANIMATED))
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags.has(SPAWNFLAGS_EXPLOSIVE_ANIMATED_FAST))
		self->s.effects |= EF_ANIM_ALLFAST;

	if (self->spawnflags.has(SPAWNFLAGS_EXPLOSIVE_ALWAYS_SHOOTABLE) || ((self->use != func_explosive_use) && (self->use != func_explosive_activate))) {
		if (!self->health)
			self->health = 100;
		self->die = func_explosive_explode;
		self->takeDamage = true;
	}

	if (self->sounds) {
		if (self->sounds == 1)
			self->noise_index = gi.soundindex("world/brkglas.wav");
		else
			gi.Com_PrintFmt("{}: invalid \"sounds\" {}\n", *self, self->sounds);
	}

	gi.linkentity(self);
}

/*QUAKED misc_explobox (0 .5 .8) (-16 -16 0) (16 16 40) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Large exploding box.  You can override its mass (100),
health (80), and dmg (150).
*/

static TOUCH(barrel_touch) (gentity_t *self, gentity_t *other, const trace_t &tr, bool otherTouchingSelf) -> void {
	float  ratio;
	vec3_t v;

	if ((!other->groundEntity) || (other->groundEntity == self))
		return;
	else if (!otherTouchingSelf)
		return;

	ratio = (float)other->mass / (float)self->mass;
	v = self->s.origin - other->s.origin;
	M_walkmove(self, vectoyaw(v), 20 * ratio * gi.frame_time_s);
}

static THINK(barrel_explode) (gentity_t *self) -> void {
	self->takeDamage = false;

	RadiusDamage(self, self->activator, (float)self->dmg, nullptr, (float)(self->dmg + 40), DAMAGE_NONE, MOD_BARREL);

	ThrowGibs(self, (1.5f * self->dmg / 200.f), {
		{ 2, "models/objects/debris1/tris.md2", GIB_METALLIC | GIB_DEBRIS },
		{ 4, "models/objects/debris3/tris.md2", GIB_METALLIC | GIB_DEBRIS },
		{ 8, "models/objects/debris2/tris.md2", GIB_METALLIC | GIB_DEBRIS }
		});

	if (deathmatch->integer && self->saved) {
		gentity_t *respawner = Spawn();
		respawner->think = Respawn_Think;
		respawner->nextThink = level.time + 1_min;
		respawner->saved = self->saved;
		self->saved = nullptr;
	}

	if (self->groundEntity)
		BecomeExplosion2(self);
	else
		BecomeExplosion1(self);
}

static THINK(barrel_burn) (gentity_t *self) -> void {
	if (level.time >= self->timeStamp)
		self->think = barrel_explode;

	self->s.effects |= EF_BARREL_EXPLODING;
	self->s.sound = gi.soundindex("weapons/bfg__l1a.wav");
	self->nextThink = level.time + FRAME_TIME_S;
}

static DIE(barrel_delay) (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void {
	// allow "dead" barrels waiting to explode to still receive knockback
	if (self->think == barrel_burn || self->think == barrel_explode)
		return;

	// allow big booms to immediately blow up barrels (rockets, rail, other explosions) because it feels good and powerful
	if (damage >= 90) {
		self->think = barrel_explode;
		self->activator = attacker;
	} else {
		self->timeStamp = level.time + 750_ms;
		self->think = barrel_burn;
		self->activator = attacker;
	}

}

static THINK(barrel_think) (gentity_t *self) -> void {
	// the think needs to be first since later stuff may override.
	self->think = barrel_think;
	self->nextThink = level.time + FRAME_TIME_S;

	M_CatagorizePosition(self, self->s.origin, self->waterlevel, self->watertype);
	self->flags |= FL_IMMUNE_SLIME;
	self->airFinished = level.time + 100_sec;
	M_WorldEffects(self);
}

static THINK(barrel_start) (gentity_t *self) -> void {
	M_droptofloor(self);
	self->think = barrel_think;
	self->nextThink = level.time + FRAME_TIME_S;
}

void SP_misc_explobox(gentity_t *self) {
	/*
	if (deathmatch->integer)
	{ // auto-remove for deathmatch
		FreeEntity(self);
		return;
	}
	*/
	gi.modelindex("models/objects/debris1/tris.md2");
	gi.modelindex("models/objects/debris2/tris.md2");
	gi.modelindex("models/objects/debris3/tris.md2");
	gi.soundindex("weapons/bfg__l1a.wav");

	self->solid = SOLID_BBOX;
	self->moveType = MOVETYPE_STEP;

	self->model = "models/objects/barrels/tris.md2";
	self->s.modelindex = gi.modelindex(self->model);

	float scale = self->s.scale;
	if (!scale)
		scale = 1.0f;
	self->mins = { -16 * scale, -16 * scale, 0 };
	self->maxs = { 16 * scale, 16 * scale, 40 * scale };

	if (!self->mass)
		self->mass = 50;
	if (!self->health)
		self->health = 10;
	if (!self->dmg)
		self->dmg = 150;

	self->die = barrel_delay;
	self->takeDamage = true;
	self->flags |= FL_TRAP;

	self->touch = barrel_touch;

	self->think = barrel_start;
	self->nextThink = level.time + 20_hz;

	gi.linkentity(self);
}

//
// miscellaneous specialty items
//

/*QUAKED misc_blackhole (1 .5 0) (-8 -8 -8) (8 8 8) AUTO_NOISE x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
model="models/objects/black/tris.md2"
*/

constexpr spawnflags_t SPAWNFLAG_BLACKHOLE_AUTO_NOISE = 1_spawnflag;

static USE(misc_blackhole_use) (gentity_t *ent, gentity_t *other, gentity_t *activator) -> void {
	/*
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_BOSSTPORT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);
	*/
	FreeEntity(ent);
}

static THINK(misc_blackhole_think) (gentity_t *self) -> void {
	if (self->timeStamp <= level.time) {
		if (++self->s.frame >= 19)
			self->s.frame = 0;

		self->timeStamp = level.time + 10_hz;
	}

	if (self->spawnflags.has(SPAWNFLAG_BLACKHOLE_AUTO_NOISE)) {
		self->s.angles[PITCH] += 50.0f * gi.frame_time_s;
		self->s.angles[YAW] += 50.0f * gi.frame_time_s;
	}

	self->nextThink = level.time + FRAME_TIME_MS;
}

void SP_misc_blackhole(gentity_t *ent) {
	ent->moveType = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->mins = { -64, -64, 0 };
	ent->maxs = { 64, 64, 8 };
	ent->s.modelindex = gi.modelindex("models/objects/black/tris.md2");
	ent->s.renderfx = RF_TRANSLUCENT;
	ent->use = misc_blackhole_use;
	ent->think = misc_blackhole_think;
	ent->nextThink = level.time + 20_hz;

	if (ent->spawnflags.has(SPAWNFLAG_BLACKHOLE_AUTO_NOISE)) {
		ent->s.sound = gi.soundindex("world/blackhole.wav");
		ent->s.loop_attenuation = ATTN_NORM;
	}

	gi.linkentity(ent);
}

/*QUAKED misc_eastertank (1 .5 0) (-32 -32 -16) (32 32 32) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
 */

static THINK(misc_eastertank_think) (gentity_t *self) -> void {
	if (++self->s.frame < 293)
		self->nextThink = level.time + 10_hz;
	else {
		self->s.frame = 254;
		self->nextThink = level.time + 10_hz;
	}
}

void SP_misc_eastertank(gentity_t *ent) {
	ent->moveType = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->mins = { -32, -32, -16 };
	ent->maxs = { 32, 32, 32 };
	ent->s.modelindex = gi.modelindex("models/monsters/tank/tris.md2");
	ent->s.frame = 254;
	ent->think = misc_eastertank_think;
	ent->nextThink = level.time + 20_hz;
	gi.linkentity(ent);
}

/*QUAKED misc_easterchick (1 .5 0) (-32 -32 0) (32 32 32) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
 */

static THINK(misc_easterchick_think) (gentity_t *self) -> void {
	if (++self->s.frame < 247)
		self->nextThink = level.time + 10_hz;
	else {
		self->s.frame = 208;
		self->nextThink = level.time + 10_hz;
	}
}

void SP_misc_easterchick(gentity_t *ent) {
	ent->moveType = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->mins = { -32, -32, 0 };
	ent->maxs = { 32, 32, 32 };
	ent->s.modelindex = gi.modelindex("models/monsters/bitch/tris.md2");
	ent->s.frame = 208;
	ent->think = misc_easterchick_think;
	ent->nextThink = level.time + 20_hz;
	gi.linkentity(ent);
}

/*QUAKED misc_easterchick2 (1 .5 0) (-32 -32 0) (32 32 32) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
 */

static THINK(misc_easterchick2_think) (gentity_t *self) -> void {
	if (++self->s.frame < 287)
		self->nextThink = level.time + 10_hz;
	else {
		self->s.frame = 248;
		self->nextThink = level.time + 10_hz;
	}
}

void SP_misc_easterchick2(gentity_t *ent) {
	ent->moveType = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->mins = { -32, -32, 0 };
	ent->maxs = { 32, 32, 32 };
	ent->s.modelindex = gi.modelindex("models/monsters/bitch/tris.md2");
	ent->s.frame = 248;
	ent->think = misc_easterchick2_think;
	ent->nextThink = level.time + 20_hz;
	gi.linkentity(ent);
}

/*QUAKED monster_commander_body (1 .5 0) (-32 -32 0) (32 32 48) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Not really a monster, this is the Tank Commander's decapitated body.
There should be a item_commander_head that has this as it's target.
*/

static THINK(commander_body_think) (gentity_t *self) -> void {
	if (++self->s.frame < 24)
		self->nextThink = level.time + 10_hz;
	else
		self->nextThink = 0_ms;

	if (self->s.frame == 22)
		gi.sound(self, CHAN_BODY, gi.soundindex("tank/thud.wav"), 1, ATTN_NORM, 0);
}

static USE(commander_body_use) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	self->think = commander_body_think;
	self->nextThink = level.time + 10_hz;
	gi.sound(self, CHAN_BODY, gi.soundindex("tank/pain.wav"), 1, ATTN_NORM, 0);
}

static THINK(commander_body_drop) (gentity_t *self) -> void {
	self->moveType = MOVETYPE_TOSS;
	self->s.origin[2] += 2;
}

void SP_monster_commander_body(gentity_t *self) {
	self->moveType = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;
	self->model = "models/monsters/commandr/tris.md2";
	self->s.modelindex = gi.modelindex(self->model);
	self->mins = { -32, -32, 0 };
	self->maxs = { 32, 32, 48 };
	self->use = commander_body_use;
	self->takeDamage = true;
	self->flags = FL_GODMODE;
	gi.linkentity(self);

	gi.soundindex("tank/thud.wav");
	gi.soundindex("tank/pain.wav");

	self->think = commander_body_drop;
	self->nextThink = level.time + 50_hz;
}

/*QUAKED misc_banner (1 .5 0) (-4 -4 -4) (4 4 4) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
The origin is the bottom of the banner.
The banner is 128 tall.
model="models/objects/banner/tris.md2"
*/
static THINK(misc_banner_think) (gentity_t *ent) -> void {
	ent->s.frame = (ent->s.frame + 1) % 16;
	ent->nextThink = level.time + 10_hz;
}

void SP_misc_banner(gentity_t *ent) {
	ent->moveType = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/objects/banner/tris.md2");
	ent->s.frame = irandom(16);
	gi.linkentity(ent);

	ent->think = misc_banner_think;
	ent->nextThink = level.time + 10_hz;
}

/*-----------------------------------------------------------------------*/
/*QUAKED misc_ctf_banner (1 .5 0) (-4 -64 0) (4 64 248) TEAM_BLUE x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
The origin is the bottom of the banner.
The banner is 248 tall.
*/
static THINK(misc_ctf_banner_think) (gentity_t *ent) -> void {
	ent->s.frame = (ent->s.frame + 1) % 16;
	ent->nextThink = level.time + 10_hz;
}

constexpr spawnflags_t SPAWNFLAG_CTF_BANNER_BLUE = 1_spawnflag;

void SP_misc_ctf_banner(gentity_t *ent) {
	ent->moveType = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/ctf/banner/tris.md2");
	if (ent->spawnflags.has(SPAWNFLAG_CTF_BANNER_BLUE)) // TEAM_BLUE
		ent->s.skinnum = 1;

	ent->s.frame = irandom(16);
	gi.linkentity(ent);

	ent->think = misc_ctf_banner_think;
	ent->nextThink = level.time + 10_hz;
}

/*QUAKED misc_ctf_small_banner (1 .5 0) (-4 -32 0) (4 32 124) TEAM_BLUE x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
The origin is the bottom of the banner.
The banner is 124 tall.
*/
void SP_misc_ctf_small_banner(gentity_t *ent) {
	ent->moveType = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/ctf/banner/small.md2");
	if (ent->spawnflags.has(SPAWNFLAG_CTF_BANNER_BLUE)) // TEAM_BLUE
		ent->s.skinnum = 1;

	ent->s.frame = irandom(16);
	gi.linkentity(ent);

	ent->think = misc_ctf_banner_think;
	ent->nextThink = level.time + 10_hz;
}

/*QUAKED misc_deadsoldier (1 .5 0) (-16 -16 0) (16 16 16) ON_BACK ON_STOMACH BACK_DECAP FETAL_POS SIT_DECAP IMPALED x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
This is the dead player model. Comes in 6 exciting different poses!
*/

constexpr spawnflags_t SPAWNFLAGS_DEADSOLDIER_ON_BACK = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_DEADSOLDIER_ON_STOMACH = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_DEADSOLDIER_BACK_DECAP = 4_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_DEADSOLDIER_FETAL_POS = 8_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_DEADSOLDIER_SIT_DECAP = 16_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_DEADSOLDIER_IMPALED = 32_spawnflag;

static DIE(misc_deadsoldier_die) (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void {
	if (self->health > -30)
		return;

	gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
	ThrowGibs(self, damage, {
		{ 4, "models/objects/gibs/sm_meat/tris.md2" },
		{ "models/objects/gibs/head2/tris.md2", GIB_HEAD }
		});
}

void SP_misc_deadsoldier(gentity_t *ent) {
	if (deathmatch->integer) { // auto-remove for deathmatch
		FreeEntity(ent);
		return;
	}

	ent->moveType = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->s.modelindex = gi.modelindex("models/deadbods/dude/tris.md2");

	// Defaults to frame 0
	if (ent->spawnflags.has(SPAWNFLAGS_DEADSOLDIER_ON_STOMACH))
		ent->s.frame = 1;
	else if (ent->spawnflags.has(SPAWNFLAGS_DEADSOLDIER_BACK_DECAP))
		ent->s.frame = 2;
	else if (ent->spawnflags.has(SPAWNFLAGS_DEADSOLDIER_FETAL_POS))
		ent->s.frame = 3;
	else if (ent->spawnflags.has(SPAWNFLAGS_DEADSOLDIER_SIT_DECAP))
		ent->s.frame = 4;
	else if (ent->spawnflags.has(SPAWNFLAGS_DEADSOLDIER_IMPALED))
		ent->s.frame = 5;
	else if (ent->spawnflags.has(SPAWNFLAGS_DEADSOLDIER_ON_BACK))
		ent->s.frame = 0;
	else
		ent->s.frame = 0;

	ent->mins = { -16, -16, 0 };
	ent->maxs = { 16, 16, 16 };
	ent->deadFlag = true;
	ent->takeDamage = true;
	// nb: SVF_MONSTER is here so it bleeds
	ent->svFlags |= SVF_MONSTER | SVF_DEADMONSTER;
	ent->die = misc_deadsoldier_die;
	ent->monsterInfo.aiFlags |= AI_GOOD_GUY | AI_DO_NOT_COUNT;

	gi.linkentity(ent);
}

/*QUAKED misc_viper (1 .5 0) (-16 -16 0) (16 16 32) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
This is the Viper for the flyby bombing.
It is trigger_spawned, so you must have something use it for it to show up.
There must be a path for it to follow once it is activated.

"speed"		How fast the Viper should fly
*/

USE(misc_viper_use) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	self->svFlags &= ~SVF_NOCLIENT;
	self->use = train_use;
	train_use(self, other, activator);
}

void SP_misc_viper(gentity_t *ent) {
	if (!ent->target) {
		gi.Com_PrintFmt("{} without a target\n", *ent);
		FreeEntity(ent);
		return;
	}

	if (!ent->speed)
		ent->speed = 300;

	ent->moveType = MOVETYPE_PUSH;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/ships/viper/tris.md2");
	ent->mins = { -16, -16, 0 };
	ent->maxs = { 16, 16, 32 };

	ent->think = func_train_find;
	ent->nextThink = level.time + 10_hz;
	ent->use = misc_viper_use;
	ent->svFlags |= SVF_NOCLIENT;
	ent->moveinfo.accel = ent->moveinfo.decel = ent->moveinfo.speed = ent->speed;

	gi.linkentity(ent);
}

/*QUAKED misc_bigviper (1 .5 0) (-176 -120 -24) (176 120 72) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
This is a large stationary viper as seen in Paul's intro
*/
void SP_misc_bigviper(gentity_t *ent) {
	ent->moveType = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->mins = { -176, -120, -24 };
	ent->maxs = { 176, 120, 72 };
	ent->s.modelindex = gi.modelindex("models/ships/bigviper/tris.md2");
	gi.linkentity(ent);
}

/*QUAKED misc_viper_bomb (1 0 0) (-8 -8 -8) (8 8 8) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
"dmg"	how much boom should the bomb make?
*/
static TOUCH(misc_viper_bomb_touch) (gentity_t *self, gentity_t *other, const trace_t &tr, bool otherTouchingSelf) -> void {
	UseTargets(self, self->activator);

	self->s.origin[2] = self->absMin[2] + 1;
	RadiusDamage(self, self, (float)self->dmg, nullptr, (float)(self->dmg + 40), DAMAGE_NONE, MOD_BOMB);
	BecomeExplosion2(self);
}

static PRETHINK(misc_viper_bomb_prethink) (gentity_t *self) -> void {
	self->groundEntity = nullptr;

	float diff = (self->timeStamp - level.time).seconds();
	if (diff < -1.0f)
		diff = -1.0f;

	vec3_t v = self->moveinfo.dir * (1.0f + diff);
	v[2] = diff;

	diff = self->s.angles[ROLL];
	self->s.angles = vectoangles(v);
	self->s.angles[ROLL] = diff + 10;
}

static USE(misc_viper_bomb_use) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	gentity_t *viper;

	self->solid = SOLID_BBOX;
	self->svFlags &= ~SVF_NOCLIENT;
	self->s.effects |= EF_ROCKET;
	self->use = nullptr;
	self->moveType = MOVETYPE_TOSS;
	self->prethink = misc_viper_bomb_prethink;
	self->touch = misc_viper_bomb_touch;
	self->activator = activator;

	viper = G_FindByString<&gentity_t::className>(nullptr, "misc_viper");
	self->velocity = viper->moveinfo.dir * viper->moveinfo.speed;

	self->timeStamp = level.time;
	self->moveinfo.dir = viper->moveinfo.dir;
}

void SP_misc_viper_bomb(gentity_t *self) {
	self->moveType = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	self->mins = { -8, -8, -8 };
	self->maxs = { 8, 8, 8 };

	self->s.modelindex = gi.modelindex("models/objects/bomb/tris.md2");

	if (!self->dmg)
		self->dmg = 1000;

	self->use = misc_viper_bomb_use;
	self->svFlags |= SVF_NOCLIENT;

	gi.linkentity(self);
}

/*QUAKED misc_strogg_ship (1 .5 0) (-16 -16 0) (16 16 32) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
This is a Storgg ship for the flybys.
It is trigger_spawned, so you must have something use it for it to show up.
There must be a path for it to follow once it is activated.

"speed"		How fast it should fly
*/
USE(misc_strogg_ship_use) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	self->svFlags &= ~SVF_NOCLIENT;
	self->use = train_use;
	train_use(self, other, activator);
}

void SP_misc_strogg_ship(gentity_t *ent) {
	if (!ent->target) {
		gi.Com_PrintFmt("{} without a target\n", *ent);
		FreeEntity(ent);
		return;
	}

	if (!ent->speed)
		ent->speed = 300;

	ent->moveType = MOVETYPE_PUSH;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/ships/strogg1/tris.md2");
	ent->mins = { -16, -16, 0 };
	ent->maxs = { 16, 16, 32 };

	ent->think = func_train_find;
	ent->nextThink = level.time + 10_hz;
	ent->use = misc_strogg_ship_use;
	ent->svFlags |= SVF_NOCLIENT;
	ent->moveinfo.accel = ent->moveinfo.decel = ent->moveinfo.speed = ent->speed;

	gi.linkentity(ent);
}

/*QUAKED misc_satellite_dish (1 .5 0) (-64 -64 0) (64 64 128) x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
model="models/objects/satellite/tris.md2"
*/
static THINK(misc_satellite_dish_think) (gentity_t *self) -> void {
	self->s.frame++;
	if (self->s.frame < 38)
		self->nextThink = level.time + 10_hz;
}

static USE(misc_satellite_dish_use) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	self->s.frame = 0;
	self->think = misc_satellite_dish_think;
	self->nextThink = level.time + 10_hz;
}

void SP_misc_satellite_dish(gentity_t *ent) {
	ent->moveType = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->mins = { -64, -64, 0 };
	ent->maxs = { 64, 64, 128 };
	ent->s.modelindex = gi.modelindex("models/objects/satellite/tris.md2");
	ent->use = misc_satellite_dish_use;
	gi.linkentity(ent);
}

/*QUAKED light_mine1 (0 1 0) (-2 -2 -12) (2 2 12) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
 */
void SP_light_mine1(gentity_t *ent) {
	ent->moveType = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->svFlags = SVF_DEADMONSTER;
	ent->s.modelindex = gi.modelindex("models/objects/minelite/light1/tris.md2");
	gi.linkentity(ent);
}

/*QUAKED light_mine2 (0 1 0) (-2 -2 -12) (2 2 12) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
 */
void SP_light_mine2(gentity_t *ent) {
	ent->moveType = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->svFlags = SVF_DEADMONSTER;
	ent->s.modelindex = gi.modelindex("models/objects/minelite/light2/tris.md2");
	gi.linkentity(ent);
}

/*QUAKED misc_gib_arm (1 0 0) (-8 -8 -8) (8 8 8) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Intended for use with the target_spawner
*/
void SP_misc_gib_arm(gentity_t *ent) {
	gi.setmodel(ent, "models/objects/gibs/arm/tris.md2");
	ent->solid = SOLID_NOT;
	ent->s.effects |= EF_GIB;
	ent->takeDamage = true;
	ent->die = gib_die;
	ent->moveType = MOVETYPE_TOSS;
	ent->deadFlag = true;
	ent->aVelocity[0] = frandom(200);
	ent->aVelocity[1] = frandom(200);
	ent->aVelocity[2] = frandom(200);
	ent->think = FreeEntity;
	ent->nextThink = level.time + 10_sec;
	gi.linkentity(ent);
}

/*QUAKED misc_gib_leg (1 0 0) (-8 -8 -8) (8 8 8) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Intended for use with the target_spawner
*/
void SP_misc_gib_leg(gentity_t *ent) {
	gi.setmodel(ent, "models/objects/gibs/leg/tris.md2");
	ent->solid = SOLID_NOT;
	ent->s.effects |= EF_GIB;
	ent->takeDamage = true;
	ent->die = gib_die;
	ent->moveType = MOVETYPE_TOSS;
	ent->deadFlag = true;
	ent->aVelocity[0] = frandom(200);
	ent->aVelocity[1] = frandom(200);
	ent->aVelocity[2] = frandom(200);
	ent->think = FreeEntity;
	ent->nextThink = level.time + 10_sec;
	gi.linkentity(ent);
}

/*QUAKED misc_gib_head (1 0 0) (-8 -8 -8) (8 8 8) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Intended for use with the target_spawner
*/
void SP_misc_gib_head(gentity_t *ent) {
	gi.setmodel(ent, "models/objects/gibs/head/tris.md2");
	ent->solid = SOLID_NOT;
	ent->s.effects |= EF_GIB;
	ent->takeDamage = true;
	ent->die = gib_die;
	ent->moveType = MOVETYPE_TOSS;
	ent->deadFlag = true;
	ent->aVelocity[0] = frandom(200);
	ent->aVelocity[1] = frandom(200);
	ent->aVelocity[2] = frandom(200);
	ent->think = FreeEntity;
	ent->nextThink = level.time + 10_sec;
	gi.linkentity(ent);
}

//=====================================================

/*QUAKED target_character (0 0 1) ? x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
used with target_string (must be on same "team")
"count" is position in the string (starts at 1)
*/

void SP_target_character(gentity_t *self) {
	self->moveType = MOVETYPE_PUSH;
	gi.setmodel(self, self->model);
	self->solid = SOLID_BSP;
	self->s.frame = 12;
	gi.linkentity(self);
	return;
}

/*QUAKED target_string (0 0 1) (-8 -8 -8) (8 8 8) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
 */

static USE(target_string_use) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	gentity_t *e;
	int		 n;
	size_t	 l;
	char	 c;

	l = strlen(self->message);
	for (e = self->teamMaster; e; e = e->teamChain) {
		if (!e->count)
			continue;
		n = e->count - 1;
		if (n > l) {
			e->s.frame = 12;
			continue;
		}

		c = self->message[n];
		if (c >= '0' && c <= '9')
			e->s.frame = c - '0';
		else if (c == '-')
			e->s.frame = 10;
		else if (c == ':')
			e->s.frame = 11;
		else
			e->s.frame = 12;
	}
}

void SP_target_string(gentity_t *self) {
	if (!self->message)
		self->message = "";
	self->use = target_string_use;
}

/*QUAKED func_clock (0 0 1) (-8 -8 -8) (8 8 8) TIMER_UP TIMER_DOWN START_OFF MULTI_USE x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
target a target_string with this

The default is to be a time of day clock

TIMER_UP and TIMER_DOWN run for "count" seconds and then fire "pathtarget"
If START_OFF, this entity must be used before it starts

"style"		0 "xx"
			1 "xx:xx"
			2 "xx:xx:xx"
*/

constexpr spawnflags_t SPAWNFLAG_TIMER_UP = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TIMER_DOWN = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TIMER_START_OFF = 4_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TIMER_MULTI_USE = 8_spawnflag;

static void func_clock_reset(gentity_t *self) {
	self->activator = nullptr;

	if (self->spawnflags.has(SPAWNFLAG_TIMER_UP)) {
		self->health = 0;
		self->wait = (float)self->count;
	} else if (self->spawnflags.has(SPAWNFLAG_TIMER_DOWN)) {
		self->health = self->count;
		self->wait = 0;
	}
}

static void func_clock_format_countdown(gentity_t *self) {
	if (self->style == 0) {
		G_FmtTo(self->clock_message, "{:2}", self->health);
		return;
	}

	if (self->style == 1) {
		G_FmtTo(self->clock_message, "{:2}:{:02}", self->health / 60, self->health % 60);
		return;
	}

	if (self->style == 2) {
		G_FmtTo(self->clock_message, "{:2}:{:02}:{:02}", self->health / 3600,
			(self->health - (self->health / 3600) * 3600) / 60, self->health % 60);
		return;
	}
}

static THINK(func_clock_think) (gentity_t *self) -> void {
	if (!self->enemy) {
		self->enemy = G_FindByString<&gentity_t::targetname>(nullptr, self->target);
		if (!self->enemy)
			return;
	}

	if (self->spawnflags.has(SPAWNFLAG_TIMER_UP)) {
		func_clock_format_countdown(self);
		self->health++;
	} else if (self->spawnflags.has(SPAWNFLAG_TIMER_DOWN)) {
		func_clock_format_countdown(self);
		self->health--;
	} else {
		struct tm *ltime;
		time_t	   gmtime;

		time(&gmtime);
		ltime = localtime(&gmtime);
		G_FmtTo(self->clock_message, "{:2}:{:02}:{:02}", ltime->tm_hour, ltime->tm_min,
			ltime->tm_sec);
	}

	self->enemy->message = self->clock_message;
	self->enemy->use(self->enemy, self, self);

	if ((self->spawnflags.has(SPAWNFLAG_TIMER_UP) && (self->health > self->wait)) ||
		(self->spawnflags.has(SPAWNFLAG_TIMER_DOWN) && (self->health < self->wait))) {
		if (self->pathtarget) {
			const char *savetarget;

			savetarget = self->target;
			self->target = self->pathtarget;
			UseTargets(self, self->activator);
			self->target = savetarget;
		}

		if (!self->spawnflags.has(SPAWNFLAG_TIMER_MULTI_USE))
			return;

		func_clock_reset(self);

		if (self->spawnflags.has(SPAWNFLAG_TIMER_START_OFF))
			return;
	}

	self->nextThink = level.time + 1_sec;
}

static USE(func_clock_use) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	if (!self->spawnflags.has(SPAWNFLAG_TIMER_MULTI_USE))
		self->use = nullptr;
	if (self->activator)
		return;
	self->activator = activator;
	self->think(self);
}

void SP_func_clock(gentity_t *self) {
	if (!self->target) {
		gi.Com_PrintFmt("{} with no target\n", *self);
		FreeEntity(self);
		return;
	}

	if (self->spawnflags.has(SPAWNFLAG_TIMER_DOWN) && !self->count) {
		gi.Com_PrintFmt("{} with no count\n", *self);
		FreeEntity(self);
		return;
	}

	if (self->spawnflags.has(SPAWNFLAG_TIMER_UP) && (!self->count))
		self->count = 60 * 60;

	func_clock_reset(self);

	self->think = func_clock_think;

	if (self->spawnflags.has(SPAWNFLAG_TIMER_START_OFF))
		self->use = func_clock_use;
	else
		self->nextThink = level.time + 1_sec;
}

//=================================================================================

constexpr spawnflags_t SPAWNFLAG_TELEPORTER_NO_SOUND = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TELEPORTER_NO_TELEPORT_EFFECT = 2_spawnflag;

static TOUCH(teleporter_touch) (gentity_t *self, gentity_t *other, const trace_t &tr, bool otherTouchingSelf) -> void {
	if (!other->client)
		return;

	gentity_t *dest = G_FindByString<&gentity_t::targetname>(nullptr, self->target);
	if (!dest) {
		gi.Com_PrintFmt("{}: Couldn't find destination, removing.\n", *self);
		FreeEntity(self);
		return;
	}

	TeleportPlayer(other, dest->s.origin, dest->s.angles);

	bool fx = !self->spawnflags.has(SPAWNFLAG_TELEPORTER_NO_TELEPORT_EFFECT);

	// draw the teleport splash at source and on the player
	if (ClientIsPlaying(other->client)) {
		self->owner->s.event = fx ? EV_PLAYER_TELEPORT : EV_OTHER_TELEPORT;
		other->s.event = fx ? EV_PLAYER_TELEPORT : EV_OTHER_TELEPORT;
	}
}

/*QUAKED misc_teleporter (1 0 0) (-32 -32 -24) (32 32 -16) NO_SOUND NO_TELEPORT_EFFECT N64_EFFECT x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Stepping onto this disc will teleport players to the targeted misc_teleporter_dest object.
*/
constexpr spawnflags_t SPAWNFLAG_TEMEPORTER_N64_EFFECT = 4_spawnflag;

void SP_misc_teleporter(gentity_t *ent) {
	vec3_t		mins, maxs;
	bool		createSpawnPad = true;

	if (ent->target) {
		if (st.was_key_specified("mins")) {
			mins = ent->mins;
		} else mins = { -8, -8, 8 };
		if (st.was_key_specified("maxs")) {
			maxs = ent->maxs;
			if (mins)
				createSpawnPad = false;
		} else maxs = { 8, 8, 24 };
	}

	if (createSpawnPad) {
		//gi.Com_PrintFmt("{}: DMSPOT\n", *ent);
		gi.setmodel(ent, "models/objects/dmspot/tris.md2");
		ent->s.skinnum = 1;
		if (level.isN64 || ent->spawnflags.has(SPAWNFLAG_TEMEPORTER_N64_EFFECT))
			ent->s.effects = EF_TELEPORTER2;
		else
			ent->s.effects = EF_TELEPORTER;
		if (!(ent->spawnflags & SPAWNFLAG_TELEPORTER_NO_SOUND))
			ent->s.sound = gi.soundindex("world/amb10.wav");
		ent->solid = SOLID_BBOX;

		ent->mins = { -32, -32, -24 };
		ent->maxs = { 32, 32, -16 };

		gi.linkentity(ent);
	}

	// N64 has some of these for visual effects
	if (!ent->target)
		return;

	gentity_t *trig = Spawn();
	trig->className = "teleporter_touch";
	trig->touch = teleporter_touch;
	trig->solid = SOLID_TRIGGER;
	trig->target = ent->target;
	trig->owner = ent;
	trig->s.origin = ent->s.origin;
	trig->mins = mins;
	trig->maxs = maxs;

	gi.linkentity(trig);
}

/*QUAKED misc_teleporter_dest (1 0 0) (-32 -32 -24) (32 32 -16) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Point teleporters at these.
*/

void SP_misc_teleporter_dest(gentity_t *ent) {
	// Paril-KEX N64 doesn't display these
	if (level.isN64)
		return;

	CreateSpawnPad(ent);
}

/*QUAKED misc_flare (1.0 1.0 0.0) (-32 -32 -32) (32 32 32) RED GREEN BLUE LOCK_ANGLE x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Creates a flare seen in the N64 version.
*/

static constexpr spawnflags_t SPAWNFLAG_FLARE_RED = 1_spawnflag;
static constexpr spawnflags_t SPAWNFLAG_FLARE_GREEN = 2_spawnflag;
static constexpr spawnflags_t SPAWNFLAG_FLARE_BLUE = 4_spawnflag;
static constexpr spawnflags_t SPAWNFLAG_FLARE_LOCK_ANGLE = 8_spawnflag;

static USE(misc_flare_use) (gentity_t *ent, gentity_t *other, gentity_t *activator) -> void {
	ent->svFlags ^= SVF_NOCLIENT;
	gi.linkentity(ent);
}

void SP_misc_flare(gentity_t *ent) {
	ent->s.modelindex = 1;
	ent->s.renderfx = RF_FLARE;
	ent->solid = SOLID_NOT;
	ent->s.scale = st.radius;

	if (ent->spawnflags.has(SPAWNFLAG_FLARE_RED))
		ent->s.renderfx |= RF_SHELL_RED;

	if (ent->spawnflags.has(SPAWNFLAG_FLARE_GREEN))
		ent->s.renderfx |= RF_SHELL_GREEN;

	if (ent->spawnflags.has(SPAWNFLAG_FLARE_BLUE))
		ent->s.renderfx |= RF_SHELL_BLUE;

	if (ent->spawnflags.has(SPAWNFLAG_FLARE_LOCK_ANGLE))
		ent->s.renderfx |= RF_FLARE_LOCK_ANGLE;

	if (st.image && *st.image) {
		ent->s.renderfx |= RF_CUSTOMSKIN;
		ent->s.frame = gi.imageindex(st.image);
	}

	ent->mins = { -32, -32, -32 };
	ent->maxs = { 32, 32, 32 };

	ent->s.modelindex2 = st.fade_start_dist;
	ent->s.modelindex3 = st.fade_end_dist;

	if (ent->targetname)
		ent->use = misc_flare_use;

	gi.linkentity(ent);
}

static THINK(misc_hologram_think) (gentity_t *ent) -> void {
	ent->s.angles[YAW] += 100 * gi.frame_time_s;
	ent->nextThink = level.time + FRAME_TIME_MS;
	ent->s.alpha = frandom(0.2f, 0.6f);
}

/*QUAKED misc_hologram (1.0 1.0 0.0) (-16 -16 0) (16 16 32) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Ship hologram seen in the N64 version.
*/
void SP_misc_hologram(gentity_t *ent) {
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/ships/strogg1/tris.md2");
	ent->mins = { -16, -16, 0 };
	ent->maxs = { 16, 16, 32 };
	ent->s.effects = EF_HOLOGRAM;
	ent->think = misc_hologram_think;
	ent->nextThink = level.time + FRAME_TIME_MS;
	ent->s.alpha = frandom(0.2f, 0.6f);
	ent->s.scale = 0.75f;
	gi.linkentity(ent);
}


/*QUAKED misc_fireball (0 .5 .8) (-8 -8 -8) (8 8 8) NO_EXPLODE x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Lava Balls. Shamelessly copied from Quake 1, like N64 guys
probably did too.
*/

constexpr spawnflags_t SPAWNFLAG_LAVABALL_NO_EXPLODE = 1_spawnflag;

static TOUCH(fire_touch) (gentity_t *self, gentity_t *other, const trace_t &tr, bool otherTouchingSelf) -> void {
	if (self->spawnflags.has(SPAWNFLAG_LAVABALL_NO_EXPLODE)) {
		FreeEntity(self);
		return;
	}

	if (other->takeDamage)
		Damage(other, self, self, vec3_origin, self->s.origin, vec3_origin, 20, 0, DAMAGE_NONE, MOD_EXPLOSIVE);

	if (gi.pointcontents(self->s.origin) & CONTENTS_LAVA)
		FreeEntity(self);
	else
		BecomeExplosion1(self);
}

static THINK(fire_fly) (gentity_t *self) -> void {
	gentity_t *fireball = Spawn();
	fireball->s.effects = EF_FIREBALL;
	fireball->s.renderfx = RF_MINLIGHT;
	fireball->solid = SOLID_BBOX;
	fireball->moveType = MOVETYPE_TOSS;
	fireball->clipMask = MASK_SHOT;
	fireball->velocity[0] = crandom() * 50;
	fireball->velocity[1] = crandom() * 50;
	fireball->aVelocity = { crandom() * 360, crandom() * 360, crandom() * 360 };
	fireball->velocity[2] = (self->speed * 1.75f) + (frandom() * 200);
	fireball->className = "fireball";
	gi.setmodel(fireball, "models/objects/gibs/sm_meat/tris.md2");
	fireball->s.origin = self->s.origin;
	fireball->nextThink = level.time + 5_sec;
	fireball->think = FreeEntity;
	if (!deathmatch->integer)
		fireball->touch = fire_touch;
	fireball->spawnflags = self->spawnflags;
	gi.linkentity(fireball);
	self->nextThink = level.time + random_time(5_sec);
}

void SP_misc_lavaball(gentity_t *self) {
	self->className = "fireball";
	self->nextThink = level.time + random_time(5_sec);
	self->think = fire_fly;
	if (!self->speed)
		self->speed = 185;
}


void SP_info_landmark(gentity_t *self) {
	self->absMin = self->s.origin;
	self->absMax = self->s.origin;
}

constexpr spawnflags_t SPAWNFLAG_WORLD_TEXT_START_OFF = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAG_WORLD_TEXT_TRIGGER_ONCE = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAG_WORLD_TEXT_REMOVE_ON_TRIGGER = 4_spawnflag;
constexpr spawnflags_t SPAWNFLAG_WORLD_TEXT_LEADER_BOARD = 8_spawnflag;

static USE(info_world_text_use) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	if (self->activator == nullptr) {
		self->activator = activator;
		self->think(self);
	} else {
		self->nextThink = 0_ms;
		self->activator = nullptr;
	}

	if (self->spawnflags.has(SPAWNFLAG_WORLD_TEXT_TRIGGER_ONCE)) {
		self->use = nullptr;
	}

	if (self->target != nullptr) {
		gentity_t *target = PickTarget(self->target);
		if (target != nullptr && target->inUse) {
			if (target->use) {
				target->use(target, self, self);
			}
		}
	}

	if (self->spawnflags.has(SPAWNFLAG_WORLD_TEXT_REMOVE_ON_TRIGGER)) {
		FreeEntity(self);
	}
}

static THINK(info_world_text_think) (gentity_t *self) -> void {
	rgba_t color = rgba_white;
	const char *s = self->message;

	switch (self->sounds) {
	case 0:
		color = rgba_white;
		break;

	case 1:
		color = rgba_red;
		break;

	case 2:
		color = rgba_blue;
		break;

	case 3:
		color = rgba_green;
		break;

	case 4:
		color = rgba_yellow;
		break;

	case 5:
		color = rgba_black;
		break;

	case 6:
		color = rgba_cyan;
		break;

	case 7:
		color = rgba_orange;
		break;

	default:
		color = rgba_white;
		gi.Com_PrintFmt("{}: invalid color\n", *self);
		break;
	}

	if (deathmatch->integer && self->spawnflags.has(SPAWNFLAG_WORLD_TEXT_LEADER_BOARD)) {
		gentity_t *e = &g_entities[level.sorted_clients[0] + 1];
		if (level.matchState == MatchState::MATCH_WARMUP_READYUP)
			s = G_Fmt("Welcome to WOR\nKindly ready the fuck up...").data();
		else if (level.matchState <= MatchState::MATCH_WARMUP_DEFAULT)
			s = G_Fmt("Welcome to WOR").data();
		else if (e && e->client && level.match.totalDeaths > 0 && e->client->resp.score > 0)
			s = G_Fmt("{} is in the lead\nwith a score of {}",
				e->client->sess.netName, e->client->resp.score).data();
	}

	if (self->s.angles[YAW] == -3.0f) {
		gi.Draw_OrientedWorldText(self->s.origin, (s != nullptr || s[0]) ? s : self->message, color, self->size[2], FRAME_TIME_MS.seconds(), true);
	} else {
		vec3_t textAngle = { 0.0f, 0.0f, 0.0f };
		textAngle[YAW] = anglemod(self->s.angles[YAW]) + 180;
		if (textAngle[YAW] > 360.0f) {
			textAngle[YAW] -= 360.0f;
		}
		gi.Draw_StaticWorldText(self->s.origin, textAngle, (s != nullptr || s[0]) ? s : self->message, color, self->size[2], FRAME_TIME_MS.seconds(), true);
	}
	self->nextThink = level.time + FRAME_TIME_MS;
}

/*QUAKED info_world_text (1.0 1.0 0.0) (-16 -16 0) (16 16 32) START_OFF TRIGGER_ONCE REMOVE_ON_TRIGGER LEADER x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Designer placed in world text for debugging.
*/
void SP_info_world_text(gentity_t *self) {
	if (self->message == nullptr) {
		if (!self->spawnflags.has(SPAWNFLAG_WORLD_TEXT_LEADER_BOARD)) {
			gi.Com_PrintFmt("{}: no message\n", *self);
			FreeEntity(self);
			return;
		}
	} // not much point without something to print...

	self->think = info_world_text_think;
	self->use = info_world_text_use;
	self->size[2] = st.radius ? st.radius : 0.2f;

	if (!self->spawnflags.has(SPAWNFLAG_WORLD_TEXT_START_OFF)) {
		self->nextThink = level.time + FRAME_TIME_MS;
		self->activator = self;
	}
}

#include "monsters/m_player.h"

static USE(misc_player_mannequin_use) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	self->monsterInfo.aiFlags |= AI_TARGET_ANGER;
	self->enemy = activator;

	switch (self->count) {
	case GESTURE_FLIP_OFF:
		self->s.frame = FRAME_flip01;
		self->monsterInfo.nextFrame = FRAME_flip12;
		break;

	case GESTURE_SALUTE:
		self->s.frame = FRAME_salute01;
		self->monsterInfo.nextFrame = FRAME_salute11;
		break;

	case GESTURE_TAUNT:
		self->s.frame = FRAME_taunt01;
		self->monsterInfo.nextFrame = FRAME_taunt17;
		break;

	case GESTURE_WAVE:
		self->s.frame = FRAME_wave01;
		self->monsterInfo.nextFrame = FRAME_wave11;
		break;

	case GESTURE_POINT:
		self->s.frame = FRAME_point01;
		self->monsterInfo.nextFrame = FRAME_point12;
		break;
	}
}

static THINK(misc_player_mannequin_think) (gentity_t *self) -> void {
	if (self->teleport_time <= level.time) {
		self->s.frame++;

		if ((self->monsterInfo.aiFlags & AI_TARGET_ANGER) == 0) {
			if (self->s.frame > FRAME_stand40) {
				self->s.frame = FRAME_stand01;
			}
		} else {
			if (self->s.frame > self->monsterInfo.nextFrame) {
				self->s.frame = FRAME_stand01;
				self->monsterInfo.aiFlags &= ~AI_TARGET_ANGER;
				self->enemy = nullptr;
			}
		}

		self->teleport_time = level.time + 10_hz;
	}

	if (self->enemy != nullptr) {
		const vec3_t vec = (self->enemy->s.origin - self->s.origin);
		self->ideal_yaw = vectoyaw(vec);
		M_ChangeYaw(self);
	}

	self->nextThink = level.time + FRAME_TIME_MS;
}

static void SetupMannequinModel(gentity_t *self, const int32_t modelType, const char *weapon, const char *skin) {
	const char *modelName = nullptr;
	const char *defaultSkin = nullptr;

	switch (modelType) {
	case 1: {
		self->s.skinnum = (MAX_CLIENTS - 1);
		modelName = "female";
		defaultSkin = "venus";
		break;
	}

	case 2: {
		self->s.skinnum = (MAX_CLIENTS - 2);
		modelName = "male";
		defaultSkin = "rampage";
		break;
	}

	case 3: {
		self->s.skinnum = (MAX_CLIENTS - 3);
		modelName = "cyborg";
		defaultSkin = "oni911";
		break;
	}

	default: {
		self->s.skinnum = (MAX_CLIENTS - 1);
		modelName = "female";
		defaultSkin = "venus";
		break;
	}
	}

	if (modelName != nullptr) {
		self->model = G_Fmt("players/{}/tris.md2", modelName).data();

		const char *weaponName = nullptr;
		if (weapon != nullptr) {
			weaponName = G_Fmt("players/{}/{}.md2", modelName, weapon).data();
		} else {
			weaponName = G_Fmt("players/{}/{}.md2", modelName, "w_hyperblaster").data();
		}
		self->s.modelindex2 = gi.modelindex(weaponName);

		const char *skinName = nullptr;
		if (skin != nullptr) {
			skinName = G_Fmt("mannequin\\{}/{}", modelName, skin).data();
		} else {
			skinName = G_Fmt("mannequin\\{}/{}", modelName, defaultSkin).data();
		}
		gi.configstring(CS_PLAYERSKINS + self->s.skinnum, skinName);
	}
}

/*QUAKED misc_player_mannequin (1.0 1.0 0.0) (-32 -32 -32) (32 32 32) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
	Creates a player mannequin that stands around.

	NOTE: this is currently very limited, and only allows one unique model
	from each of the three player model types.

 "distance"		- Sets the type of gesture mannequin when use when triggered
 "height"		- Sets the type of model to use ( valid numbers: 1 - 3 )
 "goals"		- Name of the weapon to use.
 "image"		- Name of the player skin to use.
 "radius"		- How much to scale the model in-game
*/
void SP_misc_player_mannequin(gentity_t *self) {
	self->moveType = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;
	if (!st.was_key_specified("effects"))
		self->s.effects = EF_NONE;
	if (!st.was_key_specified("renderfx"))
		self->s.renderfx = RF_MINLIGHT;
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, 32 };
	self->yaw_speed = 30;
	self->ideal_yaw = 0;
	self->teleport_time = level.time + 10_hz;
	self->s.modelindex = MODELINDEX_PLAYER;
	self->count = st.distance;

	SetupMannequinModel(self, st.height, st.goals, st.image);

	self->s.scale = 1.0f;
	if (ai_model_scale->value > 0.0f) {
		self->s.scale = ai_model_scale->value;
	} else if (st.radius > 0.0f) {
		self->s.scale = st.radius;
	}

	self->mins *= self->s.scale;
	self->maxs *= self->s.scale;

	self->think = misc_player_mannequin_think;
	self->nextThink = level.time + FRAME_TIME_MS;

	if (self->targetname) {
		self->use = misc_player_mannequin_use;
	}

	gi.linkentity(self);
}

/*QUAKED misc_model (1 0 0) (-8 -8 -8) (8 8 8) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
*/
void SP_misc_model(gentity_t *ent) {
	gi.setmodel(ent, ent->model);
	gi.linkentity(ent);
}


/*QUAKED misc_crashviper (1 .5 0) (-176 -120 -24) (176 120 72) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
A large viper about to crash.
*/
void SP_misc_crashviper(gentity_t *ent) {
	if (!ent->target) {
		gi.Com_PrintFmt("{}: no target\n", *ent);
		FreeEntity(ent);
		return;
	}

	if (!ent->speed)
		ent->speed = 300;

	ent->moveType = MOVETYPE_PUSH;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/ships/bigviper/tris.md2");
	ent->mins = { -16, -16, 0 };
	ent->maxs = { 16, 16, 32 };

	ent->think = func_train_find;
	ent->nextThink = level.time + 10_hz;
	ent->use = misc_viper_use;
	ent->svFlags |= SVF_NOCLIENT;
	ent->moveinfo.accel = ent->moveinfo.decel = ent->moveinfo.speed = ent->speed;

	gi.linkentity(ent);
}

/*QUAKED misc_viper_missile (1 0 0) (-8 -8 -8) (8 8 8) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
"dmg"	how much boom should the bomb make? the default value is 250
*/

static USE(misc_viper_missile_use) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	vec3_t forward, right, up;
	vec3_t start, dir;
	vec3_t vec;

	AngleVectors(self->s.angles, forward, right, up);

	self->enemy = G_FindByString<&gentity_t::targetname>(nullptr, self->target);

	vec = self->enemy->s.origin;

	start = self->s.origin;
	dir = vec - start;
	dir.normalize();

	monster_fire_rocket(self, start, dir, self->dmg, 500, MZ2_CHICK_ROCKET_1);

	self->nextThink = level.time + 10_hz;
	self->think = FreeEntity;
}

void SP_misc_viper_missile(gentity_t *self) {
	self->moveType = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	self->mins = { -8, -8, -8 };
	self->maxs = { 8, 8, 8 };

	if (!self->dmg)
		self->dmg = 250;

	self->s.modelindex = gi.modelindex("models/objects/bomb/tris.md2");

	self->use = misc_viper_missile_use;
	self->svFlags |= SVF_NOCLIENT;

	gi.linkentity(self);
}

/*QUAKED misc_transport (1 0 0) (-8 -8 -8) (8 8 8) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Maxx's transport at end of game
*/
void SP_misc_transport(gentity_t *ent) {
	if (!ent->target) {
		gi.Com_PrintFmt("{}: no target\n", *ent);
		FreeEntity(ent);
		return;
	}

	if (!ent->speed)
		ent->speed = 300;

	ent->moveType = MOVETYPE_PUSH;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/objects/ship/tris.md2");

	ent->mins = { -16, -16, 0 };
	ent->maxs = { 16, 16, 32 };

	ent->think = func_train_find;
	ent->nextThink = level.time + 10_hz;
	ent->use = misc_strogg_ship_use;
	ent->svFlags |= SVF_NOCLIENT;
	ent->moveinfo.accel = ent->moveinfo.decel = ent->moveinfo.speed = ent->speed;

	if (!(ent->spawnflags & SPAWNFLAG_TRAIN_START_ON))
		ent->spawnflags |= SPAWNFLAG_TRAIN_START_ON;

	gi.linkentity(ent);
}

/*QUAKED misc_amb4 (1 0 0) (-16 -16 -16) (16 16 16) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Mal's amb4 loop entity
*/
static cached_soundindex amb4sound;

static THINK(amb4_think) (gentity_t *ent) -> void {
	ent->nextThink = level.time + 2.7_sec;
	gi.sound(ent, CHAN_VOICE, amb4sound, 1, ATTN_NONE, 0);
}

void SP_misc_amb4(gentity_t *ent) {
	ent->think = amb4_think;
	ent->nextThink = level.time + 1_sec;
	amb4sound.assign("world/amb4.wav");
	gi.linkentity(ent);
}

/*QUAKED misc_nuke (1 0 0) (-16 -16 -16) (16 16 16) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
 */
extern void target_killplayers_use(gentity_t *self, gentity_t *other, gentity_t *activator);

static THINK(misc_nuke_think) (gentity_t *self) -> void {
	Nuke_Explode(self);
}

static USE(misc_nuke_use) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	gentity_t *nuke;

	nuke = Spawn();
	nuke->s.origin = self->s.origin;
	nuke->clipMask = MASK_PROJECTILE;
	nuke->solid = SOLID_NOT;
	nuke->mins = { -1, -1, 1 };
	nuke->maxs = { 1, 1, 1 };
	nuke->owner = self;
	nuke->teamMaster = self;
	nuke->nextThink = level.time + FRAME_TIME_S;
	nuke->dmg = 800;
	nuke->splashRadius = 8192;
	nuke->think = misc_nuke_think;
}

void SP_misc_nuke(gentity_t *ent) {
	ent->use = misc_nuke_use;
}

/*QUAKED misc_nuke_core (1 0 0) (-16 -16 -16) (16 16 16) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Toggles visible/not visible. Starts visible.
*/
static USE(misc_nuke_core_use) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	if (self->svFlags & SVF_NOCLIENT)
		self->svFlags &= ~SVF_NOCLIENT;
	else
		self->svFlags |= SVF_NOCLIENT;
}

void SP_misc_nuke_core(gentity_t *ent) {
	gi.setmodel(ent, "models/objects/core/tris.md2");
	gi.linkentity(ent);

	ent->use = misc_nuke_core_use;
}
