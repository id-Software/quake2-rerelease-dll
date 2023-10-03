// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

Makron -- Final Boss

==============================================================================
*/

#include "g_local.h"
#include "m_boss32.h"
#include "m_flash.h"

void MakronRailgun(edict_t *self);
void MakronSaveloc(edict_t *self);
void MakronHyperblaster(edict_t *self);
void makron_step_left(edict_t *self);
void makron_step_right(edict_t *self);
void makronBFG(edict_t *self);
void makron_dead(edict_t *self);

static cached_soundindex sound_pain4;
static cached_soundindex sound_pain5;
static cached_soundindex sound_pain6;
static cached_soundindex sound_death;
static cached_soundindex sound_step_left;
static cached_soundindex sound_step_right;
static cached_soundindex sound_attack_bfg;
static cached_soundindex sound_brainsplorch;
static cached_soundindex sound_prerailgun;
static cached_soundindex sound_popup;
static cached_soundindex sound_taunt1;
static cached_soundindex sound_taunt2;
static cached_soundindex sound_taunt3;
static cached_soundindex sound_hit;

void makron_taunt(edict_t *self)
{
	float r;

	r = frandom();
	if (r <= 0.3f)
		gi.sound(self, CHAN_AUTO, sound_taunt1, 1, ATTN_NONE, 0);
	else if (r <= 0.6f)
		gi.sound(self, CHAN_AUTO, sound_taunt2, 1, ATTN_NONE, 0);
	else
		gi.sound(self, CHAN_AUTO, sound_taunt3, 1, ATTN_NONE, 0);
}

//
// stand
//

mframe_t makron_frames_stand[] = {
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }, // 10
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }, // 20
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }, // 30
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }, // 40
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }, // 50
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand } // 60
};
MMOVE_T(makron_move_stand) = { FRAME_stand201, FRAME_stand260, makron_frames_stand, nullptr };

MONSTERINFO_STAND(makron_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &makron_move_stand);
}

mframe_t makron_frames_run[] = {
	{ ai_run, 3, makron_step_left },
	{ ai_run, 12 },
	{ ai_run, 8 },
	{ ai_run, 8 },
	{ ai_run, 8, makron_step_right },
	{ ai_run, 6 },
	{ ai_run, 12 },
	{ ai_run, 9 },
	{ ai_run, 6 },
	{ ai_run, 12 }
};
MMOVE_T(makron_move_run) = { FRAME_walk204, FRAME_walk213, makron_frames_run, nullptr };

void makron_hit(edict_t *self)
{
	gi.sound(self, CHAN_AUTO, sound_hit, 1, ATTN_NONE, 0);
}

void makron_popup(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_popup, 1, ATTN_NONE, 0);
}

void makron_step_left(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_step_left, 1, ATTN_NORM, 0);
}

void makron_step_right(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_step_right, 1, ATTN_NORM, 0);
}

void makron_brainsplorch(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_brainsplorch, 1, ATTN_NORM, 0);
}

void makron_prerailgun(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_prerailgun, 1, ATTN_NORM, 0);
}

mframe_t makron_frames_walk[] = {
	{ ai_walk, 3, makron_step_left },
	{ ai_walk, 12 },
	{ ai_walk, 8 },
	{ ai_walk, 8 },
	{ ai_walk, 8, makron_step_right },
	{ ai_walk, 6 },
	{ ai_walk, 12 },
	{ ai_walk, 9 },
	{ ai_walk, 6 },
	{ ai_walk, 12 }
};
MMOVE_T(makron_move_walk) = { FRAME_walk204, FRAME_walk213, makron_frames_run, nullptr };

MONSTERINFO_WALK(makron_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &makron_move_walk);
}

MONSTERINFO_RUN(makron_run) (edict_t *self) -> void
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		M_SetAnimation(self, &makron_move_stand);
	else
		M_SetAnimation(self, &makron_move_run);
}

mframe_t makron_frames_pain6[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }, // 10
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, makron_popup },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }, // 20
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, makron_taunt },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(makron_move_pain6) = { FRAME_pain601, FRAME_pain627, makron_frames_pain6, makron_run };

