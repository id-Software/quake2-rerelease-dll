// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"

/*
=========================================================

  PLATS

  movement options:

  linear
  smooth start, hard stop
  smooth start, smooth stop

  start
  end
  acceleration
  speed
  deceleration
  begin sound
  end sound
  target fired when reaching end
  wait at end

  object characteristics that use move segments
  ---------------------------------------------
  movetype_push, or movetype_stop
  action when touched
  action when blocked
  action when used
	disabled?
  auto trigger spawning


=========================================================
*/

constexpr spawnflags_t SPAWNFLAG_DOOR_START_OPEN = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAG_DOOR_CRUSHER = 4_spawnflag;
constexpr spawnflags_t SPAWNFLAG_DOOR_NOMONSTER = 8_spawnflag;
constexpr spawnflags_t SPAWNFLAG_DOOR_ANIMATED = 16_spawnflag;
constexpr spawnflags_t SPAWNFLAG_DOOR_TOGGLE = 32_spawnflag;
constexpr spawnflags_t SPAWNFLAG_DOOR_ANIMATED_FAST = 64_spawnflag;

constexpr spawnflags_t SPAWNFLAG_DOOR_ROTATING_X_AXIS = 64_spawnflag;
constexpr spawnflags_t SPAWNFLAG_DOOR_ROTATING_Y_AXIS = 128_spawnflag;
constexpr spawnflags_t SPAWNFLAG_DOOR_ROTATING_INACTIVE = 0x10000_spawnflag; // Paril: moved to non-reserved
constexpr spawnflags_t SPAWNFLAG_DOOR_ROTATING_SAFE_OPEN = 0x20000_spawnflag;

// support routine for setting moveinfo sounds
inline int32_t G_GetMoveinfoSoundIndex(edict_t *self, const char *default_value, const char *wanted_value)
{
	if (!wanted_value)
	{
		if (default_value)
			return gi.soundindex(default_value);

		return 0;
	}
	else if (!*wanted_value || *wanted_value == '0' || *wanted_value == ' ')
		return 0;

	return gi.soundindex(wanted_value);
}

void G_SetMoveinfoSounds(edict_t *self, const char *default_start, const char *default_mid, const char *default_end)
{
	self->moveinfo.sound_start = G_GetMoveinfoSoundIndex(self, default_start, st.noise_start);
	self->moveinfo.sound_middle = G_GetMoveinfoSoundIndex(self, default_mid, st.noise_middle);
	self->moveinfo.sound_end = G_GetMoveinfoSoundIndex(self, default_end, st.noise_end);
}

//
// Support routines for movement (changes in origin using velocity)
//

THINK(Move_Done) (edict_t *ent) -> void
{
	ent->velocity = {};
	ent->moveinfo.endfunc(ent);
}

THINK(Move_Final) (edict_t *ent) -> void
{
	if (ent->moveinfo.remaining_distance == 0)
	{
		Move_Done(ent);
		return;
	}

	// [Paril-KEX] use exact remaining distance
	ent->velocity = (ent->moveinfo.dest - ent->s.origin) * (1.f / gi.frame_time_s);

	ent->think = Move_Done;
	ent->nextthink = level.time + FRAME_TIME_S;
}

THINK(Move_Begin) (edict_t *ent) -> void
{
	float frames;

	if ((ent->moveinfo.speed * gi.frame_time_s) >= ent->moveinfo.remaining_distance)
	{
		Move_Final(ent);
		return;
	}
	ent->velocity = ent->moveinfo.dir * ent->moveinfo.speed;
	frames = floor((ent->moveinfo.remaining_distance / ent->moveinfo.speed) / gi.frame_time_s);
	ent->moveinfo.remaining_distance -= frames * ent->moveinfo.speed * gi.frame_time_s;
	ent->nextthink = level.time + (FRAME_TIME_S * frames);
	ent->think = Move_Final;
}

void Think_AccelMove_New(edict_t *ent);
void Think_AccelMove(edict_t *ent);
bool Think_AccelMove_MoveInfo(moveinfo_t *moveinfo);

constexpr float AccelerationDistance(float target, float rate)
{
	return (target * ((target / rate) + 1) / 2);
}

inline void Move_Regular(edict_t *ent, const vec3_t &dest, void(*endfunc)(edict_t *self))
{
	if (level.current_entity == ((ent->flags & FL_TEAMSLAVE) ? ent->teammaster : ent))
	{
		Move_Begin(ent);
	}
	else
	{
		ent->nextthink = level.time + FRAME_TIME_S;
		ent->think = Move_Begin;
	}
}

void Move_Calc(edict_t *ent, const vec3_t &dest, void(*endfunc)(edict_t *self))
{
	ent->velocity = {};
	ent->moveinfo.dest = dest;
	ent->moveinfo.dir = dest - ent->s.origin;
	ent->moveinfo.remaining_distance = ent->moveinfo.dir.normalize();
	ent->moveinfo.endfunc = endfunc;

	if (ent->moveinfo.speed == ent->moveinfo.accel && ent->moveinfo.speed == ent->moveinfo.decel)
	{
		Move_Regular(ent, dest, endfunc);
	}
	else
	{
		// accelerative
		ent->moveinfo.current_speed = 0;

		if (gi.tick_rate == 10)
			ent->think = Think_AccelMove;
		else
		{
			// [Paril-KEX] rewritten to work better at higher tickrates
			ent->moveinfo.curve_frame = 0;
			ent->moveinfo.num_subframes = (0.1f / gi.frame_time_s) - 1;

			float total_dist = ent->moveinfo.remaining_distance;

			std::vector<float> distances;

			if (ent->moveinfo.num_subframes)
			{
				distances.push_back(0);
				ent->moveinfo.curve_frame = 1;
			}
			else
				ent->moveinfo.curve_frame = 0;

			// simulate 10hz movement
			while (ent->moveinfo.remaining_distance)
			{
				if (!Think_AccelMove_MoveInfo(&ent->moveinfo))
					break;

				ent->moveinfo.remaining_distance -= ent->moveinfo.current_speed;
				distances.push_back(total_dist - ent->moveinfo.remaining_distance);
			}

			if (ent->moveinfo.num_subframes)
				distances.push_back(total_dist);

			ent->moveinfo.subframe = 0;
			ent->moveinfo.curve_ref = ent->s.origin;
			ent->moveinfo.curve_positions = make_savable_memory<float, TAG_LEVEL>(distances.size());
			std::copy(distances.begin(), distances.end(), ent->moveinfo.curve_positions.ptr);

			ent->moveinfo.num_frames_done = 0;

			ent->think = Think_AccelMove_New;
		}

		ent->nextthink = level.time + FRAME_TIME_S;
	}
}

THINK(Think_AccelMove_New) (edict_t *ent) -> void
{
	float t = 0.f;
	float target_dist;

	if (ent->moveinfo.num_subframes)
	{
		if (ent->moveinfo.subframe == ent->moveinfo.num_subframes + 1)
		{
			ent->moveinfo.subframe = 0;
			ent->moveinfo.curve_frame++;

			if (ent->moveinfo.curve_frame == ent->moveinfo.curve_positions.count)
			{
				Move_Final(ent);
				return;
			}
		}

		t = (ent->moveinfo.subframe + 1) / ((float) ent->moveinfo.num_subframes + 1);

		target_dist = lerp(ent->moveinfo.curve_positions[ent->moveinfo.curve_frame - 1], ent->moveinfo.curve_positions[ent->moveinfo.curve_frame], t);
		ent->moveinfo.subframe++;
	}
	else
	{
		if (ent->moveinfo.curve_frame == ent->moveinfo.curve_positions.count)
		{
			Move_Final(ent);
			return;
		}

		target_dist = ent->moveinfo.curve_positions[ent->moveinfo.curve_frame++];
	}

	ent->moveinfo.num_frames_done++;
	vec3_t target_pos = ent->moveinfo.curve_ref + (ent->moveinfo.dir * target_dist);
	ent->velocity = (target_pos - ent->s.origin) * (1.f / gi.frame_time_s);
	ent->nextthink = level.time + FRAME_TIME_S;
}

//
// Support routines for angular movement (changes in angle using avelocity)
//

THINK(AngleMove_Done) (edict_t *ent) -> void
{
	ent->avelocity = {};
	ent->moveinfo.endfunc(ent);
}

THINK(AngleMove_Final) (edict_t *ent) -> void
{
	vec3_t move;

	if (ent->moveinfo.state == STATE_UP)
	{
		if (ent->moveinfo.reversing)
			move = ent->moveinfo.end_angles_reversed - ent->s.angles;
		else
			move = ent->moveinfo.end_angles - ent->s.angles;
	}
	else
		move = ent->moveinfo.start_angles - ent->s.angles;

	if (!move)
	{
		AngleMove_Done(ent);
		return;
	}

	ent->avelocity = move * (1.0f / gi.frame_time_s);

	ent->think = AngleMove_Done;
	ent->nextthink = level.time + FRAME_TIME_S;
}

THINK(AngleMove_Begin) (edict_t *ent) -> void
{
	vec3_t destdelta;
	float  len;
	float  traveltime;
	float  frames;

	// PGM		accelerate as needed
	if (ent->moveinfo.speed < ent->speed)
	{
		ent->moveinfo.speed += ent->accel;
		if (ent->moveinfo.speed > ent->speed)
			ent->moveinfo.speed = ent->speed;
	}
	// PGM

	// set destdelta to the vector needed to move
	if (ent->moveinfo.state == STATE_UP)
	{
		if (ent->moveinfo.reversing)
			destdelta = ent->moveinfo.end_angles_reversed - ent->s.angles;
		else
			destdelta = ent->moveinfo.end_angles - ent->s.angles;
	}
	else
		destdelta = ent->moveinfo.start_angles - ent->s.angles;

	// calculate length of vector
	len = destdelta.length();

	// divide by speed to get time to reach dest
	traveltime = len / ent->moveinfo.speed;

	if (traveltime < gi.frame_time_s)
	{
		AngleMove_Final(ent);
		return;
	}

	frames = floor(traveltime / gi.frame_time_s);

	// scale the destdelta vector by the time spent traveling to get velocity
	ent->avelocity = destdelta * (1.0f / traveltime);

	// PGM
	//  if we're done accelerating, act as a normal rotation
	if (ent->moveinfo.speed >= ent->speed)
	{
		// set nextthink to trigger a think when dest is reached
		ent->nextthink = level.time + (FRAME_TIME_S * frames);
		ent->think = AngleMove_Final;
	}
	else
	{
		ent->nextthink = level.time + FRAME_TIME_S;
		ent->think = AngleMove_Begin;
	}
	// PGM
}

void AngleMove_Calc(edict_t *ent, void(*endfunc)(edict_t *self))
{
	ent->avelocity = {};
	ent->moveinfo.endfunc = endfunc;

	// PGM
	//  if we're supposed to accelerate, this will tell anglemove_begin to do so
	if (ent->accel != ent->speed)
		ent->moveinfo.speed = 0;
	// PGM

	if (level.current_entity == ((ent->flags & FL_TEAMSLAVE) ? ent->teammaster : ent))
	{
		AngleMove_Begin(ent);
	}
	else
	{
		ent->nextthink = level.time + FRAME_TIME_S;
		ent->think = AngleMove_Begin;
	}
}

