// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

// g_cmds.cpp (Game Commands)
// This file is the central dispatcher for all client-initiated commands that
// are processed on the server. It handles everything from inventory management
// and weapon selection to administrative actions and voting.
//
// Key Responsibilities:
// - Command Dispatcher: The `ClientCommand` function at the end of the file
//   is the main entry point. It finds the requested command in the `client_cmds`
//   array and executes the corresponding function.
// - Player Actions: Implements the logic for core player actions like `use`,
//   `drop`, `kill`, and inventory navigation (`invnext`, `invprev`).
// - Cheat and Debug Commands: Contains the implementation for developer and
//   server administrator commands like `give`, `god`, and `noclip`.
// - Team and Match Management: Handles commands for joining teams (`team`),
//   following other players (`follow`), and managing match state (`timeout`).

#include "g_local.hpp"
#include "monsters/m_player.hpp"
/*freeze*/
#if 0
#include "freeze.h"
#endif
/*freeze*/
#include <sstream>
#include <iomanip>			// for setw

enum cmd_flags_t : uint32_t {
	CF_NONE = 0,
	CF_ALLOW_DEAD = bit_v<0>,
	CF_ALLOW_INT = bit_v<1>,
	CF_ALLOW_SPEC = bit_v<2>,
	CF_MATCH_ONLY = bit_v<3>,
	CF_ADMIN_ONLY = bit_v<4>,
	CF_CHEAT_PROTECT = bit_v<5>,
};

struct cmds_t {
	const		char* name;
	void		(*func)(gentity_t* ent);
	uint32_t	flags;
	bool		floodExempt = false;
};

/*
=========================
FormatUsage
=========================
*/
static std::string FormatUsage(const std::string& command,
	const std::vector<std::string>& required = {},
	const std::vector<std::string>& optional = {},
	const std::string& help = {}) {
	std::ostringstream oss;
	oss << "Usage: " << command;
	for (const auto& arg : required)
		oss << " <" << arg << ">";
	for (const auto& arg : optional)
		oss << " [" << arg << "]";
	if (!help.empty())
		oss << '\n' << help << '\n';
	return oss.str();
}


/*
=========================
Cmd_Print_State
=========================
*/
static void Cmd_Print_State(gentity_t* ent, bool on_state) {
	const char* s = gi.argv(0);
	if (s)
		gi.LocClient_Print(ent, PRINT_HIGH, "{} {}\n", s, on_state ? "ON" : "OFF");
}


/*
=========================
CheatsOk
=========================
*/
static inline bool CheatsOk(gentity_t* ent) {
	if (!deathmatch->integer && !coop->integer)
		return true;
	if (!g_cheats->integer) {
		gi.Client_Print(ent, PRINT_HIGH, "Cheats must be enabled to use this command.\n");
		return false;
	}
	return true;
}


/*
=========================
AliveOk
=========================
*/
static inline bool AliveOk(gentity_t* ent) {
	if (ent->health <= 0 || ent->deadFlag) {
		//gi.LocClient_Print(ent, PRINT_HIGH, "You must be alive to use this command.\n");
		return false;
	}
	return true;
}


/*
=========================
SpectatorOk
=========================
*/
static inline bool SpectatorOk(gentity_t* ent) {
	if (!ClientIsPlaying(ent->client)) {
		//gi.LocClient_Print(ent, PRINT_HIGH, "Spectators cannot use this command.\n");
		return false;
	}
	return true;
}


/*
=========================
AdminOk
=========================
*/
static inline bool AdminOk(gentity_t* ent) {
	if (!g_allowAdmin->integer || !ent->client->sess.admin) {
		gi.Client_Print(ent, PRINT_HIGH, "Only admins can use this command.\n");
		return false;
	}
	return true;
}

//=================================================================================

static void SelectNextItem(gentity_t* ent, item_flags_t itflags, bool menu = true) {
	gclient_t* cl;
	item_id_t  i, index;
	Item* it;
	cl = ent->client;
	if (menu && cl->menu.current) {
		NextMenuItem(ent);
		return;
	}
	// check this here so that menus work
	if (level.intermission.time) {
		return;
	}
	if (menu && cl->follow.target) {
		FollowNext(ent);
		return;
	}
	// scan for the next valid one
	for (i = static_cast<item_id_t>(IT_NULL + 1); i <= IT_TOTAL; i = static_cast<item_id_t>(i + 1)) {
		index = static_cast<item_id_t>((cl->pers.selectedItem + i) % IT_TOTAL);
		if (!cl->pers.inventory[index])
			continue;
		it = &itemList[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;
		cl->pers.selectedItem = index;
		cl->pers.selected_item_time = level.time + SELECTED_ITEM_TIME;
		cl->ps.stats[STAT_SELECTED_ITEM_NAME] = CS_ITEMS + index;
		return;
	}
	cl->pers.selectedItem = IT_NULL;
}


static void Cmd_InvNextP_f(gentity_t* ent) {
	SelectNextItem(ent, IF_TIMED | IF_POWERUP | IF_SPHERE);
}


static void Cmd_InvNextW_f(gentity_t* ent) {
	SelectNextItem(ent, IF_WEAPON);
}


static void Cmd_InvNext_f(gentity_t* ent) {
	SelectNextItem(ent, IF_ANY);
}


static void SelectPrevItem(gentity_t* ent, item_flags_t itflags) {
	gclient_t* cl = ent->client;
	item_id_t  i, index;
	Item* it;
	if (cl->menu.current) {
		PreviousMenuItem(ent);
		return;
	}
	// check this here so that menus work
	if (level.intermission.time) {
		return;
	}
	if (cl->follow.target) {
		FollowPrev(ent);
		return;
	}
	// scan for the previous valid one
	for (i = static_cast<item_id_t>(IT_NULL + 1); i <= IT_TOTAL; i = static_cast<item_id_t>(i + 1)) {
		index = static_cast<item_id_t>((cl->pers.selectedItem + IT_TOTAL - i) % IT_TOTAL);
		if (!cl->pers.inventory[index])
			continue;
		it = &itemList[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;
		cl->pers.selectedItem = index;
		cl->pers.selected_item_time = level.time + SELECTED_ITEM_TIME;
		cl->ps.stats[STAT_SELECTED_ITEM_NAME] = CS_ITEMS + index;
		return;
	}
	cl->pers.selectedItem = IT_NULL;
}


static void Cmd_InvPrevP_f(gentity_t* ent) {
	SelectPrevItem(ent, IF_TIMED | IF_POWERUP | IF_SPHERE);
}


static void Cmd_InvPrevW_f(gentity_t* ent) {
	SelectPrevItem(ent, IF_WEAPON);
}


static void Cmd_InvPrev_f(gentity_t* ent) {
	SelectPrevItem(ent, IF_ANY);
}


void ValidateSelectedItem(gentity_t* ent) {
	gclient_t* cl = ent->client;
	if (cl->pers.inventory[cl->pers.selectedItem])
		return; // valid
	SelectNextItem(ent, IF_ANY, false);
}

//=================================================================================

static void SpawnAndGiveItem(gentity_t* ent, item_id_t id) {
	Item* it = GetItemByIndex(id);
	if (!it)
		return;
	gentity_t* it_ent = Spawn();
	it_ent->className = it->className;
	SpawnItem(it_ent, it);
	if (it_ent->inUse) {
		Touch_Item(it_ent, ent, null_trace, true);
		if (it_ent->inUse)
			FreeEntity(it_ent);
	}
}


/*
==================
Cmd_Give_f
Give items to a client
==================
*/
static void Cmd_Give_f(gentity_t* ent) {
	const char* name = gi.args();
	Item* it;
	size_t		i;
	bool		give_all;
	gentity_t* it_ent;
	if (Q_strcasecmp(name, "all") == 0)
		give_all = true;
	else
		give_all = false;
	if (give_all || Q_strcasecmp(gi.argv(1), "health") == 0) {
		if (gi.argc() == 3)
			ent->health = atoi(gi.argv(2));
		else
			ent->health = ent->maxHealth;
		if (!give_all)
			return;
	}
	if (give_all || Q_strcasecmp(name, "weapons") == 0) {
		for (i = 0; i < IT_TOTAL; i++) {
			it = itemList + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IF_WEAPON))
				continue;
			ent->client->pers.inventory[i] += 1;
		}
		if (!give_all)
			return;
	}
	if (give_all || Q_strcasecmp(name, "ammo") == 0) {
		if (give_all)
			SpawnAndGiveItem(ent, IT_PACK);
		for (i = 0; i < IT_TOTAL; i++) {
			it = itemList + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IF_AMMO))
				continue;
			Add_Ammo(ent, it, AMMO_INFINITE);
		}
		if (!give_all)
			return;
	}
	if (give_all || Q_strcasecmp(name, "armor") == 0) {
		ent->client->pers.inventory[IT_ARMOR_JACKET] = 0;
		ent->client->pers.inventory[IT_ARMOR_COMBAT] = 0;
		ent->client->pers.inventory[IT_ARMOR_BODY] = armor_stats[game.ruleset][Armor::Body].max_count;
		if (!give_all)
			return;
	}
	if (give_all || Q_strcasecmp(name, "keys") == 0) {
		for (i = 0; i < IT_TOTAL; i++) {
			it = itemList + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IF_KEY))
				continue;
			ent->client->pers.inventory[i]++;
		}
		ent->client->pers.powerCubes = 0xFF;
		if (!give_all)
			return;
	}
	if (give_all) {
		SpawnAndGiveItem(ent, IT_POWER_SHIELD);
		if (!give_all)
			return;
	}
	if (give_all) {
		for (i = 0; i < IT_TOTAL; i++) {
			it = itemList + i;
			if (!it->pickup)
				continue;
			if (it->flags & (IF_ARMOR | IF_POWER_ARMOR | IF_WEAPON | IF_AMMO | IF_NOT_GIVEABLE | IF_TECH))
				continue;
			else if (it->pickup == CTF_PickupFlag)
				continue;
			else if ((it->flags & IF_HEALTH) && !it->use)
				continue;
			ent->client->pers.inventory[i] = (it->flags & IF_KEY) ? 8 : 1;
		}
		CheckPowerArmorState(ent);
		ent->client->pers.powerCubes = 0xFF;
		return;
	}
	it = FindItem(name);
	if (!it) {
		name = gi.argv(1);
		it = FindItem(name);
	}
	if (!it)
		it = FindItemByClassname(name);
	if (!it) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_unknown_item");
		return;
	}
	if (it->flags & IF_NOT_GIVEABLE) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_not_giveable");
		return;
	}
	if (!it->pickup) {
		ent->client->pers.inventory[it->id] = 1;
		return;
	}
	it_ent = Spawn();
	it_ent->className = it->className;
	SpawnItem(it_ent, it);
	if (it->flags & IF_AMMO && gi.argc() == 3)
		it_ent->count = atoi(gi.argv(2));
	// since some items don't actually spawn when you say to ..
	if (!it_ent->inUse)
		return;
	Touch_Item(it_ent, ent, null_trace, true);
	if (it_ent->inUse)
		FreeEntity(it_ent);
}


static void Cmd_SetPOI_f(gentity_t* self) {
	level.poi.current = self->s.origin;
	level.poi.valid = true;
}


static void Cmd_CheckPOI_f(gentity_t* self) {
	if (!level.poi.valid)
		return;
	char visible_pvs = gi.inPVS(self->s.origin, level.poi.current, false) ? 'y' : 'n';
	char visible_pvs_portals = gi.inPVS(self->s.origin, level.poi.current, true) ? 'y' : 'n';
	char visible_phs = gi.inPHS(self->s.origin, level.poi.current, false) ? 'y' : 'n';
	char visible_phs_portals = gi.inPHS(self->s.origin, level.poi.current, true) ? 'y' : 'n';
	gi.Com_PrintFmt("pvs {} + portals {}, phs {} + portals {}\n", visible_pvs, visible_pvs_portals, visible_phs, visible_phs_portals);
}


// [Paril-KEX]
static void Cmd_Target_f(gentity_t* ent) {
	ent->target = gi.argv(1);
	UseTargets(ent, ent);
	ent->target = nullptr;
}


/*
==================
Cmd_God_f
Sets client to godmode
argv(0) god
==================
*/
static void Cmd_God_f(gentity_t* ent) {
	ent->flags ^= FL_GODMODE;
	Cmd_Print_State(ent, ent->flags & FL_GODMODE);
}


/*
==================
Cmd_Immortal_f
Sets client to immortal - take damage but never go below 1 hp
argv(0) immortal
==================
*/
static void Cmd_Immortal_f(gentity_t* ent) {
	ent->flags ^= FL_IMMORTAL;
	Cmd_Print_State(ent, ent->flags & FL_IMMORTAL);
}


void ED_ParseField(const char* key, const char* value, gentity_t* ent);
/*
=================
Cmd_Spawn_f
Spawn class name
argv(0) spawn
argv(1) <className>
argv(2+n) "key"...
argv(3+n) "value"...
=================
*/
static void Cmd_Spawn_f(gentity_t* ent) {
	solid_t backup = ent->solid;
	ent->solid = SOLID_NOT;
	gi.linkEntity(ent);
	gentity_t* other = Spawn();
	other->className = gi.argv(1);
	other->s.origin = ent->s.origin + (AngleVectors(ent->s.angles).forward * 24.f);
	other->s.angles[YAW] = ent->s.angles[YAW];
	st = {};
	if (gi.argc() > 3) {
		for (int i = 2; i < gi.argc(); i += 2)
			ED_ParseField(gi.argv(i), gi.argv(i + 1), other);
	}
	ED_CallSpawn(other);
	if (other->inUse) {
		Vector3 forward, end;
		AngleVectors(ent->client->vAngle, forward, nullptr, nullptr);
		end = ent->s.origin;
		end[2] += ent->viewHeight;
		end += (forward * 8192);
		trace_t tr = gi.traceLine(ent->s.origin + Vector3{ 0.f, 0.f, (float)ent->viewHeight }, end, other, MASK_SHOT | CONTENTS_MONSTERCLIP);
		other->s.origin = tr.endPos;
		for (size_t i = 0; i < 3; i++) {
			if (tr.plane.normal[i] > 0)
				other->s.origin[i] -= other->mins[i] * tr.plane.normal[i];
			else
				other->s.origin[i] += other->maxs[i] * -tr.plane.normal[i];
		}
		while (gi.trace(other->s.origin, other->mins, other->maxs, other->s.origin, other,
			MASK_SHOT | CONTENTS_MONSTERCLIP).startSolid) {
			float dx = other->mins[0] - other->maxs[0];
			float dy = other->mins[1] - other->maxs[1];
			other->s.origin += forward * -sqrtf(dx * dx + dy * dy);
			if ((other->s.origin - ent->s.origin).dot(forward) < 0) {
				gi.Client_Print(ent, PRINT_HIGH, "Couldn't find a suitable spawn location.\n");
				FreeEntity(other);
				break;
			}
		}
		if (other->inUse)
			gi.linkEntity(other);
		if ((other->svFlags & SVF_MONSTER) && other->think)
			other->think(other);
	}
	ent->solid = backup;
	gi.linkEntity(ent);
}


/*
=================
Cmd_Teleport_f
argv(0) teleport
argv(1) x
argv(2) y
argv(3) z
argv(4) pitch
argv(5) yaw
argv(6) roll
=================
*/
static void Cmd_Teleport_f(gentity_t* ent) {
	if (gi.argc() < 4 || !Q_strcasecmp(gi.argv(1), "?")) {
		FormatUsage(gi.argv(0), { "x", "y", "z" }, { "pitch", "yaw", "roll" }, "Instantly moves the player to the specified coordinates within the world, optionally with the specified angles");
		return;
	}
	ent->s.origin[X] = (float)atof(gi.argv(1));
	ent->s.origin[Y] = (float)atof(gi.argv(2));
	ent->s.origin[Z] = (float)atof(gi.argv(3));
	if (gi.argc() >= 4) {
		float pitch = (float)atof(gi.argv(4));
		float yaw = (float)atof(gi.argv(5));
		float roll = (float)atof(gi.argv(6));
		Vector3 ang{ pitch, yaw, roll };
		ent->client->ps.pmove.deltaAngles = (ang - ent->client->resp.cmdAngles);
		ent->client->ps.viewAngles = {};
		ent->client->vAngle = {};
	}
	gi.linkEntity(ent);
}


