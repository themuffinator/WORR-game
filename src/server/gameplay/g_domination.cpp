#include "../g_local.hpp"

#include <string>

extern const spawn_temp_t& ED_GetSpawnTemp();

namespace {
	constexpr GameTime kDominationScoreInterval = 5_sec;

	LevelLocals::DominationState::Point* FindPointForEntity(gentity_t* ent) {
		auto& dom = level.domination;
		for (size_t i = 0; i < dom.count; ++i) {
			if (dom.points[i].ent == ent)
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
		point = {};
		point.ent = ent;
		point.index = dom.count;
		point.owner = SpawnFlagOwner(ent);
		++dom.count;

		ApplyPointOwnerVisual(point);
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

void Domination_ClearState() {
	level.domination = {};
}

void Domination_InitLevel() {
	if (Game::IsNot(GameType::Domination)) {
		level.domination = {};
		return;
	}

	auto& dom = level.domination;
	if (dom.count > LevelLocals::DominationState::MAX_POINTS)
		dom.count = LevelLocals::DominationState::MAX_POINTS;

	dom.nextScoreTime = level.time + kDominationScoreInterval;

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

	if (!dom.nextScoreTime)
		dom.nextScoreTime = level.time + kDominationScoreInterval;

	if (level.time < dom.nextScoreTime)
		return;

	dom.nextScoreTime = level.time + kDominationScoreInterval;

	int redOwned = 0;
	int blueOwned = 0;

	for (size_t i = 0; i < dom.count; ++i) {
		const auto& point = dom.points[i];
		if (!point.ent)
			continue;

		if (point.owner == Team::Red)
			++redOwned;
		else if (point.owner == Team::Blue)
			++blueOwned;
	}

	if (!redOwned && !blueOwned)
		return;

	if (redOwned)
		G_AdjustTeamScore(Team::Red, redOwned);
	if (blueOwned)
		G_AdjustTeamScore(Team::Blue, blueOwned);
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
}
