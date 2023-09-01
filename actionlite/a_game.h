// AQ2:TNG Deathwatch - Updated the Version variables to show TNG Stuff
#ifndef VERSION
#define VERSION "0.1"
#endif
#define TNG_TITLE "AQ2: The Next Generation Plus"
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
bool loc_CanSee (edict_t *, edict_t *);
void ParseSayText (edict_t *, char *, size_t size);

void AttachToEntity( edict_t *self, edict_t *onto );
bool CanBeAttachedTo( const edict_t *ent );

//PG BUND - BEGIN
//void ParseSayText(edict_t *, char *);
void GetWeaponName (edict_t * ent, char *buf);
void GetItemName (edict_t * ent, char *buf);
void GetHealth (edict_t * ent, char *buf);
void GetAmmo (edict_t * ent, char *buf);
void GetNearbyTeammates (edict_t * self, char *buf);

void ResetScores (bool playerScores);
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
