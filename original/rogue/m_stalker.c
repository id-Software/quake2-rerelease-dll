// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

stalker

==============================================================================
*/

#include "g_local.h"
#include "m_stalker.h"
#include <float.h>

static int	sound_pain;
static int	sound_die;
static int	sound_sight;
static int  sound_punch_hit1;
static int  sound_punch_hit2;
static int	sound_idle;

int stalker_do_pounce(edict_t *self, vec3_t dest);
void stalker_stand (edict_t *self);
void stalker_run (edict_t *self);
void stalker_walk (edict_t *self);
void stalker_jump (edict_t *self);
void stalker_dodge_jump (edict_t *self);
void stalker_swing_check_l (edict_t *self);
void stalker_swing_check_r (edict_t *self);
void stalker_swing_attack (edict_t *self);
void stalker_jump_straightup (edict_t *self);
void stalker_jump_wait_land (edict_t *self);
void stalker_false_death (edict_t *self);
void stalker_false_death_start (edict_t *self);
qboolean stalker_ok_to_transition (edict_t *self);

#define STALKER_ON_CEILING(ent)  ( ent->gravityVector[2] > 0 ? 1 : 0 )

//extern qboolean SV_StepDirection (edict_t *ent, float yaw, float dist);
extern qboolean SV_PointCloseEnough (edict_t *ent, vec3_t goal, float dist);
extern void drawbbox(edict_t *self);

//=========================
//=========================
qboolean stalker_ok_to_transition (edict_t *self)
{
	trace_t		trace;
	vec3_t		pt, start;
	float		max_dist;
	float		margin;
	float		end_height;

	if(STALKER_ON_CEILING(self))
	{
		max_dist = -384;
		margin = self->mins[2] - 8;
	}	
	else
	{
		// her stalkers are just better
		if (self->monsterinfo.aiflags & AI_SPAWNED_WIDOW)
			max_dist = 256;
		else
			max_dist = 180;
		margin = self->maxs[2] + 8;
	}

	VectorCopy(self->s.origin, pt);
	pt[2] += max_dist;
	trace = gi.trace (self->s.origin, self->mins, self->maxs, pt, self, MASK_MONSTERSOLID);

	if(trace.fraction == 1.0 || 
	   !(trace.contents & CONTENTS_SOLID) ||
	   (trace.ent != world))
	{
		if(STALKER_ON_CEILING(self))
		{
			if(trace.plane.normal[2] < 0.9)
				return false;
		}
		else
		{
			if(trace.plane.normal[2] > -0.9)
				return false;
		}
	}
//	gi.dprintf("stalker_check_pt: main check ok\n");

	end_height = trace.endpos[2];

	// check the four corners, tracing only to the endpoint of the center trace (vertically).
	pt[0] = self->absmin[0];
	pt[1] = self->absmin[1];
	pt[2] = trace.endpos[2] + margin;	// give a little margin of error to allow slight inclines
	VectorCopy(pt, start);
	start[2] = self->s.origin[2];
	trace = gi.trace( start, vec3_origin, vec3_origin, pt, self, MASK_MONSTERSOLID);
	if(trace.fraction == 1.0 || !(trace.contents & CONTENTS_SOLID) || (trace.ent != world))
	{
//		gi.dprintf("stalker_check_pt: absmin/absmin failed\n");
		return false;
	}
	if(abs(end_height + margin - trace.endpos[2]) > 8)
		return false;

	pt[0] = self->absmax[0];
	pt[1] = self->absmin[1];
	VectorCopy(pt, start);
	start[2] = self->s.origin[2];
	trace = gi.trace( start, vec3_origin, vec3_origin, pt, self, MASK_MONSTERSOLID);
	if(trace.fraction == 1.0 || !(trace.contents & CONTENTS_SOLID) || (trace.ent != world))
	{
//		gi.dprintf("stalker_check_pt: absmax/absmin failed\n");
		return false;
	}
	if(abs(end_height + margin - trace.endpos[2]) > 8)
		return false;

	pt[0] = self->absmax[0];
	pt[1] = self->absmax[1];
	VectorCopy(pt, start);
	start[2] = self->s.origin[2];
	trace = gi.trace( start, vec3_origin, vec3_origin, pt, self, MASK_MONSTERSOLID);
	if(trace.fraction == 1.0 || !(trace.contents & CONTENTS_SOLID) || (trace.ent != world))
	{
//		gi.dprintf("stalker_check_pt: absmax/absmax failed\n");
		return false;
	}
	if(abs(end_height + margin - trace.endpos[2]) > 8)
		return false;

	pt[0] = self->absmin[0];
	pt[1] = self->absmax[1];
	VectorCopy(pt, start);
	start[2] = self->s.origin[2];
	trace = gi.trace( start, vec3_origin, vec3_origin, pt, self, MASK_MONSTERSOLID);
	if(trace.fraction == 1.0 || !(trace.contents & CONTENTS_SOLID) || (trace.ent != world))
	{
//		gi.dprintf("stalker_check_pt: absmin/absmax failed\n");
		return false;
	}
	if(abs(end_height + margin - trace.endpos[2]) > 8)
		return false;

	return true;
}

