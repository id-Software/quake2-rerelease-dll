//-----------------------------------------------------------------------------
// a_xvote.c
//
// $Id: a_xvote.c,v 1.4 2003/12/09 22:06:11 igor_rock Exp $
//
//-----------------------------------------------------------------------------
// $Log: a_xvote.c,v $
// Revision 1.4  2003/12/09 22:06:11  igor_rock
// added "ignorepart" commadn to ignore all players with the specified part in
// their name (one shot function: if player changes his name/new palyers join,
// the list will _not_ changed!)
//
// Revision 1.3  2002/03/26 21:49:01  ra
// Bufferoverflow fixes
//
// Revision 1.2  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.1.1.1  2001/05/06 17:29:27  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#include "g_local.h"

cvar_t *_InitLeaveTeam (ini_t * ini)
{
	return teamplay;
}

vote_t xvotelist[] = {
  // mapvote
  {
   NULL,			// cvar
   _InitMapVotelist,		// InitGame 
   NULL,			// ExitGame 
   _ClearMapVotes,		// InitLevel
   _MapExitLevel,		// ExitLevel
   _MapInitClient,		// InitClient
   _RemoveVoteFromMap,		// ClientDisconnect
   _MapWithMostVotes,		// Newround
   _CheckMapVotes,		// CheckVote
   MAPMENUTITLE,		// Votetitle
   MapVoteMenu,			// VoteSelected
   }
  ,

  // kickvote
  {
   NULL,			// cvar
   _InitKickVote,		// InitGame 
   NULL,			// ExitGame 
   NULL,			// InitLevel
   NULL,			// ExitLevel
   _InitKickClient,		// InitClient
   _ClientKickDisconnect,	// ClientDisconnect
   _CheckKickVote,		// Newround
   NULL,			// CheckVote
   KICKMENUTITLE,		// Votetitle
   _KickVoteSelected,		// VoteSelected
   }
  ,

  // ignore
  {
   NULL,			// cvar
   NULL,			// InitGame - no init, default cvar = deathmatch
   NULL,			// ExitGame 
   NULL,			// InitLevel
   NULL,			// ExitLevel
   _ClearIgnoreList,		// InitClient
   _ClrIgnoresOn,		// ClientDisconnect
   NULL,			// Newround
   NULL,			// CheckVote
   IGNOREMENUTITLE,		// Votetitle
   _IgnoreVoteSelected,		// VoteSelected
   }
  ,

  // configvote
  {
   NULL,			// cvar
   _InitConfiglist,		// InitGame 
   NULL,			// ExitGame 
   NULL,			// InitLevel
   _ConfigExitLevel,		// ExitLevel
   _ConfigInitClient,		// InitClient
   _RemoveVoteFromConfig,	// ClientDisconnect
   _ConfigWithMostVotes,	// Newround
   _CheckConfigVotes,		// CheckVote
   CONFIGMENUTITLE,		// Votetitle
   ConfigVoteMenu,		// VoteSelected
   }
  ,
  // Leave Team
  {
   NULL,			// cvar
   _InitLeaveTeam,	// InitGame 
   NULL,			// ExitGame 
   NULL,			// InitLevel
   NULL,			// ExitLevel
   NULL,			// InitClient
   NULL,			// ClientDisconnect
   NULL,			// Newround
   NULL,			// CheckVote
   "Leave Team",		// Votetitle
   LeaveTeams,			// VoteSelected
   }
  ,

  // scramblevote
  {
   NULL,			// cvar
   _InitScrambleVote,		// InitGame 
   NULL,			// ExitGame 
   NULL,			// InitLevel
   NULL,			// ExitLevel
   NULL,			// InitClient
   NULL,			// ClientDisconnect
   _CheckScrambleVote,		// Newround
   NULL,			// CheckVote
   "Team Scramble",		// Votetitle
   _VoteScrambleSelected,	// VoteSelected
   }

};

static const int xvlistsize = (sizeof(xvotelist)/sizeof(vote_t));

 /**/
