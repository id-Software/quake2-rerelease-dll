// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

chick

==============================================================================
*/

#include "g_local.h"
#include "m_chick.h"
#include "m_flash.h"

void chick_stand(edict_t *self);
void chick_run(edict_t *self);
void chick_reslash(edict_t *self);
void chick_rerocket(edict_t *self);
void chick_attack1(edict_t *self);

static cached_soundindex sound_missile_prelaunch;
static cached_soundindex sound_missile_launch;
static cached_soundindex sound_melee_swing;
static cached_soundindex sound_melee_hit;
static cached_soundindex sound_missile_reload;
static cached_soundindex sound_death1;
static cached_soundindex sound_death2;
static cached_soundindex sound_fall_down;
static cached_soundindex sound_idle1;
static cached_soundindex sound_idle2;
static cached_soundindex sound_pain1;
static cached_soundindex sound_pain2;
static cached_soundindex sound_pain3;
static cached_soundindex sound_sight;
static cached_soundindex sound_search;

void ChickMoan(edict_t *self)
{
	if (frandom() < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_idle1, 1, ATTN_IDLE, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_idle2, 1, ATTN_IDLE, 0);
}

mframe_t chick_frames_fidget[] = {
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand, 0, ChickMoan },
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
MMOVE_T(chick_move_fidget) = { FRAME_stand201, FRAME_stand230, chick_frames_fidget, chick_stand };

void chick_fidget(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		return;
	else if (self->enemy)
		return;
	if (frandom() <= 0.3f)
		M_SetAnimation(self, &chick_move_fidget);
}

mframe_t chick_frames_stand[] = {
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
	{ ai_stand },
	{ ai_stand, 0, chick_fidget },
};
MMOVE_T(chick_move_stand) = { FRAME_stand101, FRAME_stand130, chick_frames_stand, nullptr };

MONSTERINFO_STAND(chick_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &chick_move_stand);
}

mframe_t chick_frames_start_run[] = {
	{ ai_run, 1 },
	{ ai_run },
	{ ai_run, 0, monster_footstep },
	{ ai_run, -1 },
	{ ai_run, -1, monster_footstep },
	{ ai_run },
	{ ai_run, 1 },
	{ ai_run, 3 },
	{ ai_run, 6 },
	{ ai_run, 3 }
};
MMOVE_T(chick_move_start_run) = { FRAME_walk01, FRAME_walk10, chick_frames_start_run, chick_run };

mframe_t chick_frames_run[] = {
	{ ai_run, 6 },
	{ ai_run, 8, monster_footstep },
	{ ai_run, 13 },
	{ ai_run, 5, monster_done_dodge }, // make sure to clear dodge bit
	{ ai_run, 7 },
	{ ai_run, 4 },
	{ ai_run, 11, monster_footstep },
	{ ai_run, 5 },
	{ ai_run, 9 },
	{ ai_run, 7 }
};

MMOVE_T(chick_move_run) = { FRAME_walk11, FRAME_walk20, chick_frames_run, nullptr };

mframe_t chick_frames_walk[] = {
	{ ai_walk, 6 },
	{ ai_walk, 8, monster_footstep },
	{ ai_walk, 13 },
	{ ai_walk, 5 },
	{ ai_walk, 7 },
	{ ai_walk, 4 },
	{ ai_walk, 11, monster_footstep },
	{ ai_walk, 5 },
	{ ai_walk, 9 },
	{ ai_walk, 7 }
};

MMOVE_T(chick_move_walk) = { FRAME_walk11, FRAME_walk20, chick_frames_walk, nullptr };

MONSTERINFO_WALK(chick_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &chick_move_walk);
}

MONSTERINFO_RUN(chick_run) (edict_t *self) -> void
{
	monster_done_dodge(self);

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		M_SetAnimation(self, &chick_move_stand);
		return;
	}

	if (self->monsterinfo.active_move == &chick_move_walk ||
		self->monsterinfo.active_move == &chick_move_start_run)
	{
		M_SetAnimation(self, &chick_move_run);
	}
	else
	{
		M_SetAnimation(self, &chick_move_start_run);
	}
}

