// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

TURRET

==============================================================================
*/

#include "g_local.h"
#include "m_turret.h"

#define SPAWN_BLASTER			0x0008
#define SPAWN_MACHINEGUN		0x0010
#define SPAWN_ROCKET			0x0020
#define SPAWN_HEATBEAM			0x0040
#define SPAWN_WEAPONCHOICE		0x0078
#define SPAWN_INSTANT_WEAPON	0x0050
#define SPAWN_WALL_UNIT			0x0080

extern qboolean FindTarget (edict_t *self);

void turret_run (edict_t *self);
void TurretAim (edict_t *self);
void turret_sight (edict_t *self, edict_t *other);
void turret_search (edict_t *self);
void turret_stand (edict_t *self);
void turret_wake (edict_t *self);
void turret_ready_gun (edict_t *self);
void turret_run (edict_t *self);

void turret_attack (edict_t *self);
mmove_t turret_move_fire;
mmove_t turret_move_fire_blind;


void TurretAim(edict_t *self)
{
	vec3_t	end, dir;
	vec3_t	ang;
	float	move, idealPitch, idealYaw, current, speed;
	int		orientation;

// gi.dprintf("turret_aim: %d %d\n", self->s.frame, self->monsterinfo.nextframe);

	if(!self->enemy || self->enemy == world)
	{
		if(!FindTarget (self))
			return;
	}

	// if turret is still in inactive mode, ready the gun, but don't aim
	if(self->s.frame < FRAME_active01)
	{
		turret_ready_gun(self);
		return;
	}
	// if turret is still readying, don't aim.
	if(self->s.frame < FRAME_run01)
		return;

	// PMM - blindfire aiming here
	if (self->monsterinfo.currentmove == &turret_move_fire_blind)
	{
		VectorCopy(self->monsterinfo.blind_fire_target, end);
		if (self->enemy->s.origin[2] < self->monsterinfo.blind_fire_target[2])
			end[2] += self->enemy->viewheight + 10;
		else
			end[2] += self->enemy->mins[2] - 10;
	}
	else
	{
		VectorCopy(self->enemy->s.origin, end);
		if (self->enemy->client)
			end[2] += self->enemy->viewheight;
	}

	VectorSubtract(end, self->s.origin, dir);
	vectoangles2(dir, ang);

	//
	// Clamp first
	//

	idealPitch = ang[PITCH];
	idealYaw = ang[YAW];

	orientation = self->offset[1];
	switch(orientation)
	{
		case -1:			// up		pitch: 0 to 90
			if(idealPitch < -90)
				idealPitch += 360;
			if(idealPitch > -5)
				idealPitch = -5;
			break;
		case -2:			// down		pitch: -180 to -360
			if(idealPitch > -90)
				idealPitch -= 360;
			if(idealPitch < -355)
				idealPitch = -355;
			else if(idealPitch > -185)
				idealPitch = -185;
			break;
		case 0:				// +X		pitch: 0 to -90, -270 to -360 (or 0 to 90)
//gi.dprintf("idealpitch %0.1f  idealyaw %0.1f\n", idealPitch, idealYaw);
			if(idealPitch < -180)
				idealPitch += 360;

			if(idealPitch > 85)
				idealPitch = 85;
			else if(idealPitch < -85)
				idealPitch = -85;

//gi.dprintf("idealpitch %0.1f  idealyaw %0.1f\n", idealPitch, idealYaw);
							//			yaw: 270 to 360, 0 to 90
							//			yaw: -90 to 90 (270-360 == -90-0)
			if(idealYaw > 180)
				idealYaw -= 360;
			if(idealYaw > 85)
				idealYaw = 85;
			else if(idealYaw < -85)
				idealYaw = -85;
//gi.dprintf("idealpitch %0.1f  idealyaw %0.1f\n", idealPitch, idealYaw);
			break;
		case 90:			// +Y	pitch: 0 to 90, -270 to -360 (or 0 to 90)
			if(idealPitch < -180)
				idealPitch += 360;

			if(idealPitch > 85)
				idealPitch = 85;
			else if(idealPitch < -85)
				idealPitch = -85;

							//			yaw: 0 to 180
			if(idealYaw > 270)
				idealYaw -= 360;
			if(idealYaw > 175)	idealYaw = 175;
			else if(idealYaw < 5)	idealYaw = 5;

			break;
		case 180:			// -X	pitch: 0 to 90, -270 to -360 (or 0 to 90)
			if(idealPitch < -180)
				idealPitch += 360;

			if(idealPitch > 85)
				idealPitch = 85;
			else if(idealPitch < -85)
				idealPitch = -85;

							//			yaw: 90 to 270
			if(idealYaw > 265)	idealYaw = 265;
			else if(idealYaw < 95)	idealYaw = 95;

			break;
		case 270:			// -Y	pitch: 0 to 90, -270 to -360 (or 0 to 90)
			if(idealPitch < -180)
				idealPitch += 360;

			if(idealPitch > 85)
				idealPitch = 85;
			else if(idealPitch < -85)
				idealPitch = -85;

							//			yaw: 180 to 360
			if(idealYaw < 90)
				idealYaw += 360;
			if(idealYaw > 355)	idealYaw = 355;
			else if(idealYaw < 185)	idealYaw = 185;
			break;
	}

	//
	// adjust pitch
	//
	current = self->s.angles[PITCH];
	speed = self->yaw_speed;

	if(idealPitch != current)
	{
		move = idealPitch - current;

		while(move >= 360)
			move -= 360;
		if (move >= 90)
		{
			move = move - 360;
		}

		while(move <= -360)
			move += 360;
		if (move <= -90)
		{
			move = move + 360;
		}

		if (move > 0)
		{
			if (move > speed)
				move = speed;
		}
		else
		{
			if (move < -speed)
				move = -speed;
		}

		self->s.angles[PITCH] = anglemod (current + move);
	}

	//
	// adjust yaw
	//
	current = self->s.angles[YAW];
	speed = self->yaw_speed;

	if(idealYaw != current)
	{
		move = idealYaw - current;

//		while(move >= 360)
//			move -= 360;
		if (move >= 180)
		{
			move = move - 360;
		}

//		while(move <= -360)
//			move += 360;
		if (move <= -180)
		{
			move = move + 360;
		}

		if (move > 0)
		{
			if (move > speed)
				move = speed;
		}
		else
		{
			if (move < -speed)
				move = -speed;
		}

		self->s.angles[YAW] = anglemod (current + move);
	}

}

