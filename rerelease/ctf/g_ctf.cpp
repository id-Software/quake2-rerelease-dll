// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "../g_local.h"
#include "../m_player.h"

#include <assert.h>

enum match_t
{
	MATCH_NONE,
	MATCH_SETUP,
	MATCH_PREGAME,
	MATCH_GAME,
	MATCH_POST
};

enum elect_t
{
	ELECT_NONE,
	ELECT_MATCH,
	ELECT_ADMIN,
	ELECT_MAP
};

struct ctfgame_t
{
	int		team1, team2;
	int		total1, total2; // these are only set when going into intermission except in teamplay
	gtime_t last_flag_capture;
	int		last_capture_team;

	match_t match;	   // match state
	gtime_t matchtime; // time for match start/end (depends on state)
	int		lasttime;  // last time update, explicitly truncated to seconds
	bool	countdown; // has audio countdown started?

	elect_t	 election;	 // election type
	edict_t *etarget;	 // for admin election, who's being elected
	char	 elevel[32]; // for map election, target level
	int		 evotes;	 // votes so far
	int		 needvotes;	 // votes needed
	gtime_t	 electtime;	 // remaining time until election times out
	char	 emsg[256];	 // election name
	int		 warnactive; // true if stat string 30 is active

	ghost_t ghosts[MAX_CLIENTS]; // ghost codes
};

ctfgame_t ctfgame;

cvar_t *ctf;
cvar_t *teamplay;
cvar_t *g_teamplay_force_join;

// [Paril-KEX]
bool G_TeamplayEnabled()
{
	return ctf->integer || teamplay->integer;
}

// [Paril-KEX]
void G_AdjustTeamScore(ctfteam_t team, int32_t offset)
{
	if (team == CTF_TEAM1)
		ctfgame.total1 += offset;
	else if (team == CTF_TEAM2)
		ctfgame.total2 += offset;
}

cvar_t *competition;
cvar_t *matchlock;
cvar_t *electpercentage;
cvar_t *matchtime;
cvar_t *matchsetuptime;
cvar_t *matchstarttime;
cvar_t *admin_password;
cvar_t *allow_admin;
cvar_t *warp_list;
cvar_t *warn_unbalanced;

// Index for various CTF pics, this saves us from calling gi.imageindex
// all the time and saves a few CPU cycles since we don't have to do
// a bunch of string compares all the time.
// These are set in CTFPrecache() called from worldspawn
int imageindex_i_ctf1;
int imageindex_i_ctf2;
int imageindex_i_ctf1d;
int imageindex_i_ctf2d;
int imageindex_i_ctf1t;
int imageindex_i_ctf2t;
int imageindex_i_ctfj;
int imageindex_sbfctf1;
int imageindex_sbfctf2;
int imageindex_ctfsb1;
int imageindex_ctfsb2;
int modelindex_flag1, modelindex_flag2; // [Paril-KEX]

constexpr item_id_t tech_ids[] = { IT_TECH_RESISTANCE, IT_TECH_STRENGTH, IT_TECH_HASTE, IT_TECH_REGENERATION };

/*--------------------------------------------------------------------------*/

#ifndef KEX_Q2_GAME
/*
=================
findradius

Returns entities that have origins within a spherical area

findradius (origin, radius)
=================
*/
static edict_t *loc_findradius(edict_t *from, const vec3_t &org, float rad)
{
	vec3_t eorg;
	int	   j;

	if (!from)
		from = g_edicts;
	else
		from++;
	for (; from < &g_edicts[globals.num_edicts]; from++)
	{
		if (!from->inuse)
			continue;
		for (j = 0; j < 3; j++)
			eorg[j] = org[j] - (from->s.origin[j] + (from->mins[j] + from->maxs[j]) * 0.5f);
		if (eorg.length() > rad)
			continue;
		return from;
	}

	return nullptr;
}
#endif

static void loc_buildboxpoints(vec3_t (&p)[8], const vec3_t &org, const vec3_t &mins, const vec3_t &maxs)
{
	p[0] = org + mins;
	p[1] = p[0];
	p[1][0] -= mins[0];
	p[2] = p[0];
	p[2][1] -= mins[1];
	p[3] = p[0];
	p[3][0] -= mins[0];
	p[3][1] -= mins[1];
	p[4] = org + maxs;
	p[5] = p[4];
	p[5][0] -= maxs[0];
	p[6] = p[0];
	p[6][1] -= maxs[1];
	p[7] = p[0];
	p[7][0] -= maxs[0];
	p[7][1] -= maxs[1];
}

static bool loc_CanSee(edict_t *targ, edict_t *inflictor)
{
	trace_t trace;
	vec3_t	targpoints[8];
	int		i;
	vec3_t	viewpoint;

	// bmodels need special checking because their origin is 0,0,0
	if (targ->movetype == MOVETYPE_PUSH)
		return false; // bmodels not supported

	loc_buildboxpoints(targpoints, targ->s.origin, targ->mins, targ->maxs);

	viewpoint = inflictor->s.origin;
	viewpoint[2] += inflictor->viewheight;

	for (i = 0; i < 8; i++)
	{
		trace = gi.traceline(viewpoint, targpoints[i], inflictor, MASK_SOLID);
		if (trace.fraction == 1.0f)
			return true;
	}

	return false;
}
#

/*--------------------------------------------------------------------------*/

void CTFSpawn()
{
	memset(&ctfgame, 0, sizeof(ctfgame));
	CTFSetupTechSpawn();

	if (competition->integer > 1)
	{
		ctfgame.match = MATCH_SETUP;
		ctfgame.matchtime = level.time + gtime_t::from_min(matchsetuptime->value);
	}
}

void CTFInit()
{
	ctf = gi.cvar("ctf", "0", CVAR_SERVERINFO | CVAR_LATCH);
	competition = gi.cvar("competition", "0", CVAR_SERVERINFO);
	matchlock = gi.cvar("matchlock", "1", CVAR_SERVERINFO);
	electpercentage = gi.cvar("electpercentage", "66", CVAR_NOFLAGS);
	matchtime = gi.cvar("matchtime", "20", CVAR_SERVERINFO);
	matchsetuptime = gi.cvar("matchsetuptime", "10", CVAR_NOFLAGS);
	matchstarttime = gi.cvar("matchstarttime", "20", CVAR_NOFLAGS);
	admin_password = gi.cvar("admin_password", "", CVAR_NOFLAGS);
	allow_admin = gi.cvar("allow_admin", "1", CVAR_NOFLAGS);
	warp_list = gi.cvar("warp_list", "q2ctf1 q2ctf2 q2ctf3 q2ctf4 q2ctf5", CVAR_NOFLAGS);
	warn_unbalanced = gi.cvar("warn_unbalanced", "0", CVAR_NOFLAGS);
}

/*
 * Precache CTF items
 */

void CTFPrecache()
{
	imageindex_i_ctf1 = gi.imageindex("i_ctf1");
	imageindex_i_ctf2 = gi.imageindex("i_ctf2");
	imageindex_i_ctf1d = gi.imageindex("i_ctf1d");
	imageindex_i_ctf2d = gi.imageindex("i_ctf2d");
	imageindex_i_ctf1t = gi.imageindex("i_ctf1t");
	imageindex_i_ctf2t = gi.imageindex("i_ctf2t");
	imageindex_i_ctfj = gi.imageindex("i_ctfj");
	imageindex_sbfctf1 = gi.imageindex("sbfctf1");
	imageindex_sbfctf2 = gi.imageindex("sbfctf2");
	imageindex_ctfsb1 = gi.imageindex("tag4");
	imageindex_ctfsb2 = gi.imageindex("tag5");
	modelindex_flag1 = gi.modelindex("players/male/flag1.md2");
	modelindex_flag2 = gi.modelindex("players/male/flag2.md2");

	PrecacheItem(GetItemByIndex(IT_WEAPON_GRAPPLE));
}

/*--------------------------------------------------------------------------*/

const char *CTFTeamName(int team)
{
	switch (team)
	{
	case CTF_TEAM1:
		return "RED";
	case CTF_TEAM2:
		return "BLUE";
	case CTF_NOTEAM:
		return "SPECTATOR";
	}
	return "UNKNOWN"; // Hanzo pointed out this was spelled wrong as "UKNOWN"
}

const char *CTFOtherTeamName(int team)
{
	switch (team)
	{
	case CTF_TEAM1:
		return "BLUE";
	case CTF_TEAM2:
		return "RED";
	}
	return "UNKNOWN"; // Hanzo pointed out this was spelled wrong as "UKNOWN"
}

int CTFOtherTeam(int team)
{
	switch (team)
	{
	case CTF_TEAM1:
		return CTF_TEAM2;
	case CTF_TEAM2:
		return CTF_TEAM1;
	}
	return -1; // invalid value
}

/*--------------------------------------------------------------------------*/

float PlayersRangeFromSpot(edict_t *spot);
bool  SpawnPointClear(edict_t *spot);

void CTFAssignSkin(edict_t *ent, const char *s)
{
	int	  playernum = ent - g_edicts - 1;
	std::string_view t(s);

	if (size_t i = t.find_first_of('/'); i != std::string_view::npos)
		t = t.substr(0, i + 1);
	else
		t = "male/";

	switch (ent->client->resp.ctf_team)
	{
	case CTF_TEAM1:
		t = G_Fmt("{}\\{}{}\\default", ent->client->pers.netname, t, CTF_TEAM1_SKIN);
		break;
	case CTF_TEAM2:
		t = G_Fmt("{}\\{}{}\\default", ent->client->pers.netname, t, CTF_TEAM2_SKIN);
		break;
	default:
		t = G_Fmt("{}\\{}\\default", ent->client->pers.netname, s);
		break;
	}

	gi.configstring(CS_PLAYERSKINS + playernum, t.data());

	//	gi.LocClient_Print(ent, PRINT_HIGH, "$g_assigned_team", ent->client->pers.netname);
}

void CTFAssignTeam(gclient_t *who)
{
	edict_t *player;
	uint32_t team1count = 0, team2count = 0;

	who->resp.ctf_state = 0;

	if (!g_teamplay_force_join->integer && !(g_edicts[1 + (who - game.clients)].svflags & SVF_BOT))
	{
		who->resp.ctf_team = CTF_NOTEAM;
		return;
	}

	for (uint32_t i = 1; i <= game.maxclients; i++)
	{
		player = &g_edicts[i];

		if (!player->inuse || player->client == who)
			continue;

		switch (player->client->resp.ctf_team)
		{
		case CTF_TEAM1:
			team1count++;
			break;
		case CTF_TEAM2:
			team2count++;
			break;
		default:
			break;
		}
	}
	if (team1count < team2count)
		who->resp.ctf_team = CTF_TEAM1;
	else if (team2count < team1count)
		who->resp.ctf_team = CTF_TEAM2;
	else if (brandom())
		who->resp.ctf_team = CTF_TEAM1;
	else
		who->resp.ctf_team = CTF_TEAM2;
}

/*
================
SelectCTFSpawnPoint

go to a ctf point, but NOT the two points closest
to other players
================
*/
edict_t *SelectCTFSpawnPoint(edict_t *ent, bool force_spawn)
{
	if (ent->client->resp.ctf_state)
	{
		select_spawn_result_t result = SelectDeathmatchSpawnPoint(g_dm_spawn_farthest->integer, force_spawn, false);

		if (result.any_valid)
			return result.spot;
	}

	const char *cname;

	switch (ent->client->resp.ctf_team)
	{
	case CTF_TEAM1:
		cname = "info_player_team1";
		break;
	case CTF_TEAM2:
		cname = "info_player_team2";
		break;
	default:
	{
		select_spawn_result_t result = SelectDeathmatchSpawnPoint(g_dm_spawn_farthest->integer, force_spawn, true);

		if (result.any_valid)
			return result.spot;

		gi.Com_Error("can't find suitable spectator spawn point");
		return nullptr;
	}
	}

	static std::vector<edict_t *> spawn_points;
	edict_t *spot = nullptr;

	spawn_points.clear();

	while ((spot = G_FindByString<&edict_t::classname>(spot, cname)) != nullptr)
		spawn_points.push_back(spot);

	if (!spawn_points.size())
	{
		select_spawn_result_t result = SelectDeathmatchSpawnPoint(g_dm_spawn_farthest->integer, force_spawn, true);

		if (!result.any_valid)
			gi.Com_Error("can't find suitable CTF spawn point");

		return result.spot;
	}

	std::shuffle(spawn_points.begin(), spawn_points.end(), mt_rand);

	for (auto &point : spawn_points)
		if (SpawnPointClear(point))
			return point;
	
	if (force_spawn)
		return random_element(spawn_points);

	return nullptr;
}

/*------------------------------------------------------------------------*/
/*
CTFFragBonuses

Calculate the bonuses for flag defense, flag carrier defense, etc.
Note that bonuses are not cumaltive.  You get one, they are in importance
order.
*/
void CTFFragBonuses(edict_t *targ, edict_t *inflictor, edict_t *attacker)
{
	edict_t	*ent;
	item_id_t	flag_item, enemy_flag_item;
	int			otherteam;
	edict_t	*flag, *carrier = nullptr;
	const char *c;
	vec3_t		v1, v2;

	if (targ->client && attacker->client)
	{
		if (attacker->client->resp.ghost)
			if (attacker != targ)
				attacker->client->resp.ghost->kills++;
		if (targ->client->resp.ghost)
			targ->client->resp.ghost->deaths++;
	}

	// no bonus for fragging yourself
	if (!targ->client || !attacker->client || targ == attacker)
		return;

	otherteam = CTFOtherTeam(targ->client->resp.ctf_team);
	if (otherteam < 0)
		return; // whoever died isn't on a team

	// same team, if the flag at base, check to he has the enemy flag
	if (targ->client->resp.ctf_team == CTF_TEAM1)
	{
		flag_item = IT_FLAG1;
		enemy_flag_item = IT_FLAG2;
	}
	else
	{
		flag_item = IT_FLAG2;
		enemy_flag_item = IT_FLAG1;
	}

	// did the attacker frag the flag carrier?
	if (targ->client->pers.inventory[enemy_flag_item])
	{
		attacker->client->resp.ctf_lastfraggedcarrier = level.time;
		attacker->client->resp.score += CTF_FRAG_CARRIER_BONUS;
		gi.LocClient_Print(attacker, PRINT_MEDIUM, "$g_bonus_enemy_carrier",
				   CTF_FRAG_CARRIER_BONUS);

		// the target had the flag, clear the hurt carrier
		// field on the other team
		for (uint32_t i = 1; i <= game.maxclients; i++)
		{
			ent = g_edicts + i;
			if (ent->inuse && ent->client->resp.ctf_team == otherteam)
				ent->client->resp.ctf_lasthurtcarrier = 0_ms;
		}
		return;
	}

	if (targ->client->resp.ctf_lasthurtcarrier &&
		level.time - targ->client->resp.ctf_lasthurtcarrier < CTF_CARRIER_DANGER_PROTECT_TIMEOUT &&
		!attacker->client->pers.inventory[flag_item])
	{
		// attacker is on the same team as the flag carrier and
		// fragged a guy who hurt our flag carrier
		attacker->client->resp.score += CTF_CARRIER_DANGER_PROTECT_BONUS;
		gi.LocBroadcast_Print(PRINT_MEDIUM, "$g_bonus_flag_defense",
				   attacker->client->pers.netname,
				   CTFTeamName(attacker->client->resp.ctf_team));
		if (attacker->client->resp.ghost)
			attacker->client->resp.ghost->carrierdef++;
		return;
	}

	// flag and flag carrier area defense bonuses

	// we have to find the flag and carrier entities

	// find the flag
	switch (attacker->client->resp.ctf_team)
	{
	case CTF_TEAM1:
		c = "item_flag_team1";
		break;
	case CTF_TEAM2:
		c = "item_flag_team2";
		break;
	default:
		return;
	}

	flag = nullptr;
	while ((flag = G_FindByString<&edict_t::classname>(flag, c)) != nullptr)
	{
		if (!(flag->spawnflags & SPAWNFLAG_ITEM_DROPPED))
			break;
	}

	if (!flag)
		return; // can't find attacker's flag

	// find attacker's team's flag carrier
	for (uint32_t i = 1; i <= game.maxclients; i++)
	{
		carrier = g_edicts + i;
		if (carrier->inuse &&
			carrier->client->pers.inventory[flag_item])
			break;
		carrier = nullptr;
	}

	// ok we have the attackers flag and a pointer to the carrier

	// check to see if we are defending the base's flag
	v1 = targ->s.origin - flag->s.origin;
	v2 = attacker->s.origin - flag->s.origin;

	if ((v1.length() < CTF_TARGET_PROTECT_RADIUS ||
		 v2.length() < CTF_TARGET_PROTECT_RADIUS ||
		 loc_CanSee(flag, targ) || loc_CanSee(flag, attacker)) &&
		attacker->client->resp.ctf_team != targ->client->resp.ctf_team)
	{
		// we defended the base flag
		attacker->client->resp.score += CTF_FLAG_DEFENSE_BONUS;
		if (flag->solid == SOLID_NOT)
			gi.LocBroadcast_Print(PRINT_MEDIUM, "$g_bonus_defend_base",
					   attacker->client->pers.netname,
					   CTFTeamName(attacker->client->resp.ctf_team));
		else
			gi.LocBroadcast_Print(PRINT_MEDIUM, "$g_bonus_defend_flag",
					   attacker->client->pers.netname,
					   CTFTeamName(attacker->client->resp.ctf_team));
		if (attacker->client->resp.ghost)
			attacker->client->resp.ghost->basedef++;
		return;
	}

	if (carrier && carrier != attacker)
	{
		v1 = targ->s.origin - carrier->s.origin;
		v2 = attacker->s.origin - carrier->s.origin;

		if (v1.length() < CTF_ATTACKER_PROTECT_RADIUS ||
			v2.length() < CTF_ATTACKER_PROTECT_RADIUS ||
			loc_CanSee(carrier, targ) || loc_CanSee(carrier, attacker))
		{
			attacker->client->resp.score += CTF_CARRIER_PROTECT_BONUS;
			gi.LocBroadcast_Print(PRINT_MEDIUM, "$g_bonus_defend_carrier",
					   attacker->client->pers.netname,
					   CTFTeamName(attacker->client->resp.ctf_team));
			if (attacker->client->resp.ghost)
				attacker->client->resp.ghost->carrierdef++;
			return;
		}
	}
}

