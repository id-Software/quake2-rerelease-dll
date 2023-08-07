// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
	fixbot.c
*/


#include "g_local.h"
#include "m_fixbot.h"


#define MZ2_fixbot_BLASTER_1				MZ2_HOVER_BLASTER_1

qboolean visible (edict_t *self, edict_t *other);
qboolean infront (edict_t *self, edict_t *other);

static int	sound_pain1;
static int	sound_die;
static int  sound_weld1;
static int  sound_weld2;
static int  sound_weld3;

void fixbot_run (edict_t *self);
void fixbot_stand (edict_t *self);
void fixbot_dead (edict_t *self);
void fixbot_attack (edict_t *self);
void fixbot_fire_blaster (edict_t *self);
void fixbot_fire_welder (edict_t *self);
void fixbot_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

void use_scanner (edict_t *self);
void change_to_roam (edict_t *self);
void fly_vertical (edict_t *self);

extern mmove_t fixbot_move_forward;
extern mmove_t fixbot_move_stand;
extern mmove_t fixbot_move_stand2;
extern mmove_t fixbot_move_roamgoal;

extern mmove_t fixbot_move_weld_start;
extern mmove_t fixbot_move_weld;
extern mmove_t fixbot_move_weld_end;
extern mmove_t fixbot_move_takeoff;
extern mmove_t fixbot_move_landing;
extern mmove_t fixbot_move_turn;

extern void roam_goal (edict_t *self);
void ED_CallSpawn (edict_t *ent);

float crand(void)
{
	return (rand()&32767)*(2.0/32767)-1;	
};

edict_t *fixbot_FindDeadMonster (edict_t *self)
{
	edict_t	*ent = NULL;
	edict_t	*best = NULL;

	while ((ent = findradius(ent, self->s.origin, 1024)) != NULL)
	{
		if (ent == self)
			continue;
		if (!(ent->svflags & SVF_MONSTER))
			continue;
		if (ent->monsterinfo.aiflags & AI_GOOD_GUY)
			continue;
		if (ent->owner)
			continue;
		if (ent->health > 0)
			continue;
		if (ent->nextthink)
			continue;
		if (!visible(self, ent))
			continue;
		if (!best)
		{
			best = ent;
			continue;
		}
		if (ent->max_health <= best->max_health)
			continue;
		best = ent;
	}

	return best;
}


int fixbot_search (edict_t *self)
{
	edict_t	*ent;

	if (!self->goalentity)
	{
		ent = fixbot_FindDeadMonster(self);
		if (ent)
		{
			self->oldenemy = self->enemy;
			self->enemy = ent;
			self->enemy->owner = self;
			self->monsterinfo.aiflags |= AI_MEDIC;
			FoundTarget (self);
			return (1);
		}
	}
	return (0);
}

void landing_goal (edict_t *self)
{
	trace_t tr;
	vec3_t forward, right, up;
	vec3_t end;
	edict_t *ent;
	
	ent = G_Spawn ();	
	ent->classname = "bot_goal";
	ent->solid = SOLID_BBOX;
	ent->owner = self;
	gi.linkentity (ent);
	
	VectorSet (ent->mins, -32, -32, -24);
	VectorSet (ent->maxs, 32, 32, 24);
	
	AngleVectors (self->s.angles, forward, right, up);
	VectorMA (self->s.origin, 32, forward, end);
	VectorMA (self->s.origin, -8096, up, end);
		
	tr = gi.trace (self->s.origin, ent->mins, ent->maxs, end, self, MASK_MONSTERSOLID);

	VectorCopy (tr.endpos, ent->s.origin);
	
	self->goalentity = self->enemy = ent;
	self->monsterinfo.currentmove = &fixbot_move_landing;		
	

}


void takeoff_goal (edict_t *self)
{
	trace_t tr;
	vec3_t forward, right, up;
	vec3_t end;
	edict_t *ent;
	
	ent = G_Spawn ();	
	ent->classname = "bot_goal";
	ent->solid = SOLID_BBOX;
	ent->owner = self;
	gi.linkentity (ent);
	
	VectorSet (ent->mins, -32, -32, -24);
	VectorSet (ent->maxs, 32, 32, 24);
	
		
	AngleVectors (self->s.angles, forward, right, up);
	VectorMA (self->s.origin, 32, forward, end);
	VectorMA (self->s.origin, 128, up, end);
		
	tr = gi.trace (self->s.origin, ent->mins, ent->maxs, end, self, MASK_MONSTERSOLID);

	VectorCopy (tr.endpos, ent->s.origin);
	
	self->goalentity = self->enemy = ent;
	self->monsterinfo.currentmove = &fixbot_move_takeoff;		
	

}

