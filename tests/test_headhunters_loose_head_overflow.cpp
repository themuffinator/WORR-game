#include <cassert>
#include <cstddef>
#include <cmath>
#include <cstring>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

namespace std {
#if __cplusplus < 202302L
/*
=============
to_underlying

Backports std::to_underlying for compilers lacking C++23 support.
=============
*/
template<typename Enum>
constexpr std::underlying_type_t<Enum> to_underlying(Enum e) noexcept
{
	return static_cast<std::underlying_type_t<Enum>>(e);
}
#endif
using ::cosf;
using ::sinf;
}

std::mt19937 mt_rand{};

#include "server/gameplay/g_headhunters.cpp"

GameLocals game{};
LevelLocals level{};
game_export_t globals{};
local_game_import_t gi{};
cvar_t* g_gametype = nullptr;
gentity_t* g_entities = nullptr;

char local_game_import_t::print_buffer[0x10000];
std::array<char[MAX_INFO_STRING], MAX_LOCALIZATION_ARGS> local_game_import_t::buffers{};
std::array<const char*, MAX_LOCALIZATION_ARGS> local_game_import_t::buffer_ptrs{};

static spawn_temp_t g_spawnTemp{};
static std::vector<std::string> g_printLog{};
static constexpr size_t kEntityPoolSize = LevelLocals::HeadHuntersState::MAX_LOOSE_HEADS + 8;
static std::aligned_storage_t<sizeof(gentity_t), alignof(gentity_t)> g_entityPoolStorage[kEntityPoolSize]{};
static gentity_t* const g_entityPool = reinterpret_cast<gentity_t*>(g_entityPoolStorage);

/*
=============
ED_GetSpawnTemp

Returns stubbed spawn temp data for tests.
=============
*/
const spawn_temp_t& ED_GetSpawnTemp()
{
	return g_spawnTemp;
}

/*
=============
StubComPrint

Captures print output for verification.
=============
*/
static void StubComPrint(const char* message)
{
	g_printLog.emplace_back(message ? message : "");
}

/*
=============
StubComError

Captures error output to satisfy callbacks.
=============
*/
static void StubComError(const char* message)
{
	g_printLog.emplace_back(message ? message : "");
}

/*
=============
StubModelIndex
=============
*/
static int StubModelIndex(const char*)
{
	return 1;
}

/*
=============
StubSoundIndex
=============
*/
static int StubSoundIndex(const char*)
{
	return 1;
}

/*
=============
StubImageIndex
=============
*/
static int StubImageIndex(const char*)
{
	return 1;
}

/*
=============
StubSetModel
=============
*/
static void StubSetModel(gentity_t*, const char*)
{
}

/*
=============
StubLinkEntity
=============
*/
static void StubLinkEntity(gentity_t*)
{
}

/*
=============
StubUnlinkEntity
=============
*/
static void StubUnlinkEntity(gentity_t*)
{
}

/*
=============
StubSound
=============
*/
static void StubSound(gentity_t*, soundchan_t, int, float, float, float)
{
}

/*
=============
StubLocalSound
=============
*/
static void StubLocalSound(gentity_t*, gvec3_cptr_t, gentity_t*, soundchan_t, int, float, float, float, uint32_t)
{
}

/*
=============
StubLocPrint
=============
*/
static void StubLocPrint(gentity_t*, print_type_t, const char*, const char**, size_t)
{
}

/*
=============
StubBroadcastPrint
=============
*/
static void StubBroadcastPrint(print_type_t, const char*)
{
}

/*
=============
StubClientPrint
=============
*/
static void StubClientPrint(gentity_t*, print_type_t, const char*)
{
}

/*
=============
StubCenterPrint
=============
*/
static void StubCenterPrint(gentity_t*, const char*)
{
}

/*
=============
StubBotUnregister
=============
*/
static void StubBotUnregister(const gentity_t*)
{
}

