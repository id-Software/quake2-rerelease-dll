//-----------------------------------------------------------------------------
// Matchmode related code
//
// $Id: a_match.c,v 1.18 2004/04/08 23:19:51 slicerdw Exp $
//
//-----------------------------------------------------------------------------
// $Log: a_match.c,v $
// Revision 1.18  2004/04/08 23:19:51  slicerdw
// Optimized some code, added a couple of features and fixed minor bugs
//
// Revision 1.17  2003/06/15 21:43:53  igor
// added IRC client
//
// Revision 1.16  2003/02/10 02:12:25  ra
// Zcam fixes, kick crashbug in CTF fixed and some code cleanup.
//
// Revision 1.15  2002/03/28 12:10:11  freud
// Removed unused variables (compiler warnings).
// Added cvar mm_allowlock.
//
// Revision 1.14  2002/03/28 11:46:03  freud
// stat_mode 2 and timelimit 0 did not show stats at end of round.
// Added lock/unlock.
// A fix for use_oldspawns 1, crash bug.
//
// Revision 1.13  2002/03/25 23:34:06  slicerdw
// Small tweak on var handling ( prevent overflows )
//
// Revision 1.12  2001/12/05 15:27:35  igor_rock
// improved my english (actual -> current :)
//
// Revision 1.11  2001/12/02 16:15:32  igor_rock
// added console messages (for the IRC-Bot) to matchmode
//
// Revision 1.10  2001/11/25 19:09:25  slicerdw
// Fixed Matchtime
//
// Revision 1.9  2001/09/28 15:44:29  ra
// Removing Bill Gate's fingers from a_match.c
//
// Revision 1.8  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.7  2001/06/16 16:47:06  deathwatch
// Matchmode Fixed
//
// Revision 1.6  2001/06/13 15:36:31  deathwatch
// Small fix
//
// Revision 1.5  2001/06/13 07:55:17  igor_rock
// Re-Added a_match.h and a_match.c
// Added CTF Header for a_ctf.h and a_ctf.c
//
//-----------------------------------------------------------------------------

#include "g_local.h"
#include "a_match.h"

void SendScores(void)
{
	unsigned int mins, secs, gametime = level.matchTime;

	mins = gametime / 60;
	secs = gametime % 60;
	if(teamCount == 3) {
		gi.bprintf(PRINT_HIGH, "\x9D\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9F\n");
		gi.bprintf(PRINT_HIGH, " Team 1 Score - Team 2 Score - Team 3 Score\n");
		gi.bprintf(PRINT_HIGH, "    [%d]           [%d]           [%d]\n", teams[TEAM1].score, teams[TEAM2].score, teams[TEAM3].score);
		gi.bprintf(PRINT_HIGH, " Total Played Time: %d:%02d\n", mins, secs);
		gi.bprintf(PRINT_HIGH, "\x9D\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9F\n");
	} else {
		int team1score = 0, team2score = 0;

		if(ctf->value) {
			GetCTFScores(&team1score, &team2score);
		} else {
			team1score = teams[TEAM1].score;
			team2score = teams[TEAM2].score;
		}
		gi.bprintf(PRINT_HIGH, "\x9D\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9F\n");
		gi.bprintf(PRINT_HIGH, " Team 1 Score - Team 2 Score\n");
		gi.bprintf(PRINT_HIGH, "     [%d]           [%d]\n", team1score, team2score);
		gi.bprintf(PRINT_HIGH, " Total Played Time: %d:%02d\n", mins, secs);
		gi.bprintf(PRINT_HIGH, "\x9D\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9F\n");
	}
	gi.bprintf(PRINT_HIGH, "Match is over, waiting for next map, please vote a new one..\n");

	// Stats: Reset roundNum
	game.roundNum = 0;
	// Stats end
}

void Cmd_Sub_f(edict_t * ent)
{
	if (!matchmode->value) {
		gi.cprintf(ent, PRINT_HIGH, "This command needs matchmode to be enabled\n");
		return;
	}

	if (ent->client->resp.team == NOTEAM) {
		gi.cprintf(ent, PRINT_HIGH, "You need to be on a team for that...\n");
		return;
	}
	if (!ent->client->resp.subteam) {
		killPlayer(ent, true); // lets kill em.
		gi.bprintf(PRINT_HIGH, "%s is now a substitute for %s\n", ent->client->pers.netname, teams[ent->client->resp.team].name);
		ent->client->resp.subteam = ent->client->resp.team;
		return;
	}

	gi.bprintf(PRINT_HIGH, "%s is no longer a substitute for %s\n", ent->client->pers.netname, teams[ent->client->resp.team].name);
	ent->client->resp.subteam = 0;
	if (team_round_going && !(gameSettings & GS_ROUNDBASED))
	{
		PutClientInServer (ent);
		AddToTransparentList (ent);
	}
}


