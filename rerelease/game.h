// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

// game.h - game API stuff
#pragma once

#include <array>
#include <limits.h>

// compatibility with legacy float[3] stuff for engine
#ifdef GAME_INCLUDE
    using gvec3_t = vec3_t;
    using gvec3_ptr_t = vec3_t *;
    using gvec3_cptr_t = const vec3_t *;
    using gvec3_ref_t = vec3_t &;
    using gvec3_cref_t = const vec3_t &;
    using gvec4_t = std::array<float, 4>;
#else
    using gvec3_t = float[3];
    using gvec3_ptr_t = gvec3_t;
    using gvec3_ref_t = gvec3_t;
    using gvec3_cref_t = const gvec3_t;
    using gvec3_cptr_t = const gvec3_t;
    using gvec4_t = float[4];
#endif
    
constexpr size_t MAX_SPLIT_PLAYERS = 8;

struct rgba_t
{
    uint8_t r, g, b, a;
};

struct vec2_t
{
    float x, y;
};

constexpr rgba_t rgba_red { 255, 0, 0, 255 };
constexpr rgba_t rgba_blue { 0, 0, 255, 255 };
constexpr rgba_t rgba_green { 0, 255, 0, 255 };
constexpr rgba_t rgba_yellow { 255, 255, 0, 255 };
constexpr rgba_t rgba_white { 255, 255, 255, 255 };
constexpr rgba_t rgba_black { 0, 0, 0, 255 };
constexpr rgba_t rgba_cyan { 0, 255, 255, 255 };
constexpr rgba_t rgba_magenta { 255, 0, 255, 255 };
constexpr rgba_t rgba_orange { 116, 61, 50, 255 };

constexpr size_t MAX_NETNAME = 32;

constexpr float STEPSIZE = 18.0f;

// ugly hack to support bitflags on enums
// and use enable_if to prevent confusing cascading
// errors if you use the operators wrongly
#define MAKE_ENUM_BITFLAGS(T)                                                                                          \
	constexpr T operator~(const T &v)                                                                                  \
	{                                                                                                                  \
		return static_cast<T>(~static_cast<std::underlying_type_t<T>>(v));                                             \
	}                                                                                                                  \
	constexpr T operator|(const T &v, const T &v2)                                                                     \
	{                                                                                                                  \
		return static_cast<T>(static_cast<std::underlying_type_t<T>>(v) | static_cast<std::underlying_type_t<T>>(v2)); \
	}                                                                                                                  \
	constexpr T operator&(const T &v, const T &v2)                                                                     \
	{                                                                                                                  \
		return static_cast<T>(static_cast<std::underlying_type_t<T>>(v) & static_cast<std::underlying_type_t<T>>(v2)); \
	}                                                                                                                  \
	constexpr T operator^(const T &v, const T &v2)                                                                     \
	{                                                                                                                  \
		return static_cast<T>(static_cast<std::underlying_type_t<T>>(v) ^ static_cast<std::underlying_type_t<T>>(v2)); \
	}                                                                                                                  \
	template<typename T2 = T, typename = std::enable_if_t<std::is_same_v<T2, T>>>                                      \
	constexpr T &operator|=(T &v, const T &v2)                                                                         \
	{                                                                                                                  \
		v = v | v2;                                                                                                    \
		return v;                                                                                                      \
	}                                                                                                                  \
	template<typename T2 = T, typename = std::enable_if_t<std::is_same_v<T2, T>>>                                      \
	constexpr T &operator&=(T &v, const T &v2)                                                                         \
	{                                                                                                                  \
		v = v & v2;                                                                                                    \
		return v;                                                                                                      \
	}                                                                                                                  \
	template<typename T2 = T, typename = std::enable_if_t<std::is_same_v<T2, T>>>                                      \
	constexpr T &operator^=(T &v, const T &v2)                                                                         \
	{                                                                                                                  \
		v = v ^ v2;                                                                                                    \
		return v;                                                                                                      \
	}

using byte = uint8_t;

// bit simplification
template<size_t n>
using bit_t = std::conditional_t<n >= 32, uint64_t, uint32_t>;

// template is better for this because you can see
// it in the hover-over preview
template<size_t n>
constexpr bit_t<n> bit_v = 1ull << n;

#if defined(KEX_Q2GAME_EXPORTS)
    #define Q2GAME_API extern "C" __declspec( dllexport )
#elif defined(KEX_Q2GAME_IMPORTS)
    #define Q2GAME_API extern "C" __declspec( dllimport )
#else
    #define Q2GAME_API
#endif

// game.h -- game dll information visible to server
// PARIL_NEW_API - value likely not used by any other Q2-esque engine in the wild
constexpr int32_t GAME_API_VERSION = 2023;
constexpr int32_t CGAME_API_VERSION = 2022;

// forward declarations
struct edict_t;
struct gclient_t;

constexpr size_t MAX_STRING_CHARS = 1024; // max length of a string passed to Cmd_TokenizeString
constexpr size_t MAX_STRING_TOKENS = 80;  // max tokens resulting from Cmd_TokenizeString
constexpr size_t MAX_TOKEN_CHARS = 512;   // max length of an individual token

constexpr size_t MAX_QPATH = 64;   // max length of a quake game pathname
constexpr size_t MAX_OSPATH = 128; // max length of a filesystem pathname

//
// per-level limits
//
constexpr size_t MAX_CLIENTS = 256; // absolute limit
constexpr size_t MAX_EDICTS = 8192; // upper limit, due to svc_sound encoding as 15 bits
constexpr size_t MAX_LIGHTSTYLES = 256;
constexpr size_t MAX_MODELS = 8192; // these are sent over the net as shorts
constexpr size_t MAX_SOUNDS = 2048; // so they cannot be blindly increased
constexpr size_t MAX_IMAGES = 512;
constexpr size_t MAX_ITEMS = 256;
constexpr size_t MAX_GENERAL = (MAX_CLIENTS * 2); // general config strings

// [Sam-KEX]
constexpr size_t MAX_SHADOW_LIGHTS = 256;

// game print flags
enum print_type_t
{
	PRINT_LOW = 0,	  // pickup messages
	PRINT_MEDIUM = 1, // death messages
	PRINT_HIGH = 2,	  // critical messages
	PRINT_CHAT = 3,	  // chat messages
    PRINT_TYPEWRITER = 4, // centerprint but typed out one char at a time
    PRINT_CENTER = 5, // centerprint without a separate function (loc variants only)
    PRINT_TTS = 6, // PRINT_HIGH but will speak for players with narration on

    PRINT_BROADCAST = (1 << 3), // Bitflag, add to message to broadcast print to all clients.
    PRINT_NO_NOTIFY = (1 << 4) // Bitflag, don't put on notify
};

MAKE_ENUM_BITFLAGS(print_type_t);

// [Paril-KEX] max number of arguments (not including the base) for
// localization prints
constexpr size_t MAX_LOCALIZATION_ARGS = 8;

// destination class for gi.multicast()
enum multicast_t
{
	MULTICAST_ALL,
	MULTICAST_PHS,
	MULTICAST_PVS
};

/*
==========================================================

CVARS (console variables)

==========================================================
*/

enum cvar_flags_t : uint32_t
{
	CVAR_NOFLAGS = 0,
	CVAR_ARCHIVE = bit_v<0>,	 // set to cause it to be saved to config
	CVAR_USERINFO = bit_v<1>,	 // added to userinfo  when changed
	CVAR_SERVERINFO = bit_v<2>,  // added to serverinfo when changed
	CVAR_NOSET = bit_v<3>,		 // don't allow change from console at all,
						         // but can be set from the command line
	CVAR_LATCH = bit_v<4>,		 // save changes until server restart
    CVAR_USER_PROFILE = bit_v<5>, // like CVAR_USERINFO but not sent to server
};
MAKE_ENUM_BITFLAGS(cvar_flags_t);

// nothing outside the Cvar_*() functions should modify these fields!
struct cvar_t
{
	char		 *name;
	char		 *string;
	char		 *latched_string; // for CVAR_LATCH vars
	cvar_flags_t flags;
	int32_t      modified_count; // changed each time the cvar is changed, but never zero
	float		 value;
	cvar_t	    *next;
	int32_t      integer; // integral value
};

// convenience function to check if the given cvar ptr has been
// modified from its previous modified value, and automatically
// assigns modified to cvar's current value
inline bool Cvar_WasModified(const cvar_t *cvar, int32_t &modified)
{
    if (cvar->modified_count != modified)
    {
        modified = cvar->modified_count;
        return true;
    }

    return false;
}

/*
==============================================================

COLLISION DETECTION

==============================================================
*/

// lower bits are stronger, and will eat weaker brushes completely
enum contents_t : uint32_t
{
	CONTENTS_NONE = 0,
	CONTENTS_SOLID = bit_v<0>,	 // an eye is never valid in a solid
	CONTENTS_WINDOW = bit_v<1>, // translucent, but not watery
	CONTENTS_AUX = bit_v<2>,
	CONTENTS_LAVA = bit_v<3>,
	CONTENTS_SLIME = bit_v<4>,
	CONTENTS_WATER = bit_v<5>,
	CONTENTS_MIST = bit_v<6>,

	// remaining contents are non-visible, and don't eat brushes

    CONTENTS_NO_WATERJUMP = bit_v<13>, // [Paril-KEX] this brush cannot be waterjumped out of
    CONTENTS_PROJECTILECLIP = bit_v<14>, // [Paril-KEX] projectiles will collide with this

	CONTENTS_AREAPORTAL = bit_v<15>,

	CONTENTS_PLAYERCLIP = bit_v<16>,
	CONTENTS_MONSTERCLIP = bit_v<17>,

	// currents can be added to any other contents, and may be mixed
	CONTENTS_CURRENT_0 = bit_v<18>,
	CONTENTS_CURRENT_90 = bit_v<19>,
	CONTENTS_CURRENT_180 = bit_v<20>,
	CONTENTS_CURRENT_270 = bit_v<21>,
	CONTENTS_CURRENT_UP = bit_v<22>,
	CONTENTS_CURRENT_DOWN = bit_v<23>,

	CONTENTS_ORIGIN = bit_v<24>, // removed before bsping an entity

	CONTENTS_MONSTER = bit_v<25>, // should never be on a brush, only in game
	CONTENTS_DEADMONSTER = bit_v<26>,

	CONTENTS_DETAIL = bit_v<27>,	   // brushes to be added after vis leafs
	CONTENTS_TRANSLUCENT = bit_v<28>, // auto set if any surface has trans
	CONTENTS_LADDER = bit_v<29>,

    CONTENTS_PLAYER = bit_v<30>, // [Paril-KEX] should never be on a brush, only in game; player
    CONTENTS_PROJECTILE = bit_v<31>  // [Paril-KEX] should never be on a brush, only in game; projectiles.
                                     // used to solve deadmonster collision issues.
};

MAKE_ENUM_BITFLAGS(contents_t);

constexpr contents_t LAST_VISIBLE_CONTENTS = CONTENTS_MIST;

enum surfflags_t : uint32_t
{
    SURF_NONE = 0,
	SURF_LIGHT = bit_v<0>, // value will hold the light strength
	SURF_SLICK = bit_v<1>, // effects game physics
	SURF_SKY = bit_v<2>,	  // don't draw, but add to skybox
	SURF_WARP = bit_v<3>,  // turbulent water warp
	SURF_TRANS33 = bit_v<4>,
	SURF_TRANS66 = bit_v<5>,
	SURF_FLOWING = bit_v<6>, // scroll towards angle
	SURF_NODRAW = bit_v<7>,	 // don't bother referencing the texture
    SURF_ALPHATEST = bit_v<25>,   // [Paril-KEX] alpha test using widely supported flag
    SURF_N64_UV = bit_v<28>,  // [Sam-KEX] Stretches texture UVs
    SURF_N64_SCROLL_X = bit_v<29>,  // [Sam-KEX] Texture scroll X-axis
    SURF_N64_SCROLL_Y = bit_v<30>,  // [Sam-KEX] Texture scroll Y-axis
    SURF_N64_SCROLL_FLIP = bit_v<31>  // [Sam-KEX] Flip direction of texture scroll
};

MAKE_ENUM_BITFLAGS(surfflags_t);

