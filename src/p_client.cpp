// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

// p_client.cpp (Player Client)
// This file manages the lifecycle and state of a player connected to the
// server. It handles everything from the initial connection and spawning
// into the world to death, respawning, and disconnection.
//
// Key Responsibilities:
// - Client Lifecycle: Implements `ClientConnect`, `ClientBegin`, and
//   `ClientDisconnect` to manage a player's session.
// - Spawning and Respawning: Contains the logic for `ClientSpawn` and
//   `ClientRespawn`, including selecting a spawn point (`SelectSpawnPoint`)
//   and putting the player into the game world.
// - Per-Frame Updates: The `ClientThink` function is the main entry point for
//   processing a player's user commands each frame, triggering movement and
//   actions.
// - Death and Intermission: Handles player death (`player_die`) and moving
//   the client to the intermission state at the end of a match.
// - State Management: Initializes and maintains the `gclient_t` struct, which
//   holds all of a player's game-related state.

#include "g_local.hpp"
#include "command_registration.hpp"
#include "monsters/m_player.hpp"
#include "bots/bot_includes.hpp"

void ClientConfig_Init(gclient_t* cl, const std::string& playerID, const std::string& playerName, const std::string& gameType);

static THINK(info_player_start_drop) (gentity_t* self) -> void {
	// allow them to drop
	self->solid = SOLID_TRIGGER;
	self->moveType = MoveType::Toss;
	self->mins = PLAYER_MINS;
	self->maxs = PLAYER_MAXS;
	gi.linkEntity(self);
}

static inline void deathmatch_spawn_flags(gentity_t* self) {
	if (st.noBots)
		self->flags = FL_NO_BOTS;
	if (st.noHumans)
		self->flags = FL_NO_HUMANS;
	if (st.arena)
		self->arena = st.arena;
}

static void BroadcastReadyStatus(gentity_t* ent) {
	gi.LocBroadcast_Print(PRINT_CENTER, "%bind:+wheel2:Use Compass to toggle your ready status.%.MATCH IS IN WARMUP\n{} is {}ready.", ent->client->sess.netName, ent->client->pers.readyStatus ? "" : "NOT ");
}

void ClientSetReadyStatus(gentity_t* ent, bool state, bool toggle) {
	if (!ReadyConditions(ent, false)) return;

	client_persistant_t* pers = &ent->client->pers;

	if (toggle) {
		pers->readyStatus = !pers->readyStatus;
	}
	else {
		if (pers->readyStatus == state) {
			gi.LocClient_Print(ent, PRINT_HIGH, "You are already {}ready.\n", state ? "" : "NOT ");
			return;
		}
		else {
			pers->readyStatus = state;
		}
	}
	BroadcastReadyStatus(ent);
}

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
The normal starting point for a level.

"noBots" will prevent bots from using this spot.
"noHumans" will prevent humans from using this spot.
*/
void SP_info_player_start(gentity_t* self) {
	// fix stuck spawn points
	if (gi.trace(self->s.origin, PLAYER_MINS, PLAYER_MAXS, self->s.origin, self, MASK_SOLID).startSolid)
		G_FixStuckObject(self, self->s.origin);

	// [Paril-KEX] on n64, since these can spawn riding elevators,
	// allow them to "ride" the elevators so respawning works
	if (level.isN64) {
		self->think = info_player_start_drop;
		self->nextThink = level.time + FRAME_TIME_S;
	}

	deathmatch_spawn_flags(self);
}

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32) INITIAL x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
A potential spawning position for deathmatch games.

The first time a player enters the game, they will be at an 'INITIAL' spot.
Targets will be fired when someone spawns in on them.
"noBots" will prevent bots from using this spot.
"noHumans" will prevent humans from using this spot.
*/
void SP_info_player_deathmatch(gentity_t* self) {
	if (!deathmatch->integer) {
		FreeEntity(self);
		return;
	}
	// Paril-KEX N64 doesn't display these
	if (level.isN64)
		return;

	CreateSpawnPad(self);

	deathmatch_spawn_flags(self);
}

/*QUAKED info_player_team_red (1 0 0) (-16 -16 -24) (16 16 32) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
A potential Red Team spawning position for CTF games.
*/
void SP_info_player_team_red(gentity_t* self) {}

/*QUAKED info_player_team_blue (0 0 1) (-16 -16 -24) (16 16 32) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
A potential Blue Team spawning position for CTF games.
*/
void SP_info_player_team_blue(gentity_t* self) {}

/*QUAKED info_player_coop (1 0 1) (-16 -16 -24) (16 16 32) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
A potential spawning position for coop games.
*/
void SP_info_player_coop(gentity_t* self) {
	if (!coop->integer) {
		FreeEntity(self);
		return;
	}

	SP_info_player_start(self);
}

/*QUAKED info_player_coop_lava (1 0 1) (-16 -16 -24) (16 16 32) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
A potential spawning position for coop games on rmine2 where lava level
needs to be checked.
*/
void SP_info_player_coop_lava(gentity_t* self) {
	if (!coop->integer) {
		FreeEntity(self);
		return;
	}

	// fix stuck spawn points
	if (gi.trace(self->s.origin, PLAYER_MINS, PLAYER_MAXS, self->s.origin, self, MASK_SOLID).startSolid)
		G_FixStuckObject(self, self->s.origin);
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
The deathmatch intermission point will be at one of these
Use 'angles' instead of 'angle', so you can set pitch or roll as well as yaw.  'pitch yaw roll'
*/
void SP_info_player_intermission(gentity_t* ent) {}

/*QUAKED info_ctf_teleport_destination (0.5 0.5 0.5) (-16 -16 -24) (16 16 32) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Point trigger_teleports at these.
*/
void SP_info_ctf_teleport_destination(gentity_t* ent) {
	ent->s.origin[Z] += 16;
}

// [Paril-KEX] whether instanced items should be used or not
bool P_UseCoopInstancedItems() {
	// squad respawn forces instanced items on, since we don't
	// want players to need to backtrack just to get their stuff.
	return g_coop_instanced_items->integer || g_coop_squad_respawn->integer;
}

//=======================================================================

/*
===============
PushAward
===============
*/
void PushAward(gentity_t* ent, PlayerMedal medal) {
	if (!ent || !ent->client) return;
	constexpr int MAX_QUEUED_AWARDS = 8;

	struct MedalInfo {
		std::string_view soundKeyFirst;
		std::string_view soundKeyRepeat;
	};
	static constexpr std::array<MedalInfo, static_cast<std::size_t>(PlayerMedal::Total)> medalTable = { {
		{ "", "" },                    // None
		{ "first_excellent", "excellent1" },
		{ "", "humiliation1" },
		{ "", "impressive1" },
		{ "", "rampage1" },
		{ "", "first_frag" },
		{ "", "defense1" },
		{ "", "assist1" },
		{ "", "" },                    // Captures
		{ "", "holy_shit" }
	} };

	auto& cl = *ent->client;
	auto idx = static_cast<std::size_t>(medal);
	auto& info = medalTable[idx];

	cl.pers.medalTime = level.time;
	cl.pers.medalType = medal;

	auto& count = cl.pers.match.medalCount[idx];
	++count;

	std::string_view key = (count == 1 && !info.soundKeyFirst.empty())
		? info.soundKeyFirst
		: info.soundKeyRepeat;

	if (!key.empty()) {
		const std::string path = G_Fmt("vo/{}.wav", key).data();
		const int soundIdx = gi.soundIndex(path.c_str());

		auto& queue = cl.pers.awardQueue;
		if (queue.queueSize < MAX_QUEUED_AWARDS) {
			queue.soundIndex[queue.queueSize++] = soundIdx;

			// If no sound is playing, start immediately
			if (queue.queueSize == 1) {
				queue.nextPlayTime = level.time;
				queue.playIndex = 0;
			}
		}
	}
}
//=======================================================================

/*
===============
P_SaveGhostSlot
===============
*/
void P_SaveGhostSlot(gentity_t* ent) {
	//TODO: don't do this if less than 1 minute played

	if (!ent || !ent->client)
		return;

	if (ent == host)
		return;

	gclient_t* cl = ent->client;

	if (!cl)
		return;

	if (level.matchState != MatchState::In_Progress)
		return;

	const char* socialID = cl->sess.socialID;
	if (!socialID || !*socialID)
		return;

	// Find existing ghost slot or first free one
	Ghosts* slot = nullptr;
	for (auto& g : level.ghosts) {
		if (Q_strcasecmp(g.socialID, socialID) == 0) {
			slot = &g;
			break;
		}
		if (!*g.socialID && !slot)
			slot = &g;
	}

	if (!slot)
		return; // No available slot

	// Store name and social ID
	Q_strlcpy(slot->netName, cl->sess.netName, sizeof(slot->netName));
	Q_strlcpy(slot->socialID, socialID, sizeof(slot->socialID));

	// Store inventory and stats
	slot->inventory = cl->pers.inventory;
	slot->ammoMax = cl->pers.ammoMax;
	slot->match = cl->pers.match;
	slot->weapon = cl->pers.weapon;
	slot->lastWeapon = cl->pers.lastWeapon;
	slot->team = cl->sess.team;
	slot->score = cl->resp.score;
	slot->skillRating = cl->sess.skillRating;
	slot->skillRatingChange = cl->sess.skillRatingChange;
	slot->origin = ent->s.origin;
	slot->angles = ent->s.angles;
	slot->totalMatchPlayRealTime = cl->resp.totalMatchPlayRealTime + cl->sess.playEndRealTime - cl->sess.playStartRealTime;
}

/*
===============
P_RestoreFromGhostSlot
===============
*/
void P_RestoreFromGhostSlot(gentity_t* ent) {
	if (!ent || !ent->client)
		return;

	gclient_t* cl = ent->client;

	if (!cl->sess.socialID || !*cl->sess.socialID)
		return;

	const char* socialID = cl->sess.socialID;

	for (auto& g : level.ghosts) {
		if (Q_strcasecmp(g.socialID, socialID) != 0)
			continue;

		// Restore inventory and stats
		cl->pers.inventory = g.inventory;
		cl->pers.ammoMax = g.ammoMax;
		cl->pers.match = g.match;
		cl->pers.weapon = g.weapon;
		cl->pers.lastWeapon = g.lastWeapon;
		cl->sess.team = g.team;
		cl->ps.teamID = static_cast<int>(cl->sess.team);
		cl->resp.score = g.score;
		cl->sess.skillRating = g.skillRating;
		cl->sess.skillRatingChange = g.skillRatingChange;
		ent->s.origin = g.origin;
		ent->s.angles = g.angles;
		cl->resp.totalMatchPlayRealTime = g.totalMatchPlayRealTime;

		gi.Client_Print(ent, PRINT_HIGH, "Your game state has been restored.\n");

		// Clear the ghost slot
		g = Ghosts{};
		return;
	}
}

//=======================================================================

static const char* ClientSkinOverride(const char* s) {
	// 1) If we allow custom skins, just pass it through
	if (g_allowCustomSkins->integer)
		return s;

	static const std::array<std::pair<std::string_view,
		std::vector<std::string_view>>,
		3> stockSkins = { {
	{ "male",   { "grunt",
				  "cipher","claymore","ctf_b","ctf_r","deaddude","disguise",
				  "flak","howitzer","insane1","insane2","insane3","major",
				  "nightops","pointman","psycho","rampage","razor","recon",
				  "rogue_b","rogue_r","scout","sniper","viper" } },
	{ "female", { "athena",
				  "brianna","cobalt","ctf_b","ctf_r","disguise","ensign",
				  "jezebel","jungle","lotus","rogue_b","rogue_r","stiletto",
				  "venus","voodoo" } },
	{ "cyborg", { "oni911",
				  "ctf_b","ctf_r","disguise","ps9000","tyr574" } }
	} };

	std::string_view req{ s };
	// 2) Split "model/skin"
	auto slash = req.find('/');
	std::string_view model = (slash != std::string_view::npos)
		? req.substr(0, slash)
		: std::string_view{};
	std::string_view skin = (slash != std::string_view::npos)
		? req.substr(slash + 1)
		: std::string_view{};

	// 3) Default to "male/grunt" if nothing sensible
	if (model.empty()) {
		model = "male";
		skin = "grunt";
	}

	// 4) Look up in our stock-skins table
	for (auto& [m, skins] : stockSkins) {
		if (m == model) {
			// 4a) If the skin is known, no change
			if (std::find(skins.begin(), skins.end(), skin) != skins.end()) {
				return s;
			}
			// 4b) Otherwise revert to this model's default skin
			auto& defaultSkin = skins.front();
			gi.Com_PrintFmt("{}: reverting to default skin: \"{}\" -> \"{}/{}\"\n", __FUNCTION__, s, m, defaultSkin);
			return G_Fmt("{}/{}", m, defaultSkin).data();
		}
	}

	// 5) Model not found at all -> global default
	gi.Com_PrintFmt("{}: model not recognized, reverting to \"male/grunt\" for \"{}\"\n", __FUNCTION__, s);
	return "male/grunt";
}

//=======================================================================
// PLAYER CONFIGS
//=======================================================================
/*
static void PCfg_WriteConfig(gentity_t *ent) {
	if (!std::strcmp(ent->client->sess.socialID, "me_a_bot"))
		return;

	const char *name = G_Fmt("baseq2/pcfg/wtest/{}.cfg", ent->client->sess.socialID).data();
	char *buffer = nullptr;
	std::string string;
	string.clear();

	FILE *f = std::fopen(name, "wb");
	if (!f)
		return;

	string = G_Fmt("// {}'s Player Config\n// Generated by WOR\n", ent->client->sess.netName).data();
	string += G_Fmt("name {}\n", ent->client->sess.netName).data();
	string += G_Fmt("show_id {}\n", ent->client->sess.pc.show_id ? 1 : 0).data();
	string += G_Fmt("show_fragmessages {}\n", ent->client->sess.pc.show_fragmessages ? 1 : 0).data();
	string += G_Fmt("show_timer {}\n", ent->client->sess.pc.show_timer ? 1 : 0).data();
	string += G_Fmt("killbeep_num {}\n", (int)ent->client->sess.pc.killbeep_num).data();

	std::fwrite(buffer, 1, strlen(buffer), f);
	std::fclose(f);
	gi.Com_PrintFmt("Player config written to: \"{}\"\n", name);
}
*/
static void PCfg_ClientInitPConfig(gentity_t* ent) {
	bool	file_exists = false;
	bool	cfg_valid = true;

	if (!ent->client) return;
	if (ent->svFlags & SVF_BOT) return;

	// load up file
	const char* name = G_Fmt("baseq2/pcfg/{}.cfg", ent->client->sess.socialID).data();

	FILE* f = fopen(name, "rb");
	if (f != NULL) {
		char* buffer = nullptr;
		size_t length;
		size_t read_length;

		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);

		if (length > 0x40000) {
			cfg_valid = false;
		}
		if (cfg_valid) {
			buffer = (char*)gi.TagMalloc(length + 1, '\0');
			if (length) {
				read_length = fread(buffer, 1, length, f);

				if (length != read_length) {
					cfg_valid = false;
				}
			}
		}
		file_exists = true;
		fclose(f);

		if (!cfg_valid) {
			gi.Com_PrintFmt("{}: Player config load error for \"{}\", discarding.\n", __FUNCTION__, name);
			return;
		}
	}

	// save file if it doesn't exist
	if (!file_exists) {
		f = fopen(name, "w");
		if (f) {
			const char* str = G_Fmt("// {}'s Player Config\n// Generated by WOR\n", ent->client->sess.netName).data();

			fwrite(str, 1, strlen(str), f);
			gi.Com_PrintFmt("{}: Player config written to: \"{}\"\n", __FUNCTION__, name);
			fclose(f);
		}
		else {
			gi.Com_PrintFmt("{}: Cannot save player config: {}\n", __FUNCTION__, name);
		}
	}
	else {
		//gi.Com_PrintFmt("{}: Player config not saved as file already exists: \"{}\"\n", __FUNCTION__, name);
	}
}

//=======================================================================
struct MonsterListInfo {
	std::string_view class_name;
	std::string_view display_name;
};

constexpr std::array<MonsterListInfo, 57> MONSTER_INFO = { {
	{ "monster_arachnid", "Arachnid" },
	{ "monster_army", "Grunt" },
	{ "monster_berserk", "Berserker" },
	{ "monster_boss", "Chton" },
	{ "monster_boss2", "Hornet" },
	{ "monster_boss5", "Super Tank" },
	{ "monster_brain", "Brains" },
	{ "monster_carrier", "Carrier" },
	{ "monster_chick", "Iron Maiden" },
	{ "monster_chick_heat", "Iron Maiden" },
	{ "monster_daedalus", "Daedalus" },
	{ "monster_demon1", "Fiend" },
	{ "monster_dog", "Rottweiler" },
	{ "monster_enforcer", "Enforcer" },
	{ "monster_fish", "Rotfish" },
	{ "monster_fixbot", "Fixbot" },
	{ "monster_flipper", "Barracuda Shark" },
	{ "monster_floater", "Technician" },
	{ "monster_flyer", "Flyer" },
	{ "monster_gekk", "Gekk" },
	{ "monster_gladb", "Gladiator" },
	{ "monster_gladiator", "Gladiator" },
	{ "monster_guardian", "Guardian" },
	{ "monster_guncmdr", "Gunner Commander" },
	{ "monster_gunner", "Gunner" },
	{ "monster_hell_knight", "Hell Knight" },
	{ "monster_hover", "Icarus" },
	{ "monster_infantry", "Infantry" },
	{ "monster_jorg", "Jorg" },
	{ "monster_kamikaze", "Kamikaze" },
	{ "monster_knight", "Knight" },
	{ "monster_makron", "Makron" },
	{ "monster_medic", "Medic" },
	{ "monster_medic_commander", "Medic Commander" },
	{ "monster_mutant", "Mutant" },
	{ "monster_ogre", "Ogre" },
	{ "monster_ogre_marksman", "Ogre Marksman" },
	{ "monster_oldone", "Shub-Niggurath" },
	{ "monster_parasite", "Parasite" },
	{ "monster_shalrath", "Vore" },
	{ "monster_shambler", "Shambler" },
	{ "monster_soldier", "Machinegun Guard" },
	{ "monster_soldier_hypergun", "Hypergun Guard" },
	{ "monster_soldier_lasergun", "Laser Guard" },
	{ "monster_soldier_light", "Light Guard" },
	{ "monster_soldier_ripper", "Ripper Guard" },
	{ "monster_soldier_ss", "Shotgun Guard" },
	{ "monster_stalker", "Stalker" },
	{ "monster_supertank", "Super Tank" },
	{ "monster_tank", "Tank" },
	{ "monster_tank_commander", "Tank Commander" },
	{ "monster_tarbaby", "Spawn" },
	{ "monster_turret", "Turret" },
	{ "monster_widow", "Black Widow" },
	{ "monster_widow2", "Black Widow" },
	{ "monster_wizard", "Scrag" },
	{ "monster_zombie", "Zombie" }
} };

