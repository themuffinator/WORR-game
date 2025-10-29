// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

OGRE (Quake 1) - WOR port

- Melee: chainsaw swipe (short reach, refires if still in range)
- Ranged: arcing grenade lob (mid/long range, clear-shot check)
- Two pain sets, two death sets
- Sight, search, idle sounds
- Uses monster muzzle flash handling like m_gunner.cpp

==============================================================================
*/

#include "../g_local.hpp"
#include "m_ogre.hpp"
#include "m_flash.hpp"

/*
===============
Spawnflags
===============
*/
constexpr SpawnFlags SPAWNFLAG_OGRE_NOGRENADE = 8_spawnflag;

/*
===============
Sounds
===============
*/
static cached_soundIndex snd_melee_swing;
static cached_soundIndex snd_melee_hit;
static cached_soundIndex snd_grenade;
static cached_soundIndex snd_pain1;
static cached_soundIndex snd_death;
static cached_soundIndex snd_idle;
static cached_soundIndex snd_idle2;
static cached_soundIndex snd_sight;
static cached_soundIndex snd_search;
static cached_soundIndex snd_drag;

static void ogre_attack(gentity_t* self);
static void ogre_grenade_fire(gentity_t* self);
static void ogre_flak_fire(gentity_t* self);
static void ogre_check_refire(gentity_t* self);

/*
===============
ogre_idlesound
===============
*/
static void ogre_idlesound(gentity_t* self) {
	gi.sound(self, CHAN_VOICE, frandom() > 0.6f ? snd_idle : snd_idle2, 1, ATTN_IDLE, 0);
}

/*
===============
ogre_sight
===============
*/
MONSTERINFO_SIGHT(ogre_sight) (gentity_t* self, gentity_t* other) -> void {
	gi.sound(self, CHAN_VOICE, snd_sight, 1, ATTN_NORM, 0);
}

/*
===============
ogre_search
===============
*/
MONSTERINFO_SEARCH(ogre_search) (gentity_t* self) -> void {
	gi.sound(self, CHAN_VOICE, snd_search, 1, ATTN_IDLE, 0);
}

/*
===============
ogre_stand
===============
*/
static MonsterFrame ogre_frames_stand[] = {
        { ai_stand },
        { ai_stand },
        { ai_stand },
        { ai_stand },
        { ai_stand },
        { ai_stand },
        { ai_stand },
        { ai_stand },
        { ai_stand }
};
MMOVE_T(ogre_move_stand) = { FRAME_stand1, FRAME_stand9, ogre_frames_stand, nullptr };

MONSTERINFO_STAND(ogre_stand) (gentity_t* self) -> void {
	M_SetAnimation(self, &ogre_move_stand);
}

/*
===============
ogre_idle
===============
*/
static void ogre_idle_loop(gentity_t* self) {
        if (frandom() < 0.66f)
                self->monsterInfo.nextFrame = FRAME_stand3;
}

static MonsterFrame ogre_frames_idle[] = {
        { ai_stand, 0, ogre_idlesound },
        { ai_stand },
        { ai_stand },
        { ai_stand },
        { ai_stand },
        { ai_stand, 0, ogre_idle_loop },
        { ai_stand },
        { ai_stand }
};
MMOVE_T(ogre_move_idle) = { FRAME_stand1, FRAME_stand8, ogre_frames_idle, ogre_stand };

MONSTERINFO_IDLE(ogre_idle) (gentity_t* self) -> void {
	M_SetAnimation(self, &ogre_move_idle);
}

/*
===============
ogre_walk
===============
*/
static void ogre_drag_sound(gentity_t* self) {
        gi.sound(self, CHAN_BODY, snd_drag, 1, ATTN_IDLE, 0);
}

static MonsterFrame ogre_frames_walk[] = {
        { ai_walk, 3 },
        { ai_walk, 2 },
        { ai_walk, 2 },
        { ai_walk, 2 },
        { ai_walk, 2 },
        { ai_walk, 6, ogre_drag_sound },
        { ai_walk, 3 },
        { ai_walk, 2 },
        { ai_walk, 3 },
        { ai_walk, 1 },
        { ai_walk, 2 },
        { ai_walk, 3 },
        { ai_walk, 3 },
        { ai_walk, 3 },
        { ai_walk, 3 },
        { ai_walk, 4 }
};
MMOVE_T(ogre_move_walk) = { FRAME_walk1, FRAME_walk16, ogre_frames_walk, nullptr };

