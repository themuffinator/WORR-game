// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "g_local.h"
#include "monsters/m_player.h"
#include "bots/bot_includes.h"

static gentity_t *currentPlayer;
static gclient_t *currentClient;

static vec3_t forward, right, up;
float		  xySpeed;

float bobMove;
int	  bobCycle, bobCycleRun;	  // odd cycles are right foot going forward
float bobFracSin; // sinf(bobfrac*M_PI)

/*
===============
SkipViewModifiers
===============
*/
static inline bool SkipViewModifiers() {
	if (g_skip_view_modifiers->integer && g_cheats->integer)
		return true;

	// don't do bobbing, etc on grapple
	if (currentClient->grappleEnt &&
			currentClient->grappleState > GRAPPLE_STATE_FLY) {
		return true;
	}

	// spectator mode
	if (!ClientIsPlaying(currentClient))
		return true;

	return false;
}

/*
===============
P_CalcRoll
===============
*/
static float P_CalcRoll(const vec3_t &angles, const vec3_t &velocity) {
	if (SkipViewModifiers()) {
		return 0.0f;
	}

	float sign;
	float side;
	float value;

	side = velocity.dot(right);
	sign = side < 0 ? -1.f : 1.f;
	side = fabsf(side);

	value = g_rollangle->value;

	if (side < g_rollspeed->value)
		side = side * value / g_rollspeed->value;
	else
		side = value;

	return side * sign;
}

/*
===============
P_DamageFeedback

Handles color blends and view kicks
===============
*/
static void P_DamageFeedback(gentity_t *player) {
	gclient_t *client;
	float			 side;
	float			 realcount, count, kick;
	vec3_t			 v;
	int				 l;
	constexpr vec3_t armor_color = { 1.0, 1.0, 1.0 };
	constexpr vec3_t power_color = { 0.0, 1.0, 0.0 };
	constexpr vec3_t bcolor = { 1.0, 0.0, 0.0 };

	client = player->client;

	// flash the backgrounds behind the status numbers
	int16_t want_flashes = 0;

	if (client->damage.blood)
		want_flashes |= 1;
	if (client->damage.armor && !(player->flags & FL_GODMODE))
		want_flashes |= 2;

	if (want_flashes) {
		client->flashTime = level.time + 100_ms;
		client->ps.stats[STAT_FLASHES] = want_flashes;
	} else if (client->flashTime < level.time)
		client->ps.stats[STAT_FLASHES] = 0;

	// total points of damage shot at the player this frame
	count = (float)(client->damage.blood + client->damage.armor + client->damage.powerArmor);
	if (count == 0)
		return; // didn't take any damage

	// start a pain animation if still in the player model
	if (client->anim.priority < ANIM_PAIN && player->s.modelindex == MODELINDEX_PLAYER) {
		static int i;

		client->anim.priority = ANIM_PAIN;
		if (client->ps.pmove.pm_flags & PMF_DUCKED) {
			player->s.frame = FRAME_crpain1 - 1;
			client->anim.end = FRAME_crpain4;
		} else {
			i = (i + 1) % 3;
			switch (i) {
			case 0:
				player->s.frame = FRAME_pain101 - 1;
				client->anim.end = FRAME_pain104;
				break;
			case 1:
				player->s.frame = FRAME_pain201 - 1;
				client->anim.end = FRAME_pain204;
				break;
			case 2:
				player->s.frame = FRAME_pain301 - 1;
				client->anim.end = FRAME_pain304;
				break;
			}
		}

		client->anim.time = 0_ms;
	}

	realcount = count;

	// if we took health damage, do a minimum clamp
	if (client->damage.blood) {
		if (count < 10)
			count = 10; // always make a visible effect
	} else {
		if (count > 2)
			count = 2; // don't go too deep
	}

	// play an appropriate pain sound
	if ((level.time > player->pain_debounce_time) && !(player->flags & FL_GODMODE)) {
		player->pain_debounce_time = level.time + 700_ms;

		constexpr const char *pain_sounds[] = {
			"*pain25_1.wav",
			"*pain25_2.wav",
			"*pain50_1.wav",
			"*pain50_2.wav",
			"*pain75_1.wav",
			"*pain75_2.wav",
			"*pain100_1.wav",
			"*pain100_2.wav",
		};

		if (player->health < 25)
			l = 0;
		else if (player->health < 50)
			l = 2;
		else if (player->health < 75)
			l = 4;
		else
			l = 6;

		if (brandom())
			l |= 1;

		gi.sound(player, CHAN_VOICE, gi.soundindex(pain_sounds[l]), 1, ATTN_NORM, 0);
		// Paril: pain noises alert monsters
		PlayerNoise(player, player->s.origin, PNOISE_SELF);
	}

	// the total alpha of the blend is always proportional to count
	if (client->damageAlpha < 0)
		client->damageAlpha = 0;

	// [Paril-KEX] tweak the values to rely less on this
	// and more on damage indicators
	if (client->damage.blood || (client->damageAlpha + count * 0.06f) < 0.15f) {
		client->damageAlpha += count * 0.06f;

		if (client->damageAlpha < 0.06f)
			client->damageAlpha = 0.06f;
		if (client->damageAlpha > 0.4f)
			client->damageAlpha = 0.4f; // don't go too saturated
	}

	// mix in colors
	v = {};

	if (client->damage.powerArmor)
		v += power_color * (client->damage.powerArmor / realcount);
	if (client->damage.blood)
		v += bcolor * max(15.0f, (client->damage.blood / realcount));
	if (client->damage.armor)
		v += armor_color * (client->damage.armor / realcount);
	client->damageBlend = v.normalized();

	//
	// calculate view angle kicks
	//
	kick = (float)abs(client->damage.knockback);
	if (kick && player->health > 0) // kick of 0 means no view adjust at all
	{
		kick = kick * 100 / player->health;

		if (kick < count * 0.5f)
			kick = count * 0.5f;
		if (kick > 50)
			kick = 50;

		v = client->damage.origin - player->s.origin;
		v.normalize();

		side = v.dot(right);
		client->vDamageRoll = kick * side * 0.3f;

		side = -v.dot(forward);
		client->vDamagePitch = kick * side * 0.3f;

		client->vDamageTime = level.time + DAMAGE_TIME();
	}

	// [Paril-KEX] send view indicators
	if (client->numDamageIndicators) {
		gi.WriteByte(svc_damage);
		gi.WriteByte(client->numDamageIndicators);

		for (size_t i = 0; i < client->numDamageIndicators; i++) {
			auto &indicator = client->damageIndicators[i];

			// encode total damage into 5 bits
			uint8_t encoded = std::clamp((indicator.health + indicator.power + indicator.armor) / 3, 1, 0x1F);

			// encode types in the latter 3 bits
			if (indicator.health)
				encoded |= 0x20;
			if (indicator.armor)
				encoded |= 0x40;
			if (indicator.power)
				encoded |= 0x80;

			gi.WriteByte(encoded);
			gi.WriteDir((player->s.origin - indicator.from).normalized());
		}

		gi.unicast(player, false);
	}

	//
	// clear totals
	//
	client->damage.blood = 0;
	client->damage.armor = 0;
	client->damage.powerArmor = 0;
	client->damage.knockback = 0;
	client->numDamageIndicators = 0;
}

