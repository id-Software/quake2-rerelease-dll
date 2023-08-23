//-----------------------------------------------------------------------------
// g_itmes.c
//
// $Id: g_items.c,v 1.13 2002/12/31 17:07:22 igor_rock Exp $
//
//-----------------------------------------------------------------------------
// $Log: g_items.c,v $
// Revision 1.13  2002/12/31 17:07:22  igor_rock
// - corrected the Add_Ammo function to regard wp_flags
//
// Revision 1.12  2002/03/30 17:20:59  ra
// New cvar use_buggy_bandolier to control behavior of dropping bando and grenades
//
// Revision 1.11  2002/03/28 20:28:56  ra
// Forgot a }
//
// Revision 1.10  2002/03/28 20:24:08  ra
// I overfixed the bug...
//
// Revision 1.9  2002/03/28 20:20:30  ra
// Nasty grenade bug fixed.
//
// Revision 1.8  2002/03/27 15:16:56  freud
// Original 1.52 spawn code implemented for use_newspawns 0.
// Teamplay, when dropping bandolier, your drop the grenades.
// Teamplay, cannot pick up grenades unless wearing bandolier.
//
// Revision 1.7  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.6  2001/07/18 15:19:11  slicerdw
// Time for weapons and items dissapearing is set to "6" to prevent lag on ctf
//
// Revision 1.5  2001/05/31 16:58:14  igor_rock
// conflicts resolved
//
// Revision 1.4.2.4  2001/05/26 13:04:34  igor_rock
// added some sound to the precache list for flags
//
// Revision 1.4.2.3  2001/05/25 18:59:52  igor_rock
// Added CTF Mode completly :)
// Support for .flg files is still missing, but with "real" CTF maps like
// tq2gtd1 the ctf works fine.
// (I hope that all other modes still work, just tested DM and teamplay)
//
// Revision 1.4.2.2  2001/05/20 18:54:19  igor_rock
// added original ctf code snippets from zoid. lib compilesand runs but
// doesn't function the right way.
// Jsut committing these to have a base to return to if something wents
// awfully wrong.
//
// Revision 1.4.2.1  2001/05/20 15:17:31  igor_rock
// removed the old ctf code completly
//
// Revision 1.4  2001/05/15 15:49:14  igor_rock
// added itm_flags for deathmatch
//
// Revision 1.3  2001/05/14 21:10:16  igor_rock
// added wp_flags support (and itm_flags skeleton - doesn't disturb in the moment)
//
// Revision 1.2  2001/05/11 16:07:25  mort
// Various CTF bits and pieces...
//
// Revision 1.1.1.1  2001/05/06 17:31:33  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#include "g_local.h"


qboolean Pickup_Weapon (edict_t * ent, edict_t * other);
void Use_Weapon (edict_t * ent, gitem_t * inv);
void Drop_Weapon (edict_t * ent, gitem_t * inv);

// zucc
void Weapon_MK23 (edict_t * ent);
void Weapon_MP5 (edict_t * ent);
void Weapon_M4 (edict_t * ent);
void Weapon_M3 (edict_t * ent);
void Weapon_HC (edict_t * ent);
void Weapon_Sniper (edict_t * ent);
void Weapon_Dual (edict_t * ent);
void Weapon_Knife (edict_t * ent);
void Weapon_Gas (edict_t * ent);

#define HEALTH_IGNORE_MAX       1
#define HEALTH_TIMED            2
#define HEALTH_MEDKIT           4

void Use_Quad (edict_t * ent, gitem_t * item);
static int quad_drop_timeout_hack;

//======================================================================

/*
===============
FindItemByNum
===============
*/
gitem_t *FindItemByNum (int num)
{
	int i;
	gitem_t *it;

	it = itemlist+1;
	for (i = 1; i < game.num_items; i++, it++)
	{
		if (it->typeNum == num)
			return it;
	}

	return &itemlist[0];
}
/*
===============
FindItemByClassname

===============
*/
gitem_t *FindItemByClassname (char *classname)
{
	int i;
	gitem_t *it;

	it = itemlist+1;
	for (i = 1; i < game.num_items; i++, it++)
	{
		if (!it->classname)
			continue;
		if (!Q_stricmp (it->classname, classname))
			return it;
	}

	return NULL;
}

/*
===============
FindItem

===============
*/
gitem_t *FindItem (char *pickup_name)
{
	int i;
	gitem_t *it;

	it = itemlist+1;
	for (i = 1; i < game.num_items; i++, it++)
	{
		if (!it->pickup_name)
			continue;
		if (!Q_stricmp (it->pickup_name, pickup_name))
			return it;
	}

	return NULL;
}

//======================================================================

void DoRespawn (edict_t * ent)
{
	if (!ent)
		return;

	if (ent->team)
	{
		edict_t *master;
		int count;
		int choice;

		master = ent->teammaster;

		count = 0;
		for (ent = master; ent; ent = ent->chain)
			count++;

		choice = count ? (rand() % count) : 0;

		count = 0;
		for (ent = master; count < choice; ent = ent->chain)
			count++;
	}

	ent->svflags &= ~SVF_NOCLIENT;
	ent->solid = SOLID_TRIGGER;
	gi.linkentity(ent);

	// send an effect
	ent->s.event = EV_ITEM_RESPAWN;
}

void SetRespawn (edict_t * ent, float delay)
{
	ent->flags |= FL_RESPAWN;
	ent->svflags |= SVF_NOCLIENT;
	ent->solid = SOLID_NOT;
	ent->nextthink = level.framenum + delay * HZ;
	ent->think = DoRespawn;
	gi.linkentity (ent);
}


//======================================================================

qboolean Pickup_Powerup (edict_t * ent, edict_t * other)
{
	other->client->inventory[ITEM_INDEX (ent->item)]++;

	if (!(ent->spawnflags & DROPPED_ITEM))
		SetRespawn (ent, ent->item->quantity);
	//if (DMFLAGS(DF_INSTANT_ITEMS)
	//	|| ((ent->item->use == Use_Quad) && (ent->spawnflags & DROPPED_PLAYER_ITEM)))
	//{
		if ((ent->item->use == Use_Quad) && (ent->spawnflags & DROPPED_PLAYER_ITEM))
			quad_drop_timeout_hack = ent->nextthink - level.framenum;
		ent->item->use (other, ent->item);
	//}

	return true;
}

