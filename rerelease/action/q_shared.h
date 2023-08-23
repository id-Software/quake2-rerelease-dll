//-----------------------------------------------------------------------------
// q_shared.h -- included first by ALL program modules
//
// $Id: q_shared.h,v 1.8 2001/09/28 13:48:35 ra Exp $
//
//-----------------------------------------------------------------------------
// $Log: q_shared.h,v $
// Revision 1.8  2001/09/28 13:48:35  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.7  2001/08/19 01:22:25  deathwatch
// cleaned the formatting of some files
//
// Revision 1.6  2001/08/18 20:51:55  deathwatch
// Messing with the colours of the flashlight and the person using the
// flashlight
//
// Revision 1.5  2001/08/18 17:14:04  deathwatch
// Flashlight Added (not done yet, needs to prevent DEAD ppl from using it,
// the glow should be white and a bit smaller if possible and the daiper needs
// to be gone. Also, it should only work in 'darkmatch' I guess and it should
// make a sound when you turn it on/off.
//
// Revision 1.4  2001/05/31 16:58:14  igor_rock
// conflicts resolved
//
// Revision 1.3.2.2  2001/05/25 18:59:53  igor_rock
// Added CTF Mode completly :)
// Support for .flg files is still missing, but with "real" CTF maps like
// tq2gtd1 the ctf works fine.
// (I hope that all other modes still work, just tested DM and teamplay)
//
// Revision 1.3.2.1  2001/05/20 15:17:32  igor_rock
// removed the old ctf code completly
//
// Revision 1.3  2001/05/11 16:07:26  mort
// Various CTF bits and pieces...
//
// Revision 1.2  2001/05/07 08:32:17  mort
// Basic CTF code
// No spawns etc
// Just the cvars and flag entity
//
// Revision 1.1.1.1  2001/05/06 17:24:07  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#ifndef __Q_SHARED_H
#define __Q_SHARED_H

#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
//FIREBLADE
#include <stddef.h>
#include <stdint.h>
//FIREBLADE

// legacy ABI support for Windows
#if defined(__GNUC__) && defined(WIN32) && ! defined(WIN64)
#define		q_gameabi           __attribute__((callee_pop_aggregate_return(0)))
#else
#define		q_gameabi
#endif

//==============================================
#ifdef _WIN32
#ifdef _MSC_VER
// unknown pragmas are SUPPOSED to be ignored, but....
#pragma warning(disable : 4244)	// MIPS
#pragma warning(disable : 4136)	// X86
#pragma warning(disable : 4051)	// ALPHA
#pragma warning(disable : 4018)	// signed/unsigned mismatch
#pragma warning(disable : 4305)	// truncation from const double to float
#pragma warning(disable : 4996)	// deprecated functions
#pragma warning(disable : 4100)	// unreferenced formal parameter
#endif

# define HAVE___INLINE
# define HAVE__SNPRINTF
# define HAVE__VSNPRINTF
# define HAVE__STRICMP
# define HAVE___FASTCALL
# define HAVE__CDECL

#endif
//==============================================
#if defined(__linux__) || defined(__FreeBSD__) || defined(__GNUC__)

# define HAVE_INLINE
# define HAVE_STRCASECMP
# define HAVE_SNPRINTF
# define HAVE_VSNPRINTF

#endif
//==============================================

#if ! defined(HAVE__CDECL) && ! defined(__cdecl)
# define __cdecl
#endif

#if ! defined(HAVE___FASTCALL) && ! defined(__fastcall)
# define __fastcall
#endif

#if ! defined(HAVE_INLINE) && ! defined(inline)
# ifdef HAVE___INLINE
#  define inline __inline
# else
#  define inline
# endif
#endif

#if defined(HAVE__SNPRINTF) && ! defined(snprintf)
# define snprintf _snprintf
#endif

#if defined(HAVE__VSNPRINTF) && ! defined(vsnprintf)
# define vsnprintf(dest, size, src, list) _vsnprintf((dest), (size), (src), (list)), (dest)[(size)-1] = 0
#endif

#ifdef HAVE__STRICMP
# ifndef Q_stricmp
#  define Q_stricmp _stricmp
# endif
# ifndef Q_strnicmp
#  define Q_strnicmp _strnicmp
# endif
# ifndef strcasecmp
#  define strcasecmp _stricmp
# endif
# ifndef strncasecmp
#  define strncasecmp _strnicmp
# endif
#elif defined(HAVE_STRCASECMP)
# ifndef Q_stricmp
#  define Q_stricmp strcasecmp
# endif
# ifndef Q_strnicmp
#  define Q_strnicmp strncasecmp
# endif
#endif

// =========================================================================

// New define for this came from 3.20  -FB
#if (defined(_M_IX86) || defined(__i386__) || defined(__ia64__)) && !defined(C_ONLY)
# define id386 1
#else
# define id386 0
#endif

#if __GNUC__ >= 4
#define q_offsetof(t, m)    __builtin_offsetof(t, m)
#else
#define q_offsetof(t, m)    ((size_t)&((t *)0)->m)
#endif

//==============================================

typedef unsigned char byte;
typedef enum { qfalse = 0, qtrue } qboolean;
#define true qtrue
#define false qfalse

#ifndef NULL
#define NULL ((void *)0)
#endif


// angle indexes
#define PITCH                           0	// up / down
#define YAW                             1	// left / right
#define ROLL                            2	// fall over

//// Q2R

#if defined(KEX_Q2GAME_EXPORTS)
    #define Q2GAME_API extern "C" __declspec( dllexport )
#elif defined(KEX_Q2GAME_IMPORTS)
    #define Q2GAME_API extern "C" __declspec( dllimport )
#else
    #define Q2GAME_API
#endif

// game.h -- game dll information visible to server
// PARIL_NEW_API - value likely not used by any other Q2-esque engine in the wild
#define GAME_API_VERSION 2022
#define CGAME_API_VERSION 2022

// forward declarations
struct edict_t;
struct gclient_t;

#define MAX_STRING_CHARS 1024 // max length of a string passed to Cmd_TokenizeString
#define MAX_STRING_TOKENS 80  // max tokens resulting from Cmd_TokenizeString
#define MAX_TOKEN_CHARS 512   // max length of an individual token

#define MAX_QPATH 64   // max length of a quake game pathname
#define MAX_OSPATH 128 // max length of a filesystem pathname
//
// per-level limits
//
#define MAX_CLIENTS 256 // absolute limit
#define MAX_EDICTS 8192 // upper limit, due to svc_sound encoding as 15 bits
#define MAX_LIGHTSTYLES 256
#define MAX_MODELS 8192 // these are sent over the net as shorts
#define MAX_SOUNDS 2048 // so they cannot be blindly increased
#define MAX_IMAGES 512
#define MAX_ITEMS 256
#define MAX_GENERAL (MAX_CLIENTS * 2) // general config strings

//// Q2R

// game print flags
#define PRINT_LOW                       0	// pickup messages
#define PRINT_MEDIUM                    1	// death messages
#define PRINT_HIGH                      2	// critical messages
#define PRINT_CHAT                      3	// chat messages



