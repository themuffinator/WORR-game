/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_changelevel_missing_map.cpp implementation.*/

#include "shared/char_array_utils.hpp"

#include <array>
#include <cassert>
#include <cstring>
#include <string>

namespace {

struct FakeChangelevelEntity {
        std::array<char, 64> map{};
};

struct FakeIntermissionContext {
        std::string changeMap{};
        bool exitLevelCalled = false;

        bool BeginIntermission(FakeChangelevelEntity &ent)
        {
                if (!CharArrayHasText(ent.map))
                        return false; // would now trigger the guard instead of falling through to ExitLevel

                changeMap = ent.map.data();
                exitLevelCalled = true; // stand-in for invoking ExitLevel()
                return true;
        }
};

} // namespace

int main()
{
        FakeChangelevelEntity ent{};
        FakeIntermissionContext ctx{};

        // Regression: a missing map key should short-circuit without reaching the simulated ExitLevel path.
        bool triggered = ctx.BeginIntermission(ent);
        assert(!triggered);
        assert(ctx.changeMap.empty());
        assert(!ctx.exitLevelCalled);

        std::strncpy(ent.map.data(), "unit1", ent.map.size());
        triggered = ctx.BeginIntermission(ent);
        assert(triggered);
        assert(ctx.changeMap == "unit1");
        assert(ctx.exitLevelCalled);

        return 0;
}
