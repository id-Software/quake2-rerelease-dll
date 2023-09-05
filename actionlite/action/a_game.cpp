#include "../g_local.h"

#define MAX_MAP_ROTATION        1000	// just in case...
#define MAX_STR_LEN             1000
#define MAX_TOTAL_MOTD_LINES    30

char *map_rotation[MAX_MAP_ROTATION];
int num_maps, cur_map, rand_map, num_allvotes;	// num_allvotes added by Igor[Rock]

char motd_lines[MAX_TOTAL_MOTD_LINES][40];
int motd_num_lines;

/*
 * ReadConfigFile()
 * Config file format is backwards compatible with Action's, but doesn't need 
 * the "###" designator at end of sections.
 * -Fireblade
 */
void ReadConfigFile()
{
	FILE *config_file;
	char buf[MAX_STR_LEN], reading_section[MAX_STR_LEN], inipath[MAX_STR_LEN];
	int lines_into_section = -1;
	cvar_t *ininame;

	ininame = gi.cvar("ininame", "action.ini", CVAR_NOFLAGS);
	if (ininame->string && *(ininame->string))
		sprintf(inipath, "%s/%s", GAMEVERSION, ininame->string);
	else
		sprintf(inipath, "%s/%s", GAMEVERSION, "action.ini");

	config_file = fopen(inipath, "r");
	if (config_file == NULL) {
		gi.Com_PrintFmt("Unable to read %s\n", inipath);
		return;
	}

	while (fgets(buf, MAX_STR_LEN - 10, config_file) != NULL) {
		int bs;

		bs = strlen(buf);
		while (buf[bs - 1] == '\r' || buf[bs - 1] == '\n') {
			buf[bs - 1] = 0;
			bs--;
		}

		if ((buf[0] == '/' && buf[1] == '/') || buf[0] == 0) {
			continue;
		}

		if (buf[0] == '[') {
			char *p;

			p = strchr(buf, ']');
			if (p == NULL)
				continue;
			*p = 0;
			strcpy(reading_section, buf + 1);
			lines_into_section = 0;
			continue;
		}
		if (buf[0] == '#' && buf[1] == '#' && buf[2] == '#') {
			lines_into_section = -1;
			continue;
		}
		if (lines_into_section > -1) {
			if (!strcmp(reading_section, "team1")) {
				if (lines_into_section == 0) {
					Q_strlcpy(teams[TEAM1].name, buf, sizeof(teams[TEAM1].name));
				} else if (lines_into_section == 1) {
					Q_strlcpy(teams[TEAM1].skin, buf, sizeof(teams[TEAM1].skin));
				}
			} else if (!strcmp(reading_section, "team2")) {
				if (lines_into_section == 0) {
					Q_strlcpy(teams[TEAM2].name, buf, sizeof(teams[TEAM2].name));
				} else if (lines_into_section == 1) {
					Q_strlcpy(teams[TEAM2].skin, buf, sizeof(teams[TEAM2].skin));
				}
			} else if (!strcmp(reading_section, "team3")) {
				if (lines_into_section == 0) {
					Q_strlcpy(teams[TEAM3].name, buf, sizeof(teams[TEAM3].name));
				} else if (lines_into_section == 1) {
					Q_strlcpy(teams[TEAM3].skin, buf, sizeof(teams[TEAM3].skin));
				}
			} else if (!strcmp(reading_section, "maplist")) {
				map_rotation[num_maps] = (char *) gi.TagMalloc(strlen(buf) + 1, TAG_GAME);
				strcpy(map_rotation[num_maps], buf);
				num_maps++;
			}
			lines_into_section++;
		}
	}

	snprintf(teams[TEAM1].skin_index, sizeof(teams[TEAM1].skin_index), "../players/%s_i", teams[TEAM1].skin);
	snprintf(teams[TEAM2].skin_index, sizeof(teams[TEAM2].skin_index), "../players/%s_i", teams[TEAM2].skin);
	snprintf(teams[TEAM3].skin_index, sizeof(teams[TEAM3].skin_index), "../players/%s_i", teams[TEAM3].skin);

	cur_map = 0;
	srand(time(NULL));
	rand_map = (num_maps > 1) ? (rand() % (num_maps - 1) + 1) : 1;

	fclose(config_file);
}

void ReadMOTDFile()
{
	FILE *motd_file;
	char buf[1000];
	char motdpath[MAX_STR_LEN];
	int lbuf;
	cvar_t *motdname;

	motdname = gi.cvar("motdname", "motd.txt", CVAR_NOFLAGS);
	if (motdname->string && *(motdname->string))
		sprintf(motdpath, "%s/%s", GAMEVERSION, motdname->string);
	else
		sprintf(motdpath, "%s/%s", GAMEVERSION, "motd.txt");

	motd_file = fopen(motdpath, "r");
	if (motd_file == NULL)
		return;

	motd_num_lines = 0;
	while (fgets(buf, 900, motd_file) != NULL) {
		lbuf = strlen(buf);
		while (lbuf > 0 && (buf[lbuf - 1] == '\r' || buf[lbuf - 1] == '\n')) {
			buf[lbuf - 1] = 0;
			lbuf--;
		}

		if(!lbuf)
			continue;

		if (lbuf > 39)
			buf[39] = 0;

		strcpy(motd_lines[motd_num_lines++], buf);

		if (motd_num_lines >= MAX_TOTAL_MOTD_LINES)
			break;
	}

	fclose(motd_file);
}