MONSTERINFO_WALK(ogre_walk) (gentity_t* self) -> void {
	M_SetAnimation(self, &ogre_move_walk);
}

/*
===============
ogre_run
===============
*/
static MonsterFrame ogre_frames_run[] = {
        { ai_run, 9 },
        { ai_run, 12 },
        { ai_run, 8 },
        { ai_run, 22 },
        { ai_run, 16 },
        { ai_run, 4 },
        { ai_run, 13, ogre_attack },
        { ai_run, 24 }
};
MMOVE_T(ogre_move_run) = { FRAME_run1, FRAME_run8, ogre_frames_run, nullptr };

MONSTERINFO_RUN(ogre_run) (gentity_t* self) -> void {
	if (self->monsterInfo.aiFlags & AI_STAND_GROUND)
		M_SetAnimation(self, &ogre_move_stand);
	else
		M_SetAnimation(self, &ogre_move_run);
}

/*
===============
ogre_can_grenade
===============
*/
static bool ogre_can_grenade(gentity_t* self) {
        if (!self->enemy || !self->enemy->inUse)
                return false;
        if (self->spawnFlags.has(SPAWNFLAG_OGRE_NOGRENADE))
                return false;
        if (self->monsterInfo.aiFlags & AI_SOUND_TARGET)
                return false;
        if (self->bad_area)
                return false;

        if (!visible(self, self->enemy))
                return false;
        if (!infront(self, self->enemy))
                return false;

        Vector3 start;
        if (!M_CheckClearShot(self, { 0, 0, 32 }, start))
                return false;

        const float d = (self->enemy->s.origin - self->s.origin).length();
        if (d < 160.0f)
                return false;

        if (self->absMin[2] + 192 < self->enemy->absMin[2])
                return false;

        return true;
}

static void ogre_fire(gentity_t* self) {
        if (strcmp(self->className, "monster_ogre_marksman") == 0)
                ogre_flak_fire(self);
        else
                ogre_grenade_fire(self);
}

static void ogre_flak_fire(gentity_t* self) {
        if (!self->enemy || !self->enemy->inUse)
                return;

        const int frameOffset = std::clamp(self->s.frame - FRAME_shoot3, 0, 2);
        const MonsterMuzzleFlashID flashNumber = static_cast<MonsterMuzzleFlashID>(MZ2_GUNCMDR_GRENADE_FRONT_1 + frameOffset);

        Vector3 forward, right, up;
        AngleVectors(self->s.angles, forward, right, up);
        const Vector3 start = M_ProjectFlashSource(self, monster_flash_offset[flashNumber], forward, right);

        Vector3 aimDir{};
        PredictAim(self, self->enemy, start, 0, true, -0.2f, &aimDir, nullptr);
        aimDir.normalize();

        Vector3 aimForward, aimRight, aimUp;
        AngleVectors(VectorToAngles(aimDir), aimForward, aimRight, aimUp);

        constexpr float spread = 500.0f;
        constexpr int shotCount = 5;

        for (int i = 0; i < shotCount; ++i) {
                const float r = crandom() * spread;
                const float u = crandom() * spread;
                Vector3 dir = (aimForward * 8192.0f) + (aimRight * r) + (aimUp * u);
                dir.normalize();
                fire_flechette(self, start, dir, 4, 800, 5);
        }

        monster_muzzleflash(self, start, flashNumber);
}