void change_to_roam (edict_t *self)
{
	
	if (fixbot_search(self))
		return;

	self->monsterinfo.currentmove = &fixbot_move_roamgoal;

	
	if (self->spawnflags & 16)
	{
		landing_goal (self);
		self->monsterinfo.currentmove = &fixbot_move_landing;
		self->spawnflags &= ~16;
		self->spawnflags = 32;
	}
	if (self->spawnflags & 8) 
	{
		takeoff_goal (self);
		self->monsterinfo.currentmove = &fixbot_move_takeoff;
		self->spawnflags &= ~8;
		self->spawnflags = 32;
	}
	if (self->spawnflags & 4)
	{
		self->monsterinfo.currentmove = &fixbot_move_roamgoal;
		self->spawnflags &= ~4;
		self->spawnflags = 32;
	}
	if (!self->spawnflags)
	{
		self->monsterinfo.currentmove = &fixbot_move_stand2;
	}

}


void roam_goal (edict_t *self)
{

	trace_t		tr;
	vec3_t		forward, right, up;
	vec3_t		end;
	edict_t		*ent;
	vec3_t		dang;
	int			len, oldlen, whichi, i;
	vec3_t		vec;
	vec3_t		whichvec;
		
	ent = G_Spawn ();	
	ent->classname = "bot_goal";
	ent->solid = SOLID_BBOX;
	ent->owner = self;
	gi.linkentity (ent);

	oldlen = 0;
	whichi = 0;
	for (i=0; i<12; i++) 
	{
		
		VectorCopy (self->s.angles, dang);

		if (i < 6)
			dang[YAW] += 30 * i;
		else 
			dang[YAW] -= 30 * (i-6);

		AngleVectors (dang, forward, right, up);
		VectorMA (self->s.origin, 8192, forward, end);
		
		tr = gi.trace (self->s.origin, NULL, NULL, end, self, MASK_SHOT);

		VectorSubtract (self->s.origin, tr.endpos, vec);
		len = VectorNormalize (vec);
		
		if (len > oldlen)
		{
			oldlen=len;
			VectorCopy (tr.endpos, whichvec);
		}
		
	}
	
	VectorCopy (whichvec, ent->s.origin);
	self->goalentity = self->enemy = ent;
	
	self->monsterinfo.currentmove = &fixbot_move_turn;		


}

void use_scanner (edict_t *self)
{
	edict_t *ent = NULL;
	
	float   radius = 1024;
	vec3_t	vec;
	
	int len;
	int oldlen = 0x10000;
	edict_t *tempent = NULL; 


	while ((ent = findradius(ent, self->s.origin, radius)) != NULL)
	{
		if (ent->health >= 100)
		{
			if (strcmp (ent->classname, "object_repair") == 0)
			{
				if (visible(self, ent))
				{	
					// remove the old one
					if (strcmp (self->goalentity->classname, "bot_goal") == 0)
					{
						self->goalentity->nextthink = level.time + 0.1;
						self->goalentity->think = G_FreeEdict;
					}	
					
					self->goalentity = self->enemy = ent;
					
					VectorSubtract (self->s.origin, self->goalentity->s.origin, vec);
					len = VectorNormalize (vec);

					if (len < 32)
					{
						self->monsterinfo.currentmove = &fixbot_move_weld_start;
						return;
					}
				return;
				}
			}
		}
	}

	VectorSubtract (self->s.origin, self->goalentity->s.origin, vec);
	len = VectorLength (vec);

	if (len < 32)
	{
		if (strcmp (self->goalentity->classname, "object_repair") == 0)
		{
			self->monsterinfo.currentmove = &fixbot_move_weld_start;
		}
		else 
		{
			self->goalentity->nextthink = level.time + 0.1;
			self->goalentity->think = G_FreeEdict;
			self->goalentity = self->enemy = NULL;
			self->monsterinfo.currentmove = &fixbot_move_stand;
		}
		return;
	}

	VectorSubtract (self->s.origin, self->s.old_origin, vec);
	len = VectorLength (vec);

	/* 
	  bot is stuck get new goalentity
	*/
	if (len == 0)
	{
		if (strcmp (self->goalentity->classname , "object_repair") == 0)
		{
			self->monsterinfo.currentmove = &fixbot_move_stand;
		}
		else 
		{
			self->goalentity->nextthink = level.time + 0.1;
			self->goalentity->think = G_FreeEdict;
			self->goalentity = self->enemy = NULL;
			self->monsterinfo.currentmove = &fixbot_move_stand;
		}
	}

}


