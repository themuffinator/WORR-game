// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

// g_ai_new.cpp (Game AI - Advanced)
// This file contains advanced and alternative AI behaviors, extending the base
// logic from g_ai.cpp. It implements more complex actions and decision-making
// processes for monsters.
//
// Key Responsibilities:
// - Advanced Movement: Implements logic for monster jumping (`blocked_checkjump`),
//   dodging projectiles (`M_MonsterDodge`), and ducking under fire.
// - Pathfinding: Contains the "hint path" system, a legacy node-based pathing
//   mechanism that allows monsters to navigate complex environments when they
//   lose sight of a player (`monsterlost_checkhint`).
// - Special Interactions: Handles specific AI behaviors, such as targeting
//   and reacting to Tesla mines (`TargetTesla`).
// - Utility Functions: Provides various helper functions for AI, like checking
//   if a target is behind a monster (`inback`) or exploding bosses (`BossExplode`).

#include "g_local.hpp"

//===============================
// BLOCKED Logic
//===============================

bool face_wall(gentity_t* self);

// blocked_checkplat
//	dist: how far they are trying to walk.
bool blocked_checkplat(gentity_t* self, float dist) {
	int		 playerPosition;
	trace_t	 trace;
	Vector3	 pt1, pt2;
	Vector3	 forward;
	gentity_t* plat;

	if (!self->enemy)
		return false;

	// check player's relative altitude
	if (self->enemy->absMin[2] >= self->absMax[2])
		playerPosition = 1;
	else if (self->enemy->absMax[2] <= self->absMin[2])
		playerPosition = -1;
	else
		playerPosition = 0;

	// if we're close to the same position, don't bother trying plats.
	if (playerPosition == 0)
		return false;

	plat = nullptr;

	// see if we're already standing on a plat.
	if (self->groundEntity && self->groundEntity != world) {
		if (!strncmp(self->groundEntity->className, "func_plat", 8))
			plat = self->groundEntity;
	}

	// if we're not, check to see if we'll step onto one with this move
	if (!plat) {
		AngleVectors(self->s.angles, forward, nullptr, nullptr);
		pt1 = self->s.origin + (forward * dist);
		pt2 = pt1;
		pt2[2] -= 384;

		trace = gi.traceLine(pt1, pt2, self, MASK_MONSTERSOLID);
		if (trace.fraction < 1 && !trace.allSolid && !trace.startSolid) {
			if (!strncmp(trace.ent->className, "func_plat", 8)) {
				plat = trace.ent;
			}
		}
	}

	// if we've found a plat, trigger it.
	if (plat && plat->use) {
		if (playerPosition == 1) {
			if ((self->groundEntity == plat && plat->moveInfo.state == MoveState::Bottom) ||
				(self->groundEntity != plat && plat->moveInfo.state == MoveState::Top)) {
				plat->use(plat, self, self);
				return true;
			}
		}
		else if (playerPosition == -1) {
			if ((self->groundEntity == plat && plat->moveInfo.state == MoveState::Top) ||
				(self->groundEntity != plat && plat->moveInfo.state == MoveState::Bottom)) {
				plat->use(plat, self, self);
				return true;
			}
		}
	}

	return false;
}

//*******************
// JUMPING AIDS
//*******************

static inline void monster_jump_start(gentity_t* self) {
	monster_done_dodge(self);

	self->monsterInfo.jump_time = level.time + 3_sec;
}

bool monster_jump_finished(gentity_t* self) {
	// if we lost our forward velocity, give us more
	Vector3 forward;

	AngleVectors(self->s.angles, forward, nullptr, nullptr);

	Vector3 forward_velocity = self->velocity.scaled(forward);

	if (forward_velocity.length() < 150.f) {
		float z_velocity = self->velocity.z;
		self->velocity = forward * 150.f;
		self->velocity.z = z_velocity;
	}

	return self->monsterInfo.jump_time < level.time;
}

