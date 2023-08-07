// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"

//void plat_CalcMove (edict_t *ent, vec3_t dest, void(*func)(edict_t*));
void Move_Calc (edict_t *ent, vec3_t dest, void(*func)(edict_t*));

void fd_secret_move1(edict_t *self);
void fd_secret_move2(edict_t *self);
void fd_secret_move3(edict_t *self);
void fd_secret_move4(edict_t *self);
void fd_secret_move5(edict_t *self);
void fd_secret_move6(edict_t *self);
void fd_secret_done(edict_t *self);

/*
=============================================================================

SECRET DOORS

=============================================================================
*/

#define SEC_OPEN_ONCE		1          // stays open
#define SEC_1ST_LEFT		2          // 1st move is left of arrow
#define SEC_1ST_DOWN		4          // 1st move is down from arrow
#define SEC_NO_SHOOT		8          // only opened by trigger
#define SEC_YES_SHOOT		16		   // shootable even if targeted
#define SEC_MOVE_RIGHT		32
#define SEC_MOVE_FORWARD	64

void fd_secret_use (edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t *ent;

//	gi.dprintf("fd_secret_use\n");
	if (self->flags & FL_TEAMSLAVE)
		return;

	// trigger all paired doors
	for (ent = self ; ent ; ent = ent->teamchain)
		Move_Calc(ent, ent->moveinfo.start_origin, fd_secret_move1);

}

void fd_secret_killed (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
//	gi.dprintf("fd_secret_killed\n");
	self->health = self->max_health;
	self->takedamage = DAMAGE_NO;

	if (self->flags & FL_TEAMSLAVE && self->teammaster && self->teammaster->takedamage != DAMAGE_NO)
		fd_secret_killed (self->teammaster, inflictor, attacker, damage, point);
	else
		fd_secret_use (self, inflictor, attacker);
}

// Wait after first movement...
void fd_secret_move1(edict_t *self) 
{
//	gi.dprintf("fd_secret_move1\n");
	self->nextthink = level.time + 1.0;
	self->think = fd_secret_move2;
}

// Start moving sideways w/sound...
void fd_secret_move2(edict_t *self)
{
//	gi.dprintf("fd_secret_move2\n");
	Move_Calc(self, self->moveinfo.end_origin, fd_secret_move3);
}

// Wait here until time to go back...
void fd_secret_move3(edict_t *self)
{
//	gi.dprintf("fd_secret_move3\n");
	if (!(self->spawnflags & SEC_OPEN_ONCE))
	{
		self->nextthink = level.time + self->wait;
		self->think = fd_secret_move4;
	}
}

// Move backward...
void fd_secret_move4(edict_t *self)
{
//	gi.dprintf("fd_secret_move4\n");
	Move_Calc(self, self->moveinfo.start_origin, fd_secret_move5);          
}

// Wait 1 second...
void fd_secret_move5(edict_t *self)
{
//	gi.dprintf("fd_secret_move5\n");
	self->nextthink = level.time + 1.0;
	self->think = fd_secret_move6;
}

void fd_secret_move6(edict_t *self)
{
//	gi.dprintf("fd_secret_move6\n");
	Move_Calc(self, self->move_origin, fd_secret_done);
}

void fd_secret_done(edict_t *self)
{
//	gi.dprintf("fd_secret_done\n");
	if (!self->targetname || self->spawnflags & SEC_YES_SHOOT)
	{
		self->health = 1;
		self->takedamage = DAMAGE_YES;
		self->die = fd_secret_killed;   
	}
}

void secret_blocked(edict_t *self, edict_t *other)
{
	if (!(self->flags & FL_TEAMSLAVE))
		T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 0, 0, MOD_CRUSH);

//	if (time < self->attack_finished)
//		return;
//	self->attack_finished = time + 0.5;
//	T_Damage (other, self, self, self->dmg);
}

/*
================
secret_touch

Prints messages
================
*/
void secret_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other->health <= 0)
		return;

	if (!(other->client))
		return;

	if (self->monsterinfo.attack_finished > level.time)
		return;

	self->monsterinfo.attack_finished = level.time + 2;
	
	if (self->message)
	{
		gi.centerprintf (other, self->message);
//		fixme - put this sound back??
//		gi.sound (other, CHAN_BODY, "misc/talk.wav", 1, ATTN_NORM);
	}
}


/*QUAKED func_door_secret2 (0 .5 .8) ? open_once 1st_left 1st_down no_shoot always_shoot slide_right slide_forward
Basic secret door. Slides back, then to the left. Angle determines direction.

FLAGS:
open_once = not implemented yet
1st_left = 1st move is left/right of arrow
1st_down = 1st move is forwards/backwards
no_shoot = not implemented yet
always_shoot = even if targeted, keep shootable
reverse_left = the sideways move will be to right of arrow
reverse_back = the to/fro move will be forward

VALUES:
wait = # of seconds before coming back (5 default)
dmg  = damage to inflict when blocked (2 default)

*/

