// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "../g_local.h"

// RAFAEL
void fire_blueblaster(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, effects_t effect)
{
	edict_t *bolt;
	trace_t	 tr;

	bolt = G_Spawn();
	bolt->s.origin = start;
	bolt->s.old_origin = start;
	bolt->s.angles = vectoangles(dir);
	bolt->velocity = dir * speed;
	bolt->svflags |= SVF_PROJECTILE;
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->flags |= FL_DODGE;
	bolt->clipmask = MASK_PROJECTILE;
	bolt->solid = SOLID_BBOX;
	bolt->s.effects |= effect;
	bolt->s.modelindex = gi.modelindex("models/objects/laser/tris.md2");
	bolt->s.skinnum = 1;
	bolt->s.sound = gi.soundindex("misc/lasfly.wav");
	bolt->owner = self;
	bolt->touch = blaster_touch;
	bolt->nextthink = level.time + 2_sec;
	bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	bolt->classname = "bolt";
	bolt->style = MOD_BLUEBLASTER;
	gi.linkentity(bolt);

	tr = gi.traceline(self->s.origin, bolt->s.origin, bolt, bolt->clipmask);

	if (tr.fraction < 1.0f)
	{
		bolt->s.origin = tr.endpos + (tr.plane.normal * 1.f);
		bolt->touch(bolt, tr.ent, tr, false);
	}
}

// RAFAEL

/*
fire_ionripper
*/

THINK(ionripper_sparks) (edict_t *self) -> void
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_WELDING_SPARKS);
	gi.WriteByte(0);
	gi.WritePosition(self->s.origin);
	gi.WriteDir(vec3_origin);
	gi.WriteByte(irandom(0xe4, 0xe8));
	gi.multicast(self->s.origin, MULTICAST_PVS, false);

	G_FreeEdict(self);
}

// RAFAEL
TOUCH(ionripper_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	if (other == self->owner)
		return;

	if (tr.surface && (tr.surface->flags & SURF_SKY))
	{
		G_FreeEdict(self);
		return;
	}

	if (self->owner->client)
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	if (other->takedamage)
	{
		T_Damage(other, self, self->owner, self->velocity, self->s.origin, tr.plane.normal, self->dmg, 1, DAMAGE_ENERGY, MOD_RIPPER);
	}
	else
	{
		return;
	}

	G_FreeEdict(self);
}

// RAFAEL
void fire_ionripper(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, effects_t effect)
{
	edict_t *ion;
	trace_t	 tr;

	ion = G_Spawn();
	ion->s.origin = start;
	ion->s.old_origin = start;
	ion->s.angles = vectoangles(dir);
	ion->velocity = dir * speed;
	ion->movetype = MOVETYPE_WALLBOUNCE;
	ion->clipmask = MASK_PROJECTILE;

	// [Paril-KEX]
	if (self->client && !G_ShouldPlayersCollide(true))
		ion->clipmask &= ~CONTENTS_PLAYER;

	ion->solid = SOLID_BBOX;
	ion->s.effects |= effect;
	ion->svflags |= SVF_PROJECTILE;
	ion->flags |= FL_DODGE;
	ion->s.renderfx |= RF_FULLBRIGHT;
	ion->s.modelindex = gi.modelindex("models/objects/boomrang/tris.md2");
	ion->s.sound = gi.soundindex("misc/lasfly.wav");
	ion->owner = self;
	ion->touch = ionripper_touch;
	ion->nextthink = level.time + 3_sec;
	ion->think = ionripper_sparks;
	ion->dmg = damage;
	ion->dmg_radius = 100;
	gi.linkentity(ion);

	tr = gi.traceline(self->s.origin, ion->s.origin, ion, ion->clipmask);
	if (tr.fraction < 1.0f)
	{
		ion->s.origin = tr.endpos + (tr.plane.normal * 1.f);
		ion->touch(ion, tr.ent, tr, false);
	}
}

// RAFAEL
/*
fire_heat
*/

