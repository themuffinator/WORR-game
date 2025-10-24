#include "g_local.hpp"

// --- AI States ---
MONSTERINFO_STAND(mutant_stand) (gentity_t* self) -> void;
MONSTERINFO_WALK(mutant_walk) (gentity_t* self) -> void;
MONSTERINFO_RUN(mutant_run) (gentity_t* self) -> void;
MONSTERINFO_ATTACK(mutant_jump) (gentity_t* self) -> void;
MONSTERINFO_MELEE(mutant_melee) (gentity_t* self) -> void;
MONSTERINFO_IDLE(mutant_idle) (gentity_t* self) -> void;
MONSTERINFO_SIGHT(mutant_sight) (gentity_t* self, gentity_t* other) -> void;
MONSTERINFO_SEARCH(mutant_search) (gentity_t* self) -> void;
MONSTERINFO_CHECKATTACK(mutant_checkattack) (gentity_t* self) -> bool;
MONSTERINFO_BLOCKED(mutant_blocked) (gentity_t* self, float dist) -> bool;

// --- Events ---
PAIN(mutant_pain) (gentity_t* self, gentity_t* other, float kick, int damage, const MeansOfDeath& mod) -> void;
DIE(mutant_die) (gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, const Vector3& point, const MeansOfDeath& mod) -> void;

// --- Misc ---
MONSTERINFO_SETSKIN(mutant_setskin) (gentity_t* self) -> void;

// --- AI States ---
MONSTERINFO_STAND(soldier_stand) (gentity_t* self) -> void;
MONSTERINFO_WALK(soldier_walk) (gentity_t* self) -> void;
MONSTERINFO_RUN(soldier_run) (gentity_t* self) -> void;
MONSTERINFO_ATTACK(soldier_attack) (gentity_t* self) -> void;
MONSTERINFO_SIGHT(soldier_sight) (gentity_t* self, gentity_t* other) -> void;
MONSTERINFO_BLOCKED(soldier_blocked) (gentity_t* self, float dist) -> bool;
MONSTERINFO_DUCK(soldier_duck) (gentity_t* self, GameTime eta) -> bool;
MONSTERINFO_UNDUCK(monster_duck_up) (gentity_t* self) -> void;
MONSTERINFO_SIDESTEP(soldier_sidestep) (gentity_t* self) -> bool;

// --- Events ---
PAIN(soldier_pain) (gentity_t* self, gentity_t* other, float kick, int damage, const MeansOfDeath& mod) -> void;
DIE(soldier_die) (gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, const Vector3& point, const MeansOfDeath& mod) -> void;

// --- Misc ---
MONSTERINFO_SETSKIN(soldier_setskin) (gentity_t* self) -> void;

// --- AI States ---
MONSTERINFO_STAND(soldier_stand) (gentity_t* self) -> void;
MONSTERINFO_WALK(soldier_walk) (gentity_t* self) -> void;
MONSTERINFO_RUN(soldier_run) (gentity_t* self) -> void;
MONSTERINFO_ATTACK(soldier_attack) (gentity_t* self) -> void;
MONSTERINFO_SIGHT(soldier_sight) (gentity_t* self, gentity_t* other) -> void;
MONSTERINFO_BLOCKED(soldier_blocked) (gentity_t* self, float dist) -> bool;
MONSTERINFO_DUCK(soldier_duck) (gentity_t* self, GameTime eta) -> bool;
MONSTERINFO_UNDUCK(monster_duck_up) (gentity_t* self) -> void;
MONSTERINFO_SIDESTEP(soldier_sidestep) (gentity_t* self) -> bool;

// --- Events ---
PAIN(soldier_pain) (gentity_t* self, gentity_t* other, float kick, int damage, const MeansOfDeath& mod) -> void;
DIE(soldier_die) (gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, const Vector3& point, const MeansOfDeath& mod) -> void;

// --- Misc ---
MONSTERINFO_SETSKIN(soldier_setskin) (gentity_t* self) -> void;

