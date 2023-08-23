//-----------------------------------------------------------------------------
// Statistics Related Code
//
// $Id: tng_stats.c,v 1.33 2004/05/18 20:35:45 slicerdw Exp $
//
//-----------------------------------------------------------------------------
// $Log: tng_stats.c,v $
// Revision 1.33  2004/05/18 20:35:45  slicerdw
// Fixed a bug on stats command
//
// Revision 1.32  2002/04/03 15:05:03  freud
// My indenting broke something, rolled the source back.
//
// Revision 1.30  2002/04/01 16:08:59  freud
// Fix in hits/shots counter for each weapon
//
// Revision 1.29  2002/04/01 15:30:38  freud
// Small stat fix
//
// Revision 1.28  2002/04/01 15:16:06  freud
// Stats code redone, tng_stats now much more smarter. Removed a few global
// variables regarding stats code and added kevlar hits to stats.
//
// Revision 1.27  2002/03/28 12:10:12  freud
// Removed unused variables (compiler warnings).
// Added cvar mm_allowlock.
//
// Revision 1.26  2002/03/15 19:28:36  deathwatch
// Updated with stats rifle name fix
//
// Revision 1.25  2002/02/26 23:09:20  freud
// Stats <playerid> not working, fixed.
//
// Revision 1.24  2002/02/21 23:38:39  freud
// Fix to a BAD stats bug. CRASH
//
// Revision 1.23  2002/02/18 23:47:33  freud
// Fixed FPM if time was 0
//
// Revision 1.22  2002/02/18 19:31:40  freud
// FPM fix.
//
// Revision 1.21  2002/02/18 17:21:14  freud
// Changed Knife in stats to Slashing Knife
//
// Revision 1.20  2002/02/17 21:48:56  freud
// Changed/Fixed allignment of Scoreboard
//
// Revision 1.19  2002/02/05 09:27:17  freud
// Weapon name changes and better alignment in "stats list"
//
// Revision 1.18  2002/02/03 01:07:28  freud
// more fixes with stats
//
// Revision 1.14  2002/01/24 11:29:34  ra
// Cleanup's in stats code
//
// Revision 1.13  2002/01/24 02:24:56  deathwatch
// Major update to Stats code (thanks to Freud)
// new cvars:
// stats_afterround - will display the stats after a round ends or map ends
// stats_endmap - if on (1) will display the stats scoreboard when the map ends
//
// Revision 1.12  2001/12/31 13:29:06  deathwatch
// Added revision header
//
//
//-----------------------------------------------------------------------------

#include "g_local.h"
#include <time.h>

/* Stats Command */

void ResetStats(edict_t *ent)
{
	int i;

	if(!ent->client)
		return;

	ent->client->resp.shotsTotal = 0;
	ent->client->resp.hitsTotal = 0;

	for (i = 0; i<LOC_MAX; i++)
		ent->client->resp.hitsLocations[i] = 0;

	memset(ent->client->resp.gunstats, 0, sizeof(ent->client->resp.gunstats));
}

void Stats_AddShot( edict_t *ent, int gun )
{
	if( in_warmup )
		return;

	if ((unsigned)gun >= MAX_GUNSTAT) {
		gi.dprintf( "Stats_AddShot: Bad gun number!\n" );
		return;
	}

	if (!teamplay->value || team_round_going || stats_afterround->value) {
		ent->client->resp.shotsTotal += 1;	// TNG Stats, +1 hit
		ent->client->resp.gunstats[gun].shots += 1;	// TNG Stats, +1 hit
	}
}

void Stats_AddHit( edict_t *ent, int gun, int hitPart )
{
	int headShot = (hitPart == LOC_HDAM || hitPart == LOC_KVLR_HELMET) ? 1 : 0;

	ent->client->last_damaged_part = hitPart;

	if( in_warmup )
		return;

	// Adjusted logic to be inclusive rather than exclusive
	if (((unsigned)gun <= MAX_GUNSTAT) || ((unsigned)gun == MOD_KICK) || ((unsigned)gun == MOD_PUNCH)) {

		if (!teamplay->value || team_round_going || stats_afterround->value) {
			ent->client->resp.hitsTotal++;
			ent->client->resp.gunstats[gun].hits++;
			ent->client->resp.hitsLocations[hitPart]++;

			if (headShot)
				ent->client->resp.gunstats[gun].headshots++;
		}
		if (!headShot) {
			ent->client->resp.streakHS = 0;
		}
	}
	else {
		gi.dprintf( "Stats_AddHit: Bad gun number!\n" );
		return;
	}
}