void CTFCheckHurtCarrier(edict_t *targ, edict_t *attacker)
{
	item_id_t flag_item;

	if (!targ->client || !attacker->client)
		return;

	if (targ->client->resp.ctf_team == CTF_TEAM1)
		flag_item = IT_FLAG2;
	else
		flag_item = IT_FLAG1;

	if (targ->client->pers.inventory[flag_item] &&
		targ->client->resp.ctf_team != attacker->client->resp.ctf_team)
		attacker->client->resp.ctf_lasthurtcarrier = level.time;
}

/*------------------------------------------------------------------------*/

void CTFResetFlag(int ctf_team)
{
	const char *c;
	edict_t	*ent;

	switch (ctf_team)
	{
	case CTF_TEAM1:
		c = "item_flag_team1";
		break;
	case CTF_TEAM2:
		c = "item_flag_team2";
		break;
	default:
		return;
	}

	ent = nullptr;
	while ((ent = G_FindByString<&edict_t::classname>(ent, c)) != nullptr)
	{
		if (ent->spawnflags.has(SPAWNFLAG_ITEM_DROPPED))
			G_FreeEdict(ent);
		else
		{
			ent->svflags &= ~SVF_NOCLIENT;
			ent->solid = SOLID_TRIGGER;
			gi.linkentity(ent);
			ent->s.event = EV_ITEM_RESPAWN;
		}
	}
}

void CTFResetFlags()
{
	CTFResetFlag(CTF_TEAM1);
	CTFResetFlag(CTF_TEAM2);
}

bool CTFPickup_Flag(edict_t *ent, edict_t *other)
{
	int		  ctf_team;
	edict_t	*player;
	item_id_t flag_item, enemy_flag_item;

	// figure out what team this flag is
	if (ent->item->id == IT_FLAG1)
		ctf_team = CTF_TEAM1;
	else if (ent->item->id == IT_FLAG2)
		ctf_team = CTF_TEAM2;
	else
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "Don't know what team the flag is on.\n");
		return false;
	}

	// same team, if the flag at base, check to he has the enemy flag
	if (ctf_team == CTF_TEAM1)
	{
		flag_item = IT_FLAG1;
		enemy_flag_item = IT_FLAG2;
	}
	else
	{
		flag_item = IT_FLAG2;
		enemy_flag_item = IT_FLAG1;
	}

	if (ctf_team == other->client->resp.ctf_team)
	{

		if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED))
		{
			// the flag is at home base.  if the player has the enemy
			// flag, he's just won!

			if (other->client->pers.inventory[enemy_flag_item])
			{
				gi.LocBroadcast_Print(PRINT_HIGH, "$g_flag_captured",
						   other->client->pers.netname, CTFOtherTeamName(ctf_team));
				other->client->pers.inventory[enemy_flag_item] = 0;

				ctfgame.last_flag_capture = level.time;
				ctfgame.last_capture_team = ctf_team;
				if (ctf_team == CTF_TEAM1)
					ctfgame.team1++;
				else
					ctfgame.team2++;

				gi.sound(ent, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundindex("ctf/flagcap.wav"), 1, ATTN_NONE, 0);

				// other gets another 10 frag bonus
				other->client->resp.score += CTF_CAPTURE_BONUS;
				if (other->client->resp.ghost)
					other->client->resp.ghost->caps++;

				// Ok, let's do the player loop, hand out the bonuses
				for (uint32_t i = 1; i <= game.maxclients; i++)
				{
					player = &g_edicts[i];
					if (!player->inuse)
						continue;

					if (player->client->resp.ctf_team != other->client->resp.ctf_team)
						player->client->resp.ctf_lasthurtcarrier = -5_sec;
					else if (player->client->resp.ctf_team == other->client->resp.ctf_team)
					{
						if (player != other)
							player->client->resp.score += CTF_TEAM_BONUS;
						// award extra points for capture assists
						if (player->client->resp.ctf_lastreturnedflag && player->client->resp.ctf_lastreturnedflag + CTF_RETURN_FLAG_ASSIST_TIMEOUT > level.time)
						{
							gi.LocBroadcast_Print(PRINT_HIGH, "$g_bonus_assist_return", player->client->pers.netname);
							player->client->resp.score += CTF_RETURN_FLAG_ASSIST_BONUS;
						}
						if (player->client->resp.ctf_lastfraggedcarrier && player->client->resp.ctf_lastfraggedcarrier + CTF_FRAG_CARRIER_ASSIST_TIMEOUT > level.time)
						{
							gi.LocBroadcast_Print(PRINT_HIGH, "$g_bonus_assist_frag_carrier", player->client->pers.netname);
							player->client->resp.score += CTF_FRAG_CARRIER_ASSIST_BONUS;
						}
					}
				}

				CTFResetFlags();
				return false;
			}
			return false; // its at home base already
		}
		// hey, its not home.  return it by teleporting it back
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_returned_flag",
				   other->client->pers.netname, CTFTeamName(ctf_team));
		other->client->resp.score += CTF_RECOVERY_BONUS;
		other->client->resp.ctf_lastreturnedflag = level.time;
		gi.sound(ent, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundindex("ctf/flagret.wav"), 1, ATTN_NONE, 0);
		// CTFResetFlag will remove this entity!  We must return false
		CTFResetFlag(ctf_team);
		return false;
	}

	// hey, its not our flag, pick it up
	gi.LocBroadcast_Print(PRINT_HIGH, "$g_got_flag",
			   other->client->pers.netname, CTFTeamName(ctf_team));
	other->client->resp.score += CTF_FLAG_BONUS;

	other->client->pers.inventory[flag_item] = 1;
	other->client->resp.ctf_flagsince = level.time;

	// pick up the flag
	// if it's not a dropped flag, we just make is disappear
	// if it's dropped, it will be removed by the pickup caller
	if (!(ent->spawnflags & SPAWNFLAG_ITEM_DROPPED))
	{
		ent->flags |= FL_RESPAWN;
		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
	}
	return true;
}

TOUCH(CTFDropFlagTouch) (edict_t *ent, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	// owner (who dropped us) can't touch for two secs
	if (other == ent->owner &&
		ent->nextthink - level.time > CTF_AUTO_FLAG_RETURN_TIMEOUT - 2_sec)
		return;

	Touch_Item(ent, other, tr, other_touching_self);
}

THINK(CTFDropFlagThink) (edict_t *ent) -> void
{
	// auto return the flag
	// reset flag will remove ourselves
	if (ent->item->id == IT_FLAG1)
	{
		CTFResetFlag(CTF_TEAM1);
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_flag_returned",
				   CTFTeamName(CTF_TEAM1));
	}
	else if (ent->item->id == IT_FLAG2)
	{
		CTFResetFlag(CTF_TEAM2);
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_flag_returned",
				   CTFTeamName(CTF_TEAM2));
	}

	gi.sound(ent, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundindex("ctf/flagret.wav"), 1, ATTN_NONE, 0);
}

// Called from PlayerDie, to drop the flag from a dying player
void CTFDeadDropFlag(edict_t *self)
{
	edict_t *dropped = nullptr;

	if (self->client->pers.inventory[IT_FLAG1])
	{
		dropped = Drop_Item(self, GetItemByIndex(IT_FLAG1));
		self->client->pers.inventory[IT_FLAG1] = 0;
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_lost_flag",
				   self->client->pers.netname, CTFTeamName(CTF_TEAM1));
	}
	else if (self->client->pers.inventory[IT_FLAG2])
	{
		dropped = Drop_Item(self, GetItemByIndex(IT_FLAG2));
		self->client->pers.inventory[IT_FLAG2] = 0;
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_lost_flag",
				   self->client->pers.netname, CTFTeamName(CTF_TEAM2));
	}

	if (dropped)
	{
		dropped->think = CTFDropFlagThink;
		dropped->nextthink = level.time + CTF_AUTO_FLAG_RETURN_TIMEOUT;
		dropped->touch = CTFDropFlagTouch;
	}
}

void CTFDrop_Flag(edict_t *ent, gitem_t *item)
{
	if (brandom())
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_lusers_drop_flags");
	else
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_winners_drop_flags");
}

THINK(CTFFlagThink) (edict_t *ent) -> void
{
	if (ent->solid != SOLID_NOT)
		ent->s.frame = 173 + (((ent->s.frame - 173) + 1) % 16);
	ent->nextthink = level.time + 10_hz;
}

THINK(CTFFlagSetup) (edict_t *ent) -> void
{
	trace_t tr;
	vec3_t	dest;

	ent->mins = { -15, -15, -15 };
	ent->maxs = { 15, 15, 15 };

	if (ent->model)
		gi.setmodel(ent, ent->model);
	else
		gi.setmodel(ent, ent->item->world_model);
	ent->solid = SOLID_TRIGGER;
	ent->movetype = MOVETYPE_TOSS;
	ent->touch = Touch_Item;
	ent->s.frame = 173;

	dest = ent->s.origin + vec3_t { 0, 0, -128 };

	tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, dest, ent, MASK_SOLID);
	if (tr.startsolid)
	{
		gi.Com_PrintFmt("CTFFlagSetup: {} startsolid at {}\n", ent->classname, ent->s.origin);
		G_FreeEdict(ent);
		return;
	}

	ent->s.origin = tr.endpos;

	gi.linkentity(ent);

	ent->nextthink = level.time + 10_hz;
	ent->think = CTFFlagThink;
}

void CTFEffects(edict_t *player)
{
	player->s.effects &= ~(EF_FLAG1 | EF_FLAG2);
	if (player->health > 0)
	{
		if (player->client->pers.inventory[IT_FLAG1])
		{
			player->s.effects |= EF_FLAG1;
		}
		if (player->client->pers.inventory[IT_FLAG2])
		{
			player->s.effects |= EF_FLAG2;
		}
	}

	if (player->client->pers.inventory[IT_FLAG1])
		player->s.modelindex3 = modelindex_flag1;
	else if (player->client->pers.inventory[IT_FLAG2])
		player->s.modelindex3 = modelindex_flag2;
	else
		player->s.modelindex3 = 0;
}

// called when we enter the intermission
void CTFCalcScores()
{
	ctfgame.total1 = ctfgame.total2 = 0;
	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		if (!g_edicts[i + 1].inuse)
			continue;
		if (game.clients[i].resp.ctf_team == CTF_TEAM1)
			ctfgame.total1 += game.clients[i].resp.score;
		else if (game.clients[i].resp.ctf_team == CTF_TEAM2)
			ctfgame.total2 += game.clients[i].resp.score;
	}
}

// [Paril-KEX] end game rankings
void CTFCalcRankings(std::array<uint32_t, MAX_CLIENTS> &player_ranks)
{
	// we're all winners.. or losers. whatever
	if (ctfgame.total1 == ctfgame.total2)
	{
		player_ranks.fill(1);
		return;
	}

	ctfteam_t winning_team = (ctfgame.total1 > ctfgame.total2) ? CTF_TEAM1 : CTF_TEAM2;

	for (auto player : active_players())
		if (player->client->pers.spawned && player->client->resp.ctf_team != CTF_NOTEAM)
			player_ranks[player->s.number - 1] = player->client->resp.ctf_team == winning_team ? 1 : 2;
}

void CheckEndTDMLevel()
{
	if (ctfgame.total1 >= fraglimit->integer || ctfgame.total2 >= fraglimit->integer)
	{
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_fraglimit_hit");
		EndDMLevel();
	}
}

void CTFID_f(edict_t *ent)
{
	if (ent->client->resp.id_state)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "Disabling player identication display.\n");
		ent->client->resp.id_state = false;
	}
	else
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "Activating player identication display.\n");
		ent->client->resp.id_state = true;
	}
}

static void CTFSetIDView(edict_t *ent)
{
	vec3_t	 forward, dir;
	trace_t	 tr;
	edict_t *who, *best;
	float	 bd = 0, d;

	// only check every few frames
	if (level.time - ent->client->resp.lastidtime < 250_ms)
		return;
	ent->client->resp.lastidtime = level.time;

	ent->client->ps.stats[STAT_CTF_ID_VIEW] = 0;
	ent->client->ps.stats[STAT_CTF_ID_VIEW_COLOR] = 0;

	AngleVectors(ent->client->v_angle, forward, nullptr, nullptr);
	forward *= 1024;
	forward = ent->s.origin + forward;
	tr = gi.traceline(ent->s.origin, forward, ent, MASK_SOLID);
	if (tr.fraction < 1 && tr.ent && tr.ent->client)
	{
		ent->client->ps.stats[STAT_CTF_ID_VIEW] = (tr.ent - g_edicts);
		if (tr.ent->client->resp.ctf_team == CTF_TEAM1)
			ent->client->ps.stats[STAT_CTF_ID_VIEW_COLOR] = imageindex_sbfctf1;
		else if (tr.ent->client->resp.ctf_team == CTF_TEAM2)
			ent->client->ps.stats[STAT_CTF_ID_VIEW_COLOR] = imageindex_sbfctf2;
		return;
	}

	AngleVectors(ent->client->v_angle, forward, nullptr, nullptr);
	best = nullptr;
	for (uint32_t i = 1; i <= game.maxclients; i++)
	{
		who = g_edicts + i;
		if (!who->inuse || who->solid == SOLID_NOT)
			continue;
		dir = who->s.origin - ent->s.origin;
		dir.normalize();
		d = forward.dot(dir);

		// we have teammate indicators that are better for this
		if (ent->client->resp.ctf_team == who->client->resp.ctf_team)
			continue;

		if (d > bd && loc_CanSee(ent, who))
		{
			bd = d;
			best = who;
		}
	}
	if (bd > 0.90f)
	{
		ent->client->ps.stats[STAT_CTF_ID_VIEW] = (best - g_edicts);
		if (best->client->resp.ctf_team == CTF_TEAM1)
			ent->client->ps.stats[STAT_CTF_ID_VIEW_COLOR] = imageindex_sbfctf1;
		else if (best->client->resp.ctf_team == CTF_TEAM2)
			ent->client->ps.stats[STAT_CTF_ID_VIEW_COLOR] = imageindex_sbfctf2;
	}
}

