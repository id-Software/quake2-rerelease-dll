//-----------------------------------------------------------------------------
// g_local.h -- local definitions for game module
//
// $Id: g_local.h,v 1.68 2004/04/08 23:19:51 slicerdw Exp $
//
//-----------------------------------------------------------------------------
// $Log: g_local.h,v $
// Revision 1.68  2004/04/08 23:19:51  slicerdw
// Optimized some code, added a couple of features and fixed minor bugs
//
// Revision 1.67  2003/06/15 21:43:53  igor
// added IRC client
//
// Revision 1.66  2003/06/15 15:34:32  igor
// - removed the zcam code from this branch (see other branch)
// - added fixes from 2.72 (source only) version
// - resetted version number to 2.72
// - This version should be exactly like the release 2.72 - just with a few
//   more fixes (which whoever did the source only variant didn't get because
//   he didn't use the CVS as he should. Shame on him.
//
// Revision 1.65  2002/09/04 11:23:09  ra
// Added zcam to TNG and bumped version to 3.0
//
// Revision 1.64  2002/04/01 15:16:06  freud
// Stats code redone, tng_stats now much more smarter. Removed a few global
// variables regarding stats code and added kevlar hits to stats.
//
// Revision 1.63  2002/03/30 17:20:59  ra
// New cvar use_buggy_bandolier to control behavior of dropping bando and grenades
//
// Revision 1.62  2002/03/28 13:30:36  freud
// Included time played in ghost.
//
// Revision 1.61  2002/03/28 12:10:11  freud
// Removed unused variables (compiler warnings).
// Added cvar mm_allowlock.
//
// Revision 1.60  2002/03/28 11:46:03  freud
// stat_mode 2 and timelimit 0 did not show stats at end of round.
// Added lock/unlock.
// A fix for use_oldspawns 1, crash bug.
//
// Revision 1.59  2002/03/25 23:35:19  freud
// Ghost code, use_ghosts and more stuff..
//
// Revision 1.58  2002/03/25 18:32:11  freud
// I'm being too productive.. New ghost command needs testing.
//
// Revision 1.57  2002/03/24 22:45:53  freud
// New spawn code again, bad commit last time..
//
// Revision 1.56  2002/02/19 10:28:43  freud
// Added to %D hit in the kevlar vest and kevlar helmet, also body for handcannon
// and shotgun.
//
// Revision 1.55  2002/02/19 09:32:47  freud
// Removed PING PONGs from CVS, not fit for release.
//
// Revision 1.54  2002/02/18 20:21:36  freud
// Added PING PONG mechanism for timely disconnection of clients. This is
// based on a similar scheme as the scheme used by IRC. The client has
// cvar ping_timeout seconds to reply or will be disconnected.
//
// Revision 1.53  2002/02/18 13:55:35  freud
// Added last damaged players %P
//
// Revision 1.52  2002/02/17 20:10:09  freud
// Better naming of auto_items is auto_equip, requested by Deathwatch.
//
// Revision 1.51  2002/02/17 20:01:32  freud
// Fixed stat_mode overflows, finally.
// Added 2 new cvars:
// 	auto_join (0|1), enables auto joining teams from previous map.
// 	auto_items (0|1), enables weapon and items caching between maps.
//
// Revision 1.50  2002/02/01 17:49:56  freud
// Heavy changes in stats code. Removed lots of variables and replaced them
// with int arrays of MODs. This cleaned tng_stats.c up a whole lots and
// everything looks well, might need more testing.
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
// Revision 1.46  2001/12/23 21:19:41  deathwatch
// Updated stats with location and average
// cleaned it up a bit as well
//
// Revision 1.45  2001/12/23 16:30:50  ra
// 2.5 ready. New stats from Freud. HC and shotgun gibbing seperated.
//
// Revision 1.44  2001/12/09 14:02:11  slicerdw
// Added gl_clear check -> video_check_glclear cvar
//
// Revision 1.43  2001/11/07 11:41:37  igor_rock
// removed unused variables stats_shots_hd and stats_shots_m
//
// Revision 1.42  2001/10/18 12:55:35  deathwatch
// Added roundtimeleft
//
// Revision 1.41  2001/09/30 03:09:34  ra
// Removed new stats at end of rounds and created a new command to
// do the same functionality.   Command is called "time"
//
// Revision 1.40  2001/09/29 19:54:04  ra
// Made a CVAR to turn off extratimingstats
//
// Revision 1.39  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.38  2001/09/28 13:44:23  slicerdw
// Several Changes / improvements
//
// Revision 1.37  2001/09/02 20:33:34  deathwatch
// Added use_classic and fixed an issue with ff_afterround, also updated version
// nr and cleaned up some commands.
//
// Updated the VC Project to output the release build correctly.
//
// Revision 1.36  2001/08/20 00:41:15  slicerdw
// Added a new scoreboard for Teamplay with stats ( when map ends )
//
// Revision 1.35  2001/08/18 17:14:04  deathwatch
// Flashlight Added (not done yet, needs to prevent DEAD ppl from using it,
// the glow should be white and a bit smaller if possible and the daiper needs
// to be gone. Also, it should only work in 'darkmatch' I guess and it should
// make a sound when you turn it on/off.
//
// Revision 1.34  2001/08/18 01:28:06  deathwatch
// Fixed some stats stuff, added darkmatch + day_cycle, cleaned up several files, restructured ClientCommand
//
// Revision 1.33  2001/08/17 21:31:37  deathwatch
// Added support for stats
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
// Revision 1.29  2001/07/16 18:28:46  ra
// Changed a 40 second hard limit on mapvoting into a cvar.
//
// Revision 1.28  2001/06/25 11:59:20  slicerdw
// Small Fix
//
// Revision 1.27  2001/06/25 11:44:47  slicerdw
// New Video Check System - video_check and video_check_lockpvs no longer latched
//
// Revision 1.26  2001/06/22 16:34:05  slicerdw
// Finished Matchmode Basics, now with admins, Say command tweaked...
//
// Revision 1.25  2001/06/21 00:05:30  slicerdw
// New Video Check System done -  might need some revision but works..
//
// Revision 1.22  2001/06/20 07:21:21  igor_rock
// added use_warnings to enable/disable time/frags left msgs
// added use_rewards to enable/disable eimpressive, excellent and accuracy msgs
// change the configfile prefix for modes to "mode_" instead "../mode-" because
// they don't have to be in the q2 dir for doewnload protection (action dir is sufficient)
// and the "-" is bad in filenames because of linux command line parameters start with "-"
//
// Revision 1.21  2001/06/19 18:56:38  deathwatch
// New Last killed target system
//
// Revision 1.20  2001/06/18 12:36:40  igor_rock
// added new irvision mode (with reddish screen and alpha blend) and corresponding
// new cvar "new_irvision" to enable the new mode
//
// Revision 1.19  2001/06/13 08:39:13  igor_rock
// changed "cvote" to "use_cvote" (like the other votecvars)
//
// Revision 1.18  2001/06/13 08:00:25  igor_rock
// corrected stupid error (wrong changes committed)
//
// Revision 1.17  2001/06/13 07:55:17  igor_rock
// Re-Added a_match.h and a_match.c
// Added CTF Header for a_ctf.h and a_ctf.c
//
// Revision 1.16  2001/06/01 19:18:42  slicerdw
// Added Matchmode Code
//
// Revision 1.15  2001/05/31 16:58:14  igor_rock
// conflicts resolved
//
// Revision 1.14.2.4  2001/05/25 18:59:52  igor_rock
// Added CTF Mode completly :)
// Support for .flg files is still missing, but with "real" CTF maps like
// tq2gtd1 the ctf works fine.
// (I hope that all other modes still work, just tested DM and teamplay)
//
// Revision 1.14.2.3  2001/05/20 18:54:19  igor_rock
// added original ctf code snippets from zoid. lib compilesand runs but
// doesn't function the right way.
// Jsut committing these to have a base to return to if something wents
// awfully wrong.
//
// Revision 1.14.2.2  2001/05/20 15:54:43  igor_rock
// added ctf variables and include, "ctf_team" (zoid) changes to "team" (aq2:tng)
//
// Revision 1.14.2.1  2001/05/20 15:17:31  igor_rock
// removed the old ctf code completly
//
// Revision 1.14  2001/05/14 21:10:16  igor_rock
// added wp_flags support (and itm_flags skeleton - doesn't disturb in the moment)
//
// Revision 1.13  2001/05/13 01:23:01  deathwatch
// Added Single Barreled Handcannon mode, made the menus and scoreboards
// look nicer and made the voice command a bit less loud.
//
// Revision 1.12  2001/05/12 21:19:51  ra
//
//
// Added punishkills.
//
// Revision 1.11  2001/05/12 20:58:22  ra
//
//
// Adding public mapvoting and kickvoting. Its controlable via cvar's mv_public
// and vk_public (both default off)
//
// Revision 1.10  2001/05/12 13:48:58  mort
// Fixed CTF ForceSpawn bug
//
// Revision 1.9  2001/05/11 16:07:25  mort
// Various CTF bits and pieces...
//
// Revision 1.8  2001/05/11 12:21:19  slicerdw
// Commented old Location support ( ADF ) With the ML/ETE Compatible one
//
// Revision 1.6  2001/05/08 12:55:43  igor_rock
// added inlcude file <ctype.h>
//
// Revision 1.5  2001/05/07 22:03:15  slicerdw
// Added sv stuffcmd
//
// Revision 1.4  2001/05/07 21:18:34  slicerdw
// Added Video Checking System
//
// Revision 1.3  2001/05/07 08:32:17  mort
// Basic CTF code
// No spawns etc
// Just the cvars and flag entity
//
// Revision 1.2  2001/05/07 02:05:36  ra
//
//
// Added tkok command to forgive teamkills.
//
// Revision 1.1.1.1  2001/05/06 17:29:25  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#include <ctype.h>

