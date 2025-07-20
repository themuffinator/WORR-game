// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_weapon.c

#include "g_local.h"
#include "monsters/m_player.h"

bool is_quad;
bool is_haste;
player_muzzle_t is_silenced;
uint8_t damageMultiplier;

/*
================
InfiniteAmmoOn
================
*/
bool InfiniteAmmoOn(Item *item) {
	if (item && item->flags & IF_NO_INFINITE_AMMO)
		return false;

	return g_infiniteAmmo->integer || (deathmatch->integer && (g_instagib->integer || g_nadefest->integer));
}

/*
================
PlayerDamageModifier
================
*/
uint8_t PlayerDamageModifier(gentity_t *ent) {
	is_quad = false;
	damageMultiplier = 0;

	//WOR: make these stack but additive rather than multiplicative

	if (ent->client->powerupTime.quadDamage > level.time) {
		damageMultiplier += 4;
		is_quad = true;
	}

	if (ent->client->powerupTime.doubleDamage > level.time) {
		damageMultiplier += 2;
		is_quad = true;
	}

	if (ent->client->pers.inventory[IT_TECH_POWER_AMP])
		damageMultiplier += 2;

	damageMultiplier = std::max<uint8_t>(1, damageMultiplier);
	return damageMultiplier;
}

/*
================
P_CurrentKickFactor

[Paril-KEX] kicks in vanilla take place over 2 10hz server
frames; this is to mimic that visual behavior on any tickrate.
================
*/

static inline float P_CurrentKickFactor(gentity_t *ent) {
	if (ent->client->kick.time < level.time)
		return 0.f;

	float f = (ent->client->kick.time - level.time).seconds() / ent->client->kick.total.seconds();
	return f;
}

/*
================
P_CurrentKickAngles
================
*/
vec3_t P_CurrentKickAngles(gentity_t *ent) {
	return ent->client->kick.angles * P_CurrentKickFactor(ent);
}

/*
================
P_CurrentKickOrigin
================
*/
vec3_t P_CurrentKickOrigin(gentity_t *ent) {
	return ent->client->kick.origin * P_CurrentKickFactor(ent);
}

/*
================
P_AddWeaponKick
================
*/
void P_AddWeaponKick(gentity_t *ent, const vec3_t &origin, const vec3_t &angles) {
	ent->client->kick.origin = origin;
	ent->client->kick.angles = angles;
	ent->client->kick.total = 200_ms;
	ent->client->kick.time = level.time + ent->client->kick.total;
}

/*
================
P_ProjectSource

Projects the weapon muzzle position and direction from the player's view.
================
*/
void P_ProjectSource(gentity_t *ent, const vec3_t &angles, vec3_t distance, vec3_t &result_start, vec3_t &result_dir) {
	// Adjust distance based on projection settings or handedness
	if (g_weapon_projection->integer > 0) {
		distance[1] = 0;
		if (g_weapon_projection->integer > 1)
			distance[2] = 0;
	} else {
		switch (ent->client->pers.hand) {
		case LEFT_HANDED:   distance[1] *= -1; break;
		case CENTER_HANDED: distance[1] = 0;   break;
		default: break;
		}
	}

	vec3_t forward, right, up;
	AngleVectors(angles, forward, right, up);

	vec3_t eye_pos = ent->s.origin + vec3_t{ 0, 0, static_cast<float>(ent->viewHeight) };
	result_start = G_ProjectSource2(eye_pos, distance, forward, right, up);

	vec3_t end = eye_pos + forward * 8192.0f;

	contents_t mask = MASK_PROJECTILE & ~CONTENTS_DEADMONSTER;
	if (!G_ShouldPlayersCollide(true))
		mask &= ~CONTENTS_PLAYER;

	trace_t tr = gi.traceline(eye_pos, end, ent, mask);

	bool close_to_target = (tr.fraction * 8192.0f) < 128.0f;
	bool hit_entity = tr.startsolid || (tr.contents & (CONTENTS_MONSTER | CONTENTS_PLAYER));

	// Use raw forward if we hit something close (e.g., monster/player)
	if (hit_entity && close_to_target)
		result_dir = forward;
	else
		result_dir = (tr.endpos - result_start).normalized();
}

/*
===============
PlayerNoise

Each player can have two noise objects:
- mynoise: personal sounds (jumping, pain, firing)
- mynoise2: impact sounds (bullet wall impacts)

These allow AI to move toward noise origins to locate players.
===============
*/
void PlayerNoise(gentity_t *who, const vec3_t &where, player_noise_t type) {
	if (type == PNOISE_WEAPON) {
		if (who->client->powerupTime.silencerShots) {
			who->client->invisibility_fade_time = level.time + (INVISIBILITY_TIME / 5);
			who->client->powerupTime.silencerShots--;
			return;
		}

		who->client->invisibility_fade_time = level.time + INVISIBILITY_TIME;

		if (who->client->powerupTime.spawnProtection > level.time)
			who->client->powerupTime.spawnProtection = 0_sec;
	}

	if (deathmatch->integer || (who->flags & FL_NOTARGET))
		return;

	if (type == PNOISE_SELF && (who->client->landmark_free_fall || who->client->landmark_noise_time >= level.time))
		return;

	if (who->flags & FL_DISGUISED) {
		if (type == PNOISE_WEAPON) {
			level.campaign.disguiseViolator = who;
			level.campaign.disguiseViolationTime = level.time + 500_ms;
		}
		return;
	}

	// Create noise entities if not yet created
	if (!who->mynoise) {
		auto create_noise = [who]() {
			gentity_t *noise = Spawn();
			noise->className = "player_noise";
			noise->mins = { -8, -8, -8 };
			noise->maxs = { 8, 8, 8 };
			noise->owner = who;
			noise->svFlags = SVF_NOCLIENT;
			return noise;
			};

		who->mynoise = create_noise();
		who->mynoise2 = create_noise();
	}

	// Select appropriate noise entity
	gentity_t *noise = (type == PNOISE_SELF || type == PNOISE_WEAPON) ? who->mynoise : who->mynoise2;

	// Update client's sound entity refs
	if (type == PNOISE_SELF || type == PNOISE_WEAPON) {
		who->client->sound_entity = noise;
		who->client->sound_entity_time = level.time;
	} else {
		who->client->sound2_entity = noise;
		who->client->sound2_entity_time = level.time;
	}

	// Position and activate noise entity
	noise->s.origin = where;
	noise->absMin = where - noise->maxs;
	noise->absMax = where + noise->maxs;
	noise->teleport_time = level.time;

	gi.linkentity(noise);
}

/*
================
G_WeaponShouldStay
================
*/
static inline bool G_WeaponShouldStay() {
	if (deathmatch->integer)
		return match_weaponsStay->integer;
	else if (coop->integer)
		return !P_UseCoopInstancedItems();

	return false;
}

/*
================
Pickup_Weapon
================
*/
void G_CheckAutoSwitch(gentity_t *ent, Item *item, bool is_new);

bool Pickup_Weapon(gentity_t *ent, gentity_t *other) {
	const item_id_t index = ent->item->id;

	// Respect weapon stay logic unless the weapon was dropped
	if (G_WeaponShouldStay() && other->client->pers.inventory[index]) {
		if (!(ent->spawnflags & (SPAWNFLAG_ITEM_DROPPED | SPAWNFLAG_ITEM_DROPPED_PLAYER)))
			return false;
	}

	const bool is_new = !other->client->pers.inventory[index];

	// Only give ammo if not a dropped player weapon or count is specified
	if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED) || ent->count) {
		if (ent->item->ammo) {
			Item *ammo = GetItemByIndex(ent->item->ammo);
			if (InfiniteAmmoOn(ammo)) {
				Add_Ammo(other, ammo, AMMO_INFINITE);
			} else {
				int count = 0;

				if (RS(RS_Q3A)) {
					count = ent->count ? ent->count : (
						ammo->id == IT_AMMO_GRENADES ||
						ammo->id == IT_AMMO_ROCKETS ||
						ammo->id == IT_AMMO_SLUGS ? 10 : ammo->quantity
						);

					if (other->client->pers.inventory[ammo->id] < count)
						count -= other->client->pers.inventory[ammo->id];
					else
						count = 1;

				} else {
					if (ammo && InfiniteAmmoOn(ent->item)) {
						count = AMMO_INFINITE;
					} else if (ent->count) {
						count = ent->count;
					} else if (ammo->id == IT_AMMO_SLUGS) {
						switch (game.ruleset) {
						case RS_Q1:
							count = 1;
							break;
						case RS_Q3A:
							count = 10;
							break;
						default:
							count = 8;
							break;
						}
					} else {
						count = ammo->quantity;
					}
				}

				Add_Ammo(other, ammo, count);
			}
		}

		// Handle respawn logic
		if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED_PLAYER)) {
			if (deathmatch->integer) {
				if (match_weaponsStay->integer)
					ent->flags |= FL_RESPAWN;

				SetRespawn(ent, gtime_t::from_sec(g_weapon_respawn_time->integer), !match_weaponsStay->integer);
			}
			if (coop->integer)
				ent->flags |= FL_RESPAWN;
		}
	}

	// Increment inventory and consider auto-switch
	other->client->pers.inventory[index]++;
	G_CheckAutoSwitch(other, ent->item, is_new);

	return true;
}

/*
================
Weapon_RunThink
================
*/
static void Weapon_RunThink(gentity_t *ent) {
	// call active weapon think routine
	if (!ent->client->pers.weapon->weaponthink)
		return;

	PlayerDamageModifier(ent);

	is_haste = (ent->client->powerupTime.haste > level.time);

	if (ent->client->powerupTime.silencerShots)
		is_silenced = MZ_SILENCED;
	else
		is_silenced = MZ_NONE;
	ent->client->pers.weapon->weaponthink(ent);
}

/*
===============
Change_Weapon

The old weapon has been fully holstered; equip the new one.
===============
*/
void Change_Weapon(gentity_t *ent) {
	auto *client = ent->client;

	// Don't allow holstering unless switching is instant or in frenzy mode
	if (ent->health > 0 && !g_instantWeaponSwitch->integer && !g_frenzy->integer &&
		((client->latchedButtons | client->buttons) & BUTTON_HOLSTER)) {
		return;
	}

	// Drop held grenade if active
	if (client->grenadeTime) {
		client->weaponSound = 0;
		Weapon_RunThink(ent);
		client->grenadeTime = 0_ms;
	}

	if (client->pers.weapon) {
		client->pers.lastWeapon = client->pers.weapon;

		// Play switch sound only when changing weapons and quick switch enabled
		if (client->newWeapon && client->newWeapon != client->pers.weapon) {
			if (g_quick_weapon_switch->integer || g_instantWeaponSwitch->integer)
				gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/change.wav"), 1, ATTN_NORM, 0);
		}
	}

	client->pers.weapon = client->newWeapon;
	client->newWeapon = nullptr;

	// Update model skin if applicable
	if (ent->s.modelindex == MODELINDEX_PLAYER)
		P_AssignClientSkinnum(ent);

	if (!client->pers.weapon) {
		// No weapon: hide model
		client->ps.gunIndex = 0;
		client->ps.gunSkin = 0;
		return;
	}

	// Begin weapon animation
	client->weaponState = WEAPON_ACTIVATING;
	client->ps.gunframe = 0;
	client->ps.gunIndex = gi.modelindex(client->pers.weapon->view_model);
	client->ps.gunSkin = 0;
	client->weaponSound = 0;

	// Apply transition animation
	client->anim.priority = ANIM_PAIN;
	if (client->ps.pmove.pm_flags & PMF_DUCKED) {
		ent->s.frame = FRAME_crpain1;
		client->anim.end = FRAME_crpain4;
	} else {
		ent->s.frame = FRAME_pain301;
		client->anim.end = FRAME_pain304;
	}
	client->anim.time = 0_ms;

	// Apply immediate think if switching is instant
	if (g_instantWeaponSwitch->integer || g_frenzy->integer)
		Weapon_RunThink(ent);
}

constexpr item_id_t weaponPriorityList[] = {
	IT_WEAPON_DISRUPTOR,
	IT_WEAPON_BFG,
	IT_WEAPON_RAILGUN,
	IT_WEAPON_PLASMABEAM,
	IT_WEAPON_IONRIPPER,
	IT_WEAPON_HYPERBLASTER,
	IT_WEAPON_ETF_RIFLE,
	IT_WEAPON_CHAINGUN,
	IT_WEAPON_MACHINEGUN,
	IT_WEAPON_SSHOTGUN,
	IT_WEAPON_SHOTGUN,
	IT_WEAPON_PHALANX,
	IT_WEAPON_RLAUNCHER,
	IT_WEAPON_GLAUNCHER,
	IT_WEAPON_PROXLAUNCHER,
	IT_AMMO_GRENADES,
	IT_WEAPON_BLASTER,
	IT_WEAPON_CHAINFIST
};