// AQ2:TNG Deathwatch - Ohh, lovely MOTD - edited it
void PrintMOTD(edict_t * ent)
{
	int mapnum, i, lines = 0;
	int max_lines = MAX_TOTAL_MOTD_LINES;
	char msg_buf[1024];
	const char *server_type;


	//Welcome Message. This shows the Version Number and website URL, followed by an empty line
	strcpy(msg_buf, TNG_TITLE " v" VERSION "\n" "https://github.com/actionquake/quake2-rerelease-dll" "\n\n");
	lines = 3;

	/*
	   As long as we don't skip the MOTD, we want to print all information
	 */
	//if (!skipmotd->value) {
		// This line will show the hostname. If not set, the default name will be "Unnamed TNG Server" (used to be "unnamed")
		
		// TODO: When hostnames become a thing, re-enable this
		//if (hostname->string[0] && strcmp(hostname->string, "Unnamed TNG Server"))
		
		const char *hostname = "An Action Server";
		Q_strlcat(msg_buf, hostname, strlen(msg_buf)+40);
		strcat(msg_buf, "\n");
		lines++;
		

		/* 
		   Now all the settings
		 */

		// Check what game type it is
		if (teamplay->value)
		{
			if (teamdm->value) // Is it TeamDM?
			{
				if (teamCount == 3)
					server_type = "3 Team Deathmatch";
				else
					server_type = "Team Deathmatch";
			}
		}
		else  // So it's not Teamplay?
		{
            server_type = "Deathmatch (Free For All)";
			sprintf(msg_buf + strlen(msg_buf), "Game Type: %s\n", server_type);
		}
		lines++;

		// Adding an empty line
		strcat(msg_buf, "\n");
		lines++;

		/*
		   Now for the map rules, such as Timelimit, Roundlimit, etc
		 */
		if (fraglimit->integer) // What is the fraglimit?
			sprintf(msg_buf + strlen(msg_buf), "Fraglimit: %d", fraglimit->integer);
		else
			strcat(msg_buf, "Fraglimit: none");

		if (timelimit->integer) // What is the timelimit?
			sprintf(msg_buf + strlen(msg_buf), "  Timelimit: %d\n", timelimit->integer);
		else
			strcat(msg_buf, "  Timelimit: none\n");
		lines++;

		// If we're in Teamplay, and not CTF, we want to see what the roundlimit and roundtimelimit is
		if (gameSettings & GS_ROUNDBASED)
		{
			if ((int)roundlimit->value) // What is the roundlimit?
				sprintf(msg_buf + strlen(msg_buf), "Roundlimit: %d", (int)roundlimit->value);
			else
				strcat(msg_buf, "Roundlimit: none");
			lines++;
		}

		/*
		   Check for the number of weapons and items people can carry
		 */
		if ((int)unique_weapons->value != 1 || (int)unique_items->value != 1) {
			sprintf(msg_buf + strlen(msg_buf), "Max number of spec weapons: %d  items: %d\n",
				(int) unique_weapons->value, (int) unique_items->value);
			lines++;
		}

		/*
		   What can we use with the Bandolier?
		 */
		if (tgren->value > 0 || !(ir->value)) {
			char grenade_num[32];

			// Show the number of grenades with the Bandolier
			if (tgren->value > 0)
				sprintf(grenade_num, "%d grenade%s", (int)tgren->value, (int)tgren->value == 1 ? "" : "s");

			sprintf(msg_buf + strlen(msg_buf), "Bandolier w/ %s%s%s\n",
				!(ir->value) ? "no IR" : "",
				(tgren->value > 0 && !(ir->value)) ? " & " : "",
				tgren->value > 0 ? grenade_num : "");
			lines++;
		}

		/*
		   Is allitem and/or allweapon enabled?
		 */
		if (allitem->value || allweapon->value) {
			sprintf(msg_buf + strlen(msg_buf), "Players receive %s%s%s\n",
				allweapon->value ? "all weapons" : "",
				(allweapon->value && allitem->value) ? " & " : "",
				allitem->value ? "all items" : "");
			lines++;
		}

		/*
		 * Are we using limchasecam?
		 */
		if (limchasecam->value) {
			if ((int) limchasecam->value == 2)
				sprintf(msg_buf + strlen(msg_buf), "Chase Cam Disallowed\n");
			else
				sprintf(msg_buf + strlen(msg_buf), "Chase Cam Restricted\n");
			lines++;
		}

		/*
		 *  Are the dmflags set to disallow Friendly Fire?
		 */
		if (teamplay->value && !g_friendly_fire->integer) {
			sprintf(msg_buf + strlen(msg_buf), "Friendly Fire Enabled\n");
			lines++;
		}

		/*
		   Are we using any types of voting?
		 */
		// if (use_mapvote->value || use_cvote->value || use_kickvote->value) {
		// 	sprintf(msg_buf + strlen(msg_buf), "Vote Types: %s%s%s%s%s\n",
		// 		use_mapvote->value ? "Map" : "", (use_mapvote->value
		// 						  && use_cvote->value) ? " & " : "",
		// 		use_cvote->value ? "Config" : "", ((use_mapvote->value && use_kickvote->value)
		// 						   || (use_cvote->value
		// 						       && use_kickvote->value)) ? " & " : "",
		// 		use_kickvote->value ? "Kick" : "");
		// 	lines++;	// lines+=3;
		// }

		/*
		   Map Locations
		 */
		if (ml_count != 0) {
			sprintf(msg_buf + strlen(msg_buf), "\n%d Locations, by: %s\n", ml_count, ml_creator);
			lines++;
		}
		/* 
		   If actionmaps, put a blank line then the maps list
		 */
		if (actionmaps->value && num_maps > 0)
		{
			int chars_on_line = 0, len_mr;

			if (vrot->value)		// Using Vote Rotation?
				strcat(msg_buf, "\nRunning these maps in vote order:\n");
			else if (rrot->value)	// Using Random Rotation?
				strcat(msg_buf, "\nRunning the following maps randomly:\n");
			else 
				strcat(msg_buf, "\nRunning the following maps in order:\n");

			lines += 2;

			for (mapnum = 0; mapnum < num_maps; mapnum++)
			{
				len_mr = strlen(*(map_rotation + mapnum));
				if ((chars_on_line + len_mr + 2) > 39) {
					Q_strlcat(msg_buf, "\n", sizeof(msg_buf));
					lines++;
					if (lines >= max_lines)
						break;
					chars_on_line = 0;
				}
				Q_strlcat(msg_buf, *(map_rotation + mapnum), sizeof(msg_buf));
				chars_on_line += len_mr;
				if (mapnum < (num_maps - 1)) {
					Q_strlcat(msg_buf, ", ", sizeof(msg_buf));
					chars_on_line += 2;
				}
			}

			if (lines < max_lines) {
				Q_strlcat(msg_buf, "\n", sizeof(msg_buf));
				lines++;
			}
		}

		//If we're in teamplay, we want to inform people that they can open the menu with TAB
		if (teamplay->value && lines < max_lines && !auto_menu->value) {
			Q_strlcat(msg_buf, "\nHit TAB to open the Team selection menu", sizeof(msg_buf));
			lines++;
		}
	//}

	/* 
	   Insert action/motd.txt contents (whole MOTD gets truncated after 30 lines)
	 */

	if (motd_num_lines && lines < max_lines-1)
	{
		Q_strlcat(msg_buf, "\n", sizeof(msg_buf));
		lines++;
		for (i = 0; i < motd_num_lines; i++) {
			Q_strlcat(msg_buf, motd_lines[i], sizeof(msg_buf));
			lines++;
			if (lines >= max_lines)
				break;
			Q_strlcat(msg_buf, "\n", sizeof(msg_buf));
		}
	}

	if (!auto_menu->value || ent->client->pers.menu_shown) {
		gi.LocCenter_Print(ent, "%s", msg_buf);
	} else {
		gi.LocClient_Print(ent, PRINT_LOW, "%s", msg_buf);
	}
}

