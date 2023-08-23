//-----------------------------------------------------------------------------
// a_tourney.c
//
// $Id: a_tourney.c,v 1.2 2001/09/28 13:48:34 ra Exp $
//
//-----------------------------------------------------------------------------
// $Log: a_tourney.c,v $
// Revision 1.2  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.1.1.1  2001/05/06 17:24:40  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#include "g_local.h"

int NextOpponent = 2;
int LastOpponent = 0;

tourneyindex_t t_eventlist[TOURNEYMAXEVENTS];
int t_eventcount = 0;

static int _istourneysection(const char *atoken)
{
	if (Q_stricmp(atoken, "[START]") == 0)
		return 1;
	if (Q_stricmp(atoken, "[END]") == 0)
		return 2;
	if (Q_stricmp(atoken, "[SPAWN]") == 0)
		return 3;

	return 0;
}

static void _tourneyparseerror(parse_t *parse, const char *msg, const char *atoken)
{
	char buf[512];

	if (atoken)
		Com_sprintf(buf, sizeof(buf), msg, atoken);
	else
		Q_strncpyz(buf, msg, sizeof(buf));
	gi.dprintf("Error in " TOURNEYINI " at line %i: %s.\n", parse->lnumber, buf);
}

static void _tourneysetsection (tourneyindex_t * ti, int clevel)
{
  switch (clevel)
    {
    case 1:
      ti->ttime = T_START;
      break;

    case 2:
      ti->ttime = T_END;
      break;

    case 3:
      ti->ttime = T_SPAWN;
      break;

    default:
      ti->ttime = T_NONE;
    }
}

//format: set [starttime/startspawn/endtime] at [time]
static qboolean _tourneyset (parse_t parse, int clevel, int cevent)
{
  char *mytok;
  int toknr;
  tourneyindex_t ti;

  toknr = 0;
  ti.taction = A_SET;
  while (toknr < 3 && (mytok = ParseNextToken (&parse, STDSEPERATOR)) != NULL)
    {
      switch (toknr)
	{
	case 0:		//"starttime" or "roundstart" expected

	  if (Q_stricmp (mytok, "starttime") == 0)
	    {
	      toknr++;
	      _tourneysetsection (&ti, clevel);
	    }
	  else if (Q_stricmp (mytok, "roundstart") == 0)
	    {
	      if (clevel != 1)
		{
		  _tourneyparseerror(&parse,
				      "%s is only supported in section [START]",
				      mytok);
		  toknr = 1000;

		}
	      else
		{
		  ti.ttime = T_RSTART;
		  toknr++;
		}
	    }
	  else
	    {
	      _tourneyparseerror(&parse, "not supported set option %s",
				  mytok);
	      toknr = 1000;
	    }
	  break;

	case 1:
	  if (Q_stricmp (mytok, "at") == 0)
	    toknr++;
	  else
	    {
	      _tourneyparseerror(&parse, "\"at\" expected, %s found", mytok);
	      toknr = 1000;
	    }
	  break;

	case 2:
	  ti.attime = atoi (mytok);
	  toknr++;
	  break;

	}
    }
  if (toknr == 3)
    {
      //anything was ok
      memcpy (&t_eventlist[cevent], &ti, sizeof (tourneyindex_t));
      return true;
    }
  if (toknr < 3)
    {
      _tourneyparseerror(&parse, "unexpected end of file in SET", NULL);
    }
  return false;
}

//format: play [sound] at [time]
qboolean _tourneyplay (parse_t parse, int clevel, int cevent)
{
  char *mytok;
  int toknr;
  tourneyindex_t ti;

  toknr = 0;
  ti.taction = A_PLAY;
  _tourneysetsection (&ti, clevel);

  while (toknr < 3 && (mytok = ParseNextToken (&parse, STDSEPERATOR)) != NULL)
    {
      switch (toknr)
	{
	case 0:
	  strcpy (ti.info, mytok);
	  toknr++;
	  break;

	case 1:
	  if (Q_stricmp (mytok, "at") == 0)
	    toknr++;
	  else
	    {
	      _tourneyparseerror(&parse, "\"at\" expected, %s found", mytok);
	      toknr = 1000;
	    }
	  break;

	case 2:
	  ti.attime = atoi (mytok);
	  toknr++;
	  break;
	}
    }
  if (toknr == 3)
    {
      //anything was ok
      memcpy (&t_eventlist[cevent], &ti, sizeof (tourneyindex_t));
      return true;
    }
  if (toknr < 3)
    {
      _tourneyparseerror(&parse, "unexpected end of file in PLAY", NULL);
    }
  return false;
}