//=========================
//=========================
void stalker_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_WEAPON, sound_sight, 1, ATTN_NORM, 0);
}

// ******************
// IDLE
// ******************

void stalker_idle_noise (edict_t *self)
{
	gi.sound (self, CHAN_WEAPON, sound_idle, 0.5, ATTN_IDLE, 0);
}

mframe_t stalker_frames_idle [] =
{
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, stalker_idle_noise,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL
};
mmove_t stalker_move_idle = {FRAME_idle01, FRAME_idle21, stalker_frames_idle, stalker_stand};

mframe_t stalker_frames_idle2 [] =
{
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL
};
mmove_t stalker_move_idle2 = {FRAME_idle201, FRAME_idle213, stalker_frames_idle2, stalker_stand};

void stalker_idle (edict_t *self)
{ 
	if (random() < 0.35)
		self->monsterinfo.currentmove = &stalker_move_idle;
	else
		self->monsterinfo.currentmove = &stalker_move_idle2;
}

// ******************
// STAND
// ******************

mframe_t stalker_frames_stand [] =
{
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, stalker_idle_noise,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,

	ai_stand, 0, NULL
};
mmove_t	stalker_move_stand = {FRAME_idle01, FRAME_idle21, stalker_frames_stand, stalker_stand};

void stalker_stand (edict_t *self)
{
	if (random() < 0.25)
		self->monsterinfo.currentmove = &stalker_move_stand;
	else
		self->monsterinfo.currentmove = &stalker_move_idle2;
}

// ******************
// RUN
// ******************

mframe_t stalker_frames_run [] =
{
	ai_run, 13, NULL,
	ai_run, 17, NULL,
	ai_run, 21, NULL,
	ai_run, 18, NULL

/*	ai_run, 15, NULL,
	ai_run, 20, NULL,
	ai_run, 18, NULL,
	ai_run, 14, NULL*/
};
mmove_t stalker_move_run = {FRAME_run01, FRAME_run04, stalker_frames_run, NULL};

void stalker_run (edict_t *self)
{
//	gi.dprintf("stalker_run %5.1f\n", level.time);
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &stalker_move_stand;
	else
		self->monsterinfo.currentmove = &stalker_move_run;
}

// ******************
// WALK
// ******************

mframe_t stalker_frames_walk [] =
{
	ai_walk, 4, NULL,
	ai_walk, 6, NULL,
	ai_walk, 8, NULL,
	ai_walk, 5, NULL,

	ai_walk, 4, NULL,
	ai_walk, 6, NULL,
	ai_walk, 8, NULL,
	ai_walk, 4, NULL
};
mmove_t stalker_move_walk = {FRAME_walk01, FRAME_walk08, stalker_frames_walk, stalker_walk};

void stalker_walk (edict_t *self)
{
//	gi.dprintf("stalker_walk\n");
	self->monsterinfo.currentmove = &stalker_move_walk;
}

// ******************
// false death
// ******************
mframe_t stalker_frames_reactivate [] = 
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t stalker_move_false_death_end = { FRAME_reactive01, FRAME_reactive04, stalker_frames_reactivate, stalker_run };

void stalker_reactivate (edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_STAND_GROUND;
	self->monsterinfo.currentmove = &stalker_move_false_death_end;
}

