// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

black widow

==============================================================================
*/

// self->timestamp used to prevent rapid fire of railgun
// self->plat2flags used for fire count (flashes)
// self->monsterinfo.pausetime used for timing of blaster shots

#include "g_local.h"
#include "m_widow.h"

#define	NUM_STALKERS_SPAWNED		6		// max # of stalkers she can spawn

#define	RAIL_TIME					3
#define	BLASTER_TIME				2
#define	BLASTER2_DAMAGE				10
#define	WIDOW_RAIL_DAMAGE			50

#define	DRAWBBOX					NULL
#define	SHOWME						NULL	// showme

void BossExplode (edict_t *self);

qboolean infront (edict_t *self, edict_t *other);

static int	sound_pain1;
static int	sound_pain2;
static int	sound_pain3;
static int	sound_search1;
static int	sound_rail;
static int	sound_sight;

static unsigned long shotsfired;

static vec3_t spawnpoints[] = {
	{30,  100, 16},
	{30, -100, 16}
};

static vec3_t beameffects[] = {
	{12.58, -43.71, 68.88},
	{3.43, 58.72, 68.41}
};

static float sweep_angles[] = {
//	32.0, 26.0, 20.0, 11.5, 3.0, -8.0, -13.0, -27.0, -41.0
	32.0, 26.0, 20.0, 10.0, 0.0, -6.5, -13.0, -27.0, -41.0
};

vec3_t stalker_mins = {-28, -28, -18};
vec3_t stalker_maxs = {28, 28, 18};

unsigned int	widow_damage_multiplier;

void widow_run (edict_t *self);
void widow_stand (edict_t *self);
void widow_dead (edict_t *self);
void widow_attack (edict_t *self);
void widow_attack_blaster (edict_t *self);
void widow_reattack_blaster (edict_t *self);
void widow_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

void widow_start_spawn (edict_t *self);
void widow_done_spawn (edict_t *self);
void widow_spawn_check (edict_t *self);
void widow_prep_spawn (edict_t *self);
void widow_attack_rail (edict_t *self);

void widow_start_run_5 (edict_t *self);
void widow_start_run_10 (edict_t *self);
void widow_start_run_12 (edict_t *self);

void WidowCalcSlots (edict_t *self);

void drawbbox (edict_t *self);

void showme (edict_t *self)
{
	gi.dprintf ("frame %d\n", self->s.frame);
}

void widow_search (edict_t *self)
{
//	if (random() < 0.5)
//		gi.sound (self, CHAN_VOICE, sound_search1, 1, ATTN_NONE, 0);
}

void widow_sight (edict_t *self, edict_t *other)
{
	self->monsterinfo.pausetime = 0;
//	gi.sound (self, CHAN_WEAPON, sound_sight, 1, ATTN_NORM, 0);

//	if ((g_showlogic) && (g_showlogic->value))
//		gi.dprintf ("widow: found target!\n");
}

mmove_t widow_move_attack_post_blaster;
mmove_t widow_move_attack_post_blaster_r;
mmove_t widow_move_attack_post_blaster_l;
mmove_t widow_move_attack_blaster;

float target_angle (edict_t *self)
{
	vec3_t target;
	float enemy_yaw;

	VectorSubtract (self->s.origin, self->enemy->s.origin, target);
	enemy_yaw = self->s.angles[YAW] - vectoyaw2(target);
	if (enemy_yaw < 0)
		enemy_yaw += 360.0;

	// this gets me 0 degrees = forward
	enemy_yaw -= 180.0;
	// positive is to right, negative to left

	return enemy_yaw;
}

int WidowTorso (edict_t *self)
{
	float enemy_yaw;

	enemy_yaw = target_angle (self);

//	if ((g_showlogic) && (g_showlogic->value))
//		gi.dprintf ("%2.2f -> ", enemy_yaw);

	if (enemy_yaw >= 105)
	{
		self->monsterinfo.currentmove = &widow_move_attack_post_blaster_r;
		self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
		return 0;
	}

	if (enemy_yaw <= -75.0)
	{
		self->monsterinfo.currentmove = &widow_move_attack_post_blaster_l;
		self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
		return 0;
	}

	if (enemy_yaw >= 95)
		return FRAME_fired03;
	else if (enemy_yaw >= 85)
		return FRAME_fired04;
	else if (enemy_yaw >= 75)
		return FRAME_fired05;
	else if (enemy_yaw >= 65)
		return FRAME_fired06;
	else if (enemy_yaw >= 55)
		return FRAME_fired07;
	else if (enemy_yaw >= 45)
		return FRAME_fired08;
	else if (enemy_yaw >= 35)
		return FRAME_fired09;
	else if (enemy_yaw >= 25)
		return FRAME_fired10;
	else if (enemy_yaw >= 15)
		return FRAME_fired11;
	else if (enemy_yaw >= 5)
		return FRAME_fired12;
	else if (enemy_yaw >= -5)
		return FRAME_fired13;
	else if (enemy_yaw >= -15)
		return FRAME_fired14;
	else if (enemy_yaw >= -25)
		return FRAME_fired15;
	else if (enemy_yaw >= -35)
		return FRAME_fired16;
	else if (enemy_yaw >= -45)
		return FRAME_fired17;
	else if (enemy_yaw >= -55)
		return FRAME_fired18;
	else if (enemy_yaw >= -65)
		return FRAME_fired19;
	else if (enemy_yaw >= -75)
		return FRAME_fired20;
/*
	if (fabs(enemy_yaw) < 11.25)
		return FRAME_fired03;
	else if (fabs(enemy_yaw) > 56.25)
	{
		self->monsterinfo.currentmove = &widow_move_attack_post_blaster;
		self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
		return;
	}
	else if ((enemy_yaw >= 11.25) && (enemy_yaw < 33.75))
		return FRAME_fired04;
	else if (enemy_yaw >= 33.75)
		return FRAME_fired05;
	else if ((enemy_yaw <= -11.25) && (enemy_yaw > -33.75))
		return FRAME_fired06;
	else if (enemy_yaw <= -33.75)
		return FRAME_fired07;
*/
}

#define	VARIANCE 15.0

