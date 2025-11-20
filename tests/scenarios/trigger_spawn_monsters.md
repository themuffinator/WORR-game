# Trigger-spawn monster coverage

This scenario exercises both stationary and mobile monsters that use `SPAWNFLAG_MONSTER_TRIGGER_SPAWN` to ensure the flag is cleared after activation.

## Layout
- A `trigger_once` targets two monsters:
  - `monster_turret` configured as a stationary monster with the trigger-spawn flag.
  - `monster_soldier` configured as a walking monster with the trigger-spawn flag.
- Both monsters are positioned so they will not see the player until triggered.

## Steps
1. Load the map in a local server session.
2. Approach the trigger volume to spawn both monsters.
3. Confirm that:
   - Neither monster retains the trigger-spawn flag after appearing (they should no longer ignore follow-up triggers or remain hidden).
   - The stationary turret remains solid and attack-capable after spawning.
   - The walking soldier immediately resumes normal AI, including patrolling or engaging the player.

## Expected result
After the trigger fires once, both monsters behave identically to monsters placed without the trigger-spawn flag, proving the flag was cleared as part of activation.