/*
	when the bot has found a landing pad
	it will proceed to its goalentity
	just above the landing pad and
	decend translated along the z the current
	frames are at 10fps
*/ 
void blastoff (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_impact, int hspread, int vspread)
{
	trace_t		tr;
	vec3_t		dir;
	vec3_t		forward, right, up;
	vec3_t		end;
	float		r;
	float		u;
	vec3_t		water_start;
	qboolean	water = false;
	int			content_mask = MASK_SHOT | MASK_WATER;

	hspread+= (self->s.frame - FRAME_takeoff_01);
	vspread+= (self->s.frame - FRAME_takeoff_01);

	tr = gi.trace (self->s.origin, NULL, NULL, start, self, MASK_SHOT);
	if (!(tr.fraction < 1.0))
	{
		vectoangles (aimdir, dir);
		AngleVectors (dir, forward, right, up);

		r = crandom()*hspread;
		u = crandom()*vspread;
		VectorMA (start, 8192, forward, end);
		VectorMA (end, r, right, end);
		VectorMA (end, u, up, end);

		if (gi.pointcontents (start) & MASK_WATER)
		{
			water = true;
			VectorCopy (start, water_start);
			content_mask &= ~MASK_WATER;
		}

		tr = gi.trace (start, NULL, NULL, end, self, content_mask);

		// see if we hit water
		if (tr.contents & MASK_WATER)
		{
			int		color;

			water = true;
			VectorCopy (tr.endpos, water_start);

			if (!VectorCompare (start, tr.endpos))
			{
				if (tr.contents & CONTENTS_WATER)
				{
					if (strcmp(tr.surface->name, "*brwater") == 0)
						color = SPLASH_BROWN_WATER;
					else
						color = SPLASH_BLUE_WATER;
				}
				else if (tr.contents & CONTENTS_SLIME)
					color = SPLASH_SLIME;
				else if (tr.contents & CONTENTS_LAVA)
					color = SPLASH_LAVA;
				else
					color = SPLASH_UNKNOWN;

				if (color != SPLASH_UNKNOWN)
				{
					gi.WriteByte (svc_temp_entity);
					gi.WriteByte (TE_SPLASH);
					gi.WriteByte (8);
					gi.WritePosition (tr.endpos);
					gi.WriteDir (tr.plane.normal);
					gi.WriteByte (color);
					gi.multicast (tr.endpos, MULTICAST_PVS);
				}

				// change bullet's course when it enters water
				VectorSubtract (end, start, dir);
				vectoangles (dir, dir);
				AngleVectors (dir, forward, right, up);
				r = crandom()*hspread*2;
				u = crandom()*vspread*2;
				VectorMA (water_start, 8192, forward, end);
				VectorMA (end, r, right, end);
				VectorMA (end, u, up, end);
			}

			// re-trace ignoring water this time
			tr = gi.trace (water_start, NULL, NULL, end, self, MASK_SHOT);
		}
	}

	// send gun puff / flash
	if (!((tr.surface) && (tr.surface->flags & SURF_SKY)))
	{
		if (tr.fraction < 1.0)
		{
			if (tr.ent->takedamage)
			{
				T_Damage (tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, kick, DAMAGE_BULLET, MOD_BLASTOFF);
			}
			else
			{
				if (strncmp (tr.surface->name, "sky", 3) != 0)
				{
					gi.WriteByte (svc_temp_entity);
					gi.WriteByte (te_impact);
					gi.WritePosition (tr.endpos);
					gi.WriteDir (tr.plane.normal);
					gi.multicast (tr.endpos, MULTICAST_PVS);

					if (self->client)
						PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
				}
			}
		}
	}

	// if went through water, determine where the end and make a bubble trail
	if (water)
	{
		vec3_t	pos;

		VectorSubtract (tr.endpos, water_start, dir);
		VectorNormalize (dir);
		VectorMA (tr.endpos, -2, dir, pos);
		if (gi.pointcontents (pos) & MASK_WATER)
			VectorCopy (pos, tr.endpos);
		else
			tr = gi.trace (pos, NULL, NULL, water_start, tr.ent, MASK_WATER);

		VectorAdd (water_start, tr.endpos, pos);
		VectorScale (pos, 0.5, pos);

		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BUBBLETRAIL);
		gi.WritePosition (water_start);
		gi.WritePosition (tr.endpos);
		gi.multicast (pos, MULTICAST_PVS);
	}
}


