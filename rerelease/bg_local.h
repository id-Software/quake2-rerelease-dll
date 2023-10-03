// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

// g_local.h -- local definitions for game module
#pragma once

#include "q_std.h"

// define GAME_INCLUDE so that game.h does not define the
// short, server-visible gclient_t and edict_t structures,
// because we define the full size ones in this file
#define GAME_INCLUDE
#include "game.h"

//
// p_move.c
//
struct pm_config_t
{
	int32_t		airaccel = 0;
	bool		n64_physics = false;
};

extern pm_config_t pm_config;

void Pmove(pmove_t *pmove);
using pm_trace_func_t = trace_t(const vec3_t &start, const vec3_t &mins, const vec3_t &maxs, const vec3_t &end);
using pm_trace_t = std::function<pm_trace_func_t>;
void PM_StepSlideMove_Generic(vec3_t &origin, vec3_t &velocity, float frametime, const vec3_t &mins, const vec3_t &maxs, touch_list_t &touch, bool has_time, pm_trace_t trace);

enum class stuck_result_t
{
	GOOD_POSITION,
	FIXED,
	NO_GOOD_POSITION
};

using stuck_object_trace_fn_t = trace_t(const vec3_t &, const vec3_t &, const vec3_t &, const vec3_t &);

stuck_result_t G_FixStuckObject_Generic(vec3_t &origin, const vec3_t &own_mins, const vec3_t &own_maxs, std::function<stuck_object_trace_fn_t> trace);

// state for coop respawning; used to select which
// message to print for the player this is set on.
enum coop_respawn_t
{
	COOP_RESPAWN_NONE, // no messagee
	COOP_RESPAWN_IN_COMBAT, // player is in combat
	COOP_RESPAWN_BAD_AREA, // player not in a good spot
	COOP_RESPAWN_BLOCKED, // spawning was blocked by something
	COOP_RESPAWN_WAITING, // for players that are waiting to respawn
	COOP_RESPAWN_NO_LIVES, // out of lives, so need to wait until level switch
	COOP_RESPAWN_TOTAL
};

// reserved general CS ranges
enum
{
	CONFIG_CTF_MATCH = CS_GENERAL,
	CONFIG_CTF_TEAMINFO,
	CONFIG_CTF_PLAYER_NAME,
	CONFIG_CTF_PLAYER_NAME_END = CONFIG_CTF_PLAYER_NAME + MAX_CLIENTS,

	// nb: offset by 1 since NONE is zero
	CONFIG_COOP_RESPAWN_STRING,
	CONFIG_COOP_RESPAWN_STRING_END = CONFIG_COOP_RESPAWN_STRING + (COOP_RESPAWN_TOTAL - 1),

	// [Paril-KEX] if 1, n64 player physics apply
	CONFIG_N64_PHYSICS,
	CONFIG_HEALTH_BAR_NAME, // active health bar name

	CONFIG_STORY,

	CONFIG_LAST
};

static_assert(CONFIG_LAST <= CS_GENERAL + MAX_GENERAL);

// ammo IDs
enum ammo_t : uint8_t
{
	AMMO_BULLETS,
	AMMO_SHELLS,
	AMMO_ROCKETS,
	AMMO_GRENADES,
	AMMO_CELLS,
	AMMO_SLUGS,
	// RAFAEL
	AMMO_MAGSLUG,
	AMMO_TRAP,
	// RAFAEL
	// ROGUE
	AMMO_FLECHETTES,
	AMMO_TESLA,
	AMMO_DISRUPTOR,
	AMMO_PROX,
	// ROGUE
    AMMO_MAX
};

// powerup IDs
enum powerup_t : uint8_t
{
	POWERUP_SCREEN,
	POWERUP_SHIELD,

	POWERUP_AM_BOMB,

	POWERUP_QUAD,
	POWERUP_QUADFIRE,
	POWERUP_INVULNERABILITY,
	POWERUP_INVISIBILITY,
	POWERUP_SILENCER,
	POWERUP_REBREATHER,
	POWERUP_ENVIROSUIT,
	POWERUP_ADRENALINE,
	POWERUP_IR_GOGGLES,
	POWERUP_DOUBLE,
	POWERUP_SPHERE_VENGEANCE,
	POWERUP_SPHERE_HUNTER,
	POWERUP_SPHERE_DEFENDER,
	POWERUP_DOPPELGANGER,

	POWERUP_FLASHLIGHT,
	POWERUP_COMPASS,
	POWERUP_TECH1,
	POWERUP_TECH2,
	POWERUP_TECH3,
	POWERUP_TECH4,
	POWERUP_MAX
};

// ammo stats compressed in 9 bits per entry
// since the range is 0-300
constexpr size_t BITS_PER_AMMO = 9;

template<typename TI>
constexpr size_t num_of_type_for_bits(size_t num_bits)
{
	return (num_bits + (sizeof(TI) * 8) - 1) / ((sizeof(TI) * 8) + 1);
}

template<size_t bits_per_value>
constexpr void set_compressed_integer(uint16_t *start, uint8_t id, uint16_t count)
{
	uint16_t bit_offset = bits_per_value * id;
	uint16_t byte = bit_offset / 8;
	uint16_t bit_shift = bit_offset % 8;
	uint16_t mask = (bit_v<bits_per_value> - 1) << bit_shift;
	uint16_t *base = (uint16_t *) ((uint8_t *) start + byte);
	*base = (*base & ~mask) | ((count << bit_shift) & mask);
}

