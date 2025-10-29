#include "../g_local.hpp"
#include "m_knight.hpp"

constexpr SpawnFlags SPAWNFLAG_KNIGHT_NOJUMPING = 8_spawnflag;
constexpr SpawnFlags SPAWNFLAG_KNIGHT_KNEEL = 16_spawnflag;
constexpr SpawnFlags SPAWNFLAG_STATUE_STATIONARY = 32_spawnflag;

static cached_soundIndex s_idle;
static cached_soundIndex s_sight;
static cached_soundIndex s_pain;
static cached_soundIndex s_death;
static cached_soundIndex s_sword_hit;
static cached_soundIndex s_sword_miss;
static cached_soundIndex s_sword_swing;

void knight_stand(gentity_t* self);
void knight_walk(gentity_t* self);
void knight_run(gentity_t* self);

static void knight_idle(gentity_t* self) {
    if (frandom() < 0.2f)
        gi.sound(self, CHAN_VOICE, s_idle, 1, ATTN_IDLE, 0);
}

static void knight_search(gentity_t* self) {
    gi.sound(self, CHAN_VOICE, s_idle, 1, ATTN_IDLE, 0);
}

static void knight_step(gentity_t* self) {
    monster_footstep(self);
}

MONSTERINFO_SIGHT(knight_sight) (gentity_t* self, gentity_t* other) -> void {
    gi.sound(self, CHAN_VOICE, s_sight, 1, ATTN_NORM, 0);
}

static void knight_sword_sound(gentity_t* self) {
    if (frandom() <= 0.5f && s_sword_swing)
        gi.sound(self, CHAN_WEAPON, s_sword_swing, 1, ATTN_NORM, 0);
    else
        gi.sound(self, CHAN_WEAPON, s_sword_hit, 1, ATTN_NORM, 0);
}

static void knight_melee_hit(gentity_t* self) {
    Vector3 aim = { MELEE_DISTANCE, self->mins[0], 4.0f };

    if (fire_hit(self, aim, irandom(1, 9), 100)) {
        gi.sound(self, CHAN_WEAPON, s_sword_hit, 1, ATTN_NORM, 0);
    } else {
        gi.sound(self, CHAN_WEAPON, s_sword_miss, 1, ATTN_NORM, 0);
        self->monsterInfo.melee_debounce_time = level.time + 1.5_sec;
    }
}

static void knight_run_strike(gentity_t* self) {
    Vector3 aim = { MELEE_DISTANCE, self->mins[0], 2.0f };

    if (fire_hit(self, aim, irandom(1, 9), 100)) {
        gi.sound(self, CHAN_WEAPON, s_sword_hit, 1, ATTN_NORM, 0);
    } else {
        gi.sound(self, CHAN_WEAPON, s_sword_hit, 1, ATTN_NORM, 0);
        self->monsterInfo.melee_debounce_time = level.time + 1.5_sec;
    }
}

static void knight_check_dist(gentity_t* self) {
    if (!self->enemy || !self->enemy->inUse || self->enemy->health <= 0)
        return;

    if (range_to(self, self->enemy) <= RANGE_MELEE) {
        self->monsterInfo.nextFrame = FRAME_attackb1;
    } else if (frandom() > 0.6f) {
        self->monsterInfo.nextFrame = FRAME_runattack1;
    } else {
        self->monsterInfo.nextFrame = FRAME_runb1;
    }
}

static MonsterFrame knight_frames_stationary[] = {
    { ai_stand }
};
MMOVE_T(knight_move_stationary) = { FRAME_stand1, FRAME_stand1, knight_frames_stationary, nullptr };

static MonsterFrame knight_frames_kneel[] = {
    { ai_stand }
};
MMOVE_T(knight_move_kneel) = { FRAME_kneel5, FRAME_kneel5, knight_frames_kneel, nullptr };

static MonsterFrame knight_frames_stand[] = {
    { ai_stand, 0, knight_idle },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand },
    { ai_stand }
};
MMOVE_T(knight_move_stand) = { FRAME_stand1, FRAME_stand9, knight_frames_stand, knight_stand };

MONSTERINFO_STAND(knight_stand) (gentity_t* self) -> void {
    if (self->monsterInfo.active_move == &knight_move_stationary)
        M_SetAnimation(self, &knight_move_stationary);
    else if (self->monsterInfo.active_move == &knight_move_kneel)
        M_SetAnimation(self, &knight_move_kneel);
    else
        M_SetAnimation(self, &knight_move_stand);
}

