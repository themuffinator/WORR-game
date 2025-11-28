/*Copyright (c) 2024 The DarkMatter Project
Licensed under the GNU General Public License 2.0.

menu_main.cpp (Menu Main) This file serves as the primary entry point and hub for the modern,
C++ object-oriented menu system. It defines the main join menu that players first see, from
which they can navigate to other menus like settings, server info, or calling a vote. Key
Responsibilities: - Main Menu Construction: `OpenJoinMenu` (and its helper `G_Menu_Join_Update`)
dynamically builds the main menu based on the current gametype (FFA vs. Team), player counts,
and server settings. - Navigation Hub: Provides the functions that act as entry points to all
other sub-menus (e.g., `G_Menu_HostInfo`, `G_Menu_CallVote`). - State-Sensitive Options:
Dynamically enables or disables menu options based on game state, such as showing "Admin"
options only for admin players or hiding team-join options in FFA.*/

#include <algorithm>
#include <cmath>
#include <memory>

namespace std {
	using ::sinf;
}

#include "../g_local.hpp"
#include "../gameplay/g_statusbar.hpp"

#include <algorithm>

namespace {

/*
=============
CountScrollableEntries

Returns the total number of scrollable menu entries in the provided list.
=============
*/
	int CountScrollableEntries(const std::vector<MenuEntry>& entries) {
		return static_cast<int>(std::count_if(entries.begin(), entries.end(), [](const MenuEntry& entry) {
			return entry.scrollable;
		}));
	}

/*
=============
ScrollableIndexFor

Converts an entry index into its corresponding scrollable index position.
=============
*/
	int ScrollableIndexFor(const std::vector<MenuEntry>& entries, int index) {
		int count = 0;
		for (int i = 0; i < index && i < static_cast<int>(entries.size()); ++i) {
			if (entries[i].scrollable)
				++count;
		}
		return count;
	}

/*
=============
CollectVisibleEntries

Builds the list of entries that should be rendered based on the current
scroll offset.
=============
*/
	std::vector<const MenuEntry*> CollectVisibleEntries(const std::vector<MenuEntry>& entries, int offset, int maxOffset) {
		int skippedScrollable = offset;
		int visibleScrollable = 0;
		std::vector<const MenuEntry*> visibleEntries;
		visibleEntries.reserve(entries.size());

		for (const MenuEntry& entry : entries) {
			if (entry.scrollable) {
				if (skippedScrollable > 0) {
					--skippedScrollable;
					continue;
				}

				if (visibleScrollable >= MAX_VISIBLE_LINES)
					continue;

				++visibleScrollable;
				visibleEntries.push_back(&entry);
				continue;
			}

			if (visibleScrollable < MAX_VISIBLE_LINES || offset == maxOffset)
				visibleEntries.push_back(&entry);
		}

		return visibleEntries;
}

/*
=============
TrimUpdatedEntries

Re-applies width trimming to menu entries after runtime updates to keep
alignment and scroll calculations consistent with the rendered text.
=============
*/
	void TrimUpdatedEntries(Menu& menu) {
		for (MenuEntry& entry : menu.entries) {
			const std::string trimmed = TrimToWidth(entry.text);
			if (trimmed != entry.text)
				entry.text = trimmed;
		}
	}

} // namespace

/*
===============
Menu::Next
===============
*/
void Menu::Next() {
	if (entries.empty())
		return;

	if (std::none_of(entries.begin(), entries.end(), [](const MenuEntry& entry) { return static_cast<bool>(entry.onSelect); }))
		return;

	const int count = static_cast<int>(entries.size());
	const int start = (current >= 0 && current < count) ? current : (count - 1);
	int index = start;

	do {
		index = (index + 1) % count;

		if (entries[index].onSelect) {
			current = index;
			return;
		}
	} while (index != start);
}

