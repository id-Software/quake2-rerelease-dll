// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"
#include "bots/bot_includes.h"

//
// monster weapons
//
void monster_muzzleflash(edict_t *self, const vec3_t &start, monster_muzzleflash_id_t id)
{
	if (id <= 255)
		gi.WriteByte(svc_muzzleflash2);
	else
		gi.WriteByte(svc_muzzleflash3);

	gi.WriteEntity(self);

	if (id <= 255)
		gi.WriteByte(id);
	else
		gi.WriteShort(id);

	gi.multicast(start, MULTICAST_PHS, false);
}

void monster_fire_bullet(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int kick, int hspread,
						 int vspread, monster_muzzleflash_id_t flashtype)
{
	fire_bullet(self, start, dir, damage, kick, hspread, vspread, MOD_UNKNOWN);
	monster_muzzleflash(self, start, flashtype);
}

void monster_fire_shotgun(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int damage, int kick, int hspread,
						  int vspread, int count, monster_muzzleflash_id_t flashtype)
{
	fire_shotgun(self, start, aimdir, damage, kick, hspread, vspread, count, MOD_UNKNOWN);
	monster_muzzleflash(self, start, flashtype);
}

void monster_fire_blaster(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed,
						  monster_muzzleflash_id_t flashtype, effects_t effect)
{
	fire_blaster(self, start, dir, damage, speed, effect, MOD_BLASTER);
	monster_muzzleflash(self, start, flashtype);
}

void monster_fire_flechette(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed,
						    monster_muzzleflash_id_t flashtype)
{
	fire_flechette(self, start, dir, damage, speed, damage / 2);
	monster_muzzleflash(self, start, flashtype);
}

void monster_fire_grenade(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int damage, int speed,
						  monster_muzzleflash_id_t flashtype, float right_adjust, float up_adjust)
{
	fire_grenade(self, start, aimdir, damage, speed, 2.5_sec, damage + 40.f, right_adjust, up_adjust, true);
	monster_muzzleflash(self, start, flashtype);
}

void monster_fire_rocket(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed,
						 monster_muzzleflash_id_t flashtype)
{
	fire_rocket(self, start, dir, damage, speed, (float) damage + 20, damage);
	monster_muzzleflash(self, start, flashtype);
}

void monster_fire_railgun(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int damage, int kick,
						  monster_muzzleflash_id_t flashtype)
{
	if (gi.pointcontents(start) & MASK_SOLID)
		return;

	fire_rail(self, start, aimdir, damage, kick);

	monster_muzzleflash(self, start, flashtype);
}

void monster_fire_bfg(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int damage, int speed, int kick,
					  float damage_radius, monster_muzzleflash_id_t flashtype)
{
	fire_bfg(self, start, aimdir, damage, speed, damage_radius);
	monster_muzzleflash(self, start, flashtype);
}

// [Paril-KEX]
vec3_t M_ProjectFlashSource(edict_t *self, const vec3_t &offset, const vec3_t &forward, const vec3_t &right)
{
	return G_ProjectSource(self->s.origin, self->s.scale ? (offset * self->s.scale) : offset, forward, right);
}

// [Paril-KEX] check if shots fired from the given offset
// might be blocked by something
bool M_CheckClearShot(edict_t *self, const vec3_t &offset, vec3_t &start)
{
	// no enemy, just do whatever
	if (!self->enemy)
		return false;

	vec3_t f, r;

	vec3_t real_angles = { self->s.angles[0], self->ideal_yaw, 0.f };

	AngleVectors(real_angles, f, r, nullptr);
	start = M_ProjectFlashSource(self, offset, f, r);

	vec3_t target;

	bool is_blind = self->monsterinfo.attack_state == AS_BLIND || (self->monsterinfo.aiflags & (AI_MANUAL_STEERING | AI_LOST_SIGHT));
	
	if (is_blind)
		target = self->monsterinfo.blind_fire_target;
	else
		target = self->enemy->s.origin + vec3_t{ 0, 0, (float) self->enemy->viewheight };

	trace_t tr = gi.traceline(start, target, self, MASK_PROJECTILE & ~CONTENTS_DEADMONSTER);

	if (tr.ent == self->enemy || tr.ent->client || (tr.fraction > 0.8f && !tr.startsolid))
		return true;

	if (!is_blind)
	{
		target = self->enemy->s.origin;

		trace_t tr = gi.traceline(start, target, self, MASK_PROJECTILE & ~CONTENTS_DEADMONSTER);

		if (tr.ent == self->enemy || tr.ent->client || (tr.fraction > 0.8f && !tr.startsolid))
			return true;
	}

	return false;
}

bool M_CheckClearShot(edict_t *self, const vec3_t &offset)
{
	vec3_t start;
	return M_CheckClearShot(self, offset, start);
}

void M_CheckGround(edict_t *ent, contents_t mask)
{
	vec3_t	point;
	trace_t trace;

	if (ent->flags & (FL_SWIM | FL_FLY))
		return;

	if ((ent->velocity[2] * ent->gravityVector[2]) < -100) // PGM
	{
		ent->groundentity = nullptr;
		return;
	}

	// if the hull point one-quarter unit down is solid the entity is on ground
	point[0] = ent->s.origin[0];
	point[1] = ent->s.origin[1];
	point[2] = ent->s.origin[2] + (0.25f * ent->gravityVector[2]); // PGM

	trace = gi.trace(ent->s.origin, ent->mins, ent->maxs, point, ent, mask);

	// check steepness
	// PGM
	if (ent->gravityVector[2] < 0) // normal gravity
	{
		if (trace.plane.normal[2] < 0.7f && !trace.startsolid)
		{
			ent->groundentity = nullptr;
			return;
		}
	}
	else // inverted gravity
	{
		if (trace.plane.normal[2] > -0.7f && !trace.startsolid)
		{
			ent->groundentity = nullptr;
			return;
		}
	}
	// PGM

	if (!trace.startsolid && !trace.allsolid)
	{
		ent->s.origin = trace.endpos;
		ent->groundentity = trace.ent;
		ent->groundentity_linkcount = trace.ent->linkcount;
		ent->velocity[2] = 0;
	}
}

void M_CatagorizePosition(edict_t *self, const vec3_t &in_point, water_level_t &waterlevel, contents_t &watertype)
{
	vec3_t	   point;
	contents_t cont;

	//
	// get waterlevel
	//
	point[0] = in_point[0];
	point[1] = in_point[1];
	if (self->gravityVector[2] > 0)
		point[2] = in_point[2] + self->maxs[2] - 1;
	else
		point[2] = in_point[2] + self->mins[2] + 1;
	cont = gi.pointcontents(point);

	if (!(cont & MASK_WATER))
	{
		waterlevel = WATER_NONE;
		watertype = CONTENTS_NONE;
		return;
	}

	watertype = cont;
	waterlevel = WATER_FEET;
	point[2] += 26;
	cont = gi.pointcontents(point);
	if (!(cont & MASK_WATER))
		return;

	waterlevel = WATER_WAIST;
	point[2] += 22;
	cont = gi.pointcontents(point);
	if (cont & MASK_WATER)
		waterlevel = WATER_UNDER;
}

