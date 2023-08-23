extern cvar_t *dom;

int DomFlagOwner( edict_t *flag );
qboolean DomCheckRules( void );
void DomRemember( const edict_t *ent, const gitem_t *item );
qboolean DomLoadConfig( const char *mapname );
void DomSetupStatusbar( void );
void SetDomStats( edict_t *ent );
