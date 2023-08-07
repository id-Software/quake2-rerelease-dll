// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

GUNNER

==============================================================================
*/

#include "g_local.h"
#include "m_gunner.h"


static int	sound_pain;
static int	sound_pain2;
static int	sound_death;
static int	sound_idle;
static int	sound_open;
static int	sound_search;
static int	sound_sight;

void gunner_idlesound (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

void gunner_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void gunner_search (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}


qboolean visible (edict_t *self, edict_t *other);
void GunnerGrenade (edict_t *self);
void GunnerFire (edict_t *self);
void gunner_fire_chain(edict_t *self);
void gunner_refire_chain(edict_t *self);


void gunner_stand (edict_t *self);

mframe_t gunner_frames_fidget [] =
{
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, gunner_idlesound,
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
	ai_stand, 0, NULL
};
mmove_t	gunner_move_fidget = {FRAME_stand31, FRAME_stand70, gunner_frames_fidget, gunner_stand};

void gunner_fidget (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		return;
	if (random() <= 0.05)
		self->monsterinfo.currentmove = &gunner_move_fidget;
}

mframe_t gunner_frames_stand [] =
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
	ai_stand, 0, gunner_fidget,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, gunner_fidget,

	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, gunner_fidget
};
mmove_t	gunner_move_stand = {FRAME_stand01, FRAME_stand30, gunner_frames_stand, NULL};

void gunner_stand (edict_t *self)
{
		self->monsterinfo.currentmove = &gunner_move_stand;
}


mframe_t gunner_frames_walk [] =
{
	ai_walk, 0, NULL,
	ai_walk, 3, NULL,
	ai_walk, 4, NULL,
	ai_walk, 5, NULL,
	ai_walk, 7, NULL,
	ai_walk, 2, NULL,
	ai_walk, 6, NULL,
	ai_walk, 4, NULL,
	ai_walk, 2, NULL,
	ai_walk, 7, NULL,
	ai_walk, 5, NULL,
	ai_walk, 7, NULL,
	ai_walk, 4, NULL
};
mmove_t gunner_move_walk = {FRAME_walk07, FRAME_walk19, gunner_frames_walk, NULL};

void gunner_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &gunner_move_walk;
}

mframe_t gunner_frames_run [] =
{
	ai_run, 26, NULL,
	ai_run, 9,  NULL,
	ai_run, 9,  NULL,
	ai_run, 9,  monster_done_dodge,
	ai_run, 15, NULL,
	ai_run, 10, NULL,
	ai_run, 13, NULL,
	ai_run, 6,  NULL
};

mmove_t gunner_move_run = {FRAME_run01, FRAME_run08, gunner_frames_run, NULL};

void gunner_run (edict_t *self)
{
	monster_done_dodge(self);
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &gunner_move_stand;
	else
		self->monsterinfo.currentmove = &gunner_move_run;
}

mframe_t gunner_frames_runandshoot [] =
{
	ai_run, 32, NULL,
	ai_run, 15, NULL,
	ai_run, 10, NULL,
	ai_run, 18, NULL,
	ai_run, 8,  NULL,
	ai_run, 20, NULL
};

mmove_t gunner_move_runandshoot = {FRAME_runs01, FRAME_runs06, gunner_frames_runandshoot, NULL};

void gunner_runandshoot (edict_t *self)
{
	self->monsterinfo.currentmove = &gunner_move_runandshoot;
}

mframe_t gunner_frames_pain3 [] =
{
	ai_move, -3, NULL,
	ai_move, 1,	 NULL,
	ai_move, 1,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 1,	 NULL
};
mmove_t gunner_move_pain3 = {FRAME_pain301, FRAME_pain305, gunner_frames_pain3, gunner_run};

mframe_t gunner_frames_pain2 [] =
{
	ai_move, -2, NULL,
	ai_move, 11, NULL,
	ai_move, 6,	 NULL,
	ai_move, 2,	 NULL,
	ai_move, -1, NULL,
	ai_move, -7, NULL,
	ai_move, -2, NULL,
	ai_move, -7, NULL
};
mmove_t gunner_move_pain2 = {FRAME_pain201, FRAME_pain208, gunner_frames_pain2, gunner_run};

