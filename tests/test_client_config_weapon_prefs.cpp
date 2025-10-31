#include "g_local.hpp"

#include <json/json.h>

#include <cassert>
#include <string>
#include <vector>

local_game_import_t gi{};

int main() {
        gclient_t original{};
        original.sess.weaponPrefs = {"rg", "BL", "rg", "hb", "invalid"};
        Client_RebuildWeaponPreferenceOrder(&original);

        const std::vector<std::string> sanitized = GetSanitizedWeaponPrefStrings(original);
        assert((sanitized == std::vector<std::string>{"RG", "BL", "HB"}));

        Json::Value playerData(Json::objectValue);
        Json::Value config(Json::objectValue);
        Json::Value prefs(Json::arrayValue);
        for (const auto& pref : sanitized)
                prefs.append(pref);
        config["weaponPrefs"] = prefs;
        playerData["config"] = config;

        gclient_t reloaded{};
        const Json::Value& stored = playerData["config"]["weaponPrefs"];
        if (stored.isArray()) {
                for (const auto& entry : stored) {
                        if (entry.isString())
                                reloaded.sess.weaponPrefs.push_back(entry.asString());
                }
        }
        Client_RebuildWeaponPreferenceOrder(&reloaded);
        const std::vector<std::string> roundTripped = GetSanitizedWeaponPrefStrings(reloaded);

        assert(roundTripped == sanitized);
        return 0;
}
