// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

boss2

==============================================================================
*/

#include "g_local.h"
#include "m_boss2.h"

void BossExplode (edict_t *self);

qboolean infront (edict_t *self, edict_t *other);

static int	sound_pain1;
static int	sound_pain2;
static int	sound_pain3;
static int	sound_death;
static int	sound_search1;

void boss2_search (edict_t *self)
{
	if (random() < 0.5)
		gi.sound (self, CHAN_VOICE, sound_search1, 1, ATTN_NONE, 0);
}

void boss2_run (edict_t *self);
void boss2_stand (edict_t *self);
void boss2_dead (edict_t *self);
void boss2_attack (edict_t *self);
void boss2_attack_mg (edict_t *self);
void boss2_reattack_mg (edict_t *self);
void boss2_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

#define BOSS2_ROCKET_SPEED	750

void Boss2PredictiveRocket  (edict_t *self)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;
	float	time, dist;

gi.dprintf("predictive fire\n");

	AngleVectors (self->s.angles, forward, right, NULL);

//1
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_BOSS2_ROCKET_1], forward, right, start);
		VectorSubtract(self->enemy->s.origin, start, dir);
//		dir[2] += self->enemy->viewheight;
		dist = VectorLength(dir);
		time = dist / BOSS2_ROCKET_SPEED;
		VectorMA(self->enemy->s.origin, time-0.3, self->enemy->velocity, vec);

//	VectorCopy (self->enemy->s.origin, vec);
//	vec[2] += self->enemy->viewheight;
	VectorSubtract (vec, start, dir);
	VectorNormalize (dir);
	monster_fire_rocket (self, start, dir, 50, BOSS2_ROCKET_SPEED, MZ2_BOSS2_ROCKET_1);

//2
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_BOSS2_ROCKET_2], forward, right, start);
		VectorSubtract(self->enemy->s.origin, start, dir);
//		dir[2] += self->enemy->viewheight;
		dist = VectorLength(dir);
		time = dist / BOSS2_ROCKET_SPEED;
		VectorMA(self->enemy->s.origin, time-0.15, self->enemy->velocity, vec);

//	VectorCopy (self->enemy->s.origin, vec);
//	vec[2] += self->enemy->viewheight;
	VectorSubtract (vec, start, dir);
	VectorNormalize (dir);
	monster_fire_rocket (self, start, dir, 50, BOSS2_ROCKET_SPEED, MZ2_BOSS2_ROCKET_2);

//3
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_BOSS2_ROCKET_3], forward, right, start);
		VectorSubtract(self->enemy->s.origin, start, dir);
//		dir[2] += self->enemy->viewheight;
		dist = VectorLength(dir);
		time = dist / BOSS2_ROCKET_SPEED;
		VectorMA(self->enemy->s.origin, time, self->enemy->velocity, vec);

//	VectorCopy (self->enemy->s.origin, vec);
//	vec[2] += self->enemy->viewheight;
	VectorSubtract (vec, start, dir);
	VectorNormalize (dir);
	monster_fire_rocket (self, start, dir, 50, BOSS2_ROCKET_SPEED, MZ2_BOSS2_ROCKET_3);

//4
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_BOSS2_ROCKET_4], forward, right, start);
		VectorSubtract(self->enemy->s.origin, start, dir);
//		dir[2] += self->enemy->viewheight;
		dist = VectorLength(dir);
		time = dist / BOSS2_ROCKET_SPEED;
		VectorMA(self->enemy->s.origin, time+0.15, self->enemy->velocity, vec);

//	VectorCopy (self->enemy->s.origin, vec);
//	vec[2] += self->enemy->viewheight;
	VectorSubtract (vec, start, dir);
	VectorNormalize (dir);
	monster_fire_rocket (self, start, dir, 50, BOSS2_ROCKET_SPEED, MZ2_BOSS2_ROCKET_4);
}	

void Boss2Rocket (edict_t *self)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;

	if(self->enemy)
	{
		if(self->enemy->client && random() < 0.9)
		{
			Boss2PredictiveRocket(self);
			return;
		}
	}

	AngleVectors (self->s.angles, forward, right, NULL);

//1
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_BOSS2_ROCKET_1], forward, right, start);
	VectorCopy (self->enemy->s.origin, vec);
//	vec[2] += self->enemy->viewheight;
	vec[2] -= 15;
	VectorSubtract (vec, start, dir);
	VectorNormalize (dir);
	VectorMA (dir, 0.4, right, dir);
	VectorNormalize (dir);
	monster_fire_rocket (self, start, dir, 50, 500, MZ2_BOSS2_ROCKET_1);

//2
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_BOSS2_ROCKET_2], forward, right, start);
	VectorCopy (self->enemy->s.origin, vec);
