// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"

// PGM - some of these are mine, some id's. I added the define's.
constexpr spawnflags_t SPAWNFLAG_TRIGGER_MONSTER = 0x01_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TRIGGER_NOT_PLAYER = 0x02_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TRIGGER_TRIGGERED = 0x04_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TRIGGER_TOGGLE = 0x08_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TRIGGER_LATCHED = 0x10_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TRIGGER_CLIP = 0x20_spawnflag;
// PGM

void InitTrigger(edict_t *self)
{
	if (st.was_key_specified("angle") || st.was_key_specified("angles") || self->s.angles)
		G_SetMovedir(self->s.angles, self->movedir);

	self->solid = SOLID_TRIGGER;
	self->movetype = MOVETYPE_NONE;
	// [Paril-KEX] adjusted to allow mins/maxs to be defined
	// by hand instead
	if (self->model)
		gi.setmodel(self, self->model);
	self->svflags = SVF_NOCLIENT;
}

// the wait time has passed, so set back up for another activation
THINK(multi_wait) (edict_t *ent) -> void
{
	ent->nextthink = 0_ms;
}

// the trigger was just activated
// ent->activator should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
void multi_trigger(edict_t *ent)
{
	if (ent->nextthink)
		return; // already been triggered

	G_UseTargets(ent, ent->activator);

	if (ent->wait > 0)
	{
		ent->think = multi_wait;
		ent->nextthink = level.time + gtime_t::from_sec(ent->wait);
	}
	else
	{ // we can't just remove (self) here, because this is a touch function
		// called while looping through area links...
		ent->touch = nullptr;
		ent->nextthink = level.time + FRAME_TIME_S;
		ent->think = G_FreeEdict;
	}
}

USE(Use_Multi) (edict_t *ent, edict_t *other, edict_t *activator) -> void
{
	// PGM
	if (ent->spawnflags.has(SPAWNFLAG_TRIGGER_TOGGLE))
	{
		if (ent->solid == SOLID_TRIGGER)
			ent->solid = SOLID_NOT;
		else
			ent->solid = SOLID_TRIGGER;
		gi.linkentity(ent);
	}
	else
	{
		ent->activator = activator;
		multi_trigger(ent);
	}
	// PGM
}

TOUCH(Touch_Multi) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	if (other->client)
	{
		if (self->spawnflags.has(SPAWNFLAG_TRIGGER_NOT_PLAYER))
			return;
	}
	else if (other->svflags & SVF_MONSTER)
	{
		if (!self->spawnflags.has(SPAWNFLAG_TRIGGER_MONSTER))
			return;
	}
	else
		return;

	if (self->spawnflags.has(SPAWNFLAG_TRIGGER_CLIP))
	{
		trace_t clip = gi.clip(self, other->s.origin, other->mins, other->maxs, other->s.origin, G_GetClipMask(other));

		if (clip.fraction == 1.0f)
			return;
	}

	if (self->movedir)
	{
		vec3_t forward;

		AngleVectors(other->s.angles, forward, nullptr, nullptr);
		if (forward.dot(self->movedir) < 0)
			return;
	}

	self->activator = other;
	multi_trigger(self);
}

/*QUAKED trigger_multiple (.5 .5 .5) ? MONSTER NOT_PLAYER TRIGGERED TOGGLE LATCHED
Variable sized repeatable trigger.  Must be targeted at one or more entities.
If "delay" is set, the trigger waits some time after activating before firing.
"wait" : Seconds between triggerings. (.2 default)

TOGGLE - using this trigger will activate/deactivate it. trigger will begin inactive.

sounds
1)	secret
2)	beep beep
3)	large switch
4)
set "message" to text string
*/
USE(trigger_enable) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	self->solid = SOLID_TRIGGER;
	self->use = Use_Multi;
	gi.linkentity(self);
}

static BoxEdictsResult_t latched_trigger_filter(edict_t *other, void *data)
{
	edict_t *self = (edict_t *) data;

	if (other->client)
	{
		if (self->spawnflags.has(SPAWNFLAG_TRIGGER_NOT_PLAYER))
			return BoxEdictsResult_t::Skip;
	}
	else if (other->svflags & SVF_MONSTER)
	{
		if (!self->spawnflags.has(SPAWNFLAG_TRIGGER_MONSTER))
			return BoxEdictsResult_t::Skip;
	}
	else
		return BoxEdictsResult_t::Skip;

	if (self->movedir)
	{
		vec3_t forward;

		AngleVectors(other->s.angles, forward, nullptr, nullptr);
		if (forward.dot(self->movedir) < 0)
			return BoxEdictsResult_t::Skip;
	}

	self->activator = other;
	return BoxEdictsResult_t::Keep | BoxEdictsResult_t::End;
}

THINK(latched_trigger_think) (edict_t *self) -> void
{
	self->nextthink = level.time + 1_ms;

	bool any_inside = !!gi.BoxEdicts(self->absmin, self->absmax, nullptr, 0, AREA_SOLID, latched_trigger_filter, self);

	if (!!self->count != any_inside)
	{
		G_UseTargets(self, self->activator);
		self->count = any_inside ? 1 : 0;
	}
}

