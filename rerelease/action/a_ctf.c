//-----------------------------------------------------------------------------
// CTF related code
//
// $Id: a_ctf.c,v 1.20 2003/06/15 21:43:53 igor Exp $
//
//-----------------------------------------------------------------------------
// $Log: a_ctf.c,v $
// Revision 1.20  2003/06/15 21:43:53  igor
// added IRC client
//
// Revision 1.19  2003/02/10 02:12:25  ra
// Zcam fixes, kick crashbug in CTF fixed and some code cleanup.
//
// Revision 1.18  2002/04/01 15:16:06  freud
// Stats code redone, tng_stats now much more smarter. Removed a few global
// variables regarding stats code and added kevlar hits to stats.
//
// Revision 1.17  2002/02/18 17:17:20  freud
// Fixed the CTF leaving team bug. Also made the shield more efficient,
// No falling damage.
//
// Revision 1.16  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.15  2001/08/08 12:42:22  slicerdw
// Ctf Should finnaly be fixed now, lets hope so
//
// Revision 1.14  2001/08/06 14:38:44  ra
// Adding UVtime for ctf
//
// Revision 1.13  2001/06/26 18:47:30  igor_rock
// added ctf_respawn cvar
//
// Revision 1.12  2001/06/20 10:04:13  igor_rock
// corrected the path for the flagsounds (from ctf/ to tng/)
//
// Revision 1.11  2001/06/15 14:18:07  igor_rock
// corrected bug with destroyed flags (won't be destroyed anymore, instead they
// return to the base).
//
// Revision 1.10  2001/06/13 13:05:41  igor_rock
// corrected a minor error in CTFEffects
//
// Revision 1.9  2001/06/13 07:55:17  igor_rock
// Re-Added a_match.h and a_match.c
// Added CTF Header for a_ctf.h and a_ctf.c
//
//-----------------------------------------------------------------------------

#include "g_local.h"

ctfgame_t ctfgame;

cvar_t *ctf = NULL;
cvar_t *ctf_forcejoin = NULL;
cvar_t *ctf_mode = NULL;
cvar_t *ctf_dropflag = NULL;
cvar_t *ctf_respawn = NULL;
cvar_t *ctf_model = NULL;

//-----------------------------------------------------------------------------


/*--------------------------------------------------------------------------*/

gitem_t *team_flag[TEAM_TOP];

//FIXME: should ctfgame score/last_flag_capture/last_capture_team reset when match is over/reset score cmd is used?

void CTFInit(void)
{
	team_flag[TEAM1] = FindItemByClassname("item_flag_team1");
	team_flag[TEAM2] = FindItemByClassname("item_flag_team2");

	memset(&ctfgame, 0, sizeof(ctfgame));
}

/*--------------------------------------------------------------------------*/

qboolean CTFLoadConfig(char *mapname)
{
	char buf[1024];
	char *ptr;
	FILE *fh;

	memset(&ctfgame, 0, sizeof(ctfgame));

	gi.dprintf("Trying to load CTF configuration file\n", mapname);

	/* zero is perfectly acceptable respawn time, but we want to know if it came from the config or not */
	ctfgame.spawn_red = -1;
	ctfgame.spawn_blue = -1;

	sprintf (buf, "%s/tng/%s.ctf", GAMEVERSION, mapname);
	fh = fopen (buf, "r");
	if (!fh) {
		gi.dprintf ("Warning: CTF configuration file %s was not found.\n", buf);
		return false;
	}

	gi.dprintf("-------------------------------------\n");
	gi.dprintf("CTF configuration loaded from %s\n", buf);
	ptr = INI_Find(fh, "ctf", "author");
	if(ptr) {
		gi.dprintf(" Author    : %s\n", ptr);
		Q_strncpyz(ctfgame.author, ptr, sizeof(ctfgame.author));
	}
	ptr = INI_Find(fh, "ctf", "comment");
	if(ptr) {
		gi.dprintf(" Comment   : %s\n", ptr);
		Q_strncpyz(ctfgame.comment, ptr, sizeof(ctfgame.comment));
	}

	ptr = INI_Find(fh, "ctf", "type");
	if(ptr) {
		gi.dprintf(" Game type : %s\n", ptr);
		if(strcmp(ptr, "balanced") == 0)
			ctfgame.type = 1;
		if(strcmp(ptr, "offdef") == 0)
			ctfgame.type = 2;
	}
	ptr = INI_Find(fh, "ctf", "offence");
	if(ptr) {
		gi.dprintf(" Offence   : %s\n", ptr);
		ctfgame.offence = TEAM1;
		if(strcmp(ptr, "blue") == 0)
			ctfgame.offence = TEAM2;
	}
	ptr = INI_Find(fh, "ctf", "grapple");
	gi.cvar_forceset("use_grapple", "0");
	if(ptr) {
		gi.dprintf(" Grapple   : %s\n", ptr);
		if(strcmp(ptr, "1") == 0)
			gi.cvar_forceset("use_grapple", "1");
		else if(strcmp(ptr, "2") == 0)
			gi.cvar_forceset("use_grapple", "2");
	}

	gi.dprintf(" Spawn times\n");
	ptr = INI_Find(fh, "respawn", "red");
	if(ptr) {
		gi.dprintf("  Red      : %s\n", ptr);
		ctfgame.spawn_red = atoi(ptr);
	}
	ptr = INI_Find(fh, "respawn", "blue");
	if(ptr) {
		gi.dprintf("  Blue     : %s\n", ptr);
		ctfgame.spawn_blue = atoi(ptr);
	}

	gi.dprintf(" Flags\n");
	ptr = INI_Find(fh, "flags", "red");
	if(ptr) {
		gi.dprintf("  Red      : %s\n", ptr);
		CTFSetFlag(TEAM1, ptr);
	}
	ptr = INI_Find(fh, "flags", "blue");
	if(ptr) {
		gi.dprintf("  Blue     : %s\n", ptr);
		CTFSetFlag(TEAM2, ptr);
	}

	gi.dprintf(" Spawns\n");
	ptr = INI_Find(fh, "spawns", "red");
	if(ptr) {
		gi.dprintf("  Red      : %s\n", ptr);
		CTFSetTeamSpawns(TEAM1, ptr);
		ctfgame.custom_spawns = true;
	}
	ptr = INI_Find(fh, "spawns", "blue");
	if(ptr) {
		gi.dprintf("  Blue     : %s\n", ptr);
		CTFSetTeamSpawns(TEAM2, ptr);
		ctfgame.custom_spawns = true;
	}

	// automagically change spawns *only* when we do not have team spawns
	if(!ctfgame.custom_spawns)
		ChangePlayerSpawns();

	gi.dprintf("-------------------------------------\n");

	fclose(fh);

	return true;
}