static item_id_t weaponIndexToItemID(int weaponIndex) {
	switch (weaponIndex) {
	case WEAP_DISRUPTOR:      return IT_WEAPON_DISRUPTOR;
	case WEAP_BFG:            return IT_WEAPON_BFG;
	case WEAP_RAILGUN:        return IT_WEAPON_RAILGUN;
	case WEAP_PLASMABEAM:     return IT_WEAPON_PLASMABEAM;
	case WEAP_IONRIPPER:      return IT_WEAPON_IONRIPPER;
	case WEAP_HYPERBLASTER:   return IT_WEAPON_HYPERBLASTER;
	case WEAP_ETF_RIFLE:      return IT_WEAPON_ETF_RIFLE;
	case WEAP_CHAINGUN:       return IT_WEAPON_CHAINGUN;
	case WEAP_MACHINEGUN:     return IT_WEAPON_MACHINEGUN;
	case WEAP_SUPER_SHOTGUN:  return IT_WEAPON_SSHOTGUN;
	case WEAP_SHOTGUN:        return IT_WEAPON_SHOTGUN;
	case WEAP_PHALANX:        return IT_WEAPON_PHALANX;
	case WEAP_ROCKET_LAUNCHER:return IT_WEAPON_RLAUNCHER;
	case WEAP_GRENADE_LAUNCHER:return IT_WEAPON_GLAUNCHER;
	case WEAP_PROX_LAUNCHER:  return IT_WEAPON_PROXLAUNCHER;
	case WEAP_HAND_GRENADES:  return IT_AMMO_GRENADES;
	case WEAP_BLASTER:        return IT_WEAPON_BLASTER;
	case WEAP_CHAINFIST:      return IT_WEAPON_CHAINFIST;
	default:                  return IT_NULL;
	}
}

/*
=============
BuildEffectiveWeaponPriority
Combines client preferences with default weapon priority list.
=============
*/
static std::vector<item_id_t> BuildEffectiveWeaponPriority(gclient_t *cl) {
	std::vector<item_id_t> finalList;
	std::unordered_set<item_id_t> seen;

	// 1. Add preferred weapons first, in client-specified order
	for (const std::string &abbr : cl->sess.weaponPrefs) {
		int weaponIndex = GetWeaponIndexByAbbrev(abbr);
		if (weaponIndex == WEAP_NONE)
			continue;

		item_id_t item = weaponIndexToItemID(weaponIndex); // You'll need this lookup
		if (item && seen.find(item) == seen.end()) {
			finalList.push_back(item);
			seen.insert(item);
		}
	}

	// 2. Add all other weapons from default list, preserving order
	for (item_id_t def : weaponPriorityList) {
		if (seen.find(def) == seen.end())
			finalList.push_back(def);
	}

	return finalList;
}

/*
=============
GetWeaponPriorityIndex
Returns effective priority index for a weapon based on client preference.
Lower index = higher priority.
=============
*/
static int GetWeaponPriorityIndex(gclient_t *cl, const std::string &abbr) {
	std::string upperAbbr = abbr;
	std::transform(upperAbbr.begin(), upperAbbr.end(), upperAbbr.begin(), ::toupper);

	// First: check client preference list
	auto &prefs = cl->sess.weaponPrefs;
	for (size_t i = 0; i < prefs.size(); ++i) {
		std::string pref = prefs[i];
		std::transform(pref.begin(), pref.end(), pref.begin(), ::toupper);
		if (pref == upperAbbr)
			return static_cast<int>(i); // higher priority
	}

	// Then: fall back to default priority list
	int weaponIndex = GetWeaponIndexByAbbrev(upperAbbr);
	if (weaponIndex == WEAP_NONE)
		return 9999; // unknown weapon = lowest priority

	item_id_t item = weaponIndexToItemID(weaponIndex);

	for (size_t i = 0; i < sizeof(weaponPriorityList) / sizeof(weaponPriorityList[0]); ++i) {
		if (weaponPriorityList[i] == item)
			return static_cast<int>(100 + i); // below any client-listed
	}

	return 9999; // not in known weapon list
}

/*
=================
NoAmmoWeaponChange

Automatically switches to the next available weapon when out of ammo.
Optionally plays a "click" sound indicating no ammo.
=================
*/
void NoAmmoWeaponChange(gentity_t *ent, bool playSound) {
	auto *client = ent->client;

	if (playSound && level.time >= client->emptyClickSound) {
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
		client->emptyClickSound = level.time + 1_sec;
	}

	constexpr std::array<item_id_t, 18> fallbackWeapons = {
		IT_WEAPON_DISRUPTOR,
		IT_WEAPON_BFG,
		IT_WEAPON_RAILGUN,
		IT_WEAPON_PLASMABEAM,
		IT_WEAPON_IONRIPPER,
		IT_WEAPON_HYPERBLASTER,
		IT_WEAPON_ETF_RIFLE,
		IT_WEAPON_CHAINGUN,
		IT_WEAPON_MACHINEGUN,
		IT_WEAPON_SSHOTGUN,
		IT_WEAPON_SHOTGUN,
		IT_WEAPON_PHALANX,
		IT_WEAPON_RLAUNCHER,
		IT_WEAPON_GLAUNCHER,
		IT_WEAPON_PROXLAUNCHER,
		IT_AMMO_GRENADES,
		IT_WEAPON_BLASTER,
		IT_WEAPON_CHAINFIST
	};

	for (item_id_t id : fallbackWeapons) {
		Item *item = GetItemByIndex(id);
		if (!item) {
			gi.Com_ErrorFmt("Invalid fallback weapon ID: {}\n", static_cast<int32_t>(id));
			continue;
		}

		if (client->pers.inventory[item->id] <= 0)
			continue;

		if (item->ammo && client->pers.inventory[item->ammo] < item->quantity)
			continue;

		client->newWeapon = item;
		return;
	}
}

/*
================
RemoveAmmo

Reduces the player's ammo count for their current weapon.
Triggers a low ammo warning sound if the threshold is crossed.
================
*/
static void RemoveAmmo(gentity_t *ent, int32_t quantity) {
	auto *cl = ent->client;
	auto *weapon = cl->pers.weapon;

	if (InfiniteAmmoOn(weapon))
		return;

	const int ammoIndex = weapon->ammo;
	const int threshold = weapon->quantity_warn;
	int &ammoCount = cl->pers.inventory[ammoIndex];

	const bool wasAboveWarning = ammoCount > threshold;

	ammoCount -= quantity;

	if (wasAboveWarning && ammoCount <= threshold) {
		gi.local_sound(ent, CHAN_AUTO, gi.soundindex("weapons/lowammo.wav"), 1, ATTN_NORM, 0);
	}

	CheckPowerArmorState(ent);
}

/*
================
Weapon_AnimationTime

Determines the duration of one weapon animation frame based on modifiers
such as quick switching, haste, time acceleration, and frenzy mode.
================
*/
static inline gtime_t Weapon_AnimationTime(gentity_t *ent) {
	auto &cl = *ent->client;
	auto &ps = cl.ps;

	// Determine base gunrate
	if ((g_quick_weapon_switch->integer || g_frenzy->integer) && gi.tick_rate >= 20 &&
		(cl.weaponState == WEAPON_ACTIVATING || cl.weaponState == WEAPON_DROPPING)) {
		ps.gunrate = 20;
	} else {
		ps.gunrate = 10;
	}

	// Apply haste and modifiers if allowed
	if (ps.gunframe != 0 && (!(cl.pers.weapon->flags & IF_NO_HASTE) || cl.weaponState != WEAPON_FIRING)) {
		if (is_haste)
			ps.gunrate *= 1.5f;
		if (Tech_ApplyTimeAccel(ent))
			ps.gunrate *= 2.0f;
		if (g_frenzy->integer)
			ps.gunrate *= 2.0f;
	}

	// Optimization: encode default rate as 0 for networking
	if (ps.gunrate == 10) {
		ps.gunrate = 0;
		return 100_ms;
	}

	const float ms = (1.0f / ps.gunrate) * 1000.0f;
	return gtime_t::from_ms(ms);
}

/*
=================
Think_Weapon

Called by ClientBeginServerFrame and ClientThink.
Handles weapon logic including death handling, animation timing,
and compensating for low tick-rate overflows.
=================
*/
void Think_Weapon(gentity_t *ent) {
	if (!ClientIsPlaying(ent->client) || ent->client->eliminated)
		return;

	// Put away weapon if dead
	if (ent->health < 1) {
		ent->client->newWeapon = nullptr;
		Change_Weapon(ent);
	}

	// If no active weapon, try switching
	if (!ent->client->pers.weapon) {
		if (ent->client->newWeapon)
			Change_Weapon(ent);
		return;
	}

	// Run the current weapon's think logic
	Weapon_RunThink(ent);

	// Compensate for missed animations due to fast tick rate (e.g. 33ms vs 50ms)
	if (33_ms < FRAME_TIME_MS) {
		const gtime_t animTime = Weapon_AnimationTime(ent);

		if (animTime < FRAME_TIME_MS) {
			const gtime_t nextFrameTime = level.time + FRAME_TIME_S;
			int64_t overrunMs = (nextFrameTime - ent->client->weaponThinkTime).milliseconds();

			while (overrunMs > 0) {
				ent->client->weaponThinkTime -= animTime;
				ent->client->weaponFireFinished -= animTime;
				Weapon_RunThink(ent);
				overrunMs -= animTime.milliseconds();
			}
		}
	}
}

/*
================
Weapon_AttemptSwitch
================
*/
enum WeaponSwitch {
	WEAP_SWITCH_ALREADY_USING,
	WEAP_SWITCH_NO_WEAPON,
	WEAP_SWITCH_NO_AMMO,
	WEAP_SWITCH_NOT_ENOUGH_AMMO,
	WEAP_SWITCH_VALID
};

/*
================
Weapon_AttemptSwitch

Checks whether a weapon can be switched to, considering inventory and ammo.
================
*/
static WeaponSwitch Weapon_AttemptSwitch(gentity_t *ent, Item *item, bool silent) {
	if (!item)
		return WEAP_SWITCH_NO_WEAPON;

	if (ent->client->pers.weapon == item)
		return WEAP_SWITCH_ALREADY_USING;

	if (ent->client->pers.inventory[item->id] < 1)
		return WEAP_SWITCH_NO_WEAPON;

	const bool requiresAmmo = item->ammo && !g_select_empty->integer && !(item->flags & IF_AMMO);

	if (requiresAmmo) {
		Item *ammoItem = GetItemByIndex(item->ammo);
		const int ammoCount = ent->client->pers.inventory[item->ammo];

		if (ammoCount <= 0) {
			if (!silent && ammoItem)
				gi.LocClient_Print(ent, PRINT_HIGH, "$g_no_ammo", ammoItem->pickup_name, item->pickup_name_definite);
			return WEAP_SWITCH_NO_AMMO;
		}

		if (ammoCount < item->quantity) {
			if (!silent && ammoItem)
				gi.LocClient_Print(ent, PRINT_HIGH, "$g_not_enough_ammo", ammoItem->pickup_name, item->pickup_name_definite);
			return WEAP_SWITCH_NOT_ENOUGH_AMMO;
		}
	}

	return WEAP_SWITCH_VALID;
}

static inline bool Weapon_IsPartOfChain(Item *item, Item *other) {
	return other && other->chain && item->chain && other->chain == item->chain;
}

/*
================
Use_Weapon

Make the weapon ready if there is ammo
================
*/
void Use_Weapon(gentity_t *ent, Item *item) {
	if (!ent || !ent->client || !item)
		return;

	Item *wanted = nullptr;
	Item *root = nullptr;
	WeaponSwitch result = WEAP_SWITCH_NO_WEAPON;

	const bool noChains = ent->client->noWeaponChains;

	// Determine starting point in weapon chain
	if (!noChains && Weapon_IsPartOfChain(item, ent->client->newWeapon)) {
		root = ent->client->newWeapon;
		wanted = root->chainNext;
	} else if (!noChains && Weapon_IsPartOfChain(item, ent->client->pers.weapon)) {
		root = ent->client->pers.weapon;
		wanted = root->chainNext;
	} else {
		root = item;
		wanted = item;
	}

	while (true) {
		result = Weapon_AttemptSwitch(ent, wanted, /*force=*/false);
		if (result == WEAP_SWITCH_VALID)
			break;

		if (noChains || !wanted || !wanted->chainNext)
			break;

		wanted = wanted->chainNext;
		if (wanted == root)
			break;
	}

	if (result == WEAP_SWITCH_VALID) {
		ent->client->newWeapon = wanted;
	} else if ((result = Weapon_AttemptSwitch(ent, wanted, /*force=*/true)) == WEAP_SWITCH_NO_WEAPON) {
		// Only print warning if it wasn't already the active or pending weapon
		if (wanted != ent->client->pers.weapon && wanted != ent->client->newWeapon)
			gi.LocClient_Print(ent, PRINT_HIGH, "$g_out_of_item", wanted->pickup_name);
	}
}

