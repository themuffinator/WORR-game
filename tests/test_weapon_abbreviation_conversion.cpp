#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <string>

namespace {
enum class Weapon : std::uint8_t {
        None,
        Blaster,
        Total,
};

constexpr std::array<const char*, static_cast<std::size_t>(Weapon::Total)> kWeaponAbbreviations = {
        "NONE",
        "BL",
};

Weapon GetWeaponIndexByAbbrev(std::string abbr) {
        std::transform(abbr.begin(), abbr.end(), abbr.begin(), [](unsigned char ch) {
                return static_cast<char>(std::toupper(ch));
        });

        for (std::size_t i = 0; i < kWeaponAbbreviations.size(); ++i) {
                if (abbr == kWeaponAbbreviations[i])
                        return static_cast<Weapon>(i);
        }
        return Weapon::None;
}
}

int main() {
        assert(GetWeaponIndexByAbbrev("bl") == Weapon::Blaster);

        std::string extendedSingle = "b\xE9"; // lowercase + extended character
        assert(GetWeaponIndexByAbbrev(extendedSingle) == Weapon::None);

        std::string extendedTrailing = "bl";
        extendedTrailing.push_back(static_cast<char>(0xFF));
        assert(GetWeaponIndexByAbbrev(extendedTrailing) == Weapon::None);

        std::string localized = "rg";
        localized.push_back(static_cast<char>(0xDF)); // German sharp s in Latin-1
        assert(GetWeaponIndexByAbbrev(localized) == Weapon::None);

        return 0;
}
