// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

boss2

==============================================================================
*/

#include "g_local.h"
#include "m_boss2.h"
#include "m_flash.h"

// [Paril-KEX]
constexpr spawnflags_t SPAWNFLAG_BOSS2_N64 = 8_spawnflag;

bool infront(edict_t *self, edict_t *other);

static cached_soundindex sound_pain1;
static cached_soundindex sound_pain2;
static cached_soundindex sound_pain3;
static cached_soundindex sound_death;
static cached_soundindex sound_search1;

MONSTERINFO_SEARCH(boss2_search) (edict_t *self) -> void
{
	if (frandom() < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_search1, 1, ATTN_NONE, 0);
}

void boss2_run(edict_t *self);
void boss2_dead(edict_t *self);
void boss2_attack_mg(edict_t *self);
void boss2_reattack_mg(edict_t *self);

constexpr int32_t BOSS2_ROCKET_SPEED = 750;

void Boss2PredictiveRocket(edict_t *self)
{
	vec3_t forward, right;
	vec3_t start;
	vec3_t dir;

	AngleVectors(self->s.angles, forward, right, nullptr);

	// 1
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_BOSS2_ROCKET_1], forward, right);
	PredictAim(self, self->enemy, start, BOSS2_ROCKET_SPEED, false, -0.10f, &dir, nullptr);
	monster_fire_rocket(self, start, dir, 50, BOSS2_ROCKET_SPEED, MZ2_BOSS2_ROCKET_1);

	// 2
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_BOSS2_ROCKET_2], forward, right);
	PredictAim(self, self->enemy, start, BOSS2_ROCKET_SPEED, false, -0.05f, &dir, nullptr);
	monster_fire_rocket(self, start, dir, 50, BOSS2_ROCKET_SPEED, MZ2_BOSS2_ROCKET_2);

	// 3
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_BOSS2_ROCKET_3], forward, right);
	PredictAim(self, self->enemy, start, BOSS2_ROCKET_SPEED, false, 0.05f, &dir, nullptr);
	monster_fire_rocket(self, start, dir, 50, BOSS2_ROCKET_SPEED, MZ2_BOSS2_ROCKET_3);

	// 4
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_BOSS2_ROCKET_4], forward, right);
	PredictAim(self, self->enemy, start, BOSS2_ROCKET_SPEED, false, 0.10f, &dir, nullptr);
	monster_fire_rocket(self, start, dir, 50, BOSS2_ROCKET_SPEED, MZ2_BOSS2_ROCKET_4);
}

void Boss2Rocket(edict_t *self)
{
	vec3_t forward, right;
	vec3_t start;
	vec3_t dir;
	vec3_t vec;

	if (self->enemy)
	{
		if (self->enemy->client && frandom() < 0.9f)
		{
			Boss2PredictiveRocket(self);
			return;
		}
	}

	AngleVectors(self->s.angles, forward, right, nullptr);

	// 1
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_BOSS2_ROCKET_1], forward, right);
	vec = self->enemy->s.origin;
	vec[2] -= 15;
	dir = vec - start;
	dir.normalize();
	dir += (right * 0.4f);
	dir.normalize();
	monster_fire_rocket(self, start, dir, 50, 500, MZ2_BOSS2_ROCKET_1);

	// 2
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_BOSS2_ROCKET_2], forward, right);
	vec = self->enemy->s.origin;
	dir = vec - start;
	dir.normalize();
	dir += (right * 0.025f);
	dir.normalize();
	monster_fire_rocket(self, start, dir, 50, 500, MZ2_BOSS2_ROCKET_2);

	// 3
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_BOSS2_ROCKET_3], forward, right);
	vec = self->enemy->s.origin;
	dir = vec - start;
	dir.normalize();
	dir += (right * -0.025f);
	dir.normalize();
	monster_fire_rocket(self, start, dir, 50, 500, MZ2_BOSS2_ROCKET_3);

	// 4
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_BOSS2_ROCKET_4], forward, right);
	vec = self->enemy->s.origin;
	vec[2] -= 15;
	dir = vec - start;
	dir.normalize();
	dir += (right * -0.4f);
	dir.normalize();
	monster_fire_rocket(self, start, dir, 50, 500, MZ2_BOSS2_ROCKET_4);
}

