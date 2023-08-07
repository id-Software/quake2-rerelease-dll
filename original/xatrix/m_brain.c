// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

brain

==============================================================================
*/

#include "g_local.h"
#include "m_brain.h"


static int	sound_chest_open;
static int	sound_tentacles_extend;
static int	sound_tentacles_retract;
static int	sound_death;
static int	sound_idle1;
static int	sound_idle2;
static int	sound_idle3;
static int	sound_pain1;
static int	sound_pain2;
static int	sound_sight;
static int	sound_search;
static int	sound_melee1;
static int	sound_melee2;
static int	sound_melee3;


void brain_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void brain_search (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}


void brain_run (edict_t *self);
void brain_dead (edict_t *self);


//
// STAND
//

mframe_t brain_frames_stand [] =
{
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,

	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,

	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL
};
mmove_t brain_move_stand = {FRAME_stand01, FRAME_stand30, brain_frames_stand, NULL};

void brain_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &brain_move_stand;
}


//
// IDLE
//

mframe_t brain_frames_idle [] =
{
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,

	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,

	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL,
	ai_stand,	0,	NULL
};
mmove_t brain_move_idle = {FRAME_stand31, FRAME_stand60, brain_frames_idle, brain_stand};

void brain_idle (edict_t *self)
{
	gi.sound (self, CHAN_AUTO, sound_idle3, 1, ATTN_IDLE, 0);
	self->monsterinfo.currentmove = &brain_move_idle;
}


//
// WALK
//
mframe_t brain_frames_walk1 [] =
{
	ai_walk,	7,	NULL,
	ai_walk,	2,	NULL,
	ai_walk,	3,	NULL,
	ai_walk,	3,	NULL,
	ai_walk,	1,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	9,	NULL,
	ai_walk,	-4,	NULL,
	ai_walk,	-1,	NULL,
	ai_walk,	2,	NULL
};
mmove_t brain_move_walk1 = {FRAME_walk101, FRAME_walk111, brain_frames_walk1, NULL};

// walk2 is FUBAR, do not use
#if 0
void brain_walk2_cycle (edict_t *self)
{
	if (random() > 0.1)
		self->monsterinfo.nextframe = FRAME_walk220;
}

mframe_t brain_frames_walk2 [] =
{
	ai_walk,	3,	NULL,
	ai_walk,	-2,	NULL,
	ai_walk,	-4,	NULL,
	ai_walk,	-3,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	1,	NULL,
	ai_walk,	12,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	-3,	NULL,
	ai_walk,	0,	NULL,

	ai_walk,	-2,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	1,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	10,	NULL,		// Cycle Start

	ai_walk,	-1,	NULL,
	ai_walk,	7,	NULL,
	ai_walk,	0,	NULL,
	ai_walk,	3,	NULL,
	ai_walk,	-3,	NULL,
	ai_walk,	2,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	-3,	NULL,
	ai_walk,	2,	NULL,
	ai_walk,	0,	NULL,

	ai_walk,	4,	brain_walk2_cycle,
	ai_walk,	-1,	NULL,
	ai_walk,	-1,	NULL,
	ai_walk,	-8,	NULL,		
	ai_walk,	0,	NULL,
	ai_walk,	1,	NULL,
	ai_walk,	5,	NULL,
	ai_walk,	2,	NULL,
	ai_walk,	-1,	NULL,
	ai_walk,	-5,	NULL
};
mmove_t brain_move_walk2 = {FRAME_walk201, FRAME_walk240, brain_frames_walk2, NULL};
#endif

void brain_walk (edict_t *self)
{
//	if (random() <= 0.5)
		self->monsterinfo.currentmove = &brain_move_walk1;
//	else
//		self->monsterinfo.currentmove = &brain_move_walk2;
}



mframe_t brain_frames_defense [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL
};
mmove_t brain_move_defense = {FRAME_defens01, FRAME_defens08, brain_frames_defense, NULL};

mframe_t brain_frames_pain3 [] =
{
	ai_move,	-2,	NULL,
	ai_move,	2,	NULL,
	ai_move,	1,	NULL,
	ai_move,	3,	NULL,
	ai_move,	0,	NULL,
	ai_move,	-4,	NULL
};
mmove_t brain_move_pain3 = {FRAME_pain301, FRAME_pain306, brain_frames_pain3, brain_run};

mframe_t brain_frames_pain2 [] =
{
	ai_move,	-2,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	3,	NULL,
	ai_move,	1,	NULL,
	ai_move,	-2,	NULL
};
mmove_t brain_move_pain2 = {FRAME_pain201, FRAME_pain208, brain_frames_pain2, brain_run};

