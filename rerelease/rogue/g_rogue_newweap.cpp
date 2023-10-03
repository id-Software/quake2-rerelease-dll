// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "../g_local.h"

/*
========================
fire_flechette
========================
*/
TOUCH(flechette_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	if (other == self->owner)
		return;

	if (tr.surface && (tr.surface->flags & SURF_SKY))
	{
		G_FreeEdict(self);
		return;
	}

	if (self->client)
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	if (other->takedamage)
	{
		T_Damage(other, self, self->owner, self->velocity, self->s.origin, tr.plane.normal,
				 self->dmg, (int) self->dmg_radius, DAMAGE_NO_REG_ARMOR, MOD_ETF_RIFLE);
	}
	else
	{
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_FLECHETTE);
		gi.WritePosition(self->s.origin);
		gi.WriteDir(tr.plane.normal);
		gi.multicast(self->s.origin, MULTICAST_PHS, false);
	}

	G_FreeEdict(self);
}

void fire_flechette(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, int kick)
{
	edict_t *flechette;

	flechette = G_Spawn();
	flechette->s.origin = start;
	flechette->s.old_origin = start;
	flechette->s.angles = vectoangles(dir);
	flechette->velocity = dir * speed;
	flechette->svflags |= SVF_PROJECTILE;
	flechette->movetype = MOVETYPE_FLYMISSILE;
	flechette->clipmask = MASK_PROJECTILE;
	flechette->flags |= FL_DODGE;

	// [Paril-KEX]
	if (self->client && !G_ShouldPlayersCollide(true))
		flechette->clipmask &= ~CONTENTS_PLAYER;

	flechette->solid = SOLID_BBOX;
	flechette->s.renderfx = RF_FULLBRIGHT;
	flechette->s.modelindex = gi.modelindex("models/proj/flechette/tris.md2");

	flechette->owner = self;
	flechette->touch = flechette_touch;
	flechette->nextthink = level.time + gtime_t::from_sec(8000.f / speed);
	flechette->think = G_FreeEdict;
	flechette->dmg = damage;
	flechette->dmg_radius = (float) kick;

	gi.linkentity(flechette);

	trace_t tr = gi.traceline(self->s.origin, flechette->s.origin, flechette, flechette->clipmask);
	if (tr.fraction < 1.0f)
	{
		flechette->s.origin = tr.endpos + (tr.plane.normal * 1.f);
		flechette->touch(flechette, tr.ent, tr, false);
	}
}

// **************************
// PROX
// **************************

constexpr gtime_t PROX_TIME_TO_LIVE = 45_sec; // 45, 30, 15, 10
constexpr gtime_t PROX_TIME_DELAY = 500_ms;
constexpr float	  PROX_BOUND_SIZE = 96;
constexpr float	  PROX_DAMAGE_RADIUS = 192;
constexpr int32_t PROX_HEALTH = 20;
constexpr int32_t PROX_DAMAGE = 90;

//===============
//===============
THINK(Prox_Explode) (edict_t *ent) -> void
{
	vec3_t	 origin;
	edict_t *owner;

	// free the trigger field

	// PMM - changed teammaster to "mover" .. owner of the field is the prox
	if (ent->teamchain && ent->teamchain->owner == ent)
		G_FreeEdict(ent->teamchain);

	owner = ent;
	if (ent->teammaster)
	{
		owner = ent->teammaster;
		PlayerNoise(owner, ent->s.origin, PNOISE_IMPACT);
	}

	// play quad sound if appopriate
	if (ent->dmg > PROX_DAMAGE)
		gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);

	ent->takedamage = false;
	T_RadiusDamage(ent, owner, (float) ent->dmg, ent, PROX_DAMAGE_RADIUS, DAMAGE_NONE, MOD_PROX);

	origin = ent->s.origin + (ent->velocity * -0.02f);
	gi.WriteByte(svc_temp_entity);
	if (ent->groundentity)
		gi.WriteByte(TE_GRENADE_EXPLOSION);
	else
		gi.WriteByte(TE_ROCKET_EXPLOSION);
	gi.WritePosition(origin);
	gi.multicast(ent->s.origin, MULTICAST_PHS, false);

	G_FreeEdict(ent);
}

//===============
//===============
DIE(prox_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	// if set off by another prox, delay a little (chained explosions)
	if (strcmp(inflictor->classname, "prox_mine"))
	{
		self->takedamage = false;
		Prox_Explode(self);
	}
	else
	{
		self->takedamage = false;
		self->think = Prox_Explode;
		self->nextthink = level.time + FRAME_TIME_S;
	}
}

//===============
//===============
TOUCH(Prox_Field_Touch) (edict_t *ent, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	edict_t *prox;

	if (!(other->svflags & SVF_MONSTER) && !other->client)
		return;

	// trigger the prox mine if it's still there, and still mine.
	prox = ent->owner;

	// teammate avoidance
	if (CheckTeamDamage(prox->teammaster, other))
		return;

	if (!deathmatch->integer && other->client)
		return;

	if (other == prox) // don't set self off
		return;

	if (prox->think == Prox_Explode) // we're set to blow!
		return;

	if (prox->teamchain == ent)
	{
		gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/proxwarn.wav"), 1, ATTN_NORM, 0);
		prox->think = Prox_Explode;
		prox->nextthink = level.time + PROX_TIME_DELAY;
		return;
	}

	ent->solid = SOLID_NOT;
	G_FreeEdict(ent);
}

//===============
//===============
THINK(prox_seek) (edict_t *ent) -> void
{
	if (level.time > gtime_t::from_sec(ent->wait))
	{
		Prox_Explode(ent);
	}
	else
	{
		ent->s.frame++;
		if (ent->s.frame > 13)
			ent->s.frame = 9;
		ent->think = prox_seek;
		ent->nextthink = level.time + 10_hz;
	}
}