mframe_t chick_frames_pain1[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(chick_move_pain1) = { FRAME_pain101, FRAME_pain105, chick_frames_pain1, chick_run };

mframe_t chick_frames_pain2[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(chick_move_pain2) = { FRAME_pain201, FRAME_pain205, chick_frames_pain2, chick_run };

mframe_t chick_frames_pain3[] = {
	{ ai_move },
	{ ai_move, 0, monster_footstep },
	{ ai_move, -6 },
	{ ai_move, 3, monster_footstep },
	{ ai_move, 11 },
	{ ai_move, 3, monster_footstep },
	{ ai_move },
	{ ai_move },
	{ ai_move, 4 },
	{ ai_move, 1 },
	{ ai_move },
	{ ai_move, -3 },
	{ ai_move, -4 },
	{ ai_move, 5 },
	{ ai_move, 7 },
	{ ai_move, -2 },
	{ ai_move, 3 },
	{ ai_move, -5 },
	{ ai_move, -2 },
	{ ai_move, -8 },
	{ ai_move, 2, monster_footstep }
};
MMOVE_T(chick_move_pain3) = { FRAME_pain301, FRAME_pain321, chick_frames_pain3, chick_run };

PAIN(chick_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	float r;

	monster_done_dodge(self);

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3_sec;

	r = frandom();
	if (r < 0.33f)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else if (r < 0.66f)
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain3, 1, ATTN_NORM, 0);

	if (!M_ShouldReactToPain(self, mod))
		return; // no pain anims in nightmare

	// PMM - clear this from blindfire
	self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;

	if (damage <= 10)
		M_SetAnimation(self, &chick_move_pain1);
	else if (damage <= 25)
		M_SetAnimation(self, &chick_move_pain2);
	else
		M_SetAnimation(self, &chick_move_pain3);

	// PMM - clear duck flag
	if (self->monsterinfo.aiflags & AI_DUCKED)
		monster_duck_up(self);
}

MONSTERINFO_SETSKIN(chick_setpain) (edict_t *self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;
	else
		self->s.skinnum &= ~1;
}

void chick_dead(edict_t *self)
{
	self->mins = { -16, -16, 0 };
	self->maxs = { 16, 16, 8 };
	monster_dead(self);
}

static void chick_shrink(edict_t *self)
{
	self->maxs[2] = 12;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

mframe_t chick_frames_death2[] = {
	{ ai_move, -6 },
	{ ai_move },
	{ ai_move, -1 },
	{ ai_move, -5, monster_footstep },
	{ ai_move },
	{ ai_move, -1 },
	{ ai_move, -2 },
	{ ai_move, 1 },
	{ ai_move, 10 },
	{ ai_move, 2 },
	{ ai_move, 3, monster_footstep },
	{ ai_move, 1 },
	{ ai_move, 2 },
	{ ai_move },
	{ ai_move, 3 },
	{ ai_move, 3 },
	{ ai_move, 1, monster_footstep },
	{ ai_move, -3 },
	{ ai_move, -5 },
	{ ai_move, 4 },
	{ ai_move, 15, chick_shrink },
	{ ai_move, 14, monster_footstep },
	{ ai_move, 1 }
};
MMOVE_T(chick_move_death2) = { FRAME_death201, FRAME_death223, chick_frames_death2, chick_dead };

mframe_t chick_frames_death1[] = {
	{ ai_move },
	{ ai_move, 0, monster_footstep },
	{ ai_move, -7 },
	{ ai_move, 4, monster_footstep },
	{ ai_move, 11, chick_shrink },
	{ ai_move },
	{ ai_move, 0, monster_footstep },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, monster_footstep },
	{ ai_move }
};
MMOVE_T(chick_move_death1) = { FRAME_death101, FRAME_death112, chick_frames_death1, chick_dead };

DIE(chick_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	int n;

	// check for gib
	if (M_CheckGib(self, mod))
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		self->s.skinnum /= 2;

		ThrowGibs(self, damage, {
			{ 2, "models/objects/gibs/bone/tris.md2" },
			{ 3, "models/objects/gibs/sm_meat/tris.md2" },
			{ "models/monsters/bitch/gibs/arm.md2", GIB_SKINNED | GIB_UPRIGHT },
			{ "models/monsters/bitch/gibs/foot.md2", GIB_SKINNED | GIB_UPRIGHT },
			{ "models/monsters/bitch/gibs/tube.md2", GIB_SKINNED | GIB_UPRIGHT },
			{ "models/monsters/bitch/gibs/chest.md2", GIB_SKINNED },
			{ "models/monsters/bitch/gibs/head.md2", GIB_HEAD | GIB_SKINNED }
		});
		self->deadflag = true;

		return;
	}

	if (self->deadflag)
		return;

	// regular death
	self->deadflag = true;
	self->takedamage = true;

	n = brandom();

	if (n == 0)
	{
		M_SetAnimation(self, &chick_move_death1);
		gi.sound(self, CHAN_VOICE, sound_death1, 1, ATTN_NORM, 0);
	}
	else
	{
		M_SetAnimation(self, &chick_move_death2);
		gi.sound(self, CHAN_VOICE, sound_death2, 1, ATTN_NORM, 0);
	}
}

