#include "../g_local.h"

void Cmd_Say_f (edict_t * ent, bool team, bool arg0,
		bool partner_msg);

// Each of the possible radio messages and their length
struct radio_msg_t
{
	const char *msg;			// the msg name 
	int length;			// length in server frames (ie tenths of a second), rounded up
	int sndIndex;
};

static radio_msg_t male_radio_msgs[] = {
  {"1", 6, 0},
  {"2", 6, 0},
  {"3", 8, 0},
  {"4", 7, 0},
  {"5", 8, 0},
  {"6", 9, 0},
  {"7", 8, 0},
  {"8", 7, 0},
  {"9", 7, 0},
  {"10", 6, 0},
  {"back", 6, 0},
  {"cover", 7, 0},
  {"down", 13, 0},
  {"enemyd", 10, 0},
  {"enemys", 9, 0},
  {"forward", 6, 0},
  {"go", 6, 0},
  {"im_hit", 7, 0},
  {"left", 7, 0},
  {"reportin", 9, 0},
  {"right", 6, 0},
  {"taking_f", 22, 0},
  {"teamdown", 13, 0},
  {"treport", 12, 0},
  {"up", 4, 0}
  //{"END", 0, 0}			// end of list delimiter
};

static radio_msg_t female_radio_msgs[] = {
  {"1", 5, 0},
  {"2", 5, 0},
  {"3", 5, 0},
  {"4", 5, 0},
  {"5", 5, 0},
  {"6", 8, 0},
  {"7", 7, 0},
  {"8", 5, 0},
  {"9", 5, 0},
  {"10", 5, 0},
  {"back", 6, 0},
  {"cover", 5, 0},
  {"down", 6, 0},
  {"enemyd", 9, 0},
  {"enemys", 9, 0},
  {"forward", 8, 0},
  {"go", 6, 0},
  {"im_hit", 7, 0},
  {"left", 8, 0},
  {"reportin", 9, 0},
  {"right", 5, 0},
  {"taking_f", 22, 0},
  {"teamdown", 10, 0},
  {"treport", 12, 0},
  {"up", 6, 0}
  //{"END", 0, 0},			// end of list delimiter
};

static const int numMaleSnds = ( sizeof( male_radio_msgs ) / sizeof( male_radio_msgs[0] ) );
static const int numFemaleSnds = ( sizeof( female_radio_msgs ) / sizeof( female_radio_msgs[0] ) );

#define RADIO_MALE_DIR		"radio/male/"
#define	RADIO_FEMALE_DIR	"radio/female/"
#define RADIO_CLICK			0
#define RADIO_DEATH_MALE	1
#define RADIO_DEATH_FEMALE	2

radio_msg_t globalRadio[] = {
	{"radio/click.wav", 2, 0},
	{"radio/male/rdeath.wav", 27, 0},
	{"radio/female/rdeath.wav", 30, 0}
};

void PrecacheRadioSounds ()
{
	int i;
	char path[MAX_QPATH];

	globalRadio[RADIO_CLICK].sndIndex = gi.soundindex(globalRadio[RADIO_CLICK].msg);
	globalRadio[RADIO_DEATH_MALE].sndIndex = gi.soundindex(globalRadio[RADIO_DEATH_MALE].msg);
	globalRadio[RADIO_DEATH_FEMALE].sndIndex = gi.soundindex(globalRadio[RADIO_DEATH_FEMALE].msg);

	//male
	for(i = 0; i < numMaleSnds; i++)
	{
		snprintf(path, sizeof(path), "%s%s.wav", RADIO_MALE_DIR, male_radio_msgs[i].msg);
		male_radio_msgs[i].sndIndex = gi.soundindex(path);
	}

	//female
	for(i = 0; i < numFemaleSnds; i++)
	{
		snprintf(path, sizeof(path), "%s%s.wav", RADIO_FEMALE_DIR, female_radio_msgs[i].msg);
		female_radio_msgs[i].sndIndex = gi.soundindex(path);
	}
}

