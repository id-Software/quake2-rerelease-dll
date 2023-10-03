// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#pragma once

void	Bot_SetWeapon( edict_t * bot, const int weaponIndex, const bool instantSwitch );
void	Bot_TriggerEdict( edict_t * bot, edict_t * edict );
int32_t Bot_GetItemID( const char * classname );
void	Bot_UseItem( edict_t * bot, const int32_t itemID );
void    Edict_ForceLookAtPoint( edict_t * edict, gvec3_cref_t point );
bool    Bot_PickedUpItem( edict_t * bot, edict_t * item );
