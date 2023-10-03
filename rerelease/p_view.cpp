// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "g_local.h"
#include "m_player.h"
#include "bots/bot_includes.h"

static edict_t   *current_player;
static gclient_t *current_client;

static vec3_t forward, right, up;
float		  xyspeed;

float bobmove;
int	  bobcycle, bobcycle_run;	  // odd cycles are right foot going forward
float bobfracsin; // sinf(bobfrac*M_PI)

/*
===============
SkipViewModifiers
===============
*/
inline bool SkipViewModifiers() {
	if ( g_skipViewModifiers->integer && sv_cheats->integer ) {
		return true;
	}
	// don't do bobbing, etc on grapple
	if (current_client->ctf_grapple &&
		 current_client->ctf_grapplestate > CTF_GRAPPLE_STATE_FLY) {
		return true;
	}
	// spectator mode
	if (current_client->resp.spectator || (G_TeamplayEnabled() && current_client->resp.ctf_team == CTF_NOTEAM)) {
		return true;
	}
	return false;
}

/*
===============
SV_CalcRoll

===============
*/
float SV_CalcRoll(const vec3_t &angles, const vec3_t &velocity)
{
	if ( SkipViewModifiers() ) {
		return 0.0f;
	}

	float sign;
	float side;
	float value;

	side = velocity.dot(right);
	sign = side < 0 ? -1.f : 1.f;
	side = fabsf(side);

	value = sv_rollangle->value;

	if (side < sv_rollspeed->value)
		side = side * value / sv_rollspeed->value;
	else
		side = value;

	return side * sign;
}

/*
===============
P_DamageFeedback

Handles color blends and view kicks
===============
*/
void P_DamageFeedback(edict_t *player)
{
	gclient_t		  *client;
	float			 side;
	float			 realcount, count, kick;
	vec3_t			 v;
	int				 l;
	constexpr vec3_t armor_color = { 1.0, 1.0, 1.0 };
	constexpr vec3_t power_color = { 0.0, 1.0, 0.0 };
	constexpr vec3_t bcolor = { 1.0, 0.0, 0.0 };

	client = player->client;

	// flash the backgrounds behind the status numbers
	int16_t want_flashes = 0;

	if (client->damage_blood)
		want_flashes |= 1;
	if (client->damage_armor && !(player->flags & FL_GODMODE) && (client->invincible_time <= level.time))
		want_flashes |= 2;

	if (want_flashes)
	{
		client->flash_time = level.time + 100_ms;
		client->ps.stats[STAT_FLASHES] = want_flashes;
	}
	else if (client->flash_time < level.time)
		client->ps.stats[STAT_FLASHES] = 0;

	// total points of damage shot at the player this frame
	count = (float) (client->damage_blood + client->damage_armor + client->damage_parmor);
	if (count == 0)
		return; // didn't take any damage

	// start a pain animation if still in the player model
	if (client->anim_priority < ANIM_PAIN && player->s.modelindex == MODELINDEX_PLAYER)
	{
		static int i;

		client->anim_priority = ANIM_PAIN;
		if (client->ps.pmove.pm_flags & PMF_DUCKED)
		{
			player->s.frame = FRAME_crpain1 - 1;
			client->anim_end = FRAME_crpain4;
		}
		else
		{
			i = (i + 1) % 3;
			switch (i)
			{
			case 0:
				player->s.frame = FRAME_pain101 - 1;
				client->anim_end = FRAME_pain104;
				break;
			case 1:
				player->s.frame = FRAME_pain201 - 1;
				client->anim_end = FRAME_pain204;
				break;
			case 2:
				player->s.frame = FRAME_pain301 - 1;
				client->anim_end = FRAME_pain304;
				break;
			}
		}

		client->anim_time = 0_ms;
	}

	realcount = count;

	// if we took health damage, do a minimum clamp
	if (client->damage_blood)
	{
		if (count < 10)
			count = 10; // always make a visible effect
	}
	else
	{
		if (count > 2)
			count = 2; // don't go too deep
	}

	// play an appropriate pain sound
	if ((level.time > player->pain_debounce_time) && !(player->flags & FL_GODMODE) && (client->invincible_time <= level.time))
	{
		player->pain_debounce_time = level.time + 700_ms;

		constexpr const char *pain_sounds[] = {
			"*pain25_1.wav",
			"*pain25_2.wav",
			"*pain50_1.wav",
			"*pain50_2.wav",
			"*pain75_1.wav",
			"*pain75_2.wav",
			"*pain100_1.wav",
			"*pain100_2.wav",
		};

		if (player->health < 25)
			l = 0;
		else if (player->health < 50)
			l = 2;
		else if (player->health < 75)
			l = 4;
		else
			l = 6;

		if (brandom())
			l |= 1;

		gi.sound(player, CHAN_VOICE, gi.soundindex(pain_sounds[l]), 1, ATTN_NORM, 0);
		// Paril: pain noises alert monsters
		PlayerNoise(player, player->s.origin, PNOISE_SELF);
	}

	// the total alpha of the blend is always proportional to count
	if (client->damage_alpha < 0)
		client->damage_alpha = 0;

	// [Paril-KEX] tweak the values to rely less on this
	// and more on damage indicators
	if (client->damage_blood || (client->damage_alpha + count * 0.06f) < 0.15f)
	{
		client->damage_alpha += count * 0.06f;

		if (client->damage_alpha < 0.06f)
			client->damage_alpha = 0.06f;
		if (client->damage_alpha > 0.4f)
			client->damage_alpha = 0.4f; // don't go too saturated
	}

	// mix in colors
	v = {};

	if (client->damage_parmor)
		v += power_color * (client->damage_parmor / realcount);
	if (client->damage_blood)
		v += bcolor * max(15.0f, (client->damage_blood / realcount));
	if (client->damage_armor)
		v += armor_color * (client->damage_armor / realcount);
	client->damage_blend = v.normalized();

	//
	// calculate view angle kicks
	//
	kick = (float) abs(client->damage_knockback);
	if (kick && player->health > 0) // kick of 0 means no view adjust at all
	{
		kick = kick * 100 / player->health;

		if (kick < count * 0.5f)
			kick = count * 0.5f;
		if (kick > 50)
			kick = 50;

		v = client->damage_from - player->s.origin;
		v.normalize();

		side = v.dot(right);
		client->v_dmg_roll = kick * side * 0.3f;

		side = -v.dot(forward);
		client->v_dmg_pitch = kick * side * 0.3f;

		client->v_dmg_time = level.time + DAMAGE_TIME();
	}

	// [Paril-KEX] send view indicators
	if (client->num_damage_indicators)
	{
		gi.WriteByte(svc_damage);
		gi.WriteByte(client->num_damage_indicators);

		for (uint8_t i = 0; i < client->num_damage_indicators; i++)
		{
			auto &indicator = client->damage_indicators[i];

			// encode total damage into 5 bits
			uint8_t encoded = std::clamp((indicator.health + indicator.power + indicator.armor) / 3, 1, 0x1F);

			// encode types in the latter 3 bits
			if (indicator.health)
				encoded |= 0x20;
			if (indicator.armor)
				encoded |= 0x40;
			if (indicator.power)
				encoded |= 0x80;

			gi.WriteByte(encoded);
			gi.WriteDir((player->s.origin - indicator.from).normalized());
		}

		gi.unicast(player, false);
	}

	//
	// clear totals
	//
	client->damage_blood = 0;
	client->damage_armor = 0;
	client->damage_parmor = 0;
	client->damage_knockback = 0;
	client->num_damage_indicators = 0;
}

