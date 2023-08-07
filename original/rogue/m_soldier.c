// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

SOLDIER

==============================================================================
*/

#include "g_local.h"
#include "m_soldier.h"

//ROGUE
#define RUN_SHOOT		1
#define CHECK_TARGET	1
//ROGUE

static int	sound_idle;
static int	sound_sight1;
static int	sound_sight2;
static int	sound_pain_light;
static int	sound_pain;
static int	sound_pain_ss;
static int	sound_death_light;
static int	sound_death;
static int	sound_death_ss;
static int	sound_cock;

void soldier_duck_up (edict_t *self);

void soldier_start_charge (edict_t *self)
{
	self->monsterinfo.aiflags |= AI_CHARGING;
}

void soldier_stop_charge (edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_CHARGING;
}

void soldier_idle (edict_t *self)
{
	if (random() > 0.8)
		gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

void soldier_cock (edict_t *self)
{
	if (self->s.frame == FRAME_stand322)
		gi.sound (self, CHAN_WEAPON, sound_cock, 1, ATTN_IDLE, 0);
	else
		gi.sound (self, CHAN_WEAPON, sound_cock, 1, ATTN_NORM, 0);
}


// STAND

void soldier_stand (edict_t *self);

mframe_t soldier_frames_stand1 [] =
{
	ai_stand, 0, soldier_idle,
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
mmove_t soldier_move_stand1 = {FRAME_stand101, FRAME_stand130, soldier_frames_stand1, soldier_stand};

mframe_t soldier_frames_stand3 [] =
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
	ai_stand, 0, soldier_cock,
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
mmove_t soldier_move_stand3 = {FRAME_stand301, FRAME_stand339, soldier_frames_stand3, soldier_stand};

#if 0
mframe_t soldier_frames_stand4 [] =
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
	ai_stand, 4, NULL,
	ai_stand, 1, NULL,
	ai_stand, -1, NULL,
	ai_stand, -2, NULL,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL
};
mmove_t soldier_move_stand4 = {FRAME_stand401, FRAME_stand452, soldier_frames_stand4, NULL};
#endif

void soldier_stand (edict_t *self)
{
	if ((self->monsterinfo.currentmove == &soldier_move_stand3) || (random() < 0.8))
		self->monsterinfo.currentmove = &soldier_move_stand1;
	else
		self->monsterinfo.currentmove = &soldier_move_stand3;
}


//
// WALK
//

void soldier_walk1_random (edict_t *self)
{
	if (random() > 0.1)
		self->monsterinfo.nextframe = FRAME_walk101;
}

mframe_t soldier_frames_walk1 [] =
{
	ai_walk, 3,  NULL,
	ai_walk, 6,  NULL,
	ai_walk, 2,  NULL,
	ai_walk, 2,  NULL,
	ai_walk, 2,  NULL,
	ai_walk, 1,  NULL,
	ai_walk, 6,  NULL,
	ai_walk, 5,  NULL,
	ai_walk, 3,  NULL,
	ai_walk, -1, soldier_walk1_random,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 0,  NULL
};
mmove_t soldier_move_walk1 = {FRAME_walk101, FRAME_walk133, soldier_frames_walk1, NULL};

mframe_t soldier_frames_walk2 [] =
{
	ai_walk, 4,  NULL,
	ai_walk, 4,  NULL,
	ai_walk, 9,  NULL,
	ai_walk, 8,  NULL,
	ai_walk, 5,  NULL,
	ai_walk, 1,  NULL,
	ai_walk, 3,  NULL,
	ai_walk, 7,  NULL,
	ai_walk, 6,  NULL,
	ai_walk, 7,  NULL
};
mmove_t soldier_move_walk2 = {FRAME_walk209, FRAME_walk218, soldier_frames_walk2, NULL};

void soldier_walk (edict_t *self)
{
	if (random() < 0.5)
		self->monsterinfo.currentmove = &soldier_move_walk1;
	else
		self->monsterinfo.currentmove = &soldier_move_walk2;
}


//
// RUN
//

void soldier_run (edict_t *self);

mframe_t soldier_frames_start_run [] =
{
	ai_run, 7,  NULL,
	ai_run, 5,  NULL
};
mmove_t soldier_move_start_run = {FRAME_run01, FRAME_run02, soldier_frames_start_run, soldier_run};

#ifdef RUN_SHOOT
void soldier_fire (edict_t *self, int);

void soldier_fire_run (edict_t *self) {
	if ((self->s.skinnum <= 1) && (self->enemy) && visible(self, self->enemy)) {
		soldier_fire(self, 0);
	}
}
#endif

mframe_t soldier_frames_run [] =
{
	ai_run, 10, NULL,
	ai_run, 11, monster_done_dodge,
	ai_run, 11, NULL,
	ai_run, 16, NULL,
	ai_run, 10, NULL,
	ai_run, 15, monster_done_dodge
};

mmove_t soldier_move_run = {FRAME_run03, FRAME_run08, soldier_frames_run, NULL};

void soldier_run (edict_t *self)
{
	monster_done_dodge (self);

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		self->monsterinfo.currentmove = &soldier_move_stand1;
		return;
	}

	if (self->monsterinfo.currentmove == &soldier_move_walk1 ||
		self->monsterinfo.currentmove == &soldier_move_walk2 ||
		self->monsterinfo.currentmove == &soldier_move_start_run)
	{
		self->monsterinfo.currentmove = &soldier_move_run;
	}
	else
	{
		self->monsterinfo.currentmove = &soldier_move_start_run;
	}
}


