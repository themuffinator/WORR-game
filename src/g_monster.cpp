// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"
#include "bots/bot_includes.h"

//
// monster weapons
//
void monster_muzzleflash(gentity_t *self, const vec3_t &start, monster_muzzleflash_id_t id) {
	if (id <= 255)
		gi.WriteByte(svc_muzzleflash2);
	else
		gi.WriteByte(svc_muzzleflash3);

	gi.WriteEntity(self);

	if (id <= 255)
		gi.WriteByte(id);
	else
		gi.WriteShort(id);

	gi.multicast(start, MULTICAST_PHS, false);
}

void monster_fire_bullet(gentity_t *self, const vec3_t &start, const vec3_t &dir, int damage, int kick, int hSpread,
	int vSpread, monster_muzzleflash_id_t flashtype) {
	fire_bullet(self, start, dir, damage, kick, hSpread, vSpread, MOD_MACHINEGUN);
	monster_muzzleflash(self, start, flashtype);
}

void monster_fire_shotgun(gentity_t *self, const vec3_t &start, const vec3_t &aimDir, int damage, int kick, int hSpread,
	int vSpread, int count, monster_muzzleflash_id_t flashtype) {
	fire_shotgun(self, start, aimDir, damage, kick, hSpread, vSpread, count, MOD_SHOTGUN);
	monster_muzzleflash(self, start, flashtype);
}

void monster_fire_blaster(gentity_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed,
	monster_muzzleflash_id_t flashtype, Effect effect) {
	fire_blaster(self, start, dir, damage, speed, effect, MOD_BLASTER);
	monster_muzzleflash(self, start, flashtype);
}

void monster_fire_flechette(gentity_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed,
	monster_muzzleflash_id_t flashtype) {
	fire_flechette(self, start, dir, damage, speed, damage / 2);
	monster_muzzleflash(self, start, flashtype);
}

void monster_fire_grenade(gentity_t *self, const vec3_t &start, const vec3_t &aimDir, int damage, int speed,
	monster_muzzleflash_id_t flashtype, float rightAdjust, float upAdjust) {
	fire_grenade(self, start, aimDir, damage, speed, 2.5_sec, damage + 40.f, rightAdjust, upAdjust, true);
	monster_muzzleflash(self, start, flashtype);
}

void monster_fire_rocket(gentity_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed,
	monster_muzzleflash_id_t flashtype) {
	fire_rocket(self, start, dir, damage, speed, (float)damage + 20, damage);
	monster_muzzleflash(self, start, flashtype);
}

void monster_fire_railgun(gentity_t *self, const vec3_t &start, const vec3_t &aimDir, int damage, int kick,
	monster_muzzleflash_id_t flashtype) {
	if (gi.pointcontents(start) & MASK_SOLID)
		return;

	fire_rail(self, start, aimDir, damage, kick);

	monster_muzzleflash(self, start, flashtype);
}

void monster_fire_bfg(gentity_t *self, const vec3_t &start, const vec3_t &aimDir, int damage, int speed, int kick,
	float splashRadius, monster_muzzleflash_id_t flashtype) {
	fire_bfg(self, start, aimDir, damage, speed, splashRadius);
	monster_muzzleflash(self, start, flashtype);
}

// [Paril-KEX]
vec3_t M_ProjectFlashSource(gentity_t *self, const vec3_t &offset, const vec3_t &forward, const vec3_t &right) {
	return G_ProjectSource(self->s.origin, self->s.scale ? (offset * self->s.scale) : offset, forward, right);
}

// [Paril-KEX] check if shots fired from the given offset
// might be blocked by something
bool M_CheckClearShot(gentity_t *self, const vec3_t &offset, vec3_t &start) {
	// no enemy, just do whatever
	if (!self->enemy)
		return false;

	vec3_t f, r;

	vec3_t real_angles = { self->s.angles[PITCH], self->ideal_yaw, 0.f };

	AngleVectors(real_angles, f, r, nullptr);
	start = M_ProjectFlashSource(self, offset, f, r);

	vec3_t target;

	bool is_blind = self->monsterInfo.attack_state == AS_BLIND || (self->monsterInfo.aiFlags & (AI_MANUAL_STEERING | AI_LOST_SIGHT));

	if (is_blind)
		target = self->monsterInfo.blind_fire_target;
	else
		target = self->enemy->s.origin + vec3_t{ 0, 0, (float)self->enemy->viewHeight };

	trace_t tr = gi.traceline(start, target, self, MASK_PROJECTILE & ~CONTENTS_DEADMONSTER);

	if (tr.ent == self->enemy || tr.ent->client || (tr.fraction > 0.8f && !tr.startsolid))
		return true;

	if (!is_blind) {
		target = self->enemy->s.origin;

		trace_t tr = gi.traceline(start, target, self, MASK_PROJECTILE & ~CONTENTS_DEADMONSTER);

		if (tr.ent == self->enemy || tr.ent->client || (tr.fraction > 0.8f && !tr.startsolid))
			return true;
	}

	return false;
}

bool M_CheckClearShot(gentity_t *self, const vec3_t &offset) {
	vec3_t start;
	return M_CheckClearShot(self, offset, start);
}

void M_CheckGround(gentity_t *ent, contents_t mask) {
	vec3_t	point{};
	trace_t trace;

	if (ent->flags & (FL_SWIM | FL_FLY))
		return;

	if ((ent->velocity[2] * ent->gravityVector[2]) < -100) {
		ent->groundEntity = nullptr;
		return;
	}

	// if the hull point one-quarter unit down is solid the entity is on ground
	point[0] = ent->s.origin[0];
	point[1] = ent->s.origin[1];
	point[2] = ent->s.origin[2] + (0.25f * ent->gravityVector[2]);

	trace = gi.trace(ent->s.origin, ent->mins, ent->maxs, point, ent, mask);

	// check steepness
	if (ent->gravityVector[2] < 0) // normal gravity
	{
		if (trace.plane.normal[2] < 0.7f && !trace.startsolid) {
			ent->groundEntity = nullptr;
			return;
		}
	} else // inverted gravity
	{
		if (trace.plane.normal[2] > -0.7f && !trace.startsolid) {
			ent->groundEntity = nullptr;
			return;
		}
	}

	if (!trace.startsolid && !trace.allsolid) {
		ent->s.origin = trace.endpos;
		ent->groundEntity = trace.ent;
		ent->groundEntity_linkCount = trace.ent->linkCount;
		ent->velocity[2] = 0;
	}
}

void M_CatagorizePosition(gentity_t *self, const vec3_t &in_point, water_level_t &waterlevel, contents_t &watertype) {
	vec3_t	   point{};
	contents_t cont;

	//
	// get waterlevel
	//
	point[0] = in_point[0];
	point[1] = in_point[1];
	if (self->gravityVector[2] > 0)
		point[2] = in_point[2] + self->maxs[2] - 1;
	else
		point[2] = in_point[2] + self->mins[2] + 1;
	cont = gi.pointcontents(point);

	if (!(cont & MASK_WATER)) {
		waterlevel = WATER_NONE;
		watertype = CONTENTS_NONE;
		return;
	}

	watertype = cont;
	waterlevel = WATER_FEET;
	point[2] += 26;
	cont = gi.pointcontents(point);
	if (!(cont & MASK_WATER))
		return;

	waterlevel = WATER_WAIST;
	point[2] += 22;
	cont = gi.pointcontents(point);
	if (cont & MASK_WATER)
		waterlevel = WATER_UNDER;
}

bool M_ShouldReactToPain(gentity_t *self, const mod_t &mod) {
	if (self->monsterInfo.aiFlags & (AI_DUCKED | AI_COMBAT_POINT))
		return false;

	return mod.id == MOD_CHAINFIST || skill->integer < 3;
}

