// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"
#include "m_player.h"
#include "bots/bot_includes.h"

void SP_misc_teleporter_dest(edict_t *ent);

THINK(info_player_start_drop) (edict_t *self) -> void
{
	// allow them to drop
	self->solid = SOLID_TRIGGER;
	self->movetype = MOVETYPE_TOSS;
	self->mins = PLAYER_MINS;
	self->maxs = PLAYER_MAXS;
	gi.linkentity(self);
}

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
The normal starting point for a level.
*/
void SP_info_player_start(edict_t *self)
{
	// fix stuck spawn points
	if (gi.trace(self->s.origin, PLAYER_MINS, PLAYER_MAXS, self->s.origin, self, MASK_SOLID).startsolid)
		G_FixStuckObject(self, self->s.origin);

	// [Paril-KEX] on n64, since these can spawn riding elevators,
	// allow them to "ride" the elevators so respawning works
	if (level.is_n64)
	{
		self->think = info_player_start_drop;
		self->nextthink = level.time + FRAME_TIME_S;
	}
}

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for deathmatch games
*/
void SP_info_player_deathmatch(edict_t *self)
{
	if (!deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}
	SP_misc_teleporter_dest(self);
}

/*QUAKED info_player_coop (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for coop games
*/
void SP_info_player_coop(edict_t *self)
{
	if (!coop->integer)
	{
		G_FreeEdict(self);
		return;
	}

	SP_info_player_start(self);
}

/*QUAKED info_player_coop_lava (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for coop games on rmine2 where lava level
needs to be checked
*/
void SP_info_player_coop_lava(edict_t *self)
{
	if (!coop->integer)
	{
		G_FreeEdict(self);
		return;
	}

	// fix stuck spawn points
	if (gi.trace(self->s.origin, PLAYER_MINS, PLAYER_MAXS, self->s.origin, self, MASK_SOLID).startsolid)
		G_FixStuckObject(self, self->s.origin);
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The deathmatch intermission point will be at one of these
Use 'angles' instead of 'angle', so you can set pitch or roll as well as yaw.  'pitch yaw roll'
*/
void SP_info_player_intermission(edict_t *ent)
{
}

// [Paril-KEX] whether instanced items should be used or not
bool P_UseCoopInstancedItems()
{
	// squad respawn forces instanced items on, since we don't
	// want players to need to backtrack just to get their stuff.
	return g_coop_instanced_items->integer || g_coop_squad_respawn->integer;
}

//=======================================================================

void ClientObituary(edict_t *self, edict_t *inflictor, edict_t *attacker, mod_t mod)
{
	const char *base = nullptr;

	if (coop->integer && attacker->client)
		mod.friendly_fire = true;

	switch (mod.id)
	{
	case MOD_SUICIDE:
		base = "$g_mod_generic_suicide";
		break;
	case MOD_FALLING:
		base = "$g_mod_generic_falling";
		break;
	case MOD_CRUSH:
		base = "$g_mod_generic_crush";
		break;
	case MOD_WATER:
		base = "$g_mod_generic_water";
		break;
	case MOD_SLIME:
		base = "$g_mod_generic_slime";
		break;
	case MOD_LAVA:
		base = "$g_mod_generic_lava";
		break;
	case MOD_EXPLOSIVE:
	case MOD_BARREL:
		base = "$g_mod_generic_explosive";
		break;
	case MOD_EXIT:
		base = "$g_mod_generic_exit";
		break;
	case MOD_TARGET_LASER:
		base = "$g_mod_generic_laser";
		break;
	case MOD_TARGET_BLASTER:
		base = "$g_mod_generic_blaster";
		break;
	case MOD_BOMB:
	case MOD_SPLASH:
	case MOD_TRIGGER_HURT:
		base = "$g_mod_generic_hurt";
		break;
	// RAFAEL
	case MOD_GEKK:
	case MOD_BRAINTENTACLE:
		base = "$g_mod_generic_gekk";
		break;
	// RAFAEL
	default:
		base = nullptr;
		break;
	}

	if (attacker == self)
	{
		switch (mod.id)
		{
		case MOD_HELD_GRENADE:
			base = "$g_mod_self_held_grenade";
			break;
		case MOD_HG_SPLASH:
		case MOD_G_SPLASH:
			base = "$g_mod_self_grenade_splash";
			break;
		case MOD_R_SPLASH:
			base = "$g_mod_self_rocket_splash";
			break;
		case MOD_BFG_BLAST:
			base = "$g_mod_self_bfg_blast";
			break;
		// RAFAEL 03-MAY-98
		case MOD_TRAP:
			base = "$g_mod_self_trap";
			break;
			// RAFAEL
			// ROGUE
		case MOD_DOPPLE_EXPLODE:
			base = "$g_mod_self_dopple_explode";
			break;
			// ROGUE
		default:
			base = "$g_mod_self_default";
			break;
		}
	}

	// send generic/self
	if (base)
	{
		gi.LocBroadcast_Print(PRINT_MEDIUM, base, self->client->pers.netname);
		if (deathmatch->integer && !mod.no_point_loss)
		{
			self->client->resp.score--;

			if (teamplay->integer)
				G_AdjustTeamScore(self->client->resp.ctf_team, -1);
		}
		self->enemy = nullptr;
		return;
	}

	// has a killer
	self->enemy = attacker;
	if (attacker && attacker->client)
	{
		switch (mod.id)
		{
		case MOD_BLASTER:
			base = "$g_mod_kill_blaster";
			break;
		case MOD_SHOTGUN:
			base = "$g_mod_kill_shotgun";
			break;
		case MOD_SSHOTGUN:
			base = "$g_mod_kill_sshotgun";
			break;
		case MOD_MACHINEGUN:
			base = "$g_mod_kill_machinegun";
			break;
		case MOD_CHAINGUN:
			base = "$g_mod_kill_chaingun";
			break;
		case MOD_GRENADE:
			base = "$g_mod_kill_grenade";
			break;
		case MOD_G_SPLASH:
			base = "$g_mod_kill_grenade_splash";
			break;
		case MOD_ROCKET:
			base = "$g_mod_kill_rocket";
			break;
		case MOD_R_SPLASH:
			base = "$g_mod_kill_rocket_splash";
			break;
		case MOD_HYPERBLASTER:
			base = "$g_mod_kill_hyperblaster";
			break;
		case MOD_RAILGUN:
			base = "$g_mod_kill_railgun";
			break;
		case MOD_BFG_LASER:
			base = "$g_mod_kill_bfg_laser";
			break;
		case MOD_BFG_BLAST:
			base = "$g_mod_kill_bfg_blast";
			break;
		case MOD_BFG_EFFECT:
			base = "$g_mod_kill_bfg_effect";
			break;
		case MOD_HANDGRENADE:
			base = "$g_mod_kill_handgrenade";
			break;
		case MOD_HG_SPLASH:
			base = "$g_mod_kill_handgrenade_splash";
			break;
		case MOD_HELD_GRENADE:
			base = "$g_mod_kill_held_grenade";
			break;
		case MOD_TELEFRAG:
		case MOD_TELEFRAG_SPAWN:
			base = "$g_mod_kill_telefrag";
			break;
		// RAFAEL 14-APR-98
		case MOD_RIPPER:
			base = "$g_mod_kill_ripper";
			break;
		case MOD_PHALANX:
			base = "$g_mod_kill_phalanx";
			break;
		case MOD_TRAP:
			base = "$g_mod_kill_trap";
			break;
			// RAFAEL
			//===============
			// ROGUE
		case MOD_CHAINFIST:
			base = "$g_mod_kill_chainfist";
			break;
		case MOD_DISINTEGRATOR:
			base = "$g_mod_kill_disintegrator";
			break;
		case MOD_ETF_RIFLE:
			base = "$g_mod_kill_etf_rifle";
			break;
		case MOD_HEATBEAM:
			base = "$g_mod_kill_heatbeam";
			break;
		case MOD_TESLA:
			base = "$g_mod_kill_tesla";
			break;
		case MOD_PROX:
			base = "$g_mod_kill_prox";
			break;
		case MOD_NUKE:
			base = "$g_mod_kill_nuke";
			break;
		case MOD_VENGEANCE_SPHERE:
			base = "$g_mod_kill_vengeance_sphere";
			break;
		case MOD_DEFENDER_SPHERE:
			base = "$g_mod_kill_defender_sphere";
			break;
		case MOD_HUNTER_SPHERE:
			base = "$g_mod_kill_hunter_sphere";
			break;
		case MOD_TRACKER:
			base = "$g_mod_kill_tracker";
			break;
		case MOD_DOPPLE_EXPLODE:
			base = "$g_mod_kill_dopple_explode";
			break;
		case MOD_DOPPLE_VENGEANCE:
			base = "$g_mod_kill_dopple_vengeance";
			break;
		case MOD_DOPPLE_HUNTER:
			base = "$g_mod_kill_dopple_hunter";
			break;
			// ROGUE
			//===============
			// ZOID
		case MOD_GRAPPLE:
			base = "$g_mod_kill_grapple";
			break;
			// ZOID
		default:
			base = "$g_mod_kill_generic";
			break;
		}

		gi.LocBroadcast_Print(PRINT_MEDIUM, base, self->client->pers.netname, attacker->client->pers.netname);

		if (G_TeamplayEnabled())
		{
			// ZOID
			//  if at start and same team, clear.
			// [Paril-KEX] moved here so it's not an outlier in player_die.
			if (mod.id == MOD_TELEFRAG_SPAWN &&
				self->client->resp.ctf_state < 2 &&
				self->client->resp.ctf_team == attacker->client->resp.ctf_team)
			{
				self->client->resp.ctf_state = 0;
				return;
			}
		}

		// ROGUE
		if (gamerules->integer)
		{
			if (DMGame.Score)
			{
				if (mod.friendly_fire)
				{
					if (!mod.no_point_loss)
						DMGame.Score(attacker, self, -1, mod);
				}
				else
					DMGame.Score(attacker, self, 1, mod);
			}
			return;
		}
		// ROGUE

		if (deathmatch->integer)
		{
			if (mod.friendly_fire)
			{
				if (!mod.no_point_loss)
				{
					attacker->client->resp.score--;

					if (teamplay->integer)
						G_AdjustTeamScore(attacker->client->resp.ctf_team, -1);
				}
			}
			else
			{
				attacker->client->resp.score++;

				if (teamplay->integer)
					G_AdjustTeamScore(attacker->client->resp.ctf_team, 1);
			}
		}
		else if (!coop->integer)
			self->client->resp.score--;

		return;
	}

	gi.LocBroadcast_Print(PRINT_MEDIUM, "$g_mod_generic_died", self->client->pers.netname);
	if (deathmatch->integer && !mod.no_point_loss)
	// ROGUE
	{
		if (gamerules->integer)
		{
			if (DMGame.Score)
			{
				DMGame.Score(self, self, -1, mod);
			}
			return;
		}
		else
		{
			self->client->resp.score--;

			if (teamplay->integer)
				G_AdjustTeamScore(attacker->client->resp.ctf_team, -1);
		}
	}
	// ROGUE
}

void TossClientWeapon(edict_t *self)
{
	gitem_t *item;
	edict_t *drop;
	bool	 quad;
	// RAFAEL
	bool quadfire;
	// RAFAEL
	float spread;

	if (!deathmatch->integer)
		return;

	item = self->client->pers.weapon;
	if (item && g_instagib->integer)
		item = nullptr;
	if (item && !self->client->pers.inventory[self->client->pers.weapon->ammo])
		item = nullptr;
	if (item && !item->drop)
		item = nullptr;

	if (g_dm_no_quad_drop->integer)
		quad = false;
	else
		quad = (self->client->quad_time > (level.time + 1_sec));

	// RAFAEL
	if (g_dm_no_quadfire_drop->integer)
		quadfire = false;
	else
		quadfire = (self->client->quadfire_time > (level.time + 1_sec));
	// RAFAEL

	if (item && quad)
		spread = 22.5;
	// RAFAEL
	else if (item && quadfire)
		spread = 12.5;
	// RAFAEL
	else
		spread = 0.0;

	if (item)
	{
		self->client->v_angle[YAW] -= spread;
		drop = Drop_Item(self, item);
		self->client->v_angle[YAW] += spread;
		drop->spawnflags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
		drop->spawnflags &= ~SPAWNFLAG_ITEM_DROPPED;
		drop->svflags &= ~SVF_INSTANCED;
	}

	if (quad)
	{
		self->client->v_angle[YAW] += spread;
		drop = Drop_Item(self, GetItemByIndex(IT_ITEM_QUAD));
		self->client->v_angle[YAW] -= spread;
		drop->spawnflags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
		drop->spawnflags &= ~SPAWNFLAG_ITEM_DROPPED;
		drop->svflags &= ~SVF_INSTANCED;

		drop->touch = Touch_Item;
		drop->nextthink = self->client->quad_time;
		drop->think = G_FreeEdict;
	}

	// RAFAEL
	if (quadfire)
	{
		self->client->v_angle[YAW] += spread;
		drop = Drop_Item(self, GetItemByIndex(IT_ITEM_QUADFIRE));
		self->client->v_angle[YAW] -= spread;
		drop->spawnflags |= SPAWNFLAG_ITEM_DROPPED_PLAYER;
		drop->spawnflags &= ~SPAWNFLAG_ITEM_DROPPED;
		drop->svflags &= ~SVF_INSTANCED;

		drop->touch = Touch_Item;
		drop->nextthink = self->client->quadfire_time;
		drop->think = G_FreeEdict;
	}
	// RAFAEL
}

/*
==================
LookAtKiller
==================
*/
void LookAtKiller(edict_t *self, edict_t *inflictor, edict_t *attacker)
{
	vec3_t dir;

	if (attacker && attacker != world && attacker != self)
	{
		dir = attacker->s.origin - self->s.origin;
	}
	else if (inflictor && inflictor != world && inflictor != self)
	{
		dir = inflictor->s.origin - self->s.origin;
	}
	else
	{
		self->client->killer_yaw = self->s.angles[YAW];
		return;
	}
	// PMM - fixed to correct for pitch of 0
	if (dir[0])
		self->client->killer_yaw = 180 / PIf * atan2f(dir[1], dir[0]);
	else if (dir[1] > 0)
		self->client->killer_yaw = 90;
	else if (dir[1] < 0)
		self->client->killer_yaw = 270;
	else
		self->client->killer_yaw = 0;
}

/*
==================
player_die
==================
*/
DIE(player_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	PlayerTrail_Destroy(self);

	self->avelocity = {};

	self->takedamage = true;
	self->movetype = MOVETYPE_TOSS;

	self->s.modelindex2 = 0; // remove linked weapon model
							 // ZOID
	self->s.modelindex3 = 0; // remove linked ctf flag
							 // ZOID

	self->s.angles[0] = 0;
	self->s.angles[2] = 0;

	self->s.sound = 0;
	self->client->weapon_sound = 0;

	self->maxs[2] = -8;

	//	self->solid = SOLID_NOT;
	self->svflags |= SVF_DEADMONSTER;

	if (!self->deadflag)
	{
		self->client->respawn_time = ( level.time + 1_sec );
		if ( deathmatch->integer && g_dm_force_respawn_time->integer ) {
			self->client->respawn_time = ( level.time + gtime_t::from_sec( g_dm_force_respawn_time->value ) );
		}

		LookAtKiller(self, inflictor, attacker);
		self->client->ps.pmove.pm_type = PM_DEAD;
		ClientObituary(self, inflictor, attacker, mod);

		CTFFragBonuses(self, inflictor, attacker);
		// ZOID
		TossClientWeapon(self);
		// ZOID
		CTFPlayerResetGrapple(self);
		CTFDeadDropFlag(self);
		CTFDeadDropTech(self);
		// ZOID
		if (deathmatch->integer && !self->client->showscores)
			Cmd_Help_f(self); // show scores

		if (coop->integer && !P_UseCoopInstancedItems())
		{
			// clear inventory
			// this is kind of ugly, but it's how we want to handle keys in coop
			for (int n = 0; n < IT_TOTAL; n++)
			{
				if (coop->integer && (itemlist[n].flags & IF_KEY))
					self->client->resp.coop_respawn.inventory[n] = self->client->pers.inventory[n];
				self->client->pers.inventory[n] = 0;
			}
		}
	}

	if (gamerules->integer) // if we're in a dm game, alert the game
	{
		if (DMGame.PlayerDeath)
			DMGame.PlayerDeath(self, inflictor, attacker);
	}

	// remove powerups
	self->client->quad_time = 0_ms;
	self->client->invincible_time = 0_ms;
	self->client->breather_time = 0_ms;
	self->client->enviro_time = 0_ms;
	self->client->invisible_time = 0_ms;
	self->flags &= ~FL_POWER_ARMOR;

	// clear inventory
	if (G_TeamplayEnabled())
		self->client->pers.inventory.fill(0);

	// RAFAEL
	self->client->quadfire_time = 0_ms;
	// RAFAEL

	//==============
	// ROGUE stuff
	self->client->double_time = 0_ms;

	// if there's a sphere around, let it know the player died.
	// vengeance and hunter will die if they're not attacking,
	// defender should always die
	if (self->client->owned_sphere)
	{
		edict_t *sphere;

		sphere = self->client->owned_sphere;
		sphere->die(sphere, self, self, 0, vec3_origin, mod);
	}

	// if we've been killed by the tracker, GIB!
	if (mod.id == MOD_TRACKER)
	{
		self->health = -100;
		damage = 400;
	}

	// make sure no trackers are still hurting us.
	if (self->client->tracker_pain_time)
	{
		RemoveAttackingPainDaemons(self);
	}

	// if we got obliterated by the nuke, don't gib
	if ((self->health < -80) && (mod.id == MOD_NUKE))
		self->flags |= FL_NOGIB;

	// ROGUE
	//==============

	if (self->health < -40)
	{
		// PMM
		// don't toss gibs if we got vaped by the nuke
		if (!(self->flags & FL_NOGIB))
		{
			// pmm
			// gib
			gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

			// more meaty gibs for your dollar!
			if (deathmatch->integer && (self->health < -80))
				ThrowGibs(self, damage, { { 4, "models/objects/gibs/sm_meat/tris.md2" } });
			
			ThrowGibs(self, damage, { { 4, "models/objects/gibs/sm_meat/tris.md2" } });
			// PMM
		}
		self->flags &= ~FL_NOGIB;
		// pmm

		ThrowClientHead(self, damage);
		// ZOID
		self->client->anim_priority = ANIM_DEATH;
		self->client->anim_end = 0;
		// ZOID
		self->takedamage = false;
	}
	else
	{ // normal death
		if (!self->deadflag)
		{
			// start a death animation
			self->client->anim_priority = ANIM_DEATH;
			if (self->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				self->s.frame = FRAME_crdeath1 - 1;
				self->client->anim_end = FRAME_crdeath5;
			}
			else
			{
				switch (irandom(3))
				{
				case 0:
					self->s.frame = FRAME_death101 - 1;
					self->client->anim_end = FRAME_death106;
					break;
				case 1:
					self->s.frame = FRAME_death201 - 1;
					self->client->anim_end = FRAME_death206;
					break;
				case 2:
					self->s.frame = FRAME_death301 - 1;
					self->client->anim_end = FRAME_death308;
					break;
				}
			}
			static constexpr const char *death_sounds[] = {
				"*death1.wav",
				"*death2.wav",
				"*death3.wav",
				"*death4.wav"
			};
			gi.sound(self, CHAN_VOICE, gi.soundindex(random_element(death_sounds)), 1, ATTN_NORM, 0);
			self->client->anim_time = 0_ms;
		}
	}

	if (!self->deadflag)
	{
		if (coop->integer && (g_coop_squad_respawn->integer || g_coop_enable_lives->integer))
		{
			if (g_coop_enable_lives->integer && self->client->pers.lives)
			{
				self->client->pers.lives--;
				self->client->resp.coop_respawn.lives--;
			}

			bool allPlayersDead = true;

			for (auto player : active_players())
				if (player->health > 0 || (!level.deadly_kill_box && g_coop_enable_lives->integer && player->client->pers.lives > 0))
				{
					allPlayersDead = false;
					break;
				}

			if (allPlayersDead) // allow respawns for telefrags and weird shit
			{
				level.coop_level_restart_time = level.time + 5_sec;

				for (auto player : active_players())
					gi.LocCenter_Print(player, "$g_coop_lose");
			}
		
			// in 3 seconds, attempt a respawn or put us into
			// spectator mode
			if (!level.coop_level_restart_time)
				self->client->respawn_time = level.time + 3_sec;
		}
	}

	self->deadflag = true;

	gi.linkentity(self);
}

//=======================================================================

#include <string>
#include <sstream>

// [Paril-KEX]
static void Player_GiveStartItems(edict_t *ent, const char *ptr)
{
	char token_copy[MAX_TOKEN_CHARS];
	const char *token;

	while (*(token = COM_ParseEx(&ptr, ";")))
	{
		Q_strlcpy(token_copy, token, sizeof(token_copy));
		const char *ptr_copy = token_copy;

		const char *item_name = COM_Parse(&ptr_copy);
		gitem_t *item = FindItemByClassname(item_name);

		if (!item || !item->pickup)
			gi.Com_ErrorFmt("Invalid g_start_item entry: {}\n", item_name);

		int32_t count = 1;

		if (*ptr_copy)
			count = atoi(COM_Parse(&ptr_copy));

		if (count == 0)
		{
			ent->client->pers.inventory[item->id] = 0;
			continue;
		}

		edict_t *dummy = G_Spawn();
		dummy->item = item;
		dummy->count = count;
		dummy->spawnflags |= SPAWNFLAG_ITEM_DROPPED;
		item->pickup(dummy, ent);
		G_FreeEdict(dummy);
	}
}

/*
==============
InitClientPersistant

This is only called when the game first initializes in single player,
but is called after each death and level change in deathmatch
==============
*/
void InitClientPersistant(edict_t *ent, gclient_t *client)
{
	// backup & restore userinfo
	char userinfo[MAX_INFO_STRING];
	Q_strlcpy(userinfo, client->pers.userinfo, sizeof(userinfo));

	memset(&client->pers, 0, sizeof(client->pers));
	ClientUserinfoChanged(ent, userinfo);

	client->pers.health = 100;
	client->pers.max_health = 100;

	// don't give us weapons if we shouldn't have any
	if ((G_TeamplayEnabled() && client->resp.ctf_team != CTF_NOTEAM) ||
		(!G_TeamplayEnabled() && !client->resp.spectator))
	{
		// in coop, if there's already a player in the game and we're new,
		// steal their loadout. this would fix a potential softlock where a new
		// player may not have weapons at all.
		bool taken_loadout = false;

		if (coop->integer)
		{
			for (auto player : active_players())
			{
				if (player == ent || !player->client->pers.spawned ||
					player->client->resp.spectator || player->movetype == MOVETYPE_NOCLIP)
					continue;

				client->pers.inventory = player->client->pers.inventory;
				client->pers.max_ammo = player->client->pers.max_ammo;
				client->pers.power_cubes = player->client->pers.power_cubes;
				taken_loadout = true;
				break;
			}
		}

		if (!taken_loadout)
		{
			// fill with 50s, since it's our most common value
			client->pers.max_ammo.fill(50);
			client->pers.max_ammo[AMMO_BULLETS] = 200;
			client->pers.max_ammo[AMMO_SHELLS] = 100;
			client->pers.max_ammo[AMMO_CELLS] = 200;

			// RAFAEL
			client->pers.max_ammo[AMMO_TRAP] = 5;
			// RAFAEL
			// ROGUE
			client->pers.max_ammo[AMMO_FLECHETTES] = 200;
			client->pers.max_ammo[AMMO_DISRUPTOR] = 12;
			client->pers.max_ammo[AMMO_TESLA] = 5;
			// ROGUE

			if (!deathmatch->integer || !g_instagib->integer)
				client->pers.inventory[IT_WEAPON_BLASTER] = 1;

			// [Kex]
			// start items!
			if (*g_start_items->string)
				Player_GiveStartItems(ent, g_start_items->string);
			else if (deathmatch->integer && g_instagib->integer)
			{
				client->pers.inventory[IT_WEAPON_RAILGUN] = 1;
				client->pers.inventory[IT_AMMO_SLUGS] = 99;
			}

			if (level.start_items && *level.start_items)
				Player_GiveStartItems(ent, level.start_items);

			if (!deathmatch->integer)
				client->pers.inventory[IT_ITEM_COMPASS] = 1;

			// ZOID
			bool give_grapple = (!strcmp(g_allow_grapple->string, "auto")) ?
				(ctf->integer ? !level.no_grapple : 0) :
				g_allow_grapple->integer;

			if (give_grapple)
				client->pers.inventory[IT_WEAPON_GRAPPLE] = 1;
			// ZOID
		}

		NoAmmoWeaponChange(ent, false);

		client->pers.weapon = client->newweapon;
		if (client->newweapon)
			client->pers.selected_item = client->newweapon->id;
		client->newweapon = nullptr;
		// ZOID
		client->pers.lastweapon = client->pers.weapon;
		// ZOID
	}

	if (coop->value && g_coop_enable_lives->integer)
		client->pers.lives = g_coop_num_lives->integer + 1;

	if (ent->client->pers.autoshield >= AUTO_SHIELD_AUTO)
		ent->flags |= FL_WANTS_POWER_ARMOR;

	client->pers.connected = true;
	client->pers.spawned = true;
}

void InitClientResp(gclient_t *client)
{
	// ZOID
	ctfteam_t ctf_team = client->resp.ctf_team;
	bool id_state = client->resp.id_state;
	// ZOID

	memset(&client->resp, 0, sizeof(client->resp));

	// ZOID
	client->resp.ctf_team = ctf_team;
	client->resp.id_state = id_state;
	// ZOID

	client->resp.entertime = level.time;
	client->resp.coop_respawn = client->pers;
}

/*
==================
SaveClientData

Some information that should be persistant, like health,
is still stored in the edict structure, so it needs to
be mirrored out to the client structure before all the
edicts are wiped.
==================
*/
void SaveClientData()
{
	edict_t *ent;

	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		ent = &g_edicts[1 + i];
		if (!ent->inuse)
			continue;
		game.clients[i].pers.health = ent->health;
		game.clients[i].pers.max_health = ent->max_health;
		game.clients[i].pers.savedFlags = (ent->flags & (FL_FLASHLIGHT | FL_GODMODE | FL_NOTARGET | FL_POWER_ARMOR | FL_WANTS_POWER_ARMOR));
		if (coop->integer)
			game.clients[i].pers.score = ent->client->resp.score;
	}
}

