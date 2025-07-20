// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"

void FreeFollower(gentity_t *ent) {
	if (!ent)
		return;

	if (!ent->client->followTarget)
		return;

	ent->client->followTarget = nullptr;
	ent->client->ps.pmove.pm_flags &= ~(PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION);

	ent->client->ps.kick_angles = {};
	ent->client->ps.gunangles = {};
	ent->client->ps.gunoffset = {};
	ent->client->ps.gunIndex = 0;
	ent->client->ps.gunSkin = 0;
	ent->client->ps.gunframe = 0;
	ent->client->ps.gunrate = 0;
	ent->client->ps.screenBlend = {};
	ent->client->ps.damageBlend = {};
	ent->client->ps.rdFlags = RDF_NONE;
}

void FreeClientFollowers(gentity_t *ent) {
	if (!ent)
		return;

	for (auto ec : active_clients()) {
		if (!ec->client->followTarget)
			continue;
		if (ec->client->followTarget == ent)
			FreeFollower(ec);
	}
}

void UpdateChaseCam(gentity_t *ent) {
	vec3_t	o, ownerv, goal;
	gentity_t	*targ = ent->client->followTarget;
	vec3_t	forward, right;
	trace_t	trace;
	vec3_t	oldgoal;
	vec3_t	angles;
	
	// is our follow target gone?
	if (!targ || !targ->inUse || !targ->client || !ClientIsPlaying(targ->client) || targ->client->eliminated) {
		//SetTeam(ent, TEAM_SPECTATOR, false, false, false);
		FreeClientFollowers(targ);
		return;
	}

	ownerv = targ->s.origin;
	oldgoal = ent->s.origin;

	// Q2Eaks eyecam handling
	if (g_eyecam->integer) {
		// mark the chased player as instanced so we can disable their model's visibility
		targ->svFlags |= SVF_INSTANCED;

		// copy everything from ps but pmove, pov, stats, and team
		ent->client->ps.viewAngles = targ->client->ps.viewAngles;
		ent->client->ps.viewoffset = targ->client->ps.viewoffset;
		ent->client->ps.kick_angles = targ->client->ps.kick_angles;
		ent->client->ps.gunangles = targ->client->ps.gunangles;
		ent->client->ps.gunoffset = targ->client->ps.gunoffset;
		ent->client->ps.gunIndex = targ->client->ps.gunIndex;
		ent->client->ps.gunSkin = targ->client->ps.gunSkin;
		ent->client->ps.gunframe = targ->client->ps.gunframe;
		ent->client->ps.gunrate = targ->client->ps.gunrate;
		ent->client->ps.screenBlend = targ->client->ps.screenBlend;
		ent->client->ps.damageBlend = targ->client->ps.damageBlend;
		ent->client->ps.rdFlags = targ->client->ps.rdFlags;

		// do pmove stuff so view looks right, but not pm_flags
		ent->client->ps.pmove.origin = targ->client->ps.pmove.origin;
		ent->client->ps.pmove.velocity = targ->client->ps.pmove.velocity;
		ent->client->ps.pmove.pm_time = targ->client->ps.pmove.pm_time;
		ent->client->ps.pmove.gravity = targ->client->ps.pmove.gravity;
		ent->client->ps.pmove.delta_angles = targ->client->ps.pmove.delta_angles;
		ent->client->ps.pmove.viewHeight = targ->client->ps.pmove.viewHeight;
		
		ent->client->pers.hand = targ->client->pers.hand;
		ent->client->pers.weapon = targ->client->pers.weapon;
		
		//FIXME: color shells not working

		// unadjusted view and origin handling
		angles = targ->client->vAngle;
		AngleVectors(angles, forward, right, nullptr);
		forward.normalize();
		o = ownerv;
		trace = gi.traceline(ownerv, o, targ, MASK_SOLID);
		goal = trace.endpos;
	}
	// vanilla chasecam code
	else {
		targ->svFlags &= ~SVF_INSTANCED;

		ownerv[2] += targ->viewHeight;

		angles = targ->client->vAngle;
		if (angles[PITCH] > 56)
			angles[PITCH] = 56;
		AngleVectors(angles, forward, right, nullptr);
		forward.normalize();
		o = ownerv + (forward * -30);

		if (o[2] < targ->s.origin[2] + 20)
			o[2] = targ->s.origin[2] + 20;

		// jump animation lifts
		if (!targ->groundEntity)
			o[2] += 16;

		trace = gi.traceline(ownerv, o, targ, MASK_SOLID);

		goal = trace.endpos;

		goal += (forward * 2);

		// pad for floors and ceilings
		o = goal;
		o[2] += 6;
		trace = gi.traceline(goal, o, targ, MASK_SOLID);
		if (trace.fraction < 1) {
			goal = trace.endpos;
			goal[2] -= 6;
		}

		o = goal;
		o[2] -= 6;
		trace = gi.traceline(goal, o, targ, MASK_SOLID);
		if (trace.fraction < 1) {
			goal = trace.endpos;
			goal[2] += 6;
		}

		ent->client->ps.gunIndex = 0;
		ent->client->ps.gunSkin = 0;
		ent->s.modelindex = 0;
		ent->s.modelindex2 = 0;
		ent->s.modelindex3 = 0;
	}

	if (targ->deadFlag)
		ent->client->ps.pmove.pm_type = PM_DEAD;
	else
		ent->client->ps.pmove.pm_type = PM_FREEZE;

	ent->s.origin = goal;
	ent->client->ps.pmove.delta_angles = targ->client->vAngle - ent->client->resp.cmdAngles;

	if (targ->deadFlag) {
		ent->client->ps.viewAngles[ROLL] = 40;
		ent->client->ps.viewAngles[PITCH] = -15;
		ent->client->ps.viewAngles[YAW] = targ->client->killer_yaw;
	} else {
		ent->client->ps.viewAngles = targ->client->vAngle;
		ent->client->vAngle = targ->client->vAngle;
		AngleVectors(ent->client->vAngle, ent->client->vForward, nullptr, nullptr);
	}
	
	gentity_t *e = targ ? targ : ent;
	ent->client->ps.stats[STAT_SHOW_STATUSBAR] = !ClientIsPlaying(e->client) || e->client->eliminated ? 0 : 1;

	ent->viewHeight = 0;
	if (g_eyecam->integer != 1)
		ent->client->ps.pmove.pm_flags |= PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION;
	gi.linkentity(ent);
}