void M_WorldEffects(gentity_t *ent) {
	int dmg;

	if (ent->health > 0) {
		if (!(ent->flags & FL_SWIM)) {
			if (ent->waterlevel < WATER_UNDER) {
				ent->airFinished = level.time + 12_sec;
			} else if (ent->airFinished < level.time) { // drown!
				if (ent->pain_debounce_time < level.time) {
					dmg = 2 + (int)(2 * floorf((level.time - ent->airFinished).seconds()));
					if (dmg > 15)
						dmg = 15;
					Damage(ent, world, world, vec3_origin, ent->s.origin, vec3_origin, dmg, 0, DAMAGE_NO_ARMOR,
						MOD_WATER);
					ent->pain_debounce_time = level.time + 1_sec;
				}
			}
		} else {
			if (ent->waterlevel > WATER_NONE) {
				ent->airFinished = level.time + 9_sec;
			} else if (ent->airFinished < level.time) { // suffocate!
				if (ent->pain_debounce_time < level.time) {
					dmg = 2 + (int)(2 * floorf((level.time - ent->airFinished).seconds()));
					if (dmg > 15)
						dmg = 15;
					Damage(ent, world, world, vec3_origin, ent->s.origin, vec3_origin, dmg, 0, DAMAGE_NO_ARMOR,
						MOD_WATER);
					ent->pain_debounce_time = level.time + 1_sec;
				}
			}
		}
	}

	if (ent->waterlevel == WATER_NONE) {
		if (ent->flags & FL_INWATER) {
			gi.sound(ent, CHAN_BODY, gi.soundindex("player/watr_out.wav"), 1, ATTN_NORM, 0);
			ent->flags &= ~FL_INWATER;
		}
	} else {
		if ((ent->watertype & CONTENTS_LAVA) && !(ent->flags & FL_IMMUNE_LAVA)) {
			if (ent->damage_debounce_time < level.time) {
				ent->damage_debounce_time = level.time + 100_ms;
				Damage(ent, world, world, vec3_origin, ent->s.origin, vec3_origin, 10 * ent->waterlevel, 0, DAMAGE_NONE,
					MOD_LAVA);
			}
		}
		if ((ent->watertype & CONTENTS_SLIME) && !(ent->flags & FL_IMMUNE_SLIME)) {
			if (ent->damage_debounce_time < level.time) {
				ent->damage_debounce_time = level.time + 100_ms;
				Damage(ent, world, world, vec3_origin, ent->s.origin, vec3_origin, 4 * ent->waterlevel, 0, DAMAGE_NONE,
					MOD_SLIME);
			}
		}

		if (!(ent->flags & FL_INWATER)) {
			if (ent->watertype & CONTENTS_LAVA) {
				if ((ent->svFlags & SVF_MONSTER) && ent->health > 0) {
					if (frandom() <= 0.5f)
						gi.sound(ent, CHAN_BODY, gi.soundindex("player/lava1.wav"), 1, ATTN_NORM, 0);
					else
						gi.sound(ent, CHAN_BODY, gi.soundindex("player/lava2.wav"), 1, ATTN_NORM, 0);
				} else
					gi.sound(ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);

				gi.WriteByte(svc_temp_entity);
				gi.WriteByte(TE_SPLASH);
				gi.WriteByte(32);
				gi.WritePosition(ent->s.origin);
				gi.WriteDir(ent->movedir);
				gi.WriteByte(5);
				gi.multicast(ent->s.origin, MULTICAST_PVS, false);
			} else if (ent->watertype & CONTENTS_SLIME) {
				gi.sound(ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);

				gi.WriteByte(svc_temp_entity);
				gi.WriteByte(TE_SPLASH);
				gi.WriteByte(32);
				gi.WritePosition(ent->s.origin);
				gi.WriteDir(ent->movedir);
				gi.WriteByte(4);
				gi.multicast(ent->s.origin, MULTICAST_PVS, false);
			} else if (ent->watertype & CONTENTS_WATER) {
				gi.sound(ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);

				gi.WriteByte(svc_temp_entity);
				gi.WriteByte(TE_SPLASH);
				gi.WriteByte(32);
				gi.WritePosition(ent->s.origin);
				gi.WriteDir(ent->movedir);
				gi.WriteByte(2);
				gi.multicast(ent->s.origin, MULTICAST_PVS, false);
			}

			ent->flags |= FL_INWATER;
			ent->damage_debounce_time = 0_ms;
		}
	}
}

bool M_droptofloor_generic(vec3_t &origin, const vec3_t &mins, const vec3_t &maxs, bool ceiling, gentity_t *ignore, contents_t mask, bool allow_partial) {
	vec3_t	end;
	trace_t trace;

	if (gi.trace(origin, mins, maxs, origin, ignore, mask).startsolid) {
		if (!ceiling)
			origin[2] += 1;
		else
			origin[2] -= 1;
	}

	if (!ceiling) {
		end = origin;
		end[2] -= 256;
	} else {
		end = origin;
		end[2] += 256;
	}

	trace = gi.trace(origin, mins, maxs, end, ignore, mask);

	if (trace.fraction == 1 || trace.allsolid || (!allow_partial && trace.startsolid))
		return false;

	origin = trace.endpos;

	return true;
}

bool M_droptofloor(gentity_t *ent) {
	contents_t mask = G_GetClipMask(ent);

	if (!ent->spawnflags.has(SPAWNFLAG_MONSTER_NO_DROP)) {
		if (!M_droptofloor_generic(ent->s.origin, ent->mins, ent->maxs, ent->gravityVector[2] > 0, ent, mask, true))
			return false;
	} else {
		if (gi.trace(ent->s.origin, ent->mins, ent->maxs, ent->s.origin, ent, mask).startsolid)
			return false;
	}

	gi.linkentity(ent);
	M_CheckGround(ent, mask);
	M_CatagorizePosition(ent, ent->s.origin, ent->waterlevel, ent->watertype);

	return true;
}

void M_SetEffects(gentity_t *ent) {
	ent->s.effects &= ~(EF_COLOR_SHELL | EF_POWERSCREEN | EF_DOUBLE | EF_QUAD | EF_PENT | EF_FLIES);
	ent->s.renderfx &= ~(RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE);

	ent->s.sound = 0;
	ent->s.loop_attenuation = 0;

	// we're gibbed
	if (ent->s.renderfx & RF_LOW_PRIORITY)
		return;

	if (ent->monsterInfo.weaponSound && ent->health > 0) {
		ent->s.sound = ent->monsterInfo.weaponSound;
		ent->s.loop_attenuation = ATTN_NORM;
	} else if (ent->monsterInfo.engine_sound)
		ent->s.sound = ent->monsterInfo.engine_sound;

	if (ent->monsterInfo.aiFlags & AI_RESURRECTING) {
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= RF_SHELL_RED;
	}

	ent->s.renderfx |= RF_DOT_SHADOW;

	// no power armor/powerup effects if we died
	if (ent->health <= 0)
		return;

	if (ent->powerarmor_time > level.time) {
		if (ent->monsterInfo.powerArmorType == IT_POWER_SCREEN) {
			ent->s.effects |= EF_POWERSCREEN;
		} else if (ent->monsterInfo.powerArmorType == IT_POWER_SHIELD) {
			ent->s.effects |= EF_COLOR_SHELL;
			ent->s.renderfx |= RF_SHELL_GREEN;
		}
	}

	// monster powerups
	if (ent->monsterInfo.quad_time > level.time) {
		if (G_PowerUpExpiring(ent->monsterInfo.quad_time))
			ent->s.effects |= EF_QUAD;
	}

	if (ent->monsterInfo.double_time > level.time) {
		if (G_PowerUpExpiring(ent->monsterInfo.double_time))
			ent->s.effects |= EF_DOUBLE;
	}

	if (ent->monsterInfo.invincibility_time > level.time) {
		if (G_PowerUpExpiring(ent->monsterInfo.invincibility_time))
			ent->s.effects |= EF_PENT;
	}
}

bool M_AllowSpawn(gentity_t *self) {
	if (deathmatch->integer && !(ai_allow_dm_spawn->integer || GT(GT_HORDE))) {
		return false;
	}
	return true;
}

