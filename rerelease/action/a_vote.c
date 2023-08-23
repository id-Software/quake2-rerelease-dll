//-----------------------------------------------------------------------------
// a_vote.c
//
// $Id: a_vote.c,v 1.14 2003/12/09 22:06:11 igor_rock Exp $
//
//-----------------------------------------------------------------------------
// $Log: a_vote.c,v $
// Revision 1.14  2003/12/09 22:06:11  igor_rock
// added "ignorepart" commadn to ignore all players with the specified part in
// their name (one shot function: if player changes his name/new palyers join,
// the list will _not_ changed!)
//
// Revision 1.13  2003/10/01 19:39:08  igor_rock
// corrected underflow bugs (thanks to nopcode for bug report)
//
// Revision 1.12  2001/11/27 20:36:32  igor_rock
// corrected the mapvoting spamm protection
//
// Revision 1.11  2001/11/08 11:01:10  igor_rock
// finally got this damn configvote bug - configvote is OK now! :-)
//
// Revision 1.10  2001/11/03 17:21:57  deathwatch
// Fixed something in the time command, removed the .. message from the voice command, fixed the vote spamming with mapvote, removed addpoint command (old pb command that wasnt being used). Some cleaning up of the source at a few points.
//
// Revision 1.9  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.8  2001/07/25 23:02:02  slicerdw
// Fixed the source, added the weapons and items capping to choose command
//
// Revision 1.7  2001/07/16 18:28:46  ra
// Changed a 40 second hard limit on mapvoting into a cvar.
//
// Revision 1.6  2001/06/20 07:21:21  igor_rock
// added use_warnings to enable/disable time/frags left msgs
// added use_rewards to enable/disable eimpressive, excellent and accuracy msgs
// change the configfile prefix for modes to "mode_" instead "../mode-" because
// they don't have to be in the q2 dir for doewnload protection (action dir is sufficient)
// and the "-" is bad in filenames because of linux command line parameters start with "-"
//
// Revision 1.5  2001/06/18 11:01:42  igor_rock
// added "mode-" prefix to votet configfiles, so all mode configs are close together
// when someone makes a "dir" or "ls -al" command on the server (cosmetic change)
//
// Revision 1.4  2001/06/13 09:14:23  igor_rock
// change the path for configs from "config/" to "../" because of possibility
// of exploit with the "download" command if "allow_download" is set
//
// Revision 1.3  2001/06/13 08:39:13  igor_rock
// changed "cvote" to "use_cvote" (like the other votecvars)
//
// Revision 1.2  2001/05/12 20:58:22  ra
//
//
// Adding public mapvoting and kickvoting. Its controlable via cvar's mv_public
// and vk_public (both default off)
//
// Revision 1.1.1.1  2001/05/06 17:25:12  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#include "g_local.h"

#ifndef MAX_STR_LEN
#define MAX_STR_LEN 1000
#endif // MAX_STR_LEN


//=== misc functions =======================================================
//
//==========================================================================

void _printplayerlist (edict_t * self, char *buf,
		  qboolean (*markthis) (edict_t * self, edict_t * other))
{
	int count = 0, i;
	edict_t *other;
	char dummy, tmpbuf[32];

	Q_strncatz (buf, " #  Name\n", MAX_STRING_CHARS);
	Q_strncatz (buf, "------------------------------------\n", MAX_STRING_CHARS);
	for (i = 0, other = g_edicts + 1; i < game.maxclients; i++, other++)
	{
		if (!other->inuse || !other->client || other->client->pers.mvdspec)
			continue;

		if (other == self)
			continue;

		if (markthis (self, other) == true)
			dummy = '*';
		else
			dummy = ' ';
		sprintf (tmpbuf, "%2i %c%s\n", i, dummy, other->client->pers.netname);
		count++;

		Q_strncatz (buf, tmpbuf, MAX_STRING_CHARS);
	}
	if (!count)
		Q_strncatz (buf, "None\n", MAX_STRING_CHARS);
	Q_strncatz (buf, "\n", MAX_STRING_CHARS);
}


int _numclients (void)
{
	int count, i;
	edict_t *other;

	count = 0;
	for (i = 0, other = g_edicts + 1; i < game.maxclients; i++, other++)
	{
		if (!other->inuse || !other->client || !other->client->pers.connected || other->client->pers.mvdspec)
			continue;
		count++;
	}
	return count;
}

/*
Kicks the given (client) edict out of the server, reason will be printed before
*/
static void KickClient(edict_t * target, char *reason)
{
	if (target && target->client && target->inuse) {
		gi.bprintf(PRINT_HIGH, "%s has to be KICKED from the server.\n", target->client->pers.netname);
		gi.bprintf(PRINT_MEDIUM, "Reason: %s\n", reason);
		Kick_Client(target);
	}
}

//=== map voting ===========================================================
//
// Original programed by Black Cross[NL], adapted, major changes.
//
//==========================================================================

votelist_t *map_votes;
int map_num_maps;
int map_num_votes;
int map_num_clients;
qboolean map_need_to_check_votes;

cvar_t *mapvote_min;
cvar_t *mapvote_need;
cvar_t *mapvote_pass;
cvar_t *mapvote_next = NULL;

//Igor[Rock] BEGIN
// moved here from the func ReadMapListFile because I need it global
char maplistpath[MAX_STR_LEN];
//Igor[Rock] END

#define MAPVOTESECTION "mapvote"

// forward declarations
votelist_t *MapWithMostVotes(float *p);
votelist_t *MapWithMostAllVotes(void);
int AddVoteToMap(const char *mapname, edict_t * ent);
void ReadMaplistFile(void);
qboolean _iCheckMapVotes(void);

//
static void Votemap(edict_t *ent, const char *mapname)
{
	char *oldvote;
	int voteWaitTime;
	int gametime = 0;
	int remaining = 0;

	gametime = level.matchTime;
	remaining = (timelimit->value * 60) - gametime;

	if (!use_mapvote->value) {
		gi.cprintf(ent, PRINT_HIGH, "Map voting is disabled.\n");
		return;
	}

	// If timelimit is set and if mapvote_next is set, and the remaining time is less than the mapvote_next_limit, do not allow the mapvote
	if (timelimit->value && mapvote_next->value && remaining < mapvote_next_limit->value){
		gi.cprintf(ent, PRINT_HIGH, "It is too late to vote for the next map.\n");
		return;
	}

	if (!*mapname) {
		MapVoteMenu( ent, NULL );
		return;
	}

	if (level.intermission_framenum) {
		gi.cprintf(ent, PRINT_HIGH, "Mapvote disabled during intermission\n");
		return;
	}

	voteWaitTime = (int)(mapvote_waittime->value * HZ);
	if (level.realFramenum < voteWaitTime)
	{
		gi.cprintf(ent, PRINT_HIGH, "Mapvote currently blocked - Please vote again in %d seconds\n",
			(voteWaitTime + HZ - level.realFramenum) / HZ );
		return;
	}

	oldvote = ent->client->resp.mapvote;

	switch (AddVoteToMap(mapname, ent))
	{
	case 0:
		gi.cprintf(ent, PRINT_HIGH, "You have voted on map \"%s\"\n", mapname);
		if (mv_public->value)
			gi.bprintf(PRINT_HIGH, "%s voted for \"%s\"\n", ent->client->pers.netname, mapname);
	break;
	case 1:
		gi.cprintf(ent, PRINT_HIGH, "You have changed your vote to map \"%s\"\n", mapname);
		if (mv_public->value && !FloodCheck(ent)) {
			if (Q_stricmp(mapname, oldvote))
				gi.bprintf(PRINT_HIGH, "%s changed his mind and voted for \"%s\"\n", ent->client->pers.netname, mapname);
			else
				gi.cprintf(ent, PRINT_HIGH, "We heard you the first time!\n");
		}
		break;
	default:
		//error
		gi.cprintf(ent, PRINT_HIGH, "Map \"%s\" is not in the votelist!\n", mapname);
		break;
	}
}

