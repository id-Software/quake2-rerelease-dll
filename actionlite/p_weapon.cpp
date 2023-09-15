// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_weapon.c

#include "g_local.h"
#include "m_player.h"

bool is_quad;
// RAFAEL
bool is_quadfire;
// RAFAEL
player_muzzle_t is_silenced;

// PGM
byte damage_multiplier;
// PGM

void weapon_grenade_fire(edict_t *ent, bool held);
// RAFAEL
void weapon_trap_fire(edict_t *ent, bool held);
// RAFAEL

//========
// [Kex]
bool G_CheckInfiniteAmmo(gitem_t *item)
{
	if (item->flags & IF_NO_INFINITE_AMMO)
		return false;

	return g_infinite_ammo->integer || g_instagib->integer;
}

//========
// ROGUE
byte P_DamageModifier(edict_t *ent)
{
	is_quad = 0;
	damage_multiplier = 1;

	if (ent->client->quad_time > level.time)
	{
		damage_multiplier *= 4;
		is_quad = 1;

		// if we're quad and DF_NO_STACK_DOUBLE is on, return now.
		// if (g_dm_no_stack_double->integer)
		// 	return damage_multiplier;
	}

	if (ent->client->double_time > level.time)
	{
		damage_multiplier *= 2;
		is_quad = 1;
	}

	return damage_multiplier;
}
// ROGUE
//========

// [Paril-KEX] kicks in vanilla take place over 2 10hz server
// frames; this is to mimic that visual behavior on any tickrate.
inline float P_CurrentKickFactor(edict_t *ent)
{
	if (ent->client->kick.time < level.time)
		return 0.f;

	float f = (ent->client->kick.time - level.time).seconds() / ent->client->kick.total.seconds();
	return f;
}

// [Paril-KEX]
vec3_t P_CurrentKickAngles(edict_t *ent)
{
	return ent->client->kick.angles * P_CurrentKickFactor(ent);
}

vec3_t P_CurrentKickOrigin(edict_t *ent)
{
	return ent->client->kick.origin * P_CurrentKickFactor(ent);
}

void P_AddWeaponKick(edict_t *ent, const vec3_t &origin, const vec3_t &angles)
{
	ent->client->kick.origin = origin;
	ent->client->kick.angles = angles;
	ent->client->kick.total = 200_ms;
	ent->client->kick.time = level.time + ent->client->kick.total;
}

void P_ProjectSource(edict_t *ent, const vec3_t &angles, vec3_t distance, vec3_t &result_start, vec3_t &result_dir)
{
	if (ent->client->pers.hand == LEFT_HANDED)
		distance[1] *= -1;
	else if (ent->client->pers.hand == CENTER_HANDED)
		distance[1] = 0;

	vec3_t forward, right, up;
	vec3_t eye_position = (ent->s.origin + vec3_t{ 0, 0, (float) ent->viewheight });

	AngleVectors(angles, forward, right, up);

	result_start = G_ProjectSource2(eye_position, distance, forward, right, up);

	vec3_t	   end = eye_position + forward * 8192;
	contents_t mask = MASK_PROJECTILE & ~CONTENTS_DEADMONSTER;

	// [Paril-KEX]
	if (!G_ShouldPlayersCollide(true))
		mask &= ~CONTENTS_PLAYER;

	trace_t tr = gi.traceline(eye_position, end, ent, mask);

	// if the point was a monster & close to us, use raw forward
	// so railgun pierces properly
	if (tr.startsolid || ((tr.contents & (CONTENTS_MONSTER | CONTENTS_PLAYER)) && (tr.fraction * 8192.f) < 128.f))
		result_dir = forward;
	else
	{
		end = tr.endpos;
		result_dir = (end - result_start).normalized();

#if 0
		// correction for blocked shots
		trace_t eye_tr = gi.traceline(result_start, result_start + (result_dir * tr.fraction * 8192.f), ent, mask);

		if ((eye_tr.endpos - tr.endpos).length() > 32.f)
		{
			result_start = eye_position;
			result_dir = (end - result_start).normalized();
			return;
		}
#endif
	}
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
void PlayerNoise(edict_t *who, const vec3_t &where, player_noise_t type)
{
	edict_t *noise;

	if (type == PNOISE_WEAPON)
	{
		if (who->client->silencer_shots)
			who->client->invisibility_fade_time = level.time + (INVISIBILITY_TIME / 5);
		else
			who->client->invisibility_fade_time = level.time + INVISIBILITY_TIME;

		if (who->client->silencer_shots)
		{
			who->client->silencer_shots--;
			return;
		}
	}

	if (deathmatch->integer)
		return;

	if (who->flags & FL_NOTARGET)
		return;

	if (type == PNOISE_SELF &&
		(who->client->landmark_free_fall || who->client->landmark_noise_time >= level.time))
		return;

	// ROGUE
	if (who->flags & FL_DISGUISED)
	{
		if (type == PNOISE_WEAPON)
		{
			level.disguise_violator = who;
			level.disguise_violation_time = level.time + 500_ms;
		}
		else
			return;
	}
	// ROGUE

	if (!who->mynoise)
	{
		noise = G_Spawn();
		noise->classname = "player_noise";
		noise->mins = { -8, -8, -8 };
		noise->maxs = { 8, 8, 8 };
		noise->owner = who;
		noise->svflags = SVF_NOCLIENT;
		who->mynoise = noise;

		noise = G_Spawn();
		noise->classname = "player_noise";
		noise->mins = { -8, -8, -8 };
		noise->maxs = { 8, 8, 8 };
		noise->owner = who;
		noise->svflags = SVF_NOCLIENT;
		who->mynoise2 = noise;
	}

	if (type == PNOISE_SELF || type == PNOISE_WEAPON)
	{
		noise = who->mynoise;
		who->client->sound_entity = noise;
		who->client->sound_entity_time = level.time;
	}
	else // type == PNOISE_IMPACT
	{
		noise = who->mynoise2;
		who->client->sound2_entity = noise;
		who->client->sound2_entity_time = level.time;
	}

	noise->s.origin = where;
	noise->absmin = where - noise->maxs;
	noise->absmax = where + noise->maxs;
	noise->teleport_time = level.time;
	gi.linkentity(noise);
}

inline bool G_WeaponShouldStay()
{
	if (deathmatch->integer)
		return g_dm_weapons_stay->integer;
	else if (coop->integer)
		return !P_UseCoopInstancedItems();

	return false;
}

void G_CheckAutoSwitch(edict_t *ent, gitem_t *item, bool is_new);

// Vanilla Pickup_Weapon
// bool Pickup_Weapon(edict_t *ent, edict_t *other)
//  {
//  	item_id_t index;
//  	gitem_t	*ammo;

//  	index = ent->item->id;

//  	if (G_WeaponShouldStay() && other->client->pers.inventory[index])
//  	{
//  		if (!(ent->spawnflags & (SPAWNFLAG_ITEM_DROPPED | SPAWNFLAG_ITEM_DROPPED_PLAYER)))
//  			return false; // leave the weapon for others to pickup
//  	}

//  	bool is_new = !other->client->pers.inventory[index];

//  	other->client->pers.inventory[index]++;
	
//  	if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED))
//  	{
//  		// give them some ammo with it
//  		// PGM -- IF APPROPRIATE!
//  		if (ent->item->ammo) // PGM
//  		{
//  			ammo = GetItemByIndex(ent->item->ammo);
//  			// RAFAEL: Don't get infinite ammo with trap
//  			if (G_CheckInfiniteAmmo(ammo))
//  				Add_Ammo(other, ammo, 1000);
//  			else
//  				Add_Ammo(other, ammo, ammo->quantity);
//  		}

//  		if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED_PLAYER))
//  		{
//  			if (deathmatch->integer)
//  			{
//  				if (g_dm_weapons_stay->integer)
//  					ent->flags |= FL_RESPAWN;

//  				SetRespawn( ent, gtime_t::from_sec(g_weapon_respawn_time->integer), !g_dm_weapons_stay->integer);
//  			}
//  			if (coop->integer)
//  				ent->flags |= FL_RESPAWN;
//  		}
//  	}

//  	G_CheckAutoSwitch(other, ent->item, is_new);

//  	return true;
//  }

// keep the entity around so we can find it later if we need to respawn the weapon there
void SetSpecWeapHolder(edict_t* ent)
{
	ent->flags |= FL_RESPAWN;
	ent->svflags |= SVF_NOCLIENT;
	ent->solid = SOLID_NOT;
	ent->think = PlaceHolder;
	gi.linkentity(ent);
}

bool Pickup_Weapon(edict_t* ent, edict_t* other)
{
	int index, index2;
	gitem_t* ammo;
	bool addAmmo;
	int special = 0;
	int band = 0;

	index = ITEM_INDEX(ent->item);

	if (g_dm_weapons_stay->value && other->client->inventory[index])
	{
		if (!(ent->spawnflags & (SPAWNFLAG_ITEM_DROPPED | SPAWNFLAG_ITEM_DROPPED_PLAYER)))
			return false;		// leave the weapon for others to pickup
	}

	// find out if they have a bandolier
	if (INV_AMMO(other, BAND_NUM))
		band = 1;
	else
		band = 0;


	// zucc special cases for picking up weapons
	// the mk23 should never be dropped, probably

	switch (index) {
	case IT_WEAPON_MK23:
		// if (!WPF_ALLOWED(MK23_NUM))
		// 	return false;

		if (other->client->inventory[index])	// already has one
		{
			if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED))
			{
				ammo = FindItem(MK23_AMMO_NAME);
				addAmmo = Add_Ammo(other, ammo, ammo->quantity);
				if (addAmmo && !(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED_PLAYER))
					SetRespawn(ent, gtime_t::from_sec((weapon_respawn->integer)));
				return addAmmo;
			}
		}
		other->client->inventory[index]++;
		if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED)) {
			other->client->mk23_rds = other->client->mk23_max;
			if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED_PLAYER))
				SetRespawn(ent, gtime_t::from_sec((weapon_respawn->integer)));
		}
		return true;

	case IT_WEAPON_MP5:
		if (other->client->unique_weapon_total >= unique_weapons->value + band)
			return false;		// we can't get it
		if ((!allow_hoarding->value) && other->client->inventory[index])
			return false;		// we already have one

		other->client->inventory[index]++;
		other->client->unique_weapon_total++;
		if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED)) {
			other->client->mp5_rds = other->client->mp5_max;
		}
		special = 1;
		gi.LocClient_Print(other, PRINT_HIGH, "{} - Unique Weapon\n", ent->item->pickup_name);
		break;

	case IT_WEAPON_M4:
		if (other->client->unique_weapon_total >= unique_weapons->value + band)
			return false;		// we can't get it
		if ((!allow_hoarding->value) && other->client->inventory[index])
			return false;		// we already have one

		other->client->inventory[index]++;
		other->client->unique_weapon_total++;
		if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED)) {
			other->client->m4_rds = other->client->m4_max;
		}
		special = 1;
		gi.LocClient_Print(other, PRINT_HIGH, "{} - Unique Weapon\n", ent->item->pickup_name);
		break;

	case IT_WEAPON_M3:
		if (other->client->unique_weapon_total >= unique_weapons->value + band)
			return false;		// we can't get it
		if ((!allow_hoarding->value) && other->client->inventory[index])
			return false;		// we already have one

		other->client->inventory[index]++;
		other->client->unique_weapon_total++;
		if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED))
		{
			// any weapon that doesn't completely fill up each reload can 
			//end up in a state where it has a full weapon and pending reload(s)
			if (other->client->weaponstate == WEAPON_RELOADING &&
				other->client->curr_weap.id == IT_WEAPON_M3)
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
		gi.LocClient_Print(other, PRINT_HIGH, "{} - Unique Weapon\n", ent->item->pickup_name);
		break;

	case IT_WEAPON_HANDCANNON:
		if (other->client->unique_weapon_total >= unique_weapons->value + band)
			return false;		// we can't get it
		if ((!allow_hoarding->value) && other->client->inventory[index])
			return false;		// we already have one

		other->client->inventory[index]++;
		other->client->unique_weapon_total++;
		if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED))
		{
			other->client->cannon_rds = other->client->cannon_max;
			index2 = ITEM_INDEX(FindItem(SHOTGUN_AMMO_NAME));
			if (other->client->inventory[index2] + 5 > other->client->max_shells)
				other->client->inventory[index2] = other->client->max_shells;
			else
				other->client->inventory[index2] += 5;
		}
		gi.LocClient_Print(other, PRINT_HIGH, "{} - Unique Weapon\n", ent->item->pickup_name);
		special = 1;
		break;

	case IT_WEAPON_SNIPER:
		if (other->client->unique_weapon_total >= unique_weapons->value + band)
			return false;		// we can't get it
		if ((!allow_hoarding->value) && other->client->inventory[index])
			return false;		// we already have one

		other->client->inventory[index]++;
		other->client->unique_weapon_total++;
		if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED))
		{
			if (other->client->weaponstate == WEAPON_RELOADING &&
				other->client->curr_weap.id == IT_WEAPON_SNIPER)
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
		gi.LocClient_Print(other, PRINT_HIGH, "{} - Unique Weapon\n", ent->item->pickup_name);
		break;

	case IT_WEAPON_DUALMK23:
		// if (!WPF_ALLOWED(MK23_NUM))
		// 	return false;

		if (other->client->inventory[index])	// already has one
		{
			if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED))
			{
				ammo = FindItem(MK23_AMMO_NAME);
				addAmmo = Add_Ammo(other, ammo, ammo->quantity);
				if (addAmmo && !(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED_PLAYER))
					SetRespawn(ent, gtime_t::from_sec((weapon_respawn->integer)));

				return addAmmo;
			}
		}
		other->client->inventory[index]++;
		if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED))
		{
			other->client->dual_rds += other->client->mk23_max;
			// assume the player uses the new (full) pistol
			other->client->mk23_rds = other->client->mk23_max;
			if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED_PLAYER))
				SetRespawn(ent, gtime_t::from_sec((weapon_respawn->integer)));
		}
		return true;

	case IT_WEAPON_KNIFE:
		if (other->client->inventory[index] < other->client->knife_max)
		{
			other->client->inventory[index]++;
			return true;
		}

		return false;

	case IT_WEAPON_GRENADES:
		if (!(gameSettings & GS_DEATHMATCH) && ctf->value != 2 && !band)
			return false;

		if (other->client->inventory[index] >= other->client->grenade_max)
			return false;

		other->client->inventory[index]++;
		if (!(ent->spawnflags & (SPAWNFLAG_ITEM_DROPPED | SPAWNFLAG_ITEM_DROPPED_PLAYER)))
		{
			if (g_dm_weapons_stay->integer)
				ent->flags |= FL_RESPAWN;
			else
				SetRespawn(ent, gtime_t::from_sec((ammo_respawn->integer)));
				//SetRespawn(ent, ammo_respawn->value);
		}
		return true;

	default:
		other->client->inventory[index]++;

		if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED))
		{
			// give them some ammo with it
			ammo = FindItem(MK23_AMMO_NAME);
			
			if (g_infinite_ammo->value)
				Add_Ammo(other, ammo, 1000);
			else
				Add_Ammo(other, ammo, ammo->quantity);

			if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED_PLAYER))
			{
				if (g_dm_weapons_stay->integer)
					ent->flags |= FL_RESPAWN;
				else
					SetRespawn(ent, 30_sec);
			}
		}
		break;
	}

	if (!(ent->spawnflags & (SPAWNFLAG_ITEM_DROPPED | SPAWNFLAG_ITEM_DROPPED_PLAYER))
		&& (SPEC_WEAPON_RESPAWN) && special)
	{
		if (g_weapon_respawn_time->integer && ((gameSettings & GS_DEATHMATCH) || ctf->value == 2))
			SetRespawn(ent, gtime_t::from_sec((weapon_respawn->integer)));
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
	if (!ent->client->pers.weapon) {
		ent->s.modelindex2 = 0;
		return;
	}

	// Determine the weapon's precache index.
	nIndex = ent->client->pers.weapon->id;

	// Clear previous weapon model.
	ent->s.skinnum &= 255;

	// Set new weapon model.
	ent->s.skinnum |= (nIndex << 8);
	ent->s.modelindex2 = 255;
}

