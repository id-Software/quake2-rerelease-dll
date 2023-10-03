// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "../g_local.h"
#include "../m_player.h"
#include "bot_utils.h"

constexpr int Team_Coop_Monster = 0;

/*
================
Player_UpdateState
================
*/
void Player_UpdateState( edict_t * player ) {
	const client_persistant_t & persistant = player->client->pers;

	player->sv.ent_flags = SVFL_NONE;
	if ( player->groundentity != nullptr || ( player->flags & FL_PARTIALGROUND ) != 0 ) {
		player->sv.ent_flags |= SVFL_ONGROUND;
	} else {
		if ( player->client->ps.pmove.pm_flags & PMF_JUMP_HELD ) {
			player->sv.ent_flags |= SVFL_IS_JUMPING;
		}
	}

	if ( player->client->ps.pmove.pm_flags & PMF_ON_LADDER ) {
		player->sv.ent_flags |= SVFL_ON_LADDER;
	}

	if ( ( player->client->ps.pmove.pm_flags & PMF_DUCKED ) != 0 ) {
		player->sv.ent_flags |= SVFL_IS_CROUCHING;
	}

	if ( player->client->quad_time > level.time ) {
		player->sv.ent_flags |= SVFL_HAS_DMG_BOOST;
	} else if ( player->client->quadfire_time > level.time ) {
		player->sv.ent_flags |= SVFL_HAS_DMG_BOOST;
	} else if ( player->client->double_time > level.time ) {
		player->sv.ent_flags |= SVFL_HAS_DMG_BOOST;
	}

	if ( player->client->invincible_time > level.time ) {
		player->sv.ent_flags |= SVFL_HAS_PROTECTION;
	}

	if ( player->client->invisible_time > level.time ) {
		player->sv.ent_flags |= SVFL_HAS_INVISIBILITY;
	}

	if ( ( player->client->ps.pmove.pm_flags & PMF_TIME_TELEPORT ) != 0 ) {
		player->sv.ent_flags |= SVFL_HAS_TELEPORTED;
	}

	if ( player->takedamage ) {
		player->sv.ent_flags |= SVFL_TAKES_DAMAGE;
	}

	if ( player->solid == SOLID_NOT ) {
		player->sv.ent_flags |= SVFL_IS_HIDDEN;
	}

	if ( ( player->flags & FL_INWATER ) != 0 ) {
		if ( player->waterlevel >= WATER_WAIST ) {
			player->sv.ent_flags |= SVFL_IN_WATER;
		}
	}

	if ( ( player->flags & FL_NOTARGET ) != 0 ) {
		player->sv.ent_flags |= SVFL_NO_TARGET;
	}

	if ( ( player->flags & FL_GODMODE ) != 0 ) {
		player->sv.ent_flags |= SVFL_GOD_MODE;
	}

	if ( player->movetype == MOVETYPE_NOCLIP ) {
		player->sv.ent_flags |= SVFL_IS_NOCLIP;
	}

	if ( player->client->anim_end == FRAME_flip12 ) {
		player->sv.ent_flags |= SVFL_IS_FLIPPING_OFF;
	}

	if ( player->client->anim_end == FRAME_salute11 ) {
		player->sv.ent_flags |= SVFL_IS_SALUTING;
	}

	if ( player->client->anim_end == FRAME_taunt17 ) {
		player->sv.ent_flags |= SVFL_IS_TAUNTING;
	}

	if ( player->client->anim_end == FRAME_wave11 ) {
		player->sv.ent_flags |= SVFL_IS_WAVING;
	}

	if ( player->client->anim_end == FRAME_point12 ) {
		player->sv.ent_flags |= SVFL_IS_POINTING;
	}

	if ( ( player->client->ps.pmove.pm_flags & PMF_DUCKED ) == 0 && player->client->anim_priority <= ANIM_WAVE ) {
		player->sv.ent_flags |= SVFL_CAN_GESTURE;
	}

	if ( player->lastMOD.id == MOD_TELEFRAG || player->lastMOD.id == MOD_TELEFRAG_SPAWN ) {
		player->sv.ent_flags |= SVFL_WAS_TELEFRAGGED;
	}

	if ( player->client->resp.spectator ) {
		player->sv.ent_flags |= SVFL_IS_SPECTATOR;
	}

	player_skinnum_t pl_skinnum;
	pl_skinnum.skinnum = player->s.skinnum;
	player->sv.team = pl_skinnum.team_index;

	player->sv.buttons = player->client->buttons;

	const item_id_t armorType = ArmorIndex( player );
	player->sv.armor_type = armorType;
	player->sv.armor_value = persistant.inventory[ armorType ];

	player->sv.health = ( player->deadflag != true ) ? player->health : -1;
	player->sv.weapon = ( persistant.weapon != nullptr ) ? persistant.weapon->id : IT_NULL;

	player->sv.last_attackertime = static_cast<int32_t>( player->client->last_attacker_time.milliseconds() );
	player->sv.respawntime = static_cast<int32_t>( player->client->respawn_time.milliseconds() );
	player->sv.waterlevel = player->waterlevel;
	player->sv.viewheight = player->viewheight;

	player->sv.viewangles = player->client->v_angle;
	player->sv.viewforward = player->client->v_forward;
	player->sv.velocity = player->velocity;

	player->sv.ground_entity = player->groundentity;
	player->sv.enemy = player->enemy;

	static_assert( sizeof( persistant.inventory ) <= sizeof( player->sv.inventory ) );
	memcpy( &player->sv.inventory, &persistant.inventory, sizeof( persistant.inventory ) );

	if ( !player->sv.init ) {
		player->sv.init = true;
		player->sv.classname = player->classname;
		player->sv.targetname = player->targetname;
		player->sv.lobby_usernum = P_GetLobbyUserNum( player );
		player->sv.starting_health = player->health;
		player->sv.max_health = player->max_health;

		// NOTE: entries are assumed to be ranked with the first armor assumed
		// NOTE: to be the "best", and last the "worst". You don't need to add
		// NOTE: entries for things like armor shards, only actual armors.
		// NOTE: Check "Max_Armor_Types" to raise/lower the armor count.
		armorInfo_t * armorInfo = player->sv.armor_info;
		armorInfo[ 0 ].item_id = IT_ARMOR_BODY;
		armorInfo[ 0 ].max_count = bodyarmor_info.max_count;
		armorInfo[ 1 ].item_id = IT_ARMOR_COMBAT;
		armorInfo[ 1 ].max_count = combatarmor_info.max_count;
		armorInfo[ 2 ].item_id = IT_ARMOR_JACKET;
		armorInfo[ 2 ].max_count = jacketarmor_info.max_count;

		gi.Info_ValueForKey( player->client->pers.userinfo, "name", player->sv.netname, sizeof( player->sv.netname ) );

		gi.Bot_RegisterEdict( player );
	}
}

