// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

black widow

==============================================================================
*/

// self->timestamp used to prevent rapid fire of railgun
// self->plat2flags used for fire count (flashes)

#include "../g_local.h"
#include "m_rogue_widow.h"
#include "../m_flash.h"

constexpr gtime_t RAIL_TIME = 3_sec;
constexpr gtime_t BLASTER_TIME = 2_sec;
constexpr int	BLASTER2_DAMAGE = 10;
constexpr int	WIDOW_RAIL_DAMAGE = 50;

bool infront(edict_t *self, edict_t *other);

static cached_soundindex sound_pain1;
static cached_soundindex sound_pain2;
static cached_soundindex sound_pain3;
static cached_soundindex sound_rail;

static uint32_t shotsfired;

constexpr vec3_t spawnpoints[] = {
	{ 30, 100, 16 },
	{ 30, -100, 16 }
};

constexpr vec3_t beameffects[] = {
	{ 12.58f, -43.71f, 68.88f },
	{ 3.43f, 58.72f, 68.41f }
};

constexpr float sweep_angles[] = {
	32.f, 26.f, 20.f, 10.f, 0.f, -6.5f, -13.f, -27.f, -41.f
};

constexpr vec3_t stalker_mins = { -28, -28, -18 };
constexpr vec3_t stalker_maxs = { 28, 28, 18 };

unsigned int widow_damage_multiplier;

void widow_run(edict_t *self);
void widow_dead(edict_t *self);
void widow_attack_blaster(edict_t *self);
void widow_reattack_blaster(edict_t *self);

void widow_start_spawn(edict_t *self);
void widow_done_spawn(edict_t *self);
void widow_spawn_check(edict_t *self);
void widow_prep_spawn(edict_t *self);
void widow_attack_rail(edict_t *self);

void widow_start_run_5(edict_t *self);
void widow_start_run_10(edict_t *self);
void widow_start_run_12(edict_t *self);

void WidowCalcSlots(edict_t *self);

MONSTERINFO_SEARCH(widow_search) (edict_t *self) -> void
{
}

MONSTERINFO_SIGHT(widow_sight) (edict_t *self, edict_t *other) -> void
{
	self->monsterinfo.fire_wait = 0_ms;
}

extern const mmove_t widow_move_attack_post_blaster;
extern const mmove_t widow_move_attack_post_blaster_r;
extern const mmove_t widow_move_attack_post_blaster_l;
extern const mmove_t widow_move_attack_blaster;

float target_angle(edict_t *self)
{
	vec3_t target;
	float  enemy_yaw;

	target = self->s.origin - self->enemy->s.origin;
	enemy_yaw = self->s.angles[YAW] - vectoyaw(target);
	if (enemy_yaw < 0)
		enemy_yaw += 360.0f;

	// this gets me 0 degrees = forward
	enemy_yaw -= 180.0f;
	// positive is to right, negative to left

	return enemy_yaw;
}