void WidowBlaster (edict_t *self)
{
	vec3_t	forward, right, target, vec, targ_angles;
	vec3_t	start;
	int		flashnum;
	int		effect;

	if (!self->enemy)
		return;

	shotsfired++;
	if (!(shotsfired % 4))
		effect = EF_BLASTER;
	else
		effect = 0;

	AngleVectors (self->s.angles, forward, right, NULL);
	if ((self->s.frame >= FRAME_spawn05) && (self->s.frame <= FRAME_spawn13))
	{
		// sweep
		flashnum = MZ2_WIDOW_BLASTER_SWEEP1 + self->s.frame - FRAME_spawn05;
		G_ProjectSource (self->s.origin, monster_flash_offset[flashnum], forward, right, start);
		VectorSubtract (self->enemy->s.origin, start, target);
		vectoangles2 (target, targ_angles);
		
		VectorCopy (self->s.angles, vec);

		vec[PITCH] += targ_angles[PITCH];
		vec[YAW] -= sweep_angles[flashnum-MZ2_WIDOW_BLASTER_SWEEP1];

		AngleVectors (vec, forward, NULL, NULL);
		monster_fire_blaster2 (self, start, forward, BLASTER2_DAMAGE*widow_damage_multiplier, 1000, flashnum, effect);

/*		if (self->s.frame == FRAME_spawn13)
		{
			VectorMA (start, 1024, forward, debugend);

			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_DEBUGTRAIL);
			gi.WritePosition (start);
			gi.WritePosition (debugend);
			gi.multicast (start, MULTICAST_ALL);

			drawbbox (self);
			self->monsterinfo.aiflags |= AI_HOLD_FRAME|AI_MANUAL_STEERING;
		}
*/
	}
	else if ((self->s.frame >= FRAME_fired02a) && (self->s.frame <= FRAME_fired20))
	{
		vec3_t angles;
		float aim_angle, target_angle;
		float error;

		self->monsterinfo.aiflags |= AI_MANUAL_STEERING;

		self->monsterinfo.nextframe = WidowTorso (self);

		if (!self->monsterinfo.nextframe)
			self->monsterinfo.nextframe = self->s.frame;

//		if ((g_showlogic) && (g_showlogic->value))
//			gi.dprintf ("%d\n", self->monsterinfo.nextframe);

		if (self->s.frame == FRAME_fired02a)
			flashnum = MZ2_WIDOW_BLASTER_0;
		else
			flashnum = MZ2_WIDOW_BLASTER_100 + self->s.frame - FRAME_fired03;

		G_ProjectSource (self->s.origin, monster_flash_offset[flashnum], forward, right, start);

		PredictAim (self->enemy, start, 1000, true, ((random()*0.1)-0.05), forward, NULL);

		// clamp it to within 10 degrees of the aiming angle (where she's facing)
		vectoangles2 (forward, angles);
		// give me 100 -> -70
		aim_angle = 100 - (10*(flashnum-MZ2_WIDOW_BLASTER_100));
		if (aim_angle <= 0)
			aim_angle += 360;
		target_angle = self->s.angles[YAW] - angles[YAW];
		if (target_angle <= 0)
			target_angle += 360;

		error = aim_angle - target_angle;

		// positive error is to entity's left, aka positive direction in engine
		// unfortunately, I decided that for the aim_angle, positive was right.  *sigh*
		if (error > VARIANCE)
		{
//			if ((g_showlogic) && (g_showlogic->value))
//				gi.dprintf ("angle %2.2f (really %2.2f) (%2.2f off of %2.2f) corrected to", target_angle, angles[YAW], error, aim_angle);
			angles[YAW] = (self->s.angles[YAW] - aim_angle) + VARIANCE;
//			if ((g_showlogic) && (g_showlogic->value))
//			{
//				if (angles[YAW] <= 0)
//					angles[YAW] += 360;
//				gi.dprintf (" %2.2f\n", angles[YAW]);
//			}
			AngleVectors (angles, forward, NULL, NULL);
		}
		else if (error < -VARIANCE)
		{
//			if ((g_showlogic) && (g_showlogic->value))
//				gi.dprintf ("angle %2.2f (really %2.2f) (%2.2f off of %2.2f) corrected to", target_angle, angles[YAW], error, aim_angle);
			angles[YAW] = (self->s.angles[YAW] - aim_angle) - VARIANCE;
//			if ((g_showlogic) && (g_showlogic->value))
//			{
//				if (angles[YAW] <= 0)
//					angles[YAW] += 360;
//				gi.dprintf (" %2.2f\n", angles[YAW]);
//			}
			AngleVectors (angles, forward, NULL, NULL);
		}
//		gi.dprintf ("%2.2f - %2.2f - %2.2f - %2.2f\n", aim_angle, self->s.angles[YAW] - angles[YAW], target_angle, error);
//		gi.dprintf ("%2.2f - %2.2f - %2.2f\n", angles[YAW], aim_angle, self->s.angles[YAW]);

/*
		if (self->s.frame == FRAME_fired20)
		{
			VectorMA (start, 512, forward, debugend);
			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_DEBUGTRAIL);
			gi.WritePosition (start);
			gi.WritePosition (forward);
			gi.multicast (start, MULTICAST_ALL);

			drawbbox (self);
			self->monsterinfo.aiflags |= AI_HOLD_FRAME;
			self->monsterinfo.nextframe = FRAME_fired20;
			self->monsterinfo.aiflags |= AI_MANUAL_STEERING;
		}
*/
/*
		if (!(self->plat2flags % 3))
			effect = EF_HYPERBLASTER;
		else
			effect = 0;
		self->plat2flags ++;
*/
		monster_fire_blaster2 (self, start, forward, BLASTER2_DAMAGE*widow_damage_multiplier, 1000, flashnum, effect);
	}
	else if ((self->s.frame >= FRAME_run01) && (self->s.frame <= FRAME_run08))
	{
		flashnum = MZ2_WIDOW_RUN_1 + self->s.frame - FRAME_run01;
		G_ProjectSource (self->s.origin, monster_flash_offset[flashnum], forward, right, start);
		
		VectorSubtract (self->enemy->s.origin, start, target);
		target[2] += self->enemy->viewheight;

		monster_fire_blaster2 (self, start, target, BLASTER2_DAMAGE*widow_damage_multiplier, 1000, flashnum, effect);
	}
//	else
//	{
//		if ((g_showlogic) && (g_showlogic->value))
//			gi.dprintf ("widow: firing on non-fire frame!\n");
//	}
}	