// content masks
constexpr contents_t MASK_ALL = static_cast<contents_t>(-1);
constexpr contents_t MASK_SOLID = (CONTENTS_SOLID | CONTENTS_WINDOW);
constexpr contents_t MASK_PLAYERSOLID = (CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_WINDOW | CONTENTS_MONSTER | CONTENTS_PLAYER);
constexpr contents_t MASK_DEADSOLID = (CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_WINDOW);
constexpr contents_t MASK_MONSTERSOLID = (CONTENTS_SOLID | CONTENTS_MONSTERCLIP | CONTENTS_WINDOW | CONTENTS_MONSTER | CONTENTS_PLAYER);
constexpr contents_t MASK_WATER = (CONTENTS_WATER | CONTENTS_LAVA | CONTENTS_SLIME);
constexpr contents_t MASK_OPAQUE = (CONTENTS_SOLID | CONTENTS_SLIME | CONTENTS_LAVA);
constexpr contents_t MASK_SHOT = (CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_PLAYER | CONTENTS_WINDOW | CONTENTS_DEADMONSTER);
constexpr contents_t MASK_CURRENT = (CONTENTS_CURRENT_0 | CONTENTS_CURRENT_90 | CONTENTS_CURRENT_180 | CONTENTS_CURRENT_270 | CONTENTS_CURRENT_UP | CONTENTS_CURRENT_DOWN);
constexpr contents_t MASK_BLOCK_SIGHT = ( CONTENTS_SOLID | CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_MONSTER | CONTENTS_PLAYER );
constexpr contents_t MASK_NAV_SOLID = ( CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_WINDOW );
constexpr contents_t MASK_LADDER_NAV_SOLID = ( CONTENTS_SOLID | CONTENTS_WINDOW );
constexpr contents_t MASK_WALK_NAV_SOLID = ( CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_WINDOW | CONTENTS_MONSTERCLIP );
constexpr contents_t MASK_PROJECTILE = MASK_SHOT | CONTENTS_PROJECTILECLIP;

// gi.BoxEdicts() can return a list of either solid or trigger entities
// FIXME: eliminate AREA_ distinction?
enum solidity_area_t
{
	AREA_SOLID = 1,
	AREA_TRIGGERS = 2
};

// plane_t structure
// !!! if this is changed, it must be changed in asm code too !!!
struct cplane_t
{
	gvec3_t	normal;
	float	dist;
	byte	type;	 // for fast side tests
	byte	signbits; // signx + (signy<<1) + (signz<<1)
	byte	pad[2];
};

// [Paril-KEX]
constexpr size_t MAX_MATERIAL_NAME = 16;

struct csurface_t
{
	char        name[32];
	surfflags_t flags;
	int32_t		value;

    // [Paril-KEX]
    uint32_t    id; // unique texinfo ID, offset by 1 (0 is 'null')
    char        material[MAX_MATERIAL_NAME];
};

// a trace is returned when a box is swept through the world
struct trace_t
{
	bool	    allsolid;	// if true, plane is not valid
	bool        startsolid; // if true, the initial point was in a solid area
	float		fraction;	// time completed, 1.0 = didn't hit anything
	gvec3_t		endpos;		// final position
	cplane_t	plane;		// surface normal at impact
	csurface_t *surface;	// surface hit
	contents_t	contents;	// contents on other side of surface hit
	edict_t	    *ent;		// not set by CM_*() functions

    // [Paril-KEX] the second-best surface hit from a trace
    cplane_t	plane2;		// second surface normal at impact
	csurface_t *surface2;	// second surface hit
};

// pmove_state_t is the information necessary for client side movement
// prediction
enum pmtype_t
{
	// can accelerate and turn
	PM_NORMAL,
    PM_GRAPPLE, // [Paril-KEX] pull towards velocity, no gravity
	PM_NOCLIP,
    PM_SPECTATOR, // [Paril-KEX] clip against walls, but not entities
	// no acceleration or turning
	PM_DEAD,
	PM_GIB, // different bounding box
	PM_FREEZE
};

// pmove->pm_flags
enum pmflags_t : uint16_t
{
    PMF_NONE = 0,
	PMF_DUCKED = bit_v<0>,
	PMF_JUMP_HELD = bit_v<1>,
	PMF_ON_GROUND = bit_v<2>,
	PMF_TIME_WATERJUMP = bit_v<3>, // pm_time is waterjump
	PMF_TIME_LAND = bit_v<4>,		// pm_time is time before rejump
	PMF_TIME_TELEPORT = bit_v<5>, // pm_time is non-moving time
	PMF_NO_POSITIONAL_PREDICTION = bit_v<6>,	// temporarily disables positional prediction (used for grappling hook)
    PMF_ON_LADDER = bit_v<7>,    // signal to game that we are on a ladder
    PMF_NO_ANGULAR_PREDICTION = bit_v<8>, // temporary disables angular prediction
    PMF_IGNORE_PLAYER_COLLISION = bit_v<9>, // don't collide with other players
    PMF_TIME_TRICK = bit_v<10>, // pm_time is trick jump time
};

MAKE_ENUM_BITFLAGS(pmflags_t);

// this structure needs to be communicated bit-accurate
// from the server to the client to guarantee that
// prediction stays in sync.
// if any part of the game code modifies this struct, it
// will result in a prediction error of some degree.
struct pmove_state_t
{
	pmtype_t pm_type;

	vec3_t                 origin;
	vec3_t                 velocity;
	pmflags_t			   pm_flags; // ducked, jump_held, etc
	uint16_t			   pm_time;
	int16_t				   gravity;
	gvec3_t                delta_angles; // add to command angles to get view direction
										 // changed by spawns, rotating objects, and teleporters
    int8_t                 viewheight; // view height, added to origin[2] + viewoffset[2], for crouching
};

//
// button bits
//
enum button_t : uint8_t
{
	BUTTON_NONE = 0,
	BUTTON_ATTACK = bit_v<0>,
	BUTTON_USE = bit_v<1>,
    BUTTON_HOLSTER = bit_v<2>, // [Paril-KEX]
    BUTTON_JUMP = bit_v<3>,
    BUTTON_CROUCH = bit_v<4>,
	BUTTON_ANY = bit_v<7> // any key whatsoever
};

MAKE_ENUM_BITFLAGS(button_t);

// usercmd_t is sent to the server each client frame
struct usercmd_t
{
	byte				   msec;
	button_t			   buttons;
	gvec3_t                angles;
	float				   forwardmove, sidemove;
	uint32_t               server_frame;	   // for integrity, etc
};

enum water_level_t : uint8_t
{
	WATER_NONE,
	WATER_FEET,
	WATER_WAIST,
	WATER_UNDER
};

// player_state_t->refdef flags
enum refdef_flags_t : uint8_t
{
    RDF_NONE = 0,
    RDF_UNDERWATER = bit_v<0>,	  // warp the screen as appropriate
    RDF_NOWORLDMODEL = bit_v<1>, // used for player configuration screen

    // ROGUE
    RDF_IRGOGGLES = bit_v<2>,
    RDF_UVGOGGLES = bit_v<3>,
    // ROGUE

    RDF_NO_WEAPON_LERP = bit_v<4>
};

constexpr size_t MAXTOUCH = 32;

struct touch_list_t
{
    uint32_t num = 0;
    std::array<trace_t, MAXTOUCH> traces;
};

struct pmove_t
{
    // state (in / out)
    pmove_state_t s;

    // command (in)
    usercmd_t cmd;
    bool      snapinitial; // if s has been changed outside pmove

    // results (out)
    touch_list_t touch;

    gvec3_t viewangles; // clamped

    gvec3_t mins, maxs; // bounding box size

    edict_t      *groundentity;
    cplane_t      groundplane;
    contents_t    watertype;
    water_level_t waterlevel;

    edict_t *player; // opaque handle

    // clip against world & entities
    trace_t (*trace)(gvec3_cref_t start, gvec3_cptr_t mins, gvec3_cptr_t maxs, gvec3_cref_t end, const edict_t* passent, contents_t contentmask);
    // [Paril-KEX] clip against world only
    trace_t (*clip)(gvec3_cref_t start, gvec3_cptr_t mins, gvec3_cptr_t maxs, gvec3_cref_t end, contents_t contentmask);

    contents_t (*pointcontents)(gvec3_cref_t point);

    // [KEX] variables (in)
    vec3_t viewoffset; // last viewoffset (for accurate calculation of blending)

    // [KEX] results (out)
    gvec4_t screen_blend;
    refdef_flags_t rdflags; // merged with rdflags from server
    bool jump_sound; // play jump sound
    bool step_clip; // we clipped on top of an object from below
    float impact_delta; // impact delta, for falling damage
};


// entity_state_t->effects
// Effects are things handled on the client side (lights, particles, frame animations)
// that happen constantly on the given entity.
// An entity that has effects will be sent to the client
// even if it has a zero index model.
enum effects_t : uint64_t
{
    EF_NONE             = 0,           // no effects
    EF_ROTATE           = bit_v<0>,  // rotate (bonus items)
    EF_GIB              = bit_v<1>,  // leave a trail
    EF_BOB              = bit_v<2>,  // bob (bonus items)
    EF_BLASTER          = bit_v<3>,  // redlight + trail
    EF_ROCKET           = bit_v<4>,  // redlight + trail
    EF_GRENADE          = bit_v<5>,
    EF_HYPERBLASTER     = bit_v<6>,
    EF_BFG              = bit_v<7>,
    EF_COLOR_SHELL      = bit_v<8>,
    EF_POWERSCREEN      = bit_v<9>,
    EF_ANIM01           = bit_v<10>,  // automatically cycle between frames 0 and 1 at 2 hz
    EF_ANIM23           = bit_v<11>,  // automatically cycle between frames 2 and 3 at 2 hz
    EF_ANIM_ALL         = bit_v<12>,  // automatically cycle through all frames at 2hz
    EF_ANIM_ALLFAST     = bit_v<13>,  // automatically cycle through all frames at 10hz
    EF_FLIES            = bit_v<14>,
    EF_QUAD             = bit_v<15>,
    EF_PENT             = bit_v<16>,
    EF_TELEPORTER       = bit_v<17>,  // particle fountain
    EF_FLAG1            = bit_v<18>,
    EF_FLAG2            = bit_v<19>,
    // RAFAEL
    EF_IONRIPPER        = bit_v<20>,
    EF_GREENGIB         = bit_v<21>,
    EF_BLUEHYPERBLASTER = bit_v<22>,
    EF_SPINNINGLIGHTS   = bit_v<23>,
    EF_PLASMA           = bit_v<24>,
    EF_TRAP             = bit_v<25>,

    // ROGUE
    EF_TRACKER          = bit_v<26>,
    EF_DOUBLE           = bit_v<27>,
    EF_SPHERETRANS      = bit_v<28>,
    EF_TAGTRAIL         = bit_v<29>,
    EF_HALF_DAMAGE      = bit_v<30>,
    EF_TRACKERTRAIL     = bit_v<31>,
    // ROGUE

    EF_DUALFIRE        = bit_v<32>, // [KEX] dualfire damage color shell
    EF_HOLOGRAM        = bit_v<33>, // [Paril-KEX] N64 hologram
    EF_FLASHLIGHT      = bit_v<34>, // [Paril-KEX] project flashlight, only for players
    EF_BARREL_EXPLODING= bit_v<35>,
    EF_TELEPORTER2     = bit_v<36>, // [Paril-KEX] n64 teleporter
    EF_GRENADE_LIGHT   = bit_v<37>
};

MAKE_ENUM_BITFLAGS(effects_t);

constexpr effects_t EF_FIREBALL = EF_ROCKET | EF_GIB;

// entity_state_t->renderfx flags
enum renderfx_t : uint32_t
{
    RF_NONE = 0,
    RF_MINLIGHT = bit_v<0>,	// always have some light (viewmodel)
    RF_VIEWERMODEL = bit_v<1>, // don't draw through eyes, only mirrors
    RF_WEAPONMODEL = bit_v<2>, // only draw through eyes
    RF_FULLBRIGHT = bit_v<3>,	// always draw full intensity
    RF_DEPTHHACK = bit_v<4>,	// for view weapon Z crunching
    RF_TRANSLUCENT = bit_v<5>,
    RF_NO_ORIGIN_LERP = bit_v<6>, // no interpolation for origins
    RF_BEAM = bit_v<7>,
    RF_CUSTOMSKIN = bit_v<8>, // [Paril-KEX] implemented; set skinnum (or frame for RF_FLARE) to specify
                              // an image in CS_IMAGES to use as skin.
    RF_GLOW = bit_v<9>,		 // pulse lighting for bonus items
    RF_SHELL_RED = bit_v<10>,
    RF_SHELL_GREEN = bit_v<11>,
    RF_SHELL_BLUE = bit_v<12>,
    RF_NOSHADOW = bit_v<13>,
    RF_CASTSHADOW = bit_v<14>, // [Sam-KEX]

    // ROGUE
    RF_IR_VISIBLE = bit_v<15>,
    RF_SHELL_DOUBLE = bit_v<16>,
    RF_SHELL_HALF_DAM = bit_v<17>,
    RF_USE_DISGUISE = bit_v<18>,
    // ROGUE

