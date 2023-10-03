// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

/*
==============================================================================

carrier

==============================================================================
*/

// self->timestamp used for frame calculations in grenade & spawn code
// self->monsterinfo.fire_wait used to prevent rapid refire of rocket launcher

#include "../g_local.h"
#include "m_rogue_carrier.h"
#include "../m_flash.h"

// nb: specifying flyer multiple times so it has a higher chance
constexpr const char *default_reinforcements = "monster_flyer 1;monster_flyer 1;monster_flyer 1;monster_kamikaze 1";
constexpr int32_t default_monster_slots_base = 3;

constexpr gtime_t CARRIER_ROCKET_TIME = 2_sec; // number of seconds between rocket shots
constexpr int32_t CARRIER_ROCKET_SPEED = 750;
constexpr gtime_t RAIL_FIRE_TIME = 3_sec;

bool infront(edict_t *self, edict_t *other);
bool inback(edict_t *self, edict_t *other);
bool below(edict_t *self, edict_t *other);
void drawbbox(edict_t *self);

void ED_CallSpawn(edict_t *ent);

static cached_soundindex sound_pain1;
static cached_soundindex sound_pain2;
static cached_soundindex sound_pain3;
static cached_soundindex sound_death;
static cached_soundindex sound_sight;
static cached_soundindex sound_rail;
static cached_soundindex sound_spawn;

static cached_soundindex sound_cg_down, sound_cg_loop, sound_cg_up;

float orig_yaw_speed;

void M_SetupReinforcements(const char *reinforcements, reinforcement_list_t &list);
std::array<uint8_t, MAX_REINFORCEMENTS> M_PickReinforcements(edict_t *self, int32_t &num_chosen, int32_t max_slots);

extern const mmove_t flyer_move_attack2, flyer_move_attack3, flyer_move_kamikaze;

void carrier_run(edict_t *self);
void carrier_dead(edict_t *self);
void carrier_attack_mg(edict_t *self);
void carrier_reattack_mg(edict_t *self);

void carrier_attack_gren(edict_t *self);
void carrier_reattack_gren(edict_t *self);

void carrier_start_spawn(edict_t *self);
void carrier_spawn_check(edict_t *self);
void carrier_prep_spawn(edict_t *self);

void CarrierMachineGunHold(edict_t *self);
void CarrierRocket(edict_t *self);

MONSTERINFO_SIGHT(carrier_sight) (edict_t *self, edict_t *other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

//
// this is the smarts for the rocket launcher in coop
//
// if there is a player behind/below the carrier, and we can shoot, and we can trace a LOS to them ..
// pick one of the group, and let it rip
void CarrierCoopCheck(edict_t *self)
{
	// no more than 8 players in coop, so..
	std::array<edict_t *, MAX_SPLIT_PLAYERS> targets;
	uint32_t num_targets = 0;
	int32_t  target;
	edict_t *ent;
	trace_t	 tr;

	// if we're not in coop, this is a noop
	// [Paril-KEX] might as well let this work in SP too, so he fires it
	// if you get below him
	//if (!coop->integer)
	//	return;
	// if we are, and we have recently fired, bail
	if (self->monsterinfo.fire_wait > level.time)
		return;

	targets = {};

	// cycle through players
	for (uint32_t player = 1; player <= game.maxclients; player++)
	{
		ent = &g_edicts[player];
		if (!ent->inuse)
			continue;
		if (!ent->client)
			continue;
		if (inback(self, ent) || below(self, ent))
		{
			tr = gi.traceline(self->s.origin, ent->s.origin, self, MASK_SOLID);
			if (tr.fraction == 1.0f)
				targets[num_targets++] = ent;
		}
	}

	if (!num_targets)
		return;

	// get a number from 0 to (num_targets-1)
	target = irandom(num_targets);

	// make sure to prevent rapid fire rockets
	self->monsterinfo.fire_wait = level.time + CARRIER_ROCKET_TIME;

	// save off the real enemy
	ent = self->enemy;
	// set the new guy as temporary enemy
	self->enemy = targets[target];
	CarrierRocket(self);
	// put the real enemy back
	self->enemy = ent;

	// we're done
	return;
}

void CarrierGrenade(edict_t *self)
{
	vec3_t					 start;
	vec3_t					 forward, right, up;
	vec3_t					 aim;
	monster_muzzleflash_id_t flash_number;
	float					 direction; // from lower left to upper right, or lower right to upper left
	float					 spreadR, spreadU;
	int						 mytime;

	CarrierCoopCheck(self);

	if (!self->enemy)
		return;

	if (frandom() < 0.5f)
		direction = -1.0f;
	else
		direction = 1.0f;

	mytime = (int) ((level.time - self->timestamp) / 0.4f).seconds();

	if (mytime == 0)
	{
		spreadR = 0.15f * direction;
		spreadU = 0.1f - 0.1f * direction;
	}
	else if (mytime == 1)
	{
		spreadR = 0;
		spreadU = 0.1f;
	}
	else if (mytime == 2)
	{
		spreadR = -0.15f * direction;
		spreadU = 0.1f - -0.1f * direction;
	}
	else if (mytime == 3)
	{
		spreadR = 0;
		spreadU = 0.1f;
	}
	else
	{
		// error, shoot straight
		spreadR = 0;
		spreadU = 0;
	}

	AngleVectors(self->s.angles, forward, right, up);
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_CARRIER_GRENADE], forward, right);

	aim = self->enemy->s.origin - start;
	aim.normalize();

	aim += (right * spreadR);
	aim += (up * spreadU);

	if (aim[2] > 0.15f)
		aim[2] = 0.15f;
	else if (aim[2] < -0.5f)
		aim[2] = -0.5f;

	flash_number = MZ2_GUNNER_GRENADE_1;
	monster_fire_grenade(self, start, aim, 50, 600, flash_number, (crandom_open() * 10.0f), 200.f + (crandom_open() * 10.0f));
}

