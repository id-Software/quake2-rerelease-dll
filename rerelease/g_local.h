// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

// g_local.h -- local definitions for game module
#pragma once

#include "bg_local.h"

// the "gameversion" client command will print this plus compile date
constexpr const char *GAMEVERSION = "baseq2";

//==================================================================

constexpr vec3_t PLAYER_MINS = { -16, -16, -24 };
constexpr vec3_t PLAYER_MAXS = { 16, 16, 32 };

#include <charconv>

template<typename T>
constexpr bool is_char_ptr_v = std::is_convertible_v<T, const char*>;

template<typename T>
constexpr bool is_valid_loc_embed_v = !std::is_null_pointer_v<T> && (std::is_floating_point_v<std::remove_reference_t<T>> || std::is_integral_v<std::remove_reference_t<T>> || is_char_ptr_v<T>);

struct local_game_import_t : game_import_t
{
	inline local_game_import_t() = default;
	inline local_game_import_t(const game_import_t &imports) :
		game_import_t(imports)
	{
	}

private:
	// shared buffer for wrappers below
    static char print_buffer[0x10000];

public:
#ifdef USE_CPP20_FORMAT
	template<typename... Args>
	inline void Com_PrintFmt(std::format_string<Args...> format_str, Args &&... args)
#else
#define Com_PrintFmt(str, ...) \
	Com_PrintFmt_(FMT_STRING(str), __VA_ARGS__)

    template<typename S, typename... Args>
    inline void Com_PrintFmt_(const S &format_str, Args &&... args)
#endif
    {
		G_FmtTo_(print_buffer, format_str, std::forward<Args>(args)...);
		Com_Print(print_buffer);
    }

#ifdef USE_CPP20_FORMAT
	template<typename... Args>
	inline void Com_ErrorFmt(std::format_string<Args...> format_str, Args &&... args)
#else
#define Com_ErrorFmt(str, ...) \
	Com_ErrorFmt_(FMT_STRING(str), __VA_ARGS__)

    template<typename S, typename... Args>
    inline void Com_ErrorFmt_(const S &format_str, Args &&... args)
#endif
    {
		G_FmtTo_(print_buffer, format_str, std::forward<Args>(args)...);
		Com_Error(print_buffer);
    }

private:
	// localized print functions
	template<typename T>
	inline void loc_embed(T input, char* buffer, const char*& output)
	{
		if constexpr (std::is_floating_point_v<T> || std::is_integral_v<T>)
		{
			auto result = std::to_chars(buffer, buffer + MAX_INFO_STRING - 1, input);
			*result.ptr = '\0';
			output = buffer;
		}
		else if constexpr (is_char_ptr_v<T>)
		{
			if (!input)
				Com_Error("null const char ptr passed to loc");

			output = input;
		}
		else
			Com_Error("invalid loc argument");
	}

	static std::array<char[MAX_INFO_STRING], MAX_LOCALIZATION_ARGS> buffers;
	static std::array<const char*, MAX_LOCALIZATION_ARGS> buffer_ptrs;

public:
	template<typename... Args>
	inline void LocClient_Print(edict_t* e, print_type_t level, const char* base, Args&& ...args)
	{
		static_assert(sizeof...(args) < MAX_LOCALIZATION_ARGS, "too many arguments to gi.LocClient_Print");
		static_assert(((is_valid_loc_embed_v<Args>) && ...), "invalid argument passed to gi.LocClient_Print");

		size_t n = 0;
		((loc_embed(args, buffers[n], buffer_ptrs[n]), ++n), ...);

		Loc_Print(e, level, base, &buffer_ptrs.front(), sizeof...(args));
	}

	template<typename... Args>
	inline void LocBroadcast_Print(print_type_t level, const char* base, Args&& ...args)
	{
		static_assert(sizeof...(args) < MAX_LOCALIZATION_ARGS, "too many arguments to gi.LocBroadcast_Print");
		static_assert(((is_valid_loc_embed_v<Args>) && ...), "invalid argument passed to gi.LocBroadcast_Print");

		size_t n = 0;
		((loc_embed(args, buffers[n], buffer_ptrs[n]), ++n), ...);

		Loc_Print(nullptr, (print_type_t)(level | print_type_t::PRINT_BROADCAST), base, &buffer_ptrs.front(), sizeof...(args));
	}

	template<typename... Args>
	inline void LocCenter_Print(edict_t* e, const char* base, Args&& ...args)
	{
		static_assert(sizeof...(args) < MAX_LOCALIZATION_ARGS, "too many arguments to gi.LocCenter_Print");
		static_assert(((is_valid_loc_embed_v<Args>) && ...), "invalid argument passed to gi.LocCenter_Print");

		size_t n = 0;
		((loc_embed(args, buffers[n], buffer_ptrs[n]), ++n), ...);

		Loc_Print(e, PRINT_CENTER, base, &buffer_ptrs.front(), sizeof...(args));
	}

    // collision detection
    [[nodiscard]] inline trace_t trace(const vec3_t &start, const vec3_t &mins, const vec3_t &maxs, const vec3_t &end, const edict_t *passent, contents_t contentmask)
	{
		return game_import_t::trace(start, &mins, &maxs, end, passent, contentmask);
	}

    [[nodiscard]] inline trace_t traceline(const vec3_t &start, const vec3_t &end, const edict_t *passent, contents_t contentmask)
	{
		return game_import_t::trace(start, nullptr, nullptr, end, passent, contentmask);
	}

	// [Paril-KEX] clip the box against the specified entity
    [[nodiscard]] inline trace_t clip(edict_t *entity, const vec3_t &start, const vec3_t &mins, const vec3_t &maxs, const vec3_t &end, contents_t contentmask)
	{
		return game_import_t::clip(entity, start, &mins, &maxs, end, contentmask);
	}

    [[nodiscard]] inline trace_t clip(edict_t *entity, const vec3_t &start, const vec3_t &end, contents_t contentmask)
	{
		return game_import_t::clip(entity, start, nullptr, nullptr, end, contentmask);
	}

	void unicast(edict_t *ent, bool reliable, uint32_t dupe_key = 0)
	{
		game_import_t::unicast(ent, reliable, dupe_key);
	}

	void local_sound(edict_t *target, const vec3_t &origin, edict_t *ent, soundchan_t channel, int soundindex, float volume, float attenuation, float timeofs, uint32_t dupe_key = 0)
	{
		game_import_t::local_sound(target, &origin, ent, channel, soundindex, volume, attenuation, timeofs, dupe_key);
	}

	void local_sound(edict_t *target, edict_t *ent, soundchan_t channel, int soundindex, float volume, float attenuation, float timeofs, uint32_t dupe_key = 0)
	{
		game_import_t::local_sound(target, nullptr, ent, channel, soundindex, volume, attenuation, timeofs, dupe_key);
	}

	void local_sound(const vec3_t &origin, edict_t *ent, soundchan_t channel, int soundindex, float volume, float attenuation, float timeofs, uint32_t dupe_key = 0)
	{
		game_import_t::local_sound(ent, &origin, ent, channel, soundindex, volume, attenuation, timeofs, dupe_key);
	}

	void local_sound(edict_t *ent, soundchan_t channel, int soundindex, float volume, float attenuation, float timeofs, uint32_t dupe_key = 0)
	{
		game_import_t::local_sound(ent, nullptr, ent, channel, soundindex, volume, attenuation, timeofs, dupe_key);
	}
};

extern local_game_import_t  gi;

// edict->spawnflags
// these are set with checkboxes on each entity in the map editor.
// the following 8 are reserved and should never be used by any entity.
// (power cubes in coop use these after spawning as well)
struct spawnflags_t
{
	uint32_t		value;

	explicit constexpr spawnflags_t(uint32_t v) :
		value(v)
	{
	}

	explicit operator uint32_t() const
	{
		return value;
	}
	
	// has any flags at all (!!a)
	constexpr bool any() const { return !!value; }
	// has any of the given flags (!!(a & b))
	constexpr bool has(const spawnflags_t &flags) const { return !!(value & flags.value); }
	// has all of the given flags ((a & b) == b)
	constexpr bool has_all(const spawnflags_t &flags) const { return (value & flags.value) == flags.value; }
	constexpr bool operator!() const { return !value; }

	constexpr bool operator==(const spawnflags_t &flags) const
	{
		return value == flags.value;
	}

	constexpr bool operator!=(const spawnflags_t &flags) const
	{
		return value != flags.value;
	}

	constexpr spawnflags_t operator~() const
	{
		return spawnflags_t(~value);
	}
	constexpr spawnflags_t operator|(const spawnflags_t &v2) const
	{
		return spawnflags_t(value | v2.value);
	}
	constexpr spawnflags_t operator&(const spawnflags_t &v2) const
	{
		return spawnflags_t(value & v2.value);
	}
	constexpr spawnflags_t operator^(const spawnflags_t &v2) const
	{
		return spawnflags_t(value ^ v2.value);
	}
	constexpr spawnflags_t &operator|=(const spawnflags_t &v2)
	{
		value |= v2.value;
		return *this;
	}
	constexpr spawnflags_t &operator&=(const spawnflags_t &v2)
	{
		value &= v2.value;
		return *this;
	}
	constexpr spawnflags_t &operator^=(const spawnflags_t &v2)
	{
		value ^= v2.value;
		return *this;
	}
};

// these spawnflags affect every entity. note that items are a bit special
// because these 8 bits are instead used for power cube bits.
constexpr spawnflags_t  SPAWNFLAG_NONE = spawnflags_t(0);
constexpr spawnflags_t	SPAWNFLAG_NOT_EASY = spawnflags_t(0x00000100),
						SPAWNFLAG_NOT_MEDIUM = spawnflags_t(0x00000200),
						SPAWNFLAG_NOT_HARD = spawnflags_t(0x00000400),
						SPAWNFLAG_NOT_DEATHMATCH = spawnflags_t(0x00000800),
						SPAWNFLAG_NOT_COOP = spawnflags_t(0x00001000),
						SPAWNFLAG_RESERVED1 = spawnflags_t(0x00002000),
						SPAWNFLAG_COOP_ONLY = spawnflags_t(0x00004000),
						SPAWNFLAG_RESERVED2 = spawnflags_t(0x00008000);

constexpr spawnflags_t SPAWNFLAG_EDITOR_MASK = (SPAWNFLAG_NOT_EASY | SPAWNFLAG_NOT_MEDIUM | SPAWNFLAG_NOT_HARD | SPAWNFLAG_NOT_DEATHMATCH |
							SPAWNFLAG_NOT_COOP | SPAWNFLAG_RESERVED1 | SPAWNFLAG_COOP_ONLY | SPAWNFLAG_RESERVED2);

// use this for global spawnflags
constexpr spawnflags_t operator "" _spawnflag(unsigned long long int v)
{
	if (v & SPAWNFLAG_EDITOR_MASK.value)
		throw std::invalid_argument("attempting to use reserved spawnflag");

	return static_cast<spawnflags_t>(static_cast<uint32_t>(v));
}

// use this for global spawnflags
constexpr spawnflags_t operator "" _spawnflag_bit(unsigned long long int v)
{
	v = 1ull << v;

	if (v & SPAWNFLAG_EDITOR_MASK.value)
		throw std::invalid_argument("attempting to use reserved spawnflag");

	return static_cast<spawnflags_t>(static_cast<uint32_t>(v));
}

// stores a level time; most newer engines use int64_t for
// time storage, but seconds are handy for compatibility
// with Quake and older mods.
struct gtime_t
{
private:
	// times always start at zero, just to prevent memory issues
	int64_t _ms = 0;

	// internal; use _sec/_ms/_min or gtime_t::from_sec(n)/gtime_t::from_ms(n)/gtime_t::from_min(n)
	constexpr explicit gtime_t(const int64_t &ms) : _ms(ms)
	{
	}

public:
	constexpr gtime_t() = default;
	constexpr gtime_t(const gtime_t &) = default;
	constexpr gtime_t &operator=(const gtime_t &) = default;

	// constructors are here, explicitly named, so you always
	// know what you're getting.

	// new time from ms
	static constexpr gtime_t from_ms(const int64_t &ms)
	{
		return gtime_t(ms);
	}

	// new time from seconds
	template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T> || std::is_integral_v<T>>>
	static constexpr gtime_t from_sec(const T &s)
	{
		return gtime_t(static_cast<int64_t>(s * 1000));
	}

	// new time from minutes
	template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T> || std::is_integral_v<T>>>
	static constexpr gtime_t from_min(const T &s)
	{
		return gtime_t(static_cast<int64_t>(s * 60000));
	}

	// new time from hz
	static constexpr gtime_t from_hz(const uint64_t &hz)
	{
		return from_ms(static_cast<int64_t>((1.0 / hz) * 1000));
	}

	// get value in minutes (truncated for integers)
	template<typename T = float>
	constexpr T minutes() const
	{
		return static_cast<T>(_ms / static_cast<std::conditional_t<std::is_floating_point_v<T>, T, float>>(60000));
	}

	// get value in seconds (truncated for integers)
	template<typename T = float>
	constexpr T seconds() const
	{
		return static_cast<T>(_ms / static_cast<std::conditional_t<std::is_floating_point_v<T>, T, float>>(1000));
	}

	// get value in milliseconds
	constexpr const int64_t &milliseconds() const
	{
		return _ms;
	}

	int64_t frames() const
	{
		return _ms / gi.frame_time_ms;
	}

	// check if non-zero
	constexpr explicit operator bool() const
	{
		return !!_ms;
	}

	// invert time
	constexpr gtime_t operator-() const
	{
		return gtime_t(-_ms);
	}

	// operations with other times as input
	constexpr gtime_t operator+(const gtime_t &r) const
	{
		return gtime_t(_ms + r._ms);
	}
	constexpr gtime_t operator-(const gtime_t &r) const
	{
		return gtime_t(_ms - r._ms);
	}
	constexpr gtime_t &operator+=(const gtime_t &r)
	{
		return *this = *this + r;
	}
	constexpr gtime_t &operator-=(const gtime_t &r)
	{
		return *this = *this - r;
	}

	// operations with scalars as input
	template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T> || std::is_integral_v<T>>>
	constexpr gtime_t operator*(const T &r) const
	{
		return gtime_t::from_ms(static_cast<int64_t>(_ms * r));
	}
	template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T> || std::is_integral_v<T>>>
	constexpr gtime_t operator/(const T &r) const
	{
		return gtime_t::from_ms(static_cast<int64_t>(_ms / r));
	}
	template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T> || std::is_integral_v<T>>>
	constexpr gtime_t &operator*=(const T &r)
	{
		return *this = *this * r;
	}
	template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T> || std::is_integral_v<T>>>
	constexpr gtime_t &operator/=(const T &r)
	{
		return *this = *this / r;
	}

	// comparisons with gtime_ts
	constexpr bool operator==(const gtime_t &time) const
	{
		return _ms == time._ms;
	}
	constexpr bool operator!=(const gtime_t &time) const
	{
		return _ms != time._ms;
	}
	constexpr bool operator<(const gtime_t &time) const
	{
		return _ms < time._ms;
	}
	constexpr bool operator>(const gtime_t &time) const
	{
		return _ms > time._ms;
	}
	constexpr bool operator<=(const gtime_t &time) const
	{
		return _ms <= time._ms;
	}
	constexpr bool operator>=(const gtime_t &time) const
	{
		return _ms >= time._ms;
	}
};

// user literals, allowing you to specify times
// as 128_sec and 128_ms
constexpr gtime_t operator"" _min(long double s)
{
	return gtime_t::from_min(s);
}
constexpr gtime_t operator"" _min(unsigned long long int s)
{
	return gtime_t::from_min(s);
}
constexpr gtime_t operator"" _sec(long double s)
{
	return gtime_t::from_sec(s);
}
constexpr gtime_t operator"" _sec(unsigned long long int s)
{
	return gtime_t::from_sec(s);
}
constexpr gtime_t operator"" _ms(long double s)
{
	return gtime_t::from_ms(static_cast<int64_t>(s));
}
constexpr gtime_t operator"" _ms(unsigned long long int s)
{
	return gtime_t::from_ms(static_cast<int64_t>(s));
}
constexpr gtime_t operator"" _hz(unsigned long long int s)
{
	return gtime_t::from_ms(static_cast<int64_t>((1.0 / s) * 1000));
}

#define SERVER_TICK_RATE gi.tick_rate // in hz
extern gtime_t FRAME_TIME_S;
extern gtime_t FRAME_TIME_MS;

// view pitching times
inline gtime_t DAMAGE_TIME_SLACK()
{
	return (100_ms - FRAME_TIME_MS);
}

