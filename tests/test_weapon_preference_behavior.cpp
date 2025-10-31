#include "server/g_local.hpp"

#include <cassert>
#include <cstddef>

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
std::mt19937 mt_rand{};

static void TestComPrint(const char*) {}
static void TestBroadcast(print_type_t, const char*) {}
static void TestClientPrint(gentity_t*, print_type_t, const char*) {}
static void TestCenterPrint(gentity_t*, const char*) {}
static void TestSound(gentity_t*, soundchan_t, int, float, float, float) {}
static void TestComError(const char*) { assert(false && "Com_Error called"); }

int main() {
        gi.Com_Print = TestComPrint;
        gi.Broadcast_Print = TestBroadcast;
        gi.Client_Print = TestClientPrint;
        gi.Center_Print = TestCenterPrint;
        gi.sound = TestSound;
        gi.Com_Error = TestComError;

        cvar_t deathmatchVar{};
        deathmatchVar.integer = 1;
        deathmatch = &deathmatchVar;

        gclient_t client{};
        client.sess.weaponPrefs = {
                Weapon::Railgun,
                Weapon::Shotgun
        };

        Client_RebuildWeaponPreferenceOrder(client);
        assert(!client.sess.weaponPrefOrder.empty());
        assert(client.sess.weaponPrefOrder[0] == IT_WEAPON_RAILGUN);
        assert(client.sess.weaponPrefOrder[1] == IT_WEAPON_SHOTGUN);

        gentity_t ent{};
        ent.client = &client;

        Item* railgun = GetItemByIndex(IT_WEAPON_RAILGUN);
        Item* shotgun = GetItemByIndex(IT_WEAPON_SHOTGUN);
        assert(railgun && shotgun);

        client.pers.inventory.fill(0);
        client.pers.inventory[railgun->id] = 1;
        client.pers.inventory[railgun->ammo] = railgun->quantity;
        client.pers.inventory[shotgun->id] = 1;
        client.pers.inventory[shotgun->ammo] = 0; // simulate dry shotgun

        level.time = 0_ms;
        client.weapon.pending = nullptr;
        client.pers.weapon = shotgun;

        NoAmmoWeaponChange(&ent, false);
        assert(client.weapon.pending == railgun);

        client.weapon.pending = nullptr;
        client.pers.autoswitch = WeaponAutoSwitch::Always;
        client.pers.inventory[shotgun->ammo] = shotgun->quantity; // restock shells

        // Holding shotgun, prefer railgun, so switch when picking it up
        client.pers.weapon = shotgun;
        G_CheckAutoSwitch(&ent, railgun, true);
        assert(client.weapon.pending == railgun);

        // Holding railgun, picking shotgun should not override the higher priority weapon
        client.weapon.pending = nullptr;
        client.pers.weapon = railgun;
        G_CheckAutoSwitch(&ent, shotgun, true);
        assert(client.weapon.pending == nullptr);

        return 0;
}
