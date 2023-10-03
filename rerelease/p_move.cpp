// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "q_std.h"

#define GAME_INCLUDE
#include "bg_local.h"

// [Paril-KEX] generic code to detect & fix a stuck object
stuck_result_t G_FixStuckObject_Generic(vec3_t &origin, const vec3_t &own_mins, const vec3_t &own_maxs, std::function<stuck_object_trace_fn_t> trace)
{
	if (!trace(origin, own_mins, own_maxs, origin).startsolid)
		return stuck_result_t::GOOD_POSITION;

	struct {
		float distance;
		vec3_t origin;
	} good_positions[6];
	size_t num_good_positions = 0;

	constexpr struct {
		std::array<int8_t, 3> normal;
		std::array<int8_t, 3> mins, maxs;
	} side_checks[] = {
		{ { 0, 0, 1 }, { -1, -1, 0 }, { 1, 1, 0 } },
		{ { 0, 0, -1 }, { -1, -1, 0 }, { 1, 1, 0 } },
		{ { 1, 0, 0 }, { 0, -1, -1 }, { 0, 1, 1 } },
		{ { -1, 0, 0 }, { 0, -1, -1 }, { 0, 1, 1 } },
		{ { 0, 1, 0 }, { -1, 0, -1 }, { 1, 0, 1 } },
		{ { 0, -1, 0 }, { -1, 0, -1 }, { 1, 0, 1 } },
	};

	for (size_t sn = 0; sn < q_countof(side_checks); sn++)
	{
		auto &side = side_checks[sn];
		vec3_t start = origin;
		vec3_t mins {}, maxs {};

		for (size_t n = 0; n < 3; n++)
		{
			if (side.normal[n] < 0)
				start[n] += own_mins[n];
			else if (side.normal[n] > 0)
				start[n] += own_maxs[n];

			if (side.mins[n] == -1)
				mins[n] = own_mins[n];
			else if (side.mins[n] == 1)
				mins[n] = own_maxs[n];

			if (side.maxs[n] == -1)
				maxs[n] = own_mins[n];
			else if (side.maxs[n] == 1)
				maxs[n] = own_maxs[n];
		}

		trace_t tr = trace(start, mins, maxs, start);

		int8_t needed_epsilon_fix = -1;
		int8_t needed_epsilon_dir;

		if (tr.startsolid)
		{
			for (size_t e = 0; e < 3; e++)
			{
				if (side.normal[e] != 0)
					continue;

				vec3_t ep_start = start;
				ep_start[e] += 1;

				tr = trace(ep_start, mins, maxs, ep_start);

				if (!tr.startsolid)
				{
					start = ep_start;
					needed_epsilon_fix = e;
					needed_epsilon_dir = 1;
					break;
				}

				ep_start[e] -= 2;
				tr = trace(ep_start, mins, maxs, ep_start);

				if (!tr.startsolid)
				{
					start = ep_start;
					needed_epsilon_fix = e;
					needed_epsilon_dir = -1;
					break;
				}
			}
		}

		// no good
		if (tr.startsolid)
			continue;

		vec3_t opposite_start = origin;
		auto &other_side = side_checks[sn ^ 1];

		for (size_t n = 0; n < 3; n++)
		{
			if (other_side.normal[n] < 0)
				opposite_start[n] += own_mins[n];
			else if (other_side.normal[n] > 0)
				opposite_start[n] += own_maxs[n];
		}

		if (needed_epsilon_fix >= 0)
			opposite_start[needed_epsilon_fix] += needed_epsilon_dir;

		// potentially a good side; start from our center, push back to the opposite side
		// to find how much clearance we have
		tr = trace(start, mins, maxs, opposite_start);

		// ???
		if (tr.startsolid)
			continue;

		// check the delta
		vec3_t end = tr.endpos;
		// push us very slightly away from the wall
		end += vec3_t{(float) side.normal[0], (float) side.normal[1], (float) side.normal[2]} * 0.125f;

		// calculate delta
		const vec3_t delta = end - opposite_start;
		vec3_t new_origin = origin + delta;

		if (needed_epsilon_fix >= 0)
			new_origin[needed_epsilon_fix] += needed_epsilon_dir;

		tr = trace(new_origin, own_mins, own_maxs, new_origin);

		// bad
		if (tr.startsolid)
			continue;

		good_positions[num_good_positions].origin = new_origin;
		good_positions[num_good_positions].distance = delta.lengthSquared();
		num_good_positions++;
	}

	if (num_good_positions)
	{
		std::sort(&good_positions[0], &good_positions[num_good_positions - 1], [](const auto &a, const auto &b) { return a.distance < b.distance; });

		origin = good_positions[0].origin;

		return stuck_result_t::FIXED;
	}

	return stuck_result_t::NO_GOOD_POSITION;
}

// all of the locals will be zeroed before each
// pmove, just to make damn sure we don't have
// any differences when running on client or server

struct pml_t
{
	vec3_t origin;	 // full float precision
	vec3_t velocity; // full float precision

	vec3_t forward, right, up;
	float  frametime;

	csurface_t *groundsurface;
	int			groundcontents;

	vec3_t previous_origin;
	vec3_t start_velocity;
};

pm_config_t pm_config;

pmove_t *pm;
pml_t	 pml;

// movement parameters
float pm_stopspeed = 100;
float pm_maxspeed = 300;
float pm_duckspeed = 100;
float pm_accelerate = 10;
float pm_wateraccelerate = 10;
float pm_friction = 6;
float pm_waterfriction = 1;
float pm_waterspeed = 400;
float pm_laddermod = 0.5f;

/*

  walking up a step should kill some velocity

*/

