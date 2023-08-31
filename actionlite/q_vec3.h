// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#pragma once

// q_vec3 - vec3 stuff
#include <stdexcept>
#include <type_traits>

using nullptr_t = std::nullptr_t;

struct vec3_t
{
	float x, y, z;

	[[nodiscard]] constexpr const float &operator[](size_t i) const
	{
		if (i == 0)
			return x;
		else if (i == 1)
			return y;
		else if (i == 2)
			return z;
		throw std::out_of_range("i");
	}

	[[nodiscard]] constexpr float &operator[](size_t i)
	{
		if (i == 0)
			return x;
		else if (i == 1)
			return y;
		else if (i == 2)
			return z;
		throw std::out_of_range("i");
	}

	// comparison
	[[nodiscard]] constexpr bool equals(const vec3_t &v) const
	{
		return x == v.x && y == v.y && z == v.z;
	}
	[[nodiscard]] inline bool equals(const vec3_t &v, const float &epsilon) const
	{
		return fabsf(x - v.x) <= epsilon && fabsf(y - v.y) <= epsilon && fabsf(z - v.z) <= epsilon;
	}
	[[nodiscard]] constexpr bool operator==(const vec3_t &v) const
	{
		return equals(v);
	}
	[[nodiscard]] constexpr bool operator!=(const vec3_t &v) const
	{
		return !(*this == v);
	}
	[[nodiscard]] constexpr explicit operator bool() const
	{
		return x || y || z;
	}

	// dot
	[[nodiscard]] constexpr float dot(const vec3_t &v) const
	{
		return (x * v.x) + (y * v.y) + (z * v.z);
	}
	[[nodiscard]] constexpr vec3_t scaled(const vec3_t &v) const
	{
		return { x * v.x, y * v.y, z * v.z };
	}
	constexpr vec3_t &scale(const vec3_t &v)
	{
		*this = this->scaled(v);
		return *this;
	}

	// basic operators
	[[nodiscard]] constexpr vec3_t operator-(const vec3_t &v) const
	{
		return { x - v.x, y - v.y, z - v.z };
	}
	[[nodiscard]] constexpr vec3_t operator+(const vec3_t &v) const
	{
		return { x + v.x, y + v.y, z + v.z };
	}
	[[nodiscard]] constexpr vec3_t operator/(const vec3_t &v) const
	{
		return { x / v.x, y / v.y, z / v.z };
	}
	template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T> || std::is_integral_v<T>>>
	[[nodiscard]] constexpr vec3_t operator/(const T &v) const
	{
		return { static_cast<float>(x / v), static_cast<float>(y / v), static_cast<float>(z / v) };
	}
	template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T> || std::is_integral_v<T>>>
	[[nodiscard]] constexpr vec3_t operator*(const T &v) const
	{
		return { static_cast<float>(x * v), static_cast<float>(y * v), static_cast<float>(z * v) };
	}
	[[nodiscard]] constexpr vec3_t operator-() const
	{
		return { -x, -y, -z };
	}

	constexpr vec3_t &operator-=(const vec3_t &v)
	{
		*this = *this - v;
		return *this;
	}
	constexpr vec3_t &operator+=(const vec3_t &v)
	{
		*this = *this + v;
		return *this;
	}
	constexpr vec3_t &operator/=(const vec3_t &v)
	{
		*this = *this / v;
		return *this;
	}
	template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T> || std::is_integral_v<T>>>
	constexpr vec3_t &operator/=(const T &v)
	{
		*this = *this / v;
		return *this;
	}
	template<typename T, typename = std::enable_if_t<std::is_floating_point_v<T> || std::is_integral_v<T>>>
	constexpr vec3_t &operator*=(const T &v)
	{
		*this = *this * v;
		return *this;
	}