void CarrierPredictiveRocket(edict_t *self)
{
	vec3_t forward, right;
	vec3_t start;
	vec3_t dir;

	AngleVectors(self->s.angles, forward, right, nullptr);

	// 1
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_CARRIER_ROCKET_1], forward, right);
	PredictAim(self, self->enemy, start, CARRIER_ROCKET_SPEED, false, -0.3f, &dir, nullptr);
	monster_fire_rocket(self, start, dir, 50, CARRIER_ROCKET_SPEED, MZ2_CARRIER_ROCKET_1);

	// 2
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_CARRIER_ROCKET_2], forward, right);
	PredictAim(self, self->enemy, start, CARRIER_ROCKET_SPEED, false, -0.15f, &dir, nullptr);
	monster_fire_rocket(self, start, dir, 50, CARRIER_ROCKET_SPEED, MZ2_CARRIER_ROCKET_2);

	// 3
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_CARRIER_ROCKET_3], forward, right);
	PredictAim(self, self->enemy, start, CARRIER_ROCKET_SPEED, false, 0, &dir, nullptr);
	monster_fire_rocket(self, start, dir, 50, CARRIER_ROCKET_SPEED, MZ2_CARRIER_ROCKET_3);

	// 4
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_CARRIER_ROCKET_4], forward, right);
	PredictAim(self, self->enemy, start, CARRIER_ROCKET_SPEED, false, 0.15f, &dir, nullptr);
	monster_fire_rocket(self, start, dir, 50, CARRIER_ROCKET_SPEED, MZ2_CARRIER_ROCKET_4);
}

void CarrierRocket(edict_t *self)
{
	vec3_t forward, right;
	vec3_t start;
	vec3_t dir;
	vec3_t vec;

	if (self->enemy)
	{
		if (self->enemy->client && frandom() < 0.5f)
		{
			CarrierPredictiveRocket(self);
			return;
		}
	}
	else
		return;

	AngleVectors(self->s.angles, forward, right, nullptr);

	// 1
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_CARRIER_ROCKET_1], forward, right);
	vec = self->enemy->s.origin;
	vec[2] -= 15;
	dir = vec - start;
	dir.normalize();
	dir += (right * 0.4f);
	dir.normalize();
	monster_fire_rocket(self, start, dir, 50, 500, MZ2_CARRIER_ROCKET_1);

	// 2
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_CARRIER_ROCKET_2], forward, right);
	vec = self->enemy->s.origin;
	dir = vec - start;
	dir.normalize();
	dir += (right * 0.025f);
	dir.normalize();
	monster_fire_rocket(self, start, dir, 50, 500, MZ2_CARRIER_ROCKET_2);

	// 3
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_CARRIER_ROCKET_3], forward, right);
	vec = self->enemy->s.origin;
	dir = vec - start;
	dir.normalize();
	dir += (right * -0.025f);
	dir.normalize();
	monster_fire_rocket(self, start, dir, 50, 500, MZ2_CARRIER_ROCKET_3);

	// 4
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_CARRIER_ROCKET_4], forward, right);
	vec = self->enemy->s.origin;
	vec[2] -= 15;
	dir = vec - start;
	dir.normalize();
	dir += (right * -0.4f);
	dir.normalize();
	monster_fire_rocket(self, start, dir, 50, 500, MZ2_CARRIER_ROCKET_4);
}

void carrier_firebullet_right(edict_t *self)
{
	vec3_t					 forward, right, start;
	monster_muzzleflash_id_t flashnum;

	// if we're in manual steering mode, it means we're leaning down .. use the lower shot
	if (self->monsterinfo.aiflags & AI_MANUAL_STEERING)
		flashnum = MZ2_CARRIER_MACHINEGUN_R2;
	else
		flashnum = MZ2_CARRIER_MACHINEGUN_R1;

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, monster_flash_offset[flashnum], forward, right);
	PredictAim(self, self->enemy, start, 0, true, -0.3f, &forward, nullptr);
	monster_fire_bullet(self, start, forward, 6, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flashnum);
}