// blocked_checkjump
//	dist: how far they are trying to walk.
//  self->monsterInfo.dropHeight/self->monsterInfo.jumpHeight: how far they'll ok a jump for. set to 0 to disable that direction.
BlockedJumpResult blocked_checkjump(gentity_t* self, float dist) {
	// can't jump even if we physically can
	if (!self->monsterInfo.canJump)
		return BlockedJumpResult::No_Jump;
	// no enemy to path to
	else if (!self->enemy)
		return BlockedJumpResult::No_Jump;

	// we just jumped recently, don't try again
	if (self->monsterInfo.jump_time > level.time)
		return BlockedJumpResult::No_Jump;

	// if we're pathing, the nodes will ensure we can reach the destination.
	if (self->monsterInfo.aiFlags & AI_PATHING) {
		if (self->monsterInfo.nav_path.returnCode != PathReturnCode::TraversalPending)
			return BlockedJumpResult::No_Jump;

		float yaw = vectoyaw((self->monsterInfo.nav_path.firstMovePoint - self->monsterInfo.nav_path.secondMovePoint).normalized());
		self->ideal_yaw = yaw + 180;
		if (self->ideal_yaw > 360)
			self->ideal_yaw -= 360;

		if (!FacingIdeal(self)) {
			M_ChangeYaw(self);
			return BlockedJumpResult::Jump_Turn;
		}

		monster_jump_start(self);

		if (self->monsterInfo.nav_path.secondMovePoint.z > self->monsterInfo.nav_path.firstMovePoint.z)
			return BlockedJumpResult::Jump_Turn_Up;
		else
			return BlockedJumpResult::Jump_Turn_Down;
	}

	int		playerPosition;
	trace_t trace;
	Vector3	pt1, pt2;
	Vector3	forward, up;

	AngleVectors(self->s.angles, forward, nullptr, up);

	if (self->monsterInfo.aiFlags & AI_PATHING) {
		if (self->monsterInfo.nav_path.secondMovePoint[2] > (self->absMin[2] + (self->s.origin[Z] < 0 ? STEPSIZE_BELOW : STEPSIZE)))
			playerPosition = 1;
		else if (self->monsterInfo.nav_path.secondMovePoint[2] < (self->absMin[2] - (self->s.origin[Z] < 0 ? STEPSIZE_BELOW : STEPSIZE)))
			playerPosition = -1;
		else
			playerPosition = 0;
	}
	else {
		if (self->enemy->absMin[2] > (self->absMin[2] + (self->s.origin[Z] < 0 ? STEPSIZE_BELOW : STEPSIZE)))
			playerPosition = 1;
		else if (self->enemy->absMin[2] < (self->absMin[2] - (self->s.origin[Z] < 0 ? STEPSIZE_BELOW : STEPSIZE)))
			playerPosition = -1;
		else
			playerPosition = 0;
	}

	if (playerPosition == -1 && self->monsterInfo.dropHeight) {
		// check to make sure we can even get to the spot we're going to "fall" from
		pt1 = self->s.origin + (forward * 48);
		trace = gi.trace(self->s.origin, self->mins, self->maxs, pt1, self, MASK_MONSTERSOLID);
		if (trace.fraction < 1)
			return BlockedJumpResult::No_Jump;

		pt2 = pt1;
		pt2[2] = self->absMin[2] - self->monsterInfo.dropHeight - 1;

		trace = gi.traceLine(pt1, pt2, self, MASK_MONSTERSOLID | MASK_WATER);
		if (trace.fraction < 1 && !trace.allSolid && !trace.startSolid) {
			// check how deep the water is
			if (trace.contents & CONTENTS_WATER) {
				trace_t deep = gi.traceLine(trace.endPos, pt2, self, MASK_MONSTERSOLID);

				water_level_t waterLevel;
				contents_t waterType;
				M_CatagorizePosition(self, deep.endPos, waterLevel, waterType);

				if (waterLevel > WATER_WAIST)
					return BlockedJumpResult::No_Jump;
			}

			if ((self->absMin[2] - trace.endPos[2]) >= 24 && (trace.contents & (MASK_SOLID | CONTENTS_WATER))) {
				if (self->monsterInfo.aiFlags & AI_PATHING) {
					if ((self->monsterInfo.nav_path.secondMovePoint[2] - trace.endPos[2]) > 32)
						return BlockedJumpResult::No_Jump;
				}
				else {
					if ((self->enemy->absMin[2] - trace.endPos[2]) > 32)
						return BlockedJumpResult::No_Jump;

					if (trace.plane.normal[2] < 0.9f)
						return BlockedJumpResult::No_Jump;
				}

				monster_jump_start(self);

				return BlockedJumpResult::Jump_Turn_Down;
			}
		}
	}
	else if (playerPosition == 1 && self->monsterInfo.jumpHeight) {
		pt1 = self->s.origin + (forward * 48);
		pt2 = pt1;
		pt1[2] = self->absMax[2] + self->monsterInfo.jumpHeight;

		trace = gi.traceLine(pt1, pt2, self, MASK_MONSTERSOLID | MASK_WATER);
		if (trace.fraction < 1 && !trace.allSolid && !trace.startSolid) {
			if ((trace.endPos[2] - self->absMin[2]) <= self->monsterInfo.jumpHeight && (trace.contents & (MASK_SOLID | CONTENTS_WATER))) {
				face_wall(self);

				monster_jump_start(self);

				return BlockedJumpResult::Jump_Turn_Up;
			}
		}
	}

	return BlockedJumpResult::No_Jump;
}

// *************************
// HINT PATHS
// *************************

constexpr SpawnFlags SPAWNFLAG_HINT_ENDPOINT = 0x0001_spawnflag;
constexpr size_t   MAX_HINT_CHAINS = 100;

int		 hint_paths_present;
gentity_t* hint_path_start[MAX_HINT_CHAINS];
int		 num_hint_paths;

//
// AI code
//

// =============
// hintpath_findstart - given any hintpath node, finds the start node
// =============
static gentity_t* hintpath_findstart(gentity_t* ent) {
	gentity_t* e;
	gentity_t* last;

	if (ent->target) // starting point
	{
		last = world;
		e = G_FindByString<&gentity_t::targetName>(nullptr, ent->target);
		while (e) {
			last = e;
			if (!e->target)
				break;
			e = G_FindByString<&gentity_t::targetName>(nullptr, e->target);
		}
	}
	else // end point
	{
		last = world;
		e = G_FindByString<&gentity_t::target>(nullptr, ent->targetName);
		while (e) {
			last = e;
			if (!e->targetName)
				break;
			e = G_FindByString<&gentity_t::target>(nullptr, e->targetName);
		}
	}

	if (!last->spawnFlags.has(SPAWNFLAG_HINT_ENDPOINT))
		return nullptr;

	if (last == world)
		last = nullptr;
	return last;
}

// =============
// hintpath_other_end - given one endpoint of a hintpath, returns the other end.
// =============
static gentity_t* hintpath_other_end(gentity_t* ent) {
	gentity_t* e;
	gentity_t* last;

	if (ent->target) // starting point
	{
		last = world;
		e = G_FindByString<&gentity_t::targetName>(nullptr, ent->target);
		while (e) {
			last = e;
			if (!e->target)
				break;
			e = G_FindByString<&gentity_t::targetName>(nullptr, e->target);
		}
	}
	else // end point
	{
		last = world;
		e = G_FindByString<&gentity_t::target>(nullptr, ent->targetName);
		while (e) {
			last = e;
			if (!e->targetName)
				break;
			e = G_FindByString<&gentity_t::target>(nullptr, e->targetName);
		}
	}

	if (!(last->spawnFlags & SPAWNFLAG_HINT_ENDPOINT))
		return nullptr;

	if (last == world)
		last = nullptr;
	return last;
}

