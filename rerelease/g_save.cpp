// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include <sstream>

#include "g_local.h"
#include <float.h>
#ifdef __clang__
#pragma clang diagnostic push
#pragma GCC diagnostic ignored "-Weverything"
#endif
#include "json/json.h"
#include "json/config.h"
#ifdef __clang__
#pragma clang diagnostic pop
#endif

// new save format;
// - simple JSON format
// - work via 'type' definitions which declare the type
//   that is at the specified offset of a struct
// - backwards & forwards compatible with this same format
// - I wrote this initially when the codebase was in C, so it
//   does have some C-isms in here.
constexpr size_t SAVE_FORMAT_VERSION = 1;

#include <unordered_map>

// Professor Daniel J. Bernstein; https://www.partow.net/programming/hashfunctions/#APHashFunction MIT
struct cstring_hash
{
	inline std::size_t operator()(const char *str) const
	{
		size_t   len  = strlen(str);
		uint32_t hash = 5381;
		uint32_t i    = 0;

		for (i = 0; i < len; ++str, ++i)
			hash = ((hash << 5) + hash) + (*str);

		return hash;
	}
};

struct cstring_equal
{
	inline bool operator()(const char *a, const char *b) const
	{
		return strcmp(a, b) == 0;
	}
};

struct ptr_tag_hash
{
	inline std::size_t operator()(const std::tuple<const void *, save_data_tag_t> &v) const
	{
		return (std::hash<const void *>()(std::get<0>(v)) * 8747) + std::get<1>(v);
	}
};

static bool save_data_initialized = false;
static const save_data_list_t *list_head = nullptr;
static std::unordered_map<const void *, const save_data_list_t *> list_hash;
static std::unordered_map<const char *, const save_data_list_t *, cstring_hash, cstring_equal> list_str_hash;
static std::unordered_map<std::tuple<const void *, save_data_tag_t>, const save_data_list_t *, ptr_tag_hash> list_from_ptr_hash; 

#include <cassert>

void InitSave()
{
	if (save_data_initialized)
		return;

	for (const save_data_list_t *link = list_head; link; link = link->next)
	{
		const void *link_ptr = link;

		if (list_hash.find(link_ptr) != list_hash.end())
		{
			auto existing = *list_hash.find(link_ptr);

			// [0] is just to silence warning
			assert(false || "invalid save pointer; break here to find which pointer it is"[0]);

			if (g_strict_saves->integer)
				gi.Com_ErrorFmt("link pointer {} already linked as {}; fatal error", link_ptr, existing.second->name);
			else
				gi.Com_PrintFmt("link pointer {} already linked as {}; fatal error", link_ptr, existing.second->name);
		}

		if (list_str_hash.find(link->name) != list_str_hash.end())
		{
			auto existing = *list_str_hash.find(link->name);

			// [0] is just to silence warning
			assert(false || "invalid save pointer; break here to find which pointer it is"[0]);

			if (g_strict_saves->integer)
				gi.Com_ErrorFmt("link pointer {} already linked as {}; fatal error", link_ptr, existing.second->name);
			else
				gi.Com_PrintFmt("link pointer {} already linked as {}; fatal error", link_ptr, existing.second->name);
		}

		list_hash.emplace(link_ptr, link);
		list_str_hash.emplace(link->name, link);
		list_from_ptr_hash.emplace(std::make_tuple(link->ptr, link->tag), link);
	}

	save_data_initialized = true;
}

// initializer for save data
save_data_list_t::save_data_list_t(const char *name_in, save_data_tag_t tag_in, const void *ptr_in) :
	name(name_in),
	tag(tag_in),
	ptr(ptr_in)
{
	if (save_data_initialized)
		gi.Com_Error("attempted to create save_data_list at runtime");

	next = list_head;
	list_head = this;
}

const save_data_list_t *save_data_list_t::fetch(const void *ptr, save_data_tag_t tag)
{
	auto link = list_from_ptr_hash.find(std::make_tuple(ptr, tag));

	if (link != list_from_ptr_hash.end() && link->second->tag == tag)
		return link->second;

	// [0] is just to silence warning
	assert(false || "invalid save pointer; break here to find which pointer it is"[0]);
	
	if (g_strict_saves->integer)
		gi.Com_ErrorFmt("value pointer {} was not linked to save tag {}", ptr, (int32_t) tag);
	else
		gi.Com_PrintFmt("value pointer {} was not linked to save tag {}", ptr, (int32_t) tag);

	return nullptr;
}

std::string json_error_stack;

void json_push_stack(const std::string &stack)
{
	json_error_stack += "::" + stack;
}

void json_pop_stack()
{
	size_t o = json_error_stack.find_last_of("::");

	if (o != std::string::npos)
		json_error_stack.resize(o - 1);
}

void json_print_error(const char *field, const char *message, bool fatal)
{
	if (fatal || g_strict_saves->integer)
		gi.Com_ErrorFmt("Error loading JSON\n{}.{}: {}", json_error_stack, field, message);

	gi.Com_PrintFmt("Warning loading JSON\n{}.{}: {}\n", json_error_stack, field, message);
}

using save_void_t = save_data_t<void, UINT_MAX>;

enum save_type_id_t
{
	// never valid
	ST_INVALID,

	// integral types
	ST_BOOL,
	ST_INT8,
	ST_INT16,
	ST_INT32,
	ST_INT64,
	ST_UINT8,
	ST_UINT16,
	ST_UINT32,
	ST_UINT64,
	ST_ENUM, // "count" = sizeof(enum_type)

	// floating point
	ST_FLOAT,
	ST_DOUBLE,

	ST_STRING, // "tag" = memory tag, "count" = fixed length if specified, otherwise dynamic

	ST_FIXED_STRING, // "count" = length

	ST_FIXED_ARRAY,
	// for simple types (ones that don't require extra info), "tag" = type and "count" = N
	// otherwise, "type_resolver" for nested arrays, structures, string/function arrays, etc

	ST_STRUCT, // "count" = sizeof(struct), "structure" = ptr to save_struct_t to save

	ST_BITSET, // bitset; "count" = N bits

	// special Quake types
	ST_ENTITY,		 // serialized as s.number
	ST_ITEM_POINTER, // serialized as classname
	ST_ITEM_INDEX,	 // serialized as classname
	ST_TIME,		 // serialized as milliseconds
	ST_DATA,		 // serialized as name of data ptr from global list; `tag` = list tag
	ST_INVENTORY,	 // serialized as classname => number key/value pairs
	ST_REINFORCEMENTS, // serialized as array of data
	ST_SAVABLE_DYNAMIC // serialized similar to ST_FIXED_ARRAY but includes count
};

struct save_struct_t;

struct save_type_t
{
	save_type_id_t id;
	int32_t		   tag = 0;
	size_t		   count = 0;
	save_type_t (*type_resolver)() = nullptr;
	const save_struct_t *structure = nullptr;
	bool				 never_empty = false;	  // this should be persisted even if all empty
	bool (*is_empty)(const void *data) = nullptr; // override default check

	void (*read)(void *data, const Json::Value &json, const char *field) = nullptr; // for custom reading
	bool (*write)(const void *data, bool null_for_empty, Json::Value &output) = nullptr; // for custom writing
};

struct save_field_t
{
	const char *name;
	size_t		offset;
	save_type_t type;

	// for easily chaining with FIELD_AUTO
	constexpr save_field_t &set_is_empty(decltype(save_type_t::is_empty) empty)
	{
		type.is_empty = empty;
		return *this;
	}
};

struct save_struct_t
{
	const char							   *name;
	const std::initializer_list<save_field_t> fields; // field list

	std::string debug() const
	{
		std::stringstream s;

		for (auto &field : fields)
			s << field.name << " " << field.offset << " " << field.type.id << " " << field.type.tag << " "
			  << field.type.count << '\n';

		return s.str();
	}
};

// field header macro
#define SAVE_FIELD(n, f) #f, offsetof(n, f)

// save struct header macro
#define INTERNAL_SAVE_STRUCT_START2(struct_name) static const save_struct_t struct_name##_savestruct = { #struct_name, {
#define INTERNAL_SAVE_STRUCT_START1(struct_name) INTERNAL_SAVE_STRUCT_START2(struct_name)
#define SAVE_STRUCT_START INTERNAL_SAVE_STRUCT_START1(DECLARE_SAVE_STRUCT)

#define SAVE_STRUCT_END                                                                                                \
	}                                                                                                                  \
	};

// field header macro for current struct
#define FIELD(f) SAVE_FIELD(DECLARE_SAVE_STRUCT, f)

template<typename T, typename T2 = void>
struct save_type_deducer
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		static_assert(
			!std::is_same_v<T2, void>,
			"Can't automatically deduce type for save; implement save_type_deducer or use an explicit FIELD_ macro.");
		return {};
	}
};

// bool
template<>
struct save_type_deducer<bool>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		return { name, offset, { ST_BOOL } };
	}
};

// integral
template<>
struct save_type_deducer<int8_t>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		return { name, offset, { ST_INT8 } };
	}
};
template<>
struct save_type_deducer<int16_t>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		return { name, offset, { ST_INT16 } };
	}
};
template<>
struct save_type_deducer<int32_t>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		return { name, offset, { ST_INT32 } };
	}
};
template<>
struct save_type_deducer<int64_t>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		return { name, offset, { ST_INT64 } };
	}
};
template<>
struct save_type_deducer<uint8_t>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		return { name, offset, { ST_UINT8 } };
	}
};
template<>
struct save_type_deducer<uint16_t>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		return { name, offset, { ST_UINT16 } };
	}
};
template<>
struct save_type_deducer<uint32_t>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		return { name, offset, { ST_UINT32 } };
	}
};
template<>
struct save_type_deducer<uint64_t>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		return { name, offset, { ST_UINT64 } };
	}
};

// floating point
template<>
struct save_type_deducer<float>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		return { name, offset, { ST_FLOAT } };
	}
};
template<>
struct save_type_deducer<double>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		return { name, offset, { ST_DOUBLE } };
	}
};

// special types
template<>
struct save_type_deducer<edict_t *>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		return { name, offset, { ST_ENTITY } };
	}
};

template<>
struct save_type_deducer<gitem_t *>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		return { name, offset, { ST_ITEM_POINTER } };
	}
};

