// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_weapon.c

#include "g_local.h"
#include "m_player.h"


static qboolean	is_quad;
static byte		is_silenced;

//PGM
static byte		damage_multiplier;
//PGM

void weapon_grenade_fire (edict_t *ent, qboolean held);

//========
//ROGUE
byte P_DamageModifier(edict_t *ent)
{
	is_quad = 0;
	damage_multiplier = 1;

	if(ent->client->quad_framenum > level.framenum)
	{
		damage_multiplier *= 4;
		is_quad = 1;

		// if we're quad and DF_NO_STACK_DOUBLE is on, return now.
		if(((int)(dmflags->value) & DF_NO_STACK_DOUBLE))
			return damage_multiplier;
	}
	if(ent->client->double_framenum > level.framenum)
	{
		if ((deathmatch->value) || (damage_multiplier == 1))
		{
			damage_multiplier *= 2;
			is_quad = 1;
		}
	}
	
	return damage_multiplier;
}
//ROGUE
//========

static void P_ProjectSource (gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
{
	vec3_t	_distance;

	VectorCopy (distance, _distance);
	if (client->pers.hand == LEFT_HANDED)
		_distance[1] *= -1;
	else if (client->pers.hand == CENTER_HANDED)
		_distance[1] = 0;
	G_ProjectSource (point, _distance, forward, right, result);
}

static void P_ProjectSource2 (gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, 
							  vec3_t right, vec3_t up, vec3_t result)
{
	vec3_t	_distance;

	VectorCopy (distance, _distance);
	if (client->pers.hand == LEFT_HANDED)
		_distance[1] *= -1;
	else if (client->pers.hand == CENTER_HANDED)
		_distance[1] = 0;
	G_ProjectSource2 (point, _distance, forward, right, up, result);
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
void PlayerNoise(edict_t *who, vec3_t where, int type)
{
	edict_t		*noise;

	if (type == PNOISE_WEAPON)
	{
		if (who->client->silencer_shots)
		{
			who->client->silencer_shots--;
			return;
		}
	}

	if (deathmatch->value)
		return;

	if (who->flags & FL_NOTARGET)
		return;

	if (who->flags & FL_DISGUISED)
	{
		if (type == PNOISE_WEAPON)
		{
			level.disguise_violator = who;
			level.disguise_violation_framenum = level.framenum + 5;
		}
		else
			return;
	}

	if (!who->mynoise)
	{
		noise = G_Spawn();
		noise->classname = "player_noise";
		VectorSet (noise->mins, -8, -8, -8);
		VectorSet (noise->maxs, 8, 8, 8);
		noise->owner = who;
		noise->svflags = SVF_NOCLIENT;
		who->mynoise = noise;

		noise = G_Spawn();
		noise->classname = "player_noise";
		VectorSet (noise->mins, -8, -8, -8);
		VectorSet (noise->maxs, 8, 8, 8);
		noise->owner = who;
		noise->svflags = SVF_NOCLIENT;
		who->mynoise2 = noise;
	}

	if (type == PNOISE_SELF || type == PNOISE_WEAPON)
	{
		noise = who->mynoise;
		level.sound_entity = noise;
		level.sound_entity_framenum = level.framenum;
	}
	else // type == PNOISE_IMPACT
	{
		noise = who->mynoise2;
		level.sound2_entity = noise;
		level.sound2_entity_framenum = level.framenum;
	}

	VectorCopy (where, noise->s.origin);
	VectorSubtract (where, noise->maxs, noise->absmin);
	VectorAdd (where, noise->maxs, noise->absmax);
	noise->teleport_time = level.time;
	gi.linkentity (noise);
}


qboolean Pickup_Weapon (edict_t *ent, edict_t *other)
{
	int			index;
	gitem_t		*ammo;

	index = ITEM_INDEX(ent->item);

	if ( ( ((int)(dmflags->value) & DF_WEAPONS_STAY) || coop->value) 
		&& other->client->pers.inventory[index])
	{
		if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM) ) )
			return false;	// leave the weapon for others to pickup
	}

	other->client->pers.inventory[index]++;

	if (!(ent->spawnflags & DROPPED_ITEM) )
	{
		// give them some ammo with it
		// PGM -- IF APPROPRIATE!
		if(ent->item->ammo)			//PGM
		{
			ammo = FindItem (ent->item->ammo);
			if ( (int)dmflags->value & DF_INFINITE_AMMO )
				Add_Ammo (other, ammo, 1000);
			else
				Add_Ammo (other, ammo, ammo->quantity);
		}

		if (! (ent->spawnflags & DROPPED_PLAYER_ITEM) )
		{
			if (deathmatch->value)
			{
				if ((int)(dmflags->value) & DF_WEAPONS_STAY)
					ent->flags |= FL_RESPAWN;
				else
					SetRespawn (ent, 30);
			}
			if (coop->value)
				ent->flags |= FL_RESPAWN;
		}
	}

	if (other->client->pers.weapon != ent->item && 
		(other->client->pers.inventory[index] == 1) &&
		( !deathmatch->value || other->client->pers.weapon == FindItem("blaster") ) )
		other->client->newweapon = ent->item;

	return true;
}


/*
===============
ChangeWeapon

The old weapon has been dropped all the way, so make the new one
current
===============
*/
void ChangeWeapon (edict_t *ent)
{
	int i;

	if (ent->client->grenade_time)
	{
		ent->client->grenade_time = level.time;
		ent->client->weapon_sound = 0;
		weapon_grenade_fire (ent, false);
		ent->client->grenade_time = 0;
	}

	ent->client->pers.lastweapon = ent->client->pers.weapon;
	ent->client->pers.weapon = ent->client->newweapon;
	ent->client->newweapon = NULL;
	ent->client->machinegun_shots = 0;

	// set visible model
	if (ent->s.modelindex == 255)
	{
		if (ent->client->pers.weapon)
			i = ((ent->client->pers.weapon->weapmodel & 0xff) << 8);
		else
			i = 0;
		ent->s.skinnum = (ent - g_edicts - 1) | i;
	}

	if (ent->client->pers.weapon && ent->client->pers.weapon->ammo)
		ent->client->ammo_index = ITEM_INDEX(FindItem(ent->client->pers.weapon->ammo));
	else
		ent->client->ammo_index = 0;

	if (!ent->client->pers.weapon)
	{	// dead
		ent->client->ps.gunindex = 0;
		return;
	}

	ent->client->weaponstate = WEAPON_ACTIVATING;
	ent->client->ps.gunframe = 0;
	ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);

	ent->client->anim_priority = ANIM_PAIN;
	if(ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
			ent->s.frame = FRAME_crpain1;
			ent->client->anim_end = FRAME_crpain4;
	}
	else
	{
			ent->s.frame = FRAME_pain301;
			ent->client->anim_end = FRAME_pain304;
			
	}
}

/*
=================
NoAmmoWeaponChange
=================
*/

// PMM - added rogue weapons to the list

