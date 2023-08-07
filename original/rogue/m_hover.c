// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

hover

==============================================================================
*/

#include "g_local.h"
#include "m_hover.h"

qboolean visible (edict_t *self, edict_t *other);


static int	sound_pain1;
static int	sound_pain2;
static int	sound_death1;
static int	sound_death2;
static int	sound_sight;
static int	sound_search1;
static int	sound_search2;

// daedalus sounds
static int	daed_sound_pain1;
static int	daed_sound_pain2;
static int	daed_sound_death1;
static int	daed_sound_death2;
static int	daed_sound_sight;
static int	daed_sound_search1;
static int	daed_sound_search2;


void hover_sight (edict_t *self, edict_t *other)
{
	// PMM - daedalus sounds
	if (self->mass < 225)
		gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
	else
		gi.sound (self, CHAN_VOICE, daed_sound_sight, 1, ATTN_NORM, 0);
}

void hover_search (edict_t *self)
{
	// PMM - daedalus sounds
	if (self->mass < 225)
	{
		if (random() < 0.5)
			gi.sound (self, CHAN_VOICE, sound_search1, 1, ATTN_NORM, 0);
		else
			gi.sound (self, CHAN_VOICE, sound_search2, 1, ATTN_NORM, 0);
	}
	else
	{
		if (random() < 0.5)
			gi.sound (self, CHAN_VOICE, daed_sound_search1, 1, ATTN_NORM, 0);
		else
			gi.sound (self, CHAN_VOICE, daed_sound_search2, 1, ATTN_NORM, 0);
	}
}


void hover_run (edict_t *self);
void hover_stand (edict_t *self);
void hover_dead (edict_t *self);
void hover_attack (edict_t *self);
void hover_reattack (edict_t *self);
void hover_fire_blaster (edict_t *self);
void hover_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

mframe_t hover_frames_stand [] =
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
mmove_t	hover_move_stand = {FRAME_stand01, FRAME_stand30, hover_frames_stand, NULL};
/*
mframe_t hover_frames_stop1 [] =
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
mmove_t hover_move_stop1 = {FRAME_stop101, FRAME_stop109, hover_frames_stop1, NULL};

mframe_t hover_frames_stop2 [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL
};
mmove_t hover_move_stop2 = {FRAME_stop201, FRAME_stop208, hover_frames_stop2, NULL};
*/
/*
mframe_t hover_frames_takeoff [] =
{
	ai_move,	0,	NULL,
	ai_move,	-2,	NULL,
	ai_move,	5,	NULL,
	ai_move,	-1,	NULL,
	ai_move,	1,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	-1,	NULL,
	ai_move,	-1,	NULL,
	ai_move,	-1,	NULL,
	ai_move,	0,	NULL,
	ai_move,	2,	NULL,
	ai_move,	2,	NULL,
	ai_move,	1,	NULL,
	ai_move,	1,	NULL,
	ai_move,	-6,	NULL,
	ai_move,	-9,	NULL,
	ai_move,	1,	NULL,
	ai_move,	0,	NULL,
	ai_move,	2,	NULL,
	ai_move,	2,	NULL,
	ai_move,	1,	NULL,
	ai_move,	1,	NULL,
	ai_move,	1,	NULL,
	ai_move,	2,	NULL,
	ai_move,	0,	NULL,
	ai_move,	2,	NULL,
	ai_move,	3,	NULL,
	ai_move,	2,	NULL,
	ai_move,	0,	NULL
};
mmove_t hover_move_takeoff = {FRAME_takeof01, FRAME_takeof30, hover_frames_takeoff, NULL};
*/
mframe_t hover_frames_pain3 [] =
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
mmove_t hover_move_pain3 = {FRAME_pain301, FRAME_pain309, hover_frames_pain3, hover_run};

mframe_t hover_frames_pain2 [] =
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
	ai_move,	0,	NULL
};
mmove_t hover_move_pain2 = {FRAME_pain201, FRAME_pain212, hover_frames_pain2, hover_run};

