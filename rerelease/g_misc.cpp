// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_misc.c

#include "g_local.h"

/*QUAKED func_group (0 0 0) ?
Used to group brushes together just for editor convenience.
*/

//=====================================================

USE(Use_Areaportal) (edict_t *ent, edict_t *other, edict_t *activator) -> void
{
	ent->count ^= 1; // toggle state
	gi.SetAreaPortalState(ent->style, ent->count);
}

/*QUAKED func_areaportal (0 0 0) ?

This is a non-visible object that divides the world into
areas that are seperated when this portal is not activated.
Usually enclosed in the middle of a door.
*/
void SP_func_areaportal(edict_t *ent)
{
	ent->use = Use_Areaportal;
	ent->count = 0; // always start closed;
}

//=====================================================

/*
=================
Misc functions
=================
*/
void VelocityForDamage(int damage, vec3_t &v)
{
	v[0] = 100.0f * crandom();
	v[1] = 100.0f * crandom();
	v[2] = frandom(200.0f, 300.0f);

	if (damage < 50)
		v = v * 0.7f;
	else
		v = v * 1.2f;
}

void ClipGibVelocity(edict_t *ent)
{
	if (ent->velocity[0] < -300)
		ent->velocity[0] = -300;
	else if (ent->velocity[0] > 300)
		ent->velocity[0] = 300;
	if (ent->velocity[1] < -300)
		ent->velocity[1] = -300;
	else if (ent->velocity[1] > 300)
		ent->velocity[1] = 300;
	if (ent->velocity[2] < 200)
		ent->velocity[2] = 200; // always some upwards
	else if (ent->velocity[2] > 500)
		ent->velocity[2] = 500;
}

/*
=================
gibs
=================
*/
DIE(gib_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	if (mod.id == MOD_CRUSH)
		G_FreeEdict(self);
}

TOUCH(gib_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	if (tr.plane.normal[2] > 0.7f)
	{
		self->s.angles[0] = clamp(self->s.angles[0], -5.0f, 5.0f);
		self->s.angles[2] = clamp(self->s.angles[2], -5.0f, 5.0f);
	}
}

edict_t *ThrowGib(edict_t *self, const char *gibname, int damage, gib_type_t type, float scale)
{
	edict_t *gib;
	vec3_t	 vd;
	vec3_t	 origin;
	vec3_t	 size;
	float	 vscale;

	if (type & GIB_HEAD)
	{
		gib = self;
		gib->s.event = EV_OTHER_TELEPORT;
		// remove setskin so that it doesn't set the skin wrongly later
		self->monsterinfo.setskin = nullptr;
	}
	else
		gib = G_Spawn();

	size = self->size * 0.5f;
	// since absmin is bloated by 1, un-bloat it here
	origin = (self->absmin + vec3_t { 1, 1, 1 }) + size;

	int32_t i;

	for (i = 0; i < 3; i++)
	{
		gib->s.origin = origin + vec3_t { crandom(), crandom(), crandom() }.scaled(size);

		// try 3 times to get a good, non-solid position
		if (!(gi.pointcontents(gib->s.origin) & MASK_SOLID))
			break;
	}

	if (i == 3)
	{
		// only free us if we're not being turned into the gib, otherwise
		// just spawn inside a wall
		if (gib != self)
		{
			G_FreeEdict(gib);
			return nullptr;
		}
	}
	
	gib->s.modelindex = gi.modelindex(gibname);
	gib->s.modelindex2 = 0;
	gib->s.scale = scale;
	gib->solid = SOLID_NOT;
	gib->svflags |= SVF_DEADMONSTER;
	gib->svflags &= ~SVF_MONSTER;
	gib->clipmask = MASK_SOLID;
	gib->s.effects = EF_NONE;
	gib->s.renderfx = RF_LOW_PRIORITY;
	gib->s.renderfx |= RF_NOSHADOW;
	if (!(type & GIB_DEBRIS))
	{
		if (type & GIB_ACID)
			gib->s.effects |= EF_GREENGIB;
		else
			gib->s.effects |= EF_GIB;
		gib->s.renderfx |= RF_IR_VISIBLE;
	}
	gib->flags |= FL_NO_KNOCKBACK | FL_NO_DAMAGE_EFFECTS;
	gib->takedamage = true;
	gib->die = gib_die;
	gib->classname = "gib";
	if (type & GIB_SKINNED)
		gib->s.skinnum = self->s.skinnum;
	else
		gib->s.skinnum = 0;
	gib->s.frame = 0;
	gib->mins = gib->maxs = {};
	gib->s.sound = 0;
	gib->monsterinfo.engine_sound = 0;

	if (!(type & GIB_METALLIC))
	{
		gib->movetype = MOVETYPE_TOSS;
		vscale = (type & GIB_ACID) ? 3.0 : 0.5;
	}
	else
	{
		gib->movetype = MOVETYPE_BOUNCE;
		vscale = 1.0;
	}

	if (type & GIB_DEBRIS)
	{
		vec3_t v;
		v[0] = 100 * crandom();
		v[1] = 100 * crandom();
		v[2] = 100 + 100 * crandom();
		gib->velocity = self->velocity + (v * damage);
	}
	else
	{
		VelocityForDamage(damage, vd);
		gib->velocity = self->velocity + (vd * vscale);
		ClipGibVelocity(gib);
	}

	if (type & GIB_UPRIGHT)
	{
		gib->touch = gib_touch;
		gib->flags |= FL_ALWAYS_TOUCH;
	}

	gib->avelocity[0] = frandom(600);
	gib->avelocity[1] = frandom(600);
	gib->avelocity[2] = frandom(600);

	gib->s.angles[0] = frandom(359);
	gib->s.angles[1] = frandom(359);
	gib->s.angles[2] = frandom(359);

	gib->think = G_FreeEdict;

	if (g_instagib->integer)
		gib->nextthink = level.time + random_time(1_sec, 5_sec);
	else
		gib->nextthink = level.time + random_time(10_sec, 20_sec);

	gi.linkentity(gib);

	gib->watertype = gi.pointcontents(gib->s.origin);

	if (gib->watertype & MASK_WATER)
		gib->waterlevel = WATER_FEET;
	else
		gib->waterlevel = WATER_NONE;

	return gib;
}

void ThrowClientHead(edict_t *self, int damage)
{
	vec3_t		vd;
	const char *gibname;

	if (brandom())
	{
		gibname = "models/objects/gibs/head2/tris.md2";
		self->s.skinnum = 1; // second skin is player
	}
	else
	{
		gibname = "models/objects/gibs/skull/tris.md2";
		self->s.skinnum = 0;
	}

	self->s.origin[2] += 32;
	self->s.frame = 0;
	gi.setmodel(self, gibname);
	self->mins = { -16, -16, 0 };
	self->maxs = { 16, 16, 16 };

	self->takedamage = true; // [Paril-KEX] allow takedamage so we get crushed
	self->solid = SOLID_TRIGGER; // [Paril-KEX] make 'trigger' so we still move but don't block shots/explode
	self->svflags |= SVF_DEADMONSTER;
	self->s.effects = EF_GIB;
	// PGM
	self->s.renderfx |= RF_IR_VISIBLE;
	// PGM
	self->s.sound = 0;
	self->flags |= FL_NO_KNOCKBACK | FL_NO_DAMAGE_EFFECTS;

	self->movetype = MOVETYPE_BOUNCE;
	VelocityForDamage(damage, vd);
	self->velocity += vd;

	if (self->client) // bodies in the queue don't have a client anymore
	{
		self->client->anim_priority = ANIM_DEATH;
		self->client->anim_end = self->s.frame;
	}
	else
	{
		self->think = nullptr;
		self->nextthink = 0_ms;
	}

	gi.linkentity(self);
}

void BecomeExplosion1(edict_t *self)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PHS, false);

	G_FreeEdict(self);
}

void BecomeExplosion2(edict_t *self)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION2);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PHS, false);

	G_FreeEdict(self);
}

/*QUAKED path_corner (.5 .3 0) (-8 -8 -8) (8 8 8) TELEPORT
Target: next path corner
Pathtarget: gets used when an entity that has
	this path_corner targeted touches it
*/

TOUCH(path_corner_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	vec3_t	 v;
	edict_t *next;

	if (other->movetarget != self)
		return;

	if (other->enemy)
		return;

	if (self->pathtarget)
	{
		const char *savetarget;

		savetarget = self->target;
		self->target = self->pathtarget;
		G_UseTargets(self, other);
		self->target = savetarget;
	}

	// see m_move; this is just so we don't needlessly check it
	self->flags |= FL_PARTIALGROUND;

	if (self->target)
		next = G_PickTarget(self->target);
	else
		next = nullptr;

	// [Paril-KEX] don't teleport to a point_combat, it means HOLD for them.
	if ((next) && !strcmp(next->classname, "path_corner") && next->spawnflags.has(SPAWNFLAG_PATH_CORNER_TELEPORT))
	{
		v = next->s.origin;
		v[2] += next->mins[2];
		v[2] -= other->mins[2];
		other->s.origin = v;
		next = G_PickTarget(next->target);
		other->s.event = EV_OTHER_TELEPORT;
	}

	other->goalentity = other->movetarget = next;

	if (self->wait)
	{
		other->monsterinfo.pausetime = level.time + gtime_t::from_sec(self->wait);
		other->monsterinfo.stand(other);
		return;
	}

	if (!other->movetarget)
	{
		// N64 cutscene behavior
		if (other->hackflags & HACKFLAG_END_CUTSCENE)
		{
			G_FreeEdict(other);
			return;
		}

		other->monsterinfo.pausetime = HOLD_FOREVER;
		other->monsterinfo.stand(other);
	}
	else
	{
		v = other->goalentity->s.origin - other->s.origin;
		other->ideal_yaw = vectoyaw(v);
	}
}

