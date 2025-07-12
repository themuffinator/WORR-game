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

	// Bounds check to avoid accessing invalid index
	if (current < 0 || current >= static_cast<int>(entries.size()))
		return;

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
		if (entry->text.empty())
			continue;

		int x = 64;
		const char *loc_func = "loc_string";

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
		sb.sb << loc_func << " 1 \"" << entry->text << "\" \"" << entry->textArg << "\" ";

		if (entry == &entries[current]) {
			sb.xv(56);
			sb.string2("> ");
		}

		y += 8;
	}

	if (scrolledDown) {
		sb.yv(y).xv(4);
		sb.string("...\n");
	}

	gi.WriteByte(svc_layout);
	gi.WriteString(sb.sb.str().c_str());
}