/*
==================
TimeoutEnd
==================
*/
void TimeoutEnd() {
	level.timeoutActive = 0_ms;
	level.timeoutOwner = nullptr;
	gi.Broadcast_Print(PRINT_CENTER, "Timeout has ended.\n");
	gi.positionedSound(world->s.origin, world, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundIndex("misc/tele_up.wav"), 1, ATTN_NONE, 0);
	G_LogEvent("MATCH TIMEOUT ENDED");
}


/*
==================
Cmd_TimeIn_f
Ends a timeout session.
==================
*/
static void Cmd_TimeIn_f(gentity_t* ent) {
	if (!level.timeoutActive) {
		gi.Client_Print(ent, PRINT_HIGH, "A timeout is not currently in effect.\n");
		return;
	}
	if (!ent->client->sess.admin && level.timeoutOwner != ent) {
		gi.Client_Print(ent, PRINT_HIGH, "The timeout can only be ended by the timeout caller or an admin.\n");
		return;
	}
	gi.LocBroadcast_Print(PRINT_HIGH, "{} is resuming the match.\n", ent->client->sess.netName);
	level.timeoutActive = 3_sec;
}


/*
==================
Cmd_TimeOut_f
Calls a timeout session.
==================
*/
static void Cmd_TimeOut_f(gentity_t* ent) {
	if (match_timeoutLength->integer <= 0) {
		gi.Client_Print(ent, PRINT_HIGH, "Server has disabled timeouts.\n");
		return;
	}
	if (level.matchState != MatchState::In_Progress) {
		gi.Client_Print(ent, PRINT_HIGH, "Timeouts can only be issued during a match.\n");
		return;
	}
	if (ent->client->pers.timeout_used && !ent->client->sess.admin) {
		gi.Client_Print(ent, PRINT_HIGH, "You have already used your timeout.\n");
		return;
	}
	if (level.timeoutActive > 0_ms) {
		gi.Client_Print(ent, PRINT_HIGH, "A timeout is already in progress.\n");
		return;
	}
	level.timeoutOwner = ent;
	level.timeoutActive = GameTime::from_sec(match_timeoutLength->integer);
	gi.LocBroadcast_Print(PRINT_CENTER, "{} called a timeout!\n{} has been granted.", ent->client->sess.netName, TimeString(match_timeoutLength->integer * 1000, false, false));
	gi.positionedSound(world->s.origin, world, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundIndex("world/klaxon2.wav"), 1, ATTN_NONE, 0);
	ent->client->pers.timeout_used = true;
	G_LogEvent("MATCH TIMEOUT STARTED");
}


/*
==================
Cmd_NoTarget_f
Sets client to notarget
argv(0) notarget
==================
*/
static void Cmd_NoTarget_f(gentity_t* ent) {
	ent->flags ^= FL_NOTARGET;
	Cmd_Print_State(ent, ent->flags & FL_NOTARGET);
}


/*
==================
Cmd_NoVisible_f
Sets client to "super notarget"
argv(0) novisible
==================
*/
static void Cmd_NoVisible_f(gentity_t* ent) {
	ent->flags ^= FL_NOVISIBLE;
	Cmd_Print_State(ent, ent->flags & FL_NOVISIBLE);
}


/*
==================
Cmd_AlertAll_f
argv(0) alertall
==================
*/
static void Cmd_AlertAll_f(gentity_t* ent) {
	for (size_t i = 0; i < globals.numEntities; i++) {
		gentity_t* t = &g_entities[i];
		if (!t->inUse || t->health <= 0 || !(t->svFlags & SVF_MONSTER))
			continue;
		t->enemy = ent;
		FoundTarget(t);
	}
}


/*
==================
Cmd_NoClip_f
argv(0) noclip
==================
*/
static void Cmd_NoClip_f(gentity_t* ent) {
	ent->moveType = ent->moveType == MoveType::NoClip ? MoveType::Walk : MoveType::NoClip;
	Cmd_Print_State(ent, ent->moveType == MoveType::NoClip);
}


/*
==================
Cmd_Use_f
Use an inventory item
==================
*/
static void Cmd_Use_f(gentity_t* ent) {
	item_id_t	index;
	Item* it = nullptr;
	const char* s = gi.args();
	const char* cmd = gi.argv(0);
	if (!Q_strcasecmp(cmd, "use_index") || !Q_strcasecmp(cmd, "use_index_only")) {
		it = GetItemByIndex((item_id_t)atoi(s));
	}
	else {
		if (!Q_strcasecmp(s, "holdable")) {
			if (ent->client->pers.inventory[IT_AMMO_NUKE])
				it = GetItemByIndex(IT_AMMO_NUKE);
			else if (ent->client->pers.inventory[IT_DOPPELGANGER])
				it = GetItemByIndex(IT_DOPPELGANGER);
			else if (ent->client->pers.inventory[IT_TELEPORTER])
				it = GetItemByIndex(IT_TELEPORTER);
			else if (ent->client->pers.inventory[IT_ADRENALINE])
				it = GetItemByIndex(IT_ADRENALINE);
			else if (ent->client->pers.inventory[IT_COMPASS])
				it = GetItemByIndex(IT_COMPASS);
			else return;
		}
		if (!it)
			it = FindItem(s);
	}
	if (!it) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_unknown_item_name", s);
		return;
	}
	if (!it->use) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_item_not_usable");
		return;
	}
	index = it->id;
	if (CombatIsDisabled() && !(it->flags & IF_WEAPON))
		return;
	// Paril: Use_Weapon handles weapon availability
	if (!(it->flags & IF_WEAPON) && !ent->client->pers.inventory[index]) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_out_of_item", it->pickupName);
		return;
	}
	// allow weapon chains for use
	ent->client->noWeaponChains = !!strcmp(gi.argv(0), "use") && !!strcmp(gi.argv(0), "use_index");
	it->use(ent, it);
	ValidateSelectedItem(ent);
}


/*
==================
Cmd_Drop_f
Drop an inventory item
==================
*/
static void Cmd_Drop_f(gentity_t* ent) {
	item_id_t	index;
	Item* it;
	const char* s;
	// don't drop anything when combat is disabled
	if (CombatIsDisabled())
		return;
	s = gi.args();
	const char* cmd = gi.argv(0);
	if (!Q_strcasecmp(cmd, "drop_index")) {
		it = GetItemByIndex((item_id_t)atoi(s));
	}
	else {
		it = FindItem(s);
	}
	if (!it) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Unknown item : {}\n", s);
		return;
	}
	if (!it->drop) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_item_not_droppable");
		return;
	}
	const char* t = nullptr;
	if (it->id == IT_FLAG_RED || it->id == IT_FLAG_BLUE) {
		if (!(match_dropCmdFlags->integer & 1))
			t = "Flag";
	}
	else if (it->flags & IF_POWERUP) {
		if (!(match_dropCmdFlags->integer & 2))
			t = "Powerup";
	}
	else if (it->flags & IF_WEAPON || it->flags & IF_AMMO) {
		if (!(match_dropCmdFlags->integer & 4))
			t = "Weapon and ammo";
		else if (!ItemSpawnsEnabled()) {
			gi.Client_Print(ent, PRINT_HIGH, "Weapon and ammo dropping is not available in this mode.\n");
			return;
		}
	}
	else if (it->flags & IF_WEAPON && deathmatch->integer && match_weaponsStay->integer) {
		gi.Client_Print(ent, PRINT_HIGH, "Weapon dropping is not available during weapons stay mode.\n");
	}
	if (t != nullptr) {
		gi.LocClient_Print(ent, PRINT_HIGH, "{} dropping has been disabled on this server.\n", t);
		return;
	}
	index = it->id;
	if (!ent->client->pers.inventory[index]) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_out_of_item", it->pickupName);
		return;
	}
	if (!Q_strcasecmp(gi.args(), "tech")) {
		it = Tech_Held(ent);
		if (it) {
			it->drop(ent, it);
			ValidateSelectedItem(ent);
		}
		return;
	}
	if (!Q_strcasecmp(gi.args(), "weapon")) {
		it = ent->client->pers.weapon;
		if (it) {
			it->drop(ent, it);
			ValidateSelectedItem(ent);
		}
		return;
	}
	it->drop(ent, it);
	if (Teams() && g_teamplay_item_drop_notice->integer) {
		// add drop notice to all team mates
		//BroadcastTeamMessage(ent->client->sess.team, PRINT_CHAT, G_Fmt("[TEAM]: {} drops {}\n", ent->client->sess.netName, it->useName).data());
		uint32_t key = GetUnicastKey();
		for (auto ec : active_clients()) {
			if (ent == ec)
				continue;
			if (ClientIsPlaying(ec->client) && !OnSameTeam(ent, ec))
				continue;
			if (!ClientIsPlaying(ec->client) && !ec->client->follow.target)
				continue;
			if (!ClientIsPlaying(ec->client) && ec->client->follow.target && !OnSameTeam(ent, ec->client->follow.target))
				continue;
			if (!ClientIsPlaying(ec->client) && ec->client->follow.target && ent == ec->client->follow.target)
				continue;
			gi.WriteByte(svc_poi);
			gi.WriteShort(POI_PING + (ent->s.number - 1));
			gi.WriteShort(5000);
			gi.WritePosition(ent->s.origin);
			gi.WriteShort(gi.imageIndex(it->icon));
			gi.WriteByte(215);
			gi.WriteByte(POI_FLAG_NONE);
			gi.unicast(ec, false);
			gi.localSound(ec, CHAN_AUTO, gi.soundIndex("misc/help_marker.wav"), 1.0f, ATTN_NONE, 0.0f, key);
			gi.LocClient_Print(ec, PRINT_TTS, G_Fmt("[TEAM]: {} drops {}\n", ent->client->sess.netName, it->useName).data(), ent->client->sess.netName);
		}
	}
	ValidateSelectedItem(ent);
}


/*
=================
Cmd_Inven_f
=================
*/
void MapSelectorBegin();
void OpenJoinMenu(gentity_t* ent);
static void Cmd_Inven_f(gentity_t* ent) {
	size_t		i;
	gclient_t* cl;
	cl = ent->client;
	cl->showScores = false;
	cl->showHelp = false;
	globals.serverFlags &= ~SERVER_FLAG_SLOW_TIME;
	if (deathmatch->integer && ent->client->menu.current) {
		if (Vote_Menu_Active(ent))
			return;
		//gi.Client_Print(ent, PRINT_HIGH, "ARGH!\n");
		CloseActiveMenu(ent);
		ent->client->follow.update = true;
		if (!ent->client->initialMenu.closure) {
			gi.LocClient_Print(ent, PRINT_CENTER, "%bind:inven:Toggles Menu%{}", " ");
			ent->client->initialMenu.closure = true;
		}
		return;
	}
	if (cl->showInventory) {
		cl->showInventory = false;
		return;
	}
	if (deathmatch->integer) {
		if (Vote_Menu_Active(ent))
			return;
		//G_Menu_Join_Open(ent);
		OpenJoinMenu(ent);
		return;
	}
	globals.serverFlags |= SERVER_FLAG_SLOW_TIME;
	cl->showInventory = true;
	gi.WriteByte(svc_inventory);
	for (i = 0; i < IT_TOTAL; i++)
		gi.WriteShort(cl->pers.inventory[i]);
	for (; i < MAX_ITEMS; i++)
		gi.WriteShort(0);
	gi.unicast(ent, true);
}


/*
=================
Cmd_InvUse_f
=================
*/
static void Cmd_InvUse_f(gentity_t* ent) {
	Item* it;
	if (deathmatch->integer && ent->client->menu.current) {
		ActivateSelectedMenuItem(ent);
		return;
	}
	// check this here so that menus work
	if (level.intermission.time) {
		return;
	}
	if (!ClientIsPlaying(ent->client))
		return;
	if (ent->health <= 0 || ent->deadFlag)
		return;
	ValidateSelectedItem(ent);
	if (ent->client->pers.selectedItem == IT_NULL) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_no_item_to_use");
		return;
	}
	it = &itemList[ent->client->pers.selectedItem];
	if (!it->use) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_item_not_usable");
		return;
	}
	// don't allow weapon chains for invuse
	ent->client->noWeaponChains = true;
	it->use(ent, it);
	ValidateSelectedItem(ent);
}


/*
=================
Cmd_WeapPrev_f
=================
*/
static void Cmd_WeapPrev_f(gentity_t* ent) {
	gclient_t* cl = ent->client;
	item_id_t	i, index;
	Item* it;
	item_id_t	selected_weapon;
	if (!cl->pers.weapon)
		return;
	// don't allow weapon chains for weapprev
	cl->noWeaponChains = true;
	selected_weapon = cl->pers.weapon->id;
	// scan  for the next valid one
	for (i = static_cast<item_id_t>(IT_NULL + 1); i <= IT_TOTAL; i = static_cast<item_id_t>(i + 1)) {
		// PMM - prevent scrolling through ALL weapons
		index = static_cast<item_id_t>((selected_weapon + IT_TOTAL - i) % IT_TOTAL);
		if (!cl->pers.inventory[index])
			continue;
		it = &itemList[index];
		if (!it->use)
			continue;
		if (!(it->flags & IF_WEAPON))
			continue;
		it->use(ent, it);
		if (cl->weapon.pending == it)
			return; // successful
	}
}


/*
=================
Cmd_WeapNext_f
=================
*/
static void Cmd_WeapNext_f(gentity_t* ent) {
	gclient_t* cl = ent->client;
	item_id_t	i, index;
	Item* it;
	item_id_t	selected_weapon;
	if (!cl->pers.weapon)
		return;
	// don't allow weapon chains for weapnext
	cl->noWeaponChains = true;
	selected_weapon = cl->pers.weapon->id;
	// scan  for the next valid one
	for (i = static_cast<item_id_t>(IT_NULL + 1); i <= IT_TOTAL; i = static_cast<item_id_t>(i + 1)) {
		// PMM - prevent scrolling through ALL weapons
		index = static_cast<item_id_t>((selected_weapon + i) % IT_TOTAL);
		if (!cl->pers.inventory[index])
			continue;
		it = &itemList[index];
		if (!it->use)
			continue;
		if (!(it->flags & IF_WEAPON))
			continue;
		it->use(ent, it);
		// PMM - prevent scrolling through ALL weapons
		if (cl->weapon.pending == it)
			return;
	}
}


/*
=================
Cmd_WeapLast_f
=================
*/
static void Cmd_WeapLast_f(gentity_t* ent) {
	gclient_t* cl = ent->client;
	int			index;
	Item* it;
	if (!cl->pers.weapon || !cl->pers.lastWeapon)
		return;
	// don't allow weapon chains for weaplast
	cl->noWeaponChains = true;
	index = cl->pers.lastWeapon->id;
	if (!cl->pers.inventory[index])
		return;
	it = &itemList[index];
	if (!it->use)
		return;
	if (!(it->flags & IF_WEAPON))
		return;
	it->use(ent, it);
}


/*
=================
Cmd_InvDrop_f
=================
*/
static void Cmd_InvDrop_f(gentity_t* ent) {
	Item* it;
	ValidateSelectedItem(ent);
	if (ent->client->pers.selectedItem == IT_NULL) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_no_item_to_drop");
		return;
	}
	it = &itemList[ent->client->pers.selectedItem];
	if (!it->drop) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_item_not_droppable");
		return;
	}
	it->drop(ent, it);
	ValidateSelectedItem(ent);
}


/*
=================
Cmd_Forfeit_f
=================
*/
static void Cmd_Forfeit_f(gentity_t* ent) {
	if (!Game::Has(GameFlags::OneVOne)) {
		gi.Client_Print(ent, PRINT_HIGH, "Forfeit is only available during Duel or Gauntlet.\n");
		return;
	}
	if (level.matchState < MatchState::In_Progress) {
		gi.Client_Print(ent, PRINT_HIGH, "Forfeit is not available during warmup.\n");
		return;
	}
	if (ent->client != &game.clients[level.sortedClients[1]]) {
		gi.Client_Print(ent, PRINT_HIGH, "Forfeit is only available to the losing player.\n");
		return;
	}
	if (!g_allowForfeit->integer) {
		gi.Client_Print(ent, PRINT_HIGH, "Forfeits are not enabled on this server.\n");
		return;
	}
	QueueIntermission(G_Fmt("{} forfeits the match.", ent->client->sess.netName).data(), true, false);
}