template<size_t bits_per_value>
constexpr uint16_t get_compressed_integer(uint16_t *start, uint8_t id)
{
	uint16_t bit_offset = bits_per_value * id;
	uint16_t byte = bit_offset / 8;
	uint16_t bit_shift = bit_offset % 8;
	uint16_t mask = (bit_v<bits_per_value> - 1) << bit_shift;
	uint16_t *base = (uint16_t *) ((uint8_t *) start + byte);
	return (*base & mask) >> bit_shift;
}

constexpr size_t NUM_BITS_FOR_AMMO = 9;
constexpr size_t NUM_AMMO_STATS = num_of_type_for_bits<uint16_t>(NUM_BITS_FOR_AMMO * AMMO_MAX);
// if this value is set on an STAT_AMMO_INFO_xxx, don't render ammo
constexpr uint16_t AMMO_VALUE_INFINITE = bit_v<NUM_BITS_FOR_AMMO> - 1;

constexpr void G_SetAmmoStat(uint16_t *start, uint8_t ammo_id, uint16_t count)
{
	set_compressed_integer<NUM_BITS_FOR_AMMO>(start, ammo_id, count);
}

constexpr uint16_t G_GetAmmoStat(uint16_t *start, uint8_t ammo_id)
{
	return get_compressed_integer<NUM_BITS_FOR_AMMO>(start, ammo_id);
}

// powerup stats compressed in 2 bits per entry;
// 3 is the max you'll ever hold, and for some
// (flashlight) it's to indicate on/off state
constexpr size_t NUM_BITS_PER_POWERUP = 2;
constexpr size_t NUM_POWERUP_STATS = num_of_type_for_bits<uint16_t>(NUM_BITS_PER_POWERUP * POWERUP_MAX);

constexpr void G_SetPowerupStat(uint16_t *start, uint8_t powerup_id, uint16_t count)
{
	set_compressed_integer<NUM_BITS_PER_POWERUP>(start, powerup_id, count);
}

constexpr uint16_t G_GetPowerupStat(uint16_t *start, uint8_t powerup_id)
{
	return get_compressed_integer<NUM_BITS_PER_POWERUP>(start, powerup_id);
}

// player_state->stats[] indexes
enum player_stat_t
{
    STAT_HEALTH_ICON = 0,
    STAT_HEALTH = 1,
    STAT_AMMO_ICON = 2,
    STAT_AMMO = 3,
    STAT_ARMOR_ICON = 4,
    STAT_ARMOR = 5,
    STAT_SELECTED_ICON = 6,
    STAT_PICKUP_ICON = 7,
    STAT_PICKUP_STRING = 8,
    STAT_TIMER_ICON = 9,
    STAT_TIMER = 10,
    STAT_HELPICON = 11,
    STAT_SELECTED_ITEM = 12,
    STAT_LAYOUTS = 13,
    STAT_FRAGS = 14,
    STAT_FLASHES = 15, // cleared each frame, 1 = health, 2 = armor
    STAT_CHASE = 16,
    STAT_SPECTATOR = 17,

	STAT_CTF_TEAM1_PIC = 18,
	STAT_CTF_TEAM1_CAPS = 19,
	STAT_CTF_TEAM2_PIC = 20,
	STAT_CTF_TEAM2_CAPS = 21,
	STAT_CTF_FLAG_PIC = 22,
	STAT_CTF_JOINED_TEAM1_PIC = 23,
	STAT_CTF_JOINED_TEAM2_PIC = 24,
	STAT_CTF_TEAM1_HEADER = 25,
	STAT_CTF_TEAM2_HEADER = 26,
	STAT_CTF_TECH = 27,
	STAT_CTF_ID_VIEW = 28,
	STAT_CTF_MATCH = 29,
	STAT_CTF_ID_VIEW_COLOR = 30,
	STAT_CTF_TEAMINFO = 31,

    // [Kex] More stats for weapon wheel
    STAT_WEAPONS_OWNED_1 = 32,
    STAT_WEAPONS_OWNED_2 = 33,
    STAT_AMMO_INFO_START = 34,
    STAT_AMMO_INFO_END = STAT_AMMO_INFO_START + NUM_AMMO_STATS - 1,
	STAT_POWERUP_INFO_START,
	STAT_POWERUP_INFO_END = STAT_POWERUP_INFO_START + NUM_POWERUP_STATS - 1,

    // [Paril-KEX] Key display
    STAT_KEY_A,
    STAT_KEY_B,
    STAT_KEY_C,

    // [Paril-KEX] currently active wheel weapon (or one we're switching to)
    STAT_ACTIVE_WHEEL_WEAPON,
	// [Paril-KEX] top of screen coop respawn state
	STAT_COOP_RESPAWN,
	// [Paril-KEX] respawns remaining
	STAT_LIVES,
	// [Paril-KEX] hit marker; # of damage we successfully landed
	STAT_HIT_MARKER,
	// [Paril-KEX]
	STAT_SELECTED_ITEM_NAME,
	// [Paril-KEX]
	STAT_HEALTH_BARS, // two health bar values; 7 bits for value, 1 bit for active
	// [Paril-KEX]
	STAT_ACTIVE_WEAPON,

	// don't use; just for verification
    STAT_LAST
};

static_assert(STAT_LAST <= MAX_STATS + 1, "stats list overflow");