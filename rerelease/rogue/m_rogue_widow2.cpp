// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

black widow, part 2

==============================================================================
*/

// timestamp used to prevent rapid fire of melee attack

#include "../g_local.h"
#include "m_rogue_widow2.h"
#include "../m_flash.h"

static cached_soundindex sound_pain1;
static cached_soundindex sound_pain2;
static cached_soundindex sound_pain3;
static cached_soundindex sound_death;
static cached_soundindex sound_search1;
static cached_soundindex sound_tentacles_retract;

// sqrt(64*64*2) + sqrt(28*28*2) => 130.1
constexpr vec3_t spawnpoints[] = {
	{ 30, 135, 0 },
	{ 30, -135, 0 }
};

constexpr float sweep_angles[] = {
	-40.0, -32.0, -24.0, -16.0, -8.0, 0.0, 8.0, 16.0, 24.0, 32.0, 40.0
};

constexpr vec3_t stalker_mins = { -28, -28, -18 };
constexpr vec3_t stalker_maxs = { 28, 28, 18 };

bool infront(edict_t *self, edict_t *other);
void WidowCalcSlots(edict_t *self);
void WidowPowerups(edict_t *self);

void widow2_run(edict_t *self);
void widow2_dead(edict_t *self);
void widow2_attack_beam(edict_t *self);
void widow2_reattack_beam(edict_t *self);
void widow_start_spawn(edict_t *self);
void widow_done_spawn(edict_t *self);
void widow2_spawn_check(edict_t *self);
void Widow2SaveBeamTarget(edict_t *self);

// death stuff
void WidowExplode(edict_t *self);
void ThrowWidowGibReal(edict_t *self, const char *gibname, int damage, gib_type_t type, const vec3_t *startpos, bool large, int hitsound, bool fade);
void ThrowWidowGibSized(edict_t *self, const char *gibname, int damage, gib_type_t type, const vec3_t *startpos, int hitsound, bool fade);
void ThrowWidowGibLoc(edict_t *self, const char *gibname, int damage, gib_type_t type, const vec3_t *startpos, bool fade);
void WidowExplosion1(edict_t *self);
void WidowExplosion2(edict_t *self);
void WidowExplosion3(edict_t *self);
void WidowExplosion4(edict_t *self);
void WidowExplosion5(edict_t *self);
void WidowExplosion6(edict_t *self);
void WidowExplosion7(edict_t *self);
void WidowExplosionLeg(edict_t *self);
void ThrowArm1(edict_t *self);
void ThrowArm2(edict_t *self);
void ClipGibVelocity(edict_t *ent);
// end of death stuff

// these offsets used by the tongue
constexpr vec3_t offsets[] = {
	{ 17.48f, 0.10f, 68.92f },
	{ 17.47f, 0.29f, 68.91f },
	{ 17.45f, 0.53f, 68.87f },
	{ 17.42f, 0.78f, 68.81f },
	{ 17.39f, 1.02f, 68.75f },
	{ 17.37f, 1.20f, 68.70f },
	{ 17.36f, 1.24f, 68.71f },
	{ 17.37f, 1.21f, 68.72f },
};

void showme(edict_t *self);

