// Domination Mode by Raptor007

#include "g_local.h"

cvar_t *dom = NULL;

unsigned int dom_team_effect[] = {
	EF_BLASTER | EF_TELEPORTER,
	EF_FLAG1,
	EF_FLAG2,
	EF_GREEN_LIGHT | EF_COLOR_SHELL
};

unsigned int dom_team_fx[] = {
	RF_GLOW,
	RF_FULLBRIGHT,
	RF_FULLBRIGHT,
	RF_SHELL_GREEN
};

int dom_flag_count = 0;
int dom_team_flags[ TEAM_TOP ] = {0};
int dom_winner = NOTEAM;
int dom_red_flag = 0, dom_blue_flag = 0;
int dom_pics[ TEAM_TOP ] = {0};
int dom_last_score = 0;


int DomFlagOwner( edict_t *flag )
{
	if( flag->s.effects == dom_team_effect[ TEAM1 ] )
		return TEAM1;
	if( flag->s.effects == dom_team_effect[ TEAM2 ] )
		return TEAM2;
	if( flag->s.effects == dom_team_effect[ TEAM3 ] )
		return TEAM3;
	return NOTEAM;
}


qboolean DomCheckRules( void )
{
	int max_score = dom_flag_count * ((teamCount == 3) ? 150 : 200);
	int winning_teams = 0;

	if( (int) level.time > dom_last_score )
	{
		dom_last_score = level.time;

		teams[ TEAM1 ].score += dom_team_flags[ TEAM1 ];
		teams[ TEAM2 ].score += dom_team_flags[ TEAM2 ];
		teams[ TEAM3 ].score += dom_team_flags[ TEAM3 ];
	}

	dom_winner = NOTEAM;

	if( max_score <= 0 )
		return true;

	if( teams[ TEAM1 ].score >= max_score )
	{
		dom_winner = TEAM1;
		winning_teams ++;
	}
	if( teams[ TEAM2 ].score >= max_score )
	{
		dom_winner = TEAM2;
		winning_teams ++;
	}
	if( teams[ TEAM3 ].score >= max_score )
	{
		dom_winner = TEAM3;
		winning_teams ++;
	}

	if( winning_teams == 1 )
	{
		// Winner: just show that they hit the score limit, not how far beyond they went.
		teams[ dom_winner ].score = max_score;
	}
	else if( winning_teams > 1 )
	{
		// Overtime: multiple teams hit the score limit.

		max_score = max(max( teams[ TEAM1 ].score, teams[ TEAM2 ].score ), teams[ TEAM3 ].score );
		winning_teams = 0;

		if( teams[ TEAM1 ].score == max_score )
		{
			dom_winner = TEAM1;
			winning_teams ++;
		}
		if( teams[ TEAM2 ].score == max_score )
		{
			dom_winner = TEAM2;
			winning_teams ++;
		}
		if( teams[ TEAM3 ].score == max_score )
		{
			dom_winner = TEAM3;
			winning_teams ++;
		}

		// Don't allow a tie.
		if( winning_teams > 1 )
			dom_winner = NOTEAM;
	}

	if( dom_winner != NOTEAM )
	{
		gi.bprintf( PRINT_HIGH, "%s team wins!\n", teams[ dom_winner ].name );
		return true;
	}

	return false;
}


void DomFlagThink( edict_t *flag )
{
	int prev = flag->s.frame;

	// If the flag was touched this frame, make it owned by that team.
	if( flag->owner && flag->owner->client && flag->owner->client->resp.team )
	{
		unsigned int effect = dom_team_effect[ flag->owner->client->resp.team ];
		if( flag->s.effects != effect )
		{
			char location[ 128 ] = "(";
			qboolean has_loc = false;
			edict_t *ent = NULL;
			int prev_owner = DomFlagOwner( flag );

			if( prev_owner != NOTEAM )
				dom_team_flags[ prev_owner ] --;

			flag->s.effects = effect;
			flag->s.renderfx = dom_team_fx[ flag->owner->client->resp.team ];
			dom_team_flags[ flag->owner->client->resp.team ] ++;

			if( flag->owner->client->resp.team == TEAM1 )
				flag->s.modelindex = dom_red_flag;
			else
				flag->s.modelindex = dom_blue_flag;

			// Get flag location if possible.
			has_loc = GetPlayerLocation( flag, location + 1 );
			if( has_loc )
				strcat( location, ") " );
			else
				location[0] = '\0';

			gi.bprintf( PRINT_HIGH, "%s secured %s flag %sfor %s!\n",
				flag->owner->client->pers.netname,
				(dom_flag_count == 1) ? "the" : "a",
				location,
				teams[ flag->owner->client->resp.team ].name );

			if( (dom_team_flags[ flag->owner->client->resp.team ] == dom_flag_count) && (dom_flag_count > 1) )
				gi.bprintf( PRINT_HIGH, "%s TEAM IS DOMINATING!\n",
				teams[ flag->owner->client->resp.team ].name );

			gi.sound( flag, CHAN_ITEM, gi.soundindex("tng/flagret.wav"), 0.75, 0.125, 0 );

			for( ent = g_edicts + 1; ent <= g_edicts + game.maxclients; ent ++ )
			{
				if( ! (ent->inuse && ent->client && ent->client->resp.team) )
					continue;
				else if( ent == flag->owner )
					unicastSound( ent, gi.soundindex("tng/flagcap.wav"), 0.75 );
				else if( ent->client->resp.team != flag->owner->client->resp.team )
					unicastSound( ent, gi.soundindex("tng/flagtk.wav"), 0.75 );
			}
		}
	}

	// Reset so the flag can be touched again.
	flag->owner = NULL;

	// Animate the flag waving.
	flag->s.frame = 173 + (((flag->s.frame - 173) + 1) % 16);

	// Blink between red and blue if it's unclaimed.
	if( (flag->s.frame < prev) && (flag->s.effects == dom_team_effect[ NOTEAM ]) )
		flag->s.modelindex = (flag->s.modelindex == dom_blue_flag) ? dom_red_flag : dom_blue_flag;

	flag->nextthink = level.framenum + FRAMEDIV;
}