void SetCTFStats(edict_t *ent)
{
	uint32_t i;
	int		 p1, p2;
	edict_t *e;

	if (ctfgame.match > MATCH_NONE)
		ent->client->ps.stats[STAT_CTF_MATCH] = CONFIG_CTF_MATCH;
	else
		ent->client->ps.stats[STAT_CTF_MATCH] = 0;

	if (ctfgame.warnactive)
		ent->client->ps.stats[STAT_CTF_TEAMINFO] = CONFIG_CTF_TEAMINFO;
	else
		ent->client->ps.stats[STAT_CTF_TEAMINFO] = 0;

	// ghosting
	if (ent->client->resp.ghost)
	{
		ent->client->resp.ghost->score = ent->client->resp.score;
		Q_strlcpy(ent->client->resp.ghost->netname, ent->client->pers.netname, sizeof(ent->client->resp.ghost->netname));
		ent->client->resp.ghost->number = ent->s.number;
	}

	// logo headers for the frag display
	ent->client->ps.stats[STAT_CTF_TEAM1_HEADER] = imageindex_ctfsb1;
	ent->client->ps.stats[STAT_CTF_TEAM2_HEADER] = imageindex_ctfsb2;

	bool blink = (level.time.milliseconds() % 1000) < 500;

	// if during intermission, we must blink the team header of the winning team
	if (level.intermissiontime && blink)
	{
		// blink half second
		// note that ctfgame.total[12] is set when we go to intermission
		if (ctfgame.team1 > ctfgame.team2)
			ent->client->ps.stats[STAT_CTF_TEAM1_HEADER] = 0;
		else if (ctfgame.team2 > ctfgame.team1)
			ent->client->ps.stats[STAT_CTF_TEAM2_HEADER] = 0;
		else if (ctfgame.total1 > ctfgame.total2) // frag tie breaker
			ent->client->ps.stats[STAT_CTF_TEAM1_HEADER] = 0;
		else if (ctfgame.total2 > ctfgame.total1)
			ent->client->ps.stats[STAT_CTF_TEAM2_HEADER] = 0;
		else
		{ // tie game!
			ent->client->ps.stats[STAT_CTF_TEAM1_HEADER] = 0;
			ent->client->ps.stats[STAT_CTF_TEAM2_HEADER] = 0;
		}
	}

	// tech icon
	i = 0;
	ent->client->ps.stats[STAT_CTF_TECH] = 0;
	for (; i < q_countof(tech_ids); i++)
	{
		if (ent->client->pers.inventory[tech_ids[i]])
		{
			ent->client->ps.stats[STAT_CTF_TECH] = gi.imageindex(GetItemByIndex(tech_ids[i])->icon);
			break;
		}
	}

	if (ctf->integer)
	{
		// figure out what icon to display for team logos
		// three states:
		//   flag at base
		//   flag taken
		//   flag dropped
		p1 = imageindex_i_ctf1;
		e = G_FindByString<&edict_t::classname>(nullptr, "item_flag_team1");
		if (e != nullptr)
		{
			if (e->solid == SOLID_NOT)
			{
				// not at base
				// check if on player
				p1 = imageindex_i_ctf1d; // default to dropped
				for (i = 1; i <= game.maxclients; i++)
					if (g_edicts[i].inuse &&
						g_edicts[i].client->pers.inventory[IT_FLAG1])
					{
						// enemy has it
						p1 = imageindex_i_ctf1t;
						break;
					}

				// [Paril-KEX] make sure there is a dropped version on the map somewhere
				if (p1 == imageindex_i_ctf1d)
				{
					e = G_FindByString<&edict_t::classname>(e, "item_flag_team1");

					if (e == nullptr)
					{
						CTFResetFlag(CTF_TEAM1);
						gi.LocBroadcast_Print(PRINT_HIGH, "$g_flag_returned",
							CTFTeamName(CTF_TEAM1));
						gi.sound(ent, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundindex("ctf/flagret.wav"), 1, ATTN_NONE, 0);
					}
				}
			}
			else if (e->spawnflags.has(SPAWNFLAG_ITEM_DROPPED))
				p1 = imageindex_i_ctf1d; // must be dropped
		}
		p2 = imageindex_i_ctf2;
		e = G_FindByString<&edict_t::classname>(nullptr, "item_flag_team2");
		if (e != nullptr)
		{
			if (e->solid == SOLID_NOT)
			{
				// not at base
				// check if on player
				p2 = imageindex_i_ctf2d; // default to dropped
				for (i = 1; i <= game.maxclients; i++)
					if (g_edicts[i].inuse &&
						g_edicts[i].client->pers.inventory[IT_FLAG2])
					{
						// enemy has it
						p2 = imageindex_i_ctf2t;
						break;
					}

				// [Paril-KEX] make sure there is a dropped version on the map somewhere
				if (p2 == imageindex_i_ctf2d)
				{
					e = G_FindByString<&edict_t::classname>(e, "item_flag_team2");

					if (e == nullptr)
					{
						CTFResetFlag(CTF_TEAM2);
						gi.LocBroadcast_Print(PRINT_HIGH, "$g_flag_returned",
							CTFTeamName(CTF_TEAM2));
						gi.sound(ent, CHAN_RELIABLE | CHAN_NO_PHS_ADD | CHAN_AUX, gi.soundindex("ctf/flagret.wav"), 1, ATTN_NONE, 0);
					}
				}
			}
			else if (e->spawnflags.has(SPAWNFLAG_ITEM_DROPPED))
				p2 = imageindex_i_ctf2d; // must be dropped
		}

		ent->client->ps.stats[STAT_CTF_TEAM1_PIC] = p1;
		ent->client->ps.stats[STAT_CTF_TEAM2_PIC] = p2;

		if (ctfgame.last_flag_capture && level.time - ctfgame.last_flag_capture < 5_sec)
		{
			if (ctfgame.last_capture_team == CTF_TEAM1)
				if (blink)
					ent->client->ps.stats[STAT_CTF_TEAM1_PIC] = p1;
				else
					ent->client->ps.stats[STAT_CTF_TEAM1_PIC] = 0;
			else if (blink)
				ent->client->ps.stats[STAT_CTF_TEAM2_PIC] = p2;
			else
				ent->client->ps.stats[STAT_CTF_TEAM2_PIC] = 0;
		}

		ent->client->ps.stats[STAT_CTF_TEAM1_CAPS] = ctfgame.team1;
		ent->client->ps.stats[STAT_CTF_TEAM2_CAPS] = ctfgame.team2;

		ent->client->ps.stats[STAT_CTF_FLAG_PIC] = 0;
		if (ent->client->resp.ctf_team == CTF_TEAM1 &&
			ent->client->pers.inventory[IT_FLAG2] &&
			(blink))
			ent->client->ps.stats[STAT_CTF_FLAG_PIC] = imageindex_i_ctf2;

		else if (ent->client->resp.ctf_team == CTF_TEAM2 &&
				 ent->client->pers.inventory[IT_FLAG1] &&
				 (blink))
			ent->client->ps.stats[STAT_CTF_FLAG_PIC] = imageindex_i_ctf1;
	}
	else
	{
		ent->client->ps.stats[STAT_CTF_TEAM1_PIC] = imageindex_i_ctf1;
		ent->client->ps.stats[STAT_CTF_TEAM2_PIC] = imageindex_i_ctf2;

		ent->client->ps.stats[STAT_CTF_TEAM1_CAPS] = ctfgame.total1;
		ent->client->ps.stats[STAT_CTF_TEAM2_CAPS] = ctfgame.total2;
	}

	ent->client->ps.stats[STAT_CTF_JOINED_TEAM1_PIC] = 0;
	ent->client->ps.stats[STAT_CTF_JOINED_TEAM2_PIC] = 0;
	if (ent->client->resp.ctf_team == CTF_TEAM1)
		ent->client->ps.stats[STAT_CTF_JOINED_TEAM1_PIC] = imageindex_i_ctfj;
	else if (ent->client->resp.ctf_team == CTF_TEAM2)
		ent->client->ps.stats[STAT_CTF_JOINED_TEAM2_PIC] = imageindex_i_ctfj;

	if (ent->client->resp.id_state)
		CTFSetIDView(ent);
	else
	{
		ent->client->ps.stats[STAT_CTF_ID_VIEW] = 0;
		ent->client->ps.stats[STAT_CTF_ID_VIEW_COLOR] = 0;
	}
}

/*------------------------------------------------------------------------*/

/*QUAKED info_player_team1 (1 0 0) (-16 -16 -24) (16 16 32)
potential team1 spawning position for ctf games
*/
void SP_info_player_team1(edict_t *self)
{
}

/*QUAKED info_player_team2 (0 0 1) (-16 -16 -24) (16 16 32)
potential team2 spawning position for ctf games
*/
void SP_info_player_team2(edict_t *self)
{
}

/*------------------------------------------------------------------------*/
/* GRAPPLE																  */
/*------------------------------------------------------------------------*/

// ent is player
void CTFPlayerResetGrapple(edict_t *ent)
{
	if (ent->client && ent->client->ctf_grapple)
		CTFResetGrapple(ent->client->ctf_grapple);
}

// self is grapple, not player
void CTFResetGrapple(edict_t *self)
{
	if (!self->owner->client->ctf_grapple)
		return;
	
	gi.sound(self->owner, CHAN_WEAPON, gi.soundindex("weapons/grapple/grreset.wav"), self->owner->client->silencer_shots ? 0.2f : 1.0f, ATTN_NORM, 0);

	gclient_t *cl;
	cl = self->owner->client;
	cl->ctf_grapple = nullptr;
	cl->ctf_grapplereleasetime = level.time + 1_sec;
	cl->ctf_grapplestate = CTF_GRAPPLE_STATE_FLY; // we're firing, not on hook
	self->owner->flags &= ~FL_NO_KNOCKBACK;
	G_FreeEdict(self);
}

TOUCH(CTFGrappleTouch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	float volume = 1.0;

	if (other == self->owner)
		return;

	if (self->owner->client->ctf_grapplestate != CTF_GRAPPLE_STATE_FLY)
		return;

	if (tr.surface && (tr.surface->flags & SURF_SKY))
	{
		CTFResetGrapple(self);
		return;
	}

	self->velocity = {};

	PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	if (other->takedamage)
	{
		if (self->dmg)
			T_Damage(other, self, self->owner, self->velocity, self->s.origin, tr.plane.normal, self->dmg, 1, DAMAGE_NONE, MOD_GRAPPLE);
		CTFResetGrapple(self);
		return;
	}

	self->owner->client->ctf_grapplestate = CTF_GRAPPLE_STATE_PULL; // we're on hook
	self->enemy = other;

	self->solid = SOLID_NOT;

	if (self->owner->client->silencer_shots)
		volume = 0.2f;

	gi.sound(self, CHAN_WEAPON, gi.soundindex("weapons/grapple/grhit.wav"), volume, ATTN_NORM, 0);
	self->s.sound = gi.soundindex("weapons/grapple/grpull.wav");

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_SPARKS);
	gi.WritePosition(self->s.origin);
	gi.WriteDir(tr.plane.normal);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);
}

// draw beam between grapple and self
void CTFGrappleDrawCable(edict_t *self)
{
	if (self->owner->client->ctf_grapplestate == CTF_GRAPPLE_STATE_HANG)
		return;

	vec3_t start, dir;
	P_ProjectSource(self->owner, self->owner->client->v_angle, { 7, 2, -9 }, start, dir);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_GRAPPLE_CABLE_2);
	gi.WriteEntity(self->owner);
	gi.WritePosition(start);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);
}

void SV_AddGravity(edict_t *ent);

// pull the player toward the grapple
void CTFGrapplePull(edict_t *self)
{
	vec3_t hookdir, v;
	float  vlen;

	if (self->owner->client->pers.weapon && self->owner->client->pers.weapon->id == IT_WEAPON_GRAPPLE &&
		!(self->owner->client->newweapon || ((self->owner->client->latched_buttons | self->owner->client->buttons) & BUTTON_HOLSTER)) &&
		self->owner->client->weaponstate != WEAPON_FIRING &&
		self->owner->client->weaponstate != WEAPON_ACTIVATING)
	{
		if (!self->owner->client->newweapon)
			self->owner->client->newweapon = self->owner->client->pers.weapon;

		CTFResetGrapple(self);
		return;
	}

	if (self->enemy)
	{
		if (self->enemy->solid == SOLID_NOT)
		{
			CTFResetGrapple(self);
			return;
		}
		if (self->enemy->solid == SOLID_BBOX)
		{
			v = self->enemy->size * 0.5f;
			v += self->enemy->s.origin;
			self->s.origin = v + self->enemy->mins;
			gi.linkentity(self);
		}
		else
			self->velocity = self->enemy->velocity;

		if (self->enemy->deadflag)
		{ // he died
			CTFResetGrapple(self);
			return;
		}
	}

	CTFGrappleDrawCable(self);

	if (self->owner->client->ctf_grapplestate > CTF_GRAPPLE_STATE_FLY)
	{
		// pull player toward grapple
		vec3_t forward, up;

		AngleVectors(self->owner->client->v_angle, forward, nullptr, up);
		v = self->owner->s.origin;
		v[2] += self->owner->viewheight;
		hookdir = self->s.origin - v;

		vlen = hookdir.length();

		if (self->owner->client->ctf_grapplestate == CTF_GRAPPLE_STATE_PULL &&
			vlen < 64)
		{
			self->owner->client->ctf_grapplestate = CTF_GRAPPLE_STATE_HANG;
			self->s.sound = gi.soundindex("weapons/grapple/grhang.wav");
		}

		hookdir.normalize();
		hookdir = hookdir * g_grapple_pull_speed->value;
		self->owner->velocity = hookdir;
		self->owner->flags |= FL_NO_KNOCKBACK;
		SV_AddGravity(self->owner);
	}
}

DIE(grapple_die) (edict_t *self, edict_t *other, edict_t *inflictor, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	if (mod.id == MOD_CRUSH)
		CTFResetGrapple(self);
}

bool CTFFireGrapple(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, effects_t effect)
{
	edict_t *grapple;
	trace_t	 tr;
	vec3_t	 normalized = dir.normalized();

	grapple = G_Spawn();
	grapple->s.origin = start;
	grapple->s.old_origin = start;
	grapple->s.angles = vectoangles(normalized);
	grapple->velocity = normalized * speed;
	grapple->movetype = MOVETYPE_FLYMISSILE;
	grapple->clipmask = MASK_PROJECTILE;
	// [Paril-KEX]
	if (self->client && !G_ShouldPlayersCollide(true))
		grapple->clipmask &= ~CONTENTS_PLAYER;
	grapple->solid = SOLID_BBOX;
	grapple->s.effects |= effect;
	grapple->s.modelindex = gi.modelindex("models/weapons/grapple/hook/tris.md2");
	grapple->owner = self;
	grapple->touch = CTFGrappleTouch;
	grapple->dmg = damage;
	grapple->flags |= FL_NO_KNOCKBACK | FL_NO_DAMAGE_EFFECTS;
	grapple->takedamage = true;
	grapple->die = grapple_die;
	self->client->ctf_grapple = grapple;
	self->client->ctf_grapplestate = CTF_GRAPPLE_STATE_FLY; // we're firing, not on hook
	gi.linkentity(grapple);

	tr = gi.traceline(self->s.origin, grapple->s.origin, grapple, grapple->clipmask);
	if (tr.fraction < 1.0f)
	{
		grapple->s.origin = tr.endpos + (tr.plane.normal * 1.f);
		grapple->touch(grapple, tr.ent, tr, false);
		return false;
	}

	grapple->s.sound = gi.soundindex("weapons/grapple/grfly.wav");

	return true;
}