/*
==============
Think_AccelMove

The team has completed a frame of movement, so
change the speed for the next frame
==============
*/
void plat_CalcAcceleratedMove(moveinfo_t *moveinfo)
{
	float accel_dist;
	float decel_dist;

	if (moveinfo->remaining_distance < moveinfo->accel)
	{
		moveinfo->move_speed = moveinfo->speed;
		moveinfo->current_speed = moveinfo->remaining_distance;
		return;
	}

	accel_dist = AccelerationDistance(moveinfo->speed, moveinfo->accel);
	decel_dist = AccelerationDistance(moveinfo->speed, moveinfo->decel);

	if ((moveinfo->remaining_distance - accel_dist - decel_dist) < 0)
	{
		float f;

		f = (moveinfo->accel + moveinfo->decel) / (moveinfo->accel * moveinfo->decel);
		moveinfo->move_speed = moveinfo->current_speed =
			(-2 + sqrt(4 - 4 * f * (-2 * moveinfo->remaining_distance))) / (2 * f);
		decel_dist = AccelerationDistance(moveinfo->move_speed, moveinfo->decel);
	}
	else
		moveinfo->move_speed = moveinfo->speed;

	moveinfo->decel_distance = decel_dist;
};

void plat_Accelerate(moveinfo_t *moveinfo)
{
	// are we decelerating?
	if (moveinfo->remaining_distance <= moveinfo->decel_distance)
	{
		if (moveinfo->remaining_distance < moveinfo->decel_distance)
		{
			if (moveinfo->next_speed)
			{
				moveinfo->current_speed = moveinfo->next_speed;
				moveinfo->next_speed = 0;
				return;
			}
			if (moveinfo->current_speed > moveinfo->decel)
			{
				moveinfo->current_speed -= moveinfo->decel;

				// [Paril-KEX] fix platforms in xdm6, etc
				if (fabsf(moveinfo->current_speed) < 0.01f)
					moveinfo->current_speed = moveinfo->remaining_distance + 1;
			}
		}
		return;
	}
	
	// are we at full speed and need to start decelerating during this move?
	if (moveinfo->current_speed == moveinfo->move_speed)
		if ((moveinfo->remaining_distance - moveinfo->current_speed) < moveinfo->decel_distance)
		{
			float p1_distance;
			float p2_distance;
			float distance;

			p1_distance = moveinfo->remaining_distance - moveinfo->decel_distance;
			p2_distance = moveinfo->move_speed * (1.0f - (p1_distance / moveinfo->move_speed));
			distance = p1_distance + p2_distance;
			moveinfo->current_speed = moveinfo->move_speed;
			moveinfo->next_speed = moveinfo->move_speed - moveinfo->decel * (p2_distance / distance);
			return;
		}

	// are we accelerating?
	if (moveinfo->current_speed < moveinfo->speed)
	{
		float old_speed;
		float p1_distance;
		float p1_speed;
		float p2_distance;
		float distance;

		old_speed = moveinfo->current_speed;

		// figure simple acceleration up to move_speed
		moveinfo->current_speed += moveinfo->accel;
		if (moveinfo->current_speed > moveinfo->speed)
			moveinfo->current_speed = moveinfo->speed;

		// are we accelerating throughout this entire move?
		if ((moveinfo->remaining_distance - moveinfo->current_speed) >= moveinfo->decel_distance)
			return;

		// during this move we will accelerate from current_speed to move_speed
		// and cross over the decel_distance; figure the average speed for the
		// entire move
		p1_distance = moveinfo->remaining_distance - moveinfo->decel_distance;
		p1_speed = (old_speed + moveinfo->move_speed) / 2.0f;
		p2_distance = moveinfo->move_speed * (1.0f - (p1_distance / p1_speed));
		distance = p1_distance + p2_distance;
		moveinfo->current_speed =
			(p1_speed * (p1_distance / distance)) + (moveinfo->move_speed * (p2_distance / distance));
		moveinfo->next_speed = moveinfo->move_speed - moveinfo->decel * (p2_distance / distance);
		return;
	}

	// we are at constant velocity (move_speed)
	return;
}

bool Think_AccelMove_MoveInfo (moveinfo_t *moveinfo)
{
	if (moveinfo->current_speed == 0)		// starting or blocked
		plat_CalcAcceleratedMove(moveinfo);

	plat_Accelerate(moveinfo);

	// will the entire move complete on next frame?
	return moveinfo->remaining_distance > moveinfo->current_speed;
}

// Paril: old acceleration code; this is here only to support old save games.
THINK(Think_AccelMove) (edict_t *ent) -> void
{
	// [Paril-KEX] calculate distance dynamically
	if (ent->moveinfo.state == STATE_UP)
		ent->moveinfo.remaining_distance = (ent->moveinfo.start_origin - ent->s.origin).length();
	else
		ent->moveinfo.remaining_distance = (ent->moveinfo.end_origin - ent->s.origin).length();

	// will the entire move complete on next frame?
	if (!Think_AccelMove_MoveInfo(&ent->moveinfo))
	{
		Move_Final(ent);
		return;
	}

	if (ent->moveinfo.remaining_distance <= ent->moveinfo.current_speed)
	{
		Move_Final(ent);
		return;
	}

	ent->velocity = ent->moveinfo.dir * (ent->moveinfo.current_speed * 10);
	ent->nextthink = level.time + 10_hz;
	ent->think = Think_AccelMove;
}

void plat_go_down(edict_t *ent);

MOVEINFO_ENDFUNC(plat_hit_top) (edict_t *ent) -> void
{
	if (!(ent->flags & FL_TEAMSLAVE))
	{
		if (ent->moveinfo.sound_end)
			gi.sound(ent, CHAN_NO_PHS_ADD | CHAN_VOICE, ent->moveinfo.sound_end, 1, ATTN_STATIC, 0);
	}
	ent->s.sound = 0;
	ent->moveinfo.state = STATE_TOP;

	ent->think = plat_go_down;
	ent->nextthink = level.time + 3_sec;
}

MOVEINFO_ENDFUNC(plat_hit_bottom) (edict_t *ent) -> void
{
	if (!(ent->flags & FL_TEAMSLAVE))
	{
		if (ent->moveinfo.sound_end)
			gi.sound(ent, CHAN_NO_PHS_ADD | CHAN_VOICE, ent->moveinfo.sound_end, 1, ATTN_STATIC, 0);
	}
	ent->s.sound = 0;
	ent->moveinfo.state = STATE_BOTTOM;

	// ROGUE
	plat2_kill_danger_area(ent);
	// ROGUE
}

THINK(plat_go_down) (edict_t *ent) -> void
{
	if (!(ent->flags & FL_TEAMSLAVE))
	{
		if (ent->moveinfo.sound_start)
			gi.sound(ent, CHAN_NO_PHS_ADD | CHAN_VOICE, ent->moveinfo.sound_start, 1, ATTN_STATIC, 0);
	}

	ent->s.sound = ent->moveinfo.sound_middle;

	ent->moveinfo.state = STATE_DOWN;
	Move_Calc(ent, ent->moveinfo.end_origin, plat_hit_bottom);
}

void plat_go_up(edict_t *ent)
{
	if (!(ent->flags & FL_TEAMSLAVE))
	{
		if (ent->moveinfo.sound_start)
			gi.sound(ent, CHAN_NO_PHS_ADD | CHAN_VOICE, ent->moveinfo.sound_start, 1, ATTN_STATIC, 0);
	}

	ent->s.sound = ent->moveinfo.sound_middle;

	ent->moveinfo.state = STATE_UP;
	Move_Calc(ent, ent->moveinfo.start_origin, plat_hit_top);

	// ROGUE
	plat2_spawn_danger_area(ent);
	// ROGUE
}

MOVEINFO_BLOCKED(plat_blocked) (edict_t *self, edict_t *other) -> void
{
	if (!(other->svflags & SVF_MONSTER) && (!other->client))
	{
		// give it a chance to go away on it's own terms (like gibs)
		T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, DAMAGE_NONE, MOD_CRUSH);
		// if it's still there, nuke it
		if (other && other->inuse && other->solid) // PGM
			BecomeExplosion1(other);
		return;
	}

	// PGM
	//  gib dead things
	if (other->health < 1)
		T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 100, 1, DAMAGE_NONE, MOD_CRUSH);
	// PGM

	T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, DAMAGE_NONE, MOD_CRUSH);

	// [Paril-KEX] killed the thing, so don't switch directions
	if (!other->inuse || !other->solid)
		return;

	if (self->moveinfo.state == STATE_UP)
		plat_go_down(self);
	else if (self->moveinfo.state == STATE_DOWN)
		plat_go_up(self);
}

constexpr spawnflags_t SPAWNFLAG_PLAT_LOW_TRIGGER = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAG_PLAT_NO_MONSTER = 2_spawnflag;

USE(Use_Plat) (edict_t *ent, edict_t *other, edict_t *activator) -> void
{
	//======
	// ROGUE
	// if a monster is using us, then allow the activity when stopped.
	if ((other->svflags & SVF_MONSTER) && !(ent->spawnflags & SPAWNFLAG_PLAT_NO_MONSTER))
	{
		if (ent->moveinfo.state == STATE_TOP)
			plat_go_down(ent);
		else if (ent->moveinfo.state == STATE_BOTTOM)
			plat_go_up(ent);

		return;
	}
	// ROGUE
	//======

	if (ent->think)
		return; // already down
	plat_go_down(ent);
}

TOUCH(Touch_Plat_Center) (edict_t *ent, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	if (!other->client)
		return;

	if (other->health <= 0)
		return;

	ent = ent->enemy; // now point at the plat, not the trigger
	if (ent->moveinfo.state == STATE_BOTTOM)
		plat_go_up(ent);
	else if (ent->moveinfo.state == STATE_TOP)
		ent->nextthink = level.time + 1_sec; // the player is still on the plat, so delay going down
}

// PGM - plat2's change the trigger field
edict_t *plat_spawn_inside_trigger(edict_t *ent)
{
	edict_t *trigger;
	vec3_t	 tmin, tmax;

	//
	// middle trigger
	//
	trigger = G_Spawn();
	trigger->touch = Touch_Plat_Center;
	trigger->movetype = MOVETYPE_NONE;
	trigger->solid = SOLID_TRIGGER;
	trigger->enemy = ent;

	tmin[0] = ent->mins[0] + 25;
	tmin[1] = ent->mins[1] + 25;
	tmin[2] = ent->mins[2];

	tmax[0] = ent->maxs[0] - 25;
	tmax[1] = ent->maxs[1] - 25;
	tmax[2] = ent->maxs[2] + 8;

	tmin[2] = tmax[2] - (ent->pos1[2] - ent->pos2[2] + st.lip);

	if (ent->spawnflags.has(SPAWNFLAG_PLAT_LOW_TRIGGER))
		tmax[2] = tmin[2] + 8;

	if (tmax[0] - tmin[0] <= 0)
	{
		tmin[0] = (ent->mins[0] + ent->maxs[0]) * 0.5f;
		tmax[0] = tmin[0] + 1;
	}
	if (tmax[1] - tmin[1] <= 0)
	{
		tmin[1] = (ent->mins[1] + ent->maxs[1]) * 0.5f;
		tmax[1] = tmin[1] + 1;
	}

	trigger->mins = tmin;
	trigger->maxs = tmax;

	gi.linkentity(trigger);

	return trigger; // PGM 11/17/97
}

