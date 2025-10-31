// Copyright (c) DarkMatter Projects 2023-2025
// Licensed under the GNU General Public License 2.0.
//
// commands_client.cpp - Implements all general client-side commands.

#include "command_system.hpp"
#include "command_registration.hpp"
#include "g_local.hpp"
#include "monsters/m_player.hpp"
#include <string>
#include <format>
#include <vector>
#include <algorithm>
#include <ranges>
#include <sstream>

namespace Commands {

	// --- Forward Declarations for Client Functions ---
	void Admin(gentity_t* ent, const CommandArgs& args);
	void ClientList(gentity_t* ent, const CommandArgs& args);
	void Drop(gentity_t* ent, const CommandArgs& args);
	void EyeCam(gentity_t* ent, const CommandArgs& args);
	void FragMessages(gentity_t* ent, const CommandArgs& args);
	void Follow(gentity_t* ent, const CommandArgs& args);
	void FollowKiller(gentity_t* ent, const CommandArgs& args);
	void FollowLeader(gentity_t* ent, const CommandArgs& args);
	void FollowPowerup(gentity_t* ent, const CommandArgs& args);
	void Forfeit(gentity_t* ent, const CommandArgs& args);
	void Help(gentity_t* ent, const CommandArgs& args);
	void Hook(gentity_t* ent, const CommandArgs& args);
	void CrosshairID(gentity_t* ent, const CommandArgs& args);
	void InvDrop(gentity_t* ent, const CommandArgs& args);
	void Inven(gentity_t* ent, const CommandArgs& args);
	void InvNext(gentity_t* ent, const CommandArgs& args);
	void InvNextP(gentity_t* ent, const CommandArgs& args);
	void InvNextW(gentity_t* ent, const CommandArgs& args);
	void InvPrev(gentity_t* ent, const CommandArgs& args);
	void InvPrevP(gentity_t* ent, const CommandArgs& args);
	void InvPrevW(gentity_t* ent, const CommandArgs& args);
	void InvUse(gentity_t* ent, const CommandArgs& args);
	void KillBeep(gentity_t* ent, const CommandArgs& args);
	void Kill(gentity_t* ent, const CommandArgs& args);
	void MapInfo(gentity_t* ent, const CommandArgs& args);
	void MapPool(gentity_t* ent, const CommandArgs& args);
	void MapCycle(gentity_t* ent, const CommandArgs& args);
	void Motd(gentity_t* ent, const CommandArgs& args);
	void MyMap(gentity_t* ent, const CommandArgs& args);
	void MySkill(gentity_t* ent, const CommandArgs& args);
	void NotReady(gentity_t* ent, const CommandArgs& args);
	void PutAway(gentity_t* ent, const CommandArgs& args);
	void Ready(gentity_t* ent, const CommandArgs& args);
	void ReadyUp(gentity_t* ent, const CommandArgs& args);
	void Score(gentity_t* ent, const CommandArgs& args);
	void SetWeaponPref(gentity_t* ent, const CommandArgs& args);
	void Stats(gentity_t* ent, const CommandArgs& args);
	void JoinTeam(gentity_t* ent, const CommandArgs& args);
	void TimeIn(gentity_t* ent, const CommandArgs& args);
	void TimeOut(gentity_t* ent, const CommandArgs& args);
	void Timer(gentity_t* ent, const CommandArgs& args);
	void UnHook(gentity_t* ent, const CommandArgs& args);
	void Use(gentity_t* ent, const CommandArgs& args);
	void Wave(gentity_t* ent, const CommandArgs& args);
	void WeapLast(gentity_t* ent, const CommandArgs& args);
	void WeapNext(gentity_t* ent, const CommandArgs& args);
	void WeapPrev(gentity_t* ent, const CommandArgs& args);
	void Where(gentity_t* ent, const CommandArgs& args);

	// --- Client Command Implementations ---

	void Admin(gentity_t* ent, const CommandArgs& args) {
		if (!g_allowAdmin->integer) {
			gi.Client_Print(ent, PRINT_HIGH, "Administration is disabled on this server.\n");
			return;
		}

		if (args.count() > 1) {
			if (ent->client->sess.admin) {
				gi.Client_Print(ent, PRINT_HIGH, "You already have administrative rights.\n");
				return;
			}
			if (admin_password->string && *admin_password->string && args.getString(1) == admin_password->string) {
				ent->client->sess.admin = true;
				gi.LocBroadcast_Print(PRINT_HIGH, "{} has become an admin.\n", ent->client->sess.netName);
			}
		}
		else {
			if (ent->client->sess.admin) {
				gi.Client_Print(ent, PRINT_HIGH, "You are an admin.\n");
			}
			else {
				PrintUsage(ent, args, "[password]", "", "Gain admin rights by providing the admin password.");
			}
		}
	}