void WidowSpawn (edict_t *self)
{
	vec3_t	f, r, u, offset, startpoint, spawnpoint;
	edict_t	*ent, *designated_enemy;
	int		i;

	AngleVectors (self->s.angles, f, r, u);

	for (i=0; i < 2; i++)
	{
		VectorCopy (spawnpoints[i], offset);

		G_ProjectSource2 (self->s.origin, offset, f, r, u, startpoint);

		if (FindSpawnPoint (startpoint, stalker_mins, stalker_maxs, spawnpoint, 64))
		{
			ent = CreateGroundMonster (spawnpoint, self->s.angles, stalker_mins, stalker_maxs, "monster_stalker", 256);

			if (!ent)
				continue;
			
			self->monsterinfo.monster_used++;
			ent->monsterinfo.commander = self;
//			if ((g_showlogic) && (g_showlogic->value))
//				gi.dprintf ("widow: post-spawn : %d slots left out of %d\n", SELF_SLOTS_LEFT, self->monsterinfo.monster_slots);

			ent->nextthink = level.time;
			ent->think (ent);
			
			ent->monsterinfo.aiflags |= AI_SPAWNED_WIDOW|AI_DO_NOT_COUNT|AI_IGNORE_SHOTS;

			if (!(coop && coop->value))
			{
				designated_enemy = self->enemy;
			}
			else
			{
				designated_enemy = PickCoopTarget(ent);
				if (designated_enemy)
				{
					// try to avoid using my enemy
					if (designated_enemy == self->enemy)
					{
						designated_enemy = PickCoopTarget(ent);
						if (designated_enemy)
						{
//							if ((g_showlogic) && (g_showlogic->value))
//							{
//								gi.dprintf ("PickCoopTarget returned a %s - ", designated_enemy->classname);
//								if (designated_enemy->client)
//									gi.dprintf ("with name %s\n", designated_enemy->client->pers.netname);
//								else
//									gi.dprintf ("NOT A CLIENT\n");
//							}
						}
						else
						{
//							if ((g_showlogic) && (g_showlogic->value))
//								gi.dprintf ("pick coop failed, using my current enemy\n");
							designated_enemy = self->enemy;
						}
					}
				}
				else
				{
//					if ((g_showlogic) && (g_showlogic->value))
//						gi.dprintf ("pick coop failed, using my current enemy\n");
					designated_enemy = self->enemy;
				}
			}

			if ((designated_enemy->inuse) && (designated_enemy->health > 0))
			{
				ent->enemy = designated_enemy;
				FoundTarget (ent);
				ent->monsterinfo.attack(ent);
			}
		}
	}
}

void widow_spawn_check (edict_t *self)
{
	WidowBlaster(self);
	WidowSpawn (self);
}

void widow_ready_spawn (edict_t *self)
{
	vec3_t	f, r, u, offset, startpoint, spawnpoint;
	int		i;

	WidowBlaster(self);
	AngleVectors (self->s.angles, f, r, u);

	for (i=0; i < 2; i++)
	{
		VectorCopy (spawnpoints[i], offset);
		G_ProjectSource2 (self->s.origin, offset, f, r, u, startpoint);
		if (FindSpawnPoint (startpoint, stalker_mins, stalker_maxs, spawnpoint, 64))
		{
			SpawnGrow_Spawn (spawnpoint, 1);
		}
	}
}

void widow_step (edict_t *self)
{
	gi.sound (self, CHAN_BODY, gi.soundindex("widow/bwstep3.wav"), 1, ATTN_NORM, 0);
}

mframe_t widow_frames_stand [] =
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
	ai_stand, 0, NULL
};
mmove_t	widow_move_stand = {FRAME_idle01, FRAME_idle11, widow_frames_stand, NULL};

mframe_t widow_frames_walk [] =
{
	// hand generated numbers
/*
	ai_run,	6,	NULL,
	ai_run,	3,	NULL,
	ai_run,	3,	NULL,
	ai_run,	3,	NULL,
	ai_run,	4,	NULL,			//5
	ai_run,	4,	NULL,
	ai_run,	4,	NULL,
	ai_run,	4.5,	NULL,
	ai_run,	3,	NULL,
	ai_run,	5,	NULL,			//10
	ai_run,	8,	NULL,
	ai_run,	8,	NULL,
	ai_run,	6.5,	NULL
*/
	// auto generated numbers
	ai_walk,	2.79,	widow_step,
	ai_walk,	2.77,	NULL,
	ai_walk,	3.53,	NULL,
	ai_walk,	3.97,	NULL,
	ai_walk,	4.13,	NULL,			//5
	ai_walk,	4.09,	NULL,
	ai_walk,	3.84,	NULL,
	ai_walk,	3.62,	widow_step,
	ai_walk,	3.29,	NULL,
	ai_walk,	6.08,	NULL,			//10
	ai_walk,	6.94,	NULL,
	ai_walk,	5.73,	NULL,
	ai_walk,	2.85,	NULL
};
mmove_t widow_move_walk = {FRAME_walk01, FRAME_walk13, widow_frames_walk, NULL};


mframe_t widow_frames_run [] =
{
	ai_run,	2.79,	widow_step,
	ai_run,	2.77,	NULL,
	ai_run,	3.53,	NULL,
	ai_run,	3.97,	NULL,
	ai_run,	4.13,	NULL,			//5
	ai_run,	4.09,	NULL,
	ai_run,	3.84,	NULL,
	ai_run,	3.62,	widow_step,
	ai_run,	3.29,	NULL,
	ai_run,	6.08,	NULL,			//10
	ai_run,	6.94,	NULL,
	ai_run,	5.73,	NULL,
	ai_run,	2.85,	NULL
};
mmove_t widow_move_run = {FRAME_walk01, FRAME_walk13, widow_frames_run, NULL};

void widow_stepshoot (edict_t *self)
{
	gi.sound (self, CHAN_BODY, gi.soundindex("widow/bwstep2.wav"), 1, ATTN_NORM,0);
	WidowBlaster (self);
}

mframe_t widow_frames_run_attack [] =
{
	ai_charge,	13,	widow_stepshoot,
	ai_charge,	11.72,	WidowBlaster,
	ai_charge,	18.04,	WidowBlaster,
	ai_charge,	14.58,	WidowBlaster,
	ai_charge,	13,	widow_stepshoot,			//5
	ai_charge,	12.12,	WidowBlaster,
	ai_charge,	19.63,	WidowBlaster,
	ai_charge,	11.37,	WidowBlaster
};
mmove_t widow_move_run_attack = {FRAME_run01, FRAME_run08, widow_frames_run_attack, widow_run};


//
// These three allow specific entry into the run sequence
//

void widow_start_run_5 (edict_t *self)
{
	self->monsterinfo.currentmove = &widow_move_run;
	self->monsterinfo.nextframe = FRAME_walk05;
}

void widow_start_run_10 (edict_t *self)
{
	self->monsterinfo.currentmove = &widow_move_run;
	self->monsterinfo.nextframe = FRAME_walk10;
}

void widow_start_run_12 (edict_t *self)
{
	self->monsterinfo.currentmove = &widow_move_run;
	self->monsterinfo.nextframe = FRAME_walk12;
}


mframe_t widow_frames_attack_pre_blaster [] =
{
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	widow_attack_blaster
};
mmove_t widow_move_attack_pre_blaster = {FRAME_fired01, FRAME_fired02a, widow_frames_attack_pre_blaster, NULL};