//
// PAIN
//

mframe_t soldier_frames_pain1 [] =
{
	ai_move, -3, NULL,
	ai_move, 4,  NULL,
	ai_move, 1,  NULL,
	ai_move, 1,  NULL,
	ai_move, 0,  NULL
};
mmove_t soldier_move_pain1 = {FRAME_pain101, FRAME_pain105, soldier_frames_pain1, soldier_run};

mframe_t soldier_frames_pain2 [] =
{
	ai_move, -13, NULL,
	ai_move, -1,  NULL,
	ai_move, 2,   NULL,
	ai_move, 4,   NULL,
	ai_move, 2,   NULL,
	ai_move, 3,   NULL,
	ai_move, 2,   NULL
};
mmove_t soldier_move_pain2 = {FRAME_pain201, FRAME_pain207, soldier_frames_pain2, soldier_run};

mframe_t soldier_frames_pain3 [] =
{
	ai_move, -8, NULL,
	ai_move, 10, NULL,
	ai_move, -4, NULL,
	ai_move, -1, NULL,
	ai_move, -3, NULL,
	ai_move, 0,  NULL,
	ai_move, 3,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 0,  NULL,
	ai_move, 1,  NULL,
	ai_move, 0,  NULL,
	ai_move, 1,  NULL,
	ai_move, 2,  NULL,
	ai_move, 4,  NULL,
	ai_move, 3,  NULL,
	ai_move, 2,  NULL
};
mmove_t soldier_move_pain3 = {FRAME_pain301, FRAME_pain318, soldier_frames_pain3, soldier_run};

mframe_t soldier_frames_pain4 [] =
{
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, -10, NULL,
	ai_move, -6,  NULL,
	ai_move, 8,   NULL,
	ai_move, 4,   NULL,
	ai_move, 1,   NULL,
	ai_move, 0,   NULL,
	ai_move, 2,   NULL,
	ai_move, 5,   NULL,
	ai_move, 2,   NULL,
	ai_move, -1,  NULL,
	ai_move, -1,  NULL,
	ai_move, 3,   NULL,
	ai_move, 2,   NULL,
	ai_move, 0,   NULL
};
mmove_t soldier_move_pain4 = {FRAME_pain401, FRAME_pain417, soldier_frames_pain4, soldier_run};


void soldier_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	float	r;
	int		n;

	if (self->health < (self->max_health / 2))
			self->s.skinnum |= 1;

	monster_done_dodge (self);
	soldier_stop_charge(self);

	// if we're blind firing, this needs to be turned off here
	self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;

	if (level.time < self->pain_debounce_time)
	{
		if ((self->velocity[2] > 100) && ( (self->monsterinfo.currentmove == &soldier_move_pain1) || (self->monsterinfo.currentmove == &soldier_move_pain2) || (self->monsterinfo.currentmove == &soldier_move_pain3)))
		{
			// PMM - clear duck flag
			if (self->monsterinfo.aiflags & AI_DUCKED)
				monster_duck_up(self);
			self->monsterinfo.currentmove = &soldier_move_pain4;
		}
		return;
	}

	self->pain_debounce_time = level.time + 3;

	n = self->s.skinnum | 1;
	if (n == 1)
		gi.sound (self, CHAN_VOICE, sound_pain_light, 1, ATTN_NORM, 0);
	else if (n == 3)
		gi.sound (self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
	else
		gi.sound (self, CHAN_VOICE, sound_pain_ss, 1, ATTN_NORM, 0);

	if (self->velocity[2] > 100)
	{
		// PMM - clear duck flag
		if (self->monsterinfo.aiflags & AI_DUCKED)
			monster_duck_up(self);
		self->monsterinfo.currentmove = &soldier_move_pain4;
//		self->monsterinfo.pausetime = 0;
		return;
	}

	if (skill->value == 3)
		return;		// no pain anims in nightmare

	r = random();

	if (r < 0.33)
		self->monsterinfo.currentmove = &soldier_move_pain1;
	else if (r < 0.66)
		self->monsterinfo.currentmove = &soldier_move_pain2;
	else
		self->monsterinfo.currentmove = &soldier_move_pain3;

	// PMM - clear duck flag
	if (self->monsterinfo.aiflags & AI_DUCKED)
		monster_duck_up(self);
//	self->monsterinfo.pausetime = 0;

}


//
// ATTACK
//