void Cmd_Votemap_f(edict_t *ent) {
	Votemap(ent, gi.args());
}

void Cmd_Maplist_f(edict_t *ent)
{
	//go through the votelist and list out all the maps and % votes
	int lines, chars_on_line, len_mr;
	float p_test = 0.0f, p_most = 0.0f;
	votelist_t *search, *most;
	char msg_buf[MAX_STRING_CHARS], tmp_buf[128];	//only 40 are used

	if (!use_mapvote->value) {
		gi.cprintf(ent, PRINT_HIGH, "Map voting is disabled.\n");
		return;
	}

	most = MapWithMostVotes (&p_most);

	sprintf (msg_buf,
		"List of maps that can be voted on:\nRequire more than %d%% votes (%.2f)\n\n",
		(int)mapvote_pass->value, mapvote_pass->value / 100.0f);

	lines = chars_on_line = 0;
	for (search = map_votes; search != NULL; search = search->next)
	{

		if (map_num_clients > 0)
			p_test = (float) ((float) search->num_votes / (float) map_num_clients);

		if (p_test >= 10.0)
			len_mr = 11;
		else
			len_mr = 10;

		len_mr += strlen (search->mapname);
		//Igor[Rock] begin
		//    if (num_allvotes && vrot->value)
		//       len_mr += 4;
		//Igor[Rock] end
		if ((chars_on_line + len_mr + 2) > 39)
		{
			Q_strncatz (msg_buf, "\n", sizeof(msg_buf));
			lines++;
			chars_on_line = 0;
			if (lines > 25)
				break;
		}

		Com_sprintf (tmp_buf, sizeof(tmp_buf), "%s (%.2f)  ", search->mapname, p_test);
		Q_strncatz(msg_buf, tmp_buf, sizeof(msg_buf));
		chars_on_line += len_mr;
	}

	if (map_votes == NULL)
		Q_strncatz(msg_buf, "None!", sizeof(msg_buf));
	else if (most != NULL)
	{
		Com_sprintf (tmp_buf, sizeof(tmp_buf), "\n\nMost votes: %s (%.2f)", most->mapname, p_most);
		Q_strncatz(msg_buf, tmp_buf, sizeof(msg_buf));
	}

	Q_strncatz(msg_buf, "\n\n", sizeof(msg_buf));
	Com_sprintf (tmp_buf, sizeof(tmp_buf),
		"%d/%d (%.2f%%) clients voted\n%d client%s minimum (%d%% required)",
		map_num_votes, map_num_clients,
		(float)( (float)map_num_votes /
		(float)(map_num_clients > 0 ? map_num_clients : 1) * 100.0f),	// TempFile changed to percentual display
		(int)mapvote_min->value, (mapvote_min->value > 1 ? "s" : ""),
		(int)mapvote_need->value);

	Q_strncatz(msg_buf, tmp_buf, sizeof(msg_buf));

	gi.centerprintf (ent, "%s", msg_buf);

	return;
}

//
void _MapInitClient (edict_t * ent)
{
	ent->client->resp.mapvote = NULL;
}

//
void _RemoveVoteFromMap (edict_t * ent)
{
	votelist_t *search;

	map_need_to_check_votes = true;

	if (ent->client->resp.mapvote == NULL)
		return;

	for (search = map_votes; search != NULL; search = search->next)
	{
		if (Q_stricmp (search->mapname, ent->client->resp.mapvote) == 0)
		{
			map_num_votes--;
			search->num_votes--;
			ent->client->resp.mapvote = NULL;
			break;
		}
	}

	return;
}

//
void _MapExitLevel (char *NextMap)
{
	votelist_t *votemap = NULL;
	//Igor[Rock] BEGIN
	//FILE *votefile;
	//char buf[MAX_STR_LEN];
	//Igor[Rock] END

	// If mapvote_next=1 and level has ended, ignore minimums required for mapvote.
	if( _iCheckMapVotes() || ((map_num_votes > 0) && mapvote_next && mapvote_next->value) )
	{
		votemap = MapWithMostVotes (NULL);
		Q_strncpyz (NextMap, votemap->mapname, MAX_QPATH);
		gi.bprintf (PRINT_HIGH, "Next map was voted on and is %s.\n", NextMap);
	}

	//clear stats
	for (votemap = map_votes; votemap != NULL; votemap = votemap->next)
	{
		//Igor[Rock] BEGIN
		if (votemap->num_votes)
		{
			votemap->num_allvotes += votemap->num_votes;
			num_allvotes += votemap->num_votes;
		}
		if (Q_stricmp (level.mapname, votemap->mapname) == 0)
		{
			num_allvotes -= votemap->num_allvotes;
			votemap->num_allvotes = 0;
			/*
			if (map_num_clients > 1)
			{
				if (votemap->num_allvotes < (map_num_clients / 2))
				{
					num_allvotes -= votemap->num_allvotes;
					votemap->num_allvotes = 0;
				}
				else
				{
					num_allvotes -= (map_num_clients / 2);
					votemap->num_allvotes -= (map_num_clients / 2);
				}
			}
			else
			{
				if (votemap->num_allvotes)
				{
					num_allvotes--;
					votemap->num_allvotes--;
				}
			}
			*/
		}
		//Igor[Rock] END
		votemap->num_votes = 0;
	}

	/*
	//Igor[Rock] BEGIN
	// Save the actual votes to a file
	votefile = fopen (maplistpath, "w");
	if (votefile != NULL)
	{
		sprintf (buf, "%d\n", num_allvotes);
		fputs (buf, votefile);

		for (votemap = map_votes; votemap != NULL; votemap = votemap->next)
		{
			sprintf (buf, "%s,%d\n", votemap->mapname, votemap->num_allvotes);
			fputs (buf, votefile);
		}

		fclose (votefile);
	}
	*/

	//Igor[Rock] END
	map_num_votes = 0;
	map_num_clients = 0;
	map_need_to_check_votes = true;
}

//
qboolean _CheckMapVotes (void)
{
	if (use_mapvote->value == 2 && !matchmode->value) //Change to voted map only at mapchange
		return false;

	if (_iCheckMapVotes() == true)
	{
		gi.bprintf (PRINT_HIGH, "More than %i%% map votes reached.\n", (int)mapvote_pass->value);
		return true;
	}
	return false;
}

//
qboolean _MostVotesStr (char *buf)
{
	float p_most = 0.0f;
	votelist_t *most;

	most = MapWithMostVotes(&p_most);
	if (most != NULL)
	{
		sprintf (buf, "%s (%.2f%%)", most->mapname, p_most * 100.0f);
		return true;
	}
	else
		strcpy (buf, "(no map)");

	return false;
}

//
void _MapWithMostVotes (void)
{
	char buf[1024], sbuf[512];

	if (_MostVotesStr(sbuf))
	{
		Com_sprintf(buf, sizeof(buf), "Most wanted map: %s", sbuf);
		G_HighlightStr(buf, buf, sizeof(buf));
		gi.bprintf(PRINT_HIGH, "%s\n", buf);
	}
}

//
void _ClearMapVotes (void)
{
	votelist_t *search = NULL;

	map_num_votes = 0;
	map_num_clients = 0;
	map_need_to_check_votes = true;

	for( search = map_votes; search != NULL; search = search->next )
	{
		search->num_votes = 0;
		if( Q_stricmp( level.mapname, search->mapname ) == 0 )
		{
			num_allvotes -= search->num_allvotes;
			search->num_allvotes = 0;
		}
	}

	level.nextmap[0] = '\0';
}