mframe_t gunner_frames_pain1 [] =
{
	ai_move, 2,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, -5, NULL,
	ai_move, 3,	 NULL,
	ai_move, -1, NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 1,	 NULL,
	ai_move, 1,	 NULL,
	ai_move, 2,	 NULL,
	ai_move, 1,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, -2, NULL,
	ai_move, -2, NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL
};
mmove_t gunner_move_pain1 = {FRAME_pain101, FRAME_pain118, gunner_frames_pain1, gunner_run};

void gunner_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	monster_done_dodge (self);

	if (!self->groundentity)
	{
//		if ((g_showlogic) && (g_showlogic->value))
//			gi.dprintf ("gunner: pain avoided due to no ground\n");
		return;
	}

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;

	if (rand()&1)
		gi.sound (self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
	else
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

	if (skill->value == 3)
		return;		// no pain anims in nightmare

	if (damage <= 10)
		self->monsterinfo.currentmove = &gunner_move_pain3;
	else if (damage <= 25)
		self->monsterinfo.currentmove = &gunner_move_pain2;
	else
		self->monsterinfo.currentmove = &gunner_move_pain1;

	self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;

	// PMM - clear duck flag
	if (self->monsterinfo.aiflags & AI_DUCKED)
		monster_duck_up(self);
}

void gunner_dead (edict_t *self)
{
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity (self);
}

mframe_t gunner_frames_death [] =
{
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, -7, NULL,
	ai_move, -3, NULL,
	ai_move, -5, NULL,
	ai_move, 8,	 NULL,
	ai_move, 6,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL
};
mmove_t gunner_move_death = {FRAME_death01, FRAME_death11, gunner_frames_death, gunner_dead};

void gunner_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		n;

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
	self->monsterinfo.currentmove = &gunner_move_death;
}

// PMM - changed to duck code for new dodge

//
// this is specific to the gunner, leave it be
//
void gunner_duck_down (edict_t *self)
{
//	if (self->monsterinfo.aiflags & AI_DUCKED)
//		return;
	self->monsterinfo.aiflags |= AI_DUCKED;
	if (skill->value >= 2)
	{
		if (random() > 0.5)
			GunnerGrenade (self);
	}

//	self->maxs[2] -= 32;
	self->maxs[2] = self->monsterinfo.base_height - 32;
	self->takedamage = DAMAGE_YES;
	if (self->monsterinfo.duck_wait_time < level.time)
		self->monsterinfo.duck_wait_time = level.time + 1;
	gi.linkentity (self);
}

mframe_t gunner_frames_duck [] =
{
	ai_move, 1,  gunner_duck_down,
	ai_move, 1,  NULL,
	ai_move, 1,  monster_duck_hold,
	ai_move, 0,  NULL,
	ai_move, -1, NULL,
	ai_move, -1, NULL,
	ai_move, 0,  monster_duck_up,
	ai_move, -1, NULL
};
mmove_t	gunner_move_duck = {FRAME_duck01, FRAME_duck08, gunner_frames_duck, gunner_run};

// PMM - gunner dodge moved below so I know about attack sequences

void gunner_opengun (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_open, 1, ATTN_IDLE, 0);
}

void GunnerFire (edict_t *self)
{
	vec3_t	start;
	vec3_t	forward, right;
	vec3_t	target;
	vec3_t	aim;
	int		flash_number;

	if(!self->enemy || !self->enemy->inuse)		//PGM
		return;									//PGM

	flash_number = MZ2_GUNNER_MACHINEGUN_1 + (self->s.frame - FRAME_attak216);

	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource (self->s.origin, monster_flash_offset[flash_number], forward, right, start);

	// project enemy back a bit and target there
	VectorCopy (self->enemy->s.origin, target);
	VectorMA (target, -0.2, self->enemy->velocity, target);
	target[2] += self->enemy->viewheight;

	VectorSubtract (target, start, aim);
	VectorNormalize (aim);
	monster_fire_bullet (self, start, aim, 3, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_number);
}