/*
==============
MM_SetCaptain
==============
Set ent to be a captain of team, ent can be NULL to remove captain
*/
void MM_SetCaptain( int teamNum, edict_t *ent )
{
	int i;
	edict_t *oldCaptain = teams[teamNum].captain;

	if (teamNum == NOTEAM)
		ent = NULL;

	teams[teamNum].captain = ent;
	if (!ent) {
		if (!team_round_going || (gameSettings & GS_ROUNDBASED)) {
			if (teams[teamNum].ready) {
				char temp[128];
				Com_sprintf( temp, sizeof( temp ), "%s is no longer ready to play!", teams[teamNum].name );
				CenterPrintAll( temp );
			}
			teams[teamNum].ready = 0;
		}
		if (oldCaptain) {
			gi.bprintf( PRINT_HIGH, "%s is no longer %s's captain\n", oldCaptain->client->pers.netname, teams[teamNum].name );
		}
		teams[teamNum].locked = 0;
		return;
	}

	if (ent != oldCaptain) {
		gi.bprintf( PRINT_HIGH, "%s is now %s's captain\n", ent->client->pers.netname, teams[teamNum].name );
		gi.cprintf( ent, PRINT_CHAT, "You are the captain of '%s'\n", teams[teamNum].name );
		gi.sound( &g_edicts[0], CHAN_VOICE | CHAN_NO_PHS_ADD, gi.soundindex( "misc/comp_up.wav" ), 1.0, ATTN_NONE, 0.0 );

		for (i = TEAM1; i <= teamCount; i++) {
			if (i != teamNum && teams[i].wantReset)
				gi.cprintf( ent, PRINT_HIGH, "Team %i wants to reset scores, type 'resetscores' to accept\n", i );
		}
	}
}

void MM_LeftTeam( edict_t *ent )
{
	int teamNum = ent->client->resp.team;

	if (teams[teamNum].captain == ent) {
		MM_SetCaptain( teamNum, NULL );
	}
	ent->client->resp.subteam = 0;
}

qboolean TeamsReady( void )
{
	int i, ready = 0;

	for( i = TEAM1; i <= teamCount; i++ )
	{
		if( teams[i].ready )
			ready ++;
		else if( TeamHasPlayers(i) )
			return false;
	}

	return (ready >= 2);
}

void Cmd_Captain_f(edict_t * ent)
{
	int teamNum;
	edict_t *oldCaptain;

	if (!matchmode->value) {
		gi.cprintf(ent, PRINT_HIGH, "This command needs matchmode to be enabled\n");
		return;
	}

	teamNum = ent->client->resp.team;
	if (teamNum == NOTEAM) {
		gi.cprintf(ent, PRINT_HIGH, "You need to be on a team for that...\n");
		return;
	}

	oldCaptain = teams[teamNum].captain;
	if (oldCaptain == ent) {
		MM_SetCaptain( teamNum, NULL );
		return;
	}

	if (oldCaptain) {
		gi.cprintf( ent, PRINT_HIGH, "Your team already has a captain\n" );
		return;
	}

	MM_SetCaptain( teamNum, ent );
}

//extern int started; // AQ2:M - Matchmode - Used for ready command
void Cmd_Ready_f(edict_t * ent)
{
	char temp[128];
	int		teamNum;
	team_t	*team;

	if (!matchmode->value) {
		gi.cprintf(ent, PRINT_HIGH, "This command needs matchmode to be enabled\n");
		return;
	}

	teamNum = ent->client->resp.team;
	if (teamNum == NOTEAM) {
		gi.cprintf( ent, PRINT_HIGH, "You need to be on a team for that...\n" );
		return;
	}

	team = &teams[teamNum];
	if (team->captain != ent) {
		gi.cprintf( ent, PRINT_HIGH, "You need to be a captain for that\n" );
		return;
	}

	if (!(gameSettings & GS_ROUNDBASED) && team_round_going) {
		if(teamdm->value)
			gi.cprintf(ent, PRINT_HIGH, "You can't unready in teamdm, use 'pausegame' instead\n");
		else
			gi.cprintf(ent, PRINT_HIGH, "You can't unready in ctf, use 'pausegame' instead\n");
		return;
	}

	team->ready = !team->ready;
	Com_sprintf( temp, sizeof( temp ), "%s %s ready to play!", team->name, (team->ready) ? "is" : "is no longer" );
	CenterPrintAll( temp );
}