void Boss2Rocket64(edict_t *self)
{
	vec3_t forward, right;
	vec3_t start;
	vec3_t dir;
	vec3_t vec;
	float  time, dist;

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_BOSS2_ROCKET_1], forward, right);

	float scale = self->s.scale ? self->s.scale : 1;

	start[2] += 10.f * scale;
	start -= right * 2.f * scale;
	start -= right * ((self->count++ % 4) * 8.f * scale);
	
	if (self->enemy && self->enemy->client && frandom() < 0.9f)
	{
		// 1
		dir = self->enemy->s.origin - start;
		dist = dir.length();
		time = dist / BOSS2_ROCKET_SPEED;
		vec = self->enemy->s.origin + (self->enemy->velocity * (time - 0.3f));
	}
	else
	{
		// 1
		vec = self->enemy->s.origin;
		vec[2] -= 15;
	}
	
	dir = vec - start;
	dir.normalize();

	monster_fire_rocket(self, start, dir, 35, BOSS2_ROCKET_SPEED, MZ2_BOSS2_ROCKET_1);
}

void boss2_firebullet_right(edict_t *self)
{
	vec3_t forward, right, start;
	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_BOSS2_MACHINEGUN_R1], forward, right);
	PredictAim(self, self->enemy, start, 0, true, -0.2f, &forward, nullptr);
	monster_fire_bullet(self, start, forward, 6, 4, DEFAULT_BULLET_HSPREAD * 3, DEFAULT_BULLET_VSPREAD, MZ2_BOSS2_MACHINEGUN_R1);
}

void boss2_firebullet_left(edict_t *self)
{
	vec3_t forward, right, start;
	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_BOSS2_MACHINEGUN_L1], forward, right);
	PredictAim(self, self->enemy, start, 0, true, -0.2f, &forward, nullptr);
	monster_fire_bullet(self, start, forward, 6, 4, DEFAULT_BULLET_HSPREAD * 3, DEFAULT_BULLET_VSPREAD, MZ2_BOSS2_MACHINEGUN_L1);
}

void Boss2MachineGun(edict_t *self)
{
	boss2_firebullet_left(self);
	boss2_firebullet_right(self);
}

mframe_t boss2_frames_stand[] = {
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
MMOVE_T(boss2_move_stand) = { FRAME_stand30, FRAME_stand50, boss2_frames_stand, nullptr };

mframe_t boss2_frames_walk[] = {
	{ ai_walk, 10 },
	{ ai_walk, 10 },
	{ ai_walk, 10 },
	{ ai_walk, 10 },
	{ ai_walk, 10 },
	{ ai_walk, 10 },
	{ ai_walk, 10 },
	{ ai_walk, 10 },
	{ ai_walk, 10 },
	{ ai_walk, 10 },
	{ ai_walk, 10 },
	{ ai_walk, 10 },
	{ ai_walk, 10 },
	{ ai_walk, 10 },
	{ ai_walk, 10 },
	{ ai_walk, 10 },
	{ ai_walk, 10 },
	{ ai_walk, 10 },
	{ ai_walk, 10 },
	{ ai_walk, 10 }
};
MMOVE_T(boss2_move_walk) = { FRAME_walk1, FRAME_walk20, boss2_frames_walk, nullptr };

mframe_t boss2_frames_run[] = {
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 },
	{ ai_run, 10 }
};
MMOVE_T(boss2_move_run) = { FRAME_walk1, FRAME_walk20, boss2_frames_run, nullptr };