void stalker_heal (edict_t *self)
{
	if(skill->value == 2)
		self->health+=2;
	else if(skill->value == 3)
		self->health+=3;
	else
		self->health++;

//	gi.dprintf("stalker_heal: %d\n", self->health);

	if(self->health > (self->max_health/2))
		self->s.skinnum = 0;

	if(self->health >= self->max_health)
	{
		self->health = self->max_health;
		stalker_reactivate(self);
	}
}

mframe_t stalker_frames_false_death [] =
{
	ai_move, 0, stalker_heal,
	ai_move, 0, stalker_heal,
	ai_move, 0, stalker_heal,
	ai_move, 0, stalker_heal,
	ai_move, 0, stalker_heal,

	ai_move, 0, stalker_heal,
	ai_move, 0, stalker_heal,
	ai_move, 0, stalker_heal,
	ai_move, 0, stalker_heal,
	ai_move, 0, stalker_heal
};
mmove_t stalker_move_false_death = {FRAME_twitch01, FRAME_twitch10, stalker_frames_false_death, stalker_false_death};

void stalker_false_death (edict_t *self)
{
	self->monsterinfo.currentmove = &stalker_move_false_death;
}

mframe_t stalker_frames_false_death_start [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,

	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
};
mmove_t stalker_move_false_death_start = {FRAME_death01, FRAME_death09, stalker_frames_false_death_start, stalker_false_death};

void stalker_false_death_start (edict_t *self)
{
	self->s.angles[2] = 0;
	VectorSet(self->gravityVector, 0, 0, -1);

	self->monsterinfo.aiflags |= AI_STAND_GROUND;
	self->monsterinfo.currentmove = &stalker_move_false_death_start;
}


// ******************
// PAIN
// ******************

mframe_t stalker_frames_pain [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0,	NULL,
	ai_move, 0, NULL
};
mmove_t stalker_move_pain = {FRAME_pain01, FRAME_pain04, stalker_frames_pain, stalker_run};

void stalker_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->deadflag == DEAD_DEAD)
		return;

	if (self->health < (self->max_health / 2)) 
	{
		self->s.skinnum = 1;
	}

	if (skill->value == 3)
		return;		// no pain anims in nightmare

//	if (self->monsterinfo.aiflags & AI_DODGING)
//		monster_done_dodge (self);

	if (self->groundentity == NULL)
		return;

	// if we're reactivating or false dying, ignore the pain.
	if (self->monsterinfo.currentmove == &stalker_move_false_death_end ||
		self->monsterinfo.currentmove == &stalker_move_false_death_start )
		return;

	if (self->monsterinfo.currentmove == &stalker_move_false_death)
	{
		stalker_reactivate(self);
		return;
	}

	if ((self->health > 0) && (self->health < (self->max_health / 4)))
	{
		if(random() < (0.2 * skill->value))
		{
			if( !STALKER_ON_CEILING(self) || stalker_ok_to_transition(self) )
			{
//				gi.dprintf("starting false death sequence\n");
				stalker_false_death_start(self);
				return;
			}
		}	
	}

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;

//	gi.dprintf("stalker_pain\n");
	if (damage > 10)		// don't react unless the damage was significant
	{
		// stalker should dodge jump periodically to help avoid damage.
		if(self->groundentity && (random() < 0.5))
			stalker_dodge_jump(self);
		else
			self->monsterinfo.currentmove = &stalker_move_pain;

		gi.sound (self, CHAN_WEAPON, sound_pain, 1, ATTN_NORM, 0);
	}
}


// ******************
// STALKER ATTACK
// ******************

//extern qboolean infront (edict_t *self, edict_t *other);

void stalker_shoot_attack (edict_t *self)
{
	vec3_t	offset, start, f, r, dir;
	vec3_t	end;
	float	time, dist;
	trace_t	trace;

	if(!has_valid_enemy(self))
		return;

	if(self->groundentity && random() < 0.33)
	{
		VectorSubtract (self->enemy->s.origin, self->s.origin, dir);
		dist = VectorLength (dir);

		if((dist > 256) || (random() < 0.5))
			stalker_do_pounce(self, self->enemy->s.origin);
		else
			stalker_jump_straightup (self);
	}

	// FIXME -- keep this but use a custom one
//	if (!infront(self, self->enemy))
//		return;

	AngleVectors (self->s.angles, f, r, NULL);
	VectorSet (offset, 24, 0, 6);
	G_ProjectSource (self->s.origin, offset, f, r, start);

	VectorSubtract(self->enemy->s.origin, start, dir);
	if(random() < (0.20 + 0.1 * skill->value))
	{
		dist = VectorLength(dir);
		time = dist / 1000;
		VectorMA(self->enemy->s.origin, time, self->enemy->velocity, end);
		VectorSubtract(end, start, dir);
	}
	else
		VectorCopy(self->enemy->s.origin, end);

	trace = gi.trace(start, vec3_origin, vec3_origin, end, self, MASK_SHOT);
	if(trace.ent == self->enemy || trace.ent == world)
		monster_fire_blaster2(self, start, dir, 15, 800, MZ2_STALKER_BLASTER, EF_BLASTER);
//	else
//		gi.dprintf("blocked by entity %s\n", trace.ent->classname);
}