/*
==================
PM_ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
void PM_ClipVelocity(const vec3_t &in, const vec3_t &normal, vec3_t &out, float overbounce)
{
	float backoff;
	float change;
	int	  i;

	backoff = in.dot(normal) * overbounce;

	for (i = 0; i < 3; i++)
	{
		change = normal[i] * backoff;
		out[i] = in[i] - change;
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
	}
}

trace_t PM_Clip(const vec3_t &start, const vec3_t &mins, const vec3_t &maxs, const vec3_t &end, contents_t mask)
{
	return pm->clip(start, &mins, &maxs, end, mask);
}

trace_t PM_Trace(const vec3_t &start, const vec3_t &mins, const vec3_t &maxs, const vec3_t &end, contents_t mask = CONTENTS_NONE)
{
	if (pm->s.pm_type == PM_SPECTATOR)
		return PM_Clip(start, mins, maxs, end, MASK_SOLID);

	if (mask == CONTENTS_NONE)
	{
		if (pm->s.pm_type == PM_DEAD || pm->s.pm_type == PM_GIB)
			mask = MASK_DEADSOLID;
		else if (pm->s.pm_type == PM_SPECTATOR)
			mask = MASK_SOLID;
		else
			mask = MASK_PLAYERSOLID;

		if (pm->s.pm_flags & PMF_IGNORE_PLAYER_COLLISION)
			mask &= ~CONTENTS_PLAYER;
	}

	return pm->trace(start, &mins, &maxs, end, pm->player, mask);
}

// only here to satisfy pm_trace_t
inline trace_t PM_Trace_Auto(const vec3_t &start, const vec3_t &mins, const vec3_t &maxs, const vec3_t &end)
{
	return PM_Trace(start, mins, maxs, end);
}

/*
==================
PM_StepSlideMove

Each intersection will try to step over the obstruction instead of
sliding along it.

Returns a new origin, velocity, and contact entity
Does not modify any world state?
==================
*/
constexpr float	 MIN_STEP_NORMAL = 0.7f; // can't step up onto very steep slopes
constexpr size_t MAX_CLIP_PLANES = 5;

inline void PM_RecordTrace(touch_list_t &touch, trace_t &tr)
{
	if (touch.num == MAXTOUCH)
		return;

	for (size_t i = 0; i < touch.num; i++)
		if (touch.traces[i].ent == tr.ent)
			return;

	touch.traces[touch.num++] = tr;
}

// [Paril-KEX] made generic so you can run this without
// needing a pml/pm
void PM_StepSlideMove_Generic(vec3_t &origin, vec3_t &velocity, float frametime, const vec3_t &mins, const vec3_t &maxs, touch_list_t &touch, bool has_time, pm_trace_t trace_func)
{
	int		bumpcount, numbumps;
	vec3_t	dir;
	float	d;
	int		numplanes;
	vec3_t	planes[MAX_CLIP_PLANES];
	vec3_t	primal_velocity;
	int		i, j;
	trace_t trace;
	vec3_t	end;
	float	time_left;

	numbumps = 4;

	primal_velocity = velocity;
	numplanes = 0;

	time_left = frametime;

	for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		for (i = 0; i < 3; i++)
			end[i] = origin[i] + time_left * velocity[i];

		trace = trace_func(origin, mins, maxs, end);

		if (trace.allsolid)
		{						 // entity is trapped in another solid
			velocity[2] = 0; // don't build up falling damage
			
			// save entity for contact
			PM_RecordTrace(touch, trace);
			return;
		}

		// [Paril-KEX] experimental attempt to fix stray collisions on curved
		// surfaces; easiest to see on q2dm1 by running/jumping against the sides
		// of the curved map.
		if (trace.surface2)
		{
			vec3_t clipped_a, clipped_b;
			PM_ClipVelocity(velocity, trace.plane.normal, clipped_a, 1.01f);
			PM_ClipVelocity(velocity, trace.plane2.normal, clipped_b, 1.01f);

			bool better = false;

			for (int i = 0; i < 3; i++)
			{
				if (fabsf(clipped_a[i]) < fabsf(clipped_b[i]))
				{
					better = true;
					break;
				}
			}

			if (better)
			{
				trace.plane = trace.plane2;
				trace.surface = trace.surface2;
			}
		}

		if (trace.fraction > 0)
		{ // actually covered some distance
			origin = trace.endpos;
			numplanes = 0;
		}

		if (trace.fraction == 1)
			break; // moved the entire distance

		// save entity for contact
		PM_RecordTrace(touch, trace);

		time_left -= time_left * trace.fraction;

		// slide along this plane
		if (numplanes >= MAX_CLIP_PLANES)
		{ // this shouldn't really happen
			velocity = vec3_origin;
			break;
		}

		//
		// if this is the same plane we hit before, nudge origin
		// out along it, which fixes some epsilon issues with
		// non-axial planes (xswamp, q2dm1 sometimes...)
		//
		for (i = 0; i < numplanes; i++)
		{
			if (trace.plane.normal.dot(planes[i]) > 0.99f)
			{
				pml.origin.x += trace.plane.normal.x * 0.01f;
				pml.origin.y += trace.plane.normal.y * 0.01f;
				G_FixStuckObject_Generic(pml.origin, mins, maxs, trace_func);
				break;
			}
		}

		if (i < numplanes)
			continue;

		planes[numplanes] = trace.plane.normal;
		numplanes++;

		//
		// modify original_velocity so it parallels all of the clip planes
		//
		for (i = 0; i < numplanes; i++)
		{
			PM_ClipVelocity(velocity, planes[i], velocity, 1.01f);
			for (j = 0; j < numplanes; j++)
				if (j != i)
				{
					if (velocity.dot(planes[j]) < 0)
						break; // not ok
				}
			if (j == numplanes)
				break;
		}

		if (i != numplanes)
		{ // go along this plane
		}
		else
		{ // go along the crease
			if (numplanes != 2)
			{
				velocity = vec3_origin;
				break;
			}
			dir = planes[0].cross(planes[1]);
			d = dir.dot(velocity);
			velocity = dir * d;
		}

		//
		// if velocity is against the original velocity, stop dead
		// to avoid tiny oscillations in sloping corners
		//
		if (velocity.dot(primal_velocity) <= 0)
		{
			velocity = vec3_origin;
			break;
		}
	}

	if (has_time)
	{
		velocity = primal_velocity;
	}
}