mframe_t brain_frames_pain1 [] =
{
	ai_move,	-6,	NULL,
	ai_move,	-2,	NULL,
	ai_move,	-6,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	2,	NULL,
	ai_move,	0,	NULL,
	ai_move,	2,	NULL,
	ai_move,	1,	NULL,
	ai_move,	7,	NULL,
	ai_move,	0,	NULL,
	ai_move,	3,	NULL,
	ai_move,	-1,	NULL
};
mmove_t brain_move_pain1 = {FRAME_pain101, FRAME_pain121, brain_frames_pain1, brain_run};


//
// DUCK
//

void brain_duck_down (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_DUCKED)
		return;
	self->monsterinfo.aiflags |= AI_DUCKED;
	self->maxs[2] -= 32;
	self->takedamage = DAMAGE_YES;
	gi.linkentity (self);
}

void brain_duck_hold (edict_t *self)
{
	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

void brain_duck_up (edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] += 32;
	self->takedamage = DAMAGE_AIM;
	gi.linkentity (self);
}

mframe_t brain_frames_duck [] =
{
	ai_move,	0,	NULL,
	ai_move,	-2,	brain_duck_down,
	ai_move,	17,	brain_duck_hold,
	ai_move,	-3,	NULL,
	ai_move,	-1,	brain_duck_up,
	ai_move,	-5,	NULL,
	ai_move,	-6,	NULL,
	ai_move,	-6,	NULL
};
mmove_t brain_move_duck = {FRAME_duck01, FRAME_duck08, brain_frames_duck, brain_run};

void brain_dodge (edict_t *self, edict_t *attacker, float eta)
{
	if (random() > 0.25)
		return;

	if (!self->enemy)
		self->enemy = attacker;

	self->monsterinfo.pausetime = level.time + eta + 0.5;
	self->monsterinfo.currentmove = &brain_move_duck;
}


mframe_t brain_frames_death2 [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	9,	NULL,
	ai_move,	0,	NULL
};
mmove_t brain_move_death2 = {FRAME_death201, FRAME_death205, brain_frames_death2, brain_dead};

mframe_t brain_frames_death1 [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	-2,	NULL,
	ai_move,	9,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL
};
mmove_t brain_move_death1 = {FRAME_death101, FRAME_death118, brain_frames_death1, brain_dead};


//
// MELEE
//

void brain_swing_right (edict_t *self)
{
	gi.sound (self, CHAN_BODY, sound_melee1, 1, ATTN_NORM, 0);
}

void brain_hit_right (edict_t *self)
{
	vec3_t	aim;

	VectorSet (aim, MELEE_DISTANCE, self->maxs[0], 8);
	if (fire_hit (self, aim, (15 + (rand() %5)), 40))
		gi.sound (self, CHAN_WEAPON, sound_melee3, 1, ATTN_NORM, 0);
}

void brain_swing_left (edict_t *self)
{
	gi.sound (self, CHAN_BODY, sound_melee2, 1, ATTN_NORM, 0);
}

void brain_hit_left (edict_t *self)
{
	vec3_t	aim;

	VectorSet (aim, MELEE_DISTANCE, self->mins[0], 8);
	if (fire_hit (self, aim, (15 + (rand() %5)), 40))
		gi.sound (self, CHAN_WEAPON, sound_melee3, 1, ATTN_NORM, 0);
}

mframe_t brain_frames_attack1 [] =
{
	ai_charge,	8,	NULL,
	ai_charge,	3,	NULL,
	ai_charge,	5,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	-3,	brain_swing_right,
	ai_charge,	0,	NULL,
	ai_charge,	-5,	NULL,
	ai_charge,	-7,	brain_hit_right,
	ai_charge,	0,	NULL,
	ai_charge,	6,	brain_swing_left,
	ai_charge,	1,	NULL,
	ai_charge,	2,	brain_hit_left,
	ai_charge,	-3,	NULL,
	ai_charge,	6,	NULL,
	ai_charge,	-1,	NULL,
	ai_charge,	-3,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	-11,NULL
};
mmove_t brain_move_attack1 = {FRAME_attak101, FRAME_attak118, brain_frames_attack1, brain_run};

void brain_chest_open (edict_t *self)
{
	self->spawnflags &= ~65536;
	self->monsterinfo.power_armor_type = POWER_ARMOR_NONE;
	gi.sound (self, CHAN_BODY, sound_chest_open, 1, ATTN_NORM, 0);
}

