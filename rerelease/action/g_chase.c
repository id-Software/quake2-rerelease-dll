//-----------------------------------------------------------------------------
// This source file comes from the 3.20 source originally.
// 
// Added through-the-eyes cam mode, as well as the ability to spin around the player
// when in regular chase cam mode, among other Axshun-related mods.
// -Fireblade
//
// $Id: g_chase.c,v 1.4 2003/06/15 15:34:32 igor Exp $
//
//-----------------------------------------------------------------------------
// $Log: g_chase.c,v $
// Revision 1.4  2003/06/15 15:34:32  igor
// - removed the zcam code from this branch (see other branch)
// - added fixes from 2.72 (source only) version
// - resetted version number to 2.72
// - This version should be exactly like the release 2.72 - just with a few
//   more fixes (which whoever did the source only variant didn't get because
//   he didn't use the CVS as he should. Shame on him.
//
// Revision 1.3  2002/09/04 11:23:09  ra
// Added zcam to TNG and bumped version to 3.0
//
// Revision 1.2  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.1.1.1  2001/05/06 17:30:00  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#include "g_local.h"

void DisableChaseCam( edict_t *ent )
{
	//remove a gun model if we were using one for in-eyes
	ent->client->ps.gunframe = ent->client->ps.gunindex = 0;
	VectorClear( ent->client->ps.gunoffset );
	VectorClear( ent->client->ps.kick_angles );
	ent->client->ps.viewangles[ROLL] = 0;

	ent->client->chase_mode = 0;
	ent->client->chase_target = NULL;
	ent->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;

	ent->client->desired_fov = 90;
	ent->client->ps.fov = 90;

	ent->client->clientNum = (ent - g_edicts) - 1;
}

int ChaseTargetGone( edict_t * ent )
{
	edict_t *targ = ent->client->chase_target;

	// is our chase target gone?
	if (!targ || !targ->inuse
		|| (targ->solid == SOLID_NOT && targ->deadflag != DEAD_DEAD))
	{
		ChaseNext( ent );
		if (ent->client->chase_target == targ) {
			DisableChaseCam( ent );
			return 1;
		}
	}
	return 0;
}

void NextChaseMode( edict_t *ent )
{
	ent->client->chase_mode = (ent->client->chase_mode + 1) % 3;

	if (ent->client->chase_mode == 1)
	{
		//going 3rd person, remove gun and invisible player
		ent->client->clientNum = (ent - g_edicts) - 1;
		ent->client->ps.gunindex = ent->client->ps.gunframe = 0;
		if (!ent->client->chase_target) {
			GetChaseTarget( ent );
			if (!ent->client->chase_target) {
				ent->client->chase_mode = 0;
				return;
			}
		}

		if (ent->client->resp.team && teamplay->value && limchasecam->value == 2) {
			NextChaseMode( ent );
			return;
		}
	}
	else if (ent->client->chase_mode == 2)
	{
		if (!ent->client->chase_target) {
			GetChaseTarget( ent );
			if (!ent->client->chase_target) {
				ent->client->chase_mode = 0;
				return;
			}
		}
		//set clientnum to hide chased person on supported server
		ent->client->clientNum = (ent->client->chase_target - g_edicts) - 1;
		ent->client->ps.gunindex = ent->client->ps.gunframe = 0;
	}
	else// if (ent->client->chase_mode == CHASE_FREE)
	{
		if (ent->client->resp.team && teamplay->value && limchasecam->value) {
			if (limchasecam->value == 1)  {
				NextChaseMode( ent );
			}
			else {
				ent->client->chase_mode = 2;
			}
			return;
		}

		DisableChaseCam( ent );
	}
}