inline void PM_StepSlideMove_()
{
	PM_StepSlideMove_Generic(pml.origin, pml.velocity, pml.frametime, pm->mins, pm->maxs, pm->touch, pm->s.pm_time, PM_Trace_Auto);
}

/*
==================
PM_StepSlideMove

==================
*/
void PM_StepSlideMove()
{
	vec3_t	start_o, start_v;
	vec3_t	down_o, down_v;
	trace_t trace;
	float	down_dist, up_dist;
	//	vec3_t		delta;
	vec3_t up, down;

	start_o = pml.origin;
	start_v = pml.velocity;

	PM_StepSlideMove_();

	down_o = pml.origin;
	down_v = pml.velocity;

	up = start_o;
	up[2] += STEPSIZE;

	trace = PM_Trace(start_o, pm->mins, pm->maxs, up);
	if (trace.allsolid)
		return; // can't step up

	float stepSize = trace.endpos[2] - start_o[2];

	// try sliding above
	pml.origin = trace.endpos;
	pml.velocity = start_v;

	PM_StepSlideMove_();

	// push down the final amount
	down = pml.origin;
	down[2] -= stepSize;

	// [Paril-KEX] jitspoe suggestion for stair clip fix; store
	// the old down position, and pick a better spot for downwards
	// trace if the start origin's Z position is lower than the down end pt.
	vec3_t original_down = down;

	if (start_o[2] < down[2])
		down[2] = start_o[2] - 1.f;

	trace = PM_Trace(pml.origin, pm->mins, pm->maxs, down);
	if (!trace.allsolid)
	{
		// [Paril-KEX] from above, do the proper trace now
		trace_t real_trace = PM_Trace(pml.origin, pm->mins, pm->maxs, original_down);
		pml.origin = real_trace.endpos;

		// only an upwards jump is a stair clip
		if (pml.velocity.z > 0.f)
		{
			pm->step_clip = true;
		}
	}

	up = pml.origin;

	// decide which one went farther
	down_dist = (down_o[0] - start_o[0]) * (down_o[0] - start_o[0]) + (down_o[1] - start_o[1]) * (down_o[1] - start_o[1]);
	up_dist = (up[0] - start_o[0]) * (up[0] - start_o[0]) + (up[1] - start_o[1]) * (up[1] - start_o[1]);

	if (down_dist > up_dist || trace.plane.normal[2] < MIN_STEP_NORMAL)
	{
		pml.origin = down_o;
		pml.velocity = down_v;
	}
	// [Paril-KEX] NB: this line being commented is crucial for ramp-jumps to work.
	// thanks to Jitspoe for pointing this one out.
	else// if (pm->s.pm_flags & PMF_ON_GROUND)
		//!! Special case
		// if we were walking along a plane, then we need to copy the Z over
		pml.velocity[2] = down_v[2];

	// Paril: step down stairs/slopes
	if ((pm->s.pm_flags & PMF_ON_GROUND) && !(pm->s.pm_flags & PMF_ON_LADDER) &&
		(pm->waterlevel < WATER_WAIST || (!(pm->cmd.buttons & BUTTON_JUMP) && pml.velocity.z <= 0)))
	{
		down = pml.origin;
		down[2] -= STEPSIZE;
		trace = PM_Trace(pml.origin, pm->mins, pm->maxs, down);
		if (trace.fraction < 1.f)
		{
			pml.origin = trace.endpos;
		}
	}
}

/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
void PM_Friction()
{
	float *vel;
	float  speed, newspeed, control;
	float  friction;
	float  drop;

	vel = &pml.velocity.x;

	speed = sqrt(vel[0] * vel[0] + vel[1] * vel[1] + vel[2] * vel[2]);
	if (speed < 1)
	{
		vel[0] = 0;
		vel[1] = 0;
		return;
	}

	drop = 0;

	// apply ground friction
	if ((pm->groundentity && pml.groundsurface && !(pml.groundsurface->flags & SURF_SLICK)) || (pm->s.pm_flags & PMF_ON_LADDER))
	{
		friction = pm_friction;
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control * friction * pml.frametime;
	}

	// apply water friction
	if (pm->waterlevel && !(pm->s.pm_flags & PMF_ON_LADDER))
		drop += speed * pm_waterfriction * (float) pm->waterlevel * pml.frametime;

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0)
	{
		newspeed = 0;
	}
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}

/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
void PM_Accelerate(const vec3_t &wishdir, float wishspeed, float accel)
{
	int	  i;
	float addspeed, accelspeed, currentspeed;

	currentspeed = pml.velocity.dot(wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = accel * pml.frametime * wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i = 0; i < 3; i++)
		pml.velocity[i] += accelspeed * wishdir[i];
}

void PM_AirAccelerate(const vec3_t &wishdir, float wishspeed, float accel)
{
	int	  i;
	float addspeed, accelspeed, currentspeed, wishspd = wishspeed;

	if (wishspd > 30)
		wishspd = 30;
	currentspeed = pml.velocity.dot(wishdir);
	addspeed = wishspd - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = accel * wishspeed * pml.frametime;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i = 0; i < 3; i++)
		pml.velocity[i] += accelspeed * wishdir[i];
}