void stalker_shoot_attack2 (edict_t *self)
{
//	if (random() < (0.4+(float)skill->value))
//		stalker_shoot_attack (self);

	if (random() < (0.4 + (0.1 * (float)skill->value)))
		stalker_shoot_attack (self);
}

mframe_t stalker_frames_shoot [] =
{
	ai_charge, 13, NULL,
	ai_charge, 17, stalker_shoot_attack,
	ai_charge, 21, NULL,
	ai_charge, 18, stalker_shoot_attack2
};
mmove_t stalker_move_shoot = {FRAME_run01, FRAME_run04, stalker_frames_shoot, stalker_run};

void stalker_attack_ranged (edict_t *self)
{
	if(!has_valid_enemy(self))
		return;

	// PMM - circle strafe stuff
	if (random() > (1.0 - (0.5/(float)(skill->value))))
	{
		self->monsterinfo.attack_state = AS_STRAIGHT;
	}
	else
	{
		if (random () <= 0.5) // switch directions
			self->monsterinfo.lefty = 1 - self->monsterinfo.lefty;
		self->monsterinfo.attack_state = AS_SLIDING;
	}
	self->monsterinfo.currentmove = &stalker_move_shoot;
}

// ******************
// close combat
// ******************

void stalker_swing_attack (edict_t *self)
{
	vec3_t	aim;

	VectorSet (aim, MELEE_DISTANCE, 0, 0);
	if (fire_hit (self, aim, (5 + (rand() % 5)), 50))
		if (self->s.frame < FRAME_attack08)
			gi.sound (self, CHAN_WEAPON, sound_punch_hit2, 1, ATTN_NORM, 0);
		else
			gi.sound (self, CHAN_WEAPON, sound_punch_hit1, 1, ATTN_NORM, 0);
}

mframe_t stalker_frames_swing_l [] =
{
	ai_charge, 2, NULL,
	ai_charge, 4, NULL,
	ai_charge, 6, NULL,
	ai_charge, 10, NULL,

	ai_charge, 5, stalker_swing_attack,
	ai_charge, 5, NULL,
	ai_charge, 5, NULL,
	ai_charge, 5, NULL  // stalker_swing_check_l
};
mmove_t stalker_move_swing_l = {FRAME_attack01, FRAME_attack08, stalker_frames_swing_l, stalker_run};

mframe_t stalker_frames_swing_r [] =
{
	ai_charge, 4, NULL,
	ai_charge, 6, NULL,
	ai_charge, 6, stalker_swing_attack,
	ai_charge, 10, NULL,
	ai_charge, 5, NULL	// stalker_swing_check_r
};
mmove_t stalker_move_swing_r = {FRAME_attack11, FRAME_attack15, stalker_frames_swing_r, stalker_run};

void stalker_attack_melee (edict_t *self)
{
	if(!has_valid_enemy(self))
		return;

	if(random() < 0.5)
	{
		self->monsterinfo.currentmove = &stalker_move_swing_l;
	}
	else
	{
		self->monsterinfo.currentmove = &stalker_move_swing_r;
	}
}


// ******************
// POUNCE
// ******************

#define PI 3.14159
#define RAD2DEG(x)	(x * (float)180.0 / (float)PI)
#define DEG2RAD(x)	(x * (float)PI / (float)180.0)
#define FAUX_GRAVITY	800.0

