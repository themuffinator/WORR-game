/*Copyright (c) 2024 ZeniMax Media Inc.
Licensed under the GNU General Public License 2.0.

cg_main.cpp (Client Game Main) This file serves as the main entry point and API bridge for the client-side game module (cgame). It is responsible for initializing and shutting down the client-side game logic and exporting the necessary functions to the main engine. Key Responsibilities: - Implements `GetCGameAPI`, the function the engine calls to get the table of client-side game functions (`cgame_export_t`). - Handles the initialization (`InitCGame`) and shutdown (`ShutdownCGame`) of the client-side module, setting up necessary systems like the HUD. - Provides wrapper functions that are exposed to the engine, which in turn call the actual implementation logic located in other `cg_` files (e.g., `CG_DrawHUD`, `CG_ParseCenterPrint`). - Manages client-side state that depends on server configstrings, like physics settings (`pm_config`).*/

#include "cg_local.hpp"
#include "../server/monsters/m_flash.hpp"
#include "../shared/logger.hpp"
#include <array>
#include <charconv>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <span>
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

Fetch a named extension interface from the client module. Currently no extensions
are exposed; unknown requests are logged to aid engine integration.
=============
*/
static void* CG_GetExtension(const char* name)
{
	struct extension_entry_t {
		const char* name;
		void* interface_ptr;
	};
	
	static constexpr std::array<extension_entry_t, 0> extensions{};
	
	if (!name)
		return nullptr;
	
	for (const auto& extension : extensions)
	{
		if (std::strcmp(name, extension.name) == 0)
			return extension.interface_ptr;
	}
	
	if (cgi.Com_Print)
	{
		char message_buffer[256] = {};
		std::snprintf(
			message_buffer,
			sizeof(message_buffer),
			"CG_GetExtension: requested unknown extension '%s' (no client extensions are exposed)\n",
			name);
		cgi.Com_Print(message_buffer);
	}
	
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

	pm_config.n64Physics = std::atoi(cgi.get_configString(CONFIG_N64_PHYSICS)) != 0;
	pm_config.airAccel = std::atoi(cgi.get_configString(CS_AIRACCEL));
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
CG_GetAmmoStatsSpan

Build a span over the packed ammo statistics in the player state.
=============
*/
static std::span<uint16_t> CG_GetAmmoStatsSpan(const player_state_t* ps)
{
	auto stats_span = std::span<const int16_t>{ps->stats};
	auto ammo_info = stats_span.subspan(STAT_AMMO_INFO_START, NUM_AMMO_STATS);

	return std::span<uint16_t>{const_cast<uint16_t*>(reinterpret_cast<const uint16_t*>(ammo_info.data())), ammo_info.size()};
}

/*
=============
CG_GetPowerupStatsSpan

Build a span over the packed powerup statistics in the player state.
=============
*/
static std::span<uint16_t> CG_GetPowerupStatsSpan(const player_state_t* ps)
{
	auto stats_span = std::span<const int16_t>{ps->stats};
	auto powerup_info = stats_span.subspan(STAT_POWERUP_INFO_START, NUM_POWERUP_STATS);

	return std::span<uint16_t>{const_cast<uint16_t*>(reinterpret_cast<const uint16_t*>(powerup_info.data())), powerup_info.size()};
}

/*
=============
CG_GetAmmoStatValue

Retrieve a packed ammo statistic from the player state.
=============
*/
static uint16_t CG_GetAmmoStatValue(const player_state_t* ps, int32_t ammoID)
{
	std::span<uint16_t> ammo_stats = CG_GetAmmoStatsSpan(ps);

	return GetAmmoStat(ammo_stats.data(), ammoID);
}

/*
=============
CG_GetPowerupStatValue

Retrieve a packed powerup statistic from the player state.
=============
*/
static uint16_t CG_GetPowerupStatValue(const player_state_t* ps, int32_t powerup_id)
{
	std::span<uint16_t> powerup_stats = CG_GetPowerupStatsSpan(ps);

	return GetPowerupStat(powerup_stats.data(), powerup_id);
}

/*
=============
CG_GetWeaponWheelAmmoCount
=============
*/
static int16_t CG_GetWeaponWheelAmmoCount(const player_state_t* ps, int32_t ammoID)
{
	uint16_t ammo = CG_GetAmmoStatValue(ps, ammoID);

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
	return CG_GetPowerupStatValue(ps, powerup_id);
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
	switch (i) {
	case CONFIG_N64_PHYSICS:
		pm_config.n64Physics = std::atoi(s) != 0;
		break;

	case CS_AIRACCEL:
		pm_config.airAccel = std::atoi(s);
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