void DomTouchFlag( edict_t *flag, edict_t *player, cplane_t *plane, csurface_t *surf )
{
	if( ! player->client )
		return;
	if( ! player->client->resp.team )
		return;
	if( (player->health < 1) || ! IS_ALIVE(player) )
		return;
	if( lights_camera_action || in_warmup )
		return;
	if( player->client->uvTime )
		return;

	// If the flag hasn't been touched this frame, the player will take it.
	if( ! flag->owner )
		flag->owner = player;
	// If somebody on another team also touched the flag this frame, nobody takes it.
	else if( flag->owner->client && (flag->owner->client->resp.team != player->client->resp.team) )
		flag->owner = flag;
}


void DomMakeFlag( edict_t *flag )
{
	vec3_t dest = {0};
	trace_t tr = {0};

	VectorSet( flag->mins, -15, -15, -15 );
	VectorSet( flag->maxs,  15,  15,  15 );

	// Put the flag on the ground.
	VectorCopy( flag->s.origin, dest );
	dest[2] -= 128;
	tr = gi.trace( flag->s.origin, flag->mins, flag->maxs, dest, flag, MASK_SOLID );
	if( ! tr.startsolid )
		VectorCopy( tr.endpos, flag->s.origin );

	VectorCopy( flag->s.origin, flag->old_origin );

	flag->solid = SOLID_TRIGGER;
	flag->movetype = MOVETYPE_NONE;
	flag->s.modelindex = dom_blue_flag;
	flag->s.skinnum = 0;
	flag->s.effects = dom_team_effect[ NOTEAM ];
	flag->s.renderfx = dom_team_fx[ NOTEAM ];
	flag->owner = NULL;
	flag->touch = DomTouchFlag;
	NEXT_KEYFRAME( flag, DomFlagThink );
	flag->classname = "item_flag";
	flag->svflags &= ~SVF_NOCLIENT;
	gi.linkentity( flag );

	dom_flag_count ++;
}


