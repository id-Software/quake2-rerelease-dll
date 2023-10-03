// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

stalker

==============================================================================
*/

#include "../g_local.h"
#include "m_rogue_stalker.h"
#include <float.h>

static cached_soundindex sound_pain;
static cached_soundindex sound_die;
static cached_soundindex sound_sight;
static cached_soundindex sound_punch_hit1;
static cached_soundindex sound_punch_hit2;
static cached_soundindex sound_idle;

bool stalker_do_pounce(edict_t *self, const vec3_t &dest);
void stalker_walk(edict_t *self);
void stalker_dodge_jump(edict_t *self);
void stalker_swing_attack(edict_t *self);
void stalker_jump_straightup(edict_t *self);
void stalker_jump_wait_land(edict_t *self);
void stalker_false_death(edict_t *self);
void stalker_false_death_start(edict_t *self);
bool stalker_ok_to_transition(edict_t *self);
void stalker_stand(edict_t *self);

inline bool STALKER_ON_CEILING(edict_t *ent)
{
	return (ent->gravityVector[2] > 0);
}

//=========================
//=========================
bool stalker_ok_to_transition(edict_t *self)
{
	trace_t trace;
	vec3_t	pt, start;
	float	max_dist;
	float	margin;
	float	end_height;

	if (STALKER_ON_CEILING(self))
	{
		// [Paril-KEX] if we get knocked off the ceiling, always
		// fall downwards
		if (!self->groundentity)
			return true;

		max_dist = -384;
		margin = self->mins[2] - 8;
	}
	else
	{
		// her stalkers are just better
		if (self->monsterinfo.aiflags & AI_SPAWNED_WIDOW)
			max_dist = 256;
		else
			max_dist = 180;
		margin = self->maxs[2] + 8;
	}

	pt = self->s.origin;
	pt[2] += max_dist;
	trace = gi.trace(self->s.origin, self->mins, self->maxs, pt, self, MASK_MONSTERSOLID);

	if (trace.fraction == 1.0f ||
		!(trace.contents & CONTENTS_SOLID) ||
		(trace.ent != world))
	{
		if (STALKER_ON_CEILING(self))
		{
			if (trace.plane.normal[2] < 0.9f)
				return false;
		}
		else
		{
			if (trace.plane.normal[2] > -0.9f)
				return false;
		}
	}

	end_height = trace.endpos[2];

	// check the four corners, tracing only to the endpoint of the center trace (vertically).
	pt[0] = self->absmin[0];
	pt[1] = self->absmin[1];
	pt[2] = trace.endpos[2] + margin; // give a little margin of error to allow slight inclines
	start = pt;
	start[2] = self->s.origin[2];
	trace = gi.traceline(start, pt, self, MASK_MONSTERSOLID);
	if (trace.fraction == 1.0f || !(trace.contents & CONTENTS_SOLID) || (trace.ent != world))
		return false;
	if (fabsf(end_height + margin - trace.endpos[2]) > 8)
		return false;

	pt[0] = self->absmax[0];
	pt[1] = self->absmin[1];
	start = pt;
	start[2] = self->s.origin[2];
	trace = gi.traceline(start, pt, self, MASK_MONSTERSOLID);
	if (trace.fraction == 1.0f || !(trace.contents & CONTENTS_SOLID) || (trace.ent != world))
		return false;
	if (fabsf(end_height + margin - trace.endpos[2]) > 8)
		return false;

	pt[0] = self->absmax[0];
	pt[1] = self->absmax[1];
	start = pt;
	start[2] = self->s.origin[2];
	trace = gi.traceline(start, pt, self, MASK_MONSTERSOLID);
	if (trace.fraction == 1.0f || !(trace.contents & CONTENTS_SOLID) || (trace.ent != world))
		return false;
	if (fabsf(end_height + margin - trace.endpos[2]) > 8)
		return false;

	pt[0] = self->absmin[0];
	pt[1] = self->absmax[1];
	start = pt;
	start[2] = self->s.origin[2];
	trace = gi.traceline(start, pt, self, MASK_MONSTERSOLID);
	if (trace.fraction == 1.0f || !(trace.contents & CONTENTS_SOLID) || (trace.ent != world))
		return false;
	if (fabsf(end_height + margin - trace.endpos[2]) > 8)
		return false;

	return true;
}