// --- AI States ---
MONSTERINFO_STAND(tank_stand) (gentity_t* self) -> void;
MONSTERINFO_WALK(tank_walk) (gentity_t* self) -> void;
MONSTERINFO_RUN(tank_run) (gentity_t* self) -> void;
MONSTERINFO_ATTACK(tank_attack) (gentity_t* self) -> void;
MONSTERINFO_IDLE(tank_idle) (gentity_t* self) -> void;
MONSTERINFO_SIGHT(tank_sight) (gentity_t* self, gentity_t* other) -> void;
MONSTERINFO_BLOCKED(tank_blocked) (gentity_t* self, float dist) -> bool;

// --- Events ---
PAIN(tank_pain) (gentity_t* self, gentity_t* other, float kick, int damage, const MeansOfDeath& mod) -> void;
DIE(tank_die) (gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, const Vector3& point, const MeansOfDeath& mod) -> void;

// --- Misc ---
MONSTERINFO_SETSKIN(tank_setskin) (gentity_t* self) -> void;

enum class MonsterID {
    Berserker,
    Gladiator,
    Gunner,
    Infantry,
    SoldierLight,
    Soldier,
    SoldierSSG,
    Tank,
    TankCommander,
    Medic,
    Flipper,
    IronMaiden, // monster_chick
    Parasite,
    Flyer,
    Brain,
    Floater,
    Hover,
    Mutant,
    SuperTank,
    Hornet, // monster_boss2
    Boss3,  // monster_boss3_stand
    Jorg,
    Makron,
    TankStand,
    Guardian,
    Arachnid,
    GunCommander,
    CommanderBody,
    SoldierHypergun,
    SoldierLasergun,
    SoldierRipper,
    Fixbot,
    Gekk,
    IronMaidenHeat, // monster_chick_heat
    GladiatorBeta,  // monster_gladb
    BlackWidow,     // monster_boss5
    Stalker,
    Turret,
    Daedalus,
    Carrier,
    Widow,
    Widow2,
    MedicCommander,
    Kamikaze,
    Shambler,
    Dog,
    Ogre,
    OgreMarksman,
    Fish,
    Grunt, // monster_army
    Fiend, // monster_demon1
    Zombie,
    Spawn, // monster_tarbaby
    Vore,  // monster_shalrath
    Enforcer,
    Knight,
    HellKnight,
    Scrag, // monster_wizard
    OldOne,
    Chthon, // monster_boss
    _Count
};

// A data-driven structure to define core monster properties.
struct MonsterDefinition {
    MonsterID                       index;
    std::string_view                longName;
    std::string_view                className;
    std::string_view                modelName;
    Vector3                         mins;
    Vector3                         maxs;
    float                           scale = 1.0f;
    int                             mass = 100;
    int                             health;
    int                             gibHealth;
    void                            (*startFunc)(gentity_t*);
    save_monsterinfo_stand_t        standFunc;
    save_monsterinfo_walk_t         walkFunc;
    save_monsterinfo_run_t          runFunc;
    save_monsterinfo_attack_t       attackFunc;
    save_monsterinfo_checkattack_t  checkAttackFunc;
    save_pain_t                     painFunc;
    save_die_t                      dieFunc;
    save_monsterinfo_sight_t        sightFunc;
    save_monsterinfo_search_t       searchFunc = nullptr;
    save_monsterinfo_idle_t         idleFunc = nullptr;
    save_monsterinfo_melee_t        meleeFunc = nullptr;
    save_monsterinfo_dodge_t        dodgeFunc = nullptr;
    save_monsterinfo_blocked_t      blockedFunc = nullptr;
    save_monsterinfo_duck_t         duckFunc = nullptr;
    save_monsterinfo_unduck_t       unDuckFunc = nullptr;
    save_monsterinfo_sidestep_t     sideStepFunc = nullptr;
    save_monsterinfo_setskin_t      setSkinFunc = nullptr;
    bool                            canJump = false;
    float                           dropHeight = 0.0f;
    float                           jumpHeight = 0.0f;
    CombatStyle                     combatStyle = CombatStyle::Unknown;
    monster_ai_flags_t              initialFlags = AI_NONE;
    bool                            blindFire = false;
    std::vector<std::string_view>   precaches;
};