/*
==================
SanitizeString

Remove case and control characters
==================
*/
static void SanitizeString(const char *in, char *out) {
	while (*in) {
		if (*in < ' ') {
			in++;
			continue;
		}
		*out = tolower(*in);
		out++;
		in++;
	}

	*out = '\0';
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
static int ClientNumberFromString(gentity_t *to, char *s) {
	gclient_t	*cl;
	uint32_t	idnum;
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];
	
	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9') {
		idnum = atoi(s);
		if ((unsigned)idnum >= (unsigned)game.maxclients) {
			gi.LocClient_Print(to, PRINT_HIGH, "Bad client slot: {}\n\"", idnum);
			return -1;
		}

		cl = &game.clients[idnum];
		if (!cl->pers.connected) {
			gi.LocClient_Print(to, PRINT_HIGH, "gclient_t {} is not active.\n\"", idnum);
			return -1;
		}
		return idnum;
	}

	// check for a name match
	SanitizeString(s, s2);
	for (idnum = 0, cl = game.clients; idnum < game.maxclients; idnum++, cl++) {
		if (!cl->pers.connected)
			continue;
		SanitizeString(cl->sess.netName, n2);
		if (!strcmp(n2, s2)) {
			return idnum;
		}
	}

	gi.LocClient_Print(to, PRINT_HIGH, "User {} is not on the server.\n\"", s);
	return -1;
}

void FollowNext(gentity_t *ent) {
	ptrdiff_t i;
	gentity_t *e;

	if (!ent->client->followTarget)
		return;

	i = ent->client->followTarget - g_entities;
	do {
		i++;
		if (i > game.maxclients)
			i = 1;
		e = g_entities + i;
		if (!e->inUse)
			continue;
		if (ent->client->eliminated && ent->client->sess.team != e->client->sess.team)
			continue;
		if (ClientIsPlaying(e->client) && !e->client->eliminated)
			break;
	} while (e != ent->client->followTarget);

	ent->client->followTarget = e;
	ent->client->followUpdate = true;
}

void FollowPrev(gentity_t *ent) {
	int		 i;
	gentity_t *e;

	if (!ent->client->followTarget)
		return;

	i = ent->client->followTarget - g_entities;
	do {
		i--;
		if (i < 1)
			i = game.maxclients;
		e = g_entities + i;
		if (!e->inUse)
			continue;
		if (ent->client->eliminated && ent->client->sess.team != e->client->sess.team)
			continue;
		if (ClientIsPlaying(e->client) && !e->client->eliminated)
			break;
	} while (e != ent->client->followTarget);

	ent->client->followTarget = e;
	ent->client->followUpdate = true;
}

void GetFollowTarget(gentity_t *ent) {
	for (auto ec : active_clients()) {
		if (ec->inUse && ClientIsPlaying(ec->client) && !ec->client->eliminated) {
			if (ent->client->eliminated && ent->client->sess.team != ec->client->sess.team)
				continue;
			ent->client->followTarget = ec;
			ent->client->followUpdate = true;
			UpdateChaseCam(ent);
			return;
		}
	}
}