void carrier_firebullet_left(edict_t *self)
{
	vec3_t					 forward, right, start;
	monster_muzzleflash_id_t flashnum;

	// if we're in manual steering mode, it means we're leaning down .. use the lower shot
	if (self->monsterinfo.aiflags & AI_MANUAL_STEERING)
		flashnum = MZ2_CARRIER_MACHINEGUN_L2;
	else
		flashnum = MZ2_CARRIER_MACHINEGUN_L1;

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, monster_flash_offset[flashnum], forward, right);
	PredictAim(self, self->enemy, start, 0, true, -0.3f, &forward, nullptr);
	monster_fire_bullet(self, start, forward, 6, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flashnum);
}

void CarrierMachineGun(edict_t *self)
{
	CarrierCoopCheck(self);
	if (self->enemy)
		carrier_firebullet_left(self);
	if (self->enemy)
		carrier_firebullet_right(self);
}

void CarrierSpawn(edict_t *self)
{
	vec3_t	 f, r, offset, startpoint, spawnpoint;
	edict_t *ent;

	//	offset = { 105, 0, -30 }; // real distance needed is (sqrt (56*56*2) + sqrt(16*16*2)) or 101.8
	offset = { 105, 0, -58 }; // real distance needed is (sqrt (56*56*2) + sqrt(16*16*2)) or 101.8
	AngleVectors(self->s.angles, f, r, nullptr);

	startpoint = M_ProjectFlashSource(self, offset, f, r);

	if (self->monsterinfo.chosen_reinforcements[0] == 255)
		return;

	auto &reinforcement = self->monsterinfo.reinforcements.reinforcements[self->monsterinfo.chosen_reinforcements[0]];

	if (FindSpawnPoint(startpoint, reinforcement.mins, reinforcement.maxs, spawnpoint, 32, false))
	{
		ent = CreateFlyMonster(spawnpoint, self->s.angles, reinforcement.mins, reinforcement.maxs, reinforcement.classname);

		if (!ent)
			return;

		gi.sound(self, CHAN_BODY, sound_spawn, 1, ATTN_NONE, 0);

		ent->nextthink = level.time;
		ent->think(ent);

		ent->monsterinfo.aiflags |= AI_SPAWNED_CARRIER | AI_DO_NOT_COUNT | AI_IGNORE_SHOTS;
		ent->monsterinfo.commander = self;
		ent->monsterinfo.monster_slots = reinforcement.strength;
		self->monsterinfo.monster_used += reinforcement.strength;

		if ((self->enemy->inuse) && (self->enemy->health > 0))
		{
			ent->enemy = self->enemy;
			FoundTarget(ent);

			if (!strcmp(ent->classname, "monster_kamikaze"))
			{
				ent->monsterinfo.lefty = false;
				ent->monsterinfo.attack_state = AS_STRAIGHT;
				M_SetAnimation(ent, &flyer_move_kamikaze);
				ent->monsterinfo.aiflags |= AI_CHARGING;
				ent->owner = self;
			}
			else if (!strcmp(ent->classname, "monster_flyer"))
			{
				if (brandom())
				{
					ent->monsterinfo.lefty = false;
					ent->monsterinfo.attack_state = AS_SLIDING;
					M_SetAnimation(ent, &flyer_move_attack3);
				}
				else
				{
					ent->monsterinfo.lefty = true;
					ent->monsterinfo.attack_state = AS_SLIDING;
					M_SetAnimation(ent, &flyer_move_attack3);
				}
			}
		}
	}
}

void carrier_prep_spawn(edict_t *self)
{
	CarrierCoopCheck(self);
	self->monsterinfo.aiflags |= AI_MANUAL_STEERING;
	self->timestamp = level.time;
	self->yaw_speed = 10;
}

void carrier_spawn_check(edict_t *self)
{
	CarrierCoopCheck(self);
	CarrierSpawn(self);

	if (level.time > (self->timestamp + 2.0_sec)) // 0.5 seconds per flyer.  this gets three
	{
		self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
		self->yaw_speed = orig_yaw_speed;
	}
	else
		self->monsterinfo.nextframe = FRAME_spawn08;
}

void carrier_ready_spawn(edict_t *self)
{
	float  current_yaw;
	vec3_t offset, f, r, startpoint, spawnpoint;

	CarrierCoopCheck(self);

	current_yaw = anglemod(self->s.angles[YAW]);

	if (fabsf(current_yaw - self->ideal_yaw) > 0.1f)
	{
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
		self->timestamp += FRAME_TIME_S;
		return;
	}

	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;

	int num_summoned;
	self->monsterinfo.chosen_reinforcements = M_PickReinforcements(self, num_summoned, 1);

	if (!num_summoned)
		return;

	auto &reinforcement = self->monsterinfo.reinforcements.reinforcements[self->monsterinfo.chosen_reinforcements[0]];

	offset = { 105, 0, -58 };
	AngleVectors(self->s.angles, f, r, nullptr);
	startpoint = M_ProjectFlashSource(self, offset, f, r);
	if (FindSpawnPoint(startpoint, reinforcement.mins, reinforcement.maxs, spawnpoint, 32, false))
	{
		float radius = (reinforcement.maxs - reinforcement.mins).length() * 0.5f;

		SpawnGrow_Spawn(spawnpoint + (reinforcement.mins + reinforcement.maxs), radius, radius * 2.f);
	}
}