/*
===============
Menu::Prev
===============
*/
void Menu::Prev() {
	if (entries.empty())
		return;

	if (std::none_of(entries.begin(), entries.end(), [](const MenuEntry& entry) { return static_cast<bool>(entry.onSelect); }))
		return;

	const int count = static_cast<int>(entries.size());
	const int start = (current >= 0 && current < count) ? current : 0;
	int index = start;

	do {
		index = (index - 1 + count) % count;

		if (entries[index].onSelect) {
			current = index;
			return;
		}
	} while (index != start);
}

/*
===============
Menu::Select
===============
*/
void Menu::Select(gentity_t* ent) {
	if (current < 0 || current >= static_cast<int>(entries.size()))
		return;

	if (entries[current].onSelect)
		entries[current].onSelect(ent, *this);
}

/*
===============
Menu::Render
===============
*/
void Menu::Render(gentity_t* ent) const {
	Menu& mutableMenu = *const_cast<Menu*>(this);

	if (onUpdate)
		onUpdate(ent, *this);

	TrimUpdatedEntries(mutableMenu);

	// Do not early-return if current is invalid; still render the menu
	const int selected = (current >= 0 && current < static_cast<int>(entries.size())) ? current : -1;

	statusbar_t sb;
	sb.xv(32).yv(8).picn("inventory");

	const int totalScrollable = CountScrollableEntries(entries);
	const int maxOffset = std::max(0, totalScrollable - MAX_VISIBLE_LINES);
	const int offset = std::clamp(scrollOffset, 0, maxOffset);

	const bool hasAbove = (offset > 0);
	const bool hasBelow = (offset < maxOffset);

	std::vector<const MenuEntry*> visibleEntries = CollectVisibleEntries(entries, offset, maxOffset);

	int y = 32;

	if (hasAbove) {
		sb.yv(y).xv(4);
		sb.string("^\n");
		y += 8;
	}

	for (const MenuEntry* entry : visibleEntries) {
		int x = 64;
		const char* loc_func = "loc_string";

		if (!entry->text.empty()) {
			switch (entry->align) {
			case MenuAlign::Center:
				x = 0;
				loc_func = "loc_cstring";
				break;
			case MenuAlign::Right:
				x = 260;
				loc_func = "loc_rstring";
				break;
			default:
				break;
			}

			sb.yv(y).xv(x);

			if (selected >= 0 && entry == &entries[selected]) {
				sb.string2("> ");
				sb.xv(x + 12); // indent text slightly after >
				sb.string2(entry->text.c_str());
			}
			else {
				sb.sb << loc_func << " 1 \"" << entry->text << "\" \"" << entry->textArg << "\" ";
			}
		}

		y += 8;  // always advance line
	}

	if (hasBelow) {
		sb.yv(y).xv(4);
		sb.string("v\n");
	}

	gi.WriteByte(svc_layout);
	gi.WriteString(sb.sb.str().c_str());
}

/*
===============
Menu::EnsureCurrentVisible
===============
*/
void Menu::EnsureCurrentVisible() {
	const int totalScrollable = CountScrollableEntries(entries);
	const int maxOffset = std::max(0, totalScrollable - MAX_VISIBLE_LINES);

	scrollOffset = std::clamp(scrollOffset, 0, maxOffset);

	if (current < 0 || current >= static_cast<int>(entries.size()))
		return;

	if (!entries[current].scrollable) {
		if (current == 0) {
			scrollOffset = 0;
		}
		else if (current == static_cast<int>(entries.size()) - 1) {
			scrollOffset = maxOffset;
		}
		return;
	}

	const int scrollIndex = ScrollableIndexFor(entries, current);
	const int halfWindow = MAX_VISIBLE_LINES / 2;
	const int desiredOffset = std::clamp(scrollIndex - halfWindow, 0, maxOffset);

	if (scrollIndex < scrollOffset || scrollIndex >= scrollOffset + MAX_VISIBLE_LINES) {
		scrollOffset = desiredOffset;
	}

	scrollOffset = std::clamp(scrollOffset, 0, maxOffset);
}
