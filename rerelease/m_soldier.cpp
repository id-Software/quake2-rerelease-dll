// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

SOLDIER

==============================================================================
*/

#include "g_local.h"
#include "m_soldier.h"
#include "m_flash.h"

static cached_soundindex sound_idle;
static cached_soundindex sound_sight1;
static cached_soundindex sound_sight2;
static cached_soundindex sound_pain_light;
static cached_soundindex sound_pain;
static cached_soundindex sound_pain_ss;
static cached_soundindex sound_death_light;
static cached_soundindex sound_death;
static cached_soundindex sound_death_ss;
static cached_soundindex sound_cock;

void soldier_start_charge(edict_t *self)
{
	self->monsterinfo.aiflags |= AI_CHARGING;
}

void soldier_stop_charge(edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_CHARGING;
}

void soldier_idle(edict_t *self)
{
	if (frandom() > 0.8f)
		gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

void soldier_cock(edict_t *self)
{
	if (self->s.frame == FRAME_stand322)
		gi.sound(self, CHAN_WEAPON, sound_cock, 1, ATTN_IDLE, 0);
	else
		gi.sound(self, CHAN_WEAPON, sound_cock, 1, ATTN_NORM, 0);

	// [Paril-KEX] reset cockness
	self->dmg = 0;
}

// RAFAEL
void soldierh_hyper_laser_sound_start(edict_t *self)
{
	if (self->style == 1)
	{
		if (self->count >= 2 && self->count < 4)
			self->monsterinfo.weapon_sound = gi.soundindex("weapons/hyprbl1a.wav");
	}
}

void soldierh_hyper_laser_sound_end(edict_t *self)
{
	if (self->monsterinfo.weapon_sound)
	{
		if (self->count >= 2 && self->count < 4)
			gi.sound(self, CHAN_AUTO, gi.soundindex("weapons/hyprbd1a.wav"), 1, ATTN_NORM, 0);

		self->monsterinfo.weapon_sound = 0;
	}
}
// RAFAEL

// STAND

void soldier_stand(edict_t *self);

mframe_t soldier_frames_stand1[] = {
	{ ai_stand, 0, soldier_idle },
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
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }
};
MMOVE_T(soldier_move_stand1) = { FRAME_stand101, FRAME_stand130, soldier_frames_stand1, soldier_stand };

mframe_t soldier_frames_stand2[] = {
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
	{ ai_stand },
	{ ai_stand, 0, monster_footstep },
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
	{ ai_stand },
	{ ai_stand, 0, monster_footstep }
};
MMOVE_T(soldier_move_stand2) = { FRAME_stand201, FRAME_stand240, soldier_frames_stand2, soldier_stand };

mframe_t soldier_frames_stand3[] = {
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
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },

	{ ai_stand },
	{ ai_stand, 0, soldier_cock },
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
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }
};
MMOVE_T(soldier_move_stand3) = { FRAME_stand301, FRAME_stand339, soldier_frames_stand3, soldier_stand };

MONSTERINFO_STAND(soldier_stand) (edict_t *self) -> void
{
	float r = frandom();

	if ((self->monsterinfo.active_move != &soldier_move_stand1) || (r < 0.6f))
		M_SetAnimation(self, &soldier_move_stand1);
	else if (r < 0.8f)
		M_SetAnimation(self, &soldier_move_stand2);
	else
		M_SetAnimation(self, &soldier_move_stand3);
	soldierh_hyper_laser_sound_end(self);
}

//
// WALK
//

void soldier_walk1_random(edict_t *self)
{
	if (frandom() > 0.1f)
		self->monsterinfo.nextframe = FRAME_walk101;
}

mframe_t soldier_frames_walk1[] = {
	{ ai_walk, 3 },
	{ ai_walk, 6 },
	{ ai_walk, 2 },
	{ ai_walk, 2, monster_footstep },
	{ ai_walk, 2 },
	{ ai_walk, 1 },
	{ ai_walk, 6 },
	{ ai_walk, 5 },
	{ ai_walk, 3, monster_footstep },
	{ ai_walk, -1, soldier_walk1_random },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk },
	{ ai_walk }
};
MMOVE_T(soldier_move_walk1) = { FRAME_walk101, FRAME_walk133, soldier_frames_walk1, nullptr };

mframe_t soldier_frames_walk2[] = {
	{ ai_walk, 4, monster_footstep },
	{ ai_walk, 4 },
	{ ai_walk, 9 },
	{ ai_walk, 8 },
	{ ai_walk, 5 },
	{ ai_walk, 1, monster_footstep },
	{ ai_walk, 3 },
	{ ai_walk, 7 },
	{ ai_walk, 6 },
	{ ai_walk, 7 }
};
MMOVE_T(soldier_move_walk2) = { FRAME_walk209, FRAME_walk218, soldier_frames_walk2, nullptr };

MONSTERINFO_WALK(soldier_walk) (edict_t *self) -> void
{
	// [Paril-KEX] during N64 cutscene, always use fast walk or we bog down the line
	if (!(self->hackflags & HACKFLAG_END_CUTSCENE) && frandom() < 0.5f)
		M_SetAnimation(self, &soldier_move_walk1);
	else
		M_SetAnimation(self, &soldier_move_walk2);
}

//
// RUN
//

void soldier_run(edict_t *self);

mframe_t soldier_frames_start_run[] = {
	{ ai_run, 7 },
	{ ai_run, 5 }
};
MMOVE_T(soldier_move_start_run) = { FRAME_run01, FRAME_run02, soldier_frames_start_run, soldier_run };

mframe_t soldier_frames_run[] = {
	{ ai_run, 10 },
	{ ai_run, 11, [](edict_t *self) { monster_done_dodge(self); monster_footstep(self); } },
	{ ai_run, 11 },
	{ ai_run, 16 },
	{ ai_run, 10, monster_footstep },
	{ ai_run, 15, monster_done_dodge }
};
MMOVE_T(soldier_move_run) = { FRAME_run03, FRAME_run08, soldier_frames_run, nullptr };

MONSTERINFO_RUN(soldier_run) (edict_t *self) -> void
{
	monster_done_dodge(self);
	soldierh_hyper_laser_sound_end(self);

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		M_SetAnimation(self, &soldier_move_stand1);
		return;
	}

	if (self->monsterinfo.active_move == &soldier_move_walk1 ||
		self->monsterinfo.active_move == &soldier_move_walk2 ||
		self->monsterinfo.active_move == &soldier_move_start_run ||
		self->monsterinfo.active_move == &soldier_move_run)
	{
		M_SetAnimation(self, &soldier_move_run);
	}
	else
	{
		M_SetAnimation(self, &soldier_move_start_run);
	}
}

//
// PAIN
//