void fly_vertical (edict_t *self)
{
	int i;
	vec3_t v;
	vec3_t forward, right, up;
	vec3_t start;
	vec3_t tempvec;
	
	VectorSubtract (self->goalentity->s.origin, self->s.origin, v);
	self->ideal_yaw = vectoyaw(v);	
	M_ChangeYaw (self);
	
	if (self->s.frame == FRAME_landing_58 || self->s.frame == FRAME_takeoff_16)
	{
		self->goalentity->nextthink = level.time + 0.1;
		self->goalentity->think = G_FreeEdict;
		self->monsterinfo.currentmove = &fixbot_move_stand;
		self->goalentity = self->enemy = NULL;
	}

	// kick up some particles
	VectorCopy (self->s.angles, tempvec);
	tempvec[PITCH] += 90;

	AngleVectors (tempvec, forward, right, up);
	VectorCopy (self->s.origin, start);
	
	for (i=0; i< 10; i++)
		blastoff (self, start, forward, 2, 1, TE_SHOTGUN, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD);

	// needs sound
}


void fly_vertical2 (edict_t *self)
{
	vec3_t v;
	int len;
	
	VectorSubtract (self->goalentity->s.origin, self->s.origin, v);
	len = VectorLength (v);
	self->ideal_yaw = vectoyaw(v);	
	M_ChangeYaw (self);
	
	if (len < 32)
	{
		self->goalentity->nextthink = level.time + 0.1;
		self->goalentity->think = G_FreeEdict;
		self->monsterinfo.currentmove = &fixbot_move_stand;
		self->goalentity = self->enemy = NULL;
	}
		
	// needs sound
}

mframe_t fixbot_frames_landing [] =
{
	ai_move, 0,  NULL,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,

	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,

	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,

	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,

	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,

	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2,
	ai_move, 0, fly_vertical2
};
mmove_t fixbot_move_landing = { FRAME_landing_01, FRAME_landing_58, fixbot_frames_landing, NULL };

/*
	generic ambient stand
*/
mframe_t fixbot_frames_stand [] =
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
	ai_move, 0, NULL,

	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, change_to_roam
	
};
mmove_t	fixbot_move_stand = {FRAME_ambient_01, FRAME_ambient_19, fixbot_frames_stand, NULL};

mframe_t fixbot_frames_stand2 [] =
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
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL
	
};
mmove_t	fixbot_move_stand2 = {FRAME_ambient_01, FRAME_ambient_19, fixbot_frames_stand2, NULL};


/*
	will need the pickup offset for the front pincers
	object will need to stop forward of the object 
	and take the object with it ( this may require a variant of liftoff and landing )
*/
mframe_t fixbot_frames_pickup [] =
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
	ai_move, 0, NULL,

	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,

	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
	
};
mmove_t fixbot_move_pickup = { FRAME_pickup_01, FRAME_pickup_27, fixbot_frames_pickup, NULL};

/*
	generic frame to move bot
*/
mframe_t fixbot_frames_roamgoal [] =
{
	ai_move, 0, roam_goal
};
mmove_t fixbot_move_roamgoal = {FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_roamgoal, NULL};


void ai_facing (edict_t *self, float dist)
{
	vec3_t v;

	if (infront (self, self->goalentity))
		self->monsterinfo.currentmove = &fixbot_move_forward;
	else
	{
		VectorSubtract (self->goalentity->s.origin, self->s.origin, v);
		self->ideal_yaw = vectoyaw(v);	
		M_ChangeYaw (self);
	}

};