//
cvar_t *_InitMapVotelist (ini_t * ini)
{
	char buf[1024];

	// note that this is done whether we have set "use_mapvote" or not!
	map_votes = NULL;
	map_num_maps = 0;
	num_allvotes = 0;
	_ClearMapVotes();
	ReadMaplistFile ();

	mapvote_min = gi.cvar ("mapvote_min",
		ReadIniStr (ini, MAPVOTESECTION, "mapvote_min", buf, "1"), CVAR_LATCH);
	mapvote_need = gi.cvar ("mapvote_need",
		ReadIniStr (ini, MAPVOTESECTION, "mapvote_need", buf, "0"), CVAR_LATCH);
	mapvote_pass = gi.cvar ("mapvote_pass",
		ReadIniStr (ini, MAPVOTESECTION, "mapvote_pass", buf, "51"), CVAR_LATCH);
	mapvote_next = gi.cvar( "mapvote_next", "1", 0 );

	return (use_mapvote);
}

//---------------

qboolean _iCheckMapVotes (void)
{
	static qboolean enough = false;
	float p;
	votelist_t *tmp;

	if (!map_need_to_check_votes)
		return (enough);

	tmp = MapWithMostVotes (&p);

	enough = (tmp != NULL && p >= mapvote_pass->value / 100.0f);
	if (map_num_clients < mapvote_min->value)
		enough = false;
	if (mapvote_need->value)
	{
		if ((float)((float) map_num_votes / (float) map_num_clients) <
			(float)(mapvote_need->value / 100.0f))
			enough = false;
	}

	map_need_to_check_votes = false;

	return (enough);
}


votelist_t *MapWithMostVotes (float *p)
{
	float p_most = 0.0f, votes = 0.f;
	votelist_t *search = NULL, *most = NULL;

	if (map_votes == NULL)
		return (NULL);

	//find map_num_clients
	map_num_clients = _numclients();

	if (map_num_clients == 0)
		return (NULL);

	most = NULL;
	for (search = map_votes; search != NULL; search = search->next)
	{
		votes = (float)((float)search->num_votes / (float)map_num_clients);
		if (votes > p_most)
		{
			p_most = votes;
			most = search;
		}
		/*
		else if( votes && (votes == p_most) )
		{
			// FIXME: Tie-breaker?
		}
		*/
	}

	if (p != NULL)
		*p = p_most;
	return (most);
}


votelist_t *MapWithMostAllVotes( void )
{
	votelist_t *search = NULL, *most = NULL;
	int highest_total = 0;

	if( ! _numclients() )
		return NULL;

	for( search = map_votes; search != NULL; search = search->next )
	{
		int total = search->num_allvotes + search->num_votes;
		if( total > highest_total )
		{
			highest_total = total;
			most = search;
		}
		/*
		else if( total && (total == highest_total) )
		{
			// FIXME: Tie-breaker?
		}
		*/
	}

	return most;
}


int AddVoteToMap(const char *mapname, edict_t * ent)
{
	int changed = 0;
	votelist_t *search;

	map_need_to_check_votes = true;

	if (ent->client->resp.mapvote != NULL)
	{
		_RemoveVoteFromMap(ent);
		changed = 1;
	}

	for (search = map_votes; search != NULL; search = search->next)
	{
		if (!Q_stricmp(search->mapname, mapname))
		{
			map_num_votes++;
			search->num_votes++;
			ent->client->resp.mapvote = search->mapname;
			return changed;
		}
	}

	// if we get here we didn't find the map!
	return -1;
}

void MapSelected (edict_t * ent, pmenu_t * p)
{
	char *ch;

	ch = p->text;
	if (ch)
	{
		while (*ch != ' ' && *ch != '\0')
			ch++;
		*ch = '\0';
	}
	ch = p->text;
	if (ch && *ch == '*')
		ch++;
	PMenu_Close (ent);
	Votemap(ent, ch);
}

void AddMapToMenu (edict_t * ent, int fromix)
{
	int i;
	char buffer[512], spc[64];
	votelist_t *search;
	float prozent;

	i = 0;
	search = map_votes;
	while (search && i < fromix)
	{
		search = search->next;
		i++;
	}
	while (search)
	{
		prozent =
		(float) (((float) search->num_votes / (float) map_num_clients) * 100.0f);
		i = 27 - strlen (search->mapname);
		if (prozent < 10.00)
			i -= 6;
		else if (prozent < 100.00)
			i -= 7;
		else
			i -= 8;
		if (i < 0)
			i = 0;
		spc[i--] = '\0';
		while (i >= 0)
			spc[i--] = ' ';
		//+ Marker: Hier einbauen, daß die gewählte Karte markiert ist
		// problem: '*' am anfang wird nicht berücksichtigt. - erledigt -
		//alt: sprintf(buffer, "%s%s%.1f%%", search->mapname, spc, prozent);
		sprintf (buffer, "%s%s%s%.1f%%",
		ent->client->resp.mapvote == search->mapname ? "*" : "",
		search->mapname, spc, prozent);


		if (xMenu_Add (ent, buffer, MapSelected) == true)
			search = search->next;
		else
			search = NULL;
	}
}

void MapVoteMenu (edict_t * ent, pmenu_t * p)
{
	char buf[1024], sbuf[512];

	sbuf[0] = 0;
	PMenu_Close(ent);
	_MostVotesStr(sbuf);
	Com_sprintf(buf, sizeof(buf), "most: %s", sbuf);

	if (xMenu_New (ent, MAPMENUTITLE, buf, AddMapToMenu) == false)
		gi.cprintf (ent, PRINT_MEDIUM, "No map to vote for.\n");
}

votelist_t *VotelistInsert( votelist_t *start, votelist_t *insert )
{
	votelist_t *tmp, *after;

	// If this is the first element or goes before the first, it's the new start point.
	if( (! start) || (Q_stricmp( insert->mapname, start->mapname ) < 0) )
	{
		insert->next = start;
		return insert;
	}
	
	// Loop until we find a place to insert the new element into the list.
	for( tmp = start; tmp->next && (Q_stricmp( insert->mapname, tmp->next->mapname ) >= 0); tmp = tmp->next ){}
	
	// Insert the new element.
	after = tmp->next;
	tmp->next = insert;
	insert->next = after;
	
	// We didn't replace it, so return the original start point.
	return start;
}