int WidowTorso(edict_t *self)
{
	float enemy_yaw = target_angle(self);

	if (enemy_yaw >= 105)
	{
		M_SetAnimation(self, &widow_move_attack_post_blaster_r);
		self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
		return 0;
	}

	if (enemy_yaw <= -75.0f)
	{
		M_SetAnimation(self, &widow_move_attack_post_blaster_l);
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

	return 0;
}

constexpr float VARIANCE = 15.0f;

void WidowBlaster(edict_t *self)
{
	vec3_t					 forward, right, target, vec, targ_angles;
	vec3_t					 start;
	monster_muzzleflash_id_t flashnum;
	effects_t				 effect;

	if (!self->enemy)
		return;

	shotsfired++;
	if (!(shotsfired % 4))
		effect = EF_BLASTER;
	else
		effect = EF_NONE;

	AngleVectors(self->s.angles, forward, right, nullptr);
	if ((self->s.frame >= FRAME_spawn05) && (self->s.frame <= FRAME_spawn13))
	{
		// sweep
		flashnum = static_cast<monster_muzzleflash_id_t>(MZ2_WIDOW_BLASTER_SWEEP1 + self->s.frame - FRAME_spawn05);
		start = G_ProjectSource(self->s.origin, monster_flash_offset[flashnum], forward, right);
		target = self->enemy->s.origin - start;
		targ_angles = vectoangles(target);

		vec = self->s.angles;

		vec[PITCH] += targ_angles[PITCH];
		vec[YAW] -= sweep_angles[flashnum - MZ2_WIDOW_BLASTER_SWEEP1];

		AngleVectors(vec, forward, nullptr, nullptr);
		monster_fire_blaster2(self, start, forward, BLASTER2_DAMAGE * widow_damage_multiplier, 1000, flashnum, effect);
	}
	else if ((self->s.frame >= FRAME_fired02a) && (self->s.frame <= FRAME_fired20))
	{
		vec3_t angles;
		float  aim_angle, target_angle;
		float  error;

		self->monsterinfo.aiflags |= AI_MANUAL_STEERING;

		self->monsterinfo.nextframe = WidowTorso(self);

		if (!self->monsterinfo.nextframe)
			self->monsterinfo.nextframe = self->s.frame;

		if (self->s.frame == FRAME_fired02a)
			flashnum = MZ2_WIDOW_BLASTER_0;
		else
			flashnum = static_cast<monster_muzzleflash_id_t>(MZ2_WIDOW_BLASTER_100 + self->s.frame - FRAME_fired03);

		start = G_ProjectSource(self->s.origin, monster_flash_offset[flashnum], forward, right);

		PredictAim(self, self->enemy, start, 1000, true, crandom() * 0.1f, &forward, nullptr);

		// clamp it to within 10 degrees of the aiming angle (where she's facing)
		angles = vectoangles(forward);
		// give me 100 -> -70
		aim_angle = (float) (100 - (10 * (flashnum - MZ2_WIDOW_BLASTER_100)));
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
			angles[YAW] = (self->s.angles[YAW] - aim_angle) + VARIANCE;
			AngleVectors(angles, forward, nullptr, nullptr);
		}
		else if (error < -VARIANCE)
		{
			angles[YAW] = (self->s.angles[YAW] - aim_angle) - VARIANCE;
			AngleVectors(angles, forward, nullptr, nullptr);
		}

		monster_fire_blaster2(self, start, forward, BLASTER2_DAMAGE * widow_damage_multiplier, 1000, flashnum, effect);
	}
	else if ((self->s.frame >= FRAME_run01) && (self->s.frame <= FRAME_run08))
	{
		flashnum = static_cast<monster_muzzleflash_id_t>(MZ2_WIDOW_RUN_1 + self->s.frame - FRAME_run01);
		start = G_ProjectSource(self->s.origin, monster_flash_offset[flashnum], forward, right);

		target = self->enemy->s.origin - start;
		target[2] += self->enemy->viewheight;
		target.normalize();

		monster_fire_blaster2(self, start, target, BLASTER2_DAMAGE * widow_damage_multiplier, 1000, flashnum, effect);
	}
}

void WidowSpawn(edict_t *self)
{
	vec3_t	 f, r, u, offset, startpoint, spawnpoint;
	edict_t *ent, *designated_enemy;
	int		 i;

	AngleVectors(self->s.angles, f, r, u);

	for (i = 0; i < 2; i++)
	{
		offset = spawnpoints[i];

		startpoint = G_ProjectSource2(self->s.origin, offset, f, r, u);

		if (FindSpawnPoint(startpoint, stalker_mins, stalker_maxs, spawnpoint, 64))
		{
			ent = CreateGroundMonster(spawnpoint, self->s.angles, stalker_mins, stalker_maxs, "monster_stalker", 256);

			if (!ent)
				continue;

			self->monsterinfo.monster_used++;
			ent->monsterinfo.commander = self;

			ent->nextthink = level.time;
			ent->think(ent);

			ent->monsterinfo.aiflags |= AI_SPAWNED_WIDOW | AI_DO_NOT_COUNT | AI_IGNORE_SHOTS;

			if (!coop->integer)
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
						if (!designated_enemy)
							designated_enemy = self->enemy;
					}
				}
				else
					designated_enemy = self->enemy;
			}

			if ((designated_enemy->inuse) && (designated_enemy->health > 0))
			{
				ent->enemy = designated_enemy;
				FoundTarget(ent);
				ent->monsterinfo.attack(ent);
			}
		}
	}
}

void widow_spawn_check(edict_t *self)
{
	WidowBlaster(self);
	WidowSpawn(self);
}

void widow_ready_spawn(edict_t *self)
{
	vec3_t f, r, u, offset, startpoint, spawnpoint;
	int	   i;

	WidowBlaster(self);
	AngleVectors(self->s.angles, f, r, u);

	for (i = 0; i < 2; i++)
	{
		offset = spawnpoints[i];
		startpoint = G_ProjectSource2(self->s.origin, offset, f, r, u);
		if (FindSpawnPoint(startpoint, stalker_mins, stalker_maxs, spawnpoint, 64))
		{
			float radius = (stalker_maxs - stalker_mins).length() * 0.5f;

			SpawnGrow_Spawn(spawnpoint + (stalker_mins + stalker_maxs), radius, radius * 2.f);
		}
	}
}