mframe_t hover_frames_pain1 [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	2,	NULL,
	ai_move,	-8,	NULL,
	ai_move,	-4,	NULL,
	ai_move,	-6,	NULL,
	ai_move,	-4,	NULL,
	ai_move,	-3,	NULL,
	ai_move,	1,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	3,	NULL,
	ai_move,	1,	NULL,
	ai_move,	0,	NULL,
	ai_move,	2,	NULL,
	ai_move,	3,	NULL,
	ai_move,	2,	NULL,
	ai_move,	7,	NULL,
	ai_move,	1,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	2,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	5,	NULL,
	ai_move,	3,	NULL,
	ai_move,	4,	NULL
};
mmove_t hover_move_pain1 = {FRAME_pain101, FRAME_pain128, hover_frames_pain1, hover_run};

/*
mframe_t hover_frames_land [] =
{
	ai_move,	0,	NULL
};
mmove_t hover_move_land = {FRAME_land01, FRAME_land01, hover_frames_land, NULL};
*/
/*
mframe_t hover_frames_forward [] =
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
	ai_move,	0,	NULL
};
mmove_t hover_move_forward = {FRAME_forwrd01, FRAME_forwrd35, hover_frames_forward, NULL};
*/
mframe_t hover_frames_walk [] =
{
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL,
	ai_walk,	4,	NULL
};
mmove_t hover_move_walk = {FRAME_forwrd01, FRAME_forwrd35, hover_frames_walk, NULL};

mframe_t hover_frames_run [] =
{
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
mmove_t hover_move_run = {FRAME_forwrd01, FRAME_forwrd35, hover_frames_run, NULL};

mframe_t hover_frames_death1 [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	-10,NULL,
	ai_move,	3,	NULL,
	ai_move,	5,	NULL,
	ai_move,	4,	NULL,
	ai_move,	7,	NULL
};
mmove_t hover_move_death1 = {FRAME_death101, FRAME_death111, hover_frames_death1, hover_dead};
/*
mframe_t hover_frames_backward [] =
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
	ai_move,	0,	NULL
};
mmove_t hover_move_backward = {FRAME_backwd01, FRAME_backwd24, hover_frames_backward, NULL};
*/
mframe_t hover_frames_start_attack [] =
{
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL
};
mmove_t hover_move_start_attack = {FRAME_attak101, FRAME_attak103, hover_frames_start_attack, hover_attack};

mframe_t hover_frames_attack1 [] =
{
	ai_charge,	-10,	hover_fire_blaster,
	ai_charge,	-10,	hover_fire_blaster,
	ai_charge,	0,		hover_reattack,
};
mmove_t hover_move_attack1 = {FRAME_attak104, FRAME_attak106, hover_frames_attack1, NULL};


mframe_t hover_frames_end_attack [] =
{
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL
};
mmove_t hover_move_end_attack = {FRAME_attak107, FRAME_attak108, hover_frames_end_attack, hover_run};

/* PMM - circle strafing code */

mframe_t hover_frames_start_attack2 [] =
{
	ai_charge,	15,	NULL,
	ai_charge,	15,	NULL,
	ai_charge,	15,	NULL
};
mmove_t hover_move_start_attack2 = {FRAME_attak101, FRAME_attak103, hover_frames_start_attack2, hover_attack};

mframe_t hover_frames_attack2 [] =
{
	ai_charge,	10,	hover_fire_blaster,
	ai_charge,	10,	hover_fire_blaster,
	ai_charge,	10,		hover_reattack,
};
mmove_t hover_move_attack2 = {FRAME_attak104, FRAME_attak106, hover_frames_attack2, NULL};


mframe_t hover_frames_end_attack2 [] =
{
	ai_charge,	15,	NULL,
	ai_charge,	15,	NULL
};
mmove_t hover_move_end_attack2 = {FRAME_attak107, FRAME_attak108, hover_frames_end_attack2, hover_run};

// end of circle strafe


void hover_reattack (edict_t *self)
{
	if (self->enemy->health > 0 )
		if (visible (self, self->enemy) )
			if (random() <= 0.6)		
			{
				if (self->monsterinfo.attack_state == AS_STRAIGHT)
				{
					self->monsterinfo.currentmove = &hover_move_attack1;
					return;
				}
				else if (self->monsterinfo.attack_state == AS_SLIDING)
				{
					self->monsterinfo.currentmove = &hover_move_attack2;
					return;
				}
				else
					gi.dprintf ("hover_reattack: unexpected state %d\n", self->monsterinfo.attack_state);
			}
	self->monsterinfo.currentmove = &hover_move_end_attack;
}


void hover_fire_blaster (edict_t *self)
{
	vec3_t	start;
	vec3_t	forward, right;
	vec3_t	end;
	vec3_t	dir;
	int		effect;

	if(!self->enemy || !self->enemy->inuse)		//PGM
		return;									//PGM

	if (self->s.frame == FRAME_attak104)
		effect = EF_HYPERBLASTER;
	else
		effect = 0;

	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_HOVER_BLASTER_1], forward, right, start);

	VectorCopy (self->enemy->s.origin, end);
	end[2] += self->enemy->viewheight;
	VectorSubtract (end, start, dir);

