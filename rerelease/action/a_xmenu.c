//-----------------------------------------------------------------------------
// a_xmenu.c
//
// $Id: a_xmenu.c,v 1.2 2001/09/28 13:48:34 ra Exp $
//
//-----------------------------------------------------------------------------
// $Log: a_xmenu.c,v $
// Revision 1.2  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.1.1.1  2001/05/06 17:25:35  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#include "g_local.h"

static XMENU_TITLE xRaw[] = {
  "\x80\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x82",  // double line
  "previous page",
  "next page",
  "Use [ and ] to move cursor",
  "ENTER to select, TAB to exit",
  "\x1D\x1E\x1E\x1E\x1E\x1E\x1E\x1E\x1E\x1E\x1E\x1E\x1E\x1E\x1E\x1E\x1E\x1E\x1E\x1E\x1E\x1E\x1E\x1E\x1E\x1E\x1E\x1E\x1F",  // single line
  "\x80\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x81\x82"  // double line
};

qboolean xMenu_Add (edict_t * ent, char *name,
	   void (*SelectFunc) (edict_t * ent, pmenu_t * p))
{
	xmenu_t *x_menu = &ent->client->pers.x_menu;

	if (x_menu->xmenucount < XMENU_TOTAL_ENTRIES)
	{
		if (name)
			Q_strncpyz(x_menu->xmenuentries[x_menu->xmenucount].name, name, XMENU_TITLE_MAX);
		else
			x_menu->xmenuentries[x_menu->xmenucount].name[0] = '\0';
		x_menu->xmenuentries[x_menu->xmenucount].SelectFunc = SelectFunc;
		x_menu->xmenucount++;
	}

	return (x_menu->xmenucount < XMENU_TOTAL_ENTRIES);
}


void xMenu_Set (edict_t * ent);

void xMenu_Next (edict_t * ent, pmenu_t * p)
{
	xmenu_t *x_menu = &ent->client->pers.x_menu;

	x_menu->xmenucount = 2;
	x_menu->DoAddMenu(ent, x_menu->xmenutop + XMENU_MAX_ENTRIES);
	if (x_menu->xmenucount > 2)
	{
		x_menu->xmenutop += XMENU_MAX_ENTRIES;
		xMenu_Set(ent);

		PMenu_Close(ent);
		PMenu_Open(ent, x_menu->themenu, 5, XMENU_END_ENTRY);
		//PMenu_Update(ent);
	}
}

void xMenu_Prev (edict_t * ent, pmenu_t * p)
{
	xmenu_t *x_menu = &ent->client->pers.x_menu;
	int temptop;

	x_menu->xmenucount = 2;
	if (x_menu->xmenutop < XMENU_MAX_ENTRIES)
		temptop = 0;
	else
		temptop = x_menu->xmenutop - XMENU_MAX_ENTRIES;
	x_menu->DoAddMenu(ent, temptop);
	if (x_menu->xmenucount > 2)
	{
		x_menu->xmenutop = temptop;
		xMenu_Set(ent);

		PMenu_Close(ent);
		PMenu_Open(ent, x_menu->themenu, 5, XMENU_END_ENTRY);
		//PMenu_Update(ent);
	}
}

