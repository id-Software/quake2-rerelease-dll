
#include "g_local.h"

cvar_t *sv_antilag;
cvar_t *sv_antilag_interp;

void antilag_update(edict_t *ent)
{
	antilag_t *state = &(ent->client->antilag_state);
	float time_stamp;

	state->seek++;
	state->curr_timestamp = level.time;
	
	time_stamp = level.time;
	if (sv_antilag_interp->value) // offset by 1 server frame to account for interpolation
		time_stamp += FRAMETIME;

	state->hist_timestamp[state->seek & ANTILAG_MASK] = time_stamp;
	VectorCopy(ent->s.origin, state->hist_origin[state->seek & ANTILAG_MASK]);
	VectorCopy(ent->mins, state->hist_mins[state->seek & ANTILAG_MASK]);
	VectorCopy(ent->maxs, state->hist_maxs[state->seek & ANTILAG_MASK]);
}


void antilag_clear(edict_t *ent)
{
	memset(&ent->client->antilag_state, 0, sizeof(antilag_t));
}


float antilag_findseek(edict_t *ent, float time_stamp)
{
	antilag_t *state = &(ent->client->antilag_state);

	int offs = 0;
	while (offs < ANTILAG_MAX)
	{
		if (state->hist_timestamp[(state->seek - offs) & ANTILAG_MASK] && state->hist_timestamp[(state->seek - offs) & ANTILAG_MASK] <= time_stamp)
		{
			if ((offs - 1) < 0) // never return a timestamp from the future aka erroneous crap
				return -1;

			float frac = 1;
			float stamp_last = state->hist_timestamp[(state->seek - offs) & ANTILAG_MASK];
			float stamp_next = state->hist_timestamp[(state->seek - (offs - 1)) & ANTILAG_MASK];

			time_stamp -= stamp_last;
			frac = time_stamp / (stamp_next - stamp_last);

			return (float)(state->seek - offs) + frac;
		}

		offs++;
	}

	return -1;
}


void antilag_rewind_all(edict_t *ent)
{
	if (!sv_antilag->value)
		return;

	if (ent->client->pers.antilag_optout)
		return;

	float time_to_seek = ent->client->antilag_state.curr_timestamp;

	time_to_seek -= ((float)ent->client->ping) / 1000;
	if (time_to_seek < level.time - ANTILAG_REWINDCAP)
		time_to_seek = level.time - ANTILAG_REWINDCAP;

	edict_t *who;
	antilag_t *state;
	int i;
	for (i = 1; i < game.maxclients; i++)
	{
		who = g_edicts + i;
		if (!who->inuse)
			continue;

		state = &who->client->antilag_state;
		state->rewound = false;

		if (who == ent)
			continue;

		if (who->deadflag != DEAD_NO)
			continue;

		float rewind_seek = antilag_findseek(who, time_to_seek);
		//Com_Printf("rewind seek %f\n", rewind_seek);
		if (rewind_seek < 0)
			continue;

		state->rewound = true;
		VectorCopy(who->s.origin, state->hold_origin);
		VectorCopy(who->mins, state->hold_mins);
		VectorCopy(who->maxs, state->hold_maxs);

		//Com_Printf("seek diff %f\n", (float)state->seek - rewind_seek);
		LerpVector(state->hist_origin[((int)rewind_seek) & ANTILAG_MASK], state->hist_origin[((int)(rewind_seek+1)) & ANTILAG_MASK], rewind_seek - ((float)(int)rewind_seek), who->s.origin);

		VectorCopy(state->hist_mins[(int)rewind_seek & ANTILAG_MASK], who->mins);
		VectorCopy(state->hist_maxs[(int)rewind_seek & ANTILAG_MASK], who->maxs);

		gi.linkentity(who);
	}
}


void antilag_unmove_all(void)
{
	if (!sv_antilag->value)
		return;

	edict_t *who;
	antilag_t *state;
	int i;
	for (i = 1; i < game.maxclients; i++)
	{
		who = g_edicts + i;
		if (!who->inuse)
			continue;

		state = &who->client->antilag_state;

		if (!state->rewound)
			continue;

		state->rewound = false;
		VectorCopy(state->hold_origin, who->s.origin);
		VectorCopy(state->hold_mins, who->mins);
		VectorCopy(state->hold_maxs, who->maxs);

		gi.linkentity(who);
	}
}