/*
================
Drop_Weapon
================
*/
void Drop_Weapon(gentity_t *ent, Item *item) {
	if (!ent || !ent->client || !item)
		return;

	if (deathmatch->integer && match_weaponsStay->integer)
		return;

	const item_id_t itemId = item->id;

	if (ent->client->pers.inventory[itemId] < 1)
		return;

	gentity_t *drop = Drop_Item(ent, item);
	if (!drop)
		return;

	drop->spawnflags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
	drop->svFlags &= ~SVF_INSTANCED;

	// Weapons that need to drop ammo with them
	if (itemId != IT_WEAPON_BLASTER && itemId != IT_WEAPON_GRAPPLE && itemId != IT_WEAPON_CHAINFIST) {
		Item *ammo = GetItemByIndex(drop->item->ammo);
		if (!ammo) {
			FreeEntity(drop);
			return;
		}

		const int ammoId = ammo->id;
		const int playerAmmo = ent->client->pers.inventory[ammoId];

		if (playerAmmo <= 0) {
			FreeEntity(drop);
			return;
		}

		int ammoCount = ammo->quantity;

		if (itemId == IT_WEAPON_RAILGUN)
			ammoCount += 5;
		else if (itemId == IT_WEAPON_BLASTER)
			ammoCount = AMMO_INFINITE;

		ammoCount = clamp(ammoCount, ammoCount, playerAmmo);

		if (ammoCount <= 0 || playerAmmo - ammoCount < 0) {
			FreeEntity(drop);
			return;
		}

		drop->count = ammoCount;

		if (ammoCount != AMMO_INFINITE)
			Add_Ammo(ent, ammo, -ammoCount);

		// Auto-switch weapon if we were using it and now out
		if ((item == ent->client->pers.weapon || item == ent->client->newWeapon) &&
			(ent->client->pers.inventory[itemId] < 1 || ent->client->pers.inventory[ammoId] < 1)) {
			NoAmmoWeaponChange(ent, true);
		}
	} else {
		drop->count = AMMO_INFINITE;

		// Auto-switch weapon if we were using it and have none left
		if ((item == ent->client->pers.weapon || item == ent->client->newWeapon) &&
			ent->client->pers.inventory[itemId] == 1) {
			NoAmmoWeaponChange(ent, true);
		}
	}

	ent->client->pers.inventory[itemId]--;
}

/*
=========================
Weapon_PowerupSound
=========================
*/
void Weapon_PowerupSound(gentity_t *ent) {
	if (!ent || !ent->client)
		return;

	auto *cl = ent->client;

	// Attempt to play power amp sound first
	if (!Tech_ApplyPowerAmpSound(ent)) {
		const bool hasQuad = cl->powerupTime.quadDamage > level.time;
		const bool hasDouble = cl->powerupTime.doubleDamage > level.time;
		const bool hasHaste = cl->powerupTime.haste > level.time;
		const bool canPlayHasteSound = cl->techSoundTime < level.time;

		if (hasQuad && hasDouble) {
			gi.sound(ent, CHAN_ITEM, gi.soundindex("ctf/tech2x.wav"), 1, ATTN_NORM, 0);
		} else if (hasQuad) {
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);
		} else if (hasDouble) {
			gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/ddamage3.wav"), 1, ATTN_NORM, 0);
		} else if (hasHaste && canPlayHasteSound) {
			cl->techSoundTime = level.time + 1_sec;
			gi.sound(ent, CHAN_ITEM, gi.soundindex("ctf/tech3.wav"), 1, ATTN_NORM, 0);
		}
	}

	Tech_ApplyTimeAccelSound(ent);
}

/*
================
Weapon_CanAnimate
================
*/
static inline bool Weapon_CanAnimate(gentity_t *ent) {
	// VWep animations screw up corpses
	return !ent->deadFlag && ent->s.modelindex == MODELINDEX_PLAYER;
}

/*
================
Weapon_SetFinished

[Paril-KEX] called when finished to set time until
we're allowed to switch to fire again
================
*/
static inline void Weapon_SetFinished(gentity_t *ent) {
	ent->client->weaponFireFinished = level.time + Weapon_AnimationTime(ent);
}

/*
===========================
Weapon_HandleDropping
===========================
*/
static inline bool Weapon_HandleDropping(gentity_t *ent, int FRAME_DEACTIVATE_LAST) {
	if (!ent || !ent->client)
		return false;

	auto *cl = ent->client;

	if (cl->weaponState != WEAPON_DROPPING)
		return false;

	if (cl->weaponThinkTime > level.time)
		return true;

	if (cl->ps.gunframe == FRAME_DEACTIVATE_LAST) {
		Change_Weapon(ent);
		return true;
	}

	// Trigger reversed pain animation for short deactivate sequences
	if ((FRAME_DEACTIVATE_LAST - cl->ps.gunframe) == 4) {
		cl->anim.priority = ANIM_ATTACK | ANIM_REVERSED;

		if (cl->ps.pmove.pm_flags & PMF_DUCKED) {
			ent->s.frame = FRAME_crpain4 + 1;
			cl->anim.end = FRAME_crpain1;
		} else {
			ent->s.frame = FRAME_pain304 + 1;
			cl->anim.end = FRAME_pain301;
		}

		cl->anim.time = 0_ms;
	}

	cl->ps.gunframe++;
	cl->weaponThinkTime = level.time + Weapon_AnimationTime(ent);

	return true;
}

/*
===========================
Weapon_HandleActivating
===========================
*/
static inline bool Weapon_HandleActivating(gentity_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_IDLE_FIRST) {
	if (!ent || !ent->client)
		return false;

	auto *cl = ent->client;

	if (cl->weaponState != WEAPON_ACTIVATING)
		return false;

	const bool instantSwitch = g_instantWeaponSwitch->integer || g_frenzy->integer;

	if (cl->weaponThinkTime > level.time && !instantSwitch)
		return false;

	cl->weaponThinkTime = level.time + Weapon_AnimationTime(ent);

	if (cl->ps.gunframe == FRAME_ACTIVATE_LAST || instantSwitch) {
		cl->weaponState = WEAPON_READY;
		cl->ps.gunframe = FRAME_IDLE_FIRST;
		cl->weaponFireBuffered = false;

		if (!g_instantWeaponSwitch->integer || g_frenzy->integer)
			Weapon_SetFinished(ent);
		else
			cl->weaponFireFinished = 0_ms;

		return true;
	}

	cl->ps.gunframe++;
	return true;
}

/*
============================
Weapon_HandleNewWeapon
============================
*/
static inline bool Weapon_HandleNewWeapon(gentity_t *ent, int FRAME_DEACTIVATE_FIRST, int FRAME_DEACTIVATE_LAST) {
	if (!ent || !ent->client)
		return false;

	auto *cl = ent->client;
	bool isHolstering = false;

	// Determine holster intent
	if (!g_instantWeaponSwitch->integer || g_frenzy->integer) {
		isHolstering = (cl->latchedButtons | cl->buttons) & BUTTON_HOLSTER;
	}

	// Only allow weapon switch if not firing
	const bool wantsNewWeapon = (cl->newWeapon || isHolstering);
	if (!wantsNewWeapon || cl->weaponState == WEAPON_FIRING)
		return false;

	// Proceed if switch delay expired or instant switching enabled
	if (g_instantWeaponSwitch->integer || g_frenzy->integer || cl->weaponThinkTime <= level.time) {
		if (!cl->newWeapon)
			cl->newWeapon = cl->pers.weapon;

		cl->weaponState = WEAPON_DROPPING;

		// Instant switch: no animation
		if (g_instantWeaponSwitch->integer || g_frenzy->integer) {
			Change_Weapon(ent);
			return true;
		}

		cl->ps.gunframe = FRAME_DEACTIVATE_FIRST;

		// If short deactivation animation, play reversed pain animation
		if ((FRAME_DEACTIVATE_LAST - FRAME_DEACTIVATE_FIRST) < 4) {
			cl->anim.priority = ANIM_ATTACK | ANIM_REVERSED;

			if (cl->ps.pmove.pm_flags & PMF_DUCKED) {
				ent->s.frame = FRAME_crpain4 + 1;
				cl->anim.end = FRAME_crpain1;
			} else {
				ent->s.frame = FRAME_pain304 + 1;
				cl->anim.end = FRAME_pain301;
			}
			cl->anim.time = 0_ms;
		}

		cl->weaponThinkTime = level.time + Weapon_AnimationTime(ent);
		return true;
	}

	return false;
}

/*
===========================
Weapon_HandleReady
===========================
*/
enum weapon_ready_state_t {
	READY_NONE,
	READY_CHANGING,
	READY_FIRING
};

static inline weapon_ready_state_t Weapon_HandleReady(
	gentity_t *ent,
	int FRAME_FIRE_FIRST,
	int FRAME_IDLE_FIRST,
	int FRAME_IDLE_LAST,
	const int *pauseFrames
) {
	if (!ent || !ent->client || ent->client->weaponState != WEAPON_READY)
		return READY_NONE;

	auto *cl = ent->client;

	// Determine if player is trying to fire
	bool requestFiring = false;
	if (CombatIsDisabled()) {
		cl->latchedButtons &= ~BUTTON_ATTACK;
	} else {
		requestFiring = cl->weaponFireBuffered || ((cl->latchedButtons | cl->buttons) & BUTTON_ATTACK);
	}

	if (requestFiring && cl->weaponFireFinished <= level.time) {
		cl->latchedButtons &= ~BUTTON_ATTACK;
		cl->weaponThinkTime = level.time;

		// Has ammo or doesn't need it
		const int ammoIndex = cl->pers.weapon->ammo;
		const bool hasAmmo = (ammoIndex == 0) || (cl->pers.inventory[ammoIndex] >= cl->pers.weapon->quantity);

		if (hasAmmo) {
			cl->weaponState = WEAPON_FIRING;
			cl->last_firing_time = level.time + COOP_DAMAGE_FIRING_TIME;
			return READY_FIRING;
		} else {
			NoAmmoWeaponChange(ent, true);
			return READY_CHANGING;
		}
	}

	// Advance idle frames
	if (cl->weaponThinkTime <= level.time) {
		cl->weaponThinkTime = level.time + Weapon_AnimationTime(ent);

		if (cl->ps.gunframe == FRAME_IDLE_LAST) {
			cl->ps.gunframe = FRAME_IDLE_FIRST;
			return READY_CHANGING;
		}

		// Pause frames
		if (pauseFrames) {
			for (int i = 0; pauseFrames[i]; ++i) {
				if (cl->ps.gunframe == pauseFrames[i] && irandom(16))
					return READY_CHANGING;
			}
		}

		cl->ps.gunframe++;
		return READY_CHANGING;
	}

	return READY_NONE;
}

/*
=========================
Weapon_HandleFiring
=========================
*/
static inline void Weapon_HandleFiring(gentity_t *ent, int FRAME_IDLE_FIRST, const std::function<void()> &fireHandler) {
	if (!ent || !ent->client)
		return;

	auto *cl = ent->client;

	Weapon_SetFinished(ent);

	// Consume buffered fire input
	if (cl->weaponFireBuffered) {
		cl->buttons |= BUTTON_ATTACK;
		cl->weaponFireBuffered = false;

		if (cl->powerupTime.spawnProtection > level.time)
			cl->powerupTime.spawnProtection = 0_ms;
	}

	// Execute weapon firing behavior
	fireHandler();

	// If frame reached idle, transition state
	if (cl->ps.gunframe == FRAME_IDLE_FIRST) {
		cl->weaponState = WEAPON_READY;
		cl->weaponFireBuffered = false;
	}

	cl->weaponThinkTime = level.time + Weapon_AnimationTime(ent);
}

/*
=======================
Weapon_Generic
=======================
*/
void Weapon_Generic(
	gentity_t *ent,
	int FRAME_ACTIVATE_LAST,
	int FRAME_FIRE_LAST,
	int FRAME_IDLE_LAST,
	int FRAME_DEACTIVATE_LAST,
	const int *pauseFrames,
	const int *fireFrames,
	void (*fire)(gentity_t *ent)
) {
	if (!ent || !ent->client)
		return;

	auto *cl = ent->client;

	const int FRAME_FIRE_FIRST = FRAME_ACTIVATE_LAST + 1;
	const int FRAME_IDLE_FIRST = FRAME_FIRE_LAST + 1;
	const int FRAME_DEACTIVATE_FIRST = FRAME_IDLE_LAST + 1;

	if (!Weapon_CanAnimate(ent))
		return;

	if (Weapon_HandleDropping(ent, FRAME_DEACTIVATE_LAST))
		return;

	if (Weapon_HandleActivating(ent, FRAME_ACTIVATE_LAST, FRAME_IDLE_FIRST))
		return;

	if (Weapon_HandleNewWeapon(ent, FRAME_DEACTIVATE_FIRST, FRAME_DEACTIVATE_LAST))
		return;

	const auto readyState = Weapon_HandleReady(ent, FRAME_FIRE_FIRST, FRAME_IDLE_FIRST, FRAME_IDLE_LAST, pauseFrames);

	if (readyState == READY_FIRING) {
		cl->ps.gunframe = FRAME_FIRE_FIRST;
		cl->weaponFireBuffered = false;

		if (cl->weaponThunk)
			cl->weaponThinkTime += FRAME_TIME_S;

		cl->weaponThinkTime += Weapon_AnimationTime(ent);
		Weapon_SetFinished(ent);

		// Play attack animation
		cl->anim.priority = ANIM_ATTACK;
		if (cl->ps.pmove.pm_flags & PMF_DUCKED) {
			ent->s.frame = FRAME_crattak1 - 1;
			cl->anim.end = FRAME_crattak9;
		} else {
			ent->s.frame = FRAME_attack1 - 1;
			cl->anim.end = FRAME_attack8;
		}
		cl->anim.time = 0_ms;

		for (int i = 0; fireFrames[i]; ++i) {
			if (cl->ps.gunframe == fireFrames[i]) {
				Weapon_PowerupSound(ent);
				fire(ent);
				break;
			}
		}

		return;
	}

	// Handle held firing state
	if (cl->weaponState == WEAPON_FIRING && cl->weaponThinkTime <= level.time) {
		cl->last_firing_time = level.time + COOP_DAMAGE_FIRING_TIME;
		cl->ps.gunframe++;

		Weapon_HandleFiring(ent, FRAME_IDLE_FIRST, [&]() {
			for (int i = 0; fireFrames[i]; ++i) {
				if (cl->ps.gunframe == fireFrames[i]) {
					Weapon_PowerupSound(ent);
					fire(ent);
					break;
				}
			}
			});
	}
}