/*
================
Monster_UpdateState
================
*/
void Monster_UpdateState( edict_t * monster ) {
	monster->sv.ent_flags = SVFL_NONE;
	if ( monster->groundentity != nullptr ) {
		monster->sv.ent_flags |= SVFL_ONGROUND;
	}

	if ( monster->takedamage ) {
		monster->sv.ent_flags |= SVFL_TAKES_DAMAGE;
	}

	if ( monster->solid == SOLID_NOT || monster->movetype == MOVETYPE_NONE ) {
		monster->sv.ent_flags |= SVFL_IS_HIDDEN;
	}

	if ( ( monster->flags & FL_INWATER ) != 0 ) {
		monster->sv.ent_flags |= SVFL_IN_WATER;
	}

	if ( coop->integer ) {
		monster->sv.team = Team_Coop_Monster;
	} else {
		monster->sv.team = Team_None; // TODO: CTF/TDM/etc...
	}

	monster->sv.health = ( monster->deadflag != true ) ? monster->health : -1;
	monster->sv.waterlevel = monster->waterlevel;
	monster->sv.enemy = monster->enemy;
	monster->sv.ground_entity = monster->groundentity;

	int32_t viewHeight = monster->viewheight;
	if ( ( monster->monsterinfo.aiflags & AI_DUCKED ) != 0 ) {
		viewHeight = int32_t( monster->maxs[ 2 ] - 4.0f );
	}
	monster->sv.viewheight = viewHeight;

	monster->sv.viewangles = monster->s.angles;

	AngleVectors( monster->s.angles, monster->sv.viewforward, nullptr, nullptr );

	monster->sv.velocity = monster->velocity;

	if ( !monster->sv.init ) {
		monster->sv.init = true;
		monster->sv.classname = monster->classname;
		monster->sv.targetname = monster->targetname;
		monster->sv.starting_health = monster->health;
		monster->sv.max_health = monster->max_health;

		gi.Bot_RegisterEdict( monster );
	}
}