bool M_ShouldReactToPain(edict_t *self, const mod_t &mod)
{
	if (self->monsterinfo.aiflags & (AI_DUCKED | AI_COMBAT_POINT))
		return false;

	return mod.id == MOD_CHAINFIST || skill->integer < 3;
}

void M_WorldEffects(edict_t *ent)
{
	int dmg;

	if (ent->health > 0)
	{
		if (!(ent->flags & FL_SWIM))
		{
			if (ent->waterlevel < WATER_UNDER)
			{
				ent->air_finished = level.time + 12_sec;
			}
			else if (ent->air_finished < level.time)
			{ // drown!
				if (ent->pain_debounce_time < level.time)
				{
					dmg = 2 + (int) (2 * floorf((level.time - ent->air_finished).seconds()));
					if (dmg > 15)
						dmg = 15;
					T_Damage(ent, world, world, vec3_origin, ent->s.origin, vec3_origin, dmg, 0, DAMAGE_NO_ARMOR,
							 MOD_WATER);
					ent->pain_debounce_time = level.time + 1_sec;
				}
			}
		}
		else
		{
			if (ent->waterlevel > WATER_NONE)
			{
				ent->air_finished = level.time + 9_sec;
			}
			else if (ent->air_finished < level.time)
			{ // suffocate!
				if (ent->pain_debounce_time < level.time)
				{
					dmg = 2 + (int) (2 * floorf((level.time - ent->air_finished).seconds()));
					if (dmg > 15)
						dmg = 15;
					T_Damage(ent, world, world, vec3_origin, ent->s.origin, vec3_origin, dmg, 0, DAMAGE_NO_ARMOR,
							 MOD_WATER);
					ent->pain_debounce_time = level.time + 1_sec;
				}
			}
		}
	}

	if (ent->waterlevel == WATER_NONE)
	{
		if (ent->flags & FL_INWATER)
		{
			gi.sound(ent, CHAN_BODY, gi.soundindex("player/watr_out.wav"), 1, ATTN_NORM, 0);
			ent->flags &= ~FL_INWATER;
		}
	}
	else
	{
		if ((ent->watertype & CONTENTS_LAVA) && !(ent->flags & FL_IMMUNE_LAVA))
		{
			if (ent->damage_debounce_time < level.time)
			{
				ent->damage_debounce_time = level.time + 100_ms;
				T_Damage(ent, world, world, vec3_origin, ent->s.origin, vec3_origin, 10 * ent->waterlevel, 0, DAMAGE_NONE,
						 MOD_LAVA);
			}
		}
		if ((ent->watertype & CONTENTS_SLIME) && !(ent->flags & FL_IMMUNE_SLIME))
		{
			if (ent->damage_debounce_time < level.time)
			{
				ent->damage_debounce_time = level.time + 100_ms;
				T_Damage(ent, world, world, vec3_origin, ent->s.origin, vec3_origin, 4 * ent->waterlevel, 0, DAMAGE_NONE,
						 MOD_SLIME);
			}
		}

		if (!(ent->flags & FL_INWATER))
		{
			if (ent->watertype & CONTENTS_LAVA)
			{
				if ((ent->svflags & SVF_MONSTER) && ent->health > 0)
				{
					if (frandom() <= 0.5f)
						gi.sound(ent, CHAN_BODY, gi.soundindex("player/lava1.wav"), 1, ATTN_NORM, 0);
					else
						gi.sound(ent, CHAN_BODY, gi.soundindex("player/lava2.wav"), 1, ATTN_NORM, 0);
				}
				else
					gi.sound(ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
			}
			else if (ent->watertype & CONTENTS_SLIME)
				gi.sound(ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
			else if (ent->watertype & CONTENTS_WATER)
				gi.sound(ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);

			ent->flags |= FL_INWATER;
			ent->damage_debounce_time = 0_ms;
		}
	}
}

bool M_droptofloor_generic(vec3_t &origin, const vec3_t &mins, const vec3_t &maxs, bool ceiling, edict_t *ignore, contents_t mask, bool allow_partial)
{
	vec3_t	end;
	trace_t trace;

	// PGM
	if (gi.trace(origin, mins, maxs, origin, ignore, mask).startsolid)
	{
		if (!ceiling)
			origin[2] += 1;
		else
			origin[2] -= 1;
	}

	if (!ceiling)
	{
		end = origin;
		end[2] -= 256;
	}
	else
	{
		end = origin;
		end[2] += 256;
	}
	// PGM

	trace = gi.trace(origin, mins, maxs, end, ignore, mask);

	if (trace.fraction == 1 || trace.allsolid || (!allow_partial && trace.startsolid))
		return false;

	origin = trace.endpos;

	return true;
}

bool M_droptofloor(edict_t *ent)
{
	contents_t mask = G_GetClipMask(ent);

	if (!ent->spawnflags.has(SPAWNFLAG_MONSTER_NO_DROP))
	{
		if (!M_droptofloor_generic(ent->s.origin, ent->mins, ent->maxs, ent->gravityVector[2] > 0, ent, mask, true))
			return false;
	}
	else
	{
		if (gi.trace(ent->s.origin, ent->mins, ent->maxs, ent->s.origin, ent, mask).startsolid)
			return false;
	}

	gi.linkentity(ent);
	M_CheckGround(ent, mask);
	M_CatagorizePosition(ent, ent->s.origin, ent->waterlevel, ent->watertype);

	return true;
}

void M_SetEffects(edict_t *ent)
{
	ent->s.effects &= ~(EF_COLOR_SHELL | EF_POWERSCREEN | EF_DOUBLE | EF_QUAD | EF_PENT | EF_FLIES);
	ent->s.renderfx &= ~(RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE);

	ent->s.sound = 0;
	ent->s.loop_attenuation = 0;

	// we're gibbed
	if (ent->s.renderfx & RF_LOW_PRIORITY)
		return;

	if (ent->monsterinfo.weapon_sound && ent->health > 0)
	{
		ent->s.sound = ent->monsterinfo.weapon_sound;
		ent->s.loop_attenuation = ATTN_NORM;
	}
	else if (ent->monsterinfo.engine_sound)
		ent->s.sound = ent->monsterinfo.engine_sound;

	if (ent->monsterinfo.aiflags & AI_RESURRECTING)
	{
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= RF_SHELL_RED;
	}

	ent->s.renderfx |= RF_DOT_SHADOW;

	// no power armor/powerup effects if we died
	if (ent->health <= 0)
		return;

	if (ent->powerarmor_time > level.time)
	{
		if (ent->monsterinfo.power_armor_type == IT_ITEM_POWER_SCREEN)
		{
			ent->s.effects |= EF_POWERSCREEN;
		}
		else if (ent->monsterinfo.power_armor_type == IT_ITEM_POWER_SHIELD)
		{
			ent->s.effects |= EF_COLOR_SHELL;
			ent->s.renderfx |= RF_SHELL_GREEN;
		}
	}

	// PMM - new monster powerups
	if (ent->monsterinfo.quad_time > level.time)
	{
		if (G_PowerUpExpiring(ent->monsterinfo.quad_time))
			ent->s.effects |= EF_QUAD;
	}

	if (ent->monsterinfo.double_time > level.time)
	{
		if (G_PowerUpExpiring(ent->monsterinfo.double_time))
			ent->s.effects |= EF_DOUBLE;
	}

	if (ent->monsterinfo.invincible_time > level.time)
	{
		if (G_PowerUpExpiring(ent->monsterinfo.invincible_time))
			ent->s.effects |= EF_PENT;
	}
}

bool M_AllowSpawn( edict_t * self ) {
	if ( deathmatch->integer && !ai_allow_dm_spawn->integer ) {
		return false;
	}
	return true;
}

void M_SetAnimation(edict_t *self, const save_mmove_t &move, bool instant)
{
	// [Paril-KEX] free the beams if we switch animations.
	if (self->beam)
	{
		G_FreeEdict(self->beam);
		self->beam = nullptr;
	}

	if (self->beam2)
	{
		G_FreeEdict(self->beam2);
		self->beam2 = nullptr;
	}

	// instant switches will cause active_move to change on the next frame
	if (instant)
	{
		self->monsterinfo.active_move = move;
		self->monsterinfo.next_move = nullptr;
		return;
	}

	// these wait until the frame is ready to be finished
	self->monsterinfo.next_move = move;
}

void M_MoveFrame(edict_t *self)
{
	const mmove_t *move = self->monsterinfo.active_move.pointer();

	// [Paril-KEX] high tick rate adjustments;
	// monsters still only step frames and run thinkfunc's at
	// 10hz, but will run aifuncs at full speed with
	// distance spread over 10hz

	self->nextthink = level.time + FRAME_TIME_S;

	// time to run next 10hz move yet?
	bool run_frame = self->monsterinfo.next_move_time <= level.time;

	// we asked nicely to switch frames when the timer ran up
	if (run_frame && self->monsterinfo.next_move.pointer() && self->monsterinfo.active_move != self->monsterinfo.next_move)
	{
		M_SetAnimation(self, self->monsterinfo.next_move, true);
		move = self->monsterinfo.active_move.pointer();
	}

	if (!move)
		return;

	// no, but maybe we were explicitly forced into another move (pain,
	// death, etc)
	if (!run_frame)
		run_frame = (self->s.frame < move->firstframe || self->s.frame > move->lastframe);

	if (run_frame)
	{
		// [Paril-KEX] allow next_move and nextframe to work properly after an endfunc
		bool explicit_frame = false;

		if ((self->monsterinfo.nextframe) && (self->monsterinfo.nextframe >= move->firstframe) &&
			(self->monsterinfo.nextframe <= move->lastframe))
		{
			self->s.frame = self->monsterinfo.nextframe;
			self->monsterinfo.nextframe = 0;
		}
		else
		{
			if (self->s.frame == move->lastframe)
			{
				if (move->endfunc)
				{
					move->endfunc(self);

					if (self->monsterinfo.next_move)
					{
						M_SetAnimation(self, self->monsterinfo.next_move, true);

						if (self->monsterinfo.nextframe)
						{
							self->s.frame = self->monsterinfo.nextframe;
							self->monsterinfo.nextframe = 0;
							explicit_frame = true;
						}
					}

					// regrab move, endfunc is very likely to change it
					move = self->monsterinfo.active_move.pointer();

					// check for death
					if (self->svflags & SVF_DEADMONSTER)
						return;
				}
			}

			if (self->s.frame < move->firstframe || self->s.frame > move->lastframe)
			{
				self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
				self->s.frame = move->firstframe;
			}
			else if (!explicit_frame)
			{
				if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
				{
					self->s.frame++;
					if (self->s.frame > move->lastframe)
						self->s.frame = move->firstframe;
				}
			}
		}

		if (self->monsterinfo.aiflags & AI_HIGH_TICK_RATE)
			self->monsterinfo.next_move_time = level.time;
		else
			self->monsterinfo.next_move_time = level.time + 10_hz;

		if ((self->monsterinfo.nextframe) && !((self->monsterinfo.nextframe >= move->firstframe) &&
			(self->monsterinfo.nextframe <= move->lastframe)))
			self->monsterinfo.nextframe = 0;
	}

	// NB: frame thinkfunc can be called on the same frame
	// as the animation changing

	int32_t index = self->s.frame - move->firstframe;
	if (move->frame[index].aifunc)
	{
		if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
		{
			float dist = move->frame[index].dist * self->monsterinfo.scale;
			dist /= gi.tick_rate / 10;
			move->frame[index].aifunc(self, dist);
		}
		else
			move->frame[index].aifunc(self, 0);
	}

	if (run_frame && move->frame[index].thinkfunc)
		move->frame[index].thinkfunc(self);

	if (move->frame[index].lerp_frame != -1)
	{
		self->s.renderfx |= RF_OLD_FRAME_LERP;
		self->s.old_frame = move->frame[index].lerp_frame;
	}
}

void G_MonsterKilled(edict_t *self)
{
	level.killed_monsters++;

	if (coop->integer && self->enemy && self->enemy->client)
		self->enemy->client->resp.score++;

	if (g_debug_monster_kills->integer)
	{
		bool found = false;

		for (auto &ent : level.monsters_registered)
		{
			if (ent == self)
			{
				ent = nullptr;
				found = true;
				break;
			}
		}

		if (!found)
		{
#if defined(_DEBUG) && defined(KEX_PLATFORM_WINPC)
			__debugbreak();
#endif
			gi.Center_Print(&g_edicts[1], "found missing monster?");
		}

		if (level.killed_monsters == level.total_monsters)
		{
			gi.Center_Print(&g_edicts[1], "all monsters dead");
		}
	}
}

void M_ProcessPain(edict_t *e)
{
	if (!e->monsterinfo.damage_blood)
		return;

	if (e->health <= 0)
	{
		// ROGUE
		if (e->monsterinfo.aiflags & AI_MEDIC)
		{
			if (e->enemy && e->enemy->inuse && (e->enemy->svflags & SVF_MONSTER)) // god, I hope so
			{
				cleanupHealTarget(e->enemy);
			}

			// clean up self
			e->monsterinfo.aiflags &= ~AI_MEDIC;
		}
		// ROGUE

		if (!e->deadflag)
		{
			e->enemy = e->monsterinfo.damage_attacker;

			// ROGUE
			// ROGUE - free up slot for spawned monster if it's spawned
			if (e->monsterinfo.aiflags & AI_SPAWNED_CARRIER)
			{
				if (e->monsterinfo.commander && e->monsterinfo.commander->inuse &&
					!strcmp(e->monsterinfo.commander->classname, "monster_carrier"))
					e->monsterinfo.commander->monsterinfo.monster_slots++;
				e->monsterinfo.commander = nullptr;
			}
			if (e->monsterinfo.aiflags & AI_SPAWNED_WIDOW)
			{
				// need to check this because we can have variable numbers of coop players
				if (e->monsterinfo.commander && e->monsterinfo.commander->inuse &&
					!strncmp(e->monsterinfo.commander->classname, "monster_widow", 13))
				{
					if (e->monsterinfo.commander->monsterinfo.monster_used > 0)
						e->monsterinfo.commander->monsterinfo.monster_used--;
					e->monsterinfo.commander = nullptr;
				}
			}

			if (!(e->monsterinfo.aiflags & AI_DO_NOT_COUNT) && !(e->spawnflags & SPAWNFLAG_MONSTER_DEAD))
				G_MonsterKilled(e);
		
			e->touch = nullptr;
			monster_death_use(e);
		}

		e->die(e, e->monsterinfo.damage_inflictor, e->monsterinfo.damage_attacker, e->monsterinfo.damage_blood, e->monsterinfo.damage_from, e->monsterinfo.damage_mod);
		
		// [Paril-KEX] medic commander only gets his slots back after the monster is gibbed, since we can revive them
		if (e->health <= e->gib_health)
		{
			if (e->monsterinfo.aiflags & AI_SPAWNED_MEDIC_C)
			{
				if (e->monsterinfo.commander && e->monsterinfo.commander->inuse && !strcmp(e->monsterinfo.commander->classname, "monster_medic_commander"))
					e->monsterinfo.commander->monsterinfo.monster_used -= e->monsterinfo.monster_slots;

				e->monsterinfo.commander = nullptr;
			}
		}

		if (e->inuse && e->health > e->gib_health && e->s.frame == e->monsterinfo.active_move->lastframe)
		{
			e->s.frame -= irandom(1, 3);

			if (e->groundentity && e->movetype == MOVETYPE_TOSS && !(e->flags & FL_STATIONARY))
				e->s.angles[1] += brandom() ? 4.5f : -4.5f;
		}
	}
	else
		e->pain(e, e->monsterinfo.damage_attacker, (float) e->monsterinfo.damage_knockback, e->monsterinfo.damage_blood, e->monsterinfo.damage_mod);

	if (!e->inuse)
		return;

	if (e->monsterinfo.setskin)
		e->monsterinfo.setskin(e);

	e->monsterinfo.damage_blood = 0;
	e->monsterinfo.damage_knockback = 0;
	e->monsterinfo.damage_attacker = e->monsterinfo.damage_inflictor = nullptr;

	// [Paril-KEX] fire health target
	if (e->healthtarget)
	{
		const char *target = e->target;
		e->target = e->healthtarget;
		G_UseTargets(e, e->enemy);
		e->target = target;
	}
}

//
// Monster utility functions
//
THINK(monster_dead_think) (edict_t *self) -> void
{
	// flies
	if ((self->monsterinfo.aiflags & AI_STINKY) && !(self->monsterinfo.aiflags & AI_STUNK))
	{
		if (!self->fly_sound_debounce_time)
			self->fly_sound_debounce_time = level.time + random_time(5_sec, 15_sec);
		else if (self->fly_sound_debounce_time < level.time)
		{
			if (!self->s.sound)
			{
				self->s.effects |= EF_FLIES;
				self->s.sound = gi.soundindex("infantry/inflies1.wav");
				self->fly_sound_debounce_time = level.time + 60_sec;
			}
			else
			{
				self->s.effects &= ~EF_FLIES;
				self->s.sound = 0;
				self->monsterinfo.aiflags |= AI_STUNK;
			}
		}
	}

	if (!self->monsterinfo.damage_blood)
	{
		if (self->s.frame != self->monsterinfo.active_move->lastframe)
			self->s.frame++;
	}

	self->nextthink = level.time + 10_hz;
}

void monster_dead(edict_t *self)
{
	self->think = monster_dead_think;
	self->nextthink = level.time + 10_hz;
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->monsterinfo.damage_blood = 0;
	self->fly_sound_debounce_time = 0_ms;
	self->monsterinfo.aiflags &= ~AI_STUNK;
	gi.linkentity(self);
}

/*
=============
infront

returns 1 if the entity is in front (in sight) of self
=============
*/
static bool projectile_infront(edict_t *self, edict_t *other)
{
	vec3_t vec;
	float  dot;
	vec3_t forward;

	AngleVectors(self->s.angles, forward, nullptr, nullptr);
	vec = other->s.origin - self->s.origin;
	vec.normalize();
	dot = vec.dot(forward);
	return dot > 0.35f;
}

static BoxEdictsResult_t M_CheckDodge_BoxEdictsFilter(edict_t *ent, void *data)
{
	edict_t *self = (edict_t *) data;

	// not a valid projectile
	if (!(ent->svflags & SVF_PROJECTILE) || !(ent->flags & FL_DODGE))
		return BoxEdictsResult_t::Skip;

	// not moving
	if (ent->velocity.lengthSquared() < 16.f)
		return BoxEdictsResult_t::Skip;

	// projectile is behind us, we can't see it
	if (!projectile_infront(self, ent))
		return BoxEdictsResult_t::Skip;

	// will it hit us within 1 second? gives us enough time to dodge
	trace_t tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, ent->s.origin + ent->velocity, ent, ent->clipmask);

	if (tr.ent == self)
	{
		vec3_t v = tr.endpos - ent->s.origin;
		gtime_t eta = gtime_t::from_sec(v.length() / ent->velocity.length());

		self->monsterinfo.dodge(self, ent->owner, eta, &tr, (ent->movetype == MOVETYPE_BOUNCE || ent->movetype == MOVETYPE_TOSS));

		return BoxEdictsResult_t::End;
	}

	return BoxEdictsResult_t::Skip;
}

// [Paril-KEX] active checking for projectiles to dodge
static void M_CheckDodge(edict_t *self)
{
	// we recently made a valid dodge, don't try again for a bit
	if (self->monsterinfo.dodge_time > level.time)
		return;

	gi.BoxEdicts(self->absmin - vec3_t{512, 512, 512}, self->absmax + vec3_t{512, 512, 512}, nullptr, 0, AREA_SOLID, M_CheckDodge_BoxEdictsFilter, self);
}

static bool CheckPathVisibility(const vec3_t &start, const vec3_t &end)
{
	trace_t tr = gi.traceline(start, end, nullptr, MASK_SOLID | CONTENTS_PROJECTILECLIP | CONTENTS_MONSTERCLIP | CONTENTS_PLAYERCLIP);

	bool valid = tr.fraction == 1.0f;

	if (!valid)
	{
		// try raising some of the points
		bool can_raise_start = false, can_raise_end = false;
		vec3_t raised_start = start + vec3_t{0.f, 0.f, 16.f};
		vec3_t raised_end = end + vec3_t{0.f, 0.f, 16.f};

		if (gi.traceline(start, raised_start, nullptr, MASK_SOLID | CONTENTS_PROJECTILECLIP | CONTENTS_MONSTERCLIP | CONTENTS_PLAYERCLIP).fraction == 1.0f)
			can_raise_start = true;

		if (gi.traceline(end, raised_end, nullptr, MASK_SOLID | CONTENTS_PROJECTILECLIP | CONTENTS_MONSTERCLIP | CONTENTS_PLAYERCLIP).fraction == 1.0f)
			can_raise_end = true;

		// try raised start -> end
		if (can_raise_start)
		{
			tr = gi.traceline(raised_start, end, nullptr, MASK_SOLID | CONTENTS_PROJECTILECLIP | CONTENTS_MONSTERCLIP | CONTENTS_PLAYERCLIP);

			if (tr.fraction == 1.0f)
				return true;
		}

		// try start -> raised end
		if (can_raise_end)
		{
			tr = gi.traceline(start, raised_end, nullptr, MASK_SOLID | CONTENTS_PROJECTILECLIP | CONTENTS_MONSTERCLIP | CONTENTS_PLAYERCLIP);

			if (tr.fraction == 1.0f)
				return true;
		}

		// try both raised
		if (can_raise_start && can_raise_end)
		{
			tr = gi.traceline(raised_start, raised_end, nullptr, MASK_SOLID | CONTENTS_PROJECTILECLIP | CONTENTS_MONSTERCLIP | CONTENTS_PLAYERCLIP);

			if (tr.fraction == 1.0f)
				return true;
		}

		//gi.Draw_Line(start, end, rgba_red, 0.1f, false);
	}

	return valid;
}

THINK(monster_think) (edict_t *self) -> void
{
	// [Paril-KEX] monster sniff testing; if we can make an unobstructed path to the player, murder ourselves.
	if (g_debug_monster_kills->integer)
	{
		if (g_edicts[1].inuse)
		{
			trace_t enemy_trace = gi.traceline(self->s.origin, g_edicts[1].s.origin, self, MASK_SHOT);

			if (enemy_trace.fraction < 1.0f && enemy_trace.ent == &g_edicts[1])
				T_Damage(self, &g_edicts[1], &g_edicts[1], { 0, 0, -1 }, self->s.origin, { 0, 0, -1 }, 9999, 9999, DAMAGE_NO_PROTECTION, MOD_BFG_BLAST);
			else
			{
				static vec3_t points[64];

				if (self->disintegrator_time <= level.time)
				{
					PathRequest request;
					request.goal = g_edicts[1].s.origin;
					request.moveDist = 4.0f;
					request.nodeSearch.ignoreNodeFlags = true;
					request.nodeSearch.radius = 9999;
					request.pathFlags = PathFlags::All;
					request.start = self->s.origin;
					request.traversals.dropHeight = 9999;
					request.traversals.jumpHeight = 9999;
					request.pathPoints.array = points;
					request.pathPoints.count = q_countof(points);

					PathInfo info;

					if (gi.GetPathToGoal(request, info))
					{
						if (info.returnCode != PathReturnCode::NoStartNode &&
							info.returnCode != PathReturnCode::NoGoalNode &&
							info.returnCode != PathReturnCode::NoPathFound &&
							info.returnCode != PathReturnCode::NoNavAvailable &&
							info.numPathPoints < q_countof(points))
						{
							if (CheckPathVisibility(g_edicts[1].s.origin + vec3_t { 0.f, 0.f, g_edicts[1].mins.z }, points[info.numPathPoints - 1]) &&
								CheckPathVisibility(self->s.origin + vec3_t { 0.f, 0.f, self->mins.z }, points[0]))
							{
								size_t i = 0;

								for (; i < info.numPathPoints - 1; i++)
									if (!CheckPathVisibility(points[i], points[i + 1]))
										break;

								if (i == info.numPathPoints - 1)
									T_Damage(self, &g_edicts[1], &g_edicts[1], { 0, 0, 1 }, self->s.origin, { 0, 0, 1 }, 9999, 9999, DAMAGE_NO_PROTECTION, MOD_BFG_BLAST);
								else
									self->disintegrator_time = level.time + 500_ms;
							}
							else
								self->disintegrator_time = level.time + 500_ms;
						}
						else
						{
							self->disintegrator_time = level.time + 1_sec;
						}
					}
					else
					{
						self->disintegrator_time = level.time + 1_sec;
					}
				}
			}

			if (!self->deadflag && !(self->monsterinfo.aiflags & AI_DO_NOT_COUNT))
				gi.Draw_Bounds(self->absmin, self->absmax, rgba_red, gi.frame_time_s, false);
		}
	}

	self->s.renderfx &= ~(RF_STAIR_STEP | RF_OLD_FRAME_LERP);

	M_ProcessPain(self);

	// pain/die above freed us
	if (!self->inuse || self->think != monster_think)
		return;

	if (self->hackflags & HACKFLAG_ATTACK_PLAYER)
	{
		if (!self->enemy && g_edicts[1].inuse)
		{
			self->enemy = &g_edicts[1];
			FoundTarget(self);
		}
	}

	if (self->health > 0 && self->monsterinfo.dodge && !(globals.server_flags & SERVER_FLAG_LOADING))
		M_CheckDodge(self);

	M_MoveFrame(self);
	if (self->linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround(self, G_GetClipMask(self));
	}
	M_CatagorizePosition(self, self->s.origin, self->waterlevel, self->watertype);
	M_WorldEffects(self);
	M_SetEffects(self);
}

/*
================
monster_use

Using a monster makes it angry at the current activator
================
*/
USE(monster_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	if (self->enemy)
		return;
	if (self->health <= 0)
		return;
	if (!activator)
		return;
	if (activator->flags & FL_NOTARGET)
		return;
	if (!(activator->client) && !(activator->monsterinfo.aiflags & AI_GOOD_GUY))
		return;
	if (activator->flags & FL_DISGUISED) // PGM
		return;							 // PGM

	// delay reaction so if the monster is teleported, its sound is still heard
	self->enemy = activator;
	FoundTarget(self);
}

void monster_start_go(edict_t *self);

THINK(monster_triggered_spawn) (edict_t *self) -> void
{
	self->s.origin[2] += 1;

	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->svflags &= ~SVF_NOCLIENT;
	self->air_finished = level.time + 12_sec;
	gi.linkentity(self);

	KillBox(self, false);

	monster_start_go(self);

	// RAFAEL
	if (strcmp(self->classname, "monster_fixbot") == 0)
	{
		if (self->spawnflags.has(SPAWNFLAG_FIXBOT_LANDING | SPAWNFLAG_FIXBOT_TAKEOFF | SPAWNFLAG_FIXBOT_FIXIT))
		{
			self->enemy = nullptr;
			return;
		}
	}
	// RAFAEL

	if (self->enemy && !(self->spawnflags & SPAWNFLAG_MONSTER_AMBUSH) && !(self->enemy->flags & FL_NOTARGET) && !(self->monsterinfo.aiflags & AI_GOOD_GUY))
	{
		// ROGUE
		if (!(self->enemy->flags & FL_DISGUISED))
			// ROGUE
			FoundTarget(self);
		// ROGUE
		else // PMM - just in case, make sure to clear the enemy so FindTarget doesn't get confused
			self->enemy = nullptr;
		// ROGUE
	}
	else
	{
		self->enemy = nullptr;
	}
}

USE(monster_triggered_spawn_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	// we have a one frame delay here so we don't telefrag the guy who activated us
	self->think = monster_triggered_spawn;
	self->nextthink = level.time + FRAME_TIME_S;
	if (activator && activator->client && !(self->hackflags & HACKFLAG_END_CUTSCENE))
		self->enemy = activator;
	self->use = monster_use;

	if (self->spawnflags.has(SPAWNFLAG_MONSTER_SCENIC))
	{
		M_droptofloor(self);

		self->nextthink = 0_ms;
		self->think(self);

		if (self->spawnflags.has(SPAWNFLAG_MONSTER_AMBUSH))
			monster_use(self, other, activator);

		for (int i = 0; i < 30; i++)
		{
			self->think(self);
			self->monsterinfo.next_move_time = 0_ms;
		}
	}
}

THINK(monster_triggered_think) (edict_t *self) -> void
{
	if (!(self->monsterinfo.aiflags & AI_DO_NOT_COUNT))
		gi.Draw_Bounds(self->absmin, self->absmax, rgba_blue, gi.frame_time_s, false);

	self->nextthink = level.time + 1_ms;
}

void monster_triggered_start(edict_t *self)
{
	self->solid = SOLID_NOT;
	self->movetype = MOVETYPE_NONE;
	self->svflags |= SVF_NOCLIENT;
	self->nextthink = 0_ms;
	self->use = monster_triggered_spawn_use;

	if (g_debug_monster_kills->integer)
	{
		self->think = monster_triggered_think;
		self->nextthink = level.time + 1_ms;
	}

	if (!self->targetname ||
		(G_FindByString<&edict_t::target>(nullptr, self->targetname) == nullptr &&
		 G_FindByString<&edict_t::pathtarget>(nullptr, self->targetname) == nullptr &&
		 G_FindByString<&edict_t::deathtarget>(nullptr, self->targetname) == nullptr &&
		 G_FindByString<&edict_t::itemtarget>(nullptr, self->targetname) == nullptr &&
		 G_FindByString<&edict_t::healthtarget>(nullptr, self->targetname) == nullptr &&
		 G_FindByString<&edict_t::combattarget>(nullptr, self->targetname) == nullptr))
	{
		gi.Com_PrintFmt("{}: is trigger spawned, but has no targetname or no entity to spawn it\n", *self);
	}
}

/*
================
monster_death_use

When a monster dies, it fires all of its targets with the current
enemy as activator.
================
*/
void monster_death_use(edict_t *self)
{
	self->flags &= ~(FL_FLY | FL_SWIM);
	self->monsterinfo.aiflags &= (AI_DOUBLE_TROUBLE | AI_GOOD_GUY | AI_STINKY | AI_SPAWNED_MASK);

	if (self->item)
	{
		edict_t *dropped = Drop_Item(self, self->item);

		if (self->itemtarget)
		{
			dropped->target = self->itemtarget;
			self->itemtarget = nullptr;
		}

		self->item = nullptr;
	}

	if (self->deathtarget)
		self->target = self->deathtarget;

	if (self->target)
		G_UseTargets(self, self->enemy);

	// [Paril-KEX] fire health target
	if (self->healthtarget)
	{
		self->target = self->healthtarget;
		G_UseTargets(self, self->enemy);
	}
}

// [Paril-KEX] adjust the monster's health from how
// many active players we have
void G_Monster_ScaleCoopHealth(edict_t *self)
{
	// already scaled
	if (self->monsterinfo.health_scaling >= level.coop_scale_players)
		return;

	// this is just to fix monsters that change health after spawning...
	// looking at you, soldiers
	if (!self->monsterinfo.base_health)
		self->monsterinfo.base_health = self->max_health;

	int32_t delta = level.coop_scale_players - self->monsterinfo.health_scaling;
	int32_t additional_health = delta * (int32_t) (self->monsterinfo.base_health * level.coop_health_scaling);

	self->health = max(1, self->health + additional_health);
	self->max_health += additional_health;

	self->monsterinfo.health_scaling = level.coop_scale_players;
}

struct monster_filter_t
{
	inline bool operator()(edict_t *self) const
	{
		return self->inuse && (self->flags & FL_COOP_HEALTH_SCALE) && self->health > 0;
	}
};

// check all active monsters' scaling
void G_Monster_CheckCoopHealthScaling()
{
	for (auto monster : entity_iterable_t<monster_filter_t>())
		G_Monster_ScaleCoopHealth(monster);
}

//============================================================================
constexpr spawnflags_t SPAWNFLAG_MONSTER_FUBAR = 4_spawnflag;

bool monster_start(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return false;
	}

	if (self->spawnflags.has(SPAWNFLAG_MONSTER_SCENIC))
		self->monsterinfo.aiflags |= AI_GOOD_GUY;

	// [Paril-KEX] n64
	if (self->hackflags & (HACKFLAG_END_CUTSCENE | HACKFLAG_ATTACK_PLAYER))
		self->monsterinfo.aiflags |= AI_DO_NOT_COUNT;

	if (self->spawnflags.has(SPAWNFLAG_MONSTER_FUBAR) && !(self->monsterinfo.aiflags & AI_GOOD_GUY))
	{
		self->spawnflags &= ~SPAWNFLAG_MONSTER_FUBAR;
		self->spawnflags |= SPAWNFLAG_MONSTER_AMBUSH;
	}

	// [Paril-KEX] simplify other checks
	if (self->monsterinfo.aiflags & AI_GOOD_GUY)
		self->monsterinfo.aiflags |= AI_DO_NOT_COUNT;

	// ROGUE
	if (!(self->monsterinfo.aiflags & AI_DO_NOT_COUNT) && !self->spawnflags.has(SPAWNFLAG_MONSTER_DEAD))
	{
		if (g_debug_monster_kills->integer)
			level.monsters_registered[level.total_monsters] = self;
		// ROGUE
		level.total_monsters++;
	}

	self->nextthink = level.time + FRAME_TIME_S;
	self->svflags |= SVF_MONSTER;
	self->takedamage = true;
	self->air_finished = level.time + 12_sec;
	self->use = monster_use;
	self->max_health = self->health;
	self->clipmask = MASK_MONSTERSOLID;
	self->deadflag = false;
	self->svflags &= ~SVF_DEADMONSTER;
	self->flags &= ~FL_ALIVE_KNOCKBACK_ONLY;
	self->flags |= FL_COOP_HEALTH_SCALE;
	self->s.old_origin = self->s.origin;
	self->monsterinfo.initial_power_armor_type = self->monsterinfo.power_armor_type;
	self->monsterinfo.max_power_armor_power = self->monsterinfo.power_armor_power;

	if (!self->monsterinfo.checkattack)
		self->monsterinfo.checkattack = M_CheckAttack;

	if ( ai_model_scale->value > 0 ) {
		self->s.scale = ai_model_scale->value;
	}

	if (self->s.scale)
	{
		self->monsterinfo.scale *= self->s.scale;
		self->mins *= self->s.scale;
		self->maxs *= self->s.scale;
		self->mass *= self->s.scale;
	}

	// set combat style if unset
	if (self->monsterinfo.combat_style == COMBAT_UNKNOWN)
	{
		if (!self->monsterinfo.attack && self->monsterinfo.melee)
			self->monsterinfo.combat_style = COMBAT_MELEE;
		else
			self->monsterinfo.combat_style = COMBAT_MIXED;
	}

	if (st.item)
	{
		self->item = FindItemByClassname(st.item);
		if (!self->item)
			gi.Com_PrintFmt("{}: bad item: {}\n", *self, st.item);
	}

	// randomize what frame they start on
	if (self->monsterinfo.active_move)
		self->s.frame =
			irandom(self->monsterinfo.active_move->firstframe, self->monsterinfo.active_move->lastframe + 1);

	// PMM - get this so I don't have to do it in all of the monsters
	self->monsterinfo.base_height = self->maxs[2];

	// Paril: monsters' old default viewheight (25)
	// is all messed up for certain monsters. Calculate
	// from maxs to make a bit more sense.
	if (!self->viewheight)
		self->viewheight = (int) (self->maxs[2] - 8.f);

	// PMM - clear these
	self->monsterinfo.quad_time = 0_ms;
	self->monsterinfo.double_time = 0_ms;
	self->monsterinfo.invincible_time = 0_ms;

	// set base health & set base scaling to 1 player
	self->monsterinfo.base_health = self->health;
	self->monsterinfo.health_scaling = 1;

	// [Paril-KEX] co-op health scale
	G_Monster_ScaleCoopHealth(self);

	return true;
}

