# WORR Wiki Overview

Welcome to the WORR knowledge base. This page outlines the mod's overall scope, major multiplayer and system innovations, and key interface upgrades to help you navigate the rest of the documentation.

## Persona Guides

- **[Server Hosts Guide](server-hosts.md)** — Deployment strategies, server presets, and administrative tooling.
- **[Players Guide](players.md)** — Gameplay fundamentals, gametype tips, and quality-of-life features.
- **[Level Designers Guide](level-designers.md)** — Entity support, map logic extensions, and content authoring workflows.
- **[Programmers Guide](programmers.md)** — Code architecture, scripting hooks, and extension points.
- **[Cvar Reference](cvars.md)** — Full catalogue of server variables, default values, and scope notes.

## Reference Material

- [WORR cvar matrix](cvars.md) — Cross-link of settings discussed throughout the persona guides.

> **What’s new vs. the stock QUAKE II Rerelease?**
> 
> WORR layers modern systems on top of the rerelease foundation: expanded entities, competitive infrastructure, adaptive UI, and more. For a deeper side-by-side comparison, see [docs/worr_vs_quake2.md](../worr_vs_quake2.md).

## Project Scope

WORR is an advanced gameplay expansion and server mod for the **QUAKE II Rerelease**. It builds on the rerelease to deliver a richer, more dynamic single-player and multiplayer experience. Extensive refinements and bug fixes span the campaign and multiplayer modes, with vastly expanded support for entities, monsters, and map logic that pull in elements from other QUAKE titles and popular community mods. Gameplay is tuned for both veterans seeking greater challenge and newcomers who benefit from improved accessibility.

## Multiplayer & Systems Highlights

WORR transforms multiplayer into a flexible, competitive, and feature-rich platform:

- **Seventeen gametypes** including Duel, Domination, Clan Arena, Freeze Tag, and more.
- **Game modifiers** such as Vampiric Damage, Marathon Mode, and Gravity Lotto to remix match flow.
- **Hosting and server tools** with match presets, tournament mode for scheduled events, and a powerful map management system featuring pools, voting, and rotation logic.
- **Competitive infrastructure** covering match state progression and recovery, extended administrative and match management commands, server-side client configuration storage, and an Auto-Ghost system that lets disconnected players rejoin with their score, inventory, and stats restored.

## Interface & Presentation Improvements

WORR's presentation layer is crafted to convey more information without sacrificing speed:

- A sleek yet informative adaptive HUD tailored to each gametype.
- Integrated mini-score display and frag messages directly in the UI.
- Dynamic match announcer feedback.
- Automatic application of QUAKE and QUAKE III: ARENA-style rulesets when maps support them.

## Release Channels

WORR ships through several release channels so you can balance stability with early access:

- **Stable** — Fully vetted releases recommended for production servers, with only critical hotfixes between major updates.
- **Release Candidate (RC)** — Final verification builds that preview the next stable release and suit staging environments.
- **Beta** — Feature-complete builds under active testing for enthusiasts comfortable with occasional regressions.
- **Nightly** — Automated snapshots of ongoing development aimed at contributors validating the latest fixes.

See [docs/versioning.md](../versioning.md) for full versioning policies, upgrade guidance, and support timelines.
