// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
//#include "menu/Menu.h"
#include "g_local.h"
//#include "fmt/format.h"
#include "monsters/m_player.h"
#include "bots/bot_includes.h"

void ClientConfig_Init(gclient_t *cl, const std::string &playerID, const std::string &playerName, const std::string &gameType);

static THINK(info_player_start_drop) (gentity_t *self) -> void {
	// allow them to drop
	self->solid = SOLID_TRIGGER;
	self->moveType = MOVETYPE_TOSS;
	self->mins = PLAYER_MINS;
	self->maxs = PLAYER_MAXS;
	gi.linkentity(self);
}

static inline void deathmatch_spawn_flags(gentity_t *self) {
	if (st.noBots)
		self->flags = FL_NO_BOTS;
	if (st.noHumans)
		self->flags = FL_NO_HUMANS;
	if (st.arena)
		self->arena = st.arena;
}

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
The normal starting point for a level.

"noBots" will prevent bots from using this spot.
"noHumans" will prevent humans from using this spot.
*/
void SP_info_player_start(gentity_t *self) {
	// fix stuck spawn points
	if (gi.trace(self->s.origin, PLAYER_MINS, PLAYER_MAXS, self->s.origin, self, MASK_SOLID).startsolid)
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
void SP_info_player_deathmatch(gentity_t *self) {
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
void SP_info_player_team_red(gentity_t *self) {}

/*QUAKED info_player_team_blue (0 0 1) (-16 -16 -24) (16 16 32) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
A potential Blue Team spawning position for CTF games.
*/
void SP_info_player_team_blue(gentity_t *self) {}

/*QUAKED info_player_coop (1 0 1) (-16 -16 -24) (16 16 32) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
A potential spawning position for coop games.
*/
void SP_info_player_coop(gentity_t *self) {
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
void SP_info_player_coop_lava(gentity_t *self) {
	if (!coop->integer) {
		FreeEntity(self);
		return;
	}

	// fix stuck spawn points
	if (gi.trace(self->s.origin, PLAYER_MINS, PLAYER_MAXS, self->s.origin, self, MASK_SOLID).startsolid)
		G_FixStuckObject(self, self->s.origin);
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
The deathmatch intermission point will be at one of these
Use 'angles' instead of 'angle', so you can set pitch or roll as well as yaw.  'pitch yaw roll'
*/
void SP_info_player_intermission(gentity_t *ent) {}

/*QUAKED info_ctf_teleport_destination (0.5 0.5 0.5) (-16 -16 -24) (16 16 32) x x x x x x x x NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP
Point trigger_teleports at these.
*/
void SP_info_ctf_teleport_destination(gentity_t *ent) {
	ent->s.origin[2] += 16;
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
void PushAward(gentity_t *ent, PlayerMedal medal) {
	if (!ent || !ent->client) return;
	constexpr int MAX_QUEUED_AWARDS = 8;

	struct MedalInfo {
		std::string_view soundKeyFirst;
		std::string_view soundKeyRepeat;
	};
	static constexpr std::array<MedalInfo, static_cast<std::size_t>(PlayerMedal::MEDAL_TOTAL)> medalTable = { {
		{ "", "" },                    // MEDAL_NONE
		{ "first_excellent", "excellent1" },
		{ "", "humiliation1" },
		{ "", "impressive1" },
		{ "", "rampage1" },
		{ "", "first_frag" },
		{ "", "defense1" },
		{ "", "assist1" },
		{ "", "" },                    // MEDAL_CAPTURES
		{ "", "holy_shit" }
	} };

	auto &cl = *ent->client;
	auto idx = static_cast<std::size_t>(medal);
	auto &info = medalTable[idx];

	cl.pers.medalTime = level.time;
	cl.pers.medalType = medal;

	auto &count = cl.pers.match.medalCount[idx];
	++count;

	std::string_view key = (count == 1 && !info.soundKeyFirst.empty())
		? info.soundKeyFirst
		: info.soundKeyRepeat;

	if (!key.empty()) {
		const std::string path = G_Fmt("vo/{}.wav", key).data();
		const int soundIdx = gi.soundindex(path.c_str());

		auto &queue = cl.pers.awardQueue;
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
void P_SaveGhostSlot(gentity_t *ent) {
	//TODO: don't do this if less than 1 minute played

	if (!ent || !ent->client)
		return;

	if (ent == host)
		return;

	gclient_t *cl = ent->client;

	if (!cl)
		return;

	if (level.matchState != MatchState::MATCH_IN_PROGRESS)
		return;

	const char *socialID = cl->sess.socialID;
	if (!socialID || !*socialID)
		return;

	// Find existing ghost slot or first free one
	Ghosts *slot = nullptr;
	for (auto &g : level.ghosts) {
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
	Q_strncasecmp(slot->netName, cl->sess.netName, sizeof(slot->netName));
	Q_strncasecmp(slot->socialID, socialID, sizeof(slot->socialID));

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
}

/*
===============
P_RestoreFromGhostSlot
===============
*/
void P_RestoreFromGhostSlot(gentity_t *ent) {
	if (!ent || !ent->client)
		return;

	gclient_t *cl = ent->client;

	if (!cl->sess.socialID || !*cl->sess.socialID)
		return;

	const char *socialID = cl->sess.socialID;

	for (auto &g : level.ghosts) {
		if (Q_strcasecmp(g.socialID, socialID) != 0)
			continue;

		// Restore inventory and stats
		cl->pers.inventory = g.inventory;
		cl->pers.ammoMax = g.ammoMax;
		cl->pers.match = g.match;
		cl->pers.weapon = g.weapon;
		cl->pers.lastWeapon = g.lastWeapon;
		cl->sess.team = g.team;
		cl->resp.score = g.score;
		cl->sess.skillRating = g.skillRating;
		cl->sess.skillRatingChange = g.skillRatingChange;

		gi.Client_Print(ent, PRINT_HIGH, "Your game state has been restored.\n");

		// Clear the ghost slot
		g = Ghosts{};
		return;
	}
}

//=======================================================================

static const char *ClientSkinOverride(const char *s) {
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
	for (auto &[m, skins] : stockSkins) {
		if (m == model) {
			// 4a) If the skin is known, no change
			if (std::find(skins.begin(), skins.end(), skin) != skins.end()) {
				return s;
			}
			// 4b) Otherwise revert to this model's default skin
			auto &defaultSkin = skins.front();
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
static void PCfg_ClientInitPConfig(gentity_t *ent) {
	bool	file_exists = false;
	bool	cfg_valid = true;
	
	if (!ent->client) return;
	if (ent->svFlags & SVF_BOT) return;

	// load up file
	const char *name = G_Fmt("baseq2/pcfg/{}.cfg", ent->client->sess.socialID).data();

	FILE *f = fopen(name, "rb");
	if (f != NULL) {
		char *buffer = nullptr;
		size_t length;
		size_t read_length;

		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);

		if (length > 0x40000) {
			cfg_valid = false;
		}
		if (cfg_valid) {
			buffer = (char *)gi.TagMalloc(length + 1, '\0');
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
			const char *str = G_Fmt("// {}'s Player Config\n// Generated by WOR\n", ent->client->sess.netName).data();

			fwrite(str, 1, strlen(str), f);
			gi.Com_PrintFmt("{}: Player config written to: \"{}\"\n", __FUNCTION__, name);
			fclose(f);
		} else {
			gi.Com_PrintFmt("{}: Cannot save player config: {}\n", __FUNCTION__, name);
		}
	} else {
		//gi.Com_PrintFmt("{}: Player config not saved as file already exists: \"{}\"\n", __FUNCTION__, name);
	}
}

//=======================================================================
struct mon_name_t {
	const char *className;
	const char *longname;
};
mon_name_t monname[] = {
	{ "monster_arachnid", "Arachnid" },
	{ "monster_berserk", "Berserker" },
	{ "monster_boss2", "Hornet" },
	{ "monster_boss5", "Super Tank" },
	{ "monster_brain", "Brains" },
	{ "monster_carrier", "Carrier" },
	{ "monster_chick", "Iron Maiden" },
	{ "monster_chick_heat", "Iron Maiden" },
	{ "monster_daedalus", "Daedalus" },
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
	{ "monster_hover", "Icarus" },
	{ "monster_infantry", "Infantry" },
	{ "monster_jorg", "Jorg" },
	{ "monster_kamikaze", "Kamikaze" },
	{ "monster_makron", "Makron" },
	{ "monster_medic", "Medic" },
	{ "monster_medic_commander", "Medic Commander" },
	{ "monster_mutant", "Mutant" },
	{ "monster_parasite", "Parasite" },
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
	{ "monster_turret", "Turret" },
	{ "monster_widow", "Black Widow" },
	{ "monster_widow2", "Black Widow" },
};

static const char *MonsterName(const char *className) {
	for (size_t i = 0; i < ARRAY_LEN(monname); i++) {
		if (!Q_strncasecmp(className, monname[i].className, strlen(className)))
			return monname[i].longname;
	}
	return nullptr;
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

static void ClientObituary(gentity_t *victim, gentity_t *inflictor, gentity_t *attacker, mod_t mod) {
	std::string base{};

	if (!victim || !victim->client)
		return;

	if (attacker && CooperativeModeOn() && attacker->client)
		mod.friendly_fire = true;

	if (mod.id == MOD_CHANGE_TEAM)
		return;

	int killStreakCount = victim->client->killStreakCount;
	victim->client->killStreakCount = 0;

	switch (mod.id) {
	case MOD_SUICIDE:
		base = "{} suicides.\n";
		break;
	case MOD_EXPIRE:
		base = "{} ran out of blood.\n";
		break;
	case MOD_FALLING:
		base = "{} cratered.\n";
		break;
	case MOD_CRUSH:
		base = "{} was squished.\n";
		break;
	case MOD_WATER:
		base = "{} sank like a rock.\n";
		break;
	case MOD_SLIME:
		base = "{} melted.\n";
		break;
	case MOD_LAVA:
		base = "{} does a back flip into the lava.\n";
		break;
	case MOD_EXPLOSIVE:
	case MOD_BARREL:
		base = "{} blew up.\n";
		break;
	case MOD_EXIT:
		base = "{} found a way out.\n";
		break;
	case MOD_TARGET_LASER:
		base = "{} saw the light.\n";
		break;
	case MOD_TARGET_BLASTER:
		base = "{} got blasted.\n";
		break;
	case MOD_BOMB:
	case MOD_SPLASH:
	case MOD_TRIGGER_HURT:
		base = "{} was in the wrong place.\n";
		break;
	default:
		//base = nullptr;
		break;
	}

	if (base.empty() && attacker == victim) {
		switch (mod.id) {
		case MOD_HELD_GRENADE:
			base = "{} tried to put the pin back in.\n";
			break;
		case MOD_HANDGRENADE_SPLASH:
		case MOD_GRENADE_SPLASH:
			base = "{} tripped on their own grenade.\n";
			break;
		case MOD_ROCKET_SPLASH:
			base = "{} blew themselves up.\n";
			break;
		case MOD_BFG_BLAST:
			base = "{} should have used a smaller gun.\n";
			break;
		case MOD_TRAP:
			base = "{} was sucked into their own trap.\n";
			break;
		case MOD_DOPPEL_EXPLODE:
			base = "{} was fooled by their own doppelganger.\n";
			break;
		case MOD_EXPIRE:
			base = "{} ran out of blood.\n";
			break;
		case MOD_TESLA:
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
		const char *monname = MonsterName(attacker->className);

		if (monname) {
			std::string message = fmt::format("{} was killed by a {}.\n", victim->client->sess.netName, monname);
			gi.LocBroadcast_Print(PRINT_MEDIUM, message.c_str());

			G_LogEvent(message);
			victim->enemy = nullptr;
			return;
		}
		return;
	}

	if (!attacker->client)
		return;
	switch (mod.id) {
	case MOD_BLASTER:
		base = "{} was blasted by {}.\n";
		break;
	case MOD_SHOTGUN:
		base = "{} was gunned down by {}.\n";
		break;
	case MOD_SSHOTGUN:
		base = "{} was blown away by {}'s Super Shotgun.\n";
		break;
	case MOD_MACHINEGUN:
		base = "{} was machinegunned by {}.\n";
		break;
	case MOD_CHAINGUN:
		base = "{} was cut in half by {}'s Chaingun.\n";
		break;
	case MOD_GRENADE:
		base = "{} was popped by {}'s grenade.\n";
		break;
	case MOD_GRENADE_SPLASH:
		base = "{} was shredded by {}'s shrapnel.\n";
		break;
	case MOD_ROCKET:
		base = "{} ate {}'s rocket.\n";
		break;
	case MOD_ROCKET_SPLASH:
		base = "{} almost dodged {}'s rocket.\n";
		break;
	case MOD_HYPERBLASTER:
		base = "{} was melted by {}'s HyperBlaster.\n";
		break;
	case MOD_RAILGUN:
		base = "{} was railed by {}.\n";
		break;
	case MOD_BFG_LASER:
		base = "{} saw the pretty lights from {}'s BFG.\n";
		break;
	case MOD_BFG_BLAST:
		base = "{} was disintegrated by {}'s BFG blast.\n";
		break;
	case MOD_BFG_EFFECT:
		base = "{} couldn't hide from {}'s BFG.\n";
		break;
	case MOD_HANDGRENADE:
		base = "{} caught {}'s handgrenade.\n";
		break;
	case MOD_HANDGRENADE_SPLASH:
		base = "{} didn't see {}'s handgrenade.\n";
		break;
	case MOD_HELD_GRENADE:
		base = "{} feels {}'s pain.\n";
		break;
	case MOD_TELEFRAG:
	case MOD_TELEFRAG_SPAWN:
		base = "{} tried to invade {}'s personal space.\n";
		break;
	case MOD_RIPPER:
		base = "{} ripped to shreds by {}'s ripper gun.\n";
		break;
	case MOD_PHALANX:
		base = "{} was evaporated by {}.\n";
		break;
	case MOD_TRAP:
		base = "{} was caught in {}'s trap.\n";
		break;
	case MOD_CHAINFIST:
		base = "{} was shredded by {}'s ripsaw.\n";
		break;
	case MOD_DISRUPTOR:
		base = "{} lost his grip courtesy of {}'s Disintegrator.\n";
		break;
	case MOD_ETF_RIFLE:
		base = "{} was perforated by {}.\n";
		break;
	case MOD_PLASMABEAM:
		base = "{} was scorched by {}'s Plasma Beam.\n";
		break;
	case MOD_TESLA:
		base = "{} was enlightened by {}'s tesla mine.\n";
		break;
	case MOD_PROX:
		base = "{} got too close to {}'s proximity mine.\n";
		break;
	case MOD_NUKE:
		base = "{} was nuked by {}'s antimatter bomb.\n";
		break;
	case MOD_VENGEANCE_SPHERE:
		base = "{} was purged by {}'s Vengeance Sphere.\n";
		break;
	case MOD_DEFENDER_SPHERE:
		base = "{} had a blast with {}'s Defender Sphere.\n";
		break;
	case MOD_HUNTER_SPHERE:
		base = "{} was hunted down by {}'s Hunter Sphere.\n";
		break;
	case MOD_TRACKER:
		base = "{} was annihilated by {}'s Disruptor.\n";
		break;
	case MOD_DOPPEL_EXPLODE:
		base = "{} was tricked by {}'s Doppelganger.\n";
		break;
	case MOD_DOPPEL_VENGEANCE:
		base = "{} was purged by {}'s Doppelganger.\n";
		break;
	case MOD_DOPPEL_HUNTER:
		base = "{} was hunted down by {}'s Doppelganger.\n";
		break;
	case MOD_GRAPPLE:
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
		if (mod.id == MOD_TELEFRAG_SPAWN &&
				victim->client->resp.ctf_state < 2 &&
				victim->client->sess.team == attacker->client->sess.team) {
			victim->client->resp.ctf_state = 0;
			return;
		}
	}

	// frag messages
	if (deathmatch->integer && victim != attacker && victim->client && attacker->client) {
		if (!(victim->svFlags & SVF_BOT)) {
			if (level.matchState == MatchState::MATCH_WARMUP_READYUP) {
				BroadcastReadyReminderMessage();
			} else {
				if (GTF(GTF_ROUNDS) && GTF(GTF_ELIMINATION) && level.roundState == RoundState::ROUND_IN_PROGRESS) {
					gi.LocClient_Print(victim, PRINT_CENTER, ".You were fragged by {}\nYou will respawn next round.", attacker->client->sess.netName);
				} else if (GT(GT_FREEZE) && level.roundState == RoundState::ROUND_IN_PROGRESS) {
					bool last_standing = true;
					if (victim->client->sess.team == TEAM_RED && level.pop.num_living_red > 1 ||
						victim->client->sess.team == TEAM_BLUE && level.pop.num_living_blue > 1)
						last_standing = false;
					gi.LocClient_Print(victim, PRINT_CENTER, ".You were frozen by {}{}",
						attacker->client->sess.netName,
						last_standing ? "" : "\nYou will respawn once thawed.");
				} else {
					gi.LocClient_Print(victim, PRINT_CENTER, ".You were {} by {}", GT(GT_FREEZE) ? "frozen" : "fragged", attacker->client->sess.netName);
				}
			}
		}
		if (!(attacker->svFlags & SVF_BOT)) {
			if (Teams() && OnSameTeam(victim, attacker)) {
				gi.LocClient_Print(attacker, PRINT_CENTER, ".You fragged {}, your team mate :(", victim->client->sess.netName);
			} else {
				if (level.matchState == MatchState::MATCH_WARMUP_READYUP) {
					BroadcastReadyReminderMessage();
				} else if (attacker->client->killStreakCount && !(attacker->client->killStreakCount % 10)) {
					gi.LocBroadcast_Print(PRINT_CENTER, ".{} is on a rampage\nwith {} frags!", attacker->client->sess.netName, attacker->client->killStreakCount);
					PushAward(attacker, PlayerMedal::MEDAL_RAMPAGE);
				} else if (killStreakCount >= 10) {
					gi.LocBroadcast_Print(PRINT_CENTER, ".{} put an end to {}'s\nrampage!", attacker->client->sess.netName, victim->client->sess.netName);
				} else if (Teams() || level.matchState != MatchState::MATCH_IN_PROGRESS) {
					if (attacker->client->sess.pc.show_fragmessages)
						gi.LocClient_Print(attacker, PRINT_CENTER, ".You {} {}", GT(GT_FREEZE) ? "froze" : "fragged", victim->client->sess.netName);
				} else {
					if (attacker->client->sess.pc.show_fragmessages)
						gi.LocClient_Print(attacker, PRINT_CENTER, ".You {} {}\n{} place with {}", GT(GT_FREEZE) ? "froze" : "fragged",
							victim->client->sess.netName, PlaceString(attacker->client->pers.currentRank + 1), attacker->client->resp.score);
				}
			}
			if (attacker->client->sess.pc.killbeep_num > 0 && attacker->client->sess.pc.killbeep_num < 5) {
				const char *sb[5] = { "", "nav_editor/select_node.wav", "misc/comp_up.wav", "insane/insane7.wav", "nav_editor/finish_node_move.wav" };
				gi.local_sound(attacker, CHAN_AUTO, gi.soundindex(sb[attacker->client->sess.pc.killbeep_num]), 1, ATTN_NONE, 0);
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
static void TossClientItems(gentity_t *self) {
	if (!deathmatch->integer)
		return;

	if (GTF(GTF_ARENA))
		return;

	if (!ClientIsPlaying(self->client))
		return;

	if (!self->client->sess.initialised)
		return;

	// don't drop anything when combat is disabled
	if (CombatIsDisabled())
		return;

	Item *wp;
	gentity_t *drop;
	bool	quad, doubled, haste, protection, invis, regen;
	
	if (RS(RS_Q1)) {
		Drop_Backpack(self);
	} else {
		// drop weapon
		wp = self->client->pers.weapon;
		if (wp) {
			if (g_instagib->integer)
				wp = nullptr;
			else if (g_nadefest->integer)
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
				drop->spawnflags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
				drop->spawnflags &= ~SPAWNFLAG_ITEM_DROPPED;
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
		drop->spawnflags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
		drop->spawnflags &= ~SPAWNFLAG_ITEM_DROPPED;
		drop->svFlags &= ~SVF_INSTANCED;

		drop->touch = Touch_Item;
		drop->nextThink = self->client->powerupTime.quadDamage;
		drop->think = g_quadhog->integer ? QuadHog_DoReset : FreeEntity;

		if (g_quadhog->integer) {
			drop->s.renderfx |= RF_SHELL_BLUE;
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
		drop->spawnflags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
		drop->spawnflags &= ~SPAWNFLAG_ITEM_DROPPED;
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
		drop->spawnflags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
		drop->spawnflags &= ~SPAWNFLAG_ITEM_DROPPED;
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
		drop->spawnflags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
		drop->spawnflags &= ~SPAWNFLAG_ITEM_DROPPED;
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
		drop->spawnflags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
		drop->spawnflags &= ~SPAWNFLAG_ITEM_DROPPED;
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
		drop->spawnflags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
		drop->spawnflags &= ~SPAWNFLAG_ITEM_DROPPED;
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
void LookAtKiller(gentity_t *self, gentity_t *inflictor, gentity_t *attacker) {
	vec3_t dir;

	if (attacker && attacker != world && attacker != self) {
		dir = attacker->s.origin - self->s.origin;
	} else if (inflictor && inflictor != world && inflictor != self) {
		dir = inflictor->s.origin - self->s.origin;
	} else {
		self->client->killer_yaw = self->s.angles[YAW];
		return;
	}

	// PMM - fixed to correct for pitch of 0
	if (dir[0])
		self->client->killer_yaw = 180 / PIf * atan2f(dir[1], dir[0]);
	else if (dir[1] > 0)
		self->client->killer_yaw = 90;
	else if (dir[1] < 0)
		self->client->killer_yaw = 270;
	else
		self->client->killer_yaw = 0;
}

/*
================
Match_CanScore
================
*/
static bool Match_CanScore() {
	if (level.intermissionQueued)
		return false;

	switch (level.matchState) {
	case MatchState::MATCH_WARMUP_DELAYED:
	case MatchState::MATCH_WARMUP_DEFAULT:
	case MatchState::MATCH_WARMUP_READYUP:
	case MatchState::MATCH_COUNTDOWN:
	case MatchState::MATCH_ENDED:
		return false;
	}

	return true;
}

/*
==================
G_LogDeathEvent
==================
*/
static void G_LogDeathEvent(gentity_t *victim, gentity_t *attacker, mod_t mod) {
	match_death_event_t ev;

	if (level.matchState != MatchState::MATCH_IN_PROGRESS) {
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
	} else {
		ev.attacker.name = "Environment";
		ev.attacker.id = "0";
	}
	ev.mod = mod;

	try {
		level.match.deathLog.push_back(std::move(ev));
	} catch (const std::exception &e) {
		gi.Com_ErrorFmt("deathLog push_back failed: {}", e.what());
	}
}

/*
==================
PushDeathStats
==================
*/
static void PushDeathStats(gentity_t *victim, gentity_t *attacker, const mod_t &mod) {
	auto  now = level.time;
	auto &glob = level.match;
	auto *vcl = victim->client;
	auto &vSess = vcl->pers.match;
	bool  isSuicide = (attacker == victim);
	bool  validKill = (attacker && attacker->client && !isSuicide && !mod.friendly_fire);

	// -- handle a valid non-suicide kill --
	if (validKill) {
		auto *acl = attacker->client;
		auto &aSess = acl->pers.match;

		if (glob.totalKills == 0) {
			PushAward(attacker, PlayerMedal::MEDAL_FIRST_FRAG);
		}

		if (attacker->health > 0) {
			++acl->killStreakCount;
		}

		G_AdjustPlayerScore(acl, 1, GT(GT_TDM), 1);

		++aSess.totalKills;
		++aSess.modTotalKills[mod.id];
		++glob.totalKills;
		++glob.modKills[mod.id];
		if (now - victim->client->respawnMaxTime < 1_sec) {
			++glob.totalSpawnKills;
			++acl->pers.match.totalSpawnKills;
		}


		if (OnSameTeam(attacker, victim)) {
			++glob.totalTeamKills;
			++acl->pers.match.totalTeamKills;
		}

		if (acl->pers.kill_time && acl->pers.kill_time + 2_sec > now) {
			PushAward(attacker, PlayerMedal::MEDAL_EXCELLENT);
		}
		acl->pers.kill_time = now;

		if (mod.id == MOD_BLASTER || mod.id == MOD_CHAINFIST) {
			PushAward(attacker, PlayerMedal::MEDAL_HUMILIATION);
		}
	}

	// -- always record the victim's death --
	++vSess.totalDeaths;
	++glob.totalDeaths;
	++glob.modDeaths[mod.id];
	++vSess.modTotalDeaths[mod.id];

	if (isSuicide) {
		++vSess.totalSuicides;
	} else if (now - victim->client->respawnMaxTime < 1_sec) {
		++vSess.totalSpawnDeaths;
	}

	// -- penalty / follow-killer logic -- 
	bool inPlay = (level.matchState == MatchState::MATCH_IN_PROGRESS);

	if (inPlay && attacker && attacker->client) {
		// attacker killed themselves or hit a teammate?
		if (isSuicide || mod.friendly_fire) {
			if (!mod.no_point_loss) {
				G_AdjustPlayerScore(attacker->client, -1, GT(GT_TDM), -1);
			}
			attacker->client->killStreakCount = 0;
		} else {
			// queue any spectators who want to follow the killer
			for (auto ec : active_clients()) {
				if (!ClientIsPlaying(ec->client)
					&& ec->client->sess.pc.follow_killer) {
					ec->client->followQueuedTarget = attacker;
					ec->client->followQueuedTime = now;
				}
			}
		}
	} else {
		// penalty to the victim
		if (!mod.no_point_loss) {
			G_AdjustPlayerScore(victim->client, -1, GT(GT_TDM), -1);
		}
	}
}

/*
==================
GibPlayer
==================
*/
static void GibPlayer(gentity_t *self, int damage) {
	if (self->flags & FL_NOGIB) {
		return;
	}

	// 1) udeath sound
	gi.sound(self,
		CHAN_BODY,
		gi.soundindex("misc/udeath.wav"),
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
		for (auto &stage : gibStages) {
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
DIE(player_die) (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void {
	if (self->client->ps.pmove.pm_type == PM_DEAD)
		return;

	if (level.intermissionTime)
		return;

	PlayerTrail_Destroy(self);

	self->aVelocity = {};

	self->takeDamage = true;
	self->moveType = MOVETYPE_TOSS;

	self->s.modelindex2 = 0; // remove linked weapon model
	self->s.modelindex3 = 0; // remove linked ctf flag

	self->s.angles[PITCH] = 0;
	self->s.angles[ROLL] = 0;

	self->s.sound = 0;
	self->client->weaponSound = 0;

	self->maxs[2] = -8;

	self->svFlags |= SVF_DEADMONSTER;

	if (!self->deadFlag) {

		if (deathmatch->integer) {
			if (match_playerRespawnMinDelay->value) {
				self->client->respawnMinTime = (level.time + gtime_t::from_sec(match_playerRespawnMinDelay->value));
			} else {
				self->client->respawnMinTime = level.time;
			}

			if (match_forceRespawnTime->value) {
				self->client->respawnMaxTime = (level.time + gtime_t::from_sec(match_forceRespawnTime->value));
			} else {
				self->client->respawnMaxTime = (level.time + 1_sec);
			}
		}

		PushDeathStats(self, attacker, mod);

		LookAtKiller(self, inflictor, attacker);
		self->client->ps.pmove.pm_type = PM_DEAD;
		ClientObituary(self, inflictor, attacker, mod);

		CTF_ScoreBonuses(self, inflictor, attacker);
		TossClientItems(self);
		Weapon_Grapple_DoReset(self->client);

		if (deathmatch->integer && !self->client->showScores)
			Cmd_Help_f(self); // show scores

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

	// clear inventory
	if (Teams())
		self->client->pers.inventory.fill(0);

	// if there's a sphere around, let it know the player died.
	// vengeance and hunter will die if they're not attacking,
	// defender should always die
	if (self->client->ownedSphere) {
		gentity_t *sphere;

		sphere = self->client->ownedSphere;
		sphere->die(sphere, self, self, 0, vec3_origin, mod);
	}

	// if we've been killed by the tracker, GIB!
	if (mod.id == MOD_TRACKER) {
		self->health = -100;
		damage = 400;
	}

	if (GT(GT_FREEZE) && !level.intermissionTime && self->client->eliminated && !self->client->resp.thawer) {
		self->s.effects |= EF_COLOR_SHELL;
		self->s.renderfx |= (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);
	} else {
		self->s.effects = EF_NONE;
		self->s.renderfx = RF_NONE;
	}

	// make sure no trackers are still hurting us.
	if (self->client->trackerPainTime) {
		RemoveAttackingPainDaemons(self);
	}

	// if we got obliterated by the nuke, don't gib
	if ((self->health < -80) && (mod.id == MOD_NUKE))
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

	} else {
		// --- normal death animation & sound ---
		if (!self->deadFlag) {
			// Freeze-mode gets a single static pose
			if (GT(GT_FREEZE)) {
				self->s.frame = FRAME_crstnd01 - 1;
				self->client->anim.end = self->s.frame;
			} else {
				// pick one of the death animations
				self->client->anim.priority = ANIM_DEATH;
				bool ducked = (self->client->ps.pmove.pm_flags & PMF_DUCKED) != 0;

				if (ducked) {
					self->s.frame = FRAME_crdeath1 - 1;
					self->client->anim.end = FRAME_crdeath5;
				} else {
					static constexpr std::array<std::pair<int, int>, 3> deathRanges = { {
						{ FRAME_death101, FRAME_death106 },
						{ FRAME_death201, FRAME_death206 },
						{ FRAME_death301, FRAME_death308 }
					} };

					const auto &[start, end] = deathRanges[irandom(3)];

					self->s.frame = start - 1;
					self->client->anim.end = end;
				}
			}

			// play one of four death cries
			static constexpr std::array<const char *, 4> death_sounds = {
				"*death1.wav", "*death2.wav", "*death3.wav", "*death4.wav"
			};
			gi.sound(self,
				CHAN_VOICE,
				gi.soundindex(random_element(death_sounds)),
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
				if (player->health > 0 || (!level.deadly_kill_box && g_coop_enable_lives->integer && player->client->pers.lives > 0)) {
					allPlayersDead = false;
					break;
				}

			if (allPlayersDead) // allow respawns for telefrags and weird shit
			{
				level.coopLevelRestartTime = level.time + 5_sec;

				for (auto player : active_clients())
					gi.LocCenter_Print(player, "$g_coop_lose");
			}

			// in 3 seconds, attempt a respawn or put us into
			// spectator mode
			if (!level.coopLevelRestartTime)
				self->client->respawnMaxTime = level.time + 3_sec;
		}
	}

	G_LogDeathEvent(self, attacker, mod);

	self->deadFlag = true;

	gi.linkentity(self);
}

//=======================================================================

#include <string>
#include <sstream>

/*
===========================
Player_GiveStartItems
===========================
*/
static void Player_GiveStartItems(gentity_t *ent, const char *input) {
	const char *token;
	while ((token = COM_ParseEx(&input, ";")) && *token) {
		char token_copy[MAX_TOKEN_CHARS];
		Q_strlcpy(token_copy, token, sizeof(token_copy));

		const char *cursor = token_copy;
		const char *item_name = COM_Parse(&cursor);
		if (!item_name || !*item_name)
			continue;

		Item *item = FindItemByClassname(item_name);
		if (!item || !item->pickup) {
			gi.Com_PrintFmt("Invalid g_start_item entry: '{}'\n", item_name);
			continue;
		}

		int32_t count = 1;
		if (cursor && *cursor) {
			const char *count_str = COM_Parse(&cursor);
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

		gentity_t *dummy = Spawn();
		dummy->item = item;
		dummy->count = count;
		dummy->spawnflags |= SPAWNFLAG_ITEM_DROPPED;
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
void InitClientPersistant(gentity_t *ent, gclient_t *client) {
	// backup & restore userinfo
	char userinfo[MAX_INFO_STRING];
	Q_strlcpy(userinfo, client->pers.userInfo, sizeof(userinfo));

	memset(&client->pers, 0, sizeof(client->pers));
	ClientUserinfoChanged(ent, userinfo);

	client->pers.health = 100;
	client->pers.max_health = 100;

	client->pers.medalTime = 0_sec;
	client->pers.medalType = PlayerMedal::MEDAL_NONE;
	std::fill(client->pers.match.medalCount.begin(), client->pers.match.medalCount.end(), 0);

	// don't give us weapons if we shouldn't have any
	if (ClientIsPlaying(client)) {
		// in coop, if there's already a player in the game and we're new,
		// steal their loadout. this would fix a potential softlock where a new
		// player may not have weapons at all.
		bool taken_loadout = false;

		int health, armor;
		gitem_armor_t armor_type = armor_stats[game.ruleset][ARMOR_JACKET];

		if (GTF(GTF_ARENA)) {
			health = clamp(g_arenaStartingHealth->integer, 1, 9999);
			armor = clamp(g_arenaStartingArmor->integer, 0, 999);
		} else {
			health = clamp(g_starting_health->integer, 1, 9999);
			armor = clamp(g_starting_armor->integer, 0, 999);
		}

		if (armor > armor_stats[game.ruleset][ARMOR_JACKET].max_count)
			if (armor > armor_stats[game.ruleset][ARMOR_COMBAT].max_count)
				armor_type = armor_stats[game.ruleset][ARMOR_BODY];
			else armor_type = armor_stats[game.ruleset][ARMOR_COMBAT];

		client->pers.health = client->pers.max_health = health;

		if (deathmatch->integer) {
			int bonus = RS(RS_Q3A) ? 25 : g_starting_health_bonus->integer;
			if (!(GTF(GTF_ARENA)) && bonus > 0) {
				client->pers.health += bonus;
				if (!(RS(RS_Q3A))) {
					client->pers.healthBonus = bonus;
				}
				ent->client->timeResidual = level.time;
			}
		}

		if (armor_type.base_count == armor_stats[game.ruleset][ARMOR_JACKET].base_count)
			client->pers.inventory[IT_ARMOR_JACKET] = armor;
		else if (armor_type.base_count == armor_stats[game.ruleset][ARMOR_COMBAT].base_count)
			client->pers.inventory[IT_ARMOR_COMBAT] = armor;
		else if (armor_type.base_count == armor_stats[game.ruleset][ARMOR_BODY].base_count)
			client->pers.inventory[IT_ARMOR_BODY] = armor;

		if (coop->integer) {
			for (auto player : active_clients()) {
				if (player == ent || !player->client->pers.spawned ||
					!ClientIsPlaying(player->client) ||
					player->moveType == MOVETYPE_NOCLIP || player->moveType == MOVETYPE_FREECAM)
					continue;

				client->pers.inventory = player->client->pers.inventory;
				client->pers.ammoMax = player->client->pers.ammoMax;
				client->pers.powerCubes = player->client->pers.powerCubes;
				taken_loadout = true;
				break;
			}
		}

		if (GT(GT_BALL)) {
			client->pers.inventory[IT_WEAPON_CHAINFIST] = 1;
		} else if (!taken_loadout) {
			if (g_instagib->integer) {
				client->pers.inventory[IT_WEAPON_RAILGUN] = 1;
				client->pers.inventory[IT_AMMO_SLUGS] = AMMO_INFINITE;
			} else if (g_nadefest->integer) {
				client->pers.inventory[IT_AMMO_GRENADES] = AMMO_INFINITE;
			} else if (GTF(GTF_ARENA)) {
				client->pers.ammoMax.fill(50);
				client->pers.ammoMax[AMMO_SHELLS] = 50;
				client->pers.ammoMax[AMMO_BULLETS] = 300;
				client->pers.ammoMax[AMMO_GRENADES] = 50;
				client->pers.ammoMax[AMMO_ROCKETS] = 50;
				client->pers.ammoMax[AMMO_CELLS] = 200;
				client->pers.ammoMax[AMMO_SLUGS] = 25;
				/*
				client->pers.ammoMax[AMMO_TRAP] = 5;
				client->pers.ammoMax[AMMO_FLECHETTES] = 200;
				client->pers.ammoMax[AMMO_DISRUPTOR] = 12;
				client->pers.ammoMax[AMMO_TESLA] = 5;
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
			} else {
				if (RS(RS_Q3A)) {
					client->pers.ammoMax.fill(200);
					client->pers.ammoMax[AMMO_BULLETS] = 200;
					client->pers.ammoMax[AMMO_SHELLS] = 200;
					client->pers.ammoMax[AMMO_CELLS] = 200;

					client->pers.ammoMax[AMMO_TRAP] = 200;
					client->pers.ammoMax[AMMO_FLECHETTES] = 200;
					client->pers.ammoMax[AMMO_DISRUPTOR] = 200;
					client->pers.ammoMax[AMMO_TESLA] = 200;

					client->pers.inventory[IT_WEAPON_CHAINFIST] = 1;
					client->pers.inventory[IT_WEAPON_MACHINEGUN] = 1;
					client->pers.inventory[IT_AMMO_BULLETS] = (GT(GT_TDM)) ? 50 : 100;
				} else if (RS(RS_Q1)) {
					client->pers.ammoMax.fill(200);
					client->pers.ammoMax[AMMO_BULLETS] = 200;
					client->pers.ammoMax[AMMO_SHELLS] = 200;
					client->pers.ammoMax[AMMO_CELLS] = 200;

					client->pers.ammoMax[AMMO_TRAP] = 200;
					client->pers.ammoMax[AMMO_FLECHETTES] = 200;
					client->pers.ammoMax[AMMO_DISRUPTOR] = 200;
					client->pers.ammoMax[AMMO_TESLA] = 200;

					client->pers.inventory[IT_WEAPON_CHAINFIST] = 1;
					client->pers.inventory[IT_WEAPON_SHOTGUN] = 1;
					client->pers.inventory[IT_AMMO_SHELLS] = 10;
				} else {
					// fill with 50s, since it's our most common value
					client->pers.ammoMax.fill(50);
					client->pers.ammoMax[AMMO_BULLETS] = 200;
					client->pers.ammoMax[AMMO_SHELLS] = 100;
					client->pers.ammoMax[AMMO_CELLS] = 200;

					client->pers.ammoMax[AMMO_TRAP] = 5;
					client->pers.ammoMax[AMMO_FLECHETTES] = 200;
					client->pers.ammoMax[AMMO_DISRUPTOR] = 12;
					client->pers.ammoMax[AMMO_TESLA] = 5;

					client->pers.inventory[IT_WEAPON_BLASTER] = 1;
				}

				if (deathmatch->integer) {
					if (level.matchState < MatchState::MATCH_IN_PROGRESS) {
						for (size_t i = FIRST_WEAPON; i < LAST_WEAPON; i++) {
							if (!level.weaponCount[i - FIRST_WEAPON])
								continue;

							if (!itemList[i].ammo)
								continue;

							client->pers.inventory[i] = 1;

							Item *ammo = GetItemByIndex(itemList[i].ammo);
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

			if (!deathmatch->integer || level.matchState < MatchState::MATCH_IN_PROGRESS)
				// compass also used for ready status toggling in deathmatch
				client->pers.inventory[IT_COMPASS] = 1;

			bool give_grapple = (!strcmp(g_allow_grapple->string, "auto")) ?
				(GTF(GTF_CTF) ? !level.no_grapple : 0) :
				(g_allow_grapple->integer > 0 && !g_grapple_offhand->integer);
			if (give_grapple)
				client->pers.inventory[IT_WEAPON_GRAPPLE] = 1;
		}

		NoAmmoWeaponChange(ent, false);

		client->pers.weapon = client->newWeapon;
		if (client->newWeapon)
			client->pers.selected_item = client->newWeapon->id;
		client->newWeapon = nullptr;
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

static void InitClientResp(gclient_t *cl) {
	memset(&cl->resp, 0, sizeof(cl->resp));

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
	gentity_t *ent;

	for (size_t i = 0; i < game.maxclients; i++) {
		ent = &g_entities[1 + i];
		if (!ent->inUse)
			continue;
		game.clients[i].pers.health = ent->health;
		game.clients[i].pers.max_health = ent->max_health;
		game.clients[i].pers.saved_flags = (ent->flags & (FL_FLASHLIGHT | FL_GODMODE | FL_NOTARGET | FL_POWER_ARMOR | FL_WANTS_POWER_ARMOR));
		if (coop->integer)
			game.clients[i].pers.score = ent->client->resp.score;
	}
}

void FetchClientEntData(gentity_t *ent) {
	ent->health = ent->client->pers.health;
	ent->max_health = ent->client->pers.max_health;
	ent->flags |= ent->client->pers.saved_flags;
	if (coop->integer)
		G_SetPlayerScore(ent->client, ent->client->pers.score);
}

/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
PlayersRangeFromSpot

Returns the distance to the nearest player from the given spot
muffmode: excludes current client
================
*/
static float PlayersRangeFromSpot(gentity_t *ent, gentity_t *spot) {
	float	bestplayerdistance;
	vec3_t	v;
	float	playerdistance;

	bestplayerdistance = 9999999;

	for (auto ec : active_clients()) {
		if (ec->health <= 0 || ec->client->eliminated)
			continue;
#if 0
		if (ent != nullptr)
			if (ec == ent)
				continue;
#endif
		v = spot->s.origin - ec->s.origin;
		playerdistance = v.length();

		if (playerdistance < bestplayerdistance)
			bestplayerdistance = playerdistance;
	}

	return bestplayerdistance;
}

static bool SpawnPointClear(gentity_t *spot) {
	vec3_t p = spot->s.origin + vec3_t{ 0, 0, 9.f };
	return !gi.trace(p, PLAYER_MINS, PLAYER_MAXS, p, spot, CONTENTS_PLAYER | CONTENTS_MONSTER).startsolid;
}

select_spawn_result_t SelectDeathmatchSpawnPoint(gentity_t *ent, vec3_t avoid_point, bool force_spawn, bool fallback_to_ctf_or_start, bool intermission, bool initial) {
	float cv_dist = match_playerRespawnMinDistance->value;
	struct spawn_point_t {
		gentity_t *point;
		float dist;
	};

	static std::vector<spawn_point_t> spawn_points;

	spawn_points.clear();

	// gather all spawn points 
	gentity_t *spot = nullptr;

	if (cv_dist > 512) cv_dist = 512;
	else if (cv_dist < 0) cv_dist = 0;

	if (intermission)
		while ((spot = G_FindByString<&gentity_t::className>(spot, "info_player_intermission")) != nullptr)
			spawn_points.push_back({ spot, PlayersRangeFromSpot(ent, spot) });

	if (spawn_points.size() == 0) {
		spot = nullptr;
		while ((spot = G_FindByString<&gentity_t::className>(spot, "info_player_deathmatch")) != nullptr)
			spawn_points.push_back({ spot, PlayersRangeFromSpot(ent, spot) });

		// no points
		if (spawn_points.size() == 0) {
			// try CTF spawns...
			if (fallback_to_ctf_or_start) {
				spot = nullptr;
				while ((spot = G_FindByString<&gentity_t::className>(spot, "info_player_team_red")) != nullptr)
					spawn_points.push_back({ spot, PlayersRangeFromSpot(ent, spot) });
				spot = nullptr;
				while ((spot = G_FindByString<&gentity_t::className>(spot, "info_player_team_blue")) != nullptr)
					spawn_points.push_back({ spot, PlayersRangeFromSpot(ent, spot) });

				// we only have an info_player_start then
				if (spawn_points.size() == 0) {
					spot = G_FindByString<&gentity_t::className>(nullptr, "info_player_start");

					if (spot)
						spawn_points.push_back({ spot, PlayersRangeFromSpot(ent, spot) });

					// map is malformed
					if (spawn_points.size() == 0)
						return { nullptr, false };
				}
			} else
				return { nullptr, false };
		}
	}

	// if there's only one spawn point, that's the one.
	if (spawn_points.size() == 1) {
		if (force_spawn || SpawnPointClear(spawn_points[0].point))
			return { spawn_points[0].point, true };

		return { nullptr, true };
	}

	// order by distances ascending (top of list has closest players to point)
	std::sort(spawn_points.begin(), spawn_points.end(), [](const spawn_point_t &a, const spawn_point_t &b) { return a.dist < b.dist; });

		size_t margin = spawn_points.size() / 2;

		// for random, select a random point other than the two
		// that are closest to the player if possible.
		// shuffle the non-distance-related spawn points
		std::shuffle(spawn_points.begin() + margin, spawn_points.end(), mt_rand);

		// run down the list and pick the first one that we can use
		for (auto it = spawn_points.begin() + margin; it != spawn_points.end(); ++it) {
			auto spot = it->point;

			if (avoid_point == spot->s.origin)
				continue;
			//muff: avoid respawning at or close to last spawn point
			if (avoid_point && cv_dist) {
				vec3_t v = spot->s.origin - avoid_point;
				float d = v.length();

				if (d <= cv_dist) {
					if (match_playerRespawnMinDistanceDebug->integer)
						gi.Com_PrintFmt("{}: avoiding spawn point\n", *spot);
					continue;
				}
			}
			if (spot->arena && spot->arena != level.arenaActive)
				continue;

			if (ent && ent->client) {
				if (ent->client->sess.is_a_bot)
					if (spot->flags & FL_NO_BOTS)
						continue;
				if (!ent->client->sess.is_a_bot)
					if (spot->flags & FL_NO_HUMANS)
						continue;
			}

			if (SpawnPointClear(spot))
				return { spot, true };
		}

		// none clear, so we have to pick one of the other two
		if (SpawnPointClear(spawn_points[1].point))
			return { spawn_points[1].point, true };
		else if (SpawnPointClear(spawn_points[0].point))
			return { spawn_points[0].point, true };

	if (force_spawn)
		return { random_element(spawn_points).point, true };

	return { nullptr, true };
}

static vec3_t TeamCentralPoint(team_t team) {
	vec3_t	team_origin = { 0, 0, 0 };
	uint8_t team_count = 0;
	for (auto ec : active_players()) {
		if (ec->client->sess.team != team)
			continue;

		team_origin += ec->s.origin;
		team_count++;
	}
	if (team_origin)
		return team_origin / team_count;
	else
		return team_origin;
}

/*
================
SelectTeamSpawnPoint

Go to a team point, but NOT the two points closest
to other players
================
*/
static gentity_t *SelectTeamSpawnPoint(gentity_t *ent, bool force_spawn) {
	if (ent->client->resp.ctf_state) {
		select_spawn_result_t result = SelectDeathmatchSpawnPoint(ent, ent->client->spawn_origin, force_spawn, false, false, false);	// !ClientIsPlaying(ent->client));

		if (result.any_valid)
			return result.spot;
	}

	const char *cname;

	switch (ent->client->sess.team) {
		case TEAM_RED:
			cname = "info_player_team_red";
			break;
		case TEAM_BLUE:
			cname = "info_player_team_blue";
			break;
		default:
		{
			select_spawn_result_t result = SelectDeathmatchSpawnPoint(ent, ent->client->spawn_origin, force_spawn, true, false, false);

			if (result.any_valid)
				return result.spot;

			gi.Com_Error("Can't find suitable spectator spawn point.");
			return nullptr;
		}
	}

	static std::vector<gentity_t *> spawn_points;
	gentity_t *spot = nullptr;

	spawn_points.clear();

	while ((spot = G_FindByString<&gentity_t::className>(spot, cname)) != nullptr)
		spawn_points.push_back(spot);

	if (!spawn_points.size()) {
		select_spawn_result_t result = SelectDeathmatchSpawnPoint(ent, ent->client->spawn_origin, force_spawn, true, false, false);

		if (!result.any_valid)
			gi.Com_Error("Can't find suitable team spawn point.");

		return result.spot;
	}

	std::shuffle(spawn_points.begin(), spawn_points.end(), mt_rand);

	for (auto &point : spawn_points)
		if (SpawnPointClear(point))
			return point;

	if (force_spawn)
		return random_element(spawn_points);

	return nullptr;
}

static gentity_t *SelectLavaCoopSpawnPoint(gentity_t *ent) {
	int		 index;
	gentity_t *spot = nullptr;
	float	 lavatop;
	gentity_t *lava;
	gentity_t *pointWithLeastLava;
	float	 lowest;
	gentity_t *spawnPoints[64]{};
	vec3_t	 center;
	int		 numPoints;
	gentity_t *highestlava;

	lavatop = -99999;
	highestlava = nullptr;

	// first, find the highest lava
	// remember that some will stop moving when they've filled their
	// areas...
	lava = nullptr;
	while (1) {
		lava = G_FindByString<&gentity_t::className>(lava, "func_water");
		if (!lava)
			break;

		center = lava->absMax + lava->absMin;
		center *= 0.5f;

		if (lava->spawnflags.has(SPAWNFLAG_WATER_SMART) && (gi.pointcontents(center) & MASK_WATER)) {
			if (lava->absMax[2] > lavatop) {
				lavatop = lava->absMax[2];
				highestlava = lava;
			}
		}
	}

	// if we didn't find ANY lava, then return nullptr
	if (!highestlava)
		return nullptr;

	// find the top of the lava and include a small margin of error (plus bbox size)
	lavatop = highestlava->absMax[2] + 64;

	// find all the lava spawn points and store them in spawnPoints[]
	spot = nullptr;
	numPoints = 0;
	while ((spot = G_FindByString<&gentity_t::className>(spot, "info_player_coop_lava"))) {
		if (numPoints == 64)
			break;

		spawnPoints[numPoints++] = spot;
	}

	// walk up the sorted list and return the lowest, open, non-lava spawn point
	spot = nullptr;
	lowest = 999999;
	pointWithLeastLava = nullptr;
	for (index = 0; index < numPoints; index++) {
		if (spawnPoints[index]->s.origin[2] < lavatop)
			continue;

		if (PlayersRangeFromSpot(ent, spawnPoints[index]) > 32) {
			if (spawnPoints[index]->s.origin[2] < lowest) {
				// save the last point
				pointWithLeastLava = spawnPoints[index];
				lowest = spawnPoints[index]->s.origin[2];
			}
		}
	}

	return pointWithLeastLava;
}

// [Paril-KEX]
static gentity_t *SelectSingleSpawnPoint(gentity_t *ent) {
	gentity_t *spot = nullptr;

	while ((spot = G_FindByString<&gentity_t::className>(spot, "info_player_start")) != nullptr) {
		if (!game.spawnpoint[0] && !spot->targetname)
			break;

		if (!game.spawnpoint[0] || !spot->targetname)
			continue;

		if (Q_strcasecmp(game.spawnpoint, spot->targetname) == 0)
			break;
	}

	if (!spot) {
		// there wasn't a matching targeted spawnpoint, use one that has no targetname
		while ((spot = G_FindByString<&gentity_t::className>(spot, "info_player_start")) != nullptr)
			if (!spot->targetname)
				return spot;
	}

	// none at all, so just pick any
	if (!spot)
		return G_FindByString<&gentity_t::className>(spot, "info_player_start");

	return spot;
}

// [Paril-KEX]
static gentity_t *G_UnsafeSpawnPosition(vec3_t spot, bool check_players) {
	contents_t mask = MASK_PLAYERSOLID;

	if (!check_players)
		mask &= ~CONTENTS_PLAYER;

	trace_t tr = gi.trace(spot, PLAYER_MINS, PLAYER_MAXS, spot, nullptr, mask);

	// sometimes the spot is too close to the ground, give it a bit of slack
	if (tr.startsolid && !tr.ent->client) {
		spot[2] += 1;
		tr = gi.trace(spot, PLAYER_MINS, PLAYER_MAXS, spot, nullptr, mask);
	}

	// no idea why this happens in some maps..
	if (tr.startsolid && !tr.ent->client) {
		// try a nudge
		if (G_FixStuckObject_Generic(spot, PLAYER_MINS, PLAYER_MAXS, [mask](const vec3_t &start, const vec3_t &mins, const vec3_t &maxs, const vec3_t &end) {
			return gi.trace(start, mins, maxs, end, nullptr, mask);
			}) == stuck_result_t::NO_GOOD_POSITION)
			return tr.ent; // what do we do here...?

			trace_t tr = gi.trace(spot, PLAYER_MINS, PLAYER_MAXS, spot, nullptr, mask);

			if (tr.startsolid && !tr.ent->client)
				return tr.ent; // what do we do here...?
	}

	if (tr.fraction == 1.f)
		return nullptr;
	else if (check_players && tr.ent && tr.ent->client)
		return tr.ent;

	return nullptr;
}

static gentity_t *SelectCoopSpawnPoint(gentity_t *ent, bool force_spawn, bool check_players) {
	gentity_t *spot = nullptr;
	const char *target;

	//  rogue hack, but not too gross...
	if (!Q_strcasecmp(level.mapname, "rmine2"))
		return SelectLavaCoopSpawnPoint(ent);

	// try the main spawn point first
	spot = SelectSingleSpawnPoint(ent);

	if (spot && !G_UnsafeSpawnPosition(spot->s.origin, check_players))
		return spot;

	spot = nullptr;

	// assume there are four coop spots at each spawnpoint
	int32_t num_valid_spots = 0;

	while (1) {
		spot = G_FindByString<&gentity_t::className>(spot, "info_player_coop");
		if (!spot)
			break; // we didn't have enough...

		target = spot->targetname;
		if (!target)
			target = "";
		if (Q_strcasecmp(game.spawnpoint, target) == 0) { // this is a coop spawn point for one of the clients here
			num_valid_spots++;

			if (!G_UnsafeSpawnPosition(spot->s.origin, check_players))
				return spot; // this is it
		}
	}

	bool use_targetname = true;

	// if we didn't find any spots, map is probably set up wrong.
	// use empty targetname ones.
	if (!num_valid_spots) {
		use_targetname = false;

		while (1) {
			spot = G_FindByString<&gentity_t::className>(spot, "info_player_coop");
			if (!spot)
				break; // we didn't have enough...

			target = spot->targetname;
			if (!target) {
				// this is a coop spawn point for one of the clients here
				num_valid_spots++;

				if (!G_UnsafeSpawnPosition(spot->s.origin, check_players))
					return spot; // this is it
			}
		}
	}

	// if player collision is disabled, just pick a random spot
	if (!g_coop_player_collision->integer) {
		spot = nullptr;

		num_valid_spots = irandom(num_valid_spots);

		while (1) {
			spot = G_FindByString<&gentity_t::className>(spot, "info_player_coop");

			if (!spot)
				break; // we didn't have enough...

			target = spot->targetname;
			if (use_targetname && !target)
				target = "";
			if (use_targetname ? (Q_strcasecmp(game.spawnpoint, target) == 0) : !target) { // this is a coop spawn point for one of the clients here
				num_valid_spots++;

				if (!num_valid_spots)
					return spot;

				--num_valid_spots;
			}
		}

		// if this fails, just fall through to some other spawn.
	}

	// no safe spots..?
	if (force_spawn || !g_coop_player_collision->integer)
		return SelectSingleSpawnPoint(spot);

	return nullptr;
}

static bool TryLandmarkSpawn(gentity_t *ent, vec3_t &origin, vec3_t &angles) {
	// if transitioning from another level with a landmark seamless transition
	// just set the location here
	if (!ent->client->landmark_name || !strlen(ent->client->landmark_name)) {
		return false;
	}

	gentity_t *landmark = PickTarget(ent->client->landmark_name);
	if (!landmark) {
		return false;
	}

	vec3_t old_origin = origin;
	vec3_t spot_origin = origin;
	origin = ent->client->landmark_rel_pos;

	// rotate our relative landmark into our new landmark's frame of reference
	origin = RotatePointAroundVector({ 1, 0, 0 }, origin, landmark->s.angles[PITCH]);
	origin = RotatePointAroundVector({ 0, 1, 0 }, origin, landmark->s.angles[ROLL]);
	origin = RotatePointAroundVector({ 0, 0, 1 }, origin, landmark->s.angles[YAW]);

	origin += landmark->s.origin;

	angles = ent->client->oldViewAngles + landmark->s.angles;

	if (landmark->spawnflags.has(SPAWNFLAG_LANDMARK_KEEP_Z))
		origin[2] = spot_origin[2];

	// sometimes, landmark spawns can cause slight inconsistencies in collision;
	// we'll do a bit of tracing to make sure the bbox is clear
	if (G_FixStuckObject_Generic(origin, PLAYER_MINS, PLAYER_MAXS, [ent](const vec3_t &start, const vec3_t &mins, const vec3_t &maxs, const vec3_t &end) {
		return gi.trace(start, mins, maxs, end, ent, MASK_PLAYERSOLID & ~CONTENTS_PLAYER);
		}) == stuck_result_t::NO_GOOD_POSITION) {
		origin = old_origin;
		return false;
	}

	ent->s.origin = origin;

	// rotate the velocity that we grabbed from the map
	if (ent->velocity) {
		ent->velocity = RotatePointAroundVector({ 1, 0, 0 }, ent->velocity, landmark->s.angles[PITCH]);
		ent->velocity = RotatePointAroundVector({ 0, 1, 0 }, ent->velocity, landmark->s.angles[ROLL]);
		ent->velocity = RotatePointAroundVector({ 0, 0, 1 }, ent->velocity, landmark->s.angles[YAW]);
	}

	return true;
}

/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, coop start, etc
============
*/
bool SelectSpawnPoint(gentity_t *ent, vec3_t &origin, vec3_t &angles, bool force_spawn, bool &landmark) {
	gentity_t *spot = nullptr;

	// DM spots are simple
	if (deathmatch->integer) {
		if (Teams() && ClientIsPlaying(ent->client))
			spot = SelectTeamSpawnPoint(ent, force_spawn);
		else {
			if (!spot) {
				if (ent) {
					select_spawn_result_t result = SelectDeathmatchSpawnPoint(ent, ent->client->spawn_origin, force_spawn, true, !ClientIsPlaying(ent->client) || ent->client->eliminated, false);

					if (!result.any_valid)
						gi.Com_Error("No valid spawn points found.");

					spot = result.spot;
				}
			}
		}

		if (spot) {
			origin = spot->s.origin + vec3_t{ 0, 0, (float)(match_allowSpawnPads->integer ? 9 : 1) };
			angles = spot->s.angles;

			//wor: we just want yaw really, definitely no roll!
			//if (ClientIsPlaying(ent->client))
				//angles[PITCH] = 0;
			angles[ROLL] = 0;

			return true;
		}

		return false;
	}

	if (coop->integer) {
		spot = SelectCoopSpawnPoint(ent, force_spawn, true);

		if (!spot)
			spot = SelectCoopSpawnPoint(ent, force_spawn, false);

		// no open spot yet
		if (!spot) {
			// in worst case scenario in coop during intermission, just spawn us at intermission
			// spot. this only happens for a single frame, and won't break
			// anything if they come back.
			if (level.intermissionTime) {
				origin = level.intermission.origin;
				angles = level.intermission.angles;
				return true;
			}

			return false;
		}
	} else {
		spot = SelectSingleSpawnPoint(ent);

		// in SP, just put us at the origin if spawn fails
		if (!spot) {
			gi.Com_PrintFmt("Couldn't find spawn point {}\n", game.spawnpoint);

			origin = { 0, 0, 0 };
			angles = { 0, 0, 0 };

			return true;
		}
	}

	// spot should always be non-null here

	origin = spot->s.origin;
	angles = spot->s.angles;

	// check landmark
	if (TryLandmarkSpawn(ent, origin, angles))
		landmark = true;

	return true;
}

/*
===========
SelectSpectatorSpawnPoint

============
*/
static gentity_t *SelectSpectatorSpawnPoint(vec3_t origin, vec3_t angles) {
	FindIntermissionPoint();
	//SetIntermissionPoint();
	origin = level.intermission.origin;
	angles = level.intermission.angles;

	return level.spawnSpots[SPAWN_SPOT_INTERMISSION]; // was NULL
}

//======================================================================

void InitBodyQue() {
	gentity_t *ent;

	level.bodyQue = 0;
	for (size_t i = 0; i < BODY_QUEUE_SIZE; i++) {
		ent = Spawn();
		ent->className = "bodyque";
	}
}

static DIE(body_die) (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void {
	if (self->s.modelindex == MODELINDEX_PLAYER &&
		self->health < self->gibHealth) {
		gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		ThrowGibs(self, damage, { { 4, "models/objects/gibs/sm_meat/tris.md2" } });
		self->s.origin[2] -= 48;
		ThrowClientHead(self, damage);
	}

	if (mod.id == MOD_CRUSH) {
		// prevent explosion singularities
		self->svFlags = SVF_NOCLIENT;
		self->takeDamage = false;
		self->solid = SOLID_NOT;
		self->moveType = MOVETYPE_NOCLIP;
		gi.linkentity(self);
	}
}

/*
=============
BodySink

After sitting around for x seconds, fall into the ground and disappear
=============
*/
static THINK(BodySink) (gentity_t *ent) -> void {
	if (!ent->linked)
		return;

	if (level.time > ent->timeStamp) {
		ent->svFlags = SVF_NOCLIENT;
		ent->takeDamage = false;
		ent->solid = SOLID_NOT;
		ent->moveType = MOVETYPE_NOCLIP;

		// the body ques are never actually freed, they are just unlinked
		gi.unlinkentity(ent);
		return;
	}
	ent->nextThink = level.time + 50_ms;
	ent->s.origin[2] -= 0.5;
	gi.linkentity(ent);
}

void CopyToBodyQue(gentity_t *ent) {
	// if we were completely removed, don't bother with a body
	if (!ent->s.modelindex)
		return;

	gentity_t *body;
	bool frozen = !!(GT(GT_FREEZE) && !level.intermissionTime && ent->client->eliminated && !ent->client->resp.thawer);

	// grab a body que and cycle to the next one
	body = &g_entities[game.maxclients + level.bodyQue + 1];
	level.bodyQue = (static_cast<size_t>(level.bodyQue) + 1u) % BODY_QUEUE_SIZE;

	// FIXME: send an effect on the removed body

	gi.unlinkentity(ent);

	gi.unlinkentity(body);
	body->s = ent->s;
	body->s.number = body - g_entities;
	body->s.skinnum = ent->s.skinnum & 0xFF; // only copy the client #

	if (frozen) {
		body->s.effects |= EF_COLOR_SHELL;
		body->s.renderfx |= (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);
	} else {
		body->s.effects = EF_NONE;
		body->s.renderfx = RF_NONE;
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
	} else
		body->mins = body->maxs = {};

	if (CORPSE_SINK_TIME > 0 && notGT(GT_FREEZE)) {
		body->timeStamp = level.time + gtime_t::from_sec(CORPSE_SINK_TIME + 1.5);
		body->nextThink = level.time + gtime_t::from_sec(CORPSE_SINK_TIME);
		body->think = BodySink;
	}

	body->die = body_die;
	body->takeDamage = true;

	gi.linkentity(body);
}

void G_PostRespawn(gentity_t *self) {
	if (self->svFlags & SVF_NOCLIENT)
		return;

	// add a teleportation effect
	self->s.event = EV_PLAYER_TELEPORT;

	// hold in place briefly
	self->client->ps.pmove.pm_flags |= PMF_TIME_KNOCKBACK;
	self->client->ps.pmove.pm_time = 112;
	
	self->client->respawnMinTime = 0_ms;
	self->client->respawnMaxTime = level.time;
	
	if (deathmatch->integer && level.matchState == MatchState::MATCH_WARMUP_READYUP)
		BroadcastReadyReminderMessage();
}

static void ClientSetEliminated(gentity_t *self) {
	self->client->eliminated = true;
	//MoveClientToFreeCam(self);
}

void ClientRespawn(gentity_t *ent) {
	if (deathmatch->integer || coop->integer) {
		// spectators don't leave bodies
		if (ClientIsPlaying(ent->client))
			CopyToBodyQue(ent);
		ent->svFlags &= ~SVF_NOCLIENT;

		if (GT(GT_RR) && level.matchState == MatchState::MATCH_IN_PROGRESS) {
			ent->client->sess.team = Teams_OtherTeam(ent->client->sess.team);
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
// skinnum was historically used to pack data
// so we're going to build onto that.
void P_AssignClientSkinnum(gentity_t *ent) {
	if (ent->s.modelindex != 255)
		return;

	player_skinnum_t packed{};

	packed.client_num = ent->client - game.clients;
	if (ent->client->pers.weapon)
		packed.vwep_index = ent->client->pers.weapon->vwep_index - level.viewWeaponOffset + 1;
	else
		packed.vwep_index = 0;
	packed.viewHeight = ent->client->ps.viewoffset.z + ent->client->ps.pmove.viewHeight;

	if (CooperativeModeOn())
		packed.team_index = 1; // all players are teamed in coop
	else if (Teams())
		packed.team_index = ent->client->sess.team;
	else
		packed.team_index = 0;

	if (ent->deadFlag)
		packed.poi_icon = 1;
	else
		packed.poi_icon = 0;

	ent->s.skinnum = packed.skinnum;
}

// [Paril-KEX] send player level POI
void P_SendLevelPOI(gentity_t *ent) {
	if (!level.validPOI)
		return;

	gi.WriteByte(svc_poi);
	gi.WriteShort(POI_OBJECTIVE);
	gi.WriteShort(10000);
	gi.WritePosition(ent->client->help_poi_location);
	gi.WriteShort(ent->client->help_poi_image);
	gi.WriteByte(208);
	gi.WriteByte(POI_FLAG_NONE);
	gi.unicast(ent, true);
}

// [Paril-KEX] force the fog transition on the given player,
// optionally instantaneously (ignore any transition time)
void P_ForceFogTransition(gentity_t *ent, bool instant) {
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
		fog.time = clamp(ent->client->pers.fog_transition_time.milliseconds(), (int64_t)0, (int64_t)std::numeric_limits<uint16_t>::max());
	}

	// check heightfog stuff
	auto &hf = ent->client->heightfog;
	const auto &wanted_hf = ent->client->pers.wanted_heightfog;

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

static void MoveClientToFreeCam(gentity_t *ent) {
	ent->moveType = MOVETYPE_FREECAM;
	ent->solid = SOLID_NOT;
	ent->svFlags |= SVF_NOCLIENT;
	ent->client->ps.gunindex = 0;
	ent->client->ps.gunskin = 0;

	ent->client->ps.stats[STAT_SHOW_STATUSBAR] = 0;

	ent->takeDamage = false;
	ent->s.modelindex = 0;
	ent->s.modelindex2 = 0;
	ent->s.modelindex3 = 0;
	ent->s.effects = EF_NONE;
	ent->client->ps.damageBlend[3] = ent->client->ps.screen_blend[3] = 0;
	ent->client->ps.rdflags = RDF_NONE;
	ent->s.sound = 0;

	gi.linkentity(ent);
}

/*
===========
InitPlayerTeam
============
*/
static bool InitPlayerTeam(gentity_t *ent) {
	// Non-deathmatch (e.g. single-player or coop) - everyone plays
	if (!deathmatch->integer) {
		ent->client->sess.team = TEAM_FREE;
		ent->client->ps.stats[STAT_SHOW_STATUSBAR] = 1;
		return true;
	}

	// If we've already been placed on a team, do nothing
	if (ent->client->sess.team != TEAM_NONE) {
		return true;
	}

	bool matchLocked = ((level.matchState >= MatchState::MATCH_COUNTDOWN) && match_lock->integer);

	if (!matchLocked) {
		if (ent == host) {
			if (g_owner_auto_join->integer) {
				SetTeam(ent, PickTeam(-1), false, false, false);
				return true;
			}
		} else {
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
	ent->client->sess.team = TEAM_SPECTATOR;
	MoveClientToFreeCam(ent);

	if (!ent->client->initial_menu_shown)
		ent->client->initial_menu_delay = level.time + 10_hz;

	return false;
}

// [Paril-KEX] ugly global to handle squad respawn origin
static bool use_squad_respawn = false;
static bool spawn_from_begin = false;
static vec3_t squad_respawn_position, squad_respawn_angles;

static inline void PutClientOnSpawnPoint(gentity_t *ent, const vec3_t &spawn_origin, const vec3_t &spawn_angles) {
	gclient_t *client = ent->client;

	client->spawn_origin = spawn_origin;
	client->ps.pmove.origin = spawn_origin;

	ent->s.origin = spawn_origin;
	if (!use_squad_respawn)
		ent->s.origin[2] += 1; // make sure off ground
	ent->s.old_origin = ent->s.origin;

	// set the delta angle
	client->ps.pmove.delta_angles = spawn_angles - client->resp.cmdAngles;

	ent->s.angles = spawn_angles;
	//ent->s.angles[PITCH] /= 3;		//muff: why??

	client->ps.viewangles = ent->s.angles;
	client->vAngle = ent->s.angles;

	AngleVectors(client->vAngle, client->vForward, nullptr, nullptr);
}

/*
===========
ClientSpawn

Previously known as 'PutClientInServer'

Called when a player connects to a server or respawns in
a deathmatch.
============
*/
void ClientSpawn(gentity_t *ent) {
	int						index = ent - g_entities - 1;
	vec3_t					spawn_origin, spawn_angles;
	gclient_t				*client = ent->client;
	client_persistant_t		saved_pers;
	client_respawn_t		saved_resp;
	client_session_t		saved_sess;

	if (GTF(GTF_ROUNDS) && GTF(GTF_ELIMINATION) && level.matchState == MatchState::MATCH_IN_PROGRESS && notGT(GT_HORDE))
		if (level.roundState == RoundState::ROUND_IN_PROGRESS || level.roundState == RoundState::ROUND_ENDED)
			ClientSetEliminated(ent);
		else ent->client->eliminated = false;
	bool eliminated = ent->client->eliminated;
	int lives = 0;
	if (CooperativeModeOn() && g_coop_enable_lives->integer)
		lives = ent->client->pers.spawned ? ent->client->pers.lives : g_coop_enable_lives->integer + 1;
	
	// clear velocity now, since landmark may change it
	ent->velocity = {};

	if (client->landmark_name != nullptr)
		ent->velocity = client->oldVelocity;

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	bool valid_spawn = false;
	bool force_spawn = client->awaiting_respawn && level.time > client->respawn_timeout;
	bool is_landmark = false;

	InitPlayerTeam(ent);

	if (!ClientIsPlaying(ent->client) || eliminated)
		ent->flags |= FL_NOTARGET;
	else
		ent->flags &= ~FL_NOTARGET;

	if (use_squad_respawn) {
		spawn_origin = squad_respawn_position;
		spawn_angles = squad_respawn_angles;
		valid_spawn = true;
	} else
		valid_spawn = SelectSpawnPoint(ent, spawn_origin, spawn_angles, force_spawn, is_landmark);

	// [Paril-KEX] if we didn't get a valid spawn, hold us in
	// limbo for a while until we do get one
	if (!valid_spawn) {
		// only do this once per spawn
		if (!client->awaiting_respawn) {
			char userInfo[MAX_INFO_STRING];
			memcpy(userInfo, client->pers.userInfo, sizeof(userInfo));
			ClientUserinfoChanged(ent, userInfo);

			client->respawn_timeout = level.time + 3_sec;
		}

		// find a spot to place us
		//SetIntermissionPoint();
		FindIntermissionPoint();

		ent->s.origin = level.intermission.origin;
		ent->client->ps.pmove.origin = level.intermission.origin;
		ent->client->ps.viewangles = level.intermission.angles;

		client->awaiting_respawn = true;
		client->ps.pmove.pm_type = PM_FREEZE;
		client->ps.rdflags = RDF_NONE;
		ent->deadFlag = false;

		MoveClientToFreeCam(ent);
		gi.linkentity(ent);

		return;
	}
	
	client->resp.ctf_state++;

	bool was_waiting_for_respawn = client->awaiting_respawn;

	if (client->awaiting_respawn)
		ent->svFlags &= ~SVF_NOCLIENT;

	client->awaiting_respawn = false;
	client->respawn_timeout = 0_ms;

	// deathmatch wipes most client data every spawn
	if (deathmatch->integer) {
		client->pers.health = 0;
		saved_resp = client->resp;
		saved_sess = client->sess;
	} else {
		// [Kex] Maintain user info in singleplayer to keep the player skin. 
		char userInfo[MAX_INFO_STRING];
		memcpy(userInfo, client->pers.userInfo, sizeof(userInfo));

		if (coop->integer) {
			saved_resp = client->resp;
			saved_sess = client->sess;

			if (!P_UseCoopInstancedItems()) {
				saved_resp.coopRespawn.game_help1changed = client->pers.game_help1changed;
				saved_resp.coopRespawn.game_help2changed = client->pers.game_help2changed;
				saved_resp.coopRespawn.helpchanged = client->pers.helpchanged;
				client->pers = saved_resp.coopRespawn;
			} else {
				// fix weapon
				if (!client->pers.weapon)
					client->pers.weapon = client->pers.lastWeapon;
			}
		}

		ClientUserinfoChanged(ent, userInfo);

		if (coop->integer) {
			if (saved_resp.score > client->pers.score)
				client->pers.score = saved_resp.score;
		} else {
			memset(&saved_resp, 0, sizeof(saved_resp));
			memset(&saved_sess, 0, sizeof(saved_sess));
			client->sess.team = TEAM_FREE;
		}
	}

	// clear everything but the persistant data
	saved_pers = client->pers;
	memset(client, 0, sizeof(*client));
	client->pers = saved_pers;
	client->resp = saved_resp;
	client->sess = saved_sess;

	// on a new, fresh spawn (always in DM, clear inventory
	// or new spawns in SP/coop)
	if (client->pers.health <= 0)
		InitClientPersistant(ent, client);

	// fix level switch issue
	ent->client->pers.connected = true;

	// slow time will be unset here
	globals.server_flags &= ~SERVER_FLAG_SLOW_TIME;

	// copy some data from the client to the entity
	FetchClientEntData(ent);

	// clear entity values
	ent->groundEntity = nullptr;
	ent->client = &game.clients[index];
	ent->takeDamage = true;
	ent->moveType = MOVETYPE_WALK;
	ent->viewHeight = DEFAULT_VIEWHEIGHT;
	ent->inUse = true;
	ent->className = "player";
	ent->mass = 200;
	ent->solid = SOLID_BBOX;
	ent->deadFlag = false;
	ent->airFinished = level.time + 12_sec;
	ent->clipMask = MASK_PLAYERSOLID;
	ent->model = "players/male/tris.md2";
	ent->die = player_die;
	ent->waterlevel = WATER_NONE;
	ent->watertype = CONTENTS_NONE;
	ent->flags &= ~(FL_NO_KNOCKBACK | FL_ALIVE_KNOCKBACK_ONLY | FL_NO_DAMAGE_EFFECTS | FL_SAM_RAIMI);
	ent->svFlags &= ~SVF_DEADMONSTER;
	ent->svFlags |= SVF_PLAYER;
	ent->client->pers.last_spawn_time = level.time;
	ent->client->timeResidual = level.time + 1_sec;

	ent->mins = PLAYER_MINS;
	ent->maxs = PLAYER_MAXS;

	ent->client->pers.lives = lives;

	// clear playerstate values
	memset(&ent->client->ps, 0, sizeof(client->ps));

	char val[MAX_INFO_VALUE];
	gi.Info_ValueForKey(ent->client->pers.userInfo, "fov", val, sizeof(val));
	ent->client->ps.fov = clamp((float)strtoul(val, nullptr, 10), 1.f, 160.f);

	ent->client->ps.pmove.viewHeight = ent->viewHeight;

	if (!G_ShouldPlayersCollide(false))
		ent->clipMask &= ~CONTENTS_PLAYER;

	if (client->pers.weapon)
		client->ps.gunindex = gi.modelindex(client->pers.weapon->view_model);
	else
		client->ps.gunindex = 0;
	client->ps.gunskin = 0;

	// clear entity state values
	ent->s.effects = EF_NONE;
	ent->s.modelindex = MODELINDEX_PLAYER;	// will use the skin specified model
	ent->s.modelindex2 = MODELINDEX_PLAYER; // custom gun model
	// sknum is player num and weapon number
	// weapon number will be added in changeweapon
	P_AssignClientSkinnum(ent);

	CalculateRanks();

	ent->s.frame = 0;

	PutClientOnSpawnPoint(ent, spawn_origin, spawn_angles);

	// [Paril-KEX] set up world fog & send it instantly
	ent->client->pers.wanted_fog = {
		world->fog.density,
		world->fog.color[0],
		world->fog.color[1],
		world->fog.color[2],
		world->fog.sky_factor
	};
	ent->client->pers.wanted_heightfog = {
		{ world->heightfog.start_color[0], world->heightfog.start_color[1], world->heightfog.start_color[2], world->heightfog.start_dist },
		{ world->heightfog.end_color[0], world->heightfog.end_color[1], world->heightfog.end_color[2], world->heightfog.end_dist },
		world->heightfog.falloff,
		world->heightfog.density
	};
	P_ForceFogTransition(ent, true);
	
	// spawn as spectator
	if (!ClientIsPlaying(client) || eliminated) {
		FreeFollower(ent);

		MoveClientToFreeCam(ent);
		ent->client->ps.stats[STAT_SHOW_STATUSBAR] = 0;
		if (!ent->client->initial_menu_shown)
			ent->client->initial_menu_delay = level.time + 10_hz;
		ent->client->eliminated = eliminated;
		gi.linkentity(ent);
		return;
	}
	ent->client->ps.stats[STAT_SHOW_STATUSBAR] = 1;

	// [Paril-KEX] a bit of a hack, but landmark spawns can sometimes cause
	// intersecting spawns, so we'll do a sanity check here...
	if (spawn_from_begin) {
		if (coop->integer) {
			gentity_t *collision = G_UnsafeSpawnPosition(ent->s.origin, true);

			if (collision) {
				gi.linkentity(ent);

				if (collision->client) {
					// we spawned in somebody else, so we're going to change their spawn position
					bool lm = false;
					SelectSpawnPoint(collision, spawn_origin, spawn_angles, true, lm);
					PutClientOnSpawnPoint(collision, spawn_origin, spawn_angles);
				}
				// else, no choice but to accept where ever we spawned :(
			}
		}

		// give us one (1) free fall ticket even if
		// we didn't spawn from landmark
		ent->client->landmark_free_fall = true;
	}

	gi.linkentity(ent);

	if (!KillBox(ent, true, MOD_TELEFRAG_SPAWN)) { // could't spawn in?
	}

	// my tribute to cash's level-specific hacks. I hope I live
	// up to his trailblazing cheese.
	if (Q_strcasecmp(level.mapname, "rboss") == 0) {
		// if you get on to rboss in single player or coop, ensure
		// the player has the nuke key. (not in DM)
		if (!deathmatch->integer)
			client->pers.inventory[IT_KEY_NUKE] = 1;
	}
	
	// force the current weapon up
	if (GTF(GTF_ARENA) && client->pers.inventory[IT_WEAPON_RLAUNCHER])
		client->newWeapon = &itemList[IT_WEAPON_RLAUNCHER];
	else
		client->newWeapon = client->pers.weapon;
	Change_Weapon(ent);

	if (was_waiting_for_respawn)
		G_PostRespawn(ent);
}

/*
=====================
ClientBeginDeathmatch

A client has just connected to the server in
deathmatch mode, so clear everything out before starting them.
=====================
*/
static void ClientBeginDeathmatch(gentity_t *ent) {
	InitGEntity(ent);

	// make sure we have a known default
	ent->svFlags |= SVF_PLAYER;

	InitClientResp(ent->client);

	// locate ent at a spawn point
	ClientSpawn(ent);

	if (level.intermissionTime) {
		MoveClientToIntermission(ent);
	} else {
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

static void G_SetLevelEntry() {
	if (deathmatch->integer)
		return;
	// map is a hub map, so we shouldn't bother tracking any of this.
	// the next map will pick up as the start.
	if (level.hub_map)
		return;

	level_entry_t *found_entry = nullptr;
	int32_t highest_order = 0;

	for (size_t i = 0; i < MAX_LEVELS_PER_UNIT; i++) {
		level_entry_t *entry = &game.level_entries[i];

		highest_order = max(highest_order, entry->visit_order);

		if (!strcmp(entry->map_name, level.mapname) || !*entry->map_name) {
			found_entry = entry;
			break;
		}
	}

	if (!found_entry) {
		gi.Com_PrintFmt("WARNING: more than {} maps in unit, can't track the rest\n", MAX_LEVELS_PER_UNIT);
		return;
	}

	level.entry = found_entry;
	Q_strlcpy(level.entry->map_name, level.mapname, sizeof(level.entry->map_name));

	// we're visiting this map for the first time, so
	// mark it in our order as being recent
	if (!*level.entry->pretty_name) {
		Q_strlcpy(level.entry->pretty_name, level.levelName, sizeof(level.entry->pretty_name));
		level.entry->visit_order = highest_order + 1;

		// give all of the clients an extra life back
		if (g_coop_enable_lives->integer)
			for (auto ec : active_clients())
				ec->client->pers.lives = min(g_coop_num_lives->integer + 1, ec->client->pers.lives + 1);
	}

	// scan for all new maps we can go to, for secret levels
	gentity_t *changelevel = nullptr;
	while ((changelevel = G_FindByString<&gentity_t::className>(changelevel, "target_changelevel"))) {
		if (!changelevel->map || !*changelevel->map)
			continue;

		// next unit map, don't count it
		if (strchr(changelevel->map, '*'))
			continue;

		const char *level = strchr(changelevel->map, '+');

		if (level)
			level++;
		else
			level = changelevel->map;

		// don't include end screen levels
		if (strstr(level, ".cin") || strstr(level, ".pcx"))
			continue;

		size_t level_length;

		const char *spawnpoint = strchr(level, '$');

		if (spawnpoint)
			level_length = spawnpoint - level;
		else
			level_length = strlen(level);

		// make an entry for this level that we may or may not visit
		level_entry_t *found_entry = nullptr;

		for (size_t i = 0; i < MAX_LEVELS_PER_UNIT; i++) {
			level_entry_t *entry = &game.level_entries[i];

			if (!*entry->map_name || !strncmp(entry->map_name, level, level_length)) {
				found_entry = entry;
				break;
			}
		}

		if (!found_entry) {
			gi.Com_PrintFmt("WARNING: more than {} maps in unit, can't track the rest\n", MAX_LEVELS_PER_UNIT);
			return;
		}

		Q_strlcpy(found_entry->map_name, level, min(level_length + 1, sizeof(found_entry->map_name)));
	}
}

/*
=================
ClientIsPlaying
=================
*/
bool ClientIsPlaying(gclient_t *cl) {
	if (!cl) return false;

	if (!deathmatch->integer)
		return true;
	
	return !(!cl->sess.team || cl->sess.team == TEAM_SPECTATOR);
}

/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the game.  This will happen every level load.
============
*/
void ClientBegin(gentity_t *ent) {
	ent->client = game.clients + (ent - g_entities - 1);
	ent->client->awaiting_respawn = false;
	ent->client->respawn_timeout = 0_ms;

	// set inactivity timer
	gtime_t cv = gtime_t::from_sec(g_inactivity->integer);
	if (cv) {
		if (cv < 15_sec) cv = 15_sec;
		ent->client->sess.inactivityTime = level.time + cv;
		ent->client->sess.inactivityWarning = false;
	}

	// [Paril-KEX] we're always connected by this point...
	ent->client->pers.connected = true;

	if (deathmatch->integer) {
		ClientBeginDeathmatch(ent);

		// count current clients and rank for scoreboard
		CalculateRanks();
		return;
	}

	// [Paril-KEX] set enter time now, so we can send messages slightly
	// after somebody first joins
	ent->client->resp.enterTime = level.time;
	ent->client->pers.spawned = true;

	// if there is already a body waiting for us (a loadgame), just
	// take it, otherwise spawn one from scratch
	if (ent->inUse) {
		// the client has cleared the client side viewangles upon
		// connecting to the server, which is different than the
		// state when the game is saved, so we need to compensate
		// with deltaangles
		ent->client->ps.pmove.delta_angles = ent->client->ps.viewangles;
	} else {
		// a spawn point will completely reinitialize the entity
		// except for the persistant data that was initialized at
		// ClientConnect() time
		InitGEntity(ent);
		ent->className = "player";
		InitClientResp(ent->client);
		spawn_from_begin = true;
		ClientSpawn(ent);
		spawn_from_begin = false;

		if (!ent->client->sess.inGame)
			BroadcastTeamChange(ent, -1, false, false);
	}

	// make sure we have a known default
	ent->svFlags |= SVF_PLAYER;

	if (level.intermissionTime) {
		MoveClientToIntermission(ent);
	} else {
		// send effect if in a multiplayer game
		if (game.maxclients > 1 && !(ent->svFlags & SVF_NOCLIENT))
			gi.LocBroadcast_Print(PRINT_HIGH, "$g_entered_game", ent->client->sess.netName);
	}

	level.coopScalePlayers++;
	G_Monster_CheckCoopHealthScaling();

	// make sure all view stuff is valid
	ClientEndServerFrame(ent);

	// [Paril-KEX] send them goal, if needed
	G_PlayerNotifyGoal(ent);

	// [Paril-KEX] we're going to set this here just to be certain
	// that the level entry timer only starts when a player is actually
	// *in* the level
	G_SetLevelEntry();

	ent->client->sess.inGame = true;
}

/*
================
P_GetLobbyUserNum
================
*/
unsigned int P_GetLobbyUserNum(const gentity_t *player) {
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
static std::string G_EncodedPlayerName(gentity_t *player) {
	unsigned int playernum = P_GetLobbyUserNum(player);
	return std::string("##P") + std::to_string(playernum);
}

/*
===========
ClientUserInfoChanged

called whenever the player updates a userInfo variable.
============
*/
void ClientUserinfoChanged(gentity_t *ent, const char *userInfo) {
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
	ent->client->sess.skinIconIndex = gi.imageindex(iconPath.c_str());

	int playernum = ent - g_entities - 1;

	// combine name and skin into a configstring
	if (Teams())
		AssignPlayerSkin(ent, val);
	else {
		gi.configstring(CS_PLAYERSKINS + playernum, G_Fmt("{}\\{}", ent->client->sess.netName, val).data());
	}

	//  set player name field (used in id_state view)
	gi.configstring(CONFIG_CHASE_PLAYER_NAME + playernum, ent->client->sess.netName);

	// [Kex] netname is used for a couple of other things, so we update this after those.
	if (!(ent->svFlags & SVF_BOT))
		Q_strlcpy(ent->client->pers.netname, G_EncodedPlayerName(ent).c_str(), sizeof(ent->client->pers.netname));

	// fov
	gi.Info_ValueForKey(userInfo, "fov", val, sizeof(val));
	ent->client->ps.fov = clamp((float)strtoul(val, nullptr, 10), 1.f, 160.f);

	// handedness
	if (gi.Info_ValueForKey(userInfo, "hand", val, sizeof(val))) {
		ent->client->pers.hand = static_cast<handedness_t>(clamp(atoi(val), (int32_t)RIGHT_HANDED, (int32_t)CENTER_HANDED));
	} else {
		ent->client->pers.hand = RIGHT_HANDED;
	}

	// [Paril-KEX] auto-switch
	if (gi.Info_ValueForKey(userInfo, "autoswitch", val, sizeof(val))) {
		ent->client->pers.autoswitch = static_cast<auto_switch_t>(clamp(atoi(val), (int32_t)auto_switch_t::SMART, (int32_t)auto_switch_t::NEVER));
	} else {
		ent->client->pers.autoswitch = auto_switch_t::SMART;
	}

	if (gi.Info_ValueForKey(userInfo, "autoshield", val, sizeof(val))) {
		ent->client->pers.autoshield = atoi(val);
	} else {
		ent->client->pers.autoshield = -1;
	}

	// [Paril-KEX] wants bob
	if (gi.Info_ValueForKey(userInfo, "bobskip", val, sizeof(val))) {
		ent->client->pers.bob_skip = val[0] == '1';
	} else {
		ent->client->pers.bob_skip = false;
	}

	// save off the userInfo in case we want to check something later
	Q_strlcpy(ent->client->pers.userInfo, userInfo, sizeof(ent->client->pers.userInfo));
}

static inline bool IsSlotIgnored(gentity_t *slot, gentity_t **ignore, size_t num_ignore) {
	for (size_t i = 0; i < num_ignore; i++)
		if (slot == ignore[i])
			return true;

	return false;
}

static inline gentity_t *ClientChooseSlot_Any(gentity_t **ignore, size_t num_ignore) {
	for (size_t i = 0; i < game.maxclients; i++)
		if (!IsSlotIgnored(globals.gentities + i + 1, ignore, num_ignore) && !game.clients[i].pers.connected)
			return globals.gentities + i + 1;

	return nullptr;
}

static inline gentity_t *ClientChooseSlot_Coop(const char *userInfo, const char *socialID, bool isBot, gentity_t **ignore, size_t num_ignore) {
	char name[MAX_INFO_VALUE] = { 0 };
	gi.Info_ValueForKey(userInfo, "name", name, sizeof(name));

	// the host should always occupy slot 0, some systems rely on this
	// (CHECK: is this true? is it just bots?)
	{
		size_t num_players = 0;

		for (size_t i = 0; i < game.maxclients; i++)
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
		gentity_t *slot = nullptr;
		size_t total = 0;
	} matches[SLOT_MATCH_TYPES];

	for (size_t i = 0; i < game.maxclients; i++) {
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
				matches[i].slot->s.modelindex = MODELINDEX_PLAYER;
				matches[i].slot->solid = SOLID_BBOX;

				InitGEntity(matches[i].slot);
				matches[i].slot->className = "player";
				InitClientResp(matches[i].slot->client);
				spawn_from_begin = true;
				ClientSpawn(matches[i].slot);
				spawn_from_begin = false;

				// make sure we have a known default
				matches[i].slot->svFlags |= SVF_PLAYER;

				matches[i].slot->sv.init = false;
				matches[i].slot->className = "player";
				matches[i].slot->client->pers.connected = true;
				matches[i].slot->client->pers.spawned = true;
				P_AssignClientSkinnum(matches[i].slot);
				gi.linkentity(matches[i].slot);
			}

			return matches[i].slot;
		}
	}

	// in the case where we can't find a match, we're probably a new
	// player, so pick a slot that hasn't been occupied yet
	for (size_t i = 0; i < game.maxclients; i++)
		if (!IsSlotIgnored(globals.gentities + i + 1, ignore, num_ignore) && !game.clients[i].pers.userInfo[0]) {
			gi.Com_PrintFmt("coop slot {} issuing new for {}+{}\n", i + 1, name, socialID);
			return globals.gentities + i + 1;
		}

	// all slots have some player data in them, we're forced to replace one.
	gentity_t *any_slot = ClientChooseSlot_Any(ignore, num_ignore);

	gi.Com_PrintFmt("coop slot {} any slot for {}+{}\n", !any_slot ? -1 : (ptrdiff_t)(any_slot - globals.gentities), name, socialID);

	return any_slot;
}

// [Paril-KEX] for coop, we want to try to ensure that players will always get their
// proper slot back when they connect.
gentity_t *ClientChooseSlot(const char *userInfo, const char *socialID, bool isBot, gentity_t **ignore, size_t num_ignore, bool cinematic) {
	// coop and non-bots is the only thing that we need to do special behavior on
	if (!cinematic && coop->integer && !isBot)
		return ClientChooseSlot_Coop(userInfo, socialID, isBot, ignore, num_ignore);

	// just find any free slot
	return ClientChooseSlot_Any(ignore, num_ignore);
}

static inline bool CheckBanned(gentity_t *ent, char *userInfo, const char *socialID) {
	// currently all bans are in Steamworks and Epic, don't bother if not from there
	if (socialID[0] != 'S' && socialID[0] != 'E')
		return false;

	// Israel
	if (!Q_strcasecmp(socialID, "Steamworks-76561198026297488")) {
		gi.Info_SetValueForKey(userInfo, "rejmsg", "Antisemite detected!\n");

		if (host && host->client) {
			if (level.time > host->client->last_banned_message_time + 10_sec) {

				char name[MAX_INFO_VALUE] = { 0 };
				gi.Info_ValueForKey(userInfo, "name", name, sizeof(name));

				gi.LocClient_Print(host, PRINT_TTS, "ANTISEMITE DETECTED ({})!\n", name);
				host->client->last_banned_message_time = level.time;
				gi.LocBroadcast_Print(PRINT_CHAT, "{}: God Bless Palestine\n", name);
			}
		}

		gi.local_sound(ent, CHAN_AUTO, gi.soundindex("world/klaxon3.wav"), 1, ATTN_NONE, 0);
		gi.AddCommandString(G_Fmt("kick {}\n", ent - g_entities - 1).data());
		return true;
	}

	// Kirlomax
	if (!Q_strcasecmp(socialID, "Steamworks-76561198001774610")) {
		gi.Info_SetValueForKey(userInfo, "rejmsg", "WARNING! KNOWN CHEATER DETECTED\n");

		if (host && host->client) {
			if (level.time > host->client->last_banned_message_time + 10_sec) {

				char name[MAX_INFO_VALUE] = { 0 };
				gi.Info_ValueForKey(userInfo, "name", name, sizeof(name));

				gi.LocClient_Print(host, PRINT_TTS, "WARNING! KNOWN CHEATER DETECTED ({})!\n", name);
				host->client->last_banned_message_time = level.time;
				gi.LocBroadcast_Print(PRINT_CHAT, "{}: I am a known cheater, banned from all servers.\n", name);
			}
		}

		gi.local_sound(ent, CHAN_AUTO, gi.soundindex("world/klaxon3.wav"), 1, ATTN_NONE, 0);
		gi.AddCommandString(G_Fmt("kick {}\n", ent - g_entities - 1).data());
		return true;
	}

	// Model192
	if (!Q_strcasecmp(socialID, "Steamworks-76561197972296343")) {
		gi.Info_SetValueForKey(userInfo, "rejmsg", "WARNING! MOANERTONE DETECTED\n");

		if (host && host->client) {
			if (level.time > host->client->last_banned_message_time + 10_sec) {

				char name[MAX_INFO_VALUE] = { 0 };
				gi.Info_ValueForKey(userInfo, "name", name, sizeof(name));

				gi.LocClient_Print(host, PRINT_TTS, "WARNING! MOANERTONE DETECTED ({})!\n", name);
				host->client->last_banned_message_time = level.time;
				gi.LocBroadcast_Print(PRINT_CHAT, "{}: Listen up, I have something to moan about.\n", name);
			}
		}

		gi.local_sound(ent, CHAN_AUTO, gi.soundindex("world/klaxon3.wav"), 1, ATTN_NONE, 0);
		gi.AddCommandString(G_Fmt("kick {}\n", ent - g_entities - 1).data());
		return true;
	}

	// Dalude
	if (!Q_strcasecmp(socialID, "Steamworks-76561199001991246") || !Q_strcasecmp(socialID, "EOS-07e230c273be4248bbf26c89033923c1")) {
		ent->client->sess.is_888 = true;
		gi.Info_SetValueForKey(userInfo, "rejmsg", "Fake 888 Agent detected!\n");
		gi.Info_SetValueForKey(userInfo, "name", "Fake 888 Agent");

		if (host && host->client) {
			if (level.time > host->client->last_banned_message_time + 10_sec) {

				char name[MAX_INFO_VALUE] = { 0 };
				gi.Info_ValueForKey(userInfo, "name", name, sizeof(name));

				gi.LocClient_Print(host, PRINT_TTS, "FAKE 888 AGENT DETECTED ({})!\n", name);
				host->client->last_banned_message_time = level.time;
				gi.LocBroadcast_Print(PRINT_CHAT, "{}: bejesus, what a lovely lobby! certainly better than 888's!\n", name);
			}
		}
		gi.local_sound(ent, CHAN_AUTO, gi.soundindex("world/klaxon3.wav"), 1, ATTN_NONE, 0);
		gi.AddCommandString(G_Fmt("kick {}\n", ent - g_entities - 1).data());
		return true;
	}
	return false;
}

static void CheckSpecial(gentity_t *ent, const char *socialID) {
	// 30day
	if (!Q_strcasecmp(socialID, "Steamworks-76561199535711299")) {
		ent->client->sess.is_30day = true;
	}
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
bool ClientConnect(gentity_t *ent, char *userInfo, const char *socialID, bool isBot) {

#if 0
	// check to see if they are on the banned IP list
	char value[MAX_INFO_VALUE] = { 0 };
	gi.Info_ValueForKey(userInfo, "ip", value, sizeof(value));
	if (G_FilterPacket(value)) {
		gi.Info_SetValueForKey(userInfo, "rejmsg", "Banned.");
		return false;
	}
#endif
	
	if (!isBot) {
		if (CheckBanned(ent, userInfo, socialID))
			return false;
		CheckSpecial(ent, socialID);

		ApplyPlayerStatus(ent, socialID);
	}

	ent->client->sess.team = deathmatch->integer ? TEAM_NONE : TEAM_FREE;

	// they can connect
	ent->client = game.clients + (ent - g_entities - 1);

	// set up userInfo early
	ClientUserinfoChanged(ent, userInfo);

	// if there is already a body waiting for us (a loadgame), just
	// take it, otherwise spawn one from scratch
	if (ent->inUse == false) {
		// clear the respawning variables

		if (!ent->client->sess.initialised && !ent->client->sess.team) {
			ent->client->pers.introTime = 3_sec;

			//gi.Com_PrintFmt_("ClientConnect: {} q={}\n", ent->client->sess.netName, ent->client->sess.matchQueued);
			// force team join
			ent->client->sess.team = deathmatch->integer ? TEAM_NONE : TEAM_FREE;
			//InitPlayerTeam(ent);
			ent->client->sess.pc.show_id = true;
			ent->client->sess.pc.show_timer = true;
			ent->client->sess.pc.show_fragmessages = true;
			ent->client->sess.pc.killbeep_num = 1;
			ent->client->sess.pc.follow_killer = false;
			ent->client->sess.pc.follow_powerup = false;

			InitClientResp(ent->client);

			ent->client->sess.playStartRealTime = GetCurrentRealTimeMillis();
		}

		if (!game.autosaved || !ent->client->pers.weapon)
			InitClientPersistant(ent, ent->client);
	}

	// make sure we start with known default(s)
	ent->svFlags = SVF_PLAYER;

	if (isBot) {
		ent->svFlags |= SVF_BOT;
		ent->client->sess.is_a_bot = true;

		if (bot_name_prefix->string[0] && *bot_name_prefix->string) {
			char oldname[MAX_INFO_VALUE];
			char newname[MAX_NETNAME];

			gi.Info_ValueForKey(userInfo, "name", oldname, sizeof(oldname));
			strcpy(newname, bot_name_prefix->string);
			Q_strlcat(newname, oldname, sizeof(oldname));
			gi.Info_SetValueForKey(userInfo, "name", newname);
		}
	}

	Q_strlcpy(ent->client->sess.socialID, socialID, sizeof(ent->client->sess.socialID));

	char value[MAX_INFO_VALUE] = { 0 };
	// [Paril-KEX] fetch name because now netname is kinda unsuitable
	gi.Info_ValueForKey(userInfo, "name", value, sizeof(value));
	Q_strlcpy(ent->client->sess.netName, value, sizeof(ent->client->sess.netName));
	
	ent->client->sess.skillRating = 0;
	
	if (!isBot)
		ClientConfig_Init(ent->client, ent->client->sess.socialID, value, gt_short_name_upper[g_gametype->integer]);

	if (ent->client->sess.banned) {
		gi.LocBroadcast_Print(PRINT_HIGH, "BANNED PLAYER {} connects.\n", value);
		gi.AddCommandString(G_Fmt("kick {}\n", ent - g_entities - 1).data());
		return false;
	}
	
	if (game.maxclients >= 1) {
		if (!isBot && ent->client->sess.skillRating > 0) {
			gi.LocBroadcast_Print(PRINT_HIGH, "{} connects. (SR: {})\n", value, ent->client->sess.skillRating);
		} else {
			gi.LocBroadcast_Print(PRINT_HIGH, "$g_player_connected", value);
		}
	}

	if (level.endmatch_grace)
		level.endmatch_grace = 0_ms;

	// set skin
	char val[MAX_INFO_VALUE] = { 0 };
	if (!gi.Info_ValueForKey(userInfo, "skin", val, sizeof(val)))
		Q_strlcpy(val, "male/grunt", sizeof(val));
	if (Q_strncasecmp(ent->client->sess.skinName, val, sizeof(ent->client->sess.skinName))) {
		Q_strlcpy(ent->client->sess.skinName, ClientSkinOverride(val), sizeof(ent->client->sess.skinName));
		//ent->client->sess.skinName = ClientSkinOverride(val);
		ent->client->sess.skinIconIndex = gi.imageindex(G_Fmt("/players/{}_i", ent->client->sess.skinName).data());
	}

	ent->client->pers.connected = true;

	ent->client->sess.inGame = true;

	// entity 1 is always server host, so make admin
	if (ent == &g_entities[1])
		ent->client->sess.admin = true;
	else {
		//TODO: check admins.txt for socialID

	}

	// Detect if client is on a console system
	if (socialID && (
		_strnicmp(socialID, "PSN", 3) == 0 ||
		_strnicmp(socialID, "NX", 2) == 0 ||
		_strnicmp(socialID, "GDK", 3) == 0)) {
		ent->client->sess.consolePlayer = true;
	} else {
		ent->client->sess.consolePlayer = false;
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	//PCfg_WriteConfig(ent);
	//PCfg_ClientInitPConfig(ent);

	// [Paril-KEX] force a state update
	ent->sv.init = false;

	gi.WriteByte(svc_stufftext);
	gi.WriteString("bind F3 readyup\n");
	//gi.WriteString("say hello\n");
	gi.unicast(ent, true);

	return true;
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.
============
*/
void ClientDisconnect(gentity_t *ent) {
	if (!ent->client)
		return;

	if (ent->client->pers.spawned) {
		TossClientItems(ent);
		PlayerTrail_Destroy(ent);
	}

	// make sure no trackers are still hurting us.
	if (ent->client->trackerPainTime)
		RemoveAttackingPainDaemons(ent);

	if (ent->client->ownedSphere) {
		if (ent->client->ownedSphere->inUse)
			FreeEntity(ent->client->ownedSphere);
		ent->client->ownedSphere = nullptr;
	}

	// send effect
	if (!(ent->svFlags & SVF_NOCLIENT)) {
		gi.WriteByte(svc_muzzleflash);
		gi.WriteEntity(ent);
		gi.WriteByte(MZ_LOGOUT);
		gi.multicast(ent->s.origin, MULTICAST_PVS, false);
	}

	// free any followers
	FreeClientFollowers(ent);

	G_RevertVote(ent->client);

	P_SaveGhostSlot(ent);

	gi.unlinkentity(ent);
	ent->s.modelindex = 0;
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
				ec->client->menuTime = level.time;
	}

	if (ent->client->pers.connected)
		if (ent->client->sess.netName && ent->client->sess.netName[0] && ent->client->sess.initialised)
			gi.LocBroadcast_Print(PRINT_CENTER, "{} disconnected.",
				ent->client->sess.netName);
}

//==============================================================

static trace_t G_PM_Clip(const vec3_t &start, const vec3_t *mins, const vec3_t *maxs, const vec3_t &end, contents_t mask) {
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
static void P_FallingDamage(gentity_t *ent, const pmove_t &pm) {
	int	   damage;
	vec3_t dir;

	// dead stuff can't crater
	if (ent->health <= 0 || ent->deadFlag)
		return;

	if (ent->s.modelindex != MODELINDEX_PLAYER)
		return; // not in the player model

	if (ent->moveType == MOVETYPE_NOCLIP || ent->moveType == MOVETYPE_FREECAM)
		return;

	// never take falling damage if completely underwater
	if (pm.waterlevel == WATER_UNDER)
		return;

	//  never take damage if just release grapple or on grapple
	if (ent->client->grappleReleaseTime >= level.time ||
		(ent->client->grappleEnt &&
			ent->client->grappleState > GRAPPLE_STATE_FLY))
		return;

	float delta = pm.impact_delta;

	delta = delta * delta * 0.0001f;

	if (pm.waterlevel == WATER_WAIST)
		delta *= 0.25f;
	if (pm.waterlevel == WATER_FEET)
		delta *= 0.5f;

	if (delta < 1)
		return;

	// restart footstep timer
	ent->client->bobTime = 0;

	if (ent->client->landmark_free_fall) {
		delta = min(30.f, delta);
		ent->client->landmark_free_fall = false;
		ent->client->landmark_noise_time = level.time + 100_ms;
	}

	if (delta < 15) {
		if (!(pm.s.pm_flags & PMF_ON_LADDER))
			ent->s.event = EV_FOOTSTEP;
		return;
	}

	ent->client->fallValue = delta * 0.5f;
	if (ent->client->fallValue > 40)
		ent->client->fallValue = 40;
	ent->client->fallTime = level.time + FALL_TIME();

	int med_min = RS(RS_Q3A) ? 40 : 30;
	int far_min = RS(RS_Q3A) ? 61 : 55;

	if (delta > med_min) {
		if (delta >= far_min)
			ent->s.event = EV_FALL_FAR;
		else
			ent->s.event = EV_FALL_MEDIUM;
		if (g_fallingDamage->integer && notGTF(GTF_ARENA)) {
			ent->pain_debounce_time = level.time + FRAME_TIME_S; // no normal pain sound
			if (RS(RS_Q3A))
				damage = ent->s.event == EV_FALL_FAR ? 10 : 5;
			else {
				damage = (int)((delta - 30) / 3);	// 2 originally
				if (damage < 1)
					damage = 1;
			}
			dir = { 0, 0, 1 };

			Damage(ent, world, world, dir, ent->s.origin, vec3_origin, damage, 0, DAMAGE_NONE, MOD_FALLING);
		}
	} else
		ent->s.event = EV_FALL_SHORT;

	// Paril: falling damage noises alert monsters
	if (ent->health)
		PlayerNoise(ent, pm.s.origin, PNOISE_SELF);
}

inline void PreviousMenuItem(gentity_t *ent);
inline void NextMenuItem(gentity_t *ent);
static bool HandleMenuMovement(gentity_t *ent, usercmd_t *ucmd) {
	if (!ent->client->menu)
		return false;

	// [Paril-KEX] handle menu movement
	int32_t menu_sign = ucmd->forwardmove > 0 ? 1 : ucmd->forwardmove < 0 ? -1 : 0;

	if (ent->client->menu_sign != menu_sign) {
		ent->client->menu_sign = menu_sign;

		if (menu_sign > 0) {
			PreviousMenuItem(ent);
			return true;
		} else if (menu_sign < 0) {
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
static bool ClientInactivityTimer(gentity_t *ent) {
	gtime_t cv = gtime_t::from_sec(g_inactivity->integer);

	if (!ent->client)
		return true;
	
	if (cv && cv < 15_sec) cv = 15_sec;
	if (!ent->client->sess.inactivityTime) {
		ent->client->sess.inactivityTime = level.time + cv;
		ent->client->sess.inactivityWarning = false;
		return true;
	}
	if (!deathmatch->integer || !cv || !ClientIsPlaying(ent->client) || ent->client->eliminated || ent->client->sess.is_a_bot || ent->s.number == 0) {
		// give everyone some time, so if the operator sets g_inactivity during
		// gameplay, everyone isn't kicked
		ent->client->sess.inactivityTime = level.time + 1_min;
		ent->client->sess.inactivityWarning = false;
	} else if (ent->client->latchedButtons & BUTTON_ANY) {
		ent->client->sess.inactivityTime = level.time + cv;
		ent->client->sess.inactivityWarning = false;
	} else {
		if (level.time > ent->client->sess.inactivityTime) {
			gi.LocClient_Print(ent, PRINT_CENTER, "You have been removed from the match\ndue to inactivity.\n");
			SetTeam(ent, TEAM_SPECTATOR, true, true, false);
			return false;
		}
		if (level.time > ent->client->sess.inactivityTime - gtime_t::from_sec(10) && !ent->client->sess.inactivityWarning) {
			ent->client->sess.inactivityWarning = true;
			gi.LocClient_Print(ent, PRINT_CENTER, "Ten seconds until inactivity trigger!\n");	//TODO: "$g_ten_sec_until_drop");
			gi.local_sound(ent, CHAN_AUTO, gi.soundindex("world/fish.wav"), 1, ATTN_NONE, 0);
		}
	}

	return true;
}

static void ClientTimerActions_ApplyRegeneration(gentity_t *ent) {
	gclient_t *cl = ent->client;
	if (!cl)
		return;

	if (ent->health <= 0 || ent->client->eliminated)
		return;

	if (cl->powerupTime.regeneration <= level.time)
		return;

	if (g_vampiric_damage->integer || !game.spawnHealth)
		return;

	if (CombatIsDisabled())
		return;

	bool	noise = false;
	float	volume = cl->powerupTime.silencerShots ? 0.2f : 1.0;
	int		max = cl->pers.max_health;
	int		bonus = 0;

	if (ent->health < max) {
		bonus = 15;
	} else if (ent->health < max * 2) {
		bonus = 5;
	}

	if (!bonus)
		return;

	ent->health += bonus;
	if (ent->health > max)
		ent->health = max;
	gi.sound(ent, CHAN_AUX, gi.soundindex("items/regen.wav"), volume, ATTN_NORM, 0);
	cl->pu_regen_time_blip = level.time + 100_ms;
}

/*
==================
ClientTimerActions

Actions that happen once a second
==================
*/
static void ClientTimerActions(gentity_t *ent) {
	if (ent->client->timeResidual > level.time)
		return;

	if (RS(RS_Q3A)) {
		// count down health when over max
		if (ent->health > ent->client->pers.max_health)
			ent->health--;

		// count down armor when over max
		if (ent->client->pers.inventory[IT_ARMOR_COMBAT] > ent->client->pers.max_health)
			ent->client->pers.inventory[IT_ARMOR_COMBAT]--;
	} else {
		if (ent->client->pers.healthBonus > 0) {
			if (ent->health <= 0 || ent->health <= ent->client->pers.max_health) {
				ent->client->pers.healthBonus = 0;
			} else {
				ent->health--;
				ent->client->pers.healthBonus--;
			}
		}
	}
	ClientTimerActions_ApplyRegeneration(ent);
	ent->client->timeResidual = level.time + 1_sec;
}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame.
==============
*/
void OpenJoinMenu(gentity_t *ent);
void ClientThink(gentity_t *ent, usercmd_t *ucmd) {
	gclient_t *client;
	gentity_t *other;
	uint32_t   i;
	pmove_t	   pm;

	level.currentEntity = ent;
	client = ent->client;

	//no movement during map or match intermission
	if (level.timeoutActive > 0_ms) {
		client->resp.cmdAngles[0] = ucmd->angles[0];
		client->resp.cmdAngles[1] = ucmd->angles[1];
		client->resp.cmdAngles[2] = ucmd->angles[2];
		client->ps.pmove.pm_type = PM_FREEZE;
		return;
	}

	// [Paril-KEX] pass buttons through even if we are in intermission or
	// chasing.
	client->oldButtons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latchedButtons |= client->buttons & ~client->oldButtons;
	client->cmd = *ucmd;

	if (!client->initial_menu_shown && client->initial_menu_delay && level.time > client->initial_menu_delay) {
		if (!ClientIsPlaying(client) && (!client->sess.initialised || client->sess.inactiveStatus)) {
			if (ent->client->sess.admin && g_owner_push_scores->integer)
				Cmd_Score_f(ent);
			else
				//G_Menu_Join_Open(ent);
				OpenJoinMenu(ent);
			//if (!client->initial_menu_shown)
			//	gi.LocClient_Print(ent, PRINT_CHAT, "Welcome to {} v{}.\n", GAMEMOD_TITLE, GAMEMOD_VERSION);
			client->initial_menu_delay = 0_sec;
			client->initial_menu_shown = true;
		}
	}

	// check for queued follow targets
	if (!ClientIsPlaying(client)) {
		if (client->followQueuedTarget && level.time > client->followQueuedTime + 500_ms) {
			client->followTarget = client->followQueuedTarget;
			client->followUpdate = true;
			client->followQueuedTarget = nullptr;
			client->followQueuedTime = 0_sec;
			UpdateChaseCam(ent);
		}
	}
	
	// check for inactivity timer
	if (!ClientInactivityTimer(ent))
		return;

	if (g_quadhog->integer)
		if (ent->client->powerupTime.quadDamage > 0_sec && level.time >= ent->client->powerupTime.quadDamage)
			QuadHog_SetupSpawn(0_ms);
	
	if (ent->client->sess.teamJoinTime) {
		gtime_t delay = 5_sec;
		if (ent->client->sess.motdModificationCount != game.motdModificationCount) {
			if (level.time >= ent->client->sess.teamJoinTime + delay) {
				if (g_showmotd->integer && game.motd.size()) {
					gi.LocCenter_Print(ent, "{}", game.motd.c_str());
					delay += 5_sec;
					ent->client->sess.motdModificationCount = game.motdModificationCount;
				}
			}
		}
		if (!ent->client->sess.showed_help && g_showhelp->integer) {
			if (level.time >= ent->client->sess.teamJoinTime + delay) {
				if (g_quadhog->integer) {
					gi.LocClient_Print(ent, PRINT_CENTER, "QUAD HOG\nFind the Quad Damage to become the Quad Hog!\nScore by fragging the Quad Hog or fragging while Quad Hog.");
				} else if (g_vampiric_damage->integer) {
					gi.LocClient_Print(ent, PRINT_CENTER, "VAMPIRIC DAMAGE\nSurvive by inflicting damage on your foes,\ntheir pain makes you stronger!");
				} else if (g_frenzy->integer) {
					gi.LocClient_Print(ent, PRINT_CENTER, "WEAPONS FRENZY\nWeapons fire faster, rockets move faster, ammo regenerates.");
				} else if (g_nadefest->integer) {
					gi.LocClient_Print(ent, PRINT_CENTER, "NADE FEST\nOnly grenades, nothing else!");
				} else if (g_instagib->integer) {
					gi.LocClient_Print(ent, PRINT_CENTER, "INSTAGIB\nA rail-y good time!");
				}

				ent->client->sess.showed_help = true;
				//ent->client->sess.teamJoinTime = 0_sec;
			}
		}
	}

	if ((ucmd->buttons & BUTTON_CROUCH) && pm_config.n64_physics) {
		if (client->pers.n64_crouch_warn_times < 12 &&
			client->pers.n64_crouch_warning < level.time &&
			(++client->pers.n64_crouch_warn_times % 3) == 0) {
			client->pers.n64_crouch_warning = level.time + 10_sec;
			gi.LocClient_Print(ent, PRINT_CENTER, "$g_n64_crouching");
		}
	}

	if (level.intermissionTime || ent->client->awaiting_respawn) {
		client->ps.pmove.pm_type = PM_FREEZE;

		bool n64_sp = false;

		if (level.intermissionTime) {
			n64_sp = !deathmatch->integer && level.isN64;

			// can exit intermission after five seconds
			// Paril: except in N64. the camera handles it.
			// Paril again: except on unit exits, we can leave immediately after camera finishes
			if (level.changeMap && (!n64_sp || level.intermission.set) && level.time > level.intermissionTime + 5_sec && (ucmd->buttons & BUTTON_ANY))
				level.intermission.preExit = true;
		}

		if (!n64_sp)
			client->ps.pmove.viewHeight = ent->viewHeight = DEFAULT_VIEWHEIGHT;
		else
			client->ps.pmove.viewHeight = ent->viewHeight = 0;
		ent->moveType = MOVETYPE_FREECAM;
		return;
	}

	if (ent->client->followTarget) {
		client->resp.cmdAngles = ucmd->angles;
		ent->moveType = MOVETYPE_FREECAM;
	} else {

		// set up for pmove
		memset(&pm, 0, sizeof(pm));

		if (ent->moveType == MOVETYPE_FREECAM) {
			if (ent->client->menu) {
				client->ps.pmove.pm_type = PM_FREEZE;

				// [Paril-KEX] handle menu movement
				HandleMenuMovement(ent, ucmd);
			} else if (ent->client->awaiting_respawn)
				client->ps.pmove.pm_type = PM_FREEZE;
			else if (!ClientIsPlaying(ent->client) || client->eliminated)
				client->ps.pmove.pm_type = PM_SPECTATOR;
			else
				client->ps.pmove.pm_type = PM_NOCLIP;
		} else if (ent->moveType == MOVETYPE_NOCLIP) {
			client->ps.pmove.pm_type = PM_NOCLIP;
		} else if (ent->s.modelindex != MODELINDEX_PLAYER)
			client->ps.pmove.pm_type = PM_GIB;
		else if (ent->deadFlag)
			client->ps.pmove.pm_type = PM_DEAD;
		else if (ent->client->grappleState >= GRAPPLE_STATE_PULL)
			client->ps.pmove.pm_type = PM_GRAPPLE;
		else
			client->ps.pmove.pm_type = PM_NORMAL;

		// [Paril-KEX]
		if (!G_ShouldPlayersCollide(false) ||
			(CooperativeModeOn() && !(ent->clipMask & CONTENTS_PLAYER)) // if player collision is on and we're temporarily ghostly...
			)
			client->ps.pmove.pm_flags |= PMF_IGNORE_PLAYER_COLLISION;
		else
			client->ps.pmove.pm_flags &= ~PMF_IGNORE_PLAYER_COLLISION;

		// haste support
		client->ps.pmove.haste = client->powerupTime.haste > level.time;

		// trigger_gravity support
		client->ps.pmove.gravity = (short)(level.gravity * ent->gravity);
		pm.s = client->ps.pmove;

		pm.s.origin = ent->s.origin;
		pm.s.velocity = ent->velocity;

		if (memcmp(&client->old_pmove, &pm.s, sizeof(pm.s)))
			pm.snapinitial = true;

		pm.cmd = *ucmd;
		pm.player = ent;
		pm.trace = gi.game_import_t::trace;
		pm.clip = G_PM_Clip;
		pm.pointcontents = gi.pointcontents;
		pm.viewoffset = ent->client->ps.viewoffset;

		// perform a pmove
		Pmove(&pm);

		if (pm.groundEntity && ent->groundEntity) {
			float stepsize = fabs(ent->s.origin[2] - pm.s.origin[2]);

			if (stepsize > 4.f && stepsize < (ent->s.origin[2] < 0 ? STEPSIZE_BELOW : STEPSIZE)) {
				ent->s.renderfx |= RF_STAIR_STEP;
				ent->client->step_frame = gi.ServerFrame() + 1;
			}
		}

		P_FallingDamage(ent, pm);

		if (ent->client->landmark_free_fall && pm.groundEntity) {
			ent->client->landmark_free_fall = false;
			ent->client->landmark_noise_time = level.time + 100_ms;
		}

		// [Paril-KEX] save old position for G_TouchProjectiles
		vec3_t old_origin = ent->s.origin;

		ent->s.origin = pm.s.origin;
		ent->velocity = pm.s.velocity;

		// [Paril-KEX] if we stepped onto/off of a ladder, reset the
		// last ladder pos
		if ((pm.s.pm_flags & PMF_ON_LADDER) != (client->ps.pmove.pm_flags & PMF_ON_LADDER)) {
			client->last_ladder_pos = ent->s.origin;

			if (pm.s.pm_flags & PMF_ON_LADDER) {
				if (!deathmatch->integer &&
					client->last_ladder_sound < level.time) {
					ent->s.event = EV_LADDER_STEP;
					client->last_ladder_sound = level.time + LADDER_SOUND_TIME;
				}
			}
		}

		// save results of pmove
		client->ps.pmove = pm.s;
		client->old_pmove = pm.s;

		ent->mins = pm.mins;
		ent->maxs = pm.maxs;

		if (!ent->client->menu)
			client->resp.cmdAngles = ucmd->angles;

		if (pm.jump_sound && !(pm.s.pm_flags & PMF_ON_LADDER)) {
			gi.sound(ent, CHAN_VOICE, gi.soundindex("*jump1.wav"), 1, ATTN_NORM, 0);
			// Paril: removed to make ambushes more effective and to
			// not have monsters around corners come to jumps
			// PlayerNoise(ent, ent->s.origin, PNOISE_SELF);
		}

		// sam raimi cam support
		if (ent->flags & FL_SAM_RAIMI)
			ent->viewHeight = 8;
		else
			ent->viewHeight = (int)pm.s.viewHeight;

		ent->waterlevel = pm.waterlevel;
		ent->watertype = pm.watertype;
		ent->groundEntity = pm.groundEntity;
		if (pm.groundEntity)
			ent->groundEntity_linkCount = pm.groundEntity->linkCount;

		if (ent->deadFlag) {
			client->ps.viewangles[ROLL] = 40;
			client->ps.viewangles[PITCH] = -15;
			client->ps.viewangles[YAW] = client->killer_yaw;
		} else if (!ent->client->menu) {
			client->vAngle = pm.viewangles;
			client->ps.viewangles = pm.viewangles;
			AngleVectors(client->vAngle, client->vForward, nullptr, nullptr);
		}

		if (client->grappleEnt)
			Weapon_Grapple_Pull(client->grappleEnt);

		gi.linkentity(ent);

		ent->gravity = 1.0;

		if (ent->moveType != MOVETYPE_NOCLIP) {
			TouchTriggers(ent);
			if (ent->moveType != MOVETYPE_FREECAM)
				G_TouchProjectiles(ent, old_origin);
		}

		// touch other objects
		for (i = 0; i < pm.touch.num; i++) {
			trace_t &tr = pm.touch.traces[i];
			other = tr.ent;

			if (other->touch)
				other->touch(other, ent, tr, true);
		}
	}

	// fire weapon from final position if needed
	if (client->latchedButtons & BUTTON_ATTACK) {
		if (!ClientIsPlaying(client) || (client->eliminated && !client->sess.is_a_bot)) {
			client->latchedButtons = BUTTON_NONE;

			if (client->followTarget) {
				FreeFollower(ent);
			} else
				GetFollowTarget(ent);
		} else if (!ent->client->weaponThunk) {
			// we can only do this during a ready state and
			// if enough time has passed from last fire
			if (ent->client->weaponState == WEAPON_READY && !CombatIsDisabled()) {
				ent->client->weaponFireBuffered = true;

				if (ent->client->weaponFireFinished <= level.time) {
					ent->client->weaponThunk = true;
					Think_Weapon(ent);
				}
			}
		}
	}

	if (!ClientIsPlaying(client) || (client->eliminated && !client->sess.is_a_bot)) {
		if (!HandleMenuMovement(ent, ucmd)) {
			if (ucmd->buttons & BUTTON_JUMP) {
				if (!(client->ps.pmove.pm_flags & PMF_JUMP_HELD)) {
					client->ps.pmove.pm_flags |= PMF_JUMP_HELD;
					if (client->followTarget)
						FollowNext(ent);
					else
						GetFollowTarget(ent);
				}
			} else
				client->ps.pmove.pm_flags &= ~PMF_JUMP_HELD;
		}
	}

	// update chase cam if being followed
	for (auto ec : active_clients())
		if (ec->client->followTarget == ent)
			UpdateChaseCam(ec);

	// perform once-a-second actions
	ClientTimerActions(ent);
}

// active monsters
struct active_monsters_filter_t {
	inline bool operator()(gentity_t *ent) const {
		return (ent->inUse && (ent->svFlags & SVF_MONSTER) && ent->health > 0);
	}
};

static inline entity_iterable_t<active_monsters_filter_t> active_monsters() {
	return entity_iterable_t<active_monsters_filter_t> { game.maxclients + (uint32_t)BODY_QUEUE_SIZE + 1U };
}

static inline bool G_MonstersSearchingFor(gentity_t *player) {
	for (auto ent : active_monsters()) {
		// check for *any* player target
		if (player == nullptr && ent->enemy && !ent->enemy->client)
			continue;
		// they're not targeting us, so who cares
		else if (player != nullptr && ent->enemy != player)
			continue;

		// they lost sight of us
		if ((ent->monsterInfo.aiFlags & AI_LOST_SIGHT) && level.time > ent->monsterInfo.trail_time + 5_sec)
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
static inline bool G_FindRespawnSpot(gentity_t *player, vec3_t &spot) {
	constexpr float yawOffsets[] = { 0, 90, 45, -45, -90 };
	constexpr float backDistance = 128.0f;
	constexpr float upDistance = 128.0f;
	constexpr float viewHeight = static_cast<float>(DEFAULT_VIEWHEIGHT);
	constexpr contents_t solidMask = MASK_PLAYERSOLID | CONTENTS_LAVA | CONTENTS_SLIME;

	// Sanity check: make sure player isn't already stuck
	if (gi.trace(player->s.origin, PLAYER_MINS, PLAYER_MAXS, player->s.origin, player, MASK_PLAYERSOLID).startsolid)
		return false;

	for (float yawOffset : yawOffsets) {
		vec3_t yawAngles = { 0, player->s.angles[YAW] + 180.0f + yawOffset, 0 };

		// Step 1: Try moving up first
		vec3_t start = player->s.origin;
		vec3_t end = start + vec3_t{ 0, 0, upDistance };
		trace_t tr = gi.trace(start, PLAYER_MINS, PLAYER_MAXS, end, player, solidMask);
		if (tr.startsolid || tr.allsolid || (tr.contents & (CONTENTS_LAVA | CONTENTS_SLIME)))
			continue;

		// Step 2: Then move backwards from that elevated point
		vec3_t forward;
		AngleVectors(yawAngles, forward, nullptr, nullptr);
		start = tr.endpos;
		end = start + forward * backDistance;
		tr = gi.trace(start, PLAYER_MINS, PLAYER_MAXS, end, player, solidMask);
		if (tr.startsolid || tr.allsolid || (tr.contents & (CONTENTS_LAVA | CONTENTS_SLIME)))
			continue;

		// Step 3: Now cast downward to find solid ground
		start = tr.endpos;
		end = start - vec3_t{ 0, 0, upDistance * 4 };
		tr = gi.trace(start, PLAYER_MINS, PLAYER_MAXS, end, player, solidMask);
		if (tr.startsolid || tr.allsolid || tr.fraction == 1.0f || tr.ent != world || tr.plane.normal.z < 0.7f)
			continue;

		// Avoid liquids
		if (gi.pointcontents(tr.endpos + vec3_t{ 0, 0, viewHeight }) & MASK_WATER)
			continue;

		// Height delta check
		float zDelta = fabsf(player->s.origin[2] - tr.endpos[2]);
		float stepLimit = (player->s.origin[2] < 0 ? STEPSIZE_BELOW : STEPSIZE);
		if (zDelta > stepLimit * 4.0f)
			continue;

		// If stepped up/down, ensure visibility
		if (zDelta > stepLimit) {
			if (gi.traceline(player->s.origin, tr.endpos, player, solidMask).fraction != 1.0f)
				continue;
			if (gi.traceline(player->s.origin + vec3_t{ 0, 0, viewHeight }, tr.endpos + vec3_t{ 0, 0, viewHeight }, player, solidMask).fraction != 1.0f)
				continue;
		}

		spot = tr.endpos;
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
static inline std::tuple<gentity_t *, vec3_t> G_FindSquadRespawnTarget() {
	const bool anyMonstersSearching = G_MonstersSearchingFor(nullptr);

	for (gentity_t *player : active_clients()) {
		auto *cl = player->client;

		// Skip invalid candidates
		if (player->deadFlag)
			continue;
		if (cl->last_damage_time >= level.time) {
			cl->coop_respawn_state = COOP_RESPAWN_IN_COMBAT;
			continue;
		}
		if (G_MonstersSearchingFor(player)) {
			cl->coop_respawn_state = COOP_RESPAWN_IN_COMBAT;
			continue;
		}
		if (anyMonstersSearching && cl->last_firing_time >= level.time) {
			cl->coop_respawn_state = COOP_RESPAWN_IN_COMBAT;
			continue;
		}
		if (player->groundEntity != world) {
			cl->coop_respawn_state = COOP_RESPAWN_BAD_AREA;
			continue;
		}
		if (player->waterlevel >= WATER_UNDER) {
			cl->coop_respawn_state = COOP_RESPAWN_BAD_AREA;
			continue;
		}

		vec3_t spot;
		if (!G_FindRespawnSpot(player, spot)) {
			cl->coop_respawn_state = COOP_RESPAWN_BLOCKED;
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
static bool G_CoopRespawn(gentity_t *ent) {
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
			ent->client->coop_respawn_state = COOP_RESPAWN_NO_LIVES;
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

					squad_respawn_position = good_spot;
					squad_respawn_angles = good_player->s.angles;
					squad_respawn_angles[2] = 0;

					use_squad_respawn = true;
				} else {
					state = RESPAWN_SPECTATE;
				}
			}
		} else
			state = RESPAWN_START;
	}

	if (state == RESPAWN_SQUAD || state == RESPAWN_START) {
		// give us our max health back since it will reset
		// to pers.health; in instanced items we'd lose the items
		// we touched so we always want to respawn with our max.
		if (P_UseCoopInstancedItems())
			ent->client->pers.health = ent->client->pers.max_health = ent->max_health;

		ClientRespawn(ent);

		ent->client->latchedButtons = BUTTON_NONE;
		use_squad_respawn = false;
	} else if (state == RESPAWN_SPECTATE) {
		if (!ent->client->coop_respawn_state)
			ent->client->coop_respawn_state = COOP_RESPAWN_WAITING;

		if (ClientIsPlaying(ent->client)) {
			// move us to spectate just so we don't have to twiddle
			// our thumbs forever
			CopyToBodyQue(ent);
			ent->client->sess.team = TEAM_SPECTATOR;
			MoveClientToFreeCam(ent);
			gi.linkentity(ent);
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
inline void ActivateSelectedMenuItem(gentity_t *ent);
void ClientBeginServerFrame(gentity_t *ent) {
	gclient_t *client;
	int		   buttonMask;

	if (gi.ServerFrame() != ent->client->step_frame)
		ent->s.renderfx &= ~RF_STAIR_STEP;

	if (level.intermissionTime)
		return;

	client = ent->client;

	if (client->awaiting_respawn) {
		if ((level.time.milliseconds() % 500) == 0)
			ClientSpawn(ent);
		return;
	}

	if ((ent->svFlags & SVF_BOT) != 0) {
		Bot_BeginFrame(ent);
	}

	// run weapon animations if it hasn't been done by a ucmd_t
	if (!client->weaponThunk && ClientIsPlaying(client) && !client->eliminated)
		Think_Weapon(ent);
	else
		client->weaponThunk = false;

	if (ent->client->menu) {
		if ((client->latchedButtons & BUTTON_ATTACK)) {
			ActivateSelectedMenuItem(ent);
			client->latchedButtons = BUTTON_NONE;
		}
		return;
	} else if (ent->deadFlag) {
		//wor: add minimum delay in dm
		if (deathmatch->integer && client->respawnMinTime && level.time > client->respawnMinTime && level.time <= client->respawnMaxTime && !level.intermissionQueued) {
			if ((client->latchedButtons & BUTTON_ATTACK)) {
				ClientRespawn(ent);
				client->latchedButtons = BUTTON_NONE;
			}
		} else if (level.time > client->respawnMaxTime && !level.coopLevelRestartTime) {
			// don't respawn if level is waiting to restart
			// check for coop handling
			if (!G_CoopRespawn(ent)) {
				// in deathmatch, only wait for attack button
				if (deathmatch->integer)
					buttonMask = BUTTON_ATTACK;
				else
					buttonMask = -1;

				if ((client->latchedButtons & buttonMask) ||
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
void RemoveAttackingPainDaemons(gentity_t *self) {
	gentity_t *tracker;

	tracker = G_FindByString<&gentity_t::className>(nullptr, "pain daemon");
	while (tracker) {
		if (tracker->enemy == self)
			FreeEntity(tracker);
		tracker = G_FindByString<&gentity_t::className>(tracker, "pain daemon");
	}

	if (self->client)
		self->client->trackerPainTime = 0_ms;
}
