// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"


qboolean	Pickup_Weapon (edict_t *ent, edict_t *other);
void		Use_Weapon (edict_t *ent, gitem_t *inv);
void		Drop_Weapon (edict_t *ent, gitem_t *inv);

void Weapon_Blaster (edict_t *ent);
void Weapon_Shotgun (edict_t *ent);
void Weapon_SuperShotgun (edict_t *ent);
void Weapon_Machinegun (edict_t *ent);
void Weapon_Chaingun (edict_t *ent);
void Weapon_HyperBlaster (edict_t *ent);
void Weapon_RocketLauncher (edict_t *ent);
void Weapon_Grenade (edict_t *ent);
void Weapon_GrenadeLauncher (edict_t *ent);
void Weapon_Railgun (edict_t *ent);
void Weapon_BFG (edict_t *ent);

//=========
//Rogue Weapons
void Weapon_ChainFist (edict_t *ent);
void Weapon_Disintegrator (edict_t *ent);
void Weapon_ETF_Rifle (edict_t *ent);
void Weapon_Heatbeam (edict_t *ent);
void Weapon_Prox (edict_t *ent);
void Weapon_Tesla (edict_t *ent);
void Weapon_ProxLauncher (edict_t *ent);
//void Weapon_Nuke (edict_t *ent);
//Rogue Weapons
//=========

gitem_armor_t jacketarmor_info	= { 25,  50, .30, .00, ARMOR_JACKET};
gitem_armor_t combatarmor_info	= { 50, 100, .60, .30, ARMOR_COMBAT};
gitem_armor_t bodyarmor_info	= {100, 200, .80, .60, ARMOR_BODY};

static int	jacket_armor_index;
static int	combat_armor_index;
static int	body_armor_index;
static int	power_screen_index;
static int	power_shield_index;

#define HEALTH_IGNORE_MAX	1
#define HEALTH_TIMED		2

void Use_Quad (edict_t *ent, gitem_t *item);
static int	quad_drop_timeout_hack;

//======================================================================

/*
===============
GetItemByIndex
===============
*/
gitem_t	*GetItemByIndex (int index)
{
	if (index == 0 || index >= game.num_items)
		return NULL;

	return &itemlist[index];
}


/*
===============
FindItemByClassname

===============
*/
gitem_t	*FindItemByClassname (char *classname)
{
	int		i;
	gitem_t	*it;

	it = itemlist;
	for (i=0 ; i<game.num_items ; i++, it++)
	{
		if (!it->classname)
			continue;
		if (!Q_stricmp(it->classname, classname))
			return it;
	}

	return NULL;
}

/*
===============
FindItem

===============
*/
gitem_t	*FindItem (char *pickup_name)
{
	int		i;
	gitem_t	*it;

	it = itemlist;
	for (i=0 ; i<game.num_items ; i++, it++)
	{
		if (!it->pickup_name)
			continue;
		if (!Q_stricmp(it->pickup_name, pickup_name))
			return it;
	}

	return NULL;
}

//======================================================================

void DoRespawn (edict_t *ent)
{
	if (ent->team)
	{
		edict_t	*master;
		int	count;
		int choice;

		master = ent->teammaster;

		for (count = 0, ent = master; ent; ent = ent->chain, count++)
			;

		choice = rand() % count;

		for (count = 0, ent = master; count < choice; ent = ent->chain, count++)
			;
	}

//=====
//ROGUE
	if(randomrespawn && randomrespawn->value)
	{
		edict_t *newEnt;

		newEnt = DoRandomRespawn (ent);
		
		// if we've changed entities, then do some sleight of hand.
		// otherwise, the old entity will respawn
		if(newEnt)
		{
			G_FreeEdict (ent);
			ent = newEnt;
		}
	}
//ROGUE
//=====

	ent->svflags &= ~SVF_NOCLIENT;
	ent->solid = SOLID_TRIGGER;
	gi.linkentity (ent);

	// send an effect
	ent->s.event = EV_ITEM_RESPAWN;
}

void SetRespawn (edict_t *ent, float delay)
{
	ent->flags |= FL_RESPAWN;
	ent->svflags |= SVF_NOCLIENT;
	ent->solid = SOLID_NOT;
	ent->nextthink = level.time + delay;
	ent->think = DoRespawn;
	gi.linkentity (ent);
}


//======================================================================

qboolean Pickup_Powerup (edict_t *ent, edict_t *other)
{
	int		quantity;

	quantity = other->client->pers.inventory[ITEM_INDEX(ent->item)];
	if ((skill->value == 1 && quantity >= 2) || (skill->value >= 2 && quantity >= 1))
		return false;

	if ((coop->value) && (ent->item->flags & IT_STAY_COOP) && (quantity > 0))
		return false;

	other->client->pers.inventory[ITEM_INDEX(ent->item)]++;

	if (deathmatch->value)
	{
		if (!(ent->spawnflags & DROPPED_ITEM) )
			SetRespawn (ent, ent->item->quantity);
		if (((int)dmflags->value & DF_INSTANT_ITEMS) || ((ent->item->use == Use_Quad) && (ent->spawnflags & DROPPED_PLAYER_ITEM)))
		{
			if ((ent->item->use == Use_Quad) && (ent->spawnflags & DROPPED_PLAYER_ITEM))
				quad_drop_timeout_hack = (ent->nextthink - level.time) / FRAMETIME;
//PGM
			if(ent->item->use)
				ent->item->use (other, ent->item);
			else
				gi.dprintf("Powerup has no use function!\n");
//PGM
		}
	}

	return true;
}

void Drop_General (edict_t *ent, gitem_t *item)
{
	Drop_Item (ent, item);
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);
}


//======================================================================

qboolean Pickup_Adrenaline (edict_t *ent, edict_t *other)
{
	if (!deathmatch->value)
		other->max_health += 1;

	if (other->health < other->max_health)
		other->health = other->max_health;

	if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SetRespawn (ent, ent->item->quantity);

	return true;
}

qboolean Pickup_AncientHead (edict_t *ent, edict_t *other)
{
	other->max_health += 2;

	if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SetRespawn (ent, ent->item->quantity);

	return true;
}

qboolean Pickup_Bandolier (edict_t *ent, edict_t *other)
{
	gitem_t	*item;
	int		index;

	if (other->client->pers.max_bullets < 250)
		other->client->pers.max_bullets = 250;
	if (other->client->pers.max_shells < 150)
		other->client->pers.max_shells = 150;
	if (other->client->pers.max_cells < 250)
		other->client->pers.max_cells = 250;
	if (other->client->pers.max_slugs < 75)
		other->client->pers.max_slugs = 75;
	//PMM
	if (other->client->pers.max_flechettes < 250)
		other->client->pers.max_flechettes = 250;
#ifndef KILL_DISRUPTOR
	if (other->client->pers.max_rounds < 150)
		other->client->pers.max_rounds = 150;
#endif
	//pmm

	item = FindItem("Bullets");
	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;
		if (other->client->pers.inventory[index] > other->client->pers.max_bullets)
			other->client->pers.inventory[index] = other->client->pers.max_bullets;
	}

	item = FindItem("Shells");
	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;
		if (other->client->pers.inventory[index] > other->client->pers.max_shells)
			other->client->pers.inventory[index] = other->client->pers.max_shells;
	}

	if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SetRespawn (ent, ent->item->quantity);

	return true;
}

qboolean Pickup_Pack (edict_t *ent, edict_t *other)
{
	gitem_t	*item;
	int		index;

	if (other->client->pers.max_bullets < 300)
		other->client->pers.max_bullets = 300;
	if (other->client->pers.max_shells < 200)
		other->client->pers.max_shells = 200;
	if (other->client->pers.max_rockets < 100)
		other->client->pers.max_rockets = 100;
	if (other->client->pers.max_grenades < 100)
		other->client->pers.max_grenades = 100;
	if (other->client->pers.max_cells < 300)
		other->client->pers.max_cells = 300;
	if (other->client->pers.max_slugs < 100)
		other->client->pers.max_slugs = 100;
	//PMM
	if (other->client->pers.max_flechettes < 200)
		other->client->pers.max_flechettes = 200;
#ifndef KILL_DISRUPTOR
	if (other->client->pers.max_rounds < 200)
		other->client->pers.max_rounds = 200;
#endif
	//pmm

	item = FindItem("Bullets");
	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;
		if (other->client->pers.inventory[index] > other->client->pers.max_bullets)
			other->client->pers.inventory[index] = other->client->pers.max_bullets;
	}

	item = FindItem("Shells");
	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;
		if (other->client->pers.inventory[index] > other->client->pers.max_shells)
			other->client->pers.inventory[index] = other->client->pers.max_shells;
	}

	item = FindItem("Cells");
	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;
		if (other->client->pers.inventory[index] > other->client->pers.max_cells)
			other->client->pers.inventory[index] = other->client->pers.max_cells;
	}

	item = FindItem("Grenades");
	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;
		if (other->client->pers.inventory[index] > other->client->pers.max_grenades)
			other->client->pers.inventory[index] = other->client->pers.max_grenades;
	}

	item = FindItem("Rockets");
	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;
		if (other->client->pers.inventory[index] > other->client->pers.max_rockets)
			other->client->pers.inventory[index] = other->client->pers.max_rockets;
	}

	item = FindItem("Slugs");
	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;
		if (other->client->pers.inventory[index] > other->client->pers.max_slugs)
			other->client->pers.inventory[index] = other->client->pers.max_slugs;
	}