static void Weapon_RunThink(edict_t *ent)
{
	// call active weapon think routine
	if (!ent->client->pers.weapon->weaponthink)
		return;

	P_DamageModifier(ent);
	// RAFAEL
	is_quadfire = (ent->client->quadfire_time > level.time);
	// RAFAEL
	if (ent->client->silencer_shots)
		is_silenced = MZ_SILENCED;
	else
		is_silenced = MZ_NONE;
	ent->client->pers.weapon->weaponthink(ent);
}

/*
===============
ChangeWeapon

The old weapon has been dropped all the way, so make the new one
current
===============
*/

//void ChangeWeapon(edict_t *ent)
//{
//	// [Paril-KEX]
//	if (ent->health > 0 && !g_instant_weapon_switch->integer && ((ent->client->latched_buttons | ent->client->buttons) & BUTTON_HOLSTER))
//		return;
//
//	if (ent->client->grenade_time)
//	{
//		// force a weapon think to drop the held grenade
//		ent->client->weapon_sound = 0;
//		Weapon_RunThink(ent);
//		ent->client->grenade_time = 0_ms;
//	}
//
//	if (ent->client->pers.weapon)
//	{
//		ent->client->pers.lastweapon = ent->client->pers.weapon;
//
//		if (ent->client->newweapon && ent->client->newweapon != ent->client->pers.weapon)
//			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/change.wav"), 1, ATTN_NORM, 0);
//	}
//
//	ent->client->pers.weapon = ent->client->newweapon;
//	ent->client->newweapon = nullptr;
//	ent->client->machinegun_shots = 0;
//
//	// set visible model
//	if (ent->s.modelindex == MODELINDEX_PLAYER)
//		P_AssignClientSkinnum(ent);
//
//	if (!ent->client->pers.weapon)
//	{ // dead
//		ent->client->ps.gunindex = 0;
//		ent->client->ps.gunskin = 0;
//		return;
//	}
//
//	ent->client->weaponstate = WEAPON_ACTIVATING;
//	ent->client->ps.gunframe = 0;
//	ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);
//	ent->client->ps.gunskin = 0;
//	ent->client->weapon_sound = 0;
//
//	ent->client->anim_priority = ANIM_PAIN;
//	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
//	{
//		ent->s.frame = FRAME_crpain1;
//		ent->client->anim_end = FRAME_crpain4;
//	}
//	else
//	{
//		ent->s.frame = FRAME_pain301;
//		ent->client->anim_end = FRAME_pain304;
//	}
//	ent->client->anim_time = 0_ms;
//
//	// for instantweap, run think immediately
//	// to set up correct start frame
//	if (g_instant_weapon_switch->integer)
//		Weapon_RunThink(ent);
//}

void ChangeWeapon(edict_t* ent)
{

	if (ent->client->grenade_time)
	{
		ent->client->grenade_time = level.time;
		weapon_grenade_fire(ent, false);
		ent->client->grenade_time = gtime_t::from_sec(0);
	}

	if (ent->client->ctf_grapple)
		CTFPlayerResetGrapple(ent);

	// zucc - prevent reloading queue for previous weapon from doing anything
	ent->client->reload_attempts = 0;

	ent->client->pers.lastweapon = ent->client->pers.weapon;
	ent->client->pers.weapon = ent->client->newweapon;
	ent->client->newweapon = NULL;
	ent->client->machinegun_shots = 0;

	if (ent->client->pers.weapon && ent->client->pers.weapon->ammo)
		ent->client->ammo_index = ent->client->inventory[ent->client->ammo_index];
		//ent->client->ammo_index = ITEM_INDEX(FindItem(ent->client->pers.weapon->ammo));
	else
		ent->client->ammo_index = 0;

	if (!ent->client->pers.weapon || ent->s.modelindex != 255)	// zucc vwep
	{				// dead 
		ent->client->ps.gunindex = 0;
		return;
	}

	ent->client->weaponstate = WEAPON_ACTIVATING;
	ent->client->ps.gunframe = 0;

	//FIREBLADE
	if (ent->solid == SOLID_NOT && ent->deadflag != false)
		return;
	//FIREBLADE

	ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);

	// zucc hentai's animation for vwep
	//if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	//	SetAnimation(ent, FRAME_crpain1, FRAME_crpain4, ANIM_PAIN);
	//else
	//	SetAnimation(ent, FRAME_pain301, FRAME_pain304, ANIM_PAIN);

	ent->client->anim_priority = ANIM_PAIN;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->s.frame = FRAME_crpain1;
		ent->client->anim_end = FRAME_crpain4;
	}
	else
	{
		ent->s.frame = FRAME_pain301;
		ent->client->anim_end = FRAME_pain304;
	}
	ent->client->anim_time = 0_ms;

	ShowGun(ent);
	// zucc done

	ent->client->curr_weap.id = ent->client->pers.weapon->id;
	if (ent->client->curr_weap.id == IT_WEAPON_GRENADES) {
		// Fix the "use grenades;drop bandolier" bug, caused infinite grenades.
		if (teamplay->value && INV_AMMO(ent, GRENADE_NUM) == 0)
			INV_AMMO(ent, GRENADE_NUM) = 1;
	}

	if (INV_AMMO(ent, LASER_NUM))
		SP_LaserSight(ent, GET_ITEM(LASER_NUM));	//item->use(ent, item);
}

/*
=================
NoAmmoWeaponChange
=================
*/
void NoAmmoWeaponChange(edict_t *ent, bool sound)
{
	if (sound)
	{
		if (level.time >= ent->client->empty_click_sound)
		{
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->client->empty_click_sound = level.time + 1_sec;
		}
	}

	constexpr item_id_t no_ammo_order[] = {
		IT_WEAPON_SNIPER,
		IT_WEAPON_MP5,
		IT_WEAPON_M4,
		IT_WEAPON_M3,
		IT_WEAPON_HANDCANNON,
		IT_WEAPON_MK23,
		IT_WEAPON_DUALMK23,
		IT_WEAPON_KNIFE,
		IT_WEAPON_GRENADES
	};

	for (size_t i = 0; i < q_countof(no_ammo_order); i++)
	{
		gitem_t *item = GetItemByIndex(no_ammo_order[i]);

		if (!item)
			gi.Com_ErrorFmt("Invalid no ammo weapon switch weapon {}\n", (int32_t) no_ammo_order[i]);

		if (!ent->client->pers.inventory[item->id])
			continue;

		if (item->ammo && ent->client->pers.inventory[item->ammo] < item->quantity)
			continue;

		ent->client->newweapon = item;
		return;
	}
}

void G_RemoveAmmo(edict_t *ent, int32_t quantity)
{
	if (G_CheckInfiniteAmmo(ent->client->pers.weapon))
		return;

	bool pre_warning = ent->client->pers.inventory[ent->client->pers.weapon->ammo] <=
		ent->client->pers.weapon->quantity_warn;

	ent->client->pers.inventory[ent->client->pers.weapon->ammo] -= quantity;

	bool post_warning = ent->client->pers.inventory[ent->client->pers.weapon->ammo] <=
		ent->client->pers.weapon->quantity_warn;

	if (!pre_warning && post_warning)
		gi.local_sound(ent, CHAN_AUTO, gi.soundindex("weapons/lowammo.wav"), 1, ATTN_NORM, 0);

	// if (ent->client->pers.weapon->ammo == IT_AMMO_CELLS)
	// 	G_CheckPowerArmor(ent);
}

void G_RemoveAmmo(edict_t *ent)
{
	G_RemoveAmmo(ent, ent->client->pers.weapon->quantity);
}

// [Paril-KEX] get time per animation frame
inline gtime_t Weapon_AnimationTime(edict_t *ent)
{
	if (g_quick_weapon_switch->integer && (gi.tick_rate == 20 || gi.tick_rate == 40) &&
		(ent->client->weaponstate == WEAPON_ACTIVATING || ent->client->weaponstate == WEAPON_DROPPING))
		ent->client->ps.gunrate = 20;
	else
		ent->client->ps.gunrate = 10;

	// No speed enhancers in Action
	/*if (ent->client->ps.gunframe != 0 && (!(ent->client->pers.weapon->flags & IF_NO_HASTE) || ent->client->weaponstate != WEAPON_FIRING))
	{
		if (is_quadfire)
			ent->client->ps.gunrate *= 2;
		if (CTFApplyHaste(ent))
			ent->client->ps.gunrate *= 2;
	}*/

	// network optimization...
	if (ent->client->ps.gunrate == 10)
	{
		ent->client->ps.gunrate = 0;
		return 100_ms;
	}

	return gtime_t::from_ms((1.f / ent->client->ps.gunrate) * 1000);
}

/*
=================
Think_Weapon

Called by ClientBeginServerFrame and ClientThink
=================
*/
void Think_Weapon(edict_t *ent)
{
	if (ent->client->resp.spectator)
		return;

	// if just died, put the weapon away
	if (ent->health < 1)
	{
		ent->client->newweapon = nullptr;
		ChangeWeapon(ent);
	}

	if (!ent->client->pers.weapon)
	{
		if (ent->client->newweapon)
			ChangeWeapon(ent);
		return;
	}

	// call active weapon think routine
	Weapon_RunThink(ent);

	// check remainder from haste; on 100ms/50ms server frames we may have
	// 'run next frame in' times that we can't possibly catch up to,
	// so we have to run them now.
	if (33_ms < FRAME_TIME_MS)
	{
		gtime_t relative_time = Weapon_AnimationTime(ent);

		if (relative_time < FRAME_TIME_MS)
		{
			// check how many we can't run before the next server tick
			gtime_t next_frame = level.time + FRAME_TIME_S;
			int64_t remaining_ms = (next_frame - ent->client->weapon_think_time).milliseconds();

			while (remaining_ms > 0)
			{
				ent->client->weapon_think_time -= relative_time;
				ent->client->weapon_fire_finished -= relative_time;
				Weapon_RunThink(ent);
				remaining_ms -= relative_time.milliseconds();
			}
		}
	}
}

enum weap_switch_t
{
	WEAP_SWITCH_ALREADY_USING,
	WEAP_SWITCH_NO_WEAPON,
	WEAP_SWITCH_NO_AMMO,
	WEAP_SWITCH_NOT_ENOUGH_AMMO,
	WEAP_SWITCH_VALID
};

weap_switch_t Weapon_AttemptSwitch(edict_t *ent, gitem_t *item, bool silent)
{
	if (ent->client->pers.weapon == item)
		return WEAP_SWITCH_ALREADY_USING;
	else if (!ent->client->pers.inventory[item->id])
		return WEAP_SWITCH_NO_WEAPON;

	//if (item->ammo && !g_select_empty->integer && !(item->flags & IF_AMMO))
	//{
	//	gitem_t *ammo_item = GetItemByIndex(item->ammo);

	//	if (!ent->client->pers.inventory[item->ammo])
	//	{
	//		if (!silent)
	//			gi.LocClient_Print(ent, PRINT_HIGH, "$g_no_ammo", ammo_item->pickup_name, item->pickup_name_definite);
	//		return WEAP_SWITCH_NO_AMMO;
	//	}
	//	else if (ent->client->pers.inventory[item->ammo] < item->quantity)
	//	{
	//		if (!silent)
	//			gi.LocClient_Print(ent, PRINT_HIGH, "$g_not_enough_ammo", ammo_item->pickup_name, item->pickup_name_definite);
	//		return WEAP_SWITCH_NOT_ENOUGH_AMMO;
	//	}
	//}

	return WEAP_SWITCH_VALID;
}

inline bool Weapon_IsPartOfChain(gitem_t *item, gitem_t *other)
{
	return other && other->chain && item->chain && other->chain == item->chain;
}

/*
================
Use_Weapon

Make the weapon ready if there is ammo
================
*/

// Vanilla Q2R Use_Weapon
//void Use_Weapon(edict_t *ent, gitem_t *item)
//{
//	gitem_t		*wanted, *root;
//	weap_switch_t result = WEAP_SWITCH_NO_WEAPON;
//
//	// if we're switching to a weapon in this chain already,
//	// start from the weapon after this one in the chain
//	if (!ent->client->no_weapon_chains && Weapon_IsPartOfChain(item, ent->client->newweapon))
//	{
//		root = ent->client->newweapon;
//		wanted = root->chain_next;
//	}
//	// if we're already holding a weapon in this chain,
//	// start from the weapon after that one
//	else if (!ent->client->no_weapon_chains && Weapon_IsPartOfChain(item, ent->client->pers.weapon))
//	{
//		root = ent->client->pers.weapon;
//		wanted = root->chain_next;
//	}
//	// start from beginning of chain (if any)
//	else
//		wanted = root = item;
//
//	while (true)
//	{
//		// try the weapon currently in the chain
//		if ((result = Weapon_AttemptSwitch(ent, wanted, false)) == WEAP_SWITCH_VALID)
//			break;
//
//		// no chains
//		if (!wanted->chain_next || ent->client->no_weapon_chains)
//			break;
//
//		wanted = wanted->chain_next;
//
//		// we wrapped back to the root item
//		if (wanted == root)
//			break;
//	}
//
//	if (result == WEAP_SWITCH_VALID)
//		ent->client->newweapon = wanted; // change to this weapon when down
//	else if ((result = Weapon_AttemptSwitch(ent, wanted, true)) == WEAP_SWITCH_NO_WEAPON && wanted != ent->client->pers.weapon && wanted != ent->client->newweapon)
//		gi.LocClient_Print(ent, PRINT_HIGH, "$g_out_of_item", wanted->pickup_name);
//}

// Action Use_Weapon
void Use_Weapon(edict_t* ent, gitem_t* item)
{
	if (item == ent->client->pers.weapon)
		return;

	ent->client->newweapon = item;
}