void NoAmmoWeaponChange (edict_t *ent)
{
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("slugs"))]
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("railgun"))] )
	{
		ent->client->newweapon = FindItem ("railgun");
		return;
	}
	// ROGUE
	if ( (ent->client->pers.inventory[ITEM_INDEX(FindItem("cells"))] >= 2)
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("Plasma Beam"))] )
	{
		ent->client->newweapon = FindItem ("Plasma Beam");
		return;
	}
	// -ROGUE
	/*
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("cells"))]
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("hyperblaster"))] )
	{
		ent->client->newweapon = FindItem ("hyperblaster");
		return;
	}
	*/
	// ROGUE
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("flechettes"))]
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("etf rifle"))] )
	{
		ent->client->newweapon = FindItem ("etf rifle");
		return;
	}
	// -ROGUE
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("bullets"))]
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("chaingun"))] )
	{
		ent->client->newweapon = FindItem ("chaingun");
		return;
	}
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("bullets"))]
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("machinegun"))] )
	{
		ent->client->newweapon = FindItem ("machinegun");
		return;
	}
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("shells"))] > 1
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("super shotgun"))] )
	{
		ent->client->newweapon = FindItem ("super shotgun");
		return;
	}
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("shells"))]
		&&  ent->client->pers.inventory[ITEM_INDEX(FindItem("shotgun"))] )
	{
		ent->client->newweapon = FindItem ("shotgun");
		return;
	}
	ent->client->newweapon = FindItem ("blaster");
}

/*
=================
Think_Weapon

Called by ClientBeginServerFrame and ClientThink
=================
*/
void Think_Weapon (edict_t *ent)
{
	// if just died, put the weapon away
	if (ent->health < 1)
	{
		ent->client->newweapon = NULL;
		ChangeWeapon (ent);
	}

	// call active weapon think routine
	if (ent->client->pers.weapon && ent->client->pers.weapon->weaponthink)
	{
//PGM
		P_DamageModifier(ent);	
//		is_quad = (ent->client->quad_framenum > level.framenum);
//PGM
		if (ent->client->silencer_shots)
			is_silenced = MZ_SILENCED;
		else
			is_silenced = 0;
		ent->client->pers.weapon->weaponthink (ent);
	}
}


/*
================
Use_Weapon

Make the weapon ready if there is ammo
================
*/
void Use_Weapon (edict_t *ent, gitem_t *item)
{
	int			ammo_index;
	gitem_t		*ammo_item;

	// see if we're already using it
	if (item == ent->client->pers.weapon)
		return;

	if (item->ammo && !g_select_empty->value && !(item->flags & IT_AMMO))
	{
		ammo_item = FindItem(item->ammo);
		ammo_index = ITEM_INDEX(ammo_item);

		if (!ent->client->pers.inventory[ammo_index])
		{
			gi.cprintf (ent, PRINT_HIGH, "No %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
			return;
		}

		if (ent->client->pers.inventory[ammo_index] < item->quantity)
		{
			gi.cprintf (ent, PRINT_HIGH, "Not enough %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
			return;
		}
	}

	// change to this weapon when down
	ent->client->newweapon = item;
}



/*
================
Drop_Weapon
================
*/
void Drop_Weapon (edict_t *ent, gitem_t *item)
{
	int		index;

	if ((int)(dmflags->value) & DF_WEAPONS_STAY)
		return;

	index = ITEM_INDEX(item);
	// see if we're already using it
	if ( ((item == ent->client->pers.weapon) || (item == ent->client->newweapon))&& (ent->client->pers.inventory[index] == 1) )
	{
		gi.cprintf (ent, PRINT_HIGH, "Can't drop current weapon\n");
		return;
	}

	Drop_Item (ent, item);
	ent->client->pers.inventory[index]--;
}


/*
================
Weapon_Generic

A generic function to handle the basics of weapon thinking
================
*/
#define FRAME_FIRE_FIRST		(FRAME_ACTIVATE_LAST + 1)
#define FRAME_IDLE_FIRST		(FRAME_FIRE_LAST + 1)
#define FRAME_DEACTIVATE_FIRST	(FRAME_IDLE_LAST + 1)

void Weapon_Generic (edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST, int *pause_frames, int *fire_frames, void (*fire)(edict_t *ent))
{
	int		n;

	if(ent->deadflag || ent->s.modelindex != 255) // VWep animations screw up corpses
	{
		return;
	}

	if (ent->client->weaponstate == WEAPON_DROPPING)
	{
		if (ent->client->ps.gunframe == FRAME_DEACTIVATE_LAST)
		{
			ChangeWeapon (ent);
			return;
		}
		else if ((FRAME_DEACTIVATE_LAST - ent->client->ps.gunframe) == 4)
		{
			ent->client->anim_priority = ANIM_REVERSE;
			if(ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent->s.frame = FRAME_crpain4+1;
				ent->client->anim_end = FRAME_crpain1;
			}
			else
			{
				ent->s.frame = FRAME_pain304+1;
				ent->client->anim_end = FRAME_pain301;
				
			}
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

		ent->client->ps.gunframe++;
		return;
	}

	if ((ent->client->newweapon) && (ent->client->weaponstate != WEAPON_FIRING))
	{
		ent->client->weaponstate = WEAPON_DROPPING;
		ent->client->ps.gunframe = FRAME_DEACTIVATE_FIRST;

		if ((FRAME_DEACTIVATE_LAST - FRAME_DEACTIVATE_FIRST) < 4)
		{
			ent->client->anim_priority = ANIM_REVERSE;
			if(ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent->s.frame = FRAME_crpain4+1;
				ent->client->anim_end = FRAME_crpain1;
			}
			else
			{
				ent->s.frame = FRAME_pain304+1;
				ent->client->anim_end = FRAME_pain301;
				
			}
		}
		return;
	}

	if (ent->client->weaponstate == WEAPON_READY)
	{
		if ( ((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK) )
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;
			if ((!ent->client->ammo_index) || 
				( ent->client->pers.inventory[ent->client->ammo_index] >= ent->client->pers.weapon->quantity))
			{
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING;

				// start the animation
				ent->client->anim_priority = ANIM_ATTACK;
				if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
				{
					ent->s.frame = FRAME_crattak1-1;
					ent->client->anim_end = FRAME_crattak9;
				}
				else
				{
					ent->s.frame = FRAME_attack1-1;
					ent->client->anim_end = FRAME_attack8;
				}
			}
			else
			{
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1;
				}
				NoAmmoWeaponChange (ent);
			}
		}
		else
		{
			if (ent->client->ps.gunframe == FRAME_IDLE_LAST)
			{
				ent->client->ps.gunframe = FRAME_IDLE_FIRST;
				return;
			}

			if (pause_frames)
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

			ent->client->ps.gunframe++;
			return;
		}
	}

	if (ent->client->weaponstate == WEAPON_FIRING)
	{
		for (n = 0; fire_frames[n]; n++)
		{
			if (ent->client->ps.gunframe == fire_frames[n])
			{
				// FIXME - double should use different sound
				if (ent->client->quad_framenum > level.framenum)
					gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);
				else if (ent->client->double_framenum > level.framenum)
					gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/ddamage3.wav"), 1, ATTN_NORM, 0);

				fire (ent);
				break;
			}
		}

		if (!fire_frames[n])
			ent->client->ps.gunframe++;

		if (ent->client->ps.gunframe == FRAME_IDLE_FIRST+1)
			ent->client->weaponstate = WEAPON_READY;
	}
}


/*
======================================================================

GRENADE

======================================================================
*/

#define GRENADE_TIMER		3.0
#define GRENADE_MINSPEED	400
#define GRENADE_MAXSPEED	800

void weapon_grenade_fire (edict_t *ent, qboolean held)
{
	vec3_t	offset;
	vec3_t	forward, right, up;
	vec3_t	start;
	int		damage = 125;
	float	timer;
	int		speed;
	float	radius;

	radius = damage+40;
	if (is_quad)
//		damage *= 4;
		damage *= damage_multiplier;		// PGM

	AngleVectors (ent->client->v_angle, forward, right, up);
	if (ent->client->pers.weapon->tag == AMMO_TESLA)
	{
//		VectorSet(offset, 0, -12, ent->viewheight-26);
		VectorSet(offset, 0, -4, ent->viewheight-22);
	}
	else
	{
//		VectorSet(offset, 8, 8, ent->viewheight-8);
		VectorSet(offset, 2, 6, ent->viewheight-14);
	}
	P_ProjectSource2 (ent->client, ent->s.origin, offset, forward, right, up, start);

	timer = ent->client->grenade_time - level.time;
	speed = GRENADE_MINSPEED + (GRENADE_TIMER - timer) * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER);
	if (speed > GRENADE_MAXSPEED)
		speed = GRENADE_MAXSPEED;

//	fire_grenade2 (ent, start, forward, damage, speed, timer, radius, held);

// ============
// PGM
	switch(ent->client->pers.weapon->tag)
	{
		case AMMO_GRENADES:
			fire_grenade2 (ent, start, forward, damage, speed, timer, radius, held);
			break;
		case AMMO_TESLA:
			fire_tesla (ent, start, forward, damage_multiplier, speed);
			break;
		default:
			fire_prox (ent, start, forward, damage_multiplier, speed);
			break;
	}
// PGM
// ============

	if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
		ent->client->pers.inventory[ent->client->ammo_index]--;

	ent->client->grenade_time = level.time + 1.0;

	if(ent->deadflag || ent->s.modelindex != 255) // VWep animations screw up corpses
	{
		return;
	}

	if (ent->health <= 0)
		return;

	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->client->anim_priority = ANIM_ATTACK;
		ent->s.frame = FRAME_crattak1-1;
		ent->client->anim_end = FRAME_crattak3;
	}
	else
	{
		ent->client->anim_priority = ANIM_REVERSE;
		ent->s.frame = FRAME_wave08;
		ent->client->anim_end = FRAME_wave01;
	}
}
/*
void Weapon_Grenade (edict_t *ent)
{
	if ((ent->client->newweapon) && (ent->client->weaponstate == WEAPON_READY))
	{
		ChangeWeapon (ent);
		return;
	}

	if (ent->client->weaponstate == WEAPON_ACTIVATING)
	{
		ent->client->weaponstate = WEAPON_READY;
		ent->client->ps.gunframe = 16;
		return;
	}

	if (ent->client->weaponstate == WEAPON_READY)
	{
		if ( ((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK) )
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;
			if (ent->client->pers.inventory[ent->client->ammo_index])
			{
				ent->client->ps.gunframe = 1;
				ent->client->weaponstate = WEAPON_FIRING;
				ent->client->grenade_time = 0;
			}
			else
			{
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1;
				}
				NoAmmoWeaponChange (ent);
			}
			return;
		}

		if ((ent->client->ps.gunframe == 29) || (ent->client->ps.gunframe == 34) || (ent->client->ps.gunframe == 39) || (ent->client->ps.gunframe == 48))
		{
			if (rand()&15)
				return;
		}

		if (++ent->client->ps.gunframe > 48)
			ent->client->ps.gunframe = 16;
		return;
	}

	if (ent->client->weaponstate == WEAPON_FIRING)
	{
		if (ent->client->ps.gunframe == 5)
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/hgrena1b.wav"), 1, ATTN_NORM, 0);

		if (ent->client->ps.gunframe == 11)
		{
			if (!ent->client->grenade_time)
			{
				ent->client->grenade_time = level.time + GRENADE_TIMER + 0.2;
				ent->client->weapon_sound = gi.soundindex("weapons/hgrenc1b.wav");
			}

			// they waited too long, detonate it in their hand
			if (!ent->client->grenade_blew_up && level.time >= ent->client->grenade_time)
			{
				ent->client->weapon_sound = 0;
				weapon_grenade_fire (ent, true);
				ent->client->grenade_blew_up = true;
			}

			if (ent->client->buttons & BUTTON_ATTACK)
				return;

			if (ent->client->grenade_blew_up)
			{
				if (level.time >= ent->client->grenade_time)
				{
					ent->client->ps.gunframe = 15;
					ent->client->grenade_blew_up = false;
				}
				else
				{
					return;
				}
			}
		}

		if (ent->client->ps.gunframe == 12)
		{
			ent->client->weapon_sound = 0;
			weapon_grenade_fire (ent, false);
		}

		if ((ent->client->ps.gunframe == 15) && (level.time < ent->client->grenade_time))
			return;

		ent->client->ps.gunframe++;

		if (ent->client->ps.gunframe == 16)
		{
			ent->client->grenade_time = 0;
			ent->client->weaponstate = WEAPON_READY;
		}
	}
}
*/

