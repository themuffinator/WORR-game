#include "../g_local.hpp"

#include <optional>
#include <string>

extern const spawn_temp_t& ED_GetSpawnTemp();

namespace {
	constexpr GameTime kDominationScoreInterval = 5_sec;

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

	std::optional<size_t> TeamProgressIndex(Team team) {
		switch (team) {
		case Team::Red:
			return 0;
		case Team::Blue:
			return 1;
		default:
			return std::nullopt;
		}
	}

	GameTime SecondsToTime(const cvar_t* cvar) {
		float value = cvar ? cvar->value : 0.0f;
		if (value < 0.0f)
			value = 0.0f;
		return GameTime::from_sec(value);
	}

	void FreePointBeam(LevelLocals::DominationState::Point& point) {
		if (point.beam) {
			FreeEntity(point.beam);
			point.beam = nullptr;
		}
	}

	void EnsurePointBeam(LevelLocals::DominationState::Point& point) {
		if (!point.ent || !point.ent->inUse) {
			FreePointBeam(point);
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

	void AnnounceNeutralize(gentity_t* ent, Team team, Team previousOwner, size_t index) {
		const std::string label = PointLabel(ent, index);
		if (previousOwner == Team::None) {
			gi.LocBroadcast_Print(PRINT_HIGH, "{} neutralized {}.\n", Teams_TeamName(team), label.c_str());
		}
		else {
			gi.LocBroadcast_Print(PRINT_HIGH, "{} neutralized {} from {}.\n", Teams_TeamName(team), label.c_str(),
				Teams_TeamName(previousOwner));
		}
	}

	TOUCH(Domination_PointTouch)(gentity_t* self, gentity_t* other, const trace_t&, bool) -> void {
		if (!other->client)
			return;
		if (!ClientIsPlaying(other->client) || other->client->eliminated)
			return;
		if (Game::IsNot(GameType::Domination))
			return;

		const Team team = other->client->sess.team;
		const auto teamIndex = TeamProgressIndex(team);
		if (!teamIndex)
			return;

		auto* point = FindPointForEntity(self);
		if (!point)
			return;

		const auto opponentIndex = TeamProgressIndex(Teams_OtherTeam(team));
		auto& captureProgress = point->captureProgress[*teamIndex];
		auto& neutralizeProgress = point->neutralizeProgress[*teamIndex];
		auto& lastUpdate = point->lastProgressUpdate[*teamIndex];

		const GameTime timeout = FRAME_TIME_MS * 2;
		if (lastUpdate && (level.time - lastUpdate) > timeout) {
			captureProgress = 0_ms;
			neutralizeProgress = 0_ms;
		}
		lastUpdate = level.time;

		if (opponentIndex) {
			point->captureProgress[*opponentIndex] = 0_ms;
			point->neutralizeProgress[*opponentIndex] = 0_ms;
			point->lastProgressUpdate[*opponentIndex] = 0_ms;
		}

		if (point->owner == team) {
			captureProgress = 0_ms;
			neutralizeProgress = 0_ms;
			return;
		}

		GameTime carryOver = FRAME_TIME_MS;
		const GameTime neutralizeTime = SecondsToTime(g_domination_neutralize_time);
		const GameTime captureTime = SecondsToTime(g_domination_capture_time);

		if (point->owner != Team::None) {
			neutralizeProgress += carryOver;
			if (neutralizeTime > 0_ms && neutralizeProgress < neutralizeTime)
				return;

			if (neutralizeTime > 0_ms)
				carryOver = neutralizeProgress - neutralizeTime;
			else
				carryOver = neutralizeProgress;

			const Team previousOwner = point->owner;
			neutralizeProgress = 0_ms;
			point->owner = Team::None;
			ApplyPointOwnerVisual(*point);
			AnnounceNeutralize(self, team, previousOwner, point->index);
		}

		if (carryOver < 0_ms)
			carryOver = 0_ms;

		captureProgress += carryOver;
		if (captureTime > 0_ms && captureProgress < captureTime)
			return;

		point->owner = team;
		point->captureProgress.fill(0_ms);
		point->neutralizeProgress.fill(0_ms);
		point->lastProgressUpdate.fill(0_ms);

		ApplyPointOwnerVisual(*point);
		AnnounceCapture(self, team, point->index);

		if (g_domination_capture_bonus && g_domination_capture_bonus->integer)
			G_AdjustPlayerScore(other->client, g_domination_capture_bonus->integer, false, 0);
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

	if (point)
		ApplyPointOwnerVisual(*point);
}
