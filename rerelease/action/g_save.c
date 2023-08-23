//-----------------------------------------------------------------------------
// g_save.c
//
// $Id: g_save.c,v 1.65 2004/04/08 23:19:51 slicerdw Exp $
//
//-----------------------------------------------------------------------------
// $Log: g_save.c,v $
// Revision 1.65  2004/04/08 23:19:51  slicerdw
// Optimized some code, added a couple of features and fixed minor bugs
//
// Revision 1.64  2004/01/18 11:25:31  igor_rock
// added flashgrenades
//
// Revision 1.63  2003/06/15 21:43:53  igor
// added IRC client
//
// Revision 1.62  2003/06/15 15:34:32  igor
// - removed the zcam code from this branch (see other branch)
// - added fixes from 2.72 (source only) version
// - resetted version number to 2.72
// - This version should be exactly like the release 2.72 - just with a few
//   more fixes (which whoever did the source only variant didn't get because
//   he didn't use the CVS as he should. Shame on him.
//
// Revision 1.61  2002/09/04 11:23:10  ra
// Added zcam to TNG and bumped version to 3.0
//
// Revision 1.60  2002/03/30 17:20:59  ra
// New cvar use_buggy_bandolier to control behavior of dropping bando and grenades
//
// Revision 1.59  2002/03/28 12:10:11  freud
// Removed unused variables (compiler warnings).
// Added cvar mm_allowlock.
//
// Revision 1.58  2002/03/28 11:46:03  freud
// stat_mode 2 and timelimit 0 did not show stats at end of round.
// Added lock/unlock.
// A fix for use_oldspawns 1, crash bug.
//
// Revision 1.57  2002/03/25 23:35:19  freud
// Ghost code, use_ghosts and more stuff..
//
// Revision 1.56  2002/03/24 22:45:54  freud
// New spawn code again, bad commit last time..
//
// Revision 1.55  2002/02/26 23:13:41  freud
// Added tgren and use_classic to serverinfo string
//
// Revision 1.54  2002/02/19 09:32:47  freud
// Removed PING PONGs from CVS, not fit for release.
//
// Revision 1.53  2002/02/18 20:21:36  freud
// Added PING PONG mechanism for timely disconnection of clients. This is
// based on a similar scheme as the scheme used by IRC. The client has
// cvar ping_timeout seconds to reply or will be disconnected.
//
// Revision 1.52  2002/02/18 18:25:51  ra
// Bumped version to 2.6, fixed ctf falling and kicking of players in ctf
// uvtime
//
// Revision 1.51  2002/02/17 20:10:09  freud
// Better naming of auto_items is auto_equip, requested by Deathwatch.
//
// Revision 1.50  2002/02/17 20:01:32  freud
// Fixed stat_mode overflows, finally.
// Added 2 new cvars:
// 	auto_join (0|1), enables auto joining teams from previous map.
// 	auto_items (0|1), enables weapon and items caching between maps.
//
// Revision 1.49  2002/01/24 02:24:56  deathwatch
// Major update to Stats code (thanks to Freud)
// new cvars:
// stats_afterround - will display the stats after a round ends or map ends
// stats_endmap - if on (1) will display the stats scoreboard when the map ends
//
// Revision 1.48  2001/12/24 18:06:05  slicerdw
// changed dynamic check for darkmatch only
//
// Revision 1.46  2001/12/23 16:30:50  ra
// 2.5 ready. New stats from Freud. HC and shotgun gibbing seperated.
//
// Revision 1.45  2001/12/09 14:02:11  slicerdw
// Added gl_clear check -> video_check_glclear cvar
//
// Revision 1.44  2001/11/27 19:09:52  igor_rock
// removed tgren, limchasecam and ir from serverinfo
// changed the cvarname of the teamscore to t1, t2 and t3 - the c variables are still team1score and so on.
//
// Revision 1.43  2001/11/08 10:05:09  igor_rock
// day/night changing smoothened
// changed default for day_cycle to 10 (because of more steps)
//
// Revision 1.42  2001/11/07 11:03:35  igor_rock
// corrected some disformatting (removed linebreak at wrong position)
//
// Revision 1.41  2001/11/04 15:18:49  ra
// Unlatch wpn_flag, itm_flag, rrot, vrot
//
// Revision 1.40  2001/10/18 12:04:44  deathwatch
// Fixed sv_crlf's default (0 == dont allow)
//
// Revision 1.39  2001/09/30 03:09:34  ra
// Removed new stats at end of rounds and created a new command to
// do the same functionality.   Command is called "time"
//
// Revision 1.38  2001/09/29 19:54:04  ra
// Made a CVAR to turn off extratimingstats
//
// Revision 1.37  2001/09/28 20:45:55  ra
// Switched punching on by default
//
// Revision 1.36  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.35  2001/09/28 13:44:23  slicerdw
// Several Changes / improvements
//
// Revision 1.34  2001/09/02 20:33:34  deathwatch
// Added use_classic and fixed an issue with ff_afterround, also updated version
// nr and cleaned up some commands.
//
// Updated the VC Project to output the release build correctly.
//
// Revision 1.33  2001/08/18 01:28:06  deathwatch
// Fixed some stats stuff, added darkmatch + day_cycle, cleaned up several files, restructured ClientCommand
//
// Revision 1.32  2001/08/15 14:50:48  slicerdw
// Added Flood protections to Radio & Voice, Fixed the sniper bug AGAIN
//
// Revision 1.31  2001/08/06 14:38:45  ra
// Adding UVtime for ctf
//
// Revision 1.30  2001/08/06 03:00:49  ra
// Added FF after rounds. Please someone look at the EVIL if statments for me :)
//
// Revision 1.29  2001/07/27 00:20:55  deathwatch
// Latched wp_flags and itm_flags
//
// Revision 1.28  2001/07/16 18:28:46  ra
// Changed a 40 second hard limit on mapvoting into a cvar.
//
// Revision 1.27  2001/06/28 14:36:40  deathwatch
// Updated the Credits Menu a slight bit (added Kobra)
//
// Revision 1.26  2001/06/26 18:47:30  igor_rock
// added ctf_respawn cvar
//
// Revision 1.25  2001/06/25 11:44:47  slicerdw
// New Video Check System - video_check and video_check_lockpvs no longer latched
//
// Revision 1.24  2001/06/22 16:34:05  slicerdw
// Finished Matchmode Basics, now with admins, Say command tweaked...
//
// Revision 1.23  2001/06/21 00:05:30  slicerdw
// New Video Check System done -  might need some revision but works..
//
// Revision 1.22  2001/06/20 07:29:27  igor_rock
// corrected typo
//
// Revision 1.21  2001/06/20 07:21:21  igor_rock
// added use_warnings to enable/disable time/frags left msgs
// added use_rewards to enable/disable eimpressive, excellent and accuracy msgs
// change the configfile prefix for modes to "mode_" instead "../mode-" because
// they don't have to be in the q2 dir for doewnload protection (action dir is sufficient)
// and the "-" is bad in filenames because of linux command line parameters start with "-"
//
// Revision 1.20  2001/06/19 21:26:20  igor_rock
// changed sv_crlf to be 0 as default
//
// Revision 1.19  2001/06/18 12:36:40  igor_rock
// added new irvision mode (with reddish screen and alpha blend) and corresponding
// new cvar "new_irvision" to enable the new mode
//
// Revision 1.18  2001/06/13 08:39:13  igor_rock
// changed "cvote" to "use_cvote" (like the other votecvars)
//
// Revision 1.17  2001/06/06 18:57:14  slicerdw
// Some tweaks on Ctf and related things
//
// Revision 1.14  2001/06/01 19:18:42  slicerdw
// Added Matchmode Code
//
// Revision 1.13  2001/05/31 16:58:14  igor_rock
// conflicts resolved
//
// Revision 1.12.2.5  2001/05/27 13:44:07  igor_rock
// corredted the bug with gi.cvar and ctf_dropflag (mixed paremeters)
//
// Revision 1.12.2.4  2001/05/27 13:41:57  igor_rock
// added ctf_dropflag (default: 1)
//
// Revision 1.12.2.3  2001/05/25 18:59:52  igor_rock
// Added CTF Mode completly :)
// Support for .flg files is still missing, but with "real" CTF maps like
// tq2gtd1 the ctf works fine.
// (I hope that all other modes still work, just tested DM and teamplay)
//
// Revision 1.12.2.2  2001/05/20 18:54:19  igor_rock
// added original ctf code snippets from zoid. lib compilesand runs but
// doesn't function the right way.
// Jsut committing these to have a base to return to if something wents
// awfully wrong.
//
// Revision 1.12.2.1  2001/05/20 15:17:31  igor_rock
// removed the old ctf code completly
//
// Revision 1.12  2001/05/19 19:33:19  igor_rock
// changed itm_flags and wp_flags to Non-CVAR_LATCH (so you can change them without restart
//
// Revision 1.11  2001/05/17 14:54:47  igor_rock
// added itm_flags for teamplay and ctf
//
// Revision 1.10  2001/05/16 13:26:38  slicerdw
// Too Many Userinfo Cvars( commented some) & Enabled death messages on CTF
//
// Revision 1.9  2001/05/15 15:49:14  igor_rock
// added itm_flags for deathmatch
//
// Revision 1.8  2001/05/14 21:10:16  igor_rock
// added wp_flags support (and itm_flags skeleton - doesn't disturb in the moment)
//
// Revision 1.7  2001/05/13 01:23:01  deathwatch
// Added Single Barreled Handcannon mode, made the menus and scoreboards
// look nicer and made the voice command a bit less loud.
//
// Revision 1.6  2001/05/12 21:19:51  ra
//
//
// Added punishkills.
//
// Revision 1.5  2001/05/12 20:58:22  ra
//
//
// Adding public mapvoting and kickvoting. Its controlable via cvar's mv_public
// and vk_public (both default off)
//
// Revision 1.4  2001/05/12 00:37:03  ra
//
//
// Fixing various compilerwarnings.
//
// Revision 1.3  2001/05/07 21:18:35  slicerdw
// Added Video Checking System
//
// Revision 1.2  2001/05/07 08:32:17  mort
// Basic CTF code
// No spawns etc
// Just the cvars and flag entity
//
// Revision 1.1.1.1  2001/05/06 17:29:57  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#include "g_local.h"
#include "cgf_sfx_glass.h"


