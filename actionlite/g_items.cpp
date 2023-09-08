// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"
#include "bots/bot_includes.h"

bool Pickup_Weapon(edict_t *ent, edict_t *other);
void Use_Weapon(edict_t *ent, gitem_t *inv);
void Drop_Weapon(edict_t *ent, gitem_t *inv);

void Weapon_Blaster(edict_t *ent);
void Weapon_Shotgun(edict_t *ent);
void Weapon_SuperShotgun(edict_t *ent);
void Weapon_Machinegun(edict_t *ent);
void Weapon_Chaingun(edict_t *ent);
void Weapon_HyperBlaster(edict_t *ent);
void Weapon_RocketLauncher(edict_t *ent);
void Weapon_Grenade(edict_t *ent);
void Weapon_GrenadeLauncher(edict_t *ent);
void Weapon_Railgun(edict_t *ent);
void Weapon_BFG(edict_t *ent);
// RAFAEL
void Weapon_Ionripper(edict_t *ent);
void Weapon_Phalanx(edict_t *ent);
void Weapon_Trap(edict_t *ent);
// RAFAEL
// ROGUE
void Weapon_ChainFist(edict_t *ent);
void Weapon_Disintegrator(edict_t *ent);
void Weapon_ETF_Rifle(edict_t *ent);
void Weapon_Heatbeam(edict_t *ent);
void Weapon_Prox(edict_t *ent);
void Weapon_Tesla(edict_t *ent);
void Weapon_ProxLauncher(edict_t *ent);
// ROGUE
void Weapon_Beta_Disintegrator(edict_t *ent);

void	   Use_Quad(edict_t *ent, gitem_t *item);
static gtime_t quad_drop_timeout_hack;

// RAFAEL
void	   Use_QuadFire(edict_t *ent, gitem_t *item);
static gtime_t quad_fire_drop_timeout_hack;
// RAFAEL

// ACTION
bool Pickup_Weapon (edict_t * ent, edict_t * other);
void Use_Weapon (edict_t * ent, gitem_t * inv);
void Drop_Weapon (edict_t * ent, gitem_t * inv);

void Weapon_MK23 (edict_t * ent);
void Weapon_MP5 (edict_t * ent);
void Weapon_M4 (edict_t * ent);
void Weapon_M3 (edict_t * ent);
void Weapon_HC (edict_t * ent);
void Weapon_Sniper (edict_t * ent);
void Weapon_Dual (edict_t * ent);
void Weapon_Knife (edict_t * ent);
void Weapon_Gas (edict_t * ent);

void SpecThink(edict_t * spec);
static void SpawnSpec(gitem_t * item, edict_t * spot);

#define HEALTH_IGNORE_MAX       1
#define HEALTH_TIMED            2
#define HEALTH_MEDKIT           4
// ACTION

//======================================================================

/*
===============
GetItemByIndex
===============
*/
gitem_t *GetItemByIndex(item_id_t index)
{
	if (index <= IT_NULL || index >= IT_TOTAL)
		return nullptr;

	return &itemlist[index];
}

static gitem_t *ammolist[AMMO_MAX];

gitem_t *GetItemByAmmo(ammo_t ammo)
{
	return ammolist[ammo];
}

static gitem_t *poweruplist[POWERUP_MAX];

gitem_t *GetItemByPowerup(powerup_t powerup)
{
	return poweruplist[powerup];
}

/*
===============
FindItemByClassname

===============
*/
gitem_t *FindItemByClassname(const char *classname)
{
	int		 i;
	gitem_t *it;

	it = itemlist;
	for (i = 0; i < IT_TOTAL; i++, it++)
	{
		if (!it->classname)
			continue;
		if (!Q_strcasecmp(it->classname, classname))
			return it;
	}

	return nullptr;
}

/*
===============
FindItem

===============
*/
gitem_t *FindItem(const char *pickup_name)
{
	int		 i;
	gitem_t *it;

	it = itemlist;
	for (i = 0; i < IT_TOTAL; i++, it++)
	{
		if (!it->use_name)
			continue;
		if (!Q_strcasecmp(it->use_name, pickup_name))
			return it;
	}

	return nullptr;
}

//======================================================================

THINK(DoRespawn) (edict_t *ent) -> void
{
	if (ent->team)
	{
		edict_t *master;
		int		 count;
		int		 choice;

		master = ent->teammaster;

		// ZOID
		// in ctf, when we are weapons stay, only the master of a team of weapons
		// is spawned
		if (ctf->integer && g_dm_weapons_stay->integer && master->item && (master->item->flags & IF_WEAPON))
			ent = master;
		else
		{
			// ZOID

			for (count = 0, ent = master; ent; ent = ent->chain, count++)
				;

			choice = irandom(count);

			for (count = 0, ent = master; count < choice; ent = ent->chain, count++)
				;
		}
	}

	ent->svflags &= ~SVF_NOCLIENT;
	ent->svflags &= ~SVF_RESPAWNING;
	ent->solid = SOLID_TRIGGER;
	gi.linkentity(ent);

	// send an effect
	ent->s.event = EV_ITEM_RESPAWN;

	// ROGUE
	// if (g_dm_random_items->integer)
	// {
	// 	item_id_t new_item = DoRandomRespawn(ent);

	// 	// if we've changed entities, then do some sleight of hand.
	// 	// otherwise, the old entity will respawn
	// 	if (new_item)
	// 	{
	// 		ent->item = GetItemByIndex(new_item);

	// 		ent->classname = ent->item->classname;
	// 		ent->s.effects = ent->item->world_model_flags;
	// 		gi.setmodel(ent, ent->item->world_model);
	// 	}
	// }
	// ROGUE
}

void SetRespawn(edict_t *ent, gtime_t delay, bool hide_self)
{
	// already respawning
	if (ent->think == DoRespawn && ent->nextthink >= level.time)
		return;

	ent->flags |= FL_RESPAWN;

	if (hide_self)
	{
		ent->svflags |= ( SVF_NOCLIENT | SVF_RESPAWNING );
		ent->solid = SOLID_NOT;
		gi.linkentity(ent);
	}

	ent->nextthink = level.time + delay;
	ent->think = DoRespawn;
}

//======================================================================

bool IsInstantItemsEnabled()
{
	if (deathmatch->integer && g_dm_instant_items->integer)
	{
		return true;
	}

	if (!deathmatch->integer && level.instantitems)
	{
		return true;
	}

	return false;
}

bool Pickup_Powerup(edict_t *ent, edict_t *other)
{
	int quantity;

	quantity = other->client->pers.inventory[ent->item->id];
	if ((skill->integer == 0 && quantity >= 3) ||
		(skill->integer == 1 && quantity >= 2) ||
		(skill->integer >= 2 && quantity >= 1))
		return false;

	if (coop->integer && !P_UseCoopInstancedItems() && (ent->item->flags & IF_STAY_COOP) && (quantity > 0))
		return false;

	other->client->pers.inventory[ent->item->id]++;

	bool is_dropped_from_death = ent->spawnflags.has(SPAWNFLAG_ITEM_DROPPED_PLAYER) && !ent->spawnflags.has(SPAWNFLAG_ITEM_DROPPED);

	if (IsInstantItemsEnabled() ||
		((ent->item->use == Use_Quad) && is_dropped_from_death) ||
		((ent->item->use == Use_QuadFire) && is_dropped_from_death))
	{
		if ((ent->item->use == Use_Quad) && is_dropped_from_death)
			quad_drop_timeout_hack = (ent->nextthink - level.time);
		else if ((ent->item->use == Use_QuadFire) && is_dropped_from_death)
			quad_fire_drop_timeout_hack = (ent->nextthink - level.time);

		if (ent->item->use)
			ent->item->use(other, ent->item);
	}

	if (deathmatch->integer)
	{
		if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED))
			SetRespawn(ent, gtime_t::from_sec(ent->item->quantity));
	}

	return true;
}

bool Pickup_General(edict_t *ent, edict_t *other)
{
	if (other->client->pers.inventory[ent->item->id])
		return false;

	other->client->pers.inventory[ent->item->id]++;

	if (deathmatch->integer)
	{
		if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED))
			SetRespawn(ent, gtime_t::from_sec(ent->item->quantity));
	}

	return true;
}

void Drop_General(edict_t *ent, gitem_t *item)
{
	edict_t *dropped = Drop_Item(ent, item);
	dropped->spawnflags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
	dropped->svflags &= ~SVF_INSTANCED;
	ent->client->pers.inventory[item->id]--;
}

//======================================================================

void Use_Adrenaline(edict_t *ent, gitem_t *item)
{
	if (!deathmatch->integer)
		ent->max_health += 1;

	if (ent->health < ent->max_health)
		ent->health = ent->max_health;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/n_health.wav"), 1, ATTN_NORM, 0);

	ent->client->pers.inventory[item->id]--;
}

bool Pickup_LegacyHead(edict_t *ent, edict_t *other)
{
	other->max_health += 5;
	other->health += 5;

	if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED) && deathmatch->integer)
		SetRespawn(ent, gtime_t::from_sec(ent->item->quantity));

	return true;
}

inline bool G_AddAmmoAndCap(edict_t *other, item_id_t item, int32_t max, int32_t quantity)
{
	if (other->client->pers.inventory[item] >= max)
		return false;

	other->client->pers.inventory[item] += quantity;
	if (other->client->pers.inventory[item] > max)
		other->client->pers.inventory[item] = max;

	//G_CheckPowerArmor(other);

	return true;
}

inline bool G_AddAmmoAndCapQuantity(edict_t *other, ammo_t ammo)
{
	gitem_t *item = GetItemByAmmo(ammo);
	return G_AddAmmoAndCap(other, item->id, other->client->pers.max_ammo[ammo], item->quantity);
}

inline void G_AdjustAmmoCap(edict_t *other, ammo_t ammo, int16_t new_max)
{
	other->client->pers.max_ammo[ammo] = max(other->client->pers.max_ammo[ammo], new_max);
}