#include "q_shared.h"
#include "q_ghud.h"

// define GAME_INCLUDE so that game.h does not define the
// short, server-visible gclient_t and edict_t structures,
// because we define the full size ones in this file
#define		GAME_INCLUDE
#include	"game.h"

#include	"a_team.h"
#include	"a_game.h"
#include	"a_menu.h"
#include	"a_radio.h"
#include	"a_xcmds.h"
#include	"a_xgame.h"
#include	"tng_ini.h"
#include	"a_tourney.h"
#include	"a_xvote.h"
#include	"a_xmenu.h"
#include	"a_vote.h"
#include	"a_match.h"
#include	"tng_stats.h"		// Adding TNG Stats File
#include	"tng_balancer.h"
#include	"tng_jump.h"
#include	"g_grapple.h"
#include	"p_antilag.h"

#define		getEnt(entnum)	(edict_t *)((char *)globals.edicts + (globals.edict_size * entnum))	//AQ:TNG Slicer - This was missing
#define		GAMEVERSION			"action"	// the "gameversion" client command will print this plus compile date


#define GMF_CLIENTNUM				0x00000001
#define GMF_PROPERINUSE             0x00000002
#define GMF_MVDSPEC					0x00000004
#define GMF_WANT_ALL_DISCONNECTS    0x00000008
#define GMF_VARIABLE_FPS			0x00000800
#define GMF_EXTRA_USERINFO			0x00001000

#ifndef NO_FPS
#define G_GMF_VARIABLE_FPS GMF_VARIABLE_FPS
#else
#define G_GMF_VARIABLE_FPS 0
#endif

#define G_FEATURES (GMF_CLIENTNUM | GMF_PROPERINUSE | GMF_MVDSPEC | GMF_WANT_ALL_DISCONNECTS | G_GMF_VARIABLE_FPS)

// protocol bytes that can be directly added to messages
#define svc_muzzleflash         1
#define svc_muzzleflash2        2
#define svc_temp_entity         3
#define svc_layout              4
#define svc_inventory           5
#define svc_sound				9
#define svc_stufftext           11
#define	svc_configstring		13

#define svc_extend				30
#define svc_userstatistic		31

//==================================================================

#define MASK_VOLUME			1
#define MASK_ATTENUATION	2
#define MASK_POSITION		4
#define MASK_ENTITY_CHANNEL 8
#define MASK_TIMEOFS		16

// view pitching times
#define DAMAGE_TIME             0.5f
#define FALL_TIME               0.3f

// edict->spawnflags
// these are set with checkboxes on each entity in the map editor
#define SPAWNFLAG_NOT_EASY              0x00000100
#define SPAWNFLAG_NOT_MEDIUM            0x00000200
#define SPAWNFLAG_NOT_HARD              0x00000400
#define SPAWNFLAG_NOT_DEATHMATCH        0x00000800
#define SPAWNFLAG_NOT_COOP              0x00001000

// edict->flags
#define FL_FLY                          0x00000001
#define FL_SWIM                         0x00000002	// implied immunity to drowining
#define FL_IMMUNE_LASER                 0x00000004
#define FL_INWATER                      0x00000008
#define FL_GODMODE                      0x00000010
#define FL_NOTARGET                     0x00000020
#define FL_IMMUNE_SLIME                 0x00000040
#define FL_IMMUNE_LAVA                  0x00000080
#define FL_PARTIALGROUND                0x00000100	// not all corners are valid
#define FL_WATERJUMP                    0x00000200	// player jumping out of water
#define FL_TEAMSLAVE                    0x00000400	// not the first on the team
#define FL_NO_KNOCKBACK                 0x00000800
#define FL_POWER_ARMOR                  0x00001000	// power armor (if any) is active
#define FL_ACCELERATE					0x20000000  // accelerative movement
#define FL_RESPAWN                      0x80000000	// used for item respawning

// edict->client->pers.spec_flags
#define SPECFL_KILLFEED					0x00000001
#define SPECFL_SPECHUD					0x00000002
#define SPECFL_SPECHUD_NEW				0x00000004

// variable server FPS
#ifndef NO_FPS
#define HZ              game.framerate
#define FRAMETIME       game.frametime
#define FRAMEDIV        game.framediv
#define FRAMESYNC       !(level.framenum % game.framediv)
#else
#define HZ              BASE_FRAMERATE
#define FRAMETIME       BASE_FRAMETIME_1000
#define FRAMEDIV        1
#define FRAMESYNC       1
#endif

#define KEYFRAME(x)   (level.framenum + (x) - (level.framenum % FRAMEDIV))

#define NEXT_FRAME(ent, func) \
    ((ent)->think = (func), (ent)->nextthink = level.framenum + 1)

#define NEXT_KEYFRAME(ent, func) \
    ((ent)->think = (func), (ent)->nextthink = KEYFRAME(FRAMEDIV))

// memory tags to allow dynamic memory to be cleaned up
#define TAG_GAME        765	// clear when unloading the dll
#define TAG_LEVEL       766	// clear when loading a new level


#define MELEE_DISTANCE  80

#define BODY_QUEUE_SIZE  8

typedef enum
{
  DAMAGE_NO,
  DAMAGE_YES,			// will take damage if hit
  DAMAGE_AIM			// auto targeting recognizes this
}
damage_t;

typedef enum
{
  WEAPON_READY,
  WEAPON_ACTIVATING,
  WEAPON_DROPPING,
  WEAPON_FIRING,

  WEAPON_END_MAG,
  WEAPON_RELOADING,
  WEAPON_BURSTING,
  WEAPON_BUSY,			// used by sniper rifle when engaging zoom, if I want to make laser sight toggle on/off  this could be used for that too...
  WEAPON_BANDAGING
}
weaponstate_t;

typedef enum
{
  AMMO_BULLETS,
  AMMO_SHELLS,
  AMMO_ROCKETS,
  AMMO_GRENADES,
  AMMO_CELLS,
  AMMO_SLUGS
}
ammo_t;


//deadflag
#define DEAD_NO                         0
#define DEAD_DYING                      1
#define DEAD_DEAD                       2
#define DEAD_RESPAWNABLE                3

//range
#define RANGE_MELEE                     0
#define RANGE_NEAR                      1
#define RANGE_MID                       2
#define RANGE_FAR                       3

//gib types
#define GIB_ORGANIC                     0
#define GIB_METALLIC                    1

//monster ai flags
#define AI_STAND_GROUND                 0x00000001
#define AI_TEMP_STAND_GROUND            0x00000002
#define AI_SOUND_TARGET                 0x00000004
#define AI_LOST_SIGHT                   0x00000008
#define AI_PURSUIT_LAST_SEEN            0x00000010
#define AI_PURSUE_NEXT                  0x00000020
#define AI_PURSUE_TEMP                  0x00000040
#define AI_HOLD_FRAME                   0x00000080
#define AI_GOOD_GUY                     0x00000100
#define AI_BRUTAL                       0x00000200
#define AI_NOSTEP                       0x00000400
#define AI_DUCKED                       0x00000800
#define AI_COMBAT_POINT                 0x00001000
#define AI_MEDIC                        0x00002000
#define AI_RESURRECTING                 0x00004000

//monster attack state
#define AS_STRAIGHT                     1
#define AS_SLIDING                      2
#define AS_MELEE                        3
#define AS_MISSILE                      4

// armor types
#define ARMOR_NONE                      0
#define ARMOR_JACKET                    1
#define ARMOR_COMBAT                    2
#define ARMOR_BODY                      3
#define ARMOR_SHARD                     4

// power armor types
#define POWER_ARMOR_NONE                0
#define POWER_ARMOR_SCREEN              1
#define POWER_ARMOR_SHIELD              2

// handedness values
#define RIGHT_HANDED                    0
#define LEFT_HANDED                     1
#define CENTER_HANDED                   2


