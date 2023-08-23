//-----------------------------------------------------------------------------
// g_weapon.c
//
// $Id: p_weapon.c,v 1.20 2004/04/08 23:19:52 slicerdw Exp $
//
//-----------------------------------------------------------------------------
// $Log: p_weapon.c,v $
// Revision 1.20  2004/04/08 23:19:52  slicerdw
// Optimized some code, added a couple of features and fixed minor bugs
//
// Revision 1.19  2002/04/16 16:47:46  freud
// Fixed the use grenades, drop bandolier bug with use_buggy_bandolier 0
//
// Revision 1.18  2002/03/27 15:16:56  freud
// Original 1.52 spawn code implemented for use_newspawns 0.
// Teamplay, when dropping bandolier, your drop the grenades.
// Teamplay, cannot pick up grenades unless wearing bandolier.
//
// Revision 1.17  2002/02/01 17:49:56  freud
// Heavy changes in stats code. Removed lots of variables and replaced them
// with int arrays of MODs. This cleaned tng_stats.c up a whole lots and
// everything looks well, might need more testing.
//
// Revision 1.16  2002/01/24 02:24:56  deathwatch
// Major update to Stats code (thanks to Freud)
// new cvars:
// stats_afterround - will display the stats after a round ends or map ends
// stats_endmap - if on (1) will display the stats scoreboard when the map ends
//
// Revision 1.15  2002/01/22 14:13:37  deathwatch
// Fixed spread (back to original)
//
// Revision 1.14  2001/12/29 00:52:10  deathwatch
// Fixed HC sound bug (not making a sound when hc_single was 0)
//
// Revision 1.13  2001/12/23 21:19:41  deathwatch
// Updated stats with location and average
// cleaned it up a bit as well
//
// Revision 1.12  2001/12/23 16:30:51  ra
// 2.5 ready. New stats from Freud. HC and shotgun gibbing seperated.
//
// Revision 1.11  2001/11/08 20:56:24  igor_rock
// - changed some things related to wp_flags
// - corrected use_punch bug when player only has an empty weapon left
//
// Revision 1.10  2001/09/28 13:48:35  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.9  2001/09/02 20:33:34  deathwatch
// Added use_classic and fixed an issue with ff_afterround, also updated version
// nr and cleaned up some commands.
//
// Updated the VC Project to output the release build correctly.
//
// Revision 1.8  2001/08/17 21:31:37  deathwatch
// Added support for stats
//
// Revision 1.7  2001/08/06 14:38:45  ra
// Adding UVtime for ctf
//
// Revision 1.6  2001/07/18 15:19:11  slicerdw
// Time for weapons and items dissapearing is set to "6" to prevent lag on ctf
//
// Revision 1.5  2001/05/31 16:58:14  igor_rock
// conflicts resolved
//
// Revision 1.4.2.3  2001/05/27 17:24:10  igor_rock
// changed spawnpoint behavior in CTF
//
// Revision 1.4.2.2  2001/05/25 18:59:53  igor_rock
// Added CTF Mode completly :)
// Support for .flg files is still missing, but with "real" CTF maps like
// tq2gtd1 the ctf works fine.
// (I hope that all other modes still work, just tested DM and teamplay)
//
// Revision 1.4.2.1  2001/05/20 15:17:32  igor_rock
// removed the old ctf code completly
//
// Revision 1.4  2001/05/13 01:23:01  deathwatch
// Added Single Barreled Handcannon mode, made the menus and scoreboards
// look nicer and made the voice command a bit less loud.
//
// Revision 1.3  2001/05/11 16:07:26  mort
// Various CTF bits and pieces...
//
// Revision 1.2  2001/05/07 01:38:51  ra
//
//
// Added fixes for Ammo and Weaponsfarming.
//
// Revision 1.1.1.1  2001/05/06 17:25:06  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#include "g_local.h"
#include "m_player.h"


static qboolean is_quad;


void P_ProjectSource(gclient_t* client, vec3_t point, vec3_t distance,
	vec3_t forward, vec3_t right, vec3_t result)
{
	vec3_t _distance;

	VectorCopy(distance, _distance);

	if (client->pers.firing_style == ACTION_FIRING_CLASSIC ||
		client->pers.firing_style == ACTION_FIRING_CLASSIC_HIGH)
	{
		if (client->pers.hand == LEFT_HANDED)
			_distance[1] *= -1;
		else if (client->pers.hand == CENTER_HANDED)
			_distance[1] = 0;
	}
	else
	{
		_distance[1] = 0;		// fire from center always
	}

	G_ProjectSource(point, _distance, forward, right, result);
}

// used for setting up the positions of the guns in shell ejection
void
Old_ProjectSource(gclient_t* client, vec3_t point, vec3_t distance,
	vec3_t forward, vec3_t right, vec3_t result)
{
	vec3_t _distance;

	VectorCopy(distance, _distance);
	if (client->pers.hand == LEFT_HANDED)
		_distance[1] = -1;		// changed from = to *=
	  // Fireblade 2/28/99
	  // zucc reversed, this is only used for setting up shell ejection and 
	  // since those look good this shouldn't be changed
	else if (client->pers.hand == CENTER_HANDED)
		_distance[1] = 0;
	G_ProjectSource(point, _distance, forward, right, result);
}

// this one is the real old project source
void
Knife_ProjectSource(gclient_t* client, vec3_t point, vec3_t distance,
	vec3_t forward, vec3_t right, vec3_t result)
{
	vec3_t _distance;

	VectorCopy(distance, _distance);
	if (client->pers.hand == LEFT_HANDED)
		_distance[1] *= -1;		// changed from = to *=                                         
	else if (client->pers.hand == CENTER_HANDED)
		_distance[1] = 0;
	G_ProjectSource(point, _distance, forward, right, result);
}



/*
===============
PlayerNoise

  Each player can have two noise objects associated with it:
  a personal noise (jumping, pain, weapon firing), and a weapon
  target noise (bullet wall impacts)

		Monsters that don't directly see the player can move
		to a noise in hopes of seeing the player from there.
		===============
*/
void PlayerNoise(edict_t* who, vec3_t where, int type)
{
	/*
	if (type == PNOISE_WEAPON)
	{
		if (who->client->silencer_shots)
		{
			who->client->silencer_shots--;
			return;
		}
	}
	*/
}


void PlaceHolder(edict_t* ent)
{
	ent->nextthink = level.framenum + 1000 * HZ;
}

// keep the entity around so we can find it later if we need to respawn the weapon there
void SetSpecWeapHolder(edict_t* ent)
{
	ent->flags |= FL_RESPAWN;
	ent->svflags |= SVF_NOCLIENT;
	ent->solid = SOLID_NOT;
	ent->think = PlaceHolder;
	gi.linkentity(ent);
}

qboolean Pickup_Weapon(edict_t* ent, edict_t* other)
{
	int index, index2;
	gitem_t* ammo;
	qboolean addAmmo;
	int special = 0;
	int band = 0;

	index = ITEM_INDEX(ent->item);

	if (DMFLAGS(DF_WEAPONS_STAY) && other->client->inventory[index])
	{
		if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
			return false;		// leave the weapon for others to pickup
	}

	// find out if they have a bandolier
	if (INV_AMMO(other, BAND_NUM))
		band = 1;
	else
		band = 0;


	// zucc special cases for picking up weapons
	// the mk23 should never be dropped, probably

	switch (ent->item->typeNum) {
	case MK23_NUM:
		if (!WPF_ALLOWED(MK23_NUM))
			return false;

		if (other->client->inventory[index])	// already has one
		{
			if (!(ent->spawnflags & DROPPED_ITEM))
			{
				ammo = FindItem(ent->item->ammo);
				addAmmo = Add_Ammo(other, ammo, ammo->quantity);
				if (addAmmo && !(ent->spawnflags & DROPPED_PLAYER_ITEM))
					SetRespawn(ent, weapon_respawn->value);

				return addAmmo;
			}
		}
		other->client->inventory[index]++;
		if (!(ent->spawnflags & DROPPED_ITEM)) {
			other->client->mk23_rds = other->client->mk23_max;
			if (!(ent->spawnflags & DROPPED_PLAYER_ITEM))
				SetRespawn(ent, weapon_respawn->value);
		}
		return true;

	case MP5_NUM:
		if (other->client->unique_weapon_total >= unique_weapons->value + band)
			return false;		// we can't get it
		if ((!allow_hoarding->value) && other->client->inventory[index])
			return false;		// we already have one

		other->client->inventory[index]++;
		other->client->unique_weapon_total++;
		if (!(ent->spawnflags & DROPPED_ITEM)) {
			other->client->mp5_rds = other->client->mp5_max;
		}
		special = 1;
		gi.cprintf(other, PRINT_HIGH, "%s - Unique Weapon\n", ent->item->pickup_name);
		break;

	case M4_NUM:
		if (other->client->unique_weapon_total >= unique_weapons->value + band)
			return false;		// we can't get it
		if ((!allow_hoarding->value) && other->client->inventory[index])
			return false;		// we already have one

		other->client->inventory[index]++;
		other->client->unique_weapon_total++;
		if (!(ent->spawnflags & DROPPED_ITEM)) {
			other->client->m4_rds = other->client->m4_max;
		}
		special = 1;
		gi.cprintf(other, PRINT_HIGH, "%s - Unique Weapon\n", ent->item->pickup_name);
		break;

	case M3_NUM:
		if (other->client->unique_weapon_total >= unique_weapons->value + band)
			return false;		// we can't get it
		if ((!allow_hoarding->value) && other->client->inventory[index])
			return false;		// we already have one

		other->client->inventory[index]++;
		other->client->unique_weapon_total++;
		if (!(ent->spawnflags & DROPPED_ITEM))
		{
			// any weapon that doesn't completely fill up each reload can 
			//end up in a state where it has a full weapon and pending reload(s)
			if (other->client->weaponstate == WEAPON_RELOADING &&
				other->client->curr_weap == M3_NUM)
			{
				if (other->client->fast_reload)
					other->client->shot_rds = other->client->shot_max - 2;
				else
					other->client->shot_rds = other->client->shot_max - 1;
			}
			else
			{
				other->client->shot_rds = other->client->shot_max;
			}
		}
		special = 1;
		gi.cprintf(other, PRINT_HIGH, "%s - Unique Weapon\n", ent->item->pickup_name);
		break;

	case HC_NUM:
		if (other->client->unique_weapon_total >= unique_weapons->value + band)
			return false;		// we can't get it
		if ((!allow_hoarding->value) && other->client->inventory[index])
			return false;		// we already have one

		other->client->inventory[index]++;
		other->client->unique_weapon_total++;
		if (!(ent->spawnflags & DROPPED_ITEM))
		{
			other->client->cannon_rds = other->client->cannon_max;
			index2 = ITEM_INDEX(FindItem(ent->item->ammo));
			if (other->client->inventory[index2] + 5 > other->client->max_shells)
				other->client->inventory[index2] = other->client->max_shells;
			else
				other->client->inventory[index2] += 5;
		}
		gi.cprintf(other, PRINT_HIGH, "%s - Unique Weapon\n", ent->item->pickup_name);
		special = 1;
		break;

	case SNIPER_NUM:
		if (other->client->unique_weapon_total >= unique_weapons->value + band)
			return false;		// we can't get it
		if ((!allow_hoarding->value) && other->client->inventory[index])
			return false;		// we already have one

		other->client->inventory[index]++;
		other->client->unique_weapon_total++;
		if (!(ent->spawnflags & DROPPED_ITEM))
		{
			if (other->client->weaponstate == WEAPON_RELOADING &&
				other->client->curr_weap == SNIPER_NUM)
			{
				if (other->client->fast_reload)
					other->client->sniper_rds = other->client->sniper_max - 2;
				else
					other->client->sniper_rds = other->client->sniper_max - 1;
			}
			else
			{
				other->client->sniper_rds = other->client->sniper_max;
			}
		}
		special = 1;
		gi.cprintf(other, PRINT_HIGH, "%s - Unique Weapon\n", ent->item->pickup_name);
		break;

	case DUAL_NUM:
		if (!WPF_ALLOWED(MK23_NUM))
			return false;

		if (other->client->inventory[index])	// already has one
		{
			if (!(ent->spawnflags & DROPPED_ITEM))
			{
				ammo = FindItem(ent->item->ammo);
				addAmmo = Add_Ammo(other, ammo, ammo->quantity);
				if (addAmmo && !(ent->spawnflags & DROPPED_PLAYER_ITEM))
					SetRespawn(ent, weapon_respawn->value);

				return addAmmo;
			}
		}
		other->client->inventory[index]++;
		if (!(ent->spawnflags & DROPPED_ITEM))
		{
			other->client->dual_rds += other->client->mk23_max;
			// assume the player uses the new (full) pistol
			other->client->mk23_rds = other->client->mk23_max;
			if (!(ent->spawnflags & DROPPED_PLAYER_ITEM))
				SetRespawn(ent, weapon_respawn->value);
		}
		return true;

	case KNIFE_NUM:
		if (other->client->inventory[index] < other->client->knife_max)
		{
			other->client->inventory[index]++;
			return true;
		}

		return false;

	case GRENADE_NUM:
		if (!(gameSettings & GS_DEATHMATCH) && ctf->value != 2 && !band)
			return false;

		if (other->client->inventory[index] >= other->client->grenade_max)
			return false;

		other->client->inventory[index]++;
		if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
		{
			if (DMFLAGS(DF_WEAPONS_STAY))
				ent->flags |= FL_RESPAWN;
			else
				SetRespawn(ent, ammo_respawn->value);
		}
		return true;

	default:
		other->client->inventory[index]++;

		if (!(ent->spawnflags & DROPPED_ITEM))
		{
			// give them some ammo with it
			ammo = FindItem(ent->item->ammo);

			if (DMFLAGS(DF_INFINITE_AMMO))
				Add_Ammo(other, ammo, 1000);
			else
				Add_Ammo(other, ammo, ammo->quantity);

			if (!(ent->spawnflags & DROPPED_PLAYER_ITEM))
			{
				if (DMFLAGS(DF_WEAPONS_STAY))
					ent->flags |= FL_RESPAWN;
				else
					SetRespawn(ent, 30);
			}
		}
		break;
	}

	if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM))
		&& (SPEC_WEAPON_RESPAWN) && special)
	{
		if (DMFLAGS(DF_WEAPON_RESPAWN) && ((gameSettings & GS_DEATHMATCH) || ctf->value == 2))
			SetRespawn(ent, weapon_respawn->value);
		else
			SetSpecWeapHolder(ent);
	}

	return true;
}


// zucc vwep 3.17(?) vwep support
void ShowGun(edict_t* ent)
{

	int nIndex;

	// No weapon?
	if (!ent->client->weapon) {
		ent->s.modelindex2 = 0;
		return;
	}

	// Determine the weapon's precache index.
	nIndex = ent->client->weapon->typeNum;

	// Clear previous weapon model.
	ent->s.skinnum &= 255;

	// Set new weapon model.
	ent->s.skinnum |= (nIndex << 8);
	ent->s.modelindex2 = 255;
}

