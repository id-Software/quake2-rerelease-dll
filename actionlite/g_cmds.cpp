// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"
#include "m_player.h"

//======================================================================
// Action Add
//======================================================================

/*----------------------------------------
 * SP_LaserSight
 *
 * Create/remove the laser sight entity
 *---------------------------------------*/

void SP_LaserSight(edict_t * self, gitem_t * item)
{
	edict_t *lasersight = self->client->lasersight;

	if (!INV_AMMO(self, IT_ITEM_LASERSIGHT) || !self->client->pers.weapon) {
		if (lasersight) {  // laser is on
			G_FreeEdict(lasersight);
			self->client->lasersight = NULL;
		}
		return;
	}
	//zucc code to make it be used with the right weapons

	switch (self->client->pers.weapon->id) {
	case IT_WEAPON_MK23:
	case IT_WEAPON_MP5:
	case IT_WEAPON_M4:
		break;
	default:
		// laser is on but we want it off
		if (lasersight) {
			G_FreeEdict(lasersight);
			self->client->lasersight = NULL;
		}
		return;
	}

	if (lasersight) { //Lasersight is already on
		return;
	}

	lasersight = G_Spawn();
	self->client->lasersight = lasersight;
	lasersight->owner = self;
	lasersight->movetype = MOVETYPE_NOCLIP;
	lasersight->solid = SOLID_NOT;
	lasersight->classname = "lasersight";
	lasersight->s.modelindex = level.model_lsight;
	lasersight->s.renderfx = RF_TRANSLUCENT;
	lasersight->ideal_yaw = self->viewheight;
	lasersight->count = 0;
	lasersight->think = LaserSightThink;
	lasersight->nextthink = level.time + 1_ms;
	LaserSightThink( lasersight );
	VectorCopy( lasersight->s.origin, lasersight->s.old_origin );
	//VectorCopy( lasersight->s.origin, lasersight->old_origin );
}

/*---------------------------------------------
 * LaserSightThink
 *
 * Updates the sights position, angle, and shape
 * is the lasersight entity
 *-------------------------------------------*/

void LaserSightThink(edict_t * self)
{
	vec3_t start, end, endp, offset;
	vec3_t forward, right, up, angles;
	vec3_t result_start, result_dir;
	trace_t tr;
	float viewheight = self->owner->viewheight;

	// zucc compensate for weapon ride up
	VectorAdd(self->owner->client->v_angle, self->owner->client->kick_angles, angles);
	AngleVectors(angles, forward, right, up);

	if (self->owner->client->lasersight != self) {
		self->think = G_FreeEdict;
	}

	if (self->owner->client->pers.firing_style == ACTION_FIRING_CLASSIC)
		viewheight -= 8;

	//VectorSet(offset, 24, 8, viewheight);
	//P_ProjectSource(self, self->owner->s.origin, offset, forward, right, start);

	//vec3_t offset;
    VectorSet(offset, 24, 8, viewheight);
    P_ProjectSource(self->owner, angles, offset, result_start, result_dir);
    
	VectorMA(start, 8192, forward, end);

	PRETRACE();
	tr = gi.trace(start, self->mins, self->maxs, end, self->owner, CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_DEADMONSTER);
	POSTTRACE();

	if (tr.fraction != 1) {
		VectorMA(tr.endpos, -4, forward, endp);
		VectorCopy(endp, tr.endpos);
	}

	vec3_t newAngles = vectoangles(tr.endpos);
	// Copy the pitch and yaw from the newAngles result to self->s.angles
    self->s.angles[0] = newAngles[0];
    self->s.angles[1] = newAngles[1];
    self->s.angles[2] = newAngles[2];

	// vectoangles(self->s.angles);
	// VectorCopy(tr.endpos, self->s.origin);

	self->s.modelindex = (tr.surface && (tr.surface->flags & SURF_SKY)) ? level.model_null : level.model_lsight;

	gi.linkentity(self);
	self->nextthink = level.time + 1_ms;
}


// sets variable to toggle nearby door status
void Cmd_OpenDoor_f(edict_t * ent)
{
	ent->client->doortoggle = 1;
	return;
}

void Cmd_Bandage_f(edict_t *ent)
{
	if (!IS_ALIVE(ent))
		return;

	if (ent->client->bandaging) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Already bandaging\n");
		return;
	}

	bool can_use_medkit = (ent->client->medkit > 0) && (ent->health < ent->max_health);

	// No need to bandage if enhanced slippers are enabled and you only have fall damage
	// but you can still use the medkit to regain health
	if (ent->client->bleeding == 0 && e_enhancedSlippers->value && ! can_use_medkit){
		gi.LocClient_Print(ent, PRINT_HIGH, "No need to bandage\n");
		return;
	}

	if (ent->client->bleeding == 0 && ent->client->leg_damage == 0 && ! can_use_medkit) {
		gi.LocClient_Print(ent, PRINT_HIGH, "No need to bandage\n");
		return;
	}

	ent->client->reload_attempts = 0;	// prevent any further reloading

	if (ent->client->weaponstate != WEAPON_READY && ent->client->weaponstate != WEAPON_END_MAG) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Can't bandage now\n");
		return;
	}

	// zucc - check if they have a primed grenade
	if (ent->client->pers.weapon->id == IT_WEAPON_GRENADES
		&& ((ent->client->ps.gunframe >= GRENADE_IDLE_FIRST
			&& ent->client->ps.gunframe <= GRENADE_IDLE_LAST)
		|| (ent->client->ps.gunframe >= GRENADE_THROW_FIRST
			&& ent->client->ps.gunframe <= GRENADE_THROW_LAST)))
	{
		int damage;

		ent->client->ps.gunframe = 0;

		damage = GRENADE_DAMRAD;

		fire_grenade2(ent, ent->s.origin, vec3_origin, damage, 0, 80_ms, damage * 2, false);

		INV_AMMO(ent, IT_WEAPON_GRENADES)--;
		if (INV_AMMO(ent, IT_WEAPON_GRENADES) <= 0) {
			ent->client->newweapon = GetItemByIndex(IT_WEAPON_MK23);
		}
	}

	ent->client->bandaging = 1;
	ent->client->resp.sniper_mode = SNIPER_1X;
	ent->client->ps.fov = 90;
	ent->client->desired_fov = 90;
	if (ent->client->pers.weapon)
		ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);
	gi.LocClient_Print(ent, PRINT_HIGH, "You've started bandaging\n");
}

// function called in generic_weapon function that does the bandaging

void Bandage(edict_t * ent)
{
	ClientFixLegs(ent);
	ent->client->bleeding = 0;
	ent->client->bleed_remain = 0;
	ent->client->bandaging = 0;
	ent->client->attacker = NULL;
	ent->client->bandage_stopped = 1;
	ent->client->idle_weapon = BANDAGE_TIME;
}

void Cmd_New_Reload_f(edict_t * ent)
{
//FB 6/1/99 - refuse to reload during LCA
	if (lights_camera_action)
		return;
	ent->client->reload_attempts++;
}

// Handles weapon reload requests
void Cmd_Reload_f(edict_t * ent)
{
	//+BD - If the player is dead, don't bother
	if (!IS_ALIVE(ent) || !ent->client->pers.weapon)
		return;

	if (ent->client->weaponstate == WEAPON_BANDAGING ||
	    ent->client->bandaging == 1 ||
	    ent->client->bandage_stopped == 1 ||
	    ent->client->weaponstate == WEAPON_ACTIVATING ||
	    ent->client->weaponstate == WEAPON_DROPPING || ent->client->weaponstate == WEAPON_FIRING) {
		return;
	}

	if (!ent->client->fast_reload)
		ent->client->reload_attempts--;
	if (ent->client->reload_attempts < 0)
		ent->client->reload_attempts = 0;

	//First, grab the current magazine max count...

	//Set the weaponstate...
	switch(ent->client->pers.weapon->id) {
	case IT_WEAPON_M3:
		if (ent->client->shot_rds >= ent->client->shot_max)
			return;

		if(ent->client->inventory[ent->client->pers.weapon->ammo] <= 0) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Out of ammo\n");
			return;
		}
		// already in the process of reloading!
		if (ent->client->weaponstate == WEAPON_RELOADING &&
		    (ent->client->shot_rds < (ent->client->shot_max - 1)) &&
		    !(ent->client->fast_reload) &&
		    ((ent->client->pers.weapon->ammo - 1) > 0)) {
			// don't let them start fast reloading until far enough into the firing sequence
			// this gives them a chance to break off from reloading to fire the weapon - zucc
			if (ent->client->ps.gunframe >= 48) {
				ent->client->fast_reload = 1;
				(ent->client->inventory[ent->client->ammo_index])--;
			} else {
				ent->client->reload_attempts++;
			}
		}
		break;
	case IT_WEAPON_HANDCANNON:
		if (ent->client->cannon_rds >= ent->client->cannon_max)
			return;

		if(ent->client->inventory[ent->client->pers.weapon->ammo] <= 0) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Out of ammo\n");
			return;
		}
		if(hc_single->value)
		{
			if(ent->client->pers.hc_mode || ent->client->cannon_rds == 1)
			{	if(ent->client->pers.weapon->ammo < 1)
					return;
			}
			else if(ent->client->pers.weapon->ammo < 2)
				return;
		}
		else if (ent->client->pers.weapon->ammo < 2)
			return;
		break;
	case IT_WEAPON_SNIPER:
		if (ent->client->sniper_rds >= ent->client->sniper_max)
			return;

		if(ent->client->inventory[ent->client->pers.weapon->ammo] <= 0) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Out of ammo\n");
			return;
		}
		// already in the process of reloading!
		if (ent->client->weaponstate == WEAPON_RELOADING
		    && (ent->client->sniper_rds < (ent->client->sniper_max - 1))
		    && !(ent->client->fast_reload)
		    && ((ent->client->pers.weapon->ammo - 1) > 0)) {
			// don't let them start fast reloading until far enough into the firing sequence
			// this gives them a chance to break off from reloading to fire the weapon - zucc
			if (ent->client->ps.gunframe >= 72) {
				ent->client->fast_reload = 1;
				(ent->client->inventory[ent->client->ammo_index])--;
			} else {
				ent->client->reload_attempts++;
			}
		}
		ent->client->ps.fov = 90;
		if (ent->client->pers.weapon)
			ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);
		break;
	case IT_WEAPON_DUALMK23:
		if (ent->client->dual_rds == ent->client->dual_max)
			return;

		if(ent->client->pers.weapon->ammo <= 0) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Out of ammo\n");
			return;
		}
		//TempFile change to pistol, then reload
		if (ent->client->pers.weapon->ammo == 1) {
			gitem_t *it;

			it = GetItemByIndex(IT_WEAPON_MK23);
			it->use(ent, it);
			ent->client->autoreloading = true;
			return;
		}

		break;
	case IT_WEAPON_MP5:
		if (ent->client->mp5_rds == ent->client->mp5_max)
			return;
		if(ent->client->pers.weapon->ammo <= 0) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Out of ammo\n");
			return;
		}
		break;
	case IT_WEAPON_M4:
		if (ent->client->m4_rds == ent->client->m4_max)
			return;
		if(ent->client->pers.weapon->ammo <= 0) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Out of ammo\n");
			return;
		}
		break;
	case IT_WEAPON_MK23:
		if (ent->client->mk23_rds == ent->client->mk23_max)
			return;
		if(ent->client->pers.weapon->ammo <= 0) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Out of ammo\n");
			return;
		}
		break;
	default:
	//We should never get here, but...
	//BD 5/26 - Actually we get here quite often right now. Just exit for weaps that we
	//          don't want reloaded or that never reload (grenades)
		return;
	}

	ent->client->weaponstate = WEAPON_RELOADING;
}