mframe_t soldier_frames_pain1[] = {
	{ ai_move, -3 },
	{ ai_move, 4 },
	{ ai_move, 1 },
	{ ai_move, 1 },
	{ ai_move }
};
MMOVE_T(soldier_move_pain1) = { FRAME_pain101, FRAME_pain105, soldier_frames_pain1, soldier_run };

mframe_t soldier_frames_pain2[] = {
	{ ai_move, -13 },
	{ ai_move, -1 },
	{ ai_move, 2 },
	{ ai_move, 4 },
	{ ai_move, 2 },
	{ ai_move, 3 },
	{ ai_move, 2 }
};
MMOVE_T(soldier_move_pain2) = { FRAME_pain201, FRAME_pain207, soldier_frames_pain2, soldier_run };

mframe_t soldier_frames_pain3[] = {
	{ ai_move, -8 },
	{ ai_move, 10 },
	{ ai_move, -4, monster_footstep },
	{ ai_move, -1 },
	{ ai_move, -3 },
	{ ai_move },
	{ ai_move, 3 },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 1 },
	{ ai_move },
	{ ai_move, 1 },
	{ ai_move, 2 },
	{ ai_move, 4 },
	{ ai_move, 3 },
	{ ai_move, 2, monster_footstep }
};
MMOVE_T(soldier_move_pain3) = { FRAME_pain301, FRAME_pain318, soldier_frames_pain3, soldier_run };

mframe_t soldier_frames_pain4[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, -10 },
	{ ai_move, -6 },
	{ ai_move, 8 },
	{ ai_move, 4 },
	{ ai_move, 1 },
	{ ai_move },
	{ ai_move, 2 },
	{ ai_move, 5 },
	{ ai_move, 2 },
	{ ai_move, -1 },
	{ ai_move, -1 },
	{ ai_move, 3 },
	{ ai_move, 2 },
	{ ai_move }
};
MMOVE_T(soldier_move_pain4) = { FRAME_pain401, FRAME_pain417, soldier_frames_pain4, soldier_run };

PAIN(soldier_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	float r;
	int	  n;

	monster_done_dodge(self);
	soldier_stop_charge(self);

	// if we're blind firing, this needs to be turned off here
	self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;

	if (level.time < self->pain_debounce_time)
	{
		if ((self->velocity[2] > 100) && ((self->monsterinfo.active_move == &soldier_move_pain1) || (self->monsterinfo.active_move == &soldier_move_pain2) || (self->monsterinfo.active_move == &soldier_move_pain3)))
		{
			// PMM - clear duck flag
			if (self->monsterinfo.aiflags & AI_DUCKED)
				monster_duck_up(self);
			M_SetAnimation(self, &soldier_move_pain4);
			soldierh_hyper_laser_sound_end(self);
		}
		return;
	}

	self->pain_debounce_time = level.time + 3_sec;

	n = self->count | 1;
	if (n == 1)
		gi.sound(self, CHAN_VOICE, sound_pain_light, 1, ATTN_NORM, 0);
	else if (n == 3)
		gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain_ss, 1, ATTN_NORM, 0);

	if (self->velocity[2] > 100)
	{
		// PMM - clear duck flag
		if (self->monsterinfo.aiflags & AI_DUCKED)
			monster_duck_up(self);
		M_SetAnimation(self, &soldier_move_pain4);
		soldierh_hyper_laser_sound_end(self);
		return;
	}
	
	if (!M_ShouldReactToPain(self, mod))
		return; // no pain anims in nightmare

	r = frandom();

	if (r < 0.33f)
		M_SetAnimation(self, &soldier_move_pain1);
	else if (r < 0.66f)
		M_SetAnimation(self, &soldier_move_pain2);
	else
		M_SetAnimation(self, &soldier_move_pain3);

	// PMM - clear duck flag
	if (self->monsterinfo.aiflags & AI_DUCKED)
		monster_duck_up(self);
	soldierh_hyper_laser_sound_end(self);
}

MONSTERINFO_SETSKIN(soldier_setskin) (edict_t *self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;
	else
		self->s.skinnum &= ~1;
}

//
// ATTACK
//

constexpr monster_muzzleflash_id_t blaster_flash[] = { MZ2_SOLDIER_BLASTER_1, MZ2_SOLDIER_BLASTER_2, MZ2_SOLDIER_BLASTER_3, MZ2_SOLDIER_BLASTER_4, MZ2_SOLDIER_BLASTER_5, MZ2_SOLDIER_BLASTER_6, MZ2_SOLDIER_BLASTER_7, MZ2_SOLDIER_BLASTER_8, MZ2_SOLDIER_BLASTER_9 };
constexpr monster_muzzleflash_id_t shotgun_flash[] = { MZ2_SOLDIER_SHOTGUN_1, MZ2_SOLDIER_SHOTGUN_2, MZ2_SOLDIER_SHOTGUN_3, MZ2_SOLDIER_SHOTGUN_4, MZ2_SOLDIER_SHOTGUN_5, MZ2_SOLDIER_SHOTGUN_6, MZ2_SOLDIER_SHOTGUN_7, MZ2_SOLDIER_SHOTGUN_8, MZ2_SOLDIER_SHOTGUN_9 };
constexpr monster_muzzleflash_id_t machinegun_flash[] = { MZ2_SOLDIER_MACHINEGUN_1, MZ2_SOLDIER_MACHINEGUN_2, MZ2_SOLDIER_MACHINEGUN_3, MZ2_SOLDIER_MACHINEGUN_4, MZ2_SOLDIER_MACHINEGUN_5, MZ2_SOLDIER_MACHINEGUN_6, MZ2_SOLDIER_MACHINEGUN_7, MZ2_SOLDIER_MACHINEGUN_8, MZ2_SOLDIER_MACHINEGUN_9 };