    RF_SHELL_LITE_GREEN = bit_v<19>,
    RF_CUSTOM_LIGHT = bit_v<20>, // [Paril-KEX] custom point dlight that is designed to strobe/be turned off; s.frame is radius, s.skinnum is color
    RF_FLARE = bit_v<21>, // [Sam-KEX]
    RF_OLD_FRAME_LERP = bit_v<22>, // [Paril-KEX] force model to lerp from oldframe in entity state; otherwise it uses last frame client received
    RF_DOT_SHADOW = bit_v<23>, // [Paril-KEX] draw blobby shadow
    RF_LOW_PRIORITY = bit_v<24>, // [Paril-KEX] low priority object; if we can't be added to the scene, don't bother replacing entities,
                                 // and we can be replaced if anything non-low-priority needs room
    RF_NO_LOD = bit_v<25>, // [Paril-KEX] never LOD
    RF_NO_STEREO = RF_WEAPONMODEL, // [Paril-KEX] this is a bit dumb, but, for looping noises if this is set there's no stereo
    RF_STAIR_STEP = bit_v<26>, // [Paril-KEX] re-tuned, now used to handle stair steps for monsters

    RF_FLARE_LOCK_ANGLE = RF_MINLIGHT
};

MAKE_ENUM_BITFLAGS(renderfx_t);

constexpr renderfx_t RF_BEAM_LIGHTNING = RF_BEAM | RF_GLOW; // [Paril-KEX] make a lightning bolt instead of a laser

MAKE_ENUM_BITFLAGS(refdef_flags_t);

//
// muzzle flashes / player effects
//
enum player_muzzle_t : uint8_t
{
    MZ_BLASTER = 0,
    MZ_MACHINEGUN = 1,
    MZ_SHOTGUN = 2,
    MZ_CHAINGUN1 = 3,
    MZ_CHAINGUN2 = 4,
    MZ_CHAINGUN3 = 5,
    MZ_RAILGUN = 6,
    MZ_ROCKET = 7,
    MZ_GRENADE = 8,
    MZ_LOGIN = 9,
    MZ_LOGOUT = 10,
    MZ_RESPAWN = 11,
    MZ_BFG = 12,
    MZ_SSHOTGUN = 13,
    MZ_HYPERBLASTER = 14,
    MZ_ITEMRESPAWN = 15,
    // RAFAEL
    MZ_IONRIPPER = 16,
    MZ_BLUEHYPERBLASTER = 17,
    MZ_PHALANX = 18,
    MZ_BFG2 = 19,
    MZ_PHALANX2 = 20,

    // ROGUE
    MZ_ETF_RIFLE = 30,
    MZ_PROX = 31, // [Paril-KEX]
    MZ_ETF_RIFLE_2 = 32, // [Paril-KEX] unused, so using it for the other barrel
    MZ_HEATBEAM = 33,
    MZ_BLASTER2 = 34,
    MZ_TRACKER = 35,
    MZ_NUKE1 = 36,
    MZ_NUKE2 = 37,
    MZ_NUKE4 = 38,
    MZ_NUKE8 = 39,
    // ROGUE

    MZ_SILENCED = bit_v<7>, // bit flag ORed with one of the above numbers
    MZ_NONE = 0        // "no" bitflags
};

MAKE_ENUM_BITFLAGS(player_muzzle_t);

//
// monster muzzle flashes
// NOTE: this needs to match the m_flash table!
//
enum monster_muzzleflash_id_t : uint16_t
{
    MZ2_UNUSED_0,

    MZ2_TANK_BLASTER_1,
    MZ2_TANK_BLASTER_2,
    MZ2_TANK_BLASTER_3,
    MZ2_TANK_MACHINEGUN_1,
    MZ2_TANK_MACHINEGUN_2,
    MZ2_TANK_MACHINEGUN_3,
    MZ2_TANK_MACHINEGUN_4,
    MZ2_TANK_MACHINEGUN_5,
    MZ2_TANK_MACHINEGUN_6,
    MZ2_TANK_MACHINEGUN_7,
    MZ2_TANK_MACHINEGUN_8,
    MZ2_TANK_MACHINEGUN_9,
    MZ2_TANK_MACHINEGUN_10,
    MZ2_TANK_MACHINEGUN_11,
    MZ2_TANK_MACHINEGUN_12,
    MZ2_TANK_MACHINEGUN_13,
    MZ2_TANK_MACHINEGUN_14,
    MZ2_TANK_MACHINEGUN_15,
    MZ2_TANK_MACHINEGUN_16,
    MZ2_TANK_MACHINEGUN_17,
    MZ2_TANK_MACHINEGUN_18,
    MZ2_TANK_MACHINEGUN_19,
    MZ2_TANK_ROCKET_1,
    MZ2_TANK_ROCKET_2,
    MZ2_TANK_ROCKET_3,

    MZ2_INFANTRY_MACHINEGUN_1,
    MZ2_INFANTRY_MACHINEGUN_2,
    MZ2_INFANTRY_MACHINEGUN_3,
    MZ2_INFANTRY_MACHINEGUN_4,
    MZ2_INFANTRY_MACHINEGUN_5,
    MZ2_INFANTRY_MACHINEGUN_6,
    MZ2_INFANTRY_MACHINEGUN_7,
    MZ2_INFANTRY_MACHINEGUN_8,
    MZ2_INFANTRY_MACHINEGUN_9,
    MZ2_INFANTRY_MACHINEGUN_10,
    MZ2_INFANTRY_MACHINEGUN_11,
    MZ2_INFANTRY_MACHINEGUN_12,
    MZ2_INFANTRY_MACHINEGUN_13,

    MZ2_SOLDIER_BLASTER_1,
    MZ2_SOLDIER_BLASTER_2,
    MZ2_SOLDIER_SHOTGUN_1,
    MZ2_SOLDIER_SHOTGUN_2,
    MZ2_SOLDIER_MACHINEGUN_1,
    MZ2_SOLDIER_MACHINEGUN_2,

    MZ2_GUNNER_MACHINEGUN_1,
    MZ2_GUNNER_MACHINEGUN_2,
    MZ2_GUNNER_MACHINEGUN_3,
    MZ2_GUNNER_MACHINEGUN_4,
    MZ2_GUNNER_MACHINEGUN_5,
    MZ2_GUNNER_MACHINEGUN_6,
    MZ2_GUNNER_MACHINEGUN_7,
    MZ2_GUNNER_MACHINEGUN_8,
    MZ2_GUNNER_GRENADE_1,
    MZ2_GUNNER_GRENADE_2,
    MZ2_GUNNER_GRENADE_3,
    MZ2_GUNNER_GRENADE_4,

    MZ2_CHICK_ROCKET_1,

    MZ2_FLYER_BLASTER_1,
    MZ2_FLYER_BLASTER_2,

    MZ2_MEDIC_BLASTER_1,

    MZ2_GLADIATOR_RAILGUN_1,

    MZ2_HOVER_BLASTER_1,

    MZ2_ACTOR_MACHINEGUN_1,

    MZ2_SUPERTANK_MACHINEGUN_1,
    MZ2_SUPERTANK_MACHINEGUN_2,
    MZ2_SUPERTANK_MACHINEGUN_3,
    MZ2_SUPERTANK_MACHINEGUN_4,
    MZ2_SUPERTANK_MACHINEGUN_5,
    MZ2_SUPERTANK_MACHINEGUN_6,
    MZ2_SUPERTANK_ROCKET_1,
    MZ2_SUPERTANK_ROCKET_2,
    MZ2_SUPERTANK_ROCKET_3,

    MZ2_BOSS2_MACHINEGUN_L1,
    MZ2_BOSS2_MACHINEGUN_L2,
    MZ2_BOSS2_MACHINEGUN_L3,
    MZ2_BOSS2_MACHINEGUN_L4,
    MZ2_BOSS2_MACHINEGUN_L5,
    MZ2_BOSS2_ROCKET_1,
    MZ2_BOSS2_ROCKET_2,
    MZ2_BOSS2_ROCKET_3,
    MZ2_BOSS2_ROCKET_4,

    MZ2_FLOAT_BLASTER_1,

    MZ2_SOLDIER_BLASTER_3,
    MZ2_SOLDIER_SHOTGUN_3,
    MZ2_SOLDIER_MACHINEGUN_3,
    MZ2_SOLDIER_BLASTER_4,
    MZ2_SOLDIER_SHOTGUN_4,
    MZ2_SOLDIER_MACHINEGUN_4,
    MZ2_SOLDIER_BLASTER_5,
    MZ2_SOLDIER_SHOTGUN_5,
    MZ2_SOLDIER_MACHINEGUN_5,
    MZ2_SOLDIER_BLASTER_6,
    MZ2_SOLDIER_SHOTGUN_6,
    MZ2_SOLDIER_MACHINEGUN_6,
    MZ2_SOLDIER_BLASTER_7,
    MZ2_SOLDIER_SHOTGUN_7,
    MZ2_SOLDIER_MACHINEGUN_7,
    MZ2_SOLDIER_BLASTER_8,
    MZ2_SOLDIER_SHOTGUN_8,
    MZ2_SOLDIER_MACHINEGUN_8,

    // --- Xian shit below ---
    MZ2_MAKRON_BFG,
    MZ2_MAKRON_BLASTER_1,
    MZ2_MAKRON_BLASTER_2,
    MZ2_MAKRON_BLASTER_3,
    MZ2_MAKRON_BLASTER_4,
    MZ2_MAKRON_BLASTER_5,
    MZ2_MAKRON_BLASTER_6,
    MZ2_MAKRON_BLASTER_7,
    MZ2_MAKRON_BLASTER_8,
    MZ2_MAKRON_BLASTER_9,
    MZ2_MAKRON_BLASTER_10,
    MZ2_MAKRON_BLASTER_11,
    MZ2_MAKRON_BLASTER_12,
    MZ2_MAKRON_BLASTER_13,
    MZ2_MAKRON_BLASTER_14,
    MZ2_MAKRON_BLASTER_15,
    MZ2_MAKRON_BLASTER_16,
    MZ2_MAKRON_BLASTER_17,
    MZ2_MAKRON_RAILGUN_1,
    MZ2_JORG_MACHINEGUN_L1,
    MZ2_JORG_MACHINEGUN_L2,
    MZ2_JORG_MACHINEGUN_L3,
    MZ2_JORG_MACHINEGUN_L4,
    MZ2_JORG_MACHINEGUN_L5,
    MZ2_JORG_MACHINEGUN_L6,
    MZ2_JORG_MACHINEGUN_R1,
    MZ2_JORG_MACHINEGUN_R2,
    MZ2_JORG_MACHINEGUN_R3,
    MZ2_JORG_MACHINEGUN_R4,
    MZ2_JORG_MACHINEGUN_R5,
    MZ2_JORG_MACHINEGUN_R6,
    MZ2_JORG_BFG_1,
    MZ2_BOSS2_MACHINEGUN_R1,
    MZ2_BOSS2_MACHINEGUN_R2,
    MZ2_BOSS2_MACHINEGUN_R3,
    MZ2_BOSS2_MACHINEGUN_R4,
    MZ2_BOSS2_MACHINEGUN_R5,