//===============
//===============
THINK(prox_open) (edict_t *ent) -> void
{
	edict_t *search;

	search = nullptr;

	if (ent->s.frame == 9) // end of opening animation
	{
		// set the owner to nullptr so the owner can walk through it.  needs to be done here so the owner
		// doesn't get stuck on it while it's opening if fired at point blank wall
		ent->s.sound = 0;

		if (deathmatch->integer)
			ent->owner = nullptr;

		if (ent->teamchain)
			ent->teamchain->touch = Prox_Field_Touch;
		while ((search = findradius(search, ent->s.origin, PROX_DAMAGE_RADIUS + 10)) != nullptr)
		{
			if (!search->classname) // tag token and other weird shit
				continue;
			
			// teammate avoidance
			if (CheckTeamDamage(search, ent->teammaster))
				continue;

			// if it's a monster or player with health > 0
			// or it's a player start point
			// and we can see it
			// blow up
			if (
				search != ent &&
				(
					(((search->svflags & SVF_MONSTER) || (deathmatch->integer && (search->client || (search->classname && !strcmp(search->classname, "prox_mine"))))) && (search->health > 0)) ||
					(deathmatch->integer &&
					 ((!strncmp(search->classname, "info_player_", 12)) ||
					  (!strcmp(search->classname, "misc_teleporter_dest")) ||
					  (!strncmp(search->classname, "item_flag_", 10))))) &&
				(visible(search, ent)))
			{
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/proxwarn.wav"), 1, ATTN_NORM, 0);
				Prox_Explode(ent);
				return;
			}
		}

		if (g_dm_strong_mines->integer)
			ent->wait = (level.time + PROX_TIME_TO_LIVE).seconds();
		else
		{
			switch (ent->dmg / PROX_DAMAGE)
			{
			case 1:
				ent->wait = (level.time + PROX_TIME_TO_LIVE).seconds();
				break;
			case 2:
				ent->wait = (level.time + 30_sec).seconds();
				break;
			case 4:
				ent->wait = (level.time + 15_sec).seconds();
				break;
			case 8:
				ent->wait = (level.time + 10_sec).seconds();
				break;
			default:
				ent->wait = (level.time + PROX_TIME_TO_LIVE).seconds();
				break;
			}
		}

		ent->think = prox_seek;
		ent->nextthink = level.time + 200_ms;
	}
	else
	{
		if (ent->s.frame == 0)
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/proxopen.wav"), 1, ATTN_NORM, 0);
		ent->s.frame++;
		ent->think = prox_open;
		ent->nextthink = level.time + 10_hz;
	}
}

//===============
//===============
TOUCH(prox_land) (edict_t *ent, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	edict_t	*field;
	vec3_t	   dir;
	vec3_t	   forward, right, up;
	movetype_t movetype = MOVETYPE_NONE;
	int		   stick_ok = 0;
	vec3_t	   land_point;

	// must turn off owner so owner can shoot it and set it off
	// moved to prox_open so owner can get away from it if fired at pointblank range into
	// wall
	if (tr.surface && (tr.surface->flags & SURF_SKY))
	{
		G_FreeEdict(ent);
		return;
	}

	if (tr.plane.normal)
	{
		land_point = ent->s.origin + (tr.plane.normal * -10.0f);
		if (gi.pointcontents(land_point) & (CONTENTS_SLIME | CONTENTS_LAVA))
		{
			Prox_Explode(ent);
			return;
		}
	}

	constexpr float PROX_STOP_EPSILON = 0.1f;

	if (!tr.plane.normal || (other->svflags & SVF_MONSTER) || other->client || (other->flags & FL_DAMAGEABLE))
	{
		if (other != ent->teammaster)
			Prox_Explode(ent);

		return;
	}
	else if (other != world)
	{
		// Here we need to check to see if we can stop on this entity.
		// Note that plane can be nullptr

		// PMM - code stolen from g_phys (ClipVelocity)
		vec3_t out;
		float  backoff, change;
		int	   i;

		if ((other->movetype == MOVETYPE_PUSH) && (tr.plane.normal[2] > 0.7f))
			stick_ok = 1;
		else
			stick_ok = 0;

		backoff = ent->velocity.dot(tr.plane.normal) * 1.5f;
		for (i = 0; i < 3; i++)
		{
			change = tr.plane.normal[i] * backoff;
			out[i] = ent->velocity[i] - change;
			if (out[i] > -PROX_STOP_EPSILON && out[i] < PROX_STOP_EPSILON)
				out[i] = 0;
		}

		if (out[2] > 60)
			return;

		movetype = MOVETYPE_BOUNCE;

		// if we're here, we're going to stop on an entity
		if (stick_ok)
		{ // it's a happy entity
			ent->velocity = {};
			ent->avelocity = {};
		}
		else // no-stick.  teflon time
		{
			if (tr.plane.normal[2] > 0.7f)
			{
				Prox_Explode(ent);
				return;
			}
			return;
		}
	}
	else if (other->s.modelindex != MODELINDEX_WORLD)
		return;

	dir = vectoangles(tr.plane.normal);
	AngleVectors(dir, forward, right, up);

	if (gi.pointcontents(ent->s.origin) & (CONTENTS_LAVA | CONTENTS_SLIME))
	{
		Prox_Explode(ent);
		return;
	}

	ent->svflags &= ~SVF_PROJECTILE;

	field = G_Spawn();

	field->s.origin = ent->s.origin;
	field->mins = { -PROX_BOUND_SIZE, -PROX_BOUND_SIZE, -PROX_BOUND_SIZE };
	field->maxs = { PROX_BOUND_SIZE, PROX_BOUND_SIZE, PROX_BOUND_SIZE };
	field->movetype = MOVETYPE_NONE;
	field->solid = SOLID_TRIGGER;
	field->owner = ent;
	field->classname = "prox_field";
	field->teammaster = ent;
	gi.linkentity(field);

	ent->velocity = {};
	ent->avelocity = {};
	// rotate to vertical
	dir[PITCH] = dir[PITCH] + 90;
	ent->s.angles = dir;
	ent->takedamage = true;
	ent->movetype = movetype; // either bounce or none, depending on whether we stuck to something
	ent->die = prox_die;
	ent->teamchain = field;
	ent->health = PROX_HEALTH;
	ent->nextthink = level.time;
	ent->think = prox_open;
	ent->touch = nullptr;
	ent->solid = SOLID_BBOX;

	gi.linkentity(ent);
}

THINK(Prox_Think) (edict_t *self) -> void
{
	if (self->timestamp <= level.time)
	{
		Prox_Explode(self);
		return;
	}

	self->s.angles = vectoangles(self->velocity.normalized());
	self->s.angles[PITCH] -= 90;
	self->nextthink = level.time;
}