//======================================================================
// Action Add End
//======================================================================

void SelectNextItem(edict_t *ent, item_flags_t itflags)
{
	gclient_t *cl;
	item_id_t  i, index;
	gitem_t	*it;

	cl = ent->client;

	// ZOID
	if (cl->menu)
	{
		PMenu_Next(ent);
		return;
	}
	else if (cl->chase_target)
	{
		ChaseNext(ent);
		return;
	}
	// ZOID

	// scan  for the next valid one
	for (i = static_cast<item_id_t>(IT_NULL + 1); i <= IT_TOTAL; i = static_cast<item_id_t>(i + 1))
	{
		index = static_cast<item_id_t>((cl->pers.selected_item + i) % IT_TOTAL);
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		cl->pers.selected_item_time = level.time + SELECTED_ITEM_TIME;
		cl->ps.stats[STAT_SELECTED_ITEM_NAME] = CS_ITEMS + index;
		return;
	}

	cl->pers.selected_item = IT_NULL;
}

void SelectPrevItem(edict_t *ent, item_flags_t itflags)
{
	gclient_t *cl;
	item_id_t  i, index;
	gitem_t	*it;

	cl = ent->client;

	// ZOID
	if (cl->menu)
	{
		PMenu_Prev(ent);
		return;
	}
	else if (cl->chase_target)
	{
		ChasePrev(ent);
		return;
	}
	// ZOID

	// scan  for the next valid one
	for (i = static_cast<item_id_t>(IT_NULL + 1); i <= IT_TOTAL; i = static_cast<item_id_t>(i + 1))
	{
		index = static_cast<item_id_t>((cl->pers.selected_item + IT_TOTAL - i) % IT_TOTAL);
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		cl->pers.selected_item = index;
		cl->pers.selected_item_time = level.time + SELECTED_ITEM_TIME;
		cl->ps.stats[STAT_SELECTED_ITEM_NAME] = CS_ITEMS + index;
		return;
	}

	cl->pers.selected_item = IT_NULL;
}

void ValidateSelectedItem(edict_t *ent)
{
	gclient_t *cl;

	cl = ent->client;

	if (cl->pers.inventory[cl->pers.selected_item])
		return; // valid

	SelectNextItem(ent, IF_ANY);
}

//=================================================================================

inline bool G_CheatCheck(edict_t *ent)
{
	if (game.maxclients > 1 && !sv_cheats->integer)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_need_cheats");
		return false;
	}

	return true;
}

static void SpawnAndGiveItem(edict_t *ent, item_id_t id)
{
	gitem_t *it = GetItemByIndex(id);

	if (!it)
		return;

	edict_t *it_ent = G_Spawn();
	it_ent->classname = it->classname;
	SpawnItem(it_ent, it);

	if (it_ent->inuse)
	{
		Touch_Item(it_ent, ent, null_trace, true);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);
	}
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f(edict_t *ent)
{
	const char	 *name;
	gitem_t	*it;
	item_id_t index;
	int		  i;
	bool	  give_all;
	edict_t	*it_ent;

	if (!G_CheatCheck(ent))
		return;

	name = gi.args();

	if (Q_strcasecmp(name, "all") == 0)
		give_all = true;
	else
		give_all = false;

	if (give_all || Q_strcasecmp(gi.argv(1), "health") == 0)
	{
		if (gi.argc() == 3)
			ent->health = atoi(gi.argv(2));
		else
			ent->health = ent->max_health;
		if (!give_all)
			return;
	}

	if (give_all || Q_strcasecmp(name, "weapons") == 0)
	{
		for (i = 0; i < IT_TOTAL; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IF_WEAPON))
				continue;
			ent->client->pers.inventory[i] += 1;
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_strcasecmp(name, "ammo") == 0)
	{
		for (i = 0; i < IT_TOTAL; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			if (!(it->flags & IF_AMMO))
				continue;
			Add_Ammo(ent, it, 1000);
		}
		if (!give_all)
			return;
	}

	if (give_all || Q_strcasecmp(name, "armor") == 0)
	{
		return;
		if (!give_all)
			return;
	}

	if (give_all)
	{
		if (!give_all)
			return;
	}

	if (give_all)
	{
		for (i = 0; i < IT_TOTAL; i++)
		{
			it = itemlist + i;
			if (!it->pickup)
				continue;
			// ROGUE
			if (it->flags & (IF_ARMOR | IF_WEAPON | IF_AMMO | IF_NOT_GIVEABLE | IF_TECH))
				continue;
			else if (it->pickup == CTFPickup_Flag)
				continue;
			else if ((it->flags & IF_HEALTH) && !it->use)
				continue;
			// ROGUE
			ent->client->pers.inventory[i] = (it->flags & IF_KEY) ? 8 : 1;
		}

		//G_CheckPowerArmor(ent);
		ent->client->pers.power_cubes = 0xFF;
		return;
	}

	it = FindItem(name);
	if (!it)
	{
		name = gi.argv(1);
		it = FindItem(name);
	}
	if (!it)
		it = FindItemByClassname(name);

	if (!it)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_unknown_item");
		return;
	}

	// ROGUE
	if (it->flags & IF_NOT_GIVEABLE)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_not_giveable");
		return;
	}
	// ROGUE

	index = it->id;

	if (!it->pickup)
	{
		ent->client->pers.inventory[index] = 1;
		return;
	}

	if (it->flags & IF_AMMO)
	{
		if (gi.argc() == 3)
			ent->client->pers.inventory[index] = atoi(gi.argv(2));
		else
			ent->client->pers.inventory[index] += it->quantity;
	}
	else
	{
		it_ent = G_Spawn();
		it_ent->classname = it->classname;
		SpawnItem(it_ent, it);
		// PMM - since some items don't actually spawn when you say to ..
		if (!it_ent->inuse)
			return;
		// pmm
		Touch_Item(it_ent, ent, null_trace, true);
		if (it_ent->inuse)
			G_FreeEdict(it_ent);
	}
}

void Cmd_SetPOI_f(edict_t *self)
{
	if (!G_CheatCheck(self))
		return;

	level.current_poi = self->s.origin;
	level.valid_poi = true;
}

void Cmd_CheckPOI_f(edict_t *self)
{
	if (!G_CheatCheck(self))
		return;

	if (!level.valid_poi)
		return;
	
	char visible_pvs = gi.inPVS(self->s.origin, level.current_poi, false) ? 'y' : 'n';
	char visible_pvs_portals = gi.inPVS(self->s.origin, level.current_poi, true) ? 'y' : 'n';
	char visible_phs = gi.inPHS(self->s.origin, level.current_poi, false) ? 'y' : 'n';
	char visible_phs_portals = gi.inPHS(self->s.origin, level.current_poi, true) ? 'y' : 'n';

	gi.Com_PrintFmt("pvs {} + portals {}, phs {} + portals {}\n", visible_pvs, visible_pvs_portals, visible_phs, visible_phs_portals);
}

// [Paril-KEX]
static void Cmd_Target_f(edict_t *ent)
{
	if (!G_CheatCheck(ent))
		return;

	ent->target = gi.argv(1);
	G_UseTargets(ent, ent);
	ent->target = nullptr;
}

/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f(edict_t *ent)
{
	const char *msg;

	if (!G_CheatCheck(ent))
		return;

	ent->flags ^= FL_GODMODE;
	if (!(ent->flags & FL_GODMODE))
		msg = "godmode OFF\n";
	else
		msg = "godmode ON\n";

	gi.LocClient_Print(ent, PRINT_HIGH, msg);
}
void ED_ParseField(const char *key, const char *value, edict_t *ent);

/*
==================
Cmd_Immortal_f

Sets client to immortal - take damage but never go below 1 hp

argv(0) immortal
==================
*/
void Cmd_Immortal_f(edict_t *ent)
{
	const char *msg;

	if (!G_CheatCheck(ent))
		return;

	ent->flags ^= FL_IMMORTAL;
	if (!(ent->flags & FL_IMMORTAL))
		msg = "immortal OFF\n";
	else
		msg = "immortal ON\n";

	gi.LocClient_Print(ent, PRINT_HIGH, msg);
}

/*
=================
Cmd_Spawn_f

Spawn class name

argv(0) spawn
argv(1) <classname>
argv(2+n) "key"...
argv(3+n) "value"...
=================
*/
void Cmd_Spawn_f(edict_t *ent)
{
	if (!G_CheatCheck(ent))
		return;

	solid_t backup = ent->solid;
	ent->solid = SOLID_NOT;
	gi.linkentity(ent);

	edict_t* other = G_Spawn();
	other->classname = gi.argv(1);

	other->s.origin = ent->s.origin + (AngleVectors(ent->s.angles).forward * 24.f);
	other->s.angles[1] = ent->s.angles[1];

	st = {};

	if (gi.argc() > 3)
	{
		for (int i = 2; i < gi.argc(); i += 2)
			ED_ParseField(gi.argv(i), gi.argv(i + 1), other);
	}

	ED_CallSpawn(other);

	if (other->inuse)
	{
		vec3_t forward, end;
		AngleVectors(ent->client->v_angle, forward, nullptr, nullptr);
		end = ent->s.origin;
		end[2] += ent->viewheight;
		end += (forward * 8192);

		trace_t tr = gi.traceline(ent->s.origin + vec3_t{0.f, 0.f, (float) ent->viewheight}, end, other, MASK_SHOT | CONTENTS_MONSTERCLIP);
		other->s.origin = tr.endpos;

		for (int32_t i = 0; i < 3; i++)
		{
			if (tr.plane.normal[i] > 0)
				other->s.origin[i] -= other->mins[i] * tr.plane.normal[i];
			else
				other->s.origin[i] += other->maxs[i] * -tr.plane.normal[i];
		}

		while (gi.trace(other->s.origin, other->mins, other->maxs, other->s.origin, other,
			MASK_SHOT | CONTENTS_MONSTERCLIP)
			.startsolid)
		{
			float dx = other->mins[0] - other->maxs[0];
			float dy = other->mins[1] - other->maxs[1];
			other->s.origin += forward * -sqrtf(dx * dx + dy * dy);

			if ((other->s.origin - ent->s.origin).dot(forward) < 0)
			{
				gi.Client_Print(ent, PRINT_HIGH, "Couldn't find a suitable spawn location\n");
				G_FreeEdict(other);
				break;
			}
		}

		if (other->inuse)
			gi.linkentity(other);

		if (other->svflags & SVF_MONSTER)
			other->think(other);
	}

	ent->solid = backup;
	gi.linkentity(ent);
}