void CTFGrappleFire(edict_t *ent, const vec3_t &g_offset, int damage, effects_t effect)
{
	float volume = 1.0;

	if (ent->client->ctf_grapplestate > CTF_GRAPPLE_STATE_FLY)
		return; // it's already out

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->v_angle, vec3_t{ 24, 8, -8 + 2 } + g_offset, start, dir);

	if (ent->client->silencer_shots)
		volume = 0.2f;

	if (CTFFireGrapple(ent, start, dir, damage, g_grapple_fly_speed->value, effect))
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/grapple/grfire.wav"), volume, ATTN_NORM, 0);

	PlayerNoise(ent, start, PNOISE_WEAPON);
}

void CTFWeapon_Grapple_Fire(edict_t *ent)
{
	CTFGrappleFire(ent, vec3_origin, g_grapple_damage->integer, EF_NONE);
}

void CTFWeapon_Grapple(edict_t *ent)
{
	constexpr int pause_frames[] = { 10, 18, 27, 0 };
	constexpr int fire_frames[] = { 6, 0 };
	int			  prevstate;

	// if the the attack button is still down, stay in the firing frame
	if ((ent->client->buttons & (BUTTON_ATTACK | BUTTON_HOLSTER)) &&
		ent->client->weaponstate == WEAPON_FIRING &&
		ent->client->ctf_grapple)
		ent->client->ps.gunframe = 6;

	if (!(ent->client->buttons & (BUTTON_ATTACK | BUTTON_HOLSTER)) &&
		ent->client->ctf_grapple)
	{
		CTFResetGrapple(ent->client->ctf_grapple);
		if (ent->client->weaponstate == WEAPON_FIRING)
			ent->client->weaponstate = WEAPON_READY;
	}

	if ((ent->client->newweapon || ((ent->client->latched_buttons | ent->client->buttons) & BUTTON_HOLSTER)) &&
		ent->client->ctf_grapplestate > CTF_GRAPPLE_STATE_FLY &&
		ent->client->weaponstate == WEAPON_FIRING)
	{
		// he wants to change weapons while grappled
		if (!ent->client->newweapon)
			ent->client->newweapon = ent->client->pers.weapon;
		ent->client->weaponstate = WEAPON_DROPPING;
		ent->client->ps.gunframe = 32;
	}

	prevstate = ent->client->weaponstate;
	Weapon_Generic(ent, 5, 10, 31, 36, pause_frames, fire_frames,
				   CTFWeapon_Grapple_Fire);

	// if the the attack button is still down, stay in the firing frame
	if ((ent->client->buttons & (BUTTON_ATTACK | BUTTON_HOLSTER)) &&
		ent->client->weaponstate == WEAPON_FIRING &&
		ent->client->ctf_grapple)
		ent->client->ps.gunframe = 6;

	// if we just switched back to grapple, immediately go to fire frame
	if (prevstate == WEAPON_ACTIVATING &&
		ent->client->weaponstate == WEAPON_READY &&
		ent->client->ctf_grapplestate > CTF_GRAPPLE_STATE_FLY)
	{
		if (!(ent->client->buttons & (BUTTON_ATTACK | BUTTON_HOLSTER)))
			ent->client->ps.gunframe = 6;
		else
			ent->client->ps.gunframe = 5;
		ent->client->weaponstate = WEAPON_FIRING;
	}
}

void CTFDirtyTeamMenu()
{
	for (auto player : active_players())
		if (player->client->menu)
		{
			player->client->menudirty = true;
			player->client->menutime = level.time;
		}
}

void CTFTeam_f(edict_t *ent)
{
	if (!G_TeamplayEnabled())
		return;

	const char *t;
	ctfteam_t	  desired_team;

	t = gi.args();
	if (!*t)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_you_are_on_team",
				   CTFTeamName(ent->client->resp.ctf_team));
		return;
	}

	if (ctfgame.match > MATCH_SETUP)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_cant_change_teams");
		return;
	}

	// [Paril-KEX] with force-join, don't allow us to switch
	// using this command.
	if (g_teamplay_force_join->integer)
	{
		if (!(ent->svflags & SVF_BOT))
		{
			gi.LocClient_Print(ent, PRINT_HIGH, "$g_cant_change_teams");
			return;
		}
	}

	if (Q_strcasecmp(t, "red") == 0)
		desired_team = CTF_TEAM1;
	else if (Q_strcasecmp(t, "blue") == 0)
		desired_team = CTF_TEAM2;
	else
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_unknown_team", t);
		return;
	}

	if (ent->client->resp.ctf_team == desired_team)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "$g_already_on_team",
				   CTFTeamName(ent->client->resp.ctf_team));
		return;
	}

	////
	ent->svflags = SVF_NONE;
	ent->flags &= ~FL_GODMODE;
	ent->client->resp.ctf_team = desired_team;
	ent->client->resp.ctf_state = 0;
	char value[MAX_INFO_VALUE] = { 0 };
	gi.Info_ValueForKey(ent->client->pers.userinfo, "skin", value, sizeof(value));
	CTFAssignSkin(ent, value);

	// if anybody has a menu open, update it immediately
	CTFDirtyTeamMenu();

	if (ent->solid == SOLID_NOT)
	{
		// spectator
		PutClientInServer(ent);

		G_PostRespawn(ent);

		gi.LocBroadcast_Print(PRINT_HIGH, "$g_joined_team",
				   ent->client->pers.netname, CTFTeamName(desired_team));
		return;
	}

	ent->health = 0;
	player_die(ent, ent, ent, 100000, vec3_origin, { MOD_SUICIDE, true });

	// don't even bother waiting for death frames
	ent->deadflag = true;
	respawn(ent);

	ent->client->resp.score = 0;

	gi.LocBroadcast_Print(PRINT_HIGH, "$g_changed_team",
			   ent->client->pers.netname, CTFTeamName(desired_team));
}

constexpr size_t MAX_CTF_STAT_LENGTH = 1024;

/*
==================
CTFScoreboardMessage
==================
*/
void CTFScoreboardMessage(edict_t *ent, edict_t *killer)
{
	uint32_t   i, j, k, n;
	uint32_t   sorted[2][MAX_CLIENTS];
	int32_t    sortedscores[2][MAX_CLIENTS];
	int		   score;
	uint32_t   total[2];
	int        totalscore[2];
	uint32_t   last[2];
	gclient_t *cl;
	edict_t	*cl_ent;
	int		   team;

	// sort the clients by team and score
	total[0] = total[1] = 0;
	last[0] = last[1] = 0;
	totalscore[0] = totalscore[1] = 0;
	for (i = 0; i < game.maxclients; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse)
			continue;
		if (game.clients[i].resp.ctf_team == CTF_TEAM1)
			team = 0;
		else if (game.clients[i].resp.ctf_team == CTF_TEAM2)
			team = 1;
		else
			continue; // unknown team?

		score = game.clients[i].resp.score;
		for (j = 0; j < total[team]; j++)
		{
			if (score > sortedscores[team][j])
				break;
		}
		for (k = total[team]; k > j; k--)
		{
			sorted[team][k] = sorted[team][k - 1];
			sortedscores[team][k] = sortedscores[team][k - 1];
		}
		sorted[team][j] = i;
		sortedscores[team][j] = score;
		totalscore[team] += score;
		total[team]++;
	}

	// print level name and exit rules
	// add the clients in sorted order
	static std::string string;
	string.clear();

	// [Paril-KEX] time & frags
	if (teamplay->integer)
	{
		if (fraglimit->integer)
		{
			fmt::format_to(std::back_inserter(string), FMT_STRING("xv -20 yv -10 loc_string2 1 $g_score_frags \"{}\" "), fraglimit->integer);
		}
	}
	else
	{
		if (capturelimit->integer)
		{
			fmt::format_to(std::back_inserter(string), FMT_STRING("xv -20 yv -10 loc_string2 1 $g_score_captures \"{}\" "), capturelimit->integer);
		}
	}
	if (timelimit->value)
	{
		fmt::format_to(std::back_inserter(string), FMT_STRING("xv 340 yv -10 time_limit {} "), gi.ServerFrame() + ((gtime_t::from_min(timelimit->value) - level.time)).milliseconds() / gi.frame_time_ms);
	}

	// team one
	if (teamplay->integer)
	{
		fmt::format_to(std::back_inserter(string),
			FMT_STRING("if 25 xv -32 yv 8 pic 25 endif "
			"xv -123 yv 28 cstring \"{}\" "
			"xv 41 yv 12 num 3 19 "
			"if 26 xv 208 yv 8 pic 26 endif "
			"xv 117 yv 28 cstring \"{}\" "
			"xv 280 yv 12 num 3 21 "),
			total[0],
			total[1]);
	}
	else
	{
		fmt::format_to(std::back_inserter(string),
			FMT_STRING("if 25 xv -32 yv 8 pic 25 endif "
			"xv 0 yv 28 string \"{:4}/{:<3}\" "
			"xv 58 yv 12 num 2 19 "
			"if 26 xv 208 yv 8 pic 26 endif "
			"xv 240 yv 28 string \"{:4}/{:<3}\" "
			"xv 296 yv 12 num 2 21 "),
			totalscore[0], total[0],
			totalscore[1], total[1]);
	}

	for (i = 0; i < 16; i++)
	{
		if (i >= total[0] && i >= total[1])
			break; // we're done

		// left side
		if (i < total[0])
		{
			cl = &game.clients[sorted[0][i]];
			cl_ent = g_edicts + 1 + sorted[0][i];

			std::string_view entry = G_Fmt("ctf -40 {} {} {} {} {} ",
						42 + i * 8,
						sorted[0][i],
						cl->resp.score,
						cl->ping > 999 ? 999 : cl->ping,
						cl_ent->client->pers.inventory[IT_FLAG2] ? "sbfctf2" : "\"\"");

			if (string.size() + entry.size() < MAX_CTF_STAT_LENGTH)
			{
				string += entry;
				last[0] = i;
			}
		}

		// right side
		if (i < total[1])
		{
			cl = &game.clients[sorted[1][i]];
			cl_ent = g_edicts + 1 + sorted[1][i];

			std::string_view entry = G_Fmt("ctf 200 {} {} {} {} {} ",
						42 + i * 8,
						sorted[1][i],
						cl->resp.score,
						cl->ping > 999 ? 999 : cl->ping,
						cl_ent->client->pers.inventory[IT_FLAG1] ? "sbfctf1" : "\"\"");
			
			if (string.size() + entry.size() < MAX_CTF_STAT_LENGTH)
			{
				string += entry;
				last[1] = i;
			}
		}
	}

	// put in spectators if we have enough room
	if (last[0] > last[1])
		j = last[0];
	else
		j = last[1];
	j = (j + 2) * 8 + 42;

	k = n = 0;
	if (string.size() < MAX_CTF_STAT_LENGTH - 50)
	{
		for (i = 0; i < game.maxclients; i++)
		{
			cl_ent = g_edicts + 1 + i;
			cl = &game.clients[i];
			if (!cl_ent->inuse ||
				cl_ent->solid != SOLID_NOT ||
				cl_ent->client->resp.ctf_team != CTF_NOTEAM)
				continue;

			if (!k)
			{
				k = 1;
				fmt::format_to(std::back_inserter(string), FMT_STRING("xv 0 yv {} loc_string2 0 \"$g_pc_spectators\" "), j);
				j += 8;
			}

			std::string_view entry = G_Fmt("ctf {} {} {} {} {} \"\" ",
						(n & 1) ? 200 : -40, // x
						j,				   // y
						i,				   // playernum
						cl->resp.score,
						cl->ping > 999 ? 999 : cl->ping);

			if (string.size() + entry.size() < MAX_CTF_STAT_LENGTH)
				string += entry;

			if (n & 1)
				j += 8;
			n++;
		}
	}

	if (total[0] - last[0] > 1) // couldn't fit everyone
		fmt::format_to(std::back_inserter(string), FMT_STRING("xv -32 yv {} loc_string 1 $g_ctf_and_more {} "),
					42 + (last[0] + 1) * 8, total[0] - last[0] - 1);
	if (total[1] - last[1] > 1) // couldn't fit everyone
		fmt::format_to(std::back_inserter(string), FMT_STRING("xv 208 yv {} loc_string 1 $g_ctf_and_more {} "),
					42 + (last[1] + 1) * 8, total[1] - last[1] - 1);

	if (level.intermissiontime)
		fmt::format_to(std::back_inserter(string), FMT_STRING("ifgef {} yb -48 xv 0 loc_cstring2 0 \"$m_eou_press_button\" endif "), (level.intermission_server_frame + (5_sec).frames()));

	gi.WriteByte(svc_layout);
	gi.WriteString(string.c_str());
}

/*------------------------------------------------------------------------*/
/* TECH																	  */
/*------------------------------------------------------------------------*/

void CTFHasTech(edict_t *who)
{
	if (level.time - who->client->ctf_lasttechmsg > 2_sec)
	{
		gi.LocCenter_Print(who, "$g_already_have_tech");
		who->client->ctf_lasttechmsg = level.time;
	}
}

gitem_t *CTFWhat_Tech(edict_t *ent)
{
	int i;

	i = 0;
	for (; i < q_countof(tech_ids); i++)
	{
		if (ent->client->pers.inventory[tech_ids[i]])
		{
			return GetItemByIndex(tech_ids[i]);
		}
	}
	return nullptr;
}

bool CTFPickup_Tech(edict_t *ent, edict_t *other)
{
	int i;

	i = 0;
	for (; i < q_countof(tech_ids); i++)
	{
		if (other->client->pers.inventory[tech_ids[i]])
		{
			CTFHasTech(other);
			return false; // has this one
		}
	}

	// client only gets one tech
	other->client->pers.inventory[ent->item->id]++;
	other->client->ctf_regentime = level.time;
	return true;
}

static void SpawnTech(gitem_t *item, edict_t *spot);

static edict_t *FindTechSpawn()
{
	return SelectDeathmatchSpawnPoint(false, true, true).spot;
}

THINK(TechThink) (edict_t *tech) -> void
{
	edict_t *spot;

	if ((spot = FindTechSpawn()) != nullptr)
	{
		SpawnTech(tech->item, spot);
		G_FreeEdict(tech);
	}
	else
	{
		tech->nextthink = level.time + CTF_TECH_TIMEOUT;
		tech->think = TechThink;
	}
}

void CTFDrop_Tech(edict_t *ent, gitem_t *item)
{
	edict_t *tech;

	tech = Drop_Item(ent, item);
	tech->nextthink = level.time + CTF_TECH_TIMEOUT;
	tech->think = TechThink;
	ent->client->pers.inventory[item->id] = 0;
}

void CTFDeadDropTech(edict_t *ent)
{
	edict_t *dropped;
	int		 i;

	i = 0;
	for (; i < q_countof(tech_ids); i++)
	{
		if (ent->client->pers.inventory[tech_ids[i]])
		{
			dropped = Drop_Item(ent, GetItemByIndex(tech_ids[i]));
			// hack the velocity to make it bounce random
			dropped->velocity[0] = crandom_open() * 300;
			dropped->velocity[1] = crandom_open() * 300;
			dropped->nextthink = level.time + CTF_TECH_TIMEOUT;
			dropped->think = TechThink;
			dropped->owner = nullptr;
			ent->client->pers.inventory[tech_ids[i]] = 0;
		}
	}
}