bool Pickup_Bandolier(edict_t *ent, edict_t *other)
{
	G_AdjustAmmoCap(other, AMMO_BULLETS, 250);
	G_AdjustAmmoCap(other, AMMO_SHELLS, 150);
	G_AdjustAmmoCap(other, AMMO_CELLS, 250);
	G_AdjustAmmoCap(other, AMMO_SLUGS, 75);


	G_AddAmmoAndCapQuantity(other, AMMO_BULLETS);
	G_AddAmmoAndCapQuantity(other, AMMO_SHELLS);

	if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED) && deathmatch->integer)
		SetRespawn(ent, gtime_t::from_sec(ent->item->quantity));

	return true;
}

bool Pickup_Pack(edict_t *ent, edict_t *other)
{
	G_AdjustAmmoCap(other, AMMO_BULLETS, 300);
	G_AdjustAmmoCap(other, AMMO_SHELLS, 200);
	G_AdjustAmmoCap(other, AMMO_ROCKETS, 100);
	G_AdjustAmmoCap(other, AMMO_CELLS, 300);
	G_AdjustAmmoCap(other, AMMO_SLUGS, 100);

	G_AddAmmoAndCapQuantity(other, AMMO_BULLETS);
	G_AddAmmoAndCapQuantity(other, AMMO_SHELLS);
	G_AddAmmoAndCapQuantity(other, AMMO_CELLS);
	G_AddAmmoAndCapQuantity(other, AMMO_ROCKETS);
	G_AddAmmoAndCapQuantity(other, AMMO_SLUGS);


	if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED) && deathmatch->integer)
		SetRespawn(ent, gtime_t::from_sec(ent->item->quantity));

	return true;
}

//======================================================================

void Use_Quad(edict_t *ent, gitem_t *item)
{
	gtime_t timeout;

	ent->client->pers.inventory[item->id]--;

	if (quad_drop_timeout_hack)
	{
		timeout = quad_drop_timeout_hack;
		quad_drop_timeout_hack = 0_ms;
	}
	else
	{
		timeout = 30_sec;
	}

	ent->client->quad_time = max(level.time, ent->client->quad_time) + timeout;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}
// =====================================================================

// RAFAEL
void Use_QuadFire(edict_t *ent, gitem_t *item)
{
	gtime_t timeout;

	ent->client->pers.inventory[item->id]--;

	if (quad_fire_drop_timeout_hack)
	{
		timeout = quad_fire_drop_timeout_hack;
		quad_fire_drop_timeout_hack = 0_ms;
	}
	else
	{
		timeout = 30_sec;
	}

	ent->client->quadfire_time = max(level.time, ent->client->quadfire_time) + timeout;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/quadfire1.wav"), 1, ATTN_NORM, 0);
}
// RAFAEL

//======================================================================

void Use_Breather(edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[item->id]--;

	ent->client->breather_time = max(level.time, ent->client->breather_time) + 30_sec;

	//	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void Use_Envirosuit(edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[item->id]--;

	ent->client->enviro_time = max(level.time, ent->client->enviro_time) + 30_sec;

	//	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void Use_Invulnerability(edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[item->id]--;

	ent->client->invincible_time = max(level.time, ent->client->invincible_time) + 30_sec;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect.wav"), 1, ATTN_NORM, 0);
}

void Use_Invisibility(edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[item->id]--;

	ent->client->invisible_time = max(level.time, ent->client->invisible_time) + 30_sec;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void Use_Silencer(edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[item->id]--;
	ent->client->silencer_shots += 30;

	//	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

bool Add_Ammo(edict_t *ent, gitem_t *item, int count)
{
	if (!ent->client || item->tag < AMMO_BULLETS || item->tag >= AMMO_MAX)
		return false;

	return G_AddAmmoAndCap(ent, item->id, ent->client->pers.max_ammo[item->tag], count);
}

// we just got weapon `item`, check if we should switch to it
void G_CheckAutoSwitch(edict_t *ent, gitem_t *item, bool is_new)
{
	// already using or switching to
	if (ent->client->pers.weapon == item ||
		ent->client->newweapon == item)
		return;
	// need ammo
	else if (item->ammo)
	{
		int32_t required_ammo = (item->flags & IF_AMMO) ? 1 : item->quantity;
		
		if (ent->client->pers.inventory[item->ammo] < required_ammo)
			return;
	}

	// check autoswitch setting
	if (ent->client->pers.autoswitch == auto_switch_t::NEVER)
		return;
	else if ((item->flags & IF_AMMO) && ent->client->pers.autoswitch == auto_switch_t::ALWAYS_NO_AMMO)
		return;
	else if (ent->client->pers.autoswitch == auto_switch_t::SMART)
	{
		bool using_blaster = ent->client->pers.weapon && ent->client->pers.weapon->id == IT_WEAPON_MK23;

		// smartness algorithm: in DM, we will always switch if we have the blaster out
		// otherwise leave our active weapon alone
		if (deathmatch->integer && !using_blaster)
			return;
		// in SP, only switch if it's a new weapon, or we have the blaster out
		else if (!deathmatch->integer && !using_blaster && !is_new)
			return;
	}

	// switch!
	ent->client->newweapon = item;
}

bool Pickup_Ammo(edict_t *ent, edict_t *other)
{
	int	 oldcount;
	int	 count;
	bool weapon;

	weapon = (ent->item->flags & IF_WEAPON);
	if (weapon && G_CheckInfiniteAmmo(ent->item))
		count = 1000;
	else if (ent->count)
		count = ent->count;
	else
		count = ent->item->quantity;

	oldcount = other->client->pers.inventory[ent->item->id];

	if (!Add_Ammo(other, ent->item, count))
		return false;

	if (weapon)
		G_CheckAutoSwitch(other, ent->item, !oldcount);

	if (!(ent->spawnflags & (SPAWNFLAG_ITEM_DROPPED | SPAWNFLAG_ITEM_DROPPED_PLAYER)) && deathmatch->integer)
		SetRespawn(ent, 30_sec);
	return true;
}

void Drop_Ammo(edict_t *ent, gitem_t *item)
{
	item_id_t index = item->id;
	edict_t *dropped = Drop_Item(ent, item);
	dropped->spawnflags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
	dropped->svflags &= ~SVF_INSTANCED;

	if (ent->client->pers.inventory[index] >= item->quantity)
		dropped->count = item->quantity;
	else
		dropped->count = ent->client->pers.inventory[index];

	if (ent->client->pers.weapon && ent->client->pers.weapon == item && (item->flags & IF_AMMO) &&
		ent->client->pers.inventory[index] - dropped->count <= 0)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_cant_drop_weapon");
		G_FreeEdict(dropped);
		return;
	}

	ent->client->pers.inventory[index] -= dropped->count;
	//G_CheckPowerArmor(ent);
}

//======================================================================

THINK(MegaHealth_think) (edict_t *self) -> void
{
	if (self->owner->health > self->owner->max_health)
	{
		self->nextthink = level.time + 1_sec;
		self->owner->health -= 1;
		return;
	}

	if (!(self->spawnflags & SPAWNFLAG_ITEM_DROPPED) && deathmatch->integer)
		SetRespawn(self, 20_sec);
	else
		G_FreeEdict(self);
}

bool Pickup_Health(edict_t *ent, edict_t *other)
{
	int health_flags = (ent->style ? ent->style : ent->item->tag);

	if (!(health_flags & HEALTH_IGNORE_MAX))
		if (other->health >= other->max_health)
			return false;

	int count = ent->count ? ent->count : ent->item->quantity;

	// ZOID
	if (deathmatch->integer && other->health >= 250 && count > 25)
		return false;
	// ZOID

	other->health += count;

	if (!(health_flags & HEALTH_IGNORE_MAX))
	{
		if (other->health > other->max_health)
			other->health = other->max_health;
	}

	if (ent->item->tag & HEALTH_TIMED)
	{
		if (!deathmatch->integer)
		{
			// mega health doesn't need to be special in SP
			// since it never respawns.
			other->client->pers.megahealth_time = 5_sec;
		}
		else
		{
			ent->think = MegaHealth_think;
			ent->nextthink = level.time + 5_sec;
			ent->owner = other;
			ent->flags |= FL_RESPAWN;
			ent->svflags |= SVF_NOCLIENT;
			ent->solid = SOLID_NOT;
		}
	}
	else
	{
		if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED) && deathmatch->integer)
			SetRespawn(ent, 30_sec);
	}

	return true;
}

//======================================================================

bool Entity_IsVisibleToPlayer(edict_t* ent, edict_t* player)
{
	return !ent->item_picked_up_by[player->s.number - 1];
}