//#define GRENADE_IDLE_FIRST 40
//#define GRENADE_IDLE_LAST 69

/*
===============
ChangeWeapon

The old weapon has been dropped all the way, so make the new one
current
===============
*/
void ChangeWeapon(edict_t* ent)
{

	if (ent->client->grenade_framenum)
	{
		ent->client->grenade_framenum = level.framenum;
		weapon_grenade_fire(ent, false);
		ent->client->grenade_framenum = 0;
	}

	if (ent->client->ctf_grapple)
		CTFPlayerResetGrapple(ent);

	// zucc - prevent reloading queue for previous weapon from doing anything
	ent->client->reload_attempts = 0;

	ent->client->lastweapon = ent->client->weapon;
	ent->client->weapon = ent->client->newweapon;
	ent->client->newweapon = NULL;
	ent->client->machinegun_shots = 0;

	if (ent->client->weapon && ent->client->weapon->ammo)
		ent->client->ammo_index = ITEM_INDEX(FindItem(ent->client->weapon->ammo));
	else
		ent->client->ammo_index = 0;

	if (!ent->client->weapon || ent->s.modelindex != 255)	// zucc vwep
	{				// dead 
		ent->client->ps.gunindex = 0;
		return;
	}

	ent->client->weaponstate = WEAPON_ACTIVATING;
	ent->client->ps.gunframe = 0;

	//FIREBLADE
	if (ent->solid == SOLID_NOT && ent->deadflag != DEAD_DEAD)
		return;
	//FIREBLADE

	ent->client->ps.gunindex = gi.modelindex(ent->client->weapon->view_model);

	// zucc hentai's animation for vwep
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		SetAnimation( ent, FRAME_crpain1, FRAME_crpain4, ANIM_PAIN );
	else
		SetAnimation( ent, FRAME_pain301, FRAME_pain304, ANIM_PAIN );

	ShowGun(ent);
	// zucc done

	ent->client->curr_weap = ent->client->weapon->typeNum;
	if (ent->client->curr_weap == GRENADE_NUM) {
		// Fix the "use grenades;drop bandolier" bug, caused infinite grenades.
		if (teamplay->value && INV_AMMO(ent, GRENADE_NUM) == 0)
			INV_AMMO(ent, GRENADE_NUM) = 1;
	}

	if (INV_AMMO(ent, LASER_NUM))
		SP_LaserSight(ent, GET_ITEM(LASER_NUM));	//item->use(ent, item);
}

/*
=================
Think_Weapon

Called by ClientBeginServerFrame and ClientThink
=================
*/
void Think_Weapon(edict_t* ent)
{
	// if just died, put the weapon away
	if (ent->health < 1)
	{
		ent->client->newweapon = NULL;
		ChangeWeapon(ent);
	}

	// call active weapon think routine
	if (ent->client->weapon && ent->client->weapon->weaponthink)
	{
		is_quad = (ent->client->quad_framenum > level.framenum);
		ent->client->weapon->weaponthink(ent);
	}
}


/*
================
Use_Weapon

Make the weapon ready if there is ammo
================
*/
void
Use_Weapon(edict_t* ent, gitem_t* item)
{
	//int                   ammo_index;
	//gitem_t               *ammo_item;


  //        if(ent->client->weaponstate == WEAPON_BANDAGING || ent->client->bandaging == 1 )
	//                      return;




	// see if we're already using it
	if (item == ent->client->weapon)
		return;

	// zucc - let them change if they want
	/*if (item->ammo && !g_select_empty->value && !(item->flags & IT_AMMO))
	   {
	   ammo_item = FindItem(item->ammo);
	   ammo_index = ITEM_INDEX(ammo_item);

	   if (!ent->client->inventory[ammo_index])
	   {
	   gi.cprintf (ent, PRINT_HIGH, "No %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
	   return;
	   }

	   if (ent->client->inventory[ammo_index] < item->quantity)
	   {
	   gi.cprintf (ent, PRINT_HIGH, "Not enough %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
	   return;
	   }
	   } */

	   // change to this weapon when down
	ent->client->newweapon = item;
}




edict_t*
FindSpecWeapSpawn(edict_t* ent)
{
	edict_t* spot = NULL;

	//gi.bprintf (PRINT_HIGH, "Calling the FindSpecWeapSpawn\n");
	spot = G_Find(spot, FOFS(classname), ent->classname);
	//gi.bprintf (PRINT_HIGH, "spot = %p and spot->think = %p and playerholder = %p, spot, (spot ? spot->think : 0), PlaceHolder\n");
	while (spot && spot->think != PlaceHolder)	//(spot->spawnflags & DROPPED_ITEM ) && spot->think != PlaceHolder )//spot->solid == SOLID_NOT )        
	{
		//              gi.bprintf (PRINT_HIGH, "Calling inside the loop FindSpecWeapSpawn\n");
		spot = G_Find(spot, FOFS(classname), ent->classname);
	}
	/*      if (!spot)
			{
					gi.bprintf(PRINT_HIGH, "Failed to find a spawn spot for %s\n", ent->classname);
			}
			else
					gi.bprintf(PRINT_HIGH, "Found a spawn spot for %s\n", ent->classname);
	*/
	return spot;
}

static void
SpawnSpecWeap(gitem_t* item, edict_t* spot)
{
	/*        edict_t *ent;
			vec3_t  forward, right;
			vec3_t  angles;

			ent = G_Spawn();

			ent->classname = item->classname;
			ent->item = item;
			ent->spawnflags = DROPPED_PLAYER_ITEM;//DROPPED_ITEM;
			ent->s.effects = item->world_model_flags;
			ent->s.renderfx = RF_GLOW;
			VectorSet (ent->mins, -15, -15, -15);
			VectorSet (ent->maxs, 15, 15, 15);
			gi.setmodel (ent, ent->item->world_model);
			ent->solid = SOLID_TRIGGER;
			ent->movetype = MOVETYPE_TOSS;
			ent->touch = Touch_Item;
			ent->owner = ent;

			angles[0] = 0;
			angles[1] = rand() % 360;
			angles[2] = 0;

			AngleVectors (angles, forward, right, NULL);
			VectorCopy (spot->s.origin, ent->s.origin);
			ent->s.origin[2] += 16;
			VectorScale (forward, 100, ent->velocity);
			ent->velocity[2] = 300;

			ent->think = NULL;

			gi.linkentity (ent);
					*/
	SetRespawn(spot, 1);
	gi.linkentity(spot);
}

void temp_think_specweap(edict_t* ent)
{
	ent->touch = Touch_Item;

	if (allweapon->value) { // allweapon set
		ent->nextthink = level.framenum + 1 * HZ;
		ent->think = G_FreeEdict;
		return;
	}

	if (gameSettings & GS_ROUNDBASED) {
		ent->nextthink = level.framenum + 1000 * HZ;
		ent->think = PlaceHolder;
		return;
	}

	if (gameSettings & GS_WEAPONCHOOSE) {
		ent->nextthink = level.framenum + 6 * HZ;
		ent->think = ThinkSpecWeap;
	}
	else if (DMFLAGS(DF_WEAPON_RESPAWN)) {
		ent->nextthink = level.framenum + (weapon_respawn->value * 0.6f) * HZ;
		ent->think = G_FreeEdict;
	}
	else {
		ent->nextthink = level.framenum + weapon_respawn->value * HZ;
		ent->think = ThinkSpecWeap;
	}
}



// zucc make dropped weapons respawn elsewhere
void
ThinkSpecWeap(edict_t* ent)
{
	edict_t* spot;

	if ((spot = FindSpecWeapSpawn(ent)) != NULL)
	{
		SpawnSpecWeap(ent->item, spot);
		G_FreeEdict(ent);
	}
	else
	{
		ent->nextthink = level.framenum + 1 * HZ;
		ent->think = G_FreeEdict;
	}
}





/*
================
Drop_Weapon
================
*/
void Drop_Weapon(edict_t* ent, gitem_t* item)
{
	int index;
	gitem_t* replacement;
	edict_t* temp = NULL;

	if (DMFLAGS(DF_WEAPONS_STAY))
		return;

	// AQ:TNG - JBravo fixing weapon farming
	if (ent->client->weaponstate == WEAPON_DROPPING ||
		ent->client->weaponstate == WEAPON_BUSY)
		return;
	// Weapon farming fix end.

	if (ent->client->weaponstate == WEAPON_BANDAGING ||
		ent->client->bandaging == 1 || ent->client->bandage_stopped == 1)
	{
		gi.cprintf(ent, PRINT_HIGH,
			"You are too busy bandaging right now...\n");
		return;
	}
	// don't let them drop this, causes duplication
	if (ent->client->newweapon == item)
		return;

	index = ITEM_INDEX(item);
	// see if we're already using it
	//zucc special cases for dropping       
	if (item->typeNum == MK23_NUM)
	{
		gi.cprintf(ent, PRINT_HIGH, "Can't drop the %s.\n", MK23_NAME);
		return;
	}
	else if (item->typeNum == MP5_NUM)
	{

		if (ent->client->weapon == item && ent->client->inventory[index] == 1)
		{
			replacement = GET_ITEM(MK23_NUM);	// back to the pistol then
			ent->client->newweapon = replacement;
			ent->client->weaponstate = WEAPON_DROPPING;
			ent->client->ps.gunframe = 51;

			//ChangeWeapon( ent );
		}
		ent->client->unique_weapon_total--;	// dropping 1 unique weapon
		temp = Drop_Item(ent, item);
		temp->think = temp_think_specweap;
		ent->client->inventory[index]--;
	}
	else if (item->typeNum == M4_NUM)
	{

		if (ent->client->weapon == item && ent->client->inventory[index] == 1)
		{
			replacement = GET_ITEM(MK23_NUM);	// back to the pistol then
			ent->client->newweapon = replacement;
			ent->client->weaponstate = WEAPON_DROPPING;
			ent->client->ps.gunframe = 44;

			//ChangeWeapon( ent );
		}
		ent->client->unique_weapon_total--;	// dropping 1 unique weapon
		temp = Drop_Item(ent, item);
		temp->think = temp_think_specweap;
		ent->client->inventory[index]--;
	}
	else if (item->typeNum == M3_NUM)
	{

		if (ent->client->weapon == item && ent->client->inventory[index] == 1)
		{
			replacement = GET_ITEM(MK23_NUM);	// back to the pistol then
			ent->client->newweapon = replacement;
			ent->client->weaponstate = WEAPON_DROPPING;
			ent->client->ps.gunframe = 41;
			//ChangeWeapon( ent );
		}
		ent->client->unique_weapon_total--;	// dropping 1 unique weapon
		temp = Drop_Item(ent, item);
		temp->think = temp_think_specweap;
		ent->client->inventory[index]--;
	}
	else if (item->typeNum == HC_NUM)
	{
		if (ent->client->weapon == item && ent->client->inventory[index] == 1)
		{
			replacement = GET_ITEM(MK23_NUM);	// back to the pistol then
			ent->client->newweapon = replacement;
			ent->client->weaponstate = WEAPON_DROPPING;
			ent->client->ps.gunframe = 61;
			//ChangeWeapon( ent );
		}
		ent->client->unique_weapon_total--;	// dropping 1 unique weapon
		temp = Drop_Item(ent, item);
		temp->think = temp_think_specweap;
		ent->client->inventory[index]--;
	}
	else if (item->typeNum == SNIPER_NUM)
	{
		if (ent->client->weapon == item && ent->client->inventory[index] == 1)
		{
			// in case they are zoomed in
			ent->client->ps.fov = 90;
			ent->client->desired_fov = 90;
			replacement = GET_ITEM(MK23_NUM);	// back to the pistol then
			ent->client->newweapon = replacement;
			ent->client->weaponstate = WEAPON_DROPPING;
			ent->client->ps.gunframe = 50;
			//ChangeWeapon( ent );
		}
		ent->client->unique_weapon_total--;	// dropping 1 unique weapon
		temp = Drop_Item(ent, item);
		temp->think = temp_think_specweap;
		ent->client->inventory[index]--;
	}
	else if (item->typeNum == DUAL_NUM)
	{
		if (ent->client->weapon == item)
		{
			replacement = GET_ITEM(MK23_NUM);	// back to the pistol then
			ent->client->newweapon = replacement;
			ent->client->weaponstate = WEAPON_DROPPING;
			ent->client->ps.gunframe = 40;
			//ChangeWeapon( ent );
		}
		ent->client->dual_rds = ent->client->mk23_rds;
	}
	else if (item->typeNum == KNIFE_NUM)
	{
		//gi.cprintf(ent, PRINT_HIGH, "Before checking knife inven frames = %d\n", ent->client->ps.gunframe);

		if (ent->client->weapon == item && ent->client->inventory[index] == 1)
		{
			replacement = GET_ITEM(MK23_NUM);	// back to the pistol then
			ent->client->newweapon = replacement;
			if (ent->client->pers.knife_mode)	// hack to avoid an error
			{
				ent->client->weaponstate = WEAPON_DROPPING;
				ent->client->ps.gunframe = 111;
			}
			else
				ChangeWeapon(ent);
			//      gi.cprintf(ent, PRINT_HIGH, "After change weap from knife drop frames = %d\n", ent->client->ps.gunframe);
		}
	}
	else if (item->typeNum == GRENADE_NUM)
	{
		if (ent->client->weapon == item && ent->client->inventory[index] == 1)
		{
			if (((ent->client->ps.gunframe >= GRENADE_IDLE_FIRST)
				&& (ent->client->ps.gunframe <= GRENADE_IDLE_LAST))
				|| ((ent->client->ps.gunframe >= GRENADE_THROW_FIRST
					&& ent->client->ps.gunframe <= GRENADE_THROW_LAST)))
			{
				int damage;

				ent->client->ps.gunframe = 0;

				// Reset Grenade Damage to 1.52 when requested:
				if (use_classic->value)
					damage = GRENADE_DAMRAD_CLASSIC;
				else
					damage = GRENADE_DAMRAD;

				if (ent->client->quad_framenum > level.framenum)
					damage *= 1.5f;

				fire_grenade2(ent, ent->s.origin, vec3_origin, damage, 0, 2 * HZ, damage * 2, false);

				INV_AMMO(ent, GRENADE_NUM)--;
				ent->client->newweapon = GET_ITEM(MK23_NUM);
				ent->client->weaponstate = WEAPON_DROPPING;
				ent->client->ps.gunframe = 0;
				return;
			}

			replacement = GET_ITEM(MK23_NUM);	// back to the pistol then
			ent->client->newweapon = replacement;
			ent->client->weaponstate = WEAPON_DROPPING;
			ent->client->ps.gunframe = 0;
			//ChangeWeapon( ent );
		}
	}
	else if ((item == ent->client->weapon || item == ent->client->newweapon)
		&& ent->client->inventory[index] == 1)
	{
		gi.cprintf(ent, PRINT_HIGH, "Can't drop current weapon\n");
		return;
	}

	// AQ:TNG - JBravo fixing weapon farming
	if (ent->client->unique_weapon_total < 0)
		ent->client->unique_weapon_total = 0;
	if (ent->client->inventory[index] < 0)
		ent->client->inventory[index] = 0;
	// Weapon farming fix end

	if (!temp)
	{
		temp = Drop_Item(ent, item);
		ent->client->inventory[index]--;
	}

}





