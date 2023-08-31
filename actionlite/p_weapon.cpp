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
		if (g_dm_no_stack_double->integer)
			return damage_multiplier;
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

bool Pickup_Weapon(edict_t *ent, edict_t *other)
{
	item_id_t index;
	gitem_t	*ammo;

	index = ent->item->id;

	if (G_WeaponShouldStay() && other->client->pers.inventory[index])
	{
		if (!(ent->spawnflags & (SPAWNFLAG_ITEM_DROPPED | SPAWNFLAG_ITEM_DROPPED_PLAYER)))
			return false; // leave the weapon for others to pickup
	}

	bool is_new = !other->client->pers.inventory[index];

	other->client->pers.inventory[index]++;

	if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED))
	{
		// give them some ammo with it
		// PGM -- IF APPROPRIATE!
		if (ent->item->ammo) // PGM
		{
			ammo = GetItemByIndex(ent->item->ammo);
			// RAFAEL: Don't get infinite ammo with trap
			if (G_CheckInfiniteAmmo(ammo))
				Add_Ammo(other, ammo, 1000);
			else
				Add_Ammo(other, ammo, ammo->quantity);
		}

		if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED_PLAYER))
		{
			if (deathmatch->integer)
			{
				if (g_dm_weapons_stay->integer)
					ent->flags |= FL_RESPAWN;

				SetRespawn( ent, gtime_t::from_sec(g_weapon_respawn_time->integer), !g_dm_weapons_stay->integer);
			}
			if (coop->integer)
				ent->flags |= FL_RESPAWN;
		}
	}

	G_CheckAutoSwitch(other, ent->item, is_new);

	return true;
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
void ChangeWeapon(edict_t *ent)
{
	// [Paril-KEX]
	if (ent->health > 0 && !g_instant_weapon_switch->integer && ((ent->client->latched_buttons | ent->client->buttons) & BUTTON_HOLSTER))
		return;

	if (ent->client->grenade_time)
	{
		// force a weapon think to drop the held grenade
		ent->client->weapon_sound = 0;
		Weapon_RunThink(ent);
		ent->client->grenade_time = 0_ms;
	}

	if (ent->client->pers.weapon)
	{
		ent->client->pers.lastweapon = ent->client->pers.weapon;

		if (ent->client->newweapon && ent->client->newweapon != ent->client->pers.weapon)
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/change.wav"), 1, ATTN_NORM, 0);
	}

	ent->client->pers.weapon = ent->client->newweapon;
	ent->client->newweapon = nullptr;
	ent->client->machinegun_shots = 0;

	// set visible model
	if (ent->s.modelindex == MODELINDEX_PLAYER)
		P_AssignClientSkinnum(ent);

	if (!ent->client->pers.weapon)
	{ // dead
		ent->client->ps.gunindex = 0;
		ent->client->ps.gunskin = 0;
		return;
	}

	ent->client->weaponstate = WEAPON_ACTIVATING;
	ent->client->ps.gunframe = 0;
	ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);
	ent->client->ps.gunskin = 0;
	ent->client->weapon_sound = 0;

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

	// for instantweap, run think immediately
	// to set up correct start frame
	if (g_instant_weapon_switch->integer)
		Weapon_RunThink(ent);
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

	if (ent->client->pers.weapon->ammo == IT_AMMO_CELLS)
		G_CheckPowerArmor(ent);
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

	if (ent->client->ps.gunframe != 0 && (!(ent->client->pers.weapon->flags & IF_NO_HASTE) || ent->client->weaponstate != WEAPON_FIRING))
	{
		if (is_quadfire)
			ent->client->ps.gunrate *= 2;
		if (CTFApplyHaste(ent))
			ent->client->ps.gunrate *= 2;
	}

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

	if (item->ammo && !g_select_empty->integer && !(item->flags & IF_AMMO))
	{
		gitem_t *ammo_item = GetItemByIndex(item->ammo);

		if (!ent->client->pers.inventory[item->ammo])
		{
			if (!silent)
				gi.LocClient_Print(ent, PRINT_HIGH, "$g_no_ammo", ammo_item->pickup_name, item->pickup_name_definite);
			return WEAP_SWITCH_NO_AMMO;
		}
		else if (ent->client->pers.inventory[item->ammo] < item->quantity)
		{
			if (!silent)
				gi.LocClient_Print(ent, PRINT_HIGH, "$g_not_enough_ammo", ammo_item->pickup_name, item->pickup_name_definite);
			return WEAP_SWITCH_NOT_ENOUGH_AMMO;
		}
	}

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
void Use_Weapon(edict_t *ent, gitem_t *item)
{
	gitem_t		*wanted, *root;
	weap_switch_t result = WEAP_SWITCH_NO_WEAPON;

	// if we're switching to a weapon in this chain already,
	// start from the weapon after this one in the chain
	if (!ent->client->no_weapon_chains && Weapon_IsPartOfChain(item, ent->client->newweapon))
	{
		root = ent->client->newweapon;
		wanted = root->chain_next;
	}
	// if we're already holding a weapon in this chain,
	// start from the weapon after that one
	else if (!ent->client->no_weapon_chains && Weapon_IsPartOfChain(item, ent->client->pers.weapon))
	{
		root = ent->client->pers.weapon;
		wanted = root->chain_next;
	}
	// start from beginning of chain (if any)
	else
		wanted = root = item;

	while (true)
	{
		// try the weapon currently in the chain
		if ((result = Weapon_AttemptSwitch(ent, wanted, false)) == WEAP_SWITCH_VALID)
			break;

		// no chains
		if (!wanted->chain_next || ent->client->no_weapon_chains)
			break;

		wanted = wanted->chain_next;

		// we wrapped back to the root item
		if (wanted == root)
			break;
	}

	if (result == WEAP_SWITCH_VALID)
		ent->client->newweapon = wanted; // change to this weapon when down
	else if ((result = Weapon_AttemptSwitch(ent, wanted, true)) == WEAP_SWITCH_NO_WEAPON && wanted != ent->client->pers.weapon && wanted != ent->client->newweapon)
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_out_of_item", wanted->pickup_name);
}

