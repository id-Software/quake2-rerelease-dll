// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// dm_ball.c
// pmack
// june 98

#include "../g_local.h"

// defines

[[maybe_unused]] constexpr spawnflags_t SPAWNFLAG_DBALL_GOAL_TEAM1 = 0x0001_spawnflag;
// unused; assumed by not being team1
// constexpr uint32_t SPAWNFLAG_DBALL_GOAL_TEAM2	= 0x0002;

// globals

edict_t *dball_ball_entity = nullptr;
int		 dball_ball_startpt_count;
int		 dball_team1_goalscore;
int		 dball_team2_goalscore;

cvar_t *dball_team1_skin;
cvar_t *dball_team2_skin;
cvar_t *goallimit;

// prototypes

void  EndDMLevel();
float PlayersRangeFromSpot(edict_t *spot);

void DBall_BallDie(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod);
void DBall_BallRespawn(edict_t *self);

// **************************
// Game rules
// **************************

int DBall_CheckDMRules()
{
	if (goallimit->integer)
	{
		if (dball_team1_goalscore >= goallimit->integer)
			gi.LocBroadcast_Print(PRINT_HIGH, "Team 1 Wins.\n");
		else if (dball_team2_goalscore >= goallimit->integer)
			gi.LocBroadcast_Print(PRINT_HIGH, "Team 2 Wins.\n");
		else
			return 0;

		EndDMLevel();
		return 1;
	}

	return 0;
}

//==================
//==================
void DBall_ClientBegin(edict_t *ent)
{
#if 0
	uint32_t 	 team1, team2, unassigned;
	edict_t	    *other;
	char		*p;
	static char  value[512];

	team1 = 0;
	team2 = 0;
	unassigned = 0;

	for (uint32_t j = 1; j <= game.maxclients; j++)
	{
		other = &g_edicts[j];
		if (!other->inuse)
			continue;
		if (!other->client)
			continue;
		if (other == ent) // don't count the new player
			continue;

		Q_strlcpy(value, Info_ValueForKey(other->client->pers.userinfo, "skin"), sizeof(value));
		p = strchr(value, '/');
		if (p)
		{
			if (!strcmp(dball_team1_skin->string, value))
				team1++;
			else if (!strcmp(dball_team2_skin->string, value))
				team2++;
			else
				unassigned++;
		}
		else
			unassigned++;
	}

	if (team1 > team2)
	{
		gi.Com_Print("assigned to team 2\n");
		Info_SetValueForKey(ent->client->pers.userinfo, "skin", dball_team2_skin->string);
	}
	else
	{
		gi.Com_Print("assigned to team 1\n");
		Info_SetValueForKey(ent->client->pers.userinfo, "skin", dball_team1_skin->string);
	}

	ClientUserinfoChanged(ent, ent->client->pers.userinfo);

	if (unassigned)
		gi.Com_PrintFmt("{} unassigned players present!\n", unassigned);
#endif
}

//==================
//==================
bool DBall_SelectSpawnPoint(edict_t *ent, vec3_t &origin, vec3_t &angles, bool force_spawn)
{
#if 0
	edict_t	*bestspot;
	float		bestdistance, bestplayerdistance;
	edict_t	*spot;
	const char *spottype;
	static char skin[512];

	Q_strlcpy(skin, Info_ValueForKey(ent->client->pers.userinfo, "skin"), sizeof(skin));
	if (!strcmp(dball_team1_skin->string, skin))
		spottype = "dm_dball_team1_start";
	else if (!strcmp(dball_team2_skin->string, skin))
		spottype = "dm_dball_team2_start";
	else
		spottype = "info_player_deathmatch";

	spot = nullptr;
	bestspot = nullptr;
	bestdistance = 0;
	while ((spot = G_FindByString<&edict_t::classname>(spot, spottype)) != nullptr)
	{
		bestplayerdistance = PlayersRangeFromSpot(spot);

		if (bestplayerdistance > bestdistance)
		{
			bestspot = spot;
			bestdistance = bestplayerdistance;
		}
	}

	if (bestspot)
	{
		origin = bestspot->s.origin;
		origin[2] += 9;
		angles = bestspot->s.angles;
		return true;
	}

	// if we didn't find an appropriate spawnpoint, just
	// call the standard one.
#endif
	bool lm = false;
	return SelectSpawnPoint(ent, origin, angles, force_spawn, lm);
}