// ====================
// ====================
void calcJumpAngle(vec3_t start, vec3_t end, float velocity, vec3_t angles)
{
	float	distV, distH;
	float	one, cosU;
	float	l, U;
	vec3_t	dist;

	VectorSubtract(end, start, dist);
	distH = (float)sqrt(dist[0]*dist[0] + dist[1]*dist[1]);
	distV = dist[2];
	if(distV < 0)
		distV = 0 - distV;

	if(distV)
	{
		l = (float) sqrt(distH*distH + distV*distV);
		U = (float) atan(distV / distH);
		if(dist[2] > 0)
			U = (float)0.0 - U;

		angles[2] = 0.0;

		cosU = (float)cos(U);
		one = l * FAUX_GRAVITY * (cosU * cosU);
		one = one / (velocity * velocity);
		one = one - (float)sin(U);
	//	one = ((l * FAUX_GRAVITY * (cosU * cosU)) / (velocity * velocity)) - (float)sin(U);
		angles[0] = (float)asin(one);
		if(_isnan(angles[0]))
			angles[2] = 1.0;
		angles[1] = (float)PI - angles[0];
		if(_isnan(angles[1]))
			angles[2] = 1.0;

		angles[0] = RAD2DEG ( (angles[0] - U) / 2.0 );
		angles[1] = RAD2DEG ( (angles[1] - U) / 2.0 );
	}
	else
	{
		l = (float) sqrt(distH*distH + distV*distV);

		angles[2] = 0.0;

		one = l * FAUX_GRAVITY;
		one = one / (velocity * velocity);
		angles[0] = (float)asin(one);
		if(_isnan(angles[0]))
			angles[2] = 1.0;
		angles[1] = (float)PI - angles[0];
		if(_isnan(angles[1]))
			angles[2] = 1.0;

		angles[0] = RAD2DEG ( (angles[0]) / 2.0 );
		angles[1] = RAD2DEG ( (angles[1]) / 2.0 );
	}
}

// ====================
// ====================
int stalker_check_lz (edict_t *self, edict_t *target, vec3_t dest)
{
	vec3_t	jumpLZ;

	if( (gi.pointcontents (dest) & MASK_WATER) || (target->waterlevel))
	{
//		gi.dprintf ("you won't make me jump in water!\n");
		return false;
	}

	if( !target->groundentity )
	{
//		gi.dprintf( "I'll wait until you land..\n");
		return false;
	}

	// check under the player's four corners
	// if they're not solid, bail.
	jumpLZ[0] = self->enemy->mins[0];
	jumpLZ[1] = self->enemy->mins[1];
	jumpLZ[2] = self->enemy->mins[2] - 0.25;
	if( !(gi.pointcontents (jumpLZ) & MASK_SOLID) )
		return false;

	jumpLZ[0] = self->enemy->maxs[0];
	jumpLZ[1] = self->enemy->mins[1];
	if( !(gi.pointcontents (jumpLZ) & MASK_SOLID) )
		return false;

	jumpLZ[0] = self->enemy->maxs[0];
	jumpLZ[1] = self->enemy->maxs[1];
	if( !(gi.pointcontents (jumpLZ) & MASK_SOLID) )
		return false;

	jumpLZ[0] = self->enemy->mins[0];
	jumpLZ[1] = self->enemy->maxs[1];
	if( !(gi.pointcontents (jumpLZ) & MASK_SOLID) )
		return false;

	return true;
}