/*
=================
Cmd_Spawn_f

Telepo'

argv(0) teleport
argv(1) x
argv(2) y
argv(3) z
=================
*/
void Cmd_Teleport_f(edict_t *ent)
{
	if (!G_CheatCheck(ent))
		return;

	if (gi.argc() < 4)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "Not enough args; teleport x y z\n");
		return;
	}

	ent->s.origin[0] = (float) atof(gi.argv(1));
	ent->s.origin[1] = (float) atof(gi.argv(2));
	ent->s.origin[2] = (float) atof(gi.argv(3));
	gi.linkentity(ent);
}

/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f(edict_t *ent)
{
	const char *msg;

	if (!G_CheatCheck(ent))
		return;

	ent->flags ^= FL_NOTARGET;
	if (!(ent->flags & FL_NOTARGET))
		msg = "notarget OFF\n";
	else
		msg = "notarget ON\n";

	gi.LocClient_Print(ent, PRINT_HIGH, msg);
}

/*
==================
Cmd_Novisible_f

Sets client to "super notarget"

argv(0) notarget
==================
*/
void Cmd_Novisible_f(edict_t *ent)
{
	const char *msg;

	if (!G_CheatCheck(ent))
		return;

	ent->flags ^= FL_NOVISIBLE;
	if (!(ent->flags & FL_NOVISIBLE))
		msg = "novisible OFF\n";
	else
		msg = "novisible ON\n";

	gi.LocClient_Print(ent, PRINT_HIGH, msg);
}

void Cmd_AlertAll_f(edict_t *ent)
{
	if (!G_CheatCheck(ent))
		return;

	for (size_t i = 0; i < globals.num_edicts; i++)
	{
		edict_t *t = &g_edicts[i];

		if (!t->inuse || t->health <= 0 || !(t->svflags & SVF_MONSTER))
			continue;

		t->enemy = ent;
		FoundTarget(t);
	}
}

/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f(edict_t *ent)
{
	const char *msg;

	if (!G_CheatCheck(ent))
		return;

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

	gi.LocClient_Print(ent, PRINT_HIGH, msg);
}

/*
==================
Cmd_Use_f

Use an inventory item
==================
*/
void Cmd_Use_f(edict_t *ent)
{
	item_id_t index;
	gitem_t	*it;
	const char	 *s;

	if (ent->health <= 0 || ent->deadflag)
		return;

	s = gi.args();

	const char *cmd = gi.argv(0);
	if (!Q_strcasecmp(cmd, "use_index") || !Q_strcasecmp(cmd, "use_index_only"))
	{
		it = GetItemByIndex((item_id_t) atoi(s));
	}
	else
	{
		it = FindItem(s);
	}

	if (!it)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_unknown_item_name", s);
		return;
	}
	if (!it->use)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_item_not_usable");
		return;
	}
	index = it->id;

	// Paril: Use_Weapon handles weapon availability
	if (!(it->flags & IF_WEAPON) && !ent->client->pers.inventory[index])
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_out_of_item", it->pickup_name);
		return;
	}

	// allow weapon chains for use
	ent->client->no_weapon_chains = !!strcmp(gi.argv(0), "use") && !!strcmp(gi.argv(0), "use_index");

	it->use(ent, it);

	ValidateSelectedItem(ent);
}

/*
==================
Cmd_Drop_f

Drop an inventory item
==================
*/
void Cmd_Drop_f(edict_t *ent)
{
	item_id_t index;
	gitem_t	*it;
	const char *s;

	if (ent->health <= 0 || ent->deadflag)
		return;

	// ZOID--special case for tech powerups
	// if (Q_strcasecmp(gi.args(), "tech") == 0 && (it = CTFWhat_Tech(ent)) != nullptr)
	// {
	// 	it->drop(ent, it);
	// 	ValidateSelectedItem(ent);
	// 	return;
	// }
	// ZOID

	s = gi.args();

	const char *cmd = gi.argv(0);

	if (!Q_strcasecmp(cmd, "drop_index"))
	{
		it = GetItemByIndex((item_id_t) atoi(s));
	}
	else
	{
		it = FindItem(s);
	}

	if (!it)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "Unknown item : {}\n", s);
		return;
	}
	if (!it->drop)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_item_not_droppable");
		return;
	}
	index = it->id;
	if (!ent->client->pers.inventory[index])
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_out_of_item", it->pickup_name);
		return;
	}

	it->drop(ent, it);

	ValidateSelectedItem(ent);
}

/*
=================
Cmd_Inven_f
=================
*/
void Cmd_Inven_f(edict_t *ent)
{
	int		   i;
	gclient_t *cl;

	cl = ent->client;

	cl->showscores = false;
	cl->showhelp = false;

	globals.server_flags &= ~SERVER_FLAG_SLOW_TIME;

	// ZOID
	if (ent->client->menu)
	{
		PMenu_Close(ent);
		ent->client->update_chase = true;
		return;
	}
	// ZOID

	if (cl->showinventory)
	{
		cl->showinventory = false;
		return;
	}

	// ZOID
	if (G_TeamplayEnabled())
	{
		if (ctf->integer && cl->resp.ctf_team == CTF_NOTEAM){
			CTFOpenJoinMenu(ent);
			return;
		} else {
			OpenJoinMenu(ent);
			return;
		}
	}
	// ZOID

	cl->showinventory = true;

	gi.WriteByte(svc_inventory);
	for (i = 0; i < IT_TOTAL; i++)
		gi.WriteShort(cl->pers.inventory[i]);
	for (; i < MAX_ITEMS; i++)
		gi.WriteShort(0);
	gi.unicast(ent, true);
}

/*
=================
Cmd_InvUse_f
=================
*/
void Cmd_InvUse_f(edict_t *ent)
{
	gitem_t *it;

	// ZOID
	if (ent->client->menu)
	{
		PMenu_Select(ent);
		return;
	}
	// ZOID

	if (ent->health <= 0 || ent->deadflag)
		return;

	ValidateSelectedItem(ent);

	if (ent->client->pers.selected_item == IT_NULL)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_no_item_to_use");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->use)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_item_not_usable");
		return;
	}

	// don't allow weapon chains for invuse
	ent->client->no_weapon_chains = true;
	it->use(ent, it);

	ValidateSelectedItem(ent);
}

/*
=================
Cmd_WeapPrev_f
=================
*/
void Cmd_WeapPrev_f(edict_t *ent)
{
	gclient_t *cl;
	item_id_t  i, index;
	gitem_t	*it;
	item_id_t  selected_weapon;

	cl = ent->client;

	if (ent->health <= 0 || ent->deadflag)
		return;
	if (!cl->pers.weapon)
		return;

	// don't allow weapon chains for weapprev
	cl->no_weapon_chains = true;

	selected_weapon = cl->pers.weapon->id;

	// scan  for the next valid one
	for (i = static_cast<item_id_t>(IT_NULL + 1); i <= IT_TOTAL; i = static_cast<item_id_t>(i + 1))
	{
		// PMM - prevent scrolling through ALL weapons
		index = static_cast<item_id_t>((selected_weapon + IT_TOTAL - i) % IT_TOTAL);
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & IF_WEAPON))
			continue;
		it->use(ent, it);
		// ROGUE
		if (cl->newweapon == it)
			return; // successful
					// ROGUE
	}
}

/*
=================
Cmd_WeapNext_f
=================
*/
void Cmd_WeapNext_f(edict_t *ent)
{
	gclient_t *cl;
	item_id_t  i, index;
	gitem_t	*it;
	item_id_t  selected_weapon;

	cl = ent->client;

	if (ent->health <= 0 || ent->deadflag)
		return;
	if (!cl->pers.weapon)
		return;

	// don't allow weapon chains for weapnext
	cl->no_weapon_chains = true;

	selected_weapon = cl->pers.weapon->id;

	// scan  for the next valid one
	for (i = static_cast<item_id_t>(IT_NULL + 1); i <= IT_TOTAL; i = static_cast<item_id_t>(i + 1))
	{
		// PMM - prevent scrolling through ALL weapons
		index = static_cast<item_id_t>((selected_weapon + i) % IT_TOTAL);
		if (!cl->pers.inventory[index])
			continue;
		it = &itemlist[index];
		if (!it->use)
			continue;
		if (!(it->flags & IF_WEAPON))
			continue;
		it->use(ent, it);
		// PMM - prevent scrolling through ALL weapons

		// ROGUE
		if (cl->newweapon == it)
			return;
		// ROGUE
	}
}

/*
=================
Cmd_WeapLast_f
=================
*/
void Cmd_WeapLast_f(edict_t *ent)
{
	gclient_t *cl;
	int		   index;
	gitem_t	*it;

	cl = ent->client;

	if (ent->health <= 0 || ent->deadflag)
		return;
	if (!cl->pers.weapon || !cl->pers.lastweapon)
		return;

	// don't allow weapon chains for weaplast
	cl->no_weapon_chains = true;

	index = cl->pers.lastweapon->id;
	if (!cl->pers.inventory[index])
		return;
	it = &itemlist[index];
	if (!it->use)
		return;
	if (!(it->flags & IF_WEAPON))
		return;
	it->use(ent, it);
}

/*
=================
Cmd_InvDrop_f
=================
*/
void Cmd_InvDrop_f(edict_t *ent)
{
	gitem_t *it;

	if (ent->health <= 0 || ent->deadflag)
		return;

	ValidateSelectedItem(ent);

	if (ent->client->pers.selected_item == IT_NULL)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_no_item_to_drop");
		return;
	}

	it = &itemlist[ent->client->pers.selected_item];
	if (!it->drop)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_item_not_droppable");
		return;
	}
	it->drop(ent, it);

	ValidateSelectedItem(ent);
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f(edict_t *ent)
{
	// ZOID
	if (ent->client->resp.spectator)
		return;
	// ZOID

	if ((level.time - ent->client->respawn_time) < 5_sec)
		return;

	ent->flags &= ~FL_GODMODE;
	ent->health = 0;

	// ROGUE
	//  make sure no trackers are still hurting us.
	if (ent->client->tracker_pain_time)
		RemoveAttackingPainDaemons(ent);

	if (ent->client->owned_sphere)
	{
		G_FreeEdict(ent->client->owned_sphere);
		ent->client->owned_sphere = nullptr;
	}
	// ROGUE

	// [Paril-KEX] don't allow kill to take points away in TDM
	player_die(ent, ent, ent, 100000, vec3_origin, { MOD_SUICIDE, !!teamplay->integer });
}