/*
=======================
Weapon_Repeating
=======================
*/
void Weapon_Repeating(
	gentity_t *ent,
	int FRAME_ACTIVATE_LAST,
	int FRAME_FIRE_LAST,
	int FRAME_IDLE_LAST,
	int FRAME_DEACTIVATE_LAST,
	const int *pauseFrames,
	void (*fire)(gentity_t *ent)
) {
	if (!ent || !ent->client)
		return;

	const int FRAME_FIRE_FIRST = FRAME_ACTIVATE_LAST + 1;
	const int FRAME_IDLE_FIRST = FRAME_FIRE_LAST + 1;
	const int FRAME_DEACTIVATE_FIRST = FRAME_IDLE_LAST + 1;

	if (!Weapon_CanAnimate(ent))
		return;

	if (Weapon_HandleDropping(ent, FRAME_DEACTIVATE_LAST))
		return;

	if (Weapon_HandleActivating(ent, FRAME_ACTIVATE_LAST, FRAME_IDLE_FIRST))
		return;

	if (Weapon_HandleNewWeapon(ent, FRAME_DEACTIVATE_FIRST, FRAME_DEACTIVATE_LAST))
		return;

	if (Weapon_HandleReady(ent, FRAME_FIRE_FIRST, FRAME_IDLE_FIRST, FRAME_IDLE_LAST, pauseFrames) == READY_CHANGING)
		return;

	// Handle firing state
	if (ent->client->weaponState == WEAPON_FIRING && ent->client->weaponThinkTime <= level.time) {
		ent->client->last_firing_time = level.time + COOP_DAMAGE_FIRING_TIME;

		Weapon_HandleFiring(ent, FRAME_IDLE_FIRST, [&]() {
			fire(ent);
			});

		if (ent->client->weaponThunk)
			ent->client->weaponThinkTime += FRAME_TIME_S;
	}
}

/*
======================================================================

HAND GRENADES

======================================================================
*/

static void Weapon_HandGrenade_Fire(gentity_t *ent, bool held) {
	if (!ent || !ent->client)
		return;

	int damage = 125;
	float radius = static_cast<float>(damage + 40);

	if (is_quad)
		damage *= damageMultiplier;

	// Clamp vertical angle to prevent backward throws
	vec3_t clampedAngles = {
		std::max(-62.5f, ent->client->vAngle[PITCH]),
		ent->client->vAngle[YAW],
		ent->client->vAngle[ROLL]
	};

	vec3_t start{}, dir{};
	P_ProjectSource(ent, clampedAngles, { 2, 0, -14 }, start, dir);

	// Determine grenade throw speed based on hold duration or death fallback
	gtime_t timer = ent->client->grenadeTime - level.time;
	const float holdSeconds = GRENADE_TIMER.seconds();
	int speed;

	if (ent->health <= 0) {
		speed = GRENADE_MINSPEED;
	} else {
		const float heldTime = (GRENADE_TIMER - timer).seconds();
		const float maxDelta = (GRENADE_MAXSPEED - GRENADE_MINSPEED) / holdSeconds;
		speed = static_cast<int>(std::min(GRENADE_MINSPEED + heldTime * maxDelta, static_cast<float>(GRENADE_MAXSPEED)));
	}

	ent->client->grenadeTime = 0_ms;

	fire_handgrenade(ent, start, dir, damage, speed, timer, radius, held);

	ent->client->pers.match.totalShots++;
	ent->client->pers.match.totalShotsPerWeapon[WEAP_HAND_GRENADES]++;
	RemoveAmmo(ent, 1);
}

/*
===================
Throw_Generic
===================
*/
void Throw_Generic(
	gentity_t *ent,
	int FRAME_FIRE_LAST,
	int FRAME_IDLE_LAST,
	int FRAME_PRIME_SOUND,
	const char *prime_sound,
	int FRAME_THROW_HOLD,
	int FRAME_THROW_FIRE,
	const int *pauseFrames,
	int EXPLODE,
	const char *primed_sound,
	void (*fire)(gentity_t *ent, bool held),
	bool extra_idle_frame
) {
	if (!ent || !ent->client)
		return;

	auto *cl = ent->client;
	const int FRAME_IDLE_FIRST = FRAME_FIRE_LAST + 1;

	// On death: toss held grenade
	if (ent->health <= 0) {
		fire(ent, true);
		return;
	}

	// Weapon change queued
	if (cl->newWeapon && cl->weaponState == WEAPON_READY) {
		if (cl->weaponThinkTime <= level.time) {
			Change_Weapon(ent);
			cl->weaponThinkTime = level.time + Weapon_AnimationTime(ent);
		}
		return;
	}

	// Weapon is activating
	if (cl->weaponState == WEAPON_ACTIVATING) {
		if (cl->weaponThinkTime <= level.time) {
			cl->weaponState = WEAPON_READY;
			cl->ps.gunframe = extra_idle_frame ? FRAME_IDLE_LAST + 1 : FRAME_IDLE_FIRST;
			cl->weaponThinkTime = level.time + Weapon_AnimationTime(ent);
			Weapon_SetFinished(ent);
		}
		return;
	}

	// Weapon ready: listen for throw intent
	if (cl->weaponState == WEAPON_READY) {
		bool request_firing = false;

		if (CombatIsDisabled()) {
			cl->latchedButtons &= ~BUTTON_ATTACK;
		} else {
			request_firing = cl->weaponFireBuffered || ((cl->latchedButtons | cl->buttons) & BUTTON_ATTACK);
		}

		if (request_firing && cl->weaponFireFinished <= level.time) {
			cl->latchedButtons &= ~BUTTON_ATTACK;

			if (cl->pers.inventory[cl->pers.weapon->ammo]) {
				cl->ps.gunframe = 1;
				cl->weaponState = WEAPON_FIRING;
				cl->grenadeTime = 0_ms;
				cl->weaponThinkTime = level.time + Weapon_AnimationTime(ent);
			} else {
				NoAmmoWeaponChange(ent, true);
			}
			return;
		}

		// Idle animation progression
		if (cl->weaponThinkTime <= level.time) {
			cl->weaponThinkTime = level.time + Weapon_AnimationTime(ent);

			if (cl->ps.gunframe >= FRAME_IDLE_LAST) {
				cl->ps.gunframe = FRAME_IDLE_FIRST;
				return;
			}

			if (pauseFrames) {
				for (int i = 0; pauseFrames[i]; ++i) {
					if (cl->ps.gunframe == pauseFrames[i] && irandom(16))
						return;
				}
			}

			cl->ps.gunframe++;
		}
		return;
	}

	// Weapon is firing
	if (cl->weaponState == WEAPON_FIRING && cl->weaponThinkTime <= level.time) {
		cl->last_firing_time = level.time + COOP_DAMAGE_FIRING_TIME;

		if (prime_sound && cl->ps.gunframe == FRAME_PRIME_SOUND) {
			gi.sound(ent, CHAN_WEAPON, gi.soundindex(prime_sound), 1, ATTN_NORM, 0);
		}

		// Adjust fuse delay for time effects
		gtime_t fuseWait = 1_sec;
		if (Tech_ApplyTimeAccel(ent) || is_haste || g_frenzy->integer)
			fuseWait *= 0.5f;

		// Primed and held state
		if (cl->ps.gunframe == FRAME_THROW_HOLD) {
			if (!cl->grenadeTime && !cl->grenadeFinishedTime)
				cl->grenadeTime = level.time + GRENADE_TIMER + 200_ms;

			if (primed_sound && !cl->grenadeBlewUp)
				cl->weaponSound = gi.soundindex(primed_sound);

			// Detonate in hand
			if (EXPLODE && !cl->grenadeBlewUp && level.time >= cl->grenadeTime) {
				Weapon_PowerupSound(ent);
				cl->weaponSound = 0;
				fire(ent, true);
				cl->grenadeBlewUp = true;
				cl->grenadeFinishedTime = level.time + fuseWait;
			}

			// Still holding the button
			if (cl->buttons & BUTTON_ATTACK) {
				cl->weaponThinkTime = level.time + 1_ms;
				return;
			}

			if (cl->grenadeBlewUp) {
				if (level.time >= cl->grenadeFinishedTime) {
					cl->ps.gunframe = FRAME_FIRE_LAST;
					cl->grenadeBlewUp = false;
					cl->weaponThinkTime = level.time + Weapon_AnimationTime(ent);
				}
				return;
			}

			// Normal throw
			cl->ps.gunframe++;
			Weapon_PowerupSound(ent);
			cl->weaponSound = 0;
			fire(ent, false);

			if (!EXPLODE || !cl->grenadeBlewUp)
				cl->grenadeFinishedTime = level.time + fuseWait;

			// Play throw animation
			if (!ent->deadFlag && ent->s.modelindex == MODELINDEX_PLAYER && ent->health > 0) {
				if (cl->ps.pmove.pm_flags & PMF_DUCKED) {
					cl->anim.priority = ANIM_ATTACK;
					ent->s.frame = FRAME_crattak1 - 1;
					cl->anim.end = FRAME_crattak3;
				} else {
					cl->anim.priority = ANIM_ATTACK | ANIM_REVERSED;
					ent->s.frame = FRAME_wave08;
					cl->anim.end = FRAME_wave01;
				}
				cl->anim.time = 0_ms;
			}
		}

		cl->weaponThinkTime = level.time + Weapon_AnimationTime(ent);

		// Delay if not ready to return to idle
		if (cl->ps.gunframe == FRAME_FIRE_LAST && level.time < cl->grenadeFinishedTime)
			return;

		cl->ps.gunframe++;

		// Return to idle
		if (cl->ps.gunframe == FRAME_IDLE_FIRST) {
			cl->grenadeFinishedTime = 0_ms;
			cl->weaponState = WEAPON_READY;
			cl->weaponFireBuffered = false;
			Weapon_SetFinished(ent);

			if (extra_idle_frame)
				cl->ps.gunframe = FRAME_IDLE_LAST + 1;

			// Out of grenades: auto-switch
			if (!cl->pers.inventory[cl->pers.weapon->ammo]) {
				NoAmmoWeaponChange(ent, false);
				Change_Weapon(ent);
			}
		}
	}
}

void Weapon_HandGrenade(gentity_t *ent) {
	constexpr int pauseFrames[] = { 29, 34, 39, 48, 0 };

	Throw_Generic(ent, 15, 48, 5, "weapons/hgrena1b.wav", 11, 12, pauseFrames, true, "weapons/hgrenc1b.wav", Weapon_HandGrenade_Fire, true);

	// [Paril-KEX] skip the duped frame
	if (ent->client->ps.gunframe == 1)
		ent->client->ps.gunframe = 2;
}

/*
======================================================================

GRENADE LAUNCHER

======================================================================
*/

static void Weapon_GrenadeLauncher_Fire(gentity_t *ent) {
	if (!ent || !ent->client)
		return;

	int damage;
	float splashRadius;
	int speed;

	if (RS(RS_Q3A)) {
		damage = 100;
		splashRadius = 150.0f;
		speed = 700;
	} else {
		damage = 120;
		splashRadius = static_cast<float>(damage + 40);
		speed = 600;
	}

	if (is_quad)
		damage *= damageMultiplier;

	// Clamp upward angle to avoid backward fire
	vec3_t clampedAngles = {
		std::max(-62.5f, ent->client->vAngle[PITCH]),
		ent->client->vAngle[YAW],
		ent->client->vAngle[ROLL]
	};

	vec3_t start{}, dir{};
	P_ProjectSource(ent, clampedAngles, { 8, 0, -8 }, start, dir);

	// Weapon kick
	vec3_t kickOrigin{};
	for (int i = 0; i < 3; ++i)
		kickOrigin[i] = ent->client->vForward[i] * -2.0f;

	vec3_t kickAngles = { -1.0f, 0.0f, 0.0f };
	P_AddWeaponKick(ent, kickOrigin, kickAngles);

	// Fire grenade
	const float bounce = crandom_open() * 10.0f;
	const float fuseVel = 200.0f + crandom_open() * 10.0f;

	fire_grenade(ent, start, dir, damage, speed, 2.5_sec, splashRadius, bounce, fuseVel, false);

	// Muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_GRENADE | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->pers.match.totalShots++;
	ent->client->pers.match.totalShotsPerWeapon[WEAP_GRENADE_LAUNCHER]++;
	RemoveAmmo(ent, 1);
}

void Weapon_GrenadeLauncher(gentity_t *ent) {
	constexpr int pauseFrames[] = { 34, 51, 59, 0 };
	constexpr int fireFrames[] = { 6, 0 };

	Weapon_Generic(ent, 5, 16, 59, 64, pauseFrames, fireFrames, Weapon_GrenadeLauncher_Fire);
}