mframe_t fixbot_frames_turn [] =
{
	ai_facing,	0,	NULL
};
mmove_t fixbot_move_turn = {FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_turn, NULL};


void go_roam (edict_t *self)
{
	self->monsterinfo.currentmove = &fixbot_move_stand;	
}


/*
	takeoff
*/
mframe_t fixbot_frames_takeoff [] =
{
	ai_move,	0.01,	fly_vertical,
	ai_move,	0.01,	fly_vertical,
	ai_move,	0.01,	fly_vertical,
	ai_move,	0.01,	fly_vertical,
	ai_move,	0.01,	fly_vertical,
	ai_move,	0.01,	fly_vertical,
	ai_move,	0.01,	fly_vertical,
	ai_move,	0.01,	fly_vertical,
	ai_move,	0.01,	fly_vertical,
	ai_move,	0.01,	fly_vertical,

	ai_move,	0.01,	fly_vertical,
	ai_move,	0.01,	fly_vertical,
	ai_move,	0.01,	fly_vertical,
	ai_move,	0.01,	fly_vertical,
	ai_move,	0.01,	fly_vertical,
	ai_move,	0.01,	fly_vertical
};
mmove_t fixbot_move_takeoff = {FRAME_takeoff_01, FRAME_takeoff_16, fixbot_frames_takeoff, NULL};