static void SpawnTech(gitem_t *item, edict_t *spot)
{
	edict_t *ent;
	vec3_t	 forward, right;
	vec3_t	 angles;

	ent = G_Spawn();

	ent->classname = item->classname;
	ent->item = item;
	ent->spawnflags = SPAWNFLAG_ITEM_DROPPED;
	ent->s.effects = item->world_model_flags;
	ent->s.renderfx = RF_GLOW | RF_NO_LOD;
	ent->mins = { -15, -15, -15 };
	ent->maxs = { 15, 15, 15 };
	gi.setmodel(ent, ent->item->world_model);
	ent->solid = SOLID_TRIGGER;
	ent->movetype = MOVETYPE_TOSS;
	ent->touch = Touch_Item;
	ent->owner = ent;

	angles[0] = 0;
	angles[1] = (float) irandom(360);
	angles[2] = 0;

	AngleVectors(angles, forward, right, nullptr);
	ent->s.origin = spot->s.origin;
	ent->s.origin[2] += 16;
	ent->velocity = forward * 100;
	ent->velocity[2] = 300;

	ent->nextthink = level.time + CTF_TECH_TIMEOUT;
	ent->think = TechThink;

	gi.linkentity(ent);
}

THINK(SpawnTechs) (edict_t *ent) -> void
{
	edict_t *spot;
	int		 i;

	i = 0;
	for (; i < q_countof(tech_ids); i++)
	{
		if ((spot = FindTechSpawn()) != nullptr)
			SpawnTech(GetItemByIndex(tech_ids[i]), spot);
	}
	if (ent)
		G_FreeEdict(ent);
}

// frees the passed edict!
void CTFRespawnTech(edict_t *ent)
{
	edict_t *spot;

	if ((spot = FindTechSpawn()) != nullptr)
		SpawnTech(ent->item, spot);
	G_FreeEdict(ent);
}

void CTFSetupTechSpawn()
{
	edict_t *ent;
	bool techs_allowed;

	// [Paril-KEX]
	if (!strcmp(g_allow_techs->string, "auto"))
		techs_allowed = !!ctf->integer;
	else
		techs_allowed = !!g_allow_techs->integer;

	if (!techs_allowed)
		return;

	ent = G_Spawn();
	ent->nextthink = level.time + 2_sec;
	ent->think = SpawnTechs;
}

void CTFResetTech()
{
	edict_t *ent;
	uint32_t i;

	for (ent = g_edicts + 1, i = 1; i < globals.num_edicts; i++, ent++)
	{
		if (ent->inuse)
			if (ent->item && (ent->item->flags & IF_TECH))
				G_FreeEdict(ent);
	}
	SpawnTechs(nullptr);
}

int CTFApplyResistance(edict_t *ent, int dmg)
{
	float volume = 1.0;

	if (ent->client && ent->client->silencer_shots)
		volume = 0.2f;

	if (dmg && ent->client && ent->client->pers.inventory[IT_TECH_RESISTANCE])
	{
		// make noise
		gi.sound(ent, CHAN_AUX, gi.soundindex("ctf/tech1.wav"), volume, ATTN_NORM, 0);
		return dmg / 2;
	}
	return dmg;
}

int CTFApplyStrength(edict_t *ent, int dmg)
{
	if (dmg && ent->client && ent->client->pers.inventory[IT_TECH_STRENGTH])
	{
		return dmg * 2;
	}
	return dmg;
}

bool CTFApplyStrengthSound(edict_t *ent)
{
	float volume = 1.0;

	if (ent->client && ent->client->silencer_shots)
		volume = 0.2f;

	if (ent->client &&
		ent->client->pers.inventory[IT_TECH_STRENGTH])
	{
		if (ent->client->ctf_techsndtime < level.time)
		{
			ent->client->ctf_techsndtime = level.time + 1_sec;
			if (ent->client->quad_time > level.time)
				gi.sound(ent, CHAN_AUX, gi.soundindex("ctf/tech2x.wav"), volume, ATTN_NORM, 0);
			else
				gi.sound(ent, CHAN_AUX, gi.soundindex("ctf/tech2.wav"), volume, ATTN_NORM, 0);
		}
		return true;
	}
	return false;
}

bool CTFApplyHaste(edict_t *ent)
{
	if (ent->client &&
		ent->client->pers.inventory[IT_TECH_HASTE])
		return true;
	return false;
}

void CTFApplyHasteSound(edict_t *ent)
{
	float volume = 1.0;

	if (ent->client && ent->client->silencer_shots)
		volume = 0.2f;

	if (ent->client &&
		ent->client->pers.inventory[IT_TECH_HASTE] &&
		ent->client->ctf_techsndtime < level.time)
	{
		ent->client->ctf_techsndtime = level.time + 1_sec;
		gi.sound(ent, CHAN_AUX, gi.soundindex("ctf/tech3.wav"), volume, ATTN_NORM, 0);
	}
}

void CTFApplyRegeneration(edict_t *ent)
{
	bool	   noise = false;
	gclient_t *client;
	int		   index;
	float	   volume = 1.0;

	client = ent->client;
	if (!client)
		return;

	if (ent->client->silencer_shots)
		volume = 0.2f;

	if (client->pers.inventory[IT_TECH_REGENERATION])
	{
		if (client->ctf_regentime < level.time)
		{
			client->ctf_regentime = level.time;
			if (ent->health < 150)
			{
				ent->health += 5;
				if (ent->health > 150)
					ent->health = 150;
				client->ctf_regentime += 500_ms;
				noise = true;
			}
			index = ArmorIndex(ent);
			if (index && client->pers.inventory[index] < 150)
			{
				client->pers.inventory[index] += 5;
				if (client->pers.inventory[index] > 150)
					client->pers.inventory[index] = 150;
				client->ctf_regentime += 500_ms;
				noise = true;
			}
		}
		if (noise && ent->client->ctf_techsndtime < level.time)
		{
			ent->client->ctf_techsndtime = level.time + 1_sec;
			gi.sound(ent, CHAN_AUX, gi.soundindex("ctf/tech4.wav"), volume, ATTN_NORM, 0);
		}
	}
}

bool CTFHasRegeneration(edict_t *ent)
{
	if (ent->client &&
		ent->client->pers.inventory[IT_TECH_REGENERATION])
		return true;
	return false;
}

void CTFSay_Team(edict_t *who, const char *msg_in)
{
	edict_t *cl_ent;
	char outmsg[256];

	if (CheckFlood(who))
		return;

	Q_strlcpy(outmsg, msg_in, sizeof(outmsg));

	char *msg = outmsg;

	if (*msg == '\"')
	{
		msg[strlen(msg) - 1] = 0;
		msg++;
	}

	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse)
			continue;
		if (cl_ent->client->resp.ctf_team == who->client->resp.ctf_team)
			gi.LocClient_Print(cl_ent, PRINT_CHAT, "({}): {}\n",
					   who->client->pers.netname, msg);
	}
}

/*-----------------------------------------------------------------------*/
/*QUAKED misc_ctf_banner (1 .5 0) (-4 -64 0) (4 64 248) TEAM2
The origin is the bottom of the banner.
The banner is 248 tall.
*/
THINK(misc_ctf_banner_think) (edict_t *ent) -> void
{
	ent->s.frame = (ent->s.frame + 1) % 16;
	ent->nextthink = level.time + 10_hz;
}

constexpr spawnflags_t SPAWNFLAG_CTF_BANNER_BLUE = 1_spawnflag;

void SP_misc_ctf_banner(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/ctf/banner/tris.md2");
	if (ent->spawnflags.has(SPAWNFLAG_CTF_BANNER_BLUE)) // team2
		ent->s.skinnum = 1;

	ent->s.frame = irandom(16);
	gi.linkentity(ent);

	ent->think = misc_ctf_banner_think;
	ent->nextthink = level.time + 10_hz;
}

/*QUAKED misc_ctf_small_banner (1 .5 0) (-4 -32 0) (4 32 124) TEAM2
The origin is the bottom of the banner.
The banner is 124 tall.
*/
void SP_misc_ctf_small_banner(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/ctf/banner/small.md2");
	if (ent->spawnflags.has(SPAWNFLAG_CTF_BANNER_BLUE)) // team2
		ent->s.skinnum = 1;

	ent->s.frame = irandom(16);
	gi.linkentity(ent);

	ent->think = misc_ctf_banner_think;
	ent->nextthink = level.time + 10_hz;
}

/*-----------------------------------------------------------------------*/

static void SetGameName(pmenu_t *p)
{
	if (ctf->integer)
		Q_strlcpy(p->text, "$g_pc_3wctf", sizeof(p->text));
	else
		Q_strlcpy(p->text, "$g_pc_teamplay", sizeof(p->text));
}

static void SetLevelName(pmenu_t *p)
{
	static char levelname[33];

	levelname[0] = '*';
	if (g_edicts[0].message)
		Q_strlcpy(levelname + 1, g_edicts[0].message, sizeof(levelname) - 1);
	else
		Q_strlcpy(levelname + 1, level.mapname, sizeof(levelname) - 1);
	levelname[sizeof(levelname) - 1] = 0;
	Q_strlcpy(p->text, levelname, sizeof(p->text));
}

/*-----------------------------------------------------------------------*/

/* ELECTIONS */

bool CTFBeginElection(edict_t *ent, elect_t type, const char *msg)
{
	int		 count;
	edict_t *e;

	if (electpercentage->value == 0)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "Elections are disabled, only an admin can process this action.\n");
		return false;
	}

	if (ctfgame.election != ELECT_NONE)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "Election already in progress.\n");
		return false;
	}

	// clear votes
	count = 0;
	for (uint32_t i = 1; i <= game.maxclients; i++)
	{
		e = g_edicts + i;
		e->client->resp.voted = false;
		if (e->inuse)
			count++;
	}

	if (count < 2)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "Not enough players for election.\n");
		return false;
	}

	ctfgame.etarget = ent;
	ctfgame.election = type;
	ctfgame.evotes = 0;
	ctfgame.needvotes = (int) ((count * electpercentage->value) / 100);
	ctfgame.electtime = level.time + 20_sec; // twenty seconds for election
	Q_strlcpy(ctfgame.emsg, msg, sizeof(ctfgame.emsg));

	// tell everyone
	gi.Broadcast_Print(PRINT_CHAT, ctfgame.emsg);
	gi.LocBroadcast_Print(PRINT_HIGH, "Type YES or NO to vote on this request.\n");
	gi.LocBroadcast_Print(PRINT_HIGH, "Votes: {}  Needed: {}  Time left: {}s\n", ctfgame.evotes, ctfgame.needvotes,
			   (ctfgame.electtime - level.time).seconds<int>());

	return true;
}

void DoRespawn(edict_t *ent);

void CTFResetAllPlayers()
{
	uint32_t i;
	edict_t *ent;

	for (i = 1; i <= game.maxclients; i++)
	{
		ent = g_edicts + i;
		if (!ent->inuse)
			continue;

		if (ent->client->menu)
			PMenu_Close(ent);

		CTFPlayerResetGrapple(ent);
		CTFDeadDropFlag(ent);
		CTFDeadDropTech(ent);

		ent->client->resp.ctf_team = CTF_NOTEAM;
		ent->client->resp.ready = false;

		ent->svflags = SVF_NONE;
		ent->flags &= ~FL_GODMODE;
		PutClientInServer(ent);
	}

	// reset the level
	CTFResetTech();
	CTFResetFlags();

	for (ent = g_edicts + 1, i = 1; i < globals.num_edicts; i++, ent++)
	{
		if (ent->inuse && !ent->client)
		{
			if (ent->solid == SOLID_NOT && ent->think == DoRespawn &&
				ent->nextthink >= level.time)
			{
				ent->nextthink = 0_ms;
				DoRespawn(ent);
			}
		}
	}
	if (ctfgame.match == MATCH_SETUP)
		ctfgame.matchtime = level.time + gtime_t::from_min(matchsetuptime->value);
}

void CTFAssignGhost(edict_t *ent)
{
	int ghost, i;

	for (ghost = 0; ghost < MAX_CLIENTS; ghost++)
		if (!ctfgame.ghosts[ghost].code)
			break;
	if (ghost == MAX_CLIENTS)
		return;
	ctfgame.ghosts[ghost].team = ent->client->resp.ctf_team;
	ctfgame.ghosts[ghost].score = 0;
	for (;;)
	{
		ctfgame.ghosts[ghost].code = irandom(10000, 100000);
		for (i = 0; i < MAX_CLIENTS; i++)
			if (i != ghost && ctfgame.ghosts[i].code == ctfgame.ghosts[ghost].code)
				break;
		if (i == MAX_CLIENTS)
			break;
	}
	ctfgame.ghosts[ghost].ent = ent;
	Q_strlcpy(ctfgame.ghosts[ghost].netname, ent->client->pers.netname, sizeof(ctfgame.ghosts[ghost].netname));
	ent->client->resp.ghost = ctfgame.ghosts + ghost;
	gi.LocClient_Print(ent, PRINT_CHAT, "Your ghost code is **** {} ****\n", ctfgame.ghosts[ghost].code);
	gi.LocClient_Print(ent, PRINT_HIGH, "If you lose connection, you can rejoin with your score intact by typing \"ghost {}\".\n",
			   ctfgame.ghosts[ghost].code);
}

// start a match
void CTFStartMatch()
{
	edict_t *ent;

	ctfgame.match = MATCH_GAME;
	ctfgame.matchtime = level.time + gtime_t::from_min(matchtime->value);
	ctfgame.countdown = false;

	ctfgame.team1 = ctfgame.team2 = 0;

	memset(ctfgame.ghosts, 0, sizeof(ctfgame.ghosts));

	for (uint32_t i = 1; i <= game.maxclients; i++)
	{
		ent = g_edicts + i;
		if (!ent->inuse)
			continue;

		ent->client->resp.score = 0;
		ent->client->resp.ctf_state = 0;
		ent->client->resp.ghost = nullptr;

		gi.LocCenter_Print(ent, "******************\n\nMATCH HAS STARTED!\n\n******************");

		if (ent->client->resp.ctf_team != CTF_NOTEAM)
		{
			// make up a ghost code
			CTFAssignGhost(ent);
			CTFPlayerResetGrapple(ent);
			ent->svflags = SVF_NOCLIENT;
			ent->flags &= ~FL_GODMODE;

			ent->client->respawn_time = level.time + random_time(1_sec, 4_sec);
			ent->client->ps.pmove.pm_type = PM_DEAD;
			ent->client->anim_priority = ANIM_DEATH;
			ent->s.frame = FRAME_death308 - 1;
			ent->client->anim_end = FRAME_death308;
			ent->deadflag = true;
			ent->movetype = MOVETYPE_NOCLIP;
			ent->client->ps.gunindex = 0;
			ent->client->ps.gunskin = 0;
			gi.linkentity(ent);
		}
	}
}

void CTFEndMatch()
{
	ctfgame.match = MATCH_POST;
	gi.LocBroadcast_Print(PRINT_CHAT, "MATCH COMPLETED!\n");

	CTFCalcScores();

	gi.LocBroadcast_Print(PRINT_HIGH, "RED TEAM:  {} captures, {} points\n",
			   ctfgame.team1, ctfgame.total1);
	gi.LocBroadcast_Print(PRINT_HIGH, "BLUE TEAM:  {} captures, {} points\n",
			   ctfgame.team2, ctfgame.total2);

	if (ctfgame.team1 > ctfgame.team2)
		gi.LocBroadcast_Print(PRINT_CHAT, "$g_ctf_red_wins_caps",
				   ctfgame.team1 - ctfgame.team2);
	else if (ctfgame.team2 > ctfgame.team1)
		gi.LocBroadcast_Print(PRINT_CHAT, "$g_ctf_blue_wins_caps",
				   ctfgame.team2 - ctfgame.team1);
	else if (ctfgame.total1 > ctfgame.total2) // frag tie breaker
		gi.LocBroadcast_Print(PRINT_CHAT, "$g_ctf_red_wins_points",
				   ctfgame.total1 - ctfgame.total2);
	else if (ctfgame.total2 > ctfgame.total1)
		gi.LocBroadcast_Print(PRINT_CHAT, "$g_ctf_blue_wins_points",
				   ctfgame.total2 - ctfgame.total1);
	else
		gi.LocBroadcast_Print(PRINT_CHAT, "$g_ctf_tie_game");

	EndDMLevel();
}

bool CTFNextMap()
{
	if (ctfgame.match == MATCH_POST)
	{
		ctfgame.match = MATCH_SETUP;
		CTFResetAllPlayers();
		return true;
	}
	return false;
}