//PGM	- daedalus fires blaster2
	if(self->mass < 200)
		monster_fire_blaster (self, start, dir, 1, 1000, MZ2_HOVER_BLASTER_1, effect);
	else
		monster_fire_blaster2 (self, start, dir, 1, 1000, MZ2_DAEDALUS_BLASTER, EF_BLASTER);
		// fixme - different muzzle flash
//PGM
}


void hover_stand (edict_t *self)
{
		self->monsterinfo.currentmove = &hover_move_stand;
}

void hover_run (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &hover_move_stand;
	else
		self->monsterinfo.currentmove = &hover_move_run;
}

void hover_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &hover_move_walk;
}

void hover_start_attack (edict_t *self)
{
	self->monsterinfo.currentmove = &hover_move_start_attack;
}

void hover_attack(edict_t *self)
{
	float chance;
/*	if (random() <= 0.5)	
		self->monsterinfo.currentmove = &flyer_move_attack1;
	else */
	// 0% chance of circle in easy
	// 50% chance in normal
	// 75% chance in hard
	// 86.67% chance in nightmare
	if (!skill->value)
		chance = 0;
	else
		chance = 1.0 - (0.5/(float)(skill->value));

	if (self->mass > 150)  // the daedalus strafes more
		chance += 0.1;

	if (random() > chance)
	{
		self->monsterinfo.currentmove = &hover_move_attack1;
		self->monsterinfo.attack_state = AS_STRAIGHT;
	}
	else // circle strafe
	{
		if (random () <= 0.5) // switch directions
			self->monsterinfo.lefty = 1 - self->monsterinfo.lefty;
		self->monsterinfo.currentmove = &hover_move_attack2;
		self->monsterinfo.attack_state = AS_SLIDING;
	}
}


void hover_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;	// PGM support for skins 2 & 3.

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;

	if (skill->value == 3)
		return;		// no pain anims in nightmare

	if (damage <= 25)
	{
		if (random() < 0.5)
		{
			// PMM - daedalus sounds
			if (self->mass < 225)
				gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
			else
				gi.sound (self, CHAN_VOICE, daed_sound_pain1, 1, ATTN_NORM, 0);
			self->monsterinfo.currentmove = &hover_move_pain3;
		}
		else
		{
			// PMM - daedalus sounds
			if (self->mass < 225)
				gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
			else
				gi.sound (self, CHAN_VOICE, daed_sound_pain2, 1, ATTN_NORM, 0);
			self->monsterinfo.currentmove = &hover_move_pain2;
		}
	}
	else
	{
//====
//PGM pain sequence is WAY too long
		if (random() < (0.45 - (0.1 * skill->value)))
		{
			// PMM - daedalus sounds
			if (self->mass < 225)
				gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
			else
				gi.sound (self, CHAN_VOICE, daed_sound_pain1, 1, ATTN_NORM, 0);
			self->monsterinfo.currentmove = &hover_move_pain1;
		}
		else
		{
			// PMM - daedalus sounds
			if (self->mass < 225)
				gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
			else
				gi.sound (self, CHAN_VOICE, daed_sound_pain2, 1, ATTN_NORM, 0);
			self->monsterinfo.currentmove = &hover_move_pain2;
		}
//PGM
//====
	}
}

void hover_deadthink (edict_t *self)
{
	if (!self->groundentity && level.time < self->timestamp)
	{
		self->nextthink = level.time + FRAMETIME;
		return;
	}
	BecomeExplosion1(self);
}

