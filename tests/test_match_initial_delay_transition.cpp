#include "server/match/match_state_utils.hpp"

#include <cassert>

enum class TestMatchState {
        Initial_Delay,
        Warmup_Default,
        Other
};

enum class TestWarmupState {
        Default,
        Other
};

struct TestTime {
        int ms = 0;

        constexpr TestTime() = default;
        explicit constexpr TestTime(int milliseconds) : ms(milliseconds) {}

        friend constexpr bool operator>(const TestTime &lhs, const TestTime &rhs) {
                return lhs.ms > rhs.ms;
        }

        friend constexpr bool operator==(const TestTime &lhs, const TestTime &rhs) {
                return lhs.ms == rhs.ms;
        }
};

int main() {
        TestMatchState matchState = TestMatchState::Initial_Delay;
        TestWarmupState warmupState = TestWarmupState::Other;
        TestTime timer{5000};
        TestTime notice{};

        TestTime now{4000};
        bool transitioned = MatchWarmup::PromoteInitialDelayToWarmup(
                matchState,
                timer,
                now,
                warmupState,
                notice,
                TestMatchState::Initial_Delay,
                TestMatchState::Warmup_Default,
                TestWarmupState::Default,
                TestTime{});
        assert(!transitioned);
        assert(matchState == TestMatchState::Initial_Delay);
        assert(timer == TestTime{5000});
        assert(warmupState == TestWarmupState::Other);

        now = TestTime{5000};
        transitioned = MatchWarmup::PromoteInitialDelayToWarmup(
                matchState,
                timer,
                now,
                warmupState,
                notice,
                TestMatchState::Initial_Delay,
                TestMatchState::Warmup_Default,
                TestWarmupState::Default,
                TestTime{});
        assert(transitioned);
        assert(matchState == TestMatchState::Warmup_Default);
        assert(timer == TestTime{});
        assert(warmupState == TestWarmupState::Default);
        assert(notice == now);

        transitioned = MatchWarmup::PromoteInitialDelayToWarmup(
                matchState,
                timer,
                now,
                warmupState,
                notice,
                TestMatchState::Initial_Delay,
                TestMatchState::Warmup_Default,
                TestWarmupState::Default,
                TestTime{});
        assert(!transitioned);
        assert(matchState == TestMatchState::Warmup_Default);
        assert(timer == TestTime{});

        return 0;
}