// =============
// hintpath_go - starts a monster (self) moving towards the hintpath (point)
//		disables all contrary AI flags.
// =============
static void hintpath_go(gentity_t* self, gentity_t* point) {
	Vector3 dir;

	dir = point->s.origin - self->s.origin;

	self->ideal_yaw = vectoyaw(dir);
	self->goalEntity = self->moveTarget = point;
	self->monsterInfo.pauseTime = 0_ms;
	self->monsterInfo.aiFlags |= AI_HINT_PATH;
	self->monsterInfo.aiFlags &= ~(AI_SOUND_TARGET | AI_PURSUIT_LAST_SEEN | AI_PURSUE_NEXT | AI_PURSUE_TEMP);
	// run for it
	self->monsterInfo.searchTime = level.time;
	self->monsterInfo.run(self);
}

// =============
// hintpath_stop - bails a monster out of following hint paths
// =============
void hintpath_stop(gentity_t* self) {
	self->goalEntity = nullptr;
	self->moveTarget = nullptr;
	self->monsterInfo.last_hint_time = level.time;
	self->monsterInfo.goal_hint = nullptr;
	self->monsterInfo.aiFlags &= ~AI_HINT_PATH;
	if (has_valid_enemy(self)) {
		// if we can see our target, go nuts
		if (visible(self, self->enemy)) {
			FoundTarget(self);
			return;
		}
		// otherwise, keep chasing
		HuntTarget(self);
		return;
	}
	// if our enemy is no longer valid, forget about our enemy and go into stand
	self->enemy = nullptr;
	// we need the pauseTime otherwise the stand code
	// will just revert to walking with no target and
	// the monsters will wonder around aimlessly trying
	// to hunt the world entity
	self->monsterInfo.pauseTime = HOLD_FOREVER;
	self->monsterInfo.stand(self);
}