// game.serverflags values
#define SFL_CROSS_TRIGGER_1             0x00000001
#define SFL_CROSS_TRIGGER_2             0x00000002
#define SFL_CROSS_TRIGGER_3             0x00000004
#define SFL_CROSS_TRIGGER_4             0x00000008
#define SFL_CROSS_TRIGGER_5             0x00000010
#define SFL_CROSS_TRIGGER_6             0x00000020
#define SFL_CROSS_TRIGGER_7             0x00000040
#define SFL_CROSS_TRIGGER_8             0x00000080
#define SFL_CROSS_TRIGGER_MASK          0x000000ff


// noise types for PlayerNoise
#define PNOISE_SELF                     0
#define PNOISE_WEAPON                   1
#define PNOISE_IMPACT                   2


// edict->movetype values
typedef enum
{
  MOVETYPE_NONE,		// never moves
  MOVETYPE_NOCLIP,		// origin and angles change with no interaction
  MOVETYPE_PUSH,		// no clip to world, push on box contact
  MOVETYPE_STOP,		// no clip to world, stops on box contact

  MOVETYPE_WALK,		// gravity
  MOVETYPE_STEP,		// gravity, special edge handling
  MOVETYPE_FLY,
  MOVETYPE_TOSS,		// gravity
  MOVETYPE_TOSS_NOPUSH,	// gravity, don't get pushed by doors/plats
  MOVETYPE_FLYMISSILE,		// extra size to monsters
  MOVETYPE_BOUNCE,
  MOVETYPE_BLOOD
}
movetype_t;



typedef struct
{
  int base_count;
  int max_count;
  float normal_protection;
  float energy_protection;
  int armor;
}
gitem_armor_t;


// gitem_t->flags
#define IT_WEAPON               1	// use makes active weapon
#define IT_AMMO                 2
#define IT_ARMOR                4
#define IT_STAY_COOP            8
#define IT_KEY                  16
#define IT_POWERUP              32
#define IT_ITEM                 64
#define IT_FLAG                 128


// weapon names
/*
bind 2 "use M3 Super 90 Assault Shotgun;"
bind 3 "use MP5/10 Submachinegun"
bind 4 "use Handcannon"
bind 5 "use M4 Assault Rifle"
bind 6 "use Sniper Rifle"
*/
#define MK23_NAME    "MK23 Pistol"
#define MP5_NAME     "MP5/10 Submachinegun"
#define M4_NAME      "M4 Assault Rifle"
#define M3_NAME      "M3 Super 90 Assault Shotgun"
#define HC_NAME      "Handcannon"
#define SNIPER_NAME  "Sniper Rifle"
#define DUAL_NAME    "Dual MK23 Pistols"
#define KNIFE_NAME   "Combat Knife"
#define GRENADE_NAME "M26 Fragmentation Grenade"

#define MK23_AMMO_NAME    "Pistol Magazine"
#define MP5_AMMO_NAME     "MP5 Magazine"
#define M4_AMMO_NAME      "M4 Magazine"
#define SHOTGUN_AMMO_NAME "12 Gauge Shells"
#define SNIPER_AMMO_NAME  "AP Sniper Ammo"

#define SIL_NAME     "Silencer"
#define SLIP_NAME    "Stealth Slippers"
#define BAND_NAME    "Bandolier"
#define KEV_NAME     "Kevlar Vest"
#define HELM_NAME    "Kevlar Helmet"
#define LASER_NAME   "Lasersight"

#define NO_NUM					0

#define MK23_NUM				1
#define MP5_NUM					2
#define M4_NUM					3
#define M3_NUM					4
#define HC_NUM					5
#define SNIPER_NUM				6
#define DUAL_NUM				7
#define KNIFE_NUM				8
#define GRENADE_NUM				9

#define SIL_NUM					10
#define SLIP_NUM				11
#define BAND_NUM				12
#define KEV_NUM					13
#define LASER_NUM				14
#define HELM_NUM				15

#define MK23_ANUM				16
#define MP5_ANUM				17
#define M4_ANUM					18
#define SHELL_ANUM				19
#define SNIPER_ANUM				20

#define FLAG_T1_NUM				21
#define FLAG_T2_NUM				22

#define GRAPPLE_NUM				23

#define ITEM_MAX_NUM			24

#define WEAPON_COUNT			9
#define ITEM_COUNT				6
#define AMMO_COUNT				5
#define WEAPON_FIRST			1
#define WEAPON_MAX				WEAPON_FIRST+WEAPON_COUNT
#define ITEM_FIRST				WEAPON_MAX
#define ITEM_MAX				ITEM_FIRST+ITEM_COUNT
#define AMMO_FIRST				ITEM_MAX
#define AMMO_MAX				AMMO_FIRST+AMMO_COUNT

//AQ2:TNG - Igor adding wp_flags/itm_flags
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define WPF_MASK				((1 << WEAPON_COUNT) - 1)
#define WPF_DEFAULT				511 //WPF_MASK
#define WPF_DEFAULT_STR			TOSTRING(WPF_DEFAULT)
#define WPF_ALLOWED(typeNum)	((int)wp_flags->value & items[typeNum].flag)

#define ITF_MASK				((1 << ITEM_COUNT) - 1)
#define ITF_DEFAULT				63 //ITF_MASK
#define ITF_DEFAULT_STR			TOSTRING(ITF_DEFAULT)
#define ITF_ALLOWED(typeNum)	((int)itm_flags->value & items[typeNum].flag)
//AQ2:TNG End adding flags

typedef struct itemList_s
{
	int		index;
	int		flag;
} itemList_t;

extern itemList_t items[ITEM_MAX_NUM];

// sniper modes
#define SNIPER_1X		0
#define SNIPER_2X		1
#define SNIPER_4X		2
#define SNIPER_6X		3
#define SNIPER_MODE_MAX	4

//TempFile sniper zoom moved to constants
#define SNIPER_FOV1		90
#define SNIPER_FOV2		45
#define SNIPER_FOV4		20
#define SNIPER_FOV6		10

#define GRENADE_IDLE_FIRST  40
#define GRENADE_IDLE_LAST   69
#define GRENADE_THROW_FIRST 4
#define GRENADE_THROW_LAST  9	// throw it on frame 8?


// these should be server variables, when I get around to it
//#define UNIQUE_WEAPONS_ALLOWED 2
//#define UNIQUE_ITEMS_ALLOWED   1
#define SPEC_WEAPON_RESPAWN 1
#define BANDAGE_TIME    27	// 10 = 1 second
#define BLEED_TIME      10	// 10 = 1 second is time for losing 1 health at slowest bleed rate
// Igor's back in Time to hard grenades :-)
#define GRENADE_DAMRAD_CLASSIC  170
#define GRENADE_DAMRAD          250


typedef struct gitem_s
{
  char *classname;		// spawning name

    qboolean (*pickup) (struct edict_s * ent, struct edict_s * other);
  void (*use) (struct edict_s * ent, struct gitem_s * item);
  void (*drop) (struct edict_s * ent, struct gitem_s * item);
  void (*weaponthink) (struct edict_s * ent);
  char *pickup_sound;
  char *world_model;
  int world_model_flags;
  char *view_model;

  // client side info
  char *icon;
  char *pickup_name;		// for printing on pickup
  int count_width;		// number of digits to display by icon
  int quantity;			// for ammo how much, for weapons how much is used per shot
  char *ammo;			// for weapons
  int flags;			// IT_* flags
  void *info;
  int tag;
  char *precaches;		// string of all models, sounds, and images this item will use
  int typeNum;
}
gitem_t;


//
// this structure is left intact through an entire game
// it should be initialized at dll load time, and read/written to
// the server.ssv file for savegames
//
typedef struct
{
  gclient_t *clients;		// [maxclients]

  // can't store spawnpoint in level, because
  // it would get overwritten by the savegame restore
  char spawnpoint[512];		// needed for coop respawns

  // store latched cvars here that we want to get at often
  int maxclients;
  int maxentities;

  // cross level triggers
  int serverflags;
  int serverfeatures;

  int framerate;
  float frametime;
  int framediv;

  // items
  int num_items;
	
  // stats
  char matchid[MAX_QPATH];
  int gamemode;
  int gamemodeflags;
  int roundNum;
  qboolean ai_ent_found;
  int bot_count;
}
game_locals_t;