/*
===============
G_CalcViewOffset

Auto pitching on slopes?

  fall from 128: 400 = 160000
  fall from 256: 580 = 336400
  fall from 384: 720 = 518400
  fall from 512: 800 = 640000
  fall from 640: 960 =

  damage = deltavelocity*deltavelocity  * 0.0001

===============
*/
static void G_CalcViewOffset(gentity_t *ent) {
	float  bob;
	float  ratio;
	float  delta;
	vec3_t v;

	//===================================

	// base angles
	vec3_t &angles = ent->client->ps.kick_angles;

	// if dead, fix the angle and don't add any kick
	if (ent->deadFlag && ClientIsPlaying(ent->client)) {
		angles = {};

		if (ent->flags & FL_SAM_RAIMI) {
			ent->client->ps.viewangles[ROLL] = 0;
			ent->client->ps.viewangles[PITCH] = 0;
		} else {
			ent->client->ps.viewangles[ROLL] = 40;
			ent->client->ps.viewangles[PITCH] = -15;
		}
		ent->client->ps.viewangles[YAW] = ent->client->killer_yaw;
	} else if (!ent->client->pers.bob_skip && !SkipViewModifiers()) {
		// add angles based on weapon kick
		angles = P_CurrentKickAngles(ent);

		// add angles based on damage kick
		if (ent->client->vDamageTime > level.time) {
			// [Paril-KEX] 100ms of slack is added to account for
			// visual difference in higher tickrates
			gtime_t diff = ent->client->vDamageTime - level.time;

			// slack time remaining
			if (DAMAGE_TIME_SLACK()) {
				if (diff > DAMAGE_TIME() - DAMAGE_TIME_SLACK())
					ratio = (DAMAGE_TIME() - diff).seconds() / DAMAGE_TIME_SLACK().seconds();
				else
					ratio = diff.seconds() / (DAMAGE_TIME() - DAMAGE_TIME_SLACK()).seconds();
			} else
				ratio = diff.seconds() / (DAMAGE_TIME() - DAMAGE_TIME_SLACK()).seconds();

			angles[PITCH] += ratio * ent->client->vDamagePitch;
			angles[ROLL] += ratio * ent->client->vDamageRoll;
		}

		// add pitch based on fall kick
		if (ent->client->fallTime > level.time) {
			// [Paril-KEX] 100ms of slack is added to account for
			// visual difference in higher tickrates
			gtime_t diff = ent->client->fallTime - level.time;

			// slack time remaining
			if (DAMAGE_TIME_SLACK()) {
				if (diff > FALL_TIME() - DAMAGE_TIME_SLACK())
					ratio = (FALL_TIME() - diff).seconds() / DAMAGE_TIME_SLACK().seconds();
				else
					ratio = diff.seconds() / (FALL_TIME() - DAMAGE_TIME_SLACK()).seconds();
			} else
				ratio = diff.seconds() / (FALL_TIME() - DAMAGE_TIME_SLACK()).seconds();
			angles[PITCH] += ratio * ent->client->fallValue;
		}

		// add angles based on velocity
		if (!ent->client->pers.bob_skip && !SkipViewModifiers()) {
			delta = ent->velocity.dot(forward);
			angles[PITCH] += delta * run_pitch->value;

			delta = ent->velocity.dot(right);
			angles[ROLL] += delta * run_roll->value;

			// add angles based on bob
			delta = bobFracSin * bob_pitch->value * xySpeed;
			if ((ent->client->ps.pmove.pm_flags & PMF_DUCKED) && ent->groundEntity)
				delta *= 6; // crouching
			delta = min(delta, 1.2f);
			angles[PITCH] += delta;
			delta = bobFracSin * bob_roll->value * xySpeed;
			if ((ent->client->ps.pmove.pm_flags & PMF_DUCKED) && ent->groundEntity)
				delta *= 6; // crouching
			delta = min(delta, 1.2f);
			if (bobCycle & 1)
				delta = -delta;
			angles[ROLL] += delta;
		}

		// add earthquake angles
		if (ent->client->quakeTime > level.time) {
			float factor = min(1.0f, (ent->client->quakeTime.seconds() / level.time.seconds()) * 0.25f);

			angles.x += crandom_open() * factor;
			angles.z += crandom_open() * factor;
			angles.y += crandom_open() * factor;
		}
	}

	// [Paril-KEX] clamp angles
	for (int i = 0; i < 3; i++)
		angles[i] = clamp(angles[i], -31.f, 31.f);

	//===================================

	// base origin

	v = {};

	// add fall height

	if (!ent->client->pers.bob_skip && !SkipViewModifiers()) {
		if (ent->client->fallTime > level.time) {
			// [Paril-KEX] 100ms of slack is added to account for
			// visual difference in higher tickrates
			gtime_t diff = ent->client->fallTime - level.time;

			// slack time remaining
			if (DAMAGE_TIME_SLACK()) {
				if (diff > FALL_TIME() - DAMAGE_TIME_SLACK())
					ratio = (FALL_TIME() - diff).seconds() / DAMAGE_TIME_SLACK().seconds();
				else
					ratio = diff.seconds() / (FALL_TIME() - DAMAGE_TIME_SLACK()).seconds();
			} else
				ratio = diff.seconds() / (FALL_TIME() - DAMAGE_TIME_SLACK()).seconds();
			v[2] -= ratio * ent->client->fallValue * 0.4f;
		}

		// add bob height
		bob = bobFracSin * xySpeed * bob_up->value;
		if (bob > 6)
			bob = 6;
		// gi.DebugGraph (bob *2, 255);
		v[2] += bob;
	}

	// add kick offset

	if (!ent->client->pers.bob_skip && !SkipViewModifiers())
		v += P_CurrentKickOrigin(ent);

	// absolutely bound offsets
	// so the view can never be outside the player box

	if (v[0] < -14)
		v[0] = -14;
	else if (v[0] > 14)
		v[0] = 14;
	if (v[1] < -14)
		v[1] = -14;
	else if (v[1] > 14)
		v[1] = 14;
	if (v[2] < -22)
		v[2] = -22;
	else if (v[2] > 30)
		v[2] = 30;

	ent->client->ps.viewoffset = v;
}