void pauseme(edict_t *self)
{
	self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

MONSTERINFO_SEARCH(widow2_search) (edict_t *self) -> void
{
	if (frandom() < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_search1, 1, ATTN_NONE, 0);
}

void Widow2Beam(edict_t *self)
{
	vec3_t					 forward, right, target;
	vec3_t					 start, targ_angles, vec;
	monster_muzzleflash_id_t flashnum;

	if ((!self->enemy) || (!self->enemy->inuse))
		return;

	AngleVectors(self->s.angles, forward, right, nullptr);

	if ((self->s.frame >= FRAME_fireb05) && (self->s.frame <= FRAME_fireb09))
	{
		// regular beam attack
		Widow2SaveBeamTarget(self);
		flashnum = static_cast<monster_muzzleflash_id_t>(MZ2_WIDOW2_BEAMER_1 + self->s.frame - FRAME_fireb05);
		start = G_ProjectSource(self->s.origin, monster_flash_offset[flashnum], forward, right);
		target = self->pos2;
		target[2] += self->enemy->viewheight - 10;
		forward = target - start;
		forward.normalize();
		monster_fire_heatbeam(self, start, forward, vec3_origin, 10, 50, flashnum);
	}
	else if ((self->s.frame >= FRAME_spawn04) && (self->s.frame <= FRAME_spawn14))
	{
		// sweep
		flashnum = static_cast<monster_muzzleflash_id_t>(MZ2_WIDOW2_BEAM_SWEEP_1 + self->s.frame - FRAME_spawn04);
		start = G_ProjectSource(self->s.origin, monster_flash_offset[flashnum], forward, right);
		target = self->enemy->s.origin - start;
		targ_angles = vectoangles(target);

		vec = self->s.angles;

		vec[PITCH] += targ_angles[PITCH];
		vec[YAW] -= sweep_angles[flashnum - MZ2_WIDOW2_BEAM_SWEEP_1];

		AngleVectors(vec, forward, nullptr, nullptr);
		monster_fire_heatbeam(self, start, forward, vec3_origin, 10, 50, flashnum);
	}
	else
	{
		Widow2SaveBeamTarget(self);
		start = G_ProjectSource(self->s.origin, monster_flash_offset[MZ2_WIDOW2_BEAMER_1], forward, right);

		target = self->pos2;
		target[2] += self->enemy->viewheight - 10;

		forward = target - start;
		forward.normalize();

		monster_fire_heatbeam(self, start, forward, vec3_origin, 10, 50, MZ2_WIDOW2_BEAM_SWEEP_1);
	}
}

void Widow2Spawn(edict_t *self)
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

void widow2_spawn_check(edict_t *self)
{
	Widow2Beam(self);
	Widow2Spawn(self);
}

void widow2_ready_spawn(edict_t *self)
{
	vec3_t f, r, u, offset, startpoint, spawnpoint;
	int	   i;

	Widow2Beam(self);
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

void widow2_step(edict_t *self)
{
	gi.sound(self, CHAN_BODY, gi.soundindex("widow/bwstep1.wav"), 1, ATTN_NORM, 0);
}

mframe_t widow2_frames_stand[] = {
	{ ai_stand }
};
MMOVE_T(widow2_move_stand) = { FRAME_blackwidow3, FRAME_blackwidow3, widow2_frames_stand, nullptr };

mframe_t widow2_frames_walk[] = {
	{ ai_walk, 9.01f, widow2_step },
	{ ai_walk, 7.55f },
	{ ai_walk, 7.01f },
	{ ai_walk, 6.66f },
	{ ai_walk, 6.20f },
	{ ai_walk, 5.78f, widow2_step },
	{ ai_walk, 7.25f },
	{ ai_walk, 8.37f },
	{ ai_walk, 10.41f }
};
MMOVE_T(widow2_move_walk) = { FRAME_walk01, FRAME_walk09, widow2_frames_walk, nullptr };

mframe_t widow2_frames_run[] = {
	{ ai_run, 9.01f, widow2_step },
	{ ai_run, 7.55f },
	{ ai_run, 7.01f },
	{ ai_run, 6.66f },
	{ ai_run, 6.20f },
	{ ai_run, 5.78f, widow2_step },
	{ ai_run, 7.25f },
	{ ai_run, 8.37f },
	{ ai_run, 10.41f }
};
MMOVE_T(widow2_move_run) = { FRAME_walk01, FRAME_walk09, widow2_frames_run, nullptr };

mframe_t widow2_frames_attack_pre_beam[] = {
	{ ai_charge, 4 },
	{ ai_charge, 4, widow2_step },
	{ ai_charge, 4 },
	{ ai_charge, 4, widow2_attack_beam }
};
MMOVE_T(widow2_move_attack_pre_beam) = { FRAME_fireb01, FRAME_fireb04, widow2_frames_attack_pre_beam, nullptr };

// Loop this
mframe_t widow2_frames_attack_beam[] = {
	{ ai_charge, 0, Widow2Beam },
	{ ai_charge, 0, Widow2Beam },
	{ ai_charge, 0, Widow2Beam },
	{ ai_charge, 0, Widow2Beam },
	{ ai_charge, 0, widow2_reattack_beam }
};
MMOVE_T(widow2_move_attack_beam) = { FRAME_fireb05, FRAME_fireb09, widow2_frames_attack_beam, nullptr };

mframe_t widow2_frames_attack_post_beam[] = {
	{ ai_charge, 4 },
	{ ai_charge, 4 }
};
MMOVE_T(widow2_move_attack_post_beam) = { FRAME_fireb06, FRAME_fireb07, widow2_frames_attack_post_beam, widow2_run };

void WidowDisrupt(edict_t *self)
{
	vec3_t start;
	vec3_t dir;
	vec3_t forward, right;
	float  len;

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = G_ProjectSource(self->s.origin, monster_flash_offset[MZ2_WIDOW_DISRUPTOR], forward, right);

	dir = self->pos1 - self->enemy->s.origin;
	len = dir.length();

	if (len < 30)
	{
		// calc direction to where we targeted
		dir = self->pos1 - start;
		dir.normalize();

		monster_fire_tracker(self, start, dir, 20, 500, self->enemy, MZ2_WIDOW_DISRUPTOR);
	}
	else
	{
		PredictAim(self, self->enemy, start, 1200, true, 0, &dir, nullptr);
		monster_fire_tracker(self, start, dir, 20, 1200, nullptr, MZ2_WIDOW_DISRUPTOR);
	}

	widow2_step(self);
}

void Widow2SaveDisruptLoc(edict_t *self)
{
	if (self->enemy && self->enemy->inuse)
	{
		self->pos1 = self->enemy->s.origin; // save for aiming the shot
		self->pos1[2] += self->enemy->viewheight;
	}
	else
		self->pos1 = {};
}

void widow2_disrupt_reattack(edict_t *self)
{
	float luck = frandom();

	if (luck < (0.25f + (skill->integer * 0.15f)))
		self->monsterinfo.nextframe = FRAME_firea01;
}

mframe_t widow2_frames_attack_disrupt[] = {
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2, Widow2SaveDisruptLoc },
	{ ai_charge, -20, WidowDisrupt },
	{ ai_charge, 2 },
	{ ai_charge, 2 },
	{ ai_charge, 2, widow2_disrupt_reattack }
};
MMOVE_T(widow2_move_attack_disrupt) = { FRAME_firea01, FRAME_firea07, widow2_frames_attack_disrupt, widow2_run };

void Widow2SaveBeamTarget(edict_t *self)
{
	if (self->enemy && self->enemy->inuse)
	{
		self->pos2 = self->pos1;
		self->pos1 = self->enemy->s.origin; // save for aiming the shot
	}
	else
	{
		self->pos1 = {};
		self->pos2 = {};
	}
}

void Widow2BeamTargetRemove(edict_t *self)
{
	self->pos1 = {};
	self->pos2 = {};
}

void Widow2StartSweep(edict_t *self)
{
	Widow2SaveBeamTarget(self);
}

mframe_t widow2_frames_spawn[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, [](edict_t *self) { widow_start_spawn(self); widow2_step(self); } },
	{ ai_charge, 0, Widow2Beam },
	{ ai_charge, 0, Widow2Beam }, // 5
	{ ai_charge, 0, Widow2Beam },
	{ ai_charge, 0, Widow2Beam },
	{ ai_charge, 0, Widow2Beam },
	{ ai_charge, 0, Widow2Beam },
	{ ai_charge, 0, widow2_ready_spawn }, // 10
	{ ai_charge, 0, Widow2Beam },
	{ ai_charge, 0, Widow2Beam },
	{ ai_charge, 0, Widow2Beam },
	{ ai_charge, 0, widow2_spawn_check },
	{ ai_charge }, // 15
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, widow2_reattack_beam }
};
MMOVE_T(widow2_move_spawn) = { FRAME_spawn01, FRAME_spawn18, widow2_frames_spawn, nullptr };

