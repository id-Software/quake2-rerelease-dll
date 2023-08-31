// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "g_local.h"

/*
==============================================================================

PLAYER TRAIL

==============================================================================

This is a two-way list containing the a list of points of where
the player has been recently. It is used by monsters for pursuit.

This is improved from vanilla; now, the list itself is stored in
client data so it can be stored for multiple clients.

chain = next
enemy = prev

The head node will always have a null "chain", the tail node
will always have a null "enemy".
*/

constexpr size_t TRAIL_LENGTH = 8;

// places a new entity at the head of the player trail.
// the tail entity may be moved to the front if the length
// is at the end.
static edict_t *PlayerTrail_Spawn(edict_t *owner)
{
	size_t len = 0;

	for (edict_t *tail = owner->client->trail_tail; tail; tail = tail->chain)
		len++;

	edict_t *trail;

	// move the tail to the head
	if (len == TRAIL_LENGTH)
	{
		// unlink the old tail
		trail = owner->client->trail_tail;
		owner->client->trail_tail = trail->chain;
		owner->client->trail_tail->enemy = nullptr;
		trail->chain = trail->enemy = nullptr;
	}
	else
	{
		// spawn a new head
		trail = G_Spawn();
		trail->classname = "player_trail";
	}

	// link as new head
	if (owner->client->trail_head)
		owner->client->trail_head->chain = trail;
	trail->enemy = owner->client->trail_head;
	owner->client->trail_head = trail;

	// if there's no tail, we become the tail too
	if (!owner->client->trail_tail)
		owner->client->trail_tail = trail;

	return trail;
}

// destroys all player trail entities in the map.
// we don't want these to stay around across level loads.
void PlayerTrail_Destroy(edict_t *player)
{
	for (size_t i = 0; i < globals.num_edicts; i++)
		if (g_edicts[i].classname && strcmp(g_edicts[i].classname, "player_trail") == 0)
			if (!player || g_edicts[i].owner == player)
				G_FreeEdict(&g_edicts[i]);

	if (player)
		player->client->trail_head = player->client->trail_tail = nullptr;
	else for (size_t i = 0; i < game.maxclients; i++)
		game.clients[i].trail_head = game.clients[i].trail_tail = nullptr;
}

// check to see if we can add a new player trail spot
// for this player.
void PlayerTrail_Add(edict_t *player)
{
	// if we can still see the head, we don't want a new one.
	if (player->client->trail_head && visible(player, player->client->trail_head))
		return;
	// don't spawn trails in intermission, if we're dead, if we're noclipping or not on ground yet
	else if (level.intermissiontime || player->health <= 0 || player->movetype == MOVETYPE_NOCLIP ||
		!player->groundentity)
		return;

	edict_t *trail = PlayerTrail_Spawn(player);
	trail->s.origin = player->s.old_origin;
	trail->timestamp = level.time;
	trail->owner = player;
}

// pick a trail node that matches the player
// we're hunting that is visible to us.
edict_t *PlayerTrail_Pick(edict_t *self, bool next)
{
	// not player or doesn't have a trail yet
	if (!self->enemy->client || !self->enemy->client->trail_head)
		return nullptr;

	// find which marker head that was dropped while we
	// were searching for this enemy
	edict_t *marker;

	for (marker = self->enemy->client->trail_head; marker; marker = marker->enemy)
	{
		if (marker->timestamp <= self->monsterinfo.trail_time)
			continue;

		break;
	}

	if (next)
	{
		// find the marker we're closest to
		float closest_dist = std::numeric_limits<float>::infinity();
		edict_t *closest = nullptr;

		for (edict_t *m2 = marker; m2; m2 = m2->enemy)
		{
			float len = (m2->s.origin - self->s.origin).lengthSquared();

			if (len < closest_dist)
			{
				closest_dist = len;
				closest = m2;
			}
		}

		// should never happen
		if (!closest)
			return nullptr;

		// use the next one from the closest one
		marker = closest->chain;
	}
	else
	{
		// from that marker, find the first one we can see
		for (; marker && !visible(self, marker); marker = marker->enemy)
			continue;
	}

	return marker;
}