stuck_result_t G_FixStuckObject(edict_t *self, vec3_t check)
{
	contents_t mask = G_GetClipMask(self);
	stuck_result_t result = G_FixStuckObject_Generic(check, self->mins, self->maxs, [self, mask] (const vec3_t &start, const vec3_t &mins, const vec3_t &maxs, const vec3_t &end) {
		return gi.trace(start, mins, maxs, end, self, mask);
	});

	if (result == stuck_result_t::NO_GOOD_POSITION)
		return result;

	self->s.origin = check;

	if (result == stuck_result_t::FIXED)
		gi.Com_PrintFmt("fixed stuck {}\n", *self);

	return result;
}

void monster_start_go(edict_t *self)
{
	// Paril: moved here so this applies to swim/fly monsters too
	if (!(self->flags & FL_STATIONARY))
	{
		const vec3_t check = self->s.origin;

		// [Paril-KEX] different nudge method; see if any of the bbox sides are clear,
		// if so we can see how much headroom we have in that direction and shift us.
		// most of the monsters stuck in solids will only be stuck on one side, which
		// conveniently leaves only one side not in a solid; this won't fix monsters
		// stuck in a corner though.
		bool is_stuck = false;

		if ((self->monsterinfo.aiflags & AI_GOOD_GUY) || (self->flags & (FL_FLY | FL_SWIM)))
			is_stuck = gi.trace(self->s.origin, self->mins, self->maxs, self->s.origin, self, MASK_MONSTERSOLID).startsolid;
		else
			is_stuck = !M_droptofloor(self) || !M_walkmove(self, 0, 0);

		if (is_stuck)
		{
			if (G_FixStuckObject(self, check) != stuck_result_t::NO_GOOD_POSITION)
			{
				if (self->monsterinfo.aiflags & AI_GOOD_GUY)
					is_stuck = gi.trace(self->s.origin, self->mins, self->maxs, self->s.origin, self, MASK_MONSTERSOLID).startsolid;
				else if (!(self->flags & (FL_FLY | FL_SWIM)))
					M_droptofloor(self);
				is_stuck = false;
			}
		}

		// last ditch effort: brute force
		if (is_stuck)
		{
			// Paril: try nudging them out. this fixes monsters stuck
			// in very shallow slopes.
			constexpr const int32_t adjust[] = { 0, -1, 1, -2, 2, -4, 4, -8, 8 };
			bool					walked = false;

			for (int32_t y = 0; !walked && y < 3; y++)
				for (int32_t x = 0; !walked && x < 3; x++)
					for (int32_t z = 0; !walked && z < 3; z++)
					{
						self->s.origin[0] = check[0] + adjust[x];
						self->s.origin[1] = check[1] + adjust[y];
						self->s.origin[2] = check[2] + adjust[z];
						
						if (self->monsterinfo.aiflags & AI_GOOD_GUY)
						{
							is_stuck = gi.trace(self->s.origin, self->mins, self->maxs, self->s.origin, self, MASK_MONSTERSOLID).startsolid;

							if (!is_stuck)
								walked = true;
						}
						else if (!(self->flags & (FL_FLY | FL_SWIM)))
						{
							M_droptofloor(self);
							walked = M_walkmove(self, 0, 0);
						}
					}
		}

		if (is_stuck)
			gi.Com_PrintFmt("WARNING: {} stuck in solid\n", *self);
	}

	vec3_t v;

	if (self->health <= 0)
		return;

	self->s.old_origin = self->s.origin;

	// check for target to combat_point and change to combattarget
	if (self->target)
	{
		bool	 notcombat;
		bool	 fixup;
		edict_t *target;

		target = nullptr;
		notcombat = false;
		fixup = false;
		while ((target = G_FindByString<&edict_t::targetname>(target, self->target)) != nullptr)
		{
			if (strcmp(target->classname, "point_combat") == 0)
			{
				self->combattarget = self->target;
				fixup = true;
			}
			else
			{
				notcombat = true;
			}
		}
		if (notcombat && self->combattarget)
			gi.Com_PrintFmt("{}: has target with mixed types\n", *self);
		if (fixup)
			self->target = nullptr;
	}

	// validate combattarget
	if (self->combattarget)
	{
		edict_t *target;

		target = nullptr;
		while ((target = G_FindByString<&edict_t::targetname>(target, self->combattarget)) != nullptr)
		{
			if (strcmp(target->classname, "point_combat") != 0)
			{
				gi.Com_PrintFmt("{} has a bad combattarget {} ({})\n", *self, self->combattarget, *target);
			}
		}
	}

	// allow spawning dead
	bool spawn_dead = self->spawnflags.has(SPAWNFLAG_MONSTER_DEAD);

	if (self->target)
	{
		self->goalentity = self->movetarget = G_PickTarget(self->target);
		if (!self->movetarget)
		{
			gi.Com_PrintFmt("{}: can't find target {}\n", *self, self->target);
			self->target = nullptr;
			self->monsterinfo.pausetime = HOLD_FOREVER;
			if (!spawn_dead)
				self->monsterinfo.stand(self);
		}
		else if (strcmp(self->movetarget->classname, "path_corner") == 0)
		{
			v = self->goalentity->s.origin - self->s.origin;
			self->ideal_yaw = self->s.angles[YAW] = vectoyaw(v);
			if (!spawn_dead)
				self->monsterinfo.walk(self);
			self->target = nullptr;
		}
		else
		{
			self->goalentity = self->movetarget = nullptr;
			self->monsterinfo.pausetime = HOLD_FOREVER;
			if (!spawn_dead)
				self->monsterinfo.stand(self);
		}
	}
	else
	{
		self->monsterinfo.pausetime = HOLD_FOREVER;
		if (!spawn_dead)
			self->monsterinfo.stand(self);
	}
	
	if (spawn_dead)
	{
		// to spawn dead, we'll mimick them dying naturally
		self->health = 0;

		vec3_t f = self->s.origin;

		if (self->die)
			self->die(self, self, self, 0, vec3_origin, MOD_SUICIDE);

		if (!self->inuse)
			return;

		if (self->monsterinfo.setskin)
			self->monsterinfo.setskin(self);

		self->monsterinfo.aiflags |= AI_SPAWNED_DEAD;

		auto move = self->monsterinfo.active_move.pointer();

		for (size_t i = move->firstframe; i < move->lastframe; i++)
		{
			self->s.frame = i;

			if (move->frame[i - move->firstframe].thinkfunc)
				move->frame[i - move->firstframe].thinkfunc(self);

			if (!self->inuse)
				return;
		}

		if (move->endfunc)
			move->endfunc(self);

		if (!self->inuse)
			return;

		if (self->monsterinfo.start_frame)
			self->s.frame = self->monsterinfo.start_frame;
		else
			self->s.frame = move->lastframe;

		self->s.origin = f;
		gi.linkentity(self);

		self->monsterinfo.aiflags &= ~AI_SPAWNED_DEAD;
	}
	else
	{
		self->think = monster_think;
		self->nextthink = level.time + FRAME_TIME_S;
		self->monsterinfo.aiflags |= AI_SPAWNED_ALIVE;
	}
}