void SP_path_corner(edict_t *self)
{
	if (!self->targetname)
	{
		gi.Com_PrintFmt("{} with no targetname\n", *self);
		G_FreeEdict(self);
		return;
	}

	self->solid = SOLID_TRIGGER;
	self->touch = path_corner_touch;
	self->mins = { -8, -8, -8 };
	self->maxs = { 8, 8, 8 };
	self->svflags |= SVF_NOCLIENT;
	gi.linkentity(self);
}

/*QUAKED point_combat (0.5 0.3 0) (-8 -8 -8) (8 8 8) Hold
Makes this the target of a monster and it will head here
when first activated before going after the activator.  If
hold is selected, it will stay here.
*/
TOUCH(point_combat_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	edict_t *activator;

	if (other->movetarget != self)
		return;

	if (self->target)
	{
		other->target = self->target;
		other->goalentity = other->movetarget = G_PickTarget(other->target);
		if (!other->goalentity)
		{
			gi.Com_PrintFmt("{} target {} does not exist\n", *self, self->target);
			other->movetarget = self;
		}
		// [Paril-KEX] allow them to be re-used
		//self->target = nullptr;
	}
	else if (self->spawnflags.has(SPAWNFLAG_POINT_COMBAT_HOLD) && !(other->flags & (FL_SWIM | FL_FLY)))
	{
		// already standing
		if (other->monsterinfo.aiflags & AI_STAND_GROUND)
			return;

		other->monsterinfo.pausetime = HOLD_FOREVER;
		other->monsterinfo.aiflags |= AI_STAND_GROUND | AI_REACHED_HOLD_COMBAT | AI_THIRD_EYE;
		other->monsterinfo.stand(other);
	}

	if (other->movetarget == self)
	{
		// [Paril-KEX] if we're holding, keep movetarget set; we will
		// use this to make sure we haven't moved too far from where
		// we want to "guard".
		if (!self->spawnflags.has(SPAWNFLAG_POINT_COMBAT_HOLD))
		{
			other->target = nullptr;
			other->movetarget = nullptr;
		}

		other->goalentity = other->enemy;
		other->monsterinfo.aiflags &= ~AI_COMBAT_POINT;
	}

	if (self->pathtarget)
	{
		const char *savetarget;

		savetarget = self->target;
		self->target = self->pathtarget;
		if (other->enemy && other->enemy->client)
			activator = other->enemy;
		else if (other->oldenemy && other->oldenemy->client)
			activator = other->oldenemy;
		else if (other->activator && other->activator->client)
			activator = other->activator;
		else
			activator = other;
		G_UseTargets(self, activator);
		self->target = savetarget;
	}
}

void SP_point_combat(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}
	self->solid = SOLID_TRIGGER;
	self->touch = point_combat_touch;
	self->mins = { -8, -8, -16 };
	self->maxs = { 8, 8, 16 };
	self->svflags = SVF_NOCLIENT;
	gi.linkentity(self);
}

/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for spotlights, etc.
*/
void SP_info_null(edict_t *self)
{
	G_FreeEdict(self);
}

/*QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for lightning.
*/
void SP_info_notnull(edict_t *self)
{
	self->absmin = self->s.origin;
	self->absmax = self->s.origin;
}

/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) START_OFF ALLOW_IN_DM
Non-displayed light.
Default light value is 300.
Default style is 0.
If targeted, will toggle between on and off.
Default _cone value is 10 (used to set size of light for spotlights)
*/

constexpr spawnflags_t SPAWNFLAG_LIGHT_START_OFF = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAG_LIGHT_ALLOW_IN_DM = 2_spawnflag;

USE(light_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	if (self->spawnflags.has(SPAWNFLAG_LIGHT_START_OFF))
	{
		gi.configstring(CS_LIGHTS + self->style, self->style_on);
		self->spawnflags &= ~SPAWNFLAG_LIGHT_START_OFF;
	}
	else
	{
		gi.configstring(CS_LIGHTS + self->style, self->style_off);
		self->spawnflags |= SPAWNFLAG_LIGHT_START_OFF;
	}
}

// ---------------------------------------------------------------------------------
// [Sam-KEX] For keeping track of shadow light parameters and setting them up on
// the server side.

// TODO move to level_locals_t
struct shadow_light_info_t
{
	int entity_number;
	shadow_light_data_t shadowlight;
};

static shadow_light_info_t shadowlightinfo[MAX_SHADOW_LIGHTS];

const shadow_light_data_t *GetShadowLightData(int32_t entity_number)
{
	for (int32_t i = 0; i < level.shadow_light_count; i++)
	{
		if (shadowlightinfo[i].entity_number == entity_number)
			return &shadowlightinfo[i].shadowlight;
	}

	return nullptr;
}

void setup_shadow_lights()
{
	for(int i = 0; i < level.shadow_light_count; ++i)
	{
		edict_t *self = &g_edicts[shadowlightinfo[i].entity_number];

		shadowlightinfo[i].shadowlight.lighttype = shadow_light_type_t::point;
		shadowlightinfo[i].shadowlight.conedirection = {};

		if(self->target)
		{
			edict_t* target = G_FindByString<&edict_t::targetname>(nullptr, self->target);
			if(target)
			{
				shadowlightinfo[i].shadowlight.conedirection = (target->s.origin - self->s.origin).normalized();
				shadowlightinfo[i].shadowlight.lighttype = shadow_light_type_t::cone;
			}
		}

		if (self->itemtarget)
		{
			edict_t* target = G_FindByString<&edict_t::targetname>(nullptr, self->itemtarget);
			if(target)
				shadowlightinfo[i].shadowlight.lightstyle = target->style;
		}

		gi.configstring(CS_SHADOWLIGHTS + i, G_Fmt("{};{};{:1};{};{:1};{:1};{:1};{};{:1};{:1};{:1};{:1}",
			self->s.number,
			(int)shadowlightinfo[i].shadowlight.lighttype,
			shadowlightinfo[i].shadowlight.radius,
			shadowlightinfo[i].shadowlight.resolution,
			shadowlightinfo[i].shadowlight.intensity,
			shadowlightinfo[i].shadowlight.fade_start,
			shadowlightinfo[i].shadowlight.fade_end,
			shadowlightinfo[i].shadowlight.lightstyle,
			shadowlightinfo[i].shadowlight.coneangle,
			shadowlightinfo[i].shadowlight.conedirection[0],
			shadowlightinfo[i].shadowlight.conedirection[1],
			shadowlightinfo[i].shadowlight.conedirection[2]).data());
	}
}

// fix an oversight in shadow light code that causes
// lights to be ordered wrong on return levels
// if the spawn functions are changed.
// this will work without changing the save/load code.
void G_LoadShadowLights()
{
	for (size_t i = 0; i < level.shadow_light_count; i++)
	{
		const char *cstr = gi.get_configstring(CS_SHADOWLIGHTS + i);
		const char *token = COM_ParseEx(&cstr, ";");

		if (token && *token)
		{
			shadowlightinfo[i].entity_number = atoi(token);

			token = COM_ParseEx(&cstr, ";");
			shadowlightinfo[i].shadowlight.lighttype = (shadow_light_type_t) atoi(token);

			token = COM_ParseEx(&cstr, ";");
			shadowlightinfo[i].shadowlight.radius = atof(token);

			token = COM_ParseEx(&cstr, ";");
			shadowlightinfo[i].shadowlight.resolution = atoi(token);

			token = COM_ParseEx(&cstr, ";");
			shadowlightinfo[i].shadowlight.intensity = atof(token);

			token = COM_ParseEx(&cstr, ";");
			shadowlightinfo[i].shadowlight.fade_start = atof(token);

			token = COM_ParseEx(&cstr, ";");
			shadowlightinfo[i].shadowlight.fade_end = atof(token);

			token = COM_ParseEx(&cstr, ";");
			shadowlightinfo[i].shadowlight.lightstyle = atoi(token);

			token = COM_ParseEx(&cstr, ";");
			shadowlightinfo[i].shadowlight.coneangle = atof(token);

			token = COM_ParseEx(&cstr, ";");
			shadowlightinfo[i].shadowlight.conedirection[0] = atof(token);

			token = COM_ParseEx(&cstr, ";");
			shadowlightinfo[i].shadowlight.conedirection[1] = atof(token);

			token = COM_ParseEx(&cstr, ";");
			shadowlightinfo[i].shadowlight.conedirection[2] = atof(token);
		}
	}
}
// ---------------------------------------------------------------------------------

static void setup_dynamic_light(edict_t* self)
{
	// [Sam-KEX] Shadow stuff
	if (st.sl.data.radius > 0)
	{
		self->s.renderfx = RF_CASTSHADOW;
		self->itemtarget = st.sl.lightstyletarget;

		shadowlightinfo[level.shadow_light_count].entity_number = self->s.number;
		shadowlightinfo[level.shadow_light_count].shadowlight = st.sl.data;
		level.shadow_light_count++;

		self->mins[0] = self->mins[1] = self->mins[2] = 0;
		self->maxs[0] = self->maxs[1] = self->maxs[2] = 0;

		gi.linkentity(self);
	}
}

USE(dynamic_light_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	self->svflags ^= SVF_NOCLIENT;
}

void SP_dynamic_light(edict_t* self)
{
	setup_dynamic_light(self);

	if (self->targetname)
	{
		self->use = dynamic_light_use;
	}
	
	if (self->spawnflags.has(SPAWNFLAG_LIGHT_START_OFF))
		self->svflags ^= SVF_NOCLIENT;
}