/*QUAKED func_plat (0 .5 .8) ? PLAT_LOW_TRIGGER
speed	default 150

Plats are always drawn in the extended position, so they will light correctly.

If the plat is the target of another trigger or button, it will start out disabled in the extended position until it is triggered, when it will lower and become a normal plat.

"speed"	overrides default 200.
"accel" overrides default 500
"lip"	overrides default 8 pixel lip

If the "height" key is set, that will determine the amount the plat moves, instead of being implicitly determined by the model's height.

Set "sounds" to one of the following:
1) base fast
2) chain slow
*/
void SP_func_plat(edict_t *ent)
{
	ent->s.angles = {};
	ent->solid = SOLID_BSP;
	ent->movetype = MOVETYPE_PUSH;

	gi.setmodel(ent, ent->model);

	ent->moveinfo.blocked = plat_blocked;

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

	if (!ent->dmg)
		ent->dmg = 2;

	if (!st.lip)
		st.lip = 8;

	// pos1 is the top position, pos2 is the bottom
	ent->pos1 = ent->s.origin;
	ent->pos2 = ent->s.origin;
	if (st.height)
		ent->pos2[2] -= st.height;
	else
		ent->pos2[2] -= (ent->maxs[2] - ent->mins[2]) - st.lip;

	ent->use = Use_Plat;

	plat_spawn_inside_trigger(ent); // the "start moving" trigger

	if (ent->targetname)
	{
		ent->moveinfo.state = STATE_UP;
	}
	else
	{
		ent->s.origin = ent->pos2;
		gi.linkentity(ent);
		ent->moveinfo.state = STATE_BOTTOM;
	}

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

//====================================================================

// Paril: Rogue added a spawnflag in func_rotating that
// is a reserved editor flag.
constexpr spawnflags_t SPAWNFLAG_ROTATING_START_ON = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAG_ROTATING_REVERSE = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAG_ROTATING_X_AXIS = 4_spawnflag;
constexpr spawnflags_t SPAWNFLAG_ROTATING_Y_AXIS = 8_spawnflag;
constexpr spawnflags_t SPAWNFLAG_ROTATING_TOUCH_PAIN = 16_spawnflag;
constexpr spawnflags_t SPAWNFLAG_ROTATING_STOP = 32_spawnflag;
constexpr spawnflags_t SPAWNFLAG_ROTATING_ANIMATED = 64_spawnflag;
constexpr spawnflags_t SPAWNFLAG_ROTATING_ANIMATED_FAST = 128_spawnflag;
constexpr spawnflags_t SPAWNFLAG_ROTATING_ACCEL = 0x00010000_spawnflag;

/*QUAKED func_rotating (0 .5 .8) ? START_ON REVERSE X_AXIS Y_AXIS TOUCH_PAIN STOP ANIMATED ANIMATED_FAST NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP RESERVED1 COOP_ONLY RESERVED2 ACCEL
You need to have an origin brush as part of this entity.
The center of that brush will be the point around which it is rotated. It will rotate around the Z axis by default.
You can check either the X_AXIS or Y_AXIS box to change that.

func_rotating will use it's targets when it stops and starts.

"speed" determines how fast it moves; default value is 100.
"dmg"	damage to inflict when blocked (2 default)
"accel" if specified, is how much the rotation speed will increase per .1sec.

REVERSE will cause the it to rotate in the opposite direction.
STOP mean it will stop moving instead of pushing entities
ACCEL means it will accelerate to it's final speed and decelerate when shutting down.
*/

//============
// PGM
THINK(rotating_accel) (edict_t *self) -> void
{
	float current_speed;

	current_speed = self->avelocity.length();
	if (current_speed >= (self->speed - self->accel)) // done
	{
		self->avelocity = self->movedir * self->speed;
		G_UseTargets(self, self);
	}
	else
	{
		current_speed += self->accel;
		self->avelocity = self->movedir * current_speed;
		self->think = rotating_accel;
		self->nextthink = level.time + FRAME_TIME_S;
	}
}

THINK(rotating_decel) (edict_t *self) -> void
{
	float current_speed;

	current_speed = self->avelocity.length();
	if (current_speed <= self->decel) // done
	{
		self->avelocity = {};
		G_UseTargets(self, self);
		self->touch = nullptr;
	}
	else
	{
		current_speed -= self->decel;
		self->avelocity = self->movedir * current_speed;
		self->think = rotating_decel;
		self->nextthink = level.time + FRAME_TIME_S;
	}
}
// PGM
//============

MOVEINFO_BLOCKED(rotating_blocked) (edict_t *self, edict_t *other) -> void
{
	if (!self->dmg)
		return;
	if (level.time < self->touch_debounce_time)
		return;
	self->touch_debounce_time = level.time + 10_hz;
	T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, DAMAGE_NONE, MOD_CRUSH);
}

TOUCH(rotating_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	if (self->avelocity[0] || self->avelocity[1] || self->avelocity[2])
		T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, DAMAGE_NONE, MOD_CRUSH);
}

USE(rotating_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	if (self->avelocity)
	{
		self->s.sound = 0;
		// PGM
		if (self->spawnflags.has(SPAWNFLAG_ROTATING_ACCEL)) // Decelerate
			rotating_decel(self);
		else
		{
			self->avelocity = {};
			G_UseTargets(self, self);
			self->touch = nullptr;
		}
		// PGM
	}
	else
	{
		self->s.sound = self->moveinfo.sound_middle;
		// PGM
		if (self->spawnflags.has(SPAWNFLAG_ROTATING_ACCEL)) // accelerate
			rotating_accel(self);
		else
		{
			self->avelocity = self->movedir * self->speed;
			G_UseTargets(self, self);
		}
		if (self->spawnflags.has(SPAWNFLAG_ROTATING_TOUCH_PAIN))
			self->touch = rotating_touch;
		// PGM
	}
}

void SP_func_rotating(edict_t *ent)
{
	ent->solid = SOLID_BSP;
	if (ent->spawnflags.has(SPAWNFLAG_ROTATING_STOP))
		ent->movetype = MOVETYPE_STOP;
	else
		ent->movetype = MOVETYPE_PUSH;

	if (st.noise)
	{
		ent->moveinfo.sound_middle = gi.soundindex(st.noise);

		// [Paril-KEX] for rhangar1 doors
		if (!st.was_key_specified("attenuation"))
			ent->attenuation = ATTN_STATIC;
		else
		{
			if (ent->attenuation == -1)
			{
				ent->s.loop_attenuation = ATTN_LOOP_NONE;
				ent->attenuation = ATTN_NONE;
			}
			else
			{
				ent->s.loop_attenuation = ent->attenuation;
			}
		}
	}

	// set the axis of rotation
	ent->movedir = {};
	if (ent->spawnflags.has(SPAWNFLAG_ROTATING_X_AXIS))
		ent->movedir[2] = 1.0;
	else if (ent->spawnflags.has(SPAWNFLAG_ROTATING_Y_AXIS))
		ent->movedir[0] = 1.0;
	else // Z_AXIS
		ent->movedir[1] = 1.0;

	// check for reverse rotation
	if (ent->spawnflags.has(SPAWNFLAG_ROTATING_REVERSE))
		ent->movedir = -ent->movedir;

	if (!ent->speed)
		ent->speed = 100;
	if (!st.was_key_specified("dmg"))
		ent->dmg = 2;

	ent->use = rotating_use;
	if (ent->dmg)
		ent->moveinfo.blocked = rotating_blocked;

	if (ent->spawnflags.has(SPAWNFLAG_ROTATING_START_ON))
		ent->use(ent, nullptr, nullptr);

	if (ent->spawnflags.has(SPAWNFLAG_ROTATING_ANIMATED))
		ent->s.effects |= EF_ANIM_ALL;
	if (ent->spawnflags.has(SPAWNFLAG_ROTATING_ANIMATED_FAST))
		ent->s.effects |= EF_ANIM_ALLFAST;

	// PGM
	if (ent->spawnflags.has(SPAWNFLAG_ROTATING_ACCEL)) // Accelerate / Decelerate
	{
		if (!ent->accel)
			ent->accel = 1;
		else if (ent->accel > ent->speed)
			ent->accel = ent->speed;

		if (!ent->decel)
			ent->decel = 1;
		else if (ent->decel > ent->speed)
			ent->decel = ent->speed;
	}
	// PGM

	gi.setmodel(ent, ent->model);
	gi.linkentity(ent);
}

THINK(func_spinning_think) (edict_t *ent) -> void
{
	if (ent->timestamp <= level.time)
	{
		ent->timestamp = level.time + random_time(1_sec, 6_sec);
		ent->movedir = { ent->decel + frandom(ent->speed - ent->decel), ent->decel + frandom(ent->speed - ent->decel), ent->decel + frandom(ent->speed - ent->decel) };

		for (int32_t i = 0; i < 3; i++)
		{
			if (brandom())
				ent->movedir[i] = -ent->movedir[i];
		}
	}

	for (int32_t i = 0; i < 3; i++)
	{
		if (ent->avelocity[i] == ent->movedir[i])
			continue;

		if (ent->avelocity[i] < ent->movedir[i])
			ent->avelocity[i] = min(ent->movedir[i], ent->avelocity[i] + ent->accel);
		else
			ent->avelocity[i] = max(ent->movedir[i], ent->avelocity[i] - ent->accel);
	}

	ent->nextthink = level.time + FRAME_TIME_MS;
}

// [Paril-KEX]
void SP_func_spinning(edict_t *ent)
{
	ent->solid = SOLID_BSP;

	if (!ent->speed)
		ent->speed = 100;
	if (!ent->dmg)
		ent->dmg = 2;

	ent->movetype = MOVETYPE_PUSH;

	ent->timestamp = 0_ms;
	ent->nextthink = level.time + FRAME_TIME_MS;
	ent->think = func_spinning_think;

	gi.setmodel(ent, ent->model);
	gi.linkentity(ent);
}

/*
======================================================================

BUTTONS

======================================================================
*/

/*QUAKED func_button (0 .5 .8) ?
When a button is touched, it moves some distance in the direction of it's angle, triggers all of it's targets, waits some time, then returns to it's original position where it can be triggered again.

"angle"		determines the opening direction
"target"	all entities with a matching targetname will be used
"speed"		override the default 40 speed
"wait"		override the default 1 second wait (-1 = never return)
"lip"		override the default 4 pixel lip remaining at end of move
"health"	if set, the button must be killed instead of touched
"sounds"
1) silent
2) steam metal
3) wooden clunk
4) metallic click
5) in-out
*/

MOVEINFO_ENDFUNC(button_done) (edict_t *self) -> void
{
	self->moveinfo.state = STATE_BOTTOM;
	if (!self->bmodel_anim.enabled)
	{
		if (level.is_n64)
			self->s.frame = 0;
		else
			self->s.effects &= ~EF_ANIM23;
		self->s.effects |= EF_ANIM01;
	}
	else
		self->bmodel_anim.alternate = false;
}

THINK(button_return) (edict_t *self) -> void
{
	self->moveinfo.state = STATE_DOWN;

	Move_Calc(self, self->moveinfo.start_origin, button_done);

	if (self->health)
		self->takedamage = true;
}

MOVEINFO_ENDFUNC(button_wait) (edict_t *self) -> void
{
	self->moveinfo.state = STATE_TOP;
	
	if (!self->bmodel_anim.enabled)
	{
		self->s.effects &= ~EF_ANIM01;
		if (level.is_n64)
			self->s.frame = 2;
		else
			self->s.effects |= EF_ANIM23;
	}
	else
		self->bmodel_anim.alternate = true;

	G_UseTargets(self, self->activator);

	if (self->moveinfo.wait >= 0)
	{
		self->nextthink = level.time + gtime_t::from_sec(self->moveinfo.wait);
		self->think = button_return;
	}
}