//	vec[2] += self->enemy->viewheight;
	VectorSubtract (vec, start, dir);
	VectorNormalize (dir);
	VectorMA (dir, 0.025, right, dir);
	VectorNormalize (dir);
	monster_fire_rocket (self, start, dir, 50, 500, MZ2_BOSS2_ROCKET_2);

//3
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_BOSS2_ROCKET_3], forward, right, start);
	VectorCopy (self->enemy->s.origin, vec);
//	vec[2] += self->enemy->viewheight;
	VectorSubtract (vec, start, dir);
	VectorNormalize (dir);
	VectorMA (dir, -0.025, right, dir);
	VectorNormalize (dir);
	monster_fire_rocket (self, start, dir, 50, 500, MZ2_BOSS2_ROCKET_3);

//4
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_BOSS2_ROCKET_4], forward, right, start);
	VectorCopy (self->enemy->s.origin, vec);
//	vec[2] += self->enemy->viewheight;
	vec[2] -= 15;
	VectorSubtract (vec, start, dir);
	VectorNormalize (dir);
	VectorMA (dir, -0.4, right, dir);
	VectorNormalize (dir);
	monster_fire_rocket (self, start, dir, 50, 500, MZ2_BOSS2_ROCKET_4);

//5
//	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_BOSS2_ROCKET_4], forward, right, start);
//	VectorCopy (self->enemy->s.origin, vec);
//	vec[2] += self->enemy->viewheight;
//	VectorSubtract (vec, start, dir);
//	VectorNormalize (dir);
//	monster_fire_rocket (self, start, dir, 50, 500, MZ2_BOSS2_ROCKET_2);
}	

void boss2_firebullet_right (edict_t *self)
{
	vec3_t	forward, right, target;
	vec3_t	start;

	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_BOSS2_MACHINEGUN_R1], forward, right, start);

//	VectorMA (self->enemy->s.origin, -0.2, self->enemy->velocity, target);
	VectorMA (self->enemy->s.origin, 0.2, self->enemy->velocity, target);
	target[2] += self->enemy->viewheight;
	VectorSubtract (target, start, forward);
	VectorNormalize (forward);

	monster_fire_bullet (self, start, forward, 6, 4, DEFAULT_BULLET_HSPREAD*3, DEFAULT_BULLET_VSPREAD, MZ2_BOSS2_MACHINEGUN_R1);
//	monster_fire_bullet (self, start, forward, 6, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MZ2_BOSS2_MACHINEGUN_R1);
}	

void boss2_firebullet_left (edict_t *self)
{
	vec3_t	forward, right, target;
	vec3_t	start;
	
	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_BOSS2_MACHINEGUN_L1], forward, right, start);

//	VectorMA (self->enemy->s.origin, 0.2, self->enemy->velocity, target);
	VectorMA (self->enemy->s.origin, -0.2, self->enemy->velocity, target);

	target[2] += self->enemy->viewheight;
	VectorSubtract (target, start, forward);
	VectorNormalize (forward);

	monster_fire_bullet (self, start, forward, 6, 4, DEFAULT_BULLET_HSPREAD*3, DEFAULT_BULLET_VSPREAD, MZ2_BOSS2_MACHINEGUN_L1);
//	monster_fire_bullet (self, start, forward, 6, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MZ2_BOSS2_MACHINEGUN_L1);
}	

void Boss2MachineGun (edict_t *self)
{
/*	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;
	int		flash_number;

	AngleVectors (self->s.angles, forward, right, NULL);

	flash_number = MZ2_BOSS2_MACHINEGUN_1 + (self->s.frame - FRAME_attack10);
	G_ProjectSource (self->s.origin, monster_flash_offset[flash_number], forward, right, start);

	VectorCopy (self->enemy->s.origin, vec);
	vec[2] += self->enemy->viewheight;
	VectorSubtract (vec, start, dir);
	VectorNormalize (dir);
	monster_fire_bullet (self, start, dir, 3, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_number);
*/
	boss2_firebullet_left(self);
	boss2_firebullet_right(self);
}	


mframe_t boss2_frames_stand [] =
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
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL
};
mmove_t	boss2_move_stand = {FRAME_stand30, FRAME_stand50, boss2_frames_stand, NULL};

mframe_t boss2_frames_fidget [] =
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
mmove_t boss2_move_fidget = {FRAME_stand1, FRAME_stand30, boss2_frames_fidget, NULL};