/*
================
Drop_Weapon
================
*/
void Drop_Weapon(edict_t *ent, gitem_t *item)
{
	item_id_t index = item->id;
	// see if we're already using it
	if (((item == ent->client->pers.weapon) || (item == ent->client->newweapon)) && (ent->client->pers.inventory[index] == 1))
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_cant_drop_weapon");
		return;
	}

	edict_t *drop = Drop_Item(ent, item);
	drop->spawnflags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
	drop->svflags &= ~SVF_INSTANCED;
	ent->client->pers.inventory[index]--;
}

void Weapon_PowerupSound(edict_t *ent)
{
	if (!CTFApplyStrengthSound(ent))
	{
		if (ent->client->quad_time > level.time && ent->client->double_time > level.time)
			gi.sound(ent, CHAN_ITEM, gi.soundindex("ctf/tech2x.wav"), 1, ATTN_NORM, 0);
		else if (ent->client->quad_time > level.time)
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);
		else if (ent->client->double_time > level.time)
			gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/ddamage3.wav"), 1, ATTN_NORM, 0);
		else if (ent->client->quadfire_time > level.time
			&& ent->client->ctf_techsndtime < level.time)
		{
			ent->client->ctf_techsndtime = level.time + 1_sec;
			gi.sound(ent, CHAN_ITEM, gi.soundindex("ctf/tech3.wav"), 1, ATTN_NORM, 0);
		}
	}

	CTFApplyHasteSound(ent);
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