// Loop this
mframe_t widow_frames_attack_blaster [] =
{
	ai_charge,	0,	widow_reattack_blaster,		// straight ahead
	ai_charge,	0,	widow_reattack_blaster,		// 100 degrees right
	ai_charge,	0,	widow_reattack_blaster,
	ai_charge,	0,	widow_reattack_blaster,
	ai_charge,	0,	widow_reattack_blaster,
	ai_charge,	0,	widow_reattack_blaster,
	ai_charge,	0,	widow_reattack_blaster,		// 50 degrees right
	ai_charge,	0,	widow_reattack_blaster,
	ai_charge,	0,	widow_reattack_blaster,
	ai_charge,	0,	widow_reattack_blaster,
	ai_charge,	0,	widow_reattack_blaster,
	ai_charge,	0,	widow_reattack_blaster,		// straight
	ai_charge,	0,	widow_reattack_blaster,
	ai_charge,	0,	widow_reattack_blaster,
	ai_charge,	0,	widow_reattack_blaster,
	ai_charge,	0,	widow_reattack_blaster,
	ai_charge,	0,	widow_reattack_blaster,		// 50 degrees left
	ai_charge,	0,	widow_reattack_blaster,
	ai_charge,	0,	widow_reattack_blaster		// 70 degrees left
};
mmove_t widow_move_attack_blaster = {FRAME_fired02a, FRAME_fired20, widow_frames_attack_blaster, NULL};

mframe_t widow_frames_attack_post_blaster [] =
{
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL
};
mmove_t widow_move_attack_post_blaster = {FRAME_fired21, FRAME_fired22, widow_frames_attack_post_blaster, widow_run};

mframe_t widow_frames_attack_post_blaster_r [] =
{
	ai_charge,	-2,	NULL,
	ai_charge,	-10,	NULL,
	ai_charge,	-2,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	widow_start_run_12
};
mmove_t widow_move_attack_post_blaster_r = {FRAME_transa01, FRAME_transa05, widow_frames_attack_post_blaster_r, NULL};

mframe_t widow_frames_attack_post_blaster_l [] =
{
	ai_charge,	0,	NULL,
	ai_charge,	14,	NULL,
	ai_charge,	-2,	NULL,
	ai_charge,	10,	NULL,
	ai_charge,	10,	widow_start_run_12
};
mmove_t widow_move_attack_post_blaster_l = {FRAME_transb01, FRAME_transb05, widow_frames_attack_post_blaster_l, NULL};

mmove_t widow_move_attack_rail;
mmove_t widow_move_attack_rail_l;
mmove_t widow_move_attack_rail_r;

void WidowRail (edict_t *self)
{
	vec3_t	start;
	vec3_t	dir;
	vec3_t	forward, right;
	int		flash;

//	gi.dprintf ("railing!\n");
	AngleVectors (self->s.angles, forward, right, NULL);

	if (self->monsterinfo.currentmove == &widow_move_attack_rail)
		flash = MZ2_WIDOW_RAIL;
	else if (self->monsterinfo.currentmove == &widow_move_attack_rail_l)
	{
		flash = MZ2_WIDOW_RAIL_LEFT;
	}
	else if (self->monsterinfo.currentmove == &widow_move_attack_rail_r)
	{
		flash = MZ2_WIDOW_RAIL_RIGHT;
	}

	G_ProjectSource (self->s.origin, monster_flash_offset[flash], forward, right, start);

	// calc direction to where we targeted
	VectorSubtract (self->pos1, start, dir);
	VectorNormalize (dir);

	monster_fire_railgun (self, start, dir, WIDOW_RAIL_DAMAGE*widow_damage_multiplier, 100, flash);
	self->timestamp = level.time + RAIL_TIME;
}

void WidowSaveLoc (edict_t *self)
{
	VectorCopy (self->enemy->s.origin, self->pos1);	//save for aiming the shot
	self->pos1[2] += self->enemy->viewheight;
};

void widow_start_rail (edict_t *self)
{
	self->monsterinfo.aiflags |= AI_MANUAL_STEERING;
}

void widow_rail_done (edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
}

mframe_t widow_frames_attack_pre_rail [] =
{
	ai_charge,	0,	widow_start_rail,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	widow_attack_rail
};
mmove_t widow_move_attack_pre_rail = {FRAME_transc01, FRAME_transc04, widow_frames_attack_pre_rail, NULL};

mframe_t widow_frames_attack_rail [] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, WidowSaveLoc,
	ai_charge, -10, WidowRail,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, widow_rail_done
};
mmove_t widow_move_attack_rail = {FRAME_firea01, FRAME_firea09, widow_frames_attack_rail, widow_run};

mframe_t widow_frames_attack_rail_r [] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, WidowSaveLoc,
	ai_charge, -10, WidowRail,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, widow_rail_done
};
mmove_t widow_move_attack_rail_r = {FRAME_fireb01, FRAME_fireb09, widow_frames_attack_rail_r, widow_run};

mframe_t widow_frames_attack_rail_l [] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, WidowSaveLoc,
	ai_charge, -10, WidowRail,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, widow_rail_done
};
mmove_t widow_move_attack_rail_l = {FRAME_firec01, FRAME_firec09, widow_frames_attack_rail_l, widow_run};

void widow_attack_rail (edict_t *self)
{
	float	enemy_angle;
//	gi.dprintf ("going to the rail!\n");

	enemy_angle = target_angle (self);

	if (enemy_angle < -15)
		self->monsterinfo.currentmove = &widow_move_attack_rail_l;
	else if (enemy_angle > 15)
		self->monsterinfo.currentmove = &widow_move_attack_rail_r;
	else
		self->monsterinfo.currentmove = &widow_move_attack_rail;
}

void widow_start_spawn (edict_t *self)
{
	self->monsterinfo.aiflags |= AI_MANUAL_STEERING;
}

void widow_done_spawn (edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
}

mframe_t widow_frames_spawn [] =
{
	ai_charge,	0,	NULL,						//1
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	widow_start_spawn,
	ai_charge,	0,	NULL,						//5
	ai_charge,	0,	WidowBlaster,				//6
	ai_charge,	0,	widow_ready_spawn,			//7
	ai_charge,	0,	WidowBlaster,
	ai_charge,	0,	WidowBlaster,				//9
	ai_charge,	0,	widow_spawn_check,
	ai_charge,	0,	WidowBlaster,				//11
	ai_charge,	0,	WidowBlaster,
	ai_charge,	0,	WidowBlaster,				//13
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	widow_done_spawn
};
mmove_t widow_move_spawn = {FRAME_spawn01, FRAME_spawn18, widow_frames_spawn, widow_run};