/*
===============
Touch_Item
===============
*/
TOUCH(Touch_Item) (edict_t *ent, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	bool taken;

	if (!other->client)
		return;
	if (other->health < 1)
		return; // dead people can't pickup
	if (!ent->item->pickup)
		return; // not a grabbable item?

	// already got this instanced item
	if (coop->integer && P_UseCoopInstancedItems())
	{
		if (ent->item_picked_up_by[other->s.number - 1])
			return;
	}

	// ZOID
	if (CTFMatchSetup())
		return; // can't pick stuff up right now
				// ZOID

	taken = ent->item->pickup(ent, other);

	ValidateSelectedItem(other);

	if (taken)
	{
		// flash the screen
		other->client->bonus_alpha = 0.25;

		// show icon and name on status bar
		other->client->ps.stats[STAT_PICKUP_ICON] = gi.imageindex(ent->item->icon);
		other->client->ps.stats[STAT_PICKUP_STRING] = CS_ITEMS + ent->item->id;
		other->client->pickup_msg_time = level.time + 3_sec;

		// change selected item if we still have it
		if (ent->item->use && other->client->pers.inventory[ent->item->id])
		{
			other->client->ps.stats[STAT_SELECTED_ITEM] = other->client->pers.selected_item = ent->item->id;
			other->client->ps.stats[STAT_SELECTED_ITEM_NAME] = 0; // don't set name on pickup item since it's already there
		}

		if (ent->noise_index)
			gi.sound(other, CHAN_ITEM, ent->noise_index, 1, ATTN_NORM, 0);
		else if (ent->item->pickup_sound)
			gi.sound(other, CHAN_ITEM, gi.soundindex(ent->item->pickup_sound), 1, ATTN_NORM, 0);
		
		int32_t player_number = other->s.number - 1;

		if (coop->integer && P_UseCoopInstancedItems() && !ent->item_picked_up_by[player_number])
		{
			ent->item_picked_up_by[player_number] = true;

			// [Paril-KEX] this is to fix a coop quirk where items
			// that send a message on pick up will only print on the
			// player that picked them up, and never anybody else; 
			// when instanced items are enabled we don't need to limit
			// ourselves to this, but it does mean that relays that trigger
			// messages won't work, so we'll have to fix those
			if (ent->message)
				G_PrintActivationMessage(ent, other, false);
		}
	}

	if (!(ent->spawnflags & SPAWNFLAG_ITEM_TARGETS_USED))
	{
		// [Paril-KEX] see above msg; this also disables the message in DM
		// since there's no need to print pickup messages in DM (this wasn't
		// even a documented feature, relays were traditionally used for this)
		const char *message_backup = nullptr;

		if (deathmatch->integer || (coop->integer && P_UseCoopInstancedItems()))
			std::swap(message_backup, ent->message);

		G_UseTargets(ent, other);
		
		if (deathmatch->integer || (coop->integer && P_UseCoopInstancedItems()))
			std::swap(message_backup, ent->message);

		ent->spawnflags |= SPAWNFLAG_ITEM_TARGETS_USED;
	}

	if (taken)
	{
		bool should_remove = false;

		if (coop->integer)
		{
			// in coop with instanced items, *only* dropped 
			// player items will ever get deleted permanently.
			if (P_UseCoopInstancedItems())
				should_remove = ent->spawnflags.has(SPAWNFLAG_ITEM_DROPPED_PLAYER);
			// in coop without instanced items, IF_STAY_COOP items remain
			// if not dropped
			else
				should_remove = ent->spawnflags.has(SPAWNFLAG_ITEM_DROPPED | SPAWNFLAG_ITEM_DROPPED_PLAYER) || !(ent->item->flags & IF_STAY_COOP);
		}
		else
			should_remove = !deathmatch->integer || ent->spawnflags.has(SPAWNFLAG_ITEM_DROPPED | SPAWNFLAG_ITEM_DROPPED_PLAYER);

		if (should_remove)
		{
			if (ent->flags & FL_RESPAWN)
				ent->flags &= ~FL_RESPAWN;
			else
				G_FreeEdict(ent);
		}
	}
}

//======================================================================

TOUCH(drop_temp_touch) (edict_t *ent, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	if (other == ent->owner)
		return;

	Touch_Item(ent, other, tr, other_touching_self);
}

THINK(drop_make_touchable) (edict_t *ent) -> void
{
	ent->touch = Touch_Item;
	if (deathmatch->integer)
	{
		ent->nextthink = level.time + 29_sec;
		ent->think = G_FreeEdict;
	}
}

edict_t *Drop_Item(edict_t *ent, gitem_t *item)
{
	edict_t *dropped;
	vec3_t	 forward, right;
	vec3_t	 offset;

	dropped = G_Spawn();

	dropped->item = item;
	dropped->spawnflags = SPAWNFLAG_ITEM_DROPPED;
	dropped->classname = item->classname;
	dropped->s.effects = item->world_model_flags;
	gi.setmodel(dropped, dropped->item->world_model);
	dropped->s.renderfx = RF_GLOW | RF_NO_LOD | RF_IR_VISIBLE; // PGM
	dropped->mins = { -15, -15, -15 };
	dropped->maxs = { 15, 15, 15 };
	dropped->solid = SOLID_TRIGGER;
	dropped->movetype = MOVETYPE_TOSS;
	dropped->touch = drop_temp_touch;
	dropped->owner = ent;

	if (ent->client)
	{
		trace_t trace;

		AngleVectors(ent->client->v_angle, forward, right, nullptr);
		offset = { 24, 0, -16 };
		dropped->s.origin = G_ProjectSource(ent->s.origin, offset, forward, right);
		trace = gi.trace(ent->s.origin, dropped->mins, dropped->maxs, dropped->s.origin, ent, CONTENTS_SOLID);
		dropped->s.origin = trace.endpos;
	}
	else
	{
		AngleVectors(ent->s.angles, forward, right, nullptr);
		dropped->s.origin = (ent->absmin + ent->absmax) / 2;
	}

	G_FixStuckObject(dropped, dropped->s.origin);

	dropped->velocity = forward * 100;
	dropped->velocity[2] = 300;

	dropped->think = drop_make_touchable;
	dropped->nextthink = level.time + 1_sec;

	if (coop->integer && P_UseCoopInstancedItems())
		dropped->svflags |= SVF_INSTANCED;

	gi.linkentity(dropped);
	return dropped;
}

USE(Use_Item) (edict_t *ent, edict_t *other, edict_t *activator) -> void
{
	ent->svflags &= ~SVF_NOCLIENT;
	ent->use = nullptr;

	if (ent->spawnflags.has(SPAWNFLAG_ITEM_NO_TOUCH))
	{
		ent->solid = SOLID_BBOX;
		ent->touch = nullptr;
	}
	else
	{
		ent->solid = SOLID_TRIGGER;
		ent->touch = Touch_Item;
	}

	gi.linkentity(ent);
}

//======================================================================

/*
================
droptofloor
================
*/
THINK(droptofloor) (edict_t *ent) -> void
{
	trace_t tr;
	vec3_t	dest;

	// [Paril-KEX] scale foodcube based on how much we ingested
	if (strcmp(ent->classname, "item_foodcube") == 0)
	{
		ent->mins = vec3_t { -8, -8, -8 } * ent->s.scale;
		ent->maxs = vec3_t { 8, 8, 8 } * ent->s.scale;
	}
	else
	{
		ent->mins = { -15, -15, -15 };
		ent->maxs = { 15, 15, 15 };
	}

	if (ent->model)
		gi.setmodel(ent, ent->model);
	else
		gi.setmodel(ent, ent->item->world_model);
	ent->solid = SOLID_TRIGGER;
	ent->movetype = MOVETYPE_TOSS;
	ent->touch = Touch_Item;

	dest = ent->s.origin + vec3_t { 0, 0, -128 };

	tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, dest, ent, MASK_SOLID);
	if (tr.startsolid)
	{
		if (G_FixStuckObject(ent, ent->s.origin) == stuck_result_t::NO_GOOD_POSITION)
		{
			// RAFAEL
			if (strcmp(ent->classname, "item_foodcube") == 0)
				ent->velocity[2] = 0;
			else
			{
				// RAFAEL
				gi.Com_PrintFmt("{}: droptofloor: startsolid\n", *ent);
				G_FreeEdict(ent);
				return;
				// RAFAEL
			}
			// RAFAEL
		}
	}
	else
		ent->s.origin = tr.endpos;

	if (ent->team)
	{
		ent->flags &= ~FL_TEAMSLAVE;
		ent->chain = ent->teamchain;
		ent->teamchain = nullptr;

		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;

		if (ent == ent->teammaster)
		{
			ent->nextthink = level.time + 10_hz;
			ent->think = DoRespawn;
		}
	}

	if (ent->spawnflags.has(SPAWNFLAG_ITEM_NO_TOUCH))
	{
		ent->solid = SOLID_BBOX;
		ent->touch = nullptr;
		ent->s.effects &= ~(EF_ROTATE | EF_BOB);
		ent->s.renderfx &= ~RF_GLOW;
	}

	if (ent->spawnflags.has(SPAWNFLAG_ITEM_TRIGGER_SPAWN))
	{
		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
		ent->use = Use_Item;
	}

	ent->watertype = gi.pointcontents(ent->s.origin);
	gi.linkentity(ent);
}

