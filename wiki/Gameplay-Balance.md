# Gameplay Balance

This page consolidates the live gameplay balance in WORR-KEX and compares key values with the original Quake II rerelease codebase. It draws on the design outline in `docs/gameplay.html` and cross-references the current implementation to highlight where WORR diverges from id Software's defaults.

## Weapon tuning

### Rocket Launcher
- WORR fixes rocket damage at 100 with a 100-unit splash radius and increases projectile speed to 800 UPS by default (1000 in Quake 1 rulesets and 900 under Quake 3 rules).【F:src/server/player/p_weapon.cpp†L1611-L1645】 In Quake II the rocket dealt 100–120 variable damage, used a 120-unit splash radius, and travelled at 650 UPS.【F:refs/quake2-rerelease-dll-main/original/baseq2/p_weapon.c†L759-L784】

### Plasma Beam
- In deathmatch the plasma beam now inflicts 8 damage with 8 points of knockback, while single-player keeps the 15-damage baseline (30 in Quake 1 rulesets). Knockback scales with the chosen damage value.【F:src/server/player/p_weapon.cpp†L3022-L3071】 The beam length is capped at 768 units (600 in Quake 1 rulesets).【F:src/server/gameplay/g_weapon.cpp†L1394-L1413】 Originally the Rogue “heatbeam” dealt 15 damage in deathmatch with 75 points of knockback and effectively unlimited range until a collision.【F:refs/quake2-rerelease-dll-main/original/rogue/p_weapon.c†L2133-L2170】

### Hunter & Vengeance spheres
- Hunter, Vengeance, and Defender spheres now expire after 10 seconds instead of 30.【F:src/server/gameplay/g_items.cpp†L442-L575】【F:refs/quake2-rerelease-dll-main/original/rogue/g_sphere.c†L13-L120】
- The hunter sphere’s chase speed is reduced to 300 UPS (down from 500), although it still surges to 1000 UPS during its final attack dash.【F:src/server/gameplay/g_items.cpp†L524-L572】【F:refs/quake2-rerelease-dll-main/original/rogue/g_sphere.c†L121-L206】 This curbs pursuit strength while keeping the high-speed finishing burst.

### Hitscan spread (machinegun, chaingun, shotgun)
- WORR raises the default bullet spread cone to 500 units horizontally and vertically, replacing Quake II’s 300×500 box spread. Shotgun constants remain 1000×500, matching the legacy values.【F:src/server/g_local.hpp†L3740-L3743】【F:refs/quake2-rerelease-dll-main/original/baseq2/g_local.h†L668-L674】
- The machinegun continues to fire eight-damage shots but now uses the wider spread, with Quake 3 rulesets reducing damage and narrowing spread to 200×200 for tighter duels.【F:src/server/player/p_weapon.cpp†L2150-L2205】

### Haste fire-rate scaling
- Weapon animation timing multiplies the base gun rate by 1.5 whenever Haste is active, delivering the advertised 50 % fire-rate boost.【F:src/server/player/p_weapon.cpp†L630-L651】

## Item economy

### Ammo capacities and pickups

| Ammo | WORR max (base / bandolier / pack) | Quake II max (base / bandolier / pack) | Pickup amounts (ammo / weapon / bandolier / pack) |
| --- | --- | --- | --- |
| Bullets | 200 / 250 / 300【F:src/server/g_local.hpp†L1263-L1274】 | 200 / 250 / 300【F:refs/quake2-rerelease-dll-main/original/baseq2/p_client.c†L619-L627】【F:refs/quake2-rerelease-dll-main/original/baseq2/g_items.c†L221-L332】 | 50 / 50 / 50 / 30【F:src/server/g_local.hpp†L1263-L1274】【F:refs/quake2-rerelease-dll-main/original/baseq2/g_items.c†L1546-L1658】 |
| Shells | 50 / 100 / 150【F:src/server/g_local.hpp†L1263-L1274】 | 100 / 150 / 200【F:refs/quake2-rerelease-dll-main/original/baseq2/p_client.c†L619-L627】【F:refs/quake2-rerelease-dll-main/original/baseq2/g_items.c†L221-L332】 | 10 / 10 / 20 / 10【F:src/server/g_local.hpp†L1263-L1274】【F:refs/quake2-rerelease-dll-main/original/baseq2/g_items.c†L1546-L1658】 |
| Rockets | 25 / 25 / 50【F:src/server/g_local.hpp†L1263-L1274】 | 50 / 50 / 100【F:refs/quake2-rerelease-dll-main/original/baseq2/p_client.c†L619-L627】【F:refs/quake2-rerelease-dll-main/original/baseq2/g_items.c†L260-L332】 | 10 / 5 / 0 / 5【F:src/server/g_local.hpp†L1263-L1274】【F:refs/quake2-rerelease-dll-main/original/baseq2/g_items.c†L1546-L1658】 |
| Grenades | 25 / 25 / 50【F:src/server/g_local.hpp†L1263-L1274】 | 50 / 50 / 100【F:refs/quake2-rerelease-dll-main/original/baseq2/p_client.c†L619-L627】【F:refs/quake2-rerelease-dll-main/original/baseq2/g_items.c†L260-L332】 | 10 / 5 / 0 / 5【F:src/server/g_local.hpp†L1263-L1274】【F:refs/quake2-rerelease-dll-main/original/baseq2/g_items.c†L1546-L1658】 |
| Cells | 200 / 200 / 300【F:src/server/g_local.hpp†L1263-L1274】 | 200 / 250 / 300【F:refs/quake2-rerelease-dll-main/original/baseq2/p_client.c†L619-L627】【F:refs/quake2-rerelease-dll-main/original/baseq2/g_items.c†L221-L332】 | 50 / 50 / 0 / 30【F:src/server/g_local.hpp†L1263-L1274】【F:refs/quake2-rerelease-dll-main/original/baseq2/g_items.c†L1546-L1658】 |
| Slugs | 25 / 25 / 50【F:src/server/g_local.hpp†L1263-L1274】 | 50 / 75 / 100【F:refs/quake2-rerelease-dll-main/original/baseq2/p_client.c†L619-L627】【F:refs/quake2-rerelease-dll-main/original/baseq2/g_items.c†L221-L332】 | 10 / 5 / 0 / 5【F:src/server/g_local.hpp†L1263-L1274】【F:refs/quake2-rerelease-dll-main/original/baseq2/g_items.c†L1546-L1658】 |

