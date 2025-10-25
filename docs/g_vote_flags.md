# `g_vote_flags` Reference

The `g_vote_flags` cvar controls which vote types players can initiate. Set it to
the bitwise OR of the options you want to allow (for example, `g_vote_flags 8191`
enables every vote listed below).

| Bit (1 << n) | Value | Vote Command | Description |
|--------------|-------|--------------|-------------|
| 0            | 1     | `map`        | Change to the specified map (supports map flags). |
| 1            | 2     | `nextmap`    | Advance to the next map in the rotation. |
| 2            | 4     | `restart`    | Restart the current match. |
| 3            | 8     | `gametype`   | Switch to a different gametype. |
| 4            | 16    | `timelimit`  | Update or disable the time limit. |
| 5            | 32    | `scorelimit` | Update or disable the score limit. |
| 6            | 64    | `shuffle`    | Shuffle teams based on skill. |
| 7            | 128   | `unlagged`   | Enable or disable lag compensation. |
| 8            | 256   | `cointoss`   | Flip a coin and broadcast the result. |
| 9            | 512   | `random`     | Roll a random number in the requested range. |
| 10           | 1024  | `balance`    | Balance teams without a shuffle. |
| 11           | 2048  | `ruleset`    | Change the active ruleset. |
| 12           | 4096  | `arena`      | Switch to another arena in Arena/RA2 modes. |

Enable the flags that match the entries you want to appear in the Call Vote menu.