mframe_t boss2_frames_walk [] =
{
	/* PMM - normally, this is all 8's .. I upped it */
	ai_walk,	10,	NULL,
	ai_walk,	10,	NULL,
	ai_walk,	10,	NULL,
	ai_walk,	10,	NULL,
	ai_walk,	10,	NULL,
	ai_walk,	10,	NULL,
	ai_walk,	10,	NULL,
	ai_walk,	10,	NULL,
	ai_walk,	10,	NULL,
	ai_walk,	10,	NULL,
	ai_walk,	10,	NULL,
	ai_walk,	10,	NULL,
	ai_walk,	10,	NULL,
	ai_walk,	10,	NULL,
	ai_walk,	10,	NULL,
	ai_walk,	10,	NULL,
	ai_walk,	10,	NULL,
	ai_walk,	10,	NULL,
	ai_walk,	10,	NULL,
	ai_walk,	10,	NULL
};
mmove_t boss2_move_walk = {FRAME_walk1, FRAME_walk20, boss2_frames_walk, NULL};


mframe_t boss2_frames_run [] =
{
	/* PMM - normally, this is all 8's .. I upped it */
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL,
	ai_run,	10,	NULL
};
mmove_t boss2_move_run = {FRAME_walk1, FRAME_walk20, boss2_frames_run, NULL};

mframe_t boss2_frames_attack_pre_mg [] =
{
	/* used to be all 1's .. what a slow guy */
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	boss2_attack_mg
};
mmove_t boss2_move_attack_pre_mg = {FRAME_attack1, FRAME_attack9, boss2_frames_attack_pre_mg, NULL};


// Loop this
mframe_t boss2_frames_attack_mg [] =
{
	/* used to be all 1's .. what a slow guy */
	ai_charge,	2,	Boss2MachineGun,
	ai_charge,	2,	Boss2MachineGun,
	ai_charge,	2,	Boss2MachineGun,
	ai_charge,	2,	Boss2MachineGun,
	ai_charge,	2,	Boss2MachineGun,
	ai_charge,	2,	boss2_reattack_mg
};
mmove_t boss2_move_attack_mg = {FRAME_attack10, FRAME_attack15, boss2_frames_attack_mg, NULL};

mframe_t boss2_frames_attack_post_mg [] =
{
	/* used to be all 1's .. what a slow guy */
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL
};
mmove_t boss2_move_attack_post_mg = {FRAME_attack16, FRAME_attack19, boss2_frames_attack_post_mg, boss2_run};

mframe_t boss2_frames_attack_rocket [] =
{
	/* used to be all 1's .. except the Boss2Rocket frame, which was -20(!) */
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_move,	-5,	Boss2Rocket,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL,
	ai_charge,	2,	NULL
};
mmove_t boss2_move_attack_rocket = {FRAME_attack20, FRAME_attack40, boss2_frames_attack_rocket, boss2_run};

mframe_t boss2_frames_pain_heavy [] =
{
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
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL
};
mmove_t boss2_move_pain_heavy = {FRAME_pain2, FRAME_pain19, boss2_frames_pain_heavy, boss2_run};

mframe_t boss2_frames_pain_light [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL
};
mmove_t boss2_move_pain_light = {FRAME_pain20, FRAME_pain23, boss2_frames_pain_light, boss2_run};

mframe_t boss2_frames_death [] =
{
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
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	BossExplode
};
mmove_t boss2_move_death = {FRAME_death2, FRAME_death50, boss2_frames_death, boss2_dead};

void boss2_stand (edict_t *self)
{
		self->monsterinfo.currentmove = &boss2_move_stand;
}

void boss2_run (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &boss2_move_stand;
	else
		self->monsterinfo.currentmove = &boss2_move_run;
}

void boss2_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &boss2_move_walk;
}

void boss2_attack (edict_t *self)
{
	vec3_t	vec;
	float	range;

	VectorSubtract (self->enemy->s.origin, self->s.origin, vec);
	range = VectorLength (vec);
	
	if (range <= 125)
	{
		self->monsterinfo.currentmove = &boss2_move_attack_pre_mg;
	}
	else 
	{
		if (random() <= 0.6)
			self->monsterinfo.currentmove = &boss2_move_attack_pre_mg;
		else
			self->monsterinfo.currentmove = &boss2_move_attack_rocket;
	}
}

void boss2_attack_mg (edict_t *self)
{
	self->monsterinfo.currentmove = &boss2_move_attack_mg;
}

void boss2_reattack_mg (edict_t *self)
{
	if ( infront(self, self->enemy) )
		if (random() <= 0.7)
			self->monsterinfo.currentmove = &boss2_move_attack_mg;
		else
			self->monsterinfo.currentmove = &boss2_move_attack_post_mg;
	else
		self->monsterinfo.currentmove = &boss2_move_attack_post_mg;
}


void boss2_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;
// American wanted these at no attenuation
	if (damage < 10)
	{
		gi.sound (self, CHAN_VOICE, sound_pain3, 1, ATTN_NONE, 0);
		self->monsterinfo.currentmove = &boss2_move_pain_light;
	}
	else if (damage < 30)
	{
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NONE, 0);
		self->monsterinfo.currentmove = &boss2_move_pain_light;
	}
	else 
	{
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NONE, 0);
		self->monsterinfo.currentmove = &boss2_move_pain_heavy;
	}
}

