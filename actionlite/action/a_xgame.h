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

bool GetPlayerLocation( edict_t *self, char *buf );

void ParseSayText(edict_t *ent, char *text, size_t size);

void Cmd_SetFlag1_f(edict_t *self);
void Cmd_SetFlag2_f(edict_t *self);
void Cmd_SaveFlags_f(edict_t *self);