	// operations
	[[nodiscard]] constexpr float lengthSquared() const
	{
		return this->dot(*this);
	}
	[[nodiscard]] inline float length() const
	{
		return sqrtf(lengthSquared());
	}
	[[nodiscard]] inline vec3_t normalized() const
	{
		float len = length();
		return len ? (*this * (1.f / len)) : *this;
	}
	[[nodiscard]] inline vec3_t normalized(float &len) const
	{
		len = length();
		return len ? (*this * (1.f / len)) : *this;
	}
	inline float normalize()
	{
		float len = length();

		if (len)
			*this *= (1.f / len);

		return len;
	}
	[[nodiscard]] constexpr vec3_t cross(const vec3_t &v) const
	{
		return {
			y * v.z - z * v.y,
			z * v.x - x * v.z,
			x * v.y - y * v.x
		};
	}
};

constexpr vec3_t vec3_origin{};

inline void AngleVectors(const vec3_t &angles, vec3_t *forward, vec3_t *right, vec3_t *up)
{
	float angle = angles[YAW] * (PIf * 2 / 360);
	float sy = sinf(angle);
	float cy = cosf(angle);
	angle = angles[PITCH] * (PIf * 2 / 360);
	float sp = sinf(angle);
	float cp = cosf(angle);
	angle = angles[ROLL] * (PIf * 2 / 360);
	float sr = sinf(angle);
	float cr = cosf(angle);

	if (forward)
	{
		forward->x = cp * cy;
		forward->y = cp * sy;
		forward->z = -sp;
	}
	if (right)
	{
		right->x = (-1 * sr * sp * cy + -1 * cr * -sy);
		right->y = (-1 * sr * sp * sy + -1 * cr * cy);
		right->z = -1 * sr * cp;
	}
	if (up)
	{
		up->x = (cr * sp * cy + -sr * -sy);
		up->y = (cr * sp * sy + -sr * cy);
		up->z = cr * cp;
	}
}

struct angle_vectors_t {
	vec3_t forward, right, up;
};

// for destructuring
inline angle_vectors_t AngleVectors(const vec3_t &angles)
{
	angle_vectors_t v;
	AngleVectors(angles, &v.forward, &v.right, &v.up);
	return v;
}

// silly wrappers to allow old C code to work
inline void AngleVectors(const vec3_t &angles, vec3_t &forward, vec3_t &right, vec3_t &up)
{
	AngleVectors(angles, &forward, &right, &up);
}
inline void AngleVectors(const vec3_t &angles, vec3_t &forward, vec3_t &right, nullptr_t)
{
	AngleVectors(angles, &forward, &right, nullptr);
}
inline void AngleVectors(const vec3_t &angles, vec3_t &forward, nullptr_t, vec3_t &up)
{
	AngleVectors(angles, &forward, nullptr, &up);
}
inline void AngleVectors(const vec3_t &angles, vec3_t &forward, nullptr_t, nullptr_t)
{
	AngleVectors(angles, &forward, nullptr, nullptr);
}
inline void AngleVectors(const vec3_t &angles, nullptr_t, nullptr_t, vec3_t &up)
{
	AngleVectors(angles, nullptr, nullptr, &up);
}
inline void AngleVectors(const vec3_t &angles, nullptr_t, vec3_t &right, nullptr_t)
{
	AngleVectors(angles, nullptr, &right, nullptr);
}

inline void ClearBounds(vec3_t &mins, vec3_t &maxs)
{
	mins[0] = mins[1] = mins[2] = std::numeric_limits<float>::infinity();
	maxs[0] = maxs[1] = maxs[2] = -std::numeric_limits<float>::infinity();
}

inline void AddPointToBounds(const vec3_t &v, vec3_t &mins, vec3_t &maxs)
{
	for (int i = 0; i < 3; i++)
	{
		float val = v[i];
		if (val < mins[i])
			mins[i] = val;
		if (val > maxs[i])
			maxs[i] = val;
	}
}

[[nodiscard]] constexpr vec3_t ProjectPointOnPlane(const vec3_t &p, const vec3_t &normal)
{
	float inv_denom = 1.0f / normal.dot(normal);
	float d = normal.dot(p) * inv_denom;
	return p - ((normal * inv_denom) * d);
}