qboolean DomLoadConfig( const char *mapname )
{
	char buf[1024] = "";
	char *ptr = NULL;
	FILE *fh = NULL;
	size_t i = 0;

	gi.dprintf("-------------------------------------\n");

	dom_flag_count = 0;
	memset( &dom_team_flags, 0, sizeof(dom_team_flags) );
	dom_winner = NOTEAM;
	teams[ TEAM1 ].score = 0;
	teams[ TEAM2 ].score = 0;
	teams[ TEAM3 ].score = 0;
	dom_last_score = 0;
	dom_red_flag = gi.modelindex("models/flags/flag1.md2");
	dom_blue_flag = gi.modelindex("models/flags/flag2.md2");
	dom_pics[ TEAM1 ] = gi.imageindex(teams[ TEAM1 ].skin_index);
	dom_pics[ TEAM2 ] = gi.imageindex(teams[ TEAM2 ].skin_index);
	dom_pics[ TEAM3 ] = gi.imageindex(teams[ TEAM3 ].skin_index);

	if( teamCount == 3 )
	{
		// 3 team mode uses color shells because there's no green flag.
		dom_team_effect[ TEAM1 ] |= EF_COLOR_SHELL;
		dom_team_effect[ TEAM2 ] |= EF_COLOR_SHELL;
		dom_team_fx[ TEAM1 ] |= RF_SHELL_RED;
		dom_team_fx[ TEAM2 ] |= RF_SHELL_BLUE;
	}

	Com_sprintf( buf, sizeof(buf), "%s/tng/%s.dom", GAMEVERSION, mapname );
	fh = fopen( buf, "rt" );
	if( fh )
	{
		// Found a Domination config file for this map.

		gi.dprintf( "%s\n", buf );

		ptr = INI_Find( fh, "dom", "flags" );
		if( ptr )
			ptr = strchr( ptr, '<' );
		while( ptr )
		{
			edict_t *flag = G_Spawn();

			char *space = NULL, *end = strchr( ptr + 1, '>' );
			if( end )
				*end = '\0';

			flag->s.origin[0] = atof( ptr + 1 );
			space = strchr( ptr + 1, ' ' );
			if( space )
			{
				flag->s.origin[1] = atof( space );
				space = strchr( space + 1, ' ' );
				if( space )
				{
					flag->s.origin[2] = atof( space );
					space = strchr( space + 1, ' ' );
					if( space )
						flag->s.angles[YAW] = atof( space );
				}
			}

			DomMakeFlag( flag );
			ptr = strchr( (end ? end : ptr) + 1, '<' );
		}

		fclose( fh );
		fh = NULL;
	}

	if( dom_flag_count )
		gi.dprintf( "Domination mode: %i flags loaded.\n", dom_flag_count );
	else
	{
		// Try to automatically generate flags.

		edict_t *spawns[ 32 ] = {0}, *spot = NULL;
		int spawn_count = 0, need = 3;

		if(( spot = G_Find( NULL, FOFS(classname), "item_flag_team1" )) != NULL)
		{
			DomMakeFlag( spot );
			need --;
		}
		if(( spot = G_Find( NULL, FOFS(classname), "item_flag_team2" )) != NULL)
		{
			DomMakeFlag( spot );
			need --;
		}

		spot = NULL;
		while( ((spot = G_Find( spot, FOFS(classname), "info_player_deathmatch" )) != NULL) && (spawn_count < 32) )
		{
			spawns[ spawn_count ] = spot;
			spawn_count ++;
		}

		// If we have flags, don't convert scarce player spawns.
		if( dom_flag_count && (spawn_count <= 3) )
			need = 0;
		// Can't convert more spawns than we have.
		else if( need > spawn_count )
			need = spawn_count;
		else if( need < spawn_count )
		{
			// If we have plenty of choices, randomize which spawns we convert.
			for( i = 0; i < need; i ++ )
			{
				edict_t *swap = spawns[ i ];
				int index = rand() % spawn_count;
				spawns[ i ] = spawns[ index ];
				spawns[ index ] = swap;
			}
		}

		for( i = 0; i < need; i ++ )
		{
			if( spawn_count > 3 )
			{
				// Turn a spawn location into a flag.
				DomMakeFlag( spawns[ i ] );
				spawn_count --;
			}
			else if( ! dom_flag_count )
			{
				// We're desperate, so make a copy of a player spawn as a flag location.
				edict_t *flag = G_Spawn();
				VectorCopy( spawns[ i ]->s.origin, flag->s.origin );
				VectorCopy( spawns[ i ]->s.angles, flag->s.angles );
				DomMakeFlag( flag );
			}
		}

		if( dom_flag_count )
			gi.dprintf( "Domination mode: %i flags generated.\n", dom_flag_count );
		else
			gi.dprintf( "Warning: Domination needs flags in: tng/%s.dom\n", mapname );
	}

	gi.dprintf("-------------------------------------\n");

	return (dom_flag_count > 0);
}


void DomSetupStatusbar( void )
{
	Q_strncatz(level.statusbar, 
		// Red Team
		"yb -172 " "if 24 xr -24 pic 24 endif " "xr -92 num 4 26 "
		// Blue Team
		"yb -148 " "if 25 xr -24 pic 25 endif " "xr -92 num 4 27 ",
		sizeof(level.statusbar) );
	
	if( teamCount >= 3 )
	{
		Q_strncatz(level.statusbar, 
			// Green Team
			"yb -124 " "if 30 xr -24 pic 30 endif " "xr -92 num 4 31 ",
			sizeof(level.statusbar) );
	}
}


void SetDomStats( edict_t *ent )
{
	// Team scores for the score display and HUD.
	ent->client->ps.stats[ STAT_TEAM1_SCORE ] = teams[ TEAM1 ].score;
	ent->client->ps.stats[ STAT_TEAM2_SCORE ] = teams[ TEAM2 ].score;
	ent->client->ps.stats[ STAT_TEAM3_SCORE ] = teams[ TEAM3 ].score;

	// Team icons for the score display and HUD.
	ent->client->ps.stats[ STAT_TEAM1_PIC ] = dom_pics[ TEAM1 ];
	ent->client->ps.stats[ STAT_TEAM2_PIC ] = dom_pics[ TEAM2 ];
	ent->client->ps.stats[ STAT_TEAM3_PIC ] = dom_pics[ TEAM3 ];

	// During intermission, blink the team icon of the winning team.
	if( level.intermission_framenum && ((level.realFramenum / FRAMEDIV) & 8) )
	{
		if (dom_winner == TEAM1)
			ent->client->ps.stats[ STAT_TEAM1_PIC ] = 0;
		else if (dom_winner == TEAM2)
			ent->client->ps.stats[ STAT_TEAM2_PIC ] = 0;
		else if (dom_winner == TEAM3)
			ent->client->ps.stats[ STAT_TEAM3_PIC ] = 0;
	}
}