/*
=============
PM_AddCurrents
=============
*/
void PM_AddCurrents(vec3_t &wishvel)
{
	vec3_t v;
	float  s;

	//
	// account for ladders
	//

	if (pm->s.pm_flags & PMF_ON_LADDER)
	{
		if (pm->cmd.buttons & (BUTTON_JUMP | BUTTON_CROUCH))
		{
			// [Paril-KEX]: if we're underwater, use full speed on ladders
			float ladder_speed = pm->waterlevel >= WATER_WAIST ? pm_maxspeed : 200;
			
			if (pm->cmd.buttons & BUTTON_JUMP)
				wishvel[2] = ladder_speed;
			else if (pm->cmd.buttons & BUTTON_CROUCH)
				wishvel[2] = -ladder_speed;
		}
		else if (pm->cmd.forwardmove)
		{
			// [Paril-KEX] clamp the speed a bit so we're not too fast
			float ladder_speed = std::clamp(pm->cmd.forwardmove, -200.f, 200.f);

			if (pm->cmd.forwardmove > 0)
			{
				if (pm->viewangles[PITCH] < 15)
					wishvel[2] = ladder_speed;
				else
					wishvel[2] = -ladder_speed;
			}
			// [Paril-KEX] allow using "back" arrow to go down on ladder
			else if (pm->cmd.forwardmove < 0)
			{
				// if we haven't touched ground yet, remove x/y so we don't
				// slide off of the ladder
				if (!pm->groundentity)
					wishvel[0] = wishvel[1] = 0;

				wishvel[2] = ladder_speed;
			}
		}
		else
			wishvel[2] = 0;

		// limit horizontal speed when on a ladder
		// [Paril-KEX] unless we're on the ground
		if (!pm->groundentity)
		{
			// [Paril-KEX] instead of left/right not doing anything,
			// have them move you perpendicular to the ladder plane
			if (pm->cmd.sidemove)
			{
				// clamp side speed so it's not jarring...
				float ladder_speed = std::clamp(pm->cmd.sidemove, -150.f, 150.f);

				if (pm->waterlevel < WATER_WAIST)
					ladder_speed *= pm_laddermod;

				// check for ladder
				vec3_t flatforward, spot;
				flatforward[0] = pml.forward[0];
				flatforward[1] = pml.forward[1];
				flatforward[2] = 0;
				flatforward.normalize();

				spot = pml.origin + (flatforward * 1);
				trace_t trace = PM_Trace(pml.origin, pm->mins, pm->maxs, spot, CONTENTS_LADDER);

				if (trace.fraction != 1.f && (trace.contents & CONTENTS_LADDER))
				{
					vec3_t right = trace.plane.normal.cross({ 0, 0, 1 });

					wishvel[0] = wishvel[1] = 0;
					wishvel += (right * -ladder_speed);
				}
			}
			else
			{
				if (wishvel[0] < -25)
					wishvel[0] = -25;
				else if (wishvel[0] > 25)
					wishvel[0] = 25;

				if (wishvel[1] < -25)
					wishvel[1] = -25;
				else if (wishvel[1] > 25)
					wishvel[1] = 25;
			}
		}
	}

	//
	// add water currents
	//

	if (pm->watertype & MASK_CURRENT)
	{
		v = {};

		if (pm->watertype & CONTENTS_CURRENT_0)
			v[0] += 1;
		if (pm->watertype & CONTENTS_CURRENT_90)
			v[1] += 1;
		if (pm->watertype & CONTENTS_CURRENT_180)
			v[0] -= 1;
		if (pm->watertype & CONTENTS_CURRENT_270)
			v[1] -= 1;
		if (pm->watertype & CONTENTS_CURRENT_UP)
			v[2] += 1;
		if (pm->watertype & CONTENTS_CURRENT_DOWN)
			v[2] -= 1;

		s = pm_waterspeed;
		if ((pm->waterlevel == WATER_FEET) && (pm->groundentity))
			s /= 2;

		wishvel += (v * s);
	}

	//
	// add conveyor belt velocities
	//

	if (pm->groundentity)
	{
		v = {};

		if (pml.groundcontents & CONTENTS_CURRENT_0)
			v[0] += 1;
		if (pml.groundcontents & CONTENTS_CURRENT_90)
			v[1] += 1;
		if (pml.groundcontents & CONTENTS_CURRENT_180)
			v[0] -= 1;
		if (pml.groundcontents & CONTENTS_CURRENT_270)
			v[1] -= 1;
		if (pml.groundcontents & CONTENTS_CURRENT_UP)
			v[2] += 1;
		if (pml.groundcontents & CONTENTS_CURRENT_DOWN)
			v[2] -= 1;

		wishvel += v * 100;
	}
}

/*
===================
PM_WaterMove

===================
*/
void PM_WaterMove()
{
	int	   i;
	vec3_t wishvel;
	float  wishspeed;
	vec3_t wishdir;

	//
	// user intentions
	//
	for (i = 0; i < 3; i++)
		wishvel[i] = pml.forward[i] * pm->cmd.forwardmove + pml.right[i] * pm->cmd.sidemove;

	if (!pm->cmd.forwardmove && !pm->cmd.sidemove &&
		!(pm->cmd.buttons & (BUTTON_JUMP | BUTTON_CROUCH)))
	{
		if (!pm->groundentity)
			wishvel[2] -= 60; // drift towards bottom
	}
	else
	{
		if (pm->cmd.buttons & BUTTON_CROUCH)
			wishvel[2] -= pm_waterspeed * 0.5f;
		else if (pm->cmd.buttons & BUTTON_JUMP)
			wishvel[2] += pm_waterspeed * 0.5f;
	}

	PM_AddCurrents(wishvel);

	wishdir = wishvel;
	wishspeed = wishdir.normalize();

	if (wishspeed > pm_maxspeed)
	{
		wishvel *= pm_maxspeed / wishspeed;
		wishspeed = pm_maxspeed;
	}
	wishspeed *= 0.5f;

	if ((pm->s.pm_flags & PMF_DUCKED) && wishspeed > pm_duckspeed)
	{
		wishvel *= pm_duckspeed / wishspeed;
		wishspeed = pm_duckspeed;
	}

	PM_Accelerate(wishdir, wishspeed, pm_wateraccelerate);

	PM_StepSlideMove();
}

