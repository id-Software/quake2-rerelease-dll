// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
#include "../g_local.h"

// RAFAEL
/*
	RipperGun
*/

void weapon_ionripper_fire(edict_t *ent)
{
	vec3_t tempang;
	int	   damage;

	if (deathmatch->integer)
		// tone down for deathmatch
		damage = 30;
	else
		damage = 50;

	if (is_quad)
		damage *= damage_multiplier;

	tempang = ent->client->v_angle;
	tempang[YAW] += crandom();

	vec3_t start, dir;
	P_ProjectSource(ent, tempang, { 16, 7, -8 }, start, dir);

	P_AddWeaponKick(ent, ent->client->v_forward * -3, { -3.f, 0.f, 0.f });

	fire_ionripper(ent, start, dir, damage, 500, EF_IONRIPPER);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_IONRIPPER | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	G_RemoveAmmo(ent);
}

void Weapon_Ionripper(edict_t *ent)
{
	constexpr int pause_frames[] = { 36, 0 };
	constexpr int fire_frames[] = { 6, 0 };

	Weapon_Generic(ent, 5, 7, 36, 39, pause_frames, fire_frames, weapon_ionripper_fire);
}

//
//	Phalanx
//

void weapon_phalanx_fire(edict_t *ent)
{
	vec3_t v;
	int	   damage;
	float  damage_radius;
	int	   radius_damage;

	damage = irandom(70, 80);
	radius_damage = 120;
	damage_radius = 120;

	if (is_quad)
	{
		damage *= damage_multiplier;
		radius_damage *= damage_multiplier;
	}

	vec3_t dir;

	if (ent->client->ps.gunframe == 8)
	{
		v[PITCH] = ent->client->v_angle[PITCH];
		v[YAW] = ent->client->v_angle[YAW] - 1.5f;
		v[ROLL] = ent->client->v_angle[ROLL];

		vec3_t start;
		P_ProjectSource(ent, v, { 0, 8, -8 }, start, dir);

		radius_damage = 30;
		damage_radius = 120;

		fire_plasma(ent, start, dir, damage, 725, damage_radius, radius_damage);

		// send muzzle flash
		gi.WriteByte(svc_muzzleflash);
		gi.WriteEntity(ent);
		gi.WriteByte(MZ_PHALANX2 | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS, false);
		
		G_RemoveAmmo(ent);
	}
	else
	{
		v[PITCH] = ent->client->v_angle[PITCH];
		v[YAW] = ent->client->v_angle[YAW] + 1.5f;
		v[ROLL] = ent->client->v_angle[ROLL];

		vec3_t start;
		P_ProjectSource(ent, v, { 0, 8, -8 }, start, dir);

		fire_plasma(ent, start, dir, damage, 725, damage_radius, radius_damage);

		// send muzzle flash
		gi.WriteByte(svc_muzzleflash);
		gi.WriteEntity(ent);
		gi.WriteByte(MZ_PHALANX | is_silenced);
		gi.multicast(ent->s.origin, MULTICAST_PVS, false);

		PlayerNoise(ent, start, PNOISE_WEAPON);
	}

	P_AddWeaponKick(ent, ent->client->v_forward * -2, { -2.f, 0.f, 0.f });
}

void Weapon_Phalanx(edict_t *ent)
{
	constexpr int pause_frames[] = { 29, 42, 55, 0 };
	constexpr int fire_frames[] = { 7, 8, 0 };

	Weapon_Generic(ent, 5, 20, 58, 63, pause_frames, fire_frames, weapon_phalanx_fire);
}

/*
======================================================================

TRAP

======================================================================
*/

constexpr gtime_t TRAP_TIMER = 5_sec;
constexpr float TRAP_MINSPEED = 300.f;
constexpr float TRAP_MAXSPEED = 700.f;

void weapon_trap_fire(edict_t *ent, bool held)
{
	int	  speed;

	vec3_t start, dir;
	// Paril: kill sideways angle on grenades
	// limit upwards angle so you don't throw behind you
	P_ProjectSource(ent, { max(-62.5f, ent->client->v_angle[0]), ent->client->v_angle[1], ent->client->v_angle[2] }, { 8, 0, -8 }, start, dir);

	gtime_t timer = ent->client->grenade_time - level.time;
	speed = (int) (ent->health <= 0 ? TRAP_MINSPEED : min(TRAP_MINSPEED + (TRAP_TIMER - timer).seconds() * ((TRAP_MAXSPEED - TRAP_MINSPEED) / TRAP_TIMER.seconds()), TRAP_MAXSPEED));

	ent->client->grenade_time = 0_ms;

	fire_trap(ent, start, dir, speed);

	G_RemoveAmmo(ent, 1);
}

void Weapon_Trap(edict_t *ent)
{
	constexpr int pause_frames[] = { 29, 34, 39, 48, 0 };

	Throw_Generic(ent, 15, 48, 5, "weapons/trapcock.wav", 11, 12, pause_frames, false, "weapons/traploop.wav", weapon_trap_fire, false);
}