void soldier_fire_vanilla(edict_t *self, int flash_number, bool angle_limited)
{
	vec3_t					 start;
	vec3_t					 forward, right, up;
	vec3_t					 aim;
	vec3_t					 dir;
	vec3_t					 end;
	float					 r, u;
	monster_muzzleflash_id_t flash_index;
	vec3_t					 aim_norm;
	float					 angle;
	vec3_t					 aim_good;

	if (self->count < 2)
		flash_index = blaster_flash[flash_number];
	else if (self->count < 4)
		flash_index = shotgun_flash[flash_number];
	else
		flash_index = machinegun_flash[flash_number];

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, monster_flash_offset[flash_index], forward, right);

	if (flash_number == 5 || flash_number == 6) // he's dead
	{
		if (self->spawnflags.has(SPAWNFLAG_MONSTER_DEAD))
			return;

		aim = forward;
	}
	else
	{
		if ((!self->enemy) || (!self->enemy->inuse))
		{
			self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
			return;
		}

		// PMM
		if (self->monsterinfo.attack_state == AS_BLIND)
			end = self->monsterinfo.blind_fire_target;
		else
			end = self->enemy->s.origin;
		// pmm
		end[2] += self->enemy->viewheight;
		aim = end - start;
		aim_good = end;
		// PMM
		if (angle_limited)
		{
			aim_norm = aim;
			aim_norm.normalize();
			angle = aim_norm.dot(forward);
			if (angle < 0.5f) // ~25 degree angle
			{
				if (level.time >= self->monsterinfo.fire_wait)
					self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
				else
					self->monsterinfo.aiflags |= AI_HOLD_FRAME;

				return;
			}
		}
		//-PMM
		dir = vectoangles(aim);
		AngleVectors(dir, forward, right, up);

		r = crandom() * 1000;
		u = crandom() * 500;

		end = start + (forward * 8192);
		end += (right * r);
		end += (up * u);

		aim = end - start;
		aim.normalize();
	}

	if (self->count <= 1)
	{
		monster_fire_blaster(self, start, aim, 5, 600, flash_index, EF_BLASTER);
	}
	else if (self->count <= 3)
	{
		monster_fire_shotgun(self, start, aim, 2, 1, 1500, 750, 9, flash_index);
		// [Paril-KEX] indicates to soldier that he must cock
		self->dmg = 1;
	}
	else
	{
		// PMM - changed to wait from pausetime to not interfere with dodge code
		if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
			self->monsterinfo.fire_wait = level.time + random_time(300_ms, 1.1_sec);

		monster_fire_bullet(self, start, aim, 2, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_index);

		if (level.time >= self->monsterinfo.fire_wait)
			self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		else
			self->monsterinfo.aiflags |= AI_HOLD_FRAME;
	}
}

PRETHINK(soldierh_laser_update) (edict_t *laser) -> void
{
	edict_t *self = laser->owner;

	vec3_t forward, right, up;
	vec3_t start;
	vec3_t tempvec;

	AngleVectors(self->s.angles, forward, right, up);
	start = self->s.origin;
	tempvec = monster_flash_offset[self->radius_dmg];
	start += (forward * tempvec[0]);
	start += (right * tempvec[1]);
	start += (up * (tempvec[2] + 6));

	if (!self->deadflag)
		PredictAim(self, self->enemy, start, 0, false, frandom(0.1f, 0.2f), &forward, nullptr);
	
	laser->s.origin = start;
	laser->movedir = forward;
	gi.linkentity(laser);
	dabeam_update(laser, false);
}

// RAFAEL
void soldierh_laserbeam(edict_t *self, int flash_index)
{
	self->radius_dmg = flash_index;
	monster_fire_dabeam(self, 1, false, soldierh_laser_update);
}

constexpr monster_muzzleflash_id_t ripper_flash[] = { MZ2_SOLDIER_RIPPER_1, MZ2_SOLDIER_RIPPER_2, MZ2_SOLDIER_RIPPER_3, MZ2_SOLDIER_RIPPER_4, MZ2_SOLDIER_RIPPER_5, MZ2_SOLDIER_RIPPER_6, MZ2_SOLDIER_RIPPER_7, MZ2_SOLDIER_RIPPER_8, MZ2_SOLDIER_RIPPER_9 };
constexpr monster_muzzleflash_id_t hyper_flash[] = { MZ2_SOLDIER_HYPERGUN_1, MZ2_SOLDIER_HYPERGUN_2, MZ2_SOLDIER_HYPERGUN_3, MZ2_SOLDIER_HYPERGUN_4, MZ2_SOLDIER_HYPERGUN_5, MZ2_SOLDIER_HYPERGUN_6, MZ2_SOLDIER_HYPERGUN_7, MZ2_SOLDIER_HYPERGUN_8, MZ2_SOLDIER_HYPERGUN_9 };

void soldier_fire_xatrix(edict_t *self, int flash_number, bool angle_limited)
{
	vec3_t					 start;
	vec3_t					 forward, right, up;
	vec3_t					 aim;
	vec3_t					 dir;
	vec3_t					 end;
	float					 r, u;
	monster_muzzleflash_id_t flash_index;
	vec3_t					 aim_norm;
	float					 angle;
	vec3_t					 aim_good;

	if (self->count < 2)
		flash_index = ripper_flash[flash_number]; // ripper
	else if (self->count < 4)
		flash_index = hyper_flash[flash_number]; // hyperblaster
	else
		flash_index = machinegun_flash[flash_number]; // laserbeam

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, monster_flash_offset[flash_index], forward, right);

	if (flash_number == 5 || flash_number == 6)
	{
		if (self->spawnflags.has(SPAWNFLAG_MONSTER_DEAD))
			return;

		aim = forward;
	}
	else
	{
		// [Paril-KEX] no enemy = no fire
		if ((!self->enemy) || (!self->enemy->inuse))
		{
			self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
			return;
		}

		// PMM
		if (self->monsterinfo.attack_state == AS_BLIND)
			end = self->monsterinfo.blind_fire_target;
		else
			end = self->enemy->s.origin;
		// pmm
		end[2] += self->enemy->viewheight;

		aim = end - start;
		aim_good = end;

		// PMM
		if (angle_limited)
		{
			aim_norm = aim;
			aim_norm.normalize();
			angle = aim_norm.dot(forward);
			
			if (angle < 0.5f) // ~25 degree angle
			{
				if (level.time >= self->monsterinfo.fire_wait)
					self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
				else
					self->monsterinfo.aiflags |= AI_HOLD_FRAME;

				return;
			}
		}
		//-PMM

		dir = vectoangles(aim);
		AngleVectors(dir, forward, right, up);

		r = crandom() * 100;
		u = crandom() * 50;
		end = start + (forward * 8192);
		end += (right * r);
		end += (up * u);

		aim = end - start;
		aim.normalize();
	}

	if (self->count <= 1)
	{
		// RAFAEL 24-APR-98
		// droped the damage from 15 to 5
		monster_fire_ionripper(self, start, aim, 5, 600, flash_index, EF_IONRIPPER);
	}
	else if (self->count <= 3)
	{
		monster_fire_blueblaster(self, start, aim, 1, 600, flash_index, EF_BLUEHYPERBLASTER);
	}
	else
	{
		// PMM - changed to wait from pausetime to not interfere with dodge code
		if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
			self->monsterinfo.fire_wait = level.time + random_time(300_ms, 1.1_sec);

		soldierh_laserbeam(self, flash_index);

		if (level.time >= self->monsterinfo.fire_wait)
			self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		else
			self->monsterinfo.aiflags |= AI_HOLD_FRAME;
	}
}
// RAFAEL

