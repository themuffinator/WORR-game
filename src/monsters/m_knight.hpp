
/*
===============
Knight enums
===============
*/

enum {
	// stand
	FRAME_stand1, FRAME_stand2, FRAME_stand3, FRAME_stand4, FRAME_stand5, FRAME_stand6, FRAME_stand7, FRAME_stand8, FRAME_stand9,

	// walk 1..16
	FRAME_walk1, FRAME_walk2, FRAME_walk3, FRAME_walk4, FRAME_walk5, FRAME_walk6, FRAME_walk7, FRAME_walk8,
	FRAME_walk9, FRAME_walk10, FRAME_walk11, FRAME_walk12, FRAME_walk13, FRAME_walk14, FRAME_walk15, FRAME_walk16,

	// run 1..8
	FRAME_run1, FRAME_run2, FRAME_run3, FRAME_run4, FRAME_run5, FRAME_run6, FRAME_run7, FRAME_run8,

	// attackb1..attackb10 (melee combo)
	FRAME_attackb1, FRAME_attackb2, FRAME_attackb3, FRAME_attackb4, FRAME_attackb5, FRAME_attackb6, FRAME_attackb7, FRAME_attackb8, FRAME_attackb9, FRAME_attackb10,

	// pain groups
	FRAME_paina1, FRAME_paina2, FRAME_paina3, FRAME_paina4, FRAME_paina5,
	FRAME_painb1, FRAME_painb2, FRAME_painb3, FRAME_painb4, FRAME_painb5,
	FRAME_painc1, FRAME_painc2, FRAME_painc3, FRAME_painc4, FRAME_painc5, FRAME_painc6, FRAME_painc7, FRAME_painc8,
	FRAME_paind1, FRAME_paind2, FRAME_paind3, FRAME_paind4, FRAME_paind5, FRAME_paind6, FRAME_paind7, FRAME_paind8,
	FRAME_paind9, FRAME_paind10, FRAME_paind11, FRAME_paind12, FRAME_paind13, FRAME_paind14, FRAME_paind15, FRAME_paind16,
	FRAME_paind17, FRAME_paind18, FRAME_paind19,

	// death1..death10 then corpse hold
	FRAME_death1, FRAME_death2, FRAME_death3, FRAME_death4, FRAME_death5, FRAME_death6, FRAME_death7, FRAME_death8, FRAME_death9, FRAME_death10
};

void SP_monster_knight(gentity_t *self);