void button_fire(edict_t *self)
{
	if (self->moveinfo.state == STATE_UP || self->moveinfo.state == STATE_TOP)
		return;

	self->moveinfo.state = STATE_UP;
	if (self->moveinfo.sound_start && !(self->flags & FL_TEAMSLAVE))
		gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, self->moveinfo.sound_start, 1, ATTN_STATIC, 0);
	Move_Calc(self, self->moveinfo.end_origin, button_wait);
}

USE(button_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	self->activator = activator;
	button_fire(self);
}

TOUCH(button_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	if (!other->client)
		return;

	if (other->health <= 0)
		return;

	self->activator = other;
	button_fire(self);
}

DIE(button_killed) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	self->activator = attacker;
	self->health = self->max_health;
	self->takedamage = false;
	button_fire(self);
}

void SP_func_button(edict_t *ent)
{
	vec3_t abs_movedir;
	float  dist;

	G_SetMovedir(ent->s.angles, ent->movedir);
	ent->movetype = MOVETYPE_STOP;
	ent->solid = SOLID_BSP;
	gi.setmodel(ent, ent->model);

	if (ent->sounds != 1)
		G_SetMoveinfoSounds(ent, "switches/butn2.wav", nullptr, nullptr);
	else
		G_SetMoveinfoSounds(ent, nullptr, nullptr, nullptr);

	if (!ent->speed)
		ent->speed = 40;
	if (!ent->accel)
		ent->accel = ent->speed;
	if (!ent->decel)
		ent->decel = ent->speed;

	if (!ent->wait)
		ent->wait = 3;
	if (!st.lip)
		st.lip = 4;

	ent->pos1 = ent->s.origin;
	abs_movedir[0] = fabsf(ent->movedir[0]);
	abs_movedir[1] = fabsf(ent->movedir[1]);
	abs_movedir[2] = fabsf(ent->movedir[2]);
	dist = abs_movedir[0] * ent->size[0] + abs_movedir[1] * ent->size[1] + abs_movedir[2] * ent->size[2] - st.lip;
	ent->pos2 = ent->pos1 + (ent->movedir * dist);

	ent->use = button_use;

	if (!ent->bmodel_anim.enabled)
		ent->s.effects |= EF_ANIM01;

	if (ent->health)
	{
		ent->max_health = ent->health;
		ent->die = button_killed;
		ent->takedamage = true;
	}
	else if (!ent->targetname)
		ent->touch = button_touch;

	ent->moveinfo.state = STATE_BOTTOM;

	ent->moveinfo.speed = ent->speed;
	ent->moveinfo.accel = ent->accel;
	ent->moveinfo.decel = ent->decel;
	ent->moveinfo.wait = ent->wait;
	ent->moveinfo.start_origin = ent->pos1;
	ent->moveinfo.start_angles = ent->s.angles;
	ent->moveinfo.end_origin = ent->pos2;
	ent->moveinfo.end_angles = ent->s.angles;

	gi.linkentity(ent);
}

/*
======================================================================

DOORS

  spawn a trigger surrounding the entire team unless it is
  already targeted by another

======================================================================
*/

/*QUAKED func_door (0 .5 .8) ? START_OPEN x CRUSHER NOMONSTER ANIMATED TOGGLE ANIMATED_FAST
TOGGLE		wait in both the start and end states for a trigger event.
START_OPEN	the door to moves to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not useful for touch or takedamage doors).
NOMONSTER	monsters will not trigger this door

"message"	is printed when the door is touched if it is a trigger door and it hasn't been fired yet
"angle"		determines the opening direction
"targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
"health"	if set, door must be shot open
"speed"		movement speed (100 default)
"wait"		wait before returning (3 default, -1 = never return)
"lip"		lip remaining at end of move (8 default)
"dmg"		damage to inflict when blocked (2 default)
"sounds"
1)	silent
2)	light
3)	medium
4)	heavy
*/

void door_use_areaportals(edict_t *self, bool open)
{
	edict_t *t = nullptr;

	if (!self->target)
		return;

	while ((t = G_FindByString<&edict_t::targetname>(t, self->target)))
	{
		if (Q_strcasecmp(t->classname, "func_areaportal") == 0)
		{
			gi.SetAreaPortalState(t->style, open);
		}
	}
}

void door_go_down(edict_t *self);

static void door_play_sound(edict_t *self, int32_t sound)
{
	if (!self->teammaster)
	{
		gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, sound, 1, self->attenuation, 0);
		return;
	}

	vec3_t p = {};
	int32_t c = 0;

	for (edict_t *t = self->teammaster; t; t = t->teamchain)
	{
		p += (t->absmin + t->absmax) * 0.5f;
		c++;
	}

	if (c == 1)
	{
		gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, sound, 1, self->attenuation, 0);
		return;
	}

	p /= c;

	if (gi.pointcontents(p) & CONTENTS_SOLID)
	{
		gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, sound, 1, self->attenuation, 0);
		return;
	}

	gi.positioned_sound(p, self, CHAN_NO_PHS_ADD | CHAN_VOICE, sound, 1, self->attenuation, 0);
}

MOVEINFO_ENDFUNC(door_hit_top) (edict_t *self) -> void
{
	if (!(self->flags & FL_TEAMSLAVE))
	{
		if (self->moveinfo.sound_end)
			door_play_sound(self, self->moveinfo.sound_end);
	}
	self->s.sound = 0;
	self->moveinfo.state = STATE_TOP;
	if (self->spawnflags.has(SPAWNFLAG_DOOR_TOGGLE))
		return;
	if (self->moveinfo.wait >= 0)
	{
		self->think = door_go_down;
		self->nextthink = level.time + gtime_t::from_sec(self->moveinfo.wait);
	}

	if (self->spawnflags.has(SPAWNFLAG_DOOR_START_OPEN))
		door_use_areaportals(self, false);
}

MOVEINFO_ENDFUNC(door_hit_bottom) (edict_t *self) -> void
{
	if (!(self->flags & FL_TEAMSLAVE))
	{
		if (self->moveinfo.sound_end)
			door_play_sound(self, self->moveinfo.sound_end);
	}
	self->s.sound = 0;
	self->moveinfo.state = STATE_BOTTOM;

	if (!self->spawnflags.has(SPAWNFLAG_DOOR_START_OPEN))
		door_use_areaportals(self, false);
}

THINK(door_go_down) (edict_t *self) -> void
{
	if (!(self->flags & FL_TEAMSLAVE))
	{
		if (self->moveinfo.sound_start)
			door_play_sound(self, self->moveinfo.sound_start);
	}

	self->s.sound = self->moveinfo.sound_middle;

	if (self->max_health)
	{
		self->takedamage = true;
		self->health = self->max_health;
	}

	self->moveinfo.state = STATE_DOWN;
	if (strcmp(self->classname, "func_door") == 0 ||
		strcmp(self->classname, "func_water") == 0 ||
		strcmp(self->classname, "func_door_secret") == 0)
		Move_Calc(self, self->moveinfo.start_origin, door_hit_bottom);
	else if (strcmp(self->classname, "func_door_rotating") == 0)
		AngleMove_Calc(self, door_hit_bottom);

	if (self->spawnflags.has(SPAWNFLAG_DOOR_START_OPEN))
		door_use_areaportals(self, true);
}

void door_go_up(edict_t *self, edict_t *activator)
{
	if (self->moveinfo.state == STATE_UP)
		return; // already going up

	if (self->moveinfo.state == STATE_TOP)
	{ // reset top wait time
		if (self->moveinfo.wait >= 0)
			self->nextthink = level.time + gtime_t::from_sec(self->moveinfo.wait);
		return;
	}

	if (!(self->flags & FL_TEAMSLAVE))
	{
		if (self->moveinfo.sound_start)
			door_play_sound(self, self->moveinfo.sound_start);
	}

	self->s.sound = self->moveinfo.sound_middle;

	self->moveinfo.state = STATE_UP;
	if (strcmp(self->classname, "func_door") == 0 ||
		strcmp(self->classname, "func_water") == 0 ||
		strcmp(self->classname, "func_door_secret") == 0)
		Move_Calc(self, self->moveinfo.end_origin, door_hit_top);
	else if (strcmp(self->classname, "func_door_rotating") == 0)
		AngleMove_Calc(self, door_hit_top);

	G_UseTargets(self, activator);

	if (!(self->spawnflags & SPAWNFLAG_DOOR_START_OPEN))
		door_use_areaportals(self, true);
}

//======
// PGM

THINK(smart_water_go_up) (edict_t *self) -> void
{
	float	 distance;
	edict_t *lowestPlayer;
	edict_t *ent;
	float	 lowestPlayerPt;

	if (self->moveinfo.state == STATE_TOP)
	{ // reset top wait time
		if (self->moveinfo.wait >= 0)
			self->nextthink = level.time + gtime_t::from_sec(self->moveinfo.wait);
		return;
	}

	if (self->health)
	{
		if (self->absmax[2] >= self->health)
		{
			self->velocity = {};
			self->nextthink = 0_ms;
			self->moveinfo.state = STATE_TOP;
			return;
		}
	}

	if (!(self->flags & FL_TEAMSLAVE))
	{
		if (self->moveinfo.sound_start)
			gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, self->moveinfo.sound_start, 1, ATTN_STATIC, 0);
	}

	self->s.sound = self->moveinfo.sound_middle;

	// find the lowest player point.
	lowestPlayerPt = 999999;
	lowestPlayer = nullptr;
	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		ent = &g_edicts[1 + i];

		// don't count dead or unused player slots
		if ((ent->inuse) && (ent->health > 0))
		{
			if (ent->absmin[2] < lowestPlayerPt)
			{
				lowestPlayerPt = ent->absmin[2];
				lowestPlayer = ent;
			}
		}
	}

	if (!lowestPlayer)
	{
		return;
	}

	distance = lowestPlayerPt - self->absmax[2];

	// for the calculations, make sure we intend to go up at least a little.
	if (distance < self->accel)
	{
		distance = 100;
		self->moveinfo.speed = 5;
	}
	else
		self->moveinfo.speed = distance / self->accel;

	if (self->moveinfo.speed < 5)
		self->moveinfo.speed = 5;
	else if (self->moveinfo.speed > self->speed)
		self->moveinfo.speed = self->speed;

	// FIXME - should this allow any movement other than straight up?
	self->moveinfo.dir = { 0, 0, 1 };
	self->velocity = self->moveinfo.dir * self->moveinfo.speed;
	self->moveinfo.remaining_distance = distance;

	if (self->moveinfo.state != STATE_UP)
	{
		G_UseTargets(self, lowestPlayer);
		door_use_areaportals(self, true);
		self->moveinfo.state = STATE_UP;
	}

	self->think = smart_water_go_up;
	self->nextthink = level.time + FRAME_TIME_S;
}
// PGM
//======

