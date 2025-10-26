// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

// g_teamplay.cpp (Game Teamplay)
// This file contains the core logic for all team-based game modes, with a
// primary focus on Capture the Flag (CTF). It manages flag state, player
// interactions with flags, and team scoring bonuses.
//
// Key Responsibilities:
// - CTF Flag Management: Handles the entire lifecycle of the CTF flags,
//   including spawning (`CTF_FlagSetup`), pickup (`CTF_PickupFlag`), drop
//   (`CTF_DropFlag`), and automatic return logic.
// - Scoring and Bonuses: Implements the scoring rules for CTF, such as awarding
//   points for captures, flag recoveries, carrier protection, and fragging the
//   enemy carrier.
// - Player State: Manages CTF-specific player state, such as which flag a
//   player is carrying and applying visual effects (auras, icons) to flag
//   carriers.
// - Team-Based Checks: Provides utility functions for team-based logic, such
//   as checking if a player has hurt their own flag carrier.

#include "g_local.hpp"

namespace CTF {
	// Capture the Flag scoring bonuses
	inline constexpr int	CAPTURE_BONUS = 100;	// Player bonus for capturing the flag
	inline constexpr int	TEAM_BONUS = 25;	// Team bonus for a capture
	inline constexpr int	RECOVERY_BONUS = 10;	// Bonus for returning your flag
	inline constexpr int	FLAG_BONUS = 10;	// Bonus for picking up the enemy flag
	inline constexpr int	FRAG_CARRIER_BONUS = 20;	// Bonus for fragging the enemy flag carrier

	// Automatic flag return time (milliseconds)
	inline constexpr GameTime	FLAG_RETURN_TIME = 40_sec;

	// Flag defense and assist bonuses
	inline constexpr int	CARRIER_DANGER_PROTECT_BONUS = 5;	// Bonus for killing someone who recently damaged your flag carrier
	inline constexpr int	CARRIER_PROTECT_BONUS = 2;	// Bonus for killing someone near your flag carrier
	inline constexpr int	FLAG_DEFENSE_BONUS = 10;	// Bonus for defending the flag
	inline constexpr int	RETURN_FLAG_ASSIST_BONUS = 10;	// Bonus for returning a flag that leads to a quick capture
	inline constexpr int	FRAG_CARRIER_ASSIST_BONUS = 10;	// Bonus for fragging carrier leading to a capture

	// Radius within which protection bonuses apply (game units)
	inline constexpr float	TARGET_PROTECT_RADIUS = 1000;
	inline constexpr float	ATTACKER_PROTECT_RADIUS = 1000;

	// Timeouts for assist tracking (milliseconds)
	inline constexpr GameTime	CARRIER_DANGER_PROTECT_TIMEOUT = 8_sec;
	inline constexpr GameTime	FRAG_CARRIER_ASSIST_TIMEOUT = 10_sec;
	inline constexpr GameTime	RETURN_FLAG_ASSIST_TIMEOUT = 10_sec;

	inline constexpr GameTime AUTO_FLAG_RETURN_TIMEOUT = 30_sec; // number of seconds before dropped flag auto-returns
}

constexpr int32_t CTF_CAPTURE_BONUS = 15;	  // what you get for capture
constexpr int32_t CTF_TEAM_BONUS = 10;   // what your team gets for capture
constexpr int32_t CTF_RECOVERY_BONUS = 1;	  // what you get for recovery
constexpr int32_t CTF_FLAG_BONUS = 0;   // what you get for picking up enemy flag
constexpr int32_t CTF_FRAG_CARRIER_BONUS = 2; // what you get for fragging enemy flag carrier
constexpr GameTime CTF_FLAG_RETURN_TIME = 40_sec;  // seconds until auto return