static int blaster_flash [] = {MZ2_SOLDIER_BLASTER_1, MZ2_SOLDIER_BLASTER_2, MZ2_SOLDIER_BLASTER_3, MZ2_SOLDIER_BLASTER_4, MZ2_SOLDIER_BLASTER_5, MZ2_SOLDIER_BLASTER_6, MZ2_SOLDIER_BLASTER_7, MZ2_SOLDIER_BLASTER_8};
static int shotgun_flash [] = {MZ2_SOLDIER_SHOTGUN_1, MZ2_SOLDIER_SHOTGUN_2, MZ2_SOLDIER_SHOTGUN_3, MZ2_SOLDIER_SHOTGUN_4, MZ2_SOLDIER_SHOTGUN_5, MZ2_SOLDIER_SHOTGUN_6, MZ2_SOLDIER_SHOTGUN_7, MZ2_SOLDIER_SHOTGUN_8};
static int machinegun_flash [] = {MZ2_SOLDIER_MACHINEGUN_1, MZ2_SOLDIER_MACHINEGUN_2, MZ2_SOLDIER_MACHINEGUN_3, MZ2_SOLDIER_MACHINEGUN_4, MZ2_SOLDIER_MACHINEGUN_5, MZ2_SOLDIER_MACHINEGUN_6, MZ2_SOLDIER_MACHINEGUN_7, MZ2_SOLDIER_MACHINEGUN_8};

//void soldier_fire (edict_t *self, int flash_number)  PMM
void soldier_fire (edict_t *self, int in_flash_number)
{
	vec3_t	start;
	vec3_t	forward, right, up;
	vec3_t	aim;
	vec3_t	dir;
	vec3_t	end;
	float	r, u;
	int		flash_index;
	int		flash_number;
#ifdef RUN_SHOOT
	vec3_t	aim_norm;
	float	angle;
#endif
#ifdef CHECK_TARGET
	trace_t	tr;
	vec3_t aim_good;
#endif

	if ((!self->enemy) || (!self->enemy->inuse))
	{
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		return;
	}

	if (in_flash_number < 0)
	{
		flash_number = -1 * in_flash_number;
	}
	else
		flash_number = in_flash_number;

	if (self->s.skinnum < 2)
		flash_index = blaster_flash[flash_number];
	else if (self->s.skinnum < 4)
		flash_index = shotgun_flash[flash_number];
	else
		flash_index = machinegun_flash[flash_number];

	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource (self->s.origin, monster_flash_offset[flash_index], forward, right, start);

	if (flash_number == 5 || flash_number == 6) // he's dead
	{
		VectorCopy (forward, aim);
	}
	else
	{
		VectorCopy (self->enemy->s.origin, end);
		end[2] += self->enemy->viewheight;
		VectorSubtract (end, start, aim);
#ifdef CHECK_TARGET
		VectorCopy (end, aim_good);
#endif
#ifdef RUN_SHOOT
		//PMM
		if (in_flash_number < 0)
		{
			VectorCopy (aim, aim_norm);
			VectorNormalize (aim_norm);
			angle = DotProduct (aim_norm, forward);
			//gi.dprintf ("Dot Product:  %f", DotProduct (aim_norm, forward));
			if (angle < 0.9)  // ~25 degree angle
			{
//				if(g_showlogic && g_showlogic->value)
//					gi.dprintf (" not firing due to bad dotprod %f\n", angle);
				return;
			}
//			else
//			{
//				if(g_showlogic && g_showlogic->value)
//					gi.dprintf (" firing:  dotprod = %f\n", angle);
//			}
		}
		//-PMM
#endif
		vectoangles (aim, dir);
		AngleVectors (dir, forward, right, up);
		
		if (skill->value < 2)
		{
			r = crandom()*1000;
			u = crandom()*500;
		}
		else
		{
			r = crandom()*500;
			u = crandom()*250;
		}
		VectorMA (start, 8192, forward, end);
		VectorMA (end, r, right, end);
		VectorMA (end, u, up, end);

		VectorSubtract (end, start, aim);
		VectorNormalize (aim);
	}
#ifdef CHECK_TARGET
	if (!(flash_number == 5 || flash_number == 6)) // he's dead
	{
		tr = gi.trace (start, NULL, NULL, aim_good, self, MASK_SHOT);
		if ((tr.ent != self->enemy) && (tr.ent != world))
		{
//			if(g_showlogic && g_showlogic->value)
//				gi.dprintf ("infantry shot aborted due to bad target\n");
			return;
		}
	}
#endif
	if (self->s.skinnum <= 1)
	{
		monster_fire_blaster (self, start, aim, 5, 600, flash_index, EF_BLASTER);
	}
	else if (self->s.skinnum <= 3)
	{
		monster_fire_shotgun (self, start, aim, 2, 1, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SHOTGUN_COUNT, flash_index);
	}
	else
	{
		// PMM - changed to wait from pausetime to not interfere with dodge code
		if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
			self->wait = level.time + (3 + rand() % 8) * FRAMETIME;

		monster_fire_bullet (self, start, aim, 2, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_index);

		if (level.time >= self->wait)
			self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		else
			self->monsterinfo.aiflags |= AI_HOLD_FRAME;
	}
}

// ATTACK1 (blaster/shotgun)

void soldier_fire1 (edict_t *self)
{
	soldier_fire (self, 0);
}