//===============
//===============
void fire_prox(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int prox_damage_multiplier, int speed)
{
	edict_t *prox;
	vec3_t	 dir;
	vec3_t	 forward, right, up;

	dir = vectoangles(aimdir);
	AngleVectors(dir, forward, right, up);

	prox = G_Spawn();
	prox->s.origin = start;
	prox->velocity = aimdir * speed;

	float gravityAdjustment = level.gravity / 800.f;

	prox->velocity += up * (200 + crandom() * 10.0f) * gravityAdjustment;
	prox->velocity += right * (crandom() * 10.0f);

	prox->s.angles = dir;
	prox->s.angles[PITCH] -= 90;
	prox->movetype = MOVETYPE_BOUNCE;
	prox->solid = SOLID_BBOX;
	prox->svflags |= SVF_PROJECTILE;
	prox->s.effects |= EF_GRENADE;
	prox->flags |= ( FL_DODGE | FL_TRAP );
	prox->clipmask = MASK_PROJECTILE | CONTENTS_LAVA | CONTENTS_SLIME;

	// [Paril-KEX]
	if (self->client && !G_ShouldPlayersCollide(true))
		prox->clipmask &= ~CONTENTS_PLAYER;

	prox->s.renderfx |= RF_IR_VISIBLE;
	// FIXME - this needs to be bigger.  Has other effects, though.  Maybe have to change origin to compensate
	//  so it sinks in correctly.  Also in lavacheck, might have to up the distance
	prox->mins = { -6, -6, -6 };
	prox->maxs = { 6, 6, 6 };
	prox->s.modelindex = gi.modelindex("models/weapons/g_prox/tris.md2");
	prox->owner = self;
	prox->teammaster = self;
	prox->touch = prox_land;
	prox->think = Prox_Think;
	prox->nextthink = level.time;
	prox->dmg = PROX_DAMAGE * prox_damage_multiplier;
	prox->classname = "prox_mine";
	prox->flags |= FL_DAMAGEABLE;
	prox->flags |= FL_MECHANICAL;

	switch (prox_damage_multiplier)
	{
	case 1:
		prox->timestamp = level.time + PROX_TIME_TO_LIVE;
		break;
	case 2:
		prox->timestamp = level.time + 30_sec;
		break;
	case 4:
		prox->timestamp = level.time + 15_sec;
		break;
	case 8:
		prox->timestamp = level.time + 10_sec;
		break;
	default:
		prox->timestamp = level.time + PROX_TIME_TO_LIVE;
		break;
	}

	gi.linkentity(prox);
}

// *************************
// MELEE WEAPONS
// *************************

struct player_melee_data_t
{
	edict_t *self;
	const vec3_t &start;
	const vec3_t &aim;
	int reach;
};

static BoxEdictsResult_t fire_player_melee_BoxFilter(edict_t *check, void *data_v)
{
	const player_melee_data_t *data = (const player_melee_data_t *) data_v;

	if (!check->inuse || !check->takedamage || check == data->self)
		return BoxEdictsResult_t::Skip;

	// check distance
	vec3_t closest_point_to_check = closest_point_to_box(data->start, check->s.origin + check->mins, check->s.origin + check->maxs);
	vec3_t closest_point_to_self = closest_point_to_box(closest_point_to_check, data->self->s.origin + data->self->mins, data->self->s.origin + data->self->maxs);

	vec3_t dir = (closest_point_to_check - closest_point_to_self);
	float len = dir.normalize();

	if (len > data->reach)
		return BoxEdictsResult_t::Skip;

	// check angle if we aren't intersecting
	vec3_t shrink { 2, 2, 2 };
	if (!boxes_intersect(check->absmin + shrink, check->absmax - shrink, data->self->absmin + shrink, data->self->absmax - shrink))
	{
		dir = (((check->absmin + check->absmax) / 2) - data->start).normalized();

		if (dir.dot(data->aim) < 0.70f)
			return BoxEdictsResult_t::Skip;
	}

	return BoxEdictsResult_t::Keep;
}

bool fire_player_melee(edict_t *self, const vec3_t &start, const vec3_t &aim, int reach, int damage, int kick, mod_t mod)
{
	constexpr size_t MAX_HIT = 4;

	vec3_t reach_vec{ float(reach - 1), float(reach - 1), float(reach - 1) };
	edict_t *targets[MAX_HIT];

	player_melee_data_t data {
		self,
		start,
		aim,
		reach
	};

	// find all the things we could maybe hit
	size_t num = gi.BoxEdicts(self->absmin - reach_vec, self->absmax + reach_vec, targets, q_countof(targets), AREA_SOLID, fire_player_melee_BoxFilter, &data);

	if (!num)
		return false;

	bool was_hit = false;

	for (size_t i = 0; i < num; i++)
	{
		edict_t *hit = targets[i];

		if (!hit->inuse || !hit->takedamage)
			continue;
		else if (!CanDamage(self, hit))
			continue;

		// do the damage
		vec3_t closest_point_to_check = closest_point_to_box(start, hit->s.origin + hit->mins, hit->s.origin + hit->maxs);

		if (hit->svflags & SVF_MONSTER)
			hit->pain_debounce_time -= random_time(5_ms, 75_ms);

		if (mod.id == MOD_CHAINFIST)
			T_Damage(hit, self, self, aim, closest_point_to_check, -aim, damage, kick / 2,
					 DAMAGE_DESTROY_ARMOR | DAMAGE_NO_KNOCKBACK, mod);
		else
			T_Damage(hit, self, self, aim, closest_point_to_check, -aim, damage, kick / 2, DAMAGE_NO_KNOCKBACK, mod);

		was_hit = true;
	}

	return was_hit;
}

// *************************
// NUKE
// *************************

constexpr gtime_t NUKE_DELAY = 4_sec;
constexpr gtime_t NUKE_TIME_TO_LIVE = 6_sec;
constexpr float	  NUKE_RADIUS = 512;
constexpr int32_t NUKE_DAMAGE = 400;
constexpr gtime_t NUKE_QUAKE_TIME = 3_sec;
constexpr float	  NUKE_QUAKE_STRENGTH = 100;

THINK(Nuke_Quake) (edict_t *self) -> void
{
	uint32_t i;
	edict_t *e;

	if (self->last_move_time < level.time)
	{
		gi.positioned_sound(self->s.origin, self, CHAN_AUTO, self->noise_index, 0.75, ATTN_NONE, 0);
		self->last_move_time = level.time + 500_ms;
	}

	for (i = 1, e = g_edicts + i; i < globals.num_edicts; i++, e++)
	{
		if (!e->inuse)
			continue;
		if (!e->client)
			continue;
		if (!e->groundentity)
			continue;

		e->groundentity = nullptr;
		e->velocity[0] += crandom() * 150;
		e->velocity[1] += crandom() * 150;
		e->velocity[2] = self->speed * (100.0f / e->mass);
	}

	if (level.time < self->timestamp)
		self->nextthink = level.time + FRAME_TIME_S;
	else
		G_FreeEdict(self);
}

