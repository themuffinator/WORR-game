// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// models/monsters/fish

enum {
	// swim (idle/locomotion loop)
	FRAME_swim01,
	FRAME_swim02,
	FRAME_swim03,
	FRAME_swim04,
	FRAME_swim05,
	FRAME_swim06,
	FRAME_swim07,
	FRAME_swim08,

	// fast swim (run)
	FRAME_fswim01,
	FRAME_fswim02,
	FRAME_fswim03,
	FRAME_fswim04,
	FRAME_fswim05,
	FRAME_fswim06,
	FRAME_fswim07,
	FRAME_fswim08,

	// bite attack
	FRAME_bite01,
	FRAME_bite02,
	FRAME_bite03,
	FRAME_bite04,
	FRAME_bite05,
	FRAME_bite06,

	// pain short
	FRAME_pain01,
	FRAME_pain02,
	FRAME_pain03,
	FRAME_pain04,
	FRAME_pain05,

	// death
	FRAME_death01,
	FRAME_death02,
	FRAME_death03,
	FRAME_death04,
	FRAME_death05,
	FRAME_death06,
	FRAME_death07,
	FRAME_death08,
	FRAME_death09
};

constexpr float MODEL_SCALE = 1.000000f;
