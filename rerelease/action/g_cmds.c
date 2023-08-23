//-----------------------------------------------------------------------------
// g_cmds.c
//
// $Id: g_cmds.c,v 1.60 2004/04/08 23:19:51 slicerdw Exp $
//
//-----------------------------------------------------------------------------
// $Log: g_cmds.c,v $
// Revision 1.60  2004/04/08 23:19:51  slicerdw
// Optimized some code, added a couple of features and fixed minor bugs
//
// Revision 1.59  2003/06/15 21:43:53  igor
// added IRC client
//
// Revision 1.58  2003/06/15 15:34:32  igor
// - removed the zcam code from this branch (see other branch)
// - added fixes from 2.72 (source only) version
// - resetted version number to 2.72
// - This version should be exactly like the release 2.72 - just with a few
//   more fixes (which whoever did the source only variant didn't get because
//   he didn't use the CVS as he should. Shame on him.
//
// Revision 1.57  2002/09/04 11:23:09  ra
// Added zcam to TNG and bumped version to 3.0
//
// Revision 1.56  2002/03/28 11:46:03  freud
// stat_mode 2 and timelimit 0 did not show stats at end of round.
// Added lock/unlock.
// A fix for use_oldspawns 1, crash bug.
//
// Revision 1.55  2002/03/26 21:49:01  ra
// Bufferoverflow fixes
//
// Revision 1.54  2002/03/25 18:32:11  freud
// I'm being too productive.. New ghost command needs testing.
//
// Revision 1.53  2002/02/19 09:32:47  freud
// Removed PING PONGs from CVS, not fit for release.
//
// Revision 1.52  2002/02/18 20:21:36  freud
// Added PING PONG mechanism for timely disconnection of clients. This is
// based on a similar scheme as the scheme used by IRC. The client has
// cvar ping_timeout seconds to reply or will be disconnected.
//
// Revision 1.51  2002/02/13 12:13:00  deathwatch
// INVDROP Weaponfarming fix
//
// Revision 1.50  2002/02/01 12:54:08  ra
// messin with stat_mode
//
// Revision 1.49  2002/01/24 02:55:58  ra
// Fixed the mm_forceteamtalk 2 bug.
//
// Revision 1.48  2002/01/24 02:24:56  deathwatch
// Major update to Stats code (thanks to Freud)
// new cvars:
// stats_afterround - will display the stats after a round ends or map ends
// stats_endmap - if on (1) will display the stats scoreboard when the map ends
//
// Revision 1.47  2002/01/24 01:40:40  deathwatch
// Freud's AutoRecord
//
// Revision 1.46  2001/12/24 18:06:05  slicerdw
// changed dynamic check for darkmatch only
//
// Revision 1.44  2001/12/09 14:02:11  slicerdw
// Added gl_clear check -> video_check_glclear cvar
//
// Revision 1.43  2001/11/29 16:04:29  deathwatch
// Fixed the playerlist command
//
// Revision 1.42  2001/11/16 22:45:58  deathwatch
// Fixed %me again
//
// Revision 1.41  2001/11/09 23:58:12  deathwatch
// Fiixed the %me command to display '[DEAD]' properly
// Added the resetting of the teamXscore cvars at the exitlevel function where the team scores are reset as well. (not sure if that is correct)
//
// Revision 1.40  2001/11/03 20:10:18  slicerdw
// Removed the if that prevented people from talking at the end
//
// Revision 1.39  2001/11/03 17:43:20  deathwatch
// Fixed matchadmin - it should only work when in matchmode and not use say when typing it normally in any other mode (security issue for dumb ppl typing matchadmin password on a server without matchmode)
//
// Revision 1.38  2001/11/03 17:21:57  deathwatch
// Fixed something in the time command, removed the .. message from the voice command, fixed the vote spamming with mapvote, removed addpoint command (old pb command that wasnt being used). Some cleaning up of the source at a few points.
//
// Revision 1.37  2001/10/18 12:55:35  deathwatch
// Added roundtimeleft
//
// Revision 1.36  2001/09/30 03:09:34  ra
// Removed new stats at end of rounds and created a new command to
// do the same functionality.   Command is called "time"
//
// Revision 1.35  2001/09/28 22:00:46  deathwatch
// changed pheer to ph34rs in the kill denying statement
//
// Revision 1.34  2001/09/28 21:43:21  deathwatch
// Fixed a but caused due to the reformatting of the source making the say stuff not working in non-matchmode modes
//
// Revision 1.33  2001/09/28 14:20:25  slicerdw
// Few tweaks..
//
// Revision 1.32  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.31  2001/09/28 13:44:23  slicerdw
// Several Changes / improvements
//
// Revision 1.30  2001/08/20 00:41:15  slicerdw
// Added a new scoreboard for Teamplay with stats ( when map ends )
//
// Revision 1.29  2001/08/18 18:45:19  deathwatch
// Edited the Flashlight movement code to the Lasersight's movement code, its probably better
// and I added checks for darkmatch/being dead/being a spectator for its use
//
// Revision 1.28  2001/08/18 17:14:04  deathwatch
// Flashlight Added (not done yet, needs to prevent DEAD ppl from using it,
// the glow should be white and a bit smaller if possible and the daiper needs
// to be gone. Also, it should only work in 'darkmatch' I guess and it should
// make a sound when you turn it on/off.
//
// Revision 1.27  2001/08/17 21:31:37  deathwatch
// Added support for stats
//
// Revision 1.26  2001/08/08 12:42:22  slicerdw
// Ctf Should finnaly be fixed now, lets hope so
//
// Revision 1.25  2001/07/30 16:07:25  igor_rock
// added correct gender to "pheer" message
//
// Revision 1.24  2001/07/28 19:30:05  deathwatch
// Fixed the choose command (replaced weapon for item when it was working with items)
// and fixed some tabs on other documents to make it more readable
//
// Revision 1.23  2001/07/20 11:56:04  slicerdw
// Added a check for the players spawning during countdown on ctf ( lets hope it works )
//
// Revision 1.21  2001/06/25 12:39:38  slicerdw
// Cleaning up something i left behind..
//
// Revision 1.20  2001/06/25 11:44:47  slicerdw
// New Video Check System - video_check and video_check_lockpvs no longer latched
//
// Revision 1.19  2001/06/22 16:34:05  slicerdw
// Finished Matchmode Basics, now with admins, Say command tweaked...
//
// Revision 1.18  2001/06/21 00:05:30  slicerdw
// New Video Check System done -  might need some revision but works..
//
// Revision 1.15  2001/06/06 18:57:14  slicerdw
// Some tweaks on Ctf and related things
//
// Revision 1.12  2001/06/01 19:18:42  slicerdw
// Added Matchmode Code
//
// Revision 1.11  2001/06/01 08:25:42  igor_rock
// Merged New_Ctf-1_0 into main branch - this time hopefuly the whole one...
//
// Revision 1.10  2001/05/31 16:58:14  igor_rock
// conflicts resolved
//
// Revision 1.9  2001/05/20 15:00:19  slicerdw
// Some minor fixes and changings on Video Checking system
//
// Revision 1.8.2.4  2001/05/31 06:47:51  igor_rock
// - removed crash bug with non exisitng flag files
// - added new commands "setflag1", "setflag2" and "saveflags" to create
//   .flg files
//
// Revision 1.8.2.3  2001/05/27 13:33:37  igor_rock
// added flag drop command ("drop flag")
//
// Revision 1.8.2.2  2001/05/25 18:59:52  igor_rock
// Added CTF Mode completly :)
// Support for .flg files is still missing, but with "real" CTF maps like
// tq2gtd1 the ctf works fine.
// (I hope that all other modes still work, just tested DM and teamplay)
//
// Revision 1.8.2.1  2001/05/20 15:17:31  igor_rock
// removed the old ctf code completly
//
// Revision 1.8  2001/05/13 14:55:11  igor_rock
// corrected the lens command which was commented out in error
//
// Revision 1.7  2001/05/12 21:19:51  ra
//
//
// Added punishkills.
//
// Revision 1.6  2001/05/11 16:07:25  mort
// Various CTF bits and pieces...
//
// Revision 1.5  2001/05/11 12:21:19  slicerdw
// Commented old Location support ( ADF ) With the ML/ETE Compatible one
//
// Revision 1.4  2001/05/07 21:18:34  slicerdw
// Added Video Checking System
//
// Revision 1.3  2001/05/07 02:05:36  ra
//
//
// Added tkok command to forgive teamkills.
//
// Revision 1.2  2001/05/07 01:38:51  ra
//
//
// Added fixes for Ammo and Weaponsfarming.
//
// Revision 1.1.1.1  2001/05/06 17:30:36  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#include "g_local.h"
#include "m_player.h"

