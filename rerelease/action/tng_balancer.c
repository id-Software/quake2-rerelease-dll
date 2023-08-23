#include "g_local.h"

cvar_t *eventeams;
cvar_t *use_balancer;

edict_t *FindNewestPlayer(int team)
{
	edict_t *e, *newest = NULL;
	int i;

	for (i = 0, e = &g_edicts[1]; i < game.maxclients; i++, e++)
	{
		if (!e->inuse || e->client->resp.team != team)
			continue;

		if (!newest || e->client->resp.joined_team > newest->client->resp.joined_team) {
			newest = e;
		}
	}

	return newest;
}

void CalculatePlayers(int *players)
{
	edict_t *e;
	int i;

	for (i = 0, e = &g_edicts[1]; i < game.maxclients; i++, e++)
	{
		if (!e->inuse)
			continue;

		players[e->client->resp.team]++;
	}
}

/* parameter can be current (dead) player or null */
qboolean CheckForUnevenTeams (edict_t *ent)
{
	edict_t *swap_ent = NULL;
	int i, other_team, players[TEAM_TOP] = {0}, leastPlayers, mostPlayers;

	if(!use_balancer->value)
		return false;

	CalculatePlayers(players);

	leastPlayers = mostPlayers = TEAM1;
	for (i = TEAM1; i <= teamCount; i++) {
		if (players[i] > players[mostPlayers])
			mostPlayers = i;

		if (players[i] < players[leastPlayers])
			leastPlayers = i;
	}
	if (players[mostPlayers] > players[leastPlayers] + 1) {
		other_team = leastPlayers;
		swap_ent = FindNewestPlayer(mostPlayers);
	}

	if(swap_ent && (!ent || ent == swap_ent)) {
		gi.centerprintf (swap_ent, "You have been swapped to the other team to even the game.");
		unicastSound(swap_ent, gi.soundindex("misc/talk1.wav"), 1.0);
		swap_ent->client->team_force = true;
		JoinTeam(swap_ent, other_team, 1);
		return true;
	}

	return false;
}

qboolean IsAllowedToJoin(edict_t *ent, int desired_team)
{
	int i, players[TEAM_TOP] = {0}, mostPlayers;

	if(ent->client->team_force) {
		ent->client->team_force = false;
		return true;
	}

	CalculatePlayers(players);

	mostPlayers = 0;
	for (i = TEAM1; i <= teamCount; i++) {
		if (i == desired_team)
			continue;

		if (!mostPlayers || players[i] > players[mostPlayers])
			mostPlayers = i;
	}

	/* can join both teams if they are even and can join if the other team has less players than current */
	if (players[desired_team] < players[mostPlayers] ||
		(ent->client->resp.team == NOTEAM && players[desired_team] == players[mostPlayers]))
		return true;
	return false;
}