constexpr int32_t CTF_CARRIER_DANGER_PROTECT_BONUS = 2; // bonus for fraggin someone who has recently hurt your flag carrier
constexpr int32_t CTF_CARRIER_PROTECT_BONUS = 1; // bonus for fraggin someone while either you or your target are near your flag carrier
constexpr int32_t CTF_FLAG_DEFENSE_BONUS = 1; 	// bonus for fraggin someone while either you or your target are near your flag
constexpr int32_t CTF_RETURN_FLAG_ASSIST_BONUS = 1; // awarded for returning a flag that causes a capture to happen almost immediately
constexpr int32_t CTF_FRAG_CARRIER_ASSIST_BONUS = 2;	// award for fragging a flag carrier if a capture happens almost immediately

constexpr float CTF_TARGET_PROTECT_RADIUS = 400;   // the radius around an object being defended where a target will be worth extra frags
constexpr float CTF_ATTACKER_PROTECT_RADIUS = 400; // the radius around an object being defended where an attacker will get extra frags when making kills

constexpr GameTime CTF_CARRIER_DANGER_PROTECT_TIMEOUT = 8_sec;
constexpr GameTime CTF_FRAG_CARRIER_ASSIST_TIMEOUT = 10_sec;
constexpr GameTime CTF_RETURN_FLAG_ASSIST_TIMEOUT = 10_sec;

constexpr GameTime CTF_AUTO_FLAG_RETURN_TIMEOUT = 30_sec; // number of seconds before dropped flag auto-returns

/*
=================
FlagStatus

Represents the status of a flag in CTF and One Flag CTF modes.
=================
*/
enum class FlagStatus : int {
	Invalid = -1,
	AtBase = 0,
	Taken = 1,		// CTF
	TakenRed = 2,	// One Flag CTF
	TakenBlue = 3,	// One Flag CTF
	Dropped = 4
};

/*
=================
TeamGame

Holds team-based gameplay state for CTF and One Flag CTF modes.
=================
*/
struct TeamGame {
	GameTime		lastFlagCaptureTime = 0_sec;
	Team		lastFlagCaptureTeam = Team::None;

	FlagStatus	redFlagStatus = FlagStatus::AtBase;
	FlagStatus	blueFlagStatus = FlagStatus::AtBase;
	FlagStatus	neutralFlagStatus = FlagStatus::AtBase;

	GameTime		redTakenTime = 0_sec;
	GameTime		blueTakenTime = 0_sec;
	GameTime		redObeliskAttackedTime = 0_sec;
	GameTime		blueObeliskAttackedTime = 0_sec;
};

// Global instance
TeamGame teamGame;

gentity_t* neutralObelisk;

static void Team_SetFlagStatus(Team team, FlagStatus status);

/*
================
Team_ReturnFlagSound

Plays a global sound when a flag is returned to base.
================
*/
static void Team_ReturnFlagSound(Team team) {
	// vo/enemy_flag_returned.wav
	// vo/your_flag_returned.wav
}

/*
================
Team_TakeFlagSound

Plays a global sound when a flag is taken from the base.
Only plays if the flag was previously at base or not recently taken.
================
*/
static void Team_TakeFlagSound(Team team) {

	switch (team) {
	case Team::Red:
		if (teamGame.blueFlagStatus != FlagStatus::AtBase &&
			teamGame.blueTakenTime > level.time - 5_sec) {
			return;
		}
		teamGame.blueTakenTime = level.time;
		break;

	case Team::Blue:
		if (teamGame.redFlagStatus != FlagStatus::AtBase &&
			teamGame.redTakenTime > level.time - 5_sec) {
			return;
		}
		teamGame.redTakenTime = level.time;
		break;

	default:
		return;
	}

	// vo/enemy_flag_taken.wav
	// vo/your_flag_taken.wav
}

/*
================
Team_CaptureFlagSound

Plays a global sound when a team captures the flag.
================
*/
static void Team_CaptureFlagSound(Team team) {
	// vo/enemy_flag_scores.wav
	// vo/your_flag_scores.wav
}

bool CTF_ResetTeamFlag(Team team);
/*
================
Team_ReturnFlag

Resets the team's flag and broadcasts the return message.
================
*/
void Team_ReturnFlag(Team team) {
	if (!CTF_ResetTeamFlag(team))
		return;

	Team_ReturnFlagSound(team);

	if (team == Team::Free) {
		gi.Broadcast_Print(PRINT_HIGH, "The flag has returned!\n");
	}
	else {
		gi.LocBroadcast_Print(PRINT_HIGH, "The {} flag has returned!\n", Teams_TeamName(team));
	}
}