void turret_sight (edict_t *self, edict_t *other)
{
}

void turret_search (edict_t *self)
{
}

mframe_t turret_frames_stand [] =
{
	ai_stand, 0, NULL,
	ai_stand, 0, NULL
};
mmove_t turret_move_stand = {FRAME_stand01, FRAME_stand02, turret_frames_stand, NULL};

void turret_stand (edict_t *self)
{
//gi.dprintf("turret_stand\n");
	self->monsterinfo.currentmove = &turret_move_stand;
}

mframe_t turret_frames_ready_gun [] =
{
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	
	ai_stand, 0, NULL
};
mmove_t turret_move_ready_gun = { FRAME_active01, FRAME_run01, turret_frames_ready_gun, turret_run };

void turret_ready_gun (edict_t *self)
{
	self->monsterinfo.currentmove = &turret_move_ready_gun;
}

mframe_t turret_frames_seek [] =
{
	ai_walk, 0, TurretAim,
	ai_walk, 0, TurretAim
};
mmove_t turret_move_seek = {FRAME_run01, FRAME_run02, turret_frames_seek, NULL};

void turret_walk (edict_t *self)
{
	if(self->s.frame < FRAME_run01)
		turret_ready_gun(self);
	else
		self->monsterinfo.currentmove = &turret_move_seek;
}


mframe_t turret_frames_run [] =
{
	ai_run, 0, TurretAim,
	ai_run, 0, TurretAim
};
mmove_t turret_move_run = {FRAME_run01, FRAME_run02, turret_frames_run, turret_run};

void turret_run (edict_t *self)
{
	if(self->s.frame < FRAME_run01)
		turret_ready_gun(self);
	else
		self->monsterinfo.currentmove = &turret_move_run;
}

// **********************
//  ATTACK
// **********************

#define TURRET_BULLET_DAMAGE	4
#define TURRET_HEAT_DAMAGE		4