void CTFSetFlag(int team, char *str)
{
	char *flag_name;
	edict_t *ent = NULL;
	vec3_t position;

	if(team == TEAM1)
		flag_name = "item_flag_team1";
	else if(team == TEAM2)
		flag_name = "item_flag_team2";
	else
		return;

	if (sscanf(str, "<%f %f %f>", &position[0], &position[1], &position[2]) != 3)
		return;

	/* find and remove existing flag(s) if any */
	while ((ent = G_Find(ent, FOFS(classname), flag_name)) != NULL) {
		G_FreeEdict (ent);
	}

	ent = G_Spawn ();

	ent->classname = ED_NewString (flag_name);
	ent->spawnflags &=
		~(SPAWNFLAG_NOT_EASY | SPAWNFLAG_NOT_MEDIUM | SPAWNFLAG_NOT_HARD |
		SPAWNFLAG_NOT_COOP | SPAWNFLAG_NOT_DEATHMATCH);

	VectorCopy(position, ent->s.origin);
	VectorCopy(position, ent->old_origin);

	ED_CallSpawn (ent);
}

void CTFSetTeamSpawns(int team, char *str)
{
	edict_t *spawn = NULL;
	char *next;
	vec3_t pos;
	float angle;

	char *team_spawn_name = "info_player_team1";
	if(team == TEAM2)
		team_spawn_name = "info_player_team2";

	/* find and remove all team spawns for this team */
	while ((spawn = G_Find(spawn, FOFS(classname), team_spawn_name)) != NULL) {
		G_FreeEdict (spawn);
	}

	next = strtok(str, ",");
	do {
		if (sscanf(next, "<%f %f %f %f>", &pos[0], &pos[1], &pos[2], &angle) != 4) {
			gi.dprintf("CTFSetTeamSpawns: invalid spawn point: %s, expected <x y z a>\n", next);
			continue;
		}

		spawn = G_Spawn ();
		VectorCopy(pos, spawn->s.origin);
		spawn->s.angles[YAW] = angle;
		spawn->classname = ED_NewString (team_spawn_name);
		ED_CallSpawn (spawn);

		next = strtok(NULL, ",");
	} while(next != NULL);
}

/* returns the respawn time for this particular client */
int CTFGetRespawnTime(edict_t *ent)
{
	int spawntime = ctf_respawn->value;
	if(ent->client->resp.team == TEAM1 && ctfgame.spawn_red > -1)
		spawntime = ctfgame.spawn_red;
	else if(ent->client->resp.team == TEAM2 && ctfgame.spawn_blue > -1)
		spawntime = ctfgame.spawn_blue;

	gi.cprintf(ent, PRINT_HIGH, "You will respawn in %d seconds\n", spawntime);

	return spawntime;
}

/* Has flag:
 * JBravo: Does the player have a flag ?
 */
qboolean HasFlag(edict_t * ent)
{
	if (!ctf->value)
		return false;
	if (ent->client->inventory[items[FLAG_T1_NUM].index] || ent->client->inventory[items[FLAG_T2_NUM].index])
		return true;
	return false;
}

char *CTFTeamName(int team)
{
	switch (team) {
	case TEAM1:
		return "RED";
	case TEAM2:
		return "BLUE";
	}
	return "UKNOWN";
}

char *CTFOtherTeamName(int team)
{
	switch (team) {
	case TEAM1:
		return "BLUE";
	case TEAM2:
		return "RED";
	}
	return "UKNOWN";
}

int CTFOtherTeam(int team)
{
	switch (team) {
	case TEAM1:
		return TEAM2;
	case TEAM2:
		return TEAM1;
	case NOTEAM:
		return NOTEAM; /* there is no other team for NOTEAM, but I want it back! */
	}
	return -1;		// invalid value
}

/*--------------------------------------------------------------------------*/

void ResetPlayers(void)
{
	edict_t *ent;
	int i;

	for (i = 0; i < game.maxclients; i++) {
		ent = &g_edicts[1 + i];
		if (ent->inuse) {
//			ent->client->resp.team = NOTEAM;
			PutClientInServer(ent);
		}
	}
}

void CTFSwapTeams()
{
	edict_t *ent;
	int i;

	for (i = 0; i < game.maxclients; i++) {
		ent = &g_edicts[1 + i];
		if (ent->inuse && ent->client->resp.team) {
			ent->client->resp.team = CTFOtherTeam(ent->client->resp.team);
			AssignSkin(ent, teams[ent->client->resp.team].skin, false);
		}
	}

	/* swap scores too! */
	i = ctfgame.team1;
	ctfgame.team1 = ctfgame.team2;
	ctfgame.team2 = i;

	// Swap matchmode team captains.
	ent = teams[TEAM1].captain;
	teams[TEAM1].captain = teams[TEAM2].captain;
	teams[TEAM2].captain = ent;

	teams_changed = true;
}

void CTFAssignTeam(gclient_t * who)
{
	edict_t *player;
	int i, team1count = 0, team2count = 0;

	who->resp.ctf_state = CTF_STATE_START;

	if (!DMFLAGS(DF_CTF_FORCEJOIN)) {
		who->resp.team = NOTEAM;
		return;
	}

	for (i = 1; i <= game.maxclients; i++) {
		player = &g_edicts[i];
		if (!player->inuse || player->client == who)
			continue;
		switch (player->client->resp.team) {
		case TEAM1:
			team1count++;
			break;
		case TEAM2:
			team2count++;
		}
	}
	if (team1count < team2count)
		who->resp.team = TEAM1;
	else if (team2count < team1count)
		who->resp.team = TEAM2;
	else if (rand() & 1)
		who->resp.team = TEAM1;
	else
		who->resp.team = TEAM2;

	teams_changed = true;
}