/*
===============
SV_CalcViewOffset

Auto pitching on slopes?

  fall from 128: 400 = 160000
  fall from 256: 580 = 336400
  fall from 384: 720 = 518400
  fall from 512: 800 = 640000
  fall from 640: 960 =

  damage = deltavelocity*deltavelocity  * 0.0001

===============
*/
void SV_CalcViewOffset(edict_t *ent)
{
	float  bob;
	float  ratio;
	float  delta;
	vec3_t v;

	//===================================

	// base angles
	vec3_t &angles = ent->client->ps.kick_angles;

	// if dead, fix the angle and don't add any kick
	if (ent->deadflag && !ent->client->resp.spectator)
	{
		angles = {};

		if (ent->flags & FL_SAM_RAIMI)
		{
			ent->client->ps.viewangles[ROLL] = 0;
			ent->client->ps.viewangles[PITCH] = 0;
		}
		else
		{
			ent->client->ps.viewangles[ROLL] = 40;
			ent->client->ps.viewangles[PITCH] = -15;
		}
		ent->client->ps.viewangles[YAW] = ent->client->killer_yaw;
	}
	else if (!ent->client->pers.bob_skip && !SkipViewModifiers())
	{
		// add angles based on weapon kick
		angles = P_CurrentKickAngles(ent);

		// add angles based on damage kick
		if (ent->client->v_dmg_time > level.time)
		{
			// [Paril-KEX] 100ms of slack is added to account for
			// visual difference in higher tickrates
			gtime_t diff = ent->client->v_dmg_time - level.time;

			// slack time remaining
			if (DAMAGE_TIME_SLACK())
			{
				if (diff > DAMAGE_TIME() - DAMAGE_TIME_SLACK())
					ratio = (DAMAGE_TIME() - diff).seconds() / DAMAGE_TIME_SLACK().seconds();
				else
					ratio = diff.seconds() / (DAMAGE_TIME() - DAMAGE_TIME_SLACK()).seconds();
			}
			else
				ratio = diff.seconds() / (DAMAGE_TIME() - DAMAGE_TIME_SLACK()).seconds();

			angles[PITCH] += ratio * ent->client->v_dmg_pitch;
			angles[ROLL] += ratio * ent->client->v_dmg_roll;
		}

		// add pitch based on fall kick
		if (ent->client->fall_time > level.time)
		{
			// [Paril-KEX] 100ms of slack is added to account for
			// visual difference in higher tickrates
			gtime_t diff = ent->client->fall_time - level.time;

			// slack time remaining
			if (DAMAGE_TIME_SLACK())
			{
				if (diff > FALL_TIME() - DAMAGE_TIME_SLACK())
					ratio = (FALL_TIME() - diff).seconds() / DAMAGE_TIME_SLACK().seconds();
				else
					ratio = diff.seconds() / (FALL_TIME() - DAMAGE_TIME_SLACK()).seconds();
			}
			else
				ratio = diff.seconds() / (FALL_TIME() - DAMAGE_TIME_SLACK()).seconds();
			angles[PITCH] += ratio * ent->client->fall_value;
		}

		// add angles based on velocity
		if (!ent->client->pers.bob_skip && !SkipViewModifiers())
		{
			delta = ent->velocity.dot(forward);
			angles[PITCH] += delta * run_pitch->value;

			delta = ent->velocity.dot(right);
			angles[ROLL] += delta * run_roll->value;

			// add angles based on bob
			delta = bobfracsin * bob_pitch->value * xyspeed;
			if ((ent->client->ps.pmove.pm_flags & PMF_DUCKED) && ent->groundentity)
				delta *= 6; // crouching
			delta = min(delta, 1.2f);
			angles[PITCH] += delta;
			delta = bobfracsin * bob_roll->value * xyspeed;
			if ((ent->client->ps.pmove.pm_flags & PMF_DUCKED) && ent->groundentity)
				delta *= 6; // crouching
			delta = min(delta, 1.2f);
			if (bobcycle & 1)
				delta = -delta;
			angles[ROLL] += delta;
		}

		// add earthquake angles
		if (ent->client->quake_time > level.time)
		{
			float factor = min(1.0f, (ent->client->quake_time.seconds() / level.time.seconds()) * 0.25f);

			angles.x += crandom_open() * factor;
			angles.z += crandom_open() * factor;
			angles.y += crandom_open() * factor;
		}
	}

	// [Paril-KEX] clamp angles
	for (int i = 0; i < 3; i++)
		angles[i] = clamp(angles[i], -31.f, 31.f);

	//===================================

	// base origin

	v = {};

	// add fall height

	if (!ent->client->pers.bob_skip && !SkipViewModifiers())
	{
		if (ent->client->fall_time > level.time)
		{
			// [Paril-KEX] 100ms of slack is added to account for
			// visual difference in higher tickrates
			gtime_t diff = ent->client->fall_time - level.time;

			// slack time remaining
			if (DAMAGE_TIME_SLACK())
			{
				if (diff > FALL_TIME() - DAMAGE_TIME_SLACK())
					ratio = (FALL_TIME() - diff).seconds() / DAMAGE_TIME_SLACK().seconds();
				else
					ratio = diff.seconds() / (FALL_TIME() - DAMAGE_TIME_SLACK()).seconds();
			}
			else
				ratio = diff.seconds() / (FALL_TIME() - DAMAGE_TIME_SLACK()).seconds();
			v[2] -= ratio * ent->client->fall_value * 0.4f;
		}

	// add bob height
		bob = bobfracsin * xyspeed * bob_up->value;
		if (bob > 6)
			bob = 6;
		// gi.DebugGraph (bob *2, 255);
		v[2] += bob;
	}

	// add kick offset

	if (!ent->client->pers.bob_skip && !SkipViewModifiers())
		v += P_CurrentKickOrigin(ent);

	// absolutely bound offsets
	// so the view can never be outside the player box

	if (v[0] < -14)
		v[0] = -14;
	else if (v[0] > 14)
		v[0] = 14;
	if (v[1] < -14)
		v[1] = -14;
	else if (v[1] > 14)
		v[1] = 14;
	if (v[2] < -22)
		v[2] = -22;
	else if (v[2] > 30)
		v[2] = 30;

	ent->client->ps.viewoffset = v;
}

