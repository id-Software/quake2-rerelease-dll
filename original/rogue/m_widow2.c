// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

black widow, part 2

==============================================================================
*/

// timestamp used to prevent rapid fire of melee attack

#include "g_local.h"
#include "m_widow2.h"

#define	NUM_STALKERS_SPAWNED		6		// max # of stalkers she can spawn

#define	DISRUPT_TIME					3

static int	sound_pain1;
static int	sound_pain2;
static int	sound_pain3;
static int	sound_death;
static int	sound_search1;
static int	sound_disrupt;
static int	sound_tentacles_retract;

// sqrt(64*64*2) + sqrt(28*28*2) => 130.1
static vec3_t spawnpoints[] = {
	{30,  135, 0},
	{30, -135, 0}
};

static float sweep_angles[] = {
	-40.0, -32.0, -24.0, -16.0, -8.0, 0.0, 8.0, 16.0, 24.0, 32.0, 40.0
};

extern vec3_t	stalker_mins, stalker_maxs;

qboolean infront (edict_t *self, edict_t *other);
void WidowCalcSlots (edict_t *self);
void WidowPowerups (edict_t *self);

void widow2_run (edict_t *self);
void widow2_stand (edict_t *self);
void widow2_dead (edict_t *self);
void widow2_attack (edict_t *self);
void widow2_attack_beam (edict_t *self);
void widow2_reattack_beam (edict_t *self);
void widow2_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);
void widow_start_spawn (edict_t *self);
void widow_done_spawn (edict_t *self);
void widow2_spawn_check (edict_t *self);
void widow2_prep_spawn (edict_t *self);
void Widow2SaveBeamTarget(edict_t *self);

// death stuff
void WidowExplode (edict_t *self);
void gib_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);
void gib_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);
void ThrowWidowGibReal (edict_t *self, char *gibname, int damage, int type, vec3_t startpos, qboolean large, int hitsound, qboolean fade);
void ThrowWidowGibSized (edict_t *self, char *gibname, int damage, int type, vec3_t startpos, int hitsound, qboolean fade);
void ThrowWidowGibLoc (edict_t *self, char *gibname, int damage, int type, vec3_t startpos, qboolean fade);
void WidowExplosion1 (edict_t *self);
void WidowExplosion2 (edict_t *self);
void WidowExplosion3 (edict_t *self);
void WidowExplosion4 (edict_t *self);
void WidowExplosion5 (edict_t *self);
void WidowExplosion6 (edict_t *self);
void WidowExplosion7 (edict_t *self);
void WidowExplosionLeg (edict_t *self);
void ThrowArm1 (edict_t *self);
void ThrowArm2 (edict_t *self);
void ClipGibVelocity (edict_t *ent);
// end of death stuff

// these offsets used by the tongue
static vec3_t offsets[] = {
	{17.48, 0.10, 68.92},
	{17.47, 0.29, 68.91},
	{17.45, 0.53, 68.87},
	{17.42, 0.78, 68.81},
	{17.39, 1.02, 68.75},
	{17.37, 1.20, 68.70},
	{17.36, 1.24, 68.71},
	{17.37, 1.21, 68.72},
};

void showme (edict_t *self);