// stuffcmd: forces a player to execute a command.
void stuffcmd(edict_t * ent, char *c)
{
	gi.WriteByte(svc_stufftext);
	gi.WriteString(c);
	gi.unicast(ent, true);
}


// void unicastSound(edict_t *ent, int soundIndex, float volume)
// {
//     int mask = MASK_ENTITY_CHANNEL;

//     if (volume != 1.0)
//         mask |= MASK_VOLUME;
 
//     gi.WriteByte(svc_sound);
//     gi.WriteByte((byte)mask);
//     gi.WriteByte((byte)soundIndex);
//     if (mask & MASK_VOLUME)
//         gi.WriteByte((byte)(volume * 255));

//     // hack when first person spectating, the sound source must be the spectated player
//     if (ent->client->chase_mode == 2 && ent->client->chase_target) {
// 	    gi.WriteShort(((ent->client->chase_target - g_edicts - 1) << 3) + CHAN_NO_PHS_ADD);
//     } else {
// 	    gi.WriteShort(((ent - g_edicts - 1) << 3) + CHAN_NO_PHS_ADD);
//     }

//     gi.unicast (ent, true);
// }
// AQ2:TNG END

/********************************************************************************
*
*  zucc: following are EjectBlooder, EjectShell, AddSplat, and AddDecal
*  code.  All from actionquake, some had to be modified to fit Axshun or fix
*  bugs. 
*
*/

// int decals = 0;
// int shells = 0;
// int splats = 0;

// //blooder used for bleeding

// void BlooderTouch(edict_t * self, edict_t * other, cplane_t * plane, csurface_t * surf)
// {
// 	if( (other == self->owner) || other->client )  // Don't stop on players.
// 		return;
// 	self->think = G_FreeEdict;
// 	self->nextthink = level.time + 1_ms;
// }

// void EjectBlooder(edict_t * self, vec3_t start, vec3_t veloc)
// {
// 	edict_t *blooder;
// 	vec3_t forward;
// 	int spd = 0;

// 	blooder = G_Spawn();
// 	VectorCopy(veloc, forward);
// 	VectorCopy(start, blooder->s.origin);
// 	//VectorCopy(start, blooder->old_origin);
// 	spd = 0;
// 	VectorScale(forward, spd, blooder->velocity);
// 	blooder->solid = SOLID_NOT;
// 	blooder->movetype = MOVETYPE_BLOOD;  // Allow dripping blood to make a splat.
// 	blooder->s.modelindex = level.model_null;
// 	blooder->s.effects |= EF_GIB;
// 	blooder->owner = self;
// 	blooder->touch = BlooderTouch;
// 	blooder->nextthink = level.time + 32_ms;
// 	blooder->think = G_FreeEdict;
// 	blooder->classname = "blooder";

// 	gi.linkentity(blooder);
// }

// zucc - Adding EjectShell code from action quake, modified for Axshun.
/********* SHELL EJECTION **************/

void PlaceHolder( edict_t * ent );  // p_weapon.c