/*
======================================================================

ROCKET LAUNCHER

======================================================================
*/

static void Weapon_RocketLauncher_Fire(gentity_t *ent) {
	if (!ent || !ent->client)
		return;

	constexpr int baseDamage = 100;
	constexpr int baseSplashRadius = 100;

	int damage = baseDamage;
	int splashDamage = baseDamage;
	int splashRadius = baseSplashRadius;
	int speed = 800;

	switch (game.ruleset) {
	case RS_Q1:
		speed = 1000;
		break;
	case RS_Q3A:
		speed = 900;
		break;
	default:
		speed = 800;
		break;
	}

	if (g_frenzy->integer)
		speed = static_cast<int>(speed * 1.5f);

	if (is_quad) {
		damage *= damageMultiplier;
		splashDamage *= damageMultiplier;
	}

	vec3_t start{}, dir{};
	P_ProjectSource(ent, ent->client->vAngle, { 8, 8, -8 }, start, dir);
	fire_rocket(ent, start, dir, damage, speed, splashRadius, splashDamage);

	vec3_t kickOrigin{};
	for (int i = 0; i < 3; ++i)
		kickOrigin[i] = ent->client->vForward[i] * -2.0f;

	vec3_t kickAngles = { -1.0f, 0.0f, 0.0f };
	P_AddWeaponKick(ent, kickOrigin, kickAngles);

	// Muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_ROCKET | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->pers.match.totalShots++;
	ent->client->pers.match.totalShotsPerWeapon[WEAP_ROCKET_LAUNCHER]++;
	RemoveAmmo(ent, 1);
}

void Weapon_RocketLauncher(gentity_t *ent) {
	constexpr int pauseFrames[] = { 25, 33, 42, 50, 0 };
	constexpr int fireFrames[] = { 5, 0 };

	Weapon_Generic(ent, 4, 12, 50, 54, pauseFrames, fireFrames, Weapon_RocketLauncher_Fire);
}


/*
======================================================================

GRAPPLE

======================================================================
*/

// self is grapple, not player
static void Weapon_Grapple_Reset(gentity_t *self) {
	if (!self || !self->owner->client || !self->owner->client->grappleEnt)
		return;

	gi.sound(self->owner, CHAN_WEAPON, gi.soundindex("weapons/grapple/grreset.wav"), self->owner->client->powerupTime.silencerShots ? 0.2f : 1.0f, ATTN_NORM, 0);

	gclient_t *cl;
	cl = self->owner->client;
	cl->grappleEnt = nullptr;
	cl->grappleReleaseTime = level.time + 1_sec;
	cl->grappleState = GRAPPLE_STATE_FLY; // we're firing, not on hook
	self->owner->flags &= ~FL_NO_KNOCKBACK;
	FreeEntity(self);
}

void Weapon_Grapple_DoReset(gclient_t *cl) {
	if (cl && cl->grappleEnt)
		Weapon_Grapple_Reset(cl->grappleEnt);
}

static TOUCH(Weapon_Grapple_Touch) (gentity_t *self, gentity_t *other, const trace_t &tr, bool otherTouchingSelf) -> void {
	float volume = 1.0;

	if (other == self->owner)
		return;

	if (self->owner->client->grappleState != GRAPPLE_STATE_FLY)
		return;

	if (tr.surface && (tr.surface->flags & SURF_SKY)) {
		Weapon_Grapple_Reset(self);
		return;
	}

	self->velocity = {};

	PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	if (other->takeDamage) {
		if (self->dmg)
			Damage(other, self, self->owner, self->velocity, self->s.origin, tr.plane.normal, self->dmg, 1, DAMAGE_NONE | DAMAGE_STAT_ONCE, MOD_GRAPPLE);
		Weapon_Grapple_Reset(self);
		return;
	}

	self->owner->client->grappleState = GRAPPLE_STATE_PULL; // we're on hook
	self->enemy = other;

	self->solid = SOLID_NOT;

	if (self->owner->client->powerupTime.silencerShots)
		volume = 0.2f;

	gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/grapple/grhit.wav"), volume, ATTN_NORM, 0);
	self->s.sound = gi.soundindex("weapons/grapple/grpull.wav");

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_SPARKS);
	gi.WritePosition(self->s.origin);
	gi.WriteDir(tr.plane.normal);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);
}

// draw beam between grapple and self
static void Weapon_Grapple_DrawCable(gentity_t *self) {
	if (self->owner->client->grappleState == GRAPPLE_STATE_HANG)
		return;

	vec3_t start, dir;
	P_ProjectSource(self->owner, self->owner->client->vAngle, { 7, 2, -9 }, start, dir);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_GRAPPLE_CABLE_2);
	gi.WriteEntity(self->owner);
	gi.WritePosition(start);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);
}

// pull the player toward the grapple
void Weapon_Grapple_Pull(gentity_t *self) {
	vec3_t hookdir, v;
	float  vlen;

	if (self->owner->client->pers.weapon && self->owner->client->pers.weapon->id == IT_WEAPON_GRAPPLE &&
		!(self->owner->client->newWeapon || ((self->owner->client->latchedButtons | self->owner->client->buttons) & BUTTON_HOLSTER)) &&
		self->owner->client->weaponState != WEAPON_FIRING &&
		self->owner->client->weaponState != WEAPON_ACTIVATING) {
		if (!self->owner->client->newWeapon)
			self->owner->client->newWeapon = self->owner->client->pers.weapon;

		Weapon_Grapple_Reset(self);
		return;
	}

	if (self->enemy) {
		if (self->enemy->solid == SOLID_NOT) {
			Weapon_Grapple_Reset(self);
			return;
		}
		if (self->enemy->solid == SOLID_BBOX) {
			v = self->enemy->size * 0.5f;
			v += self->enemy->s.origin;
			self->s.origin = v + self->enemy->mins;
			gi.linkentity(self);
		} else
			self->velocity = self->enemy->velocity;

		if (self->enemy->deadFlag) { // he died
			Weapon_Grapple_Reset(self);
			return;
		}
	}

	Weapon_Grapple_DrawCable(self);

	if (self->owner->client->grappleState > GRAPPLE_STATE_FLY) {
		// pull player toward grapple
		vec3_t forward, up;

		AngleVectors(self->owner->client->vAngle, forward, nullptr, up);
		v = self->owner->s.origin;
		v[2] += self->owner->viewHeight;
		hookdir = self->s.origin - v;

		vlen = hookdir.length();

		if (self->owner->client->grappleState == GRAPPLE_STATE_PULL &&
			vlen < 64) {
			self->owner->client->grappleState = GRAPPLE_STATE_HANG;
			self->s.sound = gi.soundindex("weapons/grapple/grhang.wav");
		}

		hookdir.normalize();
		hookdir = hookdir * g_grapple_pull_speed->value;
		self->owner->velocity = hookdir;
		self->owner->flags |= FL_NO_KNOCKBACK;
		G_AddGravity(self->owner);
	}
}

static DIE(Weapon_Grapple_Die) (gentity_t *self, gentity_t *other, gentity_t *inflictor, int damage, const vec3_t &point, const mod_t &mod) -> void {
	if (mod.id == MOD_CRUSH)
		Weapon_Grapple_Reset(self);
}

static bool Weapon_Grapple_FireHook(gentity_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, Effect effect) {
	gentity_t *grapple;
	trace_t	tr;
	vec3_t	normalized = dir.normalized();

	grapple = Spawn();
	grapple->s.origin = start;
	grapple->s.old_origin = start;
	grapple->s.angles = vectoangles(normalized);
	grapple->velocity = normalized * speed;
	grapple->moveType = MOVETYPE_FLYMISSILE;
	grapple->clipMask = MASK_PROJECTILE;
	// [Paril-KEX]
	if (self->client && !G_ShouldPlayersCollide(true))
		grapple->clipMask &= ~CONTENTS_PLAYER;
	grapple->solid = SOLID_BBOX;
	grapple->s.effects |= effect;
	grapple->s.modelindex = gi.modelindex("models/weapons/grapple/hook/tris.md2");
	grapple->owner = self;
	grapple->touch = Weapon_Grapple_Touch;
	grapple->dmg = damage;
	grapple->flags |= FL_NO_KNOCKBACK | FL_NO_DAMAGE_EFFECTS;
	grapple->takeDamage = true;
	grapple->die = Weapon_Grapple_Die;
	self->client->grappleEnt = grapple;
	self->client->grappleState = GRAPPLE_STATE_FLY; // we're firing, not on hook
	gi.linkentity(grapple);

	tr = gi.traceline(self->s.origin, grapple->s.origin, grapple, grapple->clipMask);
	if (tr.fraction < 1.0f) {
		grapple->s.origin = tr.endpos + (tr.plane.normal * 1.f);
		grapple->touch(grapple, tr.ent, tr, false);
		return false;
	}

	grapple->s.sound = gi.soundindex("weapons/grapple/grfly.wav");

	return true;
}

static void Weapon_Grapple_DoFire(gentity_t *ent, const vec3_t &g_offset, int damage, Effect effect) {
	float volume = 1.0;

	if (ent->client->grappleState > GRAPPLE_STATE_FLY)
		return; // it's already out

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->vAngle, vec3_t{ 24, 8, -8 + 2 } + g_offset, start, dir);

	if (ent->client->powerupTime.silencerShots)
		volume = 0.2f;

	if (Weapon_Grapple_FireHook(ent, start, dir, damage, g_grapple_fly_speed->value, effect))
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/grapple/grfire.wav"), volume, ATTN_NORM, 0);

	PlayerNoise(ent, start, PNOISE_WEAPON);
}

static void Weapon_Grapple_Fire(gentity_t *ent) {
	Weapon_Grapple_DoFire(ent, vec3_origin, g_grapple_damage->integer, EF_NONE);
}

void Weapon_Grapple(gentity_t *ent) {
	constexpr int pauseFrames[] = { 10, 18, 27, 0 };
	constexpr int fireFrames[] = { 6, 0 };
	int			  prevstate;

	// if the the attack button is still down, stay in the firing frame
	if ((ent->client->buttons & (BUTTON_ATTACK | BUTTON_HOLSTER)) &&
		ent->client->weaponState == WEAPON_FIRING &&
		ent->client->grappleEnt)
		ent->client->ps.gunframe = 6;

	if (!(ent->client->buttons & (BUTTON_ATTACK | BUTTON_HOLSTER)) &&
		ent->client->grappleEnt) {
		Weapon_Grapple_Reset(ent->client->grappleEnt);
		if (ent->client->weaponState == WEAPON_FIRING)
			ent->client->weaponState = WEAPON_READY;
	}

	if ((ent->client->newWeapon || ((ent->client->latchedButtons | ent->client->buttons) & BUTTON_HOLSTER)) &&
		ent->client->grappleState > GRAPPLE_STATE_FLY &&
		ent->client->weaponState == WEAPON_FIRING) {
		// he wants to change weapons while grappled
		if (!ent->client->newWeapon)
			ent->client->newWeapon = ent->client->pers.weapon;
		ent->client->weaponState = WEAPON_DROPPING;
		ent->client->ps.gunframe = 32;
	}

	prevstate = ent->client->weaponState;
	Weapon_Generic(ent, 5, 10, 31, 36, pauseFrames, fireFrames,
		Weapon_Grapple_Fire);

	// if the the attack button is still down, stay in the firing frame
	if ((ent->client->buttons & (BUTTON_ATTACK | BUTTON_HOLSTER)) &&
		ent->client->weaponState == WEAPON_FIRING &&
		ent->client->grappleEnt)
		ent->client->ps.gunframe = 6;

	// if we just switched back to grapple, immediately go to fire frame
	if (prevstate == WEAPON_ACTIVATING &&
		ent->client->weaponState == WEAPON_READY &&
		ent->client->grappleState > GRAPPLE_STATE_FLY) {
		if (!(ent->client->buttons & (BUTTON_ATTACK | BUTTON_HOLSTER)))
			ent->client->ps.gunframe = 6;
		else
			ent->client->ps.gunframe = 5;
		ent->client->weaponState = WEAPON_FIRING;
	}
}

/*
======================================================================

OFF-HAND HOOK

======================================================================
*/

static void Weapon_Hook_DoFire(gentity_t *ent, const vec3_t &g_offset, int damage, Effect effect) {
	if (ent->client->grappleState > GRAPPLE_STATE_FLY)
		return; // it's already out

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->vAngle, vec3_t{ 24, 0, 0 } + g_offset, start, dir);

	if (Weapon_Grapple_FireHook(ent, start, dir, damage, g_grapple_fly_speed->value, effect))
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/grapple/grfire.wav"), ent->client->powerupTime.silencerShots ? 0.2f : 1.0f, ATTN_NORM, 0);

	PlayerNoise(ent, start, PNOISE_WEAPON);
}

void Weapon_Hook(gentity_t *ent) {
	Weapon_Hook_DoFire(ent, vec3_origin, g_grapple_damage->integer, EF_NONE);
}

/*
======================================================================

BLASTER / HYPERBLASTER

======================================================================
*/