inline gtime_t DAMAGE_TIME()
{
	return 500_ms + DAMAGE_TIME_SLACK();
}

inline gtime_t FALL_TIME()
{
	return 300_ms + DAMAGE_TIME_SLACK();
}

// every save_data_list_t has a tag
// which is used for integrity checks.
enum save_data_tag_t
{
	SAVE_DATA_MMOVE,

	SAVE_FUNC_MONSTERINFO_STAND,
	SAVE_FUNC_MONSTERINFO_IDLE,
	SAVE_FUNC_MONSTERINFO_SEARCH,
	SAVE_FUNC_MONSTERINFO_WALK,
	SAVE_FUNC_MONSTERINFO_RUN,
	SAVE_FUNC_MONSTERINFO_DODGE,
	SAVE_FUNC_MONSTERINFO_ATTACK,
	SAVE_FUNC_MONSTERINFO_MELEE,
	SAVE_FUNC_MONSTERINFO_SIGHT,
	SAVE_FUNC_MONSTERINFO_CHECKATTACK,
	SAVE_FUNC_MONSTERINFO_SETSKIN,

	SAVE_FUNC_MONSTERINFO_BLOCKED,
	SAVE_FUNC_MONSTERINFO_DUCK,
	SAVE_FUNC_MONSTERINFO_UNDUCK,
	SAVE_FUNC_MONSTERINFO_SIDESTEP,
	SAVE_FUNC_MONSTERINFO_PHYSCHANGED,

	SAVE_FUNC_MOVEINFO_ENDFUNC,
	SAVE_FUNC_MOVEINFO_BLOCKED,

	SAVE_FUNC_PRETHINK,
	SAVE_FUNC_THINK,
	SAVE_FUNC_TOUCH,
	SAVE_FUNC_USE,
	SAVE_FUNC_PAIN,
	SAVE_FUNC_DIE
};

// forward-linked list, storing data for
// saving pointers. every save_data_ptr has an
// instance of this; there's one head instance of this
// in g_save.cpp.
struct save_data_list_t
{
	const char			   *name; // name of savable object; persisted in the JSON file
	save_data_tag_t			tag;
	const void			   *ptr; // pointer to raw data
	const save_data_list_t *next; // next in list

	save_data_list_t(const char *name, save_data_tag_t tag, const void *ptr);

	static const save_data_list_t *fetch(const void *link_ptr, save_data_tag_t tag);
};

#include <functional>

// save data wrapper, which holds a pointer to a T
// and the tag value for integrity. this is how you
// store a savable pointer of data safely.
template<typename T, size_t Tag>
struct save_data_t
{
	using value_type = typename std::conditional<std::is_pointer<T>::value &&
												 std::is_function<typename std::remove_pointer<T>::type>::value,
												 T,
												 const T *>::type;
private:
	value_type value;
	const save_data_list_t *list;

public:
	constexpr save_data_t() :
		value(nullptr),
		list(nullptr)
	{
	}

	constexpr save_data_t(nullptr_t) :
		save_data_t()
	{
	}

	constexpr save_data_t(const save_data_list_t *list_in) :
		value(list_in->ptr),
		list(list_in)
	{
	}

	inline save_data_t(value_type ptr_in) :
		value(ptr_in),
		list(ptr_in ? save_data_list_t::fetch(reinterpret_cast<const void *>(ptr_in), static_cast<save_data_tag_t>(Tag)) : nullptr)
	{
	}

	inline save_data_t(const save_data_t<T, Tag> &ref_in) :
		save_data_t(ref_in.value)
	{
	}

	inline save_data_t &operator=(value_type ptr_in)
	{
		if (value != ptr_in)
		{
			value = ptr_in;
			list = value ? save_data_list_t::fetch(reinterpret_cast<const void *>(ptr_in), static_cast<save_data_tag_t>(Tag)) : nullptr;
		}

		return *this;
	}
	
	constexpr const value_type pointer() const { return value; }
	constexpr const save_data_list_t *save_list() const { return list; }
	constexpr const char *name() const { return value ? list->name : "null"; }
	constexpr const value_type operator->() const { return value; }
	constexpr explicit operator bool() const { return value; }
	constexpr bool operator==(value_type ptr_in) const { return value == ptr_in; }
	constexpr bool operator!=(value_type ptr_in) const { return value != ptr_in; }
	constexpr bool operator==(const save_data_t<T, Tag> *ptr_in) const { return value == ptr_in->value; }
	constexpr bool operator==(const save_data_t<T, Tag> &ref_in) const { return value == ref_in.value; }
	constexpr bool operator!=(const save_data_t<T, Tag> *ptr_in) const { return value != ptr_in->value; }
	constexpr bool operator!=(const save_data_t<T, Tag> &ref_in) const { return value != ref_in.value; }

	// invoke wrapper, for function-likes
	template<typename... Args>
	inline auto operator()(Args&& ...args) const
	{
		static_assert(std::is_invocable_v<std::remove_pointer_t<T>, Args...>, "bad invoke args");
		return std::invoke(value, std::forward<Args>(args)...);
	}
};

// memory tags to allow dynamic memory to be cleaned up
enum
{
	TAG_GAME = 765, // clear when unloading the dll
	TAG_LEVEL = 766 // clear when loading a new level
};

constexpr float MELEE_DISTANCE = 50;

constexpr size_t BODY_QUEUE_SIZE = 8;

// null trace used when touches don't need a trace
constexpr trace_t null_trace {};

enum weaponstate_t
{
	WEAPON_READY,
	WEAPON_ACTIVATING,
	WEAPON_DROPPING,
	WEAPON_FIRING
};

// gib flags
enum gib_type_t
{
	GIB_NONE =      0, // no flags (organic)
	GIB_METALLIC =  1, // bouncier
	GIB_ACID =		2, // acidic (gekk)
	GIB_HEAD =		4, // head gib; the input entity will transform into this
	GIB_DEBRIS =	8, // explode outwards rather than in velocity, no blood
	GIB_SKINNED =	16, // use skinnum
	GIB_UPRIGHT =   32, // stay upright on ground
};
MAKE_ENUM_BITFLAGS(gib_type_t);

// monster ai flags
enum monster_ai_flags_t : uint64_t
{
	AI_NONE = 0,
	AI_STAND_GROUND = bit_v<0>,
	AI_TEMP_STAND_GROUND = bit_v<1>,
	AI_SOUND_TARGET = bit_v<2>,
	AI_LOST_SIGHT = bit_v<3>,
	AI_PURSUIT_LAST_SEEN = bit_v<4>,
	AI_PURSUE_NEXT = bit_v<5>,
	AI_PURSUE_TEMP = bit_v<6>,
	AI_HOLD_FRAME = bit_v<7>,
	AI_GOOD_GUY = bit_v<8>,
	AI_BRUTAL = bit_v<9>,
	AI_NOSTEP = bit_v<10>,
	AI_DUCKED = bit_v<11>,
	AI_COMBAT_POINT = bit_v<12>,
	AI_MEDIC = bit_v<13>,
	AI_RESURRECTING = bit_v<14>,

	// ROGUE
	AI_MANUAL_STEERING = bit_v<15>,
	AI_TARGET_ANGER = bit_v<16>,
	AI_DODGING = bit_v<17>,
	AI_CHARGING = bit_v<18>,
	AI_HINT_PATH = bit_v<19>,
	AI_IGNORE_SHOTS = bit_v<20>,
	// PMM - FIXME - last second added for E3 .. there's probably a better way to do this, but
	// this works
	AI_DO_NOT_COUNT = bit_v<21>,	 // set for healed monsters
	AI_SPAWNED_CARRIER = bit_v<22>, // both do_not_count and spawned are set for spawned monsters
	AI_SPAWNED_MEDIC_C = bit_v<23>, // both do_not_count and spawned are set for spawned monsters
	AI_SPAWNED_WIDOW = bit_v<24>,	 // both do_not_count and spawned are set for spawned monsters
	AI_BLOCKED = bit_v<25>, // used by blocked_checkattack: set to say I'm attacking while blocked
							// (prevents run-attacks)
							// ROGUE
	AI_SPAWNED_ALIVE = bit_v<26>, // [Paril-KEX] for spawning dead
	AI_SPAWNED_DEAD = bit_v<27>,
	AI_HIGH_TICK_RATE = bit_v<28>, // not limited by 10hz actions
	AI_NO_PATH_FINDING = bit_v<29>, // don't try nav nodes for path finding
	AI_PATHING = bit_v<30>, // using nav nodes currently
	AI_STINKY = bit_v<31>, // spawn flies
	AI_STUNK = bit_v<32>, // already spawned files

	AI_ALTERNATE_FLY = bit_v<33>, // use alternate flying mechanics; see monsterinfo.fly_xxx
	AI_TEMP_MELEE_COMBAT = bit_v<34>, // temporarily switch to the melee combat style
	AI_FORGET_ENEMY = bit_v<35>,			// forget the current enemy
	AI_DOUBLE_TROUBLE = bit_v<36>, // JORG only
	AI_REACHED_HOLD_COMBAT = bit_v<37>,
	AI_THIRD_EYE = bit_v<38>
};
MAKE_ENUM_BITFLAGS(monster_ai_flags_t);

constexpr monster_ai_flags_t AI_SPAWNED_MASK =
	AI_SPAWNED_CARRIER | AI_SPAWNED_MEDIC_C | AI_SPAWNED_WIDOW; // mask to catch all three flavors of spawned

// monster attack state
enum monster_attack_state_t
{
	AS_NONE,
	AS_STRAIGHT,
	AS_SLIDING,
	AS_MELEE,
	AS_MISSILE,
	AS_BLIND // PMM - used by boss code to do nasty things even if it can't see you
};

// handedness values
enum handedness_t
{
	RIGHT_HANDED,
	LEFT_HANDED,
	CENTER_HANDED
};

enum class auto_switch_t
{
	SMART,
	ALWAYS,
	ALWAYS_NO_AMMO,
	NEVER
};

constexpr uint32_t SFL_CROSS_TRIGGER_MASK = (0xffffffffu & ~SPAWNFLAG_EDITOR_MASK.value);

// noise types for PlayerNoise
enum player_noise_t
{
	PNOISE_SELF,
	PNOISE_WEAPON,
	PNOISE_IMPACT
};

struct gitem_armor_t
{
	int32_t base_count;
	int32_t max_count;
	float	normal_protection;
	float	energy_protection;
};

static constexpr gitem_armor_t jacketarmor_info = { 25, 50, .30f, .00f };
static constexpr gitem_armor_t combatarmor_info = { 50, 100, .60f, .30f };
static constexpr gitem_armor_t bodyarmor_info = { 100, 200, .80f, .60f };

// edict->movetype values
enum movetype_t {
	MOVETYPE_NONE,	 // never moves
	MOVETYPE_NOCLIP, // origin and angles change with no interaction
	MOVETYPE_PUSH,	 // no clip to world, push on box contact
	MOVETYPE_STOP,	 // no clip to world, stops on box contact

	MOVETYPE_WALK, // gravity
	MOVETYPE_STEP, // gravity, special edge handling
	MOVETYPE_FLY,
	MOVETYPE_TOSS,		 // gravity
	MOVETYPE_FLYMISSILE, // extra size to monsters
	MOVETYPE_BOUNCE,
	// RAFAEL
	MOVETYPE_WALLBOUNCE,
	// RAFAEL
	// ROGUE
	MOVETYPE_NEWTOSS // PGM - for deathball
					 // ROGUE
};

// edict->flags
enum ent_flags_t : uint64_t {
	FL_NONE                 = 0, // no flags
	FL_FLY                  = bit_v<0>,
	FL_SWIM                 = bit_v<1>, // implied immunity to drowning
	FL_IMMUNE_LASER         = bit_v<2>,
	FL_INWATER              = bit_v<3>,
	FL_GODMODE              = bit_v<4>,
	FL_NOTARGET             = bit_v<5>,
	FL_IMMUNE_SLIME         = bit_v<6>,
	FL_IMMUNE_LAVA          = bit_v<7>,
	FL_PARTIALGROUND        = bit_v<8>, // not all corners are valid
	FL_WATERJUMP            = bit_v<9>, // player jumping out of water
	FL_TEAMSLAVE            = bit_v<10>, // not the first on the team
	FL_NO_KNOCKBACK         = bit_v<11>,
	FL_POWER_ARMOR          = bit_v<12>, // power armor (if any) is active

	// ROGUE
	FL_MECHANICAL           = bit_v<13>, // entity is mechanical, use sparks not blood
	FL_SAM_RAIMI            = bit_v<14>, // entity is in sam raimi cam mode
	FL_DISGUISED            = bit_v<15>, // entity is in disguise, monsters will not recognize.
	FL_NOGIB                = bit_v<16>, // player has been vaporized by a nuke, drop no gibs
	FL_DAMAGEABLE           = bit_v<17>,
	FL_STATIONARY           = bit_v<18>,
	// ROGUE

	FL_ALIVE_KNOCKBACK_ONLY = bit_v<19>, // only apply knockback if alive or on same frame as death
	FL_NO_DAMAGE_EFFECTS    = bit_v<20>,

	// [Paril-KEX] gets scaled by coop health scaling
	FL_COOP_HEALTH_SCALE    = bit_v<21>,
	FL_FLASHLIGHT			= bit_v<22>, // enable flashlight
	FL_KILL_VELOCITY		= bit_v<23>, // for berserker slam
	FL_NOVISIBLE            = bit_v<24>, // super invisibility
	FL_DODGE				= bit_v<25>, // monster should try to dodge this
	FL_TEAMMASTER			= bit_v<26>, // is a team master (only here so that entities abusing teammaster/teamchain for stuff don't break)
	FL_LOCKED				= bit_v<27>, // entity is locked for the purposes of navigation
	FL_ALWAYS_TOUCH         = bit_v<28>, // always touch, even if we normally wouldn't
	FL_NO_STANDING			= bit_v<29>, // don't allow "standing" on non-brush entities
	FL_WANTS_POWER_ARMOR    = bit_v<30>, // for players, auto-shield

    FL_RESPAWN              = bit_v<31>, // used for item respawning
	FL_TRAP					= bit_v<32>, // entity is a trap of some kind
	FL_TRAP_LASER_FIELD		= bit_v<33>, // enough of a special case to get it's own flag...
	FL_IMMORTAL             = bit_v<34>  // never go below 1hp
};
MAKE_ENUM_BITFLAGS( ent_flags_t );

// gitem_t->flags
enum item_flags_t : uint32_t
{
	IF_NONE			= 0,
	IF_WEAPON		= bit_v<0>, // use makes active weapon
	IF_AMMO			= bit_v<1>,
	IF_ARMOR		= bit_v<2>,
	IF_STAY_COOP	= bit_v<3>,
	IF_KEY			= bit_v<4>,
	IF_POWERUP		= bit_v<5>,
	// ROGUE
	IF_NOT_GIVEABLE = bit_v<6>, // item can not be given
	// ROGUE
	IF_HEALTH		= bit_v<7>,
	// ZOID
	IF_TECH			= bit_v<8>,
	IF_NO_HASTE		= bit_v<9>,
	// ZOID

	IF_NO_INFINITE_AMMO = bit_v<10>, // [Paril-KEX] don't allow infinite ammo to affect
	IF_POWERUP_WHEEL    = bit_v<11>, // [Paril-KEX] item should be in powerup wheel
	IF_POWERUP_ONOFF    = bit_v<12>, // [Paril-KEX] for wheel; can't store more than one, show on/off state

	IF_NOT_RANDOM = bit_v<13>, // [Paril-KEX] item never shows up in randomizations

	IF_ANY			= 0xFFFFFFFF
};

MAKE_ENUM_BITFLAGS(item_flags_t);

// health edict_t->style
enum
{
	HEALTH_IGNORE_MAX = 1,
	HEALTH_TIMED = 2
};

// item IDs; must match itemlist order
enum item_id_t : int32_t {
	IT_NULL, // must always be zero

	IT_ARMOR_BODY,
	IT_ARMOR_COMBAT,
	IT_ARMOR_JACKET,
	IT_ARMOR_SHARD,

	IT_ITEM_POWER_SCREEN,
	IT_ITEM_POWER_SHIELD,

