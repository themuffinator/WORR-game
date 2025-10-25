# Manual QA: Limited Lives Warmup Transition

## Goal
Ensure that elimination-style limited-lives modes (e.g., Last Man Standing) preserve the lives players consumed during warmup once the live match begins, while full match resets still restore everyone to the maximum lives count.

## Steps
1. Start a Last Man Standing match with warmup enabled.
2. Join the match as a player and self-eliminate during warmup to consume one life.
3. Allow the countdown to expire so the match transitions into the live in-progress state.
4. Observe the scoreboard and HUD to confirm your remaining lives did **not** reset to the maximum value at match start.
5. Issue a match reset (e.g., `callvote match_reset`) and wait for the reset to complete.
6. Verify that your lives are restored to the maximum once the reset finishes and warmup resumes.

## Expected Results
- Steps 1–4: The consumed life persists into the live match, confirming warmup deaths carry over.
- Steps 5–6: The forced match reset restores the maximum lives, ensuring administrative resets behave as before.