static MonsterFrame knight_frames_walk[] = {
    { ai_walk, 2 },
    { ai_walk, 5 },
    { ai_walk, 5 },
    { ai_walk, 4 },
    { ai_walk, 2 },
    { ai_walk, 2 },
    { ai_walk, 3 },
    { ai_walk, 3 },
    { ai_walk, 4 },
    { ai_walk, 4 },
    { ai_walk, 2 },
    { ai_walk, 3 },
    { ai_walk, 2 },
    { ai_walk, 2 }
};
MMOVE_T(knight_move_walk) = { FRAME_walk1, FRAME_walk14, knight_frames_walk, nullptr };

MONSTERINFO_WALK(knight_walk) (gentity_t* self) -> void {
    M_SetAnimation(self, &knight_move_walk);
}

static MonsterFrame knight_frames_run[] = {
    { ai_run, 16, knight_step },
    { ai_run, 20 },
    { ai_run, 13 },
    { ai_run, 7, knight_step },
    { ai_run, 16 },
    { ai_run, 20, knight_step },
    { ai_run, 14 },
    { ai_run, 6, knight_check_dist }
};
MMOVE_T(knight_move_run) = { FRAME_runb1, FRAME_runb8, knight_frames_run, nullptr };

MONSTERINFO_RUN(knight_run) (gentity_t* self) -> void {
    if (self->monsterInfo.active_move == &knight_move_stationary || self->monsterInfo.active_move == &knight_move_kneel)
        M_SetAnimation(self, &knight_move_run);
    else if (self->monsterInfo.aiFlags & AI_STAND_GROUND)
        M_SetAnimation(self, &knight_move_stand);
    else
        M_SetAnimation(self, &knight_move_run);
}

static MonsterFrame knight_frames_attack_run[] = {
    { ai_charge, 20 },
    { ai_charge, 20, knight_sword_sound },
    { ai_charge, 13 },
    { ai_charge, 7 },
    { ai_charge, 16 },
    { ai_charge, 20, knight_run_strike },
    { ai_charge, 14 },
    { ai_charge, 14 },
    { ai_charge, 14 },
    { ai_charge, 14 },
    { ai_charge, 6, knight_check_dist }
};
MMOVE_T(knight_move_attack_run) = { FRAME_runattack1, FRAME_runattack11, knight_frames_attack_run, knight_run };

MONSTERINFO_ATTACK(knight_attack_run) (gentity_t* self) -> void {
    M_SetAnimation(self, &knight_move_attack_run);
}

static MonsterFrame knight_frames_attack[] = {
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, knight_melee_hit },
    { ai_charge },
    { ai_charge },
    { ai_charge, 0, knight_check_dist }
};
MMOVE_T(knight_move_attack) = { FRAME_attackb1, FRAME_attackb10, knight_frames_attack, knight_run };

MONSTERINFO_MELEE(knight_melee) (gentity_t* self) -> void {
    M_SetAnimation(self, &knight_move_attack);
}

static MonsterFrame knight_frames_pain1[] = {
    { ai_move },
    { ai_move },
    { ai_move }
};
MMOVE_T(knight_move_pain1) = { FRAME_pain1, FRAME_pain3, knight_frames_pain1, knight_run };

static MonsterFrame knight_frames_pain2[] = {
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
    { ai_move }
};
MMOVE_T(knight_move_pain2) = { FRAME_painb1, FRAME_painb11, knight_frames_pain2, knight_run };

static PAIN(knight_pain) (gentity_t* self, gentity_t* other, float kick, int damage, const MeansOfDeath& mod) -> void {
    if (level.time < self->pain_debounce_time)
        return;

    self->pain_debounce_time = level.time + 3_sec;

    if (!M_ShouldReactToPain(self, mod))
        return;

    gi.sound(self, CHAN_VOICE, s_pain, 1, ATTN_NORM, 0);

    if (frandom() < 0.5f)
        M_SetAnimation(self, &knight_move_pain1);
    else
        M_SetAnimation(self, &knight_move_pain2);
}

MONSTERINFO_SETSKIN(knight_setskin) (gentity_t* self) -> void {
    if (self->health < (self->maxHealth / 2))
        self->s.skinNum |= 1;
    else
        self->s.skinNum &= ~1;
}

static void knight_shrink(gentity_t* self) {
    self->maxs[2] = 0;
    self->svFlags |= SVF_DEADMONSTER;
    gi.linkEntity(self);
}

static MonsterFrame knight_frames_death1[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 0, knight_shrink },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
MMOVE_T(knight_move_death1) = { FRAME_death1, FRAME_death10, knight_frames_death1, monster_dead };

