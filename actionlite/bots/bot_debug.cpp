// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "../g_local.h"
#include "bot_utils.h"
#include "bot_debug.h"

static const edict_t * escortBot = nullptr;
static const edict_t * escortActor = nullptr;

static const edict_t * moveToPointBot = nullptr;
static vec3_t moveToPointPos = vec3_origin;

// how close the bot will try to get to the move to point goal
constexpr float moveToPointTolerance = 16.0f;

/*
================
ShowMonsterPathToPlayer
================
*/
void ShowMonsterPathToPlayer( const edict_t * player ) {
	const edict_t * monster = FindFirstMonster();
	if ( monster == nullptr ) {
		return;
	}

	const float moveDist = 8.0f;

	std::array<vec3_t, 512> pathPoints;

	PathRequest request;
	request.start = monster->s.origin;
	request.goal = player->s.origin;
	request.moveDist = moveDist;
	request.pathFlags = PathFlags::All;
	request.debugging.drawTime = 0.10f;
	request.nodeSearch.minHeight = 64.0f;
	request.nodeSearch.maxHeight = 64.0f;
	request.nodeSearch.radius = 512.0f;
	request.pathPoints.array = &pathPoints.front();
	request.pathPoints.count = pathPoints.size();

	PathInfo info;
	if ( gi.GetPathToGoal( request, info ) ) {
		// Do movement stuff....
		for ( int i = 0; i < info.numPathPoints; ++i ) {
			const gvec3_t & point = pathPoints[ i ];
			gi.Draw_Point( point, 8.0f, rgba_yellow, 0.10f, false );
		}
	}
}

/*
================
UpdateFollowActorDebug

Set cvar "bot_debug_follow_actor" to 1
and then run your cursor over any player/monster to pick
that "actor" for the bot to follow. 

When successful, you will see the player/monster highlighted
with a yellow box, and the bot will follow them around the map until 
the actor they're following dies, or the bot is told to do something
else by you.

Check the console for debugging feedback...
================
*/
void UpdateFollowActorDebug( const edict_t * localPlayer ) {
	if ( bot_debug_follow_actor->integer ) {
		if ( bot_debug_follow_actor->integer == 1 ) {
			escortBot = FindFirstBot();
			escortActor = FindActorUnderCrosshair( localPlayer );

			if ( gi.Bot_FollowActor( escortBot, escortActor ) != GoalReturnCode::Error ) {
				gi.cvar_set( "bot_debug_follow_actor", "2" );
				gi.Com_Print( "Follow_Actor: Bot Found Actor To Follow!\n" );
			} else {
				gi.Com_Print( "Follow_Actor: Hover Over Monster/Player To Follow...\n" );
			}
		} else {
			if ( gi.Bot_FollowActor( escortBot, escortActor ) != GoalReturnCode::Error ) {
				gi.Draw_Bounds( escortActor->absmin, escortActor->absmax, rgba_yellow, gi.frame_time_s, false );
				gi.Draw_Bounds( escortBot->absmin, escortBot->absmax, rgba_cyan, gi.frame_time_s, false );
			} else {
				gi.Com_Print( "Follow_Actor: Bot Or Actor Removed...\n" );
				gi.cvar_set( "bot_debug_follow_actor", "0" );
			}
		}
	} else {
		escortBot = nullptr;
		escortActor = nullptr;
	}
}

/*
================
UpdateMoveToPointDebug

Set cvar "bot_debug_move_to_point" to 1,
look anywhere in world you'd like the bot to move to, 
and then fire your weapon. The point at the end of your crosshair
will be the point in the world the bot will move toward.

When successful, a point marker will be drawn where the bot will move
toward, and the bot itself will have a box drawn around it.

Once bot reaches the point, it will clear the goal and go about it's 
business until you give it something else to do.

Check the console for debugging feedback...
================
*/
void UpdateMoveToPointDebug( const edict_t * localPlayer ) {
	if ( bot_debug_move_to_point->integer ) {
		if ( bot_debug_move_to_point->integer == 1 ) {
			if ( localPlayer->client->buttons & BUTTON_ATTACK ) {
				vec3_t localPlayerForward, right, up;
				AngleVectors( localPlayer->client->v_angle, localPlayerForward, right, up );

				const vec3_t localPlayerViewPos = ( localPlayer->s.origin + vec3_t{ 0.0f, 0.0f, (float)localPlayer->viewheight } );
				const vec3_t end = ( localPlayerViewPos + ( localPlayerForward * 8192.0f ) );
				const contents_t mask = ( MASK_PROJECTILE & ~CONTENTS_DEADMONSTER );

				trace_t tr = gi.traceline( localPlayerViewPos, end, localPlayer, mask );
				moveToPointPos = tr.endpos;

				moveToPointBot = FindFirstBot();
				if ( gi.Bot_MoveToPoint( moveToPointBot, moveToPointPos, moveToPointTolerance ) != GoalReturnCode::Error ) {
					gi.cvar_set( "bot_debug_move_to_point", "2" );
					gi.Com_Print( "Move_To_Point: Bot Has Position To Move Toward!\n" );
				}
			} else {
				gi.Com_Print( "Move_To_Point: Fire Weapon To Select Move Point...\n" );
			}
		} else {
			const GoalReturnCode result = gi.Bot_MoveToPoint( moveToPointBot, moveToPointPos, moveToPointTolerance );
			if ( result == GoalReturnCode::Error ) {
				gi.cvar_set( "bot_debug_move_to_point", "0" );
				gi.Com_Print( "Move_To_Point: Bot Can't Reach Goal Position!\n" );
			} else if ( result == GoalReturnCode::Finished ) {
				gi.cvar_set( "bot_debug_move_to_point", "0" );
				gi.Com_Print( "Move_To_Point: Bot Reached Goal Position!\n" );
			} else {
				gi.Draw_Point( moveToPointPos, 8.0f, rgba_yellow, gi.frame_time_s, false );
				gi.Draw_Bounds( moveToPointBot->absmin, moveToPointBot->absmax, rgba_cyan, gi.frame_time_s, false );
			}
		}
	} else {
		moveToPointBot = nullptr;
		moveToPointPos = vec3_origin;
	}
}

/*
================
Bot_UpdateDebug
================
*/
void Bot_UpdateDebug() {
	if ( !sv_cheats->integer ) {
		return;
	}

	const edict_t * localPlayer = FindLocalPlayer();
	if ( localPlayer == nullptr ) {
		return;
	}

	if ( g_debug_monster_paths->integer == 2 ) {
		ShowMonsterPathToPlayer( localPlayer );
	}

	UpdateFollowActorDebug( localPlayer );

	UpdateMoveToPointDebug( localPlayer );
}