#define FRAME_IDLE_FIRST		(FRAME_FIRE_LAST + 1)

//void Weapon_Generic (edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST, int *pause_frames, int *fire_frames, void (*fire)(edict_t *ent))
//									15                      48						5						11					12				29,34,39,48
void Throw_Generic (edict_t *ent, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_THROW_SOUND,
					int FRAME_THROW_HOLD, int FRAME_THROW_FIRE, int *pause_frames, int EXPLODE,
					void (*fire)(edict_t *ent, qboolean held))
{
	int n;

	if ((ent->client->newweapon) && (ent->client->weaponstate == WEAPON_READY))
	{
		ChangeWeapon (ent);
		return;
	}

	if (ent->client->weaponstate == WEAPON_ACTIVATING)
	{
		ent->client->weaponstate = WEAPON_READY;
		ent->client->ps.gunframe = FRAME_IDLE_FIRST;
		return;
	}

	if (ent->client->weaponstate == WEAPON_READY)
	{
		if ( ((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK) )
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;
			if (ent->client->pers.inventory[ent->client->ammo_index])
			{
				ent->client->ps.gunframe = 1;
				ent->client->weaponstate = WEAPON_FIRING;
				ent->client->grenade_time = 0;
			}
			else
			{
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1;
				}
				NoAmmoWeaponChange (ent);
			}
			return;
		}

		if (ent->client->ps.gunframe == FRAME_IDLE_LAST)
		{
			ent->client->ps.gunframe = FRAME_IDLE_FIRST;
			return;
		}

		if (pause_frames)
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

		ent->client->ps.gunframe++;
		return;
	}

	if (ent->client->weaponstate == WEAPON_FIRING)
	{
		if (ent->client->ps.gunframe == FRAME_THROW_SOUND)
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/hgrena1b.wav"), 1, ATTN_NORM, 0);

		if (ent->client->ps.gunframe == FRAME_THROW_HOLD)
		{
			if (!ent->client->grenade_time)
			{
				ent->client->grenade_time = level.time + GRENADE_TIMER + 0.2;
				switch(ent->client->pers.weapon->tag)
				{
					case AMMO_GRENADES:
						ent->client->weapon_sound = gi.soundindex("weapons/hgrenc1b.wav");
						break;
				}
			}

			// they waited too long, detonate it in their hand
			if (EXPLODE && !ent->client->grenade_blew_up && level.time >= ent->client->grenade_time)
			{
				ent->client->weapon_sound = 0;
				fire (ent, true);
				ent->client->grenade_blew_up = true;
			}

			if (ent->client->buttons & BUTTON_ATTACK)
				return;

			if (ent->client->grenade_blew_up)
			{
				if (level.time >= ent->client->grenade_time)
				{
					ent->client->ps.gunframe = FRAME_FIRE_LAST;
					ent->client->grenade_blew_up = false;
				}
				else
				{
					return;
				}
			}
		}

		if (ent->client->ps.gunframe == FRAME_THROW_FIRE)
		{
			ent->client->weapon_sound = 0;
			fire (ent, true);
		}

		if ((ent->client->ps.gunframe == FRAME_FIRE_LAST) && (level.time < ent->client->grenade_time))
			return;

		ent->client->ps.gunframe++;

		if (ent->client->ps.gunframe == FRAME_IDLE_FIRST)
		{
			ent->client->grenade_time = 0;
			ent->client->weaponstate = WEAPON_READY;
		}
	}
}

