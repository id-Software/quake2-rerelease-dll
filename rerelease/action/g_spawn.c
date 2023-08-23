//-----------------------------------------------------------------------------
// g_spawn.c
//
// $Id: g_spawn.c,v 1.38 2002/03/28 12:10:11 freud Exp $
//
//-----------------------------------------------------------------------------
// $Log: g_spawn.c,v $
// Revision 1.38  2002/03/28 12:10:11  freud
// Removed unused variables (compiler warnings).
// Added cvar mm_allowlock.
//
// Revision 1.37  2002/03/28 11:46:03  freud
// stat_mode 2 and timelimit 0 did not show stats at end of round.
// Added lock/unlock.
// A fix for use_oldspawns 1, crash bug.
//
// Revision 1.36  2002/03/27 15:16:56  freud
// Original 1.52 spawn code implemented for use_newspawns 0.
// Teamplay, when dropping bandolier, your drop the grenades.
// Teamplay, cannot pick up grenades unless wearing bandolier.
//
// Revision 1.35  2002/03/25 23:35:19  freud
// Ghost code, use_ghosts and more stuff..
//
// Revision 1.34  2002/03/25 18:32:11  freud
// I'm being too productive.. New ghost command needs testing.
//
// Revision 1.33  2002/03/24 22:45:54  freud
// New spawn code again, bad commit last time..
//
// Revision 1.31  2002/01/24 01:47:17  ra
// Further fixes to M$ files
//
// Revision 1.30  2002/01/24 01:32:34  ra
// Enabling .aqg files to be in either M$ form or real text files.
//
// Revision 1.29  2001/11/27 23:23:40  igor_rock
// Bug fixed: day_cycle_at wasn't reset at mapchange
//
// Revision 1.28  2001/11/16 13:01:39  deathwatch
// Fixed 'no team wins' sound - it wont play now with use_warnings 0
// Precaching misc/flashlight.wav
//
// Revision 1.27  2001/11/10 14:00:14  deathwatch
// Fixed resetting of teamXscores
//
// Revision 1.26  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.25  2001/09/28 13:44:23  slicerdw
// Several Changes / improvements
//
// Revision 1.24  2001/08/18 01:28:06  deathwatch
// Fixed some stats stuff, added darkmatch + day_cycle, cleaned up several files, restructured ClientCommand
//
// Revision 1.23  2001/07/15 16:02:18  slicerdw
// Added checks for teamplay on when using 3teams or tourney
//
// Revision 1.22  2001/06/26 11:41:39  igor_rock
// removed the debug messages for flag and playerstart positions in ctf
//
// Revision 1.21  2001/06/25 11:44:47  slicerdw
// New Video Check System - video_check and video_check_lockpvs no longer latched
//
// Revision 1.20  2001/06/22 16:34:05  slicerdw
// Finished Matchmode Basics, now with admins, Say command tweaked...
//
// Revision 1.19  2001/06/21 00:05:30  slicerdw
// New Video Check System done -  might need some revision but works..
//
// Revision 1.16  2001/06/13 09:43:49  igor_rock
// if ctf is enabled, friendly fire automatically set to off (ff doesn't make any sense in ctf)
//
// Revision 1.15  2001/06/01 19:18:42  slicerdw
// Added Matchmode Code
//
// Revision 1.14  2001/05/31 16:58:14  igor_rock
// conflicts resolved
//
// Revision 1.13.2.8  2001/05/31 15:15:52  igor_rock
// added a parameter check for sscanf
//
// Revision 1.13.2.7  2001/05/31 06:47:51  igor_rock
// - removed crash bug with non exisitng flag files
// - added new commands "setflag1", "setflag2" and "saveflags" to create
//   .flg files
//
// Revision 1.13.2.6  2001/05/27 17:24:10  igor_rock
// changed spawnpoint behavior in CTF
//
// Revision 1.13.2.5  2001/05/27 16:11:10  igor_rock
// added function to replace nearest spawnpoint to flag through info_player_teamX
//
// Revision 1.13.2.4  2001/05/27 11:47:53  igor_rock
// added .flg file support and timelimit bug fix
//
// Revision 1.13.2.3  2001/05/25 18:59:52  igor_rock
// Added CTF Mode completly :)
// Support for .flg files is still missing, but with "real" CTF maps like
// tq2gtd1 the ctf works fine.
// (I hope that all other modes still work, just tested DM and teamplay)
//
// Revision 1.13.2.2  2001/05/20 18:54:19  igor_rock
// added original ctf code snippets from zoid. lib compilesand runs but
// doesn't function the right way.
// Jsut committing these to have a base to return to if something wents
// awfully wrong.
//
// Revision 1.13.2.1  2001/05/20 15:17:31  igor_rock
// removed the old ctf code completly
//
// Revision 1.13  2001/05/15 15:49:14  igor_rock
// added itm_flags for deathmatch
//
// Revision 1.12  2001/05/14 21:10:16  igor_rock
// added wp_flags support (and itm_flags skeleton - doesn't disturb in the moment)
//
// Revision 1.11  2001/05/14 14:08:51  igor_rock
// added tng sounds for precaching
//
// Revision 1.10  2001/05/12 19:29:28  mort
// Fixed more various map change bugs
//
// Revision 1.9  2001/05/12 19:27:17  mort
// Fixed various map change bugs
//
// Revision 1.8  2001/05/12 17:20:27  mort
// Reset started variable in SP_worldspawn
//
// Revision 1.7  2001/05/12 13:45:59  mort
// CTF status bar now sends correctly
//
// Revision 1.6  2001/05/12 13:15:04  mort
// Forces teamplay on when ctf is enabled
//
// Revision 1.5  2001/05/12 08:20:01  mort
// CTF bug fix, makes sure flags have actually spawned before certain functions attempt to use them
//
// Revision 1.4  2001/05/12 00:37:03  ra
//
//
// Fixing various compilerwarnings.
//
// Revision 1.3  2001/05/11 16:07:25  mort
// Various CTF bits and pieces...
//
// Revision 1.2  2001/05/11 12:21:19  slicerdw
// Commented old Location support ( ADF ) With the ML/ETE Compatible one
//
// Revision 1.1.1.1  2001/05/06 17:31:52  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#include "g_local.h"


typedef struct
{
  const char *name;
  void (*spawn) (edict_t * ent);
}
spawn_t;


void SP_item_health (edict_t * self);
void SP_item_health_small (edict_t * self);
void SP_item_health_large (edict_t * self);
void SP_item_health_mega (edict_t * self);

void SP_info_player_start (edict_t * ent);
void SP_info_player_deathmatch (edict_t * ent);
void SP_info_player_intermission (edict_t * ent);

void SP_func_plat (edict_t * ent);
void SP_func_rotating (edict_t * ent);
void SP_func_button (edict_t * ent);
void SP_func_door (edict_t * ent);
void SP_func_door_secret (edict_t * ent);
void SP_func_door_rotating (edict_t * ent);
void SP_func_water (edict_t * ent);
void SP_func_train (edict_t * ent);
void SP_func_conveyor (edict_t * self);
void SP_func_wall (edict_t * self);
void SP_func_object (edict_t * self);
void SP_func_explosive (edict_t * self);
void SP_func_timer (edict_t * self);
void SP_func_areaportal (edict_t * ent);
void SP_func_clock (edict_t * ent);
void SP_func_killbox (edict_t * ent);

void SP_trigger_always (edict_t * ent);
void SP_trigger_once (edict_t * ent);
void SP_trigger_multiple (edict_t * ent);
void SP_trigger_relay (edict_t * ent);
void SP_trigger_push (edict_t * ent);
void SP_trigger_hurt (edict_t * ent);
void SP_trigger_key (edict_t * ent);
void SP_trigger_counter (edict_t * ent);
void SP_trigger_elevator (edict_t * ent);
void SP_trigger_gravity (edict_t * ent);
void SP_trigger_monsterjump (edict_t * ent);

void SP_target_temp_entity (edict_t * ent);
void SP_target_speaker (edict_t * ent);
void SP_target_explosion (edict_t * ent);
void SP_target_changelevel (edict_t * ent);
void SP_target_secret (edict_t * ent);
void SP_target_goal (edict_t * ent);
void SP_target_splash (edict_t * ent);
void SP_target_spawner (edict_t * ent);
void SP_target_blaster (edict_t * ent);
void SP_target_crosslevel_trigger (edict_t * ent);
void SP_target_crosslevel_target (edict_t * ent);
void SP_target_laser (edict_t * self);
void SP_target_help (edict_t * ent);
void SP_target_lightramp (edict_t * self);
void SP_target_earthquake (edict_t * ent);
void SP_target_character (edict_t * ent);
void SP_target_string (edict_t * ent);