static void ogre_grenade_fire(gentity_t* self) {
        if (!self->enemy || !self->enemy->inUse)
                return;

        MonsterMuzzleFlashID flashNumber = MZ2_GUNCMDR_GRENADE_FRONT_1;

        bool blindFire = (self->monsterInfo.aiFlags & AI_MANUAL_STEERING) != 0;
        Vector3 target;

        if (blindFire && !visible(self, self->enemy)) {
                if (!self->monsterInfo.blind_fire_target)
                        return;
                target = self->monsterInfo.blind_fire_target;
        }
        else {
                target = self->enemy->s.origin;
        }

        Vector3 forward, right, up;
        AngleVectors(self->s.angles, forward, right, up);
        Vector3 start = M_ProjectFlashSource(self, monster_flash_offset[flashNumber], forward, right);

        Vector3 aim = target - self->s.origin;
        const float dist = aim.length();

        if (dist > 512.0f && std::fabs(aim[2]) < 64.0f)
                aim[2] += (dist - 512.0f);

        aim.normalize();

        float pitch = std::clamp(aim[2], -0.5f, 0.4f);
        aim += up * pitch;

        const float speed = 600.0f;
        const bool isMortar = false;

        const bool isMulti = strcmp(self->className, "monster_ogre_multigrenade") == 0;

        Vector3 fireDir = aim;
        bool pitched = M_CalculatePitchToFire(self, target, start, fireDir, speed, 2.5f, isMortar);

        if (!pitched) {
                Vector3 lead{};
                PredictAim(self, self->enemy, start, 0, true, 0.0f, &lead, nullptr);
                lead[2] += 0.2f;
                fireDir = lead.normalized();
        }

        gi.sound(self, CHAN_WEAPON, snd_grenade, 1, ATTN_NORM, 0);

        const float rightAdjust = crandom_open() * 10.0f;
        float upAdjust = frandom() * 10.0f;

        if (!pitched && !isMulti)
                upAdjust = 200.0f + (crandom_open() * 10.0f);

        if (isMulti)
                monster_fire_multigrenade(self, start, fireDir, 40, speed, flashNumber, rightAdjust, upAdjust);
        else
                monster_fire_grenade(self, start, fireDir, 40, speed, flashNumber, rightAdjust, upAdjust);
}

static void ogre_swing_left(gentity_t* self) {
        Vector3 aim = { MELEE_DISTANCE, self->maxs[0], 8.0f };
        if (fire_hit(self, aim, irandom(12, 20), 100))
                gi.sound(self, CHAN_WEAPON, snd_melee_hit, 1, ATTN_NORM, 0);
        else
                self->monsterInfo.melee_debounce_time = level.time + 1_sec;
}

static void ogre_swing_right(gentity_t* self) {
        Vector3 aim = { MELEE_DISTANCE, self->maxs[0], 8.0f };
        if (fire_hit(self, aim, irandom(12, 20), 100))
                gi.sound(self, CHAN_WEAPON, snd_melee_hit, 1, ATTN_NORM, 0);
        else
                self->monsterInfo.melee_debounce_time = level.time + 1_sec;
}

static void ogre_smash(gentity_t* self) {
        Vector3 aim = { MELEE_DISTANCE, self->maxs[0], 8.0f };
        if (!fire_hit(self, aim, 25 + static_cast<int>(frandom() * 5.0f), 100))
                self->monsterInfo.melee_debounce_time = level.time + 1.2_sec;

        gi.sound(self, CHAN_WEAPON, snd_melee_swing, 1, ATTN_NORM, 0);
}

static void ogre_sawswingsound(gentity_t* self) {
        gi.sound(self, CHAN_WEAPON, snd_melee_swing, 1, ATTN_NORM, 0);
}

static void ogre_check_refire(gentity_t* self) {
        if (!self->enemy || !self->enemy->inUse || self->enemy->health <= 0) {
                M_SetAnimation(self, &ogre_move_run);
                return;
        }

        if (skill->integer == 3 || range_to(self, self->enemy) == RANGE_MELEE) {
                if (frandom() > 0.5f)
                        M_SetAnimation(self, &ogre_move_swing_attack);
                else
                        M_SetAnimation(self, &ogre_move_smash_attack);
        }
        else {
                ogre_attack(self);
        }
}

static MonsterFrame ogre_frames_swing[] = {
        { ai_charge },
        { ai_charge, 0, ogre_sawswingsound },
        { ai_charge },
        { ai_charge },
        { ai_charge },
        { ai_charge, 0, ogre_swing_right },
        { ai_charge },
        { ai_charge, 0, ogre_sawswingsound },
        { ai_charge },
        { ai_charge },
        { ai_charge },
        { ai_charge, 0, ogre_swing_left },
        { ai_charge },
        { ai_charge, 0, ogre_check_refire }
};
MMOVE_T(ogre_move_swing_attack) = { FRAME_swing1, FRAME_swing14, ogre_frames_swing, ogre_run };

