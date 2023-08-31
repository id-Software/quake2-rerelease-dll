// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_sphere.c
// pmack
// april 1998

// defender - actively finds and shoots at enemies
// hunter - waits until < 25% health and vore ball tracks person who hurt you
// vengeance - kills person who killed you.

#include "../g_local.h"

constexpr gtime_t DEFENDER_LIFESPAN = 30_sec;
constexpr gtime_t HUNTER_LIFESPAN = 30_sec;
constexpr gtime_t VENGEANCE_LIFESPAN = 30_sec;
constexpr gtime_t MINIMUM_FLY_TIME = 15_sec;

void LookAtKiller(edict_t *self, edict_t *inflictor, edict_t *attacker);

void vengeance_touch(edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self);
void hunter_touch(edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self);

// *************************
// General Sphere Code
// *************************

// =================
// =================
THINK(sphere_think_explode) (edict_t *self) -> void
{
	if (self->owner && self->owner->client && !(self->spawnflags & SPHERE_DOPPLEGANGER))
	{
		self->owner->client->owned_sphere = nullptr;
	}
	BecomeExplosion1(self);
}

// =================
// sphere_explode
// =================
DIE(sphere_explode) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	sphere_think_explode(self);
}

// =================
// sphere_if_idle_die - if the sphere is not currently attacking, blow up.
// =================
DIE(sphere_if_idle_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	if (!self->enemy)
		sphere_think_explode(self);
}

// *************************
// Sphere Movement
// *************************

// =================
// =================
void sphere_fly(edict_t *self)
{
	vec3_t dest;
	vec3_t dir;

	if (level.time >= gtime_t::from_sec(self->wait))
	{
		sphere_think_explode(self);
		return;
	}

	dest = self->owner->s.origin;
	dest[2] = self->owner->absmax[2] + 4;

	if (level.time.seconds() == level.time.seconds<int>())
	{
		if (!visible(self, self->owner))
		{
			self->s.origin = dest;
			gi.linkentity(self);
			return;
		}
	}

	dir = dest - self->s.origin;
	self->velocity = dir * 5;
}

// =================
// =================
void sphere_chase(edict_t *self, int stupidChase)
{
	vec3_t dest;
	vec3_t dir;
	float  dist;

	if (level.time >= gtime_t::from_sec(self->wait) || (self->enemy && self->enemy->health < 1))
	{
		sphere_think_explode(self);
		return;
	}

	dest = self->enemy->s.origin;
	if (self->enemy->client)
		dest[2] += self->enemy->viewheight;

	if (visible(self, self->enemy) || stupidChase)
	{
		// if moving, hunter sphere uses active sound
		if (!stupidChase)
			self->s.sound = gi.soundindex("spheres/h_active.wav");

		dir = dest - self->s.origin;
		dir.normalize();
		self->s.angles = vectoangles(dir);
		self->velocity = dir * 500;
		self->monsterinfo.saved_goal = dest;
	}
	else if (!self->monsterinfo.saved_goal)
	{
		dir = self->enemy->s.origin - self->s.origin;
		dist = dir.normalize();
		self->s.angles = vectoangles(dir);

		// if lurking, hunter sphere uses lurking sound
		self->s.sound = gi.soundindex("spheres/h_lurk.wav");
		self->velocity = {};
	}
	else
	{
		dir = self->monsterinfo.saved_goal - self->s.origin;
		dist = dir.normalize();

		if (dist > 1)
		{
			self->s.angles = vectoangles(dir);

			if (dist > 500)
				self->velocity = dir * 500;
			else if (dist < 20)
				self->velocity = dir * (dist / gi.frame_time_s);
			else
				self->velocity = dir * dist;

			// if moving, hunter sphere uses active sound
			if (!stupidChase)
				self->s.sound = gi.soundindex("spheres/h_active.wav");
		}
		else
		{
			dir = self->enemy->s.origin - self->s.origin;
			dist = dir.normalize();
			self->s.angles = vectoangles(dir);

			// if not moving, hunter sphere uses lurk sound
			if (!stupidChase)
				self->s.sound = gi.soundindex("spheres/h_lurk.wav");

			self->velocity = {};
		}
	}
}

// *************************
// Attack related stuff
// *************************

// =================
// =================
void sphere_fire(edict_t *self, edict_t *enemy)
{
	vec3_t dest;
	vec3_t dir;

	if (!enemy || level.time >= gtime_t::from_sec(self->wait))
	{
		sphere_think_explode(self);
		return;
	}

	dest = enemy->s.origin;
	self->s.effects |= EF_ROCKET;

	dir = dest - self->s.origin;
	dir.normalize();
	self->s.angles = vectoangles(dir);
	self->velocity = dir * 1000;

	self->touch = vengeance_touch;
	self->think = sphere_think_explode;
	self->nextthink = gtime_t::from_sec(self->wait);
}