static void DeleteRadioQueueEntry( radio_t *radio, int entry_num )
{
	int i;

	if (radio->queue_size <= entry_num)
	{
		gi.Com_PrintFmt("DeleteRadioQueueEntry: attempt to delete out of range queue entry: %i\n", entry_num);
		return;
	}

	for (i = entry_num + 1; i < radio->queue_size; i++)
	{
		memcpy(&radio->queue[i - 1], &radio->queue[i], sizeof(radio_queue_entry_t));
	}

	radio->queue_size--;
}

// RadioThink should be called once on each player per server frame.
void RadioThink (edict_t * ent)
{
	radio_t *radio = &ent->client->resp.radio;

	// Try to clean things up, a bit....
	if (radio->partner)
	{
		if (!radio->partner->inuse ||
			radio->partner->client->resp.radio.partner != ent)
		{
			radio->partner = NULL;
		}
	}
	if (radio->partner_last_offered_to)
	{
		if (!radio->partner_last_offered_to->inuse ||
			radio->partner_last_offered_to->solid == SOLID_NOT)
		{
			radio->partner_last_offered_to = NULL;
		}
	}
	if (radio->partner_last_denied_from)
	{
		if (!radio->partner_last_denied_from->inuse ||
			radio->partner_last_denied_from->solid == SOLID_NOT)
		{
			radio->partner_last_denied_from = NULL;
		}
	}
	// ................................

	if (radio->power_off)
	{
		radio->queue_size = 0;
		return;
	}

	if (radio->delay > 0)
	{
		radio->delay--;
		if (radio->delay)
			return;
	}


	if (radio->queue_size)
	{
		edict_t *from;
		int check;

		from = radio->queue[0].from_player;

		if (!radio->queue[0].click && (!from->inuse || !IS_ALIVE(from)))
		{
			if (radio->queue[0].from_gender)
			{
				radio->queue[0].sndIndex = globalRadio[RADIO_DEATH_FEMALE].sndIndex;
				radio->queue[0].length = globalRadio[RADIO_DEATH_FEMALE].length;
			}
			else
			{
				radio->queue[0].sndIndex = globalRadio[RADIO_DEATH_MALE].sndIndex;
				radio->queue[0].length = globalRadio[RADIO_DEATH_MALE].length;
			}

			for (check = 1; check < radio->queue_size; check++)
			{
				if (!radio->queue[check].click && radio->queue[check].from_player == from)
				{
					DeleteRadioQueueEntry( radio, check );
					check--;
				}
			}
		}

		// if( ! IsInIgnoreList( ent, from ) )
		// {
		// 	unicastSound( ent, radio->queue[0].sndIndex, 1.0 );
		// 	radio->delay = radio->queue[0].length;
		// }
		DeleteRadioQueueEntry( radio, 0 ); //We can remove it here?
	}
}

static void AppendRadioMsgToQueue( radio_t *radio, int sndIndex, int len, int click, edict_t *from_player )
{
	radio_queue_entry_t *newentry;

	if (radio->queue_size >= MAX_RADIO_QUEUE_SIZE)
	{
		gi.Com_Print("AppendRadioMsgToQueue: Maximum radio queue size exceeded\n");
		return;
	}
	
	newentry = &radio->queue[radio->queue_size];

	newentry->sndIndex = sndIndex;
	newentry->from_player = from_player;
	newentry->from_gender = from_player->client->resp.radio.gender;
	newentry->length = len;
	newentry->click = click;

	radio->queue_size++;
}

static void InsertRadioMsgInQueueBeforeClick( radio_t *radio, int sndIndex, int len, edict_t *from_player )
{
	radio_queue_entry_t *newentry;

	if (radio->queue_size >= MAX_RADIO_QUEUE_SIZE)
	{
		gi.Com_Print("InsertRadioMsgInQueueBeforeClick: Maximum radio queue size exceeded\n");
		return;
	}

	newentry = &radio->queue[radio->queue_size - 1];

	memcpy( &radio->queue[radio->queue_size], newentry, sizeof(radio_queue_entry_t));

	newentry->sndIndex = sndIndex;
	newentry->from_player = from_player;
	newentry->from_gender = from_player->client->resp.radio.gender;
	newentry->length = len;
	newentry->click = 0;

	radio->queue_size++;
}