//PMM
	item = FindItem("Flechettes");
	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;
		if (other->client->pers.inventory[index] > other->client->pers.max_flechettes)
			other->client->pers.inventory[index] = other->client->pers.max_flechettes;
	}
#ifndef KILL_DISRUPTOR
	item = FindItem("Rounds");
	if (item)
	{
		index = ITEM_INDEX(item);
		other->client->pers.inventory[index] += item->quantity;
		if (other->client->pers.inventory[index] > other->client->pers.max_rounds)
			other->client->pers.inventory[index] = other->client->pers.max_rounds;
	}
#endif
//pmm
	if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SetRespawn (ent, ent->item->quantity);

	return true;
}
// ================
// PMM
qboolean Pickup_Nuke (edict_t *ent, edict_t *other)
{
	int		quantity;

//	if (!deathmatch->value)
//		return;
	quantity = other->client->pers.inventory[ITEM_INDEX(ent->item)];
//	if ((skill->value == 1 && quantity >= 2) || (skill->value >= 2 && quantity >= 1))
//		return false;

	if (quantity >= 1)
		return false;

	if ((coop->value) && (ent->item->flags & IT_STAY_COOP) && (quantity > 0))
		return false;

	other->client->pers.inventory[ITEM_INDEX(ent->item)]++;

	if (deathmatch->value)
	{
		if (!(ent->spawnflags & DROPPED_ITEM) )
			SetRespawn (ent, ent->item->quantity);
	}

	return true;
}

// ================
// PGM
void Use_IR (edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	if (ent->client->ir_framenum > level.framenum)
		ent->client->ir_framenum += 600;
	else
		ent->client->ir_framenum = level.framenum + 600;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/ir_start.wav"), 1, ATTN_NORM, 0);
}

void Use_Double (edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	if (ent->client->double_framenum > level.framenum)
		ent->client->double_framenum += 300;
	else
		ent->client->double_framenum = level.framenum + 300;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/ddamage1.wav"), 1, ATTN_NORM, 0);
}

/*
void Use_Torch (edict_t *ent, gitem_t *item)
{
	ent->client->torch_framenum = level.framenum + 600;
}
*/

void Use_Compass (edict_t *ent, gitem_t *item)
{
	int ang;

	ang = (int)(ent->client->v_angle[1]);
	if(ang<0)
		ang += 360;

	gi.cprintf(ent, PRINT_HIGH, "Origin: %0.0f,%0.0f,%0.0f    Dir: %d\n", ent->s.origin[0], ent->s.origin[1],
				ent->s.origin[2], ang);
}

void Use_Nuke (edict_t *ent, gitem_t *item)
{
	vec3_t	forward, right, start;
	float	speed;

	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorCopy (ent->s.origin, start);
	speed = 100;
	fire_nuke (ent, start, forward, speed);
}

void Use_Doppleganger (edict_t *ent, gitem_t *item)
{
	vec3_t		forward, right;
	vec3_t		createPt, spawnPt;
	vec3_t		ang;

	VectorClear(ang);
	ang[YAW] = ent->client->v_angle[YAW];
	AngleVectors (ang, forward, right, NULL);

	VectorMA(ent->s.origin, 48, forward, createPt);

	if(!FindSpawnPoint(createPt, ent->mins, ent->maxs, spawnPt, 32))
		return;

	if(!CheckGroundSpawnPoint(spawnPt, ent->mins, ent->maxs, 64, -1))
		return;

	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	SpawnGrow_Spawn (spawnPt, 0);
	fire_doppleganger (ent, spawnPt, forward);
}

qboolean Pickup_Doppleganger (edict_t *ent, edict_t *other)
{
	int		quantity;

	if(!(deathmatch->value))		// item is DM only
		return false;

	quantity = other->client->pers.inventory[ITEM_INDEX(ent->item)];
	if (quantity >= 1)		// FIXME - apply max to dopplegangers
		return false;

	other->client->pers.inventory[ITEM_INDEX(ent->item)]++;

	if (!(ent->spawnflags & DROPPED_ITEM) )
		SetRespawn (ent, ent->item->quantity);

	return true;
}


qboolean Pickup_Sphere (edict_t *ent, edict_t *other)
{
	int		quantity;

	if(other->client && other->client->owned_sphere)
	{
//		gi.cprintf(other, PRINT_HIGH, "Only one sphere to a customer!\n");
		return false;
	}

	quantity = other->client->pers.inventory[ITEM_INDEX(ent->item)];
	if ((skill->value == 1 && quantity >= 2) || (skill->value >= 2 && quantity >= 1))
		return false;

	if ((coop->value) && (ent->item->flags & IT_STAY_COOP) && (quantity > 0))
		return false;

	other->client->pers.inventory[ITEM_INDEX(ent->item)]++;

	if (deathmatch->value)
	{
		if (!(ent->spawnflags & DROPPED_ITEM) )
			SetRespawn (ent, ent->item->quantity);
		if (((int)dmflags->value & DF_INSTANT_ITEMS))
		{
//PGM
			if(ent->item->use)
				ent->item->use (other, ent->item);
			else
				gi.dprintf("Powerup has no use function!\n");
//PGM
		}
	}

	return true;
}

void Use_Defender (edict_t *ent, gitem_t *item)
{
	if(ent->client && ent->client->owned_sphere)
	{
		gi.cprintf(ent, PRINT_HIGH, "Only one sphere at a time!\n");
		return;
	}

	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	Defender_Launch (ent);
}

void Use_Hunter (edict_t *ent, gitem_t *item)
{
	if(ent->client && ent->client->owned_sphere)
	{
		gi.cprintf(ent, PRINT_HIGH, "Only one sphere at a time!\n");
		return;
	}

	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	Hunter_Launch (ent);
}

void Use_Vengeance (edict_t *ent, gitem_t *item)
{
	if(ent->client && ent->client->owned_sphere)
	{
		gi.cprintf(ent, PRINT_HIGH, "Only one sphere at a time!\n");
		return;
	}

	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	Vengeance_Launch (ent);
}

// PGM
// ================


//======================================================================