static bool widow2_tongue_attack_ok(const vec3_t &start, const vec3_t &end, float range)
{
	vec3_t dir, angles;

	// check for max distance
	dir = start - end;
	if (dir.length() > range)
		return false;

	// check for min/max pitch
	angles = vectoangles(dir);
	if (angles[0] < -180)
		angles[0] += 360;
	if (fabsf(angles[0]) > 30)
		return false;

	return true;
}

void Widow2Tongue(edict_t *self)
{
	vec3_t	f, r, u;
	vec3_t	start, end, dir;
	trace_t tr;

	AngleVectors(self->s.angles, f, r, u);
	start = G_ProjectSource2(self->s.origin, offsets[self->s.frame - FRAME_tongs01], f, r, u);
	end = self->enemy->s.origin;
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
	end = self->enemy->s.origin;

	tr = gi.traceline(start, end, self, MASK_PROJECTILE);
	if (tr.ent != self->enemy)
		return;

	gi.sound(self, CHAN_WEAPON, sound_tentacles_retract, 1, ATTN_NORM, 0);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_PARASITE_ATTACK);
	gi.WriteEntity(self);
	gi.WritePosition(start);
	gi.WritePosition(end);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);

	dir = start - end;
	T_Damage(self->enemy, self, self, dir, self->enemy->s.origin, vec3_origin, 2, 0, DAMAGE_NO_KNOCKBACK, MOD_UNKNOWN);
}

void Widow2TonguePull(edict_t *self)
{
	vec3_t vec;
	vec3_t f, r, u;
	vec3_t start, end;

	if ((!self->enemy) || (!self->enemy->inuse))
	{
		self->monsterinfo.run(self);
		return;
	}

	AngleVectors(self->s.angles, f, r, u);
	start = G_ProjectSource2(self->s.origin, offsets[self->s.frame - FRAME_tongs01], f, r, u);
	end = self->enemy->s.origin;

	if (!widow2_tongue_attack_ok(start, end, 256))
		return;

	if (self->enemy->groundentity)
	{
		self->enemy->s.origin[2] += 1;
		self->enemy->groundentity = nullptr;
		// interesting, you don't have to relink the player
	}

	vec = self->s.origin - self->enemy->s.origin;

	if (self->enemy->client)
	{
		vec.normalize();
		self->enemy->velocity += (vec * 1000);
	}
	else
	{
		self->enemy->ideal_yaw = vectoyaw(vec);
		M_ChangeYaw(self->enemy);
		self->enemy->velocity = f * 1000;
	}
}

void Widow2Crunch(edict_t *self)
{
	vec3_t aim;

	if ((!self->enemy) || (!self->enemy->inuse))
	{
		self->monsterinfo.run(self);
		return;
	}

	Widow2TonguePull(self);

	// 70 + 32
	aim = { 150, 0, 4 };
	if (self->s.frame != FRAME_tongs07)
		fire_hit(self, aim, irandom(20, 26), 0);
	else if (self->enemy->groundentity)
		fire_hit(self, aim, irandom(20, 26), 500);
	else // not as much kick if they're in the air .. makes it harder to land on her head
		fire_hit(self, aim, irandom(20, 26), 250);
}

void Widow2Toss(edict_t *self)
{
	self->timestamp = level.time + 3_sec;
}

mframe_t widow2_frames_tongs[] = {
	{ ai_charge, 0, Widow2Tongue },
	{ ai_charge, 0, Widow2Tongue },
	{ ai_charge, 0, Widow2Tongue },
	{ ai_charge, 0, Widow2TonguePull },
	{ ai_charge, 0, Widow2TonguePull }, // 5
	{ ai_charge, 0, Widow2TonguePull },
	{ ai_charge, 0, Widow2Crunch },
	{ ai_charge, 0, Widow2Toss }
};
MMOVE_T(widow2_move_tongs) = { FRAME_tongs01, FRAME_tongs08, widow2_frames_tongs, widow2_run };

