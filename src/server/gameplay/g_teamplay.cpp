// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

// Modernized teamplay implementation covering Capture the Flag, One Flag CTF and
// Harvester logic.

#include "../g_local.hpp"
#include "g_teamplay.hpp"
#include "g_headhunters.hpp"
#include "g_harvester.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace CTF {
	inline constexpr int CAPTURE_BONUS = 100;
	inline constexpr int TEAM_BONUS = 25;
	inline constexpr int RECOVERY_BONUS = 10;
	inline constexpr int FLAG_BONUS = 10;
	inline constexpr int FRAG_CARRIER_BONUS = 20;

	inline constexpr GameTime FLAG_RETURN_TIME = 40_sec;

	inline constexpr int CARRIER_DANGER_PROTECT_BONUS = 5;
	inline constexpr int CARRIER_PROTECT_BONUS = 2;
	inline constexpr int FLAG_DEFENSE_BONUS = 10;
	inline constexpr int RETURN_FLAG_ASSIST_BONUS = 10;
	inline constexpr int FRAG_CARRIER_ASSIST_BONUS = 10;

	inline constexpr float TARGET_PROTECT_RADIUS = 1000.0f;
	inline constexpr float ATTACKER_PROTECT_RADIUS = 1000.0f;

	inline constexpr GameTime CARRIER_DANGER_PROTECT_TIMEOUT = 8_sec;
	inline constexpr GameTime FRAG_CARRIER_ASSIST_TIMEOUT = 10_sec;
	inline constexpr GameTime RETURN_FLAG_ASSIST_TIMEOUT = 10_sec;

	inline constexpr GameTime AUTO_FLAG_RETURN_TIMEOUT = 30_sec;
}

struct TeamGame {
	GameTime lastFlagCaptureTime = 0_sec;
	Team lastFlagCaptureTeam = Team::None;

	FlagStatus redFlagStatus = FlagStatus::AtBase;
	FlagStatus blueFlagStatus = FlagStatus::AtBase;
	FlagStatus neutralFlagStatus = FlagStatus::AtBase;

	GameTime redTakenTime = 0_sec;
	GameTime blueTakenTime = 0_sec;
	GameTime redObeliskAttackedTime = 0_sec;
	GameTime blueObeliskAttackedTime = 0_sec;
};

TeamGame teamGame{};

gentity_t* neutralObelisk = nullptr;

namespace {

	template <typename Fn>
	void ForEachClient(Fn&& fn) {
		for (auto entity : active_clients()) {
			if (!entity || !entity->client) {
				continue;
			}
			std::forward<Fn>(fn)(entity);
		}
	}

	[[nodiscard]] bool SupportsCTF() {
		return Game::Has(GameFlags::CTF);
	}

	[[nodiscard]] bool IsPrimaryTeam(Team team) {
		return team == Team::Red || team == Team::Blue;
	}

	[[nodiscard]] const char* TeamFlagClassName(Team team) {
		switch (team) {
		case Team::Red:
			return ITEM_CTF_FLAG_RED;
		case Team::Blue:
			return ITEM_CTF_FLAG_BLUE;
		case Team::Free:
			return ITEM_CTF_FLAG_NEUTRAL;
		default:
			return nullptr;
		}
	}

	[[nodiscard]] item_id_t TeamFlagItem(Team team) {
		switch (team) {
		case Team::Red:
			return IT_FLAG_RED;
		case Team::Blue:
			return IT_FLAG_BLUE;
		case Team::Free:
			return IT_FLAG_NEUTRAL;
		default:
			return IT_NULL;
		}
	}

	[[nodiscard]] std::optional<Team> TeamFromFlagItem(item_id_t item) {
		switch (item) {
		case IT_FLAG_RED:
			return Team::Red;
		case IT_FLAG_BLUE:
			return Team::Blue;
		case IT_FLAG_NEUTRAL:
			return Team::Free;
		default:
			return std::nullopt;
		}
	}

	template <typename Flags>
	[[nodiscard]] bool HasSpawnFlagImpl(const Flags& flags, SpawnFlags flag) {
		if constexpr (requires(const Flags & f) { f.has(flag); }) {
			return flags.has(flag);
		}
		else {
			return (flags & flag) != 0;
		}
	}