static const std::array<MonsterDefinition, static_cast<size_t>(MonsterID::_Count)> monsterDefinitionTable = { {
        // --- Berserker (Partially detailed from g_horde.cpp) ---
        [static_cast<size_t>(MonsterID::Berserker)] = {
            .index = MonsterID::Berserker, .longName = "Berserker", .className = "monster_berserk",
            .startFunc = walkmonster_start,
            .itemDrops = {IT_ARMOR_SHARD}
        },
    // --- Gladiator (Partially detailed from g_horde.cpp) ---
    [static_cast<size_t>(MonsterID::Gladiator)] = {
        .index = MonsterID::Gladiator, .longName = "Gladiator", .className = "monster_gladiator",
        .startFunc = walkmonster_start,
        .itemDrops = {IT_AMMO_SLUGS}
    },
    // --- Gunner (Partially detailed from g_horde.cpp) ---
    [static_cast<size_t>(MonsterID::Gunner)] = {
        .index = MonsterID::Gunner, .longName = "Gunner", .className = "monster_gunner",
        .startFunc = walkmonster_start,
        .itemDrops = {IT_AMMO_GRENADES, IT_AMMO_BULLETS_SMALL}
    },
    // --- Infantry (Partially detailed from g_horde.cpp) ---
    [static_cast<size_t>(MonsterID::Infantry)] = {
        .index = MonsterID::Infantry, .longName = "Marine", .className = "monster_infantry",
        .startFunc = walkmonster_start,
        .itemDrops = {IT_AMMO_BULLETS_SMALL, IT_AMMO_BULLETS}
    },
    // --- Light Guard (Fully detailed from m_soldier.cpp) ---
    [static_cast<size_t>(MonsterID::SoldierLight)] = {
        .index = MonsterID::SoldierLight, .longName = "Light Guard", .className = "monster_soldier_light",
        .modelName = "models/monsters/soldier/tris.md2",
        .mins = {-16, -16, -24}, .maxs = {16, 16, 32}, .scale = 1.2f, .mass = 100,
        .health = 20, .gibHealth = -30,
        .startFunc = walkmonster_start,
        .standFunc = soldier_stand, .walkFunc = soldier_walk, .runFunc = soldier_run,
        .attackFunc = soldier_attack, .checkAttackFunc = M_CheckAttack,
        .painFunc = soldier_pain, .dieFunc = soldier_die, .sightFunc = soldier_sight,
        .dodgeFunc = M_MonsterDodge, .blockedFunc = soldier_blocked,
        .duckFunc = soldier_duck, .unDuckFunc = monster_duck_up, .sideStepFunc = soldier_sidestep,
        .setSkinFunc = soldier_setskin, .blindFire = true,
        .precaches = {"soldier/solpain2.wav", "soldier/soldeth2.wav", "soldier/solatck2.wav"},
        .itemDrops = {IT_HEALTH_SMALL}
    },
    // --- Shotgun Guard (Fully detailed from m_soldier.cpp) ---
    [static_cast<size_t>(MonsterID::Soldier)] = {
        .index = MonsterID::Soldier, .longName = "Shotgun Guard", .className = "monster_soldier",
        .modelName = "models/monsters/soldier/tris.md2",
        .mins = {-16, -16, -24}, .maxs = {16, 16, 32}, .scale = 1.2f, .mass = 100,
        .health = 30, .gibHealth = -30,
        .startFunc = walkmonster_start,
        .standFunc = soldier_stand, .walkFunc = soldier_walk, .runFunc = soldier_run,
        .attackFunc = soldier_attack, .checkAttackFunc = M_CheckAttack,
        .painFunc = soldier_pain, .dieFunc = soldier_die, .sightFunc = soldier_sight,
        .dodgeFunc = M_MonsterDodge, .blockedFunc = soldier_blocked,
        .duckFunc = soldier_duck, .unDuckFunc = monster_duck_up, .sideStepFunc = soldier_sidestep,
        .setSkinFunc = soldier_setskin,
        .precaches = {"soldier/solpain1.wav", "soldier/soldeth1.wav", "soldier/solatck1.wav"},
        .itemDrops = {IT_AMMO_BULLETS_SMALL, IT_HEALTH_SMALL}
    },
    // --- Machinegun Guard (Fully detailed from m_soldier.cpp) ---
    [static_cast<size_t>(MonsterID::SoldierSSG)] = {
        .index = MonsterID::SoldierSSG, .longName = "Machinegun Guard", .className = "monster_soldier_ss",
        .modelName = "models/monsters/soldier/tris.md2",
        .mins = {-16, -16, -24}, .maxs = {16, 16, 32}, .scale = 1.2f, .mass = 100,
        .health = 40, .gibHealth = -30,
        .startFunc = walkmonster_start,
        .standFunc = soldier_stand, .walkFunc = soldier_walk, .runFunc = soldier_run,
        .attackFunc = soldier_attack, .checkAttackFunc = M_CheckAttack,
        .painFunc = soldier_pain, .dieFunc = soldier_die, .sightFunc = soldier_sight,
        .dodgeFunc = M_MonsterDodge, .blockedFunc = soldier_blocked,
        .duckFunc = soldier_duck, .unDuckFunc = monster_duck_up, .sideStepFunc = soldier_sidestep,
        .setSkinFunc = soldier_setskin,
        .precaches = {"soldier/solpain3.wav", "soldier/soldeth3.wav", "soldier/solatck3.wav"},
        .itemDrops = {IT_AMMO_SHELLS_SMALL, IT_HEALTH_SMALL}
    },
    // --- Tank (Fully detailed from m_tank.cpp) ---
    [static_cast<size_t>(MonsterID::Tank)] = {
        .index = MonsterID::Tank, .longName = "Tank", .className = "monster_tank",
        .modelName = "models/monsters/tank/tris.md2",
        .mins = {-32, -32, -16}, .maxs = {32, 32, 64}, .scale = 1.0f, .mass = 500,
        .health = 750, .gibHealth = -200,
        .startFunc = walkmonster_start,
        .standFunc = tank_stand, .walkFunc = tank_walk, .runFunc = tank_run,
        .attackFunc = tank_attack, .checkAttackFunc = M_CheckAttack,
        .painFunc = tank_pain, .dieFunc = tank_die, .sightFunc = tank_sight,
        .idleFunc = tank_idle, .blockedFunc = tank_blocked, .setSkinFunc = tank_setskin,
        .initialFlags = AI_IGNORE_SHOTS, .blindFire = true,
        .precaches = {"tank/tnkpain2.wav", "tank/death.wav"},
        .itemDrops = {IT_AMMO_ROCKETS}
    },
    // --- Tank Commander (Fully detailed from m_tank.cpp) ---
    [static_cast<size_t>(MonsterID::TankCommander)] = {
        .index = MonsterID::TankCommander, .longName = "Tank Commander", .className = "monster_tank_commander",
        .modelName = "models/monsters/tank/tris.md2",
        .mins = {-32, -32, -16}, .maxs = {32, 32, 64}, .scale = 1.0f, .mass = 500,
        .health = 1000, .gibHealth = -225,
        .startFunc = walkmonster_start,
        .standFunc = tank_stand, .walkFunc = tank_walk, .runFunc = tank_run,
        .attackFunc = tank_attack, .checkAttackFunc = M_CheckAttack,
        .painFunc = tank_pain, .dieFunc = tank_die, .sightFunc = tank_sight,
        .idleFunc = tank_idle, .blockedFunc = tank_blocked, .setSkinFunc = tank_setskin,
        .initialFlags = AI_IGNORE_SHOTS, .blindFire = true,
        .precaches = {"tank/pain.wav", "tank/death.wav"},
        .itemDrops = {IT_AMMO_ROCKETS_SMALL, IT_AMMO_BULLETS_SMALL, IT_AMMO_ROCKETS, IT_AMMO_BULLETS}
    },
    // --- Medic (Partially detailed from g_horde.cpp) ---
    [static_cast<size_t>(MonsterID::Medic)] = {
        .index = MonsterID::Medic, .longName = "Medic", .className = "monster_medic",
        .startFunc = walkmonster_start,
        .itemDrops = {IT_HEALTH_SMALL, IT_HEALTH_MEDIUM}
    },
    // --- Flipper (Placeholder) ---
    [static_cast<size_t>(MonsterID::Flipper)] = {
        .index = MonsterID::Flipper, .longName = "Barracuda Shark", .className = "monster_flipper",
        .startFunc = swimmonster_start
    },
    // --- Iron Maiden (Partially detailed from g_horde.cpp) ---
    [static_cast<size_t>(MonsterID::IronMaiden)] = {
        .index = MonsterID::IronMaiden, .longName = "Iron Maiden", .className = "monster_chick",
        .startFunc = walkmonster_start,
        .itemDrops = {IT_AMMO_ROCKETS_SMALL, IT_AMMO_ROCKETS}
    },
    // --- Parasite (Partially detailed from g_horde.cpp) ---
    [static_cast<size_t>(MonsterID::Parasite)] = {
        .index = MonsterID::Parasite, .longName = "Parasite", .className = "monster_parasite",
        .startFunc = walkmonster_start,
        .itemDrops = {IT_NULL}
    },
    // --- Flyer (Partially detailed from g_horde.cpp) ---
    [static_cast<size_t>(MonsterID::Flyer)] = {
        .index = MonsterID::Flyer, .longName = "Flyer", .className = "monster_flyer",
        .startFunc = flymonster_start,
        .itemDrops = {IT_AMMO_CELLS_SMALL}
    },
    // --- Brain (Partially detailed from g_horde.cpp) ---
    [static_cast<size_t>(MonsterID::Brain)] = {
        .index = MonsterID::Brain, .longName = "Brain", .className = "monster_brain",
        .startFunc = walkmonster_start,
        .itemDrops = {IT_AMMO_CELLS_SMALL}
    },
    // --- Technician (Partially detailed from g_horde.cpp) ---
    [static_cast<size_t>(MonsterID::Floater)] = {
        .index = MonsterID::Floater, .longName = "Technician", .className = "monster_floater",
        .startFunc = flymonster_start,
        .itemDrops = {IT_NULL}
    },
    // --- Icarus (Partially detailed from g_horde.cpp) ---
    [static_cast<size_t>(MonsterID::Hover)] = {
        .index = MonsterID::Hover, .longName = "Icarus", .className = "monster_hover",
        .startFunc = flymonster_start,
        .itemDrops = {IT_NULL}
    },
    // --- Mutant (Fully detailed from m_mutant.cpp) ---
    [static_cast<size_t>(MonsterID::Mutant)] = {
        .index = MonsterID::Mutant, .longName = "Mutant", .className = "monster_mutant",
        .modelName = "models/monsters/mutant/tris.md2",
        .mins = {-18, -18, -24}, .maxs = {18, 18, 30}, .scale = 1.0f, .mass = 300,
        .health = 300, .gibHealth = -120,
        .startFunc = walkmonster_start,
        .standFunc = mutant_stand, .walkFunc = mutant_walk, .runFunc = mutant_run,
        .attackFunc = mutant_jump, .checkAttackFunc = mutant_checkattack,
        .painFunc = mutant_pain, .dieFunc = mutant_die, .sightFunc = mutant_sight,
        .searchFunc = mutant_search, .idleFunc = mutant_idle, .meleeFunc = mutant_melee,
        .blockedFunc = mutant_blocked, .setSkinFunc = mutant_setskin,
        .canJump = true, .dropHeight = 256.0f, .jumpHeight = 68.0f,
        .combatStyle = CombatStyle::Melee, .initialFlags = AI_STINKY,
        .precaches = {"mutant/mutatck1.wav", "mutant/mutdeth1.wav"},
        .itemDrops = {IT_NULL}
    },
    // --- SuperTank (Placeholder) ---
    [static_cast<size_t>(MonsterID::SuperTank)] = {
        .index = MonsterID::SuperTank, .longName = "Super Tank", .className = "monster_supertank",
        .startFunc = walkmonster_start
    },
    // --- Hornet (Placeholder) ---
    [static_cast<size_t>(MonsterID::Hornet)] = {
        .index = MonsterID::Hornet, .longName = "Hornet", .className = "monster_boss2",
        .startFunc = walkmonster_start
    },
    // --- Boss3 (Placeholder) ---
    [static_cast<size_t>(MonsterID::Boss3)] = {
        .index = MonsterID::Boss3, .longName = "Strogg Leader", .className = "monster_boss3_stand"
    },
    // --- Jorg (Placeholder) ---
    [static_cast<size_t>(MonsterID::Jorg)] = {
        .index = MonsterID::Jorg, .longName = "Jorg", .className = "monster_jorg",
        .startFunc = walkmonster_start
    },
    // --- Makron (Placeholder) ---
    [static_cast<size_t>(MonsterID::Makron)] = {
        .index = MonsterID::Makron, .longName = "Makron", .className = "monster_makron",
        .startFunc = walkmonster_start
    },
    // --- TankStand (Placeholder) ---
    [static_cast<size_t>(MonsterID::TankStand)] = {
        .index = MonsterID::TankStand, .longName = "Ceremonial Tank", .className = "monster_tank_stand"
    },
    // --- Guardian (Placeholder) ---
    [static_cast<size_t>(MonsterID::Guardian)] = {
        .index = MonsterID::Guardian, .longName = "Guardian", .className = "monster_guardian",
        .startFunc = walkmonster_start
    },
    // --- Arachnid (Placeholder) ---
    [static_cast<size_t>(MonsterID::Arachnid)] = {
        .index = MonsterID::Arachnid, .longName = "Arachnid", .className = "monster_arachnid",
        .startFunc = walkmonster_start
    },
    // --- Gun Commander (Partially detailed from g_horde.cpp) ---
    [static_cast<size_t>(MonsterID::GunCommander)] = {
        .index = MonsterID::GunCommander, .longName = "Gun Commander", .className = "monster_guncmdr",
        .startFunc = walkmonster_start,
        .itemDrops = {IT_AMMO_GRENADES, IT_AMMO_BULLETS_SMALL, IT_AMMO_BULLETS, IT_AMMO_CELLS_SMALL}
    },

    // ... All other monsters would follow as placeholders ...

    // --- Daedalus (Partially detailed from g_horde.cpp) ---
    [static_cast<size_t>(MonsterID::Daedalus)] = {
        .index = MonsterID::Daedalus, .longName = "Daedalus", .className = "monster_daedalus",
        .startFunc = flymonster_start,
        .itemDrops = {IT_AMMO_CELLS_SMALL}
    },
    // --- Medic Commander (Partially detailed from g_horde.cpp) ---
    [static_cast<size_t>(MonsterID::MedicCommander)] = {
        .index = MonsterID::MedicCommander, .longName = "Medic Commander", .className = "monster_medic_commander",
        .startFunc = walkmonster_start,
        .itemDrops = {IT_AMMO_CELLS_SMALL, IT_HEALTH_MEDIUM, IT_HEALTH_LARGE}
    },
    // --- Kamikaze (Partially detailed from g_horde.cpp) ---
    [static_cast<size_t>(MonsterID::Kamikaze)] = {
        .index = MonsterID::Kamikaze, .longName = "Kamikaze", .className = "monster_kamikaze",
        .startFunc = flymonster_start,
        .itemDrops = {IT_NULL}
    }
} };