//zucc drop special weapon (only 1 of them)
void DropSpecialWeapon(edict_t* ent)
{
	int itemNum = ent->client->weapon ? ent->client->weapon->typeNum : 0;

	// first check if their current weapon is a special weapon, if so, drop it.
	if (itemNum >= MP5_NUM && itemNum <= SNIPER_NUM)
		Drop_Weapon(ent, ent->client->weapon);
	else if (INV_AMMO(ent, SNIPER_NUM) > 0)
		Drop_Weapon(ent, GET_ITEM(SNIPER_NUM));
	else if (INV_AMMO(ent, HC_NUM) > 0)
		Drop_Weapon(ent, GET_ITEM(HC_NUM));
	else if (INV_AMMO(ent, M3_NUM) > 0)
		Drop_Weapon(ent, GET_ITEM(M3_NUM));
	else if (INV_AMMO(ent, MP5_NUM) > 0)
		Drop_Weapon(ent, GET_ITEM(MP5_NUM));
	else if (INV_AMMO(ent, M4_NUM) > 0)
		Drop_Weapon(ent, GET_ITEM(M4_NUM));
	// special case, aq does this, guess I can support it
	else if (itemNum == DUAL_NUM)
		ent->client->newweapon = GET_ITEM(MK23_NUM);

}

// used for when we want to force a player to drop an extra special weapon
// for when they drop the bandolier and they are over the weapon limit
void DropExtraSpecial(edict_t* ent)
{
	int itemNum;

	itemNum = ent->client->weapon ? ent->client->weapon->typeNum : 0;
	if (itemNum >= MP5_NUM && itemNum <= SNIPER_NUM)
	{
		// if they have more than 1 then they are willing to drop one           
		if (INV_AMMO(ent, itemNum) > 1) {
			Drop_Weapon(ent, ent->client->weapon);
			return;
		}
	}
	// otherwise drop some weapon they aren't using
	if (INV_AMMO(ent, SNIPER_NUM) > 0 && SNIPER_NUM != itemNum)
		Drop_Weapon(ent, GET_ITEM(SNIPER_NUM));
	else if (INV_AMMO(ent, HC_NUM) > 0 && HC_NUM != itemNum)
		Drop_Weapon(ent, GET_ITEM(HC_NUM));
	else if (INV_AMMO(ent, M3_NUM) > 0 && M3_NUM != itemNum)
		Drop_Weapon(ent, GET_ITEM(M3_NUM));
	else if (INV_AMMO(ent, MP5_NUM) > 0 && MP5_NUM != itemNum)
		Drop_Weapon(ent, GET_ITEM(MP5_NUM));
	else if (INV_AMMO(ent, M4_NUM) > 0 && M4_NUM != itemNum)
		Drop_Weapon(ent, GET_ITEM(M4_NUM));
	else
		gi.dprintf("Couldn't find the appropriate weapon to drop.\n");
}

//zucc ready special weapon
void ReadySpecialWeapon(edict_t* ent)
{
	int weapons[5] = { MP5_NUM, M4_NUM, M3_NUM, HC_NUM, SNIPER_NUM };
	int curr, i;
	int last;


	if (ent->client->weaponstate == WEAPON_BANDAGING || ent->client->bandaging == 1)
		return;


	for (curr = 0; curr < 5; curr++)
	{
		if (ent->client->curr_weap == weapons[curr])
			break;
	}
	if (curr >= 5)
	{
		curr = -1;
		last = 5;
	}
	else
	{
		last = curr + 5;
	}

	for (i = (curr + 1); i != last; i = (i + 1))
	{
		if (INV_AMMO(ent, weapons[i % 5]))
		{
			ent->client->newweapon = GET_ITEM(weapons[i % 5]);
			return;
		}
	}
}

/*
================
Weapon_Generic

A generic function to handle the basics of weapon thinking
================
*/

//zucc - copied in BD's code, modified for use with other weapons

#define FRAME_FIRE_FIRST                (FRAME_ACTIVATE_LAST + 1)
#define FRAME_IDLE_FIRST                (FRAME_FIRE_LAST + 1)
#define FRAME_DEACTIVATE_FIRST  (FRAME_IDLE_LAST + 1)

#define FRAME_RELOAD_FIRST              (FRAME_DEACTIVATE_LAST +1)
#define FRAME_LASTRD_FIRST   (FRAME_RELOAD_LAST +1)

#define MK23MAG 12
#define MP5MAG  30
#define M4MAG   24
#define DUALMAG 24

