// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

carrier

==============================================================================
*/

// self->timestamp used for frame calculations in grenade & spawn code
// self->wait used to prevent rapid refire of rocket launcher

#include "g_local.h"
#include "m_carrier.h"

#define	CARRIER_ROCKET_TIME		2		// number of seconds between rocket shots
#define CARRIER_ROCKET_SPEED	750
#define	NUM_FLYERS_SPAWNED		6		// max # of flyers he can spawn

#define	RAIL_FIRE_TIME			3

void BossExplode (edict_t *self);
void Grenade_Explode (edict_t *ent);

qboolean infront (edict_t *self, edict_t *other);
qboolean inback (edict_t *self, edict_t *other);
qboolean below (edict_t *self, edict_t *other);
void drawbbox (edict_t *self);

//char *ED_NewString (char *string);
void ED_CallSpawn (edict_t *ent);

static int	sound_pain1;
static int	sound_pain2;
static int	sound_pain3;
static int	sound_death;
//static int	sound_search1;
static int	sound_sight;
static int	sound_rail;
static int	sound_spawn;

float	orig_yaw_speed;

vec3_t flyer_mins = {-16, -16, -24};
vec3_t flyer_maxs = {16, 16, 16};

extern mmove_t flyer_move_attack2, flyer_move_attack3, flyer_move_kamikaze;


void carrier_run (edict_t *self);
void carrier_stand (edict_t *self);
void carrier_dead (edict_t *self);
void carrier_attack (edict_t *self);
void carrier_attack_mg (edict_t *self);
void carrier_reattack_mg (edict_t *self);
void carrier_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

void carrier_attack_gren (edict_t *self);
void carrier_reattack_gren (edict_t *self);

void carrier_start_spawn (edict_t *self);
void carrier_spawn_check (edict_t *self);
void carrier_prep_spawn (edict_t *self);

void CarrierMachineGunHold (edict_t *self);
void CarrierRocket (edict_t *self);


void carrier_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

// code starts here
//void carrier_search (edict_t *self)
//{
//	if (random() < 0.5)
//		gi.sound (self, CHAN_VOICE, sound_search1, 1, ATTN_NONE, 0);
//}

//
// this is the smarts for the rocket launcher in coop
//
// if there is a player behind/below the carrier, and we can shoot, and we can trace a LOS to them ..
// pick one of the group, and let it rip
void CarrierCoopCheck (edict_t *self)
{
	// no more than 4 players in coop, so..
	edict_t *targets[4];
	int		num_targets = 0, target, player;
	edict_t *ent;
	trace_t	tr;

	// if we're not in coop, this is a noop
	if (!coop || !coop->value)
		return;
	// if we are, and we have recently fired, bail
	if (self->wait > level.time)
		return;

	memset (targets, 0, 4*sizeof(edict_t *));

	// cycle through players
	for (player = 1; player <= game.maxclients; player++)
	{
		ent = &g_edicts[player];
		if (!ent->inuse)
			continue;
		if (!ent->client)
			continue;
		if (inback(self, ent) || below(self, ent))
		{
			tr = gi.trace (self->s.origin, NULL, NULL, ent->s.origin, self, MASK_SOLID);
			if (tr.fraction == 1.0)
			{
//				if ((g_showlogic) && (g_showlogic->value))
//					gi.dprintf ("Carrier: found a player who I can shoot\n");
				targets[num_targets++] = ent;
			}
		}
	}

	if (!num_targets)
		return;

	// get a number from 0 to (num_targets-1)
	target = random() * num_targets;
	
	// just in case we got a 1.0 from random
	if (target == num_targets)
		target--;

	// make sure to prevent rapid fire rockets
	self->wait = level.time + CARRIER_ROCKET_TIME;

	// save off the real enemy
	ent = self->enemy;
	// set the new guy as temporary enemy
	self->enemy = targets[target];
	CarrierRocket (self);
	// put the real enemy back
	self->enemy = ent;

	// we're done
	return;
}

void CarrierGrenade (edict_t *self)
{
	vec3_t	start;
	vec3_t	forward, right, up;
	vec3_t	aim;
	int		flash_number;
	float	direction;		// from lower left to upper right, or lower right to upper left
	float	spreadR, spreadU;
	int		mytime;

	CarrierCoopCheck(self);

	if (!self->enemy)
		return;

	if (random() < 0.5)
		direction = -1.0;
	else
		direction = 1.0;

	mytime = (int)((level.time - self->timestamp)/0.4);

	if (mytime == 0)
	{
		spreadR = 0.15 * direction;
//		spreadU = 0.1 * direction;
		spreadU = 0.1 - 0.1 * direction;
	}
	else if (mytime == 1)
	{
		spreadR = 0;
//		spreadU = 0;
		spreadU = 0.1;
	}
	else if (mytime == 2)
	{
		spreadR = -0.15 * direction;
//		spreadU = -0.1 * direction;
		spreadU = 0.1 - -0.1 * direction;
	}
	else if (mytime == 3)
	{
		spreadR = 0;
//		spreadU = 0;
		spreadU = 0.1;
	}
	else
	{
		// error, shoot straight
//		if ((g_showlogic) && (g_showlogic->value))
//			gi.dprintf ("CarrierGrenade: bad time  %2.2f   %2.2f\n", level.time, self->timestamp);
		spreadR = 0;
		spreadU = 0;
	}

	AngleVectors (self->s.angles, forward, right, up);
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_CARRIER_GRENADE], forward, right, start);

	VectorSubtract (self->enemy->s.origin, start, aim);
	VectorNormalize (aim);

	VectorMA (aim, spreadR, right, aim);
	VectorMA (aim, spreadU, up, aim);

	if(aim[2] > 0.15)
		aim[2] = 0.15;
	else if(aim[2] < -0.5)
		aim[2] = -0.5;

	flash_number = MZ2_GUNNER_GRENADE_1;
	monster_fire_grenade (self, start, aim, 50, 600, flash_number);
}

