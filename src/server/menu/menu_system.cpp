/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

menu_system.cpp (Menu System) This file implements the core functionality for the modern, object-oriented menu system. It defines the behavior of the `Menu` class and the `MenuSystem` static class, which work together to manage the lifecycle of in-game menus. Key Responsibilities: - Menu Navigation: Implements the `Menu::Next()` and `Menu::Prev()` methods for navigating between selectable menu items. - Menu Action: The `Menu::Select()` method handles the execution of the callback function associated with the currently selected menu item. - Menu Rendering: `Menu::Render()` constructs the layout string for the current menu state, which is then sent to the client for display. It supports scrolling for menus with more items than can be displayed at once. - System Management: The `MenuSystem` class provides the main interface for opening (`Open`), closing (`Close`), and updating (`Update`) menus for a given player.*/

#include "../g_local.hpp"

/*
===============
TrimToWidth
===============
*/
static std::string TrimToWidth(const std::string& text) {
	if (text.size() > MAX_MENU_WIDTH)
		return text.substr(0, static_cast<size_t>(MAX_MENU_WIDTH - 3)) + "...";
	return text;
}

/*
===============
MenuSystem::Open
===============
*/
void MenuSystem::Open(gentity_t* ent, std::unique_ptr<Menu> menu) {
	if (!ent || !ent->client)
		return;

	if (ent->client->menu.current)
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

	menu->scrollOffset = 0;
	menu->EnsureCurrentVisible();

	auto& menuState = ent->client->menu;
	ent->client->menu.current = std::move(menu);

	menuState.previousStatusBar = ent->client->ps.stats[STAT_SHOW_STATUSBAR];
	menuState.restoreStatusBar = true;
	ent->client->ps.stats[STAT_SHOW_STATUSBAR] = 1;

	// These two are required to render layouts!
	ent->client->showScores = true;   // <- must be true!

	menuState.updateTime = level.time;
	menuState.doUpdate = true;
}

/*
===============
MenuSystem::Close
===============
*/
void MenuSystem::Close(gentity_t* ent) {
	if (!ent || !ent->client)
		return;

	auto& menuState = ent->client->menu;

	if (menuState.current) {
		menuState.current.reset();
		menuState.current = nullptr;
	}

	if (menuState.restoreStatusBar) {
		ent->client->ps.stats[STAT_SHOW_STATUSBAR] = menuState.previousStatusBar;
		menuState.restoreStatusBar = false;
		menuState.previousStatusBar = 0;
	}

	//ent->client->showScores = false;
}

/*
===============
MenuSystem::Update
===============
*/
void MenuSystem::Update(gentity_t* ent) {
	if (!ent || !ent->client || !ent->client->menu.current) {
		//gi.Com_Print("MenuSystem::Update skipped (nullptr)\n");
		return;
	}

	//gi.Com_PrintFmt("MenuSystem::Update: rendering for {}\n", ent->client->pers.netName);
	ent->client->menu.current->Render(ent);
	gi.unicast(ent, true);
	ent->client->menu.doUpdate = false;
	ent->client->menu.updateTime = level.time;
}

/*
===============
MenuSystem::DirtyAll
===============
*/
void MenuSystem::DirtyAll() {
	for (gentity_t* player : active_clients()) {
		if (player->client->menu.current) {
			player->client->menu.doUpdate = true;
			player->client->menu.updateTime = level.time;
		}
	}
}