void Cmd_Stats_f (edict_t *targetent, char *arg)
{
/* Variables Used:                              *
* stats_shots_t - Total nr of Shots             *
* stats_shots_h - Total nr of Hits              *
* headshots     - Total nr of Headshots         *
*                                               */
	
	double	perc_hit;
	int		total, hits, i, y, len, locHits;
	char		*string, stathead[64];
	edict_t		*ent, *cl_ent;
	gunStats_t	*gun;

	if (!targetent->inuse)
		return;


	if (arg[0] != '\0') {
		if (strcmp (arg, "list") == 0) {
			gi.cprintf (targetent, PRINT_HIGH, "\n\x9D\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9F\n");
			gi.cprintf (targetent, PRINT_HIGH, "PlayerID  Name                  Accuracy\n");

			for (i = 0; i < game.maxclients; i++)
			{
				cl_ent = &g_edicts[1 + i];

				if (!cl_ent->inuse || cl_ent->client->pers.mvdspec)
					continue;

				hits = total = 0;
				gun = cl_ent->client->resp.gunstats;
				for (y = 0; y < MAX_GUNSTAT; y++, gun++) {
					hits += gun->hits;
					total += gun->shots;
				}

				if (total > 0)
					perc_hit = (double)hits * 100.0 / (double)total;
				else
					perc_hit = 0.0;

				gi.cprintf (targetent, PRINT_HIGH, "   %-3i    %-16s        %6.2f\n", i, cl_ent->client->pers.netname, perc_hit);
			}
			gi.cprintf (targetent, PRINT_HIGH, "\n  Use \"stats <PlayerID>\" for\n  individual stats\n\x9D\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9F\n");
			return;
		}

		ent = LookupPlayer(targetent, gi.args(), true, true);
		if (!ent)
			return;

	} else {
		ent = targetent;
	}

	// Global Stats:
	hits = total = 0;
	gun = ent->client->resp.gunstats;
	for (y = 0; y < MAX_GUNSTAT; y++, gun++) {
		hits += gun->hits;
		total += gun->shots;
	}

	sprintf(stathead, "\n\x9D\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9F Statistics for %s \x9D", ent->client->pers.netname);
	len = strlen(stathead);
	for (i = len; i < 55; i++) {
		stathead[i] = '\x9E';
	}
	stathead[i] = 0;
	strcat(stathead, "\x9F\n");

	gi.cprintf (targetent, PRINT_HIGH, "%s", stathead);

	if (!total) {
		gi.cprintf (targetent, PRINT_HIGH, "\n  Player has not fired a shot.\n");
		gi.cprintf (targetent, PRINT_HIGH, "\n\x9D\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9F\n\n");
		return;
	}

	gi.cprintf (targetent, PRINT_HIGH, "Weapon            Accuracy Hits/Shots Kills Headshots\n");		

	gun = ent->client->resp.gunstats;
	for (y = 0; y < MAX_GUNSTAT; y++, gun++) {

		if (gun->shots <= 0)
			continue;

		switch (y) {
		case MOD_MK23:
			string = "Pistol";
			break;
		case MOD_DUAL:
			string = "Dual Pistols";
			break;
		case MOD_KNIFE:
			string = "Slashing Knife";
			break;
		case MOD_KNIFE_THROWN:
			string = "Throwing Knife";
			break;
		case MOD_M4:
			string = "M4 Assault Rifle";
			break;
		case MOD_MP5:
			string = "MP5 Submachinegun";
			break;
		case MOD_SNIPER:
			string = "Sniper Rifle";
			break;
		case MOD_HC:
			string = "Handcannon";
			break;
		case MOD_M3:
			string = "M3 Shotgun";
			break;
		default:
			string = "Unknown Weapon";
			break;
		}

		perc_hit = (double)gun->hits * 100.0 / (double)gun->shots; // Percentage of shots that hit
		gi.cprintf( targetent, PRINT_HIGH, "%-17s  %6.2f  %4i/%-4i  %3i   %5i\n",
			string, perc_hit, gun->hits, gun->shots, gun->kills, gun->headshots );
	}

	gi.cprintf (targetent, PRINT_HIGH, "\n\x9D\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9F\n");


	// Final Part
	gi.cprintf (targetent, PRINT_HIGH, "Location                          Hits     (%%)\n");		

	for (y = 0; y < LOC_MAX; y++) {
		locHits = ent->client->resp.hitsLocations[y];

		if (locHits <= 0)
			continue;

		switch (y) {
		case LOC_HDAM:
			string = "Head";
			break;
		case LOC_CDAM:
			string = "Chest";
			break;
		case LOC_SDAM:
			string = "Stomach";
			break;
		case LOC_LDAM:
			string = "Legs";
			break;
		case LOC_KVLR_HELMET:
			string = "Kevlar Helmet";
			break;
		case LOC_KVLR_VEST:
			string = "Kevlar Vest";
			break;
		case LOC_NO:
			string = "Spread (Shotgun/Handcannon)";
			break;
		default:
			string = "Unknown";
			break;
		}

		perc_hit = (double)locHits * 100.0 / (double)hits;
		gi.cprintf( targetent, PRINT_HIGH, "%-27s %10i  (%6.2f)\n", string, locHits, perc_hit );
	}
	gi.cprintf (targetent, PRINT_HIGH, "\n");

	if (total > 0)
		perc_hit = (double)hits * 100.0 / (double)total;
	else
		perc_hit = 0.0;

	gi.cprintf (targetent, PRINT_HIGH, "Average Accuracy:                         %.2f\n", perc_hit); // Average
	gi.cprintf (targetent, PRINT_HIGH, "\n\x9D\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9F\n\n");
	gi.cprintf(targetent, PRINT_HIGH, "Highest streaks:  kills: %d headshots: %d\n", ent->client->resp.streakKillsHighest, ent->client->resp.streakHSHighest);
}