void InitCommandList( void );

field_t fields[] = {
  {"classname", FOFS (classname), F_LSTRING},
  {"origin", FOFS (s.origin), F_VECTOR},
  {"model", FOFS (model), F_LSTRING},
  {"spawnflags", FOFS (spawnflags), F_INT},
  {"speed", FOFS (speed), F_FLOAT},
  {"accel", FOFS (accel), F_FLOAT},
  {"decel", FOFS (decel), F_FLOAT},
  {"target", FOFS (target), F_LSTRING},
  {"targetname", FOFS (targetname), F_LSTRING},
  {"pathtarget", FOFS (pathtarget), F_LSTRING},
  {"deathtarget", FOFS (deathtarget), F_LSTRING},
  {"killtarget", FOFS (killtarget), F_LSTRING},
  {"combattarget", FOFS (combattarget), F_LSTRING},
  {"message", FOFS (message), F_LSTRING},
  {"team", FOFS (team), F_LSTRING},
  {"wait", FOFS (wait), F_FLOAT},
  {"delay", FOFS (delay), F_FLOAT},
  {"random", FOFS (random), F_FLOAT},
  {"move_origin", FOFS (move_origin), F_VECTOR},
  {"move_angles", FOFS (move_angles), F_VECTOR},
  {"style", FOFS (style), F_INT},
  {"count", FOFS (count), F_INT},
  {"health", FOFS (health), F_INT},
  {"sounds", FOFS (sounds), F_INT},
  {"light", 0, F_IGNORE},
  {"dmg", FOFS (dmg), F_INT},
  {"angles", FOFS (s.angles), F_VECTOR},
  {"angle", FOFS (s.angles), F_ANGLEHACK},
  {"mass", FOFS (mass), F_INT},
  {"volume", FOFS (volume), F_FLOAT},
  {"attenuation", FOFS (attenuation), F_FLOAT},
  {"map", FOFS (map), F_LSTRING},

  // temp spawn vars -- only valid when the spawn function is called
  {"lip", STOFS (lip), F_INT, FFL_SPAWNTEMP},
  {"distance", STOFS (distance), F_INT, FFL_SPAWNTEMP},
  {"height", STOFS (height), F_INT, FFL_SPAWNTEMP},
  {"noise", STOFS (noise), F_LSTRING, FFL_SPAWNTEMP},
  {"pausetime", STOFS (pausetime), F_FLOAT, FFL_SPAWNTEMP},
  {"item", STOFS (item), F_LSTRING, FFL_SPAWNTEMP},
  {"gravity", STOFS (gravity), F_LSTRING, FFL_SPAWNTEMP},
  {"sky", STOFS (sky), F_LSTRING, FFL_SPAWNTEMP},
  {"skyrotate", STOFS (skyrotate), F_FLOAT, FFL_SPAWNTEMP},
  {"skyaxis", STOFS (skyaxis), F_VECTOR, FFL_SPAWNTEMP},
  {"minyaw", STOFS (minyaw), F_FLOAT, FFL_SPAWNTEMP},
  {"maxyaw", STOFS (maxyaw), F_FLOAT, FFL_SPAWNTEMP},
  {"minpitch", STOFS (minpitch), F_FLOAT, FFL_SPAWNTEMP},
  {"maxpitch", STOFS (maxpitch), F_FLOAT, FFL_SPAWNTEMP},
  {"nextmap", STOFS (nextmap), F_LSTRING, FFL_SPAWNTEMP}
};