qboolean FloodCheck (edict_t *ent)
{
	if (flood_threshold->value)
	{
		ent->client->resp.penalty++;

		if (ent->client->resp.penalty > flood_threshold->value)
		{
			gi.cprintf(ent, PRINT_HIGH, "You can't talk for %d seconds.\n", ent->client->resp.penalty - (int)flood_threshold->value);
			return 1;
		}
	}

	return 0;
}

/*
==============
LookupPlayer
==============
Look up a player by partial subname, full name or client id. If multiple
matches, show a list. Return NULL on failure. Case insensitive.
*/
edict_t *LookupPlayer(edict_t *ent, const char *text, qboolean checkNUM, qboolean checkNick)
{
	edict_t		*p = NULL, *entMatch = NULL;
	int			i, matchCount, numericMatch;
	char		match[32];
	const char	*m, *name;

	if (!text[0])
		return NULL;

	Q_strncpyz(match, text, sizeof(match));
	matchCount = numericMatch = 0;

	m = match;

	while (*m) {
		if (!Q_isdigit(*m)) {
			break;
		}
		m++;
	}

	if (!*m && checkNUM)
	{
		numericMatch = atoi(match);
		if (numericMatch < 0 || numericMatch >= game.maxclients)
		{
			if (ent)
				gi.cprintf(ent, PRINT_HIGH, "Invalid client id %d.\n", numericMatch);
			return 0;
		}

		p = g_edicts + 1 + numericMatch;
		if (!p->inuse || !p->client || p->client->pers.mvdspec) {
			if (ent)
				gi.cprintf(ent, PRINT_HIGH, "Client %d is not active.\n", numericMatch);
			return NULL;
		}

		return p;
	}

	if (!checkNick) {
		if (ent)
			gi.cprintf(ent, PRINT_HIGH, "Invalid client id '%s'\n", match);

		return NULL;
	}

	for (i = 0, p = g_edicts + 1; i < game.maxclients; i++, p++)
	{
		if (!p->inuse || !p->client || p->client->pers.mvdspec)
			continue;

		name = p->client->pers.netname;
		if (!Q_stricmp(name, match)) { //Exact match
			return p;
		}

		if (Q_stristr(name, match)) {
			matchCount++;
			entMatch = p;
			continue;
		}
	}

	if (matchCount == 1) {
		return entMatch;
	}

	if (matchCount > 1)
	{
		if (ent)
		{
			gi.cprintf(ent, PRINT_HIGH, "'%s' matches multiple players:\n", match);
			for (i = 0, p = g_edicts + 1; i < game.maxclients; i++, p++) {
				if (!p->inuse || !p->client || p->client->pers.mvdspec)
					continue;

				name = p->client->pers.netname;
				if (Q_stristr(name, match)) {
					gi.cprintf(ent, PRINT_HIGH, "%3d. %s\n", i, name);
				}
			}
		}
		return NULL;
	}

	if (ent)
		gi.cprintf(ent, PRINT_HIGH, "No player match found for '%s'\n", match);

	return NULL;
}

char *ClientTeam (edict_t * ent)
{
	char *p;
	static char value[128];

	value[0] = 0;

	if (!ent->client)
		return value;

	Q_strncpyz(value, Info_ValueForKey (ent->client->pers.userinfo, "skin"), sizeof(value));
	p = strchr (value, '/');
	if (!p)
		return value;

	if (DMFLAGS(DF_MODELTEAMS))
	{
		*p = 0;
		return value;
	}

	// if (DMFLAGS(DF_SKINTEAMS))
	return ++p;
}

qboolean OnSameTeam (edict_t * ent1, edict_t * ent2)
{
	char ent1Team[128], ent2Team[128];

	//FIREBLADE
	if (!ent1->client || !ent2->client)
		return false;

	if (teamplay->value)
		return ent1->client->resp.team == ent2->client->resp.team;
	//FIREBLADE

	if (!DMFLAGS( (DF_MODELTEAMS | DF_SKINTEAMS) ))
		return false;

	Q_strncpyz (ent1Team, ClientTeam(ent1), sizeof(ent1Team));
	Q_strncpyz (ent2Team, ClientTeam(ent2), sizeof(ent2Team));

	if (strcmp (ent1Team, ent2Team) == 0)
		return true;

	return false;
}