void
Weapon_Generic(edict_t* ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST,
	int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST,
	int FRAME_RELOAD_LAST, int FRAME_LASTRD_LAST,
	int* pause_frames, int* fire_frames,
	void (*fire) (edict_t* ent))
{
	int n;
	int bFire = 0;
	int bOut = 0;
	/*        int                     bBursting = 0;*/


	  // zucc vwep
	if (ent->s.modelindex != 255)
		return;			// not on client, so VWep animations could do wacky things

	if (ent->client->ctf_grapple && ent->client->weapon) {
		if (Q_stricmp(ent->client->weapon->pickup_name, "Grapple") == 0 &&
			ent->client->weaponstate == WEAPON_FIRING)
			return;
	}

	//FIREBLADE
	if (ent->client->weaponstate == WEAPON_FIRING &&
		((ent->solid == SOLID_NOT && ent->deadflag != DEAD_DEAD) ||
			lights_camera_action))
	{
		ent->client->weaponstate = WEAPON_READY;
	}
	//FIREBLADE

	  //+BD - Added Reloading weapon, done manually via a cmd
	if (ent->client->weaponstate == WEAPON_RELOADING)
	{
		if (ent->client->ps.gunframe < FRAME_RELOAD_FIRST
			|| ent->client->ps.gunframe > FRAME_RELOAD_LAST)
			ent->client->ps.gunframe = FRAME_RELOAD_FIRST;
		else if (ent->client->ps.gunframe < FRAME_RELOAD_LAST)
		{
			ent->client->ps.gunframe++;
			switch (ent->client->curr_weap)
			{
				//+BD - Check weapon to find out when to play reload sounds
			case MK23_NUM:
			{
				if (ent->client->ps.gunframe == 46)
					gi.sound(ent, CHAN_WEAPON,
						gi.soundindex("weapons/mk23out.wav"), 1,
						ATTN_NORM, 0);
				else if (ent->client->ps.gunframe == 53)
					gi.sound(ent, CHAN_WEAPON,
						gi.soundindex("weapons/mk23in.wav"), 1,
						ATTN_NORM, 0);
				else if (ent->client->ps.gunframe == 59)	// 3
					gi.sound(ent, CHAN_WEAPON,
						gi.soundindex("weapons/mk23slap.wav"), 1,
						ATTN_NORM, 0);
				break;
			}
			case MP5_NUM:
			{
				if (ent->client->ps.gunframe == 55)
					gi.sound(ent, CHAN_WEAPON,
						gi.soundindex("weapons/mp5out.wav"), 1,
						ATTN_NORM, 0);
				else if (ent->client->ps.gunframe == 59)
					gi.sound(ent, CHAN_WEAPON,
						gi.soundindex("weapons/mp5in.wav"), 1, ATTN_NORM,
						0);
				else if (ent->client->ps.gunframe == 63)	//61
					gi.sound(ent, CHAN_WEAPON,
						gi.soundindex("weapons/mp5slap.wav"), 1,
						ATTN_NORM, 0);
				break;
			}
			case M4_NUM:
			{
				if (ent->client->ps.gunframe == 52)
					gi.sound(ent, CHAN_WEAPON,
						gi.soundindex("weapons/m4a1out.wav"), 1,
						ATTN_NORM, 0);
				else if (ent->client->ps.gunframe == 58)
					gi.sound(ent, CHAN_WEAPON,
						gi.soundindex("weapons/m4a1in.wav"), 1,
						ATTN_NORM, 0);
				break;
			}
			case M3_NUM:
			{
				if (ent->client->shot_rds >= ent->client->shot_max)
				{
					ent->client->ps.gunframe = FRAME_IDLE_FIRST;
					ent->client->weaponstate = WEAPON_READY;
					return;
				}

				if (ent->client->ps.gunframe == 48)
				{
					gi.sound(ent, CHAN_WEAPON,
						gi.soundindex("weapons/m3in.wav"), 1,
						ATTN_NORM, 0);
				}
				if (ent->client->ps.gunframe == 49)
				{
					if (ent->client->fast_reload == 1)
					{
						ent->client->fast_reload = 0;
						ent->client->shot_rds++;
						ent->client->ps.gunframe = 44;
					}
				}
				break;
			}
			case HC_NUM:
			{
				if (ent->client->ps.gunframe == 64)
					gi.sound(ent, CHAN_WEAPON,
						gi.soundindex("weapons/copen.wav"), 1, ATTN_NORM,
						0);
				else if (ent->client->ps.gunframe == 67)
					gi.sound(ent, CHAN_WEAPON,
						gi.soundindex("weapons/cout.wav"), 1, ATTN_NORM,
						0);
				else if (ent->client->ps.gunframe == 76)	//61
					gi.sound(ent, CHAN_WEAPON,
						gi.soundindex("weapons/cin.wav"), 1, ATTN_NORM,
						0);
				else if (ent->client->ps.gunframe == 80)	// 3
					gi.sound(ent, CHAN_WEAPON,
						gi.soundindex("weapons/cclose.wav"), 1,
						ATTN_NORM, 0);
				break;
			}
			case SNIPER_NUM:
			{

				if (ent->client->sniper_rds >= ent->client->sniper_max)
				{
					ent->client->ps.gunframe = FRAME_IDLE_FIRST;
					ent->client->weaponstate = WEAPON_READY;
					return;
				}

				if (ent->client->ps.gunframe == 59)
				{
					gi.sound(ent, CHAN_WEAPON,
						gi.soundindex("weapons/ssgbolt.wav"), 1,
						ATTN_NORM, 0);
				}
				else if (ent->client->ps.gunframe == 71)
				{
					gi.sound(ent, CHAN_WEAPON,
						gi.soundindex("weapons/ssgin.wav"), 1,
						ATTN_NORM, 0);
				}
				else if (ent->client->ps.gunframe == 73)
				{
					if (ent->client->fast_reload == 1)
					{
						ent->client->fast_reload = 0;
						ent->client->sniper_rds++;
						ent->client->ps.gunframe = 67;
					}
				}
				else if (ent->client->ps.gunframe == 76)
				{
					gi.sound(ent, CHAN_WEAPON,
						gi.soundindex("weapons/ssgbolt.wav"), 1,
						ATTN_NORM, 0);
				}
				break;
			}
			case DUAL_NUM:
			{
				if (ent->client->ps.gunframe == 45)
					gi.sound(ent, CHAN_WEAPON,
						gi.soundindex("weapons/mk23out.wav"), 1,
						ATTN_NORM, 0);
				else if (ent->client->ps.gunframe == 53)
					gi.sound(ent, CHAN_WEAPON,
						gi.soundindex("weapons/mk23slap.wav"), 1,
						ATTN_NORM, 0);
				else if (ent->client->ps.gunframe == 60)	// 3
					gi.sound(ent, CHAN_WEAPON,
						gi.soundindex("weapons/mk23slap.wav"), 1,
						ATTN_NORM, 0);
				break;
			}
			default:
				gi.dprintf("No weapon choice for reloading (sounds).\n");
				break;

			}
		}
		else
		{
			ent->client->ps.gunframe = FRAME_IDLE_FIRST;
			ent->client->weaponstate = WEAPON_READY;
			switch (ent->client->curr_weap)
			{
			case MK23_NUM:
			{
				ent->client->dual_rds -= ent->client->mk23_rds;
				ent->client->mk23_rds = ent->client->mk23_max;
				ent->client->dual_rds += ent->client->mk23_max;
				(ent->client->inventory[ent->client->ammo_index])--;
				if (ent->client->inventory[ent->client->ammo_index] < 0)
				{
					ent->client->inventory[ent->client->ammo_index] = 0;
				}
				ent->client->fired = 0;	// reset any firing delays
				break;
				//else
				//      ent->client->mk23_rds = ent->client->inventory[ent->client->ammo_index];
			}
			case MP5_NUM:
			{

				ent->client->mp5_rds = ent->client->mp5_max;
				(ent->client->inventory[ent->client->ammo_index])--;
				if (ent->client->inventory[ent->client->ammo_index] < 0)
				{
					ent->client->inventory[ent->client->ammo_index] = 0;
				}
				ent->client->fired = 0;	// reset any firing delays
				ent->client->burst = 0;	// reset any bursting
				break;
			}
			case M4_NUM:
			{

				ent->client->m4_rds = ent->client->m4_max;
				(ent->client->inventory[ent->client->ammo_index])--;
				if (ent->client->inventory[ent->client->ammo_index] < 0)
				{
					ent->client->inventory[ent->client->ammo_index] = 0;
				}
				ent->client->fired = 0;	// reset any firing delays
				ent->client->burst = 0;	// reset any bursting                           
				ent->client->machinegun_shots = 0;
				break;
			}
			case M3_NUM:
			{
				ent->client->shot_rds++;
				(ent->client->inventory[ent->client->ammo_index])--;
				if (ent->client->inventory[ent->client->ammo_index] < 0)
				{
					ent->client->inventory[ent->client->ammo_index] = 0;
				}
				break;
			}
			case HC_NUM:
			{
				if (hc_single->value)
				{
					int count = 0;

					if (ent->client->cannon_rds == 1)
						count = 1;
					else if ((ent->client->inventory[ent->client->ammo_index]) == 1)
						count = 1;
					else
						count = ent->client->cannon_max;
					ent->client->cannon_rds += count;
					if (ent->client->cannon_rds > ent->client->cannon_max)
						ent->client->cannon_rds = ent->client->cannon_max;

					(ent->client->inventory[ent->client->ammo_index]) -= count;
				}
				else
				{
					ent->client->cannon_rds = ent->client->cannon_max;
					(ent->client->inventory[ent->client->ammo_index]) -= 2;
				}

				if (ent->client->inventory[ent->client->ammo_index] < 0)
				{
					ent->client->inventory[ent->client->ammo_index] = 0;
				}
				break;
			}
			case SNIPER_NUM:
			{
				ent->client->sniper_rds++;
				(ent->client->inventory[ent->client->ammo_index])--;
				if (ent->client->inventory[ent->client->ammo_index] < 0)
				{
					ent->client->inventory[ent->client->ammo_index] = 0;
				}
				return;
			}
			case DUAL_NUM:
			{
				ent->client->dual_rds = ent->client->dual_max;
				ent->client->mk23_rds = ent->client->mk23_max;
				(ent->client->inventory[ent->client->ammo_index]) -= 2;
				if (ent->client->inventory[ent->client->ammo_index] < 0)
				{
					ent->client->inventory[ent->client->ammo_index] = 0;
				}
				break;
			}
			default:
				gi.dprintf("No weapon choice for reloading.\n");
				break;
			}


		}
	}

	if (ent->client->weaponstate == WEAPON_END_MAG)
	{
		if (ent->client->ps.gunframe < FRAME_LASTRD_LAST)
			ent->client->ps.gunframe++;
		else
			ent->client->ps.gunframe = FRAME_LASTRD_LAST;
		// see if our weapon has ammo (from something other than reloading)
		if (
			((ent->client->curr_weap == MK23_NUM)
				&& (ent->client->mk23_rds > 0))
			|| ((ent->client->curr_weap == MP5_NUM)
				&& (ent->client->mp5_rds > 0))
			|| ((ent->client->curr_weap == M4_NUM) && (ent->client->m4_rds > 0))
			|| ((ent->client->curr_weap == M3_NUM)
				&& (ent->client->shot_rds > 0))
			|| ((ent->client->curr_weap == HC_NUM)
				&& (ent->client->cannon_rds > 0))
			|| ((ent->client->curr_weap == SNIPER_NUM)
				&& (ent->client->sniper_rds > 0))
			|| ((ent->client->curr_weap == DUAL_NUM)
				&& (ent->client->dual_rds > 0)))
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = FRAME_IDLE_FIRST;
		}
		if (((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK)
			&& (ent->solid != SOLID_NOT || ent->deadflag == DEAD_DEAD)
			&& !lights_camera_action && !ent->client->uvTime)
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;
			{
				if (level.framenum >= ent->pain_debounce_framenum)
				{
					gi.sound(ent, CHAN_VOICE, level.snd_noammo, 1, ATTN_NORM, 0);
					ent->pain_debounce_framenum = level.framenum + 1 * HZ;
				}


			}

		}
	}

	if (ent->client->weaponstate == WEAPON_DROPPING)
	{
		if (ent->client->ps.gunframe == FRAME_DEACTIVATE_LAST)
		{

			ChangeWeapon(ent);
			return;
		}
		// zucc for vwep
		else if ((FRAME_DEACTIVATE_LAST - ent->client->ps.gunframe) == 4)
		{
			if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
				SetAnimation( ent, FRAME_crpain4 + 1, FRAME_crpain1, ANIM_REVERSE );
			else
				SetAnimation( ent, FRAME_pain304 + 1, FRAME_pain301, ANIM_REVERSE );
		}


		ent->client->ps.gunframe++;
		return;
	}

	if (ent->client->weaponstate == WEAPON_BANDAGING)
	{
		if (ent->client->ps.gunframe == FRAME_DEACTIVATE_LAST)
		{
			ent->client->weaponstate = WEAPON_BUSY;
			ent->client->idle_weapon = BANDAGE_TIME;
			return;
		}
		ent->client->ps.gunframe++;
		return;
	}


	if (ent->client->weaponstate == WEAPON_ACTIVATING)
	{
		if (ent->client->ps.gunframe == FRAME_ACTIVATE_LAST)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = FRAME_IDLE_FIRST;
			return;
		}

		// sounds for activation?
		switch (ent->client->curr_weap)
		{
		case MK23_NUM:
		{
			if (ent->client->dual_rds >= ent->client->mk23_max)
				ent->client->mk23_rds = ent->client->mk23_max;
			else
				ent->client->mk23_rds = ent->client->dual_rds;
			if (ent->client->ps.gunframe == 3)	// 3
			{
				if (ent->client->mk23_rds > 0)
				{
					gi.sound(ent, CHAN_WEAPON,
						gi.soundindex("weapons/mk23slide.wav"), 1,
						ATTN_NORM, 0);
				}
				else
				{
					gi.sound(ent, CHAN_WEAPON, level.snd_noammo, 1, ATTN_NORM, 0);
					//mk23slap
					ent->client->ps.gunframe = 62;
					ent->client->weaponstate = WEAPON_END_MAG;
				}
			}
			ent->client->fired = 0;	//reset any firing delays
			break;
		}
		case MP5_NUM:
		{
			if (ent->client->ps.gunframe == 3)	// 3
				gi.sound(ent, CHAN_WEAPON,
					gi.soundindex("weapons/mp5slide.wav"), 1, ATTN_NORM,
					0);
			ent->client->fired = 0;	//reset any firing delays
			ent->client->burst = 0;
			break;
		}
		case M4_NUM:
		{
			if (ent->client->ps.gunframe == 3)	// 3
				gi.sound(ent, CHAN_WEAPON,
					gi.soundindex("weapons/m4a1slide.wav"), 1, ATTN_NORM,
					0);
			ent->client->fired = 0;	//reset any firing delays
			ent->client->burst = 0;
			ent->client->machinegun_shots = 0;
			break;
		}
		case M3_NUM:
		{
			ent->client->fired = 0;	//reset any firing delays
			ent->client->burst = 0;
			ent->client->fast_reload = 0;
			break;
		}
		case HC_NUM:
		{
			ent->client->fired = 0;	//reset any firing delays
			ent->client->burst = 0;
			break;
		}
		case SNIPER_NUM:
		{
			ent->client->fired = 0;	//reset any firing delays
			ent->client->burst = 0;
			ent->client->fast_reload = 0;
			break;
		}
		case DUAL_NUM:
		{
			if (ent->client->dual_rds <= 0 && ent->client->ps.gunframe == 3)
				gi.sound(ent, CHAN_WEAPON, level.snd_noammo, 1, ATTN_NORM, 0);
			if (ent->client->dual_rds <= 0 && ent->client->ps.gunframe == 4)
			{
				gi.sound(ent, CHAN_WEAPON, level.snd_noammo, 1, ATTN_NORM, 0);
				ent->client->ps.gunframe = 68;
				ent->client->weaponstate = WEAPON_END_MAG;
				ent->client->resp.sniper_mode = 0;

				ent->client->desired_fov = 90;
				ent->client->ps.fov = 90;
				ent->client->fired = 0;	//reset any firing delays
				ent->client->burst = 0;

				return;
			}
			ent->client->fired = 0;	//reset any firing delays
			ent->client->burst = 0;
			break;
		}
		case GRAPPLE_NUM:
			break;

		default:
			gi.dprintf("Activated unknown weapon.\n");
			break;
		}

		ent->client->resp.sniper_mode = 0;
		// has to be here for dropping the sniper rifle, in the drop command didn't work...
		ent->client->desired_fov = 90;
		ent->client->ps.fov = 90;
		ent->client->ps.gunframe++;
		return;
	}

	if (ent->client->weaponstate == WEAPON_BUSY)
	{
		if (ent->client->bandaging == 1)
		{
			if (!(ent->client->idle_weapon))
			{
				Bandage(ent);
				return;
			}
			else
			{
				(ent->client->idle_weapon)--;
				return;
			}
		}

		// for after bandaging delay
		if (!(ent->client->idle_weapon) && ent->client->bandage_stopped)
		{
			ent->client->weaponstate = WEAPON_ACTIVATING;
			ent->client->ps.gunframe = 0;
			ent->client->bandage_stopped = 0;
			return;
		}
		else if (ent->client->bandage_stopped)
		{
			(ent->client->idle_weapon)--;
			return;
		}

		if (ent->client->curr_weap == SNIPER_NUM)
		{
			if (ent->client->desired_fov == 90)
			{
				ent->client->ps.fov = 90;
				ent->client->weaponstate = WEAPON_READY;
				ent->client->idle_weapon = 0;
			}
			if (!(ent->client->idle_weapon) && ent->client->desired_fov != 90)
			{
				ent->client->ps.fov = ent->client->desired_fov;
				ent->client->weaponstate = WEAPON_READY;
				ent->client->ps.gunindex = 0;
				return;
			}
			else
				(ent->client->idle_weapon)--;

		}
	}


	if ((ent->client->newweapon) && (ent->client->weaponstate != WEAPON_FIRING)
		&& (ent->client->weaponstate != WEAPON_BURSTING)
		&& (ent->client->bandage_stopped == 0))
	{
		ent->client->weaponstate = WEAPON_DROPPING;
		ent->client->ps.gunframe = FRAME_DEACTIVATE_FIRST;
		ent->client->desired_fov = 90;
		ent->client->ps.fov = 90;
		ent->client->resp.sniper_mode = 0;
		if (ent->client->weapon)
			ent->client->ps.gunindex = gi.modelindex(ent->client->weapon->view_model);

		// zucc more vwep stuff
		if ((FRAME_DEACTIVATE_LAST - FRAME_DEACTIVATE_FIRST) < 4)
		{
			if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
				SetAnimation( ent, FRAME_crpain4 + 1, FRAME_crpain1, ANIM_REVERSE );
			else
				SetAnimation( ent, FRAME_pain304 + 1, FRAME_pain301, ANIM_REVERSE );
		}
		return;
	}

	// bandaging case
	if ((ent->client->bandaging)
		&& (ent->client->weaponstate != WEAPON_FIRING)
		&& (ent->client->weaponstate != WEAPON_BURSTING)
		&& (ent->client->weaponstate != WEAPON_BUSY)
		&& (ent->client->weaponstate != WEAPON_BANDAGING))
	{
		ent->client->weaponstate = WEAPON_BANDAGING;
		ent->client->ps.gunframe = FRAME_DEACTIVATE_FIRST;
		return;
	}

	if (ent->client->weaponstate == WEAPON_READY)
	{
		if (((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK)
			&& (ent->solid != SOLID_NOT || ent->deadflag == DEAD_DEAD)
			&& !lights_camera_action && (!ent->client->uvTime || dm_shield->value > 1))
		{
			if (ent->client->uvTime) {
				ent->client->uvTime = 0;
				gi.centerprintf(ent, "Shields are DOWN!");
			}
			ent->client->latched_buttons &= ~BUTTON_ATTACK;
			switch (ent->client->curr_weap)
			{
			case MK23_NUM:
			{
				//      gi.cprintf (ent, PRINT_HIGH, "Calling ammo check %d\n", ent->client->mk23_rds);
				if (ent->client->mk23_rds > 0)
				{
					//              gi.cprintf(ent, PRINT_HIGH, "Entered fire selection\n");
					if (ent->client->pers.mk23_mode != 0
						&& ent->client->fired == 0)
					{
						ent->client->fired = 1;
						bFire = 1;
					}
					else if (ent->client->pers.mk23_mode == 0)
					{
						bFire = 1;
					}
				}
				else
					bOut = 1;
				break;

			}
			case MP5_NUM:
			{
				if (ent->client->mp5_rds > 0)
				{
					if (ent->client->pers.mp5_mode != 0
						&& ent->client->fired == 0 && ent->client->burst == 0)
					{
						ent->client->fired = 1;
						ent->client->ps.gunframe = 71;
						ent->client->burst = 1;
						ent->client->weaponstate = WEAPON_BURSTING;

						// start the animation
						if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
							SetAnimation( ent, FRAME_crattak1 - 1, FRAME_crattak9, ANIM_ATTACK );
						else
							SetAnimation( ent, FRAME_attack1 - 1, FRAME_attack8, ANIM_ATTACK );
					}
					else if (ent->client->pers.mp5_mode == 0
						&& ent->client->fired == 0)
					{
						bFire = 1;
					}
				}
				else
					bOut = 1;
				break;
			}
			case M4_NUM:
			{
				if (ent->client->m4_rds > 0)
				{
					if (ent->client->pers.m4_mode != 0
						&& ent->client->fired == 0 && ent->client->burst == 0)
					{
						ent->client->fired = 1;
						ent->client->ps.gunframe = 65;
						ent->client->burst = 1;
						ent->client->weaponstate = WEAPON_BURSTING;

						// start the animation
						if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
							SetAnimation( ent, FRAME_crattak1 - 1, FRAME_crattak9, ANIM_ATTACK );
						else
							SetAnimation( ent, FRAME_attack1 - 1, FRAME_attack8, ANIM_ATTACK );
					}
					else if (ent->client->pers.m4_mode == 0
						&& ent->client->fired == 0)
					{
						bFire = 1;
					}
				}
				else
					bOut = 1;
				break;
			}
			case M3_NUM:
			{
				if (ent->client->shot_rds > 0)
				{
					bFire = 1;
				}
				else
					bOut = 1;
				break;
			}
			// AQ2:TNG Deathwatch - Single Barreled HC
			// DW: SingleBarrel HC
			case HC_NUM:

				//if client is set to single shot mode, then allow
				//fire if only have 1 round left
			{
				if (ent->client->pers.hc_mode)
				{
					if (ent->client->cannon_rds > 0)
					{
						bFire = 1;
					}
					else
						bOut = 1;
				}
				else
				{
					if (ent->client->cannon_rds == 2)
					{
						bFire = 1;
					}
					else
						bOut = 1;
				}
				break;
			}
			/*
									case HC_NUM:
											{
													if ( ent->client->cannon_rds == 2 )
													{
															bFire = 1;
													}
													else
															bOut = 1;
													break;
											}
			*/
			// AQ2:TNG END
			case SNIPER_NUM:
			{
				if (ent->client->ps.fov != ent->client->desired_fov)
					ent->client->ps.fov = ent->client->desired_fov;
				// if they aren't at 90 then they must be zoomed, so remove their weapon from view
				if (ent->client->ps.fov != 90)
				{
					ent->client->ps.gunindex = 0;
					ent->client->no_sniper_display = 0;
				}

				if (ent->client->sniper_rds > 0)
				{
					bFire = 1;
				}
				else
					bOut = 1;
				break;
			}
			case DUAL_NUM:
			{
				if (ent->client->dual_rds > 0)
				{
					bFire = 1;
				}
				else
					bOut = 1;
				break;
			}
			case GRAPPLE_NUM:
				bFire = 1;
				bOut = 0;
				break;

			default:
			{
				gi.cprintf(ent, PRINT_HIGH,
					"Calling non specific ammo code\n");
				if ((!ent->client->ammo_index)
					|| (ent->client->inventory[ent->client->ammo_index] >=
						ent->client->weapon->quantity))
				{
					bFire = 1;
				}
				else
				{
					bFire = 0;
					bOut = 1;
				}
			}

			}
			if (bFire)
			{
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING;

				// start the animation
				if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
					SetAnimation( ent, FRAME_crattak1 - 1, FRAME_crattak9, ANIM_ATTACK );
				else
					SetAnimation( ent, FRAME_attack1 - 1, FRAME_attack8, ANIM_ATTACK );
			}
			else if (bOut)	// out of ammo
			{
				if (level.framenum >= ent->pain_debounce_framenum)
				{
					gi.sound(ent, CHAN_VOICE, level.snd_noammo, 1, ATTN_NORM, 0);
					ent->pain_debounce_framenum = level.framenum + 1 * HZ;
				}
			}
		}
		else
		{

			if (ent->client->ps.fov != ent->client->desired_fov)
				ent->client->ps.fov = ent->client->desired_fov;
			// if they aren't at 90 then they must be zoomed, so remove their weapon from view
			if (ent->client->ps.fov != 90)
			{
				ent->client->ps.gunindex = 0;
				ent->client->no_sniper_display = 0;
			}

			if (ent->client->ps.gunframe == FRAME_IDLE_LAST)
			{
				ent->client->ps.gunframe = FRAME_IDLE_FIRST;
				return;
			}

			/*       if (pause_frames)
			   {
			   for (n = 0; pause_frames[n]; n++)
			   {
			   if (ent->client->ps.gunframe == pause_frames[n])
			   {
			   if (rand()&15)
			   return;
			   }
			   }
			   }
			 */
			ent->client->ps.gunframe++;
			ent->client->fired = 0;	// weapon ready and button not down, now they can fire again
			ent->client->burst = 0;
			ent->client->machinegun_shots = 0;
			return;
		}
	}

	if (ent->client->weaponstate == WEAPON_FIRING)
	{
		for (n = 0; fire_frames[n]; n++)
		{
			if (ent->client->ps.gunframe == fire_frames[n])
			{
				if (ent->client->quad_framenum > level.framenum)
					gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"),
						1, ATTN_NORM, 0);

				fire(ent);
				break;
			}
		}

		if (!fire_frames[n])
			ent->client->ps.gunframe++;

		if (ent->client->ps.gunframe == FRAME_IDLE_FIRST + 1)
			ent->client->weaponstate = WEAPON_READY;
	}
	// player switched into 
	if (ent->client->weaponstate == WEAPON_BURSTING)
	{
		for (n = 0; fire_frames[n]; n++)
		{
			if (ent->client->ps.gunframe == fire_frames[n])
			{
				if (ent->client->quad_framenum > level.framenum)
					gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"),
						1, ATTN_NORM, 0);
				{
					//                      gi.cprintf (ent, PRINT_HIGH, "Calling fire code, frame = %d.\n", ent->client->ps.gunframe);
					fire(ent);
				}
				break;
			}
		}

		if (!fire_frames[n])
			ent->client->ps.gunframe++;

		//gi.cprintf (ent, PRINT_HIGH, "Calling Q_stricmp, frame = %d.\n", ent->client->ps.gunframe);


		if (ent->client->ps.gunframe == FRAME_IDLE_FIRST + 1)
			ent->client->weaponstate = WEAPON_READY;

		if (ent->client->curr_weap == MP5_NUM)
		{
			if (ent->client->ps.gunframe >= 76)
			{
				ent->client->weaponstate = WEAPON_READY;
				ent->client->ps.gunframe = FRAME_IDLE_FIRST + 1;
			}
			//      gi.cprintf (ent, PRINT_HIGH, "Succes Q_stricmp now: frame = %d.\n", ent->client->ps.gunframe);

		}
		if (ent->client->curr_weap == M4_NUM)
		{
			if (ent->client->ps.gunframe >= 69)
			{
				ent->client->weaponstate = WEAPON_READY;
				ent->client->ps.gunframe = FRAME_IDLE_FIRST + 1;
			}
			//      gi.cprintf (ent, PRINT_HIGH, "Succes Q_stricmp now: frame = %d.\n", ent->client->ps.gunframe);

		}

	}
}