/*
=================
Team_SetFlagStatus

Updates internal flag status and sends a configstring update to clients
when the status changes. Used in CTF and One Flag CTF.
=================
*/
static void Team_SetFlagStatus(Team team, FlagStatus status) {
	static constexpr std::array<char, 5> ctfFlagStatusRemap = { '0', '1', '*', '*', '2' };
	static constexpr std::array<char, 5> oneFlagStatusRemap = { '0', '1', '2', '3', '4' };

	bool modified = false;

	switch (team) {
	case Team::Red:
		if (teamGame.redFlagStatus != status) {
			teamGame.redFlagStatus = status;
			modified = true;
		}
		break;

	case Team::Blue:
		if (teamGame.blueFlagStatus != status) {
			teamGame.blueFlagStatus = status;
			modified = true;
		}
		break;

	case Team::Free:
		if (teamGame.neutralFlagStatus != status) {
			teamGame.neutralFlagStatus = status;
			modified = true;
		}
		break;

	default:
		return;
	}

	if (!modified)
		return;

	std::string flagStatusStr;

	if (g_gametype->integer == static_cast<int>(GameType::CaptureTheFlag)) {
		flagStatusStr += ctfFlagStatusRemap.at(static_cast<int>(teamGame.redFlagStatus));
		flagStatusStr += ctfFlagStatusRemap.at(static_cast<int>(teamGame.blueFlagStatus));
	}
	else {
		flagStatusStr += oneFlagStatusRemap.at(static_cast<int>(teamGame.neutralFlagStatus));
	}

	//trap_SetConfigstring(CS_FLAGSTATUS, flagStatusStr);
}

void Team_CheckDroppedItem(gentity_t* dropped) {
	if (dropped->item->id == IT_FLAG_RED) {
		Team_SetFlagStatus(Team::Red, FlagStatus::Dropped);
	}
	else if (dropped->item->id == IT_FLAG_BLUE) {
		Team_SetFlagStatus(Team::Blue, FlagStatus::Dropped);
	}
	else if (dropped->item->id == IT_FLAG_NEUTRAL) {
		Team_SetFlagStatus(Team::Free, FlagStatus::Dropped);
	}
}

