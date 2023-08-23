//-----------------------------------------------------------------------------
// p_client.c
//
// $Id: p_client.c,v 1.90 2004/09/23 00:09:44 slicerdw Exp $
//
//-----------------------------------------------------------------------------
// $Log: p_client.c,v $
// Revision 1.90  2004/09/23 00:09:44  slicerdw
// Radio kill count was missing for falling death
//
// Revision 1.89  2004/04/08 23:19:51  slicerdw
// Optimized some code, added a couple of features and fixed minor bugs
//
// Revision 1.88  2003/10/01 19:24:14  igor_rock
// corrected a smaller bug (thanks to nopcode for the bug report)
//
// Revision 1.87  2003/06/15 21:43:53  igor
// added IRC client
//
// Revision 1.86  2003/06/15 15:34:32  igor
// - removed the zcam code from this branch (see other branch)
// - added fixes from 2.72 (source only) version
// - resetted version number to 2.72
// - This version should be exactly like the release 2.72 - just with a few
//   more fixes (which whoever did the source only variant didn't get because
//   he didn't use the CVS as he should. Shame on him.
//
// Revision 1.85  2003/02/10 02:12:25  ra
// Zcam fixes, kick crashbug in CTF fixed and some code cleanup.
//
// Revision 1.84  2002/12/31 17:07:22  igor_rock
// - corrected the Add_Ammo function to regard wp_flags
//
// Revision 1.83  2002/12/30 12:58:16  igor_rock
// - Corrected some comments (now it looks better)
// - allweapon mode now recognizes wp_flags
//
// Revision 1.82  2002/09/04 11:23:10  ra
// Added zcam to TNG and bumped version to 3.0
//
// Revision 1.81  2002/04/01 15:16:06  freud
// Stats code redone, tng_stats now much more smarter. Removed a few global
// variables regarding stats code and added kevlar hits to stats.
//
// Revision 1.80  2002/03/28 13:30:36  freud
// Included time played in ghost.
//
// Revision 1.79  2002/03/28 12:10:11  freud
// Removed unused variables (compiler warnings).
// Added cvar mm_allowlock.
//
// Revision 1.78  2002/03/25 23:35:19  freud
// Ghost code, use_ghosts and more stuff..
//
// Revision 1.77  2002/03/25 18:57:36  freud
// Added maximum number of stored player sessions (ghosts)
//
// Revision 1.76  2002/03/25 18:32:11  freud
// I'm being too productive.. New ghost command needs testing.
//
// Revision 1.75  2002/02/23 18:33:52  freud
// Fixed newline bug with announcer (EXCELLENT.. 1 FRAG LEFT) for logfiles
//
// Revision 1.74  2002/02/23 18:12:14  freud
// Added newlines back to the CenterPrintAll for IMPRESSIVE, EXCELLENT,
// ACCURACY and X FRAGS Left, it was screwing up the logfile.
//
// Revision 1.73  2002/02/19 09:32:47  freud
// Removed PING PONGs from CVS, not fit for release.
//
// Revision 1.72  2002/02/18 23:38:05  freud
// PING PONG..
//
// Revision 1.71  2002/02/18 23:25:42  freud
// More tweaks
//
// Revision 1.70  2002/02/18 20:21:36  freud
// Added PING PONG mechanism for timely disconnection of clients. This is
// based on a similar scheme as the scheme used by IRC. The client has
// cvar ping_timeout seconds to reply or will be disconnected.
//
// Revision 1.69  2002/02/18 18:25:51  ra
// Bumped version to 2.6, fixed ctf falling and kicking of players in ctf
// uvtime
//
// Revision 1.68  2002/02/18 17:17:21  freud
// Fixed the CTF leaving team bug. Also made the shield more efficient,
// No falling damage.
//
// Revision 1.67  2002/02/18 13:55:35  freud
// Added last damaged players %P
//
// Revision 1.66  2002/02/17 20:10:09  freud
// Better naming of auto_items is auto_equip, requested by Deathwatch.
//
// Revision 1.65  2002/02/17 20:01:32  freud
// Fixed stat_mode overflows, finally.
// Added 2 new cvars:
//      auto_join (0|1), enables auto joining teams from previous map.
//      auto_items (0|1), enables weapon and items caching between maps.
//
// Revision 1.64  2002/02/17 19:04:15  freud
// Possible bugfix for overflowing clients with stat_mode set.
//
// Revision 1.63  2002/02/01 16:09:49  freud
// Fixed the Taught how to fly bug
//
// Revision 1.62  2002/02/01 14:29:18  ra
// Attempting to fix tought how to fly no frag bug
//
// Revision 1.61  2002/01/31 11:15:06  freud
// Fix for crashes with stat_mode, not sure it works.
//
// Revision 1.60  2002/01/24 11:38:01  ra
// Cleanups
//
// Revision 1.59  2002/01/23 13:08:32  ra
// fixing tought how to fly bug
//
// Revision 1.58  2001/12/24 18:06:05  slicerdw
// changed dynamic check for darkmatch only
//
// Revision 1.56  2001/12/23 16:30:50  ra
// 2.5 ready. New stats from Freud. HC and shotgun gibbing seperated.
//
// Revision 1.55  2001/12/09 14:02:11  slicerdw
// Added gl_clear check -> video_check_glclear cvar
//
// Revision 1.54  2001/11/29 17:58:31  igor_rock
// TNG IRC Bot - First Version
//
// Revision 1.53  2001/11/08 20:56:24  igor_rock
// - changed some things related to wp_flags
// - corrected use_punch bug when player only has an empty weapon left
//
// Revision 1.52  2001/09/28 16:24:20  deathwatch
// use_rewards now silences the teamX wins sounds and added gibbing for the Shotgun
//
// Revision 1.51  2001/09/28 13:48:35  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.50  2001/09/28 13:44:23  slicerdw
// Several Changes / improvements
//
// Revision 1.49  2001/09/05 14:33:57  slicerdw
// Added Fix's from the 2.1 release
//
// Revision 1.48  2001/09/03 14:25:00  deathwatch
// Added gibbing with HC (only happens rarely) when sv_gib is on and cleaned up
// the player_die function and made sure the flashlight gets turned off when someone
// is dead.
//
// Revision 1.47  2001/09/02 20:33:34  deathwatch
// Added use_classic and fixed an issue with ff_afterround, also updated version
// nr and cleaned up some commands.
//
// Updated the VC Project to output the release build correctly.
//
// Revision 1.46  2001/08/19 01:22:25  deathwatch
// cleaned the formatting of some files
//
// Revision 1.45  2001/08/17 21:31:37  deathwatch
// Added support for stats
//
// Revision 1.44  2001/08/15 14:50:48  slicerdw
// Added Flood protections to Radio & Voice, Fixed the sniper bug AGAIN
//
// Revision 1.43  2001/08/08 12:42:22  slicerdw
// Ctf Should finnaly be fixed now, lets hope so
//
// Revision 1.42  2001/08/06 23:35:31  ra
// Fixed an uvtime bug when clients join server while CTF rounds are already
// going.
//
// Revision 1.41  2001/08/06 14:38:45  ra
// Adding UVtime for ctf
//
// Revision 1.40  2001/08/06 13:41:41  slicerdw
// Added a fix for ctf..
//
// Revision 1.39  2001/08/06 03:00:49  ra
// Added FF after rounds. Please someone look at the EVIL if statments for me :)
//
// Revision 1.38  2001/08/03 15:08:32  ra
// Fix small bug in %K related to "tought how to fly" deaths.
//
// Revision 1.37  2001/07/20 11:56:04  slicerdw
// Added a check for the players spawning during countdown on ctf ( lets hope it works )
//
// Revision 1.36  2001/06/27 16:58:14  igor_rock
// corrected some limchasecam bugs
//
// Revision 1.35  2001/06/26 18:47:30  igor_rock
// added ctf_respawn cvar
//
// Revision 1.34  2001/06/25 11:44:47  slicerdw
// New Video Check System - video_check and video_check_lockpvs no longer latched
//
// Revision 1.33  2001/06/22 20:35:07  igor_rock
// fixed the flying corpse bug
//
// Revision 1.32  2001/06/22 18:37:01  igor_rock
// fixed than damn limchasecam bug - eentually :)
//
// Revision 1.31  2001/06/21 07:37:10  igor_rock
// fixed some limchasecam bugs
//
// Revision 1.30  2001/06/21 00:05:30  slicerdw
// New Video Check System done -  might need some revision but works..
//
// Revision 1.27  2001/06/20 07:21:21  igor_rock
// added use_warnings to enable/disable time/frags left msgs
// added use_rewards to enable/disable eimpressive, excellent and accuracy msgs
// change the configfile prefix for modes to "mode_" instead "../mode-" because
// they don't have to be in the q2 dir for doewnload protection (action dir is sufficient)
// and the "-" is bad in filenames because of linux command line parameters start with "-"
//
// Revision 1.26  2001/06/19 21:35:54  igor_rock
// If you select Sniper, sniper is your startweapon now.
//
// Revision 1.25  2001/06/19 21:10:05  igor_rock
// changed the "is now known" message to the normal namelimit
//
// Revision 1.24  2001/06/19 18:56:38  deathwatch
// New Last killed target system
//
// Revision 1.23  2001/06/18 18:14:09  igor_rock
// corrected bug with team none players shooting and flying around
//
// Revision 1.22  2001/06/01 19:18:42  slicerdw
// Added Matchmode Code
//
// Revision 1.21  2001/05/31 16:58:14  igor_rock
// conflicts resolved
//
// Revision 1.20  2001/05/20 15:00:19  slicerdw
// Some minor fixes and changings on Video Checking system
//
// Revision 1.19.2.3  2001/05/25 18:59:52  igor_rock
// Added CTF Mode completly :)
// Support for .flg files is still missing, but with "real" CTF maps like
// tq2gtd1 the ctf works fine.
// (I hope that all other modes still work, just tested DM and teamplay)
//
// Revision 1.19.2.2  2001/05/20 18:54:19  igor_rock
// added original ctf code snippets from zoid. lib compilesand runs but
// doesn't function the right way.
// Jsut committing these to have a base to return to if something wents
// awfully wrong.
//
// Revision 1.19.2.1  2001/05/20 15:17:31  igor_rock
// removed the old ctf code completly
//
// Revision 1.19  2001/05/20 12:54:18  igor_rock
// Removed newlines from Centered Messages like "Impressive"
//
// Revision 1.18  2001/05/16 13:26:38  slicerdw
// Too Many Userinfo Cvars( commented some) & Enabled death messages on CTF
//
// Revision 1.17  2001/05/13 15:01:45  ra
//
//
// In teamplay mode it is OK to teamkill as soon as the rounds are over. Changed
// things so that when the rounds are over it is also OK to plummet.
//
// Revision 1.16  2001/05/13 01:23:01  deathwatch
// Added Single Barreled Handcannon mode, made the menus and scoreboards
// look nicer and made the voice command a bit less loud.
//
// Revision 1.14  2001/05/12 13:51:20  mort
// Fixed ClientObituary Add_Frag bug in CTF
//
// Revision 1.13  2001/05/12 13:48:58  mort
// Fixed CTF ForceSpawn bug
//
// Revision 1.12  2001/05/12 13:37:38  mort
// Fixed CTF bug, god mode is now on when players spawn
//
// Revision 1.11  2001/05/12 10:50:16  slicerdw
// Fixed That Transparent List Thingy
//
// Revision 1.10  2001/05/11 16:07:26  mort
// Various CTF bits and pieces...
//
// Revision 1.9  2001/05/11 12:21:19  slicerdw
// Commented old Location support ( ADF ) With the ML/ETE Compatible one
//
// Revision 1.7  2001/05/07 21:18:35  slicerdw
// Added Video Checking System
//
// Revision 1.6  2001/05/07 20:06:45  igor_rock
// changed sound dir from sound/rock to sound/tng
//
// Revision 1.5  2001/05/07 02:05:36  ra
//
//
// Added tkok command to forgive teamkills.
//
// Revision 1.4  2001/05/07 01:44:07  ra
//
//
// Add a fix for the $$ skin crashing the server.
//
// Revision 1.3  2001/05/06 20:29:21  ra
//
//
// Adding comments to the limchasecam fix.
//
// Revision 1.2  2001/05/06 20:20:49  ra
//
//
// Fixing limchasecam.
//
// Revision 1.1.1.1  2001/05/06 17:29:49  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#include "g_local.h"
#include "m_player.h"
#include "cgf_sfx_glass.h"


static void FreeClientEdicts(gclient_t *client)
{
	//remove lasersight
	if (client->lasersight) {
		G_FreeEdict(client->lasersight);
		client->lasersight = NULL;
	}

	//Turn Flashlight off
	if (client->flashlight) {
		G_FreeEdict(client->flashlight);
		client->flashlight = NULL;
	}

	//Remove grapple
	if (client->ctf_grapple) {
		G_FreeEdict(client->ctf_grapple);
		client->ctf_grapple = NULL;
	}

}

void Announce_Reward(edict_t *ent, int rewardType){
	char buf[256];

	if (rewardType == IMPRESSIVE) {
		sprintf(buf, "IMPRESSIVE %s!", ent->client->pers.netname);
		CenterPrintAll(buf);
		gi.sound(&g_edicts[0], CHAN_VOICE | CHAN_NO_PHS_ADD, gi.soundindex("tng/impressive.wav"), 1.0, ATTN_NONE, 0.0);
	} else if (rewardType == EXCELLENT) {
		sprintf(buf, "EXCELLENT %s (%dx)!", ent->client->pers.netname,ent->client->resp.streakKills/12);
		CenterPrintAll(buf);
		gi.sound(&g_edicts[0], CHAN_VOICE | CHAN_NO_PHS_ADD, gi.soundindex("tng/excellent.wav"), 1.0, ATTN_NONE, 0.0);
	} else if (rewardType == ACCURACY) {
		sprintf(buf, "ACCURACY %s!", ent->client->pers.netname);
		CenterPrintAll(buf);
		gi.sound(&g_edicts[0], CHAN_VOICE | CHAN_NO_PHS_ADD, gi.soundindex("tng/accuracy.wav"), 1.0, ATTN_NONE, 0.0);
	}
}