//void Throw_Generic (edict_t *ent, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_THROW_SOUND,
//						int FRAME_THROW_HOLD, int FRAME_THROW_FIRE, int *pause_frames, 
//						int EXPLOSION_TIME, void (*fire)(edict_t *ent))

void Weapon_Grenade (edict_t *ent)
{
	static int	pause_frames[]	= {29,34,39,48,0};

	Throw_Generic (ent, 15, 48, 5, 11, 12, pause_frames, GRENADE_TIMER, weapon_grenade_fire);
}

void Weapon_Prox (edict_t *ent)
{
	static int	pause_frames[]	= {22, 29, 0};

	Throw_Generic (ent, 7, 27, 99, 2, 4, pause_frames, 0, weapon_grenade_fire);
}

void Weapon_Tesla (edict_t *ent)
{
	static int	pause_frames[]	= {21, 0};

	if ((ent->client->ps.gunframe > 1) && (ent->client->ps.gunframe < 9))
	{
		ent->client->ps.gunindex = gi.modelindex  ("models/weapons/v_tesla2/tris.md2");
	}
	else
	{
		ent->client->ps.gunindex = gi.modelindex  ("models/weapons/v_tesla/tris.md2");
	}

	Throw_Generic (ent, 8, 32, 99, 1, 2, pause_frames, 0, weapon_grenade_fire);
}



/*
======================================================================

GRENADE LAUNCHER

======================================================================
*/

void weapon_grenadelauncher_fire (edict_t *ent)
{
	vec3_t	offset;
	vec3_t	forward, right;
	vec3_t	start;
//	int		damage = 120;
	int		damage;			// PGM
	float	radius;

// =====
// PGM
	switch(ent->client->pers.weapon->tag)
	{
		case AMMO_PROX:
			damage = 90;
			break;
		default:
			damage = 120;
			break;
	}
// PGM
// =====

	radius = damage+40;
	if (is_quad)
//		damage *= 4;
		damage *= damage_multiplier;		//pgm

	VectorSet(offset, 8, 8, ent->viewheight-8);
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

//	fire_grenade (ent, start, forward, damage, 600, 2.5, radius);
// =====
// PGM
	switch(ent->client->pers.weapon->tag)
	{
		case AMMO_PROX:
			fire_prox (ent, start, forward, damage_multiplier, 600);
			break;
		default:
			fire_grenade (ent, start, forward, damage, 600, 2.5, radius);
			break;
	}
// PGM
// =====

	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_GRENADE | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
		ent->client->pers.inventory[ent->client->ammo_index]--;
}

void Weapon_GrenadeLauncher (edict_t *ent)
{
	static int	pause_frames[]	= {34, 51, 59, 0};
	static int	fire_frames[]	= {6, 0};

	Weapon_Generic (ent, 5, 16, 59, 64, pause_frames, fire_frames, weapon_grenadelauncher_fire);
}

//==========
//PGM
void Weapon_ProxLauncher (edict_t *ent)
{
	static int      pause_frames[]  = {34, 51, 59, 0};
	static int      fire_frames[]   = {6, 0};

	Weapon_Generic (ent, 5, 16, 59, 64, pause_frames, fire_frames, weapon_grenadelauncher_fire);
}
//PGM
//==========

/*
======================================================================

ROCKET

======================================================================
*/

void Weapon_RocketLauncher_Fire (edict_t *ent)
{
	vec3_t	offset, start;
	vec3_t	forward, right;
	int		damage;
	float	damage_radius;
	int		radius_damage;

	damage = 100 + (int)(random() * 20.0);
	radius_damage = 120;
	damage_radius = 120;
	if (is_quad)
	{
//PGM
//		damage *= 4;
		damage *= damage_multiplier;
//		radius_damage *= 4;
		radius_damage *= damage_multiplier;
//PGM
	}

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	VectorSet(offset, 8, 8, ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
	fire_rocket (ent, start, forward, damage, 650, damage_radius, radius_damage);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_ROCKET | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
		ent->client->pers.inventory[ent->client->ammo_index]--;
}

void Weapon_RocketLauncher (edict_t *ent)
{
	static int	pause_frames[]	= {25, 33, 42, 50, 0};
	static int	fire_frames[]	= {5, 0};

	Weapon_Generic (ent, 4, 12, 50, 54, pause_frames, fire_frames, Weapon_RocketLauncher_Fire);
}


/*
======================================================================

BLASTER / HYPERBLASTER

======================================================================
*/

void Blaster_Fire (edict_t *ent, vec3_t g_offset, int damage, qboolean hyper, int effect)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	offset;

	if (is_quad)
//		damage *= 4;		
		damage *= damage_multiplier;		//pgm

	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 24, 8, ent->viewheight-8);
	VectorAdd (offset, g_offset, offset);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	fire_blaster (ent, start, forward, damage, 1000, effect, hyper);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	if (hyper)
		gi.WriteByte (MZ_HYPERBLASTER | is_silenced);
	else
		gi.WriteByte (MZ_BLASTER | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);
}


void Weapon_Blaster_Fire (edict_t *ent)
{
	int		damage;

	if (deathmatch->value)
		damage = 15;
	else
		damage = 10;
	Blaster_Fire (ent, vec3_origin, damage, false, EF_BLASTER);
	ent->client->ps.gunframe++;
}

void Weapon_Blaster (edict_t *ent)
{
	static int	pause_frames[]	= {19, 32, 0};
	static int	fire_frames[]	= {5, 0};

	Weapon_Generic (ent, 4, 8, 52, 55, pause_frames, fire_frames, Weapon_Blaster_Fire);
}