void Use_Quad (edict_t *ent, gitem_t *item)
{
	int		timeout;

	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	if (quad_drop_timeout_hack)
	{
		timeout = quad_drop_timeout_hack;
		quad_drop_timeout_hack = 0;
	}
	else
	{
		timeout = 300;
	}

	if (ent->client->quad_framenum > level.framenum)
		ent->client->quad_framenum += timeout;
	else
		ent->client->quad_framenum = level.framenum + timeout;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void Use_Breather (edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	if (ent->client->breather_framenum > level.framenum)
		ent->client->breather_framenum += 300;
	else
		ent->client->breather_framenum = level.framenum + 300;

//	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void Use_Envirosuit (edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	if (ent->client->enviro_framenum > level.framenum)
		ent->client->enviro_framenum += 300;
	else
		ent->client->enviro_framenum = level.framenum + 300;

//	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void	Use_Invulnerability (edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	if (ent->client->invincible_framenum > level.framenum)
		ent->client->invincible_framenum += 300;
	else
		ent->client->invincible_framenum = level.framenum + 300;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void	Use_Silencer (edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);
	ent->client->silencer_shots += 30;

//	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

qboolean Pickup_Key (edict_t *ent, edict_t *other)
{
	if (coop->value)
	{
		if (strcmp(ent->classname, "key_power_cube") == 0)
		{
			if (other->client->pers.power_cubes & ((ent->spawnflags & 0x0000ff00)>> 8))
				return false;
			other->client->pers.inventory[ITEM_INDEX(ent->item)]++;
			other->client->pers.power_cubes |= ((ent->spawnflags & 0x0000ff00) >> 8);
		}
		else
		{
			if (other->client->pers.inventory[ITEM_INDEX(ent->item)])
				return false;
			other->client->pers.inventory[ITEM_INDEX(ent->item)] = 1;
		}
		return true;
	}
	other->client->pers.inventory[ITEM_INDEX(ent->item)]++;
	return true;
}

//======================================================================

qboolean Add_Ammo (edict_t *ent, gitem_t *item, int count)
{
	int			index;
	int			max;

	if (!ent->client)
		return false;

	if (item->tag == AMMO_BULLETS)
		max = ent->client->pers.max_bullets;
	else if (item->tag == AMMO_SHELLS)
		max = ent->client->pers.max_shells;
	else if (item->tag == AMMO_ROCKETS)
		max = ent->client->pers.max_rockets;
	else if (item->tag == AMMO_GRENADES)
		max = ent->client->pers.max_grenades;
	else if (item->tag == AMMO_CELLS)
		max = ent->client->pers.max_cells;
	else if (item->tag == AMMO_SLUGS)
		max = ent->client->pers.max_slugs;
// ROGUE
//	else if (item->tag == AMMO_MINES)
//		max = ent->client->pers.max_mines;
	else if (item->tag == AMMO_FLECHETTES)
		max = ent->client->pers.max_flechettes;
	else if (item->tag == AMMO_PROX)
		max = ent->client->pers.max_prox;
	else if (item->tag == AMMO_TESLA)
		max = ent->client->pers.max_tesla;
#ifndef KILL_DISRUPTOR
	else if (item->tag == AMMO_DISRUPTOR)
		max = ent->client->pers.max_rounds;
#endif
// ROGUE
	else
	{
		gi.dprintf("undefined ammo type\n");
		return false;
	}

	index = ITEM_INDEX(item);

	if (ent->client->pers.inventory[index] == max)
		return false;

	ent->client->pers.inventory[index] += count;

	if (ent->client->pers.inventory[index] > max)
		ent->client->pers.inventory[index] = max;

	return true;
}

qboolean Pickup_Ammo (edict_t *ent, edict_t *other)
{
	int			oldcount;
	int			count;
	qboolean	weapon;

	weapon = (ent->item->flags & IT_WEAPON);
	if ( (weapon) && ( (int)dmflags->value & DF_INFINITE_AMMO ) )
		count = 1000;
	else if (ent->count)
		count = ent->count;
	else
		count = ent->item->quantity;

	oldcount = other->client->pers.inventory[ITEM_INDEX(ent->item)];

	if (!Add_Ammo (other, ent->item, count))
		return false;

	if (weapon && !oldcount)
	{
		// don't switch to tesla
		if (other->client->pers.weapon != ent->item
				&& ( !deathmatch->value || other->client->pers.weapon == FindItem("blaster"))
				&& (strcmp(ent->classname, "ammo_tesla")) )

			other->client->newweapon = ent->item;
	}

	if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)) && (deathmatch->value))
		SetRespawn (ent, 30);
	return true;
}

void Drop_Ammo (edict_t *ent, gitem_t *item)
{
	edict_t	*dropped;
	int		index;

	index = ITEM_INDEX(item);
	dropped = Drop_Item (ent, item);
	if (ent->client->pers.inventory[index] >= item->quantity)
		dropped->count = item->quantity;
	else
		dropped->count = ent->client->pers.inventory[index];

	if (ent->client->pers.weapon && 
		ent->client->pers.weapon->tag == AMMO_GRENADES &&
		item->tag == AMMO_GRENADES &&
		ent->client->pers.inventory[index] - dropped->count <= 0) {
		gi.cprintf (ent, PRINT_HIGH, "Can't drop current weapon\n");
		G_FreeEdict(dropped);
		return;
	}

	ent->client->pers.inventory[index] -= dropped->count;
	ValidateSelectedItem (ent);
}


//======================================================================

void MegaHealth_think (edict_t *self)
{
	if (self->owner->health > self->owner->max_health)
	{
		self->nextthink = level.time + 1;
		self->owner->health -= 1;
		return;
	}

	if (!(self->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SetRespawn (self, 20);
	else
		G_FreeEdict (self);
}

qboolean Pickup_Health (edict_t *ent, edict_t *other)
{
	if (!(ent->style & HEALTH_IGNORE_MAX))
		if (other->health >= other->max_health)
			return false;

	other->health += ent->count;

	// PMM - health sound fix
	/*
	if (ent->count == 2)
		ent->item->pickup_sound = "items/s_health.wav";
	else if (ent->count == 10)
		ent->item->pickup_sound = "items/n_health.wav";
	else if (ent->count == 25)
		ent->item->pickup_sound = "items/l_health.wav";
	else // (ent->count == 100)
		ent->item->pickup_sound = "items/m_health.wav";
	*/

	if (!(ent->style & HEALTH_IGNORE_MAX))
	{
		if (other->health > other->max_health)
			other->health = other->max_health;
	}

	if (ent->style & HEALTH_TIMED)
	{
		ent->think = MegaHealth_think;
		ent->nextthink = level.time + 5;
		ent->owner = other;
		ent->flags |= FL_RESPAWN;
		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
	}
	else
	{
		if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->value))
			SetRespawn (ent, 30);
	}

	return true;
}

//======================================================================

int ArmorIndex (edict_t *ent)
{
	if (!ent->client)
		return 0;

	if (ent->client->pers.inventory[jacket_armor_index] > 0)
		return jacket_armor_index;

	if (ent->client->pers.inventory[combat_armor_index] > 0)
		return combat_armor_index;

	if (ent->client->pers.inventory[body_armor_index] > 0)
		return body_armor_index;

	return 0;
}

qboolean Pickup_Armor (edict_t *ent, edict_t *other)
{
	int				old_armor_index;
	gitem_armor_t	*oldinfo;
	gitem_armor_t	*newinfo;
	int				newcount;
	float			salvage;
	int				salvagecount;

	// get info on new armor
	newinfo = (gitem_armor_t *)ent->item->info;

	old_armor_index = ArmorIndex (other);

	// handle armor shards specially
	if (ent->item->tag == ARMOR_SHARD)
	{
		if (!old_armor_index)
			other->client->pers.inventory[jacket_armor_index] = 2;
		else
			other->client->pers.inventory[old_armor_index] += 2;
	}

	// if player has no armor, just use it
	else if (!old_armor_index)
	{
		other->client->pers.inventory[ITEM_INDEX(ent->item)] = newinfo->base_count;
	}

	// use the better armor
	else
	{
		// get info on old armor
		if (old_armor_index == jacket_armor_index)
			oldinfo = &jacketarmor_info;
		else if (old_armor_index == combat_armor_index)
			oldinfo = &combatarmor_info;
		else // (old_armor_index == body_armor_index)
			oldinfo = &bodyarmor_info;

		if (newinfo->normal_protection > oldinfo->normal_protection)
		{
			// calc new armor values
			salvage = oldinfo->normal_protection / newinfo->normal_protection;
			salvagecount = salvage * other->client->pers.inventory[old_armor_index];
			newcount = newinfo->base_count + salvagecount;
			if (newcount > newinfo->max_count)
				newcount = newinfo->max_count;

			// zero count of old armor so it goes away
			other->client->pers.inventory[old_armor_index] = 0;

			// change armor to new item with computed value
			other->client->pers.inventory[ITEM_INDEX(ent->item)] = newcount;
		}
		else
		{
			// calc new armor values
			salvage = newinfo->normal_protection / oldinfo->normal_protection;
			salvagecount = salvage * newinfo->base_count;
			newcount = other->client->pers.inventory[old_armor_index] + salvagecount;
			if (newcount > oldinfo->max_count)
				newcount = oldinfo->max_count;

			// if we're already maxed out then we don't need the new armor
			if (other->client->pers.inventory[old_armor_index] >= newcount)
				return false;

			// update current armor value
			other->client->pers.inventory[old_armor_index] = newcount;
		}
	}

	if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SetRespawn (ent, 20);

	return true;
}

//======================================================================

int PowerArmorType (edict_t *ent)
{
	if (!ent->client)
		return POWER_ARMOR_NONE;

	if (!(ent->flags & FL_POWER_ARMOR))
		return POWER_ARMOR_NONE;

	if (ent->client->pers.inventory[power_shield_index] > 0)
		return POWER_ARMOR_SHIELD;

	if (ent->client->pers.inventory[power_screen_index] > 0)
		return POWER_ARMOR_SCREEN;

	return POWER_ARMOR_NONE;
}

void Use_PowerArmor (edict_t *ent, gitem_t *item)
{
	int		index;

	if (ent->flags & FL_POWER_ARMOR)
	{
		ent->flags &= ~FL_POWER_ARMOR;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power2.wav"), 1, ATTN_NORM, 0);
	}
	else
	{
		index = ITEM_INDEX(FindItem("cells"));
		if (!ent->client->pers.inventory[index])
		{
			gi.cprintf (ent, PRINT_HIGH, "No cells for power armor.\n");
			return;
		}
		ent->flags |= FL_POWER_ARMOR;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power1.wav"), 1, ATTN_NORM, 0);
	}
}

qboolean Pickup_PowerArmor (edict_t *ent, edict_t *other)
{
	int		quantity;

	quantity = other->client->pers.inventory[ITEM_INDEX(ent->item)];

	other->client->pers.inventory[ITEM_INDEX(ent->item)]++;

	if (deathmatch->value)
	{
		if (!(ent->spawnflags & DROPPED_ITEM) )
			SetRespawn (ent, ent->item->quantity);
		// auto-use for DM only if we didn't already have one
		if (!quantity)
			ent->item->use (other, ent->item);
	}

	return true;
}

