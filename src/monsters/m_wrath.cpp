// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

Wrath (Quake 1 Mission Pack) - WORR Port

Retains the Ionized behaviour: blind-fire capable lightning pods,
hovering movement, and explosive death gibs.

==============================================================================
*/

#include "../g_local.hpp"
#include "m_wrath.hpp"
#include "q1_support.hpp"

namespace {

constexpr float WRATH_MODEL_SCALE = 1.0f;
constexpr int WRATH_HEALTH = 400;
constexpr int WRATH_PROJECTILE_DAMAGE = 20;
constexpr int WRATH_PROJECTILE_SPEED = 400;
const Vector3 WRATH_MINS = { -16.0f, -16.0f, -24.0f };
const Vector3 WRATH_MAXS = { 16.0f, 16.0f, 32.0f };
const Vector3 WRATH_MUZZLE_OFFSET = { 0.0f, 0.0f, 10.0f };

cached_soundIndex s_sight;
cached_soundIndex s_attack;
cached_soundIndex s_die;
cached_soundIndex s_pain;
cached_soundIndex s_attack_loop;

MONSTERINFO_SIGHT(wrath_sight) (gentity_t* self, gentity_t* /*other*/) -> void {
        gi.sound(self, CHAN_VOICE, s_sight, 1, ATTN_NORM, 0);
}

static void wrath_attack_sound(gentity_t* self) {
        gi.sound(self, CHAN_VOICE, s_attack, 1, ATTN_NORM, 0);
}

static MonsterFrame wrath_frames_stand[] = {
        { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand },
        { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand },
        { ai_stand }, { ai_stand }, { ai_stand }, { ai_stand }
};
MMOVE_T(wrath_move_stand) = { FRAME_wrthwk01, FRAME_wrthwk12, wrath_frames_stand, nullptr };

MONSTERINFO_STAND(wrath_stand) (gentity_t* self) -> void {
        M_SetAnimation(self, &wrath_move_stand);
}

static MonsterFrame wrath_frames_walk[] = {
        { ai_walk, 5 }, { ai_walk, 5 }, { ai_walk, 5 }, { ai_walk, 5 },
        { ai_walk, 5 }, { ai_walk, 5 }, { ai_walk, 5 }, { ai_walk, 5 },
        { ai_walk, 5 }, { ai_walk, 5 }, { ai_walk, 5 }, { ai_walk, 5 }
};
MMOVE_T(wrath_move_walk) = { FRAME_wrthwk01, FRAME_wrthwk12, wrath_frames_walk, nullptr };

MONSTERINFO_WALK(wrath_walk) (gentity_t* self) -> void {
        M_SetAnimation(self, &wrath_move_walk);
}

static MonsterFrame wrath_frames_run[] = {
        { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 },
        { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 },
        { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 }, { ai_run, 10 }
};
MMOVE_T(wrath_move_run) = { FRAME_wrthwk01, FRAME_wrthwk12, wrath_frames_run, nullptr };

MONSTERINFO_RUN(wrath_run) (gentity_t* self) -> void {
        M_SetAnimation(self, &wrath_move_run);
}

static MonsterFrame wrath_frames_pain1[] = {
        { ai_move }, { ai_move }, { ai_move }, { ai_move }, { ai_move }, { ai_move }
};
MMOVE_T(wrath_move_pain1) = { FRAME_wrthpa01, FRAME_wrthpa06, wrath_frames_pain1, wrath_run };

static MonsterFrame wrath_frames_pain2[] = {
        { ai_move }, { ai_move }, { ai_move }, { ai_move }, { ai_move },
        { ai_move }, { ai_move }, { ai_move }, { ai_move }, { ai_move }, { ai_move }
};
MMOVE_T(wrath_move_pain2) = { FRAME_wrthpb01, FRAME_wrthpb11, wrath_frames_pain2, wrath_run };

static PAIN(wrath_pain) (gentity_t* self, gentity_t* /*other*/, float /*kick*/, int damage, const MeansOfDeath& mod) -> void {
        if (level.time < self->pain_debounce_time)
                return;

        self->pain_debounce_time = level.time + 2_sec;

        if (!M_ShouldReactToPain(self, mod))
                return;

        gi.sound(self, CHAN_VOICE, s_pain, 1, ATTN_NORM, 0);

        if (damage > 40 && frandom() < 0.6f)
                M_SetAnimation(self, &wrath_move_pain2);
        else if (frandom() >= 0.4f)
                M_SetAnimation(self, &wrath_move_pain1);
        else
                M_SetAnimation(self, &wrath_move_pain2);
}

static void wrath_dead(gentity_t* self) {
        RadiusDamage(self, self, 60.0f, nullptr, 105.0f, DamageFlags::Normal, ModID::Barrel);

        gi.WriteByte(svc_temp_entity);
        gi.WriteByte(TE_EXPLOSION1);
        gi.WritePosition(self->s.origin);
        gi.multicast(self->s.origin, MULTICAST_PHS, false);

        self->s.skinNum /= 2;

        ThrowGibs(self, 55, {
                { 2, "models/objects/gibs/bone/tris.md2" },
                { 4, "models/monsters/wrath/gibs/claw.md2" },
                { 4, "models/monsters/wrath/gibs/arm.md2" },
                { "models/monsters/overlord/gibs/ribs.md2" },
                { "models/monsters/wrath/gibs/bone.md2", GIB_HEAD }
        });

        self->touch = nullptr;
}

static MonsterFrame wrath_frames_die[] = {
        { ai_move }, { ai_move }, { ai_move }, { ai_move }, { ai_move },
        { ai_move }, { ai_move }, { ai_move }, { ai_move }, { ai_move },
        { ai_move }, { ai_move }, { ai_move }, { ai_move }, { ai_move }
};
MMOVE_T(wrath_move_die) = { FRAME_wrthdt01, FRAME_wrthdt15, wrath_frames_die, wrath_dead };

static DIE(wrath_die) (gentity_t* self, gentity_t* /*inflictor*/, gentity_t* /*attacker*/, int /*damage*/,
        const Vector3& /*point*/, const MeansOfDeath& /*mod*/) -> void {
        if (self->deadFlag)
                return;

        gi.sound(self, CHAN_VOICE, s_die, 1, ATTN_NORM, 0);
        self->deadFlag = true;
        self->takeDamage = true;

        M_SetAnimation(self, &wrath_move_die);
}

static void wrath_fire(gentity_t* self) {
        if (!self->enemy || !self->enemy->inUse)
                return;

        const bool blindfire = (self->monsterInfo.aiFlags & AI_MANUAL_STEERING) != 0;

        Vector3 forward, right;
        AngleVectors(self->s.angles, forward, right, nullptr);

        const Vector3 start = M_ProjectFlashSource(self, WRATH_MUZZLE_OFFSET, forward, right);
        const int rocketSpeed = WRATH_PROJECTILE_SPEED;

        Vector3 target = blindfire ? self->monsterInfo.blind_fire_target : self->enemy->s.origin;
        Vector3 aimPoint;
        Vector3 dir;

        if (blindfire) {
                aimPoint = target;
                dir = aimPoint - start;
        } else if (frandom() < 0.33f || start[2] < self->enemy->absMin[2]) {
                aimPoint = target;
                aimPoint[2] += self->enemy->viewHeight;
                dir = aimPoint - start;
        } else {
                aimPoint = target;
                aimPoint[2] = self->enemy->absMin[2] + 1.0f;
                dir = aimPoint - start;
        }

        if (!blindfire && frandom() < 0.35f)
                PredictAim(self, self->enemy, start, static_cast<float>(rocketSpeed), false, 0.0f, &dir, &aimPoint);

        dir.normalize();

        trace_t trace = gi.traceLine(start, aimPoint, self, MASK_PROJECTILE);
        auto try_fire = [&](const Vector3& shotDir, float turnFraction) {
                fire_vorepod(self, start, shotDir, WRATH_PROJECTILE_DAMAGE, rocketSpeed,
                        static_cast<float>(WRATH_PROJECTILE_DAMAGE), WRATH_PROJECTILE_DAMAGE, turnFraction, 1);
        };

        if (blindfire) {
                if (!(trace.startsolid || trace.allsolid || trace.fraction < 0.5f)) {
                        try_fire(dir, 0.075f);
                } else {
                        Vector3 altTarget = target + (right * -10.0f);
                        Vector3 altDir = altTarget - start;
                        altDir.normalize();
                        trace = gi.traceLine(start, altTarget, self, MASK_PROJECTILE);
                        if (!(trace.startsolid || trace.allsolid || trace.fraction < 0.5f)) {
                                try_fire(altDir, 0.075f);
                        } else {
                                altTarget = target + (right * 10.0f);
                                altDir = altTarget - start;
                                altDir.normalize();
                                trace = gi.traceLine(start, altTarget, self, MASK_PROJECTILE);
                                if (!(trace.startsolid || trace.allsolid || trace.fraction < 0.5f))
                                        try_fire(altDir, 0.075f);
                        }
                }
        } else {
                if (trace.fraction > 0.5f || !trace.ent || trace.ent->solid != SOLID_BSP)
                        try_fire(dir, 0.15f);
        }

        gi.sound(self, CHAN_WEAPON | CHAN_RELIABLE, s_attack_loop, 1, ATTN_NORM, 0);
}

static MonsterFrame wrath_frames_attack1[] = {
        { ai_charge }, { ai_charge }, { ai_charge, 0, wrath_attack_sound },
        { ai_charge }, { ai_charge }, { ai_charge }, { ai_charge }, { ai_charge },
        { ai_charge, -1 }, { ai_charge, -2 }, { ai_charge, -3 },
        { ai_charge, -2, wrath_fire }, { ai_charge, -1 }, { ai_charge }
};
MMOVE_T(wrath_move_attack1) = { FRAME_wrthaa01, FRAME_wrthaa14, wrath_frames_attack1, wrath_run };

static MonsterFrame wrath_frames_attack2[] = {
        { ai_charge }, { ai_charge }, { ai_charge, 0, wrath_attack_sound },
        { ai_charge }, { ai_charge }, { ai_charge, -1 }, { ai_charge, -2 }, { ai_charge, -3 },
        { ai_charge, -2, wrath_fire }, { ai_charge, -1 }, { ai_charge }, { ai_charge }, { ai_charge }
};
MMOVE_T(wrath_move_attack2) = { FRAME_wrthab01, FRAME_wrthab13, wrath_frames_attack2, wrath_run };

static MonsterFrame wrath_frames_attack3[] = {
        { ai_charge }, { ai_charge }, { ai_charge, 0, wrath_attack_sound },
        { ai_charge }, { ai_charge }, { ai_charge }, { ai_charge, -1 }, { ai_charge, -2 },
        { ai_charge, -3 }, { ai_charge, -2, wrath_fire }, { ai_charge, -1 },
        { ai_charge }, { ai_charge }, { ai_charge }, { ai_charge }
};
MMOVE_T(wrath_move_attack3) = { FRAME_wrthac01, FRAME_wrthac15, wrath_frames_attack3, wrath_run };

static MONSTERINFO_ATTACK(wrath_attack) (gentity_t* self) -> void {
        const Vector3 offset = WRATH_MUZZLE_OFFSET;
        if (!M_CheckClearShot(self, offset))
                return;

        monster_done_dodge(self);

        if (self->monsterInfo.attackState == MonsterAttackState::Blind) {
                float chance;
                if (self->monsterInfo.blind_fire_delay < 1_sec)
                        chance = 1.0f;
                else if (self->monsterInfo.blind_fire_delay < 7.5_sec)
                        chance = 0.4f;
                else
                        chance = 0.1f;

                const float roll = frandom();
                self->monsterInfo.blind_fire_delay += random_time(5.5_sec, 6.5_sec);

                if (!self->monsterInfo.blind_fire_target)
                        return;

                if (roll > chance)
                        return;

                self->monsterInfo.aiFlags |= AI_MANUAL_STEERING;

                if (frandom() > 0.66f)
                        M_SetAnimation(self, &wrath_move_attack3);
                else if (frandom() > 0.33f)
                        M_SetAnimation(self, &wrath_move_attack2);
                else
                        M_SetAnimation(self, &wrath_move_attack1);

                self->monsterInfo.attackFinished = level.time + random_time(2_sec);
                return;
        }

        const float roll = frandom();
        if (roll > 0.66f)
                M_SetAnimation(self, &wrath_move_attack3);
        else if (roll > 0.33f)
                M_SetAnimation(self, &wrath_move_attack2);
        else
                M_SetAnimation(self, &wrath_move_attack1);
}

static void wrath_set_fly_parameters(gentity_t* self) {
        self->monsterInfo.fly_thrusters = false;
        self->monsterInfo.fly_acceleration = 20.0f;
        self->monsterInfo.fly_speed = 120.0f;
        self->monsterInfo.fly_min_distance = 200.0f;
        self->monsterInfo.fly_max_distance = 400.0f;
}

} // namespace

