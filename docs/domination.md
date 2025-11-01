# Domination Gametype

Domination is WORR's territory-control ruleset. Each supported map places several control points that can be owned by either team. Standing uncontested on a point starts a brief capture progress; once the progress meter fills the point flips to your colors, announces the change, and begins awarding score to your team.

## Scoring Model
- **Scoring tick:** every tick the server grants one point for each control point your team owns.
- **Capture bonus:** flipping a point immediately pays out an additional burst of score so aggressive play is rewarded even if the defense retakes the location moments later.
- **Win conditions:** the match ends when a team reaches the configured point target (`fraglimit`) or when the `timelimit` expires. Standard overtime rules apply when scores are tied.

## Server Controls
| Cvar | Default | Description |
| --- | --- | --- |
| `g_gametype` | `0` (`ffa`) | Set to the short name `dom` (or the numeric index for `GameType::Domination`) to run the mode. |
| `fraglimit` | `0` | Points required to win. Recommended `100` for classic Domination pacing. |
| `timelimit` | `0` | Minutes before the server stops the round. The Domination ruleset suggests `20`. |
| `match_do_overtime` | `120` | Adds sudden-death time in tied games. |
| `g_teamplay_force_balance` | `0` | When enabled, keeps the teams within one player of each other. |

### Admin Tips
- Add `dom` entries to your map rotation or voting configuration so players can swap modes quickly.
- Because `teamplay` is forced on and `ctf` is forced off when Domination loads, you can safely reuse existing TDM maplists.
- Use the scoreboard’s “PT” column to monitor total points per team while the ticker tracks score as the match progresses.
