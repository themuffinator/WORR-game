#include "server/g_local.hpp"
#include "shared/weapon_pref_utils.hpp"

#if defined(_MSC_VER)
#define TEST_WEAK __declspec(selectany)
#else
#define TEST_WEAK __attribute__((weak))
#endif

#include <array>
#include <cstring>
#include <string>
#include <vector>

TEST_WEAK local_game_import_t gi{};
TEST_WEAK game_export_t globals{};
TEST_WEAK gentity_t* g_entities = nullptr;
TEST_WEAK GameLocals game{};
TEST_WEAK LevelLocals level{};
TEST_WEAK spawn_temp_t st{};
TEST_WEAK std::mt19937 mt_rand{};
TEST_WEAK cvar_t* g_gametype = nullptr;
TEST_WEAK int CVAR_NONE = 0;

namespace {

/*
=============
TestComPrint

Lightweight print stub used to satisfy logging callbacks during tests.
=============
*/
static void TestComPrint(const char* message)
{
	(void)message;
}

/*
=============
TestComError

Ensures fatal error callbacks do not abort the process during tests.
=============
*/
static void TestComError(const char* message)
{
	(void)message;
}

/*
=============
GiInitializer

Wires safe defaults for the global import table before tests run.
=============
*/
struct GiInitializer {
	GiInitializer()
{
		gi.Com_Print = &TestComPrint;
	gi.Com_Error = &TestComError;
}
};

	GiInitializer g_giInitializer{};

}

/*
=============
Q_strlcpy

Provides a simple bounded string copy implementation for tests.
=============
*/
TEST_WEAK size_t Q_strlcpy(char* dst, const char* src, size_t siz)
{
	if (!dst || !src || siz == 0)
		return 0;

	std::strncpy(dst, src, siz - 1);
	dst[siz - 1] = '\0';
	return std::strlen(src);
}

/*
=============
Q_strlcat

Appends src to dst with basic bounds checking for tests.
=============
*/
TEST_WEAK size_t Q_strlcat(char* dst, const char* src, size_t siz)
{
	const size_t dst_len = std::strlen(dst);
	const size_t src_len = std::strlen(src);

	if (dst_len + 1 >= siz)
		return dst_len + src_len;

	const size_t copy_len = (src_len < (siz - dst_len - 1)) ? src_len : (siz - dst_len - 1);
	std::memcpy(dst + dst_len, src, copy_len);
	dst[dst_len + copy_len] = '\0';
	return dst_len + src_len;
}

TEST_WEAK char local_game_import_t::print_buffer[0x10000];
TEST_WEAK std::array<char[MAX_INFO_STRING], MAX_LOCALIZATION_ARGS> local_game_import_t::buffers{};
TEST_WEAK std::array<const char*, MAX_LOCALIZATION_ARGS> local_game_import_t::buffer_ptrs{};

static constexpr std::array<item_id_t, 21> weaponPriorityList{ {
IT_WEAPON_DISRUPTOR,
	IT_WEAPON_BFG,
	IT_WEAPON_RAILGUN,
	IT_WEAPON_THUNDERBOLT,
	IT_WEAPON_PLASMABEAM,
	IT_WEAPON_IONRIPPER,
	IT_WEAPON_HYPERBLASTER,
	IT_WEAPON_ETF_RIFLE,
	IT_WEAPON_CHAINGUN,
	IT_WEAPON_MACHINEGUN,
	IT_WEAPON_SSHOTGUN,
	IT_WEAPON_SHOTGUN,
	IT_WEAPON_PHALANX,
	IT_WEAPON_RLAUNCHER,
	IT_WEAPON_GLAUNCHER,
	IT_WEAPON_PROXLAUNCHER,
	IT_AMMO_GRENADES,
	IT_AMMO_TRAP,
IT_AMMO_TESLA,
IT_WEAPON_BLASTER,
IT_WEAPON_CHAINFIST,
} };

/*
=============
weaponIndexToItemID

Maps a Weapon enum value to its corresponding item_id_t used in inventory code.
=============
*/
static item_id_t weaponIndexToItemID(Weapon weaponIndex)
{
switch (weaponIndex) {
		using enum Weapon;
	case Blaster:
		return IT_WEAPON_BLASTER;
	case Chainfist:
		return IT_WEAPON_CHAINFIST;
	case Shotgun:
		return IT_WEAPON_SHOTGUN;
	case SuperShotgun:
		return IT_WEAPON_SSHOTGUN;
	case Machinegun:
		return IT_WEAPON_MACHINEGUN;
	case ETFRifle:
		return IT_WEAPON_ETF_RIFLE;
	case Chaingun:
		return IT_WEAPON_CHAINGUN;
	case HandGrenades:
		return IT_AMMO_GRENADES;
	case Trap:
		return IT_AMMO_TRAP;
	case TeslaMine:
		return IT_AMMO_TESLA;
	case GrenadeLauncher:
		return IT_WEAPON_GLAUNCHER;
	case ProxLauncher:
		return IT_WEAPON_PROXLAUNCHER;
	case RocketLauncher:
		return IT_WEAPON_RLAUNCHER;
	case HyperBlaster:
		return IT_WEAPON_HYPERBLASTER;
	case IonRipper:
		return IT_WEAPON_IONRIPPER;
	case PlasmaBeam:
		return IT_WEAPON_PLASMABEAM;
	case Thunderbolt:
		return IT_WEAPON_THUNDERBOLT;
	case Railgun:
		return IT_WEAPON_RAILGUN;
	case Phalanx:
		return IT_WEAPON_PHALANX;
	case BFG10K:
		return IT_WEAPON_BFG;
	case Disruptor:
		return IT_WEAPON_DISRUPTOR;
	default:
		return IT_NULL;
	}
}

