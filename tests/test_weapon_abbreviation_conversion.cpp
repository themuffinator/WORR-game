/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_weapon_abbreviation_conversion.cpp implementation.*/

#include "shared/weapon_pref_utils.hpp"

#include <cassert>
#include <string>

int main() {
        auto mixedCase = ParseWeaponAbbreviation("bL");
        assert(mixedCase.has_value());
        assert(*mixedCase == Weapon::Blaster);

        std::string extendedSingle = "b";
        extendedSingle.push_back(static_cast<char>(0xE9));
        assert(!ParseWeaponAbbreviation(extendedSingle).has_value());

        std::string extendedTrailing = "bl";
        extendedTrailing.push_back(static_cast<char>(0xFF));
        assert(!ParseWeaponAbbreviation(extendedTrailing).has_value());

        std::string mixedExtended = "r";
        mixedExtended.push_back(static_cast<char>(0xDF));
        assert(!ParseWeaponAbbreviation(mixedExtended).has_value());

        assert(NormalizeWeaponAbbreviation("hb") == "HB");

        assert(!ParseWeaponAbbreviation("unknown").has_value());
        assert(!ParseWeaponAbbreviation("none").has_value());

        assert(WeaponToAbbreviation(Weapon::Blaster) == "BL");

        return 0;
}