mframe_t widow2_frames_pain[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(widow2_move_pain) = { FRAME_pain01, FRAME_pain05, widow2_frames_pain, widow2_run };

mframe_t widow2_frames_death[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, WidowExplosion1 }, // 3 boom
	{ ai_move },
	{ ai_move }, // 5

	{ ai_move, 0, WidowExplosion2 }, // 6 boom
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }, // 10

	{ ai_move },
	{ ai_move }, // 12
	{ ai_move },
	{ ai_move },
	{ ai_move }, // 15

	{ ai_move },
	{ ai_move },
	{ ai_move, 0, WidowExplosion3 }, // 18
	{ ai_move },					 // 19
	{ ai_move },					 // 20

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, WidowExplosion4 }, // 25

	{ ai_move }, // 26
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, WidowExplosion5 },
	{ ai_move, 0, WidowExplosionLeg }, // 30

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, WidowExplosion6 },
	{ ai_move }, // 35

	{ ai_move },
	{ ai_move },
	{ ai_move, 0, WidowExplosion7 },
	{ ai_move },
	{ ai_move }, // 40

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, WidowExplode } // 44
};
MMOVE_T(widow2_move_death) = { FRAME_death01, FRAME_death44, widow2_frames_death, nullptr };

void widow2_start_searching(edict_t *self);
void widow2_keep_searching(edict_t *self);
void widow2_finaldeath(edict_t *self);

mframe_t widow2_frames_dead[] = {
	{ ai_move, 0, widow2_start_searching },
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
	{ ai_move, 0, widow2_keep_searching }
};
MMOVE_T(widow2_move_dead) = { FRAME_dthsrh01, FRAME_dthsrh15, widow2_frames_dead, nullptr };

mframe_t widow2_frames_really_dead[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move, 0, widow2_finaldeath }
};
MMOVE_T(widow2_move_really_dead) = { FRAME_dthsrh16, FRAME_dthsrh22, widow2_frames_really_dead, nullptr };

void widow2_start_searching(edict_t *self)
{
	self->count = 0;
}

void widow2_keep_searching(edict_t *self)
{
	if (self->count <= 2)
	{
		M_SetAnimation(self, &widow2_move_dead);
		self->s.frame = FRAME_dthsrh01;
		self->count++;
		return;
	}

	M_SetAnimation(self, &widow2_move_really_dead);
}

void widow2_finaldeath(edict_t *self)
{
	self->mins = { -70, -70, 0 };
	self->maxs = { 70, 70, 80 };
	self->movetype = MOVETYPE_TOSS;
	self->takedamage = true;
	self->nextthink = 0_ms;
	gi.linkentity(self);
}

MONSTERINFO_STAND(widow2_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &widow2_move_stand);
}

MONSTERINFO_RUN(widow2_run) (edict_t *self) -> void
{
	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		M_SetAnimation(self, &widow2_move_stand);
	else
		M_SetAnimation(self, &widow2_move_run);
}

MONSTERINFO_WALK(widow2_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &widow2_move_walk);
}

void widow2_attack(edict_t *self);

MONSTERINFO_MELEE(widow2_melee) (edict_t *self) -> void
{
	if (self->timestamp >= level.time)
		widow2_attack(self);
	else
		M_SetAnimation(self, &widow2_move_tongs);
}

MONSTERINFO_ATTACK(widow2_attack) (edict_t *self) -> void
{
	float luck;
	bool  blocked = false;

	if (self->monsterinfo.aiflags & AI_BLOCKED)
	{
		blocked = true;
		self->monsterinfo.aiflags &= ~AI_BLOCKED;
	}

	if (!self->enemy)
		return;

	float real_enemy_range = realrange(self, self->enemy);

	// melee attack
	if (self->timestamp < level.time)
	{
		if (real_enemy_range < 300)
		{
			vec3_t f, r, u;
			AngleVectors(self->s.angles, f, r, u);
			vec3_t spot1 = G_ProjectSource2(self->s.origin, offsets[0], f, r, u);
			vec3_t spot2 = self->enemy->s.origin;
			if (widow2_tongue_attack_ok(spot1, spot2, 256))
			{
				// melee attack ok

				// be nice in easy mode
				if (skill->integer != 0 || irandom(4))
				{
					M_SetAnimation(self, &widow2_move_tongs);
					return;
				}
			}
		}
	}

	if (self->bad_area)
	{
		if ((frandom() < 0.75f) || (level.time < self->monsterinfo.attack_finished))
			M_SetAnimation(self, &widow2_move_attack_pre_beam);
		else
		{
			M_SetAnimation(self, &widow2_move_attack_disrupt);
		}
		return;
	}

	WidowCalcSlots(self);

	// if we can't see the target, spawn stuff
	if ((self->monsterinfo.attack_state == AS_BLIND) && (M_SlotsLeft(self) >= 2))
	{
		M_SetAnimation(self, &widow2_move_spawn);
		return;
	}

	// accept bias towards spawning
	if (blocked && (M_SlotsLeft(self) >= 2))
	{
		M_SetAnimation(self, &widow2_move_spawn);
		return;
	}

	if (real_enemy_range < 600)
	{
		luck = frandom();
		if (M_SlotsLeft(self) >= 2)
		{
			if (luck <= 0.40f)
				M_SetAnimation(self, &widow2_move_attack_pre_beam);
			else if ((luck <= 0.7f) && !(level.time < self->monsterinfo.attack_finished))
			{
				M_SetAnimation(self, &widow2_move_attack_disrupt);
			}
			else
				M_SetAnimation(self, &widow2_move_spawn);
		}
		else
		{
			if ((luck <= 0.50f) || (level.time < self->monsterinfo.attack_finished))
				M_SetAnimation(self, &widow2_move_attack_pre_beam);
			else
			{
				M_SetAnimation(self, &widow2_move_attack_disrupt);
			}
		}
	}
	else
	{
		luck = frandom();
		if (M_SlotsLeft(self) >= 2)
		{
			if (luck < 0.3f)
				M_SetAnimation(self, &widow2_move_attack_pre_beam);
			else if ((luck < 0.65f) || (level.time < self->monsterinfo.attack_finished))
				M_SetAnimation(self, &widow2_move_spawn);
			else
			{
				M_SetAnimation(self, &widow2_move_attack_disrupt);
			}
		}
		else
		{
			if ((luck < 0.45f) || (level.time < self->monsterinfo.attack_finished))
				M_SetAnimation(self, &widow2_move_attack_pre_beam);
			else
			{
				M_SetAnimation(self, &widow2_move_attack_disrupt);
			}
		}
	}
}