void ReadMaplistFile (void)
{
	int i, bs;
	votelist_t *tmp = NULL;
	FILE *maplist_file;
	char buf[MAX_STR_LEN];
	//Igor[Rock] BEGIN
	// added variable maplist.ini Files with Variable "maplistname"
	// changed maplistpath to a global variable!
	cvar_t *maplistname;

	map_votes = NULL;
	map_num_maps = 0;

	maplistname = gi.cvar ("maplistname", "maplist.ini", 0);
	if (maplistname->string && *(maplistname->string))
		Com_sprintf(maplistpath, sizeof(maplistpath), "%s/%s", GAMEVERSION, maplistname->string);
	else
		Com_sprintf(maplistpath, sizeof(maplistpath), "%s/%s", GAMEVERSION, "maplist.ini");

	maplist_file = fopen(maplistpath, "r");
	//Igor[Rock] End
	if (maplist_file == NULL)
	{
		// no "maplist.ini" file so use the maps from "action.ini"
		strcpy( maplistpath, IniPath() );

		for (i = 0; i < num_maps; i++)
		{
			tmp = (struct votelist_s *)gi.TagMalloc(sizeof(struct votelist_s), TAG_GAME);
			tmp->mapname = map_rotation[i];
			tmp->num_votes = 0;
			tmp->num_allvotes = 0;
			tmp->next = NULL;
			map_votes = VotelistInsert( map_votes, tmp );
			map_num_maps ++;
		}
	}
	else
	{
		// read the maplist.ini file
		while( fgets(buf, MAX_STR_LEN - 10, maplist_file) )
		{
			//first remove trailing spaces
			bs = strlen(buf);
			while (bs > 0 && buf[bs - 1] <= ' ')
				buf[--bs] = '\0';

			if (bs < 3 || !strncmp(buf, "#", 1) || !strncmp(buf, "//", 2))
				continue;

			tmp = (struct votelist_s *)gi.TagMalloc(sizeof(struct votelist_s), TAG_GAME);
			tmp->mapname = gi.TagMalloc (bs + 1, TAG_GAME);
			strcpy(tmp->mapname, buf);
			tmp->num_votes = 0;
			tmp->num_allvotes = 0;
			tmp->next = NULL;
			map_votes = VotelistInsert( map_votes, tmp );
			map_num_maps ++;
		}
		fclose(maplist_file);
	}

	/*
	num_allvotes = 0;

	//Igor[Rock] BEGIN
	//load the saved values from the last run of the server
	Q_strncatz(maplistpath, "-votes", sizeof(maplistpath));

	maplist_file = fopen(maplistpath, "r");
	if (maplist_file != NULL)
	{
		for (i = 0; fgets (buf, MAX_STR_LEN - 10, maplist_file) != NULL;)
		{
			//first remove trailing spaces
			bs = strlen(buf);
			while (bs > 0 && buf[bs - 1] <= ' ')
				buf[--bs] = '\0';

			if (bs < 1 || !strncmp(buf, "#", 1) || !strncmp(buf, "//", 2))
				continue;

			if (i == 0)
			{
				//num_allvotes = atoi(buf);  // Don't trust this; count as we go.
			}
			else if (map_votes)
			{
				// Split the buffer on the comma.
				char *num = strrchr( buf, ',' );
				if( ! num )
					continue;
				*num = '\0';
				num ++;

				for (tmp = map_votes; tmp->next != NULL; tmp = tmp->next)
				{
					if( Q_stricmp( tmp->mapname, buf ) == 0 )
					{
						tmp->num_allvotes = atoi(num);
						num_allvotes += tmp->num_allvotes;
						break;
					}
				}
			}
			i++;
		}
		fclose(maplist_file);
	}
	*/
}

//=== kick voting ==========================================================
//
//==========================================================================

#define KICKVOTESECTION "kickvote"

cvar_t *kickvote_min;
cvar_t *kickvote_need;
cvar_t *kickvote_pass;
cvar_t *kickvote_tempban;

qboolean kickvotechanged = false;
edict_t *Mostkickvotes = NULL;
float Allkickvotes = 0.0;
float Mostkickpercent = 0.0;

static void Votekicknum(edict_t *ent, const char *clientNUM);

void _SetKickVote (edict_t * ent, edict_t * target)
{
	if (ent->client->resp.kickvote == target)
	{
		ent->client->resp.kickvote = NULL;
		gi.cprintf (ent, PRINT_MEDIUM, "Your kickvote on %s is removed\n",
		target->client->pers.netname);
		if (vk_public->value)
			gi.bprintf (PRINT_HIGH, "%s doesnt want to kick %s after all\n",
		ent->client->pers.netname, target->client->pers.netname);
	}
	else
	{
		if (ent->client->resp.kickvote)
		{
			gi.cprintf (ent, PRINT_MEDIUM, "Kickvote was changed to %s\n",
			target->client->pers.netname);
		}
		else
		{
			gi.cprintf (ent, PRINT_MEDIUM, "You voted on %s to be kicked\n",
				target->client->pers.netname);
			if (vk_public->value) {
				gi.bprintf (PRINT_HIGH, "%s voted to kick %s\n",
					ent->client->pers.netname, target->client->pers.netname);
			}
		}
		ent->client->resp.kickvote = target;
		kickvotechanged = true;
	}

	kickvotechanged = true;
	_CheckKickVote ();
}

void _ClrKickVotesOn (edict_t * target)
{
	edict_t *other;
	int i, count = 0;

	for (i = 0, other = g_edicts + 1; i < game.maxclients; i++, other++)
	{
		if (!other->client || !other->inuse)
			continue;

		if (other->client->resp.kickvote == target) {
			other->client->resp.kickvote = NULL;
			count++;
		}
	}

	if (count > 0 || target->client->resp.kickvote)
	{
		kickvotechanged = true;
		_CheckKickVote();
	}
}

void _DoKick (edict_t * target)
{
	char buf[128];

	sprintf (buf, "more than %i%% voted for.", (int) kickvote_pass->value);

	_ClrKickVotesOn (target);
	if (kickvote_tempban->value)
		Ban_TeamKiller( target, (int)kickvote_tempban->value ); // Ban for some games (usually 1)

	KickClient (target, buf);
}

cvar_t *_InitKickVote (ini_t * ini)
{
  char buf[1024];

  kickvote_min = gi.cvar ("kickvote_min",
			  ReadIniStr (ini, KICKVOTESECTION, "kickvote_min",
				      buf, "4"), CVAR_LATCH);
  kickvote_need =
    gi.cvar ("kickvote_need",
	     ReadIniStr (ini, KICKVOTESECTION, "kickvote_need", buf, "0"),
	     CVAR_LATCH);
  kickvote_pass =
    gi.cvar ("kickvote_pass",
	     ReadIniStr (ini, KICKVOTESECTION, "kickvote_pass", buf, "75"),
	     CVAR_LATCH);
  kickvote_tempban =
    gi.cvar ("kickvote_tempban",
	     ReadIniStr (ini, KICKVOTESECTION, "kickvote_tempban", buf, "1"),
	     CVAR_LATCH);


  kickvotechanged = false;
  return (use_kickvote);
}

void _InitKickClient (edict_t * ent)
{
	ent->client->resp.kickvote = NULL;
}

void _ClientKickDisconnect (edict_t * ent)
{
	_ClrKickVotesOn (ent);
}

void _CheckKickVote (void)
{
	int i, j, votes, maxvotes, playernum, playervoted;
	edict_t *ent, *other, *target, *mtarget;

	if (kickvotechanged == false)
		return;

	kickvotechanged = false;
	playernum = _numclients ();

	maxvotes = 0;
	mtarget = NULL;
	playervoted = 0;
	for (i = 0, other = g_edicts + 1; i < game.maxclients; i++, other++)
	{
		if (!other->client || !other->inuse)
			continue;

		target = other->client->resp.kickvote;
		if (!target || target == mtarget)
			continue;

		votes = 0;
		playervoted++;
		for (j = 0, ent = g_edicts + 1; j < game.maxclients; j++, ent++)
		{
			if (ent->client && ent->inuse && ent->client->resp.kickvote == target)
				votes++;
		}
		if (votes > maxvotes)
		{
			maxvotes = votes;
			mtarget = target;
		}
	}

	Mostkickvotes = NULL;

	if (!mtarget)
		return;

	Mostkickvotes = mtarget;
	Mostkickpercent = (float) (((float) maxvotes / (float) playernum) * 100.0);
	Allkickvotes = (float) (((float) playervoted / (float) playernum) * 100.0);

	if (playernum < kickvote_min->value)
		return;

	if (Allkickvotes < kickvote_need->value)
		return;
	if (Mostkickpercent < kickvote_pass->value)
		return;

	// finally
	_DoKick (mtarget);
}

void _KickSelected(edict_t *ent, pmenu_t *p)
{
	char *ch;

	ch = p->text;
	if (ch)
	{
		while (*ch != ':' && *ch != '\0')
			ch++;
		*ch = '\0';
	}
	ch = p->text;
	if (ch) {
		if (*ch == '*')
			ch++;
		while (*ch == ' ')
			ch++;
	}
	PMenu_Close(ent);
	Votekicknum(ent, ch);
}

#define MostKickMarker " "

