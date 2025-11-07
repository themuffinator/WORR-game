# WORR Server Administration Manual

## Core match control commands
| Command | Usage | Effect |
|---------|-------|--------|
| `gametype <mode>` | Accepts any short or long name from the gameplay manual (for example `gametype strike`). | Calls `ChangeGametype` immediately, reloading the map if necessary.【F:src/server/commands/command_admin.cpp†L209-L227】 |
| `ruleset <alias>` | Accepts `q1`, `q2`, `q3a` (or their extended aliases such as `quake2`, `vadrigar`). | Forces `g_ruleset` to the requested index, switching between SLIPGATE, STROYENT, and VADRIGAR baselines.【F:src/server/commands/command_admin.cpp†L359-L375】【F:src/server/g_local.hpp†L106-L118】 |
| `reset_match` | `reset_match` | Ends the current game in place, runs full warmup reset logic, and reapplies the active presets.【F:src/server/commands/command_admin.cpp†L346-L356】 |
| `ready_all` / `unready_all` | `ready_all` or `unready_all` | Force every active player into or out of ready status for warmup checks.【F:src/server/commands/command_admin.cpp†L288-L294】【F:src/server/commands/command_admin.cpp†L472-L477】 |

After changing a gametype or ruleset mid-match, follow up with `reset_match` so world state, item spawns, and ready-up flow reinitialize under the new configuration.【F:src/server/commands/command_admin.cpp†L346-L356】 Use the ready toggles to shepherd players through the warmup gate when strict minimums are in place.【F:src/server/match/match_state.cpp†L1427-L1467】