//
// this structure is cleared as each map is entered
// it is read/written to the level.sav file for savegames
//
typedef struct
{
  int framenum;
  float time;

  char level_name[MAX_QPATH];	// the descriptive name (Outer Base, etc)

  char mapname[MAX_QPATH];	// the server name (base1, etc)

  char nextmap[MAX_QPATH];	// go here when fraglimit is hit
  //+

  char tempmap[MAX_QPATH];	//PG BUND
 
  // intermission state
  int intermission_framenum;	// time the intermission was started
  int intermission_exit;
  vec3_t intermission_origin;
  vec3_t intermission_angle;

  char *changemap;

  char statusbar[1024];  //MAX is 1536 = (MAX_QPATH * (CS_AIRACCEL - CS_STATUSBAR))
  char spec_statusbar[1024];
  int spec_statusbar_lastupdate;

  int pic_health;
  int pic_items[ITEM_MAX_NUM];
  int pic_weapon_ammo[WEAPON_MAX];
  int pic_sniper_mode[SNIPER_MODE_MAX];
  int pic_teamskin[TEAM_TOP];
  int pic_teamtag;
  
  int pic_ctf_teamtag[TEAM_TOP];
  int pic_ctf_flagbase[TEAM_TOP];
  int pic_ctf_flagtaken[TEAM_TOP];
  int pic_ctf_flagdropped[TEAM_TOP];

  int snd_fry;
  int snd_lights;
  int snd_camera;
  int snd_action;
  int snd_teamwins[TEAM_TOP];
  int snd_silencer;
  int snd_headshot;
  int snd_vesthit;
  int snd_knifethrow;
  int snd_kick;
  int snd_noammo;

  int model_null;
  int model_lsight;

  edict_t *current_entity;	// entity running from G_RunFrame

  int body_que;			// dead bodies

  int specspawn;		// determines if initial spawning has occured

  int realFramenum; //when game paused, framenum stays the same
  int pauseFrames;
  float matchTime;
  float emptyTime;
  int weapon_sound_framenum;
}
level_locals_t;


// spawn_temp_t is only used to hold entity field values that
// can be set from the editor, but aren't actualy present
// in edict_t during gameplay
typedef struct
{
  // world vars
  char *sky;
  float skyrotate;
  vec3_t skyaxis;
  char *nextmap;

  int lip;
  int distance;
  int height;
  char *noise;
  float pausetime;
  char *item;
  char *gravity;

  float minyaw;
  float maxyaw;
  float minpitch;
  float maxpitch;
}
spawn_temp_t;


typedef struct
{
  // fixed data
  vec3_t start_origin;
  vec3_t start_angles;
  vec3_t end_origin;
  vec3_t end_angles;

  int sound_start;
  int sound_middle;
  int sound_end;

  float accel;
  float speed;
  float decel;
  float distance;

  float wait;

  // state data
  int state;
  vec3_t dir;
  float current_speed;
  float move_speed;
  float next_speed;
  float remaining_distance;
  float decel_distance;
  void (*endfunc) (edict_t *);
}
moveinfo_t;


extern game_locals_t game;
extern level_locals_t level;
extern game_import_t gi;
extern game_export_t globals;
extern spawn_temp_t st;

extern int sm_meat_index;

// means of death
#define MOD_UNKNOWN                     0

#define MOD_MK23                        1
#define MOD_MP5                         2
#define MOD_M4                          3
#define MOD_M3                          4
#define MOD_HC                          5
#define MOD_SNIPER                      6
#define MOD_DUAL                        7
#define MOD_KNIFE                       8
#define MOD_KNIFE_THROWN                9
#define MOD_GRENADE                     10
#define MOD_G_SPLASH                    11
#define MOD_HANDGRENADE                 12
#define MOD_HG_SPLASH                   13
#define MOD_PUNCH                       14
#define MOD_BLASTER                     15
#define MOD_HYPERBLASTER                16
#define MOD_WATER                       17
#define MOD_SLIME                       18
#define MOD_LAVA                        19
#define MOD_CRUSH                       20
#define MOD_TELEFRAG                    21
#define MOD_FALLING                     22
#define MOD_SUICIDE                     23
#define MOD_HELD_GRENADE                24
#define MOD_EXPLOSIVE                   25
#define MOD_BARREL                      26
#define MOD_BOMB                        27
#define MOD_EXIT                        28
#define MOD_SPLASH                      29
#define MOD_TARGET_LASER                30
#define MOD_TRIGGER_HURT                31
#define MOD_HIT                         32
#define MOD_TARGET_BLASTER              33
//zucc
#define MOD_BLEEDING                    34
#define MOD_KICK                        35
#define MOD_GRAPPLE						36
#define MOD_TOTAL						37
#define MOD_FRIENDLY_FIRE               0x8000000

// types of locations that can be hit
#define LOC_HDAM		1	// head
#define LOC_CDAM		2	// chest
#define LOC_SDAM		3	// stomach
#define LOC_LDAM		4	// legs
#define LOC_KVLR_HELMET 5	// kevlar helmet	Freud, for %D
#define LOC_KVLR_VEST	6	// kevlar vest 		Freud, for %D
#define LOC_NO			7	// Shot by shotgun or handcannon
#define LOC_MAX			8

// Awards
#define ACCURACY 0
#define IMPRESSIVE 1
#define EXCELLENT 2

// Game Modes
#define GM_TEAMPLAY 0
#define GM_TEAMDM 1
#define GM_CTF 2
#define GM_TOURNEY 3
#define GM_DEATHMATCH 4
#define GM_DOMINATION 5

// Game Mode Flags
#define GMF_NONE 0
#define GMF_3TEAMS 1
//#define NEW_MODE 2       // If new game mode flags are created, use 2 for its value first
#define GMF_DARKMATCH 4
#define GMF_MATCHMODE 8

extern int meansOfDeath;
// zucc for hitlocation of death
extern int locOfDeath;
// stop an armor piercing round that hits a vest
extern int stopAP;

extern edict_t *g_edicts;

#define FOFS(x)  q_offsetof(edict_t, x)
#define STOFS(x) q_offsetof(spawn_temp_t, x)
#define LLOFS(x) (ptrdiff_t)&(((level_locals_t *)0)->x)
#define CLOFS(x) (ptrdiff_t)&(((gclient_t *)0)->x)

#define random()        ((rand () & 0x7fff) / ((float)0x7fff))
#define crandom()       (2.0 * (random() - 0.5))

#define DMFLAGS(x)     (((int)dmflags->value & x) != 0)

extern cvar_t *maxentities;
extern cvar_t *deathmatch;
extern cvar_t *dmflags;
extern cvar_t *needpass;
extern cvar_t *hostname;
extern cvar_t *teamplay;
extern cvar_t *radiolog;
extern cvar_t *motd_time;
extern cvar_t *actionmaps;
extern cvar_t *roundtimelimit;
extern cvar_t *maxteamkills;
extern cvar_t *tkbanrounds;
extern cvar_t *twbanrounds;
extern cvar_t *limchasecam;
extern cvar_t *roundlimit;
extern cvar_t *skipmotd;
extern cvar_t *nohud;
extern cvar_t *hud_team_icon;
extern cvar_t *hud_items_cycle;
extern cvar_t *noscore;
extern cvar_t *hud_noscore;
extern cvar_t *use_newscore;
extern cvar_t *scoreboard;
extern cvar_t *actionversion;
extern cvar_t *use_voice;
extern cvar_t *ppl_idletime;
extern cvar_t *use_tourney;
extern cvar_t *use_3teams;
extern cvar_t *use_randoms; // Random weapons and items mode
extern cvar_t *use_kickvote;
extern cvar_t *mv_public;
extern cvar_t *vk_public;
extern cvar_t *punishkills;
extern cvar_t *mapvote_waittime;
extern cvar_t *ff_afterround;
extern cvar_t *use_buggy_bandolier;
extern cvar_t *uvtime;
extern cvar_t *use_mapvote;	// enable map voting
extern cvar_t *use_scramblevote;
extern cvar_t *sv_gib;
extern cvar_t *sv_crlf;
extern cvar_t *vrot;
extern cvar_t *rrot;
extern cvar_t *empty_rotate;
extern cvar_t *empty_exec;
extern cvar_t *strtwpn;
extern cvar_t *llsound;
extern cvar_t *loud_guns;
extern cvar_t *sync_guns;
extern cvar_t *silentwalk;
extern cvar_t *slopefix;
extern cvar_t *use_cvote;
extern cvar_t *new_irvision;
extern cvar_t *use_rewards;
extern cvar_t *use_warnings;
extern cvar_t *video_check;	//AQ2:TNG - Slicer: For Video Checking
extern cvar_t *video_checktime;	//interval between cheat checks
extern cvar_t *video_max_3dfx;
extern cvar_t *video_max_3dfxam;
extern cvar_t *video_max_opengl;
extern cvar_t *video_check_lockpvs;
extern cvar_t *video_check_glclear;
extern cvar_t *video_force_restart;
extern cvar_t *check_time;
extern cvar_t *matchmode;
extern cvar_t *darkmatch;
extern cvar_t *day_cycle;	// If darkmatch is on, this value is the nr of seconds between each interval (day, dusk, night, dawn)
extern cvar_t *use_flashlight;  // Allow flashlight when not darkmatch?

