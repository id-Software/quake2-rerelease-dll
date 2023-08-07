// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

/*
==============================================================================

	GLADIATOR BOSS

==============================================================================
*/

#include "g_local.h"
#include "m_gladiator.h"


static int	sound_pain1;
static int	sound_pain2;
static int	sound_die;
static int	sound_gun;
static int	sound_cleaver_swing;
static int	sound_cleaver_hit;
static int	sound_cleaver_miss;
static int	sound_idle;
static int	sound_search;
static int	sound_sight;


void gladb_idle (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

void gladb_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void gladb_search (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void gladb_cleaver_swing (edict_t *self)
{
	gi.sound (self, CHAN_WEAPON, sound_cleaver_swing, 1, ATTN_NORM, 0);
}

mframe_t gladb_frames_stand [] =
{
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL
};
mmove_t gladb_move_stand = {FRAME_stand1, FRAME_stand7, gladb_frames_stand, NULL};

void gladb_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &gladb_move_stand;
}


mframe_t gladb_frames_walk [] =
{
	ai_walk, 15, NULL,
	ai_walk, 7,  NULL,
	ai_walk, 6,  NULL,
	ai_walk, 5,  NULL,
	ai_walk, 2,  NULL,
	ai_walk, 0,  NULL,
	ai_walk, 2,  NULL,
	ai_walk, 8,  NULL,
	ai_walk, 12, NULL,
	ai_walk, 8,  NULL,
	ai_walk, 5,  NULL,
	ai_walk, 5,  NULL,
	ai_walk, 2,  NULL,
	ai_walk, 2,  NULL,
	ai_walk, 1,  NULL,
	ai_walk, 8,  NULL
};
mmove_t gladb_move_walk = {FRAME_walk1, FRAME_walk16, gladb_frames_walk, NULL};

void gladb_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &gladb_move_walk;
}


mframe_t gladb_frames_run [] =
{
	ai_run, 23,	NULL,
	ai_run, 14,	NULL,
	ai_run, 14,	NULL,
	ai_run, 21,	NULL,
	ai_run, 12,	NULL,
	ai_run, 13,	NULL
};
mmove_t gladb_move_run = {FRAME_run1, FRAME_run6, gladb_frames_run, NULL};

void gladb_run (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &gladb_move_stand;
	else
		self->monsterinfo.currentmove = &gladb_move_run;
}


void GladbMelee (edict_t *self)
{
	vec3_t	aim;

	VectorSet (aim, MELEE_DISTANCE, self->mins[0], -4);
	if (fire_hit (self, aim, (20 + (rand() %5)), 300))
		gi.sound (self, CHAN_AUTO, sound_cleaver_hit, 1, ATTN_NORM, 0);
	else
		gi.sound (self, CHAN_AUTO, sound_cleaver_miss, 1, ATTN_NORM, 0);
}

mframe_t gladb_frames_attack_melee [] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, gladb_cleaver_swing,
	ai_charge, 0, NULL,
	ai_charge, 0, GladbMelee,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, gladb_cleaver_swing,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, GladbMelee,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t gladb_move_attack_melee = {FRAME_melee1, FRAME_melee17, gladb_frames_attack_melee, gladb_run};

void gladb_melee(edict_t *self)
{
	self->monsterinfo.currentmove = &gladb_move_attack_melee;
}


void gladbGun (edict_t *self)
{
	vec3_t	start;
	vec3_t	dir;
	vec3_t	forward, right;

	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_GLADIATOR_RAILGUN_1], forward, right, start);

	// calc direction to where we targted
	VectorSubtract (self->pos1, start, dir);
	VectorNormalize (dir);

	fire_plasma (self, start, dir, 100, 725, 60, 60);
}

void gladbGun_check (edict_t *self)
{
	if (skill->value == 3)
		gladbGun (self);
}

mframe_t gladb_frames_attack_gun [] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, gladbGun,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, gladbGun,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, gladbGun_check
};
mmove_t gladb_move_attack_gun = {FRAME_attack1, FRAME_attack9, gladb_frames_attack_gun, gladb_run};