/*
================
Drop_Weapon
================
*/
void Drop_Weapon(edict_t *ent, gitem_t *item)
{
	item_id_t index = item->id;
	gitem_t* replacement = NULL;
	edict_t* temp = NULL;

	// see if we're already using it
	if (((item == ent->client->pers.weapon) || (item == ent->client->newweapon)) && (ent->client->pers.inventory[index] == 1))
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_cant_drop_weapon");
		return;
	}

	// Don't drop weaons if weapon stay is enabled
	if (g_dm_weapons_stay->integer)
		return;

	// AQ:TNG - JBravo fixing weapon farming
	if (ent->client->weaponstate == WEAPON_DROPPING ||
		ent->client->weaponstate == WEAPON_BUSY)
		return;
	// Weapon farming fix end.

	if (ent->client->weaponstate == WEAPON_BANDAGING ||
		ent->client->bandaging == 1 || ent->client->bandage_stopped == 1)
	{
		gi.LocClient_Print(ent, PRINT_HIGH,
			"You are too busy bandaging right now...\n");
		return;
	}

	if (ent->client->newweapon == item)
		return;

	if (item->id == IT_WEAPON_MK23)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "Can't drop the {}.\n", MK23_NAME);
		return;
	}
	else if (item->id == IT_WEAPON_MP5)
	{

		if (ent->client->pers.weapon == item && ent->client->inventory[index] == 1)
		{
			//replacement = GET_ITEM(MK23_NUM);	// back to the pistol then
			ent->client->newweapon = GetItemByIndex(IT_WEAPON_MK23);
			ent->client->weaponstate = WEAPON_DROPPING;
			ent->client->ps.gunframe = 51;

			//ChangeWeapon( ent );
		}
		ent->client->unique_weapon_total--;	// dropping 1 unique weapon
		temp = Drop_Item(ent, item);
		temp->think = temp_think_specweap;
		ent->client->inventory[index]--;
	}
	else if (item->id == IT_WEAPON_M4)
	{

		if (ent->client->pers.weapon == item && ent->client->inventory[index] == 1)
		{
			
			//replacement = GET_ITEM(MK23_NUM);	// back to the pistol then
			ent->client->newweapon = GetItemByIndex(IT_WEAPON_MK23); // back to the pistol then
			ent->client->weaponstate = WEAPON_DROPPING;
			ent->client->ps.gunframe = 44;

			//ChangeWeapon( ent );
		}
		ent->client->unique_weapon_total--;	// dropping 1 unique weapon
		temp = Drop_Item(ent, item);
		temp->think = temp_think_specweap;
		ent->client->inventory[index]--;
	}
	else if (item->id == IT_WEAPON_M3)
	{

		if (ent->client->pers.weapon == item && ent->client->inventory[index] == 1)
		{
			//replacement = GET_ITEM(MK23_NUM);	// back to the pistol then
			ent->client->newweapon = GetItemByIndex(IT_WEAPON_MK23); // back to the pistol then
			ent->client->weaponstate = WEAPON_DROPPING;
			ent->client->ps.gunframe = 41;
			//ChangeWeapon( ent );
		}
		ent->client->unique_weapon_total--;	// dropping 1 unique weapon
		temp = Drop_Item(ent, item);
		temp->think = temp_think_specweap;
		ent->client->inventory[index]--;
	}
	else if (item->id == IT_WEAPON_HANDCANNON)
	{
		if (ent->client->pers.weapon == item && ent->client->inventory[index] == 1)
		{
			//replacement = GET_ITEM(MK23_NUM);	// back to the pistol then
			ent->client->newweapon = GetItemByIndex(IT_WEAPON_MK23); // back to the pistol then
			ent->client->weaponstate = WEAPON_DROPPING;
			ent->client->ps.gunframe = 61;
			//ChangeWeapon( ent );
		}
		ent->client->unique_weapon_total--;	// dropping 1 unique weapon
		temp = Drop_Item(ent, item);
		temp->think = temp_think_specweap;
		ent->client->inventory[index]--;
	}
	else if (item->id == IT_WEAPON_SNIPER)
	{
		if (ent->client->pers.weapon == item && ent->client->inventory[index] == 1)
		{
			// in case they are zoomed in
			ent->client->ps.fov = 90;
			ent->client->desired_fov = 90;
			//replacement = GET_ITEM(MK23_NUM);	// back to the pistol then
			ent->client->newweapon = GetItemByIndex(IT_WEAPON_MK23); // back to the pistol then
			ent->client->weaponstate = WEAPON_DROPPING;
			ent->client->ps.gunframe = 50;
			//ChangeWeapon( ent );
		}
		ent->client->unique_weapon_total--;	// dropping 1 unique weapon
		temp = Drop_Item(ent, item);
		temp->think = temp_think_specweap;
		ent->client->inventory[index]--;
	}
	else if (item->id == IT_WEAPON_DUALMK23)
	{
		if (ent->client->pers.weapon == item)
		{
			//replacement = GET_ITEM(MK23_NUM);	// back to the pistol then
			ent->client->newweapon = GetItemByIndex(IT_WEAPON_MK23); // back to the pistol then
			ent->client->weaponstate = WEAPON_DROPPING;
			ent->client->ps.gunframe = 40;
			//ChangeWeapon( ent );
		}
		ent->client->dual_rds = ent->client->mk23_rds;
	}
	else if (item->id == IT_WEAPON_KNIFE)
	{
		//gi.cprintf(ent, PRINT_HIGH, "Before checking knife inven frames = %d\n", ent->client->ps.gunframe);

		if (ent->client->pers.weapon == item && ent->client->inventory[index] == 1)
		{
			//replacement = GET_ITEM(MK23_NUM);	// back to the pistol then
			ent->client->newweapon = GetItemByIndex(IT_WEAPON_MK23); // back to the pistol then
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
	else if (item->id == IT_WEAPON_GRENADES)
	{
		if (ent->client->pers.weapon == item && ent->client->inventory[index] == 1)
		{
			if (((ent->client->ps.gunframe >= GRENADE_IDLE_FIRST)
				&& (ent->client->ps.gunframe <= GRENADE_IDLE_LAST))
				|| ((ent->client->ps.gunframe >= GRENADE_THROW_FIRST
					&& ent->client->ps.gunframe <= GRENADE_THROW_LAST)))
			{
				int damage;

				ent->client->ps.gunframe = 0;

				// Reset Grenade Damage to 1.52 when requested:
				// if (use_classic->value)
				// 	damage = GRENADE_DAMRAD_CLASSIC;
				// else
				damage = GRENADE_DAMRAD;

				fire_grenade2(ent, ent->s.origin, vec3_origin, damage, 0, 80_ms, damage * 2, false);

				INV_AMMO(ent, GRENADE_NUM)--;
				ent->client->newweapon = GetItemByIndex(IT_WEAPON_MK23); // back to the pistol then
				ent->client->weaponstate = WEAPON_DROPPING;
				ent->client->ps.gunframe = 0;
				return;
			}

			//replacement = GET_ITEM(MK23_NUM);	// back to the pistol then
			ent->client->newweapon = GetItemByIndex(IT_WEAPON_MK23); // back to the pistol then
			ent->client->weaponstate = WEAPON_DROPPING;
			ent->client->ps.gunframe = 0;
			//ChangeWeapon( ent );
		}
	}
	else if ((item == ent->client->pers.weapon || item == ent->client->newweapon)
		&& ent->client->inventory[index] == 1)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "Can't drop current weapon\n");
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

	edict_t *drop = Drop_Item(ent, item);
	drop->spawnflags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
	drop->svflags &= ~SVF_INSTANCED;
	ent->client->pers.inventory[index]--;
}

void Weapon_PowerupSound(edict_t *ent)
{
	// if (!CTFApplyStrengthSound(ent))
	// {
	// 	if (ent->client->quad_time > level.time && ent->client->double_time > level.time)
	// 		gi.sound(ent, CHAN_ITEM, gi.soundindex("ctf/tech2x.wav"), 1, ATTN_NORM, 0);
	// 	else if (ent->client->quad_time > level.time)
	// 		gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);
	// 	else if (ent->client->double_time > level.time)
	// 		gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/ddamage3.wav"), 1, ATTN_NORM, 0);
	// 	else if (ent->client->quadfire_time > level.time
	// 		&& ent->client->ctf_techsndtime < level.time)
	// 	{
	// 		ent->client->ctf_techsndtime = level.time + 1_sec;
	// 		gi.sound(ent, CHAN_ITEM, gi.soundindex("ctf/tech3.wav"), 1, ATTN_NORM, 0);
	// 	}
	// }

	//CTFApplyHasteSound(ent);
}

inline bool Weapon_CanAnimate(edict_t *ent)
{
	// VWep animations screw up corpses
	return !ent->deadflag && ent->s.modelindex == MODELINDEX_PLAYER;
}

// [Paril-KEX] called when finished to set time until
// we're allowed to switch to fire again
inline void Weapon_SetFinished(edict_t *ent)
{
	ent->client->weapon_fire_finished = level.time + Weapon_AnimationTime(ent);
}

inline bool Weapon_HandleDropping(edict_t *ent, int FRAME_DEACTIVATE_LAST)
{
	if (ent->client->weaponstate == WEAPON_DROPPING)
	{
		if (ent->client->weapon_think_time <= level.time)
		{
			if (ent->client->ps.gunframe == FRAME_DEACTIVATE_LAST)
			{
				ChangeWeapon(ent);
				return true;
			}
			else if ((FRAME_DEACTIVATE_LAST - ent->client->ps.gunframe) == 4)
			{
				ent->client->anim_priority = ANIM_ATTACK | ANIM_REVERSED;
				if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
				{
					ent->s.frame = FRAME_crpain4 + 1;
					ent->client->anim_end = FRAME_crpain1;
				}
				else
				{
					ent->s.frame = FRAME_pain304 + 1;
					ent->client->anim_end = FRAME_pain301;
				}
				ent->client->anim_time = 0_ms;
			}

			ent->client->ps.gunframe++;
			ent->client->weapon_think_time = level.time + Weapon_AnimationTime(ent);
		}
		return true;
	}

	return false;
}

inline bool Weapon_HandleActivating(edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_IDLE_FIRST)
{
	if (ent->client->weaponstate == WEAPON_ACTIVATING)
	{
		if (ent->client->weapon_think_time <= level.time || g_instant_weapon_switch->integer)
		{
			ent->client->weapon_think_time = level.time + Weapon_AnimationTime(ent);

			if (ent->client->ps.gunframe == FRAME_ACTIVATE_LAST || g_instant_weapon_switch->integer)
			{
				ent->client->weaponstate = WEAPON_READY;
				ent->client->ps.gunframe = FRAME_IDLE_FIRST;
				ent->client->weapon_fire_buffered = false;
				if (!g_instant_weapon_switch->integer)
					Weapon_SetFinished(ent);
				else
					ent->client->weapon_fire_finished = 0_ms;
				return true;
			}

			ent->client->ps.gunframe++;
			return true;
		}
	}

	return false;
}

inline bool Weapon_HandleNewWeapon(edict_t *ent, int FRAME_DEACTIVATE_FIRST, int FRAME_DEACTIVATE_LAST)
{
	bool is_holstering = false;

	if (!g_instant_weapon_switch->integer)
		is_holstering = ((ent->client->latched_buttons | ent->client->buttons) & BUTTON_HOLSTER);

	if ((ent->client->newweapon || is_holstering) && (ent->client->weaponstate != WEAPON_FIRING))
	{
		if (g_instant_weapon_switch->integer || ent->client->weapon_think_time <= level.time)
		{
			if (!ent->client->newweapon)
				ent->client->newweapon = ent->client->pers.weapon;

			ent->client->weaponstate = WEAPON_DROPPING;

			if (g_instant_weapon_switch->integer)
			{
				ChangeWeapon(ent);
				return true;
			}

			ent->client->ps.gunframe = FRAME_DEACTIVATE_FIRST;

			if ((FRAME_DEACTIVATE_LAST - FRAME_DEACTIVATE_FIRST) < 4)
			{
				ent->client->anim_priority = ANIM_ATTACK | ANIM_REVERSED;
				if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
				{
					ent->s.frame = FRAME_crpain4 + 1;
					ent->client->anim_end = FRAME_crpain1;
				}
				else
				{
					ent->s.frame = FRAME_pain304 + 1;
					ent->client->anim_end = FRAME_pain301;
				}
				ent->client->anim_time = 0_ms;
			}

			ent->client->weapon_think_time = level.time + Weapon_AnimationTime(ent);
		}
		return true;
	}

	return false;
}

enum weapon_ready_state_t
{
	READY_NONE,
	READY_CHANGING,
	READY_FIRING
};

inline weapon_ready_state_t Weapon_HandleReady(edict_t *ent, int FRAME_FIRE_FIRST, int FRAME_IDLE_FIRST, int FRAME_IDLE_LAST, const int *pause_frames)
{
	if (ent->client->weaponstate == WEAPON_READY)
	{
		bool request_firing = ent->client->weapon_fire_buffered || ((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK);

		if (request_firing && ent->client->weapon_fire_finished <= level.time)
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;
			ent->client->weapon_think_time = level.time;

			if ((!ent->client->pers.weapon->ammo) ||
				(ent->client->pers.inventory[ent->client->pers.weapon->ammo] >= ent->client->pers.weapon->quantity))
			{
				ent->client->weaponstate = WEAPON_FIRING;
				return READY_FIRING;
			}
			else
			{
				NoAmmoWeaponChange(ent, true);
				return READY_CHANGING;
			}
		}
		else if (ent->client->weapon_think_time <= level.time)
		{
			ent->client->weapon_think_time = level.time + Weapon_AnimationTime(ent);

			if (ent->client->ps.gunframe == FRAME_IDLE_LAST)
			{
				ent->client->ps.gunframe = FRAME_IDLE_FIRST;
				return READY_CHANGING;
			}

			if (pause_frames)
				for (int n = 0; pause_frames[n]; n++)
					if (ent->client->ps.gunframe == pause_frames[n])
						if (irandom(16))
							return READY_CHANGING;

			ent->client->ps.gunframe++;
			return READY_CHANGING;
		}
	}

	return READY_NONE;
}

inline void Weapon_HandleFiring(edict_t *ent, int32_t FRAME_IDLE_FIRST, std::function<void()> fire_handler)
{
	Weapon_SetFinished(ent);

	if (ent->client->weapon_fire_buffered)
	{
		ent->client->buttons |= BUTTON_ATTACK;
		ent->client->weapon_fire_buffered = false;
	}

	fire_handler();

	if (ent->client->ps.gunframe == FRAME_IDLE_FIRST)
	{
		ent->client->weaponstate = WEAPON_READY;
		ent->client->weapon_fire_buffered = false;
	}

	ent->client->weapon_think_time = level.time + Weapon_AnimationTime(ent);
}

// Action Add
// #define FRAME_FIRE_FIRST                (FRAME_ACTIVATE_LAST + 1)
// #define FRAME_IDLE_FIRST                (FRAME_FIRE_LAST + 1)
// #define FRAME_DEACTIVATE_FIRST  (FRAME_IDLE_LAST + 1)

#define FRAME_RELOAD_FIRST              (FRAME_DEACTIVATE_LAST +1)
#define FRAME_LASTRD_FIRST   (FRAME_RELOAD_LAST +1)

#define MK23MAG 12
#define MP5MAG  30
#define M4MAG   24
#define DUALMAG 24

// Action End