	IT_WEAPON_GRAPPLE,
	IT_WEAPON_BLASTER,
	IT_WEAPON_CHAINFIST,
	IT_WEAPON_SHOTGUN,
	IT_WEAPON_SSHOTGUN,
	IT_WEAPON_MACHINEGUN,
	IT_WEAPON_ETF_RIFLE,
	IT_WEAPON_CHAINGUN,
	IT_AMMO_GRENADES,
	IT_AMMO_TRAP,
	IT_AMMO_TESLA,
	IT_WEAPON_GLAUNCHER,
	IT_WEAPON_PROXLAUNCHER,
	IT_WEAPON_RLAUNCHER,
	IT_WEAPON_HYPERBLASTER,
	IT_WEAPON_IONRIPPER,
	IT_WEAPON_PLASMABEAM,
	IT_WEAPON_RAILGUN,
	IT_WEAPON_PHALANX,
	IT_WEAPON_BFG,
	IT_WEAPON_DISRUPTOR,
#if 0
	IT_WEAPON_DISINTEGRATOR,
#endif

	IT_AMMO_SHELLS,
	IT_AMMO_BULLETS,
	IT_AMMO_CELLS,
	IT_AMMO_ROCKETS,
	IT_AMMO_SLUGS,
	IT_AMMO_MAGSLUG,
	IT_AMMO_FLECHETTES,
	IT_AMMO_PROX,
	IT_AMMO_NUKE,
	IT_AMMO_ROUNDS,

	IT_ITEM_QUAD,
	IT_ITEM_QUADFIRE,
	IT_ITEM_INVULNERABILITY,
	IT_ITEM_INVISIBILITY,
	IT_ITEM_SILENCER,
	IT_ITEM_REBREATHER,
	IT_ITEM_ENVIROSUIT,
	IT_ITEM_ANCIENT_HEAD,
	IT_ITEM_LEGACY_HEAD,
	IT_ITEM_ADRENALINE,
	IT_ITEM_BANDOLIER,
	IT_ITEM_PACK,
	IT_ITEM_IR_GOGGLES,
	IT_ITEM_DOUBLE,
	IT_ITEM_SPHERE_VENGEANCE,
	IT_ITEM_SPHERE_HUNTER,
	IT_ITEM_SPHERE_DEFENDER,
	IT_ITEM_DOPPELGANGER,
	IT_ITEM_TAG_TOKEN,

	IT_KEY_DATA_CD,
	IT_KEY_POWER_CUBE,
	IT_KEY_EXPLOSIVE_CHARGES,
	IT_KEY_YELLOW,
	IT_KEY_POWER_CORE,
	IT_KEY_PYRAMID,
	IT_KEY_DATA_SPINNER,
	IT_KEY_PASS,
	IT_KEY_BLUE_KEY,
	IT_KEY_RED_KEY,
	IT_KEY_GREEN_KEY,
	IT_KEY_COMMANDER_HEAD,
	IT_KEY_AIRSTRIKE,
	IT_KEY_NUKE_CONTAINER,
	IT_KEY_NUKE,

	IT_HEALTH_SMALL,
	IT_HEALTH_MEDIUM,
	IT_HEALTH_LARGE,
	IT_HEALTH_MEGA,

	IT_FLAG1,
	IT_FLAG2,

	IT_TECH_RESISTANCE,
	IT_TECH_STRENGTH,
	IT_TECH_HASTE,
	IT_TECH_REGENERATION,
	
	IT_ITEM_FLASHLIGHT,
	IT_ITEM_COMPASS,

	IT_TOTAL
};

struct gitem_t
{
	item_id_t	id;		   // matches item list index
	const char *classname; // spawning name
	bool (*pickup)(edict_t *ent, edict_t *other);
	void (*use)(edict_t *ent, gitem_t *item);
	void (*drop)(edict_t *ent, gitem_t *item);
	void (*weaponthink)(edict_t *ent);
	const char *pickup_sound;
	const char *world_model;
	effects_t	world_model_flags;
	const char *view_model;

	// client side info
	const char *icon;
	const char *use_name; // for use command, english only
	const char *pickup_name; // for printing on pickup
	const char *pickup_name_definite; // definite article version for languages that need it

	int			 quantity = 0;	  // for ammo how much, for weapons how much is used per shot
	item_id_t	 ammo = IT_NULL;  // for weapons
	item_id_t	 chain = IT_NULL; // weapon chain root
	item_flags_t flags = IF_NONE; // IT_* flags

	const char *vwep_model = nullptr; // vwep model string (for weapons)

	const gitem_armor_t *armor_info = nullptr;
	int					 tag = 0;

	const char *precaches = nullptr; // string of all models, sounds, and images this item will use

	int32_t				sort_id = 0; // used by some items to control their sorting
	int32_t				quantity_warn = 5; // when to warn on low ammo

	// set in InitItems, don't set by hand
	// circular list of chained weapons
	gitem_t *chain_next = nullptr;
	// set in SP_worldspawn, don't set by hand
	// model index for vwep
	int32_t vwep_index = 0;
	// set in SetItemNames, don't set by hand
	// offset into CS_WHEEL_AMMO/CS_WHEEL_WEAPONS/CS_WHEEL_POWERUPS
	int32_t ammo_wheel_index = -1;
	int32_t weapon_wheel_index = -1;
	int32_t powerup_wheel_index = -1;
};

// means of death
enum mod_id_t : uint8_t
{
	MOD_UNKNOWN,
	MOD_BLASTER,
	MOD_SHOTGUN,
	MOD_SSHOTGUN,
	MOD_MACHINEGUN,
	MOD_CHAINGUN,
	MOD_GRENADE,
	MOD_G_SPLASH,
	MOD_ROCKET,
	MOD_R_SPLASH,
	MOD_HYPERBLASTER,
	MOD_RAILGUN,
	MOD_BFG_LASER,
	MOD_BFG_BLAST,
	MOD_BFG_EFFECT,
	MOD_HANDGRENADE,
	MOD_HG_SPLASH,
	MOD_WATER,
	MOD_SLIME,
	MOD_LAVA,
	MOD_CRUSH,
	MOD_TELEFRAG,
	MOD_TELEFRAG_SPAWN,
	MOD_FALLING,
	MOD_SUICIDE,
	MOD_HELD_GRENADE,
	MOD_EXPLOSIVE,
	MOD_BARREL,
	MOD_BOMB,
	MOD_EXIT,
	MOD_SPLASH,
	MOD_TARGET_LASER,
	MOD_TRIGGER_HURT,
	MOD_HIT,
	MOD_TARGET_BLASTER,
	// RAFAEL 14-APR-98
	MOD_RIPPER,
	MOD_PHALANX,
	MOD_BRAINTENTACLE,
	MOD_BLASTOFF,
	MOD_GEKK,
	MOD_TRAP,
	// END 14-APR-98
	//========
	// ROGUE
	MOD_CHAINFIST,
	MOD_DISINTEGRATOR,
	MOD_ETF_RIFLE,
	MOD_BLASTER2,
	MOD_HEATBEAM,
	MOD_TESLA,
	MOD_PROX,
	MOD_NUKE,
	MOD_VENGEANCE_SPHERE,
	MOD_HUNTER_SPHERE,
	MOD_DEFENDER_SPHERE,
	MOD_TRACKER,
	MOD_DBALL_CRUSH,
	MOD_DOPPLE_EXPLODE,
	MOD_DOPPLE_VENGEANCE,
	MOD_DOPPLE_HUNTER,
	// ROGUE
	//========
	MOD_GRAPPLE,
	MOD_BLUEBLASTER
};

struct mod_t
{
	mod_id_t	id;
	bool		friendly_fire = false;
	bool		no_point_loss = false;

	mod_t() = default;
	constexpr mod_t(mod_id_t id, bool no_point_loss = false) :
		id(id),
		no_point_loss(no_point_loss)
	{
	}
};

// the total number of levels we'll track for the
// end of unit screen.
constexpr size_t MAX_LEVELS_PER_UNIT = 8;

struct level_entry_t
{
	// bsp name
	char map_name[MAX_QPATH];
	// map name
	char pretty_name[MAX_QPATH];
	// these are set when we leave the level
	int32_t total_secrets;
	int32_t found_secrets;
	int32_t total_monsters;
	int32_t killed_monsters;
	// total time spent in the level, for end screen
	gtime_t time;
	// the order we visited levels in
	int32_t visit_order;
};

//
// this structure is left intact through an entire game
// it should be initialized at dll load time, and read/written to
// the server.ssv file for savegames
//
struct game_locals_t
{
	char	helpmessage1[MAX_TOKEN_CHARS];
	char	helpmessage2[MAX_TOKEN_CHARS];
	int32_t help1changed, help2changed;

	gclient_t *clients; // [maxclients]

	// can't store spawnpoint in level, because
	// it would get overwritten by the savegame restore
	char spawnpoint[MAX_TOKEN_CHARS]; // needed for coop respawns

	// store latched cvars here that we want to get at often
	uint32_t maxclients;
	uint32_t maxentities;

	// cross level triggers
	uint32_t cross_level_flags, cross_unit_flags;

	bool autosaved;

	// [Paril-KEX]
	int32_t airacceleration_modified, gravity_modified;
	std::array<level_entry_t, MAX_LEVELS_PER_UNIT> level_entries;
	int32_t max_lag_origins;
	vec3_t *lag_origins; // maxclients * max_lag_origins
};

constexpr size_t MAX_HEALTH_BARS = 2;

//
// this structure is cleared as each map is entered
// it is read/written to the level.sav file for savegames
//
struct level_locals_t
{
	bool in_frame;
	gtime_t time;

	char level_name[MAX_QPATH]; // the descriptive name (Outer Base, etc)
	char mapname[MAX_QPATH];	// the server name (base1, etc)
	char nextmap[MAX_QPATH];	// go here when fraglimit is hit
	char forcemap[MAX_QPATH];	// go here

	// intermission state
	gtime_t		intermissiontime; // time the intermission was started
	const char *changemap;
	const char *achievement;
	bool		exitintermission;
	bool		intermission_eou;
	bool		intermission_clear; // [Paril-KEX] clear inventory on switch
	bool		level_intermission_set; // [Paril-KEX] for target_camera switches; don't find intermission point
	bool		intermission_fade, intermission_fading; // [Paril-KEX] fade on exit instead of immediately leaving
	gtime_t		intermission_fade_time;
	vec3_t		intermission_origin;
	vec3_t		intermission_angle;
	bool		respawn_intermission; // only set once for respawning players

	int32_t pic_health;
	int32_t pic_ping;

	int32_t total_secrets;
	int32_t found_secrets;

	int32_t total_goals;
	int32_t found_goals;

	int32_t total_monsters;
	std::array<edict_t *, MAX_EDICTS> monsters_registered; // only for debug
	int32_t killed_monsters;

	edict_t *current_entity; // entity running from G_RunFrame
	int32_t	 body_que;		 // dead bodies

	int32_t power_cubes; // ugly necessity for coop

	// ROGUE
	edict_t *disguise_violator;
	gtime_t	 disguise_violation_time;
	int32_t  disguise_icon; // [Paril-KEX]
	// ROGUE
	
	int32_t shadow_light_count; // [Sam-KEX]
	bool is_n64;
	gtime_t coop_level_restart_time; // restart the level after this time
	bool instantitems; // instantitems 1 set in worldspawn

	// N64 goal stuff
	const char *goals; // nullptr if no goals in world
	int32_t goal_num; // current relative goal number, increased with each target_goal

	// offset for the first vwep model, for
	// skinnum encoding
	int32_t vwep_offset;

	// coop health scaling factor;
	// this percentage of health is added
	// to the monster's health per player.
	float coop_health_scaling;
	// this isn't saved in the save file, but stores
	// the amount of players currently active in the
	// level, compared against monsters' individual 
	// scale #
	int32_t coop_scale_players;

	// [Paril-KEX] current level entry
	level_entry_t *entry;

	// [Paril-KEX] current poi
	bool valid_poi;
	vec3_t current_poi;
	int32_t current_poi_image;
	int32_t current_poi_stage;
	edict_t *current_dynamic_poi;
	vec3_t *poi_points[MAX_SPLIT_PLAYERS]; // temporary storage for POIs in coop

	// start items
	const char *start_items;
	// disable grappling hook
	bool no_grapple;

	// saved gravity
	float gravity;
	// level is a hub map, and shouldn't be included in EOU stuff
	bool hub_map;
	// active health bar entities
	std::array<edict_t *, MAX_HEALTH_BARS> health_bar_entities;
	int32_t intermission_server_frame;
	bool deadly_kill_box;
	bool story_active;
	gtime_t next_auto_save;
	gtime_t next_match_report;
};

struct shadow_light_temp_t
{
	shadow_light_data_t data;
	const char	*lightstyletarget = nullptr;
};

void G_LoadShadowLights();

#include <unordered_set>

// spawn_temp_t is only used to hold entity field values that
// can be set from the editor, but aren't actualy present
// in edict_t during gameplay.
// defaults can/should be set in the struct.
struct spawn_temp_t
{
	// world vars
	const char	 *sky;
	float  skyrotate;
	vec3_t skyaxis;
	int32_t skyautorotate = 1;
	const char	 *nextmap;

	int32_t		lip;
	int32_t		distance;
	int32_t		height;
	const char	*noise;
	float		pausetime;
	const char	*item;
	const char	*gravity;

	float minyaw;
	float maxyaw;
	float minpitch;
	float maxpitch;

	shadow_light_temp_t sl; // [Sam-KEX]
	const char* music; // [Edward-KEX]
	int instantitems;
	float radius; // [Paril-KEX]
	bool hub_map; // [Paril-KEX]
	const char *achievement; // [Paril-KEX]

	// [Paril-KEX]
	const char *goals;

	// [Paril-KEX]
	const char *image;

	int fade_start_dist = 96;
	int fade_end_dist = 384;
	const char *start_items;
	int no_grapple = 0;
	float health_multiplier = 1.0f;

	const char *reinforcements; // [Paril-KEX]
	const char *noise_start, *noise_middle, *noise_end; // [Paril-KEX]
	int32_t loop_count; // [Paril-KEX]

	std::unordered_set<const char *> keys_specified;

	inline bool was_key_specified(const char *key) const
	{
		return keys_specified.find(key) != keys_specified.end();
	}
};

enum move_state_t
{
	STATE_TOP,
	STATE_BOTTOM,
	STATE_UP,
	STATE_DOWN
};

#define DEFINE_DATA_FUNC(ns_lower, ns_upper, returnType, ...) \
	using save_##ns_lower##_t = save_data_t<returnType(*)(__VA_ARGS__), SAVE_FUNC_##ns_upper>

#define SAVE_DATA_FUNC(n, ns, returnType, ...) \
	using save_##n##_t = save_data_t<returnType(*)(__VA_ARGS__), SAVE_FUNC_##ns>; \
	extern returnType n(__VA_ARGS__); \
	static const save_data_list_t save__##n(#n, SAVE_FUNC_##ns, reinterpret_cast<const void *>(n)); \
	auto n

DEFINE_DATA_FUNC(moveinfo_endfunc, MOVEINFO_ENDFUNC, void, edict_t *self);
#define MOVEINFO_ENDFUNC(n) \
	SAVE_DATA_FUNC(n, MOVEINFO_ENDFUNC, void, edict_t *self)

DEFINE_DATA_FUNC(moveinfo_blocked, MOVEINFO_BLOCKED, void, edict_t *self, edict_t *other);
#define MOVEINFO_BLOCKED(n) \
	SAVE_DATA_FUNC(n, MOVEINFO_BLOCKED, void, edict_t *self, edict_t *other)

// a struct that can store type-safe allocations
// of a fixed amount of data. it self-destructs when
// re-assigned. TODO: because edicts are still kind of
// managed like C memory, the destructor may not be
// called for a freed entity if this is stored as a member.
template<typename T, int32_t tag>
struct savable_allocated_memory_t
{
	T		*ptr;
	size_t	count;

	constexpr savable_allocated_memory_t(T *ptr, size_t count) :
		ptr(ptr),
		count(count)
	{
	}

	inline ~savable_allocated_memory_t()
	{
		release();
	}

	// no copy
	constexpr savable_allocated_memory_t(const savable_allocated_memory_t &) = delete;
	constexpr savable_allocated_memory_t &operator=(const savable_allocated_memory_t &) = delete;

	// free move
	constexpr savable_allocated_memory_t(savable_allocated_memory_t &&move)
	{
		ptr = move.ptr;
		count = move.count;

		move.ptr = nullptr;
		move.count = 0;
	}

	constexpr savable_allocated_memory_t &operator=(savable_allocated_memory_t &&move)
	{
		ptr = move.ptr;
		count = move.count;

		move.ptr = nullptr;
		move.count = 0;

		return *this;
	}

	inline void release()
	{
		if (ptr)
		{
			gi.TagFree(ptr);
			count = 0;
			ptr = nullptr;
		}
	}

