// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "../g_local.h"

// RAFAEL
void SP_item_foodcube(edict_t *self)
{
	if (deathmatch->integer && g_no_health->integer)
	{
		G_FreeEdict(self);
		return;
	}

	self->model = "models/objects/trapfx/tris.md2";
	SpawnItem(self, GetItemByIndex(IT_HEALTH_SMALL));
	self->spawnflags |= SPAWNFLAG_ITEM_DROPPED;
	self->style = HEALTH_IGNORE_MAX;
	self->classname = "item_foodcube";
	self->s.effects |= EF_GIB;

	// Paril: set pickup noise for foodcube based on amount
	if (self->count < 10)
		self->noise_index = gi.soundindex("items/s_health.wav");
	else if (self->count < 25)
		self->noise_index = gi.soundindex("items/n_health.wav");
	else if (self->count < 50)
		self->noise_index = gi.soundindex("items/l_health.wav");
	else
		self->noise_index = gi.soundindex("items/m_health.wav");
}