	[[nodiscard]] bool HasSpawnFlag(const gentity_t* ent, SpawnFlags flag) {
		if (!ent) {
			return false;
		}
		return HasSpawnFlagImpl(ent->spawnFlags, flag);
	}

	[[nodiscard]] bool IsDroppedFlag(const gentity_t* ent) {
		return HasSpawnFlag(ent, SPAWNFLAG_ITEM_DROPPED);
	}

	[[nodiscard]] bool IsDroppedByPlayer(const gentity_t* ent) {
		return HasSpawnFlag(ent, SPAWNFLAG_ITEM_DROPPED_PLAYER);
	}

	[[nodiscard]] const char* TeamNameOrNeutral(Team team) {
		return team == Team::Free ? "NEUTRAL" : Teams_TeamName(team);
	}

	void ResetCarrierHurtTimers(Team team) {
		ForEachClient([team](gentity_t* entity) {
			if (entity->client->sess.team == team) {
				entity->client->resp.ctf_lasthurtcarrier = 0_ms;
			}
			});
	}

	void AwardAssistBonuses(gentity_t* scorer) {
		ForEachClient([scorer](gentity_t* teammate) {
			if (teammate->client->sess.team != scorer->client->sess.team) {
				teammate->client->resp.ctf_lasthurtcarrier = -5_sec;
				return;
			}

			if (teammate == scorer) {
				return;
			}

			G_AdjustPlayerScore(teammate->client, CTF::TEAM_BONUS, false, 0);

			if (teammate->client->resp.ctf_lastreturnedflag &&
				teammate->client->resp.ctf_lastreturnedflag + CTF::RETURN_FLAG_ASSIST_TIMEOUT > level.time) {
				gi.LocBroadcast_Print(PRINT_HIGH, "$g_bonus_assist_return", teammate->client->sess.netName);
				G_AdjustPlayerScore(teammate->client, CTF::RETURN_FLAG_ASSIST_BONUS, false, 0);
				PushAward(teammate, PlayerMedal::Assist);
			}

			if (teammate->client->resp.ctf_lastfraggedcarrier &&
				teammate->client->resp.ctf_lastfraggedcarrier + CTF::FRAG_CARRIER_ASSIST_TIMEOUT > level.time) {
				gi.LocBroadcast_Print(PRINT_HIGH, "$g_bonus_assist_frag_carrier", teammate->client->sess.netName);
				G_AdjustPlayerScore(teammate->client, CTF::FRAG_CARRIER_ASSIST_BONUS, false, 0);
				PushAward(teammate, PlayerMedal::Assist);
			}
			});
	}

	void ApplyCaptureRewards(gentity_t* flagEntity, gentity_t* scorer, Team scoringTeam) {
		if (!flagEntity || !scorer || !scorer->client) {
			return;
		}

		level.ctf_last_flag_capture = level.time;
		level.ctf_last_capture_team = scoringTeam;
		G_AdjustTeamScore(scoringTeam, Game::Is(GameType::CaptureStrike) ? 2 : 1);

		gi.sound(flagEntity, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundIndex("ctf/flagcap.wav"), 1, ATTN_NONE, 0);

		G_AdjustPlayerScore(scorer->client, CTF::CAPTURE_BONUS, false, 0);
		PushAward(scorer, PlayerMedal::Captures);

		AwardAssistBonuses(scorer);
	}

	void BroadcastCaptureMessage(Team scoringTeam, gentity_t* scorer, GameTime pickupTime) {
		if (!scorer || !scorer->client) {
			return;
		}

		if (pickupTime) {
			gi.LocBroadcast_Print(
				PRINT_HIGH,
				"{} TEAM CAPTURED the flag! ({} captured in {})\n",
				Teams_TeamName(scoringTeam),
				scorer->client->sess.netName,
				TimeString((level.time - pickupTime).milliseconds(), true, false));
		}
		else {
			gi.LocBroadcast_Print(
				PRINT_HIGH,
				"{} TEAM CAPTURED the flag! (captured by {})\n",
				Teams_TeamName(scoringTeam),
				scorer->client->sess.netName);
		}
	}

	[[nodiscard]] std::optional<gentity_t*> FindTeamFlag(Team team) {
		const char* className = TeamFlagClassName(team);
		if (!className) {
			return std::nullopt;
		}

		gentity_t* flag = nullptr;
		while ((flag = G_FindByString<&gentity_t::className>(flag, className)) != nullptr) {
			if (!IsDroppedFlag(flag)) {
				return flag;
			}
		}

		return std::nullopt;
	}

