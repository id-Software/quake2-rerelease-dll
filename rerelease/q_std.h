// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#pragma once

// q_std.h -- 'standard' library stuff for game module
// not meant to be included by engine, etc

#include <cmath>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <cstddef>
#include <cinttypes>
#include <ctime>
#include <type_traits>
#include <algorithm>
#include <array>
#include <string_view>
#include <numeric>
#include <functional>

// format!
#ifndef USE_CPP20_FORMAT
#ifdef __cpp_lib_format
#define USE_CPP20_FORMAT 1
#endif
#endif

#if USE_CPP20_FORMAT
#include <format>
namespace fmt = std;
#define FMT_STRING(s) s
#else
#include <fmt/format.h>
#endif

struct g_fmt_data_t {
	char string[2][4096];
	int  istr;
};

// static data for fmt; internal, do not touch
extern g_fmt_data_t g_fmt_data;

// like fmt::format_to_n, but automatically null terminates the output;
// returns the length of the string written (up to N)
#ifdef USE_CPP20_FORMAT
#define G_FmtTo_ G_FmtTo

template<size_t N, typename... Args>
inline size_t G_FmtTo(char (&buffer)[N], std::format_string<Args...> format_str, Args &&... args)
#else
#define G_FmtTo(buffer, str, ...) \
	G_FmtTo_(buffer, FMT_STRING(str), __VA_ARGS__)

template<size_t N, typename S, typename... Args>
inline size_t G_FmtTo_(char (&buffer)[N], const S &format_str, Args &&... args)
#endif
{
    auto end = fmt::format_to_n(buffer, N - 1, format_str, std::forward<Args>(args)...);

	*(end.out) = '\0';

	return end.out - buffer;
}

// format to temp buffers; doesn't use heap allocation
// unlike `fmt::format` does directly
#ifdef USE_CPP20_FORMAT
template<typename... Args>
[[nodiscard]] inline std::string_view G_Fmt(std::format_string<Args...> format_str, Args &&... args)
#else

#define G_Fmt(str, ...) \
	G_Fmt_(FMT_STRING(str), __VA_ARGS__)

template<typename S, typename... Args>
[[nodiscard]] inline std::string_view G_Fmt_(const S &format_str, Args &&... args)
#endif
{
	g_fmt_data.istr ^= 1;

	size_t len = G_FmtTo_(g_fmt_data.string[g_fmt_data.istr], format_str, std::forward<Args>(args)...);

	return std::string_view(g_fmt_data.string[g_fmt_data.istr], len);
}

// fmt::join replacement
template<typename T>
std::string join_strings(const T &cont, const char *separator)
{
	if (cont.empty())
		return "";

	return std::accumulate(++cont.begin(), cont.end(), *cont.begin(),
		[separator](auto &&a, auto &&b) -> auto & {
			a += separator;
			a += b;
			return a;
		});
}

using byte = uint8_t;

// note: only works on actual arrays
#define q_countof(a) std::extent_v<decltype(a)>

using std::max;
using std::min;
using std::clamp;

template<typename T>
constexpr T lerp(T from, T to, float t)
{
	return (to * t) + (from * (1.f - t));
}

// angle indexes
enum
{
	PITCH,
	YAW,
	ROLL
};

/*
==============================================================

MATHLIB

==============================================================
*/

constexpr double PI = 3.14159265358979323846; // matches value in gcc v2 math.h
constexpr float PIf = static_cast<float>(PI);

[[nodiscard]] constexpr float RAD2DEG(float x)
{
	return (x * 180.0f / PIf);
}

[[nodiscard]] constexpr float DEG2RAD(float x)
{
	return (x * PIf / 180.0f);
}

/*
=============
G_AddBlend
=============
*/
inline void G_AddBlend(float r, float g, float b, float a, std::array<float, 4> &v_blend)
{
	if (a <= 0)
		return;

	float a2 = v_blend[3] + (1 - v_blend[3]) * a; // new total alpha
	float a3 = v_blend[3] / a2;					// fraction of color from old

	v_blend[0] = v_blend[0] * a3 + r * (1 - a3);
	v_blend[1] = v_blend[1] * a3 + g * (1 - a3);
	v_blend[2] = v_blend[2] * a3 + b * (1 - a3);
	v_blend[3] = a2;
}

//============================================================================

/*
===============
LerpAngle

===============
*/
[[nodiscard]] constexpr float LerpAngle(float a2, float a1, float frac)
{
	if (a1 - a2 > 180)
		a1 -= 360;
	if (a1 - a2 < -180)
		a1 += 360;
	return a2 + frac * (a1 - a2);
}

[[nodiscard]] inline float anglemod(float a)
{
	float v = fmod(a, 360.0f);

	if (v < 0)
		return 360.f + v;

	return v;
}

#include "q_vec3.h"

//=============================================

char *COM_ParseEx(const char **data_p, const char *seps, char *buffer = nullptr, size_t buffer_size = 0);

// data is an in/out parm, returns a parsed out token
inline char *COM_Parse(const char **data_p, char *buffer = nullptr, size_t buffer_size = 0)
{
	return COM_ParseEx(data_p, "\r\n\t ", buffer, buffer_size);
}

//=============================================

// portable case insensitive compare
[[nodiscard]] int Q_strcasecmp(const char *s1, const char *s2);
[[nodiscard]] int Q_strncasecmp(const char *s1, const char *s2, size_t n);

// BSD string utils - haleyjd
size_t Q_strlcpy(char* dst, const char* src, size_t siz);
size_t Q_strlcat(char* dst, const char* src, size_t siz);

// EOF
