#include "../g_local.h"

namespace {

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

} // namespace

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

	ent->client->menu = std::move(menu);
	ent->client->showScores = false;
	ent->client->inMenu = true;
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
	ent->client->showScores = false;
}

/*
===============
MenuSystem::Update
===============
*/
void MenuSystem::Update(gentity_t *ent) {
	if (!ent || !ent->client || !ent->client->menu)
		return;

	//if (level.time - ent->client->menuTime >= 1_sec) {
		ent->client->menu->Render(ent);
		gi.unicast(ent, true);
		//ent->client->menuTime = level.time + 1_sec;
	//}

	ent->client->menuTime = level.time;
	ent->client->menuDirty = true;

	gi.local_sound(ent, CHAN_AUTO, gi.soundindex("misc/menu2.wav"), 1, ATTN_NONE, 0);
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
