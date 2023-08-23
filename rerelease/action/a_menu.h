//-----------------------------------------------------------------------------
// Action (formerly Axshun) menus
// From Zoid's CTF.
//
// $Id: a_menu.h,v 1.2 2001/09/28 13:48:34 ra Exp $
//
//-----------------------------------------------------------------------------
// $Log: a_menu.h,v $
// Revision 1.2  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.1.1.1  2001/05/06 17:24:26  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

enum
{
  PMENU_ALIGN_LEFT,
  PMENU_ALIGN_CENTER,
  PMENU_ALIGN_RIGHT
};

typedef struct pmenuhnd_s
{
  struct pmenu_s *entries;
  int cur;
  int num;
}
pmenuhnd_t;

typedef struct pmenu_s
{
  char *text;
  int align;
  void *arg;
  void (*SelectFunc) (edict_t * ent, struct pmenu_s * entry);
}
pmenu_t;

void PMenu_Open (edict_t * ent, pmenu_t * entries, int cur, int num);
void PMenu_Close (edict_t * ent);
void PMenu_Update (edict_t * ent);
void PMenu_Next (edict_t * ent);
void PMenu_Prev (edict_t * ent);
void PMenu_Select (edict_t * ent);