void _AddKickuserToMenu (edict_t * ent, int fromix)
{
	int i, j;
	edict_t *other;
	qboolean erg;
	char buf[256];

	j = 0;
	for(i = 0, other = g_edicts + 1; i < game.maxclients && j < fromix; i++, other++)
	{
		if (!other->inuse || !other->client || other->client->pers.mvdspec)
			continue;

		if (other != ent)
			j++;
	}
	erg = true;

	for (; i < game.maxclients && erg; i++, other++)
	{
		if (!other->inuse || !other->client || other->client->pers.mvdspec)
			continue;

		if (other != ent)
		{
			//+ Marker: Hier gewählten markieren - erledigt -
			sprintf (buf, "%s%2i: %s%s",
			other == ent->client->resp.kickvote ? "*" : "", i,
			other->client->pers.netname,
			other == Mostkickvotes ? MostKickMarker : "");
			erg = xMenu_Add (ent, buf, _KickSelected);
		}
	}
}

void _KickVoteSelected (edict_t * ent, pmenu_t * p)
{
	PMenu_Close (ent);
	if (xMenu_New (ent, KICKMENUTITLE, "vote for a player to kick",
		_AddKickuserToMenu) == false)
		gi.cprintf (ent, PRINT_MEDIUM, "No player to kick.\n");
}

void Cmd_Votekick_f(edict_t *ent)
{
	edict_t *target;

	if (!use_kickvote->value) {
		gi.cprintf(ent, PRINT_HIGH, "Kick voting is disabled.\n");
		return;
	}

	if (gi.argc() < 2) {
		gi.cprintf(ent, PRINT_HIGH, "Use votekick <playername>.\n");
		return;
	}

	target = LookupPlayer(ent, gi.args(), false, true);
	if (!target) {
		gi.cprintf(ent, PRINT_HIGH, "\nUse kicklist to see who can be kicked.\n");
		return;
	}
	if (target == ent)
		gi.cprintf(ent, PRINT_HIGH, "You can't votekick yourself.\n");
	else
		_SetKickVote(ent, target);
}

static void Votekicknum(edict_t *ent, const char *clientNUM)
{
	edict_t *target;

	if (!use_kickvote->value) {
		gi.cprintf(ent, PRINT_HIGH, "Kick voting is disabled.\n");
		return;
	}

	if (!*clientNUM) {
		gi.cprintf (ent, PRINT_HIGH, "Use votekicknum <playernumber>.\n");
		return;
	}

	target = LookupPlayer(ent, clientNUM, true, false);
	if (!target) {
		gi.cprintf(ent, PRINT_HIGH, "\nUse kicklist to see who can be kicked.\n");
		return;
	}
	if (target == ent)
		gi.cprintf(ent, PRINT_HIGH, "You can't votekick yourself.\n");
	else
		_SetKickVote(ent, target);

}

void Cmd_Votekicknum_f(edict_t *ent)
{
	Votekicknum(ent, gi.args());
}

qboolean _vkMarkThis(edict_t *self, edict_t *other)
{
	if (self->client->resp.kickvote == other)
		return true;
	return false;
}

void Cmd_Kicklist_f(edict_t *ent)
{
  char buf[MAX_STRING_CHARS], tbuf[256];

  if (!use_kickvote->value) {
	  gi.cprintf(ent, PRINT_HIGH, "Kick voting is disabled.\n");
	  return;
  }

  strcpy(buf, "Available players to kick:\n\n");
  _printplayerlist(ent, buf, _vkMarkThis);

  // adding vote settings
  Com_sprintf (tbuf, sizeof(tbuf), "Vote rules: %i client%s min. (currently %i),\n" \
	   "%.1f%% must have voted overall (currently %.1f%%)\n" \
	   "and %.1f%% on the same (currently %.1f%% on %s),\n" \
	   "kicked players %s be temporarily banned.\n\n",
	   (int) (kickvote_min->value),
	   (kickvote_min->value == 1) ? "" : "s",
	   _numclients(),
	   kickvote_need->value, Allkickvotes,
	   kickvote_pass->value, Mostkickpercent,
	   Mostkickvotes == NULL ? "nobody" : Mostkickvotes->client->pers.netname,
	   kickvote_tempban ? "will" : "won't");
  // double percent sign! cprintf will process them as format strings.

  Q_strncatz(buf, tbuf, sizeof(buf));
  gi.cprintf(ent, PRINT_MEDIUM, "%s", buf);
}

//=== config voting ========================================================
//
// Original programed by Black Cross[NL], adapted, major changes.
//
//==========================================================================

configlist_t *config_votes;
int config_num_configs;
int config_num_votes;
int config_num_clients;
qboolean config_need_to_check_votes;

cvar_t *cvote_min;
cvar_t *cvote_need;
cvar_t *cvote_pass;

char configlistpath[MAX_STR_LEN];

#define CONFIGVOTESECTION "configvote"

// forward declarations
configlist_t *ConfigWithMostVotes(float *p);
int AddVoteToConfig(const char *configname, edict_t *ent);
void ReadConfiglistFile(void);
qboolean _iCheckConfigVotes(void);

//
static void Voteconfig(edict_t *ent, const char *config)
{
	if (!use_cvote->value) {
		gi.cprintf(ent, PRINT_HIGH, "Config voting is disabled.\n");
		return;
	}

	if (!*config) {
		ConfigVoteMenu( ent, NULL );
		return;
	}

	if (level.intermission_framenum) {
		gi.cprintf (ent, PRINT_HIGH, "Configvote disabled during intermission\n");
		return;
	}

	if (level.realFramenum < 10 * HZ)
	{
		gi.cprintf (ent, PRINT_HIGH, "Configvote currently blocked - Please vote again in %d seconds\n",
			(11 * HZ - level.realFramenum) / HZ );
		return;
	}

	switch (AddVoteToConfig(config, ent)) {
	case 0:
		gi.cprintf(ent, PRINT_HIGH, "You have voted on config \"%s\"\n", config);
		break;
	case 1:
		gi.cprintf(ent, PRINT_HIGH, "You have changed your vote to config \"%s\"\n", config);
		break;
	default:
		//error
		gi.cprintf(ent, PRINT_HIGH, "Config \"%s\" is not in the votelist!\n", config);
		break;
	}
}

void Cmd_Voteconfig_f(edict_t *ent)
{
	Voteconfig(ent, gi.args());
}

//
void Cmd_Configlist_f(edict_t *ent)
{
  //go through the votelist and list out all the configs and % votes
  int lines, chars_on_line, len_mr;
  float p_test, p_most;
  configlist_t *search, *most;
  char msg_buf[MAX_STRING_CHARS], tmp_buf[128];	//only 40 are used

  if (!use_cvote->value) {
	  gi.cprintf(ent, PRINT_HIGH, "Config voting is disabled.\n");
	  return;
  }

	p_test = p_most = 0.0;

	most = ConfigWithMostVotes (&p_most);

	sprintf (msg_buf,
		"List of configs that can be voted on:\nRequire more than %d%% votes (%.2f)\n\n",
		(int) cvote_pass->value,
		(float) ((float) cvote_pass->value / 100.0));

	lines = chars_on_line = 0;
	for (search = config_votes; search != NULL; search = search->next)
	{

		if (config_num_clients > 0)
			p_test = (float)((float) search->num_votes / (float) config_num_clients);

		if (p_test >= 10.0)
			len_mr = 11;
		else
			len_mr = 10;

		len_mr += strlen (search->configname);

		if ((chars_on_line + len_mr + 2) > 39)
		{
			Q_strncatz (msg_buf, "\n", sizeof(msg_buf));
			lines++;
			chars_on_line = 0;
			if (lines > 25)
				break;
		}
		sprintf (tmp_buf, "%s (%.2f)  ", search->configname, p_test);
		Q_strncatz (msg_buf, tmp_buf, sizeof(msg_buf));
		chars_on_line += len_mr;
	}

	if (config_votes == NULL)
		Q_strncatz (msg_buf, "None!", sizeof(msg_buf));
	else if (most != NULL)
	{
		sprintf (tmp_buf, "\n\nMost votes: %s (%.2f)",
		most->configname, p_most);
		Q_strncatz (msg_buf, tmp_buf, sizeof(msg_buf));
	}

	Q_strncatz (msg_buf, "\n\n", sizeof(msg_buf));
	Com_sprintf (tmp_buf, sizeof(tmp_buf), 
		"%d/%d (%.2f%%) clients voted\n%d client%s minimum (%d%% required)",
		config_num_votes, config_num_clients,
		(float) ((float) config_num_votes / (float) (config_num_clients >
		0 ? config_num_clients : 1) * 100),
		(int) cvote_min->value, (cvote_min->value > 1 ? "s" : ""),
		(int) cvote_need->value);

	Q_strncatz (msg_buf, tmp_buf, sizeof(msg_buf));

	gi.centerprintf (ent, "%s", msg_buf);

	return;
}

