// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

enum
{
	PMENU_ALIGN_LEFT,
	PMENU_ALIGN_CENTER,
	PMENU_ALIGN_RIGHT
};

struct pmenu_t;

using UpdateFunc_t = void (*)(edict_t *ent);

struct pmenuhnd_t
{
	pmenu_t     *entries;
	int		     cur;
	int		     num;
	void	    *arg;
	UpdateFunc_t UpdateFunc;
};

using SelectFunc_t = void (*)(edict_t *ent, pmenuhnd_t *hnd);

struct pmenu_t
{
	char		 text[64];
	int			 align;
	SelectFunc_t SelectFunc;
	char         text_arg1[64];
};

pmenuhnd_t *PMenu_Open(edict_t *ent, const pmenu_t *entries, int cur, int num, void *arg, UpdateFunc_t UpdateFunc);
void		PMenu_Close(edict_t *ent);
void		PMenu_UpdateEntry(pmenu_t *entry, const char *text, int align, SelectFunc_t SelectFunc);
void		PMenu_Do_Update(edict_t *ent);
void		PMenu_Update(edict_t *ent);
void		PMenu_Next(edict_t *ent);
void		PMenu_Prev(edict_t *ent);
void		PMenu_Select(edict_t *ent);