static void Nuke_Explode(edict_t *ent)
{

	if (ent->teammaster->client)
		PlayerNoise(ent->teammaster, ent->s.origin, PNOISE_IMPACT);

	T_RadiusNukeDamage(ent, ent->teammaster, (float) ent->dmg, ent, ent->dmg_radius, MOD_NUKE);

	if (ent->dmg > NUKE_DAMAGE)
		gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);

	gi.sound(ent, CHAN_NO_PHS_ADD | CHAN_VOICE, gi.soundindex("weapons/grenlx1a.wav"), 1, ATTN_NONE, 0);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1_BIG);
	gi.WritePosition(ent->s.origin);
	gi.multicast(ent->s.origin, MULTICAST_PHS, false);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_NUKEBLAST);
	gi.WritePosition(ent->s.origin);
	gi.multicast(ent->s.origin, MULTICAST_ALL, false);

	// become a quake
	ent->svflags |= SVF_NOCLIENT;
	ent->noise_index = gi.soundindex("world/rumble.wav");
	ent->think = Nuke_Quake;
	ent->speed = NUKE_QUAKE_STRENGTH;
	ent->timestamp = level.time + NUKE_QUAKE_TIME;
	ent->nextthink = level.time + FRAME_TIME_S;
	ent->last_move_time = 0_ms;
}

DIE(nuke_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	self->takedamage = false;
	if ((attacker) && !(strcmp(attacker->classname, "nuke")))
	{
		G_FreeEdict(self);
		return;
	}
	Nuke_Explode(self);
}

THINK(Nuke_Think) (edict_t *ent) -> void
{
	float			attenuation, default_atten = 1.8f;
	int				nuke_damage_multiplier;
	player_muzzle_t muzzleflash;

	nuke_damage_multiplier = ent->dmg / NUKE_DAMAGE;
	switch (nuke_damage_multiplier)
	{
	case 1:
		attenuation = default_atten / 1.4f;
		muzzleflash = MZ_NUKE1;
		break;
	case 2:
		attenuation = default_atten / 2.0f;
		muzzleflash = MZ_NUKE2;
		break;
	case 4:
		attenuation = default_atten / 3.0f;
		muzzleflash = MZ_NUKE4;
		break;
	case 8:
		attenuation = default_atten / 5.0f;
		muzzleflash = MZ_NUKE8;
		break;
	default:
		attenuation = default_atten;
		muzzleflash = MZ_NUKE1;
		break;
	}

	if (ent->wait < level.time.seconds())
		Nuke_Explode(ent);
	else if (level.time >= (gtime_t::from_sec(ent->wait) - NUKE_TIME_TO_LIVE))
	{
		ent->s.frame++;

		if (ent->s.frame > 11)
			ent->s.frame = 6;

		if (gi.pointcontents(ent->s.origin) & (CONTENTS_SLIME | CONTENTS_LAVA))
		{
			Nuke_Explode(ent);
			return;
		}

		ent->think = Nuke_Think;
		ent->nextthink = level.time + 10_hz;
		ent->health = 1;
		ent->owner = nullptr;

		gi.WriteByte(svc_muzzleflash);
		gi.WriteEntity(ent);
		gi.WriteByte(muzzleflash);
		gi.multicast(ent->s.origin, MULTICAST_PHS, false);

		if (ent->timestamp <= level.time)
		{
			if ((gtime_t::from_sec(ent->wait) - level.time) <= (NUKE_TIME_TO_LIVE / 2.0f))
			{
				gi.sound(ent, CHAN_NO_PHS_ADD | CHAN_VOICE, gi.soundindex("weapons/nukewarn2.wav"), 1, attenuation, 0);
				ent->timestamp = level.time + 300_ms;
			}
			else
			{
				gi.sound(ent, CHAN_NO_PHS_ADD | CHAN_VOICE, gi.soundindex("weapons/nukewarn2.wav"), 1, attenuation, 0);
				ent->timestamp = level.time + 500_ms;
			}
		}
	}
	else
	{
		if (ent->timestamp <= level.time)
		{
			gi.sound(ent, CHAN_NO_PHS_ADD | CHAN_VOICE, gi.soundindex("weapons/nukewarn2.wav"), 1, attenuation, 0);
			ent->timestamp = level.time + 1_sec;
		}
		ent->nextthink = level.time + FRAME_TIME_S;
	}
}

TOUCH(nuke_bounce) (edict_t *ent, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	if (tr.surface && tr.surface->id)
	{
		if (frandom() > 0.5f)
			gi.sound(ent, CHAN_BODY, gi.soundindex("weapons/hgrenb1a.wav"), 1, ATTN_NORM, 0);
		else
			gi.sound(ent, CHAN_BODY, gi.soundindex("weapons/hgrenb2a.wav"), 1, ATTN_NORM, 0);
	}
}

void fire_nuke(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int speed)
{
	edict_t *nuke;
	vec3_t	 dir;
	vec3_t	 forward, right, up;
	int		 damage_modifier = P_DamageModifier(self);

	dir = vectoangles(aimdir);
	AngleVectors(dir, forward, right, up);

	nuke = G_Spawn();
	nuke->s.origin = start;
	nuke->velocity = aimdir * speed;
	nuke->velocity += up * (200 + crandom() * 10.0f);
	nuke->velocity += right * (crandom() * 10.0f);
	nuke->movetype = MOVETYPE_BOUNCE;
	nuke->clipmask = MASK_PROJECTILE;
	nuke->solid = SOLID_BBOX;
	nuke->s.effects |= EF_GRENADE;
	nuke->s.renderfx |= RF_IR_VISIBLE;
	nuke->mins = { -8, -8, 0 };
	nuke->maxs = { 8, 8, 16 };
	nuke->s.modelindex = gi.modelindex("models/weapons/g_nuke/tris.md2");
	nuke->owner = self;
	nuke->teammaster = self;
	nuke->nextthink = level.time + FRAME_TIME_S;
	nuke->wait = (level.time + NUKE_DELAY + NUKE_TIME_TO_LIVE).seconds();
	nuke->think = Nuke_Think;
	nuke->touch = nuke_bounce;

	nuke->health = 10000;
	nuke->takedamage = true;
	nuke->flags |= FL_DAMAGEABLE;
	nuke->dmg = NUKE_DAMAGE * damage_modifier;
	if (damage_modifier == 1)
		nuke->dmg_radius = NUKE_RADIUS;
	else
		nuke->dmg_radius = NUKE_RADIUS + NUKE_RADIUS * (0.25f * (float) damage_modifier);
	// this yields 1.0, 1.5, 2.0, 3.0 times radius

	nuke->classname = "nuke";
	nuke->die = nuke_die;

	gi.linkentity(nuke);
}

