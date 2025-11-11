#include "../g_local.hpp"
#include "g_domination.hpp"

#include <cmath>
#include <string>

extern const spawn_temp_t& ED_GetSpawnTemp();

static_assert(LevelLocals::DominationState::MAX_POINTS == MAX_DOMINATION_POINTS,
        "Domination HUD point count mismatch");

namespace {
	constexpr GameTime kDominationMinScoreInterval = 100_ms;
	constexpr float kDominationDefaultTickIntervalSeconds = 1.0f;
	constexpr int32_t kDominationDefaultPointsPerTick = 1;

	GameTime DominationTickInterval() {
		float seconds = kDominationDefaultTickIntervalSeconds;

		if (g_domination_tick_interval) {
			const float configured = g_domination_tick_interval->value;
			if (std::isfinite(configured) && configured > 0.0f)
				seconds = configured;
		}

		GameTime interval = GameTime::from_sec(seconds);
		if (!interval || interval < kDominationMinScoreInterval)
			interval = kDominationMinScoreInterval;

		return interval;
	}

	int32_t DominationPointsPerTick() {
		int32_t points = kDominationDefaultPointsPerTick;

		if (g_domination_points_per_tick) {
			const int32_t configured = g_domination_points_per_tick->integer;
			if (configured > 0)
				points = configured;
		}

		return points;
	}

	constexpr float kDominationBeamTraceDistance = 8192.0f;

	constexpr int32_t PackColor(const rgba_t& color) {
		return static_cast<int32_t>(color.a)
			| (static_cast<int32_t>(color.b) << 8)
			| (static_cast<int32_t>(color.g) << 16)
			| (static_cast<int32_t>(color.r) << 24);
	}

	int32_t BeamColorForTeam(Team team) {
		switch (team) {
		case Team::Red:
			return PackColor(rgba_red);
		case Team::Blue:
			return PackColor(rgba_blue);
		default:
			return PackColor(rgba_white);
		}
	}

	void FreePointBeam(LevelLocals::DominationState::Point& point) {
		if (point.beam) {
			FreeEntity(point.beam);
			point.beam = nullptr;
		}
	}

	void EnsurePointBeam(LevelLocals::DominationState::Point& point) {
		if (!point.ent || !point.ent->inUse || point.ent->spawn_count != point.spawnCount) {
			FreePointBeam(point);
			if (!point.ent || !point.ent->inUse || point.ent->spawn_count != point.spawnCount) {
				point.ent = nullptr;
				point.owner = Team::None;
				point.spawnCount = 0;
			}
			return;
		}

		gentity_t*& beam = point.beam;

		if (!beam) {
			beam = Spawn();
			beam->className = "domination_point_beam";
			beam->moveType = MoveType::None;
			beam->solid = SOLID_NOT;
			beam->s.renderFX = RF_BEAM;
			beam->s.modelIndex = MODELINDEX_WORLD;
			beam->s.frame = 4;
		}

		beam->owner = point.ent;
		beam->count = static_cast<int32_t>(point.index);
		beam->moveType = MoveType::None;
		beam->solid = SOLID_NOT;
		beam->s.renderFX |= RF_BEAM;
		beam->s.modelIndex = MODELINDEX_WORLD;
		beam->s.frame = 4;
		beam->svFlags &= ~SVF_NOCLIENT;

		const Vector3 start = point.ent->s.origin;
		const Vector3 end = start + Vector3{ 0.0f, 0.0f, kDominationBeamTraceDistance };

		const trace_t tr = gi.trace(start, vec3_origin, vec3_origin, end, point.ent, MASK_SOLID);

		beam->s.origin = start;
		beam->s.oldOrigin = tr.endPos;
		beam->s.skinNum = BeamColorForTeam(point.owner);

		gi.linkEntity(beam);
	}

	LevelLocals::DominationState::Point* FindPointForEntity(gentity_t* ent) {
		auto& dom = level.domination;
		for (size_t i = 0; i < dom.count; ++i) {
			if (dom.points[i].ent == ent && dom.points[i].spawnCount == ent->spawn_count)
				return &dom.points[i];
		}
		return nullptr;
	}

	void ApplyPointOwnerVisual(LevelLocals::DominationState::Point& point) {
		if (!point.ent)
			return;

		switch (point.owner) {
		case Team::Red:
			point.ent->s.skinNum = 1;
			break;
		case Team::Blue:
			point.ent->s.skinNum = 2;
			break;
		default:
			point.ent->s.skinNum = 0;
			break;
		}

		EnsurePointBeam(point);
	}

	Team SpawnFlagOwner(const gentity_t* ent) {
		const bool red = ent->spawnFlags.has(SPAWNFLAG_DOMINATION_START_RED);
		const bool blue = ent->spawnFlags.has(SPAWNFLAG_DOMINATION_START_BLUE);

		if (red == blue)
			return Team::None;

		return red ? Team::Red : Team::Blue;
	}

	LevelLocals::DominationState::Point* RegisterPoint(gentity_t* ent) {
		auto& dom = level.domination;

		if (dom.count >= LevelLocals::DominationState::MAX_POINTS) {
			gi.Com_PrintFmt("Domination: ignoring {} because the maximum number of points ({}) has been reached.\n", *ent,
				LevelLocals::DominationState::MAX_POINTS);
			return nullptr;
		}

		auto& point = dom.points[dom.count];
		FreePointBeam(point);
		point = {};
		point.ent = ent;
		point.index = dom.count;
		point.owner = SpawnFlagOwner(ent);
		point.spawnCount = ent->spawn_count;
		++dom.count;

		return &point;
	}