#define ERR_FATAL                       0	// exit the entire game with a popup window
#define ERR_DROP                        1	// print to console and disconnect from game
#define ERR_DISCONNECT                  2	// don't kill server

#define PRINT_ALL                       0
#define PRINT_DEVELOPER                 1	// only print when "developer 1"
#define PRINT_ALERT                     2


// destination class for gi.multicast()
typedef enum
{
  MULTICAST_ALL,
  MULTICAST_PHS,
  MULTICAST_PVS,
  MULTICAST_ALL_R,
  MULTICAST_PHS_R,
  MULTICAST_PVS_R
}
multicast_t;


/*
   ==============================================================

   MATHLIB

   ==============================================================
 */

typedef float vec_t;
typedef vec_t vec3_t[3];
typedef vec_t vec5_t[5];

typedef int fixed4_t;
typedef int fixed8_t;
typedef int fixed16_t;

#ifndef M_PI
#define M_PI            3.14159265358979323846f	// matches value in gcc v2 math.h
#endif

#ifndef M_TWOPI
# define M_TWOPI		6.28318530717958647692f
#endif

#define M_PI_DIV_180	0.0174532925199432957692f
#define M_180_DIV_PI	57.295779513082320876798f

#define DEG2RAD( a ) (a * M_PI_DIV_180)
#define RAD2DEG( a ) (a * M_180_DIV_PI)


#ifndef max
# define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
# define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#define bound(a,b,c) ((a) >= (c) ? (a) : (b) < (a) ? (a) : (b) > (c) ? (c) : (b))

#define clamp(a,b,c) ((b) >= (c) ? (a)=(b) : (a) < (b) ? (a)=(b) : (a) > (c) ? (a)=(c) : (a))

struct cplane_s;

extern vec3_t vec3_origin;

#define nanmask (255<<23)

#define IS_NAN(x) (((*(int *)&x)&nanmask)==nanmask)

// microsoft's fabs seems to be ungodly slow...
//float Q_fabs (float f);
//#define       fabs(f) Q_fabs(f)
// next define line was changed by 3.20  -FB
#if !defined C_ONLY && !defined __linux__ && !defined __sgi
extern long Q_ftol (float f);
#else
#define Q_ftol( f ) ( long ) (f)
#endif

#define DotProduct(x,y)			((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#define CrossProduct(v1,v2,c)	((c)[0]=(v1)[1]*(v2)[2]-(v1)[2]*(v2)[1],(c)[1]=(v1)[2]*(v2)[0]-(v1)[0]*(v2)[2],(c)[2]=(v1)[0]*(v2)[1]-(v1)[1]*(v2)[0])

#define PlaneDiff(point,plane) (((plane)->type < 3 ? (point)[(plane)->type] : DotProduct((point), (plane)->normal)) - (plane)->dist)

#define VectorSubtract(a,b,c)   ((c)[0]=(a)[0]-(b)[0],(c)[1]=(a)[1]-(b)[1],(c)[2]=(a)[2]-(b)[2])
#define VectorAdd(a,b,c)		((c)[0]=(a)[0]+(b)[0],(c)[1]=(a)[1]+(b)[1],(c)[2]=(a)[2]+(b)[2])
#define VectorCopy(a,b)			((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
#define VectorClear(a)			((a)[0]=(a)[1]=(a)[2]=0)
#define VectorNegate(a,b)		((b)[0]=-(a)[0],(b)[1]=-(a)[1],(b)[2]=-(a)[2])
#define VectorSet(v, x, y, z)	((v)[0]=(x),(v)[1]=(y),(v)[2]=(z))
#define VectorAvg(a,b,c)		((c)[0]=((a)[0]+(b)[0])*0.5f,(c)[1]=((a)[1]+(b)[1])*0.5f, (c)[2]=((a)[2]+(b)[2])*0.5f)
#define VectorMA(a,b,c,d)		((d)[0]=(a)[0]+(b)*(c)[0],(d)[1]=(a)[1]+(b)*(c)[1],(d)[2]=(a)[2]+(b)*(c)[2])
#define VectorCompare(v1,v2)	((v1)[0]==(v2)[0] && (v1)[1]==(v2)[1] && (v1)[2]==(v2)[2])
#define VectorLength(v)			(sqrtf(DotProduct((v),(v))))
#define VectorInverse(v)		((v)[0]=-(v)[0],(v)[1]=-(v)[1],(v)[2]=-(v)[2])
#define VectorScale(in,s,out)	((out)[0]=(in)[0]*(s),(out)[1]=(in)[1]*(s),(out)[2]=(in)[2]*(s))
#define LerpVector(a,b,c,d) \
    ((d)[0]=(a)[0]+(c)*((b)[0]-(a)[0]), \
     (d)[1]=(a)[1]+(c)*((b)[1]-(a)[1]), \
     (d)[2]=(a)[2]+(c)*((b)[2]-(a)[2]))

#define VectorCopyMins(a,b,c)	((c)[0]=min((a)[0],(b)[0]),(c)[0]=min((a)[1],(b)[1]),(c)[2]=min((a)[2],(b)[2]))
#define VectorCopyMaxs(a,b,c)	((c)[0]=max((a)[0],(b)[0]),(c)[0]=max((a)[1],(b)[1]),(c)[2]=max((a)[2],(b)[2]))

#define DistanceSquared(v1,v2)	(((v1)[0]-(v2)[0])*((v1)[0]-(v2)[0])+((v1)[1]-(v2)[1])*((v1)[1]-(v2)[1])+((v1)[2]-(v2)[2])*((v1)[2]-(v2)[2]))
#define Distance(v1,v2)			(sqrtf(DistanceSquared(v1,v2)))

#define ClearBounds(mins,maxs)	((mins)[0]=(mins)[1]=(mins)[2]=99999,(maxs)[0]=(maxs)[1]=(maxs)[2]=-99999)


void AddPointToBounds (const vec3_t v, vec3_t mins, vec3_t maxs);
vec_t VectorNormalize (vec3_t v);	// returns vector length
vec_t VectorNormalize2 (const vec3_t v, vec3_t out);

int Q_log2 (int val);

void R_ConcatRotations (float in1[3][3], float in2[3][3], float out[3][3]);
void R_ConcatTransforms (float in1[3][4], float in2[3][4], float out[3][4]);