void Weapon_Generic(edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST, int FRAME_RELOAD_LAST, int FRAME_LASTRD_LAST, const int *pause_frames, const int *fire_frames, void (*fire)(edict_t *ent))
{
	int FRAME_FIRE_FIRST = (FRAME_ACTIVATE_LAST + 1);
	int FRAME_IDLE_FIRST = (FRAME_FIRE_LAST + 1);
	int FRAME_DEACTIVATE_FIRST = (FRAME_IDLE_LAST + 1);

	int n;
	int bFire = 0;
	int bOut = 0;

	if (level.time < ent->client->weapon_think_time) {
		return;
	}
	ent->client->weapon_think_time = level.time + 100_ms;
	  // zucc vwep
	if (ent->s.modelindex != 255)
		return;			// not on client, so VWep animations could do wacky things

	if (ent->client->ctf_grapple && ent->client->pers.weapon) {
		if (strcmp(ent->client->pers.weapon->pickup_name, "Grapple") == 0 &&
			ent->client->weaponstate == WEAPON_FIRING)
			return;
	}

	//FIREBLADE
	if (ent->client->weaponstate == WEAPON_FIRING &&
		//((ent->solid == SOLID_NOT && ent->deadflag != DEAD_DEAD) ||
		(IS_ALIVE(ent) ||
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
			switch (ent->client->pers.weapon->id)
			{
				//+BD - Check weapon to find out when to play reload sounds
			case IT_WEAPON_MK23:
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
			case IT_WEAPON_MP5:
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
			case IT_WEAPON_M4:
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
			case IT_WEAPON_M3:
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
			case IT_WEAPON_HANDCANNON:
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
			case IT_WEAPON_SNIPER:
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
			case IT_WEAPON_DUALMK23:
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
				gi.Com_Print("No weapon choice for reloading (sounds).\n");
				break;

			}
		}
		else
		{
			ent->client->ps.gunframe = FRAME_IDLE_FIRST;
			ent->client->weaponstate = WEAPON_READY;
			switch (ent->client->pers.weapon->id)
			{
			case IT_WEAPON_MK23:
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
			case IT_WEAPON_MP5:
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
			case IT_WEAPON_M4:
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
			case IT_WEAPON_M3:
			{
				ent->client->shot_rds++;
				(ent->client->inventory[ent->client->ammo_index])--;
				if (ent->client->inventory[ent->client->ammo_index] < 0)
				{
					ent->client->inventory[ent->client->ammo_index] = 0;
				}
				break;
			}
			case IT_WEAPON_HANDCANNON:
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
			case IT_WEAPON_SNIPER:
			{
				ent->client->sniper_rds++;
				(ent->client->inventory[ent->client->ammo_index])--;
				if (ent->client->inventory[ent->client->ammo_index] < 0)
				{
					ent->client->inventory[ent->client->ammo_index] = 0;
				}
				return;
			}
			case IT_WEAPON_DUALMK23:
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
				gi.Com_Print("No weapon choice for reloading.\n");
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
			((ent->client->pers.weapon->id == IT_WEAPON_MK23)
				&& (ent->client->mk23_rds > 0))
			|| ((ent->client->pers.weapon->id == IT_WEAPON_MP5)
				&& (ent->client->mp5_rds > 0))
			|| ((ent->client->pers.weapon->id == IT_WEAPON_M4) && (ent->client->m4_rds > 0))
			|| ((ent->client->pers.weapon->id == IT_WEAPON_M3)
				&& (ent->client->shot_rds > 0))
			|| ((ent->client->pers.weapon->id == IT_WEAPON_HANDCANNON)
				&& (ent->client->cannon_rds > 0))
			|| ((ent->client->pers.weapon->id == IT_WEAPON_SNIPER)
				&& (ent->client->sniper_rds > 0))
			|| ((ent->client->pers.weapon->id == IT_WEAPON_DUALMK23)
				&& (ent->client->dual_rds > 0)))
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = FRAME_IDLE_FIRST;
		}
		if (((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK)
			&& (ent->solid != SOLID_NOT || ent->deadflag)
			&& !lights_camera_action && !ent->client->uvTime)
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;
			{
				if (level.time >= ent->touch_debounce_time)
				{
					gi.sound(ent, CHAN_VOICE, level.snd_noammo, 1, ATTN_NORM, 0);
					ent->touch_debounce_time = level.time + 1_sec;
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
			// if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			// 	SetAnimation( ent, FRAME_crpain4 + 1, FRAME_crpain1, ANIM_REVERSE );
			// else
			// 	SetAnimation( ent, FRAME_pain304 + 1, FRAME_pain301, ANIM_REVERSE );
		
			// if (ent->client->anim_priority != ANIM_ATTACK || frandom() < 0.25f)
	
			ent->client->anim_priority = ANIM_REVERSED;
			if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent->s.frame = FRAME_crpain4 + 1;
				ent->client->anim_end = FRAME_crpain1;
			}
			else
			{
				ent->s.frame = FRAME_pain304 + 1;
				ent->client->anim_end = FRAME_pain301;
			}
			ent->client->anim_time = 0_ms;

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
		switch (ent->client->pers.weapon->id)
		{
		case IT_WEAPON_MK23:
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
		case IT_WEAPON_MP5:
		{
			if (ent->client->ps.gunframe == 3)	// 3
				gi.sound(ent, CHAN_WEAPON,
					gi.soundindex("weapons/mp5slide.wav"), 1, ATTN_NORM,
					0);
			ent->client->fired = 0;	//reset any firing delays
			ent->client->burst = 0;
			break;
		}
		case IT_WEAPON_M4:
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
		case IT_WEAPON_M3:
		{
			ent->client->fired = 0;	//reset any firing delays
			ent->client->burst = 0;
			ent->client->fast_reload = 0;
			break;
		}
		case IT_WEAPON_HANDCANNON:
		{
			ent->client->fired = 0;	//reset any firing delays
			ent->client->burst = 0;
			break;
		}
		case IT_WEAPON_SNIPER:
		{
			ent->client->fired = 0;	//reset any firing delays
			ent->client->burst = 0;
			ent->client->fast_reload = 0;
			break;
		}
		case IT_WEAPON_DUALMK23:
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

		default:
			gi.Com_Print("Activated unknown weapon.\n");
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

		if (ent->client->curr_weap.id == IT_WEAPON_SNIPER)
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
		if (ent->client->pers.weapon)
			ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);

		// zucc more vwep stuff
		if ((FRAME_DEACTIVATE_LAST - FRAME_DEACTIVATE_FIRST) < 4)
		{
			// if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			// 	SetAnimation( ent, FRAME_crpain4 + 1, FRAME_crpain1, ANIM_REVERSE );
			// else
			// 	SetAnimation( ent, FRAME_pain304 + 1, FRAME_pain301, ANIM_REVERSE );
		
			ent->client->anim_priority = ANIM_REVERSED;
			if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent->s.frame = FRAME_crpain4 + 1;
				ent->client->anim_end = FRAME_crpain1;
			}
			else
			{
				ent->s.frame = FRAME_pain304 + 1;
				ent->client->anim_end = FRAME_pain301;
			}
			ent->client->anim_time = 0_ms;
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
			&& (ent->solid != SOLID_NOT || ent->deadflag)
			&& !lights_camera_action && (!ent->client->uvTime || dm_shield->value > 1))
		{
			if (ent->client->uvTime) {
				ent->client->uvTime = 0;
				gi.Client_Print(ent, PRINT_CENTER, "Shields are DOWN!");
			}
			ent->client->latched_buttons &= ~BUTTON_ATTACK;
			switch (ent->client->pers.weapon->id)
			{
			case IT_WEAPON_MK23:
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
			case IT_WEAPON_MP5:
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
						// if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
						// 	SetAnimation( ent, FRAME_crattak1 - 1, FRAME_crattak9, ANIM_ATTACK );
						// else
						// 	SetAnimation( ent, FRAME_attack1 - 1, FRAME_attack8, ANIM_ATTACK );
					
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
						ent->client->anim_time = 0_ms;
					
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
			case IT_WEAPON_M4:
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
						// if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
						// 	SetAnimation( ent, FRAME_crattak1 - 1, FRAME_crattak9, ANIM_ATTACK );
						// else
						// 	SetAnimation( ent, FRAME_attack1 - 1, FRAME_attack8, ANIM_ATTACK );

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
						ent->client->anim_time = 0_ms;
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
			case IT_WEAPON_M3:
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
			case IT_WEAPON_HANDCANNON:

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
			// AQ2:TNG END
			case IT_WEAPON_SNIPER:
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
			case IT_WEAPON_DUALMK23:
			{
				if (ent->client->dual_rds > 0)
				{
					bFire = 1;
				}
				else
					bOut = 1;
				break;
			}

			default:
			{
				gi.LocClient_Print(ent, PRINT_HIGH,
					"Calling non specific ammo code\n");
				if ((!ent->client->ammo_index)
					|| (ent->client->inventory[ent->client->ammo_index] >=
						ent->client->pers.weapon->quantity))
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
				// if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
				// 	SetAnimation( ent, FRAME_crattak1 - 1, FRAME_crattak9, ANIM_ATTACK );
				// else
				// 	SetAnimation( ent, FRAME_attack1 - 1, FRAME_attack8, ANIM_ATTACK );

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
				ent->client->anim_time = 0_ms;
			}
			else if (bOut)	// out of ammo
			{
				if (level.time >= ent->touch_debounce_time)
				{
					gi.sound(ent, CHAN_VOICE, level.snd_noammo, 1, ATTN_NORM, 0);
					ent->touch_debounce_time = level.time + 1_sec;
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
				// No quad in Action
				// if (ent->client->quad_framenum > level.framenum)
				// 	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"),
				// 		1, ATTN_NORM, 0);

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
				// No quad in Action
				// if (ent->client->quad_framenum > level.framenum)
				// 	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"),
				// 		1, ATTN_NORM, 0);
				fire(ent);
				break;
			}
		}

		if (!fire_frames[n])
			ent->client->ps.gunframe++;

		//gi.cprintf (ent, PRINT_HIGH, "Calling Q_stricmp, frame = %d.\n", ent->client->ps.gunframe);


		if (ent->client->ps.gunframe == FRAME_IDLE_FIRST + 1)
			ent->client->weaponstate = WEAPON_READY;

		if (ent->client->curr_weap.id == IT_WEAPON_MP5)
		{
			if (ent->client->ps.gunframe >= 76)
			{
				ent->client->weaponstate = WEAPON_READY;
				ent->client->ps.gunframe = FRAME_IDLE_FIRST + 1;
			}
			//      gi.cprintf (ent, PRINT_HIGH, "Succes Q_stricmp now: frame = %d.\n", ent->client->ps.gunframe);

		}
		if (ent->client->curr_weap.id == IT_WEAPON_M4)
		{
			if (ent->client->ps.gunframe >= 69)
			{
				ent->client->weaponstate = WEAPON_READY;
				ent->client->ps.gunframe = FRAME_IDLE_FIRST + 1;
			}
			//      gi.cprintf (ent, PRINT_HIGH, "Succes Q_stricmp now: frame = %d.\n", ent->client->ps.gunframe);

		}

	}



	// Vanilla Q2R
	// if (!Weapon_CanAnimate(ent))
	// 	return;

	// if (Weapon_HandleDropping(ent, FRAME_DEACTIVATE_LAST))
	// 	return;
	// else if (Weapon_HandleActivating(ent, FRAME_ACTIVATE_LAST, FRAME_IDLE_FIRST))
	// 	return;
	// else if (Weapon_HandleNewWeapon(ent, FRAME_DEACTIVATE_FIRST, FRAME_DEACTIVATE_LAST))
	// 	return;
	// else if (auto state = Weapon_HandleReady(ent, FRAME_FIRE_FIRST, FRAME_IDLE_FIRST, FRAME_IDLE_LAST, pause_frames))
	// {
	// 	if (state == READY_FIRING)
	// 	{
	// 		ent->client->ps.gunframe = FRAME_FIRE_FIRST;
	// 		ent->client->weapon_fire_buffered = false;

	// 		if (ent->client->weapon_thunk)
	// 			ent->client->weapon_think_time += FRAME_TIME_S;

	// 		ent->client->weapon_think_time += Weapon_AnimationTime(ent);
	// 		Weapon_SetFinished(ent);

	// 		for (int n = 0; fire_frames[n]; n++)
	// 		{
	// 			if (ent->client->ps.gunframe == fire_frames[n])
	// 			{
 	// 				Weapon_PowerupSound(ent);
	// 				fire(ent);
	// 				break;
	// 			}
	// 		}

	// 		// start the animation
	// 		ent->client->anim_priority = ANIM_ATTACK;
	// 		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	// 		{
	// 			ent->s.frame = FRAME_crattak1 - 1;
	// 			ent->client->anim_end = FRAME_crattak9;
	// 		}
	// 		else
	// 		{
	// 			ent->s.frame = FRAME_attack1 - 1;
	// 			ent->client->anim_end = FRAME_attack8;
	// 		}
	// 		ent->client->anim_time = 0_ms;
	// 	}

	// 	return;
	// }

	// if (ent->client->weaponstate == WEAPON_FIRING && ent->client->weapon_think_time <= level.time)
	// {
	// 	ent->client->ps.gunframe++;
	// 	Weapon_HandleFiring(ent, FRAME_IDLE_FIRST, [&]() {
	// 		for (int n = 0; fire_frames[n]; n++)
	// 		{
	// 			if (ent->client->ps.gunframe == fire_frames[n])
	// 			{
	// 				Weapon_PowerupSound(ent);
	// 				fire(ent);
	// 				break;
	// 			}
	// 		}
	// 	});
	// }
}

void Weapon_Repeating(edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST, const int *pause_frames, void (*fire)(edict_t *ent))
{
	int FRAME_FIRE_FIRST = (FRAME_ACTIVATE_LAST + 1);
	int FRAME_IDLE_FIRST = (FRAME_FIRE_LAST + 1);
	int FRAME_DEACTIVATE_FIRST = (FRAME_IDLE_LAST + 1);

	if (!Weapon_CanAnimate(ent))
		return;

	if (Weapon_HandleDropping(ent, FRAME_DEACTIVATE_LAST))
		return;
	else if (Weapon_HandleActivating(ent, FRAME_ACTIVATE_LAST, FRAME_IDLE_FIRST))
		return;
	else if (Weapon_HandleNewWeapon(ent, FRAME_DEACTIVATE_FIRST, FRAME_DEACTIVATE_LAST))
		return;
	else if (Weapon_HandleReady(ent, FRAME_FIRE_FIRST, FRAME_IDLE_FIRST, FRAME_IDLE_LAST, pause_frames) == READY_CHANGING)
		return;

	if (ent->client->weaponstate == WEAPON_FIRING && ent->client->weapon_think_time <= level.time)
	{
		Weapon_HandleFiring(ent, FRAME_IDLE_FIRST, [&]() { fire(ent); });

		if (ent->client->weapon_thunk)
			ent->client->weapon_think_time += FRAME_TIME_S;
	}
}

void DropSpecialWeapon(edict_t* ent)
{
	int itemNum = ent->client->pers.weapon->id;

	// first check if their current weapon is a special weapon, if so, drop it.
	if (itemNum >= IT_WEAPON_MP5 && itemNum <= IT_WEAPON_SNIPER)
		Drop_Weapon(ent, ent->client->pers.weapon);
	else if (INV_AMMO(ent, IT_WEAPON_SNIPER) > 0)
		Drop_Weapon(ent, GET_ITEM(IT_WEAPON_SNIPER));
	else if (INV_AMMO(ent, IT_WEAPON_HANDCANNON) > 0)
		Drop_Weapon(ent, GET_ITEM(IT_WEAPON_HANDCANNON));
	else if (INV_AMMO(ent, IT_WEAPON_M3) > 0)
		Drop_Weapon(ent, GET_ITEM(IT_WEAPON_M3));
	else if (INV_AMMO(ent, IT_WEAPON_MP5) > 0)
		Drop_Weapon(ent, GET_ITEM(IT_WEAPON_MP5));
	else if (INV_AMMO(ent, IT_WEAPON_M4) > 0)
		Drop_Weapon(ent, GET_ITEM(IT_WEAPON_M4));
	// special case, aq does this, guess I can support it
	else if (itemNum == IT_WEAPON_DUALMK23)
		ent->client->newweapon = GET_ITEM(IT_WEAPON_MK23);

}

/*
======================================================================

GRENADE

======================================================================
*/

void weapon_grenade_fire(edict_t *ent, bool held)
{
	vec3_t offset;
	vec3_t forward, right;
	vec3_t start;
	int damage = 125;
	int speed;
	int damrad = GRENADE_DAMRAD;

	if (is_quad)
		damage *= 4;

	VectorSet(offset, 8, 8, ent->viewheight - 8);
	AngleVectors(ent->client->v_angle, forward, right, NULL);
	//P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
	P_ProjectSource(ent, ent->s.origin, offset, right, forward);

	gtime_t timer = ent->client->grenade_time - level.time;
	//speed = GRENADE_MINSPEED + (GRENADE_TIMER - timer * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER));
	speed = (int) (ent->health <= 0 ? GRENADE_MINSPEED : min(GRENADE_MINSPEED + (GRENADE_TIMER - timer).seconds() * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER.seconds()), GRENADE_MAXSPEED));
	// Reset Grenade Damage to 1.52 when requested:
	//damrad = use_classic->value ? GRENADE_DAMRAD_CLASSIC : GRENADE_DAMRAD;
	damrad = GRENADE_DAMRAD;
	fire_grenade2(ent, start, forward, damrad, speed, timer, damrad * 2, held);

	if (!g_infinite_ammo->integer)
		ent->client->inventory[ent->client->ammo_index]--;

	ent->client->grenade_time = level.time + 3_sec;

	// Vanilla Q2R
	// int	  damage = 125;
	// int	  speed;
	// float radius;

	// radius = (float) (damage + 40);
	// if (is_quad)
	// 	damage *= damage_multiplier;

	// vec3_t start, dir;
	// // Paril: kill sideways angle on grenades
	// // limit upwards angle so you don't throw behind you
	// P_ProjectSource(ent, { max(-62.5f, ent->client->v_angle[0]), ent->client->v_angle[1], ent->client->v_angle[2] }, { 2, 0, -14 }, start, dir);

	// gtime_t timer = ent->client->grenade_time - level.time;
	// speed = (int) (ent->health <= 0 ? GRENADE_MINSPEED : min(GRENADE_MINSPEED + (GRENADE_TIMER - timer).seconds() * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER.seconds()), GRENADE_MAXSPEED));

	// ent->client->grenade_time = 0_ms;

	// fire_grenade2(ent, start, dir, damage, speed, timer, radius, held);

	// G_RemoveAmmo(ent, 1);
}

void Throw_Generic(edict_t *ent, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_PRIME_SOUND,
					const char *prime_sound,
					int FRAME_THROW_HOLD, int FRAME_THROW_FIRE, const int *pause_frames, int EXPLODE,
					const char *primed_sound,
					void (*fire)(edict_t *ent, bool held), bool extra_idle_frame)
{
	// when we die, just toss what we had in our hands.
	if (ent->health <= 0)
	{
		fire(ent, true);
		return;
	}

	int n;
	int FRAME_IDLE_FIRST = (FRAME_FIRE_LAST + 1);

	if (ent->client->newweapon && (ent->client->weaponstate == WEAPON_READY))
	{
		if (ent->client->weapon_think_time <= level.time)
		{
			ChangeWeapon(ent);
			ent->client->weapon_think_time = level.time + Weapon_AnimationTime(ent);
		}
		return;
	}

	if (ent->client->weaponstate == WEAPON_ACTIVATING)
	{
		if (ent->client->weapon_think_time <= level.time)
		{
			ent->client->weaponstate = WEAPON_READY;
			if (!extra_idle_frame)
				ent->client->ps.gunframe = FRAME_IDLE_FIRST;
			else
				ent->client->ps.gunframe = FRAME_IDLE_LAST + 1;
			ent->client->weapon_think_time = level.time + Weapon_AnimationTime(ent);
			Weapon_SetFinished(ent);
		}
		return;
	}

	if (ent->client->weaponstate == WEAPON_READY)
	{
		bool request_firing = ent->client->weapon_fire_buffered || ((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK);

		if (request_firing && ent->client->weapon_fire_finished <= level.time)
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;

			if (ent->client->pers.inventory[ent->client->pers.weapon->ammo])
			{
				ent->client->ps.gunframe = 1;
				ent->client->weaponstate = WEAPON_FIRING;
				ent->client->grenade_time = 0_ms;
				ent->client->weapon_think_time = level.time + Weapon_AnimationTime(ent);
			}
			else
				NoAmmoWeaponChange(ent, true);
			return;
		}
		else if (ent->client->weapon_think_time <= level.time)
		{
			ent->client->weapon_think_time = level.time + Weapon_AnimationTime(ent);

			if (ent->client->ps.gunframe >= FRAME_IDLE_LAST)
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
						if (irandom(16))
							return;
					}
				}
			}

			ent->client->ps.gunframe++;
		}
		return;
	}

	if (ent->client->weaponstate == WEAPON_FIRING)
	{
		if (ent->client->weapon_think_time <= level.time)
		{
			if (prime_sound && ent->client->ps.gunframe == FRAME_PRIME_SOUND)
				gi.sound(ent, CHAN_WEAPON, gi.soundindex(prime_sound), 1, ATTN_NORM, 0);

			// [Paril-KEX] dualfire/time accel
			gtime_t grenade_wait_time = 1_sec;

			// if (CTFApplyHaste(ent))
			// 	grenade_wait_time *= 0.5f;
			// if (is_quadfire)
			// 	grenade_wait_time *= 0.5f;

			if (ent->client->ps.gunframe == FRAME_THROW_HOLD)
			{
				if (!ent->client->grenade_time && !ent->client->grenade_finished_time)
				{
					ent->client->grenade_time = level.time + GRENADE_TIMER + 200_ms;

					if (primed_sound)
						ent->client->weapon_sound = gi.soundindex(primed_sound);
				}

				// they waited too long, detonate it in their hand
				if (EXPLODE && !ent->client->grenade_blew_up && level.time >= ent->client->grenade_time)
				{
					Weapon_PowerupSound(ent);
					ent->client->weapon_sound = 0;
					fire(ent, true);
					ent->client->grenade_blew_up = true;

					ent->client->grenade_finished_time = level.time + grenade_wait_time;
				}

				if (ent->client->buttons & BUTTON_ATTACK)
				{
					ent->client->weapon_think_time = level.time + 1_ms;
					return;
				}

				if (ent->client->grenade_blew_up)
				{
					if (level.time >= ent->client->grenade_finished_time)
					{
						ent->client->ps.gunframe = FRAME_FIRE_LAST;
						ent->client->grenade_blew_up = false;
						ent->client->weapon_think_time = level.time + Weapon_AnimationTime(ent);
					}
					else
					{
						return;
					}
				}
				else
				{
					ent->client->ps.gunframe++;

					Weapon_PowerupSound(ent);
					ent->client->weapon_sound = 0;
					fire(ent, false);

					if (!EXPLODE || !ent->client->grenade_blew_up)
						ent->client->grenade_finished_time = level.time + grenade_wait_time;

					if (!ent->deadflag && ent->s.modelindex == MODELINDEX_PLAYER && ent->health > 0) // VWep animations screw up corpses
					{
						if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
						{
							ent->client->anim_priority = ANIM_ATTACK;
							ent->s.frame = FRAME_crattak1 - 1;
							ent->client->anim_end = FRAME_crattak3;
						}
						else
						{
							ent->client->anim_priority = ANIM_ATTACK | ANIM_REVERSED;
							ent->s.frame = FRAME_wave08;
							ent->client->anim_end = FRAME_wave01;
						}
						ent->client->anim_time = 0_ms;
					}
				}
			}

			ent->client->weapon_think_time = level.time + Weapon_AnimationTime(ent);

			if ((ent->client->ps.gunframe == FRAME_FIRE_LAST) && (level.time < ent->client->grenade_finished_time))
				return;

			ent->client->ps.gunframe++;

			if (ent->client->ps.gunframe == FRAME_IDLE_FIRST)
			{
				ent->client->grenade_finished_time = 0_ms;
				ent->client->weaponstate = WEAPON_READY;
				ent->client->weapon_fire_buffered = false;
				Weapon_SetFinished(ent);
				
				if (extra_idle_frame)
					ent->client->ps.gunframe = FRAME_IDLE_LAST + 1;

				// Paril: if we ran out of the throwable, switch
				// so we don't appear to be holding one that we
				// can't throw
				if (!ent->client->pers.inventory[ent->client->pers.weapon->ammo])
				{
					NoAmmoWeaponChange(ent, false);
					ChangeWeapon(ent);
				}
			}
		}
	}
}

