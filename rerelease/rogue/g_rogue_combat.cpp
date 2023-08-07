// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_combat.c

#include "../g_local.h"

void M_SetEffects(edict_t *self);

/*
ROGUE
clean up heal targets for medic
*/
void cleanupHealTarget(edict_t *ent)
{
	ent->monsterinfo.healer = nullptr;
	ent->takedamage = true;
	ent->monsterinfo.aiflags &= ~AI_RESURRECTING;
	M_SetEffects(ent);
}

// **********************
// ROGUE

/*
============
T_RadiusNukeDamage

Like T_RadiusDamage, but ignores walls (skips CanDamage check, among others)
// up to KILLZONE radius, do 10,000 points
// after that, do damage linearly out to KILLZONE2 radius
============
*/

void T_RadiusNukeDamage(edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, mod_t mod)
{
	float	 points;
	edict_t *ent = nullptr;
	vec3_t	 v;
	vec3_t	 dir;
	float	 len;
	float	 killzone, killzone2;
	trace_t	 tr;
	float	 dist;

	killzone = radius;
	killzone2 = radius * 2.0f;

	while ((ent = findradius(ent, inflictor->s.origin, killzone2)) != nullptr)
	{
		// ignore nobody
		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;
		if (!ent->inuse)
			continue;
		if (!(ent->client || (ent->svflags & SVF_MONSTER) || (ent->flags & FL_DAMAGEABLE)))
			continue;

		v = ent->mins + ent->maxs;
		v = ent->s.origin + (v * 0.5f);
		v = inflictor->s.origin - v;
		len = v.length();
		if (len <= killzone)
		{
			if (ent->client)
				ent->flags |= FL_NOGIB;
			points = 10000;
		}
		else if (len <= killzone2)
			points = (damage / killzone) * (killzone2 - len);
		else
			points = 0;

		if (points > 0)
		{
			if (ent->client)
				ent->client->nuke_time = level.time + 2_sec;
			dir = ent->s.origin - inflictor->s.origin;
			T_Damage(ent, inflictor, attacker, dir, inflictor->s.origin, vec3_origin, (int) points, (int) points, DAMAGE_RADIUS, mod);
		}
	}
	ent = g_edicts + 1; // skip the worldspawn
	// cycle through players
	while (ent)
	{
		if ((ent->client) && (ent->client->nuke_time != level.time + 2_sec) && (ent->inuse))
		{
			tr = gi.traceline(inflictor->s.origin, ent->s.origin, inflictor, MASK_SOLID);
			if (tr.fraction == 1.0f)
				ent->client->nuke_time = level.time + 2_sec;
			else
			{
				dist = realrange(ent, inflictor);
				if (dist < 2048)
					ent->client->nuke_time = max(ent->client->nuke_time, level.time + 1.5_sec);
				else
					ent->client->nuke_time = max(ent->client->nuke_time, level.time + 1_sec);
			}
			ent++;
		}
		else
			ent = nullptr;
	}
}

/*
============
T_RadiusClassDamage

Like T_RadiusDamage, but ignores anything with classname=ignoreClass
============
*/
void T_RadiusClassDamage(edict_t *inflictor, edict_t *attacker, float damage, char *ignoreClass, float radius, mod_t mod)
{
	float	 points;
	edict_t *ent = nullptr;
	vec3_t	 v;
	vec3_t	 dir;

	while ((ent = findradius(ent, inflictor->s.origin, radius)) != nullptr)
	{
		if (ent->classname && !strcmp(ent->classname, ignoreClass))
			continue;
		if (!ent->takedamage)
			continue;

		v = ent->mins + ent->maxs;
		v = ent->s.origin + (v * 0.5f);
		v = inflictor->s.origin - v;
		points = damage - 0.5f * v.length();
		if (ent == attacker)
			points = points * 0.5f;
		if (points > 0)
		{
			if (CanDamage(ent, inflictor))
			{
				dir = ent->s.origin - inflictor->s.origin;
				T_Damage(ent, inflictor, attacker, dir, inflictor->s.origin, vec3_origin, (int) points, (int) points, DAMAGE_RADIUS, mod);
			}
		}
	}
}

// ROGUE
// ********************