/*
=================
Cmd_Kill_f
=================
*/
static void Cmd_Kill_f(gentity_t* ent) {
	if (deathmatch->integer && (level.time - ent->client->respawnMaxTime) < 5_sec)
		return;
	if (CombatIsDisabled())
		return;
	ent->flags &= ~FL_GODMODE;
	ent->health = 0;
	//  make sure no trackers are still hurting us.
	if (ent->client->trackerPainTime)
		RemoveAttackingPainDaemons(ent);
	if (ent->client->ownedSphere) {
		FreeEntity(ent->client->ownedSphere);
		ent->client->ownedSphere = nullptr;
	}
	// [Paril-KEX] don't allow kill to take points away in TDM
	player_die(ent, ent, ent, 100000, vec3_origin, { ModID::Suicide, Game::Is(GameType::TeamDeathmatch) });
}


/*
=================
Cmd_Kill_AI_f
=================
*/
static void Cmd_Kill_AI_f(gentity_t* ent) {
	// except the one we're looking at...
	gentity_t* looked_at = nullptr;
	Vector3 start = ent->s.origin + Vector3{ 0.f, 0.f, (float)ent->viewHeight };
	Vector3 end = start + ent->client->vForward * 1024.f;
	looked_at = gi.traceLine(start, end, ent, MASK_SHOT).ent;
	const int numEntities = globals.numEntities;
	for (int entnum = 1; entnum < numEntities; ++entnum) {
		gentity_t* entity = &g_entities[entnum];
		if (!entity->inUse || entity == looked_at) {
			continue;
		}
		if ((entity->svFlags & SVF_MONSTER) == 0) {
			continue;
		}
		FreeEntity(entity);
	}
	gi.LocClient_Print(ent, PRINT_HIGH, "{}: All AI Are Dead...\n", __FUNCTION__);
}


/*
=================
Cmd_Where_f
=================
*/
static void Cmd_Where_f(gentity_t* ent) {
	if (ent == nullptr || ent->client == nullptr)
		return;
	const Vector3& origin = ent->s.origin;
	std::string location;
	fmt::format_to(std::back_inserter(location), FMT_STRING("{:.1f} {:.1f} {:.1f} {:.1f} {:.1f} {:.1f}\n"), origin[X], origin[Y], origin[Z], ent->client->ps.viewAngles[PITCH], ent->client->ps.viewAngles[YAW], ent->client->ps.viewAngles[ROLL]);
	gi.LocClient_Print(ent, PRINT_HIGH, "Location: {}\n", location.c_str());
	gi.SendToClipBoard(location.c_str());
}


/*
=================
Cmd_Clear_AI_Enemy_f
=================
*/
static void Cmd_Clear_AI_Enemy_f(gentity_t* ent) {
	for (size_t i = 1; i < globals.numEntities; i++) {
		gentity_t* entity = &g_entities[i];
		if (!entity->inUse)
			continue;
		if ((entity->svFlags & SVF_MONSTER) == 0)
			continue;
		entity->monsterInfo.aiFlags |= AI_FORGET_ENEMY;
	}
	gi.LocClient_Print(ent, PRINT_HIGH, "{}: Clear All AI Enemies...\n", __FUNCTION__);
}


/*
=================
Cmd_PutAway_f
=================
*/
static void Cmd_PutAway_f(gentity_t* ent) {
	ent->client->showScores = false;
	ent->client->showHelp = false;
	ent->client->showInventory = false;
	gentity_t* e = ent->client->follow.target ? ent->client->follow.target : ent;
	ent->client->ps.stats[STAT_SHOW_STATUSBAR] = !ClientIsPlaying(e->client) || e->client->eliminated ? 0 : 1;
	globals.serverFlags &= ~SERVER_FLAG_SLOW_TIME;
	ent->client->follow.update = true;
	if (deathmatch->integer && ent->client->menu.current) {
		if (Vote_Menu_Active(ent))
			return;
		//gi.Client_Print(ent, PRINT_HIGH, "ARGH! 2\n");
		CloseActiveMenu(ent);
	}
}


/*
=================
ClientListSortByScore
=================
*/
static int ClientListSortByScore(const void* a, const void* b) {
	int anum, bnum;
	anum = *(const int*)a;
	bnum = *(const int*)b;
	anum = game.clients[anum].resp.score;
	bnum = game.clients[bnum].resp.score;
	if (anum < bnum)
		return -1;
	if (anum > bnum)
		return 1;
	return 0;
}


/*
=================
ClientListSortByJoinTime
=================
*/
static int ClientListSortByJoinTime(const void* a, const void* b) {
	int anum, bnum;
	anum = *(const int*)a;
	bnum = *(const int*)b;
	anum = game.clients[anum].sess.teamJoinTime.milliseconds();
	bnum = game.clients[bnum].sess.teamJoinTime.milliseconds();
	if (anum > bnum)
		return -1;
	if (anum < bnum)
		return 1;
	return 0;
}
const enum class ClientListSort {
	CLIENTSORT_NONE,
	CLIENTSORT_SCORE,
	CLIENTSORT_TIME,
};
/*
=================
ClientList
=================
*/
static void ClientList(gentity_t* ent, ClientListSort sort) {
	size_t i, count_total = 0, count_bots = 0, skill = 0;
	int index[MAX_CLIENTS] = { 0 };
	std::string row, chunk;
	std::vector<std::string> messageChunks;
	for (auto ec : active_clients()) {
		index[count_total] = ec - g_entities - 1;
		count_total++;
		if (ec->client->sess.skillRating > 0)
			skill += ec->client->sess.skillRating;
		if (ec->client->sess.is_a_bot)
			count_bots++;
	}
	switch (sort) {
	case ClientListSort::CLIENTSORT_SCORE:
		qsort(index, count_total, sizeof(index[0]), ClientListSortByScore);
		break;
	case ClientListSort::CLIENTSORT_TIME:
		qsort(index, count_total, sizeof(index[0]), ClientListSortByJoinTime);
		break;
	}
	constexpr const char* header = "\nclientnum name                             id                                  sr   time ping score team\n";
	constexpr const char* divider = "--------------------------------------------------------------------------------------------------------------------\n";
	std::string fullOutput;
	fullOutput += header;
	fullOutput += divider;
	for (i = 0; i < count_total; i++) {
		gclient_t* cl = &game.clients[index[i]];
		fmt::format_to(std::back_inserter(row), FMT_STRING("{:9} {:32} {:32} {:5} {:3}:{:02} {:4} {:5} {}{}{}\n"),
			index[i],
			cl->sess.netName,
			cl->sess.socialID,
			cl->sess.skillRating,
			(level.time - cl->resp.enterTime).milliseconds() / 60000,
			((level.time - cl->resp.enterTime).milliseconds() % 60000) / 1000,
			cl->ping,
			cl->resp.score,
			cl->sess.matchQueued ? "QUEUE" : Teams_TeamName(cl->sess.team),
			(index[i] == 0) ? " (host)" : cl->sess.admin ? " (admin)" : "",
			cl->sess.inactiveStatus ? " (inactive)" : ""
		);
		if (fullOutput.length() + row.length() > 950) {
			messageChunks.push_back(fullOutput);
			fullOutput = header;
			fullOutput += divider;
		}
		fullOutput += row;
		row.clear();
	}
	if (!fullOutput.empty()) {
		messageChunks.push_back(fullOutput);
	}
	for (const auto& msg : messageChunks) {
		gi.LocClient_Print(ent, PRINT_HIGH | PRINT_NO_NOTIFY, "{}", msg.c_str());
		gi.LocClient_Print(ent, PRINT_HIGH | PRINT_NO_NOTIFY, divider);
	}
	if ((count_total - count_bots) > 0)
		gi.LocClient_Print(ent, PRINT_HIGH | PRINT_NO_NOTIFY, "total human players: {}\n", count_total - count_bots);
	if (count_bots > 0)
		gi.LocClient_Print(ent, PRINT_HIGH | PRINT_NO_NOTIFY, "total bot players: {}\n", count_bots);
	if (skill > 0 && (count_total - count_bots) > 0)
		gi.LocClient_Print(ent, PRINT_HIGH | PRINT_NO_NOTIFY, "average skill rating: {}\n", skill / (count_total - count_bots));
	gi.Client_Print(ent, PRINT_HIGH | PRINT_NO_NOTIFY, "\n");
}


/*
=================
Cmd_ClientList_f
=================
*/
static void Cmd_ClientList_f(gentity_t* ent) {
	ClientListSort sortMode = ClientListSort::CLIENTSORT_NONE;
	if (gi.argc() > 1) {
		std::string arg = gi.argv(1);
		std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower);
		if (arg == "score") {
			sortMode = ClientListSort::CLIENTSORT_SCORE;
		}
		else if (arg == "time") {
			sortMode = ClientListSort::CLIENTSORT_TIME;
		}
	}
	ClientList(ent, sortMode);
}


/*
=================
CheckFlood
=================
*/
bool CheckFlood(gentity_t* ent) {
	if (!flood_msgs->integer)
		return false;
	gclient_t* cl = ent->client;
	if (level.time < cl->flood.lockUntil) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_flood_cant_talk",
			(cl->flood.lockUntil - level.time).seconds<int32_t>());
		return true;
	}
	const size_t maxMsgs = static_cast<size_t>(flood_msgs->integer);
	const size_t bufferSize = std::size(cl->flood.messageTimes);
	size_t i = (static_cast<size_t>(cl->flood.time) + bufferSize - maxMsgs + 1) % bufferSize;
	if (cl->flood.messageTimes[i] && (level.time - cl->flood.messageTimes[i] < GameTime::from_sec(flood_persecond->value))) {
		cl->flood.lockUntil = level.time + GameTime::from_sec(flood_waitdelay->value);
		gi.LocClient_Print(ent, PRINT_CHAT, "$g_flood_cant_talk", flood_waitdelay->integer);
		return true;
	}
	cl->flood.time = static_cast<int>(
		(static_cast<size_t>(cl->flood.time) + 1) % bufferSize
		);
	cl->flood.messageTimes[cl->flood.time] = level.time;
	return false;
}


/*
=================
Cmd_Wave_f
=================
*/
static void Cmd_Wave_f(gentity_t* ent) {
	int i = atoi(gi.argv(1));
	// no dead or noclip waving
	if (ent->deadFlag || ent->moveType == MoveType::NoClip)
		return;
	// can't wave when ducked
	bool do_animate = ent->client->anim.priority <= ANIM_WAVE && !(ent->client->ps.pmove.pmFlags & PMF_DUCKED);
	if (do_animate)
		ent->client->anim.priority = ANIM_WAVE;
	const char* other_notify_msg = nullptr, * other_notify_none_msg = nullptr;
	Vector3 start, dir;
	P_ProjectSource(ent, ent->client->vAngle, { 0, 0, 0 }, start, dir);
	// see who we're aiming at
	gentity_t* aiming_at = nullptr;
	float best_dist = -9999;
	for (auto player : active_clients()) {
		if (player == ent)
			continue;
		Vector3 cdir = player->s.origin - start;
		float dist = cdir.normalize();
		float dot = ent->client->vForward.dot(cdir);
		if (dot < 0.97)
			continue;
		else if (dist < best_dist)
			continue;
		best_dist = dist;
		aiming_at = player;
	}
	switch (i) {
	case GESTURE_FLIP_OFF:
		other_notify_msg = "$g_flipoff_other";
		other_notify_none_msg = "$g_flipoff_none";
		if (do_animate) {
			ent->s.frame = FRAME_flip01 - 1;
			ent->client->anim.end = FRAME_flip12;
		}
		break;
	case GESTURE_SALUTE:
		other_notify_msg = "$g_salute_other";
		other_notify_none_msg = "$g_salute_none";
		if (do_animate) {
			ent->s.frame = FRAME_salute01 - 1;
			ent->client->anim.end = FRAME_salute11;
		}
		break;
	case GESTURE_TAUNT:
		other_notify_msg = "$g_taunt_other";
		other_notify_none_msg = "$g_taunt_none";
		if (do_animate) {
			ent->s.frame = FRAME_taunt01 - 1;
			ent->client->anim.end = FRAME_taunt17;
		}
		break;
	case GESTURE_WAVE:
		other_notify_msg = "$g_wave_other";
		other_notify_none_msg = "$g_wave_none";
		if (do_animate) {
			ent->s.frame = FRAME_wave01 - 1;
			ent->client->anim.end = FRAME_wave11;
		}
		break;
	case GESTURE_POINT:
	default:
		other_notify_msg = "$g_point_other";
		other_notify_none_msg = "$g_point_none";
		if (do_animate) {
			ent->s.frame = FRAME_point01 - 1;
			ent->client->anim.end = FRAME_point12;
		}
		break;
	}
	bool has_a_target = false;
	if (i == GESTURE_POINT) {
		for (auto player : active_clients()) {
			if (player == ent)
				continue;
			else if (!OnSameTeam(ent, player))
				continue;
			has_a_target = true;
			break;
		}
	}
	if (i == GESTURE_POINT && has_a_target) {
		// don't do this stuff if we're flooding
		if (CheckFlood(ent))
			return;
		trace_t tr = gi.traceLine(start, start + (ent->client->vForward * 2048), ent, MASK_SHOT & ~CONTENTS_WINDOW);
		other_notify_msg = "$g_point_other_ping";
		uint32_t key = GetUnicastKey();
		if (tr.fraction != 1.0f) {
			// send to all teammates
			for (auto player : active_clients()) {
				if (player != ent && !OnSameTeam(ent, player))
					continue;
				gi.WriteByte(svc_poi);
				gi.WriteShort(POI_PING + (ent->s.number - 1));
				gi.WriteShort(5000);
				gi.WritePosition(tr.endPos);
				gi.WriteShort(level.picPing);
				gi.WriteByte(208);
				gi.WriteByte(POI_FLAG_NONE);
				gi.unicast(player, false);
				gi.localSound(player, CHAN_AUTO, gi.soundIndex("misc/help_marker.wav"), 1.0f, ATTN_NONE, 0.0f, key);
				gi.LocClient_Print(player, PRINT_HIGH, other_notify_msg, ent->client->sess.netName);
			}
		}
	}
	else {
		if (CheckFlood(ent))
			return;
		gentity_t* targ = nullptr;
		while ((targ = FindRadius(targ, ent->s.origin, 1024)) != nullptr) {
			if (ent == targ) continue;
			if (!targ->client) continue;
			if (!gi.inPVS(ent->s.origin, targ->s.origin, false)) continue;
			if (aiming_at && other_notify_msg)
				gi.LocClient_Print(targ, PRINT_TTS, other_notify_msg, ent->client->sess.netName, aiming_at->client->sess.netName);
			else if (other_notify_none_msg)
				gi.LocClient_Print(targ, PRINT_TTS, other_notify_none_msg, ent->client->sess.netName);
		}
		if (aiming_at && other_notify_msg)
			gi.LocClient_Print(ent, PRINT_TTS, other_notify_msg, ent->client->sess.netName, aiming_at->client->sess.netName);
		else if (other_notify_none_msg)
			gi.LocClient_Print(ent, PRINT_TTS, other_notify_none_msg, ent->client->sess.netName);
	}
	ent->client->anim.time = 0_ms;
}
#ifndef KEX_Q2_GAME
/*
==================
Cmd_Say_f
NB: only used for non-Playfab stuff
==================
*/
static void Cmd_Say_f(gentity_t* ent, bool arg0) {
	gentity_t* other;
	const char* p_in;
	static std::string text;
	if (gi.argc() < 2 && !arg0)
		return;
	else if (CheckFlood(ent))
		return;
	text.clear();
	fmt::format_to(std::back_inserter(text), FMT_STRING("{}: "), ent->client->sess.netName);
	if (arg0) {
		text += gi.argv(0);
		text += " ";
		text += gi.args();
	}
	else {
		p_in = gi.args();
		size_t in_len = strlen(p_in);
		if (p_in[0] == '\"' && p_in[in_len - 1] == '\"')
			text += std::string_view(p_in + 1, in_len - 2);
		else
			text += p_in;
	}
	// don't let text be too long for malicious reasons
	if (text.length() > 150)
		text.resize(150);
	if (text.back() != '\n')
		text.push_back('\n');
	if (g_dedicated->integer)
		gi.Client_Print(nullptr, PRINT_CHAT, text.c_str());
	for (uint32_t j = 1; j <= game.maxClients; j++) {
		other = &g_entities[j];
		if (!other->inUse)
			continue;
		if (!other->client)
			continue;
		gi.Client_Print(other, PRINT_CHAT, text.c_str());
	}
}


