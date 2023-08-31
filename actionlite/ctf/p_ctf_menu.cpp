// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "../g_local.h"

// Note that the pmenu entries are duplicated
// this is so that a static set of pmenu entries can be used
// for multiple clients and changed without interference
// note that arg will be freed when the menu is closed, it must be allocated memory
pmenuhnd_t *PMenu_Open(edict_t *ent, const pmenu_t *entries, int cur, int num, void *arg, UpdateFunc_t UpdateFunc)
{
	pmenuhnd_t *hnd;
	const pmenu_t	*p;
	int			i;

	if (!ent->client)
		return nullptr;

	if (ent->client->menu)
	{
		gi.Com_Print("warning, ent already has a menu\n");
		PMenu_Close(ent);
	}

	hnd = (pmenuhnd_t *) gi.TagMalloc(sizeof(*hnd), TAG_LEVEL);
	hnd->UpdateFunc = UpdateFunc;

	hnd->arg = arg;
	hnd->entries = (pmenu_t *) gi.TagMalloc(sizeof(pmenu_t) * num, TAG_LEVEL);
	memcpy(hnd->entries, entries, sizeof(pmenu_t) * num);
	// duplicate the strings since they may be from static memory
	for (i = 0; i < num; i++)
		Q_strlcpy(hnd->entries[i].text, entries[i].text, sizeof(entries[i].text));

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

	ent->client->showscores = true;
	ent->client->inmenu = true;
	ent->client->menu = hnd;

	if (UpdateFunc)
		UpdateFunc(ent);

	PMenu_Do_Update(ent);
	gi.unicast(ent, true);

	return hnd;
}

void PMenu_Close(edict_t *ent)
{
	pmenuhnd_t *hnd;

	if (!ent->client->menu)
		return;

	hnd = ent->client->menu;
	gi.TagFree(hnd->entries);
	if (hnd->arg)
		gi.TagFree(hnd->arg);
	gi.TagFree(hnd);
	ent->client->menu = nullptr;
	ent->client->showscores = false;
}

// only use on pmenu's that have been called with PMenu_Open
void PMenu_UpdateEntry(pmenu_t *entry, const char *text, int align, SelectFunc_t SelectFunc)
{
	Q_strlcpy(entry->text, text, sizeof(entry->text));
	entry->align = align;
	entry->SelectFunc = SelectFunc;
}

#include "../g_statusbar.h"

void PMenu_Do_Update(edict_t *ent)
{
	int			i;
	pmenu_t	*p;
	int			x;
	pmenuhnd_t *hnd;
	const char *t;
	bool		alt = false;

	if (!ent->client->menu)
	{
		gi.Com_Print("warning:  ent has no menu\n");
		return;
	}

	hnd = ent->client->menu;

	if (hnd->UpdateFunc)
		hnd->UpdateFunc(ent);

	statusbar_t sb;

	sb.xv(32).yv(8).picn("inventory");

	for (i = 0, p = hnd->entries; i < hnd->num; i++, p++)
	{
		if (!*(p->text))
			continue; // blank line

		t = p->text;

		if (*t == '*')
		{
			alt = true;
			t++;
		}

		sb.yv(32 + i * 8);

		const char *loc_func = "loc_string";

		if (p->align == PMENU_ALIGN_CENTER)
		{
			x = 0;
			loc_func = "loc_cstring";
		}
		else if (p->align == PMENU_ALIGN_RIGHT)
		{
			x = 260;
			loc_func = "loc_rstring";
		}
		else
			x = 64;

		sb.xv(x);

		sb.sb << loc_func;

		if (hnd->cur == i || alt)
			sb.sb << '2';

		sb.sb << " 1 \"" << t << "\" \"" << p->text_arg1 << "\" ";

		if (hnd->cur == i)
		{
			sb.xv(56);
			sb.string2("\">\"");
		}

		alt = false;
	}

	gi.WriteByte(svc_layout);
	gi.WriteString(sb.sb.str().c_str());
}

void PMenu_Update(edict_t *ent)
{
	if (!ent->client->menu)
	{
		gi.Com_Print("warning:  ent has no menu\n");
		return;
	}

	if (level.time - ent->client->menutime >= 1_sec)
	{
		// been a second or more since last update, update now
		PMenu_Do_Update(ent);
		gi.unicast(ent, true);
		ent->client->menutime = level.time + 1_sec;
		ent->client->menudirty = false;
	}
	ent->client->menutime = level.time;
	ent->client->menudirty = true;
}

void PMenu_Next(edict_t *ent)
{
	pmenuhnd_t *hnd;
	int			i;
	pmenu_t	*p;

	if (!ent->client->menu)
	{
		gi.Com_Print("warning:  ent has no menu\n");
		return;
	}

	hnd = ent->client->menu;

	if (hnd->cur < 0)
		return; // no selectable entries

	i = hnd->cur;
	p = hnd->entries + hnd->cur;
	do
	{
		i++;
		p++;
		if (i == hnd->num)
		{
			i = 0;
			p = hnd->entries;
		}
		if (p->SelectFunc)
			break;
	} while (i != hnd->cur);

	hnd->cur = i;

	PMenu_Update(ent);
}

void PMenu_Prev(edict_t *ent)
{
	pmenuhnd_t *hnd;
	int			i;
	pmenu_t	*p;

	if (!ent->client->menu)
	{
		gi.Com_Print("warning:  ent has no menu\n");
		return;
	}

	hnd = ent->client->menu;

	if (hnd->cur < 0)
		return; // no selectable entries

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
		{
			i--;
			p--;
		}
		if (p->SelectFunc)
			break;
	} while (i != hnd->cur);

	hnd->cur = i;

	PMenu_Update(ent);
}

void PMenu_Select(edict_t *ent)
{
	pmenuhnd_t *hnd;
	pmenu_t	*p;

	if (!ent->client->menu)
	{
		gi.Com_Print("warning:  ent has no menu\n");
		return;
	}

	hnd = ent->client->menu;

	if (hnd->cur < 0)
		return; // no selectable entries

	p = hnd->entries + hnd->cur;

	if (p->SelectFunc)
		p->SelectFunc(ent, hnd);
}