void TurretFire (edict_t *self)
{
	vec3_t	forward;
	vec3_t	start, end, dir;
	float	time, dist, chance;
	trace_t	trace;
	int		rocketSpeed;

	TurretAim(self);

	if(!self->enemy || !self->enemy->inuse)
		return;

	VectorSubtract(self->enemy->s.origin, self->s.origin, dir);
	VectorNormalize(dir);
	AngleVectors(self->s.angles, forward, NULL, NULL);
	chance = DotProduct(dir, forward);
	if(chance < 0.98)
	{
//		gi.dprintf("off-angle\n");
		return;
	}

	chance = random();

	// rockets fire less often than the others do.
	if (self->spawnflags & SPAWN_ROCKET)
	{
		chance = chance * 3;

		rocketSpeed = 550;
		if (skill->value == 2)
		{
			rocketSpeed += 200 * random();
		}
		else if (skill->value == 3)
		{
			rocketSpeed += 100 + (200 * random());
		}
	}
	else if (self->spawnflags & SPAWN_BLASTER)
	{
		if (skill->value == 0)
			rocketSpeed = 600;
		else if (skill->value == 1)
			rocketSpeed = 800;
		else
			rocketSpeed = 1000;
		chance = chance * 2;
	}
	
	// up the fire chance 20% per skill level.
	chance = chance - (0.2 * skill->value);

	if(/*chance < 0.5 && */visible(self, self->enemy))
	{
		VectorCopy(self->s.origin, start);
		VectorCopy(self->enemy->s.origin, end);
		
		// aim for the head.
		if ((self->enemy) && (self->enemy->client))
			end[2]+=self->enemy->viewheight;
		else
			end[2]+=22;

		VectorSubtract(end, start, dir);
		dist = VectorLength(dir);
		
		// check for predictive fire if distance less than 512
		if(!(self->spawnflags & SPAWN_INSTANT_WEAPON) && (dist<512))
		{
			chance = random();
			// ramp chance. easy - 50%, avg - 60%, hard - 70%, nightmare - 80%
			chance += (3 - skill->value) * 0.1;
			if(chance < 0.8)
			{
				// lead the target....
				time = dist / 1000;
				VectorMA(end, time, self->enemy->velocity, end);
				VectorSubtract(end, start, dir);
			}
		}

		VectorNormalize(dir);
		trace = gi.trace(start, vec3_origin, vec3_origin, end, self, MASK_SHOT);
		if(trace.ent == self->enemy || trace.ent == world)
		{
			if(self->spawnflags & SPAWN_BLASTER)
				monster_fire_blaster(self, start, dir, 20, rocketSpeed, MZ2_TURRET_BLASTER, EF_BLASTER);
			else if(self->spawnflags & SPAWN_MACHINEGUN)
				monster_fire_bullet (self, start, dir, TURRET_BULLET_DAMAGE, 0, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MZ2_TURRET_MACHINEGUN);
			else if(self->spawnflags & SPAWN_ROCKET)
			{
				if(dist * trace.fraction > 72)
					monster_fire_rocket (self, start, dir, 50, rocketSpeed, MZ2_TURRET_ROCKET);
			}
		}	
	}
}

// PMM
void TurretFireBlind (edict_t *self)
{
	vec3_t	forward;
	vec3_t	start, end, dir;
	float	dist, chance;
	int		rocketSpeed;

	TurretAim(self);

	if(!self->enemy || !self->enemy->inuse)
		return;

	VectorSubtract(self->monsterinfo.blind_fire_target, self->s.origin, dir);
	VectorNormalize(dir);
	AngleVectors(self->s.angles, forward, NULL, NULL);
	chance = DotProduct(dir, forward);
	if(chance < 0.98)
	{
//		gi.dprintf("off-angle\n");
		return;
	}

	if (self->spawnflags & SPAWN_ROCKET)
	{
		rocketSpeed = 550;
		if (skill->value == 2)
		{
			rocketSpeed += 200 * random();
		}
		else if (skill->value == 3)
		{
			rocketSpeed += 100 + (200 * random());
		}
	}

	VectorCopy(self->s.origin, start);
	VectorCopy(self->monsterinfo.blind_fire_target, end);
		
	if (self->enemy->s.origin[2] < self->monsterinfo.blind_fire_target[2])
		end[2] += self->enemy->viewheight + 10;
	else
		end[2] += self->enemy->mins[2] - 10;

	VectorSubtract(end, start, dir);
	dist = VectorLength(dir);
		
	VectorNormalize(dir);

	if(self->spawnflags & SPAWN_BLASTER)
		monster_fire_blaster(self, start, dir, 20, 1000, MZ2_TURRET_BLASTER, EF_BLASTER);
	else if(self->spawnflags & SPAWN_ROCKET)
		monster_fire_rocket (self, start, dir, 50, rocketSpeed, MZ2_TURRET_ROCKET);
}
//pmm