extern cvar_t *hearall;		// used in match mode
extern cvar_t *deadtalk;
extern cvar_t *force_skin;
extern cvar_t *mm_forceteamtalk;
extern cvar_t *mm_adminpwd;
extern cvar_t *mm_allowlock;
extern cvar_t *mm_pausecount;
extern cvar_t *mm_pausetime;

extern cvar_t *teamdm;
extern cvar_t *teamdm_respawn;
extern cvar_t *respawn_effect;

extern cvar_t *item_respawnmode;

extern cvar_t *use_mvd2;	// JBravo: activate mvd2 recording on servers running q2pro

extern cvar_t *item_respawn;
extern cvar_t *weapon_respawn;
extern cvar_t *ammo_respawn;

extern cvar_t *wave_time;

extern cvar_t *use_punch;

extern cvar_t *radio_max;
extern cvar_t *radio_time;
extern cvar_t *radio_ban;
extern cvar_t *radio_repeat;
//SLIC2
extern cvar_t *radio_repeat_time;

extern cvar_t *hc_single;
extern cvar_t *wp_flags;
extern cvar_t *itm_flags;
extern cvar_t *use_classic;	// Use_classic resets weapon balance to 1.52

extern cvar_t *warmup;
extern cvar_t *warmup_bots;
extern cvar_t *round_begin;
extern cvar_t *spectator_hud;

extern cvar_t *fraglimit;
extern cvar_t *timelimit;
extern cvar_t *maptime;
extern cvar_t *capturelimit;
extern cvar_t *password;
extern cvar_t *g_select_empty;
extern cvar_t *dedicated;
extern cvar_t *steamid;

extern cvar_t *filterban;
extern cvar_t* silenceban; //rekkie -- silence ban
extern cvar_t *flood_threshold;

extern cvar_t *sv_gravity;
extern cvar_t *sv_maxvelocity;

extern cvar_t *gun_x, *gun_y, *gun_z;
extern cvar_t *sv_rollspeed;
extern cvar_t *sv_rollangle;

extern cvar_t *run_pitch;
extern cvar_t *run_roll;
extern cvar_t *bob_up;
extern cvar_t *bob_pitch;
extern cvar_t *bob_roll;

extern cvar_t *sv_cheats;
extern cvar_t *maxclients;

extern cvar_t *unique_weapons;
extern cvar_t *unique_items;
extern cvar_t *ir;		//toggles if bandolier works as infra-red sensor

extern cvar_t *knifelimit;
extern cvar_t *tgren;
extern cvar_t *allweapon;
extern cvar_t *allitem;
extern cvar_t *allow_hoarding; // Allow carrying multiple of the same special item or unique weapon.

extern cvar_t *stats_endmap; // If on (1), show the accuracy/etc stats at the end of a map
extern cvar_t *stats_afterround; // TNG Stats, collect stats between rounds

extern cvar_t *auto_join;	// Automaticly join clients to teams they were on in last map.
extern cvar_t *auto_equip;	// Remember weapons and items for players between maps.
extern cvar_t *auto_menu;	// Automatically show the join menu

extern cvar_t *dm_choose;
extern cvar_t *dm_shield;

extern cvar_t *tourney_lca; // Enables or disabled lights camera action for tourney mode

// TNG:Freud - new spawning system
extern cvar_t *use_oldspawns;
// TNG:Freud - ghosts
extern cvar_t *use_ghosts;

// zucc from action
extern cvar_t *sv_shelloff;
extern cvar_t *shelllimit;
extern cvar_t *shelllife;
extern cvar_t *splatlimit;
extern cvar_t *bholelimit;
extern cvar_t *splatlife;
extern cvar_t *bholelife;

extern cvar_t *medkit_drop;
extern cvar_t *medkit_time;
extern cvar_t *medkit_instant;

// BEGIN AQ2 ETE
extern cvar_t *e_enhancedSlippers;

// END AQ2 ETE

// 2023
extern cvar_t *use_killcounts;
extern cvar_t *am;
extern cvar_t *am_newnames;
extern cvar_t *am_botcount;
extern cvar_t *am_delay;
extern cvar_t *am_team;
extern cvar_t *zoom_comp;

// 2022
extern cvar_t *sv_limp_highping;
extern cvar_t *server_id; // Unique server_id
extern cvar_t *stat_logs; // Enables/disables logging of stats
extern cvar_t *mapvote_next_limit; // Time left that disables map voting
extern cvar_t *stat_apikey; // Stats URL key
extern cvar_t *stat_url; // Stats URL endpoint
extern cvar_t *g_spawn_items; // Enables item spawning in GS_WEAPONCHOOSE games
extern cvar_t *gm; // Gamemode
extern cvar_t *gmf; // Gamemodeflags
extern cvar_t *sv_idleremove; // Remove idlers

// Discord SDK integration with Q2Pro
extern cvar_t *cl_discord;
extern cvar_t *cl_discord_id;
extern cvar_t *cl_discord_discriminator;
extern cvar_t *cl_discord_username;
extern cvar_t *cl_discord_avatar;

#define world   (&g_edicts[0])

// item spawnflags
#define ITEM_TRIGGER_SPAWN              0x00000001
#define ITEM_NO_TOUCH                   0x00000002
// 6 bits reserved for editor flags
// 8 bits used as power cube id bits for coop games
#define DROPPED_ITEM                    0x00010000
#define DROPPED_PLAYER_ITEM             0x00020000
#define ITEM_TARGETS_USED               0x00040000

//
// fields are needed for spawning from the entity string
// and saving / loading games
//
#define FFL_SPAWNTEMP           1
#define FFL_NOSPAWN             2

typedef enum
{
  F_INT,
  F_FLOAT,
  F_LSTRING,			// string on disk, pointer in memory, TAG_LEVEL
  F_GSTRING,			// string on disk, pointer in memory, TAG_GAME
  F_VECTOR,
  F_ANGLEHACK,
  F_EDICT,			// index on disk, pointer in memory
  F_ITEM,			// index on disk, pointer in memory
  F_CLIENT,			// index on disk, pointer in memory
  F_FUNCTION,
  F_MMOVE,
  F_IGNORE
}
fieldtype_t;

typedef struct
{
  char *name;
  int ofs;
  fieldtype_t type;
  int flags;
}
field_t;


extern field_t fields[];
extern gitem_t itemlist[];

//
// g_cmds.c
//
qboolean FloodCheck(edict_t *ent);
void	Cmd_Help_f(edict_t *ent);
void	Cmd_Score_f(edict_t *ent);
void	Cmd_Inven_f(edict_t *ent);
edict_t *LookupPlayer(edict_t *ent, const char *text, qboolean checkNUM, qboolean checkNick);

//
// g_items.c
//
void PrecacheItem (gitem_t * it);
void InitItems (void);
void SetItemNames (void);
gitem_t *FindItem (char *pickup_name);
gitem_t *FindItemByClassname (char *classname);
gitem_t *FindItemByNum (int num);
#define ITEM_INDEX(x) ((x)-itemlist)
#define INV_AMMO(ent, num) ((ent)->client->inventory[items[(num)].index])
#define GET_ITEM(num) (&itemlist[items[(num)].index])
edict_t *Drop_Item (edict_t * ent, gitem_t * item);
void SetRespawn (edict_t * ent, float delay);
void ChangeWeapon (edict_t * ent);
void PrecacheItems( void );
void SpawnItem (edict_t * ent, gitem_t * item);
void Think_Weapon (edict_t * ent);
qboolean Add_Ammo (edict_t * ent, gitem_t * item, int count);
void Touch_Item (edict_t * ent, edict_t * other, cplane_t * plane,
		 csurface_t * surf);

//
// g_utils.c
//
qboolean KillBox (edict_t *ent);
void	G_ProjectSource(vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result);
edict_t *G_Find(edict_t *from, ptrdiff_t fieldofs, char *match);
edict_t *findradius(edict_t *from, vec3_t org, float rad);
edict_t *G_PickTarget(char *targetname);
void	G_UseTargets(edict_t *ent, edict_t * activator);
void	G_SetMovedir(vec3_t angles, vec3_t movedir);

void	G_InitEdict(edict_t *e);
edict_t *G_Spawn(void);
edict_t *G_Spawn_Decal( void );
void	G_FreeEdict(edict_t *e);

void	G_TouchTriggers(edict_t *ent);
void	G_TouchSolids(edict_t *ent);

size_t  G_HighlightStr(char *dst, const char *src, size_t size);

char	*G_CopyString(char *in);
qboolean visible(edict_t *self, edict_t *other, int mask);

// Re-enabled for bots
float *tv (float x, float y, float z);
char *vtos (vec3_t v);

float vectoyaw (vec3_t vec);
void vectoangles (vec3_t vec, vec3_t angles);