void widow2_attack_beam(edict_t *self)
{
	M_SetAnimation(self, &widow2_move_attack_beam);
	widow2_step(self);
}

void widow2_reattack_beam(edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;

	if (infront(self, self->enemy))
		if (frandom() <= 0.5f)
			if ((frandom() < 0.7f) || (M_SlotsLeft(self) < 2))
				M_SetAnimation(self, &widow2_move_attack_beam);
			else
				M_SetAnimation(self, &widow2_move_spawn);
		else
			M_SetAnimation(self, &widow2_move_attack_post_beam);
	else
		M_SetAnimation(self, &widow2_move_attack_post_beam);
}

PAIN(widow2_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
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

	if (damage >= 15)
	{
		if (damage < 75)
		{
			if ((skill->integer < 3) && (frandom() < (0.6f - (0.2f * skill->integer))))
			{
				self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
				M_SetAnimation(self, &widow2_move_pain);
			}
		}
		else
		{
			if ((skill->integer < 3) && (frandom() < (0.75f - (0.1f * skill->integer))))
			{
				self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
				M_SetAnimation(self, &widow2_move_pain);
			}
		}
	}
}

MONSTERINFO_SETSKIN(widow2_setskin) (edict_t *self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

void widow2_dead(edict_t *self)
{
}

void KillChildren(edict_t *self)
{
	edict_t *ent = nullptr;

	while (1)
	{
		ent = G_FindByString<&edict_t::classname>(ent, "monster_stalker");
		if (!ent)
			return;

		// FIXME - may need to stagger
		if ((ent->inuse) && (ent->health > 0))
			T_Damage(ent, self, self, vec3_origin, self->enemy->s.origin, vec3_origin, (ent->health + 1), 0, DAMAGE_NO_KNOCKBACK, MOD_UNKNOWN);
	}
}

DIE(widow2_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	int n;
	int clipped;

	// check for gib
	if (self->deadflag && M_CheckGib(self, mod))
	{
		clipped = min(damage, 100);

		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n = 0; n < 2; n++)
			ThrowWidowGibLoc(self, "models/objects/gibs/bone/tris.md2", clipped, GIB_NONE, nullptr, false);
		for (n = 0; n < 3; n++)
			ThrowWidowGibLoc(self, "models/objects/gibs/sm_meat/tris.md2", clipped, GIB_NONE, nullptr, false);
		for (n = 0; n < 3; n++)
		{
			ThrowWidowGibSized(self, "models/monsters/blackwidow2/gib1/tris.md2", clipped, GIB_METALLIC, nullptr,
							   0, false);
			ThrowWidowGibSized(self, "models/monsters/blackwidow2/gib2/tris.md2", clipped, GIB_METALLIC, nullptr,
							   gi.soundindex("misc/fhit3.wav"), false);
		}
		for (n = 0; n < 2; n++)
		{
			ThrowWidowGibSized(self, "models/monsters/blackwidow2/gib3/tris.md2", clipped, GIB_METALLIC, nullptr,
							   0, false);
			ThrowWidowGibSized(self, "models/monsters/blackwidow/gib3/tris.md2", clipped, GIB_METALLIC, nullptr,
							   0, false);
		}
		ThrowGibs(self, damage, {
			{ "models/objects/gibs/chest/tris.md2" },
			{ "models/objects/gibs/head2/tris.md2", GIB_HEAD }
		});

		return;
	}

	if (self->deadflag)
		return;

	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NONE, 0);
	self->deadflag = true;
	self->takedamage = false;
	self->count = 0;
	KillChildren(self);
	self->monsterinfo.quad_time = 0_ms;
	self->monsterinfo.double_time = 0_ms;
	self->monsterinfo.invincible_time = 0_ms;
	M_SetAnimation(self, &widow2_move_death);
}

MONSTERINFO_CHECKATTACK(Widow2_CheckAttack) (edict_t *self) -> bool
{
	if (!self->enemy)
		return false;

	WidowPowerups(self);

	if ((frandom() < 0.8f) && (M_SlotsLeft(self) >= 2) && (realrange(self, self->enemy) > 150))
	{
		self->monsterinfo.aiflags |= AI_BLOCKED;
		self->monsterinfo.attack_state = AS_MISSILE;
		return true;
	}

	return M_CheckAttack_Base(self, 0.4f, 0.8f, 0.8f, 0.5f, 0.f, 0.f);
}