/*
==============
SV_CalcGunOffset
==============
*/
void SV_CalcGunOffset(edict_t *ent)
{
	int	  i;
	// ROGUE

	// ROGUE - heatbeam shouldn't bob so the beam looks right
	if (ent->client->pers.weapon && 
		!((ent->client->pers.weapon->id == IT_WEAPON_PLASMABEAM || ent->client->pers.weapon->id == IT_WEAPON_GRAPPLE) && ent->client->weaponstate == WEAPON_FIRING)
		&& !SkipViewModifiers())
	{
		// ROGUE
		// gun angles from bobbing
		ent->client->ps.gunangles[ROLL] = xyspeed * bobfracsin * 0.005f;
		ent->client->ps.gunangles[YAW] = xyspeed * bobfracsin * 0.01f;
		if (bobcycle & 1)
		{
			ent->client->ps.gunangles[ROLL] = -ent->client->ps.gunangles[ROLL];
			ent->client->ps.gunangles[YAW] = -ent->client->ps.gunangles[YAW];
		}

		ent->client->ps.gunangles[PITCH] = xyspeed * bobfracsin * 0.005f;

		vec3_t viewangles_delta = ent->client->oldviewangles - ent->client->ps.viewangles;
		
		for (i = 0; i < 3; i++)
			ent->client->slow_view_angles[i] += viewangles_delta[i];

		// gun angles from delta movement
		for (i = 0; i < 3; i++)
		{
			float &d = ent->client->slow_view_angles[i];

			if (!d)
				continue;

			if (d > 180)
				d -= 360;
			if (d < -180)
				d += 360;
			if (d > 45)
				d = 45;
			if (d < -45)
				d = -45;

			// [Sam-KEX] Apply only half-delta. Makes the weapons look less detatched from the player.
			if (i == ROLL)
				ent->client->ps.gunangles[i] += (0.1f * d) * 0.5f;
			else
				ent->client->ps.gunangles[i] += (0.2f * d) * 0.5f;

			float reduction_factor = viewangles_delta[i] ? 0.05f : 0.15f;

			if (d > 0)
				d = max(0.f, d - gi.frame_time_ms * reduction_factor);
			else if (d < 0)
				d = min(0.f, d + gi.frame_time_ms * reduction_factor);
		}

		// [Paril-KEX] cl_rollhack
		ent->client->ps.gunangles[ROLL] = -ent->client->ps.gunangles[ROLL];
	}
	// ROGUE
	else
	{
		for (i = 0; i < 3; i++)
			ent->client->ps.gunangles[i] = 0;
	}
	// ROGUE

	// gun height
	ent->client->ps.gunoffset = {};

	// gun_x / gun_y / gun_z are development tools
	for (i = 0; i < 3; i++)
	{
		ent->client->ps.gunoffset[i] += forward[i] * (gun_y->value);
		ent->client->ps.gunoffset[i] += right[i] * gun_x->value;
		ent->client->ps.gunoffset[i] += up[i] * (-gun_z->value);
	}
}