    // ROGUE
    MZ2_CARRIER_MACHINEGUN_L1,
    MZ2_CARRIER_MACHINEGUN_R1,
    MZ2_CARRIER_GRENADE,
    MZ2_TURRET_MACHINEGUN,
    MZ2_TURRET_ROCKET,
    MZ2_TURRET_BLASTER,
    MZ2_STALKER_BLASTER,
    MZ2_DAEDALUS_BLASTER,
    MZ2_MEDIC_BLASTER_2,
    MZ2_CARRIER_RAILGUN,
    MZ2_WIDOW_DISRUPTOR,
    MZ2_WIDOW_BLASTER,
    MZ2_WIDOW_RAIL,
    MZ2_WIDOW_PLASMABEAM, // PMM - not used
    MZ2_CARRIER_MACHINEGUN_L2,
    MZ2_CARRIER_MACHINEGUN_R2,
    MZ2_WIDOW_RAIL_LEFT,
    MZ2_WIDOW_RAIL_RIGHT,
    MZ2_WIDOW_BLASTER_SWEEP1,
    MZ2_WIDOW_BLASTER_SWEEP2,
    MZ2_WIDOW_BLASTER_SWEEP3,
    MZ2_WIDOW_BLASTER_SWEEP4,
    MZ2_WIDOW_BLASTER_SWEEP5,
    MZ2_WIDOW_BLASTER_SWEEP6,
    MZ2_WIDOW_BLASTER_SWEEP7,
    MZ2_WIDOW_BLASTER_SWEEP8,
    MZ2_WIDOW_BLASTER_SWEEP9,
    MZ2_WIDOW_BLASTER_100,
    MZ2_WIDOW_BLASTER_90,
    MZ2_WIDOW_BLASTER_80,
    MZ2_WIDOW_BLASTER_70,
    MZ2_WIDOW_BLASTER_60,
    MZ2_WIDOW_BLASTER_50,
    MZ2_WIDOW_BLASTER_40,
    MZ2_WIDOW_BLASTER_30,
    MZ2_WIDOW_BLASTER_20,
    MZ2_WIDOW_BLASTER_10,
    MZ2_WIDOW_BLASTER_0,
    MZ2_WIDOW_BLASTER_10L,
    MZ2_WIDOW_BLASTER_20L,
    MZ2_WIDOW_BLASTER_30L,
    MZ2_WIDOW_BLASTER_40L,
    MZ2_WIDOW_BLASTER_50L,
    MZ2_WIDOW_BLASTER_60L,
    MZ2_WIDOW_BLASTER_70L,
    MZ2_WIDOW_RUN_1,
    MZ2_WIDOW_RUN_2,
    MZ2_WIDOW_RUN_3,
    MZ2_WIDOW_RUN_4,
    MZ2_WIDOW_RUN_5,
    MZ2_WIDOW_RUN_6,
    MZ2_WIDOW_RUN_7,
    MZ2_WIDOW_RUN_8,
    MZ2_CARRIER_ROCKET_1,
    MZ2_CARRIER_ROCKET_2,
    MZ2_CARRIER_ROCKET_3,
    MZ2_CARRIER_ROCKET_4,
    MZ2_WIDOW2_BEAMER_1,
    MZ2_WIDOW2_BEAMER_2,
    MZ2_WIDOW2_BEAMER_3,
    MZ2_WIDOW2_BEAMER_4,
    MZ2_WIDOW2_BEAMER_5,
    MZ2_WIDOW2_BEAM_SWEEP_1,
    MZ2_WIDOW2_BEAM_SWEEP_2,
    MZ2_WIDOW2_BEAM_SWEEP_3,
    MZ2_WIDOW2_BEAM_SWEEP_4,
    MZ2_WIDOW2_BEAM_SWEEP_5,
    MZ2_WIDOW2_BEAM_SWEEP_6,
    MZ2_WIDOW2_BEAM_SWEEP_7,
    MZ2_WIDOW2_BEAM_SWEEP_8,
    MZ2_WIDOW2_BEAM_SWEEP_9,
    MZ2_WIDOW2_BEAM_SWEEP_10,
    MZ2_WIDOW2_BEAM_SWEEP_11,
    // ROGUE

    // [Paril-KEX]
    MZ2_SOLDIER_RIPPER_1,
    MZ2_SOLDIER_RIPPER_2,
    MZ2_SOLDIER_RIPPER_3,
    MZ2_SOLDIER_RIPPER_4,
    MZ2_SOLDIER_RIPPER_5,
    MZ2_SOLDIER_RIPPER_6,
    MZ2_SOLDIER_RIPPER_7,
    MZ2_SOLDIER_RIPPER_8,

    MZ2_SOLDIER_HYPERGUN_1,
    MZ2_SOLDIER_HYPERGUN_2,
    MZ2_SOLDIER_HYPERGUN_3,
    MZ2_SOLDIER_HYPERGUN_4,
    MZ2_SOLDIER_HYPERGUN_5,
    MZ2_SOLDIER_HYPERGUN_6,
    MZ2_SOLDIER_HYPERGUN_7,
    MZ2_SOLDIER_HYPERGUN_8,
    MZ2_GUARDIAN_BLASTER,
    MZ2_ARACHNID_RAIL1,
    MZ2_ARACHNID_RAIL2,
    MZ2_ARACHNID_RAIL_UP1,
    MZ2_ARACHNID_RAIL_UP2,
        
    MZ2_INFANTRY_MACHINEGUN_14, // run-attack
    MZ2_INFANTRY_MACHINEGUN_15, // run-attack
    MZ2_INFANTRY_MACHINEGUN_16, // run-attack
    MZ2_INFANTRY_MACHINEGUN_17, // run-attack
    MZ2_INFANTRY_MACHINEGUN_18, // run-attack
    MZ2_INFANTRY_MACHINEGUN_19, // run-attack
    MZ2_INFANTRY_MACHINEGUN_20, // run-attack
    MZ2_INFANTRY_MACHINEGUN_21, // run-attack
    
    MZ2_GUNCMDR_CHAINGUN_1, // straight
    MZ2_GUNCMDR_CHAINGUN_2, // dodging

    MZ2_GUNCMDR_GRENADE_MORTAR_1,
    MZ2_GUNCMDR_GRENADE_MORTAR_2,
    MZ2_GUNCMDR_GRENADE_MORTAR_3,
    MZ2_GUNCMDR_GRENADE_FRONT_1,
    MZ2_GUNCMDR_GRENADE_FRONT_2,
    MZ2_GUNCMDR_GRENADE_FRONT_3,
    MZ2_GUNCMDR_GRENADE_CROUCH_1,
    MZ2_GUNCMDR_GRENADE_CROUCH_2,
    MZ2_GUNCMDR_GRENADE_CROUCH_3,

    // prone
    MZ2_SOLDIER_BLASTER_9,
    MZ2_SOLDIER_SHOTGUN_9,
    MZ2_SOLDIER_MACHINEGUN_9,
    MZ2_SOLDIER_RIPPER_9,
    MZ2_SOLDIER_HYPERGUN_9,

    // alternate frontwards grenades
    MZ2_GUNNER_GRENADE2_1,
    MZ2_GUNNER_GRENADE2_2,
    MZ2_GUNNER_GRENADE2_3,
    MZ2_GUNNER_GRENADE2_4,

    MZ2_INFANTRY_MACHINEGUN_22,

    // supertonk
    MZ2_SUPERTANK_GRENADE_1,
    MZ2_SUPERTANK_GRENADE_2,

    // hover & daedalus other side
    MZ2_HOVER_BLASTER_2,
    MZ2_DAEDALUS_BLASTER_2,

    // medic (commander) sweeps
    MZ2_MEDIC_HYPERBLASTER1_1,
    MZ2_MEDIC_HYPERBLASTER1_2,
    MZ2_MEDIC_HYPERBLASTER1_3,
    MZ2_MEDIC_HYPERBLASTER1_4,
    MZ2_MEDIC_HYPERBLASTER1_5,
    MZ2_MEDIC_HYPERBLASTER1_6,
    MZ2_MEDIC_HYPERBLASTER1_7,
    MZ2_MEDIC_HYPERBLASTER1_8,
    MZ2_MEDIC_HYPERBLASTER1_9,
    MZ2_MEDIC_HYPERBLASTER1_10,
    MZ2_MEDIC_HYPERBLASTER1_11,
    MZ2_MEDIC_HYPERBLASTER1_12,

    MZ2_MEDIC_HYPERBLASTER2_1,
    MZ2_MEDIC_HYPERBLASTER2_2,
    MZ2_MEDIC_HYPERBLASTER2_3,
    MZ2_MEDIC_HYPERBLASTER2_4,
    MZ2_MEDIC_HYPERBLASTER2_5,
    MZ2_MEDIC_HYPERBLASTER2_6,
    MZ2_MEDIC_HYPERBLASTER2_7,
    MZ2_MEDIC_HYPERBLASTER2_8,
    MZ2_MEDIC_HYPERBLASTER2_9,
    MZ2_MEDIC_HYPERBLASTER2_10,
    MZ2_MEDIC_HYPERBLASTER2_11,
    MZ2_MEDIC_HYPERBLASTER2_12,

    // only used for compile time checks
    MZ2_LAST
};

// temp entity events
//
// Temp entity events are for things that happen
// at a location seperate from any existing entity.
// Temporary entity messages are explicitly constructed
// and broadcast.
enum temp_event_t : uint8_t
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
    TE_BOSSTPORT, // used as '22' in a map, so DON'T RENUMBER!!!
    TE_BFG_LASER,
    TE_GRAPPLE_CABLE,
    TE_WELDING_SPARKS,
    TE_GREENBLOOD,
    TE_BLUEHYPERBLASTER_DUMMY, // [Paril-KEX] leaving for compatibility, do not use; use TE_BLUEHYPERBLASTER
    TE_PLASMA_EXPLOSION,
    TE_TUNNEL_SPARKS,
    // ROGUE
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
    TE_FLECHETTE,
    // ROGUE

    // [Paril-KEX]
    TE_BLUEHYPERBLASTER,
    TE_BFG_ZAP,
    TE_BERSERK_SLAM,
    TE_GRAPPLE_CABLE_2,
    TE_POWER_SPLASH,
    TE_LIGHTNING_BEAM,
    TE_EXPLOSION1_NL,
    TE_EXPLOSION2_NL,
};

enum splash_color_t : uint8_t
{
    SPLASH_UNKNOWN = 0,
    SPLASH_SPARKS = 1,
    SPLASH_BLUE_WATER = 2,
    SPLASH_BROWN_WATER = 3,
    SPLASH_SLIME = 4,
    SPLASH_LAVA = 5,
    SPLASH_BLOOD = 6,

    // [Paril-KEX] N64 electric sparks that go zap
    SPLASH_ELECTRIC = 7
};

// sound channels
// channel 0 never willingly overrides
// other channels (1-7) always override a playing sound on that channel
enum soundchan_t : uint8_t
{
    CHAN_AUTO = 0,
    CHAN_WEAPON = 1,
    CHAN_VOICE = 2,
    CHAN_ITEM = 3,
    CHAN_BODY = 4,
    CHAN_AUX = 5,
    CHAN_FOOTSTEP = 6,
    CHAN_AUX3 = 7,

    // modifier flags
    CHAN_NO_PHS_ADD = bit_v<3>, // send to all clients, not just ones in PHS (ATTN 0 will also do this)
    CHAN_RELIABLE = bit_v<4>,   // send by reliable message, not datagram
    CHAN_FORCE_POS = bit_v<5>,  // always use position sent in packet
};

MAKE_ENUM_BITFLAGS(soundchan_t);

// sound attenuation values
constexpr float ATTN_LOOP_NONE = -1; // full volume the entire level, for loop only
constexpr float ATTN_NONE = 0; // full volume the entire level, for sounds only
constexpr float ATTN_NORM = 1;
constexpr float ATTN_IDLE = 2;
constexpr float ATTN_STATIC = 3; // diminish very rapidly with distance

// total stat count
constexpr size_t MAX_STATS = 64;

/*
ROGUE - VERSIONS
1234	08/13/1998		Activision
1235	08/14/1998		Id Software
1236	08/15/1998		Steve Tietze
1237	08/15/1998		Phil Dobranski
1238	08/15/1998		John Sheley
1239	08/17/1998		Barrett Alexander
1230	08/17/1998		Brandon Fish
1245	08/17/1998		Don MacAskill
1246	08/17/1998		David "Zoid" Kirsch
1247	08/17/1998		Manu Smith
1248	08/17/1998		Geoff Scully
1249	08/17/1998		Andy Van Fossen
1240	08/20/1998		Activision Build 2
1256	08/20/1998		Ranger Clan
1257	08/20/1998		Ensemble Studios
1258	08/21/1998		Robert Duffy
1259	08/21/1998		Stephen Seachord
1250	08/21/1998		Stephen Heaslip
1267	08/21/1998		Samir Sandesara
1268	08/21/1998		Oliver Wyman
1269	08/21/1998		Steven Marchegiano
1260	08/21/1998		Build #2 for Nihilistic
1278	08/21/1998		Build #2 for Ensemble
1279	08/26/1998		Build for Ron Solo - DEFUNCT
1270	08/26/1998		Build #3 for Activision
1289	08/26/1998		Build for Don MacAskill
1280	08/26/1998		Build for Robert Duffy
1290	08/26/1998		Build #2 for Rangers
1345	08/28/1998		Build #4 for Activision
2345	08/26/1998		Build for Zoid

9999	08/20/1998		Internal Use
*/

// ROGUE
/*
==========================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

==========================================================
*/

//=============================================
// INFO STRINGS
// 
// NB: the Q2 protocol does not dictate the type
// of strings being used, so it's kind of a crapshoot.
// Kex's protocol assumes info strings are always UTF8.
//=============================================

//
// key / value info strings
//
constexpr size_t MAX_INFO_KEY		= 64;
constexpr size_t MAX_INFO_VALUE		= 256;
constexpr size_t MAX_INFO_STRING	= 2048;

// CONFIG STRINGS

// bound by number of things we can fit in two stats
constexpr size_t MAX_WHEEL_ITEMS = 32;

// CS_WHEEL_xxx are special configstrings that
// map individual weapon and ammo ids to each other, separated by a pipe |
// the format for CS_WHEEL_WEAPONS is:
// <CS_ITEMS INDEX>|<CS_IMAGES INDEX>|<CS_WHEEL_AMMO INDEX>|<min ammo>|<on powerup wheel>|<sort id>|<warn quantity>|<droppable>
// if the weapon does not take ammo, the index will be -1
// the format for CS_WHEEL_AMMO is:
// <CS_ITEMS INDEX>|<CS_IMAGES INDEX>
// the indices here are not related to the IT_ or AMMO_
// indices, and are just as they appear in the configstrings.
// the format for CS_WHEEL_POWERUP is:
// <CS_ITEMS INDEX>|<CS_IMAGES INDEX>|<USE ON/OFF INSTEAD OF COUNT>|<SORT_ID>|<DROPPABLE>|<AMMO, IF APPLICABLE>