void soldier_attack1_refire1 (edict_t *self)
{
	// PMM - blindfire
	if (self->monsterinfo.aiflags & AI_MANUAL_STEERING)
	{
		self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
		return;
	}
	// pmm

	if (!self->enemy)
		return;

	if (self->s.skinnum > 1)
		return;

	if (self->enemy->health <= 0)
		return;

	if ( ((skill->value == 3) && (random() < 0.5)) || (range(self, self->enemy) == RANGE_MELEE) )
		self->monsterinfo.nextframe = FRAME_attak102;
	else
		self->monsterinfo.nextframe = FRAME_attak110;
}

void soldier_attack1_refire2 (edict_t *self)
{
	if (!self->enemy)
		return;

	if (self->s.skinnum < 2)
		return;

	if (self->enemy->health <= 0)
		return;

	if ( ((skill->value == 3) && (random() < 0.5)) || (range(self, self->enemy) == RANGE_MELEE) )
		self->monsterinfo.nextframe = FRAME_attak102;
}

mframe_t soldier_frames_attack1 [] =
{
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  soldier_fire1,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  soldier_attack1_refire1,
	ai_charge, 0,  NULL,
	ai_charge, 0,  soldier_cock,
	ai_charge, 0,  soldier_attack1_refire2,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL
};
mmove_t soldier_move_attack1 = {FRAME_attak101, FRAME_attak112, soldier_frames_attack1, soldier_run};

// ATTACK2 (blaster/shotgun)

void soldier_fire2 (edict_t *self)
{
	soldier_fire (self, 1);
}

void soldier_attack2_refire1 (edict_t *self)
{
	if (!self->enemy)
		return;

	if (self->s.skinnum > 1)
		return;

	if (self->enemy->health <= 0)
		return;

	if ( ((skill->value == 3) && (random() < 0.5)) || (range(self, self->enemy) == RANGE_MELEE) )
		self->monsterinfo.nextframe = FRAME_attak204;
	else
		self->monsterinfo.nextframe = FRAME_attak216;
}

void soldier_attack2_refire2 (edict_t *self)
{
	if (!self->enemy)
		return;

	if (self->s.skinnum < 2)
		return;

	if (self->enemy->health <= 0)
		return;

	if ( ((skill->value == 3) && (random() < 0.5)) || (range(self, self->enemy) == RANGE_MELEE) )
		self->monsterinfo.nextframe = FRAME_attak204;
}

mframe_t soldier_frames_attack2 [] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, soldier_fire2,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, soldier_attack2_refire1,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, soldier_cock,
	ai_charge, 0, NULL,
	ai_charge, 0, soldier_attack2_refire2,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t soldier_move_attack2 = {FRAME_attak201, FRAME_attak218, soldier_frames_attack2, soldier_run};

// ATTACK3 (duck and shoot)
/*
void soldier_duck_down (edict_t *self)
{
	if ((g_showlogic) && (g_showlogic->value))
		gi.dprintf ("duck down - %d!\n", self->s.frame);

	self->monsterinfo.aiflags |= AI_DUCKED;
//	self->maxs[2] -= 32;
	self->maxs[2] =  self->monsterinfo.base_height - 32;
	self->takedamage = DAMAGE_YES;
	if (self->monsterinfo.duck_wait_time < level.time)
	{
		if ((g_showlogic) && (g_showlogic->value))
			gi.dprintf ("soldier duck with no time!\n");
		self->monsterinfo.duck_wait_time = level.time + 1;
	}
	gi.linkentity (self);
}

void soldier_duck_up (edict_t *self)
{
	if ((g_showlogic) && (g_showlogic->value))
		gi.dprintf ("duck up - %d!\n", self->s.frame);
	self->monsterinfo.aiflags &= ~AI_DUCKED;
//	self->maxs[2] += 32;
	self->maxs[2] = self->monsterinfo.base_height;
	self->takedamage = DAMAGE_AIM;
	gi.linkentity (self);
}
*/
void soldier_fire3 (edict_t *self)
{
	monster_duck_down (self);
	soldier_fire (self, 2);
}

void soldier_attack3_refire (edict_t *self)
{
	if ((level.time + 0.4) < self->monsterinfo.duck_wait_time)
		self->monsterinfo.nextframe = FRAME_attak303;
}

mframe_t soldier_frames_attack3 [] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, soldier_fire3,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, soldier_attack3_refire,
	ai_charge, 0, monster_duck_up,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t soldier_move_attack3 = {FRAME_attak301, FRAME_attak309, soldier_frames_attack3, soldier_run};

// ATTACK4 (machinegun)

void soldier_fire4 (edict_t *self)
{
	soldier_fire (self, 3);
//
//	if (self->enemy->health <= 0)
//		return;
//
//	if ( ((skill->value == 3) && (random() < 0.5)) || (range(self, self->enemy) == RANGE_MELEE) )
//		self->monsterinfo.nextframe = FRAME_attak402;
}

mframe_t soldier_frames_attack4 [] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, soldier_fire4,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t soldier_move_attack4 = {FRAME_attak401, FRAME_attak406, soldier_frames_attack4, soldier_run};

#if 0
// ATTACK5 (prone)

void soldier_fire5 (edict_t *self)
{
	soldier_fire (self, 4);
}