/*
=============
SV_CalcBlend
=============
*/
void SV_CalcBlend(edict_t *ent)
{
	gtime_t remaining;

	ent->client->ps.damage_blend = ent->client->ps.screen_blend = {};

	// add for powerups
	if (ent->client->quad_time > level.time)
	{
		remaining = ent->client->quad_time - level.time;
		if (remaining.milliseconds() == 3000) // beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage2.wav"), 1, ATTN_NORM, 0);
		if (G_PowerUpExpiringRelative(remaining))
			G_AddBlend(0, 0, 1, 0.08f, ent->client->ps.screen_blend);
	}
	// RAFAEL
	else if (ent->client->quadfire_time > level.time)
	{
		remaining = ent->client->quadfire_time - level.time;
		if (remaining.milliseconds() == 3000) // beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/quadfire2.wav"), 1, ATTN_NORM, 0);
		if (G_PowerUpExpiringRelative(remaining))
			G_AddBlend(1, 0.2f, 0.5f, 0.08f, ent->client->ps.screen_blend);
	}
	// RAFAEL
	// PMM - double damage
	else if (ent->client->double_time > level.time)
	{
		remaining = ent->client->double_time - level.time;
		if (remaining.milliseconds() == 3000) // beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("misc/ddamage2.wav"), 1, ATTN_NORM, 0);
		if (G_PowerUpExpiringRelative(remaining))
			G_AddBlend(0.9f, 0.7f, 0, 0.08f, ent->client->ps.screen_blend);
	}
	// PMM
	else if (ent->client->invincible_time > level.time)
	{
		remaining = ent->client->invincible_time - level.time;
		if (remaining.milliseconds() == 3000) // beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect2.wav"), 1, ATTN_NORM, 0);
		if (G_PowerUpExpiringRelative(remaining))
			G_AddBlend(1, 1, 0, 0.08f, ent->client->ps.screen_blend);
	}
	else if (ent->client->invisible_time > level.time)
	{
		remaining = ent->client->invisible_time - level.time;
		if (remaining.milliseconds() == 3000) // beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect2.wav"), 1, ATTN_NORM, 0);
		if (G_PowerUpExpiringRelative(remaining))
			G_AddBlend(0.8f, 0.8f, 0.8f, 0.08f, ent->client->ps.screen_blend);
	}
	else if (ent->client->enviro_time > level.time)
	{
		remaining = ent->client->enviro_time - level.time;
		if (remaining.milliseconds() == 3000) // beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/airout.wav"), 1, ATTN_NORM, 0);
		if (G_PowerUpExpiringRelative(remaining))
			G_AddBlend(0, 1, 0, 0.08f, ent->client->ps.screen_blend);
	}
	else if (ent->client->breather_time > level.time)
	{
		remaining = ent->client->breather_time - level.time;
		if (remaining.milliseconds() == 3000) // beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/airout.wav"), 1, ATTN_NORM, 0);
		if (G_PowerUpExpiringRelative(remaining))
			G_AddBlend(0.4f, 1, 0.4f, 0.04f, ent->client->ps.screen_blend);
	}

	// PGM
	if (ent->client->nuke_time > level.time)
	{
		float brightness = (ent->client->nuke_time - level.time).seconds() / 2.0f;
		G_AddBlend(1, 1, 1, brightness, ent->client->ps.screen_blend);
	}
	if (ent->client->ir_time > level.time)
	{
		remaining = ent->client->ir_time - level.time;
		if (G_PowerUpExpiringRelative(remaining))
		{
			ent->client->ps.rdflags |= RDF_IRGOGGLES;
			G_AddBlend(1, 0, 0, 0.2f, ent->client->ps.screen_blend);
		}
		else
			ent->client->ps.rdflags &= ~RDF_IRGOGGLES;
	}
	else
	{
		ent->client->ps.rdflags &= ~RDF_IRGOGGLES;
	}
	// PGM

	// add for damage
	if (ent->client->damage_alpha > 0)
		G_AddBlend(ent->client->damage_blend[0], ent->client->damage_blend[1], ent->client->damage_blend[2], ent->client->damage_alpha, ent->client->ps.damage_blend);

	// [Paril-KEX] drowning visual indicator
	if (ent->air_finished < level.time + 9_sec)
	{
		constexpr vec3_t drown_color = { 0.1f, 0.1f, 0.2f };
		constexpr float max_drown_alpha = 0.75f;
		float alpha = (ent->air_finished < level.time) ? 1 : (1.f - ((ent->air_finished - level.time).seconds() / 9.0f));
		G_AddBlend(drown_color[0], drown_color[1], drown_color[2], min(alpha, max_drown_alpha), ent->client->ps.damage_blend);
	}

#if 0
	if (ent->client->bonus_alpha > 0)
		G_AddBlend(0.85f, 0.7f, 0.3f, ent->client->bonus_alpha, ent->client->ps.damage_blend);
#endif

	// drop the damage value
	ent->client->damage_alpha -= gi.frame_time_s * 0.6f;
	if (ent->client->damage_alpha < 0)
		ent->client->damage_alpha = 0;

	// drop the bonus value
	ent->client->bonus_alpha -= gi.frame_time_s;
	if (ent->client->bonus_alpha < 0)
		ent->client->bonus_alpha = 0;
}