void pauseme (edict_t *self)
{
	self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

void widow2_search (edict_t *self)
{
	if (random() < 0.5)
		gi.sound (self, CHAN_VOICE, sound_search1, 1, ATTN_NONE, 0);
}

void Widow2Beam (edict_t *self)
{
	vec3_t	forward, right, target;
	vec3_t	start, targ_angles, vec;
	int		flashnum;

	if ((!self->enemy) || (!self->enemy->inuse))
		return;

	AngleVectors (self->s.angles, forward, right, NULL);
	
	if ((self->s.frame >= FRAME_fireb05) && (self->s.frame <= FRAME_fireb09))
	{
		// regular beam attack
		Widow2SaveBeamTarget(self);
		flashnum = MZ2_WIDOW2_BEAMER_1 + self->s.frame - FRAME_fireb05;
		G_ProjectSource (self->s.origin, monster_flash_offset[flashnum], forward, right, start);
		VectorCopy (self->pos2, target);
		target[2] += self->enemy->viewheight-10;
		VectorSubtract (target, start, forward);
		VectorNormalize (forward);
		monster_fire_heat (self, start, forward, vec3_origin, 10, 50, flashnum);
	}
	else if ((self->s.frame >= FRAME_spawn04) && (self->s.frame <= FRAME_spawn14))
	{
		// sweep
		flashnum = MZ2_WIDOW2_BEAM_SWEEP_1 + self->s.frame - FRAME_spawn04;
		G_ProjectSource (self->s.origin, monster_flash_offset[flashnum], forward, right, start);
		VectorSubtract (self->enemy->s.origin, start, target);
		vectoangles2 (target, targ_angles);
		
		VectorCopy (self->s.angles, vec);

		vec[PITCH] += targ_angles[PITCH];
		vec[YAW] -= sweep_angles[flashnum-MZ2_WIDOW2_BEAM_SWEEP_1];

		AngleVectors (vec, forward, NULL, NULL);
		monster_fire_heat (self, start, forward, vec3_origin, 10, 50, flashnum);
/*
		if (self->s.frame == FRAME_spawn04)
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
	else
	{
//		if ((g_showlogic) && (g_showlogic->value))
//			gi.dprintf ("bad fire frame for widow2 beam -- tell me you saw this!\n");

		Widow2SaveBeamTarget(self);
		G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_WIDOW2_BEAMER_1], forward, right, start);

		VectorCopy (self->pos2, target);
		target[2] += self->enemy->viewheight-10;
		
		VectorSubtract (target, start, forward);
		VectorNormalize (forward);

		monster_fire_heat (self, start, forward, vec3_origin, 10, 50, 0);
	}	
}

void Widow2Spawn (edict_t *self)
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
//				gi.dprintf ("widow: post-spawn : %d slots left\n", SELF_SLOTS_LEFT);

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

void widow2_spawn_check (edict_t *self)
{
	Widow2Beam(self);
	Widow2Spawn (self);
}

void widow2_ready_spawn (edict_t *self)
{
	vec3_t	f, r, u, offset, startpoint, spawnpoint;
	int		i;

	Widow2Beam(self);
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

mframe_t widow2_frames_stand [] =
{
//	ai_stand, 0, drawbbox
	ai_stand, 0, NULL
};
mmove_t	widow2_move_stand = {FRAME_blackwidow3, FRAME_blackwidow3, widow2_frames_stand, NULL};

mframe_t widow2_frames_walk [] =
{
//	ai_walk,	9.01,	drawbbox,
	ai_walk,	9.01,	NULL,
	ai_walk,	7.55,	NULL,
	ai_walk,	7.01,	NULL,
	ai_walk,	6.66,	NULL,
	ai_walk,	6.20,	NULL,
	ai_walk,	5.78,	NULL,
	ai_walk,	7.25,	NULL,
	ai_walk,	8.37,	NULL,
	ai_walk,	10.41,	NULL
};
mmove_t widow2_move_walk = {FRAME_walk01, FRAME_walk09, widow2_frames_walk, NULL};


mframe_t widow2_frames_run [] =
{
//	ai_run,	9.01,	drawbbox,
	ai_run,	9.01,	NULL,
	ai_run,	7.55,	NULL,
	ai_run,	7.01,	NULL,
	ai_run,	6.66,	NULL,
	ai_run,	6.20,	NULL,
	ai_run,	5.78,	NULL,
	ai_run,	7.25,	NULL,
	ai_run,	8.37,	NULL,
	ai_run,	10.41,	NULL
};
mmove_t widow2_move_run = {FRAME_walk01, FRAME_walk09, widow2_frames_run, NULL};

mframe_t widow2_frames_attack_pre_beam [] =
{
	ai_charge,	4,	NULL,
	ai_charge,	4,	NULL,
	ai_charge,	4,	NULL,
	ai_charge,	4,	widow2_attack_beam
};
mmove_t widow2_move_attack_pre_beam = {FRAME_fireb01, FRAME_fireb04, widow2_frames_attack_pre_beam, NULL};


// Loop this
mframe_t widow2_frames_attack_beam [] =
{
	ai_charge,	0,	Widow2Beam,
	ai_charge,	0,	Widow2Beam,
	ai_charge,	0,	Widow2Beam,
	ai_charge,	0,	Widow2Beam,
	ai_charge,	0,	widow2_reattack_beam
};
mmove_t widow2_move_attack_beam = {FRAME_fireb05, FRAME_fireb09, widow2_frames_attack_beam, NULL};

mframe_t widow2_frames_attack_post_beam [] =
{
	ai_charge,	4,	NULL,
	ai_charge,	4,	NULL,
	ai_charge,	4,	NULL
};
mmove_t widow2_move_attack_post_beam = {FRAME_fireb06, FRAME_fireb07, widow2_frames_attack_post_beam, widow2_run};


void WidowDisrupt (edict_t *self)
{
	vec3_t	start;
	vec3_t	dir;
	vec3_t	forward, right;
	float	len;

	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_WIDOW_DISRUPTOR], forward, right, start);

	VectorSubtract (self->pos1, self->enemy->s.origin, dir);
	len = VectorLength (dir);

	if (len < 30)
	{
//		if ((g_showlogic) && (g_showlogic->value))
//			gi.dprintf ("target locked - dist %2.2f\n", len);
		// calc direction to where we targeted
		VectorSubtract (self->pos1, start, dir);
		VectorNormalize (dir);

		monster_fire_tracker(self, start, dir, 20, 500, self->enemy, MZ2_WIDOW_DISRUPTOR);
	}
	else
	{
//		if ((g_showlogic) && (g_showlogic->value))
//			gi.dprintf ("target missed - dist %2.2f\n", len);

		PredictAim (self->enemy, start, 1200, true, 0, dir, NULL);

//		VectorSubtract (self->enemy->s.origin, start, dir);
//		VectorNormalize (dir);
		monster_fire_tracker(self, start, dir, 20, 1200, NULL, MZ2_WIDOW_DISRUPTOR);
	}
}

void Widow2SaveDisruptLoc (edict_t *self)
{
	if (self->enemy && self->enemy->inuse)
	{
		VectorCopy (self->enemy->s.origin, self->pos1);	//save for aiming the shot
		self->pos1[2] += self->enemy->viewheight;
	}
	else
		VectorCopy (vec3_origin, self->pos1);
};

void widow2_disrupt_reattack (edict_t *self)
{
	float luck;
	
	luck = random();

	if (luck < (0.25 + ((float)(skill->value))*0.15))
		self->monsterinfo.nextframe = FRAME_firea01;
}

mframe_t widow2_frames_attack_disrupt [] =
{
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, Widow2SaveDisruptLoc,
	ai_charge, -20, WidowDisrupt,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, widow2_disrupt_reattack
};
mmove_t widow2_move_attack_disrupt = {FRAME_firea01, FRAME_firea07, widow2_frames_attack_disrupt, widow2_run};

void Widow2SaveBeamTarget (edict_t *self)
{
	if (self->enemy && self->enemy->inuse)
	{
		VectorCopy (self->pos1, self->pos2);
		VectorCopy (self->enemy->s.origin, self->pos1);	//save for aiming the shot
	}
	else
	{
		VectorCopy (vec3_origin, self->pos1);
		VectorCopy (vec3_origin, self->pos2);
	}
}

void Widow2BeamTargetRemove (edict_t *self)
{
	VectorCopy (vec3_origin, self->pos1);
	VectorCopy (vec3_origin, self->pos2);
}

void Widow2StartSweep (edict_t *self)
{
	Widow2SaveBeamTarget (self);
}

mframe_t widow2_frames_spawn [] =
{
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	widow_start_spawn,
	ai_charge,	0,	Widow2Beam,
	ai_charge,	0,	Widow2Beam,				//5
	ai_charge,	0,	Widow2Beam,
	ai_charge,	0,	Widow2Beam,
	ai_charge,	0,	Widow2Beam,
	ai_charge,	0,	Widow2Beam,
	ai_charge,	0,	widow2_ready_spawn,				//10
	ai_charge,	0,	Widow2Beam,
	ai_charge,	0,	Widow2Beam,
	ai_charge,	0,	Widow2Beam,
	ai_charge,	0,	widow2_spawn_check,
	ai_charge,	0,	NULL,				//15
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	widow2_reattack_beam
};
mmove_t widow2_move_spawn = {FRAME_spawn01, FRAME_spawn18, widow2_frames_spawn, NULL};

static qboolean widow2_tongue_attack_ok (vec3_t start, vec3_t end, float range)
{
	vec3_t	dir, angles;

	// check for max distance
	VectorSubtract (start, end, dir);
	if (VectorLength(dir) > range)
		return false;

	// check for min/max pitch
	vectoangles (dir, angles);
	if (angles[0] < -180)
		angles[0] += 360;
	if (fabs(angles[0]) > 30)
		return false;

	return true;
}

void Widow2Tongue (edict_t *self)
{
	vec3_t	f, r, u;
	vec3_t	start, end, dir;
	trace_t	tr;

	AngleVectors (self->s.angles, f, r, u);
	G_ProjectSource2 (self->s.origin, offsets[self->s.frame - FRAME_tongs01], f, r, u, start);
	VectorCopy (self->enemy->s.origin, end);
	if (!widow2_tongue_attack_ok(start, end, 256))
	{
		end[2] = self->enemy->s.origin[2] + self->enemy->maxs[2] - 8;
		if (!widow2_tongue_attack_ok(start, end, 256))
		{
			end[2] = self->enemy->s.origin[2] + self->enemy->mins[2] + 8;
			if (!widow2_tongue_attack_ok(start, end, 256))
				return;
		}
	}
	VectorCopy (self->enemy->s.origin, end);

	tr = gi.trace (start, NULL, NULL, end, self, MASK_SHOT);
	if (tr.ent != self->enemy)
		return;

	gi.sound (self, CHAN_WEAPON, sound_tentacles_retract, 1, ATTN_NORM, 0);

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_PARASITE_ATTACK);
	gi.WriteShort (self - g_edicts);
	gi.WritePosition (start);
	gi.WritePosition (end);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	VectorSubtract (start, end, dir);
	T_Damage (self->enemy, self, self, dir, self->enemy->s.origin, vec3_origin, 2, 0, DAMAGE_NO_KNOCKBACK, MOD_UNKNOWN);
}

void Widow2TonguePull (edict_t *self)
{
	vec3_t	vec;
	float	len;
	vec3_t	f, r, u;
	vec3_t	start, end;

	if ((!self->enemy) || (!self->enemy->inuse))
	{
		self->monsterinfo.run (self);
		return;
	}

	AngleVectors (self->s.angles, f, r, u);
	G_ProjectSource2 (self->s.origin, offsets[self->s.frame - FRAME_tongs01], f, r, u, start);
	VectorCopy (self->enemy->s.origin, end);

	if (!widow2_tongue_attack_ok(start, end, 256))
	{
		return;
	}

	if (self->enemy->groundentity)
	{
		self->enemy->s.origin[2] += 1;
		self->enemy->groundentity = NULL;
		// interesting, you don't have to relink the player
	}
	
	VectorSubtract (self->s.origin, self->enemy->s.origin, vec);
	len = VectorLength (vec);
	if (self->enemy->client)
	{
		VectorNormalize (vec);
		VectorMA (self->enemy->velocity, 1000, vec, self->enemy->velocity);
	}
	else
	{
		self->enemy->ideal_yaw = vectoyaw(vec);	
		M_ChangeYaw (self->enemy);
		VectorScale (f, 1000, self->enemy->velocity);
	}
}

void Widow2Crunch (edict_t *self)
{
	vec3_t	aim;

	if ((!self->enemy) || (!self->enemy->inuse))
	{
		self->monsterinfo.run (self);
		return;
	}

	Widow2TonguePull (self);

	// 70 + 32
	VectorSet (aim, 150, 0, 4);
	if (self->s.frame != FRAME_tongs07)
		fire_hit (self, aim, 20 + (rand() % 6), 0);
	else
	{
		if (self->enemy->groundentity)
			fire_hit (self, aim, (20 + (rand() % 6)), 500);
		else	// not as much kick if they're in the air .. makes it harder to land on her head
			fire_hit (self, aim, (20 + (rand() % 6)), 250);
	}
}

void Widow2Toss (edict_t *self)
{
	self->timestamp = level.time + 3;
	return;
}

mframe_t widow2_frames_tongs [] =
{
	ai_charge,	0,	Widow2Tongue,
	ai_charge,	0,	Widow2Tongue,
	ai_charge,	0,	Widow2Tongue,
	ai_charge,	0,	Widow2TonguePull,
	ai_charge,	0,	Widow2TonguePull,				//5
	ai_charge,	0,	Widow2TonguePull,
	ai_charge,	0,	Widow2Crunch,
	ai_charge,	0,	Widow2Toss
};
mmove_t widow2_move_tongs = {FRAME_tongs01, FRAME_tongs08, widow2_frames_tongs, widow2_run};

mframe_t widow2_frames_pain [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL
};
mmove_t widow2_move_pain = {FRAME_pain01, FRAME_pain05, widow2_frames_pain, widow2_run};

mframe_t widow2_frames_death [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	WidowExplosion1,	// 3 boom
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,				// 5

	ai_move,	0,	WidowExplosion2,	// 6 boom
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,				// 10

	ai_move,	0,	NULL,
	ai_move,	0,	NULL,				// 12
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,				// 15

	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	WidowExplosion3,	// 18
	ai_move,	0,	NULL,				// 19
	ai_move,	0,	NULL,				// 20

	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	WidowExplosion4,	// 25

	ai_move,	0,	NULL,				// 26
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	WidowExplosion5,
	ai_move,	0,	WidowExplosionLeg,	// 30

	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	WidowExplosion6,
	ai_move,	0,	NULL,				// 35

	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	WidowExplosion7,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,				// 40

	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	WidowExplode		// 44
};
mmove_t widow2_move_death = {FRAME_death01, FRAME_death44, widow2_frames_death, NULL};

void widow2_start_searching (edict_t *self);
void widow2_keep_searching (edict_t *self);
void widow2_finaldeath (edict_t *self);

mframe_t widow2_frames_dead [] =
{
	ai_move,	0,	widow2_start_searching,
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
	ai_move,	0,	widow2_keep_searching
};
mmove_t widow2_move_dead = {FRAME_dthsrh01, FRAME_dthsrh15, widow2_frames_dead, NULL};

mframe_t widow2_frames_really_dead [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,

	ai_move,	0,	NULL,
	ai_move,	0,	widow2_finaldeath
};
mmove_t widow2_move_really_dead = {FRAME_dthsrh16, FRAME_dthsrh22, widow2_frames_really_dead, NULL};

void widow2_start_searching (edict_t *self)
{
	self->count = 0;
}

void widow2_keep_searching (edict_t *self)
{
	if (self->count <= 2)
	{
		self->monsterinfo.currentmove = &widow2_move_dead;
		self->s.frame = FRAME_dthsrh01;
		self->count++;
		return;
	}

	self->monsterinfo.currentmove = &widow2_move_really_dead;
}

void widow2_finaldeath (edict_t *self)
{
	VectorSet (self->mins, -70, -70, 0);
	VectorSet (self->maxs, 70, 70, 80);
	self->movetype = MOVETYPE_TOSS;
//	self->svflags |= SVF_DEADMONSTER;
	self->takedamage = DAMAGE_YES;
	self->nextthink = 0;
	gi.linkentity (self);
}

void widow2_stand (edict_t *self)
{
//	gi.dprintf ("widow2 stand\n");
	self->monsterinfo.currentmove = &widow2_move_stand;
}

void widow2_run (edict_t *self)
{

//	gi.dprintf ("widow2 run - %2.2f - %s \n", level.time, self->enemy->classname);
	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &widow2_move_stand;
	else
		self->monsterinfo.currentmove = &widow2_move_run;
}

void widow2_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &widow2_move_walk;
}

void widow2_melee (edict_t *self)
{
	self->monsterinfo.currentmove = &widow2_move_tongs;
}

void widow2_attack (edict_t *self)
{
	float	range, luck;
	qboolean blocked = false;

	if (self->monsterinfo.aiflags & AI_BLOCKED)
	{
		blocked = true;
		self->monsterinfo.aiflags &= ~AI_BLOCKED;
	}

//	gi.dprintf ("widow2 attack\n");
	
	if (!self->enemy)
		return;

	if (self->bad_area)
	{
		if ((random() < 0.75) || (level.time < self->monsterinfo.attack_finished))
			self->monsterinfo.currentmove = &widow2_move_attack_pre_beam;
		else
		{
			self->monsterinfo.currentmove = &widow2_move_attack_disrupt;
		}
		return;
	}

	WidowCalcSlots(self);

	// if we can't see the target, spawn stuff
	if ((self->monsterinfo.attack_state == AS_BLIND) && (SELF_SLOTS_LEFT >= 2))
	{
		self->monsterinfo.currentmove = &widow2_move_spawn;
		return;
	}

	// accept bias towards spawning
	if (blocked  && (SELF_SLOTS_LEFT >= 2))
	{
		self->monsterinfo.currentmove = &widow2_move_spawn;
		return;
	}

	range = realrange (self, self->enemy);

	if (range < 600)
	{
		luck = random();
		if (SELF_SLOTS_LEFT >= 2)
		{
			if (luck <= 0.40)
				self->monsterinfo.currentmove = &widow2_move_attack_pre_beam;
			else if ((luck <= 0.7) && !(level.time < self->monsterinfo.attack_finished))
			{
//				gi.sound (self, CHAN_WEAPON, sound_disrupt, 1, ATTN_NORM, 0);
				self->monsterinfo.currentmove = &widow2_move_attack_disrupt;
			}
			else
				self->monsterinfo.currentmove = &widow2_move_spawn;
		}
		else
		{
			if ((luck <= 0.50) || (level.time < self->monsterinfo.attack_finished))
				self->monsterinfo.currentmove = &widow2_move_attack_pre_beam;
			else
			{
//				gi.sound (self, CHAN_WEAPON, sound_disrupt, 1, ATTN_NORM, 0);
				self->monsterinfo.currentmove = &widow2_move_attack_disrupt;
			}
		}
	}
	else
	{
		luck = random();
		if (SELF_SLOTS_LEFT >= 2)
		{
			if (luck < 0.3)
				self->monsterinfo.currentmove = &widow2_move_attack_pre_beam;
			else if ((luck < 0.65) || (level.time < self->monsterinfo.attack_finished))
				self->monsterinfo.currentmove = &widow2_move_spawn;
			else
			{
//				gi.sound (self, CHAN_WEAPON, sound_disrupt, 1, ATTN_NORM, 0);
				self->monsterinfo.currentmove = &widow2_move_attack_disrupt;
			}
		}
		else
		{
			if ((luck < 0.45) || (level.time < self->monsterinfo.attack_finished))
				self->monsterinfo.currentmove = &widow2_move_attack_pre_beam;
			else
			{
//				gi.sound (self, CHAN_WEAPON, sound_disrupt, 1, ATTN_NORM, 0);
				self->monsterinfo.currentmove = &widow2_move_attack_disrupt;
			}
		}
	}
}

void widow2_attack_beam (edict_t *self)
{
	self->monsterinfo.currentmove = &widow2_move_attack_beam;
}

void widow2_reattack_beam (edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;

	if ( infront(self, self->enemy) )
		if (random() <= 0.5)
			if ((random() < 0.7) || (SELF_SLOTS_LEFT < 2))
				self->monsterinfo.currentmove = &widow2_move_attack_beam;
			else
				self->monsterinfo.currentmove = &widow2_move_spawn;
		else
			self->monsterinfo.currentmove = &widow2_move_attack_post_beam;
	else
		self->monsterinfo.currentmove = &widow2_move_attack_post_beam;
}



void widow2_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (skill->value == 3)
		return;		// no pain anims in nightmare

//	gi.dprintf ("widow2 pain\n");
	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 5;

	if (damage < 15)
	{
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NONE, 0);
	}
	else if (damage < 75)
	{
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NONE, 0);
		if ((skill->value < 3) && (random() < (0.6 - (0.2*((float)skill->value)))))
		{
			self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
			self->monsterinfo.currentmove = &widow2_move_pain;
		}
	}
	else 
	{
		gi.sound (self, CHAN_VOICE, sound_pain3, 1, ATTN_NONE, 0);
		if ((skill->value < 3) && (random() < (0.75 - (0.1*((float)skill->value)))))
		{
			self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
			self->monsterinfo.currentmove = &widow2_move_pain;
		}
	}
}

void widow2_dead (edict_t *self)
{
}

void KillChildren (edict_t *self)
{
	edict_t *ent;
	int		field;

	ent = NULL;
	field = FOFS(classname);
	while (1)
	{
		ent = G_Find (ent, field, "monster_stalker");
		if(!ent)
			return;
		
		// FIXME - may need to stagger
		if ((ent->inuse) && (ent->health > 0))
			T_Damage (ent, self, self, vec3_origin, self->enemy->s.origin, vec3_origin, (ent->health + 1), 0, DAMAGE_NO_KNOCKBACK, MOD_UNKNOWN);
	}
}

void widow2_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int n;
	int	clipped;

// check for gib
	if (self->health <= self->gib_health)
	{
		clipped = min (damage, 100);

		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n= 0; n < 2; n++)
			ThrowWidowGibLoc (self, "models/objects/gibs/bone/tris.md2", clipped, GIB_ORGANIC, NULL, false);
		for (n= 0; n < 3; n++)
			ThrowWidowGibLoc (self, "models/objects/gibs/sm_meat/tris.md2", clipped, GIB_ORGANIC, NULL, false);
		for (n= 0; n < 3; n++)
		{
			ThrowWidowGibSized (self, "models/monsters/blackwidow2/gib1/tris.md2", clipped, GIB_METALLIC, NULL,
				0, false);
			ThrowWidowGibSized (self, "models/monsters/blackwidow2/gib2/tris.md2", clipped, GIB_METALLIC, NULL, 
				gi.soundindex ("misc/fhit3.wav"), false);
		}
		for (n= 0; n < 2; n++)
		{
			ThrowWidowGibSized (self, "models/monsters/blackwidow2/gib3/tris.md2", clipped, GIB_METALLIC, NULL, 
				0, false);
			ThrowWidowGibSized (self, "models/monsters/blackwidow/gib3/tris.md2", clipped, GIB_METALLIC, NULL, 
				0, false);
		}
		ThrowGib (self, "models/objects/gibs/chest/tris.md2", clipped, GIB_ORGANIC);
		ThrowHead (self, "models/objects/gibs/head2/tris.md2", clipped, GIB_ORGANIC);
		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	gi.sound (self, CHAN_VOICE, sound_death, 1, ATTN_NONE, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_NO;
	self->count = 0;
	KillChildren (self);
	self->monsterinfo.quad_framenum = 0;
	self->monsterinfo.double_framenum = 0;
	self->monsterinfo.invincible_framenum = 0;
	self->monsterinfo.currentmove = &widow2_move_death;
}

qboolean Widow2_CheckAttack (edict_t *self)
{
	vec3_t		spot1, spot2;
	vec3_t		temp;
	float		chance;
	trace_t		tr;
	qboolean	enemy_infront;
	int			enemy_range;
	float		enemy_yaw;
	float		real_enemy_range;
	vec3_t		f, r, u;

	if (!self->enemy)
		return false;

	WidowPowerups(self);

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

	// melee attack
	if (self->timestamp < level.time)
	{
		real_enemy_range = realrange (self, self->enemy);
		if (real_enemy_range < 300)
		{
			AngleVectors (self->s.angles, f, r, u);
			G_ProjectSource2 (self->s.origin, offsets[0], f, r, u, spot1);
			VectorCopy (self->enemy->s.origin, spot2);
			if (widow2_tongue_attack_ok(spot1, spot2, 256))
			{
				// melee attack ok

				// be nice in easy mode
				if (skill->value == 0 && (rand()&3) )
					return false;

				if (self->monsterinfo.melee)
					self->monsterinfo.attack_state = AS_MELEE;
				else
					self->monsterinfo.attack_state = AS_MISSILE;
				return true;
			}
		}
	}
	
	if (level.time < self->monsterinfo.attack_finished)
		return false;
		
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		chance = 0.4;
	}
	else if (enemy_range == RANGE_NEAR)
	{
		chance = 0.8;
	}
	else if (enemy_range == RANGE_MID)
	{
		chance = 0.8;
	}
	else if (enemy_range == RANGE_FAR)
	{
		chance = 0.5;
	}

	// PGM - go ahead and shoot every time if it's a info_notnull
	if ((random () < chance) || (self->enemy->solid == SOLID_NOT))
	{
		self->monsterinfo.attack_state = AS_MISSILE;
//		self->monsterinfo.attack_finished = level.time + 1.0 + 2*random();
		return true;
	}

	return false;
}

void Widow2Precache ()
{
	// cache in all of the stalker stuff, widow stuff, spawngro stuff, gibs
	gi.soundindex ("parasite/parpain1.wav");	
	gi.soundindex ("parasite/parpain2.wav");	
	gi.soundindex ("parasite/pardeth1.wav");	
	gi.soundindex ("parasite/paratck1.wav");
	gi.soundindex ("parasite/parsght1.wav");
	gi.soundindex ("infantry/melee2.wav");
	gi.soundindex ("misc/fhit3.wav");

	gi.soundindex ("tank/tnkatck3.wav");
	gi.soundindex ("weapons/disrupt.wav");
	gi.soundindex ("weapons/disint2.wav");

	gi.modelindex ("models/monsters/stalker/tris.md2");
	gi.modelindex ("models/items/spawngro2/tris.md2");
	gi.modelindex ("models/objects/gibs/sm_metal/tris.md2");
	gi.modelindex ("models/proj/laser2/tris.md2");
	gi.modelindex ("models/proj/disintegrator/tris.md2");

	gi.modelindex ("models/monsters/blackwidow/gib1/tris.md2");
	gi.modelindex ("models/monsters/blackwidow/gib2/tris.md2");
	gi.modelindex ("models/monsters/blackwidow/gib3/tris.md2");
	gi.modelindex ("models/monsters/blackwidow/gib4/tris.md2");
	gi.modelindex ("models/monsters/blackwidow2/gib1/tris.md2");
	gi.modelindex ("models/monsters/blackwidow2/gib2/tris.md2");
	gi.modelindex ("models/monsters/blackwidow2/gib3/tris.md2");
	gi.modelindex ("models/monsters/blackwidow2/gib4/tris.md2");
}

/*QUAKED monster_widow2 (1 .5 0) (-70 -70 0) (70 70 144) Ambush Trigger_Spawn Sight
*/
void SP_monster_widow2 (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	sound_pain1 = gi.soundindex ("widow/bw2pain1.wav");
	sound_pain2 = gi.soundindex ("widow/bw2pain2.wav");
	sound_pain3 = gi.soundindex ("widow/bw2pain3.wav");
	sound_death = gi.soundindex ("widow/death.wav");
	sound_search1 = gi.soundindex ("bosshovr/bhvunqv1.wav");
//	sound_disrupt = gi.soundindex ("gladiator/railgun.wav");
	sound_tentacles_retract = gi.soundindex ("brain/brnatck3.wav");

//	self->s.sound = gi.soundindex ("bosshovr/bhvengn1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex ("models/monsters/blackwidow2/tris.md2");
	VectorSet (self->mins, -70, -70, 0);
	VectorSet (self->maxs, 70, 70, 144);

	self->health = 2000 + 800 + 1000*(skill->value);
	if (coop->value)
		self->health += 500*(skill->value);
//	self->health = 1;
	self->gib_health = -900;
	self->mass = 2500;

/*	if (skill->value == 2)
	{
		self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
		self->monsterinfo.power_armor_power = 500;
	}
	else */if (skill->value == 3)
	{
		self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
		self->monsterinfo.power_armor_power = 750;
	}

	self->yaw_speed = 30;
	
	self->flags |= FL_IMMUNE_LASER;
	self->monsterinfo.aiflags |= AI_IGNORE_SHOTS;

	self->pain = widow2_pain;
	self->die = widow2_die;

	self->monsterinfo.melee = widow2_melee;
	self->monsterinfo.stand = widow2_stand;
	self->monsterinfo.walk = widow2_walk;
	self->monsterinfo.run = widow2_run;
	self->monsterinfo.attack = widow2_attack;
	self->monsterinfo.search = widow2_search;
	self->monsterinfo.checkattack = Widow2_CheckAttack;
	gi.linkentity (self);

	self->monsterinfo.currentmove = &widow2_move_stand;	
	self->monsterinfo.scale = MODEL_SCALE;

	Widow2Precache();
	WidowCalcSlots(self);
	walkmonster_start (self);
}

//
// Death sequence stuff
//

void WidowVelocityForDamage (int damage, vec3_t v)
{
	v[0] = damage * crandom();
	v[1] = damage * crandom();
	v[2] = damage * crandom() + 200.0;
}

void widow_gib_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{

	self->solid = SOLID_NOT;
	self->touch = NULL;
	self->s.angles[PITCH] = 0;
	self->s.angles[ROLL] = 0;
	VectorClear (self->avelocity);

	if (self->plat2flags)
		gi.sound (self, CHAN_VOICE, self->plat2flags, 1, ATTN_NORM, 0);
/*
	if (plane)
	{
		if (plane->normal[2] < -0.8)
		{
			gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/fhit3.wav"), 1, ATTN_NORM, 0);
		}
		
		//vectoangles (plane->normal, normal_angles);
		//AngleVectors (normal_angles, NULL, right, NULL);
		//vectoangles (right, self->s.angles);
		//VectorClear (self->avelocity);
	}
*/
}

void ThrowWidowGib (edict_t *self, char *gibname, int damage, int type)
{
	ThrowWidowGibReal (self, gibname, damage, type, NULL, false, 0, true);
}

void ThrowWidowGibLoc (edict_t *self, char *gibname, int damage, int type, vec3_t startpos, qboolean fade)
{
	ThrowWidowGibReal (self, gibname, damage, type, startpos, false, 0, fade);
}

void ThrowWidowGibSized (edict_t *self, char *gibname, int damage, int type, vec3_t startpos, int hitsound, qboolean fade)
{
	ThrowWidowGibReal (self, gibname, damage, type, startpos, true, hitsound, fade);
}

void ThrowWidowGibReal (edict_t *self, char *gibname, int damage, int type, vec3_t startpos, qboolean sized, int hitsound, qboolean fade)
{
	edict_t *gib;
	vec3_t	vd;
	vec3_t	origin;
	vec3_t	size;
	float	vscale;

	if (!gibname)
		return;

	gib = G_Spawn();

	if (startpos)
		VectorCopy (startpos, gib->s.origin);
	else
	{
		VectorScale (self->size, 0.5, size);
		VectorAdd (self->absmin, size, origin);
		gib->s.origin[0] = origin[0] + crandom() * size[0];
		gib->s.origin[1] = origin[1] + crandom() * size[1];
		gib->s.origin[2] = origin[2] + crandom() * size[2];
	}

	gib->solid = SOLID_NOT;
	gib->s.effects |= EF_GIB;
	gib->flags |= FL_NO_KNOCKBACK;
	gib->takedamage = DAMAGE_YES;
	gib->die = gib_die;
	gib->s.renderfx |= RF_IR_VISIBLE;

	if (fade)
	{
		gib->think = G_FreeEdict;
		// sized gibs last longer
		if (sized)
			gib->nextthink = level.time + 20 + random()*15;
		else
			gib->nextthink = level.time + 5 + random()*10;
	}
	else
	{
		gib->think = G_FreeEdict;
		// sized gibs last longer
		if (sized)
			gib->nextthink = level.time + 60 + random()*15;
		else
			gib->nextthink = level.time + 25 + random()*10;
	}

	if (type == GIB_ORGANIC)
	{
		gib->movetype = MOVETYPE_TOSS;
		gib->touch = gib_touch;
		vscale = 0.5;
	}
	else
	{
		gib->movetype = MOVETYPE_BOUNCE;
		vscale = 1.0;
	}

	WidowVelocityForDamage (damage, vd);
	VectorMA (self->velocity, vscale, vd, gib->velocity);
	ClipGibVelocity (gib);

	gi.setmodel (gib, gibname);

	if (sized)
	{
		gib->plat2flags = hitsound;
		gib->solid = SOLID_BBOX;
		gib->avelocity[0] = random()*400;
		gib->avelocity[1] = random()*400;
		gib->avelocity[2] = random()*200;
		if (gib->velocity[2] < 0)
			gib->velocity[2] *= -1;
		gib->velocity[0] *= 2;
		gib->velocity[1] *= 2;
		ClipGibVelocity (gib);
		gib->velocity[2] = max((350 + (random()*100.0)), gib->velocity[2]);
		gib->gravity = 0.25;
		gib->touch = widow_gib_touch;
		gib->owner = self;
		if (gib->s.modelindex == gi.modelindex ("models/monsters/blackwidow2/gib2/tris.md2"))
		{
			VectorSet (gib->mins, -10, -10, 0);
			VectorSet (gib->maxs, 10, 10, 10);
		}
		else
		{
			VectorSet (gib->mins, -5, -5, 0);
			VectorSet (gib->maxs, 5, 5, 5);
		}
	}
	else
	{
		gib->velocity[0] *= 2;
		gib->velocity[1] *= 2;
		gib->avelocity[0] = random()*600;
		gib->avelocity[1] = random()*600;
		gib->avelocity[2] = random()*600;
	}

//	gib->think = G_FreeEdict;
//	gib->nextthink = level.time + 10 + random()*10;

	gi.linkentity (gib);
}

void BloodFountain (edict_t *self, int number, vec3_t startpos, int damage)
{
	int n;
	vec3_t	vd;
	vec3_t	origin, size, velocity;

	return;

	for (n= 0; n < number; n++)
	{
		if (startpos)
			VectorCopy (startpos, origin);
		else
		{
			VectorScale (self->size, 0.5, size);
			VectorAdd (self->absmin, size, origin);
			origin[0] = origin[0] + crandom() * size[0];
			origin[1] = origin[1] + crandom() * size[1];
			origin[2] = origin[2] + crandom() * size[2];
		}

		WidowVelocityForDamage (damage, vd);
		VectorMA (self->velocity, 1.0, vd, velocity);
		velocity[0] *= 2;
		velocity[1] *= 2;

//		gi.WriteByte (svc_temp_entity);
//		gi.WriteByte (TE_BLOOD_FOUNTAIN);
//		gi.WritePosition (origin);
//		gi.WritePosition (velocity);
//		gi.WriteShort (50);
//		gi.multicast (self->s.origin, MULTICAST_ALL);
	}
}

void ThrowSmallStuff (edict_t *self, vec3_t point)
{
	int n;

	for (n= 0; n < 2; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_ORGANIC, point, false);
	ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, point, false);
	ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, point, false);

}

void ThrowMoreStuff (edict_t *self, vec3_t point)
{
	int n;

	if (coop && coop->value)
	{
		ThrowSmallStuff (self, point);
		return;
	}

	for (n= 0; n < 1; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_ORGANIC, point, false);
	for (n= 0; n < 2; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, point, false);
	for (n= 0; n < 3; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, point, false);

}

void WidowExplode (edict_t *self)
{
	vec3_t	org;
	int		n;

	self->think = WidowExplode;
//	gi.dprintf ("count = %d\n");

//redo:
	VectorCopy (self->s.origin, org);
	org[2] += 24 + (rand()&15);
	if (self->count < 8)
		org[2] += 24 + (rand()&31);
	switch (self->count)
	{
	case 0:
		org[0] -= 24;
		org[1] -= 24;
		break;
	case 1:
		org[0] += 24;
		org[1] += 24;
		ThrowSmallStuff(self, org);
		break;
	case 2:
		org[0] += 24;
		org[1] -= 24;
		break;
	case 3:
		org[0] -= 24;
		org[1] += 24;
		ThrowMoreStuff(self, org);
		break;
	case 4:
		org[0] -= 48;
		org[1] -= 48;
		break;
	case 5:
		org[0] += 48;
		org[1] += 48;
		ThrowArm1 (self);
		break;
	case 6:
		org[0] -= 48;
		org[1] += 48;
		ThrowArm2 (self);
		break;
	case 7:
		org[0] += 48;
		org[1] -= 48;
		ThrowSmallStuff(self, org);
		break;
	case 8:
		org[0] += 18;
		org[1] += 18;
		org[2] = self->s.origin[2] + 48;
		ThrowMoreStuff(self, org);
		break;
	case 9:
		org[0] -= 18;
		org[1] += 18;
		org[2] = self->s.origin[2] + 48;
		break;
	case 10:
		org[0] += 18;
		org[1] -= 18;
		org[2] = self->s.origin[2] + 48;
		break;
	case 11:
		org[0] -= 18;
		org[1] -= 18;
		org[2] = self->s.origin[2] + 48;
		break;
	case 12:
		self->s.sound = 0;
		for (n= 0; n < 1; n++)
			ThrowWidowGib (self, "models/objects/gibs/sm_meat/tris.md2", 400, GIB_ORGANIC);
		for (n= 0; n < 2; n++)
			ThrowWidowGib (self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC);
		for (n= 0; n < 2; n++)
			ThrowWidowGib (self, "models/objects/gibs/sm_metal/tris.md2", 400, GIB_METALLIC);
//		ThrowGib (self, "models/objects/gibs/chest/tris.md2", 1000, GIB_ORGANIC);
//		ThrowHead (self, "models/objects/gibs/gear/tris.md2", 1000, GIB_METALLIC);
		self->deadflag = DEAD_DEAD;
		self->think = monster_think;
		self->nextthink = level.time + 0.1;
		self->monsterinfo.currentmove = &widow2_move_dead;
		return;
	}

	self->count++;
	if (self->count >=9 && self->count <=12)
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_EXPLOSION1_BIG);
		gi.WritePosition (org);
		gi.multicast (self->s.origin, MULTICAST_ALL);
//		goto redo;
	} 
	else
	{
		// else
		gi.WriteByte (svc_temp_entity);
		if (self->count %2)
			gi.WriteByte (TE_EXPLOSION1);
		else
			gi.WriteByte (TE_EXPLOSION1_NP);
		gi.WritePosition (org);
		gi.multicast (self->s.origin, MULTICAST_ALL);
	}

	self->nextthink = level.time + 0.1;
}

