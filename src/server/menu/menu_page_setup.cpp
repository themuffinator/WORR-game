// menu_page_setup.cpp (Menu Page - Match Setup)
// This file implements a multi-page wizard-style menu for setting up a custom
// match. It guides the user through selecting a gametype, modifiers, player
// count, and other options.
//
// Key Responsibilities:
// - State Management: Uses a `MatchSetupState` struct to hold the user's
//   selections as they navigate through the different setup pages.
// - Wizard Flow: Each menu page (e.g., `OpenSetupGametypeMenu`,
//   `OpenSetupModifierMenu`) handles one aspect of the setup and then calls
//   the next function in the sequence, creating a step-by-step setup process.
// - Finalization: The final step in the wizard calls `FinishMatchSetup`, which
//   applies the chosen settings to the server and closes the menu.

#include "../g_local.hpp"

/*
===============
MatchSetupState
===============
*/
struct MatchSetupState {
	std::string gametype = "ffa";
	std::string modifier = "standard";
	int maxPlayers = 8;
	std::string length = "standard";
	std::string type = "standard";
	std::string bestOf = "bo1";
};

static void FinishMatchSetup(gentity_t* ent, MatchSetupState* state) {
	// Example: Apply config and broadcast
	gi.Com_PrintFmt("Match setup complete: gametype=%s modifier=%s players=%d length=%s type=%s bestof=%s\n",
		state->gametype.c_str(), state->modifier.c_str(), state->maxPlayers,
		state->length.c_str(), state->type.c_str(), state->bestOf.c_str());

	gi.TagFree(state);
	MenuSystem::Close(ent);
}

/*
===============
OpenSetupBestOfMenu
===============
*/
static void OpenSetupBestOfMenu(gentity_t* ent, MatchSetupState* state) {
	MenuBuilder b;
	b.add("Best Of", MenuAlign::Center).spacer();
	b.add("BO1", MenuAlign::Left, [=](gentity_t* e, Menu& m) {
		state->bestOf = "bo1";
		FinishMatchSetup(e, state);
		});
	b.add("BO3", MenuAlign::Left, [=](gentity_t* e, Menu& m) {
		state->bestOf = "bo3";
		FinishMatchSetup(e, state);
		});
	b.add("BO5", MenuAlign::Left, [=](gentity_t* e, Menu& m) {
		state->bestOf = "bo5";
		FinishMatchSetup(e, state);
		});
	MenuSystem::Open(ent, b.build());
}

/*
===============
OpenSetupMatchTypeMenu
===============
*/
static void OpenSetupMatchTypeMenu(gentity_t* ent, MatchSetupState* state) {
	MenuBuilder b;
	b.add("Match Type", MenuAlign::Center).spacer();
	b.add("Casual", MenuAlign::Left, [=](gentity_t* e, Menu& m) {
		state->type = "casual";
		FinishMatchSetup(e, state);
		});
	b.add("Standard", MenuAlign::Left, [=](gentity_t* e, Menu& m) {
		state->type = "standard";
		FinishMatchSetup(e, state);
		});
	b.add("Competitive", MenuAlign::Left, [=](gentity_t* e, Menu& m) {
		state->type = "competitive";
		FinishMatchSetup(e, state);
		});
	b.add("Tournament", MenuAlign::Left, [=](gentity_t* e, Menu& m) {
		state->type = "tournament";
		if (state->gametype == "duel" || state->gametype == "tdm" || state->gametype == "ctf") {
			OpenSetupBestOfMenu(e, state);
		}
		else {
			FinishMatchSetup(e, state);
		}
		});
	MenuSystem::Open(ent, b.build());
}