static void Weapon_Blaster_Fire(gentity_t *ent, const vec3_t &g_offset, int damage, bool hyper, Effect effect) {
	if (!ent || !ent->client)
		return;

	if (is_quad)
		damage *= damageMultiplier;

	// Calculate final offset from muzzle
	vec3_t offset = {
		24.0f + g_offset[0],
		 8.0f + g_offset[1],
		-8.0f + g_offset[2]
	};

	vec3_t start{}, dir{};
	P_ProjectSource(ent, ent->client->vAngle, offset, start, dir);

	// Kick origin
	vec3_t kickOrigin{};
	for (int i = 0; i < 3; ++i)
		kickOrigin[i] = ent->client->vForward[i] * -2.0f;

	// Kick angles
	vec3_t kickAngles{};
	if (hyper) {
		for (int i = 0; i < 3; ++i)
			kickAngles[i] = crandom() * 0.7f;
	} else {
		kickAngles[0] = -1.0f;
		kickAngles[1] = 0.0f;
		kickAngles[2] = 0.0f;
	}

	P_AddWeaponKick(ent, kickOrigin, kickAngles);

	// Determine projectile speed
	int speed = 0;
	if (RS(RS_Q3A)) {
		speed = hyper ? 2000 : 2500;
	} else {
		speed = hyper ? 1000 : 1500;
	}

	fire_blaster(ent, start, dir, damage, speed, effect, hyper ? MOD_HYPERBLASTER : MOD_BLASTER);

	// Muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte((hyper ? MZ_HYPERBLASTER : MZ_BLASTER) | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->pers.match.totalShots++;
	ent->client->pers.match.totalShotsPerWeapon[WEAP_BLASTER]++;
}

static void Weapon_Blaster_DoFire(gentity_t *ent) {
	int damage = 15;
	Weapon_Blaster_Fire(ent, vec3_origin, damage, false, EF_BLASTER);
}

void Weapon_Blaster(gentity_t *ent) {
	constexpr int pauseFrames[] = { 19, 32, 0 };
	constexpr int fireFrames[] = { 5, 0 };

	Weapon_Generic(ent, 4, 8, 52, 55, pauseFrames, fireFrames, Weapon_Blaster_DoFire);
}

static void Weapon_HyperBlaster_Fire(gentity_t *ent) {
	if (!ent || !ent->client)
		return;

	gclient_t *client = ent->client;
	int damage;
	float rotation;

	// Advance or reset gunframe
	if (client->ps.gunframe > 20) {
		client->ps.gunframe = 6;
	} else {
		client->ps.gunframe++;
	}

	// Loop logic or wind-down sound
	if (client->ps.gunframe == 12) {
		if ((client->pers.inventory[client->pers.weapon->ammo] > 0) &&
			(client->buttons & BUTTON_ATTACK)) {
			client->ps.gunframe = 6;
		} else {
			gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/hyprbd1a.wav"), 1, ATTN_NORM, 0);
		}
	}

	// Weapon sound during firing loop
	if (client->ps.gunframe >= 6 && client->ps.gunframe <= 11) {
		client->weaponSound = gi.soundindex("weapons/hyprbl1a.wav");
	} else {
		client->weaponSound = 0;
	}

	// Firing logic
	const bool isFiring = client->weaponFireBuffered || (client->buttons & BUTTON_ATTACK);

	if (isFiring && client->ps.gunframe >= 6 && client->ps.gunframe <= 11) {
		client->weaponFireBuffered = false;

		if (client->pers.inventory[client->pers.weapon->ammo] < 1) {
			NoAmmoWeaponChange(ent, true);
			return;
		}

		// Calculate rotating offset
		vec3_t offset = { 0 };
		rotation = (client->ps.gunframe - 5) * 2.0f * PIf / 6.0f;
		offset[0] = -4.0f * sinf(rotation);
		offset[1] = 4.0f * cosf(rotation);
		offset[2] = 0.0f;

		// Set damage based on ruleset
		if (RS(RS_Q3A)) {
			damage = deathmatch->integer ? 20 : 25;
		} else {
			damage = deathmatch->integer ? 15 : 20;
		}

		const Effect effect = (client->ps.gunframe % 4 == 0) ? EF_HYPERBLASTER : EF_NONE;

		Weapon_Blaster_Fire(ent, offset, damage, true, effect);
		Weapon_PowerupSound(ent);

		client->pers.match.totalShots++;
		client->pers.match.totalShotsPerWeapon[WEAP_HYPERBLASTER]++;
		RemoveAmmo(ent, 1);

		// Play attack animation
		client->anim.priority = ANIM_ATTACK;
		if (client->ps.pmove.pm_flags & PMF_DUCKED) {
			ent->s.frame = FRAME_crattak1 - (int)(frandom() + 0.25f);
			client->anim.end = FRAME_crattak9;
		} else {
			ent->s.frame = FRAME_attack1 - (int)(frandom() + 0.25f);
			client->anim.end = FRAME_attack8;
		}
		client->anim.time = 0_ms;
	}
}

void Weapon_HyperBlaster(gentity_t *ent) {
	constexpr int pauseFrames[] = { 0 };

	Weapon_Repeating(ent, 5, 20, 49, 53, pauseFrames, Weapon_HyperBlaster_Fire);
}

/*
======================================================================

MACHINEGUN / CHAINGUN

======================================================================
*/

static void Weapon_Machinegun_Fire(gentity_t *ent) {
	if (!ent || !ent->client)
		return;

	auto &client = *ent->client;
	auto &ps = client.ps;

	int damage = 8;
	int kick = 2;
	int hSpread = DEFAULT_BULLET_HSPREAD;
	int vSpread = DEFAULT_BULLET_VSPREAD;

	if (RS(RS_Q3A)) {
		damage = GT(GT_TDM) ? 5 : 7;
		hSpread = 200;
		vSpread = 200;
	}

	if (!(client.buttons & BUTTON_ATTACK)) {
		ps.gunframe = 6;
		return;
	}

	ps.gunframe = (ps.gunframe == 4) ? 5 : 4;

	int &ammo = client.pers.inventory[client.pers.weapon->ammo];
	if (ammo < 1) {
		ps.gunframe = 6;
		NoAmmoWeaponChange(ent, true);
		return;
	}

	if (is_quad) {
		damage *= damageMultiplier;
		kick *= damageMultiplier;
	}

	vec3_t kickOrigin{
		crandom() * 0.35f,
		crandom() * 0.35f,
		crandom() * 0.35f
	};

	vec3_t kickAngles{
		crandom() * 0.7f,
		crandom() * 0.7f,
		crandom() * 0.7f
	};

	P_AddWeaponKick(ent, kickOrigin, kickAngles);

	vec3_t start, dir;
	P_ProjectSource(ent, client.vAngle, { 0, 0, -8 }, start, dir);

	LagCompensate(ent, start, dir);
	fire_bullet(ent, start, dir, damage, kick, hSpread, vSpread, MOD_MACHINEGUN);
	UnLagCompensate();

	Weapon_PowerupSound(ent);

	// Muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_MACHINEGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	client.pers.match.totalShots++;
	client.pers.match.totalShotsPerWeapon[WEAP_MACHINEGUN]++;
	RemoveAmmo(ent, 1);

	// Attack animation
	client.anim.priority = ANIM_ATTACK;
	if (ps.pmove.pm_flags & PMF_DUCKED) {
		ent->s.frame = FRAME_crattak1 - static_cast<int>(frandom() + 0.25f);
		client.anim.end = FRAME_crattak9;
	} else {
		ent->s.frame = FRAME_attack1 - static_cast<int>(frandom() + 0.25f);
		client.anim.end = FRAME_attack8;
	}
	client.anim.time = 0_ms;
}

void Weapon_Machinegun(gentity_t *ent) {
	constexpr int pauseFrames[] = { 23, 45, 0 };

	Weapon_Repeating(ent, 3, 5, 45, 49, pauseFrames, Weapon_Machinegun_Fire);
}

static void Weapon_Chaingun_Fire(gentity_t *ent) {
	if (!ent || !ent->client)
		return;

	auto &client = *ent->client;
	auto &ps = client.ps;
	const int damageBase = deathmatch->integer ? 6 : 8;
	int damage = damageBase;
	int kick = 2;

	// Handle gunframe animation
	if (ps.gunframe > 31) {
		ps.gunframe = 5;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);
	} else if (ps.gunframe == 14 && !(client.buttons & BUTTON_ATTACK)) {
		ps.gunframe = 32;
		client.weaponSound = 0;
		return;
	} else if (ps.gunframe == 21 && (client.buttons & BUTTON_ATTACK) && client.pers.inventory[client.pers.weapon->ammo]) {
		ps.gunframe = 15;
	} else {
		ps.gunframe++;
	}

	if (ps.gunframe == 22) {
		client.weaponSound = 0;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnd1a.wav"), 1, ATTN_IDLE, 0);
	}

	if (ps.gunframe < 5 || ps.gunframe > 21)
		return;

	client.weaponSound = gi.soundindex("weapons/chngnl1a.wav");

	// Set animation
	client.anim.priority = ANIM_ATTACK;
	if (ps.pmove.pm_flags & PMF_DUCKED) {
		ent->s.frame = FRAME_crattak1 - (ps.gunframe & 1);
		client.anim.end = FRAME_crattak9;
	} else {
		ent->s.frame = FRAME_attack1 - (ps.gunframe & 1);
		client.anim.end = FRAME_attack8;
	}
	client.anim.time = 0_ms;

	// Determine number of shots
	int shots = 0;
	if (ps.gunframe <= 9) {
		shots = 1;
	} else if (ps.gunframe <= 14) {
		shots = (client.buttons & BUTTON_ATTACK) ? 2 : 1;
	} else {
		shots = 3;
	}

	int &ammo = client.pers.inventory[client.pers.weapon->ammo];
	if (ammo < shots)
		shots = ammo;

	if (shots == 0) {
		NoAmmoWeaponChange(ent, true);
		return;
	}

	if (is_quad) {
		damage *= damageMultiplier;
		kick *= damageMultiplier;
	}

	// Apply weapon kick
	vec3_t kickOrigin{
		crandom() * 0.35f,
		crandom() * 0.35f,
		crandom() * 0.35f
	};

	vec3_t kickAngles{
		crandom() * (0.5f + shots * 0.15f),
		crandom() * (0.5f + shots * 0.15f),
		crandom() * (0.5f + shots * 0.15f)
	};

	P_AddWeaponKick(ent, kickOrigin, kickAngles);

	vec3_t start, dir;
	P_ProjectSource(ent, client.vAngle, { 0, 0, -8 }, start, dir);

	LagCompensate(ent, start, dir);

	for (int i = 0; i < shots; ++i) {
		// Recalculate for each shot
		P_ProjectSource(ent, client.vAngle, { 0, 0, -8 }, start, dir);

		fire_bullet(ent, start, dir, damage, kick,
			DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_CHAINGUN);
	}

	UnLagCompensate();

	Weapon_PowerupSound(ent);

	// Muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte((MZ_CHAINGUN1 + shots - 1) | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	client.pers.match.totalShots += shots;
	client.pers.match.totalShotsPerWeapon[WEAP_CHAINGUN] += shots;

	RemoveAmmo(ent, shots);
}

void Weapon_Chaingun(gentity_t *ent) {
	constexpr int pauseFrames[] = { 38, 43, 51, 61, 0 };

	Weapon_Repeating(ent, 4, 31, 61, 64, pauseFrames, Weapon_Chaingun_Fire);
}

/*
======================================================================

SHOTGUN / SUPERSHOTGUN

======================================================================
*/

static void Weapon_Shotgun_Fire(gentity_t *ent) {
	// Calculate damage and kick
	int damage = RS(RS_Q3A) ? 10 : 4;
	int kick = 4;

	if (is_quad) {
		damage *= damageMultiplier;
		kick *= damageMultiplier;
	}

	const int pelletCount = RS(RS_Q3A) ? 11 : 12;

	// Setup source and direction
	constexpr vec3_t viewOffset = { 0.f, 0.f, -8.f };
	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->vAngle, viewOffset, start, dir);

	// Apply weapon kickback
	P_AddWeaponKick(ent, ent->client->vForward * -2.f, { -2.f, 0.f, 0.f });

	// Fire with lag compensation
	LagCompensate(ent, start, dir);
	fire_shotgun(ent, start, dir, damage, kick, 500, 500, pelletCount, MOD_SHOTGUN);
	UnLagCompensate();

	// Muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_SHOTGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	// Weapon noise and stats
	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->pers.match.totalShots += pelletCount;
	ent->client->pers.match.totalShotsPerWeapon[WEAP_SHOTGUN] += pelletCount;
	RemoveAmmo(ent, 1);
}

void Weapon_Shotgun(gentity_t *ent) {
	constexpr int pauseFrames[] = { 22, 28, 34, 0 };
	constexpr int fireFrames[] = { 8, 0 };

	Weapon_Generic(ent, 7, 18, 36, 39, pauseFrames, fireFrames, Weapon_Shotgun_Fire);
}