//==================
//==================
void DBall_GameInit()
{
	// we don't want a minimum speed for friction to take effect.
	// this will allow any knockback to move stuff.
	gi.cvar_forceset("sv_stopspeed", "0");
	dball_team1_goalscore = 0;
	dball_team2_goalscore = 0;
	
	gi.cvar_forceset(g_no_mines->name, "1");
	gi.cvar_forceset(g_no_nukes->name, "1");
	gi.cvar_forceset(g_dm_no_stack_double->name, "1");
	gi.cvar_forceset(g_friendly_fire->name, "0");
	//gi.cvar_forceset(g_no_mines->name, "1"); note: skin teams gone...

	dball_team1_skin = gi.cvar("dball_team1_skin", "male/ctf_r", CVAR_NOFLAGS);
	dball_team2_skin = gi.cvar("dball_team2_skin", "male/ctf_b", CVAR_NOFLAGS);
	goallimit = gi.cvar("goallimit", "0", CVAR_NOFLAGS);
}

//==================
//==================
void DBall_PostInitSetup()
{
	edict_t *e;

	e = nullptr;
	// turn teleporter destinations nonsolid.
	while ((e = G_FindByString<&edict_t::classname>(e, "misc_teleporter_dest")))
	{
		e->solid = SOLID_NOT;
		gi.linkentity(e);
	}

	// count the ball start points
	dball_ball_startpt_count = 0;
	e = nullptr;
	while ((e = G_FindByString<&edict_t::classname>(e, "dm_dball_ball_start")))
	{
		dball_ball_startpt_count++;
	}

	if (dball_ball_startpt_count == 0)
		gi.Com_Print("No Deathball start points!\n");
}

//==================
// DBall_ChangeDamage - half damage between players. full if it involves
//		the ball entity
//==================
int DBall_ChangeDamage(edict_t *targ, edict_t *attacker, int damage, mod_t mod)
{
	// cut player -> ball damage to 1
	if (targ == dball_ball_entity)
		return 1;

	// damage player -> player is halved
	if (attacker != dball_ball_entity)
		return damage / 2;

	return damage;
}

//==================
//==================
int DBall_ChangeKnockback(edict_t *targ, edict_t *attacker, int knockback, mod_t mod)
{
	if (targ != dball_ball_entity)
		return knockback;

	if (knockback < 1)
	{
		// FIXME - these don't account for quad/double
		if (mod.id == MOD_ROCKET) // rocket
			knockback = 70;
		else if (mod.id == MOD_BFG_EFFECT) // bfg
			knockback = 90;
		else
			gi.Com_PrintFmt("zero knockback, mod {}\n", (int32_t) mod.id);
	}
	else
	{
		// FIXME - change this to an array?
		switch (mod.id)
		{
		case MOD_BLASTER:
			knockback *= 3;
			break;
		case MOD_SHOTGUN:
			knockback = (knockback * 3) / 8;
			break;
		case MOD_SSHOTGUN:
			knockback = knockback / 3;
			break;
		case MOD_MACHINEGUN:
			knockback = (knockback * 3) / 2;
			break;
		case MOD_HYPERBLASTER:
			knockback *= 4;
			break;
		case MOD_GRENADE:
		case MOD_HANDGRENADE:
		case MOD_PROX:
		case MOD_G_SPLASH:
		case MOD_HG_SPLASH:
		case MOD_HELD_GRENADE:
		case MOD_TRACKER:
		case MOD_DISINTEGRATOR:
			knockback /= 2;
			break;
		case MOD_R_SPLASH:
			knockback = (knockback * 3) / 2;
			break;
		case MOD_RAILGUN:
		case MOD_HEATBEAM:
			knockback /= 3;
			break;
		default:
			break;
		}
	}

	//	gi.dprintf("mod: %d    knockback: %d\n", mod, knockback);
	return knockback;
}

// **************************
// Goals
// **************************