void Weapon_Grenade(edict_t *ent)
{
	constexpr int pause_frames[] = { 29, 34, 39, 48, 0 };

	Throw_Generic(ent, 15, 48, 5, "weapons/hgrena1b.wav", 11, 12, pause_frames, true, "weapons/hgrenc1b.wav", weapon_grenade_fire, true);

	// [Paril-KEX] skip the duped frame
	if (ent->client->ps.gunframe == 1)
		ent->client->ps.gunframe = 2;
}

/*
======================================================================

GRENADE LAUNCHER

======================================================================
*/

void weapon_grenadelauncher_fire(edict_t *ent)
{
	int	  damage = 120;
	float radius;

	radius = (float) (damage + 40);
	if (is_quad)
		damage *= damage_multiplier;

	vec3_t start, dir;
	// Paril: kill sideways angle on grenades
	// limit upwards angle so you don't fire it behind you
	P_ProjectSource(ent, { max(-62.5f, ent->client->v_angle[0]), ent->client->v_angle[1], ent->client->v_angle[2] }, { 8, 0, -8 }, start, dir);

	P_AddWeaponKick(ent, ent->client->v_forward * -2, { -1.f, 0.f, 0.f });

	fire_grenade(ent, start, dir, damage, 600, 2.5_sec, radius, (crandom_open() * 10.0f), (200 + crandom_open() * 10.0f), false);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_GRENADE | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);
	
	G_RemoveAmmo(ent);
}

/*
======================================================================

ROCKET

======================================================================
*/

void Weapon_RocketLauncher_Fire(edict_t *ent)
{
	int	  damage;
	float damage_radius;
	int	  radius_damage;

	damage = irandom(100, 120);
	radius_damage = 120;
	damage_radius = 120;
	if (is_quad)
	{
		damage *= damage_multiplier;
		radius_damage *= damage_multiplier;
	}

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->v_angle, { 8, 8, -8 }, start, dir);
	fire_rocket(ent, start, dir, damage, 650, damage_radius, radius_damage);

	P_AddWeaponKick(ent, ent->client->v_forward * -2, { -1.f, 0.f, 0.f });

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_ROCKET | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);
	
	G_RemoveAmmo(ent);
}

/*
======================================================================

BLASTER / HYPERBLASTER

======================================================================
*/

void Blaster_Fire(edict_t *ent, const vec3_t &g_offset, int damage, bool hyper, effects_t effect)
{
	if (is_quad)
		damage *= damage_multiplier;

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->v_angle, vec3_t{ 24, 8, -8 } + g_offset, start, dir);

	if (hyper)
		P_AddWeaponKick(ent, ent->client->v_forward * -2, { crandom() * 0.7f, crandom() * 0.7f, crandom() * 0.7f });
	else
		P_AddWeaponKick(ent, ent->client->v_forward * -2, { -1.f, 0.f, 0.f });

	// let the regular blaster projectiles travel a bit faster because it is a completely useless gun
	int speed = hyper ? 1000 : 1500;

	fire_blaster(ent, start, dir, damage, speed, effect, hyper ? MOD_HYPERBLASTER : MOD_BLASTER);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	if (hyper)
		gi.WriteByte(MZ_HYPERBLASTER | is_silenced);
	else
		gi.WriteByte(MZ_BLASTER | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);
}

void Weapon_Blaster_Fire(edict_t *ent)
{
	// give the blaster 15 across the board instead of just in dm
	int damage = 15;
	Blaster_Fire(ent, vec3_origin, damage, false, EF_BLASTER);
}

void Weapon_HyperBlaster_Fire(edict_t *ent)
{
	float	  rotation;
	vec3_t	  offset;
	int		  damage;

	// start on frame 6
	if (ent->client->ps.gunframe > 20)
		ent->client->ps.gunframe = 6;
	else
		ent->client->ps.gunframe++;

	// if we reached end of loop, have ammo & holding attack, reset loop
	// otherwise play wind down
	if (ent->client->ps.gunframe == 12)
	{
		if (ent->client->pers.inventory[ent->client->pers.weapon->ammo] && (ent->client->buttons & BUTTON_ATTACK))
			ent->client->ps.gunframe = 6;
		else
			gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/hyprbd1a.wav"), 1, ATTN_NORM, 0);
	}

	// play weapon sound for firing loop
	if (ent->client->ps.gunframe >= 6 && ent->client->ps.gunframe <= 11)
		ent->client->weapon_sound = gi.soundindex("weapons/hyprbl1a.wav");
	else
		ent->client->weapon_sound = 0;

	// fire frames
	bool request_firing = ent->client->weapon_fire_buffered || (ent->client->buttons & BUTTON_ATTACK);

	if (request_firing)
	{
		if (ent->client->ps.gunframe >= 6 && ent->client->ps.gunframe <= 11)
		{
			ent->client->weapon_fire_buffered = false;

			if (!ent->client->pers.inventory[ent->client->pers.weapon->ammo])
			{
				NoAmmoWeaponChange(ent, true);
				return;
			}

			rotation = (ent->client->ps.gunframe - 5) * 2 * PIf / 6;
			offset[0] = -4 * sinf(rotation);
			offset[2] = 0;
			offset[1] = 4 * cosf(rotation);

			if (deathmatch->integer)
				damage = 15;
			else
				damage = 20;
			Blaster_Fire(ent, offset, damage, true, (ent->client->ps.gunframe % 4) ? EF_NONE : EF_HYPERBLASTER);
			Weapon_PowerupSound(ent);

			G_RemoveAmmo(ent);

			ent->client->anim_priority = ANIM_ATTACK;
			if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent->s.frame = FRAME_crattak1 - (int) (frandom() + 0.25f);
				ent->client->anim_end = FRAME_crattak9;
			}
			else
			{
				ent->s.frame = FRAME_attack1 - (int) (frandom() + 0.25f);
				ent->client->anim_end = FRAME_attack8;
			}
			ent->client->anim_time = 0_ms;
		}
	}
}

void Weapon_HyperBlaster(edict_t *ent)
{
	constexpr int pause_frames[] = { 0 };

	Weapon_Repeating(ent, 5, 20, 49, 53, pause_frames, Weapon_HyperBlaster_Fire);
}

/*
======================================================================

MACHINEGUN / CHAINGUN

======================================================================
*/

void Machinegun_Fire(edict_t *ent)
{
	int i;
	int damage = 8;
	int kick = 2;

	if (!(ent->client->buttons & BUTTON_ATTACK))
	{
		ent->client->machinegun_shots = 0;
		ent->client->ps.gunframe = 6;
		return;
	}

	if (ent->client->ps.gunframe == 4)
		ent->client->ps.gunframe = 5;
	else
		ent->client->ps.gunframe = 4;

	if (ent->client->pers.inventory[ent->client->pers.weapon->ammo] < 1)
	{
		ent->client->ps.gunframe = 6;
		NoAmmoWeaponChange(ent, true);
		return;
	}

	if (is_quad)
	{
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	vec3_t kick_origin {}, kick_angles {};
	for (i = 0; i < 3; i++)
	{
		kick_origin[i] = crandom() * 0.35f;
		kick_angles[i] = crandom() * 0.7f;
	}
	//kick_angles[0] = ent->client->machinegun_shots * -1.5f;
	P_AddWeaponKick(ent, kick_origin, kick_angles);

	// raise the gun as it is firing
	// [Paril-KEX] disabled as this is a bit hard to do with high
	// tickrate, but it also just sucks in general.
	/*if (!deathmatch->integer)
	{
		ent->client->machinegun_shots++;
		if (ent->client->machinegun_shots > 9)
			ent->client->machinegun_shots = 9;
	}*/

	// get start / end positions
	vec3_t start, dir;
	// Paril: kill sideways angle on hitscan
	P_ProjectSource(ent, ent->client->v_angle, { 0, 0, -8 }, start, dir);
	G_LagCompensate(ent, start, dir);
	fire_bullet(ent, start, dir, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_MP5);
	G_UnLagCompensate();
	Weapon_PowerupSound(ent);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_MACHINEGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);
	
	G_RemoveAmmo(ent);

	ent->client->anim_priority = ANIM_ATTACK;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->s.frame = FRAME_crattak1 - (int) (frandom() + 0.25f);
		ent->client->anim_end = FRAME_crattak9;
	}
	else
	{
		ent->s.frame = FRAME_attack1 - (int) (frandom() + 0.25f);
		ent->client->anim_end = FRAME_attack8;
	}
	ent->client->anim_time = 0_ms;
}