/*
===============
PrecacheItem

Precaches all data needed for a given item.
This will be called for each item spawned in a level,
and for each item in each client's inventory.
===============
*/
void PrecacheItem(gitem_t *it)
{
	const char *s, *start;
	char		data[MAX_QPATH];
	ptrdiff_t	len;
	gitem_t	*ammo;

	if (!it)
		return;

	if (it->pickup_sound)
		gi.soundindex(it->pickup_sound);
	if (it->world_model)
		gi.modelindex(it->world_model);
	if (it->view_model)
		gi.modelindex(it->view_model);
	if (it->icon)
		gi.imageindex(it->icon);

	// parse everything for its ammo
	if (it->ammo)
	{
		ammo = GetItemByIndex(it->ammo);
		if (ammo != it)
			PrecacheItem(ammo);
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
		if (len >= MAX_QPATH || len < 5)
			gi.Com_ErrorFmt("PrecacheItem: {} has bad precache string", it->classname);
		memcpy(data, start, len);
		data[len] = 0;
		if (*s)
			s++;

		// determine type based on extension
		if (!strcmp(data + len - 3, "md2"))
			gi.modelindex(data);
		else if (!strcmp(data + len - 3, "sp2"))
			gi.modelindex(data);
		else if (!strcmp(data + len - 3, "wav"))
			gi.soundindex(data);
		if (!strcmp(data + len - 3, "pcx"))
			gi.imageindex(data);
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
void SpawnItem(edict_t *ent, gitem_t *item)
{
	// [Sam-KEX]
	// Paril: allow all keys to be trigger_spawn'd (N64 uses this
	// a few different times)
	if (item->flags & IF_KEY)
	{
		if (ent->spawnflags.has(SPAWNFLAG_ITEM_TRIGGER_SPAWN))
		{
			ent->svflags |= SVF_NOCLIENT;
			ent->solid = SOLID_NOT;
			ent->use = Use_Item;
		}
		if (ent->spawnflags.has(SPAWNFLAG_ITEM_NO_TOUCH))
		{
			ent->solid = SOLID_BBOX;
			ent->touch = nullptr;
			ent->s.effects &= ~(EF_ROTATE | EF_BOB);
			ent->s.renderfx &= ~RF_GLOW;
		}
	}
	else if (ent->spawnflags.value >= SPAWNFLAG_ITEM_MAX.value) // PGM
	{
		ent->spawnflags = SPAWNFLAG_NONE;
		gi.Com_PrintFmt("{} has invalid spawnflags set\n", *ent);
	}

	// some items will be prevented in deathmatch
	if (deathmatch->integer)
	{
		// [Kex] In instagib, spawn no pickups!
		if (g_instagib->value)
		{
			if (item->pickup == Pickup_Powerup ||
				(item->flags & IF_HEALTH) || (item->flags & IF_AMMO) || item->pickup == Pickup_Weapon || item->pickup == Pickup_Pack)
			{
				G_FreeEdict(ent);
				return;
			}
		}

		if (g_no_items->integer)
		{
			if (item->pickup == Pickup_Powerup)
			{
				G_FreeEdict(ent);
				return;
			}
		// 	//=====
		// 	// ROGUE
		// 	if (item->pickup == Pickup_Sphere)
		// 	{
		// 		G_FreeEdict(ent);
		// 		return;
		// 	}
		// 	if (item->pickup == Pickup_Doppleganger)
		// 	{
		// 		G_FreeEdict(ent);
		// 		return;
		// 	}
		// 	// ROGUE
		// 	//=====
		// }
		}
		
		if (g_no_health->integer)
		{
			if (item->flags & IF_HEALTH)
			{
				G_FreeEdict(ent);
				return;
			}
		}
		if (G_CheckInfiniteAmmo(item))
		{
			if (item->flags == IF_AMMO)
			{
				G_FreeEdict(ent);
				return;
			}
		}

		// if (g_no_spheres->integer)
		// {
		// 	if (item->pickup == Pickup_Sphere)
		// 	{
		// 		G_FreeEdict(ent);
		// 		return;
		// 	}
		// }
		// ROGUE
		//==========
	}

	//==========
	// ROGUE
	// DM only items
	// if (!deathmatch->integer)
	// {
	// 	if (item->pickup == Pickup_Doppleganger || item->pickup == Pickup_Nuke)
	// 	{
	// 		gi.Com_PrintFmt("{} spawned in non-DM; freeing...\n", *ent);
	// 		G_FreeEdict(ent);
	// 		return;
	// 	}
	// 	if ((item->use == Use_Vengeance) || (item->use == Use_Hunter))
	// 	{
	// 		gi.Com_PrintFmt("{} spawned in non-DM; freeing...\n", *ent);
	// 		G_FreeEdict(ent);
	// 		return;
	// 	}
	// }
	// ZOID
	// Don't spawn the flags unless enabled
	if (!ctf->integer && (item->id == IT_FLAG1 || item->id == IT_FLAG2))
	{
		G_FreeEdict(ent);
		return;
	}
	// ZOID

	// set final classname now
	ent->classname = item->classname;

	PrecacheItem(item);

	// mark all items as instanced
	if (coop->integer)
	{
		if (P_UseCoopInstancedItems())
			ent->svflags |= SVF_INSTANCED;
	}

	ent->item = item;
	ent->nextthink = level.time + 20_hz; // items start after other solids
	ent->think = droptofloor;
	ent->s.effects = item->world_model_flags;
	ent->s.renderfx = RF_GLOW | RF_NO_LOD;
	if (ent->model)
		gi.modelindex(ent->model);

	// if (ent->spawnflags.has(SPAWNFLAG_ITEM_TRIGGER_SPAWN))
	// 	SetTriggeredSpawn(ent);

	// ZOID
	// flags are server animated and have special handling
	if (item->id == IT_FLAG1 || item->id == IT_FLAG2)
	{
		ent->think = CTFFlagSetup;
	}
	// ZOID
}

void P_ToggleFlashlight(edict_t *ent, bool state)
{
	if (!!(ent->flags & FL_FLASHLIGHT) == state)
		return;

	ent->flags ^= FL_FLASHLIGHT;

	gi.sound(ent, CHAN_AUTO, gi.soundindex(ent->flags & FL_FLASHLIGHT ? "items/flashlight_on.wav" : "items/flashlight_off.wav"), 1.f, ATTN_STATIC, 0);
}

static void Use_Flashlight(edict_t *ent, gitem_t *inv)
{
	P_ToggleFlashlight(ent, !(ent->flags & FL_FLASHLIGHT));
}

constexpr size_t MAX_TEMP_POI_POINTS = 128;

void Compass_Update(edict_t *ent, bool first)
{
	vec3_t *&points = level.poi_points[ent->s.number - 1];

	// deleted for some reason
	if (!points)
		return;

	if (!ent->client->help_draw_points)
		return;
	if (ent->client->help_draw_time >= level.time)
		return;

	// don't draw too many points
	float distance = (points[ent->client->help_draw_index] - ent->s.origin).length();
	if (distance > 4096 ||
		!gi.inPHS(ent->s.origin, points[ent->client->help_draw_index], false))
	{
		ent->client->help_draw_points = false;
		return;
	}

	gi.WriteByte(svc_help_path);
	gi.WriteByte(first ? 1 : 0);
	gi.WritePosition(points[ent->client->help_draw_index]);
	
	if (ent->client->help_draw_index == ent->client->help_draw_count - 1)
		gi.WriteDir((ent->client->help_poi_location - points[ent->client->help_draw_index]).normalized());
	else
		gi.WriteDir((points[ent->client->help_draw_index + 1] - points[ent->client->help_draw_index]).normalized());
	gi.unicast(ent, false);

	P_SendLevelPOI(ent);

	gi.local_sound(ent, points[ent->client->help_draw_index], world, CHAN_AUTO, gi.soundindex("misc/help_marker.wav"), 1.0f, ATTN_NORM, 0.0f, GetUnicastKey());

	// done
	if (ent->client->help_draw_index == ent->client->help_draw_count - 1)
	{
		ent->client->help_draw_points = false;
		return;
	}

	ent->client->help_draw_index++;
	ent->client->help_draw_time = level.time + 200_ms;
}

static void Use_Compass(edict_t *ent, gitem_t *inv)
{
	if (!level.valid_poi)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "$no_valid_poi");
		return;
	}

	if (level.current_dynamic_poi)
		level.current_dynamic_poi->use(level.current_dynamic_poi, ent, ent);
	
	ent->client->help_poi_location = level.current_poi;
	ent->client->help_poi_image = level.current_poi_image;

	vec3_t *&points = level.poi_points[ent->s.number - 1];

	if (!points)
		points = (vec3_t *) gi.TagMalloc(sizeof(vec3_t) * (MAX_TEMP_POI_POINTS + 1), TAG_LEVEL);

	PathRequest request;
	request.start = ent->s.origin;
	request.goal = level.current_poi;
	request.moveDist = 64.f;
	request.pathFlags = PathFlags::All;
	request.nodeSearch.ignoreNodeFlags = true;
	request.nodeSearch.minHeight = 128.0f;
	request.nodeSearch.maxHeight = 128.0f;
	request.nodeSearch.radius = 1024.0f;
	request.pathPoints.array = points + 1;
	request.pathPoints.count = MAX_TEMP_POI_POINTS;

	PathInfo info;

	if (gi.GetPathToGoal(request, info))
	{
		// TODO: optimize points?
		ent->client->help_draw_points = true;
		ent->client->help_draw_count = min((size_t)info.numPathPoints, MAX_TEMP_POI_POINTS);
		ent->client->help_draw_index = 1;

		// remove points too close to the player so they don't have to backtrack
		for (int i = 1; i < 1 + ent->client->help_draw_count; i++)
		{
			float distance = (points[i] - ent->s.origin).length();
			if (distance > 192)
			{
				break;
			}

			ent->client->help_draw_index = i;
		}

		// create an extra point in front of us if we're facing away from the first real point
		float d = ((*(points + ent->client->help_draw_index)) - ent->s.origin).normalized().dot(ent->client->v_forward);

		if (d < 0.3f)
		{
			vec3_t p = ent->s.origin + (ent->client->v_forward * 64.f);

			trace_t tr = gi.traceline(ent->s.origin + vec3_t{0.f, 0.f, (float) ent->viewheight}, p, nullptr, MASK_SOLID);

			ent->client->help_draw_index--;
			ent->client->help_draw_count++;

			if (tr.fraction < 1.0f)
				tr.endpos += tr.plane.normal * 8.f;

			*(points + ent->client->help_draw_index) = tr.endpos;
		}

		ent->client->help_draw_time = 0_ms;
		Compass_Update(ent, true);
	}
	else
	{
		P_SendLevelPOI(ent);
		gi.local_sound(ent, CHAN_AUTO, gi.soundindex("misc/help_marker.wav"), 1.f, ATTN_NORM, 0, GetUnicastKey());
	}
}

//======================================================================
// Action Add
//======================================================================

// time too wait between failures to respawn?
#define SPEC_RESPAWN_TIME       60
// time before they will get respawned
#define SPEC_TECH_TIMEOUT       60

static edict_t *FindSpecSpawn()
{
	return SelectDeathmatchSpawnPoint(false, true, true).spot;
}

THINK(SpecThink) (edict_t *special) -> void
{
	edict_t *spot;

	if ((spot = FindSpecSpawn()) != nullptr)
	{
		SpawnSpec(special->item, spot);
		G_FreeEdict(special);
	}
	else
	{
		special->nextthink = level.time + 10_ms;
		special->think = SpecThink;
	}
}

static void SpawnSpec(gitem_t * item, edict_t * spot)
{
	edict_t *ent;
	vec3_t forward, right, angles;

	ent = G_Spawn();
	ent->classname = item->classname;
	ent->item = item;
	ent->spawnflags = SPAWNFLAG_ITEM_DROPPED;
	ent->s.effects = item->world_model_flags;
	ent->s.renderfx = RF_GLOW;
	VectorSet(ent->mins, -15, -15, -15);
	VectorSet(ent->maxs, 15, 15, 15);
	// zucc dumb hack to make laser look like it is on the ground
	if (item->tag == POWERUP_LASERSIGHT) {
		VectorSet(ent->mins, -15, -15, -1);
		VectorSet(ent->maxs, 15, 15, 1);
	}
	gi.setmodel(ent, ent->item->world_model);
	ent->solid = SOLID_TRIGGER;
	ent->movetype = MOVETYPE_TOSS;
	ent->touch = Touch_Item;
	ent->owner = ent;

	angles[0] = 0;
	angles[1] = rand() % 360;
	angles[2] = 0;

	AngleVectors(angles, forward, right, nullptr);
	ent->s.origin = spot->s.origin;
	ent->s.origin[2] += 16;
	ent->velocity = forward * 100;
	ent->velocity[2] = 300;

	ent->nextthink = level.time + 60_ms;
	ent->think = SpecThink;

	gi.linkentity(ent);
}