void CarrierPredictiveRocket  (edict_t *self)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;

//	if ((g_showlogic) && (g_showlogic->value))
//		gi.dprintf("predictive fire\n");

	AngleVectors (self->s.angles, forward, right, NULL);

//1
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_CARRIER_ROCKET_1], forward, right, start);
	PredictAim (self->enemy, start, CARRIER_ROCKET_SPEED, false, -0.3, dir, NULL);
	monster_fire_rocket (self, start, dir, 50, CARRIER_ROCKET_SPEED, MZ2_CARRIER_ROCKET_1);

//2
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_CARRIER_ROCKET_2], forward, right, start);
	PredictAim (self->enemy, start, CARRIER_ROCKET_SPEED, false, -0.15, dir, NULL);
	monster_fire_rocket (self, start, dir, 50, CARRIER_ROCKET_SPEED, MZ2_CARRIER_ROCKET_2);

//3
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_CARRIER_ROCKET_3], forward, right, start);
	PredictAim (self->enemy, start, CARRIER_ROCKET_SPEED, false, 0, dir, NULL);
	monster_fire_rocket (self, start, dir, 50, CARRIER_ROCKET_SPEED, MZ2_CARRIER_ROCKET_3);

//4
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_CARRIER_ROCKET_4], forward, right, start);
	PredictAim (self->enemy, start, CARRIER_ROCKET_SPEED, false, 0.15, dir, NULL);
	monster_fire_rocket (self, start, dir, 50, CARRIER_ROCKET_SPEED, MZ2_CARRIER_ROCKET_4);
}	

void CarrierRocket (edict_t *self)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;

	if(self->enemy)
	{
		if(self->enemy->client && random() < 0.5)
		{
			CarrierPredictiveRocket(self);
			return;
		}
	}
	else
		return;

	AngleVectors (self->s.angles, forward, right, NULL);

//1
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_CARRIER_ROCKET_1], forward, right, start);
	VectorCopy (self->enemy->s.origin, vec);
//	vec[2] += self->enemy->viewheight;
	vec[2] -= 15;
	VectorSubtract (vec, start, dir);
	VectorNormalize (dir);
	VectorMA (dir, 0.4, right, dir);
	VectorNormalize (dir);
	monster_fire_rocket (self, start, dir, 50, 500, MZ2_CARRIER_ROCKET_1);

//2
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_CARRIER_ROCKET_2], forward, right, start);
	VectorCopy (self->enemy->s.origin, vec);
//	vec[2] += self->enemy->viewheight;
	VectorSubtract (vec, start, dir);
	VectorNormalize (dir);
	VectorMA (dir, 0.025, right, dir);
	VectorNormalize (dir);
	monster_fire_rocket (self, start, dir, 50, 500, MZ2_CARRIER_ROCKET_2);

//3
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_CARRIER_ROCKET_3], forward, right, start);
	VectorCopy (self->enemy->s.origin, vec);
//	vec[2] += self->enemy->viewheight;
	VectorSubtract (vec, start, dir);
	VectorNormalize (dir);
	VectorMA (dir, -0.025, right, dir);
	VectorNormalize (dir);
	monster_fire_rocket (self, start, dir, 50, 500, MZ2_CARRIER_ROCKET_3);

//4
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_CARRIER_ROCKET_4], forward, right, start);
	VectorCopy (self->enemy->s.origin, vec);
//	vec[2] += self->enemy->viewheight;
	vec[2] -= 15;
	VectorSubtract (vec, start, dir);
	VectorNormalize (dir);
	VectorMA (dir, -0.4, right, dir);
	VectorNormalize (dir);
	monster_fire_rocket (self, start, dir, 50, 500, MZ2_CARRIER_ROCKET_4);

//5
//	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_BOSS2_ROCKET_4], forward, right, start);
//	VectorCopy (self->enemy->s.origin, vec);
//	vec[2] += self->enemy->viewheight;
//	VectorSubtract (vec, start, dir);
//	VectorNormalize (dir);
//	monster_fire_rocket (self, start, dir, 50, 500, MZ2_BOSS2_ROCKET_2);
}	