// *************************
// TESLA
// *************************

constexpr gtime_t TESLA_TIME_TO_LIVE = 30_sec;
constexpr float	  TESLA_DAMAGE_RADIUS = 128;
constexpr int32_t TESLA_DAMAGE = 3;
constexpr int32_t TESLA_KNOCKBACK = 8;

constexpr gtime_t TESLA_ACTIVATE_TIME = 3_sec;

constexpr int32_t TESLA_EXPLOSION_DAMAGE_MULT = 50; // this is the amount the damage is multiplied by for underwater explosions
constexpr float	  TESLA_EXPLOSION_RADIUS = 200;

void tesla_remove(edict_t *self)
{
	edict_t *cur, *next;

	self->takedamage = false;
	if (self->teamchain)
	{
		cur = self->teamchain;
		while (cur)
		{
			next = cur->teamchain;
			G_FreeEdict(cur);
			cur = next;
		}
	}
	else if (self->air_finished)
		gi.Com_Print("tesla_mine without a field!\n");

	self->owner = self->teammaster; // Going away, set the owner correctly.
	// PGM - grenade explode does damage to self->enemy
	self->enemy = nullptr;

	// play quad sound if quadded and an underwater explosion
	if ((self->dmg_radius) && (self->dmg > (TESLA_DAMAGE * TESLA_EXPLOSION_DAMAGE_MULT)))
		gi.sound(self, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);

	Grenade_Explode(self);
}

DIE(tesla_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	tesla_remove(self);
}

void tesla_blow(edict_t *self)
{
	self->dmg *= TESLA_EXPLOSION_DAMAGE_MULT;
	self->dmg_radius = TESLA_EXPLOSION_RADIUS;
	tesla_remove(self);
}

TOUCH(tesla_zap) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
}

static BoxEdictsResult_t tesla_think_active_BoxFilter(edict_t *check, void *data)
{
	edict_t *self = (edict_t *) data;

	if (!check->inuse)
		return BoxEdictsResult_t::Skip;
	if (check == self)
		return BoxEdictsResult_t::Skip;
	if (check->health < 1)
		return BoxEdictsResult_t::Skip;
	// don't hit teammates
	if (check->client)
	{
		if (!deathmatch->integer)
			return BoxEdictsResult_t::Skip;
		else if (CheckTeamDamage(check, self->teammaster))
			return BoxEdictsResult_t::Skip;
	}
	if (!(check->svflags & SVF_MONSTER) && !(check->flags & FL_DAMAGEABLE) && !check->client)
		return BoxEdictsResult_t::Skip;

	// don't hit other teslas in SP/coop
	if (!deathmatch->integer && check->classname && (check->flags & FL_TRAP))
		return BoxEdictsResult_t::Skip;

	return BoxEdictsResult_t::Keep;
}

THINK(tesla_think_active) (edict_t *self) -> void
{
	int		 i, num;
	static edict_t *touch[MAX_EDICTS];
	edict_t *hit;
	vec3_t	 dir, start;
	trace_t	 tr;

	if (level.time > self->air_finished)
	{
		tesla_remove(self);
		return;
	}

	start = self->s.origin;
	start[2] += 16;

	num = gi.BoxEdicts(self->teamchain->absmin, self->teamchain->absmax, touch, MAX_EDICTS, AREA_SOLID, tesla_think_active_BoxFilter, self);
	for (i = 0; i < num; i++)
	{
		// if the tesla died while zapping things, stop zapping.
		if (!(self->inuse))
			break;

		hit = touch[i];
		if (!hit->inuse)
			continue;
		if (hit == self)
			continue;
		if (hit->health < 1)
			continue;
		// don't hit teammates
		if (hit->client)
		{
			if (!deathmatch->integer)
				continue;
			else if (CheckTeamDamage(hit, self->teamchain->owner))
				continue;
		}
		if (!(hit->svflags & SVF_MONSTER) && !(hit->flags & FL_DAMAGEABLE) && !hit->client)
			continue;

		tr = gi.traceline(start, hit->s.origin, self, MASK_PROJECTILE);
		if (tr.fraction == 1 || tr.ent == hit)
		{
			dir = hit->s.origin - start;

			// PMM - play quad sound if it's above the "normal" damage
			if (self->dmg > TESLA_DAMAGE)
				gi.sound(self, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);

			// PGM - don't do knockback to walking monsters
			if ((hit->svflags & SVF_MONSTER) && !(hit->flags & (FL_FLY | FL_SWIM)))
				T_Damage(hit, self, self->teammaster, dir, tr.endpos, tr.plane.normal,
						 self->dmg, 0, DAMAGE_NONE, MOD_TESLA);
			else
				T_Damage(hit, self, self->teammaster, dir, tr.endpos, tr.plane.normal,
						 self->dmg, TESLA_KNOCKBACK, DAMAGE_NONE, MOD_TESLA);

			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_LIGHTNING);
			gi.WriteEntity(self);	// source entity
			gi.WriteEntity(hit); // destination entity
			gi.WritePosition(start);
			gi.WritePosition(tr.endpos);
			gi.multicast(start, MULTICAST_PVS, false);
		}
	}

	if (self->inuse)
	{
		self->think = tesla_think_active;
		self->nextthink = level.time + 10_hz;
	}
}

THINK(tesla_activate) (edict_t *self) -> void
{
	edict_t *trigger;
	edict_t *search;

	if (gi.pointcontents(self->s.origin) & (CONTENTS_SLIME | CONTENTS_LAVA | CONTENTS_WATER))
	{
		tesla_blow(self);
		return;
	}

	// only check for spawn points in deathmatch
	if (deathmatch->integer)
	{
		search = nullptr;
		while ((search = findradius(search, self->s.origin, 1.5f * TESLA_DAMAGE_RADIUS)) != nullptr)
		{
			// [Paril-KEX] don't allow traps to be placed near flags or teleporters
			// if it's a monster or player with health > 0
			// or it's a player start point
			// and we can see it
			// blow up
			if (search->classname && ((deathmatch->integer &&
					((!strncmp(search->classname, "info_player_", 12)) ||
					(!strcmp(search->classname, "misc_teleporter_dest")) ||
					(!strncmp(search->classname, "item_flag_", 10))))) &&
				(visible(search, self)))
			{
				BecomeExplosion1(self);
				return;
			}
		}
	}

	trigger = G_Spawn();
	trigger->s.origin = self->s.origin;
	trigger->mins = { -TESLA_DAMAGE_RADIUS, -TESLA_DAMAGE_RADIUS, self->mins[2] };
	trigger->maxs = { TESLA_DAMAGE_RADIUS, TESLA_DAMAGE_RADIUS, TESLA_DAMAGE_RADIUS };
	trigger->movetype = MOVETYPE_NONE;
	trigger->solid = SOLID_TRIGGER;
	trigger->owner = self;
	trigger->touch = tesla_zap;
	trigger->classname = "tesla trigger";
	// doesn't need to be marked as a teamslave since the move code for bounce looks for teamchains
	gi.linkentity(trigger);

	self->s.angles = {};
	// clear the owner if in deathmatch
	if (deathmatch->integer)
		self->owner = nullptr;
	self->teamchain = trigger;
	self->think = tesla_think_active;
	self->nextthink = level.time + FRAME_TIME_S;
	self->air_finished = level.time + TESLA_TIME_TO_LIVE;
}