// void ShellTouch(edict_t * self, edict_t * other, cplane_t * plane, csurface_t * surf)
// {
// 	if (self->owner->client->pers.weapon->id == IT_WEAPON_M3)
// 		gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/shellhit1.wav"), 1, ATTN_STATIC, 0);
// 	else if (random() < 0.5)
// 		gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/tink1.wav"), 0.2, ATTN_STATIC, 0);
// 	else
// 		gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/tink2.wav"), 0.2, ATTN_STATIC, 0);
// }

// void ShellDie(edict_t * self)
// {
// 	G_FreeEdict(self);
// 	--shells;
// }

// static void RemoveOldestShell( void )
// {
// 	int i = 0;
// 	edict_t *it = NULL, *found = NULL;

// 	for( i = 0; i < globals.num_edicts; i ++ )
// 	{
// 		it = &g_edicts[i];
// 		if( it->inuse && it->classname && (strcmp( it->classname, "shell" ) == 0) )
// 		{
// 			if( (! found) || (it->freetime < found->freetime) )
// 				found = it;
// 		}
// 	}

// 	if( found )
// 		ShellDie( found );
// }

// // zucc fixed this so it works with the sniper rifle and checks handedness
// // had to add the toggle feature to handle the akimbos correctly, if 1
// // it sets up for ejecting the shell from the left akimbo weapon, if 2
// // it fires right handed akimbo

// void EjectShell(edict_t * self, vec3_t start, int toggle)
// {
// 	edict_t *shell;
// 	vec3_t forward, right, up;
// 	float r;
// 	float fix = 1.0;
// 	int left = 0;

// 	// if (sv_shelloff->value)
// 	// 	return;

// 	// if( (shelllimit->value > 0) && (shells >= shelllimit->value) )
// 	// 	RemoveOldestShell();

// 	shell = G_Spawn();
// 	++shells;

// 	AngleVectors(self->client->v_angle, forward, right, up);

// 	if (self->client->pers.hand == LEFT_HANDED) {
// 		left = 1;
// 		fix = -1.0;
// 	} else if (self->client->pers.hand == CENTER_HANDED)
// 		fix = 0;

// 	// zucc spent a fair amount of time hacking these until they look ok,
// 	// several of them could be improved however.

// 	if (self->client->pers.weapon->id == IT_WEAPON_MK23) {
// 		VectorMA(start, left ? -7 : .4, right, start);
// 		VectorMA(start, left ? 5 : 2, forward, start);
// 		VectorMA(start, left ? -10 : -8, up, start);
// 	} else if (self->client->pers.weapon->id == IT_WEAPON_M4) {
// 		VectorMA(start, left ? -10 : 5, right, start);
// 		VectorMA(start, left ? 6 : 12, forward, start);
// 		VectorMA(start, left ? -9 : -11, up, start);
// 	} else if (self->client->pers.weapon->id == IT_WEAPON_MP5) {
// 		VectorMA(start, left ? -10 : 6, right, start);
// 		VectorMA(start, left ? 6 : 8, forward, start);
// 		VectorMA(start, left ? -9 : -10, up, start);
// 	} else if (self->client->pers.weapon->id == IT_WEAPON_SNIPER) {
// 		VectorMA(start, fix * 11, right, start);
// 		VectorMA(start, 2, forward, start);
// 		VectorMA(start, -11, up, start);

// 	} else if (self->client->pers.weapon->id == IT_WEAPON_M3) {
// 		VectorMA(start, left ? -9 : 3, right, start);
// 		VectorMA(start, left ? 4 : 4, forward, start);
// 		VectorMA(start, left ? -1 : -1, up, start);
// 	}

// 	else if (self->client->pers.weapon->id == IT_WEAPON_DUALMK23) {
// 		if (self->client->pers.hand == LEFT_HANDED)
// 			VectorMA(start, ((toggle == 1) ? 8 : -8), right, start);
// 		else
// 			VectorMA(start, ((toggle == 1) ? -4 : 4), right, start);
// 		VectorMA(start, 6, forward, start);
// 		VectorMA(start, -9, up, start);

// 	}