// =================
// =================
void sphere_touch(edict_t *self, edict_t *other, const trace_t &tr, mod_t mod)
{
	if (self->spawnflags.has(SPHERE_DOPPLEGANGER))
	{
		if (other == self->teammaster)
			return;

		self->takedamage = false;
		self->owner = self->teammaster;
		self->teammaster = nullptr;
	}
	else
	{
		if (other == self->owner)
			return;
		// PMM - don't blow up on bodies
		if (!strcmp(other->classname, "bodyque"))
			return;
	}

	if (tr.surface && (tr.surface->flags & SURF_SKY))
	{
		G_FreeEdict(self);
		return;
	}

	if (self->owner)
	{
		if (other->takedamage)
		{
			T_Damage(other, self, self->owner, self->velocity, self->s.origin, tr.plane.normal,
					 10000, 1, DAMAGE_DESTROY_ARMOR, mod);
		}
		else
		{
			T_RadiusDamage(self, self->owner, 512, self->owner, 256, DAMAGE_NONE, mod);
		}
	}

	sphere_think_explode(self);
}

// =================
// =================
TOUCH(vengeance_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	if (self->spawnflags.has(SPHERE_DOPPLEGANGER))
		sphere_touch(self, other, tr, MOD_DOPPLE_VENGEANCE);
	else
		sphere_touch(self, other, tr, MOD_VENGEANCE_SPHERE);
}

// =================
// =================
TOUCH(hunter_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	edict_t *owner;

	// don't blow up if you hit the world.... sheesh.
	if (other == world)
		return;

	if (self->owner)
	{
		// if owner is flying with us, make sure they stop too.
		owner = self->owner;
		if (owner->flags & FL_SAM_RAIMI)
		{
			owner->velocity = {};
			owner->movetype = MOVETYPE_NONE;
			gi.linkentity(owner);
		}
	}

	if (self->spawnflags.has(SPHERE_DOPPLEGANGER))
		sphere_touch(self, other, tr, MOD_DOPPLE_HUNTER);
	else
		sphere_touch(self, other, tr, MOD_HUNTER_SPHERE);
}

// =================
// =================
void defender_shoot(edict_t *self, edict_t *enemy)
{
	vec3_t dir;
	vec3_t start;

	if (!(enemy->inuse) || enemy->health <= 0)
		return;

	if (enemy == self->owner)
		return;

	dir = enemy->s.origin - self->s.origin;
	dir.normalize();

	if (self->monsterinfo.attack_finished > level.time)
		return;

	if (!visible(self, self->enemy))
		return;

	start = self->s.origin;
	start[2] += 2;
	fire_blaster2(self->owner, start, dir, 10, 1000, EF_BLASTER, 0);

	self->monsterinfo.attack_finished = level.time + 400_ms;
}

// *************************
// Activation Related Stuff
// *************************

// =================
// =================
void body_gib(edict_t *self)
{
	gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
	ThrowGibs(self, 50, {
		{ 4, "models/objects/gibs/sm_meat/tris.md2" },
		{ "models/objects/gibs/skull/tris.md2" }
	});
}

// =================
// =================
PAIN(hunter_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	edict_t *owner;
	float	 dist;
	vec3_t	 dir;

	if (self->enemy)
		return;

	owner = self->owner;

	if (!(self->spawnflags & SPHERE_DOPPLEGANGER))
	{
		if (owner && (owner->health > 0))
			return;

		// PMM
		if (other == owner)
			return;
		// pmm
	}
	else
	{
		// if fired by a doppleganger, set it to 10 second timeout
		self->wait = (level.time + MINIMUM_FLY_TIME).seconds();
	}

	if ((gtime_t::from_sec(self->wait) - level.time) < MINIMUM_FLY_TIME)
		self->wait = (level.time + MINIMUM_FLY_TIME).seconds();
	self->s.effects |= EF_BLASTER | EF_TRACKER;
	self->touch = hunter_touch;
	self->enemy = other;

	// if we're not owned by a player, no sam raimi
	// if we're spawned by a doppleganger, no sam raimi
	if (self->spawnflags.has(SPHERE_DOPPLEGANGER) || !(owner && owner->client))
		return;

	// sam raimi cam is disabled if FORCE_RESPAWN is set.
	// sam raimi cam is also disabled if huntercam->value is 0.
	if (!g_dm_force_respawn->integer && huntercam->integer)
	{
		dir = other->s.origin - self->s.origin;
		dist = dir.length();

		if (owner && (dist >= 192))
		{
			// detach owner from body and send him flying
			owner->movetype = MOVETYPE_FLYMISSILE;

			// gib like we just died, even though we didn't, really.
			body_gib(owner);

			// move the sphere to the owner's current viewpoint.
			// we know it's a valid spot (or will be momentarily)
			self->s.origin = owner->s.origin;
			self->s.origin[2] += owner->viewheight;

			// move the player's origin to the sphere's new origin
			owner->s.origin = self->s.origin;
			owner->s.angles = self->s.angles;
			owner->client->v_angle = self->s.angles;
			owner->mins = { -5, -5, -5 };
			owner->maxs = { 5, 5, 5 };
			owner->client->ps.fov = 140;
			owner->s.modelindex = 0;
			owner->s.modelindex2 = 0;
			owner->viewheight = 8;
			owner->solid = SOLID_NOT;
			owner->flags |= FL_SAM_RAIMI;
			gi.linkentity(owner);

			self->solid = SOLID_BBOX;
			gi.linkentity(self);
		}
	}
}