void SP_worldspawn (edict_t * ent);
void SP_viewthing (edict_t * ent);

void SP_light (edict_t * self);
void SP_light_mine1 (edict_t * ent);
void SP_light_mine2 (edict_t * ent);
void SP_info_null (edict_t * self);
void SP_info_notnull (edict_t * self);
void SP_path_corner (edict_t * self);
void SP_point_combat (edict_t * self);

void SP_misc_explobox (edict_t * self);
void SP_misc_banner (edict_t * self);
void SP_misc_satellite_dish (edict_t * self);
void SP_misc_deadsoldier (edict_t * self);
void SP_misc_viper (edict_t * self);
void SP_misc_viper_bomb (edict_t * self);
void SP_misc_bigviper (edict_t * self);
void SP_misc_strogg_ship (edict_t * self);
void SP_misc_teleporter (edict_t * self);
void SP_misc_teleporter_dest (edict_t * self);
void SP_misc_blackhole (edict_t * self);


//zucc - item replacement function
void CheckItem (edict_t * ent);
int LoadFlagsFromFile (const char *mapname);
void SVCmd_CheckSB_f(void); //rekkie -- silence ban
extern void UnBan_TeamKillers(void);

//AQ2:TNG - Slicer New location code
int ml_count = 0;
char ml_creator[101];
//AQ2:TNG END
placedata_t locationbase[MAX_LOCATIONS_IN_BASE];

//AQ2:M
static const spawn_t spawns[] = {
  {"item_health", SP_item_health},
  {"item_health_small", SP_item_health_small},
  {"item_health_large", SP_item_health_large},
  {"item_health_mega", SP_item_health_mega},
  {"info_player_start", SP_info_player_start},
  {"info_player_deathmatch", SP_info_player_deathmatch},
  {"info_player_intermission", SP_info_player_intermission},
  {"info_player_team1", SP_info_player_team1},
  {"info_player_team2", SP_info_player_team2},
  {"func_plat", SP_func_plat},
  {"func_button", SP_func_button},
  {"func_door", SP_func_door},
  {"func_door_secret", SP_func_door_secret},
  {"func_door_rotating", SP_func_door_rotating},
  {"func_rotating", SP_func_rotating},
  {"func_train", SP_func_train},
  {"func_water", SP_func_water},
  {"func_conveyor", SP_func_conveyor},
  {"func_areaportal", SP_func_areaportal},
  {"func_clock", SP_func_clock},
  {"func_wall", SP_func_wall},
  {"func_object", SP_func_object},
  {"func_timer", SP_func_timer},
  {"func_explosive", SP_func_explosive},
  {"func_killbox", SP_func_killbox},
  {"trigger_always", SP_trigger_always},
  {"trigger_once", SP_trigger_once},
  {"trigger_multiple", SP_trigger_multiple},
  {"trigger_relay", SP_trigger_relay},
  {"trigger_push", SP_trigger_push},
  {"trigger_hurt", SP_trigger_hurt},
  {"trigger_key", SP_trigger_key},
  {"trigger_counter", SP_trigger_counter},
  {"trigger_elevator", SP_trigger_elevator},
  {"trigger_gravity", SP_trigger_gravity},
  {"trigger_monsterjump", SP_trigger_monsterjump},
  {"target_temp_entity", SP_target_temp_entity},
  {"target_speaker", SP_target_speaker},
  {"target_explosion", SP_target_explosion},
  {"target_changelevel", SP_target_changelevel},
  {"target_splash", SP_target_splash},
  {"target_spawner", SP_target_spawner},
  {"target_blaster", SP_target_blaster},
  {"target_crosslevel_trigger", SP_target_crosslevel_trigger},
  {"target_crosslevel_target", SP_target_crosslevel_target},
  {"target_laser", SP_target_laser},
  {"target_earthquake", SP_target_earthquake},
  {"target_character", SP_target_character},
  {"target_string", SP_target_string},
  {"worldspawn", SP_worldspawn},
  {"viewthing", SP_viewthing},
  {"light_mine1", SP_light_mine1},
  {"light_mine2", SP_light_mine2},
  {"info_null", SP_info_null},
  {"func_group", SP_info_null},
  {"info_notnull", SP_info_notnull},
  {"path_corner", SP_path_corner},
  {"misc_banner", SP_misc_banner},
  {"misc_ctf_banner", SP_misc_ctf_banner},
  {"misc_ctf_small_banner", SP_misc_ctf_small_banner},
  {"misc_satellite_dish", SP_misc_satellite_dish},
  {"misc_viper", SP_misc_viper},
  {"misc_viper_bomb", SP_misc_viper_bomb},
  {"misc_bigviper", SP_misc_bigviper},
  {"misc_strogg_ship", SP_misc_strogg_ship},
  {"misc_teleporter", SP_misc_teleporter},
  {"misc_teleporter_dest", SP_misc_teleporter_dest},
  {"trigger_teleport", SP_trigger_teleport},
  {"info_teleport_destination", SP_info_teleport_destination},
  {"misc_blackhole", SP_misc_blackhole},

  {NULL, NULL}
};

/*
===============
ED_CallSpawn

Finds the spawn function for the entity and calls it
===============
*/
void ED_CallSpawn (edict_t * ent)
{
	const spawn_t *s;
	gitem_t *item;
	int i;

	if (!ent->classname) {
		gi.dprintf("ED_CallSpawn: NULL classname\n");
		return;
	}

	// zucc - BD's item replacement idea
	CheckItem(ent);

	// check item spawn functions
	for (i = 0, item = itemlist; i < game.num_items; i++, item++)
	{
		if (!item->classname)
			continue;
		if (!strcmp(item->classname, ent->classname))
		{	// found it

			//FIXME: We do same checks in SpawnItem, do we need these here? -M
			if ((gameSettings & GS_TEAMPLAY) && g_spawn_items->value && !matchmode->value) // Force spawn ammo/items/weapons for teamplay, non-matchmode
			{
				SpawnItem(ent, item);
			}
			else if (gameSettings & GS_DEATHMATCH)
			{
				if ((gameSettings & GS_WEAPONCHOOSE) && g_spawn_items->value) // Force spawn ammo/items/weapons for DM modes
					SpawnItem(ent, item);
				else if (gameSettings & GS_WEAPONCHOOSE) // Traditional teamplay / dm_choose 1 mode
					G_FreeEdict( ent );
				else if (item->flags & (IT_AMMO|IT_WEAPON))
					SpawnItem(ent, item);
				else if ((item->flags & IT_ITEM) && item_respawnmode->value)
					SpawnItem( ent, item );
				else
					G_FreeEdict(ent);
			}
			else if (ctf->value)
			{
				if(item->flags & IT_FLAG)
					SpawnItem(ent, item);
				else if(ctf->value == 2 && (item->flags & (IT_AMMO|IT_WEAPON|IT_ITEM|IT_POWERUP)))
					SpawnItem(ent, item);
				else
					G_FreeEdict(ent);
			}
			else
			{
				G_FreeEdict(ent);
			}

			return;
		}
	}

	// check normal spawn functions
	for (s = spawns; s->name; s++)
	{
		if (!strcmp (s->name, ent->classname))
		{			// found it
			s->spawn (ent);
			return;
		}
	}

	/*if(strcmp (ent->classname, "freed") != 0) {
		gi.dprintf ("%s doesn't have a spawn function\n", ent->classname);
	}*/

	G_FreeEdict( ent );
}

// zucc BD's checkitem function
//An 2D array of items to look for and replace with...
//item[i][0] = the Q2 item to look for
//item[i][1] = the NS2 item to actually spawn

#define ITEM_SWITCH_COUNT 15