static void AddRadioMsg( radio_t *radio, int sndIndex, int len, edict_t *from_player )
{
	if (radio->queue_size == 0)
	{
		AppendRadioMsgToQueue( radio, globalRadio[RADIO_CLICK].sndIndex, globalRadio[RADIO_CLICK].length, 1, from_player );
		AppendRadioMsgToQueue( radio, sndIndex, len, 0, from_player );
		AppendRadioMsgToQueue( radio, globalRadio[RADIO_CLICK].sndIndex, globalRadio[RADIO_CLICK].length, 1, from_player );
	}
	else // we have some msgs in it already...
	{
		if (radio->queue_size < MAX_RADIO_QUEUE_SIZE)
			InsertRadioMsgInQueueBeforeClick( radio, sndIndex, len, from_player );
		// else ignore the message...
	}
}

void RadioBroadcast (edict_t * ent, int partner, const char *msg)
{
	int j, i, msg_len, numSnds;
	edict_t *other;
	radio_msg_t *radio_msgs;
	int  msg_soundIndex = 0;
	char msgname_num[8], filteredmsg[48];
	bool found = false;
	radio_t *radio;

	if (!IS_ALIVE(ent))
		return;

	radio = &ent->client->resp.radio;
	if (radio->power_off)
	{
		gi.Center_Print (ent, "Your radio is off!");
		return;
	}

	if (partner && radio->partner == NULL)
	{
		gi.LocClient_Print (ent, PRINT_HIGH, "You don't have a partner.\n");
		return;
	}

	if (radio->gender)
	{
		radio_msgs = female_radio_msgs;
		numSnds = numFemaleSnds;
	}
	else
	{
		radio_msgs = male_radio_msgs;
		numSnds = numMaleSnds;
	}

	i = found = 0;
	msg_len = 0;

	Q_strlcpy(filteredmsg, msg, sizeof(filteredmsg));

	for(i = 0; i < numSnds; i++)
	{
		if (!strcmp(radio_msgs[i].msg, filteredmsg))
		{
			found = true;
			msg_soundIndex = radio_msgs[i].sndIndex;
			msg_len = radio_msgs[i].length;
			break;
		}
	}

	if (!found)
	{
		gi.LocCenter_Print (ent, "'%s' is not a valid radio message", filteredmsg);
		return;
	}

	if (radiolog->value)
	{
		gi.LocClient_Print (NULL, PRINT_CHAT, "[%s RADIO] %s: %s\n",
			partner ? "PARTNER" : "TEAM", ent->client->pers.netname, filteredmsg);
	}

	//TempFile BEGIN
	if (strcmp (filteredmsg, "enemyd") == 0)
	{
		if (ent->client->radio_num_kills > 1 && ent->client->radio_num_kills <= 10)
		{
			// If we are reporting enemy down, add the number of kills.
			sprintf( msgname_num, "%i", ent->client->radio_num_kills );
			ent->client->radio_num_kills = 0;	// prevent from getting into an endless loop

			RadioBroadcast(ent, partner, msgname_num);	// Now THAT'S recursion! =)
		}
		ent->client->radio_num_kills = 0;
	}
//TempFile END
	//AQ2:TNG Slicer
	if (radio_repeat->value)
	{  //SLIC2 Optimization
		if (CheckForRepeat (ent, i) == false)
			return;
	}

	if (radio_max->value)
	{
		if (CheckForFlood (ent) == false)
			return;
	}


	//AQ2:TNG END
	// for (j = 1; j <= game.maxclients; j++)
	// {
	// 	other = &g_edicts[j];
	for (auto other : active_players()) {
		if (!other->inuse)
			continue;
		if (!other->client)
			continue;
		if (!OnSameTeam(ent, other))
			continue;
		if (partner && other != radio->partner)
			continue;
		AddRadioMsg( &other->client->resp.radio, msg_soundIndex, msg_len, ent );
	}
}

void Cmd_Radio_f (edict_t * ent)
{
	RadioBroadcast(ent, ent->client->resp.radio.partner_mode, gi.args());
}

void Cmd_Radiopartner_f (edict_t * ent)
{
	RadioBroadcast(ent, 1, gi.args());
}

void Cmd_Radioteam_f (edict_t * ent)
{
	RadioBroadcast(ent, 0, gi.args());
}