void M_SetAnimation(gentity_t *self, const save_mmove_t &move, bool instant) {
	// [Paril-KEX] free the beams if we switch animations.
	if (self->beam) {
		FreeEntity(self->beam);
		self->beam = nullptr;
	}

	if (self->beam2) {
		FreeEntity(self->beam2);
		self->beam2 = nullptr;
	}

	// instant switches will cause active_move to change on the next frame
	if (instant) {
		self->monsterInfo.active_move = move;
		self->monsterInfo.next_move = nullptr;
		return;
	}

	// these wait until the frame is ready to be finished
	self->monsterInfo.next_move = move;
}

static void M_MoveFrame(gentity_t *self) {
	const mmove_t *move = self->monsterInfo.active_move.pointer();

	// [Paril-KEX] high tick rate adjustments;
	// monsters still only step frames and run thinkfunc's at
	// 10hz, but will run aifuncs at full speed with
	// distance spread over 10hz

	self->nextThink = level.time + FRAME_TIME_S;

	// time to run next 10hz move yet?
	bool run_frame = self->monsterInfo.next_move_time <= level.time;

	// we asked nicely to switch frames when the timer ran up
	if (run_frame && self->monsterInfo.next_move.pointer() && self->monsterInfo.active_move != self->monsterInfo.next_move) {
		M_SetAnimation(self, self->monsterInfo.next_move, true);
		move = self->monsterInfo.active_move.pointer();
	}

	if (!move)
		return;

	// no, but maybe we were explicitly forced into another move (pain,
	// death, etc)
	if (!run_frame)
		run_frame = (self->s.frame < move->firstFrame || self->s.frame > move->lastFrame);

	if (run_frame) {
		// [Paril-KEX] allow next_move and nextFrame to work properly after an endFunc
		bool explicit_frame = false;

		if ((self->monsterInfo.nextFrame) && (self->monsterInfo.nextFrame >= move->firstFrame) &&
			(self->monsterInfo.nextFrame <= move->lastFrame)) {
			self->s.frame = self->monsterInfo.nextFrame;
			self->monsterInfo.nextFrame = 0;
		} else {
			if (self->s.frame == move->lastFrame) {
				if (move->endFunc) {
					move->endFunc(self);

					if (self->monsterInfo.next_move) {
						M_SetAnimation(self, self->monsterInfo.next_move, true);

						if (self->monsterInfo.nextFrame) {
							self->s.frame = self->monsterInfo.nextFrame;
							self->monsterInfo.nextFrame = 0;
							explicit_frame = true;
						}
					}

					// regrab move, endFunc is very likely to change it
					move = self->monsterInfo.active_move.pointer();

					// check for death
					if (self->svFlags & SVF_DEADMONSTER)
						return;
				}
			}

			if (self->s.frame < move->firstFrame || self->s.frame > move->lastFrame) {
				self->monsterInfo.aiFlags &= ~AI_HOLD_FRAME;
				self->s.frame = move->firstFrame;
			} else if (!explicit_frame) {
				if (!(self->monsterInfo.aiFlags & AI_HOLD_FRAME)) {
					self->s.frame++;
					if (self->s.frame > move->lastFrame)
						self->s.frame = move->firstFrame;
				}
			}
		}

		if (self->monsterInfo.aiFlags & AI_HIGH_TICK_RATE)
			self->monsterInfo.next_move_time = level.time;
		else
			self->monsterInfo.next_move_time = level.time + 10_hz;

		if ((self->monsterInfo.nextFrame) && !((self->monsterInfo.nextFrame >= move->firstFrame) &&
			(self->monsterInfo.nextFrame <= move->lastFrame)))
			self->monsterInfo.nextFrame = 0;
	}

	// NB: frame thinkfunc can be called on the same frame
	// as the animation changing

	int32_t index = self->s.frame - move->firstFrame;
	if (move->frame[index].aiFunc) {
		if (!(self->monsterInfo.aiFlags & AI_HOLD_FRAME)) {
			float dist = move->frame[index].dist * self->monsterInfo.scale;
			dist /= static_cast<float>(gi.tick_rate) / 10.0f;
			move->frame[index].aiFunc(self, dist);
		} else
			move->frame[index].aiFunc(self, 0);
	}

	if (run_frame && move->frame[index].thinkfunc)
		move->frame[index].thinkfunc(self);

	if (move->frame[index].lerp_frame != -1) {
		self->s.renderfx |= RF_OLD_FRAME_LERP;
		self->s.old_frame = move->frame[index].lerp_frame;
	}
}

void G_MonsterKilled(gentity_t *self) {
	level.killedMonsters++;

	if (coop->integer && self->enemy && self->enemy->client)
		G_AdjustPlayerScore(self->enemy->client, 1, false, 0);

	if (g_debug_monster_kills->integer) {
		bool found = false;

		for (auto &ent : level.monstersRegistered) {
			if (ent == self) {
				ent = nullptr;
				found = true;
				break;
			}
		}

		if (!found) {
#if defined(_DEBUG) && defined(KEX_PLATFORM_WINPC)
			__debugbreak();
#endif
			gi.Center_Print(&g_entities[1], "found missing monster?");
		}

		if (level.killedMonsters == level.totalMonsters) {
			gi.Center_Print(&g_entities[1], "all monsters dead");
		}
	}
}

void M_ProcessPain(gentity_t *e) {
	if (!e->monsterInfo.damage.blood)
		return;

	if (e->health <= 0) {
		if (e->monsterInfo.aiFlags & AI_MEDIC) {
			if (e->enemy && e->enemy->inUse && (e->enemy->svFlags & SVF_MONSTER)) // god, I hope so
			{
				M_CleanupHealTarget(e->enemy);
			}

			// clean up self
			e->monsterInfo.aiFlags &= ~AI_MEDIC;
		}

		if (!e->deadFlag) {
			e->enemy = e->monsterInfo.damage.attacker;

			// free up slot for spawned monster if it's spawned
			if (e->monsterInfo.aiFlags & AI_SPAWNED_CARRIER) {
				if (e->monsterInfo.commander && e->monsterInfo.commander->inUse &&
					!strcmp(e->monsterInfo.commander->className, "monster_carrier"))
					e->monsterInfo.commander->monsterInfo.monster_slots++;
				e->monsterInfo.commander = nullptr;
			}
			if (e->monsterInfo.aiFlags & AI_SPAWNED_WIDOW) {
				// need to check this because we can have variable numbers of coop players
				if (e->monsterInfo.commander && e->monsterInfo.commander->inUse &&
					!strncmp(e->monsterInfo.commander->className, "monster_widow", 13)) {
					if (e->monsterInfo.commander->monsterInfo.monster_used > 0)
						e->monsterInfo.commander->monsterInfo.monster_used--;
					e->monsterInfo.commander = nullptr;
				}
			}

			if (!(e->monsterInfo.aiFlags & AI_DO_NOT_COUNT) && !(e->spawnflags & SPAWNFLAG_MONSTER_DEAD))
				G_MonsterKilled(e);

			e->touch = nullptr;
			monster_death_use(e);
		}

		if (!e->deadFlag) {
			int32_t score_value = ceil(e->monsterInfo.base_health / 100);
			if (score_value < 1) score_value = 1;
			Horde_AdjustPlayerScore(e->monsterInfo.damage.attacker->client, score_value);
		}
		e->die(e, e->monsterInfo.damage.inflictor, e->monsterInfo.damage.attacker, e->monsterInfo.damage.blood, e->monsterInfo.damage.origin, e->monsterInfo.damage.mod);

		// [Paril-KEX] medic commander only gets his slots back after the monster is gibbed, since we can revive them
		if (e->health <= e->gibHealth) {
			if (e->monsterInfo.aiFlags & AI_SPAWNED_MEDIC_C) {
				if (e->monsterInfo.commander && e->monsterInfo.commander->inUse && !strcmp(e->monsterInfo.commander->className, "monster_medic_commander"))
					e->monsterInfo.commander->monsterInfo.monster_used -= e->monsterInfo.monster_slots;

				e->monsterInfo.commander = nullptr;
			}
		}

		if (e->inUse && e->health > e->gibHealth && e->s.frame == e->monsterInfo.active_move->lastFrame) {
			e->s.frame -= irandom(1, 3);

			if (e->groundEntity && e->moveType == MOVETYPE_TOSS && !(e->flags & FL_STATIONARY))
				e->s.angles[YAW] += brandom() ? 4.5f : -4.5f;
		}
	} else
		e->pain(e, e->monsterInfo.damage.attacker, (float)e->monsterInfo.damage.knockback, e->monsterInfo.damage.blood, e->monsterInfo.damage.mod);

	if (!e->inUse)
		return;

	if (e->monsterInfo.setskin)
		e->monsterInfo.setskin(e);

	e->monsterInfo.damage.blood = 0;
	e->monsterInfo.damage.knockback = 0;
	e->monsterInfo.damage.attacker = e->monsterInfo.damage.inflictor = nullptr;

	// [Paril-KEX] fire health target
	if (e->healthtarget) {
		const char *target = e->target;
		e->target = e->healthtarget;
		UseTargets(e, e->enemy);
		e->target = target;
	}
}

