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
#include "../server/monsters/m_flash.hpp"
#include "../shared/logger.hpp"
#include <charconv>
#include <cstring>
#include <system_error>

cgame_import_t cgi;
cgame_export_t cglobals;

static cgame_import_t base_cgi;

/*
=============
ParseUnsignedInteger

Parse an unsigned integer from the provided C-string using std::from_chars.
=============
*/
static bool ParseUnsignedInteger(const char* text, uint32_t& value)
{
	if (!text)
	return false;

	const char* end = text + std::strlen(text);
	auto result = std::from_chars(text, end, value, 10);

	return result.ec == std::errc() && result.ptr == end;
}

/*
=============
InitClientLogging

Configure shared logging for the client game module.
=============
*/
static void InitClientLogging()
{
	base_cgi = cgi;
	const auto print_sink = [print_fn = base_cgi.Com_Print](std::string_view message) {
		print_fn(message.data());
	};
	const auto error_sink = [error_fn = base_cgi.Com_Error](std::string_view message) {
		error_fn(message.data());
	};

	worr::InitLogger("client", print_sink, error_sink);
	cgi.Com_Print = worr::LoggerPrint;
}

/*
=============
CG_GetExtension
=============
*/
static void* CG_GetExtension(const char* name)
{
	return nullptr;
}

void CG_InitScreen();

uint64_t cgame_init_time = 0;

/*
=============
InitCGame

Initialize client-side systems and cache configuration values.
=============
*/
static void InitCGame()
{
	CG_InitScreen();

	cgame_init_time = cgi.CL_ClientRealTime();

	uint32_t config_value = 0;
	if (!ParseUnsignedInteger(cgi.get_configString(CONFIG_N64_PHYSICS_MEDAL), config_value))
	cgi.Com_Error("Invalid CONFIG_N64_PHYSICS_MEDAL configstring");

	pm_config.n64Physics = config_value != 0;

	config_value = 0;
	if (!ParseUnsignedInteger(cgi.get_configString(CS_AIRACCEL), config_value))
	cgi.Com_Error("Invalid CS_AIRACCEL configstring");

	pm_config.airAccel = static_cast<int32_t>(config_value);
}

/*
=============
ShutdownCGame
=============
*/
static void ShutdownCGame()
{

}

void CG_DrawHUD(int32_t isplit, const cg_server_data_t* data, vrect_t hud_vrect, vrect_t hud_safe, int32_t scale, int32_t playernum, const player_state_t* ps);
void CG_TouchPics();
layout_flags_t CG_LayoutFlags(const player_state_t* ps);

/*
=============
CG_GetActiveWeaponWheelWeapon
=============
*/
static int32_t CG_GetActiveWeaponWheelWeapon(const player_state_t* ps)
{
	return ps->stats[STAT_ACTIVE_WHEEL_WEAPON];
}

/*
=============
CG_GetOwnedWeaponWheelWeapons
=============
*/
static uint32_t CG_GetOwnedWeaponWheelWeapons(const player_state_t* ps)
{
	return ((uint32_t)(uint16_t)ps->stats[STAT_WEAPONS_OWNED_1]) | ((uint32_t)(uint16_t)(ps->stats[STAT_WEAPONS_OWNED_2]) << 16);
}

/*
=============
CG_GetWeaponWheelAmmoCount
=============
*/
static int16_t CG_GetWeaponWheelAmmoCount(const player_state_t* ps, int32_t ammoID)
{
	uint16_t ammo = GetAmmoStat((uint16_t*)&ps->stats[STAT_AMMO_INFO_START], ammoID);

	if (ammo == AMMO_VALUE_INFINITE)
	return -1;

	return ammo;
}

/*
=============
CG_GetPowerupWheelCount
=============
*/
static int16_t CG_GetPowerupWheelCount(const player_state_t* ps, int32_t powerup_id)
{
	return GetPowerupStat((uint16_t*)&ps->stats[STAT_POWERUP_INFO_START], powerup_id);
}

/*
=============
CG_GetHitMarkerDamage
=============
*/
static int16_t CG_GetHitMarkerDamage(const player_state_t* ps)
{
	return ps->stats[STAT_HIT_MARKER];
}

/*
=============
CG_ParseConfigString
=============
*/
static void CG_ParseConfigString(int32_t i, const char* s)
{
	uint32_t config_value = 0;

	switch (i) {
		case CONFIG_N64_PHYSICS_MEDAL:
		if (!ParseUnsignedInteger(s, config_value))
		cgi.Com_Error("Invalid CONFIG_N64_PHYSICS_MEDAL configstring");

		pm_config.n64Physics = config_value != 0;
		break;

		case CS_AIRACCEL:
		if (!ParseUnsignedInteger(s, config_value))
		cgi.Com_Error("Invalid CS_AIRACCEL configstring");

		pm_config.airAccel = static_cast<int32_t>(config_value);
		break;

		default:
		break;
	}
}

void CG_ParseCenterPrint(const char* str, int isplit, bool instant);
void CG_ClearNotify(int32_t isplit);
void CG_ClearCenterprint(int32_t isplit);
void CG_NotifyMessage(int32_t isplit, const char* msg, bool is_chat);

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
Q2GAME_API cgame_export_t* GetCGameAPI(cgame_import_t* import) {
	cgi = *import;

	InitClientLogging();

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