//
// g_combat.c
//
qboolean OnSameTeam (edict_t * ent1, edict_t * ent2);
qboolean CanDamage (edict_t * targ, edict_t * inflictor);
void T_Damage (edict_t * targ, edict_t * inflictor, edict_t * attacker,
	       vec3_t dir, vec3_t point, vec3_t normal, int damage,
	       int knockback, int dflags, int mod);
void T_RadiusDamage (edict_t * inflictor, edict_t * attacker, float damage,
		     edict_t * ignore, float radius, int mod);

// damage flags
#define DAMAGE_RADIUS                   0x00000001	// damage was indirect
#define DAMAGE_NO_ARMOR                 0x00000002	// armour does not protect from this damage
#define DAMAGE_ENERGY                   0x00000004	// damage is from an energy based weapon
#define DAMAGE_NO_KNOCKBACK             0x00000008	// do not affect velocity, just view angles
#define DAMAGE_BULLET                   0x00000010	// damage is from a bullet (used for ricochets)
#define DAMAGE_NO_PROTECTION            0x00000020	// armor, shields, invulnerability, and godmode have no effect

#define DEFAULT_BULLET_HSPREAD                  300
#define DEFAULT_BULLET_VSPREAD                  500
#define DEFAULT_SHOTGUN_HSPREAD                 1000
#define DEFAULT_SHOTGUN_VSPREAD                 500
#define DEFAULT_DEATHMATCH_SHOTGUN_COUNT        12
#define DEFAULT_SHOTGUN_COUNT                   12
#define DEFAULT_SSHOTGUN_COUNT                  20

//
// g_misc.c
//
void ThrowHead (edict_t * self, char *gibname, int damage, int type);
void ThrowClientHead (edict_t * self, int damage);
void ThrowGib (edict_t * self, char *gibname, int damage, int type);
void BecomeExplosion1 (edict_t * self);
void SP_misc_teleporter_dest(edict_t* ent);

//
// g_weapon.c
//
void ThrowDebris (edict_t * self, char *modelname, float speed,
		  vec3_t origin);
void fire_bullet (edict_t * self, vec3_t start, vec3_t aimdir, int damage,
		  int kick, int hspread, int vspread, int mod);
void fire_shotgun (edict_t * self, vec3_t start, vec3_t aimdir, int damage,
		   int kick, int hspread, int vspread, int count, int mod);
//SLIC2 changed argument name hyper to hyperb
void fire_blaster (edict_t * self, vec3_t start, vec3_t aimdir, int damage,
		   int speed, int effect, qboolean hyperb);
void fire_grenade2 (edict_t * self, vec3_t start, vec3_t aimdir, int damage,
	int speed, int timer, float damage_radius,
		    qboolean held);

void kick_attack(edict_t *ent);
void punch_attack(edict_t *ent);
int knife_attack(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick);
void knife_throw(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed);
void knife_touch(edict_t* ent, edict_t* other, cplane_t* plane, csurface_t* surf);
void fire_bullet_sparks(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int mod);
void fire_bullet_sniper(edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int mod);
void setFFState(edict_t* ent);

//
// g_client.c
//
void respawn (edict_t * ent);
void BeginIntermission (edict_t * targ);
void PutClientInServer (edict_t * ent);
void InitBodyQue (void);
void ClientBeginServerFrame (edict_t * ent);

//
// g_player.c
//
void player_pain (edict_t * self, edict_t * other, float kick, int damage);
void player_die (edict_t * self, edict_t * inflictor, edict_t * attacker,
		 int damage, vec3_t point);

//
// g_svcmds.c
//
void ServerCommand (void);
qboolean SV_FilterPacket (char *from, int *temp);
qboolean SV_FilterSBPacket(char* from, int* temp); //rekkie -- silence ban
void Kick_Client (edict_t * ent);
qboolean Ban_TeamKiller (edict_t * ent, int rounds);

//
// p_view.c
//
void ClientEndServerFrame (edict_t * ent);
void SetAnimation( edict_t *ent, int frame, int anim_end, int anim_priority );
qboolean OnLadder( edict_t *ent );

//
// p_hud.c
//
void MoveClientToIntermission (edict_t * client);
void G_SetStats (edict_t * ent);
void G_CheckChaseStats (edict_t * ent);
void ValidateSelectedItem (edict_t * ent);
void DeathmatchScoreboard(edict_t* ent);
void DeathmatchScoreboardMessage (edict_t * client, edict_t * killer);

//
// g_pweapon.c
//
void PlayerNoise (edict_t * who, vec3_t where, int type);

//
// g_phys.c
//
void G_RunEntity (edict_t * ent);

//
// g_main.c
//
void EndDMLevel (void);
void ExitLevel (void);

//
// g_chase.c
//
void UpdateChaseCam (edict_t * ent);
int ChaseTargetGone (edict_t * ent);
void NextChaseMode( edict_t *ent );
void SetChase( edict_t *ent, edict_t *target );
void ChaseNext (edict_t * ent);
void ChasePrev (edict_t * ent);
void GetChaseTarget (edict_t * ent);


//
// g_spawn.c
//
void ChangePlayerSpawns();
void ED_CallSpawn( edict_t *ent );
char* ED_NewString(char* string);
void G_UpdateSpectatorStatusbar( void );
void G_UpdatePlayerStatusbar( edict_t *ent, int force );
int Gamemodeflag(void);
int Gamemode(void);
//
// p_client.c
//
edict_t* SelectRandomDeathmatchSpawnPoint(void);
edict_t* SelectFarthestDeathmatchSpawnPoint(void);
float PlayersRangeFromSpot(edict_t* spot);
void ClientLegDamage(edict_t* ent);
void ClientFixLegs(edict_t *ent);
void ClientUserinfoChanged(edict_t* ent, char* userinfo);
void ClientDisconnect(edict_t* ent);
void CopyToBodyQue(edict_t* ent);
void Announce_Reward(edict_t *ent, int rewardType);

//p_weapon.c
void Weapon_Generic( edict_t * ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST,
	int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST,
	int FRAME_RELOAD_LAST, int FRAME_LASTRD_LAST,
	int *pause_frames, int *fire_frames,
	void( *fire ) (edict_t * ent) );
void PlayWeaponSound( edict_t *ent );

void P_ProjectSource(gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result);
void weapon_grenade_fire(edict_t* ent, qboolean held);
void InitTookDamage(void);
void ProduceShotgunDamageReport(edict_t*);

//tng_stats.c
void StatBotCheck(void);

//============================================================================

// client_t->anim_priority
#define ANIM_BASIC              0	// stand / run
#define ANIM_WAVE               1
#define ANIM_JUMP               2
#define ANIM_PAIN               3
#define ANIM_ATTACK             4
#define ANIM_DEATH              5
// in 3.20 there is #define ANIM_REVERSE 6    -FB
// zucc vwep - based on info from Hentai
#define ANIM_REVERSE            -1

#define MAX_SKINLEN				32

#define MAX_GUNSTAT MOD_GRENADE //Max MOD to track

typedef enum {
	GENDER_MALE,
	GENDER_FEMALE,
	GENDER_NEUTRAL
} gender_t;

typedef enum {
	LAYOUT_NONE,
	LAYOUT_SCORES,
	LAYOUT_SCORES2,
	LAYOUT_MENU
} layout_t;

#define GENDER_STR( ent, he, she, it ) (((ent)->client->pers.gender == GENDER_MALE) ? he : (((ent)->client->pers.gender == GENDER_FEMALE) ? she : it))

typedef struct gunStats_s
{
	int shots;		//Number of shots
	int hits;		//Number of hits
	int headshots;	//Number of headshots
	int kills;		//Number of kills
	int damage;		//Damage dealt
} gunStats_t;


// client data that stays across multiple level loads
typedef struct
{
	char userinfo[MAX_INFO_STRING];
	char netname[16];
	char ip[64];
	int hand;

	qboolean connected;		// a loadgame will leave valid entities that
	// just don't have a connection yet

	qboolean silence_banned; //rekkie -- silence ban

	int admin;

	gender_t	gender;
	qboolean spectator;
	int firing_style;

	qboolean mvdspec;

	xmenu_t x_menu;

	gitem_t *chosenItem;		// item for teamplay
	gitem_t *chosenWeapon;	// weapon for teamplay

	int menu_shown;		// has the main menu been shown
	qboolean dm_selected;		// if dm weapon selection has been done once

	// Reki - added these options, controllable via userinfo cvar (Reki update 2/24/23, controllable via cvarsync as well)
	int limp_nopred;
	int spec_flags;
	qboolean antilag_optout;

	int mk23_mode;		// firing mode, semi or auto
	int mp5_mode;
	int m4_mode;
	int knife_mode;
	int grenade_mode;
	int hc_mode;
	int id;			// id command on or off
	int irvision;			// ir on or off (only matters if player has ir device, currently bandolier)

	ignorelist_t ignorelist;
}
client_persistant_t;