THINK(heat_think) (edict_t *self) -> void
{
	edict_t *target = nullptr;
	edict_t *acquire = nullptr;
	vec3_t	 vec;
	vec3_t	 oldang;
	float	 len;
	float	 oldlen = 0;
	float	 dot, olddot = 1;

	vec3_t fwd = AngleVectors(self->s.angles).forward;

	// acquire new target
	while ((target = findradius(target, self->s.origin, 1024)) != nullptr)
	{
		if (self->owner == target)
			continue;
		if (!target->client)
			continue;
		if (target->health <= 0)
			continue;
		if (!visible(self, target))
			continue;
		//if (!infront(self, target))
		//	continue;

		vec = self->s.origin - target->s.origin;
		len = vec.length();

		dot = vec.normalized().dot(fwd);

		// targets that require us to turn less are preferred
		if (dot >= olddot)
			continue;

		if (acquire == nullptr || dot < olddot || len < oldlen)
		{
			acquire = target;
			oldlen = len;
			olddot = dot;
		}
	}

	if (acquire != nullptr)
	{
		oldang = self->s.angles;
		vec = (acquire->s.origin - self->s.origin).normalized();
		float t = self->accel;

		float d = self->movedir.dot(vec);

		if (d < 0.45f && d > -0.45f)
			vec = -vec;

		self->movedir = slerp(self->movedir, vec, t).normalized();
		self->s.angles = vectoangles(self->movedir);

		if (!self->enemy)
		{
			gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/railgr1a.wav"), 1.f, 0.25f, 0);
			self->enemy = acquire;
		}
	}
	else
		self->enemy = nullptr;

	self->velocity = self->movedir * self->speed;
	self->nextthink = level.time + FRAME_TIME_MS;
}

// RAFAEL
void fire_heat(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, float damage_radius, int radius_damage, float turn_fraction)
{
	edict_t *heat;

	heat = G_Spawn();
	heat->s.origin = start;
	heat->movedir = dir;
	heat->s.angles = vectoangles(dir);
	heat->velocity = dir * speed;
	heat->flags |= FL_DODGE;
	heat->movetype = MOVETYPE_FLYMISSILE;
	heat->svflags |= SVF_PROJECTILE;
	heat->clipmask = MASK_PROJECTILE;
	heat->solid = SOLID_BBOX;
	heat->s.effects |= EF_ROCKET;
	heat->s.modelindex = gi.modelindex("models/objects/rocket/tris.md2");
	heat->owner = self;
	heat->touch = rocket_touch;
	heat->speed = speed;
	heat->accel = turn_fraction;

	heat->nextthink = level.time + FRAME_TIME_MS;
	heat->think = heat_think;

	heat->dmg = damage;
	heat->radius_dmg = radius_damage;
	heat->dmg_radius = damage_radius;
	heat->s.sound = gi.soundindex("weapons/rockfly.wav");

	gi.linkentity(heat);
}

// RAFAEL

/*
fire_plasma
*/

TOUCH(plasma_touch) (edict_t *ent, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	vec3_t origin;

	if (other == ent->owner)
		return;

	if (tr.surface && (tr.surface->flags & SURF_SKY))
	{
		G_FreeEdict(ent);
		return;
	}

	if (ent->owner->client)
		PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

	// calculate position for the explosion entity
	origin = ent->s.origin + (ent->velocity * -0.02f);

	if (other->takedamage)
	{
		T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, tr.plane.normal, ent->dmg, 0, DAMAGE_ENERGY, MOD_PHALANX);
	}

	T_RadiusDamage(ent, ent->owner, (float) ent->radius_dmg, other, ent->dmg_radius, DAMAGE_ENERGY, MOD_PHALANX);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_PLASMA_EXPLOSION);
	gi.WritePosition(origin);
	gi.multicast(ent->s.origin, MULTICAST_PHS, false);

	G_FreeEdict(ent);
}

// RAFAEL
void fire_plasma(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, float damage_radius, int radius_damage)
{
	edict_t *plasma;

	plasma = G_Spawn();
	plasma->s.origin = start;
	plasma->movedir = dir;
	plasma->s.angles = vectoangles(dir);
	plasma->velocity = dir * speed;
	plasma->movetype = MOVETYPE_FLYMISSILE;
	plasma->clipmask = MASK_PROJECTILE;

	// [Paril-KEX]
	if (self->client && !G_ShouldPlayersCollide(true))
		plasma->clipmask &= ~CONTENTS_PLAYER;

	plasma->solid = SOLID_BBOX;
	plasma->svflags |= SVF_PROJECTILE;
	plasma->flags |= FL_DODGE;
	plasma->owner = self;
	plasma->touch = plasma_touch;
	plasma->nextthink = level.time + gtime_t::from_sec(8000.f / speed);
	plasma->think = G_FreeEdict;
	plasma->dmg = damage;
	plasma->radius_dmg = radius_damage;
	plasma->dmg_radius = damage_radius;
	plasma->s.sound = gi.soundindex("weapons/rockfly.wav");

	plasma->s.modelindex = gi.modelindex("sprites/s_photon.sp2");
	plasma->s.effects |= EF_PLASMA | EF_ANIM_ALLFAST;

	gi.linkentity(plasma);
}