//format: print ["text"] at [time]
qboolean _tourneyprint (parse_t parse, int clevel, int cevent)
{
  char *mytok;
  int toknr;
  tourneyindex_t ti;

  toknr = 0;
  ti.taction = A_PRINT;
  _tourneysetsection (&ti, clevel);
  //
  mytok = ParseNextToken (&parse, "\"");
  strcpy (ti.info, mytok);
  strcat (ti.info, "\n");

  while (toknr < 2 && (mytok = ParseNextToken (&parse, STDSEPERATOR)) != NULL)
    {
      switch (toknr)
	{
	case 0:
	  if (Q_stricmp (mytok, "at") == 0)
	    toknr++;
	  else
	    {
	      _tourneyparseerror(&parse, "\"at\" expected, %s found", mytok);
	      toknr = 1000;
	    }
	  break;

	case 1:
	  ti.attime = atoi (mytok);
	  toknr++;
	  break;
	}
    }
  if (toknr == 2)
    {
      //anything was ok
      memcpy (&t_eventlist[cevent], &ti, sizeof (tourneyindex_t));
      return true;
    }
  if (toknr < 2)
    {
      _tourneyparseerror(&parse, "unexpected end of file in PRINT", NULL);
    }
  return false;
}


//
void
TourneyTimeEvent (TOURNEYTIME ttime, int attime)
{
  int i;

  for (i = 0; i < t_eventcount; i++)
    {
      if (t_eventlist[i].ttime == ttime && t_eventlist[i].attime == attime)
	{
	  if (t_eventlist[i].taction == A_PLAY)
	    {
	      gi.sound (&g_edicts[0], CHAN_VOICE | CHAN_NO_PHS_ADD,
			gi.soundindex (t_eventlist[i].info), 1.0, ATTN_NONE,
			0.0);
	    }
	  else if (t_eventlist[i].taction == A_PRINT)
	    {
	      CenterPrintAll (t_eventlist[i].info);
	    }
/*     
   else
   {
   if (t_eventlist[i].taction == A_SET)
   }
 */
	}
    }
}

//
int
TourneySetTime (TOURNEYTIME ttime)
{
  int i, j;

  j = -1;
  for (i = 0; i < t_eventcount; i++)
    {
      if (t_eventlist[i].ttime == ttime && t_eventlist[i].taction == A_SET)
	{
	  j = t_eventlist[i].attime;
	  break;
	}
    }
  if (j <= 0)
    j = 71;
  return j;
}


void
TourneyReadIni (void)
{
  parse_t parse;
  int clevel = 0;
  char *mytok = NULL;
  //qboolean inevent = false;  // FIXME: This was set but never used.

  t_eventcount = 0;
  //inevent = false;  // FIXME: This was never used.
  if (ParseStartFile (GAMEVERSION "/" TOURNEYINI, &parse) == true)
    {
      while ((mytok = ParseNextToken (&parse, STDSEPERATOR)) != NULL)
	{
	  switch (clevel)
	    {
	    case 0:		//we're not in any section

	      clevel = _istourneysection (mytok);
	      if (!clevel)
		_tourneyparseerror(&parse, "unknown command %s", mytok);
	      break;

	    default:		// we're are in any other section


	      if (Q_stricmp (mytok, "set") == 0)
		{
		  if (_tourneyset (parse, clevel, t_eventcount) == true)
		    t_eventcount++;
		}
	      else if (Q_stricmp (mytok, "play") == 0)
		{
		  if (_tourneyplay (parse, clevel, t_eventcount) == true)
		    t_eventcount++;
		}
	      else if (Q_stricmp (mytok, "print") == 0)
		{
		  if (_tourneyprint (parse, clevel, t_eventcount) == true)
		    t_eventcount++;
		}
	      else if (_istourneysection (mytok))
		clevel = _istourneysection (mytok);
	      else
		_tourneyparseerror(&parse, "unknown command %s", mytok);
	    }
	}
      ParseEndFile (&parse);
    }
  gi.dprintf ("%i tourney events...\n", t_eventcount);
}