/*
=============
Client_RebuildWeaponPreferenceOrder

Rebuilds the client's cached weapon priority list while removing duplicates.
=============
*/
void Client_RebuildWeaponPreferenceOrder(gclient_t& cl)
{
	auto& cache = cl.sess.weaponPrefOrder;
	cache.clear();

	std::array<bool, static_cast<size_t>(IT_TOTAL)> seen{};
	auto add_item = [&](item_id_t item) {
		const auto index = static_cast<size_t>(item);
		if (!item || index >= seen.size() || seen[index])
			return;

		seen[index] = true;
		cache.push_back(item);
	};

	for (Weapon weaponIndex : cl.sess.weaponPrefs) {
		const auto weaponEnumIndex = static_cast<size_t>(weaponIndex);
		if (weaponIndex == Weapon::None || weaponEnumIndex >= static_cast<size_t>(Weapon::Total))
			continue;

		add_item(weaponIndexToItemID(weaponIndex));
	}

	for (item_id_t def : weaponPriorityList)
		add_item(def);
}

/*
=============
GetSanitizedWeaponPrefStrings

Returns a unique list of weapon abbreviations from the client's preferences.
=============
*/
std::vector<std::string> GetSanitizedWeaponPrefStrings(const gclient_t& cl)
{
std::vector<std::string> result;

	if (cl.sess.weaponPrefs.empty())
		return result;

	std::array<bool, static_cast<size_t>(Weapon::Total)> seen{};
	for (Weapon weaponIndex : cl.sess.weaponPrefs) {
		const auto index = static_cast<size_t>(weaponIndex);
		if (weaponIndex == Weapon::None || index >= seen.size() || seen[index])
			continue;

		seen[index] = true;
		std::string_view abbr = WeaponToAbbreviation(weaponIndex);
		if (!abbr.empty())
			result.emplace_back(abbr);
	}

return result;
}

/*
=============
ClientCheckPermissionsForTesting

Assigns admin and ban flags based on a provided social ID, clearing the
flags entirely when no ID is supplied.
=============
*/
void ClientCheckPermissionsForTesting(GameLocals& gameRef, gentity_t* ent, const char* socialID)
{
	if (!ent || !ent->client)
		return;

	ent->client->sess.admin = false;
	ent->client->sess.banned = false;

	if (!socialID || !socialID[0])
		return;

	const std::string idString{ socialID };
	ent->client->sess.admin = gameRef.adminIDs.contains(idString);
	ent->client->sess.banned = gameRef.bannedIDs.contains(idString);
}

/*
=============
Teamplay_IsPrimaryTeam

Determines whether the supplied team participates in primary team modes.
=============
*/
TEST_WEAK bool Teamplay_IsPrimaryTeam(Team team)
{
	return team == Team::Red || team == Team::Blue;
}

/*
=============
SupportsCTF

Reports whether the current gametype supports Capture the Flag mechanics.
=============
*/
TEST_WEAK bool SupportsCTF()
{
	return g_gametype && g_gametype->integer == static_cast<int>(GameType::CaptureTheFlag);
}

/*
=============
CTF_CheckHurtCarrier

Records when an attacker damages the opposing flag carrier in CTF games.
=============
*/
TEST_WEAK void CTF_CheckHurtCarrier(gentity_t* targ, gentity_t* attacker)
{
	if (!SupportsCTF() || !targ || !attacker || !targ->client || !attacker->client)
		return;

	const item_id_t flagItem = targ->client->sess.team == Team::Red ? IT_FLAG_BLUE : IT_FLAG_RED;
	const Team targetTeam = targ->client->sess.team;
	const Team attackerTeam = attacker->client->sess.team;
	if (!Teamplay_IsPrimaryTeam(targetTeam) || !Teamplay_IsPrimaryTeam(attackerTeam))
		return;

	if (targ->client->pers.inventory[flagItem] && targetTeam != attackerTeam)
		attacker->client->resp.ctf_lasthurtcarrier = level.time;
}

/*
=============
VerifyEntityString

Performs a minimal structural validation of an entity string, flagging
truncated definitions.
=============
*/
TEST_WEAK bool VerifyEntityString(const char* entities)
{
	if (!entities)
		return false;

	int braceDepth = 0;
	for (const char* ch = entities; *ch; ++ch) {
		if (*ch == '{')
			++braceDepth;
		else if (*ch == '}') {
			if (braceDepth == 0)
				return false;
			--braceDepth;
		}
	}

	if (braceDepth != 0) {
		if (gi.Com_Error)
			gi.Com_Error("Malformed entity string: unbalanced braces\n");
		return false;
	}

	return true;
}

/*
=============
TimeStamp

Provides a deterministic timestamp for test environments.
=============
*/
std::string TimeStamp()
{
	return "1970-01-01T00:00:00Z";
}

/*
=============
FileTimeStamp

Returns a filesystem-safe timestamp string for test artifacts.
=============
*/
std::string FileTimeStamp()
{
	return "19700101-000000";
}