/*
======================================================================

GRENADE

======================================================================
*/

#define GRENADE_TIMER           (3 * HZ)
#define GRENADE_MINSPEED        400
#define GRENADE_MAXSPEED        800

void weapon_grenade_fire(edict_t* ent, qboolean held)
{
	vec3_t offset;
	vec3_t forward, right;
	vec3_t start;
	int damage = 125;
	int timer;
	int speed;
	int damrad = GRENADE_DAMRAD;

	if (is_quad)
		damage *= 4;

	VectorSet(offset, 8, 8, ent->viewheight - 8);
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	timer = ent->client->grenade_framenum - level.framenum;
	speed = GRENADE_MINSPEED + (GRENADE_TIMER - timer) * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER);

	// Reset Grenade Damage to 1.52 when requested:
	damrad = use_classic->value ? GRENADE_DAMRAD_CLASSIC : GRENADE_DAMRAD;
	fire_grenade2(ent, start, forward, damrad, speed, timer, damrad * 2, held);

	if (!DMFLAGS(DF_INFINITE_AMMO))
		ent->client->inventory[ent->client->ammo_index]--;

	ent->client->grenade_framenum = level.framenum + 1 * HZ;
}

#define MK23_SPREAD		140
#define MP5_SPREAD		250 // DW: Changed this back to original from Edition's 240
#define M4_SPREAD			300
#define SNIPER_SPREAD 425
#define DUAL_SPREAD   300 // DW: Changed this back to original from Edition's 275

int AdjustSpread(edict_t* ent, int spread)
{
	int running = 225;		// minimum speed for running
	int walking = 10;		// minimum speed for walking
	int laser = 0;
	float factor[] = { .7f, 1, 2, 6 };
	int stage = 0;

	// 225 is running
	// < 10 will be standing
	float xyspeed = (ent->velocity[0] * ent->velocity[0] +
		ent->velocity[1] * ent->velocity[1]);

	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)	// crouching
		return (spread * .65);

	if (INV_AMMO(ent, LASER_NUM) && (ent->client->curr_weap == MK23_NUM
		|| ent->client->curr_weap == MP5_NUM || ent->client->curr_weap == M4_NUM))
		laser = 1;


	// running
	if (xyspeed > running* running)
		stage = 3;
	// walking
	else if (xyspeed >= walking * walking)
		stage = 2;
	// standing
	else
		stage = 1;

	// laser advantage
	if (laser)
	{
		if (stage == 1)
			stage = 0;
		else
			stage = 1;
	}

	return (int)(spread * factor[stage]);
}



//======================================================================


void MuzzleFlash(edict_t* ent, int mz)
{
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(mz);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);
}


void PlayWeaponSound(edict_t* ent)
{
	if (!ent->client->weapon_sound)
		return;

	// Synchronize weapon sounds so any framerate sounds like 10fps.
	if ((sync_guns->value == 2) && !FRAMESYNC)
		return;
	if (sync_guns->value
		&& (level.framenum > level.weapon_sound_framenum)
		&& (level.framenum < level.weapon_sound_framenum + game.framediv))
		return;


	// Because MZ_BLASTER is 0, use this stupid workaround.
	if ((ent->client->weapon_sound & ~MZ_SILENCED) == MZ_BLASTER2)
		ent->client->weapon_sound &= ~MZ_BLASTER2;


	if (ent->client->weapon_sound & MZ_SILENCED)
		// Silencer suppresses both sound and muzzle flash.
		gi.sound(ent, CHAN_WEAPON, level.snd_silencer, 1, ATTN_NORM, 0);

	else if (llsound->value)
		MuzzleFlash(ent, ent->client->weapon_sound);

	else switch (ent->client->weapon_sound)
	{
	case MZ_BLASTER:
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mk23fire.wav"), 1, ATTN_LOUD, 0);
		MuzzleFlash(ent, MZ_MACHINEGUN);
		break;
	case MZ_MACHINEGUN:
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mp5fire.wav"), 1, ATTN_LOUD, 0);
		MuzzleFlash(ent, MZ_MACHINEGUN);
		break;
	case MZ_ROCKET:
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/m4a1fire.wav"), 1, ATTN_LOUD, 0);
		MuzzleFlash(ent, MZ_MACHINEGUN);
		break;
	case MZ_SHOTGUN:
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/shotgf1b.wav"), 1, ATTN_LOUD, 0);
		MuzzleFlash(ent, MZ_SHOTGUN);
		break;
	case MZ_SSHOTGUN:
		if (!ent->client->pers.hc_mode)
			// Both barrels: sound on both WEAPON and ITEM to produce a louder boom.
			gi.sound(ent, CHAN_ITEM, gi.soundindex("weapons/cannon_fire.wav"), 1, ATTN_NORM, 0);
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/cannon_fire.wav"), 1, ATTN_LOUD, 0);
		MuzzleFlash(ent, MZ_SSHOTGUN);
		break;
	case MZ_HYPERBLASTER:
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/ssgfire.wav"), 1, ATTN_LOUD, 0);
		MuzzleFlash(ent, MZ_MACHINEGUN);
		break;
	default:
		MuzzleFlash(ent, ent->client->weapon_sound);
	}

	ent->client->weapon_sound = 0;
	level.weapon_sound_framenum = level.framenum;
}


//======================================================================
// mk23 derived from tutorial by GreyBear

void Pistol_Fire(edict_t* ent)
{
	int i;
	vec3_t start;
	vec3_t forward, right;
	vec3_t angles;
	int damage = 90;
	int kick = 150;
	vec3_t offset;
	int spread = MK23_SPREAD;
	int height;


	if (ent->client->pers.firing_style == ACTION_FIRING_CLASSIC)
		height = 8;
	else
		height = 0;

	//If the user isn't pressing the attack button, advance the frame and go away....
	if (!(ent->client->buttons & BUTTON_ATTACK))
	{
		ent->client->ps.gunframe++;
		return;
	}
	ent->client->ps.gunframe++;

	//Oops! Out of ammo!
	if (ent->client->mk23_rds < 1)
	{
		ent->client->ps.gunframe = 13;
		if (level.framenum >= ent->pain_debounce_framenum)
		{
			gi.sound(ent, CHAN_VOICE, level.snd_noammo, 1, ATTN_NORM, 0);
			ent->pain_debounce_framenum = level.framenum + 1 * HZ;
		}
		return;
	}


	//Calculate the kick angles
	for (i = 1; i < 3; i++)
	{
		ent->client->kick_origin[i] = crandom() * 0.35;
		ent->client->kick_angles[i] = crandom() * 0.7;
	}
	ent->client->kick_origin[0] = crandom() * 0.35;
	ent->client->kick_angles[0] = ent->client->machinegun_shots * -1.5;

	// get start / end positions
	VectorAdd(ent->client->v_angle, ent->client->kick_angles, angles);
	AngleVectors(angles, forward, right, NULL);
	VectorSet(offset, 0, 8, ent->viewheight - height);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	if (!sv_shelloff->value)
	{
		vec3_t result;
		Old_ProjectSource(ent->client, ent->s.origin, offset, forward, right, result);
		EjectShell(ent, result, 0);
	}

	spread = AdjustSpread(ent, spread);

	if (ent->client->pers.mk23_mode)
		spread *= .7;

	//      gi.cprintf(ent, PRINT_HIGH, "Spread is %d\n", spread);

	if (ent->client->mk23_rds == 1)
	{
		//Hard coded for reload only.
		ent->client->ps.gunframe = 62;
		ent->client->weaponstate = WEAPON_END_MAG;
	}

	fire_bullet(ent, start, forward, damage, kick, spread, spread, MOD_MK23);

	Stats_AddShot(ent, MOD_MK23);

	ent->client->mk23_rds--;
	ent->client->dual_rds--;


	ent->client->weapon_sound = MZ_BLASTER2;  // Becomes MZ_BLASTER.
	if (INV_AMMO(ent, SIL_NUM))
		ent->client->weapon_sound |= MZ_SILENCED;
	PlayWeaponSound(ent);
}

void Weapon_MK23(edict_t* ent)
{
	//Idle animation entry points - These make the fidgeting look more random
	static int pause_frames[] = { 13, 22, 40 };
	//The frames at which the weapon will fire
	static int fire_frames[] = { 10, 0 };

	//The call is made...
	Weapon_Generic(ent, 9, 12, 37, 40, 61, 65, pause_frames, fire_frames, Pistol_Fire);
}


void MP5_Fire(edict_t* ent)
{
	int i;
	vec3_t start;
	vec3_t forward, right;
	vec3_t angles;
	int damage = 55;
	int kick = 90;
	vec3_t offset;
	int spread = MP5_SPREAD;
	int height;

	if (ent->client->pers.firing_style == ACTION_FIRING_CLASSIC)
		height = 8;
	else
		height = 0;

	//If the user isn't pressing the attack button, advance the frame and go away....
	if (!(ent->client->buttons & BUTTON_ATTACK) && !(ent->client->burst))
	{
		ent->client->ps.gunframe++;
		return;
	}

	if (ent->client->burst == 0 && !(ent->client->pers.mp5_mode))
	{
		if (ent->client->ps.gunframe == 12)
			ent->client->ps.gunframe = 11;
		else
			ent->client->ps.gunframe = 12;
	}
	//burst mode
	else if (ent->client->burst == 0 && ent->client->pers.mp5_mode)
	{
		ent->client->ps.gunframe = 72;
		ent->client->weaponstate = WEAPON_BURSTING;
		ent->client->burst = 1;
		ent->client->fired = 1;
	}
	else if (ent->client->ps.gunframe >= 70 && ent->client->ps.gunframe <= 75)
	{
		ent->client->ps.gunframe++;
	}


	//Oops! Out of ammo!
	if (ent->client->mp5_rds < 1)
	{
		ent->client->ps.gunframe = 13;
		if (level.framenum >= ent->pain_debounce_framenum)
		{
			gi.sound(ent, CHAN_VOICE, level.snd_noammo, 1, ATTN_NORM, 0);
			ent->pain_debounce_framenum = level.framenum + 1 * HZ;
		}
		return;
	}


	spread = AdjustSpread(ent, spread);
	if (ent->client->burst)
		spread *= .7;


	//Calculate the kick angles
	for (i = 1; i < 3; i++)
	{
		ent->client->kick_origin[i] = crandom() * 0.25;
		ent->client->kick_angles[i] = crandom() * 0.5;
	}
	ent->client->kick_origin[0] = crandom() * 0.35;
	ent->client->kick_angles[0] = ent->client->machinegun_shots * -1.5;

	// get start / end positions
	VectorAdd(ent->client->v_angle, ent->client->kick_angles, angles);
	AngleVectors(angles, forward, right, NULL);
	VectorSet(offset, 0, 8, ent->viewheight - height);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	fire_bullet(ent, start, forward, damage, kick, spread, spread, MOD_MP5);
	Stats_AddShot(ent, MOD_MP5);

	ent->client->mp5_rds--;

	if (!sv_shelloff->value)
	{
		vec3_t result;
		Old_ProjectSource(ent->client, ent->s.origin, offset, forward, right, result);
		EjectShell(ent, result, 0);
	}



	// zucc vwep
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		SetAnimation( ent, FRAME_crattak1 - (int)(random() + 0.25), FRAME_crattak9, ANIM_ATTACK );
	else
		SetAnimation( ent, FRAME_attack1 - (int)(random() + 0.25), FRAME_attack8, ANIM_ATTACK );
	// zucc vwep done


	ent->client->weapon_sound = MZ_MACHINEGUN;
	if (INV_AMMO(ent, SIL_NUM))
		ent->client->weapon_sound |= MZ_SILENCED;
	PlayWeaponSound(ent);
}

void Weapon_MP5(edict_t* ent)
{
	//Idle animation entry points - These make the fidgeting look more random
	static int pause_frames[] = { 13, 30, 47 };
	//The frames at which the weapon will fire
	static int fire_frames[] = { 11, 12, 71, 72, 73, 0 };

	//The call is made...
	Weapon_Generic(ent, 10, 12, 47, 51, 69, 77, pause_frames, fire_frames, MP5_Fire);
}