/*
============
CTF_ScoreBonuses

Calculate the bonuses for flag defense, flag carrier defense, etc.
Note that bonuses are not cumaltive.  You get one, they are in importance
order.
============
*/
void CTF_ScoreBonuses(gentity_t* targ, gentity_t* inflictor, gentity_t* attacker) {
	item_id_t	flag_item, enemy_flag_item;
	Team		otherTeam = Team::None;
	gentity_t* flag, * carrier = nullptr;
	const char* c;
	Vector3		v1, v2;

	if (!Game::Has(GameFlags::CTF))
		return;

	// no bonus for fragging yourself
	if (!targ->client || !attacker->client || targ == attacker)
		return;

	otherTeam = Teams_OtherTeam(targ->client->sess.team);
	if (otherTeam < Team::None)
		return; // whoever died isn't on a team

	// same team, if the flag at base, check to he has the enemy flag
	if (targ->client->sess.team == Team::Red) {
		flag_item = IT_FLAG_RED;
		enemy_flag_item = IT_FLAG_BLUE;
	}
	else {
		flag_item = IT_FLAG_BLUE;
		enemy_flag_item = IT_FLAG_RED;
	}

	// did the attacker frag the flag carrier?
	if (targ->client->pers.inventory[enemy_flag_item]) {
		attacker->client->resp.ctf_lastfraggedcarrier = level.time;
		G_AdjustPlayerScore(attacker->client, CTF_FRAG_CARRIER_BONUS, false, 0);
		gi.LocBroadcast_Print(PRINT_MEDIUM, "{} fragged {}'s flag carrier!",
			attacker->client->sess.netName,
			Teams_TeamName(targ->client->sess.team));

		// the target had the flag, clear the hurt carrier
		// field on the other team
		for (auto ec : active_clients()) {
			if (ec->inUse && ec->client->sess.team == otherTeam)
				ec->client->resp.ctf_lasthurtcarrier = 0_ms;
		}
		return;
	}

	if (targ->client->resp.ctf_lasthurtcarrier &&
		level.time - targ->client->resp.ctf_lasthurtcarrier < CTF_CARRIER_DANGER_PROTECT_TIMEOUT &&
		!attacker->client->pers.inventory[flag_item]) {
		// attacker is on the same team as the flag carrier and
		// fragged a guy who hurt our flag carrier
		G_AdjustPlayerScore(attacker->client, CTF_CARRIER_DANGER_PROTECT_BONUS, false, 0);
		PushAward(attacker, PlayerMedal::Defence);
		return;
	}

	// flag and flag carrier area defense bonuses

	// we have to find the flag and carrier entities

	// find the flag
	switch (attacker->client->sess.team) {
	case Team::Red:
		c = ITEM_CTF_FLAG_RED;
		break;
	case Team::Blue:
		c = ITEM_CTF_FLAG_BLUE;
		break;
	default:
		return;
	}

	flag = nullptr;
	while ((flag = G_FindByString<&gentity_t::className>(flag, c)) != nullptr) {
		if (!(flag->spawnFlags & SPAWNFLAG_ITEM_DROPPED))
			break;
	}

	if (!flag)
		return; // can't find attacker's flag

	// find attacker's team's flag carrier
	for (auto ec : active_clients()) {
		if (ec->client->pers.inventory[flag_item]) {
			carrier = ec;
			break;
		}
	}

	// ok we have the attackers flag and a pointer to the carrier

	// check to see if we are defending the base's flag
	v1 = targ->s.origin - flag->s.origin;
	v2 = attacker->s.origin - flag->s.origin;

	if ((v1.length() < CTF_TARGET_PROTECT_RADIUS ||
		v2.length() < CTF_TARGET_PROTECT_RADIUS ||
		LocCanSee(flag, targ) || LocCanSee(flag, attacker)) &&
		attacker->client->sess.team != targ->client->sess.team) {
		// we defended the base flag
		G_AdjustPlayerScore(attacker->client, CTF_FLAG_DEFENSE_BONUS, false, 0);
		PushAward(attacker, PlayerMedal::Defence);
		return;
	}

	if (carrier && carrier != attacker) {
		v1 = targ->s.origin - carrier->s.origin;
		v2 = attacker->s.origin - carrier->s.origin;

		if (v1.length() < CTF_ATTACKER_PROTECT_RADIUS ||
			v2.length() < CTF_ATTACKER_PROTECT_RADIUS ||
			LocCanSee(carrier, targ) || LocCanSee(carrier, attacker)) {
			G_AdjustPlayerScore(attacker->client, CTF_CARRIER_PROTECT_BONUS, false, 0);
			return;
		}
	}
}

/*
============
CTF_CheckHurtCarrier
============
*/
void CTF_CheckHurtCarrier(gentity_t* targ, gentity_t* attacker) {
	if (!Game::Has(GameFlags::CTF))
		return;

	if (!targ->client || !attacker->client)
		return;

	item_id_t flag_item = targ->client->sess.team == Team::Red ? IT_FLAG_BLUE : IT_FLAG_RED;

	if (targ->client->pers.inventory[flag_item] &&
		targ->client->sess.team != attacker->client->sess.team)
		attacker->client->resp.ctf_lasthurtcarrier = level.time;
}

/*
============
CTF_ResetTeamFlag
============
*/
bool CTF_ResetTeamFlag(Team team) {
	if (!Game::Has(GameFlags::CTF))
		return false;

	gentity_t* ent;
	const char* c = team == Team::Red ? ITEM_CTF_FLAG_RED : ITEM_CTF_FLAG_BLUE;
	bool found = false;

	ent = nullptr;
	while ((ent = G_FindByString<&gentity_t::className>(ent, c)) != nullptr) {
		if (ent->spawnFlags.has(SPAWNFLAG_ITEM_DROPPED)) {
			FreeEntity(ent);
			found = true;
		}
		else {
			ent->svFlags &= ~SVF_NOCLIENT;
			ent->solid = SOLID_TRIGGER;
			gi.linkEntity(ent);
			ent->s.event = EV_ITEM_RESPAWN;
			found = true;
		}
	}

	return found;
}