/*
================
SelectCTFSpawnPoint

go to a ctf point, but NOT the two points closest
to other players
================
*/
edict_t *SelectCTFSpawnPoint(edict_t * ent)
{
	edict_t *spot, *spot1, *spot2;
	int count = 0;
	int selection;
	float range, range1, range2;
	char *cname;

	ent->client->resp.ctf_state = CTF_STATE_PLAYING;

	switch (ent->client->resp.team) {
	case TEAM1:
		cname = "info_player_team1";
		break;
	case TEAM2:
		cname = "info_player_team2";
		break;
	default:
		/* FIXME: might return NULL when dm spawns are converted to team ones */
		return SelectRandomDeathmatchSpawnPoint();
	}

	spot = NULL;
	range1 = range2 = 99999;
	spot1 = spot2 = NULL;

	while ((spot = G_Find(spot, FOFS(classname), cname)) != NULL) {
		count++;
		range = PlayersRangeFromSpot(spot);
		if (range < range1) {
			if (range1 < range2) {
				range2 = range1;
				spot2 = spot1;
			}
			range1 = range;
			spot1 = spot;
		} else if (range < range2) {
			range2 = range;
			spot2 = spot;
		}
	}

	if (!count)
		return SelectRandomDeathmatchSpawnPoint();

	if (count <= 2) {
		spot1 = spot2 = NULL;
	} else
		count -= 2;

	selection = rand() % count;

	spot = NULL;
	do {
		spot = G_Find(spot, FOFS(classname), cname);
		if (spot == spot1 || spot == spot2)
			selection++;
	}
	while (selection--);

	return spot;
}

/*------------------------------------------------------------------------*/
/*
CTFFragBonuses

Calculate the bonuses for flag defense, flag carrier defense, etc.
Note that bonuses are not cumaltive.  You get one, they are in importance
order.
*/
void CTFFragBonuses(edict_t * targ, edict_t * inflictor, edict_t * attacker)
{
	int i, otherteam;
	gitem_t *flag_item, *enemy_flag_item;
	edict_t *ent, *flag, *carrier;
	vec3_t v1, v2;

	carrier = NULL;

	// no bonus for fragging yourself
	if (!targ->client || !attacker->client || targ == attacker)
		return;

	otherteam = CTFOtherTeam(targ->client->resp.team);
	if (otherteam < 1)
		return;		// whoever died isn't on a team

	// same team, if the flag at base, check to he has the enemy flag
	flag_item = team_flag[targ->client->resp.team];
	enemy_flag_item = team_flag[otherteam];

	// did the attacker frag the flag carrier?
	if (targ->client->inventory[ITEM_INDEX(enemy_flag_item)]) {
		attacker->client->resp.ctf_lastfraggedcarrier = level.framenum;
		attacker->client->resp.score += CTF_FRAG_CARRIER_BONUS;
		gi.cprintf(attacker, PRINT_MEDIUM,
			   "BONUS: %d points for fragging enemy flag carrier.\n", CTF_FRAG_CARRIER_BONUS);

		// the the target had the flag, clear the hurt carrier
		// field on the other team
		for (i = 1; i <= game.maxclients; i++) {
			ent = g_edicts + i;
			if (ent->inuse && ent->client->resp.team == otherteam)
				ent->client->resp.ctf_lasthurtcarrier = 0;
		}
		return;
	}

	if (targ->client->resp.ctf_lasthurtcarrier &&
	    level.framenum - targ->client->resp.ctf_lasthurtcarrier <
	    CTF_CARRIER_DANGER_PROTECT_TIMEOUT * HZ && !attacker->client->inventory[ITEM_INDEX(flag_item)]) {
		// attacker is on the same team as the flag carrier and
		// fragged a guy who hurt our flag carrier
		attacker->client->resp.score += CTF_CARRIER_DANGER_PROTECT_BONUS;
		gi.bprintf(PRINT_MEDIUM,
			   "%s defends %s's flag carrier against an agressive enemy\n",
			   attacker->client->pers.netname, CTFTeamName(attacker->client->resp.team));
		return;
	}
	// flag and flag carrier area defense bonuses
	// we have to find the flag and carrier entities
	// find the flag
	flag = NULL;
	while ((flag = G_Find(flag, FOFS(classname), flag_item->classname)) != NULL) {
		if (!(flag->spawnflags & DROPPED_ITEM))
			break;
	}

	if (!flag)
		return;		// can't find attacker's flag

	// find attacker's team's flag carrier
	for (i = 1; i <= game.maxclients; i++) {
		carrier = g_edicts + i;
		if (carrier->inuse && carrier->client->inventory[ITEM_INDEX(flag_item)])
			break;
		carrier = NULL;
	}

	// ok we have the attackers flag and a pointer to the carrier
	// check to see if we are defending the base's flag
	VectorSubtract(targ->s.origin, flag->s.origin, v1);
	VectorSubtract(attacker->s.origin, flag->s.origin, v2);

	if (VectorLength(v1) < CTF_TARGET_PROTECT_RADIUS || VectorLength(v2) < CTF_TARGET_PROTECT_RADIUS
		|| visible(flag, targ, MASK_SOLID) || visible(flag, attacker, MASK_SOLID)) {
		// we defended the base flag
		attacker->client->resp.score += CTF_FLAG_DEFENSE_BONUS;
		if (flag->solid == SOLID_NOT) {
			gi.bprintf(PRINT_MEDIUM, "%s defends the %s base.\n",
				   attacker->client->pers.netname, CTFTeamName(attacker->client->resp.team));
		} else {
			gi.bprintf(PRINT_MEDIUM, "%s defends the %s flag.\n",
				   attacker->client->pers.netname, CTFTeamName(attacker->client->resp.team));
		}
		return;
	}

	if (carrier && carrier != attacker) {
		VectorSubtract(targ->s.origin, carrier->s.origin, v1);
		VectorSubtract(attacker->s.origin, carrier->s.origin, v1);

		if (VectorLength(v1) < CTF_ATTACKER_PROTECT_RADIUS ||
		    VectorLength(v2) < CTF_ATTACKER_PROTECT_RADIUS ||
			visible(carrier, targ, MASK_SOLID) || visible(carrier, attacker, MASK_SOLID)) {
			attacker->client->resp.score += CTF_CARRIER_PROTECT_BONUS;
			gi.bprintf(PRINT_MEDIUM, "%s defends the %s's flag carrier.\n", attacker->client->pers.netname, CTFTeamName(attacker->client->resp.team));
			return;
		}
	}
}

