//-----------------------------------------------------------------------------
// PG BUND
// a_xcmds.c
//
// contains all new non standard command functions
//
// $Id: a_xcmds.c,v 1.15 2004/04/08 23:19:51 slicerdw Exp $
//
//-----------------------------------------------------------------------------
// $Log: a_xcmds.c,v $
// Revision 1.15  2004/04/08 23:19:51  slicerdw
// Optimized some code, added a couple of features and fixed minor bugs
//
// Revision 1.14  2004/01/18 11:20:14  igor_rock
// added flashgrenades
//
// Revision 1.13  2002/04/01 15:16:06  freud
// Stats code redone, tng_stats now much more smarter. Removed a few global
// variables regarding stats code and added kevlar hits to stats.
//
// Revision 1.12  2002/03/26 21:49:01  ra
// Bufferoverflow fixes
//
// Revision 1.11  2001/11/08 20:56:24  igor_rock
// - changed some things related to wp_flags
// - corrected use_punch bug when player only has an empty weapon left
//
// Revision 1.10  2001/11/08 13:22:18  igor_rock
// added missing parenthises (use_punch didn't function correct)
//
// Revision 1.9  2001/11/03 17:21:57  deathwatch
// Fixed something in the time command, removed the .. message from the voice command, fixed the vote spamming with mapvote, removed addpoint command (old pb command that wasnt being used). Some cleaning up of the source at a few points.
//
// Revision 1.8  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.7  2001/09/28 13:44:23  slicerdw
// Several Changes / improvements
//
// Revision 1.6  2001/08/15 14:50:48  slicerdw
// Added Flood protections to Radio & Voice, Fixed the sniper bug AGAIN
//
// Revision 1.5  2001/07/13 00:34:52  slicerdw
// Adjusted Punch command
//
// Revision 1.4  2001/05/31 16:58:14  igor_rock
// conflicts resolved
//
// Revision 1.3.2.2  2001/05/31 06:47:51  igor_rock
// - removed crash bug with non exisitng flag files
// - added new commands "setflag1", "setflag2" and "saveflags" to create
//   .flg files
//
// Revision 1.3.2.1  2001/05/25 18:59:52  igor_rock
// Added CTF Mode completly :)
// Support for .flg files is still missing, but with "real" CTF maps like
// tq2gtd1 the ctf works fine.
// (I hope that all other modes still work, just tested DM and teamplay)
//
// Revision 1.3  2001/05/13 01:23:01  deathwatch
// Added Single Barreled Handcannon mode, made the menus and scoreboards
// look nicer and made the voice command a bit less loud.
//
// Revision 1.2  2001/05/11 12:21:18  slicerdw
// Commented old Location support ( ADF ) With the ML/ETE Compatible one
//
// Revision 1.1.1.1  2001/05/06 17:25:16  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#include "g_local.h"
#include "m_player.h"

//AQ2:TNG - Slicer Old Location support
//loccube_t *setcube = NULL;
//AQ2:TNG End

//
void _Cmd_Rules_f (edict_t * self, char *argument)
{
	char section[32], mbuf[1024], *p, buf[30][INI_STR_LEN];
	int i, j = 0;
	ini_t ini;

	strcpy (mbuf, "\n");
	if (*argument)
		Q_strncpyz(section, argument, sizeof(section));
	else
		strcpy (section, "main");

	if (OpenIniFile (GAMEVERSION "/prules.ini", &ini))
	{
		i = ReadIniSection (&ini, section, buf, 30);
		while (j < i)
		{
			p = buf[j++];
			if (*p == '.')
				p++;
			Q_strncatz(mbuf, p, sizeof(mbuf));
			Q_strncatz(mbuf, "\n", sizeof(mbuf));
		}
		CloseIniFile (&ini);
	}
	if (!j)
		gi.cprintf (self, PRINT_MEDIUM, "No rules on %s available\n", section);
	else
		gi.cprintf (self, PRINT_MEDIUM, "%s", mbuf);
}