mframe_t turret_frames_fire [] =
{
	ai_run,   0, TurretFire,
	ai_run,   0, TurretAim,
	ai_run,   0, TurretAim,
	ai_run,   0, TurretAim
};
mmove_t turret_move_fire = {FRAME_pow01, FRAME_pow04, turret_frames_fire, turret_run};

//PMM

// the blind frames need to aim first
mframe_t turret_frames_fire_blind [] =
{
	ai_run,   0, TurretAim,
	ai_run,   0, TurretAim,
	ai_run,   0, TurretAim,
	ai_run,   0, TurretFireBlind
};
mmove_t turret_move_fire_blind = {FRAME_pow01, FRAME_pow04, turret_frames_fire_blind, turret_run};
//pmm

void turret_attack(edict_t *self)
{
	float r, chance;

	if(self->s.frame < FRAME_run01)
		turret_ready_gun(self);
	// PMM
	else if (self->monsterinfo.attack_state != AS_BLIND)
	{
		self->monsterinfo.nextframe = FRAME_pow01;
		self->monsterinfo.currentmove = &turret_move_fire;
	}
	else
	{
		// setup shot probabilities
		if (self->monsterinfo.blind_fire_delay < 1.0)
			chance = 1.0;
		else if (self->monsterinfo.blind_fire_delay < 7.5)
			chance = 0.4;
		else
			chance = 0.1;

		r = random();

		// minimum of 3 seconds, plus 0-4, after the shots are done - total time should be max less than 7.5
		self->monsterinfo.blind_fire_delay += 0.4 + 3.0 + random()*4.0;
		// don't shoot at the origin
		if (VectorCompare (self->monsterinfo.blind_fire_target, vec3_origin))
			return;

		// don't shoot if the dice say not to
		if (r > chance)
			return;

		self->monsterinfo.nextframe = FRAME_pow01;
		self->monsterinfo.currentmove = &turret_move_fire_blind;
	}
	// pmm
}

// **********************
//  PAIN
// **********************

void turret_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	return;
}

// **********************
//  DEATH
// **********************

void turret_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	vec3_t		forward;
	vec3_t		start;
	edict_t		*base;

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_PLAIN_EXPLOSION);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PHS);

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorMA(self->s.origin, 1, forward, start);

	ThrowDebris (self, "models/objects/debris1/tris.md2", 1, start);
	ThrowDebris (self, "models/objects/debris1/tris.md2", 2, start);
	ThrowDebris (self, "models/objects/debris1/tris.md2", 1, start);
	ThrowDebris (self, "models/objects/debris1/tris.md2", 2, start);

	if(self->teamchain)
	{
		base = self->teamchain;
		base->solid = SOLID_BBOX;
		base->takedamage = DAMAGE_NO;
		base->movetype = MOVETYPE_NONE;
		gi.linkentity (base);
	}

	if(self->target)
	{
		if(self->enemy && self->enemy->inuse)
			G_UseTargets (self, self->enemy);
		else
			G_UseTargets (self, self);
	}

	G_FreeEdict(self);
}

// **********************
//  WALL SPAWN
// **********************