void Drop_PowerArmor (edict_t *ent, gitem_t *item)
{
	if ((ent->flags & FL_POWER_ARMOR) && (ent->client->pers.inventory[ITEM_INDEX(item)] == 1))
		Use_PowerArmor (ent, item);
	Drop_General (ent, item);
}

//======================================================================

/*
===============
Touch_Item
===============
*/
void Touch_Item (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	qboolean	taken;

	if (!other->client)
		return;
	if (other->health < 1)
		return;		// dead people can't pickup
	if (!ent->item->pickup)
		return;		// not a grabbable item?

	taken = ent->item->pickup(ent, other);

	if (taken)
	{
		// flash the screen
		other->client->bonus_alpha = 0.25;	

		// show icon and name on status bar
		other->client->ps.stats[STAT_PICKUP_ICON] = gi.imageindex(ent->item->icon);
		other->client->ps.stats[STAT_PICKUP_STRING] = CS_ITEMS+ITEM_INDEX(ent->item);
		other->client->pickup_msg_time = level.time + 3.0;

		// change selected item
		if (ent->item->use)
			other->client->pers.selected_item = other->client->ps.stats[STAT_SELECTED_ITEM] = ITEM_INDEX(ent->item);

		// PMM - health sound fix
		if (ent->item->pickup == Pickup_Health)
		{
			if (ent->count == 2)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/s_health.wav"), 1, ATTN_NORM, 0);
			else if (ent->count == 10)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/n_health.wav"), 1, ATTN_NORM, 0);
			else if (ent->count == 25)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/l_health.wav"), 1, ATTN_NORM, 0);
			else // (ent->count == 100)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/m_health.wav"), 1, ATTN_NORM, 0);
		}
		else if (ent->item->pickup_sound) // PGM - paranoia
		{
		//
			gi.sound(other, CHAN_ITEM, gi.soundindex(ent->item->pickup_sound), 1, ATTN_NORM, 0);
		}
	}

	if (!(ent->spawnflags & ITEM_TARGETS_USED))
	{
		G_UseTargets (ent, other);
		ent->spawnflags |= ITEM_TARGETS_USED;
	}

	if (!taken)
		return;

	if (!((coop->value) &&  (ent->item->flags & IT_STAY_COOP)) || (ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
	{
		if (ent->flags & FL_RESPAWN)
			ent->flags &= ~FL_RESPAWN;
		else
			G_FreeEdict (ent);
	}
}

//======================================================================

static void drop_temp_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent->owner)
		return;

	Touch_Item (ent, other, plane, surf);
}

static void drop_make_touchable (edict_t *ent)
{
	ent->touch = Touch_Item;
	if (deathmatch->value)
	{
		ent->nextthink = level.time + 29;
		ent->think = G_FreeEdict;
	}
}

edict_t *Drop_Item (edict_t *ent, gitem_t *item)
{
	edict_t	*dropped;
	vec3_t	forward, right;
	vec3_t	offset;

	dropped = G_Spawn();

	dropped->classname = item->classname;
	dropped->item = item;
	dropped->spawnflags = DROPPED_ITEM;
	dropped->s.effects = item->world_model_flags;
	dropped->s.renderfx = RF_GLOW | RF_IR_VISIBLE;		// PGM
	VectorSet (dropped->mins, -15, -15, -15);
	VectorSet (dropped->maxs, 15, 15, 15);
	gi.setmodel (dropped, dropped->item->world_model);
	dropped->solid = SOLID_TRIGGER;
	dropped->movetype = MOVETYPE_TOSS;  
	dropped->touch = drop_temp_touch;
	dropped->owner = ent;

	if (ent->client)
	{
		trace_t	trace;

		AngleVectors (ent->client->v_angle, forward, right, NULL);
		VectorSet(offset, 24, 0, -16);
		G_ProjectSource (ent->s.origin, offset, forward, right, dropped->s.origin);
		trace = gi.trace (ent->s.origin, dropped->mins, dropped->maxs,
			dropped->s.origin, ent, CONTENTS_SOLID);
		VectorCopy (trace.endpos, dropped->s.origin);
	}
	else
	{
		AngleVectors (ent->s.angles, forward, right, NULL);
		VectorCopy (ent->s.origin, dropped->s.origin);
	}

	VectorScale (forward, 100, dropped->velocity);
	dropped->velocity[2] = 300;

	dropped->think = drop_make_touchable;
	dropped->nextthink = level.time + 1;

	gi.linkentity (dropped);

	return dropped;
}

void Use_Item (edict_t *ent, edict_t *other, edict_t *activator)
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
void droptofloor (edict_t *ent)
{
	trace_t		tr;
	vec3_t		dest;
	float		*v;

	v = tv(-15,-15,-15);
	VectorCopy (v, ent->mins);
	v = tv(15,15,15);
	VectorCopy (v, ent->maxs);

	if (ent->model)
		gi.setmodel (ent, ent->model);
	else if (ent->item->world_model)	// PGM we shouldn't need this check, but paranoia...
		gi.setmodel (ent, ent->item->world_model);
	ent->solid = SOLID_TRIGGER;
	ent->movetype = MOVETYPE_TOSS;  
	ent->touch = Touch_Item;

	v = tv(0,0,-128);
	VectorAdd (ent->s.origin, v, dest);

	tr = gi.trace (ent->s.origin, ent->mins, ent->maxs, dest, ent, MASK_SOLID);
	if (tr.startsolid)
	{
		gi.dprintf ("droptofloor: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
		G_FreeEdict (ent);
		return;
	}

	VectorCopy (tr.endpos, ent->s.origin);

	if (ent->team)
	{
		ent->flags &= ~FL_TEAMSLAVE;
		ent->chain = ent->teamchain;
		ent->teamchain = NULL;

		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
		if (ent == ent->teammaster)
		{
			ent->nextthink = level.time + FRAMETIME;
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
void PrecacheItem (gitem_t *it)
{
	char	*s, *start;
	char	data[MAX_QPATH];
	int		len;
	gitem_t	*ammo;

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

		len = s-start;
		if (len >= MAX_QPATH || len < 5)
			gi.error ("PrecacheItem: %s has bad precache string", it->classname);
		memcpy (data, start, len);
		data[len] = 0;
		if (*s)
			s++;

		// determine type based on extension
		if (!strcmp(data+len-3, "md2"))
			gi.modelindex (data);
		else if (!strcmp(data+len-3, "sp2"))
			gi.modelindex (data);
		else if (!strcmp(data+len-3, "wav"))
			gi.soundindex (data);
		if (!strcmp(data+len-3, "pcx"))
			gi.imageindex (data);
	}
}


//=================
// Item_TriggeredSpawn - create the item marked for spawn creation
//=================
void Item_TriggeredSpawn (edict_t *self, edict_t *other, edict_t *activator)
{
//	self->nextthink = level.time + 2 * FRAMETIME;    // items start after other solids
//	self->think = droptofloor;
	self->svflags &= ~SVF_NOCLIENT;
	self->use = NULL;
	if(strcmp(self->classname, "key_power_cube"))	// leave them be on key_power_cube..
		self->spawnflags = 0;
	droptofloor (self);
}

//=================
// SetTriggeredSpawn - set up an item to spawn in later.
//=================
void SetTriggeredSpawn (edict_t *ent)
{
	// don't do anything on key_power_cubes.
	if(!strcmp(ent->classname, "key_power_cube"))
		return;

	ent->think = NULL;
	ent->nextthink = 0;
	ent->use = Item_TriggeredSpawn;
	ent->svflags |= SVF_NOCLIENT;
	ent->solid = SOLID_NOT;
}

/*
============
SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void SpawnItem (edict_t *ent, gitem_t *item)
{
#if KILL_DISRUPTOR
	if ((!strcmp(ent->classname, "ammo_disruptor")) || (!strcmp(ent->classname, "weapon_disintegrator")))
	{
		G_FreeEdict (ent);
		return;
	}
#endif

// PGM - since the item may be freed by the following rules, go ahead
//		 and move the precache until AFTER the following rules have been checked.
//		 keep an eye on this.
//	PrecacheItem (item);

	if (ent->spawnflags > 1)		// PGM
	{
		if (strcmp(ent->classname, "key_power_cube") != 0)
		{
			ent->spawnflags = 0;
			gi.dprintf("%s at %s has invalid spawnflags set\n", ent->classname, vtos(ent->s.origin));
		}
	}

	// some items will be prevented in deathmatch
	if (deathmatch->value)
	{
		if ( (int)dmflags->value & DF_NO_ARMOR )
		{
			if (item->pickup == Pickup_Armor || item->pickup == Pickup_PowerArmor)
			{
				G_FreeEdict (ent);
				return;
			}
		}
		if ( (int)dmflags->value & DF_NO_ITEMS )
		{
			if (item->pickup == Pickup_Powerup)
			{
				G_FreeEdict (ent);
				return;
			}
			//=====
			//ROGUE
			if (item->pickup  == Pickup_Sphere)
			{
				G_FreeEdict (ent);
				return;
			}
			if (item->pickup == Pickup_Doppleganger)
			{
				G_FreeEdict (ent);
				return;
			}
			//ROGUE
			//=====
		}
		if ( (int)dmflags->value & DF_NO_HEALTH )
		{
			if (item->pickup == Pickup_Health || item->pickup == Pickup_Adrenaline || item->pickup == Pickup_AncientHead)
			{
				G_FreeEdict (ent);
				return;
			}
		}
		if ( (int)dmflags->value & DF_INFINITE_AMMO )
		{
			if ( (item->flags == IT_AMMO) || (strcmp(ent->classname, "weapon_bfg") == 0) )
			{
				G_FreeEdict (ent);
				return;
			}
		}

//==========
//ROGUE
		if ( (int)dmflags->value & DF_NO_MINES )
		{
			if ( !strcmp(ent->classname, "ammo_prox") || 
				 !strcmp(ent->classname, "ammo_tesla") )
			{
				G_FreeEdict (ent);
				return;
			}
		}
		if ( (int)dmflags->value & DF_NO_NUKES )
		{
			if ( !strcmp(ent->classname, "ammo_nuke") )
			{
				G_FreeEdict (ent);
				return;
			}
		}
		if ( (int)dmflags->value & DF_NO_SPHERES )
		{
			if (item->pickup  == Pickup_Sphere)
			{
				G_FreeEdict (ent);
				return;
			}
		}
//ROGUE
//==========

	}
//==========
//ROGUE
// DM only items
	if (!deathmatch->value)
	{
		if (item->pickup == Pickup_Doppleganger || item->pickup == Pickup_Nuke)
		{
			G_FreeEdict (ent);
			return;
		}
		if ((item->use == Use_Vengeance) || (item->use == Use_Hunter))
		{
			G_FreeEdict (ent);
			return;
		}
	}
//ROGUE
//==========

//PGM 
	PrecacheItem (item);		
//PGM

	if (coop->value && (strcmp(ent->classname, "key_power_cube") == 0))
	{
		ent->spawnflags |= (1 << (8 + level.power_cubes));
		level.power_cubes++;
	}

	// don't let them drop items that stay in a coop game
	if ((coop->value) && (item->flags & IT_STAY_COOP))
	{
		item->drop = NULL;
	}

	ent->item = item;
	ent->nextthink = level.time + 2 * FRAMETIME;    // items start after other solids
	ent->think = droptofloor;
	ent->s.effects = item->world_model_flags;
	ent->s.renderfx = RF_GLOW;
	if (ent->model)
		gi.modelindex (ent->model);

	if (ent->spawnflags & 1)
		SetTriggeredSpawn (ent);
}

//======================================================================

gitem_t	itemlist[] = 
{
	{
		NULL
	},	// leave index 0 alone

	//
	// ARMOR
	//

/*QUAKED item_armor_body (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"item_armor_body", 
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar1_pkup.wav",
		"models/items/armor/body/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"i_bodyarmor",
/* pickup */	"Body Armor",
/* width */		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		&bodyarmor_info,
		ARMOR_BODY,
/* precache */ ""
	},

/*QUAKED item_armor_combat (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"item_armor_combat", 
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar1_pkup.wav",
		"models/items/armor/combat/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"i_combatarmor",
/* pickup */	"Combat Armor",
/* width */		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		&combatarmor_info,
		ARMOR_COMBAT,
/* precache */ ""
	},

/*QUAKED item_armor_jacket (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"item_armor_jacket", 
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar1_pkup.wav",
		"models/items/armor/jacket/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"i_jacketarmor",
/* pickup */	"Jacket Armor",
/* width */		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		&jacketarmor_info,
		ARMOR_JACKET,
/* precache */ ""
	},