/*
===============
OpenSetupMatchLengthMenu
===============
*/
static void OpenSetupMatchLengthMenu(gentity_t* ent, MatchSetupState* state) {
	MenuBuilder b;
	b.add("Match Length", MenuAlign::Center).spacer();
	b.add("Short", MenuAlign::Left, [=](gentity_t* e, Menu& m) {
		state->length = "short";
		(state->gametype == "practice") ? FinishMatchSetup(e, state) : OpenSetupMatchTypeMenu(e, state);
		});
	b.add("Standard", MenuAlign::Left, [=](gentity_t* e, Menu& m) {
		state->length = "standard";
		(state->gametype == "practice") ? FinishMatchSetup(e, state) : OpenSetupMatchTypeMenu(e, state);
		});
	b.add("Long", MenuAlign::Left, [=](gentity_t* e, Menu& m) {
		state->length = "long";
		(state->gametype == "practice") ? FinishMatchSetup(e, state) : OpenSetupMatchTypeMenu(e, state);
		});
	b.add("Endurance", MenuAlign::Left, [=](gentity_t* e, Menu& m) {
		state->length = "endurance";
		(state->gametype == "practice") ? FinishMatchSetup(e, state) : OpenSetupMatchTypeMenu(e, state);
		});
	MenuSystem::Open(ent, b.build());
}

/*
===============
OpenSetupMaxPlayersMenu
===============
*/
static void OpenSetupMaxPlayersMenu(gentity_t* ent, MatchSetupState* state) {
	if (state->gametype == "duel") {
		state->maxPlayers = 2;
		OpenSetupMatchLengthMenu(ent, state);
		return;
	}

	MenuBuilder b;
	b.add("Max Players", MenuAlign::Center).spacer();
	for (int count : {2, 4, 8, 16}) {
		b.add(G_Fmt("%d", count).data(), MenuAlign::Left, [=](gentity_t* e, Menu& m) {
			state->maxPlayers = count;
			OpenSetupMatchLengthMenu(e, state);
			});
	}
	MenuSystem::Open(ent, b.build());
}

/*
===============
OpenSetupModifierMenu
===============
*/
static void OpenSetupModifierMenu(gentity_t* ent, MatchSetupState* state) {
	MenuBuilder b;
	b.add("Modifiers", MenuAlign::Center).spacer();
	b.add("Standard", MenuAlign::Left, [=](gentity_t* e, Menu& m) {
		state->modifier = "standard";
		OpenSetupMaxPlayersMenu(e, state);
		});
	b.add("InstaGib", MenuAlign::Left, [=](gentity_t* e, Menu& m) {
		state->modifier = "instagib";
		OpenSetupMaxPlayersMenu(e, state);
		});
	b.add("Vampiric Damage", MenuAlign::Left, [=](gentity_t* e, Menu& m) {
		state->modifier = "vampiric";
		OpenSetupMaxPlayersMenu(e, state);
		});
	b.add("Frenzy", MenuAlign::Left, [=](gentity_t* e, Menu& m) {
		state->modifier = "frenzy";
		OpenSetupMaxPlayersMenu(e, state);
		});
	MenuSystem::Open(ent, b.build());
}

/*
===============
OpenSetupGametypeMenu
===============
*/
static void OpenSetupGametypeMenu(gentity_t* ent, MatchSetupState* state) {
	MenuBuilder b;
	b.add("Gametype", MenuAlign::Center).spacer();
        for (auto& [label, value] : std::vector<std::pair<const char*, const char*>>{
                {"Practice", "practice"},
                {"Free For All", "ffa"},
                {"Duel", "duel"},
                {"Team Deathmatch", "tdm"},
                {"Domination", "dom"},
                {"Capture the Flag", "ctf"},
                }) {
		b.add(label, MenuAlign::Left, [=](gentity_t* e, Menu& m) {
			state->gametype = value;
			OpenSetupModifierMenu(e, state);
			});
	}
	MenuSystem::Open(ent, b.build());
}

/*
===============
OpenSetupWelcomeMenu
===============
*/
void OpenSetupWelcomeMenu(gentity_t* ent) {
	auto* state = static_cast<MatchSetupState*>(gi.TagMalloc(sizeof(MatchSetupState), TAG_LEVEL));
	new (state) MatchSetupState();

	MenuBuilder b;
	b.add("Welcome to", MenuAlign::Center).spacer();
	b.add(G_Fmt("{} v{}", worr::version::kGameTitle, worr::version::kGameVersion).data(), MenuAlign::Center).spacer();
	b.add("Set Up Match", MenuAlign::Left, [=](gentity_t* e, Menu& m) {
		OpenSetupGametypeMenu(e, state);
		});
	b.add("Run Custom Config", MenuAlign::Left, [](gentity_t* e, Menu& m) {
		gi.AddCommandString("exec server.cfg\n");
		MenuSystem::Close(e);
		});
	MenuSystem::Open(ent, b.build());
}