// =============
// monsterlost_checkhint - the monster (self) will check around for valid hintpaths.
//		a valid hintpath is one where the two endpoints can see both the monster
//		and the monster's enemy. if only one person is visible from the endpoints,
//		it will not go for it.
// =============
bool monsterlost_checkhint(gentity_t* self) {
	gentity_t* e, * monster_pathchain, * target_pathchain, * checkpoint = nullptr;
	gentity_t* closest;
	float	 closest_range = 1000000;
	gentity_t* start, * destination;
	int		 count5 = 0;
	float	 r;
	int		 i;
	bool	 hint_path_represented[MAX_HINT_CHAINS]{};

	// if there are no hint paths on this map, exit immediately.
	if (!hint_paths_present)
		return false;

	if (!self->enemy)
		return false;

	// [Paril-KEX] don't do hint paths if we're using nav nodes
	if (self->monsterInfo.aiFlags & (AI_STAND_GROUND | AI_PATHING))
		return false;

	if (!Q_strcasecmp(self->className, "monster_turret"))
		return false;

	monster_pathchain = nullptr;

	// find all the hint_paths.
	// FIXME - can we not do this every time?
	for (i = 0; i < num_hint_paths; i++) {
		e = hint_path_start[i];
		while (e) {
			if (e->monster_hint_chain)
				e->monster_hint_chain = nullptr;

			if (monster_pathchain) {
				checkpoint->monster_hint_chain = e;
				checkpoint = e;
			}
			else {
				monster_pathchain = e;
				checkpoint = e;
			}
			e = e->hint_chain;
		}
	}

	// filter them by distance and visibility to the monster
	e = monster_pathchain;
	checkpoint = nullptr;
	while (e) {
		r = realrange(self, e);

		if (r > 512) {
			if (checkpoint) {
				checkpoint->monster_hint_chain = e->monster_hint_chain;
				e->monster_hint_chain = nullptr;
				e = checkpoint->monster_hint_chain;
				continue;
			}
			else {
				// use checkpoint as temp pointer
				checkpoint = e;
				e = e->monster_hint_chain;
				checkpoint->monster_hint_chain = nullptr;
				// and clear it again
				checkpoint = nullptr;
				// since we have yet to find a valid one (or else checkpoint would be set) move the
				// start of monster_pathchain
				monster_pathchain = e;
				continue;
			}
		}
		if (!visible(self, e)) {
			if (checkpoint) {
				checkpoint->monster_hint_chain = e->monster_hint_chain;
				e->monster_hint_chain = nullptr;
				e = checkpoint->monster_hint_chain;
				continue;
			}
			else {
				// use checkpoint as temp pointer
				checkpoint = e;
				e = e->monster_hint_chain;
				checkpoint->monster_hint_chain = nullptr;
				// and clear it again
				checkpoint = nullptr;
				// since we have yet to find a valid one (or else checkpoint would be set) move the
				// start of monster_pathchain
				monster_pathchain = e;
				continue;
			}
		}

		count5++;
		checkpoint = e;
		e = e->monster_hint_chain;
	}

	// at this point, we have a list of all of the eligible hint nodes for the monster
	// we now take them, figure out what hint chains they're on, and traverse down those chains,
	// seeing whether any can see the player
	//
	// first, we figure out which hint chains we have represented in monster_pathchain
	if (count5 == 0)
		return false;

	for (i = 0; i < num_hint_paths; i++)
		hint_path_represented[i] = false;

	e = monster_pathchain;
	checkpoint = nullptr;
	while (e) {
		if ((e->hint_chain_id < 0) || (e->hint_chain_id > num_hint_paths))
			return false;

		hint_path_represented[e->hint_chain_id] = true;
		e = e->monster_hint_chain;
	}

	count5 = 0;

	// now, build the target_pathchain which contains all of the hint_path nodes we need to check for
	// validity (within range, visibility)
	target_pathchain = nullptr;
	checkpoint = nullptr;
	for (i = 0; i < num_hint_paths; i++) {
		// if this hint chain is represented in the monster_hint_chain, add all of it's nodes to the target_pathchain
		// for validity checking
		if (hint_path_represented[i]) {
			e = hint_path_start[i];
			while (e) {
				if (target_pathchain) {
					checkpoint->target_hint_chain = e;
					checkpoint = e;
				}
				else {
					target_pathchain = e;
					checkpoint = e;
				}
				e = e->hint_chain;
			}
		}
	}

	// target_pathchain is a list of all of the hint_path nodes we need to check for validity relative to the target
	e = target_pathchain;
	checkpoint = nullptr;
	while (e) {
		r = realrange(self->enemy, e);

		if (r > 512) {
			if (checkpoint) {
				checkpoint->target_hint_chain = e->target_hint_chain;
				e->target_hint_chain = nullptr;
				e = checkpoint->target_hint_chain;
				continue;
			}
			else {
				// use checkpoint as temp pointer
				checkpoint = e;
				e = e->target_hint_chain;
				checkpoint->target_hint_chain = nullptr;
				// and clear it again
				checkpoint = nullptr;
				target_pathchain = e;
				continue;
			}
		}
		if (!visible(self->enemy, e)) {
			if (checkpoint) {
				checkpoint->target_hint_chain = e->target_hint_chain;
				e->target_hint_chain = nullptr;
				e = checkpoint->target_hint_chain;
				continue;
			}
			else {
				// use checkpoint as temp pointer
				checkpoint = e;
				e = e->target_hint_chain;
				checkpoint->target_hint_chain = nullptr;
				// and clear it again
				checkpoint = nullptr;
				target_pathchain = e;
				continue;
			}
		}

		count5++;
		checkpoint = e;
		e = e->target_hint_chain;
	}

	// at this point we should have:
	// monster_pathchain - a list of "monster valid" hint_path nodes linked together by monster_hint_chain
	// target_pathcain - a list of "target valid" hint_path nodes linked together by target_hint_chain.  these
	//                   are filtered such that only nodes which are on the same chain as "monster valid" nodes
	//
	// Now, we figure out which "monster valid" node we want to use
	//
	// To do this, we first off make sure we have some target nodes.  If we don't, there are no valid hint_path nodes
	// for us to take
	//
	// If we have some, we filter all of our "monster valid" nodes by which ones have "target valid" nodes on them
	//
	// Once this filter is finished, we select the closest "monster valid" node, and go to it.

	if (count5 == 0)
		return false;

	// reuse the hint_chain_represented array, this time to see which chains are represented by the target
	for (i = 0; i < num_hint_paths; i++)
		hint_path_represented[i] = false;

	e = target_pathchain;
	checkpoint = nullptr;
	while (e) {
		if ((e->hint_chain_id < 0) || (e->hint_chain_id > num_hint_paths))
			return false;

		hint_path_represented[e->hint_chain_id] = true;
		e = e->target_hint_chain;
	}

	// traverse the monster_pathchain - if the hint_node isn't represented in the "target valid" chain list,
	// remove it
	// if it is on the list, check it for range from the monster.  If the range is the closest, keep it
	//
	closest = nullptr;
	e = monster_pathchain;
	while (e) {
		if (!(hint_path_represented[e->hint_chain_id])) {
			checkpoint = e->monster_hint_chain;
			e->monster_hint_chain = nullptr;
			e = checkpoint;
			continue;
		}
		r = realrange(self, e);
		if (r < closest_range)
			closest = e;
		e = e->monster_hint_chain;
	}

	if (!closest)
		return false;

	start = closest;
	// now we know which one is the closest to the monster .. this is the one the monster will go to
	// we need to finally determine what the DESTINATION node is for the monster .. walk down the hint_chain,
	// and find the closest one to the player

	closest = nullptr;
	closest_range = 10000000;
	e = target_pathchain;
	while (e) {
		if (start->hint_chain_id == e->hint_chain_id) {
			r = realrange(self, e);
			if (r < closest_range)
				closest = e;
		}
		e = e->target_hint_chain;
	}

	if (!closest)
		return false;

	destination = closest;

	self->monsterInfo.goal_hint = destination;
	hintpath_go(self, start);

	return true;
}

//
// Path code
//

// =============
// hint_path_touch - someone's touched the hint_path
// =============
static TOUCH(hint_path_touch) (gentity_t* self, gentity_t* other, const trace_t& tr, bool otherTouchingSelf) -> void {
	gentity_t* e, * goal, * next = nullptr;
	bool goalFound = false;

	// make sure we're the target of it's obsession
	if (other->moveTarget == self) {
		goal = other->monsterInfo.goal_hint;

		// if the monster is where he wants to be
		if (goal == self) {
			hintpath_stop(other);
			return;
		}
		else {
			// if we aren't, figure out which way we want to go
			e = hint_path_start[self->hint_chain_id];
			while (e) {
				// if we get up to ourselves on the hint chain, we're going down it
				if (e == self) {
					next = e->hint_chain;
					break;
				}
				if (e == goal)
					goalFound = true;
				// if we get to where the next link on the chain is this hint_path and have found the goal on the way
				// we're going upstream, so remember who the previous link is
				if ((e->hint_chain == self) && goalFound) {
					next = e;
					break;
				}
				e = e->hint_chain;
			}
		}

		// if we couldn't find it, have the monster go back to normal hunting.
		if (!next) {
			hintpath_stop(other);
			return;
		}

		// send him on his way
		hintpath_go(other, next);

		// have the monster freeze if the hint path we just touched has a wait time
		// on it, for example, when riding a plat.
		if (self->wait)
			other->nextThink = level.time + GameTime::from_sec(self->wait);
	}
}