	[[nodiscard]] gentity_t* FindFlagCarrier(item_id_t flagItem) {
		gentity_t* carrier = nullptr;
		ForEachClient([&carrier, flagItem](gentity_t* entity) {
			if (!carrier && entity->client->pers.inventory[flagItem]) {
				carrier = entity;
			}
			});
		return carrier;
	}

	void AwardBaseDefense(gentity_t* attacker) {
		if (!attacker || !attacker->client) {
			return;
		}
		G_AdjustPlayerScore(attacker->client, CTF::FLAG_DEFENSE_BONUS, false, 0);
		PushAward(attacker, PlayerMedal::Defence);
	}

	void AwardCarrierDefense(gentity_t* attacker) {
		if (!attacker || !attacker->client) {
			return;
		}
		G_AdjustPlayerScore(attacker->client, CTF::CARRIER_PROTECT_BONUS, false, 0);
	}

	void AwardCarrierDangerDefense(gentity_t* attacker) {
		if (!attacker || !attacker->client) {
			return;
		}
		G_AdjustPlayerScore(attacker->client, CTF::CARRIER_DANGER_PROTECT_BONUS, false, 0);
		PushAward(attacker, PlayerMedal::Defence);
	}

	void Team_ReturnFlagSound(Team) {
		// TODO: hook up return VO
	}

	void Team_TakeFlagSound(Team team) {
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
		// TODO: hook up take VO
	}

	void Team_CaptureFlagSound_Internal(Team) {
		// TODO: hook up capture VO
	}

	void UpdateFlagConfigString() {
		std::string flagStatusStr;
		flagStatusStr.reserve(3);

		static constexpr std::array<char, 5> ctfFlagStatusRemap{ '0', '1', '*', '*', '2' };
		static constexpr std::array<char, 5> oneFlagStatusRemap{ '0', '1', '2', '3', '4' };

		if (Game::Is(GameType::CaptureTheFlag)) {
			flagStatusStr.push_back(ctfFlagStatusRemap.at(static_cast<int>(teamGame.redFlagStatus)));
			flagStatusStr.push_back(ctfFlagStatusRemap.at(static_cast<int>(teamGame.blueFlagStatus)));
		}
		else {
			flagStatusStr.push_back(oneFlagStatusRemap.at(static_cast<int>(teamGame.redFlagStatus)));
			flagStatusStr.push_back(oneFlagStatusRemap.at(static_cast<int>(teamGame.blueFlagStatus)));
			flagStatusStr.push_back(oneFlagStatusRemap.at(static_cast<int>(teamGame.neutralFlagStatus)));
		}

		// trap_SetConfigstring(CS_FLAGSTATUS, flagStatusStr);
	}

	bool SetFlagStatus_Internal(Team team, FlagStatus status) {
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
			break;
		}

		if (modified) {
			UpdateFlagConfigString();
		}

		return modified;
	}

	void AwardFlagCapture_Internal(gentity_t* flagEntity, gentity_t* scorer, Team scoringTeam, GameTime pickupTime) {
		BroadcastCaptureMessage(scoringTeam, scorer, pickupTime);
		ApplyCaptureRewards(flagEntity, scorer, scoringTeam);
		Team_CaptureFlagSound_Internal(scoringTeam);
	}

	void GiveFlagToPlayer(gentity_t* flagEntity, gentity_t* player, Team flagTeam, item_id_t flagItem) {
		if (!flagEntity || !player || !player->client) {
			return;
		}

		player->client->pers.inventory[flagItem] = 1;
		player->client->resp.ctf_flagsince = level.time;

		if (flagItem == IT_FLAG_NEUTRAL) {
			FlagStatus status = FlagStatus::Taken;
			switch (player->client->sess.team) {
			case Team::Red:
				status = FlagStatus::TakenRed;
				break;
			case Team::Blue:
				status = FlagStatus::TakenBlue;
				break;
			default:
				break;
			}
			SetFlagStatus_Internal(Team::Free, status);
			flagEntity->fteam = player->client->sess.team;
		}
		else {
			SetFlagStatus_Internal(flagTeam, FlagStatus::Taken);
		}

		Team_TakeFlagSound(player->client->sess.team);
	}

	void RemoveDroppedFlag(gentity_t* ent) {
		if (!ent) {
			return;
		}

		FreeEntity(ent);
	}

	void RespawnFlag(gentity_t* ent, Team team) {
		if (!ent) {
			return;
		}

		ent->svFlags &= ~SVF_NOCLIENT;
		ent->solid = SOLID_TRIGGER;
		gi.linkEntity(ent);
		ent->s.event = EV_ITEM_RESPAWN;
		if (team == Team::Free) {
			ent->fteam = Team::Free;
		}
	}

}