/*
============
CTF_ResetFlags
============
*/
void CTF_ResetFlags() {
	if (!Game::Has(GameFlags::CTF))
		return;

	CTF_ResetTeamFlag(Team::Red);
	CTF_ResetTeamFlag(Team::Blue);
}

/*
============
CTF_PickupFlag
============
*/
bool CTF_PickupFlag(gentity_t* ent, gentity_t* other) {
	if (!Game::Has(GameFlags::CTF))
		return false;

	Team		team;
	item_id_t	flag_item, enemy_flag_item;

	// figure out what team this flag is
	if (ent->item->id == IT_FLAG_RED)
		team = Team::Red;
	else if (ent->item->id == IT_FLAG_BLUE)
		team = Team::Blue;
	else {
		gi.LocClient_Print(other, PRINT_HIGH, "Don't know what team the flag is on, removing.\n");
		FreeEntity(ent);
		return false;
	}

	// same team, if the flag at base, check to he has the enemy flag
	if (team == Team::Red) {
		flag_item = IT_FLAG_RED;
		enemy_flag_item = IT_FLAG_BLUE;
	}
	else {
		flag_item = IT_FLAG_BLUE;
		enemy_flag_item = IT_FLAG_RED;
	}

	if (team == other->client->sess.team) {

		if (!(ent->spawnFlags & SPAWNFLAG_ITEM_DROPPED)) {
			// the flag is at home base.  if the player has the enemy
			// flag, he's just scored a capture!

			if (other->client->pers.inventory[enemy_flag_item]) {
				if (other->client->resp.teamState.flag_pickup_time) {
					gi.LocBroadcast_Print(PRINT_HIGH, "{} TEAM CAPTURED the flag! ({} captured in {})\n",
						Teams_TeamName(team), other->client->sess.netName, TimeString((level.time - other->client->resp.teamState.flag_pickup_time).milliseconds(), true, false));
				}
				else {
					gi.LocBroadcast_Print(PRINT_HIGH, "{} TEAM CAPTURED the flag! (captured by {})\n",
						Teams_TeamName(team), other->client->sess.netName);
				}
				other->client->pers.inventory[enemy_flag_item] = 0;

				level.ctf_last_flag_capture = level.time;
				level.ctf_last_capture_team = team;
				G_AdjustTeamScore(team, Game::Is(GameType::CaptureStrike) ? 2 : 1);

				gi.sound(ent, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundIndex("ctf/flagcap.wav"), 1, ATTN_NONE, 0);

				// other gets capture bonus
				G_AdjustPlayerScore(other->client, CTF_CAPTURE_BONUS, false, 0);
				PushAward(other, PlayerMedal::Captures);

				// Ok, let's do the player loop, hand out the bonuses
				for (auto ec : active_clients()) {
					if (ec->client->sess.team != other->client->sess.team)
						ec->client->resp.ctf_lasthurtcarrier = -5_sec;
					else if (ec->client->sess.team == other->client->sess.team) {
						if (ec != other)
							G_AdjustPlayerScore(ec->client, CTF_TEAM_BONUS, false, 0);
						// award extra points for capture assists
						if (ec->client->resp.ctf_lastreturnedflag && ec->client->resp.ctf_lastreturnedflag + CTF_RETURN_FLAG_ASSIST_TIMEOUT > level.time) {
							gi.LocBroadcast_Print(PRINT_HIGH, "$g_bonus_assist_return", ec->client->sess.netName);
							G_AdjustPlayerScore(ec->client, CTF_RETURN_FLAG_ASSIST_BONUS, false, 0);
							PushAward(ec, PlayerMedal::Assist);
						}
						if (ec->client->resp.ctf_lastfraggedcarrier && ec->client->resp.ctf_lastfraggedcarrier + CTF_FRAG_CARRIER_ASSIST_TIMEOUT > level.time) {
							gi.LocBroadcast_Print(PRINT_HIGH, "$g_bonus_assist_frag_carrier", ec->client->sess.netName);
							G_AdjustPlayerScore(ec->client, CTF_FRAG_CARRIER_ASSIST_BONUS, false, 0);
							PushAward(ec, PlayerMedal::Assist);
						}
					}
				}

				CTF_ResetFlags();

				if (Game::Is(GameType::CaptureStrike)) {
					gi.LocBroadcast_Print(PRINT_CENTER, "Flag captured!\n{} wins the round!\n", Teams_TeamName(team));
					Round_End();
				}

				return false;
			}
			return false; // its at home base already
		}
		// hey, its not home.  return it by teleporting it back
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_returned_flag",
			other->client->sess.netName, Teams_TeamName(team));
		G_AdjustPlayerScore(other->client, CTF_RECOVERY_BONUS, false, 0);
		other->client->resp.ctf_lastreturnedflag = level.time;
		gi.sound(ent, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundIndex("ctf/flagret.wav"), 1, ATTN_NONE, 0);
		// CTF_ResetTeamFlag will remove this entity!  We must return false
		CTF_ResetTeamFlag((Team)team);
		return false;
	}

	// capturestrike: can't pick up enemy flag if defending
	if (Game::Is(GameType::CaptureStrike)) {
		if ((level.strike_red_attacks && other->client->sess.team != Team::Red) ||
			(!level.strike_red_attacks && other->client->sess.team != Team::Blue)) {
			//gi.LocClient_Print(other, PRINT_CENTER, "Your team is defending!\n");
			return false;
		}
	}

	// hey, its not our flag, pick it up
	if (!(ent->spawnFlags & SPAWNFLAG_ITEM_DROPPED)) {
		other->client->resp.teamState.flag_pickup_time = level.time;
	}
	gi.LocBroadcast_Print(PRINT_HIGH, "$g_got_flag",
		other->client->sess.netName, Teams_TeamName(team));
	G_AdjustPlayerScore(other->client, CTF_FLAG_BONUS, false, 0);
	if (!level.strike_flag_touch) {
		G_AdjustTeamScore(other->client->sess.team, 1);
		level.strike_flag_touch = true;
	}

	other->client->pers.inventory[flag_item] = 1;
	other->client->resp.ctf_flagsince = level.time;


	// pick up the flag
	// if it's not a dropped flag, we just make is disappear
	// if it's dropped, it will be removed by the pickup caller
	if (!(ent->spawnFlags & SPAWNFLAG_ITEM_DROPPED)) {
		ent->flags |= FL_RESPAWN;
		ent->svFlags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
	}
	return true;
}