void gladb_attack(edict_t *self)
{
	float	range;
	vec3_t	v;

	// a small safe zone
	VectorSubtract (self->s.origin, self->enemy->s.origin, v);
	range = VectorLength(v);
	if (range <= (MELEE_DISTANCE + 32))
		return;

	// charge up the railgun
	gi.sound (self, CHAN_WEAPON, sound_gun, 1, ATTN_NORM, 0);
	VectorCopy (self->enemy->s.origin, self->pos1);	//save for aiming the shot
	self->pos1[2] += self->enemy->viewheight;
	self->monsterinfo.currentmove = &gladb_move_attack_gun;
}


mframe_t gladb_frames_pain [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t gladb_move_pain = {FRAME_pain1, FRAME_pain6, gladb_frames_pain, gladb_run};

mframe_t gladb_frames_pain_air [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t gladb_move_pain_air = {FRAME_painup1, FRAME_painup7, gladb_frames_pain_air, gladb_run};

void gladb_pain (edict_t *self, edict_t *other, float kick, int damage)
{

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
	{
		if ((self->velocity[2] > 100) && (self->monsterinfo.currentmove == &gladb_move_pain))
			self->monsterinfo.currentmove = &gladb_move_pain_air;
		return;
	}

	self->pain_debounce_time = level.time + 3;

	if (random() < 0.5)
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

	if (self->velocity[2] > 100)
		self->monsterinfo.currentmove = &gladb_move_pain_air;
	else
		self->monsterinfo.currentmove = &gladb_move_pain;
	
}


void gladb_dead (edict_t *self)
{
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity (self);
}

mframe_t gladb_frames_death [] =
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
	ai_move, 0, NULL
};
mmove_t gladb_move_death = {FRAME_death1, FRAME_death22, gladb_frames_death, gladb_dead};

void gladb_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
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
	gi.sound (self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	self->monsterinfo.currentmove = &gladb_move_death;
}


/*QUAKED monster_gladb (1 .5 0) (-32 -32 -24) (32 32 64) Ambush Trigger_Spawn Sight
*/
void SP_monster_gladb (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}


	sound_pain1 = gi.soundindex ("gladiator/pain.wav");	
	sound_pain2 = gi.soundindex ("gladiator/gldpain2.wav");	
	sound_die = gi.soundindex ("gladiator/glddeth2.wav");	
	// note to self
	// need to change to PHALANX sound
	sound_gun = gi.soundindex ("weapons/plasshot.wav");

	sound_cleaver_swing = gi.soundindex ("gladiator/melee1.wav");
	sound_cleaver_hit = gi.soundindex ("gladiator/melee2.wav");
	sound_cleaver_miss = gi.soundindex ("gladiator/melee3.wav");
	sound_idle = gi.soundindex ("gladiator/gldidle1.wav");
	sound_search = gi.soundindex ("gladiator/gldsrch1.wav");
	sound_sight = gi.soundindex ("gladiator/sight.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex ("models/monsters/gladb/tris.md2");
	VectorSet (self->mins, -32, -32, -24);
	VectorSet (self->maxs, 32, 32, 64);

	self->health = 800;
	self->gib_health = -175;
	self->mass = 350;

	self->pain = gladb_pain;
	self->die = gladb_die;

	self->monsterinfo.stand = gladb_stand;
	self->monsterinfo.walk = gladb_walk;
	self->monsterinfo.run = gladb_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = gladb_attack;
	self->monsterinfo.melee = gladb_melee;
	self->monsterinfo.sight = gladb_sight;
	self->monsterinfo.idle = gladb_idle;
	self->monsterinfo.search = gladb_search;

	gi.linkentity (self);
	self->monsterinfo.currentmove = &gladb_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;

	self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
	self->monsterinfo.power_armor_power = 400;


	walkmonster_start (self);
}

