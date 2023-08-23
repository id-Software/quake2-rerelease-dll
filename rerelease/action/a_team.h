//-----------------------------------------------------------------------------
// Include for Action team-related things
//
// $Id: a_team.h,v 1.9 2002/04/01 14:00:08 freud Exp $
//
//-----------------------------------------------------------------------------
// $Log: a_team.h,v $
// Revision 1.9  2002/04/01 14:00:08  freud
// After extensive checking I think I have found the spawn bug in the new
// system.
//
// Revision 1.8  2002/03/28 11:46:03  freud
// stat_mode 2 and timelimit 0 did not show stats at end of round.
// Added lock/unlock.
// A fix for use_oldspawns 1, crash bug.
//
// Revision 1.7  2002/03/24 22:45:53  freud
// New spawn code again, bad commit last time..
//
// Revision 1.5  2001/11/27 23:23:40  igor_rock
// Bug fixed: day_cycle_at wasn't reset at mapchange
//
// Revision 1.4  2001/11/03 17:31:15  ra
// Compiler warning fix.
//
// Revision 1.3  2001/10/18 12:55:35  deathwatch
// Added roundtimeleft
//
// Revision 1.2  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.1.1.1  2001/05/06 17:24:27  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#define NOTEAM          0
#define TEAM1           1
#define TEAM2           2
#define TEAM3           3

#define MAX_TEAMS       3
#define TEAM_TOP        (MAX_TEAMS+1)

#define WINNER_NONE     NOTEAM
#define WINNER_TIE      TEAM_TOP

// Pre- and post-trace code for our teamplay anti-stick stuff.  If there are
// still "transparent" (SOLID_TRIGGER) players, they need to be set to
// SOLID_BBOX before a trace is performed, then changed back again
// afterwards.  PRETRACE() and POSTTRACE() should be called before and after
// traces in all places where combat is taking place (ie "transparent" players
// should be detected), ie shots being traced etc.  
// FB 6/1/99: Now crouching players will have their bounding box adjusted here
// too, for better shot areas. (there has to be a better way to do this?)

#define PRETRACE() \
        if (transparent_list && (((int)teamplay->value && !lights_camera_action) || jump->value)) \
                TransparentListSet(SOLID_BBOX)

#define POSTTRACE() \
        if (transparent_list && (((int)teamplay->value && !lights_camera_action) || jump->value)) \
                TransparentListSet(SOLID_TRIGGER)

edict_t *SelectTeamplaySpawnPoint (edict_t *);
qboolean FallingDamageAmnesty (edict_t * targ);
char * TeamName (int team);
void UpdateJoinMenu( void );
void OpenJoinMenu (edict_t *);
void OpenWeaponMenu (edict_t *);
void OpenItemMenu (edict_t * ent);
void JoinTeam (edict_t * ent, int desired_team, int skip_menuclose);
edict_t *FindOverlap (edict_t * ent, edict_t * last_overlap);
int CheckTeamRules (void);
void A_Scoreboard (edict_t * ent);
void Team_f (edict_t * ent);
void AssignSkin (edict_t * ent, const char *s, qboolean nickChanged);
void TallyEndOfLevelTeamScores (void);
void SetupTeamSpawnPoints (void);
int CheckTeamSpawnPoints (void);
void GetSpawnPoints (void);
void CleanBodies (void);		// from p_client.c, removes all current dead bodies from map

void LeaveTeam (edict_t *);
int newrand (int top);
void InitTransparentList (void);
void AddToTransparentList (edict_t *);
void RemoveFromTransparentList (edict_t *);
qboolean OnTransparentList( const edict_t *ent );
void PrintTransparentList (void);
void CenterPrintAll (const char *msg);
int TeamHasPlayers( int team );

//TNG:Freud - new spawning system
void NS_GetSpawnPoints (void);
qboolean NS_SelectFarTeamplaySpawnPoint (int team, qboolean teams_assigned[]);
void NS_SetupTeamSpawnPoints (void);

typedef struct spawn_distances_s
{
  float distance;
  edict_t *s;
}
spawn_distances_t;

typedef struct transparent_list_s
{
  edict_t *ent;
  struct transparent_list_s *next;
}
transparent_list_t;


extern qboolean team_game_going;
extern qboolean team_round_going;
extern int lights_camera_action;
extern int holding_on_tie_check;
extern int team_round_countdown;
extern int timewarning;
extern int fragwarning;
extern transparent_list_t *transparent_list;
extern trace_t trace_t_temp;
extern int current_round_length; // For RoundTimeLeft
extern int day_cycle_at;
extern int teamCount;
extern int in_warmup;
extern qboolean teams_changed;

typedef struct menu_list_weapon
{
  int num;
  char sound[40];
  char name[40];
}
menu_list_weapon;

typedef struct menu_list_item
{
  int num;
  char sound[40];
  char name[40];
}
menu_list_item;