mframe_t widow_frames_pain_heavy [] =
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
	ai_move,	0,	NULL
};
mmove_t widow_move_pain_heavy = {FRAME_pain01, FRAME_pain13, widow_frames_pain_heavy, widow_run};

mframe_t widow_frames_pain_light [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL
};
mmove_t widow_move_pain_light = {FRAME_pain201, FRAME_pain203, widow_frames_pain_light, widow_run};

void spawn_out_start (edict_t *self)
{
	vec3_t startpoint,f,r,u;
	self->wait = level.time + 2.0;

//	gi.sound (self, CHAN_VOICE, sound_death, 1, ATTN_NONE, 0);
	AngleVectors (self->s.angles, f, r, u);

	G_ProjectSource2 (self->s.origin, beameffects[0], f, r, u, startpoint);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_WIDOWBEAMOUT);
	gi.WriteShort (20001);
	gi.WritePosition (startpoint);
	gi.multicast (startpoint, MULTICAST_ALL);

	G_ProjectSource2 (self->s.origin, beameffects[1], f, r, u, startpoint);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_WIDOWBEAMOUT);
	gi.WriteShort (20002);
	gi.WritePosition (startpoint);
	gi.multicast (startpoint, MULTICAST_ALL);

	gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/bwidowbeamout.wav"), 1, ATTN_NORM, 0);
}

void spawn_out_do (edict_t *self)
{
	vec3_t startpoint,f,r,u;

	AngleVectors (self->s.angles, f, r, u);
	G_ProjectSource2 (self->s.origin, beameffects[0], f, r, u, startpoint);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_WIDOWSPLASH);
	gi.WritePosition (startpoint);
	gi.multicast (startpoint, MULTICAST_ALL);

	G_ProjectSource2 (self->s.origin, beameffects[1], f, r, u, startpoint);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_WIDOWSPLASH);
	gi.WritePosition (startpoint);
	gi.multicast (startpoint, MULTICAST_ALL);

	VectorCopy (self->s.origin, startpoint);
	startpoint[2] += 36;
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_BOSSTPORT);
	gi.WritePosition (startpoint);
	gi.multicast (startpoint, MULTICAST_PVS);

	Widowlegs_Spawn (self->s.origin, self->s.angles);
	
	G_FreeEdict (self);
}

mframe_t widow_frames_death [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,		//5
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	spawn_out_start,	//10
	ai_move,	0,	NULL,		
	ai_move,	0,	NULL,		
	ai_move,	0,	NULL,		
	ai_move,	0,	NULL,		
	ai_move,	0,	NULL,				//15	
	ai_move,	0,	NULL,		
	ai_move,	0,	NULL,		
	ai_move,	0,	NULL,		
	ai_move,	0,	NULL,		
	ai_move,	0,	NULL,				//20	
	ai_move,	0,	NULL,		
	ai_move,	0,	NULL,		
	ai_move,	0,	NULL,		
	ai_move,	0,	NULL,		
	ai_move,	0,	NULL,				//25
	ai_move,	0,	NULL,		
	ai_move,	0,	NULL,		
	ai_move,	0,	NULL,		
	ai_move,	0,	NULL,		
	ai_move,	0,	NULL,				//30	
	ai_move,	0,	spawn_out_do
};
mmove_t widow_move_death = {FRAME_death01, FRAME_death31, widow_frames_death, NULL};

void widow_attack_kick (edict_t *self)
{
	vec3_t	aim;

//	VectorSet (aim, MELEE_DISTANCE, 0, 4);
	VectorSet (aim, 100, 0, 4);
	if (self->enemy->groundentity)
		fire_hit (self, aim, (50 + (rand() % 6)), 500);
	else	// not as much kick if they're in the air .. makes it harder to land on her head
		fire_hit (self, aim, (50 + (rand() % 6)), 250);

}

mframe_t widow_frames_attack_kick [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, widow_attack_kick,
	ai_move, 0, NULL,				// 5
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};

mmove_t widow_move_attack_kick = {FRAME_kick01, FRAME_kick08, widow_frames_attack_kick, widow_run};

void widow_stand (edict_t *self)
{
//	gi.dprintf ("widow stand\n");
	gi.sound (self, CHAN_WEAPON, gi.soundindex ("widow/laugh.wav"), 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &widow_move_stand;
}

void widow_run (edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &widow_move_stand;
	else
		self->monsterinfo.currentmove = &widow_move_run;
}

void widow_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &widow_move_walk;
}