/*
=================
Cmd_Say_Team_f
NB: only used for non-Playfab stuff
=================
*/
static void Cmd_Say_Team_f(gentity_t* who, const char* msg_in) {
	gentity_t* cl_ent;
	char outmsg[256];
	if (CheckFlood(who))
		return;
	Q_strlcpy(outmsg, msg_in, sizeof(outmsg));
	char* msg = outmsg;
	if (*msg == '\"') {
		msg[strlen(msg) - 1] = 0;
		msg++;
	}
	for (size_t i = 0; i < game.maxClients; i++) {
		cl_ent = g_entities + 1 + i;
		if (!cl_ent->inUse)
			continue;
		if (cl_ent->client->sess.team == who->client->sess.team)
			gi.LocClient_Print(cl_ent, PRINT_CHAT, "({}): {}\n",
				who->client->sess.netName, msg);
	}
}
#endif
/*
=================
Cmd_ListEntities_f
=================
*/
static void Cmd_ListEntities_f(gentity_t* ent) {
	int count = 0;
	for (size_t i = 1; i < game.maxEntities; i++) {
		gentity_t* e = &g_entities[i];
		if (!e || !e->inUse)
			continue;
		if (gi.argc() > 1) {
			if (!strstr(e->className, gi.argv(1)))
				continue;
		}
		if (gi.argc() > 2) {
			float num = atof(gi.argv(3));
			if (e->s.origin[X] != num)
				continue;
		}
		if (gi.argc() > 3) {
			float num = atof(gi.argv(4));
			if (e->s.origin[Y] != num)
				continue;
		}
		if (gi.argc() > 4) {
			float num = atof(gi.argv(5));
			if (e->s.origin[Z] != num)
				continue;
		}
		gi.Com_PrintFmt("{}: {}", i, *e);
		//#if 0
		if (e->target)
			gi.Com_PrintFmt(", target={}", e->target);
		if (e->targetName)
			gi.Com_PrintFmt(", targetName={}", e->targetName);
		//#endif
		gi.Com_Print("\n");
		count++;
	}
	gi.Com_PrintFmt("\ntotal valid entities={}\n", count);
}


/*
=================
Cmd_ListMonsters_f
=================
*/
static void Cmd_ListMonsters_f(gentity_t* ent) {
	if (!g_debug_monster_kills->integer)
		return;
	for (size_t i = 0; i < level.campaign.totalMonsters; i++) {
		gentity_t* e = level.campaign.monstersRegistered[i];
		if (!e || !e->inUse)
			continue;
		else if (!(e->svFlags & SVF_MONSTER) || (e->monsterInfo.aiFlags & AI_DO_NOT_COUNT))
			continue;
		else if (e->deadFlag)
			continue;
		gi.Com_PrintFmt("{}\n", *e);
	}
}


// =========================================
// TEAMPLAY - MOSTLY PORTED FROM QUAKE III
// =========================================
/*
================
PickTeam
================
*/
Team PickTeam(int ignore_client_num) {
	if (!Teams())
		return Team::Free;
	if (level.pop.num_playing_blue > level.pop.num_playing_red)
		return Team::Red;
	if (level.pop.num_playing_red > level.pop.num_playing_blue)
		return Team::Blue;
	// equal team count, so join the team with the lowest score
	if (level.teamScores[static_cast<int>(Team::Blue)] > level.teamScores[static_cast<int>(Team::Red)])
		return Team::Red;
	if (level.teamScores[static_cast<int>(Team::Red)] > level.teamScores[static_cast<int>(Team::Blue)])
		return Team::Blue;
	// equal team scores, so join team with lowest total individual scores
	// skip in tdm as it's redundant
	if (Game::IsNot(GameType::TeamDeathmatch)) {
		int iscore_red = 0, iscore_blue = 0;
		for (size_t i = 0; i < game.maxClients; i++) {
			if (i == ignore_client_num)
				continue;
			if (!game.clients[i].pers.connected)
				continue;
			if (game.clients[i].sess.team == Team::Red) {
				iscore_red += game.clients[i].resp.score;
				continue;
			}
			if (game.clients[i].sess.team == Team::Blue) {
				iscore_blue += game.clients[i].resp.score;
				continue;
			}
		}
		if (iscore_blue > iscore_red)
			return Team::Red;
		if (iscore_red > iscore_blue)
			return Team::Blue;
	}
	// otherwise just randomly select a team
	return brandom() ? Team::Red : Team::Blue;
}


/*
=================
BroadcastTeamChange
Let everyone know about a team change
=================
*/
void BroadcastTeamChange(gentity_t* ent, Team old_team, bool inactive, bool silent) {
	if (!deathmatch->integer || !ent || !ent->client)
		return;
	if (!Game::Has(GameFlags::OneVOne) && ent->client->sess.team == old_team)
		return;
	if (silent)
		return;
	std::string s, t;
	char name[MAX_INFO_VALUE] = {};
	gi.Info_ValueForKey(ent->client->pers.userInfo, "name", name, sizeof(name));
	const std::string_view playerName = name;
	const uint16_t skill = ent->client->sess.skillRating;
	const auto team = ent->client->sess.team;
	switch (team) {
	case Team::Free:
		s = std::format(".{} joined the battle.\n", playerName);
		if (skill > 0.f) {
			t = std::format(".You have joined the game.\nYour Skill Rating: {}", skill);
		}
		else {
			t = ".You have joined the game.";
		}
		break;
	case Team::Spectator:
		if (inactive) {
			s = std::format(".{} is inactive,\nmoved to spectators.\n", playerName);
			t = "You are inactive and have been\nmoved to spectators.";
		}
		else if (Game::Has(GameFlags::OneVOne) && ent->client->sess.matchQueued) {
			s = std::format(".{} is in the queue to play.\n", playerName);
			t = ".You are in the queue to play.";
		}
		else {
			s = std::format(".{} joined the spectators.\n", playerName);
			t = ".You are now spectating.";
		}
		break;
	case Team::Red:
	case Team::Blue: {
		std::string_view teamName = Teams_TeamName(team);
		s = std::format(".{} joined the {} Team.\n", playerName, teamName);
		if (skill > 0.f) {
			t = std::format(".You have joined the {} Team.\nYour Skill Rating: {}", teamName, skill);
		}
		else {
			t = std::format(".You have joined the {} Team.\n", teamName);
		}
		break;
	}
	default:
		break;
	}
	if (!s.empty()) {
		for (auto ec : active_clients()) {
			if (ec == ent || (ec->svFlags & SVF_BOT))
				continue;
			gi.LocClient_Print(ec, PRINT_CENTER, s.c_str());
		}
	}
	if (warmup_doReadyUp->integer && level.matchState == MatchState::Warmup_ReadyUp) {
		BroadcastReadyReminderMessage();
	}
	else if (!t.empty()) {
		std::string msg = std::format("%bind:inven:Toggles Menu%{}", t);
		gi.LocClient_Print(ent, PRINT_CENTER, msg.c_str());
	}
}


/*
=================
AllowTeamSwitch
=================
*/
static bool AllowTeamSwitch(gentity_t* ent, Team desired_team) {
	/*
	if (desired_team != ent->client->sess.team && Game::Is(GameType::RedRover) && level.matchState == MatchState::In_Progress) {
		gi.Client_Print(ent, PRINT_HIGH, "You cannot change teams during a Red Rover match.\n");
		return false;
	}
	*/
	if (desired_team != Team::Spectator && maxplayers->integer && level.pop.num_playing_human_clients >= maxplayers->integer) {
		gi.Client_Print(ent, PRINT_HIGH, "Maximum player count has been reached.\n");
		return false; // ignore the request
	}
	if (level.locked[static_cast<int>(desired_team)]) {
		gi.LocBroadcast_Print(PRINT_HIGH, "{} is locked.\n", Teams_TeamName(desired_team));
		return false; // ignore the request
	}
	if (Teams()) {
		if (g_teamplay_force_balance->integer) {
			// We allow a spread of two
			if ((desired_team == Team::Red && (level.pop.num_playing_red - level.pop.num_playing_blue > 1)) ||
				(desired_team == Team::Blue && (level.pop.num_playing_blue - level.pop.num_playing_red > 1))) {
				gi.LocClient_Print(ent, PRINT_HIGH, "{} has too many players.\n", Teams_TeamName(desired_team));
				return false; // ignore the request
			}
			// It's ok, the team we are switching to has less or same number of players
		}
	}
	return true;
}


/*
=================
AllowClientTeamSwitch
=================
*/
static bool AllowClientTeamSwitch(gentity_t* ent) {
	if (!deathmatch->integer)
		return false;
	if (match_forceJoin->integer || !g_teamplay_allow_team_pick->integer) {
		if (!(ent->svFlags & SVF_BOT)) {
			gi.Client_Print(ent, PRINT_HIGH, "Team picks are disabled.");
			return false;
		}
	}
	if (ent->client->resp.teamDelayTime > level.time) {
		gi.Client_Print(ent, PRINT_HIGH, "You may not switch teams more than once per 5 seconds.\n");
		return false;
	}
	return true;
}


/*
================
TeamBalance
Balance the teams without shuffling.
Switch last joined player(s) from stacked team.
================
*/
int TeamBalance(bool force) {
	if (!Teams())
		return 0;
	if (Game::Is(GameType::RedRover))
		return 0;
	int delta = abs(level.pop.num_playing_red - level.pop.num_playing_blue);
	if (delta < 2)
		return level.pop.num_playing_red - level.pop.num_playing_blue;
	Team stack_team = level.pop.num_playing_red > level.pop.num_playing_blue ? Team::Red : Team::Blue;
	size_t	count = 0;
	int		index[MAX_CLIENTS_KEX / 2];
	memset(index, 0, sizeof(index));
	// assemble list of client nums of everyone on stacked team
	for (auto ec : active_clients()) {
		if (ec->client->sess.team != stack_team)
			continue;
		index[count] = ec - g_entities;
		count++;
	}
	// sort client num list by join time
	qsort(index, count, sizeof(index[0]), ClientListSortByJoinTime);
	//run through sort list, switching from stack_team until teams are even
	if (count) {
		size_t	i;
		int switched = 0;
		gclient_t* cl = nullptr;
		for (i = 0; i < count && delta > 1; i++) {
			cl = &game.clients[index[i]];
			if (!cl)
				continue;
			if (!cl->pers.connected)
				continue;
			if (cl->sess.team != stack_team)
				continue;
			cl->sess.team = stack_team == Team::Red ? Team::Blue : Team::Red;
			//TODO: queue this change in round-based games
			ClientRespawn(&g_entities[cl - game.clients + 1]);
			gi.Client_Print(&g_entities[cl - game.clients + 1], PRINT_CENTER, "You have changed teams to rebalance the game.\n");
			delta--;
			switched++;
		}
		if (switched) {
			gi.Broadcast_Print(PRINT_HIGH, "Teams have been balanced.\n");
			return switched;
		}
	}
	return 0;
}


/*
==============================
SortPlayersBySkillRating
Comparator for sorting client indices by skill rating.
==============================
*/
static bool SortPlayersBySkillRating(int a, int b) {
	gclient_t* ca = &game.clients[a];
	gclient_t* cb = &game.clients[b];
	// Connecting clients go last
	if (!ca->pers.connected)
		return false;
	if (!cb->pers.connected)
		return true;
	const bool aPlaying = ClientIsPlaying(ca);
	const bool bPlaying = ClientIsPlaying(cb);
	// Spectator/queued logic
	if (!aPlaying && !bPlaying) {
		const bool aQueued = ca->sess.matchQueued;
		const bool bQueued = cb->sess.matchQueued;
		if (aQueued && bQueued)
			return ca->sess.teamJoinTime < cb->sess.teamJoinTime;
		if (aQueued)
			return true;
		if (bQueued)
			return false;
		return ca->sess.teamJoinTime < cb->sess.teamJoinTime;
	}
	// One playing, one not - playing comes first
	if (!aPlaying)
		return false;
	if (!bPlaying)
		return true;
	// Both playing - higher skill rating first
	return ca->sess.skillRating > cb->sess.skillRating;
}


/*
================
TeamSkillShuffle
Randomly shuffles all players in teamplay, tries to balance the skill
================
*/
bool TeamSkillShuffle() {
	if (!Teams())
		return false;
	int totalSkill = 0;
	int oldRedSkill = 0;
	int oldBlueSkill = 0;
	int numPlayers = 0;
	// Tally up skill ratings
	for (auto ec : active_players()) {
		const int skill = ec->client->sess.skillRating;
		totalSkill += skill;
		numPlayers++;
		switch (ec->client->sess.team) {
		case Team::Red:  oldRedSkill += skill; break;
		case Team::Blue: oldBlueSkill += skill; break;
		default: break;
		}
	}
	if (numPlayers < 2)
		return false;
	//const int averageSkill = totalSkill / numPlayers;
	// Sort by skill
	std::sort(
		level.skillSortedClients.begin(),
		level.skillSortedClients.begin() + level.pop.num_connected_clients,
		SortPlayersBySkillRating
	);
	// Pair and assign teams randomly
	for (size_t i = 0; i + 1 < numPlayers; i += 2) {
		const int index1 = level.skillSortedClients[i];
		const int index2 = level.skillSortedClients[i + 1];
		gclient_t* cl1 = &game.clients[index1];
		gclient_t* cl2 = &game.clients[index2];
		const bool assignRed = brandom();
		cl1->sess.team = assignRed ? Team::Red : Team::Blue;
		cl2->sess.team = assignRed ? Team::Blue : Team::Red;
	}
	Match_Reset();
	// Recalculate team skill totals
	int newRedSkill = 0;
	int newBlueSkill = 0;
	for (auto ec : active_players()) {
		const int skill = ec->client->sess.skillRating;
		switch (ec->client->sess.team) {
		case Team::Red:  newRedSkill += skill; break;
		case Team::Blue: newBlueSkill += skill; break;
		default: break;
		}
	}
	gi.LocBroadcast_Print(PRINT_HIGH,
		"Team shuffle result: RedSkill={}->{} BlueSkill={}->{}\n",
		oldRedSkill, newRedSkill, oldBlueSkill, newBlueSkill
	);
	return true;
}


/*
================
TeamShuffle
Randomly shuffles all players in teamplay
================
*/
bool TeamShuffle() {
	if (!Teams())
		return false;
	/*
	if (level.pop.num_playing_clients < 3)
		return false;
		*/
	bool join_red = brandom();
	gentity_t* ent;
	int32_t index[MAX_CLIENTS_KEX] = { 0 };
	memset(index, -1, sizeof(index));
	// determine max team size based from active players
	int maxteam = ceil(level.pop.num_playing_clients / 2);
	int count_red = 0, count_blue = 0;
	Team setteam = join_red ? Team::Red : Team::Blue;
	// create random array
	for (size_t i = 0; i < MAX_CLIENTS_KEX; i++) {
		if (index[i] >= 0)
			continue;
		int rnd = irandom(0, MAX_CLIENTS_KEX);
		while (index[rnd] >= 0)
			rnd = irandom(0, MAX_CLIENTS_KEX);
		index[i] = rnd;
		index[rnd] = i;
	}
#if 0
	for (size_t i = 0; i < MAX_CLIENTS_KEX; i++) {
		gi.Com_PrintFmt("{}={}\n", i, index[i]);
	}
#endif
	// set teams
	for (size_t i = 1; i <= MAX_CLIENTS_KEX; i++) {
		ent = &g_entities[index[i - 1]];
		if (!ent)
			continue;
		if (!ent->inUse)
			continue;
		if (!ent->client)
			continue;
		if (!ent->client->pers.connected)
			continue;
		if (!ClientIsPlaying(ent->client))
			continue;
		if (count_red >= maxteam || count_red > count_blue)
			setteam = Team::Blue;
		else if (count_blue >= maxteam || count_blue > count_red)
			setteam = Team::Red;
		ent->client->sess.team = setteam;
		if (setteam == Team::Red)
			count_red++;
		else count_blue++;
		join_red ^= true;
		setteam = join_red ? Team::Red : Team::Blue;
	}
	return true;
}


