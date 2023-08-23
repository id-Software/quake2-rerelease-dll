/* Stats Command */
void ResetStats(edict_t *ent);
void Stats_AddShot(edict_t *ent, int gun);
void Stats_AddHit(edict_t *ent, int gun, int hitPart);

void A_ScoreboardEndLevel (edict_t * ent, edict_t * killer);
void Cmd_Stats_f (edict_t *targetent, char *arg);
void Cmd_Statmode_f(edict_t *ent);
