//-----------------------------------------------------------------------------
// Matchmode related definitions
//
// $Id: a_match.h,v 1.7 2002/03/28 11:46:03 freud Exp $
//
//-----------------------------------------------------------------------------
// $Log: a_match.h,v $
// Revision 1.7  2002/03/28 11:46:03  freud
// stat_mode 2 and timelimit 0 did not show stats at end of round.
// Added lock/unlock.
// A fix for use_oldspawns 1, crash bug.
//
// Revision 1.6  2001/11/25 19:09:25  slicerdw
// Fixed Matchtime
//
// Revision 1.5  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.4  2001/06/13 07:55:17  igor_rock
// Re-Added a_match.h and a_match.c
// Added CTF Header for a_ctf.h and a_ctf.c
//
//-----------------------------------------------------------------------------

#define IS_CAPTAIN(ent) (teams[(ent)->client->resp.team].captain == (ent))
#define	HAVE_CAPTAIN(teamNum) (teams[(teamNum)].captain)

void SendScores (void);
qboolean TeamsReady( void );
void MM_LeftTeam( edict_t * ent );
void Cmd_Captain_f (edict_t * ent);
void Cmd_Ready_f (edict_t * ent);
void Cmd_Sub_f (edict_t * ent);
void Cmd_Teamname_f (edict_t * ent);
void Cmd_Teamskin_f (edict_t * ent);
void Cmd_TeamLock_f (edict_t * ent, int a_switch);
int CheckForCaptains (int cteam);

void Cmd_SetAdmin_f (edict_t * ent);
void Cmd_TogglePause_f(edict_t * ent, qboolean pause);
void Cmd_ResetScores_f(edict_t * ent);