enum game_style_t : uint8_t
{
    GAME_STYLE_PVE,
    GAME_STYLE_FFA,
    GAME_STYLE_TDM
};

//
// config strings are a general means of communication from
// the server to all connected clients.
// Each config string can be at most CS_MAX_STRING_LENGTH characters.
//
enum
{
    CS_NAME,
    CS_CDTRACK,
    CS_SKY,
    CS_SKYAXIS, // %f %f %f format
    CS_SKYROTATE,
    CS_STATUSBAR, // display program string

    CS_AIRACCEL = 59, // air acceleration control
    CS_MAXCLIENTS,
    CS_MAPCHECKSUM, // for catching cheater maps

    CS_MODELS,
    CS_SOUNDS           = (CS_MODELS + MAX_MODELS),
    CS_IMAGES           = (CS_SOUNDS + MAX_SOUNDS),
    CS_LIGHTS           = (CS_IMAGES + MAX_IMAGES),
    CS_SHADOWLIGHTS     = (CS_LIGHTS + MAX_LIGHTSTYLES), // [Sam-KEX]
    CS_ITEMS            = (CS_SHADOWLIGHTS + MAX_SHADOW_LIGHTS),
    CS_PLAYERSKINS      = (CS_ITEMS + MAX_ITEMS),
    CS_GENERAL          = (CS_PLAYERSKINS + MAX_CLIENTS),
    CS_WHEEL_WEAPONS    = (CS_GENERAL + MAX_GENERAL), // [Paril-KEX] see MAX_WHEEL_ITEMS
    CS_WHEEL_AMMO       = (CS_WHEEL_WEAPONS + MAX_WHEEL_ITEMS), // [Paril-KEX] see MAX_WHEEL_ITEMS
    CS_WHEEL_POWERUPS   = (CS_WHEEL_AMMO + MAX_WHEEL_ITEMS), // [Paril-KEX] see MAX_WHEEL_ITEMS
    CS_CD_LOOP_COUNT    = (CS_WHEEL_POWERUPS + MAX_WHEEL_ITEMS), // [Paril-KEX] override default loop count
    CS_GAME_STYLE, // [Paril-KEX] see game_style_t
    MAX_CONFIGSTRINGS
};

static_assert(MAX_CONFIGSTRINGS <= 0x7FFF, "configstrings too big");

// [Sam-KEX] New define for max config string length
constexpr size_t CS_MAX_STRING_LENGTH = 96;
constexpr size_t CS_MAX_STRING_LENGTH_OLD = 64;

// certain configstrings are allowed to be larger
// than CS_MAX_STRING_LENGTH; this gets the absolute size
// for the given configstring at the specified id
// since vanilla didn't do a very good job of size checking
constexpr size_t CS_SIZE(int32_t in)
{
	if (in >= CS_STATUSBAR && in < CS_AIRACCEL)
		return CS_MAX_STRING_LENGTH * (CS_AIRACCEL - in);
	else if (in >= CS_GENERAL && in < CS_WHEEL_WEAPONS)
		return CS_MAX_STRING_LENGTH * (MAX_CONFIGSTRINGS - in);
	
	return CS_MAX_STRING_LENGTH;
}

constexpr size_t MAX_MODELS_OLD = 256, MAX_SOUNDS_OLD = 256, MAX_IMAGES_OLD = 256;

enum
{
    CS_NAME_OLD,
    CS_CDTRACK_OLD,
    CS_SKY_OLD,
    CS_SKYAXIS_OLD, // %f %f %f format
    CS_SKYROTATE_OLD,
    CS_STATUSBAR_OLD, // display program string

    CS_AIRACCEL_OLD = 29, // air acceleration control
    CS_MAXCLIENTS_OLD,
    CS_MAPCHECKSUM_OLD, // for catching cheater maps

    CS_MODELS_OLD,
    CS_SOUNDS_OLD = (CS_MODELS_OLD + MAX_MODELS_OLD),
    CS_IMAGES_OLD = (CS_SOUNDS_OLD + MAX_SOUNDS_OLD),
    CS_LIGHTS_OLD = (CS_IMAGES_OLD + MAX_IMAGES_OLD),
    CS_ITEMS_OLD = (CS_LIGHTS_OLD + MAX_LIGHTSTYLES),
    CS_PLAYERSKINS_OLD = (CS_ITEMS_OLD + MAX_ITEMS),
    CS_GENERAL_OLD = (CS_PLAYERSKINS_OLD + MAX_CLIENTS),
    MAX_CONFIGSTRINGS_OLD = (CS_GENERAL_OLD + MAX_GENERAL)
};

// remaps old configstring IDs to new ones
// for old DLL & demo support
struct configstring_remap_t
{
    // start position in the configstring list
    // to write into
    size_t  start;
    // max length to write into; [start+length-1] should always
    // be set to '\0'
    size_t  length;
};

constexpr configstring_remap_t CS_REMAP(int32_t id)
{
    // direct mapping
    if (id < CS_STATUSBAR_OLD)
        return { id * CS_MAX_STRING_LENGTH, CS_MAX_STRING_LENGTH };
    // statusbar needs a bit of special handling, since we have a different
    // max configstring length and these are just segments of a longer string
    else if (id < CS_AIRACCEL_OLD)
        return { (CS_STATUSBAR * CS_MAX_STRING_LENGTH) + ((id - CS_STATUSBAR_OLD) * CS_MAX_STRING_LENGTH_OLD), (CS_AIRACCEL - CS_STATUSBAR) * CS_MAX_STRING_LENGTH };
    // offset
    else if (id < CS_MODELS_OLD)
        return { (id + (CS_AIRACCEL - CS_AIRACCEL_OLD)) * CS_MAX_STRING_LENGTH, CS_MAX_STRING_LENGTH };
    else if (id < CS_SOUNDS_OLD)
        return { (id + (CS_MODELS - CS_MODELS_OLD)) * CS_MAX_STRING_LENGTH, CS_MAX_STRING_LENGTH };
    else if (id < CS_IMAGES_OLD)
        return { (id + (CS_SOUNDS - CS_SOUNDS_OLD)) * CS_MAX_STRING_LENGTH, CS_MAX_STRING_LENGTH };
    else if (id < CS_LIGHTS_OLD)
        return { (id + (CS_IMAGES - CS_IMAGES_OLD)) * CS_MAX_STRING_LENGTH, CS_MAX_STRING_LENGTH };
    else if (id < CS_ITEMS_OLD)
        return { (id + (CS_LIGHTS - CS_LIGHTS_OLD)) * CS_MAX_STRING_LENGTH, CS_MAX_STRING_LENGTH };
    else if (id < CS_PLAYERSKINS_OLD)
        return { (id + (CS_ITEMS - CS_ITEMS_OLD)) * CS_MAX_STRING_LENGTH, CS_MAX_STRING_LENGTH };
    else if (id < CS_GENERAL_OLD)
        return { (id + (CS_PLAYERSKINS - CS_PLAYERSKINS_OLD)) * CS_MAX_STRING_LENGTH, CS_MAX_STRING_LENGTH };

    // general also needs some special handling because it's both
    // offset *and* allowed to overflow
    return { (id + (CS_GENERAL - CS_GENERAL_OLD)) * CS_MAX_STRING_LENGTH_OLD, (MAX_CONFIGSTRINGS - CS_GENERAL) * CS_MAX_STRING_LENGTH };
}

static_assert(CS_REMAP(CS_MODELS_OLD).start == (CS_MODELS * 96), "check CS_REMAP");
static_assert(CS_REMAP(CS_SOUNDS_OLD).start == (CS_SOUNDS * 96), "check CS_REMAP");
static_assert(CS_REMAP(CS_IMAGES_OLD).start == (CS_IMAGES * 96), "check CS_REMAP");
static_assert(CS_REMAP(CS_LIGHTS_OLD).start == (CS_LIGHTS * 96), "check CS_REMAP");
static_assert(CS_REMAP(CS_PLAYERSKINS_OLD).start == (CS_PLAYERSKINS * 96), "check CS_REMAP");
static_assert(CS_REMAP(CS_ITEMS_OLD).start == (CS_ITEMS * 96), "check CS_REMAP");
static_assert(CS_REMAP(CS_GENERAL_OLD).start == (CS_GENERAL * 64), "check CS_REMAP");
static_assert(CS_REMAP(CS_AIRACCEL_OLD).start == (CS_AIRACCEL * 96), "check CS_REMAP");

//==============================================

// entity_state_t->event values
// ertity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.
// All muzzle flashes really should be converted to events...
enum entity_event_t : uint8_t
{
    EV_NONE,
    EV_ITEM_RESPAWN,
    EV_FOOTSTEP,
    EV_FALLSHORT,
    EV_FALL,
    EV_FALLFAR,
    EV_PLAYER_TELEPORT,
    EV_OTHER_TELEPORT,

    // [Paril-KEX]
    EV_OTHER_FOOTSTEP,
    EV_LADDER_STEP,
};

// [Paril-KEX] player s.skinnum's encode additional data
union player_skinnum_t
{
    int32_t         skinnum;
    struct {
        uint8_t     client_num; // client index
        uint8_t     vwep_index; // vwep index
        int8_t      viewheight; // viewheight
        uint8_t     team_index : 4; // team #; note that teams are 1-indexed here, with 0 meaning no team
                                    // (spectators in CTF would be 0, for instance)
        uint8_t     poi_icon : 4;   // poi icon; 0 default friendly, 1 dead, others unused
    };
};

// entity_state_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
struct entity_state_t
{
    uint32_t       number; // edict index

    gvec3_t        origin;
    gvec3_t        angles;
    gvec3_t        old_origin; // for lerping
    int32_t        modelindex;
    int32_t        modelindex2, modelindex3, modelindex4; // weapons, CTF flags, etc
    int32_t        frame;
    int32_t        skinnum;
    effects_t      effects; // PGM - we're filling it, so it needs to be unsigned
    renderfx_t     renderfx;
    uint32_t       solid;   // for client side prediction
    int32_t        sound;   // for looping sounds, to guarantee shutoff
    entity_event_t event;   // impulse events -- muzzle flashes, footsteps, etc
                            // events only go out for a single frame, they
                            // are automatically cleared each frame
    float          alpha;   // [Paril-KEX] alpha scalar; 0 is a "default" value, which will respect other
                            // settings (default 1.0 for most things, EF_TRANSLUCENT will default this
                            // to 0.3, etc)
    float          scale;   // [Paril-KEX] model scale scalar; 0 is a "default" value, like with alpha.
    uint8_t        instance_bits; // [Paril-KEX] players that *can't* see this entity will have a bit of 1. handled by
                                  // the server, do not set directly.
    // [Paril-KEX] allow specifying volume/attn for looping noises; note that
    // zero will be defaults (1.0 and 3.0 respectively); -1 attenuation is used
    // for "none" (similar to target_speaker) for no phs/pvs looping noises
    float          loop_volume;
    float          loop_attenuation;
    // [Paril-KEX] for proper client-side owner collision skipping
    int32_t        owner;
    // [Paril-KEX] for custom interpolation stuff
    int32_t        old_frame;
};

//==============================================

// player_state_t is the information needed in addition to pmove_state_t
// to rendered a view.  There will only be 10 player_state_t sent each second,
// but the number of pmove_state_t changes will be relative to client
// frame rates
struct player_state_t
{
    pmove_state_t pmove; // for prediction

    // these fields do not need to be communicated bit-precise

    gvec3_t viewangles;  // for fixed views
    gvec3_t viewoffset;  // add to pmovestate->origin
    gvec3_t kick_angles; // add to view direction to get render angles
                         // set by weapon kicks, pain effects, etc

    gvec3_t gunangles;
    gvec3_t gunoffset;
    int32_t gunindex;
    int32_t gunskin; // [Paril-KEX] gun skin #
    int32_t gunframe;
    int32_t gunrate; // [Paril-KEX] tickrate of gun animations; 0 and 10 are equivalent

    std::array<float, 4> screen_blend; // rgba full screen effect
    std::array<float, 4> damage_blend; // [Paril-KEX] rgba damage blend effect

    float fov; // horizontal field of view

    refdef_flags_t rdflags; // refdef flags

    std::array<int16_t, MAX_STATS> stats; // fast status bar updates

    uint8_t team_id; // team identifier
};

// protocol bytes that can be directly added to messages
enum server_command_t : uint8_t
{
    svc_bad,

    svc_muzzleflash,
    svc_muzzleflash2,
    svc_temp_entity,
    svc_layout,
    svc_inventory,