/*
=================
StopFollowing
If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
static void StopFollowing(gentity_t* ent, bool release) {
	gclient_t* client;
	if (ent->svFlags & SVF_BOT || !ent->inUse)
		return;
	client = ent->client;
	client->sess.team = Team::Spectator;
	if (release) {
		client->ps.stats[STAT_HEALTH] = ent->health = 1;
		ent->client->ps.stats[STAT_SHOW_STATUSBAR] = 0;
	}
	//SetClientViewAngle(ent, client->ps.viewAngles);
	//client->ps.pmFlags &= ~PMF_FOLLOW;
	ent->svFlags &= SVF_BOT;
	//client->ps.clientnum = ent - g_entities;
	//-------------
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


/*
=================
SetTeam
=================
*/
bool SetTeam(gentity_t* ent, Team desired_team, bool inactive, bool force, bool silent) {
	Team old_team = ent->client->sess.team;
	bool queue = false;
	if (!force) {
		if (!ClientIsPlaying(ent->client) && desired_team != Team::Spectator) {
			bool revoke = false;
			if (level.matchState >= MatchState::Countdown && match_lock->integer) {
				gi.Client_Print(ent, PRINT_HIGH, "Match is locked whilst in progress, no joining permitted now.\n");
				revoke = true;
			}
			else if (level.pop.num_playing_human_clients >= maxplayers->integer) {
				gi.Client_Print(ent, PRINT_HIGH, "Maximum player load reached.\n");
				revoke = true;
			}
			if (revoke) {
				CloseActiveMenu(ent);
				return false;
			}
		}
		if (desired_team != Team::Spectator && desired_team == ent->client->sess.team) {
			CloseActiveMenu(ent);
			return false;
		}
		if (Game::Has(GameFlags::OneVOne)) {
			if (desired_team != Team::Spectator && level.pop.num_playing_clients >= 2) {
				desired_team = Team::Spectator;
				queue = true;
				CloseActiveMenu(ent);
			}
		}
		if (!AllowTeamSwitch(ent, desired_team))
			return false;
		if (!inactive && ent->client->resp.teamDelayTime > level.time) {
			gi.Client_Print(ent, PRINT_HIGH, "You may not switch teams more than once per 5 seconds.\n");
			CloseActiveMenu(ent);
			return false;
		}
	}
	else {
		if (Game::Has(GameFlags::OneVOne)) {
			if (desired_team == Team::None) {
				desired_team = Team::Spectator;
				queue = true;
			}
		}
	}
	// allow the change...
	if (ent->client->menu.current)
		CloseActiveMenu(ent);
	// start as spectator
	if (ent->moveType == MoveType::NoClip)
		Weapon_Grapple_DoReset(ent->client);
	CTF_DeadDropFlag(ent);
	Tech_DeadDrop(ent);
	FreeFollower(ent);
	ent->svFlags &= ~SVF_NOCLIENT;
	ent->client->resp.score = 0;
	ent->client->sess.team = desired_team;
	ent->client->resp.ctf_state = 0;
	ent->client->sess.inactiveStatus = inactive;
	ent->client->sess.inactivityTime = level.time + 1_min;
	ent->client->sess.teamJoinTime = desired_team == Team::Spectator ? 0_sec : level.time;
	ent->client->sess.playStartRealTime = GetCurrentRealTimeMillis();
	ent->client->resp.teamDelayTime = force || !ent->client->sess.initialised ? level.time : level.time + 5_sec;
	ent->client->sess.matchQueued = queue;
	if (desired_team != Team::Spectator) {
		if (Teams())
			AssignPlayerSkin(ent, ent->client->sess.skinName);
		G_RevertVote(ent->client);
		// free any followers
		FreeClientFollowers(ent);
		if (ent->client->pers.spawned)
			ClientConfig_SaveStats(ent->client, false);
	}
	ent->client->sess.initialised = true;
	// if they are playing gauntlet, count as a loss
	if (Game::Is(GameType::Gauntlet) && old_team == Team::Free)
		ent->client->sess.matchLosses++;
	ClientSpawn(ent);
	G_PostRespawn(ent);
	if (old_team != Team::None && old_team != Team::Spectator && desired_team == Team::Spectator) {
		if (ent->client->sess.initialised)
			P_SaveGhostSlot(ent);
	}
	BroadcastTeamChange(ent, old_team, inactive, silent);
	ent->client->ps.stats[STAT_SHOW_STATUSBAR] = desired_team == Team::Spectator || ent->client->eliminated ? 0 : 1;
	// if anybody has a menu open, update it immediately
	DirtyAllMenus();
	return true;
}


/*
=================
Cmd_Team_f
=================
*/
static void Cmd_Team_f(gentity_t* ent) {
	if (gi.argc() == 1) {
		switch (ent->client->sess.team) {
		case Team::Spectator:
			gi.Client_Print(ent, PRINT_HIGH, "You are spectating.\n");
			break;
		case Team::Free:
			gi.Client_Print(ent, PRINT_HIGH, "You are in the match.\n");
			break;
		case Team::Red:
		case Team::Blue:
			gi.LocClient_Print(ent, PRINT_HIGH, "Your team: {}\n", Teams_TeamName(ent->client->sess.team));
			break;
		default:
			break;
		}
		return;
	}
	const char* s = gi.argv(1);
	Team team = StringToTeamNum(s);
	if (team == Team::None)
		return;
	SetTeam(ent, team, false, false, false);
}


/*
=================
Cmd_CrosshairID_f
=================
*/
static void Cmd_CrosshairID_f(gentity_t* ent) {
	ent->client->sess.pc.show_id ^= true;
	gi.LocClient_Print(ent, PRINT_HIGH, "Player identication display: {}\n", ent->client->sess.pc.show_id ? "ON" : "OFF");
}


/*
=================
Cmd_Timer_f
=================
*/
static void Cmd_Timer_f(gentity_t* ent) {
	ent->client->sess.pc.show_timer ^= true;
	gi.LocClient_Print(ent, PRINT_HIGH, "Match timer display: {}\n", ent->client->sess.pc.show_timer ? "ON" : "OFF");
}


/*
=================
Cmd_EyeCam_f
=================
*/
static void Cmd_EyeCam_f(gentity_t* ent) {
	ent->client->sess.pc.show_fragmessages ^= true;
	gi.LocClient_Print(ent, PRINT_HIGH, "{} eyecam.\n", ent->client->sess.pc.show_fragmessages ? "Activating" : "Disabling");
}


/*
=================
Cmd_FragMessages_f
=================
*/
static void Cmd_FragMessages_f(gentity_t* ent) {
	ent->client->sess.pc.show_fragmessages ^= true;
	gi.LocClient_Print(ent, PRINT_HIGH, "{} frag messages.\n", ent->client->sess.pc.show_fragmessages ? "Activating" : "Disabling");
}


/*
=================
Cmd_KillBeep_f
=================
*/
static void Cmd_KillBeep_f(gentity_t* ent) {
	int num = 0;
	if (gi.argc() > 1) {
		num = atoi(gi.argv(1));
		if (num < 0)
			num = 0;
		else if (num > 4)
			num = 4;
	}
	else {
		num = (ent->client->sess.pc.killbeep_num + 1) % 5;
	}
	const char* sb[5] = { "off", "clang", "beep-boop", "insane", "tang-tang" };
	ent->client->sess.pc.killbeep_num = num;
	gi.LocClient_Print(ent, PRINT_HIGH, "Kill beep changed to: {}\n", sb[num]);
}
static void Cmd_Stats_f(gentity_t* ent) {
	if (!Game::Has(GameFlags::CTF))
		return;
}
static void Cmd_Boot_f(gentity_t* ent) {
	if (gi.argc() < 2 || !Q_strcasecmp(gi.argv(1), "?")) {
		FormatUsage(gi.argv(0), { "client name/number" }, {}, "Removes the specified client from the server. Does not work properly in Kex.");
		return;
	}
	if (*gi.argv(1) < '0' && *gi.argv(1) > '9') {
		gi.Client_Print(ent, PRINT_HIGH, "Specify the client name or number to kick.\n");
		return;
	}
	gentity_t* targ = ClientEntFromString(gi.argv(1));
	if (targ == nullptr) {
		gi.Client_Print(ent, PRINT_HIGH, "Invalid client number.\n");
		return;
	}
	if (targ == host) {
		gi.Client_Print(ent, PRINT_HIGH, "You cannot kick the lobby owner.\n");
		return;
	}
	if (targ->client->sess.admin) {
		gi.Client_Print(ent, PRINT_HIGH, "You cannot kick an admin.\n");
		return;
	}
	gi.AddCommandString(G_Fmt("kick {}\n", targ - g_entities).data());
}


/*----------------------------------------------------------------*/
/*
=================
Cmd_Follow_f
=================
*/
static void Cmd_Follow_f(gentity_t* ent) {
	if (ClientIsPlaying(ent->client)) {
		gi.Client_Print(ent, PRINT_HIGH, "You must spectate before you can follow.\n");
		return;
	}
	if (gi.argc() < 2 || !Q_strcasecmp(gi.argv(1), "?")) {
		FormatUsage(gi.argv(0), { "client name/number" }, {}, "Follows the specified player.");
		return;
	}
	gentity_t* follow_ent = ClientEntFromString(gi.argv(1));
	if (!follow_ent || !follow_ent->inUse) {
		gi.Client_Print(ent, PRINT_HIGH, "Invalid client specified.\n");
		return;
	}
	if (ClientIsPlaying(follow_ent->client)) {
		gi.Client_Print(ent, PRINT_HIGH, "Specified client is not playing.\n");
		return;
	}
	ent->client->follow.target = follow_ent;
	ent->client->follow.update = true;
	ClientUpdateFollowers(ent);
}


/*
=================
Cmd_FollowKiller_f
=================
*/
static void Cmd_FollowKiller_f(gentity_t* ent) {
	ent->client->sess.pc.follow_killer ^= true;
	gi.LocClient_Print(ent, PRINT_HIGH, "Auto-follow killer: {}\n", ent->client->sess.pc.follow_killer ? "ON" : "OFF");
}


/*
=================
Cmd_FollowLeader_f
=================
*/
static void Cmd_FollowLeader_f(gentity_t* ent) {
	gentity_t* leader = &g_entities[level.sortedClients[0] + 1];
	ent->client->sess.pc.follow_leader ^= true;
	gi.LocClient_Print(ent, PRINT_HIGH, "Auto-follow leader: {}\n", ent->client->sess.pc.follow_leader ? "ON" : "OFF");
	if (!ClientIsPlaying(ent->client) && ent->client->sess.pc.follow_leader && ent->client->follow.target != leader) {
		ent->client->follow.target = leader;
		ent->client->follow.update = true;
		ClientUpdateFollowers(ent);
	}
}


/*
=================
Cmd_FollowPowerup_f
=================
*/
static void Cmd_FollowPowerup_f(gentity_t* ent) {
	ent->client->sess.pc.follow_powerup ^= true;
	gi.LocClient_Print(ent, PRINT_HIGH, "Auto-follow powerup pick-ups: {}\n", ent->client->sess.pc.follow_powerup ? "ON" : "OFF");
}


/*----------------------------------------------------------------*/
/*
=================
Cmd_LockTeam_f
=================
*/
static void Cmd_LockTeam_f(gentity_t* ent) {
	if (gi.argc() < 2 || !Q_strcasecmp(gi.argv(1), "?")) {
		FormatUsage(gi.argv(0), { "team" }, {}, "Locks a team, prevents players from joining.");
		return;
	}
	Team team = StringToTeamNum(gi.argv(1));
	if (team == Team::None || team == Team::Spectator) {
		gi.Client_Print(ent, PRINT_HIGH, "Invalid team.\n");
		return;
	}
	if (level.locked[static_cast<int>(team)]) {
		gi.LocClient_Print(ent, PRINT_HIGH, "{} is already locked.\n", Teams_TeamName(team));
		return;
	}
	gi.LocBroadcast_Print(PRINT_HIGH, "[ADMIN]: {} has been locked.\n", Teams_TeamName(team));
	level.locked[static_cast<int>(team)] = true;
}


/*
=================
Cmd_UnlockTeam_f
=================
*/
static void Cmd_UnlockTeam_f(gentity_t* ent) {
	if (gi.argc() < 2 || !Q_strcasecmp(gi.argv(1), "?")) {
		FormatUsage(gi.argv(0), { "team" }, {}, "Unlocks a locked team, allows players to join the team.");
		return;
	}
	Team team = StringToTeamNum(gi.argv(1));
	if (team == Team::None || team == Team::Spectator) {
		gi.Client_Print(ent, PRINT_HIGH, "Invalid team.\n");
		return;
	}
	if (!level.locked[static_cast<int>(team)]) {
		gi.LocClient_Print(ent, PRINT_HIGH, "{} is already unlocked.\n", Teams_TeamName(team));
		return;
	}
	gi.LocBroadcast_Print(PRINT_HIGH, "[ADMIN]: {} has been unlocked.\n", Teams_TeamName(team));
	level.locked[static_cast<int>(team)] = false;
}


/*
=================
Cmd_SetTeam_f
=================
*/
static void Cmd_SetTeam_f(gentity_t* ent) {
	if (gi.argc() < 2 || !Q_strcasecmp(gi.argv(1), "?")) {
		FormatUsage(gi.argv(0), { "client name/number" }, {}, "Moves the client to the team.");
		return;
	}
	gentity_t* targ = ClientEntFromString(gi.argv(1));
	if (!targ || !targ->inUse || !targ->client) {
		gi.Client_Print(ent, PRINT_HIGH, "Invalid client name or number.\n");
		return;
	}
	if (gi.argc() == 2) {
		gi.LocClient_Print(ent, PRINT_HIGH, "{} is on {} team.\n", targ->client->sess.netName, gi.argv(0));
		return;
	}
	Team team = StringToTeamNum(gi.argv(2));
	if (team == Team::None) {
		gi.Client_Print(ent, PRINT_HIGH, "Invalid team.\n");
		return;
	}
	if (targ->client->sess.team == team) {
		gi.LocClient_Print(ent, PRINT_HIGH, "{} is already on {} team.\n", targ->client->sess.netName, Teams_TeamName(team));
		return;
	}
	if ((Teams() && team == Team::Free) || (!Teams() && team != Team::Spectator && team != Team::Free)) {
		gi.Client_Print(ent, PRINT_HIGH, "Invalid team.\n");
		return;
	}
	gi.LocBroadcast_Print(PRINT_HIGH, "[ADMIN]: Moved {} to {} team.\n", targ->client->sess.netName, Teams_TeamName(team));
	SetTeam(targ, team, false, true, false);
}


/*
=================
Cmd_Shuffle_f
=================
*/
static void Cmd_Shuffle_f(gentity_t* ent) {
	gi.Broadcast_Print(PRINT_HIGH, "[ADMIN]: Forced team shuffle.\n");
	TeamSkillShuffle();
	//Match_Reset();
}


/*
=================
Cmd_ForceArena_f
=================
*/
static void Cmd_ForceArena_f(gentity_t* ent) {
	const char* arg = gi.argv(1);
	if (!level.arenaTotal) {
		gi.Client_Print(ent, PRINT_HIGH, "No arenas present in current map.\n");
		return;
	}
	if (gi.argc() < 2 || !Q_strcasecmp(arg, "?")) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Active arena is: {}\nTotal arenas: {}\n", level.arenaActive, level.arenaTotal);
		return;
	}
	char* end = nullptr;
	int value = strtol(arg, &end, 10);
	if (value == level.arenaActive) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Arena {} is already active.\n", value);
		return;
	}
	if (end == arg || *end != '\0') {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid number: {}\n", arg);
		return;
	}
	if (!CheckArenaValid(value)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid arena number: {}\n", value);
		return;
	}
	if (!ChangeArena(value)) {
		gi.Client_Print(ent, PRINT_HIGH, "Failed to change arena.\n");
		return;
	}
	gi.LocBroadcast_Print(PRINT_HIGH, "[ADMIN]: Forced active arena to {}.\n", level.arenaActive);
}


/*
=================
Cmd_BalanceTeams_f
=================
*/
static void Cmd_BalanceTeams_f(gentity_t* ent) {
	gi.Broadcast_Print(PRINT_HIGH, "[ADMIN]: Forced team balancing.\n");
	TeamBalance(true);
}