// ====================
// ====================
int stalker_do_pounce(edict_t *self, vec3_t dest)
{
	vec3_t	forward, right;
	vec3_t	dist;
	vec_t	length;
	vec3_t	jumpAngles;
	vec3_t	jumpLZ;
	float	velocity = 400.1;
	trace_t	trace;
	int		preferHighJump;

	// don't pounce when we're on the ceiling
	if(STALKER_ON_CEILING(self))
		return false;

	if(!stalker_check_lz (self, self->enemy, dest))
		return false;

	VectorSubtract(dest, self->s.origin, dist);
	
	// make sure we're pointing in that direction 15deg margin of error.
	vectoangles2 (dist, jumpAngles);
	if(abs(jumpAngles[YAW] - self->s.angles[YAW]) > 45)
		return false;			// not facing the player...

	self->ideal_yaw = jumpAngles[YAW];
	M_ChangeYaw(self);

	length = VectorLength(dist);
	if(length > 450)
		return false;			// can't jump that far...

	VectorCopy(dest, jumpLZ);

	preferHighJump = 0;

	// if we're having to jump up a distance, jump a little too high to compensate.
	if(dist[2] >= 32.0)
	{
		preferHighJump = 1;
		jumpLZ[2] += 32;
	}

	trace = gi.trace (self->s.origin, vec3_origin, vec3_origin, dest, self, MASK_MONSTERSOLID);
	if((trace.fraction < 1) && (trace.ent != self->enemy))
	{
//		gi.dprintf("prefer high jump angle\n");
		preferHighJump = 1; 
	}

	// find a valid angle/velocity combination
	while(velocity <= 800)
	{
		calcJumpAngle(self->s.origin, jumpLZ, velocity, jumpAngles);
		if((!_isnan(jumpAngles[0]))  || (!_isnan(jumpAngles[1])))
			break;
		
		velocity+=200;
	};

	if(!preferHighJump && (!_isnan(jumpAngles[0])) )
	{
		AngleVectors (self->s.angles, forward, right, NULL);
		VectorNormalize ( forward ) ;

		VectorScale( forward, velocity * cos(DEG2RAD(jumpAngles[0])), self->velocity);
		self->velocity[2] = velocity * sin(DEG2RAD(jumpAngles[0])) + (0.5 * sv_gravity->value * FRAMETIME);
//		gi.dprintf("  pouncing! %0.1f,%0.1f (%0.1f)  --> %0.1f, %0.1f, %0.1f\n", 
//				jumpAngles[0], jumpAngles[1], jumpAngles[0],
//				self->velocity[0], self->velocity[1], self->velocity[2]);
		return 1;
	}

	if(!_isnan(jumpAngles[1]))
	{
		AngleVectors (self->s.angles, forward, right, NULL);
		VectorNormalize ( forward ) ;

		VectorScale( forward, velocity * cos(DEG2RAD(jumpAngles[1])), self->velocity);
		self->velocity[2] = velocity * sin(DEG2RAD(jumpAngles[1])) + (0.5 * sv_gravity->value * FRAMETIME);
//		gi.dprintf("  pouncing! %0.1f,%0.1f (%0.1f)  --> %0.1f, %0.1f, %0.1f\n", 
//				jumpAngles[0], jumpAngles[1], jumpAngles[1],
//				self->velocity[0], self->velocity[1], self->velocity[2]);
		return 1;
	}

//	gi.dprintf("  nan\n");
	return 0;
}

// ******************
// DODGE
// ******************

//===================
// stalker_jump_straightup
//===================
void stalker_jump_straightup (edict_t *self)
{
	if (self->deadflag == DEAD_DEAD)
		return;

	if(STALKER_ON_CEILING(self))
	{
		if(stalker_ok_to_transition(self))
		{
//			gi.dprintf("falling off ceiling %d\n", self->health);
			self->gravityVector[2] = -1;
			self->s.angles[2] += 180.0;
			if(self->s.angles[2] > 360.0)
				self->s.angles[2] -= 360.0;
			self->groundentity = NULL;
		}
	}
	else if(self->groundentity)	// make sure we're standing on SOMETHING...
	{
		self->velocity[0] += ((random() * 10) - 5);
		self->velocity[1] += ((random() * 10) - 5);
		self->velocity[2] += -400 * self->gravityVector[2];
		if(stalker_ok_to_transition(self))
		{
//			gi.dprintf("falling TO ceiling %d\n", self->health);
			self->gravityVector[2] = 1;
			self->s.angles[2] = 180.0;
			self->groundentity = NULL;
		}
	}
}

mframe_t stalker_frames_jump_straightup [] =
{
	ai_move, 1,  stalker_jump_straightup,
	ai_move, 1,  stalker_jump_wait_land,
	ai_move, -1, NULL,
	ai_move, -1, NULL
};

mmove_t	stalker_move_jump_straightup = {FRAME_jump04, FRAME_jump07, stalker_frames_jump_straightup, stalker_run};

//===================
// stalker_dodge_jump - abstraction so pain function can trigger a dodge jump too without
//		faking the inputs to stalker_dodge
//===================
void stalker_dodge_jump (edict_t *self)
{
	self->monsterinfo.currentmove = &stalker_move_jump_straightup;
}