mframe_t makron_frames_pain5[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(makron_move_pain5) = { FRAME_pain501, FRAME_pain504, makron_frames_pain5, makron_run };

mframe_t makron_frames_pain4[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(makron_move_pain4) = { FRAME_pain401, FRAME_pain404, makron_frames_pain4, makron_run };

/*
---
Makron Torso. This needs to be spawned in
---
*/

THINK(makron_torso_think) (edict_t *self) -> void
{
	if (++self->s.frame >= 365)
		self->s.frame = 346;
	
	self->nextthink = level.time + 10_hz;

	if (self->s.angles[0] > 0)
		self->s.angles[0] = max(0.f, self->s.angles[0] - 15);
}

void makron_torso(edict_t *ent)
{
	ent->s.frame = 346;
	ent->s.modelindex = gi.modelindex("models/monsters/boss3/rider/tris.md2");
	ent->s.skinnum = 1;
	ent->think = makron_torso_think;
	ent->nextthink = level.time + 10_hz;
	ent->s.sound = gi.soundindex("makron/spine.wav");
	ent->movetype = MOVETYPE_TOSS;
	ent->s.effects = EF_GIB;
	vec3_t forward, up;
	AngleVectors(ent->s.angles, forward, nullptr, up);
	ent->velocity += (up * 120);
	ent->velocity += (forward * -120);
	ent->s.origin += (forward * -10);
	ent->s.angles[0] = 90;
	ent->avelocity = {};
	gi.linkentity(ent);
}

void makron_spawn_torso(edict_t *self)
{
	edict_t *tempent = ThrowGib(self, "models/monsters/boss3/rider/tris.md2", 0, GIB_NONE, self->s.scale);
	tempent->s.origin = self->s.origin;
	tempent->s.angles = self->s.angles;
	self->maxs[2] -= tempent->maxs[2];
	tempent->s.origin[2] += self->maxs[2] - 15;
	makron_torso(tempent);
}

mframe_t makron_frames_death2[] = {
	{ ai_move, -15 },
	{ ai_move, 3 },
	{ ai_move, -12 },
	{ ai_move, 0, makron_step_left },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }, // 10
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 11 },
	{ ai_move, 12 },
	{ ai_move, 11, makron_step_right },
	{ ai_move },
	{ ai_move }, // 20
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }, // 30
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 5 },
	{ ai_move, 7 },
	{ ai_move, 6, makron_step_left },
	{ ai_move },
	{ ai_move },
	{ ai_move, -1 },
	{ ai_move, 2 }, // 40
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }, // 50
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, -6 },
	{ ai_move, -4 },
	{ ai_move, -6, makron_step_right },
	{ ai_move, -4 },
	{ ai_move, -4, makron_step_left },
	{ ai_move },
	{ ai_move }, // 60
	{ ai_move },
	{ ai_move },
	{ ai_move, -2 },
	{ ai_move, -5 },
	{ ai_move, -3, makron_step_right },
	{ ai_move, -8 },
	{ ai_move, -3, makron_step_left },
	{ ai_move, -7 },
	{ ai_move, -4 },
	{ ai_move, -4, makron_step_right }, // 70
	{ ai_move, -6 },
	{ ai_move, -7 },
	{ ai_move, 0, makron_step_left },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }, // 80
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, -2 },
	{ ai_move },
	{ ai_move },
	{ ai_move, 2 },
	{ ai_move }, // 90
	{ ai_move, 27, makron_hit },
	{ ai_move, 26 },
	{ ai_move, 0, makron_brainsplorch },
	{ ai_move },
	{ ai_move } // 95
};
MMOVE_T(makron_move_death2) = { FRAME_death201, FRAME_death295, makron_frames_death2, makron_dead };

#if 0
mframe_t makron_frames_death3[] = {
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
MMOVE_T(makron_move_death3) = { FRAME_death301, FRAME_death320, makron_frames_death3, nullptr };
#endif

mframe_t makron_frames_sight[] = {
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
MMOVE_T(makron_move_sight) = { FRAME_active01, FRAME_active13, makron_frames_sight, makron_run };

void makronBFG(edict_t *self)
{
	vec3_t forward, right;
	vec3_t start;
	vec3_t dir;
	vec3_t vec;

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_MAKRON_BFG], forward, right);

	vec = self->enemy->s.origin;
	vec[2] += self->enemy->viewheight;
	dir = vec - start;
	dir.normalize();
	gi.sound(self, CHAN_VOICE, sound_attack_bfg, 1, ATTN_NORM, 0);
	monster_fire_bfg(self, start, dir, 50, 300, 100, 300, MZ2_MAKRON_BFG);
}

mframe_t makron_frames_attack3[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, makronBFG },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(makron_move_attack3) = { FRAME_attak301, FRAME_attak308, makron_frames_attack3, makron_run };

mframe_t makron_frames_attack4[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_move, 0, MakronHyperblaster }, // fire
	{ ai_move, 0, MakronHyperblaster }, // fire
	{ ai_move, 0, MakronHyperblaster }, // fire
	{ ai_move, 0, MakronHyperblaster }, // fire
	{ ai_move, 0, MakronHyperblaster }, // fire
	{ ai_move, 0, MakronHyperblaster }, // fire
	{ ai_move, 0, MakronHyperblaster }, // fire
	{ ai_move, 0, MakronHyperblaster }, // fire
	{ ai_move, 0, MakronHyperblaster }, // fire
	{ ai_move, 0, MakronHyperblaster }, // fire
	{ ai_move, 0, MakronHyperblaster }, // fire
	{ ai_move, 0, MakronHyperblaster }, // fire
	{ ai_move, 0, MakronHyperblaster }, // fire
	{ ai_move, 0, MakronHyperblaster }, // fire
	{ ai_move, 0, MakronHyperblaster }, // fire
	{ ai_move, 0, MakronHyperblaster }, // fire
	{ ai_move, 0, MakronHyperblaster }, // fire
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(makron_move_attack4) = { FRAME_attak401, FRAME_attak426, makron_frames_attack4, makron_run };

