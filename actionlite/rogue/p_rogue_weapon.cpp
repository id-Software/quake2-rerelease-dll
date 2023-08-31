// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "../g_local.h"
#include "../m_player.h"

void weapon_prox_fire(edict_t *ent)
{
	vec3_t start, dir;
	// Paril: kill sideways angle on grenades
	// limit upwards angle so you don't fire behind you
	P_ProjectSource(ent, { max(-62.5f, ent->client->v_angle[0]), ent->client->v_angle[1], ent->client->v_angle[2] }, { 8, 0, -8 }, start, dir);

	P_AddWeaponKick(ent, ent->client->v_forward * -2, { -1.f, 0.f, 0.f });

	fire_prox(ent, start, dir, damage_multiplier, 600);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_PROX | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);
	
	G_RemoveAmmo(ent);
}

void Weapon_ProxLauncher(edict_t *ent)
{
	constexpr int pause_frames[] = { 34, 51, 59, 0 };
	constexpr int fire_frames[] = { 6, 0 };

	Weapon_Generic(ent, 5, 16, 59, 64, pause_frames, fire_frames, weapon_prox_fire);
}

void weapon_tesla_fire(edict_t *ent, bool held)
{
	vec3_t start, dir;
	// Paril: kill sideways angle on grenades
	// limit upwards angle so you don't throw behind you
	P_ProjectSource(ent, { max(-62.5f, ent->client->v_angle[0]), ent->client->v_angle[1], ent->client->v_angle[2] }, { 0, 0, -22 }, start, dir);

	gtime_t timer = ent->client->grenade_time - level.time;
	int	  speed = (int) (ent->health <= 0 ? GRENADE_MINSPEED : min(GRENADE_MINSPEED + (GRENADE_TIMER - timer).seconds() * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER.seconds()), GRENADE_MAXSPEED));
	
	ent->client->grenade_time = 0_ms;

	fire_tesla(ent, start, dir, damage_multiplier, speed);

	G_RemoveAmmo(ent, 1);
}

void Weapon_Tesla(edict_t *ent)
{
	constexpr int pause_frames[] = { 21, 0 };

	Throw_Generic(ent, 8, 32, -1, nullptr, 1, 2, pause_frames, false, nullptr, weapon_tesla_fire, false);
}

//======================================================================
// ROGUE MODS BELOW
//======================================================================

//
// CHAINFIST
//
constexpr int32_t CHAINFIST_REACH = 24;

void weapon_chainfist_fire(edict_t *ent)
{
	if (!(ent->client->buttons & BUTTON_ATTACK))
	{
		if (ent->client->ps.gunframe == 13 ||
			ent->client->ps.gunframe == 23 ||
			ent->client->ps.gunframe >= 32)
		{
			ent->client->ps.gunframe = 33;
			return;
		}
	}

	int damage = 7;

	if (deathmatch->integer)
		damage = 15;

	if (is_quad)
		damage *= damage_multiplier;

	// set start point
	vec3_t start, dir;

	P_ProjectSource(ent, ent->client->v_angle, { 0, 0, -4 }, start, dir);

	if (fire_player_melee(ent, start, dir, CHAINFIST_REACH, damage, 100, MOD_CHAINFIST))
	{
		if (ent->client->empty_click_sound < level.time)
		{
			ent->client->empty_click_sound = level.time + 500_ms;
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/sawslice.wav"), 1.f, ATTN_NORM, 0.f);
		}
	}

	PlayerNoise(ent, start, PNOISE_WEAPON);

	ent->client->ps.gunframe++;
	
	if (ent->client->buttons & BUTTON_ATTACK)
	{
		if (ent->client->ps.gunframe == 12)
			ent->client->ps.gunframe = 14;
		else if (ent->client->ps.gunframe == 22)
			ent->client->ps.gunframe = 24;
		else if (ent->client->ps.gunframe >= 32)
			ent->client->ps.gunframe = 7;
	}

	// start the animation
	if (ent->client->anim_priority != ANIM_ATTACK || frandom() < 0.25f)
	{
		ent->client->anim_priority = ANIM_ATTACK;
		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
		{
			ent->s.frame = FRAME_crattak1 - 1;
			ent->client->anim_end = FRAME_crattak9;
		}
		else
		{
			ent->s.frame = FRAME_attack1 - 1;
			ent->client->anim_end = FRAME_attack8;
		}
		ent->client->anim_time = 0_ms;
	}
}