static void MakeTouchSpecThink(edict_t * ent)
{

	ent->touch = Touch_Item;

	if (allitem->value) {
		ent->nextthink = level.time + 10_ms;
		ent->think = G_FreeEdict;
		return;
	}

	if (gameSettings & GS_ROUNDBASED) {
		ent->nextthink = level.time + 60_ms; //FIXME: should this be roundtime left
		ent->think = G_FreeEdict;
		return;
	}

	if (gameSettings & GS_WEAPONCHOOSE) {
		ent->nextthink = level.time + 60_ms;
		ent->think = G_FreeEdict;
		return;
	}

	// All else..
	ent->nextthink = level.time + 60_ms;
	ent->think = SpecThink;
}

void Drop_Spec(edict_t * ent, gitem_t * item)
{
	edict_t *spec;

	spec = Drop_Item(ent, item);
	//gi.cprintf(ent, PRINT_HIGH, "Dropping special item.\n");
	spec->nextthink = level.time + 10_ms;
	spec->think = MakeTouchSpecThink;
	//zucc this and the one below should probably be -- not = 0, if
	// a server turns on multiple item pickup.
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
}

void DeadDropSpec(edict_t * ent)
{
	gitem_t *spec;
	edict_t *dropped;
	int i, itemNum;

	i = 0;
	for (; i < q_countof(weap_ids); i++)
	{
		if (ent->client->pers.inventory[weap_ids[i]])
		{
			dropped = Drop_Item(ent, GET_ITEM(weap_ids[i]));
			// hack the velocity to make it bounce random
			dropped->velocity[0] = (rand() % 600) - 300;
			dropped->velocity[1] = (rand() % 600) - 300;
			dropped->nextthink = level.time + 10_ms;
			dropped->think = MakeTouchSpecThink;
			dropped->owner = nullptr;
			dropped->spawnflags = SPAWNFLAG_ITEM_DROPPED_PLAYER;
			ent->client->pers.inventory[weap_ids[i]] = 0;
		}
	}
}

void AddItem(edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[ITEM_INDEX (item)]++;
	ent->client->unique_item_total++;

	if (item->tag == POWERUP_LASERSIGHT)
	{
		SP_LaserSight(ent, item);	//ent->item->use(other, ent->item);
	}
	else if (item->tag == POWERUP_BANDOLIER)
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
	}
}

bool Pickup_Special (edict_t * ent, edict_t * other)
{
	if (other->client->unique_item_total >= unique_items->value)
		return false;

	// Don't allow picking up multiple of the same special item.
	if( (! allow_hoarding->value) && other->client->inventory[ITEM_INDEX(ent->item)] )
		return false;

	AddItem(other, ent->item);

	if(!(ent->spawnflags & (SPAWNFLAG_ITEM_DROPPED | SPAWNFLAG_ITEM_DROPPED_PLAYER)))
		SetRespawn (ent, 60_sec);

	return true;
}


void Drop_Special (edict_t *ent, gitem_t *item)
{
	int count;

	ent->client->unique_item_total--;
	if (item->tag == POWERUP_BANDOLIER && INV_AMMO(ent, IT_ITEM_BANDOLIER) <= 1)
	{
		if (gameSettings & GS_DEATHMATCH)
			count = 2;
		else
			count = 1;

		ent->client->max_pistolmags = count;
		if (INV_AMMO(ent, IT_AMMO_BULLETS) > count)
			INV_AMMO(ent, IT_AMMO_BULLETS) = count;

		if (!(gameSettings & GS_DEATHMATCH)) {
			if(ent->client->pers.chosenWeapon->id == IT_WEAPON_HANDCANNON)
				count = 12;
			else
				count = 7;
		} else {
			count = 14;
		}
		ent->client->max_shells = count;
		if (INV_AMMO(ent, IT_AMMO_SHELLS) > count)
			INV_AMMO(ent, IT_AMMO_SHELLS) = count;

		ent->client->max_m4mags = 1;
		if (INV_AMMO(ent, IT_AMMO_CELLS) > 1)
			INV_AMMO(ent, IT_AMMO_CELLS) = 1;

		ent->client->grenade_max = 2;
		if (INV_AMMO(ent, IT_WEAPON_GRENADES) > 2)
			INV_AMMO(ent, IT_WEAPON_GRENADES) = 2;
		}
		if (gameSettings & GS_DEATHMATCH)
			count = 2;
		else
			count = 1;
		ent->client->max_mp5mags = count;
		if (INV_AMMO(ent, IT_AMMO_ROCKETS) > count)
			INV_AMMO(ent, IT_AMMO_ROCKETS) = count;

		ent->client->knife_max = 10;
		if (INV_AMMO(ent, IT_WEAPON_KNIFE) > 10)
			INV_AMMO(ent, IT_WEAPON_KNIFE) = 10;

		if (gameSettings & GS_DEATHMATCH)
			count = 20;
		else
			count = 10;
		ent->client->max_sniper_rnds = count;
		if (INV_AMMO(ent, IT_AMMO_SLUGS) > count)
			INV_AMMO(ent, IT_AMMO_SLUGS) = count;

	Drop_Spec(ent, item);
	ValidateSelectedItem(ent);
	SP_LaserSight(ent, item);
}

// called by the "drop item" command
void DropSpecialItem (edict_t * ent)
{
	// this is the order I'd probably want to drop them in...       
	if (INV_AMMO(ent, IT_ITEM_LASERSIGHT))
		Drop_Special (ent, GetItemByIndex(IT_ITEM_LASERSIGHT));
	else if (INV_AMMO(ent, IT_ITEM_SLIPPERS))
		Drop_Special (ent, GetItemByIndex(IT_ITEM_SLIPPERS));
	else if (INV_AMMO(ent, IT_ITEM_SLIPPERS))
		Drop_Special (ent, GetItemByIndex(IT_ITEM_SLIPPERS));
	else if (INV_AMMO(ent, IT_ITEM_BANDOLIER))
		Drop_Special (ent, GetItemByIndex(IT_ITEM_BANDOLIER));
	else if (INV_AMMO(ent, IT_ITEM_HELM))
		Drop_Special (ent, GetItemByIndex(IT_ITEM_HELM));
	else if (INV_AMMO(ent, IT_ITEM_VEST))
		Drop_Special (ent, GetItemByIndex(IT_ITEM_VEST));
}

//======================================================================
// Action Add End
//======================================================================

