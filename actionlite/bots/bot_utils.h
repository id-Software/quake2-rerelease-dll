// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#pragma once

void Entity_UpdateState( edict_t * edict );
const edict_t * FindLocalPlayer();
const edict_t * FindFirstBot();
const edict_t * FindFirstMonster();
const edict_t * FindActorUnderCrosshair( const edict_t * player );