// this spits out some smoke from the motor. it's a two-stroke, you know.
void chainfist_smoke(edict_t *ent)
{
	vec3_t tempVec, dir;
	P_ProjectSource(ent, ent->client->v_angle, { 8, 8, -4 }, tempVec, dir);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_CHAINFIST_SMOKE);
	gi.WritePosition(tempVec);
	gi.unicast(ent, 0);
}

void Weapon_ChainFist(edict_t *ent)
{
	constexpr int pause_frames[] = { 0 };

	Weapon_Repeating(ent, 4, 32, 57, 60, pause_frames, weapon_chainfist_fire);
	
	// smoke on idle sequence
	if (ent->client->ps.gunframe == 42 && irandom(8))
	{
		if ((ent->client->pers.hand != CENTER_HANDED) && frandom() < 0.4f)
			chainfist_smoke(ent);
	}
	else if (ent->client->ps.gunframe == 51 && irandom(8))
	{
		if ((ent->client->pers.hand != CENTER_HANDED) && frandom() < 0.4f)
			chainfist_smoke(ent);
	}

	// set the appropriate weapon sound.
	if (ent->client->weaponstate == WEAPON_FIRING)
		ent->client->weapon_sound = gi.soundindex("weapons/sawhit.wav");
	else if (ent->client->weaponstate == WEAPON_DROPPING)
		ent->client->weapon_sound = 0;
	else if (ent->client->pers.weapon->id == IT_WEAPON_CHAINFIST)
		ent->client->weapon_sound = gi.soundindex("weapons/sawidle.wav");
}

//
// Disintegrator
//

void weapon_tracker_fire(edict_t *self)
{
	vec3_t	 end;
	edict_t *enemy;
	trace_t	 tr;
	int		 damage;
	vec3_t	 mins, maxs;

	// PMM - felt a little high at 25
	if (deathmatch->integer)
		damage = 45;
	else
		damage = 135;

	if (is_quad)
		damage *= damage_multiplier; // pgm

	mins = { -16, -16, -16 };
	maxs = { 16, 16, 16 };

	vec3_t start, dir;
	P_ProjectSource(self, self->client->v_angle, { 24, 8, -8 }, start, dir);

	end = start + (dir * 8192);
	enemy = nullptr;
	// PMM - doing two traces .. one point and one box.
	contents_t mask = MASK_PROJECTILE;

	// [Paril-KEX]
	if (!G_ShouldPlayersCollide(true))
		mask &= ~CONTENTS_PLAYER;

	G_LagCompensate(self, start, dir);
	tr = gi.traceline(start, end, self, mask);
	G_UnLagCompensate();
	if (tr.ent != world)
	{
		if ((tr.ent->svflags & SVF_MONSTER) || tr.ent->client || (tr.ent->flags & FL_DAMAGEABLE))
		{
			if (tr.ent->health > 0)
				enemy = tr.ent;
		}
	}
	else
	{
		tr = gi.trace(start, mins, maxs, end, self, mask);
		if (tr.ent != world)
		{
			if ((tr.ent->svflags & SVF_MONSTER) || tr.ent->client || (tr.ent->flags & FL_DAMAGEABLE))
			{
				if (tr.ent->health > 0)
					enemy = tr.ent;
			}
		}
	}

	P_AddWeaponKick(self, self->client->v_forward * -2, { -1.f, 0.f, 0.f });

	fire_tracker(self, start, dir, damage, 1000, enemy);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(self);
	gi.WriteByte(MZ_TRACKER | is_silenced);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);

	PlayerNoise(self, start, PNOISE_WEAPON);

	G_RemoveAmmo(self);
}

void Weapon_Disintegrator(edict_t *ent)
{
	constexpr int pause_frames[] = { 14, 19, 23, 0 };
	constexpr int fire_frames[] = { 5, 0 };

	Weapon_Generic(ent, 4, 9, 29, 34, pause_frames, fire_frames, weapon_tracker_fire);
}