void WidowExplosion1 (edict_t *self)
{
	int		n;
	vec3_t	f,r,u, startpoint;
	vec3_t	offset = {23.74, -37.67, 76.96};

//	gi.dprintf ("1\n");
	AngleVectors (self->s.angles, f, r, u);
	G_ProjectSource2 (self->s.origin, offset, f, r, u, startpoint);

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1);
	gi.WritePosition (startpoint);
	gi.multicast (self->s.origin, MULTICAST_ALL);
	
	for (n= 0; n < 1; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_ORGANIC, startpoint, false);
	for (n= 0; n < 1; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, startpoint, false);
	for (n= 0; n < 2; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, startpoint, false);
}

void WidowExplosion2 (edict_t *self)
{
	int		n;
	vec3_t	f,r,u, startpoint;
	vec3_t	offset = {-20.49, 36.92, 73.52};

//	gi.dprintf ("2\n");

	AngleVectors (self->s.angles, f, r, u);
	G_ProjectSource2 (self->s.origin, offset, f, r, u, startpoint);

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1);
	gi.WritePosition (startpoint);
	gi.multicast (self->s.origin, MULTICAST_ALL);
	
	for (n= 0; n < 1; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_ORGANIC, startpoint, false);
	for (n= 0; n < 1; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, startpoint, false);
	for (n= 0; n < 2; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, startpoint, false);
}