/*QUAKED item_armor_shard (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"item_armor_shard", 
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar2_pkup.wav",
		"models/items/armor/shard/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"i_jacketarmor",
/* pickup */	"Armor Shard",
/* width */		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		NULL,
		ARMOR_SHARD,
/* precache */ ""
	},


/*QUAKED item_power_screen (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"item_power_screen", 
		Pickup_PowerArmor,
		Use_PowerArmor,
		Drop_PowerArmor,
		NULL,
		"misc/ar3_pkup.wav",
		"models/items/armor/screen/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"i_powerscreen",
/* pickup */	"Power Screen",
/* width */		0,
		60,
		NULL,
		IT_ARMOR,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED item_power_shield (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"item_power_shield",
		Pickup_PowerArmor,
		Use_PowerArmor,
		Drop_PowerArmor,
		NULL,
		"misc/ar3_pkup.wav",
		"models/items/armor/shield/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"i_powershield",
/* pickup */	"Power Shield",
/* width */		0,
		60,
		NULL,
		IT_ARMOR,
		0,
		NULL,
		0,
/* precache */ "misc/power2.wav misc/power1.wav"
	},


	//
	// WEAPONS 
	//

/* weapon_blaster (.3 .3 1) (-16 -16 -16) (16 16 16)
always owned, never in the world
*/
	{
		"weapon_blaster", 
		NULL,
		Use_Weapon,
		NULL,
		Weapon_Blaster,
		"misc/w_pkup.wav",
		NULL, 0,
		"models/weapons/v_blast/tris.md2",
/* icon */		"w_blaster",
/* pickup */	"Blaster",
		0,
		0,
		NULL,
		IT_WEAPON|IT_STAY_COOP,
		WEAP_BLASTER,
		NULL,
		0,
/* precache */ "weapons/blastf1a.wav misc/lasfly.wav"
	},

/*QUAKED weapon_shotgun (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"weapon_shotgun", 
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Shotgun,
		"misc/w_pkup.wav",
		"models/weapons/g_shotg/tris.md2", EF_ROTATE,
		"models/weapons/v_shotg/tris.md2",
/* icon */		"w_shotgun",
/* pickup */	"Shotgun",
		0,
		1,
		"Shells",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_SHOTGUN,
		NULL,
		0,
/* precache */ "weapons/shotgf1b.wav weapons/shotgr1b.wav"
	},

/*QUAKED weapon_supershotgun (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"weapon_supershotgun", 
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_SuperShotgun,
		"misc/w_pkup.wav",
		"models/weapons/g_shotg2/tris.md2", EF_ROTATE,
		"models/weapons/v_shotg2/tris.md2",
/* icon */		"w_sshotgun",
/* pickup */	"Super Shotgun",
		0,
		2,
		"Shells",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_SUPERSHOTGUN,
		NULL,
		0,
/* precache */ "weapons/sshotf1b.wav"
	},

/*QUAKED weapon_machinegun (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"weapon_machinegun", 
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Machinegun,
		"misc/w_pkup.wav",
		"models/weapons/g_machn/tris.md2", EF_ROTATE,
		"models/weapons/v_machn/tris.md2",
/* icon */		"w_machinegun",
/* pickup */	"Machinegun",
		0,
		1,
		"Bullets",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_MACHINEGUN,
		NULL,
		0,
/* precache */ "weapons/machgf1b.wav weapons/machgf2b.wav weapons/machgf3b.wav weapons/machgf4b.wav weapons/machgf5b.wav"
	},

/*QUAKED weapon_chaingun (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"weapon_chaingun", 
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Chaingun,
		"misc/w_pkup.wav",
		"models/weapons/g_chain/tris.md2", EF_ROTATE,
		"models/weapons/v_chain/tris.md2",
/* icon */		"w_chaingun",
/* pickup */	"Chaingun",
		0,
		1,
		"Bullets",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_CHAINGUN,
		NULL,
		0,
/* precache */ "weapons/chngnu1a.wav weapons/chngnl1a.wav weapons/machgf3b.wav` weapons/chngnd1a.wav"
	},

	// ROGUE
/*QUAKED weapon_etf_rifle (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"weapon_etf_rifle",									// classname
		Pickup_Weapon,										// pickup function
		Use_Weapon,											// use function
		Drop_Weapon,										// drop function
		Weapon_ETF_Rifle,									// weapon think function
		"misc/w_pkup.wav",									// pick up sound
		"models/weapons/g_etf_rifle/tris.md2", EF_ROTATE,		// world model, world model flags
		"models/weapons/v_etf_rifle/tris.md2",					// view model
		"w_etf_rifle",										// icon
		"ETF Rifle",										// name printed when picked up 
		0,													// number of digits for statusbar
		1,													// amount used / contained
		"Flechettes",										// ammo type used 
		IT_WEAPON,											// inventory flags
		WEAP_ETFRIFLE,										// visible weapon
		NULL,												// info (void *)
		0,													// tag
		"weapons/nail1.wav models/proj/flechette/tris.md2",	// precaches
	},

	// rogue
/*QUAKED ammo_grenades (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"ammo_grenades",
		Pickup_Ammo,
		Use_Weapon,
		Drop_Ammo,
		Weapon_Grenade,
		"misc/am_pkup.wav",
		"models/items/ammo/grenades/medium/tris.md2", 0,
		"models/weapons/v_handgr/tris.md2",
/* icon */		"a_grenades",
/* pickup */	"Grenades",
/* width */		3,
		5,
		"grenades",
		IT_AMMO|IT_WEAPON,
		WEAP_GRENADES,
		NULL,
		AMMO_GRENADES,