void Add_Frag(edict_t * ent, int mod)
{
	int frags = 0;

	if (in_warmup)
		return;

	ent->client->resp.kills++;
	// All normal weapon damage
	if (mod > 0 && mod < MAX_GUNSTAT) {
		ent->client->resp.gunstats[mod].kills++;
	}
	// Grenade splash, kicks and punch damage
	if (mod > 0 && ((mod == MOD_HG_SPLASH) || (mod == MOD_KICK) || (mod == MOD_PUNCH))) {
		ent->client->resp.gunstats[mod].kills++;
	}

	if (IS_ALIVE(ent))
	{
		ent->client->resp.streakKills++;
		if (ent->client->resp.streakKills > ent->client->resp.streakKillsHighest)
			ent->client->resp.streakKillsHighest = ent->client->resp.streakKills;

		if (ent->client->resp.streakKills % 5 == 0 && use_rewards->value)
		{
			Announce_Reward(ent, IMPRESSIVE);
		}
		else if (ent->client->resp.streakKills % 12 == 0 && use_rewards->value)
		{
			Announce_Reward(ent, EXCELLENT);
		}
	}

	// Regular frag for teamplay/matchmode
	if (teamplay->value && teamdm->value != 2)
		ent->client->resp.score++;	// just 1 normal kill

	// Increment team score if TeamDM is enabled
	if(teamdm->value)
		teams[ent->client->resp.team].score++;

	// Streak kill rewards in Deathmatch mode
	if (deathmatch->value && !teamplay->value) {
		if (ent->client->resp.streakKills < 4 || ! use_rewards->value)
			frags = 1;
		else if (ent->client->resp.streakKills < 8)
			frags = 2;
		else if (ent->client->resp.streakKills < 16)
			frags = 4;
		else if (ent->client->resp.streakKills < 32)
			frags = 8;
		else
			frags = 16;

		if(frags > 1)
		{
			gi.bprintf(PRINT_MEDIUM,
				"%s has %d kills in a row and receives %d frags for the kill!\n",
				ent->client->pers.netname, ent->client->resp.streakKills, frags );
		}
		ent->client->resp.score += frags;

		// Award team with appropriate streak reward count
		if(teamdm->value)
			teams[ent->client->resp.team].score += frags;

		// AQ:TNG Igor[Rock] changing sound dir
		if (fraglimit->value && use_warnings->value) {
			if (ent->client->resp.score == fraglimit->value - 1) {
				if (fragwarning < 3) {
					CenterPrintAll("1 FRAG LEFT...");
					gi.sound(&g_edicts[0], CHAN_VOICE | CHAN_NO_PHS_ADD,
						gi.soundindex("tng/1_frag.wav"), 1.0, ATTN_NONE, 0.0);
					fragwarning = 3;
				}
			} else if (ent->client->resp.score == fraglimit->value - 2) {
				if (fragwarning < 2) {
					CenterPrintAll("2 FRAGS LEFT...");
					gi.sound(&g_edicts[0], CHAN_VOICE | CHAN_NO_PHS_ADD,
						gi.soundindex("tng/2_frags.wav"), 1.0, ATTN_NONE, 0.0);
					fragwarning = 2;
				}
			} else if (ent->client->resp.score == fraglimit->value - 3) {
				if (fragwarning < 1) {
					CenterPrintAll("3 FRAGS LEFT...");
					gi.sound(&g_edicts[0], CHAN_VOICE | CHAN_NO_PHS_ADD,
						gi.soundindex("tng/3_frags.wav"), 1.0, ATTN_NONE, 0.0);
					fragwarning = 1;
				}
			}
		}
		// end of changing sound dir
	}

	// Announce kill streak to player if use_killcounts is enabled on server
	if (use_killcounts->value) {
		if(ent->client->resp.streakKills)
			gi.cprintf(ent, PRINT_HIGH, "Kill count: %d\n", ent->client->resp.streakKills);
	}
}

void Subtract_Frag(edict_t * ent)
{
	if( in_warmup )
		return;

	ent->client->resp.kills--;
	ent->client->resp.score--;
	ent->client->resp.streakKills = 0;
	if(teamdm->value)
		teams[ent->client->resp.team].score--;
}

void Add_Death( edict_t *ent, qboolean end_streak )
{
	if( in_warmup )
		return;

	ent->client->resp.deaths ++;
	if( end_streak )
		ent->client->resp.streakKills = 0;
}

// FRIENDLY FIRE functions

void Add_TeamWound(edict_t * attacker, edict_t * victim, int mod)
{
	if (!teamplay->value || !attacker->client || !victim->client) {
		return;
	}

	attacker->client->resp.team_wounds++;

	// Warn both parties that they are teammates. Since shotguns are pellet based,
	// make sure we don't overflow the client when using MOD_HC or MOD_SHOTGUN. The
	// ff_warning flag should have been reset before each attack.
	if (attacker->client->ff_warning == 0) {
		attacker->client->ff_warning++;
		gi.cprintf(victim, PRINT_HIGH, "You were hit by %s, your TEAMMATE!\n", attacker->client->pers.netname);
		gi.cprintf(attacker, PRINT_HIGH, "You hit your TEAMMATE %s!\n", victim->client->pers.netname);
	}
	// We want team_wounds to increment by one for each ATTACK, not after each 
	// bullet or pellet does damage. With the HAND CANNON this means 2 attacks
	// since it is double barreled and we don't want to go into p_weapon.c...
	attacker->client->resp.team_wounds = (attacker->client->team_wounds_before + 1);

	// If count is less than MAX_TEAMKILLS*3, return. If count is greater than
	// MAX_TEAMKILLS*3 but less than MAX_TEAMKILLS*4, print off a ban warning. If
	// count equal (or greater than) MAX_TEAMKILLS*4, ban and kick the client.
	if ((int) maxteamkills->value < 1)	//FB
		return;
	if (attacker->client->resp.team_wounds < ((int)maxteamkills->value * 3)) {
		return;
	} else if (attacker->client->resp.team_wounds < ((int) maxteamkills->value * 4)) {
		// Print a note to console, and issue a warning to the player.
		gi.cprintf(NULL, PRINT_MEDIUM,
			   "%s is in danger of being banned for wounding teammates\n", attacker->client->pers.netname);
		gi.cprintf(attacker, PRINT_HIGH,
			   "WARNING: You'll be temporarily banned if you continue wounding teammates!\n");
		return;
	} else {
		if (attacker->client->pers.ip[0]) {
			if (Ban_TeamKiller(attacker, (int) twbanrounds->value)) {
				gi.cprintf(NULL, PRINT_MEDIUM,
					   "Banning %s@%s for team wounding\n",
					   attacker->client->pers.netname, attacker->client->pers.ip);

				gi.cprintf(attacker, PRINT_HIGH,
					   "You've wounded teammates too many times, and are banned for %d %s.\n",
					   (int) twbanrounds->value,
					   (((int) twbanrounds->value > 1) ? "games" : "game"));
			} else {
				gi.cprintf(NULL, PRINT_MEDIUM,
					   "Error banning %s: unable to get ip address\n", attacker->client->pers.netname);
			}
			Kick_Client(attacker);
		}
	}

	return;
}

void Add_TeamKill(edict_t * attacker)
{
	if (!teamplay->value || !attacker->client || !team_round_going) {
		return;
	}

	attacker->client->resp.team_kills++;
	// Because the stricter team kill was incremented, lower team_wounds
	// by amount inflicted in last attack (i.e., no double penalty).
	if (attacker->client->resp.team_wounds > attacker->client->team_wounds_before) {
		attacker->client->resp.team_wounds = attacker->client->team_wounds_before;
	}
	// If count is less than 1/2 MAX_TEAMKILLS, print off simple warning. If
	// count is greater than 1/2 MAX_TEAMKILLS but less than MAX_TEAMKILLS,
	// print off a ban warning. If count equal or greater than MAX_TEAMKILLS,
	// ban and kick the client.
	if (((int) maxteamkills->value < 1) ||
		(attacker->client->resp.team_kills < (((int)maxteamkills->value % 2) + (int)maxteamkills->value / 2))) {
		gi.cprintf(attacker, PRINT_HIGH, "You killed your TEAMMATE!\n");
		return;
	} else if (attacker->client->resp.team_kills < (int) maxteamkills->value) {
		// Show this on the console
		gi.cprintf(NULL, PRINT_MEDIUM,
			   "%s is in danger of being banned for killing teammates\n", attacker->client->pers.netname);
		// Issue a warning to the player
		gi.cprintf(attacker, PRINT_HIGH, "WARNING: You'll be banned if you continue killing teammates!\n");
		return;
	} else {
		// They've killed too many teammates this game - kick 'em for a while
		if (attacker->client->pers.ip[0]) {
			if (Ban_TeamKiller(attacker, (int) tkbanrounds->value)) {
				gi.cprintf(NULL, PRINT_MEDIUM,
					   "Banning %s@%s for team killing\n",
					   attacker->client->pers.netname, attacker->client->pers.ip);
				gi.cprintf(attacker, PRINT_HIGH,
					   "You've killed too many teammates, and are banned for %d %s.\n",
					   (int) tkbanrounds->value,
					   (((int) tkbanrounds->value > 1) ? "games" : "game"));
			} else {
				gi.cprintf(NULL, PRINT_MEDIUM,
					   "Error banning %s: unable to get ip address\n", attacker->client->pers.netname);
			}
		}
		Kick_Client(attacker);
	}
}

// FRIENDLY FIRE

