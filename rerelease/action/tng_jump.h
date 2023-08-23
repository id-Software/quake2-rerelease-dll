#define STAT_SPEEDX					1
#define STAT_HIGHSPEED					2
#define STAT_FALLDMGLAST				3

extern char *jump_statusbar;
extern cvar_t *jump;

void Jmp_EquipClient(edict_t *ent);
void Jmp_SetStats(edict_t *ent);

void Cmd_Jmod_f (edict_t *ent);
void Cmd_Clear_f (edict_t *ent);
void Cmd_Reset_f (edict_t *ent);
void Cmd_Store_f (edict_t *ent);
void Cmd_Recall_f (edict_t *ent);