//
// Monster utility functions
//
/*
=============
monster_body_sink

After sitting around for x seconds, fall into the ground and disappear
=============
*/
static THINK(monster_body_sink) (gentity_t *ent) -> void {
	if (level.time > ent->timeStamp) {
		ent->svFlags = SVF_NOCLIENT;
		ent->takeDamage = false;
		ent->solid = SOLID_NOT;

		// the body ques are never actually freed, they are just unlinked
		gi.unlinkentity(ent);
		return;
	}
	ent->nextThink = level.time + 50_ms;
	ent->s.origin[2] -= 0.5;
}

THINK(monster_dead_think) (gentity_t *self) -> void {

	if (self->timeStamp >= self->nextThink) {
		self->nextThink = level.time + gtime_t::from_sec(CORPSE_SINK_TIME);
		self->think = monster_body_sink;
		return;
	}

	// flies
	if ((self->monsterInfo.aiFlags & AI_STINKY) && !(self->monsterInfo.aiFlags & AI_STUNK)) {
		if (!self->fly_sound_debounce_time)
			self->fly_sound_debounce_time = level.time + random_time(5_sec, 15_sec);
		else if (self->fly_sound_debounce_time < level.time) {
			if (!self->s.sound) {
				self->s.effects |= EF_FLIES;
				self->s.sound = gi.soundindex("infantry/inflies1.wav");
				self->fly_sound_debounce_time = level.time + 60_sec;
			} else {
				self->s.effects &= ~EF_FLIES;
				self->s.sound = 0;
				self->monsterInfo.aiFlags |= AI_STUNK;
			}
		}
	}

	if (!self->monsterInfo.damage.blood) {
		if (self->s.frame != self->monsterInfo.active_move->lastFrame)
			self->s.frame++;
	}

	self->nextThink = level.time + 10_hz;
}

void monster_dead(gentity_t *self) {
	self->think = monster_dead_think;
	self->nextThink = level.time + 10_hz;
	self->timeStamp = level.time + gtime_t::from_sec(CORPSE_SINK_TIME + 1.5);
	self->moveType = MOVETYPE_TOSS;
	self->svFlags |= SVF_DEADMONSTER;
	self->monsterInfo.damage.blood = 0;
	self->fly_sound_debounce_time = 0_ms;
	self->monsterInfo.aiFlags &= ~AI_STUNK;
	gi.linkentity(self);
}

/*
=============
infront

returns 1 if the entity is in front (in sight) of self
=============
*/
static bool projectile_infront(gentity_t *self, gentity_t *other) {
	vec3_t vec;
	float  dot;
	vec3_t forward;

	AngleVectors(self->s.angles, forward, nullptr, nullptr);
	vec = other->s.origin - self->s.origin;
	vec.normalize();
	dot = vec.dot(forward);
	return dot > 0.35f;
}

static BoxEntitiesResult_t M_CheckDodge_BoxEntitiesFilter(gentity_t *ent, void *data) {
	gentity_t *self = (gentity_t *)data;

	// not a valid projectile
	if (!(ent->svFlags & SVF_PROJECTILE) || !(ent->flags & FL_DODGE))
		return BoxEntitiesResult_t::Skip;

	// not moving
	if (ent->velocity.lengthSquared() < 16.f)
		return BoxEntitiesResult_t::Skip;

	// projectile is behind us, we can't see it
	if (!projectile_infront(self, ent))
		return BoxEntitiesResult_t::Skip;

	// will it hit us within 1 second? gives us enough time to dodge
	trace_t tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, ent->s.origin + ent->velocity, ent, ent->clipMask);

	if (tr.ent == self) {
		vec3_t v = tr.endpos - ent->s.origin;
		gtime_t eta = gtime_t::from_sec(v.length() / ent->velocity.length());

		self->monsterInfo.dodge(self, ent->owner, eta, &tr, (ent->moveType == MOVETYPE_BOUNCE || ent->moveType == MOVETYPE_TOSS));

		return BoxEntitiesResult_t::End;
	}

	return BoxEntitiesResult_t::Skip;
}

// [Paril-KEX] active checking for projectiles to dodge
static void M_CheckDodge(gentity_t *self) {
	// we recently made a valid dodge, don't try again for a bit
	if (self->monsterInfo.dodge_time > level.time)
		return;

	gi.BoxEntities(self->absMin - vec3_t{ 512, 512, 512 }, self->absMax + vec3_t{ 512, 512, 512 }, nullptr, 0, AREA_SOLID, M_CheckDodge_BoxEntitiesFilter, self);
}

static bool CheckPathVisibility(const vec3_t &start, const vec3_t &end) {
	trace_t tr = gi.traceline(start, end, nullptr, MASK_SOLID | CONTENTS_PROJECTILECLIP | CONTENTS_MONSTERCLIP | CONTENTS_PLAYERCLIP);

	bool valid = tr.fraction == 1.0f;

	if (!valid) {
		// try raising some of the points
		bool can_raise_start = false, can_raise_end = false;
		vec3_t raised_start = start + vec3_t{ 0.f, 0.f, 16.f };
		vec3_t raised_end = end + vec3_t{ 0.f, 0.f, 16.f };

		if (gi.traceline(start, raised_start, nullptr, MASK_SOLID | CONTENTS_PROJECTILECLIP | CONTENTS_MONSTERCLIP | CONTENTS_PLAYERCLIP).fraction == 1.0f)
			can_raise_start = true;

		if (gi.traceline(end, raised_end, nullptr, MASK_SOLID | CONTENTS_PROJECTILECLIP | CONTENTS_MONSTERCLIP | CONTENTS_PLAYERCLIP).fraction == 1.0f)
			can_raise_end = true;

		// try raised start -> end
		if (can_raise_start) {
			tr = gi.traceline(raised_start, end, nullptr, MASK_SOLID | CONTENTS_PROJECTILECLIP | CONTENTS_MONSTERCLIP | CONTENTS_PLAYERCLIP);

			if (tr.fraction == 1.0f)
				return true;
		}

		// try start -> raised end
		if (can_raise_end) {
			tr = gi.traceline(start, raised_end, nullptr, MASK_SOLID | CONTENTS_PROJECTILECLIP | CONTENTS_MONSTERCLIP | CONTENTS_PLAYERCLIP);

			if (tr.fraction == 1.0f)
				return true;
		}

		// try both raised
		if (can_raise_start && can_raise_end) {
			tr = gi.traceline(raised_start, raised_end, nullptr, MASK_SOLID | CONTENTS_PROJECTILECLIP | CONTENTS_MONSTERCLIP | CONTENTS_PLAYERCLIP);

			if (tr.fraction == 1.0f)
				return true;
		}

		//gi.Draw_Line(start, end, rgba_red, 0.1f, false);
	}

	return valid;
}