USE(door_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	edict_t *ent;
	vec3_t	 center; // PGM

	if (self->flags & FL_TEAMSLAVE)
		return;

	if ((strcmp(self->classname, "func_door_rotating") == 0) && self->spawnflags.has(SPAWNFLAG_DOOR_ROTATING_SAFE_OPEN) &&
		(self->moveinfo.state == STATE_BOTTOM || self->moveinfo.state == STATE_DOWN))
	{
		if (self->moveinfo.dir)
		{
			vec3_t forward = (activator->s.origin - self->s.origin).normalized();
			self->moveinfo.reversing = forward.dot(self->moveinfo.dir) > 0;
		}
	}

	if (self->spawnflags.has(SPAWNFLAG_DOOR_TOGGLE))
	{
		if (self->moveinfo.state == STATE_UP || self->moveinfo.state == STATE_TOP)
		{
			// trigger all paired doors
			for (ent = self; ent; ent = ent->teamchain)
			{
				ent->message = nullptr;
				ent->touch = nullptr;
				door_go_down(ent);
			}
			return;
		}
	}

	// PGM
	//  smart water is different
	center = self->mins + self->maxs;
	center *= 0.5f;
	if ((strcmp(self->classname, "func_water") == 0) && (gi.pointcontents(center) & MASK_WATER) && self->spawnflags.has(SPAWNFLAG_WATER_SMART))
	{
		self->message = nullptr;
		self->touch = nullptr;
		self->enemy = activator;
		smart_water_go_up(self);
		return;
	}
	// PGM

	// trigger all paired doors
	for (ent = self; ent; ent = ent->teamchain)
	{
		ent->message = nullptr;
		ent->touch = nullptr;
		door_go_up(ent, activator);
	}
};

TOUCH(Touch_DoorTrigger) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	if (other->health <= 0)
		return;

	if (!(other->svflags & SVF_MONSTER) && (!other->client))
		return;

	if (self->owner->spawnflags.has(SPAWNFLAG_DOOR_NOMONSTER) && (other->svflags & SVF_MONSTER))
		return;

	if (level.time < self->touch_debounce_time)
		return;
	self->touch_debounce_time = level.time + 1_sec;

	door_use(self->owner, other, other);
}

THINK(Think_CalcMoveSpeed) (edict_t *self) -> void
{
	edict_t *ent;
	float	 min;
	float	 time;
	float	 newspeed;
	float	 ratio;
	float	 dist;

	if (self->flags & FL_TEAMSLAVE)
		return; // only the team master does this

	// find the smallest distance any member of the team will be moving
	min = fabsf(self->moveinfo.distance);
	for (ent = self->teamchain; ent; ent = ent->teamchain)
	{
		dist = fabsf(ent->moveinfo.distance);
		if (dist < min)
			min = dist;
	}

	time = min / self->moveinfo.speed;

	// adjust speeds so they will all complete at the same time
	for (ent = self; ent; ent = ent->teamchain)
	{
		newspeed = fabsf(ent->moveinfo.distance) / time;
		ratio = newspeed / ent->moveinfo.speed;
		if (ent->moveinfo.accel == ent->moveinfo.speed)
			ent->moveinfo.accel = newspeed;
		else
			ent->moveinfo.accel *= ratio;
		if (ent->moveinfo.decel == ent->moveinfo.speed)
			ent->moveinfo.decel = newspeed;
		else
			ent->moveinfo.decel *= ratio;
		ent->moveinfo.speed = newspeed;
	}
}

THINK(Think_SpawnDoorTrigger) (edict_t *ent) -> void
{
	edict_t *other;
	vec3_t	 mins, maxs;

	if (ent->flags & FL_TEAMSLAVE)
		return; // only the team leader spawns a trigger

	mins = ent->absmin;
	maxs = ent->absmax;

	for (other = ent->teamchain; other; other = other->teamchain)
	{
		AddPointToBounds(other->absmin, mins, maxs);
		AddPointToBounds(other->absmax, mins, maxs);
	}

	// expand
	mins[0] -= 60;
	mins[1] -= 60;
	maxs[0] += 60;
	maxs[1] += 60;

	other = G_Spawn();
	other->mins = mins;
	other->maxs = maxs;
	other->owner = ent;
	other->solid = SOLID_TRIGGER;
	other->movetype = MOVETYPE_NONE;
	other->touch = Touch_DoorTrigger;
	gi.linkentity(other);

	Think_CalcMoveSpeed(ent);
}

MOVEINFO_BLOCKED(door_blocked) (edict_t *self, edict_t *other) -> void
{
	edict_t *ent;

	if (!(other->svflags & SVF_MONSTER) && (!other->client))
	{
		// give it a chance to go away on it's own terms (like gibs)
		T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, DAMAGE_NONE, MOD_CRUSH);
		// if it's still there, nuke it
		if (other && other->inuse)
			BecomeExplosion1(other);
		return;
	}
	
	if (self->dmg && !(level.time < self->touch_debounce_time))
	{
		self->touch_debounce_time = level.time + 10_hz;
		T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, DAMAGE_NONE, MOD_CRUSH);
	}

	// [Paril-KEX] don't allow wait -1 doors to return
	if (self->spawnflags.has(SPAWNFLAG_DOOR_CRUSHER) || self->wait == -1)
		return;

	// if a door has a negative wait, it would never come back if blocked,
	// so let it just squash the object to death real fast
	if (self->moveinfo.wait >= 0)
	{
		if (self->moveinfo.state == STATE_DOWN)
		{
			for (ent = self->teammaster; ent; ent = ent->teamchain)
				door_go_up(ent, ent->activator);
		}
		else
		{
			for (ent = self->teammaster; ent; ent = ent->teamchain)
				door_go_down(ent);
		}
	}
}

DIE(door_killed) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	edict_t *ent;

	for (ent = self->teammaster; ent; ent = ent->teamchain)
	{
		ent->health = ent->max_health;
		ent->takedamage = false;
	}
	door_use(self->teammaster, attacker, attacker);
}

TOUCH(door_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	if (!other->client)
		return;

	if (level.time < self->touch_debounce_time)
		return;
	self->touch_debounce_time = level.time + 5_sec;

	gi.LocCenter_Print(other, "{}", self->message);
	gi.sound(other, CHAN_AUTO, gi.soundindex("misc/talk1.wav"), 1, ATTN_NORM, 0);
}

THINK(Think_DoorActivateAreaPortal) (edict_t *ent) -> void
{
	door_use_areaportals(ent, true);

	if (ent->health || ent->targetname)
		Think_CalcMoveSpeed(ent);
	else
		Think_SpawnDoorTrigger(ent);
}

void SP_func_door(edict_t *ent)
{
	vec3_t abs_movedir;

	if (ent->sounds != 1)
		G_SetMoveinfoSounds(ent, "doors/dr1_strt.wav", "doors/dr1_mid.wav", "doors/dr1_end.wav");
	else
		G_SetMoveinfoSounds(ent, nullptr, nullptr, nullptr);

	// [Paril-KEX] for rhangar1 doors
	if (!st.was_key_specified("attenuation"))
		ent->attenuation = ATTN_STATIC;
	else
	{
		if (ent->attenuation == -1)
		{
			ent->s.loop_attenuation = ATTN_LOOP_NONE;
			ent->attenuation = ATTN_NONE;
		}
		else
		{
			ent->s.loop_attenuation = ent->attenuation;
		}
	}

	G_SetMovedir(ent->s.angles, ent->movedir);
	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_BSP;
	ent->svflags |= SVF_DOOR;
	gi.setmodel(ent, ent->model);

	ent->moveinfo.blocked = door_blocked;
	ent->use = door_use;

	if (!ent->speed)
		ent->speed = 100;
	if (deathmatch->integer)
		ent->speed *= 2;

	if (!ent->accel)
		ent->accel = ent->speed;
	if (!ent->decel)
		ent->decel = ent->speed;

	if (!ent->wait)
		ent->wait = 3;
	if (!st.lip)
		st.lip = 8;
	if (!ent->dmg)
		ent->dmg = 2;

	// calculate second position
	ent->pos1 = ent->s.origin;
	abs_movedir[0] = fabsf(ent->movedir[0]);
	abs_movedir[1] = fabsf(ent->movedir[1]);
	abs_movedir[2] = fabsf(ent->movedir[2]);
	ent->moveinfo.distance =
		abs_movedir[0] * ent->size[0] + abs_movedir[1] * ent->size[1] + abs_movedir[2] * ent->size[2] - st.lip;
	ent->pos2 = ent->pos1 + (ent->movedir * ent->moveinfo.distance);

	// if it starts open, switch the positions
	if (ent->spawnflags.has(SPAWNFLAG_DOOR_START_OPEN))
	{
		ent->s.origin = ent->pos2;
		ent->pos2 = ent->pos1;
		ent->pos1 = ent->s.origin;
	}

	ent->moveinfo.state = STATE_BOTTOM;

	if (ent->health)
	{
		ent->takedamage = true;
		ent->die = door_killed;
		ent->max_health = ent->health;
	}
	else if (ent->targetname)
	{
		if (ent->message)
		{
			gi.soundindex("misc/talk.wav");
			ent->touch = door_touch;
		}
		ent->flags |= FL_LOCKED;
	}

	ent->moveinfo.speed = ent->speed;
	ent->moveinfo.accel = ent->accel;
	ent->moveinfo.decel = ent->decel;
	ent->moveinfo.wait = ent->wait;
	ent->moveinfo.start_origin = ent->pos1;
	ent->moveinfo.start_angles = ent->s.angles;
	ent->moveinfo.end_origin = ent->pos2;
	ent->moveinfo.end_angles = ent->s.angles;

	if (ent->spawnflags.has(SPAWNFLAG_DOOR_ANIMATED))
		ent->s.effects |= EF_ANIM_ALL;
	if (ent->spawnflags.has(SPAWNFLAG_DOOR_ANIMATED_FAST))
		ent->s.effects |= EF_ANIM_ALLFAST;

	// to simplify logic elsewhere, make non-teamed doors into a team of one
	if (!ent->team)
		ent->teammaster = ent;

	gi.linkentity(ent);

	ent->nextthink = level.time + FRAME_TIME_S;
	
	if (ent->spawnflags.has(SPAWNFLAG_DOOR_START_OPEN))
		ent->think = Think_DoorActivateAreaPortal;
	else if (ent->health || ent->targetname)
		ent->think = Think_CalcMoveSpeed;
	else
		ent->think = Think_SpawnDoorTrigger;
}

// PGM
USE(Door_Activate) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	self->use = nullptr;

	if (self->health)
	{
		self->takedamage = true;
		self->die = door_killed;
		self->max_health = self->health;
	}

	if (self->health)
		self->think = Think_CalcMoveSpeed;
	else
		self->think = Think_SpawnDoorTrigger;
	self->nextthink = level.time + FRAME_TIME_S;
}
// PGM

/*QUAKED func_door_rotating (0 .5 .8) ? START_OPEN REVERSE CRUSHER NOMONSTER ANIMATED TOGGLE X_AXIS Y_AXIS NOT_EASY NOT_MEDIUM NOT_HARD NOT_DM NOT_COOP RESERVED1 COOP_ONLY RESERVED2 INACTIVE SAFE_OPEN
TOGGLE causes the door to wait in both the start and end states for a trigger event.

START_OPEN	the door to moves to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not useful for touch or takedamage doors).
NOMONSTER	monsters will not trigger this door

You need to have an origin brush as part of this entity.  The center of that brush will be
the point around which it is rotated. It will rotate around the Z axis by default.  You can
check either the X_AXIS or Y_AXIS box to change that.

"distance" is how many degrees the door will be rotated.
"speed" determines how fast the door moves; default value is 100.
"accel" if specified,is how much the rotation speed will increase each .1 sec. (default: no accel)

REVERSE will cause the door to rotate in the opposite direction.
INACTIVE will cause the door to be inactive until triggered.
SAFE_OPEN will cause the door to open in reverse if you are on the `angles` side of the door.

"message"	is printed when the door is touched if it is a trigger door and it hasn't been fired yet
"angle"		determines the opening direction
"targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
"health"	if set, door must be shot open
"speed"		movement speed (100 default)
"wait"		wait before returning (3 default, -1 = never return)
"dmg"		damage to inflict when blocked (2 default)
"sounds"
1)	silent
2)	light
3)	medium
4)	heavy
*/