/*
** assumes "src" is normalized
*/
[[nodiscard]] inline vec3_t PerpendicularVector(const vec3_t &src)
{
	int	   pos;
	int	   i;
	float  minelem = 1.0F;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for (pos = 0, i = 0; i < 3; i++)
	{
		if (fabsf(src[i]) < minelem)
		{
			pos = i;
			minelem = fabsf(src[i]);
		}
	}
	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	/*
	** project the point onto the plane defined by src & normalize the result
	*/
	return ProjectPointOnPlane(tempvec, src).normalized();
}

using mat3_t = std::array<std::array<float, 3>, 3>;

/*
================
R_ConcatRotations
================
*/
[[nodiscard]] constexpr mat3_t R_ConcatRotations(const mat3_t &in1, const mat3_t &in2)
{
	return {
		std::array<float, 3> {
			in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0],
			in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1],
			in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2]
		},
		{
			in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0],
			in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1],
			in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2]
		},
		{
			in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0],
			in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1],
			in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2]
		}
	};
}

[[nodiscard]] inline vec3_t RotatePointAroundVector(const vec3_t &dir, const vec3_t &point, float degrees)
{
	mat3_t	m;
	mat3_t  im;
	mat3_t  zrot;
	mat3_t  rot;
	vec3_t vr, vup, vf;

	vf = dir;

	vr = PerpendicularVector(dir);
	vup = vr.cross(vf);

	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	im = m;

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	zrot = {};
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;

	zrot[0][0] = cosf(DEG2RAD(degrees));
	zrot[0][1] = sinf(DEG2RAD(degrees));
	zrot[1][0] = -sinf(DEG2RAD(degrees));
	zrot[1][1] = cosf(DEG2RAD(degrees));

	rot = R_ConcatRotations(R_ConcatRotations(m, zrot), im);

	return {
		rot[0][0] * point[0] + rot[0][1] * point[1] + rot[0][2] * point[2],
		rot[1][0] * point[0] + rot[1][1] * point[1] + rot[1][2] * point[2],
		rot[2][0] * point[0] + rot[2][1] * point[1] + rot[2][2] * point[2]
	};
}

[[nodiscard]] constexpr vec3_t closest_point_to_box(const vec3_t &from, const vec3_t &absmins, const vec3_t &absmaxs)
{
    return {
		(from[0] < absmins[0]) ? absmins[0] : (from[0] > absmaxs[0]) ? absmaxs[0] : from[0],
		(from[1] < absmins[1]) ? absmins[1] : (from[1] > absmaxs[1]) ? absmaxs[1] : from[1],
		(from[2] < absmins[2]) ? absmins[2] : (from[2] > absmaxs[2]) ? absmaxs[2] : from[2]
	};
}

[[nodiscard]] inline float distance_between_boxes(const vec3_t &absminsa, const vec3_t &absmaxsa, const vec3_t &absminsb, const vec3_t &absmaxsb)
{
    float len = 0;

	for (size_t i = 0; i < 3; i++)
    {
        if (absmaxsa[i] < absminsb[i])
        {
            float d = absmaxsa[i] - absminsb[i];
            len += d * d;
        }
        else if (absminsa[i] > absmaxsb[i])
        {
            float d = absminsa[i] - absmaxsb[i];
            len += d * d;
        }
    }
    
    return sqrt(len);
}

[[nodiscard]] constexpr bool boxes_intersect(const vec3_t &amins, const vec3_t &amaxs, const vec3_t &bmins, const vec3_t &bmaxs)
{
	return amins.x <= bmaxs.x &&
		   amaxs.x >= bmins.x &&
		   amins.y <= bmaxs.y &&
		   amaxs.y >= bmins.y &&
		   amins.z <= bmaxs.z &&
		   amaxs.z >= bmins.z;
}

/*
==================
ClipVelocity

Slide off of the impacting object
==================
*/
constexpr float STOP_EPSILON = 0.1f;