void FetchClientEntData(edict_t *ent)
{
	ent->health = ent->client->pers.health;
	ent->max_health = ent->client->pers.max_health;
	ent->flags |= ent->client->pers.savedFlags;
	if (coop->integer)
		ent->client->resp.score = ent->client->pers.score;
}

/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
PlayersRangeFromSpot

Returns the distance to the nearest player from the given spot
================
*/
float PlayersRangeFromSpot(edict_t *spot)
{
	edict_t *player;
	float	 bestplayerdistance;
	vec3_t	 v;
	float	 playerdistance;

	bestplayerdistance = 9999999;

	for (uint32_t n = 1; n <= game.maxclients; n++)
	{
		player = &g_edicts[n];

		if (!player->inuse)
			continue;

		if (player->health <= 0)
			continue;

		v = spot->s.origin - player->s.origin;
		playerdistance = v.length();

		if (playerdistance < bestplayerdistance)
			bestplayerdistance = playerdistance;
	}

	return bestplayerdistance;
}

bool SpawnPointClear(edict_t *spot)
{
	vec3_t p = spot->s.origin + vec3_t{0, 0, 9.f};
	return !gi.trace(p, PLAYER_MINS, PLAYER_MAXS, p, spot, CONTENTS_PLAYER | CONTENTS_MONSTER).startsolid;
}