void SP_trigger_multiple(edict_t *ent)
{
	if (ent->sounds == 1)
		ent->noise_index = gi.soundindex("misc/secret.wav");
	else if (ent->sounds == 2)
		ent->noise_index = gi.soundindex("misc/talk.wav");
	else if (ent->sounds == 3)
		ent->noise_index = gi.soundindex("misc/trigger1.wav");

	if (!ent->wait)
		ent->wait = 0.2f;

	InitTrigger(ent);

	if (ent->spawnflags.has(SPAWNFLAG_TRIGGER_LATCHED))
	{
		if (ent->spawnflags.has(SPAWNFLAG_TRIGGER_TRIGGERED | SPAWNFLAG_TRIGGER_TOGGLE))
			gi.Com_PrintFmt("{}: latched and triggered/toggle are not supported\n", *ent);

		ent->think = latched_trigger_think;
		ent->nextthink = level.time + 1_ms;
		ent->use = Use_Multi;
		return;
	}
	else
		ent->touch = Touch_Multi;

	// PGM
	if (ent->spawnflags.has(SPAWNFLAG_TRIGGER_TRIGGERED | SPAWNFLAG_TRIGGER_TOGGLE))
	// PGM
	{
		ent->solid = SOLID_NOT;
		ent->use = trigger_enable;
	}
	else
	{
		ent->solid = SOLID_TRIGGER;
		ent->use = Use_Multi;
	}

	gi.linkentity(ent);

	if (ent->spawnflags.has(SPAWNFLAG_TRIGGER_CLIP))
		ent->svflags |= SVF_HULL;
}

/*QUAKED trigger_once (.5 .5 .5) ? x x TRIGGERED
Triggers once, then removes itself.
You must set the key "target" to the name of another object in the level that has a matching "targetname".

If TRIGGERED, this trigger must be triggered before it is live.

sounds
 1)	secret
 2)	beep beep
 3)	large switch
 4)

"message"	string to be displayed when triggered
*/

void SP_trigger_once(edict_t *ent)
{
	// make old maps work because I messed up on flag assignments here
	// triggered was on bit 1 when it should have been on bit 4
	if (ent->spawnflags.has(SPAWNFLAG_TRIGGER_MONSTER))
	{
		ent->spawnflags &= ~SPAWNFLAG_TRIGGER_MONSTER;
		ent->spawnflags |= SPAWNFLAG_TRIGGER_TRIGGERED;
		gi.Com_PrintFmt("{}: fixed TRIGGERED flag\n", *ent);
	}

	ent->wait = -1;
	SP_trigger_multiple(ent);
}

/*QUAKED trigger_relay (.5 .5 .5) (-8 -8 -8) (8 8 8)
This fixed size trigger cannot be touched, it can only be fired by other events.
*/
constexpr spawnflags_t SPAWNFLAGS_TRIGGER_RELAY_NO_SOUND = 1_spawnflag;

USE(trigger_relay_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	if (self->crosslevel_flags && !(self->crosslevel_flags == (game.cross_level_flags & SFL_CROSS_TRIGGER_MASK & self->crosslevel_flags)))
		return;

	G_UseTargets(self, activator);
}

void SP_trigger_relay(edict_t *self)
{
	self->use = trigger_relay_use;

	if (self->spawnflags.has(SPAWNFLAGS_TRIGGER_RELAY_NO_SOUND))
		self->noise_index = -1;
}

/*
==============================================================================

trigger_key

==============================================================================
*/

/*QUAKED trigger_key (.5 .5 .5) (-8 -8 -8) (8 8 8)
A relay trigger that only fires it's targets if player has the proper key.
Use "item" to specify the required key, for example "key_data_cd"
*/
USE(trigger_key_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	item_id_t index;

	if (!self->item)
		return;
	if (!activator->client)
		return;

	index = self->item->id;
	if (!activator->client->pers.inventory[index])
	{
		if (level.time < self->touch_debounce_time)
			return;
		self->touch_debounce_time = level.time + 5_sec;
		gi.LocCenter_Print(activator, "$g_you_need", self->item->pickup_name_definite);
		gi.sound(activator, CHAN_AUTO, gi.soundindex("misc/keytry.wav"), 1, ATTN_NORM, 0);
		return;
	}

	gi.sound(activator, CHAN_AUTO, gi.soundindex("misc/keyuse.wav"), 1, ATTN_NORM, 0);
	if (coop->integer)
	{
		edict_t *ent;

		if (self->item->id == IT_KEY_POWER_CUBE || self->item->id == IT_KEY_EXPLOSIVE_CHARGES)
		{
			int cube;

			for (cube = 0; cube < 8; cube++)
				if (activator->client->pers.power_cubes & (1 << cube))
					break;
			for (uint32_t player = 1; player <= game.maxclients; player++)
			{
				ent = &g_edicts[player];
				if (!ent->inuse)
					continue;
				if (!ent->client)
					continue;
				if (ent->client->pers.power_cubes & (1 << cube))
				{
					ent->client->pers.inventory[index]--;
					ent->client->pers.power_cubes &= ~(1 << cube);

					// [Paril-KEX] don't allow respawning players to keep
					// used keys
					if (!P_UseCoopInstancedItems())
					{
						ent->client->resp.coop_respawn.inventory[index] = 0;
						ent->client->resp.coop_respawn.power_cubes &= ~(1 << cube);
					}
				}
			}
		}
		else
		{
			for (uint32_t player = 1; player <= game.maxclients; player++)
			{
				ent = &g_edicts[player];
				if (!ent->inuse)
					continue;
				if (!ent->client)
					continue;
				ent->client->pers.inventory[index] = 0;

				// [Paril-KEX] don't allow respawning players to keep
				// used keys
				if (!P_UseCoopInstancedItems())
					ent->client->resp.coop_respawn.inventory[index] = 0;
			}
		}
	}
	else
	{
		activator->client->pers.inventory[index]--;
	}

	G_UseTargets(self, activator);

	self->use = nullptr;
}