mframe_t boss2_frames_attack_pre_mg[] = {
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2, boss2_attack_mg }
};
MMOVE_T(boss2_move_attack_pre_mg) = { FRAME_attack1, FRAME_attack9, boss2_frames_attack_pre_mg, nullptr };

// Loop this
mframe_t boss2_frames_attack_mg[] = {
	{ ai_charge, 2, Boss2MachineGun },
	{ ai_charge, 2, Boss2MachineGun },
	{ ai_charge, 2, Boss2MachineGun },
	{ ai_charge, 2, Boss2MachineGun },
	{ ai_charge, 2, Boss2MachineGun },
	{ ai_charge, 2, boss2_reattack_mg }
};
MMOVE_T(boss2_move_attack_mg) = { FRAME_attack10, FRAME_attack15, boss2_frames_attack_mg, nullptr };

// [Paril-KEX]
void Boss2HyperBlaster(edict_t *self)
{
	vec3_t forward, right, target;
	vec3_t start;
	monster_muzzleflash_id_t id = (self->s.frame & 1) ? MZ2_BOSS2_MACHINEGUN_L2 : MZ2_BOSS2_MACHINEGUN_R2;

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, monster_flash_offset[id], forward, right);
	target = self->enemy->s.origin;
	target[2] += self->enemy->viewheight;
	forward = target - start;
	forward.normalize();

	monster_fire_blaster(self, start, forward, 2, 1000, id, (self->s.frame % 4) ? EF_NONE : EF_HYPERBLASTER);
}

mframe_t boss2_frames_attack_hb[] = {
	{ ai_charge, 2, Boss2HyperBlaster },
	{ ai_charge, 2, Boss2HyperBlaster },
	{ ai_charge, 2, Boss2HyperBlaster },
	{ ai_charge, 2, Boss2HyperBlaster },
	{ ai_charge, 2, Boss2HyperBlaster },
	{ ai_charge, 2, [](edict_t *self) -> void { Boss2HyperBlaster(self); boss2_reattack_mg(self); } }
};
MMOVE_T(boss2_move_attack_hb) = { FRAME_attack10, FRAME_attack15, boss2_frames_attack_hb, nullptr };

mframe_t boss2_frames_attack_post_mg[] = {
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 }
};
MMOVE_T(boss2_move_attack_post_mg) = { FRAME_attack16, FRAME_attack19, boss2_frames_attack_post_mg, boss2_run };

mframe_t boss2_frames_attack_rocket[] = {
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_move, -5, Boss2Rocket },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 }
};
MMOVE_T(boss2_move_attack_rocket) = { FRAME_attack20, FRAME_attack40, boss2_frames_attack_rocket, boss2_run };

// [Paril-KEX] n64 rocket behavior
mframe_t boss2_frames_attack_rocket2[] = {
	{ ai_charge, 2, Boss2Rocket64 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2, Boss2Rocket64 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2, Boss2Rocket64 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2, Boss2Rocket64 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2, Boss2Rocket64 },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2 }
};
MMOVE_T(boss2_move_attack_rocket2) = { FRAME_attack20, FRAME_attack39, boss2_frames_attack_rocket2, boss2_run };

mframe_t boss2_frames_pain_heavy[] = {
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
MMOVE_T(boss2_move_pain_heavy) = { FRAME_pain2, FRAME_pain19, boss2_frames_pain_heavy, boss2_run };

mframe_t boss2_frames_pain_light[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(boss2_move_pain_light) = { FRAME_pain20, FRAME_pain23, boss2_frames_pain_light, boss2_run };

static void boss2_shrink(edict_t *self)
{
	self->maxs.z = 50.f;
	gi.linkentity(self);
}

mframe_t boss2_frames_death[] = {
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
	{ ai_move, 0, boss2_shrink },
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
MMOVE_T(boss2_move_death) = { FRAME_death2, FRAME_death50, boss2_frames_death, boss2_dead };

MONSTERINFO_STAND(boss2_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &boss2_move_stand);
}

MONSTERINFO_RUN(boss2_run) (edict_t *self) -> void
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		M_SetAnimation(self, &boss2_move_stand);
	else
		M_SetAnimation(self, &boss2_move_run);
}

MONSTERINFO_WALK(boss2_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &boss2_move_walk);
}