/*
======================================================================

ETF RIFLE

======================================================================
*/
void weapon_etf_rifle_fire(edict_t *ent)
{
	int	   damage;
	int	   kick = 3;
	int	   i;
	vec3_t offset;

	if (deathmatch->integer)
		damage = 10;
	else
		damage = 10;

	if (!(ent->client->buttons & BUTTON_ATTACK))
	{
		ent->client->ps.gunframe = 8;
		return;
	}

	if (ent->client->ps.gunframe == 6)
		ent->client->ps.gunframe = 7;
	else
		ent->client->ps.gunframe = 6;

	// PGM - adjusted to use the quantity entry in the weapon structure.
	if (ent->client->pers.inventory[ent->client->pers.weapon->ammo] < ent->client->pers.weapon->quantity)
	{
		ent->client->ps.gunframe = 8;
		NoAmmoWeaponChange(ent, true);
		return;
	}

	if (is_quad)
	{
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	vec3_t kick_origin {}, kick_angles {};
	for (i = 0; i < 3; i++)
	{
		kick_origin[i] = crandom() * 0.85f;
		kick_angles[i] = crandom() * 0.85f;
	}
	P_AddWeaponKick(ent, kick_origin, kick_angles);

	// get start / end positions
	if (ent->client->ps.gunframe == 6)
		offset = { 15, 8, -8 };
	else
		offset = { 15, 6, -8 };

	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->v_angle + kick_angles, offset, start, dir);
	fire_flechette(ent, start, dir, damage, 1150, kick);
	Weapon_PowerupSound(ent);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte((ent->client->ps.gunframe == 6 ? MZ_ETF_RIFLE : MZ_ETF_RIFLE_2) | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	G_RemoveAmmo(ent);

	ent->client->anim_priority = ANIM_ATTACK;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->s.frame = FRAME_crattak1 - (int) (frandom() + 0.25f);
		ent->client->anim_end = FRAME_crattak9;
	}
	else
	{
		ent->s.frame = FRAME_attack1 - (int) (frandom() + 0.25f);
		ent->client->anim_end = FRAME_attack8;
	}
	ent->client->anim_time = 0_ms;
}

void Weapon_ETF_Rifle(edict_t *ent)
{
	constexpr int pause_frames[] = { 18, 28, 0 };

	Weapon_Repeating(ent, 4, 7, 37, 41, pause_frames, weapon_etf_rifle_fire);
}

constexpr int32_t HEATBEAM_DM_DMG = 15;
constexpr int32_t HEATBEAM_SP_DMG = 15;

void Heatbeam_Fire(edict_t *ent)
{
	bool firing = (ent->client->buttons & BUTTON_ATTACK);
	bool has_ammo = ent->client->pers.inventory[ent->client->pers.weapon->ammo] >= ent->client->pers.weapon->quantity;

	if (!firing || !has_ammo)
	{
		ent->client->ps.gunframe = 13;
		ent->client->weapon_sound = 0;
		ent->client->ps.gunskin = 0;

		if (firing && !has_ammo)
			NoAmmoWeaponChange(ent, true);
		return;
	}

	// start on frame 8
	if (ent->client->ps.gunframe > 12)
		ent->client->ps.gunframe = 8;
	else
		ent->client->ps.gunframe++;

	if (ent->client->ps.gunframe == 12)
		ent->client->ps.gunframe = 8;

	// play weapon sound for firing
	ent->client->weapon_sound = gi.soundindex("weapons/bfg__l1a.wav");
	ent->client->ps.gunskin = 1;

	int damage;
	int kick;

	// for comparison, the hyperblaster is 15/20
	// jim requested more damage, so try 15/15 --- PGM 07/23/98
	if (deathmatch->integer)
		damage = HEATBEAM_DM_DMG;
	else
		damage = HEATBEAM_SP_DMG;

	if (deathmatch->integer) // really knock 'em around in deathmatch
		kick = 75;
	else
		kick = 30;

	if (is_quad)
	{
		damage *= damage_multiplier;
		kick *= damage_multiplier;
	}

	ent->client->kick.time = 0_ms;

	// This offset is the "view" offset for the beam start (used by trace)
	vec3_t start, dir;
	P_ProjectSource(ent, ent->client->v_angle, { 7, 2, -3 }, start, dir);

	// This offset is the entity offset
	G_LagCompensate(ent, start, dir);
	fire_heatbeam(ent, start, dir, { 2, 7, -3 }, damage, kick, false);
	G_UnLagCompensate();
	Weapon_PowerupSound(ent);

	// send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteEntity(ent);
	gi.WriteByte(MZ_HEATBEAM | is_silenced);
	gi.multicast(ent->s.origin, MULTICAST_PVS, false);

	PlayerNoise(ent, start, PNOISE_WEAPON);
	
	G_RemoveAmmo(ent);

	ent->client->anim_priority = ANIM_ATTACK;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->s.frame = FRAME_crattak1 - (int) (frandom() + 0.25f);
		ent->client->anim_end = FRAME_crattak9;
	}
	else
	{
		ent->s.frame = FRAME_attack1 - (int) (frandom() + 0.25f);
		ent->client->anim_end = FRAME_attack8;
	}
	ent->client->anim_time = 0_ms;
}

void Weapon_Heatbeam(edict_t *ent)
{
	constexpr int pause_frames[] = { 35, 0 };

	Weapon_Repeating(ent, 8, 12, 42, 47, pause_frames, Heatbeam_Fire);
}