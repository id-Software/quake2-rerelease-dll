// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "../g_local.h"

/*QUAKED target_mal_laser (1 0 0) (-4 -4 -4) (4 4 4) START_ON RED GREEN BLUE YELLOW ORANGE FAT
Mal's laser
*/
void target_mal_laser_on(edict_t *self)
{
	if (!self->activator)
		self->activator = self;
	self->spawnflags |= SPAWNFLAG_LASER_ZAP | SPAWNFLAG_LASER_ON;
	self->svflags &= ~SVF_NOCLIENT;
	self->flags |= FL_TRAP;
	// target_laser_think (self);
	self->nextthink = level.time + gtime_t::from_sec(self->wait + self->delay);
}

USE(target_mal_laser_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	self->activator = activator;
	if (self->spawnflags.has(SPAWNFLAG_LASER_ON))
		target_laser_off(self);
	else
		target_mal_laser_on(self);
}

void mal_laser_think(edict_t *self);

THINK(mal_laser_think2) (edict_t *self) -> void
{
	self->svflags |= SVF_NOCLIENT;
	self->think = mal_laser_think;
	self->nextthink = level.time + gtime_t::from_sec(self->wait);
	self->spawnflags |= SPAWNFLAG_LASER_ZAP;
}

THINK(mal_laser_think) (edict_t *self) -> void
{
	self->svflags &= ~SVF_NOCLIENT;
	target_laser_think(self);
	self->think = mal_laser_think2;
	self->nextthink = level.time + 100_ms;
}

void SP_target_mal_laser(edict_t *self)
{
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	self->s.renderfx |= RF_BEAM;
	self->s.modelindex = MODELINDEX_WORLD; // must be non-zero
	self->flags |= FL_TRAP_LASER_FIELD;

	// set the beam diameter
	if (self->spawnflags.has(SPAWNFLAG_LASER_FAT))
		self->s.frame = 16;
	else
		self->s.frame = 4;

	// set the color
	if (self->spawnflags.has(SPAWNFLAG_LASER_RED))
		self->s.skinnum = 0xf2f2f0f0;
	else if (self->spawnflags.has(SPAWNFLAG_LASER_GREEN))
		self->s.skinnum = 0xd0d1d2d3;
	else if (self->spawnflags.has(SPAWNFLAG_LASER_BLUE))
		self->s.skinnum = 0xf3f3f1f1;
	else if (self->spawnflags.has(SPAWNFLAG_LASER_YELLOW))
		self->s.skinnum = 0xdcdddedf;
	else if (self->spawnflags.has(SPAWNFLAG_LASER_ORANGE))
		self->s.skinnum = 0xe0e1e2e3;

	G_SetMovedir(self->s.angles, self->movedir);

	if (!self->delay)
		self->delay = 0.1f;

	if (!self->wait)
		self->wait = 0.1f;

	if (!self->dmg)
		self->dmg = 5;

	self->mins = { -8, -8, -8 };
	self->maxs = { 8, 8, 8 };

	self->nextthink = level.time + gtime_t::from_sec(self->delay);
	self->think = mal_laser_think;

	self->use = target_mal_laser_use;

	gi.linkentity(self);

	if (self->spawnflags.has(SPAWNFLAG_LASER_ON))
		target_mal_laser_on(self);
	else
		target_laser_off(self);
}
// END	15-APR-98