THINK(Trap_Gib_Think) (edict_t *ent) -> void
{
	if (ent->owner->s.frame != 5)
	{
		G_FreeEdict(ent);
		return;
	}

	vec3_t forward, right, up;
	vec3_t vec;

	AngleVectors(ent->owner->s.angles, forward, right, up);

	// rotate us around the center
	float degrees = (150.f * gi.frame_time_s) + ent->owner->delay;
	vec3_t diff = ent->owner->s.origin - ent->s.origin;
	vec = RotatePointAroundVector(up, diff, degrees);
	ent->s.angles[1] += degrees;
	vec3_t new_origin = ent->owner->s.origin - vec;

	trace_t tr = gi.traceline(ent->s.origin, new_origin, ent, MASK_SOLID);
	ent->s.origin = tr.endpos;
	
	// pull us towards the trap's center
	diff.normalize();
	ent->s.origin += diff * (15.0f * gi.frame_time_s);

	ent->watertype = gi.pointcontents(ent->s.origin);
	if (ent->watertype & MASK_WATER)
		ent->waterlevel = WATER_FEET;

	ent->nextthink = level.time + FRAME_TIME_S;
	gi.linkentity(ent);
}

DIE(trap_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	BecomeExplosion1(self);
}

// RAFAEL
void SP_item_foodcube(edict_t *best);
void SpawnDamage(int type, const vec3_t &origin, const vec3_t &normal, int damage);
// RAFAEL
THINK(Trap_Think) (edict_t *ent) -> void
{
	edict_t *target = nullptr;
	edict_t *best = nullptr;
	vec3_t	 vec;
	float	 len;
	float	 oldlen = 8000;

	if (ent->timestamp < level.time)
	{
		BecomeExplosion1(ent);
		// note to self
		// cause explosion damage???
		return;
	}

	ent->nextthink = level.time + 10_hz;

	if (!ent->groundentity)
		return;

	// ok lets do the blood effect
	if (ent->s.frame > 4)
	{
		if (ent->s.frame == 5)
		{
			bool spawn = ent->wait == 64;

			ent->wait -= 2;

			if (spawn)
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/trapdown.wav"), 1, ATTN_IDLE, 0);

			ent->delay += 2.f;

			if (ent->wait < 19)
				ent->s.frame++;

			return;
		}
		ent->s.frame++;
		if (ent->s.frame == 8)
		{
			ent->nextthink = level.time + 1_sec;
			ent->think = G_FreeEdict;
			ent->s.effects &= ~EF_TRAP;

			best = G_Spawn();
			best->count = ent->mass;
			best->s.scale = 1.f + ((ent->accel - 100.f) / 300.f) * 1.0f;
			SP_item_foodcube(best);
			best->s.origin = ent->s.origin;
			best->s.origin[2] += 24 * best->s.scale;
			best->s.angles[YAW] = frandom() * 360;
			best->velocity[2] = 400;
			best->think(best);
			best->nextthink = 0_ms;
			best->s.old_origin = best->s.origin;
			gi.linkentity(best);

			gi.sound(best, CHAN_AUTO, gi.soundindex("misc/fhit3.wav"), 1.f, ATTN_NORM, 0.f);

			return;
		}
		return;
	}

	ent->s.effects &= ~EF_TRAP;
	if (ent->s.frame >= 4)
	{
		ent->s.effects |= EF_TRAP;
		// clear the owner if in deathmatch
		if (deathmatch->integer)
			ent->owner = nullptr;
	}

	if (ent->s.frame < 4)
	{
		ent->s.frame++;
		return;
	}

	while ((target = findradius(target, ent->s.origin, 256)) != nullptr)
	{
		if (target == ent)
			continue;
		
		// [Paril-KEX] don't allow traps to be placed near flags or teleporters
		// if it's a monster or player with health > 0
		// or it's a player start point
		// and we can see it
		// blow up
		if (target->classname && ((deathmatch->integer &&
				((!strncmp(target->classname, "info_player_", 12)) ||
				(!strcmp(target->classname, "misc_teleporter_dest")) ||
				(!strncmp(target->classname, "item_flag_", 10))))) &&
			(visible(target, ent)))
		{
			BecomeExplosion1(ent);
			return;
		}

		if (!(target->svflags & SVF_MONSTER) && !target->client)
			continue;
		if (target != ent->teammaster && CheckTeamDamage(target, ent->teammaster))
			continue;
		// [Paril-KEX]
		if (!deathmatch->integer && target->client)
			continue;
		if (target->health <= 0)
			continue;
		if (!visible(ent, target))
			continue;
		vec = ent->s.origin - target->s.origin;
		len = vec.length();
		if (!best)
		{
			best = target;
			oldlen = len;
			continue;
		}
		if (len < oldlen)
		{
			oldlen = len;
			best = target;
		}
	}

	// pull the enemy in
	if (best)
	{
		if (best->groundentity)
		{
			best->s.origin[2] += 1;
			best->groundentity = nullptr;
		}
		vec = ent->s.origin - best->s.origin;
		len = vec.normalize();

		float max_speed = best->client ? 290.f : 150.f;

		best->velocity += (vec * clamp(max_speed - len, 64.f, max_speed));

		ent->s.sound = gi.soundindex("weapons/trapsuck.wav");

		if (len < 48)
		{
			if (best->mass < 400)
			{
				ent->takedamage = false;
				ent->solid = SOLID_NOT;
				ent->die = nullptr;

				T_Damage(best, ent, ent->teammaster, vec3_origin, best->s.origin, vec3_origin, 100000, 1, DAMAGE_NONE, MOD_TRAP);

				if (best->svflags & SVF_MONSTER)
					M_ProcessPain(best);

				ent->enemy = best;
				ent->wait = 64;
				ent->s.old_origin = ent->s.origin;
				ent->timestamp = level.time + 30_sec;
				ent->accel = best->mass;
				if (deathmatch->integer)
					ent->mass = best->mass / 4;
				else
					ent->mass = best->mass / 10;
				// ok spawn the food cube
				ent->s.frame = 5;

				// link up any gibs that this monster may have spawned
				for (uint32_t i = 0; i < globals.num_edicts; i++)
				{
					edict_t *e = &g_edicts[i];

					if (!e->inuse)
						continue;
					else if (strcmp(e->classname, "gib"))
						continue;
					else if ((e->s.origin - ent->s.origin).length() > 128.f)
						continue;

					e->movetype = MOVETYPE_NONE;
					e->nextthink = level.time + FRAME_TIME_S;
					e->think = Trap_Gib_Think;
					e->owner = ent;
					Trap_Gib_Think(e);
				}
			}
			else
			{
				BecomeExplosion1(ent);
				// note to self
				// cause explosion damage???
				return;
			}
		}
	}
}

