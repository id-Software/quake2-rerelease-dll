//-----------------------------------------------------------------------------
// Include for base Action game-related things
//
// $Id: a_game.h,v 1.24 2004/09/23 00:09:44 slicerdw Exp $
//
//-----------------------------------------------------------------------------
// $Log: a_game.h,v $
// Revision 1.24  2004/09/23 00:09:44  slicerdw
// Radio kill count was missing for falling death
//
// Revision 1.23  2004/04/08 23:19:51  slicerdw
// Optimized some code, added a couple of features and fixed minor bugs
//
// Revision 1.22  2003/08/31 13:14:52  slicerdw
// changed version from 3.00 to 2.8
//
// Revision 1.21  2003/06/16 18:15:23  igor
// changed the version to 2.8
//
// Revision 1.20  2003/06/15 15:34:32  igor
// - removed the zcam code from this branch (see other branch)
// - added fixes from 2.72 (source only) version
// - resetted version number to 2.72
// - This version should be exactly like the release 2.72 - just with a few
//   more fixes (which whoever did the source only variant didn't get because
//   he didn't use the CVS as he should. Shame on him.
//
// Revision 1.19  2002/09/04 11:23:09  ra
// Added zcam to TNG and bumped version to 3.0
//
// Revision 1.18  2002/04/03 09:00:35  ra
// Bumped version to 2.71
//
// Revision 1.17  2002/03/26 21:50:32  ra
// Bumped version to 2.7
//
// Revision 1.16  2002/02/18 18:25:51  ra
// Bumped version to 2.6, fixed ctf falling and kicking of players in ctf
// uvtime
//
// Revision 1.15  2002/01/24 03:00:05  ra
// Made version 2.6beta
//
// Revision 1.14  2001/12/23 16:30:50  ra
// 2.5 ready. New stats from Freud. HC and shotgun gibbing seperated.
//
// Revision 1.13  2001/09/28 14:20:25  slicerdw
// Few tweaks..
//
// Revision 1.12  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.11  2001/09/02 20:33:34  deathwatch
// Added use_classic and fixed an issue with ff_afterround, also updated version
// nr and cleaned up some commands.
//
// Updated the VC Project to output the release build correctly.
//
// Revision 1.10  2001/08/15 14:50:48  slicerdw
// Added Flood protections to Radio & Voice, Fixed the sniper bug AGAIN
//
// Revision 1.9  2001/08/08 12:54:02  ra
// Increasing version number to 2.0b2
//
// Revision 1.8  2001/07/15 20:54:20  slicerdw
// Added a function to clean bodies "cleanbodies" and a "entcount" for test porpuses
//
// Revision 1.7  2001/06/21 00:05:30  slicerdw
// New Video Check System done -  might need some revision but works..
//
// Revision 1.4  2001/06/19 18:56:38  deathwatch
// New Last killed target system
//
// Revision 1.3  2001/06/01 19:18:42  slicerdw
// Added Matchmode Code
//
// Revision 1.2  2001/05/12 17:36:33  deathwatch
// Edited the version variables and updated the menus. Added variables:
// ACTION_VERSION, TNG_VERSION and TNG_VERSION2
//
// Revision 1.1.1.1  2001/05/06 17:24:24  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------


// AQ2:TNG Deathwatch - Updated the Version variables to show TNG Stuff
#ifndef VERSION
#define VERSION "2.82 git"
#endif
#define TNG_TITLE "AQ2: The Next Generation"
// AQ2:TNG Deathwatch End
//AQ2:TNG Slicer This is the max players writen on last killed target
//SLIC2
#define MAX_LAST_KILLED 8
//AQ2:TNG END

extern char *map_rotation[];
extern int num_maps, cur_map, rand_map, num_allvotes;	// num_allvotes added by Igor[Rock]

void ReadConfigFile ();
void ReadMOTDFile ();
void PrintMOTD (edict_t *ent);
void stuffcmd (edict_t *ent, char *s);
void unicastSound(edict_t *ent, int soundIndex, float volume);

int KickDoor (trace_t * tr_old, edict_t * ent, vec3_t forward);

// Prototypes of base Q2 functions that weren't included in any Q2 header
qboolean loc_CanSee (edict_t *, edict_t *);
void ParseSayText (edict_t *, char *, size_t size);

void AttachToEntity( edict_t *self, edict_t *onto );
qboolean CanBeAttachedTo( const edict_t *ent );

//PG BUND - BEGIN
//void ParseSayText(edict_t *, char *);
void GetWeaponName (edict_t * ent, char *buf);
void GetItemName (edict_t * ent, char *buf);
void GetHealth (edict_t * ent, char *buf);
void GetAmmo (edict_t * ent, char *buf);
void GetNearbyTeammates (edict_t * self, char *buf);

void ResetScores (qboolean playerScores);
void AddKilledPlayer (edict_t * self, edict_t * ent);
void VideoCheckClient (edict_t * ent);
//AQ2:TNG END
//TempFile
void GetLastLoss (edict_t * self, char *buf, char team);

// Firing styles (where shots originate from)
#define ACTION_FIRING_CENTER		0
#define ACTION_FIRING_CLASSIC		1
#define ACTION_FIRING_CLASSIC_HIGH	2

// maxs[2] of a player when crouching (we modify it from the normal 4)
// ...also the modified viewheight -FB 7/18/99
#define CROUCHING_MAXS2                 16
#define CROUCHING_VIEWHEIGHT		8
#define STANDING_VIEWHEIGHT			22

//a_team.c
void MakeAllLivePlayersObservers( void );

//a_cmds.c
void Cmd_NextMap_f( edict_t * ent );