void Weapon_HyperBlaster_Fire (edict_t *ent)
{
	float	rotation;
	vec3_t	offset;
	int		effect;
	int		damage;

	ent->client->weapon_sound = gi.soundindex("weapons/hyprbl1a.wav");

	if (!(ent->client->buttons & BUTTON_ATTACK))
	{
		ent->client->ps.gunframe++;
	}
	else
	{
		if (! ent->client->pers.inventory[ent->client->ammo_index] )
		{
			if (level.time >= ent->pain_debounce_time)
			{
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
				ent->pain_debounce_time = level.time + 1;
			}
			NoAmmoWeaponChange (ent);
		}
		else
		{
			rotation = (ent->client->ps.gunframe - 5) * 2*M_PI/6;
			offset[0] = -4 * sin(rotation);
			offset[1] = 0;
			offset[2] = 4 * cos(rotation);

			if ((ent->client->ps.gunframe == 6) || (ent->client->ps.gunframe == 9))
				effect = EF_HYPERBLASTER;
			else
				effect = 0;
			if (deathmatch->value)
				damage = 15;
			else
				damage = 20;
			Blaster_Fire (ent, offset, damage, true, effect);
			if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
				ent->client->pers.inventory[ent->client->ammo_index]--;

			ent->client->anim_priority = ANIM_ATTACK;
			if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent->s.frame = FRAME_crattak1 - 1;
				ent->client->anim_end = FRAME_crattak9;
			}
			else
			{
				ent->s.frame = FRAME_attack1 - 1;
				ent->client->anim_end = FRAME_attack8;
			}
		}

		ent->client->ps.gunframe++;
		if (ent->client->ps.gunframe == 12 && ent->client->pers.inventory[ent->client->ammo_index])
			ent->client->ps.gunframe = 6;
	}

	if (ent->client->ps.gunframe == 12)
	{
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/hyprbd1a.wav"), 1, ATTN_NORM, 0);
		ent->client->weapon_sound = 0;
	}

}

void Weapon_HyperBlaster (edict_t *ent)
{
	static int	pause_frames[]	= {0};
	static int	fire_frames[]	= {6, 7, 8, 9, 10, 11, 0};

	Weapon_Generic (ent, 5, 20, 49, 53, pause_frames, fire_frames, Weapon_HyperBlaster_Fire);
}

/*
======================================================================

MACHINEGUN / CHAINGUN

======================================================================
*/

void Machinegun_Fire (edict_t *ent)
{
	int	i;
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		angles;
	int			damage = 8;
	int			kick = 2;
	vec3_t		offset;

	if (!(ent->client->buttons & BUTTON_ATTACK))
	{
		ent->client->machinegun_shots = 0;
		ent->client->ps.gunframe++;
		return;
	}

	if (ent->client->ps.gunframe == 5)
		ent->client->ps.gunframe = 4;
	else
		ent->client->ps.gunframe = 5;

	if (ent->client->pers.inventory[ent->client->ammo_index] < 1)
	{
		ent->client->ps.gunframe = 6;
		if (level.time >= ent->pain_debounce_time)
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->pain_debounce_time = level.time + 1;
		}
		NoAmmoWeaponChange (ent);
		return;
	}

	if (is_quad)
	{
//PGM
//		damage *= 4;
		damage *= damage_multiplier;
//		kick *= 4;
		kick *= damage_multiplier;
//PGM
	}

	for (i=1 ; i<3 ; i++)
	{
		ent->client->kick_origin[i] = crandom() * 0.35;
		ent->client->kick_angles[i] = crandom() * 0.7;
	}
	ent->client->kick_origin[0] = crandom() * 0.35;
	ent->client->kick_angles[0] = ent->client->machinegun_shots * -1.5;

	// raise the gun as it is firing
	if (!deathmatch->value)
	{
		ent->client->machinegun_shots++;
		if (ent->client->machinegun_shots > 9)
			ent->client->machinegun_shots = 9;
	}

	// get start / end positions
	VectorAdd (ent->client->v_angle, ent->client->kick_angles, angles);
	AngleVectors (angles, forward, right, NULL);
	VectorSet(offset, 0, 8, ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
	fire_bullet (ent, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_MACHINEGUN);

	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_MACHINEGUN | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
		ent->client->pers.inventory[ent->client->ammo_index]--;

	ent->client->anim_priority = ANIM_ATTACK;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->s.frame = FRAME_crattak1 - (int) (random()+0.25);
		ent->client->anim_end = FRAME_crattak9;
	}
	else
	{
		ent->s.frame = FRAME_attack1 - (int) (random()+0.25);
		ent->client->anim_end = FRAME_attack8;
	}
}

void Weapon_Machinegun (edict_t *ent)
{
	static int	pause_frames[]	= {23, 45, 0};
	static int	fire_frames[]	= {4, 5, 0};

	Weapon_Generic (ent, 3, 5, 45, 49, pause_frames, fire_frames, Machinegun_Fire);
}

void Chaingun_Fire (edict_t *ent)
{
	int			i;
	int			shots;
	vec3_t		start;
	vec3_t		forward, right, up;
	float		r, u;
	vec3_t		offset;
	int			damage;
	int			kick = 2;

	if (deathmatch->value)
		damage = 6;
	else
		damage = 8;

	if (ent->client->ps.gunframe == 5)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);

	if ((ent->client->ps.gunframe == 14) && !(ent->client->buttons & BUTTON_ATTACK))
	{
		ent->client->ps.gunframe = 32;
		ent->client->weapon_sound = 0;
		return;
	}
	else if ((ent->client->ps.gunframe == 21) && (ent->client->buttons & BUTTON_ATTACK)
		&& ent->client->pers.inventory[ent->client->ammo_index])
	{
		ent->client->ps.gunframe = 15;
	}
	else
	{
		ent->client->ps.gunframe++;
	}

	if (ent->client->ps.gunframe == 22)
	{
		ent->client->weapon_sound = 0;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnd1a.wav"), 1, ATTN_IDLE, 0);
	}
	else
	{
		ent->client->weapon_sound = gi.soundindex("weapons/chngnl1a.wav");
	}

	ent->client->anim_priority = ANIM_ATTACK;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->s.frame = FRAME_crattak1 - (ent->client->ps.gunframe & 1);
		ent->client->anim_end = FRAME_crattak9;
	}
	else
	{
		ent->s.frame = FRAME_attack1 - (ent->client->ps.gunframe & 1);
		ent->client->anim_end = FRAME_attack8;
	}

	if (ent->client->ps.gunframe <= 9)
		shots = 1;
	else if (ent->client->ps.gunframe <= 14)
	{
		if (ent->client->buttons & BUTTON_ATTACK)
			shots = 2;
		else
			shots = 1;
	}
	else
		shots = 3;

	if (ent->client->pers.inventory[ent->client->ammo_index] < shots)
		shots = ent->client->pers.inventory[ent->client->ammo_index];

	if (!shots)
	{
		if (level.time >= ent->pain_debounce_time)
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->pain_debounce_time = level.time + 1;
		}
		NoAmmoWeaponChange (ent);
		return;
	}

	if (is_quad)
	{
//PGM
//		damage *= 4;
		damage *= damage_multiplier;
//		kick *= 4;
		kick *= damage_multiplier;
//PGM
	}

	for (i=0 ; i<3 ; i++)
	{
		ent->client->kick_origin[i] = crandom() * 0.35;
		ent->client->kick_angles[i] = crandom() * 0.7;
	}

	for (i=0 ; i<shots ; i++)
	{
		// get start / end positions
		AngleVectors (ent->client->v_angle, forward, right, up);
		r = 7 + crandom()*4;
		u = crandom()*4;
		VectorSet(offset, 0, r, u + ent->viewheight-8);
		P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

		fire_bullet (ent, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_CHAINGUN);
	}

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte ((MZ_CHAINGUN1 + shots - 1) | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
		ent->client->pers.inventory[ent->client->ammo_index] -= shots;
}