	constexpr explicit operator T *() { return ptr; }
	constexpr explicit operator const T *() const { return ptr; }

	constexpr std::add_lvalue_reference_t<T> operator[](const size_t index) { return ptr[index]; }
	constexpr const std::add_lvalue_reference_t<T> operator[](const size_t index) const { return ptr[index]; }

	constexpr size_t size() const { return count * sizeof(T); }
	constexpr operator bool() const { return !!ptr; }
};

template<typename T, int32_t tag>
inline savable_allocated_memory_t<T, tag> make_savable_memory(size_t count)
{
	if (!count)
		return { nullptr, 0 };

	return { reinterpret_cast<T *>(gi.TagMalloc(sizeof(T) * count, tag)), count };
}

struct moveinfo_t
{
	// fixed data
	vec3_t start_origin;
	vec3_t start_angles;
	vec3_t end_origin;
	vec3_t end_angles, end_angles_reversed;

	int32_t sound_start;
	int32_t sound_middle;
	int32_t sound_end;

	float accel;
	float speed;
	float decel;
	float distance;

	float wait;

	// state data
	move_state_t state;
	bool		 reversing;
	vec3_t		 dir;
	vec3_t		 dest;
	float		 current_speed;
	float		 move_speed;
	float		 next_speed;
	float		 remaining_distance;
	float		 decel_distance;
	save_moveinfo_endfunc_t endfunc;
	save_moveinfo_blocked_t blocked;

	// [Paril-KEX] new accel state
	vec3_t  curve_ref;
	savable_allocated_memory_t<float, TAG_LEVEL> curve_positions;
	size_t	curve_frame;
	uint8_t	subframe, num_subframes;
	size_t  num_frames_done;
};

struct mframe_t
{
	void (*aifunc)(edict_t *self, float dist) = nullptr;
	float dist = 0;
	void (*thinkfunc)(edict_t *self) = nullptr;
	int32_t lerp_frame = -1;
};

// this check only works on windows, and is only
// of importance to developers anyways
#if defined(_WIN32) && defined(_MSC_VER)
#if _MSC_VER >= 1934
#define COMPILE_TIME_MOVE_CHECK
#endif
#endif

struct mmove_t
{
	int32_t	  firstframe;
	int32_t	  lastframe;
	const mframe_t *frame;
	void (*endfunc)(edict_t *self);
	float sidestep_scale;

#ifdef COMPILE_TIME_MOVE_CHECK
	template<size_t N>
	constexpr mmove_t(int32_t firstframe, int32_t lastframe, const mframe_t (&frames)[N], void (*endfunc)(edict_t *self) = nullptr, float sidestep_scale = 0.0f) :
		firstframe(firstframe),
		lastframe(lastframe),
		frame(frames),
		endfunc(endfunc),
		sidestep_scale(sidestep_scale)
	{
		if ((lastframe - firstframe + 1) != N)
			throw std::exception("bad animation frames; check your numbers!");
	}
#endif
};

using save_mmove_t = save_data_t<mmove_t, SAVE_DATA_MMOVE>;
#ifdef COMPILE_TIME_MOVE_CHECK
#define MMOVE_T(n) \
	extern const mmove_t n; \
	static const save_data_list_t save__##n(#n, SAVE_DATA_MMOVE, &n); \
	constexpr mmove_t n
#else
#define MMOVE_T(n) \
	extern const mmove_t n; \
	static const save_data_list_t save__##n(#n, SAVE_DATA_MMOVE, &n); \
	const mmove_t n
#endif

DEFINE_DATA_FUNC(monsterinfo_stand, MONSTERINFO_STAND, void, edict_t *self);
#define MONSTERINFO_STAND(n) \
	SAVE_DATA_FUNC(n, MONSTERINFO_STAND, void, edict_t *self)

DEFINE_DATA_FUNC(monsterinfo_idle, MONSTERINFO_IDLE, void, edict_t *self);
#define MONSTERINFO_IDLE(n) \
	SAVE_DATA_FUNC(n, MONSTERINFO_IDLE, void, edict_t *self)

DEFINE_DATA_FUNC(monsterinfo_search, MONSTERINFO_SEARCH, void, edict_t *self);
#define MONSTERINFO_SEARCH(n) \
	SAVE_DATA_FUNC(n, MONSTERINFO_SEARCH, void, edict_t *self)

DEFINE_DATA_FUNC(monsterinfo_walk, MONSTERINFO_WALK, void, edict_t *self);
#define MONSTERINFO_WALK(n) \
	SAVE_DATA_FUNC(n, MONSTERINFO_WALK, void, edict_t *self)

DEFINE_DATA_FUNC(monsterinfo_run, MONSTERINFO_RUN, void, edict_t *self);
#define MONSTERINFO_RUN(n) \
	SAVE_DATA_FUNC(n, MONSTERINFO_RUN, void, edict_t *self)

DEFINE_DATA_FUNC(monsterinfo_dodge, MONSTERINFO_DODGE, void, edict_t *self, edict_t *attacker, gtime_t eta, trace_t *tr, bool gravity);
#define MONSTERINFO_DODGE(n) \
	SAVE_DATA_FUNC(n, MONSTERINFO_DODGE, void, edict_t *self, edict_t *attacker, gtime_t eta, trace_t *tr, bool gravity)

DEFINE_DATA_FUNC(monsterinfo_attack, MONSTERINFO_ATTACK, void, edict_t *self);
#define MONSTERINFO_ATTACK(n) \
	SAVE_DATA_FUNC(n, MONSTERINFO_ATTACK, void, edict_t *self)

DEFINE_DATA_FUNC(monsterinfo_melee, MONSTERINFO_MELEE, void, edict_t *self);
#define MONSTERINFO_MELEE(n) \
	SAVE_DATA_FUNC(n, MONSTERINFO_MELEE, void, edict_t *self)

DEFINE_DATA_FUNC(monsterinfo_sight, MONSTERINFO_SIGHT, void, edict_t *self, edict_t *other);
#define MONSTERINFO_SIGHT(n) \
	SAVE_DATA_FUNC(n, MONSTERINFO_SIGHT, void, edict_t *self, edict_t *other)

DEFINE_DATA_FUNC(monsterinfo_checkattack, MONSTERINFO_CHECKATTACK, bool, edict_t *self);
#define MONSTERINFO_CHECKATTACK(n) \
	SAVE_DATA_FUNC(n, MONSTERINFO_CHECKATTACK, bool, edict_t *self)

DEFINE_DATA_FUNC(monsterinfo_setskin, MONSTERINFO_SETSKIN, void, edict_t *self);
#define MONSTERINFO_SETSKIN(n) \
	SAVE_DATA_FUNC(n, MONSTERINFO_SETSKIN, void, edict_t *self)

DEFINE_DATA_FUNC(monsterinfo_blocked, MONSTERINFO_BLOCKED, bool, edict_t *self, float dist);
#define MONSTERINFO_BLOCKED(n) \
	SAVE_DATA_FUNC(n, MONSTERINFO_BLOCKED, bool, edict_t *self, float dist)

DEFINE_DATA_FUNC(monsterinfo_physicschange, MONSTERINFO_PHYSCHANGED, void, edict_t *self);
#define MONSTERINFO_PHYSCHANGED(n) \
	SAVE_DATA_FUNC(n, MONSTERINFO_PHYSCHANGED, void, edict_t *self)

DEFINE_DATA_FUNC(monsterinfo_duck, MONSTERINFO_DUCK, bool, edict_t *self, gtime_t eta);
#define MONSTERINFO_DUCK(n) \
	SAVE_DATA_FUNC(n, MONSTERINFO_DUCK, bool, edict_t *self, gtime_t eta)

DEFINE_DATA_FUNC(monsterinfo_unduck, MONSTERINFO_UNDUCK, void, edict_t *self);
#define MONSTERINFO_UNDUCK(n) \
	SAVE_DATA_FUNC(n, MONSTERINFO_UNDUCK, void, edict_t *self)

DEFINE_DATA_FUNC(monsterinfo_sidestep, MONSTERINFO_SIDESTEP, bool, edict_t *self);
#define MONSTERINFO_SIDESTEP(n) \
	SAVE_DATA_FUNC(n, MONSTERINFO_SIDESTEP, bool, edict_t *self)

// combat styles, for navigation
enum combat_style_t
{
	COMBAT_UNKNOWN, // automatically choose based on attack functions
	COMBAT_MELEE, // should attempt to get up close for melee
	COMBAT_MIXED, // has mixed melee/ranged; runs to get up close if far enough away
	COMBAT_RANGED // don't bother pathing if we can see the player
};

struct reinforcement_t
{
	const char *classname;
	int32_t strength;
	vec3_t mins, maxs;
};

struct reinforcement_list_t
{
	reinforcement_t	*reinforcements;
	uint32_t		num_reinforcements;
};

constexpr size_t MAX_REINFORCEMENTS = 5; // max number of spawns we can do at once.

constexpr gtime_t HOLD_FOREVER = gtime_t::from_ms(std::numeric_limits<int64_t>::max());

struct monsterinfo_t
{
	// [Paril-KEX] allow some moves to be done instantaneously, but
	// others can wait the full frame.
	// NB: always use `M_SetAnimation` as it handles edge cases.
	save_mmove_t	   active_move, next_move;
	monster_ai_flags_t aiflags; // PGM - unsigned, since we're close to the max
	int32_t			   nextframe; // if next_move is set, this is ignored until a frame is ran
	float			   scale;

	save_monsterinfo_stand_t stand;
	save_monsterinfo_idle_t idle;
	save_monsterinfo_search_t search;
	save_monsterinfo_walk_t walk;
	save_monsterinfo_run_t run;
	save_monsterinfo_dodge_t dodge;
	save_monsterinfo_attack_t attack;
	save_monsterinfo_melee_t melee;
	save_monsterinfo_sight_t sight;
	save_monsterinfo_checkattack_t checkattack;
	save_monsterinfo_setskin_t setskin;
	save_monsterinfo_physicschange_t physics_change;

	gtime_t pausetime;
	gtime_t attack_finished;
	gtime_t fire_wait;

	vec3_t				   saved_goal;
	gtime_t				   search_time;
	gtime_t				   trail_time;
	vec3_t				   last_sighting;
	monster_attack_state_t attack_state;
	bool				   lefty;
	gtime_t				   idle_time;
	int32_t				   linkcount;

	item_id_t power_armor_type;
	int32_t	  power_armor_power;

	// for monster revive
	item_id_t initial_power_armor_type;
	int32_t	  max_power_armor_power;
	int32_t	  weapon_sound, engine_sound;

	// ROGUE
	save_monsterinfo_blocked_t blocked;
	gtime_t	 last_hint_time; // last time the monster checked for hintpaths.
	edict_t *goal_hint;		 // which hint_path we're trying to get to
	int32_t	 medicTries;
	edict_t *badMedic1, *badMedic2; // these medics have declared this monster "unhealable"
	edict_t *healer;				// this is who is healing this monster
	save_monsterinfo_duck_t duck;
	save_monsterinfo_unduck_t unduck;
	save_monsterinfo_sidestep_t sidestep;
	float	 base_height;
	gtime_t	 next_duck_time;
	gtime_t	 duck_wait_time;
	edict_t *last_player_enemy;
	// blindfire stuff .. the boolean says whether the monster will do it, and blind_fire_time is the timing
	// (set in the monster) of the next shot
	bool	blindfire;		// will the monster blindfire?
	bool    can_jump;       // will the monster jump?
	bool	had_visibility; // Paril: used for blindfire
	float	drop_height, jump_height;
	gtime_t blind_fire_delay;
	vec3_t	blind_fire_target;
	// used by the spawners to not spawn too much and keep track of #s of monsters spawned
	int32_t	 monster_slots; // nb: for spawned monsters, this is how many slots we took from our commander
	int32_t	 monster_used;
	edict_t *commander;
	// powerup timers, used by widow, our friend
	gtime_t quad_time;
	gtime_t invincible_time;
	gtime_t double_time;
	// ROGUE

	// Paril
	gtime_t	  surprise_time;
	item_id_t armor_type;
	int32_t	  armor_power;
	bool	  close_sight_tripped;
	gtime_t   melee_debounce_time; // don't melee until this time has passed
	gtime_t	  strafe_check_time; // time until we should reconsider strafing
	int32_t	  base_health; // health that we had on spawn, before any co-op adjustments
	int32_t   health_scaling; // number of players we've been scaled up to
	gtime_t   next_move_time; // high tick rate
	gtime_t	  bad_move_time; // don't try straight moves until this is over
	gtime_t	  bump_time; // don't slide against walls for a bit
	gtime_t	  random_change_time; // high tickrate
	gtime_t	  path_blocked_counter; // break out of paths when > a certain time
	gtime_t	  path_wait_time; // don't try nav nodes until this is over
	PathInfo  nav_path; // if AI_PATHING, this is where we are trying to reach
	gtime_t	  nav_path_cache_time; // cache nav_path result for this much time
	combat_style_t combat_style; // pathing style

	edict_t   *damage_attacker;
	edict_t   *damage_inflictor;
	int32_t   damage_blood, damage_knockback;
	vec3_t	  damage_from;
	mod_t	  damage_mod;

	// alternate flying mechanics
	float fly_max_distance, fly_min_distance; // how far we should try to stay
	float fly_acceleration; // accel/decel speed
	float fly_speed; // max speed from flying
	vec3_t fly_ideal_position; // ideally where we want to end up to hover, relative to our target if not pinned
	gtime_t fly_position_time; // if <= level.time, we can try changing positions
	bool fly_buzzard, fly_above; // orbit around all sides of their enemy, not just the sides
	bool fly_pinned; // whether we're currently pinned to ideal position (made absolute)
	bool fly_thrusters; // slightly different flight mechanics, for melee attacks
	gtime_t fly_recovery_time; // time to try a new dir to get away from hazards
	vec3_t fly_recovery_dir;

	gtime_t checkattack_time;
	int32_t start_frame;
	gtime_t dodge_time;
	int32_t move_block_counter;
	gtime_t move_block_change_time;
	gtime_t react_to_damage_time;

	reinforcement_list_t					reinforcements;
	std::array<uint8_t, MAX_REINFORCEMENTS>	chosen_reinforcements; // readied for spawn; 255 is value for none

	gtime_t jump_time;

	// NOTE: if adding new elements, make sure to add them
	// in g_save.cpp too!
};

// non-monsterinfo save stuff
using save_prethink_t = save_data_t<void(*)(edict_t *self), SAVE_FUNC_PRETHINK>;
#define PRETHINK(n) \
	void n(edict_t *self); \
	static const save_data_list_t save__##n(#n, SAVE_FUNC_PRETHINK, reinterpret_cast<const void *>(n)); \
	auto n

using save_think_t = save_data_t<void(*)(edict_t *self), SAVE_FUNC_THINK>;
#define THINK(n) \
	void n(edict_t *self); \
	static const save_data_list_t save__##n(#n, SAVE_FUNC_THINK, reinterpret_cast<const void *>(n)); \
	auto n

using save_touch_t = save_data_t<void(*)(edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self), SAVE_FUNC_TOUCH>;
#define TOUCH(n) \
	void n(edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self); \
	static const save_data_list_t save__##n(#n, SAVE_FUNC_TOUCH, reinterpret_cast<const void *>(n)); \
	auto n

using save_use_t = save_data_t<void(*)(edict_t *self, edict_t *other, edict_t *activator), SAVE_FUNC_USE>;
#define USE(n) \
	void n(edict_t *self, edict_t *other, edict_t *activator); \
	static const save_data_list_t save__##n(#n, SAVE_FUNC_USE, reinterpret_cast<const void *>(n)); \
	auto n

using save_pain_t = save_data_t<void(*)(edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod), SAVE_FUNC_PAIN>;
#define PAIN(n) \
	void n(edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod); \
	static const save_data_list_t save__##n(#n, SAVE_FUNC_PAIN, reinterpret_cast<const void *>(n)); \
	auto n

using save_die_t = save_data_t<void(*)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod), SAVE_FUNC_DIE>;
#define DIE(n) \
	void n(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod); \
	static const save_data_list_t save__##n(#n, SAVE_FUNC_DIE, reinterpret_cast<const void *>(n)); \
	auto n

// ROGUE
// this determines how long to wait after a duck to duck again.
// if we finish a duck-up, this gets cut in half.
constexpr gtime_t DUCK_INTERVAL = 5000_ms;
// ROGUE

extern game_locals_t  game;
extern level_locals_t level;
extern game_export_t  globals;
extern spawn_temp_t	  st;