THINK(monster_think) (gentity_t *self) -> void {
	// [Paril-KEX] monster sniff testing; if we can make an unobstructed path to the player, murder ourselves.
	if (g_debug_monster_kills->integer) {
		if (g_entities[1].inUse) {
			trace_t enemy_trace = gi.traceline(self->s.origin, g_entities[1].s.origin, self, MASK_SHOT);

			if (enemy_trace.fraction < 1.0f && enemy_trace.ent == &g_entities[1])
				Damage(self, &g_entities[1], &g_entities[1], { 0, 0, -1 }, self->s.origin, { 0, 0, -1 }, 9999, 9999, DAMAGE_NO_PROTECTION, MOD_BFG_BLAST);
			else {
				static vec3_t points[64];

				if (self->disintegrator_time <= level.time) {
					PathRequest request;
					request.goal = g_entities[1].s.origin;
					request.moveDist = 4.0f;
					request.nodeSearch.ignoreNodeFlags = true;
					request.nodeSearch.radius = 9999;
					request.pathFlags = PathFlags::All;
					request.start = self->s.origin;
					request.traversals.dropHeight = 9999;
					request.traversals.jumpHeight = 9999;
					request.pathPoints.array = points;
					request.pathPoints.count = q_countof(points);

					PathInfo info;

					if (gi.GetPathToGoal(request, info)) {
						if (info.returnCode != PathReturnCode::NoStartNode &&
							info.returnCode != PathReturnCode::NoGoalNode &&
							info.returnCode != PathReturnCode::NoPathFound &&
							info.returnCode != PathReturnCode::NoNavAvailable &&
							info.numPathPoints < q_countof(points)) {
							if (CheckPathVisibility(g_entities[1].s.origin + vec3_t{ 0.f, 0.f, g_entities[1].mins.z }, points[info.numPathPoints - 1]) &&
								CheckPathVisibility(self->s.origin + vec3_t{ 0.f, 0.f, self->mins.z }, points[0])) {
								size_t i = 0;

								for (; i < static_cast<size_t>(info.numPathPoints - 1); i++)
									if (!CheckPathVisibility(points[i], points[i + 1]))
										break;

								if (i == static_cast<size_t>(info.numPathPoints - 1))
									Damage(self, &g_entities[1], &g_entities[1], { 0, 0, 1 }, self->s.origin, { 0, 0, 1 }, 9999, 9999, DAMAGE_NO_PROTECTION, MOD_BFG_BLAST);
								else
									self->disintegrator_time = level.time + 500_ms;
							} else
								self->disintegrator_time = level.time + 500_ms;
						} else {
							self->disintegrator_time = level.time + 1_sec;
						}
					} else {
						self->disintegrator_time = level.time + 1_sec;
					}
				}
			}

			if (!self->deadFlag && !(self->monsterInfo.aiFlags & AI_DO_NOT_COUNT))
				gi.Draw_Bounds(self->absMin, self->absMax, rgba_red, gi.frame_time_s, false);
		}
	}

	self->s.renderfx &= ~(RF_STAIR_STEP | RF_OLD_FRAME_LERP);

	M_ProcessPain(self);

	// pain/die above freed us
	if (!self->inUse || self->think != monster_think)
		return;

	if (self->hackflags & HACKFLAG_ATTACK_PLAYER || GT(GT_HORDE)) {
		if (!self->enemy && g_entities[1].inUse && ClientIsPlaying(g_entities[1].client)) {
			self->enemy = &g_entities[1];
			FoundTarget(self);
		}
	}

	if (self->health > 0 && self->monsterInfo.dodge && !(globals.server_flags & SERVER_FLAG_LOADING))
		M_CheckDodge(self);

	M_MoveFrame(self);
	if (self->linkCount != self->monsterInfo.linkCount) {
		self->monsterInfo.linkCount = self->linkCount;
		M_CheckGround(self, G_GetClipMask(self));
	}
	M_CatagorizePosition(self, self->s.origin, self->waterlevel, self->watertype);
	M_WorldEffects(self);
	M_SetEffects(self);
}

/*
================
monster_use

Using a monster makes it angry at the current activator
================
*/
USE(monster_use) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	if (self->enemy)
		return;
	if (self->health <= 0)
		return;
	if (!activator)
		return;
	if (activator->flags & FL_NOTARGET)
		return;
	if (!(activator->client) && !(activator->monsterInfo.aiFlags & AI_GOOD_GUY))
		return;
	if (activator->flags & FL_DISGUISED)
		return;

	// delay reaction so if the monster is teleported, its sound is still heard
	self->enemy = activator;
	FoundTarget(self);
}

void monster_start_go(gentity_t *self);

static THINK(monster_triggered_spawn) (gentity_t *self) -> void {
	self->s.origin[2] += 1;

	self->solid = SOLID_BBOX;
	self->moveType = MOVETYPE_STEP;
	self->svFlags &= ~SVF_NOCLIENT;
	self->airFinished = level.time + 12_sec;
	gi.linkentity(self);

	KillBox(self, false);

	monster_start_go(self);

	if (strcmp(self->className, "monster_fixbot") == 0) {
		if (self->spawnflags.has(SPAWNFLAG_FIXBOT_LANDING | SPAWNFLAG_FIXBOT_TAKEOFF | SPAWNFLAG_FIXBOT_FIXIT)) {
			self->enemy = nullptr;
			return;
		}
	}

	if (self->enemy && !(self->spawnflags & SPAWNFLAG_MONSTER_AMBUSH) && !(self->enemy->flags & FL_NOTARGET) && !(self->monsterInfo.aiFlags & AI_GOOD_GUY)) {
		if (!(self->enemy->flags & FL_DISGUISED))
			FoundTarget(self);
		else // PMM - just in case, make sure to clear the enemy so FindTarget doesn't get confused
			self->enemy = nullptr;
	} else {
		self->enemy = nullptr;
	}
}

static USE(monster_triggered_spawn_use) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	// we have a one frame delay here so we don't telefrag the guy who activated us
	self->think = monster_triggered_spawn;
	self->nextThink = level.time + FRAME_TIME_S;
	if (activator && activator->client && !(self->hackflags & HACKFLAG_END_CUTSCENE))
		self->enemy = activator;
	self->use = monster_use;

	if (self->spawnflags.has(SPAWNFLAG_MONSTER_SCENIC)) {
		M_droptofloor(self);

		self->nextThink = 0_ms;
		self->think(self);

		if (self->spawnflags.has(SPAWNFLAG_MONSTER_AMBUSH))
			monster_use(self, other, activator);

		for (int i = 0; i < 30; i++) {
			self->think(self);
			self->monsterInfo.next_move_time = 0_ms;
		}
	}
}

static THINK(monster_triggered_think) (gentity_t *self) -> void {
	if (!(self->monsterInfo.aiFlags & AI_DO_NOT_COUNT))
		gi.Draw_Bounds(self->absMin, self->absMax, rgba_blue, gi.frame_time_s, false);

	self->nextThink = level.time + 1_ms;
}

static void monster_triggered_start(gentity_t *self) {
	self->solid = SOLID_NOT;
	self->moveType = MOVETYPE_NONE;
	self->svFlags |= SVF_NOCLIENT;
	self->nextThink = 0_ms;
	self->use = monster_triggered_spawn_use;

	if (g_debug_monster_kills->integer) {
		self->think = monster_triggered_think;
		self->nextThink = level.time + 1_ms;
	}

	if (!self->targetname ||
		(G_FindByString<&gentity_t::target>(nullptr, self->targetname) == nullptr &&
			G_FindByString<&gentity_t::pathtarget>(nullptr, self->targetname) == nullptr &&
			G_FindByString<&gentity_t::deathtarget>(nullptr, self->targetname) == nullptr &&
			G_FindByString<&gentity_t::itemtarget>(nullptr, self->targetname) == nullptr &&
			G_FindByString<&gentity_t::healthtarget>(nullptr, self->targetname) == nullptr &&
			G_FindByString<&gentity_t::combattarget>(nullptr, self->targetname) == nullptr)) {
		gi.Com_PrintFmt("{}: is trigger spawned, but has no targetname or no entity to spawn it\n", *self);
	}
}

