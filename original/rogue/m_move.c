// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// m_move.c -- monster movement

#include "g_local.h"

#define	STEPSIZE	18

// this is used for communications out of sv_movestep to say what entity
// is blocking us
edict_t		*new_bad;			//pmm

/*
=============
M_CheckBottom

Returns false if any part of the bottom of the entity is off an edge that
is not a staircase.

=============
*/
int c_yes, c_no;

qboolean M_CheckBottom (edict_t *ent)
{
	vec3_t	mins, maxs, start, stop;
	trace_t	trace;
	int		x, y;
	float	mid, bottom;
	
	VectorAdd (ent->s.origin, ent->mins, mins);
	VectorAdd (ent->s.origin, ent->maxs, maxs);

// if all of the points under the corners are solid world, don't bother
// with the tougher checks
// the corners must be within 16 of the midpoint

//PGM
#ifdef ROGUE_GRAVITY
	// FIXME - this will only handle 0,0,1 and 0,0,-1 gravity vectors
	start[2] = mins[2] - 1;
	if(ent->gravityVector[2] > 0)
		start[2] = maxs[2] + 1;
#else
	start[2] = mins[2] - 1;
#endif
//PGM

	for	(x=0 ; x<=1 ; x++)
		for	(y=0 ; y<=1 ; y++)
		{
			start[0] = x ? maxs[0] : mins[0];
			start[1] = y ? maxs[1] : mins[1];
			if (gi.pointcontents (start) != CONTENTS_SOLID)
				goto realcheck;
		}

	c_yes++;
	return true;		// we got out easy

realcheck:
	c_no++;
//
// check it for real...
//
	start[2] = mins[2];
	
// the midpoint must be within 16 of the bottom
	start[0] = stop[0] = (mins[0] + maxs[0])*0.5;
	start[1] = stop[1] = (mins[1] + maxs[1])*0.5;

//PGM
#ifdef ROGUE_GRAVITY
	if(ent->gravityVector[2] < 0)
	{
		start[2] = mins[2];
		stop[2] = start[2] - STEPSIZE - STEPSIZE;
	}
	else
	{
		start[2] = maxs[2];
		stop[2] = start[2] + STEPSIZE + STEPSIZE;
	}
#else
	stop[2] = start[2] - 2*STEPSIZE;
#endif
//PGM

	trace = gi.trace (start, vec3_origin, vec3_origin, stop, ent, MASK_MONSTERSOLID);

	if (trace.fraction == 1.0)
		return false;
	mid = bottom = trace.endpos[2];
	
// the corners must be within 16 of the midpoint	
	for	(x=0 ; x<=1 ; x++)
		for	(y=0 ; y<=1 ; y++)
		{
			start[0] = stop[0] = x ? maxs[0] : mins[0];
			start[1] = stop[1] = y ? maxs[1] : mins[1];
			
			trace = gi.trace (start, vec3_origin, vec3_origin, stop, ent, MASK_MONSTERSOLID);

//PGM
#ifdef ROGUE_GRAVITY
			// FIXME - this will only handle 0,0,1 and 0,0,-1 gravity vectors
			if(ent->gravityVector[2] > 0)
			{
				if (trace.fraction != 1.0 && trace.endpos[2] < bottom)
					bottom = trace.endpos[2];
				if (trace.fraction == 1.0 || trace.endpos[2] - mid > STEPSIZE)
					return false;
			}
			else
			{
				if (trace.fraction != 1.0 && trace.endpos[2] > bottom)
					bottom = trace.endpos[2];
				if (trace.fraction == 1.0 || mid - trace.endpos[2] > STEPSIZE)
					return false;
			}
#else
			if (trace.fraction != 1.0 && trace.endpos[2] > bottom)
				bottom = trace.endpos[2];
			if (trace.fraction == 1.0 || mid - trace.endpos[2] > STEPSIZE)
				return false;
#endif
//PGM
		}

	c_yes++;
	return true;
}