void soldier_attack5_refire (edict_t *self)
{
	if (!self->enemy)
		return;

	if (self->enemy->health <= 0)
		return;

	if ( ((skill->value == 3) && (random() < 0.5)) || (range(self, self->enemy) == RANGE_MELEE) )
		self->monsterinfo.nextframe = FRAME_attak505;
}

mframe_t soldier_frames_attack5 [] =
{
	ai_charge, 8, NULL,
	ai_charge, 8, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, soldier_fire5,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, soldier_attack5_refire
};
mmove_t soldier_move_attack5 = {FRAME_attak501, FRAME_attak508, soldier_frames_attack5, soldier_run};
#endif

// ATTACK6 (run & shoot)

void soldier_fire8 (edict_t *self)
{
	soldier_fire (self, -7);
//	self->monsterinfo.aiflags |= AI_HOLD_FRAME;
//	self->monsterinfo.pausetime = level.time + 1000000;
}

void soldier_attack6_refire (edict_t *self)
{
	// PMM - make sure dodge & charge bits are cleared
	monster_done_dodge (self);
	soldier_stop_charge (self);

	if (!self->enemy)
		return;

	if (self->enemy->health <= 0)
		return;

//	if (range(self, self->enemy) < RANGE_MID)
	if (range(self, self->enemy) < RANGE_NEAR)
		return;

	if ((skill->value == 3) || ((random() < (0.25*((float)skill->value)))))
		self->monsterinfo.nextframe = FRAME_runs03;
}

mframe_t soldier_frames_attack6 [] =
{
//	PMM
//	ai_run, 10, NULL,
	ai_run, 10, soldier_start_charge,
	ai_run,  4, NULL,
	ai_run, 12, soldier_fire8,
	ai_run, 11, NULL,
	ai_run, 13, monster_done_dodge,
	ai_run, 18, NULL,
	ai_run, 15, NULL,
	ai_run, 14, NULL,
	ai_run, 11, NULL,
	ai_run,  8, NULL,
	ai_run, 11, NULL,
	ai_run, 12, NULL,
	ai_run, 12, NULL,
	ai_run, 17, soldier_attack6_refire
};
mmove_t soldier_move_attack6 = {FRAME_runs01, FRAME_runs14, soldier_frames_attack6, soldier_run};

void soldier_attack(edict_t *self)
{
	float r, chance;

	monster_done_dodge (self);

	// PMM - blindfire!
	if (self->monsterinfo.attack_state == AS_BLIND)
	{
		// setup shot probabilities
		if (self->monsterinfo.blind_fire_delay < 1.0)
			chance = 1.0;
		else if (self->monsterinfo.blind_fire_delay < 7.5)
			chance = 0.4;
		else
			chance = 0.1;

		r = random();

		// minimum of 2 seconds, plus 0-3, after the shots are done
		self->monsterinfo.blind_fire_delay += 2.1 + 2.0 + random()*3.0;

		// don't shoot at the origin
		if (VectorCompare (self->monsterinfo.blind_fire_target, vec3_origin))
			return;

		// don't shoot if the dice say not to
		if (r > chance)
		{
//			if ((g_showlogic) && (g_showlogic->value))
//				gi.dprintf ("blindfire - NO SHOT\n");
			return;
		}

		// turn on manual steering to signal both manual steering and blindfire
		self->monsterinfo.aiflags |= AI_MANUAL_STEERING;
		self->monsterinfo.currentmove = &soldier_move_attack1;
		self->monsterinfo.attack_finished = level.time + 1.5 + random();
		return;
	}
	// pmm

// PMM - added this so the soldiers now run toward you and shoot instead of just stopping and shooting
//	if ((range(self, self->enemy) >= RANGE_MID) && (r < (skill->value*0.25) && (self->s.skinnum <= 3)))

	r = random();

	if ((!(self->monsterinfo.aiflags & (AI_BLOCKED|AI_STAND_GROUND))) &&
		(range(self, self->enemy) >= RANGE_NEAR) && 
		(r < (skill->value*0.25) && 
		(self->s.skinnum <= 3)))
	{
		self->monsterinfo.currentmove = &soldier_move_attack6;
	}
	else
	{
		if (self->s.skinnum < 4)
		{
			if (random() < 0.5)
				self->monsterinfo.currentmove = &soldier_move_attack1;
			else
				self->monsterinfo.currentmove = &soldier_move_attack2;
		}
		else
		{
			self->monsterinfo.currentmove = &soldier_move_attack4;
		}
	}
}


//
// SIGHT
//

void soldier_sight(edict_t *self, edict_t *other)
{
	if (random() < 0.5)
		gi.sound (self, CHAN_VOICE, sound_sight1, 1, ATTN_NORM, 0);
	else
		gi.sound (self, CHAN_VOICE, sound_sight2, 1, ATTN_NORM, 0);

//	if ((skill->value > 0) && (self->enemy) && (range(self, self->enemy) >= RANGE_MID))
	if ((skill->value > 0) && (self->enemy) && (range(self, self->enemy) >= RANGE_NEAR))
	{
//	PMM - don't let machinegunners run & shoot
		if ((random() > 0.75) && (self->s.skinnum <= 3))
			self->monsterinfo.currentmove = &soldier_move_attack6;
	}
}

