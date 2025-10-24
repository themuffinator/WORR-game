// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

// cg_main.cpp (Client Game Main)
// This file serves as the main entry point and API bridge for the client-side
// game module (cgame). It is responsible for initializing and shutting down
// the client-side game logic and exporting the necessary functions to the
// main engine.
//
// Key Responsibilities:
// - Implements `GetCGameAPI`, the function the engine calls to get the table
//   of client-side game functions (`cgame_export_t`).
// - Handles the initialization (`InitCGame`) and shutdown (`ShutdownCGame`) of
//   the client-side module, setting up necessary systems like the HUD.
// - Provides wrapper functions that are exposed to the engine, which in turn
//   call the actual implementation logic located in other `cg_` files (e.g.,
//   `CG_DrawHUD`, `CG_ParseCenterPrint`).
// - Manages client-side state that depends on server configstrings, like
//   physics settings (`pm_config`).

#include "cg_local.hpp"
#include "monsters/m_flash.hpp"

cgame_import_t cgi;
cgame_export_t cglobals;

static void *CG_GetExtension(const char *name) {
	return nullptr;
}

void CG_InitScreen();

uint64_t cgame_init_time = 0;

static void InitCGame() {
	CG_InitScreen();

	cgame_init_time = cgi.CL_ClientRealTime();

	pm_config.n64Physics = !!strtoul(cgi.get_configString(CONFIG_N64_PHYSICS_MEDAL), nullptr, 10);
	pm_config.airAccel = strtoul(cgi.get_configString(CS_AIRACCEL), nullptr, 10);
}

static void ShutdownCGame() {}

void CG_DrawHUD(int32_t isplit, const cg_server_data_t *data, vrect_t hud_vrect, vrect_t hud_safe, int32_t scale, int32_t playernum, const player_state_t *ps);
void CG_TouchPics();
layout_flags_t CG_LayoutFlags(const player_state_t *ps);

static int32_t CG_GetActiveWeaponWheelWeapon(const player_state_t *ps) {
	return ps->stats[STAT_ACTIVE_WHEEL_WEAPON];
}

static uint32_t CG_GetOwnedWeaponWheelWeapons(const player_state_t *ps) {
	return ((uint32_t)(uint16_t)ps->stats[STAT_WEAPONS_OWNED_1]) | ((uint32_t)(uint16_t)(ps->stats[STAT_WEAPONS_OWNED_2]) << 16);
}

static int16_t CG_GetWeaponWheelAmmoCount(const player_state_t *ps, int32_t ammoID) {
	uint16_t ammo = GetAmmoStat((uint16_t *)&ps->stats[STAT_AMMO_INFO_START], ammoID);

	if (ammo == AMMO_VALUE_INFINITE)
		return -1;

	return ammo;
}

static int16_t CG_GetPowerupWheelCount(const player_state_t *ps, int32_t powerup_id) {
	return GetPowerupStat((uint16_t *)&ps->stats[STAT_POWERUP_INFO_START], powerup_id);
}

static int16_t CG_GetHitMarkerDamage(const player_state_t *ps) {
	return ps->stats[STAT_HIT_MARKER];
}

static void CG_ParseConfigString(int32_t i, const char *s) {
	if (i == CONFIG_N64_PHYSICS_MEDAL)
		pm_config.n64Physics = !!strtoul(s, nullptr, 10);
	else if (i == CS_AIRACCEL)
		pm_config.airAccel = strtoul(s, nullptr, 10);
}

void CG_ParseCenterPrint(const char *str, int isplit, bool instant);
void CG_ClearNotify(int32_t isplit);
void CG_ClearCenterprint(int32_t isplit);
void CG_NotifyMessage(int32_t isplit, const char *msg, bool is_chat);

/*
=================
CG_GetMonsterFlashOffset
=================
*/
static void CG_GetMonsterFlashOffset(MonsterMuzzleFlashID id, gvec3_ref_t offset) {
	const auto index = static_cast<unsigned int>(id);
	if (index >= std::size(monster_flash_offset))
		cgi.Com_Error("Bad muzzle flash offset");

	offset = monster_flash_offset[index];
}

/*
=================
GetCGameAPI

Returns a pointer to the structure with all entry points
and global variables
=================
*/
Q2GAME_API cgame_export_t * GetCGameAPI(cgame_import_t * import) {
	cgi = *import;

	cglobals.apiVersion = CGAME_API_VERSION;
	cglobals.Init = InitCGame;
	cglobals.Shutdown = ShutdownCGame;

	cglobals.Pmove = Pmove;
	cglobals.DrawHUD = CG_DrawHUD;
	cglobals.LayoutFlags = CG_LayoutFlags;
	cglobals.TouchPics = CG_TouchPics;

	cglobals.GetActiveWeaponWheelWeapon = CG_GetActiveWeaponWheelWeapon;
	cglobals.GetOwnedWeaponWheelWeapons = CG_GetOwnedWeaponWheelWeapons;
	cglobals.GetWeaponWheelAmmoCount = CG_GetWeaponWheelAmmoCount;
	cglobals.GetPowerupWheelCount = CG_GetPowerupWheelCount;
	cglobals.GetHitMarkerDamage = CG_GetHitMarkerDamage;
	cglobals.ParseConfigString = CG_ParseConfigString;
	cglobals.ParseCenterPrint = CG_ParseCenterPrint;
	cglobals.ClearNotify = CG_ClearNotify;
	cglobals.ClearCenterprint = CG_ClearCenterprint;
	cglobals.NotifyMessage = CG_NotifyMessage;
	cglobals.GetMonsterFlashOffset = CG_GetMonsterFlashOffset;

	cglobals.GetExtension = CG_GetExtension;

	return &cglobals;
}