static std::optional<std::string_view> GetMonsterDisplayName(std::string_view class_name) {
	for (const auto& monster : MONSTER_INFO) {
		if (Q_strcasecmp(class_name.data(), monster.class_name.data()) == 0) {
			return monster.display_name; // Return the found name
		}
	}
	return std::nullopt;
}

static bool IsVowel(const char c) {
	if (c == 'A' || c == 'a' ||
		c == 'E' || c == 'e' ||
		c == 'I' || c == 'i' ||
		c == 'O' || c == 'o' ||
		c == 'U' || c == 'u')
		return true;
	return false;
}

static void ClientObituary(gentity_t* victim, gentity_t* inflictor, gentity_t* attacker, MeansOfDeath mod) {
	std::string base{};

	if (!victim || !victim->client)
		return;

	if (attacker && CooperativeModeOn() && attacker->client)
		mod.friendly_fire = true;

	using enum ModID;

	if (mod.id == Silent)
		return;

	int killStreakCount = victim->client->killStreakCount;
	victim->client->killStreakCount = 0;

	switch (mod.id) {
	case Suicide:
		base = "{} suicides.\n";
		break;
	case Expiration:
		base = "{} ran out of blood.\n";
		break;
	case FallDamage:
		base = "{} cratered.\n";
		break;
	case Crushed:
		base = "{} was squished.\n";
		break;
	case Drowning:
		base = "{} sank like a rock.\n";
		break;
	case Slime:
		base = "{} melted.\n";
		break;
	case Lava:
		base = "{} does a back flip into the lava.\n";
		break;
	case Explosives:
	case Barrel:
		base = "{} blew up.\n";
		break;
	case ExitLevel:
		base = "{} found a way out.\n";
		break;
	case Laser:
		base = "{} saw the light.\n";
		break;
	case ShooterBlaster:
		base = "{} got blasted.\n";
		break;
	case Bomb:
	case Splash:
	case Hurt:
		base = "{} was in the wrong place.\n";
		break;
	default:
		//base = nullptr;
		break;
	}

	if (base.empty() && attacker == victim) {
		switch (mod.id) {
		case HandGrenade_Held:
			base = "{} tried to put the pin back in.\n";
			break;
		case HandGrenade_Splash:
		case GrenadeLauncher_Splash:
			base = "{} tripped on their own grenade.\n";
			break;
		case RocketLauncher_Splash:
			base = "{} blew themselves up.\n";
			break;
		case BFG10K_Blast:
			base = "{} should have used a smaller gun.\n";
			break;
		case Trap:
			base = "{} was sucked into their own trap.\n";
			break;
		case Thunderbolt_Discharge:
			base = "{} had a fatal discharge.\n";
			break;
		case Doppelganger_Explode:
			base = "{} was fooled by their own doppelganger.\n";
			break;
		case Expiration:
			base = "{} ran out of blood.\n";
			break;
		case TeslaMine:
			base = "{} got zapped by their own tesla mine.\n";
			break;
		default:
			base = "{} killed themselves.\n";
			break;
		}
	}

	// send generic/victim
	if (!base.empty()) {
		gi.LocBroadcast_Print(PRINT_MEDIUM, base.c_str(), victim->client->sess.netName);

		{
			std::string small;
			small = fmt::format("{}", victim->client->sess.netName);
			G_LogEvent(small);
		}

		victim->enemy = nullptr;
		return;
	}

	// has a killer
	victim->enemy = attacker;

	if (!attacker)
		return;

	if (attacker->svFlags & SVF_MONSTER) {
		if (auto monster_name = GetMonsterDisplayName(attacker->className)) {
			const std::string message = fmt::format("{} was killed by a {}.\n",
				victim->client->sess.netName,
				*monster_name);

			// Broadcast the message to all clients and write it to the server log.
			gi.LocBroadcast_Print(PRINT_MEDIUM, message.c_str());
			G_LogEvent(message);

			victim->enemy = nullptr;
		}
		return;
	}

	if (!attacker->client)
		return;
	switch (mod.id) {
	case Blaster:
		base = "{} was blasted by {}.\n";
		break;
	case Shotgun:
		base = "{} was gunned down by {}.\n";
		break;
	case SuperShotgun:
		base = "{} was blown away by {}'s Super Shotgun.\n";
		break;
	case Machinegun:
		base = "{} was machinegunned by {}.\n";
		break;
	case Chaingun:
		base = "{} was cut in half by {}'s Chaingun.\n";
		break;
	case GrenadeLauncher:
		base = "{} was popped by {}'s grenade.\n";
		break;
	case GrenadeLauncher_Splash:
		base = "{} was shredded by {}'s shrapnel.\n";
		break;
	case RocketLauncher:
		base = "{} ate {}'s rocket.\n";
		break;
	case RocketLauncher_Splash:
		base = "{} almost dodged {}'s rocket.\n";
		break;
	case HyperBlaster:
		base = "{} was melted by {}'s HyperBlaster.\n";
		break;
	case Railgun:
		base = "{} was railed by {}.\n";
		break;
	case BFG10K_Laser:
		base = "{} saw the pretty lights from {}'s BFG.\n";
		break;
	case BFG10K_Blast:
		base = "{} was disintegrated by {}'s BFG blast.\n";
		break;
	case BFG10K_Effect:
		base = "{} couldn't hide from {}'s BFG.\n";
		break;
	case HandGrenade:
		base = "{} caught {}'s handgrenade.\n";
		break;
	case HandGrenade_Splash:
		base = "{} didn't see {}'s handgrenade.\n";
		break;
	case HandGrenade_Held:
		base = "{} feels {}'s pain.\n";
		break;
	case Telefragged:
	case Telefrag_Spawn:
		base = "{} tried to invade {}'s personal space.\n";
		break;
	case IonRipper:
		base = "{} ripped to shreds by {}'s ripper gun.\n";
		break;
	case Phalanx:
		base = "{} was evaporated by {}.\n";
		break;
	case Trap:
		base = "{} was caught in {}'s trap.\n";
		break;
	case Chainfist:
		base = "{} was shredded by {}'s ripsaw.\n";
		break;
	case Disruptor:
		base = "{} lost his grip courtesy of {}'s Disintegrator.\n";
		break;
	case ETFRifle:
		base = "{} was perforated by {}.\n";
		break;
	case PlasmaBeam:
		base = "{} was scorched by {}'s Plasma Beam.\n";
		break;
	case Thunderbolt:
		base = "{} accepts {}'s shaft.\n";
		break;
	case Thunderbolt_Discharge:
		base = "{} accepts {}'s discharge.\n";
		break;
	case TeslaMine:
		base = "{} was enlightened by {}'s tesla mine.\n";
		break;
	case ProxMine:
		base = "{} got too close to {}'s proximity mine.\n";
		break;
	case Nuke:
		base = "{} was nuked by {}'s antimatter bomb.\n";
		break;
	case VengeanceSphere:
		base = "{} was purged by {}'s Vengeance Sphere.\n";
		break;
	case DefenderSphere:
		base = "{} had a blast with {}'s Defender Sphere.\n";
		break;
	case HunterSphere:
		base = "{} was hunted down by {}'s Hunter Sphere.\n";
		break;
	case Tracker:
		base = "{} was annihilated by {}'s Disruptor.\n";
		break;
	case Doppelganger_Explode:
		base = "{} was tricked by {}'s Doppelganger.\n";
		break;
	case Doppelganger_Vengeance:
		base = "{} was purged by {}'s Doppelganger.\n";
		break;
	case Doppelganger_Hunter:
		base = "{} was hunted down by {}'s Doppelganger.\n";
		break;
	case GrapplingHook:
		base = "{} was caught by {}'s grapple.\n";
		break;
	default:
		base = "{} was killed by {}.\n";
		break;
	}

	gi.LocBroadcast_Print(PRINT_MEDIUM, base.c_str(), victim->client->sess.netName, attacker->client->sess.netName);
	if (!base.empty()) {
		std::string small = fmt::vformat(base, fmt::make_format_args(victim->client->sess.netName, attacker->client->sess.netName));
		G_LogEvent(small);
	}

	if (Teams()) {
		// if at start and same team, clear.
		// [Paril-KEX] moved here so it's not an outlier in player_die.
		if (mod.id == Telefrag_Spawn &&
			victim->client->resp.ctf_state < 2 &&
			victim->client->sess.team == attacker->client->sess.team) {
			victim->client->resp.ctf_state = 0;
			return;
		}
	}

	// frag messages
	if (deathmatch->integer && victim != attacker && victim->client && attacker->client) {
		if (!(victim->svFlags & SVF_BOT)) {
			if (level.matchState == MatchState::Warmup_ReadyUp) {
				BroadcastReadyReminderMessage();
			}
			else {
				if (Game::Has(GameFlags::Rounds | GameFlags::Elimination) && level.roundState == RoundState::In_Progress) {
					gi.LocClient_Print(victim, PRINT_CENTER, ".You were fragged by {}\nYou will respawn next round.", attacker->client->sess.netName);
				}
				else if (Game::Is(GameType::FreezeTag) && level.roundState == RoundState::In_Progress) {
					bool last_standing = true;
					if (victim->client->sess.team == Team::Red && level.pop.num_living_red > 1 ||
						victim->client->sess.team == Team::Blue && level.pop.num_living_blue > 1)
						last_standing = false;
					gi.LocClient_Print(victim, PRINT_CENTER, ".You were frozen by {}{}",
						attacker->client->sess.netName,
						last_standing ? "" : "\nYou will respawn once thawed.");
				}
				else {
					gi.LocClient_Print(victim, PRINT_CENTER, ".You were {} by {}", Game::Is(GameType::FreezeTag) ? "frozen" : "fragged", attacker->client->sess.netName);
				}
			}
		}
		if (!(attacker->svFlags & SVF_BOT)) {
			if (Teams() && OnSameTeam(victim, attacker)) {
				gi.LocClient_Print(attacker, PRINT_CENTER, ".You fragged {}, your team mate :(", victim->client->sess.netName);
			}
			else {
				if (level.matchState == MatchState::Warmup_ReadyUp) {
					BroadcastReadyReminderMessage();
				}
				else if (attacker->client->killStreakCount && !(attacker->client->killStreakCount % 10)) {
					gi.LocBroadcast_Print(PRINT_CENTER, ".{} is on a rampage\nwith {} frags!", attacker->client->sess.netName, attacker->client->killStreakCount);
					PushAward(attacker, PlayerMedal::Rampage);
				}
				else if (killStreakCount >= 10) {
					gi.LocBroadcast_Print(PRINT_CENTER, ".{} put an end to {}'s\nrampage!", attacker->client->sess.netName, victim->client->sess.netName);
				}
				else if (Teams() || level.matchState != MatchState::In_Progress) {
					if (attacker->client->sess.pc.show_fragmessages)
						gi.LocClient_Print(attacker, PRINT_CENTER, ".You {} {}", Game::Is(GameType::FreezeTag) ? "froze" : "fragged", victim->client->sess.netName);
				}
				else {
					if (attacker->client->sess.pc.show_fragmessages)
						gi.LocClient_Print(attacker, PRINT_CENTER, ".You {} {}\n{} place with {}", Game::Is(GameType::FreezeTag) ? "froze" : "fragged",
							victim->client->sess.netName, PlaceString(attacker->client->pers.currentRank + 1), attacker->client->resp.score);
				}
			}
			if (attacker->client->sess.pc.killbeep_num > 0 && attacker->client->sess.pc.killbeep_num < 5) {
				const char* sb[5] = { "", "nav_editor/select_node.wav", "misc/comp_up.wav", "insane/insane7.wav", "nav_editor/finish_node_move.wav" };
				gi.localSound(attacker, CHAN_AUTO, gi.soundIndex(sb[attacker->client->sess.pc.killbeep_num]), 1, ATTN_NONE, 0);
			}
		}
	}

	if (base.size())
		return;

	gi.LocBroadcast_Print(PRINT_MEDIUM, "$g_mod_generic_died", victim->client->sess.netName);
}

/*
=================
TossClientItems

Toss the weapon, tech, CTF flag and powerups for the killed player
=================
*/
static void TossClientItems(gentity_t* self) {
	if (!deathmatch->integer)
		return;

	if (Game::Has(GameFlags::Arena))
		return;

	if (!ClientIsPlaying(self->client))
		return;

	if (!self->client->sess.initialised)
		return;

	// don't drop anything when combat is disabled
	if (CombatIsDisabled())
		return;

	Item* wp;
	gentity_t* drop;
	bool	quad, doubled, haste, protection, invis, regen;

	if (RS(RS_Q1)) {
		Drop_Backpack(self);
	}
	else {
		// drop weapon
		wp = self->client->pers.weapon;
		if (wp) {
			if (g_instaGib->integer)
				wp = nullptr;
			else if (g_nadeFest->integer)
				wp = nullptr;
			else if (!self->client->pers.inventory[self->client->pers.weapon->ammo])
				wp = nullptr;
			else if (!wp->drop)
				wp = nullptr;
			else if (RS(RS_Q3A) && wp->id == IT_WEAPON_MACHINEGUN)
				wp = nullptr;
			else if (RS(RS_Q1) && wp->id == IT_WEAPON_SHOTGUN)
				wp = nullptr;

			if (wp) {
				self->client->vAngle[YAW] = 0.0;
				drop = Drop_Item(self, wp);
				drop->spawnFlags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
				drop->spawnFlags &= ~SPAWNFLAG_ITEM_DROPPED;
				drop->svFlags &= ~SVF_INSTANCED;
			}
		}
	}

	//drop tech
	Tech_DeadDrop(self);

	// drop CTF flags
	CTF_DeadDropFlag(self);

	// drop powerup
	quad = (self->client->powerupTime.quadDamage > (level.time + 1_sec));
	haste = (self->client->powerupTime.haste > (level.time + 1_sec));
	doubled = (self->client->powerupTime.doubleDamage > (level.time + 1_sec));
	protection = (self->client->powerupTime.battleSuit > (level.time + 1_sec));
	invis = (self->client->powerupTime.invisibility > (level.time + 1_sec));
	regen = (self->client->powerupTime.regeneration > (level.time + 1_sec));

	if (!match_powerupDrops->integer) {
		quad = doubled = haste = protection = invis = regen = false;
	}

	if (quad) {
		self->client->vAngle[YAW] += 45.0;
		drop = Drop_Item(self, GetItemByIndex(IT_POWERUP_QUAD));
		drop->spawnFlags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
		drop->spawnFlags &= ~SPAWNFLAG_ITEM_DROPPED;
		drop->svFlags &= ~SVF_INSTANCED;

		drop->touch = Touch_Item;
		drop->nextThink = self->client->powerupTime.quadDamage;
		drop->think = g_quadhog->integer ? QuadHog_DoReset : FreeEntity;

		if (g_quadhog->integer) {
			drop->s.renderFX |= RF_SHELL_BLUE;
			drop->s.effects |= EF_COLOR_SHELL;
		}

		// decide how many seconds it has left
		drop->count = self->client->powerupTime.quadDamage.seconds<int>() - level.time.seconds<int>();
		if (drop->count < 1) {
			drop->count = 1;
		}
	}

	if (haste) {
		self->client->vAngle[YAW] += 45;
		drop = Drop_Item(self, GetItemByIndex(IT_POWERUP_HASTE));
		drop->spawnFlags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
		drop->spawnFlags &= ~SPAWNFLAG_ITEM_DROPPED;
		drop->svFlags &= ~SVF_INSTANCED;

		drop->touch = Touch_Item;
		drop->nextThink = self->client->powerupTime.haste;
		drop->think = FreeEntity;

		// decide how many seconds it has left
		drop->count = self->client->powerupTime.haste.seconds<int>() - level.time.seconds<int>();
		if (drop->count < 1) {
			drop->count = 1;
		}
	}

	if (protection) {
		self->client->vAngle[YAW] += 45;
		drop = Drop_Item(self, GetItemByIndex(IT_POWERUP_BATTLESUIT));
		drop->spawnFlags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
		drop->spawnFlags &= ~SPAWNFLAG_ITEM_DROPPED;
		drop->svFlags &= ~SVF_INSTANCED;

		drop->touch = Touch_Item;
		drop->nextThink = self->client->powerupTime.battleSuit;
		drop->think = FreeEntity;

		// decide how many seconds it has left
		drop->count = self->client->powerupTime.battleSuit.seconds<int>() - level.time.seconds<int>();
		if (drop->count < 1) {
			drop->count = 1;
		}
	}

	if (regen) {
		self->client->vAngle[YAW] += 45;
		drop = Drop_Item(self, GetItemByIndex(IT_POWERUP_REGEN));
		drop->spawnFlags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
		drop->spawnFlags &= ~SPAWNFLAG_ITEM_DROPPED;
		drop->svFlags &= ~SVF_INSTANCED;

		drop->touch = Touch_Item;
		drop->nextThink = self->client->powerupTime.regeneration;
		drop->think = FreeEntity;

		// decide how many seconds it has left
		drop->count = self->client->powerupTime.regeneration.seconds<int>() - level.time.seconds<int>();
		if (drop->count < 1) {
			drop->count = 1;
		}
	}

	if (invis) {
		self->client->vAngle[YAW] += 45;
		drop = Drop_Item(self, GetItemByIndex(IT_POWERUP_INVISIBILITY));
		drop->spawnFlags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
		drop->spawnFlags &= ~SPAWNFLAG_ITEM_DROPPED;
		drop->svFlags &= ~SVF_INSTANCED;

		drop->touch = Touch_Item;
		drop->nextThink = self->client->powerupTime.invisibility;
		drop->think = FreeEntity;

		// decide how many seconds it has left
		drop->count = self->client->powerupTime.invisibility.seconds<int>() - level.time.seconds<int>();
		if (drop->count < 1) {
			drop->count = 1;
		}
	}

	if (doubled) {
		self->client->vAngle[YAW] += 45;
		drop = Drop_Item(self, GetItemByIndex(IT_POWERUP_DOUBLE));
		drop->spawnFlags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
		drop->spawnFlags &= ~SPAWNFLAG_ITEM_DROPPED;
		drop->svFlags &= ~SVF_INSTANCED;

		drop->touch = Touch_Item;
		drop->nextThink = self->client->powerupTime.doubleDamage;
		drop->think = FreeEntity;

		// decide how many seconds it has left
		drop->count = self->client->powerupTime.doubleDamage.seconds<int>() - level.time.seconds<int>();
		if (drop->count < 1) {
			drop->count = 1;
		}
	}

	self->client->vAngle[YAW] = 0.0;
}

/*
==================
LookAtKiller
==================
*/
void LookAtKiller(gentity_t* self, gentity_t* inflictor, gentity_t* attacker) {
	Vector3 dir;

	if (attacker && attacker != world && attacker != self) {
		dir = attacker->s.origin - self->s.origin;
	}
	else if (inflictor && inflictor != world && inflictor != self) {
		dir = inflictor->s.origin - self->s.origin;
	}
	else {
		self->client->killerYaw = self->s.angles[YAW];
		return;
	}

	// PMM - fixed to correct for pitch of 0
	if (dir[0])
		self->client->killerYaw = 180 / PIf * atan2f(dir[1], dir[0]);
	else if (dir[1] > 0)
		self->client->killerYaw = 90;
	else if (dir[1] < 0)
		self->client->killerYaw = 270;
	else
		self->client->killerYaw = 0;
}