/*
===========================
Weapon_SuperShotgun_Fire
===========================
*/
static void Weapon_SuperShotgun_Fire(gentity_t *ent) {
	int damage = 6;
	int kick = 6;

	if (is_quad) {
		damage *= damageMultiplier;
		kick *= damageMultiplier;
	}

	// Prepare direction and starting positions
	constexpr vec3_t viewOffset = { 0.f, 0.f, -8.f };
	vec3_t start, dir;

	// Central shot uses original angle
	P_ProjectSource(ent, ent->client->vAngle, viewOffset, start, dir);
	LagCompensate(ent, start, dir);

	// First barrel shot (slightly left)
	vec3_t leftAngle = {
		ent->client->vAngle[PITCH],
		ent->client->vAngle[YAW] - 5.f,
		ent->client->vAngle[ROLL]
	};
	P_ProjectSource(ent, leftAngle, viewOffset, start, dir);
	fire_shotgun(ent, start, dir, damage, kick,
		DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD,
		DEFAULT_SSHOTGUN_COUNT / 2, MOD_SSHOTGUN);

	// Second barrel shot (slightly right)
	vec3_t rightAngle = {
		ent->client->vAngle[PITCH],
		ent->client->vAngle[YAW] + 5.f,
		ent->client->vAngle[ROLL]
	};
	P_ProjectSource(ent, rightAngle, viewOffset, start, dir);
	fire_shotgun(ent, start, dir, damage, kick,
		DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD,
		DEFAULT_SSHOTGUN_COUNT / 2, MOD_SSHOTGUN);

	UnLagCompensate();

	// Add recoil
	P_AddWeaponKick(ent, ent->client->vForward * -2.f, { -2.f, 0.f, 0.f });

	// Visual and sound effects
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_SSHOTGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	// Stats and ammo
	ent->client->pers.match.totalShots += DEFAULT_SSHOTGUN_COUNT;
	ent->client->pers.match.totalShotsPerWeapon[WEAP_SUPER_SHOTGUN] += DEFAULT_SSHOTGUN_COUNT;
	RemoveAmmo(ent, 2);
}

void Weapon_SuperShotgun(gentity_t *ent) {
	constexpr int pauseFrames[] = { 29, 42, 57, 0 };
	constexpr int fireFrames[] = { 7, 0 };

	Weapon_Generic(ent, 6, 17, 57, 61, pauseFrames, fireFrames, Weapon_SuperShotgun_Fire);
}

/*
======================================================================

RAILGUN

======================================================================
*/

static void Weapon_Railgun_Fire(gentity_t *ent) {
	int damage = deathmatch->integer ? 80 : 150;
	int kick = damage;

	if (is_quad) {
		damage *= damageMultiplier;
		kick *= damageMultiplier;
	}

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->vAngle, { 0.f, 7.f, -8.f }, start, dir);

	LagCompensate(ent, start, dir);
	fire_rail(ent, start, dir, damage, kick);
	UnLagCompensate();

	P_AddWeaponKick(ent, ent->client->vForward * -3.f, { -3.f, 0.f, 0.f });

	// Muzzle flash effect
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_RAILGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	// Stats and ammo tracking
	ent->client->pers.match.totalShots++;
	ent->client->pers.match.totalShotsPerWeapon[WEAP_RAILGUN]++;
	RemoveAmmo(ent, 1);
}

void Weapon_Railgun(gentity_t *ent) {
	constexpr int pauseFrames[] = { 56, 0 };
	constexpr int fireFrames[] = { 4, 0 };

	Weapon_Generic(ent, 3, 18, 56, 61, pauseFrames, fireFrames, Weapon_Railgun_Fire);
}

/*
======================================================================

BFG10K

======================================================================
*/

static void Weapon_BFG_Fire(gentity_t *ent) {
	const bool q3 = RS(RS_Q3A);
	int damage = q3 ? 100 : (deathmatch->integer ? 200 : 500);
	int speed = q3 ? 1000 : 400;
	float radius = q3 ? 120.0f : 1000.0f;
	int ammoNeeded = q3 ? 10 : 50;

	// Show muzzle flash on windup frame only
	if (ent->client->ps.gunframe == 9) {
		gi.WriteByte(svc_muzzleflash);
		gi.WriteEntity(ent);
		gi.WriteByte(MZ_BFG | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS, false);
		PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);
		return;
	}

	// Abort if not enough ammo (could have been drained during windup)
	if (ent->client->pers.inventory[ent->client->pers.weapon->ammo] < ammoNeeded)
		return;

	if (is_quad)
		damage *= damageMultiplier;

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->vAngle, { 8.f, 8.f, -8.f }, start, dir);
	fire_bfg(ent, start, dir, damage, speed, radius);

	// Apply kickback
	if (q3) {
		P_AddWeaponKick(ent, ent->client->vForward * -2.f, { -1.f, 0.f, 0.f });
	} else {
		P_AddWeaponKick(ent, ent->client->vForward * -2.f, { -20.f, 0.f, crandom() * 8.f });
		ent->client->kick.total = DAMAGE_TIME();
		ent->client->kick.time = level.time + ent->client->kick.total;
	}

	// Fire flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_BFG2 | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	// Stats and ammo
	ent->client->pers.match.totalShots++;
	ent->client->pers.match.totalShotsPerWeapon[WEAP_BFG]++;
	RemoveAmmo(ent, ammoNeeded);
}

void Weapon_BFG(gentity_t *ent) {
	constexpr int pauseFrames[] = { 39, 45, 50, 55, 0 };
	constexpr int fireFrames[] = { 9, 17, 0 };
	constexpr int fireFramesQ3A[] = { 15, 17, 0 };

	Weapon_Generic(ent, 8, 32, 54, 58, pauseFrames, RS(RS_Q3A) ? fireFramesQ3A : fireFrames, Weapon_BFG_Fire);
}

/*
======================================================================

PROX MINES

======================================================================
*/

static void Weapon_ProxLauncher_Fire(gentity_t *ent) {
	// Clamp pitch to avoid backward firing
	const vec3_t launchAngles = {
		std::max(-62.5f, ent->client->vAngle[PITCH]),
		ent->client->vAngle[YAW],
		ent->client->vAngle[ROLL]
	};

	vec3_t start, dir;
	P_ProjectSource(ent, launchAngles, { 8.f, 8.f, -8.f }, start, dir);

	// Apply recoil
	P_AddWeaponKick(ent, ent->client->vForward * -2.f, { -1.f, 0.f, 0.f });

	// Fire prox mine
	fire_prox(ent, start, dir, damageMultiplier, 600);

	// Muzzle flash and sound
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_PROX | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	// Stats and ammo
	ent->client->pers.match.totalShots++;
	ent->client->pers.match.totalShotsPerWeapon[WEAP_PROX_LAUNCHER]++;
	RemoveAmmo(ent, 1);
}

void Weapon_ProxLauncher(gentity_t *ent) {
	constexpr int pauseFrames[] = { 34, 51, 59, 0 };
	constexpr int fireFrames[] = { 6, 0 };

	Weapon_Generic(ent, 5, 16, 59, 64, pauseFrames, fireFrames, Weapon_ProxLauncher_Fire);
}

/*
======================================================================

TESLA MINES

======================================================================
*/

static void Weapon_Tesla_Fire(gentity_t *ent, bool held) {
	// Determine firing direction with pitch limit
	vec3_t angles = {
		std::max(-62.5f, ent->client->vAngle[PITCH]),
		ent->client->vAngle[YAW],
		ent->client->vAngle[ROLL]
	};

	vec3_t start, dir;
	P_ProjectSource(ent, angles, { 0.f, 0.f, -22.f }, start, dir);

	// Calculate throw speed based on grenade hold time
	const gtime_t timer = ent->client->grenadeTime - level.time;
	const float tSec = std::clamp(timer.seconds(), 0.f, GRENADE_TIMER.seconds());
	const float speed = (ent->health <= 0)
		? GRENADE_MINSPEED
		: std::min(GRENADE_MINSPEED + tSec * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER.seconds()), GRENADE_MAXSPEED);

	ent->client->grenadeTime = 0_ms;

	// Fire tesla mine
	fire_tesla(ent, start, dir, damageMultiplier, static_cast<int>(speed));

	// Stats and ammo
	ent->client->pers.match.totalShots++;
	ent->client->pers.match.totalShotsPerWeapon[WEAP_TESLA_MINE]++;
	RemoveAmmo(ent, 1);
}

void Weapon_Tesla(gentity_t *ent) {
	constexpr int pauseFrames[] = { 21, 0 };

	Throw_Generic(ent, 8, 32, -1, nullptr, 1, 2, pauseFrames, false, nullptr, Weapon_Tesla_Fire, false);
}

/*
======================================================================

CHAINFIST

======================================================================
*/

static void Weapon_ChainFist_Fire(gentity_t *ent) {
	constexpr int32_t CHAINFIST_REACH = 24;

	// Stop attacking when fire is released on certain frames
	const int frame = ent->client->ps.gunframe;
	if (!(ent->client->buttons & BUTTON_ATTACK)) {
		if (frame == 13 || frame == 23 || frame >= 32) {
			ent->client->ps.gunframe = 33;
			return;
		}
	}

	// Determine damage
	int damage = deathmatch->integer ? 15 : 7;
	if (is_quad) {
		damage *= damageMultiplier;
	}

	vec3_t start{}, dir{};

	// Check for grenade-throwing variant
	if (GT(GT_BALL) && ent->client->pers.inventory[IT_BALL] > 0) {
		constexpr int pauseFrames[] = { 29, 34, 39, 48, 0 };
		Throw_Generic(
			ent,
			15, 48, 5,
			"weapons/hgrena1b.wav",
			11, 12,
			pauseFrames,
			true,
			"weapons/hgrenc1b.wav",
			Weapon_HandGrenade_Fire,
			true
		);

		gi.WriteByte(svc_muzzleflash);
		gi.WriteEntity(ent);
		gi.WriteByte(MZ_GRENADE | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS, false);

		PlayerNoise(ent, start, PNOISE_WEAPON);
		ent->client->pers.inventory[IT_BALL] = 0;
		return;
	}

	// Fire melee strike
	P_ProjectSource(ent, ent->client->vAngle, { 0, 0, -4 }, start, dir);

	if (fire_player_melee(ent, start, dir, CHAINFIST_REACH, damage, 100, MOD_CHAINFIST)) {
		if (ent->client->emptyClickSound < level.time) {
			ent->client->emptyClickSound = level.time + 500_ms;
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/sawslice.wav"), 1.f, ATTN_NORM, 0.f);
		}
	}

	PlayerNoise(ent, start, PNOISE_WEAPON);

	// Advance animation frame
	ent->client->ps.gunframe++;

	// Handle firing frame looping
	if (ent->client->buttons & BUTTON_ATTACK) {
		switch (ent->client->ps.gunframe) {
		case 12: ent->client->ps.gunframe = 14; break;
		case 22: ent->client->ps.gunframe = 24; break;
		case 32: ent->client->ps.gunframe = 7; break;
		}
	}

	// Start attack animation if needed
	if (ent->client->anim.priority != ANIM_ATTACK || frandom() < 0.25f) {
		ent->client->anim.priority = ANIM_ATTACK;
		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
			ent->s.frame = FRAME_crattak1 - 1;
			ent->client->anim.end = FRAME_crattak9;
		} else {
			ent->s.frame = FRAME_attack1 - 1;
			ent->client->anim.end = FRAME_attack8;
		}
		ent->client->anim.time = 0_ms;
	}
}

// this spits out some smoke from the motor. it's a two-stroke, you know.
static void Weapon_ChainFist_smoke(gentity_t *ent) {
	vec3_t tempVec, dir;
	P_ProjectSource(ent, ent->client->vAngle, { 8, 8, -4 }, tempVec, dir);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_CHAINFIST_SMOKE);
	gi.WritePosition(tempVec);
	gi.unicast(ent, 0);
}

void Weapon_ChainFist(gentity_t *ent) {
	constexpr int pauseFrames[] = { 0 };

	Weapon_Repeating(ent, 4, 32, 57, 60, pauseFrames, Weapon_ChainFist_Fire);

	// smoke on idle sequence
	if (ent->client->ps.gunframe == 42 && irandom(8)) {
		if ((ent->client->pers.hand != CENTER_HANDED) && frandom() < 0.4f)
			Weapon_ChainFist_smoke(ent);
	} else if (ent->client->ps.gunframe == 51 && irandom(8)) {
		if ((ent->client->pers.hand != CENTER_HANDED) && frandom() < 0.4f)
			Weapon_ChainFist_smoke(ent);
	}

	// set the appropriate weapon sound.
	if (ent->client->weaponState == WEAPON_FIRING)
		ent->client->weaponSound = gi.soundindex("weapons/sawhit.wav");
	else if (ent->client->weaponState == WEAPON_DROPPING)
		ent->client->weaponSound = 0;
	else if (ent->client->pers.weapon->id == IT_WEAPON_CHAINFIST)
		ent->client->weaponSound = gi.soundindex("weapons/sawidle.wav");
}

/*
======================================================================

DISRUPTOR

======================================================================
*/