//============
// ROGUE
qboolean IsBadAhead (edict_t *self, edict_t *bad, vec3_t move)
{
	vec3_t	dir;
	vec3_t	forward;
	float	dp_bad, dp_move;
	vec3_t	move_copy;

	VectorCopy (move, move_copy);

	VectorSubtract (bad->s.origin, self->s.origin, dir);
	VectorNormalize (dir);
	AngleVectors (self->s.angles, forward, NULL, NULL);
	dp_bad = DotProduct (forward, dir);

	VectorNormalize (move_copy);
	AngleVectors (self->s.angles, forward, NULL, NULL);
	dp_move = DotProduct (forward, move_copy);

	if ((dp_bad < 0) && (dp_move < 0))
		return true;
	if ((dp_bad > 0) && (dp_move > 0))
		return true;

	return false;
/*
	if(DotProduct(forward, dir) > 0)
	{
//		gi.dprintf ("bad ahead...\n");
		return true;
	}

//	gi.dprintf ("bad behind...\n");
	return false;
	*/
}
// ROGUE
//============

/*
=============
SV_movestep

Called by monster program code.
The move will be adjusted for slopes and stairs, but if the move isn't
possible, no move is done, false is returned, and
pr_global_struct->trace_normal is set to the normal of the blocking wall
=============
*/
//FIXME since we need to test end position contents here, can we avoid doing
//it again later in catagorize position?
qboolean SV_movestep (edict_t *ent, vec3_t move, qboolean relink)
{
	float		dz;
	vec3_t		oldorg, neworg, end;
	trace_t		trace;
	int			i;
	float		stepsize;
	vec3_t		test;
	int			contents;
	edict_t		*current_bad;		// PGM
	float		minheight;			// pmm

//======
//PGM

	// PMM - who cares about bad areas if you're dead?
	if (ent->health > 0)
	{
		current_bad = CheckForBadArea(ent);
		if(current_bad)
		{
			ent->bad_area = current_bad;
			 
			if(ent->enemy && !strcmp(ent->enemy->classname, "tesla"))
			{
				// if the tesla is in front of us, back up...
				if (IsBadAhead (ent, current_bad, move))
					VectorScale(move, -1, move);
			}
		}
		else if(ent->bad_area)
		{
			// if we're no longer in a bad area, get back to business.
			ent->bad_area = NULL;
			if(ent->oldenemy)// && ent->bad_area->owner == ent->enemy)
			{
	//			gi.dprintf("resuming being pissed at %s\n", ent->oldenemy->classname);
				ent->enemy = ent->oldenemy;
				ent->goalentity = ent->oldenemy;
				FoundTarget(ent);
	// FIXME - remove this when ready!!!
//	if (ent->lastMoveTime == level.time)
//		if ((g_showlogic) && (g_showlogic->value))
//			gi.dprintf ("Duplicate move detected for %s, please tell programmers!\n", ent->classname);
//	ent->lastMoveTime = level.time;
	// FIXME

				return true;
			}
		}
	}
//PGM
//======

// try the move	
	VectorCopy (ent->s.origin, oldorg);
	VectorAdd (ent->s.origin, move, neworg);

// flying monsters don't step up
	if ( ent->flags & (FL_SWIM | FL_FLY) )
	{
	// try one move with vertical motion, then one without
		for (i=0 ; i<2 ; i++)
		{
			VectorAdd (ent->s.origin, move, neworg);
			if (i == 0 && ent->enemy)
			{
				if (!ent->goalentity)
					ent->goalentity = ent->enemy;
				dz = ent->s.origin[2] - ent->goalentity->s.origin[2];
				if (ent->goalentity->client)
				{
					// we want the carrier to stay a certain distance off the ground, to help prevent him
					// from shooting his fliers, who spawn in below him
					//
					if (!strcmp(ent->classname, "monster_carrier"))
						minheight = 104;
					else
						minheight = 40;
//					if (dz > 40)
					if (dz > minheight)
//	pmm		
						neworg[2] -= 8;
					if (!((ent->flags & FL_SWIM) && (ent->waterlevel < 2)))
						if (dz < (minheight - 10))
							neworg[2] += 8;
				}
				else
				{
					if (dz > 8)
						neworg[2] -= 8;
					else if (dz > 0)
						neworg[2] -= dz;
					else if (dz < -8)
						neworg[2] += 8;
					else
						neworg[2] += dz;
				}
			}

			trace = gi.trace (ent->s.origin, ent->mins, ent->maxs, neworg, ent, MASK_MONSTERSOLID);
	
			// fly monsters don't enter water voluntarily
			if (ent->flags & FL_FLY)
			{
				if (!ent->waterlevel)
				{
					test[0] = trace.endpos[0];
					test[1] = trace.endpos[1];
					test[2] = trace.endpos[2] + ent->mins[2] + 1;
					contents = gi.pointcontents(test);
					if (contents & MASK_WATER)
						return false;
				}
			}

			// swim monsters don't exit water voluntarily
			if (ent->flags & FL_SWIM)
			{
				if (ent->waterlevel < 2)
				{
					test[0] = trace.endpos[0];
					test[1] = trace.endpos[1];
					test[2] = trace.endpos[2] + ent->mins[2] + 1;
					contents = gi.pointcontents(test);
					if (!(contents & MASK_WATER))
						return false;
				}
			}

//			if (trace.fraction == 1)

			// PMM - changed above to this
			if ((trace.fraction == 1) && (!trace.allsolid) && (!trace.startsolid))
			{
				VectorCopy (trace.endpos, ent->s.origin);
//=====
//PGM				
				if(!current_bad && CheckForBadArea(ent))
				{
//						gi.dprintf("Oooh! Bad Area!\n");
					VectorCopy (oldorg, ent->s.origin);
				}
				else
				{
					if (relink)
					{
						gi.linkentity (ent);
						G_TouchTriggers (ent);
					}
	// FIXME - remove this when ready!!!
//	if (ent->lastMoveTime == level.time)
//		if ((g_showlogic) && (g_showlogic->value))
//			gi.dprintf ("Duplicate move detected for %s, please tell programmers!\n", ent->classname);
//	ent->lastMoveTime = level.time;
	// FIXME

					return true;
				}
//PGM
//=====
			}
			
			if (!ent->enemy)
				break;
		}
		
		return false;
	}

// push down from a step height above the wished position
	if (!(ent->monsterinfo.aiflags & AI_NOSTEP))
		stepsize = STEPSIZE;
	else
		stepsize = 1;

//PGM
#ifdef ROGUE_GRAVITY
	// trace from 1 stepsize gravityUp to 2 stepsize gravityDown.
	VectorMA(neworg, -1 * stepsize, ent->gravityVector, neworg);
	VectorMA(neworg, 2 * stepsize, ent->gravityVector, end);
#else
	neworg[2] += stepsize;
	VectorCopy (neworg, end);
	end[2] -= stepsize*2;
#endif
//PGM

	trace = gi.trace (neworg, ent->mins, ent->maxs, end, ent, MASK_MONSTERSOLID);

	if (trace.allsolid)
		return false;

	if (trace.startsolid)
	{
		neworg[2] -= stepsize;
		trace = gi.trace (neworg, ent->mins, ent->maxs, end, ent, MASK_MONSTERSOLID);
		if (trace.allsolid || trace.startsolid)
			return false;
	}


	// don't go in to water
	if (ent->waterlevel == 0)
	{
//PGM
#ifdef ROGUE_GRAVITY
		test[0] = trace.endpos[0];
		test[1] = trace.endpos[1];
		if(ent->gravityVector[2] > 0)
			test[2] = trace.endpos[2] + ent->maxs[2] - 1;	
		else
			test[2] = trace.endpos[2] + ent->mins[2] + 1;
#else
		test[0] = trace.endpos[0];
		test[1] = trace.endpos[1];
		test[2] = trace.endpos[2] + ent->mins[2] + 1;	
#endif
//PGM

		contents = gi.pointcontents(test);

		if (contents & MASK_WATER)
			return false;
	}

	if (trace.fraction == 1)
	{
	// if monster had the ground pulled out, go ahead and fall
		if ( ent->flags & FL_PARTIALGROUND )
		{
			VectorAdd (ent->s.origin, move, ent->s.origin);
			if (relink)
			{
				gi.linkentity (ent);
				G_TouchTriggers (ent);
			}
			ent->groundentity = NULL;
	// FIXME - remove this when ready!!!
//	if (ent->lastMoveTime == level.time)
//		if ((g_showlogic) && (g_showlogic->value))
//			gi.dprintf ("Duplicate move detected for %s, please tell programmers!\n", ent->classname);
//	ent->lastMoveTime = level.time;
	// FIXME

			return true;
		}
	
		return false;		// walked off an edge
	}

// check point traces down for dangling corners
	VectorCopy (trace.endpos, ent->s.origin);

//PGM
	// PMM - don't bother with bad areas if we're dead
	if (ent->health > 0)
	{
		// use AI_BLOCKED to tell the calling layer that we're now mad at a tesla
		new_bad = CheckForBadArea(ent);
		if(!current_bad && new_bad)
		{
			if (new_bad->owner)
			{
//				if ((g_showlogic) && (g_showlogic->value))
//					gi.dprintf("Blocked -");
				if (!strcmp(new_bad->owner->classname, "tesla"))
				{
//					if ((g_showlogic) && (g_showlogic->value))
//						gi.dprintf ("it's a tesla -");
					if ((!(ent->enemy)) || (!(ent->enemy->inuse)))
					{
//						if ((g_showlogic) && (g_showlogic->value))
//							gi.dprintf ("I don't have a valid enemy, attacking tesla!\n");
						TargetTesla (ent, new_bad->owner);
						ent->monsterinfo.aiflags |= AI_BLOCKED;
					}
					else if (!strcmp(ent->enemy->classname, "telsa"))
					{
//						if ((g_showlogic) && (g_showlogic->value))
//							gi.dprintf ("but we're already mad at a tesla\n");
					}
					else if ((ent->enemy) && (ent->enemy->client))
					{
//						if ((g_showlogic) && (g_showlogic->value))
//							gi.dprintf ("we have a player enemy -");
						if (visible(ent, ent->enemy))
						{
//							if ((g_showlogic) && (g_showlogic->value))
//								gi.dprintf ("we can see him -");
						}
						else
						{
//							if ((g_showlogic) && (g_showlogic->value))
//								gi.dprintf ("can't see him, kill the tesla! -");
							TargetTesla (ent, new_bad->owner);
							ent->monsterinfo.aiflags |= AI_BLOCKED;
						}
					}
					else
					{
//						if ((g_showlogic) && (g_showlogic->value))
//							gi.dprintf ("the enemy isn't a player, killing tesla -");
						TargetTesla (ent, new_bad->owner);
						ent->monsterinfo.aiflags |= AI_BLOCKED;
					}
				}
//				else if ((g_showlogic) && (g_showlogic->value))
//				{
//					gi.dprintf(" by non-tesla bad area!");
//				}
			}
//			gi.dprintf ("\n");

			VectorCopy (oldorg, ent->s.origin);
			return false;
		}
	}
//PGM

	if (!M_CheckBottom (ent))
	{
		if ( ent->flags & FL_PARTIALGROUND )
		{	// entity had floor mostly pulled out from underneath it
			// and is trying to correct
			if (relink)
			{
				gi.linkentity (ent);
				G_TouchTriggers (ent);
			}
	// FIXME - remove this when ready!!!
//	if (ent->lastMoveTime == level.time)
//		if ((g_showlogic) && (g_showlogic->value))
//			gi.dprintf ("Duplicate move detected for %s, please tell programmers!\n", ent->classname);
//	ent->lastMoveTime = level.time;
	// FIXME

			return true;
		}
		VectorCopy (oldorg, ent->s.origin);
		return false;
	}

	if ( ent->flags & FL_PARTIALGROUND )
	{
		ent->flags &= ~FL_PARTIALGROUND;
	}
	ent->groundentity = trace.ent;
	ent->groundentity_linkcount = trace.ent->linkcount;

// the move is ok
	if (relink)
	{
		gi.linkentity (ent);
		G_TouchTriggers (ent);
	}
	// FIXME - remove this when ready!!!
//	if (ent->lastMoveTime == level.time)
//		if ((g_showlogic) && (g_showlogic->value))
//			gi.dprintf ("Duplicate move detected for %s, please tell programmers!\n", ent->classname);
//	ent->lastMoveTime = level.time;
	// FIXME

	return true;
}