// RAFAEL
void fire_trap(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int speed)
{
	edict_t *trap;
	vec3_t	 dir;
	vec3_t	 forward, right, up;

	dir = vectoangles(aimdir);
	AngleVectors(dir, forward, right, up);

	trap = G_Spawn();
	trap->s.origin = start;
	trap->velocity = aimdir * speed;

	float gravityAdjustment = level.gravity / 800.f;

	trap->velocity += up * (200 + crandom() * 10.0f) * gravityAdjustment;
	trap->velocity += right * (crandom() * 10.0f);

	trap->avelocity = { 0, 300, 0 };
	trap->movetype = MOVETYPE_BOUNCE;

	trap->solid = SOLID_BBOX;
	trap->takedamage = true;
	trap->mins = { -4, -4, 0 };
	trap->maxs = { 4, 4, 8 };
	trap->die = trap_die;
	trap->health = 20;
	trap->s.modelindex = gi.modelindex("models/weapons/z_trap/tris.md2");
	trap->owner = trap->teammaster = self;
	trap->nextthink = level.time + 1_sec;
	trap->think = Trap_Think;
	trap->classname = "food_cube_trap";
	// RAFAEL 16-APR-98
	trap->s.sound = gi.soundindex("weapons/traploop.wav");
	// END 16-APR-98

	trap->flags |= ( FL_DAMAGEABLE | FL_MECHANICAL | FL_TRAP );
	trap->clipmask = MASK_PROJECTILE & ~CONTENTS_DEADMONSTER;

	// [Paril-KEX]
	if (self->client && !G_ShouldPlayersCollide(true))
		trap->clipmask &= ~CONTENTS_PLAYER;

	gi.linkentity(trap);

	trap->timestamp = level.time + 30_sec;
}