void Weapon_Generic(edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST, const int *pause_frames, const int *fire_frames, void (*fire)(edict_t *ent))
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
	else if (auto state = Weapon_HandleReady(ent, FRAME_FIRE_FIRST, FRAME_IDLE_FIRST, FRAME_IDLE_LAST, pause_frames))
	{
		if (state == READY_FIRING)
		{
			ent->client->ps.gunframe = FRAME_FIRE_FIRST;
			ent->client->weapon_fire_buffered = false;

			if (ent->client->weapon_thunk)
				ent->client->weapon_think_time += FRAME_TIME_S;

			ent->client->weapon_think_time += Weapon_AnimationTime(ent);
			Weapon_SetFinished(ent);

			for (int n = 0; fire_frames[n]; n++)
			{
				if (ent->client->ps.gunframe == fire_frames[n])
				{
 					Weapon_PowerupSound(ent);
					fire(ent);
					break;
				}
			}

			// start the animation
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

		return;
	}

	if (ent->client->weaponstate == WEAPON_FIRING && ent->client->weapon_think_time <= level.time)
	{
		ent->client->ps.gunframe++;
		Weapon_HandleFiring(ent, FRAME_IDLE_FIRST, [&]() {
			for (int n = 0; fire_frames[n]; n++)
			{
				if (ent->client->ps.gunframe == fire_frames[n])
				{
					Weapon_PowerupSound(ent);
					fire(ent);
					break;
				}
			}
		});
	}
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

/*
======================================================================

GRENADE

======================================================================
*/

void weapon_grenade_fire(edict_t *ent, bool held)
{
	int	  damage = 125;
	int	  speed;
	float radius;

	radius = (float) (damage + 40);
	if (is_quad)
		damage *= damage_multiplier;

	vec3_t start, dir;
	// Paril: kill sideways angle on grenades
	// limit upwards angle so you don't throw behind you
	P_ProjectSource(ent, { max(-62.5f, ent->client->v_angle[0]), ent->client->v_angle[1], ent->client->v_angle[2] }, { 2, 0, -14 }, start, dir);

	gtime_t timer = ent->client->grenade_time - level.time;
	speed = (int) (ent->health <= 0 ? GRENADE_MINSPEED : min(GRENADE_MINSPEED + (GRENADE_TIMER - timer).seconds() * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER.seconds()), GRENADE_MAXSPEED));

	ent->client->grenade_time = 0_ms;

	fire_grenade2(ent, start, dir, damage, speed, timer, radius, held);

	G_RemoveAmmo(ent, 1);
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

			if (CTFApplyHaste(ent))
				grenade_wait_time *= 0.5f;
			if (is_quadfire)
				grenade_wait_time *= 0.5f;

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

void Weapon_GrenadeLauncher(edict_t *ent)
{
	constexpr int pause_frames[] = { 34, 51, 59, 0 };
	constexpr int fire_frames[] = { 6, 0 };

	Weapon_Generic(ent, 5, 16, 59, 64, pause_frames, fire_frames, weapon_grenadelauncher_fire);
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

void Weapon_RocketLauncher(edict_t *ent)
{
	constexpr int pause_frames[] = { 25, 33, 42, 50, 0 };
	constexpr int fire_frames[] = { 5, 0 };

	Weapon_Generic(ent, 4, 12, 50, 54, pause_frames, fire_frames, Weapon_RocketLauncher_Fire);
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

void Weapon_Blaster(edict_t *ent)
{
	constexpr int pause_frames[] = { 19, 32, 0 };
	constexpr int fire_frames[] = { 5, 0 };

	Weapon_Generic(ent, 4, 8, 52, 55, pause_frames, fire_frames, Weapon_Blaster_Fire);
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
	fire_bullet(ent, start, dir, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_MACHINEGUN);
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

		fire_bullet(ent, start, dir, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_CHAINGUN);
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
		fire_shotgun(ent, start, dir, damage, kick, 500, 500, DEFAULT_DEATHMATCH_SHOTGUN_COUNT, MOD_SHOTGUN);
	else
		fire_shotgun(ent, start, dir, damage, kick, 500, 500, DEFAULT_SHOTGUN_COUNT, MOD_SHOTGUN);
	G_UnLagCompensate();

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_SHOTGUN | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);
	
	G_RemoveAmmo(ent);
}

void Weapon_Shotgun(edict_t *ent)
{
	constexpr int pause_frames[] = { 22, 28, 34, 0 };
	constexpr int fire_frames[] = { 8, 0 };

	Weapon_Generic(ent, 7, 18, 36, 39, pause_frames, fire_frames, weapon_shotgun_fire);
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
	fire_shotgun(ent, start, dir, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT / 2, MOD_SSHOTGUN);
	v[YAW] = ent->client->v_angle[YAW] + 5;
	P_ProjectSource(ent, v, { 0, 0, -8 }, start, dir);
	fire_shotgun(ent, start, dir, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT / 2, MOD_SSHOTGUN);
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

void Weapon_SuperShotgun(edict_t *ent)
{
	constexpr int pause_frames[] = { 29, 42, 57, 0 };
	constexpr int fire_frames[] = { 7, 0 };

	Weapon_Generic(ent, 6, 17, 57, 61, pause_frames, fire_frames, weapon_supershotgun_fire);
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

void Weapon_Railgun(edict_t *ent)
{
	constexpr int pause_frames[] = { 56, 0 };
	constexpr int fire_frames[] = { 4, 0 };

	Weapon_Generic(ent, 3, 18, 56, 61, pause_frames, fire_frames, weapon_railgun_fire);
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

void Weapon_BFG(edict_t *ent)
{
	constexpr int pause_frames[] = { 39, 45, 50, 55, 0 };
	constexpr int fire_frames[] = { 9, 17, 0 };

	Weapon_Generic(ent, 8, 32, 54, 58, pause_frames, fire_frames, weapon_bfg_fire);
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

void Weapon_Beta_Disintegrator(edict_t *ent)
{
	constexpr int pause_frames[] = { 30, 37, 45, 0 };
	constexpr int fire_frames[] = { 17, 0 };

	Weapon_Generic(ent, 16, 23, 46, 50, pause_frames, fire_frames, weapon_disint_fire);
}

//======================================================================
// Action Add
//======================================================================

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
			if (esp->value && esp_leaderenhance->value)
				ent->client->idle_weapon = ENHANCED_BANDAGE_TIME;
			else
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
			if (esp->value && esp_leaderenhance->value)
				ent->client->idle_weapon = ENHANCED_BANDAGE_TIME;
			else
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

//======================================================================
// Action Add
//======================================================================