        std::string PointLabel(const gentity_t* ent, size_t index) {
                if (ent->message && ent->message[0])
                        return ent->message;
                if (ent->targetName && ent->targetName[0])
                        return ent->targetName;
                return G_Fmt("Point {}", index + 1).data();
        }

	void AnnounceCapture(gentity_t* ent, Team team, size_t index) {
		const std::string label = PointLabel(ent, index);
		gi.LocBroadcast_Print(PRINT_HIGH, "{} captured {}.\n", Teams_TeamName(team), label.c_str());
	}

	TOUCH(Domination_PointTouch)(gentity_t* self, gentity_t* other, const trace_t&, bool) -> void {
		if (!other->client)
			return;
		if (!ClientIsPlaying(other->client) || other->client->eliminated)
			return;
		if (Game::IsNot(GameType::Domination))
			return;

		const Team team = other->client->sess.team;
		if (team != Team::Red && team != Team::Blue)
			return;

		if (self->touch_debounce_time && self->touch_debounce_time > level.time)
			return;

		auto* point = FindPointForEntity(self);
		if (!point)
			return;

		if (point->owner == team)
			return;

		point->owner = team;
		self->touch_debounce_time = level.time + 500_ms;

		ApplyPointOwnerVisual(*point);
		AnnounceCapture(self, team, point->index);
	}

	void EnsureBounds(gentity_t* ent, const spawn_temp_t& st) {
		if (ent->model && ent->model[0]) {
			gi.setModel(ent, ent->model);
			return;
		}

		if (ent->mins || ent->maxs)
			return;

		const float radius = (st.radius > 0.0f) ? st.radius : 64.0f;
		const float height = (st.height > 0) ? static_cast<float>(st.height) : 72.0f;

		ent->mins = { -radius, -radius, 0.0f };
		ent->maxs = { radius, radius, height };
	}

} // namespace

/*
=============
Domination_PointLabel

Exposes the shared logic for deriving Domination point labels for HUD usage.
=============
*/
std::string Domination_PointLabel(const gentity_t* ent, size_t index) {
	return PointLabel(ent, index);
}

void Domination_ClearState() {
        for (auto& point : level.domination.points) {
                FreePointBeam(point);
        }

	level.domination = {};
}

void Domination_InitLevel() {
	if (Game::IsNot(GameType::Domination)) {
		Domination_ClearState();
		return;
	}

	auto& dom = level.domination;
	if (dom.count > LevelLocals::DominationState::MAX_POINTS)
		dom.count = LevelLocals::DominationState::MAX_POINTS;

		dom.nextScoreTime = level.time + DominationTickInterval();

	for (size_t i = 0; i < dom.count; ++i) {
		dom.points[i].index = i;
		ApplyPointOwnerVisual(dom.points[i]);
	}
}

void Domination_RunFrame() {
	if (Game::IsNot(GameType::Domination))
		return;
	if (level.matchState != MatchState::In_Progress)
		return;
	if (ScoringIsDisabled())
		return;

	auto& dom = level.domination;
	if (!dom.count)
		return;

	const GameTime interval = DominationTickInterval();

	if (!dom.nextScoreTime)
		dom.nextScoreTime = level.time + interval;

	if (level.time < dom.nextScoreTime)
		return;

	dom.nextScoreTime = level.time + interval;

	int redOwned = 0;
	int blueOwned = 0;

	for (size_t i = 0; i < dom.count; ++i) {
		auto& point = dom.points[i];
		if (!point.ent || !point.ent->inUse || point.ent->spawn_count != point.spawnCount) {
			FreePointBeam(point);
			if (!point.ent || !point.ent->inUse || point.ent->spawn_count != point.spawnCount) {
				point.ent = nullptr;
				point.owner = Team::None;
				point.spawnCount = 0;
			}
			continue;
		}

		if (point.owner == Team::Red)
			++redOwned;
		else if (point.owner == Team::Blue)
			++blueOwned;
	}

	if (!redOwned && !blueOwned)
		return;

	const int32_t pointsPerTick = DominationPointsPerTick();

	if (redOwned)
		G_AdjustTeamScore(Team::Red, redOwned * pointsPerTick);
	if (blueOwned)
		G_AdjustTeamScore(Team::Blue, blueOwned * pointsPerTick);
}

void SP_domination_point(gentity_t* ent) {
	const spawn_temp_t& st = ED_GetSpawnTemp();

	ent->solid = SOLID_TRIGGER;
	ent->moveType = MoveType::None;
	ent->svFlags |= SVF_NOCLIENT;
	ent->clipMask = CONTENTS_PLAYER;
	ent->touch = Domination_PointTouch;

	EnsureBounds(ent, st);

	auto* point = RegisterPoint(ent);
	if (!point) {
		ent->touch = nullptr;
		ent->solid = SOLID_NOT;
		ent->clipMask = CONTENTS_NONE;
	}
	else {
		ent->count = static_cast<int32_t>(point->index);
	}

	gi.linkEntity(ent);

	if (point)
		ApplyPointOwnerVisual(*point);
}