static char *sp_item[ITEM_SWITCH_COUNT][2] = {
  {"weapon_machinegun", "weapon_MP5"},
  //{"weapon_supershotgun","weapon_HC"},
  {"weapon_bfg", "weapon_M4"},
  {"weapon_shotgun", "weapon_M3"},
  //{"weapon_grenadelauncher","weapon_M203"},
  {"weapon_chaingun", "weapon_Sniper"},
  {"weapon_rocketlauncher", "weapon_HC"},
  {"weapon_railgun", "weapon_Dual"},
  {"ammo_bullets", "ammo_clip"},
  {"ammo_rockets", "ammo_mag"},
  {"ammo_cells", "ammo_m4"},
  {"ammo_slugs", "ammo_sniper"},
  {"ammo_shells", "ammo_m3"},
  {"ammo_grenades", "weapon_Grenade"}
  ,
  {"ammo_box", "ammo_m3"},
  {"weapon_cannon", "weapon_HC"},
  {"weapon_sniper", "weapon_Sniper"}

};

void CheckItem (edict_t * ent)
{
	int i;

	for (i = 0; i < ITEM_SWITCH_COUNT; i++)
	{
		//If it's a null entry, bypass it
		if (!sp_item[i][0])
			continue;
		//Do the passed ent and our list match?
		if (strcmp (ent->classname, sp_item[i][0]) == 0)
		{
			//Yep. Replace the Q2 entity with our own.
			ent->classname = sp_item[i][1];
			return;
		}
	}
}


/*
=============
ED_NewString
=============
*/
char *ED_NewString (char *string)
{
	char *newb, *new_p;
	int i, l;

	l = strlen (string) + 1;

	newb = gi.TagMalloc (l, TAG_LEVEL);

	new_p = newb;

	for (i = 0; i < l; i++)
	{
		if (string[i] == '\\' && i < l - 1)
		{
			i++;
			if (string[i] == 'n')
				*new_p++ = '\n';
			else
				*new_p++ = '\\';
		}
		else
			*new_p++ = string[i];
	}

	return newb;
}




/*
===============
ED_ParseField

Takes a key/value pair and sets the binary values
in an edict
===============
*/
void ED_ParseField (char *key, char *value, edict_t * ent)
{
	field_t *f;
	byte *b;
	float v;
	vec3_t vec;

	for (f = fields; f->name; f++)
	{
		// FFL_NOSPAWN check in the following added in 3.20.  Adding here.  -FB
		if (!(f->flags & FFL_NOSPAWN) && !Q_stricmp (f->name, key))
		{			// found it
			if (f->flags & FFL_SPAWNTEMP)
				b = (byte *)&st;
			else
				b = (byte *)ent;

			switch (f->type)
			{
			case F_LSTRING:
				*(char **) (b + f->ofs) = ED_NewString (value);
				break;
			case F_VECTOR:
                if (sscanf(value, "%f %f %f", &vec[0], &vec[1], &vec[2]) != 3) {
                    gi.dprintf("ED_ParseField: couldn't parse '%s'\n", key);
                    VectorClear(vec);
                }
				((float *) (b + f->ofs))[0] = vec[0];
				((float *) (b + f->ofs))[1] = vec[1];
				((float *) (b + f->ofs))[2] = vec[2];
			break;
			case F_INT:
				*(int *) (b + f->ofs) = atoi (value);
				break;
			case F_FLOAT:
				*(float *) (b + f->ofs) = atof (value);
				break;
			case F_ANGLEHACK:
				v = atof (value);
				((float *) (b + f->ofs))[0] = 0;
				((float *) (b + f->ofs))[1] = v;
				((float *) (b + f->ofs))[2] = 0;
				break;
			case F_IGNORE:
				break;
			default:
				break;
			}
			return;
		}
	}
	gi.dprintf("ED_ParseField: %s is not a field\n", key);
}

/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
====================
*/
char *
ED_ParseEdict (char *data, edict_t * ent)
{
  qboolean init;
  char keyname[256];
  char *com_token;

  init = false;
  memset (&st, 0, sizeof (st));

// go through all the dictionary pairs
  while (1)
    {
      // parse key
      com_token = COM_Parse (&data);
      if (com_token[0] == '}')
	break;
      if (!data)
	gi.error ("ED_ParseEntity: EOF without closing brace");

      Q_strncpyz(keyname, com_token, sizeof(keyname));

      // parse value  
      com_token = COM_Parse (&data);
      if (!data)
	gi.error ("ED_ParseEntity: EOF without closing brace");

      if (com_token[0] == '}')
	gi.error ("ED_ParseEntity: closing brace without data");

      init = true;

      // keynames with a leading underscore are used for utility comments,
      // and are immediately discarded by quake
      if (keyname[0] == '_')
	continue;

      ED_ParseField (keyname, com_token, ent);
    }

  if (!init)
    memset (ent, 0, sizeof (*ent));

  return data;
}