    svc_nop,
    svc_disconnect,
    svc_reconnect,
    svc_sound,                  // <see code>
    svc_print,                  // [byte] id [string] null terminated string
    svc_stufftext,              // [string] stuffed into client's console buffer, should be \n terminated
    svc_serverdata,             // [long] protocol ...
    svc_configstring,           // [short] [string]
    svc_spawnbaseline,
    svc_centerprint,            // [string] to put in center of the screen
    svc_download,               // [short] size [size bytes]
    svc_playerinfo,             // variable
    svc_packetentities,         // [...]
    svc_deltapacketentities,    // [...]
    svc_frame,

    svc_splitclient,

    svc_configblast,            // [Kex] A compressed version of svc_configstring
    svc_spawnbaselineblast,     // [Kex] A compressed version of svc_spawnbaseline
    svc_level_restart,          // [Paril-KEX] level was soft-rebooted
    svc_damage,                 // [Paril-KEX] damage indicators
    svc_locprint,               // [Kex] localized + libfmt version of print
    svc_fog,                    // [Paril-KEX] change current fog values
    svc_waitingforplayers,      // [Kex-Edward] Inform clients that the server is waiting for remaining players
    svc_bot_chat,               // [Kex] bot specific chat
    svc_poi,                    // [Paril-KEX] point of interest
    svc_help_path,              // [Paril-KEX] help path
    svc_muzzleflash3,           // [Paril-KEX] muzzleflashes, but ushort id
    svc_achievement,            // [Paril-KEX]

    svc_last // only for checks
};

enum svc_poi_flags
{
    POI_FLAG_NONE = 0,
    POI_FLAG_HIDE_ON_AIM = 1, // hide the POI if we get close to it with our aim
};

// data for svc_fog
struct svc_fog_data_t
{
    enum bits_t : uint16_t
    {
        // global fog
        BIT_DENSITY     = bit_v<0>,
        BIT_R           = bit_v<1>,
        BIT_G           = bit_v<2>,
        BIT_B           = bit_v<3>,
        BIT_TIME        = bit_v<4>, // if set, the transition takes place over N milliseconds

        // height fog
        BIT_HEIGHTFOG_FALLOFF   = bit_v<5>,
        BIT_HEIGHTFOG_DENSITY   = bit_v<6>,
        BIT_MORE_BITS           = bit_v<7>, // read additional bit
        BIT_HEIGHTFOG_START_R   = bit_v<8>,
        BIT_HEIGHTFOG_START_G   = bit_v<9>,
        BIT_HEIGHTFOG_START_B   = bit_v<10>,
        BIT_HEIGHTFOG_START_DIST= bit_v<11>,
        BIT_HEIGHTFOG_END_R     = bit_v<12>,
        BIT_HEIGHTFOG_END_G     = bit_v<13>,
        BIT_HEIGHTFOG_END_B     = bit_v<14>,
        BIT_HEIGHTFOG_END_DIST  = bit_v<15>
    };

    bits_t      bits;
    float       density; // bits & BIT_DENSITY
    uint8_t     skyfactor; // bits & BIT_DENSITY
    uint8_t     red; // bits & BIT_R
    uint8_t     green; // bits & BIT_G
    uint8_t     blue; // bits & BIT_B
    uint16_t    time; // bits & BIT_TIME
    
    float       hf_falloff; // bits & BIT_HEIGHTFOG_FALLOFF
    float       hf_density; // bits & BIT_HEIGHTFOG_DENSITY
    uint8_t     hf_start_r; // bits & (BIT_MORE_BITS | BIT_HEIGHTFOG_START_R)
    uint8_t     hf_start_g; // bits & (BIT_MORE_BITS | BIT_HEIGHTFOG_START_G)
    uint8_t     hf_start_b; // bits & (BIT_MORE_BITS | BIT_HEIGHTFOG_START_B)
    int32_t     hf_start_dist; // bits & (BIT_MORE_BITS | BIT_HEIGHTFOG_START_DIST)
    uint8_t     hf_end_r; // bits & (BIT_MORE_BITS | BIT_HEIGHTFOG_END_R)
    uint8_t     hf_end_g; // bits & (BIT_MORE_BITS | BIT_HEIGHTFOG_END_G)
    uint8_t     hf_end_b; // bits & (BIT_MORE_BITS | BIT_HEIGHTFOG_END_B)
    int32_t     hf_end_dist; // bits & (BIT_MORE_BITS | BIT_HEIGHTFOG_END_DIST)
};

MAKE_ENUM_BITFLAGS(svc_fog_data_t::bits_t);
    
// bit masks
static constexpr svc_fog_data_t::bits_t BITS_GLOBAL_FOG = (svc_fog_data_t::BIT_DENSITY | svc_fog_data_t::BIT_R | svc_fog_data_t::BIT_G | svc_fog_data_t::BIT_B);
static constexpr svc_fog_data_t::bits_t BITS_HEIGHTFOG = (svc_fog_data_t::BIT_HEIGHTFOG_FALLOFF | svc_fog_data_t::BIT_HEIGHTFOG_DENSITY | svc_fog_data_t::BIT_HEIGHTFOG_START_R | svc_fog_data_t::BIT_HEIGHTFOG_START_G |
                                            svc_fog_data_t::BIT_HEIGHTFOG_START_B | svc_fog_data_t::BIT_HEIGHTFOG_START_DIST | svc_fog_data_t::BIT_HEIGHTFOG_END_R | svc_fog_data_t::BIT_HEIGHTFOG_END_G |
                                            svc_fog_data_t::BIT_HEIGHTFOG_END_B | svc_fog_data_t::BIT_HEIGHTFOG_END_DIST);

// edict->svflags
enum svflags_t : uint32_t
{
    SVF_NONE        = 0,          // no serverflags
    SVF_NOCLIENT    = bit_v<0>,   // don't send entity to clients, even if it has effects
    SVF_DEADMONSTER = bit_v<1>,   // treat as CONTENTS_DEADMONSTER for collision
    SVF_MONSTER     = bit_v<2>,   // treat as CONTENTS_MONSTER for collision
    SVF_PLAYER      = bit_v<3>,   // [Paril-KEX] treat as CONTENTS_PLAYER for collision
    SVF_BOT         = bit_v<4>,   // entity is controlled by a bot AI.
    SVF_NOBOTS      = bit_v<5>,   // don't allow bots to use/interact with entity
    SVF_RESPAWNING  = bit_v<6>,   // entity will respawn on it's next think.
    SVF_PROJECTILE  = bit_v<7>,   // treat as CONTENTS_PROJECTILE for collision
    SVF_INSTANCED   = bit_v<8>,   // entity has different visibility per player
    SVF_DOOR        = bit_v<9>,   // entity is a door of some kind
    SVF_NOCULL      = bit_v<10>,  // always send, even if we normally wouldn't
    SVF_HULL        = bit_v<11>   // always use hull when appropriate (triggers, etc; for gi.clip)
};
MAKE_ENUM_BITFLAGS(svflags_t);

// edict->solid values
enum solid_t : uint8_t
{
    SOLID_NOT,     // no interaction with other objects
    SOLID_TRIGGER, // only touch when inside, after moving
    SOLID_BBOX,    // touch on edge
    SOLID_BSP      // bsp clip, touch on edge
};

// bitflags for STAT_LAYOUTS
enum layout_flags_t : int16_t
{
	LAYOUTS_LAYOUT		      = bit_v<0>, // svc_layout is active; escape remapped to putaway
	LAYOUTS_INVENTORY	      = bit_v<1>, // inventory is active; escape remapped to putaway
	LAYOUTS_HIDE_HUD	      = bit_v<2>, // hide entire hud, for cameras, etc
	LAYOUTS_INTERMISSION      = bit_v<3>, // intermission is being drawn; collapse splitscreen into 1 view
	LAYOUTS_HELP              = bit_v<4>, // help is active; escape remapped to putaway
    LAYOUTS_HIDE_CROSSHAIR	  = bit_v<5> // hide crosshair only
};
MAKE_ENUM_BITFLAGS(layout_flags_t);

enum GoalReturnCode {
	Error = 0,
	Started,
	InProgress,
    Finished
};

enum gesture_type {
	GESTURE_NONE = -1,
	GESTURE_FLIP_OFF,
	GESTURE_SALUTE,
	GESTURE_TAUNT,
	GESTURE_WAVE,
	GESTURE_POINT,
    GESTURE_POINT_NO_PING,
    GESTURE_MAX
};

enum class PathReturnCode {
	ReachedGoal = 0,        // we're at our destination
	ReachedPathEnd,         // we're as close to the goal as we can get with a path
	TraversalPending,       // the upcoming path segment is a traversal
    RawPathFound,           // user wanted ( and got ) just a raw path ( no processing )
	InProgress,             // pathing in progress
	StartPathErrors,        // any code after this one indicates an error of some kind.
	InvalidStart,           // start position is invalid.
	InvalidGoal,            // goal position is invalid.
	NoNavAvailable,         // no nav file available for this map.
	NoStartNode,            // can't find a nav node near the start position
	NoGoalNode,             // can't find a nav node near the goal position
	NoPathFound,            // can't find a path from the start to the goal
    MissingWalkOrSwimFlag   // MUST have at least Walk or Water path flags set!
};

enum class PathLinkType {
	Walk,               // can walk between the path points
	WalkOffLedge,       // will walk off a ledge going between path points
	LongJump,           // will need to perform a long jump between path points
	BarrierJump,        // will need to jump over a low barrier between path points
	Elevator            // will need to use an elevator between path points
};

enum PathFlags : uint32_t {
	All             = static_cast<uint32_t>( -1 ),
	Water           = bit_v<0>,  // swim to your goal ( useful for fish/gekk/etc. )
	Walk            = bit_v<1>,  // walk to your goal
	WalkOffLedge    = bit_v<2>,  // allow walking over ledges
	LongJump        = bit_v<3>,  // allow jumping over gaps
	BarrierJump     = bit_v<4>,  // allow jumping over low barriers
	Elevator        = bit_v<5>   // allow using elevators
};
MAKE_ENUM_BITFLAGS(PathFlags);

struct PathRequest {
    gvec3_t     start = { 0.0f, 0.0f, 0.0f };
    gvec3_t     goal = { 0.0f, 0.0f, 0.0f };
	PathFlags   pathFlags = PathFlags::Walk;
	float       moveDist = 0.0f;

    struct DebugSettings {
        float   drawTime = 0.0f; // if > 0, how long ( in seconds ) to draw path in world
    } debugging;

    struct NodeSettings {
        bool    ignoreNodeFlags = false; // true = ignore node flags when considering nodes
        float   minHeight = 0.0f; // 0 <= use default values
        float   maxHeight = 0.0f; // 0 <= use default values
        float   radius = 0.0f;    // 0 <= use default values
    } nodeSearch;

	struct TraversalSettings {
		float dropHeight = 0.0f;    // 0 = don't drop down
		float jumpHeight = 0.0f;    // 0 = don't jump up
	} traversals;

	struct PathArray {
        mutable gvec3_t * array = nullptr;  // array to store raw path points
		int64_t           count = 0;        // number of elements in array
	} pathPoints;
};

struct PathInfo {
    int32_t         numPathPoints = 0;
    float           pathDistSqr = 0.0f;
	gvec3_t         firstMovePoint = { 0.0f, 0.0f, 0.0f };
	gvec3_t         secondMovePoint = { 0.0f, 0.0f, 0.0f };
	PathLinkType    pathLinkType = PathLinkType::Walk;
    PathReturnCode  returnCode = PathReturnCode::StartPathErrors;
};

//===============================================================

constexpr int32_t MODELINDEX_WORLD = 1;    // special index for world
constexpr int32_t MODELINDEX_PLAYER = MAX_MODELS_OLD - 1; // special index for player models

// short stubs only used by the engine; the game DLL's version
// must be compatible with this.
#ifndef GAME_INCLUDE
struct gclient_t
#else
struct gclient_shared_t
#endif
{
    player_state_t ps; // communicated by server to clients
    int32_t        ping;
    // the game dll can add anything it wants after
    // this point in the structure
};

static constexpr int32_t    Team_None = 0;
static constexpr int32_t    Item_UnknownRespawnTime = INT_MAX;
static constexpr int32_t    Item_Invalid = -1;
static constexpr int32_t    Item_Null = 0;

