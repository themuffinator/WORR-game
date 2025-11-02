#include "../g_local.hpp"

namespace ProBall {

namespace {

constexpr GameTime kAssistWindow = 8_sec;

LevelLocals::ProBallState& State() {
        return level.proBall;
}

inline bool BallIsLoose(const LevelLocals::ProBallState& state) {
        return state.ballEntity && state.ballEntity->inUse && !state.carrier;
}

inline bool PlayerHasBall(const gentity_t* player) {
        return player && player->client && player->client->pers.inventory[IT_BALL] > 0;
}

inline bool ValidPlayer(const gentity_t* ent) {
        return ent && ent->inUse && ent->client && ClientIsPlaying(ent->client) && !ent->client->eliminated;
}

void ClearAssist(LevelLocals::ProBallState& state) {
        state.assist = {};
}

void ResetBallToSpawn(LevelLocals::ProBallState& state) {
        if (!state.spawnEntity || !state.spawnEntity->inUse)
                return;

        gentity_t* ball = state.spawnEntity;

        ball->svFlags &= ~SVF_NOCLIENT;
        ball->solid = SOLID_TRIGGER;
        ball->moveType = MoveType::Toss;
        ball->touch = Touch_Item;
        ball->s.origin = state.spawnOrigin;
        ball->s.angles = state.spawnAngles;
        ball->velocity = vec3_origin;
        ball->avelocity = vec3_origin;
        ball->groundEntity = nullptr;
        ball->gravityVector = { 0.0f, 0.0f, -1.0f };
        gi.linkEntity(ball);

        state.ballEntity = ball;
        state.carrier = nullptr;
        state.lastToucher = nullptr;
        state.lastTouchTime = 0_ms;
        ClearAssist(state);
}

void LogGoal(const gentity_t* scorer, Team team) {
        if (!scorer || !scorer->client)
                return;

        G_LogEvent(G_Fmt("{} scored for the {} team", scorer->client->sess.netName, Teams_TeamName(team)));
}

void AnnounceGoal(const gentity_t* scorer, Team team, const gentity_t* goalEnt) {
        const char* teamName = Teams_TeamName(team);
        const char* goalLabel = (goalEnt && goalEnt->message && goalEnt->message[0]) ? goalEnt->message : "goal";

        if (scorer && scorer->client) {
                gi.LocBroadcast_Print(PRINT_HIGH, "{} scores for the {} at {}!\n",
                        scorer->client->sess.netName, teamName, goalLabel);
        }
        else {
                gi.LocBroadcast_Print(PRINT_HIGH, "{} scores!\n", teamName);
        }

        AnnouncerSound(world, team == Team::Red ? "red_scores" : "blue_scores");
}

void AwardAssist(LevelLocals::ProBallState& state, gentity_t* scorer, Team scoringTeam) {
        if (!state.assist.player || state.assist.team != scoringTeam)
                return;

        if (state.assist.expires && state.assist.expires <= level.time)
                return;

        gentity_t* assistPlayer = state.assist.player;
        if (!ValidPlayer(assistPlayer) || assistPlayer == scorer)
                return;

        if (!assistPlayer->client)
                return;

        G_AdjustPlayerScore(assistPlayer->client, 1, false, 0);
        assistPlayer->client->pers.match.proBallAssists++;
        level.match.proBallAssists++;

        gi.LocBroadcast_Print(PRINT_HIGH, "Assist: {}\n", assistPlayer->client->sess.netName);
}

void AwardGoal(gentity_t* scorer, Team team, gentity_t* goalEnt) {
        auto& state = State();

        if (team != Team::Red && team != Team::Blue)
                return;

        if (ScoringIsDisabled() || level.matchState != MatchState::In_Progress) {
                ResetBallToSpawn(state);
                return;
        }

        G_AdjustTeamScore(team, 1);
        level.match.proBallGoals++;

        if (ValidPlayer(scorer)) {
                G_AdjustPlayerScore(scorer->client, 1, false, 0);
                scorer->client->pers.match.proBallGoals++;
                scorer->client->pers.inventory[IT_BALL] = 0;
        }

        AwardAssist(state, scorer, team);

        AnnounceGoal(scorer, team, goalEnt);
        LogGoal(scorer, team);

        ResetBallToSpawn(state);
}

void ActivateLooseBall(LevelLocals::ProBallState& state, const Vector3& origin, const Vector3& velocity) {
        if (!state.spawnEntity || !state.spawnEntity->inUse)
                return;

        gentity_t* ball = state.spawnEntity;

        ball->svFlags &= ~SVF_NOCLIENT;
        ball->solid = SOLID_TRIGGER;
        ball->moveType = MoveType::Toss;
        ball->touch = Touch_Item;
        ball->s.origin = origin;
        ball->velocity = velocity;
        ball->avelocity = { 0.0f, 0.0f, 0.0f };
        ball->groundEntity = nullptr;
        ball->gravityVector = { 0.0f, 0.0f, -1.0f };
        gi.linkEntity(ball);

        state.ballEntity = ball;
        state.carrier = nullptr;
}

Vector3 CarrierDropOrigin(const gentity_t* carrier) {
        Vector3 start = carrier->s.origin;
        start.z += carrier->viewHeight * 0.5f;
        return start;
}

Vector3 ComputeDropVelocity(const gentity_t* carrier, const gentity_t* instigator, bool forced) {
        Vector3 result = carrier ? carrier->velocity : vec3_origin;

        if (forced && instigator && instigator->client) {
                result += instigator->client->vForward * 450.0f;
                result += Vector3{ 0.0f, 0.0f, 150.0f };
        }
        else {
                result += Vector3{ 0.0f, 0.0f, 150.0f };
        }

        return result;
}

Team ResolveGoalTeam(const LevelLocals::ProBallState::GoalVolume* volume, const gentity_t* actor) {
        if (volume && volume->team != Team::None)
                return volume->team;

        if (actor && actor->client)
                return actor->client->sess.team;

        return Team::None;
}

LevelLocals::ProBallState::GoalVolume* FindGoalVolume(gentity_t* ent) {
        auto& state = State();
        for (auto& goal : state.goals) {
                if (goal.ent == ent)
                        return &goal;
        }
        return nullptr;
}

bool PointInVolume(const Vector3& point, const gentity_t* volume) {
        if (!volume || !volume->inUse)
                return false;

        return point.x >= volume->absMin.x && point.x <= volume->absMax.x &&
                point.y >= volume->absMin.y && point.y <= volume->absMax.y &&
                point.z >= volume->absMin.z && point.z <= volume->absMax.z;
}

void ResetCarrierInventory(gentity_t* carrier) {
        if (carrier && carrier->client)
                carrier->client->pers.inventory[IT_BALL] = 0;
}

} // namespace

void ClearState() {
        level.proBall = {};
}

void InitLevel() {
        auto& state = State();

        if (Game::IsNot(GameType::ProBall)) {
                ClearState();
                return;
        }

        if (state.spawnEntity && state.spawnEntity->inUse) {
                state.spawnOrigin = state.spawnEntity->s.origin;
                state.spawnAngles = state.spawnEntity->s.angles;
                ResetBallToSpawn(state);
        }

        state.assist = {};
}

void RunFrame() {
        if (Game::IsNot(GameType::ProBall))
                return;

        auto& state = State();

        if (state.assist.player) {
            if (!ValidPlayer(state.assist.player) || (state.assist.expires && state.assist.expires <= level.time)) {
                        state.assist = {};
                }
        }

        if (state.carrier) {
                if (!ValidPlayer(state.carrier) || !PlayerHasBall(state.carrier)) {
                        state.carrier = nullptr;
                }
        }

        if (BallIsLoose(state)) {
                gentity_t* ball = state.ballEntity;
                const Vector3& origin = ball->s.origin;

                const contents_t contents = gi.pointContents(origin);
                if (contents & (CONTENTS_LAVA | CONTENTS_SLIME)) {
                        gi.LocBroadcast_Print(PRINT_HIGH, "Ball reset (hazard).\n");
                        ResetBallToSpawn(state);
                        return;
                }

                // Check if the ball fell below the world bounds significantly
                if (origin.z < world->absMin.z - 64.0f) {
                        gi.LocBroadcast_Print(PRINT_HIGH, "Ball reset (fell out).\n");
                        ResetBallToSpawn(state);
                        return;
                }

                for (const auto& oob : state.outOfBounds) {
                        if (PointInVolume(origin, oob)) {
                                gi.LocBroadcast_Print(PRINT_HIGH, "Ball reset (out of bounds).\n");
                                ResetBallToSpawn(state);
                                return;
                        }
                }

                for (const auto& goal : state.goals) {
                        if (!goal.ent || !goal.ent->inUse)
                                continue;

                        if (PointInVolume(origin, goal.ent)) {
                                Team scoringTeam = ResolveGoalTeam(&goal, state.lastToucher);
                                AwardGoal(state.lastToucher, scoringTeam, goal.ent);
                                return;
                        }
                }
        }
}

void RegisterBallSpawn(gentity_t* ent) {
        if (!ent)
                return;

        auto& state = State();

        state.spawnEntity = ent;
        state.ballEntity = ent;
        state.spawnOrigin = ent->s.origin;
        state.spawnAngles = ent->s.angles;
        state.carrier = nullptr;
        state.lastToucher = nullptr;
        state.lastTouchTime = 0_ms;

        ent->moveType = MoveType::Toss;
        ent->solid = SOLID_TRIGGER;
        ent->touch = Touch_Item;
        gi.linkEntity(ent);
}

void OnBallPickedUp(gentity_t* ballEnt, gentity_t* player) {
        if (Game::IsNot(GameType::ProBall) || !player || !player->client)
                return;

        auto& state = State();

        if (!state.spawnEntity)
                state.spawnEntity = ballEnt;

        state.carrier = player;
        state.lastToucher = player;
        state.lastTouchTime = level.time;
        state.ballEntity = nullptr;

        if (state.spawnEntity && state.spawnEntity->inUse) {
                state.spawnEntity->svFlags |= SVF_NOCLIENT;
                state.spawnEntity->solid = SOLID_NOT;
                state.spawnEntity->moveType = MoveType::None;
                gi.linkEntity(state.spawnEntity);
        }

        if (state.assist.player && state.assist.team != player->client->sess.team)
                state.assist = {};
}

void DropBall(gentity_t* carrier, gentity_t* instigator, bool forced) {
        if (Game::IsNot(GameType::ProBall) || !carrier || !carrier->client)
                return;

        auto& state = State();

        if (!PlayerHasBall(carrier))
                return;

        Vector3 origin = CarrierDropOrigin(carrier);
        Vector3 velocity = ComputeDropVelocity(carrier, instigator, forced);

        carrier->client->pers.inventory[IT_BALL] = 0;
        state.lastToucher = forced && instigator ? instigator : carrier;
        state.lastTouchTime = level.time;
        state.carrier = nullptr;

        if (forced && instigator && instigator->client) {
                state.assist.player = instigator;
                state.assist.team = instigator->client->sess.team;
                state.assist.expires = level.time + kAssistWindow;
        }
        else {
                state.assist = {};
        }

        ActivateLooseBall(state, origin, velocity);

        gi.LocBroadcast_Print(PRINT_HIGH, "{} drops the ball!\n",
                carrier->client->sess.netName);
}

void ThrowBall(gentity_t* carrier, const Vector3& origin, const Vector3& dir, float speed) {
        if (Game::IsNot(GameType::ProBall) || !carrier || !carrier->client)
                return;

        auto& state = State();

        if (!PlayerHasBall(carrier))
                return;

        Vector3 velocity = dir * speed + carrier->velocity;

        carrier->client->pers.inventory[IT_BALL] = 0;
        state.lastToucher = carrier;
        state.lastTouchTime = level.time;
        state.carrier = nullptr;
        state.assist = {};

        ActivateLooseBall(state, origin, velocity);

        gi.LocBroadcast_Print(PRINT_HIGH, "{} throws the ball!\n",
                carrier->client->sess.netName);
}

void HandleCarrierDeath(gentity_t* carrier) {
        if (Game::IsNot(GameType::ProBall))
                return;

        if (!PlayerHasBall(carrier))
                return;

        DropBall(carrier, nullptr, false);
}

void HandleCarrierDisconnect(gentity_t* carrier) {
        if (Game::IsNot(GameType::ProBall))
                return;

        if (!PlayerHasBall(carrier))
                return;

        DropBall(carrier, nullptr, false);
}

bool HandleCarrierHit(gentity_t* carrier, gentity_t* attacker, const MeansOfDeath& mod) {
        if (Game::IsNot(GameType::ProBall))
                return false;

        if (!PlayerHasBall(carrier))
                return false;

        if (!attacker || !attacker->client)
                return false;

        if (OnSameTeam(carrier, attacker))
                return false;

        if (mod.id != ModID::Chainfist)
                return false;

        DropBall(carrier, attacker, true);
        return true;
}

void RegisterGoalVolume(gentity_t* ent) {
        if (!ent)
                return;

        auto& state = State();

        for (auto& slot : state.goals) {
                if (!slot.ent) {
                        slot.ent = ent;
                        slot.team = Team::None;
                        if (ent->spawnFlags.has(SPAWNFLAG_PROBALL_GOAL_RED) && !ent->spawnFlags.has(SPAWNFLAG_PROBALL_GOAL_BLUE))
                                slot.team = Team::Red;
                        else if (ent->spawnFlags.has(SPAWNFLAG_PROBALL_GOAL_BLUE) && !ent->spawnFlags.has(SPAWNFLAG_PROBALL_GOAL_RED))
                                slot.team = Team::Blue;
                        return;
                }
        }

        gi.Com_PrintFmt("ProBall: ignoring goal {} (too many volumes).\n", *ent);
}

void RegisterOutOfBoundsVolume(gentity_t* ent) {
        if (!ent)
                return;

        auto& state = State();

        for (auto& slot : state.outOfBounds) {
                if (!slot) {
                        slot = ent;
                        return;
                }
        }

        gi.Com_PrintFmt("ProBall: ignoring out-of-bounds volume {} (too many volumes).\n", *ent);
}

static TOUCH(GoalTouch) (gentity_t* self, gentity_t* other, const trace_t&, bool) -> void {
        if (Game::IsNot(GameType::ProBall) || !other || !other->client)
                return;

        if (!PlayerHasBall(other))
                return;

        auto* goal = FindGoalVolume(self);
        Team scoringTeam = ResolveGoalTeam(goal, other);

        AwardGoal(other, scoringTeam, self);
}

static TOUCH(OutOfBoundsTouch) (gentity_t* self, gentity_t* other, const trace_t&, bool) -> void {
        if (Game::IsNot(GameType::ProBall))
                return;

        auto& state = State();

        if (other == state.ballEntity) {
                gi.LocBroadcast_Print(PRINT_HIGH, "Ball reset (out of bounds).\n");
                ResetBallToSpawn(state);
                return;
        }

        if (other && other->client && PlayerHasBall(other)) {
                gi.LocBroadcast_Print(PRINT_HIGH, "Ball reset (out of bounds).\n");
                ResetCarrierInventory(other);
                ResetBallToSpawn(state);
        }
}

void SpawnGoalTrigger(gentity_t* ent) {
        if (!ent)
                return;

        ent->solid = SOLID_TRIGGER;
        ent->moveType = MoveType::None;
        ent->svFlags |= SVF_NOCLIENT;
        ent->clipMask = CONTENTS_PLAYER | CONTENTS_MONSTER | CONTENTS_TRIGGER;
        ent->touch = GoalTouch;

        if (ent->model && ent->model[0])
                gi.setModel(ent, ent->model);

        gi.linkEntity(ent);
        RegisterGoalVolume(ent);
}

void SpawnOutOfBoundsTrigger(gentity_t* ent) {
        if (!ent)
                return;

        ent->solid = SOLID_TRIGGER;
        ent->moveType = MoveType::None;
        ent->svFlags |= SVF_NOCLIENT;
        ent->clipMask = CONTENTS_PLAYER | CONTENTS_MONSTER | CONTENTS_TRIGGER;
        ent->touch = OutOfBoundsTouch;

        if (ent->model && ent->model[0])
                gi.setModel(ent, ent->model);

        gi.linkEntity(ent);
        RegisterOutOfBoundsVolume(ent);
}

} // namespace ProBall

void SP_trigger_proball_goal(gentity_t* ent) {
        ProBall::SpawnGoalTrigger(ent);
}

void SP_trigger_proball_oob(gentity_t* ent) {
        ProBall::SpawnOutOfBoundsTrigger(ent);
}