mframe_t stalker_frames_dodge_run [] =
{
	ai_run, 13, NULL,
	ai_run, 17, NULL,
	ai_run, 21, NULL,
	ai_run, 18, monster_done_dodge
};
mmove_t stalker_move_dodge_run = {FRAME_run01, FRAME_run04, stalker_frames_dodge_run, NULL};

void stalker_dodge (edict_t *self, edict_t *attacker, float eta, trace_t *tr)
{
	if (!self->groundentity || self->health <= 0)
		return;

	if (!self->enemy)
	{
		self->enemy = attacker;
		FoundTarget(self);
		return;
	}
	
	// PMM - don't bother if it's going to hit anyway; fix for weird in-your-face etas (I was
	// seeing numbers like 13 and 14)
	if ((eta < 0.1) || (eta > 5))
		return;

	// this will override the foundtarget call of stalker_run
	stalker_dodge_jump(self);
}


// ******************
// Jump onto / off of things
// ******************

//===================
//===================
void stalker_jump_down (edict_t *self)
{
	vec3_t	forward,up;

	monster_jump_start (self);

	AngleVectors (self->s.angles, forward, NULL, up);
	VectorMA(self->velocity, 100, forward, self->velocity);
	VectorMA(self->velocity, 300, up, self->velocity);
}

//===================
//===================
void stalker_jump_up (edict_t *self)
{
	vec3_t	forward,up;

	monster_jump_start (self);

	AngleVectors (self->s.angles, forward, NULL, up);
	VectorMA(self->velocity, 200, forward, self->velocity);
	VectorMA(self->velocity, 450, up, self->velocity);
}

//===================
//===================
void stalker_jump_wait_land (edict_t *self)
{
	if ((random() < (0.3 + (0.1*(float)(skill->value)))) && (level.time >= self->monsterinfo.attack_finished))
	{
		self->monsterinfo.attack_finished = level.time + 0.3;
		stalker_shoot_attack(self);
	}

	if(self->groundentity == NULL)
	{
		self->gravity = 1.3;
		self->monsterinfo.nextframe = self->s.frame;

		if(monster_jump_finished (self))
		{
			self->gravity = 1;
			self->monsterinfo.nextframe = self->s.frame + 1;
		}
	}
	else 
	{
		self->gravity = 1;
		self->monsterinfo.nextframe = self->s.frame + 1;
	}
}

mframe_t stalker_frames_jump_up [] =
{
	ai_move, -8, NULL,
	ai_move, -8, NULL,
	ai_move, -8, NULL,
	ai_move, -8, NULL,

	ai_move, 0, stalker_jump_up,
	ai_move, 0, stalker_jump_wait_land,
	ai_move, 0, NULL
};
mmove_t stalker_move_jump_up = { FRAME_jump01, FRAME_jump07, stalker_frames_jump_up, stalker_run };

mframe_t stalker_frames_jump_down [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	
	ai_move, 0, stalker_jump_down,
	ai_move, 0, stalker_jump_wait_land,
	ai_move, 0, NULL
};
mmove_t stalker_move_jump_down = { FRAME_jump01, FRAME_jump07, stalker_frames_jump_down, stalker_run };

//============
// stalker_jump - this is only used for jumping onto or off of things. for dodge jumping,
//		use stalker_dodge_jump
//============
void stalker_jump (edict_t *self)
{
	if(!self->enemy)
		return;

	if(self->enemy->s.origin[2] >= self->s.origin[2])
	{
//		gi.dprintf("stalker_jump_up\n");
		self->monsterinfo.currentmove = &stalker_move_jump_up;
	}
	else
	{
//		gi.dprintf("stalker_jump_down\n");
		self->monsterinfo.currentmove = &stalker_move_jump_down;
	}
}


// ******************
// Blocked
// ******************

