#include "server/match/match_state_helper.hpp"

#include <cassert>
#include <cstdint>

enum class TestMatchState : uint8_t {
        None,
        Initial_Delay,
        Warmup_Default,
        Warmup_ReadyUp,
        Countdown,
        In_Progress
};

enum class TestWarmupState : uint8_t {
        Default,
        Not_Ready,
        Too_Few_Players
};

struct FakeContext {
        TestMatchState matchState = TestMatchState::None;
        int matchStateTimer = 0;
        TestWarmupState warmupState = TestWarmupState::Default;
        int warmupNoticeTime = 0;
        bool prepare_to_fight = false;
};

int main() {
        using Transition = MatchStateTransition<FakeContext>;

        // Initial map load should move from None -> Initial_Delay and stamp the timers.
        FakeContext initial{};
        ApplyMatchState(initial, Transition{
                TestMatchState::Initial_Delay,
                5000,
                std::optional<TestWarmupState>{TestWarmupState::Default},
                std::optional<int>{1000},
                std::optional<bool>{false}
        });
        assert(initial.matchState == TestMatchState::Initial_Delay);
        assert(initial.matchStateTimer == 5000);
        assert(initial.warmupState == TestWarmupState::Default);
        assert(initial.warmupNoticeTime == 1000);
        assert(!initial.prepare_to_fight);

        // Ready-up transition should mark the lobby as waiting on players and refresh the notice.
        FakeContext readyUp{};
        readyUp.matchState = TestMatchState::Warmup_Default;
        ApplyMatchState(readyUp, Transition{
                TestMatchState::Warmup_ReadyUp,
                0,
                std::optional<TestWarmupState>{TestWarmupState::Not_Ready},
                std::optional<int>{2500},
                std::optional<bool>{false}
        });
        assert(readyUp.matchState == TestMatchState::Warmup_ReadyUp);
        assert(readyUp.matchStateTimer == 0);
        assert(readyUp.warmupState == TestWarmupState::Not_Ready);
        assert(readyUp.warmupNoticeTime == 2500);
        assert(!readyUp.prepare_to_fight);

        // Countdown transition should keep the warning text but arm the countdown timer.
        FakeContext countdown{};
        countdown.prepare_to_fight = true;
        ApplyMatchState(countdown, Transition{
                TestMatchState::Countdown,
                3000,
                std::optional<TestWarmupState>{TestWarmupState::Default},
                std::optional<int>{0},
                std::optional<bool>{true}
        });
        assert(countdown.matchState == TestMatchState::Countdown);
        assert(countdown.matchStateTimer == 3000);
        assert(countdown.warmupState == TestWarmupState::Default);
        assert(countdown.warmupNoticeTime == 0);
        assert(countdown.prepare_to_fight);

        return 0;
}

