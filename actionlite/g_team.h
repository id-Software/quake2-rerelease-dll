extern cvar_t *competition;
extern cvar_t *matchlock;
extern cvar_t *electpercentage;
extern cvar_t *matchtime;
extern cvar_t *matchsetuptime;
extern cvar_t *matchstarttime;
extern cvar_t *admin_password;
extern cvar_t *allow_admin;
extern cvar_t *warp_list;
extern cvar_t *warn_unbalanced;


// Prototypes
bool G_TeamplayEnabled();
edict_t *loc_findradius(edict_t *from, const vec3_t &org, float rad);
void loc_buildboxpoints(vec3_t (&p)[8], const vec3_t &org, const vec3_t &mins, const vec3_t &maxs);
bool loc_CanSee(edict_t *targ, edict_t *inflictor);