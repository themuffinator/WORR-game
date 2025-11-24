#include "server/bots/bot_utils.hpp"
#include "server/gameplay/g_capture.hpp"

#include <cassert>

GameLocals game{};
LevelLocals level{};
local_game_import_t gi{};

/*
===============
NoopConfigString

Stub configstring writer for tests.
===============
*/
static void NoopConfigString(int32_t, const char*) {
}

struct FlagEntityFixture {
	gentity_t entity{};
	gitem_t item{};
};

/*
===============
MakeFlagEntity

Builds a minimal flag entity for state updates.
===============
*/
static FlagEntityFixture MakeFlagEntity(item_id_t id) {
	FlagEntityFixture fixture{};
	fixture.item.id = id;
	fixture.entity.item = &fixture.item;
	fixture.entity.solid = SOLID_TRIGGER;
	fixture.entity.sv.init = true;
	return fixture;
}

/*
===============
main

Validates flag objective state bits for home, dropped, and carried flags.
===============
*/
int main() {
	gi.configString = NoopConfigString;

	SetFlagStatus(Team::Red, FlagStatus::AtBase);
	SetFlagStatus(Team::Blue, FlagStatus::AtBase);

	auto redFlag = MakeFlagEntity(IT_FLAG_RED);
	auto blueFlag = MakeFlagEntity(IT_FLAG_BLUE);

	Entity_UpdateState(&redFlag.entity);
	assert((redFlag.entity.sv.entFlags & SVFL_OBJECTIVE_AT_BASE) != 0);
	assert((redFlag.entity.sv.entFlags & SVFL_OBJECTIVE_TAKEN) == 0);
	assert((redFlag.entity.sv.entFlags & SVFL_OBJECTIVE_DROPPED) == 0);

	SetFlagStatus(Team::Red, FlagStatus::Dropped);
	Entity_UpdateState(&redFlag.entity);
	assert((redFlag.entity.sv.entFlags & SVFL_OBJECTIVE_DROPPED) != 0);
	assert((redFlag.entity.sv.entFlags & SVFL_OBJECTIVE_TAKEN) == 0);
	assert((redFlag.entity.sv.entFlags & SVFL_OBJECTIVE_AT_BASE) == 0);

	SetFlagStatus(Team::Red, FlagStatus::Taken);
	Entity_UpdateState(&redFlag.entity);
	assert((redFlag.entity.sv.entFlags & SVFL_OBJECTIVE_TAKEN) != 0);
	assert((redFlag.entity.sv.entFlags & SVFL_OBJECTIVE_DROPPED) == 0);
	assert((redFlag.entity.sv.entFlags & SVFL_OBJECTIVE_AT_BASE) == 0);

	SetFlagStatus(Team::Blue, FlagStatus::Taken);
	Entity_UpdateState(&blueFlag.entity);
	assert((blueFlag.entity.sv.entFlags & SVFL_OBJECTIVE_TAKEN) != 0);

	return 0;
}