THINK(walkmonster_start_go) (edict_t *self) -> void
{
	if (!self->yaw_speed)
		self->yaw_speed = 20;

	if (self->spawnflags.has(SPAWNFLAG_MONSTER_TRIGGER_SPAWN))
		monster_triggered_start(self);
	else
		monster_start_go(self);
}

void walkmonster_start(edict_t *self)
{
	self->think = walkmonster_start_go;
	monster_start(self);
}

THINK(flymonster_start_go) (edict_t *self) -> void
{
	if (!self->yaw_speed)
		self->yaw_speed = 30;

	if (self->spawnflags.has(SPAWNFLAG_MONSTER_TRIGGER_SPAWN))
		monster_triggered_start(self);
	else
		monster_start_go(self);
}

void flymonster_start(edict_t *self)
{
	self->flags |= FL_FLY;
	self->think = flymonster_start_go;
	monster_start(self);
}

THINK(swimmonster_start_go) (edict_t *self) -> void
{
	if (!self->yaw_speed)
		self->yaw_speed = 30;

	if (self->spawnflags.has(SPAWNFLAG_MONSTER_TRIGGER_SPAWN))
		monster_triggered_start(self);
	else
		monster_start_go(self);
}

void swimmonster_start(edict_t *self)
{
	self->flags |= FL_SWIM;
	self->think = swimmonster_start_go;
	monster_start(self);
}

USE(trigger_health_relay_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	float percent_health = clamp((float) (other->health) / (float) (other->max_health), 0.f, 1.f);

	// not ready to trigger yet
	if (percent_health > self->speed)
		return;

	// fire!
	G_UseTargets(self, activator);

	// kill self
	G_FreeEdict(self);
}

/*QUAKED trigger_health_relay (1.0 1.0 0.0) (-8 -8 -8) (8 8 8)
Special type of relay that fires when a linked object is reduced
beyond a certain amount of health.

It will only fire once, and free itself afterwards.
*/
void SP_trigger_health_relay(edict_t *self)
{
	if (!self->targetname)
	{
		gi.Com_PrintFmt("{} missing targetname\n", *self);
		G_FreeEdict(self);
		return;
	}

	if (self->speed < 0 || self->speed > 100)
	{
		gi.Com_PrintFmt("{} has bad \"speed\" (health percentage); must be between 0 and 100, inclusive\n", *self);
		G_FreeEdict(self);
		return;
	}

	self->svflags |= SVF_NOCLIENT;
	self->use = trigger_health_relay_use;
}