//
void _ConfigInitClient (edict_t * ent)
{
	ent->client->resp.cvote = NULL;
}

//
void _RemoveVoteFromConfig (edict_t * ent)
{
	configlist_t *search;

	config_need_to_check_votes = true;

	if (ent->client->resp.cvote == NULL)
		return;

	for (search = config_votes; search != NULL; search = search->next)
	{
		if (!Q_stricmp(search->configname, ent->client->resp.cvote))
		{
			config_num_votes--;
			search->num_votes--;
			ent->client->resp.cvote = NULL;
			break;
		}
	}
	return;
}

//
void _ConfigExitLevel (char *NextMap)
{
  configlist_t *voteconfig = NULL;
  char buf[MAX_STR_LEN];

	if (_iCheckConfigVotes ())
	{
		voteconfig = ConfigWithMostVotes (NULL);
		gi.bprintf (PRINT_HIGH, "A new config was voted on and is %s.\n",
			voteconfig->configname);
		Com_sprintf (buf, sizeof (buf), "exec \"mode_%s.cfg\"\n",
			voteconfig->configname);

		//clear stats
		for (voteconfig = config_votes; voteconfig != NULL; voteconfig = voteconfig->next)
		{
			voteconfig->num_votes = 0;
		}

		//clear stats
		config_num_votes = 0;
		config_num_clients = 0;
		config_need_to_check_votes = true;
		gi.AddCommandString (buf);
	}
	else
	{
		//clear stats
		for (voteconfig = config_votes; voteconfig != NULL; voteconfig = voteconfig->next)
		{
			voteconfig->num_votes = 0;
		}

		//clear stats
		config_num_votes = 0;
		config_num_clients = 0;
		config_need_to_check_votes = true;
	}

}

//
qboolean _CheckConfigVotes (void)
{
	if (_iCheckConfigVotes () == true)
	{
		gi.bprintf (PRINT_HIGH, "More than %i%% config votes reached.\n",
			(int) cvote_pass->value);
		return true;
	}
	return false;
}

//
qboolean _ConfigMostVotesStr (char *buf)
{
	float p_most = 0.0f;
	configlist_t *most;

	most = ConfigWithMostVotes (&p_most);
	if (most != NULL)
	{
		sprintf (buf, "%s (%.2f%%)", most->configname, p_most * 100.0);
		return true;
	}
	else
		strcpy (buf, "(no config)");

	return false;
}

//
void _ConfigWithMostVotes (void)
{
	char buf[1024], sbuf[512];

	if (_ConfigMostVotesStr(sbuf))
	{
		Com_sprintf(buf, sizeof(buf), "Most wanted config: %s", sbuf);
		G_HighlightStr(buf, buf, sizeof(buf));
		gi.bprintf(PRINT_HIGH, "%s\n", buf);
	}
}

//
cvar_t *_InitConfiglist (ini_t * ini)
{
	char buf[1024];

	// note that this is done whether we have set "use_cvote" or not!
	config_votes = NULL;
	config_num_configs = 0;
	config_num_votes = 0;
	config_num_clients = 0;
	config_need_to_check_votes = true;
	ReadConfiglistFile ();

	use_cvote = gi.cvar ("use_cvote", "0", 0);

	cvote_min = gi.cvar ("cvote_min",
		ReadIniStr (ini, CONFIGVOTESECTION, "cvote_min", buf, "1"), CVAR_LATCH);
	cvote_need = gi.cvar ("cvote_need",
		ReadIniStr (ini, CONFIGVOTESECTION, "cvote_need", buf, "0"), CVAR_LATCH);
	cvote_pass = gi.cvar ("cvote_pass",
		ReadIniStr (ini, CONFIGVOTESECTION, "cvote_pass", buf, "51"), CVAR_LATCH);
	return (use_cvote);
}

//---------------

qboolean _iCheckConfigVotes (void)
{
	static qboolean enough = false;
	float p;
	configlist_t *tmp;

	if (!config_need_to_check_votes)
		return (enough);

	tmp = ConfigWithMostVotes (&p);

	enough = (tmp != NULL && p >= (float) (cvote_pass->value / 100.0f));
	if (config_num_clients < cvote_min->value)
		enough = false;
	if (cvote_need->value)
	{
		if ((float) ((float)config_num_votes / (float)config_num_clients) <
		(float)(cvote_need->value / 100.0f))
			enough = false;
	}

	config_need_to_check_votes = false;

	return (enough);
}


configlist_t *ConfigWithMostVotes (float *p)
{
  float p_most;
  configlist_t *search, *most;

  p_most = 0.0;
  if (config_votes == NULL)
    return (NULL);

  //find config_num_clients
  config_num_clients = _numclients();

  if (config_num_clients == 0)
    return (NULL);

  most = NULL;
  for (search = config_votes; search != NULL; search = search->next)
    {
      if ((float) ((float) search->num_votes / (float) config_num_clients) >
	  p_most)
	{
	  p_most =
	    (float) ((float) search->num_votes / (float) config_num_clients);
	  most = search;
	}
    }

  if (p != NULL)
    *p = p_most;
  return (most);
}

int AddVoteToConfig(const char *configname, edict_t *ent)
{
	int changed = 0;
	configlist_t *search;

	config_need_to_check_votes = true;

	if (ent->client->resp.cvote != NULL) {
		_RemoveVoteFromConfig(ent);
		changed = 1;
	}

	for (search = config_votes; search != NULL; search = search->next) {
		if (Q_stricmp(search->configname, configname) == 0)
		{
			config_num_votes++;
			search->num_votes++;
			ent->client->resp.cvote = search->configname;
			return changed;
		}
	}

	// if we get here we didn't find the config!
	return -1;
}

void ConfigSelected (edict_t * ent, pmenu_t * p)
{
	char *ch;

	ch = p->text;
	if (ch)
	{
		while (*ch != ' ' && *ch != '\0')
			ch++;
		*ch = '\0';
	}
	ch = p->text;
	if (ch && *ch == '*')
		ch++;

	PMenu_Close(ent);
	Voteconfig(ent, ch);
}

void AddConfigToMenu (edict_t * ent, int fromix)
{
  int i;
  char buffer[512], spc[64];
  configlist_t *search;
  float prozent;

  i = 0;
  search = config_votes;
  while (search && i < fromix)
    {
      search = search->next;
      i++;
    }
  while (search)
    {
      prozent =
	(float) (((float) search->num_votes / (float) config_num_clients) *
		 100);
      i = 27 - strlen (search->configname);
      if (prozent < 10.00)
	i -= 6;
      else if (prozent < 100.00)
	i -= 7;
      else
	i -= 8;
      if (i < 0)
	i = 0;
      spc[i--] = '\0';
      while (i >= 0)
	spc[i--] = ' ';
      sprintf (buffer, "%s%s%s%.1f%%",
	       ent->client->resp.cvote == search->configname ? "*" : "",
	       search->configname, spc, prozent);

      if (xMenu_Add (ent, buffer, ConfigSelected) == true)
	search = search->next;
      else
	search = NULL;
    }
}