// 	if ((forward[2] >= -1) && (forward[2] < -0.99)) {
// 		VectorMA(start, 5, forward, start);
// 		VectorMA(start, -0.5, up, start);
// 	} else if ((forward[2] >= -0.99) && (forward[2] < -0.98)) {
// 		VectorMA(start, 5, forward, start);
// 		VectorMA(start, -.1, up, start);
// 	} else if ((forward[2] >= -0.98) && (forward[2] < -0.97)) {
// 		VectorMA(start, 5.1, forward, start);
// 		VectorMA(start, 0.3, up, start);
// 	} else if ((forward[2] >= -0.97) && (forward[2] < -0.96)) {
// 		VectorMA(start, 5.2, forward, start);
// 		VectorMA(start, 0.7, up, start);
// 	} else if ((forward[2] >= -0.96) && (forward[2] < -0.95)) {
// 		VectorMA(start, 5.2, forward, start);
// 		VectorMA(start, 1.1, up, start);
// 	} else if ((forward[2] >= -0.95) && (forward[2] < -0.94)) {
// 		VectorMA(start, 5.3, forward, start);
// 		VectorMA(start, 1.5, up, start);
// 	} else if ((forward[2] >= -0.94) && (forward[2] < -0.93)) {
// 		VectorMA(start, 5.4, forward, start);
// 		VectorMA(start, 1.9, up, start);
// 	} else if ((forward[2] >= -0.93) && (forward[2] < -0.92)) {
// 		VectorMA(start, 5.5, forward, start);
// 		VectorMA(start, 2.3, up, start);
// 	} else if ((forward[2] >= -0.92) && (forward[2] < -0.91)) {
// 		VectorMA(start, 5.6, forward, start);
// 		VectorMA(start, 2.7, up, start);
// 	} else if ((forward[2] >= -0.91) && (forward[2] < -0.9)) {
// 		VectorMA(start, 5.7, forward, start);
// 		VectorMA(start, 3.1, up, start);
// 	} else if ((forward[2] >= -0.9) && (forward[2] < -0.85)) {
// 		VectorMA(start, 5.8, forward, start);
// 		VectorMA(start, 3.5, up, start);
// 	} else if ((forward[2] >= -0.85) && (forward[2] < -0.8)) {
// 		VectorMA(start, 6, forward, start);
// 		VectorMA(start, 4, up, start);
// 	} else if ((forward[2] >= -0.8) && (forward[2] < -0.6)) {
// 		VectorMA(start, 6.5, forward, start);
// 		VectorMA(start, 4.5, up, start);
// 	} else if ((forward[2] >= -0.6) && (forward[2] < -0.4)) {
// 		VectorMA(start, 8, forward, start);
// 		VectorMA(start, 5.5, up, start);
// 	} else if ((forward[2] >= -0.4) && (forward[2] < -0.2)) {
// 		VectorMA(start, 9.5, forward, start);
// 		VectorMA(start, 6, up, start);
// 	} else if ((forward[2] >= -0.2) && (forward[2] < 0)) {
// 		VectorMA(start, 11, forward, start);
// 		VectorMA(start, 6.5, up, start);
// 	} else if ((forward[2] >= 0) && (forward[2] < 0.2)) {
// 		VectorMA(start, 12, forward, start);
// 		VectorMA(start, 7, up, start);
// 	} else if ((forward[2] >= 0.2) && (forward[2] < 0.4)) {
// 		VectorMA(start, 14, forward, start);
// 		VectorMA(start, 6.5, up, start);
// 	} else if ((forward[2] >= 0.4) && (forward[2] < 0.6)) {
// 		VectorMA(start, 16, forward, start);
// 		VectorMA(start, 6, up, start);
// 	} else if ((forward[2] >= 0.6) && (forward[2] < 0.8)) {
// 		VectorMA(start, 18, forward, start);
// 		VectorMA(start, 5, up, start);
// 	} else if ((forward[2] >= 0.8) && (forward[2] < 0.85)) {
// 		VectorMA(start, 18, forward, start);
// 		VectorMA(start, 4, up, start);
// 	} else if ((forward[2] >= 0.85) && (forward[2] < 0.9)) {
// 		VectorMA(start, 18, forward, start);
// 		VectorMA(start, 2.5, up, start);
// 	} else if ((forward[2] >= 0.9) && (forward[2] < 0.91)) {
// 		VectorMA(start, 18.2, forward, start);
// 		VectorMA(start, 2.2, up, start);
// 	} else if ((forward[2] >= 0.91) && (forward[2] < 0.92)) {
// 		VectorMA(start, 18.4, forward, start);
// 		VectorMA(start, 1.9, up, start);
// 	} else if ((forward[2] >= 0.92) && (forward[2] < 0.93)) {
// 		VectorMA(start, 18.6, forward, start);
// 		VectorMA(start, 1.6, up, start);
// 	} else if ((forward[2] >= 0.93) && (forward[2] < 0.94)) {
// 		VectorMA(start, 18.8, forward, start);
// 		VectorMA(start, 1.3, up, start);
// 	} else if ((forward[2] >= 0.94) && (forward[2] < 0.95)) {
// 		VectorMA(start, 19, forward, start);
// 		VectorMA(start, 1, up, start);
// 	} else if ((forward[2] >= 0.95) && (forward[2] < 0.96)) {
// 		VectorMA(start, 19.2, forward, start);
// 		VectorMA(start, 0.7, up, start);
// 	} else if ((forward[2] >= 0.96) && (forward[2] < 0.97)) {
// 		VectorMA(start, 19.4, forward, start);
// 		VectorMA(start, 0.4, up, start);
// 	} else if ((forward[2] >= 0.97) && (forward[2] < 0.98)) {
// 		VectorMA(start, 19.6, forward, start);
// 		VectorMA(start, -0.2, up, start);
// 	} else if ((forward[2] >= 0.98) && (forward[2] < 0.99)) {
// 		VectorMA(start, 19.8, forward, start);
// 		VectorMA(start, -0.6, up, start);
// 	} else if ((forward[2] >= 0.99) && (forward[2] <= 1)) {
// 		VectorMA(start, 20, forward, start);
// 		VectorMA(start, -1, up, start);
// 	}

// 	VectorCopy(start, shell->s.origin);
// 	//VectorCopy(start, shell->old_origin);
// 	if (fix == 0)		// we want some velocity on those center handed ones
// 		fix = 1;
// 	if (self->client->pers.weapon->id == IT_WEAPON_SNIPER)
// 		VectorMA(shell->velocity, fix * (-35 + random() * -60), right, shell->velocity);
// 	else if (self->client->pers.weapon->id == IT_WEAPON_DUALMK23) {
// 		if (self->client->pers.hand == LEFT_HANDED)
// 			VectorMA(shell->velocity,
// 				 (toggle == 1 ? 1 : -1) * (35 + random() * 60), right, shell->velocity);
// 		else
// 			VectorMA(shell->velocity,
// 				 (toggle == 1 ? -1 : 1) * (35 + random() * 60), right, shell->velocity);
// 	} else
// 		VectorMA(shell->velocity, fix * (35 + random() * 60), right, shell->velocity);
// 	VectorMA(shell->avelocity, 500, right, shell->avelocity);
// 	if (self->client->pers.weapon->id == IT_WEAPON_SNIPER)
// 		VectorMA(shell->velocity, 60 + 40, up, shell->velocity);
// 	else
// 		VectorMA(shell->velocity, 60 + random() * 90, up, shell->velocity);