/*
=================
Cmd_Kill_AI_f
=================
*/
void Cmd_Kill_AI_f( edict_t * ent ) {
	if ( !sv_cheats->integer ) {
		gi.LocClient_Print( ent, PRINT_HIGH, "Kill_AI: Cheats Must Be Enabled!\n" );
		return;
	}

	// except the one we're looking at...
	edict_t *looked_at = nullptr;

	vec3_t start = ent->s.origin + vec3_t{0.f, 0.f, (float) ent->viewheight};
	vec3_t end = start + ent->client->v_forward * 1024.f;

	looked_at = gi.traceline(start, end, ent, MASK_SHOT).ent;

	const int numEdicts = globals.num_edicts;
	for ( int edictIdx = 1; edictIdx < numEdicts; ++edictIdx ) 
	{
		edict_t * edict = &g_edicts[ edictIdx ];
		if ( !edict->inuse || edict == looked_at ) {
			continue;
		}

		if ( ( edict->svflags & SVF_MONSTER ) == 0 ) 
		{
			continue;
		}

		G_FreeEdict( edict );
	}

	gi.LocClient_Print( ent, PRINT_HIGH, "Kill_AI: All AI Are Dead...\n" );
}

/*
=================
Cmd_Where_f
=================
*/
void Cmd_Where_f( edict_t * ent ) {
	if ( ent == nullptr || ent->client == nullptr ) {
		return;
	}

	const vec3_t & origin = ent->s.origin;

	std::string location;
	fmt::format_to( std::back_inserter( location ), FMT_STRING( "{:.1f} {:.1f} {:.1f}\n" ), origin[ 0 ], origin[ 1 ], origin[ 2 ] );
	gi.LocClient_Print( ent, PRINT_HIGH, "Location: {}\n", location.c_str() );
	gi.SendToClipBoard( location.c_str() );
}

/*
=================
Cmd_Clear_AI_Enemy_f
=================
*/
void Cmd_Clear_AI_Enemy_f( edict_t * ent ) {
	if ( !sv_cheats->integer ) {
		gi.LocClient_Print( ent, PRINT_HIGH, "Cmd_Clear_AI_Enemy: Cheats Must Be Enabled!\n" );
		return;
	}

	const int numEdicts = globals.num_edicts;
	for ( int edictIdx = 1; edictIdx < numEdicts; ++edictIdx ) {
		edict_t * edict = &g_edicts[ edictIdx ];
		if ( !edict->inuse ) {
			continue;
		}

		if ( ( edict->svflags & SVF_MONSTER ) == 0 ) {
			continue;
		}

		edict->monsterinfo.aiflags |= AI_FORGET_ENEMY;
	}

	gi.LocClient_Print( ent, PRINT_HIGH, "Cmd_Clear_AI_Enemy: Clear All AI Enemies...\n" );
}

/*
=================
Cmd_PutAway_f
=================
*/
void Cmd_PutAway_f(edict_t *ent)
{
	ent->client->showscores = false;
	ent->client->showhelp = false;
	ent->client->showinventory = false;

	globals.server_flags &= ~SERVER_FLAG_SLOW_TIME;

	// ZOID
	if (ent->client->menu)
		PMenu_Close(ent);
	ent->client->update_chase = true;
	// ZOID
}

int PlayerSort(const void *a, const void *b)
{
	int anum, bnum;

	anum = *(const int *) a;
	bnum = *(const int *) b;

	anum = game.clients[anum].ps.stats[STAT_FRAGS];
	bnum = game.clients[bnum].ps.stats[STAT_FRAGS];

	if (anum < bnum)
		return -1;
	if (anum > bnum)
		return 1;
	return 0;
}

constexpr size_t MAX_IDEAL_PACKET_SIZE = 1024;

/*
=================
Cmd_Players_f
=================
*/
void Cmd_Players_f(edict_t *ent)
{
	size_t	i;
	size_t	count;
	static std::string	small, large;
	int		index[MAX_CLIENTS];

	small.clear();
	large.clear();

	count = 0;
	for (i = 0; i < game.maxclients; i++)
		if (game.clients[i].pers.connected)
		{
			index[count] = i;
			count++;
		}

	// sort by frags
	qsort(index, count, sizeof(index[0]), PlayerSort);

	// print information
	large[0] = 0;

	if (count)
	{
		for (i = 0; i < count; i++)
		{
			fmt::format_to(std::back_inserter(small), FMT_STRING("{:3} {}\n"), game.clients[index[i]].ps.stats[STAT_FRAGS],
						game.clients[index[i]].pers.netname);

			if (small.length() + large.length() > MAX_IDEAL_PACKET_SIZE - 50)
			{ // can't print all of them in one packet
				large += "...\n";
				break;
			}

			large += small;
			small.clear();
		}
	
		// remove the last newline
		large.pop_back();
	}

	gi.LocClient_Print(ent, PRINT_HIGH | PRINT_NO_NOTIFY, "$g_players", large.c_str(), count);
}

bool CheckFlood(edict_t *ent)
{
	int		   i;
	gclient_t *cl;

	if (flood_msgs->integer)
	{
		cl = ent->client;

		if (level.time < cl->flood_locktill)
		{
			gi.LocClient_Print(ent, PRINT_HIGH, "$g_flood_cant_talk",
				(cl->flood_locktill - level.time).seconds<int32_t>());
			return true;
		}
		i = cl->flood_whenhead - flood_msgs->integer + 1;
		if (i < 0)
			i = (sizeof(cl->flood_when) / sizeof(cl->flood_when[0])) + i;
		if (i >= q_countof(cl->flood_when))
			i = 0;
		if (cl->flood_when[i] && level.time - cl->flood_when[i] < gtime_t::from_sec(flood_persecond->value))
		{
			cl->flood_locktill = level.time + gtime_t::from_sec(flood_waitdelay->value);
			gi.LocClient_Print(ent, PRINT_CHAT, "$g_flood_cant_talk",
				flood_waitdelay->integer);
			return true;
		}
		cl->flood_whenhead = (cl->flood_whenhead + 1) % (sizeof(cl->flood_when) / sizeof(cl->flood_when[0]));
		cl->flood_when[cl->flood_whenhead] = level.time;
	}
	return false;
}

/*
=================
Cmd_Wave_f
=================
*/
void Cmd_Wave_f(edict_t *ent)
{
	int i;

	i = atoi(gi.argv(1));

	// no dead or noclip waving
	if (ent->deadflag || ent->movetype == MOVETYPE_NOCLIP)
		return;

	// can't wave when ducked
	bool do_animate = ent->client->anim_priority <= ANIM_WAVE && !(ent->client->ps.pmove.pm_flags & PMF_DUCKED);

	if (do_animate)
		ent->client->anim_priority = ANIM_WAVE;

	constexpr float NOTIFY_DISTANCE = 256.f;
	bool notified_anybody = false;
	const char *self_notify_msg = nullptr, *other_notify_msg = nullptr, *other_notify_none_msg = nullptr;

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->v_angle, { 0, 0, 0 }, start, dir);

	// see who we're aiming at
	edict_t *aiming_at = nullptr;
	float best_dist = -9999;

	for (auto player : active_players())
	{
		if (player == ent)
			continue;

		vec3_t cdir = player->s.origin - start;
		float dist = cdir.normalize();

		float dot = ent->client->v_forward.dot(cdir);

		if (dot < 0.97)
			continue;
		else if (dist < best_dist)
			continue;

		best_dist = dist;
		aiming_at = player;
	}

	switch (i)
	{
	case GESTURE_FLIP_OFF:
		self_notify_msg = "$g_flipoff";
		other_notify_msg = "$g_flipoff_other";
		other_notify_none_msg = "$g_flipoff_none";
		if (do_animate)
		{
			ent->s.frame = FRAME_flip01 - 1;
			ent->client->anim_end = FRAME_flip12;
		}
		break;
	case GESTURE_SALUTE:
		self_notify_msg = "$g_salute";
		other_notify_msg = "$g_salute_other";
		other_notify_none_msg = "$g_salute_none";
		if (do_animate)
		{
			ent->s.frame = FRAME_salute01 - 1;
			ent->client->anim_end = FRAME_salute11;
		}
		break;
	case GESTURE_TAUNT:
		self_notify_msg = "$g_taunt";
		other_notify_msg = "$g_taunt_other";
		other_notify_none_msg = "$g_taunt_none";
		if (do_animate)
		{
			ent->s.frame = FRAME_taunt01 - 1;
			ent->client->anim_end = FRAME_taunt17;
		}
		break;
	case GESTURE_WAVE:
		self_notify_msg = "$g_wave";
		other_notify_msg = "$g_wave_other";
		other_notify_none_msg = "$g_wave_none";
		if (do_animate)
		{
			ent->s.frame = FRAME_wave01 - 1;
			ent->client->anim_end = FRAME_wave11;
		}
		break;
	case GESTURE_POINT:
	default:
		self_notify_msg = "$g_point";
		other_notify_msg = "$g_point_other";
		other_notify_none_msg = "$g_point_none";
		if (do_animate)
		{
			ent->s.frame = FRAME_point01 - 1;
			ent->client->anim_end = FRAME_point12;
		}
		break;
	}

	bool has_a_target = false;

	if (i == GESTURE_POINT)
	{
		for (auto player : active_players())
		{
			if (player == ent)
				continue;
			else if (!OnSameTeam(ent, player))
				continue;

			has_a_target = true;
			break;
		}
	}

	if (i == GESTURE_POINT && has_a_target)
	{
		// don't do this stuff if we're flooding
		if (CheckFlood(ent))
			return;

		trace_t tr = gi.traceline(start, start + (ent->client->v_forward * 2048), ent, MASK_SHOT & ~CONTENTS_WINDOW);
		other_notify_msg = "$g_point_other_ping";

		uint32_t key = GetUnicastKey();

		if (tr.fraction != 1.0f)
		{
			// send to all teammates
			for (auto player : active_players())
			{
				if (player != ent && !OnSameTeam(ent, player))
					continue;

				gi.WriteByte(svc_poi);
				gi.WriteShort(POI_PING + (ent->s.number - 1));
				gi.WriteShort(5000);
				gi.WritePosition(tr.endpos);
				gi.WriteShort(gi.imageindex("loc_ping"));
				gi.WriteByte(208);
				gi.WriteByte(POI_FLAG_NONE);
				gi.unicast(player, false);

				gi.local_sound(player, CHAN_AUTO, gi.soundindex("misc/help_marker.wav"), 1.0f, ATTN_NONE, 0.0f, key);
				gi.LocClient_Print(player, PRINT_HIGH, other_notify_msg, ent->client->pers.netname);
			}
		}
	}
	else
	{
		if (CheckFlood(ent))
			return;

		edict_t* targ = nullptr;
		while ((targ = findradius(targ, ent->s.origin, 1024)) != nullptr)
		{
			if (ent == targ) continue;
			if (!targ->client) continue;
			if (!gi.inPVS(ent->s.origin, targ->s.origin, false)) continue;

			if (aiming_at && other_notify_msg)
				gi.LocClient_Print(targ, PRINT_TTS, other_notify_msg, ent->client->pers.netname, aiming_at->client->pers.netname);
			else if (other_notify_none_msg)
				gi.LocClient_Print(targ, PRINT_TTS, other_notify_none_msg, ent->client->pers.netname);
		}

		if (aiming_at && other_notify_msg)
			gi.LocClient_Print(ent, PRINT_TTS, other_notify_msg, ent->client->pers.netname, aiming_at->client->pers.netname);
		else if (other_notify_none_msg)
			gi.LocClient_Print(ent, PRINT_TTS, other_notify_none_msg, ent->client->pers.netname);
	}

	ent->client->anim_time = 0_ms;
}