void SP_func_door_rotating(edict_t *ent)
{
	if (ent->spawnflags.has(SPAWNFLAG_DOOR_ROTATING_SAFE_OPEN))
		G_SetMovedir(ent->s.angles, ent->moveinfo.dir);

	ent->s.angles = {};

	// set the axis of rotation
	ent->movedir = {};
	if (ent->spawnflags.has(SPAWNFLAG_DOOR_ROTATING_X_AXIS))
		ent->movedir[2] = 1.0;
	else if (ent->spawnflags.has(SPAWNFLAG_DOOR_ROTATING_Y_AXIS))
		ent->movedir[0] = 1.0;
	else // Z_AXIS
		ent->movedir[1] = 1.0;

	// check for reverse rotation
	if (ent->spawnflags.has(SPAWNFLAG_DOOR_REVERSE))
		ent->movedir = -ent->movedir;

	if (!st.distance)
	{
		gi.Com_PrintFmt("{}: no distance set\n", *ent);
		st.distance = 90;
	}

	ent->pos1 = ent->s.angles;
	ent->pos2 = ent->s.angles + (ent->movedir * st.distance);
	ent->pos3 = ent->s.angles + (ent->movedir * -st.distance);
	ent->moveinfo.distance = (float) st.distance;

	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_BSP;
	ent->svflags |= SVF_DOOR;
	gi.setmodel(ent, ent->model);

	ent->moveinfo.blocked = door_blocked;
	ent->use = door_use;

	if (!ent->speed)
		ent->speed = 100;
	if (!ent->accel)
		ent->accel = ent->speed;
	if (!ent->decel)
		ent->decel = ent->speed;

	if (!ent->wait)
		ent->wait = 3;
	if (!ent->dmg)
		ent->dmg = 2;

	if (ent->sounds != 1)
		G_SetMoveinfoSounds(ent, "doors/dr1_strt.wav", "doors/dr1_mid.wav", "doors/dr1_end.wav");
	else
		G_SetMoveinfoSounds(ent, nullptr, nullptr, nullptr);

	// [Paril-KEX] for rhangar1 doors
	if (!st.was_key_specified("attenuation"))
		ent->attenuation = ATTN_STATIC;
	else
	{
		if (ent->attenuation == -1)
		{
			ent->s.loop_attenuation = ATTN_LOOP_NONE;
			ent->attenuation = ATTN_NONE;
		}
		else
		{
			ent->s.loop_attenuation = ent->attenuation;
		}
	}

	// if it starts open, switch the positions
	if (ent->spawnflags.has(SPAWNFLAG_DOOR_START_OPEN))
	{
		if (ent->spawnflags.has(SPAWNFLAG_DOOR_ROTATING_SAFE_OPEN))
		{
			ent->spawnflags &= ~SPAWNFLAG_DOOR_ROTATING_SAFE_OPEN;
			gi.Com_PrintFmt("{}: SAFE_OPEN is not compatible with START_OPEN\n", *ent);
		}

		ent->s.angles = ent->pos2;
		ent->pos2 = ent->pos1;
		ent->pos1 = ent->s.angles;
		ent->movedir = -ent->movedir;
	}

	if (ent->health)
	{
		ent->takedamage = true;
		ent->die = door_killed;
		ent->max_health = ent->health;
	}

	if (ent->targetname && ent->message)
	{
		gi.soundindex("misc/talk.wav");
		ent->touch = door_touch;
	}

	ent->moveinfo.state = STATE_BOTTOM;
	ent->moveinfo.speed = ent->speed;
	ent->moveinfo.accel = ent->accel;
	ent->moveinfo.decel = ent->decel;
	ent->moveinfo.wait = ent->wait;
	ent->moveinfo.start_origin = ent->s.origin;
	ent->moveinfo.start_angles = ent->pos1;
	ent->moveinfo.end_origin = ent->s.origin;
	ent->moveinfo.end_angles = ent->pos2;
	ent->moveinfo.end_angles_reversed = ent->pos3;

	if (ent->spawnflags.has(SPAWNFLAG_DOOR_ANIMATED))
		ent->s.effects |= EF_ANIM_ALL;

	// to simplify logic elsewhere, make non-teamed doors into a team of one
	if (!ent->team)
		ent->teammaster = ent;

	gi.linkentity(ent);

	ent->nextthink = level.time + FRAME_TIME_S;
	if (ent->health || ent->targetname)
		ent->think = Think_CalcMoveSpeed;
	else
		ent->think = Think_SpawnDoorTrigger;

	// PGM
	if (ent->spawnflags.has(SPAWNFLAG_DOOR_ROTATING_INACTIVE))
	{
		ent->takedamage = false;
		ent->die = nullptr;
		ent->think = nullptr;
		ent->nextthink = 0_ms;
		ent->use = Door_Activate;
	}
	// PGM
}

MOVEINFO_BLOCKED(smart_water_blocked) (edict_t *self, edict_t *other) -> void
{
	if (!(other->svflags & SVF_MONSTER) && (!other->client))
	{
		// give it a chance to go away on it's own terms (like gibs)
		T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, DAMAGE_NONE, MOD_LAVA);
		// if it's still there, nuke it
		if (other && other->inuse && other->solid) // PGM
			BecomeExplosion1(other);
		return;
	}

	T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, 100, 1, DAMAGE_NONE, MOD_LAVA);
}

/*QUAKED func_water (0 .5 .8) ? START_OPEN SMART
func_water is a moveable water brush.  It must be targeted to operate.  Use a non-water texture at your own risk.

START_OPEN causes the water to move to its destination when spawned and operate in reverse.

SMART causes the water to adjust its speed depending on distance to player.
(speed = distance/accel, min 5, max self->speed)
"accel"		for smart water, the divisor to determine water speed. default 20 (smaller = faster)

"health"	maximum height of this water brush
"angle"		determines the opening direction (up or down only)
"speed"		movement speed (25 default)
"wait"		wait before returning (-1 default, -1 = TOGGLE)
"lip"		lip remaining at end of move (0 default)
"sounds"	(yes, these need to be changed)
0)	no sound
1)	water
2)	lava
*/

void SP_func_water(edict_t *self)
{
	vec3_t abs_movedir;

	G_SetMovedir(self->s.angles, self->movedir);
	self->movetype = MOVETYPE_PUSH;
	self->solid = SOLID_BSP;
	gi.setmodel(self, self->model);

	switch (self->sounds)
	{
	default:
		G_SetMoveinfoSounds(self, nullptr, nullptr, nullptr);
		break;

	case 1: // water
	case 2: // lava
		G_SetMoveinfoSounds(self, "world/mov_watr.wav", nullptr, "world/stp_watr.wav");
		break;
	}

	self->attenuation = ATTN_STATIC;

	// calculate second position
	self->pos1 = self->s.origin;
	abs_movedir[0] = fabsf(self->movedir[0]);
	abs_movedir[1] = fabsf(self->movedir[1]);
	abs_movedir[2] = fabsf(self->movedir[2]);
	self->moveinfo.distance =
		abs_movedir[0] * self->size[0] + abs_movedir[1] * self->size[1] + abs_movedir[2] * self->size[2] - st.lip;
	self->pos2 = self->pos1 + (self->movedir * self->moveinfo.distance);

	// if it starts open, switch the positions
	if (self->spawnflags.has(SPAWNFLAG_DOOR_START_OPEN))
	{
		self->s.origin = self->pos2;
		self->pos2 = self->pos1;
		self->pos1 = self->s.origin;
	}

	self->moveinfo.start_origin = self->pos1;
	self->moveinfo.start_angles = self->s.angles;
	self->moveinfo.end_origin = self->pos2;
	self->moveinfo.end_angles = self->s.angles;

	self->moveinfo.state = STATE_BOTTOM;

	if (!self->speed)
		self->speed = 25;
	self->moveinfo.accel = self->moveinfo.decel = self->moveinfo.speed = self->speed;

	// ROGUE
	if (self->spawnflags.has(SPAWNFLAG_WATER_SMART)) // smart water
	{
		// this is actually the divisor of the lowest player's distance to determine speed.
		// self->speed then becomes the cap of the speed.
		if (!self->accel)
			self->accel = 20;
		self->moveinfo.blocked = smart_water_blocked;
	}
	// ROGUE

	if (!self->wait)
		self->wait = -1;
	self->moveinfo.wait = self->wait;

	self->use = door_use;

	if (self->wait == -1)
		self->spawnflags |= SPAWNFLAG_DOOR_TOGGLE;

	gi.linkentity(self);
}

constexpr spawnflags_t SPAWNFLAG_TRAIN_TOGGLE = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TRAIN_BLOCK_STOPS = 4_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TRAIN_FIX_OFFSET = 16_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TRAIN_USE_ORIGIN = 32_spawnflag;

/*QUAKED func_train (0 .5 .8) ? START_ON TOGGLE BLOCK_STOPS MOVE_TEAMCHAIN FIX_OFFSET USE_ORIGIN
Trains are moving platforms that players can ride.
The targets origin specifies the min point of the train at each corner.
The train spawns at the first target it is pointing at.
If the train is the target of a button or trigger, it will not begin moving until activated.
speed	default 100
dmg		default	2
noise	looping sound to play when the train is in motion

To have other entities move with the train, set all the piece's team value to the same thing. They will move in unison.
*/
void train_next(edict_t *self);

MOVEINFO_BLOCKED(train_blocked) (edict_t *self, edict_t *other) -> void
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

	if (level.time < self->touch_debounce_time)
		return;

	if (!self->dmg)
		return;
	self->touch_debounce_time = level.time + 500_ms;
	T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, DAMAGE_NONE, MOD_CRUSH);
}

MOVEINFO_ENDFUNC(train_wait) (edict_t *self) -> void
{
	if (self->target_ent->pathtarget)
	{
		const char	 *savetarget;
		edict_t *ent;

		ent = self->target_ent;
		savetarget = ent->target;
		ent->target = ent->pathtarget;
		G_UseTargets(ent, self->activator);
		ent->target = savetarget;

		// make sure we didn't get killed by a killtarget
		if (!self->inuse)
			return;
	}

	if (self->moveinfo.wait)
	{
		if (self->moveinfo.wait > 0)
		{
			self->nextthink = level.time + gtime_t::from_sec(self->moveinfo.wait);
			self->think = train_next;
		}
		else if (self->spawnflags.has(SPAWNFLAG_TRAIN_TOGGLE)) // && wait < 0
		{
			// PMM - clear target_ent, let train_next get called when we get used
			//			train_next (self);
			self->target_ent = nullptr;
			// pmm
			self->spawnflags &= ~SPAWNFLAG_TRAIN_START_ON;
			self->velocity = {};
			self->nextthink = 0_ms;
		}

		if (!(self->flags & FL_TEAMSLAVE))
		{
			if (self->moveinfo.sound_end)
				gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, self->moveinfo.sound_end, 1, ATTN_STATIC, 0);
		}
		self->s.sound = 0;
	}
	else
	{
		train_next(self);
	}
}

