# Match Stats JSON Overview

At the end of each match the server serializes a JSON payload describing the session. The
following table summarizes the top-level fields that analytics pipelines typically consume.

| Field | Type | Notes |
| --- | --- | --- |
| `matchID` | string | Unique identifier for the match instance. |
| `serverName` | string | Configured server name (`hostname` cvar). |
| `serverHostName` | string | Optional. Present when the lobby host is connected and exposes a `name` in their user info. |
| `gameType` | string | Short identifier for the current game type (e.g., `FFA`, `TDM`). |
| `ruleSet` | string | Long-form ruleset name. |
| `mapName` | string | Active map at match end. |
| `matchRanked` | boolean | Whether ranked rules were active. |
| `totalKills` | integer | Aggregate number of kills for all players. |
| `totalSpawnKills` | integer | Aggregate spawn kills. |
| `totalTeamKills` | integer | Aggregate team kills. |
| `totalDeaths` | integer | Aggregate deaths. |
| `totalSuicides` | integer | Aggregate suicides. |
| `avKillsPerMinute` | number | Average kills per minute across the lobby. |
| `totalFlagsCaptured` | integer | Capture-the-flag captures (0 when not applicable). |
| `totalFlagAssists` | integer | Capture-the-flag assists. |
| `totalFlagDefends` | integer | Capture-the-flag defends. |
| `matchTimeStart` | integer | Epoch milliseconds when the match started. |
| `matchTimeEnd` | integer | Epoch milliseconds when the match ended. |
| `matchTimeDuration` | integer | Match length in milliseconds. |
| `players` | array | Player-level stat objects. |
| `teams` | array | Team-level stat objects (empty for FFA/Duel). |

Downstream tooling should treat unspecified optional keys (such as `serverHostName`) as absent
rather than blank strings.