void ConfigVoteMenu (edict_t * ent, pmenu_t * p)
{
	char buf[1024], sbuf[512];

	sbuf[0] = 0;
	PMenu_Close(ent);
	_ConfigMostVotesStr(sbuf);
	Com_sprintf(buf, sizeof(buf), "most: %s", sbuf);
	if (xMenu_New(ent, CONFIGMENUTITLE, buf, AddConfigToMenu) == false)
		gi.cprintf(ent, PRINT_MEDIUM, "No config to vote for.\n");
}

void ReadConfiglistFile (void)
{
	int i, bs;
	configlist_t *list = NULL, *tmp;
	FILE *configlist_file;
	char buf[MAX_STR_LEN];
	cvar_t *configlistname;

	configlistname = gi.cvar("configlistname", "configlist.ini", 0);
	if (configlistname->string && *(configlistname->string))
		Com_sprintf(configlistpath, sizeof(configlistpath), "%s/%s", GAMEVERSION, configlistname->string);
	else
		Com_sprintf(configlistpath, sizeof(configlistpath), "%s/%s", GAMEVERSION, "configlist.ini");

  configlist_file = fopen(configlistpath, "r");
  if (!configlist_file)
	  return;

	// read the configlist.ini file
	for (i = 0; fgets (buf, MAX_STR_LEN - 10, configlist_file) != NULL;)
	{
		//first remove trailing spaces
		bs = strlen(buf);
		while (bs > 0 && buf[bs - 1] <= ' ')
			buf[--bs] = '\0';

		if (bs < 3 || !strncmp(buf, "#", 1) || !strncmp(buf, "//", 2))
			continue;

		if (i == 0)
		{
			config_votes = (struct configlist_s *)gi.TagMalloc(sizeof(struct configlist_s), TAG_GAME);
			config_votes->configname = gi.TagMalloc(bs + 1, TAG_GAME);
			strcpy(config_votes->configname, buf);
			config_votes->num_votes = 0;
			config_votes->next = NULL;
			list = config_votes;
			i++;
		}
		else
		{
			tmp = (struct configlist_s *)gi.TagMalloc(sizeof(struct configlist_s), TAG_GAME);
			tmp->configname = gi.TagMalloc(bs + 1, TAG_GAME);
			strcpy(tmp->configname, buf);
			tmp->num_votes = 0;
			tmp->next = NULL;
			list->next = tmp;
			list = tmp;
			i++;
		}
	}
	fclose(configlist_file);
	config_num_configs = i;
}


//=== player ignoring ======================================================
//
// player ingoring is no voting, but since it uses the same
// functions like kickvoting and offers a menu, I put it here.
// At least you can say, that when it's no vote, it's a choice! :)
//
//==========================================================================


#define IGNORELIST client->pers.ignorelist


//Returns the next free slot in ignore list
int _FindFreeIgnoreListEntry (edict_t * source)
{
  return (IsInIgnoreList (source, NULL));
}

//Clears a clients ignore list
void _ClearIgnoreList (edict_t * ent)
{
  int i;

  if (!ent->client)
    return;
  for (i = 0; i < PG_MAXPLAYERS; i++)
    ent->IGNORELIST[i] = NULL;
}

//Checks if an edict is to be ignored, returns position
int IsInIgnoreList (edict_t * source, edict_t * subject)
{
	int i;

	if (!source || !source->client)
		return 0;

	//ignorelist[0] is not used...
	for (i = 1; i < PG_MAXPLAYERS; i++) {
		if (source->IGNORELIST[i] == subject)
			return i;
	}
	return 0;
}

//Adds an edict to ignore list. If allready in, it will be removed
void _AddOrDelIgnoreSubject (edict_t * source, edict_t * subject, qboolean silent)
{
	int i;

	if (!source->client)
		return;
	if (!subject->client || !subject->inuse)
	{
		gi.cprintf (source, PRINT_MEDIUM, "\nOnly valid clients may be added to ignore list!\n");
		return;
	}

	i = IsInIgnoreList (source, subject);
	if (i)
	{
		//subject is in ignore list, so delete it
		source->IGNORELIST[i] = NULL;
		if (!silent)
			gi.cprintf (source, PRINT_MEDIUM, "\n%s was removed from ignore list.\n",
				subject->client->pers.netname);

		//Maybe this has to be taken out :)

		if( ! silent && ! IsInIgnoreList( subject, source ) )
			gi.cprintf (subject, PRINT_MEDIUM, "\n%s listens to your words.\n",
				source->client->pers.netname);

		source->client->resp.ignore_time = level.realFramenum;
	}
	else
	{
		//subject has to be added
		i = _FindFreeIgnoreListEntry (source);

		if (!i)
		{
			if (!silent)
				gi.cprintf (source, PRINT_MEDIUM, "\nSorry, ignore list is full!\n");
		}
		else
		{
			//we've found a place
			source->IGNORELIST[i] = subject;
			if (!silent)
				gi.cprintf (source, PRINT_MEDIUM, "\n%s was added to ignore list.\n",
					subject->client->pers.netname);

			//Maybe this has to be taken out :)

			if( ! silent && ! IsInIgnoreList( subject, source ) )
				gi.cprintf (subject, PRINT_MEDIUM, "\n%s ignores you.\n",
				source->client->pers.netname);
		}
	}
}

//
void _ClrIgnoresOn (edict_t *target)
{
	edict_t *other;
	int i;

	for (i = 0, other = g_edicts + 1; i < game.maxclients; i++, other++)
	{
		if (!other->client || !other->inuse)
			continue;

		if (IsInIgnoreList(other, target))
			_AddOrDelIgnoreSubject(other, target, true);
	}
}


//Ignores players by part of the name
void Cmd_IgnorePart_f(edict_t *self)
{
	int      i, count = 0;
	edict_t *target;
	char *name;

	if (gi.argc() < 2) {
		gi.cprintf(self, PRINT_MEDIUM, "Use ignorepart <part-of-playername>.\n");
		return;
	}
	if (level.realFramenum < (self->client->resp.ignore_time + 10 * HZ)) {
		gi.cprintf(self, PRINT_MEDIUM, "Wait 10 seconds before ignoring again.\n");
		return;
	}

	name = gi.args();
	for (i = 0, target = g_edicts + 1; i < game.maxclients; i++, target++)
	{
		if (!target->inuse || !target->client || target == self || target->client->pers.mvdspec)
			continue;

		if (strstr(target->client->pers.netname, name)) {
			_AddOrDelIgnoreSubject(self, target, false);
			count++;
		}
	}

	if (count == 0) {
		gi.cprintf (self, PRINT_MEDIUM, "\nUse ignorelist to see who can be ignored.\n");
	}
}


//Ignores a player by name
void Cmd_Ignore_f(edict_t *self)
{
	edict_t *target;
	char *name;

	if (gi.argc() < 2) {
		gi.cprintf(self, PRINT_MEDIUM, "Use ignore <playername>.\n");
		return;
	}
	
	if (level.realFramenum < (self->client->resp.ignore_time + 5 * HZ)) {
		gi.cprintf(self, PRINT_MEDIUM, "Wait 5 seconds before ignoring again.\n");
		return;
	}

	name = gi.args();
	target = LookupPlayer(self, name, false, true);
	if (!target) {
		gi.cprintf(self, PRINT_MEDIUM, "\nUse ignorelist to see who can be ignored.\n");
		return;
	}
	if (target == self)
		gi.cprintf(self, PRINT_HIGH, "You can't ignore yourself.\n");
	else
		_AddOrDelIgnoreSubject(self, target, false);
}