void soldier_fire(edict_t *self, int flash_number, bool angle_limited)
{
	// RAFAEL
	if (self->style == 1)
		soldier_fire_xatrix(self, flash_number, angle_limited);
	else
		// RAFAEL
		soldier_fire_vanilla(self, flash_number, angle_limited);
}

// ATTACK1 (blaster/shotgun)

void soldier_fire1(edict_t *self)
{
	soldier_fire(self, 0, false);
}

void soldier_attack1_refire1(edict_t *self)
{
	// [Paril-KEX]
	if (self->count <= 0)
		self->monsterinfo.nextframe = FRAME_attak110;

	// PMM - blindfire
	if (self->monsterinfo.aiflags & AI_MANUAL_STEERING)
	{
		self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
		return;
	}
	// pmm

	if (!self->enemy)
		return;

	if (self->count > 1)
		return;

	if (self->enemy->health <= 0)
		return;

	if (((frandom() < 0.5f) && visible(self, self->enemy)) || (range_to(self, self->enemy) <= RANGE_MELEE))
		self->monsterinfo.nextframe = FRAME_attak102;
	else
		self->monsterinfo.nextframe = FRAME_attak110;
}

void soldier_attack1_refire2(edict_t *self)
{
	if (!self->enemy)
		return;

	if (self->count < 2)
		return;

	if (self->enemy->health <= 0)
		return;

	if (((self->radius_dmg || frandom() < 0.5f) && visible(self, self->enemy)) || (range_to(self, self->enemy) <= RANGE_MELEE))
	{
		self->monsterinfo.nextframe = FRAME_attak102;
		self->radius_dmg = 0;
	}
}

static void soldier_attack1_shotgun_check(edict_t *self)
{
	if (self->dmg)
	{
		self->monsterinfo.nextframe = FRAME_attak106;
		// [Paril-KEX] indicate that we should force a refire
		self->radius_dmg = 1;
	}
}

static void soldier_blind_check(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_MANUAL_STEERING)
	{
		vec3_t aim = self->monsterinfo.blind_fire_target - self->s.origin;
		self->ideal_yaw = vectoyaw(aim);
	}
}

mframe_t soldier_frames_attack1[] = {
	{ ai_charge, 0, soldier_blind_check },
	{ ai_charge, 0, soldier_attack1_shotgun_check },
	{ ai_charge, 0, soldier_fire1 },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, soldier_attack1_refire1 },
	{ ai_charge },
	{ ai_charge, 0, soldier_cock },
	{ ai_charge, 0, soldier_attack1_refire2 },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }
};
MMOVE_T(soldier_move_attack1) = { FRAME_attak101, FRAME_attak112, soldier_frames_attack1, soldier_run };

// ATTACK1 (blaster/shotgun)
void soldierh_hyper_refire1(edict_t *self)
{
	if (!self->enemy)
		return;

	if (self->count >= 2 && self->count < 4)
	{
		if (frandom() < 0.7f && visible(self, self->enemy))
			self->s.frame = FRAME_attak103;
	}
}

void soldierh_hyperripper1(edict_t *self)
{
	if (self->count < 4)
		soldier_fire(self, 0, false);
}

mframe_t soldierh_frames_attack1[] = {
	{ ai_charge, 0, soldier_blind_check },
	{ ai_charge, 0, soldierh_hyper_laser_sound_start },
	{ ai_charge, 0, soldier_fire1 },
	{ ai_charge, 0, soldierh_hyperripper1 },
	{ ai_charge, 0, soldierh_hyperripper1 },
	{ ai_charge, 0, soldier_attack1_refire1 },
	{ ai_charge, 0, soldierh_hyper_refire1 },
	{ ai_charge, 0, soldier_cock },
	{ ai_charge, 0, soldier_attack1_refire2 },
	{ ai_charge, 0, soldierh_hyper_laser_sound_end },
	{ ai_charge },
	{ ai_charge }
};
MMOVE_T(soldierh_move_attack1) = { FRAME_attak101, FRAME_attak112, soldierh_frames_attack1, soldier_run };

// ATTACK2 (blaster/shotgun)

void soldier_fire2(edict_t *self)
{
	soldier_fire(self, 1, false);
}

void soldier_attack2_refire1(edict_t *self)
{
	if (self->count <= 0)
		self->monsterinfo.nextframe = FRAME_attak216;

	if (!self->enemy)
		return;

	if (self->count > 1)
		return;

	if (self->enemy->health <= 0)
		return;

	if (((frandom() < 0.5f) && visible(self, self->enemy)) || (range_to(self, self->enemy) <= RANGE_MELEE))
		self->monsterinfo.nextframe = FRAME_attak204;
}

void soldier_attack2_refire2(edict_t *self)
{
	if (!self->enemy)
		return;

	if (self->count < 2)
		return;

	if (self->enemy->health <= 0)
		return;

	// RAFAEL
	if (((self->radius_dmg || frandom() < 0.5f) && visible(self, self->enemy)) || ((self->style == 0 || self->count < 4) && (range_to(self, self->enemy) <= RANGE_MELEE)))
	{
		// RAFAEL
		self->monsterinfo.nextframe = FRAME_attak204;
		self->radius_dmg = 0;
	}
}

static void soldier_attack2_shotgun_check(edict_t *self)
{
	if (self->dmg)
	{
		self->monsterinfo.nextframe = FRAME_attak210;
		// [Paril-KEX] indicate that we should force a refire
		self->radius_dmg = 1;
	}
}

mframe_t soldier_frames_attack2[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, soldier_attack2_shotgun_check },
	{ ai_charge },
	{ ai_charge, 0, soldier_fire2 },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, soldier_attack2_refire1 },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, soldier_cock },
	{ ai_charge },
	{ ai_charge, 0, soldier_attack2_refire2 },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }
};
MMOVE_T(soldier_move_attack2) = { FRAME_attak201, FRAME_attak218, soldier_frames_attack2, soldier_run };

// RAFAEL
void soldierh_hyper_refire2(edict_t *self)
{
	if (!self->enemy)
		return;

	if (self->count < 2)
		return;
	else if (self->count < 4)
	{
		if (frandom() < 0.7f && visible(self, self->enemy))
			self->s.frame = FRAME_attak205;
	}
}

void soldierh_hyperripper2(edict_t *self)
{
	if (self->count < 4)
		soldier_fire(self, 1, false);
}

