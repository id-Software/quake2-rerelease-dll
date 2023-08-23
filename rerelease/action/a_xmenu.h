//-----------------------------------------------------------------------------
// Menu Stuff
//
// $Id: a_xmenu.h,v 1.2 2001/09/28 13:48:34 ra Exp $
//
//-----------------------------------------------------------------------------
// $Log: a_xmenu.h,v $
// Revision 1.2  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.1.1.1  2001/05/06 17:25:35  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#define XMENU_MAX_ENTRIES 6
#define XMENU_TOTAL_ENTRIES XMENU_MAX_ENTRIES+2
#define XMENU_END_ENTRY XMENU_MAX_ENTRIES+12
#define XMENU_TITLE_MAX 30

typedef char XMENU_TITLE[XMENU_TITLE_MAX];

typedef struct
{
  XMENU_TITLE name;
  void (*SelectFunc) (edict_t * ent, pmenu_t * p);
}
XMENU_ENTRY;

typedef struct
{
  XMENU_ENTRY xmenuentries[XMENU_TOTAL_ENTRIES];
  pmenu_t themenu[XMENU_END_ENTRY];
  int xmenutop;
  int xmenucount;
  void (*DoAddMenu) (edict_t * ent, int fromix);
}
xmenu_t;

qboolean xMenu_New (edict_t * ent, char *title, char *subtitle,
		    void (*DoAddMenu) (edict_t * ent, int fromix));
qboolean xMenu_Add (edict_t * ent, char *name,
		    void (*SelectFunc) (edict_t * ent, pmenu_t * p));
