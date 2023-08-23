//-----------------------------------------------------------------------------
// a_radio.h
// 
// Include file for use with radio stuff
// -Fireblade
//
// $Id: a_radio.h,v 1.4 2004/04/08 23:19:51 slicerdw Exp $
//
//-----------------------------------------------------------------------------
// $Log: a_radio.h,v $
// Revision 1.4  2004/04/08 23:19:51  slicerdw
// Optimized some code, added a couple of features and fixed minor bugs
//
// Revision 1.3  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.2  2001/08/15 14:50:48  slicerdw
// Added Flood protections to Radio & Voice, Fixed the sniper bug AGAIN
//
// Revision 1.1.1.1  2001/05/06 17:24:32  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//----------------------------------------------------------------------------- 

#define MAX_SOUNDFILE_PATH_LEN          32	// max length of a sound file path
#define MAX_RADIO_MSG_QUEUE_SIZE        4
#define MAX_RADIO_QUEUE_SIZE            6	// this must be at least 2 greater than the above

typedef struct radio_queue_entry_s
{
  int sndIndex;
  edict_t *from_player;
  int from_gender;		// true if female

  int length;
  qboolean click;
} radio_queue_entry_t;


typedef struct radio_s
{
	int delay;
	radio_queue_entry_t queue[MAX_RADIO_QUEUE_SIZE];
	int queue_size;

	qboolean gender;			// radiogender
	qboolean power_off;		// radio_power

	// Partners stuff
	qboolean partner_mode;	// 'radio' command using team or partner
	edict_t *partner;	// current partner
	edict_t *partner_last_offered_to;	// last person I offered a partnership to
	edict_t *partner_last_offered_from;	// last person I received a partnership offer from
	edict_t *partner_last_denied_from;	// last person I denied a partnership offer from

	//Flood & Repeat
	int rd_mute;		//Time to be muted
	int rd_Count;		//Counter for the last msgs in "xx" secs allowed
	int rd_time;		//Frame for the first radio message of the ones to follow

	int rd_lastRadio;	//Code of the last radio used
	int rd_repCount;	//Counter for the number of repeated radio msgs
	int rd_repTime;		//Frame for the last repeated radio msg
} radio_t;

void RadioThink (edict_t *);
void Cmd_Radio_f (edict_t *);
void Cmd_Radiogender_f (edict_t *);
void Cmd_Radio_power_f (edict_t *);
void Cmd_Radiopartner_f (edict_t *);
void Cmd_Radioteam_f (edict_t *);
void Cmd_Channel_f (edict_t *);
void Cmd_Say_partner_f (edict_t *);
void Cmd_Partner_f (edict_t *);
void Cmd_Deny_f (edict_t *);
void Cmd_Unpartner_f (edict_t *);
void PrecacheRadioSounds ();
qboolean CheckForFlood (edict_t * ent);
qboolean CheckForRepeat (edict_t * ent, int radioCode);
