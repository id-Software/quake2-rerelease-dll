// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_ai.c

#include "g_local.h"

qboolean FindTarget (edict_t *self);
extern cvar_t	*maxclients;

qboolean ai_checkattack (edict_t *self, float dist);

qboolean	enemy_vis;
qboolean	enemy_infront;
int			enemy_range;
float		enemy_yaw;

// ROGUE STUFF
#define SLIDING_TROOPS	1
#define	MAX_SIDESTEP	8.0
//

//============================================================================


/*
=================
AI_SetSightClient

Called once each frame to set level.sight_client to the
player to be checked for in findtarget.

If all clients are either dead or in notarget, sight_client
will be null.

In coop games, sight_client will cycle between the clients.
=================
*/
void AI_SetSightClient (void)
{
	edict_t	*ent;
	int		start, check;

	if (level.sight_client == NULL)
		start = 1;
	else
		start = level.sight_client - g_edicts;

	check = start;
	while (1)
	{
		check++;
		if (check > game.maxclients)
			check = 1;
		ent = &g_edicts[check];
		if (ent->inuse
			&& ent->health > 0
			&& !(ent->flags & (FL_NOTARGET|FL_DISGUISED)) )
		{
			level.sight_client = ent;
			return;		// got one
		}
		if (check == start)
		{
			level.sight_client = NULL;
			return;		// nobody to see
		}
	}
}

//============================================================================

/*
=============
ai_move

Move the specified distance at current facing.
This replaces the QC functions: ai_forward, ai_back, ai_pain, and ai_painforward
==============
*/
void ai_move (edict_t *self, float dist)
{
	M_walkmove (self, self->s.angles[YAW], dist);
}


/*
=============
ai_stand

Used for standing around and looking for players
Distance is for slight position adjustments needed by the animations
==============
*/
void ai_stand (edict_t *self, float dist)
{
	vec3_t	v;
	// PMM
	qboolean retval;

	if (dist)
		M_walkmove (self, self->s.angles[YAW], dist);

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		if (self->enemy)
		{
			VectorSubtract (self->enemy->s.origin, self->s.origin, v);
			self->ideal_yaw = vectoyaw(v);
			if (self->s.angles[YAW] != self->ideal_yaw && self->monsterinfo.aiflags & AI_TEMP_STAND_GROUND)
			{
				self->monsterinfo.aiflags &= ~(AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
				self->monsterinfo.run (self);
			}
			if (!(self->monsterinfo.aiflags & AI_MANUAL_STEERING))
				M_ChangeYaw (self);
			// PMM
			// find out if we're going to be shooting
			retval = ai_checkattack (self, 0);
			// record sightings of player
			if ((self->enemy) && (self->enemy->inuse) && (visible(self, self->enemy)))
			{
				self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
				VectorCopy (self->enemy->s.origin, self->monsterinfo.last_sighting);
				VectorCopy (self->enemy->s.origin, self->monsterinfo.blind_fire_target);
				self->monsterinfo.trail_time = level.time;
				self->monsterinfo.blind_fire_delay = 0;
			}
			// check retval to make sure we're not blindfiring
			else if (!retval)
			{
				FindTarget (self);
				return;
			}
//			ai_checkattack (self, 0);
			// pmm
		}
		else
			FindTarget (self);
		return;
	}

	if (FindTarget (self))
		return;
	
	if (level.time > self->monsterinfo.pausetime)
	{
		self->monsterinfo.walk (self);
		return;
	}

	if (!(self->spawnflags & 1) && (self->monsterinfo.idle) && (level.time > self->monsterinfo.idle_time))
	{
		if (self->monsterinfo.idle_time)
		{
			self->monsterinfo.idle (self);
			self->monsterinfo.idle_time = level.time + 15 + random() * 15;
		}
		else
		{
			self->monsterinfo.idle_time = level.time + random() * 15;
		}
	}
}


/*
=============
ai_walk

The monster is walking it's beat
=============
*/
void ai_walk (edict_t *self, float dist)
{
	M_MoveToGoal (self, dist);

	// check for noticing a player
	if (FindTarget (self))
		return;

	if ((self->monsterinfo.search) && (level.time > self->monsterinfo.idle_time))
	{
		if (self->monsterinfo.idle_time)
		{
			self->monsterinfo.search (self);
			self->monsterinfo.idle_time = level.time + 15 + random() * 15;
		}
		else
		{
			self->monsterinfo.idle_time = level.time + random() * 15;
		}
	}
}


/*
=============
ai_charge

Turns towards target and advances
Use this call with a distance of 0 to replace ai_face
==============
*/
void ai_charge (edict_t *self, float dist)
{
	vec3_t	v;
	// PMM
	float	ofs;
	// PMM

	// PMM - made AI_MANUAL_STEERING affect things differently here .. they turn, but
	// don't set the ideal_yaw

	// This is put in there so monsters won't move towards the origin after killing
	// a tesla. This could be problematic, so keep an eye on it.
	if(!self->enemy || !self->enemy->inuse)		//PGM
		return;									//PGM

	// PMM - save blindfire target
	if (visible(self, self->enemy))
		VectorCopy (self->enemy->s.origin, self->monsterinfo.blind_fire_target);
	// pmm 

	if (!(self->monsterinfo.aiflags & AI_MANUAL_STEERING))
	{
		VectorSubtract (self->enemy->s.origin, self->s.origin, v);
		self->ideal_yaw = vectoyaw(v);
//		gi.dprintf ("enemy = %s\n", vtos (self->enemy->s.origin));
//		gi.dprintf ("enemy: ideal yaw is %f\n", self->ideal_yaw);
	}
//	if (!(self->monsterinfo.aiflags & AI_MANUAL_STEERING))
	M_ChangeYaw (self);
// PMM
//	if (dist)
//		M_walkmove (self, self->s.angles[YAW], dist);

	if (dist)
	{
		if (self->monsterinfo.aiflags & AI_CHARGING)
		{
			M_MoveToGoal (self, dist);
			return;
		}
		// circle strafe support
		if (self->monsterinfo.attack_state == AS_SLIDING)
		{
			// if we're fighting a tesla, NEVER circle strafe
			if ((self->enemy) && (self->enemy->classname) && (!strcmp(self->enemy->classname, "tesla")))
				ofs = 0;
			else if (self->monsterinfo.lefty)
				ofs = 90;
			else
				ofs = -90;
			
			if (M_walkmove (self, self->ideal_yaw + ofs, dist))
				return;
				
			self->monsterinfo.lefty = 1 - self->monsterinfo.lefty;
			M_walkmove (self, self->ideal_yaw - ofs, dist);
		}
		else
			M_walkmove (self, self->s.angles[YAW], dist);
	}
// PMM
}