//
// Gross, ugly, disgustuing hack section
//

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
The normal starting point for a level.
*/
void SP_info_player_start( edict_t * self )
{
}

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for deathmatch games
*/
void SP_info_player_deathmatch(edict_t * self)
{
	SP_misc_teleporter_dest(self);
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The deathmatch intermission point will be at one of these
Use 'angles' instead of 'angle', so you can set pitch or roll as well as yaw.  'pitch yaw roll'
*/
void SP_info_player_intermission(void)
{
}

//=======================================================================

void player_pain(edict_t * self, edict_t * other, float kick, int damage)
{
	// player pain is handled at the end of the frame in P_DamageFeedback
}

// ^^^

// PrintDeathMessage: moved the actual printing of the death messages to here, to handle
//  the fact that live players shouldn't receive them in teamplay.  -FB
void PrintDeathMessage(char *msg, edict_t * gibee)
{
	int j;
	edict_t *other;

	if (!teamplay->value || in_warmup) {
		gi.bprintf(PRINT_MEDIUM, "%s", msg);
		return;
	}

	if (dedicated->value)
		gi.cprintf(NULL, PRINT_MEDIUM, "%s", msg);

	// First, let's print the message for gibee and its attacker. -TempFile
	gi.cprintf(gibee, PRINT_MEDIUM, "%s", msg);
	if (gibee->client->attacker && gibee->client->attacker != gibee)
		gi.cprintf(gibee->client->attacker, PRINT_MEDIUM, "%s", msg);

	if(!team_round_going)
		return;

	for (j = 1; j <= game.maxclients; j++) {
		other = &g_edicts[j];
		if (!other->inuse || !other->client)
			continue;

		// only print if he's NOT gibee, NOT attacker, and NOT alive! -TempFile
		if (other != gibee && other != gibee->client->attacker && other->solid == SOLID_NOT)
			gi.cprintf(other, PRINT_MEDIUM, "%s", msg);
	}
}

void ClientObituary(edict_t * self, edict_t * inflictor, edict_t * attacker)
{
	int mod;
	int loc;
	char *message;
	char *message2;
	char death_msg[1024];	// enough in all situations? -FB
	qboolean friendlyFire;
	char *special_message = NULL;
	int n;

	self->client->resp.ctf_capstreak = 0;

	friendlyFire = meansOfDeath & MOD_FRIENDLY_FIRE;
	mod = meansOfDeath & ~MOD_FRIENDLY_FIRE;
	loc = locOfDeath;	// useful for location based hits
	message = NULL;
	message2 = "";

	// Reki: Print killfeed to spectators who ask for easily parsable stuff
	edict_t *other;
	int j;
	for (j = 1; j <= game.maxclients; j++) {
		other = &g_edicts[j];
		if (!other->inuse || !other->client || !teamplay->value)
			continue;

		if (other->client->resp.team) // we only want team 0 (spectators)
			continue;

		if (!(other->client->pers.spec_flags & SPECFL_KILLFEED)) // only print to spectators who want it
			continue;

		if (attacker == world || !attacker->client)
			sprintf(death_msg, "--KF %i %s, MOD %i\n",
				self->client->resp.team, self->client->pers.netname, mod);
		else
			sprintf(death_msg, "--KF %i %s, MOD %i, %i %s\n",
				attacker->client->resp.team, attacker->client->pers.netname, mod, self->client->resp.team, self->client->pers.netname);
		gi.cprintf(other, PRINT_MEDIUM, "%s", death_msg);
	}
	//

	if (attacker == self)
	{
		switch (mod) {
		case MOD_HELD_GRENADE:
			message = "tried to put the pin back in";
			break;
		case MOD_HG_SPLASH:
			if (self->client->pers.gender == GENDER_MALE)
				message = "didn't throw his grenade far enough";
			else if (self->client->pers.gender == GENDER_FEMALE)
				message = "didn't throw her grenade far enough";
			else
				message = "didn't throw its grenade far enough";
			break;
		case MOD_G_SPLASH:
			if (self->client->pers.gender == GENDER_MALE)
				message = "tripped on his own grenade";
			else if (self->client->pers.gender == GENDER_FEMALE)
				message = "tripped on her own grenade";
			else
				message = "tripped on its own grenade";
			break;
		default:
			if (self->client->pers.gender == GENDER_MALE)
				message = "killed himself";
			else if (self->client->pers.gender == GENDER_FEMALE)
				message = "killed herself";
			else
				message = "killed itself";
			break;
		}
	}

	if (!message) {
		switch (mod) {
		case MOD_BREAKINGGLASS:
			if( self->client->push_timeout > 40 )
				special_message = "was thrown through a window by";
			message = "ate too much glass";
			break;
		case MOD_SUICIDE:
			message = "is done with the world";
			break;
		case MOD_FALLING:
			if( self->client->push_timeout )
				special_message = "was taught how to fly by";
			//message = "hit the ground hard, real hard";
			if (self->client->pers.gender == GENDER_MALE)
				message = "plummets to his death";
			else if (self->client->pers.gender == GENDER_FEMALE)
				message = "plummets to her death";
			else
				message = "plummets to its death";
			break;
		case MOD_CRUSH:
			message = "was flattened";
			break;
		case MOD_WATER:
			message = "sank like a rock";
			break;
		case MOD_SLIME:
			if( self->client->push_timeout )
				special_message = "melted thanks to";
			message = "melted";
			break;
		case MOD_LAVA:
			if( self->client->push_timeout )
				special_message = "was drop-kicked into the lava by";
			message = "does a back flip into the lava";
			break;
		case MOD_EXPLOSIVE:
		case MOD_BARREL:
			message = "blew up";
			break;
		case MOD_EXIT:
			message = "found a way out";
			break;
		case MOD_TARGET_LASER:
			message = "saw the light";
			break;
		case MOD_TARGET_BLASTER:
			message = "got blasted";
			break;
		case MOD_BOMB:
		case MOD_SPLASH:
		case MOD_TRIGGER_HURT:
			if( self->client->push_timeout )
				special_message = "was shoved off the edge by";
			message = "was in the wrong place";
			break;
		}
	}

	if (message)
	{
		// handle falling with an attacker set
		if (special_message && self->client->attacker && self->client->attacker->client
		&& (self->client->attacker->client != self->client))
		{
			sprintf(death_msg, "%s %s %s\n",
				self->client->pers.netname, special_message, self->client->attacker->client->pers.netname);
			PrintDeathMessage(death_msg, self);
			AddKilledPlayer(self->client->attacker, self);

			self->client->attacker->client->radio_num_kills++;

			//MODIFIED FOR FF -FB
			if (OnSameTeam(self, self->client->attacker))
			{
				if (!DMFLAGS(DF_NO_FRIENDLY_FIRE) && (!teamplay->value || team_round_going || !ff_afterround->value)) {
					self->enemy = self->client->attacker;
					Add_TeamKill(self->client->attacker);
					Subtract_Frag(self->client->attacker);	//attacker->client->resp.score--;
					Add_Death( self, false );
				}
			}
			else
			{
				Add_Frag(self->client->attacker, MOD_UNKNOWN);
				Add_Death( self, true );
			}

		}
		else
		{
			sprintf( death_msg, "%s %s\n", self->client->pers.netname, message );
			PrintDeathMessage(death_msg, self );

			if (!teamplay->value || team_round_going || !ff_afterround->value)  {
				Subtract_Frag( self );
				Add_Death( self, true );
			}

			self->enemy = NULL;
		}
		return;
	}
#if 0
		// handle bleeding, not used because bleeding doesn't get set
		if (mod == MOD_BLEEDING) {
			sprintf(death_msg, "%s bleeds to death\n", self->client->pers.netname);
			PrintDeathMessage(death_msg, self);
			return;
		}
#endif

	self->enemy = attacker;
	if (attacker && attacker->client)
	{
		switch (mod) {
		case MOD_MK23:	// zucc
			switch (loc) {
			case LOC_HDAM:
				if (self->client->pers.gender == GENDER_MALE)
					message = " has a hole in his head from";
				else if (self->client->pers.gender == GENDER_FEMALE)
					message = " has a hole in her head from";
				else
					message = " has a hole in its head from";
				message2 = "'s Mark 23 pistol";
				break;
			case LOC_CDAM:
				message = " loses a vital chest organ thanks to";
				message2 = "'s Mark 23 pistol";
				break;
			case LOC_SDAM:
				if (self->client->pers.gender == GENDER_MALE)
					message = " loses his lunch to";
				else if (self->client->pers.gender == GENDER_FEMALE)
					message = " loses her lunch to";
				else
					message = " loses its lunch to";
				message2 = "'s .45 caliber pistol round";
				break;
			case LOC_LDAM:
				message = " is legless because of";
				message2 = "'s .45 caliber pistol round";
				break;
			default:
				message = " was shot by";
				message2 = "'s Mark 23 Pistol";
			}
			break;
		case MOD_MP5:
			switch (loc) {
			case LOC_HDAM:
				message = "'s brains are on the wall thanks to";
				message2 = "'s 10mm MP5/10 round";
				break;
			case LOC_CDAM:
				message = " feels some chest pain via";
				message2 = "'s MP5/10 Submachinegun";
				break;
			case LOC_SDAM:
				message = " needs some Pepto Bismol after";
				message2 = "'s 10mm MP5 round";
				break;
			case LOC_LDAM:
				if (self->client->pers.gender == GENDER_MALE)
					message = " had his legs blown off thanks to";
				else if (self->client->pers.gender == GENDER_FEMALE)
					message = " had her legs blown off thanks to";
				else
					message = " had its legs blown off thanks to";
				message2 = "'s MP5/10 Submachinegun";
				break;
			default:
				message = " was shot by";
				message2 = "'s MP5/10 Submachinegun";
			}
			break;
		case MOD_M4:
			switch (loc) {
			case LOC_HDAM:
				message = " had a makeover by";
				message2 = "'s M4 Assault Rifle";
				break;
			case LOC_CDAM:
				message = " feels some heart burn thanks to";
				message2 = "'s M4 Assault Rifle";
				break;
			case LOC_SDAM:
				message = " has an upset stomach thanks to";
				message2 = "'s M4 Assault Rifle";
				break;
			case LOC_LDAM:
				message = " is now shorter thanks to";
				message2 = "'s M4 Assault Rifle";
				break;
			default:
				message = " was shot by";
				message2 = "'s M4 Assault Rifle";
			}
			break;
		case MOD_M3:
			n = rand() % 2 + 1;
			if (n == 1) {
				message = " accepts";
				message2 = "'s M3 Super 90 Assault Shotgun in hole-y matrimony";
			} else {
				message = " is full of buckshot from";
				message2 = "'s M3 Super 90 Assault Shotgun";
			}
			break;
		case MOD_HC:
			n = rand() % 3 + 1;
			if (n == 1) {
				if (attacker->client->pers.hc_mode)	// AQ2:TNG Deathwatch - Single Barreled HC Death Messages
				{
					message = " underestimated";
					message2 = "'s single barreled handcannon shot";
				} else {
					message = " ate";
					message2 = "'s sawed-off 12 gauge";
				}
			} else if (n == 2 ){
				if (attacker->client->pers.hc_mode)	// AQ2:TNG Deathwatch - Single Barreled HC Death Messages
				{
					message = " won't be able to pass a metal detector anymore thanks to";
					message2 = "'s single barreled handcannon shot";
				} else {
					message = " is full of buckshot from";
					message2 = "'s sawed off shotgun";
				} 
			} else {
				// minch <3
				message = " was minched by";
			}
			break;
		case MOD_SNIPER:
			switch (loc) {
			case LOC_HDAM:
				if (self->client->ps.fov < 90) {
					if (self->client->pers.gender == GENDER_MALE)
						message = " saw the sniper bullet go through his scope thanks to";
					else if (self->client->pers.gender == GENDER_FEMALE)
						message = " saw the sniper bullet go through her scope thanks to";
					else
						message = " saw the sniper bullet go through its scope thanks to";
				} else
					message = " caught a sniper bullet between the eyes from";
				break;
			case LOC_CDAM:
				message = " was picked off by";
				break;
			case LOC_SDAM:
				message = " was sniped in the stomach by";
				break;
			case LOC_LDAM:
				message = " was shot in the legs by";
				break;
			default:
				message = " was sniped by";
				//message2 = "'s Sniper Rifle";
			}
			break;
		case MOD_DUAL:
			switch (loc) {
			case LOC_HDAM:
				message = " was trepanned by";
				message2 = "'s akimbo Mark 23 pistols";
				break;
			case LOC_CDAM:
				message = " was John Woo'd by";
				//message2 = "'s .45 caliber pistol round";
				break;
			case LOC_SDAM:
				message = " needs some new kidneys thanks to";
				message2 = "'s akimbo Mark 23 pistols";
				break;
			case LOC_LDAM:
				message = " was shot in the legs by";
				message2 = "'s akimbo Mark 23 pistols";
				break;
			default:
				message = " was shot by";
				message2 = "'s pair of Mark 23 Pistols";
			}
			break;
		case MOD_KNIFE:
			switch (loc) {
			case LOC_HDAM:
				if (self->client->pers.gender == GENDER_MALE)
					message = " had his throat slit by";
				else if (self->client->pers.gender == GENDER_FEMALE)
					message = " had her throat slit by";
				else
					message = " had its throat slit by";
				break;
			case LOC_CDAM:
				message = " had open heart surgery, compliments of";
				break;
			case LOC_SDAM:
				message = " was gutted by";
				break;
			case LOC_LDAM:
				message = " was stabbed repeatedly in the legs by";
				break;
			default:
				message = " was slashed apart by";
				message2 = "'s Combat Knife";
			}
			break;
		case MOD_KNIFE_THROWN:
			switch (loc) {
				case LOC_HDAM:
				message = " caught";
				if (self->client->pers.gender == GENDER_MALE)
					message2 = "'s flying knife with his forehead";
				else if (self->client->pers.gender == GENDER_FEMALE)
					message2 = "'s flying knife with her forehead";
				else
					message2 = "'s flying knife with its forehead";
				break;
			case LOC_CDAM:
				message = "'s ribs don't help against";
				message2 = "'s flying knife";
				break;
			case LOC_SDAM:
				if (self->client->pers.gender == GENDER_MALE)
					message = " sees the contents of his own stomach thanks to";
				else if (self->client->pers.gender == GENDER_FEMALE)
					message = " sees the contents of her own stomach thanks to";
				else
					message = " sees the contents of its own stomach thanks to";
				message2 = "'s flying knife";
				break;
			case LOC_LDAM:
				if (self->client->pers.gender == GENDER_MALE)
					message = " had his legs cut off thanks to";
				else if (self->client->pers.gender == GENDER_FEMALE)
					message = " had her legs cut off thanks to";
				else
					message = " had its legs cut off thanks to";
				message2 = "'s flying knife";
				break;
			default:
				message = " was hit by";
				message2 = "'s flying Combat Knife";
			}
			break;
		case MOD_KICK:
			n = rand() % 3 + 1;
			if (n == 1) {
				if (self->client->pers.gender == GENDER_MALE)
					message = " got his ass kicked by";
				else if (self->client->pers.gender == GENDER_FEMALE)
					message = " got her ass kicked by";
				else
					message = " got its ass kicked by";
			} else if (n == 2) {
				if (self->client->pers.gender == GENDER_MALE) {
					message = " couldn't remove";
					message2 = "'s boot from his ass";
				} else if (self->client->pers.gender == GENDER_FEMALE) {
					message = " couldn't remove";
					message2 = "'s boot from her ass";
				} else {
					message = " couldn't remove";
					message2 = "'s boot from its ass";
				}
			} else {
				if (self->client->pers.gender == GENDER_MALE) {
					message = " had a Bruce Lee put on him by";
					message2 = ", with a quickness";
				} else if (self->client->pers.gender == GENDER_FEMALE) {
					message = " had a Bruce Lee put on her by";
					message2 = ", with a quickness";
				} else {
					message = " had a Bruce Lee put on it by";
					message2 = ", with a quickness";
				}
			}
			break;
		case MOD_PUNCH:
			n = rand() % 3 + 1;
			if (n == 1) {
				message = " got a free facelift by";
			} else if (n == 2) {
				message = " was knocked out by";
			} else {
				message = " caught";
				message2 = "'s iron fist";
			}
			break;
		case MOD_BLASTER:
			message = "was blasted by";
			break;
		case MOD_GRENADE:
			message = "was popped by";
			message2 = "'s grenade";
			break;
		case MOD_G_SPLASH:
			message = "was shredded by";
			message2 = "'s shrapnel";
			break;
		case MOD_HYPERBLASTER:
			message = "was melted by";
			message2 = "'s hyperblaster";
			break;
		case MOD_HANDGRENADE:
			message = " caught";
			message2 = "'s handgrenade";
			break;
		case MOD_HG_SPLASH:
			message = " didn't see";
			message2 = "'s handgrenade";
			break;
		case MOD_HELD_GRENADE:
			message = " feels";
			message2 = "'s pain";
			break;
		case MOD_TELEFRAG:
			message = " tried to invade";
			message2 = "'s personal space";
			break;
		case MOD_GRAPPLE:
			message = " was caught by";
			message2 = "'s grapple";
			break;
		}	//end of case (mod)

		if (message)
		{
			sprintf(death_msg, "%s%s %s%s\n", self->client->pers.netname,
			message, attacker->client->pers.netname, message2);
			PrintDeathMessage(death_msg, self);
			AddKilledPlayer(attacker, self);

			if (friendlyFire) {
				if (!teamplay->value || team_round_going || !ff_afterround->value)
				{
					self->enemy = attacker; //tkok
					Add_TeamKill(attacker);
					Subtract_Frag(attacker);	//attacker->client->resp.score--;
					Add_Death( self, false );
				}
			} else {
				if (!teamplay->value || mod != MOD_TELEFRAG) {
					Add_Frag(attacker, mod);
					attacker->client->radio_num_kills++;
					Add_Death( self, true );
				}
			}

			return;
		}	// if(message)
	}

	sprintf(death_msg, "%s died\n", self->client->pers.netname);
	PrintDeathMessage(death_msg, self);

	Subtract_Frag(self);	//self->client->resp.score--;
	Add_Death( self, true );
}

// zucc used to toss an item on death
void EjectItem(edict_t * ent, gitem_t * item)
{
	edict_t *drop;
	float spread;

	if (item) {
		spread = 300.0 * crandom();
		ent->client->v_angle[YAW] -= spread;
		drop = Drop_Item(ent, item);
		ent->client->v_angle[YAW] += spread;
		drop->spawnflags = DROPPED_PLAYER_ITEM;
	}

}

// unique weapons need to be specially treated so they respawn properly
void EjectWeapon(edict_t * ent, gitem_t * item)
{
	edict_t *drop;
	float spread;

	if (item) {
		spread = 300.0 * crandom();
		ent->client->v_angle[YAW] -= spread;
		drop = Drop_Item(ent, item);
		ent->client->v_angle[YAW] += spread;
		drop->spawnflags = DROPPED_PLAYER_ITEM;
		if (!in_warmup)
			drop->think = temp_think_specweap;
	}

}

void EjectMedKit( edict_t *ent, int medkit )
{
	gitem_t *item = FindItem("Health");
	float spread = 300.0 * crandom();
	edict_t *drop = NULL;

	if( ! item )
		return;

	item->world_model = "models/items/healing/medium/tris.md2";
	ent->client->v_angle[YAW] -= spread;
	drop = Drop_Item( ent, item );
	ent->client->v_angle[YAW] += spread;
	drop->model = item->world_model;
	drop->classname = "medkit";
	drop->count = medkit;

	if( ! medkit_instant->value )
		drop->style = 4; // HEALTH_MEDKIT (g_items.c)
}

//zucc toss items on death
void TossItemsOnDeath(edict_t * ent)
{
	gitem_t *item;
	qboolean quad;
	int i;

	// don't bother dropping stuff when allweapons/items is active
	if (allitem->value) {
		// remove the lasersight because then the observer might have it
		item = GET_ITEM(LASER_NUM);
		ent->client->inventory[ITEM_INDEX(item)] = 0;
	} else {
		DeadDropSpec(ent);
	}

	if( medkit_drop->value > 0 )
		EjectMedKit( ent, medkit_drop->value );

	if (allweapon->value)// don't drop weapons if allweapons is on
		return;

	if (WPF_ALLOWED(MK23_NUM) && WPF_ALLOWED(DUAL_NUM)) {
		// give the player a dual pistol so they can be sure to drop one
		item = GET_ITEM(DUAL_NUM);
		ent->client->inventory[ITEM_INDEX(item)]++;
		EjectItem(ent, item);
	}

	// check for every item we want to drop when a player dies
	for (i = MP5_NUM; i < DUAL_NUM; i++) {
		item = GET_ITEM( i );
		while (ent->client->inventory[ITEM_INDEX( item )] > 0) {
			ent->client->inventory[ITEM_INDEX( item )]--;
			EjectWeapon( ent, item );
		}
	}

	item = GET_ITEM(KNIFE_NUM);
	if (ent->client->inventory[ITEM_INDEX(item)] > 0) {
		EjectItem(ent, item);
	}
// special items

	if (!DMFLAGS(DF_QUAD_DROP))
		quad = false;
	else
		quad = (ent->client->quad_framenum > (level.framenum + HZ));
}

void TossClientWeapon(edict_t * self)
{
	gitem_t *item;
	edict_t *drop;
	qboolean quad;
	float spread;

	item = self->client->weapon;
	if (!self->client->inventory[self->client->ammo_index])
		item = NULL;
	if (item && (strcmp(item->pickup_name, "Blaster") == 0))
		item = NULL;

	if (!DMFLAGS(DF_QUAD_DROP))
		quad = false;
	else
		quad = (self->client->quad_framenum > (level.framenum + HZ));

	if (item && quad)
		spread = 22.5;
	else
		spread = 0.0;

	if (item) {
		self->client->v_angle[YAW] -= spread;
		drop = Drop_Item(self, item);
		self->client->v_angle[YAW] += spread;
		drop->spawnflags = DROPPED_PLAYER_ITEM;
	}
}

/*
==================
LookAtKiller
==================
*/
void LookAtKiller(edict_t * self, edict_t * inflictor, edict_t * attacker)
{
	vec3_t dir;

	if (attacker && attacker != world && attacker != self) {
		VectorSubtract(attacker->s.origin, self->s.origin, dir);
	} else if (inflictor && inflictor != world && inflictor != self) {
		VectorSubtract(inflictor->s.origin, self->s.origin, dir);
	} else {
		self->client->killer_yaw = self->s.angles[YAW];
		return;
	}

	if (dir[0])
		self->client->killer_yaw = 180 / M_PI * atan2(dir[1], dir[0]);
	else {
		self->client->killer_yaw = 0;
		if (dir[1] > 0)
			self->client->killer_yaw = 90;
		else if (dir[1] < 0)
			self->client->killer_yaw = -90;
	}
	if (self->client->killer_yaw < 0)
		self->client->killer_yaw += 360;
}

/*
==================
player_die
==================
*/
void player_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int n, mod;

	VectorClear(self->avelocity);

	self->takedamage = DAMAGE_YES;
	self->movetype = MOVETYPE_TOSS;

	self->s.modelindex2 = 0;	// remove linked weapon model
	self->s.modelindex3 = 0;	// remove linked ctf flag

	self->s.angles[0] = 0;
	self->s.angles[2] = 0;

	self->s.sound = 0;

	self->maxs[2] = -8;

	self->svflags |= SVF_DEADMONSTER;

	if (self->solid == SOLID_TRIGGER) {
		self->solid = SOLID_BBOX;
		RemoveFromTransparentList(self);
	}

	self->client->reload_attempts = 0;	// stop them from trying to reload
	self->client->weapon_attempts = 0;

	self->client->desired_zoom = 0;
	self->client->autoreloading = false;

	if (!self->deadflag) {
		if (ctf->value) {
			self->client->respawn_framenum = level.framenum + CTFGetRespawnTime(self) * HZ;
		}
		else if(teamdm->value) {
			self->client->respawn_framenum = level.framenum + (int)(teamdm_respawn->value * HZ);
		}
		else {
			self->client->respawn_framenum = level.framenum + 1 * HZ;
		}
		LookAtKiller(self, inflictor, attacker);
		self->client->ps.pmove.pm_type = PM_DEAD;
		ClientObituary(self, inflictor, attacker);
		if (ctf->value)
			CTFFragBonuses(self, inflictor, attacker);

		//TossClientWeapon (self);
		TossItemsOnDeath(self);

		if (ctf->value)
			CTFDeadDropFlag(self);

		// let's be safe, if the player was killed and grapple disabled before it
		CTFPlayerResetGrapple(self);

		if (!teamplay->value)
			Cmd_Help_f(self);	// show scores

		// always reset chase to killer, even if NULL
		if(limchasecam->value < 2 && attacker && attacker->client)
			self->client->resp.last_chase_target = attacker;
	}
	// remove powerups
	self->client->quad_framenum = 0;
	self->client->invincible_framenum = 0;
	self->client->breather_framenum = 0;
	self->client->enviro_framenum = 0;
	self->client->uvTime = 0;

	FreeClientEdicts(self->client);

	// clean up sniper rifle stuff
	self->client->no_sniper_display = 0;
	self->client->resp.sniper_mode = SNIPER_1X;
	self->client->desired_fov = 90;
	self->client->ps.fov = 90;
	Bandage(self);		// clear up the leg damage when dead sound?
	self->client->bandage_stopped = 0;
	self->client->medkit = 0;

	// clear inventory
	memset(self->client->inventory, 0, sizeof(self->client->inventory));

	// zucc - check if they have a primed grenade
	if (self->client->curr_weap == GRENADE_NUM
	&& ((self->client->ps.gunframe >= GRENADE_IDLE_FIRST  && self->client->ps.gunframe <= GRENADE_IDLE_LAST)
	||  (self->client->ps.gunframe >= GRENADE_THROW_FIRST && self->client->ps.gunframe <= GRENADE_THROW_LAST)))
	{
		// Reset Grenade Damage to 1.52 when requested:
		int damrad = use_classic->value ? GRENADE_DAMRAD_CLASSIC : GRENADE_DAMRAD;
		self->client->ps.gunframe = 0;
		fire_grenade2( self, self->s.origin, vec3_origin, damrad, 0, 2 * HZ, damrad * 2, false );
	}

	mod = meansOfDeath & ~MOD_FRIENDLY_FIRE;
	// Gibbing on really hard HC hit
	if ((((self->health < -35) && (mod == MOD_HC)) ||
		((self->health < -20) && (mod == MOD_M3))) && (sv_gib->value)) {
		gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n = 0; n < 5; n++)
			ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowClientHead(self, damage);
		self->client->anim_priority = ANIM_DEATH;
		self->client->anim_end = 0;
		self->takedamage = DAMAGE_NO;
	} else {		// normal death
		if (!self->deadflag) {
			static int i;

			i = (i + 1) % 3;
			// start a death animation
			if (self->client->ps.pmove.pm_flags & PMF_DUCKED)
				SetAnimation( self, FRAME_crdeath1 - 1, FRAME_crdeath5, ANIM_DEATH );
			else
				switch (i) {
				case 0:
					SetAnimation( self, FRAME_death101 - 1, FRAME_death106, ANIM_DEATH );
					break;
				case 1:
					SetAnimation( self, FRAME_death201 - 1, FRAME_death206, ANIM_DEATH );
					break;
				case 2:
					SetAnimation( self, FRAME_death301 - 1, FRAME_death308, ANIM_DEATH );
					break;
				}
			if ((mod == MOD_SNIPER) || (mod == MOD_KNIFE)
				|| (mod == MOD_KNIFE_THROWN)) {
				gi.sound(self, CHAN_VOICE, gi.soundindex("misc/glurp.wav"), 1, ATTN_NORM, 0);
				// TempFile - BEGIN sniper gibbing
				if (mod == MOD_SNIPER) {
					int n;

					switch (locOfDeath) {
					case LOC_HDAM:
						if (sv_gib->value) {
							for (n = 0; n < 8; n++)
								ThrowGib(self,
									 "models/objects/gibs/sm_meat/tris.md2",
									 damage, GIB_ORGANIC);
							ThrowClientHead(self, damage);
						}
					}
				}
			} else
				gi.sound(self, CHAN_VOICE,
					 gi.soundindex(va("*death%i.wav", (rand() % 4) + 1)), 1, ATTN_NORM, 0);
		}
	}

	// zucc this will fix a jump kick death generating a weapon
	self->client->curr_weap = MK23_NUM;

	self->client->resp.idletime = 0;

	// zucc solves problem of people stopping doors while in their dead bodies
	// ...only need it in DM though...
	// ...for teamplay, non-solid will get set soon after in CopyToBodyQue
	if (!(gameSettings & GS_ROUNDBASED)) {
		self->solid = SOLID_NOT;
	}

	self->deadflag = DEAD_DEAD;
	gi.linkentity(self);

	// in ctf, when a player dies check if he should be moved to the other team
	if(ctf->value)
		CheckForUnevenTeams(self);
}