// clang-format off
gitem_t	itemlist[] = 
{
	{ },	// leave index 0 alone

	//
	// WEAPONS 
	//


/* weapon_blaster (.3 .3 1) (-16 -16 -16) (16 16 16)
always owned, never in the world
*/
	{
		/* id */ IT_WEAPON_MK23,
		/* classname */ "weapon_Mk23", 
		/* pickup */ Pickup_Weapon,
		/* use */ Use_Weapon,
		/* drop */ Drop_Weapon,
		/* weaponthink */ Weapon_MK23,
		/* pickup_sound */ nullptr,
		/* world_model */ "models/weapons/g_dual/tris.md2",
		/* world_model_flags */ EF_ROTATE | EF_BOB,
		/* view_model */ "models/weapons/v_blast/tris.md2",
		/* icon */ "w_mk23",
		/* use_name */  MK23_NAME,
		/* pickup_name */  MK23_NAME,
		/* pickup_name_definite */ MK23_NAME,
		/* quantity */ 0,
		/* ammo */ IT_AMMO_BULLETS,
		/* chain */ IT_WEAPON_MK23,
		/* flags */ IF_WEAPON | IF_STAY_COOP | IF_NOT_RANDOM,
		/* vwep_model */ "#w_blaster.md2",
		/* armor_info */ nullptr,
		/* tag */ 0,
		/* precaches */ "weapons/mk23fire.wav weapons/mk23in.wav weapons/mk23out.wav weapons/mk23slap.wav weapons/mk23slide.wav misc/click.wav weapons/machgf4b.wav weapons/blastf1a.wav",

	},
	{
		/* id */ IT_WEAPON_MP5,
		/* classname */ "weapon_Mk23", 
		/* pickup */ Pickup_Weapon,
		/* use */ Use_Weapon,
		/* drop */ Drop_Weapon,
		/* weaponthink */ Weapon_MP5,
		/* pickup_sound */ nullptr,
		/* world_model */ "models/weapons/g_machn/tris.md2",
		/* world_model_flags */ EF_ROTATE | EF_BOB,
		/* view_model */ "models/weapons/v_machn/tris.md2",
		/* icon */ "w_mp5",
		/* use_name */  MP5_NAME,
		/* pickup_name */  MP5_NAME,
		/* pickup_name_definite */ MP5_NAME,
		/* quantity */ 0,
		/* ammo */ IT_AMMO_ROCKETS,
		/* chain */ IT_WEAPON_MP5,
		/* flags */ IF_WEAPON | IF_STAY_COOP | IF_NOT_RANDOM,
		/* vwep_model */ "#w_mp5.md2",
		/* armor_info */ nullptr,
		/* tag */ 0,
		/* precaches */    "weapons/mp5fire1.wav weapons/mp5in.wav weapons/mp5out.wav weapons/mp5slap.wav weapons/mp5slide.wav weapons/machgf1b.wav weapons/machgf2b.wav weapons/machgf3b.wav weapons/machgf5b.wav",


	},
	{
		/* id */ IT_WEAPON_M4,
		/* classname */ "weapon_Mk23", 
		/* pickup */ Pickup_Weapon,
		/* use */ Use_Weapon,
		/* drop */ Drop_Weapon,
		/* weaponthink */ Weapon_M4,
		/* pickup_sound */ nullptr,
		/* world_model */ "models/weapons/g_m4/tris.md2",
		/* world_model_flags */ EF_ROTATE | EF_BOB,
		/* view_model */ "models/weapons/v_m4/tris.md2",
		/* icon */ "w_m4",
		/* use_name */  M4_NAME,
		/* pickup_name */  M4_NAME,
		/* pickup_name_definite */ M4_NAME,
		/* quantity */ 0,
		/* ammo */ IT_AMMO_CELLS,
		/* chain */ IT_WEAPON_M4,
		/* flags */ IF_WEAPON | IF_STAY_COOP | IF_NOT_RANDOM,
		/* vwep_model */ "#w_m4.md2",
		/* armor_info */ nullptr,
		/* tag */ 0,
		/* precaches */ "weapons/m4a1fire.wav weapons/m4a1in.wav weapons/m4a1out.wav weapons/m4a1slide.wav weapons/rocklf1a.wav weapons/rocklr1b.wav",
	},
	{
		/* id */ IT_WEAPON_M3,
		/* classname */ "weapon_Mk23", 
		/* pickup */ Pickup_Weapon,
		/* use */ Use_Weapon,
		/* drop */ Drop_Weapon,
		/* weaponthink */ Weapon_M3,
		/* pickup_sound */ nullptr,
		/* world_model */ "models/weapons/g_shotg/tris.md2",
		/* world_model_flags */ EF_ROTATE | EF_BOB,
		/* view_model */ "models/weapons/v_shotg/tris.md2",
		/* icon */ "w_super90",
		/* use_name */  M3_NAME,
		/* pickup_name */  M3_NAME,
		/* pickup_name_definite */ M3_NAME,
		/* quantity */ 0,
		/* ammo */ IT_AMMO_SHELLS,
		/* chain */ IT_WEAPON_M3,
		/* flags */ IF_WEAPON | IF_STAY_COOP | IF_NOT_RANDOM,
		/* vwep_model */ "#w_super90.md2",
		/* armor_info */ nullptr,
		/* tag */ 0,
		/* precaches */ "weapons/m3in.wav weapons/shotgr1b.wav weapons/shotgf1b.wav",
	},
	{
		/* id */ IT_WEAPON_HANDCANNON,
		/* classname */ "weapon_Mk23", 
		/* pickup */ Pickup_Weapon,
		/* use */ Use_Weapon,
		/* drop */ Drop_Weapon,
		/* weaponthink */ Weapon_HC,
		/* pickup_sound */ nullptr,
		/* world_model */ "models/weapons/g_cannon/tris.md2",
		/* world_model_flags */ EF_ROTATE | EF_BOB,
		/* view_model */ "models/weapons/v_cannon/tris.md2",
		/* icon */ "w_cannon",
		/* use_name */  HC_NAME,
		/* pickup_name */  HC_NAME,
		/* pickup_name_definite */ HC_NAME,
		/* quantity */ 0,
		/* ammo */ IT_AMMO_SHELLS,
		/* chain */ IT_WEAPON_HANDCANNON,
		/* flags */ IF_WEAPON | IF_STAY_COOP | IF_NOT_RANDOM,
		/* vwep_model */ "#w_cannon.md2",
		/* armor_info */ nullptr,
		/* tag */ 0,
		/* precaches */ "weapons/cannon_fire.wav weapons/sshotf1b.wav weapons/cclose.wav weapons/cin.wav weapons/cout.wav weapons/copen.wav",
	},
	{
		/* id */ IT_WEAPON_SNIPER,
		/* classname */ "weapon_Sniper", 
		/* pickup */ Pickup_Weapon,
		/* use */ Use_Weapon,
		/* drop */ Drop_Weapon,
		/* weaponthink */ Weapon_Sniper,
		/* pickup_sound */ nullptr,
		/* world_model */ "models/weapons/g_sniper/tris.md2",
		/* world_model_flags */ EF_ROTATE | EF_BOB,
		/* view_model */ "models/weapons/v_sniper/tris.md2",
		/* icon */ "w_sniper",
		/* use_name */  SNIPER_NAME,
		/* pickup_name */  SNIPER_NAME,
		/* pickup_name_definite */ SNIPER_NAME,
		/* quantity */ 0,
		/* ammo */ IT_AMMO_SLUGS,
		/* chain */ IT_WEAPON_SNIPER,
		/* flags */ IF_WEAPON | IF_STAY_COOP | IF_NOT_RANDOM,
		/* vwep_model */ "#w_sniper.md2",
		/* armor_info */ nullptr,
		/* tag */ 0,
		/* precaches */ "weapons/ssgbolt.wav weapons/ssgfire.wav weapons/ssgin.wav misc/lensflik.wav weapons/hyprbl1a.wav weapons/hyprbf1a.wav",
	},
	{
		/* id */ IT_WEAPON_DUALMK23,
		/* classname */ "weapon_Dual", 
		/* pickup */ Pickup_Weapon,
		/* use */ Use_Weapon,
		/* drop */ Drop_Weapon,
		/* weaponthink */ Weapon_Dual,
		/* pickup_sound */ nullptr,
		/* world_model */ "models/weapons/g_dual/tris.md2",
		/* world_model_flags */ EF_ROTATE | EF_BOB,
		/* view_model */ "models/weapons/v_dual/tris.md2",
		/* icon */ "w_akimbo",
		/* use_name */  DUAL_NAME,
		/* pickup_name */  DUAL_NAME,
		/* pickup_name_definite */ DUAL_NAME,
		/* quantity */ 0,
		/* ammo */ IT_AMMO_BULLETS,
		/* chain */ IT_WEAPON_DUALMK23,
		/* flags */ IF_WEAPON | IF_STAY_COOP | IF_NOT_RANDOM,
		/* vwep_model */ "#w_akimbo.md2",
		/* armor_info */ nullptr,
		/* tag */ 0,
		/* precaches */ "weapons/ssgbolt.wav weapons/ssgfire.wav weapons/ssgin.wav misc/lensflik.wav weapons/hyprbl1a.wav weapons/hyprbf1a.wav",
	},
	{
		/* id */ IT_WEAPON_KNIFE,
		/* classname */ "weapon_Knife", 
		/* pickup */ Pickup_Weapon,
		/* use */ Use_Weapon,
		/* drop */ Drop_Weapon,
		/* weaponthink */ Weapon_Knife,
		/* pickup_sound */ nullptr,
		/* world_model */ "models/weapons/knife/tris.md2",
		/* world_model_flags */ EF_ROTATE | EF_BOB,
		/* view_model */ "models/weapons/v_knife/tris.md2",
		/* icon */ "w_knife",
		/* use_name */  KNIFE_NAME,
		/* pickup_name */  KNIFE_NAME,
		/* pickup_name_definite */ KNIFE_NAME,
		/* quantity */ 0,
		/* ammo */ IT_NULL,
		/* chain */ IT_WEAPON_DUALMK23,
		/* flags */ IF_WEAPON | IF_STAY_COOP | IF_NOT_RANDOM,
		/* vwep_model */ "#w_knife.md2",
		/* armor_info */ nullptr,
		/* tag */ 0,
		/* precaches */ "weapons/ssgbolt.wav weapons/ssgfire.wav weapons/ssgin.wav misc/lensflik.wav weapons/hyprbl1a.wav weapons/hyprbf1a.wav",
	},
	{
		/* id */ IT_WEAPON_GRENADES,
		/* classname */ "weapon_Grenade", 
		/* pickup */ Pickup_Weapon,
		/* use */ Use_Weapon,
		/* drop */ Drop_Weapon,
		/* weaponthink */ Weapon_Gas,
		/* pickup_sound */ nullptr,
		/* world_model */ "models/weapons/grenade2/tris.md2",
		/* world_model_flags */ EF_ROTATE | EF_BOB,
		/* view_model */ "models/weapons/v_handgr/tris.md2",
		/* icon */ "a_m61frag",
		/* use_name */  GRENADE_NAME,
		/* pickup_name */  GRENADE_NAME,
		/* pickup_name_definite */ GRENADE_NAME,
		/* quantity */ 0,
		/* ammo */ IT_NULL,
		/* chain */ IT_WEAPON_DUALMK23,
		/* flags */ IF_WEAPON | IF_STAY_COOP | IF_NOT_RANDOM,
		/* vwep_model */ "#a_m61frag.md2",
		/* armor_info */ nullptr,
		/* tag */ 0,
		/* precaches */ "misc/grenade.wav weapons/grenlb1b.wav weapons/hgrent1a.wav",
	},
/* weapon_grapple (.3 .3 1) (-16 -16 -16) (16 16 16)
always owned, never in the world
*/
	{
		/* id */ IT_WEAPON_GRAPPLE,
		/* classname */ "weapon_grapple", 
		/* pickup */ nullptr,
		/* use */ Use_Weapon,
		/* drop */ nullptr,
		/* weaponthink */ CTFWeapon_Grapple,
		/* pickup_sound */ nullptr,
		/* world_model */ nullptr,
		/* world_model_flags */ EF_NONE,
		/* view_model */ "models/weapons/grapple/tris.md2",
		/* icon */ "w_grapple",
		/* use_name */  "Grapple",
		/* pickup_name */  "$item_grapple",
		/* pickup_name_definite */ "$item_grapple_def",
		/* quantity */ 0,
		/* ammo */ IT_NULL,
		/* chain */ IT_WEAPON_MK23,
		/* flags */ IF_WEAPON | IF_NO_HASTE | IF_POWERUP_WHEEL | IF_NOT_RANDOM,
		/* vwep_model */ "#w_grapple.md2",
		/* armor_info */ nullptr,
		/* tag */ 0,
		/* precaches */ "weapons/grapple/grfire.wav weapons/grapple/grpull.wav weapons/grapple/grhang.wav weapons/grapple/grreset.wav weapons/grapple/grhit.wav weapons/grapple/grfly.wav"
	},


	//
	// AMMO ITEMS
	//

/*QUAKED ammo_bullets (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		/* id */ IT_AMMO_BULLETS,
		/* classname */ "ammo_clip",
		/* pickup */ Pickup_Ammo,
		/* use */ nullptr,
		/* drop */ Drop_Ammo,
		/* weaponthink */ nullptr,
		/* pickup_sound */ nullptr,
		/* world_model */ "models/items/ammo/clip/tris.md2",
		/* world_model_flags */ EF_NONE,
		/* view_model */ nullptr,
		/* icon */ "a_clip",
		/* use_name */  MK23_AMMO_NAME,
		/* pickup_name */  "$item_bullets",
		/* pickup_name_definite */ "$item_bullets_def",
		/* quantity */ 50,
		/* ammo */ IT_NULL,
		/* chain */ IT_NULL,
		/* flags */ IF_AMMO,
		/* vwep_model */ nullptr,
		/* armor_info */ nullptr,
		/* tag */ AMMO_BULLETS
	},