/*
=============
ai_turn

don't move, but turn towards ideal_yaw
Distance is for slight position adjustments needed by the animations
=============
*/
void ai_turn (edict_t *self, float dist)
{
	if (dist)
		M_walkmove (self, self->s.angles[YAW], dist);

	if (FindTarget (self))
		return;
	
	if (!(self->monsterinfo.aiflags & AI_MANUAL_STEERING))
		M_ChangeYaw (self);
}


/*

.enemy
Will be world if not currently angry at anyone.

.movetarget
The next path spot to walk toward.  If .enemy, ignore .movetarget.
When an enemy is killed, the monster will try to return to it's path.

.hunt_time
Set to time + something when the player is in sight, but movement straight for
him is blocked.  This causes the monster to use wall following code for
movement direction instead of sighting on the player.

.ideal_yaw
A yaw angle of the intended direction, which will be turned towards at up
to 45 deg / state.  If the enemy is in view and hunt_time is not active,
this will be the exact line towards the enemy.

.pausetime
A monster will leave it's stand state and head towards it's .movetarget when
time > .pausetime.

walkmove(angle, speed) primitive is all or nothing
*/

/*
=============
range

returns the range catagorization of an entity reletive to self
0	melee range, will become hostile even if back is turned
1	visibility and infront, or visibility and show hostile
2	infront and show hostile
3	only triggered by damage
=============
*/
int range (edict_t *self, edict_t *other)
{
	vec3_t	v;
	float	len;

	VectorSubtract (self->s.origin, other->s.origin, v);
	len = VectorLength (v);
	if (len < MELEE_DISTANCE)
		return RANGE_MELEE;
	if (len < 500)
		return RANGE_NEAR;
	if (len < 1000)
		return RANGE_MID;
	return RANGE_FAR;
}

/*
=============
visible

returns 1 if the entity is visible to self, even if not infront ()
=============
*/
qboolean visible (edict_t *self, edict_t *other)
{
	vec3_t	spot1;
	vec3_t	spot2;
	trace_t	trace;

	VectorCopy (self->s.origin, spot1);
	spot1[2] += self->viewheight;
	VectorCopy (other->s.origin, spot2);
	spot2[2] += other->viewheight;
	trace = gi.trace (spot1, vec3_origin, vec3_origin, spot2, self, MASK_OPAQUE);
	
	if (trace.fraction == 1.0 || trace.ent == other)		// PGM
		return true;
	return false;
}


/*
=============
infront

returns 1 if the entity is in front (in sight) of self
=============
*/
qboolean infront (edict_t *self, edict_t *other)
{
	vec3_t	vec;
	float	dot;
	vec3_t	forward;
	
	AngleVectors (self->s.angles, forward, NULL, NULL);
	VectorSubtract (other->s.origin, self->s.origin, vec);
	VectorNormalize (vec);
	dot = DotProduct (vec, forward);
	
	if (dot > 0.3)
		return true;
	return false;
}


//============================================================================

void HuntTarget (edict_t *self)
{
	vec3_t	vec;

	self->goalentity = self->enemy;
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.stand (self);
	else
		self->monsterinfo.run (self);
	VectorSubtract (self->enemy->s.origin, self->s.origin, vec);
	self->ideal_yaw = vectoyaw(vec);
	// wait a while before first attack
	if (!(self->monsterinfo.aiflags & AI_STAND_GROUND))
		AttackFinished (self, 1);
}

void FoundTarget (edict_t *self)
{
	// let other monsters see this monster for a while
	if (self->enemy->client)
	{
		if(self->enemy->flags & FL_DISGUISED)
		{
//			level.disguise_violator = self->enemy;
//			level.disguise_violation_framenum = level.framenum + 5;
			self->enemy->flags &= ~FL_DISGUISED;
		}
		
		level.sight_entity = self;
		level.sight_entity_framenum = level.framenum;
		level.sight_entity->light_level = 128;
	}

	self->show_hostile = level.time + 1;		// wake up other monsters

	VectorCopy(self->enemy->s.origin, self->monsterinfo.last_sighting);
	self->monsterinfo.trail_time = level.time;
	// PMM
	VectorCopy (self->enemy->s.origin, self->monsterinfo.blind_fire_target);
	self->monsterinfo.blind_fire_delay = 0;
	// PMM

	if (!self->combattarget)
	{
		HuntTarget (self);
		return;
	}

	self->goalentity = self->movetarget = G_PickTarget(self->combattarget);
	if (!self->movetarget)
	{
		self->goalentity = self->movetarget = self->enemy;
		HuntTarget (self);
		gi.dprintf("%s at %s, combattarget %s not found\n", self->classname, vtos(self->s.origin), self->combattarget);
		return;
	}

	// clear out our combattarget, these are a one shot deal
	self->combattarget = NULL;
	self->monsterinfo.aiflags |= AI_COMBAT_POINT;

	// clear the targetname, that point is ours!
	self->movetarget->targetname = NULL;
	self->monsterinfo.pausetime = 0;

	// run for it
	self->monsterinfo.run (self);
}


