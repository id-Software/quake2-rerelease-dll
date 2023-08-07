// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "../g_local.h"

//====
// PGM
constexpr spawnflags_t SPAWNFLAGS_PLAT2_TOGGLE = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_PLAT2_TOP = 4_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_PLAT2_START_ACTIVE = 8_spawnflag;
constexpr spawnflags_t SPAWNFLAGS_PLAT2_BOX_LIFT = 32_spawnflag;
// PGM
//====

void plat2_go_down(edict_t *ent);
void plat2_go_up(edict_t *ent);

void plat2_spawn_danger_area(edict_t *ent)
{
	vec3_t mins, maxs;

	mins = ent->mins;
	maxs = ent->maxs;
	maxs[2] = ent->mins[2] + 64;

	SpawnBadArea(mins, maxs, 0_ms, ent);
}

void plat2_kill_danger_area(edict_t *ent)
{
	edict_t *t;

	t = nullptr;
	while ((t = G_FindByString<&edict_t::classname>(t, "bad_area")))
	{
		if (t->owner == ent)
			G_FreeEdict(t);
	}
}

MOVEINFO_ENDFUNC(plat2_hit_top) (edict_t *ent) -> void
{
	if (!(ent->flags & FL_TEAMSLAVE))
	{
		if (ent->moveinfo.sound_end)
			gi.sound(ent, CHAN_NO_PHS_ADD | CHAN_VOICE, ent->moveinfo.sound_end, 1, ATTN_STATIC, 0);
	}
	ent->s.sound = 0;
	ent->moveinfo.state = STATE_TOP;

	if (ent->plat2flags & PLAT2_CALLED)
	{
		ent->plat2flags = PLAT2_WAITING;
		if (!ent->spawnflags.has(SPAWNFLAGS_PLAT2_TOGGLE))
		{
			ent->think = plat2_go_down;
			ent->nextthink = level.time + 5_sec;
		}
		if (deathmatch->integer)
			ent->last_move_time = level.time - 1_sec;
		else
			ent->last_move_time = level.time - 2_sec;
	}
	else if (!(ent->spawnflags & SPAWNFLAGS_PLAT2_TOP) && !ent->spawnflags.has(SPAWNFLAGS_PLAT2_TOGGLE))
	{
		ent->plat2flags = PLAT2_NONE;
		ent->think = plat2_go_down;
		ent->nextthink = level.time + 2_sec;
		ent->last_move_time = level.time;
	}
	else
	{
		ent->plat2flags = PLAT2_NONE;
		ent->last_move_time = level.time;
	}

	G_UseTargets(ent, ent);
}

MOVEINFO_ENDFUNC(plat2_hit_bottom) (edict_t *ent) -> void
{
	if (!(ent->flags & FL_TEAMSLAVE))
	{
		if (ent->moveinfo.sound_end)
			gi.sound(ent, CHAN_NO_PHS_ADD | CHAN_VOICE, ent->moveinfo.sound_end, 1, ATTN_STATIC, 0);
	}
	ent->s.sound = 0;
	ent->moveinfo.state = STATE_BOTTOM;

	if (ent->plat2flags & PLAT2_CALLED)
	{
		ent->plat2flags = PLAT2_WAITING;
		if (!(ent->spawnflags & SPAWNFLAGS_PLAT2_TOGGLE))
		{
			ent->think = plat2_go_up;
			ent->nextthink = level.time + 5_sec;
		}
		if (deathmatch->integer)
			ent->last_move_time = level.time - 1_sec;
		else
			ent->last_move_time = level.time - 2_sec;
	}
	else if (ent->spawnflags.has(SPAWNFLAGS_PLAT2_TOP) && !ent->spawnflags.has(SPAWNFLAGS_PLAT2_TOGGLE))
	{
		ent->plat2flags = PLAT2_NONE;
		ent->think = plat2_go_up;
		ent->nextthink = level.time + 2_sec;
		ent->last_move_time = level.time;
	}
	else
	{
		ent->plat2flags = PLAT2_NONE;
		ent->last_move_time = level.time;
	}

	plat2_kill_danger_area(ent);
	G_UseTargets(ent, ent);
}

THINK(plat2_go_down) (edict_t *ent) -> void
{
	if (!(ent->flags & FL_TEAMSLAVE))
	{
		if (ent->moveinfo.sound_start)
			gi.sound(ent, CHAN_NO_PHS_ADD | CHAN_VOICE, ent->moveinfo.sound_start, 1, ATTN_STATIC, 0);
	}

	ent->s.sound = ent->moveinfo.sound_middle;

	ent->moveinfo.state = STATE_DOWN;
	ent->plat2flags |= PLAT2_MOVING;

	Move_Calc(ent, ent->moveinfo.end_origin, plat2_hit_bottom);
}