//
// DUCK
//
/*
void soldier_duck_hold (edict_t *self)
{
	if (level.time >= self->monsterinfo.duck_wait_time)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}
*/
mframe_t soldier_frames_duck [] =
{
	ai_move, 5, monster_duck_down,
	ai_move, -1, monster_duck_hold,
	ai_move, 1,  NULL,
	ai_move, 0,  monster_duck_up,
	ai_move, 5,  NULL
};
mmove_t soldier_move_duck = {FRAME_duck01, FRAME_duck05, soldier_frames_duck, soldier_run};

/*
void soldier_dodge (edict_t *self, edict_t *attacker, float eta, trace_t *tr)
{
//===========
//PMM - rogue rewrite of dodge code.
// lots o' changes in here.  Basically, they now check the tr and see if ducking would help,
// and if it doesn't, they dodge like mad
	float	r = random();
	float	height;

	if ((g_showlogic) && (g_showlogic->value))
	{
		if (self->monsterinfo.aiflags & AI_DODGING)
			gi.dprintf ("dodging - ");
		if (self->monsterinfo.aiflags & AI_DUCKED)
			gi.dprintf ("ducked - ");
	}
	if (!self->enemy)
	{
		self->enemy = attacker;
		FoundTarget (self);
	}

	// PMM - don't bother if it's going to hit anyway; fix for weird in-your-face etas (I was
	// seeing numbers like 13 and 14)
	if ((eta < 0.1) || (eta > 5))
	{
		if ((g_showlogic) && (g_showlogic->value))
			gi.dprintf ("timeout\n");
		return;
	}

	// skill level determination..
	if (r > (0.25*((skill->value)+1)))
	{
		if ((g_showlogic) && (g_showlogic->value))
			gi.dprintf ("skillout\n");
		return;
	}

	// stop charging, since we're going to dodge (somehow) instead
	soldier_stop_charge (self);

	height = self->absmax[2]-32-1;  // the -1 is because the absmax is s.origin + maxs + 1

	// if we're ducking already, or the shot is at our knees
	if ((tr->endpos[2] <= height) || (self->monsterinfo.aiflags & AI_DUCKED))
	{
		vec3_t right, diff;

		// if we're already dodging, just finish the sequence, i.e. don't do anything else
		if (self->monsterinfo.aiflags & AI_DODGING)
		{
			if ((g_showlogic) && (g_showlogic->value))
				gi.dprintf ("already dodging\n");
			return;
		}

		AngleVectors (self->s.angles, NULL, right, NULL);
		VectorSubtract (tr->endpos, self->s.origin, diff);

		if (DotProduct (right, diff) < 0)
		{
			self->monsterinfo.lefty = 1;
//			gi.dprintf ("left\n");
		} else {
//			gi.dprintf ("right\n");
		}
		// if it doesn't sense to duck, try to strafe and shoot
		// we don't want the machine gun guys running & shooting (looks bad)

		// if we are currently ducked, unduck
		if (self->monsterinfo.aiflags & AI_DUCKED)
		{
			if ((g_showlogic) && (g_showlogic->value))
				gi.dprintf ("unducking - ");
			soldier_duck_up(self);
		}

		self->monsterinfo.aiflags |= AI_DODGING;
		self->monsterinfo.attack_state = AS_SLIDING;

		if (self->s.skinnum <= 3)
		{
			if ((g_showlogic) && (g_showlogic->value))
				gi.dprintf ("shooting back!\n");
			self->monsterinfo.currentmove = &soldier_move_attack6;
		}
		else
		{
			if ((g_showlogic) && (g_showlogic->value))
				gi.dprintf ("strafing away!\n");
			self->monsterinfo.currentmove = &soldier_move_start_run;
		}
		return;
	}

	// if we're here, we're ducking, so clear the dodge bit if it's set

	if ((g_showlogic) && (g_showlogic->value))
		gi.dprintf ("ducking!\n");
	if (skill->value == 0)
	{
		// set this prematurely; it doesn't hurt, and prevents extra iterations
		self->monsterinfo.aiflags |= AI_DUCKED;
		monster_done_dodge (self);
		self->monsterinfo.currentmove = &soldier_move_duck;
		// PMM - stupid dodge
		self->monsterinfo.duck_wait_time = level.time + eta + 1;
		return;
	}
// PMM - since we're only ducking some of the time, this needs to be moved down below
//	self->monsterinfo.duck_wait_time = level.time + eta + 0.3;

	r = random();

	// set this prematurely; it doesn't hurt, and prevents extra iterations
	self->monsterinfo.aiflags |= AI_DUCKED;
	monster_done_dodge (self);

	if (r > (skill->value * 0.33))
	{
		self->monsterinfo.currentmove = &soldier_move_duck;
		self->monsterinfo.duck_wait_time = level.time + eta + (0.1 * (3 - skill->value));
		// has to be done immediately otherwise he can get stuck
		soldier_duck_down(self);
	}
	else
	{
		// has to be done immediately otherwise he can get stuck
		soldier_duck_down(self);
		self->monsterinfo.duck_wait_time = level.time + eta + 1;
		self->monsterinfo.currentmove = &soldier_move_attack3;
		self->monsterinfo.nextframe = FRAME_attak301;
	}
	return;
//PMM
//===========

}
*/
// pmm - blocking code