/*
===================
PM_AirMove

===================
*/
void PM_AirMove()
{
	int	   i;
	vec3_t wishvel;
	float  fmove, smove;
	vec3_t wishdir;
	float  wishspeed;
	float  maxspeed;

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.sidemove;

	for (i = 0; i < 2; i++)
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	wishvel[2] = 0;

	PM_AddCurrents(wishvel);

	wishdir = wishvel;
	wishspeed = wishdir.normalize();

	//
	// clamp to server defined max speed
	//
	maxspeed = (pm->s.pm_flags & PMF_DUCKED) ? pm_duckspeed : pm_maxspeed;

	if (wishspeed > maxspeed)
	{
		wishvel *= maxspeed / wishspeed;
		wishspeed = maxspeed;
	}

	if (pm->s.pm_flags & PMF_ON_LADDER)
	{
		PM_Accelerate(wishdir, wishspeed, pm_accelerate);
		if (!wishvel[2])
		{
			if (pml.velocity[2] > 0)
			{
				pml.velocity[2] -= pm->s.gravity * pml.frametime;
				if (pml.velocity[2] < 0)
					pml.velocity[2] = 0;
			}
			else
			{
				pml.velocity[2] += pm->s.gravity * pml.frametime;
				if (pml.velocity[2] > 0)
					pml.velocity[2] = 0;
			}
		}
		PM_StepSlideMove();
	}
	else if (pm->groundentity)
	{						 // walking on ground
		pml.velocity[2] = 0; //!!! this is before the accel
		PM_Accelerate(wishdir, wishspeed, pm_accelerate);

		// PGM	-- fix for negative trigger_gravity fields
		//		pml.velocity[2] = 0;
		if (pm->s.gravity > 0)
			pml.velocity[2] = 0;
		else
			pml.velocity[2] -= pm->s.gravity * pml.frametime;
		// PGM

		if (!pml.velocity[0] && !pml.velocity[1])
			return;
		PM_StepSlideMove();
	}
	else
	{ // not on ground, so little effect on velocity
		if (pm_config.airaccel)
			PM_AirAccelerate(wishdir, wishspeed, pm_config.airaccel);
		else
			PM_Accelerate(wishdir, wishspeed, 1);

		// add gravity
		if (pm->s.pm_type != PM_GRAPPLE)
			pml.velocity[2] -= pm->s.gravity * pml.frametime;

		PM_StepSlideMove();
	}
}

inline void PM_GetWaterLevel(const vec3_t &position, water_level_t &level, contents_t &type)
{
	//
	// get waterlevel, accounting for ducking
	//
	level = WATER_NONE;
	type = CONTENTS_NONE;

	int32_t sample2 = (int) (pm->s.viewheight - pm->mins[2]);
	int32_t sample1 = sample2 / 2;

	vec3_t point = position;

	point[2] += pm->mins[2] + 1;

	contents_t cont = pm->pointcontents(point);

	if (cont & MASK_WATER)
	{
		type = cont;
		level = WATER_FEET;
		point[2] = pml.origin[2] + pm->mins[2] + sample1;
		cont = pm->pointcontents(point);
		if (cont & MASK_WATER)
		{
			level = WATER_WAIST;
			point[2] = pml.origin[2] + pm->mins[2] + sample2;
			cont = pm->pointcontents(point);
			if (cont & MASK_WATER)
				level = WATER_UNDER;
		}
	}
}

/*
=============
PM_CatagorizePosition
=============
*/
void PM_CatagorizePosition()
{
	vec3_t	   point;
	trace_t	   trace;

	// if the player hull point one unit down is solid, the player
	// is on ground

	// see if standing on something solid
	point[0] = pml.origin[0];
	point[1] = pml.origin[1];
	point[2] = pml.origin[2] - 0.25f;

	if (pml.velocity[2] > 180 || pm->s.pm_type == PM_GRAPPLE) //!!ZOID changed from 100 to 180 (ramp accel)
	{
		pm->s.pm_flags &= ~PMF_ON_GROUND;
		pm->groundentity = nullptr;
	}
	else
	{
		trace = PM_Trace(pml.origin, pm->mins, pm->maxs, point);
		pm->groundplane = trace.plane;
		pml.groundsurface = trace.surface;
		pml.groundcontents = trace.contents;

		// [Paril-KEX] to attempt to fix edge cases where you get stuck
		// wedged between a slope and a wall (which is irrecoverable
		// most of the time), we'll allow the player to "stand" on
		// slopes if they are right up against a wall
		bool slanted_ground = trace.fraction < 1.0f && trace.plane.normal[2] < 0.7f;

		if (slanted_ground)
		{
			trace_t slant = PM_Trace(pml.origin, pm->mins, pm->maxs, pml.origin + trace.plane.normal);

			if (slant.fraction < 1.0f && !slant.startsolid)
				slanted_ground = false;
		}

		if (trace.fraction == 1.0f || (slanted_ground && !trace.startsolid))
		{
			pm->groundentity = nullptr;
			pm->s.pm_flags &= ~PMF_ON_GROUND;
		}
		else
		{
			pm->groundentity = trace.ent;

			// hitting solid ground will end a waterjump
			if (pm->s.pm_flags & PMF_TIME_WATERJUMP)
			{
				pm->s.pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_TELEPORT | PMF_TIME_TRICK);
				pm->s.pm_time = 0;
			}

			if (!(pm->s.pm_flags & PMF_ON_GROUND))
			{
				// just hit the ground

				// [Paril-KEX]
				if (!pm_config.n64_physics && pml.velocity[2] >= 100.f && pm->groundplane.normal[2] >= 0.9f && !(pm->s.pm_flags & PMF_DUCKED))
				{
					pm->s.pm_flags |= PMF_TIME_TRICK;
					pm->s.pm_time = 64;
				}

				// [Paril-KEX] calculate impact delta; this also fixes triple jumping
				vec3_t clipped_velocity;
				PM_ClipVelocity(pml.velocity, pm->groundplane.normal, clipped_velocity, 1.01f);

				pm->impact_delta = pml.start_velocity[2] - clipped_velocity[2];

				pm->s.pm_flags |= PMF_ON_GROUND;

				if (pm_config.n64_physics || (pm->s.pm_flags & PMF_DUCKED))
				{
					pm->s.pm_flags |= PMF_TIME_LAND;
					pm->s.pm_time = 128;
				}
			}
		}

		PM_RecordTrace(pm->touch, trace);
	}

	//
	// get waterlevel, accounting for ducking
	//
	PM_GetWaterLevel(pml.origin, pm->waterlevel, pm->watertype);
}