void Cmd_Teamname_f(edict_t * ent)
{
	int i, argc, teamNum;
	char temp[32];
	team_t *team;

	if (!matchmode->value) {
		gi.cprintf(ent, PRINT_HIGH, "This command needs matchmode to be enabled\n");
		return;
	}

	if(ctf->value) {
		gi.cprintf(ent, PRINT_HIGH, "You can't change teamnames in ctf mode\n");
		return;
	}

	teamNum = ent->client->resp.team;
	if (teamNum == NOTEAM) {
		gi.cprintf( ent, PRINT_HIGH, "You need to be on a team for that...\n" );
		return;
	}

	team = &teams[teamNum];
	if (team->captain != ent) {
		gi.cprintf( ent, PRINT_HIGH, "You need to be a captain for that\n" );
		return;
	}

	if (team->ready) {
		gi.cprintf( ent, PRINT_HIGH, "You can't use this while 'ready'\n" );
		return;
	}

	if (team_round_going || team_game_going) {
		gi.cprintf(ent, PRINT_HIGH, "You can't use this while playing\n");
		return;
	}

	argc = gi.argc();
	if (argc < 2) {
		gi.cprintf( ent, PRINT_HIGH, "Your team name is %s\n", team->name );
		return;
	}

	Q_strncpyz(temp, gi.argv(1), sizeof(temp));
	for (i = 2; i < argc; i++) {
		Q_strncatz(temp, " ", sizeof(temp));
		Q_strncatz(temp, gi.argv(i), sizeof(temp));
	}
	temp[18] = 0;

	if (!temp[0])
		strcpy( temp, "noname" );

	gi.dprintf("%s (team %i) is now known as %s\n", team->name, teamNum, temp);
	strcpy(team->name, temp);
	gi.cprintf(ent, PRINT_HIGH, "New team name: %s\n", team->name);

}

void Cmd_Teamskin_f(edict_t * ent)
{
	char *s, newskin[32];
	int i, teamNum;
	team_t *team;
	edict_t *e;

	if (!matchmode->value) {
		gi.cprintf(ent, PRINT_HIGH, "This command needs matchmode to be enabled\n");
		return;
	}

	teamNum = ent->client->resp.team;
	if (teamNum == NOTEAM) {
		gi.cprintf(ent, PRINT_HIGH, "You need to be on a team for that...\n");
		return;
	}

	team = &teams[teamNum];
	if (team->captain != ent) {
		gi.cprintf(ent, PRINT_HIGH, "You need to be a captain for that\n");
		return;
	}
	if (team->ready) {
		gi.cprintf(ent, PRINT_HIGH, "You can't use this while 'Ready'\n");
		return;
	}
	if (team_round_going || team_game_going) {
		gi.cprintf(ent, PRINT_HIGH, "You can't use this while playing\n");
		return;
	}
	if (gi.argc() < 2) {
		gi.cprintf(ent, PRINT_HIGH, "Your team skin is %s\n", team->skin);
		return;
	}

	s = gi.argv(1);
	Q_strncpyz(newskin, s, sizeof(newskin));
	if(ctf->value) {
		s = strchr(newskin, '/');
		if(s)
			s[1] = 0;
		else
			strcpy(newskin, "male/");
		Q_strncatz(newskin, teamNum == 1 ? CTF_TEAM1_SKIN : CTF_TEAM2_SKIN, sizeof(newskin));
	}

	if (!strcmp(newskin, team->skin)) {
		gi.cprintf(ent, PRINT_HIGH, "Your team skin is already %s\n", newskin);
		return;
	}

	Q_strncpyz(team->skin, newskin, sizeof(team->skin));

	Com_sprintf(team->skin_index, sizeof(team->skin_index), "../players/%s_i", team->skin );
	level.pic_teamskin[teamNum] = gi.imageindex(team->skin_index);
	for (i = 0, e = &g_edicts[1]; i < game.maxclients; i++, e++) { //lets update players skin
		if (!e->inuse || !e->client)
			continue;

		if (e->client->resp.team == teamNum)
			AssignSkin(e, team->skin, false);
	}
	gi.cprintf(ent, PRINT_HIGH, "New team skin: %s\n", team->skin);
}