void SP_light(edict_t *self)
{
	// no targeted lights in deathmatch, because they cause global messages
	if((!self->targetname || (deathmatch->integer && !(self->spawnflags.has(SPAWNFLAG_LIGHT_ALLOW_IN_DM)))) && st.sl.data.radius == 0) // [Sam-KEX]
	{
		G_FreeEdict(self);
		return;
	}

	if (self->style >= 32)
	{
		self->use = light_use;

		if (!self->style_on || !*self->style_on)
			self->style_on = "m";
		else if (*self->style_on >= '0' && *self->style_on <= '9')
			self->style_on = gi.get_configstring(CS_LIGHTS + atoi(self->style_on));
		if (!self->style_off || !*self->style_off)
			self->style_off = "a";
		else if (*self->style_off >= '0' && *self->style_off <= '9')
			self->style_off = gi.get_configstring(CS_LIGHTS + atoi(self->style_off));

		if (self->spawnflags.has(SPAWNFLAG_LIGHT_START_OFF))
			gi.configstring(CS_LIGHTS + self->style, self->style_off);
		else
			gi.configstring(CS_LIGHTS + self->style, self->style_on);
	}

	setup_dynamic_light(self);
}

/*QUAKED func_wall (0 .5 .8) ? TRIGGER_SPAWN TOGGLE START_ON ANIMATED ANIMATED_FAST
This is just a solid wall if not inhibited

TRIGGER_SPAWN	the wall will not be present until triggered
				it will then blink in to existance; it will
				kill anything that was in it's way

TOGGLE			only valid for TRIGGER_SPAWN walls
				this allows the wall to be turned on and off

START_ON		only valid for TRIGGER_SPAWN walls
				the wall will initially be present
*/

constexpr spawnflags_t SPAWNFLAG_WALL_TRIGGER_SPAWN = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAG_WALL_TOGGLE = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAG_WALL_START_ON = 4_spawnflag;
constexpr spawnflags_t SPAWNFLAG_WALL_ANIMATED = 8_spawnflag;
constexpr spawnflags_t SPAWNFLAG_WALL_ANIMATED_FAST = 16_spawnflag;

USE(func_wall_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	if (self->solid == SOLID_NOT)
	{
		self->solid = SOLID_BSP;
		self->svflags &= ~SVF_NOCLIENT;
		gi.linkentity(self);
		KillBox(self, false);
	}
	else
	{
		self->solid = SOLID_NOT;
		self->svflags |= SVF_NOCLIENT;
		gi.linkentity(self);
	}

	if (!self->spawnflags.has(SPAWNFLAG_WALL_TOGGLE))
		self->use = nullptr;
}

void SP_func_wall(edict_t *self)
{
	self->movetype = MOVETYPE_PUSH;
	gi.setmodel(self, self->model);

	if (self->spawnflags.has(SPAWNFLAG_WALL_ANIMATED))
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags.has(SPAWNFLAG_WALL_ANIMATED_FAST))
		self->s.effects |= EF_ANIM_ALLFAST;

	// just a wall
	if (!self->spawnflags.has(SPAWNFLAG_WALL_TRIGGER_SPAWN | SPAWNFLAG_WALL_TOGGLE | SPAWNFLAG_WALL_START_ON))
	{
		self->solid = SOLID_BSP;
		gi.linkentity(self);
		return;
	}

	// it must be TRIGGER_SPAWN
	if (!(self->spawnflags & SPAWNFLAG_WALL_TRIGGER_SPAWN))
		self->spawnflags |= SPAWNFLAG_WALL_TRIGGER_SPAWN;

	// yell if the spawnflags are odd
	if (self->spawnflags.has(SPAWNFLAG_WALL_START_ON))
	{
		if (!self->spawnflags.has(SPAWNFLAG_WALL_TOGGLE))
		{
			gi.Com_Print("func_wall START_ON without TOGGLE\n");
			self->spawnflags |= SPAWNFLAG_WALL_TOGGLE;
		}
	}

	self->use = func_wall_use;
	if (self->spawnflags.has(SPAWNFLAG_WALL_START_ON))
	{
		self->solid = SOLID_BSP;
	}
	else
	{
		self->solid = SOLID_NOT;
		self->svflags |= SVF_NOCLIENT;
	}
	gi.linkentity(self);
}

// [Paril-KEX]
/*QUAKED func_animation (0 .5 .8) ? START_ON
Similar to func_wall, but triggering it will toggle animation
state rather than going on/off.

START_ON		will start in alterate animation
*/

constexpr spawnflags_t SPAWNFLAG_ANIMATION_START_ON = 1_spawnflag;

USE(func_animation_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	self->bmodel_anim.alternate = !self->bmodel_anim.alternate;
}

void SP_func_animation(edict_t *self)
{
	if (!self->bmodel_anim.enabled)
	{
		gi.Com_PrintFmt("{} has no animation data\n", *self);
		G_FreeEdict(self);
		return;
	}

	self->movetype = MOVETYPE_PUSH;
	gi.setmodel(self, self->model);
	self->solid = SOLID_BSP;

	self->use = func_animation_use;
	self->bmodel_anim.alternate = self->spawnflags.has(SPAWNFLAG_ANIMATION_START_ON);

	if (self->bmodel_anim.alternate)
		self->s.frame = self->bmodel_anim.alt_start;
	else
		self->s.frame = self->bmodel_anim.start;

	gi.linkentity(self);
}

/*QUAKED func_object (0 .5 .8) ? TRIGGER_SPAWN ANIMATED ANIMATED_FAST
This is solid bmodel that will fall if it's support it removed.
*/

constexpr spawnflags_t SPAWNFLAGS_OBJECT_TRIGGER_SPAWN = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_OBJECT_ANIMATED = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_OBJECT_ANIMATED_FAST = 4_spawnflag;

TOUCH(func_object_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	// only squash thing we fall on top of
	if (other_touching_self)
		return;
	if (tr.plane.normal[2] < 1.0f)
		return;
	if (other->takedamage == false)
		return;
	if (other->damage_debounce_time > level.time)
		return;
	T_Damage(other, self, self, vec3_origin, closest_point_to_box(other->s.origin, self->absmin, self->absmax), tr.plane.normal, self->dmg, 1, DAMAGE_NONE, MOD_CRUSH);
	other->damage_debounce_time = level.time + 10_hz;
}

THINK(func_object_release) (edict_t *self) -> void
{
	self->movetype = MOVETYPE_TOSS;
	self->touch = func_object_touch;
}

USE(func_object_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	self->solid = SOLID_BSP;
	self->svflags &= ~SVF_NOCLIENT;
	self->use = nullptr;
	func_object_release(self);
	KillBox(self, false);
}

void SP_func_object(edict_t *self)
{
	gi.setmodel(self, self->model);

	self->mins[0] += 1;
	self->mins[1] += 1;
	self->mins[2] += 1;
	self->maxs[0] -= 1;
	self->maxs[1] -= 1;
	self->maxs[2] -= 1;

	if (!self->dmg)
		self->dmg = 100;

	if (!(self->spawnflags & SPAWNFLAGS_OBJECT_TRIGGER_SPAWN))
	{
		self->solid = SOLID_BSP;
		self->movetype = MOVETYPE_PUSH;
		self->think = func_object_release;
		self->nextthink = level.time + 20_hz;
	}
	else
	{
		self->solid = SOLID_NOT;
		self->movetype = MOVETYPE_PUSH;
		self->use = func_object_use;
		self->svflags |= SVF_NOCLIENT;
	}

	if (self->spawnflags.has(SPAWNFLAGS_OBJECT_ANIMATED))
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags.has(SPAWNFLAGS_OBJECT_ANIMATED_FAST))
		self->s.effects |= EF_ANIM_ALLFAST;

	self->clipmask = MASK_MONSTERSOLID;
	self->flags |= FL_NO_STANDING;

	gi.linkentity(self);
}

/*QUAKED func_explosive (0 .5 .8) ? Trigger_Spawn ANIMATED ANIMATED_FAST INACTIVE ALWAYS_SHOOTABLE
Any brush that you want to explode or break apart.  If you want an
ex0plosion, set dmg and it will do a radius explosion of that amount
at the center of the bursh.

If targeted it will not be shootable.

INACTIVE - specifies that the entity is not explodable until triggered. If you use this you must
target the entity you want to trigger it. This is the only entity approved to activate it.

health defaults to 100.

mass defaults to 75.  This determines how much debris is emitted when
it explodes.  You get one large chunk per 100 of mass (up to 8) and
one small chunk per 25 of mass (up to 16).  So 800 gives the most.
*/

constexpr spawnflags_t SPAWNFLAGS_EXPLOSIVE_TRIGGER_SPAWN = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_EXPLOSIVE_ANIMATED = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_EXPLOSIVE_ANIMATED_FAST = 4_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_EXPLOSIVE_INACTIVE = 8_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_EXPLOSIVE_ALWAYS_SHOOTABLE = 16_spawnflag;

DIE(func_explosive_explode) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	size_t   count;
	int		 mass;
	edict_t *master;
	bool	 done = false;

	self->takedamage = false;

	if (self->dmg)
		T_RadiusDamage(self, attacker, (float) self->dmg, nullptr, (float) (self->dmg + 40), DAMAGE_NONE, MOD_EXPLOSIVE);

	self->velocity = inflictor->s.origin - self->s.origin;
	self->velocity.normalize();
	self->velocity *= 150;

	mass = self->mass;
	if (!mass)
		mass = 75;

	// big chunks
	if (mass >= 100)
	{
		count = mass / 100;
		if (count > 8)
			count = 8;
		ThrowGibs(self, 1, {
			{ count, "models/objects/debris1/tris.md2", GIB_METALLIC | GIB_DEBRIS }
		});
	}

	// small chunks
	count = mass / 25;
	if (count > 16)
		count = 16;
	ThrowGibs(self, 2, {
		{ count, "models/objects/debris2/tris.md2", GIB_METALLIC | GIB_DEBRIS }
	});

	// PMM - if we're part of a train, clean ourselves out of it
	if (self->flags & FL_TEAMSLAVE)
	{
		if (self->teammaster)
		{
			master = self->teammaster;
			if (master && master->inuse) // because mappers (other than jim (usually)) are stupid....
			{
				while (!done)
				{
					if (master->teamchain == self)
					{
						master->teamchain = self->teamchain;
						done = true;
					}
					master = master->teamchain;
				}
			}
		}
	}

	G_UseTargets(self, attacker);

	self->s.origin = (self->absmin + self->absmax) * 0.5f;
	
	if (self->noise_index)
		gi.positioned_sound(self->s.origin, self, CHAN_AUTO, self->noise_index, 1, ATTN_NORM, 0);

	if (self->dmg)
		BecomeExplosion1(self);
	else
		G_FreeEdict(self);
}