/*QUAKED ammo_rockets (.3 .3 1) (-16 -16 -16) (16 16 16)
model="models/items/ammo/rockets/medium/tris.md2"
*/
	{
		/* id */ IT_AMMO_ROCKETS,
		/* classname */ "ammo_mag",
		/* pickup */ Pickup_Ammo,
		/* use */ nullptr,
		/* drop */ Drop_Ammo,
		/* weaponthink */ nullptr,
		/* pickup_sound */ nullptr,
		/* world_model */ "models/items/ammo/mag/tris.md2",
		/* world_model_flags */ EF_NONE,
		/* view_model */ nullptr,
		/* icon */ "a_mag",
		/* use_name */  MP5_AMMO_NAME,
		/* pickup_name */  "$item_rockets",
		/* pickup_name_definite */ "$item_rockets_def",
		/* quantity */ 5,
		/* ammo */ IT_NULL,
		/* chain */ IT_NULL,
		/* flags */ IF_AMMO,
		/* vwep_model */ nullptr,
		/* armor_info */ nullptr,
		/* tag */ AMMO_ROCKETS
	},


/*QUAKED ammo_cells (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		/* id */ IT_AMMO_CELLS,
		/* classname */ "ammo_m4",
		/* pickup */ Pickup_Ammo,
		/* use */ nullptr,
		/* drop */ Drop_Ammo,
		/* weaponthink */ nullptr,
		/* pickup_sound */ nullptr,
		/* world_model */ "models/items/ammo/m4/tris.md2",
		/* world_model_flags */ EF_NONE,
		/* view_model */ nullptr,
		/* icon */ "a_m4",
		/* use_name */  M4_AMMO_NAME,
		/* pickup_name */  "$item_cells",
		/* pickup_name_definite */ "$item_cells_def",
		/* quantity */ 50,
		/* ammo */ IT_NULL,
		/* chain */ IT_NULL,
		/* flags */ IF_AMMO,
		/* vwep_model */ nullptr,
		/* armor_info */ nullptr,
		/* tag */ AMMO_CELLS
	},

/*QUAKED ammo_shells (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		/* id */ IT_AMMO_SHELLS,
		/* classname */ "ammo_m3",
		/* pickup */ Pickup_Ammo,
		/* use */ nullptr,
		/* drop */ Drop_Ammo,
		/* weaponthink */ nullptr,
		/* pickup_sound */ nullptr,
		/* world_model */ "models/items/ammo/shells/medium/tris.md2",
		/* world_model_flags */ EF_NONE,
		/* view_model */ nullptr,
		/* icon */ "a_shells",
		/* use_name */  SHOTGUN_AMMO_NAME,
		/* pickup_name */  "$item_shells",
		/* pickup_name_definite */ "$item_shells_def",
		/* quantity */ 7,
		/* ammo */ IT_NULL,
		/* chain */ IT_NULL,
		/* flags */ IF_AMMO,
		/* vwep_model */ nullptr,
		/* armor_info */ nullptr,
		/* tag */ AMMO_SHELLS
	},

/*QUAKED ammo_slugs (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		/* id */ IT_AMMO_SLUGS,
		/* classname */ "ammo_sniper",
		/* pickup */ Pickup_Ammo,
		/* use */ nullptr,
		/* drop */ Drop_Ammo,
		/* weaponthink */ nullptr,
		/* pickup_sound */ nullptr,
		/* world_model */ "models/items/ammo/sniper/tris.md2",
		/* world_model_flags */ EF_NONE,
		/* view_model */ nullptr,
		/* icon */ "a_sniper",
		/* use_name */  SNIPER_AMMO_NAME,
		/* pickup_name */  SNIPER_AMMO_NAME,
		/* pickup_name_definite */ "$item_slugs_def",
		/* quantity */ 10,
		/* ammo */ IT_NULL,
		/* chain */ IT_NULL,
		/* flags */ IF_AMMO,
		/* vwep_model */ nullptr,
		/* armor_info */ nullptr,
		/* tag */ AMMO_SLUGS
	},


	//
	// POWERUP ITEMS
	//

/*QUAKED item_quiet (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		/* id */ IT_ITEM_QUIET,
		/* classname */ "item_quiet", 
		/* pickup */ Pickup_Special,
		/* use */ nullptr,
		/* drop */ Drop_Special,
		/* weaponthink */ nullptr,
		/* pickup_sound */ "misc/screw.wav",
		/* world_model */ "models/items/quiet/tris.md2",
		/* world_model_flags */ EF_NONE,
		/* view_model */ nullptr,
		/* icon */ "p_silencer",
		/* use_name */  SIL_NAME,
		/* pickup_name */  "$item_quiet",
		/* pickup_name_definite */ "$item_quiet_def",
		/* quantity */ 1,
		/* ammo */ IT_NULL,
		/* chain */ IT_NULL,
		/* flags */ IF_POWERUP,
		/* vwep_model */ nullptr,
		/* armor_info */ nullptr,
		/* tag */ POWERUP_QUIET,
		/* precaches */ ""
	},

/*QUAKED item_slippers (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		/* id */ IT_ITEM_SLIPPERS,
		/* classname */ "item_slippers", 
		/* pickup */ Pickup_Special,
		/* use */ nullptr,
		/* drop */ Drop_Special,
		/* weaponthink */ nullptr,
		/* pickup_sound */ "misc/veston.wav",
		/* world_model */ "models/items/slippers/tris.md2",
		/* world_model_flags */ EF_NONE,
		/* view_model */ nullptr,
		/* icon */ "slippers",
		/* use_name */  SLIP_NAME,
		/* pickup_name */  "$item_slippers",
		/* pickup_name_definite */ "$item_slippers_def",
		/* quantity */ 1,
		/* ammo */ IT_NULL,
		/* chain */ IT_NULL,
		/* flags */ IF_POWERUP,
		/* vwep_model */ nullptr,
		/* armor_info */ nullptr,
		/* tag */ POWERUP_SLIPPERS,
		/* precaches */ ""
	},
/*QUAKED item_band (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		/* id */ IT_ITEM_BANDOLIER,
		/* classname */ "item_band", 
		/* pickup */ Pickup_Special,
		/* use */ nullptr,
		/* drop */ Drop_Special,
		/* weaponthink */ nullptr,
		/* pickup_sound */ "misc/veston.wav",
		/* world_model */ "models/items/band/tris.md2",
		/* world_model_flags */ EF_NONE,
		/* view_model */ nullptr,
		/* icon */ "p_bandolier",
		/* use_name */  BAND_NAME,
		/* pickup_name */  BAND_NAME,
		/* pickup_name_definite */ BAND_NAME,
		/* quantity */ 1,
		/* ammo */ IT_NULL,
		/* chain */ IT_NULL,
		/* flags */ IF_POWERUP,
		/* vwep_model */ nullptr,
		/* armor_info */ nullptr,
		/* tag */ POWERUP_BANDOLIER,
		/* precaches */ ""
	},
/*QUAKED item_vest (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		/* id */ IT_ITEM_VEST,
		/* classname */ "item_vest", 
		/* pickup */ Pickup_Special,
		/* use */ nullptr,
		/* drop */ Drop_Special,
		/* weaponthink */ nullptr,
		/* pickup_sound */ "misc/veston.wav",
		/* world_model */ "models/items/armor/jacket/tris.md2",
		/* world_model_flags */ EF_NONE,
		/* view_model */ nullptr,
		/* icon */ "i_jacketarmor",
		/* use_name */  KEV_NAME,
		/* pickup_name */  "$item_vest",
		/* pickup_name_definite */ "$item_vest_def",
		/* quantity */ 1,
		/* ammo */ IT_NULL,
		/* chain */ IT_NULL,
		/* flags */ IF_POWERUP,
		/* vwep_model */ nullptr,
		/* armor_info */ nullptr,
		/* tag */ POWERUP_VEST,
		/* precaches */ ""
	},

/*QUAKED item_lasersight (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		/* id */ IT_ITEM_LASERSIGHT,
		/* classname */ "item_lasersight", 
		/* pickup */ Pickup_Special,
		/* use */ nullptr,
		/* drop */ Drop_Special,
		/* weaponthink */ nullptr,
		/* pickup_sound */ "misc/lasersight.wav",
		/* world_model */ "models/items/laser/tris.md2",
		/* world_model_flags */ EF_NONE,
		/* view_model */ nullptr,
		/* icon */ "p_laser",
		/* use_name */  LASER_NAME,
		/* pickup_name */  LASER_NAME,
		/* pickup_name_definite */ LASER_NAME,
		/* quantity */ 1,
		/* ammo */ IT_NULL,
		/* chain */ IT_NULL,
		/* flags */ IF_POWERUP,
		/* vwep_model */ nullptr,
		/* armor_info */ nullptr,
		/* tag */ POWERUP_LASERSIGHT,
		/* precaches */ ""
	},

/*QUAKED item_helmet (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		/* id */ IT_ITEM_HELM,
		/* classname */ "item_helmet", 
		/* pickup */ Pickup_Special,
		/* use */ nullptr,
		/* drop */ Drop_Special,
		/* weaponthink */ nullptr,
		/* pickup_sound */ "misc/veston.wav",
		/* world_model */ "models/items/breather/tris.md2",
		/* world_model_flags */ EF_NONE,
		/* view_model */ nullptr,
		/* icon */ "p_laser",
		/* use_name */  HELM_NAME,
		/* pickup_name */  HELM_NAME,
		/* pickup_name_definite */ HELM_NAME,
		/* quantity */ 1,
		/* ammo */ IT_NULL,
		/* chain */ IT_NULL,
		/* flags */ IF_POWERUP,
		/* vwep_model */ nullptr,
		/* armor_info */ nullptr,
		/* tag */ POWERUP_HELM,
		/* precaches */ ""
	},