void Cmd_TeamLock_f(edict_t *ent, int a_switch)
{
	char msg[128], *s;
	int teamNum, i;
	team_t *team;

	if (!matchmode->value) {
		gi.cprintf(ent, PRINT_HIGH, "This command needs matchmode to be enabled\n");
		return;
	}

	if (!mm_allowlock->value) {
		gi.cprintf(ent, PRINT_HIGH, "Team locking is disabled on this server\n");
		return;
	}

	//Admin can lock teams
	if (ent->client->pers.admin && gi.argc() > 1)
	{
		s = gi.argv(1);
		teamNum = TP_GetTeamFromArg(s);
		if (teamNum < 1) {
			gi.cprintf(ent, PRINT_HIGH, "Unknown team '%s'.\n", s);
			return;
		}
		team = &teams[teamNum];
		if (a_switch == team->locked) {
			gi.cprintf(ent, PRINT_HIGH, "Team %s locked\n", (a_switch) ? "is already" : "isn't");
			return;
		}
		if (a_switch) {
			gclient_t *client;

			for (i = 0, client = game.clients; i < game.maxclients; i++, client++) {
				if (client->pers.connected && client->resp.team == teamNum)
					break;
			}
			if (i == game.maxclients) {
				gi.cprintf(ent, PRINT_HIGH, "You can't lock teams without players\n");
				return;
			}
		}
	}
	else
	{
		teamNum = ent->client->resp.team;
		if (teamNum == NOTEAM) {
			gi.cprintf(ent, PRINT_HIGH, "You are not on a team\n");
			return;
		}

		team = &teams[teamNum];
		if (team->captain != ent) {
			gi.cprintf(ent, PRINT_HIGH, "You are not the captain of your team\n");
			return;
		}

		if (a_switch == team->locked) {
			gi.cprintf(ent, PRINT_HIGH, "Your team %s locked\n", (a_switch) ? "is already" : "isn't");
			return;
		}
	}

	team->locked = a_switch;
	Com_sprintf( msg, sizeof( msg ), "%s is now %s", team->name, (a_switch) ? "locked" : "unlocked" );
	CenterPrintAll(msg);
}

void Cmd_SetAdmin_f (edict_t * ent)
{
	if (ent->client->pers.admin) {
		gi.cprintf( ent, PRINT_HIGH, "You are no longer a match admin.\n" );
		gi.dprintf( "%s is no longer a match admin\n", ent->client->pers.netname );
		ent->client->pers.admin = 0;
	}

	if(!matchmode->value) {
		gi.cprintf(ent, PRINT_HIGH, "Matchmode is not enabled on this server.\n");
		return;
	}

	if (strcmp( mm_adminpwd->string, "0" ) == 0) {
		gi.cprintf( ent, PRINT_HIGH, "Match admin mode is not enabled on this server..\n" );
		return;
	}

	if (gi.argc() < 2) {
		gi.cprintf (ent, PRINT_HIGH, "Usage: matchadmin <password>\n");
		return;
	}

	if (strcmp( mm_adminpwd->string, gi.argv(1) )) {
		gi.cprintf( ent, PRINT_HIGH, "Wrong password\n" );
		return;
	}

	gi.cprintf (ent, PRINT_HIGH, "You are now a match admin.\n");
	gi.dprintf ("%s is now a match admin\n", ent->client->pers.netname);
	ent->client->pers.admin = 1;
}

