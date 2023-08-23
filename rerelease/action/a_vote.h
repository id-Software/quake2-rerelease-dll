//-----------------------------------------------------------------------------
// Voting stuff
//
// $Id: a_vote.h,v 1.3 2003/12/09 22:06:11 igor_rock Exp $
//
//-----------------------------------------------------------------------------
// $Log: a_vote.h,v $
// Revision 1.3  2003/12/09 22:06:11  igor_rock
// added "ignorepart" commadn to ignore all players with the specified part in
// their name (one shot function: if player changes his name/new palyers join,
// the list will _not_ changed!)
//
// Revision 1.2  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.1.1.1  2001/05/06 17:24:24  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

//=== map voting ===========================================================
//==========================================================================

typedef struct votelist_s
{
  char *mapname;
  int num_votes;
  int num_allvotes;		// added by Igor[Rock]

  struct votelist_s *next;
}
votelist_t;

extern votelist_t *map_votes;
extern int map_num_maps;

#define MAPMENUTITLE "Mapmenu"

void Cmd_Votemap_f(edict_t *ent);
void Cmd_Maplist_f(edict_t *ent);
void _MapInitClient(edict_t *ent);
void _RemoveVoteFromMap (edict_t *ent);
void _MapExitLevel(char *NextMap);
qboolean _CheckMapVotes(void);
void _MapWithMostVotes(void);
votelist_t *MapWithMostAllVotes(void);
void _ClearMapVotes(void);
cvar_t *_InitMapVotelist(ini_t *ini);
void MapVoteMenu(edict_t *ent, pmenu_t *p);

//=== kick voting ==========================================================
//==========================================================================

#define KICKMENUTITLE "Kickmenu"

cvar_t *_InitKickVote(ini_t *ini);
void _InitKickClient(edict_t *ent);
void _ClientKickDisconnect (edict_t *ent);
void _KickVoteSelected(edict_t *ent, pmenu_t *p);
void _CheckKickVote(void);
void Cmd_Votekick_f(edict_t *ent);
void Cmd_Votekicknum_f(edict_t *ent);
void Cmd_Kicklist_f(edict_t *ent);

//=== player ignoring ======================================================
//==========================================================================

#define IGNOREMENUTITLE "Ignoremenu"
#define PG_MAXPLAYERS 11

typedef edict_t *ignorelist_t[PG_MAXPLAYERS];

void Cmd_Ignoreclear_f(edict_t *self);
void Cmd_Ignorelist_f(edict_t *self);
void Cmd_Ignorenum_f(edict_t *self);
void Cmd_Ignore_f(edict_t *self);
void Cmd_IgnorePart_f(edict_t *self);
void _ClrIgnoresOn(edict_t *target);
int IsInIgnoreList(edict_t *source, edict_t *subject);
void _IgnoreVoteSelected(edict_t *ent, pmenu_t *p);
void _ClearIgnoreList(edict_t *ent);

//=== config voting ========================================================
//==========================================================================

#define CONFIGMENUTITLE "Configmenu"

typedef struct configlist_s
{
  char *configname;
  int num_votes;

  struct configlist_s *next;
}
configlist_t;

extern configlist_t *config_votes;
extern int config_num_configs;

void Cmd_Voteconfig_f(edict_t *ent);
void Cmd_Configlist_f(edict_t *ent);
void _ConfigInitClient(edict_t *ent);
void _RemoveVoteFromConfig(edict_t *ent);
void _ConfigExitLevel(char *NextConfig);
qboolean _CheckConfigVotes(void);
void _ConfigWithMostVotes(void);
cvar_t *_InitConfiglist (ini_t *ini);
void ConfigVoteMenu(edict_t *ent, pmenu_t *p);

//=== leave team ==========================================================
//==========================================================================

#define LEAVETEAMSTITLE "Leave Team"

void LeaveTeams(edict_t *ent, pmenu_t *p);

cvar_t *_InitScrambleVote(ini_t *ini);
void _CheckScrambleVote(void);
void _VoteScrambleSelected(edict_t *ent, pmenu_t *p);
void Cmd_Votescramble_f(edict_t *ent);