THINK(tesla_think) (edict_t *ent) -> void
{
	if (gi.pointcontents(ent->s.origin) & (CONTENTS_SLIME | CONTENTS_LAVA))
	{
		tesla_remove(ent);
		return;
	}

	ent->s.angles = {};

	if (!(ent->s.frame))
		gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/teslaopen.wav"), 1, ATTN_NORM, 0);

	ent->s.frame++;
	if (ent->s.frame > 14)
	{
		ent->s.frame = 14;
		ent->think = tesla_activate;
		ent->nextthink = level.time + 10_hz;
	}
	else
	{
		if (ent->s.frame > 9)
		{
			if (ent->s.frame == 10)
			{
				if (ent->owner && ent->owner->client)
				{
					PlayerNoise(ent->owner, ent->s.origin, PNOISE_WEAPON); // PGM
				}
				ent->s.skinnum = 1;
			}
			else if (ent->s.frame == 12)
				ent->s.skinnum = 2;
			else if (ent->s.frame == 14)
				ent->s.skinnum = 3;
		}
		ent->think = tesla_think;
		ent->nextthink = level.time + 10_hz;
	}
}

TOUCH(tesla_lava) (edict_t *ent, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	if (tr.contents & (CONTENTS_SLIME | CONTENTS_LAVA))
	{
		tesla_blow(ent);
		return;
	}

	if (ent->velocity)
	{
		if (frandom() > 0.5f)
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/hgrenb1a.wav"), 1, ATTN_NORM, 0);
		else
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/hgrenb2a.wav"), 1, ATTN_NORM, 0);
	}
}

void fire_tesla(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int tesla_damage_multiplier, int speed)
{
	edict_t *tesla;
	vec3_t	 dir;
	vec3_t	 forward, right, up;

	dir = vectoangles(aimdir);
	AngleVectors(dir, forward, right, up);

	tesla = G_Spawn();
	tesla->s.origin = start;
	tesla->velocity = aimdir * speed;

	float gravityAdjustment = level.gravity / 800.f;

	tesla->velocity += up * (200 + crandom() * 10.0f) * gravityAdjustment;
	tesla->velocity += right * (crandom() * 10.0f);

	tesla->s.angles = {};
	tesla->movetype = MOVETYPE_BOUNCE;
	tesla->solid = SOLID_BBOX;
	tesla->s.effects |= EF_GRENADE;
	tesla->s.renderfx |= RF_IR_VISIBLE;
	tesla->mins = { -12, -12, 0 };
	tesla->maxs = { 12, 12, 20 };
	tesla->s.modelindex = gi.modelindex("models/weapons/g_tesla/tris.md2");

	tesla->owner = self; // PGM - we don't want it owned by self YET.
	tesla->teammaster = self;

	tesla->wait = (level.time + TESLA_TIME_TO_LIVE).seconds();
	tesla->think = tesla_think;
	tesla->nextthink = level.time + TESLA_ACTIVATE_TIME;

	// blow up on contact with lava & slime code
	tesla->touch = tesla_lava;

	if (deathmatch->integer)
		// PMM - lowered from 50 - 7/29/1998
		tesla->health = 20;
	else
		tesla->health = 50; // FIXME - change depending on skill?

	tesla->takedamage = true;
	tesla->die = tesla_die;
	tesla->dmg = TESLA_DAMAGE * tesla_damage_multiplier;
	tesla->classname = "tesla_mine";
	tesla->flags |= ( FL_DAMAGEABLE | FL_TRAP );
	tesla->clipmask = (MASK_PROJECTILE | CONTENTS_SLIME | CONTENTS_LAVA) & ~CONTENTS_DEADMONSTER;

	// [Paril-KEX]
	if (self->client && !G_ShouldPlayersCollide(true))
		tesla->clipmask &= ~CONTENTS_PLAYER;

	tesla->flags |= FL_MECHANICAL;

	gi.linkentity(tesla);
}

// *************************
//  HEATBEAM
// *************************

