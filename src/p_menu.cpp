// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"

/*
============
P_Menu_Dirty
============
*/
void P_Menu_Dirty() {
	for (auto player : active_clients())
		if (player->client->menu) {
			player->client->menuDirty = true;
			player->client->menuTime = level.time;
		}
}

// Note that the pmenu entries are duplicated
// this is so that a static set of pmenu entries can be used
// for multiple clients and changed without interference
// note that arg will be freed when the menu is closed, it must be allocated memory
MenuHandle *P_Menu_Open(Entity *ent, const PMenu *entries, int cur, int num, void *arg, MenuUpdateFunc UpdateFunc) {
	MenuHandle	*hnd;
	const PMenu	*p;
	size_t		i;

	if (!ent->client)
		return nullptr;

	if (ent->client->menu) {
		gi.Com_Print("Warning: client already has a menu.\n");
		if (!Vote_Menu_Active(ent))
			P_Menu_Close(ent);
	}

	hnd = (MenuHandle *)gi.TagMalloc(sizeof(*hnd), TAG_LEVEL);
	hnd->UpdateFunc = UpdateFunc;

	hnd->arg = arg;
	hnd->entries = (PMenu *)gi.TagMalloc(sizeof(PMenu) * num, TAG_LEVEL);
	memcpy(hnd->entries, entries, sizeof(PMenu) * num);
	// duplicate the strings since they may be from static memory
	for (i = 0; i < num; i++)
		Q_strlcpy(hnd->entries[i].text, entries[i].text, sizeof(entries[i].text));

	hnd->num = num;

	if (cur < 0 || !entries[cur].SelectFunc) {
		for (i = 0, p = entries; i < num; i++, p++)
			if (p->SelectFunc)
				break;
	} else
		i = cur;

	if (i >= num)
		hnd->cur = -1;
	else
		hnd->cur = i;

	ent->client->showScores = true;
	ent->client->inMenu = true;
	ent->client->menu = hnd;
	ent->client->ps.stats[STAT_SHOW_STATUSBAR] = 0;

	if (UpdateFunc)
		UpdateFunc(ent);

	P_Menu_Do_Update(ent);
	gi.unicast(ent, true);

	return hnd;
}

void P_Menu_Close(Entity *ent) {
	MenuHandle *hnd;

	if (!ent->client->menu)
		return;

	hnd = ent->client->menu;
	gi.TagFree(hnd->entries);
	if (hnd->arg)
		gi.TagFree(hnd->arg);
	gi.TagFree(hnd);
	ent->client->menu = nullptr;
	ent->client->showScores = false;
	
	Entity *e = ent->client->followTarget ? ent->client->followTarget : ent;
	ent->client->ps.stats[STAT_SHOW_STATUSBAR] = !ClientIsPlaying(e->client) ? 0 : 1;
}

// only use on pmenu's that have been called with P_Menu_Open
void P_Menu_UpdateEntry(PMenu *entry, const char *text, int align, MenuSelectFunc SelectFunc) {
	Q_strlcpy(entry->text, text, sizeof(entry->text));
	entry->align = align;
	entry->SelectFunc = SelectFunc;
}

#include "g_statusbar.h"

void P_Menu_Do_Update(Entity *ent) {
	int			i;
	PMenu		*p;
	int			x;
	MenuHandle	*hnd;
	const char	*t;
	bool		alt = false;

	if (!ent->client->menu) {
		gi.Com_Print("Warning: ent has no menu\n");
		return;
	}

	hnd = ent->client->menu;

	if (hnd->UpdateFunc)
		hnd->UpdateFunc(ent);

	statusbar_t sb;

	sb.xv(32).yv(8).picn("inventory");

	for (i = 0, p = hnd->entries; i < hnd->num; i++, p++) {
		if (!*(p->text))
			continue; // blank line

		t = p->text;

		if (*t == '*') {
			alt = true;
			t++;
		}

		sb.yv(32 + i * 8);

		const char *loc_func = "loc_string";

		if (p->align == MENU_ALIGN_CENTER) {
			x = 0;
			loc_func = "loc_cstring";
		} else if (p->align == MENU_ALIGN_RIGHT) {
			x = 260;
			loc_func = "loc_rstring";
		} else
			x = 64;

		sb.xv(x);

		sb.sb << loc_func;

		if (hnd->cur == i || alt)
			sb.sb << '2';

		sb.sb << " 1 \"" << t << "\" \"" << p->text_arg1 << "\" ";

		if (hnd->cur == i) {
			sb.xv(56);
			sb.string2("\">\"");
		}

		alt = false;
	}

	gi.WriteByte(svc_layout);
	gi.WriteString(sb.sb.str().c_str());
}