void xMenu_Set (edict_t * ent)
{
	xmenu_t *x_menu = &ent->client->pers.x_menu;
	int i;

	//setting title
	x_menu->themenu[0].text = x_menu->xmenuentries[0].name;
	x_menu->themenu[0].align = PMENU_ALIGN_CENTER;
	x_menu->themenu[1].text = NULL;
	x_menu->themenu[1].align = PMENU_ALIGN_CENTER;
	x_menu->themenu[2].text = x_menu->xmenuentries[1].name;
	x_menu->themenu[2].align = PMENU_ALIGN_CENTER;
	x_menu->themenu[3].text = xRaw[5];
	x_menu->themenu[3].align = PMENU_ALIGN_CENTER;

	//setting bottom
	x_menu->themenu[XMENU_END_ENTRY - 3].text = xRaw[5];
	x_menu->themenu[XMENU_END_ENTRY - 3].align = PMENU_ALIGN_CENTER;
	x_menu->themenu[XMENU_END_ENTRY - 2].text = xRaw[3];
	x_menu->themenu[XMENU_END_ENTRY - 2].align = PMENU_ALIGN_CENTER;
	x_menu->themenu[XMENU_END_ENTRY - 1].text = xRaw[4];
	x_menu->themenu[XMENU_END_ENTRY - 1].align = PMENU_ALIGN_CENTER;

	for (i = 2; i < x_menu->xmenucount; i++)
	{
		x_menu->themenu[i + 3].text = x_menu->xmenuentries[i].name;
		x_menu->themenu[i + 3].SelectFunc = x_menu->xmenuentries[i].SelectFunc;
	}
	while (i < XMENU_TOTAL_ENTRIES)
	{
		x_menu->themenu[i + 3].text = NULL;
		x_menu->themenu[i + 3].SelectFunc = NULL;
		i++;
	}

        x_menu->themenu[XMENU_END_ENTRY - 5].text = NULL;
        x_menu->themenu[XMENU_END_ENTRY - 5].SelectFunc = NULL;
	if (x_menu->xmenucount == XMENU_TOTAL_ENTRIES)
	{
		// The page is full.
		XMENU_ENTRY save_entry;
		memcpy( &save_entry, &(x_menu->xmenuentries[ XMENU_TOTAL_ENTRIES - 1 ]), sizeof(XMENU_ENTRY) );
		x_menu->xmenucount --;
		x_menu->DoAddMenu( ent, x_menu->xmenutop + XMENU_MAX_ENTRIES );
		if( x_menu->xmenucount == XMENU_TOTAL_ENTRIES )
		{
			// There are more choices after this page, so next must be enabled.
			x_menu->themenu[XMENU_END_ENTRY - 5].text = xRaw[2];
			x_menu->themenu[XMENU_END_ENTRY - 5].SelectFunc = xMenu_Next;
		}
		memcpy( &(x_menu->xmenuentries[ XMENU_TOTAL_ENTRIES - 1 ]), &save_entry, sizeof(XMENU_ENTRY) );
		x_menu->xmenucount = XMENU_TOTAL_ENTRIES;
	}

	if (x_menu->xmenutop)
	{
		//prev must be enabled
		x_menu->themenu[XMENU_END_ENTRY - 6].text = xRaw[1];
		x_menu->themenu[XMENU_END_ENTRY - 6].SelectFunc = xMenu_Prev;
	}
	else
	{
		x_menu->themenu[XMENU_END_ENTRY - 6].text = NULL;
		x_menu->themenu[XMENU_END_ENTRY - 6].SelectFunc = NULL;
	}
}

qboolean
xMenu_New (edict_t * ent, char *title, char *subtitle,
	   void (*DoAddMenu) (edict_t * ent, int fromix))
{
	xmenu_t *x_menu = &ent->client->pers.x_menu;

	if (!DoAddMenu)
		return false;

	x_menu->DoAddMenu = DoAddMenu;
	x_menu->xmenucount = 2;
	x_menu->xmenutop = 0;
	//memset(xmenuentries, 0, sizeof(xmenuentries));
	strcpy(x_menu->xmenuentries[0].name, "*");
	if (title)
		Q_strncatz(x_menu->xmenuentries[0].name, title, XMENU_TITLE_MAX);
	else
		Q_strncatz(x_menu->xmenuentries[0].name, "Menu", XMENU_TITLE_MAX);
	if (subtitle)
		Q_strncpyz(x_menu->xmenuentries[1].name, subtitle, XMENU_TITLE_MAX);
	else
		Q_strncpyz(x_menu->xmenuentries[1].name, "make your choice", XMENU_TITLE_MAX);

	x_menu->xmenuentries[0].SelectFunc = NULL;
	x_menu->xmenuentries[1].SelectFunc = NULL;

	DoAddMenu (ent, 0);
	if (x_menu->xmenucount > 2)
	{
		xMenu_Set(ent);

		if (ent->client->layout == LAYOUT_MENU)
			PMenu_Close(ent);

		PMenu_Open(ent, x_menu->themenu, 5, XMENU_END_ENTRY);
		return true;
	}

	return false;
}
