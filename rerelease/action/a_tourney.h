//-----------------------------------------------------------------------------
// Tourney related stuff
//
// $Id: a_tourney.h,v 1.2 2001/09/28 13:48:34 ra Exp $
//
//-----------------------------------------------------------------------------
// $Log: a_tourney.h,v $
// Revision 1.2  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.1.1.1  2001/05/06 17:24:40  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

extern int NextOpponent;
extern int LastOpponent;

#define TOURNEYINI "tourney.ini"
#define STDSEPERATOR "\r\n "
#define TOURNEYMAXEVENTS 100


typedef enum
{
  T_NONE, T_START, T_RSTART, T_SPAWN, T_END
}
TOURNEYTIME;
typedef enum
{
  A_NONE, A_PLAY, A_PRINT, A_SET
}
TOURNEYACTION;

typedef struct
{
  int attime;
  TOURNEYTIME ttime;
  TOURNEYACTION taction;
  char info[1024];
}
tourneyindex_t;


void TourneyInit (void);
void TourneyNewPlayer (edict_t * player);
edict_t *TourneyFindPlayer (int number);
void TourneyRemovePlayer (edict_t * player);
void TourneyWinner (edict_t * player);
qboolean TourneyNewRound (void);


void TourneyTimeEvent (TOURNEYTIME ttime, int attime);
int TourneySetTime (TOURNEYTIME ttime);
