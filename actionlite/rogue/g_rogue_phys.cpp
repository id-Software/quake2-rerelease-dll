// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "../g_local.h"

//============
// ROGUE
/*
=============
SV_Physics_NewToss

Toss, bounce, and fly movement. When on ground and no velocity, do nothing. With velocity,
slide.
=============
*/
void SV_Physics_NewToss(edict_t *ent)
{
	trace_t trace;
	vec3_t	move;
	//	float		backoff;
	edict_t *slave;
	bool	 wasinwater;
	bool	 isinwater;
	float	 speed, newspeed;
	vec3_t	 old_origin;
	//	float		firstmove;
	//	int			mask;

	// regular thinking
	SV_RunThink(ent);

	// if not a team captain, so movement will be handled elsewhere
	if (ent->flags & FL_TEAMSLAVE)
		return;

	wasinwater = ent->waterlevel;

	// find out what we're sitting on.
	move = ent->s.origin;
	move[2] -= 0.25f;
	trace = gi.trace(ent->s.origin, ent->mins, ent->maxs, move, ent, ent->clipmask);
	if (ent->groundentity && ent->groundentity->inuse)
		ent->groundentity = trace.ent;
	else
		ent->groundentity = nullptr;

	// if we're sitting on something flat and have no velocity of our own, return.
	if (ent->groundentity && (trace.plane.normal[2] == 1.0f) &&
		!ent->velocity[0] && !ent->velocity[1] && !ent->velocity[2])
	{
		return;
	}

	// store the old origin
	old_origin = ent->s.origin;

	SV_CheckVelocity(ent);

	// add gravity
	SV_AddGravity(ent);

	if (ent->avelocity[0] || ent->avelocity[1] || ent->avelocity[2])
		SV_AddRotationalFriction(ent);

	// add friction
	speed = ent->velocity.length();
	if (ent->waterlevel) // friction for water movement
	{
		newspeed = speed - (sv_waterfriction * 6 * (float) ent->waterlevel);
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;
		ent->velocity *= newspeed;
	}
	else if (!ent->groundentity) // friction for air movement
	{
		newspeed = speed - ((sv_friction));
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;
		ent->velocity *= newspeed;
	}
	else // use ground friction
	{
		newspeed = speed - (sv_friction * 6);
		if (newspeed < 0)
			newspeed = 0;
		newspeed /= speed;
		ent->velocity *= newspeed;
	}

	SV_FlyMove(ent, gi.frame_time_s, ent->clipmask);
	gi.linkentity(ent);

	G_TouchTriggers(ent);

	// check for water transition
	wasinwater = (ent->watertype & MASK_WATER);
	ent->watertype = gi.pointcontents(ent->s.origin);
	isinwater = ent->watertype & MASK_WATER;

	if (isinwater)
		ent->waterlevel = WATER_FEET;
	else
		ent->waterlevel = WATER_NONE;

	if (!wasinwater && isinwater)
		gi.positioned_sound(old_origin, g_edicts, CHAN_AUTO, gi.soundindex("misc/h2ohit1.wav"), 1, 1, 0);
	else if (wasinwater && !isinwater)
		gi.positioned_sound(ent->s.origin, g_edicts, CHAN_AUTO, gi.soundindex("misc/h2ohit1.wav"), 1, 1, 0);

	// move teamslaves
	for (slave = ent->teamchain; slave; slave = slave->teamchain)
	{
		slave->s.origin = ent->s.origin;
		gi.linkentity(slave);
	}
}

// ROGUE
//============