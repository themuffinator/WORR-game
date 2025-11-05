#include "../g_local.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr Vector3 BALL_MINS{ -12.f, -12.f, -12.f };
constexpr Vector3 BALL_MAXS{ 12.f, 12.f, 12.f };
constexpr GameTime BALL_THINK_INTERVAL = 50_ms;
constexpr GameTime BALL_IDLE_RESET_TIME = 10_sec;
constexpr GameTime BALL_PASS_COOLDOWN = 600_ms;
constexpr GameTime BALL_DROP_COOLDOWN = 400_ms;
constexpr GameTime BALL_OWNER_REGRAB_DELAY = 500_ms;
constexpr float BALL_PASS_SPEED = 650.f;
constexpr float BALL_DROP_OWNER_VEL_SCALE = 0.35f;
constexpr float BALL_DROP_UPWARD_SPEED = 90.f;
constexpr float BALL_THROW_UPWARD_SPEED = 180.f;
constexpr float BALL_ATTRACT_RADIUS = 256.f;
constexpr float BALL_ATTRACT_FORCE = 140.f;
constexpr float BALL_IDLE_SPEED_THRESHOLD_SQ = 64.f;
constexpr float BALL_MAX_SPEED = 1200.f;
constexpr float BALL_SIDE_JITTER = 30.f;
constexpr float BALL_OUT_OF_WORLD_DELTA = 2048.f;

[[nodiscard]] inline bool Ball_GametypeActive() {
        return Game::Is(GameType::ProBall);
}

static THINK(Ball_Think)(gentity_t* ball) -> void;
static TOUCH(Ball_Touch)(gentity_t* ball, gentity_t* other, const trace_t& tr, bool otherTouchingSelf) -> void;

[[nodiscard]] Item* Ball_Item() {
        return GetItemByIndex(IT_BALL);
}

[[nodiscard]] Team Ball_TeamForEntity(const gentity_t* owner) {
        if (!owner || !owner->client)
                return Team::None;
        return owner->client->sess.team;
}

void Ball_InitEntity(gentity_t* ball) {
        if (!ball)
                return;

        Item* item = Ball_Item();
        if (item) {
                ball->item = item;
                ball->className = item->className;
                ball->s.modelIndex = gi.modelIndex(item->worldModel);
                ball->s.effects = item->worldModelFlags;
        }

        ball->mins = BALL_MINS;
        ball->maxs = BALL_MAXS;
        ball->clipMask = MASK_SOLID;
        ball->moveType = MoveType::NewToss;
        ball->solid = SOLID_TRIGGER;
        ball->svFlags &= ~SVF_NOCLIENT;
        ball->flags &= ~FL_TEAMSLAVE;
        ball->spawnFlags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;

        if (Ball_GametypeActive()) {
                ball->s.effects |= EF_COLOR_SHELL;
                ball->s.renderFX |= RF_SHELL_RED | RF_SHELL_GREEN | RF_GLOW | RF_NO_LOD | RF_IR_VISIBLE;
        }
}

[[nodiscard]] gentity_t* Ball_EnsureEntity() {
        gentity_t* ball = level.ball.entity;
        if (ball && ball->inUse)
                return ball;

        ball = Spawn();
        if (!ball)
                return nullptr;

        Ball_InitEntity(ball);
        level.ball.entity = ball;
        return ball;
}

[[nodiscard]] Vector3 Ball_AdjustOrigin(gentity_t* owner, const Vector3& desired) {
        gentity_t* reference = Ball_EnsureEntity();
        if (!reference)
                return desired;

        const trace_t tr = gi.trace(owner ? owner->s.origin : desired, BALL_MINS, BALL_MAXS, desired, owner, MASK_SOLID);
        return tr.endPos;
}

void Ball_DetachCarrier(gentity_t* owner) {
        if (!owner || !owner->client)
                return;

        owner->client->pers.inventory[IT_BALL] = 0;
        if (owner->client->pers.selectedItem == IT_BALL) {
                owner->client->pers.selectedItem = IT_NULL;
                owner->client->ps.stats[STAT_SELECTED_ITEM] = 0;
        }
        ValidateSelectedItem(owner);

        if (level.ball.carrier == owner)
                level.ball.carrier = nullptr;
}

void Ball_StartWorldTravel(gentity_t* ball, gentity_t* owner, Team team) {
        ball->svFlags &= ~SVF_NOCLIENT;
        ball->solid = SOLID_TRIGGER;
        ball->moveType = MoveType::NewToss;
        ball->clipMask = MASK_SOLID;
        ball->touch = Ball_Touch;
        ball->think = Ball_Think;
        ball->nextThink = level.time + BALL_THINK_INTERVAL;
        ball->owner = owner;
        ball->touch_debounce_time = level.time + BALL_OWNER_REGRAB_DELAY;
        ball->timeStamp = level.time;
        ball->fteam = team;
        level.ball.carrier = nullptr;
        level.ball.idleBegin = 0_ms;
}