void AngleVectors (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
int BoxOnPlaneSide (const vec3_t emins, const vec3_t emaxs, const struct cplane_s *plane);
float anglemod (float a);
float LerpAngle (float a1, float a2, float frac);

#define BOX_ON_PLANE_SIDE(emins, emaxs, p)        \
  (((p)->type < 3)?                               \
    (                                             \
      ((p)->dist <= (emins)[(p)->type])?          \
        1                                         \
        :                                         \
          (                                       \
            ((p)->dist >= (emaxs)[(p)->type])?    \
              2                                   \
              :                                   \
              3                                   \
            )                                     \
          )                                       \
        :                                         \
          BoxOnPlaneSide( (emins), (emaxs), (p)))

void ProjectPointOnPlane (vec3_t dst, const vec3_t p, const vec3_t normal);
void PerpendicularVector (vec3_t dst, const vec3_t src);
void RotatePointAroundVector (vec3_t dst, const vec3_t dir,
			      const vec3_t point, float degrees);

void VectorRotate( vec3_t in, vec3_t angles, vec3_t out );  // a_doorkick.c
void VectorRotate2( vec3_t v, float degrees );

//=============================================

char *COM_SkipPath (char *pathname);
void COM_StripExtension (char *in, char *out);
void COM_FileBase (char *in, char *out);
void COM_FilePath (char *in, char *out);
void COM_DefaultExtension (char *path, char *extension);

char *COM_Parse (char **data_p);
// data is an in/out parm, returns a parsed out token

void Com_PageInMemory (byte * buffer, int size);

//=============================================
#define Q_isupper( c )	( (c) >= 'A' && (c) <= 'Z' )
#define Q_islower( c )	( (c) >= 'a' && (c) <= 'z' )
#define Q_isdigit( c )	( (c) >= '0' && (c) <= '9' )
#define Q_isalpha( c )	( Q_isupper( c ) || Q_islower( c ) )
#define Q_isalnum( c )	( Q_isalpha( c ) || Q_isdigit( c ) )

int Q_tolower( int c );
int Q_toupper( int c );

char *Q_strlwr( char *s );
char *Q_strupr( char *s );

#ifndef Q_strnicmp
 int Q_strnicmp (const char *s1, const char *s2, size_t size);
#endif
#ifndef Q_stricmp
#  define Q_stricmp(s1, s2) Q_strnicmp((s1), (s2), 99999)
#endif
 char *Q_stristr( const char *str1, const char *str2 );

// buffer safe operations
void Q_strncpyz (char *dest, const char *src, size_t size );
void Q_strncatz (char *dest, const char *src, size_t size );
#ifdef HAVE_SNPRINTF
# define Com_sprintf snprintf
#else
 void Com_sprintf(char *dest, size_t size, const char *fmt, ...);
#endif

//=============================================

void Swap_Init (void);
char *va(const char *format, ...);

//=============================================

//
// key / value info strings
//
#define MAX_INFO_KEY            64
#define MAX_INFO_VALUE          64
#define MAX_INFO_STRING         512

char *Info_ValueForKey (const char *s, const char *key);
void Info_RemoveKey (char *s, const char *key);
void Info_SetValueForKey (char *s, const char *key, const char *value);
qboolean Info_Validate (const char *s);

/*
   ==============================================================

   SYSTEM SPECIFIC

   ==============================================================
 */

// directory searching
#define SFF_ARCH    0x01
#define SFF_HIDDEN  0x02
#define SFF_RDONLY  0x04
#define SFF_SUBDIR  0x08
#define SFF_SYSTEM  0x10

// this is only here so the functions in q_shared.c and q_shwin.c can link
void Sys_Error (const char *error, ...);
void Com_Printf (const char *msg, ...);

/*
   ==========================================================

   CVARS (console variables)

   ==========================================================
 */

#ifndef CVAR
#define CVAR

#define CVAR_ARCHIVE            1	// set to cause it to be saved to vars.rc
#define CVAR_USERINFO           2	// added to userinfo  when changed
#define CVAR_SERVERINFO         4	// added to serverinfo when changed
#define CVAR_NOSET              8	// don't allow change from console at all, but can be set from the command line
#define CVAR_LATCH              16	// save changes until server restart

// nothing outside the Cvar_*() functions should modify these fields!
typedef struct cvar_s
{
  char *name;
  char *string;
  char *latched_string;		// for CVAR_LATCH vars

  int flags;
  qboolean modified;		// set each time the cvar is changed

  float value;
  struct cvar_s *next;
}
cvar_t;

#endif // CVAR

/*
   ==============================================================

   COLLISION DETECTION

   ==============================================================
 */

// lower bits are stronger, and will eat weaker brushes completely
#define CONTENTS_SOLID                  1	// an eye is never valid in a solid
#define CONTENTS_WINDOW                 2	// translucent, but not watery
#define CONTENTS_AUX                    4
#define CONTENTS_LAVA                   8
#define CONTENTS_SLIME                  16
#define CONTENTS_WATER                  32
#define CONTENTS_MIST                   64
#define LAST_VISIBLE_CONTENTS           64

// remaining contents are non-visible, and don't eat brushes

#define CONTENTS_AREAPORTAL             0x8000

#define CONTENTS_PLAYERCLIP             0x10000
#define CONTENTS_MONSTERCLIP            0x20000

// currents can be added to any other contents, and may be mixed
#define CONTENTS_CURRENT_0              0x40000
#define CONTENTS_CURRENT_90             0x80000
#define CONTENTS_CURRENT_180            0x100000
#define CONTENTS_CURRENT_270            0x200000
#define CONTENTS_CURRENT_UP             0x400000
#define CONTENTS_CURRENT_DOWN           0x800000

#define CONTENTS_ORIGIN                 0x1000000	// removed before bsping an entity

#define CONTENTS_MONSTER                0x2000000	// should never be on a brush, only in game
#define CONTENTS_DEADMONSTER            0x4000000
#define CONTENTS_DETAIL                 0x8000000	// brushes to be added after vis leafs
#define CONTENTS_TRANSLUCENT            0x10000000	// auto set if any surface has trans
#define CONTENTS_LADDER                 0x20000000



#define SURF_LIGHT              0x1	// value will hold the light strength

#define SURF_SLICK              0x2	// effects game physics

#define SURF_SKY                0x4	// don't draw, but add to skybox
#define SURF_WARP               0x8	// turbulent water warp
#define SURF_TRANS33            0x10
#define SURF_TRANS66            0x20
#define SURF_FLOWING            0x40	// scroll towards angle
#define SURF_NODRAW             0x80	// don't bother referencing the texture



// content masks
#define MASK_ALL                        (-1)
#define MASK_SOLID                      (CONTENTS_SOLID|CONTENTS_WINDOW)
#define MASK_PLAYERSOLID                (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define MASK_DEADSOLID                  (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW)
#define MASK_MONSTERSOLID               (CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define MASK_WATER                      (CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define MASK_OPAQUE                     (CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
//#define       MASK_SHOT               (CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEADMONSTER)
#define MASK_SHOT                       (CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_WINDOW)
#define MASK_CURRENT                    (CONTENTS_CURRENT_0|CONTENTS_CURRENT_90|CONTENTS_CURRENT_180|CONTENTS_CURRENT_270|CONTENTS_CURRENT_UP|CONTENTS_CURRENT_DOWN)


// gi.BoxEdicts() can return a list of either solid or trigger entities
// FIXME: eliminate AREA_ distinction?
#define AREA_SOLID              1
#define AREA_TRIGGERS           2


// plane_t structure
// !!! if this is changed, it must be changed in asm code too !!!
typedef struct cplane_s
{
  vec3_t normal;
  float dist;
  byte type;			// for fast side tests

  byte signbits;		// signx + (signy<<1) + (signz<<1)

  byte pad[2];
}
cplane_t;

// structure offset for asm code
#define CPLANE_NORMAL_X                 0
#define CPLANE_NORMAL_Y                 4
#define CPLANE_NORMAL_Z                 8
#define CPLANE_DIST                     12
#define CPLANE_TYPE                     16
#define CPLANE_SIGNBITS                 17
#define CPLANE_PAD0                     18
#define CPLANE_PAD1                     19

typedef struct cmodel_s
{
  vec3_t mins, maxs;
  vec3_t origin;		// for sounds or lights

  int headnode;
}
cmodel_t;

typedef struct csurface_s
{
  char name[16];
  int flags;
  int value;
}
csurface_t;

// FROM 3.20 -FB
typedef struct mapsurface_s	// used internally due to name len probs //ZOID
{
  csurface_t c;
  char rname[32];
}
mapsurface_t;
// ^^^

// a trace is returned when a box is swept through the world
typedef struct
{
  qboolean allsolid;		// if true, plane is not valid

  qboolean startsolid;		// if true, the initial point was in a solid area

  float fraction;		// time completed, 1.0 = didn't hit anything

  vec3_t endpos;		// final position

  cplane_t plane;		// surface normal at impact

  csurface_t *surface;		// surface hit

  int contents;			// contents on other side of surface hit

  struct edict_s *ent;		// not set by CM_*() functions

}
trace_t;



// pmove_state_t is the information necessary for client side movement
// prediction
typedef enum
{
  // can accelerate and turn
  PM_NORMAL,
  PM_SPECTATOR,
  // no acceleration or turning
  PM_DEAD,
  PM_GIB,			// different bounding box
  PM_FREEZE
}
pmtype_t;

// pmove->pm_flags
#define PMF_DUCKED              1
#define PMF_JUMP_HELD           2
#define PMF_ON_GROUND           4
#define PMF_TIME_WATERJUMP      8	// pm_time is waterjump
#define PMF_TIME_LAND           16	// pm_time is time before rejump
#define PMF_TIME_TELEPORT       32	// pm_time is non-moving time
#define PMF_NO_PREDICTION       64	// temporarily disables prediction (used for grappling hook)

// this structure needs to be communicated bit-accurate
// from the server to the client to guarantee that
// prediction stays in sync, so no floats are used.
// if any part of the game code modifies this struct, it
// will result in a prediction error of some degree.
typedef struct
{
  pmtype_t pm_type;

  short origin[3];		// 12.3

  short velocity[3];		// 12.3

  byte pm_flags;		// ducked, jump_held, etc

  byte pm_time;			// each unit = 8 ms

  short gravity;
  short delta_angles[3];	// add to command angles to get view direction
  // changed by spawns, rotating objects, and teleporters
}
pmove_state_t;


//
// button bits
//
#define BUTTON_ATTACK           1
#define BUTTON_USE              2
#define BUTTON_ANY              128	// any key whatsoever


// usercmd_t is sent to the server each client frame
typedef struct usercmd_s
{
  byte msec;
  byte buttons;
  short angles[3];
  short forwardmove, sidemove, upmove;
  byte impulse;			// remove?

  byte lightlevel;		// light level the player is standing on

}
usercmd_t;


#define MAXTOUCH        32
typedef struct
{
  // state (in / out)
  pmove_state_t s;

  // command (in)
  usercmd_t cmd;
  qboolean snapinitial;		// if s has been changed outside pmove

  // results (out)
  int numtouch;
  struct edict_s *touchents[MAXTOUCH];

  vec3_t viewangles;		// clamped

  float viewheight;

  vec3_t mins, maxs;		// bounding box size

  struct edict_s *groundentity;
  int watertype;
  int waterlevel;

  // callbacks to test the world
    trace_t (*trace) (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);
  int (*pointcontents) (vec3_t point);
}
pmove_t;


// entity_state_t->effects
// Effects are things handled on the client side (lights, particles, frame animations)
// that happen constantly on the given entity.
// An entity that has effects will be sent to the client
// even if it has a zero index model.
#define EF_ROTATE                       0x00000001	// rotate (bonus items)
#define EF_GIB                          0x00000002	// leave a trail
#define EF_BLASTER                      0x00000008	// redlight + trail
#define EF_ROCKET                       0x00000010	// redlight + trail
#define EF_GRENADE                      0x00000020
#define EF_HYPERBLASTER                 0x00000040
#define EF_BFG                          0x00000080
#define EF_COLOR_SHELL                  0x00000100
#define EF_POWERSCREEN                  0x00000200
#define EF_ANIM01                       0x00000400	// automatically cycle between frames 0 and 1 at 2 hz
#define EF_ANIM23                       0x00000800	// automatically cycle between frames 2 and 3 at 2 hz
#define EF_ANIM_ALL                     0x00001000	// automatically cycle through all frames at 2hz
#define EF_ANIM_ALLFAST                 0x00002000	// automatically cycle through all frames at 10hz
#define EF_FLIES                        0x00004000
#define EF_QUAD                         0x00008000
#define EF_PENT                         0x00010000
#define EF_TELEPORTER                   0x00020000	// particle fountain
#define EF_FLAG1                        0x00040000
#define EF_FLAG2                        0x00080000

// FOLLOWING FROM 3.20 CODE -FB
// RAFAEL
#define EF_IONRIPPER                    0x00100000
#define EF_GREENGIB                     0x00200000
#define EF_BLUEHYPERBLASTER             0x00400000
#define EF_SPINNINGLIGHTS               0x00800000
#define EF_PLASMA                       0x01000000
#define EF_TRAP                         0x02000000

//ROGUE
#define EF_TRACKER                      0x04000000
#define EF_DOUBLE                       0x08000000
#define EF_SPHERETRANS                  0x10000000
#define EF_TAGTRAIL                     0x20000000
#define EF_HALF_DAMAGE                  0x40000000
#define EF_TRACKERTRAIL                 0x80000000
//ROGUE

// zucc some I got from quake devels
#define EF_BLUE          0x00400000	//a blue light
#define EF_ROTATEREDSPOT 0x00800000	//fast rotate with a red spot of light at the front
#define EF_TRANSLIGHT    0x01000000	//transparant with some lighting
#define EF_PFOUNT        0x02000000	// particle foundtain
#define EF_DYNDARK       0x04000000	//DYNAMIC darkness the one i was looking for!
#define EF_YELLOWSHELL   0x08000000	//a yellow shell similar to those found with EF_COLOR_SHELL IIRC
#define EF_TRANS         0x10000000	//translucency
#define EF_YELLOWDOT     0x20000000	//yellow lighting with yellow dots under the model
#define EF_WHITESHELL    0x40000000	//a yellow shell around  the model (like EF_YELLOWSHELL)
#define EF_FLIES2        0x80000000	//Flies go a buzzin' and the sky grows dim
#define EF_EDARK         0x84000000	// Extreme Darkness, you won't believe!
#define EF_BLUE_CRUST    0x08208000	// An odd blue "crust" around the model
#define EF_QBF           0x90408000	// The best of three worlds, blue shell like a quad, dark, and covered with flies
#define EF_REDC          0x30050001	// This one is nice, a red light eminating of a red Quad crust
#define EF_GREENRC       0x35152841	// It's Christmas time!  Red shell and geen light!
#define EF_REDG          0x22010107	// RedCrust with gib effect trail
#define EF_PUP           0x86080100	// 'ere's an odd one, a straight, upwards line of particles sprays above the modle
#define EF_FLYG          0x60507800	// A few flies with a light, greenish yello crust
#define EF_YELLOW_CRUST  0x10300070	// Yellow crust with a of smoke & yellow particles
#define EF_BACKRED       0x90900900	// The Usual is black fly maham, but with a red light peeking through.

#define EF_GREEN_LIGHT   0x04000040

// entity_state_t->renderfx flags
#define RF_MINLIGHT             1	// allways have some light (viewmodel)
#define RF_VIEWERMODEL          2	// don't draw through eyes, only mirrors
#define RF_WEAPONMODEL          4	// only draw through eyes
#define RF_FULLBRIGHT           8	// allways draw full intensity
#define RF_DEPTHHACK            16	// for view weapon Z crunching
#define RF_TRANSLUCENT          32
#define RF_FRAMELERP            64
#define RF_BEAM                 128
#define RF_CUSTOMSKIN           256	// skin is an index in image_precache
#define RF_GLOW                 512	// pulse lighting for bonus items
#define RF_SHELL_RED            1024
#define RF_SHELL_GREEN          2048
#define RF_SHELL_BLUE           4096

//ROGUE (FROM 3.20)
#define RF_IR_VISIBLE           0x00008000	// 32768
#define RF_SHELL_DOUBLE         0x00010000	// 65536
#define RF_SHELL_HALF_DAM       0x00020000
#define RF_USE_DISGUISE         0x00040000
//ROGUE

// player_state_t->refdef flags
#define RDF_UNDERWATER          1	// warp the screen as apropriate
#define RDF_NOWORLDMODEL        2	// used for player configuration screen

//ROGUE
#define RDF_IRGOGGLES           4
#define RDF_UVGOGGLES           8
//ROGUE

#define RF_INDICATOR			(RF_TRANSLUCENT | RF_FULLBRIGHT | RF_DEPTHHACK)
#define IS_INDICATOR(rflags)	((rflags & RF_INDICATOR) == RF_INDICATOR)


// player_state_t->refdef flags
#define RDF_UNDERWATER          1	// warp the screen as apropriate
#define RDF_NOWORLDMODEL        2	// used for player configuration screen

//
// muzzle flashes / player effects
//
#define MZ_BLASTER                      0
#define MZ_MACHINEGUN                   1
#define MZ_SHOTGUN                      2
#define MZ_CHAINGUN1                    3
#define MZ_CHAINGUN2                    4
#define MZ_CHAINGUN3                    5
#define MZ_RAILGUN                      6
#define MZ_ROCKET                       7
#define MZ_GRENADE                      8
#define MZ_LOGIN                        9
#define MZ_LOGOUT                       10
#define MZ_RESPAWN                      11
#define MZ_BFG                          12
#define MZ_SSHOTGUN                     13
#define MZ_HYPERBLASTER                 14
#define MZ_ITEMRESPAWN                  15
// FROM 3.20 -FB
// RAFAEL
#define MZ_IONRIPPER                    16
#define MZ_BLUEHYPERBLASTER             17
#define MZ_PHALANX                      18
// ^^^
#define MZ_SILENCED                     128	// bit flag ORed with one of the above numbers

//ROGUE (3.20)
#define MZ_ETF_RIFLE                    30
#define MZ_UNUSED                       31
#define MZ_SHOTGUN2                     32
#define MZ_HEATBEAM                     33
#define MZ_BLASTER2                     34
#define MZ_TRACKER                      35
#define MZ_NUKE1                        36
#define MZ_NUKE2                        37
#define MZ_NUKE4                        38
#define MZ_NUKE8                        39
//ROGUE

//
// monster muzzle flashes
//
#define MZ2_TANK_BLASTER_1                      1
#define MZ2_TANK_BLASTER_2                      2
#define MZ2_TANK_BLASTER_3                      3
#define MZ2_TANK_MACHINEGUN_1                   4
#define MZ2_TANK_MACHINEGUN_2                   5
#define MZ2_TANK_MACHINEGUN_3                   6
#define MZ2_TANK_MACHINEGUN_4                   7
#define MZ2_TANK_MACHINEGUN_5                   8
#define MZ2_TANK_MACHINEGUN_6                   9
#define MZ2_TANK_MACHINEGUN_7                   10
#define MZ2_TANK_MACHINEGUN_8                   11
#define MZ2_TANK_MACHINEGUN_9                   12
#define MZ2_TANK_MACHINEGUN_10                  13
#define MZ2_TANK_MACHINEGUN_11                  14
#define MZ2_TANK_MACHINEGUN_12                  15
#define MZ2_TANK_MACHINEGUN_13                  16
#define MZ2_TANK_MACHINEGUN_14                  17
#define MZ2_TANK_MACHINEGUN_15                  18
#define MZ2_TANK_MACHINEGUN_16                  19
#define MZ2_TANK_MACHINEGUN_17                  20
#define MZ2_TANK_MACHINEGUN_18                  21
#define MZ2_TANK_MACHINEGUN_19                  22
#define MZ2_TANK_ROCKET_1                       23
#define MZ2_TANK_ROCKET_2                       24
#define MZ2_TANK_ROCKET_3                       25

#define MZ2_INFANTRY_MACHINEGUN_1               26
#define MZ2_INFANTRY_MACHINEGUN_2               27
#define MZ2_INFANTRY_MACHINEGUN_3               28
#define MZ2_INFANTRY_MACHINEGUN_4               29
#define MZ2_INFANTRY_MACHINEGUN_5               30
#define MZ2_INFANTRY_MACHINEGUN_6               31
#define MZ2_INFANTRY_MACHINEGUN_7               32
#define MZ2_INFANTRY_MACHINEGUN_8               33
#define MZ2_INFANTRY_MACHINEGUN_9               34
#define MZ2_INFANTRY_MACHINEGUN_10              35
#define MZ2_INFANTRY_MACHINEGUN_11              36
#define MZ2_INFANTRY_MACHINEGUN_12              37
#define MZ2_INFANTRY_MACHINEGUN_13              38

#define MZ2_SOLDIER_BLASTER_1                   39
#define MZ2_SOLDIER_BLASTER_2                   40
#define MZ2_SOLDIER_SHOTGUN_1                   41
#define MZ2_SOLDIER_SHOTGUN_2                   42
#define MZ2_SOLDIER_MACHINEGUN_1                43
#define MZ2_SOLDIER_MACHINEGUN_2                44

#define MZ2_GUNNER_MACHINEGUN_1                 45
#define MZ2_GUNNER_MACHINEGUN_2                 46
#define MZ2_GUNNER_MACHINEGUN_3                 47
#define MZ2_GUNNER_MACHINEGUN_4                 48
#define MZ2_GUNNER_MACHINEGUN_5                 49
#define MZ2_GUNNER_MACHINEGUN_6                 50
#define MZ2_GUNNER_MACHINEGUN_7                 51
#define MZ2_GUNNER_MACHINEGUN_8                 52
#define MZ2_GUNNER_GRENADE_1                    53
#define MZ2_GUNNER_GRENADE_2                    54
#define MZ2_GUNNER_GRENADE_3                    55
#define MZ2_GUNNER_GRENADE_4                    56

#define MZ2_CHICK_ROCKET_1                      57

#define MZ2_FLYER_BLASTER_1                     58
#define MZ2_FLYER_BLASTER_2                     59

#define MZ2_MEDIC_BLASTER_1                     60

#define MZ2_GLADIATOR_RAILGUN_1                 61

#define MZ2_HOVER_BLASTER_1                     62

#define MZ2_ACTOR_MACHINEGUN_1                  63

#define MZ2_SUPERTANK_MACHINEGUN_1              64
#define MZ2_SUPERTANK_MACHINEGUN_2              65
#define MZ2_SUPERTANK_MACHINEGUN_3              66
#define MZ2_SUPERTANK_MACHINEGUN_4              67
#define MZ2_SUPERTANK_MACHINEGUN_5              68
#define MZ2_SUPERTANK_MACHINEGUN_6              69
#define MZ2_SUPERTANK_ROCKET_1                  70
#define MZ2_SUPERTANK_ROCKET_2                  71
#define MZ2_SUPERTANK_ROCKET_3                  72

#define MZ2_BOSS2_MACHINEGUN_L1                 73
#define MZ2_BOSS2_MACHINEGUN_L2                 74
#define MZ2_BOSS2_MACHINEGUN_L3                 75
#define MZ2_BOSS2_MACHINEGUN_L4                 76
#define MZ2_BOSS2_MACHINEGUN_L5                 77
#define MZ2_BOSS2_ROCKET_1                      78
#define MZ2_BOSS2_ROCKET_2                      79
#define MZ2_BOSS2_ROCKET_3                      80
#define MZ2_BOSS2_ROCKET_4                      81

#define MZ2_FLOAT_BLASTER_1                     82

#define MZ2_SOLDIER_BLASTER_3                   83
#define MZ2_SOLDIER_SHOTGUN_3                   84
#define MZ2_SOLDIER_MACHINEGUN_3                85
#define MZ2_SOLDIER_BLASTER_4                   86
#define MZ2_SOLDIER_SHOTGUN_4                   87
#define MZ2_SOLDIER_MACHINEGUN_4                88
#define MZ2_SOLDIER_BLASTER_5                   89
#define MZ2_SOLDIER_SHOTGUN_5                   90
#define MZ2_SOLDIER_MACHINEGUN_5                91
#define MZ2_SOLDIER_BLASTER_6                   92
#define MZ2_SOLDIER_SHOTGUN_6                   93
#define MZ2_SOLDIER_MACHINEGUN_6                94
#define MZ2_SOLDIER_BLASTER_7                   95
#define MZ2_SOLDIER_SHOTGUN_7                   96
#define MZ2_SOLDIER_MACHINEGUN_7                97
#define MZ2_SOLDIER_BLASTER_8                   98
#define MZ2_SOLDIER_SHOTGUN_8                   99
#define MZ2_SOLDIER_MACHINEGUN_8                100

// --- Xian shit below ---
#define MZ2_MAKRON_BFG                          101
#define MZ2_MAKRON_BLASTER_1                    102
#define MZ2_MAKRON_BLASTER_2                    103
#define MZ2_MAKRON_BLASTER_3                    104
#define MZ2_MAKRON_BLASTER_4                    105
#define MZ2_MAKRON_BLASTER_5                    106
#define MZ2_MAKRON_BLASTER_6                    107
#define MZ2_MAKRON_BLASTER_7                    108
#define MZ2_MAKRON_BLASTER_8                    109
#define MZ2_MAKRON_BLASTER_9                    110
#define MZ2_MAKRON_BLASTER_10                   111
#define MZ2_MAKRON_BLASTER_11                   112
#define MZ2_MAKRON_BLASTER_12                   113
#define MZ2_MAKRON_BLASTER_13                   114
#define MZ2_MAKRON_BLASTER_14                   115
#define MZ2_MAKRON_BLASTER_15                   116
#define MZ2_MAKRON_BLASTER_16                   117
#define MZ2_MAKRON_BLASTER_17                   118
#define MZ2_MAKRON_RAILGUN_1                    119
#define MZ2_JORG_MACHINEGUN_L1                  120
#define MZ2_JORG_MACHINEGUN_L2                  121
#define MZ2_JORG_MACHINEGUN_L3                  122
#define MZ2_JORG_MACHINEGUN_L4                  123
#define MZ2_JORG_MACHINEGUN_L5                  124
#define MZ2_JORG_MACHINEGUN_L6                  125
#define MZ2_JORG_MACHINEGUN_R1                  126
#define MZ2_JORG_MACHINEGUN_R2                  127
#define MZ2_JORG_MACHINEGUN_R3                  128
#define MZ2_JORG_MACHINEGUN_R4                  129
#define MZ2_JORG_MACHINEGUN_R5                  130
#define MZ2_JORG_MACHINEGUN_R6                  131
#define MZ2_JORG_BFG_1                          132
#define MZ2_BOSS2_MACHINEGUN_R1                 133
#define MZ2_BOSS2_MACHINEGUN_R2                 134
#define MZ2_BOSS2_MACHINEGUN_R3                 135
#define MZ2_BOSS2_MACHINEGUN_R4                 136
#define MZ2_BOSS2_MACHINEGUN_R5                 137

//ROGUE (3.20 -FB)
#define MZ2_CARRIER_MACHINEGUN_L1               138
#define MZ2_CARRIER_MACHINEGUN_R1               139
#define MZ2_CARRIER_GRENADE                     140
#define MZ2_TURRET_MACHINEGUN                   141
#define MZ2_TURRET_ROCKET                       142
#define MZ2_TURRET_BLASTER                      143
#define MZ2_STALKER_BLASTER                     144
#define MZ2_DAEDALUS_BLASTER                    145
#define MZ2_MEDIC_BLASTER_2                     146
#define MZ2_CARRIER_RAILGUN                     147
#define MZ2_WIDOW_DISRUPTOR                     148
#define MZ2_WIDOW_BLASTER                       149
#define MZ2_WIDOW_RAIL                          150
#define MZ2_WIDOW_PLASMABEAM                    151	// PMM - not used
#define MZ2_CARRIER_MACHINEGUN_L2               152
#define MZ2_CARRIER_MACHINEGUN_R2               153
#define MZ2_WIDOW_RAIL_LEFT                     154
#define MZ2_WIDOW_RAIL_RIGHT                    155
#define MZ2_WIDOW_BLASTER_SWEEP1                156
#define MZ2_WIDOW_BLASTER_SWEEP2                157
#define MZ2_WIDOW_BLASTER_SWEEP3                158
#define MZ2_WIDOW_BLASTER_SWEEP4                159
#define MZ2_WIDOW_BLASTER_SWEEP5                160
#define MZ2_WIDOW_BLASTER_SWEEP6                161
#define MZ2_WIDOW_BLASTER_SWEEP7                162
#define MZ2_WIDOW_BLASTER_SWEEP8                163
#define MZ2_WIDOW_BLASTER_SWEEP9                164
#define MZ2_WIDOW_BLASTER_100                   165
#define MZ2_WIDOW_BLASTER_90                    166
#define MZ2_WIDOW_BLASTER_80                    167
#define MZ2_WIDOW_BLASTER_70                    168
#define MZ2_WIDOW_BLASTER_60                    169
#define MZ2_WIDOW_BLASTER_50                    170
#define MZ2_WIDOW_BLASTER_40                    171
#define MZ2_WIDOW_BLASTER_30                    172
#define MZ2_WIDOW_BLASTER_20                    173
#define MZ2_WIDOW_BLASTER_10                    174
#define MZ2_WIDOW_BLASTER_0                     175
#define MZ2_WIDOW_BLASTER_10L                   176
#define MZ2_WIDOW_BLASTER_20L                   177
#define MZ2_WIDOW_BLASTER_30L                   178
#define MZ2_WIDOW_BLASTER_40L                   179
#define MZ2_WIDOW_BLASTER_50L                   180
#define MZ2_WIDOW_BLASTER_60L                   181
#define MZ2_WIDOW_BLASTER_70L                   182
#define MZ2_WIDOW_RUN_1                         183
#define MZ2_WIDOW_RUN_2                         184
#define MZ2_WIDOW_RUN_3                         185
#define MZ2_WIDOW_RUN_4                         186
#define MZ2_WIDOW_RUN_5                         187
#define MZ2_WIDOW_RUN_6                         188
#define MZ2_WIDOW_RUN_7                         189
#define MZ2_WIDOW_RUN_8                         190
#define MZ2_CARRIER_ROCKET_1                    191
#define MZ2_CARRIER_ROCKET_2                    192
#define MZ2_CARRIER_ROCKET_3                    193
#define MZ2_CARRIER_ROCKET_4                    194
#define MZ2_WIDOW2_BEAMER_1                     195
#define MZ2_WIDOW2_BEAMER_2                     196
#define MZ2_WIDOW2_BEAMER_3                     197
#define MZ2_WIDOW2_BEAMER_4                     198
#define MZ2_WIDOW2_BEAMER_5                     199
#define MZ2_WIDOW2_BEAM_SWEEP_1                 200
#define MZ2_WIDOW2_BEAM_SWEEP_2                 201
#define MZ2_WIDOW2_BEAM_SWEEP_3                 202
#define MZ2_WIDOW2_BEAM_SWEEP_4                 203
#define MZ2_WIDOW2_BEAM_SWEEP_5                 204
#define MZ2_WIDOW2_BEAM_SWEEP_6                 205
#define MZ2_WIDOW2_BEAM_SWEEP_7                 206
#define MZ2_WIDOW2_BEAM_SWEEP_8                 207
#define MZ2_WIDOW2_BEAM_SWEEP_9                 208
#define MZ2_WIDOW2_BEAM_SWEEP_10                209
#define MZ2_WIDOW2_BEAM_SWEEP_11                210
// ROGUE


// temp entity events
//
// Temp entity events are for things that happen
// at a location seperate from any existing entity.
// Temporary entity messages are explicitly constructed
// and broadcast.
typedef enum
{
  TE_GUNSHOT,
  TE_BLOOD,
  TE_BLASTER,
  TE_RAILTRAIL,
  TE_SHOTGUN,
  TE_EXPLOSION1,
  TE_EXPLOSION2,
  TE_ROCKET_EXPLOSION,
  TE_GRENADE_EXPLOSION,
  TE_SPARKS,
  TE_SPLASH,
  TE_BUBBLETRAIL,
  TE_SCREEN_SPARKS,
  TE_SHIELD_SPARKS,
  TE_BULLET_SPARKS,
  TE_LASER_SPARKS,
  TE_PARASITE_ATTACK,
  TE_ROCKET_EXPLOSION_WATER,
  TE_GRENADE_EXPLOSION_WATER,
  TE_MEDIC_CABLE_ATTACK,
  TE_BFG_EXPLOSION,
  TE_BFG_BIGEXPLOSION,
  TE_BOSSTPORT,			// used as '22' in a map, so DON'T RENUMBER!!!
  TE_BFG_LASER,
  TE_GRAPPLE_CABLE,
  TE_WELDING_SPARKS,
  // FROM 3.20 -FB
  TE_GREENBLOOD,
  TE_BLUEHYPERBLASTER,
  TE_PLASMA_EXPLOSION,
  TE_TUNNEL_SPARKS,
  //ROGUE
  TE_BLASTER2,
  TE_RAILTRAIL2,
  TE_FLAME,
  TE_LIGHTNING,
  TE_DEBUGTRAIL,
  TE_PLAIN_EXPLOSION,
  TE_FLASHLIGHT,
  TE_FORCEWALL,
  TE_HEATBEAM,
  TE_MONSTER_HEATBEAM,
  TE_STEAM,
  TE_BUBBLETRAIL2,
  TE_MOREBLOOD,
  TE_HEATBEAM_SPARKS,
  TE_HEATBEAM_STEAM,
  TE_CHAINFIST_SMOKE,
  TE_ELECTRIC_SPARKS,
  TE_TRACKER_EXPLOSION,
  TE_TELEPORT_EFFECT,
  TE_DBALL_GOAL,
  TE_WIDOWBEAMOUT,
  TE_NUKEBLAST,
  TE_WIDOWSPLASH,
  TE_EXPLOSION1_BIG,
  TE_EXPLOSION1_NP,
  TE_FLECHETTE
    //ROGUE
    // ^^^
}
temp_event_t;

#define SPLASH_UNKNOWN          0
#define SPLASH_SPARKS           1
#define SPLASH_BLUE_WATER       2
#define SPLASH_BROWN_WATER      3
#define SPLASH_SLIME            4
#define SPLASH_LAVA             5
#define SPLASH_BLOOD            6


// sound channels
// channel 0 never willingly overrides
// other channels (1-7) allways override a playing sound on that channel
#define CHAN_AUTO               0
#define CHAN_WEAPON             1
#define CHAN_VOICE              2
#define CHAN_ITEM               3
#define CHAN_BODY               4
// modifier flags
#define CHAN_NO_PHS_ADD         8	// send to all clients, not just ones in PHS (ATTN 0 will also do this)
#define CHAN_RELIABLE           16	// send by reliable message, not datagram


// sound attenuation values
#define ATTN_NONE               0	// full volume the entire level
#define ATTN_LOUD (loud_guns->value ? 0.4 : 1)
#define ATTN_NORM               1
#define ATTN_IDLE               2
#define ATTN_STATIC             3	// diminish very rapidly with distance


// player_state->stats[] indexes
#define STAT_HEALTH_ICON                0
#define STAT_HEALTH                     1
#define STAT_AMMO_ICON                  2
#define STAT_AMMO                       3
#define STAT_TEAM_ICON                  4
#define STAT_ARMOR                      5
#define STAT_SELECTED_ICON              6
#define STAT_PICKUP_ICON                7
#define STAT_PICKUP_STRING              8
#define STAT_TIMER_ICON                 9
#define STAT_TIMER                      10
#define STAT_HELPICON                   11
#define STAT_SELECTED_ITEM              12
#define STAT_LAYOUTS                    13
#define STAT_FRAGS                      14
#define STAT_FLASHES                    15	// cleared each frame, 1 = health, 2 = armor

//zucc need some new ones
#define STAT_CLIP_ICON                  16
#define STAT_CLIP                       17
#define STAT_SNIPER_ICON                18
#define STAT_ITEMS_ICON                 19
#define STAT_WEAPONS_ICON               20
#define STAT_ID_VIEW                    21

//FIREBLADE
#define STAT_TEAM_HEADER                22
#define STAT_FLAG_PIC                   23
#define STAT_TEAM1_PIC                  24
#define STAT_TEAM2_PIC                  25
#define STAT_TEAM1_SCORE                26
#define STAT_TEAM2_SCORE                27
//FIREBLADE

//zucc more for me
#define STAT_GRENADE_ICON               28
#define STAT_GRENADES                   29

#define STAT_TEAM3_PIC			30
#define STAT_TEAM3_SCORE		31

#define STAT_TEAM1_HEADER               30
#define STAT_TEAM2_HEADER               31

#define MAX_STATS                       32


// dmflags->value flags
#define DF_NO_HEALTH            1
#define DF_NO_ITEMS             2
#define DF_WEAPONS_STAY         4
#define DF_NO_FALLING           8
#define DF_INSTANT_ITEMS        16
#define DF_SAME_LEVEL           32
#define DF_SKINTEAMS            64
#define DF_MODELTEAMS           128
#define DF_NO_FRIENDLY_FIRE     256
#define DF_SPAWN_FARTHEST       512
#define DF_FORCE_RESPAWN        1024
#define DF_NO_ARMOR             2048
#define DF_ALLOW_EXIT           4096
#define DF_INFINITE_AMMO        8192
#define DF_QUAD_DROP            16384
#define DF_FIXED_FOV            32768

#define DF_WEAPON_RESPAWN       0x00010000

// FROM 3.20 -FB
// RAFAEL
/*#define DF_QUADFIRE_DROP        0x00010000	// 65536

//ROGUE
#define DF_NO_MINES             0x00020000
#define DF_NO_STACK_DOUBLE      0x00040000
#define DF_NO_NUKES             0x00080000
#define DF_NO_SPHERES           0x00100000*/
//ROGUE
// ^^^


/*
   ==========================================================

   ELEMENTS COMMUNICATED ACROSS THE NET

   ==========================================================
 */

// default server FPS
#define BASE_FRAMERATE          10
#define BASE_FRAMETIME          100
#define BASE_1_FRAMETIME        0.01f   // 1/BASE_FRAMETIME
#define BASE_FRAMETIME_1000     0.1f    // BASE_FRAMETIME/1000

// maximum variable FPS factor
#define MAX_FRAMEDIV    6

#define ANGLE2SHORT(x)  ((int)((x)*65536/360) & 65535)
#define SHORT2ANGLE(x)  ((x)*(360.0/65536))


//
// config strings are a general means of communication from
// the server to all connected clients.
// Each config string can be at most MAX_QPATH characters.
//
#define CS_NAME                 0
#define CS_CDTRACK              1
#define CS_SKY                  2
#define CS_SKYAXIS              3	// %f %f %f format
#define CS_SKYROTATE            4
#define CS_STATUSBAR            5	// display program string

// FROM 3.20 -FB
#define CS_AIRACCEL             29	// air acceleration control
// ^^^
#define CS_MAXCLIENTS           30
#define CS_MAPCHECKSUM          31	// for catching cheater maps

#define CS_MODELS           32
#define CS_SOUNDS           (CS_MODELS + MAX_MODELS)
#define CS_IMAGES           (CS_SOUNDS + MAX_SOUNDS)
#define CS_LIGHTS           (CS_IMAGES + MAX_IMAGES)
#define CS_ITEMS            (CS_LIGHTS + MAX_LIGHTSTYLES)
#define CS_PLAYERSKINS      (CS_ITEMS + MAX_ITEMS)
#define CS_GENERAL          (CS_PLAYERSKINS + MAX_CLIENTS)  //1568
#define MAX_CONFIGSTRINGS   (CS_GENERAL + MAX_GENERAL)      //2080

//QW// The 2080 magic number comes from q_shared.h of the original game.
// No game mod can go over this 2080 limit.
//==============================================


// entity_state_t->event values
// ertity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.
// All muzzle flashes really should be converted to events...
typedef enum
{
  EV_NONE,
  EV_ITEM_RESPAWN,
  EV_FOOTSTEP,
  EV_FALLSHORT,
  EV_FALL,
  EV_FALLFAR,
  EV_PLAYER_TELEPORT,
  // FROM 3.20 -FB
  EV_OTHER_TELEPORT
    // ^^^
}
entity_event_t;


// entity_state_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
typedef struct entity_state_s
{
  int number;			// edict index

  vec3_t origin;
  vec3_t angles;
  vec3_t old_origin;		// for lerping

  int modelindex;
  int modelindex2, modelindex3, modelindex4;	// weapons, CTF flags, etc

  int frame;
  int skinnum;
  // FROM 3.20 -FB
  unsigned int effects;
  // ^^^
  int renderfx;
  int solid;			// for client side prediction, 8*(bits 0-4) is x/y radius
  // 8*(bits 5-9) is z down distance, 8(bits10-15) is z up
  // gi.linkentity sets this properly

  int sound;			// for looping sounds, to guarantee shutoff

  int event;			// impulse events -- muzzle flashes, footsteps, etc
  // events only go out for a single frame, they
  // are automatically cleared each frame

}
entity_state_t;

//==============================================


// player_state_t is the information needed in addition to pmove_state_t
// to rendered a view.  There will only be 10 player_state_t sent each second,
// but the number of pmove_state_t changes will be reletive to client
// frame rates
typedef struct
{
  pmove_state_t pmove;		// for prediction

  // these fields do not need to be communicated bit-precise

  vec3_t viewangles;		// for fixed views

  vec3_t viewoffset;		// add to pmovestate->origin

  vec3_t kick_angles;		// add to view direction to get render angles
  // set by weapon kicks, pain effects, etc

  vec3_t gunangles;
  vec3_t gunoffset;
  int gunindex;
  int gunframe;

  float blend[4];		// rgba full screen effect

  float fov;			// horizontal field of view

  int rdflags;			// refdef flags

  short stats[MAX_STATS];	// fast status bar updates
}
player_state_t;


// Reki : Cvar Sync info shared between engine and game
#define CVARSYNC_MAXSIZE	64
#define CVARSYNC_MAX		32
typedef struct {
	char name[CVARSYNC_MAXSIZE];
	char value[CVARSYNC_MAXSIZE];
} cvarsync_t;

typedef char cvarsyncvalue_t[CVARSYNC_MAXSIZE];

#endif
