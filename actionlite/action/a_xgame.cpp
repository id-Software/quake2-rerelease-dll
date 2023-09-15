//-----------------------------------------------------------------------------
// PG BUND
// a_xgame.c
//
// this module contains all new and changed functions from a_game.c
//
// REMARKS:
// --------
// 1.   You have to comment the original ParseSayText in
// a_game.c completly out.
// 2.   Look for the DELETEIT comment. All comments 
// regarding DELETEIT are *not* really neccesary.
// They were done because of my compiler (caused
// compiling errors), and not because of functionality!
// Try first to de-comment the DELETEIT sections, if
// you get compiler errors too, comment them out like
// I'd done.
//
// $Id: a_xgame.c,v 1.19 2004/04/08 23:19:51 slicerdw Exp $
//
//-----------------------------------------------------------------------------
// $Log: a_xgame.c,v $
// Revision 1.19  2004/04/08 23:19:51  slicerdw
// Optimized some code, added a couple of features and fixed minor bugs
//
// Revision 1.18  2003/12/09 20:54:16  igor_rock
// Say: added %M for teammate in line of sight (as %E does for enemies)
//
// Revision 1.17  2002/02/19 10:28:43  freud
// Added to %D hit in the kevlar vest and kevlar helmet, also body for handcannon
// and shotgun.
//
// Revision 1.16  2002/02/18 13:55:35  freud
// Added last damaged players %P
//
// Revision 1.15  2001/12/24 18:06:05  slicerdw
// changed dynamic check for darkmatch only
//
// Revision 1.13  2001/12/09 14:02:11  slicerdw
// Added gl_clear check -> video_check_glclear cvar
//
// Revision 1.12  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.11  2001/07/16 19:02:06  ra
// Fixed compilerwarnings (-g -Wall).  Only one remains.
//
// Revision 1.10  2001/06/25 11:44:47  slicerdw
// New Video Check System - video_check and video_check_lockpvs no longer latched
//
// Revision 1.9  2001/06/21 00:05:30  slicerdw
// New Video Check System done -  might need some revision but works..
//
// Revision 1.6  2001/06/19 18:56:38  deathwatch
// New Last killed target system
//
// Revision 1.5  2001/05/20 15:00:19  slicerdw
// Some minor fixes and changings on Video Checking system
//
// Revision 1.4  2001/05/11 12:21:18  slicerdw
// Commented old Location support ( ADF ) With the ML/ETE Compatible one
//
// Revision 1.2  2001/05/07 21:18:34  slicerdw
// Added Video Checking System
//
// Revision 1.1.1.1  2001/05/06 17:25:24  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#include "../g_local.h"

// DetermineViewedPlayer: determine the current player you're viewing (only looks for live Enemy/Teammate)
edict_t *DetermineViewedPlayer(edict_t *ent, bool teammate)
{
	vec3_t forward, dir;
	trace_t tr;
	edict_t *who, *best;
	float bd = 0, d;
	uint32_t i;

	AngleVectors(ent->client->v_angle, forward, nullptr, nullptr);
	VectorScale(forward, 8192, forward);
	VectorAdd(ent->s.origin, forward, forward);
	PRETRACE();
	tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, forward, ent, MASK_SOLID|MASK_WATER);
	POSTTRACE();
	if (tr.fraction < 1 && tr.ent && tr.ent->client) {
		return nullptr;
	}
	AngleVectors(ent->client->v_angle, forward, nullptr, nullptr);
	best = nullptr;
	for (i = 1; i <= game.maxclients; i++)
	{
		who = g_edicts + i;
		if (!who->inuse || who == ent)
			continue;

		if (!IS_ALIVE(who) || teammate != OnSameTeam(who, ent))
			continue;

		VectorSubtract(who->s.origin, ent->s.origin, dir);
		VectorNormalize(dir);
		d = DotProduct(forward, dir);
		if (d > bd && visible(ent, who, MASK_SOLID|MASK_WATER)) {
			bd = d;
			best = who;
		}
	}

	if (bd > 0.90)
		return best;

	return nullptr;
}

static void GetViewedEnemyName(edict_t *self, char *buf)
{
	edict_t *the_enemy;

	the_enemy = DetermineViewedPlayer(self, false);
	if (the_enemy && the_enemy->client)
		strcpy(buf, the_enemy->client->pers.netname);
	else
		strcpy(buf, "no enemy");
}

static void GetViewedTeammateName(edict_t *self, char *buf)
{
	edict_t *the_teammate;

	the_teammate = DetermineViewedPlayer(self, true);
	if (the_teammate && the_teammate->client)
		strcpy(buf, the_teammate->client->pers.netname);
	else
		strcpy(buf, "no teammate");
}

static void GetViewedEnemyWeapon(edict_t *self, char *buf)
{
	edict_t *the_enemy;

	the_enemy = DetermineViewedPlayer(self, false);
	if (the_enemy && the_enemy->client && the_enemy->client->pers.weapon)
		strcpy(buf, the_enemy->client->pers.weapon->pickup_name);
	else
		strcpy(buf, "no weapon");
}

