#include "g_local.h"

//cvar_t *jump;

char *jump_statusbar =
    "yb -24 "
    "if 9 "
    "xr 0 "
    "yb 0 "
    "xv 0 "
    "yv 0 "
    "pic 9 "
    "endif "
    //  Current Speed
    "xr -105 "
    "yt 2 "
    "string \"Current Speed\" "
    "xr -82 "
    "yt 12 "
    "num 6 1 "

    //  High Speed
    "xr -81 "
    "yt 34 "
    "string \"High Speed\" "
    "xr -82 "
    "yt 44 "
    "num 6 2 "

    //  last fall damage
    "xr -129 "
    "yt 66 "
    "string \"Last Fall Damage\" "
    "xr -82 "
    "yt 76 "
    "num 10 3 "
;

void Jmp_SetStats(edict_t *ent)
{
	vec3_t	velocity;
	vec_t	speed;

	// calculate speed
	VectorClear(velocity);
	VectorCopy(ent->velocity, velocity);
	speed = VectorNormalize(velocity);

	if(speed > ent->client->resp.jmp_highspeed)
		ent->client->resp.jmp_highspeed = speed;

	ent->client->ps.stats[STAT_SPEEDX] = speed;
	ent->client->ps.stats[STAT_HIGHSPEED] = ent->client->resp.jmp_highspeed;
	ent->client->ps.stats[STAT_FALLDMGLAST] = ent->client->resp.jmp_falldmglast;

	//
	// layouts
	//
	ent->client->ps.stats[STAT_LAYOUTS] = 0;

	if (ent->health <= 0 || level.intermission_framenum || ent->client->layout)
		ent->client->ps.stats[STAT_LAYOUTS] |= 1;
	if (ent->client->showinventory && ent->health > 0)
		ent->client->ps.stats[STAT_LAYOUTS] |= 2;

	SetIDView (ent);
}

void Jmp_EquipClient(edict_t *ent)
{
	memset(ent->client->inventory, 0, sizeof(ent->client->inventory));
	ent->client->weapon = 0;

	// make client non-solid
	ent->solid = SOLID_TRIGGER;
	AddToTransparentList(ent);
}

void Cmd_Jmod_f (edict_t *ent)
{
	char *cmd = NULL;

	if( ! jump->value )
	{
		gi.cprintf(ent, PRINT_HIGH, "The server does not have JumpMod enabled.\n");
		return;
	}

	if(gi.argc() < 2) {
		gi.cprintf(ent, PRINT_HIGH, "AQ2:TNG Jump mode commands:\n");
		gi.cprintf(ent, PRINT_HIGH, " jmod store - save your current point\n");
		gi.cprintf(ent, PRINT_HIGH, " jmod recall - teleport back to saved point\n");
		gi.cprintf(ent, PRINT_HIGH, " jmod reset - remove saved point\n");
		gi.cprintf(ent, PRINT_HIGH, " jmod clear - reset stats\n");
		return;
	}

	cmd = gi.argv(1);

	if(Q_stricmp(cmd, "store") == 0)
	{
		Cmd_Store_f(ent);
		return;
	}
	else if(Q_stricmp(cmd, "recall") == 0)
	{
		Cmd_Recall_f(ent);
		return;
	}
	else if(Q_stricmp(cmd, "reset") == 0)
	{
		Cmd_Reset_f(ent);
		return;
	}
	else if(Q_stricmp(cmd, "clear") == 0)
	{
		Cmd_Clear_f(ent);
		return;
	}

	gi.cprintf(ent, PRINT_HIGH, "Unknown jmod command\n");
}

void Cmd_Clear_f(edict_t *ent)
{
	ent->client->resp.jmp_highspeed = 0;
	ent->client->resp.jmp_falldmglast = 0;
	gi.cprintf(ent, PRINT_HIGH, "Statistics cleared\n");
}

void Cmd_Reset_f (edict_t *ent)
{
	VectorClear(ent->client->resp.jmp_teleport_origin);
	VectorClear(ent->client->resp.jmp_teleport_v_angle);
	gi.cprintf(ent, PRINT_HIGH, "Teleport location removed\n");
}

void Cmd_Store_f (edict_t *ent)
{
	if (ent->client->pers.spectator)
	{
		gi.cprintf(ent, PRINT_HIGH, "This command cannot be used by spectators\n");
		return;
	}

	VectorCopy (ent->s.origin, ent->client->resp.jmp_teleport_origin);
	VectorCopy(ent->client->v_angle, ent->client->resp.jmp_teleport_v_angle);

	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->client->resp.jmp_teleport_ducked = true;
	}

	gi.cprintf(ent, PRINT_HIGH, "Location stored\n");
}

void Cmd_Recall_f (edict_t *ent)
{
	int i;

	if (ent->deadflag || ent->client->pers.spectator)
	{
		gi.cprintf(ent, PRINT_HIGH, "This command cannot be used by spectators or dead players\n");
		return;
	}

	if(VectorLength(ent->client->resp.jmp_teleport_origin) == 0)
	{
		gi.cprintf(ent, PRINT_HIGH, "You must first \"store\" a location to teleport to\n");
		return;
	}

	ent->client->jumping = 0;

	ent->movetype = MOVETYPE_NOCLIP;

	/* teleport effect */
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TELEPORT_EFFECT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	gi.unlinkentity (ent);

	VectorCopy (ent->client->resp.jmp_teleport_origin, ent->s.origin);
	VectorCopy (ent->client->resp.jmp_teleport_origin, ent->s.old_origin);

	VectorClear (ent->velocity);

	ent->client->ps.pmove.pm_time = 160>>3;

	ent->s.event = EV_PLAYER_TELEPORT;

	VectorClear(ent->s.angles);
	VectorClear(ent->client->ps.viewangles);
	VectorClear(ent->client->ps.kick_angles);
	VectorClear(ent->client->v_angle);
	VectorClear(ent->client->ps.pmove.delta_angles);
	VectorClear(ent->client->kick_angles);
	ent->client->fall_time = 0;
	ent->client->fall_value = 0;

	VectorCopy(ent->client->resp.jmp_teleport_v_angle, ent->client->v_angle);
	VectorCopy(ent->client->v_angle, ent->client->ps.viewangles);

	for (i=0;i<2;i++)
		ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(ent->client->v_angle[i] - ent->client->resp.cmd_angles[i]);

	if (ent->client->resp.jmp_teleport_ducked)
		ent->client->ps.pmove.pm_flags = PMF_DUCKED;

	gi.linkentity (ent);

	/* teleport effect */
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TELEPORT_EFFECT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->movetype = MOVETYPE_WALK;
}
