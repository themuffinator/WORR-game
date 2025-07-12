// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

enum {
	MENU_ALIGN_LEFT,
	MENU_ALIGN_CENTER,
	MENU_ALIGN_RIGHT
};

struct PMenu;

using MenuUpdateFunc = void (*)(Entity *ent);

struct MenuHandle {
	PMenu	*entries;
	int		cur;
	int		num;
	void	*arg;
	MenuUpdateFunc UpdateFunc;
};

using MenuSelectFunc = void (*)(Entity *ent, MenuHandle *hnd);

struct PMenu {
	char		 text[256];	// 26];	// [64];
	int			 align;
	MenuSelectFunc SelectFunc;
	char         text_arg1[64];
};

void		P_Menu_Dirty();
MenuHandle	*P_Menu_Open(Entity *ent, const PMenu *entries, int cur, int num, void *arg, MenuUpdateFunc UpdateFunc);
void		P_Menu_Close(Entity *ent);
void		P_Menu_UpdateEntry(PMenu *entry, const char *text, int align, MenuSelectFunc SelectFunc);
void		P_Menu_Do_Update(Entity *ent);
void		P_Menu_Update(Entity *ent);
void		P_Menu_Next(Entity *ent);
void		P_Menu_Prev(Entity *ent);
void		P_Menu_Select(Entity *ent);