/*
=============
P_WorldEffects
=============
*/
void P_WorldEffects()
{
	bool		  breather;
	bool		  envirosuit;
	water_level_t waterlevel, old_waterlevel;

	if (current_player->movetype == MOVETYPE_NOCLIP)
	{
		current_player->air_finished = level.time + 12_sec; // don't need air
		return;
	}

	waterlevel = current_player->waterlevel;
	old_waterlevel = current_client->old_waterlevel;
	current_client->old_waterlevel = waterlevel;

	breather = current_client->breather_time > level.time;
	envirosuit = current_client->enviro_time > level.time;

	//
	// if just entered a water volume, play a sound
	//
	if (!old_waterlevel && waterlevel)
	{
		PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);
		if (current_player->watertype & CONTENTS_LAVA)
			gi.sound(current_player, CHAN_BODY, gi.soundindex("player/lava_in.wav"), 1, ATTN_NORM, 0);
		else if (current_player->watertype & CONTENTS_SLIME)
			gi.sound(current_player, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
		else if (current_player->watertype & CONTENTS_WATER)
			gi.sound(current_player, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
		current_player->flags |= FL_INWATER;

		// clear damage_debounce, so the pain sound will play immediately
		current_player->damage_debounce_time = level.time - 1_sec;
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if (old_waterlevel && !waterlevel)
	{
		PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);
		gi.sound(current_player, CHAN_BODY, gi.soundindex("player/watr_out.wav"), 1, ATTN_NORM, 0);
		current_player->flags &= ~FL_INWATER;
	}

	//
	// check for head just going under water
	//
	if (old_waterlevel != WATER_UNDER && waterlevel == WATER_UNDER)
	{
		gi.sound(current_player, CHAN_BODY, gi.soundindex("player/watr_un.wav"), 1, ATTN_NORM, 0);
	}

	//
	// check for head just coming out of water
	//
	if (current_player->health > 0 && old_waterlevel == WATER_UNDER && waterlevel != WATER_UNDER)
	{
		if (current_player->air_finished < level.time)
		{ // gasp for air
			gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/gasp1.wav"), 1, ATTN_NORM, 0);
			PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);
		}
		else if (current_player->air_finished < level.time + 11_sec)
		{ // just break surface
			gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/gasp2.wav"), 1, ATTN_NORM, 0);
		}
	}

	//
	// check for drowning
	//
	if (waterlevel == WATER_UNDER)
	{
		// breather or envirosuit give air
		if (breather || envirosuit)
		{
			current_player->air_finished = level.time + 10_sec;

			if (((current_client->breather_time - level.time).milliseconds() % 2500) == 0)
			{
				if (!current_client->breather_sound)
					gi.sound(current_player, CHAN_AUTO, gi.soundindex("player/u_breath1.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound(current_player, CHAN_AUTO, gi.soundindex("player/u_breath2.wav"), 1, ATTN_NORM, 0);
				current_client->breather_sound ^= 1;
				PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);
				// FIXME: release a bubble?
			}
		}

		// if out of air, start drowning
		if (current_player->air_finished < level.time)
		{ // drown!
			if (current_player->client->next_drown_time < level.time && current_player->health > 0)
			{
				current_player->client->next_drown_time = level.time + 1_sec;

				// take more damage the longer underwater
				current_player->dmg += 2;
				if (current_player->dmg > 15)
					current_player->dmg = 15;

				// play a gurp sound instead of a normal pain sound
				if (current_player->health <= current_player->dmg)
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("*drown1.wav"), 1, ATTN_NORM, 0); // [Paril-KEX]
				else if (brandom())
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("*gurp1.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("*gurp2.wav"), 1, ATTN_NORM, 0);

				current_player->pain_debounce_time = level.time;

				T_Damage(current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, current_player->dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
			}
		}
		// Paril: almost-drowning sounds
		else if (current_player->air_finished <= level.time + 3_sec)
		{
			if (current_player->client->next_drown_time < level.time)
			{
				gi.sound(current_player, CHAN_VOICE, gi.soundindex(fmt::format("player/wade{}.wav", 1 + ((int32_t) level.time.seconds() % 3)).c_str()), 1, ATTN_NORM, 0);
				current_player->client->next_drown_time = level.time + 1_sec;
			}
		}
	}
	else
	{
		current_player->air_finished = level.time + 12_sec;
		current_player->dmg = 2;
	}

	//
	// check for sizzle damage
	//
	if (waterlevel && (current_player->watertype & (CONTENTS_LAVA | CONTENTS_SLIME)) && current_player->slime_debounce_time <= level.time)
	{
		if (current_player->watertype & CONTENTS_LAVA)
		{
			if (current_player->health > 0 && current_player->pain_debounce_time <= level.time && current_client->invincible_time < level.time)
			{
				if (brandom())
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/burn1.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/burn2.wav"), 1, ATTN_NORM, 0);
				current_player->pain_debounce_time = level.time + 1_sec;
			}

			int dmg = (envirosuit ? 1 : 3) * waterlevel; // take 1/3 damage with envirosuit

			T_Damage(current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, dmg, 0, DAMAGE_NONE, MOD_LAVA);
			current_player->slime_debounce_time = level.time + 10_hz;
		}

		if (current_player->watertype & CONTENTS_SLIME)
		{
			if (!envirosuit)
			{ // no damage from slime with envirosuit
				T_Damage(current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, 1 * waterlevel, 0, DAMAGE_NONE, MOD_SLIME);
				current_player->slime_debounce_time = level.time + 10_hz;
			}
		}
	}
}

/*
===============
G_SetClientEffects
===============
*/
void G_SetClientEffects(edict_t *ent)
{
	int pa_type;

	ent->s.effects = EF_NONE;
	ent->s.renderfx &= RF_STAIR_STEP;
	ent->s.renderfx |= RF_IR_VISIBLE;
	ent->s.alpha = 1.0;

	if (ent->health <= 0 || level.intermissiontime)
		return;

	if (ent->flags & FL_FLASHLIGHT)
		ent->s.effects |= EF_FLASHLIGHT;

	//=========
	// PGM
	if (ent->flags & FL_DISGUISED)
		ent->s.renderfx |= RF_USE_DISGUISE;

	if (gamerules->integer)
	{
		if (DMGame.PlayerEffects)
			DMGame.PlayerEffects(ent);
	}
	// PGM
	//=========

	if (ent->powerarmor_time > level.time)
	{
		pa_type = PowerArmorType(ent);
		if (pa_type == IT_ITEM_POWER_SCREEN)
		{
			ent->s.effects |= EF_POWERSCREEN;
		}
		else if (pa_type == IT_ITEM_POWER_SHIELD)
		{
			ent->s.effects |= EF_COLOR_SHELL;
			ent->s.renderfx |= RF_SHELL_GREEN;
		}
	}

	// ZOID
	CTFEffects(ent);
	// ZOID

	if (ent->client->quad_time > level.time)
	{
		if (G_PowerUpExpiring(ent->client->quad_time))
			CTFSetPowerUpEffect(ent, EF_QUAD);
	}

	// RAFAEL
	if (ent->client->quadfire_time > level.time)
	{;
		if (G_PowerUpExpiring(ent->client->quadfire_time))
			CTFSetPowerUpEffect(ent, EF_DUALFIRE);
	}
	// RAFAEL
	//=======
	// ROGUE
	if (ent->client->double_time > level.time)
	{
		if (G_PowerUpExpiring(ent->client->double_time))
			CTFSetPowerUpEffect(ent, EF_DOUBLE);
	}
	if ((ent->client->owned_sphere) && (ent->client->owned_sphere->spawnflags == SPHERE_DEFENDER))
	{
		CTFSetPowerUpEffect(ent, EF_HALF_DAMAGE);
	}
	if (ent->client->tracker_pain_time > level.time)
	{
		ent->s.effects |= EF_TRACKERTRAIL;
	}
	if (ent->client->invisible_time > level.time)
	{
		if (ent->client->invisibility_fade_time <= level.time)
			ent->s.alpha = 0.1f;
		else
		{
			float x = (ent->client->invisibility_fade_time - level.time).seconds() / INVISIBILITY_TIME.seconds();
			ent->s.alpha = std::clamp(x, 0.1f, 1.0f);
		}
	}
	// ROGUE
	//=======

	if (ent->client->invincible_time > level.time)
	{
		if (G_PowerUpExpiring(ent->client->invincible_time))
			CTFSetPowerUpEffect(ent, EF_PENT);
	}

	// show cheaters!!!
	if (ent->flags & FL_GODMODE)
	{
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);
	}

#if 0
	// disintegrator stuff
	if (ent->disintegrator_time)
	{
		if (ent->disintegrator_time > 100_sec)
		{
			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_BOSSTPORT);
			gi.WritePosition(ent->s.origin);
			gi.multicast(ent->s.origin, MULTICAST_PHS, false);

			Killed(ent, ent, ent, 999999, vec3_origin, MOD_NUKE);
			ent->disintegrator_time = 0_ms;
			ThrowClientHead(ent, 9999);
			ent->s.modelindex = 0;
			ent->solid = SOLID_NOT;
		}
		else
		{
			ent->disintegrator_time = max(0_ms, ent->disintegrator_time - 1500_ms);

			if (ent->disintegrator_time)
				ent->s.alpha = max(1 / 255.f, 1.f - (ent->disintegrator_time.seconds() / 100));
			else
				ent->s.alpha = 1;
		}
	}
#endif
}