mframe_t soldierh_frames_attack2[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, soldierh_hyper_laser_sound_start },
	{ ai_charge, 0, soldier_fire2 },
	{ ai_charge, 0, soldierh_hyperripper2 },
	{ ai_charge, 0, soldierh_hyperripper2 },
	{ ai_charge, 0, soldier_attack2_refire1 },
	{ ai_charge, 0, soldierh_hyper_refire2 },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, soldier_cock },
	{ ai_charge },
	{ ai_charge, 0, soldier_attack2_refire2 },
	{ ai_charge, 0, soldierh_hyper_laser_sound_end },
	{ ai_charge },
	{ ai_charge }
};
MMOVE_T(soldierh_move_attack2) = { FRAME_attak201, FRAME_attak218, soldierh_frames_attack2, soldier_run };
// RAFAEL

// ATTACK3 (duck and shoot)
void soldier_fire3(edict_t *self)
{
	soldier_fire(self, 2, false);
}

void soldierh_hyperripper3(edict_t *self)
{
	if (self->s.skinnum >= 6 && self->count < 4)
		soldier_fire(self, 2, false);
}

void soldier_attack3_refire(edict_t *self)
{
	if (self->dmg)
		monster_duck_hold(self);
	else if ((level.time + 400_ms) < self->monsterinfo.duck_wait_time)
		self->monsterinfo.nextframe = FRAME_attak303;
}

mframe_t soldier_frames_attack3[] = {
	{ ai_charge, 0, monster_duck_down },
	{ ai_charge, 0, soldierh_hyper_laser_sound_start },
	{ ai_charge, 0, soldier_fire3 },
	{ ai_charge, 0, soldierh_hyperripper3 },
	{ ai_charge, 0, soldierh_hyperripper3 },
	{ ai_charge, 0, soldier_attack3_refire },
	{ ai_charge, 0, monster_duck_up },
	{ ai_charge, 0, soldierh_hyper_laser_sound_end },
	{ ai_charge }
};
MMOVE_T(soldier_move_attack3) = { FRAME_attak301, FRAME_attak309, soldier_frames_attack3, soldier_run };

// ATTACK4 (machinegun)

void soldier_fire4(edict_t *self)
{
	soldier_fire(self, 3, false);
}

mframe_t soldier_frames_attack4[] = {
	{ ai_charge },
	{ ai_charge, 0, soldierh_hyper_laser_sound_start },
	{ ai_charge, 0, soldier_fire4 },
	{ ai_charge, 0, soldierh_hyper_laser_sound_end },
	{ ai_charge },
	{ ai_charge }
};
MMOVE_T(soldier_move_attack4) = { FRAME_attak401, FRAME_attak406, soldier_frames_attack4, soldier_run };

// ATTACK6 (run & shoot)

void soldier_fire8(edict_t *self)
{
	soldier_fire(self, 7, true);
}

void soldier_attack6_refire1(edict_t *self)
{
	// PMM - make sure dodge & charge bits are cleared
	monster_done_dodge(self);
	soldier_stop_charge(self);

	if (!self->enemy)
		return;

	if (self->count > 1)
		return;

	if (self->enemy->health <= 0 ||
		range_to(self, self->enemy) < RANGE_NEAR ||
		!visible(self, self->enemy)) // don't endlessly run into walls
	{
		soldier_run(self);
		return;
	}

	if (frandom() < 0.25f)
		self->monsterinfo.nextframe = FRAME_runs03;
	else
		soldier_run(self);
}

void soldier_attack6_refire2(edict_t *self)
{
	// PMM - make sure dodge & charge bits are cleared
	monster_done_dodge(self);
	soldier_stop_charge(self);

	if (!self->enemy || self->count <= 0)
		return;

	if (self->enemy->health <= 0 ||
		(!self->radius_dmg && range_to(self, self->enemy) < RANGE_NEAR) ||
		!visible(self, self->enemy)) // don't endlessly run into walls
	{
		soldierh_hyper_laser_sound_end(self);
		return;
	}

	if (self->radius_dmg || frandom() < 0.25f)
	{
		self->monsterinfo.nextframe = FRAME_runs03;
		self->radius_dmg = 0;
	}
}

static void soldier_attack6_shotgun_check(edict_t *self)
{
	if (self->dmg)
	{
		self->monsterinfo.nextframe = FRAME_runs09;
		// [Paril-KEX] indicate that we should force a refire
		self->radius_dmg = 1;
	}
}

void soldierh_hyperripper8(edict_t *self)
{
	if (self->s.skinnum >= 6 && self->count < 4)
		soldier_fire(self, 7, true);
}

mframe_t soldier_frames_attack6[] = {
	{ ai_run, 10, soldier_start_charge },
	{ ai_run, 4, soldier_attack6_shotgun_check },
	{ ai_run, 12, soldierh_hyper_laser_sound_start },
	{ ai_run, 11, [](edict_t *self) { soldier_fire8(self); monster_footstep(self); } },
	{ ai_run, 13, [](edict_t *self ) { soldierh_hyperripper8(self); monster_done_dodge(self); } },
	{ ai_run, 18, soldierh_hyperripper8 },
	{ ai_run, 15, monster_footstep },
	{ ai_run, 14, soldier_attack6_refire1 },
	{ ai_run, 11 },
	{ ai_run, 8, monster_footstep },
	{ ai_run, 11, soldier_cock },
	{ ai_run, 12 },
	{ ai_run, 12, monster_footstep },
	{ ai_run, 17, soldier_attack6_refire2 }
};
MMOVE_T(soldier_move_attack6) = { FRAME_runs01, FRAME_runs14, soldier_frames_attack6, soldier_run, 0.65f };