/*
===========
FindTarget

Self is currently not attacking anything, so try to find a target

Returns TRUE if an enemy was sighted

When a player fires a missile, the point of impact becomes a fakeplayer so
that monsters that see the impact will respond as if they had seen the
player.

To avoid spending too much time, only a single client (or fakeclient) is
checked each frame.  This means multi player games will have slightly
slower noticing monsters.
============
*/
qboolean FindTarget (edict_t *self)
{
	edict_t		*client;
	qboolean	heardit;
	int			r;

	if (self->monsterinfo.aiflags & AI_GOOD_GUY)
	{
		if (self->goalentity && self->goalentity->inuse && self->goalentity->classname)
		{
			if (strcmp(self->goalentity->classname, "target_actor") == 0)
				return false;
		}

		//FIXME look for monsters?
		return false;
	}

	// if we're going to a combat point, just proceed
	if (self->monsterinfo.aiflags & AI_COMBAT_POINT)
		return false;

// if the first spawnflag bit is set, the monster will only wake up on
// really seeing the player, not another monster getting angry or hearing
// something

// revised behavior so they will wake up if they "see" a player make a noise
// but not weapon impact/explosion noises

	heardit = false;
	if ((level.sight_entity_framenum >= (level.framenum - 1)) && !(self->spawnflags & 1) )
	{
		client = level.sight_entity;
		if (client->enemy == self->enemy)
		{
			return false;
		}
	}
	else if (level.disguise_violation_framenum > level.framenum)
	{
		client = level.disguise_violator;
	}
	else if (level.sound_entity_framenum >= (level.framenum - 1))
	{
		client = level.sound_entity;
		heardit = true;
	}
	else if (!(self->enemy) && (level.sound2_entity_framenum >= (level.framenum - 1)) && !(self->spawnflags & 1) )
	{
		client = level.sound2_entity;
		heardit = true;
	}
	else
	{
		client = level.sight_client;
		if (!client)
			return false;	// no clients to get mad at
	}

	// if the entity went away, forget it
	if (!client->inuse)
		return false;

	if (client == self->enemy)
		return true;	// JDC false;

	//PMM - hintpath coop fix
	if ((self->monsterinfo.aiflags & AI_HINT_PATH) && (coop) && (coop->value))
	{
//		if ((heardit) && (g_showlogic) && (g_showlogic->value))
//			gi.dprintf ("ignoring coop sound target\n");
		heardit = false;
	}
	// pmm

	if (client->client)
	{
		if (client->flags & FL_NOTARGET)
			return false;
	}
	else if (client->svflags & SVF_MONSTER)
	{
		if (!client->enemy)
			return false;
		if (client->enemy->flags & FL_NOTARGET)
			return false;
	}
	else if (heardit)
	{
		// pgm - a little more paranoia won't hurt....
		if ((client->owner) && (client->owner->flags & FL_NOTARGET))
			return false;
	}
	else
		return false;

	if (!heardit)
	{
		r = range (self, client);

		if (r == RANGE_FAR)
			return false;

// this is where we would check invisibility

		// is client in an spot too dark to be seen?
		if (client->light_level <= 5)
			return false;

		if (!visible (self, client))
		{
			return false;
		}

		if (r == RANGE_NEAR)
		{
			if (client->show_hostile < level.time && !infront (self, client))
			{
				return false;
			}
		}
		else if (r == RANGE_MID)
		{
			if (!infront (self, client))
			{
				return false;
			}
		}

		self->enemy = client;

		if (strcmp(self->enemy->classname, "player_noise") != 0)
		{
			self->monsterinfo.aiflags &= ~AI_SOUND_TARGET;

			if (!self->enemy->client)
			{
				self->enemy = self->enemy->enemy;
				if (!self->enemy->client)
				{
					self->enemy = NULL;
					return false;
				}
			}
		}
	}
	else	// heardit
	{
		vec3_t	temp;

		if (self->spawnflags & 1)
		{
			if (!visible (self, client))
				return false;
		}
		else
		{
			if (!gi.inPHS(self->s.origin, client->s.origin))
				return false;
		}

		VectorSubtract (client->s.origin, self->s.origin, temp);

		if (VectorLength(temp) > 1000)	// too far to hear
		{
			return false;
		}

		// check area portals - if they are different and not connected then we can't hear it
		if (client->areanum != self->areanum)
			if (!gi.AreasConnected(self->areanum, client->areanum))
				return false;
	
		self->ideal_yaw = vectoyaw(temp);
		if (!(self->monsterinfo.aiflags & AI_MANUAL_STEERING))
			M_ChangeYaw (self);

		// hunt the sound for a bit; hopefully find the real player
		self->monsterinfo.aiflags |= AI_SOUND_TARGET;
		self->enemy = client;
	}

//
// got one
//
	// PMM - if we got an enemy, we need to bail out of hint paths, so take over here
	if (self->monsterinfo.aiflags & AI_HINT_PATH)
	{
//		if(g_showlogic && g_showlogic->value)
//			gi.dprintf("stopped following hint paths in FindTarget\n");

		// this calls foundtarget for us
		hintpath_stop (self);
	}
	else
	{
		FoundTarget (self);
	}
	// pmm
	if (!(self->monsterinfo.aiflags & AI_SOUND_TARGET) && (self->monsterinfo.sight))
		self->monsterinfo.sight (self, self->enemy);

	return true;
}