// PMM - changes to duck code for new dodge

mframe_t chick_frames_duck[] = {
	{ ai_move, 0, monster_duck_down },
	{ ai_move, 1 },
	{ ai_move, 4, monster_duck_hold },
	{ ai_move, -4 },
	{ ai_move, -5, monster_duck_up },
	{ ai_move, 3 },
	{ ai_move, 1 }
};
MMOVE_T(chick_move_duck) = { FRAME_duck01, FRAME_duck07, chick_frames_duck, chick_run };

void ChickSlash(edict_t *self)
{
	vec3_t aim = { MELEE_DISTANCE, self->mins[0], 10 };
	gi.sound(self, CHAN_WEAPON, sound_melee_swing, 1, ATTN_NORM, 0);
	fire_hit(self, aim, irandom(10, 16), 100);
}

void ChickRocket(edict_t *self)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	dir;
	vec3_t	vec;
	trace_t trace; // PMM - check target
	int		rocketSpeed;
	// pmm - blindfire
	vec3_t target;
	bool   blindfire = false;

	if (self->monsterinfo.aiflags & AI_MANUAL_STEERING)
		blindfire = true;
	else
		blindfire = false;

	if (!self->enemy || !self->enemy->inuse) // PGM
		return;								 // PGM

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_CHICK_ROCKET_1], forward, right);
	
	// [Paril-KEX]
	if (self->s.skinnum > 1)
		rocketSpeed = 500;
	else
		rocketSpeed = 650;

	// PMM
	if (blindfire)
		target = self->monsterinfo.blind_fire_target;
	else
		target = self->enemy->s.origin;
	// pmm
	// PGM
	//  PMM - blindfire shooting
	if (blindfire)
	{
		vec = target;
		dir = vec - start;
	}
	// pmm
	// don't shoot at feet if they're above where i'm shooting from.
	else if (frandom() < 0.33f || (start[2] < self->enemy->absmin[2]))
	{
		vec = target;
		vec[2] += self->enemy->viewheight;
		dir = vec - start;
	}
	else
	{
		vec = target;
		vec[2] = self->enemy->absmin[2] + 1;
		dir = vec - start;
	}
	// PGM

	//======
	// PMM - lead target  (not when blindfiring)
	// 20, 35, 50, 65 chance of leading
	if ((!blindfire) && (frandom() < 0.35f))
		PredictAim(self, self->enemy, start, rocketSpeed, false, 0.f, &dir, &vec);
	// PMM - lead target
	//======

	dir.normalize();

	// pmm blindfire doesn't check target (done in checkattack)
	// paranoia, make sure we're not shooting a target right next to us
	trace = gi.traceline(start, vec, self, MASK_PROJECTILE);
	if (blindfire)
	{
		// blindfire has different fail criteria for the trace
		if (!(trace.startsolid || trace.allsolid || (trace.fraction < 0.5f)))
		{
			// RAFAEL
			if (self->s.skinnum > 1)
				monster_fire_heat(self, start, dir, 50, rocketSpeed, MZ2_CHICK_ROCKET_1, 0.075f);
			else
				// RAFAEL
				monster_fire_rocket(self, start, dir, 50, rocketSpeed, MZ2_CHICK_ROCKET_1);
		}
		else
		{
			// geez, this is bad.  she's avoiding about 80% of her blindfires due to hitting things.
			// hunt around for a good shot
			// try shifting the target to the left a little (to help counter her large offset)
			vec = target;
			vec += (right * -10);
			dir = vec - start;
			dir.normalize();
			trace = gi.traceline(start, vec, self, MASK_PROJECTILE);
			if (!(trace.startsolid || trace.allsolid || (trace.fraction < 0.5f)))
			{
				// RAFAEL
				if (self->s.skinnum > 1)
					monster_fire_heat(self, start, dir, 50, rocketSpeed, MZ2_CHICK_ROCKET_1, 0.075f);
				else
					// RAFAEL
					monster_fire_rocket(self, start, dir, 50, rocketSpeed, MZ2_CHICK_ROCKET_1);
			}
			else
			{
				// ok, that failed.  try to the right
				vec = target;
				vec += (right * 10);
				dir = vec - start;
				dir.normalize();
				trace = gi.traceline(start, vec, self, MASK_PROJECTILE);
				if (!(trace.startsolid || trace.allsolid || (trace.fraction < 0.5f)))
				{
					// RAFAEL
					if (self->s.skinnum > 1)
						monster_fire_heat(self, start, dir, 50, rocketSpeed, MZ2_CHICK_ROCKET_1, 0.075f);
					else
						// RAFAEL
						monster_fire_rocket(self, start, dir, 50, rocketSpeed, MZ2_CHICK_ROCKET_1);
				}
			}
		}
	}
	else
	{
		if (trace.fraction > 0.5f || trace.ent->solid != SOLID_BSP)
		{
			// RAFAEL
			if (self->s.skinnum > 1)
				monster_fire_heat(self, start, dir, 50, rocketSpeed, MZ2_CHICK_ROCKET_1, 0.15f);
			else
				// RAFAEL
				monster_fire_rocket(self, start, dir, 50, rocketSpeed, MZ2_CHICK_ROCKET_1);
		}
	}
}