/*
================
G_FindTeams

Chain together all entities with a matching team field.

All but the first will have the FL_TEAMSLAVE flag set.
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams (void)
{
	edict_t *e, *e2, *chain;
	int i, j;
	int c, c2;

	c = 0;
	c2 = 0;
	for (i = 1, e = g_edicts + i; i < globals.num_edicts; i++, e++)
	{
		if (!e->inuse || !e->team)
			continue;
		if (e->flags & FL_TEAMSLAVE)
			continue;
		chain = e;
		e->teammaster = e;
		c++;
		c2++;
		for (j = i + 1, e2 = e + 1; j < globals.num_edicts; j++, e2++)
		{
			if (!e2->inuse || !e2->team)
				continue;
			if (e2->flags & FL_TEAMSLAVE)
				continue;
			if (!strcmp (e->team, e2->team))
			{
				c2++;
				chain->teamchain = e2;
				e2->teammaster = e;
				chain = e2;
				e2->flags |= FL_TEAMSLAVE;
			}
		}
	}

	gi.dprintf ("%i teams with %i entities\n", c, c2);
}

//Precaches and enables download options for user sounds. All sounds
//have to be listed within "sndlist.ini". called from g_spawn.c -> SP_worldspawn
static void PrecacheUserSounds(void)
{
	int count = 0;
	size_t lenght;
	FILE *soundlist;
	char buf[1024], fullpath[MAX_QPATH];

	soundlist = fopen(GAMEVERSION "/sndlist.ini", "r");
	if (!soundlist) { // no "sndlist.ini" file...
		gi.dprintf("Cannot load %s, sound download is disabled.\n", GAMEVERSION "/sndlist.ini");
		return;
	}

	// read the sndlist.ini file
	while (fgets(buf, sizeof(buf), soundlist) != NULL)
	{
		lenght = strlen(buf);
		//first remove trailing spaces
		while (lenght > 0 && buf[lenght - 1] <= ' ')
			buf[--lenght] = '\0';

		//Comments are marked with # or // at line start
		if (lenght < 5 || buf[0] == '#' || !strncmp(buf, "//", 2))
			continue;

		Q_strncpyz(fullpath, PG_SNDPATH, sizeof(fullpath));
		Q_strncatz(fullpath, buf, sizeof(fullpath));
		gi.soundindex(fullpath);
		//gi.dprintf("Sound %s: precache %i",fullpath, gi.soundindex(fullpath)); 
		count++;
		if (count == 100)
			break;
	}
	fclose(soundlist);
	if (!count)
		gi.dprintf("%s is empty, no sounds to precache.\n", GAMEVERSION "/sndlist.ini");
	else
		gi.dprintf("%i user sounds precached.\n", count);
}

void G_LoadLocations( void )
{
	//AQ2:TNG New Location Code
	char	locfile[MAX_QPATH], buffer[256];
	FILE	*f;
	int		i, x, y, z, rx, ry, rz;
	char	*locationstr, *param, *line;
	cvar_t	*game_cvar;
	placedata_t *loc;

	memset( ml_creator, 0, sizeof( ml_creator ) );
	ml_count = 0;

	game_cvar = gi.cvar ("game", "action", 0);

	if (!*game_cvar->string)
		Com_sprintf(locfile, sizeof(locfile), "%s/tng/%s.aqg", GAMEVERSION, level.mapname);
	else
		Com_sprintf(locfile, sizeof(locfile), "%s/tng/%s.aqg", game_cvar->string, level.mapname);

	f = fopen( locfile, "r" );
	if (!f) {
		gi.dprintf( "No location file for %s\n", level.mapname );
		return;
	}

	gi.dprintf( "Location file: %s\n", level.mapname );

	do
	{
		line = fgets( buffer, sizeof( buffer ), f );
		if (!line) {
			break;
		}

		if (strlen( line ) < 12)
			continue;

		if (line[0] == '#')
		{
			param = line + 1;
			while (*param == ' ') { param++; }
			if (*param && !Q_strnicmp(param, "creator", 7))
			{
				param += 8;
				while (*param == ' ') { param++; }
				for (i = 0; *param >= ' ' && i < sizeof( ml_creator ) - 1; i++) {
					ml_creator[i] = *param++;
				}
				ml_creator[i] = 0;
				while (i > 0 && ml_creator[i - 1] == ' ') //Remove railing spaces
					ml_creator[--i] = 0;
			}
			continue;
		}

		param = strtok( line, " :\r\n\0" );
		// TODO: better support for file comments
		if (!param || param[0] == '#')
			continue;

		x = atoi( param );

		param = strtok( NULL, " :\r\n\0" );
		if (!param)
			continue;
		y = atoi( param );

		param = strtok( NULL, " :\r\n\0" );
		if (!param)
			continue;
		z = atoi( param );

		param = strtok( NULL, " :\r\n\0" );
		if (!param)
			continue;
		rx = atoi( param );

		param = strtok( NULL, " :\r\n\0" );
		if (!param)
			continue;
		ry = atoi( param );

		param = strtok( NULL, " :\r\n\0" );
		if (!param)
			continue;
		rz = atoi( param );

		param = strtok( NULL, "\r\n\0" );
		if (!param)
			continue;
		locationstr = param;

		loc = &locationbase[ml_count++];
		loc->x = x;
		loc->y = y;
		loc->z = z;
		loc->rx = rx;
		loc->ry = ry;
		loc->rz = rz;
		Q_strncpyz( loc->desc, locationstr, sizeof( loc->desc ) );

		if (ml_count >= MAX_LOCATIONS_IN_BASE) {
			gi.dprintf( "Cannot read more than %d locations.\n", MAX_LOCATIONS_IN_BASE );
			break;
		}
	} while (1);

	fclose( f );
	gi.dprintf( "Found %d locations.\n", ml_count );
}



int Gamemode(void) // These are distinct game modes; you cannot have a teamdm tourney mode, for example
{
	int gamemode = 0;
	if (teamdm->value) {
		gamemode = GM_TEAMDM;
	} else if (ctf->value) {
		gamemode = GM_CTF;
	} else if (use_tourney->value) {
		gamemode = GM_TOURNEY;
	} else if (teamplay->value) {
		gamemode = GM_TEAMPLAY;
	} else if (dom->value) {
		gamemode = GM_DOMINATION;
	} else if (deathmatch->value) {
		gamemode = GM_DEATHMATCH;
	}
	return gamemode;
}

int Gamemodeflag(void)
// These are gamemode flags that change the rules of gamemodes.
// For example, you can have a darkmatch matchmode 3team teamplay server
{
	int gamemodeflag = 0;
	char gmfstr[16];

	if (use_3teams->value) {
		gamemodeflag += GMF_3TEAMS;
	}
	if (darkmatch->value) {
		gamemodeflag += GMF_DARKMATCH;
	}
	if (matchmode->value) {
		gamemodeflag += GMF_MATCHMODE;
	}
	sprintf(gmfstr, "%d", gamemodeflag);
	gi.cvar_forceset("gmf", gmfstr);
	return gamemodeflag;
}

/*
==============
SpawnEntities

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.
==============
*/
void SpawnEntities (char *mapname, char *entities, char *spawnpoint)
{
	edict_t *ent = NULL;
	gclient_t   *client;
	client_persistant_t pers;
	int i, inhibit = 0;
	char *com_token;
	int saved_team;

	// Reset teamplay stuff
	for(i = TEAM1; i < TEAM_TOP; i++)
	{
		teams[i].score = teams[i].total = 0;
		teams[i].ready = teams[i].locked = 0;
		teams[i].pauses_used = teams[i].wantReset = 0;
		teams[i].captain = NULL;
		gi.cvar_forceset(teams[i].teamscore->name, "0");
	}

	day_cycle_at = 0;
	team_round_going = team_game_going = team_round_countdown = 0;
	lights_camera_action = holding_on_tie_check = 0;
	timewarning = fragwarning = 0;

	teamCount = 2;
	gameSettings = 0;

	if (jump->value)
	{
	gi.cvar_forceset(gm->name, "jump");
	gi.cvar_forceset(stat_logs->name, "0"); // Turn off stat logs for jump mode
		if (teamplay->value)
		{
			gi.dprintf ("Jump Enabled - Forcing teamplay ff\n");
			gi.cvar_forceset(teamplay->name, "0");
		}
		if (use_3teams->value)
		{
			gi.dprintf ("Jump Enabled - Forcing 3Teams off\n");
			gi.cvar_forceset(use_3teams->name, "0");
		}
		if (teamdm->value)
		{
			gi.dprintf ("Jump Enabled - Forcing Team DM off\n");
			gi.cvar_forceset(teamdm->name, "0");
		}
		if (use_tourney->value)
		{
			gi.dprintf ("Jump Enabled - Forcing Tourney off\n");
			gi.cvar_forceset(use_tourney->name, "0");
		}
		if (ctf->value)
		{
			gi.dprintf ("Jump Enabled - Forcing CTF off\n");
			gi.cvar_forceset(ctf->name, "0");
		}
		if (dom->value)
		{
			gi.dprintf ("Jump Enabled - Forcing Domination off\n");
			gi.cvar_forceset(dom->name, "0");
		}
		if (use_randoms->value)
		{
			gi.dprintf ("Jump Enabled - Forcing Random weapons and items off\n");
			gi.cvar_forceset(use_randoms->name, "0");
		}
	}
	else if (ctf->value)
	{
	gi.cvar_forceset(gm->name, "ctf");
		if (ctf->value == 2)
			gi.cvar_forceset(ctf->name, "1"); //for now

		gameSettings |= GS_WEAPONCHOOSE;

		// Make sure teamplay is enabled
		if (!teamplay->value)
		{
			gi.dprintf ("CTF Enabled - Forcing teamplay on\n");
			gi.cvar_forceset(teamplay->name, "1");
		}
		if (use_3teams->value)
		{
			gi.dprintf ("CTF Enabled - Forcing 3Teams off\n");
			gi.cvar_forceset(use_3teams->name, "0");
		}
		if(teamdm->value)
		{
			gi.dprintf ("CTF Enabled - Forcing Team DM off\n");
			gi.cvar_forceset(teamdm->name, "0");
		}
		if (use_tourney->value)
		{
			gi.dprintf ("CTF Enabled - Forcing Tourney off\n");
			gi.cvar_forceset(use_tourney->name, "0");
		}
		if (dom->value)
		{
			gi.dprintf ("CTF Enabled - Forcing Domination off\n");
			gi.cvar_forceset(dom->name, "0");
		}
		if (!DMFLAGS(DF_NO_FRIENDLY_FIRE))
		{
			gi.dprintf ("CTF Enabled - Forcing Friendly Fire off\n");
			gi.cvar_forceset(dmflags->name, va("%i", (int)dmflags->value | DF_NO_FRIENDLY_FIRE));
		}
		if (use_randoms->value)
		{
			gi.dprintf ("CTF Enabled - Forcing Random weapons and items off\n");
			gi.cvar_forceset(use_randoms->name, "0");
		}
		Q_strncpyz(teams[TEAM1].name, "RED", sizeof(teams[TEAM1].name));
		Q_strncpyz(teams[TEAM2].name, "BLUE", sizeof(teams[TEAM2].name));
		Q_strncpyz(teams[TEAM1].skin, "male/ctf_r", sizeof(teams[TEAM1].skin));
		Q_strncpyz(teams[TEAM2].skin, "male/ctf_b", sizeof(teams[TEAM2].skin));
		Q_strncpyz(teams[TEAM1].skin_index, "i_ctf1", sizeof(teams[TEAM1].skin_index));
		Q_strncpyz(teams[TEAM2].skin_index, "i_ctf2", sizeof(teams[TEAM2].skin_index));
	}
	else if (dom->value)
	{
		gi.cvar_forceset(gm->name, "dom");
		gameSettings |= GS_WEAPONCHOOSE;
		if (!teamplay->value)
		{
			gi.dprintf ("Domination Enabled - Forcing teamplay on\n");
			gi.cvar_forceset(teamplay->name, "1");
		}
		if (teamdm->value)
		{
			gi.dprintf ("Domination Enabled - Forcing Team DM off\n");
			gi.cvar_forceset(teamdm->name, "0");
		}
		if (use_tourney->value)
		{
			gi.dprintf ("Domination Enabled - Forcing Tourney off\n");
			gi.cvar_forceset(use_tourney->name, "0");
		}
		if (use_randoms->value)
		{
			gi.dprintf ("Domination Enabled - Forcing Random weapons and items off\n");
			gi.cvar_forceset(use_randoms->name, "0");
		}
		Q_strncpyz(teams[TEAM1].name, "RED", sizeof(teams[TEAM1].name));
		Q_strncpyz(teams[TEAM2].name, "BLUE", sizeof(teams[TEAM2].name));
		Q_strncpyz(teams[TEAM3].name, "GREEN", sizeof(teams[TEAM3].name));
		Q_strncpyz(teams[TEAM1].skin, "male/ctf_r", sizeof(teams[TEAM1].skin));
		Q_strncpyz(teams[TEAM2].skin, "male/ctf_b", sizeof(teams[TEAM2].skin));
		Q_strncpyz(teams[TEAM3].skin, "male/commando", sizeof(teams[TEAM3].skin));
		Q_strncpyz(teams[TEAM1].skin_index, "i_ctf1", sizeof(teams[TEAM1].skin_index));
		Q_strncpyz(teams[TEAM2].skin_index, "i_ctf2", sizeof(teams[TEAM2].skin_index));
		Q_strncpyz(teams[TEAM3].skin_index, "i_pack", sizeof(teams[TEAM3].skin_index));
	}
	else if(teamdm->value)
	{
		gi.cvar_forceset(gm->name, "tdm");
		gameSettings |= GS_DEATHMATCH;

		if (dm_choose->value)
			gameSettings |= GS_WEAPONCHOOSE;

		if (!teamplay->value)
		{
			gi.dprintf ("Team Deathmatch Enabled - Forcing teamplay on\n");
			gi.cvar_forceset(teamplay->name, "1");
		}
		if (use_tourney->value)
		{
			gi.dprintf ("Team Deathmatch Enabled - Forcing Tourney off\n");
			gi.cvar_forceset(use_tourney->name, "0");
		}
	}
	else if (use_3teams->value)
	{
		gi.cvar_forceset(gm->name, "tp");
		gameSettings |= (GS_ROUNDBASED | GS_WEAPONCHOOSE);

		if (!teamplay->value)
		{
			gi.dprintf ("3 Teams Enabled - Forcing teamplay on\n");
			gi.cvar_forceset(teamplay->name, "1");
		}
		if (use_tourney->value)
		{
			gi.dprintf ("3 Teams Enabled - Forcing Tourney off\n");
			gi.cvar_forceset(use_tourney->name, "0");
		}
	}
	else if (matchmode->value)
	{
		gameSettings |= (GS_ROUNDBASED | GS_WEAPONCHOOSE);
		if (!teamplay->value)
		{
			gi.dprintf ("Matchmode Enabled - Forcing teamplay on\n");
			gi.cvar_forceset(teamplay->name, "1");
		}
		if (use_tourney->value)
		{
			gi.dprintf ("Matchmode Enabled - Forcing Tourney off\n");
			gi.cvar_forceset(use_tourney->name, "0");
		}
	}
	else if (use_tourney->value)
	{
		gi.cvar_forceset(gm->name, "tourney");
		gameSettings |= (GS_ROUNDBASED | GS_WEAPONCHOOSE);

		if (!teamplay->value)
		{
			gi.dprintf ("Tourney Enabled - Forcing teamplay on\n");
			gi.cvar_forceset(teamplay->name, "1");
		}
	}
	else if (teamplay->value)
	{
		gi.cvar_forceset(gm->name, "tp");
		gameSettings |= (GS_ROUNDBASED | GS_WEAPONCHOOSE);
	}
	else { //Its deathmatch
		gi.cvar_forceset(gm->name, "dm");
		gameSettings |= GS_DEATHMATCH;
		if (dm_choose->value)
			gameSettings |= GS_WEAPONCHOOSE;
	}

	if (teamplay->value)
	{
		gameSettings |= GS_TEAMPLAY;
	}
	if (matchmode->value)
	{
		gameSettings |= GS_MATCHMODE;
		gi.dprintf ("Matchmode Enabled - Forcing g_spawn_items off\n");
		gi.cvar_forceset(g_spawn_items->name, "0"); // Turn off spawning of items for matchmode games
	}
	if (use_3teams->value)
	{
		teamCount = 3;
		if (!use_oldspawns->value)
		{
			gi.dprintf ("3 Teams Enabled - Forcing use_oldspawns on\n");
			gi.cvar_forceset(use_oldspawns->name, "1");
		}
	}


	gi.cvar_forceset(maptime->name, "0:00");

	gi.FreeTags(TAG_LEVEL);

	// Set serverinfo correctly for gamemodeflags
	Gamemodeflag();

	memset(&level, 0, sizeof (level));
	memset(g_edicts, 0, game.maxentities * sizeof (g_edicts[0]));

	Q_strncpyz(level.mapname, mapname, sizeof(level.mapname));
	Q_strncpyz(game.spawnpoint, spawnpoint, sizeof(game.spawnpoint));

	InitTransparentList();

	// set client fields on player ents
	for (i = 0, ent = &g_edicts[1]; i < game.maxclients; i++, ent++)
	{
		client = &game.clients[i];
		ent->client = client;

		// clear everything but the persistant data
		pers = client->pers;
		saved_team = client->resp.team;
		memset(client, 0, sizeof(*client));
		client->pers = pers;
		if( pers.connected )
		{
			client->clientNum = i;

			if( auto_join->value )
				client->resp.team = saved_team;

			// combine name and skin into a configstring
			AssignSkin( ent, Info_ValueForKey( client->pers.userinfo, "skin" ), false );
		}
	}

	ent = NULL;
	// parse ents
	while (1)
	{
		// parse the opening brace      
		com_token = COM_Parse(&entities);
		if (!entities)
			break;

		if (com_token[0] != '{')
			gi.error ("ED_LoadFromFile: found %s when expecting {", com_token);

		if (!ent)
			ent = g_edicts;
		else
			ent = G_Spawn();

		entities = ED_ParseEdict(entities, ent);

		// yet another map hack
		if (!Q_stricmp (level.mapname, "command")
			&& !Q_stricmp (ent->classname, "trigger_once")
			&& !Q_stricmp (ent->model, "*27"))
			ent->spawnflags &= ~SPAWNFLAG_NOT_HARD;

		// remove things (except the world) from different skill levels or deathmatch
		if (ent != g_edicts)
		{
			if (ent->spawnflags & SPAWNFLAG_NOT_DEATHMATCH)
			{
				G_FreeEdict (ent);
				inhibit++;
				continue;
			}

			ent->spawnflags &=
			~(SPAWNFLAG_NOT_EASY | SPAWNFLAG_NOT_MEDIUM | SPAWNFLAG_NOT_HARD |
			SPAWNFLAG_NOT_COOP | SPAWNFLAG_NOT_DEATHMATCH);
		}

		ED_CallSpawn (ent);
	}

	gi.dprintf ("%i entities inhibited\n", inhibit);

	// AQ2:TNG Igor adding .flg files

	// CTF configuration
	if(ctf->value)
	{
		if(!CTFLoadConfig(level.mapname))
		{
			if ((!G_Find(NULL, FOFS (classname), "item_flag_team1") ||
				 !G_Find(NULL, FOFS (classname), "item_flag_team2")))
			{
				gi.dprintf ("No native CTF map, loading flag positions from file\n");
				if (LoadFlagsFromFile(level.mapname))
					ChangePlayerSpawns ();
			}
		}
	}
	else if( dom->value )
		DomLoadConfig( level.mapname );

	G_FindTeams();

	// TNG:Freud - Ghosts
	num_ghost_players = 0;

	if (!(gameSettings & GS_WEAPONCHOOSE) && !jump->value)
	{
		//zucc for special items
		SetupSpecSpawn();
	}
	else if (teamplay->value)
	{
		GetSpawnPoints();
		//TNG:Freud - New spawning system
		if(!use_oldspawns->value)
			NS_GetSpawnPoints();
	}

	G_LoadLocations();

	SVCmd_CheckSB_f(); //rekkie -- silence ban

	UnBan_TeamKillers();
}


