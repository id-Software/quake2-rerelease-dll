// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "../g_local.h"

//
// ROGUE
//

//
// Monster spawning code
//
// Used by the carrier, the medic_commander, and the black widow
//
// The sequence to create a flying monster is:
//
//  FindSpawnPoint - tries to find suitable spot to spawn the monster in
//  CreateFlyMonster  - this verifies the point as good and creates the monster

// To create a ground walking monster:
//
//  FindSpawnPoint - same thing
//  CreateGroundMonster - this checks the volume and makes sure the floor under the volume is suitable
//

// FIXME - for the black widow, if we want the stalkers coming in on the roof, we'll have to tweak some things

//
// CreateMonster
//
edict_t *CreateMonster(const vec3_t &origin, const vec3_t &angles, const char *classname)
{
	edict_t *newEnt;

	newEnt = G_Spawn();

	newEnt->s.origin = origin;
	newEnt->s.angles = angles;
	newEnt->classname = classname;
	newEnt->monsterinfo.aiflags |= AI_DO_NOT_COUNT;

	newEnt->gravityVector = { 0, 0, -1 };
	ED_CallSpawn(newEnt);
	newEnt->s.renderfx |= RF_IR_VISIBLE;

	return newEnt;
}

edict_t *CreateFlyMonster(const vec3_t &origin, const vec3_t &angles, const vec3_t &mins, const vec3_t &maxs, const char *classname)
{
	if (!CheckSpawnPoint(origin, mins, maxs))
		return nullptr;

	return (CreateMonster(origin, angles, classname));
}

// This is just a wrapper for CreateMonster that looks down height # of CMUs and sees if there
// are bad things down there or not

edict_t *CreateGroundMonster(const vec3_t &origin, const vec3_t &angles, const vec3_t &entMins, const vec3_t &entMaxs, const char *classname, float height)
{
	edict_t *newEnt;

	// check the ground to make sure it's there, it's relatively flat, and it's not toxic
	if (!CheckGroundSpawnPoint(origin, entMins, entMaxs, height, -1.f))
		return nullptr;

	newEnt = CreateMonster(origin, angles, classname);
	if (!newEnt)
		return nullptr;

	return newEnt;
}

// FindSpawnPoint
// PMM - this is used by the medic commander (possibly by the carrier) to find a good spawn point
// if the startpoint is bad, try above the startpoint for a bit

bool FindSpawnPoint(const vec3_t &startpoint, const vec3_t &mins, const vec3_t &maxs, vec3_t &spawnpoint, float maxMoveUp, bool drop)
{
	spawnpoint = startpoint;

	// drop first
	if (!drop || !M_droptofloor_generic(spawnpoint, mins, maxs, false, nullptr, MASK_MONSTERSOLID, false))
	{
		spawnpoint = startpoint;

		// fix stuck if we couldn't drop initially
		if (G_FixStuckObject_Generic(spawnpoint, mins, maxs, [] (const vec3_t &start, const vec3_t &mins, const vec3_t &maxs, const vec3_t &end) {
				return gi.trace(start, mins, maxs, end, nullptr, MASK_MONSTERSOLID);
			}) == stuck_result_t::NO_GOOD_POSITION)
			return false;

		// fixed, so drop again
		if (drop && !M_droptofloor_generic(spawnpoint, mins, maxs, false, nullptr, MASK_MONSTERSOLID, false))
			return false; // ???
	}

	return true;
}

// FIXME - all of this needs to be tweaked to handle the new gravity rules
// if we ever want to spawn stuff on the roof

//
// CheckSpawnPoint
//
// PMM - checks volume to make sure we can spawn a monster there (is it solid?)
//
// This is all fliers should need

bool CheckSpawnPoint(const vec3_t &origin, const vec3_t &mins, const vec3_t &maxs)
{
	trace_t tr;

	if (!mins || !maxs)
		return false;

	tr = gi.trace(origin, mins, maxs, origin, nullptr, MASK_MONSTERSOLID);
	if (tr.startsolid || tr.allsolid)
		return false;

	if (tr.ent != world)
		return false;

	return true;
}

//
// CheckGroundSpawnPoint
//
// PMM - used for walking monsters
//  checks:
//		1)	is there a ground within the specified height of the origin?
//		2)	is the ground non-water?
//		3)	is the ground flat enough to walk on?
//