void Ball_ApplyVelocityClamp(gentity_t* ball) {
        const float speedSq = ball->velocity.lengthSquared();
        if (speedSq > BALL_MAX_SPEED * BALL_MAX_SPEED) {
                ball->velocity = ball->velocity.normalized() * BALL_MAX_SPEED;
        }
}

void Ball_PlaySound(gentity_t* source, const char* path) {
        if (!path)
                return;
        const int idx = gi.soundIndex(path);
        gi.sound(source, CHAN_ITEM, idx, 1.f, ATTN_NORM, 0.f);
}

}

static THINK(Ball_Think)(gentity_t* ball) -> void {
        if (!ball)
                return;

        if (!Ball_GametypeActive())
                return;

        // Allow the original owner to regrab after the debounce expires.
        if (ball->owner && ball->touch_debounce_time <= level.time)
                ball->owner = nullptr;

        // Reset if submerged in hazardous liquids.
        contents_t contents = gi.pointContents(ball->s.origin);
        if (contents & (CONTENTS_SLIME | CONTENTS_LAVA)) {
                Ball_Reset(false);
                return;
        }

        // Reset if we fell out of the world.
        if (level.ball.homeValid && ball->s.origin[2] < level.ball.homeOrigin[2] - BALL_OUT_OF_WORLD_DELTA) {
                Ball_Reset(false);
                return;
        }

        // Track how long we've been idle on the ground.
        if (ball->groundEntity && ball->velocity.lengthSquared() <= BALL_IDLE_SPEED_THRESHOLD_SQ) {
                if (level.ball.idleBegin == 0_ms)
                        level.ball.idleBegin = level.time;
        }
        else {
                level.ball.idleBegin = 0_ms;
        }

        if (level.ball.idleBegin != 0_ms && level.time - level.ball.idleBegin >= BALL_IDLE_RESET_TIME) {
                Ball_Reset(false);
                return;
        }

        // Apply a soft attraction toward nearby teammates of the last thrower.
        if (ball->fteam != Team::None && ball->fteam != Team::Spectator && ball->fteam != Team::Free) {
                gentity_t* best = nullptr;
                float bestDistSq = BALL_ATTRACT_RADIUS * BALL_ATTRACT_RADIUS;

                for (auto player : active_clients()) {
                        if (!player->client)
                                continue;
                        if (player->health <= 0)
                                continue;
                        if (player == ball->owner)
                                continue;
                        if (player->client->sess.team != ball->fteam)
                                continue;

                        const Vector3 target = player->s.origin + Vector3{ 0.f, 0.f, static_cast<float>(player->viewHeight) * 0.5f };
                        const float distSq = (target - ball->s.origin).lengthSquared();
                        if (distSq < bestDistSq) {
                                bestDistSq = distSq;
                                best = player;
                        }
                }

                if (best) {
                        Vector3 desired = best->s.origin + Vector3{ 0.f, 0.f, static_cast<float>(best->viewHeight) * 0.5f };
                        Vector3 to = desired - ball->s.origin;
                        const float dist = to.length();
                        if (dist > 1.f) {
                                const float strength = std::clamp(1.f - (dist / BALL_ATTRACT_RADIUS), 0.f, 1.f);
                                ball->velocity += to.normalized() * (BALL_ATTRACT_FORCE * strength);
                                Ball_ApplyVelocityClamp(ball);
                        }
                }
        }

        ball->nextThink = level.time + BALL_THINK_INTERVAL;
}

static TOUCH(Ball_Touch)(gentity_t* ball, gentity_t* other, const trace_t& tr, bool otherTouchingSelf) -> void {
        if (!ball)
                return;

        if (!other)
                return;

        if (tr.surface && (tr.surface->flags & SURF_SKY)) {
                Ball_Reset(false);
                return;
        }

        if (!other->client)
                return;

        if (ball->owner && other == ball->owner && ball->touch_debounce_time > level.time)
                return;

        trace_t safeTrace = tr;
        safeTrace.contents &= ~(CONTENTS_LAVA | CONTENTS_SLIME);

        Touch_Item(ball, other, safeTrace, otherTouchingSelf);
}

void Ball_RegisterSpawn(gentity_t* ent) {
        if (!ent)
                return;

        if (!Ball_GametypeActive()) {
                ent->svFlags |= SVF_NOCLIENT;
                ent->solid = SOLID_NOT;
                ent->think = nullptr;
                ent->nextThink = 0_ms;
                return;
        }

        Ball_InitEntity(ent);
        level.ball.entity = ent;
        level.ball.carrier = nullptr;
        level.ball.homeOrigin = ent->s.origin;
        level.ball.homeAngles = ent->s.angles;
        level.ball.homeValid = true;
        level.ball.idleBegin = 0_ms;

        Ball_Reset(true);
}