//=========================
//=========================
MONSTERINFO_SIGHT(stalker_sight) (edict_t *self, edict_t *other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

// ******************
// IDLE
// ******************

void stalker_idle_noise(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_idle, 0.5, ATTN_IDLE, 0);
}

mframe_t stalker_frames_idle[] = {
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },

	{ ai_stand },
	{ ai_stand, 0, stalker_idle_noise },
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
MMOVE_T(stalker_move_idle) = { FRAME_idle01, FRAME_idle21, stalker_frames_idle, stalker_stand };

mframe_t stalker_frames_idle2[] = {
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
MMOVE_T(stalker_move_idle2) = { FRAME_idle201, FRAME_idle213, stalker_frames_idle2, stalker_stand };

MONSTERINFO_IDLE(stalker_idle) (edict_t *self) -> void
{
	if (frandom() < 0.35f)
		M_SetAnimation(self, &stalker_move_idle);
	else
		M_SetAnimation(self, &stalker_move_idle2);
}

// ******************
// STAND
// ******************

mframe_t stalker_frames_stand[] = {
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },

	{ ai_stand },
	{ ai_stand, 0, stalker_idle_noise },
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
MMOVE_T(stalker_move_stand) = { FRAME_idle01, FRAME_idle21, stalker_frames_stand, stalker_stand };

MONSTERINFO_STAND(stalker_stand) (edict_t *self) -> void
{
	if (frandom() < 0.25f)
		M_SetAnimation(self, &stalker_move_stand);
	else
		M_SetAnimation(self, &stalker_move_idle2);
}

// ******************
// RUN
// ******************

mframe_t stalker_frames_run[] = {
	{ ai_run, 13, monster_footstep },
	{ ai_run, 17 },
	{ ai_run, 21, monster_footstep },
	{ ai_run, 18 }
};
MMOVE_T(stalker_move_run) = { FRAME_run01, FRAME_run04, stalker_frames_run, nullptr };

MONSTERINFO_RUN(stalker_run) (edict_t *self) -> void
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		M_SetAnimation(self, &stalker_move_stand);
	else
		M_SetAnimation(self, &stalker_move_run);
}

// ******************
// WALK
// ******************

mframe_t stalker_frames_walk[] = {
	{ ai_walk, 4, monster_footstep },
	{ ai_walk, 6 },
	{ ai_walk, 8 },
	{ ai_walk, 5 },

	{ ai_walk, 4, monster_footstep },
	{ ai_walk, 6 },
	{ ai_walk, 8 },
	{ ai_walk, 4 }
};
MMOVE_T(stalker_move_walk) = { FRAME_walk01, FRAME_walk08, stalker_frames_walk, stalker_walk };

MONSTERINFO_WALK(stalker_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &stalker_move_walk);
}

// ******************
// false death
// ******************
mframe_t stalker_frames_reactivate[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, monster_footstep }
};
MMOVE_T(stalker_move_false_death_end) = { FRAME_reactive01, FRAME_reactive04, stalker_frames_reactivate, stalker_run };

void stalker_reactivate(edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_STAND_GROUND;
	M_SetAnimation(self, &stalker_move_false_death_end);
}

void stalker_heal(edict_t *self)
{
	if (skill->integer == 2)
		self->health += 2;
	else if (skill->integer == 3)
		self->health += 3;
	else
		self->health++;

	self->monsterinfo.setskin(self);

	if (self->health >= self->max_health)
	{
		self->health = self->max_health;
		stalker_reactivate(self);
	}
}

mframe_t stalker_frames_false_death[] = {
	{ ai_move, 0, stalker_heal },
	{ ai_move, 0, stalker_heal },
	{ ai_move, 0, stalker_heal },
	{ ai_move, 0, stalker_heal },
	{ ai_move, 0, stalker_heal },

	{ ai_move, 0, stalker_heal },
	{ ai_move, 0, stalker_heal },
	{ ai_move, 0, stalker_heal },
	{ ai_move, 0, stalker_heal },
	{ ai_move, 0, stalker_heal }
};
MMOVE_T(stalker_move_false_death) = { FRAME_twitch01, FRAME_twitch10, stalker_frames_false_death, stalker_false_death };