/*
================
Match_CanScore
================
*/
static bool Match_CanScore() {
	if (level.intermission.queued)
		return false;

	switch (level.matchState) {
		using enum MatchState;
	case Initial_Delay:
	case Warmup_Default:
	case Warmup_ReadyUp:
	case Countdown:
	case Ended:
		return false;
	}

	return true;
}

/*
==================
G_LogDeathEvent
==================
*/
static void G_LogDeathEvent(gentity_t* victim, gentity_t* attacker, MeansOfDeath mod) {
	MatchDeathEvent ev;

	if (level.matchState != MatchState::In_Progress) {
		return;
	}
	if (!level.match.deathLog.capacity()) {
		level.match.deathLog.reserve(2048);
	}
	if (!victim || !victim->client) {
		gi.Com_PrintFmt("{}: Invalid victim for death log\n", __FUNCTION__);
		return;
	}

	ev.time = level.time - level.levelStartTime;
	ev.victim.name.assign(victim->client->sess.netName);
	ev.victim.id.assign(victim->client->sess.socialID);
	if (attacker && attacker->client && attacker != &g_entities[0]) {
		ev.attacker.name.assign(attacker->client->sess.netName);
		ev.attacker.id.assign(attacker->client->sess.socialID);
	}
	else {
		ev.attacker.name = "Environment";
		ev.attacker.id = "0";
	}
	ev.mod = mod;

	try {
		level.match.deathLog.push_back(std::move(ev));
	}
	catch (const std::exception& e) {
		gi.Com_ErrorFmt("deathLog push_back failed: {}", e.what());
	}
}

/*
==================
PushDeathStats
==================
*/
static void PushDeathStats(gentity_t* victim, gentity_t* attacker, const MeansOfDeath& mod) {
	auto  now = level.time;
	auto& glob = level.match;
	auto* vcl = victim->client;
	auto& vSess = vcl->pers.match;
	bool  isSuicide = (attacker == victim);
	bool  validKill = (attacker && attacker->client && !isSuicide && !mod.friendly_fire);

	// -- handle a valid non-suicide kill --
	if (validKill) {
		auto* acl = attacker->client;
		auto& aSess = acl->pers.match;

		if (glob.totalKills == 0) {
			PushAward(attacker, PlayerMedal::First_Frag);
		}

		if (attacker->health > 0) {
			++acl->killStreakCount;
		}

		G_AdjustPlayerScore(acl, 1, Game::Is(GameType::TeamDeathmatch), 1);

		++aSess.totalKills;
		++aSess.modTotalKills[static_cast<int>(mod.id)];
		++glob.totalKills;
		++glob.modKills[static_cast<int>(mod.id)];
		if (now - victim->client->respawnMaxTime < 1_sec) {
			++glob.totalSpawnKills;
			++acl->pers.match.totalSpawnKills;
		}


		if (OnSameTeam(attacker, victim)) {
			++glob.totalTeamKills;
			++acl->pers.match.totalTeamKills;
		}

		if (acl->pers.lastFragTime && acl->pers.lastFragTime + 2_sec > now) {
			PushAward(attacker, PlayerMedal::Excellent);
		}
		acl->pers.lastFragTime = now;

		if (mod.id == ModID::Blaster || mod.id == ModID::Chainfist) {
			PushAward(attacker, PlayerMedal::Humiliation);
		}
	}

	// -- always record the victim's death --
	++vSess.totalDeaths;
	++glob.totalDeaths;
	++glob.modDeaths[static_cast<int>(mod.id)];
	++vSess.modTotalDeaths[static_cast<int>(mod.id)];

	if (isSuicide) {
		++vSess.totalSuicides;
	}
	else if (now - victim->client->respawnMaxTime < 1_sec) {
		++vSess.totalSpawnDeaths;
	}

	// -- penalty / follow-killer logic -- 
	bool inPlay = (level.matchState == MatchState::In_Progress);

	if (inPlay && attacker && attacker->client) {
		// attacker killed themselves or hit a teammate?
		if (isSuicide || mod.friendly_fire) {
			if (!mod.no_point_loss) {
				G_AdjustPlayerScore(attacker->client, -1, Game::Is(GameType::TeamDeathmatch), -1);
			}
			attacker->client->killStreakCount = 0;
		}
		else {
			// queue any spectators who want to follow the killer
			for (auto ec : active_clients()) {
				if (!ClientIsPlaying(ec->client)
					&& ec->client->sess.pc.follow_killer) {
					ec->client->follow.queuedTarget = attacker;
					ec->client->follow.queuedTime = now;
				}
			}
		}
	}
	else {
		// penalty to the victim
		if (!mod.no_point_loss) {
			G_AdjustPlayerScore(victim->client, -1, Game::Is(GameType::TeamDeathmatch), -1);
		}
	}
}

/*
==================
GibPlayer
==================
*/
static void GibPlayer(gentity_t* self, int damage) {
	if (self->flags & FL_NOGIB) {
		return;
	}

	// 1) udeath sound
	gi.sound(self,
		CHAN_BODY,
		gi.soundIndex("misc/udeath.wav"),
		1,
		ATTN_NORM,
		0);

	// 2) meatier gibs at deeper overkills (deathmatch only)
	static constexpr struct { int threshold; int count; } gibStages[] = {
		{ -300, 16 },
		{ -200, 12 },
		{  -100, 10 }
	};
	if (deathmatch->integer) {
		for (auto& stage : gibStages) {
			if (self->health < stage.threshold) {
				ThrowGibs(self,
					damage,
					{ { size_t(stage.count),
						"models/objects/gibs/sm_meat/tris.md2",
						GIB_NONE } });
			}
		}
	}

	// 3) always toss some small meat chunks
	ThrowGibs(self,
		damage,
		{ { size_t(8),
			"models/objects/gibs/sm_meat/tris.md2",
			GIB_NONE } });

	// 4) calculate a 'severity' from 1 (just under -40) up to 4 (really deep overkill)
	int overkill = GIB_HEALTH - self->health;           // e.g. -40 - (-100) = 60
	int severity = (overkill > 0) ? (overkill / 40) + 1 : 1;
	severity = std::min(severity, 4);

	// 5) random leg gibs (up to 2)
	{
		int maxLegs = std::min(severity, 2);
		int legCount = irandom(maxLegs + 1); // uniform [0..maxLegs]
		if (legCount > 0) {
			ThrowGibs(self,
				damage,
				{ { size_t(legCount),
					"models/objects/gibs/leg/tris.md2",
					GIB_NONE } });
		}
	}

	// 6) random bone gibs (up to 4)
	{
		int maxBones = std::min(severity * 2, 4);
		int boneCount = irandom(maxBones + 1);
		if (boneCount > 0) {
			ThrowGibs(self,
				damage,
				{ { size_t(boneCount),
					"models/objects/gibs/bone/tris.md2",
					GIB_NONE } });
		}
	}

	// 7) random forearm bones (up to 2)
	{
		int maxBone2 = std::min(severity, 2);
		int bone2Count = irandom(maxBone2 + 1);
		if (bone2Count > 0) {
			ThrowGibs(self,
				damage,
				{ { size_t(bone2Count),
					"models/objects/gibs/bone2/tris.md2",
					GIB_NONE } });
		}
	}

	// 8) random arm bones (up to 2)
	{
		int maxArms = std::min(severity, 2);
		int armCount = irandom(maxArms + 1);
		if (armCount > 0) {
			ThrowGibs(self,
				damage,
				{ { size_t(armCount),
					"models/objects/gibs/arm/tris.md2",
					GIB_NONE } });
		}
	}
}

/*
==================
player_die
==================
*/
DIE(player_die) (gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, const Vector3& point, const MeansOfDeath& mod) -> void {
	if (self->client->ps.pmove.pmType == PM_DEAD)
		return;

	if (level.intermission.time)
		return;

	PlayerTrail_Destroy(self);

	self->aVelocity = {};

	self->takeDamage = true;
	self->moveType = MoveType::Toss;

	self->s.modelIndex2 = 0; // remove linked weapon model
	self->s.modelIndex3 = 0; // remove linked ctf flag

	self->s.angles[PITCH] = 0;
	self->s.angles[ROLL] = 0;

	self->s.sound = 0;
	self->client->weaponSound = 0;

	self->maxs[2] = -8;

	self->svFlags |= SVF_DEADMONSTER;

	if (!self->deadFlag) {

		if (deathmatch->integer) {
			if (match_playerRespawnMinDelay->value) {
				self->client->respawnMinTime = (level.time + GameTime::from_sec(match_playerRespawnMinDelay->value));
			}
			else {
				self->client->respawnMinTime = level.time;
			}

			if (match_forceRespawnTime->value) {
				self->client->respawnMaxTime = (level.time + GameTime::from_sec(match_forceRespawnTime->value));
			}
			else {
				self->client->respawnMaxTime = (level.time + 1_sec);
			}
		}

		PushDeathStats(self, attacker, mod);

		LookAtKiller(self, inflictor, attacker);
		self->client->ps.pmove.pmType = PM_DEAD;
		ClientObituary(self, inflictor, attacker, mod);

		CTF_ScoreBonuses(self, inflictor, attacker);
		TossClientItems(self);
		Weapon_Grapple_DoReset(self->client);

		if (deathmatch->integer && !self->client->showScores)
			Commands::Help(self, CommandArgs{}); // show scores

		if (coop->integer && !P_UseCoopInstancedItems()) {
			// clear inventory
			// this is kind of ugly, but it's how we want to handle keys in coop
			for (int n = 0; n < IT_TOTAL; n++) {
				if (itemList[n].flags & IF_KEY)
					self->client->resp.coopRespawn.inventory[n] = self->client->pers.inventory[n];
				self->client->pers.inventory[n] = 0;
			}
		}
	}

	// remove powerups
	self->client->powerupTime.quadDamage = 0_ms;
	self->client->powerupTime.haste = 0_ms;
	self->client->powerupTime.doubleDamage = 0_ms;
	self->client->powerupTime.battleSuit = 0_ms;
	self->client->powerupTime.invisibility = 0_ms;
	self->client->powerupTime.regeneration = 0_ms;
	self->client->powerupTime.rebreather = 0_ms;
	self->client->powerupTime.enviroSuit = 0_ms;
	self->flags &= ~FL_POWER_ARMOR;

	self->client->lastDeathLocation = self->s.origin;

	// add damage event to heatmap
	HM_AddEvent(self->s.origin, 50.0f);

	// clear inventory
	if (Teams())
		self->client->pers.inventory.fill(0);

	// if there's a sphere around, let it know the player died.
	// vengeance and hunter will die if they're not attacking,
	// defender should always die
	if (self->client->ownedSphere) {
		gentity_t* sphere;

		sphere = self->client->ownedSphere;
		sphere->die(sphere, self, self, 0, vec3_origin, mod);
	}

	// if we've been killed by the tracker, GIB!
	if (mod.id == ModID::Tracker) {
		self->health = -100;
		damage = 400;
	}

	if (Game::Is(GameType::FreezeTag) && !level.intermission.time && self->client->eliminated && !self->client->resp.thawer) {
		self->s.effects |= EF_COLOR_SHELL;
		self->s.renderFX |= (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);
	}
	else {
		self->s.effects = EF_NONE;
		self->s.renderFX = RF_NONE;
	}

	// make sure no trackers are still hurting us.
	if (self->client->trackerPainTime) {
		RemoveAttackingPainDaemons(self);
	}

	// if we got obliterated by the nuke, don't gib
	if ((self->health < -80) && (mod.id == ModID::Nuke))
		self->flags |= FL_NOGIB;

	if (self->health < GIB_HEALTH) {
		GibPlayer(self, damage);

		// clear the "no-gib" flag in case it was set
		self->flags &= ~FL_NOGIB;

		ThrowClientHead(self, damage);

		// lock in a "dead" animation frame so we don't play the normal death anim
		self->client->anim.priority = ANIM_DEATH;
		self->client->anim.end = 0;
		self->takeDamage = false;

	}
	else {
		// --- normal death animation & sound ---
		if (!self->deadFlag) {
			// Freeze-mode gets a single static pose
			if (Game::Is(GameType::FreezeTag)) {
				self->s.frame = FRAME_crstnd01 - 1;
				self->client->anim.end = self->s.frame;
			}
			else {
				// pick one of the death animations
				self->client->anim.priority = ANIM_DEATH;
				bool ducked = (self->client->ps.pmove.pmFlags & PMF_DUCKED) != 0;

				if (ducked) {
					self->s.frame = FRAME_crdeath1 - 1;
					self->client->anim.end = FRAME_crdeath5;
				}
				else {
					static constexpr std::array<std::pair<int, int>, 3> deathRanges = { {
						{ FRAME_death101, FRAME_death106 },
						{ FRAME_death201, FRAME_death206 },
						{ FRAME_death301, FRAME_death308 }
					} };

					const auto& [start, end] = deathRanges[irandom(3)];

					self->s.frame = start - 1;
					self->client->anim.end = end;
				}
			}

			// play one of four death cries
			static constexpr std::array<const char*, 4> death_sounds = {
				"*death1.wav", "*death2.wav", "*death3.wav", "*death4.wav"
			};
			gi.sound(self,
				CHAN_VOICE,
				gi.soundIndex(random_element(death_sounds)),
				1,
				ATTN_NORM,
				0);

			self->client->anim.time = 0_ms;
		}
	}

	if (!self->deadFlag) {
		if (CooperativeModeOn() && (g_coop_squad_respawn->integer || g_coop_enable_lives->integer)) {
			if (g_coop_enable_lives->integer && self->client->pers.lives) {
				self->client->pers.lives--;
				self->client->resp.coopRespawn.lives--;
			}

			bool allPlayersDead = true;

			for (auto player : active_clients())
				if (player->health > 0 || (!level.campaign.deadly_kill_box && g_coop_enable_lives->integer && player->client->pers.lives > 0)) {
					allPlayersDead = false;
					break;
				}

			if (allPlayersDead) // allow respawns for telefrags and weird shit
			{
				level.campaign.coopLevelRestartTime = level.time + 5_sec;

				for (auto player : active_clients())
					gi.LocCenter_Print(player, "$g_coop_lose");
			}

			// in 3 seconds, attempt a respawn or put us into
			// spectator mode
			if (!level.campaign.coopLevelRestartTime)
				self->client->respawnMaxTime = level.time + 3_sec;
		}
	}

	G_LogDeathEvent(self, attacker, mod);

	self->deadFlag = true;

	gi.linkEntity(self);
}

//=======================================================================

#include <string>
//#include <sstream>

/*
===========================
Player_GiveStartItems
===========================
*/
static void Player_GiveStartItems(gentity_t* ent, const char* input) {
	const char* token;
	while ((token = COM_ParseEx(&input, ";")) && *token) {
		char token_copy[MAX_TOKEN_CHARS];
		Q_strlcpy(token_copy, token, sizeof(token_copy));

		const char* cursor = token_copy;
		const char* item_name = COM_Parse(&cursor);
		if (!item_name || !*item_name)
			continue;

		Item* item = FindItemByClassname(item_name);
		if (!item || !item->pickup) {
			gi.Com_PrintFmt("Invalid g_start_item entry: '{}'\n", item_name);
			continue;
		}

		int32_t count = 1;
		if (cursor && *cursor) {
			const char* count_str = COM_Parse(&cursor);
			if (count_str && *count_str)
				count = std::clamp(strtol(count_str, nullptr, 10), 0L, 999L);
		}

		if (count == 0) {
			ent->client->pers.inventory[item->id] = 0;
			continue;
		}

		if (item->id < 0 || item->id >= MAX_ITEMS) {
			gi.Com_PrintFmt("Item '{}' has invalid ID {}\n", item_name, static_cast<int>(item->id));
			continue;
		}

		gentity_t* dummy = Spawn();
		dummy->item = item;
		dummy->count = count;
		dummy->spawnFlags |= SPAWNFLAG_ITEM_DROPPED;
		item->pickup(dummy, ent);
		FreeEntity(dummy);
	}
}