/*QUAKED hint_path (.5 .3 0) (-8 -8 -8) (8 8 8) END x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Target: next hint path

END - set this flag on the endpoints of each hintpath.

"wait" - set this if you want the monster to freeze when they touch this hintpath
*/
void SP_hint_path(gentity_t* self) {
	if (deathmatch->integer) {
		FreeEntity(self);
		return;
	}

	if (!self->targetName && !self->target) {
		gi.Com_PrintFmt("{}: unlinked\n", *self);
		FreeEntity(self);
		return;
	}

	self->solid = SOLID_TRIGGER;
	self->touch = hint_path_touch;
	self->mins = { -8, -8, -8 };
	self->maxs = { 8, 8, 8 };
	self->svFlags |= SVF_NOCLIENT;
	gi.linkEntity(self);
}

// ============
// InitHintPaths - Called by InitGame (g_save) to enable quick exits if valid
// ============
void InitHintPaths() {
	gentity_t* e, * current;
	int		 i;

	hint_paths_present = 0;

	// check all the hint_paths.
	e = G_FindByString<&gentity_t::className>(nullptr, "hint_path");
	if (e)
		hint_paths_present = 1;
	else
		return;

	memset(hint_path_start, 0, MAX_HINT_CHAINS * sizeof(gentity_t*));
	num_hint_paths = 0;
	while (e) {
		if (e->spawnFlags.has(SPAWNFLAG_HINT_ENDPOINT)) {
			if (e->target) // start point
			{
				if (e->targetName) // this is a bad end, ignore it
				{
					gi.Com_PrintFmt("{}: marked as endpoint with both target ({}) and targetName ({})\n",
						*e, e->target, e->targetName);
				}
				else {
					if (num_hint_paths >= MAX_HINT_CHAINS)
						break;

					hint_path_start[num_hint_paths++] = e;
				}
			}
		}
		e = G_FindByString<&gentity_t::className>(e, "hint_path");
	}

	for (i = 0; i < num_hint_paths; i++) {
		current = hint_path_start[i];
		current->hint_chain_id = i;
		e = G_FindByString<&gentity_t::targetName>(nullptr, current->target);
		if (G_FindByString<&gentity_t::targetName>(e, current->target)) {
			gi.Com_PrintFmt("{}: Forked path detected for chain {}, target {}\n",
				*current, num_hint_paths, current->target);
			hint_path_start[i]->hint_chain = nullptr;
			continue;
		}
		while (e) {
			if (e->hint_chain) {
				gi.Com_PrintFmt("{}: Circular path detected for chain {}, targetName {}\n",
					*e, num_hint_paths, e->targetName);
				hint_path_start[i]->hint_chain = nullptr;
				break;
			}
			current->hint_chain = e;
			current = e;
			current->hint_chain_id = i;
			if (!current->target)
				break;
			e = G_FindByString<&gentity_t::targetName>(nullptr, current->target);
			if (G_FindByString<&gentity_t::targetName>(e, current->target)) {
				gi.Com_PrintFmt("{}: Forked path detected for chain {}, target {}\n",
					*current, num_hint_paths, current->target);
				hint_path_start[i]->hint_chain = nullptr;
				break;
			}
		}
	}
}

// *****************************
//	MISCELLANEOUS STUFF
// *****************************

// PMM - inback
// use to see if opponent is behind you (not to side)
// if it looks a lot like infront, well, there's a reason

bool inback(gentity_t* self, gentity_t* other) {
	Vector3 vec;
	float  dot;
	Vector3 forward;

	AngleVectors(self->s.angles, forward, nullptr, nullptr);
	vec = other->s.origin - self->s.origin;
	vec.normalize();
	dot = vec.dot(forward);
	return dot < -0.3f;
}

float realrange(gentity_t* self, gentity_t* other) {
	Vector3 dir;

	dir = self->s.origin - other->s.origin;

	return dir.length();
}

bool face_wall(gentity_t* self) {
	Vector3	pt;
	Vector3	forward;
	Vector3	ang;
	trace_t tr;

	AngleVectors(self->s.angles, forward, nullptr, nullptr);
	pt = self->s.origin + (forward * 64);
	tr = gi.traceLine(self->s.origin, pt, self, MASK_MONSTERSOLID);
	if (tr.fraction < 1 && !tr.allSolid && !tr.startSolid) {
		ang = VectorToAngles(tr.plane.normal);
		self->ideal_yaw = ang[YAW] + 180;
		if (self->ideal_yaw > 360)
			self->ideal_yaw -= 360;

		M_ChangeYaw(self);
		return true;
	}

	return false;
}

//
// Monster "Bad" Areas
//

static TOUCH(badarea_touch) (gentity_t* ent, gentity_t* other, const trace_t& tr, bool otherTouchingSelf) -> void {}

gentity_t* SpawnBadArea(const Vector3& mins, const Vector3& maxs, GameTime lifespan, gentity_t* owner) {
	gentity_t* badarea;
	Vector3	 origin;

	origin = mins + maxs;
	origin *= 0.5f;

	badarea = Spawn();
	badarea->s.origin = origin;

	badarea->maxs = maxs - origin;
	badarea->mins = mins - origin;
	badarea->touch = badarea_touch;
	badarea->moveType = MoveType::None;
	badarea->solid = SOLID_TRIGGER;
	badarea->className = "bad_area";
	gi.linkEntity(badarea);

	if (lifespan) {
		badarea->think = FreeEntity;
		badarea->nextThink = level.time + lifespan;
	}
	if (owner) {
		badarea->owner = owner;
	}

	return badarea;
}

static BoxEntitiesResult_t CheckForBadArea_BoxFilter(gentity_t* hit, void* data) {
	gentity_t*& result = (gentity_t*&)data;

	if (hit->touch == badarea_touch) {
		result = hit;
		return BoxEntitiesResult_t::End;
	}

	return BoxEntitiesResult_t::Skip;
}