// 	shell->movetype = MOVETYPE_BOUNCE;
// 	shell->solid = SOLID_BBOX;

// 	if( (self->client->pers.weapon->id == IT_WEAPON_M3) || (self->client->pers.weapon->id == IT_WEAPON_HANDCANNON) )
// 		shell->s.modelindex = gi.modelindex("models/weapons/shell/tris2.md2");
// 	else if( (self->client->pers.weapon->id == IT_WEAPON_SNIPER) || (self->client->pers.weapon->id == IT_WEAPON_M4) )
// 		shell->s.modelindex = gi.modelindex("models/weapons/shell/tris3.md2");
// 	else
// 		shell->s.modelindex = gi.modelindex("models/weapons/shell/tris.md2");

// 	r = random();
// 	if (r < 0.1)
// 		shell->s.frame = 0;
// 	else if (r < 0.2)
// 		shell->s.frame = 1;
// 	else if (r < 0.3)
// 		shell->s.frame = 2;
// 	else if (r < 0.5)
// 		shell->s.frame = 3;
// 	else if (r < 0.6)
// 		shell->s.frame = 4;
// 	else if (r < 0.7)
// 		shell->s.frame = 5;
// 	else if (r < 0.8)
// 		shell->s.frame = 6;
// 	else if (r < 0.9)
// 		shell->s.frame = 7;
// 	else
// 		shell->s.frame = 8;

// 	shell->owner = self;
// 	shell->touch = ShellTouch;
// 	shell->nextthink = level.framenum + (shelllife->value - (shells * 0.05)) * HZ;
// 	shell->think = shelllife->value ? ShellDie : PlaceHolder;
// 	shell->classname = "shell";
// 	shell->freetime = level.time;  // Used to determine oldest spawned shell.

// 	gi.linkentity(shell);
// }

/*
   ==================
   FindEdictByClassnum
   ==================
 */
// edict_t *FindEdictByClassnum(char *classname, int classnum)
// {
// 	int i;
// 	edict_t *it;
// 	for (i = 0; i < globals.num_edicts; i++)
// 	{
// 		it = &g_edicts[i];
// 		if (it->classname && (it->classnum == classnum) && (strcmp(it->classname, classname) == 0))
// 			return it;
// 	}

// 	return NULL;

// }

// /********* Bulletholes/wall stuff ***********/

// void UpdateAttachedPos( edict_t *self )
// {
// 	vec3_t fwd, right, up;

// 	if( (self->wait && (level.framenum >= self->wait)) || ! self->movetarget->inuse )
// 	{
// 		G_FreeEdict(self);
// 		return;
// 	}

// 	self->nextthink = level.framenum + 1;

// 	if( self < self->movetarget )
// 	{
// 		// If the object we're attached to hasn't been updated yet this frame,
// 		// we need to move ahead one frame's worth so we stay aligned with it.
// 		VectorScale( self->movetarget->velocity, FRAMETIME, self->s.origin );
// 		VectorAdd( self->movetarget->s.origin, self->s.origin, self->s.origin );
// 		VectorScale( self->movetarget->avelocity, FRAMETIME, self->s.angles );
// 		VectorAdd( self->movetarget->s.angles, self->s.angles, self->s.angles );
// 	}
// 	else
// 	{
// 		VectorCopy( self->movetarget->s.origin, self->s.origin );
// 		VectorCopy( self->movetarget->s.angles, self->s.angles );
// 	}

// 	AngleVectors( self->s.angles, fwd, right, up ); // At this point, this is the angles of the entity we attached to.
// 	self->s.origin[0] += fwd[0] * self->move_origin[0] + right[0] * self->move_origin[1] + up[0] * self->move_origin[2];
// 	self->s.origin[1] += fwd[1] * self->move_origin[0] + right[1] * self->move_origin[1] + up[1] * self->move_origin[2];
// 	self->s.origin[2] += fwd[2] * self->move_origin[0] + right[2] * self->move_origin[1] + up[2] * self->move_origin[2];
// 	VectorAdd( self->s.angles, self->move_angles, self->s.angles );
// 	VectorCopy( self->movetarget->velocity, self->velocity );
// 	VectorCopy( self->movetarget->avelocity, self->avelocity );
// }

// // Decal/splat/knife attached to some moving entity.
// void AttachedThink( edict_t *self )
// {
// 	UpdateAttachedPos( self );
// 	gi.linkentity( self );
// }

// // Attach a splat/decal/knife to a moving entity.
// void AttachToEntity( edict_t *self, edict_t *onto )
// {
// 	vec3_t fwd, right, up, offset;

// 	self->wait = self->nextthink;  // Use old nextthink as despawn framenum (0 is never).
// 	self->movetype = MOVETYPE_NONE;

// 	self->movetarget = onto;
// 	AngleVectors( onto->s.angles, fwd, right, up );
// 	VectorSubtract( self->s.origin, onto->s.origin, offset );
// 	self->move_origin[0] = DotProduct( offset, fwd );
// 	self->move_origin[1] = DotProduct( offset, right );
// 	self->move_origin[2] = DotProduct( offset, up );
// 	VectorSubtract( self->s.angles, onto->s.angles, self->move_angles );