USE(func_explosive_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	// Paril: pass activator to explode as attacker. this fixes
	// "strike" trying to centerprint to the relay. Should be
	// a safe change.
	func_explosive_explode(self, self, activator, self->health, vec3_origin, MOD_EXPLOSIVE);
}

// PGM
USE(func_explosive_activate) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	int approved;

	approved = 0;
	// PMM - looked like target and targetname were flipped here
	if (other != nullptr && other->target)
	{
		if (!strcmp(other->target, self->targetname))
			approved = 1;
	}
	if (!approved && activator != nullptr && activator->target)
	{
		if (!strcmp(activator->target, self->targetname))
			approved = 1;
	}

	if (!approved)
		return;

	self->use = func_explosive_use;
	if (!self->health)
		self->health = 100;
	self->die = func_explosive_explode;
	self->takedamage = true;
}
// PGM

USE(func_explosive_spawn) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	self->solid = SOLID_BSP;
	self->svflags &= ~SVF_NOCLIENT;
	self->use = nullptr;
	gi.linkentity(self);
	KillBox(self, false);
}

void SP_func_explosive(edict_t *self)
{
	if (deathmatch->integer)
	{ // auto-remove for deathmatch
		G_FreeEdict(self);
		return;
	}

	self->movetype = MOVETYPE_PUSH;

	gi.modelindex("models/objects/debris1/tris.md2");
	gi.modelindex("models/objects/debris2/tris.md2");

	gi.setmodel(self, self->model);

	if (self->spawnflags.has(SPAWNFLAGS_EXPLOSIVE_TRIGGER_SPAWN))
	{
		self->svflags |= SVF_NOCLIENT;
		self->solid = SOLID_NOT;
		self->use = func_explosive_spawn;
	}
	// PGM
	else if (self->spawnflags.has(SPAWNFLAGS_EXPLOSIVE_INACTIVE))
	{
		self->solid = SOLID_BSP;
		if (self->targetname)
			self->use = func_explosive_activate;
	}
	// PGM
	else
	{
		self->solid = SOLID_BSP;
		if (self->targetname)
			self->use = func_explosive_use;
	}

	if (self->spawnflags.has(SPAWNFLAGS_EXPLOSIVE_ANIMATED))
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags.has(SPAWNFLAGS_EXPLOSIVE_ANIMATED_FAST))
		self->s.effects |= EF_ANIM_ALLFAST;

	// PGM
	if (self->spawnflags.has(SPAWNFLAGS_EXPLOSIVE_ALWAYS_SHOOTABLE) || ((self->use != func_explosive_use) && (self->use != func_explosive_activate)))
	// PGM
	{
		if (!self->health)
			self->health = 100;
		self->die = func_explosive_explode;
		self->takedamage = true;
	}

	if (self->sounds)
	{
		if (self->sounds == 1)
			self->noise_index = gi.soundindex("world/brkglas.wav");
		else
			gi.Com_PrintFmt("{}: invalid \"sounds\" {}\n", *self, self->sounds);
	}

	gi.linkentity(self);
}

/*QUAKED misc_explobox (0 .5 .8) (-16 -16 0) (16 16 40)
Large exploding box.  You can override its mass (100),
health (80), and dmg (150).
*/

TOUCH(barrel_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	float  ratio;
	vec3_t v;

	if ((!other->groundentity) || (other->groundentity == self))
		return;
	else if (!other_touching_self)
		return;

	ratio = (float) other->mass / (float) self->mass;
	v = self->s.origin - other->s.origin;
	M_walkmove(self, vectoyaw(v), 20 * ratio * gi.frame_time_s);
}

THINK(barrel_explode) (edict_t *self) -> void
{
	self->takedamage = false;

	T_RadiusDamage(self, self->activator, (float) self->dmg, nullptr, (float) (self->dmg + 40), DAMAGE_NONE, MOD_BARREL);

	ThrowGibs(self, (1.5f * self->dmg / 200.f), {
		{ 2, "models/objects/debris1/tris.md2", GIB_METALLIC | GIB_DEBRIS },
		{ 4, "models/objects/debris3/tris.md2", GIB_METALLIC | GIB_DEBRIS },
		{ 8, "models/objects/debris2/tris.md2", GIB_METALLIC | GIB_DEBRIS }
	});

	if (self->groundentity)
		BecomeExplosion2(self);
	else
		BecomeExplosion1(self);
}

THINK(barrel_burn) (edict_t* self) -> void
{
	if (level.time >= self->timestamp)
		self->think = barrel_explode;

	self->s.effects |= EF_BARREL_EXPLODING;
	self->s.sound = gi.soundindex("weapons/bfg__l1a.wav");
	self->nextthink = level.time + FRAME_TIME_S;
}

DIE(barrel_delay) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	// allow "dead" barrels waiting to explode to still receive knockback
	if (self->think == barrel_burn || self->think == barrel_explode)
		return;
	
	// allow big booms to immediately blow up barrels (rockets, rail, other explosions) because it feels good and powerful
	if (damage >= 90)
	{
		self->think = barrel_explode;
		self->activator = attacker;
	}
	else
	{
		self->timestamp = level.time + 750_ms;
		self->think = barrel_burn;
		self->activator = attacker;
	}

}

//=========
// PGM  - change so barrels will think and hence, blow up
THINK(barrel_think) (edict_t *self) -> void
{
	// the think needs to be first since later stuff may override.
	self->think = barrel_think;
	self->nextthink = level.time + FRAME_TIME_S;

	M_CatagorizePosition(self, self->s.origin, self->waterlevel, self->watertype);
	self->flags |= FL_IMMUNE_SLIME;
	self->air_finished = level.time + 100_sec;
	M_WorldEffects(self);
}

THINK(barrel_start) (edict_t *self) -> void
{
	M_droptofloor(self);
	self->think = barrel_think;
	self->nextthink = level.time + FRAME_TIME_S;
}
// PGM
//=========

void SP_misc_explobox(edict_t *self)
{
	if (deathmatch->integer)
	{ // auto-remove for deathmatch
		G_FreeEdict(self);
		return;
	}

	gi.modelindex("models/objects/debris1/tris.md2");
	gi.modelindex("models/objects/debris2/tris.md2");
	gi.modelindex("models/objects/debris3/tris.md2");
	gi.soundindex("weapons/bfg__l1a.wav");

	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;

	self->model = "models/objects/barrels/tris.md2";
	self->s.modelindex = gi.modelindex(self->model);
	self->mins = { -16, -16, 0 };
	self->maxs = { 16, 16, 40 };

	if (!self->mass)
		self->mass = 50;
	if (!self->health)
		self->health = 10;
	if (!self->dmg)
		self->dmg = 150;

	self->die = barrel_delay;
	self->takedamage = true;
	self->flags |= FL_TRAP;

	self->touch = barrel_touch;

	// PGM - change so barrels will think and hence, blow up
	self->think = barrel_start;
	self->nextthink = level.time + 20_hz;
	// PGM

	gi.linkentity(self);
}

//
// miscellaneous specialty items
//

/*QUAKED misc_blackhole (1 .5 0) (-8 -8 -8) (8 8 8) AUTO_NOISE
model="models/objects/black/tris.md2"
*/

constexpr spawnflags_t SPAWNFLAG_BLACKHOLE_AUTO_NOISE = 1_spawnflag;

USE(misc_blackhole_use) (edict_t *ent, edict_t *other, edict_t *activator) -> void
{
	/*
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_BOSSTPORT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);
	*/
	G_FreeEdict(ent);
}

THINK(misc_blackhole_think) (edict_t *self) -> void
{
	if (self->timestamp <= level.time)
	{
		if (++self->s.frame >= 19)
			self->s.frame = 0;

		self->timestamp = level.time + 10_hz;
	}
	
	if (self->spawnflags.has(SPAWNFLAG_BLACKHOLE_AUTO_NOISE))
	{
		self->s.angles[0] += 50.0f * gi.frame_time_s;
		self->s.angles[1] += 50.0f * gi.frame_time_s;
	}

	self->nextthink = level.time + FRAME_TIME_MS;
}

void SP_misc_blackhole(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->mins = { -64, -64, 0 };
	ent->maxs = { 64, 64, 8 };
	ent->s.modelindex = gi.modelindex("models/objects/black/tris.md2");
	ent->s.renderfx = RF_TRANSLUCENT;
	ent->use = misc_blackhole_use;
	ent->think = misc_blackhole_think;
	ent->nextthink = level.time + 20_hz;

	if (ent->spawnflags.has(SPAWNFLAG_BLACKHOLE_AUTO_NOISE))
	{
		ent->s.sound = gi.soundindex("world/blackhole.wav");
		ent->s.loop_attenuation = ATTN_NORM;
	}

	gi.linkentity(ent);
}

/*QUAKED misc_eastertank (1 .5 0) (-32 -32 -16) (32 32 32)
 */

THINK(misc_eastertank_think) (edict_t *self) -> void
{
	if (++self->s.frame < 293)
		self->nextthink = level.time + 10_hz;
	else
	{
		self->s.frame = 254;
		self->nextthink = level.time + 10_hz;
	}
}