/*
=============
PM_CheckJump
=============
*/
void PM_CheckJump()
{
	if (pm->s.pm_flags & PMF_TIME_LAND)
	{ // hasn't been long enough since landing to jump again
		return;
	}
	
	if (!(pm->cmd.buttons & BUTTON_JUMP))
	{ // not holding jump
		pm->s.pm_flags &= ~PMF_JUMP_HELD;
		return;
	}

	// must wait for jump to be released
	if (pm->s.pm_flags & PMF_JUMP_HELD)
		return;

	if (pm->s.pm_type == PM_DEAD)
		return;

	if (pm->waterlevel >= WATER_WAIST)
	{ // swimming, not jumping
		pm->groundentity = nullptr;
		return;
	}

	if (pm->groundentity == nullptr)
		return; // in air, so no effect

	pm->s.pm_flags |= PMF_JUMP_HELD;
	pm->jump_sound = true;
	pm->groundentity = nullptr;
	pm->s.pm_flags &= ~PMF_ON_GROUND;

	float jump_height = 270.f;

	pml.velocity[2] += jump_height;
	if (pml.velocity[2] < jump_height)
		pml.velocity[2] = jump_height;
}

/*
=============
PM_CheckSpecialMovement
=============
*/
void PM_CheckSpecialMovement()
{
	vec3_t	spot;
	vec3_t	flatforward;
	trace_t trace;

	if (pm->s.pm_time)
		return;

	pm->s.pm_flags &= ~PMF_ON_LADDER;

	// check for ladder
	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	flatforward.normalize();

	spot = pml.origin + (flatforward * 1);
	trace = PM_Trace(pml.origin, pm->mins, pm->maxs, spot, CONTENTS_LADDER);
	if ((trace.fraction < 1) && (trace.contents & CONTENTS_LADDER) && pm->waterlevel < WATER_WAIST)
		pm->s.pm_flags |= PMF_ON_LADDER;

	if (!pm->s.gravity)
		return;

	// check for water jump
	// [Paril-KEX] don't try waterjump if we're moving against where we'll hop
	if (!(pm->cmd.buttons & BUTTON_JUMP)
		&& pm->cmd.forwardmove <= 0)
		return;

	if (pm->waterlevel != WATER_WAIST)
		return;
	// [Paril-KEX]
	else if (pm->watertype & CONTENTS_NO_WATERJUMP)
		return;

	// quick check that something is even blocking us forward
	trace = PM_Trace(pml.origin, pm->mins, pm->maxs, pml.origin + (flatforward * 40), MASK_SOLID);

	// we aren't blocked, or what we're blocked by is something we can walk up
	if (trace.fraction == 1.0f || trace.plane.normal.z >= 0.7f)
		return;

	// [Paril-KEX] improved waterjump
	vec3_t waterjump_vel = flatforward * 50;
	waterjump_vel.z = 350;

	// simulate what would happen if we jumped out here, and
	// if we land on a dry spot we're good!
	// simulate 1 sec worth of movement
	touch_list_t touches;
	vec3_t waterjump_origin = pml.origin;
	float time = 0.1f;
	bool has_time = true;

	for (size_t i = 0; i < min(50, (int32_t) (10 * (800.f / pm->s.gravity))); i++)
	{
		waterjump_vel[2] -= pm->s.gravity * time;

		if (waterjump_vel[2] < 0)
			has_time = false;

		PM_StepSlideMove_Generic(waterjump_origin, waterjump_vel, time, pm->mins, pm->maxs, touches, has_time, PM_Trace_Auto);
	}

	// snap down to ground
	trace = PM_Trace(waterjump_origin, pm->mins, pm->maxs, waterjump_origin - vec3_t { 0, 0, 2.f }, MASK_SOLID);

	// can't stand here
	if (trace.fraction == 1.0f || trace.plane.normal.z < 0.7f ||
		trace.endpos.z < pml.origin.z)
		return;

	// we're currently standing on ground, and the snapped position
	// is a step
	if (pm->groundentity && fabsf(pml.origin.z - trace.endpos.z) <= STEPSIZE)
		return;

	water_level_t level;
	contents_t type;

	PM_GetWaterLevel(trace.endpos, level, type);

	// the water jump spot will be under water, so we're
	// probably hitting something weird that isn't important
	if (level >= WATER_WAIST)
		return;

	// valid waterjump!
	// jump out of water
	pml.velocity = flatforward * 50;
	pml.velocity[2] = 350;

	pm->s.pm_flags |= PMF_TIME_WATERJUMP;
	pm->s.pm_time = 2048;
}

/*
===============
PM_FlyMove
===============
*/
void PM_FlyMove(bool doclip)
{
	float	speed, drop, friction, control, newspeed;
	float	currentspeed, addspeed, accelspeed;
	int		i;
	vec3_t	wishvel;
	float	fmove, smove;
	vec3_t	wishdir;
	float	wishspeed;

	pm->s.viewheight = doclip ? 0 : 22;

	// friction

	speed = pml.velocity.length();
	if (speed < 1)
	{
		pml.velocity = vec3_origin;
	}
	else
	{
		drop = 0;

		friction = pm_friction * 1.5f; // extra friction
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control * friction * pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;

		pml.velocity *= newspeed;
	}

	// accelerate
	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.sidemove;

	pml.forward.normalize();
	pml.right.normalize();

	for (i = 0; i < 3; i++)
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;

	if (pm->cmd.buttons & BUTTON_JUMP)
		wishvel[2] += (pm_waterspeed * 0.5f);
	if (pm->cmd.buttons & BUTTON_CROUCH)
		wishvel[2] -= (pm_waterspeed * 0.5f);

	wishdir = wishvel;
	wishspeed = wishdir.normalize();

	//
	// clamp to server defined max speed
	//
	if (wishspeed > pm_maxspeed)
	{
		wishvel *= pm_maxspeed / wishspeed;
		wishspeed = pm_maxspeed;
	}

	// Paril: newer clients do this
	wishspeed *= 2;

	currentspeed = pml.velocity.dot(wishdir);
	addspeed = wishspeed - currentspeed;

	if (addspeed > 0)
	{
		accelspeed = pm_accelerate * pml.frametime * wishspeed;
		if (accelspeed > addspeed)
			accelspeed = addspeed;

		for (i = 0; i < 3; i++)
			pml.velocity[i] += accelspeed * wishdir[i];
	}

	if (doclip)
	{
		/*for (i = 0; i < 3; i++)
			end[i] = pml.origin[i] + pml.frametime * pml.velocity[i];

		trace = PM_Trace(pml.origin, pm->mins, pm->maxs, end);

		pml.origin = trace.endpos;*/

		PM_StepSlideMove();
	}
	else
	{
		// move
		pml.origin += (pml.velocity * pml.frametime);
	}
}