/*
================
Item_UpdateState
================
*/
void Item_UpdateState( edict_t * item ) {
	item->sv.ent_flags = SVFL_IS_ITEM;
	item->sv.respawntime = 0;

	if ( item->team != nullptr ) {
		item->sv.ent_flags |= SVFL_IN_TEAM;
	} // some DM maps have items chained together in teams...

	if ( item->solid == SOLID_NOT ) {
		item->sv.ent_flags |= SVFL_IS_HIDDEN;

		if ( item->nextthink.milliseconds() > 0 ) {
			if ( ( item->svflags & SVF_RESPAWNING ) != 0 ) {
				const gtime_t pendingRespawnTime = ( item->nextthink - level.time );
				item->sv.respawntime = static_cast<int32_t>( pendingRespawnTime.milliseconds() );
			} else {
				// item will respawn at some unknown time in the future...
				item->sv.respawntime = Item_UnknownRespawnTime;
			}
		}
	}

	const item_id_t itemID = item->item->id;
	if ( itemID == IT_FLAG1 || itemID == IT_FLAG2 ) {
		item->sv.ent_flags |= SVFL_IS_OBJECTIVE;
		// TODO: figure out if the objective is dropped/carried/home...
	}

	// always need to update these for items, since random item spawning
	// could change them at any time...
	item->sv.classname = item->classname;
	item->sv.item_id = item->item->id;

	if ( !item->sv.init ) {
		item->sv.init = true;
		item->sv.targetname = item->targetname;

		gi.Bot_RegisterEdict( item );
	}
}

/*
================
Trap_UpdateState
================
*/
void Trap_UpdateState( edict_t * danger ) {
	danger->sv.ent_flags = SVFL_TRAP_DANGER;
	danger->sv.velocity = danger->velocity;

	if ( danger->owner != nullptr && danger->owner->client != nullptr ) {
		player_skinnum_t pl_skinnum;
		pl_skinnum.skinnum = danger->owner->s.skinnum;
		danger->sv.team = pl_skinnum.team_index;
	}

	if ( danger->groundentity != nullptr ) {
		danger->sv.ent_flags |= SVFL_ONGROUND;
	}

	if ( ( danger->flags & FL_TRAP_LASER_FIELD ) == 0 ) {
		danger->sv.ent_flags |= SVFL_ACTIVE; // non-lasers are always active
	} else {
		danger->sv.start_origin = danger->s.origin;
		danger->sv.end_origin = danger->s.old_origin;
		if ( ( danger->svflags & SVF_NOCLIENT ) == 0 ) {
			if ( ( danger->s.renderfx & RF_BEAM ) ) {
				danger->sv.ent_flags |= SVFL_ACTIVE; // lasers are active!!
			}
		}
	}

	if ( !danger->sv.init ) {
		danger->sv.init = true;
		danger->sv.classname = danger->classname;

		gi.Bot_RegisterEdict( danger );
	}
}

/*
================
Edict_UpdateState
================
*/
void Edict_UpdateState( edict_t * edict ) {
	edict->sv.ent_flags = SVFL_NONE;
	edict->sv.health = edict->health;

	if ( edict->takedamage ) {
		edict->sv.ent_flags |= SVFL_TAKES_DAMAGE;
	}

	// plats, movers, and doors use this to determine move state.
	const bool isDoor = ( ( edict->svflags & SVF_DOOR ) != 0 );
	const bool isReversedDoor = ( isDoor && edict->spawnflags.has( SPAWNFLAG_DOOR_REVERSE ) );

	// doors have their top/bottom states reversed from plats
	// ( unless "reverse" spawnflag is set! )
	if ( isDoor && !isReversedDoor ) {
		if ( edict->moveinfo.state == STATE_TOP ) {
			edict->sv.ent_flags |= SVFL_MOVESTATE_BOTTOM;
		} else if ( edict->moveinfo.state == STATE_BOTTOM ) {
			edict->sv.ent_flags |= SVFL_MOVESTATE_TOP;
		}
	} else {
		if ( edict->moveinfo.state == STATE_TOP ) {
			edict->sv.ent_flags |= SVFL_MOVESTATE_TOP;
		} else if ( edict->moveinfo.state == STATE_BOTTOM ) {
			edict->sv.ent_flags |= SVFL_MOVESTATE_BOTTOM;
		}
	}

	if ( edict->moveinfo.state == STATE_UP || edict->moveinfo.state == STATE_DOWN ) {
		edict->sv.ent_flags |= SVFL_MOVESTATE_MOVING;
 	} 

	edict->sv.start_origin = edict->moveinfo.start_origin;
	edict->sv.end_origin = edict->moveinfo.end_origin;

	if ( edict->svflags & SVF_DOOR ) {
		if ( edict->flags & FL_LOCKED ) {
			edict->sv.ent_flags |= SVFL_IS_LOCKED_DOOR;
		}
	}

	if ( !edict->sv.init ) {
		edict->sv.init = true;
		edict->sv.classname = edict->classname;
		edict->sv.targetname = edict->targetname;
		edict->sv.spawnflags = edict->spawnflags.value;
	}
}