extern edict_t *g_edicts;

#include <random>
extern std::mt19937 mt_rand;

// uniform float [0, 1)
[[nodiscard]] inline float frandom()
{
	return std::uniform_real_distribution<float>()(mt_rand);
}

// uniform float [min_inclusive, max_exclusive)
[[nodiscard]] inline float frandom(float min_inclusive, float max_exclusive)
{
	return std::uniform_real_distribution<float>(min_inclusive, max_exclusive)(mt_rand);
}

// uniform float [0, max_exclusive)
[[nodiscard]] inline float frandom(float max_exclusive)
{
	return std::uniform_real_distribution<float>(0, max_exclusive)(mt_rand);
}

// uniform time [min_inclusive, max_exclusive)
[[nodiscard]] inline gtime_t random_time(gtime_t min_inclusive, gtime_t max_exclusive)
{
	return gtime_t::from_ms(std::uniform_int_distribution<int64_t>(min_inclusive.milliseconds(), max_exclusive.milliseconds())(mt_rand));
}

// uniform time [0, max_exclusive)
[[nodiscard]] inline gtime_t random_time(gtime_t max_exclusive)
{
	return gtime_t::from_ms(std::uniform_int_distribution<int64_t>(0, max_exclusive.milliseconds())(mt_rand));
}

// uniform float [-1, 1)
// note: closed on min but not max
// to match vanilla behavior
[[nodiscard]] inline float crandom()
{
	return std::uniform_real_distribution<float>(-1.f, 1.f)(mt_rand);
}

// uniform float (-1, 1)
[[nodiscard]] inline float crandom_open()
{
	return std::uniform_real_distribution<float>(std::nextafterf(-1.f, 0.f), 1.f)(mt_rand);
}

// raw unsigned int32 value from random
[[nodiscard]] inline uint32_t irandom()
{
	return mt_rand();
}

// uniform int [min, max)
// always returns min if min == (max - 1)
// undefined behavior if min > (max - 1)
[[nodiscard]] inline int32_t irandom(int32_t min_inclusive, int32_t max_exclusive)
{
	if (min_inclusive == max_exclusive - 1)
		return min_inclusive;

	return std::uniform_int_distribution<int32_t>(min_inclusive, max_exclusive - 1)(mt_rand);
}

// uniform int [0, max)
// always returns 0 if max <= 0
// note for Q2 code:
// - to fix rand()%x, do irandom(x)
// - to fix rand()&x, do irandom(x + 1)
[[nodiscard]] inline int32_t irandom(int32_t max_exclusive)
{
	if (max_exclusive <= 0)
		return 0;

	return irandom(0, max_exclusive);
}

// uniform random index from given container
template<typename T>
[[nodiscard]] inline int32_t random_index(const T &container)
{
	return irandom(std::size(container));
}

// uniform random element from given container
template<typename T>
[[nodiscard]] inline auto random_element(T &container) -> decltype(*std::begin(container))
{
	return *(std::begin(container) + random_index(container));
}

// flip a coin
[[nodiscard]]inline bool brandom()
{
	return irandom(2) == 0;
}

extern cvar_t *deathmatch;
extern cvar_t *coop;
extern cvar_t *skill;
extern cvar_t *fraglimit;
extern cvar_t *timelimit;
// ZOID
extern cvar_t *capturelimit;
extern cvar_t *g_quick_weapon_switch;
extern cvar_t *g_instant_weapon_switch;
// ZOID
extern cvar_t *password;
extern cvar_t *spectator_password;
extern cvar_t *needpass;
extern cvar_t *g_select_empty;
extern cvar_t *sv_dedicated;

extern cvar_t *filterban;

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
extern cvar_t *g_debug_monster_paths;
extern cvar_t *g_debug_monster_kills;
extern cvar_t *maxspectators;

extern cvar_t *bot_debug_follow_actor;
extern cvar_t *bot_debug_move_to_point;

extern cvar_t *flood_msgs;
extern cvar_t *flood_persecond;
extern cvar_t *flood_waitdelay;

extern cvar_t *sv_maplist;

extern cvar_t *g_skipViewModifiers;

extern cvar_t *sv_stopspeed; // PGM - this was a define in g_phys.c

extern cvar_t *g_strict_saves;
extern cvar_t *g_coop_health_scaling;
extern cvar_t *g_weapon_respawn_time;

extern cvar_t* g_no_health;
extern cvar_t* g_no_items;
extern cvar_t* g_dm_weapons_stay;
extern cvar_t* g_dm_no_fall_damage;
extern cvar_t* g_dm_instant_items;
extern cvar_t* g_dm_same_level;
extern cvar_t* g_friendly_fire;
extern cvar_t* g_dm_force_respawn;
extern cvar_t* g_dm_force_respawn_time;
extern cvar_t* g_dm_spawn_farthest;
extern cvar_t* g_no_armor;
extern cvar_t* g_dm_allow_exit;
extern cvar_t* g_infinite_ammo;
extern cvar_t* g_dm_no_quad_drop;
extern cvar_t* g_dm_no_quadfire_drop;
extern cvar_t* g_no_mines;
extern cvar_t* g_dm_no_stack_double;
extern cvar_t* g_no_nukes;
extern cvar_t* g_no_spheres;
extern cvar_t* g_teamplay_armor_protect;
extern cvar_t* g_allow_techs;

extern cvar_t* g_start_items;
extern cvar_t* g_map_list;
extern cvar_t* g_map_list_shuffle;
extern cvar_t *g_lag_compensation;

// ROGUE
extern cvar_t *gamerules;
extern cvar_t *huntercam;
extern cvar_t *g_dm_strong_mines;
extern cvar_t *g_dm_random_items;
// ROGUE

// [Kex]
extern cvar_t* g_instagib;
extern cvar_t* g_coop_player_collision;
extern cvar_t* g_coop_squad_respawn;
extern cvar_t* g_coop_enable_lives;
extern cvar_t* g_coop_num_lives;
extern cvar_t* g_coop_instanced_items;
extern cvar_t* g_allow_grapple;
extern cvar_t* g_grapple_fly_speed;
extern cvar_t* g_grapple_pull_speed;
extern cvar_t* g_grapple_damage;

extern cvar_t *sv_airaccelerate;

extern cvar_t *g_damage_scale;
extern cvar_t *g_disable_player_collision;
extern cvar_t *ai_damage_scale;
extern cvar_t *ai_model_scale;
extern cvar_t *ai_allow_dm_spawn;
extern cvar_t *ai_movement_disabled;

#define world (&g_edicts[0])

uint32_t GetUnicastKey();

// item spawnflags
constexpr spawnflags_t SPAWNFLAG_ITEM_TRIGGER_SPAWN = 0x00000001_spawnflag;
constexpr spawnflags_t SPAWNFLAG_ITEM_NO_TOUCH = 0x00000002_spawnflag;
constexpr spawnflags_t SPAWNFLAG_ITEM_TOSS_SPAWN = 0x00000004_spawnflag;
constexpr spawnflags_t SPAWNFLAG_ITEM_MAX = 0x00000008_spawnflag;
// 8 bits reserved for editor flags & power cube bits
// (see SPAWNFLAG_NOT_EASY above)
constexpr spawnflags_t SPAWNFLAG_ITEM_DROPPED = 0x00010000_spawnflag;
constexpr spawnflags_t SPAWNFLAG_ITEM_DROPPED_PLAYER = 0x00020000_spawnflag;
constexpr spawnflags_t SPAWNFLAG_ITEM_TARGETS_USED = 0x00040000_spawnflag;

extern gitem_t itemlist[IT_TOTAL];

//
// g_cmds.c
//
bool CheckFlood(edict_t *ent);
void Cmd_Help_f(edict_t *ent);
void Cmd_Score_f(edict_t *ent);

//
// g_items.c
//
void	  PrecacheItem(gitem_t *it);
void	  InitItems();
void	  SetItemNames();
gitem_t	*FindItem(const char *pickup_name);
gitem_t	*FindItemByClassname(const char *classname);
edict_t	*Drop_Item(edict_t *ent, gitem_t *item);
void	  SetRespawn(edict_t *ent, gtime_t delay, bool hide_self = true);
void	  ChangeWeapon(edict_t *ent);
void	  SpawnItem(edict_t *ent, gitem_t *item);
void	  Think_Weapon(edict_t *ent);
item_id_t ArmorIndex(edict_t *ent);
item_id_t PowerArmorType(edict_t *ent);
gitem_t	*GetItemByIndex(item_id_t index);
gitem_t *GetItemByAmmo(ammo_t ammo);
gitem_t *GetItemByPowerup(powerup_t powerup);
bool	  Add_Ammo(edict_t *ent, gitem_t *item, int count);
void      G_CheckPowerArmor(edict_t *ent);
void	  Touch_Item(edict_t *ent, edict_t *other, const trace_t &tr, bool other_touching_self);
void	  droptofloor(edict_t *ent);
void      P_ToggleFlashlight(edict_t *ent, bool state);
bool      Entity_IsVisibleToPlayer(edict_t* ent, edict_t* player);
void      Compass_Update(edict_t *ent, bool first);

//
// g_utils.c
//
bool KillBox(edict_t *ent, bool from_spawning, mod_id_t mod = MOD_TELEFRAG, bool bsp_clipping = true);
edict_t *G_Find(edict_t *from, std::function<bool(edict_t *e)> matcher);

// utility template for getting the type of a field
template<typename>
struct member_object_type { };
template<typename T1, typename T2>
struct member_object_type<T1 T2::*> { using type = T1; };
template<typename T>
using member_object_type_t = typename member_object_type<std::remove_cv_t<T>>::type;

template<auto M>
edict_t *G_FindByString(edict_t *from, const std::string_view &value)
{
	static_assert(std::is_same_v<member_object_type_t<decltype(M)>, const char *>, "can only use string member functions");

	return G_Find(from, [&](edict_t *e) {
		return e->*M && strlen(e->*M) == value.length() && !Q_strncasecmp(e->*M, value.data(), value.length());
	});
}

edict_t *findradius(edict_t *from, const vec3_t &org, float rad);
edict_t *G_PickTarget(const char *targetname);
void	 G_UseTargets(edict_t *ent, edict_t *activator);
void	 G_PrintActivationMessage(edict_t *ent, edict_t *activator, bool coop_global);
void	 G_SetMovedir(vec3_t &angles, vec3_t &movedir);

void	 G_InitEdict(edict_t *e);
edict_t *G_Spawn();
void	 G_FreeEdict(edict_t *e);

void G_TouchTriggers(edict_t *ent);
void G_TouchProjectiles(edict_t *ent, vec3_t previous_origin);

char *G_CopyString(const char *in, int32_t tag);
char *G_CopyString(const char *in, size_t len, int32_t tag);

// ROGUE
edict_t *findradius2(edict_t *from, const vec3_t &org, float rad);
// ROGUE

void G_PlayerNotifyGoal(edict_t *player);

//
// g_spawn.c
//
void  ED_CallSpawn(edict_t *ent);
char *ED_NewString(char *string);

//
// g_target.c
//
void target_laser_think(edict_t *self);
void target_laser_off(edict_t *self);

constexpr spawnflags_t SPAWNFLAG_LASER_ON = 0x0001_spawnflag;
constexpr spawnflags_t SPAWNFLAG_LASER_RED = 0x0002_spawnflag;
constexpr spawnflags_t SPAWNFLAG_LASER_GREEN = 0x0004_spawnflag;
constexpr spawnflags_t SPAWNFLAG_LASER_BLUE = 0x0008_spawnflag;
constexpr spawnflags_t SPAWNFLAG_LASER_YELLOW = 0x0010_spawnflag;
constexpr spawnflags_t SPAWNFLAG_LASER_ORANGE = 0x0020_spawnflag;
constexpr spawnflags_t SPAWNFLAG_LASER_FAT = 0x0040_spawnflag;
constexpr spawnflags_t SPAWNFLAG_LASER_ZAP = 0x80000000_spawnflag;
constexpr spawnflags_t SPAWNFLAG_LASER_LIGHTNING = 0x10000_spawnflag;

constexpr spawnflags_t SPAWNFLAG_HEALTHBAR_PVS_ONLY = 1_spawnflag;

// damage flags
enum damageflags_t
{
	DAMAGE_NONE = 0,				   // no damage flags
	DAMAGE_RADIUS = 0x00000001,		   // damage was indirect
	DAMAGE_NO_ARMOR = 0x00000002,	   // armour does not protect from this damage
	DAMAGE_ENERGY = 0x00000004,		   // damage is from an energy based weapon
	DAMAGE_NO_KNOCKBACK = 0x00000008,  // do not affect velocity, just view angles
	DAMAGE_BULLET = 0x00000010,		   // damage is from a bullet (used for ricochets)
	DAMAGE_NO_PROTECTION = 0x00000020, // armor, shields, invulnerability, and godmode have no effect
									   // ROGUE
	DAMAGE_DESTROY_ARMOR = 0x00000040, // damage is done to armor and health.
	DAMAGE_NO_REG_ARMOR = 0x00000080,  // damage skips regular armor
	DAMAGE_NO_POWER_ARMOR = 0x00000100,// damage skips power armor
									   // ROGUE
	DAMAGE_NO_INDICATOR = 0x00000200   // for clients: no damage indicators
};

MAKE_ENUM_BITFLAGS(damageflags_t);

//
// g_combat.c
//
bool OnSameTeam(edict_t *ent1, edict_t *ent2);
bool CanDamage(edict_t *targ, edict_t *inflictor);
bool CheckTeamDamage(edict_t *targ, edict_t *attacker);
void T_Damage(edict_t *targ, edict_t *inflictor, edict_t *attacker, const vec3_t &dir, const vec3_t &point,
			  const vec3_t &normal, int damage, int knockback, damageflags_t dflags, mod_t mod);
void T_RadiusDamage(edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, damageflags_t dflags, mod_t mod);
void Killed(edict_t *targ, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, mod_t mod);

// ROGUE
void T_RadiusNukeDamage(edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, mod_t mod);
void T_RadiusClassDamage(edict_t *inflictor, edict_t *attacker, float damage, char *ignoreClass, float radius,
						 mod_t mod);
void cleanupHealTarget(edict_t *ent);
// ROGUE

constexpr int32_t DEFAULT_BULLET_HSPREAD = 300;
constexpr int32_t DEFAULT_BULLET_VSPREAD = 500;
constexpr int32_t DEFAULT_SHOTGUN_HSPREAD = 1000;
constexpr int32_t DEFAULT_SHOTGUN_VSPREAD = 500;
constexpr int32_t DEFAULT_DEATHMATCH_SHOTGUN_COUNT = 12;
constexpr int32_t DEFAULT_SHOTGUN_COUNT = 12;
constexpr int32_t DEFAULT_SSHOTGUN_COUNT = 20;

//
// g_func.c
//
void train_use(edict_t *self, edict_t *other, edict_t *activator);
void func_train_find(edict_t *self);
edict_t *plat_spawn_inside_trigger(edict_t *ent);
void	 Move_Calc(edict_t *ent, const vec3_t &dest, void(*endfunc)(edict_t *self));
void G_SetMoveinfoSounds(edict_t *self, const char *default_start, const char *default_mid, const char *default_end);

constexpr spawnflags_t SPAWNFLAG_TRAIN_START_ON = 1_spawnflag;

constexpr spawnflags_t SPAWNFLAG_WATER_SMART = 2_spawnflag;

constexpr spawnflags_t SPAWNFLAG_TRAIN_MOVE_TEAMCHAIN = 8_spawnflag;

constexpr spawnflags_t SPAWNFLAG_DOOR_REVERSE = 2_spawnflag;

//
// g_monster.c
//
void monster_muzzleflash(edict_t *self, const vec3_t &start, monster_muzzleflash_id_t id);
void monster_fire_bullet(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int kick, int hspread,
						 int vspread, monster_muzzleflash_id_t flashtype);
void monster_fire_shotgun(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int damage, int kick, int hspread,
						  int vspread, int count, monster_muzzleflash_id_t flashtype);
void monster_fire_blaster(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed,
						  monster_muzzleflash_id_t flashtype, effects_t effect);
void monster_fire_flechette(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed,
						    monster_muzzleflash_id_t flashtype);
void monster_fire_grenade(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int damage, int speed,
						  monster_muzzleflash_id_t flashtype, float right_adjust, float up_adjust);
void monster_fire_rocket(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed,
						 monster_muzzleflash_id_t flashtype);
void monster_fire_railgun(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int damage, int kick,
						  monster_muzzleflash_id_t flashtype);