void CTFCheckHurtCarrier(edict_t * targ, edict_t * attacker)
{
	int otherteam;

	if (!targ->client || !attacker->client)
		return;

	otherteam = CTFOtherTeam(targ->client->resp.team);
	if (otherteam < 1)
		return;

	if (targ->client->inventory[ITEM_INDEX(team_flag[otherteam])] &&
	    targ->client->resp.team != attacker->client->resp.team)
		attacker->client->resp.ctf_lasthurtcarrier = level.framenum;
}

/*------------------------------------------------------------------------*/

void CTFResetFlag(int team)
{
	int i;
	edict_t *ent = NULL;
	gitem_t *teamFlag = team_flag[team];

	/* hifi: drop this team flag if a player is carrying one (so the next loop returns it correctly) */
	for (i = 0, ent = &g_edicts[1]; i < game.maxclients; i++, ent++) {
		if (!ent->inuse)
			continue;

		if (ent->client->inventory[ITEM_INDEX(teamFlag)]) {
			Drop_Item(ent, teamFlag);
			ent->client->inventory[ITEM_INDEX(teamFlag)] = 0;
		}
	}

	while ((ent = G_Find(ent, FOFS(classname), teamFlag->classname)) != NULL) {
		if (ent->spawnflags & DROPPED_ITEM)
			G_FreeEdict(ent);
		else {
			ent->svflags &= ~SVF_NOCLIENT;
			ent->solid = SOLID_TRIGGER;
			gi.linkentity(ent);
			ent->s.event = EV_ITEM_RESPAWN;
		}
	}
}

void CTFResetFlags(void)
{
	CTFResetFlag(TEAM1);
	CTFResetFlag(TEAM2);
}

qboolean CTFPickup_Flag(edict_t * ent, edict_t * other)
{
	int team, i;
	edict_t *player;
	gitem_t *flag_item, *enemy_flag_item;

	/* FIXME: players shouldn't be able to touch flags before LCA! */
	if(!team_round_going)
		return false;

	// figure out what team this flag is
	if (strcmp(ent->classname, "item_flag_team1") == 0)
		team = TEAM1;
	else if (strcmp(ent->classname, "item_flag_team2") == 0)
		team = TEAM2;
	else {
		gi.cprintf(ent, PRINT_HIGH, "Don't know what team the flag is on.\n");
		return false;
	}

	// same team, if the flag at base, check to he has the enemy flag
	if (team == TEAM1) {
		flag_item = team_flag[TEAM1];
		enemy_flag_item = team_flag[TEAM2];
	} else {
		flag_item = team_flag[TEAM2];
		enemy_flag_item = team_flag[TEAM1];
	}

	if (team == other->client->resp.team) {

		if (!(ent->spawnflags & DROPPED_ITEM)) {
			// the flag is at home base.  if the player has the enemy
			// flag, he's just won!
			if (other->client->inventory[ITEM_INDEX(enemy_flag_item)]) {
				gi.bprintf(PRINT_HIGH, "%s captured the %s flag!\n",
					   other->client->pers.netname, CTFOtherTeamName(team));
				other->client->inventory[ITEM_INDEX(enemy_flag_item)] = 0;

				ctfgame.last_flag_capture = level.framenum;
				ctfgame.last_capture_team = team;
				if (team == TEAM1)
					ctfgame.team1++;
				else
					ctfgame.team2++;

				gi.sound(ent, CHAN_RELIABLE + CHAN_NO_PHS_ADD + CHAN_VOICE,
					 gi.soundindex("tng/flagcap.wav"), 1, ATTN_NONE, 0);

				// other gets another 10 frag bonus
				other->client->resp.score += CTF_CAPTURE_BONUS;
				other->client->resp.ctf_caps++;

				CTFCapReward(other);

				// Ok, let's do the player loop, hand out the bonuses
				for (i = 1; i <= game.maxclients; i++) {
					player = &g_edicts[i];
					if (!player->inuse)
						continue;

					if (player->client->resp.team != other->client->resp.team)
						player->client->resp.ctf_lasthurtcarrier = 0;
					else if (player->client->resp.team == other->client->resp.team) {
						if (player != other)
							player->client->resp.score += CTF_TEAM_BONUS;
						// award extra points for capture assists
						if (player->client->resp.ctf_lastreturnedflag +
							CTF_RETURN_FLAG_ASSIST_TIMEOUT * HZ > level.framenum) {
							gi.bprintf(PRINT_HIGH,
								   "%s gets an assist for returning the flag!\n",
								   player->client->pers.netname);
							player->client->resp.score += CTF_RETURN_FLAG_ASSIST_BONUS;
						}
						if (player->client->resp.ctf_lastfraggedcarrier +
							CTF_FRAG_CARRIER_ASSIST_TIMEOUT * HZ > level.framenum) {
							gi.bprintf(PRINT_HIGH,
								   "%s gets an assist for fragging the flag carrier!\n",
								   player->client->pers.netname);
							player->client->resp.score += CTF_FRAG_CARRIER_ASSIST_BONUS;
						}
					}
				}

				CTFResetFlags();
				return false;
			}
			return false;	// its at home base already
		}
		// hey, its not home.  return it by teleporting it back
		gi.bprintf(PRINT_HIGH, "%s returned the %s flag!\n", other->client->pers.netname, CTFTeamName(team));

		other->client->resp.score += CTF_RECOVERY_BONUS;
		other->client->resp.ctf_lastreturnedflag = level.framenum;
		gi.sound(ent, CHAN_RELIABLE + CHAN_NO_PHS_ADD + CHAN_VOICE,
			 gi.soundindex("tng/flagret.wav"), 1, ATTN_NONE, 0);
		//CTFResetFlag will remove this entity!  We must return false
		CTFResetFlag(team);
		return false;
	}
// AQ2:TNG - JBravo adding UVtime
	if (other->client->uvTime) {
		other->client->uvTime = 0;
		gi.centerprintf(other, "Flag taken! Shields are DOWN! Run for it!");
	} else {
		gi.centerprintf(other, "You've got the ENEMY FLAG! Run for it!");
	}
	// hey, its not our flag, pick it up
	gi.bprintf(PRINT_HIGH, "%s got the %s flag!\n", other->client->pers.netname, CTFTeamName(team));
	other->client->resp.score += CTF_FLAG_BONUS;

	other->client->inventory[ITEM_INDEX(flag_item)] = 1;
	other->client->resp.ctf_flagsince = level.framenum;

	// pick up the flag
	// if it's not a dropped flag, we just make is disappear
	// if it's dropped, it will be removed by the pickup caller
	if (!(ent->spawnflags & DROPPED_ITEM)) {
		ent->flags |= FL_RESPAWN;
		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
	}
	return true;
}