[[nodiscard]] constexpr vec3_t ClipVelocity(const vec3_t &in, const vec3_t &normal, float overbounce)
{
	float dot = in.dot(normal);
	vec3_t out = in + (normal * (-2 * dot));
	out *= overbounce - 1.f;

	if (out.lengthSquared() < STOP_EPSILON * STOP_EPSILON)
		out = {};

	return out;
}

[[nodiscard]] constexpr vec3_t SlideClipVelocity(const vec3_t &in, const vec3_t &normal, float overbounce)
{
	float backoff = in.dot(normal) * overbounce;
	vec3_t out = in - (normal * backoff);

	for (int i = 0; i < 3; i++)
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;

	return out;
}

[[nodiscard]] inline float vectoyaw(const vec3_t &vec)
{
	// PMM - fixed to correct for pitch of 0
	if (vec[PITCH] == 0)
	{
		if (vec[YAW] == 0)
			return 0.f;
		else if (vec[YAW] > 0)
			return 90.f;
		else
			return 270.f;
	}

	float yaw = (atan2(vec[YAW], vec[PITCH]) * (180.f / PIf));

	if (yaw < 0)
		yaw += 360;

	return yaw;
}

[[nodiscard]] inline vec3_t vectoangles(const vec3_t &vec)
{
	float forward;
	float yaw, pitch;

	if (vec[1] == 0 && vec[0] == 0)
	{
		if (vec[2] > 0)
			return { -90.f, 0.f, 0.f };
		else
			return { -270.f, 0.f, 0.f };
	}

	// PMM - fixed to correct for pitch of 0
	if (vec[0])
		yaw = (atan2(vec[1], vec[0]) * (180.f / PIf));
	else if (vec[1] > 0)
		yaw = 90;
	else
		yaw = 270;

	if (yaw < 0)
		yaw += 360;

	forward = sqrt(vec[0] * vec[0] + vec[1] * vec[1]);
	pitch = (atan2(vec[2], forward) * (180.f / PIf));

	if (pitch < 0)
		pitch += 360;

	return { -pitch, yaw, 0 };
}

[[nodiscard]] constexpr vec3_t G_ProjectSource(const vec3_t &point, const vec3_t &distance, const vec3_t &forward, const vec3_t &right)
{
	return point + (forward * distance[0]) + (right * distance[1]) + vec3_t{0.f, 0.f, distance[2]};
}

[[nodiscard]] constexpr vec3_t G_ProjectSource2(const vec3_t &point, const vec3_t &distance, const vec3_t &forward, const vec3_t &right, const vec3_t &up)
{
	return point + (forward * distance[0]) + (right * distance[1]) + (up * distance[2]);
}

[[nodiscard]] inline vec3_t slerp(const vec3_t &from, const vec3_t &to, float t)
{
    float dot = from.dot(to);
    float aFactor;
    float bFactor;
    if (fabsf(dot) > 0.9995f)
    {
        aFactor = 1.0f - t;
        bFactor = t;
    }
    else
    {
        float ang = acos(dot);
        float sinOmega = sin(ang);
        float sinAOmega = sin((1.0f - t) * ang);
        float sinBOmega = sin(t * ang);
        aFactor = sinAOmega / sinOmega;
        bFactor = sinBOmega / sinOmega;
    }
    return from * aFactor + to * bFactor;
}

// Fmt support
template<>
struct fmt::formatter<vec3_t> : fmt::formatter<float>
{
    template<typename FormatContext>
    auto format(const vec3_t &p, FormatContext &ctx) -> decltype(ctx.out())
    {
		auto out = fmt::formatter<float>::format(p.x, ctx);
        out = fmt::format_to(out, " ");
		ctx.advance_to(out);
		out = fmt::formatter<float>::format(p.y, ctx);
        out = fmt::format_to(out, " ");
		ctx.advance_to(out);
		return fmt::formatter<float>::format(p.z, ctx);
    }
};