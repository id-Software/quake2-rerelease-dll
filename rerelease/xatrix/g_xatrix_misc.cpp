// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "../g_local.h"

/*QUAKED misc_crashviper (1 .5 0) (-176 -120 -24) (176 120 72)
This is a large viper about to crash
*/
void SP_misc_crashviper(edict_t *ent)
{
	if (!ent->target)
	{
		gi.Com_PrintFmt("{}: no target\n", *ent);
		G_FreeEdict(ent);
		return;
	}

	if (!ent->speed)
		ent->speed = 300;

	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/ships/bigviper/tris.md2");
	ent->mins = { -16, -16, 0 };
	ent->maxs = { 16, 16, 32 };

	ent->think = func_train_find;
	ent->nextthink = level.time + 10_hz;
	ent->use = misc_viper_use;
	ent->svflags |= SVF_NOCLIENT;
	ent->moveinfo.accel = ent->moveinfo.decel = ent->moveinfo.speed = ent->speed;

	gi.linkentity(ent);
}

// RAFAEL
/*QUAKED misc_viper_missile (1 0 0) (-8 -8 -8) (8 8 8)
"dmg"	how much boom should the bomb make? the default value is 250
*/

USE(misc_viper_missile_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	vec3_t forward, right, up;
	vec3_t start, dir;
	vec3_t vec;

	AngleVectors(self->s.angles, forward, right, up);

	self->enemy = G_FindByString<&edict_t::targetname>(nullptr, self->target);

	vec = self->enemy->s.origin;

	start = self->s.origin;
	dir = vec - start;
	dir.normalize();

	monster_fire_rocket(self, start, dir, self->dmg, 500, MZ2_CHICK_ROCKET_1);

	self->nextthink = level.time + 10_hz;
	self->think = G_FreeEdict;
}

void SP_misc_viper_missile(edict_t *self)
{
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	self->mins = { -8, -8, -8 };
	self->maxs = { 8, 8, 8 };

	if (!self->dmg)
		self->dmg = 250;

	self->s.modelindex = gi.modelindex("models/objects/bomb/tris.md2");

	self->use = misc_viper_missile_use;
	self->svflags |= SVF_NOCLIENT;

	gi.linkentity(self);
}

// RAFAEL 17-APR-98
/*QUAKED misc_transport (1 0 0) (-8 -8 -8) (8 8 8)
Maxx's transport at end of game
*/
void SP_misc_transport(edict_t *ent)
{
	if (!ent->target)
	{
		gi.Com_PrintFmt("{}: no target\n", *ent);
		G_FreeEdict(ent);
		return;
	}

	if (!ent->speed)
		ent->speed = 300;

	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/objects/ship/tris.md2");

	ent->mins = { -16, -16, 0 };
	ent->maxs = { 16, 16, 32 };

	ent->think = func_train_find;
	ent->nextthink = level.time + 10_hz;
	ent->use = misc_strogg_ship_use;
	ent->svflags |= SVF_NOCLIENT;
	ent->moveinfo.accel = ent->moveinfo.decel = ent->moveinfo.speed = ent->speed;

	if (!(ent->spawnflags & SPAWNFLAG_TRAIN_START_ON))
		ent->spawnflags |= SPAWNFLAG_TRAIN_START_ON;

	gi.linkentity(ent);
}
// END 17-APR-98

/*QUAKED misc_amb4 (1 0 0) (-16 -16 -16) (16 16 16)
Mal's amb4 loop entity
*/
static cached_soundindex amb4sound;

THINK(amb4_think) (edict_t *ent) -> void
{
	ent->nextthink = level.time + 2.7_sec;
	gi.sound(ent, CHAN_VOICE, amb4sound, 1, ATTN_NONE, 0);
}

void SP_misc_amb4(edict_t *ent)
{
	ent->think = amb4_think;
	ent->nextthink = level.time + 1_sec;
	amb4sound.assign("world/amb4.wav");
	gi.linkentity(ent);
}

/*QUAKED misc_nuke (1 0 0) (-16 -16 -16) (16 16 16)
 */
extern void target_killplayers_use(edict_t *self, edict_t *other, edict_t *activator);

void SP_misc_nuke(edict_t *ent)
{
	ent->use = target_killplayers_use;
}