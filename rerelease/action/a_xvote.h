//-----------------------------------------------------------------------------
// Voting Stuff
//
// $Id: a_xvote.h,v 1.2 2001/09/28 13:48:34 ra Exp $
//
//-----------------------------------------------------------------------------
// $Log: a_xvote.h,v $
// Revision 1.2  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.1.1.1  2001/05/06 17:29:27  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

typedef struct
{
  cvar_t *DependsOn;		// if cvar = 1, vote is enabled
  // called at game initialization, DependsOn is set on result, if result is NULL
  // DependsOn is set on Deathmatch. ini is an open inifile, you may read any
  // wanted value due ReadIniStr() or ReadIniInt().

  cvar_t *(*InitGame) (ini_t * ini);
  void (*ExitGame) (void);	// called at game exit

  void (*InitLevel) (void);	// called at level initialization

  void (*ExitLevel) (char *NextMap);	// when Nextmap is set, this will be the next map

  void (*InitClient) (edict_t * ent);	// called at ClientBegin (client initialization)
  // called at ClientConnect, if false, connection is refused.

  void (*ClientDisconnect) (edict_t * ent);	// called at ClientDisconnect

  void (*NewRound) (void);	// called when round ends (teamplay)

    qboolean (*CheckVote) (void);	// if true, we'll leave level...

  char *VoteTitle;		// votemenu menuentry
  // called if vote was selected in votemenu

  void (*VoteSelected) (edict_t * ent, pmenu_t * p);
} vote_t;

//extern vote_t *xvotelist[];

void vInitGame(void);
void vExitGame(void);
void vInitLevel(void);
void vExitLevel(char *NextMap);
void vInitClient(edict_t *ent);
void vClientDisconnect(edict_t *ent);
void vNewRound(void);
qboolean vCheckVote(void);
void vShowMenu(edict_t *ent, char *menu);