void widow_attack (edict_t *self)
{
	float	luck;
	qboolean rail_frames = false, blaster_frames = false, blocked = false, anger = false;

	self->movetarget = NULL;

	if (self->monsterinfo.aiflags & AI_BLOCKED)
	{
		blocked = true;
		self->monsterinfo.aiflags &= ~AI_BLOCKED;
	}
	
	if (self->monsterinfo.aiflags & AI_TARGET_ANGER)
	{
		anger = true;
		self->monsterinfo.aiflags &= ~AI_TARGET_ANGER;
	}

	if ((!self->enemy) || (!self->enemy->inuse))
		return;

	if (self->bad_area)
	{
		if ((random() < 0.1) || (level.time < self->timestamp))
			self->monsterinfo.currentmove = &widow_move_attack_pre_blaster;
		else
		{
			gi.sound (self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
			self->monsterinfo.currentmove = &widow_move_attack_pre_rail;
		}
		return;
	}

	// frames FRAME_walk13, FRAME_walk01, FRAME_walk02, FRAME_walk03 are rail gun start frames
	// frames FRAME_walk09, FRAME_walk10, FRAME_walk11, FRAME_walk12 are spawn & blaster start frames

	if ((self->s.frame == FRAME_walk13) || ((self->s.frame >= FRAME_walk01) && (self->s.frame <= FRAME_walk03)))
		rail_frames = true;

	if ((self->s.frame >= FRAME_walk09) && (self->s.frame <= FRAME_walk12))
		blaster_frames = true;

	WidowCalcSlots(self);

	// if we can't see the target, spawn stuff regardless of frame
	if ((self->monsterinfo.attack_state == AS_BLIND) && (SELF_SLOTS_LEFT >= 2))
	{
//		if ((g_showlogic) && (g_showlogic->value))
//			gi.dprintf ("attacking blind!\n");
		self->monsterinfo.currentmove = &widow_move_spawn;
		return;
	}

	// accept bias towards spawning regardless of frame
	if (blocked && (SELF_SLOTS_LEFT >= 2))
	{
		self->monsterinfo.currentmove = &widow_move_spawn;
		return;
	}

	if ((realrange(self, self->enemy) > 300) && (!anger) && (random() < 0.5)  && (!blocked))
	{
		self->monsterinfo.currentmove = &widow_move_run_attack;
		return;
	}

	if (blaster_frames)
	{
//		gi.dprintf ("blaster frame %2.2f <= %2.2f\n", self->monsterinfo.pausetime + BLASTER_TIME, level.time);
		if (SELF_SLOTS_LEFT >= 2)
		{
			self->monsterinfo.currentmove = &widow_move_spawn;
			return;
		}
		else if (self->monsterinfo.pausetime + BLASTER_TIME <= level.time)
		{
			self->monsterinfo.currentmove = &widow_move_attack_pre_blaster;
			return;
		}
	}

	if (rail_frames)
	{
//		gi.dprintf ("rail frame %2.2f - %2.2f\n", level.time, self->timestamp);
		if (!(level.time < self->timestamp))
		{
			gi.sound (self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
			self->monsterinfo.currentmove = &widow_move_attack_pre_rail;
		}
	}

	if ((rail_frames) || (blaster_frames))
		return;

//	if ((g_showlogic) && (g_showlogic->value))
//		gi.dprintf ("widow: unknown start frame, picking randomly\n");

	luck = random();
	if (SELF_SLOTS_LEFT >= 2)
	{
		if ((luck <= 0.40) && (self->monsterinfo.pausetime + BLASTER_TIME <= level.time))
			self->monsterinfo.currentmove = &widow_move_attack_pre_blaster;
		else if ((luck <= 0.7) && !(level.time < self->timestamp))
		{
			gi.sound (self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
			self->monsterinfo.currentmove = &widow_move_attack_pre_rail;
		}
		else
			self->monsterinfo.currentmove = &widow_move_spawn;
	}
	else
	{
		if (level.time < self->timestamp)
			self->monsterinfo.currentmove = &widow_move_attack_pre_blaster;
		else if ((luck <= 0.50) || (level.time + BLASTER_TIME >= self->monsterinfo.pausetime))
		{
			gi.sound (self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
			self->monsterinfo.currentmove = &widow_move_attack_pre_rail;
		}
		else // holdout to blaster
			self->monsterinfo.currentmove = &widow_move_attack_pre_blaster;
	}
}
/*
void widow_attack (edict_t *self)
{
	float	range, luck;

//	gi.dprintf ("widow attack\n");
	
	if ((!self->enemy) || (!self->enemy->inuse))
		return;

	if (self->bad_area)
	{
		if ((random() < 0.1) || (level.time < self->timestamp))
			self->monsterinfo.currentmove = &widow_move_attack_pre_blaster;
		else
		{
			gi.sound (self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
			self->monsterinfo.currentmove = &widow_move_attack_pre_rail;
		}
		return;
	}

	// if we can't see the target, spawn stuff
	if ((self->monsterinfo.attack_state == AS_BLIND) && (blaster_frames))
	{
		self->monsterinfo.currentmove = &widow_move_spawn;
		return;
	}

	range = realrange (self, self->enemy);

	if (range < 600)
	{
		luck = random();
		if (SLOTS_LEFT >= 2)
		{
			if (luck <= 0.40)
				self->monsterinfo.currentmove = &widow_move_attack_pre_blaster;
			else if ((luck <= 0.7) && !(level.time < self->timestamp))
			{
				gi.sound (self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
				self->monsterinfo.currentmove = &widow_move_attack_pre_rail;
			}
			else
				self->monsterinfo.currentmove = &widow_move_spawn;
		}
		else
		{
			if ((luck <= 0.50) || (level.time < self->timestamp))
				self->monsterinfo.currentmove = &widow_move_attack_pre_blaster;
			else
			{
				gi.sound (self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
				self->monsterinfo.currentmove = &widow_move_attack_pre_rail;
			}
		}
	}
	else
	{
		luck = random();
		if (SLOTS_LEFT >= 2)
		{
			if (luck < 0.3)
				self->monsterinfo.currentmove = &widow_move_attack_pre_blaster;
			else if ((luck < 0.65) || (level.time < self->timestamp))
				self->monsterinfo.currentmove = &widow_move_spawn;
			else
			{
				gi.sound (self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
				self->monsterinfo.currentmove = &widow_move_attack_pre_rail;
			}
		}
		else
		{
			if ((luck < 0.45) || (level.time < self->timestamp))
				self->monsterinfo.currentmove = &widow_move_attack_pre_blaster;
			else
			{
				gi.sound (self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
				self->monsterinfo.currentmove = &widow_move_attack_pre_rail;
			}
		}
	}
}
*/
void widow_attack_blaster (edict_t *self)
{
	self->monsterinfo.pausetime = level.time + 1.0 + (2.0*random());
//	self->monsterinfo.pausetime = level.time + 100;
//	self->plat2flags = 0;
	self->monsterinfo.currentmove = &widow_move_attack_blaster;
	self->monsterinfo.nextframe = WidowTorso (self);
}

void widow_reattack_blaster (edict_t *self)
{
	WidowBlaster(self);

//	if ((g_showlogic) && (g_showlogic->value))
//	{
//		if (self->monsterinfo.currentmove == &widow_move_attack_post_blaster_l)
//			gi.dprintf ("pulling left!\n");
//		if (self->monsterinfo.currentmove == &widow_move_attack_post_blaster_r)
//			gi.dprintf ("pulling right!\n");
//	}

//	self->monsterinfo.currentmove = &widow_move_attack_blaster;
//		self->monsterinfo.aiflags |= AI_MANUAL_STEERING;
//	return;
	// if WidowBlaster bailed us out of the frames, just bail
	if ((self->monsterinfo.currentmove == &widow_move_attack_post_blaster_l) ||
		(self->monsterinfo.currentmove == &widow_move_attack_post_blaster_r))
		return;

	// if we're not done with the attack, don't leave the sequence
	if (self->monsterinfo.pausetime >= level.time)
		return;

	self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;

	self->monsterinfo.currentmove = &widow_move_attack_post_blaster;
}
/*
	if ( infront(self, self->enemy) )
		if (random() <= 0.5)
			if ((random() < 0.7) || (SLOTS_LEFT <= 1))
				self->monsterinfo.currentmove = &widow_move_attack_blaster;
			else
				self->monsterinfo.currentmove = &widow_move_spawn;
		else
			self->monsterinfo.currentmove = &widow_move_attack_post_blaster;
	else
		self->monsterinfo.currentmove = &widow_move_attack_post_blaster;
}
*/


void widow_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (skill->value == 3)
		return;		// no pain anims in nightmare

	if (level.time < self->pain_debounce_time)
		return;

	if (self->monsterinfo.pausetime == 100000000)
		self->monsterinfo.pausetime = 0;

	self->pain_debounce_time = level.time + 5;

	if (damage < 15)
	{
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NONE, 0);
	}
	else if (damage < 75)
	{
		if ((skill->value < 3) && (random() < (0.6 - (0.2*((float)skill->value)))))
		{
			self->monsterinfo.currentmove = &widow_move_pain_light;
			self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
		}
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NONE, 0);
	}
	else 
	{
		if ((skill->value < 3) && (random() < (0.75 - (0.1*((float)skill->value)))))
		{
			self->monsterinfo.currentmove = &widow_move_pain_heavy;
			self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
		}
		gi.sound (self, CHAN_VOICE, sound_pain3, 1, ATTN_NONE, 0);
	}
}

void widow_dead (edict_t *self)
{
	VectorSet (self->mins, -56, -56, 0);
	VectorSet (self->maxs, 56, 56, 80);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity (self);
}

void widow_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_NO;
	self->count = 0;
	self->monsterinfo.quad_framenum = 0;
	self->monsterinfo.double_framenum = 0;
	self->monsterinfo.invincible_framenum = 0;
	self->monsterinfo.currentmove = &widow_move_death;
}