select_spawn_result_t SelectDeathmatchSpawnPoint(bool farthest, bool force_spawn, bool fallback_to_ctf_or_start)
{
	struct spawn_point_t
	{
		edict_t *point;
		float dist;
	};

	static std::vector<spawn_point_t> spawn_points;

	spawn_points.clear();

	// gather all spawn points 
	edict_t *spot = nullptr;
	while ((spot = G_FindByString<&edict_t::classname>(spot, "info_player_deathmatch")) != nullptr)
		spawn_points.push_back({ spot, PlayersRangeFromSpot(spot) });

	// no points
	if (spawn_points.size() == 0)
	{
		// try CTF spawns...
		if (fallback_to_ctf_or_start)
		{
			spot = nullptr;
			while ((spot = G_FindByString<&edict_t::classname>(spot, "info_player_team1")) != nullptr)
				spawn_points.push_back({ spot, PlayersRangeFromSpot(spot) });
			spot = nullptr;
			while ((spot = G_FindByString<&edict_t::classname>(spot, "info_player_team2")) != nullptr)
				spawn_points.push_back({ spot, PlayersRangeFromSpot(spot) });

			// we only have an info_player_start then
			if (spawn_points.size() == 0)
			{
				spot = G_FindByString<&edict_t::classname>(nullptr, "info_player_start");

				if (spot)
					spawn_points.push_back({ spot, PlayersRangeFromSpot(spot) });

				// map is malformed
				if (spawn_points.size() == 0)
					return { nullptr, false };
			}
		}
		else
			return { nullptr, false };
	}

	// if there's only one spawn point, that's the one.
	if (spawn_points.size() == 1)
	{
		if (force_spawn || SpawnPointClear(spawn_points[0].point))
			return { spawn_points[0].point, true };

		return { nullptr, true };
	}

	// order by distances ascending (top of list has closest players to point)
	std::sort(spawn_points.begin(), spawn_points.end(), [](const spawn_point_t &a, const spawn_point_t &b) { return a.dist < b.dist; });

	// farthest spawn is simple
	if (farthest)
	{
		for (int32_t i = spawn_points.size() - 1; i >= 0; --i)
		{
			if (SpawnPointClear(spawn_points[i].point))
				return { spawn_points[i].point, true };
		}

		// none clear
	}
	else
	{
		// for random, select a random point other than the two
		// that are closest to the player if possible.
		// shuffle the non-distance-related spawn points
		std::shuffle(spawn_points.begin() + 2, spawn_points.end(), mt_rand);

		// run down the list and pick the first one that we can use
		for (auto it = spawn_points.begin() + 2; it != spawn_points.end(); ++it)
		{
			auto spot = it->point;

			if (SpawnPointClear(spot))
				return { spot, true };
		}

		// none clear, so we have to pick one of the other two
		if (SpawnPointClear(spawn_points[1].point))
			return { spawn_points[1].point, true };
		else if (SpawnPointClear(spawn_points[0].point))
			return { spawn_points[0].point, true };
	}
		
	if (force_spawn)
		return { random_element(spawn_points).point, true };

	return { nullptr, true };
}

//===============
// ROGUE
edict_t *SelectLavaCoopSpawnPoint(edict_t *ent)
{
	int		 index;
	edict_t *spot = nullptr;
	float	 lavatop;
	edict_t *lava;
	edict_t *pointWithLeastLava;
	float	 lowest;
	edict_t *spawnPoints[64];
	vec3_t	 center;
	int		 numPoints;
	edict_t *highestlava;

	lavatop = -99999;
	highestlava = nullptr;

	// first, find the highest lava
	// remember that some will stop moving when they've filled their
	// areas...
	lava = nullptr;
	while (1)
	{
		lava = G_FindByString<&edict_t::classname>(lava, "func_water");
		if (!lava)
			break;

		center = lava->absmax + lava->absmin;
		center *= 0.5f;

		if (lava->spawnflags.has(SPAWNFLAG_WATER_SMART) && (gi.pointcontents(center) & MASK_WATER))
		{
			if (lava->absmax[2] > lavatop)
			{
				lavatop = lava->absmax[2];
				highestlava = lava;
			}
		}
	}

	// if we didn't find ANY lava, then return nullptr
	if (!highestlava)
		return nullptr;

	// find the top of the lava and include a small margin of error (plus bbox size)
	lavatop = highestlava->absmax[2] + 64;

	// find all the lava spawn points and store them in spawnPoints[]
	spot = nullptr;
	numPoints = 0;
	while ((spot = G_FindByString<&edict_t::classname>(spot, "info_player_coop_lava")))
	{
		if (numPoints == 64)
			break;

		spawnPoints[numPoints++] = spot;
	}

	// walk up the sorted list and return the lowest, open, non-lava spawn point
	spot = nullptr;
	lowest = 999999;
	pointWithLeastLava = nullptr;
	for (index = 0; index < numPoints; index++)
	{
		if (spawnPoints[index]->s.origin[2] < lavatop)
			continue;

		if (PlayersRangeFromSpot(spawnPoints[index]) > 32)
		{
			if (spawnPoints[index]->s.origin[2] < lowest)
			{
				// save the last point
				pointWithLeastLava = spawnPoints[index];
				lowest = spawnPoints[index]->s.origin[2];
			}
		}
	}

	return pointWithLeastLava;
}
// ROGUE
//===============

// [Paril-KEX]
static edict_t *SelectSingleSpawnPoint(edict_t *ent)
{
	edict_t *spot = nullptr;

	while ((spot = G_FindByString<&edict_t::classname>(spot, "info_player_start")) != nullptr)
	{
		if (!game.spawnpoint[0] && !spot->targetname)
			break;

		if (!game.spawnpoint[0] || !spot->targetname)
			continue;

		if (Q_strcasecmp(game.spawnpoint, spot->targetname) == 0)
			break;
	}

	if (!spot)
	{
		// there wasn't a matching targeted spawnpoint, use one that has no targetname
		while ((spot = G_FindByString<&edict_t::classname>(spot, "info_player_start")) != nullptr)
			if (!spot->targetname)
				return spot;
	}

	// none at all, so just pick any
	if (!spot)
		return G_FindByString<&edict_t::classname>(spot, "info_player_start");

	return spot;
}

// [Paril-KEX]
static edict_t *G_UnsafeSpawnPosition(vec3_t spot, bool check_players)
{
	contents_t mask = MASK_PLAYERSOLID;

	if (!check_players)
		mask &= ~CONTENTS_PLAYER;

	trace_t tr = gi.trace(spot, PLAYER_MINS, PLAYER_MAXS, spot, nullptr, mask);

	// sometimes the spot is too close to the ground, give it a bit of slack
	if (tr.startsolid && !tr.ent->client)
	{
		spot[2] += 1;
		tr = gi.trace(spot, PLAYER_MINS, PLAYER_MAXS, spot, nullptr, mask);
	}

	// no idea why this happens in some maps..
	if (tr.startsolid && !tr.ent->client)
	{
		// try a nudge
		if (G_FixStuckObject_Generic(spot, PLAYER_MINS, PLAYER_MAXS, [mask] (const vec3_t &start, const vec3_t &mins, const vec3_t &maxs, const vec3_t &end) {
			return gi.trace(start, mins, maxs, end, nullptr, mask);
		}) == stuck_result_t::NO_GOOD_POSITION)
			return tr.ent; // what do we do here...?

		trace_t tr = gi.trace(spot, PLAYER_MINS, PLAYER_MAXS, spot, nullptr, mask);

		if (tr.startsolid && !tr.ent->client)
			return tr.ent; // what do we do here...?
	}

	if (tr.fraction == 1.f)
		return nullptr;
	else if (check_players && tr.ent && tr.ent->client)
		return tr.ent;

	return nullptr;
}

edict_t *SelectCoopSpawnPoint(edict_t *ent, bool force_spawn, bool check_players)
{
	edict_t	*spot = nullptr;
	const char *target;

	// ROGUE
	//  rogue hack, but not too gross...
	if (!Q_strcasecmp(level.mapname, "rmine2"))
		return SelectLavaCoopSpawnPoint(ent);
	// ROGUE

	// try the main spawn point first
	spot = SelectSingleSpawnPoint(ent);

	if (spot && !G_UnsafeSpawnPosition(spot->s.origin, check_players))
		return spot;

	spot = nullptr;

	// assume there are four coop spots at each spawnpoint
	int32_t num_valid_spots = 0;

	while (1)
	{
		spot = G_FindByString<&edict_t::classname>(spot, "info_player_coop");
		if (!spot)
			break; // we didn't have enough...

		target = spot->targetname;
		if (!target)
			target = "";
		if (Q_strcasecmp(game.spawnpoint, target) == 0)
		{ // this is a coop spawn point for one of the clients here
			num_valid_spots++;

			if (!G_UnsafeSpawnPosition(spot->s.origin, check_players))
				return spot; // this is it
		}
	}

	bool use_targetname = true;

	// if we didn't find any spots, map is probably set up wrong.
	// use empty targetname ones.
	if (!num_valid_spots)
	{
		use_targetname = false;

		while (1)
		{
			spot = G_FindByString<&edict_t::classname>(spot, "info_player_coop");
			if (!spot)
				break; // we didn't have enough...

			target = spot->targetname;
			if (!target)
			{
				// this is a coop spawn point for one of the clients here
				num_valid_spots++;

				if (!G_UnsafeSpawnPosition(spot->s.origin, check_players))
					return spot; // this is it
			}
		}
	}

	// if player collision is disabled, just pick a random spot
	if (!g_coop_player_collision->integer)
	{
		spot = nullptr;

		num_valid_spots = irandom(num_valid_spots);

		while (1)
		{
			spot = G_FindByString<&edict_t::classname>(spot, "info_player_coop");

			if (!spot)
				break; // we didn't have enough...

			target = spot->targetname;
			if (use_targetname && !target)
				target = "";
			if (use_targetname ? (Q_strcasecmp(game.spawnpoint, target) == 0) : !target)
			{ // this is a coop spawn point for one of the clients here
				num_valid_spots++;

				if (!num_valid_spots)
					return spot;

				--num_valid_spots;
			}
		}

		// if this fails, just fall through to some other spawn.
	}

	// no safe spots..?
	if (force_spawn || !g_coop_player_collision->integer)
		return SelectSingleSpawnPoint(spot);
	
	return nullptr;
}

bool TryLandmarkSpawn(edict_t* ent, vec3_t& origin, vec3_t& angles)
{
	// if transitioning from another level with a landmark seamless transition
	// just set the location here
	if (!ent->client->landmark_name || !strlen(ent->client->landmark_name))
	{
		return false;
	}

	edict_t* landmark = G_PickTarget(ent->client->landmark_name);
	if (!landmark)
	{
		return false;
	}

	vec3_t old_origin = origin;
	vec3_t spot_origin = origin;
	origin = ent->client->landmark_rel_pos;
	
	// rotate our relative landmark into our new landmark's frame of reference
	origin = RotatePointAroundVector({ 1, 0, 0 }, origin, landmark->s.angles[0]);
	origin = RotatePointAroundVector({ 0, 1, 0 }, origin, landmark->s.angles[2]);
	origin = RotatePointAroundVector({ 0, 0, 1 }, origin, landmark->s.angles[1]);

	origin += landmark->s.origin;

	angles = ent->client->oldviewangles + landmark->s.angles;

	if (landmark->spawnflags.has(SPAWNFLAG_LANDMARK_KEEP_Z))
		origin[2] = spot_origin[2];

	// sometimes, landmark spawns can cause slight inconsistencies in collision;
	// we'll do a bit of tracing to make sure the bbox is clear
	if (G_FixStuckObject_Generic(origin, PLAYER_MINS, PLAYER_MAXS, [ent] (const vec3_t &start, const vec3_t &mins, const vec3_t &maxs, const vec3_t &end) {
			return gi.trace(start, mins, maxs, end, ent, MASK_PLAYERSOLID & ~CONTENTS_PLAYER);
		}) == stuck_result_t::NO_GOOD_POSITION)
	{
		origin = old_origin;
		return false;
	}

	ent->s.origin = origin;

	// rotate the velocity that we grabbed from the map
	if (ent->velocity)
	{
		ent->velocity = RotatePointAroundVector({ 1, 0, 0 }, ent->velocity, landmark->s.angles[0]);
		ent->velocity = RotatePointAroundVector({ 0, 1, 0 }, ent->velocity, landmark->s.angles[2]);
		ent->velocity = RotatePointAroundVector({ 0, 0, 1 }, ent->velocity, landmark->s.angles[1]);
	}

	return true;
}

/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, coop start, etc
============
*/
bool SelectSpawnPoint(edict_t *ent, vec3_t &origin, vec3_t &angles, bool force_spawn, bool &landmark)
{
	edict_t *spot = nullptr;

	// DM spots are simple
	if (deathmatch->integer)
	{
		if (G_TeamplayEnabled())
			spot = SelectCTFSpawnPoint(ent, force_spawn);
		else
		{
			select_spawn_result_t result = SelectDeathmatchSpawnPoint(g_dm_spawn_farthest->integer, force_spawn, true);

			if (!result.any_valid)
				gi.Com_Error("no valid spawn points found");

			spot = result.spot;
		}

		if (spot)
		{
			origin = spot->s.origin + vec3_t{ 0, 0, 9 };
			angles = spot->s.angles;

			return true;
		}

		return false;
	}
	
	if (coop->integer)
	{
		spot = SelectCoopSpawnPoint(ent, force_spawn, true);

		if (!spot)
			spot = SelectCoopSpawnPoint(ent, force_spawn, false);

		// no open spot yet
		if (!spot)
		{
			// in worst case scenario in coop during intermission, just spawn us at intermission
			// spot. this only happens for a single frame, and won't break
			// anything if they come back.
			if (level.intermissiontime)
			{
				origin = level.intermission_origin;
				angles = level.intermission_angle;
				return true;
			}

			return false;
		}
	}
	else
	{
		spot = SelectSingleSpawnPoint(ent);

		// in SP, just put us at the origin if spawn fails
		if (!spot)
		{
			gi.Com_PrintFmt("Couldn't find spawn point {}\n", game.spawnpoint);

			origin = { 0, 0, 0 };
			angles = { 0, 0, 0 };

			return true;
		}
	}

	// spot should always be non-null here
	
	origin = spot->s.origin;
	angles = spot->s.angles;

	// check landmark
	if (TryLandmarkSpawn(ent, origin, angles))
		landmark = true;

	return true;
}

//======================================================================

void InitBodyQue()
{
	int		 i;
	edict_t *ent;

	level.body_que = 0;
	for (i = 0; i < BODY_QUEUE_SIZE; i++)
	{
		ent = G_Spawn();
		ent->classname = "bodyque";
	}
}