qboolean stalker_blocked (edict_t *self, float dist)
{
	qboolean	onCeiling;

//	gi.dprintf("stalker_blocked\n");
	if(!has_valid_enemy(self))
		return false;

	onCeiling = false;
	if(self->gravityVector[2] > 0)
		onCeiling = true;

	if(!onCeiling)
	{
		if(blocked_checkshot(self, 0.25 + (0.05 * skill->value) ))
		{
//			gi.dprintf("blocked: shooting\n");
			return true;
		}

		if(visible (self, self->enemy))
		{
//			gi.dprintf("blocked: jumping at player!\n");
			stalker_do_pounce(self, self->enemy->s.origin);
			return true;
		}

		if(blocked_checkjump (self, dist, 256, 68))
		{
//			gi.dprintf("blocked: jumping up/down\n");
			stalker_jump (self);
			return true;
		}

		if(blocked_checkplat (self, dist))
			return true;
	}
	else
	{
		if(blocked_checkshot(self, 0.25 + (0.05 * skill->value) ))
		{
//			gi.dprintf("blocked: shooting\n");
			return true;
		}	
		else if(stalker_ok_to_transition(self))
		{
			self->gravityVector[2] = -1;
			self->s.angles[2] += 180.0;
			if(self->s.angles[2] > 360.0)
				self->s.angles[2] -= 360.0;
			self->groundentity = NULL;
			
//			gi.dprintf("falling off ceiling\n");
			return true;
		}
//		else
//			gi.dprintf("Not OK to fall!\n");
	}

	return false;
}

// ******************
// Death
// ******************

void stalker_dead (edict_t *self)
{
	VectorSet (self->mins, -28, -28, -18);
	VectorSet (self->maxs, 28, 28, -4);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity (self);
//	drawbbox(self);
}

mframe_t stalker_frames_death [] =
{
	ai_move, 0,	 NULL,
	ai_move, -5,	 NULL,
	ai_move, -10,	 NULL,
	ai_move, -20,	 NULL,
	
	ai_move, -10,	 NULL,
	ai_move, -10,	 NULL,
	ai_move, -5,	 NULL,
	ai_move, -5,	 NULL,

	ai_move, 0,	 NULL
};
mmove_t stalker_move_death = {FRAME_death01, FRAME_death09, stalker_frames_death, stalker_dead};

void stalker_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		n;

//	gi.dprintf("stalker_die: %d\n", self->health);

// dude bit it, make him fall!
	self->movetype = MOVETYPE_TOSS;
	self->s.angles[2] = 0;
	VectorSet(self->gravityVector, 0, 0, -1);

// check for gib
	if (self->health <= self->gib_health)
	{
		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n= 0; n < 2; n++)
			ThrowGib (self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
		for (n= 0; n < 4; n++)
			ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowHead (self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

// regular death
	gi.sound (self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.currentmove = &stalker_move_death;
}


// ******************
// SPAWN
// ******************

/*QUAKED monster_stalker (1 .5 0) (-28 -28 -18) (28 28 18) Ambush Trigger_Spawn Sight OnRoof
Spider Monster

  ONROOF - Monster starts sticking to the roof.
*/
void SP_monster_stalker (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	sound_pain = gi.soundindex ("stalker/pain.wav");	
	sound_die = gi.soundindex ("stalker/death.wav");	
	sound_sight = gi.soundindex("stalker/sight.wav");
	sound_punch_hit1 = gi.soundindex ("stalker/melee1.wav");
	sound_punch_hit2 = gi.soundindex ("stalker/melee2.wav");
	sound_idle = gi.soundindex ("stalker/idle.wav");

	// PMM - precache bolt2
	gi.modelindex ("models/proj/laser2/tris.md2");

	self->s.modelindex = gi.modelindex ("models/monsters/stalker/tris.md2");
	VectorSet (self->mins, -28, -28, -18);
	VectorSet (self->maxs, 28, 28, 18);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->health = 250;
	self->gib_health = -50;		// FIXME 
	self->mass = 250;

	self->pain = stalker_pain;
	self->die = stalker_die;

	self->monsterinfo.stand = stalker_stand;
	self->monsterinfo.walk = stalker_walk;
	self->monsterinfo.run = stalker_run;
	self->monsterinfo.attack = stalker_attack_ranged;
	self->monsterinfo.sight = stalker_sight;
	self->monsterinfo.idle = stalker_idle;
	self->monsterinfo.dodge = stalker_dodge;
	self->monsterinfo.blocked = stalker_blocked;
	self->monsterinfo.melee = stalker_attack_melee;

	gi.linkentity (self);

	self->monsterinfo.currentmove = &stalker_move_stand;	
	self->monsterinfo.scale = MODEL_SCALE;

	self->monsterinfo.aiflags |= AI_WALK_WALLS;

	if(self->spawnflags & 8)
	{
		self->s.angles[2] = 180;
		self->gravityVector[2] = 1;
	}

	walkmonster_start (self);
}
