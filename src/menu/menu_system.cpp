#include "../g_local.h"

/*
===============
TrimToWidth
===============
*/
static std::string TrimToWidth(const std::string &text) {
	if (text.size() > MAX_MENU_WIDTH)
		return text.substr(0, static_cast<size_t>(MAX_MENU_WIDTH - 3)) + "...";
	return text;
}

/*
===============
MenuSystem::Open
===============
*/
void MenuSystem::Open(gentity_t *ent, std::unique_ptr<Menu> menu) {
	if (!ent || !ent->client)
		return;

	if (ent->client->menu)
		Close(ent);

	const int total = static_cast<int>(menu->entries.size());

	for (int i = 0; i < total; ++i) {
		menu->entries[i].text = TrimToWidth(menu->entries[i].text);
		menu->entries[i].scrollable = (i > 0 && i < total - 1);
	}

	// Select the first entry with a valid onSelect
	menu->current = -1;
	for (size_t i = 0; i < menu->entries.size(); ++i) {
		if (menu->entries[i].onSelect) {
			menu->current = static_cast<int>(i);
			break;
		}
	}

	ent->client->menu = std::move(menu);

	// These two are required to render layouts!
	ent->client->showScores = true;   // <- must be true!

	ent->client->menuTime = level.time;
	ent->client->menuDirty = true;
}

/*
===============
MenuSystem::Close
===============
*/
void MenuSystem::Close(gentity_t *ent) {
	if (!ent || !ent->client || !ent->client->menu)
		return;

	ent->client->menu.reset();
	ent->client->menu = nullptr;
	//ent->client->showScores = false;
}

/*
===============
MenuSystem::Update
===============
*/
void MenuSystem::Update(gentity_t *ent) {
	if (!ent || !ent->client || !ent->client->menu) {
		//gi.Com_Print("MenuSystem::Update skipped (nullptr)\n");
		return;
	}

	//gi.Com_PrintFmt("MenuSystem::Update: rendering for {}\n", ent->client->pers.netname);
	ent->client->menu->Render(ent);
	gi.unicast(ent, true);
	ent->client->menuDirty = false;
	ent->client->menuTime = level.time;
}

/*
===============
MenuSystem::DirtyAll
===============
*/
void MenuSystem::DirtyAll() {
	for (gentity_t *player : active_clients()) {
		if (player->client->menu) {
			player->client->menuDirty = true;
			player->client->menuTime = level.time;
		}
	}
}