DIE(body_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	if (self->s.modelindex == MODELINDEX_PLAYER &&
		self->health < self->gib_health)
	{
		gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		ThrowGibs(self, damage, { { 4, "models/objects/gibs/sm_meat/tris.md2" } });
		self->s.origin[2] -= 48;
		ThrowClientHead(self, damage);
	}

	if (mod.id == MOD_CRUSH)
	{
		// prevent explosion singularities
		self->svflags = SVF_NOCLIENT;
		self->takedamage = false;
		self->solid = SOLID_NOT;
		self->movetype = MOVETYPE_NOCLIP;
		gi.linkentity(self);
	}
}

void CopyToBodyQue(edict_t *ent)
{
	// if we were completely removed, don't bother with a body
	if (!ent->s.modelindex)
		return;

	edict_t *body;

	// grab a body que and cycle to the next one
	body = &g_edicts[game.maxclients + level.body_que + 1];
	level.body_que = (level.body_que + 1) % BODY_QUEUE_SIZE;

	// FIXME: send an effect on the removed body

	gi.unlinkentity(ent);

	gi.unlinkentity(body);
	body->s = ent->s;
	body->s.number = body - g_edicts;
	body->s.skinnum = ent->s.skinnum & 0xFF; // only copy the client #
	body->s.effects = EF_NONE;
	body->s.renderfx = RF_NONE;

	body->svflags = ent->svflags;
	body->absmin = ent->absmin;
	body->absmax = ent->absmax;
	body->size = ent->size;
	body->solid = ent->solid;
	body->clipmask = ent->clipmask;
	body->owner = ent->owner;
	body->movetype = ent->movetype;
	body->health = ent->health;
	body->gib_health = ent->gib_health;
	body->s.event = EV_OTHER_TELEPORT;
	body->velocity = ent->velocity;
	body->avelocity = ent->avelocity;
	body->groundentity = ent->groundentity;
	body->groundentity_linkcount = ent->groundentity_linkcount;

	if (ent->takedamage)
	{
		body->mins = ent->mins;
		body->maxs = ent->maxs;
	}
	else
		body->mins = body->maxs = {};

	body->die = body_die;
	body->takedamage = true;

	gi.linkentity(body);
}

void G_PostRespawn(edict_t *self)
{
	if (self->svflags & SVF_NOCLIENT)
		return;

	// add a teleportation effect
	self->s.event = EV_PLAYER_TELEPORT;

	// hold in place briefly
	self->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	self->client->ps.pmove.pm_time = 112;

	self->client->respawn_time = level.time;
}

void respawn(edict_t *self)
{
	if (deathmatch->integer || coop->integer)
	{
		// spectators don't leave bodies
		if (!self->client->resp.spectator)
			CopyToBodyQue(self);
		self->svflags &= ~SVF_NOCLIENT;
		PutClientInServer(self);

		G_PostRespawn(self);
		return;
	}

	// restart the entire server
	gi.AddCommandString("menu_loadgame\n");
}

/*
 * only called when pers.spectator changes
 * note that resp.spectator should be the opposite of pers.spectator here
 */
void spectator_respawn(edict_t *ent)
{
	uint32_t i, numspec;

	// if the user wants to become a spectator, make sure he doesn't
	// exceed max_spectators

	if (ent->client->pers.spectator)
	{
		char value[MAX_INFO_VALUE] = { 0 };
		gi.Info_ValueForKey(ent->client->pers.userinfo, "spectator", value, sizeof(value));

		if (*spectator_password->string &&
			strcmp(spectator_password->string, "none") &&
			strcmp(spectator_password->string, value))
		{
			gi.LocClient_Print(ent, PRINT_HIGH, "Spectator password incorrect.\n");
			ent->client->pers.spectator = false;
			gi.WriteByte(svc_stufftext);
			gi.WriteString("spectator 0\n");
			gi.unicast(ent, true);
			return;
		}

		// count spectators
		for (i = 1, numspec = 0; i <= game.maxclients; i++)
			if (g_edicts[i].inuse && g_edicts[i].client->pers.spectator)
				numspec++;

		if (numspec >= (uint32_t) maxspectators->integer)
		{
			gi.LocClient_Print(ent, PRINT_HIGH, "Server spectator limit is full.");
			ent->client->pers.spectator = false;
			// reset his spectator var
			gi.WriteByte(svc_stufftext);
			gi.WriteString("spectator 0\n");
			gi.unicast(ent, true);
			return;
		}
	}
	else
	{
		// he was a spectator and wants to join the game
		// he must have the right password
		char value[MAX_INFO_VALUE] = { 0 };
		gi.Info_ValueForKey(ent->client->pers.userinfo, "password", value, sizeof(value));

		if (*password->string && strcmp(password->string, "none") &&
			strcmp(password->string, value))
		{
			gi.LocClient_Print(ent, PRINT_HIGH, "Password incorrect.\n");
			ent->client->pers.spectator = true;
			gi.WriteByte(svc_stufftext);
			gi.WriteString("spectator 1\n");
			gi.unicast(ent, true);
			return;
		}
	}

	// clear score on respawn
	ent->client->resp.score = ent->client->pers.score = 0;

	// move us to no team
	ent->client->resp.ctf_team = CTF_NOTEAM;

	// change spectator mode
	ent->client->resp.spectator = ent->client->pers.spectator;

	ent->svflags &= ~SVF_NOCLIENT;
	PutClientInServer(ent);

	// add a teleportation effect
	if (!ent->client->pers.spectator)
	{
		// send effect
		gi.WriteByte(svc_muzzleflash);
		gi.WriteEntity(ent);
		gi.WriteByte(MZ_LOGIN);
		gi.multicast(ent->s.origin, MULTICAST_PVS, false);

		// hold in place briefly
		ent->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
		ent->client->ps.pmove.pm_time = 112;
	}

	ent->client->respawn_time = level.time;

	if (ent->client->pers.spectator)
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_observing", ent->client->pers.netname);
	else
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_joined_game", ent->client->pers.netname);
}

//==============================================================

// [Paril-KEX]
// skinnum was historically used to pack data
// so we're going to build onto that.
void P_AssignClientSkinnum(edict_t *ent)
{
	if (ent->s.modelindex != 255)
		return;

	player_skinnum_t packed;

	packed.client_num = ent->client - game.clients;
	if (ent->client->pers.weapon)
		packed.vwep_index = ent->client->pers.weapon->vwep_index - level.vwep_offset + 1;
	else
		packed.vwep_index = 0;
	packed.viewheight = ent->client->ps.viewoffset.z + ent->client->ps.pmove.viewheight;
	
	if (coop->value)
		packed.team_index = 1; // all players are teamed in coop
	else if (G_TeamplayEnabled())
		packed.team_index = ent->client->resp.ctf_team;
	else
		packed.team_index = 0;

	if (ent->deadflag)
		packed.poi_icon = 1;
	else
		packed.poi_icon = 0;

	ent->s.skinnum = packed.skinnum;
}

// [Paril-KEX] send player level POI
void P_SendLevelPOI(edict_t *ent)
{
	if (!level.valid_poi)
		return;

	gi.WriteByte(svc_poi);
	gi.WriteShort(POI_OBJECTIVE);
	gi.WriteShort(10000);
	gi.WritePosition(ent->client->help_poi_location);
	gi.WriteShort(ent->client->help_poi_image);
	gi.WriteByte(208);
	gi.WriteByte(POI_FLAG_NONE);
	gi.unicast(ent, true);
}

// [Paril-KEX] force the fog transition on the given player,
// optionally instantaneously (ignore any transition time)
void P_ForceFogTransition(edict_t *ent, bool instant)
{
	// sanity check; if we're not changing the values, don't bother
	if (ent->client->fog == ent->client->pers.wanted_fog &&
		ent->client->heightfog == ent->client->pers.wanted_heightfog)
		return;

	svc_fog_data_t fog {};
	
	// check regular fog
	if (ent->client->pers.wanted_fog[0] != ent->client->fog[0] ||
		ent->client->pers.wanted_fog[4] != ent->client->fog[4])
	{
		fog.bits |= svc_fog_data_t::BIT_DENSITY;
		fog.density = ent->client->pers.wanted_fog[0];
		fog.skyfactor = ent->client->pers.wanted_fog[4] * 255.f;
	}
	if (ent->client->pers.wanted_fog[1] != ent->client->fog[1])
	{
		fog.bits |= svc_fog_data_t::BIT_R;
		fog.red = ent->client->pers.wanted_fog[1] * 255.f;
	}
	if (ent->client->pers.wanted_fog[2] != ent->client->fog[2])
	{
		fog.bits |= svc_fog_data_t::BIT_G;
		fog.green = ent->client->pers.wanted_fog[2] * 255.f;
	}
	if (ent->client->pers.wanted_fog[3] != ent->client->fog[3])
	{
		fog.bits |= svc_fog_data_t::BIT_B;
		fog.blue = ent->client->pers.wanted_fog[3] * 255.f;
	}

	if (!instant && ent->client->pers.fog_transition_time)
	{
		fog.bits |= svc_fog_data_t::BIT_TIME;
		fog.time = clamp(ent->client->pers.fog_transition_time.milliseconds(), (int64_t) 0, (int64_t) std::numeric_limits<uint16_t>::max());
	}
	
	// check heightfog stuff
	auto &hf = ent->client->heightfog;
	const auto &wanted_hf = ent->client->pers.wanted_heightfog;
	
	if (hf.falloff != wanted_hf.falloff)
	{
		fog.bits |= svc_fog_data_t::BIT_HEIGHTFOG_FALLOFF;
		if (!wanted_hf.falloff)
			fog.hf_falloff = 0;
		else
			fog.hf_falloff = wanted_hf.falloff;
	}
	if (hf.density != wanted_hf.density)
	{
		fog.bits |= svc_fog_data_t::BIT_HEIGHTFOG_DENSITY;

		if (!wanted_hf.density)
			fog.hf_density = 0;
		else
			fog.hf_density = wanted_hf.density;
	}

	if (hf.start[0] != wanted_hf.start[0])
	{
		fog.bits |= svc_fog_data_t::BIT_HEIGHTFOG_START_R;
		fog.hf_start_r = wanted_hf.start[0] * 255.f;
	}
	if (hf.start[1] != wanted_hf.start[1])
	{
		fog.bits |= svc_fog_data_t::BIT_HEIGHTFOG_START_G;
		fog.hf_start_g = wanted_hf.start[1] * 255.f;
	}
	if (hf.start[2] != wanted_hf.start[2])
	{
		fog.bits |= svc_fog_data_t::BIT_HEIGHTFOG_START_B;
		fog.hf_start_b = wanted_hf.start[2] * 255.f;
	}
	if (hf.start[3] != wanted_hf.start[3])
	{
		fog.bits |= svc_fog_data_t::BIT_HEIGHTFOG_START_DIST;
		fog.hf_start_dist = wanted_hf.start[3];
	}

	if (hf.end[0] != wanted_hf.end[0])
	{
		fog.bits |= svc_fog_data_t::BIT_HEIGHTFOG_END_R;
		fog.hf_end_r = wanted_hf.end[0] * 255.f;
	}
	if (hf.end[1] != wanted_hf.end[1])
	{
		fog.bits |= svc_fog_data_t::BIT_HEIGHTFOG_END_G;
		fog.hf_end_g = wanted_hf.end[1] * 255.f;
	}
	if (hf.end[2] != wanted_hf.end[2])
	{
		fog.bits |= svc_fog_data_t::BIT_HEIGHTFOG_END_B;
		fog.hf_end_b = wanted_hf.end[2] * 255.f;
	}
	if (hf.end[3] != wanted_hf.end[3])
	{
		fog.bits |= svc_fog_data_t::BIT_HEIGHTFOG_END_DIST;
		fog.hf_end_dist = wanted_hf.end[3];
	}

	if (fog.bits & 0xFF00)
		fog.bits |= svc_fog_data_t::BIT_MORE_BITS;

	gi.WriteByte(svc_fog);

	if (fog.bits & svc_fog_data_t::BIT_MORE_BITS)
		gi.WriteShort(fog.bits);
	else
		gi.WriteByte(fog.bits);
	
	if (fog.bits & svc_fog_data_t::BIT_DENSITY)
	{
		gi.WriteFloat(fog.density);
		gi.WriteByte(fog.skyfactor);
	}
	if (fog.bits & svc_fog_data_t::BIT_R)
		gi.WriteByte(fog.red);
	if (fog.bits & svc_fog_data_t::BIT_G)
		gi.WriteByte(fog.green);
	if (fog.bits & svc_fog_data_t::BIT_B)
		gi.WriteByte(fog.blue);
	if (fog.bits & svc_fog_data_t::BIT_TIME)
		gi.WriteShort(fog.time);
	
	if (fog.bits & svc_fog_data_t::BIT_HEIGHTFOG_FALLOFF)
		gi.WriteFloat(fog.hf_falloff);
	if (fog.bits & svc_fog_data_t::BIT_HEIGHTFOG_DENSITY)
		gi.WriteFloat(fog.hf_density);

	if (fog.bits & svc_fog_data_t::BIT_HEIGHTFOG_START_R)
		gi.WriteByte(fog.hf_start_r);
	if (fog.bits & svc_fog_data_t::BIT_HEIGHTFOG_START_G)
		gi.WriteByte(fog.hf_start_g);
	if (fog.bits & svc_fog_data_t::BIT_HEIGHTFOG_START_B)
		gi.WriteByte(fog.hf_start_b);
	if (fog.bits & svc_fog_data_t::BIT_HEIGHTFOG_START_DIST)
		gi.WriteLong(fog.hf_start_dist);

	if (fog.bits & svc_fog_data_t::BIT_HEIGHTFOG_END_R)
		gi.WriteByte(fog.hf_end_r);
	if (fog.bits & svc_fog_data_t::BIT_HEIGHTFOG_END_G)
		gi.WriteByte(fog.hf_end_g);
	if (fog.bits & svc_fog_data_t::BIT_HEIGHTFOG_END_B)
		gi.WriteByte(fog.hf_end_b);
	if (fog.bits & svc_fog_data_t::BIT_HEIGHTFOG_END_DIST)
		gi.WriteLong(fog.hf_end_dist);

	gi.unicast(ent, true);

	ent->client->fog = ent->client->pers.wanted_fog;
	hf = wanted_hf;
}

// [Paril-KEX] ugly global to handle squad respawn origin
static bool use_squad_respawn = false;
static bool spawn_from_begin = false;
static vec3_t squad_respawn_position, squad_respawn_angles;