/* precache */ "weapons/hgrent1a.wav weapons/hgrena1b.wav weapons/hgrenc1b.wav weapons/hgrenb1a.wav weapons/hgrenb2a.wav "
	},

/*QUAKED weapon_grenadelauncher (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"weapon_grenadelauncher",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_GrenadeLauncher,
		"misc/w_pkup.wav",
		"models/weapons/g_launch/tris.md2", EF_ROTATE,
		"models/weapons/v_launch/tris.md2",
/* icon */		"w_glauncher",
/* pickup */	"Grenade Launcher",
		0,
		1,
		"Grenades",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_GRENADELAUNCHER,
		NULL,
		0,
/* precache */ "models/objects/grenade/tris.md2 weapons/grenlf1a.wav weapons/grenlr1b.wav weapons/grenlb1b.wav"
	},

	// ROGUE
/*QUAKED weapon_proxlauncher (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"weapon_proxlauncher",								// classname
		Pickup_Weapon,										// pickup
		Use_Weapon,											// use
		Drop_Weapon,										// drop
		Weapon_ProxLauncher,								// weapon think
		"misc/w_pkup.wav",									// pick up sound
		"models/weapons/g_plaunch/tris.md2", EF_ROTATE,		// world model, world model flags
		"models/weapons/v_plaunch/tris.md2",				// view model
		"w_proxlaunch",										// icon
		"Prox Launcher",									// name printed when picked up
		0,													// number of digits for statusbar
		1,													// amount used
		"Prox",												// ammo type used
		IT_WEAPON,											// inventory flags
		WEAP_PROXLAUNCH,									// visible weapon
		NULL,												// info (void *)
		AMMO_PROX,											// tag
		"weapons/grenlf1a.wav weapons/grenlr1b.wav weapons/grenlb1b.wav weapons/proxwarn.wav weapons/proxopen.wav",
	},
	// rogue

/*QUAKED weapon_rocketlauncher (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"weapon_rocketlauncher",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_RocketLauncher,
		"misc/w_pkup.wav",
		"models/weapons/g_rocket/tris.md2", EF_ROTATE,
		"models/weapons/v_rocket/tris.md2",
/* icon */		"w_rlauncher",
/* pickup */	"Rocket Launcher",
		0,
		1,
		"Rockets",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_ROCKETLAUNCHER,
		NULL,
		0,
/* precache */ "models/objects/rocket/tris.md2 weapons/rockfly.wav weapons/rocklf1a.wav weapons/rocklr1b.wav models/objects/debris2/tris.md2"
	},

/*QUAKED weapon_hyperblaster (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"weapon_hyperblaster", 
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_HyperBlaster,
		"misc/w_pkup.wav",
		"models/weapons/g_hyperb/tris.md2", EF_ROTATE,
		"models/weapons/v_hyperb/tris.md2",
/* icon */		"w_hyperblaster",
/* pickup */	"HyperBlaster",
		0,
		1,
		"Cells",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_HYPERBLASTER,
		NULL,
		0,
/* precache */ "weapons/hyprbu1a.wav weapons/hyprbl1a.wav weapons/hyprbf1a.wav weapons/hyprbd1a.wav misc/lasfly.wav"
	},

	// ROGUE
/*QUAKED weapon_plasmabeam (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/ 
	{
		"weapon_plasmabeam",								// classname
		Pickup_Weapon,										// pickup function
		Use_Weapon,											// use function
		Drop_Weapon,										// drop function
		Weapon_Heatbeam,									// weapon think function
		"misc/w_pkup.wav",									// pick up sound
		"models/weapons/g_beamer/tris.md2", EF_ROTATE,		// world model, world model flags
		"models/weapons/v_beamer/tris.md2",					// view model
		"w_heatbeam",											// icon
		"Plasma Beam",											// name printed when picked up 
		0,													// number of digits for statusbar
		// FIXME - if this changes, change it in NoAmmoWeaponChange as well
		2,													// amount used / contained
		"Cells",											// ammo type used 
		IT_WEAPON,											// inventory flags
		WEAP_PLASMA,										// visible weapon
		NULL,												// info (void *)
		0,													// tag
		"models/weapons/v_beamer2/tris.md2 weapons/bfg__l1a.wav",		// precaches
	},
	//rogue
/*QUAKED weapon_railgun (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"weapon_railgun", 
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Railgun,
		"misc/w_pkup.wav",
		"models/weapons/g_rail/tris.md2", EF_ROTATE,
		"models/weapons/v_rail/tris.md2",
/* icon */		"w_railgun",
/* pickup */	"Railgun",
		0,
		1,
		"Slugs",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_RAILGUN,
		NULL,
		0,
/* precache */ "weapons/rg_hum.wav"
	},

/*QUAKED weapon_bfg (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"weapon_bfg",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_BFG,
		"misc/w_pkup.wav",
		"models/weapons/g_bfg/tris.md2", EF_ROTATE,
		"models/weapons/v_bfg/tris.md2",
/* icon */		"w_bfg",
/* pickup */	"BFG10K",
		0,
		50,
		"Cells",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_BFG,
		NULL,
		0,
/* precache */ "sprites/s_bfg1.sp2 sprites/s_bfg2.sp2 sprites/s_bfg3.sp2 weapons/bfg__f1y.wav weapons/bfg__l1a.wav weapons/bfg__x1b.wav weapons/bfg_hum.wav"
	},

// =========================
// ROGUE WEAPONS
/*QUAKED weapon_chainfist (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"weapon_chainfist",									// classname
		Pickup_Weapon,										// pickup function
		Use_Weapon,											// use function
		Drop_Weapon,										// drop function
		Weapon_ChainFist,									// weapon think function
		"misc/w_pkup.wav",									// pick up sound
		"models/weapons/g_chainf/tris.md2", EF_ROTATE,		// world model, world model flags
		"models/weapons/v_chainf/tris.md2",					// view model
		"w_chainfist",										// icon
		"Chainfist",										// name printed when picked up 
		0,													// number of digits for statusbar
		0,													// amount used / contained
		NULL,												// ammo type used 
		IT_WEAPON | IT_MELEE,								// inventory flags
		WEAP_CHAINFIST,										// visible weapon
		NULL,												// info (void *)
		1,													// tag
		"weapons/sawidle.wav weapons/sawhit.wav",			// precaches
	},

/*QUAKED weapon_disintegrator (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"weapon_disintegrator",								// classname
		Pickup_Weapon,										// pickup function
		Use_Weapon,											// use function
		Drop_Weapon,										// drop function
		Weapon_Disintegrator,								// weapon think function
		"misc/w_pkup.wav",									// pick up sound
		"models/weapons/g_dist/tris.md2", EF_ROTATE,		// world model, world model flags
		"models/weapons/v_dist/tris.md2",					// view model
		"w_disintegrator",									// icon
		"Disruptor",										// name printed when picked up 
		0,													// number of digits for statusbar
		1,													// amount used / contained
		"Rounds",											// ammo type used 
#ifdef KILL_DISRUPTOR
		IT_NOT_GIVEABLE,
#else
		IT_WEAPON,											// inventory flags
#endif
		WEAP_DISRUPTOR,										// visible weapon
		NULL,												// info (void *)
		1,													// tag
		"models/items/spawngro/tris.md2 models/proj/disintegrator/tris.md2 weapons/disrupt.wav weapons/disint2.wav weapons/disrupthit.wav",	// precaches
	},

// ROGUE WEAPONS
// =========================


	//
	// AMMO ITEMS
	//

/*QUAKED ammo_shells (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"ammo_shells",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/shells/medium/tris.md2", 0,
		NULL,
/* icon */		"a_shells",
/* pickup */	"Shells",
/* width */		3,
		10,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_SHELLS,
/* precache */ ""
	},

/*QUAKED ammo_bullets (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"ammo_bullets",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/bullets/medium/tris.md2", 0,
		NULL,
/* icon */		"a_bullets",
/* pickup */	"Bullets",
/* width */		3,
		50,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_BULLETS,
/* precache */ ""
	},

/*QUAKED ammo_cells (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"ammo_cells",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/cells/medium/tris.md2", 0,
		NULL,
/* icon */		"a_cells",
/* pickup */	"Cells",
/* width */		3,
		50,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_CELLS,
/* precache */ ""
	},

