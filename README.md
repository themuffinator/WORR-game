# Welcome to WORR BETA!

## What is WORR?
WORR is a server-side mod for [QUAKE II Remastered](https://github.com/id-Software/quake2-rerelease-dll) that focuses on modernizing multiplayer play with streamlined rulesets, tuned weapon handling, and administrative tooling.

## Project Features

* **Streamlined hosting experience:** Default presets cover respawns, timers, and ready-up thresholds so servers boot with competitive settings, while console-accessible controls adjust warmups, overtime, damage, and join flow without rebuilding the DLL.【F:docs/wiki/server-host-manual.md†L14-L77】
* **Tournament, marathon, and modifier coverage:** Run everything from duel ladders and clan arenas to PvE Horde nights using the expanded gametype roster and runtime toggles for score limits, modifiers, and rule swaps.【F:docs/wiki/server-host-manual.md†L17-L77】
* **Player progression and balancing:** JSON-backed profiles persist HUD preferences, stats, and ratings, and match logic maintains Elo-style skill tracking to keep competition fair while welcoming new players.【F:docs/wiki/client-manual.md†L4-L61】【F:src/server/match/match_state.cpp†L927-L1150】
* **Advanced match flow tooling:** Warmup gates, forced respawns, timeout windows, and automated restarts orchestrate modern match states for leagues, ladders, and casual rotations alike.【F:docs/wiki/server-host-manual.md†L70-L77】【F:src/server/match/match_state.cpp†L239-L360】
* **Reworked map system:** JSON map pools, curated cycles, MyMap queues, and post-match selector votes respect player counts, recent history, and optional item rules while keeping rotations fresh.【F:docs/wiki/server-host-manual.md†L40-L61】【F:docs/mapsystem.html†L17-L101】
* **Extensible admin and voting controls:** Bitmask-driven vote flags, per-player limits, and live commands expose exactly the tweaks each community expects without service restarts.【F:docs/wiki/server-host-manual.md†L63-L68】

## Map Compatibility & PvE Additions

* **Expanded monster roster:** WORR ports iconic Quake 1 foes—including bosses like Chthon and classic enemies such as the attack dog—so custom maps can lean on familiar encounters without extra scripting.【F:src/server/monsters/m_chthon.cpp†L1-L102】【F:src/server/monsters/m_dog.cpp†L1-L119】
* **Legacy entity support:** Target, trigger, and ambient entities translate Quake 1 conventions into Quake II Remastered, unlocking long-requested compatibility updates for classic map packs.【F:src/server/gameplay/g_target.cpp†L285-L325】
* **Horde-ready infrastructure:** The PvE Horde ruleset and HUD integrations surface remaining monsters and rounds to keep wave-based marathons readable for hosts and players alike.【F:docs/wiki/server-host-manual.md†L34-L36】【F:docs/wiki/client-manual.md†L42-L60】

## Learn More

Explore the [WORR Wiki](docs/wiki/) for build instructions, server hosting tips, and client configuration guides.

## Credits

- Nightdive Studios for the Quake II Remastered foundation and continued engine support.
- id Software for Quake II and Quake III Arena codebases that underpin WORR’s gameplay DNA.
- Community contributors and playtesters for surfacing bugs, balance notes, and feature requests.
- Special thanks to fellow modders who shared Horde spawning patterns, entity research, and tooling feedback throughout development.
