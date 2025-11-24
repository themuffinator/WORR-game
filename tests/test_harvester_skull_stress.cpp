#include <array>
#include <cassert>
#include <cstring>

namespace std {
	using ::sinf;
}

#include "server/g_local.hpp"
#include "server/gameplay/g_harvester.hpp"

static cvar_t g_gametype_storage{};

std::mt19937 mt_rand{};
GameLocals game{};
LevelLocals level{};
game_export_t globals{};
gentity_t* g_entities = nullptr;
local_game_import_t gi{};
cvar_t* g_gametype = &g_gametype_storage;

static int clampLogCount = 0;
static int spawnCount = 0;

/*
=============
Teamplay_IsPrimaryTeam

Treats Red and Blue as valid primary teams.
=============
*/
bool Teamplay_IsPrimaryTeam(Team team)
{
	return team == Team::Red || team == Team::Blue;
}

/*
=============
Teams_TeamName

Provides a short team name for tests.
=============
*/
const char* Teams_TeamName(Team team)
{
	return team == Team::Red ? "red" : team == Team::Blue ? "blue" : "none";
}

/*
=============
Teams_OtherTeam

Returns the opposing primary team.
=============
*/
Team Teams_OtherTeam(Team team)
{
	return team == Team::Red ? Team::Blue : Team::Red;
}

/*
=============
Touch_Item

Stubbed item touch for skulls.
=============
*/
TOUCH(Touch_Item)(gentity_t*, gentity_t*, const trace_t&, bool)
{
}

/*
=============
Team_CaptureFlagSound

No-op capture sound for tests.
=============
*/
void Team_CaptureFlagSound(Team)
{
}

/*
=============
AwardFlagCapture

No-op capture award for tests.
=============
*/
void AwardFlagCapture(gentity_t*, gentity_t*, Team, GameTime)
{
}

/*
=============
SetFlagStatus

No-op flag status update for tests.
=============
*/
void SetFlagStatus(Team, FlagStatus)
{
}

/*
=============
CTF_ResetTeamFlag

No-op flag reset for tests.
=============
*/
void CTF_ResetTeamFlag(Team)
{
}

namespace HeadHunters {
	/*
	=============
	ApplyReceptacleVisuals

	No-op receptacle visuals for tests.
	=============
	*/
	void ApplyReceptacleVisuals(gentity_t*, Team)
	{
	}
}

/*
=============
GetItemByIndex

Returns the harvester skull item for tests.
=============
*/
Item* GetItemByIndex(item_id_t index)
{
	static Item skull{};
	static qboolean initialized = qfalse;
	if (!initialized) {
		skull.id = IT_HARVESTER_SKULL;
		skull.className = "harvester_skull";
		skull.worldModel = "skull.md3";
		initialized = qtrue;
	}

	return index == IT_HARVESTER_SKULL ? &skull : nullptr;
}

/*
=============
Spawn

Provides a cleared entity for tests.
=============
*/
gentity_t* Spawn()
{
	static std::array<gentity_t, 256> storage{};
	if (spawnCount >= static_cast<int>(storage.size()))
	return nullptr;

	gentity_t* ent = &storage[spawnCount++];
	std::memset(ent, 0, sizeof(gentity_t));
	ent->inUse = true;
	return ent;
}

/*
=============
FreeEntity

Marks the entity as freed for tests.
=============
*/
void FreeEntity(gentity_t* ent)
{
	if (!ent)
		return;
	ent->inUse = false;
}

/*
=============
crandom

Returns a deterministic random value for tests.
=============
*/
float crandom()
{
	return 0.25f;
}

/*
=============
frandom

Returns a deterministic random value for tests.
=============
*/
float frandom()
{
	return 0.5f;
}

/*
=============
G_AdjustPlayerScore

No-op scoring for tests.
=============
*/
void G_AdjustPlayerScore(gclient_t*, int, bool, int)
{
}

/*
=============
ScoringIsDisabled

Indicates scoring remains enabled.
=============
*/
bool ScoringIsDisabled()
{
	return false;
}

/*
=============
ClientIsPlaying

Treats connected, spawned clients as playing.
=============
*/
bool ClientIsPlaying(gclient_t* cl)
{
	return cl && cl->pers.connected && cl->pers.spawned;
}

/*
=============
ED_GetSpawnTemp

Returns an empty spawn temp for tests.
=============
*/
const spawn_temp_t& ED_GetSpawnTemp()
{
	static spawn_temp_t temp{};
	return temp;
}

/*
=============
TestTrace

Provides a flat trace to the fallback origin.
=============
*/
static trace_t TestTrace(const Vector3&, const Vector3&, const Vector3&, const Vector3& end, gentity_t*, int)
{
	trace_t tr{};
	tr.endPos = end;
	tr.startSolid = qfalse;
	return tr;
}

/*
=============
TestComPrintFmt

Tracks clamp log invocations.
=============
*/
static void TestComPrintFmt(const char* fmt, ...)
{
	if (fmt && std::strstr(fmt, "clamping"))
	++clampLogCount;
}

/*
=============
TestSetModel

No-op model setter for tests.
=============
*/
static void TestSetModel(gentity_t*, const char*)
{
}

/*
=============
TestLinkEntity

Marks entities as linked for tests.
=============
*/
static void TestLinkEntity(gentity_t* ent)
{
	if (!ent)
		return;
	ent->linked = true;
}

#include "server/gameplay/g_harvester.cpp"

/*
=============
main

Stress spawns skulls to verify clamping and pending bookkeeping.
=============
*/
int main()
{
	g_gametype_storage.integer = static_cast<int>(GameType::Harvester);
	globals.numEntities = 256;

	gi.trace = TestTrace;
	gi.Com_PrintFmt = TestComPrintFmt;
	gi.setModel = TestSetModel;
	gi.linkEntity = TestLinkEntity;

	const Vector3 fallback{ 0.0f, 0.0f, 0.0f };
	const int totalRequest = 64;

	const int firstWave = Harvester_DropSkulls(Team::Red, totalRequest, fallback, true);
	assert(firstWave == HARVESTER_MAX_SKULLS_PER_DROP);
	assert(level.harvester.pendingDrops.at(static_cast<size_t>(Team::Red)) == totalRequest - firstWave);
	assert(clampLogCount == 1);

	int totalSpawned = firstWave;
	while (level.harvester.pendingDrops.at(static_cast<size_t>(Team::Red)) > 0) {
		const int spawned = Harvester_DropSkulls(Team::Red, 0, fallback, true);
		assert(spawned <= HARVESTER_MAX_SKULLS_PER_DROP);
		totalSpawned += spawned;
	}

assert(totalSpawned == totalRequest);
return 0;
}