void SP_trigger_key(edict_t *self)
{
	if (!st.item)
	{
		gi.Com_PrintFmt("{}: no key item\n", *self);
		return;
	}
	self->item = FindItemByClassname(st.item);

	if (!self->item)
	{
		gi.Com_PrintFmt("{}: item {} not found\n", *self, st.item);
		return;
	}

	if (!self->target)
	{
		gi.Com_PrintFmt("{}: no target\n", *self);
		return;
	}

	gi.soundindex("misc/keytry.wav");
	gi.soundindex("misc/keyuse.wav");

	self->use = trigger_key_use;
}

/*
==============================================================================

trigger_counter

==============================================================================
*/

/*QUAKED trigger_counter (.5 .5 .5) ? nomessage
Acts as an intermediary for an action that takes multiple inputs.

If nomessage is not set, t will print "1 more.. " etc when triggered and "sequence complete" when finished.

After the counter has been triggered "count" times (default 2), it will fire all of it's targets and remove itself.
*/

constexpr spawnflags_t SPAWNFLAG_COUNTER_NOMESSAGE = 1_spawnflag;

USE(trigger_counter_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	if (self->count == 0)
		return;

	self->count--;

	if (self->count)
	{
		if (!(self->spawnflags & SPAWNFLAG_COUNTER_NOMESSAGE))
		{
			gi.LocCenter_Print(activator, "$g_more_to_go", self->count);
			gi.sound(activator, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
		}
		return;
	}

	if (!(self->spawnflags & SPAWNFLAG_COUNTER_NOMESSAGE))
	{
		gi.LocCenter_Print(activator, "$g_sequence_completed");
		gi.sound(activator, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
	}
	self->activator = activator;
	multi_trigger(self);
}

void SP_trigger_counter(edict_t *self)
{
	self->wait = -1;
	if (!self->count)
		self->count = 2;

	self->use = trigger_counter_use;
}

/*
==============================================================================

trigger_always

==============================================================================
*/

/*QUAKED trigger_always (.5 .5 .5) (-8 -8 -8) (8 8 8)
This trigger will always fire.  It is activated by the world.
*/
void SP_trigger_always(edict_t *ent)
{
	// we must have some delay to make sure our use targets are present
	if (!ent->delay)
		ent->delay = 0.2f;
	G_UseTargets(ent, ent);
}

/*
==============================================================================

trigger_push

==============================================================================
*/

// PGM
constexpr spawnflags_t SPAWNFLAG_PUSH_ONCE = 0x01_spawnflag;
constexpr spawnflags_t SPAWNFLAG_PUSH_PLUS = 0x02_spawnflag;
constexpr spawnflags_t SPAWNFLAG_PUSH_SILENT = 0x04_spawnflag;
constexpr spawnflags_t SPAWNFLAG_PUSH_START_OFF = 0x08_spawnflag;
constexpr spawnflags_t SPAWNFLAG_PUSH_CLIP = 0x10_spawnflag;
// PGM

static cached_soundindex windsound;

TOUCH(trigger_push_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	if (self->spawnflags.has(SPAWNFLAG_PUSH_CLIP))
	{
		trace_t clip = gi.clip(self, other->s.origin, other->mins, other->maxs, other->s.origin, G_GetClipMask(other));

		if (clip.fraction == 1.0f)
			return;
	}

	if (strcmp(other->classname, "grenade") == 0)
	{
		other->velocity = self->movedir * (self->speed * 10);
	}
	else if (other->health > 0)
	{
		other->velocity = self->movedir * (self->speed * 10);

		if (other->client)
		{
			// don't take falling damage immediately from this
			other->client->oldvelocity = other->velocity;
			other->client->oldgroundentity = other->groundentity;
			if (
				!(self->spawnflags & SPAWNFLAG_PUSH_SILENT) &&
				(other->fly_sound_debounce_time < level.time))
			{
				other->fly_sound_debounce_time = level.time + 1.5_sec;
				gi.sound(other, CHAN_AUTO, windsound, 1, ATTN_NORM, 0);
			}
		}
	}

	if (self->spawnflags.has(SPAWNFLAG_PUSH_ONCE))
		G_FreeEdict(self);
}

//======
// PGM
USE(trigger_push_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	if (self->solid == SOLID_NOT)
		self->solid = SOLID_TRIGGER;
	else
		self->solid = SOLID_NOT;
	gi.linkentity(self);
}
// PGM
//======

// RAFAEL
void trigger_push_active(edict_t *self);

void trigger_effect(edict_t *self)
{
	vec3_t origin;
	int	   i;

	origin = (self->absmin + self->absmax) * 0.5f;

	for (i = 0; i < 10; i++)
	{
		origin[2] += (self->speed * 0.01f) * (i + frandom());
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_TUNNEL_SPARKS);
		gi.WriteByte(1);
		gi.WritePosition(origin);
		gi.WriteDir(vec3_origin);
		gi.WriteByte(irandom(0x74, 0x7C));
		gi.multicast(self->s.origin, MULTICAST_PVS, false);
	}
}

THINK(trigger_push_inactive) (edict_t *self) -> void
{
	if (self->delay > level.time.seconds())
	{
		self->nextthink = level.time + 100_ms;
	}
	else
	{
		self->touch = trigger_push_touch;
		self->think = trigger_push_active;
		self->nextthink = level.time + 100_ms;
		self->delay = (self->nextthink + gtime_t::from_sec(self->wait)).seconds();
	}
}

THINK(trigger_push_active) (edict_t *self) -> void
{
	if (self->delay > level.time.seconds())
	{
		self->nextthink = level.time + 100_ms;
		trigger_effect(self);
	}
	else
	{
		self->touch = nullptr;
		self->think = trigger_push_inactive;
		self->nextthink = level.time + 100_ms;
		self->delay = (self->nextthink + gtime_t::from_sec(self->wait)).seconds();
	}
}
// RAFAEL

/*QUAKED trigger_push (.5 .5 .5) ? PUSH_ONCE PUSH_PLUS PUSH_SILENT START_OFF CLIP
Pushes the player
"speed"	defaults to 1000
"wait"  defaults to 10, must use PUSH_PLUS

If targeted, it will toggle on and off when used.

START_OFF - toggled trigger_push begins in off setting
SILENT - doesn't make wind noise
*/
void SP_trigger_push(edict_t *self)
{
	InitTrigger(self);
	if (!(self->spawnflags & SPAWNFLAG_PUSH_SILENT))
		windsound.assign("misc/windfly.wav");
	self->touch = trigger_push_touch;

	// RAFAEL
	if (self->spawnflags.has(SPAWNFLAG_PUSH_PLUS))
	{
		if (!self->wait)
			self->wait = 10;

		self->think = trigger_push_active;
		self->nextthink = level.time + 100_ms;
		self->delay = (self->nextthink + gtime_t::from_sec(self->wait)).seconds();
	}
	// RAFAEL

	if (!self->speed)
		self->speed = 1000;

	// PGM
	if (self->targetname) // toggleable
	{
		self->use = trigger_push_use;
		if (self->spawnflags.has(SPAWNFLAG_PUSH_START_OFF))
			self->solid = SOLID_NOT;
	}
	else if (self->spawnflags.has(SPAWNFLAG_PUSH_START_OFF))
	{
		gi.Com_Print("trigger_push is START_OFF but not targeted.\n");
		self->svflags = SVF_NONE;
		self->touch = nullptr;
		self->solid = SOLID_BSP;
		self->movetype = MOVETYPE_PUSH;
	}
	// PGM

	gi.linkentity(self);

	if (self->spawnflags.has(SPAWNFLAG_PUSH_CLIP))
		self->svflags |= SVF_HULL;
}

/*
==============================================================================

trigger_hurt

==============================================================================
*/

/*QUAKED trigger_hurt (.5 .5 .5) ? START_OFF TOGGLE SILENT NO_PROTECTION SLOW NO_PLAYERS NO_MONSTERS
Any entity that touches this will be hurt.

It does dmg points of damage each server frame

SILENT			supresses playing the sound
SLOW			changes the damage rate to once per second
NO_PROTECTION	*nothing* stops the damage

"dmg"			default 5 (whole numbers only)

*/

constexpr spawnflags_t SPAWNFLAG_HURT_START_OFF = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAG_HURT_TOGGLE = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAG_HURT_SILENT = 4_spawnflag;
constexpr spawnflags_t SPAWNFLAG_HURT_NO_PROTECTION = 8_spawnflag;
constexpr spawnflags_t SPAWNFLAG_HURT_SLOW = 16_spawnflag;
constexpr spawnflags_t SPAWNFLAG_HURT_NO_PLAYERS = 32_spawnflag;
constexpr spawnflags_t SPAWNFLAG_HURT_NO_MONSTERS = 64_spawnflag;
constexpr spawnflags_t SPAWNFLAG_HURT_CLIPPED = 128_spawnflag;

USE(hurt_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	if (self->solid == SOLID_NOT)
		self->solid = SOLID_TRIGGER;
	else
		self->solid = SOLID_NOT;
	gi.linkentity(self);

	if (!(self->spawnflags & SPAWNFLAG_HURT_TOGGLE))
		self->use = nullptr;
}

TOUCH(hurt_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	damageflags_t dflags;

	if (!other->takedamage)
		return;
	else if (!(other->svflags & SVF_MONSTER) && !(other->flags & FL_DAMAGEABLE) && (!other->client) && (strcmp(other->classname, "misc_explobox") != 0))
		return;
	else if (self->spawnflags.has(SPAWNFLAG_HURT_NO_MONSTERS) && (other->svflags & SVF_MONSTER))
		return;
	else if (self->spawnflags.has(SPAWNFLAG_HURT_NO_PLAYERS) && (other->client))
		return;

	if (self->timestamp > level.time)
		return;

	if (self->spawnflags.has(SPAWNFLAG_HURT_CLIPPED))
	{
		trace_t clip = gi.clip(self, other->s.origin, other->mins, other->maxs, other->s.origin, G_GetClipMask(other));

		if (clip.fraction == 1.0f)
			return;
	}

	if (self->spawnflags.has(SPAWNFLAG_HURT_SLOW))
		self->timestamp = level.time + 1_sec;
	else
		self->timestamp = level.time + 10_hz;

	if (!(self->spawnflags & SPAWNFLAG_HURT_SILENT))
	{
		if (self->fly_sound_debounce_time < level.time)
		{
			gi.sound(other, CHAN_AUTO, self->noise_index, 1, ATTN_NORM, 0);
			self->fly_sound_debounce_time = level.time + 1_sec;
		}
	}

	if (self->spawnflags.has(SPAWNFLAG_HURT_NO_PROTECTION))
		dflags = DAMAGE_NO_PROTECTION;
	else
		dflags = DAMAGE_NONE;

	T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, self->dmg, dflags, MOD_TRIGGER_HURT);
}