template<>
struct save_type_deducer<item_id_t>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		return { name, offset, { ST_ITEM_INDEX } };
	}
};

template<>
struct save_type_deducer<gtime_t>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		return { name, offset, { ST_TIME } };
	}
};

template<>
struct save_type_deducer<spawnflags_t>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		return { name, offset, { ST_UINT32 } };
	}
};

// static strings
template<size_t N>
struct save_type_deducer<char[N]>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		return { name, offset, { ST_FIXED_STRING, 0, N } };
	}
};

// enums
template<typename T>
struct save_type_deducer<T, typename std::enable_if_t<std::is_enum_v<T>>>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		return { name,
				 offset,
				 { ST_ENUM, 0,
				   std::is_same_v<std::underlying_type_t<T>, uint64_t>	 ? 8
				   : std::is_same_v<std::underlying_type_t<T>, uint32_t> ? 4
				   : std::is_same_v<std::underlying_type_t<T>, uint16_t> ? 2
				   : std::is_same_v<std::underlying_type_t<T>, uint8_t>	 ? 1
				   : std::is_same_v<std::underlying_type_t<T>, int64_t>	 ? 8
				   : std::is_same_v<std::underlying_type_t<T>, int32_t>	 ? 4
				   : std::is_same_v<std::underlying_type_t<T>, int16_t>	 ? 2
				   : std::is_same_v<std::underlying_type_t<T>, int8_t>	 ? 1
																		 : 0 } };
	}
};

// vector
template<>
struct save_type_deducer<vec3_t>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		return { name, offset, { ST_FIXED_ARRAY, ST_FLOAT, 3 } };
	}
};

// fixed-size arrays
template<typename T, size_t N>
struct save_type_deducer<T[N], typename std::enable_if_t<!std::is_same_v<T, char>>>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		auto type = save_type_deducer<std::remove_extent_t<T>>::get_save_type(nullptr, 0).type;

		if (type.id <= ST_BOOL || type.id >= ST_DOUBLE)
			return { name, offset, { ST_FIXED_ARRAY, ST_INVALID, 0, []() { return save_type_deducer<std::remove_extent_t<T>>::get_save_type(nullptr, 0).type; } } };

		return { name,
				 offset,
				 { ST_FIXED_ARRAY, type.id, N } };
	}
};

// std::array
template<typename T, size_t N>
struct save_type_deducer<std::array<T, N>>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		auto type = save_type_deducer<std::remove_extent_t<T>>::get_save_type(nullptr, 0).type;

		if (type.id <= ST_BOOL || type.id >= ST_DOUBLE)
			return { name, offset, { ST_FIXED_ARRAY, ST_INVALID, N, []() { return save_type_deducer<std::remove_extent_t<T>>::get_save_type(nullptr, 0).type; } } };

		return { name, offset, { ST_FIXED_ARRAY, type.id, N } };
	}
};

// savable_allocated_memory_t
template<typename T, int32_t Tag>
struct save_type_deducer<savable_allocated_memory_t<T, Tag>>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		auto type = save_type_deducer<std::remove_extent_t<T>>::get_save_type(nullptr, 0).type;

		if (type.id <= ST_BOOL || type.id >= ST_DOUBLE)
			return { name, offset, { ST_SAVABLE_DYNAMIC, ST_INVALID, Tag, []() { return save_type_deducer<std::remove_extent_t<T>>::get_save_type(nullptr, 0).type; } } };

		return { name, offset, { ST_SAVABLE_DYNAMIC, type.id, Tag } };
	}
};

// save_data_ref<T>
template<typename T, size_t Tag>
struct save_type_deducer<save_data_t<T, Tag>>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		return { name, offset, { ST_DATA, Tag, 0, nullptr, nullptr, false, nullptr } };
	}
};

// std::bitset<N>
template<size_t N>
struct save_type_deducer<std::bitset<N>>
{
	static constexpr save_field_t get_save_type(const char *name, size_t offset)
	{
		return { name, offset, { ST_BITSET, 0, N, nullptr, nullptr, false, nullptr, 
				[](void *data, const Json::Value &json, const char *field) {
					std::bitset<N> &as_bitset = *(std::bitset<N> *) data;

					as_bitset.reset();

					if (!json.isString())
						json_print_error(field, "expected string", false);
					else if (strlen(json.asCString()) > N)
						json_print_error(field, "bitset length overflow", false);
					else
					{
						const char *str = json.asCString();
						size_t len = strlen(str);

						for (size_t i = 0; i < len; i++)
						{
							if (str[i] == '0')
								continue;
							else if (str[i] == '1')
								as_bitset[i] = true;
							else
								json_print_error(field, "bad bitset value", false);
						}
					}
				},
				[](const void *data, bool null_for_empty, Json::Value &output) -> bool {
					const std::bitset<N> &as_bitset = *(std::bitset<N> *) data;

					if (as_bitset.none())
					{
						if (null_for_empty)
							return false;

						output = "";
						return true;
					}

					int32_t num_needed;

					for (num_needed = N - 1; num_needed >= 0; num_needed--)
						if (as_bitset[num_needed])
							break;

					// 00100000, num_needed = 2
					// num_needed always >= 0 since none() check done above
					num_needed++;

					std::string result(num_needed, '0');

					for (size_t n = 0; n < num_needed; n++)
						if (as_bitset[n])
							result[n] = '1';

					output = result;

					return true;
				}
			}
		};
	}
};

// deduce type via template deduction; use this generally
// since it prevents user error and allows seamless type upgrades.
#define FIELD_AUTO(f) save_type_deducer<decltype(DECLARE_SAVE_STRUCT::f)>::get_save_type(FIELD(f))

// simple macro for a `char*` of TAG_LEVEL allocation
#define FIELD_LEVEL_STRING(f)                                                                                          \
	{                                                                                                                  \
		FIELD(f),                                                                                                      \
		{                                                                                                              \
			ST_STRING, TAG_LEVEL                                                                                       \
		}                                                                                                              \
	}

// simple macro for a `char*` of TAG_GAME allocation
#define FIELD_GAME_STRING(f)                                                                                           \
	{                                                                                                                  \
		FIELD(f),                                                                                                      \
		{                                                                                                              \
			ST_STRING, TAG_GAME                                                                                        \
		}                                                                                                              \
	}

// simple macro for a struct type
#define FIELD_STRUCT(f, t)                                                                                             \
	{                                                                                                                  \
		FIELD(f),                                                                                                      \
		{                                                                                                              \
			ST_STRUCT, 0, sizeof(t), nullptr, &t##_savestruct                                                          \
		}                                                                                                              \
	}

// simple macro for a simple field with no parameters
#define FIELD_SIMPLE(f, t)                                                                                             \
	{                                                                                                                  \
		FIELD(f),                                                                                                      \
		{                                                                                                              \
			t                                                                                                          \
		}                                                                                                              \
	}