void carrier_start_spawn(edict_t *self)
{
	int	   mytime;
	float  enemy_yaw;
	vec3_t temp;

	CarrierCoopCheck(self);
	if (!orig_yaw_speed)
		orig_yaw_speed = self->yaw_speed;

	if (!self->enemy)
		return;

	mytime = (int) ((level.time - self->timestamp) / 0.5).seconds();

	temp = self->enemy->s.origin - self->s.origin;
	enemy_yaw = vectoyaw(temp);

	// note that the offsets are based on a forward of 105 from the end angle
	if (mytime == 0)
		self->ideal_yaw = anglemod(enemy_yaw - 30);
	else if (mytime == 1)
		self->ideal_yaw = anglemod(enemy_yaw);
	else if (mytime == 2)
		self->ideal_yaw = anglemod(enemy_yaw + 30);
}

mframe_t carrier_frames_stand[] = {
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }
};
MMOVE_T(carrier_move_stand) = { FRAME_search01, FRAME_search13, carrier_frames_stand, nullptr };

mframe_t carrier_frames_walk[] = {
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 },
	{ ai_walk, 4 }
};
MMOVE_T(carrier_move_walk) = { FRAME_search01, FRAME_search13, carrier_frames_walk, nullptr };

mframe_t carrier_frames_run[] = {
	{ ai_run, 6, CarrierCoopCheck },
	{ ai_run, 6, CarrierCoopCheck },
	{ ai_run, 6, CarrierCoopCheck },
	{ ai_run, 6, CarrierCoopCheck },
	{ ai_run, 6, CarrierCoopCheck },
	{ ai_run, 6, CarrierCoopCheck },
	{ ai_run, 6, CarrierCoopCheck },
	{ ai_run, 6, CarrierCoopCheck },
	{ ai_run, 6, CarrierCoopCheck },
	{ ai_run, 6, CarrierCoopCheck },
	{ ai_run, 6, CarrierCoopCheck },
	{ ai_run, 6, CarrierCoopCheck },
	{ ai_run, 6, CarrierCoopCheck }
};
MMOVE_T(carrier_move_run) = { FRAME_search01, FRAME_search13, carrier_frames_run, nullptr };

static void CarrierSpool(edict_t *self)
{
	CarrierCoopCheck(self);
	gi.sound(self, CHAN_BODY, sound_cg_up, 1, 0.5f, 0);

	self->monsterinfo.weapon_sound = sound_cg_loop;
}

mframe_t carrier_frames_attack_pre_mg[] = {
	{ ai_charge, 4, CarrierSpool },
	{ ai_charge, 4, CarrierCoopCheck },
	{ ai_charge, 4, CarrierCoopCheck },
	{ ai_charge, 4, CarrierCoopCheck },
	{ ai_charge, 4, CarrierCoopCheck },
	{ ai_charge, 4, CarrierCoopCheck },
	{ ai_charge, 4, CarrierCoopCheck },
	{ ai_charge, 4, carrier_attack_mg }
};
MMOVE_T(carrier_move_attack_pre_mg) = { FRAME_firea01, FRAME_firea08, carrier_frames_attack_pre_mg, nullptr };

// Loop this
mframe_t carrier_frames_attack_mg[] = {
	{ ai_charge, -2, CarrierMachineGun },
	{ ai_charge, -2, CarrierMachineGun },
	{ ai_charge, -2, carrier_reattack_mg }
};
MMOVE_T(carrier_move_attack_mg) = { FRAME_firea09, FRAME_firea11, carrier_frames_attack_mg, nullptr };

mframe_t carrier_frames_attack_post_mg[] = {
	{ ai_charge, 4, CarrierCoopCheck },
	{ ai_charge, 4, CarrierCoopCheck },
	{ ai_charge, 4, CarrierCoopCheck },
	{ ai_charge, 4, CarrierCoopCheck }
};
MMOVE_T(carrier_move_attack_post_mg) = { FRAME_firea12, FRAME_firea15, carrier_frames_attack_post_mg, carrier_run };

mframe_t carrier_frames_attack_pre_gren[] = {
	{ ai_charge, 4, CarrierCoopCheck },
	{ ai_charge, 4, CarrierCoopCheck },
	{ ai_charge, 4, CarrierCoopCheck },
	{ ai_charge, 4, CarrierCoopCheck },
	{ ai_charge, 4, CarrierCoopCheck },
	{ ai_charge, 4, carrier_attack_gren }
};
MMOVE_T(carrier_move_attack_pre_gren) = { FRAME_fireb01, FRAME_fireb06, carrier_frames_attack_pre_gren, nullptr };