void WidowExplosion3 (edict_t *self)
{
	int		n;
	vec3_t	f,r,u, startpoint;
	vec3_t	offset = {2.11, 0.05, 92.20};

//	gi.dprintf ("3\n");

	AngleVectors (self->s.angles, f, r, u);
	G_ProjectSource2 (self->s.origin, offset, f, r, u, startpoint);

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1);
	gi.WritePosition (startpoint);
	gi.multicast (self->s.origin, MULTICAST_ALL);
	
	for (n= 0; n < 1; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_ORGANIC, startpoint, false);
	for (n= 0; n < 1; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, startpoint, false);
	for (n= 0; n < 2; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, startpoint, false);
}

void WidowExplosion4 (edict_t *self)
{
	int		n;
	vec3_t	f,r,u, startpoint;
	vec3_t	offset = {-28.04, -35.57, -77.56};

//	gi.dprintf ("4\n");

	AngleVectors (self->s.angles, f, r, u);
	G_ProjectSource2 (self->s.origin, offset, f, r, u, startpoint);

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1);
	gi.WritePosition (startpoint);
	gi.multicast (self->s.origin, MULTICAST_ALL);
	
	for (n= 0; n < 1; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_ORGANIC, startpoint, false);
	for (n= 0; n < 1; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, startpoint, false);
	for (n= 0; n < 2; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, startpoint, false);
}