qboolean soldier_blocked (edict_t *self, float dist)
{
	// don't do anything if you're dodging
	if ((self->monsterinfo.aiflags & AI_DODGING) || (self->monsterinfo.aiflags & AI_DUCKED))
		return false;

	if(blocked_checkshot (self, 0.25 + (0.05 * skill->value) ))
		return true;

//	if(blocked_checkjump (self, dist, 192, 40))
//	{
//		soldier_jump(self);
//		return true;
//	}

	if(blocked_checkplat (self, dist))
		return true;

	return false;
}

//
// DEATH
//

void soldier_fire6 (edict_t *self)
{
	soldier_fire (self, 5);
}

void soldier_fire7 (edict_t *self)
{
	soldier_fire (self, 6);
}

void soldier_dead (edict_t *self)
{
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity (self);
}

// pmm - this quickie does a location trace to try to grow the bounding box
//
// this is because the frames are off; the origin is at the guy's feet.
void soldier_dead2 (edict_t *self)
{
	vec3_t	tempmins, tempmaxs, temporg;
	trace_t	tr;

	VectorCopy (self->s.origin, temporg);
	// this is because location traces done at the floor are guaranteed to hit the floor
	// (inside the sv_trace code it grows the bbox by 1 in all directions)
	temporg[2] += 1;

	VectorSet (tempmins, -32, -32, -24);
	VectorSet (tempmaxs, 32, 32, -8);

	tr = gi.trace (temporg, tempmins, tempmaxs, temporg, self, MASK_SOLID);
	if (tr.startsolid || tr.allsolid)
	{
		VectorSet (self->mins, -16, -16, -24);
		VectorSet (self->maxs, 16, 16, -8);
	}
	else
	{
		VectorCopy (tempmins, self->mins);
		VectorCopy (tempmaxs, self->maxs);
	}
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity (self);
}

mframe_t soldier_frames_death1 [] =
{
	ai_move, 0,   NULL,
	ai_move, -10, NULL,
	ai_move, -10, NULL,
	ai_move, -10, NULL,
	ai_move, -5,  NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   soldier_fire6,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   soldier_fire7,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL
};
mmove_t soldier_move_death1 = {FRAME_death101, FRAME_death136, soldier_frames_death1, soldier_dead};

mframe_t soldier_frames_death2 [] =
{
	ai_move, -5,  NULL,
	ai_move, -5,  NULL,
	ai_move, -5,  NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL
};
mmove_t soldier_move_death2 = {FRAME_death201, FRAME_death235, soldier_frames_death2, soldier_dead};

mframe_t soldier_frames_death3 [] =
{
	ai_move, -5,  NULL,
	ai_move, -5,  NULL,
	ai_move, -5,  NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
};
mmove_t soldier_move_death3 = {FRAME_death301, FRAME_death345, soldier_frames_death3, soldier_dead};

mframe_t soldier_frames_death4 [] =
{
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL
};
// PMM -changed to soldier_dead2 to get a larger bounding box
mmove_t soldier_move_death4 = {FRAME_death401, FRAME_death453, soldier_frames_death4, soldier_dead2};

mframe_t soldier_frames_death5 [] =
{
	ai_move, -5,  NULL,
	ai_move, -5,  NULL,
	ai_move, -5,  NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,

	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL
};
mmove_t soldier_move_death5 = {FRAME_death501, FRAME_death524, soldier_frames_death5, soldier_dead};

mframe_t soldier_frames_death6 [] =
{
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL
};
mmove_t soldier_move_death6 = {FRAME_death601, FRAME_death610, soldier_frames_death6, soldier_dead};

void soldier_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		n;