void SP_trigger_hurt(edict_t *self)
{
	InitTrigger(self);

	self->noise_index = gi.soundindex("world/electro.wav");
	self->touch = hurt_touch;

	if (!self->dmg)
		self->dmg = 5;

	if (self->spawnflags.has(SPAWNFLAG_HURT_START_OFF))
		self->solid = SOLID_NOT;
	else
		self->solid = SOLID_TRIGGER;

	if (self->spawnflags.has(SPAWNFLAG_HURT_TOGGLE))
		self->use = hurt_use;

	gi.linkentity(self);

	if (self->spawnflags.has(SPAWNFLAG_HURT_CLIPPED))
		self->svflags |= SVF_HULL;
}

/*
==============================================================================

trigger_gravity

==============================================================================
*/

/*QUAKED trigger_gravity (.5 .5 .5) ? TOGGLE START_OFF
Changes the touching entites gravity to the value of "gravity".
1.0 is standard gravity for the level.

TOGGLE - trigger_gravity can be turned on and off
START_OFF - trigger_gravity starts turned off (implies TOGGLE)
*/

constexpr spawnflags_t SPAWNFLAG_GRAVITY_TOGGLE = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAG_GRAVITY_START_OFF = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAG_GRAVITY_CLIPPED = 4_spawnflag;