//===================================================================

#if 0
	// cursor positioning
xl < value > xr < value > yb < value > yt < value > xv < value > yv < value >
  // drawing
  statpic < name > pic < stat > num < fieldwidth > <stat > string < stat >
  // control
  if <stat
  >ifeq < stat > <value > ifbit < stat > <value > endif
#endif

#define STATBAR_COMMON \
/* team icon (draw first to prevent covering health at 320x240) */ \
	"if 4 " \
		"xl 0 " \
		"yb -32 " \
		"pic 4 " \
	"endif " \
	"yb -24 " \
/* health */ \
	"if 0 " \
		"xv 0 " \
		"hnum " \
		"xv 50 " \
		"pic 0 " \
	"endif " \
/* ammo */ \
	"if 2 " \
		"xv 100 " \
		"anum " \
		"xv 150 " \
		"pic 2 " \
	"endif " \
/* selected item */ \
	"if 6 " \
		"xv 296 " \
		"pic 6 " \
	"endif " \
	"yb -50 " \
/* picked up item */ \
	"if 7 " \
		"xv 0 " \
		"pic 7 " \
		"xv 26 " \
		"yb -42 " \
		"stat_string 8 " \
		"yb -50 " \
	"endif " \
/*  help / weapon icon */ \
	"if 11 " \
		"xv 148 " \
		"pic 11 " \
	"endif " \