/*
============
PreInitGame

This will be called when the dll is first loaded, which
only happens when a new game is started or a save game
is loaded.
============
*/
void PreInitGame(void)
{

}

/*
  ============
  InitGame
  
  This will be called when the dll is first loaded, which
  only happens when a new game is started or a save game
  is loaded.
  ============
*/
void InitGame( void )
{
	cvar_t *cv;

	InitCommandList();

	gi.dprintf( "==== InitGame ====\n" );

	ReadConfigFile();
	ReadMOTDFile();

	gun_x = gi.cvar( "gun_x", "0", 0 );
	gun_y = gi.cvar( "gun_y", "0", 0 );
	gun_z = gi.cvar( "gun_z", "0", 0 );

	sv_rollspeed = gi.cvar( "sv_rollspeed", "200", 0 );
	sv_rollangle = gi.cvar( "sv_rollangle", "2", 0 );
	sv_maxvelocity = gi.cvar( "sv_maxvelocity", "2000", 0 );
	sv_gravity = gi.cvar( "sv_gravity", "800", 0 );

	dedicated = gi.cvar( "dedicated", "0", CVAR_NOSET );
	steamid = gi.cvar( "steamid", "0", CVAR_NOSET );

	sv_cheats = gi.cvar( "cheats", "0", CVAR_SERVERINFO | CVAR_LATCH );
	gi.cvar( "gamename", GAMEVERSION, CVAR_SERVERINFO | CVAR_NOSET ); 	// Removed it from Serverinfo, we already have game and gamedir
	gi.cvar( "gamedate", __DATE__, CVAR_SERVERINFO | CVAR_NOSET );
	actionversion = gi.cvar( "actionversion", "TNG " VERSION, CVAR_SERVERINFO | CVAR_NOSET );
	gi.cvar_set( "actionversion", "TNG " VERSION );

	maxclients = gi.cvar( "maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH );
	maxentities = gi.cvar( "maxentities", "1024", CVAR_LATCH );

	deathmatch = gi.cvar( "deathmatch", "1", CVAR_LATCH );
	if (!deathmatch->value) {
		gi.dprintf( "Turning deathmatch on.\n" );
		gi.cvar_forceset( "deathmatch", "1" );
	}
	cv = gi.cvar( "coop", "0", CVAR_LATCH );
	if (cv->value) {
		gi.dprintf( "Turning coop off.\n" );
		gi.cvar_forceset( "coop", "0" );
	}

	dmflags = gi.cvar( "dmflags", "0", CVAR_SERVERINFO );
	fraglimit = gi.cvar( "fraglimit", "0", CVAR_SERVERINFO );
	timelimit = gi.cvar( "timelimit", "0", CVAR_SERVERINFO );
	maptime = gi.cvar("maptime", "0:00", CVAR_SERVERINFO | CVAR_NOSET);
	capturelimit = gi.cvar( "capturelimit", "0", CVAR_SERVERINFO );
	password = gi.cvar( "password", "", CVAR_USERINFO );
	filterban = gi.cvar( "filterban", "1", 0 );
	silenceban = gi.cvar( "silenceban", "1", 0); //rekkie -- silence ban
	needpass = gi.cvar( "needpass", "0", CVAR_SERVERINFO );
	radiolog = gi.cvar( "radiolog", "0", 0 );
	teamplay = gi.cvar( "teamplay", "0", /*CVAR_SERVERINFO | */  CVAR_LATCH ); //Removed in favor of 'gm' (gamemode)
	motd_time = gi.cvar( "motd_time", "2", 0 );
	hostname = gi.cvar( "hostname", "unnamed", CVAR_SERVERINFO );
	strtwpn = gi.cvar( "dmweapon", MK23_NAME, 0 );
	actionmaps = gi.cvar( "actionmaps", "1", 0 );
	if (actionmaps->value && num_maps < 1)
	{
		gi.dprintf( "No maps were read from the config file, \"actionmaps\" won't be used.\n" );
		gi.cvar_forceset( "actionmaps", "0" );
	}
	nohud = gi.cvar( "nohud", "0", CVAR_LATCH );
	hud_team_icon = gi.cvar( "hud_team_icon", "0", 0 );
	hud_items_cycle = gi.cvar( "hud_items_cycle", "20", 0 );
	roundlimit = gi.cvar( "roundlimit", "0", CVAR_SERVERINFO );
	limchasecam = gi.cvar( "limchasecam", "0", CVAR_LATCH );
	skipmotd = gi.cvar( "skipmotd", "0", 0 );
	roundtimelimit = gi.cvar( "roundtimelimit", "0", CVAR_SERVERINFO );
	maxteamkills = gi.cvar( "maxteamkills", "0", 0 );
	twbanrounds = gi.cvar( "twbanrounds", "2", 0 );
	tkbanrounds = gi.cvar( "tkbanrounds", "2", 0 );
	noscore = gi.cvar( "noscore", "0", CVAR_LATCH );	// Was serverinfo
	hud_noscore = gi.cvar( "hud_noscore", "0", CVAR_LATCH ); // Hide score from HUD in teamplay.
	use_newscore = gi.cvar( "use_newscore", "0", 0 );
	scoreboard = gi.cvar( "scoreboard", "", 0 );
	use_voice = gi.cvar( "use_voice", "1", 0 );	//slicer
	ppl_idletime = gi.cvar( "ppl_idletime", "15", 0 );
	use_buggy_bandolier = gi.cvar( "use_buggy_bandolier", "0", 0 );
	use_tourney = gi.cvar( "use_tourney", "0", /*CVAR_SERVERINFO | */ CVAR_LATCH ); //Removed in favor of 'gm' (gamemode)
	use_3teams = gi.cvar( "use_3teams", "0", /*CVAR_SERVERINFO | */ CVAR_LATCH ); //Removed in favor of 'gmf' (gamemodeflags)
	use_randoms = gi.cvar( "use_randoms", "0", CVAR_SERVERINFO | CVAR_LATCH ); // Random weapons and items mode
	use_kickvote = gi.cvar( "use_kickvote", "1", 0 );	//slicer
	use_mapvote = gi.cvar( "use_mapvote", "1", 0 );	//slicer
	use_scramblevote = gi.cvar( "use_scramblevote", "1", 0 );	//slicer
	ctf = gi.cvar( "ctf", "0", /*CVAR_SERVERINFO | */ CVAR_LATCH );  //Removed in favor of 'gm' (gamemode)
	ctf_forcejoin = gi.cvar( "ctf_forcejoin", "", 0 );
	ctf_mode = gi.cvar( "ctf_mode", "0", 0 );
	ctf_dropflag = gi.cvar( "ctf_dropflag", "1", 0 );
	ctf_respawn = gi.cvar( "ctf_respawn", "4", 0 );
	ctf_model = gi.cvar( "ctf_model", "male", CVAR_LATCH );
	medkit_drop = gi.cvar( "medkit_drop", "0", 0 );
	medkit_time = gi.cvar( "medkit_time", "30", 0 );
	medkit_instant = gi.cvar( "medkit_instant", "0", 0 );
	dom = gi.cvar( "dom", "0", /*CVAR_SERVERINFO | */ CVAR_LATCH ); //Removed in favor of 'gmf' (gamemodeflags)
	use_grapple = gi.cvar( "use_grapple", "0", 0 );
	mv_public = gi.cvar( "mv_public", "0", 0 );	//slicer 
	vk_public = gi.cvar( "vk_public", "0", 0 );	//slicer
	punishkills = gi.cvar( "punishkills", "1", 0 );	//slicer
	mapvote_waittime = gi.cvar( "mapvote_waittime", "8", 0 );
	ff_afterround = gi.cvar( "ff_afterround", "1", 0 );
	uvtime = gi.cvar( "uvtime", "40", 0 );
	sv_gib = gi.cvar( "sv_gib", "1", 0 );
	sv_crlf = gi.cvar( "sv_crlf", "0", CVAR_LATCH ); // 0 == DONT ALLOW IT
	vrot = gi.cvar( "vrot", "0", 0 );
	rrot = gi.cvar( "rrot", "0", 0 );
	empty_rotate = gi.cvar( "empty_rotate", "0", 0 );
	empty_exec = gi.cvar( "empty_exec", "", 0 );
	llsound = gi.cvar( "llsound", "1", 0 );
	loud_guns = gi.cvar( "loud_guns", "0", 0 );
	sync_guns = gi.cvar( "sync_guns", "1", 0 );
	silentwalk = gi.cvar( "silentwalk", "0", 0 );
	slopefix = gi.cvar( "slopefix", "1", 0 );
	use_cvote = gi.cvar( "use_cvote", "0", 0 );	// Removed it from Serverinfo
	new_irvision = gi.cvar( "new_irvision", "0", 0 );
	use_rewards = gi.cvar( "use_rewards", "1", 0 );
	use_warnings = gi.cvar( "use_warnings", "1", 0 );
	check_time = gi.cvar( "check_time", "3", 0 );
	video_check = gi.cvar( "video_check", "0", 0 );
	video_max_3dfx = gi.cvar( "video_max_3dfx", "1.5", 0 );
	video_max_3dfxam = gi.cvar( "video_max_3dfxam", "1.5", 0 );
	video_max_opengl = gi.cvar( "video_max_opengl", "3.0", 0 );
	video_force_restart = gi.cvar( "video_force_restart", "0", CVAR_LATCH );
	video_check_lockpvs = gi.cvar( "video_check_lockpvs", "0", 0 );
	video_check_glclear = gi.cvar( "video_check_glclear", "0", 0 );
	video_checktime = gi.cvar( "video_checktime", "15", 0 );
	hc_single = gi.cvar( "hc_single", "1", CVAR_LATCH );	//default ON
	wp_flags = gi.cvar( "wp_flags", WPF_DEFAULT_STR, 0 );
	itm_flags = gi.cvar( "itm_flags", ITF_DEFAULT_STR, 0 );
	matchmode = gi.cvar( "matchmode", "0", CVAR_SERVERINFO | CVAR_LATCH );
	hearall = gi.cvar( "hearall", "0", 0 );	// used in matchmode
	deadtalk = gi.cvar( "deadtalk", "0", 0 );
	force_skin = gi.cvar( "force_skin", "", 0 );

	teamdm = gi.cvar( "teamdm", "0", CVAR_LATCH );
	teamdm_respawn = gi.cvar( "teamdm_respawn", "2", 0 );

	respawn_effect = gi.cvar( "respawn_effect", "0", 0 );

	item_respawnmode = gi.cvar( "item_respawnmode", "0", CVAR_LATCH );
	item_respawn = gi.cvar( "item_respawn", "59", 0 );
	weapon_respawn = gi.cvar( "weapon_respawn", "74", 0 );
	ammo_respawn = gi.cvar( "ammo_respawn", "30", 0 );

	wave_time = gi.cvar( "wave_time", "5", 0 );

	mm_forceteamtalk = gi.cvar( "mm_forceteamtalk", "0", 0 );
	mm_adminpwd = gi.cvar( "mm_adminpwd", "0", 0 );
	mm_allowlock = gi.cvar( "mm_allowlock", "1", CVAR_LATCH );
	mm_pausecount = gi.cvar( "mm_allowcount", "3", CVAR_LATCH );
	mm_pausetime = gi.cvar( "mm_pausetime", "2", CVAR_LATCH );

	teams[TEAM1].teamscore = gi.cvar("t1", "0", CVAR_SERVERINFO | CVAR_NOSET);
	teams[TEAM2].teamscore = gi.cvar("t2", "0", CVAR_SERVERINFO | CVAR_NOSET);
	teams[TEAM3].teamscore = gi.cvar("t3", "0", CVAR_SERVERINFO | CVAR_NOSET);
	stats_endmap = gi.cvar( "stats_endmap", "1", 0 );
	stats_afterround = gi.cvar( "stats_afterround", "0", 0 );

	auto_join = gi.cvar( "auto_join", "0", 0 );
	auto_equip = gi.cvar( "auto_equip", "0", 0 );
	auto_menu = gi.cvar( "auto_menu", "0", 0 );
	eventeams = gi.cvar( "eventeams", "0", 0 );
	use_balancer = gi.cvar( "use_balancer", "0", 0 );
	dm_choose = gi.cvar( "dm_choose", "0", CVAR_LATCH );
	dm_shield = gi.cvar( "dm_shield", "0", 0 );
	tourney_lca = gi.cvar( "tourney_lca", "0", 0 );
	use_punch = gi.cvar( "use_punch", "1", 0 );

	//TNG:Freud - new spawning system
	use_oldspawns = gi.cvar( "use_oldspawns", "0", CVAR_LATCH );
	//TNG:Freud - ghosts
	use_ghosts = gi.cvar( "use_ghosts", "0", CVAR_LATCH );

	radio_max = gi.cvar( "radio_max", "3", 0 );
	radio_time = gi.cvar( "radio_time", "2", 0 );
	radio_ban = gi.cvar( "radio_ban", "15", 0 );
	//SLIC2
	radio_repeat_time = gi.cvar( "radio_repeat_time", "1", 0 );
	radio_repeat = gi.cvar( "radio_repeat", "2", 0 );
	unique_weapons = gi.cvar( "weapons", "1", CVAR_SERVERINFO | CVAR_LATCH ); // zucc changed teamplay to 1
	unique_items = gi.cvar( "items", "1", CVAR_SERVERINFO | CVAR_LATCH );
	ir = gi.cvar( "ir", "1", 0 );
	knifelimit = gi.cvar( "knifelimit", "40", 0 );
	allweapon = gi.cvar( "allweapon", "0", 0 ); 	// Removed it from Serverinfo
	allitem = gi.cvar( "allitem", "0", 0 ); 	// Removed it from Serverinfo
	allow_hoarding = gi.cvar( "allow_hoarding", "0", CVAR_LATCH );
	tgren = gi.cvar( "tgren", "0", CVAR_SERVERINFO );
	//SLIC2
	/*flashgren = gi.cvar ("flashgren", "1", 0);
	flashradius = gi.cvar ("flashradius", "300", 0);
	flashtime = gi.cvar ("flashtime", "100", 0);*/
	//SLIC2
	sv_shelloff = gi.cvar( "shelloff", "1", 0 );
	shelllimit = gi.cvar( "shelllimit", "30", 0 );
	shelllife = gi.cvar( "shelllife", "1.2", 0 );
	bholelimit = gi.cvar( "bholelimit", "0", 0 );
	splatlimit = gi.cvar( "splatlimit", "0", 0 );
	bholelife = gi.cvar( "bholelife", "20", 0 );
	splatlife = gi.cvar( "splatlife", "25", 0 );
	darkmatch = gi.cvar( "darkmatch", "0", CVAR_LATCH ); // Darkmatch
	day_cycle = gi.cvar( "day_cycle", "10", 0 ); // Darkmatch cycle time.
	use_flashlight = gi.cvar( "use_flashlight", "0", 0 );
	use_classic = gi.cvar( "use_classic", "0", 0 ); // Reset Grenade Strength to 1.52

	CGF_SFX_InstallGlassSupport();	// william for CGF (glass fx)

	g_select_empty = gi.cvar( "g_select_empty", "0", CVAR_ARCHIVE );

	run_pitch = gi.cvar( "run_pitch", "0.002", 0 );
	run_roll = gi.cvar( "run_roll", "0.005", 0 );
	bob_up = gi.cvar( "bob_up", "0.005", 0 );
	bob_pitch = gi.cvar( "bob_pitch", "0.002", 0 );
	bob_roll = gi.cvar( "bob_roll", "0.002", 0 );

	// flood control
	flood_threshold = gi.cvar( "flood_threshold", "4", 0 );

	jump = gi.cvar ("jump", "0", /*CVAR_SERVERINFO|*/ CVAR_LATCH); // jumping mod -- removed from serverinfo 2022

	warmup = gi.cvar( "warmup", "0", CVAR_LATCH );
	warmup_bots = gi.cvar( "warmup_bots", "0", CVAR_LATCH );
	round_begin = gi.cvar( "round_begin", "15", 0 );
	spectator_hud = gi.cvar( "spectator_hud", "1", CVAR_LATCH );

	use_mvd2 = gi.cvar( "use_mvd2", "0", 0 );	// JBravo: q2pro MVD2 recording. 0 = off, 1 = on

	// BEGIN AQ2 ETE
	e_enhancedSlippers = gi.cvar( "e_enhancedSlippers", "0", 0);
	// END AQ2 ETE

	// 2022
	mapvote_next_limit = gi.cvar( "mapvote_next_limit", "0", 0);
	gm = gi.cvar("gm", "dm", CVAR_SERVERINFO);
	gmf = gi.cvar("gmf", "0", CVAR_SERVERINFO);
	sv_idleremove = gi.cvar("sv_idleremove", "0", 0);
  	g_spawn_items = gi.cvar("g_spawn_items", "0", CVAR_LATCH);

	// 2023
	use_killcounts = gi.cvar("use_killcounts", "0", 0);
	zoom_comp = gi.cvar("zoom_comp", "0", 0);

	// items
	InitItems();

	// initialize all clients for this game
	game.maxclients = (int)maxclients->value;
	game.clients = gi.TagMalloc( game.maxclients * sizeof(game.clients[0]), TAG_GAME );
	globals.num_edicts = game.maxclients + 1;

	// initialize all entities for this game
	game.maxentities = maxentities->value;
	clamp(game.maxentities, globals.num_edicts, MAX_EDICTS);
	g_edicts = gi.TagMalloc( game.maxentities * sizeof(g_edicts[0]), TAG_GAME );
	globals.edicts = g_edicts;
	globals.max_edicts = game.maxentities;

	CTFInit();

	//PG BUND - must be at end of gameinit:
	vInitGame();

	gi.dprintf( "Reading extra server features\n" );
	cv = gi.cvar( "sv_features", NULL, 0 );
	if (cv) {
		game.serverfeatures = (int)cv->value;

		if (game.serverfeatures & GMF_CLIENTNUM) {
			gi.dprintf( "...server supports GMF_CLIENTNUM\n" );
		}
	}

	// setup framerate parameters

	game.framerate = BASE_FRAMERATE;
	game.frametime = BASE_FRAMETIME_1000;
	game.framediv = 1;

#ifndef NO_FPS
	if( !(game.serverfeatures & GMF_VARIABLE_FPS) )
	{
		cv = gi.cvar( "version", NULL, 0 );
		if( cv && !strncmp( cv->string, "R1Q2 ", 5 ) ) // r1q2 doesnt report fps support
			game.serverfeatures |= GMF_VARIABLE_FPS;
	}
	if( game.serverfeatures & GMF_VARIABLE_FPS )
	{
		gi.dprintf ("...server supports GMF_VARIABLE_FPS\n");
		
		cv = gi.cvar( "sv_fps", NULL, CVAR_SERVERINFO );
		if( cv )
		{
			int framediv = (int) cv->value / BASE_FRAMERATE;
			
			clamp(framediv, 1, MAX_FRAMEDIV);
			
			game.framerate = framediv * BASE_FRAMERATE;
			game.frametime = BASE_FRAMETIME_1000 / framediv;
			game.framediv = framediv;
			
			gi.dprintf( "sv_fps = %d\n", (int) cv->value );
			if ((int) cv->value > 30) {
				gi.dprintf("Server FPS is %i, optimal values are 10/20/30\n", (int) cv->value);
			}
		}
		else
			gi.dprintf( "sv_fps not set\n" );
		
		gi.dprintf( "game.framerate = %d\n", game.framerate );
		gi.dprintf( "game.frametime = %f\n", game.frametime );
		gi.dprintf( "game.framediv = %d\n", game.framediv );
		gi.dprintf( "BASE_FRAMETIME_1000 = %f\n", BASE_FRAMETIME_1000 );
	}
#endif

	gi.cvar_forceset("g_features", va("%d", G_FEATURES));
	gi.cvar_forceset("g_view_predict", "1");
	gi.cvar_forceset("g_view_high", va("%d", STANDING_VIEWHEIGHT));
	gi.cvar_forceset("g_view_low", va("%d", CROUCHING_VIEWHEIGHT));
}

//=========================================================

void WriteGame (char *filename, qboolean autosave)
{
}

void ReadGame (char *filename)
{
}

void WriteLevel (char *filename)
{
}

void ReadLevel (char *filename)
{
}