void Team_CaptureFlagSound(Team team) {
	Team_CaptureFlagSound_Internal(team);
}

bool SetFlagStatus(Team team, FlagStatus status) {
	return SetFlagStatus_Internal(team, status);
}

void AwardFlagCapture(gentity_t* flagEntity, gentity_t* scorer, Team scoringTeam, GameTime pickupTime) {
	AwardFlagCapture_Internal(flagEntity, scorer, scoringTeam, pickupTime);
}

static void Team_ReturnFlag(Team team) {
	if (!CTF_ResetTeamFlag(team)) {
		return;
	}

	Team_ReturnFlagSound(team);

	if (team == Team::Free) {
		gi.Broadcast_Print(PRINT_HIGH, "The flag has returned!\n");
	}
	else {
		gi.LocBroadcast_Print(PRINT_HIGH, "The {} flag has returned!\n", Teams_TeamName(team));
	}
}

static void Team_CheckDroppedItem(gentity_t* dropped) {
	if (!SupportsCTF() || !dropped || !dropped->item) {
		return;
	}

	switch (dropped->item->id) {
	case IT_FLAG_RED:
		SetFlagStatus(Team::Red, FlagStatus::Dropped);
		break;
	case IT_FLAG_BLUE:
		SetFlagStatus(Team::Blue, FlagStatus::Dropped);
		break;
	case IT_FLAG_NEUTRAL:
		SetFlagStatus(Team::Free, FlagStatus::Dropped);
		break;
	default:
		break;
	}
}

void CTF_ScoreBonuses(gentity_t* targ, gentity_t*, gentity_t* attacker) {
	if (!SupportsCTF()) {
		return;
	}
	if (!targ || !attacker || !targ->client || !attacker->client || targ == attacker) {
		return;
	}

	const Team targetTeam = targ->client->sess.team;
	const Team otherTeam = Teams_OtherTeam(targetTeam);
	if (otherTeam < Team::None) {
		return;
	}

	const item_id_t flagItem = targetTeam == Team::Red ? IT_FLAG_RED : IT_FLAG_BLUE;
	const item_id_t enemyFlagItem = targetTeam == Team::Red ? IT_FLAG_BLUE : IT_FLAG_RED;

	if (targ->client->pers.inventory[enemyFlagItem]) {
		attacker->client->resp.ctf_lastfraggedcarrier = level.time;
		G_AdjustPlayerScore(attacker->client, CTF::FRAG_CARRIER_BONUS, false, 0);
		gi.LocBroadcast_Print(PRINT_MEDIUM, "{} fragged {}'s flag carrier!",
			attacker->client->sess.netName,
			Teams_TeamName(targetTeam));
		ResetCarrierHurtTimers(otherTeam);
		return;
	}

	if (targ->client->resp.ctf_lasthurtcarrier &&
		level.time - targ->client->resp.ctf_lasthurtcarrier < CTF::CARRIER_DANGER_PROTECT_TIMEOUT &&
		!attacker->client->pers.inventory[flagItem]) {
		AwardCarrierDangerDefense(attacker);
		return;
	}

	auto flagEntity = FindTeamFlag(attacker->client->sess.team);
	if (!flagEntity) {
		return;
	}

	Vector3 v1 = targ->s.origin - (*flagEntity)->s.origin;
	Vector3 v2 = attacker->s.origin - (*flagEntity)->s.origin;

	if ((v1.length() < CTF::TARGET_PROTECT_RADIUS ||
		v2.length() < CTF::TARGET_PROTECT_RADIUS ||
		LocCanSee(*flagEntity, targ) || LocCanSee(*flagEntity, attacker)) &&
		attacker->client->sess.team != targetTeam) {
		AwardBaseDefense(attacker);
		return;
	}

	gentity_t* carrier = FindFlagCarrier(flagItem);
	if (!carrier || carrier == attacker) {
		return;
	}

	v1 = targ->s.origin - carrier->s.origin;
	v2 = attacker->s.origin - carrier->s.origin;

	if (v1.length() < CTF::ATTACKER_PROTECT_RADIUS ||
		v2.length() < CTF::ATTACKER_PROTECT_RADIUS ||
		LocCanSee(carrier, targ) || LocCanSee(carrier, attacker)) {
		AwardCarrierDefense(attacker);
	}
}

