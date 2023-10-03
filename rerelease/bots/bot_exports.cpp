// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "../g_local.h"
#include "bot_exports.h"

/*
================
Bot_SetWeapon
================
*/
void Bot_SetWeapon( edict_t * bot, const int weaponIndex, const bool instantSwitch ) {
	if ( weaponIndex <= IT_NULL || weaponIndex > IT_TOTAL ) {
		return;
	}

	if ( ( bot->svflags & SVF_BOT ) == 0 ) {
		return;
	}

	gclient_t * client = bot->client;
	if ( client == nullptr || !client->pers.inventory[ weaponIndex ] ) {
		return;
	}

	const item_id_t weaponItemID = static_cast<item_id_t>( weaponIndex );

	const gitem_t * currentGun = client->pers.weapon;
	if ( currentGun != nullptr ) {
		if ( currentGun->id == weaponItemID ) {
			return;
		} // already have the gun in hand.
	}

	const gitem_t * pendingGun = client->newweapon;
	if ( pendingGun != nullptr ) {
		if ( pendingGun->id == weaponItemID ) {
			return;
		} // already in the process of switching to that gun, just be patient!
	}

	gitem_t * item = &itemlist[ weaponIndex ];
	if ( ( item->flags & IF_WEAPON ) == 0 ) {
		return;
	}

	if ( item->use == nullptr ) {
		return;
	}

	bot->client->no_weapon_chains = true;
	item->use( bot, item );

	if ( instantSwitch ) {
		// FIXME: ugly, maybe store in client later
		const int temp_instant_weapon = g_instant_weapon_switch->integer;
		g_instant_weapon_switch->integer = 1;
		ChangeWeapon( bot );
		g_instant_weapon_switch->integer = temp_instant_weapon;
	}
}

/*
================
Bot_TriggerEdict
================
*/
void Bot_TriggerEdict( edict_t * bot, edict_t * edict ) {
	if ( !bot->inuse || !edict->inuse ) {
		return;
	}

	if ( ( bot->svflags & SVF_BOT ) == 0 ) {
		return;
	}

	if ( edict->use ) {
		edict->use( edict, bot, bot );
	}

	trace_t unUsed;
	if ( edict->touch ) {
		edict->touch( edict, bot, unUsed, true );
	}
}

/*
================
Bot_UseItem
================
*/
void Bot_UseItem( edict_t * bot, const int32_t itemID ) {
	if ( !bot->inuse ) {
		return;
	}

	if ( ( bot->svflags & SVF_BOT ) == 0 ) {
		return;
	}

	const item_id_t desiredItemID = item_id_t( itemID );
	bot->client->pers.selected_item = desiredItemID;

	ValidateSelectedItem( bot );

	if ( bot->client->pers.selected_item == IT_NULL  ) {
		return;
	}

	if ( bot->client->pers.selected_item != desiredItemID ) {
		return;
	} // the itemID changed on us - don't use it!

	gitem_t * item = &itemlist[ bot->client->pers.selected_item ];
	bot->client->pers.selected_item = IT_NULL;

	if ( item->use == nullptr ) {
		return;
	}

	bot->client->no_weapon_chains = true;
	item->use( bot, item );
}

/*
================
Bot_GetItemID
================
*/
int32_t Bot_GetItemID( const char * classname ) {
	if ( classname == nullptr || classname[ 0 ] == '\0' ) {
		return Item_Invalid;
	}

	if ( Q_strcasecmp( classname, "none" ) == 0 ) {
		return Item_Null;
	}

	for ( int i = 0; i < IT_TOTAL; ++i ) {
		const gitem_t * item = itemlist + i;
		if ( item->classname == nullptr || item->classname[ 0 ] == '\0' ) {
			continue;
		}

		if ( Q_strcasecmp( item->classname, classname ) == 0 ) {
			return item->id;
		}
	}

	return Item_Invalid;
}

/*
================
Edict_ForceLookAtPoint
================
*/
void Edict_ForceLookAtPoint( edict_t * edict, gvec3_cref_t point ) {
	vec3_t viewOrigin = edict->s.origin;
	if ( edict->client != nullptr ) {
		viewOrigin += edict->client->ps.viewoffset;
	}

	const vec3_t ideal = ( point - viewOrigin ).normalized();
	
	vec3_t viewAngles = vectoangles( ideal );
	if ( viewAngles.x < -180.0f ) {
		viewAngles.x = anglemod( viewAngles.x + 360.0f );
	}
	
	if ( edict->client != nullptr ) {
		edict->client->ps.pmove.delta_angles = ( viewAngles - edict->client->resp.cmd_angles );
		edict->client->ps.viewangles = {};
		edict->client->v_angle = {};
		edict->s.angles = {};
	}
}

/*
================
Bot_PickedUpItem

Check if the given bot has picked up the given item or not.
================
*/
bool Bot_PickedUpItem( edict_t * bot, edict_t * item ) {
	return item->item_picked_up_by[ ( bot->s.number - 1 ) ];
}