#ifndef KEX_Q2_GAME
/*
==================
Cmd_Say_f

NB: only used for non-Playfab stuff
==================
*/
void Cmd_Say_f(edict_t *ent, bool arg0)
{
	edict_t *other;
	const char	 *p_in;
	static std::string text;

	if (gi.argc() < 2 && !arg0)
		return;
	else if (CheckFlood(ent))
		return;

	text.clear();
	fmt::format_to(std::back_inserter(text), FMT_STRING("{}: "), ent->client->pers.netname);

	if (arg0)
	{
		text += gi.argv(0);
		text += " ";
		text += gi.args();
	}
	else
	{
		p_in = gi.args();
		size_t in_len = strlen(p_in);

		if (p_in[0] == '\"' && p_in[in_len - 1] == '\"')
			text += std::string_view(p_in + 1, in_len - 2);
		else
			text += p_in;
	}

	// don't let text be too long for malicious reasons
	if (text.length() > 150)
		text.resize(150);

	if (text.back() != '\n')
		text.push_back('\n');

	if (sv_dedicated->integer)
		gi.Client_Print(nullptr, PRINT_CHAT, text.c_str());

	for (uint32_t j = 1; j <= game.maxclients; j++)
	{
		other = &g_edicts[j];
		if (!other->inuse)
			continue;
		if (!other->client)
			continue;
		gi.Client_Print(other, PRINT_CHAT, text.c_str());
	}
}
#endif

void Cmd_PlayerList_f(edict_t *ent)
{
	uint32_t i;
	static std::string str, text;
	edict_t *e2;

	str.clear();
	text.clear();

	// connect time, ping, score, name
	for (i = 0, e2 = g_edicts + 1; i < game.maxclients; i++, e2++)
	{
		if (!e2->inuse)
			continue;

		fmt::format_to(std::back_inserter(str), FMT_STRING("{:02}:{:02} {:4} {:3} {}{}\n"), (level.time - e2->client->resp.entertime).milliseconds() / 60000,
					((level.time - e2->client->resp.entertime).milliseconds() % 60000) / 1000, e2->client->ping,
					e2->client->resp.score, e2->client->pers.netname, e2->client->resp.spectator ? " (spectator)" : "");

		if (text.length() + str.length() > MAX_IDEAL_PACKET_SIZE - 50)
		{
			text += "...\n";
			break;
		}

		text += str;
	}

	if (text.length())
		gi.Client_Print(ent, PRINT_HIGH, text.c_str());
}

void Cmd_Switchteam_f(edict_t* ent)
{
	if (!G_TeamplayEnabled())
		return;

	// [Paril-KEX] in force-join, just do a regular team join.
	if (g_teamplay_force_join->integer)
	{
		// check if we should even switch teams
		edict_t *player;
		uint32_t team1count = 0, team2count = 0;
		// Support eventually //uint32_t team3count = 0;
		ctfteam_t best_team;
		uint32_t best_action_team = 0;

		for (uint32_t i = 1; i <= game.maxclients; i++)
		{
			player = &g_edicts[i];

			// NB: we are counting ourselves in this one, unlike
			// the other assign team func
			if (!player->inuse)
				continue;

			if (ctf->integer){
				switch (player->client->resp.ctf_team)
				{
				case CTF_TEAM1:
					team1count++;
					break;
				case CTF_TEAM2:
					team2count++;
					break;
				default:
					break;
				}
			} else {
				switch (player->client->resp.team)
				{
				case TEAM1:
					team1count++;
					break;
				case TEAM2:
					team2count++;
					break;
				// 3 team some day
				// case TEAM3:
				// 	team3count++;
				// 	break;
				default:
					break;
				}
			}
		}

		if (team1count < team2count)
				if (ctf->integer)
					best_team = CTF_TEAM1;
				else
					best_action_team = TEAM1;
		else
				if ((ctf->integer))
					best_team = CTF_TEAM2;
				else
					best_action_team = TEAM2;

		if (ent->client->resp.ctf_team != best_team || ent->client->resp.team != best_action_team)
		{
			////
			ent->svflags = SVF_NONE;
			ent->flags &= ~FL_GODMODE;

			char value[MAX_INFO_VALUE] = { 0 };
			gi.Info_ValueForKey(ent->client->pers.userinfo, "skin", value, sizeof(value));
			if (ctf->value){
				ent->client->resp.ctf_team = best_team;
				ent->client->resp.ctf_state = 0;
				CTFAssignSkin(ent, value);
				// if anybody has a menu open, update it immediately
				CTFDirtyTeamMenu();
			} else {
				ent->client->resp.team = best_action_team;
				AssignSkin(ent, value, false);
				// if anybody has a menu open, update it immediately
				//CTFDirtyTeamMenu();
			}

			if (ent->solid == SOLID_NOT)
			{
				// spectator
				PutClientInServer(ent);

				G_PostRespawn(ent);

				if (ctf->value){
					gi.LocBroadcast_Print(PRINT_HIGH, "$g_joined_team",
						ent->client->pers.netname, CTFTeamName(best_team));
					return;
				} else {
					gi.LocBroadcast_Print(PRINT_HIGH, "$g_joined_team",
						ent->client->pers.netname, TeamName(best_action_team));
					return;
				}
			}

			ent->health = 0;
			player_die(ent, ent, ent, 100000, vec3_origin, { MOD_SUICIDE, true });

			// don't even bother waiting for death frames
			ent->deadflag = true;
			respawn(ent);

			ent->client->resp.score = 0;

			if (ctf->value) {
				gi.LocBroadcast_Print(PRINT_HIGH, "$g_changed_team",
					ent->client->pers.netname, CTFTeamName(best_team));
			} else {
				gi.LocBroadcast_Print(PRINT_HIGH, "$g_changed_team",
					ent->client->pers.netname, TeamName(best_action_team));
			}
		}

		return;
	}

	if (ctf->value && ent->client->resp.ctf_team != CTF_NOTEAM)
		CTFObserver(ent);

	if (!ent->client->menu)
		if (ctf->value)
			CTFOpenJoinMenu(ent);
		// Auto-show menu
		else if (teamplay->integer && auto_menu->integer)
			OpenJoinMenu(ent);
}

static void Cmd_ListMonsters_f(edict_t *ent)
{
	if (!G_CheatCheck(ent))
		return;
	else if (!g_debug_monster_kills->integer)
		return;

	for (size_t i = 0; i < level.total_monsters; i++)
	{
		edict_t *e = level.monsters_registered[i];

		if (!e || !e->inuse)
			continue;
		else if (!(e->svflags & SVF_MONSTER) || (e->monsterinfo.aiflags & AI_DO_NOT_COUNT))
			continue;
		else if (e->deadflag)
			continue;

		gi.Com_PrintFmt("{}\n", *e);
	}
}

/*
=================
Action Commands
=================
*/

static void Cmd_Streak_f (edict_t * ent) {
	gi.LocClient_Print(ent, PRINT_HIGH, "Your Killing Streak is: %d\n", ent->client->resp.streakKills);
}

void Cmd_Choose_f(edict_t * ent)
{
	const char *s;
	const char *wpnText, *itmText;
	//int itemNum = 0;
	gitem_t *item;
	item_id_t itemNum = IT_NULL;

	// only works in teamplay
	if (!(gameSettings & GS_WEAPONCHOOSE))
		return;

	 s = gi.args();
	 //if (*s) {
	 //	itemNum = GetItemNumFromArg(s);
	 //	if (!itemNum)
	 //		itemNum = GetWeaponNumFromArg(s);
	 //}

	 switch(itemNum) {
	 case IT_WEAPON_DUALMK23:
	 case IT_WEAPON_M3:
	 case IT_WEAPON_HANDCANNON:
	 case IT_WEAPON_MP5:
	 case IT_WEAPON_SNIPER:
	 case IT_WEAPON_KNIFE:
	 case IT_WEAPON_M4:
	 	// Weapon bans maybe later
	 	// if (!WPF_ALLOWED(itemNum)) {
	 	// 	gi.LocClient_Print(ent, PRINT_HIGH, "Weapon disabled on this server.\n");
	 	// 	return;
	 	// }
	 	// ent->client->pers.chosenWeapon = GET_ITEM(itemNum);
	 	// break;
	 case IT_ITEM_LASERSIGHT:
	 case IT_ITEM_VEST:
	 case IT_ITEM_SLIPPERS:
	 case IT_ITEM_QUIET:
	 case IT_ITEM_HELM:
	 case IT_ITEM_BANDOLIER:
	 	// Item bans maybe later
	 	// if (!ITF_ALLOWED(itemNum)) {
	 	// 	gi.LocClient_Print(ent, PRINT_HIGH, "Item disabled on this server.\n");
	 	// 	return;
	 	// }
	 	// ent->client->pers.chosenItem = GET_ITEM(itemNum);
	 	// break;
	 default:
	 	gi.LocClient_Print(ent, PRINT_HIGH, "Invalid weapon or item choice.\n");
	 	return;
	 }

	item = ent->client->pers.chosenWeapon;
	//item = ent->client->pers.weapon;
	wpnText = (item && item->pickup_name) ? item->pickup_name : "NONE";

	item = ent->client->pers.chosenItem;
	itmText = (item && item->pickup_name) ? item->pickup_name : "NONE";

}

// AQ:TNG - JBravo adding tkok
void Cmd_TKOk(edict_t * ent)
{
	if (!ent->enemy || !ent->enemy->inuse || !ent->enemy->client || (ent == ent->enemy)) {
		gi.LocClient_Print(ent, PRINT_HIGH, "Nothing to forgive\n");
	} else if (ent->client->resp.team == ent->enemy->client->resp.team) {
		if (ent->enemy->client->resp.team_kills) {
			gi.LocClient_Print(ent, PRINT_HIGH, "You forgave %s\n", ent->enemy->client->pers.netname);
			gi.LocClient_Print(ent->enemy, PRINT_HIGH, "%s forgave you\n", ent->client->pers.netname);
			ent->enemy->client->resp.team_kills--;
			if (ent->enemy->client->resp.team_wounds)
				ent->enemy->client->resp.team_wounds /= 2;
		}
	} else {
		gi.LocClient_Print(ent, PRINT_HIGH, "That's very noble of you...\n");
		gi.LocBroadcast_Print(PRINT_HIGH, "%s turned the other cheek\n", ent->client->pers.netname);
	}
	ent->enemy = NULL;
	return;
}