void turret_wall_spawn (edict_t *turret)
{
	edict_t		*ent;
	int			angle;

	ent = G_Spawn();
	VectorCopy(turret->s.origin, ent->s.origin);
	VectorCopy(turret->s.angles, ent->s.angles);
	
	angle = ent->s.angles[1];
	if(ent->s.angles[0] == 90)
		angle = -1;
	else if(ent->s.angles[0] == 270)
		angle = -2;
	switch (angle)
	{
		case -1:
			VectorSet(ent->mins, -16, -16, -8);
			VectorSet(ent->maxs, 16, 16, 0);
			break;
		case -2:
			VectorSet(ent->mins, -16, -16, 0);
			VectorSet(ent->maxs, 16, 16, 8);
			break;
		case 0:
			VectorSet(ent->mins, -8, -16, -16);
			VectorSet(ent->maxs, 0, 16, 16);
			break;
		case 90:
			VectorSet(ent->mins, -16, -8, -16);
			VectorSet(ent->maxs, 16, 0, 16);
			break;
		case 180:
			VectorSet(ent->mins, 0, -16, -16);
			VectorSet(ent->maxs, 8, 16, 16);
			break;
		case 270:
			VectorSet(ent->mins, -16, 0, -16);
			VectorSet(ent->maxs, 16, 8, 16);
			break;

	}

	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_NOT;

	ent->teammaster = turret;
	turret->teammaster = turret;
	turret->teamchain = ent;
	ent->teamchain = NULL;
	ent->flags |= FL_TEAMSLAVE;
	ent->owner = turret;

	ent->s.modelindex = gi.modelindex("models/monsters/turretbase/tris.md2");

	gi.linkentity (ent);
}

void turret_wake (edict_t *self)
{
	// the wall section will call this when it stops moving.
	// just return without doing anything. easiest way to have a null function.
	if(self->flags & FL_TEAMSLAVE)
	{
		return;
	}

	self->monsterinfo.stand = turret_stand;
	self->monsterinfo.walk = turret_walk;
	self->monsterinfo.run = turret_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = turret_attack;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = turret_sight;
	self->monsterinfo.search = turret_search;
	self->monsterinfo.currentmove = &turret_move_stand;
	self->takedamage = DAMAGE_AIM;
	self->movetype = MOVETYPE_NONE;
	// prevent counting twice
	self->monsterinfo.aiflags |= AI_DO_NOT_COUNT;

	gi.linkentity (self);

	stationarymonster_start (self);
	
	if(self->spawnflags & SPAWN_MACHINEGUN)
	{
		self->s.skinnum = 1;
	}
	else if(self->spawnflags & SPAWN_ROCKET)
	{
		self->s.skinnum = 2;
	}

	// but we do want the death to count
	self->monsterinfo.aiflags &= ~AI_DO_NOT_COUNT;
}

extern void Move_Calc (edict_t *ent, vec3_t dest, void(*func)(edict_t*));

void turret_activate (edict_t *self, edict_t *other, edict_t *activator)
{
	vec3_t		endpos;
	vec3_t		forward;
	edict_t		*base;

	self->movetype = MOVETYPE_PUSH;
	if(!self->speed)
		self->speed = 15;
	self->moveinfo.speed = self->speed;
	self->moveinfo.accel = self->speed;
	self->moveinfo.decel = self->speed;

	if(self->s.angles[0] == 270)
	{
		VectorSet (forward, 0,0,1);
	}
	else if(self->s.angles[0] == 90)
	{
		VectorSet (forward, 0,0,-1);
	}
	else if(self->s.angles[1] == 0)
	{
		VectorSet (forward, 1,0,0);
	}
	else if(self->s.angles[1] == 90)
	{
		VectorSet (forward, 0,1,0);
	}
	else if(self->s.angles[1] == 180)
	{
		VectorSet (forward, -1,0,0);
	}
	else if(self->s.angles[1] == 270)
	{
		VectorSet (forward, 0,-1,0);
	}
	
	// start up the turret
	VectorMA(self->s.origin, 32, forward, endpos);
	Move_Calc(self, endpos, turret_wake);

	base = self->teamchain;
	if(base)
	{
		base->movetype = MOVETYPE_PUSH;
		base->speed = self->speed;
		base->moveinfo.speed = base->speed;
		base->moveinfo.accel = base->speed;
		base->moveinfo.decel = base->speed;

		// start up the wall section
		VectorMA(self->teamchain->s.origin, 32, forward, endpos);
		Move_Calc(self->teamchain, endpos, turret_wake);
	}

	gi.sound (self, CHAN_VOICE, gi.soundindex ("world/dr_short.wav"), 1, ATTN_NORM, 0);
}

