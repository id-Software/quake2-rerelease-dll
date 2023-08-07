// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "../g_local.h"

/*
=================
findradius2

Returns entities that have origins within a spherical area

ROGUE - tweaks for performance for tesla specific code
only returns entities that can be damaged
only returns entities that are FL_DAMAGEABLE

findradius2 (origin, radius)
=================
*/
edict_t *findradius2(edict_t *from, const vec3_t &org, float rad)
{
	// rad must be positive
	vec3_t eorg;
	int	   j;

	if (!from)
		from = g_edicts;
	else
		from++;
	for (; from < &g_edicts[globals.num_edicts]; from++)
	{
		if (!from->inuse)
			continue;
		if (from->solid == SOLID_NOT)
			continue;
		if (!from->takedamage)
			continue;
		if (!(from->flags & FL_DAMAGEABLE))
			continue;
		for (j = 0; j < 3; j++)
			eorg[j] = org[j] - (from->s.origin[j] + (from->mins[j] + from->maxs[j]) * 0.5f);
		if (eorg.length() > rad)
			continue;
		return from;
	}

	return nullptr;
}