void Cmd_Radiogender_f (edict_t * ent)
{
	const char *arg;
	radio_t *radio;

	radio = &ent->client->resp.radio;
	arg = gi.args();
	if (arg == NULL || !*arg)
	{
		if (radio->gender)
			gi.LocClient_Print (ent, PRINT_HIGH, "Radio gender currently set to female\n");
		else
			gi.LocClient_Print (ent, PRINT_HIGH, "Radio gender currently set to male\n");
		return;
	}

	if (!strcmp(arg, "male"))
	{
		gi.LocClient_Print (ent, PRINT_HIGH, "Radio gender set to male\n");
		radio->gender = 0;
	}
	else if (!strcmp(arg, "female"))
	{
		gi.LocClient_Print (ent, PRINT_HIGH, "Radio gender set to female\n");
		radio->gender = 1;
	}
	else
	{
		gi.LocClient_Print (ent, PRINT_HIGH, "Invalid gender selection, try 'male' or 'female'\n");
	}
}

void Cmd_Radio_power_f (edict_t * ent)
{
	radio_t *radio;

	radio = &ent->client->resp.radio;

	radio->power_off = !radio->power_off;



	gi.LocCenter_Print(ent, "Radio switched %s", (radio->power_off) ? "off" : "on");

	gi.sound(ent, CHAN_VOICE, globalRadio[RADIO_CLICK].sndIndex, 1, ATTN_NORM, 1.0);
	//unicastSound(ent, globalRadio[RADIO_CLICK].sndIndex, 1.0);
}

void Cmd_Channel_f (edict_t * ent)
{
	radio_t *radio;

	radio = &ent->client->resp.radio;

	radio->partner_mode = !radio->partner_mode;
	if (radio->partner_mode)
	{
		gi.Center_Print (ent, "Channel set to 1, partner channel");
	}
	else
	{
		gi.Center_Print (ent, "Channel set to 0, team channel");
	}
}

edict_t *DetermineViewedPlayer(edict_t *ent, bool teammate);

void Cmd_Partner_f (edict_t * ent)
{
	edict_t *target;
	const char *genderstr;
	radio_t *radio, *tRadio;

	if (!IS_ALIVE(ent))
		return;

	radio = &ent->client->resp.radio;
	if (radio->partner) {

		if (radio->partner->inuse) {

			gi.LocCenter_Print( ent, "You already have a partner, %s", radio->partner->client->pers.netname );

			return;

		}

		// just in case RadioThink hasn't caught it yet... avoid any problems

		radio->partner = NULL;

	}

	target = DetermineViewedPlayer(ent, true);
	if (target == NULL) {
		gi.Center_Print (ent, "No potential partner selected");
		return;
	}

	tRadio = &target->client->resp.radio;
	if (tRadio->partner) {
		gi.LocCenter_Print (ent, "%s already has a partner", target->client->pers.netname);
		return;
	}

	if (tRadio->partner_last_offered_to == ent &&
		radio->partner_last_offered_from == target)
	{
		gi.LocCenter_Print (ent, "%s is now your partner", target->client->pers.netname);
		gi.LocCenter_Print (target, "%s is now your partner", ent->client->pers.netname);
		radio->partner = target;
		tRadio->partner = ent;
		radio->partner_last_offered_from = NULL;
		tRadio->partner_last_offered_to = NULL;
		return;
	}

	if (tRadio->partner_last_denied_from == ent)
	{
		gi.LocCenter_Print (ent, "%s has already denied you", target->client->pers.netname);
		return;
	}

	if (target == radio->partner_last_offered_to)
	{
		genderstr = GENDER_STR(target, "him", "her", "it");
		gi.LocCenter_Print (ent, "Already awaiting confirmation from %s", genderstr);
		return;
	}

	genderstr = GENDER_STR(ent, "him", "her", "it");

	gi.LocCenter_Print (ent, "Awaiting confirmation from %s", target->client->pers.netname);
	gi.LocCenter_Print (target,
		"%s offers to be your partner\n"
		"To accept:\nView %s and use the 'partner' command\n"
		"To deny:\nUse the 'deny' command",
		ent->client->pers.netname, genderstr);

	radio->partner_last_offered_to = target;
	tRadio->partner_last_offered_from = ent;
}