//Inits TourneyMode
void
TourneyInit (void)
{
  NextOpponent = 2;
  LastOpponent = 0;
  TourneyReadIni ();
  if (use_tourney->value)
    gi.dprintf ("Tourney mode enabled.\n");
}

//gives a new connected player a new Tourney number.
//The first Tourney number is 1.
void
TourneyNewPlayer (edict_t * player)
{
  player->client->resp.tourneynumber = ++LastOpponent;
//  gi.cprintf(player, PRINT_HIGH,"Player %s, number %i\n", player->client->pers.netname,
  //             player->client->resp.tourneynumber);
  //  gi.dprintf("Player %s, number %i\n", player->client->pers.netname,
  //             player->client->resp.tourneynumber);
}

//returns the player with Tourney number "number".
//if "number" is not found, it returns NULL
edict_t *
TourneyFindPlayer (int number)
{
  edict_t *player;
  int i;

  for (i = 1; i <= game.maxclients; i++)
    {
      player = g_edicts + i;
      if (player->inuse)
	{
//      gi.dprintf(">Player %s, number %i\n", player->client->pers.netname,
	  //             player->client->resp.tourneynumber);

	  if (player->client->resp.tourneynumber == number)
	    return (player);
	}
    }

  gi.dprintf ("Tourney Error: Player not found (%i)\n", number);
  return NULL;
}

//removes number of player 
void
TourneyRemovePlayer (edict_t * player)
{
  edict_t *dummy;
  int i;

  LastOpponent--;
  for (i = 1; i <= game.maxclients; i++)
    {
      dummy = g_edicts + i;
      if (dummy->inuse)
	{
	  if (dummy->client->resp.tourneynumber >
	      player->client->resp.tourneynumber)
	    dummy->client->resp.tourneynumber--;
	}
    }
  if (NextOpponent == player->client->resp.tourneynumber)
    NextOpponent--;
}

//sets winner to #1 and determs next opponent
void
TourneyWinner (edict_t * player)
{
  edict_t *oldnumberone, *dummy;

  dummy = TourneyFindPlayer (1);
  if (dummy)
    dummy->client->resp.team = NOTEAM;
  dummy = TourneyFindPlayer (NextOpponent);
  if (dummy)
    dummy->client->resp.team = NOTEAM;

  if (player->client->resp.tourneynumber != 1)
    {
      //new winner, so we have to cycle the numbers    
      oldnumberone = TourneyFindPlayer (1);
      oldnumberone->client->resp.tourneynumber =
	player->client->resp.tourneynumber;
      player->client->resp.tourneynumber = 1;
    }

  NextOpponent++;
  if (NextOpponent > LastOpponent || NextOpponent < 2)
    NextOpponent = 2;
}

//sets new round, returns wether enough players are available
//or not
qboolean TourneyNewRound (void)
{
	char buf[128];
	edict_t *dummy;

	//  gi.bprintf(PRINT_HIGH,"LastOpponent: %i\n", LastOpponent);        
	if (LastOpponent < 2)
		return (false);
	strcpy (buf, "Next game: ");
	dummy = TourneyFindPlayer (1);
	if (dummy)
	{
		dummy->client->resp.team = TEAM1;
		strcat (buf, dummy->client->pers.netname);
	}
	else
	{
		gi.dprintf ("Tourney Error: cannot find player #1!\n");
		strcat (buf, "(unknown)");
	}

	strcat (buf, " vs ");

	dummy = TourneyFindPlayer (NextOpponent);
	if (dummy)
	{
		dummy->client->resp.team = TEAM2;
		strcat (buf, dummy->client->pers.netname);
	}
	else
	{
		gi.dprintf ("Tourney Error: cannot find NextOpponent (%i)!\n",
			NextOpponent);
		strcat (buf, "(unknown)");
	}

	CenterPrintAll (buf);
	return (true);
}