/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
PlayersRangeFromSpot

Returns the distance to the nearest player from the given spot
================
*/
float PlayersRangeFromSpot(edict_t * spot)
{
	edict_t *player;
	float playerdistance, bestplayerdistance = 9999999;
	int n;


	for (n = 1; n <= game.maxclients; n++) {
		player = &g_edicts[n];

		if (!player->inuse)
			continue;

		if (player->health <= 0)
			continue;
	
		playerdistance = Distance(spot->s.origin, player->s.origin);
		if (playerdistance < bestplayerdistance)
			bestplayerdistance = playerdistance;
	}

	return bestplayerdistance;
}

/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point, but NOT the two points closest
to other players
================
*/
edict_t *SelectRandomDeathmatchSpawnPoint(void)
{
	edict_t *spot, *spot1, *spot2;
	int count = 0;
	int selection;
	float range, range1, range2;

	spot = NULL;
	range1 = range2 = 99999;
	spot1 = spot2 = NULL;

	while ((spot = G_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		count++;
		range = PlayersRangeFromSpot(spot);
		if (range < range1) {
			if (range1 < range2) {
				range2 = range1;
				spot2 = spot1;
			}
			range1 = range;
			spot1 = spot;
		} else if (range < range2) {
			range2 = range;
			spot2 = spot;
		}
	}

	if (!count)
		return NULL;

	if (count <= 2) {
		return (rand() % count) ? spot2 : spot1;
	}

	
	count -= 2;

	selection = rand() % count;

	spot = NULL;
	do {
		spot = G_Find(spot, FOFS(classname), "info_player_deathmatch");
		if (spot == spot1 || spot == spot2)
			selection++;
	}
	while (selection--);

	return spot;
}

/*
================
SelectFarthestDeathmatchSpawnPoint

================
*/
edict_t *SelectFarthestDeathmatchSpawnPoint(void)
{
	edict_t *bestspot;
	float bestdistance, bestplayerdistance;
	edict_t *spot;

	spot = NULL;
	bestspot = NULL;
	bestdistance = 0;
	while ((spot = G_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		bestplayerdistance = PlayersRangeFromSpot(spot);

		if (bestplayerdistance > bestdistance) {
			bestspot = spot;
			bestdistance = bestplayerdistance;
		}
	}

	if (bestspot) {
		return bestspot;
	}
	// if there is a player just spawned on each and every start spot
	// we have no choice to turn one into a telefrag meltdown
	spot = G_Find(NULL, FOFS(classname), "info_player_deathmatch");

	return spot;
}

edict_t *SelectDeathmatchSpawnPoint(void)
{
	if (DMFLAGS(DF_SPAWN_FARTHEST))
		return SelectFarthestDeathmatchSpawnPoint();
	else
		return SelectRandomDeathmatchSpawnPoint();
}

/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, coop start, etc
============
*/
void SelectSpawnPoint(edict_t * ent, vec3_t origin, vec3_t angles)
{
	edict_t *spot = NULL;

	//FIREBLADE
	if (ctf->value)
		spot = SelectCTFSpawnPoint(ent);
	else if (dom->value)
		spot = SelectDeathmatchSpawnPoint();
	else if (!(gameSettings & GS_DEATHMATCH) && ent->client->resp.team && !in_warmup) {
		spot = SelectTeamplaySpawnPoint(ent);
	} else {
		spot = SelectDeathmatchSpawnPoint();
	}

	// find a single player start spot
	if (!spot) {
		gi.dprintf("Warning: failed to find deathmatch spawn point\n");

		while ((spot = G_Find(spot, FOFS(classname), "info_player_start")) != NULL) {
			if (!game.spawnpoint[0] && !spot->targetname)
				break;

			if (!game.spawnpoint[0] || !spot->targetname)
				continue;

			if (Q_stricmp(game.spawnpoint, spot->targetname) == 0)
				break;
		}

		if (!spot) {
			if (!game.spawnpoint[0]) {	// there wasn't a spawnpoint without a target, so use any
				spot = G_Find(spot, FOFS(classname), "info_player_start");
			}
			if (!spot) {
				gi.error("Couldn't find spawn point %s\n", game.spawnpoint);
				return;
			}
		}
	}

	VectorCopy(spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy(spot->s.angles, angles);
}

//======================================================================

void InitBodyQue(void)
{
	int i;
	edict_t *ent;

	level.body_que = 0;
	for (i = 0; i < BODY_QUEUE_SIZE; i++) {
		ent = G_Spawn();
		ent->classname = "bodyque";
	}
}

void body_die(edict_t * self, edict_t * inflictor, edict_t * attacker, int damage, vec3_t point)
{
/*      int     n;*/

	if (self->health < -40) {
		// remove gibbing
/*                gi.sound (self, CHAN_BODY, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
                for (n= 0; n < 4; n++)
                        ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
                self->s.origin[2] -= 48;
                ThrowClientHead (self, damage);*/
		self->takedamage = DAMAGE_NO;
	}
}

void CopyToBodyQue(edict_t * ent)
{
	edict_t *body;

	// grab a body que and cycle to the next one
	body = &g_edicts[game.maxclients + level.body_que + 1];
	level.body_que = (level.body_que + 1) % BODY_QUEUE_SIZE;

	// FIXME: send an effect on the removed body

	gi.unlinkentity(ent);

	gi.unlinkentity(body);

	body->s = ent->s;
	body->s.number = body - g_edicts;
	body->s.event = EV_OTHER_TELEPORT;
	VectorCopy( body->s.origin, body->s.old_origin );
	VectorCopy( body->s.origin, body->old_origin );

	body->svflags = ent->svflags;
	VectorCopy(ent->mins, body->mins);
	VectorCopy(ent->maxs, body->maxs);
	VectorCopy(ent->absmin, body->absmin);
	VectorCopy(ent->absmax, body->absmax);
	VectorCopy(ent->size, body->size);
	// All our bodies will be non-solid -FB
	body->solid = SOLID_NOT;
	//body->solid = ent->solid;
	body->clipmask = ent->clipmask;
	body->owner = ent->owner;

//FB 5/31/99
	body->movetype = MOVETYPE_TOSS;	// just in case?
//        body->movetype = ent->movetype;
	VectorCopy(ent->velocity, body->velocity);
	body->mass = ent->mass;
	body->groundentity = NULL;
//FB 5/31/99
//FB 6/1/99
	body->s.renderfx = 0;
//FB

	body->die = body_die;
	body->takedamage = DAMAGE_YES;

//PG BUND - BEGIN
	//Disable to be seen by irvision
	body->s.renderfx &= ~RF_IR_VISIBLE;
//PG BUND - END

	gi.linkentity(body);
}

void CleanBodies()
{
	int i;
	edict_t *ent;

	ent = g_edicts + game.maxclients + 1;
	for (i = 0; i < BODY_QUEUE_SIZE; i++, ent++) {
		gi.unlinkentity( ent );
		ent->solid = SOLID_NOT;
		ent->movetype = MOVETYPE_NOCLIP;
		ent->svflags |= SVF_NOCLIENT;
	}
	level.body_que = 0;
}

void respawn(edict_t *self)
{
	if (self->solid != SOLID_NOT || self->deadflag == DEAD_DEAD)
		CopyToBodyQue(self);

	PutClientInServer(self);

	if (!(self->svflags & SVF_NOCLIENT))
	{
		if (team_round_going && !(gameSettings & GS_ROUNDBASED))
			AddToTransparentList(self);

		if (respawn_effect->value) {
			gi.WriteByte(svc_muzzleflash);
			gi.WriteShort(self - g_edicts);
			gi.WriteByte(MZ_RESPAWN);
			gi.multicast(self->s.origin, MULTICAST_PVS);

			/*// add a teleportation effect
			self->s.event = EV_PLAYER_TELEPORT;

			// hold in place briefly
			self->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
			self->client->ps.pmove.pm_time = 14;*/
		}
	}

	self->client->respawn_framenum = level.framenum + 2 * HZ;
}

//==============================================================

void AllWeapons(edict_t * ent)
{
	int i;
	gitem_t *it;

	for (i = 0; i < game.num_items; i++) {
		it = itemlist + i;
		if (!it->pickup)
			continue;
		if (!(it->flags & IT_WEAPON))
			continue;

		if (!it->typeNum || !WPF_ALLOWED(it->typeNum))
			continue;

		switch(it->typeNum) {
		case MK23_NUM:
			ent->client->inventory[i] = 1;
			ent->client->mk23_rds = ent->client->mk23_max;
			break;
		case MP5_NUM:
			ent->client->inventory[i] = 1;
			ent->client->mp5_rds = ent->client->mp5_max;
			break;
		case M4_NUM:
			ent->client->inventory[i] = 1;
			ent->client->m4_rds = ent->client->m4_max;	    
			break;
		case M3_NUM:
			ent->client->inventory[i] = 1;
			ent->client->shot_rds = ent->client->shot_max;
			break;
		case HC_NUM:
			ent->client->inventory[i] = 1;
			ent->client->cannon_rds = ent->client->cannon_max;
			ent->client->shot_rds = ent->client->shot_max;
			break;
		case SNIPER_NUM:
			ent->client->inventory[i] = 1;
			ent->client->sniper_rds = ent->client->sniper_max;
			break;
		case DUAL_NUM:
			ent->client->inventory[i] = 1;
			ent->client->dual_rds = ent->client->dual_max;
			break;
		case KNIFE_NUM:
			ent->client->inventory[i] = 10;
			break;
		case GRENADE_NUM:
			ent->client->inventory[i] = tgren->value;
			break;
		}
	}
	
	for (i = 0; i < game.num_items; i++) {
		it = itemlist + i;
		if (!it->pickup)
			continue;
		if (!(it->flags & IT_AMMO))
			continue;
		Add_Ammo(ent, it, 1000);
	}
}

void AllItems(edict_t * ent)
{
	edict_t etemp;
	int i;
	gitem_t *it;

	for (i = 0; i < game.num_items; i++) {
		it = itemlist + i;
		if (!it->pickup)
			continue;
		if (!(it->flags & IT_ITEM))
			continue;

		etemp.item = it;

		if (ent->client->unique_item_total >= unique_items->value)
			ent->client->unique_item_total = unique_items->value - 1;
		Pickup_Special(&etemp, ent);
	}

}

// equips a client with item/weapon in teamplay

void EquipClient(edict_t * ent)
{
	gclient_t *client;
	gitem_t *item;
	edict_t etemp;
	int band = 0, itemNum = 0;

	client = ent->client;

	if(use_grapple->value)
		client->inventory[ITEM_INDEX(FindItem("Grapple"))] = 1;

	// Honor changes to wp_flags and itm_flags.
	if( client->pers.chosenWeapon && ! WPF_ALLOWED(client->pers.chosenWeapon->typeNum) )
		client->pers.chosenWeapon = NULL;
	if( client->pers.chosenItem && ! ITF_ALLOWED(client->pers.chosenItem->typeNum) )
		client->pers.chosenItem = NULL;

	if (client->pers.chosenItem) {
		if (client->pers.chosenItem->typeNum == BAND_NUM) {
			band = 1;
			if (tgren->value > 0)	// team grenades is turned on
			{
				item = GET_ITEM(GRENADE_NUM);
				client->inventory[ITEM_INDEX(item)] = tgren->value;
			}
		}
	}

	// set them up with initial pistol ammo
	if (WPF_ALLOWED(MK23_ANUM)) {
		item = GET_ITEM(MK23_ANUM);
		if (band)
			client->inventory[ITEM_INDEX(item)] = 2;
		else
			client->inventory[ITEM_INDEX(item)] = 1;
	}

	itemNum = client->pers.chosenWeapon ? client->pers.chosenWeapon->typeNum : 0;

	switch (itemNum) {
	case MP5_NUM:
		item = GET_ITEM(MP5_NUM);
		client->selected_item = ITEM_INDEX(item);
		client->inventory[client->selected_item] = 1;
		client->weapon = item;
		client->curr_weap = MP5_NUM;
		client->unique_weapon_total = 1;
		item = GET_ITEM(MP5_ANUM);
		if (band)
			client->inventory[ITEM_INDEX(item)] = 2;
		else
			client->inventory[ITEM_INDEX(item)] = 1;
		client->mp5_rds = client->mp5_max;
		break;
	case M4_NUM:
		item = GET_ITEM(M4_NUM);
		client->selected_item = ITEM_INDEX(item);
		client->inventory[client->selected_item] = 1;
		client->weapon = item;
		client->curr_weap = M4_NUM;
		client->unique_weapon_total = 1;
		item = GET_ITEM(M4_ANUM);
		if (band)
			client->inventory[ITEM_INDEX(item)] = 2;
		else
			client->inventory[ITEM_INDEX(item)] = 1;
		client->m4_rds = client->m4_max;
		break;
	case M3_NUM:
		item = GET_ITEM(M3_NUM);
		client->selected_item = ITEM_INDEX(item);
		client->inventory[client->selected_item] = 1;
		client->weapon = item;
		client->curr_weap = M3_NUM;
		client->unique_weapon_total = 1;
		item = GET_ITEM(SHELL_ANUM);
		if (band)
			client->inventory[ITEM_INDEX(item)] = 14;
		else
			client->inventory[ITEM_INDEX(item)] = 7;
		client->shot_rds = client->shot_max;
		break;
	case HC_NUM:
		item = GET_ITEM(HC_NUM);
		client->selected_item = ITEM_INDEX(item);
		client->inventory[client->selected_item] = 1;
		client->weapon = item;
		client->curr_weap = HC_NUM;
		client->unique_weapon_total = 1;
		item = GET_ITEM(SHELL_ANUM);
		if (band)
			client->inventory[ITEM_INDEX(item)] = 24;
		else
			client->inventory[ITEM_INDEX(item)] = 12;
		client->cannon_rds = client->cannon_max;
		break;
	case SNIPER_NUM:
		item = GET_ITEM(SNIPER_NUM);
		client->selected_item = ITEM_INDEX(item);
		client->inventory[ITEM_INDEX(item)] = 1;
		client->weapon = item;
		client->curr_weap = SNIPER_NUM;
		client->unique_weapon_total = 1;
		item = GET_ITEM(SNIPER_ANUM);
		if (band)
			client->inventory[ITEM_INDEX(item)] = 20;
		else
			client->inventory[ITEM_INDEX(item)] = 10;
		client->sniper_rds = client->sniper_max;
		break;
	case DUAL_NUM:
		item = GET_ITEM(DUAL_NUM);
		client->selected_item = ITEM_INDEX(item);
		client->inventory[client->selected_item] = 1;
		client->weapon = item;
		client->curr_weap = DUAL_NUM;
		item = GET_ITEM(MK23_ANUM);
		if (band)
			client->inventory[ITEM_INDEX(item)] = 4;
		else
			client->inventory[ITEM_INDEX(item)] = 2;
		client->dual_rds = client->dual_max;
		break;
	case KNIFE_NUM:
		item = GET_ITEM(KNIFE_NUM);
		client->selected_item = ITEM_INDEX(item);
		if (band)
			client->inventory[client->selected_item] = 20;
		else
			client->inventory[client->selected_item] = 10;
		client->weapon = item;
		client->curr_weap = KNIFE_NUM;
		break;
	}

	if (client->pers.chosenItem) {
		memset(&etemp, 0, sizeof(etemp));
		etemp.item = client->pers.chosenItem;
		Pickup_Special(&etemp, ent);
	}
}

// Igor[Rock] start
void EquipClientDM(edict_t * ent)
{
	gclient_t *client;
	gitem_t *item;
	int itemNum = 0;

	client = ent->client;

	if(use_grapple->value)
		client->inventory[ITEM_INDEX(FindItem("Grapple"))] = 1;

	if (*strtwpn->string)
		itemNum = GetWeaponNumFromArg(strtwpn->string);

	// Give some ammo for the weapon
	switch (itemNum) {
	case MK23_NUM:
		return;
	case MP5_NUM:
		item = GET_ITEM(MP5_NUM);
		client->selected_item = ITEM_INDEX(item);
		client->inventory[client->selected_item] = 1;
		client->weapon = item;
		client->mp5_rds = client->mp5_max;
		client->curr_weap = MP5_NUM;
		if (!allweapon->value) {
			client->unique_weapon_total = 1;
		}
		item = GET_ITEM(MP5_ANUM);
		client->inventory[ITEM_INDEX(item)] = 1;
		break;
	case M4_NUM:
		item = GET_ITEM(M4_NUM);
		client->selected_item = ITEM_INDEX(item);
		client->inventory[client->selected_item] = 1;
		client->weapon = item;
		client->m4_rds = client->m4_max;
		client->curr_weap = M4_NUM;
		if (!allweapon->value) {
			client->unique_weapon_total = 1;
		}
		item = GET_ITEM(M4_ANUM);
		client->inventory[ITEM_INDEX(item)] = 1;
		break;
	case M3_NUM:
		item = GET_ITEM(M3_NUM);
		client->selected_item = ITEM_INDEX(item);
		client->inventory[client->selected_item] = 1;
		client->weapon = item;
		client->shot_rds = client->shot_max;
		client->curr_weap = M3_NUM;
		if (!allweapon->value) {
			client->unique_weapon_total = 1;
		}
		item = GET_ITEM(SHELL_ANUM);
		client->inventory[ITEM_INDEX(item)] = 7;
		break;
	case HC_NUM:
		item = GET_ITEM(HC_NUM);
		client->selected_item = ITEM_INDEX(item);
		client->inventory[client->selected_item] = 1;
		client->weapon = item;
		client->cannon_rds = client->cannon_max;
		client->shot_rds = client->shot_max;
		client->curr_weap = HC_NUM;
		if (!allweapon->value) {
			client->unique_weapon_total = 1;
		}
		item = GET_ITEM(SHELL_ANUM);
		client->inventory[ITEM_INDEX(item)] = 12;
		break;
	case SNIPER_NUM:
		item = GET_ITEM(SNIPER_NUM);
		client->selected_item = ITEM_INDEX(item);
		client->inventory[client->selected_item] = 1;
		client->weapon = item;
		client->sniper_rds = client->sniper_max;
		client->curr_weap = SNIPER_NUM;
		if (!allweapon->value) {
			client->unique_weapon_total = 1;
		}
		item = GET_ITEM(SNIPER_ANUM);;
		client->inventory[ITEM_INDEX(item)] = 10;
		break;
	case DUAL_NUM:
		item = GET_ITEM(DUAL_NUM);
		client->selected_item = ITEM_INDEX(item);
		client->inventory[client->selected_item] = 1;
		client->weapon = item;
		client->dual_rds = client->dual_max;
		client->mk23_rds = client->mk23_max;
		client->curr_weap = DUAL_NUM;
		item = GET_ITEM(MK23_ANUM);
		client->inventory[ITEM_INDEX(item)] = 2;
		break;
	case GRENADE_NUM:
		item = GET_ITEM(GRENADE_NUM);
		client->selected_item = ITEM_INDEX(item);
		client->inventory[client->selected_item] = tgren->value;
		client->weapon = item;
		client->curr_weap = GRENADE_NUM;
		break;
	case KNIFE_NUM:
		item = GET_ITEM(KNIFE_NUM);
		client->selected_item = ITEM_INDEX(item);
		client->inventory[client->selected_item] = 10;
		client->weapon = item;
		client->curr_weap = KNIFE_NUM;
		break;
	}
}

// Igor[Rock] ende


/*
===========
ClientLegDamage

Called when a player takes leg damage
============
*/

void ClientLegDamage(edict_t *ent)
{
	ent->client->leg_damage = 1;
	ent->client->leghits++;

	// Reki: limp_nopred behavior
	switch (ent->client->pers.limp_nopred & 255)
	{
		case 0:
			break;
		case 2:
			if (sv_limp_highping->value <= 0)
				break;
			// if the 256 bit flag is set, we have to be cautious to only deactivate if ping swung significantly
			// so each leg break doesn't flipflop between behavior if client ping is fluctuating
			if (ent->client->pers.limp_nopred & 256)
			{
				if (ent->client->ping < (int)sv_limp_highping->value - 15)
				{
					ent->client->pers.limp_nopred &= ~256;
					break;
				}
			}
			else if (ent->client->ping < (int)sv_limp_highping->value)
				break;
			ent->client->pers.limp_nopred |= 256;
		case 1:
			if (e_enhancedSlippers->value && INV_AMMO(ent, SLIP_NUM)) // we don't limp with enhanced slippers, so just ignore this leg damage.
				break;

			ent->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
			break;
	}
	//

}

void ClientFixLegs(edict_t *ent)
{
	if (ent->client->leg_damage && ent->client->ctf_grapplestate <= CTF_GRAPPLE_STATE_FLY)
	{
		ent->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
	}

	ent->client->leg_noise = 0;
	ent->client->leg_damage = 0;
	ent->client->leghits = 0;
	ent->client->leg_dam_count = 0;
}



/*
===========
PutClientInServer

Called when a player connects to a server or respawns in
a deathmatch.
============
*/

void PutClientInServer(edict_t * ent)
{
	vec3_t mins = { -16, -16, -24 };
	vec3_t maxs = { 16, 16, 32 };
	int index, going_observer, i;
	vec3_t spawn_origin, spawn_angles;
	gclient_t *client;
	client_persistant_t pers;
	client_respawn_t resp;
	gitem_t *item;

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	SelectSpawnPoint(ent, spawn_origin, spawn_angles);

	index = ent - g_edicts - 1;
	client = ent->client;

	FreeClientEdicts(client);

	// deathmatch wipes most client data every spawn
	resp = client->resp;
	pers = client->pers;

	memset(client, 0, sizeof(*client));

	client->pers = pers;
	client->resp = resp;


	client->clientNum = index;

	//zucc give some ammo
	// changed to mk23
	item = GET_ITEM( MK23_NUM );
	client->selected_item = ITEM_INDEX( item );
	client->inventory[client->selected_item] = 1;

	client->weapon = item;
	client->lastweapon = item;

	if (WPF_ALLOWED( KNIFE_NUM )) {
		item = GET_ITEM( KNIFE_NUM );
		client->inventory[ITEM_INDEX( item )] = 1;
		if (!WPF_ALLOWED( MK23_NUM )) {
			client->selected_item = ITEM_INDEX( item );
			client->weapon = item;
			client->lastweapon = item;
		}
	}
	client->curr_weap = client->weapon->typeNum;


	ent->health = 100;
	ent->max_health = 100;
	
	client->max_pistolmags = 2;
	client->max_shells = 14;
	client->max_mp5mags = 2;
	client->max_m4mags = 1;
	client->max_sniper_rnds = 20;

	client->knife_max = 10;
	client->grenade_max = 2;

	client->mk23_max = 12;
	client->mp5_max = 30;
	client->m4_max = 24;
	client->shot_max = 7;
	client->sniper_max = 6;
	client->cannon_max = 2;
	client->dual_max = 24;
	if (WPF_ALLOWED( MK23_NUM )) {
		client->mk23_rds = client->mk23_max;
		client->dual_rds = client->mk23_max;
	}

	client->knife_max = 10;
	client->grenade_max = 2;
	client->desired_fov = 90;


	// clear entity values
	ent->groundentity = NULL;
	ent->client = &game.clients[index];
	ent->takedamage = DAMAGE_AIM;
	ent->movetype = MOVETYPE_WALK;
	ent->viewheight = 22;
	ent->inuse = true;
	ent->classname = "player";
	ent->mass = 200;
	ent->solid = SOLID_BBOX;
	ent->deadflag = DEAD_NO;
	ent->air_finished_framenum = level.framenum + 12 * HZ;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->model = "players/male/tris.md2";
	ent->pain = player_pain;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags &= ~(FL_NO_KNOCKBACK | FL_GODMODE);
	ent->svflags &= ~(SVF_DEADMONSTER | SVF_NOCLIENT);

	VectorCopy(mins, ent->mins);
	VectorCopy(maxs, ent->maxs);
	VectorClear(ent->velocity);

	// clear playerstate values
	client->ps.fov = 90;
	client->ps.gunindex = gi.modelindex(client->weapon->view_model);

	// clear entity state values
	ent->s.effects = 0;
	ent->s.skinnum = ent - g_edicts - 1;
	ent->s.modelindex = 255;	// will use the skin specified model

	// zucc vwep
	//ent->s.modelindex2 = 255;             // custom gun model
	ShowGun(ent);

	ent->s.frame = 0;
	VectorCopy(spawn_origin, ent->s.origin);
	ent->s.origin[2] += 1;	// make sure off ground
	VectorCopy(ent->s.origin, ent->s.old_origin);
	VectorCopy(ent->s.origin, ent->old_origin);

	client->ps.pmove.origin[0] = ent->s.origin[0] * 8;
	client->ps.pmove.origin[1] = ent->s.origin[1] * 8;
	client->ps.pmove.origin[2] = ent->s.origin[2] * 8;

	ent->s.angles[PITCH] = 0;
	ent->s.angles[YAW] = spawn_angles[YAW];
	ent->s.angles[ROLL] = 0;
	VectorCopy(ent->s.angles, client->ps.viewangles);
	VectorCopy(ent->s.angles, client->v_angle);

	// set the delta angle
	for (i = 0; i < 3; i++)
		client->ps.pmove.delta_angles[i] = ANGLE2SHORT(ent->s.angles[i] - client->resp.cmd_angles[i]);

	if (teamplay->value) {
		going_observer = (!ent->client->resp.team || ent->client->resp.subteam);
	}
	else {
		going_observer = ent->client->pers.spectator;
		if (dm_choose->value && !ent->client->pers.dm_selected)
			going_observer = 1;
	}

	if (going_observer || level.intermission_framenum) {
		ent->movetype = MOVETYPE_NOCLIP;
		ent->solid = SOLID_NOT;
		ent->svflags |= SVF_NOCLIENT;
		ent->client->ps.gunindex = 0;

		gi.linkentity(ent);
		return;
	}

	if (!teamplay->value) {	// this handles telefrags...
		KillBox(ent);
	} else {
		ent->solid = SOLID_TRIGGER;
	}

	gi.linkentity( ent );

	if ((int)uvtime->value > 0) {
		if (teamplay->value && ! in_warmup) {
			if (!(gameSettings & GS_ROUNDBASED) && team_round_going && !lights_camera_action) {
				client->uvTime = uvtime->value;
			}
		} else if (dm_shield->value) {
			client->uvTime = uvtime->value;
		}
	}

	ent->client->medkit = 0;

	if( jump->value )
	{
		Jmp_EquipClient(ent);
		return;
	}

	// items up here so that the bandolier will change equipclient below
	if (allitem->value)
		AllItems(ent);

	if (gameSettings & GS_WEAPONCHOOSE)
		EquipClient(ent);
	else
		EquipClientDM(ent);

	if (allweapon->value)
		AllWeapons(ent);

	// force the current weapon up
	client->newweapon = client->weapon;
	ChangeWeapon(ent);
}

/*
=====================
ClientBeginDeathmatch

A client has just connected to the server in 
deathmatch mode, so clear everything out before starting them.
=====================
*/
void ClientBeginDeathmatch(edict_t * ent)
{
	int checkFrame, saved_team = ent->client->resp.team;

	G_InitEdict(ent);

	memset(&ent->client->resp, 0, sizeof(ent->client->resp));

	ent->client->resp.enterframe = level.framenum;
	ent->client->resp.gldynamic = 1;
	
	if (!ent->client->pers.connected) {
		ent->client->pers.connected = true;
		ClientUserinfoChanged(ent, ent->client->pers.userinfo);
	}

	// clear weapons and items if not auto_equipt
	if (!auto_equip->value || !(gameSettings & GS_WEAPONCHOOSE)) {
		ent->client->pers.chosenWeapon = NULL;
		ent->client->pers.chosenItem = NULL;
		ent->client->pers.dm_selected = 0;
		ent->client->pers.menu_shown = 0;
	} else {
		if (teamplay->value)
			ent->client->pers.menu_shown = 0;
	}

	if (!dm_choose->value && !warmup->value) {
		if (!ent->client->pers.chosenWeapon) {
			if (WPF_ALLOWED(MP5_NUM))
				ent->client->pers.chosenWeapon = GET_ITEM(MP5_NUM);
			else if (WPF_ALLOWED(MK23_NUM))
				ent->client->pers.chosenWeapon = GET_ITEM(MK23_NUM);
			else if (WPF_ALLOWED(KNIFE_NUM))
				ent->client->pers.chosenWeapon = GET_ITEM(KNIFE_NUM);
			else
				ent->client->pers.chosenWeapon = GET_ITEM(MK23_NUM);
		}
		if (!ent->client->pers.chosenItem)
			ent->client->pers.chosenItem = GET_ITEM(KEV_NUM);
	} else {
		if (wp_flags->value < 2 && !ent->client->pers.chosenWeapon)
			ent->client->pers.chosenWeapon = GET_ITEM(MK23_NUM);
	}


	TourneyNewPlayer(ent);
	vInitClient(ent);

	// locate ent at a spawn point
	PutClientInServer(ent);

	if (level.intermission_framenum) {
		MoveClientToIntermission(ent);
	} else {
		if (!teamplay->value && !dm_choose->value) {	//FB 5/31/99
			// send effect
			gi.WriteByte(svc_muzzleflash);
			gi.WriteShort(ent - g_edicts);
			gi.WriteByte(MZ_LOGIN);
			gi.multicast(ent->s.origin, MULTICAST_PVS);
		}
	}

	gi.bprintf(PRINT_HIGH, "%s entered the game\n", ent->client->pers.netname);

	// TNG:Freud Automaticly join saved teams.
	if (saved_team && auto_join->value && teamplay->value)
		JoinTeam(ent, saved_team, 1);


	if (!level.intermission_framenum) {
		if (!teamplay->value && ent->solid == SOLID_NOT) {
			gi.bprintf(PRINT_HIGH, "%s became a spectator\n", ent->client->pers.netname);
		}
		PrintMOTD(ent);
	}

	if(am->value && game.bot_count > 0){
		char msg[128];
		Com_sprintf(msg, sizeof(msg), "** This server contains BOTS for you to play with until real players join up!  Enjoy! **");
		gi.centerprintf(ent, msg);
	}

	ent->client->resp.motd_refreshes = 1;

	//AQ2:TNG - Slicer: Set time to check clients
	checkFrame = level.framenum + (int)(check_time->value * HZ);
	ent->client->resp.checkframe[0] = checkFrame;
	ent->client->resp.checkframe[1] = checkFrame + 2 * HZ;
	ent->client->resp.checkframe[2] = checkFrame + 3 * HZ;

	G_UpdatePlayerStatusbar(ent, 1);

	// make sure all view stuff is valid
	ClientEndServerFrame(ent);
}

/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the game.  This will happen every level load.
============
*/
void ClientBegin(edict_t * ent)
{
	ent->client = game.clients + (ent - g_edicts - 1);

	ClientBeginDeathmatch(ent);
}

/*
===========
ClientUserInfoChanged

called whenever the player updates a userinfo variable.

The game can override any of the settings in place
(forcing skins or names, etc) before copying it off.
============
*/
void ClientUserinfoChanged(edict_t *ent, char *userinfo)
{
	char *s, tnick[16];
	qboolean nickChanged = false;
	gclient_t *client = ent->client;

	// check for malformed or illegal info strings
	if (!Info_Validate(userinfo)) {
		strcpy(userinfo, "\\name\\badinfo\\skin\\male/grunt");
	}
	// set name
	s = Info_ValueForKey(userinfo, "name");
	Q_strncpyz(tnick, s, sizeof(tnick));
	if(!tnick[0])
		strcpy(tnick, "unnamed");

	if (strcmp(client->pers.netname, tnick))
	{
		// on the initial update, we won't broadcast the message.
		if (!client->pers.mvdspec && client->pers.netname[0])
		{
			size_t i = 1;
			for( ; i <= game.maxclients; i ++ )
			{
				edict_t *other = &g_edicts[i];
				if( ! other->inuse || ! other->client )
					continue;
				if( team_round_going && (gameSettings & GS_ROUNDBASED) && ! deadtalk->value && ! IS_ALIVE(ent) && IS_ALIVE(other) )
					continue;
				if( IsInIgnoreList( other, ent ) )
					continue;
				gi.cprintf( other, PRINT_MEDIUM, "%s is now known as %s.\n", client->pers.netname, tnick ); //TempFile
			}
			if( dedicated->value )
				gi.dprintf( "%s is now known as %s.\n", client->pers.netname, tnick ); //TempFile
			nickChanged = true;
		}
		strcpy(client->pers.netname, tnick);
	}
  
	if (client->pers.mvdspec) {
		client->pers.spectator = true;
	} else {
		if (!teamplay->value) {
			s = Info_ValueForKey(userinfo, "spectator");
			client->pers.spectator = (strcmp(s, "0") != 0);
		}

		// set skin
		s = Info_ValueForKey(userinfo, "skin");

		// AQ:TNG - JBravo fixing $$ Skin server crash bug
		if (strstr(s, "$$")) {
			Info_SetValueForKey(userinfo, "skin", "male/grunt");
			s = Info_ValueForKey(userinfo, "skin");
		}

		// combine name and skin into a configstring
		AssignSkin(ent, s, nickChanged);
	}

	client->ps.fov = 90;

	client->pers.firing_style = ACTION_FIRING_CENTER;
	// handedness
	s = Info_ValueForKey(userinfo, "hand");
	if (strlen(s)) {
		client->pers.hand = atoi(s);
		if (strstr(s, "classic high") != NULL)
			client->pers.firing_style = ACTION_FIRING_CLASSIC_HIGH;
		else if (strstr(s, "classic") != NULL)
			client->pers.firing_style = ACTION_FIRING_CLASSIC;
	}
	// save off the userinfo in case we want to check something later
	Q_strncpyz(client->pers.userinfo, userinfo, sizeof(client->pers.userinfo));

	s = Info_ValueForKey( client->pers.userinfo, "gender" );
	if (s[0] == 'f' || s[0] == 'F') {
		client->pers.gender = GENDER_FEMALE;
	} else if (s[0] == 'm' || s[0] == 'M') {
		client->pers.gender = GENDER_MALE;
	} else {
		client->pers.gender = GENDER_NEUTRAL;
	}
}

inline qboolean IsSlotIgnored(edict_t *slot, edict_t **ignore, size_t num_ignore)
{
	for (size_t i = 0; i < num_ignore; i++)
		if (slot == ignore[i])
			return true;

	return false;
}

inline edict_t *ClientChooseSlot_Any(edict_t **ignore, size_t num_ignore)
{
	for (size_t i = 0; i < game.maxclients; i++)
		if (!IsSlotIgnored(globals.edicts + i + 1, ignore, num_ignore) && !game.clients[i].pers.connected)
			return globals.edicts + i + 1;

	return NULL;
}

// [Paril-KEX] for coop, we want to try to ensure that players will always get their
// proper slot back when they connect.
edict_t *ClientChooseSlot(const char *userinfo, const char *social_id, qboolean isBot, edict_t **ignore, size_t num_ignore, qboolean cinematic)
{
	// just find any free slot
	return ClientChooseSlot_Any(ignore, num_ignore);
}

/*
===========
ClientConnect

Called when a player begins connecting to the server.
The game can refuse entrance to a client by returning false.
If the client is allowed, the connection process will continue
and eventually get to ClientBegin()
Changing levels will NOT cause this to be called again, but
loadgames will.
============
*/
qboolean ClientConnect(edict_t * ent, char *userinfo)
{
	char *value, ipaddr_buf[64];
	int tempBan = 0;

	// check to see if they are on the banned IP list
	value = Info_ValueForKey( userinfo, "ip" );

	if (strlen(value) > sizeof(ipaddr_buf) - 1)
		gi.dprintf("ipaddr_buf length exceeded\n");
	Q_strncpyz(ipaddr_buf, value, sizeof(ipaddr_buf));

	if (SV_FilterPacket(ipaddr_buf, &tempBan)) {
		userinfo[0] = '\0';
		if(tempBan)
			Info_SetValueForKey(userinfo, "rejmsg", va("Temporary banned for %i games.", tempBan));
		else
			Info_SetValueForKey(userinfo, "rejmsg", "Banned.");
		return false;
	}
	// check for a password
	value = Info_ValueForKey(userinfo, "password");
	if (*password->string && strcmp(password->string, "none") && strcmp(password->string, value)) {
		userinfo[0] = '\0';
		Info_SetValueForKey(userinfo, "rejmsg", "Password required or incorrect.");
		return false;
	}

	// they can connect
	ent->client = game.clients + (ent - g_edicts - 1);

	// We're not going to attempt to support reconnection...
	if (ent->client->pers.connected) {
		ClientDisconnect(ent);
	}

	memset(ent->client, 0, sizeof(gclient_t));

	Q_strncpyz(ent->client->pers.ip, ipaddr_buf, sizeof(ent->client->pers.ip));
	Q_strncpyz(ent->client->pers.userinfo, userinfo, sizeof(ent->client->pers.userinfo));

	if (game.serverfeatures & GMF_MVDSPEC) {
		value = Info_ValueForKey(userinfo, "mvdspec");
		if (*value) {
			ent->client->pers.mvdspec = true;
		}
	}

	if (game.maxclients > 1) {
		value = Info_ValueForKey(userinfo, "name");
		gi.dprintf("%s@%s connected\n", value, ipaddr_buf);
	}

	//rekkie -- silence ban -- s
	if (SV_FilterSBPacket(ipaddr_buf, NULL)) // Check if player has been silenced
	{
		ent->client->pers.silence_banned = true;
		value = Info_ValueForKey(userinfo, "name");
		gi.dprintf("%s has been [SILENCED] because they're on the naughty list\n", value); // Notify console the player is silenced
	}
	else
		ent->client->pers.silence_banned = false;
	//rekkie -- silence ban -- e

	//set connected on ClientBeginDeathmatch as clientconnect doesn't always
	//guarantee a client is actually making it all the way into the game.
	//ent->client->pers.connected = true;
	return true;
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.
============
*/
void ClientDisconnect(edict_t * ent)
{
	int i;
	edict_t *etemp;

	if (!ent->client)
		return;

	MM_LeftTeam( ent );
	ent->client->resp.team = 0;

	// drop items if they are alive/not observer
	if (ent->solid != SOLID_NOT && !ent->deadflag)
		TossItemsOnDeath(ent);

	FreeClientEdicts(ent->client);

	if (ent->solid == SOLID_TRIGGER)
		RemoveFromTransparentList(ent);

	gi.bprintf(PRINT_HIGH, "%s disconnected\n", ent->client->pers.netname);

	if( !teamplay->value && !ent->client->pers.spectator )
	{
		// send effect
		gi.WriteByte( svc_muzzleflash );
		gi.WriteShort( ent - g_edicts );
		gi.WriteByte( MZ_LOGOUT );
		gi.multicast( ent->s.origin, MULTICAST_PVS );
	}

	if( use_ghosts->value )
		CreateGhost( ent );

	// go clear any clients that have this guy as their attacker
	for (i = 1, etemp = g_edicts + 1; i <= game.maxclients; i++, etemp++) {
		if (etemp->inuse) {
			if (etemp->client && etemp->client->attacker == ent)
				etemp->client->attacker = NULL;
			if (etemp->enemy == ent)	// AQ:TNG - JBravo adding tkok
				etemp->enemy = NULL;
		}
	}

	TourneyRemovePlayer(ent);
	vClientDisconnect(ent);	// client voting disconnect

	if (ctf->value)
		CTFDeadDropFlag(ent);

	PMenu_Close(ent);

	gi.unlinkentity(ent);

	ent->s.modelindex = 0;
	ent->s.modelindex2 = 0;
	ent->s.sound = 0;
	ent->s.event = 0;
	ent->s.effects = 0;
	ent->s.renderfx = 0;
	ent->s.solid = 0;
	ent->solid = SOLID_NOT;
	ent->inuse = false;
	ent->classname = "disconnected";
	ent->svflags = SVF_NOCLIENT;
	ent->client->pers.connected = false;

	teams_changed = true;
}

void CreateGhost(edict_t * ent)
{
	int i;
	gghost_t *ghost;

	if (ent->client->resp.score == 0 && ent->client->resp.damage_dealt == 0) {
		return;
	}

	//check if its already there
	for (i = 0, ghost = ghost_players; i < num_ghost_players; i++, ghost++) {
		if (!strcmp(ghost->ip, ent->client->pers.ip) && !strcmp(ghost->netname, ent->client->pers.netname)) {
			break;
		}
	}

	if (i >= num_ghost_players) {
		if (num_ghost_players >= MAX_GHOSTS) {
			gi.dprintf( "Maximum number of ghosts reached.\n" );
			return;
		}
		ghost = &ghost_players[num_ghost_players++];
	}

	strcpy(ghost->ip, ent->client->pers.ip);
	strcpy(ghost->netname, ent->client->pers.netname);

	ghost->enterframe = ent->client->resp.enterframe;
	ghost->disconnect_frame = level.framenum;

	// Score
	ghost->score = ent->client->resp.score;
	ghost->damage_dealt = ent->client->resp.damage_dealt;
	ghost->kills = ent->client->resp.kills;
	ghost->deaths = ent->client->resp.deaths;
	ghost->ctf_caps = ent->client->resp.ctf_caps;

	// Teamplay variables
	if (teamplay->value) {
		ghost->weapon = ent->client->pers.chosenWeapon;
		ghost->item = ent->client->pers.chosenItem;
		ghost->team = ent->client->resp.team;
	}

	// Statistics
	ghost->shotsTotal = ent->client->resp.shotsTotal;
	ghost->hitsTotal = ent->client->resp.hitsTotal;

	memcpy(ghost->hitsLocations, ent->client->resp.hitsLocations, sizeof(ghost->hitsLocations));
	memcpy(ghost->gunstats, ent->client->resp.gunstats, sizeof(ghost->gunstats));
}

//==============================================================

edict_t *pm_passent;

// pmove doesn't need to know about passent and contentmask
trace_t q_gameabi PM_trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	if (pm_passent && pm_passent->health > 0)
		return gi.trace(start, mins, maxs, end, pm_passent, MASK_PLAYERSOLID);
	else
		return gi.trace(start, mins, maxs, end, pm_passent, MASK_DEADSOLID);
}

// Raptor007: Allow weapon actions to start happening on any frame.
static void ClientThinkWeaponIfReady( edict_t *ent, qboolean update_idle )
{
	int old_weaponstate, old_gunframe;

	// If they just spawned, sync up the weapon animation with that.
	if( ! ent->client->weapon_last_activity )
		ent->client->weapon_last_activity = level.framenum;

	// If it's too soon since the last non-idle think, keep waiting.
	else if( level.framenum < ent->client->weapon_last_activity + game.framediv )
		return;

	// Clear weapon kicks.
	VectorClear( ent->client->kick_origin );
	VectorClear( ent->client->kick_angles );

	old_weaponstate = ent->client->weaponstate;
	old_gunframe = ent->client->ps.gunframe;

	Think_Weapon( ent );

	// If the weapon is or was in any state other than ready, wait before thinking again.
	if( (ent->client->weaponstate != WEAPON_READY) || (old_weaponstate != WEAPON_READY) )
	{
		ent->client->weapon_last_activity = level.framenum;
		ent->client->anim_started = ent->client->weapon_last_activity;
	}

	// Only allow the idle animation to update if it's been enough time.
	else if( ! update_idle || level.framenum % game.framediv != ent->client->weapon_last_activity % game.framediv )
		ent->client->ps.gunframe = old_gunframe;
}

void FrameStartZ( edict_t *ent )
{
#ifndef NO_FPS
	if( (FRAMEDIV == 1) || ! ent->inuse || ! IS_ALIVE(ent) || (ent->z_history_framenum != level.framenum - 1) )
		return;

	// Restore origin[2] from z_history[0] once at the very beginning of the frame.
	ent->s.origin[2] = ent->z_history[0];
	ent->z_history_framenum = 0;
/*
	if( game.serverfeatures & GMF_CLIENTNUM )
		ent->client->ps.pmove.origin[2] = ent->z_pmove;
*/
#endif
}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame.
==============
*/
void ClientThink(edict_t * ent, usercmd_t * ucmd)
{
	gclient_t *client;
	edict_t *other;
	int i, j;
	pmove_t pm;
	char ltm[64] = "\0";

	FrameStartZ( ent );

	level.current_entity = ent;
	client = ent->client;
	
	client->antilag_state.curr_timestamp += (float)ucmd->msec / 1000; // antilag needs sub-server-frame timestamps

	if (level.intermission_framenum) {
		client->ps.pmove.pm_type = PM_FREEZE;
		// 
		if (level.realFramenum > level.intermission_framenum + 4 * HZ) {
			if (ent->inuse && client->resp.stat_mode > 0
			    && client->resp.stat_mode_intermission == 0) {
				client->resp.stat_mode_intermission = 1;
				Cmd_Stats_f(ent, ltm);
			}
		}
		// can exit intermission after five seconds
		if (level.realFramenum > level.intermission_framenum + 5 * HZ && (ucmd->buttons & BUTTON_ANY))
			level.intermission_exit = 1;
		return;
	}

	if (level.pauseFrames > 0)
	{
		client->ps.pmove.pm_type = PM_FREEZE;
		return;
	}
	pm_passent = ent;
	// FROM 3.20 -FB
	if (client->chase_mode) {
		client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
		client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
		client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);
	} else {
		// ^^^
		// set up for pmove
		memset(&pm, 0, sizeof(pm));
		if (ent->movetype == MOVETYPE_NOCLIP)
			client->ps.pmove.pm_type = PM_SPECTATOR;
		else if (ent->s.modelindex != 255)
			client->ps.pmove.pm_type = PM_GIB;
		else if (ent->deadflag)
			client->ps.pmove.pm_type = PM_DEAD;
		else
			client->ps.pmove.pm_type = PM_NORMAL;

		client->ps.pmove.gravity = sv_gravity->value;

		pm.s = client->ps.pmove;
		for (i = 0; i < 3; i++) {
			pm.s.origin[i] = ent->s.origin[i] * 8;
			pm.s.velocity[i] = ent->velocity[i] * 8;
		}

		if (memcmp(&client->old_pmove, &pm.s, sizeof(pm.s))) {
			pm.snapinitial = true;
			//      gi.dprintf ("pmove changed!\n");
		}

		pm.cmd = *ucmd;
		client->cmd_last = *ucmd;

		// Stumbling movement with leg damage.
		// darksaint ETE edit:  if e_enhancedSlippers are enabled/equipped, negate all stumbling
		qboolean has_enhanced_slippers = e_enhancedSlippers->value && INV_AMMO(ent, SLIP_NUM);
		if( client->leg_damage && ent->groundentity && ! has_enhanced_slippers )
		{
			int frame_mod_6 = (level.framenum / game.framediv) % 6;
			if( frame_mod_6 <= 2 )
			{
				pm.cmd.forwardmove = 0;
				pm.cmd.sidemove = 0;
			}
			else if( frame_mod_6 == 3 )
			{
				pm.cmd.forwardmove /= client->leghits + 1;
				pm.cmd.sidemove /= client->leghits + 1;
			}

			// Prevent jumping with leg damage.
			pm.s.pm_flags |= PMF_JUMP_HELD;
		}

		pm.trace = PM_trace;	// adds default parms
		pm.pointcontents = gi.pointcontents;
		// perform a pmove
		gi.Pmove(&pm);

		//FB 6/3/99 - info from Mikael Lindh from AQ:G
		if (pm.maxs[2] == 4) {
			ent->maxs[2] = CROUCHING_MAXS2;
			pm.maxs[2] = CROUCHING_MAXS2;
			ent->viewheight = CROUCHING_VIEWHEIGHT;
			pm.viewheight = (float) ent->viewheight;
		}
		//FB 6/3/99

		// save results of pmove
		client->ps.pmove = pm.s;
		client->old_pmove = pm.s;

		for (i = 0; i < 3; i++) {
			ent->s.origin[i] = pm.s.origin[i] * 0.125;
			ent->velocity[i] = pm.s.velocity[i] * 0.125;
		}

		if( ! client->leg_damage && ent->groundentity && ! pm.groundentity && pm.cmd.upmove >= 10 && pm.waterlevel == 0 )
			ent->client->jumping = 1;

		VectorCopy(pm.mins, ent->mins);
		VectorCopy(pm.maxs, ent->maxs);
		client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
		client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
		client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);

		ent->viewheight = pm.viewheight;
		ent->waterlevel = pm.waterlevel;
		ent->watertype = pm.watertype;

		if( pm.groundentity || ! slopefix->value )
			ent->groundentity = pm.groundentity;
		else if( ent->groundentity && (ent->client->jumping || pm.waterlevel || (ent->velocity[2] > 0) || (ent->velocity[2] < -70) || ! ent->groundentity->inuse) )
			ent->groundentity = NULL;

		if( ent->groundentity )
			ent->groundentity_linkcount = ent->groundentity->linkcount;

		if (ent->deadflag) {
			client->ps.viewangles[ROLL] = 40;
			client->ps.viewangles[PITCH] = -15;
			client->ps.viewangles[YAW] = client->killer_yaw;
		} else {
			VectorCopy(pm.viewangles, client->v_angle);
			VectorCopy(pm.viewangles, client->ps.viewangles);
		}

		if(client->ctf_grapple)
			CTFGrapplePull(client->ctf_grapple);

		gi.linkentity(ent);

		if (ent->movetype != MOVETYPE_NOCLIP)
			G_TouchTriggers(ent);

		// stop manipulating doors
		client->doortoggle = 0;

		if( client->jumping && (ent->solid != SOLID_NOT) && ! lights_camera_action && ! client->uvTime && ! jump->value )
		{
			kick_attack( ent );
			client->punch_desired = false;
		}

		// touch other objects
		for (i = 0; i < pm.numtouch; i++) {
			other = pm.touchents[i];
			for (j = 0; j < i; j++)
				if (pm.touchents[j] == other)
					break;
			if (j != i)
				continue;	// duplicated
			if (!other->touch)
				continue;
			other->touch(other, ent, NULL, NULL);
		}
	}

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;
	// save light level the player is standing on for
	// monster sighting AI
	ent->light_level = ucmd->lightlevel;

	// fire weapon from final position if needed
	if (client->latched_buttons & BUTTON_ATTACK) {
		//TempFile
		//We're gonna fire in this frame? Then abort any punching.
		client->punch_framenum = level.framenum;
		client->punch_desired = false;

		if (ent->solid == SOLID_NOT && ent->deadflag != DEAD_DEAD && !in_warmup) {
			client->latched_buttons = 0;
			NextChaseMode( ent );
		} else {
			ClientThinkWeaponIfReady( ent, false );
		}
	}

	if (client->chase_mode) {
		if (ucmd->upmove >= 10) {
			if (!(client->ps.pmove.pm_flags & PMF_JUMP_HELD)) {
				client->ps.pmove.pm_flags |= PMF_JUMP_HELD;
				if (client->chase_target) {
					ChaseNext(ent);
				} else {
					GetChaseTarget(ent);
				}
			}
		} else {
			client->ps.pmove.pm_flags &= ~PMF_JUMP_HELD;
		}
	}

	if( ucmd->forwardmove || ucmd->sidemove || (client->oldbuttons != client->buttons)
		|| ((ent->solid == SOLID_NOT) && (ent->deadflag != DEAD_DEAD)) ) // No idle noises at round start.
			client->resp.idletime = 0;
	else if( ! client->resp.idletime )
		client->resp.idletime = level.framenum;
}

/*
==============
ClientBeginServerFrame

This will be called once for each server frame, before running
any other entities in the world.
==============
*/
void ClientBeginServerFrame(edict_t * ent)
{
	gclient_t *client;
	int buttonMask, going_observer = 0;

	FrameStartZ( ent );

	client = ent->client;

	if (sv_antilag->value) // if sv_antilag is enabled, we want to track our player position for later reference
		antilag_update(ent);

	if (client->resp.penalty > 0 && level.realFramenum % HZ == 0)
		client->resp.penalty--;

	if (level.intermission_framenum)
		return;

	if( team_round_going && IS_ALIVE(ent) )
		client->resp.motd_refreshes = motd_time->value;  // Stop showing motd if we're playing.
	else if( lights_camera_action )
		client->resp.last_motd_refresh = level.realFramenum;  // Don't interrupt LCA with motd.
	else if ((int)motd_time->value > client->resp.motd_refreshes * 2 && ent->client->layout != LAYOUT_MENU) {
		if (client->resp.last_motd_refresh + 2 * HZ < level.realFramenum) {
			client->resp.last_motd_refresh = level.realFramenum;
			client->resp.motd_refreshes++;
			PrintMOTD( ent );
		}
	}

	// show team or weapon menu immediately when connected
	if (auto_menu->value && ent->client->layout != LAYOUT_MENU && !client->pers.menu_shown && (teamplay->value || dm_choose->value)) {
		Cmd_Inven_f( ent );
	}

	if (!teamplay->value)
	{
		// force spawn when weapon and item selected in dm
		if (!ent->client->pers.spectator && dm_choose->value && !client->pers.dm_selected) {
			if (client->pers.chosenWeapon && (client->pers.chosenItem || itm_flags->value == 0)) {
				client->pers.dm_selected = 1;

				gi.bprintf(PRINT_HIGH, "%s joined the game\n", client->pers.netname);

				respawn(ent);

				if (!(ent->svflags & SVF_NOCLIENT)) { // send effect
					gi.WriteByte(svc_muzzleflash);
					gi.WriteShort(ent - g_edicts);
					gi.WriteByte(MZ_LOGIN);
					gi.multicast(ent->s.origin, MULTICAST_PVS);
				}
			}
			return;
		}

		if (level.framenum > client->respawn_framenum && (ent->solid == SOLID_NOT && ent->deadflag != DEAD_DEAD) != ent->client->pers.spectator)
		{
			if (ent->client->pers.spectator){
				killPlayer(ent, false);
			} else {
				gi.bprintf(PRINT_HIGH, "%s rejoined the game\n", ent->client->pers.netname);
				respawn(ent);
			}
		}
	}

	// run weapon animations if it hasn't been done by a ucmd_t
	ClientThinkWeaponIfReady( ent, true );
	PlayWeaponSound( ent );

	if (ent->deadflag) {
		// wait for any button just going down
		if (level.framenum > client->respawn_framenum)
		{

			if (teamplay->value) {
				going_observer = ((gameSettings & GS_ROUNDBASED) || !client->resp.team || client->resp.subteam);
			}
			else
			{
				going_observer = ent->client->pers.spectator;
				if (going_observer) {
					gi.bprintf(PRINT_HIGH, "%s became a spectator\n", ent->client->pers.netname);
				}
			}

			if (going_observer) {
				CopyToBodyQue(ent);
				ent->solid = SOLID_NOT;
				ent->svflags |= SVF_NOCLIENT;
				ent->movetype = MOVETYPE_NOCLIP;
				ent->health = 100;
				ent->deadflag = DEAD_NO;
				ent->client->ps.gunindex = 0;
				client->ps.pmove.delta_angles[PITCH] = ANGLE2SHORT(0 - client->resp.cmd_angles[PITCH]);
				client->ps.pmove.delta_angles[YAW] = ANGLE2SHORT(client->killer_yaw - client->resp.cmd_angles[YAW]);
				client->ps.pmove.delta_angles[ROLL] = ANGLE2SHORT(0 - client->resp.cmd_angles[ROLL]);
				ent->s.angles[PITCH] = 0;
				ent->s.angles[YAW] = client->killer_yaw;
				ent->s.angles[ROLL] = 0;
				VectorCopy(ent->s.angles, client->ps.viewangles);
				VectorCopy(ent->s.angles, client->v_angle);
				gi.linkentity(ent);

				if (teamplay->value && !in_warmup && limchasecam->value) {
					ent->client->chase_mode = 0;
					NextChaseMode( ent );
				}
			}
			else
			{
				// in deathmatch, only wait for attack button
				buttonMask = BUTTON_ATTACK;
				if ((client->latched_buttons & buttonMask) || DMFLAGS(DF_FORCE_RESPAWN)) {
					respawn(ent);
					client->latched_buttons = 0;
				}
			}
		}
		return;
	}

	if (ent->solid != SOLID_NOT)
	{
		int idleframes = client->resp.idletime ? (level.framenum - client->resp.idletime) : 0;

		if( client->punch_desired && ! client->jumping && ! lights_camera_action && ! client->uvTime )
			punch_attack( ent );
		client->punch_desired = false;

		if( (ppl_idletime->value > 0) && idleframes && (idleframes % (int)(ppl_idletime->value * HZ) == 0) )
			//plays a random sound/insane sound, insane1-9.wav
			gi.sound( ent, CHAN_VOICE, gi.soundindex(va( "insane/insane%i.wav", rand() % 9 + 1 )), 1, ATTN_NORM, 0 );

		if( (sv_idleremove->value > 0) && (idleframes > (sv_idleremove->value * HZ)) && client->resp.team )
		{
			// Removes member from team once sv_idleremove value in seconds has been reached
			int idler_team = client->resp.team;
			if( teamplay->value )
				LeaveTeam( ent );
			if( matchmode->value )
			{
				MM_LeftTeam( ent );
				teams[ idler_team ].ready = 0;
			}
			client->resp.idletime = 0;
			gi.dprintf( "%s has been removed from play due to reaching the sv_idleremove timer of %i seconds\n",
				client->pers.netname, (int) sv_idleremove->value );
		}

		if (client->autoreloading && (client->weaponstate == WEAPON_END_MAG)
			&& (client->curr_weap == MK23_NUM)) {
			client->autoreloading = false;
			Cmd_New_Reload_f( ent );
		}

		if (client->uvTime && FRAMESYNC) {
			client->uvTime--;
			if (!client->uvTime)
			{
				if (team_round_going)
				{
					if (ctf->value && ctfgame.type == 2) {
						gi.centerprintf(ent,
							"ACTION!\n"
							"\n"
							"You are %s the %s base!",
							(client->resp.team == ctfgame.offence ?
							"ATTACKING" : "DEFENDING"),
							CTFOtherTeamName(ctfgame.offence));
					}
					else {
						gi.centerprintf(ent, "ACTION!");
					}
				}
			}
			else if (client->uvTime % 10 == 0)
			{
				if (ctf->value && ctfgame.type == 2) {
					gi.centerprintf(ent,
						"Shield %d\n"
						"\n"
						"You are %s the %s base!",
						client->uvTime / 10,
						(client->resp.team == ctfgame.offence ?
						"ATTACKING" : "DEFENDING"),
						CTFOtherTeamName(ctfgame.offence));
				}
				else {
					gi.centerprintf(ent, "Shield %d", client->uvTime / 10);
				}
			}
		}
	}

	if (!in_warmup || ent->movetype != MOVETYPE_NOCLIP)
		client->latched_buttons = 0;
}