// PGM
USE(trigger_gravity_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	if (self->solid == SOLID_NOT)
		self->solid = SOLID_TRIGGER;
	else
		self->solid = SOLID_NOT;
	gi.linkentity(self);
}
// PGM

TOUCH(trigger_gravity_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	if (self->spawnflags.has(SPAWNFLAG_GRAVITY_CLIPPED))
	{
		trace_t clip = gi.clip(self, other->s.origin, other->mins, other->maxs, other->s.origin, G_GetClipMask(other));

		if (clip.fraction == 1.0f)
			return;
	}

	other->gravity = self->gravity;
}

void SP_trigger_gravity(edict_t *self)
{
	if (!st.gravity || !*st.gravity)
	{
		gi.Com_PrintFmt("{}: no gravity set\n", *self);
		G_FreeEdict(self);
		return;
	}

	InitTrigger(self);

	// PGM
	self->gravity = (float) atof(st.gravity);

	if (self->spawnflags.has(SPAWNFLAG_GRAVITY_TOGGLE))
		self->use = trigger_gravity_use;

	if (self->spawnflags.has(SPAWNFLAG_GRAVITY_START_OFF))
	{
		self->use = trigger_gravity_use;
		self->solid = SOLID_NOT;
	}

	self->touch = trigger_gravity_touch;
	// PGM

	gi.linkentity(self);

	if (self->spawnflags.has(SPAWNFLAG_GRAVITY_CLIPPED))
		self->svflags |= SVF_HULL;
}

/*
==============================================================================

trigger_monsterjump

==============================================================================
*/

/*QUAKED trigger_monsterjump (.5 .5 .5) ?
Walking monsters that touch this will jump in the direction of the trigger's angle
"speed" default to 200, the speed thrown forward
"height" default to 200, the speed thrown upwards

TOGGLE - trigger_monsterjump can be turned on and off
START_OFF - trigger_monsterjump starts turned off (implies TOGGLE)
*/

constexpr spawnflags_t SPAWNFLAG_MONSTERJUMP_TOGGLE = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAG_MONSTERJUMP_START_OFF = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAG_MONSTERJUMP_CLIPPED = 4_spawnflag;

USE(trigger_monsterjump_use) (edict_t* self, edict_t* other, edict_t* activator) -> void
{
	if (self->solid == SOLID_NOT)
		self->solid = SOLID_TRIGGER;
	else
		self->solid = SOLID_NOT;
	gi.linkentity(self);
}

TOUCH(trigger_monsterjump_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	if (other->flags & (FL_FLY | FL_SWIM))
		return;
	if (other->svflags & SVF_DEADMONSTER)
		return;
	if (!(other->svflags & SVF_MONSTER))
		return;

	if (self->spawnflags.has(SPAWNFLAG_MONSTERJUMP_CLIPPED))
	{
		trace_t clip = gi.clip(self, other->s.origin, other->mins, other->maxs, other->s.origin, G_GetClipMask(other));

		if (clip.fraction == 1.0f)
			return;
	}

	// set XY even if not on ground, so the jump will clear lips
	other->velocity[0] = self->movedir[0] * self->speed;
	other->velocity[1] = self->movedir[1] * self->speed;

	if (!other->groundentity)
		return;

	other->groundentity = nullptr;
	other->velocity[2] = self->movedir[2];
}

