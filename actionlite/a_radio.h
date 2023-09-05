#define MAX_SOUNDFILE_PATH_LEN          32	// max length of a sound file path
#define MAX_RADIO_MSG_QUEUE_SIZE        4
#define MAX_RADIO_QUEUE_SIZE            6	// this must be at least 2 greater than the above

typedef struct radio_queue_entry_s
{
  int sndIndex;
  edict_t *from_player;
  int from_gender;		// true if female

  int length;
  bool click;
} radio_queue_entry_t;


typedef struct radio_s
{
	int delay;
	radio_queue_entry_t queue[MAX_RADIO_QUEUE_SIZE];
	int queue_size;

	bool gender;			// radiogender
	bool power_off;		// radio_power

	// Partners stuff
	bool partner_mode;	// 'radio' command using team or partner
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
bool CheckForFlood (edict_t * ent);
bool CheckForRepeat (edict_t * ent, int radioCode);
