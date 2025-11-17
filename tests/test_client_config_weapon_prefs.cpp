#include "server/g_local.hpp"
#include "server/gameplay/client_config.hpp"

#include <cassert>
#include <cstring>
#include <filesystem>

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
std::mt19937 mt_rand{};

int main() {
        const std::string playerID = "test_weapon_pref_roundtrip";
        const std::string playerName = "TestPlayer";
        const std::string gameType = "FFA";
        const std::filesystem::path configDir = "baseq2/pcfg";
        const std::filesystem::path configPath = configDir / (playerID + ".json");

        std::filesystem::create_directories(configDir);
        std::error_code ec;
        std::filesystem::remove(configPath, ec);

        gclient_t client{};
        std::strncpy(client.sess.socialID, playerID.c_str(), sizeof(client.sess.socialID));
        client.sess.socialID[sizeof(client.sess.socialID) - 1] = '\0';

        GetClientConfigStore().LoadProfile(&client, playerID, playerName, gameType);

        client.sess.weaponPrefs = {
                Weapon::Blaster,
                Weapon::Railgun,
                Weapon::Blaster,
                Weapon::Thunderbolt
        };
        Client_RebuildWeaponPreferenceOrder(client);
        auto sanitized = GetSanitizedWeaponPrefStrings(client);
        assert(sanitized.size() == 3);
        assert(sanitized[0] == "BL");
        assert(sanitized[1] == "RG");
        assert(sanitized[2] == "TB");

        GetClientConfigStore().SaveWeaponPreferences(&client);

        gclient_t reloaded{};
        std::strncpy(reloaded.sess.socialID, playerID.c_str(), sizeof(reloaded.sess.socialID));
        reloaded.sess.socialID[sizeof(reloaded.sess.socialID) - 1] = '\0';
        reloaded.sess.weaponPrefs.push_back(Weapon::BFG10K);

        GetClientConfigStore().LoadProfile(&reloaded, playerID, playerName, gameType);
        Client_RebuildWeaponPreferenceOrder(reloaded);
        auto roundTripped = GetSanitizedWeaponPrefStrings(reloaded);

        assert(roundTripped == sanitized);
        assert(reloaded.sess.weaponPrefs.size() == sanitized.size());
        assert(reloaded.sess.weaponPrefs[0] == Weapon::Blaster);
        assert(reloaded.sess.weaponPrefs[1] == Weapon::Railgun);
        assert(reloaded.sess.weaponPrefs[2] == Weapon::Thunderbolt);

        std::filesystem::remove(configPath, ec);

        return 0;
}