void CTF_CheckHurtCarrier(gentity_t* targ, gentity_t* attacker) {
	if (!SupportsCTF()) {
		return;
	}
	if (!targ || !attacker || !targ->client || !attacker->client) {
		return;
	}

	const item_id_t flagItem = targ->client->sess.team == Team::Red ? IT_FLAG_BLUE : IT_FLAG_RED;
	if (targ->client->pers.inventory[flagItem] &&
		targ->client->sess.team != attacker->client->sess.team) {
		attacker->client->resp.ctf_lasthurtcarrier = level.time;
	}
}

bool CTF_ResetTeamFlag(Team team) {
	if (!SupportsCTF()) {
		return false;
	}

	const char* className = TeamFlagClassName(team);
	if (!className) {
		return false;
	}

	bool found = false;
	bool restored = false;
	gentity_t* ent = nullptr;
	while ((ent = G_FindByString<&gentity_t::className>(ent, className)) != nullptr) {
		if (IsDroppedFlag(ent) || IsDroppedByPlayer(ent)) {
			RemoveDroppedFlag(ent);
			found = true;
			restored = true;
		}
		else {
			RespawnFlag(ent, team);
			found = true;
			restored = true;
		}
	}

	if (found) {
		SetFlagStatus(team, FlagStatus::AtBase);
		if (restored && Game::Is(GameType::CaptureStrike)) {
			const Team defendingTeam = level.strike_red_attacks ? Team::Blue : Team::Red;
			if (team == defendingTeam) {
				level.strike_flag_touch = false;
			}
		}
	}

	return found;
}

void CTF_ResetFlags() {
	if (!SupportsCTF()) {
		return;
	}

	CTF_ResetTeamFlag(Team::Red);
	CTF_ResetTeamFlag(Team::Blue);
	if (Game::Is(GameType::OneFlag)) {
		CTF_ResetTeamFlag(Team::Free);
	}
}