void Ball_OnPickup(gentity_t* ball, gentity_t* player) {
        if (!ball || !player || !player->client)
                return;

        Ball_InitEntity(ball);

        ball->svFlags |= SVF_NOCLIENT;
        ball->solid = SOLID_NOT;
        ball->moveType = MoveType::None;
        ball->velocity = {};
        ball->aVelocity = {};
        ball->owner = player;
        ball->think = nullptr;
        ball->nextThink = 0_ms;
        ball->touch = nullptr;
        ball->fteam = player->client->sess.team;
        level.ball.entity = ball;
        level.ball.carrier = player;
        level.ball.idleBegin = 0_ms;

        player->client->ball.nextPassTime = level.time;
        player->client->ball.nextDropTime = level.time;

        gi.linkEntity(ball);
}

bool Ball_PlayerHasBall(const gentity_t* ent) {
        if (!ent || !ent->client)
                return false;
        return ent->client->pers.inventory[IT_BALL] > 0;
}

bool Ball_Launch(gentity_t* owner, const Vector3& start, const Vector3& dir, float speed) {
        if (!Ball_GametypeActive())
                return false;

        if (!Ball_PlayerHasBall(owner))
                return false;

        gentity_t* ball = Ball_EnsureEntity();
        if (!ball)
                return false;

        Vector3 launchDir = dir;
        if (launchDir.lengthSquared() < 1e-4f)
                launchDir = owner && owner->client ? owner->client->vForward : Vector3{ 1.f, 0.f, 0.f };
        launchDir = launchDir.normalized();

        Vector3 up{}, right{}, forward{};
        AngleVectors(VectorToAngles(launchDir), forward, right, up);

        Ball_DetachCarrier(owner);

        const Vector3 spawn = Ball_AdjustOrigin(owner, start);
        ball->s.origin = spawn;
        ball->velocity = launchDir * speed;
        ball->velocity += up * BALL_THROW_UPWARD_SPEED;
        ball->velocity += right * (crandom() * BALL_SIDE_JITTER);
        ball->aVelocity = { crandom() * 180.f, crandom() * 180.f, crandom() * 180.f };
        Ball_ApplyVelocityClamp(ball);

        Ball_StartWorldTravel(ball, owner, Ball_TeamForEntity(owner));
        gi.linkEntity(ball);

        if (owner && owner->client)
                G_PlayerNoise(owner, spawn, PlayerNoise::Weapon);

        Ball_PlaySound(owner, "weapons/hgrena1b.wav");
        return true;
}

bool Ball_Pass(gentity_t* owner, const Vector3& start, const Vector3& dir) {
        return Ball_Launch(owner, start, dir, BALL_PASS_SPEED);
}

bool Ball_Drop(gentity_t* owner, const Vector3& origin) {
        if (!Ball_GametypeActive())
                return false;

        if (!Ball_PlayerHasBall(owner))
                return false;

        gentity_t* ball = Ball_EnsureEntity();
        if (!ball)
                return false;

        Ball_DetachCarrier(owner);

        Vector3 dropOrigin = origin;
        if (owner)
                dropOrigin = owner->s.origin + Vector3{ 0.f, 0.f, static_cast<float>(owner->viewHeight) * 0.4f };

        ball->s.origin = Ball_AdjustOrigin(owner, dropOrigin);
        ball->velocity = owner ? owner->velocity * BALL_DROP_OWNER_VEL_SCALE : Vector3{};
        ball->velocity[2] += BALL_DROP_UPWARD_SPEED;
        ball->aVelocity = { crandom() * 60.f, crandom() * 60.f, crandom() * 60.f };
        Ball_ApplyVelocityClamp(ball);

        Ball_StartWorldTravel(ball, owner, Ball_TeamForEntity(owner));
        level.ball.idleBegin = level.time;
        gi.linkEntity(ball);

        Ball_PlaySound(owner, "weapons/hgrenb1a.wav");
        if (owner && owner->client)
                G_PlayerNoise(owner, ball->s.origin, PlayerNoise::Weapon);
        return true;
}

void Ball_Reset(bool silent) {
        if (!Ball_GametypeActive())
                return;

        if (!level.ball.homeValid)
                return;

        gentity_t* ball = Ball_EnsureEntity();
        if (!ball)
                return;

        Ball_InitEntity(ball);
        ball->s.origin = level.ball.homeOrigin;
        ball->s.angles = level.ball.homeAngles;
        ball->velocity = {};
        ball->aVelocity = {};
        ball->groundEntity = nullptr;
        ball->owner = nullptr;
        ball->touch_debounce_time = 0_ms;
        ball->fteam = Team::None;
        level.ball.carrier = nullptr;
        level.ball.idleBegin = 0_ms;

        Ball_StartWorldTravel(ball, nullptr, Team::None);
        gi.linkEntity(ball);

        if (!silent)
                Ball_PlaySound(ball, "items/respawn1.wav");
}

GameTime Ball_GetPassCooldown() {
        return BALL_PASS_COOLDOWN;
}

GameTime Ball_GetDropCooldown() {
        return BALL_DROP_COOLDOWN;
}

