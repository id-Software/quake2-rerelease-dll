// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"

//==========================================================

/*QUAKED target_steam (1 0 0) (-8 -8 -8) (8 8 8)
Creates a steam effect (particles w/ velocity in a line).

  speed = velocity of particles (default 50)
  count = number of particles (default 32)
  sounds = color of particles (default 8 for steam)
     the color range is from this color to this color + 6
  wait = seconds to run before stopping (overrides default
     value derived from func_timer)

  best way to use this is to tie it to a func_timer that "pokes"
  it every second (or however long you set the wait time, above)

  note that the width of the base is proportional to the speed
  good colors to use:
  6-9 - varying whites (darker to brighter)
  224 - sparks
  176 - blue water
  80  - brown water
  208 - slime
  232 - blood
*/

void use_target_steam (edict_t *self, edict_t *other, edict_t *activator)
{
	// FIXME - this needs to be a global
	static int	nextid;
	vec3_t		point;

	if (nextid > 20000)
		nextid = nextid %20000;

	nextid++;
	
	// automagically set wait from func_timer unless they set it already, or
	// default to 1000 if not called by a func_timer (eek!)
	if (!self->wait)
		if (other)
			self->wait = other->wait * 1000;
		else
			self->wait = 1000;

	if (self->enemy)
	{
		VectorMA (self->enemy->absmin, 0.5, self->enemy->size, point);
		VectorSubtract (point, self->s.origin, self->movedir);
		VectorNormalize (self->movedir);
	}

	VectorMA (self->s.origin, self->plat2flags*0.5, self->movedir, point);
	if (self->wait > 100)
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_STEAM);
		gi.WriteShort (nextid);
		gi.WriteByte (self->count);
		gi.WritePosition (self->s.origin);
		gi.WriteDir (self->movedir);
		gi.WriteByte (self->sounds&0xff);
		gi.WriteShort ( (short int)(self->plat2flags) );
		gi.WriteLong ( (int)(self->wait) );
		gi.multicast (self->s.origin, MULTICAST_PVS);
	}
	else
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_STEAM);
		gi.WriteShort ((short int)-1);
		gi.WriteByte (self->count);
		gi.WritePosition (self->s.origin);
		gi.WriteDir (self->movedir);
		gi.WriteByte (self->sounds&0xff);
		gi.WriteShort ( (short int)(self->plat2flags) );
		gi.multicast (self->s.origin, MULTICAST_PVS);
	}
}

void target_steam_start (edict_t *self)
{
	edict_t	*ent;

	self->use = use_target_steam;

	if (self->target)
	{
		ent = G_Find (NULL, FOFS(targetname), self->target);
		if (!ent)
			gi.dprintf ("%s at %s: %s is a bad target\n", self->classname, vtos(self->s.origin), self->target);
		self->enemy = ent;
	}
	else
	{
		G_SetMovedir (self->s.angles, self->movedir);
	}

	if (!self->count)
		self->count = 32;
	if (!self->plat2flags)
		self->plat2flags = 75;
	if (!self->sounds)
		self->sounds = 8;
	if (self->wait)
		self->wait *= 1000;  // we want it in milliseconds, not seconds

	// paranoia is good
	self->sounds &= 0xff;
	self->count &= 0xff;

	self->svflags = SVF_NOCLIENT;

	gi.linkentity (self);
}

void SP_target_steam (edict_t *self)
{
	self->plat2flags = self->speed;

	if (self->target)
	{
		self->think = target_steam_start;
		self->nextthink = level.time + 1;
	}
	else
		target_steam_start (self);
}


//==========================================================
// target_anger
//==========================================================

void target_anger_use (edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t		*target;
	edict_t		*t;

	t = NULL;
	target = G_Find (t, FOFS(targetname), self->killtarget);

	if (target && self->target)
	{
		// Make whatever a "good guy" so the monster will try to kill it!
		target->monsterinfo.aiflags |= AI_GOOD_GUY;
		target->svflags |= SVF_MONSTER;
		target->health = 300;

		t = NULL;
		while ((t = G_Find (t, FOFS(targetname), self->target)))
		{
			if (t == self)
			{
				gi.dprintf ("WARNING: entity used itself.\n");
			}
			else
			{
				if (t->use)
				{
					if (t->health < 0)
					{
//						if ((g_showlogic) && (g_showlogic->value))
//							gi.dprintf ("target_anger with dead monster!\n");
						return;
					}
					t->enemy = target;
					t->monsterinfo.aiflags |= AI_TARGET_ANGER;
					FoundTarget (t);
				}
			}
			if (!self->inuse)
			{
				gi.dprintf("entity was removed while using targets\n");
				return;
			}
		}
	}

}