/* findout what this is */
mframe_t fixbot_frames_paina [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t fixbot_move_paina = {FRAME_paina_01, FRAME_paina_06, fixbot_frames_paina, fixbot_run};

/* findout what this is */
mframe_t fixbot_frames_painb [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t fixbot_move_painb = {FRAME_painb_01, FRAME_painb_08, fixbot_frames_painb, fixbot_run};

/*
	backup from pain
	call a generic painsound
	some spark effects
*/
mframe_t fixbot_frames_pain3 [] =
{
	ai_move,	-1,	NULL
};
mmove_t fixbot_move_pain3 = {FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_pain3, fixbot_run};

/*
	bot has compleated landing
	and is now on the grownd
	( may need second land if the bot is releasing jib into jib vat )
*/
mframe_t fixbot_frames_land [] =
{
	ai_move,	0,	NULL
};
mmove_t fixbot_move_land = {FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_land, NULL};


void M_MoveToGoal (edict_t *ent, float dist);

void ai_movetogoal (edict_t *self, float dist)
{
	M_MoveToGoal (self, dist);
}
/*
	
*/
mframe_t fixbot_frames_forward [] =
{
	ai_movetogoal,	5,	use_scanner
};
mmove_t fixbot_move_forward = {FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_forward, NULL};


/*
	
*/
mframe_t fixbot_frames_walk [] =
{
	ai_walk,	5,	NULL
};
mmove_t fixbot_move_walk = {FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_walk, NULL};


/*
	
*/
mframe_t fixbot_frames_run [] =
{
	ai_run,	10,	NULL
};
mmove_t fixbot_move_run = {FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_run, NULL};

/*
	raf
	note to self
	they could have a timer that will cause
	the bot to explode on countdown
*/
mframe_t fixbot_frames_death1 [] =
{
	ai_move,	0,	NULL
};
mmove_t fixbot_move_death1 = {FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_death1, fixbot_dead};

//
mframe_t fixbot_frames_backward [] =
{
	ai_move,	0,	NULL
};
mmove_t fixbot_move_backward = {FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_backward, NULL};

//
mframe_t fixbot_frames_start_attack [] =
{
	ai_charge,	0,	NULL
};
mmove_t fixbot_move_start_attack = {FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_start_attack, fixbot_attack};

/*
	TBD: 
	need to get laser attack anim
	attack with the laser blast
*/
mframe_t fixbot_frames_attack1 [] =
{
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	-10,  fixbot_fire_blaster
};
mmove_t fixbot_move_attack1 = {FRAME_shoot_01, FRAME_shoot_06, fixbot_frames_attack1, NULL};


int check_telefrag (edict_t *self)
{
	vec3_t	start = { 0, 0, 0 };
	vec3_t	forward, right, up;
	trace_t	tr;

	AngleVectors (self->enemy->s.angles, forward, right, up);
	VectorMA (start, 48, up, start);
	tr = gi.trace (self->enemy->s.origin, self->enemy->mins, self->enemy->maxs, start, self, MASK_MONSTERSOLID);
	if (tr.ent->takedamage)
	{
		tr.ent->health = -1000;
		return (0);
	}
			
	return (1);
}

void fixbot_fire_laser (edict_t *self)
{
 
	vec3_t forward, right, up;
	vec3_t tempang, start;
	vec3_t	dir, angles, end;
	edict_t *ent;

	// critter dun got blown up while bein' fixed
	if (self->enemy->health <= self->enemy->gib_health)
	{
		self->monsterinfo.currentmove = &fixbot_move_stand;
		self->monsterinfo.aiflags &= ~AI_MEDIC;
		return;
	}

	gi.sound(self, CHAN_AUTO, gi.soundindex("misc/lasfly.wav"), 1, ATTN_STATIC, 0);

	VectorCopy (self->s.origin, start);
	VectorCopy (self->enemy->s.origin, end);
	VectorSubtract (end, start, dir);
	vectoangles (dir, angles);
	
	ent = G_Spawn ();
	VectorCopy (self->s.origin, ent->s.origin);
	VectorCopy (angles, tempang);
	AngleVectors (tempang, forward, right, up);
	VectorCopy (tempang, ent->s.angles);
	VectorCopy (ent->s.origin, start);
	
	VectorMA (start, 16, forward, start);
	
	VectorCopy (start, ent->s.origin);
	ent->enemy = self->enemy;
	ent->owner = self;
	ent->dmg = -1;
	monster_dabeam (ent);

	if (self->enemy->health > (self->enemy->mass/10))
	{
		// sorry guys but had to fix the problem this way
		// if it doesn't do this then two creatures can share the same space
		// and its real bad.
		if (check_telefrag (self)) 
		{
			self->enemy->spawnflags = 0;
			self->enemy->monsterinfo.aiflags = 0;
			self->enemy->target = NULL;
			self->enemy->targetname = NULL;
			self->enemy->combattarget = NULL;
			self->enemy->deathtarget = NULL;
			self->enemy->owner = self;
			ED_CallSpawn (self->enemy);
			self->enemy->owner = NULL;
			self->s.origin[2] += 1;
		
			self->enemy->monsterinfo.aiflags &= ~AI_RESURRECTING;

			self->monsterinfo.currentmove = &fixbot_move_stand;
			self->monsterinfo.aiflags &= ~AI_MEDIC;

		}
	}
	else
		self->enemy->monsterinfo.aiflags |= AI_RESURRECTING;
}

mframe_t fixbot_frames_laserattack [] =
{
	ai_charge,	0,	fixbot_fire_laser,
	ai_charge,	0,	fixbot_fire_laser,
	ai_charge,	0,	fixbot_fire_laser,
	ai_charge,	0,	fixbot_fire_laser,
	ai_charge,	0,	fixbot_fire_laser,
	ai_charge,	0,  fixbot_fire_laser
};
mmove_t fixbot_move_laserattack = {FRAME_shoot_01, FRAME_shoot_06, fixbot_frames_laserattack, NULL};


/*
	need to get forward translation data
	for the charge attack
*/
mframe_t fixbot_frames_attack2 [] =
{
	ai_charge,	0,	NULL,
	ai_charge,  0,  NULL,
	ai_charge,	0,	NULL,
	ai_charge,  0,  NULL,
	ai_charge,	0,	NULL,
	ai_charge,  0,  NULL,
	ai_charge,	0,	NULL,
	ai_charge,  0,  NULL,
	ai_charge,	0,	NULL,
	ai_charge,  0,  NULL,

	ai_charge,	-10, NULL,
	ai_charge,  -10, NULL,
	ai_charge,	-10, NULL,
	ai_charge,  -10, NULL,
	ai_charge,	-10, NULL,
	ai_charge,  -10, NULL,
	ai_charge,	-10, NULL,
	ai_charge,  -10, NULL,
	ai_charge,	-10, NULL,
	ai_charge,  -10, NULL,

	ai_charge,	0,	fixbot_fire_blaster,
	ai_charge,  0,  NULL,
	ai_charge,	0,	NULL,
	ai_charge,  0,  NULL,
	ai_charge,	0,	NULL,
	ai_charge,  0,  NULL,
	ai_charge,	0,	NULL,
	ai_charge,  0,  NULL,
	ai_charge,  0,  NULL,
	ai_charge,  0,  NULL,

	ai_charge,  0,  NULL
};
mmove_t fixbot_move_attack2 = {FRAME_charging_01, FRAME_charging_31, fixbot_frames_attack2, fixbot_run};

void weldstate (edict_t *self)
{
	if (self->s.frame == FRAME_weldstart_10)
		self->monsterinfo.currentmove = &fixbot_move_weld;
	else if (self->s.frame == FRAME_weldmiddle_07)
	{
		if (self->goalentity->health < 0) 
		{
			self->enemy->owner = NULL;
			self->monsterinfo.currentmove = &fixbot_move_weld_end;
		}
		else
			self->goalentity->health-=10;
	}
	else
	{
		self->goalentity = self->enemy = NULL;
		self->monsterinfo.currentmove = &fixbot_move_stand;
	}
}

void ai_move2 (edict_t *self, float dist)
{
	vec3_t v;
	
	if (dist)
		M_walkmove (self, self->s.angles[YAW], dist);

	VectorSubtract (self->goalentity->s.origin, self->s.origin, v);
	self->ideal_yaw = vectoyaw(v);	
	M_ChangeYaw (self);
};

mframe_t fixbot_frames_weld_start [] = 
{
	ai_move2, 0, NULL,
	ai_move2, 0, NULL,
	ai_move2, 0, NULL,
	ai_move2, 0, NULL,
	ai_move2, 0, NULL,
	ai_move2, 0, NULL,
	ai_move2, 0, NULL,
	ai_move2, 0, NULL,
	ai_move2, 0, NULL,
	ai_move2, 0, weldstate
};
mmove_t fixbot_move_weld_start = {FRAME_weldstart_01, FRAME_weldstart_10, fixbot_frames_weld_start, NULL};

mframe_t fixbot_frames_weld [] =
{
	ai_move2, 0, fixbot_fire_welder,
	ai_move2, 0, fixbot_fire_welder,
	ai_move2, 0, fixbot_fire_welder,
	ai_move2, 0, fixbot_fire_welder,
	ai_move2, 0, fixbot_fire_welder,
	ai_move2, 0, fixbot_fire_welder,
	ai_move2, 0, weldstate
};
mmove_t fixbot_move_weld = {FRAME_weldmiddle_01, FRAME_weldmiddle_07, fixbot_frames_weld, NULL};

mframe_t fixbot_frames_weld_end [] =
{
	ai_move2, -2, NULL,
	ai_move2, -2, NULL,
	ai_move2, -2, NULL,
	ai_move2, -2, NULL,
	ai_move2, -2, NULL,
	ai_move2, -2, NULL,
	ai_move2, -2, weldstate
};
mmove_t fixbot_move_weld_end = {FRAME_weldend_01, FRAME_weldend_07, fixbot_frames_weld_end, NULL};


void fixbot_fire_welder (edict_t *self)
{
	vec3_t	start;
	vec3_t	forward, right, up;
	vec3_t	end;
	vec3_t	dir;
	int		count = 2;
	vec3_t  vec;
	float	r;
	
	if (!self->enemy)
		return;


	vec[0]=24.0;
	vec[1]=-0.8;
	vec[2]=-10.0;

	AngleVectors (self->s.angles, forward, right, up);
	G_ProjectSource (self->s.origin, vec, forward, right, start);

	VectorCopy (self->enemy->s.origin, end);
	
	VectorSubtract (end, start, dir);

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_WELDING_SPARKS);
	gi.WriteByte (10);
	gi.WritePosition (start);
	gi.WriteDir (vec3_origin);
	gi.WriteByte (0xe0 + (rand()&7));
	gi.multicast (self->s.origin, MULTICAST_PVS);


	if (random() > 0.8)
	{
		r = random();

		if (r < 0.33)
			gi.sound (self, CHAN_VOICE, sound_weld1, 1, ATTN_IDLE, 0);
		else if (r < 0.66)
			gi.sound (self, CHAN_VOICE, sound_weld2, 1, ATTN_IDLE, 0);
		else
			gi.sound (self, CHAN_VOICE, sound_weld3, 1, ATTN_IDLE, 0);
	}
}

void fixbot_fire_blaster (edict_t *self)
{
	vec3_t	start;
	vec3_t	forward, right, up;
	vec3_t	end;
	vec3_t	dir;

	if (!visible(self, self->enemy))
	{
		self->monsterinfo.currentmove = &fixbot_move_run;	
	}
	
	AngleVectors (self->s.angles, forward, right, up);
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_fixbot_BLASTER_1], forward, right, start);

	VectorCopy (self->enemy->s.origin, end);
	end[2] += self->enemy->viewheight;
	VectorSubtract (end, start, dir);

	monster_fire_blaster (self, start, dir, 15, 1000, MZ2_fixbot_BLASTER_1, EF_BLASTER);
	
}