void P_Menu_Update(Entity *ent) {
	if (!ent->client->menu) {
		gi.Com_Print("Warning: ent has no menu\n");
		return;
	}

	if (level.time - ent->client->menuTime >= 1_sec) {
		// been a second or more since last update, update now
		P_Menu_Do_Update(ent);
		gi.unicast(ent, true);
		ent->client->menuTime = level.time + 1_sec;
		ent->client->menuDirty = false;
		gi.Com_PrintFmt("{} lvltime:{} menut:{} dirty:{}\n", __FUNCTION__, level.time.seconds(), ent->client->menuTime.seconds(), ent->client->menuDirty);
	}

	ent->client->menuTime = level.time;
	ent->client->menuDirty = true;
	gi.local_sound(ent, CHAN_AUTO, gi.soundindex("misc/menu2.wav"), 1, ATTN_NONE, 0);
	//gi.Com_PrintFmt("{} lvltime:{} menut:{} dirty:{}\n", __FUNCTION__, level.time.seconds(), ent->client->menuTime.seconds(), ent->client->menuDirty);
}

void P_Menu_Next(Entity *ent) {
	MenuHandle	*hnd;
	int			i;
	PMenu		*p;

	if (!ent->client->menu) {
		gi.Com_Print("Warning: ent has no menu\n");
		return;
	}

	hnd = ent->client->menu;

	if (hnd->cur < 0)
		return; // no selectable entries

	i = hnd->cur;
	p = hnd->entries + hnd->cur;
	do {
		i++;
		p++;
		if (i == hnd->num) {
			i = 0;
			p = hnd->entries;
		}
		if (p->SelectFunc)
			break;
	} while (i != hnd->cur);

	hnd->cur = i;

	//gi.Com_PrintFmt("{} cursor pos: {}\n", __FUNCTION__, hnd->cur);

	P_Menu_Update(ent);
}

void P_Menu_Prev(Entity *ent) {
	MenuHandle	*hnd;
	int			i;
	PMenu		*p;

	if (!ent->client->menu) {
		gi.Com_Print("Warning: ent has no menu\n");
		return;
	}

	hnd = ent->client->menu;

	if (hnd->cur < 0)
		return; // no selectable entries

	i = hnd->cur;
	p = hnd->entries + hnd->cur;
	do {
		if (i == 0) {
			i = hnd->num - 1;
			p = hnd->entries + i;
		} else {
			i--;
			p--;
		}
		if (p->SelectFunc)
			break;
	} while (i != hnd->cur);

	hnd->cur = i;

	//gi.Com_PrintFmt("{} cursor pos: {}\n", __FUNCTION__, hnd->cur);

	P_Menu_Update(ent);
}

void P_Menu_Select(Entity *ent) {
	MenuHandle	*hnd;
	PMenu		*p;

	if (!ent->client->menu) {
		gi.Com_Print("Warning: ent has no menu\n");
		return;
	}

	// no selecting during intermission
	if (!level.mapSelectorVoteStartTime)
		if ((level.intermissionQueued || level.intermissionTime))
			return;

	hnd = ent->client->menu;

	if (hnd->cur < 0)
		return; // no selectable entries

	p = hnd->entries + hnd->cur;

	if (p->SelectFunc)
		p->SelectFunc(ent, hnd);
	//gi.local_sound(ent, CHAN_AUTO, gi.soundindex("misc/menu1.wav"), 1, ATTN_NONE, 0);
}