static void fire_beams(edict_t *self, const vec3_t &start, const vec3_t &aimdir, const vec3_t &offset, int damage, int kick, int te_beam, int te_impact, mod_t mod)
{
	trace_t	   tr;
	vec3_t	   dir;
	vec3_t	   forward, right, up;
	vec3_t	   end;
	vec3_t	   water_start, endpoint;
	bool	   water = false, underwater = false;
	contents_t content_mask = MASK_PROJECTILE | MASK_WATER;

	// [Paril-KEX]
	if (self->client && !G_ShouldPlayersCollide(true))
		content_mask &= ~CONTENTS_PLAYER;

	vec3_t	   beam_endpt;

	//	tr = gi.traceline (self->s.origin, start, self, MASK_PROJECTILE);
	//	if (!(tr.fraction < 1.0))
	//	{
	dir = vectoangles(aimdir);
	AngleVectors(dir, forward, right, up);

	end = start + (forward * 8192);

	if (gi.pointcontents(start) & MASK_WATER)
	{
		underwater = true;
		water_start = start;
		content_mask &= ~MASK_WATER;
	}

	tr = gi.traceline(start, end, self, content_mask);

	// see if we hit water
	if (tr.contents & MASK_WATER)
	{
		water = true;
		water_start = tr.endpos;

		if (start != tr.endpos)
		{
			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_HEATBEAM_SPARKS);
			//			gi.WriteByte (50);
			gi.WritePosition(water_start);
			gi.WriteDir(tr.plane.normal);
			//			gi.WriteByte (8);
			//			gi.WriteShort (60);
			gi.multicast(tr.endpos, MULTICAST_PVS, false);
		}
		// re-trace ignoring water this time
		tr = gi.traceline(water_start, end, self, content_mask & ~MASK_WATER);
	}
	endpoint = tr.endpos;
	//	}

	// halve the damage if target underwater
	if (water)
	{
		damage = damage / 2;
	}

	// send gun puff / flash
	if (!((tr.surface) && (tr.surface->flags & SURF_SKY)))
	{
		if (tr.fraction < 1.0f)
		{
			if (tr.ent->takedamage)
			{
				T_Damage(tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, kick, DAMAGE_ENERGY, mod);
			}
			else
			{
				if ((!water) && !(tr.surface && (tr.surface->flags & SURF_SKY)))
				{
					// This is the truncated steam entry - uses 1+1+2 extra bytes of data
					gi.WriteByte(svc_temp_entity);
					gi.WriteByte(TE_HEATBEAM_STEAM);
					//					gi.WriteByte (20);
					gi.WritePosition(tr.endpos);
					gi.WriteDir(tr.plane.normal);
					//					gi.WriteByte (0xe0);
					//					gi.WriteShort (60);
					gi.multicast(tr.endpos, MULTICAST_PVS, false);

					if (self->client)
						PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
				}
			}
		}
	}

	// if went through water, determine where the end and make a bubble trail
	if ((water) || (underwater))
	{
		vec3_t pos;

		dir = tr.endpos - water_start;
		dir.normalize();
		pos = tr.endpos + (dir * -2);
		if (gi.pointcontents(pos) & MASK_WATER)
			tr.endpos = pos;
		else
			tr = gi.traceline(pos, water_start, tr.ent, MASK_WATER);

		pos = water_start + tr.endpos;
		pos *= 0.5f;

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_BUBBLETRAIL2);
		//		gi.WriteByte (8);
		gi.WritePosition(water_start);
		gi.WritePosition(tr.endpos);
		gi.multicast(pos, MULTICAST_PVS, false);
	}

	if ((!underwater) && (!water))
	{
		beam_endpt = tr.endpos;
	}
	else
	{
		beam_endpt = endpoint;
	}

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(te_beam);
	gi.WriteEntity(self);
	gi.WritePosition(start);
	gi.WritePosition(beam_endpt);
	gi.multicast(self->s.origin, MULTICAST_ALL, false);
}

/*
=================
fire_heat

Fires a single heat beam.  Zap.
=================
*/
void fire_heatbeam(edict_t *self, const vec3_t &start, const vec3_t &aimdir, const vec3_t &offset, int damage, int kick, bool monster)
{
	if (monster)
		fire_beams(self, start, aimdir, offset, damage, kick, TE_MONSTER_HEATBEAM, TE_HEATBEAM_SPARKS, MOD_HEATBEAM);
	else
		fire_beams(self, start, aimdir, offset, damage, kick, TE_HEATBEAM, TE_HEATBEAM_SPARKS, MOD_HEATBEAM);
}

// *************************
//	BLASTER 2
// *************************

/*
=================
fire_blaster2

Fires a single green blaster bolt.  Used by monsters, generally.
=================
*/
TOUCH(blaster2_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	mod_t mod;
	int	  damagestat;

	if (other == self->owner)
		return;

	if (tr.surface && (tr.surface->flags & SURF_SKY))
	{
		G_FreeEdict(self);
		return;
	}

	if (self->owner && self->owner->client)
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	if (other->takedamage)
	{
		// the only time players will be firing blaster2 bolts will be from the
		// defender sphere.
		if (self->owner && self->owner->client)
			mod = MOD_DEFENDER_SPHERE;
		else
			mod = MOD_BLASTER2;

		if (self->owner)
		{
			damagestat = self->owner->takedamage;
			self->owner->takedamage = false;
			if (self->dmg >= 5)
				T_RadiusDamage(self, self->owner, (float) (self->dmg * 2), other, self->dmg_radius, DAMAGE_ENERGY, MOD_UNKNOWN);
			T_Damage(other, self, self->owner, self->velocity, self->s.origin, tr.plane.normal, self->dmg, 1, DAMAGE_ENERGY, mod);
			self->owner->takedamage = damagestat;
		}
		else
		{
			if (self->dmg >= 5)
				T_RadiusDamage(self, self->owner, (float) (self->dmg * 2), other, self->dmg_radius, DAMAGE_ENERGY, MOD_UNKNOWN);
			T_Damage(other, self, self->owner, self->velocity, self->s.origin, tr.plane.normal, self->dmg, 1, DAMAGE_ENERGY, mod);
		}
	}
	else
	{
		// PMM - yeowch this will get expensive
		if (self->dmg >= 5)
			T_RadiusDamage(self, self->owner, (float) (self->dmg * 2), self->owner, self->dmg_radius, DAMAGE_ENERGY, MOD_UNKNOWN);

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_BLASTER2);
		gi.WritePosition(self->s.origin);
		gi.WriteDir(tr.plane.normal);
		gi.multicast(self->s.origin, MULTICAST_PHS, false);
	}

	G_FreeEdict(self);
}

void fire_blaster2(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, effects_t effect, bool hyper)
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
	bolt->clipmask = MASK_PROJECTILE;
	bolt->flags |= FL_DODGE;

	// [Paril-KEX]
	if (self->client && !G_ShouldPlayersCollide(true))
		bolt->clipmask &= ~CONTENTS_PLAYER;

	bolt->solid = SOLID_BBOX;
	bolt->s.effects |= effect;
	if (effect)
		bolt->s.effects |= EF_TRACKER;
	bolt->dmg_radius = 128;
	bolt->s.modelindex = gi.modelindex("models/objects/laser/tris.md2");
	bolt->s.skinnum = 2;
	bolt->s.scale = 2.5f;
	bolt->touch = blaster2_touch;

	bolt->owner = self;
	bolt->nextthink = level.time + 2_sec;
	bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	bolt->classname = "bolt";
	gi.linkentity(bolt);

	tr = gi.traceline(self->s.origin, bolt->s.origin, bolt, bolt->clipmask);
	if (tr.fraction < 1.0f)
	{
		bolt->s.origin = tr.endpos + (tr.plane.normal * 1.f);
		bolt->touch(bolt, tr.ent, tr, false);
	}
}

// *************************
// tracker
// *************************

constexpr damageflags_t TRACKER_DAMAGE_FLAGS = (DAMAGE_NO_POWER_ARMOR | DAMAGE_ENERGY | DAMAGE_NO_KNOCKBACK);
constexpr damageflags_t TRACKER_IMPACT_FLAGS = (DAMAGE_NO_POWER_ARMOR | DAMAGE_ENERGY);

