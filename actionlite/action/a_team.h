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
        if (transparent_list && (((int)teamplay->value && !lights_camera_action))) \
                TransparentListSet(SOLID_BBOX)

#define POSTTRACE() \
        if (transparent_list && (((int)teamplay->value && !lights_camera_action))) \
                TransparentListSet(SOLID_TRIGGER)

edict_t *SelectTeamplaySpawnPoint (edict_t *);
bool FallingDamageAmnesty (edict_t * targ);
char * TeamName (int team);
void UpdateJoinMenu( void );
void OpenJoinMenu (edict_t *);
void OpenWeaponMenu (edict_t *);
void OpenItemMenu (edict_t * ent);
void OpenItemKitMenu (edict_t * ent);
void JoinTeam (edict_t * ent, int desired_team, int skip_menuclose);
edict_t *FindOverlap (edict_t * ent, edict_t * last_overlap);
int CheckTeamRules (void);
void A_Scoreboard (edict_t * ent);
void Team_f (edict_t * ent);
void AssignSkin (edict_t * ent, const char *s, bool nickChanged);
void TallyEndOfLevelTeamScores (void);
void SetupTeamSpawnPoints ();
int CheckTeamSpawnPoints ();
void GetSpawnPoints ();
void CleanBodies ();		// from p_client.c, removes all current dead bodies from map

void LeaveTeam (edict_t *);
int newrand (int top);
void InitTransparentList ();
void AddToTransparentList (edict_t *);
void RemoveFromTransparentList (edict_t *);
bool OnTransparentList( const edict_t *ent );
void PrintTransparentList ();
void CenterPrintAll (const char *msg);
int TeamHasPlayers( int team );

//TNG:Freud - new spawning system
void NS_GetSpawnPoints ();
bool NS_SelectFarTeamplaySpawnPoint (int team, bool teams_assigned[]);
void NS_SetupTeamSpawnPoints ();

int OtherTeam(int teamNum);

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


extern bool team_game_going;
extern bool team_round_going;
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
extern bool teams_changed;

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