void carrier_firebullet_right (edict_t *self)
{
	vec3_t	forward, right, target;
	vec3_t	start;
	int		flashnum;

	// if we're in manual steering mode, it means we're leaning down .. use the lower shot
	if (self->monsterinfo.aiflags & AI_MANUAL_STEERING)
		flashnum = MZ2_CARRIER_MACHINEGUN_R2;
	else
		flashnum = MZ2_CARRIER_MACHINEGUN_R1;

	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource (self->s.origin, monster_flash_offset[flashnum], forward, right, start);

//	VectorMA (self->enemy->s.origin, -0.2, self->enemy->velocity, target);
	VectorMA (self->enemy->s.origin, 0.2, self->enemy->velocity, target);
	target[2] += self->enemy->viewheight;
/*
			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_DEBUGTRAIL);
			gi.WritePosition (start);
			gi.WritePosition (target);
			gi.multicast (start, MULTICAST_ALL);	
*/
	VectorSubtract (target, start, forward);
	VectorNormalize (forward);

	monster_fire_bullet (self, start, forward, 6, 4, DEFAULT_BULLET_HSPREAD*3, DEFAULT_BULLET_VSPREAD, flashnum);
}	

void carrier_firebullet_left (edict_t *self)
{
	vec3_t	forward, right, target;
	vec3_t	start;
	int		flashnum;

	// if we're in manual steering mode, it means we're leaning down .. use the lower shot
	if (self->monsterinfo.aiflags & AI_MANUAL_STEERING)
		flashnum = MZ2_CARRIER_MACHINEGUN_L2;
	else
		flashnum = MZ2_CARRIER_MACHINEGUN_L1;
	
	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource (self->s.origin, monster_flash_offset[flashnum], forward, right, start);

//	VectorMA (self->enemy->s.origin, 0.2, self->enemy->velocity, target);
	VectorMA (self->enemy->s.origin, -0.2, self->enemy->velocity, target);

	target[2] += self->enemy->viewheight;
	VectorSubtract (target, start, forward);
/*
			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_DEBUGTRAIL);
			gi.WritePosition (start);
			gi.WritePosition (target);
			gi.multicast (start, MULTICAST_ALL);	
*/
	VectorNormalize (forward);

	monster_fire_bullet (self, start, forward, 6, 4, DEFAULT_BULLET_HSPREAD*3, DEFAULT_BULLET_VSPREAD, flashnum);
}	

void CarrierMachineGun (edict_t *self)
{
	CarrierCoopCheck(self);
	if (self->enemy)
		carrier_firebullet_left(self);
	if (self->enemy)
		carrier_firebullet_right(self);
}	

void CarrierSpawn (edict_t *self)
{
	vec3_t	f, r, offset, startpoint, spawnpoint;
	edict_t	*ent;
	int		mytime;

//	VectorSet (offset, 105, 0, -30); // real distance needed is (sqrt (56*56*2) + sqrt(16*16*2)) or 101.8
	VectorSet (offset, 105, 0, -58); // real distance needed is (sqrt (56*56*2) + sqrt(16*16*2)) or 101.8
	AngleVectors (self->s.angles, f, r, NULL);

	G_ProjectSource (self->s.origin, offset, f, r, startpoint);

	// the +0.1 is because level.time is sometimes a little low
	mytime = (int)((level.time + 0.1 - self->timestamp)/0.5);
//	if ((g_showlogic) && (g_showlogic->value))
//		gi.dprintf ("mytime = %d, (%2.2f)\n", mytime, level.time - self->timestamp);

	if (FindSpawnPoint (startpoint, flyer_mins, flyer_maxs, spawnpoint, 32))
	{
		// the second flier should be a kamikaze flyer
		if (mytime != 2)
			ent = CreateMonster (spawnpoint, self->s.angles, "monster_flyer");
		else
			ent = CreateMonster (spawnpoint, self->s.angles, "monster_kamikaze");

		if (!ent)
			return;

		gi.sound (self, CHAN_BODY, sound_spawn, 1, ATTN_NONE, 0);

		self->monsterinfo.monster_slots--;
//		if ((g_showlogic) && (g_showlogic->value))
//			gi.dprintf ("carrier: post-spawn : %d slots left\n", self->monsterinfo.monster_slots);

		ent->nextthink = level.time;
		ent->think (ent);
		
		ent->monsterinfo.aiflags |= AI_SPAWNED_CARRIER|AI_DO_NOT_COUNT|AI_IGNORE_SHOTS;
		ent->monsterinfo.commander = self;

		if ((self->enemy->inuse) && (self->enemy->health > 0))
		{
			ent->enemy = self->enemy;
			FoundTarget (ent);
			if (mytime == 1)
			{
				ent->monsterinfo.lefty = 0;
				ent->monsterinfo.attack_state = AS_SLIDING;
				ent->monsterinfo.currentmove = &flyer_move_attack3;
			}
			else if (mytime == 2)
			{
				ent->monsterinfo.lefty = 0;
				ent->monsterinfo.attack_state = AS_STRAIGHT;
				ent->monsterinfo.currentmove = &flyer_move_kamikaze;
				ent->mass = 100;
				ent->monsterinfo.aiflags |= AI_CHARGING;
			}
			else if (mytime == 3)
			{
				ent->monsterinfo.lefty = 1;
				ent->monsterinfo.attack_state = AS_SLIDING;
				ent->monsterinfo.currentmove = &flyer_move_attack3;
			}
//			else if ((g_showlogic) && (g_showlogic->value))
//				gi.dprintf ("carrier:  unexpected time %d!\n", mytime);
		}
	}
}

