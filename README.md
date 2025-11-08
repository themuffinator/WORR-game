# WORR!
### An extensive server mod for QUAKE II Rerelease

**WORR** is an advanced gameplay expansion and server mod for **QUAKE II Rerelease**, designed to bring a richer, more dynamic single-player and multiplayer experience.
It dramatically extends entity and gameplay support, integrates modern competitive systems, and refines the core feel of QUAKE II while preserving its fast-paced essence.

It is a successor to my previous project, [Muff Mode](https://github.com/themuffinator/muffmode).

---

## Core Highlights

### General Improvements
- Extensive refinements and bug fixes throughout the game
- Vastly expanded support for **entities**, **monsters**, and **map logic** — including many elements from other **QUAKE titles and popular mods**
- Balanced gameplay for both campaign and multiplayer, offering:
  - Increased challenge and depth for veterans
  - Improved accessibility for new players

### Multiplayer Overhaul
**WORR** transforms QUAKE II’s multiplayer into a flexible, competitive, and feature-rich platform.

#### Gametypes
- **18 distinct gametypes**, including:
  - *Duel, Domination, Head Hunters, Clan Arena, Freeze Tag* and more

#### Game Modifiers
- Unique match modifiers such as:
  - *Vampiric Damage*, *Marathon Mode* and *Gravity Lotto*

#### Hosting & Server Tools
- **Match presets** for consistent, repeatable setups
- **Tournament mode** for running organized events and scheduled matches
- **Powerful map management system** with map pools, voting, and rotation logic

#### Competitive Features
- Full **match state progression** and recovery
- Extended **administrative and match management commands**
- **Server-side client configs** store player settings, preferences, and persistent stats
- **Auto-Ghost system** lets disconnected players seamlessly rejoin — restoring score, inventory, and stats

### Interface & Presentation
- Sleek yet informative, adaptive **HUD**
- **Mini-score display** and **frag messages** integrated into the UI
- Dynamic **match announcer** for event feedback
- **QUAKE** and **QUAKE III: ARENA-style rulesets** that auto-apply during compatible maps

## Documentation & Wiki
- Browse the [WORR Knowledge Base](https://github.com/themuffinator/WORR-kex/wiki) for persona guides, cvar/command references, and feature overviews tailored to hosts, players, designers, and programmers.【F:wiki/Home.md†L1-L58】
- Markdown sources for the live wiki live in the repository’s `wiki/` folder so documentation changes can travel alongside code updates.【F:wiki/Home.md†L3-L56】

---

## Documentation
- **[WORR Wiki](docs/wiki/index.md)** — Persona-focused guides for server hosts, players, level designers, and programmers, plus links to broader reference material.
- Additional technical references live throughout the [`docs/`](docs/) directory, including changelog comparisons and versioning policy.

> WORR does not currently ship a standalone web UI. To browse the wiki, open the Markdown files directly in your editor or render them with your preferred static site tooling.

---

## Release Channels & Upgrade Path
- **Stable** — Fully vetted releases recommended for production servers; expect only critical hotfixes between major updates.
- **Release Candidate (RC)** — Final verification builds that preview the upcoming stable release; suitable for staging environments that mirror production.
- **Beta** — Feature-complete builds undergoing active testing; best for enthusiasts who want early access and can tolerate occasional regressions.
- **Nightly** — Automated snapshots of the latest development work; ideal for contributors validating fixes, but not advised for live servers.

See [docs/versioning.md](docs/versioning.md) for comprehensive versioning policies, upgrade guidance, and support timelines.

## Credits
- id Software for the QUAKE franchise. WORR has taken inspiration from all other classic QUAKE titles, particularly QUAKE and QUAKE LIVE.
- Nightdive for the excellent **QUAKE** and **QUAKE II** Rereleases
- The community, particularly:
  - **Paril**, **serraboof** and **WatIsDeze** for technical help, mostly on the Map-Center Discord server
  - **ozy**, **Jobe** and many more for playtesting and providing feedback on Muff Mode which greatly assisted future development.
  - Projects that WORR has directly or indirectly borrowed from:
    - QUAKE Rerelease (Nightdive, id Software)
    - QUAKE II Rerelease (Nightdive, id Software)
    - QUAKE III: Arena (id Software)
    - Muff Mode
    - Ionized (Konig)
    - Oblivion (Team Evolve, particularly John Fitzgibbons)
    - Horde Mod (Paril)
    - PSX Mod (Paril)

## Links
- Get Quake II Rerelease on [Steam](https://store.steampowered.com/agecheck/app/2320/), [GOG.com](https://www.gog.com/en/game/quake_ii_quad_damage), [Epic Games Store](https://store.epicgames.com/en-US/p/quake-ii) and a variety of console platforms.
- Quake II Rerelease [source code](https://github.com/id-Software/quake2-rerelease-dll)
- The Ionized mod: [source](https://github.com/Konig-Varorson/quake2-rerelease-dll-ionized/) and [ModDb page](https://www.moddb.com/mods/quake-ii-ionized/)
- Join in the discussion at [DarkMatter on Discord](https://discord.gg/T32mFejwR4)

## Repository layout
- `tools/` – Development utilities, automation scripts, and packaging helpers.
- `refs/` – Reference material such as upstream source drops, change patches, and assorted notes used for historical context during development.