void M4_Fire(edict_t* ent)
{
	int i;
	vec3_t start;
	vec3_t forward, right;
	vec3_t angles;
	int damage = 90;
	int kick = 90;
	vec3_t offset;
	int spread = M4_SPREAD;
	int height;

	if (ent->client->pers.firing_style == ACTION_FIRING_CLASSIC)
		height = 8;
	else
		height = 0;

	//If the user isn't pressing the attack button, advance the frame and go away....
	if (!(ent->client->buttons & BUTTON_ATTACK) && !(ent->client->burst))
	{
		ent->client->ps.gunframe++;
		ent->client->machinegun_shots = 0;
		return;
	}

	if (ent->client->burst == 0 && !(ent->client->pers.m4_mode))
	{
		if (ent->client->ps.gunframe == 12)
			ent->client->ps.gunframe = 11;
		else
			ent->client->ps.gunframe = 12;
	}
	//burst mode
	else if (ent->client->burst == 0 && ent->client->pers.m4_mode)
	{
		ent->client->ps.gunframe = 66;
		ent->client->weaponstate = WEAPON_BURSTING;
		ent->client->burst = 1;
		ent->client->fired = 1;
	}
	else if (ent->client->ps.gunframe >= 64 && ent->client->ps.gunframe <= 69)
	{
		ent->client->ps.gunframe++;
	}


	//Oops! Out of ammo!
	if (ent->client->m4_rds < 1)
	{
		ent->client->ps.gunframe = 13;
		if (level.framenum >= ent->pain_debounce_framenum)
		{
			gi.sound(ent, CHAN_VOICE, level.snd_noammo, 1, ATTN_NORM, 0);
			ent->pain_debounce_framenum = level.framenum + 1 * HZ;
		}
		return;
	}

	// causes the ride up
	if (ent->client->weaponstate != WEAPON_BURSTING)
	{
		ent->client->machinegun_shots++;
		if (ent->client->machinegun_shots > 23)
			ent->client->machinegun_shots = 23;
	}
	else				// no kick when in burst mode
	{
		ent->client->machinegun_shots = 0;
	}


	spread = AdjustSpread(ent, spread);
	if (ent->client->burst)
		spread *= .7;


	//      gi.cprintf(ent, PRINT_HIGH, "Spread is %d\n", spread);


	//Calculate the kick angles
	for (i = 1; i < 3; i++)
	{
		ent->client->kick_origin[i] = crandom() * 0.25;
		ent->client->kick_angles[i] = crandom() * 0.5;
	}
	ent->client->kick_origin[0] = crandom() * 0.35;
	ent->client->kick_angles[0] = ent->client->machinegun_shots * -.7;

	// get start / end positions
	VectorAdd(ent->client->v_angle, ent->client->kick_angles, angles);
	AngleVectors(angles, forward, right, NULL);
	VectorSet(offset, 0, 8, ent->viewheight - height);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	if (is_quad)
		damage *= 1.5f;

	fire_bullet_sparks(ent, start, forward, damage, kick, spread, spread, MOD_M4);
	Stats_AddShot(ent, MOD_M4);

	ent->client->m4_rds--;

	if (!sv_shelloff->value)
	{
		vec3_t result;
		Old_ProjectSource(ent->client, ent->s.origin, offset, forward, right, result);
		EjectShell(ent, result, 0);
	}


	// zucc vwep
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		SetAnimation( ent, FRAME_crattak1 - (int)(random() + 0.25), FRAME_crattak9, ANIM_ATTACK );
	else
		SetAnimation( ent, FRAME_attack1 - (int)(random() + 0.25), FRAME_attack8, ANIM_ATTACK );
	// zucc vwep done


	ent->client->weapon_sound = MZ_ROCKET;
	PlayWeaponSound(ent);
}

void Weapon_M4(edict_t* ent)
{
	//Idle animation entry points - These make the fidgeting look more random
	static int pause_frames[] = { 13, 24, 39 };
	//The frames at which the weapon will fire
	static int fire_frames[] = { 11, 12, 65, 66, 67, 0 };

	//The call is made...
	Weapon_Generic(ent, 10, 12, 39, 44, 63, 71, pause_frames, fire_frames, M4_Fire);
}

