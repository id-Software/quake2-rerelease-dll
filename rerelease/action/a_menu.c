//-----------------------------------------------------------------------------
// Action (formerly Axshun) menu code
// This is directly from Zoid's CTF code.  Thanks to Zoid again.
// -Fireblade
//
// $Id: a_menu.c,v 1.1.1.1 2001/05/06 17:24:26 igor_rock Exp $
//
//-----------------------------------------------------------------------------
// $Log: a_menu.c,v $
// Revision 1.1.1.1  2001/05/06 17:24:26  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#include "g_local.h"

void PMenu_Open (edict_t *ent, pmenu_t *entries, int cur, int num)
{
	pmenuhnd_t *hnd;
	pmenu_t *p;
	int i;

	if (!ent->client)
		return;

	if (ent->client->layout == LAYOUT_MENU) {
		gi.dprintf("warning, ent already has a menu\n");
		PMenu_Close(ent);
	}

	hnd = &ent->client->menu;

	hnd->entries = entries;
	hnd->num = num;

	if (cur < 0 || !entries[cur].SelectFunc)
	{
		for (i = 0, p = entries; i < num; i++, p++)
			if (p->SelectFunc)
				break;
	}
	else
		i = cur;

	if (i >= num)
		hnd->cur = -1;
	else
		hnd->cur = i;

	ent->client->layout = LAYOUT_MENU;

	PMenu_Update(ent);
	gi.unicast(ent, true);
}

void PMenu_Close (edict_t * ent)
{
	if (ent->client->layout != LAYOUT_MENU)
		return;

	memset(&ent->client->menu, 0, sizeof(ent->client->menu));
	ent->client->layout = LAYOUT_NONE;
}

void PMenu_Update (edict_t * ent)
{
	char string[1024];
	int i, len;
	pmenu_t *p;
	int x;
	pmenuhnd_t *hnd;
	char *t;
	qboolean alt = false;

	if (ent->client->layout != LAYOUT_MENU)
		return;

	hnd = &ent->client->menu;

	strcpy(string, "xv 32 yv 8 ");
	len = strlen(string);

	for (i = 0, p = hnd->entries; i < hnd->num; i++, p++)
	{
		if (!p->text || !*(p->text))
			continue;		// blank line

		alt = false;

		t = p->text;
		if (*t == '*')
		{
			alt = true;
			t++;
		}

		if (p->align == PMENU_ALIGN_CENTER)
			x = 196 / 2 - strlen (t) * 4 + 64;
		else if (p->align == PMENU_ALIGN_RIGHT)
			x = 64 + (196 - strlen (t) * 8);
		else
			x = 64;

		Com_sprintf (string + len, sizeof(string)-len, "yv %d xv %d ",
			32 + i * 8, x - ((hnd->cur == i) ? 8 : 0));

		len = strlen(string);

		if (hnd->cur == i)
			Com_sprintf (string + len, sizeof(string)-len, "string2 \"\x0d%s\" ", t);
		else if (alt)
			Com_sprintf (string + len, sizeof(string)-len, "string2 \"%s\" ", t);
		else
			Com_sprintf (string + len, sizeof(string)-len, "string \"%s\" ", t);

		len = strlen(string);

		if(len >= sizeof(string)-1)
			break;
	}

	gi.WriteByte(svc_layout);
	gi.WriteString(string);
}

void PMenu_Next (edict_t * ent)
{
	pmenuhnd_t *hnd;
	int i;
	pmenu_t *p;

	if (ent->client->layout != LAYOUT_MENU)
		return;

	hnd = &ent->client->menu;

	if (hnd->cur < 0)
		return;			// no selectable entries

	i = hnd->cur;
	p = hnd->entries + hnd->cur;
	do
	{
		i++, p++;
		if (i == hnd->num)
			i = 0, p = hnd->entries;
		if (p->SelectFunc)
			break;
	}
	while (i != hnd->cur);

	hnd->cur = i;

	PMenu_Update(ent);
	gi.unicast(ent, true);
}

void PMenu_Prev (edict_t * ent)
{
	pmenuhnd_t *hnd;
	int i;
	pmenu_t *p;

	if (ent->client->layout != LAYOUT_MENU)
		return;

	hnd = &ent->client->menu;

	if (hnd->cur < 0)
		return;			// no selectable entries

	i = hnd->cur;
	p = hnd->entries + hnd->cur;
	do
	{
		if (i == 0)
		{
			i = hnd->num - 1;
			p = hnd->entries + i;
		}
		else
			i--, p--;
		if (p->SelectFunc)
			break;
	}
	while (i != hnd->cur);

	hnd->cur = i;

	PMenu_Update(ent);
	gi.unicast(ent, true);
}

void PMenu_Select (edict_t * ent)
{
	pmenuhnd_t *hnd;
	pmenu_t *p;

	if (ent->client->layout != LAYOUT_MENU)
		return;

	hnd = &ent->client->menu;

	if (hnd->cur < 0)
		return;			// no selectable entries

	p = hnd->entries + hnd->cur;

	if (p->SelectFunc)
		p->SelectFunc(ent, p);
}