/*
==============
G_CalcGunOffset
==============
*/
static void G_CalcGunOffset(gentity_t *ent) {
	int	  i;

	if (ent->client->pers.weapon &&
		!((ent->client->pers.weapon->id == IT_WEAPON_PLASMABEAM || ent->client->pers.weapon->id == IT_WEAPON_GRAPPLE) && ent->client->weaponState == WEAPON_FIRING)
		&& !SkipViewModifiers()) {
		// gun angles from bobbing
		ent->client->ps.gunangles[ROLL] = xySpeed * bobFracSin * 0.005f;
		ent->client->ps.gunangles[YAW] = xySpeed * bobFracSin * 0.01f;
		if (bobCycle & 1) {
			ent->client->ps.gunangles[ROLL] = -ent->client->ps.gunangles[ROLL];
			ent->client->ps.gunangles[YAW] = -ent->client->ps.gunangles[YAW];
		}

		ent->client->ps.gunangles[PITCH] = xySpeed * bobFracSin * 0.005f;

		vec3_t viewangles_delta = ent->client->oldViewAngles - ent->client->ps.viewangles;

		for (i = 0; i < 3; i++)
			ent->client->slow_view_angles[i] += viewangles_delta[i];

		// gun angles from delta movement
		for (i = 0; i < 3; i++) {
			float &d = ent->client->slow_view_angles[i];

			if (!d)
				continue;

			if (d > 180)
				d -= 360;
			if (d < -180)
				d += 360;
			if (d > 45)
				d = 45;
			if (d < -45)
				d = -45;

			// [Sam-KEX] Apply only half-delta. Makes the weapons look less detatched from the player.
			if (i == ROLL)
				ent->client->ps.gunangles[i] += (0.1f * d) * 0.5f;
			else
				ent->client->ps.gunangles[i] += (0.2f * d) * 0.5f;

			float reduction_factor = viewangles_delta[i] ? 0.05f : 0.15f;

			if (d > 0)
				d = max(0.f, d - gi.frame_time_ms * reduction_factor);
			else if (d < 0)
				d = min(0.f, d + gi.frame_time_ms * reduction_factor);
		}

		// [Paril-KEX] cl_rollhack
		ent->client->ps.gunangles[ROLL] = -ent->client->ps.gunangles[ROLL];
	} else {
		for (i = 0; i < 3; i++)
			ent->client->ps.gunangles[i] = 0;
	}

	// gun height
	ent->client->ps.gunoffset = {};

	// gun_x / gun_y / gun_z are development tools
	for (i = 0; i < 3; i++) {
		ent->client->ps.gunoffset[i] += forward[i] * (gun_y->value);
		ent->client->ps.gunoffset[i] += right[i] * gun_x->value;
		ent->client->ps.gunoffset[i] += up[i] * (-gun_z->value);
	}
}

/*
=============
G_CalcBlend
=============
*/

[[nodiscard]] static float G_PowerUpFadeAlpha(gtime_t left, float max_alpha = 0.08f) {
	if (left.milliseconds() > 3000)
		return max_alpha;

	float phase = static_cast<float>(left.milliseconds()) * 2.0f * M_PI / 1000.0f;
	return (std::sin(phase) * 0.5f + 0.5f) * max_alpha;
}