void stalker_false_death(edict_t *self)
{
	M_SetAnimation(self, &stalker_move_false_death);
}

mframe_t stalker_frames_false_death_start[] = {
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
MMOVE_T(stalker_move_false_death_start) = { FRAME_death01, FRAME_death09, stalker_frames_false_death_start, stalker_false_death };

void stalker_false_death_start(edict_t *self)
{
	self->s.angles[2] = 0;
	self->gravityVector = { 0, 0, -1 };

	self->monsterinfo.aiflags |= AI_STAND_GROUND;
	M_SetAnimation(self, &stalker_move_false_death_start);
}

// ******************
// PAIN
// ******************

mframe_t stalker_frames_pain[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(stalker_move_pain) = { FRAME_pain01, FRAME_pain04, stalker_frames_pain, stalker_run };

PAIN(stalker_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	if (self->deadflag)
		return;

	if (self->groundentity == nullptr)
		return;

	// if we're reactivating or false dying, ignore the pain.
	if (self->monsterinfo.active_move == &stalker_move_false_death_end ||
		self->monsterinfo.active_move == &stalker_move_false_death_start)
		return;

	if (self->monsterinfo.active_move == &stalker_move_false_death)
	{
		stalker_reactivate(self);
		return;
	}

	if ((self->health > 0) && (self->health < (self->max_health / 4)))
	{
		if (frandom() < 0.30f)
		{
			if (!STALKER_ON_CEILING(self) || stalker_ok_to_transition(self))
			{
				stalker_false_death_start(self);
				return;
			}
		}
	}

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3_sec;

	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	if (mod.id == MOD_CHAINFIST || damage > 10) // don't react unless the damage was significant
	{
		// stalker should dodge jump periodically to help avoid damage.
		if (self->groundentity && (frandom() < 0.5f))
			stalker_dodge_jump(self);
		else if (M_ShouldReactToPain(self, mod)) // no pain anims in nightmare
			M_SetAnimation(self, &stalker_move_pain);
	}
}

MONSTERINFO_SETSKIN(stalker_setskin) (edict_t *self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

// ******************
// STALKER ATTACK
// ******************

void stalker_shoot_attack(edict_t *self)
{
	vec3_t	offset, start, f, r, dir;
	vec3_t	end;
	float	dist;
	trace_t trace;

	if (!has_valid_enemy(self))
		return;

	if (self->groundentity && frandom() < 0.33f)
	{
		dir = self->enemy->s.origin - self->s.origin;
		dist = dir.length();

		if ((dist > 256) || (frandom() < 0.5f))
			stalker_do_pounce(self, self->enemy->s.origin);
		else
			stalker_jump_straightup(self);
	}

	AngleVectors(self->s.angles, f, r, nullptr);
	offset = { 24, 0, 6 };
	start = M_ProjectFlashSource(self, offset, f, r);

	dir = self->enemy->s.origin - start;
	if (frandom() < 0.3f)
		PredictAim(self, self->enemy, start, 1000, true, 0, &dir, &end);
	else
		end = self->enemy->s.origin;

	trace = gi.traceline(start, end, self, MASK_PROJECTILE);
	if (trace.ent == self->enemy || trace.ent == world)
	{
		dir.normalize();
		monster_fire_blaster2(self, start, dir, 5, 800, MZ2_STALKER_BLASTER, EF_BLASTER);
	}
}

void stalker_shoot_attack2(edict_t *self)
{
	if (frandom() < 0.5)
		stalker_shoot_attack(self);
}

mframe_t stalker_frames_shoot[] = {
	{ ai_charge, 13 },
	{ ai_charge, 17, stalker_shoot_attack },
	{ ai_charge, 21 },
	{ ai_charge, 18, stalker_shoot_attack2 }
};
MMOVE_T(stalker_move_shoot) = { FRAME_run01, FRAME_run04, stalker_frames_shoot, stalker_run };

MONSTERINFO_ATTACK(stalker_attack_ranged) (edict_t *self) -> void
{
	if (!has_valid_enemy(self))
		return;

	// PMM - circle strafe stuff
	if (frandom() > 0.5f)
	{
		self->monsterinfo.attack_state = AS_STRAIGHT;
	}
	else
	{
		if (frandom() <= 0.5f) // switch directions
			self->monsterinfo.lefty = !self->monsterinfo.lefty;
		self->monsterinfo.attack_state = AS_SLIDING;
	}
	M_SetAnimation(self, &stalker_move_shoot);
}

// ******************
// close combat
// ******************

void stalker_swing_attack(edict_t *self)
{
	vec3_t aim = { MELEE_DISTANCE, 0, 0 };
	if (fire_hit(self, aim, irandom(5, 10), 50))
	{
		if (self->s.frame < FRAME_attack08)
			gi.sound(self, CHAN_WEAPON, sound_punch_hit2, 1, ATTN_NORM, 0);
		else
			gi.sound(self, CHAN_WEAPON, sound_punch_hit1, 1, ATTN_NORM, 0);
	}
	else
		self->monsterinfo.melee_debounce_time = level.time + 0.8_sec;
}

mframe_t stalker_frames_swing_l[] = {
	{ ai_charge, 2 },
	{ ai_charge, 4 },
	{ ai_charge, 6 },
	{ ai_charge, 10, monster_footstep },

	{ ai_charge, 5, stalker_swing_attack },
	{ ai_charge, 5 },
	{ ai_charge, 5 },
	{ ai_charge, 5, monster_footstep } // stalker_swing_check_l
};
MMOVE_T(stalker_move_swing_l) = { FRAME_attack01, FRAME_attack08, stalker_frames_swing_l, stalker_run };

mframe_t stalker_frames_swing_r[] = {
	{ ai_charge, 4 },
	{ ai_charge, 6, monster_footstep },
	{ ai_charge, 6, stalker_swing_attack },
	{ ai_charge, 10 },
	{ ai_charge, 5, monster_footstep } // stalker_swing_check_r
};
MMOVE_T(stalker_move_swing_r) = { FRAME_attack11, FRAME_attack15, stalker_frames_swing_r, stalker_run };

MONSTERINFO_MELEE(stalker_attack_melee) (edict_t *self) -> void
{
	if (!has_valid_enemy(self))
		return;

	if (frandom() < 0.5f)
		M_SetAnimation(self, &stalker_move_swing_l);
	else
		M_SetAnimation(self, &stalker_move_swing_r);
}

// ******************
// POUNCE
// ******************

// ====================
// ====================
bool stalker_check_lz(edict_t *self, edict_t *target, const vec3_t &dest)
{
	if ((gi.pointcontents(dest) & MASK_WATER) || (target->waterlevel))
		return false;

	if (!target->groundentity)
		return false;

	vec3_t jumpLZ;

	// check under the player's four corners
	// if they're not solid, bail.
	jumpLZ[0] = self->enemy->mins[0];
	jumpLZ[1] = self->enemy->mins[1];
	jumpLZ[2] = self->enemy->mins[2] - 0.25f;
	if (!(gi.pointcontents(jumpLZ) & MASK_SOLID))
		return false;

	jumpLZ[0] = self->enemy->maxs[0];
	jumpLZ[1] = self->enemy->mins[1];
	if (!(gi.pointcontents(jumpLZ) & MASK_SOLID))
		return false;

	jumpLZ[0] = self->enemy->maxs[0];
	jumpLZ[1] = self->enemy->maxs[1];
	if (!(gi.pointcontents(jumpLZ) & MASK_SOLID))
		return false;

	jumpLZ[0] = self->enemy->mins[0];
	jumpLZ[1] = self->enemy->maxs[1];
	if (!(gi.pointcontents(jumpLZ) & MASK_SOLID))
		return false;

	return true;
}

// ====================
// ====================
bool stalker_do_pounce(edict_t *self, const vec3_t &dest)
{
	vec3_t	dist;
	float	length;
	vec3_t	jumpAngles;
	vec3_t	jumpLZ;
	float	velocity = 400.1f;

	// don't pounce when we're on the ceiling
	if (STALKER_ON_CEILING(self))
		return false;

	if (!stalker_check_lz(self, self->enemy, dest))
		return false;

	dist = dest - self->s.origin;

	// make sure we're pointing in that direction 15deg margin of error.
	jumpAngles = vectoangles(dist);
	if (fabsf(jumpAngles[YAW] - self->s.angles[YAW]) > 45)
		return false; // not facing the player...

	if (isnan(jumpAngles[YAW]))
		return false; // Switch why

	self->ideal_yaw = jumpAngles[YAW];
	M_ChangeYaw(self);

	length = dist.length();
	if (length > 450)
		return false; // can't jump that far...

	jumpLZ = dest;
	vec3_t dir = dist.normalized();

	// find a valid angle/velocity combination
	while (velocity <= 800)
	{
		if (M_CalculatePitchToFire(self, jumpLZ, self->s.origin, dir, velocity, 3, false, true))
			break;

		velocity += 200;
	}

	// nothing found
	if (velocity > 800)
		return false;

	self->velocity = dir * velocity;
	return true;
}

// ******************
// DODGE
// ******************

//===================
// stalker_jump_straightup
//===================
void stalker_jump_straightup(edict_t *self)
{
	if (self->deadflag)
		return;

	if (STALKER_ON_CEILING(self))
	{
		if (stalker_ok_to_transition(self))
		{
			self->gravityVector[2] = -1;
			self->s.angles[2] += 180.0f;
			if (self->s.angles[2] > 360.0f)
				self->s.angles[2] -= 360.0f;
			self->groundentity = nullptr;
		}
	}
	else if (self->groundentity) // make sure we're standing on SOMETHING...
	{
		self->velocity[0] += crandom() * 5;
		self->velocity[1] += crandom() * 5;
		self->velocity[2] += -400 * self->gravityVector[2];
		if (stalker_ok_to_transition(self))
		{
			self->gravityVector[2] = 1;
			self->s.angles[2] = 180.0;
			self->groundentity = nullptr;
		}
	}
}

mframe_t stalker_frames_jump_straightup[] = {
	{ ai_move, 1, stalker_jump_straightup },
	{ ai_move, 1, stalker_jump_wait_land },
	{ ai_move, -1, monster_footstep },
	{ ai_move, -1 }
};

MMOVE_T(stalker_move_jump_straightup) = { FRAME_jump04, FRAME_jump07, stalker_frames_jump_straightup, stalker_run };

//===================
// stalker_dodge_jump - abstraction so pain function can trigger a dodge jump too without
//		faking the inputs to stalker_dodge
//===================
void stalker_dodge_jump(edict_t *self)
{
	M_SetAnimation(self, &stalker_move_jump_straightup);
}

#if 0
mframe_t stalker_frames_dodge_run[] = {
	{ ai_run, 13 },
	{ ai_run, 17 },
	{ ai_run, 21 },
	{ ai_run, 18, monster_done_dodge }
};
MMOVE_T(stalker_move_dodge_run) = { FRAME_run01, FRAME_run04, stalker_frames_dodge_run, nullptr };
#endif

MONSTERINFO_DODGE(stalker_dodge) (edict_t *self, edict_t *attacker, gtime_t eta, trace_t *tr, bool gravity) -> void
{
	if (!self->groundentity || self->health <= 0)
		return;

	if (!self->enemy)
	{
		self->enemy = attacker;
		FoundTarget(self);
		return;
	}

	// PMM - don't bother if it's going to hit anyway; fix for weird in-your-face etas (I was
	// seeing numbers like 13 and 14)
	if ((eta < FRAME_TIME_MS) || (eta > 5_sec))
		return;

	if (self->timestamp > level.time)
		return;

	self->timestamp = level.time + random_time(1_sec, 5_sec);
	// this will override the foundtarget call of stalker_run
	stalker_dodge_jump(self);
}

// ******************
// Jump onto / off of things
// ******************

//===================
//===================
void stalker_jump_down(edict_t *self)
{
	vec3_t forward, up;

	AngleVectors(self->s.angles, forward, nullptr, up);
	self->velocity += (forward * 100);
	self->velocity += (up * 300);
}

//===================
//===================
void stalker_jump_up(edict_t *self)
{
	vec3_t forward, up;

	AngleVectors(self->s.angles, forward, nullptr, up);
	self->velocity += (forward * 200);
	self->velocity += (up * 450);
}

//===================
//===================
void stalker_jump_wait_land(edict_t *self)
{
	if ((frandom() < 0.4f) && (level.time >= self->monsterinfo.attack_finished))
	{
		self->monsterinfo.attack_finished = level.time + 300_ms;
		stalker_shoot_attack(self);
	}

	if (self->groundentity == nullptr)
	{
		self->gravity = 1.3f;
		self->monsterinfo.nextframe = self->s.frame;

		if (monster_jump_finished(self))
		{
			self->gravity = 1;
			self->monsterinfo.nextframe = self->s.frame + 1;
		}
	}
	else
	{
		self->gravity = 1;
		self->monsterinfo.nextframe = self->s.frame + 1;
	}
}

mframe_t stalker_frames_jump_up[] = {
	{ ai_move, -8 },
	{ ai_move, -8 },
	{ ai_move, -8 },
	{ ai_move, -8 },

	{ ai_move, 0, stalker_jump_up },
	{ ai_move, 0, stalker_jump_wait_land },
	{ ai_move, 0, monster_footstep }
};
MMOVE_T(stalker_move_jump_up) = { FRAME_jump01, FRAME_jump07, stalker_frames_jump_up, stalker_run };

mframe_t stalker_frames_jump_down[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move, 0, stalker_jump_down },
	{ ai_move, 0, stalker_jump_wait_land },
	{ ai_move, 0, monster_footstep }
};
MMOVE_T(stalker_move_jump_down) = { FRAME_jump01, FRAME_jump07, stalker_frames_jump_down, stalker_run };

//============
// stalker_jump - this is only used for jumping onto or off of things. for dodge jumping,
//		use stalker_dodge_jump
//============
void stalker_jump(edict_t *self, blocked_jump_result_t result)
{
	if (!self->enemy)
		return;

	if (result == blocked_jump_result_t::JUMP_JUMP_UP)
		M_SetAnimation(self, &stalker_move_jump_up);
	else
		M_SetAnimation(self, &stalker_move_jump_down);
}

// ******************
// Blocked
// ******************
MONSTERINFO_BLOCKED(stalker_blocked) (edict_t *self, float dist) -> bool
{
	if (!has_valid_enemy(self))
		return false;

	bool onCeiling = STALKER_ON_CEILING(self);

	if (!onCeiling)
	{
		if (auto result = blocked_checkjump(self, dist); result != blocked_jump_result_t::NO_JUMP)
		{
			if (result != blocked_jump_result_t::JUMP_TURN)
				stalker_jump(self, result);
			return true;
		}

		if (blocked_checkplat(self, dist))
			return true;

		if (visible(self, self->enemy) && frandom() < 0.1f)
		{
			stalker_do_pounce(self, self->enemy->s.origin);
			return true;
		}
	}
	else
	{
		if (stalker_ok_to_transition(self))
		{
			self->gravityVector[2] = -1;
			self->s.angles[2] += 180.0f;
			if (self->s.angles[2] > 360.0f)
				self->s.angles[2] -= 360.0f;
			self->groundentity = nullptr;
			return true;
		}
	}

	return false;
}

// [Paril-KEX] quick patch-job to fix stalkers endlessly floating up into the sky
MONSTERINFO_PHYSCHANGED(stalker_physics_change) (edict_t *self) -> void
{
	if (STALKER_ON_CEILING(self) && !self->groundentity)
	{
		self->gravityVector[2] = -1;
		self->s.angles[2] += 180.0f;
		if (self->s.angles[2] > 360.0f)
			self->s.angles[2] -= 360.0f;
	}
}

// ******************
// Death
// ******************

void stalker_dead(edict_t *self)
{
	self->mins = { -28, -28, -18 };
	self->maxs = { 28, 28, -4 };
	monster_dead(self);
}

mframe_t stalker_frames_death[] = {
	{ ai_move },
	{ ai_move, -5 },
	{ ai_move, -10 },
	{ ai_move, -20 },

	{ ai_move, -10 },
	{ ai_move, -10 },
	{ ai_move, -5 },
	{ ai_move, -5 },

	{ ai_move, 0, monster_footstep }
};
MMOVE_T(stalker_move_death) = { FRAME_death01, FRAME_death09, stalker_frames_death, stalker_dead };

DIE(stalker_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	// dude bit it, make him fall!
	self->movetype = MOVETYPE_TOSS;
	self->s.angles[2] = 0;
	self->gravityVector = { 0, 0, -1 };

	// check for gib
	if (M_CheckGib(self, mod))
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		self->s.skinnum /= 2;

		ThrowGibs(self, damage, {
			{ 2, "models/objects/gibs/sm_meat/tris.md2" },
			{ 2, "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC },
			{ "models/monsters/stalker/gibs/bodya.md2", GIB_SKINNED },
			{ "models/monsters/stalker/gibs/bodyb.md2", GIB_SKINNED },
			{ 2, "models/monsters/stalker/gibs/claw.md2", GIB_SKINNED | GIB_UPRIGHT },
			{ 2, "models/monsters/stalker/gibs/leg.md2", GIB_SKINNED | GIB_UPRIGHT },
			{ 2, "models/monsters/stalker/gibs/foot.md2", GIB_SKINNED },
			{ "models/monsters/stalker/gibs/head.md2", GIB_SKINNED | GIB_HEAD }
		});
		self->deadflag = true;
		return;
	}

	if (self->deadflag)
		return;

	// regular death
	gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	self->deadflag = true;
	self->takedamage = true;
	M_SetAnimation(self, &stalker_move_death);
}

// ******************
// SPAWN
// ******************

/*QUAKED monster_stalker (1 .5 0) (-28 -28 -18) (28 28 18) Ambush Trigger_Spawn Sight OnRoof NoJumping
Spider Monster

  ONROOF - Monster starts sticking to the roof.
*/

constexpr spawnflags_t SPAWNFLAG_STALKER_ONROOF = 8_spawnflag;
constexpr spawnflags_t SPAWNFLAG_STALKER_NOJUMPING = 16_spawnflag;

void SP_monster_stalker(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	sound_pain.assign("stalker/pain.wav");
	sound_die.assign("stalker/death.wav");
	sound_sight.assign("stalker/sight.wav");
	sound_punch_hit1.assign("stalker/melee1.wav");
	sound_punch_hit2.assign("stalker/melee2.wav");
	sound_idle.assign("stalker/idle.wav");

	// PMM - precache bolt2
	gi.modelindex("models/objects/laser/tris.md2");

	self->s.modelindex = gi.modelindex("models/monsters/stalker/tris.md2");

	gi.modelindex("models/monsters/stalker/gibs/bodya.md2");
	gi.modelindex("models/monsters/stalker/gibs/bodyb.md2");
	gi.modelindex("models/monsters/stalker/gibs/claw.md2");
	gi.modelindex("models/monsters/stalker/gibs/foot.md2");
	gi.modelindex("models/monsters/stalker/gibs/head.md2");
	gi.modelindex("models/monsters/stalker/gibs/leg.md2");

	self->mins = { -28, -28, -18 };
	self->maxs = { 28, 28, 18 };
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->health = 250 * st.health_multiplier;
	self->gib_health = -50;
	self->mass = 250;

	self->pain = stalker_pain;
	self->die = stalker_die;

	self->monsterinfo.stand = stalker_stand;
	self->monsterinfo.walk = stalker_walk;
	self->monsterinfo.run = stalker_run;
	self->monsterinfo.attack = stalker_attack_ranged;
	self->monsterinfo.sight = stalker_sight;
	self->monsterinfo.idle = stalker_idle;
	self->monsterinfo.dodge = stalker_dodge;
	self->monsterinfo.blocked = stalker_blocked;
	self->monsterinfo.melee = stalker_attack_melee;
	self->monsterinfo.setskin = stalker_setskin;
	self->monsterinfo.physics_change = stalker_physics_change;

	gi.linkentity(self);

	M_SetAnimation(self, &stalker_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	if (self->spawnflags.has(SPAWNFLAG_STALKER_ONROOF))
	{
		self->s.angles[2] = 180;
		self->gravityVector[2] = 1;
	}

	self->monsterinfo.can_jump = !self->spawnflags.has(SPAWNFLAG_STALKER_NOJUMPING);
	self->monsterinfo.drop_height = 256;
	self->monsterinfo.jump_height = 68;

	walkmonster_start(self);
}