void _AddVoteMenu(edict_t *ent, int fromix)
{
	int i = 0, j = 0;
	vote_t *xvote;

	for (i = 0, xvote = xvotelist; i < xvlistsize; i++, xvote++)
	{
		if (xvote->VoteTitle && xvote->DependsOn->value && xvote->VoteSelected)
		{
			if (j >= fromix)
			{
				if (!xMenu_Add(ent, xvote->VoteTitle, xvote->VoteSelected))
					break;
			}
			j++;
		}
	}
}

void vShowMenu(edict_t *ent, char *menu)
{
	int i;
	char fixedmenu[128];
	vote_t *xvote;

	Q_strncpyz(fixedmenu, menu, sizeof(fixedmenu));

	if (ent->client->layout == LAYOUT_MENU) {
		PMenu_Close(ent);
		return;
	}
	if (!*fixedmenu)
	{
		// general menu
		if (xMenu_New(ent, "Menu", NULL, _AddVoteMenu) == false)
		{
			gi.cprintf(ent, PRINT_MEDIUM, "Nothing to choose.\n");
		}
		return;
	}

	for (i = 0, xvote = xvotelist; i < xvlistsize; i++, xvote++)
	{
		if (xvote->DependsOn->value && xvote->VoteSelected && !Q_stricmp(fixedmenu, xvote->VoteTitle))
		{
			xvote->VoteSelected(ent, NULL);
			return;
		}
	}
	gi.cprintf(ent, PRINT_MEDIUM, "No such menu: %s\n", fixedmenu);
}

void vInitGame(void)
{
	int i;
	vote_t *xvote;
	ini_t ini;

	ini.pfile = NULL;

	if (OpenIniFile(IniPath(), &ini) == false)
		gi.dprintf("Error opening ini file %s.\n", IniPath());

	for (i = 0, xvote = xvotelist; i < xvlistsize; i++, xvote++)
	{
		if (xvote->InitGame)
			xvote->DependsOn = xvote->InitGame(&ini);
		if (!xvote->DependsOn)
			xvote->DependsOn = deathmatch;
	}
	CloseIniFile(&ini);
}

void vExitGame(void)
{
	int i;
	vote_t *xvote;

	for (i = 0, xvote = xvotelist; i < xvlistsize; i++, xvote++)
	{
		if (xvote->ExitGame)
			xvote->ExitGame();
	}
}

void vInitLevel(void)
{
	int i;
	vote_t *xvote;

	for (i = 0, xvote = xvotelist; i < xvlistsize; i++, xvote++)
	{
		if (xvote->InitLevel && xvote->DependsOn->value)
			xvote->InitLevel();
	}
}

void vExitLevel(char *NextMap)
{
	int i;
	vote_t *xvote;

	for (i = 0, xvote = xvotelist; i < xvlistsize; i++, xvote++)
	{
		if (xvote->ExitLevel && xvote->DependsOn->value)
			xvote->ExitLevel(NextMap);
	}
}

void vInitClient(edict_t *ent)
{
	int i;
	vote_t *xvote;

	for (i = 0, xvote = xvotelist; i < xvlistsize; i++, xvote++)
	{
		if (xvote->InitClient && xvote->DependsOn->value)
			xvote->InitClient(ent);
	}
}

void vClientDisconnect(edict_t *ent)
{
	int i;
	vote_t *xvote;

	for (i = 0, xvote = xvotelist; i < xvlistsize; i++, xvote++)
	{
		if (xvote->ClientDisconnect && xvote->DependsOn->value)
			xvote->ClientDisconnect(ent);
	}
}

void vNewRound(void)
{
	int i;
	vote_t *xvote;

	for (i = 0, xvote = xvotelist; i < xvlistsize; i++, xvote++)
	{
		if (xvote->NewRound && xvote->DependsOn->value)
			xvote->NewRound();
	}
}

qboolean vCheckVote(void)
{
	int i;
	vote_t *xvote;

	for (i = 0, xvote = xvotelist; i < xvlistsize; i++, xvote++)
	{
		if (xvote->CheckVote && xvote->DependsOn->value && xvote->CheckVote() == true)
			return true;
	}
	return false;
}