//=============================================================================

/*
============
FacingIdeal

============
*/
qboolean FacingIdeal(edict_t *self)
{
	float	delta;

	delta = anglemod(self->s.angles[YAW] - self->ideal_yaw);
	if (delta > 45 && delta < 315)
		return false;
	return true;
}


//=============================================================================

qboolean M_CheckAttack (edict_t *self)
{
	vec3_t	spot1, spot2;
	float	chance;
	trace_t	tr;

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
					if ((self->monsterinfo.blindfire) && (self->monsterinfo.blind_fire_delay <= 20.0))
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
//	gi.WriteByte (svc_temp_entity);
//	gi.WriteByte (TE_DEBUGTRAIL);
//	gi.WritePosition (spot1);
//	gi.WritePosition (self->monsterinfo.blind_fire_target);
//	gi.multicast (self->s.origin, MULTICAST_ALL);
							// make sure we're not going to shoot a monster
							tr = gi.trace (spot1, NULL, NULL, self->monsterinfo.blind_fire_target, self, CONTENTS_MONSTER);
							if (tr.allsolid || tr.startsolid || ((tr.fraction < 1.0) && (tr.ent != self->enemy)))
							{
//								if ((g_showlogic) && (g_showlogic->value))
//									gi.dprintf ("blindfire blocked\n");
								return false;
							}

							self->monsterinfo.attack_state = AS_BLIND;
							return true;
						}
					}
				}
				// pmm
				return false;
			}
		}
	}
	
	// melee attack
	if (enemy_range == RANGE_MELEE)
	{
		// don't always melee in easy mode
		if (skill->value == 0 && (rand()&3) )
		{
			// PMM - fix for melee only monsters & strafing
			self->monsterinfo.attack_state = AS_STRAIGHT;
			return false;
		}
		if (self->monsterinfo.melee)
			self->monsterinfo.attack_state = AS_MELEE;
		else
			self->monsterinfo.attack_state = AS_MISSILE;
		return true;
	}
	
// missile attack
	if (!self->monsterinfo.attack)
	{
		// PMM - fix for melee only monsters & strafing
		self->monsterinfo.attack_state = AS_STRAIGHT;
		return false;
	}
	
	if (level.time < self->monsterinfo.attack_finished)
		return false;
		
	if (enemy_range == RANGE_FAR)
		return false;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		chance = 0.4;
	}
	else if (enemy_range == RANGE_MELEE)
	{
		chance = 0.2;
	}
	else if (enemy_range == RANGE_NEAR)
	{
		chance = 0.1;
	}
	else if (enemy_range == RANGE_MID)
	{
		chance = 0.02;
	}
	else
	{
		return false;
	}

	if (skill->value == 0)
		chance *= 0.5;
	else if (skill->value >= 2)
		chance *= 2;

	// PGM - go ahead and shoot every time if it's a info_notnull
	if ((random () < chance) || (self->enemy->solid == SOLID_NOT))
	{
		self->monsterinfo.attack_state = AS_MISSILE;
		self->monsterinfo.attack_finished = level.time + 2*random();
		return true;
	}

	// PMM -daedalus should strafe more .. this can be done here or in a customized
	// check_attack code for the hover.
	if (self->flags & FL_FLY)
	{
		// originally, just 0.3
		float strafe_chance;
		if (!(strcmp(self->classname, "monster_daedalus")))
			strafe_chance = 0.8;
		else
			strafe_chance = 0.6;

		// if enemy is tesla, never strafe
		if ((self->enemy) && (self->enemy->classname) && (!strcmp(self->enemy->classname, "tesla")))
			strafe_chance = 0;

		if (random() < strafe_chance)
			self->monsterinfo.attack_state = AS_SLIDING;
		else
			self->monsterinfo.attack_state = AS_STRAIGHT;
	}
// do we want the monsters strafing?
#ifdef SLIDING_TROOPS
	else
	{
		if (random() < 0.4)
			self->monsterinfo.attack_state = AS_SLIDING;
		else
			self->monsterinfo.attack_state = AS_STRAIGHT;
	}
#endif
//-PMM

	return false;
}


/*
=============
ai_run_melee

Turn and close until within an angle to launch a melee attack
=============
*/
void ai_run_melee(edict_t *self)
{
	self->ideal_yaw = enemy_yaw;
	if (!(self->monsterinfo.aiflags & AI_MANUAL_STEERING))
		M_ChangeYaw (self);

	if (FacingIdeal(self))
	{
		self->monsterinfo.melee (self);
		self->monsterinfo.attack_state = AS_STRAIGHT;
	}
}


/*
=============
ai_run_missile

Turn in place until within an angle to launch a missile attack
=============
*/
void ai_run_missile(edict_t *self)
{
	self->ideal_yaw = enemy_yaw;
	if (!(self->monsterinfo.aiflags & AI_MANUAL_STEERING))
		M_ChangeYaw (self);

	if (FacingIdeal(self))
	{
		self->monsterinfo.attack (self);
//		if (self->monsterinfo.attack_state == AS_MISSILE)
		if ((self->monsterinfo.attack_state == AS_MISSILE) || (self->monsterinfo.attack_state == AS_BLIND))
			self->monsterinfo.attack_state = AS_STRAIGHT;
//		else if (self->monsterinfo.attack_state != AS_SLIDING)
//			gi.dprintf ("ai_run_missile: Unexpected attack state %d !\n", self->monsterinfo.attack_state);
	}
};