void SP_misc_eastertank(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->mins = { -32, -32, -16 };
	ent->maxs = { 32, 32, 32 };
	ent->s.modelindex = gi.modelindex("models/monsters/tank/tris.md2");
	ent->s.frame = 254;
	ent->think = misc_eastertank_think;
	ent->nextthink = level.time + 20_hz;
	gi.linkentity(ent);
}

/*QUAKED misc_easterchick (1 .5 0) (-32 -32 0) (32 32 32)
 */

THINK(misc_easterchick_think) (edict_t *self) -> void
{
	if (++self->s.frame < 247)
		self->nextthink = level.time + 10_hz;
	else
	{
		self->s.frame = 208;
		self->nextthink = level.time + 10_hz;
	}
}

void SP_misc_easterchick(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->mins = { -32, -32, 0 };
	ent->maxs = { 32, 32, 32 };
	ent->s.modelindex = gi.modelindex("models/monsters/bitch/tris.md2");
	ent->s.frame = 208;
	ent->think = misc_easterchick_think;
	ent->nextthink = level.time + 20_hz;
	gi.linkentity(ent);
}

/*QUAKED misc_easterchick2 (1 .5 0) (-32 -32 0) (32 32 32)
 */

THINK(misc_easterchick2_think) (edict_t *self) -> void
{
	if (++self->s.frame < 287)
		self->nextthink = level.time + 10_hz;
	else
	{
		self->s.frame = 248;
		self->nextthink = level.time + 10_hz;
	}
}

void SP_misc_easterchick2(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->mins = { -32, -32, 0 };
	ent->maxs = { 32, 32, 32 };
	ent->s.modelindex = gi.modelindex("models/monsters/bitch/tris.md2");
	ent->s.frame = 248;
	ent->think = misc_easterchick2_think;
	ent->nextthink = level.time + 20_hz;
	gi.linkentity(ent);
}

/*QUAKED monster_commander_body (1 .5 0) (-32 -32 0) (32 32 48)
Not really a monster, this is the Tank Commander's decapitated body.
There should be a item_commander_head that has this as it's target.
*/

THINK(commander_body_think) (edict_t *self) -> void
{
	if (++self->s.frame < 24)
		self->nextthink = level.time + 10_hz;
	else
		self->nextthink = 0_ms;

	if (self->s.frame == 22)
		gi.sound(self, CHAN_BODY, gi.soundindex("tank/thud.wav"), 1, ATTN_NORM, 0);
}

USE(commander_body_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	self->think = commander_body_think;
	self->nextthink = level.time + 10_hz;
	gi.sound(self, CHAN_BODY, gi.soundindex("tank/pain.wav"), 1, ATTN_NORM, 0);
}

THINK(commander_body_drop) (edict_t *self) -> void
{
	self->movetype = MOVETYPE_TOSS;
	self->s.origin[2] += 2;
}

void SP_monster_commander_body(edict_t *self)
{
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;
	self->model = "models/monsters/commandr/tris.md2";
	self->s.modelindex = gi.modelindex(self->model);
	self->mins = { -32, -32, 0 };
	self->maxs = { 32, 32, 48 };
	self->use = commander_body_use;
	self->takedamage = true;
	self->flags = FL_GODMODE;
	gi.linkentity(self);

	gi.soundindex("tank/thud.wav");
	gi.soundindex("tank/pain.wav");

	self->think = commander_body_drop;
	self->nextthink = level.time + 50_hz;
}

/*QUAKED misc_banner (1 .5 0) (-4 -4 -4) (4 4 4)
The origin is the bottom of the banner.
The banner is 128 tall.
model="models/objects/banner/tris.md2"
*/
THINK(misc_banner_think) (edict_t *ent) -> void
{
	ent->s.frame = (ent->s.frame + 1) % 16;
	ent->nextthink = level.time + 10_hz;
}

void SP_misc_banner(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/objects/banner/tris.md2");
	ent->s.frame = irandom(16);
	gi.linkentity(ent);

	ent->think = misc_banner_think;
	ent->nextthink = level.time + 10_hz;
}

/*QUAKED misc_deadsoldier (1 .5 0) (-16 -16 0) (16 16 16) ON_BACK ON_STOMACH BACK_DECAP FETAL_POS SIT_DECAP IMPALED
This is the dead player model. Comes in 6 exciting different poses!
*/

constexpr spawnflags_t SPAWNFLAGS_DEADSOLDIER_ON_BACK = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_DEADSOLDIER_ON_STOMACH = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_DEADSOLDIER_BACK_DECAP = 4_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_DEADSOLDIER_FETAL_POS = 8_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_DEADSOLDIER_SIT_DECAP = 16_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_DEADSOLDIER_IMPALED = 32_spawnflag;

DIE(misc_deadsoldier_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	if (self->health > -30)
		return;

	gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
	ThrowGibs(self, damage, {
		{ 4, "models/objects/gibs/sm_meat/tris.md2" },
		{ "models/objects/gibs/head2/tris.md2", GIB_HEAD }
	});
}