void SP_func_door_secret2 (edict_t *ent)
{
	vec3_t	forward,right,up;
	float	lrSize, fbSize;

	ent->moveinfo.sound_start = gi.soundindex  ("doors/dr1_strt.wav");
	ent->moveinfo.sound_middle = gi.soundindex  ("doors/dr1_mid.wav");
	ent->moveinfo.sound_end = gi.soundindex  ("doors/dr1_end.wav");

	if (!ent->dmg)
		ent->dmg = 2;
		
	AngleVectors(ent->s.angles, forward, right, up);
	VectorCopy(ent->s.origin, ent->move_origin);
	VectorCopy(ent->s.angles, ent->move_angles);

	G_SetMovedir (ent->s.angles, ent->movedir);
	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_BSP;
	gi.setmodel (ent, ent->model);

	if(ent->move_angles[1] == 0 || ent->move_angles[1] == 180)
	{
		lrSize = ent->size[1];
		fbSize = ent->size[0];
	}		
	else if(ent->move_angles[1] == 90 || ent->move_angles[1] == 270)
	{
		lrSize = ent->size[0];
		fbSize = ent->size[1];
	}		
	else
	{
		gi.dprintf("Secret door not at 0,90,180,270!\n");
	}

	if(ent->spawnflags & SEC_MOVE_FORWARD)
		VectorScale(forward, fbSize, forward);
	else
	{
		VectorScale(forward, fbSize * -1 , forward);
	}

	if(ent->spawnflags & SEC_MOVE_RIGHT)
		VectorScale(right, lrSize, right);
	else
	{
		VectorScale(right, lrSize * -1, right);
	}

	if(ent->spawnflags & SEC_1ST_DOWN)
	{
		VectorAdd(ent->s.origin, forward, ent->moveinfo.start_origin);
		VectorAdd(ent->moveinfo.start_origin, right, ent->moveinfo.end_origin);
	}
	else
	{
		VectorAdd(ent->s.origin, right, ent->moveinfo.start_origin);
		VectorAdd(ent->moveinfo.start_origin, forward, ent->moveinfo.end_origin);
	}

	ent->touch = secret_touch;
	ent->blocked = secret_blocked;
	ent->use = fd_secret_use;
	ent->moveinfo.speed = 50;
	ent->moveinfo.accel = 50;
	ent->moveinfo.decel = 50;

	if (!ent->targetname || ent->spawnflags & SEC_YES_SHOOT)
	{
		ent->health = 1;
		ent->max_health = ent->health;
		ent->takedamage = DAMAGE_YES;
		ent->die = fd_secret_killed;
	}
	if (!ent->wait)
		ent->wait = 5;          // 5 seconds before closing

	gi.linkentity(ent);
}

// ==================================================

#define FWALL_START_ON		1

void force_wall_think(edict_t *self)
{
	if(!self->wait)
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_FORCEWALL);
		gi.WritePosition (self->pos1);
		gi.WritePosition (self->pos2);
		gi.WriteByte  (self->style);
		gi.multicast (self->offset, MULTICAST_PVS);
	}

	self->think = force_wall_think;
	self->nextthink = level.time + 0.1;
}

void force_wall_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if(!self->wait)
	{
		self->wait = 1;
		self->think = NULL;
		self->nextthink = 0;
		self->solid = SOLID_NOT;
		gi.linkentity( self );
	}
	else
	{
		self->wait = 0;
		self->think = force_wall_think;
		self->nextthink = level.time + 0.1;
		self->solid = SOLID_BSP;
		KillBox(self);		// Is this appropriate?
		gi.linkentity (self);
	}
}

/*QUAKED func_force_wall (1 0 1) ? start_on
A vertical particle force wall. Turns on and solid when triggered.
If someone is in the force wall when it turns on, they're telefragged.

start_on - forcewall begins activated. triggering will turn it off.
style - color of particles to use.
	208: green, 240: red, 241: blue, 224: orange
*/
void SP_func_force_wall(edict_t *ent)
{
	gi.setmodel (ent, ent->model);

	ent->offset[0] = (ent->absmax[0] + ent->absmin[0]) / 2;
	ent->offset[1] = (ent->absmax[1] + ent->absmin[1]) / 2;
	ent->offset[2] = (ent->absmax[2] + ent->absmin[2]) / 2;

	ent->pos1[2] = ent->absmax[2];
	ent->pos2[2] = ent->absmax[2];
	if(ent->size[0] > ent->size[1])
	{
		ent->pos1[0] = ent->absmin[0];
		ent->pos2[0] = ent->absmax[0];
		ent->pos1[1] = ent->offset[1];
		ent->pos2[1] = ent->offset[1];
	}
	else
	{
		ent->pos1[0] = ent->offset[0];
		ent->pos2[0] = ent->offset[0];
		ent->pos1[1] = ent->absmin[1];
		ent->pos2[1] = ent->absmax[1];
	}
	
	if(!ent->style)
		ent->style = 208;

	ent->movetype = MOVETYPE_NONE;
	ent->wait = 1;

	if(ent->spawnflags & FWALL_START_ON)
	{
		ent->solid = SOLID_BSP;
		ent->think = force_wall_think;
		ent->nextthink = level.time + 0.1;
	}
	else
		ent->solid = SOLID_NOT;

	ent->use = force_wall_use;

	ent->svflags = SVF_NOCLIENT;
	
	gi.linkentity(ent);
}