void SP_trigger_monsterjump(edict_t *self)
{
	if (!self->speed)
		self->speed = 200;
	if (!st.height)
		st.height = 200;
	if (self->s.angles[YAW] == 0)
		self->s.angles[YAW] = 360;
	InitTrigger(self);
	self->touch = trigger_monsterjump_touch;
	self->movedir[2] = (float) st.height;

	if (self->spawnflags.has(SPAWNFLAG_MONSTERJUMP_TOGGLE))
		self->use = trigger_monsterjump_use;

	if (self->spawnflags.has(SPAWNFLAG_MONSTERJUMP_START_OFF))
	{
		self->use = trigger_monsterjump_use;
		self->solid = SOLID_NOT;
	}

	gi.linkentity(self);

	if (self->spawnflags.has(SPAWNFLAG_MONSTERJUMP_CLIPPED))
		self->svflags |= SVF_HULL;
}

/*
==============================================================================

trigger_flashlight

==============================================================================
*/

/*QUAKED trigger_flashlight (.5 .5 .5) ?
Players moving against this trigger will have their flashlight turned on or off.
"style" default to 0, set to 1 to always turn flashlight on, 2 to always turn off,
        otherwise "angles" are used to control on/off state
*/

constexpr spawnflags_t SPAWNFLAG_FLASHLIGHT_CLIPPED = 1_spawnflag;

TOUCH(trigger_flashlight_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	if (!other->client)
		return;

	if (self->spawnflags.has(SPAWNFLAG_FLASHLIGHT_CLIPPED))
	{
		trace_t clip = gi.clip(self, other->s.origin, other->mins, other->maxs, other->s.origin, G_GetClipMask(other));

		if (clip.fraction == 1.0f)
			return;
	}

	if (self->style == 1)
	{
		P_ToggleFlashlight(other, true);
	}
	else if (self->style == 2)
	{
		P_ToggleFlashlight(other, false);
	}
	else if (other->velocity.lengthSquared() > 32.f)
	{
		vec3_t forward = other->velocity.normalized();
		P_ToggleFlashlight(other, forward.dot(self->movedir) > 0);
	}
}

void SP_trigger_flashlight(edict_t *self)
{
	if (self->s.angles[YAW] == 0)
		self->s.angles[YAW] = 360;
	InitTrigger(self);
	self->touch = trigger_flashlight_touch;
	self->movedir[2] = (float) st.height;

	if (self->spawnflags.has(SPAWNFLAG_FLASHLIGHT_CLIPPED))
		self->svflags |= SVF_HULL;
	gi.linkentity(self);
}


/*
==============================================================================

trigger_fog

==============================================================================
*/

/*QUAKED trigger_fog (.5 .5 .5) ? AFFECT_FOG AFFECT_HEIGHTFOG INSTANTANEOUS FORCE BLEND
Players moving against this trigger will have their fog settings changed.
Fog/heightfog will be adjusted if the spawnflags are set. Instantaneous
ignores any delays. Force causes it to ignore movement dir and always use
the "on" values. Blend causes it to change towards how far you are into the trigger
with respect to angles.
"target" can target an info_notnull to pull the keys below from.
"delay" default to 0.5; time in seconds a change in fog will occur over
"wait" default to 0.0; time in seconds before a re-trigger can be executed

"fog_density"; density value of fog, 0-1
"fog_color"; color value of fog, 3d vector with values between 0-1 (r g b)
"fog_density_off"; transition density value of fog, 0-1
"fog_color_off"; transition color value of fog, 3d vector with values between 0-1 (r g b)
"fog_sky_factor"; sky factor value of fog, 0-1
"fog_sky_factor_off"; transition sky factor value of fog, 0-1

"heightfog_falloff"; falloff value of heightfog, 0-1
"heightfog_density"; density value of heightfog, 0-1
"heightfog_start_color"; the start color for the fog (r g b, 0-1)
"heightfog_start_dist"; the start distance for the fog (units)
"heightfog_end_color"; the start color for the fog (r g b, 0-1)
"heightfog_end_dist"; the end distance for the fog (units)

"heightfog_falloff_off"; transition falloff value of heightfog, 0-1
"heightfog_density_off"; transition density value of heightfog, 0-1
"heightfog_start_color_off"; transition the start color for the fog (r g b, 0-1)
"heightfog_start_dist_off"; transition the start distance for the fog (units)
"heightfog_end_color_off"; transition the start color for the fog (r g b, 0-1)
"heightfog_end_dist_off"; transition the end distance for the fog (units)
*/

constexpr spawnflags_t SPAWNFLAG_FOG_AFFECT_FOG = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAG_FOG_AFFECT_HEIGHTFOG = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAG_FOG_INSTANTANEOUS = 4_spawnflag;
constexpr spawnflags_t SPAWNFLAG_FOG_FORCE = 8_spawnflag;
constexpr spawnflags_t SPAWNFLAG_FOG_BLEND = 16_spawnflag;