void Cmd_Rules_f (edict_t * self)
{
	char *s;

	s = gi.args ();
	_Cmd_Rules_f (self, s);
}

//
void Cmd_Menu_f (edict_t * self)
{
	char *s;

	s = gi.args ();
	vShowMenu (self, s);
}

//
void Cmd_Punch_f (edict_t * self)
{
	if (!use_punch->value || !IS_ALIVE(self) || self->client->resp.sniper_mode != SNIPER_1X)
		return;

	if (self->client->weaponstate != WEAPON_READY && self->client->weaponstate != WEAPON_END_MAG)
		return;

	// animation moved to punch_attack() in a_xgame.c
	// punch_attack is now called in ClientThink after evaluation punch_desired
	// for "no punch when firing" stuff - TempFile
	if (level.framenum > self->client->punch_framenum + PUNCH_DELAY) {
		self->client->punch_framenum = level.framenum;	// you aren't Bruce Lee! :)
		self->client->punch_desired = true;
	}
}

/*
//Adds a point with name to location file - cheats must be enabled!
void
Cmd_Addpoint_f (edict_t * self)
{
  gi.cprintf (self, PRINT_MEDIUM,
	      "\nLocation point feature was dropped in 1.20 and\n"
	      "replaced by location area cubes.\nSee readme.txt for details.\n");
  //FILE *pntlist;
     char *s, buf[1024];

     s = gi.args();
     if (!*s)  
     { 
     gi.cprintf(self, PRINT_MEDIUM, "\nCommand needs argument, use addpoint <description>.\n");
     return;
     }
     sprintf(buf, "%s/maps/%s%s", GAMEVERSION, level.mapname, PG_LOCEXT);
     pntlist = fopen(buf, "a");
     if (pntlist == NULL)
     {
     gi.cprintf(self, PRINT_MEDIUM, "\nError accessing loc file %s.\n", buf);
     return;
     }
     sprintf(buf, "%.2f %.2f %.2f %s\n", self->s.origin[0], self->s.origin[1], self->s.origin[2], s);
     fputs(buf, pntlist);
     fclose(pntlist);
     gi.cprintf(self, PRINT_MEDIUM, "\nPoint added.\n"); 
}
*/

//Plays a sound file
void Cmd_Voice_f (edict_t * self)
{
	char *s;
	char fullpath[MAX_QPATH];

	if (!use_voice->value)
		return;

	s = gi.args ();
	//check if no sound is given
	if (!*s)
	{
		gi.cprintf (self, PRINT_MEDIUM,
			"\nCommand needs argument, use voice <soundfile.wav>.\n");
		return;
	}
	if (strlen (s) > 32)
	{
		gi.cprintf (self, PRINT_MEDIUM,
			"\nArgument is too long. Maximum length is 32 characters.\n");
		return;
	}
	// AQ2:TNG Disabled this message: why? -M
	if (strstr (s, ".."))
	{
		gi.cprintf (self, PRINT_MEDIUM,
			"\nArgument must not contain \"..\".\n");
		return;
	}
	
	//check if player is dead
	if (!IS_ALIVE(self))
		return;

	strcpy(fullpath, PG_SNDPATH);
	strcat(fullpath, s);
	// SLIC2 Taking this out.
	/*if (radio_repeat->value)
	{
	if ((d = CheckForRepeat (self, s)) == false)
	return;
	}*/
	if (radio_max->value)
	{
		if (CheckForFlood (self)== false)
			return;
	}
	// AQ2:TNG Deathwatch - This should be IDLE not NORM
	gi.sound (self, CHAN_VOICE, gi.soundindex (fullpath), 1, ATTN_IDLE, 0);
	// AQ2:TNG END
}