THINK(plat2_go_up) (edict_t *ent) -> void
{
	if (!(ent->flags & FL_TEAMSLAVE))
	{
		if (ent->moveinfo.sound_start)
			gi.sound(ent, CHAN_NO_PHS_ADD | CHAN_VOICE, ent->moveinfo.sound_start, 1, ATTN_STATIC, 0);
	}

	ent->s.sound = ent->moveinfo.sound_middle;

	ent->moveinfo.state = STATE_UP;
	ent->plat2flags |= PLAT2_MOVING;

	plat2_spawn_danger_area(ent);

	Move_Calc(ent, ent->moveinfo.start_origin, plat2_hit_top);
}

void plat2_operate(edict_t *ent, edict_t *other)
{
	int		 otherState;
	gtime_t	 pauseTime;
	float	 platCenter;
	edict_t *trigger;

	trigger = ent;
	ent = ent->enemy; // now point at the plat, not the trigger

	if (ent->plat2flags & PLAT2_MOVING)
		return;

	if ((ent->last_move_time + 2_sec) > level.time)
		return;

	platCenter = (trigger->absmin[2] + trigger->absmax[2]) / 2;

	if (ent->moveinfo.state == STATE_TOP)
	{
		otherState = STATE_TOP;
		if (ent->spawnflags.has(SPAWNFLAGS_PLAT2_BOX_LIFT))
		{
			if (platCenter > other->s.origin[2])
				otherState = STATE_BOTTOM;
		}
		else
		{
			if (trigger->absmax[2] > other->s.origin[2])
				otherState = STATE_BOTTOM;
		}
	}
	else
	{
		otherState = STATE_BOTTOM;
		if (other->s.origin[2] > platCenter)
			otherState = STATE_TOP;
	}

	ent->plat2flags = PLAT2_MOVING;

	if (deathmatch->integer)
		pauseTime = 300_ms;
	else
		pauseTime = 500_ms;

	if (ent->moveinfo.state != otherState)
	{
		ent->plat2flags |= PLAT2_CALLED;
		pauseTime = 100_ms;
	}

	ent->last_move_time = level.time;

	if (ent->moveinfo.state == STATE_BOTTOM)
	{
		ent->think = plat2_go_up;
		ent->nextthink = level.time + pauseTime;
	}
	else
	{
		ent->think = plat2_go_down;
		ent->nextthink = level.time + pauseTime;
	}
}

TOUCH(Touch_Plat_Center2) (edict_t *ent, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	// this requires monsters to actively trigger plats, not just step on them.

	// FIXME - commented out for E3
	// if (!other->client)
	//	return;

	if (other->health <= 0)
		return;

	// PMM - don't let non-monsters activate plat2s
	if ((!(other->svflags & SVF_MONSTER)) && (!other->client))
		return;

	plat2_operate(ent, other);
}

MOVEINFO_BLOCKED(plat2_blocked) (edict_t *self, edict_t *other) -> void
{
	if (!(other->svflags & SVF_MONSTER) && (!other->client))
	{
		// give it a chance to go away on it's own terms (like gibs)
		T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, DAMAGE_NONE, MOD_CRUSH);
		// if it's still there, nuke it
		if (other && other->inuse && other->solid)
			BecomeExplosion1(other);
		return;
	}

	// gib dead things
	if (other->health < 1)
	{
		T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 100, 1, DAMAGE_NONE, MOD_CRUSH);
	}

	T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, DAMAGE_NONE, MOD_CRUSH);

	// [Paril-KEX] killed, so don't change direction
	if (!other->inuse || !other->solid)
		return;

	if (self->moveinfo.state == STATE_UP)
		plat2_go_down(self);
	else if (self->moveinfo.state == STATE_DOWN)
		plat2_go_up(self);
}

USE(Use_Plat2) (edict_t *ent, edict_t *other, edict_t *activator) -> void
{
	edict_t *trigger;

	if (ent->moveinfo.state > STATE_BOTTOM)
		return;
	// [Paril-KEX] disabled this; causes confusing situations
	//if ((ent->last_move_time + 2_sec) > level.time)
	//	return;

	uint32_t i;
	for (i = 1, trigger = g_edicts + 1; i < globals.num_edicts; i++, trigger++)
	{
		if (!trigger->inuse)
			continue;
		if (trigger->touch == Touch_Plat_Center2)
		{
			if (trigger->enemy == ent)
			{
				//				Touch_Plat_Center2 (trigger, activator, nullptr, nullptr);
				plat2_operate(trigger, activator);
				return;
			}
		}
	}
}