void AddItem(edict_t *ent, gitem_t *item)
{
	ent->client->inventory[ITEM_INDEX (item)]++;
	ent->client->unique_item_total++;
	if (item->typeNum == LASER_NUM)
	{
		SP_LaserSight(ent, item);	//ent->item->use(other, ent->item);
	}
	else if (item->typeNum == BAND_NUM)
	{

		if (ent->client->max_pistolmags < 4)
			ent->client->max_pistolmags = 4;
		if (ent->client->max_shells < 28)
			ent->client->max_shells = 28;
		if (ent->client->max_m4mags < 2)
			ent->client->max_m4mags = 2;
		if (ent->client->max_sniper_rnds < 40)
			ent->client->max_sniper_rnds = 40;
		if (ent->client->max_mp5mags < 4)
			ent->client->max_mp5mags = 4;
		if (ent->client->knife_max < 20)
			ent->client->knife_max = 20;
		if (ent->client->grenade_max < 4)
			ent->client->grenade_max = 4;
		// zucc for ir
		/*if ( ir->value && other->client->pers.irvision )
		{
			other->client->ps.rdflags |= RDF_IRGOGGLES;
		}
		*/
	}
}

qboolean Pickup_ItemPack (edict_t * ent, edict_t * other)
{
	/* this gives 2 random items

	gitem_t *spec;

	int count, added, item;



	for(count = 0, added = 0; added < 2 && count < 20; count++)

	{

		item = ITEM_FIRST + newrand(ITEM_COUNT);

		if (INV_AMMO(other, item) > 0 || !ITF_ALLOWED(item))

			continue;



		spec = GET_ITEM(item);

		AddItem(other, spec);

		added++;

	}*/



	if (INV_AMMO(other, KEV_NUM) < 1 && ITF_ALLOWED(KEV_NUM))

		AddItem(other, GET_ITEM(KEV_NUM));



	if (INV_AMMO(other, LASER_NUM) < 1 && ITF_ALLOWED(LASER_NUM))

		AddItem(other, GET_ITEM(LASER_NUM));



	if (INV_AMMO(other, HELM_NUM) < 1 && ITF_ALLOWED(HELM_NUM))

		AddItem(other, GET_ITEM(HELM_NUM));



	SetRespawn(ent, 120);



	return true;
}

//zucc pickup function for special items
qboolean Pickup_Special (edict_t * ent, edict_t * other)
{
	if (other->client->unique_item_total >= unique_items->value)
		return false;

	// Don't allow picking up multiple of the same special item.
	if( (! allow_hoarding->value) && other->client->inventory[ITEM_INDEX(ent->item)] )
		return false;

	AddItem(other, ent->item);

	if(!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)) && item_respawnmode->value)
		SetRespawn (ent, item_respawn->value);

	return true;
}



void Drop_Special (edict_t * ent, gitem_t * item)
{
	int count;

	ent->client->unique_item_total--;
	if (item->typeNum == BAND_NUM && INV_AMMO(ent, BAND_NUM) <= 1)
	{
		if (gameSettings & GS_DEATHMATCH)
			count = 2;
		else
			count = 1;

		ent->client->max_pistolmags = count;
		if (INV_AMMO(ent, MK23_ANUM) > count)
			INV_AMMO(ent, MK23_ANUM) = count;

		if (!(gameSettings & GS_DEATHMATCH)) {
			if(ent->client->pers.chosenWeapon->typeNum == HC_NUM)
				count = 12;
			else
				count = 7;
		} else {
			count = 14;
		}
		ent->client->max_shells = count;
		if (INV_AMMO(ent, SHELL_ANUM) > count)
			INV_AMMO(ent, SHELL_ANUM) = count;

		ent->client->max_m4mags = 1;
		if (INV_AMMO(ent, M4_ANUM) > 1)
			INV_AMMO(ent, M4_ANUM) = 1;

		ent->client->grenade_max = 2;
		if (use_buggy_bandolier->value == 0) {
			if ((gameSettings & GS_DEATHMATCH) && INV_AMMO(ent, GRENADE_NUM) > 2)
				INV_AMMO(ent, GRENADE_NUM) = 2;
			else if (teamplay->value) {
				if (ent->client->curr_weap == GRENADE_NUM)
					INV_AMMO(ent, GRENADE_NUM) = 1;
				else
					INV_AMMO(ent, GRENADE_NUM) = 0;
			}
		} else {
			if (INV_AMMO(ent, GRENADE_NUM) > 2)
				INV_AMMO(ent, GRENADE_NUM) = 2;
		}
		if (gameSettings & GS_DEATHMATCH)
			count = 2;
		else
			count = 1;
		ent->client->max_mp5mags = count;
		if (INV_AMMO(ent, MP5_ANUM) > count)
			INV_AMMO(ent, MP5_ANUM) = count;

		ent->client->knife_max = 10;
		if (INV_AMMO(ent, KNIFE_NUM) > 10)
			INV_AMMO(ent, KNIFE_NUM) = 10;

		if (gameSettings & GS_DEATHMATCH)
			count = 20;
		else
			count = 10;
		ent->client->max_sniper_rnds = count;
		if (INV_AMMO(ent, SNIPER_ANUM) > count)
			INV_AMMO(ent, SNIPER_ANUM) = count;

		if (ent->client->unique_weapon_total > unique_weapons->value && !allweapon->value)
		{
			DropExtraSpecial (ent);
			gi.cprintf (ent, PRINT_HIGH, "One of your guns is dropped with the bandolier.\n");
		}
	}
	Drop_Spec(ent, item);
	ValidateSelectedItem(ent);
	SP_LaserSight(ent, item);
}

// called by the "drop item" command
void DropSpecialItem (edict_t * ent)
{
	// this is the order I'd probably want to drop them in...       
	if (INV_AMMO(ent, LASER_NUM))
		Drop_Special (ent, GET_ITEM(LASER_NUM));
	else if (INV_AMMO(ent, SLIP_NUM))
		Drop_Special (ent, GET_ITEM(SLIP_NUM));
	else if (INV_AMMO(ent, SIL_NUM))
		Drop_Special (ent, GET_ITEM(SIL_NUM));
	else if (INV_AMMO(ent, BAND_NUM))
		Drop_Special (ent, GET_ITEM(BAND_NUM));
	else if (INV_AMMO(ent, HELM_NUM))
		Drop_Special (ent, GET_ITEM(HELM_NUM));
	else if (INV_AMMO(ent, KEV_NUM))
		Drop_Special (ent, GET_ITEM(KEV_NUM));
}


void Drop_General (edict_t * ent, gitem_t * item)
{
	Drop_Item (ent, item);
	ent->client->inventory[ITEM_INDEX (item)]--;
	ValidateSelectedItem (ent);
}


//======================================================================

qboolean Pickup_Adrenaline (edict_t * ent, edict_t * other)
{
	if (other->health < other->max_health)
		other->health = other->max_health;

	if (!(ent->spawnflags & DROPPED_ITEM))
		SetRespawn(ent, ent->item->quantity);

	return true;
}