void Cmd_FF_f( edict_t *ent )
{
	if( teamplay->value )
		gi.LocClient_Print(ent, PRINT_MEDIUM, "Friendly Fire {}\n", g_friendly_fire->integer ? "OFF" : "ON");
	else
		gi.LocClient_Print( ent, PRINT_MEDIUM, "FF only applies to teamplay.\n" );
}

void Cmd_Time(edict_t * ent)
{
	int mins = 0, secs = 0, remaining = 0, rmins = 0, rsecs = 0, gametime = 0;

	gametime = level.matchTime;

	mins = gametime / 60;
	secs = gametime % 60;
	remaining = (timelimit->value * 60) - gametime;
	if( remaining >= 0 )
	{
		rmins = remaining / 60;
		rsecs = remaining % 60;
	}

	if( timelimit->value )
		gi.LocClient_Print( ent, PRINT_HIGH, "Elapsed time: %d:%02d. Remaining time: %d:%02d\n", mins, secs, rmins, rsecs );
	else
		gi.LocClient_Print( ent, PRINT_HIGH, "Elapsed time: %d:%02d\n", mins, secs );
}

void Cmd_Roundtimeleft_f(edict_t * ent)
{
	int remaining;

	if(!teamplay->value) {
		gi.LocClient_Print(ent, PRINT_HIGH, "This command need teamplay to be enabled\n");
		return;
	}

	if (!(gameSettings & GS_ROUNDBASED) || !team_round_going)
		return;

	if ((int)roundtimelimit->value <= 0)
		return;

	remaining = (roundtimelimit->value * 60) - (current_round_length/10);
	gi.LocClient_Print(ent, PRINT_HIGH, "There is %d:%02i left in this round\n", remaining / 60, remaining % 60);
}

void Cmd_Voice_f (edict_t * self)
{
	const char *s;
	char fullpath[MAX_QPATH];

	if (!use_voice->integer)
		return;

	s = gi.args ();
	//check if no sound is given
	if (!*s)
	{
		gi.LocClient_Print (self, PRINT_MEDIUM,
			"\nCommand needs argument, use voice <soundfile.wav>.\n");
		return;
	}
	if (strlen (s) > 32)
	{
		gi.LocClient_Print (self, PRINT_MEDIUM,
			"\nArgument is too long. Maximum length is 32 characters.\n");
		return;
	}
	// AQ2:TNG Disabled this message: why? -M
	if (strstr (s, ".."))
	{
		gi.LocClient_Print (self, PRINT_MEDIUM,
			"\nArgument must not contain \"..\".\n");
		return;
	}
	
	//check if player is dead
	if (!IS_ALIVE(self))
		return;

	strcpy(fullpath, PG_SNDPATH);
	strcat(fullpath, s);
	// SLIC2 Taking this out.
	/*if (radio_repeat->value)
	{
	if ((d = CheckForRepeat (self, s)) == false)
	return;
	}*/
	if (radio_max->value)
	{
		if (CheckForFlood (self)== false)
			return;
	}
	// AQ2:TNG Deathwatch - This should be IDLE not NORM
	gi.sound (self, CHAN_VOICE, gi.soundindex (fullpath), 1, ATTN_IDLE, 0);
	// AQ2:TNG END
}

void Cmd_WhereAmI_f( edict_t * self )
{
	char location[ 128 ] = "";
	bool found = GetPlayerLocation( self, location );

	if( found )
		gi.LocClient_Print( self, PRINT_MEDIUM, "Location: %s\n", location );
	else if( ! sv_cheats->value )
		gi.LocClient_Print( self, PRINT_MEDIUM, "Location unknown.\n" );

	if( sv_cheats->value )
	{
		gi.LocClient_Print( self, PRINT_MEDIUM, "Origin: %5.0f,%5.0f,%5.0f  Facing: %3.0f\n",
			self->s.origin[0], self->s.origin[1], self->s.origin[2], self->s.angles[1] );
	}
}


// Timing here may be broken, test!
void Cmd_Punch_f (edict_t * self)
{
	if (!use_punch->value || !IS_ALIVE(self) || self->client->resp.sniper_mode != SNIPER_1X)
		return;

	if (self->client->weaponstate != WEAPON_READY && self->client->weaponstate != WEAPON_END_MAG)
		return;
	
	float punch_delay = 0.5;

    if ((level.time.milliseconds() + punch_delay) > self->client->punch_framenum) {
        self->client->punch_framenum = level.time.milliseconds(); // Update the punch_framenum to the current time
        self->client->punch_desired = true;
    }
}

// void _Cmd_Rules_f (edict_t * self, const char *argument)
// {
// 	char section[32], mbuf[1024], *p, buf[30][INI_STR_LEN];
// 	int i, j = 0;
// 	ini_t ini;

// 	strcpy (mbuf, "\n");
// 	if (*argument)
// 		Q_strncpyz(section, argument, sizeof(section));
// 	else
// 		strcpy (section, "main");

// 	if (OpenIniFile (GAMEVERSION "/prules.ini", &ini))
// 	{
// 		i = ReadIniSection (&ini, section, buf, 30);
// 		while (j < i)
// 		{
// 			p = buf[j++];
// 			if (*p == '.')
// 				p++;
// 			Q_strncatz(mbuf, p, sizeof(mbuf));
// 			Q_strncatz(mbuf, "\n", sizeof(mbuf));
// 		}
// 		CloseIniFile (&ini);
// 	}
// 	if (!j)
// 		gi.LocClient_Print (self, PRINT_MEDIUM, "No rules on %s available\n", section);
// 	else
// 		gi.LocClient_Print (self, PRINT_MEDIUM, "%s", mbuf);
// }

// void Cmd_Rules_f (edict_t * self)
// {
// 	const char *s;

// 	s = gi.args ();
// 	_Cmd_Rules_f (self, s);
// }

static void Cmd_Ent_Count_f (edict_t * ent)
{
	int x = 0;
	edict_t *e;

	for (e = g_edicts; e < &g_edicts[globals.num_edicts]; e++)
	{
		if (e->inuse)
			x++;
	}

	gi.LocClient_Print(ent, PRINT_HIGH, "%d entities counted\n", x);
}

void _SetSniper(edict_t * ent, int zoom)
{
	int desired_fov, sniper_mode, oldmode;

	switch (zoom) {
	default:
	case 1:
		desired_fov = SNIPER_FOV1;
		sniper_mode = SNIPER_1X;
		break;
	case 2:
		desired_fov = SNIPER_FOV2;
		sniper_mode = SNIPER_2X;
		break;
	case 4:
		desired_fov = SNIPER_FOV4;
		sniper_mode = SNIPER_4X;
		break;
	case 6:
		desired_fov = SNIPER_FOV6;
		sniper_mode = SNIPER_6X;
		break;
	}

	oldmode = ent->client->resp.sniper_mode;

	if (sniper_mode == oldmode)
		return;

	//Moved here, no need to make sound if zoom isnt changed -M
	gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/lensflik.wav"), 1, ATTN_NORM, 0);

	ent->client->resp.sniper_mode = sniper_mode;
	ent->client->desired_fov = desired_fov;

	if (sniper_mode == SNIPER_1X && ent->client->pers.weapon)
		ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);
	//show the model if switching to 1x

	if (oldmode == SNIPER_1X && ent->client->weaponstate != WEAPON_RELOADING) {
		//do idleness stuff when switching from 1x, see function below
		ent->client->weaponstate = WEAPON_BUSY;
		// if(zoom_comp->value) {
		// 	ent->client->idle_weapon = calc_zoom_comp(ent);
		// } else {
		// 	ent->client->idle_weapon = 6;
		// }
		ent->client->ps.gunframe = 22;
	}
}

//tempfile END

void Cmd_New_Weapon_f(edict_t * ent)
{
	ent->client->weapon_attempts++;
	if (ent->client->weapon_attempts == 1)
		Cmd_Weapon_f(ent);
}

int _SniperMode(edict_t *ent)
{
	switch (ent->client->desired_zoom) { //lets update old desired zoom
	case 1:
		return SNIPER_1X;
	case 2:
		return SNIPER_2X;
	case 4:
		return SNIPER_4X;
	case 6:
		return SNIPER_6X;
	}
	return ent->client->resp.sniper_mode;
}

void _ZoomIn(edict_t * ent, bool overflow)
{
	switch (_SniperMode(ent)) {
	case SNIPER_1X:
		ent->client->desired_zoom = 2;
		break;
	case SNIPER_2X:
		ent->client->desired_zoom = 4;
		break;
	case SNIPER_4X:
		ent->client->desired_zoom = 6;
		break;
	case SNIPER_6X:
		if (overflow)
			ent->client->desired_zoom = 1;
		break;
	}
}

void _ZoomOut(edict_t * ent, bool overflow)
{
	switch (_SniperMode(ent)) {
	case SNIPER_1X:
		if (overflow)
			ent->client->desired_zoom = 6;
		break;
	case SNIPER_2X:
		ent->client->desired_zoom = 1;
		break;
	case SNIPER_4X:
		ent->client->desired_zoom = 2;
		break;
	case SNIPER_6X:
		ent->client->desired_zoom = 4;
		break;
	}
}

// void Cmd_NextMap_f(edict_t * ent)
// {
// 	int map_num = -1;
// 	const char *next_map = level.nextmap;
// 	const char *rot_type = "in rotation";
// 	votelist_t *voted = NULL;

// 	if( next_map[0] )
// 		rot_type = "selected";
// 	else if( vrot->value && ((voted = MapWithMostAllVotes())) )
// 	{
// 		next_map = voted->mapname;
// 		rot_type = "by votes";
// 	}

// 	if( next_map[0] )
// 	{
// 		int i;
// 		for( i = 0; i < num_maps; i ++ )
// 		{
// 			if( Q_stricmp( map_rotation[i], next_map ) == 0 )
// 			{
// 				map_num = i;
// 				break;
// 			}
// 		}
// 	}
// 	else if( num_maps )
// 	{
// 		map_num = (cur_map + (rrot->value ? rand_map : 1)) % num_maps;
// 		next_map = map_rotation[ map_num ];
// 		rot_type = rrot->value ? "randomly" : "in rotation";
// 	}

// 	if( DMFLAGS(DF_SAME_LEVEL) )
// 	{
// 		map_num = cur_map;
// 		next_map = level.mapname;
// 		rot_type = "repeated";
// 	}