void SP_misc_deadsoldier(edict_t *ent)
{
	if (deathmatch->integer)
	{ // auto-remove for deathmatch
		G_FreeEdict(ent);
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->s.modelindex = gi.modelindex("models/deadbods/dude/tris.md2");

	// Defaults to frame 0
	if (ent->spawnflags.has(SPAWNFLAGS_DEADSOLDIER_ON_STOMACH))
		ent->s.frame = 1;
	else if (ent->spawnflags.has(SPAWNFLAGS_DEADSOLDIER_BACK_DECAP))
		ent->s.frame = 2;
	else if (ent->spawnflags.has(SPAWNFLAGS_DEADSOLDIER_FETAL_POS))
		ent->s.frame = 3;
	else if (ent->spawnflags.has(SPAWNFLAGS_DEADSOLDIER_SIT_DECAP))
		ent->s.frame = 4;
	else if (ent->spawnflags.has(SPAWNFLAGS_DEADSOLDIER_IMPALED))
		ent->s.frame = 5;
	else if (ent->spawnflags.has(SPAWNFLAGS_DEADSOLDIER_ON_BACK))
		ent->s.frame = 0;
	else
		ent->s.frame = 0;

	ent->mins = { -16, -16, 0 };
	ent->maxs = { 16, 16, 16 };
	ent->deadflag = true;
	ent->takedamage = true;
	// nb: SVF_MONSTER is here so it bleeds
	ent->svflags |= SVF_MONSTER | SVF_DEADMONSTER;
	ent->die = misc_deadsoldier_die;
	ent->monsterinfo.aiflags |= AI_GOOD_GUY | AI_DO_NOT_COUNT;

	gi.linkentity(ent);
}

/*QUAKED misc_viper (1 .5 0) (-16 -16 0) (16 16 32)
This is the Viper for the flyby bombing.
It is trigger_spawned, so you must have something use it for it to show up.
There must be a path for it to follow once it is activated.

"speed"		How fast the Viper should fly
*/

USE(misc_viper_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	self->svflags &= ~SVF_NOCLIENT;
	self->use = train_use;
	train_use(self, other, activator);
}

void SP_misc_viper(edict_t *ent)
{
	if (!ent->target)
	{
		gi.Com_PrintFmt("{} without a target\n", *ent);
		G_FreeEdict(ent);
		return;
	}

	if (!ent->speed)
		ent->speed = 300;

	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/ships/viper/tris.md2");
	ent->mins = { -16, -16, 0 };
	ent->maxs = { 16, 16, 32 };

	ent->think = func_train_find;
	ent->nextthink = level.time + 10_hz;
	ent->use = misc_viper_use;
	ent->svflags |= SVF_NOCLIENT;
	ent->moveinfo.accel = ent->moveinfo.decel = ent->moveinfo.speed = ent->speed;

	gi.linkentity(ent);
}

/*QUAKED misc_bigviper (1 .5 0) (-176 -120 -24) (176 120 72)
This is a large stationary viper as seen in Paul's intro
*/
void SP_misc_bigviper(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->mins = { -176, -120, -24 };
	ent->maxs = { 176, 120, 72 };
	ent->s.modelindex = gi.modelindex("models/ships/bigviper/tris.md2");
	gi.linkentity(ent);
}

/*QUAKED misc_viper_bomb (1 0 0) (-8 -8 -8) (8 8 8)
"dmg"	how much boom should the bomb make?
*/
TOUCH(misc_viper_bomb_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	G_UseTargets(self, self->activator);

	self->s.origin[2] = self->absmin[2] + 1;
	T_RadiusDamage(self, self, (float) self->dmg, nullptr, (float) (self->dmg + 40), DAMAGE_NONE, MOD_BOMB);
	BecomeExplosion2(self);
}

PRETHINK(misc_viper_bomb_prethink) (edict_t *self) -> void
{
	self->groundentity = nullptr;

	float diff = (self->timestamp - level.time).seconds();
	if (diff < -1.0f)
		diff = -1.0f;

	vec3_t v = self->moveinfo.dir * (1.0f + diff);
	v[2] = diff;

	diff = self->s.angles[2];
	self->s.angles = vectoangles(v);
	self->s.angles[2] = diff + 10;
}

USE(misc_viper_bomb_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	edict_t *viper;

	self->solid = SOLID_BBOX;
	self->svflags &= ~SVF_NOCLIENT;
	self->s.effects |= EF_ROCKET;
	self->use = nullptr;
	self->movetype = MOVETYPE_TOSS;
	self->prethink = misc_viper_bomb_prethink;
	self->touch = misc_viper_bomb_touch;
	self->activator = activator;

	viper = G_FindByString<&edict_t::classname>(nullptr, "misc_viper");
	self->velocity = viper->moveinfo.dir * viper->moveinfo.speed;

	self->timestamp = level.time;
	self->moveinfo.dir = viper->moveinfo.dir;
}

void SP_misc_viper_bomb(edict_t *self)
{
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	self->mins = { -8, -8, -8 };
	self->maxs = { 8, 8, 8 };

	self->s.modelindex = gi.modelindex("models/objects/bomb/tris.md2");

	if (!self->dmg)
		self->dmg = 1000;

	self->use = misc_viper_bomb_use;
	self->svflags |= SVF_NOCLIENT;

	gi.linkentity(self);
}

/*QUAKED misc_strogg_ship (1 .5 0) (-16 -16 0) (16 16 32)
This is a Storgg ship for the flybys.
It is trigger_spawned, so you must have something use it for it to show up.
There must be a path for it to follow once it is activated.

"speed"		How fast it should fly
*/
USE(misc_strogg_ship_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	self->svflags &= ~SVF_NOCLIENT;
	self->use = train_use;
	train_use(self, other, activator);
}

void SP_misc_strogg_ship(edict_t *ent)
{
	if (!ent->target)
	{
		gi.Com_PrintFmt("{} without a target\n", *ent);
		G_FreeEdict(ent);
		return;
	}

	if (!ent->speed)
		ent->speed = 300;

	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/ships/strogg1/tris.md2");
	ent->mins = { -16, -16, 0 };
	ent->maxs = { 16, 16, 32 };

	ent->think = func_train_find;
	ent->nextthink = level.time + 10_hz;
	ent->use = misc_strogg_ship_use;
	ent->svflags |= SVF_NOCLIENT;
	ent->moveinfo.accel = ent->moveinfo.decel = ent->moveinfo.speed = ent->speed;

	gi.linkentity(ent);
}

/*QUAKED misc_satellite_dish (1 .5 0) (-64 -64 0) (64 64 128)
model="models/objects/satellite/tris.md2"
*/
THINK(misc_satellite_dish_think) (edict_t *self) -> void
{
	self->s.frame++;
	if (self->s.frame < 38)
		self->nextthink = level.time + 10_hz;
}

USE(misc_satellite_dish_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	self->s.frame = 0;
	self->think = misc_satellite_dish_think;
	self->nextthink = level.time + 10_hz;
}

void SP_misc_satellite_dish(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->mins = { -64, -64, 0 };
	ent->maxs = { 64, 64, 128 };
	ent->s.modelindex = gi.modelindex("models/objects/satellite/tris.md2");
	ent->use = misc_satellite_dish_use;
	gi.linkentity(ent);
}

/*QUAKED light_mine1 (0 1 0) (-2 -2 -12) (2 2 12)
 */
void SP_light_mine1(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->svflags = SVF_DEADMONSTER;
	ent->s.modelindex = gi.modelindex("models/objects/minelite/light1/tris.md2");
	gi.linkentity(ent);
}

/*QUAKED light_mine2 (0 1 0) (-2 -2 -12) (2 2 12)
 */
void SP_light_mine2(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->svflags = SVF_DEADMONSTER;
	ent->s.modelindex = gi.modelindex("models/objects/minelite/light2/tris.md2");
	gi.linkentity(ent);
}

/*QUAKED misc_gib_arm (1 0 0) (-8 -8 -8) (8 8 8)
Intended for use with the target_spawner
*/
void SP_misc_gib_arm(edict_t *ent)
{
	gi.setmodel(ent, "models/objects/gibs/arm/tris.md2");
	ent->solid = SOLID_NOT;
	ent->s.effects |= EF_GIB;
	ent->takedamage = true;
	ent->die = gib_die;
	ent->movetype = MOVETYPE_TOSS;
	ent->deadflag = true;
	ent->avelocity[0] = frandom(200);
	ent->avelocity[1] = frandom(200);
	ent->avelocity[2] = frandom(200);
	ent->think = G_FreeEdict;
	ent->nextthink = level.time + 10_sec;
	gi.linkentity(ent);
}

/*QUAKED misc_gib_leg (1 0 0) (-8 -8 -8) (8 8 8)
Intended for use with the target_spawner
*/
void SP_misc_gib_leg(edict_t *ent)
{
	gi.setmodel(ent, "models/objects/gibs/leg/tris.md2");
	ent->solid = SOLID_NOT;
	ent->s.effects |= EF_GIB;
	ent->takedamage = true;
	ent->die = gib_die;
	ent->movetype = MOVETYPE_TOSS;
	ent->deadflag = true;
	ent->avelocity[0] = frandom(200);
	ent->avelocity[1] = frandom(200);
	ent->avelocity[2] = frandom(200);
	ent->think = G_FreeEdict;
	ent->nextthink = level.time + 10_sec;
	gi.linkentity(ent);
}

/*QUAKED misc_gib_head (1 0 0) (-8 -8 -8) (8 8 8)
Intended for use with the target_spawner
*/
void SP_misc_gib_head(edict_t *ent)
{
	gi.setmodel(ent, "models/objects/gibs/head/tris.md2");
	ent->solid = SOLID_NOT;
	ent->s.effects |= EF_GIB;
	ent->takedamage = true;
	ent->die = gib_die;
	ent->movetype = MOVETYPE_TOSS;
	ent->deadflag = true;
	ent->avelocity[0] = frandom(200);
	ent->avelocity[1] = frandom(200);
	ent->avelocity[2] = frandom(200);
	ent->think = G_FreeEdict;
	ent->nextthink = level.time + 10_sec;
	gi.linkentity(ent);
}

//=====================================================

/*QUAKED target_character (0 0 1) ?
used with target_string (must be on same "team")
"count" is position in the string (starts at 1)
*/

void SP_target_character(edict_t *self)
{
	self->movetype = MOVETYPE_PUSH;
	gi.setmodel(self, self->model);
	self->solid = SOLID_BSP;
	self->s.frame = 12;
	gi.linkentity(self);
	return;
}

/*QUAKED target_string (0 0 1) (-8 -8 -8) (8 8 8)
 */

USE(target_string_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	edict_t *e;
	int		 n;
	size_t	 l;
	char	 c;

	l = strlen(self->message);
	for (e = self->teammaster; e; e = e->teamchain)
	{
		if (!e->count)
			continue;
		n = e->count - 1;
		if (n > l)
		{
			e->s.frame = 12;
			continue;
		}

		c = self->message[n];
		if (c >= '0' && c <= '9')
			e->s.frame = c - '0';
		else if (c == '-')
			e->s.frame = 10;
		else if (c == ':')
			e->s.frame = 11;
		else
			e->s.frame = 12;
	}
}

void SP_target_string(edict_t *self)
{
	if (!self->message)
		self->message = "";
	self->use = target_string_use;
}

/*QUAKED func_clock (0 0 1) (-8 -8 -8) (8 8 8) TIMER_UP TIMER_DOWN START_OFF MULTI_USE
target a target_string with this

The default is to be a time of day clock

TIMER_UP and TIMER_DOWN run for "count" seconds and then fire "pathtarget"
If START_OFF, this entity must be used before it starts

"style"		0 "xx"
			1 "xx:xx"
			2 "xx:xx:xx"
*/

constexpr spawnflags_t SPAWNFLAG_TIMER_UP = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TIMER_DOWN = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TIMER_START_OFF = 4_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TIMER_MULTI_USE = 8_spawnflag;

static void func_clock_reset(edict_t *self)
{
	self->activator = nullptr;

	if (self->spawnflags.has(SPAWNFLAG_TIMER_UP))
	{
		self->health = 0;
		self->wait = (float) self->count;
	}
	else if (self->spawnflags.has(SPAWNFLAG_TIMER_DOWN))
	{
		self->health = self->count;
		self->wait = 0;
	}
}

static void func_clock_format_countdown(edict_t *self)
{
	if (self->style == 0)
	{
		G_FmtTo(self->clock_message, "{:2}", self->health);
		return;
	}

	if (self->style == 1)
	{
		G_FmtTo(self->clock_message, "{:2}:{:02}", self->health / 60, self->health % 60);
		return;
	}

	if (self->style == 2)
	{
		G_FmtTo(self->clock_message, "{:2}:{:02}:{:02}", self->health / 3600,
					(self->health - (self->health / 3600) * 3600) / 60, self->health % 60);
		return;
	}
}

THINK(func_clock_think) (edict_t *self) -> void
{
	if (!self->enemy)
	{
		self->enemy = G_FindByString<&edict_t::targetname>(nullptr, self->target);
		if (!self->enemy)
			return;
	}

	if (self->spawnflags.has(SPAWNFLAG_TIMER_UP))
	{
		func_clock_format_countdown(self);
		self->health++;
	}
	else if (self->spawnflags.has(SPAWNFLAG_TIMER_DOWN))
	{
		func_clock_format_countdown(self);
		self->health--;
	}
	else
	{
		struct tm *ltime;
		time_t	   gmtime;

		time(&gmtime);
		ltime = localtime(&gmtime);
		G_FmtTo(self->clock_message, "{:2}:{:02}:{:02}", ltime->tm_hour, ltime->tm_min,
					ltime->tm_sec);
	}

	self->enemy->message = self->clock_message;
	self->enemy->use(self->enemy, self, self);

	if ((self->spawnflags.has(SPAWNFLAG_TIMER_UP) && (self->health > self->wait)) ||
		(self->spawnflags.has(SPAWNFLAG_TIMER_DOWN) && (self->health < self->wait)))
	{
		if (self->pathtarget)
		{
			const char *savetarget;

			savetarget = self->target;
			self->target = self->pathtarget;
			G_UseTargets(self, self->activator);
			self->target = savetarget;
		}

		if (!self->spawnflags.has(SPAWNFLAG_TIMER_MULTI_USE))
			return;

		func_clock_reset(self);

		if (self->spawnflags.has(SPAWNFLAG_TIMER_START_OFF))
			return;
	}

	self->nextthink = level.time + 1_sec;
}

USE(func_clock_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	if (!self->spawnflags.has(SPAWNFLAG_TIMER_MULTI_USE))
		self->use = nullptr;
	if (self->activator)
		return;
	self->activator = activator;
	self->think(self);
}

void SP_func_clock(edict_t *self)
{
	if (!self->target)
	{
		gi.Com_PrintFmt("{} with no target\n", *self);
		G_FreeEdict(self);
		return;
	}

	if (self->spawnflags.has(SPAWNFLAG_TIMER_DOWN) && !self->count)
	{
		gi.Com_PrintFmt("{} with no count\n", *self);
		G_FreeEdict(self);
		return;
	}

	if (self->spawnflags.has(SPAWNFLAG_TIMER_UP) && (!self->count))
		self->count = 60 * 60;

	func_clock_reset(self);

	self->think = func_clock_think;

	if (self->spawnflags.has(SPAWNFLAG_TIMER_START_OFF))
		self->use = func_clock_use;
	else
		self->nextthink = level.time + 1_sec;
}

//=================================================================================

constexpr spawnflags_t SPAWNFLAG_TELEPORTER_NO_SOUND = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TELEPORTER_NO_TELEPORT_EFFECT = 2_spawnflag;

TOUCH(teleporter_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	edict_t *dest;

	if (!other->client)
		return;
	dest = G_FindByString<&edict_t::targetname>(nullptr, self->target);
	if (!dest)
	{
		gi.Com_Print("Couldn't find destination\n");
		return;
	}

	// ZOID
	CTFPlayerResetGrapple(other);
	// ZOID

	// unlink to make sure it can't possibly interfere with KillBox
	gi.unlinkentity(other);

	other->s.origin = dest->s.origin;
	other->s.old_origin = dest->s.origin;
	other->s.origin[2] += 10;

	// clear the velocity and hold them in place briefly
	other->velocity = {};
	other->client->ps.pmove.pm_time = 160; // hold time
	other->client->ps.pmove.pm_flags |= PMF_TIME_TELEPORT;

	// draw the teleport splash at source and on the player
	if (!self->spawnflags.has(SPAWNFLAG_TELEPORTER_NO_TELEPORT_EFFECT))
	{
		self->owner->s.event = EV_PLAYER_TELEPORT;
		other->s.event = EV_PLAYER_TELEPORT;
	}
	else
	{
		self->owner->s.event = EV_OTHER_TELEPORT;
		other->s.event = EV_OTHER_TELEPORT;
	}

	// set angles
	other->client->ps.pmove.delta_angles = dest->s.angles - other->client->resp.cmd_angles;

	other->s.angles = {};
	other->client->ps.viewangles = {};
	other->client->v_angle = {};
	AngleVectors(other->client->v_angle, other->client->v_forward, nullptr, nullptr);

	gi.linkentity(other);

	// kill anything at the destination
	KillBox(other, !!other->client);

	// [Paril-KEX] move sphere, if we own it
	if (other->client->owned_sphere)
	{
		edict_t *sphere = other->client->owned_sphere;
		sphere->s.origin = other->s.origin;
		sphere->s.origin[2] = other->absmax[2];
		sphere->s.angles[YAW] = other->s.angles[YAW];
		gi.linkentity(sphere);
	}
}

/*QUAKED misc_teleporter (1 0 0) (-32 -32 -24) (32 32 -16) NO_SOUND NO_TELEPORT_EFFECT N64_EFFECT
Stepping onto this disc will teleport players to the targeted misc_teleporter_dest object.
*/
constexpr spawnflags_t SPAWNFLAG_TEMEPORTER_N64_EFFECT = 4_spawnflag;

void SP_misc_teleporter(edict_t *ent)
{
	edict_t *trig;

	gi.setmodel(ent, "models/objects/dmspot/tris.md2");
	ent->s.skinnum = 1;
	if (level.is_n64 || ent->spawnflags.has(SPAWNFLAG_TEMEPORTER_N64_EFFECT))
		ent->s.effects = EF_TELEPORTER2;
	else
		ent->s.effects = EF_TELEPORTER;
	if (!(ent->spawnflags & SPAWNFLAG_TELEPORTER_NO_SOUND))
		ent->s.sound = gi.soundindex("world/amb10.wav");
	ent->solid = SOLID_BBOX;

	ent->mins = { -32, -32, -24 };
	ent->maxs = { 32, 32, -16 };
	gi.linkentity(ent);
	
	// N64 has some of these for visual effects
	if (!ent->target)
		return;

	trig = G_Spawn();
	trig->touch = teleporter_touch;
	trig->solid = SOLID_TRIGGER;
	trig->target = ent->target;
	trig->owner = ent;
	trig->s.origin = ent->s.origin;
	trig->mins = { -8, -8, 8 };
	trig->maxs = { 8, 8, 24 };
	gi.linkentity(trig);
}

/*QUAKED misc_teleporter_dest (1 0 0) (-32 -32 -24) (32 32 -16)
Point teleporters at these.
*/
void SP_misc_teleporter_dest(edict_t *ent)
{
	// Paril-KEX N64 doesn't display these
	if (level.is_n64)
		return;

	gi.setmodel(ent, "models/objects/dmspot/tris.md2");
	ent->s.skinnum = 0;
	ent->solid = SOLID_BBOX;
	//	ent->s.effects |= EF_FLIES;
	ent->mins = { -32, -32, -24 };
	ent->maxs = { 32, 32, -16 };
	gi.linkentity(ent);
}

/*QUAKED misc_flare (1.0 1.0 0.0) (-32 -32 -32) (32 32 32) RED GREEN BLUE LOCK_ANGLE
Creates a flare seen in the N64 version.
*/

static constexpr spawnflags_t SPAWNFLAG_FLARE_RED			= 1_spawnflag;
static constexpr spawnflags_t SPAWNFLAG_FLARE_GREEN			= 2_spawnflag;
static constexpr spawnflags_t SPAWNFLAG_FLARE_BLUE			= 4_spawnflag;
static constexpr spawnflags_t SPAWNFLAG_FLARE_LOCK_ANGLE	= 8_spawnflag;

USE(misc_flare_use) (edict_t *ent, edict_t *other, edict_t *activator) -> void
{
	ent->svflags ^= SVF_NOCLIENT;
	gi.linkentity(ent);
}

void SP_misc_flare(edict_t* ent)
{
	ent->s.modelindex = 1;
	ent->s.renderfx = RF_FLARE;
	ent->solid = SOLID_NOT;
    ent->s.scale = st.radius;

	if (ent->spawnflags.has(SPAWNFLAG_FLARE_RED))
		ent->s.renderfx |= RF_SHELL_RED;

	if (ent->spawnflags.has(SPAWNFLAG_FLARE_GREEN))
		ent->s.renderfx |= RF_SHELL_GREEN;

	if (ent->spawnflags.has(SPAWNFLAG_FLARE_BLUE))
		ent->s.renderfx |= RF_SHELL_BLUE;

	if (ent->spawnflags.has(SPAWNFLAG_FLARE_LOCK_ANGLE))
		ent->s.renderfx |= RF_FLARE_LOCK_ANGLE;

	if (st.image && *st.image)
	{
		ent->s.renderfx |= RF_CUSTOMSKIN;
		ent->s.frame = gi.imageindex(st.image);
	}

    ent->mins = { -32, -32, -32 };
    ent->maxs = { 32, 32, 32 };

	ent->s.modelindex2 = st.fade_start_dist;
	ent->s.modelindex3 = st.fade_end_dist;

	if (ent->targetname)
		ent->use = misc_flare_use;

    gi.linkentity(ent);
}

THINK(misc_hologram_think) (edict_t *ent) -> void
{
	ent->s.angles[1] += 100 * gi.frame_time_s;
	ent->nextthink = level.time + FRAME_TIME_MS;
	ent->s.alpha = frandom(0.2f, 0.6f);
}

/*QUAKED misc_hologram (1.0 1.0 0.0) (-16 -16 0) (16 16 32)
Ship hologram seen in the N64 version.
*/
void SP_misc_hologram(edict_t *ent)
{
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/ships/strogg1/tris.md2");
	ent->mins = { -16, -16, 0 };
	ent->maxs = { 16, 16, 32 };
	ent->s.effects = EF_HOLOGRAM;
	ent->think = misc_hologram_think;
	ent->nextthink = level.time + FRAME_TIME_MS;
	ent->s.alpha = frandom(0.2f, 0.6f);
	ent->s.scale = 0.75f;
	gi.linkentity(ent);
}


/*QUAKED misc_fireball (0 .5 .8) (-8 -8 -8) (8 8 8) NO_EXPLODE
Lava Balls. Shamelessly copied from Quake 1, like N64 guys
probably did too.
*/

constexpr spawnflags_t SPAWNFLAG_LAVABALL_NO_EXPLODE = 1_spawnflag;

TOUCH(fire_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	if (self->spawnflags.has(SPAWNFLAG_LAVABALL_NO_EXPLODE))
	{
		G_FreeEdict(self);
		return;
	}

	if (other->takedamage)
		T_Damage (other, self, self, vec3_origin, self->s.origin, vec3_origin, 20, 0, DAMAGE_NONE, MOD_EXPLOSIVE);

	if (gi.pointcontents(self->s.origin) & CONTENTS_LAVA)
		G_FreeEdict(self);
	else
		BecomeExplosion1(self);
}

THINK(fire_fly) (edict_t *self) -> void
{
	edict_t *fireball = G_Spawn();
	fireball->s.effects = EF_FIREBALL;
	fireball->s.renderfx = RF_MINLIGHT;
	fireball->solid = SOLID_BBOX;
	fireball->movetype = MOVETYPE_TOSS;
	fireball->clipmask = MASK_SHOT;
	fireball->velocity[0] = crandom() * 50;
	fireball->velocity[1] = crandom() * 50;
	fireball->avelocity = { crandom() * 360, crandom() * 360, crandom() * 360 };
	fireball->velocity[2] = (self->speed * 1.75f) + (frandom() * 200);
	fireball->classname = "fireball";
	gi.setmodel(fireball, "models/objects/gibs/sm_meat/tris.md2");
	fireball->s.origin = self->s.origin;
	fireball->nextthink = level.time + 5_sec;
	fireball->think = G_FreeEdict;
	fireball->touch = fire_touch;
	fireball->spawnflags = self->spawnflags;
	gi.linkentity(fireball);
	self->nextthink = level.time + random_time(5_sec);
}

void SP_misc_lavaball(edict_t *self)
{
	self->classname = "fireball";
	self->nextthink = level.time + random_time(5_sec);
	self->think = fire_fly;
	if (!self->speed)
		self->speed = 185;
}


void SP_info_landmark(edict_t* self)
{
	self->absmin = self->s.origin;
	self->absmax = self->s.origin;
}

constexpr spawnflags_t SPAWNFLAG_WORLD_TEXT_START_OFF = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAG_WORLD_TEXT_TRIGGER_ONCE = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAG_WORLD_TEXT_REMOVE_ON_TRIGGER = 4_spawnflag;

USE( info_world_text_use ) ( edict_t * self, edict_t * other, edict_t * activator ) -> void {
	if ( self->activator == nullptr ) {
		self->activator = activator;
		self->think( self );
	} else {
		self->nextthink = 0_ms;
		self->activator = nullptr;
	}

	if (self->spawnflags.has(SPAWNFLAG_WORLD_TEXT_TRIGGER_ONCE)) {
		self->use = nullptr;
	}

	if ( self->target != nullptr ) {
		edict_t * target = G_PickTarget( self->target );
		if ( target != nullptr && target->inuse ) {
			if ( target->use ) {
				target->use( target, self, self );
			}
		}
	}

	if (self->spawnflags.has(SPAWNFLAG_WORLD_TEXT_REMOVE_ON_TRIGGER)) {
		G_FreeEdict( self );
	}
}

THINK( info_world_text_think ) ( edict_t * self ) -> void {
	rgba_t color = rgba_white;

	switch ( self->sounds ) {
		case 0:
			color = rgba_white;
			break;

		case 1:
			color = rgba_red;
			break;

		case 2:
			color = rgba_blue;
			break;

		case 3:
			color = rgba_green;
			break;

		case 4:
			color = rgba_yellow;
			break;

		case 5:
			color = rgba_black;
			break;

		case 6:
			color = rgba_cyan;
			break;

		case 7:
			color = rgba_orange;
			break;

		default:
			color = rgba_white;
			gi.Com_PrintFmt( "{}: invalid color\n", *self);
			break;
	}

	if ( self->s.angles[ YAW ] == -3.0f ) {
		gi.Draw_OrientedWorldText( self->s.origin, self->message, color, self->size[ 2 ], FRAME_TIME_MS.seconds(), true );
	} else {
		vec3_t textAngle = { 0.0f, 0.0f, 0.0f };
		textAngle[ YAW ] = anglemod( self->s.angles[ YAW ] ) + 180;
		if ( textAngle[ YAW ] > 360.0f ) {
			textAngle[ YAW ] -= 360.0f;
		}
		gi.Draw_StaticWorldText( self->s.origin, textAngle, self->message, color, self->size[2], FRAME_TIME_MS.seconds(), true );
	}
	self->nextthink = level.time + FRAME_TIME_MS;
}

/*QUAKED info_world_text (1.0 1.0 0.0) (-16 -16 0) (16 16 32)
designer placed in world text for debugging.
*/
void SP_info_world_text( edict_t * self ) {
	if ( self->message == nullptr ) {
		gi.Com_PrintFmt( "{}: no message\n", *self);
		G_FreeEdict( self );
		return;
	} // not much point without something to print...

	self->think = info_world_text_think;
	self->use = info_world_text_use;
	self->size[ 2 ] = st.radius ? st.radius : 0.2f;

	if ( !self->spawnflags.has( SPAWNFLAG_WORLD_TEXT_START_OFF ) ) {
		self->nextthink = level.time + FRAME_TIME_MS;
		self->activator = self;
	}
}

#include "m_player.h"

USE( misc_player_mannequin_use ) ( edict_t * self, edict_t * other, edict_t * activator ) -> void {
	self->monsterinfo.aiflags |= AI_TARGET_ANGER;
	self->enemy = activator;

	switch ( self->count ) {
		case GESTURE_FLIP_OFF:
			self->s.frame = FRAME_flip01;
			self->monsterinfo.nextframe = FRAME_flip12;
			break;

		case GESTURE_SALUTE:
			self->s.frame = FRAME_salute01;
			self->monsterinfo.nextframe = FRAME_salute11;
			break;

		case GESTURE_TAUNT:
			self->s.frame = FRAME_taunt01;
			self->monsterinfo.nextframe = FRAME_taunt17;
			break;

		case GESTURE_WAVE:
			self->s.frame = FRAME_wave01;
			self->monsterinfo.nextframe = FRAME_wave11;
			break;

		case GESTURE_POINT:
			self->s.frame = FRAME_point01;
			self->monsterinfo.nextframe = FRAME_point12;
			break;
	}
}

THINK( misc_player_mannequin_think ) ( edict_t * self ) -> void {
	if ( self->teleport_time <= level.time ) {
		self->s.frame++;

		if ( ( self->monsterinfo.aiflags & AI_TARGET_ANGER ) == 0 ) {
			if ( self->s.frame > FRAME_stand40 ) {
				self->s.frame = FRAME_stand01;
			}
		} else {
			if ( self->s.frame > self->monsterinfo.nextframe ) {
				self->s.frame = FRAME_stand01;
				self->monsterinfo.aiflags &= ~AI_TARGET_ANGER;
				self->enemy = nullptr;
			}
		}

		self->teleport_time = level.time + 10_hz;
	}

	if ( self->enemy != nullptr ) {
		const vec3_t vec = ( self->enemy->s.origin - self->s.origin );
		self->ideal_yaw = vectoyaw( vec );
		M_ChangeYaw( self );
	}

	self->nextthink = level.time + FRAME_TIME_MS;
}

void SetupMannequinModel( edict_t * self, const int32_t modelType, const char * weapon, const char * skin ) {
	const char * modelName = nullptr;
	const char * defaultSkin = nullptr;

	switch ( modelType ) {
		case 1: {
			self->s.skinnum = ( MAX_CLIENTS - 1 );
			modelName = "female";
			defaultSkin = "venus";
			break;
		}

		case 2: {
			self->s.skinnum = ( MAX_CLIENTS - 2 );
			modelName = "male";
			defaultSkin = "rampage";
			break;
		}

		case 3: {
			self->s.skinnum = ( MAX_CLIENTS - 3 );
			modelName = "cyborg";
			defaultSkin = "oni911";
			break;
		}

		default: {
			self->s.skinnum = ( MAX_CLIENTS - 1 );
			modelName = "female";
			defaultSkin = "venus";
			break;
		}
	}

	if ( modelName != nullptr ) {
		self->model = G_Fmt( "players/{}/tris.md2", modelName ).data();

		const char * weaponName = nullptr;
		if ( weapon != nullptr ) {
			weaponName = G_Fmt( "players/{}/{}.md2", modelName, weapon ).data();
		} else {
			weaponName = G_Fmt( "players/{}/{}.md2", modelName, "w_hyperblaster" ).data();
		}
		self->s.modelindex2 = gi.modelindex( weaponName );

		const char * skinName = nullptr;
		if ( skin != nullptr ) {
			skinName = G_Fmt( "mannequin\\{}/{}", modelName, skin ).data();
		} else {
			skinName = G_Fmt( "mannequin\\{}/{}", modelName, defaultSkin ).data();
		}
		gi.configstring( CS_PLAYERSKINS + self->s.skinnum, skinName );
	}
}

/*QUAKED misc_player_mannequin (1.0 1.0 0.0) (-32 -32 -32) (32 32 32)
	Creates a player mannequin that stands around.

	NOTE: this is currently very limited, and only allows one unique model
	from each of the three player model types.

 "distance"		- Sets the type of gesture mannequin when use when triggered
 "height"		- Sets the type of model to use ( valid numbers: 1 - 3 )
 "goals"		- Name of the weapon to use.
 "image"		- Name of the player skin to use.
 "radius"		- How much to scale the model in-game
*/
void SP_misc_player_mannequin( edict_t * self ) {
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;
	if (!st.was_key_specified("effects"))
		self->s.effects = EF_NONE;
	if (!st.was_key_specified("renderfx"))
		self->s.renderfx = RF_MINLIGHT;
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, 32 };
	self->yaw_speed = 30;
	self->ideal_yaw = 0;
	self->teleport_time = level.time + 10_hz;
	self->s.modelindex = MODELINDEX_PLAYER;
	self->count = st.distance;

	SetupMannequinModel( self, st.height, st.goals, st.image );

	self->s.scale = 1.0f;
	if ( ai_model_scale->value > 0.0f ) {
		self->s.scale = ai_model_scale->value;
	} else if ( st.radius > 0.0f ) {
		self->s.scale = st.radius;
	}

	self->mins *= self->s.scale;
	self->maxs *= self->s.scale;

	self->think = misc_player_mannequin_think;
	self->nextthink = level.time + FRAME_TIME_MS;

	if ( self->targetname ) {
		self->use = misc_player_mannequin_use;
	}

	gi.linkentity( self );
}

/*QUAKED misc_model (1 0 0) (-8 -8 -8) (8 8 8)
*/
void SP_misc_model(edict_t *ent)
{
	gi.setmodel(ent, ent->model);
	gi.linkentity(ent);
}