void monster_fire_bfg(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int damage, int speed, int kick,
					  float damage_radius, monster_muzzleflash_id_t flashtype);
bool M_CheckClearShot(edict_t *self, const vec3_t &offset);
bool M_CheckClearShot(edict_t *self, const vec3_t &offset, vec3_t &start);
vec3_t M_ProjectFlashSource(edict_t *self, const vec3_t &offset, const vec3_t &forward, const vec3_t &right);
bool M_droptofloor_generic(vec3_t &origin, const vec3_t &mins, const vec3_t &maxs, bool ceiling, edict_t *ignore, contents_t mask, bool allow_partial);
bool M_droptofloor(edict_t *ent);
void monster_think(edict_t *self);
void monster_dead_think(edict_t *self);
void monster_dead(edict_t *self);
void walkmonster_start(edict_t *self);
void swimmonster_start(edict_t *self);
void flymonster_start(edict_t *self);
void monster_death_use(edict_t *self);
void M_CatagorizePosition(edict_t *self, const vec3_t &in_point, water_level_t &waterlevel, contents_t &watertype);
void M_WorldEffects(edict_t *ent);
bool M_CheckAttack(edict_t *self);
void M_CheckGround(edict_t *ent, contents_t mask);
void monster_use(edict_t *self, edict_t *other, edict_t *activator);
void M_ProcessPain(edict_t *e);
bool M_ShouldReactToPain(edict_t *self, const mod_t &mod);
void M_SetAnimation(edict_t *self, const save_mmove_t &move, bool instant = true);
bool M_AllowSpawn( edict_t * self );

// Paril: used in N64. causes them to be mad at the player
// regardless of circumstance.
constexpr size_t HACKFLAG_ATTACK_PLAYER = 1;
// used in N64, appears to change their behavior for the end scene.
constexpr size_t HACKFLAG_END_CUTSCENE = 4;

bool monster_start(edict_t *self);
void monster_start_go(edict_t *self);
// RAFAEL
void monster_fire_ionripper(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed,
							monster_muzzleflash_id_t flashtype, effects_t effect);
void monster_fire_heat(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed,
					   monster_muzzleflash_id_t flashtype, float lerp_factor);
void monster_fire_dabeam(edict_t *self, int damage, bool secondary, void(*update_func)(edict_t *self));
void dabeam_update(edict_t *self, bool damage);
void monster_fire_blueblaster(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed,
							  monster_muzzleflash_id_t flashtype, effects_t effect);
void G_Monster_CheckCoopHealthScaling();
// RAFAEL
// ROGUE
void monster_fire_blaster2(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed,
						   monster_muzzleflash_id_t flashtype, effects_t effect);
void monster_fire_tracker(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, edict_t *enemy,
						  monster_muzzleflash_id_t flashtype);
void monster_fire_heatbeam(edict_t *self, const vec3_t &start, const vec3_t &dir, const vec3_t &offset, int damage,
						   int kick, monster_muzzleflash_id_t flashtype);
void stationarymonster_start(edict_t *self);
void monster_done_dodge(edict_t *self);
// ROGUE

stuck_result_t G_FixStuckObject(edict_t *self, vec3_t check);

// this is for the count of monsters
int32_t M_SlotsLeft(edict_t *self);

// shared with monsters
constexpr spawnflags_t SPAWNFLAG_MONSTER_AMBUSH = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAG_MONSTER_TRIGGER_SPAWN = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAG_MONSTER_DEAD = 16_spawnflag_bit;
constexpr spawnflags_t SPAWNFLAG_MONSTER_SUPER_STEP = 17_spawnflag_bit;
constexpr spawnflags_t SPAWNFLAG_MONSTER_NO_DROP = 18_spawnflag_bit;
constexpr spawnflags_t SPAWNFLAG_MONSTER_SCENIC = 19_spawnflag_bit;

// fixbot spawnflags
constexpr spawnflags_t SPAWNFLAG_FIXBOT_FIXIT = 4_spawnflag;
constexpr spawnflags_t SPAWNFLAG_FIXBOT_TAKEOFF = 8_spawnflag;
constexpr spawnflags_t SPAWNFLAG_FIXBOT_LANDING = 16_spawnflag;
constexpr spawnflags_t SPAWNFLAG_FIXBOT_WORKING = 32_spawnflag;

//
// g_misc.c
//
void ThrowClientHead(edict_t *self, int damage);
void gib_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod);
edict_t *ThrowGib(edict_t *self, const char *gibname, int damage, gib_type_t type, float scale);
void BecomeExplosion1(edict_t *self);
void misc_viper_use(edict_t *self, edict_t *other, edict_t *activator);
void misc_strogg_ship_use(edict_t *self, edict_t *other, edict_t *activator);
void VelocityForDamage(int damage, vec3_t &v);
void ClipGibVelocity(edict_t *ent);

constexpr spawnflags_t SPAWNFLAG_PATH_CORNER_TELEPORT = 1_spawnflag;

constexpr spawnflags_t SPAWNFLAG_POINT_COMBAT_HOLD = 1_spawnflag;

// max chars for a clock string;
// " 0:00:00" is the longest string possible
// plus null terminator.
constexpr size_t CLOCK_MESSAGE_SIZE = 9;

//
// g_ai.c
//
edict_t *AI_GetSightClient(edict_t *self);

void ai_stand(edict_t *self, float dist);
void ai_move(edict_t *self, float dist);
void ai_walk(edict_t *self, float dist);
void ai_turn(edict_t *self, float dist);
void ai_run(edict_t *self, float dist);
void ai_charge(edict_t *self, float dist);

constexpr float RANGE_MELEE = 20; // bboxes basically touching
constexpr float RANGE_NEAR = 440;
constexpr float RANGE_MID = 940;

// [Paril-KEX] adjusted to return an actual distance, measured
// in a way that is consistent regardless of what is fighting what
float range_to(edict_t *self, edict_t *other);

void FoundTarget(edict_t *self);
void HuntTarget(edict_t *self, bool animate_state = true);
bool infront(edict_t *self, edict_t *other);
bool visible(edict_t *self, edict_t *other, bool through_glass = true);
bool FacingIdeal(edict_t *self);
// [Paril-KEX] generic function
bool M_CheckAttack_Base(edict_t *self, float stand_ground_chance, float melee_chance, float near_chance, float mid_chance, float far_chance, float strafe_scalar);

//
// g_weapon.c
//
bool fire_hit(edict_t *self, vec3_t aim, int damage, int kick);
void fire_bullet(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int damage, int kick, int hspread,
				 int vspread, mod_t mod);
void fire_shotgun(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int damage, int kick, int hspread,
				  int vspread, int count, mod_t mod);
void blaster_touch(edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self);
void fire_blaster(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int damage, int speed, effects_t effect,
				  mod_t mod);
void Grenade_Explode(edict_t *ent);
void fire_grenade(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int damage, int speed, gtime_t timer,
				  float damage_radius, float right_adjust, float up_adjust, bool monster);
void fire_grenade2(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int damage, int speed, gtime_t timer,
				   float damage_radius, bool held);
void rocket_touch(edict_t *ent, edict_t *other, const trace_t &tr, bool other_touching_self);
edict_t *fire_rocket(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, float damage_radius,
				 int radius_damage);
void fire_rail(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int damage, int kick);
void fire_bfg(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, float damage_radius);
// RAFAEL
void fire_ionripper(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int damage, int speed, effects_t effect);
void fire_heat(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, float damage_radius,
			   int radius_damage, float turn_fraction);
void fire_blueblaster(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int damage, int speed,
					  effects_t effect);
void fire_plasma(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, float damage_radius,
				 int radius_damage);
void fire_trap(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int speed);
// RAFAEL
void fire_disintegrator(edict_t *self, const vec3_t &start, const vec3_t &dir, int speed);
vec3_t P_CurrentKickAngles(edict_t *ent);
vec3_t P_CurrentKickOrigin(edict_t *ent);
void P_AddWeaponKick(edict_t *ent, const vec3_t &origin, const vec3_t &angles);

// we won't ever pierce more than this many entities for a single trace.
constexpr size_t MAX_PIERCE = 16;

// base class for pierce args; this stores
// the stuff we are piercing.
struct pierce_args_t 
{
	// stuff we pierced
	std::array<edict_t *, MAX_PIERCE> pierced;
	std::array<solid_t, MAX_PIERCE> pierce_solidities;
	size_t num_pierced = 0;
	// the last trace that was done, when piercing stopped
	trace_t tr;

	// mark entity as pierced
	inline bool mark(edict_t *ent);

	// restore entities' previous solidities
	inline void restore();

	// we hit an entity; return false to stop the piercing.
	// you can adjust the mask for the re-trace (for water, etc).
	virtual bool hit(contents_t &mask, vec3_t &end) = 0;

	virtual ~pierce_args_t()
	{
		restore();
	}
};

void pierce_trace(const vec3_t &start, const vec3_t &end, edict_t *ignore, pierce_args_t &pierce, contents_t mask);

//
// g_ptrail.c
//
void PlayerTrail_Add(edict_t *player);
void PlayerTrail_Destroy(edict_t *player);
edict_t *PlayerTrail_Pick(edict_t *self, bool next);

//
// g_client.c
//
constexpr spawnflags_t SPAWNFLAG_CHANGELEVEL_CLEAR_INVENTORY = 8_spawnflag;
constexpr spawnflags_t SPAWNFLAG_CHANGELEVEL_NO_END_OF_UNIT = 16_spawnflag;
constexpr spawnflags_t SPAWNFLAG_CHANGELEVEL_FADE_OUT = 32_spawnflag;
constexpr spawnflags_t SPAWNFLAG_CHANGELEVEL_IMMEDIATE_LEAVE = 64_spawnflag;

void respawn(edict_t *ent);
void BeginIntermission(edict_t *targ);
void PutClientInServer(edict_t *ent);
void InitClientPersistant(edict_t *ent, gclient_t *client);
void InitClientResp(gclient_t *client);
void InitBodyQue();
void ClientBeginServerFrame(edict_t *ent);
void ClientUserinfoChanged(edict_t *ent, const char *userinfo);
void P_AssignClientSkinnum(edict_t *ent);
void P_ForceFogTransition(edict_t *ent, bool instant);
void P_SendLevelPOI(edict_t *ent);
unsigned int P_GetLobbyUserNum( const edict_t * player );
void G_UpdateLevelEntry();
void G_EndOfUnitMessage();
bool	 SelectSpawnPoint(edict_t *ent, vec3_t &origin, vec3_t &angles, bool force_spawn, bool &landmark);

struct select_spawn_result_t
{
	edict_t		*spot;
	bool		any_valid = false; // set if a spawn point was found, even if it was taken
};

select_spawn_result_t SelectDeathmatchSpawnPoint(bool farthest, bool force_spawn, bool fallback_to_ctf_or_start);
void G_PostRespawn(edict_t *self);

//
// g_player.c
//
void player_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod);

//
// g_svcmds.c
//
void ServerCommand();
bool SV_FilterPacket(const char *from);

//
// p_view.c
//
void ClientEndServerFrame(edict_t *ent);
void G_LagCompensate(edict_t *from_player, const vec3_t &start, const vec3_t &dir);
void G_UnLagCompensate();

//
// p_hud.c
//
void MoveClientToIntermission(edict_t *client);
void G_SetStats(edict_t *ent);
void G_SetCoopStats(edict_t *ent);
void G_SetSpectatorStats(edict_t *ent);
void G_CheckChaseStats(edict_t *ent);
void ValidateSelectedItem(edict_t *ent);
void DeathmatchScoreboardMessage(edict_t *client, edict_t *killer);
void G_ReportMatchDetails(bool is_end);

//
// p_weapon.c
//
void PlayerNoise(edict_t *who, const vec3_t &where, player_noise_t type);
void P_ProjectSource(edict_t *ent, const vec3_t &angles, vec3_t distance, vec3_t &result_start, vec3_t &result_dir);
void NoAmmoWeaponChange(edict_t *ent, bool sound);
void G_RemoveAmmo(edict_t *ent);
void G_RemoveAmmo(edict_t *ent, int32_t quantity);
void Weapon_Generic(edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST,
					int FRAME_DEACTIVATE_LAST, const int *pause_frames, const int *fire_frames,
					void (*fire)(edict_t *ent));
void Weapon_Repeating(edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST,
					  int FRAME_DEACTIVATE_LAST, const int *pause_frames, void (*fire)(edict_t *ent));
void Throw_Generic(edict_t *ent, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_PRIME_SOUND,
				   const char *prime_sound, int FRAME_THROW_HOLD, int FRAME_THROW_FIRE, const int *pause_frames,
				   int EXPLODE, const char *primed_sound, void (*fire)(edict_t *ent, bool held), bool extra_idle_frame);
byte P_DamageModifier(edict_t *ent);
bool G_CheckInfiniteAmmo(gitem_t *item);
void Weapon_PowerupSound(edict_t *ent);

constexpr gtime_t GRENADE_TIMER = 3_sec;
constexpr float GRENADE_MINSPEED = 400.f;
constexpr float GRENADE_MAXSPEED = 800.f;

extern bool is_quad;
// RAFAEL
extern bool is_quadfire;
// RAFAEL
extern player_muzzle_t is_silenced;
// ROGUE
extern byte damage_multiplier;
// ROGUE

//
// m_move.c
//
bool M_CheckBottom_Fast_Generic(const vec3_t &absmins, const vec3_t &absmaxs, bool ceiling);
bool M_CheckBottom_Slow_Generic(const vec3_t &origin, const vec3_t &absmins, const vec3_t &absmaxs, edict_t *ignore, contents_t mask, bool ceiling, bool allow_any_step_height);
bool M_CheckBottom(edict_t *ent);
bool SV_CloseEnough(edict_t *ent, edict_t *goal, float dist);
bool M_walkmove(edict_t *ent, float yaw, float dist);
void M_MoveToGoal(edict_t *ent, float dist);
void M_ChangeYaw(edict_t *ent);
bool ai_check_move(edict_t *self, float dist);

//
// g_phys.c
//
constexpr float sv_friction = 6;
constexpr float sv_waterfriction = 1;

void G_RunEntity(edict_t *ent);
bool SV_RunThink(edict_t *ent);
void SV_AddRotationalFriction(edict_t *ent);
void SV_AddGravity(edict_t *ent);
void SV_CheckVelocity(edict_t *ent);
void	SV_FlyMove(edict_t *ent, float time, contents_t mask);
contents_t G_GetClipMask(edict_t *ent);
void G_Impact(edict_t *e1, const trace_t &trace);

//
// g_main.c
//
void SaveClientData();
void FetchClientEntData(edict_t *ent);
void EndDMLevel();

//
// g_chase.c
//
void UpdateChaseCam(edict_t *ent);
void ChaseNext(edict_t *ent);
void ChasePrev(edict_t *ent);
void GetChaseTarget(edict_t *ent);

//====================
// ROGUE PROTOTYPES
//
// g_newweap.c
//
void fire_flechette(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, int kick);
void fire_prox(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int damage, int speed);
void fire_nuke(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int speed);
bool fire_player_melee(edict_t *self, const vec3_t &start, const vec3_t &aim, int reach, int damage, int kick, mod_t mod);
void fire_tesla(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int damage, int speed);
void fire_blaster2(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int damage, int speed, effects_t effect,
				   bool hyper);
void fire_heatbeam(edict_t *self, const vec3_t &start, const vec3_t &aimdir, const vec3_t &offset, int damage, int kick,
				   bool monster);
void fire_tracker(edict_t *self, const vec3_t &start, const vec3_t &dir, int damage, int speed, edict_t *enemy);

//
// g_newai.c
//
bool	 blocked_checkplat(edict_t *self, float dist);

enum class blocked_jump_result_t
{
	NO_JUMP,
	JUMP_TURN,
	JUMP_JUMP_UP,
	JUMP_JUMP_DOWN
};

blocked_jump_result_t blocked_checkjump(edict_t *self, float dist);
bool	 monsterlost_checkhint(edict_t *self);
bool	 inback(edict_t *self, edict_t *other);
float	 realrange(edict_t *self, edict_t *other);
edict_t *SpawnBadArea(const vec3_t &mins, const vec3_t &maxs, gtime_t lifespan, edict_t *owner);
edict_t *CheckForBadArea(edict_t *ent);
bool	 MarkTeslaArea(edict_t *self, edict_t *tesla);
void	 InitHintPaths();
void PredictAim(edict_t *self, edict_t *target, const vec3_t &start, float bolt_speed, bool eye_height, float offset, vec3_t *aimdir,
				vec3_t *aimpoint);
