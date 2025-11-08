# WORR Knowledge Base

Welcome to the WORR Knowledge Base, a curated hub for everything related to the WORR gameplay expansion for Quake II Rerelease. WORR dramatically broadens multiplayer systems, entity support, and presentation polish while preserving the fast, kinetic feel of id Software's classic shooter.【F:README.md†L1-L47】

> **Maintainers:** This repository ships Markdown sources for the GitHub-hosted wiki under `wiki/`. Mirror updates from this directory into the [`WORR-kex` wiki](https://github.com/themuffinator/WORR-kex/wiki) by pushing the pages into the separate `WORR-kex.wiki` repository so players always see the latest guidance.

## Getting Started
- **Why WORR?** Review the [comparison journal](docs/worr_vs_quake2.md) for a high-level breakdown of how WORR evolves the rerelease DLL’s gameplay, administration, and analytics stack.
- **Release cadence:** Consult [Versioning & Release Channels](docs/versioning.md) for stability expectations before upgrading production servers.
- **What changed recently?** Track packaged updates and patch notes in `CHANGELOG.md` once published.

## Personas & Quick Links
Choose the guide tailored to your role. Each page cross-links to supporting references and deep dives.

| Persona | Focus |
| --- | --- |
| [Server Hosts](Server-Hosts.md) | Deployment workflows, map rotation tooling, vote governance, analytics exports. |
| [Players](Players.md) | Gametype rules, combat changes, UI improvements, etiquette for votes and reconnects. |
| [Level Designers](Level-Design.md) | Entity catalogue, map metadata schema, spawn naming, gametype-specific considerations. |
| [Programmers](Programmers.md) | Repository layout, build tooling, subsystem entry points, contribution workflow. |

Additional reference pages:
- [Console Commands](Commands.md) — Administrative, competitive, and player-facing console verbs.
- [Cvar Reference](Cvars.md) — Tunable runtime settings grouped by responsibility.

## Feature Highlights
- **Multiplayer expansion:** WORR ships 17 curated gametypes, match presets, and a hardened tournament flow that goes well beyond stock Quake II.【F:README.md†L20-L40】
- **Systems depth:** JSON map pools, MyMap queues, and selectors ensure rotations reflect player counts and curated tags.【F:docs/mapsystem.html†L17-L104】
- **Administrative control:** Fine-grained vote flags, match management commands, and lockouts give server operators robust governance.【F:docs/g_vote_flags.md†L1-L24】【F:src/server/commands/command_admin.cpp†L484-L510】
- **Player experience:** Adaptive HUD elements, announcer feedback, and ghost persistence keep matches readable and continuous.【F:README.md†L42-L45】【F:src/server/gameplay/g_spawn_points.cpp†L1350-L1396】

## Keeping the Wiki Discoverable
- Add a “Wiki” link wherever documentation surfaces (game menus, website landing pages, community posts) so newcomers can find these guides quickly.
- Include direct pointers to persona pages in release announcements when rolling out features relevant to a specific audience.

## Contributing to Documentation
1. Edit the relevant Markdown under `wiki/`.
2. Run `markdownlint` (if available locally) to keep formatting consistent.
3. Commit the change in the main repository.
4. Clone `https://github.com/themuffinator/WORR-kex.wiki.git` and copy the updated pages across (or add the wiki repository as a secondary remote).
5. Push to publish the live wiki.

Pull requests that modify gameplay, commands, or configuration should update the corresponding wiki page so downstream operators and players can adopt the change confidently.