// =================
// =================
PAIN(defender_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	// PMM
	if (other == self->owner)
		return;

	// pmm
	self->enemy = other;
}

// =================
// =================
PAIN(vengeance_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	if (self->enemy)
		return;

	if (!(self->spawnflags & SPHERE_DOPPLEGANGER))
	{
		if (self->owner && self->owner->health >= 25)
			return;

		// PMM
		if (other == self->owner)
			return;
		// pmm
	}
	else
	{
		self->wait = (level.time + MINIMUM_FLY_TIME).seconds();
	}

	if ((gtime_t::from_sec(self->wait) - level.time) < MINIMUM_FLY_TIME)
		self->wait = (level.time + MINIMUM_FLY_TIME).seconds();
	self->s.effects |= EF_ROCKET;
	self->touch = vengeance_touch;
	self->enemy = other;
}

// *************************
// Think Functions
// *************************

// ===================
// ===================
THINK(defender_think) (edict_t *self) -> void
{
	if (!self->owner)
	{
		G_FreeEdict(self);
		return;
	}

	// if we've exited the level, just remove ourselves.
	if (level.intermissiontime)
	{
		sphere_think_explode(self);
		return;
	}

	if (self->owner->health <= 0)
	{
		sphere_think_explode(self);
		return;
	}

	self->s.frame++;
	if (self->s.frame > 19)
		self->s.frame = 0;

	if (self->enemy)
	{
		if (self->enemy->health > 0)
			defender_shoot(self, self->enemy);
		else
			self->enemy = nullptr;
	}

	sphere_fly(self);

	if (self->inuse)
		self->nextthink = level.time + 10_hz;
}

// =================
// =================
THINK(hunter_think) (edict_t *self) -> void
{
	// if we've exited the level, just remove ourselves.
	if (level.intermissiontime)
	{
		sphere_think_explode(self);
		return;
	}

	edict_t *owner = self->owner;

	if (!owner && !(self->spawnflags & SPHERE_DOPPLEGANGER))
	{
		G_FreeEdict(self);
		return;
	}

	if (owner)
		self->ideal_yaw = owner->s.angles[YAW];
	else if (self->enemy) // fired by doppleganger
	{
		vec3_t dir = self->enemy->s.origin - self->s.origin;
		self->ideal_yaw = vectoyaw(dir);
	}

	M_ChangeYaw(self);

	if (self->enemy)
	{
		sphere_chase(self, 0);

		// deal with sam raimi cam
		if (owner && (owner->flags & FL_SAM_RAIMI))
		{
			if (self->inuse)
			{
				owner->movetype = MOVETYPE_FLYMISSILE;
				LookAtKiller(owner, self, self->enemy);
				// owner is flying with us, move him too
				owner->movetype = MOVETYPE_FLYMISSILE;
				owner->viewheight = (int) (self->s.origin[2] - owner->s.origin[2]);
				owner->s.origin = self->s.origin;
				owner->velocity = self->velocity;
				owner->mins = {};
				owner->maxs = {};
				gi.linkentity(owner);
			}
			else // sphere timed out
			{
				owner->velocity = {};
				owner->movetype = MOVETYPE_NONE;
				gi.linkentity(owner);
			}
		}
	}
	else
		sphere_fly(self);

	if (self->inuse)
		self->nextthink = level.time + 10_hz;
}