void widow_melee (edict_t *self)
{
//	monster_done_dodge (self);
	self->monsterinfo.currentmove = &widow_move_attack_kick;
}

void WidowGoinQuad (edict_t *self, float framenum)
{
	self->monsterinfo.quad_framenum = framenum;
	widow_damage_multiplier = 4;
}

void WidowDouble (edict_t *self, float framenum)
{
	self->monsterinfo.double_framenum = framenum;
	widow_damage_multiplier = 2;
}

void WidowPent (edict_t *self, float framenum)
{
	self->monsterinfo.invincible_framenum = framenum;
}

void WidowPowerArmor (edict_t *self)
{
	self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
	// I don't like this, but it works
	if (self->monsterinfo.power_armor_power <= 0)
		self->monsterinfo.power_armor_power += 250 * skill->value;
}

void WidowRespondPowerup (edict_t *self, edict_t *other)
{
	if (other->s.effects & EF_QUAD)
	{
		if (skill->value == 1)
			WidowDouble (self, other->client->quad_framenum);
		else if (skill->value == 2)
			WidowGoinQuad (self, other->client->quad_framenum);
		else if (skill->value == 3)
		{
			WidowGoinQuad (self, other->client->quad_framenum);
			WidowPowerArmor (self);
		}
	}
	else if (other->s.effects & EF_DOUBLE)
	{
		if (skill->value == 2)
			WidowDouble (self, other->client->double_framenum);
		else if (skill->value == 3)
		{
			WidowDouble (self, other->client->double_framenum);
			WidowPowerArmor (self);
		}
	}
	else
		widow_damage_multiplier = 1;

	if (other->s.effects & EF_PENT)
	{
		if (skill->value == 1)
			WidowPowerArmor (self);
		else if (skill->value == 2)
			WidowPent (self, other->client->invincible_framenum);
		else if (skill->value == 3)
		{
			WidowPent (self, other->client->invincible_framenum);
			WidowPowerArmor (self);
		}
	}
}

void WidowPowerups (edict_t *self)
{
	int player;
	edict_t *ent;

	if (!(coop && coop->value))
	{
		WidowRespondPowerup (self, self->enemy);
	}
	else
	{
		// in coop, check for pents, then quads, then doubles
		for (player = 1; player <= game.maxclients; player++)
		{
			ent = &g_edicts[player];
			if (!ent->inuse)
				continue;
			if (!ent->client)
				continue;
			if (ent->s.effects & EF_PENT)
			{
				WidowRespondPowerup (self, ent);
				return;
			}
		}

		for (player = 1; player <= game.maxclients; player++)
		{
			ent = &g_edicts[player];
			if (!ent->inuse)
				continue;
			if (!ent->client)
				continue;
			if (ent->s.effects & EF_QUAD)
			{
				WidowRespondPowerup (self, ent);
				return;
			}
		}

		for (player = 1; player <= game.maxclients; player++)
		{
			ent = &g_edicts[player];
			if (!ent->inuse)
				continue;
			if (!ent->client)
				continue;
			if (ent->s.effects & EF_DOUBLE)
			{
				WidowRespondPowerup (self, ent);
				return;
			}
		}
	}
}

qboolean Widow_CheckAttack (edict_t *self)
{
	vec3_t	spot1, spot2;
	vec3_t	temp;
	float	chance;
	trace_t	tr;
	qboolean	enemy_infront;
	int			enemy_range;
	float		enemy_yaw;
	float		real_enemy_range;

	if (!self->enemy)
		return false;

	WidowPowerups(self);

	if (self->monsterinfo.currentmove == &widow_move_run)
	{
		// if we're in run, make sure we're in a good frame for attacking before doing anything else
		// frames 1,2,3,9,10,11,13 good to fire
		switch (self->s.frame)
		{
			case FRAME_walk04:
			case FRAME_walk05:
			case FRAME_walk06:
			case FRAME_walk07:
			case FRAME_walk08:
			case FRAME_walk12:
				{
//					if ((g_showlogic) && (g_showlogic->value))
//						gi.dprintf ("Not in good walk frame (%d), not attacking\n", (self->s.frame - FRAME_walk01+1));
					return false;
				}
			default:
				break;
		}
	}

	// give a LARGE bias to spawning things when we have room
	// use AI_BLOCKED as a signal to attack to spawn
	if ((random() < 0.8) && (SELF_SLOTS_LEFT >= 2) && (realrange(self, self->enemy) > 150))
	{
		self->monsterinfo.aiflags |= AI_BLOCKED;
		self->monsterinfo.attack_state = AS_MISSILE;
		return true;
	}

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
			// go ahead and spawn stuff if we're mad a a client
			if (self->enemy->client && SELF_SLOTS_LEFT >= 2)
			{
				self->monsterinfo.attack_state = AS_BLIND;
				return true;
			}
				
			// PGM - we want them to go ahead and shoot at info_notnulls if they can.
			if(self->enemy->solid != SOLID_NOT || tr.fraction < 1.0)		//PGM
				return false;
		}
	}
	
	enemy_infront = infront(self, self->enemy);

	enemy_range = range(self, self->enemy);
	VectorSubtract (self->enemy->s.origin, self->s.origin, temp);
	enemy_yaw = vectoyaw2(temp);

	self->ideal_yaw = enemy_yaw;

	real_enemy_range = realrange (self, self->enemy);

//	if (g_showlogic->value)
//		gi.dprintf ("range = %2.2f\n", real_enemy_range);

	// melee attack