/*
============
CTF_DropFlagTouch
============
*/
static TOUCH(CTF_DropFlagTouch) (gentity_t* ent, gentity_t* other, const trace_t& tr, bool otherTouchingSelf) -> void {
	if (!Game::Has(GameFlags::CTF))
		return;

	// owner (who dropped us) can't touch for two secs
	if (other == ent->owner &&
		ent->nextThink - level.time > CTF_AUTO_FLAG_RETURN_TIMEOUT - 2_sec)
		return;

	Touch_Item(ent, other, tr, otherTouchingSelf);
}

/*
============
CTF_DropFlagThink
============
*/
static THINK(CTF_DropFlagThink) (gentity_t* ent) -> void {
	if (!Game::Has(GameFlags::CTF))
		return;

	// auto return the flag
	// reset flag will remove ourselves
	if (ent->item->id == IT_FLAG_RED) {
		CTF_ResetTeamFlag(Team::Red);
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_flag_returned",
			Teams_TeamName(Team::Red));
	}
	else if (ent->item->id == IT_FLAG_BLUE) {
		CTF_ResetTeamFlag(Team::Blue);
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_flag_returned",
			Teams_TeamName(Team::Blue));
	}

	gi.sound(ent, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundIndex("ctf/flagret.wav"), 1, ATTN_NONE, 0);
}

