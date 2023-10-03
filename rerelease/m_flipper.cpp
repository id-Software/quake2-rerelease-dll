// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

FLIPPER

==============================================================================
*/

#include "g_local.h"
#include "m_flipper.h"

static cached_soundindex sound_chomp;
static cached_soundindex sound_attack;
static cached_soundindex sound_pain1;
static cached_soundindex sound_pain2;
static cached_soundindex sound_death;
static cached_soundindex sound_idle;
static cached_soundindex sound_search;
static cached_soundindex sound_sight;

mframe_t flipper_frames_stand[] = {
	{ ai_stand }
};

MMOVE_T(flipper_move_stand) = { FRAME_flphor01, FRAME_flphor01, flipper_frames_stand, nullptr };

MONSTERINFO_STAND(flipper_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &flipper_move_stand);
}

constexpr float FLIPPER_RUN_SPEED = 24;

mframe_t flipper_frames_run[] = {
	{ ai_run, FLIPPER_RUN_SPEED }, // 6
	{ ai_run, FLIPPER_RUN_SPEED },
	{ ai_run, FLIPPER_RUN_SPEED },
	{ ai_run, FLIPPER_RUN_SPEED },
	{ ai_run, FLIPPER_RUN_SPEED }, // 10

	{ ai_run, FLIPPER_RUN_SPEED },
	{ ai_run, FLIPPER_RUN_SPEED },
	{ ai_run, FLIPPER_RUN_SPEED },
	{ ai_run, FLIPPER_RUN_SPEED },
	{ ai_run, FLIPPER_RUN_SPEED },
	{ ai_run, FLIPPER_RUN_SPEED },
	{ ai_run, FLIPPER_RUN_SPEED },
	{ ai_run, FLIPPER_RUN_SPEED },
	{ ai_run, FLIPPER_RUN_SPEED },
	{ ai_run, FLIPPER_RUN_SPEED }, // 20

	{ ai_run, FLIPPER_RUN_SPEED },
	{ ai_run, FLIPPER_RUN_SPEED },
	{ ai_run, FLIPPER_RUN_SPEED },
	{ ai_run, FLIPPER_RUN_SPEED },
	{ ai_run, FLIPPER_RUN_SPEED },
	{ ai_run, FLIPPER_RUN_SPEED },
	{ ai_run, FLIPPER_RUN_SPEED },
	{ ai_run, FLIPPER_RUN_SPEED },
	{ ai_run, FLIPPER_RUN_SPEED } // 29
};
MMOVE_T(flipper_move_run_loop) = { FRAME_flpver06, FRAME_flpver29, flipper_frames_run, nullptr };

void flipper_run_loop(edict_t *self)
{
	M_SetAnimation(self, &flipper_move_run_loop);
}

mframe_t flipper_frames_run_start[] = {
	{ ai_run, 8 },
	{ ai_run, 8 },
	{ ai_run, 8 },
	{ ai_run, 8 },
	{ ai_run, 8 },
	{ ai_run, 8 }
};
MMOVE_T(flipper_move_run_start) = { FRAME_flpver01, FRAME_flpver06, flipper_frames_run_start, flipper_run_loop };

void flipper_run(edict_t *self)
{
	M_SetAnimation(self, &flipper_move_run_start);
}

/* Standard Swimming */
mframe_t flipper_frames_walk[] = {
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
MMOVE_T(flipper_move_walk) = { FRAME_flphor01, FRAME_flphor24, flipper_frames_walk, nullptr };

MONSTERINFO_WALK(flipper_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &flipper_move_walk);
}

mframe_t flipper_frames_start_run[] = {
	{ ai_run },
	{ ai_run },
	{ ai_run },
	{ ai_run },
	{ ai_run, 8, flipper_run }
};
MMOVE_T(flipper_move_start_run) = { FRAME_flphor01, FRAME_flphor05, flipper_frames_start_run, nullptr };

MONSTERINFO_RUN(flipper_start_run) (edict_t *self) -> void
{
	M_SetAnimation(self, &flipper_move_start_run);
}