// 	self->think = AttachedThink;

// 	UpdateAttachedPos( self );
// }

// bool CanBeAttachedTo( const edict_t *ent )
// {
// 	return (ent && ( (Q_strnicmp( ent->classname, "func_door", 9 ) == 0)
// 	               || (strcmp( ent->classname, "func_plat" ) == 0)
// 	               || (strcmp( ent->classname, "func_rotating" ) == 0)
// 	               || (strcmp( ent->classname, "func_train" ) == 0)
// 	               || (strcmp( ent->classname, "func_button" ) == 0) ));
// }

// void AddDecal(edict_t * self, trace_t * tr)
// {
// 	edict_t *decal, *dec;
// 	bool attached;

// 	if (bholelimit->value < 1)
// 		return;

// 	attached = CanBeAttachedTo(tr->ent);

// 	decal = bholelife->value ? G_Spawn() : G_Spawn_Decal();
// 	if( ! decal )
// 		return;

// 	++decals;

// 	if (decals > bholelimit->value)
// 		decals = 1;

// 	dec = FindEdictByClassnum("decal", decals);

// 	if( dec )
// 	{
// 		dec->think = G_FreeEdict;
// 		dec->nextthink = level.framenum + FRAMEDIV;
// 	}

// 	decal->solid = SOLID_NOT;
// 	decal->movetype = MOVETYPE_NONE;
// 	decal->s.modelindex = gi.modelindex("models/objects/holes/hole1/hole.md2");
// 	VectorCopy(tr->endpos, decal->s.origin);
// 	VectorCopy(tr->endpos, decal->old_origin);
// 	vectoangles(tr->plane.normal, decal->s.angles);
// 	decal->s.angles[ROLL] = crandom() * 180.f;

// 	decal->owner = self;
// 	decal->touch = NULL;
// 	decal->nextthink = bholelife->value ? (level.framenum + bholelife->value * HZ) : 0;
// 	decal->think = bholelife->value ? G_FreeEdict : PlaceHolder;
// 	decal->classname = "decal";
// 	decal->classnum = decals;

// 	if ((tr->ent) && (0 == strcmp("func_explosive", tr->ent->classname))) {
// 		CGF_SFX_AttachDecalToGlass(tr->ent, decal);
// 	}
// 	else if( attached )
// 		AttachToEntity( decal, tr->ent );

// 	gi.linkentity(decal);
// }

// void AddSplat(edict_t * self, vec3_t point, trace_t * tr)
// {
// 	edict_t *splat, *spt;
// 	float r;
// 	bool attached;

// 	if (splatlimit->value < 1)
// 		return;

// 	attached = CanBeAttachedTo(tr->ent);

// 	splat = splatlife->value ? G_Spawn() : G_Spawn_Decal();
// 	if( ! splat )
// 		return;

// 	++splats;

// 	if (splats > splatlimit->value)
// 		splats = 1;

// 	spt = FindEdictByClassnum("splat", splats);

// 	if( spt )
// 	{
// 		spt->think = G_FreeEdict;
// 		spt->nextthink = level.framenum + FRAMEDIV;
// 	}

// 	splat->solid = SOLID_NOT;
// 	splat->movetype = MOVETYPE_NONE;

// 	r = random();
// 	if (r > .67)
// 		splat->s.modelindex = gi.modelindex("models/objects/splats/splat1/splat.md2");
// 	else if (r > .33)
// 		splat->s.modelindex = gi.modelindex("models/objects/splats/splat2/splat.md2");
// 	else
// 		splat->s.modelindex = gi.modelindex("models/objects/splats/splat3/splat.md2");

// 	VectorCopy(point, splat->s.origin);
// 	VectorCopy(point, splat->old_origin);

// 	vectoangles(tr->plane.normal, splat->s.angles);
// 	splat->s.angles[ROLL] = crandom() * 180.f;

// 	splat->owner = self;
// 	splat->touch = NULL;
// 	splat->nextthink = level.framenum + splatlife->value * HZ;
// 	splat->think = splatlife->value ? G_FreeEdict : PlaceHolder;
// 	splat->classname = "splat";
// 	splat->classnum = splats;

// 	if ((tr->ent) && (0 == strcmp("func_explosive", tr->ent->classname))) {
// 		CGF_SFX_AttachDecalToGlass(tr->ent, splat);
// 	}
// 	else if( attached )
// 		AttachToEntity( splat, tr->ent );

// 	gi.linkentity(splat);
// }

/* %-variables for chat msgs */

void GetWeaponName( edict_t *ent, char *buf )
{
	if( IS_ALIVE(ent) && ent->client->pers.weapon )
	{
		strcpy( buf, ent->client->pers.weapon->pickup_name );
		return;
	}
	
	strcpy( buf, "no weapon" );
}

void GetItemName( edict_t *ent, char *buf )
{
	int i, itemNum;
	
	if( IS_ALIVE(ent) )
	{
		for( i = 0; i < ITEM_COUNT; i ++ )
		{
			itemNum = IT_ITEM_QUIET;
			if( INV_AMMO( ent, itemNum ) )
			{
				strcpy( buf, GET_ITEM(itemNum)->pickup_name );
				return;
			}
		}
	}
	
	strcpy( buf, "no item" );
}

void GetHealth( edict_t *ent, char *buf )
{
	if( IS_ALIVE(ent) )
		sprintf( buf, "%d", ent->health );
	else
		sprintf( buf, "0" );
}