enum sv_ent_flags_t : uint64_t {
    SVFL_NONE               = 0, // no flags
    SVFL_ONGROUND           = bit_v< 0 >,
    SVFL_HAS_DMG_BOOST      = bit_v< 1 >,
    SVFL_HAS_PROTECTION     = bit_v< 2 >,
    SVFL_HAS_INVISIBILITY   = bit_v< 3 >,
    SVFL_IS_JUMPING         = bit_v< 4 >,
    SVFL_IS_CROUCHING       = bit_v< 5 >,
    SVFL_IS_ITEM            = bit_v< 6 >,
    SVFL_IS_OBJECTIVE       = bit_v< 7 >,
    SVFL_HAS_TELEPORTED     = bit_v< 8 >,
    SVFL_TAKES_DAMAGE       = bit_v< 9 >,
    SVFL_IS_HIDDEN          = bit_v< 10 >,
    SVFL_IS_NOCLIP          = bit_v< 11 >,
    SVFL_IN_WATER           = bit_v< 12 >,
    SVFL_NO_TARGET          = bit_v< 13 >,
    SVFL_GOD_MODE           = bit_v< 14 >,
    SVFL_IS_FLIPPING_OFF    = bit_v< 15 >,
    SVFL_IS_SALUTING        = bit_v< 16 >,
    SVFL_IS_TAUNTING        = bit_v< 17 >,
    SVFL_IS_WAVING          = bit_v< 18 >,
    SVFL_IS_POINTING        = bit_v< 19 >,
    SVFL_ON_LADDER          = bit_v< 20 >,
    SVFL_MOVESTATE_TOP      = bit_v< 21 >,
    SVFL_MOVESTATE_BOTTOM   = bit_v< 22 >,
    SVFL_MOVESTATE_MOVING   = bit_v< 23 >,
    SVFL_IS_LOCKED_DOOR     = bit_v< 24 >,
    SVFL_CAN_GESTURE        = bit_v< 25 >,
    SVFL_WAS_TELEFRAGGED    = bit_v< 26 >,
    SVFL_TRAP_DANGER        = bit_v< 27 >,
    SVFL_ACTIVE             = bit_v< 28 >,
    SVFL_IS_SPECTATOR       = bit_v< 29 >,
    SVFL_IN_TEAM            = bit_v< 30 >
};
MAKE_ENUM_BITFLAGS( sv_ent_flags_t );

static constexpr int Max_Armor_Types = 3;

struct armorInfo_t {
    int32_t     item_id = Item_Null;
    int32_t     max_count = 0;
};

// Used by AI/Tools on the engine side...
struct sv_entity_t {
    bool                        init;
    sv_ent_flags_t              ent_flags;
    button_t                    buttons;
    uint32_t	                spawnflags;
    int32_t                     item_id;
    int32_t                     armor_type;
    int32_t                     armor_value;
    int32_t                     health;
    int32_t                     max_health;
    int32_t                     starting_health;
    int32_t                     weapon;
    int32_t                     team;
    int32_t                     lobby_usernum;
    int32_t                     respawntime;
    int32_t                     viewheight;
    int32_t                     last_attackertime;
    water_level_t               waterlevel;
    gvec3_t                     viewangles;
    gvec3_t                     viewforward;
    gvec3_t                     velocity;
    gvec3_t                     start_origin;
    gvec3_t                     end_origin;
    edict_t *                   enemy;
    edict_t *                   ground_entity;
    const char *                classname;
    const char *                targetname;
    char                        netname[ MAX_NETNAME ];
    int32_t                     inventory[ MAX_ITEMS ] = { 0 };
    armorInfo_t                 armor_info[ Max_Armor_Types ];
};

#ifndef GAME_INCLUDE
struct edict_t
#else
struct edict_shared_t
#endif
{
    entity_state_t s;
    gclient_t     *client; // nullptr if not a player
                           // the server expects the first part
                           // of gclient_t to be a player_state_t
                           // but the rest of it is opaque

    sv_entity_t sv;        // read only info about this entity for the server

	bool     inuse;

	// world linkage data
	bool     linked;
	int32_t	 linkcount;
	int32_t  areanum, areanum2;

	svflags_t  svflags;
	vec3_t	   mins, maxs;
	vec3_t	   absmin, absmax, size;
	solid_t	   solid;
	contents_t clipmask;
	edict_t	   *owner;
};