void Cmd_ResetScores_f(edict_t * ent)
{
	int i, teamNum, otherCaptain = 0;

	if (!matchmode->value) {
		gi.cprintf(ent, PRINT_HIGH, "This command needs matchmode to be enabled\n");
		return;
	}

	if (ent->client->pers.admin) //Admins can resetscores
	{
		ResetScores(true);
		gi.bprintf(PRINT_HIGH, "Scores and time were reset by match admin %s\n", ent->client->pers.netname);
		return;
	}

	teamNum = ent->client->resp.team;
	if (teamNum == NOTEAM) {
		gi.cprintf(ent, PRINT_HIGH, "You need to be on a team for that...\n");
		return;
	}
	if (teams[teamNum].captain != ent) {
		gi.cprintf(ent, PRINT_HIGH, "You need to be a captain for that\n");
		return;
	}

	if (teams[teamNum].wantReset)
	{
		teams[teamNum].wantReset = 0;
		for (i = TEAM1; i<teamCount + 1; i++) {
			if (i != teamNum && teams[i].captain) {
				gi.cprintf(teams[i].captain, PRINT_HIGH, "Team %i doesn't want to reset afterall", teamNum);
			}
		}
		gi.cprintf(ent, PRINT_HIGH, "Your score reset request cancelled\n");
		return;
	}

	teams[teamNum].wantReset = 1;
	for(i = TEAM1; i<teamCount+1; i++) {
		if(!teams[i].wantReset)
			break;
	}
	if(i == teamCount+1)
	{
		ResetScores(true);
		gi.bprintf(PRINT_HIGH, "Scores and time were reset by request of captains\n");
		return;
	}

	for (; i<teamCount + 1; i++) {
		if (!teams[i].wantReset && teams[i].captain) {
			gi.cprintf(teams[i].captain, PRINT_HIGH, "Team %i wants to reset scores, type 'resetscores' to accept\n", teamNum);
			otherCaptain = 1;
		}
	}
	if(otherCaptain)
		gi.cprintf(ent, PRINT_HIGH, "Your score reset request was sent to the other team captain\n");
	else
		gi.cprintf(ent, PRINT_HIGH, "Other team needs a captain and his acceptance to reset the scores\n");

}

void Cmd_TogglePause_f(edict_t * ent, qboolean pause)
{
	static int lastPaused = 0;
	int		teamNum;

	if (!matchmode->value) {
		gi.cprintf(ent, PRINT_HIGH, "This command needs matchmode to be enabled\n");
		return;
	}

	if ((int)mm_pausecount->value < 1) {
		gi.cprintf(ent, PRINT_HIGH, "Pause is disabled, mm_pausecount is 0\n");
		return;
	}

	if (mm_pausetime->value < FRAMETIME) {
		gi.cprintf( ent, PRINT_HIGH, "Pause is disabled, mm_pausetime is 0\n" );
		return;
	}

	teamNum = ent->client->resp.team;
	if (teamNum == NOTEAM) {
		gi.cprintf(ent, PRINT_HIGH, "You need to be on a team for that...\n");
		return;
	}

	if (!team_round_going) {
		gi.cprintf(ent, PRINT_HIGH, "No match running, so why pause?\n");
		//return;
	}

	if (ent->client->resp.subteam) {
		gi.cprintf(ent, PRINT_HIGH, "You can't pause when substitute\n");
		return;
	}

	if(pause)
	{
		if(level.pauseFrames > 0)
		{
			gi.cprintf(ent, PRINT_HIGH, "Game is already paused you silly\n");
			return;
		}
		if (level.intermission_framenum) {
			gi.cprintf(ent, PRINT_HIGH, "Can't pause in an intermission.\n");
			return;
		}
		if(teams[teamNum].pauses_used >= (int)mm_pausecount->value)
		{
			gi.cprintf(ent, PRINT_HIGH, "Your team doesn't have any pauses left.\n");
			return;
		}
		teams[teamNum].pauses_used++;

		CenterPrintAll (va("Game paused by %s\nTeam %i has %i pauses left", ent->client->pers.netname, ent->client->resp.team, (int)mm_pausecount->value - teams[ent->client->resp.team].pauses_used));
		level.pauseFrames = (int)(mm_pausetime->value * 60.0f * HZ);
		lastPaused = teamNum;
	}
	else
	{
		if (!level.pauseFrames)
		{
			gi.cprintf(ent, PRINT_HIGH, "Game is not paused\n");
			return;
		}
		if(!lastPaused)
		{
			gi.cprintf(ent, PRINT_HIGH, "Already unpausing\n");
			return;
		}
		if(lastPaused != teamNum)
		{
			gi.cprintf(ent, PRINT_HIGH, "You can't unpause when paused by the other team\n");
			return;
		}
		level.pauseFrames = 10 * HZ;
		lastPaused = 0;
	}
}