void carrier_prep_spawn (edict_t *self)
{
	CarrierCoopCheck(self);
	self->monsterinfo.aiflags |= AI_MANUAL_STEERING;
	self->timestamp = level.time;
	self->yaw_speed = 10;
	CarrierMachineGun(self);
}

void carrier_spawn_check (edict_t *self)
{
//	gi.dprintf ("times - %2.2f %2.2f\n", level.time, self->timestamp);
	CarrierCoopCheck(self);
	CarrierMachineGun(self);
	CarrierSpawn (self);

	if (level.time > (self->timestamp + 1.1))  // 0.5 seconds per flyer.  this gets three
	{
		self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
		self->yaw_speed = orig_yaw_speed;
		return;
	}
	else
		self->monsterinfo.nextframe = FRAME_spawn08;
}

void carrier_ready_spawn (edict_t *self)
{
	float	current_yaw;
	vec3_t	offset, f, r, startpoint, spawnpoint;

	CarrierCoopCheck(self);
	CarrierMachineGun(self);

	current_yaw = anglemod(self->s.angles[YAW]);

//	gi.dprintf ("yaws = %2.2f %2.2f\n", current_yaw, self->ideal_yaw);

	if (fabs(current_yaw - self->ideal_yaw) > 0.1)
	{
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
		self->timestamp += FRAMETIME;
		return;
	}

	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;

	VectorSet (offset, 105,0,-58);
	AngleVectors (self->s.angles, f, r, NULL);
	G_ProjectSource (self->s.origin, offset, f, r, startpoint);
	if (FindSpawnPoint (startpoint, flyer_mins, flyer_maxs, spawnpoint, 32))
	{
		SpawnGrow_Spawn (spawnpoint, 0);
	}
}

void carrier_start_spawn (edict_t *self)
{
	int		mytime;
	float	enemy_yaw;
	vec3_t	temp;
//	vec3_t	offset, f, r, startpoint;

	CarrierCoopCheck(self);
	if (!orig_yaw_speed)
		orig_yaw_speed = self->yaw_speed;

	if (!self->enemy)
		return;

	mytime = (int)((level.time - self->timestamp)/0.5);

	VectorSubtract (self->enemy->s.origin, self->s.origin, temp);
	enemy_yaw = vectoyaw2(temp);

	// note that the offsets are based on a forward of 105 from the end angle
	if (mytime == 0)
	{
		self->ideal_yaw = anglemod(enemy_yaw - 30);
//		VectorSet (offset, 90.9, 52.5, 0);
	}
	else if (mytime == 1)
	{
		self->ideal_yaw = anglemod(enemy_yaw);
//		VectorSet (offset, 90.9, -52.5, 0);
	}
	else if (mytime == 2)
	{
		self->ideal_yaw = anglemod(enemy_yaw + 30);
//		VectorSet (offset, 90.9, -52.5, 0);
	}
//	else if ((g_showlogic) && (g_showlogic->value))
//		gi.dprintf ("carrier: bad spawntime\n");

	CarrierMachineGun (self);
}