/*
================
monster_death_use

When a monster dies, it fires all of its targets with the current
enemy as activator.
================
*/
void monster_death_use(gentity_t *self) {
	self->flags &= ~(FL_FLY | FL_SWIM);
	self->monsterInfo.aiFlags &= (AI_DOUBLE_TROUBLE | AI_GOOD_GUY | AI_STINKY | AI_SPAWNED_MASK);

	if (self->item) {
		gentity_t *dropped = Drop_Item(self, self->item);

		if (self->itemtarget) {
			dropped->target = self->itemtarget;
			self->itemtarget = nullptr;
		}

		self->item = nullptr;
	}

	if (self->deathtarget)
		self->target = self->deathtarget;

	if (self->target)
		UseTargets(self, self->enemy);

	// [Paril-KEX] fire health target
	if (self->healthtarget) {
		self->target = self->healthtarget;
		UseTargets(self, self->enemy);
	}
}

// [Paril-KEX] adjust the monster's health from how
// many active players we have
static void G_Monster_ScaleCoopHealth(gentity_t *self) {
	// already scaled
	if (self->monsterInfo.health_scaling >= level.coopScalePlayers)
		return;

	// this is just to fix monsters that change health after spawning...
	// looking at you, soldiers
	if (!self->monsterInfo.base_health)
		self->monsterInfo.base_health = self->max_health;

	int32_t delta = level.coopScalePlayers - self->monsterInfo.health_scaling;
	int32_t additional_health = delta * (int32_t)(self->monsterInfo.base_health * level.coopHealthScaling);

	self->health = max(1, self->health + additional_health);
	self->max_health += additional_health;

	self->monsterInfo.health_scaling = level.coopScalePlayers;
}

struct monster_filter_t {
	inline bool operator()(gentity_t *self) const {
		return self->inUse && (self->flags & FL_COOP_HEALTH_SCALE) && self->health > 0;
	}
};

// check all active monsters' scaling
void G_Monster_CheckCoopHealthScaling() {
	for (auto monster : entity_iterable_t<monster_filter_t>())
		G_Monster_ScaleCoopHealth(monster);
}

//============================================================================
constexpr spawnflags_t SPAWNFLAG_MONSTER_FUBAR = 4_spawnflag;

bool monster_start(gentity_t *self) {
	if (!M_AllowSpawn(self)) {
		FreeEntity(self);
		return false;
	}

	if (self->spawnflags.has(SPAWNFLAG_MONSTER_SCENIC))
		self->monsterInfo.aiFlags |= AI_GOOD_GUY;

	// [Paril-KEX] n64
	if (self->hackflags & (HACKFLAG_END_CUTSCENE | HACKFLAG_ATTACK_PLAYER))
		self->monsterInfo.aiFlags |= AI_DO_NOT_COUNT;

	if (self->spawnflags.has(SPAWNFLAG_MONSTER_FUBAR) && !(self->monsterInfo.aiFlags & AI_GOOD_GUY)) {
		self->spawnflags &= ~SPAWNFLAG_MONSTER_FUBAR;
		self->spawnflags |= SPAWNFLAG_MONSTER_AMBUSH;
	}

	// [Paril-KEX] simplify other checks
	if (self->monsterInfo.aiFlags & AI_GOOD_GUY)
		self->monsterInfo.aiFlags |= AI_DO_NOT_COUNT;

	if (!(self->monsterInfo.aiFlags & AI_DO_NOT_COUNT) && !self->spawnflags.has(SPAWNFLAG_MONSTER_DEAD)) {
		if (g_debug_monster_kills->integer)
			level.monstersRegistered[level.totalMonsters] = self;
		level.totalMonsters++;
	}

	self->nextThink = level.time + FRAME_TIME_S;
	self->svFlags |= SVF_MONSTER;
	self->takeDamage = true;
	self->airFinished = level.time + 12_sec;
	self->use = monster_use;
	self->max_health = self->health;
	self->clipMask = MASK_MONSTERSOLID;
	self->deadFlag = false;
	self->svFlags &= ~SVF_DEADMONSTER;
	self->flags &= ~FL_ALIVE_KNOCKBACK_ONLY;
	self->flags |= FL_COOP_HEALTH_SCALE;
	self->s.old_origin = self->s.origin;
	self->monsterInfo.initial_power_armor_type = self->monsterInfo.powerArmorType;
	self->monsterInfo.max_power_armor_power = self->monsterInfo.powerArmorPower;
	//gi.Com_PrintFmt("MONSTER SPAWN: {}, maxhealth = {}\n", *self, self->max_health);
	if (!self->monsterInfo.checkAttack)
		self->monsterInfo.checkAttack = M_CheckAttack;

	if (ai_model_scale->value > 0) {
		self->s.scale = ai_model_scale->value;
	}

	if (self->s.scale) {
		self->monsterInfo.scale *= self->s.scale;
		self->mins *= self->s.scale;
		self->maxs *= self->s.scale;
		self->mass *= self->s.scale;
	}

	// set combat style if unset
	if (self->monsterInfo.combat_style == COMBAT_UNKNOWN) {
		if (!self->monsterInfo.attack && self->monsterInfo.melee)
			self->monsterInfo.combat_style = COMBAT_MELEE;
		else
			self->monsterInfo.combat_style = COMBAT_MIXED;
	}

	if (st.item) {
		self->item = FindItemByClassname(st.item);
		if (!self->item)
			gi.Com_PrintFmt("{}: bad item: {}\n", *self, st.item);
	}

	// randomize what frame they start on
	if (self->monsterInfo.active_move)
		self->s.frame =
		irandom(self->monsterInfo.active_move->firstFrame, self->monsterInfo.active_move->lastFrame + 1);

	// PMM - get this so I don't have to do it in all of the monsters
	self->monsterInfo.base_height = self->maxs[2];

	// Paril: monsters' old default viewHeight (25)
	// is all messed up for certain monsters. Calculate
	// from maxs to make a bit more sense.
	if (!self->viewHeight)
		self->viewHeight = (int)(self->maxs[2] - 8.f);

	// PMM - clear these
	self->monsterInfo.quad_time = 0_ms;
	self->monsterInfo.double_time = 0_ms;
	self->monsterInfo.invincibility_time = 0_ms;

	// set base health & set base scaling to 1 player
	self->monsterInfo.base_health = self->health;
	self->monsterInfo.health_scaling = 1;

	// [Paril-KEX] co-op health scale
	G_Monster_ScaleCoopHealth(self);

	return true;
}

stuck_result_t G_FixStuckObject(gentity_t *self, vec3_t check) {
	contents_t mask = G_GetClipMask(self);
	stuck_result_t result = G_FixStuckObject_Generic(check, self->mins, self->maxs, [self, mask](const vec3_t &start, const vec3_t &mins, const vec3_t &maxs, const vec3_t &end) {
		return gi.trace(start, mins, maxs, end, self, mask);
		});

	if (result == stuck_result_t::NO_GOOD_POSITION)
		return result;

	self->s.origin = check;

	if (result == stuck_result_t::FIXED)
		gi.Com_PrintFmt("fixed stuck {}\n", *self);

	return result;
}