/*QUAKED ammo_rockets (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"ammo_rockets",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/rockets/medium/tris.md2", 0,
		NULL,
/* icon */		"a_rockets",
/* pickup */	"Rockets",
/* width */		3,
		5,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_ROCKETS,
/* precache */ ""
	},

/*QUAKED ammo_slugs (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"ammo_slugs",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/slugs/medium/tris.md2", 0,
		NULL,
/* icon */		"a_slugs",
/* pickup */	"Slugs",
/* width */		3,
		10,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_SLUGS,
/* precache */ ""
	},

// =======================================
// ROGUE AMMO

/*QUAKED ammo_flechettes (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"ammo_flechettes",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/ammo/am_flechette/tris.md2", 0,
		NULL,
		"a_flechettes",
		"Flechettes",
		3,
		50,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_FLECHETTES
	},
/*QUAKED ammo_prox (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"ammo_prox",										// Classname
		Pickup_Ammo,										// pickup function
		NULL,												// use function
		Drop_Ammo,											// drop function
		NULL,												// weapon think function
		"misc/am_pkup.wav",									// pickup sound
		"models/ammo/am_prox/tris.md2", 0,					// world model, world model flags
		NULL,												// view model
		"a_prox",											// icon
		"Prox",												// Name printed when picked up
		3,													// number of digits for status bar
		5,													// amount contained
		NULL,												// ammo type used
		IT_AMMO,											// inventory flags
		0,													// vwep index
		NULL,												// info (void *)
		AMMO_PROX,											// tag
		"models/weapons/g_prox/tris.md2 weapons/proxwarn.wav"	// precaches
	},

/*QUAKED ammo_tesla (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"ammo_tesla",
		Pickup_Ammo,
		Use_Weapon,						// PGM
		Drop_Ammo,
		Weapon_Tesla,					// PGM
		"misc/am_pkup.wav",
//		"models/weapons/g_tesla/tris.md2", 0,
		"models/ammo/am_tesl/tris.md2", 0,
		"models/weapons/v_tesla/tris.md2",
		"a_tesla",
		"Tesla",
		3,
		5,
		"Tesla",												// PGM
		IT_AMMO | IT_WEAPON,						// inventory flags
		0,
		NULL,										// info (void *)
		AMMO_TESLA,									// tag
		"models/weapons/v_tesla2/tris.md2 weapons/teslaopen.wav weapons/hgrenb1a.wav weapons/hgrenb2a.wav models/weapons/g_tesla/tris.md2"			// precache
	},

/*QUAKED ammo_nuke (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"ammo_nuke",
		Pickup_Nuke,
		Use_Nuke,						// PMM
		Drop_Ammo,
		NULL,							// PMM
		"misc/am_pkup.wav",
		"models/weapons/g_nuke/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_nuke",
/* pickup */	"A-M Bomb",
/* width */		3,
		300, /* quantity (used for respawn time) */
		"A-M Bomb",
		IT_POWERUP,	
		0,
		NULL,
		0,
		"weapons/nukewarn2.wav world/rumble.wav"
	},

/*QUAKED ammo_disruptor (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"ammo_disruptor",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/ammo/am_disr/tris.md2", 0,
		NULL,
		"a_disruptor",
		"Rounds",		// FIXME 
		3,
		15,
		NULL,
#ifdef KILL_DISRUPTOR
		IT_NOT_GIVEABLE,
#else
		IT_AMMO,											// inventory flags
#endif
		0,
		NULL,
#ifdef KILL_DISRUPTOR
		0,
#else
		AMMO_DISRUPTOR,
#endif
	},
// ROGUE AMMO
// =======================================

	//
	// POWERUP ITEMS
	//
/*QUAKED item_quad (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"item_quad", 
		Pickup_Powerup,
		Use_Quad,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/quaddama/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_quad",
/* pickup */	"Quad Damage",
/* width */		2,
		60,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ "items/damage.wav items/damage2.wav items/damage3.wav"
	},

/*QUAKED item_invulnerability (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"item_invulnerability",
		Pickup_Powerup,
		Use_Invulnerability,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/invulner/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_invulnerability",
/* pickup */	"Invulnerability",
/* width */		2,
		300,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ "items/protect.wav items/protect2.wav items/protect4.wav"
	},

/*QUAKED item_silencer (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"item_silencer",
		Pickup_Powerup,
		Use_Silencer,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/silencer/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_silencer",
/* pickup */	"Silencer",
/* width */		2,
		60,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED item_breather (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"item_breather",
		Pickup_Powerup,
		Use_Breather,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/breather/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_rebreather",
/* pickup */	"Rebreather",
/* width */		2,
		60,
		NULL,
		IT_STAY_COOP|IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ "items/airout.wav"
	},

/*QUAKED item_enviro (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"item_enviro",
		Pickup_Powerup,
		Use_Envirosuit,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/enviro/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_envirosuit",
/* pickup */	"Environment Suit",
/* width */		2,
		60,
		NULL,
		IT_STAY_COOP|IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ "items/airout.wav"
	},

/*QUAKED item_ancient_head (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
Special item that gives +2 to maximum health
*/
	{
		"item_ancient_head",
		Pickup_AncientHead,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/items/c_head/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"i_fixme",
/* pickup */	"Ancient Head",
/* width */		2,
		60,
		NULL,
		0,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED item_adrenaline (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
gives +1 to maximum health
*/
	{
		"item_adrenaline",
		Pickup_Adrenaline,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/items/adrenal/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_adrenaline",
/* pickup */	"Adrenaline",
/* width */		2,
		60,
		NULL,
		0,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED item_bandolier (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"item_bandolier",
		Pickup_Bandolier,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/items/band/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_bandolier",
/* pickup */	"Bandolier",
/* width */		2,
		60,
		NULL,
		0,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED item_pack (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"item_pack",
		Pickup_Pack,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/items/pack/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"i_pack",
/* pickup */	"Ammo Pack",
/* width */		2,
		180,
		NULL,
		0,
		0,
		NULL,
		0,
/* precache */ ""
	},

// ======================================
// PGM

/*QUAKED item_ir_goggles (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
gives +1 to maximum health
*/
	{
		"item_ir_goggles",
		Pickup_Powerup,
		Use_IR,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/goggles/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_ir",
/* pickup */	"IR Goggles",
/* width */		2,
		60,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ "misc/ir_start.wav"
	},

/*QUAKED item_double (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"item_double", 
		Pickup_Powerup,
		Use_Double,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/ddamage/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_double",
/* pickup */	"Double Damage",
/* width */		2,
		60,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ "misc/ddamage1.wav misc/ddamage2.wav misc/ddamage3.wav"
	},

/*Q U A K E D item_torch (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
/*
	{
		"item_torch", 
		Pickup_Powerup,
		Use_Torch,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/objects/fire/tris.md2", EF_ROTATE,
		NULL,
		"p_torch",
		"torch",
		2,
		60,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
	},
*/

/*QUAKED item_compass (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"item_compass", 
		Pickup_Powerup,
		Use_Compass,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/objects/fire/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_compass",
/* pickup */	"compass",
/* width */		2,
		60,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
	},

/*QUAKED item_sphere_vengeance (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"item_sphere_vengeance", 
		Pickup_Sphere,
		Use_Vengeance,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/items/vengnce/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_vengeance",
/* pickup */	"vengeance sphere",
/* width */		2,
		60,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
		"spheres/v_idle.wav"		//precache
	},

/*QUAKED item_sphere_hunter (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"item_sphere_hunter", 
		Pickup_Sphere,
		Use_Hunter,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/items/hunter/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_hunter",
/* pickup */	"hunter sphere",
/* width */		2,
		120,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
		"spheres/h_idle.wav spheres/h_active.wav spheres/h_lurk.wav"		//precache
	},

/*QUAKED item_sphere_defender (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"item_sphere_defender", 
		Pickup_Sphere,
		Use_Defender,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/items/defender/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_defender",
/* pickup */	"defender sphere",
/* width */		2,
		60,													// respawn time
		NULL,												// ammo type used
		IT_POWERUP,											// inventory flags
		0,
		NULL,												// info (void *)
		0,													// tag
		"models/proj/laser2/tris.md2 models/items/shell/tris.md2 spheres/d_idle.wav"		// precache
	},