// PMM
// checkattack .. ignore range, just attack if available
qboolean turret_checkattack (edict_t *self)
{
	vec3_t	spot1, spot2;
	float	chance, nexttime;
	trace_t	tr;
	int		enemy_range;

	if (self->enemy->health > 0)
	{
	// see if any entities are in the way of the shot
		VectorCopy (self->s.origin, spot1);
		spot1[2] += self->viewheight;
		VectorCopy (self->enemy->s.origin, spot2);
		spot2[2] += self->enemy->viewheight;

		tr = gi.trace (spot1, NULL, NULL, spot2, self, CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_SLIME|CONTENTS_LAVA|CONTENTS_WINDOW);

		// do we have a clear shot?
		if (tr.ent != self->enemy)
		{	
			// PGM - we want them to go ahead and shoot at info_notnulls if they can.
			if(self->enemy->solid != SOLID_NOT || tr.fraction < 1.0)		//PGM
			{
				// PMM - if we can't see our target, and we're not blocked by a monster, go into blind fire if available
				if ((!(tr.ent->svflags & SVF_MONSTER)) && (!visible(self, self->enemy)))
				{
					if ((self->monsterinfo.blindfire) && (self->monsterinfo.blind_fire_delay <= 10.0))
					{
						if (level.time < self->monsterinfo.attack_finished)
						{
							return false;
						}
						if (level.time < (self->monsterinfo.trail_time + self->monsterinfo.blind_fire_delay))
						{
							// wait for our time
							return false;
						}
						else
						{
							// make sure we're not going to shoot something we don't want to shoot
							tr = gi.trace (spot1, NULL, NULL, self->monsterinfo.blind_fire_target, self, CONTENTS_MONSTER);
							if (tr.allsolid || tr.startsolid || ((tr.fraction < 1.0) && (tr.ent != self->enemy)))
							{
								return false;
							}

							self->monsterinfo.attack_state = AS_BLIND;
							self->monsterinfo.attack_finished = level.time + 0.5 + 2*random();
							return true;
						}
					}
				}
				// pmm
				return false;
			}
		}
	}
	
	if (level.time < self->monsterinfo.attack_finished)
		return false;

	enemy_range = range(self, self->enemy);

	if (enemy_range == RANGE_MELEE)
	{
		// don't always melee in easy mode
		if (skill->value == 0 && (rand()&3) )
			return false;
		self->monsterinfo.attack_state = AS_MISSILE;
		return true;
	}
	
	if (self->spawnflags & SPAWN_ROCKET)
	{
		chance = 0.10;
		nexttime = (1.8 - (0.2 * skill->value));
	}
	else if(self->spawnflags & SPAWN_BLASTER)
	{
		chance = 0.35;
		nexttime = (1.2 - (0.2 * skill->value));
	}
	else
	{
		chance = 0.50;
		nexttime = (0.8 - (0.1 * skill->value));
	}

	if (skill->value == 0)
		chance *= 0.5;
	else if (skill->value > 1)
		chance *= 2;

	// PGM - go ahead and shoot every time if it's a info_notnull
	// PMM - added visibility check
	if ( ((random () < chance) && (visible(self, self->enemy))) || (self->enemy->solid == SOLID_NOT))
	{
		self->monsterinfo.attack_state = AS_MISSILE;
//		self->monsterinfo.attack_finished = level.time + 0.3 + 2*random();
		self->monsterinfo.attack_finished = level.time + nexttime;
		return true;
	}

	self->monsterinfo.attack_state = AS_STRAIGHT;

	return false;
}


// **********************
//  SPAWN
// **********************