void monster_start_go(gentity_t *self) {
	// Paril: moved here so this applies to swim/fly monsters too
	if (!(self->flags & FL_STATIONARY)) {
		const vec3_t check = self->s.origin;

		// [Paril-KEX] different nudge method; see if any of the bbox sides are clear,
		// if so we can see how much headroom we have in that direction and shift us.
		// most of the monsters stuck in solids will only be stuck on one side, which
		// conveniently leaves only one side not in a solid; this won't fix monsters
		// stuck in a corner though.
		bool is_stuck = false;

		if ((self->monsterInfo.aiFlags & AI_GOOD_GUY) || (self->flags & (FL_FLY | FL_SWIM)))
			is_stuck = gi.trace(self->s.origin, self->mins, self->maxs, self->s.origin, self, MASK_MONSTERSOLID).startsolid;
		else
			is_stuck = !M_droptofloor(self) || !M_walkmove(self, 0, 0);

		if (is_stuck) {
			if (G_FixStuckObject(self, check) != stuck_result_t::NO_GOOD_POSITION) {
				if (self->monsterInfo.aiFlags & AI_GOOD_GUY)
					is_stuck = gi.trace(self->s.origin, self->mins, self->maxs, self->s.origin, self, MASK_MONSTERSOLID).startsolid;
				else if (!(self->flags & (FL_FLY | FL_SWIM)))
					M_droptofloor(self);
				is_stuck = false;
			}
		}

		// last ditch effort: brute force
		if (is_stuck) {
			// Paril: try nudging them out. this fixes monsters stuck
			// in very shallow slopes.
			constexpr const int32_t adjust[] = { 0, -1, 1, -2, 2, -4, 4, -8, 8 };
			bool					walked = false;

			for (int32_t y = 0; !walked && y < 3; y++)
				for (int32_t x = 0; !walked && x < 3; x++)
					for (int32_t z = 0; !walked && z < 3; z++) {
						self->s.origin[0] = check[0] + adjust[x];
						self->s.origin[1] = check[1] + adjust[y];
						self->s.origin[2] = check[2] + adjust[z];

						if (self->monsterInfo.aiFlags & AI_GOOD_GUY) {
							is_stuck = gi.trace(self->s.origin, self->mins, self->maxs, self->s.origin, self, MASK_MONSTERSOLID).startsolid;

							if (!is_stuck)
								walked = true;
						} else if (!(self->flags & (FL_FLY | FL_SWIM))) {
							M_droptofloor(self);
							walked = M_walkmove(self, 0, 0);
						}
					}
		}

		if (is_stuck)
			gi.Com_PrintFmt("WARNING: {} stuck in solid\n", *self);
	}

	vec3_t v;

	if (self->health <= 0)
		return;

	self->s.old_origin = self->s.origin;

	// check for target to combat_point and change to combattarget
	if (self->target) {
		bool	 notcombat;
		bool	 fixup;
		gentity_t *target;

		target = nullptr;
		notcombat = false;
		fixup = false;
		while ((target = G_FindByString<&gentity_t::targetname>(target, self->target)) != nullptr) {
			if (strcmp(target->className, "point_combat") == 0) {
				self->combattarget = self->target;
				fixup = true;
			} else {
				notcombat = true;
			}
		}
		if (notcombat && self->combattarget)
			gi.Com_PrintFmt("{}: has target with mixed types\n", *self);
		if (fixup)
			self->target = nullptr;
	}

	// validate combattarget
	if (self->combattarget) {
		gentity_t *target;

		target = nullptr;
		while ((target = G_FindByString<&gentity_t::targetname>(target, self->combattarget)) != nullptr) {
			if (strcmp(target->className, "point_combat") != 0) {
				gi.Com_PrintFmt("{} has a bad combattarget {} ({})\n", *self, self->combattarget, *target);
			}
		}
	}

	// allow spawning dead
	bool spawn_dead = self->spawnflags.has(SPAWNFLAG_MONSTER_DEAD);

	if (self->target) {
		self->goalentity = self->movetarget = PickTarget(self->target);
		if (!self->movetarget) {
			gi.Com_PrintFmt("{}: can't find target {}\n", *self, self->target);
			self->target = nullptr;
			self->monsterInfo.pauseTime = HOLD_FOREVER;
			if (!spawn_dead)
				self->monsterInfo.stand(self);
		} else if (strcmp(self->movetarget->className, "path_corner") == 0) {
			v = self->goalentity->s.origin - self->s.origin;
			self->ideal_yaw = self->s.angles[YAW] = vectoyaw(v);
			if (!spawn_dead)
				self->monsterInfo.walk(self);
			self->target = nullptr;
		} else {
			self->goalentity = self->movetarget = nullptr;
			self->monsterInfo.pauseTime = HOLD_FOREVER;
			if (!spawn_dead)
				self->monsterInfo.stand(self);
		}
	} else {
		self->monsterInfo.pauseTime = HOLD_FOREVER;
		if (!spawn_dead)
			self->monsterInfo.stand(self);
	}

	if (spawn_dead) {
		// to spawn dead, we'll mimick them dying naturally
		self->health = 0;

		vec3_t f = self->s.origin;

		if (self->die)
			self->die(self, self, self, 0, vec3_origin, MOD_SUICIDE);

		if (!self->inUse)
			return;

		if (self->monsterInfo.setskin)
			self->monsterInfo.setskin(self);

		self->monsterInfo.aiFlags |= AI_SPAWNED_DEAD;

		auto move = self->monsterInfo.active_move.pointer();

		for (size_t i = move->firstFrame; i < move->lastFrame; i++) {
			self->s.frame = i;

			if (move->frame[i - move->firstFrame].thinkfunc)
				move->frame[i - move->firstFrame].thinkfunc(self);

			if (!self->inUse)
				return;
		}

		if (move->endFunc)
			move->endFunc(self);

		if (!self->inUse)
			return;

		if (self->monsterInfo.start_frame)
			self->s.frame = self->monsterInfo.start_frame;
		else
			self->s.frame = move->lastFrame;

		self->s.origin = f;
		gi.linkentity(self);

		self->monsterInfo.aiFlags &= ~AI_SPAWNED_DEAD;
	} else {
		self->think = monster_think;
		self->nextThink = level.time + FRAME_TIME_S;
		self->monsterInfo.aiFlags |= AI_SPAWNED_ALIVE;
	}
}

static THINK(walkmonster_start_go) (gentity_t *self) -> void {
	if (!self->yaw_speed)
		self->yaw_speed = 20;

	if (self->spawnflags.has(SPAWNFLAG_MONSTER_TRIGGER_SPAWN))
		monster_triggered_start(self);
	else
		monster_start_go(self);
}

void walkmonster_start(gentity_t *self) {
	self->think = walkmonster_start_go;
	monster_start(self);
}

static THINK(flymonster_start_go) (gentity_t *self) -> void {
	if (!self->yaw_speed)
		self->yaw_speed = 30;

	if (self->spawnflags.has(SPAWNFLAG_MONSTER_TRIGGER_SPAWN))
		monster_triggered_start(self);
	else
		monster_start_go(self);
}

void flymonster_start(gentity_t *self) {
	self->flags |= FL_FLY;
	self->think = flymonster_start_go;
	monster_start(self);
}

static THINK(swimmonster_start_go) (gentity_t *self) -> void {
	if (!self->yaw_speed)
		self->yaw_speed = 30;

	if (self->spawnflags.has(SPAWNFLAG_MONSTER_TRIGGER_SPAWN))
		monster_triggered_start(self);
	else
		monster_start_go(self);
}

void swimmonster_start(gentity_t *self) {
	self->flags |= FL_SWIM;
	self->think = swimmonster_start_go;
	monster_start(self);
}

static USE(trigger_health_relay_use) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	float percent_health = clamp((float)(other->health) / (float)(other->max_health), 0.f, 1.f);

	// not ready to trigger yet
	if (percent_health > self->speed)
		return;

	// fire!
	UseTargets(self, activator);

	// kill self
	FreeEntity(self);
}

/*QUAKED trigger_health_relay (1.0 1.0 0.0) (-8 -8 -8) (8 8 8) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Special type of relay that fires when a linked object is reduced
beyond a certain amount of health.

It will only fire once, and free itself afterwards.
*/
void SP_trigger_health_relay(gentity_t *self) {
	if (!self->targetname) {
		gi.Com_PrintFmt("{} missing targetname\n", *self);
		FreeEntity(self);
		return;
	}

	if (self->speed < 0 || self->speed > 100) {
		gi.Com_PrintFmt("{} has bad \"speed\" (health percentage); must be between 0 and 100, inclusive\n", *self);
		FreeEntity(self);
		return;
	}

	self->svFlags |= SVF_NOCLIENT;
	self->use = trigger_health_relay_use;
}