void PM_SetDimensions()
{
	pm->mins[0] = -16;
	pm->mins[1] = -16;

	pm->maxs[0] = 16;
	pm->maxs[1] = 16;

	if (pm->s.pm_type == PM_GIB)
	{
		pm->mins[2] = 0;
		pm->maxs[2] = 16;
		pm->s.viewheight = 8;
		return;
	}

	pm->mins[2] = -24;

	if ((pm->s.pm_flags & PMF_DUCKED) || pm->s.pm_type == PM_DEAD)
	{
		pm->maxs[2] = 4;
		pm->s.viewheight = -2;
	}
	else
	{
		pm->maxs[2] = 32;
		pm->s.viewheight = 22;
	}
}

inline bool PM_AboveWater()
{
	const vec3_t below = pml.origin - vec3_t{ 0, 0, 8 };

	bool solid_below = pm->trace(pml.origin, &pm->mins, &pm->maxs, below, pm->player, MASK_SOLID).fraction < 1.0f;

	if (solid_below)
		return false;

	bool water_below = pm->trace(pml.origin, &pm->mins, &pm->maxs, below, pm->player, MASK_WATER).fraction < 1.0f;

	if (water_below)
		return true;

	return false;
}

/*
==============
PM_CheckDuck

Sets mins, maxs, and pm->viewheight
==============
*/
bool PM_CheckDuck()
{
	if (pm->s.pm_type == PM_GIB)
		return false;

	trace_t trace;
	bool flags_changed = false;

	if (pm->s.pm_type == PM_DEAD)
	{
		if (!(pm->s.pm_flags & PMF_DUCKED))
		{
			pm->s.pm_flags |= PMF_DUCKED;
			flags_changed = true;
		}
	}
	else if (
		(pm->cmd.buttons & BUTTON_CROUCH) &&
		(pm->groundentity || (pm->waterlevel <= WATER_FEET && !PM_AboveWater())) &&
		!(pm->s.pm_flags & PMF_ON_LADDER) &&
		!pm_config.n64_physics)
	{ // duck
		if (!(pm->s.pm_flags & PMF_DUCKED))
		{
			// check that duck won't be blocked
			vec3_t check_maxs = { pm->maxs[0], pm->maxs[1], 4 };
			trace = PM_Trace(pml.origin, pm->mins, check_maxs, pml.origin);
			if (!trace.allsolid)
			{
				pm->s.pm_flags |= PMF_DUCKED;
				flags_changed = true;
			}
		}
	}
	else
	{ // stand up if possible
		if (pm->s.pm_flags & PMF_DUCKED)
		{
			// try to stand up
			vec3_t check_maxs = { pm->maxs[0], pm->maxs[1], 32 };
			trace = PM_Trace(pml.origin, pm->mins, check_maxs, pml.origin);
			if (!trace.allsolid)
			{
				pm->s.pm_flags &= ~PMF_DUCKED;
				flags_changed = true;
			}
		}
	}

	if (!flags_changed)
		return false;

	PM_SetDimensions();
	return true;
}

/*
==============
PM_DeadMove
==============
*/
void PM_DeadMove()
{
	float forward;

	if (!pm->groundentity)
		return;

	// extra friction

	forward = pml.velocity.length();
	forward -= 20;
	if (forward <= 0)
	{
		pml.velocity = {};
	}
	else
	{
		pml.velocity.normalize();
		pml.velocity *= forward;
	}
}

bool PM_GoodPosition()
{
	if (pm->s.pm_type == PM_NOCLIP)
		return true;

	trace_t trace = PM_Trace(pm->s.origin, pm->mins, pm->maxs, pm->s.origin);

	return !trace.allsolid;
}

/*
================
PM_SnapPosition

On exit, the origin will have a value that is pre-quantized to the PMove
precision of the network channel and in a valid position.
================
*/
void PM_SnapPosition()
{
	pm->s.velocity = pml.velocity;
	pm->s.origin = pml.origin;

	if (PM_GoodPosition())
		return;

	if (G_FixStuckObject_Generic(pm->s.origin, pm->mins, pm->maxs, PM_Trace_Auto) == stuck_result_t::NO_GOOD_POSITION) {
		pm->s.origin = pml.previous_origin;
		return;
	}
}

/*
================
PM_InitialSnapPosition

================
*/
void PM_InitialSnapPosition()
{
	int					   x, y, z;
	vec3_t				base;
	constexpr int		   offset[3] = { 0, -1, 1 };

	base = pm->s.origin;

	for (z = 0; z < 3; z++)
	{
		pm->s.origin[2] = base[2] + offset[z];
		for (y = 0; y < 3; y++)
		{
			pm->s.origin[1] = base[1] + offset[y];
			for (x = 0; x < 3; x++)
			{
				pm->s.origin[0] = base[0] + offset[x];
				if (PM_GoodPosition())
				{
					pml.origin = pm->s.origin;
					pml.previous_origin = pm->s.origin;
					return;
				}
			}
		}
	}
}

