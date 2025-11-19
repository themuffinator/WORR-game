#include "server/gameplay/freezetag_damage.hpp"

#include <cassert>

int main() {
        FreezeTagDamageQuery query{};
        query.freezeTagActive = true;
        query.targetEliminated = true;
        query.attackerHasClient = true;

        // Player-fired shots against a frozen target should not inflict health damage.
        assert(FreezeTag_ClampDamage(query, 40) == 0);

        // Thaw damage must pass through so the victim can respawn once freed.
        query.modIsThaw = true;
        assert(FreezeTag_ClampDamage(query, 40) == 40);

        // World hazards are allowed to damage frozen players and can still gib them.
        query.modIsThaw = false;
        query.attackerHasClient = false;
        assert(FreezeTag_ClampDamage(query, 40) == 40);

        // Non-freeze-tag matches should never suppress damage.
        query.freezeTagActive = false;
        assert(FreezeTag_ClampDamage(query, 40) == 40);

        // Targets already being thawed take damage normally so the thaw sequence completes.
        query.freezeTagActive = true;
        query.attackerHasClient = true;
        query.targetThawing = true;
        assert(FreezeTag_ClampDamage(query, 40) == 40);

        return 0;
}