/*
==============
InitClientPersistant

This is only called when the game first initializes in single player,
but is called after each death and level change in deathmatch
==============
*/
void InitClientPersistant(gentity_t* ent, gclient_t* client) {
	// backup & restore userInfo
	char userInfo[MAX_INFO_STRING];
	Q_strlcpy(userInfo, client->pers.userInfo, sizeof(userInfo));

	client->pers = client_persistant_t{};

	ClientUserinfoChanged(ent, userInfo);

	client->pers.health = 100;
	client->pers.maxHealth = 100;

	client->pers.medalTime = 0_sec;
	client->pers.medalType = PlayerMedal::None;
	std::fill(client->pers.match.medalCount.begin(), client->pers.match.medalCount.end(), 0);

	// don't give us weapons if we shouldn't have any
	if (ClientIsPlaying(client)) {
		// in coop, if there's already a player in the game and we're new,
		// steal their loadout. this would fix a potential softlock where a new
		// player may not have weapons at all.
		bool taken_loadout = false;

		int health, armor;
		gitem_armor_t armorType = armor_stats[game.ruleset][Armor::Jacket];

		if (Game::Has(GameFlags::Arena)) {
			health = std::clamp(g_arenaStartingHealth->integer, 1, 9999);
			armor = std::clamp(g_arenaStartingArmor->integer, 0, 999);
		}
		else {
			health = std::clamp(g_starting_health->integer, 1, 9999);
			armor = std::clamp(g_starting_armor->integer, 0, 999);
		}

		if (armor > armor_stats[game.ruleset][Armor::Jacket].max_count)
			if (armor > armor_stats[game.ruleset][Armor::Combat].max_count)
				armorType = armor_stats[game.ruleset][Armor::Body];
			else armorType = armor_stats[game.ruleset][Armor::Combat];

		client->pers.health = client->pers.maxHealth = health;

		if (deathmatch->integer) {
			int bonus = RS(RS_Q3A) ? 25 : g_starting_health_bonus->integer;
			if (!(Game::Has(GameFlags::Arena)) && bonus > 0) {
				client->pers.health += bonus;
				if (!(RS(RS_Q3A))) {
					client->pers.healthBonus = bonus;
				}
				ent->client->timeResidual = level.time;
			}
		}

		if (armorType.base_count == armor_stats[game.ruleset][Armor::Jacket].base_count)
			client->pers.inventory[IT_ARMOR_JACKET] = armor;
		else if (armorType.base_count == armor_stats[game.ruleset][Armor::Combat].base_count)
			client->pers.inventory[IT_ARMOR_COMBAT] = armor;
		else if (armorType.base_count == armor_stats[game.ruleset][Armor::Body].base_count)
			client->pers.inventory[IT_ARMOR_BODY] = armor;

		if (coop->integer) {
			for (auto player : active_clients()) {
				if (player == ent || !player->client->pers.spawned ||
					!ClientIsPlaying(player->client) ||
					player->moveType == MoveType::NoClip || player->moveType == MoveType::FreeCam)
					continue;

				client->pers.inventory = player->client->pers.inventory;
				client->pers.ammoMax = player->client->pers.ammoMax;
				client->pers.powerCubes = player->client->pers.powerCubes;
				taken_loadout = true;
				break;
			}
		}

		if (Game::Is(GameType::ProBall)) {
			client->pers.inventory[IT_WEAPON_CHAINFIST] = 1;
		}
		else if (!taken_loadout) {
			if (g_instaGib->integer) {
				client->pers.inventory[IT_WEAPON_RAILGUN] = 1;
				client->pers.inventory[IT_AMMO_SLUGS] = AMMO_INFINITE;
			}
			else if (g_nadeFest->integer) {
				client->pers.inventory[IT_AMMO_GRENADES] = AMMO_INFINITE;
			}
			else if (Game::Has(GameFlags::Arena)) {
				client->pers.ammoMax.fill(50);
				client->pers.ammoMax[static_cast<int>(AmmoID::Shells)] = 50;
				client->pers.ammoMax[static_cast<int>(AmmoID::Bullets)] = 300;
				client->pers.ammoMax[static_cast<int>(AmmoID::Grenades)] = 50;
				client->pers.ammoMax[static_cast<int>(AmmoID::Rockets)] = 50;
				client->pers.ammoMax[static_cast<int>(AmmoID::Cells)] = 200;
				client->pers.ammoMax[static_cast<int>(AmmoID::Slugs)] = 25;
				/*
				client->pers.ammoMax[AmmoID::Traps] = 5;
				client->pers.ammoMax[AmmoID::Flechettes] = 200;
				client->pers.ammoMax[AmmoID::Rounds] = 12;
				client->pers.ammoMax[AmmoID::TeslaMines] = 5;
				*/
				client->pers.inventory[IT_AMMO_SHELLS] = 50;
				if (!(RS(RS_Q1))) {
					client->pers.inventory[IT_AMMO_BULLETS] = 200;
					client->pers.inventory[IT_AMMO_GRENADES] = 50;
				}
				client->pers.inventory[IT_AMMO_ROCKETS] = 50;
				client->pers.inventory[IT_AMMO_CELLS] = 200;
				if (!(RS(RS_Q1)))
					client->pers.inventory[IT_AMMO_SLUGS] = 50;

				client->pers.inventory[IT_WEAPON_BLASTER] = 1;
				client->pers.inventory[IT_WEAPON_SHOTGUN] = 1;
				if (!(RS(RS_Q3A)))
					client->pers.inventory[IT_WEAPON_SSHOTGUN] = 1;
				if (!(RS(RS_Q1))) {
					client->pers.inventory[IT_WEAPON_MACHINEGUN] = 1;
					client->pers.inventory[IT_WEAPON_CHAINGUN] = 1;
				}
				client->pers.inventory[IT_WEAPON_GLAUNCHER] = 1;
				client->pers.inventory[IT_WEAPON_RLAUNCHER] = 1;
				client->pers.inventory[IT_WEAPON_HYPERBLASTER] = 1;
				client->pers.inventory[IT_WEAPON_PLASMABEAM] = 1;
				if (!(RS(RS_Q1)))
					client->pers.inventory[IT_WEAPON_RAILGUN] = 1;
			}
			else {
				if (RS(RS_Q3A)) {
					client->pers.ammoMax.fill(200);
					client->pers.ammoMax[static_cast<int>(AmmoID::Bullets)] = 200;
					client->pers.ammoMax[static_cast<int>(AmmoID::Shells)] = 200;
					client->pers.ammoMax[static_cast<int>(AmmoID::Cells)] = 200;

					client->pers.ammoMax[static_cast<int>(AmmoID::Traps)] = 200;
					client->pers.ammoMax[static_cast<int>(AmmoID::Flechettes)] = 200;
					client->pers.ammoMax[static_cast<int>(AmmoID::Rounds)] = 200;
					client->pers.ammoMax[static_cast<int>(AmmoID::TeslaMines)] = 200;

					client->pers.inventory[IT_WEAPON_CHAINFIST] = 1;
					client->pers.inventory[IT_WEAPON_MACHINEGUN] = 1;
					client->pers.inventory[IT_AMMO_BULLETS] = (Game::Is(GameType::TeamDeathmatch)) ? 50 : 100;
				}
				else if (RS(RS_Q1)) {
					client->pers.ammoMax.fill(200);
					client->pers.ammoMax[static_cast<int>(AmmoID::Bullets)] = 200;
					client->pers.ammoMax[static_cast<int>(AmmoID::Shells)] = 200;
					client->pers.ammoMax[static_cast<int>(AmmoID::Cells)] = 200;

					client->pers.ammoMax[static_cast<int>(AmmoID::Traps)] = 200;
					client->pers.ammoMax[static_cast<int>(AmmoID::Flechettes)] = 200;
					client->pers.ammoMax[static_cast<int>(AmmoID::Rounds)] = 200;
					client->pers.ammoMax[static_cast<int>(AmmoID::TeslaMines)] = 200;

					client->pers.inventory[IT_WEAPON_CHAINFIST] = 1;
					client->pers.inventory[IT_WEAPON_SHOTGUN] = 1;
					client->pers.inventory[IT_AMMO_SHELLS] = 10;
				}
				else {
					// fill with 50s, since it's our most common value
					client->pers.ammoMax.fill(50);
					client->pers.ammoMax[static_cast<int>(AmmoID::Bullets)] = 200;
					client->pers.ammoMax[static_cast<int>(AmmoID::Shells)] = 100;
					client->pers.ammoMax[static_cast<int>(AmmoID::Cells)] = 200;

					client->pers.ammoMax[static_cast<int>(AmmoID::Traps)] = 5;
					client->pers.ammoMax[static_cast<int>(AmmoID::Flechettes)] = 200;
					client->pers.ammoMax[static_cast<int>(AmmoID::Rounds)] = 12;
					client->pers.ammoMax[static_cast<int>(AmmoID::TeslaMines)] = 5;

					client->pers.inventory[IT_WEAPON_BLASTER] = 1;
				}

				if (deathmatch->integer) {
					if (level.matchState < MatchState::In_Progress) {
						for (size_t i = FIRST_WEAPON; i < LAST_WEAPON; i++) {
							if (!level.weaponCount[i - FIRST_WEAPON])
								continue;

							if (!itemList[i].ammo)
								continue;

							client->pers.inventory[i] = 1;

							Item* ammo = GetItemByIndex(itemList[i].ammo);
							if (ammo)
								Add_Ammo(&g_entities[client - game.clients + 1], ammo, InfiniteAmmoOn(ammo) ? AMMO_INFINITE : ammo->quantity * 2);

							//gi.Com_PrintFmt("wp={} wc={} am={} q={}\n", i, level.weaponCount[i - FIRST_WEAPON], itemList[i].ammo, InfiniteAmmoOn(ammo) ? AMMO_INFINITE : ammo->quantity * 2);
						}
					}
				}
			}

			if (*g_start_items->string)
				Player_GiveStartItems(ent, g_start_items->string);
			if (level.start_items && *level.start_items)
				Player_GiveStartItems(ent, level.start_items);

			if (!deathmatch->integer || level.matchState < MatchState::In_Progress)
				// compass also used for ready status toggling in deathmatch
				client->pers.inventory[IT_COMPASS] = 1;

			bool give_grapple = (!strcmp(g_allow_grapple->string, "auto")) ?
				(Game::Has(GameFlags::CTF) ? !level.no_grapple : 0) :
				(g_allow_grapple->integer > 0 && !g_grapple_offhand->integer);
			if (give_grapple)
				client->pers.inventory[IT_WEAPON_GRAPPLE] = 1;
		}

		NoAmmoWeaponChange(ent, false);

		client->pers.weapon = client->weapon.pending;
		if (client->weapon.pending)
			client->pers.selectedItem = client->weapon.pending->id;
		client->weapon.pending = nullptr;
		client->pers.lastWeapon = client->pers.weapon;
	}

	if (CooperativeModeOn() && g_coop_enable_lives->integer)
		client->pers.lives = g_coop_num_lives->integer + 1;

	if (ent->client->pers.autoshield >= AUTO_SHIELD_AUTO)
		ent->flags |= FL_WANTS_POWER_ARMOR;

	client->pers.connected = true;
	client->pers.spawned = true;

	P_RestoreFromGhostSlot(ent);
}

static void InitClientResp(gclient_t* cl) {
	cl->resp = client_respawn_t{};

	cl->resp.enterTime = level.time;
	cl->resp.coopRespawn = cl->pers;
}

/*
==================
SaveClientData

Some information that should be persistant, like health,
is still stored in the entity structure, so it needs to
be mirrored out to the client structure before all the
gentities are wiped.
==================
*/
void SaveClientData() {
	gentity_t* ent;

	for (size_t i = 0; i < game.maxClients; i++) {
		ent = &g_entities[1 + i];
		if (!ent->inUse)
			continue;
		game.clients[i].pers.health = ent->health;
		game.clients[i].pers.maxHealth = ent->maxHealth;
		game.clients[i].pers.saved_flags = (ent->flags & (FL_FLASHLIGHT | FL_GODMODE | FL_NOTARGET | FL_POWER_ARMOR | FL_WANTS_POWER_ARMOR));
		if (coop->integer)
			game.clients[i].pers.score = ent->client->resp.score;
	}
}

void FetchClientEntData(gentity_t* ent) {
	ent->health = ent->client->pers.health;
	ent->maxHealth = ent->client->pers.maxHealth;
	ent->flags |= ent->client->pers.saved_flags;
	if (coop->integer)
		G_SetPlayerScore(ent->client, ent->client->pers.score);
}

//======================================================================

void InitBodyQue() {
	gentity_t* ent;

	level.bodyQue = 0;
	for (size_t i = 0; i < BODY_QUEUE_SIZE; i++) {
		ent = Spawn();
		ent->className = "bodyque";
	}
}