inline void PutClientOnSpawnPoint(edict_t *ent, const vec3_t &spawn_origin, const vec3_t &spawn_angles)
{
	gclient_t *client = ent->client;

	client->ps.pmove.origin = spawn_origin;

	ent->s.origin = spawn_origin;
	if (!use_squad_respawn)
		ent->s.origin[2] += 1; // make sure off ground
	ent->s.old_origin = ent->s.origin;

	// set the delta angle
	client->ps.pmove.delta_angles = spawn_angles - client->resp.cmd_angles;

	ent->s.angles = spawn_angles;
	ent->s.angles[PITCH] /= 3;

	client->ps.viewangles = ent->s.angles;
	client->v_angle = ent->s.angles;

	AngleVectors(client->v_angle, client->v_forward, nullptr, nullptr);
}

/*
===========
PutClientInServer

Called when a player connects to a server or respawns in
a deathmatch.
============
*/
void PutClientInServer(edict_t *ent)
{
	int					index;
	vec3_t				spawn_origin, spawn_angles;
	gclient_t		  *client;
	client_persistant_t saved;
	client_respawn_t	resp;

	index = ent - g_edicts - 1;
	client = ent->client;

	// clear velocity now, since landmark may change it
	ent->velocity = {};

	bool keepVelocity = client->landmark_name != nullptr;

	if (keepVelocity)
		ent->velocity = client->oldvelocity;

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	bool valid_spawn = false;
	bool force_spawn = client->awaiting_respawn && level.time > client->respawn_timeout;
	bool is_landmark = false;

	if (use_squad_respawn)
	{
		spawn_origin = squad_respawn_position;
		spawn_angles = squad_respawn_angles;
		valid_spawn = true;
	}
	else if (gamerules->integer && DMGame.SelectSpawnPoint) // PGM
		valid_spawn = DMGame.SelectSpawnPoint(ent, spawn_origin, spawn_angles, force_spawn); // PGM
	else										  // PGM
		valid_spawn = SelectSpawnPoint(ent, spawn_origin, spawn_angles, force_spawn, is_landmark);

	// [Paril-KEX] if we didn't get a valid spawn, hold us in
	// limbo for a while until we do get one
	if (!valid_spawn)
	{
		// only do this once per spawn
		if (!client->awaiting_respawn)
		{
			char userinfo[MAX_INFO_STRING];
			memcpy(userinfo, client->pers.userinfo, sizeof(userinfo));
			ClientUserinfoChanged(ent, userinfo);

			client->respawn_timeout = level.time + 3_sec;
		}

		// find a spot to place us
		if (!level.respawn_intermission)
		{
			// find an intermission spot
			edict_t *pt = G_FindByString<&edict_t::classname>(nullptr, "info_player_intermission");
			if (!pt)
			{ // the map creator forgot to put in an intermission point...
				pt = G_FindByString<&edict_t::classname>(nullptr, "info_player_start");
				if (!pt)
					pt = G_FindByString<&edict_t::classname>(nullptr, "info_player_deathmatch");
			}
			else
			{ // choose one of four spots
				int32_t i = irandom(4);
				while (i--)
				{
					pt = G_FindByString<&edict_t::classname>(pt, "info_player_intermission");
					if (!pt) // wrap around the list
						pt = G_FindByString<&edict_t::classname>(pt, "info_player_intermission");
				}
			}

			level.intermission_origin = pt->s.origin;
			level.intermission_angle = pt->s.angles;
			level.respawn_intermission = true;
		}

		ent->s.origin = level.intermission_origin;
		ent->client->ps.pmove.origin = level.intermission_origin;
		ent->client->ps.viewangles = level.intermission_angle;

		client->awaiting_respawn = true;
		client->ps.pmove.pm_type = PM_FREEZE;
		client->ps.rdflags = RDF_NONE;
		ent->deadflag = false;
		ent->solid = SOLID_NOT;
		ent->movetype = MOVETYPE_NOCLIP;
		ent->s.modelindex = 0;
		ent->svflags |= SVF_NOCLIENT;
		ent->client->ps.team_id = ent->client->resp.ctf_team;
		gi.linkentity(ent);

		return;
	}
	
	client->resp.ctf_state++;

	bool was_waiting_for_respawn = client->awaiting_respawn;

	if (client->awaiting_respawn)
		ent->svflags &= ~SVF_NOCLIENT;

	client->awaiting_respawn = false;
	client->respawn_timeout = 0_ms;

	char social_id[MAX_INFO_VALUE];
	Q_strlcpy(social_id, ent->client->pers.social_id, sizeof(social_id));

	// deathmatch wipes most client data every spawn
	if (deathmatch->integer)
	{
		client->pers.health = 0;
		resp = client->resp;
	}
	else
	{
		// [Kex] Maintain user info in singleplayer to keep the player skin. 
		char userinfo[MAX_INFO_STRING];
		memcpy(userinfo, client->pers.userinfo, sizeof(userinfo));

		if (coop->integer)
		{
			resp = client->resp;

			if (!P_UseCoopInstancedItems())
			{
				resp.coop_respawn.game_help1changed = client->pers.game_help1changed;
				resp.coop_respawn.game_help2changed = client->pers.game_help2changed;
				resp.coop_respawn.helpchanged = client->pers.helpchanged;
				client->pers = resp.coop_respawn;
			}
			else
			{
				// fix weapon
				if (!client->pers.weapon)
					client->pers.weapon = client->pers.lastweapon;
			}
		}

		ClientUserinfoChanged(ent, userinfo);

		if (coop->integer)
		{
			if (resp.score > client->pers.score)
				client->pers.score = resp.score;
		}
		else
			memset(&resp, 0, sizeof(resp));
	}

	// clear everything but the persistant data
	saved = client->pers;
	memset(client, 0, sizeof(*client));
	client->pers = saved;
	client->resp = resp;

	// on a new, fresh spawn (always in DM, clear inventory
	// or new spawns in SP/coop)
	if (client->pers.health <= 0)
		InitClientPersistant(ent, client);

	// restore social ID
	Q_strlcpy(ent->client->pers.social_id, social_id, sizeof(social_id));

	// fix level switch issue
	ent->client->pers.connected = true;

	// slow time will be unset here
	globals.server_flags &= ~SERVER_FLAG_SLOW_TIME;

	// copy some data from the client to the entity
	FetchClientEntData(ent);

	// clear entity values
	ent->groundentity = nullptr;
	ent->client = &game.clients[index];
	ent->takedamage = true;
	ent->movetype = MOVETYPE_WALK;
	ent->viewheight = 22;
	ent->inuse = true;
	ent->classname = "player";
	ent->mass = 200;
	ent->solid = SOLID_BBOX;
	ent->deadflag = false;
	ent->air_finished = level.time + 12_sec;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->model = "players/male/tris.md2";
	ent->die = player_die;
	ent->waterlevel = WATER_NONE;
	ent->watertype = CONTENTS_NONE;
	ent->flags &= ~( FL_NO_KNOCKBACK | FL_ALIVE_KNOCKBACK_ONLY | FL_NO_DAMAGE_EFFECTS );
	ent->svflags &= ~SVF_DEADMONSTER;
	ent->svflags |= SVF_PLAYER;

	ent->flags &= ~FL_SAM_RAIMI; // PGM - turn off sam raimi flag

	ent->mins = PLAYER_MINS;
	ent->maxs = PLAYER_MAXS;

	// clear playerstate values
	memset(&ent->client->ps, 0, sizeof(client->ps));

	char val[MAX_INFO_VALUE];
	gi.Info_ValueForKey(ent->client->pers.userinfo, "fov", val, sizeof(val));
	ent->client->ps.fov = clamp((float) atoi(val), 1.f, 160.f);

	ent->client->ps.pmove.viewheight = ent->viewheight;
	ent->client->ps.team_id = ent->client->resp.ctf_team;

	if (!G_ShouldPlayersCollide(false))
		ent->clipmask &= ~CONTENTS_PLAYER;

	// PGM
	if (client->pers.weapon)
		client->ps.gunindex = gi.modelindex(client->pers.weapon->view_model);
	else
		client->ps.gunindex = 0;
	client->ps.gunskin = 0;
	// PGM

	// clear entity state values
	ent->s.effects = EF_NONE;
	ent->s.modelindex = MODELINDEX_PLAYER;	// will use the skin specified model
	ent->s.modelindex2 = MODELINDEX_PLAYER; // custom gun model
	// sknum is player num and weapon number
	// weapon number will be added in changeweapon
	P_AssignClientSkinnum(ent);

	ent->s.frame = 0;

	PutClientOnSpawnPoint(ent, spawn_origin, spawn_angles);

	// [Paril-KEX] set up world fog & send it instantly
	ent->client->pers.wanted_fog = {
		world->fog.density,
		world->fog.color[0],
		world->fog.color[1],
		world->fog.color[2],
		world->fog.sky_factor
	};
	ent->client->pers.wanted_heightfog = {
		{ world->heightfog.start_color[0], world->heightfog.start_color[1], world->heightfog.start_color[2], world->heightfog.start_dist },
		{ world->heightfog.end_color[0], world->heightfog.end_color[1], world->heightfog.end_color[2], world->heightfog.end_dist },
		world->heightfog.falloff,
		world->heightfog.density
	};
	P_ForceFogTransition(ent, true);

	// ZOID
	if (CTFStartClient(ent))
		return;
	// ZOID

	// spawn a spectator
	if (client->pers.spectator)
	{
		client->chase_target = nullptr;

		client->resp.spectator = true;

		ent->movetype = MOVETYPE_NOCLIP;
		ent->solid = SOLID_NOT;
		ent->svflags |= SVF_NOCLIENT;
		ent->client->ps.gunindex = 0;
		ent->client->ps.gunskin = 0;
		gi.linkentity(ent);
		return;
	}

	client->resp.spectator = false;

	// [Paril-KEX] a bit of a hack, but landmark spawns can sometimes cause
	// intersecting spawns, so we'll do a sanity check here...
	if (spawn_from_begin)
	{
		if (coop->integer)
		{
			edict_t *collision = G_UnsafeSpawnPosition(ent->s.origin, true);

			if (collision)
			{
				gi.linkentity(ent);

				if (collision->client)
				{
					// we spawned in somebody else, so we're going to change their spawn position
					bool lm = false;
					SelectSpawnPoint(collision, spawn_origin, spawn_angles, true, lm);
					PutClientOnSpawnPoint(collision, spawn_origin, spawn_angles);
				}
				// else, no choice but to accept where ever we spawned :(
			}
		}

		// give us one (1) free fall ticket even if
		// we didn't spawn from landmark
		ent->client->landmark_free_fall = true;
	}

	gi.linkentity(ent);

	if (!KillBox(ent, true, MOD_TELEFRAG_SPAWN))
	{ // could't spawn in?
	}

	// my tribute to cash's level-specific hacks. I hope I live
	// up to his trailblazing cheese.
	if (Q_strcasecmp(level.mapname, "rboss") == 0)
	{
		// if you get on to rboss in single player or coop, ensure
		// the player has the nuke key. (not in DM)
		if (!deathmatch->integer)
			client->pers.inventory[IT_KEY_NUKE] = 1;
	}

	// force the current weapon up
	client->newweapon = client->pers.weapon;
	ChangeWeapon(ent);

	if (was_waiting_for_respawn)
		G_PostRespawn(ent);
}

/*
=====================
ClientBeginDeathmatch

A client has just connected to the server in
deathmatch mode, so clear everything out before starting them.
=====================
*/
void ClientBeginDeathmatch(edict_t *ent)
{
	G_InitEdict(ent);
	
	// make sure we have a known default
	ent->svflags |= SVF_PLAYER;

	InitClientResp(ent->client);

	// ZOID
	if (G_TeamplayEnabled() && ent->client->resp.ctf_team < CTF_TEAM1)
		CTFAssignTeam(ent->client);
	// ZOID

	// PGM
	if (gamerules->integer && DMGame.ClientBegin)
	{
		DMGame.ClientBegin(ent);
	}
	// PGM

	// locate ent at a spawn point
	PutClientInServer(ent);

	if (level.intermissiontime)
	{
		MoveClientToIntermission(ent);
	}
	else
	{
		if (!(ent->svflags & SVF_NOCLIENT))
		{
			// send effect
			gi.WriteByte(svc_muzzleflash);
			gi.WriteEntity(ent);
			gi.WriteByte(MZ_LOGIN);
			gi.multicast(ent->s.origin, MULTICAST_PVS, false);
		}
	}

	gi.LocBroadcast_Print(PRINT_HIGH, "$g_entered_game", ent->client->pers.netname);

	// make sure all view stuff is valid
	ClientEndServerFrame(ent);
}

static void G_SetLevelEntry()
{
	if (deathmatch->integer)
		return;
	// map is a hub map, so we shouldn't bother tracking any of this.
	// the next map will pick up as the start.
	else if (level.hub_map)
		return;

	level_entry_t *found_entry = nullptr;
	int32_t highest_order = 0;

	for (size_t i = 0; i < MAX_LEVELS_PER_UNIT; i++)
	{
		level_entry_t *entry = &game.level_entries[i];

		highest_order = max(highest_order, entry->visit_order);

		if (!strcmp(entry->map_name, level.mapname) || !*entry->map_name)
		{
			found_entry = entry;
			break;
		}
	}

	if (!found_entry)
	{
		gi.Com_PrintFmt("WARNING: more than {} maps in unit, can't track the rest\n", MAX_LEVELS_PER_UNIT);
		return;
	}

	level.entry = found_entry;
	Q_strlcpy(level.entry->map_name, level.mapname, sizeof(level.entry->map_name));

	// we're visiting this map for the first time, so
	// mark it in our order as being recent
	if (!*level.entry->pretty_name)
	{
		Q_strlcpy(level.entry->pretty_name, level.level_name, sizeof(level.entry->pretty_name));
		level.entry->visit_order = highest_order + 1;

		// give all of the clients an extra life back
		if (g_coop_enable_lives->integer)
			for (size_t i = 0; i < game.maxclients; i++)
				game.clients[i].pers.lives = min(g_coop_num_lives->integer + 1, game.clients[i].pers.lives + 1);
	}

	// scan for all new maps we can go to, for secret levels
	edict_t *changelevel = nullptr;
	while ((changelevel = G_FindByString<&edict_t::classname>(changelevel, "target_changelevel")))
	{
		if (!changelevel->map || !*changelevel->map)
			continue;

		// next unit map, don't count it
		if (strchr(changelevel->map, '*'))
			continue;

		const char *level = strchr(changelevel->map, '+');

		if (level)
			level++;
		else
			level = changelevel->map;

		// don't include end screen levels
		if (strstr(level, ".cin") || strstr(level, ".pcx"))
			continue;

		size_t level_length;

		const char *spawnpoint = strchr(level, '$');

		if (spawnpoint)
			level_length = spawnpoint - level;
		else
			level_length = strlen(level);

		// make an entry for this level that we may or may not visit
		level_entry_t *found_entry = nullptr;

		for (size_t i = 0; i < MAX_LEVELS_PER_UNIT; i++)
		{
			level_entry_t *entry = &game.level_entries[i];

			if (!*entry->map_name || !strncmp(entry->map_name, level, level_length))
			{
				found_entry = entry;
				break;
			}
		}

		if (!found_entry)
		{
			gi.Com_PrintFmt("WARNING: more than {} maps in unit, can't track the rest\n", MAX_LEVELS_PER_UNIT);
			return;
		}

		Q_strlcpy(found_entry->map_name, level, min(level_length + 1, sizeof(found_entry->map_name)));
	}
}