#define CHECK_INTEGRITY(from_type, to_type, member)                           \
    static_assert(offsetof(from_type, member) == offsetof(to_type, member) && \
                      sizeof(from_type::member) == sizeof(to_type::member),   \
                  "structure malformed; not compatible with server: check member \"" #member "\"")

#define CHECK_GCLIENT_INTEGRITY                       \
    CHECK_INTEGRITY(gclient_t, gclient_shared_t, ps); \
    CHECK_INTEGRITY(gclient_t, gclient_shared_t, ping)

#define CHECK_EDICT_INTEGRITY                               \
    CHECK_INTEGRITY(edict_t, edict_shared_t, s);            \
    CHECK_INTEGRITY(edict_t, edict_shared_t, client);       \
    CHECK_INTEGRITY(edict_t, edict_shared_t, sv);           \
    CHECK_INTEGRITY(edict_t, edict_shared_t, inuse);        \
    CHECK_INTEGRITY(edict_t, edict_shared_t, linked);       \
    CHECK_INTEGRITY(edict_t, edict_shared_t, linkcount);    \
    CHECK_INTEGRITY(edict_t, edict_shared_t, areanum);      \
    CHECK_INTEGRITY(edict_t, edict_shared_t, areanum2);     \
    CHECK_INTEGRITY(edict_t, edict_shared_t, svflags);      \
    CHECK_INTEGRITY(edict_t, edict_shared_t, mins);         \
    CHECK_INTEGRITY(edict_t, edict_shared_t, maxs);         \
    CHECK_INTEGRITY(edict_t, edict_shared_t, absmin);       \
    CHECK_INTEGRITY(edict_t, edict_shared_t, absmax);       \
    CHECK_INTEGRITY(edict_t, edict_shared_t, size);         \
    CHECK_INTEGRITY(edict_t, edict_shared_t, solid);        \
    CHECK_INTEGRITY(edict_t, edict_shared_t, clipmask);     \
    CHECK_INTEGRITY(edict_t, edict_shared_t, owner)

//===============================================================

// file system stuff
using fs_handle_t = uint64_t;

enum fs_search_flags_t
{
    FS_SEARCH_NONE          = 0,

    // flags for individual file filtering; note that if none
    // of these are set, they will all apply.
    FS_SEARCH_FOR_DIRECTORIES   = bit_v<0>, // only get directories
    FS_SEARCH_FOR_FILES         = bit_v<1> // only get files
};

MAKE_ENUM_BITFLAGS(fs_search_flags_t);

enum class BoxEdictsResult_t
{
    Keep, // keep the given entity in the result and keep looping
    Skip, // skip the given entity

    End = 64, // stop searching any further

    Flags = End
};

MAKE_ENUM_BITFLAGS(BoxEdictsResult_t);

using BoxEdictsFilter_t = BoxEdictsResult_t (*)(edict_t *, void *);

//
// functions provided by the main engine
//
struct game_import_t
{
    uint32_t    tick_rate;
    float       frame_time_s;
    uint32_t    frame_time_ms;

    // broadcast to all clients
    void (*Broadcast_Print)(print_type_t printlevel, const char *message);
    
    // print to appropriate places (console, log file, etc)
    void (*Com_Print)(const char *msg);

    // print directly to a single client (or nullptr for server console)
    void (*Client_Print)(edict_t *ent, print_type_t printlevel, const char *message);

    // center-print to player (legacy function)
    void (*Center_Print)(edict_t *ent, const char *message);

    void (*sound)(edict_t *ent, soundchan_t channel, int soundindex, float volume, float attenuation, float timeofs);
    void (*positioned_sound)(gvec3_cref_t origin, edict_t *ent, soundchan_t channel, int soundindex, float volume, float attenuation, float timeofs);
    // [Paril-KEX] like sound, but only send to the player indicated by the parameter;
    // this is mainly to handle split screen properly
    void (*local_sound)(edict_t *target, gvec3_cptr_t origin, edict_t *ent, soundchan_t channel, int soundindex, float volume, float attenuation, float timeofs, uint32_t dupe_key);

    // config strings hold all the index strings, the lightstyles,
    // and misc data like the sky definition and cdtrack.
    // All of the current configstrings are sent to clients when
    // they connect, and changes are sent to all connected clients.
    void (*configstring)(int num, const char *string);
    const char *(*get_configstring)(int num);

    void (*Com_Error)(const char *message);

    // the *index functions create configstrings and some internal server state
    int (*modelindex)(const char *name);
    int (*soundindex)(const char *name);
    // [Paril-KEX] imageindex can precache both pics for the HUD and
    // textures used for RF_CUSTOMSKIN; to register an image as a texture,
    // the path must be relative to the mod dir and end in an extension
    // ie models/my_model/skin.tga
    int (*imageindex)(const char *name);

    void (*setmodel)(edict_t *ent, const char *name);

    // collision detection
    trace_t (*trace)(gvec3_cref_t start, gvec3_cptr_t mins, gvec3_cptr_t maxs, gvec3_cref_t end, const edict_t *passent, contents_t contentmask);
    // [Paril-KEX] clip the box against the specified entity
    trace_t (*clip)(edict_t *entity, gvec3_cref_t start, gvec3_cptr_t mins, gvec3_cptr_t maxs, gvec3_cref_t end, contents_t contentmask);
    contents_t (*pointcontents)(gvec3_cref_t point);
	bool (*inPVS)(gvec3_cref_t p1, gvec3_cref_t p2, bool portals);
	bool (*inPHS)(gvec3_cref_t p1, gvec3_cref_t p2, bool portals);
    void (*SetAreaPortalState)(int portalnum, bool open);
    bool (*AreasConnected)(int area1, int area2);

    // an entity will never be sent to a client or used for collision
    // if it is not passed to linkentity.  If the size, position, or
    // solidity changes, it must be relinked.
    void (*linkentity)(edict_t *ent);
    void (*unlinkentity)(edict_t *ent); // call before removing an interactive edict

    // return a list of entities that touch the input absmin/absmax.
    // if maxcount is 0, it will return a count but not attempt to fill "list".
    // if maxcount > 0, once it reaches maxcount, it will keep going but not fill
    // any more of list (the return count will cap at maxcount).
    // the filter function can remove unnecessary entities from the final list; it is illegal
    // to modify world links in this callback.
    size_t (*BoxEdicts)(gvec3_cref_t mins, gvec3_cref_t maxs, edict_t **list, size_t maxcount, solidity_area_t areatype, BoxEdictsFilter_t filter, void *filter_data);

    // network messaging
    void (*multicast)(gvec3_cref_t origin, multicast_t to, bool reliable);
    // [Paril-KEX] `dupe_key` is a key unique to a group of calls to unicast
    // that will prevent sending the message on this frame with the same key
    // to the same player (for splitscreen players).
    void (*unicast)(edict_t *ent, bool reliable, uint32_t dupe_key);

    void (*WriteChar)(int c);
    void (*WriteByte)(int c);
    void (*WriteShort)(int c);
    void (*WriteLong)(int c);
    void (*WriteFloat)(float f);
    void (*WriteString)(const char *s);
    void (*WritePosition)(gvec3_cref_t pos);
    void (*WriteDir)(gvec3_cref_t pos);	  // single byte encoded, very coarse
    void (*WriteAngle)(float f); // legacy 8-bit angle
    void (*WriteEntity)(const edict_t *e);

    // managed memory allocation
    void *(*TagMalloc)(size_t size, int tag);
    void (*TagFree)(void *block);
    void (*FreeTags)(int tag);

    // console variable interaction
	cvar_t *(*cvar)(const char *var_name, const char *value, cvar_flags_t flags);
    cvar_t *(*cvar_set)(const char *var_name, const char *value);
    cvar_t *(*cvar_forceset)(const char *var_name, const char *value);

    // ClientCommand and ServerCommand parameter access
    int (*argc)();
	const char *(*argv)(int n);
	const char *(*args)(); // concatenation of all argv >= 1

    // add commands to the server console as if they were typed in
    // for map changing, etc
    void (*AddCommandString)(const char *text);

    void (*DebugGraph)(float value, int color);

    // Fetch named extension from engine.
    void *(*GetExtension)(const char *name);

    // === [KEX] Additional APIs ===

    // bots
    void (*Bot_RegisterEdict)(const edict_t * edict);
    void (*Bot_UnRegisterEdict)(const edict_t * edict);
    GoalReturnCode (*Bot_MoveToPoint)(const edict_t * bot, gvec3_cref_t point, const float moveTolerance);
    GoalReturnCode (*Bot_FollowActor)(const edict_t * bot, const edict_t * actor);

    // pathfinding - returns true if a path was found
    bool (*GetPathToGoal)(const PathRequest & request, PathInfo & info);

    // localization
    void (*Loc_Print)(edict_t* ent, print_type_t level, const char* base, const char** args, size_t num_args);

    // drawing
    void (*Draw_Line)(gvec3_cref_t start, gvec3_cref_t end, const rgba_t &color, const float lifeTime, const bool depthTest);
    void (*Draw_Point)(gvec3_cref_t point, const float size, const rgba_t &color, const float lifeTime, const bool depthTest);
    void (*Draw_Circle)(gvec3_cref_t origin, const float radius, const rgba_t &color, const float lifeTime, const bool depthTest);
    void (*Draw_Bounds)(gvec3_cref_t mins, gvec3_cref_t maxs, const rgba_t &color, const float lifeTime, const bool depthTest);
    void (*Draw_Sphere)(gvec3_cref_t origin, const float radius, const rgba_t &color, const float lifeTime, const bool depthTest);
    void (*Draw_OrientedWorldText)(gvec3_cref_t origin, const char * text, const rgba_t &color, const float size, const float lifeTime, const bool depthTest);
    void (*Draw_StaticWorldText)(gvec3_cref_t origin, gvec3_cref_t angles, const char * text, const rgba_t & color, const float size, const float lifeTime, const bool depthTest);
    void (*Draw_Cylinder)(gvec3_cref_t origin, const float halfHeight, const float radius, const rgba_t &color, const float lifeTime, const bool depthTest);
    void (*Draw_Ray)(gvec3_cref_t origin, gvec3_cref_t direction, const float length, const float size, const rgba_t &color, const float lifeTime, const bool depthTest);
    void (*Draw_Arrow)(gvec3_cref_t start, gvec3_cref_t end, const float size, const rgba_t & lineColor, const rgba_t & arrowColor, const float lifeTime, const bool depthTest);

    // scoreboard
    void (*ReportMatchDetails_Multicast)(bool is_end);

    // get server frame #
    uint32_t (*ServerFrame)();

    // misc utils
    void (*SendToClipBoard)(const char * text);

    // info string stuff
    size_t (*Info_ValueForKey) (const char *s, const char *key, char *buffer, size_t buffer_len);
    bool (*Info_RemoveKey) (char *s, const char *key);
    bool (*Info_SetValueForKey) (char *s, const char *key, const char *value);
};

enum class shadow_light_type_t
{
    point,
    cone
};

struct shadow_light_data_t
{
    shadow_light_type_t lighttype;
	float		radius;
	int			resolution;
	float		intensity = 1;
	float		fade_start;
	float		fade_end;
	int			lightstyle = -1;
	float		coneangle = 45;
    vec3_t      conedirection;
};

enum server_flags_t
{
    SERVER_FLAGS_NONE           = 0,
    SERVER_FLAG_SLOW_TIME       = bit_v<0>,
    SERVER_FLAG_INTERMISSION    = bit_v<1>,
    SERVER_FLAG_LOADING         = bit_v<2>
};

MAKE_ENUM_BITFLAGS(server_flags_t);

//
// functions exported by the game subsystem
//
struct game_export_t
{
    int apiversion;

    // the init function will only be called when a game starts,
    // not each time a level is loaded.  Persistant data for clients
    // and the server can be allocated in init
    void (*PreInit)(); // [Paril-KEX] called before InitGame, to potentially change maxclients
    void (*Init)();
    void (*Shutdown)();

    // each new level entered will cause a call to SpawnEntities
    void (*SpawnEntities)(const char *mapname, const char *entstring, const char *spawnpoint);

    // Read/Write Game is for storing persistant cross level information
    // about the world state and the clients.
    // WriteGame is called every time a level is exited.
    // ReadGame is called on a loadgame.

    // returns pointer to tagmalloc'd allocated string.
    // tagfree after use
    char *(*WriteGameJson)(bool autosave, size_t *out_size);
    void (*ReadGameJson)(const char *json);

    // ReadLevel is called after the default map information has been
    // loaded with SpawnEntities
    // returns pointer to tagmalloc'd allocated string.
    // tagfree after use
    char *(*WriteLevelJson)(bool transition, size_t *out_size);
    void (*ReadLevelJson)(const char *json);

    // [Paril-KEX] game can tell the server whether a save is allowed
    // currently or not.
    bool (*CanSave)();

    // [Paril-KEX] choose a free gclient_t slot for the given social ID; for
    // coop slot re-use. Return nullptr if none is available. You can not
    // return a slot that is currently in use by another client; that must
    // throw a fatal error.
    edict_t *(*ClientChooseSlot) (const char *userinfo, const char *social_id, bool isBot, edict_t **ignore, size_t num_ignore, bool cinematic);
    bool (*ClientConnect)(edict_t *ent, char *userinfo, const char *social_id, bool isBot);
    void (*ClientBegin)(edict_t *ent);
    void (*ClientUserinfoChanged)(edict_t *ent, const char *userinfo);
    void (*ClientDisconnect)(edict_t *ent);
    void (*ClientCommand)(edict_t *ent);
    void (*ClientThink)(edict_t *ent, usercmd_t *cmd);

    void (*RunFrame)(bool main_loop);
    // [Paril-KEX] allow the game DLL to clear per-frame stuff
    void (*PrepFrame)();

    // ServerCommand will be called when an "sv <command>" command is issued on the
    // server console.
    // The game can issue gi.argc() / gi.argv() commands to get the rest
    // of the parameters
    void (*ServerCommand)();

    //
    // global variables shared between game and server
    //

    // The edict array is allocated in the game dll so it
    // can vary in size from one game to another.
    //
    // The size will be fixed when ge->Init() is called
    edict_t     *edicts;
    size_t      edict_size;
    uint32_t    num_edicts; // current number, <= max_edicts
    uint32_t    max_edicts;

    // [Paril-KEX] special flags to indicate something to the server
    server_flags_t  server_flags;

    // [KEX]: Pmove as export
    void (*Pmove)(pmove_t *pmove); // player movement code called by server & client

    // Fetch named extension from game DLL.
    void *(*GetExtension)(const char *name);

    void    (*Bot_SetWeapon)(edict_t * botEdict, const int weaponIndex, const bool instantSwitch);
    void    (*Bot_TriggerEdict)(edict_t * botEdict, edict_t * edict);
    void    (*Bot_UseItem)(edict_t * botEdict, const int32_t itemID);
    int32_t (*Bot_GetItemID)(const char * classname);
    void    (*Edict_ForceLookAtPoint)(edict_t * edict, gvec3_cref_t point);
    bool    (*Bot_PickedUpItem )(edict_t * botEdict, edict_t * itemEdict);

    // [KEX]: Checks entity visibility instancing
    bool (*Entity_IsVisibleToPlayer)(edict_t* ent, edict_t* player);

    // Fetch info from the shadow light, for culling
    const shadow_light_data_t *(*GetShadowLightData)(int32_t entity_number);
};

// generic rectangle
struct vrect_t
{
    int32_t x, y, width, height;
};

enum class text_align_t
{
    LEFT,
    CENTER,
    RIGHT
};

// transient data from server
struct cg_server_data_t
{
    char                           layout[1024];
    std::array<int16_t, MAX_ITEMS> inventory;
};

constexpr int32_t PROTOCOL_VERSION_3XX   = 34;
constexpr int32_t PROTOCOL_VERSION_DEMOS = 2022;
constexpr int32_t PROTOCOL_VERSION       = 2023;

//
// functions provided by main engine for client
//
struct cgame_import_t
{
    uint32_t    tick_rate;
    float       frame_time_s;
    uint32_t    frame_time_ms;

    // print to appropriate places (console, log file, etc)
    void (*Com_Print)(const char *msg);
    
    // config strings hold all the index strings, the lightstyles,
    // and misc data like the sky definition and cdtrack.
    // All of the current configstrings are sent to clients when
    // they connect, and changes are sent to all connected clients.
    const char *(*get_configstring)(int num);

    void (*Com_Error)(const char *message);

    // managed memory allocation
    void *(*TagMalloc)(size_t size, int tag);
    void (*TagFree)(void *block);
    void (*FreeTags)(int tag);

    // console variable interaction
	cvar_t *(*cvar)(const char *var_name, const char *value, cvar_flags_t flags);
    cvar_t *(*cvar_set)(const char *var_name, const char *value);
    cvar_t *(*cvar_forceset)(const char *var_name, const char *value);

    // add commands to the server console as if they were typed in
    // for map changing, etc
    void (*AddCommandString)(const char *text);

    // Fetch named extension from engine.
    void *(*GetExtension)(const char *name);

    // Check whether current frame is valid
    bool (*CL_FrameValid) ();

    // Get client frame time delta
    float (*CL_FrameTime) ();

    // [Paril-KEX] cgame-specific stuff
    uint64_t (*CL_ClientTime) ();
    uint64_t (*CL_ClientRealTime) ();
    int32_t (*CL_ServerFrame) ();
    int32_t (*CL_ServerProtocol) ();
    const char *(*CL_GetClientName) (int32_t index);
    const char *(*CL_GetClientPic) (int32_t index);
    const char *(*CL_GetClientDogtag) (int32_t index);
    const char *(*CL_GetKeyBinding) (const char *binding); // fetch key bind for key, or empty string
    bool (*Draw_RegisterPic) (const char *name);
    void (*Draw_GetPicSize) (int *w, int *h, const char *name); // will return 0 0 if not found
    void (*SCR_DrawChar)(int x, int y, int scale, int num, bool shadow);
    void (*SCR_DrawPic) (int x, int y, int w, int h, const char *name);
    void (*SCR_DrawColorPic)(int x, int y, int w, int h, const char* name, const rgba_t &color);

    // [Paril-KEX] kfont stuff
    void(*SCR_SetAltTypeface)(bool enabled);
    void (*SCR_DrawFontString)(const char *str, int x, int y, int scale, const rgba_t &color, bool shadow, text_align_t align);
    vec2_t (*SCR_MeasureFontString)(const char *str, int scale);
    float (*SCR_FontLineHeight)(int scale);

    // [Paril-KEX] for legacy text input (not used in lobbies)
    bool (*CL_GetTextInput)(const char **msg, bool *is_team);

    // [Paril-KEX] FIXME this probably should be an export instead...
    int32_t (*CL_GetWarnAmmoCount)(int32_t weapon_id);

    // === [KEX] Additional APIs ===
    // returns a *temporary string* ptr to a localized input
    const char* (*Localize) (const char *base, const char **args, size_t num_args);

    // [Paril-KEX] Draw binding, for centerprint; returns y offset
    int32_t (*SCR_DrawBind) (int32_t isplit, const char *binding, const char *purpose, int x, int y, int scale);
    
    // [Paril-KEX]
    bool (*CL_InAutoDemoLoop) ();
};

//
// functions exported for client by game subsystem
//
struct cgame_export_t
{
    int         apiversion;

    // the init/shutdown functions will be called between levels/connections
    // and when the client initially loads.
    void (*Init)();
    void (*Shutdown)();

    // [Paril-KEX] hud drawing
    void (*DrawHUD) (int32_t isplit, const cg_server_data_t *data, vrect_t hud_vrect, vrect_t hud_safe, int32_t scale, int32_t playernum, const player_state_t *ps);
    // [Paril-KEX] precache special pics used by hud
    void (*TouchPics) ();

    // [Paril-KEX] layout flags; see layout_flags_t
    layout_flags_t (*LayoutFlags) (const player_state_t *ps);

    // [Paril-KEX] fetch the current wheel weapon ID in use
    int32_t (*GetActiveWeaponWheelWeapon) (const player_state_t *ps);

    // [Paril-KEX] fetch owned weapon IDs
    uint32_t (*GetOwnedWeaponWheelWeapons) (const player_state_t *ps);

    // [Paril-KEX] fetch ammo count for given ammo id
    int16_t (*GetWeaponWheelAmmoCount)(const player_state_t *ps, int32_t ammo_id);
    
    // [Paril-KEX] fetch powerup count for given powerup id
    int16_t (*GetPowerupWheelCount)(const player_state_t *ps, int32_t powerup_id);

    // [Paril-KEX] fetch how much damage was registered by these stats
    int16_t (*GetHitMarkerDamage)(const player_state_t *ps);

    // [KEX]: Pmove as export
    void (*Pmove)(pmove_t *pmove); // player movement code called by server & client

    // [Paril-KEX] allow cgame to react to configstring changes
    void (*ParseConfigString)(int32_t i, const char *s);

    // [Paril-KEX] parse centerprint-like messages
    void (*ParseCenterPrint)(const char *str, int isplit, bool instant);

    // [Paril-KEX] tell the cgame to clear notify stuff
    void (*ClearNotify)(int32_t isplit);

    // [Paril-KEX] tell the cgame to clear centerprint state
    void (*ClearCenterprint)(int32_t isplit);

    // [Paril-KEX] be notified by the game DLL of a message of some sort
    void (*NotifyMessage)(int32_t isplit, const char *msg, bool is_chat);

    // [Paril-KEX]
    void (*GetMonsterFlashOffset)(monster_muzzleflash_id_t id, gvec3_ref_t offset);

    // Fetch named extension from cgame DLL.
    void *(*GetExtension)(const char *name);
};

// EOF