// client data that stays across deathmatch respawns
typedef struct
{
  int enterframe;		// level.framenum the client entered the game

  int score;			// frags, etc

  vec3_t cmd_angles;		// angles sent over in the last command

  int sniper_mode;		//level of zoom

  int kills;			// real kills

  int deaths;			// deaths

  int damage_dealt;		// keep track of damage dealt by player to other players

  int team;			// team the player is on
  int subteam;

  int ctf_state;
  int ctf_caps;
  int ctf_capstreak;
  int ctf_lasthurtcarrier;
  int ctf_lastreturnedflag;
  int ctf_flagsince;
  int ctf_lastfraggedcarrier;

  int joined_team;		// last frame # at which the player joined a team
  int lastWave;			//last time used wave

  radio_t radio;
 
  int motd_refreshes;
  int last_motd_refresh;
  edict_t *last_chase_target;	// last person they chased, to resume at the same place later...

  // Number of team kills this game
  int team_kills;
  int team_wounds;
  
  int idletime;
  int totalidletime;
  int tourneynumber;
  edict_t *kickvote;

  char *mapvote;		// pointer to map voted on (if any)
  char *cvote;			// pointer to config voted on (if any)
  qboolean scramblevote;	// want scramble

  int ignore_time;		// framenum when the player called ignore - to prevent spamming
	
  int stat_mode;    		// Automatical Send of statistics to client
  int stat_mode_intermission;

  int shotsTotal;					//Total number of shots
  int hitsTotal;					//Total number of hits
  int streakKills;					//Kills in a row
  int streakHS;						//Headshots in a Row
  int streakKillsHighest;			//Highest kills in a row
  int streakHSHighest;				//Highest headshots in a Row

  int hitsLocations[LOC_MAX];		//Number of hits for different locations
  gunStats_t gunstats[MOD_TOTAL]; //Number of shots/hits for different guns, adjusted to MOD_TOTAL to allow grenade, kick and punch stats

  //AQ2:TNG - Slicer: Video Checking and further Cheat cheking vars
  char vidref[16];
  char gldriver[16];
  float gllockpvs;
  float glmodulate;
  float glclear;
  float gldynamic;
  float glbrightness;
  qboolean checked;
  int checkframe[3];

  int				penalty;

  float jmp_highspeed;
  float jmp_falldmglast;
  vec3_t jmp_teleport_origin;
  vec3_t jmp_teleport_v_angle;
  qboolean jmp_teleport_ducked;
  //char skin[MAX_SKINLEN];
}
client_respawn_t;

// this structure is cleared on each PutClientInServer(),
// except for 'client->pers'
struct gclient_s
{
	// known to server
	player_state_t	ps;		// communicated by server to clients
	int				ping;

	// known to compatible server
	int				clientNum;

	// private to game
	client_persistant_t	pers;
	client_respawn_t	resp;
	pmove_state_t		old_pmove;	// for detecting out-of-pmove changes

	layout_t	layout;		// set layout stat
	qboolean	showinventory;	// set layout stat

	pmenuhnd_t	menu;		// current menu

	int			ammo_index;

	int			buttons;
	int			oldbuttons;
	int			latched_buttons;

	int			weapon_last_activity;

	gitem_t		*newweapon;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int			damage_armor;		// damage absorbed by armor
	int			damage_parmor;		// damage absorbed by power armor
	int			damage_blood;		// damage taken out of health
	int			damage_knockback;	// impact damage
	vec3_t		damage_from;		// origin for vector calculation

	float		killer_yaw;			// when dead, look at killer

	weaponstate_t	weaponstate;
	vec3_t		kick_angles;		// weapon kicks
	vec3_t		kick_origin;
	float		v_dmg_roll, v_dmg_pitch, v_dmg_time;	// damage kicks
	float		fall_time, fall_value;	// for view drop on fall
	float		damage_alpha;
	float		bonus_alpha;
	vec3_t		damage_blend;
	vec3_t		v_angle;			// aiming direction
	float		bobtime;			// so off-ground doesn't change it
	vec3_t		oldviewangles;
	vec3_t		oldvelocity;
	qboolean	ladder, old_ladder;

	int			next_drown_framenum;
	int			old_waterlevel;
	int			breather_sound;

	int			machinegun_shots;	// for weapon raising

	// animation vars
	int			anim_end;
	int			anim_priority;
	qboolean	anim_duck;
	qboolean	anim_run;
	int			anim_started;

	// powerup timers
	int			quad_framenum;
	int			invincible_framenum;
	int			breather_framenum;
	int			enviro_framenum;

	qboolean	grenade_blew_up;
	int			grenade_framenum;
	int			silencer_shots;
	int			weapon_sound;

	int			pickup_msg_framenum;
	int			respawn_framenum;		// can respawn when time > this
  
	edict_t		*chase_target;
	int			chase_mode;

	int			selected_item;
	int			inventory[MAX_ITEMS];

	// ammo capacities
	int			max_pistolmags;
	int			max_shells;
	int			max_mp5mags;
	int			max_m4mags;
	int			max_sniper_rnds;

	int			mk23_max;
	int			mk23_rds;

	int			dual_max;
	int			dual_rds;
	int			shot_max;
	int			shot_rds;
	int			sniper_max;
	int			sniper_rds;
	int			mp5_max;
	int			mp5_rds;
	int			m4_max;
	int			m4_rds;
	int			cannon_max;
	int			cannon_rds;
	int			knife_max;
	int			grenade_max;

	gitem_t		*weapon;
	gitem_t		*lastweapon;

	int			curr_weap;		// uses NAME_NUM values

	int			fired;			// keep track of semi auto
	int			burst;			// remember if player is bursting or not
	int			fast_reload;	// for shotgun/sniper rifle
	int			idle_weapon;	// how many frames to keep our weapon idle
	int			desired_fov;	// what fov does the player want? (via zooming)
	int			desired_zoom;	// either 0, 1, 2, 4 or 6. This is set to 0 if no zooming shall be done, and is set to 0 after zooming is done.

	int			unique_weapon_total;
	int			unique_item_total;
	int			drop_knife;
	int			knife_sound;		// we attack several times when slashing but only want 1 sound

	int			punch_framenum;
	qboolean	punch_desired;	//controlled in ClientThink

	int			reload_attempts;
	int			weapon_attempts;

	qboolean	autoreloading;	//used for dual -> mk23 change with reloading

	int			took_damage;		//Took damage from multihit weapons

	int			no_sniper_display;
  
	int			bandaging;
	int			bandage_stopped;
	qboolean	weapon_after_bandage_warned;	// to fix message bug when calling weapon while bandaging

	int			leg_damage;
	int			leg_dam_count;
	int			leg_noise;
	int			leghits;

	int			bleeding;			//remaining points to bleed away
	int			bleed_remain;
	vec3_t		bleedloc_offset;	// location of bleeding (from origin)
	int			bleeddelay;			// how long until we bleed again

	int			doortoggle;			// set by player with opendoor command

	edict_t		*attacker;		// keep track of the last person to hit us
	int			attacker_mod;	// and how they hit us
	int			attacker_loc;	// location of the hit

	int			push_timeout;	// timeout for how long an attacker will get fall death credit

	int			jumping;

	antilag_t	antilag_state;

	// Number of teammate woundings this game and a "before attack" tracker
	int			team_wounds_before;
	int			ff_warning;

	int			radio_num_kills;

	int			last_damaged_part;
	char		last_damaged_players[256];
	edict_t		*last_killed_target[MAX_LAST_KILLED];

	int			uvTime;
  
	qboolean	team_force;		// are we forcing a team change

	edict_t		*lasersight; // laser
	edict_t		*flashlight; // Flashlight

	int			medkit;

	edict_t		*ctf_grapple;		// entity of grapple
	int			ctf_grapplestate;		// true if pulling
	int			ctf_grapplereleaseframe;	// frame of grapple release

	// used for extrapolation
	usercmd_t	cmd_last;

	// visiblity mask
	unsigned int dimension_observe;
};


struct edict_s
{
	entity_state_t s;
	struct gclient_s *client;	// NULL if not a player
								// the server expects the first part
								// of gclient_s to be a player_state_t
								// but the rest of it is opaque

	qboolean	inuse;
	int			linkcount;

	// FIXME: move these fields to a server private sv_entity_t
	link_t		area;			// linked to a division node or leaf

	int			num_clusters;	// if -1, use headnode instead
	int			clusternums[MAX_ENT_CLUSTERS];
	int			headnode;		// unused if num_clusters != -1
	int			areanum, areanum2;

	//================================

	int			svflags;
	vec3_t		mins, maxs;
	vec3_t		absmin, absmax, size;
	solid_t		solid;
	int			clipmask;
	edict_t		*owner;


	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!

	//================================
	int			movetype;
	int			flags;