void brain_tentacle_attack (edict_t *self)
{
	vec3_t	aim;

	VectorSet (aim, MELEE_DISTANCE, 0, 8);
	if (fire_hit (self, aim, (10 + (rand() %5)), -600) && skill->value > 0)
		self->spawnflags |= 65536;
	gi.sound (self, CHAN_WEAPON, sound_tentacles_retract, 1, ATTN_NORM, 0);
}

void brain_chest_closed (edict_t *self)
{
	self->monsterinfo.power_armor_type = POWER_ARMOR_SCREEN;
	if (self->spawnflags & 65536)
	{
		self->spawnflags &= ~65536;
		self->monsterinfo.currentmove = &brain_move_attack1;
	}
}

mframe_t brain_frames_attack2 [] =
{
	ai_charge,	5,	NULL,
	ai_charge,	-4,	NULL,
	ai_charge,	-4,	NULL,
	ai_charge,	-3,	NULL,
	ai_charge,	0,	brain_chest_open,
	ai_charge,	0,	NULL,
	ai_charge,	13,	brain_tentacle_attack,
	ai_charge,	0,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	-9,	brain_chest_closed,
	ai_charge,	0,	NULL,
	ai_charge,	4,	NULL,
	ai_charge,	3,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	-3,	NULL,
	ai_charge,	-6,	NULL
};
mmove_t brain_move_attack2 = {FRAME_attak201, FRAME_attak217, brain_frames_attack2, brain_run};

void brain_melee(edict_t *self)
{
	if (random() <= 0.5)
		self->monsterinfo.currentmove = &brain_move_attack1;
	else
		self->monsterinfo.currentmove = &brain_move_attack2;
}

static qboolean brain_tounge_attack_ok (vec3_t start, vec3_t end)
{
	vec3_t	dir, angles;

	// check for max distance
	VectorSubtract (start, end, dir);
	if (VectorLength(dir) > 512)
		return false;

	// check for min/max pitch
	vectoangles (dir, angles);
	if (angles[0] < -180)
		angles[0] += 360;
	if (fabs(angles[0]) > 30)
		return false;

	return true;
}

void brain_tounge_attack (edict_t *self)
{
	vec3_t	offset, start, f, r, end, dir;
	trace_t	tr;
	int damage;

	AngleVectors (self->s.angles, f, r, NULL);
	// VectorSet (offset, 24, 0, 6);
	VectorSet (offset, 24, 0, 16);
	G_ProjectSource (self->s.origin, offset, f, r, start);

	VectorCopy (self->enemy->s.origin, end);
	if (!brain_tounge_attack_ok(start, end))
	{
		end[2] = self->enemy->s.origin[2] + self->enemy->maxs[2] - 8;
		if (!brain_tounge_attack_ok(start, end))
		{
			end[2] = self->enemy->s.origin[2] + self->enemy->mins[2] + 8;
			if (!brain_tounge_attack_ok(start, end))
				return;
		}
	}
	VectorCopy (self->enemy->s.origin, end);

	tr = gi.trace (start, NULL, NULL, end, self, MASK_SHOT);
	if (tr.ent != self->enemy)
		return;

	damage = 5;
	gi.sound (self, CHAN_WEAPON, sound_tentacles_retract, 1, ATTN_NORM, 0);

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_PARASITE_ATTACK);
	gi.WriteShort (self - g_edicts);
	gi.WritePosition (start);
	gi.WritePosition (end);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	VectorSubtract (start, end, dir);
	T_Damage (self->enemy, self, self, dir, self->enemy->s.origin, vec3_origin, damage, 0, DAMAGE_NO_KNOCKBACK, MOD_BRAINTENTACLE);

	// pull the enemy in
	{
		vec3_t	forward;
		self->s.origin[2] += 1;
		AngleVectors (self->s.angles, forward, NULL, NULL);
		VectorScale (forward, -1200, self->enemy->velocity);
	}
	
}

// Brian right eye center

struct r_eyeball 
{
	float x;
	float y;
	float z;
} brain_reye [11] = {
		{  0.746700, 0.238370, 34.167690 },
        {  -1.076390, 0.238370, 33.386372 },
        {  -1.335500, 5.334300, 32.177170 },
        {  -0.175360, 8.846370, 30.635479 },
        {  -2.757590, 7.804610, 30.150860 },
        {  -5.575090, 5.152840, 30.056160 },
        {  -7.017550, 3.262470, 30.552521 },
        {  -7.915740, 0.638800, 33.176189 },
        {  -3.915390, 8.285730, 33.976349 },
        {  -0.913540, 10.933030, 34.141811 },
        {  -0.369900, 8.923900, 34.189079 }
};



