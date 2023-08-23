//-----------------------------------------------------------------------------
// p_hud.c
//
// $Id: p_hud.c,v 1.8 2002/01/24 02:24:56 deathwatch Exp $
//
//-----------------------------------------------------------------------------
// $Log: p_hud.c,v $
// Revision 1.8  2002/01/24 02:24:56  deathwatch
// Major update to Stats code (thanks to Freud)
// new cvars:
// stats_afterround - will display the stats after a round ends or map ends
// stats_endmap - if on (1) will display the stats scoreboard when the map ends
//
// Revision 1.7  2002/01/02 01:18:24  deathwatch
// Showing health icon when bandaging (thanks to Dome for submitting this code)
//
// Revision 1.6  2001/09/28 13:48:35  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.5  2001/08/20 00:41:15  slicerdw
// Added a new scoreboard for Teamplay with stats ( when map ends )
//
// Revision 1.4  2001/07/16 19:02:06  ra
// Fixed compilerwarnings (-g -Wall).  Only one remains.
//
// Revision 1.3  2001/05/31 16:58:14  igor_rock
// conflicts resolved
//
// Revision 1.2.2.3  2001/05/25 18:59:52  igor_rock
// Added CTF Mode completly :)
// Support for .flg files is still missing, but with "real" CTF maps like
// tq2gtd1 the ctf works fine.
// (I hope that all other modes still work, just tested DM and teamplay)
//
// Revision 1.2.2.2  2001/05/20 18:54:19  igor_rock
// added original ctf code snippets from zoid. lib compilesand runs but
// doesn't function the right way.
// Jsut committing these to have a base to return to if something wents
// awfully wrong.
//
// Revision 1.2.2.1  2001/05/20 15:17:31  igor_rock
// removed the old ctf code completly
//
// Revision 1.2  2001/05/11 16:07:26  mort
// Various CTF bits and pieces...
//
// Revision 1.1.1.1  2001/05/06 17:25:15  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#include "g_local.h"

/*
  ======================================================================
  
  INTERMISSION
  
  ======================================================================
*/

void MoveClientToIntermission(edict_t *ent)
{
	PMenu_Close(ent);

	ent->client->layout = LAYOUT_SCORES;
	VectorCopy(level.intermission_origin, ent->s.origin);
	ent->client->ps.pmove.origin[0] = level.intermission_origin[0] * 8;
	ent->client->ps.pmove.origin[1] = level.intermission_origin[1] * 8;
	ent->client->ps.pmove.origin[2] = level.intermission_origin[2] * 8;
	VectorCopy(level.intermission_angle, ent->client->ps.viewangles);
	VectorClear(ent->client->ps.kick_angles);
	ent->client->ps.pmove.pm_type = PM_FREEZE;
	ent->client->ps.gunindex = 0;
	ent->client->ps.blend[3] = 0;
	ent->client->ps.rdflags &= ~RDF_UNDERWATER;
	ent->client->ps.stats[STAT_FLASHES] = 0;

	// clean up powerup info
	ent->client->quad_framenum = 0;
	ent->client->invincible_framenum = 0;
	ent->client->breather_framenum = 0;
	ent->client->enviro_framenum = 0;
	ent->client->grenade_blew_up = false;
	ent->client->grenade_framenum = 0;

	ent->watertype = 0;
	ent->waterlevel = 0;
	ent->viewheight = 0;
	ent->s.modelindex = 0;
	ent->s.modelindex2 = 0;
	ent->s.modelindex3 = 0;
	ent->s.modelindex4 = 0;
	ent->s.effects = 0;
	ent->s.renderfx = 0;
	ent->s.sound = 0;
	ent->s.event = 0;
	ent->s.solid = 0;
	ent->solid = SOLID_NOT;
	ent->svflags = SVF_NOCLIENT;

	ent->client->resp.sniper_mode = SNIPER_1X;
	ent->client->desired_fov = 90;
	ent->client->ps.fov = 90;
	ent->client->ps.stats[STAT_SNIPER_ICON] = 0;
	ent->client->pickup_msg_framenum = 0;

	// add the layout
	DeathmatchScoreboardMessage(ent, NULL);
	gi.unicast(ent, true);
}