static MonsterFrame knight_frames_death2[] = {
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move, 0, knight_shrink },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move },
    { ai_move }
};
MMOVE_T(knight_move_death2) = { FRAME_deathb1, FRAME_deathb11, knight_frames_death2, monster_dead };

static DIE(knight_die) (gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, const Vector3& point, const MeansOfDeath& mod) -> void {
    const bool isStatue = self->spawnFlags.has(SPAWNFLAG_STATUE_STATIONARY) || (self->className && strcmp(self->className, "monster_statue") == 0);

    if (M_CheckGib(self, mod)) {
        gi.sound(self, CHAN_VOICE, gi.soundIndex("misc/udeath.wav"), 1, ATTN_NORM, 0);
        self->s.skinNum &= ~1;

        if (isStatue) {
            ThrowGibs(self, damage, {
                { 2, "models/objects/gibs/bone/tris.md2" },
                { 4, "models/objects/gibs/sm_meat/tris.md2" },
                { "models/monsters/knight/gibs/head.md2", GIB_HEAD | GIB_SKINNED | GIB_DEBRIS }
            });
        } else {
            ThrowGibs(self, damage, {
                { 2, "models/objects/gibs/bone/tris.md2" },
                { 4, "models/objects/gibs/sm_meat/tris.md2" },
                { "models/objects/gibs/chest/tris.md2" },
                { "models/monsters/knight/gibs/head.md2", GIB_HEAD | GIB_SKINNED }
            });
        }

        self->deadFlag = true;
        return;
    }

    if (self->deadFlag)
        return;

    gi.sound(self, CHAN_VOICE, s_death, 1, ATTN_NORM, 0);
    self->deadFlag = true;
    self->takeDamage = true;

    if (frandom() < 0.5f)
        M_SetAnimation(self, &knight_move_death1);
    else
        M_SetAnimation(self, &knight_move_death2);
}

static void knight_precache() {
    gi.modelIndex("models/monsters/knight/tris.md2");
    gi.modelIndex("models/monsters/knight/gibs/head.md2");
    gi.modelIndex("models/objects/gibs/chest/tris.md2");

    s_idle.assign("knight/idle.wav");
    s_sight.assign("knight/sight.wav");
    s_pain.assign("knight/pain.wav");
    s_death.assign("knight/death.wav");
    s_sword_hit.assign("knight/sword1.wav");
    s_sword_swing.assign("knight/sword2.wav");
    s_sword_miss.assign("knight/swordmiss.wav");
}

void SP_monster_knight(gentity_t* self) {
    if (!M_AllowSpawn(self)) {
        FreeEntity(self);
        return;
    }

    knight_precache();

    const bool spawnStatue = self->spawnFlags.has(SPAWNFLAG_STATUE_STATIONARY) || (self->className && strcmp(self->className, "monster_statue") == 0);

    if (!spawnStatue)
        self->className = "monster_knight";

    self->moveType = MoveType::Step;
    self->solid = SOLID_BBOX;
    self->s.modelIndex = gi.modelIndex("models/monsters/knight/tris.md2");
    self->mins = { -16, -16, -24 };
    self->maxs = { 16, 16, 32 };
    self->yawSpeed = 15;

    if (spawnStatue) {
        self->health = self->maxHealth = 125 * st.health_multiplier;
        self->gibHealth = -100;
        self->mass = 175;
        self->s.skinNum = 2;
    } else {
        self->health = self->maxHealth = 75 * st.health_multiplier;
        self->gibHealth = -40;
        self->mass = 120;
        self->s.skinNum = 0;
    }

    self->pain = knight_pain;
    self->die = knight_die;

    MonsterInfo* m = &self->monsterInfo;
    m->stand = knight_stand;
    m->walk = knight_walk;
    m->run = knight_run;
    m->attack = knight_attack_run;
    m->melee = knight_melee;
    m->sight = knight_sight;
    m->search = knight_search;
    m->idle = knight_idle;
    m->setSkin = knight_setskin;
    m->scale = MODEL_SCALE;
    m->combatStyle = CombatStyle::Melee;
    m->canJump = !(self->spawnFlags & SPAWNFLAG_KNIGHT_NOJUMPING);
    m->dropHeight = 256;
    m->jumpHeight = 68;

    gi.linkEntity(self);

    if (self->spawnFlags.has(SPAWNFLAG_STATUE_STATIONARY))
        M_SetAnimation(self, &knight_move_stationary);
    else if (self->spawnFlags.has(SPAWNFLAG_KNIGHT_KNEEL))
        M_SetAnimation(self, &knight_move_kneel);
    else
        M_SetAnimation(self, &knight_move_stand);

    walkmonster_start(self);
}
