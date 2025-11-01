# Welcome to WORR BETA!

## What is WORR?
WORR is a server-side mod for [QUAKE II Remastered](https://github.com/id-Software/quake2-rerelease-dll) that focuses on modernizing multiplayer play with streamlined rulesets, tuned weapon handling, and administrative tooling.

## New Gametype: Domination
Domination is a team mode where squads fight over map-defined control points. Standing uncontested on a point converts it to your team; once captured, each owned control point pays out score on every Domination scoring tick and flipping a point grants an immediate capture bonus to reward aggressive play. Matches end when a team hits the point target or the timelimit expires, and standard overtime/mercy rules still apply.

* **Short name:** set `g_gametype dom` (or use the Call Vote menu entry) to switch into Domination.
* **Score limit:** Domination uses the global `fraglimit` as its point goal. If the cvar is left at `0` the mode runs without a hard cap; the built-in ruleset recommends 100 points with a 20 minute timelimit.

### Server cvars
| Cvar | Default | Notes |
| --- | --- | --- |
| `g_gametype` | `0` (`ffa`) | Current gametype. Use the short name `dom` when issuing votes or configuring rotations. |
| `fraglimit` | `0` | Domination treats this as the point goal; leave at `0` for untimed score or set to your preferred target (100 recommended). |
| `timelimit` | `0` | Minutes before the match times out. The Domination ruleset suggests `20`. |
| `match_do_overtime` | `120` | Adds sudden-death time when the score is tied at the end of regulation. |
| `g_teamplay_force_balance` | `0` | Optional enforcement for team size balance; useful in Domination’s two-team setup. |

Refer to [docs/domination.md](docs/domination.md) for a deeper dive into control point behavior, scoring cadence, and admin tips.