void BeginIntermission(edict_t *targ)
{
	int i;
	edict_t *ent;

	if (level.intermission_framenum)
		return;			// already activated

	level.intermission_framenum = level.realFramenum;

	if (ctf->value) {
		CTFCalcScores();
	} else if (teamplay->value) {
		TallyEndOfLevelTeamScores();
	}

	// respawn any dead clients
	for (i = 0, ent = g_edicts + 1; i < game.maxclients; i++, ent++)
	{
		if (!ent->inuse)
			continue;
		if (ent->health <= 0)
			respawn(ent);
	}

	level.changemap = targ->map;
	level.intermission_exit = 0;

	// find an intermission spot
	ent = G_Find(NULL, FOFS(classname), "info_player_intermission");
	if (!ent)
	{				// the map creator forgot to put in an intermission point...
		ent = G_Find(NULL, FOFS(classname), "info_player_start");
		if (!ent)
			ent = G_Find(NULL, FOFS(classname), "info_player_deathmatch");
	}
	else
	{				// chose one of four spots
		i = rand () & 3;
		while (i--)
		{
			ent = G_Find(ent, FOFS(classname), "info_player_intermission");
			if (!ent)		// wrap around the list
				ent = G_Find(ent, FOFS(classname), "info_player_intermission");
		}
	}

	if (ent) {
		VectorCopy( ent->s.origin, level.intermission_origin );
		VectorCopy( ent->s.angles, level.intermission_angle );
	}

	// move all clients to the intermission point
	for (i = 0, ent = g_edicts + 1; i < game.maxclients; i++, ent++)
	{
		if (!ent->inuse)
			continue;

		MoveClientToIntermission(ent);
	}

	InitTransparentList();
}

/*
  ==================
  DeathmatchScoreboardMessage
  
  ==================
*/
void DeathmatchScoreboardMessage (edict_t * ent, edict_t * killer)
{
	char entry[128];
	char string[1024];
	int stringlength;
	int i, j, totalClients;
	gclient_t *sortedClients[MAX_CLIENTS];
	int x, y;
	gclient_t *cl;
	edict_t *cl_ent;
	char *tag;

	if (teamplay->value && !use_tourney->value)
	{
		// DW: If the map ends
		if (level.intermission_framenum) {
			if (stats_endmap->value && (gameSettings & GS_ROUNDBASED)) // And we are to show the stats screen
				A_ScoreboardEndLevel(ent, killer); // do it
			else																	// otherwise
				A_ScoreboardMessage(ent, killer);	// show the original
		}
		else
			A_ScoreboardMessage(ent, killer);

		return;
	}

	totalClients = G_SortedClients(sortedClients);

	// print level name and exit rules
	string[0] = 0;
	stringlength = 0;

	// add the clients in sorted order
	if (totalClients > 12)
		totalClients = 12;

	for (i = 0; i < totalClients; i++)
	{
		cl = sortedClients[i];
		cl_ent = g_edicts + 1 + (cl - game.clients);

		x = (i >= 6) ? 160 : 0;
		y = 32 + 32 * (i % 6);

		// add a dogtag
		if (cl_ent == ent)
			tag = "tag1";
		else if (cl_ent == killer)
			tag = "tag2";
		else
			tag = NULL;
		if (tag)
		{
			Com_sprintf (entry, sizeof (entry),
				"xv %i yv %i picn %s ", x + 32, y, tag);
			j = strlen (entry);
			if (stringlength + j > 1023)
				break;
			strcpy (string + stringlength, entry);
			stringlength += j;
		}

		// send the layout
		Com_sprintf (entry, sizeof (entry),
			"client %i %i %i %i %i %i ",
			x, y, (int)(cl - game.clients), cl->resp.score, cl->ping,
			(level.framenum - cl->resp.enterframe) / 600 / FRAMEDIV);
		j = strlen (entry);
		if (stringlength + j > 1023)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}

	gi.WriteByte (svc_layout);
	gi.WriteString (string);
}


/*
  ==================
  DeathmatchScoreboard
  
  Draw instead of help message.
  Note that it isn't that hard to overflow the 1400 byte message limit!
  ==================
*/
void DeathmatchScoreboard(edict_t *ent)
{
	DeathmatchScoreboardMessage(ent, ent->enemy);
	gi.unicast(ent, true);
}