// Brain left eye center
struct l_eyeball
{
	float x;
	float y;
	float z;
} brain_leye [11] = {
        {  -3.364710, 0.327750, 33.938381 },
        {  -5.140450, 0.493480, 32.659851 },
        {  -5.341980, 5.646980, 31.277901 },
        {  -4.134480, 9.277440, 29.925621 },
        {  -6.598340, 6.815090, 29.322620 },
        {  -8.610840, 2.529650, 29.251591 },
        {  -9.231360, 0.093280, 29.747959 },
        {  -11.004110, 1.936930, 32.395260 },
        {  -7.878310, 7.648190, 33.148151 },
        {  -4.947370, 11.430050, 33.313610 },
        {  -4.332820, 9.444570, 33.526340 }
};

// note to self
// need to get an x,y,z offset for
// each frame of the run cycle
void brain_laserbeam (edict_t *self)
{
 
	vec3_t forward, right, up;
	vec3_t tempang, start;
	vec3_t	dir, angles, end;
	edict_t *ent;

	// RAFAEL
	// cant call sound this frequent
	if (random() > 0.8)
		gi.sound(self, CHAN_AUTO, gi.soundindex("misc/lasfly.wav"), 1, ATTN_STATIC, 0);

	// check for max distance
	
	VectorCopy (self->s.origin, start);
	VectorCopy (self->enemy->s.origin, end);
	VectorSubtract (end, start, dir);
	vectoangles (dir, angles);
	
	// dis is my right eye
	ent = G_Spawn ();
	VectorCopy (self->s.origin, ent->s.origin);
	VectorCopy (angles, tempang);
	AngleVectors (tempang, forward, right, up);
	VectorCopy (tempang, ent->s.angles);
	VectorCopy (ent->s.origin, start);
	VectorMA (start, brain_reye[self->s.frame - FRAME_walk101].x, right, start);
	VectorMA (start, brain_reye[self->s.frame - FRAME_walk101].y, forward, start);
	VectorMA (start, brain_reye[self->s.frame - FRAME_walk101].z, up, start);
	VectorCopy (start, ent->s.origin);
	ent->enemy = self->enemy;
	ent->owner = self;
	ent->dmg = 1;
	monster_dabeam (ent);
   
	// dis is me left eye
	ent = G_Spawn ();  
	VectorCopy (self->s.origin, ent->s.origin);
	VectorCopy (angles, tempang);
	AngleVectors (tempang, forward, right, up);
	VectorCopy (tempang, ent->s.angles);
	VectorCopy (ent->s.origin, start);
	VectorMA (start, brain_leye[self->s.frame - FRAME_walk101].x, right, start);
	VectorMA (start, brain_leye[self->s.frame - FRAME_walk101].y, forward, start);
	VectorMA (start, brain_leye[self->s.frame - FRAME_walk101].z, up, start);
	VectorCopy (start, ent->s.origin);
	ent->enemy = self->enemy;
	ent->owner = self;
	ent->dmg = 1;
	monster_dabeam (ent);
	
}

void brain_laserbeam_reattack (edict_t *self)
{
	if (random() < 0.5)
		if (visible (self, self->enemy))
			if (self->enemy->health > 0)
				self->s.frame = FRAME_walk101;
}


mframe_t brain_frames_attack3 [] =
{
	ai_charge,	5,	NULL,
	ai_charge,	-4,	NULL,
	ai_charge,	-4,	NULL,
	ai_charge,	-3,	NULL,
	ai_charge,	0,	brain_chest_open,
	ai_charge,	0,	brain_tounge_attack,
	ai_charge,	13,	NULL,
	ai_charge,	0,	brain_tentacle_attack,
	ai_charge,	2,	NULL,
	ai_charge,	0,	brain_tounge_attack,
	ai_charge,	-9,	brain_chest_closed,
	ai_charge,	0,	NULL,
	ai_charge,	4,	NULL,
	ai_charge,	3,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	-3,	NULL,
	ai_charge,	-6,	NULL
};
mmove_t brain_move_attack3 = {FRAME_attak201, FRAME_attak217, brain_frames_attack3, brain_run};

