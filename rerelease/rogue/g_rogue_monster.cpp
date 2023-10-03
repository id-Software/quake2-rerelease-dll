// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "../g_local.h"

// ROGUE
void monster_fire_blaster2(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, monster_muzzleflash_id_t flashtype, effects_t effect)
{
	fire_blaster2(self, start, dir, damage, speed, effect, false);
	monster_muzzleflash(self, start, flashtype);
}

void monster_fire_tracker(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, edict_t *enemy, monster_muzzleflash_id_t flashtype)
{
	fire_tracker(self, start, dir, damage, speed, enemy);
	monster_muzzleflash(self, start, flashtype);
}

void monster_fire_heatbeam(edict_t *self, const vec3_t &start, const vec3_t &dir, const vec3_t &offset, int damage, int kick, monster_muzzleflash_id_t flashtype)
{
	fire_heatbeam(self, start, dir, offset, damage, kick, true);
	monster_muzzleflash(self, start, flashtype);
}
// ROGUE

// ROGUE

void stationarymonster_start_go(edict_t *self);

THINK(stationarymonster_triggered_spawn) (edict_t *self) -> void
{
	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_NONE;
	self->svflags &= ~SVF_NOCLIENT;
	self->air_finished = level.time + 12_sec;
	gi.linkentity(self);

	KillBox(self, false);

	// FIXME - why doesn't this happen with real monsters?
	self->spawnflags &= ~SPAWNFLAG_MONSTER_TRIGGER_SPAWN;

	stationarymonster_start_go(self);

	if (self->enemy && !(self->spawnflags & SPAWNFLAG_MONSTER_AMBUSH) && !(self->enemy->flags & FL_NOTARGET))
	{
		if (!(self->enemy->flags & FL_DISGUISED)) // PGM
			FoundTarget(self);
		else // PMM - just in case, make sure to clear the enemy so FindTarget doesn't get confused
			self->enemy = nullptr;
	}
	else
	{
		self->enemy = nullptr;
	}
}

USE(stationarymonster_triggered_spawn_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	// we have a one frame delay here so we don't telefrag the guy who activated us
	self->think = stationarymonster_triggered_spawn;
	self->nextthink = level.time + FRAME_TIME_S;
	if (activator && activator->client)
		self->enemy = activator;
	self->use = monster_use;
}

void stationarymonster_triggered_start(edict_t *self)
{
	self->solid = SOLID_NOT;
	self->movetype = MOVETYPE_NONE;
	self->svflags |= SVF_NOCLIENT;
	self->nextthink = 0_ms;
	self->use = stationarymonster_triggered_spawn_use;
}

THINK(stationarymonster_start_go) (edict_t *self) -> void
{
	if (!self->yaw_speed)
		self->yaw_speed = 20;

	monster_start_go(self);

	if (self->spawnflags.has(SPAWNFLAG_MONSTER_TRIGGER_SPAWN))
		stationarymonster_triggered_start(self);
}

void stationarymonster_start(edict_t *self)
{
	self->flags |= FL_STATIONARY;
	self->think = stationarymonster_start_go;
	monster_start(self);

	// fix viewheight
	self->viewheight = 0;
}

void monster_done_dodge(edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_DODGING;
	if (self->monsterinfo.attack_state == AS_SLIDING)
		self->monsterinfo.attack_state = AS_STRAIGHT;
}

int32_t M_SlotsLeft(edict_t *self)
{
	return self->monsterinfo.monster_slots - self->monsterinfo.monster_used;
}
// ROGUE