/*
=================
Cmd_StartMatch_f
=================
*/
static void Cmd_StartMatch_f(gentity_t* ent) {
	if (level.matchState > MatchState::Warmup_ReadyUp) {
		gi.Client_Print(ent, PRINT_HIGH, "Match has already started.\n");
		return;
	}
	gi.Broadcast_Print(PRINT_HIGH, "[ADMIN]: Forced match start.\n");
	Match_Start();
}


/*
=================
Cmd_EndMatch_f
=================
*/
static void Cmd_EndMatch_f(gentity_t* ent) {
	if (level.matchState < MatchState::In_Progress) {
		gi.Client_Print(ent, PRINT_HIGH, "Match has not yet begun.\n");
		return;
	}
	if (level.intermission.time) {
		gi.Client_Print(ent, PRINT_HIGH, "Match has already ended.\n");
		return;
	}
	QueueIntermission("[ADMIN]: Forced match end.", true, false);
}


/*
=================
Cmd_ResetMatch_f
=================
*/
static void Cmd_ResetMatch_f(gentity_t* ent) {
	if (level.matchState < MatchState::In_Progress) {
		gi.Client_Print(ent, PRINT_HIGH, "Match has not yet begun.\n");
		return;
	}
	if (level.intermission.time) {
		gi.Client_Print(ent, PRINT_HIGH, "Match has already ended.\n");
		return;
	}
	gi.Broadcast_Print(PRINT_HIGH, "[ADMIN]: Forced match reset.\n");
	Match_Reset();
}


/*
=================
Cmd_ForceVote_f
=================
*/
static void Cmd_ForceVote_f(gentity_t* ent) {
	if (!deathmatch->integer)
		return;
	if (!level.vote.time) {
		gi.Client_Print(ent, PRINT_HIGH, "No vote in progress.\n");
		return;
	}
	const char* arg = gi.argv(1);
	if (arg[0] == 'y' || arg[0] == 'Y' || arg[0] == '1') {
		gi.Broadcast_Print(PRINT_HIGH, "[ADMIN]: Passed the vote.\n");
		level.vote.executeTime = level.time + 3_sec;
		level.vote.client = nullptr;
	}
	else {
		gi.Broadcast_Print(PRINT_HIGH, "[ADMIN]: Failed the vote.\n");
		level.vote.time = 0_sec;
		level.vote.client = nullptr;
	}
}


/*
==================
Cmd_CallVote_f
==================
*/
static void Cmd_CallVote_f(gentity_t* ent) {
	if (!ent || !ent->client)
		return;
	if (gi.argc() < 2) {
		// Construct valid command list for usage message
		std::string validVotes;
		for (const auto& cmd : vote_cmds) {
			if (!cmd.name.empty() && !(g_vote_flags->integer & cmd.flag)) {
				validVotes += cmd.name;
				validVotes += ' ';
			}
		}
		gi.LocClient_Print(ent, PRINT_HIGH,
			"Usage: {} <command> <params>\nValid Voting Commands: {}\n",
			gi.argv(0), validVotes.c_str());
		return;
	}
	const std::string voteName = gi.argv(1);
	const std::string arg = (gi.argc() > 2) ? gi.argv(2) : "";
	if (!TryStartVote(ent, voteName, arg, false)) {
		// TryStartVote handles its own error messaging
		return;
	}
}


/*
==================
Cmd_Vote_f
==================
*/
static void Cmd_Vote_f(gentity_t* ent) {
	if (!deathmatch->integer)
		return;
	if (!ClientIsPlaying(ent->client)) {
		gi.Client_Print(ent, PRINT_HIGH, "Not allowed to vote as spectator.\n");
		return;
	}
	if (gi.argc() < 2 || !Q_strcasecmp(gi.argv(1), "?")) {
		FormatUsage(gi.argv(0), { "yes", "no" }, {}, "Casts your vote in current voting session.");
		return;
	}
	if (!level.vote.time) {
		gi.Client_Print(ent, PRINT_HIGH, "No vote in progress.\n");
		return;
	}
	if (ent->client->pers.voted != 0) {
		gi.Client_Print(ent, PRINT_HIGH, "Vote already cast.\n");
		return;
	}
	const char* arg = gi.argv(1);
	if (arg[0] == 'y' || arg[0] == 'Y' || arg[0] == '1') {
		level.vote.countYes++;
		ent->client->pers.voted = 1;
	}
	else {
		level.vote.countNo++;
		ent->client->pers.voted = -1;
	}
	gi.Client_Print(ent, PRINT_HIGH, "Vote cast.\n");
	// a majority will be determined in CheckVote, which will also account
	// for players entering or leaving
}


/*
=================
Cmd_Gametype_f
=================
*/
static void Cmd_Gametype_f(gentity_t* ent) {
	if (!deathmatch->integer)
		return;
	if (gi.argc() < 2 || !Q_strcasecmp(gi.argv(1), "?")) {
		FormatUsage(gi.argv(0),
			{ "gametype name" },
			{},
			G_Fmt("Changes the current gametype. Current gametype is {} ({}).\nValid gametypes: {}\n",
				Game::GetCurrentInfo().long_name.data(),
				g_gametype->integer,
				GametypeOptionList()).data()
		);
		return;
	}
	GameType gt = GametypeStringToIndex(gi.argv(1));
	if (gt == GameType::None) {
		gi.Client_Print(ent, PRINT_HIGH, "Invalid gametype.\n");
		return;
	}
	ChangeGametype(gt);
}


/*
=================
Cmd_Ruleset_f
=================
*/
static void Cmd_Ruleset_f(gentity_t* ent) {
	if (!deathmatch->integer)
		return;
	if (gi.argc() < 2 || !Q_strcasecmp(gi.argv(1), "?")) {
		FormatUsage(gi.argv(0),
			{ "q1/q2/q3a" },
			{},
			G_Fmt("Changes the current ruleset. Current ruleset is {} ({}).\nValid rulesets: <q1|q2|q3a>\n",
				rs_long_name[static_cast<int>(game.ruleset)],
				static_cast<int>(game.ruleset)).data()
		);
		return;
	}
	ruleset_t rs = RS_IndexFromString(gi.argv(1));
	if (rs == RS_NONE) {
		gi.Client_Print(ent, PRINT_HIGH, "Invalid ruleset.\n");
		return;
	}
	gi.cvarForceSet("g_ruleset", G_Fmt("{}", (int)rs).data());
}


/*
===============
Cmd_Score_f
Display the scoreboard.
===============
*/
void Cmd_Score_f(gentity_t* ent) {
	if (level.intermission.time)
		return;
	// If vote menu is open, just update the status bar
	if (Vote_Menu_Active(ent)) {
		ent->client->showInventory = false;
		ent->client->showHelp = false;
		gentity_t* view = ent->client->follow.target ? ent->client->follow.target : ent;
		ent->client->ps.stats[STAT_SHOW_STATUSBAR] = ClientIsPlaying(view->client) ? 1 : 0;
		return;
	}
	ent->client->showInventory = false;
	ent->client->showHelp = false;
	globals.serverFlags &= ~SERVER_FLAG_SLOW_TIME;
	if (ent->client->menu.current)
		CloseActiveMenu(ent);
	// Only valid during deathmatch or coop
	if (!deathmatch->integer && !coop->integer)
		return;
	if (ent->client->showScores) {
		// Hide scoreboard
		ent->client->showScores = false;
		ent->client->follow.update = true;
		gentity_t* view = ent->client->follow.target ? ent->client->follow.target : ent;
		ent->client->ps.stats[STAT_SHOW_STATUSBAR] = ClientIsPlaying(view->client) ? 1 : 0;
		return;
	}
	// Show scoreboard
	ent->client->ps.stats[STAT_SHOW_STATUSBAR] = 0;
	ent->client->showScores = true;
	MultiplayerScoreboard(ent);
}


/*
=================
Cmd_SetMap_f
=================
*/
static void Cmd_SetMap_f(gentity_t* ent) {
	if (gi.argc() < 2 || !Q_strcasecmp(gi.argv(1), "?")) {
		FormatUsage(gi.argv(0),
			{ "mapname" },
			{},
			"Changes to a map within the map pool."
		);
		PrintMapList(ent, false);
		return;
	}
	const char* mapName = gi.argv(1);
	const MapEntry* map = game.mapSystem.GetMapEntry(mapName);
	if (!map) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Map '{}' not found in map pool.\n", mapName);
		return;
	}
	if (map->longName.empty()) {
		gi.LocBroadcast_Print(PRINT_HIGH, "[ADMIN]: Changing map to {}\n", map->filename.c_str());
	}
	else {
		gi.LocBroadcast_Print(PRINT_HIGH, "[ADMIN]: Changing map to {} ({})\n", map->filename.c_str(), sizeof(map->longName));
	}
	level.changeMap = map->filename.c_str();
	ExitLevel();
}


extern void ClearWorldEntities();
static void Cmd_MapRestart_f(gentity_t* ent) {
	gi.Broadcast_Print(PRINT_HIGH, "[ADMIN]: Session reset.\n");
	gi.AddCommandString(G_Fmt("gamemap {}\n", level.mapName).data());
}


static void Cmd_NextMap_f(gentity_t* ent) {
	gi.Broadcast_Print(PRINT_HIGH, "[ADMIN]: Changing to next map.\n");
	Match_End();
}


static void Cmd_Admin_f(gentity_t* ent) {
	if (!g_allowAdmin->integer) {
		gi.Client_Print(ent, PRINT_HIGH, "Administration is disabled\n");
		return;
	}
	if (gi.argc() > 1) {
		if (ent->client->sess.admin) {
			gi.Client_Print(ent, PRINT_HIGH, "You already have administrative rights.\n");
			return;
		}
		if (admin_password->string && *admin_password->string && Q_strcasecmp(admin_password->string, gi.argv(1)) == 0) {
			if (!ent->client->sess.admin) {
				ent->client->sess.admin = true;
				gi.LocBroadcast_Print(PRINT_HIGH, "{} has become an admin.\n", ent->client->sess.netName);
			}
			return;
		}
	}
	// run command if valid...
}

// =======================================================

static bool ReadyConditions(gentity_t* ent, bool desired_status, bool admin_cmd) {
	if (level.matchState == MatchState::Warmup_ReadyUp)
		return true;
	const char* s = nullptr;
	if (admin_cmd) {
		s = "You cannot force ready status until ";
	}
	else {
		s = "You cannot change your ready status until ";
	}
	switch (level.warmupState) {
	case WarmupState::Too_Few_Players:
	{
		int minp = Game::Has(GameFlags::OneVOne) ? 2 : minplayers->integer;
		int req = minp - level.pop.num_playing_clients;
		gi.LocClient_Print(ent, PRINT_HIGH, "{}{} more player{} present.\n", s, req, req > 1 ? "s are" : " is");
		break;
	}
	case WarmupState::Teams_Imbalanced:
		gi.LocClient_Print(ent, PRINT_HIGH, "{}teams are balanced.\n", s);
		break;
	default:
		gi.LocClient_Print(ent, PRINT_HIGH, "You cannot {}ready at this stage of the match.\n", desired_status ? "" : "un");
		break;
	}
	return false;
}


static void Cmd_ReadyAll_f(gentity_t* ent) {
	if (!ReadyConditions(ent, true, true))
		return;
	ReadyAll();
	gi.Broadcast_Print(PRINT_HIGH, "[ADMIN]: Forced all players to ready status\n");
}


static void Cmd_UnReadyAll_f(gentity_t* ent) {
	if (!ReadyConditions(ent, false, true))
		return;
	UnReadyAll();
	gi.Broadcast_Print(PRINT_HIGH, "[ADMIN]: Forced all players to NOT ready status\n");
}


static void BroadcastReadyStatus(gentity_t* ent) {
	gi.LocBroadcast_Print(PRINT_CENTER, "%bind:+wheel2:Use Compass to toggle your ready status.%.MATCH IS IN WARMUP\n{} is {}ready.", ent->client->sess.netName, ent->client->pers.readyStatus ? "" : "NOT ");
}


static void Cmd_Ready_f(gentity_t* ent) {
	if (!ReadyConditions(ent, true, false))
		return;
	if (level.matchState != MatchState::Warmup_ReadyUp) {
		gi.Client_Print(ent, PRINT_HIGH, "You cannot ready at this stage of the match.\n");
		return;
	}
	if (ent->client->pers.readyStatus) {
		gi.Client_Print(ent, PRINT_HIGH, "You have already committed.\n");
		return;
	}
	ent->client->pers.readyStatus = true;
	BroadcastReadyStatus(ent);
}


static void Cmd_NotReady_f(gentity_t* ent) {
	if (!ReadyConditions(ent, false, false))
		return;
	if (!ent->client->pers.readyStatus) {
		gi.Client_Print(ent, PRINT_HIGH, "You haven't committed.\n");
		return;
	}
	ent->client->pers.readyStatus = false;
	BroadcastReadyStatus(ent);
}


void Cmd_ReadyUp_f(gentity_t* ent) {
	if (!ReadyConditions(ent, !ent->client->pers.readyStatus, false))
		return;
	ent->client->pers.readyStatus ^= true;
	BroadcastReadyStatus(ent);
}


static void Cmd_Hook_f(gentity_t* ent) {
	if (!g_allow_grapple->integer || !g_grapple_offhand->integer)
		return;
	Weapon_Hook(ent);
}


static void Cmd_UnHook_f(gentity_t* ent) {
	Weapon_Grapple_DoReset(ent->client);
}


static void Cmd_MapInfo_f(gentity_t* ent) {
	if (!level.mapName.empty())
		gi.LocClient_Print(ent, PRINT_HIGH, "MAP INFO:\nfilename: {}\n", level.mapName.data());
	else return;
	if (!level.longName.empty())
		gi.LocClient_Print(ent, PRINT_HIGH, "longname: {}\n", level.longName.data());
	if (level.author[0])
		gi.LocClient_Print(ent, PRINT_HIGH, "author{}: {}{}{}\n", level.author2[0] ? "s" : "", level.author, level.author2[0] ? ", " : "", level.author2[0] ? level.author2 : "");
}

// ======================================================

/*
================
IsNumeric
================
*/
static bool IsNumeric(const std::string& str) {
	return !str.empty() && std::all_of(str.begin(), str.end(), ::isdigit);
}


/*
================
FindClientBySlot
================
*/
static gentity_t* FindClientBySlot(int slot) {
	if (slot < 1 || slot > MAX_CLIENTS_KEX)
		return nullptr;
	gentity_t* ent = &g_entities[slot - 1];
	if (!ent->inUse || !ent->client)
		return nullptr;
	return ent;
}


/*
================
FindClientByName
================
*/
static gentity_t* FindClientByName(const std::string& name) {
	for (int i = 0; i < MAX_CLIENTS_KEX; ++i) {
		gentity_t* ent = &g_entities[i];
		if (!ent->inUse || !ent->client)
			continue;
		if (Q_strcasecmp(ent->client->pers.netName, name.c_str()) == 0)
			return ent;
	}
	return nullptr;
}


/*
================
ValidateSocialIDFormat
================
*/
static bool ValidateSocialIDFormat(const std::string& id) {
	size_t sep = id.find(':');
	if (sep == std::string::npos || sep == 0 || sep + 1 >= id.length())
		return false;
	std::string prefix = id.substr(0, sep);
	std::string value = id.substr(sep + 1);

	// EOS: 32-char lowercase hex
	if (prefix == "EOS") {
		if (value.length() != 32)
			return false;
		return std::all_of(value.begin(), value.end(), [](char c) {
			return std::isdigit(c) || (c >= 'a' && c <= 'f');
			});
	}

	// Galaxy: 17-20 digit numeric string
	if (prefix == "Galaxy") {
		if (value.length() < 17 || value.length() > 20)
			return false;
		return std::all_of(value.begin(), value.end(), ::isdigit);
	}

	// GDK: 15-17 digit numeric string
	if (prefix == "GDK") {
		if (value.length() < 15 || value.length() > 17)
			return false;
		return std::all_of(value.begin(), value.end(), ::isdigit);
	}

	// NX: 17-20 digit numeric string
	if (prefix == "NX") {
		if (value.length() < 17 || value.length() > 20)
			return false;
		return std::all_of(value.begin(), value.end(), ::isdigit);
	}

	// PSN: any non-empty numeric string
	if (prefix == "PSN") {
		if (value.empty())
			return false;
		return std::all_of(value.begin(), value.end(), ::isdigit);
	}

	// Steamworks: numeric string starting with 7656119
	if (prefix == "Steamworks") {
		if (!value.starts_with("7656119"))
			return false;
		return std::all_of(value.begin(), value.end(), ::isdigit);
	}
	// Unknown prefix
	return false;
}