TOUCH(DBall_GoalTouch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
#if 0
    static char value[512];
	int		 team_score;
	int		 scorechange;
	char	 *p;
	edict_t *ent;

	if (other != dball_ball_entity)
		return;

	self->health = self->max_health;

	// determine which team scored, and bump the team score
	if (self->spawnflags.has(SPAWNFLAG_DBALL_GOAL_TEAM1))
	{
		dball_team1_goalscore += (int) self->wait;
		team_score = 1;
	}
	else
	{
		dball_team2_goalscore += (int) self->wait;
		team_score = 2;
	}

	// bump the score for everyone on the correct team.
	for (uint32_t j = 1; j <= game.maxclients; j++)
	{
		ent = &g_edicts[j];
		if (!ent->inuse)
			continue;
		if (!ent->client)
			continue;

		if (ent == other->enemy)
			scorechange = (int) self->wait + 5;
		else
			scorechange = (int) self->wait;

		Q_strlcpy(value, Info_ValueForKey(ent->client->pers.userinfo, "skin"), sizeof(value));
		p = strchr(value, '/');
		if (p)
		{
			if (!strcmp(dball_team1_skin->string, value))
			{
				if (team_score == 1)
					ent->client->resp.score += scorechange;
				else if (other->enemy == ent)
					ent->client->resp.score -= scorechange;
			}
			else if (!strcmp(dball_team2_skin->string, value))
			{
				if (team_score == 2)
					ent->client->resp.score += scorechange;
				else if (other->enemy == ent)
					ent->client->resp.score -= scorechange;
			}
			else
				gi.Com_Print("unassigned player!!!!\n");
		}
	}

	if (other->enemy)
		gi.Com_PrintFmt("score for team {} by {}\n", team_score, other->enemy->client->pers.netname);
	else
		gi.Com_PrintFmt("score for team {} by someone\n", team_score);

	DBall_BallDie(other, other->enemy, other->enemy, 0, vec3_origin, MOD_SUICIDE);

	G_UseTargets(self, other);
#endif
}

// **************************
// Ball
// **************************

edict_t *PickBallStart(edict_t *ent)
{
	int		 which, current;
	edict_t *e;

	which = irandom(dball_ball_startpt_count);
	e = nullptr;
	current = 0;

	while ((e = G_FindByString<&edict_t::classname>(e, "dm_dball_ball_start")))
	{
		current++;
		if (current == which)
			return e;
	}

	if (current == 0)
		gi.Com_Print("No ball start points found!\n");

	return G_FindByString<&edict_t::classname>(nullptr, "dm_dball_ball_start");
}

//==================
// DBall_BallTouch - if the ball hit another player, hurt them
//==================
TOUCH(DBall_BallTouch) (edict_t *ent, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	vec3_t dir;
	float  dot;
	float  speed;

	if (other->takedamage == false)
		return;

	// hit a player
	if (other->client)
	{
		if (ent->velocity[0] || ent->velocity[1] || ent->velocity[2])
		{
			speed = ent->velocity.length();

			dir = ent->s.origin - other->s.origin;
			dot = dir.dot(ent->velocity);

			if (dot > 0.7f)
			{
				T_Damage(other, ent, ent, vec3_origin, ent->s.origin, vec3_origin,
						 (int) (speed / 10), (int) (speed / 10), DAMAGE_NONE, MOD_DBALL_CRUSH);
			}
		}
	}
}

//==================
// DBall_BallPain
//==================
PAIN(DBall_BallPain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	self->enemy = other;
	self->health = self->max_health;
	//	if(other->classname)
	//		gi.Com_PrintFmt("hurt by {} -- {}\n", other->classname, self->health);
}

DIE(DBall_BallDie) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	// do the splash effect
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_DBALL_GOAL);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);

	self->s.angles = {};
	self->velocity = {};
	self->avelocity = {};

	// make it invisible and desolid until respawn time
	self->solid = SOLID_NOT;
	//	self->s.modelindex = 0;
	self->think = DBall_BallRespawn;
	self->nextthink = level.time + 2_sec;
	gi.linkentity(self);
}

THINK(DBall_BallRespawn) (edict_t *self) -> void
{
	edict_t *start;

	// do the splash effect
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_DBALL_GOAL);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);

	// move the ball and stop it
	start = PickBallStart(self);
	if (start)
	{
		self->s.origin = start->s.origin;
		self->s.old_origin = start->s.origin;
	}

	self->s.angles = {};
	self->velocity = {};
	self->avelocity = {};

	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/objects/dball/tris.md2");
	self->s.event = EV_PLAYER_TELEPORT;
	self->groundentity = nullptr;

	gi.linkentity(self);

	// kill anything at the destination
	KillBox(self, false);
}

// ************************
// SPEED CHANGES
// ************************

constexpr spawnflags_t SPAWNFLAG_DBALL_SPEED_ONEWAY = 1_spawnflag;