mframe_t carrier_frames_stand [] =
{
//	ai_stand, 0, drawbbox,
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
mmove_t	carrier_move_stand = {FRAME_search01, FRAME_search13, carrier_frames_stand, NULL};

mframe_t carrier_frames_walk [] =
{
//	ai_walk,	12,	drawbbox,
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
mmove_t carrier_move_walk = {FRAME_search01, FRAME_search13, carrier_frames_walk, NULL};


mframe_t carrier_frames_run [] =
{
//	ai_run,	12,	drawbbox,
	ai_run,	6,	CarrierCoopCheck,
	ai_run,	6,	CarrierCoopCheck,
	ai_run,	6,	CarrierCoopCheck,
	ai_run,	6,	CarrierCoopCheck,
	ai_run,	6,	CarrierCoopCheck,
	ai_run,	6,	CarrierCoopCheck,
	ai_run,	6,	CarrierCoopCheck,
	ai_run,	6,	CarrierCoopCheck,
	ai_run,	6,	CarrierCoopCheck,
	ai_run,	6,	CarrierCoopCheck,
	ai_run,	6,	CarrierCoopCheck,
	ai_run,	6,	CarrierCoopCheck,
	ai_run,	6,	CarrierCoopCheck
};
mmove_t carrier_move_run = {FRAME_search01, FRAME_search13, carrier_frames_run, NULL};

mframe_t carrier_frames_attack_pre_mg [] =
{
	ai_charge,	4,	CarrierCoopCheck,
	ai_charge,	4,	CarrierCoopCheck,
	ai_charge,	4,	CarrierCoopCheck,
	ai_charge,	4,	CarrierCoopCheck,
	ai_charge,	4,	CarrierCoopCheck,
	ai_charge,	4,	CarrierCoopCheck,
	ai_charge,	4,	CarrierCoopCheck,
	ai_charge,	4,	carrier_attack_mg
};
mmove_t carrier_move_attack_pre_mg = {FRAME_firea01, FRAME_firea08, carrier_frames_attack_pre_mg, NULL};


// Loop this
mframe_t carrier_frames_attack_mg [] =
{
	ai_charge,	-2,	CarrierMachineGun,
	ai_charge,	-2,	CarrierMachineGun,
	ai_charge,	-2,	carrier_reattack_mg
/*
	ai_charge,	0,	CarrierMachineGunHold,
//	ai_charge,	0,	CarrierMachineGun,
	ai_charge,	0,	CarrierMachineGun,
	ai_charge,	0,	carrier_reattack_mg
*/
};
mmove_t carrier_move_attack_mg = {FRAME_firea09, FRAME_firea11, carrier_frames_attack_mg, NULL};

mframe_t carrier_frames_attack_post_mg [] =
{
	ai_charge,	4,	CarrierCoopCheck,
	ai_charge,	4,	CarrierCoopCheck,
	ai_charge,	4,	CarrierCoopCheck,
	ai_charge,	4,	CarrierCoopCheck
};
mmove_t carrier_move_attack_post_mg = {FRAME_firea12, FRAME_firea15, carrier_frames_attack_post_mg, carrier_run};

mframe_t carrier_frames_attack_pre_gren [] =
{
	ai_charge, 4, CarrierCoopCheck,
	ai_charge, 4, CarrierCoopCheck,
	ai_charge, 4, CarrierCoopCheck,
	ai_charge, 4, CarrierCoopCheck,
	ai_charge, 4, CarrierCoopCheck,
	ai_charge, 4, carrier_attack_gren
};
mmove_t carrier_move_attack_pre_gren = {FRAME_fireb01, FRAME_fireb06, carrier_frames_attack_pre_gren, NULL};

mframe_t carrier_frames_attack_gren [] =
{
	ai_charge, -15, CarrierGrenade,
	ai_charge, 4, CarrierCoopCheck,
	ai_charge, 4, CarrierCoopCheck,
	ai_charge, 4, carrier_reattack_gren
};
mmove_t carrier_move_attack_gren = {FRAME_fireb07, FRAME_fireb10, carrier_frames_attack_gren, NULL};

mframe_t carrier_frames_attack_post_gren [] =
{
	ai_charge, 4, CarrierCoopCheck,
	ai_charge, 4, CarrierCoopCheck,
	ai_charge, 4, CarrierCoopCheck,
	ai_charge, 4, CarrierCoopCheck,
	ai_charge, 4, CarrierCoopCheck,
	ai_charge, 4, CarrierCoopCheck
};
mmove_t carrier_move_attack_post_gren = {FRAME_fireb11, FRAME_fireb16, carrier_frames_attack_post_gren, carrier_run};

mframe_t carrier_frames_attack_rocket [] =
{
	ai_charge,	15,	CarrierRocket
};
mmove_t carrier_move_attack_rocket = {FRAME_fireb01, FRAME_fireb01, carrier_frames_attack_rocket, carrier_run};

void CarrierRail (edict_t *self)
{
	vec3_t	start;
	vec3_t	dir;
	vec3_t	forward, right;

	CarrierCoopCheck(self);
	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource (self->s.origin, monster_flash_offset[MZ2_CARRIER_RAILGUN], forward, right, start);

	// calc direction to where we targeted
	VectorSubtract (self->pos1, start, dir);
	VectorNormalize (dir);

	monster_fire_railgun (self, start, dir, 50, 100, MZ2_CARRIER_RAILGUN);
	self->monsterinfo.attack_finished = level.time + RAIL_FIRE_TIME;
}

void CarrierSaveLoc (edict_t *self)
{
	CarrierCoopCheck(self);
	VectorCopy (self->enemy->s.origin, self->pos1);	//save for aiming the shot
	self->pos1[2] += self->enemy->viewheight;
};

mframe_t carrier_frames_attack_rail [] =
{
	ai_charge, 2, CarrierCoopCheck,
	ai_charge, 2, CarrierSaveLoc,
	ai_charge, 2, CarrierCoopCheck,
	ai_charge, -20, CarrierRail,
	ai_charge, 2, CarrierCoopCheck,
	ai_charge, 2, CarrierCoopCheck,
	ai_charge, 2, CarrierCoopCheck,
	ai_charge, 2, CarrierCoopCheck,
	ai_charge, 2, CarrierCoopCheck
};
mmove_t carrier_move_attack_rail = {FRAME_search01, FRAME_search09, carrier_frames_attack_rail, carrier_run};

mframe_t carrier_frames_spawn [] =
{
	ai_charge,	-2,	CarrierMachineGun,
	ai_charge,	-2,	CarrierMachineGun,
	ai_charge,	-2,	CarrierMachineGun,
	ai_charge,	-2,	CarrierMachineGun,
	ai_charge,	-2,	CarrierMachineGun,
	ai_charge,	-2,	CarrierMachineGun,
	ai_charge,	-2,	carrier_prep_spawn,		// 7 - end of wind down
	ai_charge,	-2,	carrier_start_spawn,		// 8 - start of spawn
	ai_charge,	-2,	carrier_ready_spawn,
	ai_charge,	-2,	CarrierMachineGun,
	ai_charge,	-2,	CarrierMachineGun,
	ai_charge,	-10, carrier_spawn_check,		//12 - actual spawn
	ai_charge,	-2,	CarrierMachineGun,		//13 - begin of wind down
	ai_charge,	-2,	CarrierMachineGun,
	ai_charge,	-2,	CarrierMachineGun,
	ai_charge,	-2,	CarrierMachineGun,
	ai_charge,	-2,	CarrierMachineGun,
	ai_charge,	-2,	carrier_reattack_mg		//18 - end of wind down
};
mmove_t carrier_move_spawn = {FRAME_spawn01, FRAME_spawn18, carrier_frames_spawn, NULL};

mframe_t carrier_frames_pain_heavy [] =
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
	ai_move,	0,	NULL
};
mmove_t carrier_move_pain_heavy = {FRAME_death01, FRAME_death10, carrier_frames_pain_heavy, carrier_run};

mframe_t carrier_frames_pain_light [] =
{
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL,
	ai_move,	0,	NULL
};
mmove_t carrier_move_pain_light = {FRAME_spawn01, FRAME_spawn04, carrier_frames_pain_light, carrier_run};

mframe_t carrier_frames_death [] =
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
	ai_move,	0,	BossExplode
};
mmove_t carrier_move_death = {FRAME_death01, FRAME_death16, carrier_frames_death, carrier_dead};