/*QUAKED monster_turret (1 .5 0) (-16 -16 -16) (16 16 16) Ambush Trigger_Spawn Sight Blaster MachineGun Rocket Heatbeam WallUnit

The automated defense turret that mounts on walls. 
Check the weapon you want it to use: blaster, machinegun, rocket, heatbeam.
Default weapon is blaster.
When activated, wall units move 32 units in the direction they're facing.
*/
void SP_monster_turret (edict_t *self)
{
	int		angle;

	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	// VERSIONING
//	if (g_showlogic && g_showlogic->value)
//		gi.dprintf ("%s\n", ROGUE_VERSION_STRING);

//	self->plat2flags = ROGUE_VERSION_ID;
	// versions

	// pre-caches
	gi.soundindex ("world/dr_short.wav");
	gi.modelindex ("models/objects/debris1/tris.md2");

	self->s.modelindex = gi.modelindex("models/monsters/turret/tris.md2");

	VectorSet (self->mins, -12, -12, -12);
	VectorSet (self->maxs, 12, 12, 12);
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;

	self->health = 240;
	self->gib_health = -100;
	self->mass = 250;
	self->yaw_speed = 45;

	self->flags |= FL_MECHANICAL;

	self->pain = turret_pain;
	self->die = turret_die;

	// map designer didn't specify weapon type. set it now.
	if(!(self->spawnflags & SPAWN_WEAPONCHOICE))
	{
		self->spawnflags |= SPAWN_BLASTER;
//		self->spawnflags |= SPAWN_MACHINEGUN;
//		self->spawnflags |= SPAWN_ROCKET;
//		self->spawnflags |= SPAWN_HEATBEAM;
	}

	if(self->spawnflags & SPAWN_HEATBEAM)
	{
		self->spawnflags &= ~SPAWN_HEATBEAM;
		self->spawnflags |= SPAWN_BLASTER;
	}

	if(!(self->spawnflags & SPAWN_WALL_UNIT))
	{
		self->monsterinfo.stand = turret_stand;
		self->monsterinfo.walk = turret_walk;
		self->monsterinfo.run = turret_run;
		self->monsterinfo.dodge = NULL;
		self->monsterinfo.attack = turret_attack;
		self->monsterinfo.melee = NULL;
		self->monsterinfo.sight = turret_sight;
		self->monsterinfo.search = turret_search;
		self->monsterinfo.currentmove = &turret_move_stand;
	}

	// PMM
	self->monsterinfo.checkattack = turret_checkattack;

	self->monsterinfo.aiflags |= AI_MANUAL_STEERING;
	self->monsterinfo.scale = MODEL_SCALE;
	self->gravity = 0;

	VectorCopy(self->s.angles, self->offset);
	angle=(int)self->s.angles[1];
	switch(angle)
	{
		case -1:					// up
			self->s.angles[0] = 270;
			self->s.angles[1] = 0;
			self->s.origin[2] += 2;
			break;
		case -2:					// down
			self->s.angles[0] = 90;
			self->s.angles[1] = 0;
			self->s.origin[2] -= 2;
			break;
		case 0:
			self->s.origin[0] += 2;
			break;
		case 90:
			self->s.origin[1] += 2;
			break;
		case 180:
			self->s.origin[0] -= 2;
			break;
		case 270:
			self->s.origin[1] -= 2;
			break;
		default:
			break;
	}

	gi.linkentity (self);


	if(self->spawnflags & SPAWN_WALL_UNIT)
	{
		if(!self->targetname)
		{
//			gi.dprintf("Wall Unit Turret without targetname! %s\n", vtos(self->s.origin));
			G_FreeEdict(self);
			return;
		}

		self->takedamage = DAMAGE_NO;
		self->use = turret_activate;
		turret_wall_spawn(self);
		if ((!(self->monsterinfo.aiflags & AI_GOOD_GUY)) && (!(self->monsterinfo.aiflags & AI_DO_NOT_COUNT)))
			level.total_monsters++;

	}
	else
	{
		stationarymonster_start (self);
	}

	if(self->spawnflags & SPAWN_MACHINEGUN)
	{
		gi.soundindex ("infantry/infatck1.wav");
		self->s.skinnum = 1;
	}
	else if(self->spawnflags & SPAWN_ROCKET)
	{
		gi.soundindex ("weapons/rockfly.wav");
		gi.modelindex ("models/objects/rocket/tris.md2");
		gi.soundindex ("chick/chkatck2.wav");
		self->s.skinnum = 2;
	}
	else
	{
		if (!(self->spawnflags & SPAWN_BLASTER))
		{
			self->spawnflags |= SPAWN_BLASTER;
		}
		gi.modelindex ("models/objects/laser/tris.md2");
		gi.soundindex ("misc/lasfly.wav");
		gi.soundindex ("soldier/solatck2.wav");
	}
	
	// PMM  - turrets don't get mad at monsters, and visa versa
	self->monsterinfo.aiflags |= AI_IGNORE_SHOTS;
	// PMM - blindfire
	if(self->spawnflags & (SPAWN_ROCKET|SPAWN_BLASTER))
		self->monsterinfo.blindfire = true;
}