constexpr gtime_t TRACKER_DAMAGE_TIME = 500_ms;

THINK(tracker_pain_daemon_think) (edict_t *self) -> void
{
	constexpr vec3_t pain_normal = { 0, 0, 1 };
	int				 hurt;

	if (!self->inuse)
		return;

	if ((level.time - self->timestamp) > TRACKER_DAMAGE_TIME)
	{
		if (!self->enemy->client)
			self->enemy->s.effects &= ~EF_TRACKERTRAIL;
		G_FreeEdict(self);
	}
	else
	{
		if (self->enemy->health > 0)
		{
			vec3_t center = (self->enemy->absmax + self->enemy->absmin) * 0.5f;

			T_Damage(self->enemy, self, self->owner, vec3_origin, center, pain_normal,
					 self->dmg, 0, TRACKER_DAMAGE_FLAGS, MOD_TRACKER);

			// if we kill the player, we'll be removed.
			if (self->inuse)
			{
				// if we killed a monster, gib them.
				if (self->enemy->health < 1)
				{
					if (self->enemy->gib_health)
						hurt = -self->enemy->gib_health;
					else
						hurt = 500;

					T_Damage(self->enemy, self, self->owner, vec3_origin, center,
							 pain_normal, hurt, 0, TRACKER_DAMAGE_FLAGS, MOD_TRACKER);
				}

				self->nextthink = level.time + 10_hz;

				if (self->enemy->client)
					self->enemy->client->tracker_pain_time = self->nextthink;
				else
					self->enemy->s.effects |= EF_TRACKERTRAIL;
			}
		}
		else
		{
			if (!self->enemy->client)
				self->enemy->s.effects &= ~EF_TRACKERTRAIL;
			G_FreeEdict(self);
		}
	}
}

void tracker_pain_daemon_spawn(edict_t *owner, edict_t *enemy, int damage)
{
	edict_t *daemon;

	if (enemy == nullptr)
		return;

	daemon = G_Spawn();
	daemon->classname = "pain daemon";
	daemon->think = tracker_pain_daemon_think;
	daemon->nextthink = level.time;
	daemon->timestamp = level.time;
	daemon->owner = owner;
	daemon->enemy = enemy;
	daemon->dmg = damage;
}

void tracker_explode(edict_t *self)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_TRACKER_EXPLOSION);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PHS, false);

	G_FreeEdict(self);
}

TOUCH(tracker_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	float damagetime;

	if (other == self->owner)
		return;

	if (tr.surface && (tr.surface->flags & SURF_SKY))
	{
		G_FreeEdict(self);
		return;
	}

	if (self->client)
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	if (other->takedamage)
	{
		if ((other->svflags & SVF_MONSTER) || other->client)
		{
			if (other->health > 0) // knockback only for living creatures
			{
				// PMM - kickback was times 4 .. reduced to 3
				// now this does no damage, just knockback
				T_Damage(other, self, self->owner, self->velocity, self->s.origin, tr.plane.normal,
						 /* self->dmg */ 0, (self->dmg * 3), TRACKER_IMPACT_FLAGS, MOD_TRACKER);

				if (!(other->flags & (FL_FLY | FL_SWIM)))
					other->velocity[2] += 140;

				damagetime = ((float) self->dmg) * 0.1f;
				damagetime = damagetime / TRACKER_DAMAGE_TIME.seconds();

				tracker_pain_daemon_spawn(self->owner, other, (int) damagetime);
			}
			else // lots of damage (almost autogib) for dead bodies
			{
				T_Damage(other, self, self->owner, self->velocity, self->s.origin, tr.plane.normal,
						 self->dmg * 4, (self->dmg * 3), TRACKER_IMPACT_FLAGS, MOD_TRACKER);
			}
		}
		else // full damage in one shot for inanimate objects
		{
			T_Damage(other, self, self->owner, self->velocity, self->s.origin, tr.plane.normal,
					 self->dmg, (self->dmg * 3), TRACKER_IMPACT_FLAGS, MOD_TRACKER);
		}
	}

	tracker_explode(self);
	return;
}

THINK(tracker_fly) (edict_t *self) -> void
{
	vec3_t dest;
	vec3_t dir;
	vec3_t center;

	if ((!self->enemy) || (!self->enemy->inuse) || (self->enemy->health < 1))
	{
		tracker_explode(self);
		return;
	}

	// PMM - try to hunt for center of enemy, if possible and not client
	if (self->enemy->client)
	{
		dest = self->enemy->s.origin;
		dest[2] += self->enemy->viewheight;
	}
	// paranoia
	else if (!self->enemy->absmin || !self->enemy->absmax)
	{
		dest = self->enemy->s.origin;
	}
	else
	{
		center = (self->enemy->absmin + self->enemy->absmax) * 0.5f;
		dest = center;
	}

	dir = dest - self->s.origin;
	dir.normalize();
	self->s.angles = vectoangles(dir);
	self->velocity = dir * self->speed;
	self->monsterinfo.saved_goal = dest;

	self->nextthink = level.time + 10_hz;
}

void fire_tracker(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, edict_t *enemy)
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
	bolt->clipmask = MASK_PROJECTILE;

	// [Paril-KEX]
	if (self->client && !G_ShouldPlayersCollide(true))
		bolt->clipmask &= ~CONTENTS_PLAYER;

	bolt->solid = SOLID_BBOX;
	bolt->speed = (float) speed;
	bolt->s.effects = EF_TRACKER;
	bolt->s.sound = gi.soundindex("weapons/disrupt.wav");
	bolt->s.modelindex = gi.modelindex("models/proj/disintegrator/tris.md2");
	bolt->touch = tracker_touch;
	bolt->enemy = enemy;
	bolt->owner = self;
	bolt->dmg = damage;
	bolt->classname = "tracker";
	gi.linkentity(bolt);

	if (enemy)
	{
		bolt->nextthink = level.time + 10_hz;
		bolt->think = tracker_fly;
	}
	else
	{
		bolt->nextthink = level.time + 10_sec;
		bolt->think = G_FreeEdict;
	}

	tr = gi.traceline(self->s.origin, bolt->s.origin, bolt, bolt->clipmask);
	if (tr.fraction < 1.0f)
	{
		bolt->s.origin = tr.endpos + (tr.plane.normal * 1.f);
		bolt->touch(bolt, tr.ent, tr, false);
	}
}
