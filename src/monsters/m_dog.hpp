// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// Quake "dog" monster (ported for WOR)

#pragma once

// This file mirrors the ModelGen-style frame enum used by WOR monsters.
// Keep the order consistent with the model frame order.

enum {
	// attack
	FRAME_attack01,
	FRAME_attack02,
	FRAME_attack03,
	FRAME_attack04,
	FRAME_attack05,
	FRAME_attack06,
	FRAME_attack07,
	FRAME_attack08,

	// death set A (ends on corpse pose 1)
	FRAME_death01,
	FRAME_death02,
	FRAME_death03,
	FRAME_death04,
	FRAME_death05,
	FRAME_death06,
	FRAME_death07,
	FRAME_death08,
	FRAME_death09, // CORPSEFRAME_DOG_1

	// death set B (ends on corpse pose 2)
	FRAME_deathb01,
	FRAME_deathb02,
	FRAME_deathb03,
	FRAME_deathb04,
	FRAME_deathb05,
	FRAME_deathb06,
	FRAME_deathb07,
	FRAME_deathb08,
	FRAME_deathb09, // CORPSEFRAME_DOG_2

	// pain set A
	FRAME_pain01,
	FRAME_pain02,
	FRAME_pain03,
	FRAME_pain04,
	FRAME_pain05,
	FRAME_pain06,

	// pain set B (long)
	FRAME_painb01,
	FRAME_painb02,
	FRAME_painb03,
	FRAME_painb04,
	FRAME_painb05,
	FRAME_painb06,
	FRAME_painb07,
	FRAME_painb08,
	FRAME_painb09,
	FRAME_painb10,
	FRAME_painb11,
	FRAME_painb12,
	FRAME_painb13,
	FRAME_painb14,
	FRAME_painb15,
	FRAME_painb16,

	// run
	FRAME_run01,
	FRAME_run02,
	FRAME_run03,
	FRAME_run04,
	FRAME_run05,
	FRAME_run06,
	FRAME_run07,
	FRAME_run08,
	FRAME_run09,
	FRAME_run10,
	FRAME_run11,
	FRAME_run12,

	// leap
	FRAME_leap01,
	FRAME_leap02,
	FRAME_leap03,
	FRAME_leap04,
	FRAME_leap05,
	FRAME_leap06,
	FRAME_leap07,
	FRAME_leap08,
	FRAME_leap09,

	// stand
	FRAME_stand01,
	FRAME_stand02,
	FRAME_stand03,
	FRAME_stand04,
	FRAME_stand05,
	FRAME_stand06,
	FRAME_stand07,
	FRAME_stand08,
	FRAME_stand09,

	// walk
	FRAME_walk01,
	FRAME_walk02,
	FRAME_walk03,
	FRAME_walk04,
	FRAME_walk05,
	FRAME_walk06,
	FRAME_walk07,
	FRAME_walk08
};

constexpr float DOG_MODEL_SCALE = 1.0f;
