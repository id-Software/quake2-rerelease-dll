//-----------------------------------------------------------------------------
// PG BUND
// a_xcmds.h
//
// header file for a_xcmd.c
//
// $Id: a_xcmds.h,v 1.4 2001/11/03 17:21:57 deathwatch Exp $
//
//-----------------------------------------------------------------------------
// $Log: a_xcmds.h,v $
// Revision 1.4  2001/11/03 17:21:57  deathwatch
// Fixed something in the time command, removed the .. message from the voice command, fixed the vote spamming with mapvote, removed addpoint command (old pb command that wasnt being used). Some cleaning up of the source at a few points.
//
// Revision 1.3  2001/05/14 12:26:12  slicerdw
// Uncommented the lens command
//
// Revision 1.2  2001/05/11 12:21:18  slicerdw
// Commented old Location support ( ADF ) With the ML/ETE Compatible one
//
// Revision 1.1.1.1  2001/05/06 17:25:16  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

//needed for Cmd_voice_f
#define PG_SNDPATH "user/"

//needed for Cmd_Addpoint_f
#define PG_LOCEXT ".pg"
#define PG_LOCEXTEX ".adf"

//
void Cmd_Menu_f (edict_t * self);
//
void Cmd_Punch_f (edict_t * self);

//Adds a point with name to location file - cheats must be enabled!
//void Cmd_Addpoint_f (edict_t * self);

void Cmd_WhereAmI_f( edict_t * self );

//Plays a sound file
void Cmd_Voice_f (edict_t * self);

//Shows new rules
void Cmd_Rules_f (edict_t * self);
void _Cmd_Rules_f (edict_t * self, char *argument);

//AQ2:TNG - Slicer Old Location support
/*
//TempFile - BEGIN
// new commands for area cubes
void Cmd_BeginCube_f (edict_t *);
void Cmd_SetCubeLL_f (edict_t *);
void Cmd_SetCubeUR_f (edict_t *);
void Cmd_PrintCubeState_f (edict_t *);
void Cmd_AddCube_f (edict_t *);
void Cmd_AbortCube_f (edict_t *);
//TempFile - END
*/
//AQ2:TNG END

void Cmd_Lens_f (edict_t *);