mframe_t makron_frames_attack5[] = {
	{ ai_charge, 0, makron_prerailgun },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, MakronSaveloc },
	{ ai_move, 0, MakronRailgun }, // Fire railgun
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(makron_move_attack5) = { FRAME_attak501, FRAME_attak516, makron_frames_attack5, makron_run };

void MakronSaveloc(edict_t *self)
{
	self->pos1 = self->enemy->s.origin; // save for aiming the shot
	self->pos1[2] += self->enemy->viewheight;
};

void MakronRailgun(edict_t *self)
{
	vec3_t start;
	vec3_t dir;
	vec3_t forward, right;

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_MAKRON_RAILGUN_1], forward, right);

	// calc direction to where we targted
	dir = self->pos1 - start;
	dir.normalize();

	monster_fire_railgun(self, start, dir, 50, 100, MZ2_MAKRON_RAILGUN_1);
}

void MakronHyperblaster(edict_t *self)
{
	vec3_t dir;
	vec3_t vec;
	vec3_t start;
	vec3_t forward, right;

	monster_muzzleflash_id_t flash_number = (monster_muzzleflash_id_t) (MZ2_MAKRON_BLASTER_1 + (self->s.frame - FRAME_attak405));

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, monster_flash_offset[flash_number], forward, right);

	if (self->enemy)
	{
		vec = self->enemy->s.origin;
		vec[2] += self->enemy->viewheight;
		vec -= start;
		vec = vectoangles(vec);
		dir[0] = vec[0];
	}
	else
	{
		dir[0] = 0;
	}
	if (self->s.frame <= FRAME_attak413)
		dir[1] = self->s.angles[1] - 10 * (self->s.frame - FRAME_attak413);
	else
		dir[1] = self->s.angles[1] + 10 * (self->s.frame - FRAME_attak421);
	dir[2] = 0;

	AngleVectors(dir, forward, nullptr, nullptr);

	monster_fire_blaster(self, start, forward, 15, 1000, flash_number, EF_BLASTER);
}

PAIN(makron_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	if (self->monsterinfo.active_move == &makron_move_sight)
		return;

	if (level.time < self->pain_debounce_time)
		return;

	// Lessen the chance of him going into his pain frames
	if (mod.id != MOD_CHAINFIST && damage <= 25)
		if (frandom() < 0.2f)
			return;

	self->pain_debounce_time = level.time + 3_sec;

	bool do_pain6 = false;

	if (damage <= 40)
		gi.sound(self, CHAN_VOICE, sound_pain4, 1, ATTN_NONE, 0);
	else if (damage <= 110)
		gi.sound(self, CHAN_VOICE, sound_pain5, 1, ATTN_NONE, 0);
	else
	{
		if (damage <= 150)
		{
			if (frandom() <= 0.45f)
			{
				do_pain6 = true;
				gi.sound(self, CHAN_VOICE, sound_pain6, 1, ATTN_NONE, 0);
			}
		}
		else
		{
			if (frandom() <= 0.35f)
			{
				do_pain6 = true;
				gi.sound(self, CHAN_VOICE, sound_pain6, 1, ATTN_NONE, 0);
			}
		}
	}
	
	if (!M_ShouldReactToPain(self, mod))
		return; // no pain anims in nightmare

	if (damage <= 40)
		M_SetAnimation(self, &makron_move_pain4);
	else if (damage <= 110)
		M_SetAnimation(self, &makron_move_pain5);
	else if (do_pain6)
		M_SetAnimation(self, &makron_move_pain6);
}