void UpdateChaseCam( edict_t * ent )
{
	vec3_t o, ownerv, goal;
	edict_t *targ;
	vec3_t forward, right, angles;
	trace_t trace;
	int i;
	gclient_t *client;
	short oldStats[MAX_STATS];

	if (ChaseTargetGone( ent ))
		return;

	client = ent->client;

	targ = client->resp.last_chase_target = client->chase_target;

	memcpy(oldStats, client->ps.stats, sizeof(oldStats));

	if (client->chase_mode == 1)
	{
		VectorCopy( targ->s.origin, ownerv );

		ownerv[2] += targ->viewheight;

		VectorCopy(client->resp.cmd_angles, angles);
		for (i = 0; i < 3; i++)
			 angles[i] += SHORT2ANGLE(client->ps.pmove.delta_angles[i]);

		AngleVectors( angles, forward, right, NULL );
		VectorNormalize( forward );
		VectorMA( ownerv, -150, forward, o );

		/*
		// jump animation lifts
		// Raptor007: This made sense for baseq2 shoulder chase, but looks bad in AQ2 on slopes.
		if (!targ->groundentity)
			o[2] += 16;
		*/

		PRETRACE();
		trace = gi.trace( ownerv, vec3_origin, vec3_origin, o, targ, MASK_SOLID );

		VectorCopy( trace.endpos, goal );

		VectorMA( goal, 2, forward, goal );

		// pad for floors and ceilings
		VectorCopy( goal, o );
		o[2] += 6;
		trace = gi.trace( goal, vec3_origin, vec3_origin, o, targ, MASK_SOLID );
		if (trace.fraction < 1) {
			VectorCopy( trace.endpos, goal );
			goal[2] -= 6;
		}

		VectorCopy( goal, o );
		o[2] -= 6;
		trace = gi.trace( goal, vec3_origin, vec3_origin, o, targ, MASK_SOLID );
		POSTTRACE();
		if (trace.fraction < 1) {
			VectorCopy( trace.endpos, goal );
			goal[2] += 6;
		}

		if (targ->deadflag)
			client->ps.pmove.pm_type = PM_DEAD;
		else
			client->ps.pmove.pm_type = PM_FREEZE;

		VectorCopy(goal, ent->s.origin);
		VectorScale(goal, 8, client->ps.pmove.origin);

		for (i = 0; i < 3; i++) {
			client->ps.pmove.delta_angles[i] = ANGLE2SHORT(angles[i] - client->resp.cmd_angles[i]);
		}
		VectorCopy(angles, client->ps.viewangles);
		VectorCopy(angles, client->v_angle);

		client->clientNum = ent - g_edicts - 1;
		client->ps.gunindex = client->ps.gunframe = 0;
		client->desired_fov = 90;
		client->ps.fov = 90;

		ent->viewheight = 0;
		memcpy(client->ps.stats, targ->client->ps.stats, sizeof(client->ps.stats));
		client->ps.stats[STAT_SNIPER_ICON] = 0; // only show sniper lens when in chase mode 2
		if( client->ps.stats[STAT_HELPICON] != level.pic_health ) // Keep bandaging icon.
			client->ps.stats[STAT_HELPICON] = 0; // No weapon icon in 3rd person chase.
	}
	else             // chase_mode == 2
	{

		VectorCopy( targ->client->v_angle, angles );

		if (!(game.serverfeatures & GMF_CLIENTNUM))
		{
			VectorCopy( targ->s.origin, ownerv );
			ownerv[2] += targ->viewheight;

			AngleVectors( angles, forward, right, NULL );
			VectorNormalize( forward );
			VectorMA( ownerv, 11, forward, o );

			VectorCopy(o, ent->s.origin);
			VectorScale(o, 8, client->ps.pmove.origin);

			if (targ->deadflag)
			{
				client->ps.viewangles[ROLL] = 40;
				client->ps.viewangles[PITCH] = -15;
				client->ps.viewangles[YAW] = targ->client->killer_yaw;
				client->ps.pmove.pm_type = PM_DEAD;
			}
			else
			{
				VectorCopy(targ->client->ps.kick_angles, client->ps.kick_angles);
				for (i = 0; i < 3; i++) {
					ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(angles[i] - client->resp.cmd_angles[i]);
				}
				VectorCopy(angles, client->ps.viewangles);
				VectorCopy(angles, client->v_angle);
				client->ps.pmove.pm_type = PM_FREEZE;
			}

			ent->viewheight = 0;
			memcpy(client->ps.stats, targ->client->ps.stats, sizeof(client->ps.stats));
		}
		else
		{
			byte jump_held = client->ps.pmove.pm_flags & PMF_JUMP_HELD;
			client->ps = targ->client->ps;

			for (i = 0; i < 3; i++)
				client->ps.pmove.delta_angles[i] = ANGLE2SHORT(angles[i] - client->resp.cmd_angles[i]);

			VectorCopy(client->ps.viewangles, ent->s.angles);
			VectorCopy(client->ps.viewangles, client->v_angle);
			VectorScale(client->ps.pmove.origin, 0.125f, ent->s.origin);
			ent->viewheight = targ->viewheight;

			client->ps.pmove.pm_flags &= ~PMF_JUMP_HELD;
			client->ps.pmove.pm_flags |= jump_held;
			client->ps.pmove.pm_type = PM_FREEZE;
		}

		ent->client->clientNum = targ - g_edicts - 1;
		ent->client->ps.fov = targ->client->ps.fov;
		ent->client->desired_fov = targ->client->ps.fov;

		// Keep bandaging icon from stats, but weapon icon is based on observer's hand and fov.
		if( client->ps.stats[STAT_HELPICON] == level.pic_health )
			;
		else if( (client->pers.hand == CENTER_HANDED || client->ps.fov > 91) && targ->client->weapon )
			client->ps.stats[STAT_HELPICON] = level.pic_items[targ->client->weapon->typeNum];
		else
			client->ps.stats[STAT_HELPICON] = 0;
	}

	//protect these
	for (i = STAT_TEAM_HEADER; i <= STAT_TEAM2_SCORE; i++) {
		client->ps.stats[i] = oldStats[i];
	}
	client->ps.stats[STAT_TEAM3_PIC] = oldStats[STAT_TEAM3_PIC];
	client->ps.stats[STAT_TEAM3_SCORE] = oldStats[STAT_TEAM3_SCORE];
	client->ps.stats[STAT_LAYOUTS] = oldStats[STAT_LAYOUTS];
	client->ps.stats[STAT_ID_VIEW] = oldStats[STAT_ID_VIEW];
	client->ps.stats[STAT_FRAGS] = oldStats[STAT_FRAGS];

	client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
	gi.linkentity( ent );
}

