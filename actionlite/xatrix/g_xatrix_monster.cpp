// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "../g_local.h"

// RAFAEL
void monster_fire_blueblaster(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, monster_muzzleflash_id_t flashtype, effects_t effect)
{
	fire_blueblaster(self, start, dir, damage, speed, effect);
	monster_muzzleflash(self, start, flashtype);
}

// RAFAEL
void monster_fire_ionripper(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, monster_muzzleflash_id_t flashtype, effects_t effect)
{
	fire_ionripper(self, start, dir, damage, speed, effect);
	monster_muzzleflash(self, start, flashtype);
}

// RAFAEL
void monster_fire_heat(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, monster_muzzleflash_id_t flashtype, float turn_fraction)
{
	fire_heat(self, start, dir, damage, speed, (float) damage, damage, turn_fraction);
	monster_muzzleflash(self, start, flashtype);
}

// RAFAEL
struct dabeam_pierce_t : pierce_args_t
{
	edict_t *self;
	bool damage;

	inline dabeam_pierce_t(edict_t *self, bool damage) :
		pierce_args_t(),
		self(self),
		damage(damage)
	{
	}

	// we hit an entity; return false to stop the piercing.
	// you can adjust the mask for the re-trace (for water, etc).
	virtual bool hit(contents_t &mask, vec3_t &end) override
	{
		if (damage)
		{
			// hurt it if we can
			if (self->dmg > 0 && (tr.ent->takedamage) && !(tr.ent->flags & FL_IMMUNE_LASER) && (tr.ent != self->owner))
				T_Damage(tr.ent, self, self->owner, self->movedir, tr.endpos, vec3_origin, self->dmg, skill->integer, DAMAGE_ENERGY, MOD_TARGET_LASER);

			if (self->dmg < 0) // healer ray
			{
				// when player is at 100 health
				// just undo health fix
				// keeping fx
				if (tr.ent->health < tr.ent->max_health)
					tr.ent->health = min(tr.ent->max_health, tr.ent->health - self->dmg);
			}
		}

		// if we hit something that's not a monster or player or is immune to lasers, we're done
		if (!(tr.ent->svflags & SVF_MONSTER) && (!tr.ent->client))
		{
			if (damage)
			{
				gi.WriteByte(svc_temp_entity);
				gi.WriteByte(TE_LASER_SPARKS);
				gi.WriteByte(10);
				gi.WritePosition(tr.endpos);
				gi.WriteDir(tr.plane.normal);
				gi.WriteByte(self->s.skinnum);
				gi.multicast(tr.endpos, MULTICAST_PVS, false);
			}

			return false;
		}

		if (!mark(tr.ent))
			return false;

		return true;
	}
};

void dabeam_update(edict_t *self, bool damage)
{
	vec3_t start = self->s.origin;
	vec3_t end = start + (self->movedir * 2048);

	dabeam_pierce_t args {
		self,
		damage
	};

	pierce_trace(start, end, self, args, CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_PLAYER | CONTENTS_DEADMONSTER);

	self->s.old_origin = args.tr.endpos + (args.tr.plane.normal * 1.f);
	gi.linkentity(self);
}

constexpr spawnflags_t SPAWNFLAG_DABEAM_SECONDARY = 1_spawnflag;

THINK(beam_think) (edict_t *self) -> void
{
	if (self->spawnflags.has(SPAWNFLAG_DABEAM_SECONDARY))
		self->owner->beam2 = nullptr;
	else
		self->owner->beam = nullptr;
	G_FreeEdict(self);
}

// RAFAEL
void monster_fire_dabeam(edict_t *self, int damage, bool secondary, void(*update_func)(edict_t *self))
{
	edict_t *&beam_ptr = secondary ? self->beam2 : self->beam;

	if (!beam_ptr)
	{
		beam_ptr = G_Spawn();

		beam_ptr->movetype = MOVETYPE_NONE;
		beam_ptr->solid = SOLID_NOT;
		beam_ptr->s.renderfx |= RF_BEAM;
		beam_ptr->s.modelindex = MODELINDEX_WORLD;
		beam_ptr->owner = self;
		beam_ptr->dmg = damage;
		beam_ptr->s.frame = 2;
		beam_ptr->spawnflags = secondary ? SPAWNFLAG_DABEAM_SECONDARY : SPAWNFLAG_NONE;

		if (self->monsterinfo.aiflags & AI_MEDIC)
			beam_ptr->s.skinnum = 0xf3f3f1f1;
		else
			beam_ptr->s.skinnum = 0xf2f2f0f0;

		beam_ptr->think = beam_think;
		beam_ptr->s.sound = gi.soundindex("misc/lasfly.wav");
		beam_ptr->postthink = update_func;
	}

	beam_ptr->nextthink = level.time + 200_ms;
	update_func(beam_ptr);
	dabeam_update(beam_ptr, true);
}