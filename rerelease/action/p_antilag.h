#pragma once

#define ANTILAG_MASK		63
#define ANTILAG_MAX			64
#define ANTILAG_REWINDCAP	0.25

typedef struct antilag_s
{
	int		seek;
	int		rewound;
	float	curr_timestamp;

	float	hist_timestamp[ANTILAG_MAX];
	vec3_t	hist_origin[ANTILAG_MAX];
	vec3_t	hist_mins[ANTILAG_MAX];
	vec3_t	hist_maxs[ANTILAG_MAX];

	vec3_t	hold_origin;
	vec3_t	hold_mins;
	vec3_t	hold_maxs;
} antilag_t;

extern cvar_t *sv_antilag;
extern cvar_t *sv_antilag_interp;
void antilag_update(edict_t *ent);
void antilag_rewind_all(edict_t *ent);
void antilag_unmove_all(void);