// PGM
MOVEINFO_ENDFUNC(train_piece_wait) (edict_t *self) -> void
{
}
// PGM

THINK(train_next) (edict_t *self) -> void
{
	edict_t *ent;
	vec3_t	 dest;
	bool	 first;

	first = true;
again:
	if (!self->target)
	{
		self->s.sound = 0;
		return;
	}

	ent = G_PickTarget(self->target);
	if (!ent)
	{
		gi.Com_PrintFmt("{}: train_next: bad target {}\n", *self, self->target);
		return;
	}

	self->target = ent->target;

	// check for a teleport path_corner
	if (ent->spawnflags.has(SPAWNFLAG_PATH_CORNER_TELEPORT))
	{
		if (!first)
		{
			gi.Com_PrintFmt("{}: connected teleport path_corners\n", *ent);
			return;
		}
		first = false;

		if (self->spawnflags.has(SPAWNFLAG_TRAIN_USE_ORIGIN))
			self->s.origin = ent->s.origin;
		else
		{
			self->s.origin = ent->s.origin - self->mins;

			if (self->spawnflags.has(SPAWNFLAG_TRAIN_FIX_OFFSET))
				self->s.origin -= vec3_t{1.f, 1.f, 1.f};
		}

		self->s.old_origin = self->s.origin;
		self->s.event = EV_OTHER_TELEPORT;
		gi.linkentity(self);
		goto again;
	}

	// PGM
	if (ent->speed)
	{
		self->speed = ent->speed;
		self->moveinfo.speed = ent->speed;
		if (ent->accel)
			self->moveinfo.accel = ent->accel;
		else
			self->moveinfo.accel = ent->speed;
		if (ent->decel)
			self->moveinfo.decel = ent->decel;
		else
			self->moveinfo.decel = ent->speed;
		self->moveinfo.current_speed = 0;
	}
	// PGM

	self->moveinfo.wait = ent->wait;
	self->target_ent = ent;

	if (!(self->flags & FL_TEAMSLAVE))
	{
		if (self->moveinfo.sound_start)
			gi.sound(self, CHAN_NO_PHS_ADD | CHAN_VOICE, self->moveinfo.sound_start, 1, ATTN_STATIC, 0);
	}

	self->s.sound = self->moveinfo.sound_middle;
	
	if (self->spawnflags.has(SPAWNFLAG_TRAIN_USE_ORIGIN))
		dest = ent->s.origin;
	else
	{
		dest = ent->s.origin - self->mins;

		if (self->spawnflags.has(SPAWNFLAG_TRAIN_FIX_OFFSET))
			dest -= vec3_t{1.f, 1.f, 1.f};
	}

	self->moveinfo.state = STATE_TOP;
	self->moveinfo.start_origin = self->s.origin;
	self->moveinfo.end_origin = dest;
	Move_Calc(self, dest, train_wait);
	self->spawnflags |= SPAWNFLAG_TRAIN_START_ON;

	// PGM
	if (self->spawnflags.has(SPAWNFLAG_TRAIN_MOVE_TEAMCHAIN))
	{
		edict_t *e;
		vec3_t	 dir, dst;

		dir = dest - self->s.origin;
		for (e = self->teamchain; e; e = e->teamchain)
		{
			dst = dir + e->s.origin;
			e->moveinfo.start_origin = e->s.origin;
			e->moveinfo.end_origin = dst;

			e->moveinfo.state = STATE_TOP;
			e->speed = self->speed;
			e->moveinfo.speed = self->moveinfo.speed;
			e->moveinfo.accel = self->moveinfo.accel;
			e->moveinfo.decel = self->moveinfo.decel;
			e->movetype = MOVETYPE_PUSH;
			Move_Calc(e, dst, train_piece_wait);
		}
	}
	// PGM
}

void train_resume(edict_t *self)
{
	edict_t *ent;
	vec3_t	 dest;

	ent = self->target_ent;
	
	if (self->spawnflags.has(SPAWNFLAG_TRAIN_USE_ORIGIN))
		dest = ent->s.origin;
	else
	{
		dest = ent->s.origin - self->mins;

		if (self->spawnflags.has(SPAWNFLAG_TRAIN_FIX_OFFSET))
			dest -= vec3_t{1.f, 1.f, 1.f};
	}

	self->s.sound = self->moveinfo.sound_middle;

	self->moveinfo.state = STATE_TOP;
	self->moveinfo.start_origin = self->s.origin;
	self->moveinfo.end_origin = dest;
	Move_Calc(self, dest, train_wait);
	self->spawnflags |= SPAWNFLAG_TRAIN_START_ON;
}

THINK(func_train_find) (edict_t *self) -> void
{
	edict_t *ent;

	if (!self->target)
	{
		gi.Com_PrintFmt("{}: train_find: no target\n", *self);
		return;
	}
	ent = G_PickTarget(self->target);
	if (!ent)
	{
		gi.Com_PrintFmt("{}: train_find: target {} not found\n", *self, self->target);
		return;
	}
	self->target = ent->target;

	if (self->spawnflags.has(SPAWNFLAG_TRAIN_USE_ORIGIN))
		self->s.origin = ent->s.origin;
	else
	{
		self->s.origin = ent->s.origin - self->mins;

		if (self->spawnflags.has(SPAWNFLAG_TRAIN_FIX_OFFSET))
			self->s.origin -= vec3_t{1.f, 1.f, 1.f};
	}

	gi.linkentity(self);

	// if not triggered, start immediately
	if (!self->targetname)
		self->spawnflags |= SPAWNFLAG_TRAIN_START_ON;

	if (self->spawnflags.has(SPAWNFLAG_TRAIN_START_ON))
	{
		self->nextthink = level.time + FRAME_TIME_S;
		self->think = train_next;
		self->activator = self;
	}
}

USE(train_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	self->activator = activator;

	if (self->spawnflags.has(SPAWNFLAG_TRAIN_START_ON))
	{
		if (!self->spawnflags.has(SPAWNFLAG_TRAIN_TOGGLE))
			return;
		self->spawnflags &= ~SPAWNFLAG_TRAIN_START_ON;
		self->velocity = {};
		self->nextthink = 0_ms;
	}
	else
	{
		if (self->target_ent)
			train_resume(self);
		else
			train_next(self);
	}
}

void SP_func_train(edict_t *self)
{
	self->movetype = MOVETYPE_PUSH;

	self->s.angles = {};
	self->moveinfo.blocked = train_blocked;
	if (self->spawnflags.has(SPAWNFLAG_TRAIN_BLOCK_STOPS))
		self->dmg = 0;
	else
	{
		if (!self->dmg)
			self->dmg = 100;
	}
	self->solid = SOLID_BSP;
	gi.setmodel(self, self->model);

	if (st.noise)
	{
		self->moveinfo.sound_middle = gi.soundindex(st.noise);

		// [Paril-KEX] for rhangar1 doors
		if (!st.was_key_specified("attenuation"))
			self->attenuation = ATTN_STATIC;
		else
		{
			if (self->attenuation == -1)
			{
				self->s.loop_attenuation = ATTN_LOOP_NONE;
				self->attenuation = ATTN_NONE;
			}
			else
			{
				self->s.loop_attenuation = self->attenuation;
			}
		}
	}

	if (!self->speed)
		self->speed = 100;

	self->moveinfo.speed = self->speed;
	self->moveinfo.accel = self->moveinfo.decel = self->moveinfo.speed;

	self->use = train_use;

	gi.linkentity(self);

	if (self->target)
	{
		// start trains on the second frame, to make sure their targets have had
		// a chance to spawn
		self->nextthink = level.time + FRAME_TIME_S;
		self->think = func_train_find;
	}
	else
	{
		gi.Com_PrintFmt("{}: no target\n", *self);
	}
}

/*QUAKED trigger_elevator (0.3 0.1 0.6) (-8 -8 -8) (8 8 8)
 */
USE(trigger_elevator_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	edict_t *target;

	if (self->movetarget->nextthink)
		return;

	if (!other->pathtarget)
	{
		gi.Com_PrintFmt("{}: elevator used with no pathtarget\n", *self);
		return;
	}

	target = G_PickTarget(other->pathtarget);
	if (!target)
	{
		gi.Com_PrintFmt("{}: elevator used with bad pathtarget: {}\n", *self, other->pathtarget);
		return;
	}

	self->movetarget->target_ent = target;
	train_resume(self->movetarget);
}

THINK(trigger_elevator_init) (edict_t *self) -> void
{
	if (!self->target)
	{
		gi.Com_PrintFmt("{}: has no target\n", *self);
		return;
	}
	self->movetarget = G_PickTarget(self->target);
	if (!self->movetarget)
	{
		gi.Com_PrintFmt("{}: unable to find target {}\n", *self, self->target);
		return;
	}
	if (strcmp(self->movetarget->classname, "func_train") != 0)
	{
		gi.Com_PrintFmt("{}: target {} is not a train\n", *self, self->target);
		return;
	}

	self->use = trigger_elevator_use;
	self->svflags = SVF_NOCLIENT;
}

void SP_trigger_elevator(edict_t *self)
{
	self->think = trigger_elevator_init;
	self->nextthink = level.time + FRAME_TIME_S;
}

/*QUAKED func_timer (0.3 0.1 0.6) (-8 -8 -8) (8 8 8) START_ON
"wait"			base time between triggering all targets, default is 1
"random"		wait variance, default is 0

so, the basic time between firing is a random time between
(wait - random) and (wait + random)

"delay"			delay before first firing when turned on, default is 0

"pausetime"		additional delay used only the very first time
				and only if spawned with START_ON

These can used but not touched.
*/

constexpr spawnflags_t SPAWNFLAG_TIMER_START_ON = 1_spawnflag;

THINK(func_timer_think) (edict_t *self) -> void
{
	G_UseTargets(self, self->activator);
	self->nextthink = level.time + gtime_t::from_sec(self->wait + crandom() * self->random);
}

USE(func_timer_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	self->activator = activator;

	// if on, turn it off
	if (self->nextthink)
	{
		self->nextthink = 0_ms;
		return;
	}

	// turn it on
	if (self->delay)
		self->nextthink = level.time + gtime_t::from_sec(self->delay);
	else
		func_timer_think(self);
}

void SP_func_timer(edict_t *self)
{
	if (!self->wait)
		self->wait = 1.0;

	self->use = func_timer_use;
	self->think = func_timer_think;

	if (self->random >= self->wait)
	{
		self->random = self->wait - gi.frame_time_s;
		gi.Com_PrintFmt("{}: random >= wait\n", *self);
	}

	if (self->spawnflags.has(SPAWNFLAG_TIMER_START_ON))
	{
		self->nextthink = level.time + 1_sec + gtime_t::from_sec(st.pausetime + self->delay + self->wait + crandom() * self->random);
		self->activator = self;
	}

	self->svflags = SVF_NOCLIENT;
}

constexpr spawnflags_t SPAWNFLAG_CONVEYOR_START_ON = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAG_CONVEYOR_TOGGLE = 2_spawnflag;

/*QUAKED func_conveyor (0 .5 .8) ? START_ON TOGGLE
Conveyors are stationary brushes that move what's on them.
The brush should be have a surface with at least one current content enabled.
speed	default 100
*/