//AQ2:TNG - Slicer : Last Damage Location
static void GetLastDamagedPart(edict_t *self, char *buf)
{
	switch(self->client->last_damaged_part) {
	case LOC_HDAM:
		strcpy(buf, "head");
		break;
	case LOC_CDAM:
		strcpy(buf, "chest");
		break;
	case LOC_SDAM:
		strcpy(buf, "stomach");
		break;
	case LOC_LDAM:
		strcpy(buf, "legs");
		break;
	case LOC_KVLR_HELMET:
		strcpy(buf, "kevlar helmet");
		break;
	case LOC_KVLR_VEST:
		strcpy(buf, "kevlar vest");
		break;
	case LOC_NO:
		strcpy(buf, "body");
		break;
	default:
		strcpy(buf, "nothing");
		break;
	}
	self->client->last_damaged_part = 0;
}

//AQ2:TNG add last damaged players - Freud
static void GetLastDamagedPlayers(edict_t *self, char *buf)
{
	if (self->client->last_damaged_players[0] == '\0')
		strcpy(buf, "nobody");
	else
		Q_strlcpy(buf, self->client->last_damaged_players, PARSE_BUFSIZE);

	self->client->last_damaged_players[0] = '\0';
}
  

// Gets the location string of a location (xo,yo,zo)
// Modifies the the location areas by value of "mod"
// in the coord-inside-area tests
//
static bool GetLocation(int xo, int yo, int zo, int mod, char *buf)
{
	int count;
	int lx, ly, lz, rlx, rly, rlz;

	count = ml_count;
	while (count--)
	{
		// get next location from location base
		lx = locationbase[count].x;
		ly = locationbase[count].y;
		lz = locationbase[count].z;
		rlx = locationbase[count].rx;
		rly = locationbase[count].ry;
		rlz = locationbase[count].rz;

		// Default X-range 1500
		if (!rlx)
			rlx = 1500;

		// Default Y-range 1500
		if (!rly)
			rly = 1500;

		// Test if the (xo,yo,zo) is inside the location
		if (xo >= lx - rlx - mod && xo <= lx + rlx - mod &&
		yo >= ly - rly - mod && yo <= ly + rly - mod)
		{
			if (rlz && (zo < lz - rlz - mod || zo > lz + rlz + mod))
				continue;

			strcpy(buf, locationbase[count].desc);

			return true;
		}
	}

	strcpy(buf, "around");
	return false;
}

// Get the player location
//
bool GetPlayerLocation(edict_t *self, char *buf)
{
	if (GetLocation((int)self->s.origin[0], (int)self->s.origin[1], (int)self->s.origin[2], 0, buf))
		return true;

	return false;
}

// Get the sighted location
//
static void GetSightedLocation(edict_t *self, char *buf)
{
	vec3_t start, forward, right, end, up, offset;
	trace_t tr;

	AngleVectors (self->client->v_angle, forward, right, up);

	VectorSet(offset, 24, 8, self->viewheight);
	P_ProjectSource(self, self->s.origin, offset, forward, start);
	VectorMA(start, 8192, forward, end);

	PRETRACE();
	tr = gi.trace(start, self->mins, self->maxs, end, self, CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_DEADMONSTER);
	POSTTRACE();

	GetLocation((int)tr.endpos[0], (int)tr.endpos[1], (int)tr.endpos[2], 10, buf);
}

static void GetEnemyPosition(edict_t *self, char *buf)
{
	edict_t *the_enemy;
	vec3_t rel_pos;
	int rel_xy_pos;

	the_enemy = DetermineViewedPlayer(self, false);
	if (the_enemy && the_enemy->client)
	{
		if (GetPlayerLocation(the_enemy, buf))
			return;

		//creating relative vector from origin to destination
		VectorSubtract(self->s.origin, the_enemy->s.origin, rel_pos);

		rel_xy_pos = 0;

		//checking bounds, if one direction is less than half the other, it may
		//be ignored...
		if (fabs (rel_pos[0]) > (fabs (rel_pos[1]) * 2))
		//x width (EAST, WEST) is twice greater than y width (NORTH, SOUTH)
			rel_pos[1] = 0.0;
		if (fabs (rel_pos[1]) > (fabs (rel_pos[0]) * 2))
		//y width (NORTH, SOUTH) is twice greater than x width (EAST, WEST)
			rel_pos[0] = 0.0;

		if (rel_pos[1] > 0.0)
			rel_xy_pos |= RP_NORTH;
		else if (rel_pos[1] < 0.0)
			rel_xy_pos |= RP_SOUTH;
		if (rel_pos[0] > 0.0)
			rel_xy_pos |= RP_EAST;
		else if (rel_pos[0] < 0.0)
			rel_xy_pos |= RP_WEST;

		//creating the text message, regarding to rel_xy_pos
		strcpy (buf, "in the ");
		if (rel_xy_pos & RP_NORTH)
			strcat (buf, "north");
		if (rel_xy_pos & RP_SOUTH)
			strcat (buf, "south");
		if (rel_xy_pos & RP_EAST)
			strcat (buf, "east");
		if (rel_xy_pos & RP_WEST)
			strcat (buf, "west");
		//gi.dprintf ("rel_xy_pos: %i\n", rel_xy_pos);
		//last but not least, the height of enemy, limit for up/down: 64
		if (fabs(rel_pos[2]) > 64.0)
		{
			if (rel_pos[2] < 0.0)
				strcat(buf, ", above me");
			else
				strcat(buf, ", under me");
		}
		else
			strcat(buf, ", on the same level");
	}
	else
	{
		strcpy(buf, "somewhere");
	}
}