/*
=============
InitImports

Initializes the imported game function table with stubs.
=============
*/
static void InitImports()
{
	game_import_t& base = gi;
	base.Broadcast_Print = &StubBroadcastPrint;
	base.Com_Print = &StubComPrint;
	base.Client_Print = &StubClientPrint;
	base.Center_Print = &StubCenterPrint;
	base.sound = &StubSound;
	base.localSound = &StubLocalSound;
	base.Com_Error = &StubComError;
	base.modelIndex = &StubModelIndex;
	base.soundIndex = &StubSoundIndex;
	base.imageIndex = &StubImageIndex;
	base.setModel = &StubSetModel;
	base.linkEntity = &StubLinkEntity;
	base.unlinkEntity = &StubUnlinkEntity;
	base.Loc_Print = &StubLocPrint;
	base.Bot_UnRegisterEntity = &StubBotUnregister;
}

/*
=============
save_data_list_t::save_data_list_t
=============
*/
save_data_list_t::save_data_list_t(const char* name_in, save_data_tag_t tag_in, const void* ptr_in) :
	name(name_in),
	tag(tag_in),
	ptr(ptr_in),
	next(nullptr)
{
}

/*
=============
save_data_list_t::fetch
=============
*/
const save_data_list_t* save_data_list_t::fetch(const void*, save_data_tag_t)
{
	return nullptr;
}

/*
=============
ClientIsPlaying
=============
*/
bool ClientIsPlaying(gclient_t*)
{
	return true;
}

/*
=============
ClientInObserverMode
=============
*/
bool ClientInObserverMode(gclient_t*)
{
	return false;
}

/*
=============
ScoringIsDisabled
=============
*/
bool ScoringIsDisabled()
{
	return false;
}

/*
=============
G_AdjustPlayerScore
=============
*/
void G_AdjustPlayerScore(gclient_t*, int, bool, int)
{
}

/*
=============
InitPool

Prepares the entity pool for allocation.
=============
*/
static void InitPool()
{
g_entities = g_entityPool;
globals.numEntities = kEntityPoolSize;
game.maxEntities = kEntityPoolSize;
game.maxClients = 0;
	for (size_t i = 0; i < kEntityPoolSize; ++i)
	{
		gentity_t* ent = g_entityPool + i;
		std::memset(ent, 0, sizeof(*ent));
		ent->s.number = static_cast<int>(i);
	}
}

/*
=============
Spawn
=============
*/
gentity_t* Spawn()
{
	for (size_t i = 0; i < kEntityPoolSize; ++i)
	{
		gentity_t* ent = g_entityPool + i;
		if (!ent->inUse)
		{
			std::memset(ent, 0, sizeof(*ent));
			ent->inUse = true;
			ent->s.number = static_cast<int>(i);
			return ent;
		}
	}
	return nullptr;
}

/*
=============
FreeEntity
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
MakeLooseHead

Allocates a simple loose head entity for tests.
=============
*/
static gentity_t* MakeLooseHead()
{
	gentity_t* ent = Spawn();
	assert(ent);
	ent->inUse = true;
	return ent;
}

/*
=============
main

Validates loose head overflow handling.
=============
*/
int main()
{
	InitImports();
	InitPool();
	level.headHunters = {};
	g_spawnTemp = {};
	g_printLog.clear();

	for (size_t i = 0; i < LevelLocals::HeadHuntersState::MAX_LOOSE_HEADS; ++i)
	{
		HeadHunters::RegisterLooseHead(MakeLooseHead());
	}

	size_t trackedCount = level.headHunters.looseHeadCount;
	assert(trackedCount == LevelLocals::HeadHuntersState::MAX_LOOSE_HEADS);
	gentity_t* overflow = MakeLooseHead();
	HeadHunters::RegisterLooseHead(overflow);
	assert(!overflow->inUse);
	assert(level.headHunters.looseHeadCount == LevelLocals::HeadHuntersState::MAX_LOOSE_HEADS);
	for (gentity_t* slot : level.headHunters.looseHeads)
	{
		assert(slot != overflow);
	}
	bool foundWarning = false;
	for (const auto& message : g_printLog)
	{
		if (message.find("overflow") != std::string::npos)
		{
			foundWarning = true;
			break;
		}
	}
	assert(foundWarning);
	return 0;
}