/*
================
ResolveSocialID
================
*/
static const char* ResolveSocialID(const char* rawArg, gentity_t*& foundClient) {
	std::string arg(rawArg);
	// Check client number
	if (IsNumeric(arg)) {
		int index = std::stoi(arg);
		foundClient = FindClientBySlot(index);
		if (foundClient && foundClient->client)
			return foundClient->client->pers.socialID;
	}

	// Check player name
	foundClient = FindClientByName(arg);
	if (foundClient && foundClient->client)
		return foundClient->client->pers.socialID;

	// Fall back to raw input - treat as social ID
	if (!ValidateSocialIDFormat(arg)) {
		foundClient = nullptr;
		return nullptr;
	}

	foundClient = nullptr;
	return rawArg;
}
static void Cmd_AddAdmins_f(gentity_t* ent) {
	if (gi.argc() != 2) {
		gi.Client_Print(ent, PRINT_HIGH, "Usage: addAdmin <client# | name | social_id>\n");
		return;
	}

	const char* input = gi.argv(1);
	gentity_t* target = nullptr;
	const char* resolvedID = ResolveSocialID(input, target);
	if (!resolvedID || !*resolvedID) {
		gi.Client_Print(ent, PRINT_HIGH, "Invalid or unresolved social ID.\n");
		return;
	}

	if (AppendIDToFile("admin.txt", resolvedID)) {
		LoadAdminList();
		std::string playerName = GetPlayerNameForSocialID(resolvedID);
		if (!playerName.empty()) {
			gi.LocBroadcast_Print(PRINT_CHAT, "{} has been granted admin rights.\n", playerName.c_str());
		}
		gi.LocClient_Print(ent, PRINT_HIGH, "Admin added: {}\n", resolvedID);
	}
	else {
		gi.Client_Print(ent, PRINT_HIGH, "Failed to write to admin.txt\n");
	}
}


/*
================
Cmd_AddBans_f
================
*/
static void Cmd_AddBans_f(gentity_t* ent) {
	if (gi.argc() != 2) {
		gi.Client_Print(ent, PRINT_HIGH, "Usage: addBan <client# | name | social_id>\n");
		return;
	}

	const char* input = gi.argv(1);
	gentity_t* target = nullptr;
	const char* resolvedID = ResolveSocialID(input, target);
	if (!resolvedID || !*resolvedID) {
		gi.Client_Print(ent, PRINT_HIGH, "Invalid or unresolved social ID.\n");
		return;
	}

	// Prevent banning known admins
	if (game.adminIDs.contains(resolvedID)) {
		gi.Client_Print(ent, PRINT_HIGH, "Cannot ban: target is a listed admin.\n");
		return;
	}

	if (host && host->client && Q_strcasecmp(resolvedID, host->client->sess.socialID) == 0) {
		gi.Client_Print(ent, PRINT_HIGH, "Cannot ban the host.\n");
		return;
	}

	if (AppendIDToFile("ban.txt", resolvedID)) {
		LoadBanList();
		gi.LocClient_Print(ent, PRINT_HIGH, "Ban added: {}\n", resolvedID);
	}
	else {
		gi.Client_Print(ent, PRINT_HIGH, "Failed to write to ban.txt\n");
	}
}


static void Cmd_RemoveAdmins_f(gentity_t* ent) {
	if (gi.argc() != 2) {
		gi.Client_Print(ent, PRINT_HIGH, "Usage: removeAdmin <client# | name | social_id>\n");
		return;
	}
	const char* input = gi.argv(1);
	gentity_t* target = nullptr;
	const char* resolvedID = ResolveSocialID(input, target);
	if (!resolvedID || !*resolvedID) {
		gi.Client_Print(ent, PRINT_HIGH, "Invalid or unresolved social ID.\n");
		return;
	}
	if (host && host->client && Q_strcasecmp(resolvedID, host->client->sess.socialID) == 0) {
		gi.Client_Print(ent, PRINT_HIGH, "Cannot remove admin rights from the host.\n");
		return;
	}
	if (RemoveIDFromFile("admin.txt", resolvedID)) {
		LoadAdminList();
		std::string playerName = GetPlayerNameForSocialID(resolvedID);
		if (!playerName.empty()) {
			gi.LocBroadcast_Print(PRINT_CHAT, "{} has lost admin rights.\n", playerName.c_str());
		}
		gi.LocClient_Print(ent, PRINT_HIGH, "Admin removed: {}\n", resolvedID);
	}
	else {
		gi.Client_Print(ent, PRINT_HIGH, "Failed to remove from admin.txt\n");
	}
}


/*
================
Cmd_RemoveBans_f
================
*/
static void Cmd_RemoveBans_f(gentity_t* ent) {
	if (gi.argc() != 2) {
		gi.Client_Print(ent, PRINT_HIGH, "Usage: removeBan <client# | name | social_id>\n");
		return;
	}
	const char* input = gi.argv(1);
	gentity_t* target = nullptr;
	const char* resolvedID = ResolveSocialID(input, target);
	if (!resolvedID || !*resolvedID) {
		gi.Client_Print(ent, PRINT_HIGH, "Invalid or unresolved social ID.\n");
		return;
	}
	if (RemoveIDFromFile("ban.txt", resolvedID)) {
		LoadBanList();
		gi.LocClient_Print(ent, PRINT_HIGH, "Ban removed: {}\n", resolvedID);
	}
	else {
		gi.Client_Print(ent, PRINT_HIGH, "Failed to remove from ban.txt\n");
	}
}


static void Cmd_LoadAdmins_f(gentity_t* ent) {
	LoadAdminList();
}


static void Cmd_LoadBans_f(gentity_t* ent) {
	LoadBanList();
}


static void Cmd_LoadMotd_f(gentity_t* ent) {
	LoadMotd();
}

// ======================================================

static void Cmd_Motd_f(gentity_t* ent) {
	const char* s = nullptr;
	if (game.motd.size())
		s = G_Fmt("Message of the Day:\n{}\n", game.motd.c_str()).data();
	else
		s = "No Message of the Day set.\n";
	gi.LocClient_Print(ent, PRINT_HIGH, "{}", s);
}


static void Cmd_MySkill_f(gentity_t* ent) {
	int totalSkill = 0, numPlayers = 0;
	int averageSkill = 0;
	// count total skill rating
	for (auto ec : active_clients()) {
		if (!ClientIsPlaying(ec->client))
			continue;
		totalSkill += ec->client->sess.skillRating;
		numPlayers++;
	}
	if (totalSkill && numPlayers)
		averageSkill = totalSkill / numPlayers;
	gi.LocClient_Print(ent, PRINT_HIGH, "Your Skill Rating in {}: {} (server avg: {})\n", level.gametype_name.data(), ent->client->sess.skillRating, averageSkill);
}

// ======================================================

static void Cmd_MapPool_f(gentity_t* ent) {
	std::string query = gi.argc() > 1 ? gi.args() : "";
	PrintMapListFiltered(ent, false, query);
	/*
	int count = PrintMapList(ent, false); // Show all maps
	gi.LocClient_Print(ent, PRINT_HIGH, "Total maps in pool: {}\n", count);
	*/
}


static void Cmd_MapCycle_f(gentity_t* ent) {
	std::string query = gi.argc() > 1 ? gi.args() : "";
	PrintMapListFiltered(ent, true, query);
	/*
	int count = PrintMapList(ent, true); // Show only cycleable maps
	gi.LocClient_Print(ent, PRINT_HIGH, "Total cycleable maps: {}\n", count);
	*/
}


static void Cmd_LoadMapPool_f(gentity_t* ent) {
	LoadMapPool(ent);
	LoadMapCycle(ent);
}


static void Cmd_LoadMapCycle_f(gentity_t* ent) {
	LoadMapCycle(ent);
}


static void PrintMyMapUsage(gentity_t* ent) {
	gi.LocClient_Print(ent, PRINT_HIGH,
		"MyMap Usage:\n"
		"  mymap <mapname> [+flag] [-flag] ...\n"
		"  Flags: +pu +pa +ar +am +ht +bfg +fd +sd +ws (prefix with - to disable)\n"
		"  Use 'mymap ?' to view this message, map list, and availability.\n"
		"  Use 'mappool' to list all available maps.\n");
}


static void PrintMyMapQueue(gentity_t* ent) {
	if (game.mapSystem.playQueue.empty())
		return;

	const int maxLine = 120;
	const int maxMsg = 1024;
	std::string line = "mymap queue => ";
	std::string full;

	for (const auto& q : game.mapSystem.playQueue) {
		std::string entry = q.filename + "(";
		uint8_t ef = q.settings.to_ulong();
		if (ef & MAPFLAG_PU)  entry += "+pu ";
		if (ef & MAPFLAG_PA)  entry += "+pa ";
		if (ef & MAPFLAG_AR)  entry += "+ar ";
		if (ef & MAPFLAG_AM)  entry += "+am ";
		if (ef & MAPFLAG_HT)  entry += "-ht ";
		if (ef & MAPFLAG_BFG) entry += "+bfg ";
		if (ef & MAPFLAG_PB) entry += "+pb ";
		if (ef & MAPFLAG_FD)  entry += "-fd ";
		if (ef & MAPFLAG_SD)  entry += "-sd ";
		if (ef & MAPFLAG_WS)  entry += "+ws ";
		if (entry.back() == ' ') entry.pop_back(); // remove trailing space
		entry += ") ";
		if ((int)line.length() + (int)entry.length() >= maxLine) {
			full += line + "\n";
			line = "";
		}
		line += entry;
	}

	full += line;
	// Break into 1024-char message chunks
	for (size_t pos = 0; pos < full.size(); pos += maxMsg) {
		gi.LocClient_Print(ent, PRINT_HIGH, "{}\n", full.substr(pos, maxMsg).c_str());
	}
}


static void PrintUnavailableMaps(gentity_t* ent) {
	int64_t now = GetCurrentRealTimeMillis();
	std::string line = "The following maps are unavailable for (N) minutes:\n";
	std::string full;
	const int maxLine = 120, maxMsg = 1024;
	int count = 0;

	for (const auto& map : game.mapSystem.mapPool) {
		if (map.lastPlayed) {
			int64_t since = now - map.lastPlayed;
			if (since < 1800000) {
				int seconds = (1800000 - since) / 1000;
				std::string entry = map.filename + "(" + std::to_string(seconds) + ") ";
				if ((int)line.length() + (int)entry.length() >= maxLine) {
					full += line + "\n";
					line = "";
				}
				line += entry;
				count++;
			}
		}
	}

	if (count > 0) {
		full += line;
		for (size_t pos = 0; pos < full.size(); pos += maxMsg) {
			gi.LocClient_Print(ent, PRINT_HIGH, "{}", full.substr(pos, maxMsg).c_str());
		}
	}
}


constexpr int MAX_MYMAP_QUEUE = 8;
static void Cmd_MyMap_f(gentity_t* ent) {
	if (!ent || !ent->client || !g_maps_mymap->integer) {
		return;
	}

	if (!g_maps_mymap->integer) {
		gi.Client_Print(ent, PRINT_HIGH, "MyMap functionality is disabled on this server.\n");
		return;
	}

	const char* socialID = ent->client->sess.socialID;
	if (!socialID || !*socialID) {
		gi.Client_Print(ent, PRINT_HIGH, "You must be logged in to use MyMap.\n");
		return;
	}

	int argc = gi.argc();
	if (argc < 2) {
		PrintMyMapUsage(ent);
		PrintMyMapQueue(ent);
		return;
	}

	if (strcmp(gi.argv(1), "?") == 0) {
		PrintMyMapUsage(ent);
		gi.Client_Print(ent, PRINT_HIGH, "\n");
		PrintMapList(ent, false);
		gi.Client_Print(ent, PRINT_HIGH, "\n");
		PrintUnavailableMaps(ent);
		PrintMyMapQueue(ent);
		return;
	}

	std::string mapName = gi.argv(1);
	const MapEntry* map = game.mapSystem.GetMapEntry(mapName);
	if (!map) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Map '{}' not found in map pool.\n", mapName.c_str());
		return;
	}

	if (map->filename.empty()) {
		gi.Client_Print(ent, PRINT_HIGH, "Error: map filename is invalid.\n");
		return;
	}

	if (!Q_strcasecmp(level.mapName.data(), mapName.c_str())) {
		gi.Client_Print(ent, PRINT_HIGH, "Current map cannot be queued.\n");
		return;
	}

	if (game.mapSystem.IsMapInQueue(mapName)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Map '{}' is already in the play queue.\n", mapName.c_str());
		return;
	}

	if (game.mapSystem.IsClientInQueue(socialID)) {
		gi.Client_Print(ent, PRINT_HIGH, "You already have a map queued.\n");
		return;
	}

	int64_t timeSince = GetCurrentRealTimeMillis() - map->lastPlayed;
	if (map->lastPlayed && timeSince < 1800000) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Map '{}' was played recently. Try again in {}.\n", mapName.c_str(), FormatDuration(1800000 - timeSince).c_str());
		return;
	}

	if (game.mapSystem.playQueue.size() >= MAX_MYMAP_QUEUE) {
		gi.Client_Print(ent, PRINT_HIGH, "The play queue is full.\n");
		return;
	}

	uint8_t enableFlags = 0, disableFlags = 0;
	std::vector<std::string> flagArgs;
	for (int i = 2; i < argc; ++i)
		flagArgs.emplace_back(gi.argv(i));

	if (!ParseMyMapFlags(flagArgs, enableFlags, disableFlags)) {
		gi.Client_Print(ent, PRINT_HIGH, "Invalid flag(s). Use 'mymap ?' for compass.\n");
		return;
	}

	QueuedMap queued;
	queued.filename = map->filename;
	if (queued.filename.empty()) {
		gi.Client_Print(ent, PRINT_HIGH, "Cannot queue: map has no filename.\n");
		return;
	}

	queued.socialID = socialID;
	queued.settings = (enableFlags | disableFlags);
	game.mapSystem.playQueue.push_back(queued);
	MyMapRequest req;
	req.mapName = map->filename;
	req.socialID = socialID;
	req.enableFlags = enableFlags;
	req.disableFlags = disableFlags;
	req.queuedTime = level.time;
	game.mapSystem.myMapQueue.push_back(req);
	gi.LocClient_Print(ent, PRINT_HIGH, "Map '{}' added to the queue.\n", map->filename.c_str());
	// After successful queuing
	PrintMyMapQueue(ent);
}


/*
=============
Cmd_SetWeaponPref_f
=============
*/
static void Cmd_SetWeaponPref_f(gentity_t* ent) {
	if (!ent || !ent->client)
		return;
	auto* cl = ent->client;
	cl->sess.weaponPrefs.clear();
	for (int i = 1; i < gi.argc(); ++i) {
		std::string token = gi.argv(i);
		std::transform(token.begin(), token.end(), token.begin(), ::tolower);
		// Validate against known weapons
		if (GetWeaponIndexByAbbrev(token) != Weapon::None) {
			cl->sess.weaponPrefs.push_back(token);
		}
		else {
			gi.LocClient_Print(ent, PRINT_HIGH, "Unknown weapon abbreviation: {}\n", token.c_str());
		}
	}
#if 0
	// Save it to config
	ClientConfig_BulkUpdate(cl->sess.socialID, {
		{"config", {
			{"weaponPrefs", cl->sess.weaponPrefs}
		}}
		});
#endif
	gi.Client_Print(ent, PRINT_HIGH, "Weapon preferences updated.\n");
}

/*
===============
Cmd_Help_f

Toggles the help screen (objectives or stats).
In deathmatch, this shows the scoreboard instead.
===============
*/
static void Cmd_Help_f(gentity_t* ent) {
	// In deathmatch, "help" key shows the scoreboard
	if (deathmatch->integer) {
		Cmd_Score_f(ent);
		return;
	}

	if (level.intermission.time)
		return;

	if (ent->health <= 0)
		return;

	ent->client->showInventory = false;
	ent->client->showScores = false;

	// Toggle off if help hasn't changed
	if (ent->client->showHelp &&
		ent->client->pers.game_help1changed == game.help[0].modificationCount &&
		ent->client->pers.game_help2changed == game.help[1].modificationCount) {
		ent->client->showHelp = false;
		globals.serverFlags &= ~SERVER_FLAG_SLOW_TIME;
		return;
	}

	ent->client->showHelp = true;
	ent->client->pers.helpchanged = 0;
	globals.serverFlags |= SERVER_FLAG_SLOW_TIME;

	DrawHelpComputer(ent);
}

