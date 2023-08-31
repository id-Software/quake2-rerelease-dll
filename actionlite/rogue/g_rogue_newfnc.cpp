// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "../g_local.h"

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

constexpr spawnflags_t SPAWNFLAG_SEC_OPEN_ONCE = 1_spawnflag; // stays open
// unused
// constexpr uint32_t SPAWNFLAG_SEC_1ST_LEFT		= 2;         // 1st move is left of arrow
constexpr spawnflags_t SPAWNFLAG_SEC_1ST_DOWN = 4_spawnflag; // 1st move is down from arrow
// unused
// constexpr uint32_t SPAWNFLAG_SEC_NO_SHOOT		= 8;         // only opened by trigger
constexpr spawnflags_t SPAWNFLAG_SEC_YES_SHOOT = 16_spawnflag; // shootable even if targeted
constexpr spawnflags_t SPAWNFLAG_SEC_MOVE_RIGHT = 32_spawnflag;
constexpr spawnflags_t SPAWNFLAG_SEC_MOVE_FORWARD = 64_spawnflag;

USE(fd_secret_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	edict_t *ent;

	if (self->flags & FL_TEAMSLAVE)
		return;

	// trigger all paired doors
	for (ent = self; ent; ent = ent->teamchain)
		Move_Calc(ent, ent->moveinfo.start_origin, fd_secret_move1);
}

DIE(fd_secret_killed) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	self->health = self->max_health;
	self->takedamage = false;

	if (self->flags & FL_TEAMSLAVE && self->teammaster && self->teammaster->takedamage != false)
		fd_secret_killed(self->teammaster, inflictor, attacker, damage, point, mod);
	else
		fd_secret_use(self, inflictor, attacker);
}

// Wait after first movement...
MOVEINFO_ENDFUNC(fd_secret_move1) (edict_t *self) -> void
{
	self->nextthink = level.time + 1_sec;
	self->think = fd_secret_move2;
}

// Start moving sideways w/sound...
THINK(fd_secret_move2) (edict_t *self) -> void
{
	Move_Calc(self, self->moveinfo.end_origin, fd_secret_move3);
}

// Wait here until time to go back...
MOVEINFO_ENDFUNC(fd_secret_move3) (edict_t *self) -> void
{
	if (!self->spawnflags.has(SPAWNFLAG_SEC_OPEN_ONCE))
	{
		self->nextthink = level.time + gtime_t::from_sec(self->wait);
		self->think = fd_secret_move4;
	}
}

// Move backward...
THINK(fd_secret_move4) (edict_t *self) -> void
{
	Move_Calc(self, self->moveinfo.start_origin, fd_secret_move5);
}

// Wait 1 second...
MOVEINFO_ENDFUNC(fd_secret_move5) (edict_t *self) -> void
{
	self->nextthink = level.time + 1_sec;
	self->think = fd_secret_move6;
}

THINK(fd_secret_move6) (edict_t *self) -> void
{
	Move_Calc(self, self->move_origin, fd_secret_done);
}

MOVEINFO_ENDFUNC(fd_secret_done) (edict_t *self) -> void
{
	if (!self->targetname || self->spawnflags.has(SPAWNFLAG_SEC_YES_SHOOT))
	{
		self->health = 1;
		self->takedamage = true;
		self->die = fd_secret_killed;
	}
}

MOVEINFO_BLOCKED(secret_blocked) (edict_t *self, edict_t *other) -> void
{
	if (!(self->flags & FL_TEAMSLAVE))
		T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 0, DAMAGE_NONE, MOD_CRUSH);
}