static MonsterFrame ogre_frames_smash[] = {
        { ai_charge },
        { ai_charge },
        { ai_charge },
        { ai_charge },
        { ai_charge },
        { ai_charge },
        { ai_charge },
        { ai_charge },
        { ai_charge, 0, ogre_smash },
        { ai_charge },
        { ai_charge },
        { ai_charge },
        { ai_charge },
        { ai_charge, 0, ogre_check_refire }
};
MMOVE_T(ogre_move_smash_attack) = { FRAME_smash1, FRAME_smash14, ogre_frames_smash, ogre_run };

static MonsterFrame ogre_frames_attack_grenade[] = {
        { ai_charge },
        { ai_charge },
        { ai_charge },
        { ai_charge, 0, ogre_fire },
        { ai_charge },
        { ai_charge }
};
MMOVE_T(ogre_move_attack_grenade) = { FRAME_shoot1, FRAME_shoot6, ogre_frames_attack_grenade, ogre_run };

MONSTERINFO_MELEE(ogre_melee) (gentity_t* self) -> void {
        if (frandom() > 0.5f)
                M_SetAnimation(self, &ogre_move_swing_attack);
        else
                M_SetAnimation(self, &ogre_move_smash_attack);
}

MONSTERINFO_ATTACK(ogre_attack) (gentity_t* self) -> void {
        if (!self->enemy || !self->enemy->inUse)
                return;

        const float distance = range_to(self, self->enemy);

        if (!self->bad_area && distance <= RANGE_MELEE && self->monsterInfo.melee_debounce_time <= level.time) {
                if (frandom() > 0.5f)
                        M_SetAnimation(self, &ogre_move_swing_attack);
                else
                        M_SetAnimation(self, &ogre_move_smash_attack);
                return;
        }

        if (ogre_can_grenade(self)) {
                M_SetAnimation(self, &ogre_move_attack_grenade);
                return;
        }

        M_SetAnimation(self, &ogre_move_run);
}

MONSTERINFO_CHECKATTACK(ogre_checkattack) (gentity_t* self) -> bool {
        if (!self->enemy || self->enemy->health <= 0)
                return false;

        if (range_to(self, self->enemy) <= RANGE_MELEE && self->monsterInfo.melee_debounce_time <= level.time) {
                self->monsterInfo.attackState = MonsterAttackState::Melee;
                return true;
        }

        if (ogre_can_grenade(self)) {
                self->monsterInfo.attackState = MonsterAttackState::Missile;
                return true;
        }

        return false;
}

/*
===============
ogre_pain
===============
*/
static MonsterFrame ogre_frames_pain1[] = {
        { ai_move, -3 },
        { ai_move, 1 },
        { ai_move, 1 },
        { ai_move },
        { ai_move, 1 }
};
MMOVE_T(ogre_move_pain1) = { FRAME_pain1, FRAME_pain5, ogre_frames_pain1, ogre_run };

static MonsterFrame ogre_frames_pain2[] = {
        { ai_move, -1 },
        { ai_move },
        { ai_move, 1 }
};
MMOVE_T(ogre_move_pain2) = { FRAME_painb1, FRAME_painb3, ogre_frames_pain2, ogre_run };

static MonsterFrame ogre_frames_pain3[] = {
        { ai_move, -3 },
        { ai_move, 1 },
        { ai_move, 1 },
        { ai_move },
        { ai_move },
        { ai_move, 1 }
};
MMOVE_T(ogre_move_pain3) = { FRAME_painc1, FRAME_painc6, ogre_frames_pain3, ogre_run };

static MonsterFrame ogre_frames_pain4[] = {
        { ai_move, -3 },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move, 1 },
        { ai_move, 1 },
        { ai_move }
};
MMOVE_T(ogre_move_pain4) = { FRAME_paind1, FRAME_paind16, ogre_frames_pain4, ogre_run };

static MonsterFrame ogre_frames_pain5[] = {
        { ai_move, -3 },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move, 1 },
        { ai_move, 1 },
        { ai_move }
};
MMOVE_T(ogre_move_pain5) = { FRAME_paine1, FRAME_paine15, ogre_frames_pain5, ogre_run };