static void Weapon_Disruptor_Fire(gentity_t *ent) {
	int damage = deathmatch->integer ? 45 : 135;
	if (is_quad) {
		damage *= damageMultiplier;
	}

	constexpr vec3_t kMins = { -16.f, -16.f, -16.f };
	constexpr vec3_t kMaxs = { 16.f,  16.f,  16.f };
	constexpr vec3_t kDistance = { 24.f, 8.f, -8.f };

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->vAngle, kDistance, start, dir);

	vec3_t end = start + (dir * 8192.f);
	gentity_t *target = nullptr;
	contents_t mask = MASK_PROJECTILE;

	// Disable player collision if needed
	if (!G_ShouldPlayersCollide(true)) {
		mask &= ~CONTENTS_PLAYER;
	}

	// Lag compensation
	LagCompensate(ent, start, dir);
	trace_t tr = gi.traceline(start, end, ent, mask);
	UnLagCompensate();

	// Attempt hit from point trace
	if (tr.ent != world && tr.ent->health > 0 &&
		((tr.ent->svFlags & SVF_MONSTER) || tr.ent->client || (tr.ent->flags & FL_DAMAGEABLE))) {
		target = tr.ent;
	} else {
		// Try expanded bounding box trace
		tr = gi.trace(start, kMins, kMaxs, end, ent, mask);
		if (tr.ent != world && tr.ent->health > 0 &&
			((tr.ent->svFlags & SVF_MONSTER) || tr.ent->client || (tr.ent->flags & FL_DAMAGEABLE))) {
			target = tr.ent;
		}
	}

	// Recoil
	P_AddWeaponKick(ent, ent->client->vForward * -2.f, { -1.f, 0.f, 0.f });

	// Fire weapon
	fire_disruptor(ent, start, dir, damage, 1000, target);

	// Muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_TRACKER | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	// Stats
	ent->client->pers.match.totalShots++;
	ent->client->pers.match.totalShotsPerWeapon[WEAP_DISRUPTOR]++;
	RemoveAmmo(ent, 1);
}

void Weapon_Disruptor(gentity_t *ent) {
	constexpr int pauseFrames[] = { 14, 19, 23, 0 };
	constexpr int fireFrames[] = { 5, 0 };

	Weapon_Generic(ent, 4, 9, 29, 34, pauseFrames, fireFrames, Weapon_Disruptor_Fire);
}

/*
======================================================================

ETF RIFLE

======================================================================
*/

static void Weapon_ETF_Rifle_Fire(gentity_t *ent) {
	const int baseDamage = 10;
	const int baseKick = 3;

	if (!(ent->client->buttons & BUTTON_ATTACK)) {
		ent->client->ps.gunframe = 8;
		return;
	}

	// Alternate muzzle flashes
	ent->client->ps.gunframe = (ent->client->ps.gunframe == 6) ? 7 : 6;

	// Ammo check
	if (ent->client->pers.inventory[ent->client->pers.weapon->ammo] < ent->client->pers.weapon->quantity) {
		ent->client->ps.gunframe = 8;
		NoAmmoWeaponChange(ent, true);
		return;
	}

	// Damage + kick scaling
	int damage = baseDamage;
	int kick = baseKick;
	if (is_quad) {
		damage *= damageMultiplier;
		kick *= damageMultiplier;
	}

	// Weapon kick randomness
	vec3_t kickOrigin{}, kickAngles{};
	for (int i = 0; i < 3; ++i) {
		kickOrigin[i] = crandom() * 0.85f;
		kickAngles[i] = crandom() * 0.85f;
	}
	P_AddWeaponKick(ent, kickOrigin, kickAngles);

	// Firing position offset
	vec3_t offset = (ent->client->ps.gunframe == 6) ? vec3_t{ 15.f, 8.f, -8.f } : vec3_t{ 15.f, 6.f, -8.f };

	// Compute firing start and direction
	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->vAngle + kickAngles, offset, start, dir);
	fire_flechette(ent, start, dir, damage, 1150, kick);

	Weapon_PowerupSound(ent);

	// Muzzle flash
	const int flashType = (ent->client->ps.gunframe == 6) ? MZ_ETF_RIFLE : MZ_ETF_RIFLE_2;
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(flashType | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	// Stats tracking
	ent->client->pers.match.totalShots++;
	ent->client->pers.match.totalShotsPerWeapon[WEAP_ETF_RIFLE]++;
	RemoveAmmo(ent, 1);

	// Animation
	ent->client->anim.priority = ANIM_ATTACK;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
		ent->s.frame = FRAME_crattak1 - static_cast<int>(frandom() + 0.25f);
		ent->client->anim.end = FRAME_crattak9;
	} else {
		ent->s.frame = FRAME_attack1 - static_cast<int>(frandom() + 0.25f);
		ent->client->anim.end = FRAME_attack8;
	}
	ent->client->anim.time = 0_ms;
}

void Weapon_ETF_Rifle(gentity_t *ent) {
	constexpr int pauseFrames[] = { 18, 28, 0 };

	Weapon_Repeating(ent, 4, 7, 37, 41, pauseFrames, Weapon_ETF_Rifle_Fire);
}

/*
======================================================================

PLASMA BEAM

======================================================================
*/

static void Weapon_PlasmaBeam_Fire(gentity_t *ent) {
	const bool firing = (ent->client->buttons & BUTTON_ATTACK) && !CombatIsDisabled();
	const bool hasAmmo = ent->client->pers.inventory[ent->client->pers.weapon->ammo] >= ent->client->pers.weapon->quantity;

	// Stop firing if no input or no ammo
	if (!firing || !hasAmmo) {
		ent->client->ps.gunframe = 13;
		ent->client->weaponSound = 0;
		ent->client->ps.gunSkin = 0;

		if (firing && !hasAmmo)
			NoAmmoWeaponChange(ent, true);
		return;
	}

	// Advance gunframe
	if (ent->client->ps.gunframe > 12)
		ent->client->ps.gunframe = 8;
	else
		ent->client->ps.gunframe++;

	if (ent->client->ps.gunframe == 12)
		ent->client->ps.gunframe = 8;

	// Set weapon sound and visual effects
	ent->client->weaponSound = gi.soundindex("weapons/tesla.wav");
	ent->client->ps.gunSkin = 1;

	// Determine damage and kick
	int damage = 15;
	int kick = 15;

	switch (game.ruleset) {
	case RS_Q3A:
		damage = deathmatch->integer ? 8 : 15;
		break;
	case RS_Q1:
		damage = 30;
		break;
	default:
		damage = deathmatch->integer ? 8 : 15;
		break;
	}
	kick = damage;

	if (is_quad) {
		damage *= damageMultiplier;
		kick *= damageMultiplier;
	}

	ent->client->kick.time = 0_ms;

	// Fire origin and direction
	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->vAngle, { 7.f, 2.f, -3.f }, start, dir);

	// Lag compensation for accurate hits
	LagCompensate(ent, start, dir);
	fire_plasmabeam(ent, start, dir, { 2.f, 7.f, -3.f }, damage, kick, false);
	UnLagCompensate();

	Weapon_PowerupSound(ent);

	// Muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_HEATBEAM | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	// Stats tracking
	ent->client->pers.match.totalShots++;
	ent->client->pers.match.totalShotsPerWeapon[WEAP_PLASMABEAM]++;
	RemoveAmmo(ent, RS(RS_Q1) ? 2 : 1);

	// Animation
	ent->client->anim.priority = ANIM_ATTACK;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED) {
		ent->s.frame = FRAME_crattak1 - static_cast<int>(frandom() + 0.25f);
		ent->client->anim.end = FRAME_crattak9;
	} else {
		ent->s.frame = FRAME_attack1 - static_cast<int>(frandom() + 0.25f);
		ent->client->anim.end = FRAME_attack8;
	}
	ent->client->anim.time = 0_ms;
}

void Weapon_PlasmaBeam(gentity_t *ent) {
	constexpr int pauseFrames[] = { 35, 0 };

	Weapon_Repeating(ent, 8, 12, 42, 47, pauseFrames, Weapon_PlasmaBeam_Fire);
}

/*
======================================================================

ION RIPPER

======================================================================
*/

static void Weapon_IonRipper_Fire(gentity_t *ent) {
	constexpr vec3_t muzzleOffset = { 16.f, 7.f, -8.f };
	constexpr int baseDamage = 50;
	constexpr int dmDamage = 20;
	constexpr int speed = 500;
	constexpr Effect effectFlags = EF_IONRIPPER;

	// Determine base damage
	int damage = deathmatch->integer ? dmDamage : baseDamage;
	if (is_quad)
		damage *= damageMultiplier;

	// Slight spread
	vec3_t firingAngles = ent->client->vAngle;
	firingAngles[YAW] += crandom();

	// Get firing direction and origin
	vec3_t start, dir;
	P_ProjectSource(ent, firingAngles, muzzleOffset, start, dir);

	// Apply recoil
	P_AddWeaponKick(ent, ent->client->vForward * -3.f, { -3.f, 0.f, 0.f });

	// Fire projectile
	fire_ionripper(ent, start, dir, damage, speed, effectFlags);

	// Muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_IONRIPPER | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	// Sound alert
	PlayerNoise(ent, start, PNOISE_WEAPON);

	// Stats
	ent->client->pers.match.totalShots++;
	ent->client->pers.match.totalShotsPerWeapon[WEAP_IONRIPPER]++;

	// Ammo consumption
	RemoveAmmo(ent, 1);
}

void Weapon_IonRipper(gentity_t *ent) {
	constexpr int pauseFrames[] = { 36, 0 };
	constexpr int fireFrames[] = { 6, 0 };

	Weapon_Generic(ent, 5, 7, 36, 39, pauseFrames, fireFrames, Weapon_IonRipper_Fire);
}

/*
======================================================================

PHALANX

======================================================================
*/

static void Weapon_Phalanx_Fire(gentity_t *ent) {
	constexpr int baseDamage = 80;
	constexpr float splashRadius = 100.f;
	constexpr int projectileSpeed = 725;
	constexpr vec3_t offset = { 0.f, 8.f, -8.f };

	int damage = baseDamage;
	int splashDamage = baseDamage;

	if (is_quad) {
		damage *= damageMultiplier;
		splashDamage *= damageMultiplier;
	}

	const bool isRightBarrel = (ent->client->ps.gunframe == 8);
	const float yawOffset = isRightBarrel ? -1.5f : 1.5f;
	const int muzzleFlashType = isRightBarrel ? MZ_PHALANX2 : MZ_PHALANX;

	vec3_t firingAngles = {
		ent->client->vAngle[PITCH],
		ent->client->vAngle[YAW] + yawOffset,
		ent->client->vAngle[ROLL]
	};

	vec3_t start, dir;
	P_ProjectSource(ent, firingAngles, offset, start, dir);

	fire_phalanx(ent, start, dir, damage, projectileSpeed, splashRadius, splashDamage);

	// Muzzle flash and sound
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(muzzleFlashType | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	if (isRightBarrel) {
		ent->client->pers.match.totalShots += 2;
		ent->client->pers.match.totalShotsPerWeapon[WEAP_PHALANX] += 2;
		RemoveAmmo(ent, 1);
	} else {
		PlayerNoise(ent, start, PNOISE_WEAPON);
	}

	// Add weapon kick
	P_AddWeaponKick(ent, ent->client->vForward * -2.f, { -2.f, 0.f, 0.f });
}

void Weapon_Phalanx(gentity_t *ent) {
	constexpr int pauseFrames[] = { 29, 42, 55, 0 };
	constexpr int fireFrames[] = { 7, 8, 0 };

	Weapon_Generic(ent, 5, 20, 58, 63, pauseFrames, fireFrames, Weapon_Phalanx_Fire);
}

/*
======================================================================

TRAP

======================================================================
*/

static void Weapon_Trap_Fire(gentity_t *ent, bool held) {
	constexpr gtime_t TRAP_TIMER = 5_sec;
	constexpr float TRAP_MINSPEED = 300.f;
	constexpr float TRAP_MAXSPEED = 700.f;
	constexpr float TRAP_THROW_OFFSET_Z = -8.f;

	vec3_t start, dir;

	// Clamp pitch to avoid backwards throws and eliminate sideways offset
	vec3_t clampedAngles = {
		std::max(-62.5f, ent->client->vAngle[PITCH]),
		ent->client->vAngle[YAW],
		ent->client->vAngle[ROLL]
	};

	// Calculate projectile start and direction
	P_ProjectSource(ent, clampedAngles, { 8, 0, TRAP_THROW_OFFSET_Z }, start, dir);

	// Calculate speed based on how long the trap was held
	gtime_t heldTime = ent->client->grenadeTime - level.time;
	float speed = TRAP_MINSPEED;

	if (ent->health > 0) {
		float timeHeldSec = std::clamp(heldTime.seconds(), 0.0f, TRAP_TIMER.seconds());
		speed = TRAP_MINSPEED + timeHeldSec * ((TRAP_MAXSPEED - TRAP_MINSPEED) / TRAP_TIMER.seconds());
	}

	speed = std::min(speed, TRAP_MAXSPEED);
	ent->client->grenadeTime = 0_ms;

	fire_trap(ent, start, dir, static_cast<int>(speed));

	// Track usage stats
	ent->client->pers.match.totalShots++;
	ent->client->pers.match.totalShotsPerWeapon[WEAP_TRAP]++;
	RemoveAmmo(ent, 1);
}

void Weapon_Trap(gentity_t *ent) {
	constexpr int pauseFrames[] = { 29, 34, 39, 48, 0 };

	Throw_Generic(ent, 15, 48, 5, "weapons/trapcock.wav", 11, 12, pauseFrames, false, "weapons/traploop.wav", Weapon_Trap_Fire, false);
}