/*
================
Entity_UpdateState
================
*/
void Entity_UpdateState( edict_t * edict ) {
	if ( edict->svflags & SVF_MONSTER ) {
		Monster_UpdateState( edict );
	} else if ( edict->flags & FL_TRAP || edict->flags & FL_TRAP_LASER_FIELD ) {
		Trap_UpdateState( edict );
	} else if ( edict->item != nullptr ) {
		Item_UpdateState( edict );
	} else if ( edict->client != nullptr ) {
		Player_UpdateState( edict );
	} else {
		Edict_UpdateState( edict );
	}
}

USE( info_nav_lock_use ) ( edict_t * self, edict_t * other, edict_t * activator ) -> void {
	edict_t * n = nullptr;

	while ( ( n = G_FindByString<&edict_t::targetname>( n, self->target ) ) ) {
		if ( !( n->svflags & SVF_DOOR ) ) {
			gi.Com_PrintFmt( "{} tried targeting {}, a non-SVF_DOOR\n", *self, *n );
			continue;
		}

		n->flags ^= FL_LOCKED;
	}
}

/*QUAKED info_nav_lock (1.0 1.0 0.0) (-16 -16 0) (16 16 32)
toggle locked state on linked entity
*/
void SP_info_nav_lock( edict_t * self ) {
	if ( !self->targetname ) {
		gi.Com_PrintFmt( "{} missing targetname\n", *self );
		G_FreeEdict( self );
		return;
	}

	if ( !self->target ) {
		gi.Com_PrintFmt( "{} missing target\n", *self );
		G_FreeEdict( self );
		return;
	}

	self->svflags |= SVF_NOCLIENT;
	self->use = info_nav_lock_use;
}

/*
================
FindLocalPlayer
================
*/
const edict_t * FindLocalPlayer() {
	const edict_t * localPlayer = nullptr;

	const edict_t * ent = &g_edicts[ 0 ];
	for ( uint32_t i = 0; i < globals.num_edicts; i++, ent++ ) {
		if ( !ent->inuse || !( ent->svflags & SVF_PLAYER ) ) {
			continue;
		}

		if ( ent->health <= 0 ) {
			continue;
		}

		localPlayer = ent;
		break;
	}

	return localPlayer;
}

/*
================
FindFirstBot
================
*/
const edict_t * FindFirstBot() {
	const edict_t * firstBot = nullptr;

	const edict_t * ent = &g_edicts[ 0 ];
	for ( uint32_t i = 0; i < globals.num_edicts; i++, ent++ ) {
		if ( !ent->inuse || !( ent->svflags & SVF_PLAYER ) ) {
			continue;
		}

		if ( ent->health <= 0 ) {
			continue;
		}

		if ( !( ent->svflags & SVF_BOT ) ) {
			continue;
		}

		firstBot = ent;
		break;
	}

	return firstBot;
}

/*
================
FindFirstMonster
================
*/
const edict_t * FindFirstMonster() {
	const edict_t * firstMonster = nullptr;

	const edict_t * ent = &g_edicts[ 0 ];
	for ( uint32_t i = 0; i < globals.num_edicts; i++, ent++ ) {
		if ( !ent->inuse || !( ent->svflags & SVF_MONSTER ) ) {
			continue;
		}

		if ( ent->health <= 0 ) {
			continue;
		}

		firstMonster = ent;
		break;
	}

	return firstMonster;
}

/*
================
FindFirstMonster

"Actors" are either players or monsters - i.e. something alive and thinking.
================
*/
const edict_t * FindActorUnderCrosshair( const edict_t * player ) {
	if ( player == nullptr || !player->inuse ) {
		return nullptr;
	}

	vec3_t forward, right, up;
	AngleVectors( player->client->v_angle, forward, right, up );

	const vec3_t eye_position = ( player->s.origin + vec3_t{ 0.0f, 0.0f, (float)player->viewheight } );
	const vec3_t end = ( eye_position + ( forward * 8192.0f ) );
	const contents_t mask = ( MASK_PROJECTILE & ~CONTENTS_DEADMONSTER );

	trace_t tr = gi.traceline( eye_position, end, player, mask );

	const edict_t * traceEnt = tr.ent;
	if ( traceEnt == nullptr || !tr.ent->inuse ) {
		return nullptr;
	}

	if ( !( traceEnt->svflags & SVF_PLAYER ) && !( traceEnt->svflags & SVF_MONSTER ) ) {
		return nullptr;
	}

	if ( traceEnt->health <= 0 ) {
		return nullptr;
	}

	return traceEnt;
}
