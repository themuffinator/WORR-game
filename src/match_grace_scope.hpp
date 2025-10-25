#pragma once

// Utility scope guard used by CheckDMExitRules to track whether any
// grace-based endmatch condition fired during the current frame. When the
// scope ends without a condition being marked active, it automatically resets
// the grace timer so future violations receive a full grace window.
template<typename TimeT>
class EndmatchGraceScope {
public:
        EndmatchGraceScope(TimeT &timer, TimeT zeroValue)
                : timer(timer)
                , zeroValue(zeroValue) {}

        void MarkConditionActive() {
                active = true;
        }

        [[nodiscard]] bool ConditionWasActive() const {
                return active;
        }

        ~EndmatchGraceScope() {
                if (!active && timer)
                        timer = zeroValue;
        }

private:
        TimeT &timer;
        TimeT zeroValue;
        bool active = false;
};