void Weapon_Chaingun (edict_t *ent)
{
	static int	pause_frames[]	= {38, 43, 51, 61, 0};
	static int	fire_frames[]	= {5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 0};

	Weapon_Generic (ent, 4, 31, 61, 64, pause_frames, fire_frames, Chaingun_Fire);
}


/*
======================================================================

SHOTGUN / SUPERSHOTGUN

======================================================================
*/

void weapon_shotgun_fire (edict_t *ent)
{
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		offset;
	int			damage = 4;
	int			kick = 8;

	if (ent->client->ps.gunframe == 9)
	{
		ent->client->ps.gunframe++;
		return;
	}

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -2;

	VectorSet(offset, 0, 8,  ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	if (is_quad)
	{
//PGM
//		damage *= 4;
		damage *= damage_multiplier;
//		kick *= 4;
		kick *= damage_multiplier;
//PGM
	}

	if (deathmatch->value)
		fire_shotgun (ent, start, forward, damage, kick, 500, 500, DEFAULT_DEATHMATCH_SHOTGUN_COUNT, MOD_SHOTGUN);
	else
		fire_shotgun (ent, start, forward, damage, kick, 500, 500, DEFAULT_SHOTGUN_COUNT, MOD_SHOTGUN);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_SHOTGUN | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
		ent->client->pers.inventory[ent->client->ammo_index]--;
}

void Weapon_Shotgun (edict_t *ent)
{
	static int	pause_frames[]	= {22, 28, 34, 0};
	static int	fire_frames[]	= {8, 9, 0};

	Weapon_Generic (ent, 7, 18, 36, 39, pause_frames, fire_frames, weapon_shotgun_fire);
}


void weapon_supershotgun_fire (edict_t *ent)
{
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		offset;
	vec3_t		v;
	int			damage = 6;
	int			kick = 12;

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -2;

	VectorSet(offset, 0, 8,  ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	if (is_quad)
	{
//PGM
//		damage *= 4;
		damage *= damage_multiplier;
//		kick *= 4;
		kick *= damage_multiplier;
//PGM
	}

	v[PITCH] = ent->client->v_angle[PITCH];
	v[YAW]   = ent->client->v_angle[YAW] - 5;
	v[ROLL]  = ent->client->v_angle[ROLL];
	AngleVectors (v, forward, NULL, NULL);
	fire_shotgun (ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT/2, MOD_SSHOTGUN);
	v[YAW]   = ent->client->v_angle[YAW] + 5;
	AngleVectors (v, forward, NULL, NULL);
	fire_shotgun (ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT/2, MOD_SSHOTGUN);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_SSHOTGUN | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
		ent->client->pers.inventory[ent->client->ammo_index] -= 2;
}

void Weapon_SuperShotgun (edict_t *ent)
{
	static int	pause_frames[]	= {29, 42, 57, 0};
	static int	fire_frames[]	= {7, 0};

	Weapon_Generic (ent, 6, 17, 57, 61, pause_frames, fire_frames, weapon_supershotgun_fire);
}



/*
======================================================================

RAILGUN

======================================================================
*/

void weapon_railgun_fire (edict_t *ent)
{
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		offset;
	int			damage;
	int			kick;

	if (deathmatch->value)
	{	// normal damage is too extreme in dm
		damage = 100;
		kick = 200;
	}
	else
	{
		damage = 150;
		kick = 250;
	}

	if (is_quad)
	{
//PGM
//		damage *= 4;
		damage *= damage_multiplier;
//		kick *= 4;
		kick *= damage_multiplier;
//PGM
	}

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -3, ent->client->kick_origin);
	ent->client->kick_angles[0] = -3;

	VectorSet(offset, 0, 7,  ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
	fire_rail (ent, start, forward, damage, kick);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_RAILGUN | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
		ent->client->pers.inventory[ent->client->ammo_index]--;
}


void Weapon_Railgun (edict_t *ent)
{
	static int	pause_frames[]	= {56, 0};
	static int	fire_frames[]	= {4, 0};

	Weapon_Generic (ent, 3, 18, 56, 61, pause_frames, fire_frames, weapon_railgun_fire);
}


/*
======================================================================

BFG10K

======================================================================
*/

void weapon_bfg_fire (edict_t *ent)
{
	vec3_t	offset, start;
	vec3_t	forward, right;
	int		damage;
	float	damage_radius = 1000;

	if (deathmatch->value)
		damage = 200;
	else
		damage = 500;

	if (ent->client->ps.gunframe == 9)
	{
		// send muzzle flash
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte (MZ_BFG | is_silenced);
		gi.multicast (ent->s.origin, MULTICAST_PVS);

		ent->client->ps.gunframe++;

		PlayerNoise(ent, start, PNOISE_WEAPON);
		return;
	}

	// cells can go down during windup (from power armor hits), so
	// check again and abort firing if we don't have enough now
	if (ent->client->pers.inventory[ent->client->ammo_index] < 50)
	{
		ent->client->ps.gunframe++;
		return;
	}

	if (is_quad)
//PGM
//		damage *= 4;
		damage *= damage_multiplier;
//PGM

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -2, ent->client->kick_origin);

	// make a big pitch kick with an inverse fall
	ent->client->v_dmg_pitch = -40;
	ent->client->v_dmg_roll = crandom()*8;
	ent->client->v_dmg_time = level.time + DAMAGE_TIME;

	VectorSet(offset, 8, 8, ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
	fire_bfg (ent, start, forward, damage, 400, damage_radius);

	ent->client->ps.gunframe++;

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
		ent->client->pers.inventory[ent->client->ammo_index] -= 50;
}

void Weapon_BFG (edict_t *ent)
{
	static int	pause_frames[]	= {39, 45, 50, 55, 0};
	static int	fire_frames[]	= {9, 17, 0};

	Weapon_Generic (ent, 8, 32, 55, 58, pause_frames, fire_frames, weapon_bfg_fire);
}


//======================================================================
// ROGUE MODS BELOW
//======================================================================


//
// CHAINFIST
//
#define CHAINFIST_REACH 64

void weapon_chainfist_fire (edict_t *ent)
{
	vec3_t	offset;
	vec3_t	forward, right, up;
	vec3_t	start;
	int		damage;

	damage = 15;
	if(deathmatch->value)
		damage = 30;

	if (is_quad)
		damage *= damage_multiplier;

	AngleVectors (ent->client->v_angle, forward, right, up);

	// kick back
	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	// set start point
	VectorSet(offset, 0, 8, ent->viewheight-4);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	fire_player_melee (ent, start, forward, CHAINFIST_REACH, damage, 100, 1, MOD_CHAINFIST);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->ps.gunframe++;
	ent->client->pers.inventory[ent->client->ammo_index] -= ent->client->pers.weapon->quantity;
}

// this spits out some smoke from the motor. it's a two-stroke, you know.
void chainfist_smoke (edict_t *ent)
{
	vec3_t	tempVec, forward, right, up;
	vec3_t	offset;

	AngleVectors(ent->client->v_angle, forward, right, up);
	VectorSet(offset, 8, 8, ent->viewheight -4);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, tempVec);

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_CHAINFIST_SMOKE);
	gi.WritePosition (tempVec);
	gi.unicast (ent, 0);
//	gi.multicast (tempVec, MULTICAST_PVS);
}