void WidowExplosion5 (edict_t *self)
{
	int		n;
	vec3_t	f,r,u, startpoint;
	vec3_t	offset = {-20.11, -1.11, 40.76};

//	gi.dprintf ("5\n");

	AngleVectors (self->s.angles, f, r, u);
	G_ProjectSource2 (self->s.origin, offset, f, r, u, startpoint);

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1);
	gi.WritePosition (startpoint);
	gi.multicast (self->s.origin, MULTICAST_ALL);
	
	for (n= 0; n < 1; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_ORGANIC, startpoint, false);
	for (n= 0; n < 1; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, startpoint, false);
	for (n= 0; n < 2; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, startpoint, false);
}

void WidowExplosion6 (edict_t *self)
{
	int		n;
	vec3_t	f,r,u, startpoint;
	vec3_t	offset = {-20.11, -1.11, 40.76};

	//gi.dprintf ("6\n");

	AngleVectors (self->s.angles, f, r, u);
	G_ProjectSource2 (self->s.origin, offset, f, r, u, startpoint);

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1);
	gi.WritePosition (startpoint);
	gi.multicast (self->s.origin, MULTICAST_ALL);
	
	for (n= 0; n < 1; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_ORGANIC, startpoint, false);
	for (n= 0; n < 1; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, startpoint, false);
	for (n= 0; n < 2; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, startpoint, false);
}