void Chick_PreAttack1(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_missile_prelaunch, 1, ATTN_NORM, 0);

	if (self->monsterinfo.aiflags & AI_MANUAL_STEERING)
	{
		vec3_t aim = self->monsterinfo.blind_fire_target - self->s.origin;
		self->ideal_yaw = vectoyaw(aim);
	}
}

void ChickReload(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_missile_reload, 1, ATTN_NORM, 0);
}

mframe_t chick_frames_start_attack1[] = {
	{ ai_charge, 0, Chick_PreAttack1 },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 4 },
	{ ai_charge },
	{ ai_charge, -3 },
	{ ai_charge, 3 },
	{ ai_charge, 5 },
	{ ai_charge, 7, monster_footstep },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, chick_attack1 }
};
MMOVE_T(chick_move_start_attack1) = { FRAME_attak101, FRAME_attak113, chick_frames_start_attack1, nullptr };

mframe_t chick_frames_attack1[] = {
	{ ai_charge, 19, ChickRocket },
	{ ai_charge, -6, monster_footstep },
	{ ai_charge, -5 },
	{ ai_charge, -2 },
	{ ai_charge, -7, monster_footstep },
	{ ai_charge },
	{ ai_charge, 1 },
	{ ai_charge, 10, ChickReload },
	{ ai_charge, 4 },
	{ ai_charge, 5, monster_footstep },
	{ ai_charge, 6 },
	{ ai_charge, 6 },
	{ ai_charge, 4 },
	{ ai_charge, 3, [](edict_t *self) { chick_rerocket(self); monster_footstep(self); } }
};
MMOVE_T(chick_move_attack1) = { FRAME_attak114, FRAME_attak127, chick_frames_attack1, nullptr };

mframe_t chick_frames_end_attack1[] = {
	{ ai_charge, -3 },
	{ ai_charge },
	{ ai_charge, -6 },
	{ ai_charge, -4 },
	{ ai_charge, -2, monster_footstep }
};
MMOVE_T(chick_move_end_attack1) = { FRAME_attak128, FRAME_attak132, chick_frames_end_attack1, chick_run };

