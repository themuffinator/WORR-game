#pragma once

#include "../g_local.hpp"

/*
===============
Cthon Frames
===============
*/
enum {
	// idle/stand loop
	FRAME_idle01 = 0,
	FRAME_idle02,
	FRAME_idle03,
	FRAME_idle04,
	FRAME_idle05,
	FRAME_idle06,
	FRAME_idle07,
	FRAME_idle08,

	// attack windup/hold
	FRAME_attack01,
	FRAME_attack02,
	FRAME_attack03,
	FRAME_attack04,
	FRAME_attack05,
	FRAME_attack06,

	// pain
	FRAME_pain01,
	FRAME_pain02,
	FRAME_pain03,
	FRAME_pain04,

	// death
	FRAME_death01,
	FRAME_death02,
	FRAME_death03,
	FRAME_death04,
	FRAME_death05,
	FRAME_death06,
	FRAME_death07,
	FRAME_death08
};

// Script helper: strikes Chthon with lightning and briefly makes him vulnerable.
// Keys:
// - target: targetName of the monster_chthon
// - dmg: damage to apply immediately (default 200)
// - wait: vulnerability duration seconds (default 1.5)
void SP_target_chthon_lightning(gentity_t* self);