static void G_CalcBlend(gentity_t *ent) {
	gtime_t remaining;
	ent->client->ps.damageBlend = ent->client->ps.screen_blend = {};

	auto BlendIfExpiring = [&](gtime_t end_time, float r, float g, float b, float max_alpha, const char *sound = nullptr) {
		if (end_time > level.time) {
			remaining = end_time - level.time;
			if (remaining.milliseconds() == 3000 && sound)
				gi.sound(ent, CHAN_ITEM, gi.soundindex(sound), 1, ATTN_NORM, 0);
			if (G_PowerUpExpiringRelative(remaining))
				G_AddBlend(r, g, b, G_PowerUpFadeAlpha(remaining, max_alpha), ent->client->ps.screen_blend);
		}
		};

	// Powerups
	if (ent->client->powerupTime.spawnProtection > level.time) {
		G_AddBlend(1, 0, 0, 0.05f, ent->client->ps.screen_blend);
	}
	BlendIfExpiring(ent->client->powerupTime.quadDamage, 0, 0, 1, 0.08f, "items/damage2.wav");
	BlendIfExpiring(ent->client->powerupTime.haste, 1, 0.2f, 0.5f, 0.08f, "items/quadfire2.wav");
	BlendIfExpiring(ent->client->powerupTime.doubleDamage, 0.9f, 0.1f, 0.1f, 0.08f, "misc/ddamage2.wav");
	BlendIfExpiring(ent->client->powerupTime.battleSuit, 0.9f, 0.7f, 0, 0.08f, "items/protect2.wav");
	BlendIfExpiring(ent->client->powerupTime.invisibility, 0.8f, 0.8f, 0.8f, 0.08f, "items/protect2.wav");
	BlendIfExpiring(ent->client->powerupTime.enviroSuit, 0, 1, 0, 0.08f, "items/airout.wav");
	BlendIfExpiring(ent->client->powerupTime.rebreather, 0.4f, 1, 0.4f, 0.04f, "items/airout.wav");

	// Freeze effect
	if (GT(GT_FREEZE) && ent->client->eliminated && !ent->client->followTarget && !ent->client->resp.thawer) {
		G_AddBlend(0.5f, 0.5f, 0.6f, 0.4f, ent->client->ps.screen_blend);
	}

	// Nuke effect
	if (ent->client->nukeTime > level.time) {
		float brightness = (ent->client->nukeTime - level.time).seconds() / 2.0f;
		G_AddBlend(1, 1, 1, brightness, ent->client->ps.screen_blend);
	}

	// IR goggles
	if (ent->client->powerupTime.irGoggles > level.time) {
		remaining = ent->client->powerupTime.irGoggles - level.time;
		if (G_PowerUpExpiringRelative(remaining)) {
			ent->client->ps.rdflags |= RDF_IRGOGGLES;
			G_AddBlend(1, 0, 0, 0.2f, ent->client->ps.screen_blend);
		} else {
			ent->client->ps.rdflags &= ~RDF_IRGOGGLES;
		}
	} else {
		ent->client->ps.rdflags &= ~RDF_IRGOGGLES;
	}

	// Damage blend
	if (ent->client->damageAlpha > 0)
		G_AddBlend(ent->client->damageBlend[0], ent->client->damageBlend[1], ent->client->damageBlend[2], ent->client->damageAlpha, ent->client->ps.damageBlend);

	// Drowning
	if (ent->airFinished < level.time + 9_sec) {
		constexpr vec3_t drown_color = { 0.1f, 0.1f, 0.2f };
		constexpr float max_drown_alpha = 0.75f;
		float alpha = (ent->airFinished < level.time) ? 1 : (1.f - ((ent->airFinished - level.time).seconds() / 9.0f));
		G_AddBlend(drown_color[0], drown_color[1], drown_color[2], std::min(alpha, max_drown_alpha), ent->client->ps.damageBlend);
	}

	// Decay blend values
	ent->client->damageAlpha = std::max(0.0f, ent->client->damageAlpha - gi.frame_time_s * 0.6f);
	ent->client->bonusAlpha = std::max(0.0f, ent->client->bonusAlpha - gi.frame_time_s);
}
/*
=============
P_WorldEffects
=============
*/
static void P_WorldEffects() {
	if (level.timeoutActive)
		return;

	// Freecam or following
	if (currentPlayer->moveType == MOVETYPE_FREECAM || currentPlayer->client->followTarget) {
		currentPlayer->airFinished = level.time + 12_sec;
		return;
	}

	constexpr int32_t max_drown_dmg = 15;

	const auto waterLevel = currentPlayer->waterlevel;
	const auto oldWaterLevel = currentClient->oldWaterLevel;
	currentClient->oldWaterLevel = waterLevel;

	const bool breather = currentClient->powerupTime.rebreather > level.time;
	const bool enviroSuit = currentClient->powerupTime.enviroSuit > level.time;
	const bool battleSuit = currentClient->powerupTime.battleSuit > level.time;
	const bool spawnProtection = currentClient->powerupTime.spawnProtection > level.time;
	const bool anyProtection = breather || enviroSuit || battleSuit || spawnProtection;

	auto PlaySound = [](gentity_t *ent, soundchan_t chan, const char *sfx) {
		gi.sound(ent, chan, gi.soundindex(sfx), 1, ATTN_NORM, 0);
		};
	auto PlayerSfxNoise = []() {
		PlayerNoise(currentPlayer, currentPlayer->s.origin, PNOISE_SELF);
		};

	// Water enter
	if (!oldWaterLevel && waterLevel) {
		PlayerSfxNoise();
		const int watertype = currentPlayer->watertype;
		if (watertype & CONTENTS_LAVA)
			PlaySound(currentPlayer, CHAN_BODY, "player/lava_in.wav");
		else if (watertype & (CONTENTS_SLIME | CONTENTS_WATER))
			PlaySound(currentPlayer, CHAN_BODY, "player/watr_in.wav");

		currentPlayer->flags |= FL_INWATER;
		currentPlayer->damage_debounce_time = level.time - 1_sec;
	}

	// Water exit
	if (oldWaterLevel && !waterLevel) {
		PlayerSfxNoise();
		PlaySound(currentPlayer, CHAN_BODY, "player/watr_out.wav");
		currentPlayer->flags &= ~FL_INWATER;
	}

	// Head submerged
	if (oldWaterLevel != WATER_UNDER && waterLevel == WATER_UNDER) {
		PlaySound(currentPlayer, CHAN_BODY, "player/watr_un.wav");
	}

	// Head resurfaces
	if (currentPlayer->health > 0 && oldWaterLevel == WATER_UNDER && waterLevel != WATER_UNDER) {
		if (currentPlayer->airFinished < level.time) {
			PlaySound(currentPlayer, CHAN_VOICE, "player/gasp1.wav");
			PlayerSfxNoise();
		} else if (currentPlayer->airFinished < level.time + 11_sec) {
			PlaySound(currentPlayer, CHAN_VOICE, "player/gasp2.wav");
		}
	}

	// Drowning
	if (waterLevel == WATER_UNDER) {
		if (anyProtection) {
			currentPlayer->airFinished = level.time + 10_sec;
			if (((currentClient->powerupTime.rebreather - level.time).milliseconds() % 2500) == 0) {
				const char *breathSound = currentClient->breatherSound ? "player/u_breath2.wav" : "player/u_breath1.wav";
				PlaySound(currentPlayer, CHAN_AUTO, breathSound);
				currentClient->breatherSound ^= 1;
				PlayerSfxNoise();
			}
		}

		if (currentPlayer->airFinished < level.time && currentPlayer->health > 0) {
			if (currentClient->nextDrownTime < level.time) {
				currentClient->nextDrownTime = level.time + 1_sec;

				currentPlayer->dmg = std::min(currentPlayer->dmg + 2, max_drown_dmg);
				const char *sfx = (currentPlayer->health <= currentPlayer->dmg)
					? "*drown1.wav"
					: (brandom() ? "*gurp1.wav" : "*gurp2.wav");
				PlaySound(currentPlayer, CHAN_VOICE, sfx);

				currentPlayer->pain_debounce_time = level.time;

				Damage(currentPlayer, world, world, vec3_origin, currentPlayer->s.origin,
					vec3_origin, currentPlayer->dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
			}
		} else if (currentPlayer->airFinished <= level.time + 3_sec &&
			currentClient->nextDrownTime < level.time) {
			std::string name = fmt::format("player/wade{}.wav", 1 + (static_cast<int32_t>(level.time.seconds()) % 3));
			PlaySound(currentPlayer, CHAN_VOICE, name.c_str());
			currentClient->nextDrownTime = level.time + 1_sec;
		}
	} else {
		currentPlayer->airFinished = level.time + 12_sec;
		currentPlayer->dmg = 2;
	}

	// Lava or slime damage
	if (waterLevel && (currentPlayer->watertype & (CONTENTS_LAVA | CONTENTS_SLIME)) &&
		currentPlayer->slime_debounce_time <= level.time) {

		const bool immune = enviroSuit || battleSuit || spawnProtection;
		const int watertype = currentPlayer->watertype;

		if (watertype & CONTENTS_LAVA) {
			if (currentPlayer->health > 0 && currentPlayer->pain_debounce_time <= level.time) {
				PlaySound(currentPlayer, CHAN_VOICE, brandom() ? "player/burn1.wav" : "player/burn2.wav");
				if (immune)
					PlaySound(currentPlayer, CHAN_AUX, "items/protect3.wav");
				currentPlayer->pain_debounce_time = level.time + 1_sec;
			}
			const int dmg = (spawnProtection ? 0 : (enviroSuit || battleSuit ? 1 : 3)) * waterLevel;
			Damage(currentPlayer, world, world, vec3_origin, currentPlayer->s.origin, vec3_origin, dmg, 0, DAMAGE_NONE, MOD_LAVA);
		}

		if (watertype & CONTENTS_SLIME) {
			if (!(enviroSuit || battleSuit)) {
				Damage(currentPlayer, world, world, vec3_origin, currentPlayer->s.origin,
					vec3_origin, 1 * waterLevel, 0, DAMAGE_NONE, MOD_SLIME);
			} else if (currentPlayer->health > 0 && currentPlayer->pain_debounce_time <= level.time) {
				PlaySound(currentPlayer, CHAN_AUX, "items/protect3.wav");
				currentPlayer->pain_debounce_time = level.time + 1_sec;
			}
		}
		currentPlayer->slime_debounce_time = level.time + 10_hz;
	}
}

/*
===============
ClientSetEffects
===============
*/
static void ClientSetEffects(gentity_t *ent) {
	int pa_type;

	ent->s.effects = EF_NONE;
	ent->s.renderfx &= RF_STAIR_STEP;
	ent->s.renderfx |= RF_IR_VISIBLE;
	ent->s.alpha = 1.0;

	if (ent->health <= 0 || ent->client->eliminated || level.intermissionTime)
		return;

	if (ent->flags & FL_FLASHLIGHT)
		ent->s.effects |= EF_FLASHLIGHT;

	if (ent->flags & FL_DISGUISED)
		ent->s.renderfx |= RF_USE_DISGUISE;

	if (ent->powerarmor_time > level.time) {
		pa_type = PowerArmorType(ent);
		if (pa_type == IT_POWER_SCREEN) {
			ent->s.effects |= EF_POWERSCREEN;
		} else if (pa_type == IT_POWER_SHIELD) {
			ent->s.effects |= EF_COLOR_SHELL;
			ent->s.renderfx |= RF_SHELL_GREEN;
		}
	}

	if (ent->client->pu_regen_time_blip > level.time) {
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= RF_SHELL_RED;
	}

	if (ent->client->pu_time_spawn_protection_blip > level.time) {
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= RF_SHELL_RED;
	}

	CTF_ClientEffects(ent);

	if (GT(GT_BALL) && ent->client->pers.inventory[IT_BALL] > 0) {
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= RF_SHELL_RED | RF_SHELL_GREEN;
	}

	if (ent->client->powerupTime.quadDamage > level.time)
		if (G_PowerUpExpiring(ent->client->powerupTime.quadDamage))
			ent->s.effects |= EF_QUAD;
	if (ent->client->powerupTime.battleSuit > level.time)
		if (G_PowerUpExpiring(ent->client->powerupTime.battleSuit))
			ent->s.effects |= EF_PENT;
	if (ent->client->powerupTime.haste > level.time)
		if (G_PowerUpExpiring(ent->client->powerupTime.haste))
			ent->s.effects |= EF_DUALFIRE;
	if (ent->client->powerupTime.doubleDamage > level.time)
		if (G_PowerUpExpiring(ent->client->powerupTime.doubleDamage))
			ent->s.effects |= EF_DOUBLE;
	if ((ent->client->ownedSphere) && (ent->client->ownedSphere->spawnflags == SF_SPHERE_DEFENDER))
		ent->s.effects |= EF_HALF_DAMAGE;
	if (ent->client->trackerPainTime > level.time)
		ent->s.effects |= EF_TRACKERTRAIL;
	if (ent->client->powerupTime.invisibility > level.time) {
		if (ent->client->invisibility_fade_time <= level.time)
			ent->s.alpha = 0.05f;
		else {
			float x = (ent->client->invisibility_fade_time - level.time).seconds() / INVISIBILITY_TIME.seconds();
			ent->s.alpha = std::clamp(x, 0.0125f, 0.2f);
		}
	}
}

/*
===============
ClientSetEvent
===============
*/
static void ClientSetEvent(gentity_t *ent) {
	if (level.timeoutActive)
		return;

	if (ent->s.event)
		return;

	if (RS(RS_Q1))
		return;

	if (ent->client->ps.pmove.pm_flags & PMF_ON_LADDER) {
		if (g_ladderSteps->integer > 1 || (g_ladderSteps->integer == 1 && !deathmatch->integer)) {
			if (currentClient->last_ladder_sound < level.time &&
				(currentClient->last_ladder_pos - ent->s.origin).length() > 48.f) {
				ent->s.event = EV_LADDER_STEP;
				currentClient->last_ladder_pos = ent->s.origin;
				currentClient->last_ladder_sound = level.time + LADDER_SOUND_TIME;
			}
		}
	} else if (ent->groundEntity && xySpeed > 225) {
		if ((int)(currentClient->bobTime + bobMove) != bobCycleRun)
			ent->s.event = EV_FOOTSTEP;
	}
}

/*
===============
ClientSetSound
===============
*/
static void ClientSetSound(gentity_t *ent) {
	if (level.timeoutActive)
		return;

	// help beep (no more than three times)
	if (ent->client->pers.helpchanged && ent->client->pers.helpchanged <= 3 && ent->client->pers.help_time < level.time) {
		if (ent->client->pers.helpchanged == 1) // [KEX] haleyjd: once only
			gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/pc_up.wav"), 1, ATTN_STATIC, 0);
		ent->client->pers.helpchanged++;
		ent->client->pers.help_time = level.time + 5_sec;
	}

	// reset defaults
	ent->s.sound = 0;
	ent->s.loop_attenuation = 0;
	ent->s.loop_volume = 0;

	if (ent->waterlevel && (ent->watertype & (CONTENTS_LAVA | CONTENTS_SLIME))) {
		ent->s.sound = snd_fry;
		return;
	}

	if (ent->deadFlag || !ClientIsPlaying(ent->client) || ent->client->eliminated)
		return;

	if (ent->client->weaponSound)
		ent->s.sound = ent->client->weaponSound;
	else if (ent->client->pers.weapon) {
		switch (ent->client->pers.weapon->id) {
		case IT_WEAPON_RAILGUN:
			ent->s.sound = gi.soundindex("weapons/rg_hum.wav");
			break;
		case IT_WEAPON_BFG:
		case IT_WEAPON_PLASMABEAM:
			ent->s.sound = gi.soundindex("weapons/bfg_hum.wav");
			break;
		case IT_WEAPON_PHALANX:
			ent->s.sound = gi.soundindex("weapons/phaloop.wav");
			break;
		}
	}

	// [Paril-KEX] if no other sound is playing, play appropriate grapple sounds
	if (!ent->s.sound && ent->client->grappleEnt) {
		if (ent->client->grappleState == GRAPPLE_STATE_PULL)
			ent->s.sound = gi.soundindex("weapons/grapple/grpull.wav");
		else if (ent->client->grappleState == GRAPPLE_STATE_FLY)
			ent->s.sound = gi.soundindex("weapons/grapple/grfly.wav");
		else if (ent->client->grappleState == GRAPPLE_STATE_HANG)
			ent->s.sound = gi.soundindex("weapons/grapple/grhang.wav");
	}

	// weapon sounds play at a higher attn
	ent->s.loop_attenuation = ATTN_NORM;
}

/*
===============
ClientSetFrame
===============
*/
void ClientSetFrame(gentity_t *ent) {
	gclient_t *client;
	bool	   duck, run;

	if (ent->s.modelindex != MODELINDEX_PLAYER)
		return; // not in the player model

	client = ent->client;

	duck = client->ps.pmove.pm_flags & PMF_DUCKED ? true : false;
	run = xySpeed ? true : false;

	// check for stand/duck and stop/go transitions
	if (duck != client->anim.duck && client->anim.priority < ANIM_DEATH)
		goto newAnim;
	if (run != client->anim.run && client->anim.priority == ANIM_BASIC)
		goto newAnim;
	if (!ent->groundEntity && client->anim.priority <= ANIM_WAVE)
		goto newAnim;

	if (client->anim.time > level.time)
		return;
	else if ((client->anim.priority & ANIM_REVERSED) && (ent->s.frame > client->anim.end)) {
		if (client->anim.time <= level.time) {
			ent->s.frame--;
			client->anim.time = level.time + 10_hz;
		}
		return;
	} else if (!(client->anim.priority & ANIM_REVERSED) && (ent->s.frame < client->anim.end)) {
		// continue an animation
		if (client->anim.time <= level.time) {
			ent->s.frame++;
			client->anim.time = level.time + 10_hz;
		}
		return;
	}

	if (client->anim.priority == ANIM_DEATH)
		return; // stay there
	if (client->anim.priority == ANIM_JUMP) {
		if (!ent->groundEntity)
			return; // stay there
		ent->client->anim.priority = ANIM_WAVE;

		if (duck) {
			ent->s.frame = FRAME_jump6;
			ent->client->anim.end = FRAME_jump4;
			ent->client->anim.priority |= ANIM_REVERSED;
		} else {
			ent->s.frame = FRAME_jump3;
			ent->client->anim.end = FRAME_jump6;
		}
		ent->client->anim.time = level.time + 10_hz;
		return;
	}

newAnim:
	// return to either a running or standing frame
	client->anim.priority = ANIM_BASIC;
	client->anim.duck = duck;
	client->anim.run = run;
	client->anim.time = level.time + 10_hz;

	if (!ent->groundEntity) {
		// if on grapple, don't go into jump frame, go into standing
		// frame
		if (client->grappleEnt) {
			if (duck) {
				ent->s.frame = FRAME_crstnd01;
				client->anim.end = FRAME_crstnd19;
			} else {
				ent->s.frame = FRAME_stand01;
				client->anim.end = FRAME_stand40;
			}
		} else {
			client->anim.priority = ANIM_JUMP;

			if (duck) {
				if (ent->s.frame != FRAME_crwalk2)
					ent->s.frame = FRAME_crwalk1;
				client->anim.end = FRAME_crwalk2;
			} else {
				if (ent->s.frame != FRAME_jump2)
					ent->s.frame = FRAME_jump1;
				client->anim.end = FRAME_jump2;
			}
		}
	} else if (run) { // running
		if (duck) {
			ent->s.frame = FRAME_crwalk1;
			client->anim.end = FRAME_crwalk6;
		} else {
			ent->s.frame = FRAME_run1;
			client->anim.end = FRAME_run6;
		}
	} else { // standing
		if (duck) {
			ent->s.frame = FRAME_crstnd01;
			client->anim.end = FRAME_crstnd19;
		} else {
			ent->s.frame = FRAME_stand01;
			client->anim.end = FRAME_stand40;
		}
	}
}

/*
=================
P_RunMegaHealth
=================
*/
static void P_RunMegaHealth(gentity_t *ent) {
	if (!ent->client->pers.megaTime)
		return;
	else if (ent->health <= ent->max_health) {
		ent->client->pers.megaTime = 0_ms;
		return;
	}

	ent->client->pers.megaTime -= FRAME_TIME_S;

	if (ent->client->pers.megaTime <= 0_ms) {
		ent->health--;

		if (ent->health > ent->max_health)
			ent->client->pers.megaTime = 1000_ms;
		else
			ent->client->pers.megaTime = 0_ms;
	}
}

/*
=================
LagCompensate

[Paril-KEX] push all players' origins back to match their lag compensation
=================
*/
void LagCompensate(gentity_t *from_player, const vec3_t &start, const vec3_t &dir) {
	uint32_t current_frame = gi.ServerFrame();

	// if you need this to fight monsters, you need help
	if (!deathmatch->integer)
		return;
	else if (!g_lagCompensation->integer)
		return;
	// don't need this
	else if (from_player->client->cmd.server_frame >= current_frame ||
		(from_player->svFlags & SVF_BOT))
		return;

	int32_t frame_delta = (current_frame - from_player->client->cmd.server_frame) + 1;

	for (auto player : active_clients()) {
		// we aren't gonna hit ourselves
		if (player == from_player)
			continue;

		// not enough data, spare them
		if (player->client->num_lag_origins < frame_delta)
			continue;

		// if they're way outside of cone of vision, they won't be captured in this
		if ((player->s.origin - start).normalized().dot(dir) < 0.75f)
			continue;

		int32_t lag_id = (player->client->next_lag_origin - 1) - (frame_delta - 1);

		if (lag_id < 0)
			lag_id = game.max_lag_origins + lag_id;

		if (lag_id < 0 || lag_id >= player->client->num_lag_origins) {
			gi.Com_PrintFmt("{}: lag compensation error.\n", __FUNCTION__);
			UnLagCompensate();
			return;
		}

		const vec3_t &lag_origin = (game.lag_origins + ((player->s.number - 1) * game.max_lag_origins))[lag_id];

		// no way they'd be hit if they aren't in the PVS
		if (!gi.inPVS(lag_origin, start, false))
			continue;

		// only back up once
		if (!player->client->is_lag_compensated) {
			player->client->is_lag_compensated = true;
			player->client->lag_restore_origin = player->s.origin;
		}

		player->s.origin = lag_origin;

		gi.linkentity(player);
	}
}

/*
=================
UnLagCompensate

[Paril-KEX] pop everybody's lag compensation values
=================
*/
void UnLagCompensate() {
	for (auto player : active_clients()) {
		if (player->client->is_lag_compensated) {
			player->client->is_lag_compensated = false;
			player->s.origin = player->client->lag_restore_origin;
			gi.linkentity(player);
		}
	}
}

/*
=================
G_SaveLagCompensation

[Paril-KEX] save the current lag compensation value
=================
*/
static inline void G_SaveLagCompensation(gentity_t *ent) {
	(game.lag_origins + ((ent->s.number - 1) * game.max_lag_origins))[ent->client->next_lag_origin] = ent->s.origin;
	ent->client->next_lag_origin = (ent->client->next_lag_origin + 1) % game.max_lag_origins;

	if (ent->client->num_lag_origins < game.max_lag_origins)
		ent->client->num_lag_origins++;
}

/*
=================
Frenzy_ApplyAmmoRegen
=================
*/
static void Frenzy_ApplyAmmoRegen(gentity_t *ent) {
	if (!g_frenzy->integer || InfiniteAmmoOn(nullptr) || !ent || !ent->client)
		return;

	gclient_t *client = ent->client;

	if (!client->frenzyAmmoRegenTime) {
		client->frenzyAmmoRegenTime = level.time;
		return;
	}

	if (client->frenzyAmmoRegenTime > level.time)
		return;

	struct RegenEntry {
		const int weaponBit;  // If zero, always applies
		const int ammoIndex;
		const int amount;
		const int maxIndex;   // same index as ammo unless special
	};

	static constexpr RegenEntry regenTable[] = {
		{ IT_WEAPON_SHOTGUN | IT_WEAPON_SSHOTGUN, IT_AMMO_SHELLS,     4,  AMMO_SHELLS     },
		{ IT_WEAPON_MACHINEGUN | IT_WEAPON_CHAINGUN, IT_AMMO_BULLETS, 10, AMMO_BULLETS    },
		{ 0,                        IT_AMMO_GRENADES,   2,  AMMO_GRENADES  },
		{ IT_WEAPON_RLAUNCHER,      IT_AMMO_ROCKETS,    2,  AMMO_ROCKETS  },
		{ IT_WEAPON_HYPERBLASTER | IT_WEAPON_BFG | IT_WEAPON_IONRIPPER | IT_WEAPON_PLASMABEAM, IT_AMMO_CELLS, 8, AMMO_CELLS },
		{ IT_WEAPON_RAILGUN,        IT_AMMO_SLUGS,      1,  AMMO_SLUGS    },
		{ IT_WEAPON_PHALANX,        IT_AMMO_MAGSLUG,    2,  AMMO_MAGSLUG  },
		{ IT_WEAPON_ETF_RIFLE,      IT_AMMO_FLECHETTES,10,  AMMO_FLECHETTES },
		{ IT_WEAPON_PROXLAUNCHER,   IT_AMMO_PROX,       1,  AMMO_PROX     },
		{ IT_WEAPON_DISRUPTOR,      IT_AMMO_ROUNDS,     1,  AMMO_DISRUPTOR },
	};

	for (const auto &entry : regenTable) {
		if (entry.weaponBit == 0 || (client->pers.inventory[entry.weaponBit] != 0)) {
			int &ammo = client->pers.inventory[entry.ammoIndex];
			const int max = client->pers.ammoMax[entry.maxIndex];

			ammo += entry.amount;
			if (ammo > max)
				ammo = max;
		}
	}

	client->frenzyAmmoRegenTime = level.time + 2000_ms;
}

/*
=================
PlayQueuedAwardSound
=================
*/
static void PlayQueuedAwardSound(gentity_t *ent) {
	auto *cl = ent->client;
	auto &queue = cl->pers.awardQueue;

	if (queue.queueSize <= 0 || level.time < queue.nextPlayTime)
		return;

	int index = queue.playIndex;
	if (index >= queue.queueSize)
		return;

	// Play sound
	gi.local_sound(
		ent,
		static_cast<soundchan_t>(CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX),
		queue.soundIndex[index],
		1.0f,
		ATTN_NONE,
		0.0f,
		0
	);

	// Schedule next play
	queue.nextPlayTime = level.time + 1800_ms; // delay between awards

	// Shift queue
	queue.playIndex++;
	if (queue.playIndex >= queue.queueSize) {
		queue.queueSize = 0;
		queue.playIndex = 0;
	}
}


/*
=================
ClientEndServerFrame

Called for each player at the end of the server frame
and right after spawning
=================
*/
static int scorelimit = -1;
void ClientEndServerFrame(gentity_t *ent) {
	// no player exists yet (load game)
	if (!ent->client->pers.spawned && !level.mapSelectorVoteStartTime)
		return;

	float bobTime, bobtime_run;
	gentity_t *e = ent;	// g_eyecam->integer &&ent->client->followTarget ? ent->client->followTarget : ent;

	currentPlayer = e;
	currentClient = e->client;

	if (deathmatch->integer) {
		int limit = GT_ScoreLimit();
		if (!ent->client->ps.stats[STAT_SCORELIMIT] || limit != strtoul(gi.get_configstring(CONFIG_STORY_SCORELIMIT), nullptr, 10)) {
			ent->client->ps.stats[STAT_SCORELIMIT] = CONFIG_STORY_SCORELIMIT;
			gi.configstring(CONFIG_STORY_SCORELIMIT, limit ? G_Fmt("{}", limit).data() : "");
		}
	}

	// check fog changes
	P_ForceFogTransition(ent, false);

	// check goals
	G_PlayerNotifyGoal(ent);

	// mega health
	P_RunMegaHealth(ent);

	// vampiric damage expiration
	// don't expire if only 1 player in the match
	if (g_vampiric_damage->integer && ClientIsPlaying(ent->client) && !CombatIsDisabled() && (ent->health > g_vampiric_exp_min->integer)) {
		if (level.pop.num_playing_clients > 1 && level.time > ent->client->vampiricExpireTime) {
			int quantity = floor((ent->health - 1) / ent->max_health) + 1;
			ent->health -= quantity;
			ent->client->vampiricExpireTime = level.time + 1_sec;
			if (ent->health <= 0) {
				G_AdjustPlayerScore(ent->client, -1, GT(GT_TDM), -1);

				player_die(ent, ent, ent, 1, vec3_origin, { MOD_EXPIRE, true });
				if (!ent->client->eliminated)
					return;
			}
		}
	}

	//
	// If the origin or velocity have changed since ClientThink(),
	// update the pmove values.  This will happen when the client
	// is pushed by a bmodel or kicked by an explosion.
	//
	// If it wasn't updated here, the view position would lag a frame
	// behind the body position when pushed -- "sinking into plats"
	//
	currentClient->ps.pmove.origin = ent->s.origin;
	currentClient->ps.pmove.velocity = ent->velocity;

	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//
	if (!level.mapSelectorVoteStartTime && ((level.intermissionTime && !level.mapSelectorVoteStartTime) || ent->client->awaiting_respawn)) {
		if (ent->client->awaiting_respawn || (level.intermission.endOfUnit || level.isN64 || (deathmatch->integer && level.intermissionTime))) {
			currentClient->ps.screen_blend[3] = currentClient->ps.damageBlend[3] = 0;
			currentClient->ps.fov = 90;
			currentClient->ps.gunindex = 0;
		}
		SetStats(ent);
		SetCoopStats(ent);

		// if the scoreboard is up, update it if a client leaves
		if (deathmatch->integer && ent->client->showScores && ent->client->menuTime) {
			DeathmatchScoreboardMessage(e, e->enemy);
			gi.unicast(ent, false);
			ent->client->menuTime = 0_ms;
		}

/*freeze*/
		if (GT(GT_FREEZE) && !level.intermissionTime && ent->client->eliminated && !ent->client->resp.thawer) {	// || level.framenum & 8) {
			ent->s.effects |= EF_COLOR_SHELL;
			ent->s.renderfx |= (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);
		}
/*freeze*/

		return;
	}

	// auto doc tech
	Tech_ApplyAutoDoc(ent);

	// wor: weapons frenzy ammo regen
	Frenzy_ApplyAmmoRegen(ent);

	AngleVectors(ent->client->vAngle, forward, right, up);

	// burn from lava, etc
	P_WorldEffects();

	//
	// set model angles from view angles so other things in
	// the world can tell which direction you are looking
	//
	if (ent->client->vAngle[PITCH] > 180)
		ent->s.angles[PITCH] = (-360 + ent->client->vAngle[PITCH]) / 3;
	else
		ent->s.angles[PITCH] = ent->client->vAngle[PITCH] / 3;

	ent->s.angles[YAW] = ent->client->vAngle[YAW];
	ent->s.angles[ROLL] = 0;
	// [Paril-KEX] cl_rollhack
	ent->s.angles[ROLL] = -P_CalcRoll(ent->s.angles, ent->velocity) * 4;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	xySpeed = sqrt(ent->velocity[0] * ent->velocity[0] + ent->velocity[1] * ent->velocity[1]);

	if (xySpeed < 5) {
		bobMove = 0;
		currentClient->bobTime = 0; // start at beginning of cycle again
	} else if (ent->groundEntity) { // so bobbing only cycles when on ground
		if (xySpeed > 210)
			bobMove = gi.frame_time_ms / 400.f;
		else if (xySpeed > 100)
			bobMove = gi.frame_time_ms / 800.f;
		else
			bobMove = gi.frame_time_ms / 1600.f;
	}

	bobTime = (currentClient->bobTime += bobMove);
	bobtime_run = bobTime;

	if ((currentClient->ps.pmove.pm_flags & PMF_DUCKED) && ent->groundEntity)
		bobTime *= 4;

	bobCycle = (int)bobTime;
	bobCycleRun = (int)bobtime_run;
	bobFracSin = fabsf(sinf(bobTime * PIf));

	// apply all the damage taken this frame
	P_DamageFeedback(e);

	// determine the view offsets
	G_CalcViewOffset(e);

	// determine the gun offsets
	G_CalcGunOffset(e);

	// determine the full screen color blend
	// must be after viewoffset, so eye contents can be
	// accurately determined
	G_CalcBlend(e);

	// chase cam stuff
	if (!ClientIsPlaying(ent->client) || ent->client->eliminated) {
		SetSpectatorStats(ent);

		if (ent->client->followTarget) {
			ent->client->ps.screen_blend = ent->client->followTarget->client->ps.screen_blend;
			ent->client->ps.damageBlend = ent->client->followTarget->client->ps.damageBlend;

			ent->s.effects = ent->client->followTarget->s.effects;
			ent->s.renderfx = ent->client->followTarget->s.renderfx;
		}
	} else
		SetStats(ent);

	CheckFollowStats(ent);

	SetCoopStats(ent);

	ClientSetEvent(ent);

	ClientSetEffects(e);

	ClientSetSound(e);

	ClientSetFrame(ent);
	
	ent->client->oldVelocity = ent->velocity;
	ent->client->oldViewAngles = ent->client->ps.viewangles;
	ent->client->oldGroundEntity = ent->groundEntity;
	
	if (ent->client->menu && ent->client->inMenu) {
		// In-menu rendering
		if (ent->client->menuDirty || ent->client->menuTime <= level.time) {
			ent->client->menu->Render(ent);
			gi.unicast(ent, true);
			ent->client->menuDirty = false;
			ent->client->menuTime = level.time;
			UpdateMenu(ent);
		}
	} else if (ent->client->showScores && ent->client->menuTime <= level.time) {
		// Scoreboard-only rendering
		if (ent->client->menu) {
			CloseActiveMenu(ent);
		}
		DeathmatchScoreboardMessage(ent, ent->enemy);
		gi.unicast(ent, false);
		ent->client->menuTime = level.time + 3_sec;
	}

	if ((ent->svFlags & SVF_BOT) != 0) {
		Bot_EndFrame(ent);
	}

	P_AssignClientSkinnum(ent);

	if (deathmatch->integer)
		G_SaveLagCompensation(ent);

	Compass_Update(ent, false);

	// [Paril-KEX] in coop, if player collision is enabled and
	// we are currently in no-player-collision mode, check if
	// it's safe.
	if (CooperativeModeOn() && G_ShouldPlayersCollide(false) && !(ent->clipMask & CONTENTS_PLAYER) && ent->takeDamage) {
		bool clipped_player = false;

		for (auto player : active_clients()) {
			if (player == ent)
				continue;

			trace_t clip = gi.clip(player, ent->s.origin, ent->mins, ent->maxs, ent->s.origin, CONTENTS_MONSTER | CONTENTS_PLAYER);

			if (clip.startsolid || clip.allsolid) {
				clipped_player = true;
				break;
			}
		}

		// safe!
		if (!clipped_player)
			ent->clipMask |= CONTENTS_PLAYER;
	}
}