void GetAmmo( edict_t *ent, char *buf )
{
	int ammo;

	if( IS_ALIVE(ent) && ent->client->pers.weapon )
	{
		switch( ent->client->pers.weapon->id )
		{
		case IT_WEAPON_MK23:
			sprintf( buf, "%d round%s (%d extra mag%s)",
				ent->client->mk23_rds, ent->client->mk23_rds == 1 ? "" : "s",
				ent->client->inventory[ent->client->ammo_index],
				ent->client->inventory[ent->client->ammo_index] == 1 ? "" : "s");
			return;
		case IT_WEAPON_MP5:
			sprintf( buf, "%d round%s (%d extra mag%s)",
				ent->client->mp5_rds, ent->client->mp5_rds == 1 ? "" : "s",
				ent->client->inventory[ent->client->ammo_index],
				ent->client->inventory[ent->client->ammo_index] == 1 ? "" : "s");
			return;
		case IT_WEAPON_M4:
			sprintf( buf, "%d round%s (%d extra mag%s)",
				ent->client->m4_rds, ent->client->m4_rds == 1 ? "" : "s",
				ent->client->inventory[ent->client->ammo_index],
				ent->client->inventory[ent->client->ammo_index] == 1 ? "" : "s");
			return;
		case IT_WEAPON_M3:
			sprintf( buf, "%d shell%s (%d extra shell%s)",
				ent->client->shot_rds, ent->client->shot_rds == 1 ? "" : "s",
				ent->client->inventory[ent->client->ammo_index],
				ent->client->inventory[ent->client->ammo_index] == 1 ? "" : "s");
			return;
		case IT_WEAPON_HANDCANNON:
			sprintf( buf, "%d shell%s (%d extra shell%s)",
				ent->client->cannon_rds, ent->client->cannon_rds == 1 ? "" : "s",
				ent->client->inventory[ent->client->ammo_index],
				ent->client->inventory[ent->client->ammo_index] == 1 ? "" : "s");
			return;
		case IT_WEAPON_SNIPER:
			sprintf( buf, "%d round%s (%d extra round%s)",
				ent->client->sniper_rds, ent->client->sniper_rds == 1 ? "" : "s",
				ent->client->inventory[ent->client->ammo_index],
				ent->client->inventory[ent->client->ammo_index] == 1 ? "" : "s");
			return;
		case IT_WEAPON_DUALMK23:
			sprintf( buf, "%d round%s (%d extra mag%s)",
				ent->client->dual_rds, ent->client->dual_rds == 1 ? "" : "s",
				ent->client->inventory[ent->client->ammo_index],
				ent->client->inventory[ent->client->ammo_index] == 1 ? "" : "s");
			return;
		case IT_WEAPON_KNIFE:
			ammo = INV_AMMO( ent, IT_WEAPON_KNIFE );
			sprintf( buf, "%d kni%s", ammo, (ammo == 1) ? "fe" : "ves" );
			return;
		case IT_WEAPON_GRENADES:
			ammo = INV_AMMO( ent, IT_WEAPON_GRENADES );
			sprintf( buf, "%d grenade%s", ammo, (ammo == 1) ? "" : "s" );
			return;
		}
	}
	
	strcpy( buf, "no ammo" );
}

void GetNearbyTeammates( edict_t *self, char *buf )
{
	edict_t *nearby_teammates[8];
	size_t nearby_teammates_num = 0, l;
	edict_t *ent = NULL;

	while ((ent = findradius(ent, self->s.origin, 1500)) != NULL) {
		if (ent == self || !ent->client || !CanDamage(ent, self) || !OnSameTeam(ent, self))
			continue;

		nearby_teammates[nearby_teammates_num++] = ent;
		if (nearby_teammates_num >= 8)
			break;
	}

	if (nearby_teammates_num == 0) {
		strcpy(buf, "nobody");
		return;
	}

	strcpy(buf, nearby_teammates[0]->client->pers.netname);
	for (l = 1; l < nearby_teammates_num; l++)
	{
		if (l == nearby_teammates_num - 1)
			Q_strlcat(buf, " and ", PARSE_BUFSIZE);
		else
			Q_strlcat(buf, ", ", PARSE_BUFSIZE);

		Q_strlcat( buf, nearby_teammates[l]->client->pers.netname, PARSE_BUFSIZE );
	}
}

// Messaging adjectives/pronouns
// Borrowed from https://github.com/VortexQuake2/Vortex, thank you!

const char *GetPossesiveAdjectiveSingular(edict_t *ent) {
	int gender = ent->client->pers.gender;
	char *info;

	switch( gender ) {
		case GENDER_MALE:
			return "his";
		case GENDER_FEMALE:
			return "her";
		case GENDER_NEUTRAL:
			return "it";
		default:
			return "their";
	}
}

const char *GetPossesiveAdjective(edict_t *ent) {
	int gender = ent->client->pers.gender;
	char *info;

	switch( gender ) {
		case GENDER_MALE:
			return "his";
		case GENDER_FEMALE:
			return "her";
		case GENDER_NEUTRAL:
			return "its";
		default:
			return "their";
	}
}

const char *GetReflexivePronoun(edict_t *ent) {
	int gender = ent->client->pers.gender;
	char *info;

	switch( gender ) {
		case GENDER_MALE:
			return "himself";
		case GENDER_FEMALE:
			return "herself";
		case GENDER_NEUTRAL:
			return "itself";
		default:
			return "themselves";
	}
}