/*
=============
ai_run_slide

Strafe sideways, but stay at aproximately the same range
=============
*/
void ai_run_slide(edict_t *self, float distance)
{
	float	ofs;
	float	angle;

	self->ideal_yaw = enemy_yaw;

//	if (self->flags & FL_FLY)
//		angle = 90;
//	else
//		angle = 45;
	
	angle = 90;
	
	if (self->monsterinfo.lefty)
		ofs = angle;
	else
		ofs = -angle;
//	
//	if (!(self->flags & FL_FLY))
//	{
//		// non fliers should actually turn towards the direction their trying to run
//		self->ideal_yaw += ofs;
//	}
//
	if (!(self->monsterinfo.aiflags & AI_MANUAL_STEERING))
		M_ChangeYaw (self);

	/*
	if (!(self->flags & FL_FLY))
	{
		if (M_walkmove (self, self->ideal_yaw + ofs, distance))
			return;
	}
	else
	{
		if (M_walkmove (self, self->ideal_yaw, distance))
			return;
	}
	*/
	// PMM - clamp maximum sideways move for non flyers to make them look less jerky
	if (!self->flags & FL_FLY)
		distance = min (distance, MAX_SIDESTEP);
	if (M_walkmove (self, self->ideal_yaw + ofs, distance))
		return;
	// PMM - if we're dodging, give up on it and go straight
	if (self->monsterinfo.aiflags & AI_DODGING)
	{
		monster_done_dodge (self);
		// by setting as_straight, caller will know to try straight move
		self->monsterinfo.attack_state = AS_STRAIGHT;
		return;
	}

	self->monsterinfo.lefty = 1 - self->monsterinfo.lefty;
	if (M_walkmove (self, self->ideal_yaw - ofs, distance))
		return;
	// PMM - if we're dodging, give up on it and go straight
	if (self->monsterinfo.aiflags & AI_DODGING)
		monster_done_dodge (self);

	// PMM - the move failed, so signal the caller (ai_run) to try going straight
	self->monsterinfo.attack_state = AS_STRAIGHT;
	/*
	if (!(self->flags & FL_FLY))
	{
		M_walkmove (self, self->ideal_yaw + ofs, distance);
	}
	else
	{
		M_walkmove (self, self->ideal_yaw, distance);
	}*/
}


