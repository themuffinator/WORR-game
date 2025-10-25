#pragma once

struct FreezeTagDamageQuery {
        bool freezeTagActive = false;
        bool targetEliminated = false;
        bool targetThawing = false;
        bool attackerHasClient = false;
        bool modIsThaw = false;
};

inline bool FreezeTag_ShouldSuppressDamage(const FreezeTagDamageQuery& query) {
        if (!query.freezeTagActive)
                return false;
        if (!query.targetEliminated)
                return false;
        if (query.targetThawing)
                return false;
        if (query.modIsThaw)
                return false;
        if (!query.attackerHasClient)
                return false;
        return true;
}

inline int FreezeTag_ClampDamage(const FreezeTagDamageQuery& query, int take) {
        if (take <= 0)
                return 0;
        return FreezeTag_ShouldSuppressDamage(query) ? 0 : take;
}