/*QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16)
model="models/items/healing/medium/tris.md2"
*/
	{
		/* id */ IT_HEALTH_MEDIUM,
		/* classname */ "item_health",
		/* pickup */ Pickup_Health,
		/* use */ nullptr,
		/* drop */ nullptr,
		/* weaponthink */ nullptr,
		/* pickup_sound */ "items/n_health.wav",
		/* world_model */ "models/items/healing/medium/tris.md2",
		/* world_model_flags */ EF_NONE,
		/* view_model */ nullptr,
		/* icon */ "i_health",
		/* use_name */  "Health",
		/* pickup_name */  "$item_medium_medkit",
		/* pickup_name_definite */ "$item_medium_medkit_def",
		/* quantity */ 10,
		/* ammo */ IT_NULL,
		/* chain */ IT_NULL,
		/* flags */ IF_HEALTH
	},
	{
		/* id */ IT_FLAG1,
		/* classname */ "item_flag_team1",
		/* pickup */ CTFPickup_Flag,
		/* use */ nullptr,
		/* drop */ CTFDrop_Flag, //Should this be null if we don't want players to drop it manually?
		/* weaponthink */ nullptr,
		/* pickup_sound */ "ctf/flagtk.wav",
		/* world_model */ "players/male/flag1.md2",
		/* world_model_flags */ EF_FLAG1,
		/* view_model */ nullptr,
		/* icon */ "i_ctf1",
		/* use_name */  "Red Flag",
		/* pickup_name */  "$item_red_flag",
		/* pickup_name_definite */ "$item_red_flag_def",
		/* quantity */ 0,
		/* ammo */ IT_NULL,
		/* chain */ IT_NULL,
		/* flags */ IF_NONE,
		/* vwep_model */ nullptr,
		/* armor_info */ nullptr,
		/* tag */ 0,
		/* precaches */ "ctf/flagcap.wav"
	},

/*QUAKED item_flag_team2 (1 0.2 0) (-16 -16 -24) (16 16 32)
*/
	{
		/* id */ IT_FLAG2,
		/* classname */ "item_flag_team2",
		/* pickup */ CTFPickup_Flag,
		/* use */ nullptr,
		/* drop */ CTFDrop_Flag,
		/* weaponthink */ nullptr,
		/* pickup_sound */ "ctf/flagtk.wav",
		/* world_model */ "players/male/flag2.md2",
		/* world_model_flags */ EF_FLAG2,
		/* view_model */ nullptr,
		/* icon */ "i_ctf2",
		/* use_name */  "Blue Flag",
		/* pickup_name */  "$item_blue_flag",
		/* pickup_name_definite */ "$item_blue_flag_def",
		/* quantity */ 0,
		/* ammo */ IT_NULL,
		/* chain */ IT_NULL,
		/* flags */ IF_NONE,
		/* vwep_model */ nullptr,
		/* armor_info */ nullptr,
		/* tag */ 0,
		/* precaches */ "ctf/flagcap.wav"
	},
};
// clang-format on

void InitItems()
{
	// validate item integrity
	for (item_id_t i = IT_NULL; i < IT_TOTAL; i = static_cast<item_id_t>(i + 1))
		if (itemlist[i].id != i)
			gi.Com_ErrorFmt("Item {} has wrong enum ID {} (should be {})", itemlist[i].pickup_name, (int32_t) itemlist[i].id, (int32_t) i);

	// set up weapon chains
	for (item_id_t i = IT_NULL; i < IT_TOTAL; i = static_cast<item_id_t>(i + 1))
	{
		if (!itemlist[i].chain)
			continue;

		gitem_t *item = &itemlist[i];

		// already initialized
		if (item->chain_next)
			continue;

		gitem_t *chain_item = &itemlist[item->chain];

		if (!chain_item)
			gi.Com_ErrorFmt("Invalid item chain {} for {}", (int32_t) item->chain, item->pickup_name);

		// set up initial chain
		if (!chain_item->chain_next)
			chain_item->chain_next = chain_item;

		// if we're not the first in chain, add us now
		if (chain_item != item)
		{
			gitem_t *c;

			// end of chain is one whose chain_next points to chain_item
			for (c = chain_item; c->chain_next != chain_item; c = c->chain_next)
				continue;

			// splice us in
			item->chain_next = chain_item;
			c->chain_next = item;
		}
	}

	// set up ammo
	for (auto &it : itemlist)
	{
		if ((it.flags & IF_AMMO) && it.tag >= AMMO_BULLETS && it.tag < AMMO_MAX)
			ammolist[it.tag] = &it;
		else if ((it.flags & IF_POWERUP_WHEEL) && !(it.flags & IF_WEAPON) && it.tag < POWERUP_MAX)
			poweruplist[it.tag] = &it;
	}

	// in coop or DM with Weapons' Stay, remove drop ptr
	for (auto &it : itemlist)
	{
		if (coop->integer)
		{
			if (!P_UseCoopInstancedItems() && (it.flags & IF_STAY_COOP))
				it.drop = nullptr;
		}
		else if (deathmatch->integer)
		{
			if (g_dm_weapons_stay->integer && it.drop == Drop_Weapon)
				it.drop = nullptr;
		}
	}
}

/*
===============
SetItemNames

Called by worldspawn
===============
*/
void SetItemNames()
{
	for (item_id_t i = IT_NULL; i < IT_TOTAL; i = static_cast<item_id_t>(i + 1))
		gi.configstring(CS_ITEMS + i, itemlist[i].pickup_name);

	// [Paril-KEX] set ammo wheel indices first
	int32_t cs_index = 0;

	for (item_id_t i = IT_NULL; i < IT_TOTAL; i = static_cast<item_id_t>(i + 1))
	{
		if (!(itemlist[i].flags & IF_AMMO))
			continue;

		if (cs_index >= MAX_WHEEL_ITEMS)
			gi.Com_Error("out of wheel indices");

		gi.configstring(CS_WHEEL_AMMO + cs_index, G_Fmt("{}|{}", (int32_t) i, gi.imageindex(itemlist[i].icon)).data());
		itemlist[i].ammo_wheel_index = cs_index;
		cs_index++;
	}

	// set weapon wheel indices
	cs_index = 0;

	for (item_id_t i = IT_NULL; i < IT_TOTAL; i = static_cast<item_id_t>(i + 1))
	{
		if (!(itemlist[i].flags & IF_WEAPON))
			continue;

		if (cs_index >= MAX_WHEEL_ITEMS)
			gi.Com_Error("out of wheel indices");

		int32_t min_ammo = (itemlist[i].flags & IF_AMMO) ? 1 : itemlist[i].quantity;

		gi.configstring(CS_WHEEL_WEAPONS + cs_index, G_Fmt("{}|{}|{}|{}|{}|{}|{}|{}",
			(int32_t) i,
			gi.imageindex(itemlist[i].icon),
			itemlist[i].ammo ? GetItemByIndex(itemlist[i].ammo)->ammo_wheel_index : -1,
			min_ammo,
			(itemlist[i].flags & IF_POWERUP_WHEEL) ? 1 : 0,
			itemlist[i].sort_id,
			itemlist[i].quantity_warn,
			itemlist[i].drop != nullptr ? 1 : 0
		).data());
		itemlist[i].weapon_wheel_index = cs_index;
		cs_index++;
	}

	// set powerup wheel indices
	cs_index = 0;

	for (item_id_t i = IT_NULL; i < IT_TOTAL; i = static_cast<item_id_t>(i + 1))
	{
		if (!(itemlist[i].flags & IF_POWERUP_WHEEL) || (itemlist[i].flags & IF_WEAPON))
			continue;

		if (cs_index >= MAX_WHEEL_ITEMS)
			gi.Com_Error("out of wheel indices");

		gi.configstring(CS_WHEEL_POWERUPS + cs_index, G_Fmt("{}|{}|{}|{}|{}|{}",
			(int32_t) i,
			gi.imageindex(itemlist[i].icon),
			(itemlist[i].flags & IF_POWERUP_ONOFF) ? 1 : 0,
			itemlist[i].sort_id,
			itemlist[i].drop != nullptr ? 1 : 0,
			itemlist[i].ammo ? GetItemByIndex(itemlist[i].ammo)->ammo_wheel_index : -1
		).data());
		itemlist[i].powerup_wheel_index = cs_index;
		cs_index++;
	}
}

#define ITEM_SWITCH_COUNT 15

const char* sp_item[ITEM_SWITCH_COUNT][2] = {
  {"weapon_machinegun", "weapon_MP5"},
  //{"weapon_supershotgun","weapon_HC"},
  {"weapon_bfg", "weapon_M4"},
  {"weapon_shotgun", "weapon_M3"},
  //{"weapon_grenadelauncher","weapon_M203"},
  {"weapon_chaingun", "weapon_Sniper"},
  {"weapon_rocketlauncher", "weapon_HC"},
  {"weapon_railgun", "weapon_Dual"},
  {"ammo_bullets", "ammo_clip"},
  {"ammo_rockets", "ammo_mag"},
  {"ammo_cells", "ammo_m4"},
  {"ammo_slugs", "ammo_sniper"},
  {"ammo_shells", "ammo_m3"},
  {"ammo_grenades", "weapon_Grenade"},
  {"ammo_box", "ammo_m3"},
  {"weapon_cannon", "weapon_HC"},
  {"weapon_sniper", "weapon_Sniper"}

};

void CheckItem(edict_t* ent)
{
	int i;

	for (i = 0; i < ITEM_SWITCH_COUNT; i++)
	{
		//If it's a null entry, bypass it
		if (!sp_item[i][0])
			continue;
		//Do the passed ent and our list match?
		if (strcmp(ent->classname, sp_item[i][0]) == 0)
		{
			//Yep. Replace the Q2 entity with our own.
			ent->classname = sp_item[i][1];
			return;
		}
	}
}