void widow_step(edict_t *self)
{
	gi.sound(self, CHAN_BODY, gi.soundindex("widow/bwstep3.wav"), 1, ATTN_NORM, 0);
}

mframe_t widow_frames_stand[] = {
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
MMOVE_T(widow_move_stand) = { FRAME_idle01, FRAME_idle11, widow_frames_stand, nullptr };

mframe_t widow_frames_walk[] = {
	{ ai_walk, 2.79f, widow_step },
	{ ai_walk, 2.77f },
	{ ai_walk, 3.53f },
	{ ai_walk, 3.97f },
	{ ai_walk, 4.13f }, // 5
	{ ai_walk, 4.09f },
	{ ai_walk, 3.84f },
	{ ai_walk, 3.62f, widow_step },
	{ ai_walk, 3.29f },
	{ ai_walk, 6.08f }, // 10
	{ ai_walk, 6.94f },
	{ ai_walk, 5.73f },
	{ ai_walk, 2.85f }
};
MMOVE_T(widow_move_walk) = { FRAME_walk01, FRAME_walk13, widow_frames_walk, nullptr };

mframe_t widow_frames_run[] = {
	{ ai_run, 2.79f, widow_step },
	{ ai_run, 2.77f },
	{ ai_run, 3.53f },
	{ ai_run, 3.97f },
	{ ai_run, 4.13f }, // 5
	{ ai_run, 4.09f },
	{ ai_run, 3.84f },
	{ ai_run, 3.62f, widow_step },
	{ ai_run, 3.29f },
	{ ai_run, 6.08f }, // 10
	{ ai_run, 6.94f },
	{ ai_run, 5.73f },
	{ ai_run, 2.85f }
};
MMOVE_T(widow_move_run) = { FRAME_walk01, FRAME_walk13, widow_frames_run, nullptr };

void widow_stepshoot(edict_t *self)
{
	gi.sound(self, CHAN_BODY, gi.soundindex("widow/bwstep2.wav"), 1, ATTN_NORM, 0);
	WidowBlaster(self);
}

mframe_t widow_frames_run_attack[] = {
	{ ai_charge, 13, widow_stepshoot },
	{ ai_charge, 11.72f, WidowBlaster },
	{ ai_charge, 18.04f, WidowBlaster },
	{ ai_charge, 14.58f, WidowBlaster },
	{ ai_charge, 13, widow_stepshoot }, // 5
	{ ai_charge, 12.12f, WidowBlaster },
	{ ai_charge, 19.63f, WidowBlaster },
	{ ai_charge, 11.37f, WidowBlaster }
};
MMOVE_T(widow_move_run_attack) = { FRAME_run01, FRAME_run08, widow_frames_run_attack, widow_run };

//
// These three allow specific entry into the run sequence
//

void widow_start_run_5(edict_t *self)
{
	M_SetAnimation(self, &widow_move_run);
	self->monsterinfo.nextframe = FRAME_walk05;
}

void widow_start_run_10(edict_t *self)
{
	M_SetAnimation(self, &widow_move_run);
	self->monsterinfo.nextframe = FRAME_walk10;
}

void widow_start_run_12(edict_t *self)
{
	M_SetAnimation(self, &widow_move_run);
	self->monsterinfo.nextframe = FRAME_walk12;
}

mframe_t widow_frames_attack_pre_blaster[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, widow_attack_blaster }
};
MMOVE_T(widow_move_attack_pre_blaster) = { FRAME_fired01, FRAME_fired02a, widow_frames_attack_pre_blaster, nullptr };

// Loop this
mframe_t widow_frames_attack_blaster[] = {
	{ ai_charge, 0, widow_reattack_blaster }, // straight ahead
	{ ai_charge, 0, widow_reattack_blaster }, // 100 degrees right
	{ ai_charge, 0, widow_reattack_blaster },
	{ ai_charge, 0, widow_reattack_blaster },
	{ ai_charge, 0, widow_reattack_blaster },
	{ ai_charge, 0, widow_reattack_blaster },
	{ ai_charge, 0, widow_reattack_blaster }, // 50 degrees right
	{ ai_charge, 0, widow_reattack_blaster },
	{ ai_charge, 0, widow_reattack_blaster },
	{ ai_charge, 0, widow_reattack_blaster },
	{ ai_charge, 0, widow_reattack_blaster },
	{ ai_charge, 0, widow_reattack_blaster }, // straight
	{ ai_charge, 0, widow_reattack_blaster },
	{ ai_charge, 0, widow_reattack_blaster },
	{ ai_charge, 0, widow_reattack_blaster },
	{ ai_charge, 0, widow_reattack_blaster },
	{ ai_charge, 0, widow_reattack_blaster }, // 50 degrees left
	{ ai_charge, 0, widow_reattack_blaster },
	{ ai_charge, 0, widow_reattack_blaster } // 70 degrees left
};
MMOVE_T(widow_move_attack_blaster) = { FRAME_fired02a, FRAME_fired20, widow_frames_attack_blaster, nullptr };