void monster_fire_blueblaster(gentity_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, monster_muzzleflash_id_t flashtype, Effect effect) {
	fire_blueblaster(self, start, dir, damage, speed, effect);
	monster_muzzleflash(self, start, flashtype);
}

void monster_fire_ionripper(gentity_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, monster_muzzleflash_id_t flashtype, Effect effect) {
	fire_ionripper(self, start, dir, damage, speed, effect);
	monster_muzzleflash(self, start, flashtype);
}

void monster_fire_heat(gentity_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, monster_muzzleflash_id_t flashtype, float turnFraction) {
	fire_heat(self, start, dir, damage, speed, (float)damage, damage, turnFraction);
	monster_muzzleflash(self, start, flashtype);
}

struct dabeam_pierce_t : pierce_args_t {
	gentity_t *self;
	bool damage;

	inline dabeam_pierce_t(gentity_t *self, bool damage) :
		pierce_args_t(),
		self(self),
		damage(damage) {}

	// we hit an entity; return false to stop the piercing.
	// you can adjust the mask for the re-trace (for water, etc).
	virtual bool hit(contents_t &mask, vec3_t &end) override {
		if (damage) {
			// hurt it if we can
			if (self->dmg > 0 && (tr.ent->takeDamage) && !(tr.ent->flags & FL_IMMUNE_LASER) && (tr.ent != self->owner))
				Damage(tr.ent, self, self->owner, self->movedir, tr.endpos, vec3_origin, self->dmg, skill->integer, DAMAGE_ENERGY, MOD_PLASMABEAM);

			if (self->dmg < 0) // healer ray
			{
				// when player is at 100 health
				// just undo health fix
				// keeping fx
				if (tr.ent->health < tr.ent->max_health)
					tr.ent->health = min(tr.ent->max_health, tr.ent->health - self->dmg);
			}
		}

		// if we hit something that's not a monster or player or is immune to lasers, we're done
		if (!(tr.ent->svFlags & SVF_MONSTER) && (!tr.ent->client)) {
			if (damage) {
				gi.WriteByte(svc_temp_entity);
				gi.WriteByte(TE_LASER_SPARKS);
				gi.WriteByte(10);
				gi.WritePosition(tr.endpos);
				gi.WriteDir(tr.plane.normal);
				gi.WriteByte(self->s.skinnum);
				gi.multicast(tr.endpos, MULTICAST_PVS, false);
			}

			return false;
		}

		if (!mark(tr.ent))
			return false;

		return true;
	}
};

void dabeam_update(gentity_t *self, bool damage) {
	vec3_t start = self->s.origin;
	vec3_t end = start + (self->movedir * 2048);

	dabeam_pierce_t args{
		self,
		damage
	};

	pierce_trace(start, end, self, args, CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_PLAYER | CONTENTS_DEADMONSTER);

	self->s.old_origin = args.tr.endpos + (args.tr.plane.normal * 1.f);
	gi.linkentity(self);
}

constexpr spawnflags_t SPAWNFLAG_DABEAM_SECONDARY = 1_spawnflag;

static THINK(beam_think) (gentity_t *self) -> void {
	if (self->spawnflags.has(SPAWNFLAG_DABEAM_SECONDARY))
		self->owner->beam2 = nullptr;
	else
		self->owner->beam = nullptr;
	FreeEntity(self);
}

void monster_fire_dabeam(gentity_t *self, int damage, bool secondary, void(*update_func)(gentity_t *self)) {
	gentity_t *&beam_ptr = secondary ? self->beam2 : self->beam;

	if (!beam_ptr) {
		beam_ptr = Spawn();

		beam_ptr->moveType = MOVETYPE_NONE;
		beam_ptr->solid = SOLID_NOT;
		beam_ptr->s.renderfx |= RF_BEAM;
		beam_ptr->s.modelindex = MODELINDEX_WORLD;
		beam_ptr->owner = self;
		beam_ptr->dmg = damage;
		beam_ptr->s.frame = 2;
		beam_ptr->spawnflags = secondary ? SPAWNFLAG_DABEAM_SECONDARY : SPAWNFLAG_NONE;

		if (self->monsterInfo.aiFlags & AI_MEDIC)
			beam_ptr->s.skinnum = 0xf3f3f1f1;
		else
			beam_ptr->s.skinnum = 0xf2f2f0f0;

		beam_ptr->think = beam_think;
		beam_ptr->s.sound = gi.soundindex("misc/lasfly.wav");
		beam_ptr->postthink = update_func;
	}

	beam_ptr->nextThink = level.time + 200_ms;
	update_func(beam_ptr);
	dabeam_update(beam_ptr, true);
}

void monster_fire_blaster2(gentity_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, monster_muzzleflash_id_t flashtype, Effect effect) {
	fire_greenblaster(self, start, dir, damage, speed, effect, false);
	monster_muzzleflash(self, start, flashtype);
}

void monster_fire_disruptor(gentity_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, gentity_t *enemy, monster_muzzleflash_id_t flashtype) {
	fire_disruptor(self, start, dir, damage, speed, enemy);
	monster_muzzleflash(self, start, flashtype);
}

void monster_fire_heatbeam(gentity_t *self, const vec3_t &start, const vec3_t &dir, const vec3_t &offset, int damage, int kick, monster_muzzleflash_id_t flashtype) {
	fire_plasmabeam(self, start, dir, offset, damage, kick, true);
	monster_muzzleflash(self, start, flashtype);
}

void stationarymonster_start_go(gentity_t *self);

static THINK(stationarymonster_triggered_spawn) (gentity_t *self) -> void {
	self->solid = SOLID_BBOX;
	self->moveType = MOVETYPE_NONE;
	self->svFlags &= ~SVF_NOCLIENT;
	self->airFinished = level.time + 12_sec;
	gi.linkentity(self);

	KillBox(self, false);

	// FIXME - why doesn't this happen with real monsters?
	self->spawnflags &= ~SPAWNFLAG_MONSTER_TRIGGER_SPAWN;

	stationarymonster_start_go(self);

	if (self->enemy && !(self->spawnflags & SPAWNFLAG_MONSTER_AMBUSH) && !(self->enemy->flags & FL_NOTARGET)) {
		if (!(self->enemy->flags & FL_DISGUISED)) // PGM
			FoundTarget(self);
		else // PMM - just in case, make sure to clear the enemy so FindTarget doesn't get confused
			self->enemy = nullptr;
	} else {
		self->enemy = nullptr;
	}
}

static USE(stationarymonster_triggered_spawn_use) (gentity_t *self, gentity_t *other, gentity_t *activator) -> void {
	// we have a one frame delay here so we don't telefrag the guy who activated us
	self->think = stationarymonster_triggered_spawn;
	self->nextThink = level.time + FRAME_TIME_S;
	if (activator && activator->client)
		self->enemy = activator;
	self->use = monster_use;
}

static void stationarymonster_triggered_start(gentity_t *self) {
	self->solid = SOLID_NOT;
	self->moveType = MOVETYPE_NONE;
	self->svFlags |= SVF_NOCLIENT;
	self->nextThink = 0_ms;
	self->use = stationarymonster_triggered_spawn_use;
}

THINK(stationarymonster_start_go) (gentity_t *self) -> void {
	if (!self->yaw_speed)
		self->yaw_speed = 20;

	monster_start_go(self);

	if (self->spawnflags.has(SPAWNFLAG_MONSTER_TRIGGER_SPAWN))
		stationarymonster_triggered_start(self);
}

void stationarymonster_start(gentity_t *self) {
	self->flags |= FL_STATIONARY;
	self->think = stationarymonster_start_go;
	monster_start(self);

	// fix viewHeight
	self->viewHeight = 0;
}

void monster_done_dodge(gentity_t *self) {
	self->monsterInfo.aiFlags &= ~AI_DODGING;
	if (self->monsterInfo.attack_state == AS_SLIDING)
		self->monsterInfo.attack_state = AS_STRAIGHT;
}

int32_t M_SlotsLeft(gentity_t *self) {
	return self->monsterInfo.monster_slots - self->monsterInfo.monster_used;
}