MONSTERINFO_ATTACK(soldier_attack) (edict_t *self) -> void
{
	float r, chance;

	monster_done_dodge(self);

	// PMM - blindfire!
	if (self->monsterinfo.attack_state == AS_BLIND)
	{
		// setup shot probabilities
		if (self->monsterinfo.blind_fire_delay < 1_sec)
			chance = 1.0f;
		else if (self->monsterinfo.blind_fire_delay < 7.5_sec)
			chance = 0.4f;
		else
			chance = 0.1f;

		r = frandom();

		// minimum of 4.1 seconds, plus 0-3, after the shots are done
		self->monsterinfo.blind_fire_delay += 4.1_sec + random_time(3_sec);

		// don't shoot at the origin
		if (!self->monsterinfo.blind_fire_target)
			return;

		// don't shoot if the dice say not to
		if (r > chance)
			return;

		// turn on manual steering to signal both manual steering and blindfire
		self->monsterinfo.aiflags |= AI_MANUAL_STEERING;

		// RAFAEL
		if (self->style == 1)
			M_SetAnimation(self, &soldierh_move_attack1);
		else
			// RAFAEL
			M_SetAnimation(self, &soldier_move_attack1);
		self->monsterinfo.attack_finished = level.time + random_time(1.5_sec, 2.5_sec);
		return;
	}
	// pmm

	// PMM - added this so the soldiers now run toward you and shoot instead of just stopping and shooting
	r = frandom();

	// nb: run-shoot not limited by `M_CheckClearShot` since they will be far enough
	// away that it doesn't matter

	if ((!(self->monsterinfo.aiflags & (AI_BLOCKED | AI_STAND_GROUND))) &&
		(r < 0.25f &&
		(self->count <= 3)) &&
		(range_to(self, self->enemy) >= (RANGE_NEAR * 0.5f)))
	{
		M_SetAnimation(self, &soldier_move_attack6);
	}
	else
	{
		if (self->count < 4)
		{
			bool attack1_possible;

			// [Paril-KEX] shotgun guard only uses attack2 at close range
			if ((!self->style && self->count >= 2 && self->count <= 3) && range_to(self, self->enemy) <= (RANGE_NEAR * 0.65f))
				attack1_possible = false;
			else
				attack1_possible = M_CheckClearShot(self, monster_flash_offset[MZ2_SOLDIER_BLASTER_1]);
			
			bool attack2_possible = M_CheckClearShot(self, monster_flash_offset[MZ2_SOLDIER_BLASTER_2]);

			if (attack1_possible && (!attack2_possible || frandom() < 0.5f))
			{
				// RAFAEL
				if (self->style == 1)
					M_SetAnimation(self, &soldierh_move_attack1);
				else
					// RAFAEL
					M_SetAnimation(self, &soldier_move_attack1);
			}
			else if (attack2_possible)
			{
				// RAFAEL
				if (self->style == 1)
					M_SetAnimation(self, &soldierh_move_attack2);
				else
					// RAFAEL
					M_SetAnimation(self, &soldier_move_attack2);
			}
		}
		else if (M_CheckClearShot(self, monster_flash_offset[MZ2_SOLDIER_MACHINEGUN_4]))
		{
			M_SetAnimation(self, &soldier_move_attack4);
		}
	}
}

//
// SIGHT
//

MONSTERINFO_SIGHT(soldier_sight) (edict_t *self, edict_t *other) -> void
{
	if (frandom() < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_sight1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_sight2, 1, ATTN_NORM, 0);

	if (self->enemy && (range_to(self, self->enemy) >= RANGE_NEAR) &&
		visible(self, self->enemy) // Paril: don't run-shoot if we can't see them
	)
	{
		// RAFAEL
		if (self->style == 1 || frandom() > 0.75f)
		// RAFAEL
		{
			// RAFAEL + legacy bug fix
			// don't use run+shoot for machinegun/laser because
			// the animation is a bit weird
			if (self->count < 4)
				M_SetAnimation(self, &soldier_move_attack6);
			else if (M_CheckClearShot(self, monster_flash_offset[MZ2_SOLDIER_MACHINEGUN_4]))
				// RAFAEL
				M_SetAnimation(self, &soldier_move_attack4);
		}
	}
}

//
// DUCK
//
mframe_t soldier_frames_duck[] = {
	{ ai_move, 5, monster_duck_down },
	{ ai_move, -1, monster_duck_hold },
	{ ai_move, 1 },
	{ ai_move, 0, monster_duck_up },
	{ ai_move, 5 }
};
MMOVE_T(soldier_move_duck) = { FRAME_duck01, FRAME_duck05, soldier_frames_duck, soldier_run };

extern const mmove_t soldier_move_trip;

static void soldier_stand_up(edict_t *self)
{
	soldierh_hyper_laser_sound_end(self);
	M_SetAnimation(self, &soldier_move_trip, false);
	self->monsterinfo.nextframe = FRAME_runt08;
}

static bool soldier_prone_shoot_ok(edict_t *self)
{
	if (!self->enemy || !self->enemy->inuse)
		return false;

	vec3_t fwd;
	AngleVectors(self->s.angles, fwd, nullptr, nullptr);

	vec3_t diff = self->enemy->s.origin - self->s.origin;
	diff.z = 0;
	diff.normalize();

	float v = fwd.dot(diff);

	if (v < 0.80f)
		return false;
	
	return true;
}

static void ai_soldier_move(edict_t *self, float dist)
{
	ai_move(self, dist);

	if (!soldier_prone_shoot_ok(self))
	{
		soldier_stand_up(self);
		return;
	}
}

void soldier_fire5(edict_t *self)
{
	soldier_fire(self, 8, true);
}

void soldierh_hyperripper5(edict_t *self)
{
	if (self->style && self->count < 4)
		soldier_fire(self, 8, true);
}

mframe_t soldier_frames_attack5[] = {
	{ ai_move, 18, monster_duck_down },
	{ ai_move, 11, monster_footstep },
	{ ai_move, 0, monster_footstep },
	{ ai_soldier_move },
	{ ai_soldier_move, 0, soldierh_hyper_laser_sound_start },
	{ ai_soldier_move, 0, soldier_fire5 },
	{ ai_soldier_move, 0, soldierh_hyperripper5 },
	{ ai_soldier_move, 0, soldierh_hyperripper5 },
};
MMOVE_T(soldier_move_attack5) = { FRAME_attak501, FRAME_attak508, soldier_frames_attack5, soldier_stand_up };

static void monster_check_prone(edict_t *self)
{
	// we're a shotgun guard waiting to cock
	if (!self->style && self->count >= 2 && self->count <= 3 && self->dmg)
		return;

	// not going to shoot at this angle
	if (!soldier_prone_shoot_ok(self))
		return;

	M_SetAnimation(self, &soldier_move_attack5, false);
}

mframe_t soldier_frames_trip[] = {
	{ ai_move, 10 },
	{ ai_move, 2, monster_check_prone },
	{ ai_move, 18, monster_duck_down },
	{ ai_move, 11, monster_footstep },
	{ ai_move, 9 },
	{ ai_move, -11, monster_footstep },
	{ ai_move, -2 },
	{ ai_move, 0 },
	{ ai_move, 6 },
	{ ai_move, -5 },
	{ ai_move, 0 },
	{ ai_move, 1 },
	{ ai_move, 0, monster_footstep },
	{ ai_move, 0, monster_duck_up },
	{ ai_move, 3 },
	{ ai_move, 2, monster_footstep },
	{ ai_move, -1 },
	{ ai_move, 2 },
	{ ai_move, 0 },
};
MMOVE_T(soldier_move_trip) = { FRAME_runt01, FRAME_runt19, soldier_frames_trip, soldier_run };

// pmm - blocking code

MONSTERINFO_BLOCKED(soldier_blocked) (edict_t *self, float dist) -> bool
{
	// don't do anything if you're dodging
	if ((self->monsterinfo.aiflags & AI_DODGING) || (self->monsterinfo.aiflags & AI_DUCKED))
		return false;

	return blocked_checkplat(self, dist);
}