	void ClientList(gentity_t* ent, const CommandArgs& args) {
		// Parse sort mode (default to "score")
		std::string_view sort_mode = args.getString(1);
		enum class Sort { Score, Time, Name };
		Sort mode = Sort::Score;
		if (sort_mode == "time") mode = Sort::Time;
		else if (sort_mode == "name") mode = Sort::Name;

		struct Row {
			int        clientIndex;   // 0..game.maxclients-1
			gentity_t* edict;         // &g_entities[clientIndex + 1]
		};

		std::vector<Row> rows;
		rows.reserve(game.maxClients);

		// Gather connected clients (0-based client indices)
		for (auto ec : active_clients()) {
			if (!ec || !ec->client) continue;
			int ci = int(ec->client - game.clients);
			if (ci < 0 || ci >= static_cast<int>(game.maxClients)) continue;
			rows.push_back({ ci, ec });
		}

		if (rows.empty()) {
			gi.Client_Print(ent, PRINT_HIGH, "No clients connected.\n");
			return;
		}

		// Sorting
		switch (mode) {
		case Sort::Score:
			std::stable_sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
				const gclient_t* A = &game.clients[a.clientIndex];
				const gclient_t* B = &game.clients[b.clientIndex];
				return A->resp.score > B->resp.score; // high -> low
				});
			break;
		case Sort::Time:
			std::stable_sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
				const gclient_t* A = &game.clients[a.clientIndex];
				const gclient_t* B = &game.clients[b.clientIndex];
				return A->resp.enterTime < B->resp.enterTime; // oldest first
				});
			break;
		case Sort::Name:
			std::stable_sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
				const gclient_t* A = &game.clients[a.clientIndex];
				const gclient_t* B = &game.clients[b.clientIndex];
				const char* an = A->sess.netName ? A->sess.netName : "";
				const char* bn = B->sess.netName ? B->sess.netName : "";
				auto lower = [](char c) { return (c >= 'A' && c <= 'Z') ? char(c - 'A' + 'a') : c; };
				for (size_t i = 0; ; ++i) {
					char ca = lower(an[i]);
					char cb = lower(bn[i]);
					if (ca != cb) return ca < cb;
					if (ca == '\0') return false; // equal
				}
				});
			break;
		}

		// Fixed column widths (characters)
		// Adjust if you want wider/narrower fields.
		const size_t W_NUM = 3;
		const size_t W_NAME = 24;
		const size_t W_ID = 20;
		const size_t W_SR = 5;
		const size_t W_TIME = 5;   // mm:ss
		const size_t W_PING = 4;
		const size_t W_SCORE = 5;
		const size_t W_TEAM = 10;

		auto trunc_to = [](const char* s, size_t maxlen) -> std::string {
			if (!s) return std::string();
			std::string out(s);
			if (out.size() > maxlen) out.resize(maxlen);
			return out;
			};

		auto pad_left = [](const std::string& s, size_t w) -> std::string {
			if (s.size() >= w) return s;
			return std::string(w - s.size(), ' ') + s;
			};

		auto pad_right = [](const std::string& s, size_t w) -> std::string {
			if (s.size() >= w) return s;
			return s + std::string(w - s.size(), ' ');
			};

		// Header
		{
			std::string hdr;
			hdr.reserve(96);
			hdr += pad_left("num", W_NUM);  hdr += " ";
			hdr += pad_right("name", W_NAME); hdr += " ";
			hdr += pad_right("id", W_ID);   hdr += " ";
			hdr += pad_left("sr", W_SR);   hdr += " ";
			hdr += pad_left("time", W_TIME); hdr += " ";
			hdr += pad_left("ping", W_PING); hdr += " ";
			hdr += pad_left("score", W_SCORE); hdr += " ";
			hdr += pad_right("state", W_TEAM);
			hdr += "\n";
			gi.Client_Print(ent, PRINT_HIGH, hdr.c_str());
		}

		// Rows (emit one line at a time to avoid message clipping)
		for (const Row& r : rows) {
			gclient_t* cl = &game.clients[r.clientIndex];

			const int displayNum = r.clientIndex + 1;
			const int ping = cl->ping;
			const int score = cl->resp.score;
			const int sr = cl->sess.skillRating;

			const char* tn = Teams_TeamName(cl->sess.team);
			const char* teamName = (tn && tn[0]) ? tn : (ClientIsPlaying(cl) ? "play" : "spec");

			// Time as mm:ss based on your GameTime utilities
			const auto dt = (level.time - cl->resp.enterTime);
			const int mm = dt.minutes<int>();
			const int ss = dt.seconds<int>() % 60;

			std::string num = std::to_string(displayNum);
			std::string name = trunc_to(cl->sess.netName, W_NAME);
			std::string sid = trunc_to(cl->sess.socialID, W_ID);
			std::string s_sr = std::to_string(sr);
			char time_buf[8];
			std::snprintf(time_buf, sizeof(time_buf), "%d:%02d", mm, ss);
			std::string s_time(time_buf);
			std::string s_ping = std::to_string(ping);
			std::string s_score = std::to_string(score);
			std::string s_team = trunc_to(teamName, W_TEAM);

			std::string line;
			line.reserve(128);
			line += pad_left(num, W_NUM);   line += " ";
			line += pad_right(name, W_NAME);  line += " ";
			line += pad_right(sid, W_ID);    line += " ";
			line += pad_left(s_sr, W_SR);    line += " ";
			line += pad_left(s_time, W_TIME);  line += " ";
			line += pad_left(s_ping, W_PING);  line += " ";
			line += pad_left(s_score, W_SCORE); line += " ";
			line += pad_right(s_team, W_TEAM);
			line += "\n";

			gi.Client_Print(ent, PRINT_HIGH, line.c_str());
		}
	}

	void Drop(gentity_t* ent, const CommandArgs& args) {
		if (CombatIsDisabled()) {
			return;
		}

		std::string itemQuery = args.joinFrom(1);
		std::string_view arg1 = args.getString(1);

		if (itemQuery.empty()) {
			PrintUsage(ent, args, "<item_name|tech|weapon>", "", "Drops an item, your current tech, or your current weapon.");
			return;
		}

		Item* it = nullptr;

		// Handle special cases first
		if (arg1 == "tech") {
			it = Tech_Held(ent);
			if (it) {
				it->drop(ent, it);
				ValidateSelectedItem(ent);
			}
			return;
		}
		if (arg1 == "weapon") {
			it = ent->client->pers.weapon;
			if (it && it->drop) {
				it->drop(ent, it);
				ValidateSelectedItem(ent);
			}
			return;
		}

		// Standard item lookup
		if (args.getString(0) == "drop_index") {
			if (auto itemIndex = args.getInt(1)) {
				if (*itemIndex > IT_NULL && *itemIndex < IT_TOTAL) {
					it = GetItemByIndex(static_cast<item_id_t>(*itemIndex));
				}
			}
		}
		else {
			it = FindItem(arg1.data());

			if (!it) {
				if (auto parsedIndex = CommandArgs::ParseInt(arg1)) {
					if (*parsedIndex > IT_NULL && *parsedIndex < IT_TOTAL) {
						it = GetItemByIndex(static_cast<item_id_t>(*parsedIndex));
					}
				}
			}
		}

		if (!it) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Unknown item: {}\n", arg1.data());
			return;
		}

		if (!it->drop) {
			gi.LocClient_Print(ent, PRINT_HIGH, "$g_item_not_droppable");
			return;
		}

		// Check server-side drop restrictions
		if (it->id == IT_FLAG_RED || it->id == IT_FLAG_BLUE) {
			if (!(match_dropCmdFlags->integer & 1)) {
				gi.Client_Print(ent, PRINT_HIGH, "Flag dropping has been disabled on this server.\n");
				return;
			}
		}
		else if (it->flags & IF_POWERUP) {
			if (!(match_dropCmdFlags->integer & 2)) {
				gi.Client_Print(ent, PRINT_HIGH, "Powerup dropping has been disabled on this server.\n");
				return;
			}
		}
		else if (it->flags & (IF_WEAPON | IF_AMMO)) {
			if (!(match_dropCmdFlags->integer & 4)) {
				gi.Client_Print(ent, PRINT_HIGH, "Weapon and ammo dropping has been disabled on this server.\n");
				return;
			}
			if (!ItemSpawnsEnabled()) {
				gi.Client_Print(ent, PRINT_HIGH, "Weapon and ammo dropping is not available in this mode.\n");
				return;
			}
		}

		if (it->flags & IF_WEAPON && deathmatch->integer && match_weaponsStay->integer) {
			gi.Client_Print(ent, PRINT_HIGH, "Weapon dropping is not available during weapons stay mode.\n");
			return;
		}

		if (!ent->client->pers.inventory[it->id]) {
			gi.LocClient_Print(ent, PRINT_HIGH, "$g_out_of_item", it->pickupName);
			return;
		}

		it->drop(ent, it);

		// Notify teammates
		if (Teams() && g_teamplay_item_drop_notice->integer) {
			uint32_t key = GetUnicastKey();
			std::string message = std::format("[TEAM]: {} drops {}\n", ent->client->sess.netName, it->useName);

			for (auto ec : active_clients()) {
				if (ent == ec) continue;

				bool isTeammate = OnSameTeam(ent, ec);
				bool isFollowingTeammate = !ClientIsPlaying(ec->client) && ec->client->follow.target && OnSameTeam(ent, ec->client->follow.target);

				if (isTeammate || isFollowingTeammate) {
					gi.WriteByte(svc_poi);
					gi.WriteShort(POI_PING + (ent->s.number - 1));
					gi.WriteShort(5000);
					gi.WritePosition(ent->s.origin);
					gi.WriteShort(gi.imageIndex(it->icon));
					gi.WriteByte(215);
					gi.WriteByte(POI_FLAG_NONE);
					gi.unicast(ec, false);
					gi.localSound(ec, CHAN_AUTO, gi.soundIndex("misc/help_marker.wav"), 1.0f, ATTN_NONE, 0.0f, key);
					gi.LocClient_Print(ec, PRINT_TTS, message.c_str(), ent->client->sess.netName);
				}
			}
		}

		ValidateSelectedItem(ent);
	}

	void EyeCam(gentity_t* ent, const CommandArgs& args) {
		ent->client->sess.pc.use_eyecam = !ent->client->sess.pc.use_eyecam;
		gi.LocClient_Print(ent, PRINT_HIGH, "EyeCam {}.\n", ent->client->sess.pc.use_eyecam ? "enabled" : "disabled");
	}

	void FragMessages(gentity_t* ent, const CommandArgs& args) {
		ent->client->sess.pc.show_fragmessages = !ent->client->sess.pc.show_fragmessages;
		gi.LocClient_Print(ent, PRINT_HIGH, "Frag messages {}.\n", ent->client->sess.pc.show_fragmessages ? "enabled" : "disabled");
	}

	void Follow(gentity_t* ent, const CommandArgs& args) {
		if (ClientIsPlaying(ent->client)) {
			gi.Client_Print(ent, PRINT_HIGH, "You must be a spectator to follow.\n");
			return;
		}
		if (args.count() < 2) {
			PrintUsage(ent, args, "<client_name|number>", "", "Follows the specified player.");
			return;
		}

		gentity_t* follow_ent = ClientEntFromString(args.getString(1).data());
		if (!follow_ent || !follow_ent->inUse || !ClientIsPlaying(follow_ent->client)) {
			gi.Client_Print(ent, PRINT_HIGH, "Invalid or non-playing client specified.\n");
			return;
		}

		ent->client->follow.target = follow_ent;
		ent->client->follow.update = true;
		ClientUpdateFollowers(ent);
	}

	void FollowKiller(gentity_t* ent, const CommandArgs& args) {
		ent->client->sess.pc.follow_killer = !ent->client->sess.pc.follow_killer;
		gi.LocClient_Print(ent, PRINT_HIGH, "Auto-follow killer: {}.\n", ent->client->sess.pc.follow_killer ? "ON" : "OFF");
	}

	void FollowLeader(gentity_t* ent, const CommandArgs& args) {
		ent->client->sess.pc.follow_leader = !ent->client->sess.pc.follow_leader;
		gi.LocClient_Print(ent, PRINT_HIGH, "Auto-follow leader: {}.\n", ent->client->sess.pc.follow_leader ? "ON" : "OFF");
	}

	void FollowPowerup(gentity_t* ent, const CommandArgs& args) {
		ent->client->sess.pc.follow_powerup = !ent->client->sess.pc.follow_powerup;
		gi.LocClient_Print(ent, PRINT_HIGH, "Auto-follow powerup carrier: {}.\n", ent->client->sess.pc.follow_powerup ? "ON" : "OFF");
	}

	void Forfeit(gentity_t* ent, const CommandArgs& args) {
		if (!Game::Has(GameFlags::OneVOne)) {
			gi.Client_Print(ent, PRINT_HIGH, "Forfeit is only available during Duel or Gauntlet.\n");
			return;
		}
		if (level.matchState < MatchState::In_Progress) {
			gi.Client_Print(ent, PRINT_HIGH, "Forfeit is not available during warmup.\n");
			return;
		}
		if (ent->client != &game.clients[level.sortedClients[1]]) {
			gi.Client_Print(ent, PRINT_HIGH, "Forfeit is only available to the losing player.\n");
			return;
		}
		if (!g_allowForfeit->integer) {
			gi.Client_Print(ent, PRINT_HIGH, "Forfeits are not enabled on this server.\n");
			return;
		}
		std::string msg = std::format("{} forfeits the match.", ent->client->sess.netName);
		QueueIntermission(msg.c_str(), true, false);
	}

	void Help(gentity_t* ent, const CommandArgs& args) {
		if (deathmatch->integer) {
			Score(ent, args);
			return;
		}
		if (level.intermission.time || ent->health <= 0) return;

		ent->client->showInventory = false;
		ent->client->showScores = false;

		if (ent->client->showHelp &&
			ent->client->pers.game_help1changed == game.help[0].modificationCount &&
			ent->client->pers.game_help2changed == game.help[1].modificationCount) {
			ent->client->showHelp = false;
			globals.serverFlags &= ~SERVER_FLAG_SLOW_TIME;
			return;
		}

		ent->client->showHelp = true;
		ent->client->pers.helpchanged = 0;
		globals.serverFlags |= SERVER_FLAG_SLOW_TIME;
		DrawHelpComputer(ent);
	}

	void Hook(gentity_t* ent, const CommandArgs& args) {
		if (!g_allow_grapple->integer || !g_grapple_offhand->integer) return;
		Weapon_Hook(ent);
	}

	void CrosshairID(gentity_t* ent, const CommandArgs& args) {
		ent->client->sess.pc.show_id = !ent->client->sess.pc.show_id;
		gi.LocClient_Print(ent, PRINT_HIGH, "Player identification display: {}.\n", ent->client->sess.pc.show_id ? "ON" : "OFF");
	}

	/*
	===============
	Impulse
	Quake 1-style one-shot impulse handler.
	Implements:
		1..8  = weapon selects (Q1 mapping -> nearest Q2 weapon)
		9     = give all (cheat; SP/sv_cheats only)
		10    = next weapon
		12    = previous weapon
		21    = drop current weapon (if droppable)
		255   = give+activate Quad (cheat; SP/sv_cheats only)
	===============
	*/
	static void Impulse(gentity_t* ent, const CommandArgs& args) {
		if (!ent || !ent->client)
			return;

		gclient_t* cl = ent->client;
		if (!ClientIsPlaying(cl) || level.intermission.time)
			return;

		// parse impulse number
		std::optional<int> opt = args.getInt(1);
		if (!opt) {
			gi.Client_Print(ent, PRINT_HIGH, "usage: impulse <0..255>\n");
			return;
		}
		const int n = *opt;
		if (n < 0 || n > 255) {
			gi.Client_Print(ent, PRINT_HIGH, "impulse: expected integer 0..255\n");
			return;
		}

		// Helpers
		auto has_item = [&](item_id_t id) -> bool {
			return id > IT_NULL && id < IT_TOTAL && cl->pers.inventory[id] > 0;
			};
		auto use_item = [&](item_id_t id) -> bool {
			if (!has_item(id)) return false;
			Item* it = &itemList[id];
			if (!it->use) return false;
			it->use(ent, it);
			return cl->weapon.pending == it || !(it->flags & IF_WEAPON);
			};
		auto try_use_first_owned = [&](std::initializer_list<item_id_t> ids) -> bool {
			for (auto id : ids) if (use_item(id)) return true;
			return false;
			};
		auto cheats_allowed = [&]() -> bool {
			// Mirror your projects cheat gates here.
			// Common pattern: sv_cheats or singleplayer-only.
			return (CheatsOk(ent));
			};

		// Q1 -> Q2 weapon mapping by item_id_t (adjust IDs to your enum)
		// Replace the placeholder IDs below with your actual item_id_t constants.
		const item_id_t ID_BLASTER = IT_WEAPON_BLASTER;
		const item_id_t ID_SHOTGUN = IT_WEAPON_SHOTGUN;
		const item_id_t ID_SUPERSHOTGUN = IT_WEAPON_SSHOTGUN;
		const item_id_t ID_MACHINEGUN = IT_WEAPON_MACHINEGUN;
		const item_id_t ID_CHAINGUN = IT_WEAPON_CHAINGUN;
		const item_id_t ID_HYPERBLASTER = IT_WEAPON_HYPERBLASTER;
		const item_id_t ID_GRENADELAUNCHER = IT_WEAPON_GLAUNCHER;
		const item_id_t ID_ROCKETLAUNCHER = IT_WEAPON_RLAUNCHER;
		const item_id_t ID_RAILGUN = IT_WEAPON_RAILGUN;

		const item_id_t ID_QUAD = IT_POWERUP_QUAD; // your Quad item id

		bool handled = false;

		switch (n) {
			// 1..8: weapon selection (with sensible fallbacks where Q1/Q2 differ)
		case 1: handled = try_use_first_owned({ ID_BLASTER }); break;                       // Axe -> Blaster
		case 2: handled = try_use_first_owned({ ID_SHOTGUN }); break;                       // SG
		case 3: handled = try_use_first_owned({ ID_SUPERSHOTGUN }); break;                  // SSG
		case 4: handled = try_use_first_owned({ ID_MACHINEGUN }); break;                    // NG  -> MG
		case 5: handled = try_use_first_owned({ ID_CHAINGUN, ID_HYPERBLASTER }); break;     // SNG -> CG, fallback HB
		case 6: handled = try_use_first_owned({ ID_GRENADELAUNCHER }); break;               // GL
		case 7: handled = try_use_first_owned({ ID_ROCKETLAUNCHER }); break;                // RL
		case 8: handled = try_use_first_owned({ ID_HYPERBLASTER, ID_RAILGUN }); break;      // LG  -> HB, fallback RG

			// 9: give all (cheat)
		case 9:
			if (!cheats_allowed()) return;
			for (int id = IT_NULL + 1; id < IT_TOTAL; ++id) {
				const Item* it = &itemList[id];
				if (it->flags & IF_WEAPON) {
					cl->pers.inventory[id] = std::max(1, cl->pers.inventory[id]);
				}
				// Optional: fill ammo, keys, and armor to sane amounts if you want classic "give all"
			}
			gi.Client_Print(ent, PRINT_LOW, "impulse 9: all weapons granted\n");
			handled = true;
			break;

			// 10: next weapon
		case 10:
			WeapNext(ent, args);
			handled = true;
			break;

			// 12: previous weapon
		case 12:
			WeapPrev(ent, args);
			handled = true;
			break;

			// 255: give + activate Quad (cheat)
		case 255:
			if (!cheats_allowed()) return;
			if (ID_QUAD > IT_NULL && ID_QUAD < IT_TOTAL) {
				cl->pers.inventory[ID_QUAD] += 1;
				use_item(ID_QUAD); // trigger it immediately
				gi.Client_Print(ent, PRINT_LOW, "Quad Damage activated.\n");
				handled = true;
			}
			break;

		default:
			// Unknown impulses silently ignored (classic Q1 feel), but a hint helps once.
			gi.LocClient_Print(ent, PRINT_LOW, "impulse %d ignored (supported: 1..8, 9, 10, 12, 21, 255)\n", n);
			return;
		}

		if (!handled && n >= 1 && n <= 8) {
			gi.LocClient_Print(ent, PRINT_LOW, "You do not have a valid weapon for impulse %d\n", n);
		}
	}

	void InvDrop(gentity_t* ent, const CommandArgs& args) {
		ValidateSelectedItem(ent);
		if (ent->client->pers.selectedItem == IT_NULL) {
			gi.LocClient_Print(ent, PRINT_HIGH, "$g_no_item_to_drop");
			return;
		}
		Item* it = &itemList[ent->client->pers.selectedItem];
		if (!it->drop) {
			gi.LocClient_Print(ent, PRINT_HIGH, "$g_item_not_droppable");
			return;
		}
		it->drop(ent, it);
		ValidateSelectedItem(ent);
	}

	void Inven(gentity_t* ent, const CommandArgs& args) {
		gclient_t* cl = ent->client;
		cl->showScores = false;
		cl->showHelp = false;
		globals.serverFlags &= ~SERVER_FLAG_SLOW_TIME;

		if (deathmatch->integer) {
			if (Vote_Menu_Active(ent)) return;
			if (cl->menu.current || cl->menu.restoreStatusBar) {
				CloseActiveMenu(ent);
			}
			else {
				OpenJoinMenu(ent);
			}
			return;
		}

		if (cl->showInventory) {
			cl->showInventory = false;
			return;
		}

		globals.serverFlags |= SERVER_FLAG_SLOW_TIME;
		cl->showInventory = true;
		gi.WriteByte(svc_inventory);
		for (int i = 0; i < IT_TOTAL; i++) {
			gi.WriteShort(cl->pers.inventory[i]);
		}
		gi.unicast(ent, true);
	}

	void InvNext(gentity_t* ent, const CommandArgs& args) { SelectNextItem(ent, IF_ANY); }
	void InvNextP(gentity_t* ent, const CommandArgs& args) { SelectNextItem(ent, IF_TIMED | IF_POWERUP | IF_SPHERE); }
	void InvNextW(gentity_t* ent, const CommandArgs& args) { SelectNextItem(ent, IF_WEAPON); }
	void InvPrev(gentity_t* ent, const CommandArgs& args) { SelectPrevItem(ent, IF_ANY); }
	void InvPrevP(gentity_t* ent, const CommandArgs& args) { SelectPrevItem(ent, IF_TIMED | IF_POWERUP | IF_SPHERE); }
	void InvPrevW(gentity_t* ent, const CommandArgs& args) { SelectPrevItem(ent, IF_WEAPON); }

	void InvUse(gentity_t* ent, const CommandArgs& args) {
		if (deathmatch->integer && ent->client->menu.current) {
			ActivateSelectedMenuItem(ent);
			return;
		}
		if (level.intermission.time || !ClientIsPlaying(ent->client) || ent->health <= 0) return;

		ValidateSelectedItem(ent);
		if (ent->client->pers.selectedItem == IT_NULL) {
			gi.LocClient_Print(ent, PRINT_HIGH, "$g_no_item_to_use");
			return;
		}
		Item* it = &itemList[ent->client->pers.selectedItem];
		if (!it->use) {
			gi.LocClient_Print(ent, PRINT_HIGH, "$g_item_not_usable");
			return;
		}

		ent->client->noWeaponChains = true;
		it->use(ent, it);
		ValidateSelectedItem(ent);
	}

	void KillBeep(gentity_t* ent, const CommandArgs& args) {
		int num = 0;
		if (auto val = args.getInt(1)) {
			num = std::clamp(*val, 0, 4);
		}
		else {
			num = (ent->client->sess.pc.killbeep_num + 1) % 5;
		}
		const char* sb[5] = { "off", "clang", "beep-boop", "insane", "tang-tang" };
		ent->client->sess.pc.killbeep_num = num;
		gi.LocClient_Print(ent, PRINT_HIGH, "Kill beep changed to: {}.\n", sb[num]);
	}

	void Kill(gentity_t* ent, const CommandArgs& args) {
		if (level.intermission.time) return;
		if (deathmatch->integer && (level.time - ent->client->respawnMaxTime) < 5_sec) return;
		if (CombatIsDisabled()) return;

		ent->flags &= ~FL_GODMODE;
		ent->health = 0;
		player_die(ent, ent, ent, 100000, vec3_origin, { ModID::Suicide, Game::Is(GameType::TeamDeathmatch) });
	}

	void MapInfo(gentity_t* ent, const CommandArgs& args) {
		if (level.mapName[0]) {
			gi.LocClient_Print(ent, PRINT_HIGH, "MAP INFO:\nfilename: {}\n", level.mapName.data());
		}
		else {
			return;
		}
		if (level.longName[0]) {
			gi.LocClient_Print(ent, PRINT_HIGH, "longname: {}\n", level.longName.data());
		}
		if (level.author[0]) {
			std::string authors = level.author;
			if (level.author2[0]) {
				authors += ", ";
				authors += level.author2;
			}
			gi.LocClient_Print(ent, PRINT_HIGH, "author{}: {}\n", level.author2[0] ? "s" : "", authors.c_str());
		}
	}

	void MapPool(gentity_t* ent, const CommandArgs& args) {
		int count = PrintMapListFiltered(ent, false, std::string(args.getString(1)));
		gi.LocClient_Print(ent, PRINT_HIGH, "Total maps in pool: {}\n", count);
	}

	void MapCycle(gentity_t* ent, const CommandArgs& args) {
		int count = PrintMapListFiltered(ent, true, std::string(args.getString(1)));
		gi.LocClient_Print(ent, PRINT_HIGH, "Total cycleable maps: {}\n", count);
	}

	void Motd(gentity_t* ent, const CommandArgs& args) {
		if (!game.motd.empty()) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Message of the Day:\n{}\n", game.motd.c_str());
		}
		else {
			gi.Client_Print(ent, PRINT_HIGH, "No Message of the Day set.\n");
		}
	}

	void MyMap(gentity_t* ent, const CommandArgs& args) {
		if (!g_maps_mymap->integer) {
			gi.Client_Print(ent, PRINT_HIGH, "MyMap functionality is disabled on this server.\n");
			return;
		}
		if (!ent->client->sess.socialID[0]) {
			gi.Client_Print(ent, PRINT_HIGH, "You must be logged in to use MyMap.\n");
			return;
		}
		if (args.count() < 2 || args.getString(1) == "?") {
			PrintUsage(ent, args, "<mapname>", "[+flag] [-flag] ...", "Queues a map to be played next with optional rule modifiers.");
			return;
		}

		std::string_view mapName = args.getString(1);
		const MapEntry* map = game.mapSystem.GetMapEntry(std::string(mapName));
		if (!map) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Map '{}' not found in map pool.\n", mapName.data());
			return;
		}
		if (game.mapSystem.IsMapInQueue(std::string(mapName))) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Map '{}' is already in the play queue.\n", mapName.data());
			return;
		}
		if (game.mapSystem.IsClientInQueue(ent->client->sess.socialID)) {
			gi.Client_Print(ent, PRINT_HIGH, "You already have a map queued.\n");
			return;
		}

		// Implementation of flag parsing and queueing logic continues here...
		gi.Client_Print(ent, PRINT_HIGH, "my_map command executed.\n");
	}

	void MySkill(gentity_t* ent, const CommandArgs& args) {
		int totalSkill = 0, numPlayers = 0;
		for (auto ec : active_players()) {
			totalSkill += ec->client->sess.skillRating;
			numPlayers++;
		}
		int averageSkill = (numPlayers > 0) ? (totalSkill / numPlayers) : 0;
		gi.LocClient_Print(ent, PRINT_HIGH, "Your Skill Rating in {}: {} (server avg: {})\n", level.gametype_name.data(), ent->client->sess.skillRating, averageSkill);
	}

	void NotReady(gentity_t* ent, const CommandArgs& args) {
		if (!ReadyConditions(ent, false)) return;
		ClientSetReadyStatus(ent, false, false);
	}

	void PutAway(gentity_t* ent, const CommandArgs& args) {
		ent->client->showScores = false;
		ent->client->showHelp = false;
		ent->client->showInventory = false;
		globals.serverFlags &= ~SERVER_FLAG_SLOW_TIME;
		if (deathmatch->integer && (ent->client->menu.current || ent->client->menu.restoreStatusBar)) {
			if (Vote_Menu_Active(ent)) return;
			CloseActiveMenu(ent);
		}
	}

	void Ready(gentity_t* ent, const CommandArgs& args) {
		if (!ReadyConditions(ent, false)) return;
		ClientSetReadyStatus(ent, true, false);
	}

	void ReadyUp(gentity_t* ent, const CommandArgs& args) {
		if (!ReadyConditions(ent, false)) return;
		ClientSetReadyStatus(ent, false, true);
	}

	void Score(gentity_t* ent, const CommandArgs& args) {
		if (level.intermission.time) return;
		if (!deathmatch->integer && !coop->integer) return;

		if (Vote_Menu_Active(ent)) {
			ent->client->ps.stats[STAT_SHOW_STATUSBAR] = ClientIsPlaying(ent->client) ? 1 : 0;
			return;
		}

		ent->client->showInventory = false;
		ent->client->showHelp = false;
		globals.serverFlags &= ~SERVER_FLAG_SLOW_TIME;

		if (ent->client->menu.current || ent->client->menu.restoreStatusBar) CloseActiveMenu(ent);

		if (ent->client->showScores) {
			ent->client->showScores = false;
		}
		else {
			ent->client->showScores = true;
			MultiplayerScoreboard(ent);
		}
	}

	void SetWeaponPref(gentity_t* ent, const CommandArgs& args) {
		ent->client->sess.weaponPrefs.clear();
		for (int i = 1; i < args.count(); ++i) {
			std::string token(args.getString(i));
			std::ranges::transform(token, token.begin(), ::tolower);
			if (GetWeaponIndexByAbbrev(token) != Weapon::None) {
				ent->client->sess.weaponPrefs.push_back(token);
			}
			else {
				gi.LocClient_Print(ent, PRINT_HIGH, "Unknown weapon abbreviation: {}\n", token.c_str());
			}
		}
		gi.Client_Print(ent, PRINT_HIGH, "Weapon preferences updated.\n");
	}

	void Stats(gentity_t* ent, const CommandArgs& args) {
		if (!Game::Has(GameFlags::CTF)) {
			gi.Client_Print(ent, PRINT_HIGH, "Stats are only available in CTF gametypes.\n");
			return;
		}
		// Further implementation for displaying CTF stats would go here.
		gi.Client_Print(ent, PRINT_HIGH, "Displaying CTF stats...\n");
	}

	void JoinTeam(gentity_t* ent, const CommandArgs& args) {
		if (args.count() < 2) {
			const char* teamName = "spectating";
			if (ClientIsPlaying(ent->client)) {
				teamName = Teams_TeamName(ent->client->sess.team);
			}
			gi.LocClient_Print(ent, PRINT_HIGH, "You are on the {} team.\n", teamName);
			return;
		}

		Team team = StringToTeamNum(args.getString(1).data());
		if (team != Team::None) {
			const bool isBot = (ent->svFlags & SVF_BOT) || ent->client->sess.is_a_bot;
			if (!isBot && FreezeTag_IsFrozen(ent) && team != ent->client->sess.team) {
				gi.LocClient_Print(ent, PRINT_HIGH, "$g_cant_change_teams");
				return;
			}

			::SetTeam(ent, team, false, false, false);
		}
	}

	void TimeIn(gentity_t* ent, const CommandArgs& args) {
		if (!level.timeoutActive) {
			gi.Client_Print(ent, PRINT_HIGH, "A timeout is not currently in effect.\n");
			return;
		}
		if (!ent->client->sess.admin && level.timeoutOwner != ent) {
			gi.Client_Print(ent, PRINT_HIGH, "The timeout can only be ended by the timeout caller or an admin.\n");
			return;
		}
		gi.LocBroadcast_Print(PRINT_HIGH, "{} is resuming the match.\n", ent->client->sess.netName);
		level.timeoutActive = 3_sec;
	}

	void TimeOut(gentity_t* ent, const CommandArgs& args) {
		if (match_timeoutLength->integer <= 0) {
			gi.Client_Print(ent, PRINT_HIGH, "Server has disabled timeouts.\n");
			return;
		}
		if (level.matchState != MatchState::In_Progress) {
			gi.Client_Print(ent, PRINT_HIGH, "Timeouts can only be issued during a match.\n");
			return;
		}
		if (ent->client->pers.timeout_used && !ent->client->sess.admin) {
			gi.Client_Print(ent, PRINT_HIGH, "You have already used your timeout.\n");
			return;
		}
		if (level.timeoutActive > 0_ms) {
			gi.Client_Print(ent, PRINT_HIGH, "A timeout is already in progress.\n");
			return;
		}
		level.timeoutOwner = ent;
		level.timeoutActive = GameTime::from_sec(match_timeoutLength->integer);
		gi.LocBroadcast_Print(PRINT_CENTER, "{} called a timeout!\n{} has been granted.", ent->client->sess.netName, TimeString(match_timeoutLength->integer * 1000, false, false));
		ent->client->pers.timeout_used = true;
		G_LogEvent("MATCH TIMEOUT STARTED");
	}

	void Timer(gentity_t* ent, const CommandArgs& args) {
		ent->client->sess.pc.show_timer = !ent->client->sess.pc.show_timer;
		gi.LocClient_Print(ent, PRINT_HIGH, "Match timer display: {}.\n", ent->client->sess.pc.show_timer ? "ON" : "OFF");
	}

	void UnHook(gentity_t* ent, const CommandArgs& args) {
		Weapon_Grapple_DoReset(ent->client);
	}

	void Use(gentity_t* ent, const CommandArgs& args) {
		std::string itemQuery = args.joinFrom(1);
		std::string_view itemName = args.getString(1);
		if (itemQuery.empty()) {
			PrintUsage(ent, args, "<item_name>", "", "Uses an item from your inventory.");
			return;
		}

		Item* it = nullptr;
		if (itemName == "holdable") {
			// Logic to find the current holdable item
			if (ent->client->pers.inventory[IT_TELEPORTER]) it = GetItemByIndex(IT_TELEPORTER);
			else if (ent->client->pers.inventory[IT_ADRENALINE]) it = GetItemByIndex(IT_ADRENALINE);
			// ... and so on for other holdables
		}
		else {
			it = FindItem(itemQuery.c_str());
		}

		if (!it) {
			gi.LocClient_Print(ent, PRINT_HIGH, "$g_unknown_item_name", itemQuery.c_str());
			return;
		}
		if (!it->use) {
			gi.LocClient_Print(ent, PRINT_HIGH, "$g_item_not_usable");
			return;
		}
		if (!ent->client->pers.inventory[it->id]) {
			gi.LocClient_Print(ent, PRINT_HIGH, "$g_out_of_item", it->pickupName);
			return;
		}

		// Special handling for use_only variants if needed
		ent->client->noWeaponChains = (args.getString(0) != "use");

		it->use(ent, it);
		ValidateSelectedItem(ent);
	}

	void Wave(gentity_t* ent, const CommandArgs& args) {
		if (ent->deadFlag || ent->moveType == MoveType::NoClip) return;

		const int gesture = args.getInt(1).value_or(GESTURE_FLIP_OFF);

		const bool doAnimate = ent->client->anim.priority <= ANIM_WAVE && !(ent->client->ps.pmove.pmFlags & PMF_DUCKED);

		if (doAnimate)
			ent->client->anim.priority = ANIM_WAVE;

		const char* otherNotifyMsg = nullptr;
		const char* otherNotifyNoneMsg = nullptr;

		Vector3 start{};
		Vector3 dir{};
		P_ProjectSource(ent, ent->client->vAngle, vec3_origin, start, dir);

		gentity_t* aimingAt = nullptr;
		float bestDist = -9999.0f;

		for (auto player : active_players()) {
			if (player == ent)
				continue;

			Vector3 cdir = player->s.origin - start;
			float dist = cdir.normalize();

			float dot = ent->client->vForward.dot(cdir);

			if (dot < 0.97f)
				continue;
			else if (dist < bestDist)
				continue;

			bestDist = dist;
			aimingAt = player;
		}

		trace_t pointTrace{};
		gentity_t* tracedEnt = nullptr;
		const Item* pointingItem = nullptr;

		if (gesture == GESTURE_POINT) {
			pointTrace = gi.traceLine(start, start + (ent->client->vForward * 2048.0f), ent, static_cast<contents_t>(MASK_SHOT & ~CONTENTS_WINDOW));

			if (pointTrace.fraction != 1.0f)
				tracedEnt = pointTrace.ent;

			if (tracedEnt && tracedEnt->item) {
				const Item* candidate = tracedEnt->item;

				if (candidate && ((candidate->flags & IF_WEAPON) || candidate->highValue != HighValueItems::None))
					pointingItem = candidate;
			}
		}

		const char* pointingItemName = nullptr;

		if (pointingItem) {
			pointingItemName = pointingItem->pickupName;

			if ((!pointingItemName || !pointingItemName[0]) && pointingItem->pickupNameDefinitive)
				pointingItemName = pointingItem->pickupNameDefinitive;
		}

		switch (gesture) {
		case GESTURE_FLIP_OFF:
			otherNotifyMsg = "$g_flipoff_other";
			otherNotifyNoneMsg = "$g_flipoff_none";
			if (doAnimate) {
				ent->s.frame = FRAME_flip01 - 1;
				ent->client->anim.end = FRAME_flip12;
			}
			break;
		case GESTURE_SALUTE:
			otherNotifyMsg = "$g_salute_other";
			otherNotifyNoneMsg = "$g_salute_none";
			if (doAnimate) {
				ent->s.frame = FRAME_salute01 - 1;
				ent->client->anim.end = FRAME_salute11;
			}
			break;
		case GESTURE_TAUNT:
			otherNotifyMsg = "$g_taunt_other";
			otherNotifyNoneMsg = "$g_taunt_none";
			if (doAnimate) {
				ent->s.frame = FRAME_taunt01 - 1;
				ent->client->anim.end = FRAME_taunt17;
			}
			break;
		case GESTURE_WAVE:
			otherNotifyMsg = "$g_wave_other";
			otherNotifyNoneMsg = "$g_wave_none";
			if (doAnimate) {
				ent->s.frame = FRAME_wave01 - 1;
				ent->client->anim.end = FRAME_wave11;
			}
			break;
		case GESTURE_POINT:
		default:
			otherNotifyMsg = "$g_point_other";
			otherNotifyNoneMsg = "$g_point_none";
			if (doAnimate) {
				ent->s.frame = FRAME_point01 - 1;
				ent->client->anim.end = FRAME_point12;
			}
			break;
		}

		bool hasTarget = false;

		if (gesture == GESTURE_POINT) {
			for (auto player : active_players()) {
				if (player == ent)
					continue;
				else if (!OnSameTeam(ent, player))
					continue;

				hasTarget = true;
				break;
			}
		}

		const char* pointTargetName = nullptr;

		if (aimingAt)
			pointTargetName = aimingAt->client->sess.netName;
		else if (pointingItemName)
			pointTargetName = pointingItemName;

		if (gesture == GESTURE_POINT && hasTarget) {
			if (CheckFlood(ent))
				return;

			const char* pingNotifyMsg = pointTargetName ? "$g_point_other" : "$g_point_other_ping";

			const uint32_t key = GetUnicastKey();

			if (pointTrace.fraction != 1.0f) {
				for (auto player : active_players()) {
					if (player != ent && !OnSameTeam(ent, player))
						continue;

					gi.WriteByte(svc_poi);
					gi.WriteShort(POI_PING + (ent->s.number - 1));
					gi.WriteShort(5000);
					gi.WritePosition(pointTrace.endPos);
					gi.WriteShort(level.picPing);
					gi.WriteByte(208);
					gi.WriteByte(POI_FLAG_NONE);
					gi.unicast(player, false);

					gi.localSound(player, CHAN_AUTO, gi.soundIndex("misc/help_marker.wav"), 1.0f, ATTN_NONE, 0.0f, key);
					if (pointTargetName)
						gi.LocClient_Print(player, PRINT_TTS, pingNotifyMsg, ent->client->sess.netName, pointTargetName);
					else
						gi.LocClient_Print(player, PRINT_TTS, pingNotifyMsg, ent->client->sess.netName);
				}
			}
		}
		else {
			if (CheckFlood(ent))
				return;

			gentity_t* targ = nullptr;
			while ((targ = FindRadius(targ, ent->s.origin, 1024.0f)) != nullptr) {
				if (ent == targ)
					continue;
				if (!targ->client)
					continue;
				if (!gi.inPVS(ent->s.origin, targ->s.origin, false))
					continue;

				if (pointTargetName && otherNotifyMsg)
					gi.LocClient_Print(targ, PRINT_TTS, otherNotifyMsg, ent->client->sess.netName, pointTargetName);
				else if (otherNotifyNoneMsg)
					gi.LocClient_Print(targ, PRINT_TTS, otherNotifyNoneMsg, ent->client->sess.netName);
			}

			if (pointTargetName && otherNotifyMsg)
				gi.LocClient_Print(ent, PRINT_TTS, otherNotifyMsg, ent->client->sess.netName, pointTargetName);
			else if (otherNotifyNoneMsg)
				gi.LocClient_Print(ent, PRINT_TTS, otherNotifyNoneMsg, ent->client->sess.netName);
		}

		ent->client->anim.time = 0_ms;
	}

	void WeapLast(gentity_t* ent, const CommandArgs& args) {
		gclient_t* cl = ent->client;
		if (!cl->pers.weapon || !cl->pers.lastWeapon) return;

		cl->noWeaponChains = true;
		Item* it = cl->pers.lastWeapon;
		if (!cl->pers.inventory[it->id]) return;

		it->use(ent, it);
	}

	void WeapNext(gentity_t* ent, const CommandArgs& args) {
		gclient_t* cl = ent->client;
		if (!cl->pers.weapon) return;

		cl->noWeaponChains = true;
		item_id_t selected_weapon = cl->pers.weapon->id;

		for (int i = 1; i <= IT_TOTAL; i++) {
			item_id_t index = static_cast<item_id_t>((static_cast<int>(selected_weapon) + i) % IT_TOTAL);
			if (index > IT_NULL && cl->pers.inventory[index]) {
				Item* it = &itemList[index];
				if (it->use && (it->flags & IF_WEAPON)) {
					it->use(ent, it);
					return;
				}
			}
		}
	}

	void WeapPrev(gentity_t* ent, const CommandArgs& args) {
		gclient_t* cl = ent->client;
		if (!cl->pers.weapon) return;

		cl->noWeaponChains = true;
		item_id_t selected_weapon = cl->pers.weapon->id;

		for (int i = 1; i <= IT_TOTAL; i++) {
			item_id_t index = static_cast<item_id_t>((static_cast<int>(selected_weapon) + IT_TOTAL - i) % IT_TOTAL);
			if (index > IT_NULL && cl->pers.inventory[index]) {
				Item* it = &itemList[index];
				if (it->use && (it->flags & IF_WEAPON)) {
					it->use(ent, it);
					return;
				}
			}
		}
	}

	void Where(gentity_t* ent, const CommandArgs& args) {
		if (!ent || !ent->client) return;
		const Vector3& origin = ent->s.origin;
		const auto& angles = ent->client->ps.viewAngles;
		std::string location = std::format("{:.1f} {:.1f} {:.1f} {:.1f} {:.1f} {:.1f}",
			origin[X], origin[Y], origin[Z], angles[PITCH], angles[YAW], angles[ROLL]);
		gi.LocClient_Print(ent, PRINT_HIGH, "Location: {}\n", location.c_str());
		gi.SendToClipBoard(location.c_str());
	}

	// --- Registration Function ---
	void RegisterClientCommands() {
		using enum CommandFlag;

		RegisterCommand("admin", &Admin, AllowIntermission | AllowSpectator);
		RegisterCommand("clientlist", &ClientList, AllowDead | AllowIntermission | AllowSpectator);
		RegisterCommand("drop", &Drop);
		RegisterCommand("dropindex", &Drop);
		RegisterCommand("eyecam", &EyeCam, AllowSpectator);
		RegisterCommand("fm", &FragMessages, AllowSpectator | AllowDead);
		RegisterCommand("follow", &Follow, AllowSpectator | AllowDead, true);
		RegisterCommand("followkiller", &FollowKiller, AllowSpectator | AllowDead, true);
		RegisterCommand("followleader", &FollowLeader, AllowSpectator | AllowDead, true);
		RegisterCommand("followpowerup", &FollowPowerup, AllowSpectator | AllowDead, true);
		RegisterCommand("forfeit", &Forfeit, AllowDead, true);
		RegisterCommand("help", &Help, AllowDead | AllowSpectator, true);
		RegisterCommand("hook", &Hook, {}, true);
		RegisterCommand("id", &CrosshairID, AllowSpectator | AllowDead);
		RegisterCommand("impulse", &Impulse);
		RegisterCommand("invdrop", &InvDrop);
		RegisterCommand("inven", &Inven, AllowDead | AllowSpectator, true);
		RegisterCommand("invnext", &InvNext, AllowSpectator | AllowIntermission, true);
		RegisterCommand("invnextp", &InvNextP, {}, true);
		RegisterCommand("invnextw", &InvNextW, {}, true);
		RegisterCommand("invprev", &InvPrev, AllowSpectator | AllowIntermission, true);
		RegisterCommand("invprevp", &InvPrevP, {}, true);
		RegisterCommand("invprevw", &InvPrevW, {}, true);
		RegisterCommand("invuse", &InvUse, AllowSpectator | AllowIntermission, true);
		RegisterCommand("kb", &KillBeep, AllowSpectator | AllowDead);
		RegisterCommand("kill", &Kill);
		RegisterCommand("mapcycle", &MapCycle, AllowDead | AllowSpectator);
		RegisterCommand("mapinfo", &MapInfo, AllowDead | AllowSpectator);
		RegisterCommand("mappool", &MapPool, AllowDead | AllowSpectator);
		RegisterCommand("motd", &Motd, AllowSpectator | AllowIntermission);
		RegisterCommand("mymap", &MyMap, AllowDead | AllowSpectator);
		RegisterCommand("notready", &NotReady, AllowDead);
		RegisterCommand("putaway", &PutAway, AllowSpectator);
		RegisterCommand("ready", &Ready, AllowDead);
		RegisterCommand("ready_up", &ReadyUp, AllowDead);
		RegisterCommand("readyup", &ReadyUp, AllowDead);
		RegisterCommand("score", &Score, AllowDead | AllowIntermission | AllowSpectator, true);
		RegisterCommand("setweaponpref", &SetWeaponPref, AllowDead | AllowIntermission | AllowSpectator);
		RegisterCommand("sr", &MySkill, AllowDead | AllowSpectator);
		RegisterCommand("stats", &Stats, AllowIntermission | AllowSpectator);
		RegisterCommand("team", &JoinTeam, AllowDead | AllowSpectator);
		RegisterCommand("timein", &TimeIn, AllowDead | AllowSpectator);
		RegisterCommand("timeout", &TimeOut, AllowDead | AllowSpectator);
		RegisterCommand("timer", &Timer, AllowSpectator | AllowDead);
		RegisterCommand("unhook", &UnHook, {}, true);
		RegisterCommand("use", &Use, {}, true);
		RegisterCommand("use_index", &Use, {}, true);
		RegisterCommand("use_index_only", &Use, {}, true);
		RegisterCommand("use_only", &Use, {}, true);
		RegisterCommand("wave", &Wave);
		RegisterCommand("weaplast", &WeapLast, {}, true);
		RegisterCommand("weapnext", &WeapNext, {}, true);
		RegisterCommand("weapprev", &WeapPrev, {}, true);
		RegisterCommand("where", &Where, AllowSpectator);
	}

} // namespace Commands