qboolean gunner_grenade_check(edict_t *self)
{
	vec3_t		start;
	vec3_t		forward, right;
	trace_t		tr;
	vec3_t		target, dir;

	if(!self->enemy)
		return false;

	// if the player is above my head, use machinegun.

	// check for flag telling us that we're blindfiring
	if (self->monsterinfo.aiflags & AI_MANUAL_STEERING)
	{
		if (self->s.origin[2]+self->viewheight < self->monsterinfo.blind_fire_target[2])
		{
//			if(g_showlogic && g_showlogic->value)
//				gi.dprintf("blind_fire_target is above my head, using machinegun\n");
			return false;
		}
	}
	else if(self->absmax[2] <= self->enemy->absmin[2])
	{
//		if(g_showlogic && g_showlogic->value)
//			gi.dprintf("player is above my head, using machinegun\n");
		return false;
	}

	// check to see that we can trace to the player before we start
	// tossing grenades around.
	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_GUNNER_GRENADE_1], forward, right, start);

	// pmm - check for blindfire flag
	if (self->monsterinfo.aiflags & AI_MANUAL_STEERING)
		VectorCopy (self->monsterinfo.blind_fire_target, target);
	else
		VectorCopy (self->enemy->s.origin, target);

	// see if we're too close
	VectorSubtract (self->s.origin, target, dir);

	if (VectorLength(dir) < 100)
		return false;

	tr = gi.trace(start, vec3_origin, vec3_origin, target, self, MASK_SHOT);
	if(tr.ent == self->enemy || tr.fraction == 1)
		return true;

//	if(g_showlogic && g_showlogic->value)
//		gi.dprintf("can't trace to target, using machinegun\n");
	return false;
}

void GunnerGrenade (edict_t *self)
{
	vec3_t	start;
	vec3_t	forward, right, up;
	vec3_t	aim;
	int		flash_number;
	float	spread;
	float	pitch;
	// PMM
	vec3_t	target;	
	qboolean blindfire;

	if(!self->enemy || !self->enemy->inuse)		//PGM
		return;									//PGM

	// pmm
	if (self->monsterinfo.aiflags & AI_MANUAL_STEERING)
		blindfire = true;

	if (self->s.frame == FRAME_attak105)
	{
		spread = .02;
		flash_number = MZ2_GUNNER_GRENADE_1;
	}
	else if (self->s.frame == FRAME_attak108)
	{
		spread = .05;
		flash_number = MZ2_GUNNER_GRENADE_2;
	}
	else if (self->s.frame == FRAME_attak111)
	{
		spread = .08;
		flash_number = MZ2_GUNNER_GRENADE_3;
	}
	else // (self->s.frame == FRAME_attak114)
	{
		self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
		spread = .11;
		flash_number = MZ2_GUNNER_GRENADE_4;
	}

	//	pmm
	// if we're shooting blind and we still can't see our enemy
	if ((blindfire) && (!visible(self, self->enemy)))
	{
		// and we have a valid blind_fire_target
		if (VectorCompare (self->monsterinfo.blind_fire_target, vec3_origin))
			return;
		
//		gi.dprintf ("blind_fire_target = %s\n", vtos (self->monsterinfo.blind_fire_target));
//		gi.dprintf ("GunnerGrenade: ideal yaw is %f\n", self->ideal_yaw);
		VectorCopy (self->monsterinfo.blind_fire_target, target);
	}
	else
		VectorCopy (self->s.origin, target);
	// pmm

	AngleVectors (self->s.angles, forward, right, up);	//PGM
	G_ProjectSource (self->s.origin, monster_flash_offset[flash_number], forward, right, start);

//PGM
	if(self->enemy)
	{
		float	dist;

//		VectorSubtract(self->enemy->s.origin, self->s.origin, aim);
		VectorSubtract(target, self->s.origin, aim);
		dist = VectorLength(aim);

		// aim up if they're on the same level as me and far away.
		if((dist > 512) && (aim[2] < 64) && (aim[2] > -64))
		{
			aim[2] += (dist - 512);
		}

		VectorNormalize (aim);
		pitch = aim[2];
		if(pitch > 0.4)
		{
			pitch = 0.4;
		}
		else if(pitch < -0.5)
			pitch = -0.5;
	}
//PGM

	//FIXME : do a spread -225 -75 75 225 degrees around forward
//	VectorCopy (forward, aim);
	VectorMA (forward, spread, right, aim);
	VectorMA (aim, pitch, up, aim);

	monster_fire_grenade (self, start, aim, 50, 600, flash_number);
}

