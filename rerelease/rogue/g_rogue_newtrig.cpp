// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_newtrig.c
// pmack
// october 1997

#include "../g_local.h"

/*QUAKED info_teleport_destination (.5 .5 .5) (-16 -16 -24) (16 16 32)
Destination marker for a teleporter.
*/
void SP_info_teleport_destination(edict_t *self)
{
}

// unused; broken?
// constexpr uint32_t SPAWNFLAG_TELEPORT_PLAYER_ONLY	= 1;
// unused
// constexpr uint32_t SPAWNFLAG_TELEPORT_SILENT		= 2;
// unused
// constexpr uint32_t SPAWNFLAG_TELEPORT_CTF_ONLY		= 4;
constexpr spawnflags_t SPAWNFLAG_TELEPORT_START_ON = 8_spawnflag;

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
TOUCH(trigger_teleport_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	edict_t *dest;

	if (/*(self->spawnflags & SPAWNFLAG_TELEPORT_PLAYER_ONLY) &&*/ !(other->client))
		return;

	if (self->delay)
		return;

	dest = G_PickTarget(self->target);
	if (!dest)
	{
		gi.Com_Print("Teleport Destination not found!\n");
		return;
	}

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_TELEPORT_EFFECT);
	gi.WritePosition(other->s.origin);
	gi.multicast(other->s.origin, MULTICAST_PVS, false);

	other->s.origin = dest->s.origin;
	other->s.old_origin = dest->s.origin;
	other->s.origin[2] += 10;

	// clear the velocity and hold them in place briefly
	other->velocity = {};

	if (other->client)
	{
		other->client->ps.pmove.pm_time = 160; // hold time
		other->client->ps.pmove.pm_flags |= PMF_TIME_TELEPORT;

		// draw the teleport splash at source and on the player
		other->s.event = EV_PLAYER_TELEPORT;

		// set angles
		other->client->ps.pmove.delta_angles = dest->s.angles - other->client->resp.cmd_angles;

		other->client->ps.viewangles = {};
		other->client->v_angle = {};
	}

	other->s.angles = {};

	gi.linkentity(other);

	// kill anything at the destination
	KillBox(other, !!other->client);

	// [Paril-KEX] move sphere, if we own it
	if (other->client && other->client->owned_sphere)
	{
		edict_t *sphere = other->client->owned_sphere;
		sphere->s.origin = other->s.origin;
		sphere->s.origin[2] = other->absmax[2];
		sphere->s.angles[YAW] = other->s.angles[YAW];
		gi.linkentity(sphere);
	}
}

USE(trigger_teleport_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	if (self->delay)
		self->delay = 0;
	else
		self->delay = 1;
}

void SP_trigger_teleport(edict_t *self)
{
	if (!self->wait)
		self->wait = 0.2f;

	self->delay = 0;

	if (self->targetname)
	{
		self->use = trigger_teleport_use;
		if (!self->spawnflags.has(SPAWNFLAG_TELEPORT_START_ON))
			self->delay = 1;
	}

	self->touch = trigger_teleport_touch;

	self->solid = SOLID_TRIGGER;
	self->movetype = MOVETYPE_NONE;

	if (self->s.angles)
		G_SetMovedir(self->s.angles, self->movedir);

	gi.setmodel(self, self->model);
	gi.linkentity(self);
}

// ***************************
// TRIGGER_DISGUISE
// ***************************

/*QUAKED trigger_disguise (.5 .5 .5) ? TOGGLE START_ON REMOVE
Anything passing through this trigger when it is active will
be marked as disguised.

TOGGLE - field is turned off and on when used. (Paril N.B.: always the case)
START_ON - field is active when spawned.
REMOVE - field removes the disguise
*/

// unused
// constexpr uint32_t SPAWNFLAG_DISGUISE_TOGGLE	= 1;
constexpr spawnflags_t SPAWNFLAG_DISGUISE_START_ON = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAG_DISGUISE_REMOVE = 4_spawnflag;

TOUCH(trigger_disguise_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	if (other->client)
	{
		if (self->spawnflags.has(SPAWNFLAG_DISGUISE_REMOVE))
			other->flags &= ~FL_DISGUISED;
		else
			other->flags |= FL_DISGUISED;
	}
}

USE(trigger_disguise_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	if (self->solid == SOLID_NOT)
		self->solid = SOLID_TRIGGER;
	else
		self->solid = SOLID_NOT;

	gi.linkentity(self);
}

void SP_trigger_disguise(edict_t *self)
{
	if (!level.disguise_icon)
		level.disguise_icon = gi.imageindex("i_disguise");

	if (self->spawnflags.has(SPAWNFLAG_DISGUISE_START_ON))
		self->solid = SOLID_TRIGGER;
	else
		self->solid = SOLID_NOT;

	self->touch = trigger_disguise_touch;
	self->use = trigger_disguise_use;
	self->movetype = MOVETYPE_NONE;
	self->svflags = SVF_NOCLIENT;

	gi.setmodel(self, self->model);
	gi.linkentity(self);
}