void Weapon_Machinegun(edict_t *ent)
{
	constexpr int pause_frames[] = { 23, 45, 0 };

	Weapon_Repeating(ent, 3, 5, 45, 49, pause_frames, Machinegun_Fire);
}

void Chaingun_Fire(edict_t *ent)
{
	int	  i;
	int	  shots;
	float r, u;
	int	  damage;
	int	  kick = 2;

	if (deathmatch->integer)
		damage = 6;
	else
		damage = 8;

	if (ent->client->ps.gunframe > 31)
	{
		ent->client->ps.gunframe = 5;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1, ATTN_IDLE, 0);
	}
	else if ((ent->client->ps.gunframe == 14) && !(ent->client->buttons & BUTTON_ATTACK))
	{
		ent->client->ps.gunframe = 32;
		ent->client->weapon_sound = 0;
		return;
	}
	else if ((ent->client->ps.gunframe == 21) && (ent->client->buttons & BUTTON_ATTACK) && ent->client->pers.inventory[ent->client->pers.weapon->ammo])
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

	if (ent->client->ps.gunframe < 5 || ent->client->ps.gunframe > 21)
		return;

	ent->client->weapon_sound = gi.soundindex("weapons/chngnl1a.wav");

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
	ent->client->anim_time = 0_ms;

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

	if (ent->client->pers.inventory[ent->client->pers.weapon->ammo] < shots)
		shots = ent->client->pers.inventory[ent->client->pers.weapon->ammo];

	if (!shots)
	{
		NoAmmoWeaponChange(ent, true);
		return;
	}

	if (is_quad)
	{
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	vec3_t kick_origin {}, kick_angles {};
	for (i = 0; i < 3; i++)
	{
		kick_origin[i] = crandom() * 0.35f;
		kick_angles[i] = crandom() * (0.5f + (shots * 0.15f));
	}
	P_AddWeaponKick(ent, kick_origin, kick_angles);

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->v_angle, { 0, 0, -8 }, start, dir);

	G_LagCompensate(ent, start, dir);
	for (i = 0; i < shots; i++)
	{
		// get start / end positions
		// Paril: kill sideways angle on hitscan
		r = crandom() * 4;
		u = crandom() * 4;
		P_ProjectSource(ent, ent->client->v_angle, { 0, r, u + -8 }, start, dir);

		fire_bullet(ent, start, dir, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_M4);
	}
	G_UnLagCompensate();

	Weapon_PowerupSound(ent);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte((MZ_CHAINGUN1 + shots - 1) | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);
	
	G_RemoveAmmo(ent, shots);
}

void Weapon_Chaingun(edict_t *ent)
{
	constexpr int pause_frames[] = { 38, 43, 51, 61, 0 };

	Weapon_Repeating(ent, 4, 31, 61, 64, pause_frames, Chaingun_Fire);
}

/*
======================================================================

SHOTGUN / SUPERSHOTGUN

======================================================================
*/

void weapon_shotgun_fire(edict_t *ent)
{
	int damage = 4;
	int kick = 8;

	vec3_t start, dir;
	// Paril: kill sideways angle on hitscan
	P_ProjectSource(ent, ent->client->v_angle, { 0, 0, -8 }, start, dir);

	P_AddWeaponKick(ent, ent->client->v_forward * -2, { -2.f, 0.f, 0.f });

	if (is_quad)
	{
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	G_LagCompensate(ent, start, dir);
	if (deathmatch->integer)
		fire_shotgun(ent, start, dir, damage, kick, 500, 500, DEFAULT_DEATHMATCH_SHOTGUN_COUNT, MOD_M3);
	else
		fire_shotgun(ent, start, dir, damage, kick, 500, 500, DEFAULT_SHOTGUN_COUNT, MOD_M3);
	G_UnLagCompensate();

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_SHOTGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);
	
	G_RemoveAmmo(ent);
}


void weapon_supershotgun_fire(edict_t *ent)
{
	int damage = 6;
	int kick = 12;

	if (is_quad)
	{
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}
	
	vec3_t start, dir;
	// Paril: kill sideways angle on hitscan
	P_ProjectSource(ent, ent->client->v_angle, { 0, 0, -8 }, start, dir);
	G_LagCompensate(ent, start, dir);
	vec3_t v;
	v[PITCH] = ent->client->v_angle[PITCH];
	v[YAW] = ent->client->v_angle[YAW] - 5;
	v[ROLL] = ent->client->v_angle[ROLL];
	// Paril: kill sideways angle on hitscan
	P_ProjectSource(ent, v, { 0, 0, -8 }, start, dir);
	fire_shotgun(ent, start, dir, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT / 2, MOD_HC);
	v[YAW] = ent->client->v_angle[YAW] + 5;
	P_ProjectSource(ent, v, { 0, 0, -8 }, start, dir);
	fire_shotgun(ent, start, dir, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT / 2, MOD_HC);
	G_UnLagCompensate();

	P_AddWeaponKick(ent, ent->client->v_forward * -2, { -2.f, 0.f, 0.f });

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_SSHOTGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);
	
	G_RemoveAmmo(ent);
}

/*
======================================================================

RAILGUN

======================================================================
*/

void weapon_railgun_fire(edict_t *ent)
{
	int damage = 100;
	int kick = 200;

	if (is_quad)
	{
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->v_angle, { 0, 7, -8 }, start, dir);
	G_LagCompensate(ent, start, dir);
	fire_rail(ent, start, dir, damage, kick);
	G_UnLagCompensate();

	P_AddWeaponKick(ent, ent->client->v_forward * -3, { -3.f, 0.f, 0.f });

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_RAILGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);
	
	G_RemoveAmmo(ent);
}

/*
======================================================================

BFG10K

======================================================================
*/

void weapon_bfg_fire(edict_t *ent)
{
	int	  damage;
	float damage_radius = 1000;

	if (deathmatch->integer)
		damage = 200;
	else
		damage = 500;

	if (ent->client->ps.gunframe == 9)
	{
		// send muzzle flash
		gi.WriteByte(svc_muzzleflash);
		gi.WriteEntity(ent);
		gi.WriteByte(MZ_BFG | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS, false);

		PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);
		return;
	}

	// cells can go down during windup (from power armor hits), so
	// check again and abort firing if we don't have enough now
	if (ent->client->pers.inventory[ent->client->pers.weapon->ammo] < 50)
		return;

	if (is_quad)
		damage *= damage_multiplier;

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->v_angle, { 8, 8, -8 }, start, dir);
	fire_bfg(ent, start, dir, damage, 400, damage_radius);

	P_AddWeaponKick(ent, ent->client->v_forward * -2, { -20.f, 0, crandom() * 8 });
	ent->client->kick.total = DAMAGE_TIME();
	ent->client->kick.time = level.time + ent->client->kick.total;

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_BFG2 | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);
	
	G_RemoveAmmo(ent);
}

//======================================================================

void weapon_disint_fire(edict_t *self)
{
	vec3_t start, dir;
	P_ProjectSource(self, self->client->v_angle, { 24, 8, -8 }, start, dir);

	P_AddWeaponKick(self, self->client->v_forward * -2, { -1.f, 0.f, 0.f });

	fire_disintegrator(self, start, dir, 800);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(self);
	gi.WriteByte(MZ_BLASTER2);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);

	PlayerNoise(self, start, PNOISE_WEAPON);

	G_RemoveAmmo(self);
}

//======================================================================
// Action Add
//======================================================================

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

	if (INV_AMMO(ent, POWERUP_LASERSIGHT) && (ent->client->pers.weapon->id == IT_WEAPON_MK23
		|| ent->client->pers.weapon->id == IT_WEAPON_MP5 || ent->client->pers.weapon->id == IT_WEAPON_M4))
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

// used for when we want to force a player to drop an extra special weapon
// for when they drop the bandolier and they are over the weapon limit
void DropExtraSpecial(edict_t* ent)
{
	int itemNum;

	itemNum = ent->client->pers.weapon->id ? ent->client->pers.weapon->id : 0;
	if (itemNum >= IT_WEAPON_MP5 && itemNum <= IT_WEAPON_SNIPER)
	{
		// if they have more than 1 then they are willing to drop one           
		if (INV_AMMO(ent, itemNum) > 1) {
			Drop_Weapon(ent, ent->client->pers.weapon);
			return;
		}
	}
	// otherwise drop some weapon they aren't using
	if (INV_AMMO(ent, IT_AMMO_SLUGS) > 0 && IT_WEAPON_SNIPER != itemNum)
		Drop_Weapon(ent, GetItemByIndex(IT_WEAPON_SNIPER));
	else if (INV_AMMO(ent, IT_AMMO_SHELLS) > 0 && IT_WEAPON_HANDCANNON != itemNum)
		Drop_Weapon(ent, GetItemByIndex(IT_WEAPON_HANDCANNON));
	else if (INV_AMMO(ent, IT_AMMO_SHELLS) > 0 && IT_WEAPON_M3 != itemNum)
		Drop_Weapon(ent, GetItemByIndex(IT_WEAPON_M3));
	else if (INV_AMMO(ent, IT_AMMO_ROCKETS) > 0 && IT_WEAPON_MP5 != itemNum)
		Drop_Weapon(ent, GetItemByIndex(IT_WEAPON_MP5));
	else if (INV_AMMO(ent, IT_AMMO_CELLS) > 0 && IT_WEAPON_M4 != itemNum)
		Drop_Weapon(ent, GetItemByIndex(IT_WEAPON_M4));
	else
		gi.Com_Print("Couldn't find the appropriate weapon to drop.\n");
}

//zucc ready special weapon
void ReadySpecialWeapon(edict_t* ent)
{
	//int weapons[5] = weap_ids;
	int curr, i;
	int last;


	if (ent->client->weaponstate == WEAPON_BANDAGING || ent->client->bandaging)
		return;


	for (curr = 0; curr < 5; curr++)
	{
		if (ent->client->pers.weapon->id == weap_ids[curr])
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
		if (INV_AMMO(ent, weap_ids[i % 5]))
		{
			ent->client->newweapon = GET_ITEM(weap_ids[i % 5]);
			return;
		}
	}
}


void MuzzleFlash(edict_t* ent, int mz)
{
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(mz);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, ent->s.origin, PNOISE_WEAPON);
}


void PlayWeaponSound(edict_t* ent)
{
	if (!ent->client->weapon_sound)
		return;

	// Because MZ_BLASTER is 0, use this stupid workaround.
	if ((ent->client->weapon_sound & ~MZ_SILENCED) == MZ_BLASTER2)
		ent->client->weapon_sound &= ~MZ_BLASTER2;


	if (ent->client->weapon_sound & MZ_SILENCED)
		// Silencer suppresses both sound and muzzle flash.
		gi.sound(ent, CHAN_WEAPON, level.snd_silencer, 1, ATTN_NORM, 0);

	else switch (ent->client->weapon_sound)
	{
	case MZ_BLASTER:
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mk23fire.wav"), 1, ATTN_NORM, 0);
		MuzzleFlash(ent, MZ_MACHINEGUN);
		break;
	case MZ_MACHINEGUN:
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/mp5fire.wav"), 1, ATTN_NORM, 0);
		MuzzleFlash(ent, MZ_MACHINEGUN);
		break;
	case MZ_ROCKET:
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/m4a1fire.wav"), 1, ATTN_NORM, 0);
		MuzzleFlash(ent, MZ_MACHINEGUN);
		break;
	case MZ_SHOTGUN:
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/shotgf1b.wav"), 1, ATTN_NORM, 0);
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
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/ssgfire.wav"), 1, ATTN_NORM, 0);
		MuzzleFlash(ent, MZ_MACHINEGUN);
		break;
	default:
		MuzzleFlash(ent, ent->client->weapon_sound);
	}

	ent->client->weapon_sound = 0;
	//level.weapon_sound_framenum = level.framenum;
}


//======================================================================
// mk23 derived from tutorial by GreyBear

//void Pistol_Fire(edict_t* ent, player_state_t *ps)
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
		// if (level.framenum >= ent->pain_debounce_framenum)
		// {
		// 	gi.sound(ent, CHAN_VOICE, level.snd_noammo, 1, ATTN_NORM, 0);
		// 	ent->pain_debounce_framenum = level.time + 10_ms;
		// }
		gi.sound(ent, CHAN_VOICE, level.snd_noammo, 1, ATTN_NORM, 0);
		return;
	}
	
	//Calculate the kick angles
	vec3_t kick_origin {}, kick_angles {};
		for (i = 0; i < 3; i++)
		{
			kick_origin[i] = crandom() * 0.35;
			kick_angles[i] = crandom() * 0.7;
		}
		P_AddWeaponKick(ent, kick_origin, kick_angles);
	kick_origin[0] = crandom() * 0.35;
	kick_angles[0] = ent->client->machinegun_shots * -1.5;

	// get start / end positions
	VectorAdd(ent->client->v_angle, kick_angles, angles);
	AngleVectors(angles, forward, right, NULL);
	VectorSet(offset, 0, 8, ent->viewheight - height);
	P_ProjectSource(ent, ent->s.origin, offset, forward, start);

	spread = AdjustSpread(ent, spread);

	if (ent->client->pers.mk23_mode)
		spread *= .7;

	//      gi.LocClient_Print(ent, PRINT_HIGH, "Spread is %d\n", spread);

	if (ent->client->mk23_rds == 1)
	{
		//Hard coded for reload only.
		ent->client->ps.gunframe = 62;
		ent->client->weaponstate = WEAPON_END_MAG;
	}

	fire_bullet(ent, start, forward, damage, kick, spread, spread, MOD_MK23);

	//Stats_AddShot(ent, MOD_MK23);

	ent->client->mk23_rds--;
	ent->client->dual_rds--;


	ent->client->weapon_sound = MZ_BLASTER2;  // Becomes MZ_BLASTER.
	if (INV_AMMO(ent, IT_ITEM_QUIET))
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
	vec3_t start, dir;
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
		// if (level.framenum >= ent->pain_debounce_framenum)
		// {
		// 	gi.sound(ent, CHAN_VOICE, level.snd_noammo, 1, ATTN_NORM, 0);
		// 	ent->pain_debounce_framenum = level.framenum + 1 * HZ;
		// }
		gi.sound(ent, CHAN_VOICE, level.snd_noammo, 1, ATTN_NORM, 0);
		return;
	}


	spread = AdjustSpread(ent, spread);
	if (ent->client->burst)
		spread *= .7;


	//Calculate the kick angles
	vec3_t kick_origin {}, kick_angles {};
		for (i = 0; i < 3; i++)
		{
			kick_origin[i] = crandom() * 0.25;
			kick_angles[i] = crandom() * 0.5;
		}

	P_ProjectSource(ent, ent->client->v_angle, { 0, 7, -8 }, start, dir);
	P_AddWeaponKick(ent, kick_origin, kick_angles);

	kick_origin[0] = crandom() * 0.35;
	kick_angles[0] = ent->client->machinegun_shots * -1.5;

	// get start / end positions
	// VectorAdd(ent->client->v_angle, kick_angles, angles);
	// AngleVectors(angles, forward, right, NULL);
	// VectorSet(offset, 0, 8, ent->viewheight - height);
	// P_ProjectSource(ent, ent->s.origin, offset, forward, right, start);
	G_LagCompensate(ent, start, dir);
	fire_bullet(ent, start, forward, damage, kick, spread, spread, MOD_MP5);
	G_UnLagCompensate();

	//Stats_AddShot(ent, MOD_MP5);

	ent->client->mp5_rds--;

	// zucc vwep
	// if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	// 	SetAnimation( ent, FRAME_crattak1 - (int)(random() + 0.25), FRAME_crattak9, ANIM_ATTACK );
	// else
	// 	SetAnimation( ent, FRAME_attack1 - (int)(random() + 0.25), FRAME_attack8, ANIM_ATTACK );
	// zucc vwep done

	if (ent->client->anim_priority != ANIM_ATTACK || frandom() < 0.25f)
	{
		ent->client->anim_priority = ANIM_ATTACK;
		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		{
			ent->s.frame = FRAME_crattak1 - (int)(frandom() * 0.25f);
			ent->client->anim_end = FRAME_crattak9;
		}
		else
		{
			ent->s.frame = FRAME_attack1 - (int)(frandom() * 0.25f);
			ent->client->anim_end = FRAME_attack8;
		}
		ent->client->anim_time = 0_ms;
	}

	ent->client->weapon_sound = MZ_MACHINEGUN;
	if (INV_AMMO(ent, IT_ITEM_QUIET))
		ent->client->weapon_sound |= MZ_SILENCED;
	PlayWeaponSound(ent);
}