void CTFWinElection()
{
	switch (ctfgame.election)
	{
	case ELECT_MATCH:
		// reset into match mode
		if (competition->integer < 3)
			gi.cvar_set("competition", "2");
		ctfgame.match = MATCH_SETUP;
		CTFResetAllPlayers();
		break;

	case ELECT_ADMIN:
		ctfgame.etarget->client->resp.admin = true;
		gi.LocBroadcast_Print(PRINT_HIGH, "{} has become an admin.\n", ctfgame.etarget->client->pers.netname);
		gi.LocClient_Print(ctfgame.etarget, PRINT_HIGH, "Type 'admin' to access the adminstration menu.\n");
		break;

	case ELECT_MAP:
		gi.LocBroadcast_Print(PRINT_HIGH, "{} is warping to level {}.\n",
				   ctfgame.etarget->client->pers.netname, ctfgame.elevel);
		Q_strlcpy(level.forcemap, ctfgame.elevel, sizeof(level.forcemap));
		EndDMLevel();
		break;

	default:
		break;
	}
	ctfgame.election = ELECT_NONE;
}

void CTFVoteYes(edict_t *ent)
{
	if (ctfgame.election == ELECT_NONE)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "No election is in progress.\n");
		return;
	}
	if (ent->client->resp.voted)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "You already voted.\n");
		return;
	}
	if (ctfgame.etarget == ent)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "You can't vote for yourself.\n");
		return;
	}

	ent->client->resp.voted = true;

	ctfgame.evotes++;
	if (ctfgame.evotes == ctfgame.needvotes)
	{
		// the election has been won
		CTFWinElection();
		return;
	}
	gi.LocBroadcast_Print(PRINT_HIGH, "{}\n", ctfgame.emsg);
	gi.LocBroadcast_Print(PRINT_CHAT, "Votes: {}  Needed: {}  Time left: {}s\n", ctfgame.evotes, ctfgame.needvotes,
			   (ctfgame.electtime - level.time).seconds<int>());
}

void CTFVoteNo(edict_t *ent)
{
	if (ctfgame.election == ELECT_NONE)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "No election is in progress.\n");
		return;
	}
	if (ent->client->resp.voted)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "You already voted.\n");
		return;
	}
	if (ctfgame.etarget == ent)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "You can't vote for yourself.\n");
		return;
	}

	ent->client->resp.voted = true;

	gi.LocBroadcast_Print(PRINT_HIGH, "{}\n", ctfgame.emsg);
	gi.LocBroadcast_Print(PRINT_CHAT, "Votes: {}  Needed: {}  Time left: {}s\n", ctfgame.evotes, ctfgame.needvotes,
			   (ctfgame.electtime - level.time).seconds<int>());
}

void CTFReady(edict_t *ent)
{
	uint32_t i, j;
	edict_t *e;
	uint32_t t1, t2;

	if (ent->client->resp.ctf_team == CTF_NOTEAM)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "Pick a team first (hit <TAB> for menu)\n");
		return;
	}

	if (ctfgame.match != MATCH_SETUP)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "A match is not being setup.\n");
		return;
	}

	if (ent->client->resp.ready)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "You have already commited.\n");
		return;
	}

	ent->client->resp.ready = true;
	gi.LocBroadcast_Print(PRINT_HIGH, "{} is ready.\n", ent->client->pers.netname);

	t1 = t2 = 0;
	for (j = 0, i = 1; i <= game.maxclients; i++)
	{
		e = g_edicts + i;
		if (!e->inuse)
			continue;
		if (e->client->resp.ctf_team != CTF_NOTEAM && !e->client->resp.ready)
			j++;
		if (e->client->resp.ctf_team == CTF_TEAM1)
			t1++;
		else if (e->client->resp.ctf_team == CTF_TEAM2)
			t2++;
	}
	if (!j && t1 && t2)
	{
		// everyone has commited
		gi.LocBroadcast_Print(PRINT_CHAT, "All players have committed.  Match starting\n");
		ctfgame.match = MATCH_PREGAME;
		ctfgame.matchtime = level.time + gtime_t::from_sec(matchstarttime->value);
		ctfgame.countdown = false;
		gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("misc/talk1.wav"), 1, ATTN_NONE, 0);
	}
}

void CTFNotReady(edict_t *ent)
{
	if (ent->client->resp.ctf_team == CTF_NOTEAM)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "Pick a team first (hit <TAB> for menu)\n");
		return;
	}

	if (ctfgame.match != MATCH_SETUP && ctfgame.match != MATCH_PREGAME)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "A match is not being setup.\n");
		return;
	}

	if (!ent->client->resp.ready)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "You haven't commited.\n");
		return;
	}

	ent->client->resp.ready = false;
	gi.LocBroadcast_Print(PRINT_HIGH, "{} is no longer ready.\n", ent->client->pers.netname);

	if (ctfgame.match == MATCH_PREGAME)
	{
		gi.LocBroadcast_Print(PRINT_CHAT, "Match halted.\n");
		ctfgame.match = MATCH_SETUP;
		ctfgame.matchtime = level.time + gtime_t::from_min(matchsetuptime->value);
	}
}

void CTFGhost(edict_t *ent)
{
	int i;
	int n;

	if (gi.argc() < 2)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "Usage:  ghost <code>\n");
		return;
	}

	if (ent->client->resp.ctf_team != CTF_NOTEAM)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "You are already in the game.\n");
		return;
	}
	if (ctfgame.match != MATCH_GAME)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "No match is in progress.\n");
		return;
	}

	n = atoi(gi.argv(1));

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (ctfgame.ghosts[i].code && ctfgame.ghosts[i].code == n)
		{
			gi.LocClient_Print(ent, PRINT_HIGH, "Ghost code accepted, your position has been reinstated.\n");
			ctfgame.ghosts[i].ent->client->resp.ghost = nullptr;
			ent->client->resp.ctf_team = ctfgame.ghosts[i].team;
			ent->client->resp.ghost = ctfgame.ghosts + i;
			ent->client->resp.score = ctfgame.ghosts[i].score;
			ent->client->resp.ctf_state = 0;
			ctfgame.ghosts[i].ent = ent;
			ent->svflags = SVF_NONE;
			ent->flags &= ~FL_GODMODE;
			PutClientInServer(ent);
			gi.LocBroadcast_Print(PRINT_HIGH, "{} has been reinstated to {} team.\n",
					   ent->client->pers.netname, CTFTeamName(ent->client->resp.ctf_team));
			return;
		}
	}
	gi.LocClient_Print(ent, PRINT_HIGH, "Invalid ghost code.\n");
}

bool CTFMatchSetup()
{
	if (ctfgame.match == MATCH_SETUP || ctfgame.match == MATCH_PREGAME)
		return true;
	return false;
}

bool CTFMatchOn()
{
	if (ctfgame.match == MATCH_GAME)
		return true;
	return false;
}

/*-----------------------------------------------------------------------*/

void CTFJoinTeam1(edict_t *ent, pmenuhnd_t *p);
void CTFJoinTeam2(edict_t *ent, pmenuhnd_t *p);
void CTFReturnToMain(edict_t *ent, pmenuhnd_t *p);
void CTFChaseCam(edict_t *ent, pmenuhnd_t *p);

static const int jmenu_level = 1;
static const int jmenu_match = 2;
static const int jmenu_red = 4;
static const int jmenu_blue = 7;
static const int jmenu_chase = 10;
static const int jmenu_reqmatch = 12;

const pmenu_t joinmenu[] = {
	{ "*$g_pc_3wctf", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "$g_pc_join_red_team", PMENU_ALIGN_LEFT, CTFJoinTeam1 },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "$g_pc_join_blue_team", PMENU_ALIGN_LEFT, CTFJoinTeam2 },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "$g_pc_chase_camera", PMENU_ALIGN_LEFT, CTFChaseCam },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
};

const pmenu_t nochasemenu[] = {
	{ "$g_pc_3wctf", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "$g_pc_no_chase", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "$g_pc_return", PMENU_ALIGN_LEFT, CTFReturnToMain }
};

void CTFJoinTeam(edict_t *ent, ctfteam_t desired_team)
{
	PMenu_Close(ent);

	ent->svflags &= ~SVF_NOCLIENT;
	ent->client->resp.ctf_team = desired_team;
	ent->client->resp.ctf_state = 0;
	char value[MAX_INFO_VALUE] = { 0 };
	gi.Info_ValueForKey(ent->client->pers.userinfo, "skin", value, sizeof(value));
	CTFAssignSkin(ent, value);

	// assign a ghost if we are in match mode
	if (ctfgame.match == MATCH_GAME)
	{
		if (ent->client->resp.ghost)
			ent->client->resp.ghost->code = 0;
		ent->client->resp.ghost = nullptr;
		CTFAssignGhost(ent);
	}

	PutClientInServer(ent);

	G_PostRespawn(ent);

	gi.LocBroadcast_Print(PRINT_HIGH, "$g_joined_team",
			   ent->client->pers.netname, CTFTeamName(desired_team));

	if (ctfgame.match == MATCH_SETUP)
	{
		gi.LocCenter_Print(ent, "Type \"ready\" in console to ready up.\n");
	}

	// if anybody has a menu open, update it immediately
	CTFDirtyTeamMenu();
}

void CTFJoinTeam1(edict_t *ent, pmenuhnd_t *p)
{
	CTFJoinTeam(ent, CTF_TEAM1);
}

void CTFJoinTeam2(edict_t *ent, pmenuhnd_t *p)
{
	CTFJoinTeam(ent, CTF_TEAM2);
}

static void CTFNoChaseCamUpdate(edict_t *ent)
{
	pmenu_t		*entries = ent->client->menu->entries;

	SetGameName(&entries[0]);
	SetLevelName(&entries[jmenu_level]);
}

void CTFChaseCam(edict_t *ent, pmenuhnd_t *p)
{
	edict_t *e;

	CTFJoinTeam(ent, CTF_NOTEAM);

	if (ent->client->chase_target)
	{
		ent->client->chase_target = nullptr;
		ent->client->ps.pmove.pm_flags &= ~(PMF_NO_POSITIONAL_PREDICTION | PMF_NO_ANGULAR_PREDICTION);
		PMenu_Close(ent);
		return;
	}

	for (uint32_t i = 1; i <= game.maxclients; i++)
	{
		e = g_edicts + i;
		if (e->inuse && e->solid != SOLID_NOT)
		{
			ent->client->chase_target = e;
			PMenu_Close(ent);
			ent->client->update_chase = true;
			return;
		}
	}

	PMenu_Close(ent);
	PMenu_Open(ent, nochasemenu, -1, sizeof(nochasemenu) / sizeof(pmenu_t), nullptr, CTFNoChaseCamUpdate);
}

void CTFReturnToMain(edict_t *ent, pmenuhnd_t *p)
{
	PMenu_Close(ent);
	CTFOpenJoinMenu(ent);
}

void CTFRequestMatch(edict_t *ent, pmenuhnd_t *p)
{
	PMenu_Close(ent);

	CTFBeginElection(ent, ELECT_MATCH, G_Fmt("{} has requested to switch to competition mode.\n",
				ent->client->pers.netname).data());
}

void DeathmatchScoreboard(edict_t *ent);

void CTFShowScores(edict_t *ent, pmenu_t *p)
{
	PMenu_Close(ent);

	ent->client->showscores = true;
	ent->client->showinventory = false;
	DeathmatchScoreboard(ent);
}

void CTFUpdateJoinMenu(edict_t *ent)
{
	pmenu_t		*entries = ent->client->menu->entries;

	SetGameName(entries);

	if (ctfgame.match >= MATCH_PREGAME && matchlock->integer)
	{
		Q_strlcpy(entries[jmenu_red].text, "MATCH IS LOCKED", sizeof(entries[jmenu_red].text));
		entries[jmenu_red].SelectFunc = nullptr;
		Q_strlcpy(entries[jmenu_blue].text, "  (entry is not permitted)", sizeof(entries[jmenu_blue].text));
		entries[jmenu_blue].SelectFunc = nullptr;
	}
	else
	{
		if (ctfgame.match >= MATCH_PREGAME)
		{
			Q_strlcpy(entries[jmenu_red].text, "Join Red MATCH Team", sizeof(entries[jmenu_red].text));
			Q_strlcpy(entries[jmenu_blue].text, "Join Blue MATCH Team", sizeof(entries[jmenu_blue].text));
		}
		else
		{
			Q_strlcpy(entries[jmenu_red].text, "$g_pc_join_red_team", sizeof(entries[jmenu_red].text));
			Q_strlcpy(entries[jmenu_blue].text, "$g_pc_join_blue_team", sizeof(entries[jmenu_blue].text));
		}
		entries[jmenu_red].SelectFunc = CTFJoinTeam1;
		entries[jmenu_blue].SelectFunc = CTFJoinTeam2;
	}

	// KEX_FIXME: what's this for?
	if (g_teamplay_force_join->string && *g_teamplay_force_join->string)
	{
		if (Q_strcasecmp(g_teamplay_force_join->string, "red") == 0)
		{
			entries[jmenu_blue].text[0] = '\0';
			entries[jmenu_blue].SelectFunc = nullptr;
		}
		else if (Q_strcasecmp(g_teamplay_force_join->string, "blue") == 0)
		{
			entries[jmenu_red].text[0] = '\0';
			entries[jmenu_red].SelectFunc = nullptr;
		}
	}

	if (ent->client->chase_target)
		Q_strlcpy(entries[jmenu_chase].text, "$g_pc_leave_chase_camera", sizeof(entries[jmenu_chase].text));
	else
		Q_strlcpy(entries[jmenu_chase].text, "$g_pc_chase_camera", sizeof(entries[jmenu_chase].text));

	SetLevelName(entries + jmenu_level);

	uint32_t num1 = 0, num2 = 0;
	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		if (!g_edicts[i + 1].inuse)
			continue;
		if (game.clients[i].resp.ctf_team == CTF_TEAM1)
			num1++;
		else if (game.clients[i].resp.ctf_team == CTF_TEAM2)
			num2++;
	}

	switch (ctfgame.match)
	{
	case MATCH_NONE:
		entries[jmenu_match].text[0] = '\0';
		break;

	case MATCH_SETUP:
		Q_strlcpy(entries[jmenu_match].text, "*MATCH SETUP IN PROGRESS", sizeof(entries[jmenu_match].text));
		break;

	case MATCH_PREGAME:
		Q_strlcpy(entries[jmenu_match].text, "*MATCH STARTING", sizeof(entries[jmenu_match].text));
		break;

	case MATCH_GAME:
		Q_strlcpy(entries[jmenu_match].text, "*MATCH IN PROGRESS", sizeof(entries[jmenu_match].text));
		break;

	default:
		break;
	}

	if (*entries[jmenu_red].text)
	{
		Q_strlcpy(entries[jmenu_red + 1].text, "$g_pc_playercount", sizeof(entries[jmenu_red + 1].text));
		G_FmtTo(entries[jmenu_red + 1].text_arg1, "{}", num1);
	}
	else
	{
		entries[jmenu_red + 1].text[0] = '\0';
		entries[jmenu_red + 1].text_arg1[0] = '\0';
	}
	if (*entries[jmenu_blue].text)
	{
		Q_strlcpy(entries[jmenu_blue + 1].text, "$g_pc_playercount", sizeof(entries[jmenu_blue + 1].text));
		G_FmtTo(entries[jmenu_blue + 1].text_arg1, "{}", num2);
	}
	else
	{
		entries[jmenu_blue + 1].text[0] = '\0';
		entries[jmenu_blue + 1].text_arg1[0] = '\0';
	}

	entries[jmenu_reqmatch].text[0] = '\0';
	entries[jmenu_reqmatch].SelectFunc = nullptr;
	if (competition->integer && ctfgame.match < MATCH_SETUP)
	{
		Q_strlcpy(entries[jmenu_reqmatch].text, "Request Match", sizeof(entries[jmenu_reqmatch].text));
		entries[jmenu_reqmatch].SelectFunc = CTFRequestMatch;
	}
}