// =================
// =================
THINK(vengeance_think) (edict_t *self) -> void
{
	// if we've exited the level, just remove ourselves.
	if (level.intermissiontime)
	{
		sphere_think_explode(self);
		return;
	}

	if (!(self->owner) && !(self->spawnflags & SPHERE_DOPPLEGANGER))
	{
		G_FreeEdict(self);
		return;
	}

	if (self->enemy)
		sphere_chase(self, 1);
	else
		sphere_fly(self);

	if (self->inuse)
		self->nextthink = level.time + 10_hz;
}

// *************************
// Spawning / Creation
// *************************

// monsterinfo_t
// =================
// =================
edict_t *Sphere_Spawn(edict_t *owner, spawnflags_t spawnflags)
{
	edict_t *sphere;

	sphere = G_Spawn();
	sphere->s.origin = owner->s.origin;
	sphere->s.origin[2] = owner->absmax[2];
	sphere->s.angles[YAW] = owner->s.angles[YAW];
	sphere->solid = SOLID_BBOX;
	sphere->clipmask = MASK_PROJECTILE;
	sphere->s.renderfx = RF_FULLBRIGHT | RF_IR_VISIBLE;
	sphere->movetype = MOVETYPE_FLYMISSILE;

	if (spawnflags.has(SPHERE_DOPPLEGANGER))
		sphere->teammaster = owner->teammaster;
	else
		sphere->owner = owner;

	sphere->classname = "sphere";
	sphere->yaw_speed = 40;
	sphere->monsterinfo.attack_finished = 0_ms;
	sphere->spawnflags = spawnflags; // need this for the HUD to recognize sphere
	// PMM
	sphere->takedamage = false;

	switch ((spawnflags & SPHERE_TYPE).value)
	{
	case SPHERE_DEFENDER.value:
		sphere->s.modelindex = gi.modelindex("models/items/defender/tris.md2");
		sphere->s.modelindex2 = gi.modelindex("models/items/shell/tris.md2");
		sphere->s.sound = gi.soundindex("spheres/d_idle.wav");
		sphere->pain = defender_pain;
		sphere->wait = (level.time + DEFENDER_LIFESPAN).seconds();
		sphere->die = sphere_explode;
		sphere->think = defender_think;
		break;
	case SPHERE_HUNTER.value:
		sphere->s.modelindex = gi.modelindex("models/items/hunter/tris.md2");
		sphere->s.sound = gi.soundindex("spheres/h_idle.wav");
		sphere->wait = (level.time + HUNTER_LIFESPAN).seconds();
		sphere->pain = hunter_pain;
		sphere->die = sphere_if_idle_die;
		sphere->think = hunter_think;
		break;
	case SPHERE_VENGEANCE.value:
		sphere->s.modelindex = gi.modelindex("models/items/vengnce/tris.md2");
		sphere->s.sound = gi.soundindex("spheres/v_idle.wav");
		sphere->wait = (level.time + VENGEANCE_LIFESPAN).seconds();
		sphere->pain = vengeance_pain;
		sphere->die = sphere_if_idle_die;
		sphere->think = vengeance_think;
		sphere->avelocity = { 30, 30, 0 };
		break;
	default:
		gi.Com_Print("Tried to create an invalid sphere\n");
		G_FreeEdict(sphere);
		return nullptr;
	}

	sphere->nextthink = level.time + 10_hz;

	gi.linkentity(sphere);

	return sphere;
}

// =================
// Own_Sphere - attach the sphere to the client so we can
//		directly access it later
// =================
void Own_Sphere(edict_t *self, edict_t *sphere)
{
	if (!sphere)
		return;

	// ownership only for players
	if (self->client)
	{
		// if they don't have one
		if (!(self->client->owned_sphere))
		{
			self->client->owned_sphere = sphere;
		}
		// they already have one, take care of the old one
		else
		{
			if (self->client->owned_sphere->inuse)
			{
				G_FreeEdict(self->client->owned_sphere);
				self->client->owned_sphere = sphere;
			}
			else
			{
				self->client->owned_sphere = sphere;
			}
		}
	}
}

// =================
// =================
void Defender_Launch(edict_t *self)
{
	edict_t *sphere;

	sphere = Sphere_Spawn(self, SPHERE_DEFENDER);
	Own_Sphere(self, sphere);
}

// =================
// =================
void Hunter_Launch(edict_t *self)
{
	edict_t *sphere;

	sphere = Sphere_Spawn(self, SPHERE_HUNTER);
	Own_Sphere(self, sphere);
}

// =================
// =================
void Vengeance_Launch(edict_t *self)
{
	edict_t *sphere;

	sphere = Sphere_Spawn(self, SPHERE_VENGEANCE);
	Own_Sphere(self, sphere);
}
