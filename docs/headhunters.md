# Head Hunters Gametype

Head Hunters is a WORR multiplayer mode that rewards risk management and map awareness. Players collect the heads that drop from frags, carry them as a visible trophy rack, and cash them in at neutral receptacles for escalating point payouts.

## Core Rules

- **Heads on death.** Each player drops every head they were holding when eliminated. The server spawns physical skull pickups that can be scooped up by anyone in play.
- **Carry limit.** Players may hold up to 20 heads at a time; attempting to grab more while at the cap ignores the pickup.
- **Deposit to score.** Touching a receptacle while carrying heads awards triangular scoring—`1 + 2 + … + n`—so bigger runs pay off exponentially (e.g., 3 heads = 6 points, 5 heads = 15). The deposit also resets the carrier’s held heads and triggers the receptacle spike display.
- **HUD feedback.** While Head Hunters is active, the HUD exposes the number of heads you currently hold so you know when it is safe to bank or keep hunting.

## Map Setup

- Place one or more `headhunters_receptacle` entities where you want teams to cash in heads. WORR automatically snaps the trigger bounds and registers the receptacle with the gametype manager.
- Receptacles can be flagged for specific teams by setting spawnflags; when no team is assigned they behave as neutral drop-offs.

Thoughtful receptacle placement—on exposed high ground, in defensible alcoves, or near key travel routes—encourages coordinated pushes and counter-attacks. Balance travel time so players cannot hoard heads indefinitely without risking a score attempt.