void Widow2Precache()
{
	// cache in all of the stalker stuff, widow stuff, spawngro stuff, gibs
	gi.soundindex("parasite/parpain1.wav");
	gi.soundindex("parasite/parpain2.wav");
	gi.soundindex("parasite/pardeth1.wav");
	gi.soundindex("parasite/paratck1.wav");
	gi.soundindex("parasite/parsght1.wav");
	gi.soundindex("infantry/melee2.wav");
	gi.soundindex("misc/fhit3.wav");

	gi.soundindex("tank/tnkatck3.wav");
	gi.soundindex("weapons/disrupt.wav");
	gi.soundindex("weapons/disint2.wav");

	gi.modelindex("models/monsters/stalker/tris.md2");
	gi.modelindex("models/items/spawngro3/tris.md2");
	gi.modelindex("models/objects/gibs/sm_metal/tris.md2");
	gi.modelindex("models/objects/laser/tris.md2");
	gi.modelindex("models/proj/disintegrator/tris.md2");

	gi.modelindex("models/monsters/blackwidow/gib1/tris.md2");
	gi.modelindex("models/monsters/blackwidow/gib2/tris.md2");
	gi.modelindex("models/monsters/blackwidow/gib3/tris.md2");
	gi.modelindex("models/monsters/blackwidow/gib4/tris.md2");
	gi.modelindex("models/monsters/blackwidow2/gib1/tris.md2");
	gi.modelindex("models/monsters/blackwidow2/gib2/tris.md2");
	gi.modelindex("models/monsters/blackwidow2/gib3/tris.md2");
	gi.modelindex("models/monsters/blackwidow2/gib4/tris.md2");
}

/*QUAKED monster_widow2 (1 .5 0) (-70 -70 0) (70 70 144) Ambush Trigger_Spawn Sight
 */
void SP_monster_widow2(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	sound_pain1.assign("widow/bw2pain1.wav");
	sound_pain2.assign("widow/bw2pain2.wav");
	sound_pain3.assign("widow/bw2pain3.wav");
	sound_death.assign("widow/death.wav");
	sound_search1.assign("bosshovr/bhvunqv1.wav");
	sound_tentacles_retract.assign("brain/brnatck3.wav");

	//	self->s.sound = gi.soundindex ("bosshovr/bhvengn1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/blackwidow2/tris.md2");
	self->mins = { -70, -70, 0 };
	self->maxs = { 70, 70, 144 };

	self->health = (2000 + 800 + 1000 * skill->integer) * st.health_multiplier;
	if (coop->integer)
		self->health += 500 * skill->integer;
	//	self->health = 1;
	self->gib_health = -900;
	self->mass = 2500;

	/*	if (skill->integer == 2)
		{
			self->monsterinfo.power_armor_type = IT_ITEM_POWER_SHIELD;
			self->monsterinfo.power_armor_power = 500;
		}
		else */
	if (skill->integer == 3)
	{
		if (!st.was_key_specified("power_armor_type"))
			self->monsterinfo.power_armor_type = IT_ITEM_POWER_SHIELD;
		if (!st.was_key_specified("power_armor_power"))
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
	self->monsterinfo.setskin = widow2_setskin;
	gi.linkentity(self);

	M_SetAnimation(self, &widow2_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	Widow2Precache();
	WidowCalcSlots(self);
	walkmonster_start(self);
}

//
// Death sequence stuff
//

void WidowVelocityForDamage(int damage, vec3_t &v)
{
	v[0] = damage * crandom();
	v[1] = damage * crandom();
	v[2] = damage * crandom() + 200.0f;
}

TOUCH(widow_gib_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	self->solid = SOLID_NOT;
	self->touch = nullptr;
	self->s.angles[PITCH] = 0;
	self->s.angles[ROLL] = 0;
	self->avelocity = {};

	if (self->style)
		gi.sound(self, CHAN_VOICE, self->style, 1, ATTN_NORM, 0);
}

void ThrowWidowGib(edict_t *self, const char *gibname, int damage, gib_type_t type)
{
	ThrowWidowGibReal(self, gibname, damage, type, nullptr, false, 0, true);
}

void ThrowWidowGibLoc(edict_t *self, const char *gibname, int damage, gib_type_t type, const vec3_t *startpos, bool fade)
{
	ThrowWidowGibReal(self, gibname, damage, type, startpos, false, 0, fade);
}

void ThrowWidowGibSized(edict_t *self, const char *gibname, int damage, gib_type_t type, const vec3_t *startpos, int hitsound, bool fade)
{
	ThrowWidowGibReal(self, gibname, damage, type, startpos, true, hitsound, fade);
}

void ThrowWidowGibReal(edict_t *self, const char *gibname, int damage, gib_type_t type, const vec3_t *startpos, bool sized, int hitsound, bool fade)
{
	edict_t *gib;
	vec3_t	 vd;
	vec3_t	 origin;
	vec3_t	 size;
	float	 vscale;

	if (!gibname)
		return;

	gib = G_Spawn();

	if (startpos)
		gib->s.origin = *startpos;
	else
	{
		origin = (self->absmin + self->absmax) * 0.5f;
		gib->s.origin[0] = origin[0] + crandom() * size[0];
		gib->s.origin[1] = origin[1] + crandom() * size[1];
		gib->s.origin[2] = origin[2] + crandom() * size[2];
	}

	gib->solid = SOLID_NOT;
	gib->s.effects |= EF_GIB;
	gib->flags |= FL_NO_KNOCKBACK;
	gib->takedamage = true;
	gib->die = gib_die;
	gib->s.renderfx |= RF_IR_VISIBLE;
	gib->s.renderfx &= ~RF_DOT_SHADOW;

	if (fade)
	{
		gib->think = G_FreeEdict;
		// sized gibs last longer
		if (sized)
			gib->nextthink = level.time + random_time(20_sec, 35_sec);
		else
			gib->nextthink = level.time + random_time(5_sec, 15_sec);
	}
	else
	{
		gib->think = G_FreeEdict;
		// sized gibs last longer
		if (sized)
			gib->nextthink = level.time + random_time(60_sec, 75_sec);
		else
			gib->nextthink = level.time + random_time(25_sec, 35_sec);
	}

	if (!(type & GIB_METALLIC))
	{
		gib->movetype = MOVETYPE_TOSS;
		vscale = 0.5;
	}
	else
	{
		gib->movetype = MOVETYPE_BOUNCE;
		vscale = 1.0;
	}

	WidowVelocityForDamage(damage, vd);
	gib->velocity = self->velocity + (vd * vscale);
	ClipGibVelocity(gib);

	gi.setmodel(gib, gibname);

	if (sized)
	{
		gib->style = hitsound;
		gib->solid = SOLID_BBOX;
		gib->avelocity[0] = frandom(400);
		gib->avelocity[1] = frandom(400);
		gib->avelocity[2] = frandom(400);
		if (gib->velocity[2] < 0)
			gib->velocity[2] *= -1;
		gib->velocity[0] *= 2;
		gib->velocity[1] *= 2;
		ClipGibVelocity(gib);
		gib->velocity[2] = max(frandom(350.f, 450.f), gib->velocity[2]);
		gib->gravity = 0.25;
		gib->touch = widow_gib_touch;
		gib->owner = self;
		if (gib->s.modelindex == gi.modelindex("models/monsters/blackwidow2/gib2/tris.md2"))
		{
			gib->mins = { -10, -10, 0 };
			gib->maxs = { 10, 10, 10 };
		}
		else
		{
			gib->mins = { -5, -5, 0 };
			gib->maxs = { 5, 5, 5 };
		}
	}
	else
	{
		gib->velocity[0] *= 2;
		gib->velocity[1] *= 2;
		gib->avelocity[0] = frandom(600);
		gib->avelocity[1] = frandom(600);
		gib->avelocity[2] = frandom(600);
	}

	gi.linkentity(gib);
}

void ThrowSmallStuff(edict_t *self, const vec3_t &point)
{
	int n;

	for (n = 0; n < 2; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_NONE, &point, false);
	ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, &point, false);
	ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, &point, false);
}