//============================================================================

/*
===============
M_ChangeYaw

===============
*/
void M_ChangeYaw (edict_t *ent)
{
	float	ideal;
	float	current;
	float	move;
	float	speed;
	
	current = anglemod(ent->s.angles[YAW]);
	ideal = ent->ideal_yaw;

	if (current == ideal)
		return;

	move = ideal - current;
	speed = ent->yaw_speed;
	if (ideal > current)
	{
		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
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
	
	ent->s.angles[YAW] = anglemod (current + move);
}


/*
======================
SV_StepDirection

Turns to the movement direction, and walks the current distance if
facing it.

======================
*/
qboolean SV_StepDirection (edict_t *ent, float yaw, float dist)
{
	vec3_t		move, oldorigin;
	float		delta;
	
	if(!ent->inuse)	return true;		// PGM g_touchtrigger free problem

	ent->ideal_yaw = yaw;
	M_ChangeYaw (ent);
	
	yaw = yaw*M_PI*2 / 360;
	move[0] = cos(yaw)*dist;
	move[1] = sin(yaw)*dist;
	move[2] = 0;

	VectorCopy (ent->s.origin, oldorigin);
	if (SV_movestep (ent, move, false))
	{
		ent->monsterinfo.aiflags &= ~AI_BLOCKED;
		if(!ent->inuse)	return true;		// PGM g_touchtrigger free problem

		delta = ent->s.angles[YAW] - ent->ideal_yaw;
		if (strncmp(ent->classname, "monster_widow", 13))
		{
			if (delta > 45 && delta < 315)
			{		// not turned far enough, so don't take the step
				VectorCopy (oldorigin, ent->s.origin);
			}
		}
		gi.linkentity (ent);
		G_TouchTriggers (ent);
		return true;
	}
	gi.linkentity (ent);
	G_TouchTriggers (ent);
	return false;
}

/*
======================
SV_FixCheckBottom

======================
*/
void SV_FixCheckBottom (edict_t *ent)
{
	ent->flags |= FL_PARTIALGROUND;
}



/*
================
SV_NewChaseDir

================
*/
#define	DI_NODIR	-1
void SV_NewChaseDir (edict_t *actor, edict_t *enemy, float dist)
{
	float	deltax,deltay;
	float	d[3];
	float	tdir, olddir, turnaround;

	//FIXME: how did we get here with no enemy
	if (!enemy)
		return;

	olddir = anglemod( (int)(actor->ideal_yaw/45)*45 );
	turnaround = anglemod(olddir - 180);

	deltax = enemy->s.origin[0] - actor->s.origin[0];
	deltay = enemy->s.origin[1] - actor->s.origin[1];
	if (deltax>10)
		d[1]= 0;
	else if (deltax<-10)
		d[1]= 180;
	else
		d[1]= DI_NODIR;
	if (deltay<-10)
		d[2]= 270;
	else if (deltay>10)
		d[2]= 90;
	else
		d[2]= DI_NODIR;

// try direct route
	if (d[1] != DI_NODIR && d[2] != DI_NODIR)
	{
		if (d[1] == 0)
			tdir = d[2] == 90 ? 45 : 315;
		else
			tdir = d[2] == 90 ? 135 : 215;
			
		if (tdir != turnaround && SV_StepDirection(actor, tdir, dist))
			return;
	}

// try other directions
	if ( ((rand()&3) & 1) ||  abs(deltay)>abs(deltax))
	{
		tdir=d[1];
		d[1]=d[2];
		d[2]=tdir;
	}

	if (d[1]!=DI_NODIR && d[1]!=turnaround 
	&& SV_StepDirection(actor, d[1], dist))
			return;

	if (d[2]!=DI_NODIR && d[2]!=turnaround
	&& SV_StepDirection(actor, d[2], dist))
			return;

//ROGUE
	if(actor->monsterinfo.blocked)
	{
		if ((actor->inuse) && (actor->health > 0))
		{
			if((actor->monsterinfo.blocked)(actor, dist))
				return;
		}
	}
//ROGUE

/* there is no direct path to the player, so pick another direction */

	if (olddir!=DI_NODIR && SV_StepDirection(actor, olddir, dist))
			return;

	if (rand()&1) 	/*randomly determine direction of search*/
	{
		for (tdir=0 ; tdir<=315 ; tdir += 45)
			if (tdir!=turnaround && SV_StepDirection(actor, tdir, dist) )
					return;
	}
	else
	{
		for (tdir=315 ; tdir >=0 ; tdir -= 45)
			if (tdir!=turnaround && SV_StepDirection(actor, tdir, dist) )
					return;
	}

	if (turnaround != DI_NODIR && SV_StepDirection(actor, turnaround, dist) )
			return;

	actor->ideal_yaw = olddir;		// can't move

// if a bridge was pulled out from underneath a monster, it may not have
// a valid standing position at all

	if (!M_CheckBottom (actor))
		SV_FixCheckBottom (actor);
}

/*
======================
SV_CloseEnough

======================
*/
qboolean SV_CloseEnough (edict_t *ent, edict_t *goal, float dist)
{
	int		i;
	
	for (i=0 ; i<3 ; i++)
	{
		if (goal->absmin[i] > ent->absmax[i] + dist)
			return false;
		if (goal->absmax[i] < ent->absmin[i] - dist)
			return false;
	}
	return true;
}

/*
======================
M_MoveToGoal
======================
*/
void M_MoveToGoal (edict_t *ent, float dist)
{
	edict_t		*goal;
	
	goal = ent->goalentity;

	if (!ent->groundentity && !(ent->flags & (FL_FLY|FL_SWIM)))
		return;

// if the next step hits the enemy, return immediately
	if (ent->enemy &&  SV_CloseEnough (ent, ent->enemy, dist) )
		return;

// bump around...
//	if ( (rand()&3)==1 || !SV_StepDirection (ent, ent->ideal_yaw, dist))
// PMM - charging monsters (AI_CHARGING) don't deflect unless they have to
	if ( (((rand()&3)==1) && !(ent->monsterinfo.aiflags & AI_CHARGING)) || !SV_StepDirection (ent, ent->ideal_yaw, dist))
	{
		if (ent->monsterinfo.aiflags & AI_BLOCKED)
		{
//			if ((g_showlogic) && (g_showlogic->value))
//				gi.dprintf ("tesla attack detected, not changing direction!\n");
			ent->monsterinfo.aiflags &= ~AI_BLOCKED;
			return;
		}
		if (ent->inuse)
			SV_NewChaseDir (ent, goal, dist);
	}
}


/*
===============
M_walkmove
===============
*/
qboolean M_walkmove (edict_t *ent, float yaw, float dist)
{
	vec3_t	move;
	// PMM
	qboolean	retval;
	
	if (!ent->groundentity && !(ent->flags & (FL_FLY|FL_SWIM)))
		return false;

	yaw = yaw*M_PI*2 / 360;
	
	move[0] = cos(yaw)*dist;
	move[1] = sin(yaw)*dist;
	move[2] = 0;

	// PMM
	retval = SV_movestep(ent, move, true);
	ent->monsterinfo.aiflags &= ~AI_BLOCKED;
	return retval;
	// pmm
	//return SV_movestep(ent, move, true);
}