mframe_t widow_frames_attack_post_blaster[] = {
	{ ai_charge },
	{ ai_charge }
};
MMOVE_T(widow_move_attack_post_blaster) = { FRAME_fired21, FRAME_fired22, widow_frames_attack_post_blaster, widow_run };

mframe_t widow_frames_attack_post_blaster_r[] = {
	{ ai_charge, -2 },
	{ ai_charge, -10 },
	{ ai_charge, -2 },
	{ ai_charge },
	{ ai_charge, 0, widow_start_run_12 }
};
MMOVE_T(widow_move_attack_post_blaster_r) = { FRAME_transa01, FRAME_transa05, widow_frames_attack_post_blaster_r, nullptr };

mframe_t widow_frames_attack_post_blaster_l[] = {
	{ ai_charge },
	{ ai_charge, 14 },
	{ ai_charge, -2 },
	{ ai_charge, 10 },
	{ ai_charge, 10, widow_start_run_12 }
};
MMOVE_T(widow_move_attack_post_blaster_l) = { FRAME_transb01, FRAME_transb05, widow_frames_attack_post_blaster_l, nullptr };

extern const mmove_t widow_move_attack_rail;
extern const mmove_t widow_move_attack_rail_l;
extern const mmove_t widow_move_attack_rail_r;

void WidowRail(edict_t *self)
{
	vec3_t					 start;
	vec3_t					 dir;
	vec3_t					 forward, right;
	monster_muzzleflash_id_t flash;

	AngleVectors(self->s.angles, forward, right, nullptr);

	if (self->monsterinfo.active_move == &widow_move_attack_rail_l)
	{
		flash = MZ2_WIDOW_RAIL_LEFT;
	}
	else if (self->monsterinfo.active_move == &widow_move_attack_rail_r)
	{
		flash = MZ2_WIDOW_RAIL_RIGHT;
	}
	else
		flash = MZ2_WIDOW_RAIL;

	start = G_ProjectSource(self->s.origin, monster_flash_offset[flash], forward, right);

	// calc direction to where we targeted
	dir = self->pos1 - start;
	dir.normalize();

	monster_fire_railgun(self, start, dir, WIDOW_RAIL_DAMAGE * widow_damage_multiplier, 100, flash);
	self->timestamp = level.time + RAIL_TIME;
}

void WidowSaveLoc(edict_t *self)
{
	self->pos1 = self->enemy->s.origin; // save for aiming the shot
	self->pos1[2] += self->enemy->viewheight;
};

void widow_start_rail(edict_t *self)
{
	self->monsterinfo.aiflags |= AI_MANUAL_STEERING;
}

void widow_rail_done(edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
}

mframe_t widow_frames_attack_pre_rail[] = {
	{ ai_charge, 0, widow_start_rail },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, widow_attack_rail }
};
MMOVE_T(widow_move_attack_pre_rail) = { FRAME_transc01, FRAME_transc04, widow_frames_attack_pre_rail, nullptr };

mframe_t widow_frames_attack_rail[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, WidowSaveLoc },
	{ ai_charge, -10, WidowRail },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, widow_rail_done }
};
MMOVE_T(widow_move_attack_rail) = { FRAME_firea01, FRAME_firea09, widow_frames_attack_rail, widow_run };

mframe_t widow_frames_attack_rail_r[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, WidowSaveLoc },
	{ ai_charge, -10, WidowRail },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, widow_rail_done }
};
MMOVE_T(widow_move_attack_rail_r) = { FRAME_fireb01, FRAME_fireb09, widow_frames_attack_rail_r, widow_run };

mframe_t widow_frames_attack_rail_l[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, WidowSaveLoc },
	{ ai_charge, -10, WidowRail },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, widow_rail_done }
};
MMOVE_T(widow_move_attack_rail_l) = { FRAME_firec01, FRAME_firec09, widow_frames_attack_rail_l, widow_run };