static void SelectNextItem (edict_t * ent, int itflags)
{
	gclient_t *cl;
	int i, index;
	gitem_t *it;

	cl = ent->client;

	if (cl->layout == LAYOUT_MENU) {
		PMenu_Next(ent);
		return;
	}

	if (ent->solid == SOLID_NOT && ent->deadflag != DEAD_DEAD)
		return;

	// scan  for the next valid one
	for (i = 1; i <= MAX_ITEMS; i++)
	{
		index = (cl->selected_item + i) % MAX_ITEMS;
		if (!cl->inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->selected_item = index;
		return;
	}

	cl->selected_item = -1;
}

static void SelectPrevItem (edict_t * ent, int itflags)
{
	gclient_t *cl;
	int i, index;
	gitem_t *it;

	cl = ent->client;

	if (cl->layout == LAYOUT_MENU) {
		PMenu_Prev (ent);
		return;
	}

	if (ent->solid == SOLID_NOT && ent->deadflag != DEAD_DEAD)
		return;

	// scan  for the next valid one
	for (i = 1; i <= MAX_ITEMS; i++)
	{
		index = (cl->selected_item + MAX_ITEMS - i) % MAX_ITEMS;
		if (!cl->inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->selected_item = index;
		return;
	}

	cl->selected_item = -1;
}

void ValidateSelectedItem (edict_t * ent)
{
	gclient_t *cl;

	cl = ent->client;

	if (cl->inventory[cl->selected_item])
		return;			// valid

	SelectNextItem (ent, -1);
}


//=================================================================================

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
static void Cmd_Give_f (edict_t * ent)
{
	char *name;
	char fixedname[32];
	gitem_t *it;
	int index;
	int i;
	qboolean give_all;
	edict_t *it_ent;
	edict_t etemp;

	if (ent->solid == SOLID_NOT) {
		gi.cprintf(ent, PRINT_HIGH, "This command can't be used by spectators.\n");
		return;
	}

	Q_strncpyz(fixedname, gi.args (), sizeof(fixedname));
	name = fixedname;
//  name = gi.args ();

	if (Q_stricmp (name, "all") == 0)
		give_all = true;
	else
		give_all = false;

	if (Q_stricmp (gi.argv (1), "health") == 0)
	{
		/*    if (gi.argc() == 3)
		ent->health = atoi(gi.argv(2));
		else
		ent->health = ent->max_health;
		if (!give_all) */
		return;
	}

	if (give_all || Q_stricmp (name, "weapons") == 0)
	{
		for (i = 0; i < game.num_items; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_WEAPON))
				continue;
			ent->client->inventory[i] += 1;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_stricmp (name, "items") == 0)
	{
		for (i = 0; i < game.num_items; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_ITEM))
				continue;
			etemp.item = it;

			if (ent->client->unique_item_total >= unique_items->value)
				ent->client->unique_item_total = unique_items->value - 1;

			Pickup_Special (&etemp, ent);
		}
		if (!give_all)
			return;
	}


	if (give_all || Q_stricmp (name, "ammo") == 0)
	{
		ent->client->mk23_rds = ent->client->mk23_max;
		ent->client->dual_rds = ent->client->dual_max;
		ent->client->mp5_rds = ent->client->mp5_max;
		ent->client->m4_rds = ent->client->m4_max;
		ent->client->shot_rds = ent->client->shot_max;
		ent->client->sniper_rds = ent->client->sniper_max;
		ent->client->cannon_rds = ent->client->cannon_max;

		for (i = 0; i < game.num_items; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IT_AMMO))
				continue;
			Add_Ammo (ent, it, 1000);
		}
		if (!give_all)
			return;
	}

	if (Q_stricmp (name, "armor") == 0)
	{
		/*
		gitem_armor_t   *info;

		it = FindItem("Jacket Armor");
		ent->client->inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Combat Armor");
		ent->client->inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Body Armor");
		info = (gitem_armor_t *)it->info;
		ent->client->inventory[ITEM_INDEX(it)] = info->max_count;

		if (!give_all)
		*/
		return;
	}

	if (Q_stricmp (name, "Power Shield") == 0)
	{
		/*it = FindItem("Power Shield");
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
		G_FreeEdict(it_ent);

		if (!give_all)
		*/
		return;
	}

	/*if (give_all)
	{
		for (i=0 ; i<game.num_items ; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (it->flags & (IT_ARMOR|IT_WEAPON|IT_AMMO))
				continue;
			ent->client->inventory[i] = 1;
		}
		return;
	} */

	if (give_all)
		return;

	it = FindItem (name);
	if (!it)
	{
		Q_strncpyz(fixedname, gi.argv (1), sizeof(fixedname));
		name = fixedname;
		//      name = gi.argv (1);
		it = FindItem (name);
		if (!it)
		{
			gi.dprintf ("unknown item\n");
			return;
		}
	}

	if (!(it->flags & (IT_AMMO|IT_WEAPON|IT_ITEM)))
		return;


	if (!it->pickup)
	{
		gi.dprintf ("non-pickup item\n");
		return;
	}


	index = ITEM_INDEX (it);

	if (it->flags & IT_AMMO)
	{
		/*  if (gi.argc() == 5)
			ent->client->inventory[index] = atoi(gi.argv(4));
		else if ( (gi.argc() == 4)  && !(Q_stricmp(it->pickup_name, "12 Gauge Shells")) )
			ent->client->inventory[index] = atoi(gi.argv(3));
		else */
			ent->client->inventory[index] += it->quantity;
	}
	else if (it->flags & IT_ITEM)
	{
		etemp.item = it;
		if (ent->client->unique_item_total >= unique_items->value)
			ent->client->unique_item_total = unique_items->value - 1;

		Pickup_Special (&etemp, ent);
	}
	else
	{
		it_ent = G_Spawn ();
		it_ent->classname = it->classname;
		it_ent->typeNum = it->typeNum;
		SpawnItem (it_ent, it);
		Touch_Item (it_ent, ent, NULL, NULL);
		if (it_ent->inuse)
			G_FreeEdict (it_ent);
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
static void Cmd_God_f (edict_t * ent)
{
	char *msg;

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE))
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
static void Cmd_Notarget_f (edict_t * ent)
{
	char *msg;

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET))
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
static void Cmd_Noclip_f (edict_t * ent)
{
	char *msg;

	if (ent->movetype == MOVETYPE_NOCLIP)
	{
		ent->movetype = MOVETYPE_WALK;
		msg = "noclip OFF\n";
	}
	else
	{
		ent->movetype = MOVETYPE_NOCLIP;
		msg = "noclip ON\n";
	}

	gi.cprintf (ent, PRINT_HIGH, msg);
}