// CheckForBadArea
//		This is a customized version of TouchTriggers that will check
//		for bad area triggers and return them if they're touched.
gentity_t* CheckForBadArea(gentity_t* ent) {
	Vector3	 mins, maxs;

	mins = ent->s.origin + ent->mins;
	maxs = ent->s.origin + ent->maxs;

	gentity_t* hit = nullptr;

	gi.BoxEntities(mins, maxs, nullptr, 0, AREA_TRIGGERS, CheckForBadArea_BoxFilter, &hit);

	return hit;
}

constexpr float TESLA_DAMAGE_RADIUS = 128;

bool MarkTeslaArea(gentity_t* self, gentity_t* tesla) {
	Vector3	 mins, maxs;
	gentity_t* e;
	gentity_t* tail;
	gentity_t* area;

	if (!tesla || !self)
		return false;

	area = nullptr;

	// make sure this tesla doesn't have a bad area around it already...
	e = tesla->teamChain;
	tail = tesla;
	while (e) {
		tail = tail->teamChain;
		if (!strcmp(e->className, "bad_area"))
			return false;

		e = e->teamChain;
	}

	// see if we can grab the trigger directly
	if (tesla->teamChain && tesla->teamChain->inUse) {
		gentity_t* trigger;

		trigger = tesla->teamChain;

		mins = trigger->absMin;
		maxs = trigger->absMax;

		if (tesla->airFinished)
			area = SpawnBadArea(mins, maxs, tesla->airFinished, tesla);
		else
			area = SpawnBadArea(mins, maxs, tesla->nextThink, tesla);
	}
	// otherwise we just guess at how long it'll last.
	else {

		mins = { -TESLA_DAMAGE_RADIUS, -TESLA_DAMAGE_RADIUS, tesla->mins[2] };
		maxs = { TESLA_DAMAGE_RADIUS, TESLA_DAMAGE_RADIUS, TESLA_DAMAGE_RADIUS };

		area = SpawnBadArea(mins, maxs, 30_sec, tesla);
	}

	// if we spawned a bad area, then link it to the tesla
	if (area)
		tail->teamChain = area;

	return true;
}

// predictive calculator
// target is who you want to shoot
// start is where the shot comes from
// bolt_speed is how fast the shot is (or 0 for hitscan)
// eye_height is a boolean to say whether or not to adjust to targets eye_height
// offset is how much time to miss by
// aimDir is the resulting aim direction (pass in nullptr if you don't want it)
// aimPoint is the resulting aimPoint (pass in nullptr if don't want it)
void PredictAim(gentity_t* self, gentity_t* target, const Vector3& start, float bolt_speed, bool eye_height, float offset, Vector3* aimDir, Vector3* aimPoint) {
	Vector3 dir, vec;
	float  dist, time;

	if (!target || !target->inUse) {
		*aimDir = {};
		return;
	}

	dir = target->s.origin - start;
	if (eye_height)
		dir[2] += target->viewHeight;
	dist = dir.length();

	// [Paril-KEX] if our current attempt is blocked, try the opposite one
	trace_t tr = gi.traceLine(start, start + dir, self, MASK_PROJECTILE);

	if (tr.ent != target) {
		eye_height = !eye_height;
		dir = target->s.origin - start;
		if (eye_height)
			dir[2] += target->viewHeight;
		dist = dir.length();
	}

	if (bolt_speed)
		time = dist / bolt_speed;
	else
		time = 0;

	vec = target->s.origin + (target->velocity * (time - offset));

	// went backwards...
	if (dir.normalized().dot((vec - start).normalized()) < 0)
		vec = target->s.origin;
	else {
		// if the shot is going to impact a nearby wall from our prediction, just fire it straight.	
		if (gi.traceLine(start, vec, nullptr, MASK_SOLID).fraction < 0.9f)
			vec = target->s.origin;
	}

	if (eye_height)
		vec[2] += target->viewHeight;

	if (aimDir)
		*aimDir = (vec - start).normalized();

	if (aimPoint)
		*aimPoint = vec;
}

// [Paril-KEX] find a pitch that will at some point land on or near the player.
// very approximate. aim will be adjusted to the correct aim vector.
bool M_CalculatePitchToFire(gentity_t* self, const Vector3& target, const Vector3& start, Vector3& aim, float speed, float time_remaining, bool mortar, bool destroy_on_touch) {
	constexpr float pitches[] = { -80.f, -70.f, -60.f, -50.f, -40.f, -30.f, -20.f, -10.f, -5.f };
	float best_pitch = 0.f;
	float best_dist = std::numeric_limits<float>::infinity();

	constexpr float sim_time = 0.1f;
	Vector3 pitched_aim = VectorToAngles(aim);

	for (auto& pitch : pitches) {
		if (mortar && pitch >= -30.f)
			break;

		pitched_aim[PITCH] = pitch;
		Vector3 fwd = AngleVectors(pitched_aim).forward;

		Vector3 velocity = fwd * speed;
		Vector3 origin = start;

		float t = time_remaining;

		while (t > 0.f) {
			velocity += Vector3{ 0, 0, -1 } *level.gravity * sim_time;

			Vector3 end = origin + (velocity * sim_time);
			trace_t tr = gi.traceLine(origin, end, nullptr, MASK_SHOT);

			origin = tr.endPos;

			if (tr.fraction < 1.0f) {
				if (tr.surface->flags & SURF_SKY)
					break;

				origin += tr.plane.normal;
				velocity = ClipVelocity(velocity, tr.plane.normal, 1.6f);

				float dist = (origin - target).lengthSquared();

				if (tr.ent == self->enemy || tr.ent->client || (tr.plane.normal.z >= 0.7f && dist < (128.f * 128.f) && dist < best_dist)) {
					best_pitch = pitch;
					best_dist = dist;
				}

				if (destroy_on_touch || (tr.contents & (CONTENTS_MONSTER | CONTENTS_PLAYER | CONTENTS_DEADMONSTER)))
					break;
			}

			t -= sim_time;
		}
	}

	if (!isinf(best_dist)) {
		pitched_aim[PITCH] = best_pitch;
		aim = AngleVectors(pitched_aim).forward;
		return true;
	}

	return false;
}