void ThrowMoreStuff(edict_t *self, const vec3_t &point)
{
	int n;

	if (coop->integer)
	{
		ThrowSmallStuff(self, point);
		return;
	}

	for (n = 0; n < 1; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_NONE, &point, false);
	for (n = 0; n < 2; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, &point, false);
	for (n = 0; n < 3; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, &point, false);
}

THINK(WidowExplode) (edict_t *self) -> void
{
	vec3_t org;
	int	   n;

	self->think = WidowExplode;

	// redo:
	org = self->s.origin;
	org[2] += irandom(24, 40);
	if (self->count < 8)
		org[2] += irandom(24, 56);
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
		ThrowArm1(self);
		break;
	case 6:
		org[0] -= 48;
		org[1] += 48;
		ThrowArm2(self);
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
		for (n = 0; n < 1; n++)
			ThrowWidowGib(self, "models/objects/gibs/sm_meat/tris.md2", 400, GIB_NONE);
		for (n = 0; n < 2; n++)
			ThrowWidowGib(self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC);
		for (n = 0; n < 2; n++)
			ThrowWidowGib(self, "models/objects/gibs/sm_metal/tris.md2", 400, GIB_METALLIC);
		self->deadflag = true;
		self->think = monster_think;
		self->nextthink = level.time + 10_hz;
		M_SetAnimation(self, &widow2_move_dead);
		return;
	}

	self->count++;
	if (self->count >= 9 && self->count <= 12)
	{
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_EXPLOSION1_BIG);
		gi.WritePosition(org);
		gi.multicast(self->s.origin, MULTICAST_ALL, false);
		//		goto redo;
	}
	else
	{
		// else
		gi.WriteByte(svc_temp_entity);
		if (self->count % 2)
			gi.WriteByte(TE_EXPLOSION1);
		else
			gi.WriteByte(TE_EXPLOSION1_NP);
		gi.WritePosition(org);
		gi.multicast(self->s.origin, MULTICAST_ALL, false);
	}

	self->nextthink = level.time + 10_hz;
}

void WidowExplosion1(edict_t *self)
{
	int	   n;
	vec3_t f, r, u, startpoint;
	vec3_t offset = { 23.74f, -37.67f, 76.96f };

	AngleVectors(self->s.angles, f, r, u);
	startpoint = G_ProjectSource2(self->s.origin, offset, f, r, u);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(startpoint);
	gi.multicast(self->s.origin, MULTICAST_ALL, false);

	for (n = 0; n < 1; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_NONE, &startpoint, false);
	for (n = 0; n < 1; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, &startpoint, false);
	for (n = 0; n < 2; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, &startpoint, false);
}

void WidowExplosion2(edict_t *self)
{
	int	   n;
	vec3_t f, r, u, startpoint;
	vec3_t offset = { -20.49f, 36.92f, 73.52f };

	AngleVectors(self->s.angles, f, r, u);
	startpoint = G_ProjectSource2(self->s.origin, offset, f, r, u);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(startpoint);
	gi.multicast(self->s.origin, MULTICAST_ALL, false);

	for (n = 0; n < 1; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_NONE, &startpoint, false);
	for (n = 0; n < 1; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, &startpoint, false);
	for (n = 0; n < 2; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, &startpoint, false);
}