/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the game.  This will happen every level load.
============
*/
void ClientBegin(edict_t *ent)
{
	ent->client = game.clients + (ent - g_edicts - 1);
	ent->client->awaiting_respawn = false;
	ent->client->respawn_timeout = 0_ms;

	// [Paril-KEX] we're always connected by this point...
	ent->client->pers.connected = true;

	if (deathmatch->integer)
	{
		ClientBeginDeathmatch(ent);
		return;
	}

	// [Paril-KEX] set enter time now, so we can send messages slightly
	// after somebody first joins
	ent->client->resp.entertime = level.time;
	ent->client->pers.spawned = true;

	// if there is already a body waiting for us (a loadgame), just
	// take it, otherwise spawn one from scratch
	if (ent->inuse)
	{
		// the client has cleared the client side viewangles upon
		// connecting to the server, which is different than the
		// state when the game is saved, so we need to compensate
		// with deltaangles
		ent->client->ps.pmove.delta_angles = ent->client->ps.viewangles;
	}
	else
	{
		// a spawn point will completely reinitialize the entity
		// except for the persistant data that was initialized at
		// ClientConnect() time
		G_InitEdict(ent);
		ent->classname = "player";
		InitClientResp(ent->client);
		spawn_from_begin = true;
		PutClientInServer(ent);
		spawn_from_begin = false;
	}
	
	// make sure we have a known default
	ent->svflags |= SVF_PLAYER;

	if (level.intermissiontime)
	{
		MoveClientToIntermission(ent);
	}
	else
	{
		// send effect if in a multiplayer game
		if (game.maxclients > 1 && !(ent->svflags & SVF_NOCLIENT))
			gi.LocBroadcast_Print(PRINT_HIGH, "$g_entered_game", ent->client->pers.netname);
	}

	level.coop_scale_players++;
	G_Monster_CheckCoopHealthScaling();

	// make sure all view stuff is valid
	ClientEndServerFrame(ent);

	// [Paril-KEX] send them goal, if needed
	G_PlayerNotifyGoal(ent);

	// [Paril-KEX] we're going to set this here just to be certain
	// that the level entry timer only starts when a player is actually
	// *in* the level
	G_SetLevelEntry();
}

/*
================
P_GetLobbyUserNum
================
*/
unsigned int P_GetLobbyUserNum( const edict_t * player ) {
	unsigned int playerNum = 0;
	if ( player > g_edicts && player < g_edicts + MAX_EDICTS ) {
		playerNum = ( player - g_edicts ) - 1;
		if ( playerNum >= MAX_CLIENTS ) {
			playerNum = 0;
		}
	}
	return playerNum;
}

/*
================
G_EncodedPlayerName

Gets a token version of the players "name" to be decoded on the client.
================
*/
std::string G_EncodedPlayerName(edict_t* player)
{
	unsigned int playernum = P_GetLobbyUserNum( player );
	return std::string("##P") + std::to_string(playernum);
}

/*
===========
ClientUserInfoChanged

called whenever the player updates a userinfo variable.
============
*/
void ClientUserinfoChanged(edict_t *ent, const char *userinfo)
{
	// set name
	if (!gi.Info_ValueForKey(userinfo, "name", ent->client->pers.netname, sizeof(ent->client->pers.netname)))
		Q_strlcpy(ent->client->pers.netname, "badinfo", sizeof(ent->client->pers.netname));

	// set spectator
	char val[MAX_INFO_VALUE] = { 0 };
	gi.Info_ValueForKey(userinfo, "spectator", val, sizeof(val));

	// spectators are only supported in deathmatch
	if (deathmatch->integer && !G_TeamplayEnabled() && *val && strcmp(val, "0"))
		ent->client->pers.spectator = true;
	else
		ent->client->pers.spectator = false;

	// set skin
	if (!gi.Info_ValueForKey(userinfo, "skin", val, sizeof(val)))
		Q_strlcpy(val, "male/grunt", sizeof(val));

	int playernum = ent - g_edicts - 1;

	// combine name and skin into a configstring
	// ZOID
	if (G_TeamplayEnabled())
		CTFAssignSkin(ent, val);
	else
	{
		// set dogtag
		char dogtag[MAX_INFO_VALUE] = { 0 };
		gi.Info_ValueForKey(userinfo, "dogtag", dogtag, sizeof(dogtag));

		// ZOID
		gi.configstring(CS_PLAYERSKINS + playernum, G_Fmt("{}\\{}\\{}", ent->client->pers.netname, val, dogtag).data());
	}

	// ZOID
	//  set player name field (used in id_state view)
	gi.configstring(CONFIG_CTF_PLAYER_NAME + playernum, ent->client->pers.netname);
	// ZOID

	// [Kex] netname is used for a couple of other things, so we update this after those.
	if ( ( ent->svflags & SVF_BOT ) == 0 ) {
		Q_strlcpy( ent->client->pers.netname, G_EncodedPlayerName( ent ).c_str(), sizeof( ent->client->pers.netname ) );
	}

	// fov
	gi.Info_ValueForKey(userinfo, "fov", val, sizeof(val));
	ent->client->ps.fov = clamp((float) atoi(val), 1.f, 160.f);

	// handedness
	if (gi.Info_ValueForKey(userinfo, "hand", val, sizeof(val)))
	{
		ent->client->pers.hand = static_cast<handedness_t>(clamp(atoi(val), (int32_t) RIGHT_HANDED, (int32_t) CENTER_HANDED));
	}
	else
	{
		ent->client->pers.hand = RIGHT_HANDED;
	}

	// [Paril-KEX] auto-switch
	if (gi.Info_ValueForKey(userinfo, "autoswitch", val, sizeof(val)))
	{
		ent->client->pers.autoswitch = static_cast<auto_switch_t>(clamp(atoi(val), (int32_t)auto_switch_t::SMART, (int32_t)auto_switch_t::NEVER));
	}
	else
	{
		ent->client->pers.autoswitch = auto_switch_t::SMART;
	}

	if (gi.Info_ValueForKey(userinfo, "autoshield", val, sizeof(val)))
	{
		ent->client->pers.autoshield = atoi(val);
	}
	else
	{
		ent->client->pers.autoshield = -1;
	}

	// [Paril-KEX] wants bob
	if (gi.Info_ValueForKey(userinfo, "bobskip", val, sizeof(val)))
	{
		ent->client->pers.bob_skip = val[0] == '1';
	}
	else
	{
		ent->client->pers.bob_skip = false;
	}

	// save off the userinfo in case we want to check something later
	Q_strlcpy(ent->client->pers.userinfo, userinfo, sizeof(ent->client->pers.userinfo));
}

inline bool IsSlotIgnored(edict_t *slot, edict_t **ignore, size_t num_ignore)
{
	for (size_t i = 0; i < num_ignore; i++)
		if (slot == ignore[i])
			return true;

	return false;
}

inline edict_t *ClientChooseSlot_Any(edict_t **ignore, size_t num_ignore)
{
	for (size_t i = 0; i < game.maxclients; i++)
		if (!IsSlotIgnored(globals.edicts + i + 1, ignore, num_ignore) && !game.clients[i].pers.connected)
			return globals.edicts + i + 1;

	return nullptr;
}

inline edict_t *ClientChooseSlot_Coop(const char *userinfo, const char *social_id, bool isBot, edict_t **ignore, size_t num_ignore)
{
	char name[MAX_INFO_VALUE] = { 0 };
	gi.Info_ValueForKey(userinfo, "name", name, sizeof(name));

	// the host should always occupy slot 0, some systems rely on this
	// (CHECK: is this true? is it just bots?)
	{
		size_t num_players = 0;

		for (size_t i = 0; i < game.maxclients; i++)
			if (IsSlotIgnored(globals.edicts + i + 1, ignore, num_ignore) || game.clients[i].pers.connected)
				num_players++;

		if (!num_players)
		{
			gi.Com_PrintFmt("coop slot {} is host {}+{}\n", 1, name, social_id);
			return globals.edicts + 1;
		}
	}

	// grab matches from players that we have connected
	using match_type_t = int32_t;
	enum {
		MATCH_USERNAME,
		MATCH_SOCIAL,
		MATCH_BOTH,

		MATCH_TYPES
	};

	struct {
		edict_t *slot = nullptr;
		size_t total = 0;
	} matches[MATCH_TYPES];

	for (size_t i = 0; i < game.maxclients; i++)
	{
		if (IsSlotIgnored(globals.edicts + i + 1, ignore, num_ignore) || game.clients[i].pers.connected)
			continue;

		char check_name[MAX_INFO_VALUE] = { 0 };
		gi.Info_ValueForKey(game.clients[i].pers.userinfo, "name", check_name, sizeof(check_name));

		bool username_match = game.clients[i].pers.userinfo[0] &&
			!strcmp(check_name, name);

		bool social_match = social_id && game.clients[i].pers.social_id[0] &&
			!strcmp(game.clients[i].pers.social_id, social_id);

		match_type_t type = (match_type_t) 0;

		if (username_match)
			type |= MATCH_USERNAME;
		if (social_match)
			type |= MATCH_SOCIAL;

		if (!type)
			continue;

		matches[type].slot = globals.edicts + i + 1;
		matches[type].total++;
	}

	// pick matches in descending order, only if the total matches
	// is 1 in the particular set; this will prefer to pick
	// social+username matches first, then social, then username last.
	for (int32_t i = 2; i >= 0; i--)
	{
		if (matches[i].total == 1)
		{
			gi.Com_PrintFmt("coop slot {} restored for {}+{}\n", (ptrdiff_t) (matches[i].slot - globals.edicts), name, social_id);

			// spawn us a ghost now since we're gonna spawn eventually
			if (!matches[i].slot->inuse)
			{
				matches[i].slot->s.modelindex = MODELINDEX_PLAYER;
				matches[i].slot->solid = SOLID_BBOX;

				G_InitEdict(matches[i].slot);
				matches[i].slot->classname = "player";
				InitClientResp(matches[i].slot->client);
				spawn_from_begin = true;
				PutClientInServer(matches[i].slot);
				spawn_from_begin = false;

				// make sure we have a known default
				matches[i].slot->svflags |= SVF_PLAYER;

				matches[i].slot->sv.init = false;
				matches[i].slot->classname = "player";
				matches[i].slot->client->pers.connected = true;
				matches[i].slot->client->pers.spawned = true;
				P_AssignClientSkinnum(matches[i].slot);
				gi.linkentity(matches[i].slot);
			}

			return matches[i].slot;
		}
	}

	// in the case where we can't find a match, we're probably a new
	// player, so pick a slot that hasn't been occupied yet
	for (size_t i = 0; i < game.maxclients; i++)
		if (!IsSlotIgnored(globals.edicts + i + 1, ignore, num_ignore) && !game.clients[i].pers.userinfo[0])
		{
			gi.Com_PrintFmt("coop slot {} issuing new for {}+{}\n", i + 1, name, social_id);
			return globals.edicts + i + 1;
		}

	// all slots have some player data in them, we're forced to replace one.
	edict_t *any_slot = ClientChooseSlot_Any(ignore, num_ignore);

	gi.Com_PrintFmt("coop slot {} any slot for {}+{}\n", !any_slot ? -1 : (ptrdiff_t) (any_slot - globals.edicts), name, social_id);

	return any_slot;
}

// [Paril-KEX] for coop, we want to try to ensure that players will always get their
// proper slot back when they connect.
edict_t *ClientChooseSlot(const char *userinfo, const char *social_id, bool isBot, edict_t **ignore, size_t num_ignore, bool cinematic)
{
	// coop and non-bots is the only thing that we need to do special behavior on
	if (!cinematic && coop->integer && !isBot)
		return ClientChooseSlot_Coop(userinfo, social_id, isBot, ignore, num_ignore);

	// just find any free slot
	return ClientChooseSlot_Any(ignore, num_ignore);
}