bool M_CalculatePitchToFire(edict_t *self, const vec3_t &target, const vec3_t &start, vec3_t &aim, float speed, float time_remaining, bool mortar, bool destroy_on_touch = false);
bool below(edict_t *self, edict_t *other);
void drawbbox(edict_t *self);
void M_MonsterDodge(edict_t *self, edict_t *attacker, gtime_t eta, trace_t *tr, bool gravity);
void monster_duck_down(edict_t *self);
void monster_duck_hold(edict_t *self);
void monster_duck_up(edict_t *self);
bool has_valid_enemy(edict_t *self);
void TargetTesla(edict_t *self, edict_t *tesla);
void hintpath_stop(edict_t *self);
edict_t *PickCoopTarget(edict_t *self);
int		 CountPlayers();
bool	 monster_jump_finished(edict_t *self);
void BossExplode(edict_t *self);

// g_rogue_func
void plat2_spawn_danger_area(edict_t *ent);
void plat2_kill_danger_area(edict_t *ent);

// g_rogue_spawn
edict_t *CreateMonster(const vec3_t &origin, const vec3_t &angles, const char *classname);
edict_t *CreateFlyMonster(const vec3_t &origin, const vec3_t &angles, const vec3_t &mins, const vec3_t &maxs,
						  const char *classname);
edict_t *CreateGroundMonster(const vec3_t &origin, const vec3_t &angles, const vec3_t &mins, const vec3_t &maxs,
							 const char *classname, float height);
bool	 FindSpawnPoint(const vec3_t &startpoint, const vec3_t &mins, const vec3_t &maxs, vec3_t &spawnpoint,
						float maxMoveUp, bool drop = true);
bool	 CheckSpawnPoint(const vec3_t &origin, const vec3_t &mins, const vec3_t &maxs);
bool	 CheckGroundSpawnPoint(const vec3_t &origin, const vec3_t &entMins, const vec3_t &entMaxs, float height,
							   float gravity);
void	 SpawnGrow_Spawn(const vec3_t &startpos, float start_size, float end_size);
void	 Widowlegs_Spawn(const vec3_t &startpos, const vec3_t &angles);

// g_rogue_items
bool Pickup_Nuke(edict_t *ent, edict_t *other);
void Use_IR(edict_t *ent, gitem_t *item);
void Use_Double(edict_t *ent, gitem_t *item);
void Use_Nuke(edict_t *ent, gitem_t *item);
void Use_Doppleganger(edict_t *ent, gitem_t *item);
bool Pickup_Doppleganger(edict_t *ent, edict_t *other);
bool Pickup_Sphere(edict_t *ent, edict_t *other);
void Use_Defender(edict_t *ent, gitem_t *item);
void Use_Hunter(edict_t *ent, gitem_t *item);
void Use_Vengeance(edict_t *ent, gitem_t *item);
void Item_TriggeredSpawn(edict_t *self, edict_t *other, edict_t *activator);
void SetTriggeredSpawn(edict_t *ent);

//
// g_sphere.c
//
void Defender_Launch(edict_t *self);
void Vengeance_Launch(edict_t *self);
void Hunter_Launch(edict_t *self);

//
// g_newdm.c
//
void	 InitGameRules();
item_id_t DoRandomRespawn(edict_t *ent);
void	 PrecacheForRandomRespawn();
bool	 Tag_PickupToken(edict_t *ent, edict_t *other);
void	 Tag_DropToken(edict_t *ent, gitem_t *item);
void	 fire_doppleganger(edict_t *ent, const vec3_t &start, const vec3_t &aimdir);

//
// p_client.c
//
void RemoveAttackingPainDaemons(edict_t *self);
bool G_ShouldPlayersCollide(bool weaponry);
bool P_UseCoopInstancedItems();

constexpr spawnflags_t SPAWNFLAG_LANDMARK_KEEP_Z = 1_spawnflag;

// [Paril-KEX] convenience functions that returns true
// if the powerup should be 'active' (false to disable,
// will flash at 500ms intervals after 3 sec)
[[nodiscard]] constexpr bool G_PowerUpExpiringRelative(gtime_t left)
{
	return left.milliseconds() > 3000 || (left.milliseconds() % 1000) < 500;
}

[[nodiscard]] constexpr bool G_PowerUpExpiring(gtime_t time)
{
	return G_PowerUpExpiringRelative(time - level.time);
}

// ZOID
#include "ctf/g_ctf.h"
#include "ctf/p_ctf_menu.h"
// ZOID
//============================================================================

// client_t->anim_priority
enum anim_priority_t
{
	ANIM_BASIC, // stand / run
	ANIM_WAVE,
	ANIM_JUMP,
	ANIM_PAIN,
	ANIM_ATTACK,
	ANIM_DEATH,

	// flags
	ANIM_REVERSED	= bit_v<8>
};

MAKE_ENUM_BITFLAGS(anim_priority_t);

// height fog data values
struct height_fog_t
{
	// r g b dist
	std::array<float, 4> start;
	std::array<float, 4> end;
	float falloff;
	float density;

	inline bool operator==(const height_fog_t &o) const
	{
		return start == o.start && end == o.end && falloff == o.falloff && density == o.density;
	}
};

constexpr gtime_t SELECTED_ITEM_TIME = 3_sec;

enum bmodel_animstyle_t : int32_t
{
	BMODEL_ANIM_FORWARDS,
	BMODEL_ANIM_BACKWARDS,
	BMODEL_ANIM_RANDOM
};

struct bmodel_anim_t
{
	// range, inclusive
	int32_t				start, end;
	bmodel_animstyle_t	style;
	int32_t				speed; // in milliseconds
	bool				nowrap;

	int32_t				alt_start, alt_end;
	bmodel_animstyle_t	alt_style;
	int32_t				alt_speed; // in milliseconds
	bool				alt_nowrap;

	// game-only
	bool				enabled;
	bool				alternate, currently_alternate;
	gtime_t				next_tick;
};

// never turn back shield on automatically; this is
// the legacy behavior.
constexpr int32_t AUTO_SHIELD_MANUAL = -1;
// when it is >= 0, the shield will turn back on
// when we have that many cells in our inventory
// if possible.
constexpr int32_t AUTO_SHIELD_AUTO = 0;

// client data that stays across multiple level loads
struct client_persistant_t
{
	char		 userinfo[MAX_INFO_STRING];
	char		 social_id[MAX_INFO_VALUE];
	char		 netname[MAX_NETNAME];
	handedness_t hand;
	auto_switch_t autoswitch;
	int32_t autoshield; // see AUTO_SHIELD_*

	bool connected, spawned; // a loadgame will leave valid entities that
					// just don't have a connection yet

	// values saved and restored from edicts when changing levels
	int32_t		health;
	int32_t		max_health;
	ent_flags_t savedFlags;

	item_id_t selected_item;
	gtime_t   selected_item_time;
	std::array<int32_t, IT_TOTAL>	  inventory;

	// ammo capacities
	std::array<int16_t, AMMO_MAX> max_ammo;

	gitem_t *weapon;
	gitem_t *lastweapon;

	int32_t power_cubes; // used for tracking the cubes in coop games
	int32_t score;		 // for calculating total unit score in coop games

	int32_t game_help1changed, game_help2changed;
	int32_t helpchanged; // flash F1 icon if non 0, play sound
						 // and increment only if 1, 2, or 3
	gtime_t help_time;

	bool spectator; // client wants to be a spectator
	bool bob_skip; // [Paril-KEX] client wants no movement bob

	// [Paril-KEX] fog that we want to achieve; density rgb skyfogfactor
	std::array<float, 5> wanted_fog;
	height_fog_t wanted_heightfog;
	// relative time value, copied from last touched trigger
	gtime_t fog_transition_time;
	gtime_t megahealth_time; // relative megahealth time value
	int32_t lives; // player lives left (1 = no respawns remaining)
	uint8_t n64_crouch_warn_times;
	gtime_t n64_crouch_warning;
};

// client data that stays across deathmatch respawns
struct client_respawn_t
{
	client_persistant_t coop_respawn; // what to set client->pers to on a respawn
	gtime_t				entertime;	  // level.time the client entered the game
	int32_t				score;		  // frags, etc
	vec3_t				cmd_angles;	  // angles sent over in the last command

	bool spectator; // client is a spectator

	// ZOID
	ctfteam_t ctf_team; // CTF team
	int32_t	 ctf_state;
	gtime_t	 ctf_lasthurtcarrier;
	gtime_t	 ctf_lastreturnedflag;
	gtime_t	 ctf_flagsince;
	gtime_t	 ctf_lastfraggedcarrier;
	bool	 id_state;
	gtime_t	 lastidtime;
	bool	 voted; // for elections
	bool	 ready;
	bool	 admin;
	ghost_t *ghost; // for ghost codes
					// ZOID
};

// [Paril-KEX] seconds until we are fully invisible after
// making a racket
constexpr gtime_t INVISIBILITY_TIME = 2_sec;

// max number of individual damage indicators we'll track
constexpr size_t MAX_DAMAGE_INDICATORS = 4;

struct damage_indicator_t
{
	vec3_t from;
	int32_t health, armor, power;
};

// time between ladder sounds
constexpr gtime_t LADDER_SOUND_TIME = 300_ms;

// time after damage that we can't respawn on a player for
constexpr gtime_t COOP_DAMAGE_RESPAWN_TIME = 2000_ms;

// time after firing that we can't respawn on a player for
constexpr gtime_t COOP_DAMAGE_FIRING_TIME = 2500_ms;

// this structure is cleared on each PutClientInServer(),
// except for 'client->pers'
struct gclient_t
{
	// shared with server; do not touch members until the "private" section
	player_state_t ps; // communicated by server to clients
	int32_t		   ping;

	// private to game
	client_persistant_t pers;
	client_respawn_t	resp;
	pmove_state_t		old_pmove; // for detecting out-of-pmove changes

	bool showscores;	// set layout stat
	bool showeou;       // end of unit screen
	bool showinventory; // set layout stat
	bool showhelp;

	button_t buttons;
	button_t oldbuttons;
	button_t latched_buttons;
	usercmd_t cmd; // last CMD send

	// weapon cannot fire until this time is up
	gtime_t weapon_fire_finished;
	// time between processing individual animation frames
	gtime_t weapon_think_time;
	// if we latched fire between server frames but before
	// the weapon fire finish has elapsed, we'll "press" it
	// automatically when we have a chance
	bool weapon_fire_buffered;
	bool weapon_thunk;

	gitem_t *newweapon;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int32_t damage_armor;	  // damage absorbed by armor
	int32_t damage_parmor;	  // damage absorbed by power armor
	int32_t damage_blood;	  // damage taken out of health
	int32_t damage_knockback; // impact damage
	vec3_t	damage_from;	  // origin for vector calculation

	damage_indicator_t		  damage_indicators[MAX_DAMAGE_INDICATORS];
	uint8_t                   num_damage_indicators;

	float killer_yaw; // when dead, look at killer

	weaponstate_t weaponstate;
	struct {
		vec3_t	angles, origin;
		gtime_t	time, total;
	} kick;
	gtime_t		  quake_time;
	vec3_t		  kick_origin;
	float		  v_dmg_roll, v_dmg_pitch;
	gtime_t		  v_dmg_time; // damage kicks
	gtime_t		  fall_time;
	float		  fall_value; // for view drop on fall
	float		  damage_alpha;
	float		  bonus_alpha;
	vec3_t		  damage_blend;
	vec3_t		  v_angle, v_forward; // aiming direction
	float		  bobtime;			  // so off-ground doesn't change it
	vec3_t		  oldviewangles;
	vec3_t		  oldvelocity;
	edict_t		  *oldgroundentity; // [Paril-KEX]
	gtime_t		  flash_time; // [Paril-KEX] for high tickrate

	gtime_t		  next_drown_time;
	water_level_t old_waterlevel;
	int32_t		  breather_sound;

	int32_t machinegun_shots; // for weapon raising

	// animation vars
	int32_t			anim_end;
	anim_priority_t anim_priority;
	bool			anim_duck;
	bool			anim_run;
	gtime_t			anim_time;

	// powerup timers
	gtime_t quad_time;
	gtime_t invincible_time;
	gtime_t breather_time;
	gtime_t enviro_time;
	gtime_t invisible_time;

	bool	grenade_blew_up;
	gtime_t grenade_time, grenade_finished_time;
	// RAFAEL
	gtime_t quadfire_time;
	// RAFAEL
	int32_t silencer_shots;
	int32_t weapon_sound;

	gtime_t pickup_msg_time;

	gtime_t flood_locktill; // locked from talking
	gtime_t flood_when[10]; // when messages were said
	int32_t flood_whenhead; // head pointer for when said

	gtime_t respawn_time; // can respawn when time > this

	edict_t *chase_target; // player we are chasing
	bool	 update_chase; // need to update chase info?

	//=======
	// ROGUE
	gtime_t double_time;
	gtime_t ir_time;
	gtime_t nuke_time;
	gtime_t tracker_pain_time;

	edict_t *owned_sphere; // this points to the player's sphere
						   // ROGUE
	//=======

	gtime_t empty_click_sound;

	// ZOID
	bool		inmenu;	  // in menu
	pmenuhnd_t *menu;	  // current menu
	gtime_t		menutime; // time to update menu
	bool		menudirty;
	edict_t		*ctf_grapple;			// entity of grapple
	int32_t		ctf_grapplestate;		// true if pulling
	gtime_t		ctf_grapplereleasetime; // time of grapple release
	gtime_t		ctf_regentime;			// regen tech
	gtime_t		ctf_techsndtime;
	gtime_t		ctf_lasttechmsg;
	// ZOID

	// used for player trails.
	edict_t *trail_head, *trail_tail;
	// whether to use weapon chains
	bool no_weapon_chains;

	// seamless level transitions
	bool landmark_free_fall;
	const char* landmark_name;
	vec3_t landmark_rel_pos; // position relative to landmark, un-rotated from landmark angle
	gtime_t landmark_noise_time;

	gtime_t invisibility_fade_time; // [Paril-KEX] at this time, the player will be mostly fully cloaked
	gtime_t chase_msg_time; // to prevent CTF message spamming
	int32_t menu_sign; // menu sign
	vec3_t last_ladder_pos; // for ladder step sounds
	gtime_t last_ladder_sound;
	coop_respawn_t coop_respawn_state;
	gtime_t last_damage_time;

	// [Paril-KEX] these are now per-player, to work better in coop
	edict_t *sight_entity;
	gtime_t	 sight_entity_time;
	edict_t *sound_entity;
	gtime_t	 sound_entity_time;
	edict_t *sound2_entity;
	gtime_t  sound2_entity_time;
	// saved positions for lag compensation
	uint8_t	 num_lag_origins; // 0 to MAX_LAG_ORIGINS, how many we can go back
	uint8_t  next_lag_origin; // the next one to write to
	bool     is_lag_compensated;
	vec3_t	 lag_restore_origin;
	// for high tickrate weapon angles
	vec3_t	 slow_view_angles;
	gtime_t	 slow_view_angle_time;

	// not saved
	bool help_draw_points;
	size_t help_draw_index, help_draw_count;
	gtime_t help_draw_time;
	uint32_t step_frame;
	int32_t help_poi_image;
	vec3_t help_poi_location;

	// only set temporarily
	bool awaiting_respawn;
	gtime_t respawn_timeout; // after this time, force a respawn

	// [Paril-KEX] current active fog values; density rgb skyfogfactor
	std::array<float, 5> fog;
	height_fog_t heightfog;

	gtime_t	 last_attacker_time;
	// saved - for coop; last time we were in a firing state
	gtime_t	 last_firing_time;
};

// ==========================================
// PLAT 2
// ==========================================
enum plat2flags_t
{
	PLAT2_NONE = 0,
	PLAT2_CALLED = 1,
	PLAT2_MOVING = 2,
	PLAT2_WAITING = 4
};

MAKE_ENUM_BITFLAGS(plat2flags_t);

#include <bitset>

struct edict_t
{
	edict_t() = delete;
	edict_t(const edict_t &) = delete;
	edict_t(edict_t &&) = delete;

	// shared with server; do not touch members until the "private" section
	entity_state_t s;
	gclient_t	  *client; // nullptr if not a player
						   // the server expects the first part
						   // of gclient_t to be a player_state_t
						   // but the rest of it is opaque

	sv_entity_t sv;	       // read only info about this entity for the server

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

	//================================

	// private to game
	int32_t spawn_count; // [Paril-KEX] used to differentiate different entities that may be in the same slot
	movetype_t	movetype;
	ent_flags_t flags;