/*
=============
ai_checkattack

Decides if we're going to attack or do something else
used by ai_run and ai_stand
=============
*/
qboolean ai_checkattack (edict_t *self, float dist)
{
	vec3_t		temp;
	qboolean	hesDeadJim;
	// PMM
	qboolean	retval;

// this causes monsters to run blindly to the combat point w/o firing
	if (self->goalentity)
	{
		if (self->monsterinfo.aiflags & AI_COMBAT_POINT)
			return false;

		if (self->monsterinfo.aiflags & AI_SOUND_TARGET)
		{
			if ((level.time - self->enemy->teleport_time) > 5.0)
			{
				if (self->goalentity == self->enemy)
					if (self->movetarget)
						self->goalentity = self->movetarget;
					else
						self->goalentity = NULL;
				self->monsterinfo.aiflags &= ~AI_SOUND_TARGET;
				if (self->monsterinfo.aiflags & AI_TEMP_STAND_GROUND)
					self->monsterinfo.aiflags &= ~(AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
			}
			else
			{
				self->show_hostile = level.time + 1;
				return false;
			}
		}
	}

	enemy_vis = false;

// see if the enemy is dead
	hesDeadJim = false;
	if ((!self->enemy) || (!self->enemy->inuse))
	{
		hesDeadJim = true;
	}
	else if (self->monsterinfo.aiflags & AI_MEDIC)
	{
		if (!(self->enemy->inuse) || (self->enemy->health > 0))
		{
			hesDeadJim = true;
//			self->monsterinfo.aiflags &= ~AI_MEDIC;
		}
	}
	else
	{
		if (self->monsterinfo.aiflags & AI_BRUTAL)
		{
			if (self->enemy->health <= -80)
				hesDeadJim = true;
		}
		else
		{
			if (self->enemy->health <= 0)
				hesDeadJim = true;
		}
	}

	if (hesDeadJim)
	{
		self->monsterinfo.aiflags &= ~AI_MEDIC;
		self->enemy = NULL;
	// FIXME: look all around for other targets
		if (self->oldenemy && self->oldenemy->health > 0)
		{
			self->enemy = self->oldenemy;
			self->oldenemy = NULL;
			HuntTarget (self);
		}
//ROGUE - multiple teslas make monsters lose track of the player.
		else if(self->monsterinfo.last_player_enemy && self->monsterinfo.last_player_enemy->health > 0)
		{
//			if ((g_showlogic) && (g_showlogic->value))
//				gi.dprintf("resorting to last_player_enemy...\n");
			self->enemy = self->monsterinfo.last_player_enemy;
			self->oldenemy = NULL;
			self->monsterinfo.last_player_enemy = NULL;
			HuntTarget (self);
		}
//ROGUE
		else
		{
			if (self->movetarget)
			{
				self->goalentity = self->movetarget;
				self->monsterinfo.walk (self);
			}
			else
			{
				// we need the pausetime otherwise the stand code
				// will just revert to walking with no target and
				// the monsters will wonder around aimlessly trying
				// to hunt the world entity
				self->monsterinfo.pausetime = level.time + 100000000;
				self->monsterinfo.stand (self);
			}
			return true;
		}
	}

	self->show_hostile = level.time + 1;		// wake up other monsters

// check knowledge of enemy
	enemy_vis = visible(self, self->enemy);
	if (enemy_vis)
	{
		self->monsterinfo.search_time = level.time + 5;
		VectorCopy (self->enemy->s.origin, self->monsterinfo.last_sighting);
		// PMM
		self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
		self->monsterinfo.trail_time = level.time;
		VectorCopy (self->enemy->s.origin, self->monsterinfo.blind_fire_target);
		self->monsterinfo.blind_fire_delay = 0;
		// pmm
	}

// look for other coop players here
//	if (coop && self->monsterinfo.search_time < level.time)
//	{
//		if (FindTarget (self))
//			return true;
//	}

	enemy_infront = infront(self, self->enemy);
	enemy_range = range(self, self->enemy);
	VectorSubtract (self->enemy->s.origin, self->s.origin, temp);
	enemy_yaw = vectoyaw(temp);


	// JDC self->ideal_yaw = enemy_yaw;

	// PMM -- reordered so the monster specific checkattack is called before the run_missle/melee/checkvis
	// stuff .. this allows for, among other things, circle strafing and attacking while in ai_run
	retval = self->monsterinfo.checkattack (self);
	if (retval)
	{
		// PMM
		if (self->monsterinfo.attack_state == AS_MISSILE)
		{
			ai_run_missile (self);
			return true;
		}
		if (self->monsterinfo.attack_state == AS_MELEE)
		{
			ai_run_melee (self);
			return true;
		}
		// PMM -- added so monsters can shoot blind
		if (self->monsterinfo.attack_state == AS_BLIND)
		{
			ai_run_missile (self);
			return true;
		}
		// pmm

		// if enemy is not currently visible, we will never attack
		if (!enemy_vis)
			return false;
		// PMM
	}
	return retval;
	// PMM
//	return self->monsterinfo.checkattack (self);
}


/*
=============
ai_run

The monster has an enemy it is trying to kill
=============
*/
void ai_run (edict_t *self, float dist)
{
	vec3_t		v;
	edict_t		*tempgoal;
	edict_t		*save;
	qboolean	new;
	edict_t		*marker;
	float		d1, d2;
	trace_t		tr;
	vec3_t		v_forward, v_right;
	float		left, center, right;
	vec3_t		left_target, right_target;
	//PMM
	qboolean	retval;
	qboolean	alreadyMoved = false;
	qboolean	gotcha = false;
	edict_t		*realEnemy;
 
	// if we're going to a combat point, just proceed
	if (self->monsterinfo.aiflags & AI_COMBAT_POINT)
	{
		M_MoveToGoal (self, dist);
		return;
	}

	// PMM
	if (self->monsterinfo.aiflags & AI_DUCKED)
	{
//		if ((g_showlogic) && (g_showlogic->value))
//			gi.dprintf ("%s - duck flag cleaned up!\n", self->classname);
		self->monsterinfo.aiflags &= ~AI_DUCKED;
	}
	if (self->maxs[2] != self->monsterinfo.base_height)
	{
//		if ((g_showlogic) && (g_showlogic->value))
//			gi.dprintf ("%s - ducked height corrected!\n", self->classname);
		monster_duck_up (self);
	}
//	if ((self->monsterinfo.aiflags & AI_MANUAL_STEERING) && (strcmp(self->classname, "monster_turret")))
//	{
//		if ((g_showlogic) && (g_showlogic->value))
//			gi.dprintf ("%s - manual steering in ai_run!\n", self->classname);
//	}
	// pmm

//==========
//PGM
	// if we're currently looking for a hint path
	if (self->monsterinfo.aiflags & AI_HINT_PATH)
	{
		// determine direction to our destination hintpath.
		// FIXME - is this needed EVERY time? I was having trouble with them
		// sometimes not going after it, and this fixed it.
//		VectorSubtract(self->movetarget->s.origin, self->s.origin, v);
//		vectoangles(v, v_forward);
//		self->ideal_yaw = v_forward[YAW];
//		gi.dprintf("seeking hintpath. origin: %s %0.1f\n", vtos(v), self->ideal_yaw);
		M_MoveToGoal (self, dist);
		if(!self->inuse)
			return;			// PGM - g_touchtrigger free problem
//		return;

		// if we've already seen the player, and can't see him now, return
//		if(self->enemy && !visible(self, self->enemy))
//			return;

		// if not and can't find the player, return
//		if(!FindTarget(self))
//			return;

		// first off, make sure we're looking for the player, not a noise he made
		if (self->enemy)
		{
			if (self->enemy->inuse)
			{
				if (strcmp(self->enemy->classname, "player_noise") != 0)
					realEnemy = self->enemy;
				else if (self->enemy->owner)
					realEnemy = self->enemy->owner;
				else // uh oh, can't figure out enemy, bail
				{
					self->enemy = NULL;
					hintpath_stop (self);
					return;
				}
			}
			else
			{
				self->enemy = NULL;
				hintpath_stop (self);
				return;
			}
		}
		else
		{
			hintpath_stop (self);
			return;
		}

		if (coop && coop->value)
		{
			// if we're in coop, check my real enemy first .. if I SEE him, set gotcha to true
			if (self->enemy && visible(self, realEnemy))
				gotcha = true;
			else // otherwise, let FindTarget bump us out of hint paths, if appropriate
				FindTarget(self);
		}
		else
		{
			if(self->enemy && visible(self, realEnemy))
				gotcha = true;
		}
		
		// if we see the player, stop following hintpaths.
		if (gotcha)
		{
//			if(g_showlogic && g_showlogic->value)
//				gi.dprintf("stopped following hint paths in ai_run\n");

			// disconnect from hintpaths and start looking normally for players.
			hintpath_stop (self);
			// pmm - no longer needed, since hintpath_stop does it
//			HuntTarget(self);
		}
		return;
	}
//PGM
//==========

	if (self->monsterinfo.aiflags & AI_SOUND_TARGET)
	{
		// PMM - paranoia checking
		if (self->enemy)
			VectorSubtract (self->s.origin, self->enemy->s.origin, v);

		if ((!self->enemy) || (VectorLength(v) < 64))
		// pmm
		{
			self->monsterinfo.aiflags |= (AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
			self->monsterinfo.stand (self);
			return;
		}

		M_MoveToGoal (self, dist);
		// PMM - prevent double moves for sound_targets
		alreadyMoved = true;
		// pmm
		if(!self->inuse)
			return;			// PGM - g_touchtrigger free problem

		if (!FindTarget (self))
			return;
	}

	// PMM -- moved ai_checkattack up here so the monsters can attack while strafing or charging

	// PMM -- if we're dodging, make sure to keep the attack_state AS_SLIDING

	retval = ai_checkattack (self, dist);

	// PMM - don't strafe if we can't see our enemy
	if ((!enemy_vis) && (self->monsterinfo.attack_state == AS_SLIDING))
		self->monsterinfo.attack_state = AS_STRAIGHT;
	// unless we're dodging (dodging out of view looks smart)
	if (self->monsterinfo.aiflags & AI_DODGING)
		self->monsterinfo.attack_state = AS_SLIDING;
	// pmm

	if (self->monsterinfo.attack_state == AS_SLIDING)
	{
		// PMM - protect against double moves
		if (!alreadyMoved)
			ai_run_slide (self, dist);
		// PMM
		// we're using attack_state as the return value out of ai_run_slide to indicate whether or not the
		// move succeeded.  If the move succeeded, and we're still sliding, we're done in here (since we've
		// had our chance to shoot in ai_checkattack, and have moved).
		// if the move failed, our state is as_straight, and it will be taken care of below
		if ((!retval) && (self->monsterinfo.attack_state == AS_SLIDING))
			return;
	}
	else if (self->monsterinfo.aiflags & AI_CHARGING)
	{
		self->ideal_yaw = enemy_yaw;
		if (!(self->monsterinfo.aiflags & AI_MANUAL_STEERING))
			M_ChangeYaw (self);
	}
	if (retval)
	{
		// PMM - is this useful?  Monsters attacking usually call the ai_charge routine..
		// the only monster this affects should be the soldier
		if ((dist != 0) && (!alreadyMoved) && (self->monsterinfo.attack_state == AS_STRAIGHT) && (!(self->monsterinfo.aiflags & AI_STAND_GROUND)))
		{
			M_MoveToGoal (self, dist);
		}
		if ((self->enemy) && (self->enemy->inuse) && (enemy_vis))
		{
			self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
			VectorCopy (self->enemy->s.origin, self->monsterinfo.last_sighting);
			self->monsterinfo.trail_time = level.time;
			//PMM
			VectorCopy (self->enemy->s.origin, self->monsterinfo.blind_fire_target);
			self->monsterinfo.blind_fire_delay = 0;
			//pmm
		}
		return;
	}
	//PMM
//	if (ai_checkattack (self, dist))
//		return;

//	if (self->monsterinfo.attack_state == AS_SLIDING)
//	{
//		ai_run_slide (self, dist);
//		return;
//	}

	// PGM - added a little paranoia checking here... 9/22/98
	if ((self->enemy) && (self->enemy->inuse) && (enemy_vis))
	{
//		if (self->monsterinfo.aiflags & AI_LOST_SIGHT)
//			gi.dprintf("regained sight\n");
		// PMM - check for alreadyMoved
		if (!alreadyMoved)
			M_MoveToGoal (self, dist);
		if(!self->inuse)
			return;			// PGM - g_touchtrigger free problem

		self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
		VectorCopy (self->enemy->s.origin, self->monsterinfo.last_sighting);
		self->monsterinfo.trail_time = level.time;
		// PMM
		VectorCopy (self->enemy->s.origin, self->monsterinfo.blind_fire_target);
		self->monsterinfo.blind_fire_delay = 0;
		// pmm
		return;
	}

//=======
//PGM
	// if we've been looking (unsuccessfully) for the player for 10 seconds
	// PMM - reduced to 5, makes them much nastier
	if((self->monsterinfo.trail_time + 5) <= level.time)
	{
		// and we haven't checked for valid hint paths in the last 10 seconds
		if((self->monsterinfo.last_hint_time + 10) <= level.time)
		{
			// check for hint_paths.
			self->monsterinfo.last_hint_time = level.time;
			if(monsterlost_checkhint(self))
				return;
		}
	}
//PGM
//=======

// PMM - moved down here to allow monsters to get on hint paths
	// coop will change to another enemy if visible
	if (coop->value)
	{	// FIXME: insane guys get mad with this, which causes crashes!
		if (FindTarget (self))
			return;
	}
// pmm

	if ((self->monsterinfo.search_time) && (level.time > (self->monsterinfo.search_time + 20)))
	{
		// PMM - double move protection
		if (!alreadyMoved)
			M_MoveToGoal (self, dist);
		self->monsterinfo.search_time = 0;
//		gi.dprintf("search timeout\n");
		return;
	}

	save = self->goalentity;
	tempgoal = G_Spawn();
	self->goalentity = tempgoal;

	new = false;

	if (!(self->monsterinfo.aiflags & AI_LOST_SIGHT))
	{
		// just lost sight of the player, decide where to go first
//		gi.dprintf("lost sight of player, last seen at %s\n", vtos(self->monsterinfo.last_sighting));
		self->monsterinfo.aiflags |= (AI_LOST_SIGHT | AI_PURSUIT_LAST_SEEN);
		self->monsterinfo.aiflags &= ~(AI_PURSUE_NEXT | AI_PURSUE_TEMP);
		new = true;
	}

	if (self->monsterinfo.aiflags & AI_PURSUE_NEXT)
	{
//		vec3_t	debug_vec;

		self->monsterinfo.aiflags &= ~AI_PURSUE_NEXT;
//		VectorSubtract(self->monsterinfo.last_sighting, self->s.origin, debug_vec); 
//		gi.dprintf("reached current goal: %s %s %f", vtos(self->s.origin), vtos(self->monsterinfo.last_sighting), VectorLength(debug_vec));

		// give ourself more time since we got this far
		self->monsterinfo.search_time = level.time + 5;

		if (self->monsterinfo.aiflags & AI_PURSUE_TEMP)
		{
//			gi.dprintf("was temp goal; retrying original\n");
			self->monsterinfo.aiflags &= ~AI_PURSUE_TEMP;
			marker = NULL;
			VectorCopy (self->monsterinfo.saved_goal, self->monsterinfo.last_sighting);
			new = true;
		}
		else if (self->monsterinfo.aiflags & AI_PURSUIT_LAST_SEEN)
		{
			self->monsterinfo.aiflags &= ~AI_PURSUIT_LAST_SEEN;
			marker = PlayerTrail_PickFirst (self);
		}
		else
		{
			marker = PlayerTrail_PickNext (self);
		}

		if (marker)
		{
			VectorCopy (marker->s.origin, self->monsterinfo.last_sighting);
			self->monsterinfo.trail_time = marker->timestamp;
			self->s.angles[YAW] = self->ideal_yaw = marker->s.angles[YAW];
//			gi.dprintf("heading is %0.1f\n", self->ideal_yaw);

//			debug_drawline(self.origin, self.last_sighting, 52);
			new = true;
		}
	}

	VectorSubtract (self->s.origin, self->monsterinfo.last_sighting, v);
	d1 = VectorLength(v);
	if (d1 <= dist)
	{
		self->monsterinfo.aiflags |= AI_PURSUE_NEXT;
		dist = d1;
	}

	VectorCopy (self->monsterinfo.last_sighting, self->goalentity->s.origin);

	if (new)
	{
//		gi.dprintf("checking for course correction\n");

		tr = gi.trace(self->s.origin, self->mins, self->maxs, self->monsterinfo.last_sighting, self, MASK_PLAYERSOLID);
		if (tr.fraction < 1)
		{
			VectorSubtract (self->goalentity->s.origin, self->s.origin, v);
			d1 = VectorLength(v);
			center = tr.fraction;
			d2 = d1 * ((center+1)/2);
			self->s.angles[YAW] = self->ideal_yaw = vectoyaw(v);
			AngleVectors(self->s.angles, v_forward, v_right, NULL);

			VectorSet(v, d2, -16, 0);
			G_ProjectSource (self->s.origin, v, v_forward, v_right, left_target);
			tr = gi.trace(self->s.origin, self->mins, self->maxs, left_target, self, MASK_PLAYERSOLID);
			left = tr.fraction;

			VectorSet(v, d2, 16, 0);
			G_ProjectSource (self->s.origin, v, v_forward, v_right, right_target);
			tr = gi.trace(self->s.origin, self->mins, self->maxs, right_target, self, MASK_PLAYERSOLID);
			right = tr.fraction;

			center = (d1*center)/d2;
			if (left >= center && left > right)
			{
				if (left < 1)
				{
					VectorSet(v, d2 * left * 0.5, -16, 0);
					G_ProjectSource (self->s.origin, v, v_forward, v_right, left_target);
//					gi.dprintf("incomplete path, go part way and adjust again\n");
				}
				VectorCopy (self->monsterinfo.last_sighting, self->monsterinfo.saved_goal);
				self->monsterinfo.aiflags |= AI_PURSUE_TEMP;
				VectorCopy (left_target, self->goalentity->s.origin);
				VectorCopy (left_target, self->monsterinfo.last_sighting);
				VectorSubtract (self->goalentity->s.origin, self->s.origin, v);
				self->s.angles[YAW] = self->ideal_yaw = vectoyaw(v);
//				gi.dprintf("adjusted left\n");
//				debug_drawline(self.origin, self.last_sighting, 152);
			}
			else if (right >= center && right > left)
			{
				if (right < 1)
				{
					VectorSet(v, d2 * right * 0.5, 16, 0);
					G_ProjectSource (self->s.origin, v, v_forward, v_right, right_target);
//					gi.dprintf("incomplete path, go part way and adjust again\n");
				}
				VectorCopy (self->monsterinfo.last_sighting, self->monsterinfo.saved_goal);
				self->monsterinfo.aiflags |= AI_PURSUE_TEMP;
				VectorCopy (right_target, self->goalentity->s.origin);
				VectorCopy (right_target, self->monsterinfo.last_sighting);
				VectorSubtract (self->goalentity->s.origin, self->s.origin, v);
				self->s.angles[YAW] = self->ideal_yaw = vectoyaw(v);
//				gi.dprintf("adjusted right\n");
//				debug_drawline(self.origin, self.last_sighting, 152);
			}
		}
//		else gi.dprintf("course was fine\n");
	}

	M_MoveToGoal (self, dist);
	if(!self->inuse)
		return;			// PGM - g_touchtrigger free problem

	G_FreeEdict(tempgoal);

	if (self)
		self->goalentity = save;
}