/*
===========
ClientConnect

Called when a player begins connecting to the server.
The game can refuse entrance to a client by returning false.
If the client is allowed, the connection process will continue
and eventually get to ClientBegin()
Changing levels will NOT cause this to be called again, but
loadgames will.
============
*/
bool ClientConnect(edict_t *ent, char *userinfo, const char *social_id, bool isBot)
{
	// check to see if they are on the banned IP list
#if 0
	value = Info_ValueForKey(userinfo, "ip");
	if (SV_FilterPacket(value))
	{
		Info_SetValueForKey(userinfo, "rejmsg", "Banned.");
		return false;
	}
#endif

	// check for a spectator
	char value[MAX_INFO_VALUE] = { 0 };
	gi.Info_ValueForKey(userinfo, "spectator", value, sizeof(value));

	if (deathmatch->integer && *value && strcmp(value, "0"))
	{
		uint32_t i, numspec;

		if (*spectator_password->string &&
			strcmp(spectator_password->string, "none") &&
			strcmp(spectator_password->string, value))
		{
			gi.Info_SetValueForKey(userinfo, "rejmsg", "Spectator password required or incorrect.");
			return false;
		}

		// count spectators
		for (i = numspec = 0; i < game.maxclients; i++)
			if (g_edicts[i + 1].inuse && g_edicts[i + 1].client->pers.spectator)
				numspec++;

		if (numspec >= (uint32_t) maxspectators->integer)
		{
			gi.Info_SetValueForKey(userinfo, "rejmsg", "Server spectator limit is full.");
			return false;
		}
	}
	else
	{
		// check for a password ( if not a bot! )
		gi.Info_ValueForKey(userinfo, "password", value, sizeof(value));
		if ( !isBot && *password->string && strcmp(password->string, "none") &&
			strcmp(password->string, value))
		{
			gi.Info_SetValueForKey(userinfo, "rejmsg", "Password required or incorrect.");
			return false;
		}
	}

	// they can connect
	ent->client = game.clients + (ent - g_edicts - 1);

	// set up userinfo early
	ClientUserinfoChanged(ent, userinfo);

	// if there is already a body waiting for us (a loadgame), just
	// take it, otherwise spawn one from scratch
	if (ent->inuse == false)
	{
		// clear the respawning variables
		// ZOID -- force team join
		ent->client->resp.ctf_team = CTF_NOTEAM;
		ent->client->resp.id_state = true;
		// ZOID
		InitClientResp(ent->client);
		if (!game.autosaved || !ent->client->pers.weapon)
			InitClientPersistant(ent, ent->client);
	}

	// make sure we start with known default(s)
	ent->svflags = SVF_PLAYER;
	if ( isBot ) {
		ent->svflags |= SVF_BOT;
	}

	Q_strlcpy(ent->client->pers.social_id, social_id, sizeof(ent->client->pers.social_id));

	if (game.maxclients > 1)
	{
		// [Paril-KEX] fetch name because now netname is kinda unsuitable
		gi.Info_ValueForKey(userinfo, "name", value, sizeof(value));
		gi.LocClient_Print(nullptr, PRINT_HIGH, "$g_player_connected", value);
	}

	ent->client->pers.connected = true;

	// [Paril-KEX] force a state update
	ent->sv.init = false;
	return true;
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.
============
*/
void ClientDisconnect(edict_t *ent)
{
	if (!ent->client)
		return;

	// ZOID
	CTFDeadDropFlag(ent);
	CTFDeadDropTech(ent);
	// ZOID

	PlayerTrail_Destroy(ent);

	//============
	// ROGUE
	// make sure no trackers are still hurting us.
	if (ent->client->tracker_pain_time)
		RemoveAttackingPainDaemons(ent);

	if (ent->client->owned_sphere)
	{
		if (ent->client->owned_sphere->inuse)
			G_FreeEdict(ent->client->owned_sphere);
		ent->client->owned_sphere = nullptr;
	}

	if (gamerules->integer)
	{
		if (DMGame.PlayerDisconnect)
			DMGame.PlayerDisconnect(ent);
	}
	// ROGUE
	//============

	// send effect
	if (!(ent->svflags & SVF_NOCLIENT))
	{
		gi.WriteByte(svc_muzzleflash);
		gi.WriteEntity(ent);
		gi.WriteByte(MZ_LOGOUT);
		gi.multicast(ent->s.origin, MULTICAST_PVS, false);
	}

	gi.unlinkentity(ent);
	ent->s.modelindex = 0;
	ent->solid = SOLID_NOT;
	ent->inuse = false;
	ent->sv.init = false;
	ent->classname = "disconnected";
	ent->client->pers.connected = false;
	ent->client->pers.spawned = false;
	ent->timestamp = level.time + 1_sec;

	// update active scoreboards
	if (deathmatch->integer)
		for (auto player : active_players())
			if (player->client->showscores)
				player->client->menutime = level.time;
}

//==============================================================

trace_t SV_PM_Clip(const vec3_t &start, const vec3_t *mins, const vec3_t *maxs, const vec3_t &end, contents_t mask)
{
	return gi.game_import_t::clip(world, start, mins, maxs, end, mask);
}

bool G_ShouldPlayersCollide(bool weaponry)
{
	if (g_disable_player_collision->integer) 
		return false; // only for debugging.

	// always collide on dm
	if (!coop->integer)
		return true;

	// weaponry collides if friendly fire is enabled
	if (weaponry && g_friendly_fire->integer)
		return true;

	// check collision cvar
	return g_coop_player_collision->integer;
}

/*
=================
P_FallingDamage

Paril-KEX: this is moved here and now reacts directly
to ClientThink rather than being delayed.
=================
*/
void P_FallingDamage(edict_t *ent, const pmove_t &pm)
{
	int	   damage;
	vec3_t dir;

	// dead stuff can't crater
	if (ent->health <= 0 || ent->deadflag)
		return;

	if (ent->s.modelindex != MODELINDEX_PLAYER)
		return; // not in the player model

	if (ent->movetype == MOVETYPE_NOCLIP)
		return;

	// never take falling damage if completely underwater
	if (pm.waterlevel == WATER_UNDER)
		return;

	// ZOID
	//  never take damage if just release grapple or on grapple
	if (ent->client->ctf_grapplereleasetime >= level.time ||
		(ent->client->ctf_grapple &&
		 ent->client->ctf_grapplestate > CTF_GRAPPLE_STATE_FLY))
		return;
	// ZOID

	float delta = pm.impact_delta;

	delta = delta * delta * 0.0001f;

	if (pm.waterlevel == WATER_WAIST)
		delta *= 0.25f;
	if (pm.waterlevel == WATER_FEET)
		delta *= 0.5f;

	if (delta < 1)
		return;

	// restart footstep timer
	ent->client->bobtime = 0;

	if (ent->client->landmark_free_fall)
	{
		delta = min(30.f, delta);
		ent->client->landmark_free_fall = false;
		ent->client->landmark_noise_time = level.time + 100_ms;
	}

	if (delta < 15)
	{
		if (!(pm.s.pm_flags & PMF_ON_LADDER))
			ent->s.event = EV_FOOTSTEP;
		return;
	}

	ent->client->fall_value = delta * 0.5f;
	if (ent->client->fall_value > 40)
		ent->client->fall_value = 40;
	ent->client->fall_time = level.time + FALL_TIME();

	if (delta > 30)
	{
		if (delta >= 55)
			ent->s.event = EV_FALLFAR;
		else
			ent->s.event = EV_FALL;

		ent->pain_debounce_time = level.time + FRAME_TIME_S; // no normal pain sound
		damage = (int) ((delta - 30) / 2);
		if (damage < 1)
			damage = 1;
		dir = { 0, 0, 1 };

		if (!deathmatch->integer || !g_dm_no_fall_damage->integer)
			T_Damage(ent, world, world, dir, ent->s.origin, vec3_origin, damage, 0, DAMAGE_NONE, MOD_FALLING);
	}
	else
		ent->s.event = EV_FALLSHORT;

	// Paril: falling damage noises alert monsters
	if (ent->health)
		PlayerNoise(ent, pm.s.origin, PNOISE_SELF);
}

bool HandleMenuMovement(edict_t *ent, usercmd_t *ucmd)
{
	if (!ent->client->menu)
		return false;

	// [Paril-KEX] handle menu movement
	int32_t menu_sign = ucmd->forwardmove > 0 ? 1 : ucmd->forwardmove < 0 ? -1 : 0;

	if (ent->client->menu_sign != menu_sign)
	{
		ent->client->menu_sign = menu_sign;

		if (menu_sign > 0)
		{
			PMenu_Prev(ent);
			return true;
		}
		else if (menu_sign < 0)
		{
			PMenu_Next(ent);
			return true;
		}
	}

	if (ent->client->latched_buttons & (BUTTON_ATTACK | BUTTON_JUMP))
	{
		PMenu_Select(ent);
		return true;
	}

	return false;
}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame.
==============
*/
void ClientThink(edict_t *ent, usercmd_t *ucmd)
{
	gclient_t *client;
	edict_t	  *other;
	uint32_t   i;
	pmove_t	   pm;

	level.current_entity = ent;
	client = ent->client;

	// [Paril-KEX] pass buttons through even if we are in intermission or
	// chasing.
	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;
	client->cmd = *ucmd;

	if ((ucmd->buttons & BUTTON_CROUCH) && pm_config.n64_physics)
	{
		if (client->pers.n64_crouch_warn_times < 12 &&
			client->pers.n64_crouch_warning < level.time &&
			(++client->pers.n64_crouch_warn_times % 3) == 0)
		{
			client->pers.n64_crouch_warning = level.time + 10_sec;
			gi.LocClient_Print(ent, PRINT_CENTER, "$g_n64_crouching");
		}
	}

	if (level.intermissiontime || ent->client->awaiting_respawn)
	{
		client->ps.pmove.pm_type = PM_FREEZE;

		bool n64_sp = false;

		if (level.intermissiontime)
		{
			n64_sp = !deathmatch->integer && level.is_n64;

			// can exit intermission after five seconds
			// Paril: except in N64. the camera handles it.
			// Paril again: except on unit exits, we can leave immediately after camera finishes
			if (level.changemap && (!n64_sp || level.level_intermission_set) && level.time > level.intermissiontime + 5_sec && (ucmd->buttons & BUTTON_ANY))
				level.exitintermission = true;
		}

		if (!n64_sp)
			client->ps.pmove.viewheight = ent->viewheight = 22;
		else
			client->ps.pmove.viewheight = ent->viewheight = 0;
		ent->movetype = MOVETYPE_NOCLIP;
		return;
	}

	if (ent->client->chase_target)
	{
		client->resp.cmd_angles = ucmd->angles;
		ent->movetype = MOVETYPE_NOCLIP;
	}
	else
	{

		// set up for pmove
		memset(&pm, 0, sizeof(pm));
		
		if (ent->movetype == MOVETYPE_NOCLIP)
		{
			if (ent->client->menu)
			{
				client->ps.pmove.pm_type = PM_FREEZE;
				
				// [Paril-KEX] handle menu movement
				HandleMenuMovement(ent, ucmd);
			}
			else if (ent->client->awaiting_respawn)
				client->ps.pmove.pm_type = PM_FREEZE;
			else if (ent->client->resp.spectator || (G_TeamplayEnabled() && ent->client->resp.ctf_team == CTF_NOTEAM))
				client->ps.pmove.pm_type = PM_SPECTATOR;
			else
				client->ps.pmove.pm_type = PM_NOCLIP;
		}
		else if (ent->s.modelindex != MODELINDEX_PLAYER)
			client->ps.pmove.pm_type = PM_GIB;
		else if (ent->deadflag)
			client->ps.pmove.pm_type = PM_DEAD;
		else if (ent->client->ctf_grapplestate >= CTF_GRAPPLE_STATE_PULL)
			client->ps.pmove.pm_type = PM_GRAPPLE;
		else
			client->ps.pmove.pm_type = PM_NORMAL;

		// [Paril-KEX]
		if (!G_ShouldPlayersCollide(false) ||
				(coop->integer && !(ent->clipmask & CONTENTS_PLAYER)) // if player collision is on and we're temporarily ghostly...
			)
			client->ps.pmove.pm_flags |= PMF_IGNORE_PLAYER_COLLISION;
		else
			client->ps.pmove.pm_flags &= ~PMF_IGNORE_PLAYER_COLLISION;

		// PGM	trigger_gravity support
		client->ps.pmove.gravity = (short) (level.gravity * ent->gravity);
		pm.s = client->ps.pmove;

		pm.s.origin = ent->s.origin;
		pm.s.velocity = ent->velocity;

		if (memcmp(&client->old_pmove, &pm.s, sizeof(pm.s)))
			pm.snapinitial = true;

		pm.cmd = *ucmd;
		pm.player = ent;
		pm.trace = gi.game_import_t::trace;
		pm.clip = SV_PM_Clip;
		pm.pointcontents = gi.pointcontents;
		pm.viewoffset = ent->client->ps.viewoffset;

		// perform a pmove
		Pmove(&pm);

		if (pm.groundentity && ent->groundentity)
		{
			float stepsize = fabs(ent->s.origin[2] - pm.s.origin[2]);

			if (stepsize > 4.f && stepsize < STEPSIZE)
			{
				ent->s.renderfx |= RF_STAIR_STEP;
				ent->client->step_frame = gi.ServerFrame() + 1;
			}
		}

		P_FallingDamage(ent, pm);

		if (ent->client->landmark_free_fall && pm.groundentity)
		{
			ent->client->landmark_free_fall = false;
			ent->client->landmark_noise_time = level.time + 100_ms;
		}

		// [Paril-KEX] save old position for G_TouchProjectiles
		vec3_t old_origin = ent->s.origin;

		ent->s.origin = pm.s.origin;
		ent->velocity = pm.s.velocity;

		// [Paril-KEX] if we stepped onto/off of a ladder, reset the
		// last ladder pos
		if ((pm.s.pm_flags & PMF_ON_LADDER) != (client->ps.pmove.pm_flags & PMF_ON_LADDER))
		{
			client->last_ladder_pos = ent->s.origin;

			if (pm.s.pm_flags & PMF_ON_LADDER)
			{
				if (!deathmatch->integer && 
					client->last_ladder_sound < level.time)
				{
					ent->s.event = EV_LADDER_STEP;
					client->last_ladder_sound = level.time + LADDER_SOUND_TIME;
				}
			}
		}

		// save results of pmove
		client->ps.pmove = pm.s;
		client->old_pmove = pm.s;

		ent->mins = pm.mins;
		ent->maxs = pm.maxs;

		if (!ent->client->menu)
			client->resp.cmd_angles = ucmd->angles;

		if (pm.jump_sound && !(pm.s.pm_flags & PMF_ON_LADDER))
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("*jump1.wav"), 1, ATTN_NORM, 0);
			// Paril: removed to make ambushes more effective and to
			// not have monsters around corners come to jumps
			// PlayerNoise(ent, ent->s.origin, PNOISE_SELF);
		}

		// ROGUE sam raimi cam support
		if (ent->flags & FL_SAM_RAIMI)
			ent->viewheight = 8;
		else
			ent->viewheight = (int) pm.s.viewheight;
		// ROGUE

		ent->waterlevel = pm.waterlevel;
		ent->watertype = pm.watertype;
		ent->groundentity = pm.groundentity;
		if (pm.groundentity)
			ent->groundentity_linkcount = pm.groundentity->linkcount;

		if (ent->deadflag)
		{
			client->ps.viewangles[ROLL] = 40;
			client->ps.viewangles[PITCH] = -15;
			client->ps.viewangles[YAW] = client->killer_yaw;
		}
		else if (!ent->client->menu)
		{
			client->v_angle = pm.viewangles;
			client->ps.viewangles = pm.viewangles;
			AngleVectors(client->v_angle, client->v_forward, nullptr, nullptr);
		}

		// ZOID
		if (client->ctf_grapple)
			CTFGrapplePull(client->ctf_grapple);
		// ZOID

		gi.linkentity(ent);

		// PGM trigger_gravity support
		ent->gravity = 1.0;
		// PGM

		if (ent->movetype != MOVETYPE_NOCLIP)
		{
			G_TouchTriggers(ent);
			G_TouchProjectiles(ent, old_origin);
		}

		// touch other objects
		for (i = 0; i < pm.touch.num; i++)
		{
			trace_t &tr = pm.touch.traces[i];
			other = tr.ent;

			if (other->touch)
				other->touch(other, ent, tr, true);
		}
	}

	// fire weapon from final position if needed
	if (client->latched_buttons & BUTTON_ATTACK)
	{
		if (client->resp.spectator)
		{
			client->latched_buttons = BUTTON_NONE;

			if (client->chase_target)
			{
				client->chase_target = nullptr;
				client->ps.pmove.pm_flags &= ~(PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION);
			}
			else
				GetChaseTarget(ent);
		}
		else if (!ent->client->weapon_thunk)
		{
			// we can only do this during a ready state and
			// if enough time has passed from last fire
			if (ent->client->weaponstate == WEAPON_READY)
			{
				ent->client->weapon_fire_buffered = true;

				if (ent->client->weapon_fire_finished <= level.time)
				{
					ent->client->weapon_thunk = true;
					Think_Weapon(ent);
				}
			}
		}
	}

	if (client->resp.spectator)
	{
		if (!HandleMenuMovement(ent, ucmd))
		{
			if (ucmd->buttons & BUTTON_JUMP)
			{
				if (!(client->ps.pmove.pm_flags & PMF_JUMP_HELD))
				{
					client->ps.pmove.pm_flags |= PMF_JUMP_HELD;
					if (client->chase_target)
						ChaseNext(ent);
					else
						GetChaseTarget(ent);
				}
			}
			else
				client->ps.pmove.pm_flags &= ~PMF_JUMP_HELD;
		}
	}

	// update chase cam if being followed
	for (i = 1; i <= game.maxclients; i++)
	{
		other = g_edicts + i;
		if (other->inuse && other->client->chase_target == ent)
			UpdateChaseCam(other);
	}
}