void fixbot_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &fixbot_move_stand;
}

void fixbot_run (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &fixbot_move_stand;
	else
		self->monsterinfo.currentmove = &fixbot_move_run;
}

void fixbot_walk (edict_t *self)
{
	vec3_t	vec;
	int		len;

	if (strcmp (self->goalentity->classname, "object_repair") == 0)
	{
		VectorSubtract (self->s.origin, self->goalentity->s.origin, vec);
		len = VectorLength (vec);
		if (len < 32)
		{
			self->monsterinfo.currentmove = &fixbot_move_weld_start;
			return;
		}
	}
	self->monsterinfo.currentmove = &fixbot_move_walk;
}

void fixbot_start_attack (edict_t *self)
{
	self->monsterinfo.currentmove = &fixbot_move_start_attack;
}

void fixbot_attack(edict_t *self)
{
	vec3_t	vec;
	int		len;
	
	if (self->monsterinfo.aiflags & AI_MEDIC)
	{
		if (!visible (self, self->goalentity))
			return;
		VectorSubtract (self->s.origin, self->enemy->s.origin, vec);
		len = VectorLength(vec);
		if (len > 128)
			return;
		else
			self->monsterinfo.currentmove = &fixbot_move_laserattack;
	}
	else
		self->monsterinfo.currentmove = &fixbot_move_attack2;
	
}