/*
================
secret_touch

Prints messages
================
*/
TOUCH(secret_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	if (other->health <= 0)
		return;

	if (!(other->client))
		return;

	if (self->monsterinfo.attack_finished > level.time)
		return;

	self->monsterinfo.attack_finished = level.time + 2_sec;

	if (self->message)
		gi.LocCenter_Print(other, self->message);
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

void SP_func_door_secret2(edict_t *ent)
{
	vec3_t forward, right, up;
	float  lrSize, fbSize;

	G_SetMoveinfoSounds(ent, "doors/dr1_strt.wav", "doors/dr1_mid.wav", "doors/dr1_end.wav");

	if (!ent->dmg)
		ent->dmg = 2;

	AngleVectors(ent->s.angles, forward, right, up);
	ent->move_origin = ent->s.origin;
	ent->move_angles = ent->s.angles;

	G_SetMovedir(ent->s.angles, ent->movedir);
	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_BSP;
	gi.setmodel(ent, ent->model);

	if (ent->move_angles[1] == 0 || ent->move_angles[1] == 180)
	{
		lrSize = ent->size[1];
		fbSize = ent->size[0];
	}
	else if (ent->move_angles[1] == 90 || ent->move_angles[1] == 270)
	{
		lrSize = ent->size[0];
		fbSize = ent->size[1];
	}
	else
	{
		gi.Com_Print("Secret door not at 0,90,180,270!\n");
		G_FreeEdict(ent);
		return;
	}

	if (ent->spawnflags.has(SPAWNFLAG_SEC_MOVE_FORWARD))
		forward *= fbSize;
	else
		forward *= fbSize * -1;

	if (ent->spawnflags.has(SPAWNFLAG_SEC_MOVE_RIGHT))
		right *= lrSize;
	else
		right *= lrSize * -1;

	if (ent->spawnflags.has(SPAWNFLAG_SEC_1ST_DOWN))
	{
		ent->moveinfo.start_origin = ent->s.origin + forward;
		ent->moveinfo.end_origin = ent->moveinfo.start_origin + right;
	}
	else
	{
		ent->moveinfo.start_origin = ent->s.origin + right;
		ent->moveinfo.end_origin = ent->moveinfo.start_origin + forward;
	}

	ent->touch = secret_touch;
	ent->moveinfo.blocked = secret_blocked;
	ent->use = fd_secret_use;
	ent->moveinfo.speed = 50;
	ent->moveinfo.accel = 50;
	ent->moveinfo.decel = 50;

	if (!ent->targetname || ent->spawnflags.has(SPAWNFLAG_SEC_YES_SHOOT))
	{
		ent->health = 1;
		ent->max_health = ent->health;
		ent->takedamage = true;
		ent->die = fd_secret_killed;
	}
	if (!ent->wait)
		ent->wait = 5; // 5 seconds before closing

	gi.linkentity(ent);
}

// ==================================================

constexpr spawnflags_t SPAWNFLAG_FORCEWALL_START_ON = 1_spawnflag;

THINK(force_wall_think) (edict_t *self) -> void
{
	if (!self->wait)
	{
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_FORCEWALL);
		gi.WritePosition(self->pos1);
		gi.WritePosition(self->pos2);
		gi.WriteByte(self->style);
		gi.multicast(self->offset, MULTICAST_PVS, false);
	}

	self->think = force_wall_think;
	self->nextthink = level.time + 10_hz;
}

USE(force_wall_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	if (!self->wait)
	{
		self->wait = 1;
		self->think = nullptr;
		self->nextthink = 0_ms;
		self->solid = SOLID_NOT;
		gi.linkentity(self);
	}
	else
	{
		self->wait = 0;
		self->think = force_wall_think;
		self->nextthink = level.time + 10_hz;
		self->solid = SOLID_BSP;
		gi.linkentity(self);
		KillBox(self, false); // Is this appropriate?
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
	gi.setmodel(ent, ent->model);

	ent->offset[0] = (ent->absmax[0] + ent->absmin[0]) / 2;
	ent->offset[1] = (ent->absmax[1] + ent->absmin[1]) / 2;
	ent->offset[2] = (ent->absmax[2] + ent->absmin[2]) / 2;

	ent->pos1[2] = ent->absmax[2];
	ent->pos2[2] = ent->absmax[2];
	if (ent->size[0] > ent->size[1])
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

	if (!ent->style)
		ent->style = 208;

	ent->movetype = MOVETYPE_NONE;
	ent->wait = 1;

	if (ent->spawnflags.has(SPAWNFLAG_FORCEWALL_START_ON))
	{
		ent->solid = SOLID_BSP;
		ent->think = force_wall_think;
		ent->nextthink = level.time + 10_hz;
	}
	else
		ent->solid = SOLID_NOT;

	ent->use = force_wall_use;

	ent->svflags = SVF_NOCLIENT;

	gi.linkentity(ent);
}