// active monsters
struct active_monsters_filter_t
{
	inline bool operator()(edict_t *ent) const
	{
		return (ent->inuse && (ent->svflags & SVF_MONSTER) && ent->health > 0);
	}
};

inline entity_iterable_t<active_monsters_filter_t> active_monsters()
{
	return entity_iterable_t<active_monsters_filter_t> { game.maxclients + (uint32_t)BODY_QUEUE_SIZE + 1U };
}

inline bool G_MonstersSearchingFor(edict_t *player)
{
	for (auto ent : active_monsters())
	{
		// check for *any* player target
		if (player == nullptr && ent->enemy && !ent->enemy->client)
			continue;
		// they're not targeting us, so who cares
		else if (player != nullptr && ent->enemy != player)
			continue;

		// they lost sight of us
		if ((ent->monsterinfo.aiflags & AI_LOST_SIGHT) && level.time > ent->monsterinfo.trail_time + 5_sec)
			continue;

		// no sir
		return true;
	}

	// yes sir
	return false;
}

// [Paril-KEX] from the given player, find a good spot to
// spawn a player
inline bool G_FindRespawnSpot(edict_t *player, vec3_t &spot)
{
	// sanity check; make sure there's enough room for ourselves.
	// (crouching in a small area, etc)
	trace_t tr = gi.trace(player->s.origin, PLAYER_MINS, PLAYER_MAXS, player->s.origin, player, MASK_PLAYERSOLID);

	if (tr.startsolid || tr.allsolid)
		return false;

	// throw five boxes a short-ish distance from the player and see if they land in a good, visible spot
	constexpr float yaw_spread[] = { 0, 90, 45, -45, -90 };
	constexpr float back_distance = 128.f;
	constexpr float up_distance = 128.f;
	constexpr float player_viewheight = 22.f;

	// we don't want to spawn inside of these
	contents_t mask = MASK_PLAYERSOLID | CONTENTS_LAVA | CONTENTS_SLIME;

	for (auto &yaw : yaw_spread)
	{
		vec3_t angles = { 0, (player->s.angles[YAW] + 180) + yaw, 0 };

		// throw the box three times:
		// one up & back
		// one back
		// one up, then back
		// pick the one that went the farthest
		vec3_t start = player->s.origin;
		vec3_t end = start + vec3_t { 0, 0, up_distance };

		tr = gi.trace(start, PLAYER_MINS, PLAYER_MAXS, end, player, mask);

		// stuck
		if (tr.startsolid || tr.allsolid || (tr.contents & (CONTENTS_LAVA | CONTENTS_SLIME)))
			continue;

		vec3_t fwd;
		AngleVectors(angles, fwd, nullptr, nullptr);

		start = tr.endpos;
		end = start + fwd * back_distance;

		tr = gi.trace(start, PLAYER_MINS, PLAYER_MAXS, end, player, mask);

		// stuck
		if (tr.startsolid || tr.allsolid || (tr.contents & (CONTENTS_LAVA | CONTENTS_SLIME)))
			continue;

		// plop us down now
		start = tr.endpos;
		end = tr.endpos - vec3_t { 0, 0, up_distance * 4 };

		tr = gi.trace(start, PLAYER_MINS, PLAYER_MAXS, end, player, mask);

		// stuck, or floating, or touching some other entity
		if (tr.startsolid || tr.allsolid || (tr.contents & (CONTENTS_LAVA | CONTENTS_SLIME)) || tr.fraction == 1.0f || tr.ent != world)
			continue;

		// don't spawn us *inside* liquids
		if (gi.pointcontents(tr.endpos + vec3_t{0, 0, player_viewheight}) & MASK_WATER)
			continue;

		// don't spawn us on steep slopes
		if (tr.plane.normal.z < 0.7f)
			continue;

		spot = tr.endpos;

		float z_diff = fabsf(player->s.origin[2] - tr.endpos[2]);

		// 5 steps is way too many steps
		if (z_diff > STEPSIZE * 4.f)
			continue;

		// if we went up or down 1 step, make sure we can still see their origin and their head
		if (z_diff > STEPSIZE)
		{
			tr = gi.traceline(player->s.origin, tr.endpos, player, mask);

			if (tr.fraction != 1.0f)
				continue;

			tr = gi.traceline(player->s.origin + vec3_t{0, 0, player_viewheight}, tr.endpos + vec3_t{0, 0, player_viewheight}, player, mask);

			if (tr.fraction != 1.0f)
				continue;
		}

		// good spot!
		return true;
	}

	return false;
}

// [Paril-KEX] check each player to find a good
// respawn target & position
inline std::tuple<edict_t *, vec3_t> G_FindSquadRespawnTarget()
{
	bool monsters_searching_for_anybody = G_MonstersSearchingFor(nullptr);

	for (auto player : active_players())
	{
		// no dead players
		if (player->deadflag)
			continue;
		
		// check combat state; we can't have taken damage recently
		if (player->client->last_damage_time >= level.time)
		{
			player->client->coop_respawn_state = COOP_RESPAWN_IN_COMBAT;
			continue;
		}

		// check if any monsters are currently targeting us
		// or searching for us
		if (G_MonstersSearchingFor(player))
		{
			player->client->coop_respawn_state = COOP_RESPAWN_IN_COMBAT;
			continue;
		}

		// check firing state; if any enemies are mad at any players,
		// don't respawn until everybody has cooled down
		if (monsters_searching_for_anybody && player->client->last_firing_time >= level.time)
		{
			player->client->coop_respawn_state = COOP_RESPAWN_IN_COMBAT;
			continue;
		}

		// check positioning; we must be on world ground
		if (player->groundentity != world)
		{
			player->client->coop_respawn_state = COOP_RESPAWN_BAD_AREA;
			continue;
		}

		// can't be in liquid
		if (player->waterlevel >= WATER_UNDER)
		{
			player->client->coop_respawn_state = COOP_RESPAWN_BAD_AREA;
			continue;
		}

		// good player; pick a spot
		vec3_t spot;
		
		if (!G_FindRespawnSpot(player, spot))
		{
			player->client->coop_respawn_state = COOP_RESPAWN_BLOCKED;
			continue;
		}

		// good player most likely
		return { player, spot };
	}

	// no good player
	return { nullptr, {} };
}

enum respawn_state_t
{
	RESPAWN_NONE,     // invalid state
	RESPAWN_SPECTATE, // move to spectator
	RESPAWN_SQUAD,    // move to good squad point
	RESPAWN_START     // move to start of map
};

// [Paril-KEX] return false to fall back to click-to-respawn behavior.
// note that this is only called if they are allowed to respawn (not
// restarting the level due to all being dead)
static bool G_CoopRespawn(edict_t *ent)
{
	// don't do this in non-coop
	if (!coop->integer)
		return false;
	// if we don't have squad or lives, it doesn't matter
	else if (!g_coop_squad_respawn->integer && !g_coop_enable_lives->integer)
		return false;

	respawn_state_t state = RESPAWN_NONE;

	// first pass: if we have no lives left, just move to spectator
	if (g_coop_enable_lives->integer)
	{
		if (ent->client->pers.lives == 0)
		{
			state = RESPAWN_SPECTATE;
			ent->client->coop_respawn_state = COOP_RESPAWN_NO_LIVES;
		}
	}

	// second pass: check for where to spawn
	if (state == RESPAWN_NONE)
	{
		// if squad respawn, don't respawn until we can find a good player to spawn on.
		if (g_coop_squad_respawn->integer)
		{
			bool allDead = true;

			for (auto player : active_players())
			{
				if (player->health > 0)
				{
					allDead = false;
					break;
				}
			}

			// all dead, so if we ever get here we have lives enabled;
			// we should just respawn at the start of the level
			if (allDead)
				state = RESPAWN_START;
			else
			{
				auto [ good_player, good_spot ] = G_FindSquadRespawnTarget();

				if (good_player) {
					state = RESPAWN_SQUAD;

					squad_respawn_position = good_spot;
					squad_respawn_angles = good_player->s.angles;
					squad_respawn_angles[2] = 0;

					use_squad_respawn = true;
				} else {
					state = RESPAWN_SPECTATE;
				}
			}
		}
		else
			state = RESPAWN_START;
	}

	if (state == RESPAWN_SQUAD || state == RESPAWN_START)
	{
		// give us our max health back since it will reset
		// to pers.health; in instanced items we'd lose the items
		// we touched so we always want to respawn with our max.
		if (P_UseCoopInstancedItems())
			ent->client->pers.health = ent->client->pers.max_health = ent->max_health;

		respawn(ent);

		ent->client->latched_buttons = BUTTON_NONE;
		use_squad_respawn = false;
	}
	else if (state == RESPAWN_SPECTATE)
	{
		if (!ent->client->coop_respawn_state)
			ent->client->coop_respawn_state = COOP_RESPAWN_WAITING;

		if (!ent->client->resp.spectator)
		{
			// move us to spectate just so we don't have to twiddle
			// our thumbs forever
			CopyToBodyQue(ent);
			ent->client->resp.spectator = true;
			ent->solid = SOLID_NOT;
			ent->takedamage = false;
			ent->s.modelindex = 0;
			ent->svflags |= SVF_NOCLIENT;
			ent->client->ps.damage_blend[3] = ent->client->ps.screen_blend[3] = 0;
			ent->client->ps.rdflags = RDF_NONE;
			ent->movetype = MOVETYPE_NOCLIP;
			// TODO: check if anything else needs to be reset
			gi.linkentity(ent);
			GetChaseTarget(ent);
		}
	}

	return true;
}

/*
==============
ClientBeginServerFrame

This will be called once for each server frame, before running
any other entities in the world.
==============
*/
void ClientBeginServerFrame(edict_t *ent)
{
	gclient_t *client;
	int		   buttonMask;

	if (gi.ServerFrame() != ent->client->step_frame)
		ent->s.renderfx &= ~RF_STAIR_STEP;

	if (level.intermissiontime)
		return;

	client = ent->client;

	if (client->awaiting_respawn)
	{
		if ((level.time.milliseconds() % 500) == 0)
			PutClientInServer(ent);
		return;
	}

	if ( ( ent->svflags & SVF_BOT ) != 0 ) {
		Bot_BeginFrame( ent );
	}

	if (deathmatch->integer && !G_TeamplayEnabled() &&
		client->pers.spectator != client->resp.spectator &&
		(level.time - client->respawn_time) >= 5_sec)
	{
		spectator_respawn(ent);
		return;
	}

	// run weapon animations if it hasn't been done by a ucmd_t
	if (!client->weapon_thunk && !client->resp.spectator)
		Think_Weapon(ent);
	else
		client->weapon_thunk = false;

	if (ent->deadflag)
	{
		// don't respawn if level is waiting to restart
		if (level.time > client->respawn_time && !level.coop_level_restart_time)
		{
			// check for coop handling
			if (!G_CoopRespawn(ent))
			{
				// in deathmatch, only wait for attack button
				if (deathmatch->integer)
					buttonMask = BUTTON_ATTACK;
				else
					buttonMask = -1;

				if ((client->latched_buttons & buttonMask) ||
					(deathmatch->integer && g_dm_force_respawn->integer))
				{
					respawn(ent);
					client->latched_buttons = BUTTON_NONE;
				}
			}
		}
		return;
	}

	// add player trail so monsters can follow
	if (!deathmatch->integer)
		PlayerTrail_Add(ent);

	client->latched_buttons = BUTTON_NONE;
}
/*
==============
RemoveAttackingPainDaemons

This is called to clean up the pain daemons that the disruptor attaches
to clients to damage them.
==============
*/
void RemoveAttackingPainDaemons(edict_t *self)
{
	edict_t *tracker;

	tracker = G_FindByString<&edict_t::classname>(nullptr, "pain daemon");
	while (tracker)
	{
		if (tracker->enemy == self)
			G_FreeEdict(tracker);
		tracker = G_FindByString<&edict_t::classname>(tracker, "pain daemon");
	}

	if (self->client)
		self->client->tracker_pain_time = 0_ms;
}