void fixbot_pain (edict_t *self, edict_t *other, float kick, int damage)
{
//	if (self->health < (self->max_health / 2))
//		self->s.skinnum = 1;
	// gi.dprintf("pain\n");

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;
	gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);

	if (damage <= 10)
		self->monsterinfo.currentmove = &fixbot_move_pain3;
	else if (damage <= 25)
		self->monsterinfo.currentmove = &fixbot_move_painb;
	else
		self->monsterinfo.currentmove = &fixbot_move_paina;
}

void fixbot_dead (edict_t *self)
{
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity (self);
}

void fixbot_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	gi.sound (self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	BecomeExplosion1(self);

	// shards
	
}

/*QUAKED monster_fixbot (1 .5 0) (-32 -32 -24) (32 32 24) Ambush Trigger_Spawn Fixit Takeoff Landing
*/
void SP_monster_fixbot (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	sound_pain1 = gi.soundindex ("flyer/flypain1.wav");
	sound_die = gi.soundindex ("flyer/flydeth1.wav");

	sound_weld1 = gi.soundindex ("misc/welder1.wav");
	sound_weld2 = gi.soundindex ("misc/welder2.wav");
	sound_weld3 = gi.soundindex ("misc/welder3.wav");

	self->s.modelindex = gi.modelindex ("models/monsters/fixbot/tris.md2");
	
	VectorSet (self->mins, -32, -32, -24);
	VectorSet (self->maxs, 32, 32, 24);
	
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->health = 150;
	self->mass = 150;

	self->pain = fixbot_pain;
	self->die = fixbot_die;

	self->monsterinfo.stand = fixbot_stand;
	self->monsterinfo.walk = fixbot_walk;
	self->monsterinfo.run = fixbot_run;
	self->monsterinfo.attack = fixbot_attack;
	
	gi.linkentity (self);

	self->monsterinfo.currentmove = &fixbot_move_stand;	
	self->monsterinfo.scale = MODEL_SCALE;
			
	flymonster_start (self);
				
}