WORR therefore cuts rocket, grenade, and slug stockpiles roughly in half while leaving pickup quantities unchanged, tightening explosive and precision ammo management.

Bandoliers and ammo packs still grant the bonus ammo listed above but now respawn faster (30 s for the bandolier, 90 s for the ammo pack).【F:src/server/gameplay/g_item_list.cpp†L1545-L1584】 Bandolier and pack pickups continue to adjust the player’s maximum ammo through the shared `ammoStats` table.【F:src/server/gameplay/g_items.cpp†L2558-L2614】

### Spawn health and regeneration
- Deathmatch spawns grant a configurable health bonus (25 by default, or the Quake 3 value under RS_Q3A). The bonus is tracked in `healthBonus` and decays at one point per second until the player returns to their nominal maximum.【F:src/server/player/p_client.cpp†L2036-L2043】【F:src/server/player/p_client.cpp†L4118-L4139】
- Mega Health pickups run a timer that persists across multiple grabs; the item ticks health back toward the stored threshold before respawning.【F:src/server/gameplay/g_items.cpp†L3078-L3093】
- Adrenaline is a reusable holdable in deathmatch, granting +5 to max health (and +1 elsewhere).【F:src/server/gameplay/g_items.cpp†L2421-L2433】

### Power armor
- WORR equalizes power armor absorption: both power screen and shield consume one cell per point of prevented damage in deathmatch.【F:src/server/gameplay/g_combat.cpp†L178-L235】 In Quake II, the shield spent two cells per point, making it substantially weaker.【F:refs/quake2-rerelease-dll-main/original/baseq2/g_combat.c†L171-L235】

### Powerups
- All major powerups remain droppable on death when the server enables powerup drops.【F:src/server/player/p_client.cpp†L970-L1031】
- Picking up a powerup in deathmatch schedules a 120-second respawn unless it was a dropped item; initial map spawns hide powerups for a random 30–60 seconds to stagger control routes.【F:src/server/gameplay/g_items.cpp†L2040-L2135】【F:src/server/gameplay/g_items.cpp†L3806-L3812】
- Using Haste, Battle Suit, Double Damage, Regeneration, or related powerups consumes the item and grants 30 seconds of effect by default (or the remaining drop timeout).【F:src/server/gameplay/g_items.cpp†L2700-L2838】 Combined with the 1.5× gun-rate multiplier above, Haste materially accelerates combat pacing.

## Player spawning and the combat heatmap

- Every time damage is applied the combat heatmap records a weighted event and decays over time. `HM_AddEvent` maps each hit into 256-unit cells with a cosine falloff, while `HM_Query` and `HM_DangerAt` aggregate danger scores in a configurable radius.【F:src/server/gameplay/g_combat.cpp†L709-L718】【F:src/server/gameplay/g_combat_heatmap.cpp†L131-L209】【F:src/server/gameplay/g_combat_heatmap.cpp†L252-L266】
- The system initializes when the game starts and resets whenever a new map loads.【F:src/server/gameplay/g_main.cpp†L973-L974】【F:src/server/gameplay/g_spawn.cpp†L2576-L2577】
- Spawn selection blends the heatmap with several live factors. `CompositeDangerScore` weights recent combat (50 %), enemy line-of-sight (20 %), proximity to other players (15 %), the player’s death spot (10 %), and nearby mines (5 %) to score each spawn.【F:src/server/gameplay/g_spawn_points.cpp†L591-L616】 `SelectDeathmatchSpawnPoint` shuffles initial lists, filters unusable spots, falls back to team and classic spawn points when needed, and finally picks the lowest-danger candidate within a tolerance band.【F:src/server/gameplay/g_spawn_points.cpp†L660-L724】 This process delivers the heatmap-driven spawn behaviour described in the gameplay brief.