mframe_t flipper_frames_pain2[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(flipper_move_pain2) = { FRAME_flppn101, FRAME_flppn105, flipper_frames_pain2, flipper_run };

mframe_t flipper_frames_pain1[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(flipper_move_pain1) = { FRAME_flppn201, FRAME_flppn205, flipper_frames_pain1, flipper_run };

void flipper_bite(edict_t *self)
{
	vec3_t aim = { MELEE_DISTANCE, 0, 0 };
	fire_hit(self, aim, 5, 0);
}

void flipper_preattack(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_chomp, 1, ATTN_NORM, 0);
}

mframe_t flipper_frames_attack[] = {
	{ ai_charge, 0, flipper_preattack },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, flipper_bite },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, flipper_bite },
	{ ai_charge }
};
MMOVE_T(flipper_move_attack) = { FRAME_flpbit01, FRAME_flpbit20, flipper_frames_attack, flipper_run };

MONSTERINFO_MELEE(flipper_melee) (edict_t *self) -> void
{
	M_SetAnimation(self, &flipper_move_attack);
}

PAIN(flipper_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	int n;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3_sec;
	n = brandom();

	if (n == 0)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

	if (!M_ShouldReactToPain(self, mod))
		return; // no pain anims in nightmare

	if (n == 0)
		M_SetAnimation(self, &flipper_move_pain1);
	else
		M_SetAnimation(self, &flipper_move_pain2);
}

MONSTERINFO_SETSKIN(flipper_setskin) (edict_t *self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

void flipper_dead(edict_t *self)
{
	self->mins = { -16, -16, -8 };
	self->maxs = { 16, 16, 8 };
	monster_dead(self);
}

mframe_t flipper_frames_death[] = {
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
MMOVE_T(flipper_move_death) = { FRAME_flpdth01, FRAME_flpdth56, flipper_frames_death, flipper_dead };

MONSTERINFO_SIGHT(flipper_sight) (edict_t *self, edict_t *other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

DIE(flipper_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	// check for gib
	if (M_CheckGib(self, mod))
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		ThrowGibs(self, damage, {
			{ 2, "models/objects/gibs/bone/tris.md2" },
			{ 2, "models/objects/gibs/sm_meat/tris.md2" },
			{ "models/objects/gibs/head2/tris.md2", GIB_HEAD }
		});
		self->deadflag = true;
		return;
	}

	if (self->deadflag)
		return;

	// regular death
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->deadflag = true;
	self->takedamage = true;
	self->svflags |= SVF_DEADMONSTER;
	M_SetAnimation(self, &flipper_move_death);
}

static void flipper_set_fly_parameters(edict_t *self)
{
	self->monsterinfo.fly_thrusters = false;
	self->monsterinfo.fly_acceleration = 30.f;
	self->monsterinfo.fly_speed = 110.f;
	// only melee, so get in close
	self->monsterinfo.fly_min_distance = 10.f;
	self->monsterinfo.fly_max_distance = 10.f;
}

/*QUAKED monster_flipper (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
 */
void SP_monster_flipper(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	sound_pain1.assign("flipper/flppain1.wav");
	sound_pain2.assign("flipper/flppain2.wav");
	sound_death.assign("flipper/flpdeth1.wav");
	sound_chomp.assign("flipper/flpatck1.wav");
	sound_attack.assign("flipper/flpatck2.wav");
	sound_idle.assign("flipper/flpidle1.wav");
	sound_search.assign("flipper/flpsrch1.wav");
	sound_sight.assign("flipper/flpsght1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/flipper/tris.md2");
	self->mins = { -16, -16, -8 };
	self->maxs = { 16, 16, 20 };

	self->health = 50 * st.health_multiplier;
	self->gib_health = -30;
	self->mass = 100;

	self->pain = flipper_pain;
	self->die = flipper_die;

	self->monsterinfo.stand = flipper_stand;
	self->monsterinfo.walk = flipper_walk;
	self->monsterinfo.run = flipper_start_run;
	self->monsterinfo.melee = flipper_melee;
	self->monsterinfo.sight = flipper_sight;
	self->monsterinfo.setskin = flipper_setskin;

	gi.linkentity(self);

	M_SetAnimation(self, &flipper_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	self->monsterinfo.aiflags |= AI_ALTERNATE_FLY;
	flipper_set_fly_parameters(self);

	swimmonster_start(self);
}