void Weapon_MP5(edict_t* ent)
{
	//Idle animation entry points - These make the fidgeting look more random
	constexpr int pause_frames[] = { 13, 30, 47 };

	//The frames at which the weapon will fire
	constexpr int fire_frames[] = { 11, 12, 71, 72, 73, 0 };

	//The call is made...
	Weapon_Generic(ent, 10, 12, 47, 51, 69, 77, pause_frames, fire_frames, MP5_Fire);
	//Weapon_Repeating(ent, 10, 12, 47, 51, pause_frames, fire_frames, MP5_Fire);
}

// zucc fire_load_ap for rounds that pass through soft targets and keep going
static void fire_lead_ap(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_impact, int hspread, int vspread, mod_id_t mod)
{
	trace_t tr;
	vec3_t dir, forward, right, up, end;
	float r, u;
	vec3_t water_start;
	bool water = false;
	contents_t content_mask = MASK_SHOT | MASK_WATER;
	vec3_t from;
	edict_t *ignore;


	InitTookDamage();
	// setup
	stopAP = 0;
	dir = vectoangles(aimdir);
	AngleVectors(dir, forward, right, up);

	r = crandom() * hspread;
	u = crandom() * vspread;
	VectorMA(start, 8192, forward, end);
	VectorMA(end, r, right, end);
	VectorMA(end, u, up, end);
	VectorCopy(start, from);
	if (gi.pointcontents(start) & MASK_WATER)
	{
		water = true;
		VectorCopy (start, water_start);
		content_mask &= ~MASK_WATER;
	}

	ignore = self;

	while (ignore)
	{
		PRETRACE();
		//tr = gi.trace (from, NULL, NULL, end, ignore, mask);
		tr = gi.trace(from, self->mins, self->maxs, end, ignore, content_mask);
		POSTTRACE();

		// glass fx
		// catch case of firing thru one or breakable glasses
		while (tr.fraction < 1.0 && (tr.surface->flags & (SURF_TRANS33|SURF_TRANS66))
			&& (tr.ent) && (0 == strcmp(tr.ent->classname, "func_explosive")))
		{
			// break glass
			// Will review this later  
			//CGF_SFX_ShootBreakableGlass(tr.ent, self, &tr, mod);
			// continue trace from current endpos to start
			PRETRACE();
			tr = gi.trace(tr.endpos, self->mins, self->maxs, end, tr.ent, content_mask);
			POSTTRACE();
		}
		// ---

		// see if we hit water
		if (tr.contents & MASK_WATER)
		{
			int color;

			water = true;
			VectorCopy(tr.endpos, water_start);

			if (!VectorCompare(from, tr.endpos))
			{
				if (tr.contents & CONTENTS_WATER)
				{
					if (strcmp(tr.surface->name, "*brwater") == 0)
						color = SPLASH_BROWN_WATER;
					else
						color = SPLASH_BLUE_WATER;
				}
				else if (tr.contents & CONTENTS_SLIME)
					color = SPLASH_SLIME;
				else if (tr.contents & CONTENTS_LAVA)
					color = SPLASH_LAVA;
				else
					color = SPLASH_UNKNOWN;

				if (color != SPLASH_UNKNOWN)
				{
					gi.WriteByte(svc_temp_entity);
					gi.WriteByte(TE_SPLASH);
					gi.WriteByte(8);
					gi.WritePosition(tr.endpos);
					gi.WriteDir(tr.plane.normal);
					gi.WriteByte(color);
					gi.multicast(tr.endpos, MULTICAST_PVS, false);
				}

				// change bullet's course when it enters water
				VectorSubtract(end, from, dir);
				dir = vectoangles(dir);
				AngleVectors(dir, forward, right, up);
				r = crandom() * hspread * 2;
				u = crandom() * vspread * 2;
				VectorMA(water_start, 8192, forward, end);
				VectorMA(end, r, right, end);
				VectorMA(end, u, up, end);
			}

			// re-trace ignoring water this time
			PRETRACE();
			tr = gi.trace(water_start, self->mins, self->maxs, end, ignore, MASK_SHOT);
			POSTTRACE();
		}

		// send gun puff / flash

		ignore = NULL;

		if (tr.surface && (tr.surface->flags & SURF_SKY))
			continue;

		if (tr.fraction < 1.0)
		{

			if ((tr.ent->svflags & SVF_MONSTER) || tr.ent->client)
			{
				ignore = tr.ent;
				VectorCopy(tr.endpos, from);
				//FIREBLADE
				// Advance the "from" point a few units
				// towards "end" here
				if (tr.ent->client)
				{
					if (tr.ent->client->took_damage)
					{
						vec3_t out;
						VectorSubtract(end, from, out);
						VectorNormalize(out);
						VectorScale(out, 8, out);
						VectorAdd(out, from, from);
						continue;
					}
					tr.ent->client->took_damage++;
				}
				//FIREBLADE
			}

			if (tr.ent != self && tr.ent->takedamage)
			{
				T_Damage(tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, kick, DAMAGE_NONE, mod);
				if (stopAP)	// the AP round hit something that would stop it (kevlar)
					ignore = NULL;
			}
			else if (tr.ent != self && !water)
			{
				if (strncmp(tr.surface->name, "sky", 3) != 0)
				{
					//AddDecal(self, &tr);
					gi.WriteByte(svc_temp_entity);
					gi.WriteByte(te_impact);
					gi.WritePosition (tr.endpos);
					gi.WriteDir(tr.plane.normal);
					gi.multicast(tr.endpos, MULTICAST_PVS, false);

					if (self->client)
						PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
				}
			}
		}
	}

	// if went through water, determine where the end and make a bubble trail
	if (water)
	{
		vec3_t pos;

		VectorSubtract(tr.endpos, water_start, dir);
		VectorNormalize(dir);
		VectorMA(tr.endpos, -2, dir, pos);
		if (gi.pointcontents(pos) & MASK_WATER) {
			VectorCopy(pos, tr.endpos);
		} else {
			PRETRACE();
			tr = gi.traceline(pos, water_start, tr.ent, MASK_WATER);
			//tr = gi.trace(pos, NULL, NULL, water_start, tr.ent, MASK_WATER);
			POSTTRACE();
		}

		VectorAdd(water_start, tr.endpos, pos);
		VectorScale(pos, 0.5, pos);

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_BUBBLETRAIL);
		gi.WritePosition(water_start);
		gi.WritePosition(tr.endpos);
		gi.multicast(pos, MULTICAST_PVS, false);
	}

}

// zucc - for the M4
void fire_bullet_sparks (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, mod_id_t mod)
{
	setFFState(self);
	fire_lead_ap(self, start, aimdir, damage, kick, TE_BULLET_SPARKS, hspread, vspread, mod);
}

// zucc - for sniper
void fire_bullet_sniper (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, mod_id_t mod)
{
	setFFState (self);
	fire_lead_ap (self, start, aimdir, damage, kick, TE_GUNSHOT, hspread, vspread, mod);
}

void M4_Fire(edict_t* ent)
{
	int i;
	vec3_t start, dir;
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
		// if (level.framenum >= ent->pain_debounce_framenum)
		// {
		// 	gi.sound(ent, CHAN_VOICE, level.snd_noammo, 1, ATTN_NORM, 0);
		// 	ent->pain_debounce_framenum = level.framenum + 1 * HZ;
		// }
		gi.sound(ent, CHAN_VOICE, level.snd_noammo, 1, ATTN_NORM, 0);
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


	//      gi.LocClient_Print(ent, PRINT_HIGH, "Spread is %d\n", spread);


	//Calculate the kick angles
	vec3_t kick_origin {}, kick_angles {};
		for (i = 0; i < 3; i++)
		{
			kick_origin[i] = crandom() * 0.25;
			kick_angles[i] = crandom() * 0.5;
		}
		P_AddWeaponKick(ent, kick_origin, kick_angles);
	kick_origin[0] = crandom() * 0.35;
	kick_angles[0] = ent->client->machinegun_shots * -.7;
	VectorAdd(ent->client->v_angle, kick_angles, angles);
	AngleVectors(angles, forward, right, NULL);
	VectorSet(offset, 0, 8, ent->viewheight - height);
	
	P_ProjectSource(ent, ent->client->v_angle, { 0, 7, -8 }, start, dir);
	P_AddWeaponKick(ent, kick_origin, kick_angles);
	// get start / end positions
	// VectorAdd(ent->client->v_angle, kick_angles, angles);
	// AngleVectors(angles, forward, right, NULL);
	// VectorSet(offset, 0, 8, ent->viewheight - height);
	// P_ProjectSource(ent, ent->s.origin, offset, forward, right, start);

	if (is_quad)
		damage *= 1.5f;

	G_LagCompensate(ent, start, dir);
	fire_bullet_sparks(ent, start, forward, damage, kick, spread, spread, MOD_M4);
	G_UnLagCompensate();
	//Stats_AddShot(ent, MOD_M4);

	ent->client->m4_rds--;


	// zucc vwep
	if (ent->client->anim_priority != ANIM_ATTACK || frandom() < 0.25f)
	{
		ent->client->anim_priority = ANIM_ATTACK;
		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		{
			ent->s.frame = FRAME_crattak1 - (int)(frandom() * 0.25f);
			ent->client->anim_end = FRAME_crattak9;
		}
		else
		{
			ent->s.frame = FRAME_attack1 - (int)(frandom() * 0.25f);
			ent->client->anim_end = FRAME_attack8;
		}
		ent->client->anim_time = 0_ms;
	}
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
	vec3_t start, dir;
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
	
	vec3_t kick_angles {};
	VectorScale(forward, -2, ent->client->kick_origin);
	kick_angles[0] = -2;

	VectorSet(offset, 0, 8, ent->viewheight - height);



	if (ent->client->ps.gunframe == 14)
	{
		ent->client->ps.gunframe++;
		return;
	}

	P_ProjectSource(ent, ent->client->v_angle, { 0, 7, -8 }, start, dir);

	if (is_quad)
	{
		damage *= 1.5f;
		kick *= 1.5f;
	}

	setFFState(ent);
	InitTookDamage();	//FB 6/3/99

	G_LagCompensate(ent, start, dir);
	fire_shotgun(ent, start, forward, damage, kick, 800, 800,
		12 /*DEFAULT_DEATHMATCH_SHOTGUN_COUNT */, MOD_M3);
	G_UnLagCompensate();

	//Stats_AddShot(ent, MOD_M3);

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
	vec3_t start, dir;
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

	vec3_t kick_angles {};
	kick_angles[0] = -2;

	VectorSet(offset, 0, 8, ent->viewheight - height);
	//P_ProjectSource(ent, ent->s.origin, offset, forward, right, start);

	P_ProjectSource(ent, ent->client->v_angle, { 0, 7, -8 }, start, dir);

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
		G_LagCompensate(ent, start, dir);
		fire_shotgun(ent, start, forward, sngl_damage, sngl_kick, DEFAULT_SHOTGUN_HSPREAD * 2.5, DEFAULT_SHOTGUN_VSPREAD * 2.5, 34 / 2, MOD_HC);
		G_UnLagCompensate();

		ent->client->cannon_rds--;
	}
	else
	{
		// Both barrels.

		v[YAW] = ent->client->v_angle[YAW] - 5;
		AngleVectors(v, forward, NULL, NULL);
		G_LagCompensate(ent, start, dir);
		fire_shotgun(ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD * 4, DEFAULT_SHOTGUN_VSPREAD * 4, 34 / 2, MOD_HC);
		G_UnLagCompensate();

		v[YAW] = ent->client->v_angle[YAW] + 5;
		AngleVectors(v, forward, NULL, NULL);
		G_LagCompensate(ent, start, dir);
		fire_shotgun(ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD * 4, DEFAULT_SHOTGUN_VSPREAD * 4 /* was *5 here */, 34 / 2, MOD_HC);
		G_UnLagCompensate();

		ent->client->cannon_rds -= 2;
	}

	//Stats_AddShot(ent, MOD_HC);
	ProduceShotgunDamageReport(ent);	//FB 6/3/99

	ent->client->ps.gunframe++;


	ent->client->weapon_sound = MZ_SSHOTGUN;
	PlayWeaponSound(ent);


}

void Weapon_HC(edict_t* ent)
{
	static int pause_frames[] = { 29, 42, 57, 0 };
	static int fire_frames[] = { 7, 0 };

	Weapon_Generic(ent, 6, 17, 57, 61, 82, 83, pause_frames, fire_frames, HC_Fire);
}