//Ignores a player by number
void IgnorePlayer(edict_t *self, char *clientNUM)
{
	edict_t *target;

	if (!*clientNUM) {
		gi.cprintf (self, PRINT_MEDIUM, "Use ignorenum <playernumber>.\n");
		return;
	}

	if (level.realFramenum < (self->client->resp.ignore_time + 5 * HZ)) {
		gi.cprintf (self, PRINT_MEDIUM, "Wait 5 seconds before ignoring again.\n");
		return;
	}

	target = LookupPlayer(self, clientNUM, true, false);
	if (!target) {
		gi.cprintf(self, PRINT_MEDIUM, "\nUse ignorelist to see who can be ignored.\n");
		return;
	}
	if (target == self)
		gi.cprintf(self, PRINT_HIGH, "You can't ignore yourself.\n");
	else
		_AddOrDelIgnoreSubject(self, target, false);
}

void Cmd_Ignorenum_f(edict_t *self)
{
	IgnorePlayer(self, gi.args());
}

qboolean _ilMarkThis (edict_t *self, edict_t *other)
{
	if (IsInIgnoreList (self, other))
		return true;
	return false;
}

void Cmd_Ignorelist_f(edict_t *self)
{
	char buf[MAX_STRING_CHARS];
	strcpy(buf, "Available players to ignore:\n\n");
	_printplayerlist(self, buf, _ilMarkThis);
	gi.cprintf(self, PRINT_MEDIUM, "%s", buf);
}

//Clears ignore list - user interface :)
void Cmd_Ignoreclear_f(edict_t *self)
{
	_ClearIgnoreList(self);
	gi.cprintf(self, PRINT_MEDIUM, "Your ignorelist is now clear.\n");
}

void _IgnoreSelected (edict_t * ent, pmenu_t * p)
{
	char *ch;

	ch = p->text;
	if (ch)
	{
		while (*ch != ':' && *ch != '\0')
			ch++;
		*ch = '\0';
	}
	ch = p->text;
	if (ch) {
		if (*ch == '*')
			ch++;
		while (*ch == ' ')
			ch++;
	}
	PMenu_Close(ent);
	IgnorePlayer(ent, ch);
}

void _AddIgnoreuserToMenu (edict_t * ent, int fromix)
{
	int i, j;
	edict_t *other;
	qboolean erg;
	char buf[256];

	j = 0;
	for (i = 0, other = g_edicts + 1; i < game.maxclients && j < fromix; i++, other++)
	{
		if (other->inuse && other != ent)
			j++;
	}
	erg = true;
	for (; i < game.maxclients && erg; i++, other++)
	{
		if (other->inuse && other != ent)
		{
			//+ Marker: Hier gewählten markieren - erledigt -
			sprintf (buf, "%s%2i: %s", IsInIgnoreList (ent, other) ? "*" : "",
			i, other->client->pers.netname);
			erg = xMenu_Add (ent, buf, _IgnoreSelected);
		}
	}
}

void _IgnoreVoteSelected (edict_t * ent, pmenu_t * p)
{
  PMenu_Close (ent);
  if (xMenu_New
      (ent, IGNOREMENUTITLE, "de-/select a player to ignore",
       _AddIgnoreuserToMenu) == false)
    gi.cprintf (ent, PRINT_MEDIUM, "No player to ignore.\n");
}



//=== flag voting ==========================================================
//
//==========================================================================

// hifi
cvar_t *scramblevote_min;
cvar_t *scramblevote_need;
cvar_t *scramblevote_pass;

cvar_t *_InitScrambleVote (ini_t * ini)
{
	use_scramblevote = gi.cvar ("use_scramblevote", "0", 0);
	scramblevote_min = gi.cvar ("scramblevote_min", "4", 0);
	scramblevote_need = gi.cvar ("scramblevote_need", "0", 0);
	scramblevote_pass = gi.cvar ("scramblevote_pass", "75", 0);

	if(!teamplay->value)
		return (teamplay);

	return (use_scramblevote);
}

qboolean ScrambleTeams(void)
{
	int i, j, numplayers, newteam;
	edict_t *ent, *players[MAX_CLIENTS], *oldCaptains[TEAM_TOP] = {NULL};

	numplayers = 0;
	for (i = 0, ent = &g_edicts[1]; i < game.maxclients; i++, ent++)
	{
		if (!ent->inuse || !ent->client || !ent->client->resp.team || ent->client->resp.subteam)
			continue;

		players[numplayers++] = ent;
	}

	if (numplayers <= teamCount)
		return false;

	for (i = numplayers - 1; i > 0; i--) {
		j = rand() % (i + 1);
		ent = players[j];
		players[j] = players[i];
		players[i] = ent;
	}

	MakeAllLivePlayersObservers();
	team_round_going = 0;

	if (matchmode->value) {
		for (i = TEAM1; i <= teamCount; i++) {
			oldCaptains[i] = teams[i].captain;
			teams[i].captain = NULL;
		}
	}

	for (i = 0; i < numplayers; i++) {
		const char *s;

		ent = players[i];
		newteam = (i % teamCount) + 1;

		if (oldCaptains[ent->client->resp.team] == ent && !teams[newteam].captain)
			teams[newteam].captain = ent;

		ent->client->resp.team = newteam;

		s = Info_ValueForKey( ent->client->pers.userinfo, "skin" );
		AssignSkin( ent, s, false );
	}

	teams_changed = true;

	CenterPrintAll("The teams have been scrambled!");

	//Clear voting
	for (i = 0, ent = &g_edicts[1]; i < game.maxclients; i++, ent++)
	{
		if (!ent->inuse || !ent->client)
			continue;

		ent->client->resp.scramblevote = 0;
	}
	return true;
}


void _CalcScrambleVotes (int *numclients, int *numvotes, float *percent)
{
	int i;
	edict_t *ent;

	*numclients = _numclients ();
	*numvotes = 0;
	*percent = 0.00f;

	for (i = 1; i <= game.maxclients; i++)
	{
		ent = &g_edicts[i];
		if (ent->client && ent->inuse && ent->client->resp.scramblevote)
		{
			(*numvotes)++;
		}
	}

	if(*numvotes > 0)
		(*percent) = (float) (((float) *numvotes / (float) *numclients) * 100.0);
}

void _CheckScrambleVote (void)
{
	int numvotes = 0, playernum = 0;
	float votes = 0.0f;
	char buf[128];

	_CalcScrambleVotes(&playernum, &numvotes, &votes);

	if (numvotes > 0) {
		Com_sprintf(buf, sizeof(buf), "Scramble: %d votes (%.1f%%), need %.1f%%", numvotes, votes, scramblevote_pass->value);
		G_HighlightStr(buf, buf, sizeof(buf));
		gi.bprintf(PRINT_HIGH, "%s\n", buf);
	}

	if (playernum < scramblevote_min->value)
		return;
	if (numvotes < scramblevote_need->value)
		return;
	if (votes < scramblevote_pass->value)
		return;

	ScrambleTeams();
}

void _VoteScrambleSelected (edict_t * ent, pmenu_t * p)
{
	PMenu_Close(ent);

	Cmd_Votescramble_f(ent);
}

void Cmd_Votescramble_f(edict_t *ent)
{
	if (!teamplay->value || !use_scramblevote->value)
		return;

	ent->client->resp.scramblevote = !ent->client->resp.scramblevote;

	if(ent->client->resp.scramblevote) {
		gi.cprintf (ent, PRINT_HIGH, "You voted for team scramble.\n");
		gi.bprintf (PRINT_HIGH, "%s voted for team scramble\n", ent->client->pers.netname);
	} else {
		gi.cprintf (ent, PRINT_HIGH, "You took your scramble vote back.\n");
		gi.bprintf (PRINT_HIGH, "%s changed his mind about team scramble\n", ent->client->pers.netname);
	}
}