	const char *model;
	gtime_t		freetime; // sv.time when the object was freed

	//
	// only used locally in game, not by server
	//
	const char		*message;
	const char		*classname;
	spawnflags_t	spawnflags;

	gtime_t timestamp;

	float		angle; // set in qe3, -1 = up, -2 = down
	const char *target;
	const char *targetname;
	const char *killtarget;
	const char *team;
	const char *pathtarget;
	const char *deathtarget;
	const char *healthtarget;
	const char *itemtarget; // [Paril-KEX]
	const char *combattarget;
	edict_t	*target_ent;

	float  speed, accel, decel;
	vec3_t movedir;
	vec3_t pos1, pos2, pos3;

	vec3_t	velocity;
	vec3_t	avelocity;
	int32_t mass;
	gtime_t air_finished;
	float	gravity; // per entity gravity multiplier (1.0 is normal)
					 // use for lowgrav artifact, flares

	edict_t *goalentity;
	edict_t *movetarget;
	float	 yaw_speed;
	float	 ideal_yaw;

	gtime_t nextthink;
	save_prethink_t prethink;
	save_prethink_t postthink;
	save_think_t think;
	save_touch_t touch;
	save_use_t use;
	save_pain_t pain;
	save_die_t die;

	gtime_t touch_debounce_time; // are all these legit?  do we need more/less of them?
	gtime_t pain_debounce_time;
	gtime_t damage_debounce_time;
	gtime_t fly_sound_debounce_time; // move to clientinfo
	gtime_t last_move_time;

	int32_t		health;
	int32_t		max_health;
	int32_t		gib_health;
	gtime_t		show_hostile;

	gtime_t powerarmor_time;

	const char *map; // target_changelevel

	int32_t viewheight; // height above origin where eyesight is determined
	bool	deadflag;
	bool	takedamage;
	int32_t dmg;
	int32_t radius_dmg;
	float	dmg_radius;
	int32_t sounds; // make this a spawntemp var?
	int32_t count;

	edict_t *chain;
	edict_t *enemy;
	edict_t *oldenemy;
	edict_t *activator;
	edict_t *groundentity;
	int32_t	 groundentity_linkcount;
	edict_t *teamchain;
	edict_t *teammaster;

	edict_t *mynoise; // can go in client only
	edict_t *mynoise2;

	int32_t noise_index;
	int32_t noise_index2;
	float	volume;
	float	attenuation;

	// timing variables
	float wait;
	float delay; // before firing targets
	float random;

	gtime_t teleport_time;

	contents_t	  watertype;
	water_level_t waterlevel;

	vec3_t move_origin;
	vec3_t move_angles;

	int32_t style; // also used as areaportal number

	gitem_t *item; // for bonus items

	// common data blocks
	moveinfo_t	  moveinfo;
	monsterinfo_t monsterinfo;

	//=========
	// ROGUE
	plat2flags_t plat2flags;
	vec3_t		 offset;
	vec3_t		 gravityVector;
	edict_t		*bad_area;
	edict_t		*hint_chain;
	edict_t		*monster_hint_chain;
	edict_t		*target_hint_chain;
	int32_t		 hint_chain_id;
	// ROGUE
	//=========

	char clock_message[CLOCK_MESSAGE_SIZE];

	// Paril: we died on this frame, apply knockback even if we're dead
	gtime_t dead_time;
	// used for dabeam monsters
	edict_t *beam, *beam2;
	// proboscus for Parasite
	edict_t *proboscus;
	// for vooping things
	edict_t *disintegrator;
	gtime_t disintegrator_time;
	int32_t hackflags; // n64

	// fog stuff
	struct {
		vec3_t color;
		float density;
		float sky_factor;

		vec3_t color_off;
		float density_off;
		float sky_factor_off;
	} fog;

	struct {
		float falloff;
		float density;
		vec3_t start_color;
		float start_dist;
		vec3_t end_color;
		float end_dist;

		float falloff_off;
		float density_off;
		vec3_t start_color_off;
		float start_dist_off;
		vec3_t end_color_off;
		float end_dist_off;
	} heightfog;

	// instanced coop items
	std::bitset<MAX_CLIENTS>	item_picked_up_by;
	gtime_t						slime_debounce_time;

	// [Paril-KEX]
	bmodel_anim_t bmodel_anim;

	mod_t	lastMOD;
	const char	*style_on, *style_off;
	uint32_t crosslevel_flags;
	// NOTE: if adding new elements, make sure to add them
	// in g_save.cpp too!
};

//=============
// ROGUE
constexpr spawnflags_t SPHERE_DEFENDER = 0x0001_spawnflag;
constexpr spawnflags_t SPHERE_HUNTER = 0x0002_spawnflag;
constexpr spawnflags_t SPHERE_VENGEANCE = 0x0004_spawnflag;
constexpr spawnflags_t SPHERE_DOPPLEGANGER = 0x10000_spawnflag;

constexpr spawnflags_t SPHERE_TYPE = SPHERE_DEFENDER | SPHERE_HUNTER | SPHERE_VENGEANCE;
constexpr spawnflags_t SPHERE_FLAGS = SPHERE_DOPPLEGANGER;

//
// deathmatch games
//
enum
{
	RDM_TAG = 2,
	RDM_DEATHBALL = 3
};

struct dm_game_rt
{
	void (*GameInit)();
	void (*PostInitSetup)();
	void (*ClientBegin)(edict_t *ent);
	bool (*SelectSpawnPoint)(edict_t *ent, vec3_t &origin, vec3_t &angles, bool force_spawn);
	void (*PlayerDeath)(edict_t *targ, edict_t *inflictor, edict_t *attacker);
	void (*Score)(edict_t *attacker, edict_t *victim, int scoreChange, const mod_t &mod);
	void (*PlayerEffects)(edict_t *ent);
	void (*DogTag)(edict_t *ent, edict_t *killer, const char **pic);
	void (*PlayerDisconnect)(edict_t *ent);
	int (*ChangeDamage)(edict_t *targ, edict_t *attacker, int damage, mod_t mod);
	int (*ChangeKnockback)(edict_t *targ, edict_t *attacker, int knockback, mod_t mod);
	int (*CheckDMRules)();
};

extern dm_game_rt DMGame;

// ROGUE
//============


// [Paril-KEX]
inline void monster_footstep(edict_t *self)
{
	if (self->groundentity)
		self->s.event = EV_OTHER_FOOTSTEP;
}

// [Kex] helpers
// TFilter must be a type that is invokable with the
// signature bool(edict_t *); it must return true if
// the entity given is valid for the given filter
template<typename TFilter>
struct entity_iterator_t
{
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = edict_t *;
    using reference         = edict_t *;
    using pointer           = edict_t *;
    using difference_type   = ptrdiff_t;

private:
	uint32_t index;
	uint32_t end_index; // where the end index is located for this iterator
	// index < globals.num_edicts are valid
	TFilter filter;

	// this doubles as the "end" iterator
	inline bool is_out_of_range(uint32_t i) const
	{
		return i >= end_index;
	}

	inline bool is_out_of_range() const
	{
		return is_out_of_range(index);
	}

	inline void throw_if_out_of_range() const
	{
		if (is_out_of_range())
			throw std::out_of_range("index");
	}

	inline difference_type clamped_index() const
	{
		if (is_out_of_range())
			return end_index;

		return index;
	}

public:
	// note: index is not affected by filter. it is up to
	// the caller to ensure this index is filtered.
	constexpr entity_iterator_t(uint32_t i, uint32_t end_index = -1) : index(i), end_index((end_index >= globals.num_edicts) ? globals.num_edicts : end_index) { }
	
	inline reference operator*() { throw_if_out_of_range(); return &g_edicts[index]; }
	inline pointer operator->() { throw_if_out_of_range(); return &g_edicts[index]; }
	
	inline entity_iterator_t &operator++()
	{
		throw_if_out_of_range();
		return *this = *this + 1;
	}

	inline entity_iterator_t &operator--()
	{
		throw_if_out_of_range();
		return *this = *this - 1;
	}

	inline difference_type operator-(const entity_iterator_t &it) const
	{
		return clamped_index() - it.clamped_index();
	}

	inline entity_iterator_t operator+(const difference_type &offset) const
	{
		entity_iterator_t it(index + offset, end_index);
		
		// move in the specified direction, only stopping if we
		// run out of range or find a filtered entity
		while (!is_out_of_range(it.index) && !filter(*it))
			it.index += offset > 0 ? 1 : -1;

		return it;
	}

	// + -1 and - 1 are the same (and - -1 & + 1)
	inline entity_iterator_t operator-(const difference_type &offset) const { return *this + (-offset); }
	
	// comparison. hopefully this won't break anything, but == and != use the
	// clamped index (so -1 and num_edicts will be equal technically since they
	// are the same "invalid" entity) but <= and >= will affect them properly.
	inline bool operator==(const entity_iterator_t &it) const { return clamped_index() == it.clamped_index(); }
	inline bool operator!=(const entity_iterator_t &it) const { return clamped_index() != it.clamped_index(); }
	inline bool operator<(const entity_iterator_t &it) const { return index < it.index; }
	inline bool operator>(const entity_iterator_t &it) const { return index > it.index; }
	inline bool operator<=(const entity_iterator_t &it) const { return index <= it.index; }
	inline bool operator>=(const entity_iterator_t &it) const { return index >= it.index; }

	inline edict_t *operator[](const difference_type &offset) const { return *(*this + offset); }
};

// iterate over range of entities, with the specified filter.
// can be "open-ended" (automatically expand with num_edicts)
// by leaving the max unset.
template<typename TFilter>
struct entity_iterable_t
{
private:
	uint32_t begin_index, end_index;
	TFilter filter;

	// find the first entity that matches the filter, from the specified index,
	// in the specified direction
	inline uint32_t find_matched_index(uint32_t index, int32_t direction)
	{
		while (index < globals.num_edicts && !filter(&g_edicts[index]))
			index += direction;

		return index;
	}

public:
	// iterate all allocated entities that match the filter,
	// including ones allocated after this iterator is constructed
	inline entity_iterable_t<TFilter>() : begin_index(find_matched_index(0, 1)), end_index(game.maxentities) { }
	// iterate all allocated entities that match the filter from the specified begin offset
	// including ones allocated after this iterator is constructed
	inline entity_iterable_t<TFilter>(uint32_t start) : begin_index(find_matched_index(start, 1)), end_index(game.maxentities) { }
	// iterate all allocated entities that match the filter from the specified begin offset
	// to the specified INCLUSIVE end offset (or the first entity that matches before it),
	// including end itself but not ones that may appear after this iterator is done
	inline entity_iterable_t<TFilter>(uint32_t start, uint32_t end) :
		begin_index(find_matched_index(start, 1)),
		end_index(find_matched_index(end, -1) + 1)
	{
	}

	inline entity_iterator_t<TFilter> begin() const { return entity_iterator_t<TFilter>(begin_index, end_index); }
	inline entity_iterator_t<TFilter> end() const { return end_index; }
};

// inuse players that are connected; may not be spawned yet, however
struct active_players_filter_t
{
	inline bool operator()(edict_t *ent) const
	{
		return (ent->inuse && ent->client && ent->client->pers.connected);
	}
};

inline entity_iterable_t<active_players_filter_t> active_players()
{
	return entity_iterable_t<active_players_filter_t> { 1u, game.maxclients };
}

struct gib_def_t
{
	size_t count;
	const char *gibname;
	float scale;
	gib_type_t type;

	constexpr gib_def_t(size_t count, const char *gibname) :
		count(count),
		gibname(gibname),
		scale(1.0f),
		type(GIB_NONE)
	{
	}

	constexpr gib_def_t(size_t count, const char *gibname, gib_type_t type) :
		count(count),
		gibname(gibname),
		scale(1.0f),
		type(type)
	{
	}

	constexpr gib_def_t(size_t count, const char *gibname, float scale) :
		count(count),
		gibname(gibname),
		scale(scale),
		type(GIB_NONE)
	{
	}

	constexpr gib_def_t(size_t count, const char *gibname, float scale, gib_type_t type) :
		count(count),
		gibname(gibname),
		scale(scale),
		type(type)
	{
	}

	constexpr gib_def_t(const char *gibname, float scale, gib_type_t type) :
		count(1),
		gibname(gibname),
		scale(scale),
		type(type)
	{
	}

	constexpr gib_def_t(const char *gibname, float scale) :
		count(1),
		gibname(gibname),
		scale(scale),
		type(GIB_NONE)
	{
	}

	constexpr gib_def_t(const char *gibname, gib_type_t type) :
		count(1),
		gibname(gibname),
		scale(1.0f),
		type(type)
	{
	}

	constexpr gib_def_t(const char *gibname) :
		count(1),
		gibname(gibname),
		scale(1.0f),
		type(GIB_NONE)
	{
	}
};

// convenience function to throw different gib types
// NOTE: always throw the head gib *last* since self's size is used
// to position the gibs!
inline void ThrowGibs(edict_t *self, int32_t damage, std::initializer_list<gib_def_t> gibs)
{
	for (auto &gib : gibs)
		for (size_t i = 0; i < gib.count; i++)
			ThrowGib(self, gib.gibname, damage, gib.type, gib.scale * (self->s.scale ? self->s.scale : 1));
}

inline bool M_CheckGib(edict_t *self, const mod_t &mod)
{
	if (self->deadflag)
	{
		if (mod.id == MOD_CRUSH)
			return true;
	}

	return self->health <= self->gib_health;
}

// Fmt support for entities
template<>
struct fmt::formatter<edict_t>
{
	template<typename ParseContext>
	constexpr auto parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

    template<typename FormatContext>
    auto format(const edict_t &p, FormatContext &ctx) -> decltype(ctx.out())
    {
		if (p.linked)
			return fmt::format_to(ctx.out(), FMT_STRING("{} @ {}"), p.classname, (p.absmax + p.absmin) * 0.5f);
		return fmt::format_to(ctx.out(), FMT_STRING("{} @ {}"), p.classname, p.s.origin);
    }
};

// POI tags used by this mod
enum pois_t : uint16_t
{
	POI_OBJECTIVE = MAX_EDICTS, // current objective
	POI_RED_FLAG, // red flag/carrier
	POI_BLUE_FLAG, // blue flag/carrier
	POI_PING,
	POI_PING_END = POI_PING + MAX_CLIENTS - 1,
};

// implementation of pierce stuff
inline bool pierce_args_t::mark(edict_t *ent)
{
	// ran out of pierces
	if (num_pierced == MAX_PIERCE)
		return false;

	pierced[num_pierced] = ent;
	pierce_solidities[num_pierced] = ent->solid;
	num_pierced++;

	ent->solid = SOLID_NOT;
	gi.linkentity(ent);

	return true;
}

// implementation of pierce stuff
inline void pierce_args_t::restore()
{
	for (size_t i = 0; i < num_pierced; i++)
	{
		auto &ent = pierced[i];
		ent->solid = pierce_solidities[i];
		gi.linkentity(ent);
	}

	num_pierced = 0;
}

// [Paril-KEX] these are to fix a legacy bug with cached indices
// in save games. these can *only* be static/globals!
template<auto T>
struct cached_assetindex
{
	static cached_assetindex<T> *head;

	const char				*name;
	int32_t					index = 0;
	cached_assetindex		*next = nullptr;

	inline cached_assetindex()
	{
		next = head;
		cached_assetindex<T>::head = this;
	}
	constexpr operator int32_t() const { return index; }

	// assigned from spawn functions
	inline void assign(const char *name) { this->name = name; index = (gi.*T)(name); }
	// cleared before SpawnEntities
	constexpr void clear() { index = 0; }
	// re-find the index for the given cached entry, if we were cached
	// by the regular map load
	inline void reset() { if (index) index = (gi.*T)(this->name); }

	static void reset_all()
	{
		auto asset = head;

		while (asset)
		{
			asset->reset();
			asset = asset->next;
		}
	}

	static void clear_all()
	{
		auto asset = head;

		while (asset)
		{
			asset->clear();
			asset = asset->next;
		}
	}
};

using cached_soundindex = cached_assetindex<&local_game_import_t::soundindex>;
using cached_modelindex = cached_assetindex<&local_game_import_t::modelindex>;
using cached_imageindex = cached_assetindex<&local_game_import_t::imageindex>;

template<> cached_soundindex *cached_soundindex::head;
template<> cached_modelindex *cached_modelindex::head;
template<> cached_imageindex *cached_imageindex::head;

extern cached_modelindex sm_meat_index;
extern cached_soundindex snd_fry;