/*QUAKED monster_wrath (1 .5 0) (-16 -16 -24) (16 16 32) AMBUSH TRIGGER_SPAWN SIGHT
model="models/monsters/wrath/tris.md2"
*/
void SP_monster_wrath(gentity_t* self) {
        const spawn_temp_t& st = ED_GetSpawnTemp();

        if (!M_AllowSpawn(self)) {
                FreeEntity(self);
                return;
        }

        s_sight.assign("wrath/wsee.wav");
        s_attack.assign("wrath/watt.wav");
        s_die.assign("wrath/wdthc.wav");
        s_pain.assign("wrath/wpain.wav");
        s_attack_loop.assign("vore/attack2.wav");

        self->moveType = MoveType::Step;
        self->solid = SOLID_BBOX;
        self->s.modelIndex = gi.modelIndex("models/monsters/wrath/tris.md2");
        self->mins = WRATH_MINS;
        self->maxs = WRATH_MAXS;

        gi.modelIndex("models/monsters/wrath/gibs/claw.md2");
        gi.modelIndex("models/monsters/wrath/gibs/arm.md2");
        gi.modelIndex("models/monsters/wrath/gibs/bone.md2");
        gi.modelIndex("models/monsters/overlord/gibs/ribs.md2");

        self->health = self->maxHealth = WRATH_HEALTH * st.health_multiplier;
        self->mass = 400;

        self->pain = wrath_pain;
        self->die = wrath_die;

        self->monsterInfo.stand = wrath_stand;
        self->monsterInfo.walk = wrath_walk;
        self->monsterInfo.run = wrath_run;
        self->monsterInfo.attack = wrath_attack;
        self->monsterInfo.melee = nullptr;
        self->monsterInfo.sight = wrath_sight;
        self->monsterInfo.search = nullptr;

        self->flags |= FL_FLY;
        if (self->yawSpeed == 0)
                self->yawSpeed = 10;
        self->viewHeight = 10;

        gi.linkEntity(self);

        M_SetAnimation(self, &wrath_move_stand);
        self->monsterInfo.scale = WRATH_MODEL_SCALE;

        self->monsterInfo.aiFlags |= AI_ALTERNATE_FLY;
        wrath_set_fly_parameters(self);
        self->monsterInfo.fly_pinned = false;
        self->monsterInfo.fly_position_time = 0_ms;

        flymonster_start(self);
}