void M3_Fire(edict_t* ent)
{
	vec3_t start;
	vec3_t forward, right;
	vec3_t offset;
	int damage = 17;		//actionquake is 15 standard
	int kick = 20;
	int height;

	if (ent->client->pers.firing_style == ACTION_FIRING_CLASSIC)
		height = 8;
	else
		height = 0;

	if (ent->client->ps.gunframe == 9)
	{
		ent->client->ps.gunframe++;
		return;
	}


	AngleVectors(ent->client->v_angle, forward, right, NULL);

	VectorScale(forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -2;

	VectorSet(offset, 0, 8, ent->viewheight - height);



	if (ent->client->ps.gunframe == 14)
	{
		if (!sv_shelloff->value)
		{
			vec3_t result;
			Old_ProjectSource(ent->client, ent->s.origin, offset, forward,
				right, result);
			EjectShell(ent, result, 0);
		}
		ent->client->ps.gunframe++;
		return;
	}


	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	if (is_quad)
	{
		damage *= 1.5f;
		kick *= 1.5f;
	}

	setFFState(ent);
	InitTookDamage();	//FB 6/3/99

	fire_shotgun(ent, start, forward, damage, kick, 800, 800,
		12 /*DEFAULT_DEATHMATCH_SHOTGUN_COUNT */, MOD_M3);

	Stats_AddShot(ent, MOD_M3);

	ProduceShotgunDamageReport(ent);	//FB 6/3/99

	ent->client->ps.gunframe++;

	//if (!DMFLAGS(DF_INFINITE_AMMO))
	//      ent->client->inventory[ent->client->ammo_index]--;
	ent->client->shot_rds--;


	ent->client->weapon_sound = MZ_SHOTGUN;
	PlayWeaponSound(ent);
}

void Weapon_M3(edict_t* ent)
{
	static int pause_frames[] = { 22, 28, 0 };
	static int fire_frames[] = { 8, 9, 14, 0 };

	Weapon_Generic(ent, 7, 15, 35, 41, 52, 60, pause_frames, fire_frames, M3_Fire);
}

// AQ2:TNG Deathwatch - Modified to use Single Barreled HC Mode
void HC_Fire(edict_t* ent)
{
	vec3_t start;
	vec3_t forward, right;
	vec3_t offset;
	vec3_t v;
	int sngl_damage = 15;
	int sngl_kick = 30;
	int damage = 20;
	int kick = 40;
	int height;

	if (ent->client->pers.firing_style == ACTION_FIRING_CLASSIC)
		height = 8;
	else
		height = 0;

	AngleVectors(ent->client->v_angle, forward, right, NULL);

	VectorScale(forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -2;

	VectorSet(offset, 0, 8, ent->viewheight - height);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	if (is_quad)
	{
		damage *= 1.5f;
		kick *= 1.5f;
	}

	v[PITCH] = ent->client->v_angle[PITCH];
	v[ROLL] = ent->client->v_angle[ROLL];

	// default hspread is 1k and default vspread is 500
	setFFState(ent);
	InitTookDamage();	//FB 6/3/99

	if (ent->client->pers.hc_mode)
	{
		// Single barrel.

		if (ent->client->cannon_rds == 2)
			v[YAW] = ent->client->v_angle[YAW] - ((0.5) / 2);
		else
			v[YAW] = ent->client->v_angle[YAW] + ((0.5) / 2);
		AngleVectors(v, forward, NULL, NULL);

		//half the spread, half the pellets?
		fire_shotgun(ent, start, forward, sngl_damage, sngl_kick, DEFAULT_SHOTGUN_HSPREAD * 2.5, DEFAULT_SHOTGUN_VSPREAD * 2.5, 34 / 2, MOD_HC);

		ent->client->cannon_rds--;
	}
	else
	{
		// Both barrels.

		v[YAW] = ent->client->v_angle[YAW] - 5;
		AngleVectors(v, forward, NULL, NULL);
		fire_shotgun(ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD * 4, DEFAULT_SHOTGUN_VSPREAD * 4, 34 / 2, MOD_HC);

		v[YAW] = ent->client->v_angle[YAW] + 5;
		AngleVectors(v, forward, NULL, NULL);
		fire_shotgun(ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD * 4, DEFAULT_SHOTGUN_VSPREAD * 4 /* was *5 here */, 34 / 2, MOD_HC);

		ent->client->cannon_rds -= 2;
	}

	Stats_AddShot(ent, MOD_HC);
	ProduceShotgunDamageReport(ent);	//FB 6/3/99

	ent->client->ps.gunframe++;


	ent->client->weapon_sound = MZ_SSHOTGUN;
	PlayWeaponSound(ent);


	//      if (!DMFLAGS(DF_INFINITE_AMMO))
	//              ent->client->inventory[ent->client->ammo_index] -= 2;
}

/*
		v[YAW]   = ent->client->v_angle[YAW] - 5;
		v[ROLL]  = ent->client->v_angle[ROLL];
		AngleVectors (v, forward, NULL, NULL);
		// default hspread is 1k and default vspread is 500
	setFFState(ent);
		InitTookDamage();  //FB 6/3/99
		fire_shotgun (ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD*4, DEFAULT_SHOTGUN_VSPREAD*4, 34/2, MOD_HC);
		v[YAW]   = ent->client->v_angle[YAW] + 5;
		AngleVectors (v, forward, NULL, NULL);
		fire_shotgun (ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD*4, DEFAULT_SHOTGUN_VSPREAD*5, 34/2, MOD_HC);

	if (llsound->value == 0)
	  {
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/cannon_fire.wav"), 1, ATTN_LOUD, 0);
	  }
		// send muzzle flash
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte (MZ_SSHOTGUN | is_silenced);
		gi.multicast (ent->s.origin, MULTICAST_PVS);
		ProduceShotgunDamageReport(ent);  //FB 6/3/99

		ent->client->ps.gunframe++;
		PlayerNoise(ent, start, PNOISE_WEAPON);

	//      if (!DMFLAGS(DF_INFINITE_AMMO))
	//              ent->client->inventory[ent->client->ammo_index] -= 2;

		ent->client->cannon_rds -= 2;
}
*/
// AQ2:TNG END

void Weapon_HC(edict_t* ent)
{
	static int pause_frames[] = { 29, 42, 57, 0 };
	static int fire_frames[] = { 7, 0 };

	Weapon_Generic(ent, 6, 17, 57, 61, 82, 83, pause_frames, fire_frames, HC_Fire);
}

void Sniper_Fire(edict_t* ent)
{
	//int i;  // FIXME: Should this be used somewhere?
	vec3_t start;
	vec3_t forward, right;
	//vec3_t angles;  // FIXME: This was set below, but never used.
	int damage = 250;
	int kick = 200;
	vec3_t offset;
	int spread = SNIPER_SPREAD;


	if (ent->client->ps.gunframe == 13)
	{
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/ssgbolt.wav"), 1, ATTN_NORM, 0);
		ent->client->ps.gunframe++;

		AngleVectors(ent->client->v_angle, forward, right, NULL);
		VectorSet(offset, 0, 0, ent->viewheight - 0);

		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

		EjectShell(ent, start, 0);
		return;
	}

	if (ent->client->ps.gunframe == 21)
	{
		if (ent->client->ps.fov != ent->client->desired_fov)
			ent->client->ps.fov = ent->client->desired_fov;
		// if they aren't at 90 then they must be zoomed, so remove their weapon from view
		if (ent->client->ps.fov != 90)
		{
			ent->client->ps.gunindex = 0;
			ent->client->no_sniper_display = 0;
		}
		ent->client->ps.gunframe++;

		return;
	}


	//If the user isn't pressing the attack button, advance the frame and go away....
	if (!(ent->client->buttons & BUTTON_ATTACK))
	{
		ent->client->ps.gunframe++;
		return;
	}
	ent->client->ps.gunframe++;

	//Oops! Out of ammo!
	if (ent->client->sniper_rds < 1)
	{
		ent->client->ps.gunframe = 22;
		if (level.framenum >= ent->pain_debounce_framenum)
		{
			gi.sound(ent, CHAN_VOICE, level.snd_noammo, 1, ATTN_NORM, 0);
			ent->pain_debounce_framenum = level.framenum + 1 * HZ;
		}
		return;
	}

	spread = AdjustSpread(ent, spread);
	if (ent->client->resp.sniper_mode == SNIPER_2X)
		spread = 0;
	else if (ent->client->resp.sniper_mode == SNIPER_4X)
		spread = 0;
	else if (ent->client->resp.sniper_mode == SNIPER_6X)
		spread = 0;


	if (is_quad)
		damage *= 1.5f;

	//Calculate the kick angles
	/*for (i = 1; i < 3; i++)
	{
		ent->client->kick_origin[i] = crandom () * 0;
		ent->client->kick_angles[i] = crandom () * 0;
	}
	ent->client->kick_origin[0] = crandom () * 0;
	ent->client->kick_angles[0] = ent->client->machinegun_shots * 0;*/
	VectorClear(ent->client->kick_origin);
	VectorClear(ent->client->kick_angles);

	// get start / end positions
	//VectorAdd (ent->client->v_angle, ent->client->kick_angles, angles);  // FIXME: Should this be used?
	AngleVectors(ent->client->v_angle, forward, right, NULL);  // FIXME: Should this be angles instead of v_angle?
	VectorSet(offset, 0, 0, ent->viewheight - 0);

	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);


	//If no reload, fire normally.
	fire_bullet_sniper(ent, start, forward, damage, kick, spread, spread, MOD_SNIPER);
	Stats_AddShot(ent, MOD_SNIPER);

	ent->client->sniper_rds--;
	ent->client->ps.fov = 90;	// so we can watch the next round get chambered
	ent->client->ps.gunindex =
		gi.modelindex(ent->client->weapon->view_model);
	ent->client->no_sniper_display = 1;


	ent->client->weapon_sound = MZ_HYPERBLASTER;
	if (INV_AMMO(ent, SIL_NUM))
		ent->client->weapon_sound |= MZ_SILENCED;
	PlayWeaponSound(ent);
}

void Weapon_Sniper(edict_t* ent)
{
	//Idle animation entry points - These make the fidgeting look more random
	static int pause_frames[] = { 21, 40 };
	//The frames at which the weapon will fire
	static int fire_frames[] = { 9, 13, 21, 0 };

	//The call is made...
	Weapon_Generic(ent, 8, 21, 41, 50, 81, 95, pause_frames, fire_frames, Sniper_Fire);
}

void Dual_Fire(edict_t* ent)
{
	int i;
	vec3_t start;
	vec3_t forward, right;
	vec3_t angles;
	int damage = 90;
	int kick = 90;
	vec3_t offset;
	int spread = DUAL_SPREAD;
	int height;

	if (ent->client->pers.firing_style == ACTION_FIRING_CLASSIC)
		height = 8;
	else
		height = 0;

	spread = AdjustSpread(ent, spread);

	if (ent->client->dual_rds < 1)
	{
		ent->client->ps.gunframe = 68;
		if (level.framenum >= ent->pain_debounce_framenum)
		{
			gi.sound(ent, CHAN_VOICE, level.snd_noammo, 1, ATTN_NORM, 0);
			ent->pain_debounce_framenum = level.framenum + 1 * HZ;
		}
		return;
	}

	if (is_quad)
		damage *= 1.5f;

	//If the user isn't pressing the attack button, advance the frame and go away....
	if (ent->client->ps.gunframe == 8)
	{
		//gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mk23fire.wav"), 1, ATTN_LOUD, 0);
		ent->client->ps.gunframe++;

		VectorAdd(ent->client->v_angle, ent->client->kick_angles, angles);
		AngleVectors(angles, forward, right, NULL);

		VectorSet(offset, 0, 8, ent->viewheight - height);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
		if (ent->client->dual_rds > 1)
		{

			fire_bullet(ent, start, forward, damage, kick, spread, spread, MOD_DUAL);
			Stats_AddShot(ent, MOD_DUAL);

			if (!sv_shelloff->value)
			{
				vec3_t result;
				Old_ProjectSource(ent->client, ent->s.origin, offset, forward, right, result);
				EjectShell(ent, result, 2);
			}

			if (ent->client->dual_rds > ent->client->mk23_max + 1)
			{
				ent->client->dual_rds -= 2;
			}
			else if (ent->client->dual_rds > ent->client->mk23_max)	// 13 rounds left
			{
				ent->client->dual_rds -= 2;
				ent->client->mk23_rds--;
			}
			else
			{
				ent->client->dual_rds -= 2;
				ent->client->mk23_rds -= 2;
			}

			if (ent->client->dual_rds == 0)
			{
				ent->client->ps.gunframe = 68;
				ent->client->weaponstate = WEAPON_END_MAG;
			}


			ent->client->weapon_sound = MZ_BLASTER2;  // Becomes MZ_BLASTER.
			PlayWeaponSound(ent);
		}
		else
		{
			ent->client->dual_rds = 0;
			ent->client->mk23_rds = 0;
			gi.sound(ent, CHAN_WEAPON, level.snd_noammo, 1, ATTN_NORM, 0);
			//ent->pain_debounce_time = level.time + 1;
			ent->client->ps.gunframe = 68;
			ent->client->weaponstate = WEAPON_END_MAG;

		}
		return;
	}

	if (ent->client->ps.gunframe == 9)
	{
		ent->client->ps.gunframe += 2;
		return;
	}


	/*if (!(ent->client->buttons & BUTTON_ATTACK))
	{
		ent->client->ps.gunframe++;
		return;
	} */
	ent->client->ps.gunframe++;

	//Oops! Out of ammo!
	if (ent->client->dual_rds < 1)
	{
		ent->client->ps.gunframe = 12;
		if (level.framenum >= ent->pain_debounce_framenum)
		{
			gi.sound(ent, CHAN_VOICE, level.snd_noammo, 1, ATTN_NORM, 0);
			ent->pain_debounce_framenum = level.framenum + 1 * HZ;
		}
		return;
	}


	//Calculate the kick angles
	for (i = 1; i < 3; i++)
	{
		ent->client->kick_origin[i] = crandom() * 0.25;
		ent->client->kick_angles[i] = crandom() * 0.5;
	}
	ent->client->kick_origin[0] = crandom() * 0.35;

	// get start / end positions
	VectorAdd(ent->client->v_angle, ent->client->kick_angles, angles);
	AngleVectors(angles, forward, right, NULL);
	// first set up for left firing
	VectorSet(offset, 0, -20, ent->viewheight - height);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	if (!sv_shelloff->value)
	{
		vec3_t result;
		Old_ProjectSource(ent->client, ent->s.origin, offset, forward, right, result);
		EjectShell(ent, result, 1);
	}



	//If no reload, fire normally.
	fire_bullet(ent, start, forward, damage, kick, spread, spread, MOD_DUAL);
	Stats_AddShot(ent, MOD_DUAL);


	ent->client->weapon_sound = MZ_BLASTER2;  // Becomes MZ_BLASTER.
	PlayWeaponSound(ent);
}

void Weapon_Dual(edict_t* ent)
{
	//Idle animation entry points - These make the fidgeting look more random
	static int pause_frames[] = { 13, 22, 32 };
	//The frames at which the weapon will fire
	static int fire_frames[] = { 7, 8, 9, 0 };

	//The call is made...
	Weapon_Generic(ent, 6, 10, 32, 40, 65, 68, pause_frames, fire_frames, Dual_Fire);
}



//zucc 

#define FRAME_PREPARETHROW_FIRST        (FRAME_DEACTIVATE_LAST +1)
#define FRAME_IDLE2_FIRST                       (FRAME_PREPARETHROW_LAST +1)
#define FRAME_THROW_FIRST                       (FRAME_IDLE2_LAST +1)
#define FRAME_STOPTHROW_FIRST           (FRAME_THROW_LAST +1)
#define FRAME_NEWKNIFE_FIRST            (FRAME_STOPTHROW_LAST +1)

void
Weapon_Generic_Knife(edict_t* ent, int FRAME_ACTIVATE_LAST,
	int FRAME_FIRE_LAST, int FRAME_IDLE_LAST,
	int FRAME_DEACTIVATE_LAST, int FRAME_PREPARETHROW_LAST,
	int FRAME_IDLE2_LAST, int FRAME_THROW_LAST,
	int FRAME_STOPTHROW_LAST, int FRAME_NEWKNIFE_LAST,
	int* pause_frames, int* fire_frames,
	int (*fire) (edict_t* ent))
{

	if (ent->s.modelindex != 255)	// zucc vwep
		return;			// not on client, so VWep animations could do wacky things

	//FIREBLADE
	if (ent->client->weaponstate == WEAPON_FIRING &&
		((ent->solid == SOLID_NOT && ent->deadflag != DEAD_DEAD)
			|| lights_camera_action))
	{
		ent->client->weaponstate = WEAPON_READY;
	}
	//FIREBLADE

	if (ent->client->weaponstate == WEAPON_RELOADING)
	{
		if (ent->client->ps.gunframe < FRAME_NEWKNIFE_LAST)
		{
			ent->client->ps.gunframe++;
		}
		else
		{
			ent->client->ps.gunframe = FRAME_IDLE2_FIRST;
			ent->client->weaponstate = WEAPON_READY;
		}
	}

	if (ent->client->weaponstate == WEAPON_DROPPING)
	{
		if (ent->client->pers.knife_mode == 1)
		{
			if (ent->client->ps.gunframe == FRAME_NEWKNIFE_FIRST)
			{
				ChangeWeapon(ent);
				return;
			}
			else
			{
				// zucc going to have to do this a bit different because
				// of the way I roll gunframes backwards for the thrownknife position
				if ((ent->client->ps.gunframe - FRAME_NEWKNIFE_FIRST) == 4)
				{
					if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
						SetAnimation( ent, FRAME_crpain4 + 1, FRAME_crpain1, ANIM_REVERSE );
					else
						SetAnimation( ent, FRAME_pain304 + 1, FRAME_pain301, ANIM_REVERSE );
				}
				ent->client->ps.gunframe--;
			}

		}
		else
		{
			if (ent->client->ps.gunframe == FRAME_DEACTIVATE_LAST)
			{
				ChangeWeapon(ent);
				return;
			}
			else if ((FRAME_DEACTIVATE_LAST - ent->client->ps.gunframe) == 4)
			{
				if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
					SetAnimation( ent, FRAME_crpain4 + 1, FRAME_crpain1, ANIM_REVERSE );
				else
					SetAnimation( ent, FRAME_pain304 + 1, FRAME_pain301, ANIM_REVERSE );
			}



			ent->client->ps.gunframe++;
		}
		return;
	}
	if (ent->client->weaponstate == WEAPON_ACTIVATING)
	{

		if (ent->client->pers.knife_mode == 1 && ent->client->ps.gunframe == 0)
		{
			//                      gi.cprintf(ent, PRINT_HIGH, "NewKnifeFirst\n");
			ent->client->ps.gunframe = FRAME_PREPARETHROW_FIRST;
			return;
		}
		if (ent->client->ps.gunframe == FRAME_ACTIVATE_LAST
			|| ent->client->ps.gunframe == FRAME_STOPTHROW_LAST)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = FRAME_IDLE_FIRST;
			return;
		}
		if (ent->client->ps.gunframe == FRAME_PREPARETHROW_LAST)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = FRAME_IDLE2_FIRST;
			return;
		}

		ent->client->resp.sniper_mode = 0;
		// has to be here for dropping the sniper rifle, in the drop command didn't work...
		ent->client->desired_fov = 90;
		ent->client->ps.fov = 90;

		ent->client->ps.gunframe++;
		//              gi.cprintf(ent, PRINT_HIGH, "After increment frames = %d\n", ent->client->ps.gunframe);
		return;
	}



	// bandaging case
	if ((ent->client->bandaging)
		&& (ent->client->weaponstate != WEAPON_FIRING)
		&& (ent->client->weaponstate != WEAPON_BURSTING)
		&& (ent->client->weaponstate != WEAPON_BUSY)
		&& (ent->client->weaponstate != WEAPON_BANDAGING))
	{
		ent->client->weaponstate = WEAPON_BANDAGING;
		ent->client->ps.gunframe = FRAME_DEACTIVATE_FIRST;
		return;
	}


	if (ent->client->weaponstate == WEAPON_BUSY)
	{

		if (ent->client->bandaging == 1)
		{
			if (!(ent->client->idle_weapon))
			{
				Bandage(ent);
				//ent->client->weaponstate = WEAPON_ACTIVATING;
				//                ent->client->ps.gunframe = 0;
			}
			else
				(ent->client->idle_weapon)--;
			return;
		}

		// for after bandaging delay
		if (!(ent->client->idle_weapon) && ent->client->bandage_stopped)
		{
			ent->client->weaponstate = WEAPON_ACTIVATING;
			ent->client->ps.gunframe = 0;
			ent->client->bandage_stopped = 0;
			return;
		}
		else if (ent->client->bandage_stopped == 1)
		{
			(ent->client->idle_weapon)--;
			return;
		}


		if (ent->client->ps.gunframe == 98)
		{
			ent->client->weaponstate = WEAPON_READY;
			return;
		}
		else
			ent->client->ps.gunframe++;
	}

	if (ent->client->weaponstate == WEAPON_BANDAGING)
	{
		if (ent->client->ps.gunframe == FRAME_DEACTIVATE_LAST)
		{
			ent->client->weaponstate = WEAPON_BUSY;
			ent->client->idle_weapon = BANDAGE_TIME;
			return;
		}
		ent->client->ps.gunframe++;
		return;
	}


	if ((ent->client->newweapon) && (ent->client->weaponstate != WEAPON_FIRING)
		&& (ent->client->weaponstate != WEAPON_BUSY))
	{

		if (ent->client->pers.knife_mode == 1)	// throwing mode
		{
			ent->client->ps.gunframe = FRAME_NEWKNIFE_LAST;
			// zucc more vwep stuff
			if ((FRAME_NEWKNIFE_LAST - FRAME_NEWKNIFE_FIRST) < 4)
			{
				if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
					SetAnimation( ent, FRAME_crpain4 + 1, FRAME_crpain1, ANIM_REVERSE );
				else
					SetAnimation( ent, FRAME_pain304 + 1, FRAME_pain301, ANIM_REVERSE );
			}
		}
		else			// not in throwing mode
		{
			ent->client->ps.gunframe = FRAME_DEACTIVATE_FIRST;
			// zucc more vwep stuff
			if ((FRAME_DEACTIVATE_LAST - FRAME_DEACTIVATE_FIRST) < 4)
			{
				if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
					SetAnimation( ent, FRAME_crpain4 + 1, FRAME_crpain1, ANIM_REVERSE );
				else
					SetAnimation( ent, FRAME_pain304 + 1, FRAME_pain301, ANIM_REVERSE );
			}
		}
		ent->client->weaponstate = WEAPON_DROPPING;
		return;
	}

	if (ent->client->weaponstate == WEAPON_READY)
	{
		if (((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK)
			&& (ent->solid != SOLID_NOT || ent->deadflag == DEAD_DEAD)
			&& !lights_camera_action && !ent->client->uvTime)
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;


			if (ent->client->pers.knife_mode == 1)
			{
				ent->client->ps.gunframe = FRAME_THROW_FIRST;
			}
			else
			{
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
			}
			ent->client->weaponstate = WEAPON_FIRING;

			// start the animation
			if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
				SetAnimation( ent, FRAME_crattak1 - 1, FRAME_attack8, ANIM_ATTACK );
			else
				SetAnimation( ent, FRAME_attack1 - 1, FRAME_attack8, ANIM_ATTACK );
			return;
		}
		if (ent->client->ps.gunframe == FRAME_IDLE_LAST)
		{
			ent->client->ps.gunframe = FRAME_IDLE_FIRST;
			return;
		}

		if (ent->client->ps.gunframe == FRAME_IDLE2_LAST)
		{
			ent->client->ps.gunframe = FRAME_IDLE2_FIRST;
			return;
		}
		/* // zucc this causes you to not be able to throw a knife while it is flipping
		if ( ent->client->ps.gunframe == 93 )
		{
			ent->client->weaponstate = WEAPON_BUSY;
			return;
		}
			*/
			//gi.cprintf(ent, PRINT_HIGH, "Before a gunframe additon frames = %d\n", ent->client->ps.gunframe);
		ent->client->ps.gunframe++;
		return;
	}
	if (ent->client->weaponstate == WEAPON_FIRING)
	{
		int n;
		for (n = 0; fire_frames[n]; n++)
		{
			if (ent->client->ps.gunframe == fire_frames[n])
			{
				if (ent->client->quad_framenum > level.framenum)
					gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"),
						1, ATTN_NORM, 0);

				if (fire(ent))
					break;
				else		// we ran out of knives and switched weapons
					return;
			}
		}

		if (!fire_frames[n])
			ent->client->ps.gunframe++;

		/*if ( ent->client->ps.gunframe == FRAME_STOPTHROW_FIRST + 1 )
		{
		ent->client->ps.gunframe = FRAME_NEWKNIFE_FIRST;
		ent->client->weaponstate = WEAPON_RELOADING;
		} */

		if (ent->client->ps.gunframe == FRAME_IDLE_FIRST + 1 ||
			ent->client->ps.gunframe == FRAME_IDLE2_FIRST + 1)
			ent->client->weaponstate = WEAPON_READY;
	}

}