TOUCH(DBall_SpeedTouch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	float  dot;
	vec3_t vel;

	if (other != dball_ball_entity)
		return;

	if (self->timestamp >= level.time)
		return;

	if (other->velocity.length() < 1)
		return;

	if (self->spawnflags.has(SPAWNFLAG_DBALL_SPEED_ONEWAY))
	{
		vel = other->velocity;
		vel.normalize();
		dot = vel.dot(self->movedir);
		if (dot < 0.8f)
			return;
	}

	self->timestamp = level.time + gtime_t::from_sec(self->delay);
	other->velocity *= self->speed;
}

// ************************
// SPAWN FUNCTIONS
// ************************

/*QUAKED dm_dball_ball (1 .5 .5) (-48 -48 -48) (48 48 48) ONEWAY
Deathball Ball
*/
void SP_dm_dball_ball(edict_t *self)
{
	if (!deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	if (gamerules->integer != RDM_DEATHBALL)
	{
		G_FreeEdict(self);
		return;
	}

	dball_ball_entity = self;
	//	dball_ball_startpt = self->s.origin;

	self->s.modelindex = gi.modelindex("models/objects/dball/tris.md2");
	self->mins = { -32, -32, -32 };
	self->maxs = { 32, 32, 32 };
	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_NEWTOSS;
	self->clipmask = MASK_MONSTERSOLID;

	self->takedamage = true;
	self->mass = 50;
	self->health = 50000;
	self->max_health = 50000;
	self->pain = DBall_BallPain;
	self->die = DBall_BallDie;
	self->touch = DBall_BallTouch;

	gi.linkentity(self);
}

/*QUAKED dm_dball_team1_start (1 .5 .5) (-16 -16 -24) (16 16 32)
Deathball team 1 start point
*/
void SP_dm_dball_team1_start(edict_t *self)
{
	if (!deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}
	if (gamerules->integer != RDM_DEATHBALL)
	{
		G_FreeEdict(self);
		return;
	}
}

/*QUAKED dm_dball_team2_start (1 .5 .5) (-16 -16 -24) (16 16 32)
Deathball team 2 start point
*/
void SP_dm_dball_team2_start(edict_t *self)
{
	if (!deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}
	if (gamerules->integer != RDM_DEATHBALL)
	{
		G_FreeEdict(self);
		return;
	}
}

/*QUAKED dm_dball_ball_start (1 .5 .5) (-48 -48 -48) (48 48 48)
Deathball ball start point
*/
void SP_dm_dball_ball_start(edict_t *self)
{
	if (!deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}
	if (gamerules->integer != RDM_DEATHBALL)
	{
		G_FreeEdict(self);
		return;
	}
}

/*QUAKED dm_dball_speed_change (1 .5 .5) ? ONEWAY
Deathball ball speed changing field.

speed: multiplier for speed (.5 = half, 2 = double, etc) (default = double)
angle: used with ONEWAY so speed change is only one way.
delay: time between speed changes (default: 0.2 sec)
*/
void SP_dm_dball_speed_change(edict_t *self)
{
	if (!deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}
	if (gamerules->integer != RDM_DEATHBALL)
	{
		G_FreeEdict(self);
		return;
	}

	if (!self->speed)
		self->speed = 2;

	if (!self->delay)
		self->delay = 0.2f;

	self->touch = DBall_SpeedTouch;
	self->solid = SOLID_TRIGGER;
	self->movetype = MOVETYPE_NONE;
	self->svflags |= SVF_NOCLIENT;

	if (self->s.angles)
		G_SetMovedir(self->s.angles, self->movedir);
	else
		self->movedir = { 1, 0, 0 };

	gi.setmodel(self, self->model);
	gi.linkentity(self);
}

/*QUAKED dm_dball_goal (1 .5 .5) ? TEAM1 TEAM2
Deathball goal

Team1/Team2 - beneficiary of this goal. when the ball enters this goal, the beneficiary team will score.

"wait": score to be given for this goal (default 10) player gets score+5.
*/
void SP_dm_dball_goal(edict_t *self)
{
	if (!deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	if (gamerules->integer != RDM_DEATHBALL)
	{
		G_FreeEdict(self);
		return;
	}

	if (!self->wait)
		self->wait = 10;

	self->touch = DBall_GoalTouch;
	self->solid = SOLID_TRIGGER;
	self->movetype = MOVETYPE_NONE;
	self->svflags |= SVF_NOCLIENT;

	if (self->s.angles)
		G_SetMovedir(self->s.angles, self->movedir);

	gi.setmodel(self, self->model);
	gi.linkentity(self);
}