bool CheckGroundSpawnPoint(const vec3_t &origin, const vec3_t &entMins, const vec3_t &entMaxs, float height, float gravity)
{
	if (!CheckSpawnPoint(origin, entMins, entMaxs))
		return false;

	if (M_CheckBottom_Fast_Generic(origin + entMins, origin + entMaxs, false))
		return true;

	if (M_CheckBottom_Slow_Generic(origin, entMins, entMaxs, nullptr, MASK_MONSTERSOLID, false, false))
		return true;

	return false;
}

// ****************************
// SPAWNGROW stuff
// ****************************

constexpr gtime_t SPAWNGROW_LIFESPAN = 1000_ms;

THINK(spawngrow_think) (edict_t *self) -> void
{
	if (level.time >= self->timestamp)
	{
		G_FreeEdict(self->target_ent);
		G_FreeEdict(self);
		return;
	}

	self->s.angles += self->avelocity * gi.frame_time_s;

	float t = 1.f - ((level.time - self->teleport_time).seconds() / self->wait);

	self->s.scale = clamp(lerp(self->decel, self->accel, t) / 16.f, 0.001f, 16.f);
	self->s.alpha = t * t;

	self->nextthink += FRAME_TIME_MS;
}

static vec3_t SpawnGro_laser_pos(edict_t *ent)
{
	// pick random direction
	float theta = frandom(2 * PIf);
	float phi = acos(crandom());

	vec3_t d {
		sin(phi) * cos(theta),
		sin(phi) * sin(theta),
		cos(phi)
	};

	return ent->s.origin + (d * ent->owner->s.scale * 9.f);
}

THINK(SpawnGro_laser_think) (edict_t *self) -> void
{
	self->s.old_origin = SpawnGro_laser_pos(self);
	gi.linkentity(self);
	self->nextthink = level.time + 1_ms;
}

void SpawnGrow_Spawn(const vec3_t &startpos, float start_size, float end_size)
{
	edict_t *ent;

	ent = G_Spawn();
	ent->s.origin = startpos;

	ent->s.angles[0] = (float) irandom(360);
	ent->s.angles[1] = (float) irandom(360);
	ent->s.angles[2] = (float) irandom(360);

	ent->avelocity[0] = frandom(280.f, 360.f) * 2.f;
	ent->avelocity[1] = frandom(280.f, 360.f) * 2.f;
	ent->avelocity[2] = frandom(280.f, 360.f) * 2.f;

	ent->solid = SOLID_NOT;
	ent->s.renderfx |= RF_IR_VISIBLE;
	ent->movetype = MOVETYPE_NONE;
	ent->classname = "spawngro";

	ent->s.modelindex = gi.modelindex("models/items/spawngro3/tris.md2");
	ent->s.skinnum = 1;

	ent->accel = start_size;
	ent->decel = end_size;
	ent->think = spawngrow_think;

	ent->s.scale = clamp(start_size / 16.f, 0.001f, 8.f);

	ent->teleport_time = level.time;
	ent->wait = SPAWNGROW_LIFESPAN.seconds();
	ent->timestamp = level.time + SPAWNGROW_LIFESPAN;

	ent->nextthink = level.time + FRAME_TIME_MS;

	gi.linkentity(ent);

	// [Paril-KEX]
	edict_t *beam = ent->target_ent = G_Spawn();
	beam->s.modelindex = MODELINDEX_WORLD;
	beam->s.renderfx = RF_BEAM_LIGHTNING | RF_NO_ORIGIN_LERP;
	beam->s.frame = 1;
	beam->s.skinnum = 0x30303030;
	beam->classname = "spawngro_beam";
	beam->angle = end_size;
	beam->owner = ent;
	beam->s.origin = ent->s.origin;
	beam->think = SpawnGro_laser_think;
	beam->nextthink = level.time + 1_ms;
	beam->s.old_origin = SpawnGro_laser_pos(beam);
	gi.linkentity(beam);
}

// ****************************
// WidowLeg stuff
// ****************************

constexpr int32_t MAX_LEGSFRAME = 23;
constexpr gtime_t LEG_WAIT_TIME = 1_sec;