qboolean Pickup_AncientHead (edict_t * ent, edict_t * other)
{
	other->max_health += 2;

	if (!(ent->spawnflags & DROPPED_ITEM))
		SetRespawn (ent, ent->item->quantity);

	return true;
}

qboolean Pickup_Bandolier (edict_t * ent, edict_t * other)
{
#if 0
	gitem_t *item;
	int index;

	if (other->client->max_bullets < 250)
		other->client->max_bullets = 250;
	if (other->client->max_shells < 150)
		other->client->max_shells = 150;
	if (other->client->max_cells < 250)
		other->client->max_cells = 250;
	if (other->client->max_slugs < 75)
		other->client->max_slugs = 75;

	item = FindItem ("Bullets");
	if (item)
	{
		index = ITEM_INDEX (item);
		other->client->inventory[index] += item->quantity;
		if (other->client->inventory[index] > other->client->max_bullets)
			other->client->inventory[index] = other->client->max_bullets;
	}

	item = FindItem ("Shells");
	if (item)
	{
		index = ITEM_INDEX (item);
		other->client->inventory[index] += item->quantity;
		if (other->client->inventory[index] > other->client->max_shells)
			other->client->inventory[index] = other->client->max_shells;
	}

	if (!(ent->spawnflags & DROPPED_ITEM))
		SetRespawn (ent, ent->item->quantity);
#endif
	return true;
}

qboolean Pickup_Pack (edict_t * ent, edict_t * other)
{
#if 0
	gitem_t *item;
	int index;

	if (other->client->max_bullets < 300)
		other->client->max_bullets = 300;
	if (other->client->max_shells < 200)
		other->client->max_shells = 200;
	if (other->client->max_rockets < 100)
		other->client->max_rockets = 100;
	if (other->client->max_grenades < 100)
		other->client->max_grenades = 100;
	if (other->client->max_cells < 300)
		other->client->max_cells = 300;
	if (other->client->max_slugs < 100)
		other->client->max_slugs = 100;

	item = FindItem ("Bullets");
	if (item)
	{
		index = ITEM_INDEX (item);
		other->client->inventory[index] += item->quantity;
		if (other->client->inventory[index] > other->client->max_bullets)
			other->client->inventory[index] = other->client->max_bullets;
	}

	item = FindItem ("Shells");
	if (item)
	{
		index = ITEM_INDEX (item);
		other->client->inventory[index] += item->quantity;
		if (other->client->inventory[index] > other->client->max_shells)
			other->client->inventory[index] = other->client->max_shells;
	}

	item = FindItem ("Cells");
	if (item)
	{
		index = ITEM_INDEX (item);
		other->client->inventory[index] += item->quantity;
		if (other->client->inventory[index] > other->client->max_cells)
		other->client->inventory[index] = other->client->max_cells;
	}

	item = FindItem ("Grenades");
	if (item)
	{
		index = ITEM_INDEX (item);
		other->client->inventory[index] += item->quantity;
		if (other->client->inventory[index] > other->client->max_grenades)
			other->client->inventory[index] = other->client->max_grenades;
	}

	item = FindItem ("Rockets");
	if (item)
	{
		index = ITEM_INDEX (item);
		other->client->inventory[index] += item->quantity;
		if (other->client->inventory[index] > other->client->max_rockets)
			other->client->inventory[index] = other->client->max_rockets;
	}

	item = FindItem ("Slugs");
	if (item)
	{
		index = ITEM_INDEX (item);
		other->client->inventory[index] += item->quantity;
		if (other->client->inventory[index] > other->client->max_slugs)
		other->client->inventory[index] = other->client->max_slugs;
	}

	if (!(ent->spawnflags & DROPPED_ITEM))
		SetRespawn (ent, ent->item->quantity);
#endif
	return true;
}

//======================================================================