static void CTFDropFlagTouch(edict_t * ent, edict_t * other, cplane_t * plane, csurface_t * surf)
{
	//owner (who dropped us) can't touch for two secs
	if (other == ent->owner && ent->nextthink > level.framenum + (CTF_AUTO_FLAG_RETURN_TIMEOUT - 2) * HZ)
		return;

	Touch_Item(ent, other, plane, surf);
}

static void CTFDropFlagThink(edict_t * ent)
{
	// auto return the flag
	// reset flag will remove ourselves
	if (strcmp(ent->classname, "item_flag_team1") == 0) {
		CTFResetFlag(TEAM1);
		gi.bprintf(PRINT_HIGH, "The %s flag has returned!\n", CTFTeamName(TEAM1));
	} else if (strcmp(ent->classname, "item_flag_team2") == 0) {
		CTFResetFlag(TEAM2);
		gi.bprintf(PRINT_HIGH, "The %s flag has returned!\n", CTFTeamName(TEAM2));
	}
}

// Called from PlayerDie, to drop the flag from a dying player
void CTFDeadDropFlag(edict_t * self)
{
	edict_t *dropped = NULL;

	if (self->client->inventory[ITEM_INDEX(team_flag[TEAM1])]) {
		dropped = Drop_Item(self, team_flag[TEAM1]);
		self->client->inventory[ITEM_INDEX(team_flag[TEAM1])] = 0;
		gi.bprintf(PRINT_HIGH, "%s lost the %s flag!\n", self->client->pers.netname, CTFTeamName(TEAM1));

	} else if (self->client->inventory[ITEM_INDEX(team_flag[TEAM2])]) {
		dropped = Drop_Item(self, team_flag[TEAM2]);
		self->client->inventory[ITEM_INDEX(team_flag[TEAM2])] = 0;
		gi.bprintf(PRINT_HIGH, "%s lost the %s flag!\n", self->client->pers.netname, CTFTeamName(TEAM2));
	}

	if (dropped) {
		dropped->think = CTFDropFlagThink;
		dropped->nextthink = level.framenum + CTF_AUTO_FLAG_RETURN_TIMEOUT * HZ;
		dropped->touch = CTFDropFlagTouch;
	}
}

void CTFDrop_Flag(edict_t * ent, gitem_t * item)
{
	edict_t *dropped = NULL;

	if (ctf_dropflag->value) {
		if (ent->client->inventory[ITEM_INDEX(team_flag[TEAM1])]) {
			dropped = Drop_Item(ent, team_flag[TEAM1]);
			ent->client->inventory[ITEM_INDEX(team_flag[TEAM1])] = 0;
		} else if (ent->client->inventory[ITEM_INDEX(team_flag[TEAM2])]) {
			dropped = Drop_Item(ent, team_flag[TEAM2]);
			ent->client->inventory[ITEM_INDEX(team_flag[TEAM2])] = 0;
		}

		if (dropped) {
			dropped->think = CTFDropFlagThink;
			dropped->nextthink = level.framenum + CTF_AUTO_FLAG_RETURN_TIMEOUT * HZ;
			dropped->touch = CTFDropFlagTouch;
		}
	} else {
		if (rand() & 1)
			gi.cprintf(ent, PRINT_HIGH, "Only losers drop flags.\n");
		else
			gi.cprintf(ent, PRINT_HIGH, "Winners don't drop flags.\n");
	}
	return;
}

static void CTFFlagThink(edict_t * ent)
{
	if (ent->solid != SOLID_NOT)
		ent->s.frame = 173 + (((ent->s.frame - 173) + 1) % 16);
	ent->nextthink = level.framenum + FRAMEDIV;
}

void CTFFlagSetup(edict_t * ent)
{
	trace_t tr;
	vec3_t dest;

	VectorSet(ent->mins, -15, -15, -15);
	VectorSet(ent->maxs, 15, 15, 15);

	if (ent->model)
		gi.setmodel(ent, ent->model);
	else
		gi.setmodel(ent, ent->item->world_model);
	ent->solid = SOLID_TRIGGER;
	ent->movetype = MOVETYPE_TOSS;
	ent->touch = Touch_Item;

	VectorCopy(ent->s.origin, dest);
	dest[2] -= 128;

	tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, dest, ent, MASK_SOLID);
	if (tr.startsolid) {
		gi.dprintf("CTFFlagSetup: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
		G_FreeEdict(ent);
		return;
	}

	VectorCopy(tr.endpos, ent->s.origin);
	VectorCopy(tr.endpos, ent->old_origin);

	gi.linkentity(ent);

	ent->nextthink = level.framenum + 1;
	ent->think = CTFFlagThink;
}

void CTFEffects(edict_t * player)
{
	player->s.effects &= ~(EF_FLAG1 | EF_FLAG2);

	// megahealth players glow anyway
	if(player->health > 100)
		player->s.effects |= EF_TAGTRAIL;

	player->s.modelindex3 = 0;
	if (player->client->inventory[ITEM_INDEX(team_flag[TEAM1])])
	{
		player->s.modelindex3 = gi.modelindex("models/flags/flag1.md2");
		if (player->health > 0)
			player->s.effects |= EF_FLAG1;
	}
	else if (player->client->inventory[ITEM_INDEX(team_flag[TEAM2])])
	{
		player->s.modelindex3 = gi.modelindex("models/flags/flag2.md2");
		if (player->health > 0)
			player->s.effects |= EF_FLAG2;
	}
}