// 	gi.LocClient_Print( ent, PRINT_HIGH, "Next map %s is %s (%i/%i).\n", rot_type, next_map, map_num+1, num_maps );
// }

void Cmd_Weapon_f(edict_t * ent)
{
	int dead;

	if (!ent->client->pers.weapon)
		return;

	dead = !IS_ALIVE(ent);

	ent->client->weapon_attempts--;
	if (ent->client->weapon_attempts < 0)
		ent->client->weapon_attempts = 0;

	if (ent->client->bandaging || ent->client->bandage_stopped) {
		if (!ent->client->weapon_after_bandage_warned) {
			ent->client->weapon_after_bandage_warned = true;
			gi.LocClient_Print(ent, PRINT_HIGH, "You'll get to your weapon when you're done bandaging!\n");
		}
		ent->client->weapon_attempts++;
		return;
	}

	ent->client->weapon_after_bandage_warned = false;

	if (ent->client->weaponstate == WEAPON_FIRING || ent->client->weaponstate == WEAPON_BUSY)
	{
		//gi.LocClient_Print(ent, PRINT_HIGH, "Try again when you aren't using your weapon.\n");
		ent->client->weapon_attempts++;
		return;
	}

	switch(ent->client->pers.weapon->id) {
	case IT_WEAPON_MK23:
		if (!dead)
			gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/click.wav"), 1, ATTN_NORM, 0);
		ent->client->pers.mk23_mode = !(ent->client->pers.mk23_mode);
		if (ent->client->pers.mk23_mode)
			gi.LocClient_Print(ent, PRINT_HIGH, "MK23 Pistol set for semi-automatic action\n");
		else
			gi.LocClient_Print(ent, PRINT_HIGH, "MK23 Pistol set for automatic action\n");
		break;
	case IT_WEAPON_MP5:
		if (!dead)
			gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/click.wav"), 1, ATTN_NORM, 0);
		ent->client->pers.mp5_mode = !(ent->client->pers.mp5_mode);
		if (ent->client->pers.mp5_mode)
			gi.LocClient_Print(ent, PRINT_HIGH, "MP5 set to 3 Round Burst mode\n");
		else
			gi.LocClient_Print(ent, PRINT_HIGH, "MP5 set to Full Automatic mode\n");
		break;
	case IT_WEAPON_M4:
		if (!dead)
			gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/click.wav"), 1, ATTN_NORM, 0);
		ent->client->pers.m4_mode = !(ent->client->pers.m4_mode);
		if (ent->client->pers.m4_mode)
			gi.LocClient_Print(ent, PRINT_HIGH, "M4 set to 3 Round Burst mode\n");
		else
			gi.LocClient_Print(ent, PRINT_HIGH, "M4 set to Full Automatic mode\n");
		break;
	case IT_WEAPON_SNIPER:
		if (dead)
			return;
		
		if (!ent->client->desired_zoom)
			_ZoomIn(ent, true);	// standard behaviour

		_SetSniper(ent, ent->client->desired_zoom);
		ent->client->desired_zoom = 0;
		break;
	case IT_WEAPON_HANDCANNON:
		// AQ2:TNG Deathwatch - Single Barreled HC
		if(!hc_single->value)
			return;

		if (!dead)
			gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/click.wav"), 1, ATTN_NORM, 0);

		ent->client->pers.hc_mode = !(ent->client->pers.hc_mode);
		if (ent->client->pers.hc_mode)
			gi.LocClient_Print(ent, PRINT_HIGH, "Single Barreled Handcannon\n");
		else
			gi.LocClient_Print(ent, PRINT_HIGH, "Double Barreled Handcannon\n");
		// AQ2:TNG End
		break;
	case IT_WEAPON_KNIFE:
		if (dead)
			return;
		if (ent->client->weaponstate == WEAPON_READY) {
			ent->client->pers.knife_mode = !(ent->client->pers.knife_mode);
			ent->client->weaponstate = WEAPON_ACTIVATING;
			if (ent->client->pers.knife_mode) {
				gi.LocClient_Print(ent, PRINT_HIGH, "Switching to throwing\n");
				ent->client->ps.gunframe = 0;
			} else {
				gi.LocClient_Print(ent, PRINT_HIGH, "Switching to slashing\n");
				ent->client->ps.gunframe = 106;
			}
		}
		break;
	case IT_WEAPON_GRENADES:
		if (ent->client->pers.grenade_mode == 0) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Prepared to make a medium range throw\n");
			ent->client->pers.grenade_mode = 1;
		} else if (ent->client->pers.grenade_mode == 1) {
			gi.LocClient_Print(ent, PRINT_HIGH, "Prepared to make a long range throw\n");
			ent->client->pers.grenade_mode = 2;
		} else {
			gi.LocClient_Print(ent, PRINT_HIGH, "Prepared to make a short range throw\n");
			ent->client->pers.grenade_mode = 0;
		}
		break;
	}
}

void Cmd_IR_f(edict_t * ent)
{
	int band = 0;

	if (!ir->value) {
		gi.LocClient_Print(ent, PRINT_HIGH, "IR vision not enabled on this server.\n");
		return;
	}
	if (INV_AMMO(ent, BAND_NUM))
		band = 1;

	ent->client->pers.irvision = !ent->client->pers.irvision;
	if (ent->client->pers.irvision == 0)
	{
		if (band)
			gi.LocClient_Print(ent, PRINT_HIGH, "IR vision disabled.\n");
		else
			gi.LocClient_Print(ent, PRINT_HIGH, "IR vision will be disabled when you get a bandolier.\n");
	}
	else
	{
		if (band)
			gi.LocClient_Print(ent, PRINT_HIGH, "IR vision enabled.\n");
		else
			gi.LocClient_Print(ent, PRINT_HIGH, "IR vision will be enabled when you get a bandolier.\n");
	}
}

void Cmd_Lens_f(edict_t * ent)
{
	int nArg;
	char args[8];

	if (ent->client->pers.weapon->id != IT_WEAPON_SNIPER)
		return;

	nArg = atoi(gi.args());

	if (nArg == 0) {
		Q_strlcpy(args, gi.args(), sizeof(args));
		//perhaps in or out? let's see.
		if (Q_strcasecmp(args, "in") == 0)
			_ZoomIn(ent, false);
		else if (Q_strcasecmp(args, "out") == 0)
			_ZoomOut(ent, false);
		else
			_ZoomIn(ent, true);

		if(!ent->client->desired_zoom)
			return;
	}
	else if ((nArg == 1) || (!(nArg % 2) && (nArg <= 6)))
		ent->client->desired_zoom = nArg;
	else
		_ZoomIn(ent, true);

	if(ent->client->weapon_attempts > 0)
		return; //Already waiting to change the zoom, otherwise it
				//first change to desired zoom and then usual zoomin -M

	ent->client->weapon_attempts++;
	if (ent->client->weapon_attempts == 1)
		Cmd_Weapon_f(ent);
}