void CTFOpenJoinMenu(edict_t *ent)
{
	uint32_t num1 = 0, num2 = 0;
	for (uint32_t i = 0; i < game.maxclients; i++)
	{
		if (!g_edicts[i + 1].inuse)
			continue;
		if (game.clients[i].resp.ctf_team == CTF_TEAM1)
			num1++;
		else if (game.clients[i].resp.ctf_team == CTF_TEAM2)
			num2++;
	}

	int team;

	if (num1 > num2)
		team = CTF_TEAM1;
	else if (num2 > num1)
		team = CTF_TEAM2;
	team = brandom() ? CTF_TEAM1 : CTF_TEAM2;

	PMenu_Open(ent, joinmenu, team, sizeof(joinmenu) / sizeof(pmenu_t), nullptr, CTFUpdateJoinMenu);
}

bool CTFStartClient(edict_t *ent)
{
	if (!G_TeamplayEnabled())
		return false;

	if (ent->client->resp.ctf_team != CTF_NOTEAM)
		return false;

	if ((!(ent->svflags & SVF_BOT) && !g_teamplay_force_join->integer) || ctfgame.match >= MATCH_SETUP)
	{
		// start as 'observer'
		ent->movetype = MOVETYPE_NOCLIP;
		ent->solid = SOLID_NOT;
		ent->svflags |= SVF_NOCLIENT;
		ent->client->resp.ctf_team = CTF_NOTEAM;
		ent->client->resp.spectator = true;
		ent->client->ps.gunindex = 0;
		ent->client->ps.gunskin = 0;
		gi.linkentity(ent);

		CTFOpenJoinMenu(ent);
		return true;
	}
	return false;
}

void CTFObserver(edict_t *ent)
{
	if (!G_TeamplayEnabled() || g_teamplay_force_join->integer)
		return;

	// start as 'observer'
	if (ent->movetype == MOVETYPE_NOCLIP)
		CTFPlayerResetGrapple(ent);

	CTFDeadDropFlag(ent);
	CTFDeadDropTech(ent);

	ent->deadflag = false;
	ent->movetype = MOVETYPE_NOCLIP;
	ent->solid = SOLID_NOT;
	ent->svflags |= SVF_NOCLIENT;
	ent->client->resp.ctf_team = CTF_NOTEAM;
	ent->client->ps.gunindex = 0;
	ent->client->ps.gunskin = 0;
	ent->client->resp.score = 0;
	PutClientInServer(ent);
}

bool CTFInMatch()
{
	if (ctfgame.match > MATCH_NONE)
		return true;
	return false;
}

bool CTFCheckRules()
{
	int		 t;
	uint32_t i, j;
	char	 text[64];
	edict_t *ent;

	if (ctfgame.election != ELECT_NONE && ctfgame.electtime <= level.time)
	{
		gi.LocBroadcast_Print(PRINT_CHAT, "Election timed out and has been cancelled.\n");
		ctfgame.election = ELECT_NONE;
	}

	if (ctfgame.match != MATCH_NONE)
	{
		t = (ctfgame.matchtime - level.time).seconds<int>();

		// no team warnings in match mode
		ctfgame.warnactive = 0;

		if (t <= 0)
		{ // time ended on something
			switch (ctfgame.match)
			{
			case MATCH_SETUP:
				// go back to normal mode
				if (competition->integer < 3)
				{
					ctfgame.match = MATCH_NONE;
					gi.cvar_set("competition", "1");
					CTFResetAllPlayers();
				}
				else
				{
					// reset the time
					ctfgame.matchtime = level.time + gtime_t::from_min(matchsetuptime->value);
				}
				return false;

			case MATCH_PREGAME:
				// match started!
				CTFStartMatch();
				gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("misc/tele_up.wav"), 1, ATTN_NONE, 0);
				return false;

			case MATCH_GAME:
				// match ended!
				CTFEndMatch();
				gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("misc/bigtele.wav"), 1, ATTN_NONE, 0);
				return false;

			default:
				break;
			}
		}

		if (t == ctfgame.lasttime)
			return false;

		ctfgame.lasttime = t;

		switch (ctfgame.match)
		{
		case MATCH_SETUP:
			for (j = 0, i = 1; i <= game.maxclients; i++)
			{
				ent = g_edicts + i;
				if (!ent->inuse)
					continue;
				if (ent->client->resp.ctf_team != CTF_NOTEAM &&
					!ent->client->resp.ready)
					j++;
			}

			if (competition->integer < 3)
				G_FmtTo(text, "{:02}:{:02} SETUP: {} not ready", t / 60, t % 60, j);
			else
				G_FmtTo(text, "SETUP: {} not ready", j);

			gi.configstring(CONFIG_CTF_MATCH, text);
			break;

		case MATCH_PREGAME:
			G_FmtTo(text, "{:02}:{:02} UNTIL START", t / 60, t % 60);
			gi.configstring(CONFIG_CTF_MATCH, text);

			if (t <= 10 && !ctfgame.countdown)
			{
				ctfgame.countdown = true;
				gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("world/10_0.wav"), 1, ATTN_NONE, 0);
			}
			break;

		case MATCH_GAME:
			G_FmtTo(text, "{:02}:{:02} MATCH", t / 60, t % 60);
			gi.configstring(CONFIG_CTF_MATCH, text);
			if (t <= 10 && !ctfgame.countdown)
			{
				ctfgame.countdown = true;
				gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("world/10_0.wav"), 1, ATTN_NONE, 0);
			}
			break;

		default:
			break;
		}
		return false;
	}
	else
	{
		int team1 = 0, team2 = 0;

		if (level.time == gtime_t::from_sec(ctfgame.lasttime))
			return false;
		ctfgame.lasttime = level.time.seconds<int>();
		// this is only done in non-match (public) mode

		if (warn_unbalanced->integer)
		{
			// count up the team totals
			for (i = 1; i <= game.maxclients; i++)
			{
				ent = g_edicts + i;
				if (!ent->inuse)
					continue;
				if (ent->client->resp.ctf_team == CTF_TEAM1)
					team1++;
				else if (ent->client->resp.ctf_team == CTF_TEAM2)
					team2++;
			}

			if (team1 - team2 >= 2 && team2 >= 2)
			{
				if (ctfgame.warnactive != CTF_TEAM1)
				{
					ctfgame.warnactive = CTF_TEAM1;
					gi.configstring(CONFIG_CTF_TEAMINFO, "WARNING: Red has too many players");
				}
			}
			else if (team2 - team1 >= 2 && team1 >= 2)
			{
				if (ctfgame.warnactive != CTF_TEAM2)
				{
					ctfgame.warnactive = CTF_TEAM2;
					gi.configstring(CONFIG_CTF_TEAMINFO, "WARNING: Blue has too many players");
				}
			}
			else
				ctfgame.warnactive = 0;
		}
		else
			ctfgame.warnactive = 0;
	}

	if (capturelimit->integer &&
		(ctfgame.team1 >= capturelimit->integer ||
		 ctfgame.team2 >= capturelimit->integer))
	{
		gi.LocBroadcast_Print(PRINT_HIGH, "$g_capturelimit_hit");
		return true;
	}
	return false;
}

/*--------------------------------------------------------------------------
 * just here to help old map conversions
 *--------------------------------------------------------------------------*/

TOUCH(old_teleporter_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	edict_t *dest;
	vec3_t	 forward;

	if (!other->client)
		return;
	dest = G_PickTarget(self->target);
	if (!dest)
	{
		gi.Com_Print("Couldn't find destination\n");
		return;
	}

	// ZOID
	CTFPlayerResetGrapple(other);
	// ZOID

	// unlink to make sure it can't possibly interfere with KillBox
	gi.unlinkentity(other);

	other->s.origin = dest->s.origin;
	other->s.old_origin = dest->s.origin;
	//	other->s.origin[2] += 10;

	// clear the velocity and hold them in place briefly
	other->velocity = {};
	other->client->ps.pmove.pm_time = 160; // hold time
	other->client->ps.pmove.pm_flags |= PMF_TIME_TELEPORT;

	// draw the teleport splash at source and on the player
	self->enemy->s.event = EV_PLAYER_TELEPORT;
	other->s.event = EV_PLAYER_TELEPORT;

	// set angles
	other->client->ps.pmove.delta_angles = dest->s.angles - other->client->resp.cmd_angles;

	other->s.angles[PITCH] = 0;
	other->s.angles[YAW] = dest->s.angles[YAW];
	other->s.angles[ROLL] = 0;
	other->client->ps.viewangles = dest->s.angles;
	other->client->v_angle = dest->s.angles;

	// give a little forward velocity
	AngleVectors(other->client->v_angle, forward, nullptr, nullptr);
	other->velocity = forward * 200;

	gi.linkentity(other);

	// kill anything at the destination
	if (!KillBox(other, true))
	{
	}

	// [Paril-KEX] move sphere, if we own it
	if (other->client->owned_sphere)
	{
		edict_t *sphere = other->client->owned_sphere;
		sphere->s.origin = other->s.origin;
		sphere->s.origin[2] = other->absmax[2];
		sphere->s.angles[YAW] = other->s.angles[YAW];
		gi.linkentity(sphere);
	}
}

/*QUAKED trigger_ctf_teleport (0.5 0.5 0.5) ?
Players touching this will be teleported
*/
void SP_trigger_ctf_teleport(edict_t *ent)
{
	edict_t *s;
	int		 i;

	if (!ent->target)
	{
		gi.Com_Print("teleporter without a target.\n");
		G_FreeEdict(ent);
		return;
	}

	ent->svflags |= SVF_NOCLIENT;
	ent->solid = SOLID_TRIGGER;
	ent->touch = old_teleporter_touch;
	gi.setmodel(ent, ent->model);
	gi.linkentity(ent);

	// noise maker and splash effect dude
	s = G_Spawn();
	ent->enemy = s;
	for (i = 0; i < 3; i++)
		s->s.origin[i] = ent->mins[i] + (ent->maxs[i] - ent->mins[i]) / 2;
	s->s.sound = gi.soundindex("world/hum1.wav");
	gi.linkentity(s);
}

/*QUAKED info_ctf_teleport_destination (0.5 0.5 0.5) (-16 -16 -24) (16 16 32)
Point trigger_teleports at these.
*/
void SP_info_ctf_teleport_destination(edict_t *ent)
{
	ent->s.origin[2] += 16;
}

/*----------------------------------------------------------------------------------*/
/* ADMIN */

struct admin_settings_t
{
	int	 matchlen;
	int	 matchsetuplen;
	int	 matchstartlen;
	bool weaponsstay;
	bool instantitems;
	bool quaddrop;
	bool instantweap;
	bool matchlock;
};

void CTFAdmin_UpdateSettings(edict_t *ent, pmenuhnd_t *setmenu);
void CTFOpenAdminMenu(edict_t *ent);

void CTFAdmin_SettingsApply(edict_t *ent, pmenuhnd_t *p)
{
	admin_settings_t *settings = (admin_settings_t *) p->arg;

	if (settings->matchlen != matchtime->value)
	{
		gi.LocBroadcast_Print(PRINT_HIGH, "{} changed the match length to {} minutes.\n",
				   ent->client->pers.netname, settings->matchlen);
		if (ctfgame.match == MATCH_GAME)
		{
			// in the middle of a match, change it on the fly
			ctfgame.matchtime = (ctfgame.matchtime - gtime_t::from_min(matchtime->value)) + gtime_t::from_min(settings->matchlen);
		}
		;
		gi.cvar_set("matchtime", G_Fmt("{}", settings->matchlen).data());
	}

	if (settings->matchsetuplen != matchsetuptime->value)
	{
		gi.LocBroadcast_Print(PRINT_HIGH, "{} changed the match setup time to {} minutes.\n",
				   ent->client->pers.netname, settings->matchsetuplen);
		if (ctfgame.match == MATCH_SETUP)
		{
			// in the middle of a match, change it on the fly
			ctfgame.matchtime = (ctfgame.matchtime - gtime_t::from_min(matchsetuptime->value)) + gtime_t::from_min(settings->matchsetuplen);
		}
		;
		gi.cvar_set("matchsetuptime", G_Fmt("{}", settings->matchsetuplen).data());
	}

	if (settings->matchstartlen != matchstarttime->value)
	{
		gi.LocBroadcast_Print(PRINT_HIGH, "{} changed the match start time to {} seconds.\n",
				   ent->client->pers.netname, settings->matchstartlen);
		if (ctfgame.match == MATCH_PREGAME)
		{
			// in the middle of a match, change it on the fly
			ctfgame.matchtime = (ctfgame.matchtime - gtime_t::from_sec(matchstarttime->value)) + gtime_t::from_sec(settings->matchstartlen);
		}
		gi.cvar_set("matchstarttime", G_Fmt("{}", settings->matchstartlen).data());
	}

	if (settings->weaponsstay != !!g_dm_weapons_stay->integer)
	{
		gi.LocBroadcast_Print(PRINT_HIGH, "{} turned {} weapons stay.\n",
				   ent->client->pers.netname, settings->weaponsstay ? "on" : "off");
		gi.cvar_set("g_dm_weapons_stay", settings->weaponsstay ? "1" : "0");
	}

	if (settings->instantitems != !!g_dm_instant_items->integer)
	{
		gi.LocBroadcast_Print(PRINT_HIGH, "{} turned {} instant items.\n",
				   ent->client->pers.netname, settings->instantitems ? "on" : "off");
		gi.cvar_set("g_dm_instant_items", settings->instantitems ? "1" : "0");
	}

	if (settings->quaddrop != (bool) !g_dm_no_quad_drop->integer)
	{
		gi.LocBroadcast_Print(PRINT_HIGH, "{} turned {} quad drop.\n",
				   ent->client->pers.netname, settings->quaddrop ? "on" : "off");
		gi.cvar_set("g_dm_no_quad_drop", !settings->quaddrop ? "1" : "0");
	}

	if (settings->instantweap != !!g_instant_weapon_switch->integer)
	{
		gi.LocBroadcast_Print(PRINT_HIGH, "{} turned {} instant weapons.\n",
				   ent->client->pers.netname, settings->instantweap ? "on" : "off");
		gi.cvar_set("g_instant_weapon_switch", settings->instantweap ? "1" : "0");
	}

	if (settings->matchlock != !!matchlock->integer)
	{
		gi.LocBroadcast_Print(PRINT_HIGH, "{} turned {} match lock.\n",
				   ent->client->pers.netname, settings->matchlock ? "on" : "off");
		gi.cvar_set("matchlock", settings->matchlock ? "1" : "0");
	}

	PMenu_Close(ent);
	CTFOpenAdminMenu(ent);
}

void CTFAdmin_SettingsCancel(edict_t *ent, pmenuhnd_t *p)
{
	PMenu_Close(ent);
	CTFOpenAdminMenu(ent);
}

void CTFAdmin_ChangeMatchLen(edict_t *ent, pmenuhnd_t *p)
{
	admin_settings_t *settings = (admin_settings_t *) p->arg;

	settings->matchlen = (settings->matchlen % 60) + 5;
	if (settings->matchlen < 5)
		settings->matchlen = 5;

	CTFAdmin_UpdateSettings(ent, p);
}

void CTFAdmin_ChangeMatchSetupLen(edict_t *ent, pmenuhnd_t *p)
{
	admin_settings_t *settings = (admin_settings_t *) p->arg;

	settings->matchsetuplen = (settings->matchsetuplen % 60) + 5;
	if (settings->matchsetuplen < 5)
		settings->matchsetuplen = 5;

	CTFAdmin_UpdateSettings(ent, p);
}

void CTFAdmin_ChangeMatchStartLen(edict_t *ent, pmenuhnd_t *p)
{
	admin_settings_t *settings = (admin_settings_t *) p->arg;

	settings->matchstartlen = (settings->matchstartlen % 600) + 10;
	if (settings->matchstartlen < 20)
		settings->matchstartlen = 20;

	CTFAdmin_UpdateSettings(ent, p);
}