mframe_t carrier_frames_attack_gren[] = {
	{ ai_charge, -15, CarrierGrenade },
	{ ai_charge, 4, CarrierCoopCheck },
	{ ai_charge, 4, CarrierCoopCheck },
	{ ai_charge, 4, carrier_reattack_gren }
};
MMOVE_T(carrier_move_attack_gren) = { FRAME_fireb07, FRAME_fireb10, carrier_frames_attack_gren, nullptr };

mframe_t carrier_frames_attack_post_gren[] = {
	{ ai_charge, 4, CarrierCoopCheck },
	{ ai_charge, 4, CarrierCoopCheck },
	{ ai_charge, 4, CarrierCoopCheck },
	{ ai_charge, 4, CarrierCoopCheck },
	{ ai_charge, 4, CarrierCoopCheck },
	{ ai_charge, 4, CarrierCoopCheck }
};
MMOVE_T(carrier_move_attack_post_gren) = { FRAME_fireb11, FRAME_fireb16, carrier_frames_attack_post_gren, carrier_run };

mframe_t carrier_frames_attack_rocket[] = {
	{ ai_charge, 15, CarrierRocket }
};
MMOVE_T(carrier_move_attack_rocket) = { FRAME_fireb01, FRAME_fireb01, carrier_frames_attack_rocket, carrier_run };

void CarrierRail(edict_t *self)
{
	vec3_t start;
	vec3_t dir;
	vec3_t forward, right;

	CarrierCoopCheck(self);
	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_CARRIER_RAILGUN], forward, right);

	// calc direction to where we targeted
	dir = self->pos1 - start;
	dir.normalize();

	monster_fire_railgun(self, start, dir, 50, 100, MZ2_CARRIER_RAILGUN);
	self->monsterinfo.attack_finished = level.time + RAIL_FIRE_TIME;
}

void CarrierSaveLoc(edict_t *self)
{
	CarrierCoopCheck(self);
	self->pos1 = self->enemy->s.origin; // save for aiming the shot
	self->pos1[2] += self->enemy->viewheight;
};

mframe_t carrier_frames_attack_rail[] = {
	{ ai_charge, 2, CarrierCoopCheck },
	{ ai_charge, 2, CarrierSaveLoc },
	{ ai_charge, 2, CarrierCoopCheck },
	{ ai_charge, -20, CarrierRail },
	{ ai_charge, 2, CarrierCoopCheck },
	{ ai_charge, 2, CarrierCoopCheck },
	{ ai_charge, 2, CarrierCoopCheck },
	{ ai_charge, 2, CarrierCoopCheck },
	{ ai_charge, 2, CarrierCoopCheck }
};
MMOVE_T(carrier_move_attack_rail) = { FRAME_search01, FRAME_search09, carrier_frames_attack_rail, carrier_run };

mframe_t carrier_frames_spawn[] = {
	{ ai_charge, -2 },
	{ ai_charge, -2 },
	{ ai_charge, -2 },
	{ ai_charge, -2 },
	{ ai_charge, -2 },
	{ ai_charge, -2 },
	{ ai_charge, -2, carrier_prep_spawn },	// 7 - end of wind down
	{ ai_charge, -2, carrier_start_spawn }, // 8 - start of spawn
	{ ai_charge, -2, carrier_ready_spawn },
	{ ai_charge, -2 },
	{ ai_charge, -2 },
	{ ai_charge, -10, carrier_spawn_check }, // 12 - actual spawn
	{ ai_charge, -2 },	 // 13 - begin of wind down
	{ ai_charge, -2 },
	{ ai_charge, -2 },
	{ ai_charge, -2 },
	{ ai_charge, -2 },
	{ ai_charge, -2 } // 18 - end of wind down
};
MMOVE_T(carrier_move_spawn) = { FRAME_spawn01, FRAME_spawn18, carrier_frames_spawn, carrier_run };