/*
  ==================
  Cmd_Score_f
  
  Display the scoreboard
  ==================
*/
void Cmd_Score_f(edict_t *ent)
{
	ent->client->showinventory = false;
	
	if (ent->client->layout == LAYOUT_MENU)
		PMenu_Close(ent);
	
	if (ent->client->layout == LAYOUT_SCORES)
	{
		if (teamplay->value) {	// toggle scoreboards...
			ent->client->layout = LAYOUT_SCORES2;
			DeathmatchScoreboard(ent);
			return;
		}	
		ent->client->layout = LAYOUT_NONE;
		return;
	}

	if (ent->client->layout == LAYOUT_SCORES2) {
		ent->client->layout = LAYOUT_NONE;
		return;
	}
	
	ent->client->layout = LAYOUT_SCORES;
	DeathmatchScoreboard(ent);
}


/*
  ==================
  Cmd_Help_f
  
  Display the current help message
  ==================
*/
void Cmd_Help_f (edict_t * ent)
{
	// this is for backwards compatability
	Cmd_Score_f (ent);
}


//=======================================================================

/*
  ===============
  G_SetStats
  
  Rearranged for chase cam support -FB
  ===============
*/
void G_SetStats (edict_t * ent)
{
	gitem_t *item;

	if (jump->value)
	{
		Jmp_SetStats(ent);
		return;
	}

	if (!ent->client->chase_mode)
	{
		int icons[ 6 ], numbers[ 2 ], icon_count, i;
		int cycle = hud_items_cycle->value * FRAMEDIV;
		int weapon_ids[ 6 ] = { SNIPER_NUM, M4_NUM, MP5_NUM, M3_NUM, HC_NUM, DUAL_NUM };
		int s_item_ids[ 6 ] = { KEV_NUM, HELM_NUM, BAND_NUM, SIL_NUM, SLIP_NUM, LASER_NUM };

		//
		// health
		//
		ent->client->ps.stats[STAT_HEALTH_ICON] = level.pic_health;
		ent->client->ps.stats[STAT_HEALTH] = ent->health;

		//
		// ammo (now clips really)
		//
		// zucc modified this to do clips instead
		if (!ent->client-> ammo_index
			/* || !ent->client->inventory[ent->client->ammo_index] */ )
		{
			ent->client->ps.stats[STAT_CLIP_ICON] = 0;
			ent->client->ps.stats[STAT_CLIP] = 0;
		}
		else
		{
			item = &itemlist[ent->client->ammo_index];
			if (item->typeNum < AMMO_MAX)
				ent->client->ps.stats[STAT_CLIP_ICON] = level.pic_items[item->typeNum];
			else
				ent->client->ps.stats[STAT_CLIP_ICON] = gi.imageindex(item->icon);
			ent->client->ps.stats[STAT_CLIP] = ent->client->inventory[ent->client->ammo_index];
		}

		// zucc display special item and special weapon
		// Raptor007: Modified to rotate through all carried special weapons and items.

		icon_count = 0;
		for( i = 0; i < 6; i ++ )
		{
			if( INV_AMMO( ent, weapon_ids[i] ) )
				icons[ icon_count ++ ] = level.pic_items[ weapon_ids[i] ];
		}
		if( icon_count && ! cycle )
			icon_count = 1;
		if( icon_count )
			ent->client->ps.stats[STAT_WEAPONS_ICON] = icons[ (level.framenum/cycle) % icon_count ];
		else
			ent->client->ps.stats[STAT_WEAPONS_ICON] = 0;

		icon_count = 0;
		for( i = 0; i < 6; i ++ )
		{
			if( INV_AMMO( ent, s_item_ids[i] ) )
				icons[ icon_count ++ ] = level.pic_items[ s_item_ids[i]];
		}
		if( icon_count && ! cycle )
			icon_count = 1;
		if( icon_count )
			ent->client->ps.stats[STAT_ITEMS_ICON] = icons[ ((level.framenum+cycle/2)/cycle) % icon_count ];
		else
			ent->client->ps.stats[STAT_ITEMS_ICON] = 0;

		// grenades remaining
		icon_count = 0;
		numbers[ icon_count ] = INV_AMMO( ent, GRENADE_NUM );
		if( numbers[ icon_count ] )
			icons[ icon_count ++ ] = level.pic_weapon_ammo[GRENADE_NUM];
		// MedKit
		numbers[ icon_count ] = ent->client->medkit;
		if( numbers[ icon_count ] )
			icons[ icon_count ++ ] = level.pic_health;
		// Cycle grenades and medkit if player has both.
		if( icon_count && ! cycle )
			icon_count = 1;
		if( icon_count )
		{
			int index = ((level.framenum+cycle/4)/cycle) % icon_count;
			ent->client->ps.stats[STAT_GRENADE_ICON] = icons[ index ];
			ent->client->ps.stats[STAT_GRENADES]     = numbers[ index ];
		}
		else
		{
			ent->client->ps.stats[STAT_GRENADE_ICON] = 0;
			ent->client->ps.stats[STAT_GRENADES]     = 0;
		}

		//
		// ammo by weapon
		// 
		//
		if (ent->client->weapon && ent->client->curr_weap)
		{
			switch (ent->client->curr_weap) {
			case MK23_NUM:
				ent->client->ps.stats[STAT_AMMO_ICON] = level.pic_weapon_ammo[ent->client->curr_weap];
				ent->client->ps.stats[STAT_AMMO] = ent->client->mk23_rds;
				break;
			case MP5_NUM:
				ent->client->ps.stats[STAT_AMMO_ICON] = level.pic_weapon_ammo[ent->client->curr_weap];
				ent->client->ps.stats[STAT_AMMO] = ent->client->mp5_rds;
				break;
			case M4_NUM:
				ent->client->ps.stats[STAT_AMMO_ICON] = level.pic_weapon_ammo[ent->client->curr_weap];
				ent->client->ps.stats[STAT_AMMO] = ent->client->m4_rds;
				break;
			case M3_NUM:
				ent->client->ps.stats[STAT_AMMO_ICON] = level.pic_weapon_ammo[ent->client->curr_weap];
				ent->client->ps.stats[STAT_AMMO] = ent->client->shot_rds;
				break;
			case HC_NUM:
				ent->client->ps.stats[STAT_AMMO_ICON] = level.pic_weapon_ammo[ent->client->curr_weap];
				ent->client->ps.stats[STAT_AMMO] = ent->client->cannon_rds;
				break;
			case SNIPER_NUM:
				ent->client->ps.stats[STAT_AMMO_ICON] = level.pic_weapon_ammo[ent->client->curr_weap];
				ent->client->ps.stats[STAT_AMMO] = ent->client->sniper_rds;
				break;
			case DUAL_NUM:
				ent->client->ps.stats[STAT_AMMO_ICON] = level.pic_weapon_ammo[ent->client->curr_weap];
				ent->client->ps.stats[STAT_AMMO] = ent->client->dual_rds;
				break;
			case KNIFE_NUM:
				ent->client->ps.stats[STAT_AMMO_ICON] = level.pic_weapon_ammo[ent->client->curr_weap];
				ent->client->ps.stats[STAT_AMMO] = INV_AMMO(ent, KNIFE_NUM);
				break;
			case GRENADE_NUM:
				ent->client->ps.stats[STAT_AMMO_ICON] = level.pic_weapon_ammo[ent->client->curr_weap];
				ent->client->ps.stats[STAT_AMMO] = INV_AMMO(ent, GRENADE_NUM);
				break;
			case GRAPPLE_NUM:
				ent->client->ps.stats[STAT_AMMO_ICON] = 0;
				ent->client->ps.stats[STAT_AMMO] = 0;
				break;
			default:
				gi.dprintf ("Failed to find hud weapon/icon for num %d.\n", ent->client->curr_weap);
				break;
			}
		} else {
			ent->client->ps.stats[STAT_AMMO_ICON] = 0;
			ent->client->ps.stats[STAT_AMMO] = 0;
		}

		//
		// sniper mode icons
		//
		//if ( ent->client->sniper_mode )
		//      gi.cprintf (ent, PRINT_HIGH, "Sniper Zoom set at %d.\n", ent->client->sniper_mode);


		if (ent->client->resp.sniper_mode == SNIPER_1X
			|| ent->client->weaponstate == WEAPON_RELOADING
			|| ent->client->weaponstate == WEAPON_BUSY
			|| ent->client->no_sniper_display
			|| ! IS_ALIVE(ent) )
			ent->client->ps.stats[STAT_SNIPER_ICON] = 0;
		else
			ent->client->ps.stats[STAT_SNIPER_ICON] = level.pic_sniper_mode[ent->client->resp.sniper_mode];

		//
		// armor
		//
		//ent->client->ps.stats[STAT_ARMOR_ICON] = 0; // Replaced with STAT_TEAM_ICON.
		ent->client->ps.stats[STAT_ARMOR] = 0;

		//
		// timers
		//
		if (ent->client->quad_framenum > level.framenum)
		{
			ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_quad");
			ent->client->ps.stats[STAT_TIMER] = (ent->client->quad_framenum - level.framenum) / HZ;
		}
		else if (ent->client->invincible_framenum > level.framenum)
		{
			ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_invulnerability");
			ent->client->ps.stats[STAT_TIMER] = (ent->client->invincible_framenum - level.framenum) / HZ;
		}
		else if (ent->client->enviro_framenum > level.framenum)
		{
			ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_envirosuit");
			ent->client->ps.stats[STAT_TIMER] = (ent->client->enviro_framenum - level.framenum) / HZ;
		}
		else if (ent->client->breather_framenum > level.framenum)
		{
			ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_rebreather");
			ent->client->ps.stats[STAT_TIMER] = (ent->client->breather_framenum - level.framenum) / HZ;
		}
		else
		{
			ent->client->ps.stats[STAT_TIMER_ICON] = 0;
			ent->client->ps.stats[STAT_TIMER] = 0;
		}

		//
		// selected item
		//
		if (ent->client->selected_item < 1) {
			ent->client->ps.stats[STAT_SELECTED_ICON] = 0;
		} else {
			item = &itemlist[ent->client->selected_item];
			if (item->typeNum < AMMO_MAX)
				ent->client->ps.stats[STAT_SELECTED_ICON] = level.pic_items[item->typeNum];
			else
				ent->client->ps.stats[STAT_SELECTED_ICON] = gi.imageindex(item->icon);
		}

		ent->client->ps.stats[STAT_SELECTED_ITEM] = ent->client->selected_item;

		//
		// bandaging icon / current weapon if not shown
		//
		// TNG: Show health icon when bandaging (thanks to Dome for this code)
		if (ent->client->weaponstate == WEAPON_BANDAGING || ent->client->bandaging || ent->client->bandage_stopped)
			ent->client->ps.stats[STAT_HELPICON] = level.pic_health;
		else if ((ent->client->pers.hand == CENTER_HANDED || ent->client->ps.fov > 91) && ent->client->weapon)
			ent->client->ps.stats[STAT_HELPICON] = level.pic_items[ent->client->weapon->typeNum];
		else
			ent->client->ps.stats[STAT_HELPICON] = 0;
		
		// Hide health, ammo, weapon, and bandaging state when free spectating.
		if( ! IS_ALIVE(ent) )
		{
			ent->client->ps.stats[STAT_HEALTH_ICON] = 0;
			ent->client->ps.stats[STAT_AMMO_ICON] = 0;
			ent->client->ps.stats[STAT_SELECTED_ICON] = 0;
			ent->client->ps.stats[STAT_HELPICON] = 0;
		}
		
		// Team icon.
		if( teamplay->value && hud_team_icon->value && (ent->client->resp.team != NOTEAM) && IS_ALIVE(ent) )
			ent->client->ps.stats[STAT_TEAM_ICON] = level.pic_teamskin[ent->client->resp.team];
		else
			ent->client->ps.stats[STAT_TEAM_ICON] = 0;
	}

	//
	// pickup message
	//
	if (level.realFramenum > ent->client->pickup_msg_framenum)
	{
		ent->client->ps.stats[STAT_PICKUP_ICON] = 0;
		ent->client->ps.stats[STAT_PICKUP_STRING] = 0;
	}

	//
	// frags
	//
	ent->client->ps.stats[STAT_FRAGS] = ent->client->resp.score;

	//
	// layouts
	//
	ent->client->ps.stats[STAT_LAYOUTS] = 0;

	if (level.intermission_framenum || ent->client->layout)
		ent->client->ps.stats[STAT_LAYOUTS] |= 1;
	if (ent->client->showinventory && ent->health > 0)
		ent->client->ps.stats[STAT_LAYOUTS] |= 2;

	if (level.intermission_framenum) {
		ent->client->ps.stats[STAT_SNIPER_ICON] = 0;
		ent->client->ps.stats[STAT_HELPICON] = 0;
		ent->client->ps.stats[STAT_ID_VIEW] = 0;
	} else {
		SetIDView(ent);
	}

	//FIREBLADE
	if (ctf->value)
		SetCTFStats (ent);
	else if (dom->value)
		SetDomStats (ent);
	else if (teamplay->value)
		A_Scoreboard (ent);
	//FIREBLADE
}