void CTFAdmin_ChangeWeapStay(edict_t *ent, pmenuhnd_t *p)
{
	admin_settings_t *settings = (admin_settings_t *) p->arg;

	settings->weaponsstay = !settings->weaponsstay;
	CTFAdmin_UpdateSettings(ent, p);
}

void CTFAdmin_ChangeInstantItems(edict_t *ent, pmenuhnd_t *p)
{
	admin_settings_t *settings = (admin_settings_t *) p->arg;

	settings->instantitems = !settings->instantitems;
	CTFAdmin_UpdateSettings(ent, p);
}

void CTFAdmin_ChangeQuadDrop(edict_t *ent, pmenuhnd_t *p)
{
	admin_settings_t *settings = (admin_settings_t *) p->arg;

	settings->quaddrop = !settings->quaddrop;
	CTFAdmin_UpdateSettings(ent, p);
}

void CTFAdmin_ChangeInstantWeap(edict_t *ent, pmenuhnd_t *p)
{
	admin_settings_t *settings = (admin_settings_t *) p->arg;

	settings->instantweap = !settings->instantweap;
	CTFAdmin_UpdateSettings(ent, p);
}

void CTFAdmin_ChangeMatchLock(edict_t *ent, pmenuhnd_t *p)
{
	admin_settings_t *settings = (admin_settings_t *) p->arg;

	settings->matchlock = !settings->matchlock;
	CTFAdmin_UpdateSettings(ent, p);
}

void CTFAdmin_UpdateSettings(edict_t *ent, pmenuhnd_t *setmenu)
{
	int				  i = 2;
	admin_settings_t *settings = (admin_settings_t *) setmenu->arg;

	PMenu_UpdateEntry(setmenu->entries + i, G_Fmt("Match Len:       {:2} mins", settings->matchlen).data(), PMENU_ALIGN_LEFT, CTFAdmin_ChangeMatchLen);
	i++;

	PMenu_UpdateEntry(setmenu->entries + i, G_Fmt("Match Setup Len: {:2} mins", settings->matchsetuplen).data(), PMENU_ALIGN_LEFT, CTFAdmin_ChangeMatchSetupLen);
	i++;

	PMenu_UpdateEntry(setmenu->entries + i, G_Fmt("Match Start Len: {:2} secs", settings->matchstartlen).data(), PMENU_ALIGN_LEFT, CTFAdmin_ChangeMatchStartLen);
	i++;

	PMenu_UpdateEntry(setmenu->entries + i, G_Fmt("Weapons Stay:    {}", settings->weaponsstay ? "Yes" : "No").data(), PMENU_ALIGN_LEFT, CTFAdmin_ChangeWeapStay);
	i++;

	PMenu_UpdateEntry(setmenu->entries + i, G_Fmt("Instant Items:   {}", settings->instantitems ? "Yes" : "No").data(), PMENU_ALIGN_LEFT, CTFAdmin_ChangeInstantItems);
	i++;

	PMenu_UpdateEntry(setmenu->entries + i, G_Fmt("Quad Drop:       {}", settings->quaddrop ? "Yes" : "No").data(), PMENU_ALIGN_LEFT, CTFAdmin_ChangeQuadDrop);
	i++;

	PMenu_UpdateEntry(setmenu->entries + i, G_Fmt("Instant Weapons: {}", settings->instantweap ? "Yes" : "No").data(), PMENU_ALIGN_LEFT, CTFAdmin_ChangeInstantWeap);
	i++;

	PMenu_UpdateEntry(setmenu->entries + i, G_Fmt("Match Lock:      {}", settings->matchlock ? "Yes" : "No").data(), PMENU_ALIGN_LEFT, CTFAdmin_ChangeMatchLock);
	i++;

	PMenu_Update(ent);
}

const pmenu_t def_setmenu[] = {
	{ "*Settings Menu", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr }, // int matchlen;
	{ "", PMENU_ALIGN_LEFT, nullptr }, // int matchsetuplen;
	{ "", PMENU_ALIGN_LEFT, nullptr }, // int matchstartlen;
	{ "", PMENU_ALIGN_LEFT, nullptr }, // bool weaponsstay;
	{ "", PMENU_ALIGN_LEFT, nullptr }, // bool instantitems;
	{ "", PMENU_ALIGN_LEFT, nullptr }, // bool quaddrop;
	{ "", PMENU_ALIGN_LEFT, nullptr }, // bool instantweap;
	{ "", PMENU_ALIGN_LEFT, nullptr }, // bool matchlock;
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "Apply", PMENU_ALIGN_LEFT, CTFAdmin_SettingsApply },
	{ "Cancel", PMENU_ALIGN_LEFT, CTFAdmin_SettingsCancel }
};

void CTFAdmin_Settings(edict_t *ent, pmenuhnd_t *p)
{
	admin_settings_t *settings;
	pmenuhnd_t	   *menu;

	PMenu_Close(ent);

	settings = (admin_settings_t *) gi.TagMalloc(sizeof(*settings), TAG_LEVEL);

	settings->matchlen = matchtime->integer;
	settings->matchsetuplen = matchsetuptime->integer;
	settings->matchstartlen = matchstarttime->integer;
	settings->weaponsstay = g_dm_weapons_stay->integer;
	settings->instantitems = g_dm_instant_items->integer;
	settings->quaddrop = !g_dm_no_quad_drop->integer;
	settings->instantweap = g_instant_weapon_switch->integer != 0;
	settings->matchlock = matchlock->integer != 0;

	menu = PMenu_Open(ent, def_setmenu, -1, sizeof(def_setmenu) / sizeof(pmenu_t), settings, nullptr);
	CTFAdmin_UpdateSettings(ent, menu);
}

void CTFAdmin_MatchSet(edict_t *ent, pmenuhnd_t *p)
{
	PMenu_Close(ent);

	if (ctfgame.match == MATCH_SETUP)
	{
		gi.LocBroadcast_Print(PRINT_CHAT, "Match has been forced to start.\n");
		ctfgame.match = MATCH_PREGAME;
		ctfgame.matchtime = level.time + gtime_t::from_sec(matchstarttime->value);
		gi.positioned_sound(world->s.origin, world, CHAN_AUTO | CHAN_RELIABLE, gi.soundindex("misc/talk1.wav"), 1, ATTN_NONE, 0);
		ctfgame.countdown = false;
	}
	else if (ctfgame.match == MATCH_GAME)
	{
		gi.LocBroadcast_Print(PRINT_CHAT, "Match has been forced to terminate.\n");
		ctfgame.match = MATCH_SETUP;
		ctfgame.matchtime = level.time + gtime_t::from_min(matchsetuptime->value);
		CTFResetAllPlayers();
	}
}

void CTFAdmin_MatchMode(edict_t *ent, pmenuhnd_t *p)
{
	PMenu_Close(ent);

	if (ctfgame.match != MATCH_SETUP)
	{
		if (competition->integer < 3)
			gi.cvar_set("competition", "2");
		ctfgame.match = MATCH_SETUP;
		CTFResetAllPlayers();
	}
}

void CTFAdmin_Reset(edict_t *ent, pmenuhnd_t *p)
{
	PMenu_Close(ent);

	// go back to normal mode
	gi.LocBroadcast_Print(PRINT_CHAT, "Match mode has been terminated, reseting to normal game.\n");
	ctfgame.match = MATCH_NONE;
	gi.cvar_set("competition", "1");
	CTFResetAllPlayers();
}

void CTFAdmin_Cancel(edict_t *ent, pmenuhnd_t *p)
{
	PMenu_Close(ent);
}

pmenu_t adminmenu[] = {
	{ "*Administration Menu", PMENU_ALIGN_CENTER, nullptr },
	{ "", PMENU_ALIGN_CENTER, nullptr }, // blank
	{ "Settings", PMENU_ALIGN_LEFT, CTFAdmin_Settings },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "", PMENU_ALIGN_LEFT, nullptr },
	{ "Cancel", PMENU_ALIGN_LEFT, CTFAdmin_Cancel },
	{ "", PMENU_ALIGN_CENTER, nullptr },
};

void CTFOpenAdminMenu(edict_t *ent)
{
	adminmenu[3].text[0] = '\0';
	adminmenu[3].SelectFunc = nullptr;
	adminmenu[4].text[0] = '\0';
	adminmenu[4].SelectFunc = nullptr;
	if (ctfgame.match == MATCH_SETUP)
	{
		Q_strlcpy(adminmenu[3].text, "Force start match", sizeof(adminmenu[3].text));
		adminmenu[3].SelectFunc = CTFAdmin_MatchSet;
		Q_strlcpy(adminmenu[4].text, "Reset to pickup mode", sizeof(adminmenu[4].text));
		adminmenu[4].SelectFunc = CTFAdmin_Reset;
	}
	else if (ctfgame.match == MATCH_GAME || ctfgame.match == MATCH_PREGAME)
	{
		Q_strlcpy(adminmenu[3].text, "Cancel match", sizeof(adminmenu[3].text));
		adminmenu[3].SelectFunc = CTFAdmin_MatchSet;
	}
	else if (ctfgame.match == MATCH_NONE && competition->integer)
	{
		Q_strlcpy(adminmenu[3].text, "Switch to match mode", sizeof(adminmenu[3].text));
		adminmenu[3].SelectFunc = CTFAdmin_MatchMode;
	}

	//	if (ent->client->menu)
	//		PMenu_Close(ent->client->menu);

	PMenu_Open(ent, adminmenu, -1, sizeof(adminmenu) / sizeof(pmenu_t), nullptr, nullptr);
}

void CTFAdmin(edict_t *ent)
{
	if (!allow_admin->integer)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "Administration is disabled\n");
		return;
	}

	if (gi.argc() > 1 && admin_password->string && *admin_password->string &&
		!ent->client->resp.admin && strcmp(admin_password->string, gi.argv(1)) == 0)
	{
		ent->client->resp.admin = true;
		gi.LocBroadcast_Print(PRINT_HIGH, "{} has become an admin.\n", ent->client->pers.netname);
		gi.LocClient_Print(ent, PRINT_HIGH, "Type 'admin' to access the adminstration menu.\n");
	}

	if (!ent->client->resp.admin)
	{
		CTFBeginElection(ent, ELECT_ADMIN, G_Fmt("{} has requested admin rights.\n",
					ent->client->pers.netname).data());
		return;
	}

	if (ent->client->menu)
		PMenu_Close(ent);

	CTFOpenAdminMenu(ent);
}

/*----------------------------------------------------------------*/

void CTFStats(edict_t *ent)
{
	if (!G_TeamplayEnabled())
		return;

	ghost_t *g;
	static std::string text;
	edict_t *e2;

	text.clear();

	if (ctfgame.match == MATCH_SETUP)
	{
		for (uint32_t i = 1; i <= game.maxclients; i++)
		{
			e2 = g_edicts + i;
			if (!e2->inuse)
				continue;
			if (!e2->client->resp.ready && e2->client->resp.ctf_team != CTF_NOTEAM)
			{
				std::string_view str = G_Fmt("{} is not ready.\n", e2->client->pers.netname);

				if (text.length() + str.length() < MAX_CTF_STAT_LENGTH - 50)
					text += str;
			}
		}
	}

	uint32_t i;
	for (i = 0, g = ctfgame.ghosts; i < MAX_CLIENTS; i++, g++)
		if (g->ent)
			break;

	if (i == MAX_CLIENTS)
	{
		if (!text.length())
			text = "No statistics available.\n";

		gi.Client_Print(ent, PRINT_HIGH, text.c_str());
		return;
	}

	text += "  #|Name            |Score|Kills|Death|BasDf|CarDf|Effcy|\n";

	for (i = 0, g = ctfgame.ghosts; i < MAX_CLIENTS; i++, g++)
	{
		if (!*g->netname)
			continue;

		int32_t e;

		if (g->deaths + g->kills == 0)
			e = 50;
		else
			e = g->kills * 100 / (g->kills + g->deaths);
		std::string_view str = G_Fmt("{:3}|{:<16.16}|{:5}|{:5}|{:5}|{:5}|{:5}|{:4}%|\n",
			g->number,
			g->netname,
			g->score,
			g->kills,
			g->deaths,
			g->basedef,
			g->carrierdef,
			e);

		if (text.length() + str.length() > MAX_CTF_STAT_LENGTH - 50)
		{
			text += "And more...\n";
			break;
		}

		text += str;
	}

	gi.Client_Print(ent, PRINT_HIGH, text.c_str());
}

void CTFPlayerList(edict_t *ent)
{
	static std::string text;
	edict_t *e2;

	// number, name, connect time, ping, score, admin
	text.clear();

	for (uint32_t i = 1; i <= game.maxclients; i++)
	{
		e2 = g_edicts + i;
		if (!e2->inuse)
			continue;

		std::string_view str = G_Fmt("{:3} {:<16.16} {:02}:{:02} {:4} {:3}{}{}\n",
					i,
					e2->client->pers.netname,
					(level.time - e2->client->resp.entertime).milliseconds() / 60000,
					((level.time - e2->client->resp.entertime).milliseconds() % 60000) / 1000,
					e2->client->ping,
					e2->client->resp.score,
					(ctfgame.match == MATCH_SETUP || ctfgame.match == MATCH_PREGAME) ? (e2->client->resp.ready ? " (ready)" : " (notready)") : "",
					e2->client->resp.admin ? " (admin)" : "");

		if (text.length() + str.length() > MAX_CTF_STAT_LENGTH - 50)
		{
			text += "And more...\n";
			break;

		}

		text += str;
	}

	gi.Client_Print(ent, PRINT_HIGH, text.data());
}

void CTFWarp(edict_t *ent)
{
	char *token;

	if (gi.argc() < 2)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "Where do you want to warp to?\n");
		gi.LocClient_Print(ent, PRINT_HIGH, "Available levels are: {}\n", warp_list->string);
		return;
	}

	const char *mlist = warp_list->string;

	while (*(token = COM_Parse(&mlist)))
	{
		if (Q_strcasecmp(token, gi.argv(1)) == 0)
			break;
	}

	if (!*token)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "Unknown CTF level.\n");
		gi.LocClient_Print(ent, PRINT_HIGH, "Available levels are: {}\n", warp_list->string);
		return;
	}

	if (ent->client->resp.admin)
	{
		gi.LocBroadcast_Print(PRINT_HIGH, "{} is warping to level {}.\n",
				   ent->client->pers.netname, gi.argv(1));
		Q_strlcpy(level.forcemap, gi.argv(1), sizeof(level.forcemap));
		EndDMLevel();
		return;
	}

	if (CTFBeginElection(ent, ELECT_MAP, G_Fmt("{} has requested warping to level {}.\n",
			ent->client->pers.netname, gi.argv(1)).data()))
		Q_strlcpy(ctfgame.elevel, gi.argv(1), sizeof(ctfgame.elevel));
}

void CTFBoot(edict_t *ent)
{
	edict_t *targ;

	if (!ent->client->resp.admin)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "You are not an admin.\n");
		return;
	}

	if (gi.argc() < 2)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "Who do you want to kick?\n");
		return;
	}

	if (*gi.argv(1) < '0' && *gi.argv(1) > '9')
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "Specify the player number to kick.\n");
		return;
	}

	uint32_t i = strtoul(gi.argv(1), nullptr, 10);
	if (i < 1 || i > game.maxclients)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "Invalid player number.\n");
		return;
	}

	targ = g_edicts + i;
	if (!targ->inuse)
	{
		gi.LocClient_Print(ent, PRINT_HIGH, "That player number is not connected.\n");
		return;
	}

	gi.AddCommandString(G_Fmt("kick {}\n", i - 1).data());
}

void CTFSetPowerUpEffect(edict_t *ent, effects_t def)
{
	if (ent->client->resp.ctf_team == CTF_TEAM1 && def == EF_QUAD)
		ent->s.effects |= EF_PENT; // red
	else if (ent->client->resp.ctf_team == CTF_TEAM2 && def == EF_PENT)
		ent->s.effects |= EF_QUAD; // blue
	else
		ent->s.effects |= def;
}
