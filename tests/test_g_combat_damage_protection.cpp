/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_g_combat_damage_protection.cpp implementation.*/

#include <cmath>

namespace std {
        using ::sinf;
}

#include <memory>

#include "server/g_local.hpp"

#include <cassert>

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};
std::mt19937 mt_rand{};
static cvar_t g_gametype_storage{};
cvar_t* g_gametype = &g_gametype_storage;
static cvar_t g_selfDamage_storage{};
cvar_t* g_selfDamage = &g_selfDamage_storage;

int main() {
        MeansOfDeath mod{ ModID::Blaster };

        g_gametype_storage.integer = static_cast<int>(GameType::FreeForAll);
        g_selfDamage_storage.integer = 0;

        // Global combat disable and self-damage toggles should not suppress damage for non-clients.
        DamageProtectionContext propCtx{};
        propCtx.levelTime = 5_sec;
        propCtx.combatDisabled = true;
        propCtx.selfDamageDisabled = true;
        propCtx.isSelfDamage = true;
        DamageProtectionResult propResult = EvaluateDamageProtection(propCtx, DamageFlags::Normal, mod);
        assert(!propResult.prevented);

        // Commander body relies on FL_GODMODE to stay intact while the player retrieves the head.
        DamageProtectionContext commanderCtx{};
        commanderCtx.levelTime = 5_sec;
        commanderCtx.hasGodMode = true;
        DamageProtectionResult commanderResult = EvaluateDamageProtection(commanderCtx, DamageFlags::Normal, mod);
        assert(commanderResult.prevented);
        assert(!commanderResult.playBattleSuitSound);
        assert(!commanderResult.playMonsterSound);

        // Monsters flagged as invincible must shrug off damage and refresh their pain debounce when the sound plays.
        DamageProtectionContext monsterCtx{};
        monsterCtx.levelTime = 5_sec;
        monsterCtx.isMonster = true;
        monsterCtx.monsterInvincibilityTime = monsterCtx.levelTime + 1_sec;
        monsterCtx.painDebounceTime = monsterCtx.levelTime - 1_sec;
        DamageProtectionResult monsterResult = EvaluateDamageProtection(monsterCtx, DamageFlags::Normal, mod);
        assert(monsterResult.prevented);
        assert(monsterResult.playMonsterSound);
        assert(monsterResult.newPainDebounceTime == monsterCtx.levelTime + 2_sec);

        return 0;
}

