// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_newtrig.c
// pmack
// october 1997

#include "g_local.h"

#define TELEPORT_PLAYER_ONLY	1
#define TELEPORT_SILENT			2
#define TELEPORT_CTF_ONLY		4
#define TELEPORT_START_ON		8

extern void TeleportEffect (vec3_t origin);

/*QUAKED info_teleport_destination (.5 .5 .5) (-16 -16 -24) (16 16 32)
Destination marker for a teleporter.
*/
void SP_info_teleport_destination (edict_t *self)
{
}

/*QUAKED trigger_teleport (.5 .5 .5) ? player_only silent ctf_only start_on
Any object touching this will be transported to the corresponding 
info_teleport_destination entity. You must set the "target" field, 
and create an object with a "targetname" field that matches.

If the trigger_teleport has a targetname, it will only teleport 
entities when it has been fired.

player_only: only players are teleported
silent: <not used right now>
ctf_only: <not used right now>
start_on: when trigger has targetname, start active, deactivate when used.
*/
void trigger_teleport_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	edict_t *dest;
	int		i;

	if(/*(self->spawnflags & TELEPORT_PLAYER_ONLY) &&*/ !(other->client))
		return;

	if(self->delay)
		return;

	dest = G_Find (NULL, FOFS(targetname), self->target);
	if(!dest)
	{
		gi.dprintf("Teleport Destination not found!\n");
		return;
	}

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TELEPORT_EFFECT);
	gi.WritePosition (other->s.origin);
	gi.multicast (other->s.origin, MULTICAST_PVS);

	// unlink to make sure it can't possibly interfere with KillBox
	gi.unlinkentity (other);

	VectorCopy (dest->s.origin, other->s.origin);
	VectorCopy (dest->s.origin, other->s.old_origin);
	other->s.origin[2] += 10;

	// clear the velocity and hold them in place briefly
	VectorClear (other->velocity);
	if(other->client)
	{
		other->client->ps.pmove.pm_time = 160>>3;		// hold time
		other->client->ps.pmove.pm_flags |= PMF_TIME_TELEPORT;

		// draw the teleport splash at source and on the player
//		self->s.event = EV_PLAYER_TELEPORT;
		other->s.event = EV_PLAYER_TELEPORT;

		// set angles
		for (i=0 ; i<3 ; i++)
			other->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(dest->s.angles[i] - other->client->resp.cmd_angles[i]);

		VectorClear (other->client->ps.viewangles);
		VectorClear (other->client->v_angle);
	}


	VectorClear (other->s.angles);

	// kill anything at the destination
	KillBox (other);

	gi.linkentity (other);
}

void trigger_teleport_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if(self->delay)
		self->delay = 0;
	else
		self->delay = 1;
}

void SP_trigger_teleport(edict_t *self)
{
	if (!self->wait)
		self->wait = 0.2;

	self->delay = 0;
		
	if (self->targetname)
	{
		self->use = trigger_teleport_use;
		if(!(self->spawnflags & TELEPORT_START_ON))
			self->delay = 1;
	}

	self->touch = trigger_teleport_touch;

	self->solid = SOLID_TRIGGER;
	self->movetype = MOVETYPE_NONE;
//	self->flags |= FL_NOCLIENT;

	if (!VectorCompare(self->s.angles, vec3_origin))
		G_SetMovedir (self->s.angles, self->movedir);

	gi.setmodel (self, self->model);
	gi.linkentity (self);
}

// ***************************
// TRIGGER_DISGUISE
// ***************************

/*QUAKED trigger_disguise (.5 .5 .5) ? TOGGLE START_ON REMOVE
Anything passing through this trigger when it is active will
be marked as disguised.

TOGGLE - field is turned off and on when used.
START_ON - field is active when spawned.
REMOVE - field removes the disguise
*/

void trigger_disguise_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other->client)
	{
		if(self->spawnflags & 4)
			other->flags &= ~FL_DISGUISED;
		else
			other->flags |= FL_DISGUISED;
	}
}

void trigger_disguise_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if(self->solid == SOLID_NOT)
		self->solid = SOLID_TRIGGER;
	else
		self->solid = SOLID_NOT;

	gi.linkentity(self);
}

void SP_trigger_disguise (edict_t *self)
{
	if(self->spawnflags & 2)
		self->solid = SOLID_TRIGGER;
	else
		self->solid = SOLID_NOT;

	self->touch = trigger_disguise_touch;
	self->use = trigger_disguise_use;
	self->movetype = MOVETYPE_NONE;
	self->svflags = SVF_NOCLIENT;

	gi.setmodel (self, self->model);
	gi.linkentity(self);

}
