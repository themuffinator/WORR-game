# Manual QA: Callvote Timelimit Input Validation

This checklist verifies that vote arguments with trailing junk text (for example `30abc`) are rejected.

## Preconditions
- Start a local game session with voting enabled (e.g. `g_allowVoting 1`).
- Join the game as a client with permission to call votes.

## Steps
1. Open the in-game console.
2. Run `callvote timelimit 30abc`.
3. Observe the console message.

## Expected Result
- The vote is **not** started and the client receives the "Invalid time limit value." error message from `Validate_Timelimit`.

## Regression Context
Previously the command accepted partial numeric input (such as `30abc`) because it ignored trailing characters when parsing. The fix requires the entire argument to parse successfully before the vote can proceed.