void WidowExplosion7 (edict_t *self)
{
	int		n;
	vec3_t	f,r,u, startpoint;
	vec3_t	offset = {-20.11, -1.11, 40.76};

	//gi.dprintf ("7\n");

	AngleVectors (self->s.angles, f, r, u);
	G_ProjectSource2 (self->s.origin, offset, f, r, u, startpoint);

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1);
	gi.WritePosition (startpoint);
	gi.multicast (self->s.origin, MULTICAST_ALL);
	
	for (n= 0; n < 1; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_ORGANIC, startpoint, false);
	for (n= 0; n < 1; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, startpoint, false);
	for (n= 0; n < 2; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, startpoint, false);
}

void WidowExplosionLeg (edict_t *self)
{
//	int		n;
	vec3_t	f,r,u, startpoint;
	vec3_t	offset1 = {-31.89, -47.86, 67.02};
	vec3_t	offset2 = {-44.9, -82.14, 54.72};

	//gi.dprintf ("Leg\n");

	AngleVectors (self->s.angles, f, r, u);
	G_ProjectSource2 (self->s.origin, offset1, f, r, u, startpoint);

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1_BIG);
	gi.WritePosition (startpoint);
	gi.multicast (self->s.origin, MULTICAST_ALL);

	ThrowWidowGibSized (self, "models/monsters/blackwidow2/gib2/tris.md2", 200, GIB_METALLIC, startpoint, 
		gi.soundindex ("misc/fhit3.wav"), false);
	ThrowWidowGibLoc (self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_ORGANIC, startpoint, false);
	ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, startpoint, false);

	G_ProjectSource2 (self->s.origin, offset2, f, r, u, startpoint);

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1);
	gi.WritePosition (startpoint);
	gi.multicast (self->s.origin, MULTICAST_ALL);

	ThrowWidowGibSized (self, "models/monsters/blackwidow2/gib1/tris.md2", 300, GIB_METALLIC, startpoint,
		gi.soundindex ("misc/fhit3.wav"), false);
	ThrowWidowGibLoc (self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_ORGANIC, startpoint, false);
	ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, startpoint, false);
}

void ThrowArm1 (edict_t *self)
{
	int		n;
	vec3_t	f,r,u, startpoint;
	vec3_t	offset1 = {65.76, 17.52, 7.56};

	AngleVectors (self->s.angles, f, r, u);
	G_ProjectSource2 (self->s.origin, offset1, f, r, u, startpoint);

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1_BIG);
	gi.WritePosition (startpoint);
	gi.multicast (self->s.origin, MULTICAST_ALL);

	for (n= 0; n < 2; n++)
		ThrowWidowGibLoc (self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, startpoint, false);
}

void ThrowArm2 (edict_t *self)
{
//	int		n;
	vec3_t	f,r,u, startpoint;
	vec3_t	offset1 = {65.76, 17.52, 7.56};

	AngleVectors (self->s.angles, f, r, u);
	G_ProjectSource2 (self->s.origin, offset1, f, r, u, startpoint);

	ThrowWidowGibSized (self, "models/monsters/blackwidow2/gib4/tris.md2", 200, GIB_METALLIC, startpoint, 
		gi.soundindex ("misc/fhit3.wav"), false);
	ThrowWidowGibLoc (self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_ORGANIC, startpoint, false);
}