/*
=================
ClientCommand
=================
*/
void ClientCommand(edict_t *ent)
{
	const char *cmd;

	if (!ent->client)
		return; // not fully in game yet

	cmd = gi.argv(0);

	if (Q_strcasecmp(cmd, "players") == 0)
	{
		Cmd_Players_f(ent);
		return;
	}
	// [Paril-KEX] these have to go through the lobby system
#ifndef KEX_Q2_GAME
	if (Q_strcasecmp(cmd, "say") == 0)
	{
		Cmd_Say_f(ent, false);
		return;
	}
	if (Q_strcasecmp(cmd, "say_team") == 0 || Q_strcasecmp(cmd, "steam") == 0)
	{
		if (G_TeamplayEnabled())
			CTFSay_Team(ent, gi.args());
		else
			Cmd_Say_f(ent, false);
		return;
	}
#endif
	if (Q_strcasecmp(cmd, "score") == 0)
	{
		Cmd_Score_f(ent);
		return;
	}
	if (Q_strcasecmp(cmd, "help") == 0)
	{
		Cmd_Help_f(ent);
		return;
	}
	if (Q_strcasecmp(cmd, "listmonsters") == 0)
	{
		Cmd_ListMonsters_f(ent);
		return;
	}

	if (level.intermissiontime)
		return;
	
	if ( Q_strcasecmp( cmd, "target" ) == 0 )
		Cmd_Target_f( ent );
	else if ( Q_strcasecmp( cmd, "use" ) == 0 || Q_strcasecmp( cmd, "use_only" ) == 0 ||
		Q_strcasecmp( cmd, "use_index" ) == 0 || Q_strcasecmp( cmd, "use_index_only" ) == 0 )
		Cmd_Use_f( ent );
	else if ( Q_strcasecmp( cmd, "drop" ) == 0 ||
		Q_strcasecmp( cmd, "drop_index" ) == 0 )
		Cmd_Drop_f( ent );
	else if ( Q_strcasecmp( cmd, "give" ) == 0 )
		Cmd_Give_f( ent );
	else if ( Q_strcasecmp( cmd, "god" ) == 0 )
		Cmd_God_f( ent );
	else if (Q_strcasecmp(cmd, "immortal") == 0)
		Cmd_Immortal_f(ent);
	else if ( Q_strcasecmp( cmd, "setpoi" ) == 0 )
		Cmd_SetPOI_f( ent );
	else if ( Q_strcasecmp( cmd, "checkpoi" ) == 0 )
		Cmd_CheckPOI_f( ent );
	// Paril: cheats to help with dev
	else if ( Q_strcasecmp( cmd, "spawn" ) == 0 )
		Cmd_Spawn_f( ent );
	else if ( Q_strcasecmp( cmd, "teleport" ) == 0 )
		Cmd_Teleport_f( ent );
	else if ( Q_strcasecmp( cmd, "notarget" ) == 0 )
		Cmd_Notarget_f( ent );
	else if ( Q_strcasecmp( cmd, "novisible" ) == 0 )
		Cmd_Novisible_f( ent );
	else if ( Q_strcasecmp( cmd, "alertall" ) == 0 )
		Cmd_AlertAll_f( ent );
	else if ( Q_strcasecmp( cmd, "noclip" ) == 0 )
		Cmd_Noclip_f( ent );
	else if ( Q_strcasecmp( cmd, "inven" ) == 0 )
		Cmd_Inven_f( ent );
	else if ( Q_strcasecmp( cmd, "invnext" ) == 0 )
		SelectNextItem( ent, IF_ANY );
	else if ( Q_strcasecmp( cmd, "invprev" ) == 0 )
		SelectPrevItem( ent, IF_ANY );
	else if ( Q_strcasecmp( cmd, "invnextw" ) == 0 )
		SelectNextItem( ent, IF_WEAPON );
	else if ( Q_strcasecmp( cmd, "invprevw" ) == 0 )
		SelectPrevItem( ent, IF_WEAPON );
	else if ( Q_strcasecmp( cmd, "invnextp" ) == 0 )
		SelectNextItem( ent, IF_POWERUP );
	else if ( Q_strcasecmp( cmd, "invprevp" ) == 0 )
		SelectPrevItem( ent, IF_POWERUP );
	else if ( Q_strcasecmp( cmd, "invuse" ) == 0 )
		Cmd_InvUse_f( ent );
	else if ( Q_strcasecmp( cmd, "invdrop" ) == 0 )
		Cmd_InvDrop_f( ent );
	else if ( Q_strcasecmp( cmd, "weapprev" ) == 0 )
		Cmd_WeapPrev_f( ent );
	else if ( Q_strcasecmp( cmd, "weapnext" ) == 0 )
		Cmd_WeapNext_f( ent );
	else if ( Q_strcasecmp( cmd, "weaplast" ) == 0 || Q_strcasecmp( cmd, "lastweap" ) == 0 )
		Cmd_WeapLast_f( ent );
	else if ( Q_strcasecmp( cmd, "kill" ) == 0 )
		Cmd_Kill_f( ent );
	else if ( Q_strcasecmp( cmd, "kill_ai" ) == 0 )
		Cmd_Kill_AI_f( ent );
	else if ( Q_strcasecmp( cmd, "where" ) == 0 )
		Cmd_Where_f( ent );
	else if ( Q_strcasecmp( cmd, "clear_ai_enemy" ) == 0 )
		Cmd_Clear_AI_Enemy_f( ent );
	else if (Q_strcasecmp(cmd, "putaway") == 0)
		Cmd_PutAway_f(ent);
	else if (Q_strcasecmp(cmd, "wave") == 0)
		Cmd_Wave_f(ent);
	else if (Q_strcasecmp(cmd, "playerlist") == 0)
		Cmd_PlayerList_f(ent);
	// ZOID
	else if (Q_strcasecmp(cmd, "team") == 0)
		CTFTeam_f(ent);
	else if (Q_strcasecmp(cmd, "id") == 0)
		CTFID_f(ent);
	else if (Q_strcasecmp(cmd, "yes") == 0)
		CTFVoteYes(ent);
	else if (Q_strcasecmp(cmd, "no") == 0)
		CTFVoteNo(ent);
	else if (Q_strcasecmp(cmd, "ready") == 0)
		CTFReady(ent);
	else if (Q_strcasecmp(cmd, "notready") == 0)
		CTFNotReady(ent);
	else if (Q_strcasecmp(cmd, "ghost") == 0)
		CTFGhost(ent);
	else if (Q_strcasecmp(cmd, "admin") == 0)
		CTFAdmin(ent);
	else if (Q_strcasecmp(cmd, "stats") == 0)
		CTFStats(ent);
	else if (Q_strcasecmp(cmd, "warp") == 0)
		CTFWarp(ent);
	else if (Q_strcasecmp(cmd, "boot") == 0)
		CTFBoot(ent);
	else if (Q_strcasecmp(cmd, "playerlist") == 0)
		CTFPlayerList(ent);
	else if (Q_strcasecmp(cmd, "observer") == 0)
		CTFObserver(ent);
	// ZOID
	else if (Q_strcasecmp(cmd, "switchteam") == 0)
		Cmd_Switchteam_f(ent);
	// Action add
	else if (Q_strcasecmp(cmd, "streak") == 0)
		Cmd_Streak_f(ent);
	else if (Q_strcasecmp(cmd, "reload") == 0)
		Cmd_New_Reload_f(ent);
	else if (Q_strcasecmp(cmd, "weapon") == 0)
		Cmd_New_Weapon_f(ent);
	else if (Q_strcasecmp(cmd, "opendoor") == 0)
		Cmd_OpenDoor_f(ent);
	else if (Q_strcasecmp(cmd, "bandage") == 0)
		Cmd_Bandage_f(ent);
	else if (Q_strcasecmp(cmd, "irvision") == 0)
		Cmd_IR_f(ent);
	else if (Q_strcasecmp(cmd, "radio") == 0)
		Cmd_Radio_f(ent);
	else if (Q_strcasecmp(cmd, "radiogender") == 0)
		Cmd_Radiogender_f(ent);
	else if (Q_strcasecmp(cmd, "radio_power") == 0)
		Cmd_Radio_power_f(ent);
	else if (Q_strcasecmp(cmd, "radio_team") == 0)
		Cmd_Radioteam_f(ent);
	else if (Q_strcasecmp(cmd, "channel") == 0)
		Cmd_Channel_f(ent);
	else if (Q_strcasecmp(cmd, "motd") == 0)
		PrintMOTD(ent);
	else if (Q_strcasecmp(cmd, "deny") == 0)
		Cmd_Deny_f(ent);
	else if (Q_strcasecmp(cmd, "choose") == 0)
		Cmd_Choose_f(ent);
	else if (Q_strcasecmp(cmd, "tkok") == 0)
		Cmd_TKOk(ent);
	else if (Q_strcasecmp(cmd, "forgive") == 0)
		Cmd_TKOk(ent);
	else if (Q_strcasecmp(cmd, "ff") == 0)
		Cmd_FF_f(ent);
	else if (Q_strcasecmp(cmd, "time") == 0)
		Cmd_Time(ent);
	else if (Q_strcasecmp(cmd, "voice") == 0)
		Cmd_Voice_f(ent);
	else if (Q_strcasecmp(cmd, "whereami") == 0)
		Cmd_WhereAmI_f(ent);
	// else if (Q_strcasecmp(cmd, "setflag1") == 0)
	// 	Cmd_SetFlag1_f(ent);
	// else if (Q_strcasecmp(cmd, "setflag2") == 0)
	// 	Cmd_SetFlag2_f(ent);
	// else if (Q_strcasecmp(cmd, "saveflags") == 0)
	// 	Cmd_SaveFlags_f(ent);
	else if (Q_strcasecmp(cmd, "punch") == 0)
		Cmd_Punch_f(ent);
	// else if (Q_strcasecmp(cmd, "rules") == 0)
	// 	Cmd_Rules_f(ent);
	else if (Q_strcasecmp(cmd, "lens") == 0)
		Cmd_Lens_f(ent);
	// else if (Q_strcasecmp(cmd, "nextmap") == 0)
	// 	Cmd_NextMap_f(ent);
	// Matchmode stuff
	// else if (Q_strcasecmp(cmd, "sub") == 0)
	// 	Cmd_Sub_f(ent);
	// else if (Q_strcasecmp(cmd, "captain") == 0)
	// 	Cmd_Captain_f(ent);
	// else if (Q_strcasecmp(cmd, "ready") == 0)
	// 	Cmd_Ready_f(ent);
	// else if (Q_strcasecmp(cmd, "teamname") == 0)
	// 	Cmd_Teamname_f(ent);
	// else if (Q_strcasecmp(cmd, "teamskin") == 0)
	// 	Cmd_Teamskin_f(ent);
	// else if (Q_strcasecmp(cmd, "lock") == 0)
	// 	Cmd_LockTeam_f(ent);
	// else if (Q_strcasecmp(cmd, "unlock") == 0)
	// 	Cmd_UnlockTeam_f(ent);
	else if (Q_strcasecmp(cmd, "entcount") == 0)
		Cmd_Ent_Count_f(ent);
	// else if (Q_strcasecmp(cmd, "stats") == 0)
	// 	Cmd_PrintStats_f(ent);
	// else if (Q_strcasecmp(cmd, "flashlight") == 0)
	// 	Use_Flashlight(ent);
	// else if (Q_strcasecmp(cmd, "matchadmin") == 0)
	// 	Cmd_SetAdmin_f(ent);
	else if (Q_strcasecmp(cmd, "roundtimeleft") == 0)
		Cmd_Roundtimeleft_f(ent);
	// else if (Q_strcasecmp(cmd, "autorecord") == 0)
	// 	Cmd_AutoRecord_f(ent);
	// else if (Q_strcasecmp(cmd, "stat_mode") == 0)
	// 	Cmd_Statmode_f(ent);
	// else if (Q_strcasecmp(cmd, "cmd_stat_mode") == 0)
	// 	Cmd_Statmode_f(ent);
	// else if (Q_strcasecmp(cmd, "ghost") == 0)
	// 	Cmd_Ghost_f(ent);
	// else if (Q_strcasecmp(cmd, "resetscores") == 0)
	// 	Cmd_ResetScores_f(ent);
	// else if (Q_strcasecmp(cmd, "gamesettings") == 0)
	// 	Cmd_PrintSettings_f(ent);
	// else if (Q_strcasecmp(cmd, "follow") == 0)
	// 	Cmd_Follow_f(ent);
	//vote stuff
	// else if (Q_strcasecmp(cmd, "menu") == 0)
	// 	Cmd_Menu_f(ent);
	// else if (Q_strcasecmp(cmd, "votemap") == 0)
	// 	Cmd_Votemap_f(ent);
	// else if (Q_strcasecmp(cmd, "maplist") == 0)
	// 	Cmd_Maplist_f(ent);
	// else if (Q_strcasecmp(cmd, "votekick") == 0)
	// 	Cmd_Votekick_f(ent);
	// else if (Q_strcasecmp(cmd, "votekicknum") == 0)
	// 	Cmd_Votekicknum_f(ent);
	// else if (Q_strcasecmp(cmd, "kicklist") == 0)
	// 	Cmd_Kicklist_f(ent);
	// else if (Q_strcasecmp(cmd, "ignore") == 0)
	// 	Cmd_Ignore_f(ent);
	// else if (Q_strcasecmp(cmd, "ignorenum") == 0)
	// 	Cmd_Ignorenum_f(ent);
	// else if (Q_strcasecmp(cmd, "ignorelist") == 0)
	// 	Cmd_Ignorelist_f(ent);
	// else if (Q_strcasecmp(cmd, "ignoreclear") == 0)
	// 	Cmd_Ignoreclear_f(ent);
	// else if (Q_strcasecmp(cmd, "ignorepart") == 0)
	// 	Cmd_IgnorePart_f(ent);
	// else if (Q_strcasecmp(cmd, "voteconfig") == 0)
	// 	Cmd_Voteconfig_f(ent);
	// else if (Q_strcasecmp(cmd, "configlist") == 0)
	// 	Cmd_Configlist_f(ent);
	// else if (Q_strcasecmp(cmd, "votescramble") == 0)
	// 	Cmd_Votescramble_f(ent);
	// Espionage, aliased command so it's easy to remember
	// else if (Q_strcasecmp(cmd, "volunteer") == 0)
	// 	Cmd_Volunteer_f(ent);
	// else if (Q_strcasecmp(cmd, "leader") == 0)
	// 	Cmd_Volunteer_f(ent);
	// End Action add
#ifndef KEX_Q2_GAME
	else // anything that doesn't match a command will be a chat
		Cmd_Say_f(ent, true);
#else
	// anything that doesn't match a command will inform them
	else
		gi.LocClient_Print(ent, PRINT_HIGH, "invalid game command \"{}\"\n", gi.argv(0));
#endif
}