void ThrowMoreStuff(edict_t *self, const vec3_t &point);
void ThrowSmallStuff(edict_t *self, const vec3_t &point);
void ThrowWidowGibLoc(edict_t *self, const char *gibname, int damage, gib_type_t type, const vec3_t *startpos, bool fade);
void ThrowWidowGibSized(edict_t *self, const char *gibname, int damage, gib_type_t type, const vec3_t *startpos, int hitsound, bool fade);

THINK(widowlegs_think) (edict_t *self) -> void
{
	vec3_t offset;
	vec3_t point;
	vec3_t f, r, u;

	if (self->s.frame == 17)
	{
		offset = { 11.77f, -7.24f, 23.31f };
		AngleVectors(self->s.angles, f, r, u);
		point = G_ProjectSource2(self->s.origin, offset, f, r, u);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_EXPLOSION1);
		gi.WritePosition(point);
		gi.multicast(point, MULTICAST_ALL, false);
		ThrowSmallStuff(self, point);
	}

	if (self->s.frame < MAX_LEGSFRAME)
	{
		self->s.frame++;
		self->nextthink = level.time + 10_hz;
		return;
	}
	else if (self->wait == 0)
	{
		self->wait = (level.time + LEG_WAIT_TIME).seconds();
	}
	if (level.time > gtime_t::from_sec(self->wait))
	{
		AngleVectors(self->s.angles, f, r, u);

		offset = { -65.6f, -8.44f, 28.59f };
		point = G_ProjectSource2(self->s.origin, offset, f, r, u);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_EXPLOSION1);
		gi.WritePosition(point);
		gi.multicast(point, MULTICAST_ALL, false);
		ThrowSmallStuff(self, point);

		ThrowWidowGibSized(self, "models/monsters/blackwidow/gib1/tris.md2", 80 + (int) frandom(20.0f), GIB_METALLIC, &point, 0, true);
		ThrowWidowGibSized(self, "models/monsters/blackwidow/gib2/tris.md2", 80 + (int) frandom(20.0f), GIB_METALLIC, &point, 0, true);

		offset = { -1.04f, -51.18f, 7.04f };
		point = G_ProjectSource2(self->s.origin, offset, f, r, u);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_EXPLOSION1);
		gi.WritePosition(point);
		gi.multicast(point, MULTICAST_ALL, false);
		ThrowSmallStuff(self, point);

		ThrowWidowGibSized(self, "models/monsters/blackwidow/gib1/tris.md2", 80 + (int) frandom(20.0f), GIB_METALLIC, &point, 0, true);
		ThrowWidowGibSized(self, "models/monsters/blackwidow/gib2/tris.md2", 80 + (int) frandom(20.0f), GIB_METALLIC, &point, 0, true);
		ThrowWidowGibSized(self, "models/monsters/blackwidow/gib3/tris.md2", 80 + (int) frandom(20.0f), GIB_METALLIC, &point, 0, true);

		G_FreeEdict(self);
		return;
	}
	if ((level.time > gtime_t::from_sec(self->wait - 0.5f)) && (self->count == 0))
	{
		self->count = 1;
		AngleVectors(self->s.angles, f, r, u);

		offset = { 31, -88.7f, 10.96f };
		point = G_ProjectSource2(self->s.origin, offset, f, r, u);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_EXPLOSION1);
		gi.WritePosition(point);
		gi.multicast(point, MULTICAST_ALL, false);
		//		ThrowSmallStuff (self, point);

		offset = { -12.67f, -4.39f, 15.68f };
		point = G_ProjectSource2(self->s.origin, offset, f, r, u);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_EXPLOSION1);
		gi.WritePosition(point);
		gi.multicast(point, MULTICAST_ALL, false);
		//		ThrowSmallStuff (self, point);

		self->nextthink = level.time + 10_hz;
		return;
	}
	self->nextthink = level.time + 10_hz;
}

void Widowlegs_Spawn(const vec3_t &startpos, const vec3_t &angles)
{
	edict_t *ent;

	ent = G_Spawn();
	ent->s.origin = startpos;
	ent->s.angles = angles;
	ent->solid = SOLID_NOT;
	ent->s.renderfx = RF_IR_VISIBLE;
	ent->movetype = MOVETYPE_NONE;
	ent->classname = "widowlegs";

	ent->s.modelindex = gi.modelindex("models/monsters/legs/tris.md2");
	ent->think = widowlegs_think;

	ent->nextthink = level.time + 10_hz;
	gi.linkentity(ent);
}