/* clip(s) */ \
	"if 16 " \
		"yb -24 " \
		"xr -60 " \
		"num 2 17 " \
		"xr -24 " \
		"pic 16 " \
	"endif " \
/* special item ( vest etc ) */ \
	"if 19 " \
		"yb -72 " \
		"xr -24 " \
		"pic 19 " \
	"endif " \
/* special weapon */ \
	"if 20 " \
		"yb -48 " \
		"xr -24 " \
		"pic 20 " \
	"endif " \
/* grenades */ \
	"if 28 " \
		"yb -96 " \
		"xr -60 " \
		"num 2 29 " \
		"xr -24 " \
		"pic 28 " \
	"endif " \
/* spec viewing */ \
	"if 21 " \
		"xv 0 " \
		"yb -58 " \
		"string \"Viewing\" " \
		"xv 64 " \
		"stat_string 21 " \
	"endif " \
/* sniper graphic/icon */ \
	"if 18 " \
		"xv 0 " \
		"yv 0 " \
		"pic 18 " \
	"endif "

void G_SetupStatusbar( void )
{
	level.statusbar[0] = 0;

	if (!nohud->value)
	{
		Q_strncpyz(level.statusbar, STATBAR_COMMON, sizeof(level.statusbar));

		if(!((noscore->value || hud_noscore->value) && teamplay->value)) //  frags
			Q_strncatz(level.statusbar, "xr -50 yt 2 num 3 14 ", sizeof(level.statusbar));

		if (ctf->value)
		{
			Q_strncatz(level.statusbar, 
				// Red Team
				"yb -164 " "if 24 " "xr -24 " "pic 24 " "endif " "xr -60 " "num 2 26 "
				// Blue Team
				"yb -140 " "if 25 " "xr -24 " "pic 25 " "endif " "xr -60 " "num 2 27 "
				// Flag carried
				"if 23 " "yt 26 " "xr -24 " "pic 23 " "endif ",
				sizeof(level.statusbar) );
		}
		else if( dom->value )
			DomSetupStatusbar();
		else if( jump->value )
			strcpy( level.statusbar, jump_statusbar );
	}

	Q_strncpyz(level.spec_statusbar, level.statusbar, sizeof(level.spec_statusbar));
	level.spec_statusbar_lastupdate = 0;
}

void G_UpdateSpectatorStatusbar( void )
{
	char buffer[2048];
	int i, count, isAlive;
	char *rowText;
	gclient_t *cl;
	edict_t *cl_ent;

	Q_strncpyz(buffer, level.statusbar, sizeof(buffer));

	//Team 1
	count = 0;
	//rowText = "Name            Frg Tim Png";
	rowText = "Name            Health   Score";
	sprintf( buffer + strlen( buffer ), "xl 0 yt %d string2 \"%s\" ", 40, rowText );
	for (i = 0, cl = game.clients; i < game.maxclients; i++, cl++) {
		if (!cl->pers.connected || cl->resp.team != TEAM1)
			continue;

		if (cl->resp.subteam)
			continue;

		cl_ent = g_edicts + 1 + i;

		isAlive = IS_ALIVE(cl_ent);

		sprintf( buffer + strlen( buffer ),
			"yt %d string%s \"%-15s %6d   %5d\" ",
			48 + count * 8,
			isAlive ? "2" : "",
			cl->pers.netname,
			isAlive ? cl_ent->health : 0,
			cl->resp.score );

		count++;
		if (count >= 5)
			break;
	}

	//Team 2
	count = 0;
	sprintf( buffer + strlen( buffer ), "xr -%d yt %d string2 \"%s\" ", (int) strlen(rowText) * 8, 40, rowText );
	for (i = 0, cl = game.clients; i < game.maxclients; i++, cl++) {
		if (!cl->pers.connected || cl->resp.team != TEAM2)
			continue;

		if (cl->resp.subteam)
			continue;

		cl_ent = g_edicts + 1 + i;

		isAlive = IS_ALIVE(cl_ent);

		sprintf( buffer + strlen( buffer ),
			"yt %d string%s \"%-15s %6d   %5d\" ",
			48 + count * 8,
			isAlive ? "2" : "",
			cl->pers.netname,
			isAlive ? cl_ent->health : 0,
			cl->resp.score );

		count++;
		if (count >= 5)
			break;
	}

	if (strlen( buffer ) > 1023) {
		buffer[1023] = 0;
	}

	if (strcmp(level.spec_statusbar, buffer)) {
		Q_strncpyz(level.spec_statusbar, buffer, sizeof(level.spec_statusbar));
		level.spec_statusbar_lastupdate = level.realFramenum;
	}
}

/*
==============
G_UpdatePlayerStatusbar
==============
*/
void G_UpdatePlayerStatusbar( edict_t * ent, int force )
{
	char *playerStatusbar;
	if (!teamplay->value || teamCount != 2 || spectator_hud->value <= 0 || !(ent->client->pers.spec_flags & SPECFL_SPECHUD)) {
		return;
	}

	playerStatusbar = level.statusbar;

	if (!ent->client->resp.team)
	{
		if (level.spec_statusbar_lastupdate < level.realFramenum - 3 * HZ) {
			G_UpdateSpectatorStatusbar();
			if (level.spec_statusbar_lastupdate < level.realFramenum - 3 * HZ && !force)
				return;
		}

		playerStatusbar = level.spec_statusbar;
	}
	gi.WriteByte( svc_configstring );
	gi.WriteShort( CS_STATUSBAR );
	gi.WriteString( playerStatusbar );
	gi.unicast( ent, force ? true : false );
}

/*QUAKED worldspawn (0 0 0) ?

Only used for the world.
"sky"   environment map name
"skyaxis"       vector axis for rotating sky
"skyrotate"     speed of rotation in degrees/second
"sounds"        music cd track number
"gravity"       800 is default gravity
"message"       text to print at user logon
*/

