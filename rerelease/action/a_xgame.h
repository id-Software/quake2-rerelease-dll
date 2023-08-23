//-----------------------------------------------------------------------------
// PG BUND
// a_xgame.h
//
// header file for a_xgame.c
//
// $Id: a_xgame.h,v 1.9 2002/02/18 13:55:35 freud Exp $
//
//-----------------------------------------------------------------------------
// $Log: a_xgame.h,v $
// Revision 1.9  2002/02/18 13:55:35  freud
// Added last damaged players %P
//
// Revision 1.8  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.7  2001/06/25 11:44:47  slicerdw
// New Video Check System - video_check and video_check_lockpvs no longer latched
//
// Revision 1.6  2001/06/21 00:05:30  slicerdw
// New Video Check System done -  might need some revision but works..
//
// Revision 1.5  2001/05/31 16:58:14  igor_rock
// conflicts resolved
//
// Revision 1.4.2.1  2001/05/31 06:47:51  igor_rock
// - removed crash bug with non exisitng flag files
// - added new commands "setflag1", "setflag2" and "saveflags" to create
//   .flg files
//
// Revision 1.4  2001/05/11 12:21:18  slicerdw
// Commented old Location support ( ADF ) With the ML/ETE Compatible one
//
// Revision 1.2  2001/05/07 21:18:34  slicerdw
// Added Video Checking System
//
// Revision 1.1.1.1  2001/05/06 17:25:24  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

//relative position directions
#define RP_NORTH 1
#define RP_SOUTH 2
#define RP_EAST  4
#define RP_WEST  8

//TempFile punch delay
#define PUNCH_DELAY	HZ/2	// 5 frames, that's .5 seconds

//maximum size for location description
#define LOC_STR_LEN 128
//maximum amount of location points on a map
#define LOC_MAX_POINTS 300

qboolean GetPlayerLocation( edict_t *self, char *buf );

void ParseSayText(edict_t *ent, char *text, size_t size);

void Cmd_SetFlag1_f(edict_t *self);
void Cmd_SetFlag2_f(edict_t *self);
void Cmd_SaveFlags_f(edict_t *self);