//AQ2:TNG SLicer - Old location support
// TempFile - BEGIN
/*
void
Cmd_BeginCube_f (edict_t * self)
{
  if (setcube)
    {
      gi.cprintf (self, PRINT_MEDIUM, "There is already a cube allocated at 0x%p.\n",
		  (void *) setcube);
      return;
    }

  setcube = gi.TagMalloc (sizeof (loccube_t), TAG_GAME);
  if (!setcube)
    return;

  gi.cprintf (self, PRINT_MEDIUM, "New cube successfully allocated at 0x%p.\n", (void *) setcube);
  gi.cprintf (self, PRINT_MEDIUM, "Please set lower left and upper right corner.\n");
}

void
Cmd_SetCubeLL_f (edict_t * self)
{
  if (!setcube)
    {
      gi.cprintf (self, PRINT_MEDIUM, "Please allocate a cube first by executing BeginCube.\n");
      return;
    }

  memcpy (setcube->lowerleft, self->s.origin, sizeof (vec3_t));
  gi.cprintf (self, PRINT_MEDIUM, "Lower left has been set to <%.2f %.2f %.2f>.\n",
       setcube->lowerleft[0], setcube->lowerleft[1], setcube->lowerleft[2]);
}

void
Cmd_SetCubeUR_f (edict_t * self)
{
  if (!setcube)
    {
      gi.cprintf (self, PRINT_MEDIUM, "Please allocate a cube first by executing BeginCube.\n");
      return;
    }

  memcpy (setcube->upperright, self->s.origin, sizeof (vec3_t));
  gi.cprintf (self, PRINT_HIGH, "Upper right has been set to <%.2f %.2f %.2f>.\n",
    setcube->upperright[0], setcube->upperright[1], setcube->upperright[2]);
}

void
Cmd_AbortCube_f (edict_t * self)
{
  if (!setcube)
    {
      gi.cprintf (self, PRINT_MEDIUM, "No cube to deallocate.\n");
      return;
    }

  gi.TagFree (setcube);
  gi.cprintf (self, PRINT_MEDIUM, "Cube at 0x%p successfully deallocated.\n",
	      (void *) setcube);
  setcube = NULL;
}

//Adds a cube with name to location file - cheats must be enabled!
void
Cmd_AddCube_f (edict_t * self)
{
  FILE *pntlist;
  char *s, buf[1024];

  if (!setcube)
    {
      gi.cprintf (self, PRINT_MEDIUM, "\nPlease allocate a cube first by executing BeginCube.\n");
      return;
    }

  if (!setcube->lowerleft[0] || !setcube->lowerleft[1] || !setcube->lowerleft[2] ||
      !setcube->upperright[0] || !setcube->upperright[1] || !setcube->upperright[2])
    {
      gi.cprintf (self, PRINT_MEDIUM, "\nPlease set cube corners first using SetCubeLL and SetCubeUR.\n");
      return;
    }

  FixCubeData (setcube);
  s = gi.args ();
  strcpy (setcube->desc, s);

  if (!*s)
    {
      gi.cprintf (self, PRINT_MEDIUM, "\nCommand needs argument, use addcube <description>.\n");
      return;
    }
  sprintf (buf, "%s/location/%s%s", GAMEVERSION, level.mapname, PG_LOCEXTEX);
  pntlist = fopen (buf, "a");
  if (pntlist == NULL)
    {
      gi.cprintf (self, PRINT_MEDIUM, "\nError accessing adf file %s.\n", buf);
      return;
    }

  sprintf (buf, "<%.2f %.2f %.2f> <%.2f %.2f %.2f> %s\n",
	setcube->lowerleft[0], setcube->lowerleft[1], setcube->lowerleft[2],
	   setcube->upperright[0], setcube->upperright[1], setcube->upperright[2], setcube->desc);
  fputs (buf, pntlist);
  fclose (pntlist);
  memcpy (&mapdescex[num_loccubes], setcube, sizeof (*setcube));
  num_loccubes++;

  gi.TagFree (setcube);
  setcube = NULL;

  gi.cprintf (self, PRINT_MEDIUM, "\nCube added.\n");
}

void
Cmd_PrintCubeState_f (edict_t * self)
{
  if (!setcube)
    {
      gi.cprintf (self, PRINT_MEDIUM, "\nPlease allocate a cube first by executing BeginCube.\n");
      return;
    }

  gi.cprintf (self, PRINT_MEDIUM, "\nTemporary cube allocated at %p.\nLower left corner: "
	      "<%.2f %.2f %.2f>\nUpper right corner: <%.2f %.2f %.2f>\n", (void *) setcube,
	setcube->lowerleft[0], setcube->lowerleft[1], setcube->lowerleft[2],
    setcube->upperright[1], setcube->upperright[2], setcube->upperright[2]);
}


// TempFile - END
*/
//AQ2:TNG END