void SP_worldspawn (edict_t * ent)
{
	int i, bullets, shells;
	char *picname;

	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_BSP;
	ent->inuse = true;		// since the world doesn't use G_Spawn()
	ent->s.modelindex = 1;	// world model is always index 1

	// reserve some spots for dead player bodies for coop / deathmatch
	InitBodyQue();

	// set configstrings for items
	SetItemNames();

	level.framenum = 0;
	level.time = 0;
	level.realFramenum = 0;
	level.pauseFrames = 0;
	level.matchTime = 0;
	level.weapon_sound_framenum = 0;

	if (st.nextmap)
		strcpy(level.nextmap, st.nextmap);

	// make some data visible to the server

	if (ent->message && ent->message[0])
	{
		Q_strncpyz(level.level_name, ent->message, sizeof(level.level_name));
		gi.configstring(CS_NAME, level.level_name);
	}
	else {
		strcpy(level.level_name, level.mapname);
	}

	if (st.sky && st.sky[0])
		gi.configstring(CS_SKY, st.sky);
	else
		gi.configstring(CS_SKY, "unit1_");

	gi.configstring(CS_SKYROTATE, va("%f", st.skyrotate));
	gi.configstring(CS_SKYAXIS, va("%f %f %f", st.skyaxis[0], st.skyaxis[1], st.skyaxis[2]));
	gi.configstring(CS_CDTRACK, va("%i", ent->sounds));
	gi.configstring(CS_MAXCLIENTS, va("%i", game.maxclients));

	G_SetupStatusbar();
	gi.configstring(CS_STATUSBAR, level.statusbar);

	level.pic_health = gi.imageindex("i_health");
	gi.imageindex("field_3");

	// zucc - preload sniper stuff
	level.pic_sniper_mode[1] = gi.imageindex("scope2x");
	level.pic_sniper_mode[2] = gi.imageindex("scope4x");
	level.pic_sniper_mode[3] = gi.imageindex("scope6x");

	for (i = 1; i < AMMO_MAX; i++) {
		picname = GET_ITEM(i)->icon;
		if (picname)
			level.pic_items[i] = gi.imageindex( picname );
	}

	bullets = gi.imageindex("a_bullets");
	shells = gi.imageindex("a_shells");
	level.pic_weapon_ammo[MK23_NUM] = bullets;
	level.pic_weapon_ammo[MP5_NUM] = bullets;
	level.pic_weapon_ammo[M4_NUM] = bullets;
	level.pic_weapon_ammo[M3_NUM] = shells;
	level.pic_weapon_ammo[HC_NUM] = shells;
	level.pic_weapon_ammo[SNIPER_NUM] = bullets;
	level.pic_weapon_ammo[DUAL_NUM] = bullets;
	level.pic_weapon_ammo[KNIFE_NUM] = gi.imageindex("w_knife");
	level.pic_weapon_ammo[GRENADE_NUM] = gi.imageindex("a_m61frag");

	gi.imageindex("tag1");
	gi.imageindex("tag2");
	if (teamplay->value)
	{
		level.pic_teamtag = gi.imageindex("tag3");

		if (ctf->value) {
			level.pic_ctf_teamtag[TEAM1] = gi.imageindex("ctfsb1");
			level.pic_ctf_flagbase[TEAM1] = gi.imageindex("i_ctf1");
			level.pic_ctf_flagtaken[TEAM1] = gi.imageindex("i_ctf1t");
			level.pic_ctf_flagdropped[TEAM1] = gi.imageindex("i_ctf1d");

			level.pic_ctf_teamtag[TEAM2] = gi.imageindex("ctfsb2");
			level.pic_ctf_flagbase[TEAM2] = gi.imageindex("i_ctf2");
			level.pic_ctf_flagtaken[TEAM2] = gi.imageindex("i_ctf2t");
			level.pic_ctf_flagdropped[TEAM2] = gi.imageindex("i_ctf2d");
			gi.imageindex("sbfctf1");
			gi.imageindex("sbfctf2");
		}

		for(i = TEAM1; i <= teamCount; i++)
		{
			if (teams[i].skin_index[0] == 0) {
				// If the action.ini file isn't found, set default skins rather than kill the server
				gi.dprintf("WARNING: No skin was specified for team %i in config file, server either could not find it or is does not exist.\n", i);
				gi.dprintf("Setting default team names, skins and skin indexes.\n", i);
				Q_strncpyz(teams[TEAM1].name, "RED", sizeof(teams[TEAM1].name));
				Q_strncpyz(teams[TEAM2].name, "BLUE", sizeof(teams[TEAM2].name));
				Q_strncpyz(teams[TEAM3].name, "GREEN", sizeof(teams[TEAM3].name));
				Q_strncpyz(teams[TEAM1].skin, "male/ctf_r", sizeof(teams[TEAM1].skin));
				Q_strncpyz(teams[TEAM2].skin, "male/ctf_b", sizeof(teams[TEAM2].skin));
				Q_strncpyz(teams[TEAM3].skin, "male/commando", sizeof(teams[TEAM3].skin));
				Q_strncpyz(teams[TEAM1].skin_index, "i_ctf1", sizeof(teams[TEAM1].skin_index));
				Q_strncpyz(teams[TEAM2].skin_index, "i_ctf2", sizeof(teams[TEAM2].skin_index));
				Q_strncpyz(teams[TEAM3].skin_index, "i_pack", sizeof(teams[TEAM3].skin_index));
				//exit(1);
			}
			level.pic_teamskin[i] = gi.imageindex(teams[i].skin_index);
		}

		level.snd_lights = gi.soundindex("atl/lights.wav");
		level.snd_camera = gi.soundindex("atl/camera.wav");
		level.snd_action = gi.soundindex("atl/action.wav");
		level.snd_teamwins[0] = gi.soundindex("tng/no_team_wins.wav");
		level.snd_teamwins[1] = gi.soundindex("tng/team1_wins.wav");
		level.snd_teamwins[2] = gi.soundindex("tng/team2_wins.wav");
		level.snd_teamwins[3] = gi.soundindex("tng/team3_wins.wav");
	}

	level.snd_silencer = gi.soundindex("misc/silencer.wav");	// all silencer weapons
	level.snd_headshot = gi.soundindex("misc/headshot.wav");	// headshot sound
	level.snd_vesthit = gi.soundindex("misc/vest.wav");		// kevlar hit
	level.snd_knifethrow = gi.soundindex("misc/flyloop.wav");	// throwing knife
	level.snd_kick = gi.soundindex("weapons/kick.wav");	// not loaded by any item, kick sound
	level.snd_noammo = gi.soundindex("weapons/noammo.wav");

	gi.soundindex("tng/1_minute.wav");
	gi.soundindex("tng/3_minutes.wav");
	gi.soundindex("tng/1_frag.wav");
	gi.soundindex("tng/2_frags.wav");
	gi.soundindex("tng/3_frags.wav");
	gi.soundindex("tng/impressive.wav");
	gi.soundindex("tng/excellent.wav");
	gi.soundindex("tng/accuracy.wav");
	gi.soundindex("tng/clanwar.wav");
	gi.soundindex("tng/disabled.wav");
	gi.soundindex("tng/enabled.wav");
	gi.soundindex("misc/flashlight.wav"); // Caching Flashlight

	gi.soundindex("world/10_0.wav");	// countdown
	gi.soundindex("world/xian1.wav");	// intermission music
	gi.soundindex("misc/secret.wav");	// used for ctf swap sound
	gi.soundindex("weapons/grenlf1a.wav");	// respawn sound

	PrecacheItems();
	PrecacheRadioSounds();
	PrecacheUserSounds();

	TourneyInit();
	vInitLevel();

	if (!st.gravity)
		gi.cvar_set("sv_gravity", "800");
	else
		gi.cvar_set("sv_gravity", st.gravity);

	level.snd_fry = gi.soundindex("player/fry.wav");	// standing in lava / slime

	gi.soundindex("player/lava1.wav");
	gi.soundindex("player/lava2.wav");

	gi.soundindex("misc/pc_up.wav");
	gi.soundindex("misc/talk1.wav");

	gi.soundindex("misc/udeath.wav");
	gi.soundindex("misc/glurp.wav");

	// gibs
	gi.soundindex("items/respawn1.wav");

	// sexed sounds
	gi.soundindex("*death1.wav");
	gi.soundindex("*death2.wav");
	gi.soundindex("*death3.wav");
	gi.soundindex("*death4.wav");
	gi.soundindex("*fall1.wav");
	gi.soundindex("*fall2.wav");
	gi.soundindex("*gurp1.wav");	// drowning damage
	gi.soundindex("*gurp2.wav");
	//gi.soundindex("*jump1.wav");	// player jump - AQ2 doesn't use this
	gi.soundindex("*pain25_1.wav");
	gi.soundindex("*pain25_2.wav");
	gi.soundindex("*pain50_1.wav");
	gi.soundindex("*pain50_2.wav");
	gi.soundindex("*pain75_1.wav");
	gi.soundindex("*pain75_2.wav");
	gi.soundindex("*pain100_1.wav");
	gi.soundindex("*pain100_2.wav");

	//-------------------

	// precache vwep models
	// THIS ORDER MUST MATCH THE DEFINES IN g_local.h
	gi.modelindex("#w_mk23.md2");
	gi.modelindex("#w_mp5.md2");
	gi.modelindex("#w_m4.md2");
	gi.modelindex("#w_super90.md2");
	gi.modelindex("#w_cannon.md2");
	gi.modelindex("#w_sniper.md2");
	gi.modelindex("#w_akimbo.md2");
	gi.modelindex("#w_knife.md2");
	gi.modelindex("#a_m61frag.md2");

	level.model_null = gi.modelindex("sprites/null.sp2");      // null sprite
	level.model_lsight = gi.modelindex("sprites/lsight.sp2");  // laser sight dot sprite
	gi.soundindex("player/gasp1.wav");	// gasping for air
	gi.soundindex("player/gasp2.wav");	// head breaking surface, not gasping

	gi.soundindex("player/watr_in.wav");	// feet hitting water
	gi.soundindex("player/watr_out.wav");	// feet leaving water

	gi.soundindex("player/watr_un.wav");	// head going underwater

	gi.soundindex("player/u_breath1.wav");
	gi.soundindex("player/u_breath2.wav");

	gi.soundindex("items/pkup.wav");	// bonus item pickup
	gi.soundindex("world/land.wav");	// landing thud
	gi.soundindex("misc/h2ohit1.wav");	// landing splash

	gi.soundindex("items/damage.wav");
	gi.soundindex("items/protect.wav");
	gi.soundindex("items/protect4.wav");

	gi.soundindex("infantry/inflies1.wav");

	sm_meat_index = gi.modelindex("models/objects/gibs/sm_meat/tris.md2");
	gi.modelindex("models/objects/gibs/skull/tris.md2");


//
// Setup light animation tables. 'a' is total darkness, 'z' is doublebright.
//

	/*
		Darkmatch.
		Darkmatch has three settings:
		0 - normal, no changes to lights
		1 - all lights are off.
		2 - dusk/dawn mode.
		3 - using the day_cycle to change the lights every xx seconds as defined by day_cycle
	*/
	if (darkmatch->value == 1)
		gi.configstring (CS_LIGHTS + 0, "a");	// Pitch Black
	else if (darkmatch->value == 2)
		gi.configstring (CS_LIGHTS + 0, "b");	// Dusk
	else
		gi.configstring (CS_LIGHTS + 0, "m");	// 0 normal

	gi.configstring(CS_LIGHTS + 1, "mmnmmommommnonmmonqnmmo");	// 1 FLICKER (first variety)
	gi.configstring(CS_LIGHTS + 2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");	// 2 SLOW STRONG PULSE
	gi.configstring(CS_LIGHTS + 3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");	// 3 CANDLE (first variety)
	gi.configstring(CS_LIGHTS + 4, "mamamamamama");	// 4 FAST STROBE
	gi.configstring(CS_LIGHTS + 5, "jklmnopqrstuvwxyzyxwvutsrqponmlkj");	// 5 GENTLE PULSE 1
	gi.configstring(CS_LIGHTS + 6, "nmonqnmomnmomomno");	// 6 FLICKER (second variety)
	gi.configstring(CS_LIGHTS + 7, "mmmaaaabcdefgmmmmaaaammmaamm");	// 7 CANDLE (second variety)
	gi.configstring(CS_LIGHTS + 8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");	// 8 CANDLE (third variety)
	gi.configstring(CS_LIGHTS + 9, "aaaaaaaazzzzzzzz");	// 9 SLOW STROBE (fourth variety)
	gi.configstring(CS_LIGHTS + 10, "mmamammmmammamamaaamammma");	// 10 FLUORESCENT FLICKER
	gi.configstring(CS_LIGHTS + 11, "abcdefghijklmnopqrrqponmlkjihgfedcba");	// 11 SLOW PULSE NOT FADE TO BLACK
	// styles 32-62 are assigned by the light program for switchable lights
	gi.configstring(CS_LIGHTS + 63, "a");	// 63 testing
}

int LoadFlagsFromFile (const char *mapname)
{
	FILE *fp;
	char buf[1024], *s;
	int flagCount = 0;
	edict_t *ent;
	vec3_t position;
	size_t length;

	Com_sprintf(buf, sizeof(buf), "%s/tng/%s.flg", GAMEVERSION, mapname);
	fp = fopen(buf, "r");
	if (!fp)  {
		gi.dprintf("Warning: No flag definition file for map %s.\n", mapname);
		return 0;
	}

	// FIXME: remove this functionality completely in the future
	gi.dprintf("Warning: .flg files are deprecated, use .ctf ones for more control!\n");

	while (fgets(buf, 1000, fp) != NULL)
	{
		length = strlen(buf);
		if (length < 7)
			continue;

		//first remove trailing spaces
		s = buf + length - 1;
		for (; *s && (*s == '\r' || *s == '\n' || *s == ' '); s--)
			*s = '\0';

		//check if it's a valid line
		length = strlen(buf);
		if (length < 7 || buf[0] == '#' || !strncmp(buf, "//", 2))
			continue;

		//a little bit dirty... :)
		if (sscanf(buf, "<%f %f %f>", &position[0], &position[1], &position[2]) != 3)
			continue;

		ent = G_Spawn ();

		ent->spawnflags &=
			~(SPAWNFLAG_NOT_EASY | SPAWNFLAG_NOT_MEDIUM | SPAWNFLAG_NOT_HARD |
			SPAWNFLAG_NOT_COOP | SPAWNFLAG_NOT_DEATHMATCH);

		VectorCopy(position, ent->s.origin);

		if (!flagCount)	// Red Flag
			ent->classname = ED_NewString ("item_flag_team1");
		else	// Blue Flag
			ent->classname = ED_NewString ("item_flag_team2");

		ED_CallSpawn (ent);
		flagCount++;
		if (flagCount == 2)
			break;
	}

	fclose(fp);

	if (flagCount < 2)
		return 0;

	return 1;
}

// This function changes the nearest two spawnpoint from each flag
// to info_player_teamX, so the other team won't restart
// beneath the flag of the other team
void ChangePlayerSpawns ()
{
	edict_t *flag1 = NULL, *flag2 = NULL;
	edict_t *spot, *spot1, *spot2, *spot3, *spot4;
	float range, range1, range2, range3, range4;

	range1 = range2 = range3 = range4 = 99999;
	spot = spot1 = spot2 = spot3 = spot4 = NULL;

	flag1 = G_Find (flag1, FOFS(classname), "item_flag_team1");
	flag2 = G_Find (flag2, FOFS(classname), "item_flag_team2");

	if(!flag1 || !flag2) {
		gi.dprintf("Warning: ChangePlayerSpawns() requires both flags!\n");
		return;
	}

	while ((spot = G_Find(spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		range = Distance(spot->s.origin, flag1->s.origin);
		if (range < range1)
		{
			range3 = range1;
			spot3 = spot1;
			range1 = range;
			spot1 = spot;
		}
		else if (range < range3)
		{
			range3 = range;
			spot3 = spot;
		}

		range = Distance(spot->s.origin, flag2->s.origin);
		if (range < range2)
		{
			range4 = range2;
			spot4 = spot2;
			range2 = range;
			spot2 = spot;
		}
		else if (range < range4)
		{
			range4 = range;
			spot4 = spot;
		}
	}

	if (spot1)
	{
		// gi.dprintf ("Ersetze info_player_deathmatch auf <%f %f %f> durch info_player_team1\n", spot1->s.origin[0], spot1->s.origin[1], spot1->s.origin[2]);
		strcpy (spot1->classname, "info_player_team1");
	}

	if (spot2)
	{
		// gi.dprintf ("Ersetze info_player_deathmatch auf <%f %f %f> durch info_player_team2\n", spot2->s.origin[0], spot2->s.origin[1], spot2->s.origin[2]);
		strcpy (spot2->classname, "info_player_team2");
	}

	if (spot3)
	{
		// gi.dprintf ("Ersetze info_player_deathmatch auf <%f %f %f> durch info_player_team1\n", spot3->s.origin[0], spot3->s.origin[1], spot3->s.origin[2]);
		strcpy (spot3->classname, "info_player_team1");
	}

	if (spot4)
	{
		// gi.dprintf ("Ersetze info_player_deathmatch auf <%f %f %f> durch info_player_team2\n", spot4->s.origin[0], spot4->s.origin[1], spot4->s.origin[2]);
		strcpy (spot4->classname, "info_player_team2");
	}

}