static DIE(body_die) (gentity_t* self, gentity_t* inflictor, gentity_t* attacker, int damage, const Vector3& point, const MeansOfDeath& mod) -> void {
	if (self->s.modelIndex == MODELINDEX_PLAYER &&
		self->health < self->gibHealth) {
		gi.sound(self, CHAN_BODY, gi.soundIndex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		ThrowGibs(self, damage, { { 4, "models/objects/gibs/sm_meat/tris.md2" } });
		self->s.origin[Z] -= 48;
		ThrowClientHead(self, damage);
	}

	if (mod.id == ModID::Crushed) {
		// prevent explosion singularities
		self->svFlags = SVF_NOCLIENT;
		self->takeDamage = false;
		self->solid = SOLID_NOT;
		self->moveType = MoveType::NoClip;
		gi.linkEntity(self);
	}
}

/*
=============
BodySink

After sitting around for x seconds, fall into the ground and disappear
=============
*/
static THINK(BodySink) (gentity_t* ent) -> void {
	if (!ent->linked)
		return;

	if (level.time > ent->timeStamp) {
		ent->svFlags = SVF_NOCLIENT;
		ent->takeDamage = false;
		ent->solid = SOLID_NOT;
		ent->moveType = MoveType::NoClip;

		// the body ques are never actually freed, they are just unlinked
		gi.unlinkEntity(ent);
		return;
	}
	ent->nextThink = level.time + 50_ms;
	ent->s.origin[Z] -= 0.5;
	gi.linkEntity(ent);
}

void CopyToBodyQue(gentity_t* ent) {
	// if we were completely removed, don't bother with a body
	if (!ent->s.modelIndex)
		return;

	gentity_t* body;
	bool frozen = !!(Game::Is(GameType::FreezeTag) && !level.intermission.time && ent->client->eliminated && !ent->client->resp.thawer);

	// grab a body que and cycle to the next one
	body = &g_entities[game.maxClients + level.bodyQue + 1];
	level.bodyQue = (static_cast<size_t>(level.bodyQue) + 1u) % BODY_QUEUE_SIZE;

	// FIXME: send an effect on the removed body

	gi.unlinkEntity(ent);

	gi.unlinkEntity(body);
	body->s = ent->s;
	body->s.number = body - g_entities;
	body->s.skinNum = ent->s.skinNum & 0xFF; // only copy the client #

	if (frozen) {
		body->s.effects |= EF_COLOR_SHELL;
		body->s.renderFX |= (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);
	}
	else {
		body->s.effects = EF_NONE;
		body->s.renderFX = RF_NONE;
	}

	body->svFlags = ent->svFlags;
	body->absMin = ent->absMin;
	body->absMax = ent->absMax;
	body->size = ent->size;
	body->solid = ent->solid;
	body->clipMask = ent->clipMask;
	body->owner = ent->owner;
	body->moveType = ent->moveType;
	body->health = ent->health;
	body->gibHealth = ent->gibHealth;
	body->s.event = EV_OTHER_TELEPORT;
	body->velocity = ent->velocity;
	body->aVelocity = ent->aVelocity;
	body->groundEntity = ent->groundEntity;
	body->groundEntity_linkCount = ent->groundEntity_linkCount;

	if (ent->takeDamage) {
		body->mins = ent->mins;
		body->maxs = ent->maxs;
	}
	else
		body->mins = body->maxs = {};

	if (CORPSE_SINK_TIME > 0_ms && Game::IsNot(GameType::FreezeTag)) {
		body->timeStamp = level.time + CORPSE_SINK_TIME + 1.5_sec;
		body->nextThink = level.time + CORPSE_SINK_TIME;
		body->think = BodySink;
	}

	body->die = body_die;
	body->takeDamage = true;

	gi.linkEntity(body);
}

void G_PostRespawn(gentity_t* self) {
	if (self->svFlags & SVF_NOCLIENT)
		return;

	// add a teleportation effect
	self->s.event = EV_PLAYER_TELEPORT;

	// hold in place briefly
	self->client->ps.pmove.pmFlags |= PMF_TIME_KNOCKBACK;
	self->client->ps.pmove.pmTime = 112;

	self->client->respawnMinTime = 0_ms;
	self->client->respawnMaxTime = level.time;

	if (deathmatch->integer && level.matchState == MatchState::Warmup_ReadyUp)
		BroadcastReadyReminderMessage();
}

void ClientRespawn(gentity_t* ent) {
	if (deathmatch->integer || coop->integer) {
		// spectators don't leave bodies
		if (ClientIsPlaying(ent->client))
			CopyToBodyQue(ent);
		ent->svFlags &= ~SVF_NOCLIENT;

		if (Game::Is(GameType::RedRover) && level.matchState == MatchState::In_Progress) {
			ent->client->sess.team = Teams_OtherTeam(ent->client->sess.team);
			ent->client->ps.teamID = static_cast<int>(ent->client->sess.team);
			AssignPlayerSkin(ent, ent->client->sess.skinName);
		}

		ClientSpawn(ent);
		G_PostRespawn(ent);
		return;
	}

	// restart the entire server
	gi.AddCommandString("menu_loadgame\n");
}

//==============================================================

// [Paril-KEX]
// skinNum was historically used to pack data
// so we're going to build onto that.
void P_AssignClientSkinNum(gentity_t* ent) {
	if (ent->s.modelIndex != 255)
		return;

	player_skinnum_t packed{};

	packed.clientNum = ent->client - game.clients;
	if (ent->client->pers.weapon)
		packed.viewWeaponIndex = ent->client->pers.weapon->viewWeaponIndex - level.viewWeaponOffset + 1;
	else
		packed.viewWeaponIndex = 0;
	packed.viewHeight = ent->client->ps.viewOffset.z + ent->client->ps.pmove.viewHeight;

	if (CooperativeModeOn())
		packed.teamIndex = 1; // all players are teamed in coop
	else if (Teams())
		packed.teamIndex = static_cast<int>(ent->client->sess.team);
	else
		packed.teamIndex = 0;

	packed.poiIcon = ent->deadFlag ? 1 : 0;

	ent->s.skinNum = packed.skinNum;
}

// [Paril-KEX] send player level POI
void P_SendLevelPOI(gentity_t* ent) {
	if (!level.poi.valid)
		return;

	gi.WriteByte(svc_poi);
	gi.WriteShort(POI_OBJECTIVE);
	gi.WriteShort(10000);
	gi.WritePosition(ent->client->compass.poiLocation);
	gi.WriteShort(ent->client->compass.poiImage);
	gi.WriteByte(208);
	gi.WriteByte(POI_FLAG_NONE);
	gi.unicast(ent, true);
}

// [Paril-KEX] force the fog transition on the given player,
// optionally instantaneously (ignore any transition time)
void P_ForceFogTransition(gentity_t* ent, bool instant) {
	// sanity check; if we're not changing the values, don't bother
	if (ent->client->fog == ent->client->pers.wanted_fog &&
		ent->client->heightfog == ent->client->pers.wanted_heightfog)
		return;

	svc_fog_data_t fog{};

	// check regular fog
	if (ent->client->pers.wanted_fog[0] != ent->client->fog[0] ||
		ent->client->pers.wanted_fog[4] != ent->client->fog[4]) {
		fog.bits |= svc_fog_data_t::BIT_DENSITY;
		fog.density = ent->client->pers.wanted_fog[0];
		fog.skyfactor = ent->client->pers.wanted_fog[4] * 255.f;
	}
	if (ent->client->pers.wanted_fog[1] != ent->client->fog[1]) {
		fog.bits |= svc_fog_data_t::BIT_R;
		fog.red = ent->client->pers.wanted_fog[1] * 255.f;
	}
	if (ent->client->pers.wanted_fog[2] != ent->client->fog[2]) {
		fog.bits |= svc_fog_data_t::BIT_G;
		fog.green = ent->client->pers.wanted_fog[2] * 255.f;
	}
	if (ent->client->pers.wanted_fog[3] != ent->client->fog[3]) {
		fog.bits |= svc_fog_data_t::BIT_B;
		fog.blue = ent->client->pers.wanted_fog[3] * 255.f;
	}

	if (!instant && ent->client->pers.fog_transition_time) {
		fog.bits |= svc_fog_data_t::BIT_TIME;
		fog.time = std::clamp(ent->client->pers.fog_transition_time.milliseconds(), (int64_t)0, (int64_t)std::numeric_limits<uint16_t>::max());
	}

	// check heightfog stuff
	auto& hf = ent->client->heightfog;
	const auto& wanted_hf = ent->client->pers.wanted_heightfog;

	if (hf.falloff != wanted_hf.falloff) {
		fog.bits |= svc_fog_data_t::BIT_HEIGHTFOG_FALLOFF;
		if (!wanted_hf.falloff)
			fog.hf_falloff = 0;
		else
			fog.hf_falloff = wanted_hf.falloff;
	}
	if (hf.density != wanted_hf.density) {
		fog.bits |= svc_fog_data_t::BIT_HEIGHTFOG_DENSITY;

		if (!wanted_hf.density)
			fog.hf_density = 0;
		else
			fog.hf_density = wanted_hf.density;
	}

	if (hf.start[0] != wanted_hf.start[0]) {
		fog.bits |= svc_fog_data_t::BIT_HEIGHTFOG_START_R;
		fog.hf_start_r = wanted_hf.start[0] * 255.f;
	}
	if (hf.start[1] != wanted_hf.start[1]) {
		fog.bits |= svc_fog_data_t::BIT_HEIGHTFOG_START_G;
		fog.hf_start_g = wanted_hf.start[1] * 255.f;
	}
	if (hf.start[2] != wanted_hf.start[2]) {
		fog.bits |= svc_fog_data_t::BIT_HEIGHTFOG_START_B;
		fog.hf_start_b = wanted_hf.start[2] * 255.f;
	}
	if (hf.start[3] != wanted_hf.start[3]) {
		fog.bits |= svc_fog_data_t::BIT_HEIGHTFOG_START_DIST;
		fog.hf_start_dist = wanted_hf.start[3];
	}

	if (hf.end[0] != wanted_hf.end[0]) {
		fog.bits |= svc_fog_data_t::BIT_HEIGHTFOG_END_R;
		fog.hf_end_r = wanted_hf.end[0] * 255.f;
	}
	if (hf.end[1] != wanted_hf.end[1]) {
		fog.bits |= svc_fog_data_t::BIT_HEIGHTFOG_END_G;
		fog.hf_end_g = wanted_hf.end[1] * 255.f;
	}
	if (hf.end[2] != wanted_hf.end[2]) {
		fog.bits |= svc_fog_data_t::BIT_HEIGHTFOG_END_B;
		fog.hf_end_b = wanted_hf.end[2] * 255.f;
	}
	if (hf.end[3] != wanted_hf.end[3]) {
		fog.bits |= svc_fog_data_t::BIT_HEIGHTFOG_END_DIST;
		fog.hf_end_dist = wanted_hf.end[3];
	}

	if (fog.bits & 0xFF00)
		fog.bits |= svc_fog_data_t::BIT_MORE_BITS;

	gi.WriteByte(svc_fog);

	if (fog.bits & svc_fog_data_t::BIT_MORE_BITS)
		gi.WriteShort(fog.bits);
	else
		gi.WriteByte(fog.bits);

	if (fog.bits & svc_fog_data_t::BIT_DENSITY) {
		gi.WriteFloat(fog.density);
		gi.WriteByte(fog.skyfactor);
	}
	if (fog.bits & svc_fog_data_t::BIT_R)
		gi.WriteByte(fog.red);
	if (fog.bits & svc_fog_data_t::BIT_G)
		gi.WriteByte(fog.green);
	if (fog.bits & svc_fog_data_t::BIT_B)
		gi.WriteByte(fog.blue);
	if (fog.bits & svc_fog_data_t::BIT_TIME)
		gi.WriteShort(fog.time);

	if (fog.bits & svc_fog_data_t::BIT_HEIGHTFOG_FALLOFF)
		gi.WriteFloat(fog.hf_falloff);
	if (fog.bits & svc_fog_data_t::BIT_HEIGHTFOG_DENSITY)
		gi.WriteFloat(fog.hf_density);

	if (fog.bits & svc_fog_data_t::BIT_HEIGHTFOG_START_R)
		gi.WriteByte(fog.hf_start_r);
	if (fog.bits & svc_fog_data_t::BIT_HEIGHTFOG_START_G)
		gi.WriteByte(fog.hf_start_g);
	if (fog.bits & svc_fog_data_t::BIT_HEIGHTFOG_START_B)
		gi.WriteByte(fog.hf_start_b);
	if (fog.bits & svc_fog_data_t::BIT_HEIGHTFOG_START_DIST)
		gi.WriteLong(fog.hf_start_dist);

	if (fog.bits & svc_fog_data_t::BIT_HEIGHTFOG_END_R)
		gi.WriteByte(fog.hf_end_r);
	if (fog.bits & svc_fog_data_t::BIT_HEIGHTFOG_END_G)
		gi.WriteByte(fog.hf_end_g);
	if (fog.bits & svc_fog_data_t::BIT_HEIGHTFOG_END_B)
		gi.WriteByte(fog.hf_end_b);
	if (fog.bits & svc_fog_data_t::BIT_HEIGHTFOG_END_DIST)
		gi.WriteLong(fog.hf_end_dist);

	gi.unicast(ent, true);

	ent->client->fog = ent->client->pers.wanted_fog;
	hf = wanted_hf;
}

/*
===========
InitPlayerTeam
============
*/
bool InitPlayerTeam(gentity_t* ent) {
	// Non-deathmatch (e.g. single-player or coop) - everyone plays
	if (!deathmatch->integer) {
		ent->client->sess.team = Team::Free;
		ent->client->ps.teamID = static_cast<int>(ent->client->sess.team);
		ent->client->ps.stats[STAT_SHOW_STATUSBAR] = 1;
		return true;
	}

	// If we've already been placed on a team, do nothing
	if (ent->client->sess.team != Team::None) {
		return true;
	}

	bool matchLocked = ((level.matchState >= MatchState::Countdown) && match_lock->integer);

	if (!matchLocked) {
		if (ent == host) {
			if (g_owner_auto_join->integer) {
				SetTeam(ent, PickTeam(-1), false, false, false);
				return true;
			}
		}
		else {
			if (match_forceJoin->integer || match_autoJoin->integer) {
				SetTeam(ent, PickTeam(-1), false, false, false);
				return true;
			}
			if ((ent->svFlags & SVF_BOT) || ent->client->sess.is_a_bot) {
				SetTeam(ent, PickTeam(-1), false, false, false);
				return true;
			}
		}
	}

	// Otherwise start as spectator
	ent->client->sess.team = Team::Spectator;
	ent->client->ps.teamID = static_cast<int>(ent->client->sess.team);
	MoveClientToFreeCam(ent);

	if (!ent->client->initialMenu.shown)
		ent->client->initialMenu.delay = level.time + 10_hz;

	return false;
}


/*
=====================
ClientBeginDeathmatch

A client has just connected to the server in
deathmatch mode, so clear everything out before starting them.
=====================
*/
static void ClientBeginDeathmatch(gentity_t* ent) {
	InitGEntity(ent);

	// make sure we have a known default
	ent->svFlags |= SVF_PLAYER;

	InitClientResp(ent->client);

	// locate ent at a spawn point
	ClientSpawn(ent);

	if (level.intermission.time) {
		MoveClientToIntermission(ent);
	}
	else {
		if (!(ent->svFlags & SVF_NOCLIENT)) {
			// send effect
			gi.WriteByte(svc_muzzleflash);
			gi.WriteEntity(ent);
			gi.WriteByte(MZ_LOGIN);
			gi.multicast(ent->s.origin, MULTICAST_PVS, false);
		}
	}

	//gi.LocBroadcast_Print(PRINT_HIGH, "$g_entered_game", ent->client->sess.netName);

	// make sure all view stuff is valid
	ClientEndServerFrame(ent);
}

/*
===============
G_SetLevelEntry
===============
*/
static void G_SetLevelEntry() {
	if (deathmatch->integer)
		return;

	// Hub maps do not track visit order; the next map is treated as a fresh start.
	if (level.campaign.hub_map)
		return;

	LevelEntry* found = nullptr;
	int32_t highest_order = 0;

	// Locate an existing entry for this map (or the first empty slot).
	for (size_t i = 0; i < MAX_LEVELS_PER_UNIT; ++i) {
		LevelEntry* e = &game.levelEntries[i];

		highest_order = std::max(highest_order, e->visit_order);

		const bool name_empty = (e->mapName[0] == '\0');
		if (name_empty || std::strcmp(e->mapName.data(), level.mapName.data()) == 0) {
			found = e;
			break;
		}
	}

	if (!found) {
		gi.Com_PrintFmt("WARNING: more than {} maps in unit, can't track the rest.\n", MAX_LEVELS_PER_UNIT);
		return;
	}

	level.entry = found;
	Q_strlcpy(level.entry->mapName.data(), level.mapName.data(), level.entry->mapName.size());

	// First visit: record long name and bump visit order; optionally refund a life.
	if (level.entry->longMapName[0] == '\0') {
		Q_strlcpy(level.entry->longMapName.data(), level.longName.data(), level.entry->longMapName.size());
		level.entry->visit_order = highest_order + 1;

		if (g_coop_enable_lives->integer) {
			for (auto ec : active_clients()) {
				const int max_lives = g_coop_num_lives->integer + 1;
				ec->client->pers.lives = std::min(max_lives, ec->client->pers.lives + 1);
			}
		}
	}

	// Scan all target_changelevel entities to pre-register potential secret levels.
	for (gentity_t* changelevel = nullptr;
		(changelevel = G_FindByString<&gentity_t::className>(changelevel, "target_changelevel")) != nullptr; ) {

		if (changelevel->map[0] == '\0')
			continue;

		// Skip next-unit markers (e.g. "*unit")
		if (std::strchr(changelevel->map.data(), '*'))
			continue;

		// Start from the map name after an optional '+' segment.
		std::string_view map_sv{ changelevel->map.data() };
		const char* plus = std::strchr(changelevel->map.data(), '+');
		if (plus)
			map_sv = std::string_view{ plus + 1 };

		// Skip end screens
		if (map_sv.find(".cin") != std::string_view::npos ||
			map_sv.find(".pcx") != std::string_view::npos)
			continue;

		// Trim optional spawnpoint suffix (e.g. "map$spawn")
		size_t base_len = map_sv.size();
		if (const char* sp = std::strchr(map_sv.data(), '$'))
			base_len = static_cast<size_t>(sp - map_sv.data());

		// Find or create an entry for this candidate level.
		LevelEntry* slot = nullptr;
		for (size_t i = 0; i < MAX_LEVELS_PER_UNIT; ++i) {
			LevelEntry* e = &game.levelEntries[i];
			const bool empty = (e->mapName[0] == '\0');
			if (empty || std::strncmp(e->mapName.data(), map_sv.data(), base_len) == 0) {
				slot = e;
				break;
			}
		}

		if (!slot) {
			gi.Com_PrintFmt("WARNING: more than {} maps in unit, can't track the rest\n", MAX_LEVELS_PER_UNIT);
			return;
		}

		// Copy the base map name into the slot (bounded).
		const size_t cap = slot->mapName.size() - 1;
		const size_t n = std::min(base_len, cap);
		std::memcpy(slot->mapName.data(), map_sv.data(), n);
		slot->mapName[n] = '\0';
	}
}

/*
=================
ClientIsPlaying
=================
*/
bool ClientIsPlaying(gclient_t* cl) {
	if (!cl) return false;

	if (!deathmatch->integer)
		return true;

	return !(cl->sess.team == Team::None || cl->sess.team == Team::Spectator);
}

/*
=================
BroadcastTeamChange
Let everyone know about a team change
=================
*/
static void BroadcastTeamChange(gentity_t* ent, Team old_team, bool inactive, bool silent) {
	if (!deathmatch->integer || !ent || !ent->client)
		return;
	if (!Game::Has(GameFlags::OneVOne) && ent->client->sess.team == old_team)
		return;
	if (silent)
		return;
	std::string s, t;
	char name[MAX_INFO_VALUE] = {};
	gi.Info_ValueForKey(ent->client->pers.userInfo, "name", name, sizeof(name));
	const std::string_view playerName = name;
	const uint16_t skill = ent->client->sess.skillRating;
	const auto team = ent->client->sess.team;
	switch (team) {
	case Team::Free:
		s = std::format(".{} joined the battle.\n", playerName);
		if (skill > 0.f) {
			t = std::format(".You have joined the game.\nYour Skill Rating: {}", skill);
		}
		else {
			t = ".You have joined the game.";
		}
		break;
	case Team::Spectator:
		if (inactive) {
			s = std::format(".{} is inactive,\nmoved to spectators.\n", playerName);
			t = "You are inactive and have been\nmoved to spectators.";
		}
		else if (Game::Has(GameFlags::OneVOne) && ent->client->sess.matchQueued) {
			s = std::format(".{} is in the queue to play.\n", playerName);
			t = ".You are in the queue to play.";
		}
		else {
			s = std::format(".{} joined the spectators.\n", playerName);
			t = ".You are now spectating.";
		}
		break;
	case Team::Red:
	case Team::Blue: {
		std::string_view teamName = Teams_TeamName(team);
		s = std::format(".{} joined the {} Team.\n", playerName, teamName);
		if (skill > 0.f) {
			t = std::format(".You have joined the {} Team.\nYour Skill Rating: {}", teamName, skill);
		}
		else {
			t = std::format(".You have joined the {} Team.\n", teamName);
		}
		break;
	}
	default:
		break;
	}
	if (!s.empty()) {
		for (auto ec : active_clients()) {
			if (ec == ent || (ec->svFlags & SVF_BOT))
				continue;
			gi.LocClient_Print(ec, PRINT_CENTER, s.c_str());
		}
	}
	if (warmup_doReadyUp->integer && level.matchState == MatchState::Warmup_ReadyUp) {
		BroadcastReadyReminderMessage();
	}
	else if (!t.empty()) {
		std::string msg = std::format("%bind:inven:Toggles Menu%{}", t);
		gi.LocClient_Print(ent, PRINT_CENTER, msg.c_str());
	}
}

bool SetTeam(gentity_t* ent, Team desired_team, bool inactive, bool force, bool silent) {
	if (!ent || !ent->client)
		return false;

	gclient_t* cl = ent->client;
	const Team old_team = cl->sess.team;
	const bool wasPlaying = ClientIsPlaying(cl);
	const bool duel = Game::Has(GameFlags::OneVOne);
	const int clientNum = static_cast<int>(cl - game.clients);

	Team target = desired_team;
	bool requestQueue = duel && desired_team == Team::None;

	if (!deathmatch->integer) {
		target = (desired_team == Team::Spectator) ? Team::Spectator : Team::Free;
	}
	else if (!requestQueue) {
		if (target == Team::None)
			target = PickTeam(clientNum);
		if (!Teams()) {
			if (target != Team::Spectator)
				target = Team::Free;
		}
		else {
			if (target == Team::Free || target == Team::None)
				target = PickTeam(clientNum);
			if (target != Team::Spectator && target != Team::Red && target != Team::Blue)
				target = PickTeam(clientNum);
		}
	}

	bool joinPlaying = (target != Team::Spectator);
	const bool matchLocked = match_lock->integer && level.matchState >= MatchState::Countdown;

	if (joinPlaying && !requestQueue && !force) {
		if (matchLocked && !wasPlaying) {
			if (duel) {
				target = Team::Spectator;
				joinPlaying = false;
				requestQueue = true;
			}
			else {
				if (!silent)
					gi.LocClient_Print(ent, PRINT_HIGH, "The match is locked.\n");
				return false;
			}
		}
	}

	if (joinPlaying && !requestQueue && duel && !force && !wasPlaying) {
		int playingClients = 0;
		for (auto ec : active_clients()) {
			if (ec && ec->client && ClientIsPlaying(ec->client))
				++playingClients;
		}
		if (playingClients >= 2) {
			target = Team::Spectator;
			joinPlaying = false;
			requestQueue = true;
		}
	}

	if (requestQueue)
		target = Team::Spectator;

	const bool queueNow = duel && requestQueue;
	const bool spectatorInactive = (target == Team::Spectator) && inactive;
	const bool changedTeam = (target != old_team);
	const bool changedQueue = (queueNow != cl->sess.matchQueued);
	const bool changedInactive = (spectatorInactive != cl->sess.inactiveStatus);

	if (!changedTeam && !changedQueue && !changedInactive)
		return false;

	const int64_t now = GetCurrentRealTimeMillis();

	if (target == Team::Spectator) {
		if (wasPlaying) {
			CTF_DeadDropFlag(ent);
			Tech_DeadDrop(ent);
			Weapon_Grapple_DoReset(cl);
			cl->sess.playEndRealTime = now;
		}
		cl->sess.team = Team::Spectator;
		cl->ps.teamID = static_cast<int>(cl->sess.team);
		if (changedTeam || changedQueue)
			cl->sess.teamJoinTime = level.time;
		cl->sess.matchQueued = queueNow;
		cl->sess.inactiveStatus = spectatorInactive;
		cl->sess.inactivityWarning = false;
		cl->sess.inactivityTime = 0_sec;
		cl->sess.inGame = false;
		cl->sess.initialised = true;
		cl->pers.readyStatus = false;
		cl->pers.spawned = false;

		cl->buttons = BUTTON_NONE;
		cl->oldButtons = BUTTON_NONE;
		cl->latchedButtons = BUTTON_NONE;

		cl->weapon.fireFinished = 0_ms;
		cl->weapon.thinkTime = 0_ms;
		cl->weapon.fireBuffered = false;
		cl->weapon.pending = nullptr;

		cl->ps.pmove.pmFlags = PMF_NONE;
		cl->ps.pmove.pmTime = 0;
		cl->ps.damageBlend = {};
		cl->ps.screenBlend = {};
		cl->ps.rdFlags = RDF_NONE;

		cl->damage = {};
		cl->kick = {};
		cl->feedback = {};

		cl->respawnMinTime = 0_ms;
		cl->respawnMaxTime = level.time;
		cl->respawn_timeout = 0_ms;
		cl->teamState = {};

		FreeFollower(ent);
		MoveClientToFreeCam(ent);
		FreeClientFollowers(ent);
	}
	else {
		cl->sess.team = target;
		cl->ps.teamID = static_cast<int>(cl->sess.team);
		cl->sess.matchQueued = false;
		cl->sess.inactiveStatus = false;
		cl->sess.inactivityWarning = false;
		cl->sess.inGame = true;
		cl->sess.initialised = true;
		cl->sess.teamJoinTime = level.time;
		cl->pers.readyStatus = false;

		GameTime timeout = GameTime::from_sec(g_inactivity->integer);
		if (timeout && timeout < 15_sec)
			timeout = 15_sec;
		cl->sess.inactivityTime = timeout ? level.time + timeout : 0_sec;

		if (!wasPlaying)
			cl->sess.playStartRealTime = now;
		cl->sess.playEndRealTime = 0;

		cl->buttons = BUTTON_NONE;
		cl->oldButtons = BUTTON_NONE;
		cl->latchedButtons = BUTTON_NONE;

		cl->weapon.fireBuffered = false;
		cl->weapon.pending = nullptr;

		cl->ps.pmove.pmFlags = PMF_NONE;
		cl->ps.pmove.pmTime = 0;

		FreeFollower(ent);
		ClientRespawn(ent);
	}

	BroadcastTeamChange(ent, old_team, spectatorInactive, silent);
	CalculateRanks();
	ClientUpdateFollowers(ent);

	return true;
}



/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the game.  This will happen every level load.
============
*/
void ClientBegin(gentity_t* ent) {
	gclient_t *cl = game.clients + (ent - g_entities - 1);
	cl->awaitingRespawn = false;
	cl->respawn_timeout = 0_ms;

	// set inactivity timer
	GameTime cv = GameTime::from_sec(g_inactivity->integer);
	if (cv) {
		if (cv < 15_sec) cv = 15_sec;
		cl->sess.inactivityTime = level.time + cv;
		cl->sess.inactivityWarning = false;
	}

	// [Paril-KEX] we're always connected by this point...
	cl->pers.connected = true;

	if (deathmatch->integer) {
		ClientBeginDeathmatch(ent);

		// count current clients and rank for scoreboard
		CalculateRanks();
		return;
	}

	// [Paril-KEX] set enter time now, so we can send messages slightly
	// after somebody first joins
	cl->resp.enterTime = level.time;
	cl->pers.spawned = true;

	// if there is already a body waiting for us (a loadgame), just
	// take it, otherwise spawn one from scratch
	if (ent->inUse) {
		// the client has cleared the client side viewAngles upon
		// connecting to the server, which is different than the
		// state when the game is saved, so we need to compensate
		// with deltaangles
		cl->ps.pmove.deltaAngles = cl->ps.viewAngles;
	}
	else {
		// a spawn point will completely reinitialize the entity
		// except for the persistant data that was initialized at
		// ClientConnect() time
		InitGEntity(ent);
		ent->className = "player";
		InitClientResp(cl);
		cl->coopRespawn.spawnBegin = true;
		ClientSpawn(ent);
		cl->coopRespawn.spawnBegin = false;

		if (!cl->sess.inGame)
			BroadcastTeamChange(ent, Team::None, false, false);
	}

	// make sure we have a known default
	ent->svFlags |= SVF_PLAYER;

	if (level.intermission.time) {
		MoveClientToIntermission(ent);
	}
	else {
		// send effect if in a multiplayer game
		if (game.maxClients > 1 && !(ent->svFlags & SVF_NOCLIENT))
			gi.LocBroadcast_Print(PRINT_HIGH, "$g_entered_game", cl->sess.netName);
	}

	level.campaign.coopScalePlayers++;
	G_Monster_CheckCoopHealthScaling();

	// make sure all view stuff is valid
	ClientEndServerFrame(ent);

	// [Paril-KEX] send them goal, if needed
	G_PlayerNotifyGoal(ent);

	// [Paril-KEX] we're going to set this here just to be certain
	// that the level entry timer only starts when a player is actually
	// *in* the level
	G_SetLevelEntry();

	cl->sess.inGame = true;
}

/*
================
P_GetLobbyUserNum
================
*/
unsigned int P_GetLobbyUserNum(const gentity_t* player) {
	unsigned int playerNum = 0;
	if (player > g_entities && player < g_entities + MAX_ENTITIES) {
		playerNum = (player - g_entities) - 1;
		if (playerNum >= MAX_CLIENTS) {
			playerNum = 0;
		}
	}
	return playerNum;
}

/*
================
G_EncodedPlayerName

Gets a token version of the players "name" to be decoded on the client.
================
*/
static std::string G_EncodedPlayerName(gentity_t* player) {
	unsigned int playernum = P_GetLobbyUserNum(player);
	return std::string("##P") + std::to_string(playernum);
}

/*
===========
ClientUserInfoChanged

called whenever the player updates a userInfo variable.
============
*/
void ClientUserinfoChanged(gentity_t* ent, const char* userInfo) {
	char val[MAX_INFO_VALUE] = { 0 };
	char netName[MAX_INFO_VALUE] = { 0 };

	// set name
	if (!gi.Info_ValueForKey(userInfo, "name", netName, sizeof(netName)))
		Q_strlcpy(netName, "badinfo", sizeof(netName));
	Q_strlcpy(ent->client->sess.netName, netName, sizeof(ent->client->sess.netName));

	// set skin
	if (!gi.Info_ValueForKey(userInfo, "skin", val, sizeof(val)))
		Q_strlcpy(val, "male/grunt", sizeof(val));
	//if (Q_strncasecmp(ent->client->sess.skinName, val, sizeof(ent->client->sess.skinName))) {
		//Q_strlcpy(ent->client->sess.skinName, ClientSkinOverride(val), sizeof(ent->client->sess.skinName));
		//ent->client->sess.skinName = ClientSkinOverride(val);
	//}
	std::string iconPath = G_Fmt("/players/{}_i", ent->client->sess.skinName).data();
	ent->client->sess.skinIconIndex = gi.imageIndex(iconPath.c_str());

	int playernum = ent - g_entities - 1;

	// combine name and skin into a configstring
	if (Teams())
		AssignPlayerSkin(ent, val);
	else {
		gi.configString(CS_PLAYERSKINS + playernum, G_Fmt("{}\\{}", ent->client->sess.netName, val).data());
	}

	//  set player name field (used in id_state view)
	gi.configString(CONFIG_CHASE_PLAYER_NAME + playernum, ent->client->sess.netName);

	// [Kex] netName is used for a couple of other things, so we update this after those.
	if (!(ent->svFlags & SVF_BOT))
		Q_strlcpy(ent->client->pers.netName, G_EncodedPlayerName(ent).c_str(), sizeof(ent->client->pers.netName));

	// fov
	gi.Info_ValueForKey(userInfo, "fov", val, sizeof(val));
	ent->client->ps.fov = std::clamp((float)strtoul(val, nullptr, 10), 1.f, 160.f);

	// handedness
	if (gi.Info_ValueForKey(userInfo, "hand", val, sizeof(val))) {
		ent->client->pers.hand = static_cast<Handedness>(std::clamp(atoi(val), static_cast<int>(Handedness::Right), static_cast<int>(Handedness::Center)));
	}
	else {
		ent->client->pers.hand = Handedness::Right;
	}

	// [Paril-KEX] auto-switch
	if (gi.Info_ValueForKey(userInfo, "autoswitch", val, sizeof(val))) {
		ent->client->pers.autoswitch = static_cast<WeaponAutoSwitch>(std::clamp(atoi(val), static_cast<int>(WeaponAutoSwitch::Smart), static_cast<int>(WeaponAutoSwitch::Never)));
	}
	else {
		ent->client->pers.autoswitch = WeaponAutoSwitch::Smart;
	}

	if (gi.Info_ValueForKey(userInfo, "autoshield", val, sizeof(val))) {
		ent->client->pers.autoshield = atoi(val);
	}
	else {
		ent->client->pers.autoshield = -1;
	}

	// [Paril-KEX] wants bob
	if (gi.Info_ValueForKey(userInfo, "bobskip", val, sizeof(val))) {
		ent->client->pers.bob_skip = val[0] == '1';
	}
	else {
		ent->client->pers.bob_skip = false;
	}

	// save off the userInfo in case we want to check something later
	Q_strlcpy(ent->client->pers.userInfo, userInfo, sizeof(ent->client->pers.userInfo));
}

static inline bool IsSlotIgnored(gentity_t* slot, gentity_t** ignore, size_t num_ignore) {
	for (size_t i = 0; i < num_ignore; i++)
		if (slot == ignore[i])
			return true;

	return false;
}

static inline gentity_t* ClientChooseSlot_Any(gentity_t** ignore, size_t num_ignore) {
	for (size_t i = 0; i < game.maxClients; i++)
		if (!IsSlotIgnored(globals.gentities + i + 1, ignore, num_ignore) && !game.clients[i].pers.connected)
			return globals.gentities + i + 1;

	return nullptr;
}

static inline gentity_t* ClientChooseSlot_Coop(const char* userInfo, const char* socialID, bool isBot, gentity_t** ignore, size_t num_ignore) {
	char name[MAX_INFO_VALUE] = { 0 };
	gi.Info_ValueForKey(userInfo, "name", name, sizeof(name));

	// the host should always occupy slot 0, some systems rely on this
	// (CHECK: is this true? is it just bots?)
	{
		size_t num_players = 0;

		for (size_t i = 0; i < game.maxClients; i++)
			if (IsSlotIgnored(globals.gentities + i + 1, ignore, num_ignore) || game.clients[i].pers.connected)
				num_players++;

		if (!num_players) {
			gi.Com_PrintFmt("coop slot {} is host {}+{}\n", 1, name, socialID);
			return globals.gentities + 1;
		}
	}

	// grab matches from players that we have connected
	using match_type_t = int32_t;
	enum {
		SLOT_MATCH_USERNAME,
		SLOT_MATCH_SOCIAL,
		SLOT_MATCH_BOTH,

		SLOT_MATCH_TYPES
	};

	struct {
		gentity_t* slot = nullptr;
		size_t total = 0;
	} matches[SLOT_MATCH_TYPES];

	for (size_t i = 0; i < game.maxClients; i++) {
		if (IsSlotIgnored(globals.gentities + i + 1, ignore, num_ignore) || game.clients[i].pers.connected)
			continue;

		char check_name[MAX_INFO_VALUE] = { 0 };
		gi.Info_ValueForKey(game.clients[i].pers.userInfo, "name", check_name, sizeof(check_name));

		bool username_match = game.clients[i].pers.userInfo[0] &&
			!strcmp(check_name, name);

		bool social_match = socialID && game.clients[i].sess.socialID && game.clients[i].sess.socialID[0] &&
			!strcmp(game.clients[i].sess.socialID, socialID);

		match_type_t type = (match_type_t)0;

		if (username_match)
			type |= SLOT_MATCH_USERNAME;
		if (social_match)
			type |= SLOT_MATCH_SOCIAL;

		if (!type)
			continue;

		matches[type].slot = globals.gentities + i + 1;
		matches[type].total++;
	}

	// pick matches in descending order, only if the total matches
	// is 1 in the particular set; this will prefer to pick
	// social+username matches first, then social, then username last.
	for (int32_t i = 2; i >= 0; i--) {
		if (matches[i].total == 1) {
			gi.Com_PrintFmt("coop slot {} restored for {}+{}\n", (ptrdiff_t)(matches[i].slot - globals.gentities), name, socialID);

			// spawn us a ghost now since we're gonna spawn eventually
			if (!matches[i].slot->inUse) {
				matches[i].slot->s.modelIndex = MODELINDEX_PLAYER;
				matches[i].slot->solid = SOLID_BBOX;

				InitGEntity(matches[i].slot);
				matches[i].slot->className = "player";
				InitClientResp(matches[i].slot->client);
				matches[i].slot->client->coopRespawn.spawnBegin = true;
				ClientSpawn(matches[i].slot);
				matches[i].slot->client->coopRespawn.spawnBegin = false;

				// make sure we have a known default
				matches[i].slot->svFlags |= SVF_PLAYER;

				matches[i].slot->sv.init = false;
				matches[i].slot->className = "player";
				matches[i].slot->client->pers.connected = true;
				matches[i].slot->client->pers.spawned = true;
				P_AssignClientSkinNum(matches[i].slot);
				gi.linkEntity(matches[i].slot);
			}

			return matches[i].slot;
		}
	}

	// in the case where we can't find a match, we're probably a new
	// player, so pick a slot that hasn't been occupied yet
	for (size_t i = 0; i < game.maxClients; i++)
		if (!IsSlotIgnored(globals.gentities + i + 1, ignore, num_ignore) && !game.clients[i].pers.userInfo[0]) {
			gi.Com_PrintFmt("coop slot {} issuing new for {}+{}\n", i + 1, name, socialID);
			return globals.gentities + i + 1;
		}

	// all slots have some player data in them, we're forced to replace one.
	gentity_t* any_slot = ClientChooseSlot_Any(ignore, num_ignore);

	gi.Com_PrintFmt("coop slot {} any slot for {}+{}\n", !any_slot ? -1 : (ptrdiff_t)(any_slot - globals.gentities), name, socialID);

	return any_slot;
}

// [Paril-KEX] for coop, we want to try to ensure that players will always get their
// proper slot back when they connect.
gentity_t* ClientChooseSlot(const char* userInfo, const char* socialID, bool isBot, gentity_t** ignore, size_t num_ignore, bool cinematic) {
	// coop and non-bots is the only thing that we need to do special behavior on
	if (!cinematic && coop->integer && !isBot)
		return ClientChooseSlot_Coop(userInfo, socialID, isBot, ignore, num_ignore);

	// just find any free slot
	return ClientChooseSlot_Any(ignore, num_ignore);
}

static inline bool CheckBanned(gentity_t* ent, char* userInfo, const char* socialID) {
	// currently all bans are in Steamworks and Epic, don't bother if not from there
	if (socialID[0] != 'S' && socialID[0] != 'E')
		return false;

	// Israel
	if (!Q_strcasecmp(socialID, "Steamworks-76561198026297488")) {
		gi.Info_SetValueForKey(userInfo, "rejmsg", "Antisemite detected!\n");

		if (host && host->client) {
			if (level.time > host->client->lastBannedMessageTime + 10_sec) {

				char name[MAX_INFO_VALUE] = { 0 };
				gi.Info_ValueForKey(userInfo, "name", name, sizeof(name));

				gi.LocClient_Print(host, PRINT_TTS, "ANTISEMITE DETECTED ({})!\n", name);
				host->client->lastBannedMessageTime = level.time;
				gi.LocBroadcast_Print(PRINT_CHAT, "{}: God Bless Palestine\n", name);
			}
		}

		gi.localSound(ent, CHAN_AUTO, gi.soundIndex("world/klaxon3.wav"), 1, ATTN_NONE, 0);
		gi.AddCommandString(G_Fmt("kick {}\n", ent - g_entities - 1).data());
		return true;
	}

	// Kirlomax
	if (!Q_strcasecmp(socialID, "Steamworks-76561198001774610")) {
		gi.Info_SetValueForKey(userInfo, "rejmsg", "WARNING! KNOWN CHEATER DETECTED\n");

		if (host && host->client) {
			if (level.time > host->client->lastBannedMessageTime + 10_sec) {

				char name[MAX_INFO_VALUE] = { 0 };
				gi.Info_ValueForKey(userInfo, "name", name, sizeof(name));

				gi.LocClient_Print(host, PRINT_TTS, "WARNING! KNOWN CHEATER DETECTED ({})!\n", name);
				host->client->lastBannedMessageTime = level.time;
				gi.LocBroadcast_Print(PRINT_CHAT, "{}: I am a known cheater, banned from all servers.\n", name);
			}
		}

		gi.localSound(ent, CHAN_AUTO, gi.soundIndex("world/klaxon3.wav"), 1, ATTN_NONE, 0);
		gi.AddCommandString(G_Fmt("kick {}\n", ent - g_entities - 1).data());
		return true;
	}

	// Model192
	if (!Q_strcasecmp(socialID, "Steamworks-76561197972296343")) {
		gi.Info_SetValueForKey(userInfo, "rejmsg", "WARNING! MOANERTONE DETECTED\n");

		if (host && host->client) {
			if (level.time > host->client->lastBannedMessageTime + 10_sec) {

				char name[MAX_INFO_VALUE] = { 0 };
				gi.Info_ValueForKey(userInfo, "name", name, sizeof(name));

				gi.LocClient_Print(host, PRINT_TTS, "WARNING! MOANERTONE DETECTED ({})!\n", name);
				host->client->lastBannedMessageTime = level.time;
				gi.LocBroadcast_Print(PRINT_CHAT, "{}: Listen up, I have something to moan about.\n", name);
			}
		}

		gi.localSound(ent, CHAN_AUTO, gi.soundIndex("world/klaxon3.wav"), 1, ATTN_NONE, 0);
		gi.AddCommandString(G_Fmt("kick {}\n", ent - g_entities - 1).data());
		return true;
	}

	// Dalude
	if (!Q_strcasecmp(socialID, "Steamworks-76561199001991246") || !Q_strcasecmp(socialID, "EOS-07e230c273be4248bbf26c89033923c1")) {
		ent->client->sess.is_888 = true;
		gi.Info_SetValueForKey(userInfo, "rejmsg", "Fake 888 Agent detected!\n");
		gi.Info_SetValueForKey(userInfo, "name", "Fake 888 Agent");

		if (host && host->client) {
			if (level.time > host->client->lastBannedMessageTime + 10_sec) {

				char name[MAX_INFO_VALUE] = { 0 };
				gi.Info_ValueForKey(userInfo, "name", name, sizeof(name));

				gi.LocClient_Print(host, PRINT_TTS, "FAKE 888 AGENT DETECTED ({})!\n", name);
				host->client->lastBannedMessageTime = level.time;
				gi.LocBroadcast_Print(PRINT_CHAT, "{}: bejesus, what a lovely lobby! certainly better than 888's!\n", name);
			}
		}
		gi.localSound(ent, CHAN_AUTO, gi.soundIndex("world/klaxon3.wav"), 1, ATTN_NONE, 0);
		gi.AddCommandString(G_Fmt("kick {}\n", ent - g_entities - 1).data());
		return true;
	}
	return false;
}

/*
================
ClientCheckPermissions
================
*/
static void ClientCheckPermissions(gentity_t* ent, std::string socialID) {
	if (socialID.empty())
		return;

	std::string id(socialID);

	ent->client->sess.banned = game.bannedIDs.contains(id);
	ent->client->sess.admin = game.adminIDs.contains(id);
}

/*
===========
ClientConnect

Called when a player begins connecting to the server.
The game can refuse entrance to a client by returning false.
If the client is allowed, the connection process will continue
and eventually get to ClientBegin()
Changing levels will NOT cause this to be called again, but
loadgames will.
============
*/
bool ClientConnect(gentity_t* ent, char* userInfo, const char* socialID, bool isBot) {
	if (!isBot) {
		if (CheckBanned(ent, userInfo, socialID))
			return false;

		ClientCheckPermissions(ent, socialID);
	}

	ent->client->sess.team = deathmatch->integer ? Team::None : Team::Free;

	// they can connect
	ent->client = game.clients + (ent - g_entities - 1);

	// set up userInfo early
	ClientUserinfoChanged(ent, userInfo);

	// if there is already a body waiting for us (a loadgame), just
	// take it, otherwise spawn one from scratch
	if (ent->inUse == false) {
		// clear the respawning variables

		if (!ent->client->sess.initialised && ent->client->sess.team == Team::None) {
			ent->client->pers.introTime = 3_sec;

			// force team join
			ent->client->sess.team = deathmatch->integer ? Team::None : Team::Free;
			ent->client->sess.pc = {};

			InitClientResp(ent->client);

			ent->client->sess.playStartRealTime = GetCurrentRealTimeMillis();
		}

		if (!game.autoSaved || !ent->client->pers.weapon)
			InitClientPersistant(ent, ent->client);
	}

	// make sure we start with known default(s)
	ent->svFlags = SVF_PLAYER;

	if (isBot) {
		ent->svFlags |= SVF_BOT;
		ent->client->sess.is_a_bot = true;

		if (bot_name_prefix->string[0] && *bot_name_prefix->string) {
			std::array<char, MAX_NETNAME> oldName = {};
			std::array<char, MAX_NETNAME> newName = {};

			gi.Info_ValueForKey(userInfo, "name", oldName.data(), oldName.size());
			strncpy(newName.data(), bot_name_prefix->string, newName.size());
			Q_strlcat(newName.data(), oldName.data(), oldName.size());
			gi.Info_SetValueForKey(userInfo, "name", newName.data());
		}
	}

	Q_strlcpy(ent->client->sess.socialID, socialID, sizeof(ent->client->sess.socialID));

	std::array<char, MAX_INFO_VALUE> value = {};
	// [Paril-KEX] fetch name because now netName is kinda unsuitable
	gi.Info_ValueForKey(userInfo, "name", value.data(), value.size());
	Q_strlcpy(ent->client->sess.netName, value.data(), sizeof(ent->client->sess.netName));

	ent->client->sess.skillRating = 0;

	if (!isBot) {
		ClientConfig_Init(ent->client, ent->client->sess.socialID, value.data(), Game::GetCurrentInfo().short_name_upper.data());

		if (ent->client->sess.banned) {
			gi.LocBroadcast_Print(PRINT_HIGH, "BANNED PLAYER {} connects.\n", value.data());
			gi.AddCommandString(G_Fmt("kick {}\n", ent - g_entities - 1).data());
			return false;
		}

		if (ent->client->sess.skillRating > 0) {
			gi.LocBroadcast_Print(PRINT_HIGH, "{} connects. (SR: {})\n", value.data(), ent->client->sess.skillRating);
		}
		else {
			gi.LocBroadcast_Print(PRINT_HIGH, "$g_player_connected", value.data());
		}

		// entity 1 is always server host, so make admin
		if (ent == &g_entities[1])
			ent->client->sess.admin = true;

		// Detect if client is on a console system
		if (socialID && (
			_strnicmp(socialID, "PSN", 3) == 0 ||
			_strnicmp(socialID, "NX", 2) == 0 ||
			_strnicmp(socialID, "GDK", 3) == 0
			)) {
			ent->client->sess.consolePlayer = true;
		}
		else {
			ent->client->sess.consolePlayer = false;
		}
	}

	if (level.endmatch_grace)
		level.endmatch_grace = 0_ms;

	// set skin
	std::array<char, MAX_INFO_VALUE> val = {};
	if (!gi.Info_ValueForKey(userInfo, "skin", val.data(), sizeof(val)))
		Q_strlcpy(val.data(), "male/grunt", val.size());
	if (Q_strncasecmp(ent->client->sess.skinName, val.data(), sizeof(ent->client->sess.skinName))) {
		Q_strlcpy(ent->client->sess.skinName, ClientSkinOverride(val.data()), sizeof(ent->client->sess.skinName));
		ent->client->sess.skinIconIndex = gi.imageIndex(G_Fmt("/players/{}_i", ent->client->sess.skinName).data());
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	ent->client->pers.connected = true;

	ent->client->sess.inGame = true;

	// [Paril-KEX] force a state update
	ent->sv.init = false;

	return true;
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.
============
*/
void ClientDisconnect(gentity_t* ent) {
	if (!ent->client)
		return;

	// make sure no trackers are still hurting us.
	if (ent->client->trackerPainTime)
		RemoveAttackingPainDaemons(ent);

	if (ent->client->ownedSphere) {
		if (ent->client->ownedSphere->inUse)
			FreeEntity(ent->client->ownedSphere);
		ent->client->ownedSphere = nullptr;
	}

	PlayerTrail_Destroy(ent);

	if (!(ent->svFlags & SVF_NOCLIENT)) {
		TossClientItems(ent);

		// send effect
		gi.WriteByte(svc_muzzleflash);
		gi.WriteEntity(ent);
		gi.WriteByte(MZ_LOGOUT);
		gi.multicast(ent->s.origin, MULTICAST_PVS, false);
	}

	if (ent->client->pers.connected && ent->client->sess.initialised && !ent->client->sess.is_a_bot)
		if (ent->client->sess.netName && ent->client->sess.netName[0])
			gi.LocBroadcast_Print(PRINT_HIGH, "{} disconnected.", ent->client->sess.netName);

	// free any followers
	FreeClientFollowers(ent);

	G_RevertVote(ent->client);

	P_SaveGhostSlot(ent);

	gi.unlinkEntity(ent);
	ent->s.modelIndex = 0;
	ent->solid = SOLID_NOT;
	ent->inUse = false;
	ent->sv.init = false;
	ent->className = "disconnected";
	ent->client->pers.connected = false;
	ent->client->pers.spawned = false;
	ent->timeStamp = level.time + 1_sec;

	if (ent->client->pers.spawned)
		ClientConfig_SaveStats(ent->client, false);

	// update active scoreboards
	if (deathmatch->integer) {
		CalculateRanks();

		for (auto ec : active_clients())
			if (ec->client->showScores)
				ec->client->menu.updateTime = level.time;
	}
}

//==============================================================

static trace_t G_PM_Clip(const Vector3& start, const Vector3* mins, const Vector3* maxs, const Vector3& end, contents_t mask) {
	return gi.game_import_t::clip(world, start, mins, maxs, end, mask);
}

bool G_ShouldPlayersCollide(bool weaponry) {
	if (g_disable_player_collision->integer)
		return false; // only for debugging.

	// always collide on dm
	if (!CooperativeModeOn())
		return true;

	// weaponry collides if friendly fire is enabled
	if (weaponry && g_friendlyFireScale->integer > 0.0)
		return true;

	// check collision cvar
	return g_coop_player_collision->integer;
}

/*
=================
P_FallingDamage

Paril-KEX: this is moved here and now reacts directly
to ClientThink rather than being delayed.
=================
*/
static void P_FallingDamage(gentity_t* ent, const PMove& pm) {
	int	   damage;
	Vector3 dir;

	// dead stuff can't crater
	if (ent->health <= 0 || ent->deadFlag)
		return;

	if (ent->s.modelIndex != MODELINDEX_PLAYER)
		return; // not in the player model

	if (ent->moveType == MoveType::NoClip || ent->moveType == MoveType::FreeCam)
		return;

	// never take falling damage if completely underwater
	if (pm.waterLevel == WATER_UNDER)
		return;

	//  never take damage if just release grapple or on grapple
	if (ent->client->grapple.releaseTime >= level.time ||
		(ent->client->grapple.entity &&
			ent->client->grapple.state > GrappleState::Fly))
		return;

	float delta = pm.impactDelta;

	delta = delta * delta * 0.0001f;

	if (pm.waterLevel == WATER_WAIST)
		delta *= 0.25f;
	if (pm.waterLevel == WATER_FEET)
		delta *= 0.5f;

	if (delta < 1.0f)
		return;

	// restart footstep timer
	ent->client->feedback.bobTime = 0;

	if (ent->client->landmark_free_fall) {
		delta = min(30.f, delta);
		ent->client->landmark_free_fall = false;
		ent->client->landmark_noise_time = level.time + 100_ms;
	}

	if (delta < 15) {
		if (!(pm.s.pmFlags & PMF_ON_LADDER))
			ent->s.event = EV_FOOTSTEP;
		return;
	}

	ent->client->feedback.fallValue = delta * 0.5f;
	if (ent->client->feedback.fallValue > 40)
		ent->client->feedback.fallValue = 40;
	ent->client->feedback.fallTime = level.time + FALL_TIME();

	int med_min = RS(RS_Q3A) ? 40 : 30;
	int far_min = RS(RS_Q3A) ? 61 : 55;

	if (delta > med_min) {
		if (delta >= far_min)
			ent->s.event = EV_FALL_FAR;
		else
			ent->s.event = EV_FALL_MEDIUM;
		if (g_fallingDamage->integer && !Game::Has(GameFlags::Arena)) {
			ent->pain_debounce_time = level.time + FRAME_TIME_S; // no normal pain sound
			if (RS(RS_Q3A))
				damage = ent->s.event == EV_FALL_FAR ? 10 : 5;
			else {
				damage = (int)((delta - 30) / 3);	// 2 originally
				if (damage < 1)
					damage = 1;
			}
			dir = { 0, 0, 1 };

			Damage(ent, world, world, dir, ent->s.origin, vec3_origin, damage, 0, DamageFlags::Normal, ModID::FallDamage);
		}
	}
	else
		ent->s.event = EV_FALL_SHORT;

	// Paril: falling damage noises alert monsters
	if (ent->health)
		G_PlayerNoise(ent, pm.s.origin, PlayerNoise::Self);
}

inline void PreviousMenuItem(gentity_t* ent);
inline void NextMenuItem(gentity_t* ent);
static bool HandleMenuMovement(gentity_t* ent, usercmd_t* ucmd) {
	if (!ent->client->menu.current)
		return false;

	// [Paril-KEX] handle menu movement
	int32_t menu_sign = ucmd->forwardMove > 0 ? 1 : ucmd->forwardMove < 0 ? -1 : 0;

	if (ent->client->menu_sign != menu_sign) {
		ent->client->menu_sign = menu_sign;

		if (menu_sign > 0) {
			PreviousMenuItem(ent);
			return true;
		}
		else if (menu_sign < 0) {
			NextMenuItem(ent);
			return true;
		}
	}

	if (ent->client->latchedButtons & (BUTTON_ATTACK | BUTTON_JUMP)) {
		ActivateSelectedMenuItem(ent);
		return true;
	}

	return false;
}

/*
=================
ClientInactivityTimer

Returns false if the client is dropped
=================
*/
static bool ClientInactivityTimer(gentity_t* ent) {
	if (!ent || !ent->client)
		return true;

	auto* cl = ent->client;

	// Check if inactivity is enabled
	GameTime timeout = GameTime::from_sec(g_inactivity->integer);
	if (timeout && timeout < 15_sec)
		timeout = 15_sec;

	// First-time setup
	if (!cl->sess.inactivityTime) {
		cl->sess.inactivityTime = level.time + timeout;
		cl->sess.inactivityWarning = false;
		cl->sess.inactiveStatus = false;
		return true;
	}

	// Reset conditions (ineligible for inactivity logic)
	if (!deathmatch->integer || !timeout || !ClientIsPlaying(cl) || cl->eliminated || cl->sess.is_a_bot || ent->s.number == 0) {
		cl->sess.inactivityTime = level.time + 1_min;
		cl->sess.inactivityWarning = false;
		cl->sess.inactiveStatus = false;
		return true;
	}

	// Input activity detected, reset timer
	if (cl->latchedButtons & BUTTON_ANY) {
		cl->sess.inactivityTime = level.time + timeout;
		cl->sess.inactivityWarning = false;
		cl->sess.inactiveStatus = false;
		return true;
	}

	// Timeout reached, remove player
	if (level.time > cl->sess.inactivityTime) {
		gi.LocClient_Print(ent, PRINT_CENTER, "You have been removed from the match\ndue to inactivity.\n");
		SetTeam(ent, Team::Spectator, true, true, false);
		return false;
	}

	// Warning 10 seconds before timeout
	if (!cl->sess.inactivityWarning && level.time > cl->sess.inactivityTime - 10_sec) {
		cl->sess.inactivityWarning = true;
		gi.LocClient_Print(ent, PRINT_CENTER, "Ten seconds until inactivity trigger!\n");
		gi.localSound(ent, CHAN_AUTO, gi.soundIndex("world/fish.wav"), 1, ATTN_NONE, 0);
	}

	return true;
}

static void ClientTimerActions_ApplyRegeneration(gentity_t* ent) {
	gclient_t* cl = ent->client;
	if (!cl)
		return;

	if (ent->health <= 0 || ent->client->eliminated)
		return;

	if (cl->powerupTime.regeneration <= level.time)
		return;

	if (g_vampiric_damage->integer || !game.map.spawnHealth)
		return;

	if (CombatIsDisabled())
		return;

	bool	noise = false;
	float	volume = cl->powerupTime.silencerShots ? 0.2f : 1.0;
	int		max = cl->pers.maxHealth;
	int		bonus = 0;

	if (ent->health < max) {
		bonus = 15;
	}
	else if (ent->health < max * 2) {
		bonus = 5;
	}

	if (!bonus)
		return;

	ent->health += bonus;
	if (ent->health > max)
		ent->health = max;
	gi.sound(ent, CHAN_AUX, gi.soundIndex("items/regen.wav"), volume, ATTN_NORM, 0);
	cl->pu_regen_time_blip = level.time + 100_ms;
}

/*
==================
ClientTimerActions

Actions that happen once a second
==================
*/
static void ClientTimerActions(gentity_t* ent) {
	if (ent->client->timeResidual > level.time)
		return;

	if (RS(RS_Q3A)) {
		// count down health when over max
		if (ent->health > ent->client->pers.maxHealth)
			ent->health--;

		// count down armor when over max
		if (ent->client->pers.inventory[IT_ARMOR_COMBAT] > ent->client->pers.maxHealth)
			ent->client->pers.inventory[IT_ARMOR_COMBAT]--;
	}
	else {
		if (ent->client->pers.healthBonus > 0) {
			if (ent->health <= 0 || ent->health <= ent->client->pers.maxHealth) {
				ent->client->pers.healthBonus = 0;
			}
			else {
				ent->health--;
				ent->client->pers.healthBonus--;
			}
		}
	}
	ClientTimerActions_ApplyRegeneration(ent);
	ent->client->timeResidual = level.time + 1_sec;
}

/*
=========================
PrintModifierIntro

Displays the intro text for the active game modifier.
Only one modifier should be active at a time.
=========================
*/
static void PrintModifierIntro(gentity_t* ent) {
	if (!ent || !ent->client)
		return;

	if (g_quadhog->integer) {
		gi.LocClient_Print(ent, PRINT_CENTER,
			".QUAD HOG\nHold onto the Quad Damage and become the hog!");
	}
	else if (g_vampiric_damage->integer) {
		gi.LocClient_Print(ent, PRINT_CENTER,
			".VAMPIRIC DAMAGE\nDeal damage to heal yourself. Blood is fuel.");
	}
	else if (g_frenzy->integer) {
		gi.LocClient_Print(ent, PRINT_CENTER,
			".WEAPONS FRENZY\nFaster fire, faster rockets, infinite ammo regen.");
	}
	else if (g_nadeFest->integer) {
		gi.LocClient_Print(ent, PRINT_CENTER,
			".NADE FEST\nIt’s raining grenades!");
	}
	else if (g_instaGib->integer) {
		gi.LocClient_Print(ent, PRINT_CENTER,
			".INSTAGIB\nIt’s a raily good time!");
	}
}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame.
==============
*/
void OpenJoinMenu(gentity_t* ent);
void ClientThink(gentity_t* ent, usercmd_t* ucmd) {
	gclient_t* cl;
	gentity_t* other;
	PMove		pm;

	level.currentEntity = ent;
	cl = ent->client;

	//no movement during map or match intermission
	if (level.timeoutActive > 0_ms) {
		cl->resp.cmdAngles[PITCH] = ucmd->angles[PITCH];
		cl->resp.cmdAngles[YAW] = ucmd->angles[YAW];
		cl->resp.cmdAngles[ROLL] = ucmd->angles[ROLL];
		cl->ps.pmove.pmType = PM_FREEZE;
		return;
	}

	// [Paril-KEX] pass buttons through even if we are in intermission or
	// chasing.
	cl->oldButtons = cl->buttons;
	cl->buttons = ucmd->buttons;
	cl->latchedButtons |= cl->buttons & ~cl->oldButtons;
	cl->cmd = *ucmd;

	if (!cl->initialMenu.shown && cl->initialMenu.delay && level.time > cl->initialMenu.delay) {
		if (!ClientIsPlaying(cl) && (!cl->sess.initialised || cl->sess.inactiveStatus)) {
			if (ent == host) {
				if (!g_autoScreenshotTool->integer) {
					if (g_owner_push_scores->integer)
						Commands::Score(ent, CommandArgs{});
					else OpenJoinMenu(ent);
				}
			}
			else
				OpenJoinMenu(ent);
			//if (!cl->initialMenu.shown)
			//	gi.LocClient_Print(ent, PRINT_CHAT, "Welcome to {} v{}.\n", GAMEMOD_TITLE, GAMEMOD_VERSION);
			cl->initialMenu.delay = 0_sec;
			cl->initialMenu.shown = true;
		}
	}

	// check for queued follow targets
	if (!ClientIsPlaying(cl)) {
		if (cl->follow.queuedTarget && level.time > cl->follow.queuedTime + 500_ms) {
			cl->follow.target = cl->follow.queuedTarget;
			cl->follow.update = true;
			cl->follow.queuedTarget = nullptr;
			cl->follow.queuedTime = 0_sec;
			ClientUpdateFollowers(ent);
		}
	}

	// check for inactivity timer
	if (!ClientInactivityTimer(ent))
		return;

	if (g_quadhog->integer)
		if (cl->powerupTime.quadDamage > 0_sec && level.time >= cl->powerupTime.quadDamage)
			QuadHog_SetupSpawn(0_ms);

	if (cl->sess.teamJoinTime) {
		GameTime delay = 5_sec;
		if (cl->sess.motdModificationCount != game.motdModificationCount) {
			if (level.time >= cl->sess.teamJoinTime + delay) {
				if (g_showmotd->integer && game.motd.size()) {
					gi.LocCenter_Print(ent, "{}", game.motd.c_str());
					delay += 5_sec;
					cl->sess.motdModificationCount = game.motdModificationCount;
				}
			}
		}
		if (!cl->sess.showed_help && g_showhelp->integer) {
			if (level.time >= cl->sess.teamJoinTime + delay) {
				PrintModifierIntro(ent);

				cl->sess.showed_help = true;
			}
		}
	}

	if ((ucmd->buttons & BUTTON_CROUCH) && pm_config.n64Physics) {
		if (cl->pers.n64_crouch_warn_times < 12 &&
			cl->pers.n64_crouch_warning < level.time &&
			(++cl->pers.n64_crouch_warn_times % 3) == 0) {
			cl->pers.n64_crouch_warning = level.time + 10_sec;
			gi.LocClient_Print(ent, PRINT_CENTER, "$g_n64_crouching");
		}
	}

	if (level.intermission.time || cl->awaitingRespawn) {
		cl->ps.pmove.pmType = PM_FREEZE;

		bool n64_sp = false;

		if (level.intermission.time) {
			n64_sp = !deathmatch->integer && level.isN64;

			// can exit intermission after five seconds
			// Paril: except in N64. the camera handles it.
			// Paril again: except on unit exits, we can leave immediately after camera finishes
			if (!level.changeMap.empty() && (!n64_sp || level.intermission.set) && level.time > level.intermission.time + 5_sec && (ucmd->buttons & BUTTON_ANY))
				level.intermission.postIntermission = true;
		}

		if (!n64_sp)
			cl->ps.pmove.viewHeight = ent->viewHeight = DEFAULT_VIEWHEIGHT;
		else
			cl->ps.pmove.viewHeight = ent->viewHeight = 0;
		ent->moveType = MoveType::FreeCam;
		return;
	}

	if (cl->follow.target) {
		cl->resp.cmdAngles = ucmd->angles;
		ent->moveType = MoveType::FreeCam;
	}
	else {

		// set up for pmove
		memset(&pm, 0, sizeof(pm));

		if (ent->moveType == MoveType::FreeCam) {
			if (cl->menu.current) {
				cl->ps.pmove.pmType = PM_FREEZE;

				// [Paril-KEX] handle menu movement
				HandleMenuMovement(ent, ucmd);
			}
			else if (cl->awaitingRespawn)
				cl->ps.pmove.pmType = PM_FREEZE;
			else if (!ClientIsPlaying(ent->client) || cl->eliminated)
				cl->ps.pmove.pmType = PM_SPECTATOR;
			else
				cl->ps.pmove.pmType = PM_NOCLIP;
		}
		else if (ent->moveType == MoveType::NoClip) {
			cl->ps.pmove.pmType = PM_NOCLIP;
		}
		else if (ent->s.modelIndex != MODELINDEX_PLAYER)
			cl->ps.pmove.pmType = PM_GIB;
		else if (ent->deadFlag)
			cl->ps.pmove.pmType = PM_DEAD;
		else if (cl->grapple.state >= GrappleState::Pull)
			cl->ps.pmove.pmType = PM_GRAPPLE;
		else
			cl->ps.pmove.pmType = PM_NORMAL;

		// [Paril-KEX]
		if (!G_ShouldPlayersCollide(false) ||
			(CooperativeModeOn() && !(ent->clipMask & CONTENTS_PLAYER)) // if player collision is on and we're temporarily ghostly...
			)
			cl->ps.pmove.pmFlags |= PMF_IGNORE_PLAYER_COLLISION;
		else
			cl->ps.pmove.pmFlags &= ~PMF_IGNORE_PLAYER_COLLISION;

		// haste support
		cl->ps.pmove.haste = cl->powerupTime.haste > level.time;

		// trigger_gravity support
		cl->ps.pmove.gravity = (short)(level.gravity * ent->gravity);
		if (cl->powerupTime.antiGravBelt > level.time)
			cl->ps.pmove.gravity *= 0.25f;
		pm.s = cl->ps.pmove;

		pm.s.origin = ent->s.origin;
		pm.s.velocity = ent->velocity;

		if (memcmp(&cl->old_pmove, &pm.s, sizeof(pm.s)))
			pm.snapInitial = true;

		pm.cmd = *ucmd;
		pm.player = ent;
		pm.trace = gi.game_import_t::trace;
		pm.clip = G_PM_Clip;
		pm.pointContents = gi.pointContents;
		pm.viewOffset = cl->ps.viewOffset;

		// perform a pmove
		Pmove(&pm);

		if (pm.groundEntity && ent->groundEntity) {
			float stepsize = fabs(ent->s.origin[Z] - pm.s.origin[Z]);

			if (stepsize > 4.f && stepsize < (ent->s.origin[Z] < 0 ? STEPSIZE_BELOW : STEPSIZE)) {
				ent->s.renderFX |= RF_STAIR_STEP;
				cl->stepFrame = gi.ServerFrame() + 1;
			}
		}

		P_FallingDamage(ent, pm);

		if (cl->landmark_free_fall && pm.groundEntity) {
			cl->landmark_free_fall = false;
			cl->landmark_noise_time = level.time + 100_ms;
		}

		// [Paril-KEX] save old position for G_TouchProjectiles
		Vector3 oldOrigin = ent->s.origin;

		ent->s.origin = pm.s.origin;
		ent->velocity = pm.s.velocity;

		// [Paril-KEX] if we stepped onto/off of a ladder, reset the
		// last ladder pos
		if ((pm.s.pmFlags & PMF_ON_LADDER) != (cl->ps.pmove.pmFlags & PMF_ON_LADDER)) {
			cl->last_ladder_pos = ent->s.origin;

			if (pm.s.pmFlags & PMF_ON_LADDER) {
				if (!deathmatch->integer &&
					cl->last_ladder_sound < level.time) {
					ent->s.event = EV_LADDER_STEP;
					cl->last_ladder_sound = level.time + LADDER_SOUND_TIME;
				}
			}
		}

		// save results of pmove
		cl->ps.pmove = pm.s;
		cl->old_pmove = pm.s;

		ent->mins = pm.mins;
		ent->maxs = pm.maxs;

		if (!cl->menu.current)
			cl->resp.cmdAngles = ucmd->angles;

		if (pm.jumpSound && !(pm.s.pmFlags & PMF_ON_LADDER)) {
			gi.sound(ent, CHAN_VOICE, gi.soundIndex("*jump1.wav"), 1, ATTN_NORM, 0);
		}

		// sam raimi cam support
		if (ent->flags & FL_SAM_RAIMI)
			ent->viewHeight = 8;
		else
			ent->viewHeight = (int)pm.s.viewHeight;

		ent->waterLevel = pm.waterLevel;
		ent->waterType = pm.waterType;
		ent->groundEntity = pm.groundEntity;
		if (pm.groundEntity)
			ent->groundEntity_linkCount = pm.groundEntity->linkCount;

		if (ent->deadFlag) {
			cl->ps.viewAngles[ROLL] = 40;
			cl->ps.viewAngles[PITCH] = -15;
			cl->ps.viewAngles[YAW] = cl->killerYaw;
		}
		else if (!cl->menu.current) {
			cl->vAngle = pm.viewAngles;
			cl->ps.viewAngles = pm.viewAngles;
			AngleVectors(cl->vAngle, cl->vForward, nullptr, nullptr);
		}

		if (cl->grapple.entity)
			Weapon_Grapple_Pull(cl->grapple.entity);

		gi.linkEntity(ent);

		ent->gravity = 1.0;

		if (ent->moveType != MoveType::NoClip) {
			TouchTriggers(ent);
			if (ent->moveType != MoveType::FreeCam)
				G_TouchProjectiles(ent, oldOrigin);
		}

		// touch other objects
		for (size_t i = 0; i < pm.touch.num; i++) {
			trace_t& tr = pm.touch.traces[i];
			other = tr.ent;

			if (other->touch)
				other->touch(other, ent, tr, true);
		}
	}

	// fire weapon from final position if needed
	if (cl->latchedButtons & BUTTON_ATTACK) {
		if (!ClientIsPlaying(cl) || (cl->eliminated && !cl->sess.is_a_bot)) {
			cl->latchedButtons = BUTTON_NONE;

			if (cl->follow.target) {
				FreeFollower(ent);
			}
			else
				GetFollowTarget(ent);
		}
		else if (!cl->weapon.thunk) {
			// we can only do this during a ready state and
			// if enough time has passed from last fire
			if (cl->weaponState == WeaponState::Ready && !CombatIsDisabled()) {
				cl->weapon.fireBuffered = true;

				if (cl->weapon.fireFinished <= level.time) {
					cl->weapon.thunk = true;
					Think_Weapon(ent);
				}
			}
		}
	}

	if (!ClientIsPlaying(cl) || (cl->eliminated && !cl->sess.is_a_bot)) {
		if (!HandleMenuMovement(ent, ucmd)) {
			if (ucmd->buttons & BUTTON_JUMP) {
				if (!(cl->ps.pmove.pmFlags & PMF_JUMP_HELD)) {
					cl->ps.pmove.pmFlags |= PMF_JUMP_HELD;
					if (cl->follow.target)
						FollowNext(ent);
					else
						GetFollowTarget(ent);
				}
			}
			else
				cl->ps.pmove.pmFlags &= ~PMF_JUMP_HELD;
		}
	}

	// update followers if being followed
	for (auto ec : active_clients())
		if (ec->client->follow.target == ent)
			ClientUpdateFollowers(ec);

	// perform once-a-second actions
	ClientTimerActions(ent);
}

// active monsters
struct active_monsters_filter_t {
	inline bool operator()(gentity_t* ent) const {
		return (ent->inUse && (ent->svFlags & SVF_MONSTER) && ent->health > 0);
	}
};

static inline entity_iterable_t<active_monsters_filter_t> active_monsters() {
	return entity_iterable_t<active_monsters_filter_t> { game.maxClients + (uint32_t)BODY_QUEUE_SIZE + 1U };
}

static inline bool G_MonstersSearchingFor(gentity_t* player) {
	for (auto ent : active_monsters()) {
		// check for *any* player target
		if (player == nullptr && ent->enemy && !ent->enemy->client)
			continue;
		// they're not targeting us, so who cares
		else if (player != nullptr && ent->enemy != player)
			continue;
		
		// they lost sight of us
		if ((ent->monsterInfo.aiFlags & AI_LOST_SIGHT) && level.time > ent->monsterInfo.trailTime + 5_sec)
			continue;

		// no sir
		return true;
	}

	// yes sir
	return false;
}

/*
===============
G_FindRespawnSpot

Attempts to find a valid respawn spot near the given player.
Returns true and fills `spot` if successful.
===============
*/
static inline bool G_FindRespawnSpot(gentity_t* player, Vector3& spot) {
	constexpr float yawOffsets[] = { 0, 90, 45, -45, -90 };
	constexpr float backDistance = 128.0f;
	constexpr float upDistance = 128.0f;
	constexpr float viewHeight = static_cast<float>(DEFAULT_VIEWHEIGHT);
	constexpr contents_t solidMask = MASK_PLAYERSOLID | CONTENTS_LAVA | CONTENTS_SLIME;

	// Sanity check: make sure player isn't already stuck
	if (gi.trace(player->s.origin, PLAYER_MINS, PLAYER_MAXS, player->s.origin, player, MASK_PLAYERSOLID).startSolid)
		return false;

	for (float yawOffset : yawOffsets) {
		Vector3 yawAngles = { 0, player->s.angles[YAW] + 180.0f + yawOffset, 0 };

		// Step 1: Try moving up first
		Vector3 start = player->s.origin;
		Vector3 end = start + Vector3{ 0, 0, upDistance };
		trace_t tr = gi.trace(start, PLAYER_MINS, PLAYER_MAXS, end, player, solidMask);
		if (tr.startSolid || tr.allSolid || (tr.contents & (CONTENTS_LAVA | CONTENTS_SLIME)))
			continue;

		// Step 2: Then move backwards from that elevated point
		Vector3 forward;
		AngleVectors(yawAngles, forward, nullptr, nullptr);
		start = tr.endPos;
		end = start + forward * backDistance;
		tr = gi.trace(start, PLAYER_MINS, PLAYER_MAXS, end, player, solidMask);
		if (tr.startSolid || tr.allSolid || (tr.contents & (CONTENTS_LAVA | CONTENTS_SLIME)))
			continue;

		// Step 3: Now cast downward to find solid ground
		start = tr.endPos;
		end = start - Vector3{ 0, 0, upDistance * 4 };
		tr = gi.trace(start, PLAYER_MINS, PLAYER_MAXS, end, player, solidMask);
		if (tr.startSolid || tr.allSolid || tr.fraction == 1.0f || tr.ent != world || tr.plane.normal.z < 0.7f)
			continue;

		// Avoid liquids
		if (gi.pointContents(tr.endPos + Vector3{ 0, 0, viewHeight }) & MASK_WATER)
			continue;

		// Height delta check
		float zDelta = std::fabs(player->s.origin[Z] - tr.endPos[2]);
		float stepLimit = (player->s.origin[Z] < 0 ? STEPSIZE_BELOW : STEPSIZE);
		if (zDelta > stepLimit * 4.0f)
			continue;

		// If stepped up/down, ensure visibility
		if (zDelta > stepLimit) {
			if (gi.traceLine(player->s.origin, tr.endPos, player, solidMask).fraction != 1.0f)
				continue;
			if (gi.traceLine(player->s.origin + Vector3{ 0, 0, viewHeight }, tr.endPos + Vector3{ 0, 0, viewHeight }, player, solidMask).fraction != 1.0f)
				continue;
		}

		spot = tr.endPos;
		return true;
	}

	return false;
}

/*
==============================
G_FindSquadRespawnTarget

Scans for a valid living player who is not in combat or danger
and has a suitable spawn spot nearby. Returns the player and spot.
==============================
*/
static inline std::tuple<gentity_t*, Vector3> G_FindSquadRespawnTarget() {
	const bool anyMonstersSearching = G_MonstersSearchingFor(nullptr);

	for (gentity_t* player : active_clients()) {
		auto* cl = player->client;

		// Skip invalid candidates
		if (player->deadFlag)
			continue;

		using enum CoopRespawn;

		if (cl->last_damage_time >= level.time) {
			cl->coopRespawnState = InCombat;
			continue;
		}
		if (G_MonstersSearchingFor(player)) {
			cl->coopRespawnState = InCombat;
			continue;
		}
		if (anyMonstersSearching && cl->lastFiringTime >= level.time) {
			cl->coopRespawnState = InCombat;
			continue;
		}
		if (player->groundEntity != world) {
			cl->coopRespawnState = BadArea;
			continue;
		}
		if (player->waterLevel >= WATER_UNDER) {
			cl->coopRespawnState = BadArea;
			continue;
		}

		Vector3 spot;
		if (!G_FindRespawnSpot(player, spot)) {
			cl->coopRespawnState = Blocked;
			continue;
		}

		return { player, spot };
	}

	return { nullptr, vec3_origin };
}

enum respawn_state_t {
	RESPAWN_NONE,     // invalid state
	RESPAWN_SPECTATE, // move to spectator
	RESPAWN_SQUAD,    // move to good squad point
	RESPAWN_START     // move to start of map
};

// [Paril-KEX] return false to fall back to click-to-respawn behavior.
// note that this is only called if they are allowed to respawn (not
// restarting the level due to all being dead)
static bool G_CoopRespawn(gentity_t* ent) {
	// don't do this in non-coop
	if (!CooperativeModeOn())
		return false;
	// if we don't have squad or lives, it doesn't matter
	if (!g_coop_squad_respawn->integer && !g_coop_enable_lives->integer)
		return false;

	respawn_state_t state = RESPAWN_NONE;

	// first pass: if we have no lives left, just move to spectator
	if (g_coop_enable_lives->integer) {
		if (ent->client->pers.lives == 0) {
			state = RESPAWN_SPECTATE;
			ent->client->coopRespawnState = CoopRespawn::NoLives;
		}
	}

	// second pass: check for where to spawn
	if (state == RESPAWN_NONE) {
		// if squad respawn, don't respawn until we can find a good player to spawn on.
		if (coop->integer && g_coop_squad_respawn->integer) {
			bool allDead = true;

			for (auto player : active_clients()) {
				if (player->health > 0) {
					allDead = false;
					break;
				}
			}

			// all dead, so if we ever get here we have lives enabled;
			// we should just respawn at the start of the level
			if (allDead)
				state = RESPAWN_START;
			else {
				auto [good_player, good_spot] = G_FindSquadRespawnTarget();

				if (good_player) {
					state = RESPAWN_SQUAD;

					ent->client->coopRespawn.squadOrigin = good_spot;
					ent->client->coopRespawn.squadAngles = good_player->s.angles;
					ent->client->coopRespawn.squadAngles[ROLL] = 0;

					ent->client->coopRespawn.useSquad = true;
				}
				else {
					state = RESPAWN_SPECTATE;
				}
			}
		}
		else
			state = RESPAWN_START;
	}

	if (state == RESPAWN_SQUAD || state == RESPAWN_START) {
		// give us our max health back since it will reset
		// to pers.health; in instanced items we'd lose the items
		// we touched so we always want to respawn with our max.
		if (P_UseCoopInstancedItems())
			ent->client->pers.health = ent->client->pers.maxHealth = ent->maxHealth;

		ClientRespawn(ent);

		ent->client->latchedButtons = BUTTON_NONE;
		ent->client->coopRespawn.useSquad = false;
	}
	else if (state == RESPAWN_SPECTATE) {
		if (!static_cast<int>(ent->client->coopRespawnState))
			ent->client->coopRespawnState = CoopRespawn::Waiting;

		if (ClientIsPlaying(ent->client)) {
			// move us to spectate just so we don't have to twiddle
			// our thumbs forever
			CopyToBodyQue(ent);
			ent->client->sess.team = Team::Spectator;
			MoveClientToFreeCam(ent);
			gi.linkEntity(ent);
			GetFollowTarget(ent);
		}
	}

	return true;
}

/*
==============
ClientBeginServerFrame

This will be called once for each server frame, before running
any other entities in the world.
==============
*/
inline void ActivateSelectedMenuItem(gentity_t* ent);
void ClientBeginServerFrame(gentity_t* ent) {
	gclient_t* client;

	if (gi.ServerFrame() != ent->client->stepFrame)
		ent->s.renderFX &= ~RF_STAIR_STEP;

	if (level.intermission.time)
		return;

	client = ent->client;

	if (client->awaitingRespawn) {
		if ((level.time.milliseconds() % 500) == 0)
			ClientSpawn(ent);
		return;
	}

	if ((ent->svFlags & SVF_BOT) != 0) {
		Bot_BeginFrame(ent);
	}

	// run weapon animations if it hasn't been done by a ucmd_t
	if (!client->weapon.thunk && ClientIsPlaying(client) && !client->eliminated)
		Think_Weapon(ent);
	else
		client->weapon.thunk = false;

	if (ent->client->menu.current) {
		if ((client->latchedButtons & BUTTON_ATTACK)) {
			ActivateSelectedMenuItem(ent);
			client->latchedButtons = BUTTON_NONE;
		}
		return;
	}
	else if (ent->deadFlag) {
		//wor: add minimum delay in dm
		if (deathmatch->integer && client->respawnMinTime && level.time > client->respawnMinTime && level.time <= client->respawnMaxTime && !level.intermission.queued) {
			if ((client->latchedButtons & BUTTON_ATTACK)) {
				ClientRespawn(ent);
				client->latchedButtons = BUTTON_NONE;
			}
		}
		else if (level.time > client->respawnMaxTime && !level.campaign.coopLevelRestartTime) {
			// don't respawn if level is waiting to restart
			// check for coop handling
			if (!G_CoopRespawn(ent)) {
				// in deathmatch, only wait for attack button
				if ((client->latchedButtons & (deathmatch->integer ? BUTTON_ATTACK : -1)) ||
					(deathmatch->integer && match_doForceRespawn->integer)) {
					ClientRespawn(ent);
					client->latchedButtons = BUTTON_NONE;
				}
			}
		}
		return;
	}

	// add player trail so monsters can follow
	if (!deathmatch->integer)
		PlayerTrail_Add(ent);

	client->latchedButtons = BUTTON_NONE;
}
/*
==============
RemoveAttackingPainDaemons

This is called to clean up the pain daemons that the disruptor attaches
to clients to damage them.
==============
*/
void RemoveAttackingPainDaemons(gentity_t* self) {
	gentity_t* tracker = G_FindByString<&gentity_t::className>(nullptr, "pain daemon");

	while (tracker) {
		if (tracker->enemy == self)
			FreeEntity(tracker);
		tracker = G_FindByString<&gentity_t::className>(tracker, "pain daemon");
	}

	if (self->client)
		self->client->trackerPainTime = 0_ms;
}