void SetChase( edict_t *ent, edict_t *target )
{
	if (!target) {
		DisableChaseCam( ent );
		return;
	}

	if (target != ent->client->chase_target && ent->client->chase_mode == 2) {
		ent->client->clientNum = (target - g_edicts) - 1;
	}
	ent->client->chase_target = target;
}

void ChaseNext( edict_t * ent )
{
	int i, limchase;
	edict_t *e, *targ = ent->client->chase_target;

	if (!targ)
		return;

	limchase = (teamplay->value && limchasecam->value && ent->client->resp.team != NOTEAM);

	i = targ - g_edicts;
	do
	{
		i++;
		if (i > game.maxclients)
			i = 1;
		e = g_edicts + i;
		if (!e->inuse)
			continue;
		if (e->solid == SOLID_NOT && e->deadflag != DEAD_DEAD)
			continue;
		if (limchase && ent->client->resp.team != e->client->resp.team)
			continue;

		break;
	} while (e != targ);

	if (e != targ)
		SetChase( ent, e );
}

void ChasePrev( edict_t * ent )
{
	int i, limchase;
	edict_t *e, *targ = ent->client->chase_target;

	if (!targ)
		return;

	limchase = (teamplay->value && limchasecam->value && ent->client->resp.team != NOTEAM);

	i = targ - g_edicts;
	do
	{
		i--;
		if (i < 1)
			i = game.maxclients;
		e = g_edicts + i;
		if (!e->inuse)
			continue;
		if (e->solid == SOLID_NOT && e->deadflag != DEAD_DEAD)
			continue;
		if (limchase && ent->client->resp.team != e->client->resp.team)
			continue;

		break;
	} while (e != targ);

	if (e != targ)
		SetChase( ent, e );
}

void GetChaseTarget( edict_t * ent )
{
	int i, limchase;
	edict_t *e, *targ;

	targ = ent->client->resp.last_chase_target;
	if (!targ) {
		targ = g_edicts + game.maxclients;
	}
	else {
		targ = targ - 1;
		if (targ < (g_edicts + 1)) {
			targ = g_edicts + game.maxclients;
		}
	}

	limchase = (teamplay->value && limchasecam->value && ent->client->resp.team != NOTEAM);

	i = targ - g_edicts;
	do
	{
		i++;
		if (i > game.maxclients)
			i = 1;
		e = g_edicts + i;
		if (!e->inuse)
			continue;
		if (e->solid == SOLID_NOT && e->deadflag != DEAD_DEAD)
			continue;
		if (limchase && ent->client->resp.team != e->client->resp.team)
			continue;

		SetChase( ent, e );
		return;
	} while (e != targ);

	//gi.cprintf(ent, PRINT_HIGH, "No players to chase.\n");
}