MONSTERINFO_SETSKIN(makron_setskin) (edict_t *self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

MONSTERINFO_SIGHT(makron_sight) (edict_t *self, edict_t *other) -> void
{
	M_SetAnimation(self, &makron_move_sight);
}

MONSTERINFO_ATTACK(makron_attack) (edict_t *self) -> void
{
	float r;

	r = frandom();

	if (r <= 0.3f)
		M_SetAnimation(self, &makron_move_attack3);
	else if (r <= 0.6f)
		M_SetAnimation(self, &makron_move_attack4);
	else
		M_SetAnimation(self, &makron_move_attack5);
}

//
// death
//

void makron_dead(edict_t *self)
{
	self->mins = { -60, -60, 0 };
	self->maxs = { 60, 60, 24 };
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
	monster_dead(self);
}

DIE(makron_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	self->s.sound = 0;

	// check for gib
	if (M_CheckGib(self, mod))
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		ThrowGibs(self, damage, {
			{ "models/objects/gibs/sm_meat/tris.md2" },
			{ 4, "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC },
			{ "models/objects/gibs/gear/tris.md2", GIB_METALLIC | GIB_HEAD }
		});
		self->deadflag = true;
		return;
	}

	if (self->deadflag)
		return;

	// regular death
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NONE, 0);
	self->deadflag = true;
	self->takedamage = true;
	self->svflags |= SVF_DEADMONSTER;

	M_SetAnimation(self, &makron_move_death2);

	makron_spawn_torso(self);

	self->mins = { -60, -60, 0 };
	self->maxs = { 60, 60, 48 };
}

// [Paril-KEX] use generic function
MONSTERINFO_CHECKATTACK(Makron_CheckAttack) (edict_t *self) -> bool
{
	return M_CheckAttack_Base(self, 0.4f, 0.8f, 0.4f, 0.2f, 0.0f, 0.f);
}

//
// monster_makron
//

void MakronPrecache()
{
	sound_pain4.assign("makron/pain3.wav");
	sound_pain5.assign("makron/pain2.wav");
	sound_pain6.assign("makron/pain1.wav");
	sound_death.assign("makron/death.wav");
	sound_step_left.assign("makron/step1.wav");
	sound_step_right.assign("makron/step2.wav");
	sound_attack_bfg.assign("makron/bfg_fire.wav");
	sound_brainsplorch.assign("makron/brain1.wav");
	sound_prerailgun.assign("makron/rail_up.wav");
	sound_popup.assign("makron/popup.wav");
	sound_taunt1.assign("makron/voice4.wav");
	sound_taunt2.assign("makron/voice3.wav");
	sound_taunt3.assign("makron/voice.wav");
	sound_hit.assign("makron/bhit.wav");

	gi.modelindex("models/monsters/boss3/rider/tris.md2");
}

/*QUAKED monster_makron (1 .5 0) (-30 -30 0) (30 30 90) Ambush Trigger_Spawn Sight
 */
void SP_monster_makron(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	MakronPrecache();

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/boss3/rider/tris.md2");
	self->mins = { -30, -30, 0 };
	self->maxs = { 30, 30, 90 };

	self->health = 3000 * st.health_multiplier;
	self->gib_health = -2000;
	self->mass = 500;

	self->pain = makron_pain;
	self->die = makron_die;
	self->monsterinfo.stand = makron_stand;
	self->monsterinfo.walk = makron_walk;
	self->monsterinfo.run = makron_run;
	self->monsterinfo.dodge = nullptr;
	self->monsterinfo.attack = makron_attack;
	self->monsterinfo.melee = nullptr;
	self->monsterinfo.sight = makron_sight;
	self->monsterinfo.checkattack = Makron_CheckAttack;
	self->monsterinfo.setskin = makron_setskin;

	gi.linkentity(self);

	//	M_SetAnimation(self, &makron_move_stand);
	M_SetAnimation(self, &makron_move_sight);
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);

	// PMM
	self->monsterinfo.aiflags |= AI_IGNORE_SHOTS;
	// pmm
}

/*
=================
MakronSpawn

=================
*/
THINK(MakronSpawn) (edict_t *self) -> void
{
	vec3_t	 vec;
	edict_t *player;

	SP_monster_makron(self);
	self->think(self);

	// jump at player
	if (self->enemy && self->enemy->inuse && self->enemy->health > 0)
		player = self->enemy;
	else
		player = AI_GetSightClient(self);

	if (!player)
		return;

	vec = player->s.origin - self->s.origin;
	self->s.angles[YAW] = vectoyaw(vec);
	vec.normalize();
	self->velocity = vec * 400;
	self->velocity[2] = 200;
	self->groundentity = nullptr;
	self->enemy = player;
	FoundTarget(self);
	self->monsterinfo.sight(self, self->enemy);
	self->s.frame = self->monsterinfo.nextframe = FRAME_active01; // FIXME: why????
}

/*
=================
MakronToss

Jorg is just about dead, so set up to launch Makron out
=================
*/
void MakronToss(edict_t *self)
{
	edict_t *ent = G_Spawn();
	ent->classname = "monster_makron";
	ent->target = self->target;
	ent->s.origin = self->s.origin;
	ent->enemy = self->enemy;

	MakronSpawn(ent);

	// [Paril-KEX] set health bar over to Makron when we throw him out
	for (size_t i = 0; i < 2; i++)
		if (level.health_bar_entities[i] && level.health_bar_entities[i]->enemy == self)
			level.health_bar_entities[i]->enemy = ent;
}
