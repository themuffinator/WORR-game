#pragma once

#include "server/g_local.hpp"

#include <optional>
#include <string>
#include <string_view>

inline char ToLowerASCII(char c) {
        const unsigned char uc = static_cast<unsigned char>(c);
        if (uc >= 'A' && uc <= 'Z') {
                return static_cast<char>(uc - 'A' + 'a');
        }
        return static_cast<char>(uc);
}

inline char ToUpperASCII(char c) {
        const unsigned char uc = static_cast<unsigned char>(c);
        if (uc >= 'a' && uc <= 'z') {
                return static_cast<char>(uc - 'a' + 'A');
        }
        return static_cast<char>(uc);
}

inline std::string NormalizeWeaponAbbreviation(std::string_view abbr) {
        std::string normalized;
        normalized.reserve(abbr.size());
        for (char ch : abbr) {
                normalized.push_back(ToUpperASCII(ch));
        }
        return normalized;
}

inline std::optional<Weapon> ParseNormalizedWeaponAbbreviation(std::string_view normalized) {
        for (std::size_t i = 0; i < weaponAbbreviations.size(); ++i) {
                if (normalized == weaponAbbreviations[i]) {
                        return static_cast<Weapon>(i);
                }
        }
        return std::nullopt;
}

inline std::optional<Weapon> ParseWeaponAbbreviation(std::string_view abbr) {
        const std::string normalized = NormalizeWeaponAbbreviation(abbr);
        auto weapon = ParseNormalizedWeaponAbbreviation(normalized);
        if (!weapon || *weapon == Weapon::None) {
                return std::nullopt;
        }
        return weapon;
}

inline std::string_view WeaponToAbbreviation(Weapon weapon) {
        const std::size_t index = static_cast<std::size_t>(weapon);
        if (index >= weaponAbbreviations.size()) {
                return {};
        }
        return weaponAbbreviations[index];
}