void widow_attack_rail(edict_t *self)
{
	float enemy_angle;

	enemy_angle = target_angle(self);

	if (enemy_angle < -15)
		M_SetAnimation(self, &widow_move_attack_rail_l);
	else if (enemy_angle > 15)
		M_SetAnimation(self, &widow_move_attack_rail_r);
	else
		M_SetAnimation(self, &widow_move_attack_rail);
}

void widow_start_spawn(edict_t *self)
{
	self->monsterinfo.aiflags |= AI_MANUAL_STEERING;
}

void widow_done_spawn(edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
}

mframe_t widow_frames_spawn[] = {
	{ ai_charge }, // 1
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, widow_start_spawn },
	{ ai_charge },						 // 5
	{ ai_charge, 0, WidowBlaster },		 // 6
	{ ai_charge, 0, widow_ready_spawn }, // 7
	{ ai_charge, 0, WidowBlaster },
	{ ai_charge, 0, WidowBlaster }, // 9
	{ ai_charge, 0, widow_spawn_check },
	{ ai_charge, 0, WidowBlaster }, // 11
	{ ai_charge, 0, WidowBlaster },
	{ ai_charge, 0, WidowBlaster }, // 13
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, widow_done_spawn }
};
MMOVE_T(widow_move_spawn) = { FRAME_spawn01, FRAME_spawn18, widow_frames_spawn, widow_run };

mframe_t widow_frames_pain_heavy[] = {
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
MMOVE_T(widow_move_pain_heavy) = { FRAME_pain01, FRAME_pain13, widow_frames_pain_heavy, widow_run };

mframe_t widow_frames_pain_light[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(widow_move_pain_light) = { FRAME_pain201, FRAME_pain203, widow_frames_pain_light, widow_run };

void spawn_out_start(edict_t *self)
{
	vec3_t startpoint, f, r, u;

	//	gi.sound (self, CHAN_VOICE, sound_death, 1, ATTN_NONE, 0);
	AngleVectors(self->s.angles, f, r, u);

	startpoint = G_ProjectSource2(self->s.origin, beameffects[0], f, r, u);
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_WIDOWBEAMOUT);
	gi.WriteShort(20001);
	gi.WritePosition(startpoint);
	gi.multicast(startpoint, MULTICAST_ALL, false);

	startpoint = G_ProjectSource2(self->s.origin, beameffects[1], f, r, u);
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_WIDOWBEAMOUT);
	gi.WriteShort(20002);
	gi.WritePosition(startpoint);
	gi.multicast(startpoint, MULTICAST_ALL, false);

	gi.sound(self, CHAN_VOICE, gi.soundindex("misc/bwidowbeamout.wav"), 1, ATTN_NORM, 0);
}

void spawn_out_do(edict_t *self)
{
	vec3_t startpoint, f, r, u;

	AngleVectors(self->s.angles, f, r, u);
	startpoint = G_ProjectSource2(self->s.origin, beameffects[0], f, r, u);
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_WIDOWSPLASH);
	gi.WritePosition(startpoint);
	gi.multicast(startpoint, MULTICAST_ALL, false);

	startpoint = G_ProjectSource2(self->s.origin, beameffects[1], f, r, u);
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_WIDOWSPLASH);
	gi.WritePosition(startpoint);
	gi.multicast(startpoint, MULTICAST_ALL, false);

	startpoint = self->s.origin;
	startpoint[2] += 36;
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_BOSSTPORT);
	gi.WritePosition(startpoint);
	gi.multicast(startpoint, MULTICAST_PHS, false);

	Widowlegs_Spawn(self->s.origin, self->s.angles);

	G_FreeEdict(self);
}

mframe_t widow_frames_death[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }, // 5
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, spawn_out_start }, // 10
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }, // 15
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }, // 20
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }, // 25
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }, // 30
	{ ai_move, 0, spawn_out_do }
};
MMOVE_T(widow_move_death) = { FRAME_death01, FRAME_death31, widow_frames_death, nullptr };

void widow_attack_kick(edict_t *self)
{
	vec3_t aim = { 100, 0, 4 };
	if (self->enemy->groundentity)
		fire_hit(self, aim, irandom(50, 56), 500);
	else // not as much kick if they're in the air .. makes it harder to land on her head
		fire_hit(self, aim, irandom(50, 56), 250);
}

mframe_t widow_frames_attack_kick[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, widow_attack_kick },
	{ ai_move }, // 5
	{ ai_move },
	{ ai_move },
	{ ai_move }
};

MMOVE_T(widow_move_attack_kick) = { FRAME_kick01, FRAME_kick08, widow_frames_attack_kick, widow_run };