int Knife_Fire(edict_t* ent)
{
	vec3_t start, v;
	vec3_t forward, right;

	vec3_t offset;
	int damage = 200;
	int throwndamage = 250;
	int kick = 50;		// doesn't throw them back much..
	int knife_return = 3;

	AngleVectors(ent->client->v_angle, forward, right, NULL);

	VectorScale(forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -2;

	VectorSet(offset, 0, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);



	v[PITCH] = ent->client->v_angle[PITCH];
	v[ROLL] = ent->client->v_angle[ROLL];

	// zucc updated these to not have offsets anymore for 1.51,
	// this should fix the complaints about the knife not
	// doing enough damage 

	if (ent->client->ps.gunframe == 6)
	{
		v[YAW] = ent->client->v_angle[YAW];	// + 5;
		AngleVectors(v, forward, NULL, NULL);
	}
	else if (ent->client->ps.gunframe == 7)
	{
		v[YAW] = ent->client->v_angle[YAW];	// + 2;
		AngleVectors(v, forward, NULL, NULL);
	}
	else if (ent->client->ps.gunframe == 8)
	{
		v[YAW] = ent->client->v_angle[YAW];
		AngleVectors(v, forward, NULL, NULL);
	}
	else if (ent->client->ps.gunframe == 9)
	{
		v[YAW] = ent->client->v_angle[YAW];	// - 2;
		AngleVectors(v, forward, NULL, NULL);
	}
	else if (ent->client->ps.gunframe == 10)
	{
		v[YAW] = ent->client->v_angle[YAW];	//-5;
		AngleVectors(v, forward, NULL, NULL);
	}


	if (ent->client->pers.knife_mode == 0)
	{
		if (is_quad)
			damage *= 1.5f;

		knife_return = knife_attack(ent, start, forward, damage, kick);
		Stats_AddShot(ent, MOD_KNIFE);

		if (knife_return < ent->client->knife_sound)
			ent->client->knife_sound = knife_return;

		if (ent->client->ps.gunframe == 8)	// last slicing frame, time for some sound
		{
			if (ent->client->knife_sound == -2)
			{
				// we hit a player
				gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/stab.wav"),
					1, ATTN_NORM, 0);
			}
			else if (ent->client->knife_sound == -1)
			{
				// we hit a wall

				gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/clank.wav"),
					1, ATTN_NORM, 0);
			}
			else			// we missed
			{
				gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/swish.wav"),
					1, ATTN_NORM, 0);
			}
			ent->client->knife_sound = 0;
		}
	}
	else
	{
		// do throwing stuff here

		damage = throwndamage;

		if (is_quad)
			damage *= 1.5f;
		// throwing sound
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/throw.wav"), 1, ATTN_NORM, 0);


		// below is exact code from action for how it sets the knife up
		AngleVectors(ent->client->v_angle, forward, right, NULL);
		VectorSet(offset, 24, 8, ent->viewheight - 8);
		VectorAdd(offset, vec3_origin, offset);
		forward[2] += .17f;

		// zucc using old style because the knife coming straight out looks
		// pretty stupid


		Knife_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

		INV_AMMO(ent, KNIFE_NUM)--;
		if (INV_AMMO(ent, KNIFE_NUM) <= 0)
		{
			ent->client->newweapon = GET_ITEM(MK23_NUM);
			ChangeWeapon(ent);
			// zucc was at 1250, dropping speed to 1200
			knife_throw(ent, start, forward, damage, 1200);
			Stats_AddShot(ent, MOD_KNIFE_THROWN);
			return 0;
		}
		else
		{
			ent->client->weaponstate = WEAPON_RELOADING;
			ent->client->ps.gunframe = 111;
		}



		/*AngleVectors (ent->client->v_angle, forward, right, NULL);

		VectorScale (forward, -2, ent->client->kick_origin);
		ent->client->kick_angles[0] = -1;

		VectorSet(offset, 8, 8, ent->viewheight-8);
		P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
		*/
		//      fire_rocket (ent, start, forward, damage, 650, 200, 200);

		knife_throw(ent, start, forward, damage, 1200);
		Stats_AddShot(ent, MOD_KNIFE_THROWN);

		// 
	}

	ent->client->ps.gunframe++;
	if (ent->client->ps.gunframe == 106)	// over the throwing frame limit
		ent->client->ps.gunframe = 64;	// the idle animation frame for throwing
	// not sure why the frames weren't ordered
	// with throwing first, unwise.
	PlayerNoise(ent, start, PNOISE_WEAPON);
	return 1;

}

void Weapon_Knife(edict_t* ent)
{
	static int pause_frames[] = { 22, 28, 0 };
	static int fire_frames[] = { 6, 7, 8, 9, 10, 105, 0 };
	// I think we need a special version of the generic function for this...
	Weapon_Generic_Knife(ent, 5, 12, 52, 59, 63, 102, 105, 110, 117, pause_frames, fire_frames, Knife_Fire);
}


// zucc - I thought about doing a gas grenade and even experimented with it some.  It
// didn't work out that great though, so I just changed this code to be the standard
// action grenade.  So the function name is just misleading...

void gas_fire(edict_t* ent)
{
	vec3_t offset;
	vec3_t forward, right;
	vec3_t start;
	int damage = GRENADE_DAMRAD;
	int speed;
	/*        int held = false;*/

	// Reset Grenade Damage to 1.52 when requested:
	if (use_classic->value)
		damage = GRENADE_DAMRAD_CLASSIC;
	else
		damage = GRENADE_DAMRAD;

	if (is_quad)
		damage *= 1.5f;

	VectorSet(offset, 8, 8, ent->viewheight - 8);
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	if (ent->client->pers.grenade_mode == 0)
		speed = 400;
	else if (ent->client->pers.grenade_mode == 1)
		speed = 720;
	else
		speed = 920;

	fire_grenade2(ent, start, forward, damage, speed, 2 * HZ, damage * 2, false);

	INV_AMMO(ent, GRENADE_NUM)--;
	if (INV_AMMO(ent, GRENADE_NUM) <= 0)
	{
		ent->client->newweapon = GET_ITEM(MK23_NUM);
		ChangeWeapon(ent);
		return;
	}
	else
	{
		ent->client->weaponstate = WEAPON_RELOADING;
		ent->client->ps.gunframe = 0;
	}


	//      ent->client->grenade_framenum = level.framenum + 1 * HZ;
	ent->client->ps.gunframe++;
}


#define GRENADE_ACTIVATE_LAST 3
#define GRENADE_PULL_FIRST 72
#define GRENADE_PULL_LAST 79
//#define GRENADE_THROW_FIRST 4
//#define GRENADE_THROW_LAST 9 // throw it on frame 8?
#define GRENADE_PINIDLE_FIRST 10
#define GRENADE_PINIDLE_LAST 39
// moved these up in the file (actually now in g_local.h)
//#define GRENADE_IDLE_FIRST 40
//#define GRENADE_IDLE_LAST 69

// the 2 deactivation frames are a joke, they look like shit, probably best to roll
// the activation frames backwards.  Aren't really enough of them either.

void Weapon_Gas(edict_t* ent)
{
	if (ent->s.modelindex != 255)	// zucc vwep
		return;			// not on client, so VWep animations could do wacky things

	//FIREBLADE
	if (ent->client->weaponstate == WEAPON_FIRING &&
		((ent->solid == SOLID_NOT && ent->deadflag != DEAD_DEAD) ||
			lights_camera_action))
	{
		ent->client->weaponstate = WEAPON_READY;
	}
	//FIREBLADE

	if (ent->client->weaponstate == WEAPON_RELOADING)
	{
		if (ent->client->ps.gunframe < GRENADE_ACTIVATE_LAST)
		{
			ent->client->ps.gunframe++;
		}
		else
		{
			ent->client->ps.gunframe = GRENADE_PINIDLE_FIRST;
			ent->client->weaponstate = WEAPON_READY;
		}
	}

	if (ent->client->weaponstate == WEAPON_DROPPING)
	{
		if (ent->client->ps.gunframe == 0)
		{
			ChangeWeapon(ent);
			return;
		}
		else
		{
			// zucc going to have to do this a bit different because
			// of the way I roll gunframes backwards for the thrownknife position
			if ((ent->client->ps.gunframe) == 3)
			{
				if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
					SetAnimation( ent, FRAME_crpain4 + 1, FRAME_crpain1, ANIM_REVERSE );
				else
					SetAnimation( ent, FRAME_pain304 + 1, FRAME_pain301, ANIM_REVERSE );
			}
			ent->client->ps.gunframe--;
			return;
		}

	}
	if (ent->client->weaponstate == WEAPON_ACTIVATING)
	{

		if (ent->client->ps.gunframe == GRENADE_ACTIVATE_LAST)
		{
			ent->client->ps.gunframe = GRENADE_PINIDLE_FIRST;
			ent->client->weaponstate = WEAPON_READY;
			return;
		}

		if (ent->client->ps.gunframe == GRENADE_PULL_LAST)
		{
			ent->client->ps.gunframe = GRENADE_IDLE_FIRST;
			ent->client->weaponstate = WEAPON_READY;
			gi.cprintf(ent, PRINT_HIGH, "Pin pulled, ready for %s range throw\n",
				ent->client->pers.grenade_mode == 0 ? "short" :
				(ent->client->pers.grenade_mode == 1 ? "medium" : "long"));
			return;
		}

		if (ent->client->ps.gunframe == 75)
		{
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("misc/grenade.wav"), 1, ATTN_NORM, 0);
		}

		ent->client->resp.sniper_mode = 0;
		// has to be here for dropping the sniper rifle, in the drop command didn't work...
		ent->client->desired_fov = 90;
		ent->client->ps.fov = 90;

		ent->client->ps.gunframe++;
		//              gi.cprintf(ent, PRINT_HIGH, "After increment frames = %d\n", ent->client->ps.gunframe);
		return;
	}


	// bandaging case
	if ((ent->client->bandaging)
		&& (ent->client->weaponstate != WEAPON_FIRING)
		&& (ent->client->weaponstate != WEAPON_BURSTING)
		&& (ent->client->weaponstate != WEAPON_BUSY)
		&& (ent->client->weaponstate != WEAPON_BANDAGING))
	{
		ent->client->weaponstate = WEAPON_BANDAGING;
		ent->client->ps.gunframe = GRENADE_ACTIVATE_LAST;
		return;
	}


	if (ent->client->weaponstate == WEAPON_BUSY)
	{

		if (ent->client->bandaging == 1)
		{
			if (!(ent->client->idle_weapon))
			{
				Bandage(ent);
			}
			else
				(ent->client->idle_weapon)--;
			return;
		}
		// for after bandaging delay
		if (!(ent->client->idle_weapon) && ent->client->bandage_stopped)
		{
			if (INV_AMMO(ent, GRENADE_NUM) <= 0)
			{
				ent->client->newweapon = GET_ITEM(MK23_NUM);
				ent->client->bandage_stopped = 0;
				ChangeWeapon(ent);
				return;
			}

			ent->client->weaponstate = WEAPON_ACTIVATING;
			ent->client->ps.gunframe = 0;
			ent->client->bandage_stopped = 0;
		}
		else if (ent->client->bandage_stopped)
			(ent->client->idle_weapon)--;


	}

	if (ent->client->weaponstate == WEAPON_BANDAGING)
	{
		if (ent->client->ps.gunframe == 0)
		{
			ent->client->weaponstate = WEAPON_BUSY;
			ent->client->idle_weapon = BANDAGE_TIME;
			return;
		}
		ent->client->ps.gunframe--;
		return;
	}


	if ((ent->client->newweapon) && (ent->client->weaponstate != WEAPON_FIRING)
		&& (ent->client->weaponstate != WEAPON_BUSY))
	{

		// zucc - check if they have a primed grenade

		if (ent->client->curr_weap == GRENADE_NUM
			&& ((ent->client->ps.gunframe >= GRENADE_IDLE_FIRST
				&& ent->client->ps.gunframe <= GRENADE_IDLE_LAST)
				|| (ent->client->ps.gunframe >= GRENADE_THROW_FIRST
					&& ent->client->ps.gunframe <= GRENADE_THROW_LAST)))
		{
			int damage;

			// Reset Grenade Damage to 1.52 when requested:
			if (use_classic->value)
				damage = GRENADE_DAMRAD_CLASSIC;
			else
				damage = GRENADE_DAMRAD;

			if (is_quad)
				damage *= 1.5f;

			fire_grenade2(ent, ent->s.origin, vec3_origin, damage, 0, 2 * HZ, damage * 2, false);

			INV_AMMO(ent, GRENADE_NUM)--;
			if (INV_AMMO(ent, GRENADE_NUM) <= 0)
			{
				ent->client->newweapon = GET_ITEM(MK23_NUM);
				ChangeWeapon(ent);
				return;
			}
		}


		ent->client->ps.gunframe = GRENADE_ACTIVATE_LAST;
		// zucc more vwep stuff
		if ((GRENADE_ACTIVATE_LAST) < 4)
		{
			if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
				SetAnimation( ent, FRAME_crpain4 + 1, FRAME_crpain1, ANIM_REVERSE );
			else
				SetAnimation( ent, FRAME_pain304 + 1, FRAME_pain301, ANIM_REVERSE );
		}

		ent->client->weaponstate = WEAPON_DROPPING;
		return;
	}

	if (ent->client->weaponstate == WEAPON_READY)
	{
		if (((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK)
			&& (ent->solid != SOLID_NOT || ent->deadflag == DEAD_DEAD) &&
			!lights_camera_action && !ent->client->uvTime)
		{


			if (ent->client->ps.gunframe <= GRENADE_PINIDLE_LAST &&
				ent->client->ps.gunframe >= GRENADE_PINIDLE_FIRST)
			{
				ent->client->ps.gunframe = GRENADE_PULL_FIRST;
				ent->client->weaponstate = WEAPON_ACTIVATING;
				ent->client->latched_buttons &= ~BUTTON_ATTACK;
			}
			else
			{
				if (ent->client->ps.gunframe == GRENADE_IDLE_LAST)
				{
					ent->client->ps.gunframe = GRENADE_IDLE_FIRST;
					return;
				}
				ent->client->ps.gunframe++;
				return;
			}
		}

		if (ent->client->ps.gunframe >= GRENADE_IDLE_FIRST &&
			ent->client->ps.gunframe <= GRENADE_IDLE_LAST)
		{
			ent->client->ps.gunframe = GRENADE_THROW_FIRST;
			if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
				SetAnimation( ent, FRAME_crattak1 - 1, FRAME_crattak9, ANIM_ATTACK );
			else
				SetAnimation( ent, FRAME_attack1 - 1, FRAME_attack8, ANIM_ATTACK );
			ent->client->weaponstate = WEAPON_FIRING;
			return;

		}

		if (ent->client->ps.gunframe == GRENADE_PINIDLE_LAST)
		{
			ent->client->ps.gunframe = GRENADE_PINIDLE_FIRST;
			return;
		}

		ent->client->ps.gunframe++;
		return;
	}
	if (ent->client->weaponstate == WEAPON_FIRING)
	{

		if (ent->client->ps.gunframe == 8)
		{
			gas_fire(ent);
			return;
		}

		ent->client->ps.gunframe++;

		if (ent->client->ps.gunframe == GRENADE_IDLE_FIRST + 1 ||
			ent->client->ps.gunframe == GRENADE_PINIDLE_FIRST + 1)
			ent->client->weaponstate = WEAPON_READY;
	}
}
