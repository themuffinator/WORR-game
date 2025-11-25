/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

test_endmatch_grace.cpp implementation.*/

#include "server/match/g_match_grace_scope.hpp"

#include <cassert>

struct FakeTime {
        int value = 0;

        static FakeTime From(int v) {
                return FakeTime{ v };
        }

        explicit operator bool() const {
                return value != 0;
        }

        bool operator==(const FakeTime &other) const {
                return value == other.value;
        }
};

int main() {
        FakeTime zero{};
        FakeTime timer{};

        // When no condition is marked active, the scope resets a non-zero timer.
        timer.value = FakeTime::From(500).value;
        {
                EndmatchGraceScope<FakeTime> scope(timer, zero);
        }
        assert(!timer);

        // Marking the condition as active preserves the timer value.
        timer.value = FakeTime::From(750).value;
        {
                EndmatchGraceScope<FakeTime> scope(timer, zero);
                scope.MarkConditionActive();
        }
        assert(timer == FakeTime::From(750));

        // Simulate a grace violation starting and then clearing before the window expires.
        FakeTime firstViolation = FakeTime::From(1000);
        timer.value = zero.value;
        {
                EndmatchGraceScope<FakeTime> scope(timer, zero);
                if (!timer)
                        timer.value = firstViolation.value;
                scope.MarkConditionActive();
        }
        assert(timer == firstViolation);

        // The next frame sees the violation cleared, so the timer resets.
        {
                EndmatchGraceScope<FakeTime> scope(timer, zero);
        }
        assert(!timer);

        // A subsequent violation should receive the full grace window by getting a fresh timestamp.
        FakeTime secondViolation = FakeTime::From(1500);
        {
                EndmatchGraceScope<FakeTime> scope(timer, zero);
                assert(!timer); // regression guard: timer must have been cleared
                if (!timer)
                        timer.value = secondViolation.value;
                scope.MarkConditionActive();
        }
        assert(timer == secondViolation);

        return 0;
}