void Sniper_Fire(edict_t* ent)
{
	//int i;  // FIXME: Should this be used somewhere?
	vec3_t start, dir;
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

		P_ProjectSource(ent, ent->client->v_angle, forward, start, dir);
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
		// if (level.framenum >= ent->pain_debounce_framenum)
		// {
			
		// 	ent->pain_debounce_framenum = level.framenum + 1 * HZ;
		// }
		gi.sound(ent, CHAN_VOICE, level.snd_noammo, 1, ATTN_NORM, 0);
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

	P_ProjectSource(ent, ent->client->v_angle, forward, start, dir);
	G_LagCompensate(ent, start, dir);
	//If no reload, fire normally.
	fire_bullet_sniper(ent, start, forward, damage, kick, spread, spread, MOD_SNIPER);
	//Stats_AddShot(ent, MOD_SNIPER);
	G_UnLagCompensate();

	ent->client->sniper_rds--;
	ent->client->ps.fov = 90;	// so we can watch the next round get chambered
	ent->client->ps.gunindex =
		gi.modelindex(ent->client->pers.weapon->view_model);
	ent->client->no_sniper_display = 1;


	ent->client->weapon_sound = MZ_HYPERBLASTER;
	if (INV_AMMO(ent, IT_ITEM_QUIET))
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
	vec3_t start, dir;
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
		// if (level.framenum >= ent->pain_debounce_framenum)
		// {
			
		// 	ent->pain_debounce_framenum = level.framenum + 1 * HZ;
		// }
		gi.sound(ent, CHAN_VOICE, level.snd_noammo, 1, ATTN_NORM, 0);
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
		//P_ProjectSource(ent, ent->s.origin, offset, forward, right, start);
		P_ProjectSource(ent, ent->client->v_angle, forward, start, dir);
		
		if (ent->client->dual_rds > 1)
		{
			G_LagCompensate(ent, start, dir);
			fire_bullet(ent, start, forward, damage, kick, spread, spread, MOD_DUAL);
			G_UnLagCompensate();
			//Stats_AddShot(ent, MOD_DUAL);

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
		// if (level.framenum >= ent->pain_debounce_framenum)
		// {
		// 	gi.sound(ent, CHAN_VOICE, level.snd_noammo, 1, ATTN_NORM, 0);
		// 	ent->pain_debounce_framenum = level.framenum + 1 * HZ;
		// }
		gi.sound(ent, CHAN_VOICE, level.snd_noammo, 1, ATTN_NORM, 0);
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

	P_ProjectSource(ent, ent->client->v_angle, { 0, 7, -8 }, start, dir);
	G_LagCompensate(ent, start, dir);
	//If no reload, fire normally.
	fire_bullet(ent, start, forward, damage, kick, spread, spread, MOD_DUAL);
	//Stats_AddShot(ent, MOD_DUAL);
	G_UnLagCompensate();


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
#define FRAME_IDLE2_FIRST               (FRAME_PREPARETHROW_LAST +1)
#define FRAME_THROW_FIRST               (FRAME_IDLE2_LAST +1)
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
	if (ent->client->weaponstate == WEAPON_FIRING && !IS_ALIVE(ent) || lights_camera_action)
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
				// {
				// 	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
				// 		SetAnimation( ent, FRAME_crpain4 + 1, FRAME_crpain1, ANIM_REVERSED );
				// 	else
				// 		SetAnimation( ent, FRAME_pain304 + 1, FRAME_pain301, ANIM_REVERSED );
				// }
				{
				ent->client->anim_priority = ANIM_ATTACK;
				if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
				{
					ent->s.frame = FRAME_crpain4 + 1;
					ent->client->anim_end = FRAME_crpain1;
				}
				else
				{
					ent->s.frame = FRAME_pain304 + 1;
					ent->client->anim_end = FRAME_pain301;
				}
				ent->client->anim_time = 0_ms;
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
			// {
			// 	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			// 		SetAnimation( ent, FRAME_crpain4 + 1, FRAME_crpain1, ANIM_REVERSE );
			// 	else
			// 		SetAnimation( ent, FRAME_pain304 + 1, FRAME_pain301, ANIM_REVERSE );
			// }
			{
				ent->client->anim_priority = ANIM_ATTACK;
				if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
				{
					ent->s.frame = FRAME_crpain4 + 1;
					ent->client->anim_end = FRAME_crpain1;
				}
				else
				{
					ent->s.frame = FRAME_pain304 + 1;
					ent->client->anim_end = FRAME_pain301;
				}
				ent->client->anim_time = 0_ms;
			}
			ent->client->ps.gunframe++;
		}
		return;
	}
	if (ent->client->weaponstate == WEAPON_ACTIVATING)
	{

		if (ent->client->pers.knife_mode == 1 && ent->client->ps.gunframe == 0)
		{
			//                      gi.LocClient_Print(ent, PRINT_HIGH, "NewKnifeFirst\n");
			ent->client->ps.gunframe = FRAME_PREPARETHROW_FIRST;
			return;
		}
		if (ent->client->ps.gunframe == FRAME_ACTIVATE_LAST
			|| ent->client->ps.gunframe == FRAME_STOPTHROW_LAST)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = FRAME_FIRE_LAST + 1;
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
		//              gi.LocClient_Print(ent, PRINT_HIGH, "After increment frames = %d\n", ent->client->ps.gunframe);
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
		ent->client->ps.gunframe = FRAME_IDLE_LAST + 1;
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
			// {
			// 	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			// 		SetAnimation( ent, FRAME_crpain4 + 1, FRAME_crpain1, ANIM_REVERSE );
			// 	else
			// 		SetAnimation( ent, FRAME_pain304 + 1, FRAME_pain301, ANIM_REVERSE );
			// }
			{
				ent->client->anim_priority = ANIM_ATTACK;
				if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
				{
					ent->s.frame = FRAME_crpain4 + 1;
					ent->client->anim_end = FRAME_crpain1;
				}
				else
				{
					ent->s.frame = FRAME_pain304 + 1;
					ent->client->anim_end = FRAME_pain301;
				}
				ent->client->anim_time = 0_ms;
			}
		}
		else			// not in throwing mode
		{
			ent->client->ps.gunframe = FRAME_IDLE_LAST + 1;
			// zucc more vwep stuff
			if ((FRAME_DEACTIVATE_LAST - FRAME_IDLE_LAST + 1) < 4)
			// {
			// 	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			// 		SetAnimation( ent, FRAME_crpain4 + 1, FRAME_crpain1, ANIM_REVERSE );
			// 	else
			// 		SetAnimation( ent, FRAME_pain304 + 1, FRAME_pain301, ANIM_REVERSE );
			// }
			{
				ent->client->anim_priority = ANIM_ATTACK;
				if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
				{
					ent->s.frame = FRAME_crpain4 + 1;
					ent->client->anim_end = FRAME_crpain1;
				}
				else
				{
					ent->s.frame = FRAME_pain304 + 1;
					ent->client->anim_end = FRAME_pain301;
				}
				ent->client->anim_time = 0_ms;
			}
		}
		ent->client->weaponstate = WEAPON_DROPPING;
		return;
	}

	if (ent->client->weaponstate == WEAPON_READY)
	{
		if (((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK)
			&& (!IS_ALIVE(ent) && !lights_camera_action && !ent->client->uvTime))
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;


			if (ent->client->pers.knife_mode == 1)
			{
				ent->client->ps.gunframe = FRAME_THROW_FIRST;
			}
			else
			{
				ent->client->ps.gunframe = FRAME_ACTIVATE_LAST + 1;
			}
			ent->client->weaponstate = WEAPON_FIRING;

			// start the animation
			// if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			// 	SetAnimation( ent, FRAME_crattak1 - 1, FRAME_attack8, ANIM_ATTACK );
			// else
			// 	SetAnimation( ent, FRAME_attack1 - 1, FRAME_attack8, ANIM_ATTACK );

			ent->client->anim_priority = ANIM_ATTACK;
			if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent->s.frame = FRAME_crattak1 - 1;
				ent->client->anim_end = FRAME_attack8;
			}
			else
			{
				ent->s.frame = FRAME_attack1 - 1;
				ent->client->anim_end = FRAME_attack8;
			}
			ent->client->anim_time = 0_ms;
			
			return;
		}
		if (ent->client->ps.gunframe == FRAME_IDLE_LAST)
		{
			ent->client->ps.gunframe = FRAME_FIRE_LAST + 1;
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
			//gi.LocClient_Print(ent, PRINT_HIGH, "Before a gunframe additon frames = %d\n", ent->client->ps.gunframe);
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
				// if (ent->client->quad_framenum > level.framenum)
				// 	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"),
				// 		1, ATTN_NORM, 0);

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

		if (ent->client->ps.gunframe == FRAME_FIRE_LAST + 2 ||
			ent->client->ps.gunframe == FRAME_IDLE2_FIRST + 1)
			ent->client->weaponstate = WEAPON_READY;
	}

}

// this one is based on the real old project source
static void Knife_ProjectSource(gclient_t* client, vec3_t point, vec3_t distance,
	vec3_t forward, vec3_t right, vec3_t result)
{
	vec3_t _distance;

	VectorCopy(distance, _distance);
	if (client->pers.hand == LEFT_HANDED)
		_distance[1] *= -1;		// changed from = to *=                                         
	else if (client->pers.hand == CENTER_HANDED)
		_distance[1] = 0;
	G_ProjectSource(point, _distance, forward, right);
	
}

int Knife_Fire(edict_t* ent)
{
	vec3_t start, v, dir;
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
	//P_ProjectSource(ent, ent->s.origin, offset, forward, right, start);
	P_ProjectSource(ent, ent->client->v_angle, { 0, 7, -8 }, start, dir);


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
		//Stats_AddShot(ent, MOD_KNIFE);

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

		INV_AMMO(ent, IT_WEAPON_KNIFE)--;
		if (INV_AMMO(ent, IT_WEAPON_KNIFE) <= 0)
		{
			ent->client->newweapon = GetItemByIndex(IT_WEAPON_MK23);
			ChangeWeapon(ent);
			// zucc was at 1250, dropping speed to 1200
			knife_throw(ent, start, forward, damage, 1200);
			//Stats_AddShot(ent, MOD_KNIFE_THROWN);
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
		//Stats_AddShot(ent, MOD_KNIFE_THROWN);

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
	vec3_t start, dir;
	int damage = GRENADE_DAMRAD;
	int speed;
	/*        int held = false;*/

	damage = GRENADE_DAMRAD;

	if (is_quad)
		damage *= 1.5f;

	VectorSet(offset, 8, 8, ent->viewheight - 8);
	AngleVectors(ent->client->v_angle, forward, right, NULL);

	P_ProjectSource(ent, ent->client->v_angle, { 0, 7, -8 }, start, dir);
	//P_ProjectSource(ent, ent->s.origin, offset, forward, right, start);

	if (ent->client->pers.grenade_mode == 0)
		speed = 400;
	else if (ent->client->pers.grenade_mode == 1)
		speed = 720;
	else
		speed = 920;

	G_LagCompensate(ent, start, dir);
	fire_grenade2(ent, start, forward, damage, speed, 80_ms, damage * 2, false);
	G_UnLagCompensate();

	INV_AMMO(ent, IT_WEAPON_GRENADES)--;
	if (INV_AMMO(ent, IT_WEAPON_GRENADES) <= 0)
	{
		ent->client->newweapon = GetItemByIndex(IT_WEAPON_MK23);
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
	if (ent->client->weaponstate == WEAPON_FIRING && (!IS_ALIVE(ent) || lights_camera_action))
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
			// {
			// 	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			// 		SetAnimation( ent, FRAME_crpain4 + 1, FRAME_crpain1, ANIM_REVERSE );
			// 	else
			// 		SetAnimation( ent, FRAME_pain304 + 1, FRAME_pain301, ANIM_REVERSE );
			// }
			{
				ent->client->anim_priority = ANIM_REVERSED;
				if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
				{
					ent->s.frame = FRAME_crpain4 + 1;
					ent->client->anim_end = FRAME_crpain1;
				}
				else
				{
					ent->s.frame = FRAME_pain304 + 1;
					ent->client->anim_end = FRAME_pain301;
				}
				ent->client->anim_time = 0_ms;
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
			gi.LocClient_Print(ent, PRINT_HIGH, "Pin pulled, ready for %s range throw\n",
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
		//              gi.LocClient_Print(ent, PRINT_HIGH, "After increment frames = %d\n", ent->client->ps.gunframe);
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
			if (INV_AMMO(ent, IT_WEAPON_GRENADES) <= 0)
			{
				ent->client->newweapon = GetItemByIndex(IT_WEAPON_MK23);
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

		if (ent->client->pers.weapon->id == IT_WEAPON_GRENADES
			&& ((ent->client->ps.gunframe >= GRENADE_IDLE_FIRST
				&& ent->client->ps.gunframe <= GRENADE_IDLE_LAST)
				|| (ent->client->ps.gunframe >= GRENADE_THROW_FIRST
					&& ent->client->ps.gunframe <= GRENADE_THROW_LAST)))
		{
			int damage = GRENADE_DAMRAD;

			if (is_quad)
				damage *= 1.5f;

			fire_grenade2(ent, ent->s.origin, vec3_origin, damage, 0, 80_ms, damage * 2, false);

			INV_AMMO(ent, IT_WEAPON_GRENADES)--;
			if (INV_AMMO(ent, IT_WEAPON_GRENADES) <= 0)
			{
				ent->client->newweapon = GetItemByIndex(IT_WEAPON_MK23);
				ChangeWeapon(ent);
				return;
			}
		}


		ent->client->ps.gunframe = GRENADE_ACTIVATE_LAST;
		// zucc more vwep stuff
		if ((GRENADE_ACTIVATE_LAST) < 4)
		// {
		// 	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		// 		SetAnimation( ent, FRAME_crpain4 + 1, FRAME_crpain1, ANIM_REVERSE );
		// 	else
		// 		SetAnimation( ent, FRAME_pain304 + 1, FRAME_pain301, ANIM_REVERSE );
		// }
		{
			ent->client->anim_priority = ANIM_REVERSED;
			if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				ent->s.frame = FRAME_crpain4 + 1;
				ent->client->anim_end = FRAME_crpain1;
			}
			else
			{
				ent->s.frame = FRAME_pain304 + 1;
				ent->client->anim_end = FRAME_pain301;
			}
			ent->client->anim_time = 0_ms;
		}
		ent->client->weaponstate = WEAPON_DROPPING;
		return;
	}

	if (ent->client->weaponstate == WEAPON_READY)
	{
		if (((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK)
			&& (!IS_ALIVE(ent)) &&
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
			// if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			// 	SetAnimation( ent, FRAME_crattak1 - 1, FRAME_crattak9, ANIM_ATTACK );
			// else
			// 	SetAnimation( ent, FRAME_attack1 - 1, FRAME_attack8, ANIM_ATTACK );
			{
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
				ent->client->anim_time = 0_ms;
			}
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

void PlaceHolder(edict_t* ent)
{
	ent->nextthink = level.time + 1_sec;
}

edict_t *FindSpecWeapSpawn(edict_t* ent)
{
	edict_t* spot = NULL;

	//gi.bprintf (PRINT_HIGH, "Calling the FindSpecWeapSpawn\n");
	spot = G_FindByString<&edict_t::classname>(spot, ent->classname);
	//spot = G_Find(spot, FOFS(classname), ent->classname);
	//gi.bprintf (PRINT_HIGH, "spot = %p and spot->think = %p and playerholder = %p, spot, (spot ? spot->think : 0), PlaceHolder\n");
	while (spot && spot->think != PlaceHolder)	//(spot->spawnflags & SPAWNFLAG_ITEM_DROPPED ) && spot->think != PlaceHolder )//spot->solid == SOLID_NOT )        
	{
		//              gi.bprintf (PRINT_HIGH, "Calling inside the loop FindSpecWeapSpawn\n");
		spot = G_FindByString<&edict_t::classname>(spot, ent->classname);
		//spot = G_Find(spot, FOFS(classname), ent->classname);
	}
	return spot;
}

static void SpawnSpecWeap(gitem_t* item, edict_t* spot)
{
	SetRespawn(spot, 60_sec, false);
	gi.linkentity(spot);
}

void ThinkSpecWeap(edict_t* ent)
{
	edict_t* spot;

	if ((spot = FindSpecWeapSpawn(ent)) != NULL)
	{
		SpawnSpecWeap(ent->item, spot);
		G_FreeEdict(ent);
	}
	else
	{
		ent->nextthink = level.time + 10_ms;
		ent->think = G_FreeEdict;
	}
}

void temp_think_specweap(edict_t* ent)
{
	ent->touch = Touch_Item;

	if (allweapon->value) { // allweapon set
		ent->nextthink = level.time + 10_ms;
		ent->think = G_FreeEdict;
		return;
	}

	if (gameSettings & GS_ROUNDBASED) {
		ent->nextthink = level.time + 1_sec;
		ent->think = PlaceHolder;
		return;
	}

	if (gameSettings & GS_WEAPONCHOOSE) {
		ent->nextthink = level.time + 60_ms;
		ent->think = ThinkSpecWeap;
	}
	// else if (DMFLAGS(DF_WEAPON_RESPAWN)) {
	// 	ent->nextthink = level.time + 60_sec;
	// 	ent->think = G_FreeEdict;
	// }
	else {
		ent->nextthink = level.time + 1_sec;
		ent->think = ThinkSpecWeap;
	}
}

//======================================================================
// Action Add End
//======================================================================