/*
============
CTF_DeadDropFlag

Called from PlayerDie, to drop the flag from a dying player
============
*/
void CTF_DeadDropFlag(gentity_t* self) {
	if (!Game::Has(GameFlags::CTF))
		return;

	gentity_t* dropped = nullptr;

	if (self->client->pers.inventory[IT_FLAG_RED]) {
		dropped = Drop_Item(self, GetItemByIndex(IT_FLAG_RED));
		self->client->pers.inventory[IT_FLAG_RED] = 0;
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_lost_flag",
			self->client->sess.netName, Teams_TeamName(Team::Red));
	}
	else if (self->client->pers.inventory[IT_FLAG_BLUE]) {
		dropped = Drop_Item(self, GetItemByIndex(IT_FLAG_BLUE));
		self->client->pers.inventory[IT_FLAG_BLUE] = 0;
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_lost_flag",
			self->client->sess.netName, Teams_TeamName(Team::Blue));
	}

	self->client->resp.teamState.flag_pickup_time = 0_ms;

	if (dropped) {
		dropped->think = CTF_DropFlagThink;
		dropped->nextThink = level.time + CTF_AUTO_FLAG_RETURN_TIMEOUT;
		dropped->touch = CTF_DropFlagTouch;
	}
}

/*
============
CTF_DropFlag
============
*/
void CTF_DropFlag(gentity_t* ent, Item* item) {
	if (!Game::Has(GameFlags::CTF))
		return;

	ent->client->resp.teamState.flag_pickup_time = 0_ms;

	if (brandom())
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_lusers_drop_flags");
	else
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_winners_drop_flags");
}

/*
============
CTF_FlagThink
============
*/
static THINK(CTF_FlagThink) (gentity_t* ent) -> void {
	if (!Game::Has(GameFlags::CTF))
		return;

	if (ent->solid != SOLID_NOT)
		ent->s.frame = 173 + (((ent->s.frame - 173) + 1) % 16);
	ent->nextThink = level.time + 10_hz;
}

/*
============
CTF_FlagSetup
============
*/
THINK(CTF_FlagSetup) (gentity_t* ent) -> void {
	if (!Game::Has(GameFlags::CTF))
		return;

	trace_t tr;
	Vector3	dest;

	ent->mins = { -15, -15, -15 };
	ent->maxs = { 15, 15, 15 };

	if (ent->model)
		gi.setModel(ent, ent->model);
	else
		gi.setModel(ent, ent->item->worldModel);
	ent->solid = SOLID_TRIGGER;
	ent->moveType = MoveType::Toss;
	ent->touch = Touch_Item;
	ent->s.frame = 173;

	dest = ent->s.origin + Vector3{ 0, 0, -128 };

	tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, dest, ent, MASK_SOLID);
	if (tr.startSolid) {
		gi.Com_PrintFmt("{}: {} startSolid\n", __FUNCTION__, *ent);
		FreeEntity(ent);
		return;
	}

	ent->s.origin = tr.endPos;

	gi.linkEntity(ent);

	ent->nextThink = level.time + 10_hz;
	ent->think = CTF_FlagThink;
}

/*
============
CTF_ClientEffects
============
*/
void CTF_ClientEffects(gentity_t* player) {
	if (!Game::Has(GameFlags::CTF))
		return;

	player->s.effects &= ~(EF_FLAG_RED | EF_FLAG_BLUE);
	if (player->health > 0) {
		if (player->client->pers.inventory[IT_FLAG_RED])
			player->s.effects |= EF_FLAG_RED;
		if (player->client->pers.inventory[IT_FLAG_BLUE])
			player->s.effects |= EF_FLAG_BLUE;
	}

	if (player->client->pers.inventory[IT_FLAG_RED])
		player->s.modelIndex3 = mi_ctf_red_flag;
	else if (player->client->pers.inventory[IT_FLAG_BLUE])
		player->s.modelIndex3 = mi_ctf_blue_flag;
	else
		player->s.modelIndex3 = 0;
}