// Show your location name, plus position and heading if cheats are enabled.
void Cmd_WhereAmI_f( edict_t * self )
{
	char location[ 128 ] = "";
	qboolean found = GetPlayerLocation( self, location );

	if( found )
		gi.cprintf( self, PRINT_MEDIUM, "Location: %s\n", location );
	else if( ! sv_cheats->value )
		gi.cprintf( self, PRINT_MEDIUM, "Location unknown.\n" );

	if( sv_cheats->value )
	{
		gi.cprintf( self, PRINT_MEDIUM, "Origin: %5.0f,%5.0f,%5.0f  Facing: %3.0f\n",
			self->s.origin[0], self->s.origin[1], self->s.origin[2], self->s.angles[1] );
	}
}

// Variables for new flags

static char flagpos1[64] = { 0 };
static char flagpos2[64] = { 0 };

//sets red flag position - cheats must be enabled!
void Cmd_SetFlag1_f (edict_t * self)
{
	Com_sprintf (flagpos1, sizeof(flagpos1), "<%.2f %.2f %.2f>", self->s.origin[0], self->s.origin[1],
		self->s.origin[2]);
	gi.cprintf (self, PRINT_MEDIUM, "\nRed Flag added at %s.\n", flagpos1);
}

//sets blue flag position - cheats must be enabled!
void Cmd_SetFlag2_f (edict_t * self)
{
	Com_sprintf (flagpos2, sizeof(flagpos2), "<%.2f %.2f %.2f>", self->s.origin[0], self->s.origin[1],
		self->s.origin[2]);
	gi.cprintf (self, PRINT_MEDIUM, "\nBlue Flag added at %s.\n", flagpos2);
}

//Save flag definition file - cheats must be enabled!
void Cmd_SaveFlags_f (edict_t * self)
{
	FILE *fp;
	char buf[128];

	if (!(flagpos1[0] && flagpos2[0]))
	{
		gi.cprintf (self, PRINT_MEDIUM,
			"You only can save flag positions when you've already set them\n");
		return;
	}

	sprintf (buf, "%s/tng/%s.flg", GAMEVERSION, level.mapname);
	fp = fopen (buf, "w");
	if (fp == NULL)
	{
		gi.cprintf (self, PRINT_MEDIUM, "\nError accessing flg file %s.\n", buf);
		return;
	}
	sprintf (buf, "# %s\n", level.mapname);
	fputs (buf, fp);
	sprintf (buf, "%s\n", flagpos1);
	fputs (buf, fp);
	sprintf (buf, "%s\n", flagpos2);
	fputs (buf, fp);
	fclose (fp);

	flagpos1[0] = 0;
	flagpos2[0] = 0;

	gi.cprintf (self, PRINT_MEDIUM, "\nFlag File saved.\n");
}
//SLIC2
/*
void Cmd_FlashGrenade_f(edict_t *ent)
{
  if (ent->client->grenadeType == GRENADE_NORMAL) {
    gi.cprintf(ent, PRINT_HIGH, "Flash grenades selected.\n");
    ent->client->grenadeType = GRENADE_FLASH;
  } else {
    gi.cprintf(ent, PRINT_HIGH, "Standard grenades selected.\n");
    ent->client->grenadeType = GRENADE_NORMAL;
  }
}*/