#define HOLD_FRAMES			0

void Weapon_ChainFist (edict_t *ent)
{
	static int	pause_frames[]	= {0};
	static int	fire_frames[]	= {8, 9, 16, 17, 18, 30, 31, 0};

	// these are caches for the sound index. there's probably a better way to do this.
//	static int	idle_index;
//	static int	attack_index;
	float		chance;
	int			last_sequence;
	
	last_sequence = 0;

	// load chainsaw sounds and store the indexes for later use.
//	if(!idle_index && !attack_index)
//	{
//		idle_index = gi.soundindex("weapons/sawidle.wav");
//		attack_index = gi.soundindex("weapons/sawhit.wav");
//	}

	if(ent->client->ps.gunframe == 13 ||
		ent->client->ps.gunframe == 23)			// end of attack, go idle
		ent->client->ps.gunframe = 32;

#if HOLD_FRAMES
	else if(ent->client->ps.gunframe == 9 && ((ent->client->buttons) & BUTTON_ATTACK))
		ent->client->ps.gunframe = 7;
	else if(ent->client->ps.gunframe == 18 && ((ent->client->buttons) & BUTTON_ATTACK))
		ent->client->ps.gunframe = 16;
#endif

	// holds for idle sequence
	else if(ent->client->ps.gunframe == 42 && (rand()&7))
	{
		if((ent->client->pers.hand != CENTER_HANDED) && random() < 0.4)
			chainfist_smoke(ent);
//		ent->client->ps.gunframe = 40;
	}
	else if(ent->client->ps.gunframe == 51 && (rand()&7))
	{
		if((ent->client->pers.hand != CENTER_HANDED) && random() < 0.4)
			chainfist_smoke(ent);
//		ent->client->ps.gunframe = 49;
	}	

	// set the appropriate weapon sound.
	if(ent->client->weaponstate == WEAPON_FIRING)
//		ent->client->weapon_sound = attack_index;
		ent->client->weapon_sound = gi.soundindex("weapons/sawhit.wav");
	else if(ent->client->weaponstate == WEAPON_DROPPING)
		ent->client->weapon_sound = 0;
	else
//		ent->client->weapon_sound = idle_index;
		ent->client->weapon_sound = gi.soundindex("weapons/sawidle.wav");

	Weapon_Generic (ent, 4, 32, 57, 60, pause_frames, fire_frames, weapon_chainfist_fire);

//	gi.dprintf("chainfist %d\n", ent->client->ps.gunframe);
	if((ent->client->buttons) & BUTTON_ATTACK)
	{
		if(ent->client->ps.gunframe == 13 ||
			ent->client->ps.gunframe == 23 ||
			ent->client->ps.gunframe == 32)
		{
			last_sequence = ent->client->ps.gunframe;
			ent->client->ps.gunframe = 6;
		}
	}

	if (ent->client->ps.gunframe == 6)
	{
		chance = random();
		if(last_sequence == 13)			// if we just did sequence 1, do 2 or 3.
			chance -= 0.34;
		else if(last_sequence == 23)	// if we just did sequence 2, do 1 or 3
			chance += 0.33;
		else if(last_sequence == 32)	// if we just did sequence 3, do 1 or 2
		{
			if(chance >= 0.33)
				chance += 0.34;
		}

		if(chance < 0.33)
			ent->client->ps.gunframe = 14;
		else if(chance < 0.66)
			ent->client->ps.gunframe = 24;
	}

}

//
// Disintegrator
//

void weapon_tracker_fire (edict_t *self)
{
	vec3_t		forward, right;
	vec3_t		start;
	vec3_t		end;
	vec3_t		offset;
	edict_t		*enemy;
	trace_t		tr;
	int			damage;
	vec3_t		mins, maxs;

	// PMM - felt a little high at 25
	if(deathmatch->value)
		damage = 30;
	else
		damage = 45;

	if (is_quad)
		damage *= damage_multiplier;		//pgm

	VectorSet(mins, -16, -16, -16);
	VectorSet(maxs, 16, 16, 16);
	AngleVectors (self->client->v_angle, forward, right, NULL);
	VectorSet(offset, 24, 8, self->viewheight-8);
	P_ProjectSource (self->client, self->s.origin, offset, forward, right, start);

	// FIXME - can we shorten this? do we need to?
	VectorMA (start, 8192, forward, end);
	enemy = NULL;
	//PMM - doing two traces .. one point and one box.  
	tr = gi.trace (start, vec3_origin, vec3_origin, end, self, MASK_SHOT);
	if(tr.ent != world)
	{
		if(tr.ent->svflags & SVF_MONSTER || tr.ent->client || tr.ent->svflags & SVF_DAMAGEABLE)
		{
			if(tr.ent->health > 0)
				enemy = tr.ent;
		}
	}
	else
	{
		tr = gi.trace (start, mins, maxs, end, self, MASK_SHOT);
		if(tr.ent != world)
		{
			if(tr.ent->svflags & SVF_MONSTER || tr.ent->client || tr.ent->svflags & SVF_DAMAGEABLE)
			{
				if(tr.ent->health > 0)
					enemy = tr.ent;
			}
		}
	}

	VectorScale (forward, -2, self->client->kick_origin);
	self->client->kick_angles[0] = -1;

	fire_tracker (self, start, forward, damage, 1000, enemy);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (self-g_edicts);
	gi.WriteByte (MZ_TRACKER);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	PlayerNoise(self, start, PNOISE_WEAPON);

	self->client->ps.gunframe++;
	self->client->pers.inventory[self->client->ammo_index] -= self->client->pers.weapon->quantity;
}

void Weapon_Disintegrator (edict_t *ent)
{
	static int	pause_frames[]	= {14, 19, 23, 0};
//	static int	fire_frames[]	= {7, 0};
	static int	fire_frames[]	= {5, 0};

	Weapon_Generic (ent, 4, 9, 29, 34, pause_frames, fire_frames, weapon_tracker_fire);
}

/*
======================================================================

ETF RIFLE

======================================================================
*/
void weapon_etf_rifle_fire (edict_t *ent)
{
	vec3_t	forward, right, up;
	vec3_t	start, tempPt;
	int		damage;
	int		kick = 3;
	int		i;
	vec3_t	angles;
	vec3_t	offset;

	if(deathmatch->value)
		damage = 10;
	else
		damage = 10;

	// PGM - adjusted to use the quantity entry in the weapon structure.
	if(ent->client->pers.inventory[ent->client->ammo_index] < ent->client->pers.weapon->quantity)
	{
		VectorClear (ent->client->kick_origin);
		VectorClear (ent->client->kick_angles);
		ent->client->ps.gunframe = 8;

		if (level.time >= ent->pain_debounce_time)
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->pain_debounce_time = level.time + 1;
		}
		NoAmmoWeaponChange (ent);
		return;
	}

	if (is_quad)
	{
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	for(i=0;i<3;i++)
	{
		ent->client->kick_origin[i] = crandom() * 0.85;
		ent->client->kick_angles[i] = crandom() * 0.85;
	}

	// get start / end positions
	VectorAdd (ent->client->v_angle, ent->client->kick_angles, angles);
//	AngleVectors (angles, forward, right, NULL);
//	gi.dprintf("v_angle: %s\n", vtos(ent->client->v_angle));
	AngleVectors (ent->client->v_angle, forward, right, up);

	// FIXME - set correct frames for different offsets.

	if(ent->client->ps.gunframe == 6)					// right barrel
	{
//		gi.dprintf("right\n");
		VectorSet(offset, 15, 8, -8);
	}
	else										// left barrel
	{
//		gi.dprintf("left\n");
		VectorSet(offset, 15, 6, -8);
	}
	
	VectorCopy (ent->s.origin, tempPt);
	tempPt[2] += ent->viewheight;
	P_ProjectSource2 (ent->client, tempPt, offset, forward, right, up, start);
//	gi.dprintf("start: %s\n", vtos(start));
	fire_flechette (ent, start, forward, damage, 750, kick);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_ETF_RIFLE);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->ps.gunframe++;
	ent->client->pers.inventory[ent->client->ammo_index] -= ent->client->pers.weapon->quantity;

	ent->client->anim_priority = ANIM_ATTACK;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->s.frame = FRAME_crattak1 - 1;
		ent->client->anim_end = FRAME_crattak9;
	}
	else
	{
		ent->s.frame = FRAME_attack1 - 1;
		ent->client->anim_end = FRAME_attack8;
	}

}