void boss2_dead (edict_t *self)
{
	VectorSet (self->mins, -56, -56, 0);
	VectorSet (self->maxs, 56, 56, 80);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity (self);
}

void boss2_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	gi.sound (self, CHAN_VOICE, sound_death, 1, ATTN_NONE, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_NO;
	self->count = 0;
	self->monsterinfo.currentmove = &boss2_move_death;
#if 0
	int		n;

	self->s.sound = 0;
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

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.currentmove = &boss2_move_death;
#endif
}

qboolean Boss2_CheckAttack (edict_t *self)
{
	vec3_t	spot1, spot2;
	vec3_t	temp;
	float	chance;
	trace_t	tr;
	qboolean	enemy_infront;
	int			enemy_range;
	float		enemy_yaw;

	if (self->enemy->health > 0)
	{
	// see if any entities are in the way of the shot
		VectorCopy (self->s.origin, spot1);
		spot1[2] += self->viewheight;
		VectorCopy (self->enemy->s.origin, spot2);
		spot2[2] += self->enemy->viewheight;

		tr = gi.trace (spot1, NULL, NULL, spot2, self, CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_SLIME|CONTENTS_LAVA);

		// do we have a clear shot?
		if (tr.ent != self->enemy)
		{	
			// PGM - we want them to go ahead and shoot at info_notnulls if they can.
			if(self->enemy->solid != SOLID_NOT || tr.fraction < 1.0)		//PGM
				return false;
		}
	}
	
	enemy_infront = infront(self, self->enemy);
	enemy_range = range(self, self->enemy);
	VectorSubtract (self->enemy->s.origin, self->s.origin, temp);
	enemy_yaw = vectoyaw(temp);

	self->ideal_yaw = enemy_yaw;


	// melee attack
	if (enemy_range == RANGE_MELEE)
	{
		if (self->monsterinfo.melee)
			self->monsterinfo.attack_state = AS_MELEE;
		else
			self->monsterinfo.attack_state = AS_MISSILE;
		return true;
	}
	
// missile attack
	if (!self->monsterinfo.attack)
		return false;
		
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
		chance = 0.8;
	}
	else if (enemy_range == RANGE_NEAR)
	{
		chance = 0.8;
	}
	else if (enemy_range == RANGE_MID)
	{
		chance = 0.8;
	}
	else
	{
		return false;
	}

	// PGM - go ahead and shoot every time if it's a info_notnull
	if ((random () < chance) || (self->enemy->solid == SOLID_NOT))
	{
		self->monsterinfo.attack_state = AS_MISSILE;
		self->monsterinfo.attack_finished = level.time + 2*random();
		return true;
	}

	if (self->flags & FL_FLY)
	{
		if (random() < 0.3)
			self->monsterinfo.attack_state = AS_SLIDING;
		else
			self->monsterinfo.attack_state = AS_STRAIGHT;
	}

	return false;
}



/*QUAKED monster_boss2 (1 .5 0) (-56 -56 0) (56 56 80) Ambush Trigger_Spawn Sight
*/
void SP_monster_boss2 (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	sound_pain1 = gi.soundindex ("bosshovr/bhvpain1.wav");
	sound_pain2 = gi.soundindex ("bosshovr/bhvpain2.wav");
	sound_pain3 = gi.soundindex ("bosshovr/bhvpain3.wav");
	sound_death = gi.soundindex ("bosshovr/bhvdeth1.wav");
	sound_search1 = gi.soundindex ("bosshovr/bhvunqv1.wav");

	self->s.sound = gi.soundindex ("bosshovr/bhvengn1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex ("models/monsters/boss2/tris.md2");
	VectorSet (self->mins, -56, -56, 0);
	VectorSet (self->maxs, 56, 56, 80);

	self->health = 2000;
	self->gib_health = -200;
	self->mass = 1000;

	self->yaw_speed = 50;

	self->flags |= FL_IMMUNE_LASER;

	self->pain = boss2_pain;
	self->die = boss2_die;

	self->monsterinfo.stand = boss2_stand;
	self->monsterinfo.walk = boss2_walk;
	self->monsterinfo.run = boss2_run;
	self->monsterinfo.attack = boss2_attack;
	self->monsterinfo.search = boss2_search;
	self->monsterinfo.checkattack = Boss2_CheckAttack;
	gi.linkentity (self);

	self->monsterinfo.currentmove = &boss2_move_stand;	
	self->monsterinfo.scale = MODEL_SCALE;

	flymonster_start (self);
}