void A_ScoreboardEndLevel (edict_t * ent, edict_t * killer)
{
	char string[2048];
	gclient_t *sortedClients[MAX_CLIENTS], *cl;
	int maxsize = 1000, i, line_y;
	int totalClients, secs, shots;
	double accuracy, fpm;
	int totalplayers[TEAM_TOP] = {0};
	int totalscore[TEAM_TOP] = {0};
	int name_pos[TEAM_TOP] = {0};

	totalClients = G_SortedClients(sortedClients);

	ent->client->ps.stats[STAT_TEAM_HEADER] = level.pic_teamtag;

	for (i = 0; i < totalClients; i++) {
		cl = sortedClients[i];

		totalscore[cl->resp.team] += cl->resp.score;
		totalplayers[cl->resp.team]++;
	}

	for (i = TEAM1; i <= teamCount; i++) {
		name_pos[i] = ((20 - strlen(teams[i].name)) / 2) * 8;
		if (name_pos[TEAM1] < 0)
			name_pos[TEAM1] = 0;
	}


	if (teamCount == 3)
	{
		sprintf(string,
			// TEAM1
			"if 24 xv -80 yv 8 pic 24 endif "
			"if 22 xv -48 yv 8 pic 22 endif "
			"xv -48 yv 28 string \"%4d/%-3d\" "
			"xv 10 yv 12 num 2 26 "
			"xv %d yv 0 string \"%s\" ",
			totalscore[TEAM1], totalplayers[TEAM1], name_pos[TEAM1] - 80,
			teams[TEAM1].name);
		sprintf(string + strlen (string),
			// TEAM2
			"if 25 xv 80 yv 8 pic 25 endif "
			"if 22 xv 112 yv 8 pic 22 endif "
			"xv 112 yv 28 string \"%4d/%-3d\" "
			"xv 168 yv 12 num 2 27 "
			"xv %d yv 0 string \"%s\" ",
			totalscore[TEAM2], totalplayers[TEAM2], name_pos[TEAM2] + 80,
			teams[TEAM2].name);
		sprintf(string + strlen (string),
			// TEAM3
			"if 30 xv 240 yv 8 pic 30 endif "
			"if 22 xv 272 yv 8 pic 22 endif "
			"xv 272 yv 28 string \"%4d/%-3d\" "
			"xv 328 yv 12 num 2 31 "
			"xv %d yv 0 string \"%s\" ",
			totalscore[TEAM3], totalplayers[TEAM3], name_pos[TEAM3] + 240,
			teams[TEAM3].name);
	}
	else
	{
		sprintf (string,
			// TEAM1
			"if 24 xv 0 yv 8 pic 24 endif "
			"if 22 xv 32 yv 8 pic 22 endif "
			"xv 32 yv 28 string \"%4d/%-3d\" "
			"xv 90 yv 12 num 2 26 " "xv %d yv 0 string \"%s\" "
			// TEAM2
			"if 25 xv 160 yv 8 pic 25 endif "
			"if 22 xv 192 yv 8 pic 22 endif "
			"xv 192 yv 28 string \"%4d/%-3d\" "
			"xv 248 yv 12 num 2 27 "
			"xv %d yv 0 string \"%s\" ",
			totalscore[TEAM1], totalplayers[TEAM1], name_pos[TEAM1],
			teams[TEAM1].name, totalscore[TEAM2], totalplayers[TEAM2],
			name_pos[TEAM2] + 160, teams[TEAM2].name);
	}

	line_y = 56;
	sprintf(string + strlen (string),
		"xv 0 yv 40 string2 \"Frags Player          Shots   Acc   FPM \" "
		"xv 0 yv 48 string2 \"\x9D\x9E\x9E\x9E\x9F \x9D\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9F \x9D\x9E\x9E\x9E\x9F \x9D\x9E\x9E\x9E\x9F \x9D\x9E\x9E\x9E\x9F\" ");

//        strcpy (string, "xv 0 yv 32 string2 \"Frags Player          Time Ping Damage Kills\" "
//                "xv 0 yv 40 string2 \"\x9D\x9E\x9E\x9E\x9F \x9D\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9E\x9F \x9D\x9E\x9E\x9F \x9D\x9E\x9E\x9F \x9D\x9E\x9E\x9E\x9E\x9F \x9D\x9E\x9E\x9E\x9F\" ");
  /*
     {
     strcpy (string, "xv 0 yv 32 string2 \"Player          Time Ping\" "
     "xv 0 yv 40 string2 \"--------------- ---- ----\" ");
     }
     else
     {
     strcpy (string, "xv 0 yv 32 string2 \"Frags Player          Time Ping Damage Kills\" "
     "xv 0 yv 40 string2 \"----- --------------- ---- ---- ------ -----\" ");
     }
   */
  // AQ2:TNG END

	for (i = 0; i < totalClients; i++)
	{
		cl = sortedClients[i];

		if (!cl->resp.team)
			continue;

		shots = min( cl->resp.shotsTotal, 9999 );

		if (shots)
			accuracy = (double)cl->resp.hitsTotal * 100.0 / (double)cl->resp.shotsTotal;
		else
			accuracy = 0;

		secs = (level.framenum - cl->resp.enterframe) / HZ;
		if (secs > 0)
			fpm = (double)cl->resp.score * 60.0 / (double)secs;
		else
			fpm = 0.0;

		sprintf(string + strlen(string),
			"yv %d string \"%5d %-15s  %4d %5.1f  %4.1f\" ",
			line_y,
			cl->resp.score,
			cl->pers.netname, shots, accuracy, fpm );
		
		line_y += 8;

		if (strlen (string) > (maxsize - 100) && i < (totalClients - 2))
		{
			sprintf(string + strlen (string),
				"yv %d string \"..and %d more\" ",
				line_y, (totalClients - i - 1) );
			line_y += 8;
			break;
		}
	}
	
	if (strlen(string) > 1023)	// for debugging...
	{
		gi.dprintf("Warning: scoreboard string neared or exceeded max length\nDump:\n%s\n---\n", string);
		string[1023] = '\0';
	}

	gi.WriteByte(svc_layout);
	gi.WriteString(string);
}

void Cmd_Statmode_f(edict_t* ent)
{
	int i;
	char stuff[32], *arg;


	// Ignore if there is no argument.
	arg = gi.argv(1);
	if (!arg || !arg[0])
		return;

	// Numerical
	i = atoi (arg);

	if (i > 2 || i < 0) {
		gi.dprintf("Warning: stat_mode set to %i by %s\n", i, ent->client->pers.netname);

		// Force the old mode if it is valid else force 0
		if (ent->client->resp.stat_mode > 0 && ent->client->resp.stat_mode < 3)
			sprintf(stuff, "set stat_mode \"%i\"\n", ent->client->resp.stat_mode);
		else
			sprintf(stuff, "set stat_mode \"0\"\n");
	} else {
		sprintf(stuff, "set stat_mode \"%i\"\n", i);
		ent->client->resp.stat_mode = i;
	}
	stuffcmd(ent, stuff);
}
