// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

// g_spectator.cpp (Game Spectator Logic)
// This file contains all the logic related to the spectator mode. It handles
// how a spectator follows other players (the "chase cam"), including both
// third-person and first-person (`eyecam`) views, and manages the logic for
// cycling between different follow targets.
//
// Key Responsibilities:
// - Chase Camera: `ClientUpdateFollowers` is the core function that updates a
//   spectator's position and view angles to follow their target. It includes
//   collision detection to prevent the camera from clipping through walls.
// - Eyecam Mode: Implements the first-person spectator view by directly copying
//   the target's player state (view angles, weapon model, etc.) to the
//   spectator.
// - Target Cycling: `FollowNext` and `FollowPrev` provide the logic for a
//   spectator to cycle through the available players to watch.
// - State Management: Handles freeing and attaching followers when players
//   connect, disconnect, or switch between playing and spectating.

#include "g_local.hpp"

void FreeFollower(gentity_t* ent) {
	if (!ent)
		return;

	if (!ent->client->follow.target)
		return;

	ent->client->follow.target = nullptr;
	ent->client->ps.pmove.pmFlags &= ~(PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION);

	ent->client->ps.kickAngles = {};
	ent->client->ps.gunAngles = {};
	ent->client->ps.gunOffset = {};
	ent->client->ps.gunIndex = 0;
	ent->client->ps.gunSkin = 0;
	ent->client->ps.gunFrame = 0;
	ent->client->ps.gunRate = 0;
	ent->client->ps.screenBlend = {};
	ent->client->ps.damageBlend = {};
	ent->client->ps.rdFlags = RDF_NONE;
}

void FreeClientFollowers(gentity_t* ent) {
	if (!ent)
		return;

	for (auto ec : active_clients()) {
		if (!ec->client->follow.target)
			continue;
		if (ec->client->follow.target == ent)
			FreeFollower(ec);
	}
}

/*
========================
ClientUpdateFollowers
========================
*/
void ClientUpdateFollowers(gentity_t* ent) {
	if (!ent || !ent->client)
		return;

	gentity_t* targ = ent->client->follow.target;

	// Is our follow target invalid or gone?
	if (!targ || !targ->inUse || !targ->client || !ClientIsPlaying(targ->client) || targ->client->eliminated) {
		FreeClientFollowers(targ);
		return;
	}

	Vector3 targetOrigin = targ->s.origin;
	Vector3 cameraPos, forward, right, angles;
	trace_t trace;

	bool eyecam = g_eyecam->integer != 0;

	if (eyecam) {
		// Hide the followed player model
		targ->svFlags |= SVF_INSTANCED;

		// Sync full view state
		auto& ps = ent->client->ps;
		const auto& tps = targ->client->ps;

		ps.viewAngles = tps.viewAngles;
		ps.viewOffset = tps.viewOffset;
		ps.kickAngles = tps.kickAngles;
		ps.gunAngles = tps.gunAngles;
		ps.gunOffset = tps.gunOffset;
		ps.gunIndex = tps.gunIndex;
		ps.gunSkin = tps.gunSkin;
		ps.gunFrame = tps.gunFrame;
		ps.gunRate = tps.gunRate;
		ps.screenBlend = tps.screenBlend;
		ps.damageBlend = tps.damageBlend;
		ps.rdFlags = tps.rdFlags;

		ps.pmove.origin = tps.pmove.origin;
		ps.pmove.velocity = tps.pmove.velocity;
		ps.pmove.pmTime = tps.pmove.pmTime;
		ps.pmove.gravity = tps.pmove.gravity;
		ps.pmove.deltaAngles = tps.pmove.deltaAngles;
		ps.pmove.viewHeight = tps.pmove.viewHeight;

		ent->client->pers.hand = targ->client->pers.hand;
		ent->client->pers.weapon = targ->client->pers.weapon;

		ent->s.origin = targ->s.origin;
		ent->viewHeight = targ->viewHeight;

		ent->client->vAngle = targ->client->vAngle;
		AngleVectors(ent->client->vAngle, ent->client->vForward, nullptr, nullptr);

		// Optional trace (preserve in case used later)
		trace = gi.traceLine(targ->s.origin, targ->s.origin, targ, MASK_SOLID);
		cameraPos = trace.endPos;
	}
	else {
		// Vanilla chasecam
		targ->svFlags &= ~SVF_INSTANCED;

		angles = targ->client->vAngle;
		if (angles[PITCH] > 56)
			angles[PITCH] = 56;

		AngleVectors(angles, forward, right, nullptr);
		forward.normalize();

		Vector3 eyePos = targetOrigin;
		eyePos[2] += targ->viewHeight;

		cameraPos = eyePos + (forward * -30.0f);
		if (cameraPos[2] < targ->s.origin[Z] + 20.0f)
			cameraPos[2] = targ->s.origin[Z] + 20.0f;
		if (!targ->groundEntity)
			cameraPos[2] += 16.0f;

		// Main line-of-sight trace
		trace = gi.traceLine(eyePos, cameraPos, targ, MASK_SOLID);
		cameraPos = trace.endPos + (forward * 2.0f);

		// Ceiling pad
		Vector3 ceilingCheck = cameraPos; ceilingCheck[2] += 6.0f;
		trace = gi.traceLine(cameraPos, ceilingCheck, targ, MASK_SOLID);
		if (trace.fraction < 1.0f) {
			cameraPos = trace.endPos;
			cameraPos[2] -= 6.0f;
		}

		// Floor pad
		Vector3 floorCheck = cameraPos; floorCheck[2] -= 6.0f;
		trace = gi.traceLine(cameraPos, floorCheck, targ, MASK_SOLID);
		if (trace.fraction < 1.0f) {
			cameraPos = trace.endPos;
			cameraPos[2] += 6.0f;
		}

		ent->client->ps.gunIndex = 0;
		ent->client->ps.gunSkin = 0;
		ent->s.modelIndex = 0;
		ent->s.modelIndex2 = 0;
		ent->s.modelIndex3 = 0;

		ent->s.origin = cameraPos;
		ent->viewHeight = 0;

		// Disable prediction to prevent view jitter
		ent->client->ps.pmove.pmFlags |= PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION;
	}

	// Set movement type based on target's state
	ent->client->ps.pmove.pmType = targ->deadFlag ? PM_DEAD : PM_FREEZE;

	// Match view angles and delta
	ent->client->ps.pmove.deltaAngles = targ->client->vAngle - ent->client->resp.cmdAngles;

	if (targ->deadFlag) {
		ent->client->ps.viewAngles[ROLL] = 40;
		ent->client->ps.viewAngles[PITCH] = -15;
		ent->client->ps.viewAngles[YAW] = targ->client->killerYaw;
	}
	else {
		ent->client->ps.viewAngles = targ->client->vAngle;
		ent->client->vAngle = targ->client->vAngle;
		AngleVectors(ent->client->vAngle, ent->client->vForward, nullptr, nullptr);
	}

	// Show HUD if the target is active
	bool showStatus = ClientIsPlaying(targ->client) && !targ->client->eliminated;
	ent->client->ps.stats[STAT_SHOW_STATUSBAR] = showStatus ? 1 : 0;

	gi.linkEntity(ent);
}