TOUCH(trigger_fog_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	if (!other->client)
		return;

	if (self->timestamp > level.time)
		return;

	self->timestamp = level.time + gtime_t::from_sec(self->wait);

	edict_t *fog_value_storage = self;

	if (self->movetarget)
		fog_value_storage = self->movetarget;

	if (self->spawnflags.has(SPAWNFLAG_FOG_INSTANTANEOUS))
		other->client->pers.fog_transition_time = 0_ms;
	else
		other->client->pers.fog_transition_time = gtime_t::from_sec(fog_value_storage->delay);

	if (self->spawnflags.has(SPAWNFLAG_FOG_BLEND))
	{
		vec3_t center = (self->absmin + self->absmax) * 0.5f;
		vec3_t half_size = (self->size * 0.5f) + (other->size * 0.5f);
		vec3_t start = (-self->movedir).scaled(half_size);
		vec3_t end = (self->movedir).scaled(half_size);
		vec3_t player_dist = (other->s.origin - center).scaled(vec3_t{fabs(self->movedir[0]),fabs(self->movedir[1]),fabs(self->movedir[2])});

		float dist = (player_dist - start).length();
		dist /= (start - end).length();
		dist = clamp(dist, 0.f, 1.f);

		if (self->spawnflags.has(SPAWNFLAG_FOG_AFFECT_FOG))
		{
			other->client->pers.wanted_fog = {
				lerp(fog_value_storage->fog.density_off, fog_value_storage->fog.density, dist),
				lerp(fog_value_storage->fog.color_off[0], fog_value_storage->fog.color[0], dist),
				lerp(fog_value_storage->fog.color_off[1], fog_value_storage->fog.color[1], dist),
				lerp(fog_value_storage->fog.color_off[2], fog_value_storage->fog.color[2], dist),
				lerp(fog_value_storage->fog.sky_factor_off, fog_value_storage->fog.sky_factor, dist)
			};
		}

		if (self->spawnflags.has(SPAWNFLAG_FOG_AFFECT_HEIGHTFOG))
		{
			other->client->pers.wanted_heightfog = {
				{
					lerp(fog_value_storage->heightfog.start_color_off[0], fog_value_storage->heightfog.start_color[0], dist),
					lerp(fog_value_storage->heightfog.start_color_off[1], fog_value_storage->heightfog.start_color[1], dist),
					lerp(fog_value_storage->heightfog.start_color_off[2], fog_value_storage->heightfog.start_color[2], dist),
					lerp(fog_value_storage->heightfog.start_dist_off, fog_value_storage->heightfog.start_dist, dist)
				},
			{
				lerp(fog_value_storage->heightfog.end_color_off[0], fog_value_storage->heightfog.end_color[0], dist),
				lerp(fog_value_storage->heightfog.end_color_off[1], fog_value_storage->heightfog.end_color[1], dist),
				lerp(fog_value_storage->heightfog.end_color_off[2], fog_value_storage->heightfog.end_color[2], dist),
				lerp(fog_value_storage->heightfog.end_dist_off, fog_value_storage->heightfog.end_dist, dist)
			},
				lerp(fog_value_storage->heightfog.falloff_off, fog_value_storage->heightfog.falloff, dist),
				lerp(fog_value_storage->heightfog.density_off, fog_value_storage->heightfog.density, dist)
			};
		}

		return;
	}

	bool use_on = true;

	if (!self->spawnflags.has(SPAWNFLAG_FOG_FORCE))
	{
		float len;
		vec3_t forward = other->velocity.normalized(len);

		// not moving enough to trip; this is so we don't trip
		// the wrong direction when on an elevator, etc.
		if (len <= 0.0001f)
			return;

		use_on = forward.dot(self->movedir) > 0;
	}

	if (self->spawnflags.has(SPAWNFLAG_FOG_AFFECT_FOG))
	{
		if (use_on)
		{
			other->client->pers.wanted_fog = {
				fog_value_storage->fog.density,
				fog_value_storage->fog.color[0],
				fog_value_storage->fog.color[1],
				fog_value_storage->fog.color[2],
				fog_value_storage->fog.sky_factor
			};
		}
		else
		{
			other->client->pers.wanted_fog = {
				fog_value_storage->fog.density_off,
				fog_value_storage->fog.color_off[0],
				fog_value_storage->fog.color_off[1],
				fog_value_storage->fog.color_off[2],
				fog_value_storage->fog.sky_factor_off
			};
		}
	}

	if (self->spawnflags.has(SPAWNFLAG_FOG_AFFECT_HEIGHTFOG))
	{
		if (use_on)
		{
			other->client->pers.wanted_heightfog = {
				{
					fog_value_storage->heightfog.start_color[0],
					fog_value_storage->heightfog.start_color[1],
					fog_value_storage->heightfog.start_color[2],
					fog_value_storage->heightfog.start_dist
				},
				{
					fog_value_storage->heightfog.end_color[0],
					fog_value_storage->heightfog.end_color[1],
					fog_value_storage->heightfog.end_color[2],
					fog_value_storage->heightfog.end_dist
				},
				fog_value_storage->heightfog.falloff,
				fog_value_storage->heightfog.density
			};
		}
		else
		{
			other->client->pers.wanted_heightfog = {
				{
					fog_value_storage->heightfog.start_color_off[0],
					fog_value_storage->heightfog.start_color_off[1],
					fog_value_storage->heightfog.start_color_off[2],
					fog_value_storage->heightfog.start_dist_off
				},
				{
					fog_value_storage->heightfog.end_color_off[0],
					fog_value_storage->heightfog.end_color_off[1],
					fog_value_storage->heightfog.end_color_off[2],
					fog_value_storage->heightfog.end_dist_off
				},
				fog_value_storage->heightfog.falloff_off,
				fog_value_storage->heightfog.density_off
			};
		}
	}
}

