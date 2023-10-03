// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"
#include "m_player.h"

void SelectNextItem(edict_t *ent, item_flags_t itflags, bool menu = true)
{
	gclient_t *cl;
	item_id_t  i, index;
	gitem_t	*it;

	cl = ent->client;

	// ZOID
	if (menu && cl->menu)
	{
		PMenu_Next(ent);
		return;
	}
	else if (menu && cl->chase_target)
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

	SelectNextItem(ent, IF_ANY, false);
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
		if (give_all)
			SpawnAndGiveItem(ent, IT_ITEM_PACK);

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
		ent->client->pers.inventory[IT_ARMOR_JACKET] = 0;
		ent->client->pers.inventory[IT_ARMOR_COMBAT] = 0;
		ent->client->pers.inventory[IT_ARMOR_BODY] = GetItemByIndex(IT_ARMOR_BODY)->armor_info->max_count;

		if (!give_all)
			return;
	}

	if (give_all)
	{
		SpawnAndGiveItem(ent, IT_ITEM_POWER_SHIELD);

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

		G_CheckPowerArmor(ent);
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

		if ((other->svflags & SVF_MONSTER) && other->think)
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

	if (gi.argc() >= 4)
	{
		float pitch = (float) atof(gi.argv(4));
		float yaw = (float) atof(gi.argv(5));
		float roll = (float) atof(gi.argv(6));
		vec3_t ang { pitch, yaw, roll };

		ent->client->ps.pmove.delta_angles = ( ang - ent->client->resp.cmd_angles );
		ent->client->ps.viewangles = {};
		ent->client->v_angle = {};
	}

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
	if (Q_strcasecmp(gi.args(), "tech") == 0)
	{
		it = CTFWhat_Tech(ent);

		if (it)
		{
			it->drop(ent, it);
			ValidateSelectedItem(ent);
		}

		return;
	}
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
	if (G_TeamplayEnabled() && cl->resp.ctf_team == CTF_NOTEAM)
	{
		CTFOpenJoinMenu(ent);
		return;
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
	fmt::format_to( std::back_inserter( location ), FMT_STRING( "{:.1f} {:.1f} {:.1f} {:.1f} {:.1f} {:.1f}\n" ), origin[ 0 ], origin[ 1 ], origin[ 2 ], ent->client->ps.viewangles[0], ent->client->ps.viewangles[1], ent->client->ps.viewangles[2] );
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

	const char *other_notify_msg = nullptr, *other_notify_none_msg = nullptr;

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
		other_notify_msg = "$g_flipoff_other";
		other_notify_none_msg = "$g_flipoff_none";
		if (do_animate)
		{
			ent->s.frame = FRAME_flip01 - 1;
			ent->client->anim_end = FRAME_flip12;
		}
		break;
	case GESTURE_SALUTE:
		other_notify_msg = "$g_salute_other";
		other_notify_none_msg = "$g_salute_none";
		if (do_animate)
		{
			ent->s.frame = FRAME_salute01 - 1;
			ent->client->anim_end = FRAME_salute11;
		}
		break;
	case GESTURE_TAUNT:
		other_notify_msg = "$g_taunt_other";
		other_notify_none_msg = "$g_taunt_none";
		if (do_animate)
		{
			ent->s.frame = FRAME_taunt01 - 1;
			ent->client->anim_end = FRAME_taunt17;
		}
		break;
	case GESTURE_WAVE:
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
				gi.WriteShort(level.pic_ping);
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
		ctfteam_t best_team;

		for (uint32_t i = 1; i <= game.maxclients; i++)
		{
			player = &g_edicts[i];

			// NB: we are counting ourselves in this one, unlike
			// the other assign team func
			if (!player->inuse)
				continue;

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
		}

		if (team1count < team2count)
			best_team = CTF_TEAM1;
		else
			best_team = CTF_TEAM2;

		if (ent->client->resp.ctf_team != best_team)
		{
			////
			ent->svflags = SVF_NONE;
			ent->flags &= ~FL_GODMODE;
			ent->client->resp.ctf_team = best_team;
			ent->client->resp.ctf_state = 0;
			char value[MAX_INFO_VALUE] = { 0 };
			gi.Info_ValueForKey(ent->client->pers.userinfo, "skin", value, sizeof(value));
			CTFAssignSkin(ent, value);

			// if anybody has a menu open, update it immediately
			CTFDirtyTeamMenu();

			if (ent->solid == SOLID_NOT)
			{
				// spectator
				PutClientInServer(ent);

				G_PostRespawn(ent);

				gi.LocBroadcast_Print(PRINT_HIGH, "$g_joined_team",
					ent->client->pers.netname, CTFTeamName(best_team));
				return;
			}

			ent->health = 0;
			player_die(ent, ent, ent, 100000, vec3_origin, { MOD_SUICIDE, true });

			// don't even bother waiting for death frames
			ent->deadflag = true;
			respawn(ent);

			ent->client->resp.score = 0;

			gi.LocBroadcast_Print(PRINT_HIGH, "$g_changed_team",
				ent->client->pers.netname, CTFTeamName(best_team));
		}

		return;
	}

	if (ent->client->resp.ctf_team != CTF_NOTEAM)
		CTFObserver(ent);

	if (!ent->client->menu)
		CTFOpenJoinMenu(ent);
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
#ifndef KEX_Q2_GAME
	else // anything that doesn't match a command will be a chat
		Cmd_Say_f(ent, true);
#else
	// anything that doesn't match a command will inform them
	else
		gi.LocClient_Print(ent, PRINT_HIGH, "invalid game command \"{}\"\n", gi.argv(0));
#endif
}