mframe_t gunner_frames_attack_chain [] =
{
	/*
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	*/
	ai_charge, 0, gunner_opengun,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t gunner_move_attack_chain = {FRAME_attak209, FRAME_attak215, gunner_frames_attack_chain, gunner_fire_chain};

mframe_t gunner_frames_fire_chain [] =
{
	ai_charge,   0, GunnerFire,
	ai_charge,   0, GunnerFire,
	ai_charge,   0, GunnerFire,
	ai_charge,   0, GunnerFire,
	ai_charge,   0, GunnerFire,
	ai_charge,   0, GunnerFire,
	ai_charge,   0, GunnerFire,
	ai_charge,   0, GunnerFire
};
mmove_t gunner_move_fire_chain = {FRAME_attak216, FRAME_attak223, gunner_frames_fire_chain, gunner_refire_chain};

mframe_t gunner_frames_endfire_chain [] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t gunner_move_endfire_chain = {FRAME_attak224, FRAME_attak230, gunner_frames_endfire_chain, gunner_run};

void gunner_blind_check (edict_t *self)
{
	vec3_t	aim;

	if (self->monsterinfo.aiflags & AI_MANUAL_STEERING)
	{
		VectorSubtract(self->monsterinfo.blind_fire_target, self->s.origin, aim);
		self->ideal_yaw = vectoyaw(aim);
		
//		gi.dprintf ("blind_fire_target = %s\n", vtos (self->monsterinfo.blind_fire_target));
//		gi.dprintf ("gunner_attack: ideal yaw is %f\n", self->ideal_yaw);
	}
}

mframe_t gunner_frames_attack_grenade [] =
{
	ai_charge, 0, gunner_blind_check,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, GunnerGrenade,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, GunnerGrenade,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, GunnerGrenade,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, GunnerGrenade,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t gunner_move_attack_grenade = {FRAME_attak101, FRAME_attak121, gunner_frames_attack_grenade, gunner_run};

void gunner_attack(edict_t *self)
{
	float chance, r;

	monster_done_dodge(self);

	// PMM 
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
		if (gunner_grenade_check(self))
		{
			// if the check passes, go for the attack
			self->monsterinfo.currentmove = &gunner_move_attack_grenade;
			self->monsterinfo.attack_finished = level.time + 2*random();
		}
		// pmm - should this be active?
//		else
//			self->monsterinfo.currentmove = &gunner_move_attack_chain;
//		if ((g_showlogic) && (g_showlogic->value))
//			gi.dprintf ("blind grenade check failed, doing nothing\n");

		// turn off blindfire flag
		self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
		return;
	}
	// pmm

	// PGM - gunner needs to use his chaingun if he's being attacked by a tesla.
	if ((range (self, self->enemy) == RANGE_MELEE) || self->bad_area)
	{
		self->monsterinfo.currentmove = &gunner_move_attack_chain;
	}
	else
	{
		if (random() <= 0.5 && gunner_grenade_check(self))
			self->monsterinfo.currentmove = &gunner_move_attack_grenade;
		else
			self->monsterinfo.currentmove = &gunner_move_attack_chain;
	}
}

void gunner_fire_chain(edict_t *self)
{
	self->monsterinfo.currentmove = &gunner_move_fire_chain;
}

void gunner_refire_chain(edict_t *self)
{
	if (self->enemy->health > 0)
		if ( visible (self, self->enemy) )
			if (random() <= 0.5)
			{
				self->monsterinfo.currentmove = &gunner_move_fire_chain;
				return;
			}
	self->monsterinfo.currentmove = &gunner_move_endfire_chain;
}
/*
void gunner_dodge (edict_t *self, edict_t *attacker, float eta, trace_t *tr)
{
// original quake2 dodge code

	if (random() > 0.25)
		return;

	if (!self->enemy)
		self->enemy = attacker;

	self->monsterinfo.currentmove = &gunner_move_duck;

//===========
//PMM - rogue rewrite of gunner dodge code.
	float	r;
	float	height;
	int		shooting = 0;

	if (!self->enemy)
	{
		self->enemy = attacker;
		FoundTarget (self);
	}

	// PMM - don't bother if it's going to hit anyway; fix for weird in-your-face etas (I was
	// seeing numbers like 13 and 14)
	if ((eta < 0.1) || (eta > 5))
		return;

	r = random();
	if (r > (0.25*((skill->value)+1)))
		return;

	if ((self->monsterinfo.currentmove == &gunner_move_attack_chain) ||
		(self->monsterinfo.currentmove == &gunner_move_fire_chain) ||
		(self->monsterinfo.currentmove == &gunner_move_attack_grenade)
		)
	{
		shooting = 1;
	}
	if (self->monsterinfo.aiflags & AI_DODGING)
	{
		height = self->absmax[2];
	}
	else
	{
		height = self->absmax[2]-32-1;  // the -1 is because the absmax is s.origin + maxs + 1
	}

	// check to see if it makes sense to duck
	if (tr->endpos[2] <= height)
	{
		vec3_t right, diff;
		if (shooting)
		{
			self->monsterinfo.attack_state = AS_SLIDING;
			return;
		}
		AngleVectors (self->s.angles, NULL, right, NULL);
		VectorSubtract (tr->endpos, self->s.origin, diff);
		if (DotProduct (right, diff) < 0)
		{
			self->monsterinfo.lefty = 1;
		}
		// if it doesn't sense to duck, try to strafe away
		monster_done_dodge (self);
		self->monsterinfo.currentmove = &gunner_move_run;
		self->monsterinfo.attack_state = AS_SLIDING;
		return;
	}

	if (skill->value == 0)
	{
		self->monsterinfo.currentmove = &gunner_move_duck;
		// PMM - stupid dodge
		self->monsterinfo.duck_wait_time = level.time + eta + 1;
		self->monsterinfo.aiflags |= AI_DODGING;
		return;
	}

	if (!shooting)
	{
		self->monsterinfo.currentmove = &gunner_move_duck;
		self->monsterinfo.duck_wait_time = level.time + eta + (0.1 * (3 - skill->value));
		self->monsterinfo.aiflags |= AI_DODGING;
	}
	return;
//PMM
//===========
}
*/
//===========
//PGM
void gunner_jump_now (edict_t *self)
{
	vec3_t	forward,up;

	monster_jump_start (self);

	AngleVectors (self->s.angles, forward, NULL, up);
	VectorMA(self->velocity, 100, forward, self->velocity);
	VectorMA(self->velocity, 300, up, self->velocity);
}

void gunner_jump2_now (edict_t *self)
{
	vec3_t	forward,up;

	monster_jump_start (self);

	AngleVectors (self->s.angles, forward, NULL, up);
	VectorMA(self->velocity, 150, forward, self->velocity);
	VectorMA(self->velocity, 400, up, self->velocity);
}

void gunner_jump_wait_land (edict_t *self)
{
	if(self->groundentity == NULL)
	{
		self->monsterinfo.nextframe = self->s.frame;

		if(monster_jump_finished (self))
			self->monsterinfo.nextframe = self->s.frame + 1;
	}
	else 
		self->monsterinfo.nextframe = self->s.frame + 1;
}

mframe_t gunner_frames_jump [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, gunner_jump_now,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, gunner_jump_wait_land,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t gunner_move_jump = { FRAME_jump01, FRAME_jump10, gunner_frames_jump, gunner_run };

mframe_t gunner_frames_jump2 [] =
{
	ai_move, -8, NULL,
	ai_move, -4, NULL,
	ai_move, -4, NULL,
	ai_move, 0, gunner_jump_now,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, gunner_jump_wait_land,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t gunner_move_jump2 = { FRAME_jump01, FRAME_jump10, gunner_frames_jump2, gunner_run };

void gunner_jump (edict_t *self)
{
	if(!self->enemy)
		return;

	monster_done_dodge (self);

	if(self->enemy->s.origin[2] > self->s.origin[2])
		self->monsterinfo.currentmove = &gunner_move_jump2;
	else
		self->monsterinfo.currentmove = &gunner_move_jump;
}

//===========
//PGM
qboolean gunner_blocked (edict_t *self, float dist)
{
	if(blocked_checkshot (self, 0.25 + (0.05 * skill->value) ))
		return true;

	if(blocked_checkplat (self, dist))
		return true;

	if(blocked_checkjump (self, dist, 192, 40))
	{
		gunner_jump(self);
		return true;
	}

	return false;
}
//PGM
//===========

// PMM - new duck code
void gunner_duck (edict_t *self, float eta)
{
	if ((self->monsterinfo.currentmove == &gunner_move_jump2) ||
		(self->monsterinfo.currentmove == &gunner_move_jump))
	{
		return;
	}

	if ((self->monsterinfo.currentmove == &gunner_move_attack_chain) ||
		(self->monsterinfo.currentmove == &gunner_move_fire_chain) ||
		(self->monsterinfo.currentmove == &gunner_move_attack_grenade)
		)
	{
		// if we're shooting, and not on easy, don't dodge
		if (skill->value)
		{
			self->monsterinfo.aiflags &= ~AI_DUCKED;
			return;
		}
	}

	if (skill->value == 0)
		// PMM - stupid dodge
		self->monsterinfo.duck_wait_time = level.time + eta + 1;
	else
		self->monsterinfo.duck_wait_time = level.time + eta + (0.1 * (3 - skill->value));

	// has to be done immediately otherwise he can get stuck
	gunner_duck_down(self);

	self->monsterinfo.nextframe = FRAME_duck01;
	self->monsterinfo.currentmove = &gunner_move_duck;
	return;
}

void gunner_sidestep (edict_t *self)
{
	if ((self->monsterinfo.currentmove == &gunner_move_jump2) ||
		(self->monsterinfo.currentmove == &gunner_move_jump))
	{
		return;
	}

	if ((self->monsterinfo.currentmove == &gunner_move_attack_chain) ||
		(self->monsterinfo.currentmove == &gunner_move_fire_chain) ||
		(self->monsterinfo.currentmove == &gunner_move_attack_grenade)
		)
	{
		// if we're shooting, and not on easy, don't dodge
		if (skill->value)
		{
			self->monsterinfo.aiflags &= ~AI_DODGING;
			return;
		}
	}

	if (self->monsterinfo.currentmove != &gunner_move_run)
		self->monsterinfo.currentmove = &gunner_move_run;
}


/*QUAKED monster_gunner (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void SP_monster_gunner (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	sound_death = gi.soundindex ("gunner/death1.wav");	
	sound_pain = gi.soundindex ("gunner/gunpain2.wav");	
	sound_pain2 = gi.soundindex ("gunner/gunpain1.wav");	
	sound_idle = gi.soundindex ("gunner/gunidle1.wav");	
	sound_open = gi.soundindex ("gunner/gunatck1.wav");	
	sound_search = gi.soundindex ("gunner/gunsrch1.wav");	
	sound_sight = gi.soundindex ("gunner/sight1.wav");	

	gi.soundindex ("gunner/gunatck2.wav");
	gi.soundindex ("gunner/gunatck3.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex ("models/monsters/gunner/tris.md2");
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, 32);

	self->health = 175;
	self->gib_health = -70;
	self->mass = 200;

	self->pain = gunner_pain;
	self->die = gunner_die;

	self->monsterinfo.stand = gunner_stand;
	self->monsterinfo.walk = gunner_walk;
	self->monsterinfo.run = gunner_run;
	// pmm
	self->monsterinfo.dodge = M_MonsterDodge;
	self->monsterinfo.duck = gunner_duck;
	self->monsterinfo.unduck = monster_duck_up;
	self->monsterinfo.sidestep = gunner_sidestep;
//	self->monsterinfo.dodge = gunner_dodge;
	// pmm
	self->monsterinfo.attack = gunner_attack;
	self->monsterinfo.melee = NULL;
	self->monsterinfo.sight = gunner_sight;
	self->monsterinfo.search = gunner_search;
	self->monsterinfo.blocked = gunner_blocked;		//PGM

	gi.linkentity (self);

	self->monsterinfo.currentmove = &gunner_move_stand;	
	self->monsterinfo.scale = MODEL_SCALE;

	// PMM
	self->monsterinfo.blindfire = true;

	walkmonster_start (self);
}
