//-----------------------------------------------------------------------------
// a_doorkick.c
// Door kicking code by hal[9k] 
// originally for AQ:Espionage (http://aqdt.fear.net/)
// email: hal9000@telefragged.com
// Assembled here by Homer (homer@fear.net)
//
// $Id: a_doorkick.c,v 1.2 2003/02/10 02:12:25 ra Exp $
//
//-----------------------------------------------------------------------------
// $Log: a_doorkick.c,v $
// Revision 1.2  2003/02/10 02:12:25  ra
// Zcam fixes, kick crashbug in CTF fixed and some code cleanup.
//
// Revision 1.1.1.1  2001/05/06 17:24:15  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#include "g_local.h"

#define STATE_TOP               0
#define STATE_BOTTOM            1
#define STATE_UP                2
#define STATE_DOWN              3

#define DOOR_START_OPEN         1
#define DOOR_REVERSE            2

extern void door_use(edict_t * self, edict_t * other, edict_t * activator);

// needed for KickDoor
void VectorRotate(vec3_t in, vec3_t angles, vec3_t out)
{
	float cv, sv, angle, tv;

	VectorCopy(in, out);

	angle = (-angles[PITCH]) * M_PI / 180;
	cv = cos(angle);
	sv = sin(angle);
	tv = (out[0] * cv) - (out[2] * sv);

	out[2] = (out[2] * cv) + (out[0] * sv);
	out[0] = tv;

	angle = (angles[YAW]) * M_PI / 180;

	cv = cos(angle);
	sv = sin(angle);
	tv = (out[0] * cv) - (out[1] * sv);

	out[1] = (out[1] * cv) + (out[0] * sv);
	out[0] = tv;
	angle = (angles[ROLL]) * M_PI / 180;

	cv = cos(angle);
	sv = sin(angle);
	tv = (out[1] * cv) - (out[2] * sv);
	out[2] = (out[2] * cv) + (out[1] * sv);
	out[1] = tv;
}

int KickDoor(trace_t * tr_old, edict_t * ent, vec3_t forward)
{
	trace_t tr;

	vec3_t d_forward, right, end;
	float d;

	if (!Q_stricmp(tr_old->ent->classname, "func_door_rotating")) {
		// Make that the door is closed

		tr = *tr_old;
#if 1
		if ((!(tr.ent->spawnflags & DOOR_START_OPEN) &&
		     !(tr.ent->moveinfo.state == STATE_TOP)) ||
		    ((tr.ent->spawnflags & DOOR_START_OPEN) && !(tr.ent->moveinfo.state == STATE_BOTTOM)))
#else
		if ((!(tr.ent->spawnflags & DOOR_START_OPEN) &&
		     ((tr.ent->moveinfo.state == STATE_BOTTOM) ||
		      (tr.ent->moveinfo.state == STATE_DOWN))) ||
		    ((tr.ent->spawnflags & DOOR_START_OPEN) &&
		     ((tr.ent->moveinfo.state == STATE_TOP) || (tr.ent->moveinfo.state == STATE_UP))))
#endif
		{
			//gi.dprintf( "Kicking a closed door\n" );

			// Find out if we are on the "outside"

#if 0
			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_RAILTRAIL);
			gi.WritePosition(tr.ent->s.origin);
			gi.WritePosition(tr.endpos);
			gi.multicast(tr.ent->s.origin, MULTICAST_PHS);
#endif
			VectorSubtract(tr.endpos, tr.ent->s.origin, d_forward);

			forward[2] = 0;
			d_forward[2] = 0;
			VectorNormalize(forward);
			VectorNormalize(d_forward);
			VectorSet(right, 0, 90, 0);
			VectorRotate(d_forward, right, d_forward);

			d = DotProduct(forward, d_forward);
			if (tr.ent->spawnflags & DOOR_REVERSE)
				d = -d;
			// d = sin( acos( d ) );
			if (d > 0.0) {
				// gi.dprintf( "we think we are on the outside\n" );
				//if ( tr.ent->spawnflags & DOOR_REVERSE )
				//  gi.dprintf( "but DOOR_REVERSE is set\n" );
				// Only use the door if it's not already opening
				if ((!(tr.ent->spawnflags & DOOR_START_OPEN) &&
				     !(tr.ent->moveinfo.state == STATE_UP)) ||
				    ((tr.ent->spawnflags & DOOR_START_OPEN) && (tr.ent->moveinfo.state == STATE_DOWN)))
					door_use(tr.ent, ent, ent);
				// Find out if someone else is on the other side
				VectorMA(tr.endpos, 25, forward, end);
				PRETRACE();
				tr = gi.trace(tr.endpos, NULL, NULL, end, tr.ent, MASK_SHOT);
				POSTTRACE();
				if (!((tr.surface) && (tr.surface->flags & SURF_SKY))) {
					if (tr.fraction < 1.0) {
						if (tr.ent->client) {
							//gi.dprintf("we found a client on the other side\n");
							*tr_old = tr;
							return (1);
						}
					}
				}
			}

		}
	}

	return (0);
}