void WidowExplosion3(edict_t *self)
{
	int	   n;
	vec3_t f, r, u, startpoint;
	vec3_t offset = { 2.11f, 0.05f, 92.20f };

	AngleVectors(self->s.angles, f, r, u);
	startpoint = G_ProjectSource2(self->s.origin, offset, f, r, u);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(startpoint);
	gi.multicast(self->s.origin, MULTICAST_ALL, false);

	for (n = 0; n < 1; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_NONE, &startpoint, false);
	for (n = 0; n < 1; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, &startpoint, false);
	for (n = 0; n < 2; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, &startpoint, false);
}

void WidowExplosion4(edict_t *self)
{
	int	   n;
	vec3_t f, r, u, startpoint;
	vec3_t offset = { -28.04f, -35.57f, -77.56f };

	AngleVectors(self->s.angles, f, r, u);
	startpoint = G_ProjectSource2(self->s.origin, offset, f, r, u);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(startpoint);
	gi.multicast(self->s.origin, MULTICAST_ALL, false);

	for (n = 0; n < 1; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_NONE, &startpoint, false);
	for (n = 0; n < 1; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, &startpoint, false);
	for (n = 0; n < 2; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, &startpoint, false);
}

void WidowExplosion5(edict_t *self)
{
	int	   n;
	vec3_t f, r, u, startpoint;
	vec3_t offset = { -20.11f, -1.11f, 40.76f };

	AngleVectors(self->s.angles, f, r, u);
	startpoint = G_ProjectSource2(self->s.origin, offset, f, r, u);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(startpoint);
	gi.multicast(self->s.origin, MULTICAST_ALL, false);

	for (n = 0; n < 1; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_NONE, &startpoint, false);
	for (n = 0; n < 1; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, &startpoint, false);
	for (n = 0; n < 2; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, &startpoint, false);
}

void WidowExplosion6(edict_t *self)
{
	int	   n;
	vec3_t f, r, u, startpoint;
	vec3_t offset = { -20.11f, -1.11f, 40.76f };

	AngleVectors(self->s.angles, f, r, u);
	startpoint = G_ProjectSource2(self->s.origin, offset, f, r, u);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(startpoint);
	gi.multicast(self->s.origin, MULTICAST_ALL, false);

	for (n = 0; n < 1; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_NONE, &startpoint, false);
	for (n = 0; n < 1; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, &startpoint, false);
	for (n = 0; n < 2; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, &startpoint, false);
}

void WidowExplosion7(edict_t *self)
{
	int	   n;
	vec3_t f, r, u, startpoint;
	vec3_t offset = { -20.11f, -1.11f, 40.76f };

	AngleVectors(self->s.angles, f, r, u);
	startpoint = G_ProjectSource2(self->s.origin, offset, f, r, u);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(startpoint);
	gi.multicast(self->s.origin, MULTICAST_ALL, false);

	for (n = 0; n < 1; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_NONE, &startpoint, false);
	for (n = 0; n < 1; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, &startpoint, false);
	for (n = 0; n < 2; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, &startpoint, false);
}

void WidowExplosionLeg(edict_t *self)
{
	vec3_t f, r, u, startpoint;
	vec3_t offset1 = { -31.89f, -47.86f, 67.02f };
	vec3_t offset2 = { -44.9f, -82.14f, 54.72f };

	AngleVectors(self->s.angles, f, r, u);
	startpoint = G_ProjectSource2(self->s.origin, offset1, f, r, u);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1_BIG);
	gi.WritePosition(startpoint);
	gi.multicast(self->s.origin, MULTICAST_ALL, false);

	ThrowWidowGibSized(self, "models/monsters/blackwidow2/gib2/tris.md2", 200, GIB_METALLIC, &startpoint,
					   gi.soundindex("misc/fhit3.wav"), false);
	ThrowWidowGibLoc(self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_NONE, &startpoint, false);
	ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, &startpoint, false);

	startpoint = G_ProjectSource2(self->s.origin, offset2, f, r, u);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(startpoint);
	gi.multicast(self->s.origin, MULTICAST_ALL, false);

	ThrowWidowGibSized(self, "models/monsters/blackwidow2/gib1/tris.md2", 300, GIB_METALLIC, &startpoint,
					   gi.soundindex("misc/fhit3.wav"), false);
	ThrowWidowGibLoc(self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_NONE, &startpoint, false);
	ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, &startpoint, false);
}

void ThrowArm1(edict_t *self)
{
	int	   n;
	vec3_t f, r, u, startpoint;
	vec3_t offset1 = { 65.76f, 17.52f, 7.56f };

	AngleVectors(self->s.angles, f, r, u);
	startpoint = G_ProjectSource2(self->s.origin, offset1, f, r, u);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1_BIG);
	gi.WritePosition(startpoint);
	gi.multicast(self->s.origin, MULTICAST_ALL, false);

	for (n = 0; n < 2; n++)
		ThrowWidowGibLoc(self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, &startpoint, false);
}

void ThrowArm2(edict_t *self)
{
	vec3_t f, r, u, startpoint;
	vec3_t offset1 = { 65.76f, 17.52f, 7.56f };

	AngleVectors(self->s.angles, f, r, u);
	startpoint = G_ProjectSource2(self->s.origin, offset1, f, r, u);

	ThrowWidowGibSized(self, "models/monsters/blackwidow2/gib4/tris.md2", 200, GIB_METALLIC, &startpoint,
					   gi.soundindex("misc/fhit3.wav"), false);
	ThrowWidowGibLoc(self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_NONE, &startpoint, false);
}