/*
===============
G_SetClientEvent
===============
*/
void G_SetClientEvent(edict_t *ent)
{
	if (ent->s.event)
		return;

	if (ent->client->ps.pmove.pm_flags & PMF_ON_LADDER)
	{
		if (!deathmatch->integer &&
			current_client->last_ladder_sound < level.time &&
			(current_client->last_ladder_pos - ent->s.origin).length() > 48.f)
		{
			ent->s.event = EV_LADDER_STEP;
			current_client->last_ladder_pos = ent->s.origin;
			current_client->last_ladder_sound = level.time + LADDER_SOUND_TIME;
		}
	}
	else if (ent->groundentity && xyspeed > 225)
	{
		if ((int) (current_client->bobtime + bobmove) != bobcycle_run)
			ent->s.event = EV_FOOTSTEP;
	}
}

/*
===============
G_SetClientSound
===============
*/
void G_SetClientSound(edict_t *ent)
{
	// help beep (no more than three times)
	if (ent->client->pers.helpchanged && ent->client->pers.helpchanged <= 3 && ent->client->pers.help_time < level.time)
	{
		if (ent->client->pers.helpchanged == 1) // [KEX] haleyjd: once only
			gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/pc_up.wav"), 1, ATTN_STATIC, 0);
		ent->client->pers.helpchanged++;
		ent->client->pers.help_time = level.time + 5_sec;
	}

	// reset defaults
	ent->s.sound = 0;
	ent->s.loop_attenuation = 0;
	ent->s.loop_volume = 0;

	if (ent->waterlevel && (ent->watertype & (CONTENTS_LAVA | CONTENTS_SLIME)))
	{
		ent->s.sound = snd_fry;
		return;
	}

	if (ent->deadflag || ent->client->resp.spectator)
		return;

	if (ent->client->weapon_sound)
		ent->s.sound = ent->client->weapon_sound;
	else if (ent->client->pers.weapon)
	{
		if (ent->client->pers.weapon->id == IT_WEAPON_RAILGUN)
			ent->s.sound = gi.soundindex("weapons/rg_hum.wav");
		else if (ent->client->pers.weapon->id == IT_WEAPON_BFG)
			ent->s.sound = gi.soundindex("weapons/bfg_hum.wav");
		// RAFAEL
		else if (ent->client->pers.weapon->id == IT_WEAPON_PHALANX)
			ent->s.sound = gi.soundindex("weapons/phaloop.wav");
		// RAFAEL
	}

	// [Paril-KEX] if no other sound is playing, play appropriate grapple sounds
	if (!ent->s.sound && ent->client->ctf_grapple)
	{
		if (ent->client->ctf_grapplestate == CTF_GRAPPLE_STATE_PULL)
			ent->s.sound = gi.soundindex("weapons/grapple/grpull.wav");
		else if (ent->client->ctf_grapplestate == CTF_GRAPPLE_STATE_FLY)
			ent->s.sound = gi.soundindex("weapons/grapple/grfly.wav");
		else if (ent->client->ctf_grapplestate == CTF_GRAPPLE_STATE_HANG)
			ent->s.sound = gi.soundindex("weapons/grapple/grhang.wav");
	}

	// weapon sounds play at a higher attn
	ent->s.loop_attenuation = ATTN_NORM;
}

