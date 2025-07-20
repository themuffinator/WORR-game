#include "../g_local.h"
#include "../g_statusbar.h"

/*
===============
Menu::Next
===============
*/
void Menu::Next() {
	if (entries.empty())
		return;

	const int count = static_cast<int>(entries.size());
	const int start = current;

	do {
		current = (current + 1) % count;
	} while (!entries[current].onSelect && current != start);
}

/*
===============
Menu::Prev
===============
*/
void Menu::Prev() {
	if (entries.empty())
		return;

	const int count = static_cast<int>(entries.size());
	const int start = current;

	do {
		current = (current - 1 + count) % count;
	} while (!entries[current].onSelect && current != start);
}

/*
===============
Menu::Select
===============
*/
void Menu::Select(gentity_t *ent) {
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
void Menu::Render(gentity_t *ent) const {
	if (onUpdate)
		onUpdate(ent, *this);

	// Do not early-return if current is invalid; still render the menu
	const int selected = (current >= 0 && current < static_cast<int>(entries.size())) ? current : -1;

	statusbar_t sb;
	sb.xv(32).yv(8).picn("inventory");

	// Collect visible entries
	std::vector<const MenuEntry *> visibleEntries;
	int scrollCount = 0;
	bool scrolledDown = false;

	for (const auto &entry : entries) {
		if (!entry.scrollable || scrollCount < MAX_VISIBLE_LINES) {
			visibleEntries.push_back(&entry);
			if (entry.scrollable)
				++scrollCount;
		} else {
			scrolledDown = true;
			break;
		}
	}

	// Render visible entries
	int y = 32;

	for (const MenuEntry *entry : visibleEntries) {
		int x = 64;
		const char *loc_func = "loc_string";

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
			} else {
				sb.sb << loc_func << " 1 \"" << entry->text << "\" \"" << entry->textArg << "\" ";
			}
		}

		y += 8;  // always advance line
	}

	if (scrolledDown) {
		sb.yv(y).xv(4);
		sb.string("...\n");
	}

	gi.WriteByte(svc_layout);
	gi.WriteString(sb.sb.str().c_str());
}