// macro for creating save type deducer for
// specified struct type
#define MAKE_STRUCT_SAVE_DEDUCER(t)  \
template<> \
struct save_type_deducer<t> \
{ \
	static constexpr save_field_t get_save_type(const char *name, size_t offset) \
	{ \
		return { name, offset, { ST_STRUCT, 0, sizeof(t), nullptr, &t##_savestruct } }; \
	} \
};

#define DECLARE_SAVE_STRUCT level_entry_t
SAVE_STRUCT_START
	FIELD_AUTO(map_name),
	FIELD_AUTO(pretty_name),
	FIELD_AUTO(total_secrets),
	FIELD_AUTO(found_secrets),
	FIELD_AUTO(total_monsters),
	FIELD_AUTO(killed_monsters),
	FIELD_AUTO(time),
	FIELD_AUTO(visit_order)
SAVE_STRUCT_END
#undef DECLARE_SAVE_STRUCT

MAKE_STRUCT_SAVE_DEDUCER(level_entry_t);

// clang-format off
#define DECLARE_SAVE_STRUCT game_locals_t
SAVE_STRUCT_START
	FIELD_AUTO(helpmessage1),
	FIELD_AUTO(helpmessage2),
	FIELD_AUTO(help1changed),
	FIELD_AUTO(help2changed),

	// clients is set by load/init only

	FIELD_AUTO(spawnpoint),

	FIELD_AUTO(maxclients),
	FIELD_AUTO(maxentities),

	FIELD_AUTO(cross_level_flags),
	FIELD_AUTO(cross_unit_flags),

	FIELD_AUTO(autosaved),
	FIELD_AUTO(level_entries)
SAVE_STRUCT_END
#undef DECLARE_SAVE_STRUCT

#define DECLARE_SAVE_STRUCT level_locals_t
SAVE_STRUCT_START
	FIELD_AUTO(time),

	FIELD_AUTO(level_name),
	FIELD_AUTO(mapname),
	FIELD_AUTO(nextmap),

	FIELD_AUTO(intermissiontime),
	FIELD_LEVEL_STRING(changemap),
	FIELD_LEVEL_STRING(achievement),
	FIELD_AUTO(exitintermission),
	FIELD_AUTO(intermission_clear),
	FIELD_AUTO(intermission_origin),
	FIELD_AUTO(intermission_angle),

	// pic_health is set by worldspawn
	// pic_ping is set by worldspawn
			
	FIELD_AUTO(total_secrets),
	FIELD_AUTO(found_secrets),
			
	FIELD_AUTO(total_goals),
	FIELD_AUTO(found_goals),
			
	FIELD_AUTO(total_monsters),
	FIELD_AUTO(monsters_registered),
	FIELD_AUTO(killed_monsters),

	// current_entity not necessary to save
			
	FIELD_AUTO(body_que),

	FIELD_AUTO(power_cubes),
		
	// ROGUE
	FIELD_AUTO(disguise_violator),
	FIELD_AUTO(disguise_violation_time),
	// ROGUE

	FIELD_AUTO(coop_level_restart_time),
	FIELD_LEVEL_STRING(goals),
	FIELD_AUTO(goal_num),
	FIELD_AUTO(vwep_offset),

	FIELD_AUTO(valid_poi),
	FIELD_AUTO(current_poi),
	FIELD_AUTO(current_poi_stage),
	FIELD_AUTO(current_poi_image),
	FIELD_AUTO(current_dynamic_poi),

	FIELD_LEVEL_STRING(start_items),
	FIELD_AUTO(no_grapple),
	FIELD_AUTO(gravity),
	FIELD_AUTO(hub_map),
	FIELD_AUTO(health_bar_entities),
	FIELD_AUTO(intermission_server_frame),
	FIELD_AUTO(story_active),
	FIELD_AUTO(next_auto_save)
SAVE_STRUCT_END
#undef DECLARE_SAVE_STRUCT

#define DECLARE_SAVE_STRUCT pmove_state_t
SAVE_STRUCT_START
	FIELD_AUTO(pm_type),
	FIELD_AUTO(origin),
	FIELD_AUTO(velocity),
	FIELD_AUTO(pm_flags),
	FIELD_AUTO(pm_time),
	FIELD_AUTO(gravity),
	FIELD_AUTO(delta_angles),
	FIELD_AUTO(viewheight)
SAVE_STRUCT_END
#undef DECLARE_SAVE_STRUCT

#define DECLARE_SAVE_STRUCT player_state_t
SAVE_STRUCT_START
	FIELD_STRUCT(pmove, pmove_state_t),

	FIELD_AUTO(viewangles),
	FIELD_AUTO(viewoffset),
	// kick_angles only last 1 frame
	
	FIELD_AUTO(gunangles),
	FIELD_AUTO(gunoffset),
	FIELD_AUTO(gunindex),
	FIELD_AUTO(gunframe),
	FIELD_AUTO(gunskin),
	// blend is calculated by ClientEndServerFrame
	FIELD_AUTO(fov),

	// rdflags are generated by ClientEndServerFrame
	FIELD_AUTO(stats)
SAVE_STRUCT_END
#undef DECLARE_SAVE_STRUCT

#define DECLARE_SAVE_STRUCT height_fog_t
SAVE_STRUCT_START
	FIELD_AUTO(start),
	FIELD_AUTO(end),
	FIELD_AUTO(falloff),
	FIELD_AUTO(density)
SAVE_STRUCT_END
#undef DECLARE_SAVE_STRUCT

#define DECLARE_SAVE_STRUCT client_persistant_t
SAVE_STRUCT_START
	FIELD_AUTO(userinfo),
	FIELD_AUTO(social_id),
	FIELD_AUTO(netname),
	FIELD_AUTO(hand),

	FIELD_AUTO(health),
	FIELD_AUTO(max_health),
	FIELD_AUTO(savedFlags),

	FIELD_AUTO(selected_item),
	FIELD_SIMPLE(inventory, ST_INVENTORY),

	FIELD_AUTO(max_ammo),
	
	FIELD_AUTO(weapon),
	FIELD_AUTO(lastweapon),
	
	FIELD_AUTO(power_cubes),
	FIELD_AUTO(score),
	
	FIELD_AUTO(game_help1changed),
	FIELD_AUTO(game_help2changed),
	FIELD_AUTO(helpchanged),
	FIELD_AUTO(help_time),
	
	FIELD_AUTO(spectator),

	// save the wanted fog, but not the current fog
	// or transition time so it sends immediately
	FIELD_AUTO(wanted_fog),
	FIELD_STRUCT(wanted_heightfog, height_fog_t),
	FIELD_AUTO(megahealth_time),
	FIELD_AUTO(lives),
	FIELD_AUTO(n64_crouch_warn_times),
	FIELD_AUTO(n64_crouch_warning)
SAVE_STRUCT_END
#undef DECLARE_SAVE_STRUCT

#define DECLARE_SAVE_STRUCT gclient_t
SAVE_STRUCT_START
	FIELD_STRUCT(ps, player_state_t),
	// ping... duh

	FIELD_STRUCT(pers, client_persistant_t),

	FIELD_STRUCT(resp.coop_respawn, client_persistant_t),
	FIELD_AUTO(resp.entertime),
	FIELD_AUTO(resp.score),
	FIELD_AUTO(resp.cmd_angles),
	FIELD_AUTO(resp.spectator),
	// old_pmove is not necessary to persist

	// showscores, showinventory, showhelp not necessary

	// buttons, oldbuttons, latched_buttons not necessary
	// weapon_thunk not necessary
	
	FIELD_AUTO(newweapon),

	// damage_ members are calculated on damage

	FIELD_AUTO(killer_yaw),
	
	FIELD_AUTO(weaponstate),
	FIELD_AUTO(kick.angles),
	FIELD_AUTO(kick.origin),
	FIELD_AUTO(kick.total),
	FIELD_AUTO(kick.time),
	FIELD_AUTO(quake_time),
	FIELD_AUTO(v_dmg_roll),
	FIELD_AUTO(v_dmg_pitch),
	FIELD_AUTO(v_dmg_time),
	FIELD_AUTO(fall_time),
	FIELD_AUTO(fall_value),
	FIELD_AUTO(damage_alpha),
	FIELD_AUTO(bonus_alpha),
	FIELD_AUTO(damage_blend),
	FIELD_AUTO(v_angle),
	FIELD_AUTO(bobtime),
	FIELD_AUTO(oldviewangles),
	FIELD_AUTO(oldvelocity),
	FIELD_AUTO(oldgroundentity),

	FIELD_AUTO(next_drown_time),
	FIELD_AUTO(old_waterlevel),
	FIELD_AUTO(breather_sound),
	
	FIELD_AUTO(machinegun_shots),
	
	FIELD_AUTO(anim_end),
	FIELD_AUTO(anim_priority),
	FIELD_AUTO(anim_duck),
	FIELD_AUTO(anim_run),
	
	FIELD_AUTO(quad_time),
	FIELD_AUTO(invincible_time),
	FIELD_AUTO(breather_time),
	FIELD_AUTO(enviro_time),
	FIELD_AUTO(invisible_time),

	FIELD_AUTO(grenade_blew_up),
	FIELD_AUTO(grenade_time),
	FIELD_AUTO(grenade_finished_time),
	FIELD_AUTO(quadfire_time),
	FIELD_AUTO(silencer_shots),
	FIELD_AUTO(weapon_sound),

	FIELD_AUTO(pickup_msg_time),

	// flood stuff is dm only
	
	FIELD_AUTO(respawn_time),

	// chasecam not required to persist

	FIELD_AUTO(double_time),
	FIELD_AUTO(ir_time),
	FIELD_AUTO(nuke_time),
	FIELD_AUTO(tracker_pain_time),
	
	// owned_sphere is DM only

	FIELD_AUTO(empty_click_sound),
	FIELD_AUTO(trail_head),
	FIELD_AUTO(trail_tail),

	FIELD_GAME_STRING(landmark_name),
	FIELD_AUTO(landmark_rel_pos),
	FIELD_AUTO(landmark_free_fall),
	FIELD_AUTO(landmark_noise_time),
	FIELD_AUTO(invisibility_fade_time),
	FIELD_AUTO(last_ladder_pos),
	FIELD_AUTO(last_ladder_sound),

	FIELD_AUTO(sight_entity),
	FIELD_AUTO(sight_entity_time),
	FIELD_AUTO(sound_entity),
	FIELD_AUTO(sound_entity_time),
	FIELD_AUTO(sound2_entity),
	FIELD_AUTO(sound2_entity_time),

	FIELD_AUTO(last_firing_time),
SAVE_STRUCT_END
#undef DECLARE_SAVE_STRUCT
// clang-format on

static bool edict_t_gravity_is_empty(const void *data)
{
	return *((const float *) data) == 1.f;
}

static bool edict_t_gravityVector_is_empty(const void *data)
{
	constexpr vec3_t up_vector = { 0, 0, -1 };
	return *(const vec3_t *) data == up_vector;
}

// clang-format off
#define DECLARE_SAVE_STRUCT edict_t
SAVE_STRUCT_START
	// entity_state_t stuff; only one instance
	// so no need to do a whole save struct
	FIELD_AUTO(s.origin),
	FIELD_AUTO(s.angles),
	FIELD_AUTO(s.old_origin),
	FIELD_AUTO(s.modelindex),
	FIELD_AUTO(s.modelindex2),
	FIELD_AUTO(s.modelindex3),
	FIELD_AUTO(s.modelindex4),
	FIELD_AUTO(s.frame),
	FIELD_AUTO(s.skinnum),
	FIELD_AUTO(s.effects),
	FIELD_AUTO(s.renderfx),
	// s.solid is set by linkentity
	// events are cleared on each frame, no need to save
	FIELD_AUTO(s.sound),
	FIELD_AUTO(s.alpha),
	FIELD_AUTO(s.scale),
	FIELD_AUTO(s.instance_bits),

	// server stuff
	// client is auto-set
	// inuse is implied
	FIELD_AUTO(linkcount),
	// area, num_clusters, clusternums, headnode, areanum, areanum2
	// are set by linkentity and can't be saved

	FIELD_AUTO(svflags),
	FIELD_AUTO(mins),
	FIELD_AUTO(maxs),
	// absmin, absmax and size are set by linkentity
	FIELD_AUTO(solid),
	FIELD_AUTO(clipmask),
	FIELD_AUTO(owner),

	// game stuff
	FIELD_AUTO(spawn_count),
	FIELD_AUTO(movetype),
	FIELD_AUTO(flags),
	
	FIELD_LEVEL_STRING(model),
	FIELD_AUTO(freetime),
	
	FIELD_LEVEL_STRING(message),
	FIELD_LEVEL_STRING(classname), // FIXME: should allow loading from constants
	FIELD_AUTO(spawnflags),

	FIELD_AUTO(timestamp),

	FIELD_AUTO(angle),
	FIELD_LEVEL_STRING(target),
	FIELD_LEVEL_STRING(targetname),
	FIELD_LEVEL_STRING(killtarget),
	FIELD_LEVEL_STRING(team),
	FIELD_LEVEL_STRING(pathtarget),
	FIELD_LEVEL_STRING(deathtarget),
	FIELD_LEVEL_STRING(healthtarget),
	FIELD_LEVEL_STRING(itemtarget),
	FIELD_LEVEL_STRING(combattarget),
	FIELD_AUTO(target_ent),
	
	FIELD_AUTO(speed),
	FIELD_AUTO(accel),
	FIELD_AUTO(decel),
	FIELD_AUTO(movedir),
	FIELD_AUTO(pos1),
	FIELD_AUTO(pos2),
	FIELD_AUTO(pos3),
	
	FIELD_AUTO(velocity),
	FIELD_AUTO(avelocity),
	FIELD_AUTO(mass),
	FIELD_AUTO(air_finished),
	FIELD_AUTO(gravity).set_is_empty(edict_t_gravity_is_empty),
	
	FIELD_AUTO(goalentity),
	FIELD_AUTO(movetarget),
	FIELD_AUTO(yaw_speed),
	FIELD_AUTO(ideal_yaw),

	FIELD_AUTO(nextthink),
	FIELD_AUTO(prethink),
	FIELD_AUTO(postthink),
	FIELD_AUTO(think),
	FIELD_AUTO(touch),
	FIELD_AUTO(use),
	FIELD_AUTO(pain),
	FIELD_AUTO(die),
	
	FIELD_AUTO(touch_debounce_time),
	FIELD_AUTO(pain_debounce_time),
	FIELD_AUTO(damage_debounce_time),
	FIELD_AUTO(fly_sound_debounce_time),
	FIELD_AUTO(last_move_time),
	
	FIELD_AUTO(health),
	FIELD_AUTO(max_health),
	FIELD_AUTO(gib_health),
	FIELD_AUTO(deadflag),
	FIELD_AUTO(show_hostile),

	FIELD_AUTO(powerarmor_time),

	FIELD_LEVEL_STRING(map),
	
	FIELD_AUTO(viewheight),
	FIELD_AUTO(takedamage),
	FIELD_AUTO(dmg),
	FIELD_AUTO(radius_dmg),
	FIELD_AUTO(dmg_radius),
	FIELD_AUTO(sounds),
	FIELD_AUTO(count),
		
	FIELD_AUTO(chain),
	FIELD_AUTO(enemy),
	FIELD_AUTO(oldenemy),
	FIELD_AUTO(activator),
	FIELD_AUTO(groundentity),
	FIELD_AUTO(groundentity_linkcount),
	FIELD_AUTO(teamchain),
	FIELD_AUTO(teammaster),

	FIELD_AUTO(mynoise),
	FIELD_AUTO(mynoise2),
		
	FIELD_AUTO(noise_index),
	FIELD_AUTO(noise_index2),
	FIELD_AUTO(volume),
	FIELD_AUTO(attenuation),
		
	FIELD_AUTO(wait),
	FIELD_AUTO(delay),
	FIELD_AUTO(random),

	FIELD_AUTO(teleport_time),
		
	FIELD_AUTO(watertype),
	FIELD_AUTO(waterlevel),
		
	FIELD_AUTO(move_origin),
	FIELD_AUTO(move_angles),

	FIELD_AUTO(style),
	FIELD_LEVEL_STRING(style_on),
	FIELD_LEVEL_STRING(style_off),

	FIELD_AUTO(item),
	FIELD_AUTO(crosslevel_flags),

	// moveinfo_t
	FIELD_AUTO(moveinfo.start_origin),
	FIELD_AUTO(moveinfo.start_angles),
	FIELD_AUTO(moveinfo.end_origin),
	FIELD_AUTO(moveinfo.end_angles),
	FIELD_AUTO(moveinfo.end_angles_reversed),
		
	FIELD_AUTO(moveinfo.sound_start),
	FIELD_AUTO(moveinfo.sound_middle),
	FIELD_AUTO(moveinfo.sound_end),
		
	FIELD_AUTO(moveinfo.accel),
	FIELD_AUTO(moveinfo.speed),
	FIELD_AUTO(moveinfo.decel),
	FIELD_AUTO(moveinfo.distance),

	FIELD_AUTO(moveinfo.wait),
		
	FIELD_AUTO(moveinfo.state),
	FIELD_AUTO(moveinfo.reversing),
	FIELD_AUTO(moveinfo.dir),
	FIELD_AUTO(moveinfo.dest),
	FIELD_AUTO(moveinfo.current_speed),
	FIELD_AUTO(moveinfo.move_speed),
	FIELD_AUTO(moveinfo.next_speed),
	FIELD_AUTO(moveinfo.remaining_distance),
	FIELD_AUTO(moveinfo.decel_distance),
	FIELD_AUTO(moveinfo.endfunc),
	FIELD_AUTO(moveinfo.blocked),

	FIELD_AUTO(moveinfo.curve_ref),
	FIELD_AUTO(moveinfo.curve_positions),
	FIELD_AUTO(moveinfo.curve_frame),
	FIELD_AUTO(moveinfo.subframe),
	FIELD_AUTO(moveinfo.num_subframes),
	FIELD_AUTO(moveinfo.num_frames_done),

	// monsterinfo_t
	FIELD_AUTO(monsterinfo.active_move),
	FIELD_AUTO(monsterinfo.next_move),
	FIELD_AUTO(monsterinfo.aiflags),
	FIELD_AUTO(monsterinfo.nextframe),
	FIELD_AUTO(monsterinfo.scale),
		
	FIELD_AUTO(monsterinfo.stand),
	FIELD_AUTO(monsterinfo.idle),
	FIELD_AUTO(monsterinfo.search),
	FIELD_AUTO(monsterinfo.walk),
	FIELD_AUTO(monsterinfo.run),
	FIELD_AUTO(monsterinfo.dodge),
	FIELD_AUTO(monsterinfo.attack),
	FIELD_AUTO(monsterinfo.melee),
	FIELD_AUTO(monsterinfo.sight),
	FIELD_AUTO(monsterinfo.checkattack),
	FIELD_AUTO(monsterinfo.setskin),
		
	FIELD_AUTO(monsterinfo.pausetime),
	FIELD_AUTO(monsterinfo.attack_finished),
	FIELD_AUTO(monsterinfo.fire_wait),

	FIELD_AUTO(monsterinfo.saved_goal),
	FIELD_AUTO(monsterinfo.search_time),
	FIELD_AUTO(monsterinfo.trail_time),
	FIELD_AUTO(monsterinfo.last_sighting),
	FIELD_AUTO(monsterinfo.attack_state),
	FIELD_AUTO(monsterinfo.lefty),
	FIELD_AUTO(monsterinfo.idle_time),
	FIELD_AUTO(monsterinfo.linkcount),

	FIELD_AUTO(monsterinfo.power_armor_type),
	FIELD_AUTO(monsterinfo.power_armor_power),
	FIELD_AUTO(monsterinfo.initial_power_armor_type),
	FIELD_AUTO(monsterinfo.max_power_armor_power),
	FIELD_AUTO(monsterinfo.weapon_sound),
	FIELD_AUTO(monsterinfo.engine_sound),

	FIELD_AUTO(monsterinfo.blocked),
	FIELD_AUTO(monsterinfo.last_hint_time),
	FIELD_AUTO(monsterinfo.goal_hint),
	FIELD_AUTO(monsterinfo.medicTries),
	FIELD_AUTO(monsterinfo.badMedic1),
	FIELD_AUTO(monsterinfo.badMedic2),
	FIELD_AUTO(monsterinfo.healer),
	FIELD_AUTO(monsterinfo.duck),
	FIELD_AUTO(monsterinfo.unduck),
	FIELD_AUTO(monsterinfo.sidestep),
	FIELD_AUTO(monsterinfo.base_height),
	FIELD_AUTO(monsterinfo.next_duck_time),
	FIELD_AUTO(monsterinfo.duck_wait_time),
	FIELD_AUTO(monsterinfo.last_player_enemy),
	FIELD_AUTO(monsterinfo.blindfire),
	FIELD_AUTO(monsterinfo.can_jump),
	FIELD_AUTO(monsterinfo.had_visibility),
	FIELD_AUTO(monsterinfo.drop_height),
	FIELD_AUTO(monsterinfo.jump_height),
	FIELD_AUTO(monsterinfo.blind_fire_delay),
	FIELD_AUTO(monsterinfo.blind_fire_target),
	FIELD_AUTO(monsterinfo.monster_slots),
	FIELD_AUTO(monsterinfo.monster_used),
	FIELD_AUTO(monsterinfo.commander),
	FIELD_AUTO( monsterinfo.quad_time ),
	FIELD_AUTO( monsterinfo.invincible_time ),
	FIELD_AUTO( monsterinfo.double_time ),

	FIELD_AUTO( monsterinfo.surprise_time ),
	FIELD_AUTO( monsterinfo.armor_type ),
	FIELD_AUTO( monsterinfo.armor_power ),
	FIELD_AUTO( monsterinfo.close_sight_tripped ),
	FIELD_AUTO( monsterinfo.melee_debounce_time ),
	FIELD_AUTO( monsterinfo.strafe_check_time ),
	FIELD_AUTO( monsterinfo.base_health ),
	FIELD_AUTO( monsterinfo.health_scaling ),
	FIELD_AUTO( monsterinfo.next_move_time ),
	FIELD_AUTO( monsterinfo.bad_move_time ),
	FIELD_AUTO( monsterinfo.bump_time ),
	FIELD_AUTO( monsterinfo.random_change_time ),
	FIELD_AUTO( monsterinfo.path_blocked_counter ),
	FIELD_AUTO( monsterinfo.path_wait_time ),
	FIELD_AUTO( monsterinfo.combat_style ),

	FIELD_AUTO( monsterinfo.fly_max_distance ),
	FIELD_AUTO( monsterinfo.fly_min_distance ),
	FIELD_AUTO( monsterinfo.fly_acceleration ),
	FIELD_AUTO( monsterinfo.fly_speed ),
	FIELD_AUTO( monsterinfo.fly_ideal_position ),
	FIELD_AUTO( monsterinfo.fly_position_time ),
	FIELD_AUTO( monsterinfo.fly_buzzard ),
	FIELD_AUTO( monsterinfo.fly_above ),
	FIELD_AUTO( monsterinfo.fly_pinned ),
	FIELD_AUTO( monsterinfo.fly_thrusters ),
	FIELD_AUTO( monsterinfo.fly_recovery_time ),
	FIELD_AUTO( monsterinfo.fly_recovery_dir ),

	FIELD_AUTO( monsterinfo.checkattack_time ),
	FIELD_AUTO( monsterinfo.start_frame ),
	FIELD_AUTO( monsterinfo.dodge_time ),
	FIELD_AUTO( monsterinfo.move_block_counter ),
	FIELD_AUTO( monsterinfo.move_block_change_time ),
	FIELD_AUTO( monsterinfo.react_to_damage_time ),
	FIELD_AUTO(monsterinfo.jump_time),

	FIELD_SIMPLE( monsterinfo.reinforcements, ST_REINFORCEMENTS ),
	FIELD_AUTO( monsterinfo.chosen_reinforcements ),

// back to edict_t
	FIELD_AUTO( plat2flags ),
	FIELD_AUTO( offset ),
	FIELD_AUTO( gravityVector ).set_is_empty( edict_t_gravityVector_is_empty ),
	FIELD_AUTO( bad_area ),
	FIELD_AUTO( hint_chain ),
	FIELD_AUTO( monster_hint_chain ),
	FIELD_AUTO( target_hint_chain ),
	FIELD_AUTO( hint_chain_id ),

	FIELD_AUTO( clock_message ),
	FIELD_AUTO( dead_time ),
	FIELD_AUTO( beam ),
	FIELD_AUTO( beam2 ),
	FIELD_AUTO(proboscus),
	FIELD_AUTO( disintegrator ),
	FIELD_AUTO( disintegrator_time ),
	FIELD_AUTO( hackflags ),
	
	FIELD_AUTO( fog.color ),
	FIELD_AUTO( fog.density ),
	FIELD_AUTO( fog.color_off ),
	FIELD_AUTO( fog.density_off ),
	FIELD_AUTO( fog.sky_factor ),
	FIELD_AUTO( fog.sky_factor_off ),

	FIELD_AUTO( heightfog.falloff ),
	FIELD_AUTO( heightfog.density ),
	FIELD_AUTO( heightfog.start_color ),
	FIELD_AUTO( heightfog.start_dist ),
	FIELD_AUTO( heightfog.end_color ),
	FIELD_AUTO( heightfog.end_dist ),

	FIELD_AUTO( heightfog.falloff_off ),
	FIELD_AUTO( heightfog.density_off ),
	FIELD_AUTO( heightfog.start_color_off ),
	FIELD_AUTO( heightfog.start_dist_off ),
	FIELD_AUTO( heightfog.end_color_off ),
	FIELD_AUTO( heightfog.end_dist_off ),

	FIELD_AUTO( item_picked_up_by ),
	FIELD_AUTO( slime_debounce_time ),

	FIELD_AUTO( bmodel_anim.start ),
	FIELD_AUTO( bmodel_anim.end ),
	FIELD_AUTO( bmodel_anim.style ),
	FIELD_AUTO( bmodel_anim.speed ),
	FIELD_AUTO( bmodel_anim.nowrap ),

	FIELD_AUTO( bmodel_anim.alt_start ),
	FIELD_AUTO( bmodel_anim.alt_end ),
	FIELD_AUTO( bmodel_anim.alt_style ),
	FIELD_AUTO( bmodel_anim.alt_speed ),
	FIELD_AUTO( bmodel_anim.alt_nowrap ),

	FIELD_AUTO( bmodel_anim.enabled ),
	FIELD_AUTO( bmodel_anim.alternate ),
	FIELD_AUTO( bmodel_anim.currently_alternate ),
	FIELD_AUTO( bmodel_anim.next_tick ),
	
	FIELD_AUTO( lastMOD.id ),
	FIELD_AUTO( lastMOD.friendly_fire ),

SAVE_STRUCT_END
#undef DECLARE_SAVE_STRUCT
// clang-format on

inline size_t get_simple_type_size(save_type_id_t id, bool fatal = true)
{
	switch (id)
	{
	case ST_BOOL:
		return sizeof(bool);
	case ST_INT8:
	case ST_UINT8:
		return sizeof(uint8_t);
	case ST_INT16:
	case ST_UINT16:
		return sizeof(uint16_t);
	case ST_INT32:
	case ST_UINT32:
		return sizeof(uint32_t);
	case ST_INT64:
	case ST_UINT64:
	case ST_TIME:
		return sizeof(uint64_t);
	case ST_FLOAT:
		return sizeof(float);
	case ST_DOUBLE:
		return sizeof(double);
	case ST_ENTITY:
	case ST_ITEM_POINTER:
		return sizeof(size_t);
	case ST_ITEM_INDEX:
		return sizeof(uint32_t);
	case ST_SAVABLE_DYNAMIC:
		return sizeof(savable_allocated_memory_t<void *, 0>);
	default:
		if (fatal)
			gi.Com_ErrorFmt("Can't calculate static size for type ID {}", (int32_t) id);
		break;
	}

	return 0;
}

size_t get_complex_type_size(const save_type_t &type)
{
	// these are simple types
	if (auto simple = get_simple_type_size(type.id, false))
		return simple;

	switch (type.id)
	{
		case ST_STRUCT:
			return type.count;
		case ST_FIXED_ARRAY: {
			save_type_t element_type;
			size_t element_size;

			if (type.type_resolver)
			{
				element_type = type.type_resolver();
				element_size = get_complex_type_size(element_type);
			}
			else
			{
				element_size = get_simple_type_size((save_type_id_t) type.tag);
				element_type = { (save_type_id_t) type.tag };
			}

			return element_size * type.count;
		}
		default:
			gi.Com_ErrorFmt("Can't calculate static size for type ID {}", (int32_t) type.id);
	}

	return 0;
}

void read_save_struct_json(const Json::Value &json, void *data, const save_struct_t *structure);

void read_save_type_json(const Json::Value &json, void *data, const save_type_t *type, const char *field)
{
	switch (type->id)
	{
	case ST_BOOL:
		if (!json.isBool())
			json_print_error(field, "expected boolean", false);
		else
			*((bool *) data) = json.asBool();
		return;
	case ST_ENUM:
		if (!json.isIntegral())
			json_print_error(field, "expected integer", false);
		else if (type->count == 1)
		{
			if (json.isInt())
			{
				if (json.asInt() < INT8_MIN || json.asInt() > INT8_MAX)
					json_print_error(field, "int8 out of range", false);
				else
					*((int8_t *) data) = json.asInt();
			}
			else if (json.isUInt())
			{
				if (json.asUInt() > UINT8_MAX)
					json_print_error(field, "uint8 out of range", false);
				else
					*((uint8_t *) data) = json.asUInt();
			}
			else
				json_print_error(field, "int8 out of range (is 64-bit)", false);
			return;
		}
		else if (type->count == 2)
		{
			if (json.isInt())
			{
				if (json.asInt() < INT16_MIN || json.asInt() > INT16_MAX)
					json_print_error(field, "int16 out of range", false);
				else
					*((int16_t *) data) = json.asInt();
			}
			else if (json.isUInt())
			{
				if (json.asUInt() > UINT16_MAX)
					json_print_error(field, "uint16 out of range", false);
				else
					*((uint16_t *) data) = json.asUInt();
			}
			else
				json_print_error(field, "int16 out of range (is 64-bit)", false);
			return;
		}
		else if (type->count == 4)
		{
			if (json.isInt())
			{
				if (json.asInt64() < INT32_MIN || json.asInt64() > INT32_MAX)
					json_print_error(field, "int32 out of range", false);
				else
					*((int32_t *) data) = json.asInt();
			}
			else if (json.isUInt())
			{
				if (json.asUInt64() > UINT32_MAX)
					json_print_error(field, "uint32 out of range", false);
				else
					*((uint32_t *) data) = json.asUInt();
			}
			else
				json_print_error(field, "int32 out of range (is 64-bit)", false);
			return;
		}
		else if (type->count == 8)
		{
			if (json.isInt64())
				*((int64_t *) data) = json.asInt64();
			else if (json.isUInt64())
				*((int64_t *) data) = json.asUInt64();
			else if (json.isInt())
				*((int64_t *) data) = json.asInt();
			else if (json.isUInt())
				*((int64_t *) data) = json.asUInt();
			else
				json_print_error(field, "int64 not integral", false);
			return;
		}

		json_print_error(field, "invalid enum size", true);
		return;
	case ST_INT8:
		if (!json.isInt())
			json_print_error(field, "expected integer", false);
		else if (json.asInt() < INT8_MIN || json.asInt() > INT8_MAX)
			json_print_error(field, "int8 out of range", false);
		else
			*((int8_t *) data) = json.asInt();
		return;
	case ST_INT16:
		if (!json.isInt())
			json_print_error(field, "expected integer", false);
		else if (json.asInt() < INT16_MIN || json.asInt() > INT16_MAX)
			json_print_error(field, "int16 out of range", false);
		else
			*((int16_t *) data) = json.asInt();
		return;
	case ST_INT32:
		if (!json.isInt())
			json_print_error(field, "expected integer", false);
		else if (json.asInt() < INT32_MIN || json.asInt() > INT32_MAX)
			json_print_error(field, "int32 out of range", false);
		else
			*((int32_t *) data) = json.asInt();
		return;
	case ST_INT64:
		if (!json.isInt64())
			json_print_error(field, "expected integer", false);
		else
			*((int64_t *) data) = json.asInt64();
		return;
	case ST_UINT8:
		if (!json.isUInt())
			json_print_error(field, "expected integer", false);
		else if (json.asUInt() > UINT8_MAX)
			json_print_error(field, "uint8 out of range", false);
		else
			*((uint8_t *) data) = json.asUInt();
		return;
	case ST_UINT16:
		if (!json.isUInt())
			json_print_error(field, "expected integer", false);
		else if (json.asUInt() > UINT16_MAX)
			json_print_error(field, "uint16 out of range", false);
		else
			*((uint16_t *) data) = json.asUInt();
		return;
	case ST_UINT32:
		if (!json.isUInt())
			json_print_error(field, "expected integer", false);
		else if (json.asUInt() > UINT32_MAX)
			json_print_error(field, "uint32 out of range", false);
		else
			*((uint32_t *) data) = json.asUInt();
		return;
	case ST_UINT64:
		if (!json.isUInt64())
			json_print_error(field, "expected integer", false);
		else
			*((uint64_t *) data) = json.asUInt64();
		return;
	case ST_FLOAT:
		if (!json.isDouble())
			json_print_error(field, "expected number", false);
		else if (isnan(json.asDouble()))
			*((float *) data) = std::numeric_limits<float>::quiet_NaN();
		else
			*((float *) data) = json.asFloat();
		return;
	case ST_DOUBLE:
		if (!json.isDouble())
			json_print_error(field, "expected number", false);
		else
			*((double *) data) = json.asDouble();
		return;
	case ST_STRING:
		if (json.isNull())
			*((char **) data) = nullptr;
		else if (json.isString())
		{
			if (type->count && strlen(json.asCString()) >= type->count)
				json_print_error(field, "static-length dynamic string overrun", false);
			else
			{
				size_t len = strlen(json.asCString());
				char	 *str = *((char **) data) = (char *) gi.TagMalloc(type->count ? type->count : (len + 1), type->tag);
				strcpy(str, json.asCString());
				str[len] = 0;
			}
		}
		else if (json.isArray())
		{
			if (type->count && json.size() >= type->count - 1)
				json_print_error(field, "static-length dynamic string overrun", false);
			else
			{
				size_t len = json.size();
				char	 *str = *((char **) data) = (char *) gi.TagMalloc(type->count ? type->count : (len + 1), type->tag);

				for (Json::Value::ArrayIndex i = 0; i < json.size(); i++)
				{
					const Json::Value &chr = json[i];

					if (!chr.isInt())
						json_print_error(field, "expected number", false);
					else if (chr.asInt() < 0 || chr.asInt() > UINT8_MAX)
						json_print_error(field, "char out of range", false);

					str[i] = chr.asInt();
				}

				str[len] = 0;
			}
		}
		else
			json_print_error(field, "expected string, array or null", false);
		return;
	case ST_FIXED_STRING:
		if (json.isString())
		{
			if (type->count && strlen(json.asCString()) >= type->count)
				json_print_error(field, "fixed length string overrun", false);
			else
				strcpy((char *) data, json.asCString());
		}
		else if (json.isArray())
		{
			if (type->count && json.size() >= type->count - 1)
				json_print_error(field, "fixed length string overrun", false);
			else
			{
				Json::Value::ArrayIndex i;

				for (i = 0; i < json.size(); i++)
				{
					const Json::Value &chr = json[i];

					if (!chr.isInt())
						json_print_error(field, "expected number", false);
					else if (chr.asInt() < 0 || chr.asInt() > UINT8_MAX)
						json_print_error(field, "char out of range", false);

					((char *) data)[i] = chr.asInt();
				}

				((char *) data)[i] = 0;
			}
		}
		else
			json_print_error(field, "expected string or array", false);
		return;
	case ST_FIXED_ARRAY:
		if (!json.isArray())
			json_print_error(field, "expected array", false);
		else if (type->count != json.size())
			json_print_error(field, "fixed array length mismatch", false);
		else
		{
			uint8_t			 *element = (uint8_t *) data;
			size_t			  element_size;
			save_type_t       element_type;

			if (type->type_resolver)
			{
				element_type = type->type_resolver();
				element_size = get_complex_type_size(element_type);
			}
			else
			{
				element_size = get_simple_type_size((save_type_id_t) type->tag);
				element_type = { (save_type_id_t) type->tag };
			}

			for (Json::Value::ArrayIndex i = 0; i < type->count; i++, element += element_size)
			{
				const Json::Value &v = json[i];
				read_save_type_json(v, element, &element_type,
									fmt::format("[{}]", i).c_str());
			}
		}

		return;
	case ST_SAVABLE_DYNAMIC:
		if (!json.isArray())
			json_print_error(field, "expected array", false);
		else
		{
			savable_allocated_memory_t<void, 0> *savptr = (savable_allocated_memory_t<void, 0> *) data;
			size_t			  element_size;
			save_type_t       element_type;

			if (type->type_resolver)
			{
				element_type = type->type_resolver();
				element_size = get_complex_type_size(element_type);
			}
			else
			{
				element_size = get_simple_type_size((save_type_id_t) type->tag);
				element_type = { (save_type_id_t) type->tag };
			}

			savptr->count = json.size();
			savptr->ptr = gi.TagMalloc(element_size * savptr->count, type->count);

			byte *out_element = (byte *) savptr->ptr;

			for (Json::Value::ArrayIndex i = 0; i < savptr->count; i++, out_element += element_size)
			{
				const Json::Value &v = json[i];
				read_save_type_json(v, out_element, &element_type,
					fmt::format("[{}]", i).c_str());
			}
		}

		return;
	case ST_BITSET:
		type->read(data, json, field);
		return;
	case ST_STRUCT:
		if (!json.isNull())
		{
			json_push_stack(field);
			read_save_struct_json(json, data, type->structure);
			json_pop_stack();
		}
		return;
	case ST_ENTITY:
		if (json.isNull())
			*((edict_t **) data) = nullptr;
		else if (!json.isUInt())
			json_print_error(field, "expected null or integer", false);
		else if (json.asUInt() >= globals.max_edicts)
			json_print_error(field, "entity index out of range", false);
		else
			*((edict_t **) data) = globals.edicts + json.asUInt();

		return;
	case ST_ITEM_POINTER:
	case ST_ITEM_INDEX: {
		gitem_t *item;

		if (json.isNull())
			item = nullptr;
		else if (json.isString())
		{
			const char *classname = json.asCString();
			item = FindItemByClassname(classname);

			if (item == nullptr)
			{
				json_print_error(field, G_Fmt("item {} missing", classname).data(), false);
				return;
			}
		}
		else
		{
			json_print_error(field, "expected null or string", false);
			return;
		}

		if (type->id == ST_ITEM_POINTER)
			*((gitem_t **) data) = item;
		else
			*((int32_t *) data) = item ? item->id : 0;
		return;
	}
	case ST_TIME:
		if (!json.isInt64())
			json_print_error(field, "expected integer", false);
		else
			*((gtime_t *) data) = gtime_t::from_ms(json.asInt64());
		return;
	case ST_DATA:
		if (json.isNull())
			*((void **) data) = nullptr;
		else if (!json.isString())
			json_print_error(field, "expected null or string", false);
		else
		{
			const char *name = json.asCString();
			auto link = list_str_hash.find(name);

			if (link == list_str_hash.end())
				json_print_error(
					field, G_Fmt("unknown pointer {} in list {}", name, type->tag).data(), false);
			else
				(*reinterpret_cast<save_void_t *>(data)) = save_void_t(link->second);
		}
		return;
	case ST_INVENTORY:
		if (!json.isObject())
			json_print_error(field, "expected object", false);
		else
		{
			int32_t *inventory_ptr = (int32_t *) data;

			//for (auto key : json.getMemberNames())
			for (auto it = json.begin(); it != json.end(); it++)
			{
				//const char		   *classname = key.c_str();
				const char *dummy;
				const char *classname = it.memberName(&dummy);
				const Json::Value &value = *it;

				if (!value.isInt())
				{
					json_push_stack(classname);
					json_print_error(field, "expected integer", false);
					json_pop_stack();
					continue;
				}

				gitem_t *item = FindItemByClassname(classname);

				if (!item)
				{
					json_push_stack(classname);
					json_print_error(field, G_Fmt("can't find item {}", classname).data(), false);
					json_pop_stack();
					continue;
				}

				inventory_ptr[item->id] = value.asInt();
			}
			return;
		}
		return;
	case ST_REINFORCEMENTS:
		if (!json.isArray())
			json_print_error(field, "expected array", false);
		else
		{
			reinforcement_list_t *list_ptr = (reinforcement_list_t *) data;

			list_ptr->num_reinforcements = json.size();
			list_ptr->reinforcements = (reinforcement_t *) gi.TagMalloc(sizeof(reinforcement_t) * list_ptr->num_reinforcements, TAG_LEVEL);

			reinforcement_t *p = list_ptr->reinforcements;

			for (Json::Value::ArrayIndex i = 0; i < json.size(); i++, p++)
			{
				const Json::Value &value = json[i];

				if (!value.isObject())
				{
					json_push_stack(fmt::format("{}", i));
					json_print_error(field, "expected object", false);
					json_pop_stack();
					continue;
				}

				// quick type checks

				if (!value["classname"].isString())
				{
					json_push_stack(fmt::format("{}.classname", i));
					json_print_error(field, "expected string", false);
					json_pop_stack();
					continue;
				}

				if (!value["mins"].isArray() || value["mins"].size() != 3)
				{
					json_push_stack(fmt::format("{}.mins", i));
					json_print_error(field, "expected array[3]", false);
					json_pop_stack();
					continue;
				}

				if (!value["maxs"].isArray() || value["maxs"].size() != 3)
				{
					json_push_stack(fmt::format("{}.maxs", i));
					json_print_error(field, "expected array[3]", false);
					json_pop_stack();
					continue;
				}

				if (!value["strength"].isInt())
				{
					json_push_stack(fmt::format("{}.strength", i));
					json_print_error(field, "expected int", false);
					json_pop_stack();
					continue;
				}

				p->classname = G_CopyString(value["classname"].asCString(), TAG_LEVEL);
				p->strength = value["strength"].asInt();

				for (int32_t x = 0; x < 3; x++)
				{
					p->mins[x] = value["mins"][x].asInt();
					p->maxs[x] = value["maxs"][x].asInt();
				}
			}
		}
		return;
	default:
		gi.Com_ErrorFmt("Can't read type ID {}", (int32_t) type->id);
		break;
	}
}

bool write_save_struct_json(const void *data, const save_struct_t *structure, bool null_for_empty, Json::Value &output);

#define TYPED_DATA_IS_EMPTY(type, expr) (type->is_empty ? type->is_empty(data) : (expr))

inline bool string_is_high(const char *c)
{
	for (size_t i = 0; i < strlen(c); i++)
		if (c[i] & 128)
			return true;

	return false;
}

inline Json::Value string_to_bytes(const char *c)
{
	Json::Value array(Json::arrayValue);

	for (size_t i = 0; i < strlen(c); i++)
		array.append((int32_t) (unsigned char) c[i]);

	return array;
}

// fetch a JSON value for the specified data.
// if allow_empty is true, false will be returned for
// values that are the same as zero'd memory, to save
// space in the resulting JSON. output will be
// unmodified in that case.
bool write_save_type_json(const void *data, const save_type_t *type, bool null_for_empty, Json::Value &output)
{
	switch (type->id)
	{
	case ST_BOOL:
		if (null_for_empty && TYPED_DATA_IS_EMPTY(type, !*(const bool *) data))
			return false;

		output = Json::Value(*(const bool *) data);
		return true;
	case ST_ENUM:
		if (type->count == 1)
		{
			if (null_for_empty && TYPED_DATA_IS_EMPTY(type, !*(const int8_t *) data))
				return false;

			output = Json::Value(*(const int8_t *) data);
			return true;
		}
		else if (type->count == 2)
		{
			if (null_for_empty && TYPED_DATA_IS_EMPTY(type, !*(const int16_t *) data))
				return false;

			output = Json::Value(*(const int16_t *) data);
			return true;
		}
		else if (type->count == 4)
		{
			if (null_for_empty && TYPED_DATA_IS_EMPTY(type, !*(const int32_t *) data))
				return false;

			output = Json::Value(*(const int32_t *) data);
			return true;
		}
		else if (type->count == 8)
		{
			if (null_for_empty && TYPED_DATA_IS_EMPTY(type, !*(const int64_t *) data))
				return false;

			output = Json::Value(*(const int64_t *) data);
			return true;
		}
		gi.Com_Error("invalid enum length");
		break;
	case ST_INT8:
		if (null_for_empty && TYPED_DATA_IS_EMPTY(type, !*(const int8_t *) data))
			return false;

		output = Json::Value(*(const int8_t *) data);
		return true;
	case ST_INT16:
		if (null_for_empty && TYPED_DATA_IS_EMPTY(type, !*(const int16_t *) data))
			return false;

		output = Json::Value(*(const int16_t *) data);
		return true;
	case ST_INT32:
		if (null_for_empty && TYPED_DATA_IS_EMPTY(type, !*(const int32_t *) data))
			return false;

		output = Json::Value(*(const int32_t *) data);
		return true;
	case ST_INT64:
		if (null_for_empty && TYPED_DATA_IS_EMPTY(type, !*(const int64_t *) data))
			return false;

		output = Json::Value(*(const int64_t *) data);
		return true;
	case ST_UINT8:
		if (null_for_empty && TYPED_DATA_IS_EMPTY(type, !*(const uint8_t *) data))
			return false;

		output = Json::Value(*(const uint8_t *) data);
		return true;
	case ST_UINT16:
		if (null_for_empty && TYPED_DATA_IS_EMPTY(type, !*(const uint16_t *) data))
			return false;

		output = Json::Value(*(const uint16_t *) data);
		return true;
	case ST_UINT32:
		if (null_for_empty && TYPED_DATA_IS_EMPTY(type, !*(const uint32_t *) data))
			return false;

		output = Json::Value(*(const uint32_t *) data);
		return true;
	case ST_UINT64:
		if (null_for_empty && TYPED_DATA_IS_EMPTY(type, !*(const uint64_t *) data))
			return false;

		output = Json::Value(*(const uint64_t *) data);
		return true;
	case ST_FLOAT:
		if (null_for_empty && TYPED_DATA_IS_EMPTY(type, !*(const float *) data))
			return false;

		output = Json::Value(static_cast<double>(*(const float *) data));
		return true;
	case ST_DOUBLE:
		if (null_for_empty && TYPED_DATA_IS_EMPTY(type, !*(const double *) data))
			return false;

		output = Json::Value(*(const double *) data);
		return true;
	case ST_STRING: {
		const char *const *str = reinterpret_cast<const char *const *>(data);
		if (null_for_empty && TYPED_DATA_IS_EMPTY(type, *str == nullptr))
			return false;
		output = *str == nullptr ? Json::Value::nullSingleton() : string_is_high(*str) ? Json::Value(string_to_bytes(*str)) : Json::Value(Json::StaticString(*str));
		return true;
	}
	case ST_FIXED_STRING:
		if (null_for_empty && TYPED_DATA_IS_EMPTY(type, !strlen((const char *) data)))
			return false;
		output = string_is_high((const char *) data) ? Json::Value(string_to_bytes((const char *) data)) : Json::Value(Json::StaticString((const char *) data));
		return true;
	case ST_FIXED_ARRAY: {
		const uint8_t	  *element = (const uint8_t *) data;
		size_t			  i;
		size_t			  element_size;
		save_type_t       element_type;

		if (type->type_resolver)
		{
			element_type = type->type_resolver();
			element_size = get_complex_type_size(element_type);
		}
		else
		{
			element_size = get_simple_type_size((save_type_id_t) type->tag);
			element_type = { (save_type_id_t) type->tag };
		}

		if (null_for_empty)
		{
			if (type->is_empty)
			{
				if (type->is_empty(data))
					return false;
			}
			else
			{
				for (i = 0; i < type->count; i++, element += element_size)
				{
					Json::Value value;
					bool valid_value = write_save_type_json(element, &element_type, !element_type.never_empty, value);

					if (valid_value)
						break;
				}

				if (i == type->count)
					return false;
			}
		}

		element = (const uint8_t *) data;
		Json::Value v(Json::arrayValue);

		for (i = 0; i < type->count; i++, element += element_size)
		{
			Json::Value value;
			write_save_type_json(element, &element_type, false, value);
			v.append(std::move(value));
		}

		output = std::move(v);
		return true;
	}
	case ST_SAVABLE_DYNAMIC: {
		const savable_allocated_memory_t<void, 0> *savptr = (const savable_allocated_memory_t<void, 0> *) data;
		size_t			  i;
		size_t			  element_size;
		save_type_t       element_type;

		if (type->type_resolver)
		{
			element_type = type->type_resolver();
			element_size = get_complex_type_size(element_type);
		}
		else
		{
			element_size = get_simple_type_size((save_type_id_t) type->tag);
			element_type = { (save_type_id_t) type->tag };
		}

		const uint8_t *element = (const uint8_t *) savptr->ptr;

		if (null_for_empty)
		{
			if (type->is_empty)
			{
				if (type->is_empty(data))
					return false;
			}
			else
			{
				for (i = 0; i < savptr->count; i++, element += element_size)
				{
					Json::Value value;
					bool valid_value = write_save_type_json(element, &element_type, !element_type.never_empty, value);

					if (valid_value)
						break;
				}

				if (i == savptr->count)
					return false;
			}
		}

		element = (const uint8_t *) savptr->ptr;
		Json::Value v(Json::arrayValue);

		for (i = 0; i < savptr->count; i++, element += element_size)
		{
			Json::Value value;
			write_save_type_json(element, &element_type, false, value);
			v.append(std::move(value));
		}

		output = std::move(v);
		return true;
	}
	case ST_BITSET:
		return type->write(data, null_for_empty, output);
	case ST_STRUCT: {
		if (type->is_empty && type->is_empty(data))
			return false;

		Json::Value obj;
		bool		valid_value = write_save_struct_json(data, type->structure, true, obj);

		if (null_for_empty && (!valid_value || !obj.size()))
			return false;

		output = std::move(obj);
		return true;
	}
	case ST_ENTITY: {
		const edict_t *entity = *reinterpret_cast<const edict_t *const *>(data);

		if (null_for_empty && TYPED_DATA_IS_EMPTY(type, entity == nullptr))
			return false;

		if (!entity)
		{
			output = Json::Value::nullSingleton();
			return true;
		}

		output = Json::Value(entity->s.number);
		return true;
	}
	case ST_ITEM_POINTER: {
		const gitem_t *item = *reinterpret_cast<const gitem_t *const *>(data);

		if (item != nullptr && item->id != 0)
			if (!strlen(item->classname))
				gi.Com_ErrorFmt("Attempt to persist invalid item {} (index {})", item->pickup_name, (int32_t) item->id);

		if (null_for_empty && TYPED_DATA_IS_EMPTY(type, item == nullptr))
			return false;

		if (item == nullptr)
		{
			output = Json::Value::nullSingleton();
			return true;
		}

		output = Json::Value(Json::StaticString(item->classname));
		return true;
	}
	case ST_ITEM_INDEX: {
		const item_id_t index = *reinterpret_cast<const item_id_t *>(data);

		if (index < IT_NULL || index >= IT_TOTAL)
			gi.Com_ErrorFmt("Attempt to persist invalid item index {}", (int32_t) index);

		const gitem_t *item = GetItemByIndex(index);

		if (index)
			if (!strlen(item->classname))
				gi.Com_ErrorFmt("Attempt to persist invalid item {} (index {})", item->pickup_name, (int32_t) item->id);

		if (null_for_empty && TYPED_DATA_IS_EMPTY(type, item == nullptr))
			return false;

		if (item == nullptr)
		{
			output = Json::Value::nullSingleton();
			return true;
		}

		output = Json::Value(Json::StaticString(item->classname));
		return true;
	}
	case ST_TIME: {
		const gtime_t &time = *(const gtime_t *) data;

		if (null_for_empty && TYPED_DATA_IS_EMPTY(type, !time))
			return false;

		output = Json::Value(time.milliseconds());
		return true;
	}
	case ST_DATA: {
		const save_void_t &ptr = *reinterpret_cast<const save_void_t *>(data);

		if (null_for_empty && TYPED_DATA_IS_EMPTY(type, !ptr))
			return false;

		if (!ptr)
		{
			output = Json::Value::nullSingleton();
			return true;
		}

		if (!ptr.save_list())
		{
			gi.Com_ErrorFmt("Attempt to persist invalid data pointer {} in list {}", ptr.pointer(), type->tag);
			return false;
		}
		
		output = Json::Value(Json::StaticString(ptr.save_list()->name));
		return true;
	}
	case ST_INVENTORY: {
		Json::Value	   inventory = Json::Value(Json::objectValue);
		const int32_t *inventory_ptr = (const int32_t *) data;

		for (item_id_t i = static_cast<item_id_t>(IT_NULL + 1); i < IT_TOTAL; i = static_cast<item_id_t>(i + 1))
		{
			gitem_t *item = GetItemByIndex(i);

			if (!item || !item->classname)
			{
				if (inventory_ptr[i])
					gi.Com_ErrorFmt("Item index {} is in inventory but has no classname", (int32_t) i);

				continue;
			}

			if (inventory_ptr[i])
				inventory[item->classname] = Json::Value(inventory_ptr[i]);
		}

		if (null_for_empty && inventory.empty())
			return false;

		output = std::move(inventory);
		return true;
	}
	case ST_REINFORCEMENTS: {
		const reinforcement_list_t *reinforcement_ptr = (const reinforcement_list_t *) data;

		if (null_for_empty && !reinforcement_ptr->num_reinforcements)
			return false;

		Json::Value	   reinforcements = Json::Value(Json::arrayValue);
		reinforcements.resize(reinforcement_ptr->num_reinforcements);

		for (uint32_t i = 0; i < reinforcement_ptr->num_reinforcements; i++)
		{
			const reinforcement_t *reinforcement = &reinforcement_ptr->reinforcements[i];

			Json::Value obj = Json::Value(Json::objectValue);

			obj["classname"] = Json::StaticString(reinforcement->classname);
			obj["mins"] = Json::Value(Json::arrayValue);
			obj["maxs"] = Json::Value(Json::arrayValue);
			for (int32_t x = 0; x < 3; x++)
			{
				obj["mins"][x] = reinforcement->mins[x];
				obj["maxs"][x] = reinforcement->maxs[x];
			}
			obj["strength"] = reinforcement->strength;

			reinforcements[i] = obj;
		}

		output = std::move(reinforcements);
		return true;
	}
	default:
		gi.Com_ErrorFmt("Can't persist type ID {}", (int32_t) type->id);
	}

	return false;
}

// write the specified data+structure to the JSON object
// referred to by `json`.
bool write_save_struct_json(const void *data, const save_struct_t *structure, bool null_for_empty, Json::Value &output)
{
	Json::Value obj(Json::objectValue);

	for (auto &field : structure->fields)
	{
		const void *p = ((const uint8_t *) data) + field.offset;
		Json::Value value;
		bool		valid_value = write_save_type_json(p, &field.type, !field.type.never_empty, value);

		if (valid_value)
			obj[field.name].swap(value);
	}

	if (null_for_empty && obj.empty())
		return false;

	output = std::move(obj);
	return true;
}

// read the specified data+structure from the JSON object
// referred to by `json`.
void read_save_struct_json(const Json::Value &json, void *data, const save_struct_t *structure)
{
	if (!json.isObject())
	{
		json_print_error("", "expected object", false);
		return;
	}

	//for (auto key : json.getMemberNames())
	for (auto it = json.begin(); it != json.end(); it++)
	{
		//const char		   *classname = key.c_str();
		const char *dummy;
		const char *key = it.memberName(&dummy);
		const Json::Value  &value = *it;//json[key];
		const save_field_t *field;

		for (field = structure->fields.begin(); field != structure->fields.end(); field++)
		{
			if (strcmp(key, field->name) == 0)
			{
				void *p = ((uint8_t *) data) + field->offset;
				read_save_type_json(value, p, &field->type, field->name);
				break;
			}
		}

		if (!field->name)
			json_print_error(key, "unknown field", false);
	}
}

#include <fstream>
#include <memory>

static Json::Value parseJson(const char *jsonString)
{
	Json::CharReaderBuilder reader;
	reader["allowSpecialFloats"] = true;
	Json::Value		  json;
	JSONCPP_STRING	  errs;
	std::stringstream ss(jsonString, std::ios_base::in | std::ios_base::binary);

	if (!Json::parseFromStream(reader, ss, &json, &errs))
		gi.Com_ErrorFmt("Couldn't decode JSON: {}", errs.c_str());

	if (!json.isObject())
		gi.Com_Error("expected object at root");

	return json;
}

static char *saveJson(const Json::Value &json, size_t *out_size)
{
	Json::StreamWriterBuilder builder;
	builder["indentation"] = "\t";
	builder["useSpecialFloats"] = true;
	const std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
	std::stringstream						  ss(std::ios_base::out | std::ios_base::binary);
	writer->write(json, &ss);
	*out_size = ss.tellp();
	char *const out = static_cast<char *>(gi.TagMalloc(*out_size + 1, TAG_GAME));
	// FIXME: some day...
	std::string v = ss.str();
	memcpy(out, v.c_str(), *out_size);
	out[*out_size] = '\0';
	return out;
}

// new entry point for WriteGame.
// returns pointer to TagMalloc'd JSON string.
char *WriteGameJson(bool autosave, size_t *out_size)
{
	if (!autosave)
		SaveClientData();

	Json::Value json(Json::objectValue);

	json["save_version"] = SAVE_FORMAT_VERSION;
	// TODO: engine version ID?

	// write game
	game.autosaved = autosave;
	write_save_struct_json(&game, &game_locals_t_savestruct, false, json["game"]);
	game.autosaved = false;

	// write clients
	Json::Value clients(Json::arrayValue);
	for (size_t i = 0; i < game.maxclients; i++)
	{
		Json::Value v;
		write_save_struct_json(&game.clients[i], &gclient_t_savestruct, false, v);
		clients.append(std::move(v));
	}
	json["clients"] = std::move(clients);

	return saveJson(json, out_size);
}

void G_PrecacheInventoryItems();

// new entry point for ReadGame.
// takes in pointer to JSON data. does
// not store or modify it.
void ReadGameJson(const char *jsonString)
{
	gi.FreeTags(TAG_GAME);

	Json::Value json = parseJson(jsonString);

	uint32_t max_entities = game.maxentities;
	uint32_t max_clients = game.maxclients;
	
	game = {};
	g_edicts = (edict_t *) gi.TagMalloc(max_entities * sizeof(g_edicts[0]), TAG_GAME);
	game.clients = (gclient_t *) gi.TagMalloc(max_clients * sizeof(game.clients[0]), TAG_GAME);
	globals.edicts = g_edicts;

	// read game
	json_push_stack("game");
	read_save_struct_json(json["game"], &game, &game_locals_t_savestruct);
	json_pop_stack();

	// read clients
	const Json::Value &clients = json["clients"];

	if (!clients.isArray())
		gi.Com_Error("expected \"clients\" to be array");
	else if (clients.size() != game.maxclients)
		gi.Com_Error("mismatched client size");

	size_t i = 0;

	for (auto &v : clients)
	{
		json_push_stack(fmt::format("clients[{}]", i));
		read_save_struct_json(v, &game.clients[i++], &gclient_t_savestruct);
		json_pop_stack();
	}

	G_PrecacheInventoryItems();
}

// new entry point for WriteLevel.
// returns pointer to TagMalloc'd JSON string.
char *WriteLevelJson(bool transition, size_t *out_size)
{
	// update current level entry now, just so we can
	// use gamemap to test EOU
	G_UpdateLevelEntry();

	Json::Value json(Json::objectValue);

	json["save_version"] = SAVE_FORMAT_VERSION;

	// write level
	write_save_struct_json(&level, &level_locals_t_savestruct, false, json["level"]);

	// write entities
	Json::Value entities(Json::objectValue);
	char		number[16];

	for (uint32_t i = 0; i < globals.num_edicts; i++)
	{
		if (!globals.edicts[i].inuse)
			continue;
        // clear all the client inuse flags before saving so that
        // when the level is re-entered, the clients will spawn
        // at spawn points instead of occupying body shells
		else if (transition && i >= 1 && i <= game.maxclients)
			continue;

		auto result = std::to_chars(number, number + sizeof(number) - 1, i);

		if (result.ec == std::errc())
			*result.ptr = '\0';
		else
			gi.Com_ErrorFmt("error formatting number: {}", std::make_error_code(result.ec).message());

		write_save_struct_json(&globals.edicts[i], &edict_t_savestruct, false, entities[number]);
	}

	json["entities"] = std::move(entities);

	return saveJson(json, out_size);
}

// new entry point for ReadLevel.
// takes in pointer to JSON data. does
// not store or modify it.
void ReadLevelJson(const char *jsonString)
{
	// free any dynamic memory allocated by loading the level
	// base state
	gi.FreeTags(TAG_LEVEL);

	Json::Value json = parseJson(jsonString);

	// wipe all the entities
	memset(g_edicts, 0, game.maxentities * sizeof(g_edicts[0]));
	globals.num_edicts = game.maxclients + 1;

	// read level
	json_push_stack("level");
	read_save_struct_json(json["level"], &level, &level_locals_t_savestruct);
	json_pop_stack();

	// read entities
	const Json::Value &entities = json["entities"];

	if (!entities.isObject())
		gi.Com_Error("expected \"entities\" to be object");

	//for (auto key : json.getMemberNames())
	for (auto it = entities.begin(); it != entities.end(); it++)
	{
		//const char		   *classname = key.c_str();
		const char *dummy;
		const char *id = it.memberName(&dummy);
		const Json::Value  &value = *it;//json[key];
		uint32_t		   number = strtoul(id, nullptr, 10);

		if (number >= globals.num_edicts)
			globals.num_edicts = number + 1;

		edict_t *ent = &g_edicts[number];
		G_InitEdict(ent);
		json_push_stack(fmt::format("entities[{}]", number));
		read_save_struct_json(value, ent, &edict_t_savestruct);
		json_pop_stack();
		gi.linkentity(ent);
	}

	// mark all clients as unconnected
	for (size_t i = 0; i < game.maxclients; i++)
	{
		edict_t *ent = &g_edicts[i + 1];
		ent->client = game.clients + i;
		ent->client->pers.connected = false;
		ent->client->pers.spawned = false;
	}

	// do any load time things at this point
	for (size_t i = 0; i < globals.num_edicts; i++)
	{
		edict_t *ent = &g_edicts[i];

		if (!ent->inuse)
			continue;

		// fire any cross-level/unit triggers
		if (ent->classname)
			if (strcmp(ent->classname, "target_crosslevel_target") == 0 ||
				strcmp(ent->classname, "target_crossunit_target") == 0)
				ent->nextthink = level.time + gtime_t::from_sec(ent->delay);
	}

	G_PrecacheInventoryItems();

	// clear cached indices
	cached_soundindex::reset_all();
	cached_modelindex::reset_all();
	cached_imageindex::reset_all();

	G_LoadShadowLights();
}

// [Paril-KEX]
bool G_CanSave()
{
    if (game.maxclients == 1 && g_edicts[1].health <= 0)
    {
        gi.LocClient_Print(&g_edicts[1], PRINT_CENTER, "$g_no_save_dead");
        return false;
    }
	// don't allow saving during cameras/intermissions as this
	// causes the game to act weird when these are loaded
	else if (level.intermissiontime)
	{
		return false;
	}

	return true;
}

/*static*/ template<> cached_soundindex *cached_soundindex::head = nullptr;
/*static*/ template<> cached_modelindex *cached_modelindex::head = nullptr;
/*static*/ template<> cached_imageindex *cached_imageindex::head = nullptr;