void Weapon_ETF_Rifle (edict_t *ent)
{
	static int	pause_frames[]	= {18, 28, 0};
	static int	fire_frames[]	= {6, 7, 0};
//	static int	idle_seq;

	// note - if you change the fire frame number, fix the offset in weapon_etf_rifle_fire.

//	if (!(ent->client->buttons & BUTTON_ATTACK))
//		ent->client->machinegun_shots = 0;

	if (ent->client->weaponstate == WEAPON_FIRING)
	{
		if (ent->client->pers.inventory[ent->client->ammo_index] <= 0)
			ent->client->ps.gunframe = 8;
	}

	Weapon_Generic (ent, 4, 7, 37, 41, pause_frames, fire_frames, weapon_etf_rifle_fire);

	if(ent->client->ps.gunframe == 8 && (ent->client->buttons & BUTTON_ATTACK))
		ent->client->ps.gunframe = 6;

//	gi.dprintf("etf rifle %d\n", ent->client->ps.gunframe);
}

// pgm - this now uses ent->client->pers.weapon->quantity like all the other weapons
//#define HEATBEAM_AMMO_USE		2		
#define	HEATBEAM_DM_DMG			15
#define HEATBEAM_SP_DMG			15

void Heatbeam_Fire (edict_t *ent)
{
	vec3_t		start;
	vec3_t		forward, right, up;
	vec3_t		offset;
	int			damage;
	int			kick;

	// for comparison, the hyperblaster is 15/20
	// jim requested more damage, so try 15/15 --- PGM 07/23/98
	if (deathmatch->value)
		damage = HEATBEAM_DM_DMG;
	else
		damage = HEATBEAM_SP_DMG;

	if (deathmatch->value)  // really knock 'em around in deathmatch
		kick = 75;
	else
		kick = 30;

//	if(ent->client->pers.inventory[ent->client->ammo_index] < HEATBEAM_AMMO_USE)
//	{
//		NoAmmoWeaponChange (ent);
//		return;
//	}

	ent->client->ps.gunframe++;
	ent->client->ps.gunindex = gi.modelindex ("models/weapons/v_beamer2/tris.md2");

	if (is_quad)
	{
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	VectorClear (ent->client->kick_origin);
	VectorClear (ent->client->kick_angles);

	// get start / end positions
	AngleVectors (ent->client->v_angle, forward, right, up);

// This offset is the "view" offset for the beam start (used by trace)
	
	VectorSet(offset, 7, 2, ent->viewheight-3);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	// This offset is the entity offset
	VectorSet(offset, 2, 7, -3);

	fire_heat (ent, start, forward, offset, damage, kick, false);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_HEATBEAM | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
		ent->client->pers.inventory[ent->client->ammo_index] -= ent->client->pers.weapon->quantity;

	ent->client->anim_priority = ANIM_ATTACK;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->s.frame = FRAME_crattak1 - 1;
		ent->client->anim_end = FRAME_crattak9;
	}
	else
	{
		ent->s.frame = FRAME_attack1 - 1;
		ent->client->anim_end = FRAME_attack8;
	}

}

void Weapon_Heatbeam (edict_t *ent)
{
//	static int	pause_frames[]	= {38, 43, 51, 61, 0};
//	static int	fire_frames[]	= {5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 0};
	static int	pause_frames[]	= {35, 0};
//	static int	fire_frames[]	= {9, 0};
	static int	fire_frames[]	= {9, 10, 11, 12, 0};
//	static int	attack_index;
//	static int  off_model, on_model;

//	if ((g_showlogic) && (g_showlogic->value)) {
//		gi.dprintf ("Frame %d, skin %d\n", ent->client->ps.gunframe, ent->client->ps.gunskin);
//	}
	
//	if (!attack_index)
//	{
//		attack_index = gi.soundindex ("weapons/bfg__l1a.wav");
//		off_model = gi.modelindex ("models/weapons/v_beamer/tris.md2");
//		on_model = gi.modelindex ("models/weapons/v_beamer2/tris.md2");
		//ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);
//	}

	if (ent->client->weaponstate == WEAPON_FIRING)
	{
//		ent->client->weapon_sound = attack_index;
		ent->client->weapon_sound = gi.soundindex ("weapons/bfg__l1a.wav");
		if ((ent->client->pers.inventory[ent->client->ammo_index] >= 2) && ((ent->client->buttons) & BUTTON_ATTACK))
		{
//			if(ent->client->ps.gunframe >= 9 && ((ent->client->buttons) & BUTTON_ATTACK))
//			if(ent->client->ps.gunframe >= 12 && ((ent->client->buttons) & BUTTON_ATTACK))
			if(ent->client->ps.gunframe >= 13)
			{
				ent->client->ps.gunframe = 9;
//				ent->client->ps.gunframe = 8;
//				ent->client->ps.gunskin = 0;
//				ent->client->ps.gunindex = on_model;
				ent->client->ps.gunindex = gi.modelindex ("models/weapons/v_beamer2/tris.md2");
			}
			else
			{
//				ent->client->ps.gunskin = 1;
//				ent->client->ps.gunindex = on_model;
				ent->client->ps.gunindex = gi.modelindex ("models/weapons/v_beamer2/tris.md2");
			}
		}
		else
		{
//			ent->client->ps.gunframe = 10;
			ent->client->ps.gunframe = 13;
//			ent->client->ps.gunskin = 1;
//			ent->client->ps.gunindex = off_model;
			ent->client->ps.gunindex = gi.modelindex ("models/weapons/v_beamer/tris.md2");
		}
	}
	else
	{
//		ent->client->ps.gunskin = 1;
//		ent->client->ps.gunindex = off_model;
		ent->client->ps.gunindex = gi.modelindex ("models/weapons/v_beamer/tris.md2");
		ent->client->weapon_sound = 0;
	}

//	Weapon_Generic (ent, 8, 9, 39, 44, pause_frames, fire_frames, Heatbeam_Fire);
	Weapon_Generic (ent, 8, 12, 39, 44, pause_frames, fire_frames, Heatbeam_Fire);
}