void carrier_stand (edict_t *self)
{
//	gi.dprintf ("carrier stand\n");
	self->monsterinfo.currentmove = &carrier_move_stand;
}

void carrier_run (edict_t *self)
{

//	gi.dprintf ("carrier run - %2.2f - %s \n", level.time, self->enemy->classname);
	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &carrier_move_stand;
	else
		self->monsterinfo.currentmove = &carrier_move_run;
}

void carrier_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &carrier_move_walk;
}

void CarrierMachineGunHold (edict_t *self)
{
//	self->monsterinfo.aiflags |= AI_HOLD_FRAME;
//	self->yaw_speed = 0;
//	self->monsterinfo.currentmove = &carrier_move_attack_mg;
	CarrierMachineGun (self);
}

void carrier_attack (edict_t *self)
{
	vec3_t	vec;
	float	range, luck;
	qboolean	enemy_inback, enemy_infront, enemy_below;

//	gi.dprintf ("carrier attack\n");
	
	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;

	if ((!self->enemy) || (!self->enemy->inuse))
		return;

	enemy_inback = inback(self, self->enemy);
	enemy_infront = infront (self, self->enemy);
	enemy_below = below (self, self->enemy);

	if (self->bad_area)
	{
		if ((enemy_inback) || (enemy_below))
			self->monsterinfo.currentmove = &carrier_move_attack_rocket;
		else if ((random() < 0.1) || (level.time < self->monsterinfo.attack_finished))
			self->monsterinfo.currentmove = &carrier_move_attack_pre_mg;
		else
		{
			gi.sound (self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
			self->monsterinfo.currentmove = &carrier_move_attack_rail;
		}
		return;
	}

	if (self->monsterinfo.attack_state == AS_BLIND)
	{
		self->monsterinfo.currentmove = &carrier_move_spawn;
		return;
	}

	if (!enemy_inback && !enemy_infront && !enemy_below) // to side and not under
	{
		if ((random() < 0.1) || (level.time < self->monsterinfo.attack_finished)) 
			self->monsterinfo.currentmove = &carrier_move_attack_pre_mg;
		else
		{
			gi.sound (self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
			self->monsterinfo.currentmove = &carrier_move_attack_rail;
		}
		return;
	}

/*	if ((g_showlogic) && (g_showlogic->value))
	{
		gi.dprintf ("checking enemy ..");
		if (enemy_inback)
			gi.dprintf (" in back\n");
		else if (enemy_infront)
			gi.dprintf (" in front\n");
		else
			gi.dprintf (" inaccessible\n");
	}
*/	
	if (enemy_infront)
	{
		VectorSubtract (self->enemy->s.origin, self->s.origin, vec);
		range = VectorLength (vec);
		if (range <= 125)
		{
			if ((random() < 0.8) || (level.time < self->monsterinfo.attack_finished))
				self->monsterinfo.currentmove = &carrier_move_attack_pre_mg;
			else
			{
				gi.sound (self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
				self->monsterinfo.currentmove = &carrier_move_attack_rail;
			}
		}
		else if (range < 600)
		{
			luck = random();
			if (self->monsterinfo.monster_slots > 2)
			{
				if (luck <= 0.20)
					self->monsterinfo.currentmove = &carrier_move_attack_pre_mg;
				else if (luck <= 0.40)
					self->monsterinfo.currentmove = &carrier_move_attack_pre_gren;
				else if ((luck <= 0.7) && !(level.time < self->monsterinfo.attack_finished))
				{
					gi.sound (self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
					self->monsterinfo.currentmove = &carrier_move_attack_rail;
				}
				else
					self->monsterinfo.currentmove = &carrier_move_spawn;
			}
			else
			{
				if (luck <= 0.30)
					self->monsterinfo.currentmove = &carrier_move_attack_pre_mg;
				else if (luck <= 0.65)
					self->monsterinfo.currentmove = &carrier_move_attack_pre_gren;
				else if (level.time >= self->monsterinfo.attack_finished)
				{
					gi.sound (self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
					self->monsterinfo.currentmove = &carrier_move_attack_rail;
				}
				else
					self->monsterinfo.currentmove = &carrier_move_attack_pre_mg;
			}
		}
		else // won't use grenades at this range
		{
			luck = random();
			if (self->monsterinfo.monster_slots > 2)
			{
				if (luck < 0.3)
					self->monsterinfo.currentmove = &carrier_move_attack_pre_mg;
				else if ((luck < 0.65) && !(level.time < self->monsterinfo.attack_finished))
				{
					gi.sound (self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
					VectorCopy (self->enemy->s.origin, self->pos1);	//save for aiming the shot
					self->pos1[2] += self->enemy->viewheight;
					self->monsterinfo.currentmove = &carrier_move_attack_rail;
				}
				else
					self->monsterinfo.currentmove = &carrier_move_spawn;
			}
			else
			{
				if ((luck < 0.45) || (level.time < self->monsterinfo.attack_finished))
					self->monsterinfo.currentmove = &carrier_move_attack_pre_mg;
				else
				{
					gi.sound (self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
					self->monsterinfo.currentmove = &carrier_move_attack_rail;
				}
			}
		}
	}
	else if ((enemy_below) || (enemy_inback))
	{
		self->monsterinfo.currentmove = &carrier_move_attack_rocket;
	}
}

void carrier_attack_mg (edict_t *self)
{
	CarrierCoopCheck(self);
	self->monsterinfo.currentmove = &carrier_move_attack_mg;
}

void carrier_reattack_mg (edict_t *self)
{
	CarrierCoopCheck(self);
	if ( infront(self, self->enemy) )
		if (random() <= 0.5)
			if ((random() < 0.7) || (self->monsterinfo.monster_slots <= 2))
				self->monsterinfo.currentmove = &carrier_move_attack_mg;
			else
				self->monsterinfo.currentmove = &carrier_move_spawn;
		else
			self->monsterinfo.currentmove = &carrier_move_attack_post_mg;
	else
		self->monsterinfo.currentmove = &carrier_move_attack_post_mg;
}


void carrier_attack_gren (edict_t *self)
{
//	gi.dprintf ("carrier_attack_gren - %2.2f\n",level.time);
	CarrierCoopCheck(self);
	self->timestamp = level.time;
	self->monsterinfo.currentmove = &carrier_move_attack_gren;
}

void carrier_reattack_gren (edict_t *self)
{
	CarrierCoopCheck(self);
//	gi.dprintf ("carrier_reattack - %2.2f", level.time);
	if ( infront(self, self->enemy) )
		if (self->timestamp + 1.3 > level.time ) // four grenades
		{
//			gi.dprintf (" attacking\n");
			self->monsterinfo.currentmove = &carrier_move_attack_gren;
			return;
		}
//	gi.dprintf ("not attacking\n");
	self->monsterinfo.currentmove = &carrier_move_attack_post_gren;
}


void carrier_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	qboolean changed = false;

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (skill->value == 3)
		return;		// no pain anims in nightmare

	//	gi.dprintf ("carrier pain\n");
	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 5;

	if (damage < 10)
	{
		gi.sound (self, CHAN_VOICE, sound_pain3, 1, ATTN_NONE, 0);
	}
	else if (damage < 30)
	{
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NONE, 0);
		if (random() < 0.5)
		{
			changed = true;
			self->monsterinfo.currentmove = &carrier_move_pain_light;
		}
	}
	else 
	{
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NONE, 0);
		self->monsterinfo.currentmove = &carrier_move_pain_heavy;
		changed = true;
	}

	// if we changed frames, clean up our little messes
	if (changed)
	{
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
		self->yaw_speed = orig_yaw_speed;
	}
}

void carrier_dead (edict_t *self)
{
	VectorSet (self->mins, -56, -56, 0);
	VectorSet (self->maxs, 56, 56, 80);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity (self);
}

void carrier_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	gi.sound (self, CHAN_VOICE, sound_death, 1, ATTN_NONE, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_NO;
	self->count = 0;
	self->monsterinfo.currentmove = &carrier_move_death;
}

qboolean Carrier_CheckAttack (edict_t *self)
{
	vec3_t	spot1, spot2;
	vec3_t	temp;
	float	chance;
	trace_t	tr;
	qboolean	enemy_infront, enemy_inback, enemy_below;
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
			// go ahead and spawn stuff if we're mad a a client
			if (self->enemy->client && self->monsterinfo.monster_slots > 2)
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
	enemy_inback = inback(self, self->enemy);
	enemy_below = below (self, self->enemy);

	enemy_range = range(self, self->enemy);
	VectorSubtract (self->enemy->s.origin, self->s.origin, temp);
	enemy_yaw = vectoyaw2(temp);

	self->ideal_yaw = enemy_yaw;

	// PMM - shoot out the back if appropriate
	if ((enemy_inback) || (!enemy_infront && enemy_below))
	{
		// this is using wait because the attack is supposed to be independent
		if (level.time >= self->wait)
		{
			self->wait = level.time + CARRIER_ROCKET_TIME;
			self->monsterinfo.attack(self);
			if (random() < 0.6)
				self->monsterinfo.attack_state = AS_SLIDING;
			else
				self->monsterinfo.attack_state = AS_STRAIGHT;
			return true;
		}
	}

	// melee attack
	if (enemy_range == RANGE_MELEE)
	{
		self->monsterinfo.attack_state = AS_MISSILE;
		return true;
	}
	
//	if (level.time < self->monsterinfo.attack_finished)
//		return false;

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
	else if (enemy_range == RANGE_FAR)
	{
		chance = 0.5;
	}

	// PGM - go ahead and shoot every time if it's a info_notnull
	if ((random () < chance) || (self->enemy->solid == SOLID_NOT))
	{
		self->monsterinfo.attack_state = AS_MISSILE;
//		self->monsterinfo.attack_finished = level.time + 2*random();
		return true;
	}

	if (self->flags & FL_FLY)
	{
		if (random() < 0.6)
			self->monsterinfo.attack_state = AS_SLIDING;
		else
			self->monsterinfo.attack_state = AS_STRAIGHT;
	}

	return false;
}

void CarrierPrecache ()
{
	gi.soundindex ("flyer/flysght1.wav");
	gi.soundindex ("flyer/flysrch1.wav");
	gi.soundindex ("flyer/flypain1.wav");
	gi.soundindex ("flyer/flypain2.wav");
	gi.soundindex ("flyer/flyatck2.wav");
	gi.soundindex ("flyer/flyatck1.wav");
	gi.soundindex ("flyer/flydeth1.wav");
	gi.soundindex ("flyer/flyatck3.wav");
	gi.soundindex ("flyer/flyidle1.wav");
	gi.soundindex ("weapons/rockfly.wav");
	gi.soundindex ("infantry/infatck1.wav");
	gi.soundindex ("gunner/gunatck3.wav");
	gi.soundindex ("weapons/grenlb1b.wav");
	gi.soundindex ("tank/rocket.wav");

	gi.modelindex ("models/monsters/flyer/tris.md2");
	gi.modelindex ("models/objects/rocket/tris.md2");
	gi.modelindex ("models/objects/debris2/tris.md2");
	gi.modelindex ("models/objects/grenade/tris.md2");
	gi.modelindex("models/items/spawngro/tris.md2");
	gi.modelindex("models/items/spawngro2/tris.md2");
	gi.modelindex ("models/objects/gibs/sm_metal/tris.md2");
	gi.modelindex ("models/objects/gibs/gear/tris.md2");
}


/*QUAKED monster_carrier (1 .5 0) (-56 -56 -44) (56 56 44) Ambush Trigger_Spawn Sight
*/
void SP_monster_carrier (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	sound_pain1 = gi.soundindex ("carrier/pain_md.wav");
	sound_pain2 = gi.soundindex ("carrier/pain_lg.wav");
	sound_pain3 = gi.soundindex ("carrier/pain_sm.wav");
	sound_death = gi.soundindex ("carrier/death.wav");
//	sound_search1 = gi.soundindex ("bosshovr/bhvunqv1.wav");
	sound_rail = gi.soundindex ("gladiator/railgun.wav");
	sound_sight = gi.soundindex ("carrier/sight.wav");
	sound_spawn = gi.soundindex ("medic_commander/monsterspawn1.wav");

	self->s.sound = gi.soundindex ("bosshovr/bhvengn1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex ("models/monsters/carrier/tris.md2");
	VectorSet (self->mins, -56, -56, -44);
	VectorSet (self->maxs, 56, 56, 44);

	// 2000 - 4000 health
	self->health = max (2000, 2000 + 1000*((skill->value)-1));
	// add health in coop (500 * skill)
	if (coop->value)
		self->health += 500*(skill->value);	

	self->gib_health = -200;
	self->mass = 1000;

	self->yaw_speed = 15;
	orig_yaw_speed = self->yaw_speed;
//	self->yaw_speed = 1;
	
	self->flags |= FL_IMMUNE_LASER;
	self->monsterinfo.aiflags |= AI_IGNORE_SHOTS;

	self->pain = carrier_pain;
	self->die = carrier_die;

	self->monsterinfo.melee = NULL;
	self->monsterinfo.stand = carrier_stand;
	self->monsterinfo.walk = carrier_walk;
	self->monsterinfo.run = carrier_run;
	self->monsterinfo.attack = carrier_attack;
//	self->monsterinfo.search = carrier_search;
	self->monsterinfo.sight = carrier_sight;
	self->monsterinfo.checkattack = Carrier_CheckAttack;
	gi.linkentity (self);

	self->monsterinfo.currentmove = &carrier_move_stand;	
	self->monsterinfo.scale = MODEL_SCALE;

	CarrierPrecache();

	flymonster_start (self);

	self->monsterinfo.attack_finished = 0;
	switch ((int)skill->value)
	{
	case 0:
		self->monsterinfo.monster_slots = 3;
		break;
	case 1:
	case 2:
		self->monsterinfo.monster_slots = 6;
		break;
	case 3:
		self->monsterinfo.monster_slots = 9;
		break;
	default:
		self->monsterinfo.monster_slots = 6;
		break;
	}
}