void SP_trigger_fog(edict_t *self)
{
	if (self->s.angles[YAW] == 0)
		self->s.angles[YAW] = 360;

	InitTrigger(self);

	if (!(self->spawnflags & (SPAWNFLAG_FOG_AFFECT_FOG | SPAWNFLAG_FOG_AFFECT_HEIGHTFOG)))
		gi.Com_PrintFmt("WARNING: {} with no fog spawnflags set\n", *self);

	if (self->target)
	{
		self->movetarget = G_PickTarget(self->target);

		if (self->movetarget)
		{
			if (!self->movetarget->delay)
				self->movetarget->delay = 0.5f;
		}
	}

	if (!self->delay)
		self->delay = 0.5f;

	self->touch = trigger_fog_touch;
}

/*QUAKED trigger_coop_relay (.5 .5 .5) ? AUTO_FIRE
Like a trigger_relay, but all players must be touching its
mins/maxs in order to fire, otherwise a message will be printed.

AUTO_FIRE: check every `wait` seconds for containment instead of
requiring to be fired by something else. Frees itself after firing.

"message"; message to print to the one activating the relay if
           not all players are inside the bounds
"message2"; message to print to players not inside the trigger
            if they aren't in the bounds
*/

constexpr spawnflags_t SPAWNFLAG_COOP_RELAY_AUTO_FIRE = 1_spawnflag;

inline bool trigger_coop_relay_filter(edict_t *player)
{
	return (player->health <= 0 || player->deadflag || player->movetype == MOVETYPE_NOCLIP ||
		player->client->resp.spectator || player->s.modelindex != MODELINDEX_PLAYER);
}

static bool trigger_coop_relay_can_use(edict_t *self, edict_t *activator)
{
	// not coop, so act like a standard trigger_relay minus the message
	if (!coop->integer)
		return true;

	// coop; scan for all alive players, print appropriate message
	// to those in/out of range
	bool can_use = true;

	for (auto player : active_players())
	{
		// dead or spectator, don't count them
		if (trigger_coop_relay_filter(player))
			continue;

		if (boxes_intersect(player->absmin, player->absmax, self->absmin, self->absmax))
			continue;

		if (self->timestamp < level.time)
			gi.LocCenter_Print(player, self->map);
		can_use = false;
	}

	return can_use;
}

USE(trigger_coop_relay_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	if (!trigger_coop_relay_can_use(self, activator))
	{
		if (self->timestamp < level.time)
			gi.LocCenter_Print(activator, self->message);

		self->timestamp = level.time + 5_sec;
		return;
	}

	const char *msg = self->message;
	self->message = nullptr;
	G_UseTargets(self, activator);
	self->message = msg;
}

static BoxEdictsResult_t trigger_coop_relay_player_filter(edict_t *ent, void *data)
{
	if (!ent->client)
		return BoxEdictsResult_t::Skip;
	else if (trigger_coop_relay_filter(ent))
		return BoxEdictsResult_t::Skip;

	return BoxEdictsResult_t::Keep;
}

THINK(trigger_coop_relay_think) (edict_t *self) -> void
{
	std::array<edict_t *, MAX_SPLIT_PLAYERS> players;
	size_t num_active = 0;

	for (auto player : active_players())
		if (!trigger_coop_relay_filter(player))
			num_active++;

	size_t n = gi.BoxEdicts(self->absmin, self->absmax, players.data(), num_active, AREA_SOLID, trigger_coop_relay_player_filter, nullptr);

	if (n == num_active)
	{
		const char *msg = self->message;
		self->message = nullptr;
		G_UseTargets(self, &globals.edicts[1]);
		self->message = msg;

		G_FreeEdict(self);
		return;
	}
	else if (n && self->timestamp < level.time)
	{
		for (size_t i = 0; i < n; i++)
			gi.LocCenter_Print(players[i], self->message);

		for (auto player : active_players())
			if (std::find(players.begin(), players.end(), player) == players.end())
				gi.LocCenter_Print(player, self->map);

		self->timestamp = level.time + 5_sec;
	}

	self->nextthink = level.time + gtime_t::from_sec(self->wait);
}

void SP_trigger_coop_relay(edict_t *self)
{
	if (self->targetname && self->spawnflags.has(SPAWNFLAG_COOP_RELAY_AUTO_FIRE))
		gi.Com_PrintFmt("{}: targetname and auto-fire are mutually exclusive\n", *self);

	InitTrigger(self);
	
	if (!self->message)
		self->message = "$g_coop_wait_for_players";

	if (!self->map)
		self->map = "$g_coop_players_waiting_for_you";

	if (!self->wait)
		self->wait = 1;

	if (self->spawnflags.has(SPAWNFLAG_COOP_RELAY_AUTO_FIRE))
	{
		self->think = trigger_coop_relay_think;
		self->nextthink = level.time + gtime_t::from_sec(self->wait);
	}
	else
		self->use = trigger_coop_relay_use;
	self->svflags |= SVF_NOCLIENT;
	gi.linkentity(self);
}