/*QUAKED target_anger (1 0 0) (-8 -8 -8) (8 8 8)
This trigger will cause an entity to be angry at another entity when a player touches it. Target the
entity you want to anger, and killtarget the entity you want it to be angry at.

target - entity to piss off
killtarget - entity to be pissed off at
*/
void SP_target_anger (edict_t *self)
{	
	if (!self->target)
	{
		gi.dprintf("target_anger without target!\n");
		G_FreeEdict (self);
		return;
	}
	if (!self->killtarget)
	{
		gi.dprintf("target_anger without killtarget!\n");
		G_FreeEdict (self);
		return;
	}

	self->use = target_anger_use;
	self->svflags = SVF_NOCLIENT;
}

// ================
// target_spawn
// ================
/*
extern edict_t *CreateMonster(vec3_t origin, vec3_t angles, char *classname);

void target_spawn_use (edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t		*newEnt;

	newEnt = CreateMonster (self->s.origin, self->s.angles, "monster_infantry");
	if(newEnt)
		newEnt->enemy = other;
}
*/

/*Q U AKED target_spawn (1 0 0) (-32 -32 -24) (32 32 72) 
*/
/*
void SP_target_spawn (edict_t *self)
{
	self->use = target_spawn_use;
	self->svflags = SVF_NOCLIENT;
}
*/

// ***********************************
// target_killplayers
// ***********************************

void target_killplayers_use (edict_t *self, edict_t *other, edict_t *activator)
{
	int		i;
	edict_t	*ent, *player;

	// kill the players
	for (i=0 ; i<game.maxclients ; i++)
	{
		player = &g_edicts[1+i];
		if (!player->inuse)
			continue;

		// nail it
		T_Damage (player, self, self, vec3_origin, self->s.origin, vec3_origin, 100000, 0, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);
	}

	// kill any visible monsters
	for (ent = g_edicts; ent < &g_edicts[globals.num_edicts] ; ent++)
	{
		if (!ent->inuse)
			continue;
		if (ent->health < 1)
			continue;
		if (!ent->takedamage)
			continue;
		
		for(i=0;i<game.maxclients ; i++)
		{
			player = &g_edicts[1+i];
			if(!player->inuse)
				continue;
			
			if(visible(player, ent))
			{
				T_Damage (ent, self, self, vec3_origin, ent->s.origin, vec3_origin, 
							ent->health, 0, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);
				break;
			}
		}
	}

}

/*QUAKED target_killplayers (1 0 0) (-8 -8 -8) (8 8 8)
When triggered, this will kill all the players on the map.
*/
void SP_target_killplayers (edict_t *self)
{
	self->use = target_killplayers_use;
	self->svflags = SVF_NOCLIENT;
}

/*QUAKED target_blacklight (1 0 1) (-16 -16 -24) (16 16 24) 
Pulsing black light with sphere in the center
*/
void blacklight_think (edict_t *self)
{
	self->s.angles[0] = rand()%360;
	self->s.angles[1] = rand()%360;
	self->s.angles[2] = rand()%360;
	self->nextthink = level.time + 0.1;
}

void SP_target_blacklight(edict_t *ent)
{
	if (deathmatch->value)
	{	// auto-remove for deathmatch
		G_FreeEdict (ent);
		return;
	}

	VectorClear (ent->mins);
	VectorClear (ent->maxs);

	ent->s.effects |= (EF_TRACKERTRAIL|EF_TRACKER);
	ent->think = blacklight_think;
	ent->s.modelindex = gi.modelindex ("models/items/spawngro2/tris.md2");
	ent->s.frame = 1;
	ent->nextthink = level.time + 0.1;
	gi.linkentity (ent);
}

/*QUAKED target_orb (1 0 1) (-16 -16 -24) (16 16 24) 
Translucent pulsing orb with speckles
*/
void orb_think (edict_t *self)
{
	self->s.angles[0] = rand()%360;
	self->s.angles[1] = rand()%360;
	self->s.angles[2] = rand()%360;
//	self->s.effects |= (EF_TRACKERTRAIL|EF_DOUBLE);
	self->nextthink = level.time + 0.1;
}

void SP_target_orb(edict_t *ent)
{
	if (deathmatch->value)
	{	// auto-remove for deathmatch
		G_FreeEdict (ent);
		return;
	}

	VectorClear (ent->mins);
	VectorClear (ent->maxs);

//	ent->s.effects |= EF_TRACKERTRAIL;
	ent->think = orb_think;
	ent->nextthink = level.time + 0.1;
	ent->s.modelindex = gi.modelindex ("models/items/spawngro2/tris.md2");
	ent->s.frame = 2;
	ent->s.effects |= EF_SPHERETRANS;
	gi.linkentity (ent);
}

