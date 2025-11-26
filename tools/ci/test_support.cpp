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