/*
==================
SanitizeString

Remove case and control characters
==================
*/
static void SanitizeString(const char* in, char* out) {
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
static int ClientNumberFromString(gentity_t* to, char* s) {
	gclient_t* cl;
	uint32_t	idnum;
	char		s2[MAX_STRING_CHARS];
	char		n2[MAX_STRING_CHARS];

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9') {
		idnum = atoi(s);
		if ((unsigned)idnum >= (unsigned)game.maxClients) {
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
	for (idnum = 0, cl = game.clients; idnum < game.maxClients; idnum++, cl++) {
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

void FollowNext(gentity_t* ent) {
	ptrdiff_t i;
	gentity_t* e;

	if (!ent->client->follow.target)
		return;

	i = ent->client->follow.target - g_entities;
	do {
		i++;
		if (i > game.maxClients)
			i = 1;
		e = g_entities + i;
		if (!e->inUse)
			continue;
		if (ent->client->eliminated && ent->client->sess.team != e->client->sess.team)
			continue;
		if (ClientIsPlaying(e->client) && !e->client->eliminated)
			break;
	} while (e != ent->client->follow.target);

	ent->client->follow.target = e;
	ent->client->follow.update = true;
}

void FollowPrev(gentity_t* ent) {
	int		 i;
	gentity_t* e;

	if (!ent->client->follow.target)
		return;

	i = ent->client->follow.target - g_entities;
	do {
		i--;
		if (i < 1)
			i = game.maxClients;
		e = g_entities + i;
		if (!e->inUse)
			continue;
		if (ent->client->eliminated && ent->client->sess.team != e->client->sess.team)
			continue;
		if (ClientIsPlaying(e->client) && !e->client->eliminated)
			break;
	} while (e != ent->client->follow.target);

	ent->client->follow.target = e;
	ent->client->follow.update = true;
}

void GetFollowTarget(gentity_t* ent) {
	for (auto ec : active_clients()) {
		if (ec->inUse && ClientIsPlaying(ec->client) && !ec->client->eliminated) {
			if (ent->client->eliminated && ent->client->sess.team != ec->client->sess.team)
				continue;
			ent->client->follow.target = ec;
			ent->client->follow.update = true;
			ClientUpdateFollowers(ent);
			return;
		}
	}
}
