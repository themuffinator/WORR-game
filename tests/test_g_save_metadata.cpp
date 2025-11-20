#include "../src/server/gameplay/g_save_metadata.hpp"

#include <cassert>
#include <string>

GameLocals game{};
LevelLocals level{};
game_export_t globals{};
spawn_temp_t st{};
std::mt19937 mt_rand{};
gentity_t* g_entities = nullptr;
local_game_import_t gi{};
cvar_t strict_cvar{};
cvar_t* g_strict_saves = &strict_cvar;

static std::string g_last_print;
static std::string g_last_error;

/*
=============
CapturePrint
=============
*/
static void CapturePrint(const char* msg)
{
	g_last_print = msg ? msg : "";
}

/*
=============
CaptureError
=============
*/
static void CaptureError(const char* msg)
{
	g_last_error = msg ? msg : "";
}

/*
=============
ResetLogs
=============
*/
static void ResetLogs()
{
	g_last_print.clear();
	g_last_error.clear();
}

/*
=============
main

Validates save metadata handling for version mismatches.
=============
*/
int main()
{
	gi.Com_Print = CapturePrint;
	gi.Com_Error = CaptureError;

	Json::Value valid(Json::objectValue);
	WriteSaveMetadata(valid);

	ResetLogs();
	strict_cvar.integer = 0;
	assert(ValidateSaveMetadata(valid, "game"));
	assert(valid["engine_version"].asString() == std::string(worr::version::kGameVersion));
	assert(g_last_print.empty());
	assert(g_last_error.empty());

	Json::Value badSave = valid;
	badSave["save_version"] = static_cast<Json::UInt64>(SAVE_FORMAT_VERSION + 1);
	ResetLogs();
	strict_cvar.integer = 0;
	assert(!ValidateSaveMetadata(badSave, "game"));
	assert(!g_last_print.empty());
	assert(g_last_error.empty());

	Json::Value badEngine = valid;
	badEngine["engine_version"] = "test-engine";
	ResetLogs();
	strict_cvar.integer = 1;
	assert(!ValidateSaveMetadata(badEngine, "level"));
	assert(g_last_error.find("engine version") != std::string::npos);

	return 0;
}