// =========================================

cmds_t client_cmds[] = {
	{"addAdmin",		Cmd_AddAdmins_f,		CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// add a player social ID to admins file
	{"addBan",			Cmd_AddBans_f,			CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// add a player social ID to bans file
	{"admin",			Cmd_Admin_f,			CF_ALLOW_INT | CF_ALLOW_SPEC},						// enables admin rights, requires entering in admin password
	{"arena",			Cmd_ForceArena_f,		CF_ADMIN_ONLY | CF_ALLOW_SPEC},						// admin forcing active arena, resets game
	{"alertall",		Cmd_AlertAll_f,			CF_ALLOW_SPEC | CF_CHEAT_PROTECT},					// set all monsters in level to be alerted to presence of player
	{"balance",			Cmd_BalanceTeams_f,		CF_ADMIN_ONLY | CF_ALLOW_SPEC},						// balance the teams without shuffling
	{"boot",			Cmd_Boot_f,				CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// remove a client from the server, non-functional
	{"callvote",		Cmd_CallVote_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC},						// call a vote
	{"checkpoi",		Cmd_CheckPOI_f,			CF_ALLOW_SPEC | CF_CHEAT_PROTECT},					// details all points of interest in campaigns
	{"clear_ai_enemy",	Cmd_Clear_AI_Enemy_f,	CF_CHEAT_PROTECT},									// opposite of alertall, makes all monsters forget the player
	{"clientlist",		Cmd_ClientList_f,		CF_ALLOW_DEAD | CF_ALLOW_INT | CF_ALLOW_SPEC},		// lists all clients in server, arg1 score or time to sort
	{"cv",				Cmd_CallVote_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC},						// short-hand for callvote
	{"drop",			Cmd_Drop_f,				CF_NONE},											// drop an item from inventory
	{"drop_index",		Cmd_Drop_f,				CF_NONE},											// drop an item from inventory using index
	{"endmatch",		Cmd_EndMatch_f,			CF_ADMIN_ONLY | CF_ALLOW_SPEC},						// force an end of a match
	{"eyecam",			Cmd_EyeCam_f,			CF_ALLOW_SPEC},										// allows client to toggle spectator eyecam
	{"fm",				Cmd_FragMessages_f,		CF_ALLOW_SPEC | CF_ALLOW_DEAD},						// allow client to toggle display of frag messages
	{"follow",			Cmd_Follow_f,			CF_ALLOW_SPEC | CF_ALLOW_DEAD, true},				// follow a player
	{"followkiller",	Cmd_FollowKiller_f,		CF_ALLOW_SPEC | CF_ALLOW_DEAD, true},				// toggles behaviour to auto-follow killers as kills arise
	{"followleader",	Cmd_FollowLeader_f,		CF_ALLOW_SPEC | CF_ALLOW_DEAD, true},				// toggles behaviour to auto-follow score leader as rankings are updated
	{"followpowerup",	Cmd_FollowPowerup_f,	CF_ALLOW_SPEC | CF_ALLOW_DEAD, true},				// toggles behaviour to auto-follow players when they pick up a powerup
	{"forcevote",		Cmd_ForceVote_f,		CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// force a voting result
	{"forfeit",			Cmd_Forfeit_f,			CF_ALLOW_DEAD, true},								// forfeit a match
	{"gametype",		Cmd_Gametype_f,			CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// change the gametype, resets the game
	{"give",			Cmd_Give_f,				CF_CHEAT_PROTECT, true},							// give an item
	{"god",				Cmd_God_f,				CF_CHEAT_PROTECT, true},							// toggles god mode
	{"help",			Cmd_Help_f,				CF_ALLOW_DEAD | CF_ALLOW_SPEC, true},				// shows help (in campaigns) or scoreboard (in deathmatch)
	{"hook",			Cmd_Hook_f,				CF_NONE, true},										// fires off-hand hook (when enabled on server)
	{"id",				Cmd_CrosshairID_f,		CF_ALLOW_SPEC | CF_ALLOW_DEAD},						// allows clients to toggle crosshair ID drawing
	{"immortal",		Cmd_Immortal_f,			CF_CHEAT_PROTECT},									// cheat to take damage down to a minimum of 1 hp
	{"invdrop",			Cmd_InvDrop_f,			CF_NONE},											// drops current inventory item
	{"inven",			Cmd_Inven_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC, true},				// toggles drawing of inventory list (campaigns) or match menu (deathmatch)
	{"invnext",			Cmd_InvNext_f,			CF_ALLOW_SPEC | CF_ALLOW_INT, true},				//spec for menu up/down		// cycles to next inventory item
	{"invnextp",		Cmd_InvNextP_f,			CF_NONE, true},										// cycles to next powerup in inventory
	{"invnextw",		Cmd_InvNextW_f,			CF_NONE, true},										// cycles to next weapon in inventory
	{"invprev",			Cmd_InvPrev_f,			CF_ALLOW_SPEC | CF_ALLOW_INT, true},				//spec for menu up/down		// cycles to previous inventory item
	{"invprevp",		Cmd_InvPrevP_f,			CF_NONE, true},										// cycles to previous powerup in inventory
	{"invprevw",		Cmd_InvPrevW_f,			CF_NONE, true},										// cycles to previous weapon in inventory
	{"invuse",			Cmd_InvUse_f,			CF_ALLOW_SPEC | CF_ALLOW_INT, true},				//spec for menu up/down		// uses current inventory item
	{"kb",				Cmd_KillBeep_f,			CF_ALLOW_SPEC | CF_ALLOW_DEAD},						// allows clients to toggle or specify killbeeps
	{"kill",			Cmd_Kill_f,				CF_NONE},											// player commits suicide
	{"kill_ai",			Cmd_Kill_AI_f,			CF_CHEAT_PROTECT},									// removes all monsters from level
	{"listentities",	Cmd_ListEntities_f,		CF_ALLOW_DEAD | CF_ALLOW_INT | CF_ALLOW_SPEC | CF_CHEAT_PROTECT},	// lists all entities in level
	{"listmonsters",	Cmd_ListMonsters_f,		CF_ALLOW_DEAD | CF_ALLOW_INT | CF_ALLOW_SPEC | CF_CHEAT_PROTECT},	// lists all monsters in level
	{"loadAdmins",		Cmd_LoadAdmins_f,		CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// load/refresh admins file
	{"loadBans",		Cmd_LoadBans_f,			CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// load/refresh bans file
	{"loadmotd",		Cmd_LoadMotd_f,			CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// load/refresh message of the day from file
	{"loadmappool",		Cmd_LoadMapPool_f,		CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC },		// load map pool file
	{"loadmapcycle",	Cmd_LoadMapCycle_f,		CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC },		// load map cycle file
	{"lockteam",		Cmd_LockTeam_f,			CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// locks a team (prevents joining)
	{"map_restart",		Cmd_MapRestart_f,		CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// restarts the current map, does not reset game state
	{"mapinfo",			Cmd_MapInfo_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC},						// prints details about current map
	{"mappool",			Cmd_MapPool_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC},						// prints current map pool
	{"mapcycle",		Cmd_MapCycle_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC},						// prints current map cycle
	{"motd",			Cmd_Motd_f,				CF_ALLOW_SPEC | CF_ALLOW_INT},						// prints message of the day
	{"mymap",			Cmd_MyMap_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC},						// adds a map to the play queue
	{"nextMap",			Cmd_NextMap_f,			CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// changes to the next map in the queue/picks from map list
	{"noclip",			Cmd_NoClip_f,			CF_CHEAT_PROTECT, true},							// toggles no clipping
	{"notarget",		Cmd_NoTarget_f,			CF_CHEAT_PROTECT, true},							// toggles no targetting from monsters
	{"notready",		Cmd_NotReady_f,			CF_ALLOW_DEAD},										// sets "not ready" status during warmup readyup stage
	{"novisible",		Cmd_NoVisible_f,		CF_CHEAT_PROTECT},									// toggles players being "invisible" to monsters
	{"putaway",			Cmd_PutAway_f,			CF_ALLOW_SPEC},	//spec for menu close				// hides weapon
	{"ready",			Cmd_Ready_f,			CF_ALLOW_DEAD},										// sets "ready" status during warmup readyup stage
	{"readyall",		Cmd_ReadyAll_f,			CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// admin forces all players to be ready during warmup readyup stage
	{"readyup",			Cmd_ReadyUp_f,			CF_ALLOW_DEAD},										// toggles ready status during warmup readyup stage
	{"removeAdmin",		Cmd_RemoveAdmins_f,		CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// removes a player social ID to admins file
	{"removeBan",		Cmd_RemoveBans_f,		CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// removes a player social ID to bans file
	{"resetmatch",		Cmd_ResetMatch_f,		CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// admin forces the match back to warmup
	{"ruleset",			Cmd_Ruleset_f,			CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// changes the ruleset
	{"score",			Cmd_Score_f,			CF_ALLOW_DEAD | CF_ALLOW_INT | CF_ALLOW_SPEC, true},// show scores
	{"setpoi",			Cmd_SetPOI_f,			CF_ALLOW_SPEC | CF_CHEAT_PROTECT},					// sets the client's origin as a point of interest
	{"setmap",			Cmd_SetMap_f,			CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// admin: change the map, restricted to maps within the map list
	{"setteam",			Cmd_SetTeam_f,			CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// admin forces a client to a team
	{"setweappref",		Cmd_SetWeaponPref_f,	CF_ALLOW_DEAD | CF_ALLOW_INT | CF_ALLOW_SPEC},		// sets the clients weapon preferences
	{"shuffle",			Cmd_Shuffle_f,			CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// shuffles the teams, uses skill ratings as a reference
	{"spawn",			Cmd_Spawn_f,			CF_ADMIN_ONLY | CF_ALLOW_SPEC},						// spawns an entity
	{"sr",				Cmd_MySkill_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC},						// shows client's skill rating in current server and gametype
	{"startmatch",		Cmd_StartMatch_f,		CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// admin forces the match to start
	{"stats",			Cmd_Stats_f,			CF_ALLOW_INT | CF_ALLOW_SPEC},						// 
	{"target",			Cmd_Target_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC | CF_CHEAT_PROTECT},	// 
	{"team",			Cmd_Team_f,				CF_ALLOW_DEAD | CF_ALLOW_SPEC},						// joins a team
	{"teleport",		Cmd_Teleport_f,			CF_ALLOW_SPEC | CF_CHEAT_PROTECT},					// moves the client to specified coordinates and optionally sets view angles
	{"time-out",		Cmd_TimeOut_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC},						// calls a timeout during a match
	{"time-in",			Cmd_TimeIn_f,			CF_ALLOW_DEAD | CF_ALLOW_SPEC},						// cancels a timeout during a match
	{"timer",			Cmd_Timer_f,			CF_ALLOW_SPEC | CF_ALLOW_DEAD},						// allows client to toggle drawing/hiding of HUD clock
	{"unhook",			Cmd_UnHook_f,			CF_NONE, true},										// releases off-hand hook (when enabled on server)
	{"unlockteam",		Cmd_UnlockTeam_f,		CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// unlocks a locked team (allows joining)
	{"unreadyall",		Cmd_UnReadyAll_f,		CF_ADMIN_ONLY | CF_ALLOW_INT | CF_ALLOW_SPEC},		// admin forces all players to "not ready" status during warmup readyup stage
	{"use",				Cmd_Use_f,				CF_NONE, true},										// use a specified inventory item by name
	{"use_index",		Cmd_Use_f,				CF_NONE, true},										// use a specified inventory item by index
	{"use_index_only",	Cmd_Use_f,				CF_NONE, true},										// use a specified inventory item only by index
	{"use_only",		Cmd_Use_f,				CF_NONE, true},										// use a specified inventory item only by name
	{"vote",			Cmd_Vote_f,				CF_ALLOW_DEAD, false},								// cast a vote for the current voting session
	{"wave",			Cmd_Wave_f,				CF_NONE, false},									// issues a player animation and corresponding broadcasted message
	{"weaplast",		Cmd_WeapLast_f,			CF_NONE, true},										// switches to last used weapon
	{"weapnext",		Cmd_WeapNext_f,			CF_NONE, true},										// switches to next weapon in cycle
	{"weapprev",		Cmd_WeapPrev_f,			CF_NONE, true},										// switches to previous weapon in cycle
	{"where",			Cmd_Where_f,			CF_ALLOW_SPEC},										// prints out client's current world coordinates and viee angles, copies to clipboard
};


/*
===============
FindClientCmdByName
===============
*/
static cmds_t* FindClientCmdByName(const char* name) {
	cmds_t* cc = client_cmds;
	for (size_t i = 0; i < (sizeof(client_cmds) / sizeof(client_cmds[0])); i++, cc++) {
		if (!cc->name)
			continue;
		if (!Q_strcasecmp(cc->name, name))
			return cc;
	}
	return nullptr;
}


/*
=================
HandleDynamicCvarCommand
Allows replace_* and disable_* cvars to be used by server host at all times.
=================
*/
static inline bool HandleDynamicCvarCommand(gentity_t* ent, const char* cmd) {
	if (gi.argc() > 1 && (strstr(cmd, "replace_") || strstr(cmd, "disable_"))) {
		gi.cvarForceSet(cmd, gi.argv(1));
		return true;
	}
	return false;
}


/*
=================
HasCommandPermission
=================
*/
static inline bool HasCommandPermission(gentity_t* ent, const cmds_t* cmd) {
	if ((cmd->flags & CF_ADMIN_ONLY) && !AdminOk(ent)) return false;
	if ((cmd->flags & CF_CHEAT_PROTECT) && !CheatsOk(ent)) return false;
	if (!(cmd->flags & CF_ALLOW_DEAD) && !AliveOk(ent)) return false;
	if (!(cmd->flags & CF_ALLOW_SPEC) && !SpectatorOk(ent)) return false;
	if (!(cmd->flags & CF_ALLOW_INT) && level.intermission.time && !level.mapSelector.voteStartTime) return false;
	return true;
}

constexpr int FLOOD_LIMIT = 6;             // max allowed commands
constexpr int FLOOD_TIME_MS = 4000;        // window in milliseconds
constexpr int FLOOD_SILENCE_MS = 3000;     // lockout if flood exceeded

/*
=================
CmdFloodCheck
=================
*/
static inline bool CmdFloodCheck(gentity_t* ent) {
	if (!ent || !ent->client)
		return false;
	gclient_t* cl = ent->client;
	// If they're still in silence timeout, block them
	if (level.time < cl->sess.command_flood_time)
		return true;
	// Count and compare window
	if ((level.time - cl->sess.command_flood_time).milliseconds() > FLOOD_TIME_MS) {
		cl->sess.command_flood_count = 1;
		cl->sess.command_flood_time = level.time;
	}
	else {
		cl->sess.command_flood_count++;
		if (cl->sess.command_flood_count > FLOOD_LIMIT) {
			cl->sess.command_flood_time = level.time + GameTime::from_ms(FLOOD_SILENCE_MS);
			gi.Client_Print(ent, PRINT_HIGH, "Command flood detected. Please wait a moment before trying again.\n");
			return true;
		}
	}
	return false;
}


/*
=================
ClientCommand
=================
*/
void ClientCommand(gentity_t* ent) {
	cmds_t* cc;
	const char* cmd;
	if (!ent->client)
		return; // not fully in game yet
	cmd = gi.argv(0);
	if (!cmd || !cmd[0])
		return;
	cc = FindClientCmdByName(cmd);
	// command not found, determine if we can fallback to a replace_ or disable_ cvar
	if (!cc) {
		if (!HandleDynamicCvarCommand(ent, cmd)) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Command not found: '{}'\n", cmd);
		}
		return;
	}
#if 0
	// check flood protection
	if (!cc->floodExempt)
		if (CmdFloodCheck(ent))
			return;
#endif
	// check permissions
	if (!HasCommandPermission(ent, cc)) return;
	cc->func(ent);
}