void Cmd_Unpartner_f (edict_t * ent)
{
	edict_t *target;
	radio_t	*radio;

	radio = &ent->client->resp.radio;
	if (radio->partner && !radio->partner->inuse)
	{ // just in case RadioThink hasn't caught it yet... avoid any problems
		radio->partner = NULL;
	}

	target = radio->partner;
	if (target == NULL) {
		gi.Center_Print (ent, "You don't have a partner");
		return;
	}

	if (target->client->resp.radio.partner == ent)
	{
		gi.LocCenter_Print (target, "%s broke your partnership", ent->client->pers.netname);
		target->client->resp.radio.partner = NULL;
	}

	gi.LocCenter_Print (ent, "You broke your partnership with %s", target->client->pers.netname);
	radio->partner = NULL;
}

void Cmd_Deny_f (edict_t * ent)
{
	edict_t *target;
	radio_t *radio;

	if (!IS_ALIVE(ent))
		return;

	radio = &ent->client->resp.radio;
	target = radio->partner_last_offered_from;
	if (target && target->inuse)
	{
		gi.LocCenter_Print (ent, "You denied %s", target->client->pers.netname);
		gi.LocCenter_Print (target, "%s has denied you", ent->client->pers.netname);
		radio->partner_last_denied_from = target;

		radio->partner_last_offered_from = NULL;
		if (target->client->resp.radio.partner_last_offered_to == ent)

			target->client->resp.radio.partner_last_offered_to = NULL;
	}
	else
	{
		gi.Center_Print (ent, "No one has offered to be your partner");
		return;
	}
}

// void Cmd_Say_partner_f (edict_t * ent)
// {
// 	if (ent->client->resp.radio.partner == NULL)
// 	{
// 		gi.LocClient_Print (ent, PRINT_HIGH, "You don't have a partner.\n");
// 		return;
// 	}

// 	Cmd_Say_f (ent, false, false, true);
// }

//SLIC2 Redesigned and optimized these two functions

bool CheckForFlood( edict_t * ent )

{

	radio_t	*radio = &ent->client->resp.radio;

	//If he's muted..

	if (radio->rd_mute) {

		if (radio->rd_mute > level.time.seconds())	// Still muted..

			return false;



		radio->rd_mute = 0;	// No longer muted..

	}

	if (!radio->rd_Count) {

		radio->rd_time = level.time.seconds();

		radio->rd_Count++;

	}

	else {

		if (level.time.seconds() - radio->rd_time < (int)(radio_time->value * 10)) {

			if (++radio->rd_Count >= (int)radio_max->value) {

				gi.LocClient_Print( ent, PRINT_HIGH,

					"[RADIO FLOOD PROTECTION]: Flood Detected, you are silenced for %d secs\n", (int)radio_ban->value );

				radio->rd_mute = level.time.seconds() + (int)(radio_ban->value * 10);

				return false;

			}

		}

		else {

			radio->rd_Count = 0;

		}

	}



	return true;

}



bool CheckForRepeat( edict_t * ent, int radioCode )

{

	radio_t	*radio = &ent->client->resp.radio;



	//If he's muted..

	if (radio->rd_mute) {

		if (radio->rd_mute > level.time.seconds())	// Still muted..

			return false;



		radio->rd_mute = 0;	// No longer muted..

	}



	if (radio->rd_lastRadio == radioCode) {	//He's trying to repeat it..

		if (level.time.seconds() - radio->rd_repTime < (int)(radio_repeat_time->value * 10)) {

			if (++radio->rd_repCount == (int)radio_repeat->value) {	//Busted

				gi.LocClient_Print( ent, PRINT_HIGH, "[RADIO FLOOD PROTECTION]: Repeat Flood Detected, you are silenced for %d secs\n", (int)radio_ban->value );

				radio->rd_mute = level.time.seconds() + (int)(radio_ban->value * 10);

				return false;

			}

		}

		else {

			radio->rd_repCount = 0;

		}

	}

	else {

		radio->rd_lastRadio = radioCode;

		radio->rd_repCount = 0;

	}

	radio->rd_repTime = level.time.seconds();

	return true;

}