USE(func_conveyor_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	if (self->spawnflags.has(SPAWNFLAG_CONVEYOR_START_ON))
	{
		self->speed = 0;
		self->spawnflags &= ~SPAWNFLAG_CONVEYOR_START_ON;
	}
	else
	{
		self->speed = (float) self->count;
		self->spawnflags |= SPAWNFLAG_CONVEYOR_START_ON;
	}

	if (!self->spawnflags.has(SPAWNFLAG_CONVEYOR_TOGGLE))
		self->count = 0;
}

void SP_func_conveyor(edict_t *self)
{
	if (!self->speed)
		self->speed = 100;

	if (!self->spawnflags.has(SPAWNFLAG_CONVEYOR_START_ON))
	{
		self->count = (int) self->speed;
		self->speed = 0;
	}

	self->use = func_conveyor_use;

	gi.setmodel(self, self->model);
	self->solid = SOLID_BSP;
	gi.linkentity(self);
}

/*QUAKED func_door_secret (0 .5 .8) ? always_shoot 1st_left 1st_down
A secret door.  Slide back and then to the side.

open_once		doors never closes
1st_left		1st move is left of arrow
1st_down		1st move is down from arrow
always_shoot	door is shootebale even if targeted

"angle"		determines the direction
"dmg"		damage to inflic when blocked (default 2)
"wait"		how long to hold in the open position (default 5, -1 means hold)
*/

constexpr spawnflags_t SPAWNFLAG_SECRET_ALWAYS_SHOOT = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAG_SECRET_1ST_LEFT = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAG_SECRET_1ST_DOWN = 4_spawnflag;

void door_secret_move1(edict_t *self);
void door_secret_move2(edict_t *self);
void door_secret_move3(edict_t *self);
void door_secret_move4(edict_t *self);
void door_secret_move5(edict_t *self);
void door_secret_move6(edict_t *self);
void door_secret_done(edict_t *self);

USE(door_secret_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	// make sure we're not already moving
	if (self->s.origin)
		return;

	Move_Calc(self, self->pos1, door_secret_move1);
	door_use_areaportals(self, true);
}

MOVEINFO_ENDFUNC(door_secret_move1) (edict_t *self) -> void
{
	self->nextthink = level.time + 1_sec;
	self->think = door_secret_move2;
}

THINK(door_secret_move2) (edict_t *self) -> void
{
	Move_Calc(self, self->pos2, door_secret_move3);
}

MOVEINFO_ENDFUNC(door_secret_move3) (edict_t *self) -> void
{
	if (self->wait == -1)
		return;
	self->nextthink = level.time + gtime_t::from_sec(self->wait);
	self->think = door_secret_move4;
}

THINK(door_secret_move4) (edict_t *self) -> void
{
	Move_Calc(self, self->pos1, door_secret_move5);
}

MOVEINFO_ENDFUNC(door_secret_move5) (edict_t *self) -> void
{
	self->nextthink = level.time + 1_sec;
	self->think = door_secret_move6;
}

THINK(door_secret_move6) (edict_t *self) -> void
{
	Move_Calc(self, vec3_origin, door_secret_done);
}

MOVEINFO_ENDFUNC(door_secret_done) (edict_t *self) -> void
{
	if (!(self->targetname) || self->spawnflags.has(SPAWNFLAG_SECRET_ALWAYS_SHOOT))
	{
		self->health = 0;
		self->takedamage = true;
	}
	door_use_areaportals(self, false);
}

MOVEINFO_BLOCKED(door_secret_blocked) (edict_t *self, edict_t *other) -> void
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

	if (level.time < self->touch_debounce_time)
		return;
	self->touch_debounce_time = level.time + 500_ms;

	T_Damage(other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, DAMAGE_NONE, MOD_CRUSH);
}

DIE(door_secret_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	self->takedamage = false;
	door_secret_use(self, attacker, attacker);
}

void SP_func_door_secret(edict_t *ent)
{
	vec3_t forward, right, up;
	float  side;
	float  width;
	float  length;

	G_SetMoveinfoSounds(ent, "doors/dr1_strt.wav", "doors/dr1_mid.wav", "doors/dr1_end.wav");

	ent->attenuation = ATTN_STATIC;

	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_BSP;
	ent->svflags |= SVF_DOOR;
	gi.setmodel(ent, ent->model);

	ent->moveinfo.blocked = door_secret_blocked;
	ent->use = door_secret_use;

	if (!(ent->targetname) || ent->spawnflags.has(SPAWNFLAG_SECRET_ALWAYS_SHOOT))
	{
		ent->health = 0;
		ent->takedamage = true;
		ent->die = door_secret_die;
	}

	if (!ent->dmg)
		ent->dmg = 2;

	if (!ent->wait)
		ent->wait = 5;

	ent->moveinfo.accel = ent->moveinfo.decel = ent->moveinfo.speed = 50;

	// calculate positions
	AngleVectors(ent->s.angles, forward, right, up);
	ent->s.angles = {};
	side = 1.0f - (ent->spawnflags.has(SPAWNFLAG_SECRET_1ST_LEFT) ? 2 : 0);
	if (ent->spawnflags.has(SPAWNFLAG_SECRET_1ST_DOWN))
		width = fabsf(up.dot(ent->size));
	else
		width = fabsf(right.dot(ent->size));
	length = fabsf(forward.dot(ent->size));
	if (ent->spawnflags.has(SPAWNFLAG_SECRET_1ST_DOWN))
		ent->pos1 = ent->s.origin + (up * (-1 * width));
	else
		ent->pos1 = ent->s.origin + (right * (side * width));
	ent->pos2 = ent->pos1 + (forward * length);

	if (ent->health)
	{
		ent->takedamage = true;
		ent->die = door_killed;
		ent->max_health = ent->health;
	}
	else if (ent->targetname && ent->message)
	{
		gi.soundindex("misc/talk.wav");
		ent->touch = door_touch;
	}

	gi.linkentity(ent);
}

/*QUAKED func_killbox (1 0 0) ?
Kills everything inside when fired, irrespective of protection.
*/
constexpr spawnflags_t SPAWNFLAG_KILLBOX_DEADLY_COOP = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAG_KILLBOX_EXACT_COLLISION = 4_spawnflag;

USE(use_killbox) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	if (self->spawnflags.has(SPAWNFLAG_KILLBOX_DEADLY_COOP))
		level.deadly_kill_box = true;

	self->solid = SOLID_TRIGGER;
	gi.linkentity(self);

	KillBox(self, false, MOD_TELEFRAG, self->spawnflags.has(SPAWNFLAG_KILLBOX_EXACT_COLLISION));

	self->solid = SOLID_NOT;
	gi.linkentity(self);

	level.deadly_kill_box = false;
}

void SP_func_killbox(edict_t *ent)
{
	gi.setmodel(ent, ent->model);
	ent->use = use_killbox;
	ent->svflags = SVF_NOCLIENT;
}

/*QUAKED func_eye (0 1 0) ?
Camera-like eye that can track entities.
"pathtarget" point to an info_notnull (which gets freed after spawn) to automatically set
the eye_position
"target"/"killtarget"/"delay"/"message" target keys to fire when we first spot a player
"eye_position" manually set the eye position; note that this is in "forward right up" format, relative to
the origin of the brush and using the entity's angles
"radius" default 512, detection radius for entities
"speed" default 45, how fast, in degrees per second, we should move on each axis to reach the target
"vision_cone" default 0.5 for half cone; how wide the cone of vision should be (relative to their initial angles)
"wait" default 0, the amount of time to wait before returning to neutral angles
*/
constexpr spawnflags_t SPAWNFLAG_FUNC_EYE_FIRED_TARGETS = 17_spawnflag_bit; // internal use only

THINK(func_eye_think) (edict_t *self) -> void
{
	// find enemy to track
	float closest_dist = 0;
	edict_t *closest_player = nullptr;

	for (auto player : active_players())
	{
		vec3_t dir = player->s.origin - self->s.origin;
		float dist = dir.normalize();

		if (dir.dot(self->movedir) < self->yaw_speed)
			continue;

		if (dist >= self->dmg_radius)
			continue;

		if (!closest_player || dist < closest_dist)
		{
			closest_player = player;
			closest_dist = dist;
		}
	}

	self->enemy = closest_player;

	// tracking player
	vec3_t wanted_angles;

	vec3_t fwd, rgt, up;
	AngleVectors(self->s.angles, fwd, rgt, up);

	vec3_t eye_pos = self->s.origin;
	eye_pos += fwd * self->move_origin[0];
	eye_pos += rgt * self->move_origin[1];
	eye_pos += up * self->move_origin[2];
	
	if (self->enemy)
	{
		if (!(self->spawnflags & SPAWNFLAG_FUNC_EYE_FIRED_TARGETS))
		{
			G_UseTargets(self, self->enemy);
			self->spawnflags |= SPAWNFLAG_FUNC_EYE_FIRED_TARGETS;
		}

		vec3_t dir = (self->enemy->s.origin - eye_pos).normalized();
		wanted_angles = vectoangles(dir);

		self->s.frame = 2;
		self->timestamp = level.time + gtime_t::from_sec(self->wait);
	}
	else
	{
		if (self->timestamp <= level.time)
		{
			// return to neutral
			wanted_angles = self->move_angles;
			self->s.frame = 0;
		}
		else
			wanted_angles = self->s.angles;
	}
	
	for (int i = 0; i < 2; i++)
	{
		float current = anglemod(self->s.angles[i]);
		float ideal = wanted_angles[i];

		if (current == ideal)
			continue;

		float move = ideal - current;

		if (ideal > current)
		{
			if (move >= 180)
				move = move - 360;
		}
		else
		{
			if (move <= -180)
				move = move + 360;
		}
		if (move > 0)
		{
			if (move > self->speed)
				move = self->speed;
		}
		else
		{
			if (move < -self->speed)
				move = -self->speed;
		}

		self->s.angles[i] = anglemod(current + move);
	}

	self->nextthink = level.time + FRAME_TIME_S;
}

THINK(func_eye_setup) (edict_t *self) -> void
{
	edict_t *eye_pos = G_PickTarget(self->pathtarget);

	if (!eye_pos)
		gi.Com_PrintFmt("{}: bad target\n", *self);
	else
		self->move_origin = eye_pos->s.origin - self->s.origin;

	self->movedir = self->move_origin.normalized();

	self->think = func_eye_think;
	self->nextthink = level.time + 10_hz;
}

void SP_func_eye(edict_t *ent)
{
	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_BSP;
	gi.setmodel(ent, ent->model);

	if (!st.radius)
		ent->dmg_radius = 512;
	else
		ent->dmg_radius = st.radius;

	if (!ent->speed)
		ent->speed = 45;

	if (!ent->yaw_speed)
		ent->yaw_speed = 0.5f;

	ent->speed *= gi.frame_time_s;
	ent->move_angles = ent->s.angles;

	ent->wait = 1.0f;

	if (ent->pathtarget)
	{
		ent->think = func_eye_setup;
		ent->nextthink = level.time + 10_hz;
	}
	else
	{
		ent->think = func_eye_think;
		ent->nextthink = level.time + 10_hz;

		vec3_t right, up;
		AngleVectors(ent->move_angles, ent->movedir, right, up);

		vec3_t move_origin = ent->move_origin;
		ent->move_origin = ent->movedir * move_origin[0];
		ent->move_origin += right * move_origin[1];
		ent->move_origin += up * move_origin[2];
	}

	gi.linkentity(ent);
}