/*QUAKED item_doppleganger (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"item_doppleganger",								// classname
		Pickup_Doppleganger,								// pickup function
		Use_Doppleganger,									// use function
		Drop_General,										// drop function
		NULL,												// weapon think function
		"items/pkup.wav",									// pick up sound
		"models/items/dopple/tris.md2",						// world model
		EF_ROTATE,											// world model flags
		NULL,												// view model
		"p_doppleganger",									// icon
		"Doppleganger",										// name printed when picked up 
		0,													// number of digits for statusbar
		90,													// respawn time
		NULL,												// ammo type used 
		IT_POWERUP,											// inventory flags
		0,
		NULL,												// info (void *)
		0,													// tag
		"models/objects/dopplebase/tris.md2 models/items/spawngro2/tris.md2 models/items/hunter/tris.md2 models/items/vengnce/tris.md2",		// precaches
	},

	{
//		"dm_tag_token",										// classname
		NULL,												// classname
		Tag_PickupToken,									// pickup function
		NULL,												// use function
		NULL,												// drop function
		NULL,												// weapon think function
		"items/pkup.wav",									// pick up sound
		"models/items/tagtoken/tris.md2",					// world model
		EF_ROTATE | EF_TAGTRAIL,							// world model flags
		NULL,												// view model
		"i_tagtoken",										// icon
		"Tag Token",										// name printed when picked up 
		0,													// number of digits for statusbar
		0,													// amount used / contained
		NULL,												// ammo type used 
		IT_POWERUP | IT_NOT_GIVEABLE,						// inventory flags
		0,
		NULL,												// info (void *)
		1,													// tag
		NULL,												// precaches
	},

// PGM
// ======================================


	//
	// KEYS
	//
/*QUAKED key_data_cd (0 .5 .8) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
key for computer centers
*/
	{
		"key_data_cd",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/data_cd/tris.md2", EF_ROTATE,
		NULL,
		"k_datacd",
		"Data CD",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED key_power_cube (0 .5 .8) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN NO_TOUCH
warehouse circuits
*/
	{
		"key_power_cube",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/power/tris.md2", EF_ROTATE,
		NULL,
		"k_powercube",
		"Power Cube",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED key_pyramid (0 .5 .8) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
key for the entrance of jail3
*/
	{
		"key_pyramid",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/pyramid/tris.md2", EF_ROTATE,
		NULL,
		"k_pyramid",
		"Pyramid Key",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED key_data_spinner (0 .5 .8) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
key for the city computer
*/
	{
		"key_data_spinner",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/spinner/tris.md2", EF_ROTATE,
		NULL,
		"k_dataspin",
		"Data Spinner",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED key_pass (0 .5 .8) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
security pass for the security level
*/
	{
		"key_pass",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/pass/tris.md2", EF_ROTATE,
		NULL,
		"k_security",
		"Security Pass",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED key_blue_key (0 .5 .8) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
normal door key - blue
*/
	{
		"key_blue_key",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/key/tris.md2", EF_ROTATE,
		NULL,
		"k_bluekey",
		"Blue Key",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED key_red_key (0 .5 .8) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
normal door key - red
*/
	{
		"key_red_key",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/red_key/tris.md2", EF_ROTATE,
		NULL,
		"k_redkey",
		"Red Key",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED key_commander_head (0 .5 .8) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
tank commander's head
*/
	{
		"key_commander_head",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/monsters/commandr/head/tris.md2", EF_GIB,
		NULL,
/* icon */		"k_comhead",
/* pickup */	"Commander's Head",
/* width */		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED key_airstrike_target (0 .5 .8) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
tank commander's head
*/
	{
		"key_airstrike_target",
		Pickup_Key,
		NULL,
		Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/target/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"i_airstrike",
/* pickup */	"Airstrike Marker",
/* width */		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

// ======================================
// PGM

/*QUAKED key_nuke_container (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"key_nuke_container",								// classname
		Pickup_Key,											// pickup function
		NULL,												// use function
		Drop_General,										// drop function
		NULL,												// weapon think function
		"items/pkup.wav",									// pick up sound
		"models/weapons/g_nuke/tris.md2",					// world model
		EF_ROTATE,											// world model flags
		NULL,												// view model
		"i_contain",										// icon
		"Antimatter Pod",									// name printed when picked up 
		2,													// number of digits for statusbar
		0,													// respawn time
		NULL,												// ammo type used 
		IT_STAY_COOP|IT_KEY,								// inventory flags
		0,
		NULL,												// info (void *)
		0,													// tag
		NULL,												// precaches
	},

/*QUAKED key_nuke (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
	{
		"key_nuke",											// classname
		Pickup_Key,											// pickup function
		NULL,												// use function
		Drop_General,										// drop function
		NULL,												// weapon think function
		"items/pkup.wav",									// pick up sound
		"models/weapons/g_nuke/tris.md2",					// world model
		EF_ROTATE,											// world model flags
		NULL,												// view model
		"i_nuke",											// icon
		"Antimatter Bomb",									// name printed when picked up 
		2,													// number of digits for statusbar
		0,													// respawn time
		NULL,												// ammo type used 
		IT_STAY_COOP|IT_KEY,								// inventory flags
		0,
		NULL,												// info (void *)
		0,													// tag
		NULL,												// precaches
	},

// PGM
// ======================================

	{
		NULL,
		Pickup_Health,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		NULL, 0,
		NULL,
/* icon */		"i_health",
/* pickup */	"Health",
/* width */		3,
		0,
		NULL,
		0,
		0,
		NULL,
		0,
/* precache */ "items/s_health.wav items/n_health.wav items/l_health.wav items/m_health.wav"  // PMM - health sound fix
	},

	// end of list marker
	{NULL}
};


/*QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
void SP_item_health (edict_t *self)
{
	if ( deathmatch->value && ((int)dmflags->value & DF_NO_HEALTH) )
	{
		G_FreeEdict (self);
		return;
	}

	self->model = "models/items/healing/medium/tris.md2";
	self->count = 10;
	SpawnItem (self, FindItem ("Health"));
	gi.soundindex ("items/n_health.wav");
}

/*QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
void SP_item_health_small (edict_t *self)
{
	if ( deathmatch->value && ((int)dmflags->value & DF_NO_HEALTH) )
	{
		G_FreeEdict (self);
		return;
	}

	self->model = "models/items/healing/stimpack/tris.md2";
	self->count = 2;
	SpawnItem (self, FindItem ("Health"));
	self->style = HEALTH_IGNORE_MAX;
	gi.soundindex ("items/s_health.wav");
}

/*QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
void SP_item_health_large (edict_t *self)
{
	if ( deathmatch->value && ((int)dmflags->value & DF_NO_HEALTH) )
	{
		G_FreeEdict (self);
		return;
	}

	self->model = "models/items/healing/large/tris.md2";
	self->count = 25;
	SpawnItem (self, FindItem ("Health"));
	gi.soundindex ("items/l_health.wav");
}

/*QUAKED item_health_mega (.3 .3 1) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN
*/
void SP_item_health_mega (edict_t *self)
{
	if ( deathmatch->value && ((int)dmflags->value & DF_NO_HEALTH) )
	{
		G_FreeEdict (self);
		return;
	}

	self->model = "models/items/mega_h/tris.md2";
	self->count = 100;
	SpawnItem (self, FindItem ("Health"));
	gi.soundindex ("items/m_health.wav");
	self->style = HEALTH_IGNORE_MAX|HEALTH_TIMED;
}


void InitItems (void)
{
	game.num_items = sizeof(itemlist)/sizeof(itemlist[0]) - 1;
}



/*
===============
SetItemNames

Called by worldspawn
===============
*/
void SetItemNames (void)
{
	int		i;
	gitem_t	*it;

	for (i=0 ; i<game.num_items ; i++)
	{
		it = &itemlist[i];
		gi.configstring (CS_ITEMS+i, it->pickup_name);
	}

	jacket_armor_index = ITEM_INDEX(FindItem("Jacket Armor"));
	combat_armor_index = ITEM_INDEX(FindItem("Combat Armor"));
	body_armor_index   = ITEM_INDEX(FindItem("Body Armor"));
	power_screen_index = ITEM_INDEX(FindItem("Power Screen"));
	power_shield_index = ITEM_INDEX(FindItem("Power Shield"));
}


//===============
//ROGUE
void SP_xatrix_item (edict_t *self)
{
	gitem_t	*item;
	int		i;
	char	*spawnClass;

	if(!self->classname)
		return;

	if(!strcmp(self->classname, "ammo_magslug"))
		spawnClass = "ammo_flechettes";
	else if(!strcmp(self->classname, "ammo_trap"))
		spawnClass = "weapon_proxlauncher";
	else if(!strcmp(self->classname, "item_quadfire"))
	{
		float	chance;

		chance = random();
		if(chance < 0.2)
			spawnClass = "item_sphere_hunter";
		else if (chance < 0.6)
			spawnClass = "item_sphere_vengeance";
		else
			spawnClass = "item_sphere_defender";
	}
	else if(!strcmp(self->classname, "weapon_boomer"))
		spawnClass = "weapon_etf_rifle";
	else if(!strcmp(self->classname, "weapon_phalanx"))
		spawnClass = "weapon_plasmabeam";


	// check item spawn functions
	for (i=0,item=itemlist ; i<game.num_items ; i++,item++)
	{
		if (!item->classname)
			continue;
		if (!strcmp(item->classname, spawnClass))
		{	// found it
			SpawnItem (self, item);
			return;
		}
	}
}
//ROGUE
//===============