void chick_rerocket(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_MANUAL_STEERING)
	{
		self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
		M_SetAnimation(self, &chick_move_end_attack1);
		return;
	}

	if (!M_CheckClearShot(self, monster_flash_offset[MZ2_CHICK_ROCKET_1]))
	{
		M_SetAnimation(self, &chick_move_end_attack1);
		return;
	}

	if (self->enemy->health > 0)
	{
		if (range_to(self, self->enemy) > RANGE_MELEE)
			if (visible(self, self->enemy))
				if (frandom() <= 0.7f)
				{
					M_SetAnimation(self, &chick_move_attack1);
					return;
				}
	}
	M_SetAnimation(self, &chick_move_end_attack1);
}

void chick_attack1(edict_t *self)
{
	M_SetAnimation(self, &chick_move_attack1);
}

mframe_t chick_frames_slash[] = {
	{ ai_charge, 1 },
	{ ai_charge, 7, ChickSlash },
	{ ai_charge, -7, monster_footstep },
	{ ai_charge, 1 },
	{ ai_charge, -1 },
	{ ai_charge, 1 },
	{ ai_charge },
	{ ai_charge, 1 },
	{ ai_charge, -2, chick_reslash }
};
MMOVE_T(chick_move_slash) = { FRAME_attak204, FRAME_attak212, chick_frames_slash, nullptr };

mframe_t chick_frames_end_slash[] = {
	{ ai_charge, -6 },
	{ ai_charge, -1 },
	{ ai_charge, -6 },
	{ ai_charge, 0, monster_footstep }
};
MMOVE_T(chick_move_end_slash) = { FRAME_attak213, FRAME_attak216, chick_frames_end_slash, chick_run };

void chick_reslash(edict_t *self)
{
	if (self->enemy->health > 0)
	{
		if (range_to(self, self->enemy) <= RANGE_MELEE)
		{
			if (frandom() <= 0.9f)
			{
				M_SetAnimation(self, &chick_move_slash);
				return;
			}
			else
			{
				M_SetAnimation(self, &chick_move_end_slash);
				return;
			}
		}
	}
	M_SetAnimation(self, &chick_move_end_slash);
}

void chick_slash(edict_t *self)
{
	M_SetAnimation(self, &chick_move_slash);
}

mframe_t chick_frames_start_slash[] = {
	{ ai_charge, 1 },
	{ ai_charge, 8 },
	{ ai_charge, 3 }
};
MMOVE_T(chick_move_start_slash) = { FRAME_attak201, FRAME_attak203, chick_frames_start_slash, chick_slash };

MONSTERINFO_MELEE(chick_melee) (edict_t *self) -> void
{
	M_SetAnimation(self, &chick_move_start_slash);
}

MONSTERINFO_ATTACK(chick_attack) (edict_t *self) -> void
{
	if (!M_CheckClearShot(self, monster_flash_offset[MZ2_CHICK_ROCKET_1]))
		return;

	float r, chance;

	monster_done_dodge(self);

	// PMM
	if (self->monsterinfo.attack_state == AS_BLIND)
	{
		// setup shot probabilities
		if (self->monsterinfo.blind_fire_delay < 1.0_sec)
			chance = 1.0;
		else if (self->monsterinfo.blind_fire_delay < 7.5_sec)
			chance = 0.4f;
		else
			chance = 0.1f;

		r = frandom();

		// minimum of 5.5 seconds, plus 0-1, after the shots are done
		self->monsterinfo.blind_fire_delay += random_time(5.5_sec, 6.5_sec);

		// don't shoot at the origin
		if (!self->monsterinfo.blind_fire_target)
			return;

		// don't shoot if the dice say not to
		if (r > chance)
			return;

		// turn on manual steering to signal both manual steering and blindfire
		self->monsterinfo.aiflags |= AI_MANUAL_STEERING;
		M_SetAnimation(self, &chick_move_start_attack1);
		self->monsterinfo.attack_finished = level.time + random_time(2_sec);
		return;
	}
	// pmm

	M_SetAnimation(self, &chick_move_start_attack1);
}