MONSTERINFO_ATTACK(boss2_attack) (edict_t *self) -> void
{
	vec3_t vec;
	float  range;

	vec = self->enemy->s.origin - self->s.origin;
	range = vec.length();

	if (range <= 125 || frandom() <= 0.6f)
		M_SetAnimation(self, self->spawnflags.has(SPAWNFLAG_BOSS2_N64) ? &boss2_move_attack_hb : &boss2_move_attack_pre_mg);
	else
		M_SetAnimation(self, self->spawnflags.has(SPAWNFLAG_BOSS2_N64) ? &boss2_move_attack_rocket2 : &boss2_move_attack_rocket);
}

void boss2_attack_mg(edict_t *self)
{
	M_SetAnimation(self, self->spawnflags.has(SPAWNFLAG_BOSS2_N64) ? &boss2_move_attack_hb : &boss2_move_attack_mg);
}

void boss2_reattack_mg(edict_t *self)
{
	if (infront(self, self->enemy) && frandom() <= 0.7f)
		boss2_attack_mg(self);
	else
		M_SetAnimation(self, &boss2_move_attack_post_mg);
}

PAIN(boss2_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3_sec;

	// American wanted these at no attenuation
	if (damage < 10)
		gi.sound(self, CHAN_VOICE, sound_pain3, 1, ATTN_NONE, 0);
	else if (damage < 30)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NONE, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NONE, 0);
	
	if (!M_ShouldReactToPain(self, mod))
		return; // no pain anims in nightmare

	if (damage < 10)
		M_SetAnimation(self, &boss2_move_pain_light);
	else if (damage < 30)
		M_SetAnimation(self, &boss2_move_pain_light);
	else
		M_SetAnimation(self, &boss2_move_pain_heavy);
}

MONSTERINFO_SETSKIN(boss2_setskin) (edict_t *self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

static void boss2_gib(edict_t *self)
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
		{ 2, "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC },
		{ "models/monsters/boss2/gibs/chest.md2", GIB_SKINNED },
		{ 2, "models/monsters/boss2/gibs/chaingun.md2", GIB_SKINNED | GIB_UPRIGHT },
		{ "models/monsters/boss2/gibs/cpu.md2", GIB_SKINNED | GIB_UPRIGHT },
		{ "models/monsters/boss2/gibs/engine.md2", GIB_SKINNED },
		{ "models/monsters/boss2/gibs/rocket.md2", GIB_SKINNED | GIB_UPRIGHT },
		{ "models/monsters/boss2/gibs/spine.md2", GIB_SKINNED },
		{ 2, "models/monsters/boss2/gibs/wing.md2", GIB_SKINNED | GIB_UPRIGHT },
		{ "models/monsters/boss2/gibs/larm.md2", GIB_SKINNED | GIB_UPRIGHT },
		{ "models/monsters/boss2/gibs/rarm.md2", GIB_SKINNED | GIB_UPRIGHT },
		{ "models/monsters/boss2/gibs/larm.md2", 2.0f, GIB_SKINNED | GIB_UPRIGHT },
		{ "models/monsters/boss2/gibs/rarm.md2", 2.0f, GIB_SKINNED | GIB_UPRIGHT },
		{ "models/monsters/boss2/gibs/larm.md2", 1.35f, GIB_SKINNED | GIB_UPRIGHT },
		{ "models/monsters/boss2/gibs/rarm.md2", 1.35f, GIB_SKINNED | GIB_UPRIGHT },
		{ "models/monsters/boss2/gibs/head.md2", GIB_SKINNED | GIB_METALLIC | GIB_HEAD }
	});
}