bool below(gentity_t* self, gentity_t* other) {
	Vector3 vec;
	float  dot;
	Vector3 down;

	vec = other->s.origin - self->s.origin;
	vec.normalize();
	down = { 0, 0, -1 };
	dot = vec.dot(down);

	if (dot > 0.95f) // 18 degree arc below
		return true;
	return false;
}

void drawbbox(gentity_t* self) {
	int lines[4][3] = {
		{ 1, 2, 4 },
		{ 1, 2, 7 },
		{ 1, 4, 5 },
		{ 2, 4, 7 }
	};

	int starts[4] = { 0, 3, 5, 6 };

	Vector3 pt[8]{};
	int	   i, j, k;
	Vector3 coords[2]{};
	Vector3 newbox;
	Vector3 f, r, u, dir;

	coords[0] = self->absMin;
	coords[1] = self->absMax;

	for (i = 0; i <= 1; i++) {
		for (j = 0; j <= 1; j++) {
			for (k = 0; k <= 1; k++) {
				pt[4 * i + 2 * j + k][0] = coords[i][0];
				pt[4 * i + 2 * j + k][1] = coords[j][1];
				pt[4 * i + 2 * j + k][2] = coords[k][2];
			}
		}
	}

	for (i = 0; i <= 3; i++) {
		for (j = 0; j <= 2; j++) {
			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_DEBUGTRAIL);
			gi.WritePosition(pt[starts[i]]);
			gi.WritePosition(pt[lines[i][j]]);
			gi.multicast(pt[starts[i]], MULTICAST_ALL, false);
		}
	}

	dir = VectorToAngles(self->s.angles);
	AngleVectors(dir, f, r, u);

	newbox = self->s.origin + (f * 50);
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_DEBUGTRAIL);
	gi.WritePosition(self->s.origin);
	gi.WritePosition(newbox);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);

	newbox = self->s.origin + (r * 50);
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_DEBUGTRAIL);
	gi.WritePosition(self->s.origin);
	gi.WritePosition(newbox);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);

	newbox = self->s.origin + (u * 50);
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_DEBUGTRAIL);
	gi.WritePosition(self->s.origin);
	gi.WritePosition(newbox);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);
}

// [Paril-KEX] returns true if the skill check passes
static inline bool G_SkillCheck(const std::initializer_list<float>& skills) {
	if (skills.size() < skill->integer)
		return true;

	const float& skill_switch = *(skills.begin() + skill->integer);
	return skill_switch == 1.0f ? true : frandom() < skill_switch;
}

//
// New dodge code
//
MONSTERINFO_DODGE(M_MonsterDodge) (gentity_t* self, gentity_t* attacker, GameTime eta, trace_t* tr, bool gravity) -> void {
	float r = frandom();
	float height;
	bool  ducker = false, dodger = false;

	// this needs to be here since this can be called after the monster has "died"
	if (self->health < 1)
		return;

	if ((self->monsterInfo.duck) && (self->monsterInfo.unDuck) && !gravity)
		ducker = true;
	if ((self->monsterInfo.sideStep) && !(self->monsterInfo.aiFlags & AI_STAND_GROUND))
		dodger = true;

	if ((!ducker) && (!dodger))
		return;

	if (!self->enemy) {
		self->enemy = attacker;
		FoundTarget(self);
	}

	// PMM - don't bother if it's going to hit anyway; fix for weird in-your-face etas (I was
	// seeing numbers like 13 and 14)
	if ((eta < FRAME_TIME_MS) || (eta > 2.5_sec))
		return;

	// skill level determination..
	if (r > 0.50f)
		return;

	if (ducker && tr) {
		height = self->absMax[2] - 32 - 1; // the -1 is because the absMax is s.origin + maxs + 1

		if ((!dodger) && ((tr->endPos[2] <= height) || (self->monsterInfo.aiFlags & AI_DUCKED)))
			return;
	}
	else
		height = self->absMax[2];

	if (dodger) {
		// if we're already dodging, just finish the sequence, i.e. don't do anything else
		if (self->monsterInfo.aiFlags & AI_DODGING)
			return;

		// if we're ducking already, or the shot is at our knees
		if ((!ducker || !tr || tr->endPos[2] <= height) || (self->monsterInfo.aiFlags & AI_DUCKED)) {
			// on Easy & Normal, don't sideStep as often (25% on Easy, 50% on Normal)
			if (!G_SkillCheck({ 0.25f, 0.50f, 1.0f, 1.0f, 1.0f })) {
				GameTime delay = skill->integer > 3 ? random_time(400_ms, 500_ms) : random_time(0.8_sec, 1.4_sec);
				self->monsterInfo.dodge_time = level.time + delay;
				return;
			}
			else {
				if (tr) {
					Vector3 right, diff;

					AngleVectors(self->s.angles, nullptr, right, nullptr);
					diff = tr->endPos - self->s.origin;

					if (right.dot(diff) < 0)
						self->monsterInfo.lefty = false;
					else
						self->monsterInfo.lefty = true;
				}
				else
					self->monsterInfo.lefty = brandom();

				// call the monster specific code here
				if (self->monsterInfo.sideStep(self)) {
					// if we are currently ducked, unDuck
					if ((ducker) && (self->monsterInfo.aiFlags & AI_DUCKED))
						self->monsterInfo.unDuck(self);

					self->monsterInfo.aiFlags |= AI_DODGING;
					self->monsterInfo.attackState = MonsterAttackState::Sliding;

					GameTime delay = skill->integer > 3 ? random_time(400_ms, 500_ms) : random_time(0.4_sec, 2.0_sec);
					self->monsterInfo.dodge_time = level.time + delay;
				}
				return;
			}
		}
	}

	// [Paril-KEX] we don't need to duck until projectiles are going to hit us very
	// soon.
	if (ducker && tr && eta < 0.5_sec) {
		if (self->monsterInfo.next_duck_time > level.time)
			return;

		monster_done_dodge(self);

		if (self->monsterInfo.duck(self, eta)) {
			// if duck didn't set us yet, do it now
			if (self->monsterInfo.duck_wait_time < level.time)
				self->monsterInfo.duck_wait_time = level.time + eta;

			monster_duck_down(self);

			// on Easy & Normal mode, duck longer
			if (skill->integer == 0)
				self->monsterInfo.duck_wait_time += random_time(500_ms, 1000_ms);
			else if (skill->integer == 1)
				self->monsterInfo.duck_wait_time += random_time(100_ms, 350_ms);
		}

		self->monsterInfo.dodge_time = level.time + random_time(0.2_sec, 0.7_sec);
	}
}