// called when we enter the intermission
void CTFCalcScores(void)
{
	int i;

	ctfgame.total1 = ctfgame.total2 = 0;
	for (i = 0; i < game.maxclients; i++) {
		if (!g_edicts[i + 1].inuse)
			continue;
		if (game.clients[i].resp.team == TEAM1)
			ctfgame.total1 += game.clients[i].resp.score;
		else if (game.clients[i].resp.team == TEAM2)
			ctfgame.total2 += game.clients[i].resp.score;
	}
	// Stats: Reset roundNum
	game.roundNum = 0;
	// Stats end
}

void GetCTFScores(int *t1score, int *t2score)
{
	*t1score = ctfgame.team1;
	*t2score = ctfgame.team2;
}

void SetCTFStats(edict_t * ent)
{
	int teamnum, flagpic[TEAM_TOP] = {0}, doBlink = ((level.realFramenum / FRAMEDIV) & 8);
	edict_t *e;

	// logo headers for the frag display
	ent->client->ps.stats[STAT_TEAM1_HEADER] = level.pic_ctf_teamtag[TEAM1];
	ent->client->ps.stats[STAT_TEAM2_HEADER] = level.pic_ctf_teamtag[TEAM2];

	// if during intermission, we must blink the team header of the winning team
	if (level.intermission_framenum && doBlink) {	// blink 1/8th second
		// note that ctfgame.total[12] is set when we go to intermission
		if (ctfgame.team1 > ctfgame.team2)
			ent->client->ps.stats[STAT_TEAM1_HEADER] = 0;
		else if (ctfgame.team2 > ctfgame.team1)
			ent->client->ps.stats[STAT_TEAM2_HEADER] = 0;
		else if (ctfgame.total1 > ctfgame.total2)	// frag tie breaker
			ent->client->ps.stats[STAT_TEAM1_HEADER] = 0;
		else if (ctfgame.total2 > ctfgame.total1)
			ent->client->ps.stats[STAT_TEAM2_HEADER] = 0;
		else {		// tie game!
			ent->client->ps.stats[STAT_TEAM1_HEADER] = 0;
			ent->client->ps.stats[STAT_TEAM2_HEADER] = 0;
		}
	}
	// figure out what icon to display for team logos
	// three states:
	//   flag at base
	//   flag taken
	//   flag dropped
	for (teamnum = TEAM1; teamnum <= TEAM2; teamnum++) {
		flagpic[teamnum] = level.pic_ctf_flagbase[teamnum];
		e = G_Find(NULL, FOFS(classname), team_flag[teamnum]->classname);
		if (e != NULL) {
			if (e->solid == SOLID_NOT) {
				int i;

				// not at base
				// check if on player
				flagpic[teamnum] = level.pic_ctf_flagdropped[teamnum];	// default to dropped
				for (i = 1; i <= game.maxclients; i++)
					if (g_edicts[i].inuse && g_edicts[i].client->inventory[ITEM_INDEX(team_flag[teamnum])]) {
						// enemy has it
						flagpic[teamnum] = level.pic_ctf_flagtaken[teamnum];
						break;
					}
			} else if (e->spawnflags & DROPPED_ITEM)
				flagpic[teamnum] = level.pic_ctf_flagdropped[teamnum];	// must be dropped
		}
	}

	if (ctfgame.last_flag_capture && level.framenum < ctfgame.last_flag_capture + 5 * HZ && !doBlink) {
		flagpic[ctfgame.last_capture_team] = 0;
	}

	ent->client->ps.stats[STAT_TEAM1_PIC] = flagpic[TEAM1];
	ent->client->ps.stats[STAT_TEAM2_PIC] = flagpic[TEAM2];

	ent->client->ps.stats[STAT_TEAM1_SCORE] = ctfgame.team1;
	ent->client->ps.stats[STAT_TEAM2_SCORE] = ctfgame.team2;

	ent->client->ps.stats[STAT_FLAG_PIC] = 0;
	if (doBlink)
	{
		teamnum = CTFOtherTeam(ent->client->resp.team);
		if (teamnum > 0 && ent->client->inventory[ITEM_INDEX(team_flag[teamnum])])
			ent->client->ps.stats[STAT_FLAG_PIC] = level.pic_ctf_flagbase[teamnum];
	}

	ent->client->ps.stats[STAT_ID_VIEW] = 0;
	if (!ent->client->pers.id)
		SetIDView(ent);
}

/*------------------------------------------------------------------------*/

/*QUAKED info_player_team1 (1 0 0) (-16 -16 -24) (16 16 32)
potential team1 spawning position for ctf games
*/
void SP_info_player_team1(edict_t * self)
{
}

/*QUAKED info_player_team2 (0 0 1) (-16 -16 -24) (16 16 32)
potential team2 spawning position for ctf games
*/
void SP_info_player_team2(edict_t * self)
{
}

/*-----------------------------------------------------------------------*/
/*QUAKED misc_ctf_banner (1 .5 0) (-4 -64 0) (4 64 248) TEAM2
The origin is the bottom of the banner.
The banner is 248 tall.
*/
static void misc_ctf_banner_think(edict_t * ent)
{
	ent->s.frame = (ent->s.frame + 1) % 16;
	ent->nextthink = level.framenum + FRAMEDIV;
}

void SP_misc_ctf_banner(edict_t * ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/ctf/banner/tris.md2");
	if (ent->spawnflags & 1)	// team2
		ent->s.skinnum = 1;

	ent->s.frame = rand() % 16;
	gi.linkentity(ent);

	ent->think = misc_ctf_banner_think;
	ent->nextthink = level.framenum + 1;
}

/*QUAKED misc_ctf_small_banner (1 .5 0) (-4 -32 0) (4 32 124) TEAM2
The origin is the bottom of the banner.
The banner is 124 tall.
*/
void SP_misc_ctf_small_banner(edict_t * ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_NOT;
	ent->s.modelindex = gi.modelindex("models/ctf/banner/small.md2");
	if (ent->spawnflags & 1)	// team2
		ent->s.skinnum = 1;

	ent->s.frame = rand() % 16;
	gi.linkentity(ent);

	ent->think = misc_ctf_banner_think;
	ent->nextthink = level.framenum + 1;
}