//	if (enemy_range == RANGE_MELEE)
	if (real_enemy_range <= (MELEE_DISTANCE+20))
	{
		// don't always melee in easy mode
		if (skill->value == 0 && (rand()&3) )
			return false;
		if (self->monsterinfo.melee)
			self->monsterinfo.attack_state = AS_MELEE;
		else
			self->monsterinfo.attack_state = AS_MISSILE;
		return true;
	}

	if (level.time < self->monsterinfo.attack_finished)
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
		chance = 0.7;
	}
	else if (enemy_range == RANGE_MID)
	{
		chance = 0.6;
	}
	else if (enemy_range == RANGE_FAR)
	{
		chance = 0.5;
	}

	// PGM - go ahead and shoot every time if it's a info_notnull
	if ((random () < chance) || (self->enemy->solid == SOLID_NOT))
	{
		self->monsterinfo.attack_state = AS_MISSILE;
		return true;
	}

	return false;
}

qboolean widow_blocked (edict_t *self, float dist)
{
	// if we get blocked while we're in our run/attack mode, turn on a meaningless (in this context)AI flag, 
	// and call attack to get a new attack sequence.  make sure to turn it off when we're done.
	//
	// I'm using AI_TARGET_ANGER for this purpose

	if (self->monsterinfo.currentmove == &widow_move_run_attack)
	{
		self->monsterinfo.aiflags |= AI_TARGET_ANGER;
		if (self->monsterinfo.checkattack(self))
			self->monsterinfo.attack(self);
		else
			self->monsterinfo.run(self);
		return true;
	}

	if(blocked_checkshot (self, 0.25 + (0.05 * skill->value) ))
		return true;

/*
	if(blocked_checkjump (self, dist, 192, 40))
	{
		infantry_jump(self);
		return true;
	}

	if(blocked_checkplat (self, dist))
		return true;
*/
	return false;
}

void WidowCalcSlots (edict_t *self)
{
	int old_slots;

	old_slots = self->monsterinfo.monster_slots;

	switch ((int)skill->value)
	{
		case 0:
		case 1:
			self->monsterinfo.monster_slots = 3;
			break;
		case 2:
			self->monsterinfo.monster_slots = 4;
			break;
		case 3:
			self->monsterinfo.monster_slots = 6;
			break;
		default:
			self->monsterinfo.monster_slots = 3;
			break;
	}
	if (coop->value)
	{
		self->monsterinfo.monster_slots = min (6, self->monsterinfo.monster_slots + ((skill->value)*(CountPlayers()-1)));
	}
//	if ((g_showlogic) && (g_showlogic->value) && (old_slots != self->monsterinfo.monster_slots))
//		gi.dprintf ("number of slots changed from %d to %d\n", old_slots, self->monsterinfo.monster_slots);
}

void WidowPrecache ()
{
	// cache in all of the stalker stuff, widow stuff, spawngro stuff, gibs
	gi.soundindex ("stalker/pain.wav");	
	gi.soundindex ("stalker/death.wav");	
	gi.soundindex ("stalker/sight.wav");
	gi.soundindex ("stalker/melee1.wav");
	gi.soundindex ("stalker/melee2.wav");
	gi.soundindex ("stalker/idle.wav");

	gi.soundindex ("tank/tnkatck3.wav");
	gi.modelindex ("models/proj/laser2/tris.md2");

	gi.modelindex ("models/monsters/stalker/tris.md2");
	gi.modelindex ("models/items/spawngro2/tris.md2");
	gi.modelindex ("models/objects/gibs/sm_metal/tris.md2");
	gi.modelindex ("models/objects/gibs/gear/tris.md2");
	gi.modelindex ("models/monsters/blackwidow/gib1/tris.md2");
	gi.modelindex ("models/monsters/blackwidow/gib2/tris.md2");
	gi.modelindex ("models/monsters/blackwidow/gib3/tris.md2");
	gi.modelindex ("models/monsters/blackwidow/gib4/tris.md2");
	gi.modelindex ("models/monsters/blackwidow2/gib1/tris.md2");
	gi.modelindex ("models/monsters/blackwidow2/gib2/tris.md2");
	gi.modelindex ("models/monsters/blackwidow2/gib3/tris.md2");
	gi.modelindex ("models/monsters/blackwidow2/gib4/tris.md2");
	gi.modelindex ("models/monsters/legs/tris.md2");
	gi.soundindex ("misc/bwidowbeamout.wav");

	gi.soundindex ("misc/bigtele.wav");
	gi.soundindex ("widow/bwstep3.wav");
	gi.soundindex ("widow/bwstep2.wav");
}


/*QUAKED monster_widow (1 .5 0) (-40 -40 0) (40 40 144) Ambush Trigger_Spawn Sight
*/
void SP_monster_widow (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	sound_pain1 = gi.soundindex ("widow/bw1pain1.wav");
	sound_pain2 = gi.soundindex ("widow/bw1pain2.wav");
	sound_pain3 = gi.soundindex ("widow/bw1pain3.wav");
	sound_search1 = gi.soundindex ("bosshovr/bhvunqv1.wav");
//	sound_sight	= gi.soundindex ("widow/sight.wav");
	sound_rail = gi.soundindex ("gladiator/railgun.wav");

//	self->s.sound = gi.soundindex ("bosshovr/bhvengn1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex ("models/monsters/blackwidow/tris.md2");
	VectorSet (self->mins, -40, -40, 0);
	VectorSet (self->maxs, 40, 40, 144);

	self->health = 2000 + 1000*(skill->value);
	if (coop->value)
		self->health += 500*(skill->value);
//	self->health = 1;
	self->gib_health = -5000;
	self->mass = 1500;
/*
	if (skill->value == 2)
	{
		self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
		self->monsterinfo.power_armor_power = 250;
	}
	else */if (skill->value == 3)
	{
		self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
		self->monsterinfo.power_armor_power = 500;
	}

	self->yaw_speed = 30;
	
	self->flags |= FL_IMMUNE_LASER;
	self->monsterinfo.aiflags |= AI_IGNORE_SHOTS;

	self->pain = widow_pain;
	self->die = widow_die;

	self->monsterinfo.melee = widow_melee;
	self->monsterinfo.stand = widow_stand;
	self->monsterinfo.walk = widow_walk;
	self->monsterinfo.run = widow_run;
	self->monsterinfo.attack = widow_attack;
	self->monsterinfo.search = widow_search;
	self->monsterinfo.checkattack = Widow_CheckAttack;
	self->monsterinfo.sight = widow_sight;
	
	self->monsterinfo.blocked = widow_blocked;

	gi.linkentity (self);

	self->monsterinfo.currentmove = &widow_move_stand;	
	self->monsterinfo.scale = MODEL_SCALE;

	WidowPrecache();
	WidowCalcSlots(self);
	widow_damage_multiplier = 1;

	walkmonster_start (self);
}