/*
================
PM_ClampAngles

================
*/
void PM_ClampAngles()
{
	if (pm->s.pm_flags & PMF_TIME_TELEPORT)
	{
		pm->viewangles[YAW] = pm->cmd.angles[YAW] + pm->s.delta_angles[YAW];
		pm->viewangles[PITCH] = 0;
		pm->viewangles[ROLL] = 0;
	}
	else
	{
		// circularly clamp the angles with deltas
		pm->viewangles = pm->cmd.angles + pm->s.delta_angles;

		// don't let the player look up or down more than 90 degrees
		if (pm->viewangles[PITCH] > 89 && pm->viewangles[PITCH] < 180)
			pm->viewangles[PITCH] = 89;
		else if (pm->viewangles[PITCH] < 271 && pm->viewangles[PITCH] >= 180)
			pm->viewangles[PITCH] = 271;
	}
	AngleVectors(pm->viewangles, pml.forward, pml.right, pml.up);
}

// [Paril-KEX]
static void PM_ScreenEffects()
{
	// add for contents
	vec3_t vieworg = pml.origin + pm->viewoffset + vec3_t{ 0, 0, (float) pm->s.viewheight };
	contents_t contents = pm->pointcontents(vieworg);

	if (contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER))
		pm->rdflags |= RDF_UNDERWATER;
	else
		pm->rdflags &= ~RDF_UNDERWATER;

	if (contents & (CONTENTS_SOLID | CONTENTS_LAVA))
		G_AddBlend(1.0f, 0.3f, 0.0f, 0.6f, pm->screen_blend);
	else if (contents & CONTENTS_SLIME)
		G_AddBlend(0.0f, 0.1f, 0.05f, 0.6f, pm->screen_blend);
	else if (contents & CONTENTS_WATER)
		G_AddBlend(0.5f, 0.3f, 0.2f, 0.4f, pm->screen_blend);
}

/*
================
Pmove

Can be called by either the server or the client
================
*/
void Pmove(pmove_t *pmove)
{
	pm = pmove;

	// clear results
	pm->touch.num = 0;
	pm->viewangles = {};
	pm->s.viewheight = 0;
	pm->groundentity = nullptr;
	pm->watertype = CONTENTS_NONE;
	pm->waterlevel = WATER_NONE;
	pm->screen_blend = {};
	pm->rdflags = RDF_NONE;
	pm->jump_sound = false;
	pm->step_clip = false;
	pm->impact_delta = 0;

	// clear all pmove local vars
	pml = {};

	// convert origin and velocity to float values
	pml.origin = pm->s.origin;
	pml.velocity = pm->s.velocity;

	pml.start_velocity = pml.velocity;

	// save old org in case we get stuck
	pml.previous_origin = pm->s.origin;

	pml.frametime = pm->cmd.msec * 0.001f;

	PM_ClampAngles();

	if (pm->s.pm_type == PM_SPECTATOR || pm->s.pm_type == PM_NOCLIP)
	{
		pm->s.pm_flags = PMF_NONE;

		if (pm->s.pm_type == PM_SPECTATOR)
		{
			pm->mins[0] = -8;
			pm->mins[1] = -8;
			pm->maxs[0] = 8;
			pm->maxs[1] = 8;
			pm->mins[2] = -8;
			pm->maxs[2] = 8;
		}

		PM_FlyMove(pm->s.pm_type == PM_SPECTATOR);
		PM_SnapPosition();
		return;
	}

	if (pm->s.pm_type >= PM_DEAD)
	{
		pm->cmd.forwardmove = 0;
		pm->cmd.sidemove = 0;
		pm->cmd.buttons &= ~(BUTTON_JUMP | BUTTON_CROUCH);
	}

	if (pm->s.pm_type == PM_FREEZE)
		return; // no movement at all

	// set mins, maxs, and viewheight
	PM_SetDimensions();

	// catagorize for ducking
	PM_CatagorizePosition();

	if (pm->snapinitial)
		PM_InitialSnapPosition();

	// set groundentity, watertype, and waterlevel
	if (PM_CheckDuck())
		PM_CatagorizePosition();

	if (pm->s.pm_type == PM_DEAD)
		PM_DeadMove();

	PM_CheckSpecialMovement();

	// drop timing counter
	if (pm->s.pm_time)
	{
		if (pm->cmd.msec >= pm->s.pm_time)
		{
			pm->s.pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_TELEPORT | PMF_TIME_TRICK);
			pm->s.pm_time = 0;
		}
		else
			pm->s.pm_time -= pm->cmd.msec;
	}

	if (pm->s.pm_flags & PMF_TIME_TELEPORT)
	{ // teleport pause stays exactly in place
	}
	else if (pm->s.pm_flags & PMF_TIME_WATERJUMP)
	{ // waterjump has no control, but falls
		pml.velocity[2] -= pm->s.gravity * pml.frametime;
		if (pml.velocity[2] < 0)
		{ // cancel as soon as we are falling down again
			pm->s.pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_TELEPORT | PMF_TIME_TRICK);
			pm->s.pm_time = 0;
		}

		PM_StepSlideMove();
	}
	else
	{
		PM_CheckJump();

		PM_Friction();

		if (pm->waterlevel >= WATER_WAIST)
			PM_WaterMove();
		else
		{
			vec3_t angles;

			angles = pm->viewangles;
			if (angles[PITCH] > 180)
				angles[PITCH] = angles[PITCH] - 360;
			angles[PITCH] /= 3;

			AngleVectors(angles, pml.forward, pml.right, pml.up);

			PM_AirMove();
		}
	}

	// set groundentity, watertype, and waterlevel for final spot
	PM_CatagorizePosition();

	// trick jump
	if (pm->s.pm_flags & PMF_TIME_TRICK)
		PM_CheckJump();

	// [Paril-KEX]
	PM_ScreenEffects();

	PM_SnapPosition();
}