/*-----------------------------------------------------------------------*/

void CTFShowScores(edict_t * ent, pmenu_t * p)
{
	if (ent->client->layout == LAYOUT_MENU)
		PMenu_Close(ent);

	ent->client->layout = LAYOUT_SCORES;
	ent->client->showinventory = false;
	DeathmatchScoreboard(ent);
}

qboolean CTFCheckRules(void)
{
	if( capturelimit->value && (ctfgame.team1 >= capturelimit->value || ctfgame.team2 >= capturelimit->value) )
	{
		gi.bprintf(PRINT_HIGH, "Capturelimit hit.\n");
		return true;
	}

	if( timelimit->value > 0 && ctfgame.type > 0 )
	{
		if( ctfgame.halftime == 0 && level.matchTime >= (timelimit->value * 60) / 2 - 60 )
		{
			if( use_warnings->value )
			{
				CenterPrintAll( "1 MINUTE LEFT..." );
				gi.sound( &g_edicts[0], CHAN_VOICE | CHAN_NO_PHS_ADD, gi.soundindex("tng/1_minute.wav"), 1.0, ATTN_NONE, 0.0 );
			}
			ctfgame.halftime = 1;
		}
		else if( ctfgame.halftime == 1 && level.matchTime >= (timelimit->value * 60) / 2 - 10 )
		{
			if( use_warnings->value )
				gi.sound( &g_edicts[0], CHAN_VOICE | CHAN_NO_PHS_ADD, gi.soundindex("world/10_0.wav"), 1.0, ATTN_NONE, 0.0 );
			ctfgame.halftime = 2;
		}
		else if( ctfgame.halftime < 3 && level.matchTime >= (timelimit->value * 60) / 2 + 1 )
		{
			team_round_going = team_round_countdown = team_game_going = 0;
			MakeAllLivePlayersObservers ();
			CTFSwapTeams();
			CenterPrintAll("The teams have been switched!");
			gi.sound(&g_edicts[0], CHAN_VOICE | CHAN_NO_PHS_ADD,
				 gi.soundindex("misc/secret.wav"), 1.0, ATTN_NONE, 0.0);
			ctfgame.halftime = 3;
		}
	}
	return false;
}

/*--------------------------------------------------------------------------
 * just here to help old map conversions
 *--------------------------------------------------------------------------*/

static void old_teleporter_touch(edict_t * self, edict_t * other, cplane_t * plane, csurface_t * surf)
{
	edict_t *dest;
	int i;
	vec3_t forward;

	if (!other->client)
		return;
	dest = G_Find(NULL, FOFS(targetname), self->target);
	if (!dest) {
		gi.dprintf("Couldn't find destination\n");
		return;
	}
	// unlink to make sure it can't possibly interfere with KillBox
	gi.unlinkentity(other);

	VectorCopy(dest->s.origin, other->s.origin);
	VectorCopy(dest->s.origin, other->s.old_origin);
	VectorCopy(dest->s.origin, other->old_origin);

	// clear the velocity and hold them in place briefly
	VectorClear(other->velocity);
	other->client->ps.pmove.pm_time = 160 >> 3;	// hold time
	other->client->ps.pmove.pm_flags |= PMF_TIME_TELEPORT;

	// draw the teleport splash at source and on the player
	self->enemy->s.event = EV_PLAYER_TELEPORT;
	other->s.event = EV_PLAYER_TELEPORT;

	// set angles
	for (i = 0; i < 3; i++)
		other->client->ps.pmove.delta_angles[i] =
		    ANGLE2SHORT(dest->s.angles[i] - other->client->resp.cmd_angles[i]);

	other->s.angles[PITCH] = 0;
	other->s.angles[YAW] = dest->s.angles[YAW];
	other->s.angles[ROLL] = 0;
	VectorCopy(dest->s.angles, other->client->ps.viewangles);
	VectorCopy(dest->s.angles, other->client->v_angle);

	// give a little forward velocity
	AngleVectors(other->client->v_angle, forward, NULL, NULL);
	VectorScale(forward, 200, other->velocity);

	// kill anything at the destination
	if (!KillBox(other)) {
	}

	gi.linkentity(other);
}