/*
===============
G_SetClientFrame
===============
*/
void G_SetClientFrame(edict_t *ent)
{
	gclient_t *client;
	bool	   duck, run;

	if (ent->s.modelindex != MODELINDEX_PLAYER)
		return; // not in the player model

	client = ent->client;

	if (client->ps.pmove.pm_flags & PMF_DUCKED)
		duck = true;
	else
		duck = false;
	if (xyspeed)
		run = true;
	else
		run = false;

	// check for stand/duck and stop/go transitions
	if (duck != client->anim_duck && client->anim_priority < ANIM_DEATH)
		goto newanim;
	if (run != client->anim_run && client->anim_priority == ANIM_BASIC)
		goto newanim;
	if (!ent->groundentity && client->anim_priority <= ANIM_WAVE)
		goto newanim;
	
	if (client->anim_time > level.time)
		return;
	else if ((client->anim_priority & ANIM_REVERSED) && (ent->s.frame > client->anim_end))
	{
		if (client->anim_time <= level.time)
		{
			ent->s.frame--;
			client->anim_time = level.time + 10_hz;
		}
		return;
	}
	else if (!(client->anim_priority & ANIM_REVERSED) && (ent->s.frame < client->anim_end))
	{
		// continue an animation
		if (client->anim_time <= level.time)
		{
			ent->s.frame++;
			client->anim_time = level.time + 10_hz;
		}
		return;
	}

	if (client->anim_priority == ANIM_DEATH)
		return; // stay there
	if (client->anim_priority == ANIM_JUMP)
	{
		if (!ent->groundentity)
			return; // stay there
		ent->client->anim_priority = ANIM_WAVE;

		if (duck)
		{
			ent->s.frame = FRAME_jump6;
			ent->client->anim_end = FRAME_jump4;
			ent->client->anim_priority |= ANIM_REVERSED;
		}
		else
		{
			ent->s.frame = FRAME_jump3;
			ent->client->anim_end = FRAME_jump6;
		}
		ent->client->anim_time = level.time + 10_hz;
		return;
	}

newanim:
	// return to either a running or standing frame
	client->anim_priority = ANIM_BASIC;
	client->anim_duck = duck;
	client->anim_run = run;
	client->anim_time = level.time + 10_hz;

	if (!ent->groundentity)
	{
		// ZOID: if on grapple, don't go into jump frame, go into standing
		// frame
		if (client->ctf_grapple)
		{
			if (duck)
			{
				ent->s.frame = FRAME_crstnd01;
				client->anim_end = FRAME_crstnd19;
			}
			else
			{
				ent->s.frame = FRAME_stand01;
				client->anim_end = FRAME_stand40;
			}
		}
		else
		{
			// ZOID
			client->anim_priority = ANIM_JUMP;

			if (duck)
			{
				if (ent->s.frame != FRAME_crwalk2)
					ent->s.frame = FRAME_crwalk1;
				client->anim_end = FRAME_crwalk2;
			}
			else
			{
				if (ent->s.frame != FRAME_jump2)
					ent->s.frame = FRAME_jump1;
				client->anim_end = FRAME_jump2;
			}
		}
	}
	else if (run)
	{ // running
		if (duck)
		{
			ent->s.frame = FRAME_crwalk1;
			client->anim_end = FRAME_crwalk6;
		}
		else
		{
			ent->s.frame = FRAME_run1;
			client->anim_end = FRAME_run6;
		}
	}
	else
	{ // standing
		if (duck)
		{
			ent->s.frame = FRAME_crstnd01;
			client->anim_end = FRAME_crstnd19;
		}
		else
		{
			ent->s.frame = FRAME_stand01;
			client->anim_end = FRAME_stand40;
		}
	}
}

// [Paril-KEX]
static void P_RunMegaHealth(edict_t *ent)
{
	if (!ent->client->pers.megahealth_time)
		return;
	else if (ent->health <= ent->max_health)
	{
		ent->client->pers.megahealth_time = 0_ms;
		return;
	}

	ent->client->pers.megahealth_time -= FRAME_TIME_S;

	if (ent->client->pers.megahealth_time <= 0_ms)
	{
		ent->health--;

		if (ent->health > ent->max_health)
			ent->client->pers.megahealth_time = 1000_ms;
		else
			ent->client->pers.megahealth_time = 0_ms;
	}
}

// [Paril-KEX] push all players' origins back to match their lag compensation
void G_LagCompensate(edict_t *from_player, const vec3_t &start, const vec3_t &dir)
{
	uint32_t current_frame = gi.ServerFrame();

	// if you need this to fight monsters, you need help
	if (!deathmatch->integer)
		return;
	else if (!g_lag_compensation->integer)
		return;
	// don't need this
	else if (from_player->client->cmd.server_frame >= current_frame ||
		(from_player->svflags & SVF_BOT))
		return;

	int32_t frame_delta = (current_frame - from_player->client->cmd.server_frame) + 1;

	for (auto player : active_players())
	{
		// we aren't gonna hit ourselves
		if (player == from_player)
			continue;

		// not enough data, spare them
		if (player->client->num_lag_origins < frame_delta)
			continue;

		// if they're way outside of cone of vision, they won't be captured in this
		if ((player->s.origin - start).normalized().dot(dir) < 0.75f)
			continue;

		int32_t lag_id = (player->client->next_lag_origin - 1) - (frame_delta - 1);

		if (lag_id < 0)
			lag_id = game.max_lag_origins + lag_id;

		if (lag_id < 0 || lag_id >= player->client->num_lag_origins)
		{
			gi.Com_Print("lag compensation error\n");
			G_UnLagCompensate();
			return;
		}

		const vec3_t &lag_origin = (game.lag_origins + ((player->s.number - 1) * game.max_lag_origins))[lag_id];

		// no way they'd be hit if they aren't in the PVS
		if (!gi.inPVS(lag_origin, start, false))
			continue;

		// only back up once
		if (!player->client->is_lag_compensated)
		{
			player->client->is_lag_compensated = true;
			player->client->lag_restore_origin = player->s.origin;
		}
			
		player->s.origin = lag_origin;

		gi.linkentity(player);
	}
}

// [Paril-KEX] pop everybody's lag compensation values
void G_UnLagCompensate()
{
	for (auto player : active_players())
	{
		if (player->client->is_lag_compensated)
		{
			player->client->is_lag_compensated = false;
			player->s.origin = player->client->lag_restore_origin;
			gi.linkentity(player);
		}
	}
}

// [Paril-KEX] save the current lag compensation value
static void G_SaveLagCompensation(edict_t *ent)
{
	(game.lag_origins + ((ent->s.number - 1) * game.max_lag_origins))[ent->client->next_lag_origin] = ent->s.origin;
	ent->client->next_lag_origin = (ent->client->next_lag_origin + 1) % game.max_lag_origins;

	if (ent->client->num_lag_origins < game.max_lag_origins)
		ent->client->num_lag_origins++;
}