mframe_t carrier_frames_pain_heavy[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(carrier_move_pain_heavy) = { FRAME_death01, FRAME_death10, carrier_frames_pain_heavy, carrier_run };

mframe_t carrier_frames_pain_light[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(carrier_move_pain_light) = { FRAME_spawn01, FRAME_spawn04, carrier_frames_pain_light, carrier_run };

mframe_t carrier_frames_death[] = {
	{ ai_move, 0, BossExplode },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(carrier_move_death) = { FRAME_death01, FRAME_death16, carrier_frames_death, carrier_dead };

MONSTERINFO_STAND(carrier_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &carrier_move_stand);
}

MONSTERINFO_RUN(carrier_run) (edict_t *self) -> void
{
	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		M_SetAnimation(self, &carrier_move_stand);
	else
		M_SetAnimation(self, &carrier_move_run);
}

MONSTERINFO_WALK(carrier_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &carrier_move_walk);
}

void CarrierMachineGunHold(edict_t *self)
{
	CarrierMachineGun(self);
}

MONSTERINFO_ATTACK(carrier_attack) (edict_t *self) -> void
{
	vec3_t vec;
	float  range, luck;
	bool   enemy_inback, enemy_infront, enemy_below;

	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;

	if ((!self->enemy) || (!self->enemy->inuse))
		return;

	enemy_inback = inback(self, self->enemy);
	enemy_infront = infront(self, self->enemy);
	enemy_below = below(self, self->enemy);

	if (self->bad_area)
	{
		if ((enemy_inback) || (enemy_below))
			M_SetAnimation(self, &carrier_move_attack_rocket);
		else if ((frandom() < 0.1f) || (level.time < self->monsterinfo.attack_finished))
			M_SetAnimation(self, &carrier_move_attack_pre_mg);
		else
		{
			gi.sound(self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
			M_SetAnimation(self, &carrier_move_attack_rail);
		}
		return;
	}

	if (self->monsterinfo.attack_state == AS_BLIND)
	{
		M_SetAnimation(self, &carrier_move_spawn);
		return;
	}

	if (!enemy_inback && !enemy_infront && !enemy_below) // to side and not under
	{
		if ((frandom() < 0.1f) || (level.time < self->monsterinfo.attack_finished))
			M_SetAnimation(self, &carrier_move_attack_pre_mg);
		else
		{
			gi.sound(self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
			M_SetAnimation(self, &carrier_move_attack_rail);
		}
		return;
	}

	if (enemy_infront)
	{
		vec = self->enemy->s.origin - self->s.origin;
		range = vec.length();
		if (range <= 125)
		{
			if ((frandom() < 0.8f) || (level.time < self->monsterinfo.attack_finished))
				M_SetAnimation(self, &carrier_move_attack_pre_mg);
			else
			{
				gi.sound(self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
				M_SetAnimation(self, &carrier_move_attack_rail);
			}
		}
		else if (range < 600)
		{
			luck = frandom();
			if (M_SlotsLeft(self) > 2)
			{
				if (luck <= 0.20f)
					M_SetAnimation(self, &carrier_move_attack_pre_mg);
				else if (luck <= 0.40f)
					M_SetAnimation(self, &carrier_move_attack_pre_gren);
				else if ((luck <= 0.7f) && !(level.time < self->monsterinfo.attack_finished))
				{
					gi.sound(self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
					M_SetAnimation(self, &carrier_move_attack_rail);
				}
				else
					M_SetAnimation(self, &carrier_move_spawn);
			}
			else
			{
				if (luck <= 0.30f)
					M_SetAnimation(self, &carrier_move_attack_pre_mg);
				else if (luck <= 0.65f)
					M_SetAnimation(self, &carrier_move_attack_pre_gren);
				else if (level.time >= self->monsterinfo.attack_finished)
				{
					gi.sound(self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
					M_SetAnimation(self, &carrier_move_attack_rail);
				}
				else
					M_SetAnimation(self, &carrier_move_attack_pre_mg);
			}
		}
		else // won't use grenades at this range
		{
			luck = frandom();
			if (M_SlotsLeft(self) > 2)
			{
				if (luck < 0.3f)
					M_SetAnimation(self, &carrier_move_attack_pre_mg);
				else if ((luck < 0.65f) && !(level.time < self->monsterinfo.attack_finished))
				{
					gi.sound(self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
					self->pos1 = self->enemy->s.origin; // save for aiming the shot
					self->pos1[2] += self->enemy->viewheight;
					M_SetAnimation(self, &carrier_move_attack_rail);
				}
				else
					M_SetAnimation(self, &carrier_move_spawn);
			}
			else
			{
				if ((luck < 0.45f) || (level.time < self->monsterinfo.attack_finished))
					M_SetAnimation(self, &carrier_move_attack_pre_mg);
				else
				{
					gi.sound(self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
					M_SetAnimation(self, &carrier_move_attack_rail);
				}
			}
		}
	}
	else if ((enemy_below) || (enemy_inback))
	{
		M_SetAnimation(self, &carrier_move_attack_rocket);
	}
}

void carrier_attack_mg(edict_t *self)
{
	CarrierCoopCheck(self);
	M_SetAnimation(self, &carrier_move_attack_mg);
	self->monsterinfo.melee_debounce_time = level.time + random_time(1.2_sec, 2_sec);
}

void carrier_reattack_mg(edict_t *self)
{
	CarrierMachineGun(self);

	CarrierCoopCheck(self);
	if (visible(self, self->enemy) && infront(self, self->enemy))
	{
		if (frandom() < 0.6f)
		{
			self->monsterinfo.melee_debounce_time += random_time(250_ms, 500_ms);
			M_SetAnimation(self, &carrier_move_attack_mg);
			return;
		}
		else if (self->monsterinfo.melee_debounce_time > level.time)
		{
			M_SetAnimation(self, &carrier_move_attack_mg);
			return;
		}
	}

	M_SetAnimation(self, &carrier_move_attack_post_mg);
	self->monsterinfo.weapon_sound = 0;
	gi.sound(self, CHAN_BODY, sound_cg_down, 1, 0.5f, 0);
}

void carrier_attack_gren(edict_t *self)
{
	CarrierCoopCheck(self);
	self->timestamp = level.time;
	M_SetAnimation(self, &carrier_move_attack_gren);
}

void carrier_reattack_gren(edict_t *self)
{
	CarrierCoopCheck(self);
	if (infront(self, self->enemy))
		if (self->timestamp + 1.3_sec > level.time) // four grenades
		{
			M_SetAnimation(self, &carrier_move_attack_gren);
			return;
		}
	M_SetAnimation(self, &carrier_move_attack_post_gren);
}

PAIN(carrier_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	bool changed = false;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 5_sec;

	if (damage < 10)
		gi.sound(self, CHAN_VOICE, sound_pain3, 1, ATTN_NONE, 0);
	else if (damage < 30)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NONE, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NONE, 0);
	
	if (!M_ShouldReactToPain(self, mod))
		return; // no pain anims in nightmare

	self->monsterinfo.weapon_sound = 0;

	if (damage >= 10)
	{
		if (damage < 30)
		{
			if (mod.id == MOD_CHAINFIST || frandom() < 0.5f)
			{
				changed = true;
				M_SetAnimation(self, &carrier_move_pain_light);
			}
		}
		else
		{
			M_SetAnimation(self, &carrier_move_pain_heavy);
			changed = true;
		}
	}

	// if we changed frames, clean up our little messes
	if (changed)
	{
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
		self->yaw_speed = orig_yaw_speed;
	}
}

MONSTERINFO_SETSKIN(carrier_setskin) (edict_t *self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

void carrier_dead(edict_t *self)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1_BIG);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PHS, false);

	self->s.sound = 0;
	self->s.skinnum /= 2;

	self->gravityVector.z = -1.0f;

	ThrowGibs(self, 500, {
		{ 2, "models/objects/gibs/sm_meat/tris.md2" },
		{ 3, "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC },
		{ "models/monsters/carrier/gibs/base.md2", GIB_SKINNED },
		{ "models/monsters/carrier/gibs/chest.md2", GIB_SKINNED | GIB_UPRIGHT },
		{ "models/monsters/carrier/gibs/gl.md2", GIB_SKINNED },
		{ "models/monsters/carrier/gibs/lcg.md2", GIB_SKINNED | GIB_UPRIGHT },
		{ "models/monsters/carrier/gibs/lwing.md2", GIB_SKINNED | GIB_UPRIGHT },
		{ "models/monsters/carrier/gibs/rcg.md2", GIB_SKINNED | GIB_UPRIGHT },
		{ "models/monsters/carrier/gibs/rwing.md2", GIB_SKINNED | GIB_UPRIGHT },
		{ 2, "models/monsters/carrier/gibs/spawner.md2", GIB_SKINNED },
		{ 2, "models/monsters/carrier/gibs/thigh.md2", GIB_SKINNED },
		{ "models/monsters/carrier/gibs/head.md2", GIB_SKINNED | GIB_METALLIC | GIB_HEAD }
	});
}

DIE(carrier_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NONE, 0);
	self->deadflag = true;
	self->takedamage = false;
	self->count = 0;
	M_SetAnimation(self, &carrier_move_death);
	self->velocity = {};
	self->gravityVector.z *= 0.01f;
	self->monsterinfo.weapon_sound = 0;
}

MONSTERINFO_CHECKATTACK(Carrier_CheckAttack) (edict_t *self) -> bool
{
	bool enemy_infront = infront(self, self->enemy);
	bool enemy_inback = inback(self, self->enemy);
	bool enemy_below = below(self, self->enemy);

	// PMM - shoot out the back if appropriate
	if ((enemy_inback) || (!enemy_infront && enemy_below))
	{
		// this is using wait because the attack is supposed to be independent
		if (level.time >= self->monsterinfo.fire_wait)
		{
			self->monsterinfo.fire_wait = level.time + CARRIER_ROCKET_TIME;
			self->monsterinfo.attack(self);
			if (frandom() < 0.6f)
				self->monsterinfo.attack_state = AS_SLIDING;
			else
				self->monsterinfo.attack_state = AS_STRAIGHT;
			return true;
		}
	}

	return M_CheckAttack_Base(self, 0.4f, 0.8f, 0.8f, 0.8f, 0.5f, 0.f);
}

void CarrierPrecache()
{
	gi.soundindex("flyer/flysght1.wav");
	gi.soundindex("flyer/flysrch1.wav");
	gi.soundindex("flyer/flypain1.wav");
	gi.soundindex("flyer/flypain2.wav");
	gi.soundindex("flyer/flyatck2.wav");
	gi.soundindex("flyer/flyatck1.wav");
	gi.soundindex("flyer/flydeth1.wav");
	gi.soundindex("flyer/flyatck3.wav");
	gi.soundindex("flyer/flyidle1.wav");
	gi.soundindex("weapons/rockfly.wav");
	gi.soundindex("infantry/infatck1.wav");
	gi.soundindex("gunner/gunatck3.wav");
	gi.soundindex("weapons/grenlb1b.wav");
	gi.soundindex("tank/rocket.wav");

	gi.modelindex("models/monsters/flyer/tris.md2");
	gi.modelindex("models/objects/rocket/tris.md2");
	gi.modelindex("models/objects/debris2/tris.md2");
	gi.modelindex("models/objects/grenade/tris.md2");
	gi.modelindex("models/items/spawngro3/tris.md2");
	gi.modelindex("models/objects/gibs/sm_metal/tris.md2");
	gi.modelindex("models/objects/gibs/gear/tris.md2");
}

/*QUAKED monster_carrier (1 .5 0) (-56 -56 -44) (56 56 44) Ambush Trigger_Spawn Sight
 */
void SP_monster_carrier(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	sound_pain1.assign("carrier/pain_md.wav");
	sound_pain2.assign("carrier/pain_lg.wav");
	sound_pain3.assign("carrier/pain_sm.wav");
	sound_death.assign("carrier/death.wav");
	sound_rail.assign("gladiator/railgun.wav");
	sound_sight.assign("carrier/sight.wav");
	sound_spawn.assign("medic_commander/monsterspawn1.wav");

	sound_cg_down.assign("weapons/chngnd1a.wav");
	sound_cg_loop.assign("weapons/chngnl1a.wav");
	sound_cg_up.assign("weapons/chngnu1a.wav");

	self->monsterinfo.engine_sound = gi.soundindex("bosshovr/bhvengn1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/carrier/tris.md2");
	
	gi.modelindex("models/monsters/carrier/gibs/base.md2");
	gi.modelindex("models/monsters/carrier/gibs/chest.md2");
	gi.modelindex("models/monsters/carrier/gibs/gl.md2");
	gi.modelindex("models/monsters/carrier/gibs/head.md2");
	gi.modelindex("models/monsters/carrier/gibs/lcg.md2");
	gi.modelindex("models/monsters/carrier/gibs/lwing.md2");
	gi.modelindex("models/monsters/carrier/gibs/rcg.md2");
	gi.modelindex("models/monsters/carrier/gibs/rwing.md2");
	gi.modelindex("models/monsters/carrier/gibs/spawner.md2");
	gi.modelindex("models/monsters/carrier/gibs/thigh.md2");

	self->mins = { -56, -56, -44 };
	self->maxs = { 56, 56, 44 };

	// 2000 - 4000 health
	self->health = max(2000, 2000 + 1000 * (skill->integer - 1)) * st.health_multiplier;
	// add health in coop (500 * skill)
	if (coop->integer)
		self->health += 500 * skill->integer;

	self->gib_health = -200;
	self->mass = 1000;

	self->yaw_speed = 15;
	orig_yaw_speed = self->yaw_speed;

	self->flags |= FL_IMMUNE_LASER;
	self->monsterinfo.aiflags |= AI_IGNORE_SHOTS;

	self->pain = carrier_pain;
	self->die = carrier_die;

	self->monsterinfo.melee = nullptr;
	self->monsterinfo.stand = carrier_stand;
	self->monsterinfo.walk = carrier_walk;
	self->monsterinfo.run = carrier_run;
	self->monsterinfo.attack = carrier_attack;
	self->monsterinfo.sight = carrier_sight;
	self->monsterinfo.checkattack = Carrier_CheckAttack;
	self->monsterinfo.setskin = carrier_setskin;
	gi.linkentity(self);

	M_SetAnimation(self, &carrier_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	CarrierPrecache();

	flymonster_start(self);

	self->monsterinfo.attack_finished = 0_ms;

	const char *reinforcements = default_reinforcements;

	if (!st.was_key_specified("monster_slots"))
		self->monsterinfo.monster_slots = default_monster_slots_base;
	if (st.was_key_specified("reinforcements"))
		reinforcements = st.reinforcements;

	if (self->monsterinfo.monster_slots && reinforcements && *reinforcements)
	{
		if (skill->integer)
			self->monsterinfo.monster_slots += floor(self->monsterinfo.monster_slots * (skill->value / 2.f));

		M_SetupReinforcements(reinforcements, self->monsterinfo.reinforcements);
	}

	self->monsterinfo.aiflags |= AI_ALTERNATE_FLY;
	self->monsterinfo.fly_acceleration = 5.f;
	self->monsterinfo.fly_speed = 50.f;
	self->monsterinfo.fly_above = true;
	self->monsterinfo.fly_min_distance = 1000.f;
	self->monsterinfo.fly_max_distance = 1000.f;
}