/*QUAKED trigger_teleport (0.5 0.5 0.5) ?
Players touching this will be teleported
*/
void SP_trigger_teleport(edict_t * ent)
{
	edict_t *s;
	int i;

	if (!ent->target) {
		gi.dprintf("teleporter without a target.\n");
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

/*QUAKED info_teleport_destination (0.5 0.5 0.5) (-16 -16 -24) (16 16 32)
Point trigger_teleports at these.
*/
void SP_info_teleport_destination(edict_t * ent)
{
	ent->s.origin[2] += 16;
}

void CTFDestroyFlag(edict_t * self)
{
	//flags are important
	if (ctf->value) {
		if (strcmp(self->classname, "item_flag_team1") == 0) {
			CTFResetFlag(TEAM1);	// this will free self!
			gi.bprintf(PRINT_HIGH, "The %s flag has returned!\n", CTFTeamName(TEAM1));
			return;
		}
		if (strcmp(self->classname, "item_flag_team2") == 0) {
			CTFResetFlag(TEAM2);	// this will free self!
			gi.bprintf(PRINT_HIGH, "The %s flag has returned!\n", CTFTeamName(TEAM2));
			return;
		}
	}
	// just release it.
	G_FreeEdict(self);
}

/* give client full health and some ammo for reward */
void CTFCapReward(edict_t * ent)
{
	gclient_t *client;
	gitem_t *item;
	int was_bandaging = 0;
	int band;
	int player_weapon;

	if(!ctf_mode->value)
		return;

	if(ctf_mode->value > 1)
		ent->client->resp.ctf_capstreak++;
	else /* capstreak is used as a multiplier so default it to one */
		ent->client->resp.ctf_capstreak = 1;

	band = ent->client->resp.ctf_capstreak;
	client = ent->client;

	// give initial knife if none
	if (WPF_ALLOWED(KNIFE_NUM) && ent->client->inventory[ITEM_INDEX(GET_ITEM(KNIFE_NUM))] == 0)
		ent->client->inventory[ITEM_INDEX(GET_ITEM(KNIFE_NUM))] += 1;

	if (client->pers.chosenItem->typeNum == BAND_NUM) {
		band += 1;
		if (tgren->value > 0)	// team grenades is turned on
		{
			item = GET_ITEM(GRENADE_NUM);
			client->inventory[ITEM_INDEX(item)] = tgren->value;
		}

	}

	// give pistol clips
	if (WPF_ALLOWED(MK23_ANUM)) {
		item = GET_ITEM(MK23_ANUM);
		client->mk23_rds = client->mk23_max;
		client->inventory[ITEM_INDEX(item)] = 1*band;
	}


	player_weapon = client->pers.chosenWeapon->typeNum;
	// find out which weapon the player is holding in it's inventory
	if(client->unique_weapon_total > 0) {
		if(ent->client->inventory[ITEM_INDEX(GET_ITEM(MP5_NUM))])
			player_weapon = MP5_NUM;
		if(ent->client->inventory[ITEM_INDEX(GET_ITEM(M4_NUM))])
			player_weapon = M4_NUM;
		if(ent->client->inventory[ITEM_INDEX(GET_ITEM(M3_NUM))])
			player_weapon = M3_NUM;
		if(ent->client->inventory[ITEM_INDEX(GET_ITEM(HC_NUM))])
			player_weapon = HC_NUM;
		if(ent->client->inventory[ITEM_INDEX(GET_ITEM(SNIPER_NUM))])
			player_weapon = SNIPER_NUM;
	}


	// if player has no special weapon, give the initial one
	if (player_weapon == MP5_NUM) {
		if(client->unique_weapon_total < 1) {
			item = GET_ITEM(MP5_NUM);
			client->inventory[ITEM_INDEX(item)] = 1;
			client->unique_weapon_total = 1;
		}
		item = GET_ITEM(MP5_ANUM);
		client->inventory[ITEM_INDEX(item)] = 1*band;
		client->mp5_rds = client->mp5_max;
	} else if (player_weapon == M4_NUM) {
		if(client->unique_weapon_total < 1) {
			item = GET_ITEM(M4_NUM);
			client->inventory[ITEM_INDEX(item)] = 1;
			client->unique_weapon_total = 1;
		}
		item = GET_ITEM(M4_ANUM);
		client->inventory[ITEM_INDEX(item)] = 1*band;
		client->m4_rds = client->m4_max;
	} else if (player_weapon == M3_NUM) {
		if(client->unique_weapon_total < 1) {
			item = GET_ITEM(M3_NUM);
			client->inventory[ITEM_INDEX(item)] = 1;
			client->unique_weapon_total = 1;
		}
		item = GET_ITEM(SHELL_ANUM);
		client->inventory[ITEM_INDEX(item)] = 7*band;
		client->shot_rds = client->shot_max;
	} else if (player_weapon == HC_NUM) {
		if(client->unique_weapon_total < 1) {
			item = GET_ITEM(HC_NUM);
			client->inventory[ITEM_INDEX(item)] = 1;
			client->unique_weapon_total = 1;
		}
		item = GET_ITEM(SHELL_ANUM);
		client->inventory[ITEM_INDEX(item)] = 12*band;
		client->cannon_rds = client->cannon_max;
	} else if (player_weapon == SNIPER_NUM) {
		if(client->unique_weapon_total < 1) {
			item = GET_ITEM(SNIPER_NUM);
			client->inventory[ITEM_INDEX(item)] = 1;
			client->unique_weapon_total = 1;
		}
		item = GET_ITEM(SNIPER_ANUM);
		client->inventory[ITEM_INDEX(item)] = 10*band;
		client->sniper_rds = client->sniper_max;
	} else if (player_weapon == DUAL_NUM) {
		item = GET_ITEM(DUAL_NUM);
		client->inventory[ITEM_INDEX(item)] = 1;

		item = GET_ITEM(MK23_ANUM);
		client->inventory[ITEM_INDEX(item)] = 2*band;
		client->dual_rds = client->dual_max;
	} else if (player_weapon == KNIFE_NUM) {
		item = GET_ITEM(KNIFE_NUM);
		client->inventory[ITEM_INDEX(item)] = 10*band;
	}

	if(ent->client->bandaging || ent->client->bandage_stopped)
		was_bandaging = 1;
	
	ClientFixLegs(ent);
	ent->client->bleeding = 0;
	ent->client->bleed_remain = 0;
	ent->client->bandaging = 0;
	ent->client->attacker = NULL;

	ent->client->bandage_stopped = 0;
	ent->client->idle_weapon = 0;

	// automagically change to special in any case, it's fully reloaded
	if((client->curr_weap != player_weapon && ent->client->weaponstate == WEAPON_READY) || was_bandaging) {
		client->newweapon = client->weapon;
		ent->client->weaponstate = WEAPON_READY;
		ent->client->ps.gunframe = 0;
		ReadySpecialWeapon(ent);
	}

	// give health times cap streak
	ent->health = ent->max_health * (ent->client->resp.ctf_capstreak > 4 ? 4 : ent->client->resp.ctf_capstreak);

	if(ent->client->resp.ctf_capstreak == 2)
		gi.centerprintf(ent, "CAPTURED TWO TIMES IN A ROW!\n\nYou have been rewarded with DOUBLE health and ammo!\n\nNow go get some more!");
	else if(ent->client->resp.ctf_capstreak == 3)
		gi.centerprintf(ent, "CAPTURED THREE TIMES IN A ROW!\n\nYou have been rewarded with TRIPLE health and ammo!\n\nNow go get some more!");
	else if(ent->client->resp.ctf_capstreak == 4)
		gi.centerprintf(ent, "CAPTURED FOUR TIMES IN A ROW!\n\nYou have been rewarded with QUAD health and ammo!\n\nNow go get some more!");
	else if(ent->client->resp.ctf_capstreak > 4)
		gi.centerprintf(ent, "CAPTURED YET AGAIN!\n\nYou have been rewarded QUAD health and %d times your ammo!\n\nNow go get some more!",
				ent->client->resp.ctf_capstreak);
	else	gi.centerprintf(ent, "CAPTURED!\n\nYou have been rewarded.\n\nNow go get some more!");
}