//
// DEATH
//

void soldier_fire6(edict_t *self)
{
	soldier_fire(self, 5, false);

	if (self->dmg)
		self->monsterinfo.nextframe = FRAME_death126;
}

void soldier_fire7(edict_t *self)
{
	soldier_fire(self, 6, false);
}

void soldier_dead(edict_t *self)
{
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, -8 };
	monster_dead(self);
}

static void soldier_death_shrink(edict_t *self)
{
	self->svflags |= SVF_DEADMONSTER;
	self->maxs[2] = 0;
	gi.linkentity(self);
}

mframe_t soldier_frames_death1[] = {
	{ ai_move },
	{ ai_move, -10 },
	{ ai_move, -10 },
	{ ai_move, -10, soldier_death_shrink },
	{ ai_move, -5 },
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
	{ ai_move },

	{ ai_move, 0, soldierh_hyper_laser_sound_start },
	{ ai_move, 0, soldier_fire6 },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, soldier_fire7 },
	{ ai_move, 0, soldierh_hyper_laser_sound_end },
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
MMOVE_T(soldier_move_death1) = { FRAME_death101, FRAME_death136, soldier_frames_death1, soldier_dead };

mframe_t soldier_frames_death2[] = {
	{ ai_move, -5 },
	{ ai_move, -5 },
	{ ai_move, -5 },
	{ ai_move, 0, soldier_death_shrink },
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
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(soldier_move_death2) = { FRAME_death201, FRAME_death235, soldier_frames_death2, soldier_dead };

mframe_t soldier_frames_death3[] = {
	{ ai_move, -5 },
	{ ai_move, -5 },
	{ ai_move, -5 },
	{ ai_move, 0, soldier_death_shrink },
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
};
MMOVE_T(soldier_move_death3) = { FRAME_death301, FRAME_death345, soldier_frames_death3, soldier_dead };

mframe_t soldier_frames_death4[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move, 1.5f },
	{ ai_move, 2.5f },
	{ ai_move, -1.5f },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, -0.5f },
	{ ai_move },

	{ ai_move },
	{ ai_move, 4.0f },
	{ ai_move, 4.0f },
	{ ai_move, 8.0f, soldier_death_shrink },
	{ ai_move, 8.0f },
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
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 5.5f },

	{ ai_move, 2.5f },
	{ ai_move, -2.0f },
	{ ai_move, -2.0f }
};
MMOVE_T(soldier_move_death4) = { FRAME_death401, FRAME_death453, soldier_frames_death4, soldier_dead };

mframe_t soldier_frames_death5[] = {
	{ ai_move, -5 },
	{ ai_move, -5 },
	{ ai_move, -5 },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, soldier_death_shrink },
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

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(soldier_move_death5) = { FRAME_death501, FRAME_death524, soldier_frames_death5, soldier_dead };

mframe_t soldier_frames_death6[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, soldier_death_shrink },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(soldier_move_death6) = { FRAME_death601, FRAME_death610, soldier_frames_death6, soldier_dead };

DIE(soldier_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	int n;

	soldierh_hyper_laser_sound_end(self);

	// check for gib
	if (M_CheckGib(self, mod))
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		self->s.skinnum /= 2;

		if (self->beam)
		{
			G_FreeEdict(self->beam);
			self->beam = nullptr;
		}

		ThrowGibs(self, damage, {
			{ 3, "models/objects/gibs/sm_meat/tris.md2" },
			{ "models/objects/gibs/bone2/tris.md2" },
			{ "models/objects/gibs/bone/tris.md2" },
			{ "models/monsters/soldier/gibs/arm.md2", GIB_SKINNED },
			{ "models/monsters/soldier/gibs/gun.md2", GIB_SKINNED | GIB_UPRIGHT },
			{ "models/monsters/soldier/gibs/chest.md2", GIB_SKINNED },
			{ "models/monsters/soldier/gibs/head.md2", GIB_HEAD | GIB_SKINNED }
		});
		self->deadflag = true;
		return;
	}

	if (self->deadflag)
		return;

	// regular death
	self->deadflag = true;
	self->takedamage = true;
	
	n = self->count | 1;

	if (n == 1)
		gi.sound(self, CHAN_VOICE, sound_death_light, 1, ATTN_NORM, 0);
	else if (n == 3)
		gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	else // (n == 5)
		gi.sound(self, CHAN_VOICE, sound_death_ss, 1, ATTN_NORM, 0);

	if (fabsf((self->s.origin[2] + self->viewheight) - point[2]) <= 4 &&
		self->velocity.z < 65.f)
	{
		// head shot
		M_SetAnimation(self, &soldier_move_death3);
		return;
	}

	// if we die while on the ground, do a quicker death4
	if (self->monsterinfo.active_move == &soldier_move_trip ||
		self->monsterinfo.active_move == &soldier_move_attack5)
	{
		M_SetAnimation(self, &soldier_move_death4);
		self->monsterinfo.nextframe = FRAME_death413;
		soldier_death_shrink(self);
		return;
	}

	// only do the spin-death if we have enough velocity to justify it
	if (self->velocity.z > 65.f || self->velocity.length() > 150.f)
		n = irandom(5);
	else
		n = irandom(4);

	if (n == 0)
		M_SetAnimation(self, &soldier_move_death1);
	else if (n == 1)
		M_SetAnimation(self, &soldier_move_death2);
	else if (n == 2)
		M_SetAnimation(self, &soldier_move_death4);
	else if (n == 3)
		M_SetAnimation(self, &soldier_move_death5);
	else
		M_SetAnimation(self, &soldier_move_death6);
}

//
// NEW DODGE CODE
//

MONSTERINFO_SIDESTEP(soldier_sidestep) (edict_t *self) -> bool
{
	// don't sidestep during trip or up pain
	if (self->monsterinfo.active_move == &soldier_move_trip ||
		self->monsterinfo.active_move == &soldier_move_attack5 ||
		self->monsterinfo.active_move == &soldier_move_pain4)
		return false;

	if (self->count <= 3)
	{
		if (self->monsterinfo.active_move != &soldier_move_attack6)
		{
			M_SetAnimation(self, &soldier_move_attack6);
			soldierh_hyper_laser_sound_end(self);
		}
	}
	else
	{
		if (self->monsterinfo.active_move != &soldier_move_start_run &&
			self->monsterinfo.active_move != &soldier_move_run)
		{
			M_SetAnimation(self, &soldier_move_start_run);
			soldierh_hyper_laser_sound_end(self);
		}
	}

	return true;
}