// check for gib
	if (self->health <= self->gib_health)
	{
		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n= 0; n < 3; n++)
			ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowGib (self, "models/objects/gibs/chest/tris.md2", damage, GIB_ORGANIC);
		ThrowHead (self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

// regular death
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->s.skinnum |= 1;

	if (self->s.skinnum == 1)
		gi.sound (self, CHAN_VOICE, sound_death_light, 1, ATTN_NORM, 0);
	else if (self->s.skinnum == 3)
		gi.sound (self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	else // (self->s.skinnum == 5)
		gi.sound (self, CHAN_VOICE, sound_death_ss, 1, ATTN_NORM, 0);

	if (fabs((self->s.origin[2] + self->viewheight) - point[2]) <= 4)
	{
		// head shot
		self->monsterinfo.currentmove = &soldier_move_death3;
		return;
	}

	n = rand() % 5;
	if (n == 0)
		self->monsterinfo.currentmove = &soldier_move_death1;
	else if (n == 1)
		self->monsterinfo.currentmove = &soldier_move_death2;
	else if (n == 2)
		self->monsterinfo.currentmove = &soldier_move_death4;
	else if (n == 3)
		self->monsterinfo.currentmove = &soldier_move_death5;
	else
		self->monsterinfo.currentmove = &soldier_move_death6;
}

//
// NEW DODGE CODE
//

void soldier_sidestep (edict_t *self)
{
	if (self->s.skinnum <= 3)
	{
//		if ((g_showlogic) && (g_showlogic->value))
//			gi.dprintf ("shooting back!\n");
		if (self->monsterinfo.currentmove != &soldier_move_attack6)
			self->monsterinfo.currentmove = &soldier_move_attack6;
	}
	else
	{
//		if ((g_showlogic) && (g_showlogic->value))
//			gi.dprintf ("strafing away!\n");
		if (self->monsterinfo.currentmove != &soldier_move_start_run)
			self->monsterinfo.currentmove = &soldier_move_start_run;
	}
}

void soldier_duck (edict_t *self, float eta)
{
	float r;

	// has to be done immediately otherwise he can get stuck
	monster_duck_down(self);

	if (skill->value == 0)
	{
		// PMM - stupid dodge
		self->monsterinfo.nextframe = FRAME_duck01;
		self->monsterinfo.currentmove = &soldier_move_duck;
		self->monsterinfo.duck_wait_time = level.time + eta + 1;
		return;
	}

	r = random();

	if (r > (skill->value * 0.3))
	{
		self->monsterinfo.nextframe = FRAME_duck01;
		self->monsterinfo.currentmove = &soldier_move_duck;
		self->monsterinfo.duck_wait_time = level.time + eta + (0.1 * (3 - skill->value));
	}
	else
	{
		self->monsterinfo.nextframe = FRAME_attak301;
		self->monsterinfo.currentmove = &soldier_move_attack3;
		self->monsterinfo.duck_wait_time = level.time + eta + 1;
	}
	return;
}

//=========
//ROGUE
void soldier_blind (edict_t *self);

mframe_t soldier_frames_blind [] =
{
	ai_move, 0, soldier_idle,
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
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t soldier_move_blind = {FRAME_stand101, FRAME_stand130, soldier_frames_blind, soldier_blind};

void soldier_blind (edict_t *self)
{
	self->monsterinfo.currentmove = &soldier_move_blind;
}
//ROGUE
//=========

//
// SPAWN
//

void SP_monster_soldier_x (edict_t *self)
{

	self->s.modelindex = gi.modelindex ("models/monsters/soldier/tris.md2");
	//PMM
//	self->s.effects |= EF_SPLATTER;
	//PMM
	self->monsterinfo.scale = MODEL_SCALE;
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, 32);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	sound_idle =	gi.soundindex ("soldier/solidle1.wav");
	sound_sight1 =	gi.soundindex ("soldier/solsght1.wav");
	sound_sight2 =	gi.soundindex ("soldier/solsrch1.wav");
	sound_cock =	gi.soundindex ("infantry/infatck3.wav");

	self->mass = 100;

	self->pain = soldier_pain;
	self->die = soldier_die;

	self->monsterinfo.stand = soldier_stand;
	self->monsterinfo.walk = soldier_walk;
	self->monsterinfo.run = soldier_run;
	self->monsterinfo.dodge = M_MonsterDodge;
	self->monsterinfo.attack = soldier_attack;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = soldier_sight;

//=====
//ROGUE
	self->monsterinfo.blocked = soldier_blocked;
	self->monsterinfo.duck = soldier_duck;
	self->monsterinfo.unduck = monster_duck_up;
	self->monsterinfo.sidestep = soldier_sidestep;

	if(self->spawnflags & 8)	// blind
		self->monsterinfo.stand = soldier_blind;
//ROGUE
//=====

	gi.linkentity (self);

	self->monsterinfo.stand (self);

	walkmonster_start (self);
}


/*QUAKED monster_soldier_light (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight Blind

Blind - monster will just stand there until triggered
*/
void SP_monster_soldier_light (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	SP_monster_soldier_x (self);

	sound_pain_light = gi.soundindex ("soldier/solpain2.wav");
	sound_death_light =	gi.soundindex ("soldier/soldeth2.wav");
	gi.modelindex ("models/objects/laser/tris.md2");
	gi.soundindex ("misc/lasfly.wav");
	gi.soundindex ("soldier/solatck2.wav");

	self->s.skinnum = 0;
	self->health = 20;
	self->gib_health = -30;

	// PMM - blindfire
	self->monsterinfo.blindfire = true;
}

/*QUAKED monster_soldier (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight Blind

Blind - monster will just stand there until triggered
*/
void SP_monster_soldier (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	SP_monster_soldier_x (self);

	sound_pain = gi.soundindex ("soldier/solpain1.wav");
	sound_death = gi.soundindex ("soldier/soldeth1.wav");
	gi.soundindex ("soldier/solatck1.wav");

	self->s.skinnum = 2;
	self->health = 30;
	self->gib_health = -30;
}

/*QUAKED monster_soldier_ss (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight Blind

Blind - monster will just stand there until triggered
*/
void SP_monster_soldier_ss (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	SP_monster_soldier_x (self);

	sound_pain_ss = gi.soundindex ("soldier/solpain3.wav");
	sound_death_ss = gi.soundindex ("soldier/soldeth3.wav");
	gi.soundindex ("soldier/solatck3.wav");

	self->s.skinnum = 4;
	self->health = 40;
	self->gib_health = -30;
}