bool CTF_PickupFlag(gentity_t* ent, gentity_t* other) {
	if (!SupportsCTF() || !ent || !ent->item || !other || !other->client) {
		return false;
	}

	const auto flagTeamOpt = TeamFromFlagItem(ent->item->id);
	if (!flagTeamOpt) {
		gi.LocClient_Print(other, PRINT_HIGH, "Don't know what team the flag is on, removing.\n");
		RemoveDroppedFlag(ent);
		return false;
	}

	const Team flagTeam = *flagTeamOpt;
	const Team playerTeam = other->client->sess.team;
	const item_id_t flagItem = TeamFlagItem(flagTeam);
	const item_id_t enemyFlagItem = IsPrimaryTeam(flagTeam) ? TeamFlagItem(Teams_OtherTeam(flagTeam)) : IT_NULL;
	const bool droppedFlag = IsDroppedFlag(ent);
	const bool oneFlag = Game::Is(GameType::OneFlag);

	if (flagTeam == playerTeam && IsPrimaryTeam(playerTeam)) {
		if (!droppedFlag) {
			if (enemyFlagItem != IT_NULL && other->client->pers.inventory[enemyFlagItem]) {
				const GameTime pickupTime = other->client->pers.teamState.flag_pickup_time;
				other->client->pers.inventory[enemyFlagItem] = 0;
				other->client->resp.ctf_flagsince = 0_ms;

				AwardFlagCapture(ent, other, flagTeam, pickupTime);
				CTF_ResetFlags();

				if (Game::Is(GameType::CaptureStrike)) {
					gi.LocBroadcast_Print(PRINT_CENTER, "Flag captured!\n{} wins the round!\n", Teams_TeamName(flagTeam));
					Round_End();
				}

				return false;
			}
			return false;
		}

		gi.LocBroadcast_Print(PRINT_HIGH, "$g_returned_flag",
			other->client->sess.netName, Teams_TeamName(flagTeam));
		G_AdjustPlayerScore(other->client, CTF::RECOVERY_BONUS, false, 0);
		other->client->resp.ctf_lastreturnedflag = level.time;
		gi.sound(ent, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundIndex("ctf/flagret.wav"), 1, ATTN_NONE, 0);
		SetFlagStatus(flagTeam, FlagStatus::AtBase);
		CTF_ResetTeamFlag(flagTeam);
		return false;
	}

	if (oneFlag && !droppedFlag && flagTeam != Team::Free && IsPrimaryTeam(playerTeam) &&
		other->client->pers.inventory[IT_FLAG_NEUTRAL]) {
		const Team scoringTeam = playerTeam;
		const GameTime pickupTime = other->client->pers.teamState.flag_pickup_time;
		other->client->pers.inventory[IT_FLAG_NEUTRAL] = 0;
		other->client->resp.ctf_flagsince = 0_ms;
		other->client->pers.teamState.flag_pickup_time = 0_ms;

		AwardFlagCapture(ent, other, scoringTeam, pickupTime);
		CTF_ResetTeamFlag(Team::Free);
		return false;
	}

	if (Game::Is(GameType::CaptureStrike)) {
		if ((level.strike_red_attacks && playerTeam != Team::Red) ||
			(!level.strike_red_attacks && playerTeam != Team::Blue)) {
			return false;
		}
	}

	if (!droppedFlag) {
		other->client->pers.teamState.flag_pickup_time = level.time;
	}

	gi.LocBroadcast_Print(PRINT_HIGH, "$g_got_flag",
		other->client->sess.netName, TeamNameOrNeutral(flagTeam));
	G_AdjustPlayerScore(other->client, CTF::FLAG_BONUS, false, 0);
	if (Game::Is(GameType::CaptureStrike) && !level.strike_flag_touch) {
		level.strike_flag_touch = true;
	}

	GiveFlagToPlayer(ent, other, flagTeam, flagItem);
	return true;
}

static TOUCH(CTF_DropFlagTouch)(gentity_t* ent, gentity_t* other, const trace_t& tr, bool otherTouchingSelf)->void {
	if (!SupportsCTF()) {
		return;
	}

	if (other == ent->owner && ent->nextThink - level.time > CTF::AUTO_FLAG_RETURN_TIMEOUT - 2_sec) {
		return;
	}

	Touch_Item(ent, other, tr, otherTouchingSelf);
}

static THINK(CTF_DropFlagThink)(gentity_t* ent)->void {
	if (!SupportsCTF() || !ent || !ent->item) {
		return;
	}

	switch (ent->item->id) {
	case IT_FLAG_RED:
		CTF_ResetTeamFlag(Team::Red);
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_flag_returned", Teams_TeamName(Team::Red));
		break;
	case IT_FLAG_BLUE:
		CTF_ResetTeamFlag(Team::Blue);
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_flag_returned", Teams_TeamName(Team::Blue));
		break;
	case IT_FLAG_NEUTRAL:
		Team_ReturnFlag(Team::Free);
		break;
	default:
		break;
	}

	gi.sound(ent, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundIndex("ctf/flagret.wav"), 1, ATTN_NONE, 0);
}

