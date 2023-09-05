#define IS_CAPTAIN(ent) (teams[(ent)->client->resp.team].captain == (ent))
#define	HAVE_CAPTAIN(teamNum) (teams[(teamNum)].captain)

void SendScores (void);
bool TeamsReady( void );
void MM_LeftTeam( edict_t * ent );
void Cmd_Captain_f (edict_t * ent);
void Cmd_Ready_f (edict_t * ent);
void Cmd_Sub_f (edict_t * ent);
void Cmd_Teamname_f (edict_t * ent);
void Cmd_Teamskin_f (edict_t * ent);
void Cmd_TeamLock_f (edict_t * ent, int a_switch);
int CheckForCaptains (int cteam);

void Cmd_SetAdmin_f (edict_t * ent);
void Cmd_TogglePause_f(edict_t * ent, bool pause);
void Cmd_ResetScores_f(edict_t * ent);