mframe_t brain_frames_attack4 [] =
{
	ai_charge,	9,	brain_laserbeam,
	ai_charge,	2,	brain_laserbeam,
	ai_charge,	3,	brain_laserbeam,
	ai_charge,	3,	brain_laserbeam,
	ai_charge,	1,	brain_laserbeam,
	ai_charge,	0,	brain_laserbeam,
	ai_charge,	0,	brain_laserbeam,
	ai_charge,	10,	brain_laserbeam,
	ai_charge,	-4,	brain_laserbeam,
	ai_charge,	-1,	brain_laserbeam,
	ai_charge,	2,	brain_laserbeam_reattack
};
mmove_t brain_move_attack4 = {FRAME_walk101, FRAME_walk111, brain_frames_attack4, brain_run};


// RAFAEL
void brain_attack (edict_t *self)
{
	int r;
	
	if (random() < 0.8)
	{
		r = range (self, self->enemy);
		if (r == RANGE_NEAR)
		{
			if (random() < 0.5)
				self->monsterinfo.currentmove = &brain_move_attack3;
			else 
				self->monsterinfo.currentmove = &brain_move_attack4;
		}
		else if (r > RANGE_NEAR)
			self->monsterinfo.currentmove = &brain_move_attack4;
	}
	
}

//
// RUN
//

mframe_t brain_frames_run [] =
{
	ai_run,	9,	NULL,
	ai_run,	2,	NULL,
	ai_run,	3,	NULL,
	ai_run,	3,	NULL,
	ai_run,	1,	NULL,
	ai_run,	0,	NULL,
	ai_run,	0,	NULL,
	ai_run,	10,	NULL,
	ai_run,	-4,	NULL,
	ai_run,	-1,	NULL,
	ai_run,	2,	NULL
};
mmove_t brain_move_run = {FRAME_walk101, FRAME_walk111, brain_frames_run, NULL};

void brain_run (edict_t *self)
{
	self->monsterinfo.power_armor_type = POWER_ARMOR_SCREEN;
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &brain_move_stand;
	else
		self->monsterinfo.currentmove = &brain_move_run;
}


void brain_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	float	r;

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;

	r = random();
	if (r < 0.33)
	{
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &brain_move_pain1;
	}
	else if (r < 0.66)
	{
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &brain_move_pain2;
	}
	else
	{
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
		self->monsterinfo.currentmove = &brain_move_pain3;
	}
}

void brain_dead (edict_t *self)
{
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity (self);
}



void brain_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		n;

	self->s.effects = 0;
	self->monsterinfo.power_armor_type = POWER_ARMOR_NONE;

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
	gi.sound (self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	if (random() <= 0.5)
		self->monsterinfo.currentmove = &brain_move_death1;
	else
		self->monsterinfo.currentmove = &brain_move_death2;
}

/*QUAKED monster_brain (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_brain (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	sound_chest_open = gi.soundindex ("brain/brnatck1.wav");
	sound_tentacles_extend = gi.soundindex ("brain/brnatck2.wav");
	sound_tentacles_retract = gi.soundindex ("brain/brnatck3.wav");
	sound_death = gi.soundindex ("brain/brndeth1.wav");
	sound_idle1 = gi.soundindex ("brain/brnidle1.wav");
	sound_idle2 = gi.soundindex ("brain/brnidle2.wav");
	sound_idle3 = gi.soundindex ("brain/brnlens1.wav");
	sound_pain1 = gi.soundindex ("brain/brnpain1.wav");
	sound_pain2 = gi.soundindex ("brain/brnpain2.wav");
	sound_sight = gi.soundindex ("brain/brnsght1.wav");
	sound_search = gi.soundindex ("brain/brnsrch1.wav");
	sound_melee1 = gi.soundindex ("brain/melee1.wav");
	sound_melee2 = gi.soundindex ("brain/melee2.wav");
	sound_melee3 = gi.soundindex ("brain/melee3.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex ("models/monsters/brain/tris.md2");
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, 32);

	self->health = 300;
	self->gib_health = -150;
	self->mass = 400;

	self->pain = brain_pain;
	self->die = brain_die;

	self->monsterinfo.stand = brain_stand;
	self->monsterinfo.walk = brain_walk;
	self->monsterinfo.run = brain_run;
	self->monsterinfo.dodge = brain_dodge;
	self->monsterinfo.attack = brain_attack;
	self->monsterinfo.melee = brain_melee;
	self->monsterinfo.sight = brain_sight;
	self->monsterinfo.search = brain_search;
	self->monsterinfo.idle = brain_idle;

	self->monsterinfo.power_armor_type = POWER_ARMOR_SCREEN;
	self->monsterinfo.power_armor_power = 100;

	gi.linkentity (self);

	self->monsterinfo.currentmove = &brain_move_stand;	
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start (self);
}