MONSTERINFO_DUCK(soldier_duck) (edict_t *self, gtime_t eta) -> bool
{
	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;

	if (self->monsterinfo.active_move == &soldier_move_attack6)
	{
		M_SetAnimation(self, &soldier_move_trip);
	}
	else if (self->dmg || brandom())
	{
		M_SetAnimation(self, &soldier_move_duck);
	}
	else
	{
		M_SetAnimation(self, &soldier_move_attack3);
	}

	soldierh_hyper_laser_sound_end(self);
	return true;
}

//=========
// ROGUE
void soldier_blind(edict_t *self);

mframe_t soldier_frames_blind[] = {
	{ ai_move, 0, soldier_idle },
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
MMOVE_T(soldier_move_blind) = { FRAME_stand101, FRAME_stand130, soldier_frames_blind, soldier_blind };

MONSTERINFO_STAND(soldier_blind) (edict_t *self) -> void
{
	M_SetAnimation(self, &soldier_move_blind);
}
// ROGUE
//=========

//
// SPAWN
//

constexpr spawnflags_t SPAWNFLAG_SOLDIER_BLIND = 8_spawnflag;

void SP_monster_soldier_x(edict_t *self)
{
	self->s.modelindex = gi.modelindex("models/monsters/soldier/tris.md2");
	self->monsterinfo.scale = MODEL_SCALE;
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, 32 };
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	sound_idle.assign("soldier/solidle1.wav");
	sound_sight1.assign("soldier/solsght1.wav");
	sound_sight2.assign("soldier/solsrch1.wav");
	sound_cock.assign("infantry/infatck3.wav");
	
	gi.modelindex("models/monsters/soldier/gibs/head.md2");
	gi.modelindex("models/monsters/soldier/gibs/gun.md2");
	gi.modelindex("models/monsters/soldier/gibs/arm.md2");
	gi.modelindex("models/monsters/soldier/gibs/chest.md2");

	self->mass = 100;

	self->pain = soldier_pain;
	self->die = soldier_die;

	self->monsterinfo.stand = soldier_stand;
	self->monsterinfo.walk = soldier_walk;
	self->monsterinfo.run = soldier_run;
	self->monsterinfo.dodge = M_MonsterDodge;
	self->monsterinfo.attack = soldier_attack;
	self->monsterinfo.melee = nullptr;
	self->monsterinfo.sight = soldier_sight;
	self->monsterinfo.setskin = soldier_setskin;

	//=====
	// ROGUE
	self->monsterinfo.blocked = soldier_blocked;
	self->monsterinfo.duck = soldier_duck;
	self->monsterinfo.unduck = monster_duck_up;
	self->monsterinfo.sidestep = soldier_sidestep;

	if (self->spawnflags.has(SPAWNFLAG_SOLDIER_BLIND)) // blind
		self->monsterinfo.stand = soldier_blind;
	// ROGUE
	//=====

	gi.linkentity(self);

	self->monsterinfo.stand(self);

	walkmonster_start(self);
}

void SP_monster_soldier_vanilla(edict_t *self)
{
	SP_monster_soldier_x(self);
}

/*QUAKED monster_soldier_light (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
 */
void SP_monster_soldier_light(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	SP_monster_soldier_x(self);

	sound_pain_light.assign("soldier/solpain2.wav");
	sound_death_light.assign("soldier/soldeth2.wav");
	gi.modelindex("models/objects/laser/tris.md2");
	gi.soundindex("misc/lasfly.wav");
	gi.soundindex("soldier/solatck2.wav");

	self->s.skinnum = 0;
	self->count = self->s.skinnum;
	self->health = self->max_health = 20 * st.health_multiplier;
	self->gib_health = -30;

	// PMM - blindfire
	self->monsterinfo.blindfire = true;
}

/*QUAKED monster_soldier (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
 */
void SP_monster_soldier(edict_t *self)
{
	if( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	SP_monster_soldier_x(self);

	sound_pain.assign("soldier/solpain1.wav");
	sound_death.assign("soldier/soldeth1.wav");
	gi.soundindex("soldier/solatck1.wav");

	self->s.skinnum = 2;
	self->count = self->s.skinnum;
	self->health = self->max_health = 30 * st.health_multiplier;
	self->gib_health = -30;
}

/*QUAKED monster_soldier_ss (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
 */
void SP_monster_soldier_ss(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	SP_monster_soldier_x(self);

	sound_pain_ss.assign("soldier/solpain3.wav");
	sound_death_ss.assign("soldier/soldeth3.wav");
	gi.soundindex("soldier/solatck3.wav");

	self->s.skinnum = 4;
	self->count = self->s.skinnum;
	self->health = self->max_health = 40 * st.health_multiplier;
	self->gib_health = -30;
}

//
// SPAWN
//

void SP_monster_soldier_h(edict_t *self)
{
	SP_monster_soldier_x(self);
	self->style = 1;
}

/*QUAKED monster_soldier_ripper (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
 */
void SP_monster_soldier_ripper(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	SP_monster_soldier_h(self);

	sound_pain_light.assign("soldier/solpain2.wav");
	sound_death_light.assign("soldier/soldeth2.wav");

	gi.modelindex("models/objects/boomrang/tris.md2");
	gi.soundindex("misc/lasfly.wav");
	gi.soundindex("soldier/solatck2.wav");

	self->s.skinnum = 6;
	self->count = self->s.skinnum - 6;
	self->health = self->max_health = 50 * st.health_multiplier;
	self->gib_health = -30;

	// PMM - blindfire
	self->monsterinfo.blindfire = true;
}

/*QUAKED monster_soldier_hypergun (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
 */
void SP_monster_soldier_hypergun(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	SP_monster_soldier_h(self);

	gi.modelindex("models/objects/laser/tris.md2");
	sound_pain.assign("soldier/solpain1.wav");
	sound_death.assign("soldier/soldeth1.wav");
	gi.soundindex("soldier/solatck1.wav");
	gi.soundindex("weapons/hyprbd1a.wav");
	gi.soundindex("weapons/hyprbl1a.wav");

	self->s.skinnum = 8;
	self->count = self->s.skinnum - 6;
	self->health = self->max_health = 60 * st.health_multiplier;
	self->gib_health = -30;

	// PMM - blindfire
	self->monsterinfo.blindfire = true;
}

/*QUAKED monster_soldier_lasergun (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
 */
void SP_monster_soldier_lasergun(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	SP_monster_soldier_h(self);

	sound_pain_ss.assign("soldier/solpain3.wav");
	sound_death_ss.assign("soldier/soldeth3.wav");
	gi.soundindex("soldier/solatck3.wav");

	self->s.skinnum = 10;
	self->count = self->s.skinnum - 6;
	self->health = self->max_health = 70 * st.health_multiplier;
	self->gib_health = -30;
}

// END 13-APR-98