USE(plat2_activate) (edict_t *ent, edict_t *other, edict_t *activator) -> void
{
	edict_t *trigger;

	//	if(ent->targetname)
	//		ent->targetname[0] = 0;

	ent->use = Use_Plat2;

	trigger = plat_spawn_inside_trigger(ent); // the "start moving" trigger

	trigger->maxs[0] += 10;
	trigger->maxs[1] += 10;
	trigger->mins[0] -= 10;
	trigger->mins[1] -= 10;

	gi.linkentity(trigger);

	trigger->touch = Touch_Plat_Center2; // Override trigger touch function

	plat2_go_down(ent);
}

/*QUAKED func_plat2 (0 .5 .8) ? PLAT_LOW_TRIGGER PLAT2_TOGGLE PLAT2_TOP PLAT2_START_ACTIVE UNUSED BOX_LIFT
speed	default 150

PLAT_LOW_TRIGGER - creates a short trigger field at the bottom
PLAT2_TOGGLE - plat will not return to default position.
PLAT2_TOP - plat's default position will the the top.
PLAT2_START_ACTIVE - plat will trigger it's targets each time it hits top
UNUSED
BOX_LIFT - this indicates that the lift is a box, rather than just a platform

Plats are always drawn in the extended position, so they will light correctly.

If the plat is the target of another trigger or button, it will start out disabled in the extended position until it is trigger, when it will lower and become a normal plat.

"speed"	overrides default 200.
"accel" overrides default 500
"lip"	no default

If the "height" key is set, that will determine the amount the plat moves, instead of being implicitly determoveinfoned by the model's height.

*/
void SP_func_plat2(edict_t *ent)
{
	edict_t *trigger;

	ent->s.angles = {};
	ent->solid = SOLID_BSP;
	ent->movetype = MOVETYPE_PUSH;

	gi.setmodel(ent, ent->model);

	ent->moveinfo.blocked = plat2_blocked;

	if (!ent->speed)
		ent->speed = 20;
	else
		ent->speed *= 0.1f;

	if (!ent->accel)
		ent->accel = 5;
	else
		ent->accel *= 0.1f;

	if (!ent->decel)
		ent->decel = 5;
	else
		ent->decel *= 0.1f;

	if (deathmatch->integer)
	{
		ent->speed *= 2;
		ent->accel *= 2;
		ent->decel *= 2;
	}

	// PMM Added to kill things it's being blocked by
	if (!ent->dmg)
		ent->dmg = 2;

	//	if (!st.lip)
	//		st.lip = 8;

	// pos1 is the top position, pos2 is the bottom
	ent->pos1 = ent->s.origin;
	ent->pos2 = ent->s.origin;

	if (st.height)
		ent->pos2[2] -= (st.height - st.lip);
	else
		ent->pos2[2] -= (ent->maxs[2] - ent->mins[2]) - st.lip;

	ent->moveinfo.state = STATE_TOP;

	if (ent->targetname && !(ent->spawnflags & SPAWNFLAGS_PLAT2_START_ACTIVE))
	{
		ent->use = plat2_activate;
	}
	else
	{
		ent->use = Use_Plat2;

		trigger = plat_spawn_inside_trigger(ent); // the "start moving" trigger

		// PGM - debugging??
		trigger->maxs[0] += 10;
		trigger->maxs[1] += 10;
		trigger->mins[0] -= 10;
		trigger->mins[1] -= 10;

		gi.linkentity(trigger);

		trigger->touch = Touch_Plat_Center2; // Override trigger touch function

		if (!(ent->spawnflags & SPAWNFLAGS_PLAT2_TOP))
		{
			ent->s.origin = ent->pos2;
			ent->moveinfo.state = STATE_BOTTOM;
		}
	}

	gi.linkentity(ent);

	ent->moveinfo.speed = ent->speed;
	ent->moveinfo.accel = ent->accel;
	ent->moveinfo.decel = ent->decel;
	ent->moveinfo.wait = ent->wait;
	ent->moveinfo.start_origin = ent->pos1;
	ent->moveinfo.start_angles = ent->s.angles;
	ent->moveinfo.end_origin = ent->pos2;
	ent->moveinfo.end_angles = ent->s.angles;

	G_SetMoveinfoSounds(ent, "plats/pt1_strt.wav", "plats/pt1_mid.wav", "plats/pt1_end.wav");
}