void CTF_DeadDropFlag(gentity_t* self) {
	if (!SupportsCTF() || !self || !self->client) {
		return;
	}

	gentity_t* dropped = nullptr;
	Team droppedTeam = Team::None;

	if (self->client->pers.inventory[IT_FLAG_RED]) {
		dropped = Drop_Item(self, GetItemByIndex(IT_FLAG_RED));
		self->client->pers.inventory[IT_FLAG_RED] = 0;
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_lost_flag",
			self->client->sess.netName, Teams_TeamName(Team::Red));
		droppedTeam = Team::Red;
	}
	else if (self->client->pers.inventory[IT_FLAG_BLUE]) {
		dropped = Drop_Item(self, GetItemByIndex(IT_FLAG_BLUE));
		self->client->pers.inventory[IT_FLAG_BLUE] = 0;
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_lost_flag",
			self->client->sess.netName, Teams_TeamName(Team::Blue));
		droppedTeam = Team::Blue;
	}
	else if (self->client->pers.inventory[IT_FLAG_NEUTRAL]) {
		dropped = Drop_Item(self, GetItemByIndex(IT_FLAG_NEUTRAL));
		self->client->pers.inventory[IT_FLAG_NEUTRAL] = 0;
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_lost_flag",
			self->client->sess.netName, Teams_TeamName(Team::Free));
		droppedTeam = Team::Free;
	}

	self->client->pers.teamState.flag_pickup_time = 0_ms;

	if (!dropped) {
		return;
	}

	dropped->think = CTF_DropFlagThink;
	dropped->nextThink = level.time + CTF::AUTO_FLAG_RETURN_TIMEOUT;
	dropped->touch = CTF_DropFlagTouch;
	dropped->fteam = self->client->sess.team;

	switch (droppedTeam) {
	case Team::Red:
		SetFlagStatus(Team::Red, FlagStatus::Dropped);
		break;
	case Team::Blue:
		SetFlagStatus(Team::Blue, FlagStatus::Dropped);
		break;
	case Team::Free:
		SetFlagStatus(Team::Free, FlagStatus::Dropped);
		break;
	default:
		break;
	}
}

void CTF_DropFlag(gentity_t* ent, Item*) {
	if (!SupportsCTF() || !ent || !ent->client) {
		return;
	}

	ent->client->pers.teamState.flag_pickup_time = 0_ms;

	if (brandom()) {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_lusers_drop_flags");
	}
	else {
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_winners_drop_flags");
	}
}

static THINK(CTF_FlagThink)(gentity_t* ent)->void {
	if (!SupportsCTF() || !ent) {
		return;
	}

	if (ent->solid != SOLID_NOT) {
		ent->s.frame = 173 + (((ent->s.frame - 173) + 1) % 16);
	}
	ent->nextThink = level.time + 10_hz;
}

THINK(CTF_FlagSetup)(gentity_t* ent)->void {
	if (!SupportsCTF() || !ent) {
		return;
	}

	ent->mins = { -15, -15, -15 };
	ent->maxs = { 15, 15, 15 };

	if (ent->model) {
		gi.setModel(ent, ent->model);
	}
	else if (ent->item) {
		gi.setModel(ent, ent->item->worldModel);
	}

	ent->solid = SOLID_TRIGGER;
	ent->moveType = MoveType::Toss;
	ent->touch = Touch_Item;
	ent->s.frame = 173;

	const Vector3 dest = ent->s.origin + Vector3{ 0, 0, -128 };
	const trace_t tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, dest, ent, MASK_SOLID);
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

void CTF_ClientEffects(gentity_t* player) {
	if (!SupportsCTF() || !player || !player->client) {
		return;
	}

	player->s.effects &= ~(EF_FLAG_RED | EF_FLAG_BLUE);
	if (player->health > 0) {
		if (player->client->pers.inventory[IT_FLAG_NEUTRAL]) {
			switch (player->client->sess.team) {
			case Team::Red:
				player->s.effects |= EF_FLAG_RED;
				break;
			case Team::Blue:
				player->s.effects |= EF_FLAG_BLUE;
				break;
			default:
				player->s.effects |= EF_FLAG_RED | EF_FLAG_BLUE;
				break;
			}
		}
		else {
			if (player->client->pers.inventory[IT_FLAG_RED]) {
				player->s.effects |= EF_FLAG_RED;
			}
			if (player->client->pers.inventory[IT_FLAG_BLUE]) {
				player->s.effects |= EF_FLAG_BLUE;
			}
		}
	}

	if (player->client->pers.inventory[IT_FLAG_NEUTRAL]) {
		switch (player->client->sess.team) {
		case Team::Red:
			player->s.modelIndex3 = mi_ctf_red_flag;
			break;
		case Team::Blue:
			player->s.modelIndex3 = mi_ctf_blue_flag;
			break;
		default:
			player->s.modelIndex3 = 0;
			break;
		}
	}
	else if (player->client->pers.inventory[IT_FLAG_RED]) {
		player->s.modelIndex3 = mi_ctf_red_flag;
	}
	else if (player->client->pers.inventory[IT_FLAG_BLUE]) {
		player->s.modelIndex3 = mi_ctf_blue_flag;
	}
	else {
		player->s.modelIndex3 = 0;
	}
}