static PAIN(ogre_pain) (gentity_t* self, gentity_t* other, float kick, int damage, const MeansOfDeath& mod) -> void {
        if (level.time < self->pain_debounce_time)
                return;

        if (!M_ShouldReactToPain(self, mod))
                return;

        gi.sound(self, CHAN_VOICE, snd_pain1, 1, ATTN_NORM, 0);

        const float r = frandom();

        if (r < 0.20f) {
                self->pain_debounce_time = level.time + 1_sec;
                M_SetAnimation(self, &ogre_move_pain1);
        }
        else if (r < 0.40f) {
                self->pain_debounce_time = level.time + 1_sec;
                M_SetAnimation(self, &ogre_move_pain2);
        }
        else if (r < 0.60f) {
                self->pain_debounce_time = level.time + 1_sec;
                M_SetAnimation(self, &ogre_move_pain3);
        }
        else if (r < 0.80f) {
                self->pain_debounce_time = level.time + 2_sec;
                M_SetAnimation(self, &ogre_move_pain4);
        }
        else {
                self->pain_debounce_time = level.time + 2_sec;
                M_SetAnimation(self, &ogre_move_pain5);
        }
}

/*
===============
ogre_death helpers
===============
*/
static void ogre_shrink(gentity_t* self) {
        self->maxs[2] = 0;
        self->svFlags |= SVF_DEADMONSTER;
        gi.linkEntity(self);
}

static void ogre_droprockets(gentity_t* self) {
        if (self->health <= self->gibHealth)
                return;
}

static void ogre_dead(gentity_t* self) {
        self->mins = { -16.0f, -16.0f, -24.0f };
        self->maxs = { 16.0f, 16.0f, -8.0f };
        monster_dead(self);
}

/*
===============
ogre_die
===============
*/
static MonsterFrame ogre_frames_death1[] = {
        { ai_move },
        { ai_move },
        { ai_move, 0, ogre_droprockets },
        { ai_move, -7 },
        { ai_move, -3 },
        { ai_move, -5 },
        { ai_move, 8 },
        { ai_move, 6 },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move },
        { ai_move, 0, ogre_shrink }
};
MMOVE_T(ogre_move_death1) = { FRAME_death1, FRAME_death14, ogre_frames_death1, ogre_dead };

static MonsterFrame ogre_frames_death2[] = {
        { ai_move },
        { ai_move },
        { ai_move, 0, ogre_droprockets },
        { ai_move, -7 },
        { ai_move, -3 },
        { ai_move, -5 },
        { ai_move, 8 },
        { ai_move, 6 },
        { ai_move },
        { ai_move, 0, ogre_shrink }
};
MMOVE_T(ogre_move_death2) = { FRAME_bdeath1, FRAME_bdeath10, ogre_frames_death2, ogre_dead };

static DIE(ogre_die) (gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, const Vector3& point, const MeansOfDeath& mod) -> void {
        if (M_CheckGib(self, mod)) {
                gi.sound(self, CHAN_VOICE, gi.soundIndex("misc/udeath.wav"), 1, ATTN_NORM, 0);

                self->s.skinNum &= ~1;
                ThrowGibs(self, damage, {
                        { 2, "models/objects/gibs/bone/tris.md2" },
                        { 4, "models/objects/gibs/sm_meat/tris.md2" },
                        { "models/objects/gibs/head2/tris.md2", GIB_HEAD }
                        });

		self->deadFlag = true;
		return;
	}

	if (self->deadFlag)
		return;

	gi.sound(self, CHAN_VOICE, snd_death, 1, ATTN_NORM, 0);
	self->deadFlag = true;
	self->takeDamage = true;

	if (brandom())
		M_SetAnimation(self, &ogre_move_death1);
	else
		M_SetAnimation(self, &ogre_move_death2);
}

/*
===============
ogre_setskin
===============
*/
MONSTERINFO_SETSKIN(ogre_setskin) (gentity_t* self) -> void {
        int baseSkin = 0;

        if (strcmp(self->className, "monster_ogre_marksman") == 0)
                baseSkin = 2;
        else if (strcmp(self->className, "monster_ogre_multigrenade") == 0)
                baseSkin = 4;

        if (self->health < (self->maxHealth / 2))
                self->s.skinNum = baseSkin + 1;
        else
                self->s.skinNum = baseSkin;
}