/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
static void Cmd_Use_f (edict_t * ent)
{
	gitem_t *it = NULL;
	char *s;
	int itemNum = 0;

	s = gi.args();

	if (!*s || (ent->solid == SOLID_NOT && ent->deadflag != DEAD_DEAD)) {
		gi.cprintf(ent, PRINT_HIGH, "Unknown item: %s\n", s);
		return;
	}

	//zucc - check for "special"
	if (!Q_stricmp(s, "special")) {
		ReadySpecialWeapon(ent);
		return;
	}

	if (!Q_stricmp(s, "throwing combat knife"))
	{
		if (ent->client->curr_weap != KNIFE_NUM)
		{
			ent->client->pers.knife_mode = 1;
		}
		else // switch to throwing mode if a knife is already out
		{
			//if(!ent->client->pers.knife_mode)
				Cmd_New_Weapon_f(ent);
		}
		itemNum = KNIFE_NUM;
	}
	else if (!Q_stricmp(s, "slashing combat knife"))
	{
		if (ent->client->curr_weap != KNIFE_NUM)
		{
			ent->client->pers.knife_mode = 0;
		}
		else // switch to slashing mode if a knife is already out
		{
			//if(ent->client->pers.knife_mode)
				Cmd_New_Weapon_f(ent);
		}
		itemNum = KNIFE_NUM;
	}

	if (!itemNum) {
		itemNum = GetWeaponNumFromArg(s);
		if (!itemNum) //Check Q2 weapon names
		{
			if (!Q_stricmp(s, "blaster"))
				itemNum = MK23_NUM;
			else if (!Q_stricmp(s, "railgun"))
				itemNum = DUAL_NUM;
			else if (!Q_stricmp(s, "machinegun"))
				itemNum = HC_NUM;
			else if (!Q_stricmp(s, "super shotgun"))
				itemNum = MP5_NUM;
			else if (!Q_stricmp(s, "chaingun"))
				itemNum = SNIPER_NUM;
			else if (!Q_stricmp(s, "bfg10k"))
				itemNum = KNIFE_NUM;
			else if (!Q_stricmp(s, "grenade launcher"))
				itemNum = M4_NUM;
			else if (!Q_stricmp(s, "grenades"))
				itemNum = GRENADE_NUM;
		}
	}

	if (itemNum)
		it = GET_ITEM(itemNum);
	else
		it = FindItem(s);

	if (!it) {
		gi.cprintf(ent, PRINT_HIGH, "Unknown item: %s\n", s);
		return;
	}

	if (!it->use) {
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}

	if (!ent->client->inventory[ITEM_INDEX(it)]) {
		//gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	ent->client->autoreloading = false;
	it->use(ent, it);
}

/*
==================
Cmd_Drop_f

Drop an inventory item
==================
*/
static void Cmd_Drop_f (edict_t * ent)
{
	int index;
	gitem_t *it;
	char *s;

	if (!IS_ALIVE(ent))
		return;

	s = gi.args ();

	//zucc check to see if the string is weapon
	if (Q_stricmp (s, "weapon") == 0)
	{
		DropSpecialWeapon (ent);
		return;
	}

	//zucc now for item
	if (Q_stricmp (s, "item") == 0)
	{
		DropSpecialItem (ent);
		return;
	}

	if (Q_stricmp (s, "flag") == 0)
	{
		CTFDrop_Flag (ent, NULL);
		return;
	}

	// AQ:TNG - JBravo fixing ammo clip farming
	if (ent->client->weaponstate == WEAPON_RELOADING)
		return;
	// Ammo clip farming fix end

	it = FindItem (s);
	if (!it)
	{
		gi.cprintf (ent, PRINT_HIGH, "unknown item: %s\n", s);
		return;
	}
	if (!it->drop)
	{
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	index = ITEM_INDEX (it);
	if (!ent->client->inventory[index])
	{
		gi.cprintf (ent, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->drop (ent, it);
}


/*
=================
Cmd_Inven_f
=================
*/
void Cmd_Inven_f (edict_t * ent)
{
	int i;
	gclient_t *cl;

	cl = ent->client;

	if (cl->layout == LAYOUT_MENU) {
		PMenu_Close(ent);
		return;
	}

	if (cl->showinventory) {
		cl->showinventory = false;
		return;
	}

	cl->pers.menu_shown = true;

	if (teamplay->value && !ent->client->resp.team) {
		OpenJoinMenu (ent);
		return;
	}

	if (gameSettings & GS_WEAPONCHOOSE) {
		OpenWeaponMenu(ent);
		return;
	}

	if (teamplay->value)
		return;

	cl->showinventory = true;

	gi.WriteByte(svc_inventory);
	for (i = 0; i < MAX_ITEMS; i++) {
		gi.WriteShort(cl->inventory[i]);
	}
	gi.unicast(ent, true);
}

/*
=================
Cmd_InvUse_f
=================
*/
void Cmd_InvUse_f (edict_t * ent)
{
	gitem_t *it;

	if (ent->client->layout == LAYOUT_MENU) {
		PMenu_Select(ent);
		return;
	}

	if (!IS_ALIVE(ent))
		return;

	ValidateSelectedItem (ent);

	if (ent->client->selected_item < 1) {
		gi.cprintf(ent, PRINT_HIGH, "No item to use.\n");
		return;
	}

	it = &itemlist[ent->client->selected_item];
	if (!it->use) {
		gi.cprintf (ent, PRINT_HIGH, "Item is not usable.\n");
		return;
	}
	it->use (ent, it);
}

/*
=================
Cmd_WeapPrev_f
=================
*/
static void Cmd_WeapPrev_f (edict_t * ent)
{
	gclient_t *cl;
	int i, index;
	gitem_t *it;
	int selected_weapon;

	if (!IS_ALIVE(ent))
		return;

	cl = ent->client;

	if (!cl->weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->weapon);

	// scan  for the next valid one
	for (i = 1; i <= MAX_ITEMS; i++)
	{
		index = (selected_weapon + i) % MAX_ITEMS;
		if (!cl->inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & IT_WEAPON))
			continue;
		it->use (ent, it);
		if (cl->weapon == it)
			return;			// successful
	}
}

/*
=================
Cmd_WeapNext_f
=================
*/
static void Cmd_WeapNext_f (edict_t * ent)
{
	gclient_t *cl;
	int i, index;
	gitem_t *it;
	int selected_weapon;

	if (!IS_ALIVE(ent))
		return;

	cl = ent->client;

	if (!cl->weapon)
		return;

	selected_weapon = ITEM_INDEX(cl->weapon);

	// scan  for the next valid one
	for (i = 1; i <= MAX_ITEMS; i++)
	{
		index = (selected_weapon + MAX_ITEMS - i) % MAX_ITEMS;
		if (!cl->inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & IT_WEAPON))
			continue;
		it->use (ent, it);
		if (cl->weapon == it)
			return;			// successful
	}
}

/*
=================
Cmd_WeapLast_f
=================
*/
static void Cmd_WeapLast_f (edict_t * ent)
{
	gclient_t *cl;
	int index;
	gitem_t *it;

	if (!IS_ALIVE(ent))
		return;

	cl = ent->client;

	if (!cl->weapon || !cl->lastweapon)
		return;

	index = ITEM_INDEX(cl->lastweapon);
	if (!cl->inventory[index])
		return;
	it = &itemlist[index];
	if (!it->use)
		return;
	if (!(it->flags & IT_WEAPON))
		return;
	it->use (ent, it);
}

/*
=================
Cmd_InvDrop_f
=================
*/
static void Cmd_InvDrop_f (edict_t * ent)
{
	gitem_t *it;

	// TNG: Temp fix for INVDROP weapon farming
	if(teamplay->value)
		return;
	// TNG: End

	if (!IS_ALIVE(ent))
		return;

	ValidateSelectedItem(ent);

	if (ent->client->selected_item < 1) {
		gi.cprintf(ent, PRINT_HIGH, "No item to drop.\n");
		return;
	}

	it = &itemlist[ent->client->selected_item];
	if (!it->drop) {
		gi.cprintf (ent, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}
	it->drop(ent, it);
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f( edict_t *ent )
{
	//FIREBLADE
	if (!IS_ALIVE(ent))
		return;
	
	if ((level.framenum - ent->client->respawn_framenum) < 5 * HZ)
		return;

	killPlayer(ent, true);
}

/*
=================
Cmd_PutAway_f
=================
*/
void Cmd_PutAway_f(edict_t *ent)
{
	if (ent->client->layout == LAYOUT_MENU)
		PMenu_Close(ent);

	ent->client->layout = LAYOUT_NONE;
}

/*
=================
Cmd_Players_f
=================
*/
static void Cmd_Players_f (edict_t * ent)
{
	int i;
	int count = 0;
	char small[64];
	char large[1024];
	gclient_t *sortedClients[MAX_CLIENTS], *cl;


	if (!teamplay->value || !noscore->value)
		count = G_SortedClients( sortedClients );
	else
		count = G_NotSortedClients( sortedClients );

	// print information
	large[0] = 0;

	for (i = 0; i < count; i++)
	{
		cl = sortedClients[i];
		if (!teamplay->value || !noscore->value)
			Com_sprintf (small, sizeof (small), "%3i %s\n",
				cl->ps.stats[STAT_FRAGS],
				cl->pers.netname );
		else
			Com_sprintf (small, sizeof (small), "%s\n",
				cl->pers.netname);

		if (strlen(small) + strlen(large) > sizeof (large) - 20)
		{			// can't print all of them in one packet
			strcat (large, "...\n");
			break;
		}
		strcat (large, small);
	}

	gi.cprintf(ent, PRINT_HIGH, "%s\n%i players\n", large, count);
}

/*
=================
Cmd_Wave_f
=================
*/
static void Cmd_Wave_f (edict_t * ent)
{
	int i;

	// can't wave when ducked
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		return;

	if (ent->client->anim_priority > ANIM_WAVE)
		return;

	if(wave_time->value > 0)
	{
		if(ent->client->resp.lastWave + ((int)wave_time->value * HZ) > level.framenum)
			return;

		ent->client->resp.lastWave = level.framenum;
	}

	i = atoi (gi.argv (1));

	switch (i)
	{
	case 0:
		gi.cprintf (ent, PRINT_HIGH, "flipoff\n");
		SetAnimation( ent, FRAME_flip01 - 1, FRAME_flip12, ANIM_WAVE );
		break;
	case 1:
		gi.cprintf (ent, PRINT_HIGH, "salute\n");
		SetAnimation( ent, FRAME_salute01 - 1, FRAME_salute11, ANIM_WAVE );
		break;
	case 2:
		gi.cprintf (ent, PRINT_HIGH, "taunt\n");
		SetAnimation( ent, FRAME_taunt01 - 1, FRAME_taunt17, ANIM_WAVE );
		break;
	case 3:
		gi.cprintf (ent, PRINT_HIGH, "wave\n");
		SetAnimation( ent, FRAME_wave01 - 1, FRAME_wave11, ANIM_WAVE );
		break;
	case 4:
	default:
		gi.cprintf (ent, PRINT_HIGH, "point\n");
		SetAnimation( ent, FRAME_point01 - 1, FRAME_point12, ANIM_WAVE );
		break;
	}
}

/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f (edict_t * ent, qboolean team, qboolean arg0, qboolean partner_msg)
{
	int j, offset_of_text;
	edict_t *other;
	char *args, text[256], *s;
	int meing = 0, isadmin = 0;
	qboolean show_info = false;

	if (gi.argc() < 2 && !arg0)
		return;
	
	args = gi.args();
	if (!args)
		return;

	if (!sv_crlf->value)
	{
		if (strchr(args, '\r') || strchr(args, '\n'))
		{
			gi.cprintf (ent, PRINT_HIGH, "No control characters in chat messages!\n");
			return;
		}
	}

	//TempFile - BEGIN
	if (arg0)
	{
		if (!Q_stricmp("%me", gi.argv(0))) {
			meing = 4;
			if (!*args)
				return;
		}
	}
	else
	{
		if (!*args)
			return;

		if (!Q_strnicmp("%me", args, 3))
			meing = 4;
		else if (!Q_strnicmp("%me", args + 1, 3))
			meing = 5;

		if(meing)
		{
			if (!*(args+meing-1))
				return;
			if (*(args+meing-1) != ' ')
				meing = 0;
			else if(!*(args+meing))
				return;
		}
	}
	//TempFile - END

	if (!teamplay->value)
	{
		//FIREBLADE
		if (!DMFLAGS( (DF_MODELTEAMS | DF_SKINTEAMS) ))
			team = false;
	}
	else if (matchmode->value)
	{	
		if (ent->client->pers.admin)
			isadmin = 1;

		if (mm_forceteamtalk->value == 1)
		{
			if (!IS_CAPTAIN(ent) && !partner_msg && !isadmin)
				team = true;
		}
		else if (mm_forceteamtalk->value == 2)
		{
			if (!IS_CAPTAIN(ent) && !partner_msg && !isadmin &&
				(TeamsReady() || team_round_going))
				team = true;
		}
	}

	if (team)
	{
		if (ent->client->resp.team == NOTEAM)
		{
			gi.cprintf (ent, PRINT_HIGH, "You're not on a team.\n");
			return;
		}
		if (!meing)		// TempFile
			Com_sprintf (text, sizeof (text), "%s(%s): ",
			(teamplay->value && !IS_ALIVE(ent)) ? "[DEAD] " : "",
			ent->client->pers.netname);
		//TempFile - BEGIN
		else
			Com_sprintf (text, sizeof (text), "(%s%s ",
			(teamplay->value && !IS_ALIVE(ent)) ? "[DEAD] " : "",
			ent->client->pers.netname);
		//TempFile - END
	}
	else if (partner_msg)
	{
		if (ent->client->resp.radio.partner == NULL)
		{
			gi.cprintf (ent, PRINT_HIGH, "You don't have a partner.\n");
			return;
		}
		if (!meing)		//TempFile
			Com_sprintf (text, sizeof (text), "[%sPARTNER] %s: ",
			(teamplay->value && !IS_ALIVE(ent)) ? "DEAD " : "",
			ent->client->pers.netname);
		//TempFile - BEGIN
		else
			Com_sprintf (text, sizeof (text), "%s partner %s ",
			(teamplay->value && !IS_ALIVE(ent)) ? "[DEAD] " : "",
			ent->client->pers.netname);
		//TempFile - END
	}
	else
	{
		if (!meing)		//TempFile
		{
			if (isadmin)
				Com_sprintf (text, sizeof (text), "[ADMIN] %s: ",
				ent->client->pers.netname);
			else
				Com_sprintf (text, sizeof (text), "%s%s: ",
				(teamplay->value && !IS_ALIVE(ent)) ? "[DEAD] " : "",
				ent->client->pers.netname);
		}
		else
			Com_sprintf (text, sizeof (text), "%s%s ",
			(teamplay->value && !IS_ALIVE(ent)) ? "[DEAD] " : "",
			ent->client->pers.netname);
	}
	//TempFile - END
	
	offset_of_text = strlen (text);	//FB 5/31/99
	if (!meing)		//TempFile
	{
		if (arg0)
		{
			Q_strncatz(text, gi.argv (0), sizeof(text));
			if(*args) {
				Q_strncatz(text, " ", sizeof(text));
				Q_strncatz(text, args, sizeof(text));
			}
		}
		else
		{	
			if (*args == '"') {
				args++;
				args[strlen(args) - 1] = 0;
			}
			Q_strncatz(text, args, sizeof(text));
		}
	}
	else			// if meing
	{
		if (arg0)
		{
			//this one is easy: gi.args() cuts /me off for us!
			Q_strncatz(text, args, sizeof(text));
		}
		else
		{
			// we have to cut off "%me ".
			args += meing;
			if (args[strlen(args) - 1] == '"')
				args[strlen(args) - 1] = 0;
			Q_strncatz(text, args, sizeof(text));
		}
		
		if (team)
			Q_strncatz(text, ")", sizeof(text));
	}
	//TempFile - END
	// don't let text be too long for malicious reasons
	if (strlen(text) >= 254)
		text[254] = 0;

	show_info = ! strncmp( text + offset_of_text, "!actionversion", 14 );

	//if( IS_ALIVE(ent) )  // Disabled so we parse dead chat too.
	{
		s = strchr(text + offset_of_text, '%');
		if(s) {
			// this will parse the % variables,
			ParseSayText (ent, s, sizeof(text) - (s - text+1) - 2);
		}
	}

	Q_strncatz(text, "\n", sizeof(text));

	if (FloodCheck(ent))
		return;
	
	if (dedicated->value) {
		gi.cprintf (NULL, PRINT_CHAT, "%s", text);
		if ((!team) && (!partner_msg)) {
		}
	}
	
	for (j = 1; j <= game.maxclients; j++)
	{
		other = &g_edicts[j];
		if (!other->inuse || !other->client)
			continue;

		if (other != ent)
		{
			if (team)
			{
				// if we are the adminent... we might want to hear (if hearall is set)
				if (!matchmode->value || !hearall->value || !other->client->pers.admin)	// hearall isn't set and we aren't adminent
					if (!OnSameTeam(ent, other))
						continue;
			}

			if (partner_msg && other != ent->client->resp.radio.partner)
				continue;

			if (team_round_going && (gameSettings & GS_ROUNDBASED))
			{
				if (!deadtalk->value && !IS_ALIVE(ent) && IS_ALIVE(other))
					continue;
			}

			if (IsInIgnoreList(other, ent))
				continue;
		}

		gi.cprintf(other, PRINT_CHAT, "%s", text);

		if( show_info )
			gi.cprintf( other, PRINT_CHAT, "console: AQ2:TNG %s (%i fps)\n", VERSION, game.framerate );
	}

}

static void Cmd_PlayerList_f (edict_t * ent)
{
	int i;
	char st[64];
	char text[1024] = { 0 };
	edict_t *e2;

	// connect time, ping, score, name

	// Set the lines:
	for (i = 0, e2 = g_edicts + 1; i < game.maxclients; i++, e2++)
	{
		int seconds = ((level.framenum - e2->client->resp.enterframe) / HZ) % 60;
		int minutes = ((level.framenum - e2->client->resp.enterframe) / HZ) / 60;

		if (!e2->inuse || !e2->client || e2->client->pers.mvdspec)
			continue;

		if(limchasecam->value)
			Com_sprintf (st, sizeof (st), "%02d:%02d %4d %3d %s\n", minutes, seconds, e2->client->ping, e2->client->resp.team, e2->client->pers.netname); // This shouldn't show player's being 'spectators' during games with limchasecam set and/or during matchmode
		else if (!teamplay->value || !noscore->value)
			Com_sprintf (st, sizeof (st), "%02d:%02d %4d %3d %s%s\n", minutes, seconds, e2->client->ping, e2->client->resp.score, e2->client->pers.netname, (e2->solid == SOLID_NOT && e2->deadflag != DEAD_DEAD) ? " (dead)" : ""); // replaced 'spectator' with 'dead'
		else
			Com_sprintf (st, sizeof (st), "%02d:%02d %4d %s%s\n", minutes, seconds, e2->client->ping, e2->client->pers.netname, (e2->solid == SOLID_NOT && e2->deadflag != DEAD_DEAD) ? " (dead)" : ""); // replaced 'spectator' with 'dead'

		if (strlen(text) + strlen(st) > sizeof(text) - 6)
		{
			strcat(text, "...\n");
			break;
		}
		strcat (text, st);
	}
	gi.cprintf(ent, PRINT_HIGH, "%s", text);
}

//SLICER
static void Cmd_Ent_Count_f (edict_t * ent)
{
	int x = 0;
	edict_t *e;

	for (e = g_edicts; e < &g_edicts[globals.num_edicts]; e++)
	{
		if (e->inuse)
			x++;
	}

	gi.cprintf(ent, PRINT_HIGH, "%d entities counted\n", x);
}

//SLICER END
static void dmflagsSettings( char *s, size_t size, int flags )
{
	if (!flags) {
		Q_strncatz( s, "NONE", size );
		return;
	}
	if (flags & DF_NO_HEALTH)
		Q_strncatz( s, "1 = no health ", size );
	if (flags & DF_NO_ITEMS)
		Q_strncatz( s, "2 = no items ", size );
	if (flags & DF_WEAPONS_STAY)
		Q_strncatz( s, "4 = weapons stay ", size );
	if (flags & DF_NO_FALLING)
		Q_strncatz( s, "8 = no fall damage ", size );
	if (flags & DF_INSTANT_ITEMS)
		Q_strncatz( s, "16 = instant items ", size );
	if (flags & DF_SAME_LEVEL)
		Q_strncatz( s, "32 = same level ", size );
	if (flags & DF_SKINTEAMS)
		Q_strncatz( s, "64 = skinteams ", size );
	if (flags & DF_MODELTEAMS)
		Q_strncatz( s, "128 = modelteams ", size );
	if (flags & DF_NO_FRIENDLY_FIRE)
		Q_strncatz( s, "256 = no ff ", size );
	if (flags & DF_SPAWN_FARTHEST)
		Q_strncatz( s, "512 = spawn farthest ", size );
	if (flags & DF_FORCE_RESPAWN)
		Q_strncatz( s, "1024 = force respawn ", size );
	//if(flags & DF_NO_ARMOR)
	//	Q_strncatz(s, "2048 = no armor ", size);
	if (flags & DF_ALLOW_EXIT)
		Q_strncatz( s, "4096 = allow exit ", size );
	if (flags & DF_INFINITE_AMMO)
		Q_strncatz( s, "8192 = infinite ammo ", size );
	if (flags & DF_QUAD_DROP)
		Q_strncatz( s, "16384 = quad drop ", size );
	if (flags & DF_FIXED_FOV)
		Q_strncatz( s, "32768 = fixed fov ", size );

	if (flags & DF_WEAPON_RESPAWN)
		Q_strncatz( s, "65536 = weapon respawn ", size );
}

extern char *menu_itemnames[ITEM_MAX_NUM];

static void wpflagsSettings( char *s, size_t size, int flags )
{
	int i, num;

	if (!(flags & WPF_MASK)) {
		Q_strncatz( s, "No weapons", size );
		return;
	}
	if ((flags & WPF_MASK) == WPF_MASK) {
		Q_strncatz( s, "All weapons", size );
		return;
	}

	for (i = 0; i<WEAPON_COUNT; i++) {
		num = WEAPON_FIRST + i;
		if (flags == items[WEAPON_FIRST + i].flag) {
			Q_strncatz( s, va("%s only", menu_itemnames[num]), size );
			return;
		}
	}

	for (i = 0; i<WEAPON_COUNT; i++) {
		num = WEAPON_FIRST + i;
		if (flags & items[num].flag) {
			Q_strncatz( s, va("%d = %s ", items[num].flag, menu_itemnames[num]), size );
		}
	}
}

static void itmflagsSettings( char *s, size_t size, int flags )
{
	int i, num;

	if (!(flags & ITF_MASK)) {
		Q_strncatz( s, "No items", size );
		return;
	}
	if ((flags & ITF_MASK) == ITF_MASK) {
		Q_strncatz( s, "All items", size );
		return;
	}

	for (i = 0; i<ITEM_COUNT; i++) {
		num = ITEM_FIRST + i;
		if (flags == items[num].flag) {
			Q_strncatz( s, va("%s only", menu_itemnames[num]), size );
			return;
		}
	}

	for (i = 0; i<ITEM_COUNT; i++) {
		num = ITEM_FIRST + i;
		if (flags & items[num].flag) {
			Q_strncatz( s, va("%d = %s ", items[num].flag, menu_itemnames[num]), size );
		}
	}
}

//Print current match settings
static void Cmd_PrintSettings_f( edict_t * ent )
{
	char text[1024] = "\0";
	size_t length = 0;

	if (game.serverfeatures & GMF_VARIABLE_FPS) {
		Com_sprintf( text, sizeof( text ), "Server fps = %d\n", game.framerate );
		length = strlen( text );
	}

	Com_sprintf( text + length, sizeof( text ) - length, "sv_antilag = %d\n", (int)sv_antilag->value );
	length = strlen( text );
	
	Com_sprintf( text + length, sizeof( text ) - length, "dmflags %i: ", (int)dmflags->value );
	dmflagsSettings( text, sizeof( text ), (int)dmflags->value );

	length = strlen( text );
	Com_sprintf( text + length, sizeof( text ) - length, "\nwp_flags %i: ", (int)wp_flags->value );
	wpflagsSettings( text, sizeof( text ), (int)wp_flags->value );

	length = strlen( text );
	Com_sprintf( text + length, sizeof( text ) - length, "\nitm_flags %i: ", (int)itm_flags->value );
	itmflagsSettings( text, sizeof( text ), (int)itm_flags->value );

	length = strlen( text );
	Com_sprintf( text + length, sizeof( text ) - length, "\n"
		"timelimit   %2d roundlimit  %2d roundtimelimit %2d\n"
		"limchasecam %2d tgren       %2d antilag_interp %2d\n"
		"llsound     %2d\n",
		(int)timelimit->value, (int)roundlimit->value, (int)roundtimelimit->value,
		(int)limchasecam->value, (int)tgren->value, (int)sv_antilag_interp->value,
		(int)llsound->value );

	gi.cprintf( ent, PRINT_HIGH, text );
}

static void Cmd_Follow_f( edict_t *ent )
{
	edict_t *target = NULL;

	if( (ent->solid != SOLID_NOT) || (ent->deadflag == DEAD_DEAD) )
	{
		gi.cprintf( ent, PRINT_HIGH, "Only spectators may follow!\n" );
		return;
	}

	target = LookupPlayer( ent, gi.argv(1), true, true );
	if( target == ent )
	{
		if( ! limchasecam->value )
			SetChase( ent, NULL );
	}
	else if( target )
	{
		if( limchasecam->value && teamplay->value
		&& (ent->client->resp.team != NOTEAM)
		&& (ent->client->resp.team != target->client->resp.team) )
		{
			gi.cprintf( ent, PRINT_HIGH, "You may not follow enemies!\n" );
			return;
		}

		if( ! ent->client->chase_mode )
			NextChaseMode( ent );
		SetChase( ent, target );
	}
}

static void Cmd_SayAll_f (edict_t * ent) {
	Cmd_Say_f (ent, false, false, false);
}
static void Cmd_SayTeam_f (edict_t * ent) {
	if (teamplay->value) // disable mm2 trigger flooding
		Cmd_Say_f (ent, true, false, false);
}

static void Cmd_Streak_f (edict_t * ent) {
	gi.cprintf(ent,PRINT_HIGH, "Your Killing Streak is: %d\n", ent->client->resp.streakKills);
}

static void Cmd_LockTeam_f (edict_t * ent) {
	Cmd_TeamLock_f(ent, 1);
}

static void Cmd_UnlockTeam_f (edict_t * ent) {
	Cmd_TeamLock_f(ent, 0);
}

static void Cmd_PrintStats_f (edict_t *ent) {
	Cmd_Stats_f(ent, gi.argv(1));
}

static void Cmd_PauseGame_f (edict_t *ent) {
	Cmd_TogglePause_f(ent, true);
}

static void Cmd_UnpauseGame_f (edict_t *ent) {
	Cmd_TogglePause_f(ent, false);
}

static void Cmd_InvNext_f (edict_t *ent) {
	SelectNextItem(ent, -1);
}

static void Cmd_InvPrev_f (edict_t *ent) {
	SelectPrevItem(ent, -1);
}

static void Cmd_InvNextw_f (edict_t *ent) {
	SelectNextItem(ent, IT_WEAPON);
}

static void Cmd_InvPrevw_f (edict_t *ent) {
	SelectPrevItem(ent, IT_WEAPON);
}

static void Cmd_InvNextp_f (edict_t *ent) {
	SelectNextItem(ent, IT_POWERUP);
}

static void Cmd_InvPrevp_f (edict_t *ent) {
	SelectPrevItem(ent, IT_POWERUP);
}

 // AQ2:TNG - Slicer : Video Check
static void Cmd_VidRef_f (edict_t * ent)
{
	if (video_check->value || video_check_lockpvs->value)
	{
		Q_strncpyz(ent->client->resp.vidref, gi.argv(1), sizeof(ent->client->resp.vidref));
	}

}

static void Cmd_CPSI_f (edict_t * ent)
{
	if (video_check->value || video_check_lockpvs->value || video_check_glclear->value || darkmatch->value)
	{
		ent->client->resp.glmodulate = atoi(gi.argv(1));
		ent->client->resp.gllockpvs = atoi(gi.argv(2));
		ent->client->resp.glclear = atoi(gi.argv(3));
		ent->client->resp.gldynamic = atoi(gi.argv(4));
		ent->client->resp.glbrightness = atoi(gi.argv(5));
		Q_strncpyz(ent->client->resp.gldriver, gi.argv (6), sizeof(ent->client->resp.gldriver));
		//      strncpy(ent->client->resp.vidref,gi.argv(4),sizeof(ent->client->resp.vidref-1));
		//      ent->client->resp.vidref[15] = 0;
	}
}

#define CMDF_CHEAT	1 //Need cheat to be enabled
#define CMDF_PAUSE	2 //Cant use while pause

typedef struct cmdList_s
{
	const char *name;
	void( *function ) (edict_t *ent);
	int flags;
	struct cmdList_s *next;
	struct cmdList_s *hashNext;
} cmdList_t;

static cmdList_t commandList[] =
{
	{ "players", Cmd_Players_f, 0 },
	{ "say", Cmd_SayAll_f, 0 },
	{ "say_team", Cmd_SayTeam_f, 0 },
	{ "score", Cmd_Score_f, 0 },
	{ "help", Cmd_Help_f, 0 },
	{ "use", Cmd_Use_f, CMDF_PAUSE },
	{ "drop", Cmd_Drop_f, CMDF_PAUSE },
	//cheats
	{ "give", Cmd_Give_f, CMDF_CHEAT },
	{ "god", Cmd_God_f, CMDF_CHEAT },
	{ "notarget", Cmd_Notarget_f, CMDF_CHEAT },
	{ "noclip", Cmd_Noclip_f, CMDF_CHEAT },
	//-
	{ "inven", Cmd_Inven_f, 0 },
	{ "invnext", Cmd_InvNext_f, 0 },
	{ "invprev", Cmd_InvPrev_f, 0 },
	{ "invnextw", Cmd_InvNextw_f, 0 },
	{ "invprevw", Cmd_InvPrevw_f, 0 },
	{ "invnextp", Cmd_InvNextp_f, 0 },
	{ "invprevp", Cmd_InvPrevp_f, 0 },
	{ "invuse", Cmd_InvUse_f, CMDF_PAUSE },
	{ "invdrop", Cmd_InvDrop_f, CMDF_PAUSE },
	{ "weapprev", Cmd_WeapPrev_f, CMDF_PAUSE },
	{ "weapnext", Cmd_WeapNext_f, CMDF_PAUSE },
	{ "weaplast", Cmd_WeapLast_f, CMDF_PAUSE },
	{ "kill", Cmd_Kill_f, 0 },
	{ "putaway", Cmd_PutAway_f, 0 },
	{ "wave", Cmd_Wave_f, CMDF_PAUSE },
	{ "streak", Cmd_Streak_f, 0 },
	{ "reload", Cmd_New_Reload_f, CMDF_PAUSE },
	{ "weapon", Cmd_New_Weapon_f, CMDF_PAUSE },
	{ "opendoor", Cmd_OpenDoor_f, CMDF_PAUSE },
	{ "bandage", Cmd_Bandage_f, CMDF_PAUSE },
	{ "id", Cmd_ID_f, 0 },
	{ "irvision", Cmd_IR_f, CMDF_PAUSE },
	{ "playerlist", Cmd_PlayerList_f, 0 },
	{ "team", Team_f, 0 },
	{ "radio", Cmd_Radio_f, 0 },
	{ "radiogender", Cmd_Radiogender_f, 0 },
	{ "radio_power", Cmd_Radio_power_f, 0 },
	{ "radiopartner", Cmd_Radiopartner_f, 0 },
	{ "radioteam", Cmd_Radioteam_f, 0 },
	{ "channel", Cmd_Channel_f, 0 },
	{ "say_partner", Cmd_Say_partner_f, 0 },
	{ "partner", Cmd_Partner_f, 0 },
	{ "unpartner", Cmd_Unpartner_f, 0 },
	{ "motd", PrintMOTD, 0 },
	{ "deny", Cmd_Deny_f, 0 },
	{ "choose", Cmd_Choose_f, 0 },
	{ "tkok", Cmd_TKOk, 0 },
	{ "forgive", Cmd_TKOk, 0 },
	{ "ff", Cmd_FF_f, 0 },
	{ "time", Cmd_Time, 0 },
	{ "voice", Cmd_Voice_f, CMDF_PAUSE },
	{ "whereami", Cmd_WhereAmI_f, 0 },
	{ "setflag1", Cmd_SetFlag1_f, CMDF_PAUSE|CMDF_CHEAT },
	{ "setflag2", Cmd_SetFlag2_f, CMDF_PAUSE|CMDF_CHEAT },
	{ "saveflags", Cmd_SaveFlags_f, CMDF_PAUSE|CMDF_CHEAT },
	{ "punch", Cmd_Punch_f, CMDF_PAUSE },
	{ "menu", Cmd_Menu_f, 0 },
	{ "rules", Cmd_Rules_f, 0 },
	{ "lens", Cmd_Lens_f, CMDF_PAUSE },
	{ "nextmap", Cmd_NextMap_f, 0 },
	{ "%cpsi", Cmd_CPSI_f, 0 },
	{ "%!fc", Cmd_VidRef_f, 0 },
	{ "sub", Cmd_Sub_f, 0 },
	{ "captain", Cmd_Captain_f, 0 },
	{ "ready", Cmd_Ready_f, 0 },
	{ "teamname", Cmd_Teamname_f, 0 },
	{ "teamskin", Cmd_Teamskin_f, 0 },
	{ "lock", Cmd_LockTeam_f, 0 },
	{ "unlock", Cmd_UnlockTeam_f, 0 },
	{ "entcount", Cmd_Ent_Count_f, 0 },
	{ "stats", Cmd_PrintStats_f, 0 },
	{ "flashlight", FL_make, CMDF_PAUSE },
	{ "matchadmin", Cmd_SetAdmin_f, 0 },
	{ "roundtimeleft", Cmd_Roundtimeleft_f, 0 },
	{ "autorecord", Cmd_AutoRecord_f, 0 },
	{ "stat_mode", Cmd_Statmode_f, 0 },
	{ "cmd_stat_mode", Cmd_Statmode_f, 0 },
	{ "ghost", Cmd_Ghost_f, 0 },
	{ "pausegame", Cmd_PauseGame_f, 0 },
	{ "unpausegame", Cmd_UnpauseGame_f, 0 },
	{ "resetscores", Cmd_ResetScores_f, 0 },
	{ "gamesettings", Cmd_PrintSettings_f, 0 },
	{ "follow", Cmd_Follow_f, 0 },
	//vote stuff
	{ "votemap", Cmd_Votemap_f, 0 },
	{ "maplist", Cmd_Maplist_f, 0 },
	{ "votekick", Cmd_Votekick_f, 0 },
	{ "votekicknum", Cmd_Votekicknum_f, 0 },
	{ "kicklist", Cmd_Kicklist_f, 0 },
	{ "ignore", Cmd_Ignore_f, 0 },
	{ "ignorenum", Cmd_Ignorenum_f, 0 },
	{ "ignorelist", Cmd_Ignorelist_f, 0 },
	{ "ignoreclear", Cmd_Ignoreclear_f, 0 },
	{ "ignorepart", Cmd_IgnorePart_f, 0 },
	{ "voteconfig", Cmd_Voteconfig_f, 0 },
	{ "configlist", Cmd_Configlist_f, 0 },
	{ "votescramble", Cmd_Votescramble_f, 0 },
	// JumpMod
	{ "jmod", Cmd_Jmod_f, 0 }
};

#define MAX_COMMAND_HASH 64

static cmdList_t *commandHash[MAX_COMMAND_HASH];
static const int numCommands = sizeof( commandList ) / sizeof( commandList[0] );

size_t Cmd_HashValue( const char *name )
{
	size_t hash = 0;

	while (*name) {
		hash = hash * 33 + Q_tolower( *name++ );
	}

	return hash + (hash >> 5);
}

void InitCommandList( void )
{

	int i;
	size_t hash;

	for (i = 0; i < numCommands - 1; i++) {
		commandList[i].next = &commandList[i + 1];
	}
	commandList[i].next = NULL;

	memset( commandHash, 0, sizeof( commandHash ) );
	for (i = 0; i < numCommands; i++) {
		hash = Cmd_HashValue( commandList[i].name ) & (MAX_COMMAND_HASH - 1);
		commandList[i].hashNext = commandHash[hash];
		commandHash[hash] = &commandList[i];
	}
}

/*
=================
ClientCommand
=================
*/
void ClientCommand (edict_t * ent)
{
	char		*text;
	cmdList_t	*cmd;
	size_t		hash;

	if (!ent->client)
		return;			// not fully in game yet

	// if (level.intermission_framenum)
	// return;

	text = gi.argv(0);

	hash = Cmd_HashValue( text ) & (MAX_COMMAND_HASH - 1);
	for (cmd = commandHash[hash]; cmd; cmd = cmd->hashNext) {
		if (!Q_stricmp( text, cmd->name )) {
			if ((cmd->flags & CMDF_CHEAT) && !sv_cheats->value) {
				gi.cprintf(ent, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
				return;
			}

			if ((cmd->flags & CMDF_PAUSE) && level.pauseFrames)
				return;

			cmd->function( ent );
			return;
		}
	}

	// anything that doesn't match a command will be a chat
	Cmd_Say_f(ent, false, true, false);
}