void monster_duck_down(gentity_t* self) {
	self->monsterInfo.aiFlags |= AI_DUCKED;

	self->maxs[2] = self->monsterInfo.base_height - 32;
	self->takeDamage = true;
	self->monsterInfo.next_duck_time = level.time + DUCK_INTERVAL;
	gi.linkEntity(self);
}

void monster_duck_hold(gentity_t* self) {
	if (level.time >= self->monsterInfo.duck_wait_time)
		self->monsterInfo.aiFlags &= ~AI_HOLD_FRAME;
	else
		self->monsterInfo.aiFlags |= AI_HOLD_FRAME;
}

MONSTERINFO_UNDUCK(monster_duck_up) (gentity_t* self) -> void {
	if (!(self->monsterInfo.aiFlags & AI_DUCKED))
		return;

	self->monsterInfo.aiFlags &= ~AI_DUCKED;
	self->maxs[2] = self->monsterInfo.base_height;
	self->takeDamage = true;
	// we finished a duck-up successfully, so cut the time remaining in half
	if (self->monsterInfo.next_duck_time > level.time)
		self->monsterInfo.next_duck_time = level.time + ((self->monsterInfo.next_duck_time - level.time) / 2);
	gi.linkEntity(self);
}

//=========================
//=========================
bool has_valid_enemy(gentity_t* self) {
	if (!self->enemy)
		return false;

	if (!self->enemy->inUse)
		return false;

	if (self->enemy->health < 1)
		return false;

	return true;
}

void TargetTesla(gentity_t* self, gentity_t* tesla) {
	if ((!self) || (!tesla))
		return;

	// PMM - medic bails on healing things
	if (self->monsterInfo.aiFlags & AI_MEDIC) {
		if (self->enemy)
			M_CleanupHealTarget(self->enemy);
		self->monsterInfo.aiFlags &= ~AI_MEDIC;
	}

	// store the player enemy in case we lose track of him.
	if (self->enemy && self->enemy->client)
		self->monsterInfo.last_player_enemy = self->enemy;

	if (self->enemy != tesla) {
		self->oldEnemy = self->enemy;
		self->enemy = tesla;
		if (self->monsterInfo.attack) {
			if (self->health <= 0)
				return;

			self->monsterInfo.attack(self);
		}
		else
			FoundTarget(self);
	}
}

// this returns a randomly selected coop player who is visible to self
// returns nullptr if bad

gentity_t* PickCoopTarget(gentity_t* self) {
	gentity_t** targets;
	int		 num_targets = 0, targetID;

	// if we're not in coop, this is a noop
	if (!CooperativeModeOn())
		return nullptr;

	targets = (gentity_t**)alloca(sizeof(gentity_t*) * game.maxClients);

	for (auto ec : active_clients())
		if (visible(self, ec))
			targets[num_targets++] = ec;

	if (!num_targets)
		return nullptr;

	// get a number from 0 to (num_targets-1)
	targetID = irandom(num_targets);

	return targets[targetID];
}

// only meant to be used in coop
int CountPlayers() {
	// if we're not in coop, this is a noop
	if (!CooperativeModeOn())
		return 1;

	int count = 0;
	for (auto ac : active_clients())
		count++;

	return count;
}

static THINK(BossExplode_think) (gentity_t* self) -> void {
	// owner gone or changed
	if (!self->owner->inUse || self->owner->s.modelIndex != self->style || self->count != self->owner->spawn_count) {
		FreeEntity(self);
		return;
	}

	Vector3 org = self->owner->s.origin + self->owner->mins;

	org.x += frandom() * self->owner->size.x;
	org.y += frandom() * self->owner->size.y;
	org.z += frandom() * self->owner->size.z;

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(!(self->viewHeight % 3) ? TE_EXPLOSION1 : TE_EXPLOSION1_NL);
	gi.WritePosition(org);
	gi.multicast(org, MULTICAST_PVS, false);

	self->viewHeight++;

	self->nextThink = level.time + random_time(50_ms, 200_ms);
}

void BossExplode(gentity_t* self) {
	// no blowy on deady
	if (self->spawnFlags.has(SPAWNFLAG_MONSTER_CORPSE))
		return;

	gentity_t* exploder = Spawn();
	exploder->owner = self;
	exploder->count = self->spawn_count;
	exploder->style = self->s.modelIndex;
	exploder->think = BossExplode_think;
	exploder->nextThink = level.time + random_time(75_ms, 250_ms);
	exploder->viewHeight = 0;
}