/*
===============
SP_monster_ogre
===============
*/
/*QUAKED monster_ogre (1 0 0) (-24 -24 -24) (24 24 32) AMBUSH TRIGGER_SPAWN SIGHT NOGRENADE x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
model="models/monsters/ogre/tris.md2"
*/
void SP_monster_ogre(gentity_t* self) {
        if (!M_AllowSpawn(self)) {
                FreeEntity(self);
                return;
        }

        const spawn_temp_t& st = ED_GetSpawnTemp();

	// sounds
	snd_melee_swing.assign("ogre/ogsawatk.wav");
	snd_melee_hit.assign("ogre/oghit.wav");
	snd_grenade.assign("ogre/ogthrow.wav");
	snd_pain1.assign("ogre/ogpain1.wav");
	snd_death.assign("ogre/ogdth.wav");
	snd_idle.assign("ogre/ogidle.wav");
	snd_idle2.assign("ogre/ogidle2.wav");
        snd_sight.assign("ogre/ogsight.wav");
        snd_search.assign("ogre/ogsearch.wav");
        snd_drag.assign("ogre/ogdrag.wav");

	// model
	self->moveType = MoveType::Step;
	self->solid = SOLID_BBOX;
	self->s.modelIndex = gi.modelIndex("models/monsters/ogre/tris.md2");

	// bounds and stats
        self->mins = { -20, -20, -24 };
        self->maxs = { 20, 20, 32 };

        self->health = 300 * st.health_multiplier;
        self->maxHealth = self->health;
        self->gibHealth = -80;
        self->mass = 250;

	// callbacks
	self->pain = ogre_pain;
	self->die = ogre_die;

	self->monsterInfo.stand = ogre_stand;
	self->monsterInfo.walk = ogre_walk;
	self->monsterInfo.run = ogre_run;
	self->monsterInfo.dodge = nullptr;
        self->monsterInfo.attack = ogre_attack;     // grenade
        self->monsterInfo.melee = ogre_melee;      // chainsaw
        self->monsterInfo.sight = ogre_sight;
        self->monsterInfo.search = ogre_search;
        self->monsterInfo.idle = ogre_idle;
        self->monsterInfo.checkAttack = ogre_checkattack;
        self->monsterInfo.blocked = nullptr;
        self->monsterInfo.setSkin = ogre_setskin;

        self->monsterInfo.aiFlags |= AI_STINKY;

        gi.linkEntity(self);

        M_SetAnimation(self, &ogre_move_stand);

        self->monsterInfo.combatStyle = CombatStyle::Mixed;

        self->monsterInfo.scale = OGRE_MODEL_SCALE;
        self->monsterInfo.dropHeight = 256;
        self->monsterInfo.jumpHeight = 68;

        if (self->monsterInfo.setSkin)
                self->monsterInfo.setSkin(self);

        walkmonster_start(self);
}

/*
===============
SP_monster_ogre_marksman
===============
*/
/*QUAKED monster_ogre_marksman (1 0 0) (-24 -24 -24) (24 24 32) AMBUSH TRIGGER_SPAWN SIGHT NOGRENADE x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
model="models/monsters/ogre/tris.md2"
*/
void SP_monster_ogre_marksman(gentity_t* self) {
        SP_monster_ogre(self);
        self->s.skinNum = 2;
        if (self->monsterInfo.setSkin)
                self->monsterInfo.setSkin(self);
}

/*
===============
SP_monster_ogre_multigrenade
===============
*/
/*QUAKED monster_ogre_multigrenade (1 0 0) (-24 -24 -24) (24 24 32) AMBUSH TRIGGER_SPAWN SIGHT NOGRENADE x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
model="models/monsters/ogre/tris.md2"
*/
void SP_monster_ogre_multigrenade(gentity_t* self) {
        SP_monster_ogre(self);
        self->s.skinNum = 4;
        if (self->monsterInfo.setSkin)
                self->monsterInfo.setSkin(self);
}