void boss2_dead(edict_t *self)
{
	// no blowy on deady
	if (self->spawnflags.has(SPAWNFLAG_MONSTER_DEAD))
	{
		self->deadflag = false;
		self->takedamage = true;
		return;
	}

	boss2_gib(self);
}

DIE(boss2_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	if (self->spawnflags.has(SPAWNFLAG_MONSTER_DEAD))
	{
		// check for gib
		if (M_CheckGib(self, mod))
		{
			boss2_gib(self);
			self->deadflag = true;
			return;
		}

		if (self->deadflag)
			return;
	}
	else
	{
		gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NONE, 0);
		self->deadflag = true;
		self->takedamage = false;
		self->count = 0;
		self->velocity = {};
		self->gravityVector.z *= 0.30f;
	}

	M_SetAnimation(self, &boss2_move_death);
}

// [Paril-KEX] use generic function
MONSTERINFO_CHECKATTACK(Boss2_CheckAttack) (edict_t *self) -> bool
{
	return M_CheckAttack_Base(self, 0.4f, 0.8f, 0.8f, 0.8f, 0.f, 0.f);
}

/*QUAKED monster_boss2 (1 .5 0) (-56 -56 0) (56 56 80) Ambush Trigger_Spawn Sight Hyperblaster
 */
void SP_monster_boss2(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	sound_pain1.assign("bosshovr/bhvpain1.wav");
	sound_pain2.assign("bosshovr/bhvpain2.wav");
	sound_pain3.assign("bosshovr/bhvpain3.wav");
	sound_death.assign("bosshovr/bhvdeth1.wav");
	sound_search1.assign("bosshovr/bhvunqv1.wav");

	gi.soundindex("tank/rocket.wav");

	if (self->spawnflags.has(SPAWNFLAG_BOSS2_N64))
		gi.soundindex("flyer/flyatck3.wav");
	else
		gi.soundindex("infantry/infatck1.wav");

	self->monsterinfo.weapon_sound = gi.soundindex("bosshovr/bhvengn1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/boss2/tris.md2");
	
	gi.modelindex("models/monsters/boss2/gibs/chaingun.md2");
	gi.modelindex("models/monsters/boss2/gibs/chest.md2");
	gi.modelindex("models/monsters/boss2/gibs/cpu.md2");
	gi.modelindex("models/monsters/boss2/gibs/engine.md2");
	gi.modelindex("models/monsters/boss2/gibs/head.md2");
	gi.modelindex("models/monsters/boss2/gibs/larm.md2");
	gi.modelindex("models/monsters/boss2/gibs/rarm.md2");
	gi.modelindex("models/monsters/boss2/gibs/rocket.md2");
	gi.modelindex("models/monsters/boss2/gibs/spine.md2");
	gi.modelindex("models/monsters/boss2/gibs/wing.md2");

	self->mins = { -56, -56, 0 };
	self->maxs = { 56, 56, 80 };

	self->health = 2000 * st.health_multiplier;
	self->gib_health = -200;
	self->mass = 1000;

	self->yaw_speed = 50;

	self->flags |= FL_IMMUNE_LASER;

	self->pain = boss2_pain;
	self->die = boss2_die;

	self->monsterinfo.stand = boss2_stand;
	self->monsterinfo.walk = boss2_walk;
	self->monsterinfo.run = boss2_run;
	self->monsterinfo.attack = boss2_attack;
	self->monsterinfo.search = boss2_search;
	self->monsterinfo.checkattack = Boss2_CheckAttack;
	self->monsterinfo.setskin = boss2_setskin;
	gi.linkentity(self);

	M_SetAnimation(self, &boss2_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	// [Paril-KEX]
	self->monsterinfo.aiflags |= AI_IGNORE_SHOTS;

	flymonster_start(self);
}