/*
=================
ClientEndServerFrame

Called for each player at the end of the server frame
and right after spawning
=================
*/
void ClientEndServerFrame(edict_t *ent)
{
	// no player exists yet (load game)
	if (!ent->client->pers.spawned)
		return;

	float bobtime, bobtime_run;

	current_player = ent;
	current_client = ent->client;

	// check fog changes
	P_ForceFogTransition(ent, false);

	// check goals
	G_PlayerNotifyGoal(ent);

	// mega health
	P_RunMegaHealth(ent);

	//
	// If the origin or velocity have changed since ClientThink(),
	// update the pmove values.  This will happen when the client
	// is pushed by a bmodel or kicked by an explosion.
	//
	// If it wasn't updated here, the view position would lag a frame
	// behind the body position when pushed -- "sinking into plats"
	//
	current_client->ps.pmove.origin = ent->s.origin;
	current_client->ps.pmove.velocity = ent->velocity;

	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//
	if (level.intermissiontime || ent->client->awaiting_respawn)
	{
		if (ent->client->awaiting_respawn || (level.intermission_eou || level.is_n64 || (deathmatch->integer && level.intermissiontime)))
		{
			current_client->ps.screen_blend[3] = current_client->ps.damage_blend[3] = 0;
			current_client->ps.fov = 90;
			current_client->ps.gunindex = 0;
		}
		G_SetStats(ent);
		G_SetCoopStats(ent);

		// if the scoreboard is up, update it if a client leaves
		if (deathmatch->integer && ent->client->showscores && ent->client->menutime)
		{
			DeathmatchScoreboardMessage(ent, ent->enemy);
			gi.unicast(ent, false);
			ent->client->menutime = 0_ms;
		}

		return;
	}

	// ZOID
	// regen tech
	CTFApplyRegeneration(ent);
	// ZOID

	AngleVectors(ent->client->v_angle, forward, right, up);

	// burn from lava, etc
	P_WorldEffects();

	//
	// set model angles from view angles so other things in
	// the world can tell which direction you are looking
	//
	if (ent->client->v_angle[PITCH] > 180)
		ent->s.angles[PITCH] = (-360 + ent->client->v_angle[PITCH]) / 3;
	else
		ent->s.angles[PITCH] = ent->client->v_angle[PITCH] / 3;
	
	ent->s.angles[YAW] = ent->client->v_angle[YAW];
	ent->s.angles[ROLL] = 0;
	// [Paril-KEX] cl_rollhack
	ent->s.angles[ROLL] = -SV_CalcRoll(ent->s.angles, ent->velocity) * 4;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	xyspeed = sqrt(ent->velocity[0] * ent->velocity[0] + ent->velocity[1] * ent->velocity[1]);

	if (xyspeed < 5)
	{
		bobmove = 0;
		current_client->bobtime = 0; // start at beginning of cycle again
	}
	else if (ent->groundentity)
	{ // so bobbing only cycles when on ground
		if (xyspeed > 210)
			bobmove = gi.frame_time_ms / 400.f;
		else if (xyspeed > 100)
			bobmove = gi.frame_time_ms / 800.f;
		else
			bobmove = gi.frame_time_ms / 1600.f;
	}

	bobtime = (current_client->bobtime += bobmove);
	bobtime_run = bobtime;

	if ((current_client->ps.pmove.pm_flags & PMF_DUCKED) && ent->groundentity)
		bobtime *= 4;

	bobcycle = (int) bobtime;
	bobcycle_run = (int) bobtime_run;
	bobfracsin = fabsf(sinf(bobtime * PIf));

	// apply all the damage taken this frame
	P_DamageFeedback(ent);

	// determine the view offsets
	SV_CalcViewOffset(ent);

	// determine the gun offsets
	SV_CalcGunOffset(ent);

	// determine the full screen color blend
	// must be after viewoffset, so eye contents can be
	// accurately determined
	SV_CalcBlend(ent);

	// chase cam stuff
	if (ent->client->resp.spectator)
		G_SetSpectatorStats(ent);
	else
		G_SetStats(ent);

	G_CheckChaseStats(ent);

	G_SetCoopStats(ent);

	G_SetClientEvent(ent);

	G_SetClientEffects(ent);

	G_SetClientSound(ent);

	G_SetClientFrame(ent);

	ent->client->oldvelocity = ent->velocity;
	ent->client->oldviewangles = ent->client->ps.viewangles;
	ent->client->oldgroundentity = ent->groundentity;

	// ZOID
	if (ent->client->menudirty && ent->client->menutime <= level.time)
	{
		if (ent->client->menu)
		{
			PMenu_Do_Update(ent);
			gi.unicast(ent, true);
		}
		ent->client->menutime = level.time;
		ent->client->menudirty = false;
	}
	// ZOID

	// if the scoreboard is up, update it
	if (ent->client->showscores && ent->client->menutime <= level.time)
	{
		// ZOID
		if (ent->client->menu)
		{
			PMenu_Do_Update(ent);
			ent->client->menudirty = false;
		}
		else
			// ZOID
			DeathmatchScoreboardMessage(ent, ent->enemy);
		gi.unicast(ent, false);
		ent->client->menutime = level.time + 3_sec;
	}

	if ( ( ent->svflags & SVF_BOT ) != 0 ) {
		Bot_EndFrame( ent );
	}

	P_AssignClientSkinnum(ent);

	if (deathmatch->integer)
		G_SaveLagCompensation(ent);

	Compass_Update(ent, false);

	// [Paril-KEX] in coop, if player collision is enabled and
	// we are currently in no-player-collision mode, check if
	// it's safe.
	if (coop->integer && G_ShouldPlayersCollide(false) && !(ent->clipmask & CONTENTS_PLAYER) && ent->takedamage)
	{
		bool clipped_player = false;

		for (auto player : active_players())
		{
			if (player == ent)
				continue;

			trace_t clip = gi.clip(player, ent->s.origin, ent->mins, ent->maxs, ent->s.origin, CONTENTS_MONSTER | CONTENTS_PLAYER);

			if (clip.startsolid || clip.allsolid)
			{
				clipped_player = true;
				break;
			}
		}

		// safe!
		if (!clipped_player)
			ent->clipmask |= CONTENTS_PLAYER;
	}
}