	char		*model;
	float		freetime;		// sv.time when the object was freed

	//
	// only used locally in game, not by server
	//
	char		*message;
	char		*classname;
	int			spawnflags;

	float		timestamp;

	float		angle;			// set in qe3, -1 = up, -2 = down

	char		*target;
	char		*targetname;
	char		*killtarget;
	char		*team;
	char		*pathtarget;
	char		*deathtarget;
	char		*combattarget;
	edict_t		*target_ent;

	float		speed, accel, decel;
	vec3_t		movedir;
	vec3_t		pos1, pos2;

	vec3_t		velocity;
	vec3_t		avelocity;
	int			mass;
	int			air_finished_framenum;
	float		gravity;		// per entity gravity multiplier (1.0 is normal)
								// use for lowgrav artifact, flares

	edict_t		*goalentity;
	edict_t		*movetarget;
	float		yaw_speed;
	float		ideal_yaw;

	int			nextthink;
	void		(*prethink)(edict_t *ent);
	void		(*think)(edict_t *self);
	void		(*blocked)(edict_t *self, edict_t *other);	//move to moveinfo?
	void		(*touch)(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);
	void		(*use)(edict_t *self, edict_t *other, edict_t *activator);
	void		(*pain)(edict_t *self, edict_t *other, float kick, int damage);
	void		(*die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

	int			touch_debounce_framenum;	// are all these legit?  do we need more/less of them?
	int			pain_debounce_framenum;
	int			damage_debounce_framenum;
	int			fly_sound_debounce_framenum;	//move to clientinfo
	float		last_move_time;

	int			health;
	int			max_health;
	int			gib_health;
	int			deadflag;
	qboolean	show_hostile;

	char		*map;			// target_changelevel

	int			viewheight;		// height above origin where eyesight is determined
	int			takedamage;
	int			dmg;
	int			radius_dmg;
	float		dmg_radius;
	int			sounds;			//make this a spawntemp var?
	int			count;

	edict_t		*chain;
	edict_t		*enemy;
	edict_t		*oldenemy;
	edict_t		*activator;
	edict_t		*groundentity;
	int			groundentity_linkcount;
	edict_t		*teamchain;
	edict_t		*teammaster;

	edict_t		*mynoise;		// can go in client only
	edict_t		*mynoise2;

	int			noise_index;
	int			noise_index2;
	float		volume;
	float		attenuation;

	// timing variables
	float		wait;
	float		delay;			// before firing targets
	float		random;

	int			watertype;
	int			waterlevel;

	vec3_t		move_origin;
	vec3_t		move_angles;

	// move this to clientinfo?
	int			light_level;

	int			style;			// also used as areaportal number

	gitem_t		*item;		// for bonus items

	// common data blocks
	moveinfo_t	moveinfo;

	// hack for proper s.old_origin updates
	vec3_t		old_origin;

#ifndef NO_FPS
	float		z_history[ MAX_FRAMEDIV ];
	//short		z_pmove;
	int			z_history_framenum;
	int			z_history_count;
#endif

	// visibility mask
	unsigned int dimension_visible;

	// action
	qboolean	splatted;
	int			classnum;
	int			typeNum;
};

typedef struct
{
	char netname[16];
	char ip[64];
	int enterframe;
	int disconnect_frame;
	int score;
	int kills;
	int deaths;
	int damage_dealt;
	int ctf_caps;
	int shotsTotal;
	int hitsTotal;
	int hitsLocations[LOC_MAX];
	gunStats_t gunstats[MAX_GUNSTAT];
	int team;
	gitem_t *weapon;
	gitem_t *item;
}
gghost_t;

#define MAX_GHOSTS 64 //MAX_CLIENTS
extern gghost_t ghost_players[MAX_GHOSTS];
extern int num_ghost_players;

void CreateGhost (edict_t * ent);

//zucc
void LaserSightThink (edict_t * self);
void SP_LaserSight (edict_t * self, gitem_t * item);
void Cmd_Reload_f (edict_t * ent);
void Cmd_New_Reload_f (edict_t * ent);
void Cmd_New_Weapon_f (edict_t * ent);
void Cmd_Weapon_f (edict_t * ent);
void Cmd_OpenDoor_f (edict_t * ent);
void Cmd_Bandage_f (edict_t * ent);
void Cmd_ID_f (edict_t * ent);
void Cmd_IR_f (edict_t * ent);
int GetWeaponNumFromArg(const char *s);
int GetItemNumFromArg(const char *s);
void Cmd_Choose_f (edict_t * ent);
void Cmd_TKOk (edict_t * ent);	// AQ:TNG - JBravo adding tkok
void Cmd_FF_f( edict_t *ent );
void Cmd_Time (edict_t * ent);	// AQ:TNG - JBravo adding time
void Cmd_Roundtimeleft_f(edict_t *ent); // AQ:TNG - DW added roundtimeleft
void DropSpecialWeapon (edict_t * ent);
void ReadySpecialWeapon (edict_t * ent);
void DropSpecialItem (edict_t * ent);
void Bandage (edict_t * ent);
void ShowGun (edict_t * ent);	// hentai's vwep function added by zucc
void FL_think (edict_t * self);	// TNG Flashlight
void FL_make (edict_t * self);	// TNG Flashlight

// spec functions
void SetupSpecSpawn (void);
void RespawnSpec (edict_t * ent);
void Drop_Spec (edict_t * ent, gitem_t * item);
void SpecThink (edict_t * spec);
void DeadDropSpec (edict_t * ent);

void temp_think_specweap (edict_t * ent);	// p_weapons.c
void ThinkSpecWeap (edict_t * ent);
void DropExtraSpecial (edict_t * ent);
void TransparentListSet (solid_t solid_type);

void killPlayer(edict_t *ent, qboolean suicidePunish);
int TP_GetTeamFromArg(const char *name);
int G_SortedClients( gclient_t **sortedList );
int G_NotSortedClients( gclient_t **sortedList );
void A_ScoreboardMessage( edict_t * ent, edict_t * killer );

//local to g_combat but needed in p_view
void SpawnDamage (int type, vec3_t origin, vec3_t normal, int damage);
void Killed (edict_t * targ, edict_t * inflictor, edict_t * attacker,
	     int damage, vec3_t point);

void Add_Frag(edict_t * ent, int mod);
void Subtract_Frag (edict_t * ent);
void Add_Death( edict_t *ent, qboolean end_streak );

void PrintDeathMessage(char *msg, edict_t * gibee);

void SetIDView (edict_t * ent);

qboolean Pickup_Special (edict_t * ent, edict_t * other);

// action function
edict_t *FindEdictByClassnum (char *classname, int classnum);

void EjectBlooder (edict_t * self, vec3_t start, vec3_t veloc);
void EjectShell (edict_t * self, vec3_t start, int toggle);
void AddDecal (edict_t * self, trace_t * tr);
void AddSplat (edict_t * self, vec3_t point, trace_t * tr);

//AQ2:TNG - Slicer New location support
#define MAX_LOCATIONS_IN_BASE		256	// Max amount of locations
// location structure
typedef struct
{
  int x;
  int y;
  int z;
  int rx;
  int ry;
  int rz;
  char desc[128];
}
placedata_t;

// Externals for accessing location structures
extern int ml_count;
extern placedata_t locationbase[];
extern char ml_creator[101];
//AQ2:TNG END

void Cmd_Ghost_f (edict_t * ent);
void Cmd_AutoRecord_f(edict_t * ent);

typedef struct team_s
{
	char name[20];
	char skin[MAX_SKINLEN];
	char skin_index[MAX_QPATH];
	int score, total;
	int ready, locked;
	int pauses_used, wantReset;
	cvar_t	*teamscore;
	edict_t	*captain;
}team_t;

extern team_t teams[TEAM_TOP];
#define PARSE_BUFSIZE 256
#define IS_ALIVE(ent) ((ent)->solid != SOLID_NOT && (ent)->deadflag != DEAD_DEAD)

#define GS_DEATHMATCH	1
#define GS_TEAMPLAY		2
#define GS_MATCHMODE	4
#define GS_ROUNDBASED	8
#define GS_WEAPONCHOOSE 16

extern int gameSettings;

#include "a_ctf.h"
#include "a_dom.h"

// Q2R

// typedef enum {
//     point,
//     cone
// } shadow_light_type_t;

// struct shadow_light_data_s
// {
//     shadow_light_type_t lighttype;
// 	float		radius;
// 	int			resolution;
// 	float		intensity = 1;
// 	float		fade_start;
// 	float		fade_end;
// 	int			lightstyle = -1;
// 	float		coneangle = 45;
//     vec3_t      conedirection;
// } shadow_light_data_t;

// typedef struct shadow_light_temp_s
// {
// 	shadow_light_data_t data;
// 	const char	*lightstyletarget = NULL;
// } shadow_light_temp_t;

// Q2R