//AQ2:TNG Slicer - New last killed target functions
static int ReadKilledPlayers(edict_t *ent)
{
	int i, results = 0, j = 0;
	edict_t *targ;

	for (i = 0; i < MAX_LAST_KILLED; i++)
	{
		targ = ent->client->last_killed_target[i];
		if (!targ)
			break;

		if (!targ->inuse || !targ->client) //Remove disconnected players from list
		{
			for (j = i + 1; j < MAX_LAST_KILLED; j++)
				ent->client->last_killed_target[j - 1] = ent->client->last_killed_target[j];

			ent->client->last_killed_target[MAX_LAST_KILLED - 1] = NULL;
			i--;
			continue;
		}

		results++;
	}

	return results;
}

void AddKilledPlayer(edict_t *self, edict_t *ent)
{
	int kills;

	kills = ReadKilledPlayers(self);
	self->client->last_killed_target[kills % MAX_LAST_KILLED] = ent;
}

static void GetLastKilledTarget(edict_t *self, char *buf)
{
	int kills, i;

	kills = ReadKilledPlayers(self);
	if (!kills) {
		strcpy(buf, "nobody");
		return;
	}

	strcpy(buf, self->client->last_killed_target[0]->client->pers.netname);

	for (i = 1; i < kills; i++)
	{
		if (i == kills - 1)
			Q_strlcat(buf, " and ", PARSE_BUFSIZE);
		else
			Q_strlcat(buf, ", ", PARSE_BUFSIZE);

		Q_strlcat(buf, self->client->last_killed_target[i]->client->
			pers.netname, PARSE_BUFSIZE);
	}

	self->client->last_killed_target[0] = NULL;
}


static char *SeekBufEnd(char *buf)
{
	while (*buf != 0)
		buf++;

	return buf;
}


void ParseSayText(edict_t *ent, char *text, size_t size)
{
	char buf[PARSE_BUFSIZE + 256] = "\0"; //Parsebuf + chatpuf size
	char *p, *pbuf;

	p = text;
	pbuf = buf;

	while (*p != 0)
	{
		if (*p == '%')
		{
			switch (*(p + 1))
			{
			case 'H':
				GetHealth(ent, pbuf);
				pbuf = SeekBufEnd(pbuf);
				p += 2;
				break;
			case 'A':
				GetAmmo(ent, pbuf);
				pbuf = SeekBufEnd(pbuf);
				p += 2;
				break;
			case 'W':
				GetWeaponName(ent, pbuf);
				pbuf = SeekBufEnd(pbuf);
				p += 2;
				break;
			case 'I':
				GetItemName(ent, pbuf);
				pbuf = SeekBufEnd(pbuf);
				p += 2;
				break;
			case 'T':
				GetNearbyTeammates(ent, pbuf);
				pbuf = SeekBufEnd(pbuf);
				p += 2;
				break;
			case 'M':
				GetViewedTeammateName(ent, pbuf);
				pbuf = SeekBufEnd(pbuf);
				p += 2;
				break;
			case 'E':
				GetViewedEnemyName(ent, pbuf);
				pbuf = SeekBufEnd(pbuf);
				p += 2;
				break;
			case 'F':
				GetViewedEnemyWeapon(ent, pbuf);
				pbuf = SeekBufEnd(pbuf);
				p += 2;
				break;
			case 'G':
				GetEnemyPosition(ent, pbuf);
				pbuf = SeekBufEnd(pbuf);
				p += 2;
				break;
			case 'K':
				GetLastKilledTarget(ent, pbuf);
				pbuf = SeekBufEnd(pbuf);
				p += 2;
				break;
			case 'S':
				GetSightedLocation(ent, pbuf);
				pbuf = SeekBufEnd(pbuf);
				p += 2;
				break;
			case 'L':
				GetPlayerLocation(ent, pbuf);
				pbuf = SeekBufEnd(pbuf);
				p += 2;
				break;
				//AQ2:TNG Slicer Last Damage Location
			case 'D':
				GetLastDamagedPart(ent, pbuf);
				pbuf = SeekBufEnd(pbuf);
				p += 2;
				break;
				//AQ2:TNG END
				//AQ2:TNG Freud Last Player Damaged
			case 'P':
				GetLastDamagedPlayers(ent, pbuf);
				pbuf = SeekBufEnd(pbuf);
				p += 2;
				break;
				//AQ2:TNG END
			default:
				*pbuf++ = *p++;
				break;
			}
		}
		else
		{
			*pbuf++ = *p++;
		}

		if (buf[size - 1])
		{
			buf[size - 1] = 0;
			break;
		}
	}

	*pbuf = 0;
	strcpy(text, buf);
}