MONSTERINFO_STAND(widow_stand) (edict_t *self) -> void
{
	gi.sound(self, CHAN_WEAPON, gi.soundindex("widow/laugh.wav"), 1, ATTN_NORM, 0);
	M_SetAnimation(self, &widow_move_stand);
}

MONSTERINFO_RUN(widow_run) (edict_t *self) -> void
{
	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		M_SetAnimation(self, &widow_move_stand);
	else
		M_SetAnimation(self, &widow_move_run);
}

MONSTERINFO_WALK(widow_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &widow_move_walk);
}

MONSTERINFO_ATTACK(widow_attack) (edict_t *self) -> void
{
	float luck;
	bool  rail_frames = false, blaster_frames = false, blocked = false, anger = false;

	self->movetarget = nullptr;

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
		if ((frandom() < 0.1f) || (level.time < self->timestamp))
			M_SetAnimation(self, &widow_move_attack_pre_blaster);
		else
		{
			gi.sound(self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
			M_SetAnimation(self, &widow_move_attack_pre_rail);
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
	if ((self->monsterinfo.attack_state == AS_BLIND) && (M_SlotsLeft(self) >= 2))
	{
		M_SetAnimation(self, &widow_move_spawn);
		return;
	}

	// accept bias towards spawning regardless of frame
	if (blocked && (M_SlotsLeft(self) >= 2))
	{
		M_SetAnimation(self, &widow_move_spawn);
		return;
	}

	if ((realrange(self, self->enemy) > 300) && (!anger) && (frandom() < 0.5f) && (!blocked))
	{
		M_SetAnimation(self, &widow_move_run_attack);
		return;
	}

	if (blaster_frames)
	{
		if (M_SlotsLeft(self) >= 2)
		{
			M_SetAnimation(self, &widow_move_spawn);
			return;
		}
		else if (self->monsterinfo.fire_wait + BLASTER_TIME <= level.time)
		{
			M_SetAnimation(self, &widow_move_attack_pre_blaster);
			return;
		}
	}

	if (rail_frames)
	{
		if (!(level.time < self->timestamp))
		{
			gi.sound(self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
			M_SetAnimation(self, &widow_move_attack_pre_rail);
		}
	}

	if ((rail_frames) || (blaster_frames))
		return;

	luck = frandom();
	if (M_SlotsLeft(self) >= 2)
	{
		if ((luck <= 0.40f) && (self->monsterinfo.fire_wait + BLASTER_TIME <= level.time))
			M_SetAnimation(self, &widow_move_attack_pre_blaster);
		else if ((luck <= 0.7f) && !(level.time < self->timestamp))
		{
			gi.sound(self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
			M_SetAnimation(self, &widow_move_attack_pre_rail);
		}
		else
			M_SetAnimation(self, &widow_move_spawn);
	}
	else
	{
		if (level.time < self->timestamp)
			M_SetAnimation(self, &widow_move_attack_pre_blaster);
		else if ((luck <= 0.50f) || (level.time + BLASTER_TIME >= self->monsterinfo.fire_wait))
		{
			gi.sound(self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
			M_SetAnimation(self, &widow_move_attack_pre_rail);
		}
		else // holdout to blaster
			M_SetAnimation(self, &widow_move_attack_pre_blaster);
	}
}

void widow_attack_blaster(edict_t *self)
{
	self->monsterinfo.fire_wait = level.time + random_time(1_sec, 3_sec);
	M_SetAnimation(self, &widow_move_attack_blaster);
	self->monsterinfo.nextframe = WidowTorso(self);
}

void widow_reattack_blaster(edict_t *self)
{
	WidowBlaster(self);

	// if WidowBlaster bailed us out of the frames, just bail
	if ((self->monsterinfo.active_move == &widow_move_attack_post_blaster_l) ||
		(self->monsterinfo.active_move == &widow_move_attack_post_blaster_r))
		return;

	// if we're not done with the attack, don't leave the sequence
	if (self->monsterinfo.fire_wait >= level.time)
		return;

	self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;

	M_SetAnimation(self, &widow_move_attack_post_blaster);
}

PAIN(widow_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 5_sec;

	if (damage < 15)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NONE, 0);
	else if (damage < 75)
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NONE, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain3, 1, ATTN_NONE, 0);

	if (!M_ShouldReactToPain(self, mod))
		return; // no pain anims in nightmare

	self->monsterinfo.fire_wait = 0_ms;

	if (damage >= 15)
	{
		if (damage < 75)
		{
			if ((skill->integer < 3) && (frandom() < (0.6f - (0.2f * skill->integer))))
			{
				M_SetAnimation(self, &widow_move_pain_light);
				self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
			}
		}
		else
		{
			if ((skill->integer < 3) && (frandom() < (0.75f - (0.1f * skill->integer))))
			{
				M_SetAnimation(self, &widow_move_pain_heavy);
				self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
			}
		}
	}
}

MONSTERINFO_SETSKIN(widow_setskin) (edict_t *self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

void widow_dead(edict_t *self)
{
	self->mins = { -56, -56, 0 };
	self->maxs = { 56, 56, 80 };
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0_ms;
	gi.linkentity(self);
}

DIE(widow_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	self->deadflag = true;
	self->takedamage = false;
	self->count = 0;
	self->monsterinfo.quad_time = 0_ms;
	self->monsterinfo.double_time = 0_ms;
	self->monsterinfo.invincible_time = 0_ms;
	M_SetAnimation(self, &widow_move_death);
}

MONSTERINFO_MELEE(widow_melee) (edict_t *self) -> void
{
	//	monster_done_dodge (self);
	M_SetAnimation(self, &widow_move_attack_kick);
}

void WidowGoinQuad(edict_t *self, gtime_t time)
{
	self->monsterinfo.quad_time = time;
	widow_damage_multiplier = 4;
}

void WidowDouble(edict_t *self, gtime_t time)
{
	self->monsterinfo.double_time = time;
	widow_damage_multiplier = 2;
}

void WidowPent(edict_t *self, gtime_t time)
{
	self->monsterinfo.invincible_time = time;
}

void WidowPowerArmor(edict_t *self)
{
	self->monsterinfo.power_armor_type = IT_ITEM_POWER_SHIELD;
	// I don't like this, but it works
	if (self->monsterinfo.power_armor_power <= 0)
		self->monsterinfo.power_armor_power += 250 * skill->integer;
}

void WidowRespondPowerup(edict_t *self, edict_t *other)
{
	if (other->s.effects & EF_QUAD)
	{
		if (skill->integer == 1)
			WidowDouble(self, other->client->quad_time);
		else if (skill->integer == 2)
			WidowGoinQuad(self, other->client->quad_time);
		else if (skill->integer == 3)
		{
			WidowGoinQuad(self, other->client->quad_time);
			WidowPowerArmor(self);
		}
	}
	else if (other->s.effects & EF_DOUBLE)
	{
		if (skill->integer == 2)
			WidowDouble(self, other->client->double_time);
		else if (skill->integer == 3)
		{
			WidowDouble(self, other->client->double_time);
			WidowPowerArmor(self);
		}
	}
	else
		widow_damage_multiplier = 1;

	if (other->s.effects & EF_PENT)
	{
		if (skill->integer == 1)
			WidowPowerArmor(self);
		else if (skill->integer == 2)
			WidowPent(self, other->client->invincible_time);
		else if (skill->integer == 3)
		{
			WidowPent(self, other->client->invincible_time);
			WidowPowerArmor(self);
		}
	}
}

void WidowPowerups(edict_t *self)
{
	edict_t *ent;

	if (!coop->integer)
	{
		WidowRespondPowerup(self, self->enemy);
	}
	else
	{
		// in coop, check for pents, then quads, then doubles
		for (uint32_t player = 1; player <= game.maxclients; player++)
		{
			ent = &g_edicts[player];
			if (!ent->inuse)
				continue;
			if (!ent->client)
				continue;
			if (ent->s.effects & EF_PENT)
			{
				WidowRespondPowerup(self, ent);
				return;
			}
		}

		for (uint32_t player = 1; player <= game.maxclients; player++)
		{
			ent = &g_edicts[player];
			if (!ent->inuse)
				continue;
			if (!ent->client)
				continue;
			if (ent->s.effects & EF_QUAD)
			{
				WidowRespondPowerup(self, ent);
				return;
			}
		}

		for (uint32_t player = 1; player <= game.maxclients; player++)
		{
			ent = &g_edicts[player];
			if (!ent->inuse)
				continue;
			if (!ent->client)
				continue;
			if (ent->s.effects & EF_DOUBLE)
			{
				WidowRespondPowerup(self, ent);
				return;
			}
		}
	}
}

MONSTERINFO_CHECKATTACK(Widow_CheckAttack) (edict_t *self) -> bool
{
	if (!self->enemy)
		return false;

	WidowPowerups(self);

	if (self->monsterinfo.active_move == &widow_move_run)
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
			return false;
		default:
			break;
		}
	}

	// give a LARGE bias to spawning things when we have room
	// use AI_BLOCKED as a signal to attack to spawn
	if ((frandom() < 0.8f) && (M_SlotsLeft(self) >= 2) && (realrange(self, self->enemy) > 150))
	{
		self->monsterinfo.aiflags |= AI_BLOCKED;
		self->monsterinfo.attack_state = AS_MISSILE;
		return true;
	}

	return M_CheckAttack_Base(self, 0.4f, 0.8f, 0.7f, 0.6f, 0.5f, 0.f);
}

MONSTERINFO_BLOCKED(widow_blocked) (edict_t *self, float dist) -> bool
{
	// if we get blocked while we're in our run/attack mode, turn on a meaningless (in this context)AI flag,
	// and call attack to get a new attack sequence.  make sure to turn it off when we're done.
	//
	// I'm using AI_TARGET_ANGER for this purpose

	if (self->monsterinfo.active_move == &widow_move_run_attack)
	{
		self->monsterinfo.aiflags |= AI_TARGET_ANGER;
		if (self->monsterinfo.checkattack(self))
			self->monsterinfo.attack(self);
		else
			self->monsterinfo.run(self);
		return true;
	}

	return false;
}

void WidowCalcSlots(edict_t *self)
{
	switch (skill->integer)
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
	if (coop->integer)
	{
		self->monsterinfo.monster_slots = min(6, self->monsterinfo.monster_slots + (skill->integer * (CountPlayers() - 1)));
	}
}

void WidowPrecache()
{
	// cache in all of the stalker stuff, widow stuff, spawngro stuff, gibs
	gi.soundindex("stalker/pain.wav");
	gi.soundindex("stalker/death.wav");
	gi.soundindex("stalker/sight.wav");
	gi.soundindex("stalker/melee1.wav");
	gi.soundindex("stalker/melee2.wav");
	gi.soundindex("stalker/idle.wav");

	gi.soundindex("tank/tnkatck3.wav");
	gi.modelindex("models/objects/laser/tris.md2");

	gi.modelindex("models/monsters/stalker/tris.md2");
	gi.modelindex("models/items/spawngro3/tris.md2");
	gi.modelindex("models/objects/gibs/sm_metal/tris.md2");
	gi.modelindex("models/objects/gibs/gear/tris.md2");
	gi.modelindex("models/monsters/blackwidow/gib1/tris.md2");
	gi.modelindex("models/monsters/blackwidow/gib2/tris.md2");
	gi.modelindex("models/monsters/blackwidow/gib3/tris.md2");
	gi.modelindex("models/monsters/blackwidow/gib4/tris.md2");
	gi.modelindex("models/monsters/blackwidow2/gib1/tris.md2");
	gi.modelindex("models/monsters/blackwidow2/gib2/tris.md2");
	gi.modelindex("models/monsters/blackwidow2/gib3/tris.md2");
	gi.modelindex("models/monsters/blackwidow2/gib4/tris.md2");
	gi.modelindex("models/monsters/legs/tris.md2");
	gi.soundindex("misc/bwidowbeamout.wav");

	gi.soundindex("misc/bigtele.wav");
	gi.soundindex("widow/bwstep3.wav");
	gi.soundindex("widow/bwstep2.wav");
	gi.soundindex("widow/bwstep1.wav");
}

/*QUAKED monster_widow (1 .5 0) (-40 -40 0) (40 40 144) Ambush Trigger_Spawn Sight
 */
void SP_monster_widow(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	sound_pain1.assign("widow/bw1pain1.wav");
	sound_pain2.assign("widow/bw1pain2.wav");
	sound_pain3.assign("widow/bw1pain3.wav");
	sound_rail.assign("gladiator/railgun.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/blackwidow/tris.md2");
	self->mins = { -40, -40, 0 };
	self->maxs = { 40, 40, 144 };

	self->health = (2000 + 1000 * skill->integer) * st.health_multiplier;
	if (coop->integer)
		self->health += 500 * skill->integer;
	self->gib_health = -5000;
	self->mass = 1500;

	if (skill->integer == 3)
	{
		if (!st.was_key_specified("power_armor_type"))
			self->monsterinfo.power_armor_type = IT_ITEM_POWER_SHIELD;
		if (!st.was_key_specified("power_armor_power"))
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
	self->monsterinfo.setskin = widow_setskin;
	self->monsterinfo.blocked = widow_blocked;

	gi.linkentity(self);

	M_SetAnimation(self, &widow_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	WidowPrecache();
	WidowCalcSlots(self);
	widow_damage_multiplier = 1;

	walkmonster_start(self);
}