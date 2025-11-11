#include "../g_local.hpp"

#include <algorithm>
#include <cmath>
#include <string>

extern const spawn_temp_t& ED_GetSpawnTemp();

namespace {
constexpr GameTime kDominationMinScoreInterval = 100_ms;
constexpr float kDominationDefaultTickIntervalSeconds = 1.0f;
constexpr int32_t kDominationDefaultPointsPerTick = 1;
constexpr float kDominationDefaultCaptureSeconds = 3.0f;
constexpr int32_t kDominationOccupantGraceMinMs = 50;
constexpr int32_t kDominationOccupantGraceMaxMs = 250;

/*
=============
DominationTickInterval

Returns the amount of time between passive domination score ticks.
=============
*/
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

/*
=============
DominationPointsPerTick

Returns the number of score points earned each domination tick.
=============
*/
int32_t DominationPointsPerTick() {
int32_t points = kDominationDefaultPointsPerTick;

if (g_domination_points_per_tick) {
const int32_t configured = g_domination_points_per_tick->integer;
if (configured > 0)
points = configured;
}

return points;
}

/*
=============
DominationCaptureTime

Returns how long a team must hold a point to capture it.
=============
*/
GameTime DominationCaptureTime() {
float seconds = kDominationDefaultCaptureSeconds;

if (g_domination_capture_time) {
const float configured = g_domination_capture_time->value;
if (std::isfinite(configured))
seconds = configured;
}

if (seconds <= 0.0f)
return 0_ms;

return GameTime::from_sec(seconds);
}

/*
=============
DominationOccupantGrace

Returns the grace period a player remains registered inside a point volume between touch events.
=============
*/
GameTime DominationOccupantGrace() {
const uint32_t frameMs = gi.frameTimeMs ? gi.frameTimeMs : 16;
const uint32_t graceMs = std::clamp<uint32_t>(frameMs * 2, kDominationOccupantGraceMinMs, kDominationOccupantGraceMaxMs);
return GameTime::from_ms(static_cast<int64_t>(graceMs));
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

/*
=============
FreePointBeam

Releases the beam entity that visually marks a domination point.
=============
*/
void FreePointBeam(LevelLocals::DominationState::Point& point) {
		if (point.beam) {
			FreeEntity(point.beam);
			point.beam = nullptr;
		}
	}

/*
=============
EnsurePointBeam

Creates or updates the beam entity for a domination point.
=============
*/
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

/*
=============
FindPointForEntity

Finds the domination point that owns the provided entity.
=============
*/
LevelLocals::DominationState::Point* FindPointForEntity(gentity_t* ent) {
		auto& dom = level.domination;
		for (size_t i = 0; i < dom.count; ++i) {
			if (dom.points[i].ent == ent && dom.points[i].spawnCount == ent->spawn_count)
				return &dom.points[i];
		}
		return nullptr;
	}

/*
=============
ApplyPointOwnerVisual

Updates skin and beam colors to reflect the owning team.
=============
*/
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

/*
=============
SpawnFlagOwner

Determines which team initially owns the point based on spawn flags.
=============
*/
Team SpawnFlagOwner(const gentity_t* ent) {
		const bool red = ent->spawnFlags.has(SPAWNFLAG_DOMINATION_START_RED);
		const bool blue = ent->spawnFlags.has(SPAWNFLAG_DOMINATION_START_BLUE);

		if (red == blue)
			return Team::None;

		return red ? Team::Red : Team::Blue;
	}

/*
=============
RegisterPoint

Registers a domination point entity with the level state.
=============
*/
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

/*
=============
PointLabel

Returns a friendly label for a domination point used in announcements.
=============
*/
std::string PointLabel(const gentity_t* ent, size_t index) {
		if (ent->message && ent->message[0])
			return ent->message;
		if (ent->targetName && ent->targetName[0])
			return ent->targetName;
		return G_Fmt("Point {}", index + 1).data();
	}

/*
=============
AnnounceCapture

Broadcasts that a team has captured the specified point.
=============
*/
void AnnounceCapture(gentity_t* ent, Team team, size_t index) {
\tconst std::string label = PointLabel(ent, index);
\tgi.LocBroadcast_Print(PRINT_HIGH, "{} captured {}.\n", Teams_TeamName(team), label.c_str());
}

/*
=============
FinalizeCapture

Applies the ownership change for a point capture and triggers visuals/announcements.
=============
*/
void FinalizeCapture(LevelLocals::DominationState::Point& point, Team newOwner) {
\tpoint.owner = newOwner;
\tpoint.capturingTeam = Team::None;
\tpoint.captureProgress = 0.0f;
\tpoint.lastProgressTime = level.time;
\tApplyPointOwnerVisual(point);
\tAnnounceCapture(point.ent, newOwner, point.index);
}

/*
=============
UpdatePointOccupants

Refreshes the tracked player counts occupying a domination point.
=============
*/
void UpdatePointOccupants(LevelLocals::DominationState::Point& point) {
\tpoint.occupantCounts.fill(0);

\tconst bool hasClients = game.clients && g_entities && game.maxClients > 0;
\tconst GameTime now = level.time;

\tfor (size_t i = 0; i < point.occupantExpiry.size(); ++i) {
\tGameTime& expiry = point.occupantExpiry[i];
\tif (!expiry)
\tcontinue;

\tif (expiry <= now) {
\texpiry = 0_ms;
\tcontinue;
\t}

\tif (!hasClients || i >= static_cast<size_t>(game.maxClients)) {
\texpiry = 0_ms;
\tcontinue;
\t}

\tgclient_t* cl = &game.clients[i];
\tgentity_t* ent = &g_entities[i + 1];
\tif (!ent || !ent->inUse || ent->client != cl) {
\texpiry = 0_ms;
\tcontinue;
\t}

\tif (!ClientIsPlaying(cl) || cl->eliminated) {
\texpiry = 0_ms;
\tcontinue;
\t}

\tconst Team team = cl->sess.team;
\tif (team != Team::Red && team != Team::Blue) {
\texpiry = 0_ms;
\tcontinue;
\t}

\t++point.occupantCounts[static_cast<size_t>(team)];
\t}
}

/*
=============
AdvanceCaptureProgress

Advances or decays capture progress depending on the players present.
=============
*/
void AdvanceCaptureProgress(LevelLocals::DominationState::Point& point) {
\tconst int redCount = point.occupantCounts[static_cast<size_t>(Team::Red)];
\tconst int blueCount = point.occupantCounts[static_cast<size_t>(Team::Blue)];
\tconst bool contested = redCount > 0 && blueCount > 0;
\tTeam activeTeam = Team::None;

\tif (!contested) {
\tif (redCount > 0 && blueCount == 0)
\tactiveTeam = Team::Red;
\telse if (blueCount > 0 && redCount == 0)
\tactiveTeam = Team::Blue;
\t}

\tconst GameTime now = level.time;
\tGameTime delta = point.lastProgressTime ? (now - point.lastProgressTime) : 0_ms;
\tif (delta.milliseconds() < 0)
\tdelta = 0_ms;
\tpoint.lastProgressTime = now;

\tconst GameTime captureTime = DominationCaptureTime();
\tconst int64_t captureMs = captureTime.milliseconds();

\tconst auto decayProgress = [&](float amount) {
\tpoint.captureProgress = std::max(0.0f, point.captureProgress - amount);
\tif (point.captureProgress == 0.0f)
\tpoint.capturingTeam = Team::None;
\t};

\tif (captureMs <= 0) {
\tif (activeTeam != Team::None && activeTeam != point.owner)
\tFinalizeCapture(point, activeTeam);
\telse if (contested || activeTeam == Team::None)
\tpoint.capturingTeam = Team::None;
\treturn;
\t}

\tconst float deltaProgress = static_cast<float>(delta.milliseconds()) / static_cast<float>(captureMs);

\tif (contested) {
\tif (point.capturingTeam != Team::None && deltaProgress > 0.0f)
\tdecayProgress(deltaProgress);
\treturn;
\t}

\tif (activeTeam == Team::None) {
\tif (point.capturingTeam != Team::None && deltaProgress > 0.0f)
\tdecayProgress(deltaProgress);
\treturn;
\t}

\tif (point.owner == activeTeam) {
\tpoint.capturingTeam = Team::None;
\tpoint.captureProgress = 0.0f;
\treturn;
\t}

\tif (point.capturingTeam != activeTeam) {
\tpoint.capturingTeam = activeTeam;
\tpoint.captureProgress = 0.0f;
\t}

\tif (deltaProgress > 0.0f)
\tpoint.captureProgress = std::min(1.0f, point.captureProgress + deltaProgress);

\tif (point.captureProgress >= 1.0f)
\tFinalizeCapture(point, activeTeam);
}

/*
=============
Domination_PointTouch

Registers a player touching a domination point so capture logic can track occupancy.
=============
*/
TOUCH(Domination_PointTouch)(gentity_t* self, gentity_t* other, const trace_t&, bool) -> void {
\tif (!other->client)
\treturn;
\tif (!ClientIsPlaying(other->client) || other->client->eliminated)
\treturn;
\tif (Game::IsNot(GameType::Domination))
\treturn;

\tconst Team team = other->client->sess.team;
\tif (team != Team::Red && team != Team::Blue)
\treturn;

\tauto* point = FindPointForEntity(self);
\tif (!point)
\treturn;

\tif (!game.clients)
\treturn;

\tconst ptrdiff_t clientIndex = other->client - game.clients;
\tif (clientIndex < 0 || clientIndex >= game.maxClients)
\treturn;

\tpoint->occupantExpiry[static_cast<size_t>(clientIndex)] = level.time + DominationOccupantGrace();
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
Domination_ClearState

Resets domination state and frees transient entities.
=============
*/
void Domination_ClearState() {
\tfor (auto& point : level.domination.points) {
\tFreePointBeam(point);
\t}

\tlevel.domination = {};
}

/*
=============
Domination_InitLevel

Initializes domination state when a level loads.
=============
*/
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
auto& point = dom.points[i];
point.index = i;
point.capturingTeam = Team::None;
point.captureProgress = 0.0f;
point.lastProgressTime = level.time;
point.occupantCounts.fill(0);
point.occupantExpiry.fill(0_ms);
ApplyPointOwnerVisual(point);
}
}

/*
=============
Domination_RunFrame

Advances domination capture logic each frame and awards periodic scoring.
=============
*/
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

	const bool readyToScore = level.time >= dom.nextScoreTime;
	if (readyToScore)
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
point.capturingTeam = Team::None;
point.captureProgress = 0.0f;
point.lastProgressTime = level.time;
point.occupantCounts.fill(0);
point.occupantExpiry.fill(0_ms);
}
continue;
}

		UpdatePointOccupants(point);
		AdvanceCaptureProgress(point);

		if (point.owner == Team::Red)
		++redOwned;
		else if (point.owner == Team::Blue)
		++blueOwned;
	}

	if (!readyToScore)
	return;

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