void Use_Quad (edict_t * ent, gitem_t * item)
{
	int timeout;

	ent->client->inventory[ITEM_INDEX (item)]--;
	ValidateSelectedItem (ent);

	if (quad_drop_timeout_hack)
	{
		timeout = quad_drop_timeout_hack;
		quad_drop_timeout_hack = 0;
	}
	else
	{
		timeout = 30 * HZ;
	}

	if (ent->client->quad_framenum > level.framenum)
		ent->client->quad_framenum += timeout;
	else
		ent->client->quad_framenum = level.framenum + timeout;

	gi.sound (ent, CHAN_ITEM, gi.soundindex ("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void Use_Breather (edict_t * ent, gitem_t * item)
{
	ent->client->inventory[ITEM_INDEX (item)]--;
	ValidateSelectedItem (ent);

	if (ent->client->breather_framenum > level.framenum)
		ent->client->breather_framenum += 30 * HZ;
	else
		ent->client->breather_framenum = level.framenum + 30 * HZ;

//      gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void Use_Envirosuit (edict_t * ent, gitem_t * item)
{
	ent->client->inventory[ITEM_INDEX (item)]--;
	ValidateSelectedItem (ent);

	if (ent->client->enviro_framenum > level.framenum)
		ent->client->enviro_framenum += 30 * HZ;
	else
		ent->client->enviro_framenum = level.framenum + 30 * HZ;

//      gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void Use_Invulnerability (edict_t * ent, gitem_t * item)
{
	ent->client->inventory[ITEM_INDEX (item)]--;
	ValidateSelectedItem (ent);

	if (ent->client->invincible_framenum > level.framenum)
		ent->client->invincible_framenum += 30 * HZ;
	else
		ent->client->invincible_framenum = level.framenum + 30 * HZ;

	gi.sound (ent, CHAN_ITEM, gi.soundindex ("items/protect.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void Use_Silencer (edict_t * ent, gitem_t * item)
{
	ent->client->inventory[ITEM_INDEX (item)]--;
	ValidateSelectedItem (ent);
	ent->client->silencer_shots += 30;  // For grappling hook?

	// Turn Q2 silencer into AQ2 silencer.
	AddItem( ent, GET_ITEM(SIL_NUM) );

//      gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

qboolean Add_Ammo (edict_t * ent, gitem_t * item, int count)
{
	int index;
	int max = 0;

	if (!ent->client)
		return false;

	switch(item->typeNum) {
	case MK23_ANUM:
		if (WPF_ALLOWED(item->typeNum))
			max = ent->client->max_pistolmags;
		break;
	case SHELL_ANUM:
		if (WPF_ALLOWED(item->typeNum))
			max = ent->client->max_shells;
		break;
	case MP5_ANUM:
		if (WPF_ALLOWED(item->typeNum))
			max = ent->client->max_mp5mags;
		break;
	case M4_ANUM:
		if (WPF_ALLOWED(item->typeNum))
			max = ent->client->max_m4mags;
		break;
	case SNIPER_ANUM:
		if (WPF_ALLOWED(item->typeNum))
			max = ent->client->max_sniper_rnds;
		break;
	default:
		return false;
	}

	index = ITEM_INDEX (item);

	if (ent->client->inventory[index] == max)
		return false;

	ent->client->inventory[index] += count;

	if (ent->client->inventory[index] > max)
		ent->client->inventory[index] = max;

	return true;
}

qboolean Pickup_Ammo (edict_t * ent, edict_t * other)
{
	int count;
	qboolean weapon;

	weapon = (ent->item->flags & IT_WEAPON);
	if ((weapon) && DMFLAGS(DF_INFINITE_AMMO))
		count = 1000;
	else if (ent->count)
		count = ent->count;
	else
		count = ent->item->quantity;

	if (!Add_Ammo (other, ent->item, count))
		return false;

	if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
		SetRespawn (ent, ammo_respawn->value);

	return true;
}

void Drop_Ammo (edict_t * ent, gitem_t * item)
{
	edict_t *dropped;
	int index;

	if (ent->client->weaponstate == WEAPON_RELOADING)
	{
		gi.cprintf (ent, PRINT_HIGH, "Cant drop ammo while reloading\n");
		return;
	}

	index = ITEM_INDEX (item);
	dropped = Drop_Item (ent, item);
	if (ent->client->inventory[index] >= item->quantity)
		dropped->count = item->quantity;
	else
		dropped->count = ent->client->inventory[index];
	ent->client->inventory[index] -= dropped->count;
	ValidateSelectedItem (ent);
}


//======================================================================

void MegaHealth_think (edict_t * self)
{
	if (self->owner->health > self->owner->max_health)
	{
		self->nextthink = level.framenum + 1 * HZ;
		self->owner->health -= 1;
		return;
	}

	if (!(self->spawnflags & DROPPED_ITEM))
		SetRespawn (self, 20);
	else
		G_FreeEdict (self);
}

qboolean Pickup_Health (edict_t * ent, edict_t * other)
{
	// Raptor007: MedKit heals when bandaging, not on item pickup.
	if( ent->style & HEALTH_MEDKIT )
	{
		int max_medkit = INV_AMMO( other, BAND_NUM ) ? 99 : medkit_drop->value;
		if( other->client->medkit >= max_medkit )
			return false;

		other->client->medkit += ent->count;
		if( other->client->medkit > max_medkit )
			other->client->medkit = max_medkit;

		ent->item->pickup_sound = NULL;

		return true;
	}

	if (!(ent->style & HEALTH_IGNORE_MAX))
		if (other->health >= other->max_health)
			return false;

	other->health += ent->count;

	if (ent->count < 10)        // (ent->count == 2)
		ent->item->pickup_sound = "items/s_health.wav";
	else if (ent->count < 25)   // (ent->count == 10)
		ent->item->pickup_sound = "items/n_health.wav";
	else if (ent->count < 100)  // (ent->count == 25)
		ent->item->pickup_sound = "items/l_health.wav";
	else				// (ent->count == 100)
		ent->item->pickup_sound = "items/m_health.wav";

	if (!(ent->style & HEALTH_IGNORE_MAX))
	{
		if (other->health > other->max_health)
			other->health = other->max_health;
	}

	if (ent->style & HEALTH_TIMED)
	{
		ent->think = MegaHealth_think;
		ent->nextthink = level.framenum + 5 * HZ;
		ent->owner = other;
		ent->flags |= FL_RESPAWN;
		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
	}
	else
	{
		if (!(ent->spawnflags & DROPPED_ITEM))
			SetRespawn (ent, 30);
	}

	return true;
}

//======================================================================

/*
===============
Touch_Item
===============
*/
void Touch_Item (edict_t * ent, edict_t * other, cplane_t * plane,
	    csurface_t * surf)
{
	qboolean taken;

	if (!other->client)
		return;
	if (other->health < 1)
		return;			// dead people can't pickup
	if (!ent->item || !ent->item->pickup)
		return;			// not a grabbable item?

	taken = ent->item->pickup(ent, other);
	if (taken)
	{
		// flash the screen
		other->client->bonus_alpha = 0.25;

		// show icon and name on status bar
		if (ent->item->typeNum < AMMO_MAX)
			other->client->ps.stats[STAT_PICKUP_ICON] = level.pic_items[ent->item->typeNum];
		else
			other->client->ps.stats[STAT_PICKUP_ICON] = gi.imageindex(ent->item->icon);

		other->client->ps.stats[STAT_PICKUP_STRING] = CS_ITEMS + ITEM_INDEX (ent->item);
		other->client->pickup_msg_framenum = level.realFramenum + 3 * HZ;

		// change selected item
		if (ent->item->use)
			other->client->selected_item = other->client->ps.stats[STAT_SELECTED_ITEM] = ITEM_INDEX(ent->item);
		else
			gi.sound(other, CHAN_ITEM, gi.soundindex(ent->item->pickup_sound), 1, ATTN_NORM, 0);
	}

	if (!(ent->spawnflags & ITEM_TARGETS_USED))
	{
		G_UseTargets(ent, other);
		ent->spawnflags |= ITEM_TARGETS_USED;
	}

	if (!taken)
		return;

	if (ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM))
	{
		if (ent->flags & FL_RESPAWN)
			ent->flags &= ~FL_RESPAWN;
		else
			G_FreeEdict (ent);
	}
}

//======================================================================

static void drop_temp_touch (edict_t * ent, edict_t * other, cplane_t * plane,
		 csurface_t * surf)
{
  if (other == ent->owner)
    return;

  Touch_Item (ent, other, plane, surf);
}

static void drop_make_touchable (edict_t * ent)
{
	ent->touch = Touch_Item;

	//AQ2:TNG - Slicer
	if( ent->item && (ent->item->pickup == Pickup_Health) )
	{
		ent->nextthink = level.framenum + (medkit_time->value - 1) * HZ;
		ent->think = G_FreeEdict;
	}
	else if( ctf->value )
	{
		ent->nextthink = level.framenum + 6 * HZ;
		ent->think = G_FreeEdict;
	}
	else
	{
		ent->nextthink = level.framenum + 119 * HZ;
		ent->think = G_FreeEdict;
	}
}

edict_t *Drop_Item (edict_t * ent, gitem_t * item)
{
	edict_t *dropped;
	vec3_t forward, right;
	vec3_t offset;

	dropped = G_Spawn ();

	dropped->classname = item->classname;
	dropped->typeNum = item->typeNum;
	dropped->item = item;
	dropped->spawnflags = DROPPED_ITEM;
	dropped->s.effects = item->world_model_flags;
	dropped->s.renderfx = RF_GLOW;
	VectorSet (dropped->mins, -15, -15, -15);
	VectorSet (dropped->maxs, 15, 15, 15);
	// zucc dumb hack to make knife look like it is on the ground
	if (item->typeNum == KNIFE_NUM
	 || item->typeNum == LASER_NUM
	 || item->typeNum == GRENADE_NUM)
	{
		VectorSet (dropped->mins, -15, -15, -1);
		VectorSet (dropped->maxs, 15, 15, 1);
	}
	// spin?
	VectorSet (dropped->avelocity, 0, 600, 0);
	gi.setmodel (dropped, dropped->item->world_model);
	dropped->solid = SOLID_TRIGGER;
	dropped->movetype = MOVETYPE_TOSS;

	dropped->touch = drop_temp_touch;
	dropped->owner = ent;

	if (ent->client)
	{
		trace_t trace;

		AngleVectors (ent->client->v_angle, forward, right, NULL);
		VectorSet (offset, 24, 0, -16);
		G_ProjectSource (ent->s.origin, offset, forward, right,
		dropped->s.origin);
		PRETRACE ();
		trace = gi.trace (ent->s.origin, dropped->mins, dropped->maxs,
		dropped->s.origin, ent, CONTENTS_SOLID);
		POSTTRACE ();
		VectorCopy (trace.endpos, dropped->s.origin);
	}
	else
	{
		AngleVectors (ent->s.angles, forward, right, NULL);
		VectorCopy (ent->s.origin, dropped->s.origin);
	}

	VectorCopy(dropped->s.origin, dropped->old_origin);

	VectorScale (forward, 100, dropped->velocity);
	dropped->velocity[2] = 300;

	dropped->think = drop_make_touchable;
	dropped->nextthink = level.framenum + 1 * HZ;

	gi.linkentity (dropped);

	return dropped;
}

void Use_Item (edict_t * ent, edict_t * other, edict_t * activator)
{
	ent->svflags &= ~SVF_NOCLIENT;
	ent->use = NULL;

	if (ent->spawnflags & ITEM_NO_TOUCH)
	{
		ent->solid = SOLID_BBOX;
		ent->touch = NULL;
	}
	else
	{
		ent->solid = SOLID_TRIGGER;
		ent->touch = Touch_Item;
	}

	gi.linkentity (ent);
}

//======================================================================

/*
================
droptofloor
================
*/
void droptofloor (edict_t * ent)
{
	trace_t tr;
	vec3_t dest;

	VectorSet(ent->mins, -15, -15, -15);
	VectorSet(ent->maxs, 15, 15, 15);

	if (ent->item)
	{
		if (ent->item->typeNum == KNIFE_NUM
		 || ent->item->typeNum == LASER_NUM
		 || ent->item->typeNum == GRENADE_NUM)
		{
			VectorSet (ent->mins, -15, -15, -1);
			VectorSet (ent->maxs, 15, 15, 1);
		}
	}

	if (ent->model)
		gi.setmodel(ent, ent->model);
	else if (ent->item)
		gi.setmodel(ent, ent->item->world_model);

	ent->solid = SOLID_TRIGGER;
	ent->movetype = MOVETYPE_TOSS;
	ent->touch = Touch_Item;

	VectorCopy(ent->s.origin, dest);
	dest[2] -= 128;

	PRETRACE ();
	tr = gi.trace (ent->s.origin, ent->mins, ent->maxs, dest, ent, MASK_SOLID);
	POSTTRACE ();
	if (tr.startsolid)
	{
		gi.dprintf ("droptofloor: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
		G_FreeEdict (ent);
		return;
	}

	VectorCopy (tr.endpos, ent->s.origin);
	VectorCopy (tr.endpos, ent->old_origin);

	if (ent->team)
	{
		ent->flags &= ~FL_TEAMSLAVE;
		ent->chain = ent->teamchain;
		ent->teamchain = NULL;

		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
		if (ent == ent->teammaster)
		{
			ent->nextthink = level.framenum + 1;
			ent->think = DoRespawn;
		}
	}

	if (ent->spawnflags & ITEM_NO_TOUCH)
	{
		ent->solid = SOLID_BBOX;
		ent->touch = NULL;
		ent->s.effects &= ~EF_ROTATE;
		ent->s.renderfx &= ~RF_GLOW;
	}

	if (ent->spawnflags & ITEM_TRIGGER_SPAWN)
	{
		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
		ent->use = Use_Item;
	}

	gi.linkentity (ent);
}


/*
===============
PrecacheItem

Precaches all data needed for a given item.
This will be called for each item spawned in a level,
and for each item in each client's inventory.
===============
*/
void PrecacheItem (gitem_t * it)
{
	char *s, *start;
	char data[MAX_QPATH];
	size_t len;
	gitem_t *ammo;

	if (!it)
		return;

	if (it->pickup_sound)
		gi.soundindex (it->pickup_sound);
	if (it->world_model)
		gi.modelindex (it->world_model);
	if (it->view_model)
		gi.modelindex (it->view_model);
	if (it->icon)
		gi.imageindex (it->icon);

	// parse everything for its ammo
	if (it->ammo && it->ammo[0])
	{
		ammo = FindItem (it->ammo);
		if (ammo != it)
			PrecacheItem (ammo);
	}

	// parse the space seperated precache string for other items
	s = it->precaches;
	if (!s || !s[0])
		return;

	while (*s)
	{
		start = s;
		while (*s && *s != ' ')
			s++;

		len = s - start;
		if (len >= MAX_QPATH || len < 5) {
			gi.error( "PrecacheItem: %s has bad precache string", it->classname );
			continue;
		}
		memcpy(data, start, len);
		data[len] = 0;
		if (*s)
			s++;

		// determine type based on extension
		if (!strcmp (data + len - 3, "md2"))
			gi.modelindex (data);
		else if (!strcmp (data + len - 3, "sp2"))
			gi.modelindex (data);
		else if (!strcmp (data + len - 3, "wav"))
			gi.soundindex (data);
		else if (!strcmp (data + len - 3, "pcx"))
			gi.imageindex (data);
	}
}


/*
============
PrecacheItems

Makes sure the client loads all necessary data on connect to avoid lag.
============
*/
void PrecacheItems( void )
{
	int i;

	for (i = 1; i<AMMO_FIRST; i++) {

		PrecacheItem( GET_ITEM(i) );

	}

	if (ctf->value) {
		PrecacheItem( GET_ITEM(FLAG_T1_NUM) );
		PrecacheItem( GET_ITEM(FLAG_T2_NUM) );
	}
}


/*
============
SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void SpawnItem (edict_t * ent, gitem_t * item)
{
	// Weapons and Ammo
	if (item->typeNum)
	{
		if (item->typeNum < ITEM_FIRST) { //Weapons
			if (!WPF_ALLOWED( item->typeNum )) {
				G_FreeEdict( ent );
				return;
			}
		}
		else if (item->typeNum < AMMO_FIRST) { //Items
			if (!ITF_ALLOWED( item->typeNum )) {
				G_FreeEdict( ent );
				return;
			}
		}
		else if (item->typeNum < AMMO_FIRST + AMMO_COUNT) { //Ammo
			if (!WPF_ALLOWED( item->typeNum )) {
				G_FreeEdict( ent );
				return;
			}
		}
		else if (item->typeNum == FLAG_T1_NUM || item->typeNum == FLAG_T2_NUM) {
			//Don't spawn the flags unless enabled
			if (!ctf->value) {
				G_FreeEdict( ent );
				return;
			}
		}
		else { //Weapons/items/ammo is always precached
			PrecacheItem( item );
		}
	}
	else
	{
		PrecacheItem( item );
	}

	// some items will be prevented in deathmatch
	if (DMFLAGS(DF_NO_ITEMS))
	{
		if (item->pickup == Pickup_Powerup)
		{
			G_FreeEdict (ent);
			return;
		}
	}
	// zucc remove health from the game
	if (1 /*DMFLAGS(DF_NO_HEALTH) */ )
	{
		if (item->pickup == Pickup_Health
		|| item->pickup == Pickup_Adrenaline
		|| item->pickup == Pickup_AncientHead)
		{
			G_FreeEdict (ent);
			return;
		}
	}
	if (DMFLAGS(DF_INFINITE_AMMO))
	{
		if (item->flags == IT_AMMO)
		{
			G_FreeEdict (ent);
			return;
		}
	}

	ent->item = item;
	ent->nextthink = level.framenum + 2;	// items start after other solids
	ent->think = droptofloor;
	ent->s.effects = item->world_model_flags;
	ent->s.renderfx = RF_GLOW;
	ent->typeNum = item->typeNum;
	if (ent->model)
		gi.modelindex (ent->model);

	//flags are server animated and have special handling
	if (item->typeNum == FLAG_T1_NUM || item->typeNum == FLAG_T2_NUM) {
		ent->think = CTFFlagSetup;
	}
}

//======================================================================

gitem_t itemlist[] = {
  {
   NULL}
  ,				// leave index 0 alone

  //
  // WEAPONS 
  //

// zucc - New Weapons
/*
gitem_t

referenced by 'entity_name->item.attribute'

Name              Type              Notes

ammo              char *            type of ammo to use
classname         char *            name when spawning it
count_width       int               number of digits to display by icon
drop              void              function called when entity dropped
flags             int               type of pickup :
                                    IT_WEAPON, IT_AMMO, IT_ARMOR
icon              char *            filename of icon
info              void *            ? unused
pickup            qboolean          function called when entity picked up
pickup_name       char *            displayed onscreen when item picked up
pickup_sound      char *            filename of sound to play when picked up
precaches         char *            string containing all models, sounds etc. needed by this
                                    item
quantity          int               ammo gained by item/ammo used per shot by item
tag               int               ? unused
use               void              function called when entity used
view_model        char *            filename of model when being held
weaponthink       void              unused function
world_model       char *            filename of model when item is sitting on level
world_model_flags int               copied to 'ent->s.effects' (see s.effects for values)



  */
  {
   "weapon_Mk23",
   Pickup_Weapon,
   Use_Weapon,
   Drop_Weapon,
   Weapon_MK23,
   //"misc/w_pkup.wav",
   NULL,
   "models/weapons/g_dual/tris.md2",
   0,
   "models/weapons/v_blast/tris.md2",
   "w_mk23",
   MK23_NAME,
   0,
   1,
   MK23_AMMO_NAME,
   IT_WEAPON,
   NULL,
   0,
   "weapons/mk23fire.wav weapons/mk23in.wav weapons/mk23out.wav weapons/mk23slap.wav weapons/mk23slide.wav misc/click.wav weapons/machgf4b.wav weapons/blastf1a.wav",
  MK23_NUM}
  ,


  {
   "weapon_MP5",
   Pickup_Weapon,
   Use_Weapon,
   Drop_Weapon,
   Weapon_MP5,
   //"misc/w_pkup.wav",
   NULL,
   "models/weapons/g_machn/tris.md2",
   0,
   "models/weapons/v_machn/tris.md2",
   "w_mp5",
   MP5_NAME,
   0,
   0,
   MP5_AMMO_NAME,
   IT_WEAPON,
   NULL,
   0,
   "weapons/mp5fire1.wav weapons/mp5in.wav weapons/mp5out.wav weapons/mp5slap.wav weapons/mp5slide.wav weapons/machgf1b.wav weapons/machgf2b.wav weapons/machgf3b.wav weapons/machgf5b.wav",
   MP5_NUM}
  ,
  {
   "weapon_M4",
   Pickup_Weapon,
   Use_Weapon,
   Drop_Weapon,
   Weapon_M4,
   //"misc/w_pkup.wav",
   NULL,
   "models/weapons/g_m4/tris.md2",
   0,
   "models/weapons/v_m4/tris.md2",
   "w_m4",
   M4_NAME,
   0,
   0,
   M4_AMMO_NAME,
   IT_WEAPON,
   NULL,
   0,
   "weapons/m4a1fire.wav weapons/m4a1in.wav weapons/m4a1out.wav weapons/m4a1slide.wav weapons/rocklf1a.wav weapons/rocklr1b.wav",
  M4_NUM}
  ,
  {
   "weapon_M3",
   Pickup_Weapon,
   Use_Weapon,
   Drop_Weapon,
   Weapon_M3,
   //"misc/w_pkup.wav",
   NULL,
   "models/weapons/g_shotg/tris.md2",
   0,
   "models/weapons/v_shotg/tris.md2",
   "w_super90",
   M3_NAME,
   0,
   0,
   SHOTGUN_AMMO_NAME,
   IT_WEAPON,
   NULL,
   0,
   "weapons/m3in.wav weapons/shotgr1b.wav weapons/shotgf1b.wav",
  M3_NUM}
  ,
  {
   "weapon_HC",
   Pickup_Weapon,
   Use_Weapon,
   Drop_Weapon,
   Weapon_HC,
   //"misc/w_pkup.wav",
   NULL,
   "models/weapons/g_cannon/tris.md2",
   0,
   "models/weapons/v_cannon/tris.md2",
   "w_cannon",
   HC_NAME,
   0,
   0,
   SHOTGUN_AMMO_NAME,
   IT_WEAPON,
   NULL,
   0,
   "weapons/cannon_fire.wav weapons/sshotf1b.wav weapons/cclose.wav weapons/cin.wav weapons/cout.wav weapons/copen.wav",
  HC_NUM}
  ,
  {
   "weapon_Sniper",
   Pickup_Weapon,
   Use_Weapon,
   Drop_Weapon,
   Weapon_Sniper,
   //"misc/w_pkup.wav",
   NULL,
   "models/weapons/g_sniper/tris.md2",
   0,
   "models/weapons/v_sniper/tris.md2",
   "w_sniper",
   SNIPER_NAME,
   0,
   0,
   SNIPER_AMMO_NAME,
   IT_WEAPON,
   NULL,
   0,
   "weapons/ssgbolt.wav weapons/ssgfire.wav weapons/ssgin.wav misc/lensflik.wav weapons/hyprbl1a.wav weapons/hyprbf1a.wav",
  SNIPER_NUM}
  ,
  {
   "weapon_Dual",
   Pickup_Weapon,
   Use_Weapon,
   Drop_Weapon,
   Weapon_Dual,
   //"misc/w_pkup.wav",
   NULL,
   "models/weapons/g_dual/tris.md2",
   0,
   "models/weapons/v_dual/tris.md2",
   "w_akimbo",
   DUAL_NAME,
   0,
   0,
   MK23_AMMO_NAME,
   IT_WEAPON,
   NULL,
   0,
   "weapons/mk23fire.wav weapons/mk23in.wav weapons/mk23out.wav weapons/mk23slap.wav weapons/mk23slide.wav",
  DUAL_NUM}
  ,
  {
   "weapon_Knife",
   Pickup_Weapon,
   Use_Weapon,
   Drop_Weapon,
   Weapon_Knife,
   NULL,
   "models/objects/knife/tris.md2",
   0,
   "models/weapons/v_knife/tris.md2",
   "w_knife",
   KNIFE_NAME,
   0,
   0,
   NULL,
   IT_WEAPON,
   NULL,
   0,
   "weapons/throw.wav weapons/stab.wav weapons/swish.wav weapons/clank.wav",
  KNIFE_NUM}
  ,
  {
   "weapon_Grenade",
   Pickup_Weapon,
   Use_Weapon,
   Drop_Weapon,
   Weapon_Gas,
   NULL,
   "models/objects/grenade2/tris.md2",
   0,
   "models/weapons/v_handgr/tris.md2",
   "a_m61frag",
   GRENADE_NAME,
   0,
   0,
   NULL,
   IT_WEAPON,
   NULL,
   0,
   "misc/grenade.wav weapons/grenlb1b.wav weapons/hgrent1a.wav",
  GRENADE_NUM}
  ,

/* weapon_grapple (.3 .3 1) (-16 -16 -16) (16 16 16)^M

always owned, never in the world^M

*/
	{
		"weapon_grapple",
		NULL,
		Use_Weapon,
		NULL,
		CTFWeapon_Grapple,
		"misc/w_pkup.wav",
		NULL, 0,
		"models/weapons/grapple/tris.md2",
		/* icon */              "w_grapple",
		/* pickup */    "Grapple",
		0,
		0,
		NULL,
		IT_WEAPON,
		NULL,
		0,
		/* precache */ "weapons/grapple/grfire.wav weapons/grapple/grpull.wav weapons/grapple/grhang.wav weapons/grapple/grreset.wav weapons/grapple/grhit.wav",
		GRAPPLE_NUM
	},


  //
  // AMMO ITEMS
  //

// zucc new ammo
  {
   "ammo_clip",
   Pickup_Ammo,
   NULL,
   Drop_Ammo,
   NULL,
   //"misc/click.wav",
   NULL,
   "models/items/ammo/clip/tris.md2", 0,
   NULL,
/* icon */ "a_clip",
/* pickup */ MK23_AMMO_NAME,
/* width */ 3,
   1,
   NULL,
   IT_AMMO,
   NULL,
   AMMO_BULLETS,
/* precache */ "",
	MK23_ANUM
   }
  ,

  {
   "ammo_mag",
   Pickup_Ammo,
   NULL,
   Drop_Ammo,
   NULL,
   //"misc/click.wav",
   NULL,
   "models/items/ammo/mag/tris.md2", 0,
   NULL,
/* icon */ "a_mag",
/* pickup */ MP5_AMMO_NAME,
/* width */ 3,
   1,
   NULL,
   IT_AMMO,
   NULL,
   AMMO_ROCKETS,
/* precache */ "",
	MP5_ANUM
   }
  ,

  {
   "ammo_m4",
   Pickup_Ammo,
   NULL,
   Drop_Ammo,
   NULL,
   //"misc/click.wav",
   NULL,
   "models/items/ammo/m4/tris.md2", 0,
   NULL,
/* icon */ "a_m4",
/* pickup */ M4_AMMO_NAME,
/* width */ 3,
   1,
   NULL,
   IT_AMMO,
   NULL,
   AMMO_CELLS,
/* precache */ "",
	M4_ANUM
   }
  ,
  {
   "ammo_m3",
   Pickup_Ammo,
   NULL,
   Drop_Ammo,
   NULL,
   //"misc/click.wav",
   NULL,
   "models/items/ammo/shells/medium/tris.md2", 0,
   NULL,
/* icon */ "a_shells",
/* pickup */ SHOTGUN_AMMO_NAME,
/* width */ 3,
   7,
   NULL,
   IT_AMMO,
   NULL,
   AMMO_SHELLS,
/* precache */ "",
	SHELL_ANUM
   }
  ,
  {
   "ammo_sniper",
   Pickup_Ammo,
   NULL,
   Drop_Ammo,
   NULL,
   //"misc/click.wav",
   NULL,
   "models/items/ammo/sniper/tris.md2", 0,
   NULL,
/* icon */ "a_bullets",
/* pickup */ SNIPER_AMMO_NAME,
/* width */ 3,
   10,
   NULL,
   IT_AMMO,
   NULL,
   AMMO_SLUGS,
/* precache */ "",
	SNIPER_ANUM
   }
  ,



  //
  // POWERUP ITEMS
  //

  // zucc the main items
  {
   "item_quiet",
   Pickup_Special,
   NULL,
   Drop_Special,
   NULL,
   "misc/screw.wav",
   "models/items/quiet/tris.md2",
   0,
   NULL,
   /* icon */ "p_silencer",
   /* pickup */ SIL_NAME,
   /* width */ 2,
   60,
   NULL,
   IT_ITEM,
   NULL,
   0,
   /* precache */ "",
   SIL_NUM
   }
  ,

  {
   "item_slippers",
   Pickup_Special,
   NULL,
   Drop_Special,
   NULL,
   "misc/veston.wav",		// sound
   "models/items/slippers/slippers.md2",
   0,
   NULL,
/* icon */ "slippers",
/* pickup */ SLIP_NAME,
/* width */ 2,
   60,
   NULL,
   IT_ITEM,
   NULL,
   0,
   /* precache */ "",
   SLIP_NUM
   }
  ,

  {
   "item_band",
   Pickup_Special,
   NULL,
   Drop_Special,
   NULL,
   "misc/veston.wav",		// sound
   "models/items/band/tris.md2",
   0,
   NULL,
   /* icon */ "p_bandolier",
   /* pickup */ BAND_NAME,
   /* width */ 2,
   60,
   NULL,
   IT_ITEM,
   NULL,
   0,
   /* precache */ "",
   BAND_NUM
   }
  ,
  {
   "item_vest",
   Pickup_Special,
   NULL,
   Drop_Special,
   NULL,
   "misc/veston.wav",		// sound
   "models/items/armor/jacket/tris.md2",
   0,
   NULL,
   /* icon */ "i_jacketarmor",
   /* pickup */ KEV_NAME,
   /* width */ 2,
   60,
   NULL,
   IT_ITEM,
   NULL,
   0,
   /* precache */ "",
   KEV_NUM
   }
  ,
  {
   "item_lasersight",
   Pickup_Special,
   NULL,			//SP_LaserSight,
   Drop_Special,
   NULL,
   "misc/lasersight.wav",	// sound
   "models/items/laser/tris.md2",
   0,
   NULL,
   /* icon */ "p_laser",
   /* pickup */ LASER_NAME,
   /* width */ 2,
   60,
   NULL,
   IT_ITEM,
   NULL,
   0,
   /* precache */ "",
   LASER_NUM
   }
  ,
  {
   "item_helmet",
   Pickup_Special,
   NULL,
   Drop_Special,
   NULL,
   "misc/veston.wav",		// sound
   "models/items/breather/tris.md2",
   0,
   NULL,
   /* icon */ "p_rebreather",
   /* pickup */ HELM_NAME,
   /* width */ 2,
   60,
   NULL,
   IT_ITEM,
   NULL,
   0,
   /* precache */ "",
   HELM_NUM
   }
  ,

  {
   NULL,
   Pickup_Health,
   NULL,
   NULL,
   NULL,
   "items/pkup.wav",
   NULL, 0,
   NULL,
/* icon */ "i_health",
/* pickup */ "Health",
/* width */ 3,
   0,
   NULL,
   0,
   NULL,
   0,
/* precache */ "",
	NO_NUM
   }
  ,

  /*QUAKED item_flag_team1 (1 0.2 0) (-16 -16 -24) (16 16 32)
   */
  {
   "item_flag_team1",
   CTFPickup_Flag,
   NULL,
   CTFDrop_Flag,		//Should this be null if we don't want players to drop it manually?
   NULL,
   "tng/flagtk.wav",
   "models/flags/flag1.md2", EF_FLAG1,
   NULL,
   /* icon */ "i_ctf1",
   /* pickup */ "Red Flag",
   /* width */ 2,
   0,
   NULL,
   IT_FLAG,
   NULL,
   0,
   /* precache */ "tng/flagcap.wav tng/flagret.wav",
	FLAG_T1_NUM
   }
  ,

  /*QUAKED item_flag_team2 (1 0.2 0) (-16 -16 -24) (16 16 32)
   */
  {
   "item_flag_team2",
   CTFPickup_Flag,
   NULL,
   CTFDrop_Flag,		//Should this be null if we don't want players to drop it manually?
   NULL,
   "tng/flagtk.wav",
   "models/flags/flag2.md2", EF_FLAG2,
   NULL,
   /* icon */ "i_ctf2",
   /* pickup */ "Blue Flag",
   /* width */ 2,
   0,
   NULL,
   IT_FLAG,
   NULL,
   0,
   /* precache */ "tng/flagcap.wav tng/flagret.wav",
   FLAG_T2_NUM
   }
  ,

  // end of list marker
  {NULL}
};


/*QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
void SP_item_health (edict_t * self)
{
	if (1)	//DMFLAGS(DF_NO_HEALTH) )
	{
		G_FreeEdict(self);
		return;
	}

	self->model = "models/items/healing/medium/tris.md2";
	self->count = 10;
	SpawnItem(self, FindItem ("Health"));
	gi.soundindex("items/n_health.wav");
}

/*QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
void SP_item_health_small (edict_t * self)
{
	if (1)	//DMFLAGS(DF_NO_HEALTH) )
	{
		G_FreeEdict(self);
		return;
	}

	self->model = "models/items/healing/stimpack/tris.md2";
	self->count = 2;
	SpawnItem(self, FindItem ("Health"));
	self->style = HEALTH_IGNORE_MAX;
	gi.soundindex("items/s_health.wav");
}

/*QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
void SP_item_health_large (edict_t * self)
{
	if (1)	//DMFLAGS(DF_NO_HEALTH) )
	{
		G_FreeEdict(self);
		return;
	}

	self->model = "models/items/healing/large/tris.md2";
	self->count = 25;
	SpawnItem(self, FindItem ("Health"));
	gi.soundindex("items/l_health.wav");
}

/*QUAKED item_health_mega (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
void SP_item_health_mega (edict_t * self)
{
	if (1)	//DMFLAGS(DF_NO_HEALTH) )
	{
		G_FreeEdict(self);
		return;
	}

	self->model = "models/items/mega_h/tris.md2";
	self->count = 100;
	SpawnItem(self, FindItem ("Health"));
	gi.soundindex("items/m_health.wav");
	self->style = HEALTH_IGNORE_MAX | HEALTH_TIMED;
}


itemList_t items[ITEM_MAX_NUM];

void InitItems (void)
{
	int i;

	game.num_items = sizeof (itemlist) / sizeof (itemlist[0]) - 1;
	
	memset( items, 0, sizeof( items ) );
	for (i = 1; i < ITEM_MAX_NUM; i++) {
		items[i].index = ITEM_INDEX(FindItemByNum(i));
	}
	

	for (i = 0; i<WEAPON_COUNT; i++) {

		items[WEAPON_FIRST + i].flag = 1 << i;

	}
	for (i = 0; i<ITEM_COUNT; i++) {

		items[ITEM_FIRST + i].flag = 1 << i;

	}
	items[MK23_ANUM].flag = items[MK23_NUM].flag | items[DUAL_NUM].flag;
	items[MP5_ANUM].flag = items[MP5_NUM].flag;
	items[M4_ANUM].flag = items[M4_NUM].flag;
	items[SHELL_ANUM].flag = items[M3_NUM].flag | items[HC_NUM].flag;
	items[SNIPER_ANUM].flag = items[SNIPER_NUM].flag;
}



/*
===============
SetItemNames

Called by worldspawn
===============
*/
void SetItemNames (void)
{
	int i;
	gitem_t *it;

	for (i = 0; i < game.num_items; i++)
	{
		it = &itemlist[i];
		gi.configstring (CS_ITEMS + i, it->pickup_name);
	}

}