MONSTERINFO_SIGHT(chick_sight) (edict_t *self, edict_t *other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

//===========
// PGM
MONSTERINFO_BLOCKED(chick_blocked) (edict_t *self, float dist) -> bool
{
	if (blocked_checkplat(self, dist))
		return true;

	return false;
}
// PGM
//===========

MONSTERINFO_DUCK(chick_duck) (edict_t *self, gtime_t eta) -> bool
{
	if ((self->monsterinfo.active_move == &chick_move_start_attack1) ||
		(self->monsterinfo.active_move == &chick_move_attack1))
	{
		// if we're shooting don't dodge
		self->monsterinfo.unduck(self);
		return false;
	}

	M_SetAnimation(self, &chick_move_duck);

	return true;
}

MONSTERINFO_SIDESTEP(chick_sidestep) (edict_t *self) -> bool
{
	if ((self->monsterinfo.active_move == &chick_move_start_attack1) ||
		(self->monsterinfo.active_move == &chick_move_attack1) ||
		(self->monsterinfo.active_move == &chick_move_pain3))
	{
		// if we're shooting, don't dodge
		return false;
	}

	if (self->monsterinfo.active_move != &chick_move_run)
		M_SetAnimation(self, &chick_move_run);

	return true;
}

/*QUAKED monster_chick (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
 */
void SP_monster_chick(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	sound_missile_prelaunch.assign("chick/chkatck1.wav");
	sound_missile_launch.assign("chick/chkatck2.wav");
	sound_melee_swing.assign("chick/chkatck3.wav");
	sound_melee_hit.assign("chick/chkatck4.wav");
	sound_missile_reload.assign("chick/chkatck5.wav");
	sound_death1.assign("chick/chkdeth1.wav");
	sound_death2.assign("chick/chkdeth2.wav");
	sound_fall_down.assign("chick/chkfall1.wav");
	sound_idle1.assign("chick/chkidle1.wav");
	sound_idle2.assign("chick/chkidle2.wav");
	sound_pain1.assign("chick/chkpain1.wav");
	sound_pain2.assign("chick/chkpain2.wav");
	sound_pain3.assign("chick/chkpain3.wav");
	sound_sight.assign("chick/chksght1.wav");
	sound_search.assign("chick/chksrch1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/bitch/tris.md2");
	
	gi.modelindex("models/monsters/bitch/gibs/arm.md2");
	gi.modelindex("models/monsters/bitch/gibs/chest.md2");
	gi.modelindex("models/monsters/bitch/gibs/foot.md2");
	gi.modelindex("models/monsters/bitch/gibs/head.md2");
	gi.modelindex("models/monsters/bitch/gibs/tube.md2");

	self->mins = { -16, -16, 0 };
	self->maxs = { 16, 16, 56 };

	self->health = 175 * st.health_multiplier;
	self->gib_health = -70;
	self->mass = 200;

	self->pain = chick_pain;
	self->die = chick_die;

	self->monsterinfo.stand = chick_stand;
	self->monsterinfo.walk = chick_walk;
	self->monsterinfo.run = chick_run;
	// pmm
	self->monsterinfo.dodge = M_MonsterDodge;
	self->monsterinfo.duck = chick_duck;
	self->monsterinfo.unduck = monster_duck_up;
	self->monsterinfo.sidestep = chick_sidestep;
	self->monsterinfo.blocked = chick_blocked; // PGM
	// pmm
	self->monsterinfo.attack = chick_attack;
	self->monsterinfo.melee = chick_melee;
	self->monsterinfo.sight = chick_sight;
	self->monsterinfo.setskin = chick_setpain;

	gi.linkentity(self);

	M_SetAnimation(self, &chick_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	// PMM
	self->monsterinfo.blindfire = true;
	// pmm
	walkmonster_start(self);
}

// RAFAEL
/*QUAKED monster_chick_heat (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
 */
void SP_monster_chick_heat(edict_t *self)
{
	SP_monster_chick(self);
	self->s.skinnum = 2;
	gi.soundindex("weapons/railgr1a.wav");
}
// RAFAEL