void hover_dead (edict_t *self)
{
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->think = hover_deadthink;
	self->nextthink = level.time + FRAMETIME;
	self->timestamp = level.time + 15;
	gi.linkentity (self);
}

void hover_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
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
		for (n= 0; n < 2; n++)
			ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowHead (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

// regular death
	// PMM - daedalus sounds
	if (self->mass < 225)
	{
		if (random() < 0.5)
			gi.sound (self, CHAN_VOICE, sound_death1, 1, ATTN_NORM, 0);
		else
			gi.sound (self, CHAN_VOICE, sound_death2, 1, ATTN_NORM, 0);
	}
	else
	{
		if (random() < 0.5)
			gi.sound (self, CHAN_VOICE, daed_sound_death1, 1, ATTN_NORM, 0);
		else
			gi.sound (self, CHAN_VOICE, daed_sound_death2, 1, ATTN_NORM, 0);
	}
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.currentmove = &hover_move_death1;
}

//===========
//PGM
qboolean hover_blocked (edict_t *self, float dist)
{
	if(blocked_checkshot (self, 0.25 + (0.05 * skill->value) ))
		return true;

	return false;
}
//PGM
//===========

/*QUAKED monster_hover (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
/*QUAKED monster_daedalus (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
This is the improved icarus monster.
*/
void SP_monster_hover (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/hover/tris.md2");
	VectorSet (self->mins, -24, -24, -24);
	VectorSet (self->maxs, 24, 24, 32);

	self->health = 240;
	self->gib_health = -100;
	self->mass = 150;

	self->pain = hover_pain;
	self->die = hover_die;

	self->monsterinfo.stand = hover_stand;
	self->monsterinfo.walk = hover_walk;
	self->monsterinfo.run = hover_run;
//	self->monsterinfo.dodge = hover_dodge;
	self->monsterinfo.attack = hover_start_attack;
	self->monsterinfo.sight = hover_sight;
	self->monsterinfo.search = hover_search;
	self->monsterinfo.blocked = hover_blocked;		// PGM

//PGM
	if (strcmp(self->classname, "monster_daedalus") == 0)
	{
		self->health = 450;
		self->mass = 225;
		self->yaw_speed = 25;
		self->monsterinfo.power_armor_type = POWER_ARMOR_SCREEN;
		self->monsterinfo.power_armor_power = 100;
		// PMM - daedalus sounds
		self->s.sound = gi.soundindex ("daedalus/daedidle1.wav");
		daed_sound_pain1 = gi.soundindex ("daedalus/daedpain1.wav");	
		daed_sound_pain2 = gi.soundindex ("daedalus/daedpain2.wav");	
		daed_sound_death1 = gi.soundindex ("daedalus/daeddeth1.wav");	
		daed_sound_death2 = gi.soundindex ("daedalus/daeddeth2.wav");	
		daed_sound_sight = gi.soundindex ("daedalus/daedsght1.wav");	
		daed_sound_search1 = gi.soundindex ("daedalus/daedsrch1.wav");	
		daed_sound_search2 = gi.soundindex ("daedalus/daedsrch2.wav");	
		gi.soundindex ("tank/tnkatck3.wav");	
		// pmm 
	}
	else
	{
		sound_pain1 = gi.soundindex ("hover/hovpain1.wav");	
		sound_pain2 = gi.soundindex ("hover/hovpain2.wav");	
		sound_death1 = gi.soundindex ("hover/hovdeth1.wav");	
		sound_death2 = gi.soundindex ("hover/hovdeth2.wav");	
		sound_sight = gi.soundindex ("hover/hovsght1.wav");	
		sound_search1 = gi.soundindex ("hover/hovsrch1.wav");	
		sound_search2 = gi.soundindex ("hover/hovsrch2.wav");	
		gi.soundindex ("hover/hovatck1.wav");	

		self->s.sound = gi.soundindex ("hover/hovidle1.wav");
	}
//PGM

	gi.linkentity (self);

	self->monsterinfo.currentmove = &hover_move_stand;	
	self->monsterinfo.scale = MODEL_SCALE;

	flymonster_start (self);

//PGM
	if (strcmp(self->classname, "monster_daedalus") == 0)
		self->s.skinnum = 2;
//PGM

}
