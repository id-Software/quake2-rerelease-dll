//-----------------------------------------------------------------------------
// p_view.c
//
// $Id: p_view.c,v 1.20 2002/02/18 18:25:51 ra Exp $
//
//-----------------------------------------------------------------------------
// $Log: p_view.c,v $
// Revision 1.20  2002/02/18 18:25:51  ra
// Bumped version to 2.6, fixed ctf falling and kicking of players in ctf
// uvtime
//
// Revision 1.19  2001/12/24 18:06:05  slicerdw
// changed dynamic check for darkmatch only
//
// Revision 1.17  2001/12/09 14:02:11  slicerdw
// Added gl_clear check -> video_check_glclear cvar
//
// Revision 1.16  2001/09/28 13:48:35  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.15  2001/08/19 01:22:25  deathwatch
// cleaned the formatting of some files
//
// Revision 1.14  2001/08/18 20:51:55  deathwatch
// Messing with the colours of the flashlight and the person using the
// flashlight
//
// Revision 1.13  2001/08/06 14:38:45  ra
// Adding UVtime for ctf
//
// Revision 1.12  2001/06/25 11:44:47  slicerdw
// New Video Check System - video_check and video_check_lockpvs no longer latched
//
// Revision 1.11  2001/06/21 00:05:31  slicerdw
// New Video Check System done -  might need some revision but works..
//
// Revision 1.8  2001/06/18 12:36:40  igor_rock
// added new irvision mode (with reddish screen and alpha blend) and corresponding
// new cvar "new_irvision" to enable the new mode
//
// Revision 1.7  2001/05/31 16:58:14  igor_rock
// conflicts resolved
//
// Revision 1.6  2001/05/20 15:00:19  slicerdw
// Some minor fixes and changings on Video Checking system
//
// Revision 1.5.2.3  2001/05/25 18:59:52  igor_rock
// Added CTF Mode completly :)
// Support for .flg files is still missing, but with "real" CTF maps like
// tq2gtd1 the ctf works fine.
// (I hope that all other modes still work, just tested DM and teamplay)
//
// Revision 1.5.2.2  2001/05/20 18:54:19  igor_rock
// added original ctf code snippets from zoid. lib compilesand runs but
// doesn't function the right way.
// Jsut committing these to have a base to return to if something wents
// awfully wrong.
//
// Revision 1.5.2.1  2001/05/20 15:17:31  igor_rock
// removed the old ctf code completly
//
// Revision 1.5  2001/05/11 16:07:26  mort
// Various CTF bits and pieces...
//
// Revision 1.4  2001/05/08 12:54:17  igor_rock
// removed another debug message ;)
//
// Revision 1.3  2001/05/07 21:43:02  slicerdw
// Removed Some Debug Messages Left Over
//
// Revision 1.2  2001/05/07 21:18:35  slicerdw
// Added Video Checking System
//
// Revision 1.1.1.1  2001/05/06 17:24:10  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#include "g_local.h"
#include "m_player.h"



static edict_t *current_player;
static gclient_t *current_client;

static vec3_t forward, right, up;
float xyspeed;

float bobmove;
int bobcycle;			// odd cycles are right foot going forward
float bobfracsin;		// sin(bobfrac*M_PI)


/*
===============
SV_CalcRoll

===============
*/
float SV_CalcRoll (vec3_t angles, vec3_t velocity)
{
	float sign, side, value;

	side = DotProduct (velocity, right);
	sign = side < 0 ? -1 : 1;
	side = fabs (side);

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
void P_DamageFeedback (edict_t * player)
{
	gclient_t *client;
	float side;
	float realcount, count, kick;
	vec3_t v;
	int r, l;
	static const vec3_t power_color = { 0.0, 1.0, 0.0 };
	static const vec3_t acolor = { 1.0, 1.0, 1.0 };
	static const vec3_t bcolor = { 1.0, 0.0, 0.0 };
	float max_damage_alpha = 0.6f;

	//if (!FRAMESYNC)
	//	return;

	client = player->client;

	// flash the backgrounds behind the status numbers
	if( FRAMESYNC )
		client->ps.stats[STAT_FLASHES] = 0;
	if (client->damage_blood)
		client->ps.stats[STAT_FLASHES] |= 1;
	if (client->damage_armor && !(player->flags & FL_GODMODE)
		&& (client->invincible_framenum <= level.framenum))
		client->ps.stats[STAT_FLASHES] |= 2;

	// total points of damage shot at the player this frame
	count =	client->damage_blood + client->damage_armor + client->damage_parmor;
	if (count == 0)
	{
		if( FRAMESYNC )
			client->damage_knockback = 0;

		return;			// didn't take any damage
	}

	// start a pain animation if still in the player model
	if (client->anim_priority < ANIM_PAIN && player->s.modelindex == 255)
	{
		static int i;

		if (client->ps.pmove.pm_flags & PMF_DUCKED)
			SetAnimation( player, FRAME_crpain1 - 1, FRAME_crpain4, ANIM_PAIN );
		else
		{
			i = (i + 1) % 3;
			switch (i)
			{
			case 0:
				SetAnimation( player, FRAME_pain101 - 1, FRAME_pain104, ANIM_PAIN );
				break;
			case 1:
				SetAnimation( player, FRAME_pain201 - 1, FRAME_pain204, ANIM_PAIN );
				break;
			case 2:
				SetAnimation( player, FRAME_pain301 - 1, FRAME_pain304, ANIM_PAIN );
				break;
			}
		}
	}

	realcount = count;
	if (count < 10)
		count = 10;			// always make a visible effect

	// play an apropriate pain sound
	if ((level.framenum > player->pain_debounce_framenum) && !(player->flags & FL_GODMODE)
	&& (client->invincible_framenum <= level.framenum))
	{
		r = 1 + (rand () & 1);
		player->pain_debounce_framenum = level.framenum + 0.7 * HZ;
		if (player->health < 25)
			l = 25;
		else if (player->health < 50)
			l = 50;
		else if (player->health < 75)
			l = 75;
		else
			l = 100;
		gi.sound (player, CHAN_VOICE,
		gi.soundindex (va ("*pain%i_%i.wav", l, r)), 1, ATTN_NORM, 0);
	}

	// the total alpha of the blend is always proportional to count
	if (client->damage_alpha < 0)
		client->damage_alpha = 0;
	client->damage_alpha += count * 0.01f;
	if (client->damage_alpha < 0.2f)
		client->damage_alpha = 0.2f;
	max_damage_alpha = 0.6f + (game.framediv - level.framenum % game.framediv - 1) * 0.6f * FRAMETIME;  // stay solid red in lava
	if (client->damage_alpha > max_damage_alpha)
		client->damage_alpha = max_damage_alpha;  // don't go too saturated

	// the color of the blend will vary based on how much was absorbed
	// by different armors
	VectorClear (v);
	if (client->damage_parmor)
		VectorMA (v, (float) client->damage_parmor / realcount, power_color, v);
	if (client->damage_armor)
		VectorMA (v, (float) client->damage_armor / realcount, acolor, v);
	if (client->damage_blood)
		VectorMA (v, (float) client->damage_blood / realcount, bcolor, v);
	VectorCopy (v, client->damage_blend);

	//
	// calculate view angle kicks
	//
	kick = abs (client->damage_knockback);
	if (kick && player->health > 0)	// kick of 0 means no view adjust at all
	{
		kick = kick * 100 / player->health;

		if (kick < count * 0.5)
			kick = count * 0.5;
		if (kick > 50)
			kick = 50;

		VectorSubtract (client->damage_from, player->s.origin, v);
		VectorNormalize (v);

		side = DotProduct (v, right);
		client->v_dmg_roll = kick * side * 0.3;

		side = -DotProduct (v, forward);
		client->v_dmg_pitch = kick * side * 0.3;

		client->v_dmg_time = level.time + DAMAGE_TIME;
	}

	//
	// clear totals
	//
	client->damage_blood = 0;
	client->damage_armor = 0;
	client->damage_parmor = 0;

	if( FRAMESYNC )
		client->damage_knockback = 0;
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
void SV_CalcViewOffset (edict_t * ent)
{
	float *angles;
	float bob;
	float ratio;
	float delta;
	vec3_t v = {0, 0, 0};

	//if (!FRAMESYNC)
	//	return;

	// base angles
	angles = ent->client->ps.kick_angles;

	// if dead, fix the angle and don't add any kick
	if (ent->deadflag)
	{
		VectorClear (angles);

		ent->client->ps.viewangles[ROLL] = 40;
		ent->client->ps.viewangles[PITCH] = -15;
		ent->client->ps.viewangles[YAW] = ent->client->killer_yaw;
	}
	else
	{
		// add angles based on weapon kick
		VectorCopy (ent->client->kick_angles, angles);

		// add angles based on damage kick
		ratio = (ent->client->v_dmg_time - level.time) / DAMAGE_TIME;
		if (ratio < 0)
		{
			ratio = 0;
			ent->client->v_dmg_pitch = 0;
			ent->client->v_dmg_roll = 0;
		}
		angles[PITCH] += ratio * ent->client->v_dmg_pitch;
		angles[ROLL] += ratio * ent->client->v_dmg_roll;

		// add pitch based on fall kick
		ratio = (ent->client->fall_time - level.time) / FALL_TIME;
		if (ratio < 0)
			ratio = 0;
		angles[PITCH] += ratio * ent->client->fall_value;

		// add angles based on velocity
		delta = DotProduct (ent->velocity, forward);
		angles[PITCH] += delta * run_pitch->value;

		delta = DotProduct (ent->velocity, right);
		angles[ROLL] += delta * run_roll->value;

		// add angles based on bob
		delta = bobfracsin * bob_pitch->value * xyspeed;
		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			delta *= 6;		// crouching
		angles[PITCH] += delta;
		delta = bobfracsin * bob_roll->value * xyspeed;
		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			delta *= 6;		// crouching
		if (bobcycle & 1)
			delta = -delta;
		angles[ROLL] += delta;
	}

	//===================================

	// add view height
	v[2] += ent->viewheight;

	// add fall height
	ratio = (ent->client->fall_time - level.time) / FALL_TIME;
	if (ratio < 0)
		ratio = 0;
	v[2] -= ratio * ent->client->fall_value * 0.4f;

	// add bob height
	bob = bobfracsin * xyspeed * bob_up->value;
	if (bob > 6)
		bob = 6;
	//gi.DebugGraph (bob *2, 255);
	v[2] += bob;

	// add kick offset
	VectorAdd (v, ent->client->kick_origin, v);

	// absolutely bound offsets
	// so the view can never be outside the player box
	clamp(v[0], -14, 14);
	clamp(v[1], -14, 14);
	clamp(v[2], -22, 30);

	VectorCopy (v, ent->client->ps.viewoffset);
}

/*
==============
SV_CalcGunOffset
==============
*/
void SV_CalcGunOffset (edict_t * ent)
{
	int i;
	float delta;

	if (!FRAMESYNC)
		return;

	// gun angles from bobbing
	ent->client->ps.gunangles[ROLL] = xyspeed * bobfracsin * 0.005;
	ent->client->ps.gunangles[YAW] = xyspeed * bobfracsin * 0.01;
	if (bobcycle & 1)
	{
		ent->client->ps.gunangles[ROLL] = -ent->client->ps.gunangles[ROLL];
		ent->client->ps.gunangles[YAW] = -ent->client->ps.gunangles[YAW];
	}

	ent->client->ps.gunangles[PITCH] = xyspeed * bobfracsin * 0.005;

	// gun angles from delta movement
	for (i = 0; i < 3; i++)
	{
		delta = ent->client->oldviewangles[i] - ent->client->ps.viewangles[i];
		//delta *= game.framediv;
		if (delta > 180)
			delta -= 360;
		if (delta < -180)
			delta += 360;
		if (delta > 45)
			delta = 45;
		if (delta < -45)
			delta = -45;
		if (i == YAW)
			ent->client->ps.gunangles[ROLL] += 0.1 * delta;
		ent->client->ps.gunangles[i] += 0.2 * delta;
	}

	// gun height
	VectorClear (ent->client->ps.gunoffset);
	//      ent->ps->gunorigin[2] += bob;

	// gun_x / gun_y / gun_z are development tools
	for (i = 0; i < 3; i++)
	{
		ent->client->ps.gunoffset[i] += forward[i] * (gun_y->value);
		ent->client->ps.gunoffset[i] += right[i] * gun_x->value;
		ent->client->ps.gunoffset[i] += up[i] * (-gun_z->value);
	}

	VectorCopy( ent->client->ps.viewangles, ent->client->oldviewangles );
}


/*
=============
SV_AddBlend
=============
*/
void SV_AddBlend (float r, float g, float b, float a, float *v_blend)
{
	float a2, a3;

	if (a <= 0)
		return;
	a2 = v_blend[3] + (1 - v_blend[3]) * a;	// new total alpha
	a3 = v_blend[3] / a2;		// fraction of color from old

	v_blend[0] = v_blend[0] * a3 + r * (1 - a3);
	v_blend[1] = v_blend[1] * a3 + g * (1 - a3);
	v_blend[2] = v_blend[2] * a3 + b * (1 - a3);
	v_blend[3] = a2;
}


/*
=============
SV_CalcBlend
=============
*/
void SV_CalcBlend (edict_t * ent)
{
	int contents;
	vec3_t vieworg;
	int remaining;

	// enable ir vision if appropriate
	if (ir->value)
	{
		if ((INV_AMMO(ent, BAND_NUM) &&	ent->client->pers.irvision) ||
			(ent->client->chase_target != NULL &&
			 ent->client->chase_target->client != NULL &&
			 ent->client->chase_mode == 2 &&
			ent->client->chase_target->client->pers.irvision &&
			INV_AMMO(ent->client->chase_target, BAND_NUM)))
		{
			ent->client->ps.rdflags |= RDF_IRGOGGLES;
		}
		else
		{
			ent->client->ps.rdflags &= ~RDF_IRGOGGLES;
		}
	}

	ent->client->ps.blend[0] = ent->client->ps.blend[1] =
	ent->client->ps.blend[2] = ent->client->ps.blend[3] = 0;

	// add for contents
	VectorAdd (ent->s.origin, ent->client->ps.viewoffset, vieworg);
	contents = gi.pointcontents (vieworg);
	if (contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WATER))
		ent->client->ps.rdflags |= RDF_UNDERWATER;
	else
		ent->client->ps.rdflags &= ~RDF_UNDERWATER;

	if (contents & (CONTENTS_SOLID | CONTENTS_LAVA))
		SV_AddBlend (1.0f, 0.3f, 0.0f, 0.6f, ent->client->ps.blend);
	else if (contents & CONTENTS_SLIME)
		SV_AddBlend (0.0f, 0.1f, 0.05f, 0.6f, ent->client->ps.blend);
	else if (contents & CONTENTS_WATER)
		SV_AddBlend (0.5f, 0.3f, 0.2f, 0.4f, ent->client->ps.blend);

	// AQ2:TNG - Igor[Rock] adding new irvision mode
	if (new_irvision->value && ent->client->pers.irvision && INV_AMMO(ent, BAND_NUM))
	{
		SV_AddBlend (0.1f, 0.0f, 0.0f, 0.4f, ent->client->ps.blend);
	}
	// AQ2:TNG end of new irvision mode

	// add for powerups
	if (ent->client->quad_framenum > level.framenum)
	{
		remaining = ent->client->quad_framenum - level.framenum;
		if (remaining == 3 * HZ)	// beginning to fade
			gi.sound (ent, CHAN_ITEM, gi.soundindex("items/damage2.wav"), 1, ATTN_NORM, 0);
		if (remaining > 3 * HZ || ((remaining / FRAMEDIV) & 4))
			SV_AddBlend (0, 0, 1, 0.08f, ent->client->ps.blend);
	}
	else if (ent->client->invincible_framenum > level.framenum)
	{
		remaining = ent->client->invincible_framenum - level.framenum;
		if (remaining == 3 * HZ)	// beginning to fade
			gi.sound (ent, CHAN_ITEM, gi.soundindex("items/protect2.wav"), 1, ATTN_NORM, 0);
		if (remaining > 3 * HZ || ((remaining / FRAMEDIV) & 4))
			SV_AddBlend (1, 1, 0, 0.08f, ent->client->ps.blend);
	}
	else if (ent->client->enviro_framenum > level.framenum)
	{
		remaining = ent->client->enviro_framenum - level.framenum;
		if (remaining == 3 * HZ)	// beginning to fade
			gi.sound (ent, CHAN_ITEM, gi.soundindex("items/airout.wav"), 1, ATTN_NORM, 0);
		if (remaining > 3 * HZ || ((remaining / FRAMEDIV) & 4))
			SV_AddBlend (0, 1, 0, 0.08f, ent->client->ps.blend);
	}
	else if (ent->client->breather_framenum > level.framenum)
	{
		remaining = ent->client->breather_framenum - level.framenum;
		if (remaining == 3 * HZ)	// beginning to fade
			gi.sound (ent, CHAN_ITEM, gi.soundindex ("items/airout.wav"), 1, ATTN_NORM, 0);
		if (remaining > 3 * HZ || ((remaining / FRAMEDIV) & 4))
			SV_AddBlend (0.4f, 1, 0.4f, 0.04f, ent->client->ps.blend);
	}

	// add for damage
	if (ent->client->damage_alpha > 0)
		SV_AddBlend (ent->client->damage_blend[0], ent->client->damage_blend[1],
	ent->client->damage_blend[2], min( 0.6f, ent->client->damage_alpha ),
	ent->client->ps.blend);

	if (ent->client->bonus_alpha > 0)
		SV_AddBlend (0.85f, 0.7f, 0.3f, ent->client->bonus_alpha,
	ent->client->ps.blend);

	// drop the damage value
	ent->client->damage_alpha -= 0.6f * FRAMETIME;
	if (ent->client->damage_alpha < 0)
		ent->client->damage_alpha = 0;

	// drop the bonus value
	ent->client->bonus_alpha -= 1.0f * FRAMETIME;
	if (ent->client->bonus_alpha < 0)
		ent->client->bonus_alpha = 0;
}


/*
========
OnLadder
========
*/
qboolean OnLadder( edict_t *ent )
{
	float yaw_rad = 0;
	vec3_t fwd = {0}, end = {0};
	trace_t tr;

	if( ! IS_ALIVE(ent) )
		return false;

	yaw_rad = DEG2RAD(ent->s.angles[YAW]);
	fwd[0] = cos(yaw_rad);
	fwd[1] = sin(yaw_rad);

	VectorMA( ent->s.origin, 1, fwd, end );

	tr = gi.trace( ent->s.origin, ent->mins, ent->maxs, end, ent, MASK_PLAYERSOLID );

	return ((tr.fraction < 1) && (tr.contents & CONTENTS_LADDER));
}


/*
=================
P_FallingDamage
=================
*/
void P_FallingDamage (edict_t * ent)
{
	float delta;
	int damage;
	vec3_t dir, oldvelocity;

	//if (!FRAMESYNC)
	//	return;

	VectorCopy( ent->client->oldvelocity, oldvelocity );
	VectorCopy( ent->velocity, ent->client->oldvelocity );

	ent->client->old_ladder = ent->client->ladder;
	ent->client->ladder = OnLadder(ent);

	if (lights_camera_action || ent->client->uvTime > 0)
		return;
	
	if (ent->s.modelindex != 255)
		return;			// not in the player model

	if (ent->movetype == MOVETYPE_NOCLIP)
		return;

	if ((oldvelocity[2] < 0)
		&& (ent->velocity[2] > oldvelocity[2])
		&& (!ent->groundentity))
	{
		delta = oldvelocity[2];
	}
	else
	{
		if (!ent->groundentity)
			return;
		delta = ent->velocity[2] - oldvelocity[2];
		ent->client->jumping = 0;
	}
	delta = delta * delta * 0.0001;

	// never take damage if just release grapple or on grapple
	if (level.framenum - ent->client->ctf_grapplereleaseframe <= 2*FRAMEDIV ||
			(ent->client->ctf_grapple &&
			 ent->client->ctf_grapplestate > CTF_GRAPPLE_STATE_FLY))
			return;

	// never take falling damage if completely underwater
	if (ent->waterlevel == 3)
		return;
	else if (ent->waterlevel == 2)
		delta *= 0.25;
	else if (ent->waterlevel == 1)
		delta *= 0.5;

	if (delta < 1)
		return;

	if (delta < 15)
	{
		// Raptor007: Don't make footsteps when climbing down ladders.
		if( ent->client->old_ladder )
			return;

		// zucc look for slippers to avoid noise
		if(!INV_AMMO(ent, SLIP_NUM))
			ent->s.event = EV_FOOTSTEP;

		return;
	}

	ent->client->fall_value = delta * 0.5;
	if (ent->client->fall_value > 40)
		ent->client->fall_value = 40;
	ent->client->fall_time = level.time + FALL_TIME;

	if (delta <= 30)
	{
		//zucc added check for slippers, this is just another noise
		if(!INV_AMMO(ent, SLIP_NUM))
			ent->s.event = EV_FALLSHORT;

		return;
	}

	/* when fall damage is disabled, play the normal fall sound */
	if(DMFLAGS(DF_NO_FALLING))
	{
		ent->s.event = EV_FALLSHORT;
		return;
	}


	if (ent->health > 0)
	{
		if (delta >= 55)
			ent->s.event = EV_FALLFAR;
		else			// all falls are far
			ent->s.event = EV_FALLFAR;
	}

	ent->pain_debounce_framenum = KEYFRAME(FRAMEDIV);	// no normal pain sound

	if (!DMFLAGS(DF_NO_FALLING))
	{
		damage = (int) (((delta - 30) / 2));
		if (damage < 1)
			damage = 1;
		// zucc scale this up
		damage *= 10;

		// darksaint - reduce damage if e_enhancedSlippers are on and equipped
		if (e_enhancedSlippers->value && INV_AMMO(ent, SLIP_NUM))
			damage /= 2;

		VectorSet (dir, 0, 0, 1);

		if (jump->value)
		{
			gi.cprintf(ent, PRINT_HIGH, "Fall Damage: %d\n", damage);
			ent->client->resp.jmp_falldmglast = damage;
		} else {
		T_Damage (ent, world, world, dir, ent->s.origin, vec3_origin,
			damage, 0, 0, MOD_FALLING);
		}
	}
}



/*
=============
P_WorldEffects
=============
*/
void P_WorldEffects (void)
{
	qboolean breather;
	qboolean envirosuit;
	int waterlevel, old_waterlevel;

	if (current_player->movetype == MOVETYPE_NOCLIP)
	{
		current_player->air_finished_framenum = level.framenum + 12 * HZ;	// don't need air
		return;
	}

	waterlevel = current_player->waterlevel;
	old_waterlevel = current_client->old_waterlevel;
	current_client->old_waterlevel = waterlevel;

	breather = current_client->breather_framenum > level.framenum;
	envirosuit = current_client->enviro_framenum > level.framenum;

	//
	// if just entered a water volume, play a sound
	//
	if (!old_waterlevel && waterlevel)
	{
		PlayerNoise (current_player, current_player->s.origin, PNOISE_SELF);
		if (current_player->watertype & CONTENTS_LAVA)
			gi.sound (current_player, CHAN_BODY,
			gi.soundindex ("player/lava_in.wav"), 1, ATTN_NORM, 0);
		else if (current_player->watertype & CONTENTS_SLIME)
			gi.sound (current_player, CHAN_BODY,
			gi.soundindex ("player/watr_in.wav"), 1, ATTN_NORM, 0);
		else if (current_player->watertype & CONTENTS_WATER)
			gi.sound (current_player, CHAN_BODY,
			gi.soundindex ("player/watr_in.wav"), 1, ATTN_NORM, 0);

		current_player->flags |= FL_INWATER;

		// clear damage_debounce, so the pain sound will play immediately
		current_player->damage_debounce_framenum = level.framenum - 1 * HZ;
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if (old_waterlevel && !waterlevel)
	{
		PlayerNoise (current_player, current_player->s.origin, PNOISE_SELF);
		gi.sound (current_player, CHAN_BODY,
		gi.soundindex ("player/watr_out.wav"), 1, ATTN_NORM, 0);
		current_player->flags &= ~FL_INWATER;
	}

	//
	// check for head just going under water
	//
	if (old_waterlevel != 3 && waterlevel == 3)
	{
		gi.sound (current_player, CHAN_BODY,
		gi.soundindex ("player/watr_un.wav"), 1, ATTN_NORM, 0);
	}

	//
	// check for head just coming out of water
	//
	if (old_waterlevel == 3 && waterlevel != 3)
	{
		if (current_player->air_finished_framenum < level.framenum)
		{			// gasp for air
			gi.sound (current_player, CHAN_VOICE,
			gi.soundindex ("player/gasp1.wav"), 1, ATTN_NORM, 0);
			PlayerNoise (current_player, current_player->s.origin, PNOISE_SELF);
		}
		else if (current_player->air_finished_framenum < level.framenum + 11 * HZ)
		{			// just break surface
			gi.sound (current_player, CHAN_VOICE,
			gi.soundindex ("player/gasp2.wav"), 1, ATTN_NORM, 0);
		}
	}

	//
	// check for drowning
	//
	if (waterlevel == 3)
	{
		// breather or envirosuit give air
		// AQ2 doesn't use the rebreather
		// if (breather || envirosuit)
		// {
		// 	current_player->air_finished_framenum = level.framenum + 10 * HZ;

		// 	if (((current_client->breather_framenum - level.framenum) % (25 * FRAMEDIV)) == 0)
		// 	{
		// 		if (!current_client->breather_sound)
		// 			gi.sound (current_player, CHAN_AUTO,
		// 			gi.soundindex("player/u_breath1.wav"), 1, ATTN_NORM, 0);
		// 		else
		// 			gi.sound (current_player, CHAN_AUTO,
		// 			gi.soundindex("player/u_breath2.wav"), 1, ATTN_NORM, 0);
		// 		current_client->breather_sound ^= 1;
		// 		PlayerNoise (current_player, current_player->s.origin,
		// 		PNOISE_SELF);
		// 		//FIXME: release a bubble?
		// 	}
		// }

		// if out of air, start drowning
		if (current_player->air_finished_framenum < level.framenum)
		{			// drown!
			if (current_player->client->next_drown_framenum < level.framenum
				&& current_player->health > 0)
			{
				current_player->client->next_drown_framenum = level.framenum + HZ;

				// take more damage the longer underwater
				current_player->dmg += 2;
				if (current_player->dmg > 15)
					current_player->dmg = 15;

				// play a gurp sound instead of a normal pain sound
				if (current_player->health <= current_player->dmg)
					gi.sound (current_player, CHAN_VOICE,
					gi.soundindex ("player/drown1.wav"), 1, ATTN_NORM, 0);
				else if (rand () & 1)
					gi.sound (current_player, CHAN_VOICE,
					gi.soundindex ("*gurp1.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound (current_player, CHAN_VOICE,
					gi.soundindex ("*gurp2.wav"), 1, ATTN_NORM, 0);

				current_player->pain_debounce_framenum = KEYFRAME(FRAMEDIV);

				T_Damage (current_player, world, world, vec3_origin,
				current_player->s.origin, vec3_origin,
				current_player->dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
			}
		}
	}
	else
	{
		current_player->air_finished_framenum = level.framenum + 12 * HZ;
		current_player->dmg = 2;
	}

	//
	// check for sizzle damage
	//
	if (waterlevel && (current_player->watertype & (CONTENTS_LAVA | CONTENTS_SLIME)))
	{
		if (current_player->watertype & CONTENTS_LAVA)
		{
			if (current_player->health > 0
			&& current_player->pain_debounce_framenum <= level.framenum
			&& current_client->invincible_framenum < level.framenum)
			{
				if (rand () & 1)
					gi.sound (current_player, CHAN_VOICE,
					gi.soundindex ("player/burn1.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound (current_player, CHAN_VOICE,
					gi.soundindex ("player/burn2.wav"), 1, ATTN_NORM, 0);

				current_player->pain_debounce_framenum = level.framenum + 1 * HZ;
			}

			if (FRAMESYNC)
			{
				if (envirosuit)	// take 1/3 damage with envirosuit
					T_Damage (current_player, world, world, vec3_origin,
					current_player->s.origin, vec3_origin, 1 * waterlevel, 0, 0, MOD_LAVA);
				else
					T_Damage (current_player, world, world, vec3_origin,
					current_player->s.origin, vec3_origin, 3 * waterlevel, 0, 0, MOD_LAVA);
			}
		}

		if (current_player->watertype & CONTENTS_SLIME && !envirosuit && FRAMESYNC)
		{			// no damage from slime with envirosuit
			T_Damage (current_player, world, world, vec3_origin,
				current_player->s.origin, vec3_origin, 1 * waterlevel, 0, 0, MOD_SLIME);
		}
	}
}


/*
===============
G_SetClientEffects
===============
*/
void G_SetClientEffects (edict_t * ent)
{
	int remaining;

	ent->s.effects = 0;
	// zucc added RF_IR_VISIBLE
	//FB 6/1/99 - only for live players
	if (ent->deadflag != DEAD_DEAD)
		ent->s.renderfx = RF_IR_VISIBLE;
	else
		ent->s.renderfx = 0;

	if (ent->health <= 0 || level.intermission_framenum)
		return;

	if (ctf->value)
		CTFEffects (ent);

	if (ent->client->quad_framenum > level.framenum)
	{
		remaining = ent->client->quad_framenum - level.framenum;
		if (remaining > 3 * HZ || ((remaining / FRAMEDIV) & 4))
			ent->s.effects |= EF_QUAD;
	}

	if (ent->client->invincible_framenum > level.framenum)
	{
		remaining = ent->client->invincible_framenum - level.framenum;
		if (remaining > 3 * HZ || ((remaining / FRAMEDIV) & 4))
			ent->s.effects |= EF_PENT;
	}

	// show cheaters!!!
	if (ent->flags & FL_GODMODE)
	{
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= (RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE);
	}
	// AQ2:TNG - JBravo adding UVtime
	if ((ent->client->uvTime & 4) || (ctf->value && (lights_camera_action & 4)))
	{
		ent->s.effects |= EF_COLOR_SHELL;
		if (ent->client->resp.team == TEAM1)
			ent->s.renderfx |= RF_SHELL_RED;
		else if (ent->client->resp.team == TEAM2)
			ent->s.renderfx |= RF_SHELL_BLUE;
		else
			ent->s.renderfx |= RF_SHELL_GREEN;
	}
	// TNG Flashlight
	if( (darkmatch->value || use_flashlight->value) && ent->client && ent->client->flashlight )
	{
		//ent->s.effects |= EF_YELLOWSHELL; // no good one? :/
		ent->s.renderfx |= RF_FULLBRIGHT;
	}
}


/*
===============
G_SetClientEvent
===============
*/
void G_SetClientEvent (edict_t * ent)
{
	int footstep_speed = 225;
	float bobmove2 = bobmove;

	if (ent->s.event)
		return;

	//if (sync_footsteps->value)
	{
		if (!FRAMESYNC)
			return;
		bobmove2 *= game.framediv;
	}

	footstep_speed = silentwalk->value ? 290 : 225;
	if (ent->groundentity && (xyspeed > footstep_speed))
	{
		//zucc added item check to see if they have slippers
		if ((int)(current_client->bobtime + bobmove2) != bobcycle && !INV_AMMO(ent, SLIP_NUM))
			ent->s.event = EV_FOOTSTEP;
	}
}

/*
===============
G_SetClientSound
===============
*/
void G_SetClientSound (edict_t * ent)
{
	if (ent->waterlevel && (ent->watertype & (CONTENTS_LAVA | CONTENTS_SLIME)))
		ent->s.sound = level.snd_fry;
	else
		ent->s.sound = 0;
}


void SetAnimation( edict_t *ent, int frame, int anim_end, int anim_priority )
{
	ent->s.frame = frame;
	ent->client->anim_end = anim_end;
	ent->client->anim_priority = anim_priority;
	ent->client->anim_started = level.framenum;
}

/*
===============
G_SetClientFrame
===============
*/
void G_SetClientFrame (edict_t * ent)
{
	gclient_t *client;
	qboolean duck, run;
	qboolean anim_framesync;

	if (ent->s.modelindex != 255)
		return;			// not in the player model

	//if (!FRAMESYNC)
	//	return;

	client = ent->client;

	if (client->ps.pmove.pm_flags & PMF_DUCKED)
		duck = true;
	else
		duck = false;
	if (xyspeed)
		run = true;
	else
		run = false;

	anim_framesync = level.framenum % game.framediv == client->anim_started % game.framediv;

	// check for stand/duck and stop/go transitions
	if( anim_framesync || (level.framenum >= client->anim_started + game.framediv) )
	{
		if (duck != client->anim_duck && client->anim_priority < ANIM_DEATH)
			goto newanim;
		if (run != client->anim_run && client->anim_priority == ANIM_BASIC)
			goto newanim;
		if (!ent->groundentity && client->anim_priority <= ANIM_WAVE)
			goto newanim;
	}

	if( ! anim_framesync )
		return;

	// zucc vwep
	if (client->anim_priority == ANIM_REVERSE)
	{
		if (ent->s.frame > client->anim_end)
		{
			ent->s.frame--;
			return;
		}
	}
	else if (ent->s.frame < client->anim_end)
	{	// continue an animation
		ent->s.frame++;
		return;
	}

	if (client->anim_priority == ANIM_DEATH)
		return;			// stay there
	if (client->anim_priority == ANIM_JUMP)
	{
		if (!ent->groundentity)
			return;			// stay there
		ent->client->anim_priority = ANIM_WAVE;
		ent->s.frame = FRAME_jump3;
		ent->client->anim_end = FRAME_jump6;
		return;
	}

newanim:
	client->anim_started = level.framenum;

	// return to either a running or standing frame
	client->anim_priority = ANIM_BASIC;
	client->anim_duck = duck;
	client->anim_run = run;

	if (!ent->groundentity)
	{
		// if on grapple, don't go into jump frame, go into standing
		if (client->ctf_grapple) {
			ent->s.frame = FRAME_stand01;
			client->anim_end = FRAME_stand40;
		}

		client->anim_priority = ANIM_JUMP;
		if (ent->s.frame != FRAME_jump2)
			ent->s.frame = FRAME_jump1;
		client->anim_end = FRAME_jump2;
	}
	else if (run)
	{				// running
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
	{				// standing
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

void Do_Bleeding (edict_t * ent)
{
	int damage;
	int temp;
	//vec3_t norm = {0.0, 0.0, 0.0};

	if (!FRAMESYNC)
		return;

	if (!(ent->client->bleeding) || (ent->health <= 0))
		return;

	temp = (int) (ent->client->bleeding * .2);
	ent->client->bleeding -= temp;
	if (temp <= 0)
		temp = 1;
	ent->client->bleed_remain += temp;
	damage = (int) (ent->client->bleed_remain / BLEED_TIME);
	if (ent->client->bleed_remain >= BLEED_TIME)
	{
		ent->health -= damage;
		if (damage > 1)
		{
			// action doens't do this
			//ent->client->damage_blood += damage; // for feedback                                
		}
		if (ent->health <= 0)
		{
			meansOfDeath = ent->client->attacker_mod;
			locOfDeath = ent->client->attacker_loc;
			Killed(ent, ent->client->attacker, ent->client->attacker, damage, ent->s.origin);
		}
		else
		{
			ent->client->bleed_remain %= BLEED_TIME;
		}
		if (ent->client->bleeddelay <= level.framenum)
		{			
			vec3_t fwd, right, up, pos, vel;

			ent->client->bleeddelay = level.framenum + 2 * HZ;  // 2 seconds
			AngleVectors( ent->s.angles, fwd, right, up );
			vel[0] = fwd[0] * ent->client->bleedloc_offset[0] + right[0] * ent->client->bleedloc_offset[1] + up[0] * ent->client->bleedloc_offset[2];
			vel[1] = fwd[1] * ent->client->bleedloc_offset[0] + right[1] * ent->client->bleedloc_offset[1] + up[1] * ent->client->bleedloc_offset[2];
			vel[2] = fwd[2] * ent->client->bleedloc_offset[0] + right[2] * ent->client->bleedloc_offset[1] + up[2] * ent->client->bleedloc_offset[2];
			VectorAdd( ent->s.origin, vel, pos );
			if( vel[2] < 0. )
				vel[2] = 0;
			
			//gi.cprintf(ent, PRINT_HIGH, "Bleeding now.\n");
			EjectBlooder( ent, pos, vel );
		}
	}

}


void Do_MedKit( edict_t *ent )
{
	int i = 0;

	// Synchronize with weapon_framesync for consistent healing, as bandaging time is controlled by weapon think.
	if( level.framenum % game.framediv != ent->client->weapon_last_activity % game.framediv )
		return;

	if( ent->health <= 0 )
		return;
	if( ! IS_ALIVE(ent) )
		return;

	// Heal from medkit only while bandaging.
	if( !(ent->client->bandaging || ent->client->bandage_stopped) )
		return;

	// If there is bleeding or leg damage, take care of that separately before using medkit.
	if( ent->client->bandaging && (ent->client->bleeding || ent->client->leg_damage) )
		return;

	for( i = 0; i < 2; i ++ )
	{
		// Make sure we have any medkit and need to use it.
		if( ent->client->medkit <= 0 )
			return;
		if( ent->health >= ent->max_health )
			return;

		ent->health ++;
		ent->client->medkit --;
	}
}


int canFire (edict_t * ent)
{
	int result = 0;

	switch (ent->client->curr_weap)
	{
	case MK23_NUM:
		if (ent->client->mk23_rds > 0)
			result = 1;
		break;
	case MP5_NUM:
		if (ent->client->mp5_rds > 0)
			result = 1;
		break;
	case M4_NUM:
		if (ent->client->m4_rds > 0)
			result = 1;
		break;
	case M3_NUM:
		if (ent->client->shot_rds > 0)
			result = 1;
		break;
	case HC_NUM:
		if (ent->client->cannon_rds == 2)
			result = 1;
		break;
	case SNIPER_NUM:
		if (ent->client->sniper_rds > 0)
			result = 1;
		break;
	case DUAL_NUM:
		if (ent->client->dual_rds > 0)
			result = 1;
		break;
	default:
		result = 0;
		break;
	}

	return result;
}


void FrameEndZ( edict_t *ent )
{
#ifndef NO_FPS
	int i;

	if( FRAMEDIV == 1 )
		return;

	// Advance the history.
	for( i = FRAMEDIV - 1; i >= 1; i -- )
		ent->z_history[ i ] = ent->z_history[ i - 1 ];

	// Store the real origin[2] in z_history[0] to be restored next frame.
	ent->z_history[0] = ent->s.origin[2];

	// Only smooth Z-axis values when walking on regular ground.
	if(  ent->inuse && IS_ALIVE(ent)
	&&  (ent->client->ps.pmove.pm_type == PM_NORMAL)
	&& !(ent->client->ps.pmove.pm_flags & PMF_NO_PREDICTION)
	&&  (ent->client->ps.pmove.pm_flags & PMF_ON_GROUND)
	&&  (ent->groundentity == &(globals.edicts[0])) )
	{
		if( ent->z_history_count < FRAMEDIV )
			ent->z_history_count ++;
	}
	else
		ent->z_history_count = 0;

	// If we have multiple valid frames to smooth, temporarily set origin[2] as the average.
	if( ent->z_history_count > 1 )
	{
		ent->z_history_framenum = level.framenum;

		for( i = 1; i < ent->z_history_count; i ++ )
			ent->s.origin[2] += ent->z_history[ i ];

		ent->s.origin[2] /= (float) ent->z_history_count;
/*
		// Smooth in-eyes spectator height.
		// Commented-out because this causes player views to jiggle when walking up ramps!
		if( game.serverfeatures & GMF_CLIENTNUM )
		{
			ent->z_pmove = ent->client->ps.pmove.origin[2];
			ent->client->ps.pmove.origin[2] = ent->s.origin[2] * 8;
		}
*/
	}
#endif
}

/*
=================
ClientEndServerFrame

Called for each player at the end of the server frame
and right after spawning
=================
*/
void ClientEndServerFrame (edict_t * ent)
{
	int i;
	//char player_name[30];
	//char temp[40];
	//        int             damage; // zucc for bleeding
	qboolean weapon_framesync;

	current_player = ent;
	current_client = ent->client;

	//AQ2:TNG - Slicer : Stuffs the client x seconds after he enters the server, needed for Video check
	if (ent->client->resp.checkframe[0] <= level.framenum)
	{
		ent->client->resp.checkframe[0] = level.framenum + (int)(video_checktime->value * HZ);
		if (video_check->value || video_check_lockpvs->value
			|| video_check_glclear->value || darkmatch->value)
			stuffcmd (ent, "%!fc $vid_ref\n");
		if (video_force_restart->value && video_check->value && !ent->client->resp.checked)
		{
			stuffcmd (ent, "vid_restart\n");
			ent->client->resp.checked = true;
		}

	}
	else if (ent->client->resp.checkframe[1] <= level.framenum)
	{
		ent->client->resp.checkframe[1] = level.framenum + (int)(video_checktime->value * HZ);
		ent->client->resp.checkframe[2] = level.framenum + HZ;
		if (video_check->value || video_check_lockpvs->value
			|| video_check_glclear->value || darkmatch->value)
		{
			if (Q_stricmp(ent->client->resp.vidref, "soft"))
				stuffcmd (ent, "%cpsi $gl_modulate $gl_lockpvs $gl_clear $gl_dynamic $gl_driver\n");
		}

	}
	else if (ent->client->resp.checkframe[2] <= level.framenum)
	{
		ent->client->resp.checkframe[2] = level.framenum + 100 * HZ;
		if (video_check->value || video_check_lockpvs->value
			|| video_check_glclear->value || darkmatch->value)
		{
			if (Q_stricmp(ent->client->resp.vidref, "soft"))
				VideoCheckClient (ent);
		}
	}

	if (level.pauseFrames) {
		G_SetStats (ent);
		FrameEndZ( ent );
		return;
	}

	//
	// If the origin or velocity have changed since ClientThink(),
	// update the pmove values.  This will happen when the client
	// is pushed by a bmodel or kicked by an explosion.
	// 
	// If it wasn't updated here, the view position would lag a frame
	// behind the body position when pushed -- "sinking into plats"
	//
	for (i = 0; i < 3; i++)
	{
		current_client->ps.pmove.origin[i] = ent->s.origin[i] * 8.0;
		current_client->ps.pmove.velocity[i] = ent->velocity[i] * 8.0;
	}

	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//
	if (level.intermission_framenum) {
		current_client->ps.blend[3] = 0;
		current_client->ps.fov = 90;
		current_client->desired_fov = 90;
		G_SetStats(ent);
		FrameEndZ( ent );
		return;
	}

	//FIREBLADE - Unstick avoidance stuff.
	if (ent->solid == SOLID_TRIGGER && !lights_camera_action && !jump->value)
	{
		edict_t *overlap;
		if ((overlap = FindOverlap(ent, NULL)) == NULL)
		{
			ent->solid = SOLID_BBOX;
			gi.linkentity(ent);
			RemoveFromTransparentList(ent);
		}
		else
		{
			do
			{
				if (overlap->solid == SOLID_BBOX)
				{
					overlap->solid = SOLID_TRIGGER;
					gi.linkentity(overlap);
					AddToTransparentList(overlap);
				}
				overlap = FindOverlap(ent, overlap);
			} while (overlap != NULL);
		}
	}

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
	ent->s.angles[ROLL] = SV_CalcRoll (ent->s.angles, ent->velocity) * 4;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	//if (FRAMESYNC)
	{
		xyspeed = sqrtf(ent->velocity[0]*ent->velocity[0] + ent->velocity[1]*ent->velocity[1]);

		if (xyspeed < 5 || ent->solid == SOLID_NOT)
		{
			bobmove = 0;
			current_client->bobtime = 0;	// start at beginning of cycle again
		}
		else if (ent->groundentity)
		{	// so bobbing only cycles when on ground
			if (xyspeed > 210)
				bobmove = 0.25;
			else if (xyspeed > 100)
				bobmove = 0.125;
			else
				bobmove = 0.0625;
		}
		else
			bobmove = 0;

		bobmove /= game.framediv;
		if (current_client->ps.pmove.pm_flags & PMF_DUCKED)
			bobmove *= 4;

		current_client->bobtime += bobmove;

		bobcycle = (int) current_client->bobtime;
		bobfracsin = fabsf (sinf (current_client->bobtime * M_PI));
	}

	// detect hitting the floor
	P_FallingDamage (ent);

	// zucc handle any bleeding damage here
	Do_Bleeding (ent);

	// If we have a medkit and are bandaging, gradually transfer medkit to health.
	Do_MedKit (ent);

	// apply all the damage taken this frame
	P_DamageFeedback (ent);

	// determine the view offsets
	SV_CalcViewOffset (ent);

	// determine the gun offsets
	SV_CalcGunOffset (ent);

	// determine the full screen color blend
	// must be after viewoffset, so eye contents can be
	// accurately determined
	// FIXME: with client prediction, the contents
	// should be determined by the client
	SV_CalcBlend (ent);

	G_SetStats (ent);

/*
	// FIXME: Remove this section?
	//FIREBLADE
	for (i = 1; i <= game.maxclients; i++)
	{
		int stats_copy;
		edict_t *e = g_edicts + i;

		if (!ent->inuse || e->client->chase_mode == 0 || e->client->chase_target != ent)
			continue;

		for (stats_copy = 0; stats_copy < MAX_STATS; stats_copy++)
		{
			if (stats_copy == STAT_FLAG_PIC)
			{
				if (e->client->chase_mode != 2)
					continue;	// only show team/flag icon when in chase mode 2
			}
			else if (stats_copy >= STAT_TEAM_HEADER && stats_copy <= STAT_TEAM2_SCORE)
				continue;		// protect these
			if (stats_copy >= STAT_TEAM3_PIC && stats_copy <= STAT_TEAM3_SCORE)
				continue;		// protect these
			if (stats_copy == STAT_LAYOUTS || stats_copy == STAT_ID_VIEW)
				continue;		// protect these
			if (stats_copy == STAT_SNIPER_ICON && e->client->chase_mode != 2)
				continue;		// only show sniper lens when in chase mode 2
			if (stats_copy == STAT_FRAGS)
				continue;
			e->client->ps.stats[stats_copy] = ent->client->ps.stats[stats_copy];
		}

		//
		// help icon / bandaging icon / current weapon if not shown
		//
		if (e->client->resp.helpchanged && (level.framenum & 8))
			e->client->ps.stats[STAT_HELPICON] = gi.imageindex ("i_help");
		// TNG: Show health icon when bandaging (thanks to Dome for this code)
		else if (ent->client->weaponstate == WEAPON_BANDAGING || ent->client->bandaging || ent->client->bandage_stopped)
			e->client->ps.stats[STAT_HELPICON] = gi.imageindex ("i_health");
		else if ((e->client->pers.hand == CENTER_HANDED || e->client->ps.fov > 91) && ent->client->pers.weapon && e->client->chase_mode == 2)
			e->client->ps.stats[STAT_HELPICON] = gi.imageindex (ent->client->pers.weapon->icon);
		else
			e->client->ps.stats[STAT_HELPICON] = 0;

	//FB                e->client->ps.stats[STAT_LAYOUTS] = 1;
	//FB                break;
	}
	//FIREBLADE
*/

	G_SetClientEvent (ent);

	G_SetClientEffects (ent);

	G_SetClientSound (ent);

	G_SetClientFrame (ent);

	// Raptor007: Fix scooting on platforms if ClientThink didn't happen this frame.
	if( ent->groundentity && (ent->groundentity->linkcount == ent->groundentity_linkcount + 1) )
		ent->groundentity_linkcount = ent->groundentity->linkcount;

	// zucc - clear the open door command
	ent->client->doortoggle = 0;


	// If they just spawned, sync up the weapon animation with that.
	if( ! ent->client->weapon_last_activity )
		ent->client->weapon_last_activity = level.framenum;

	weapon_framesync = (level.framenum % game.framediv == ent->client->weapon_last_activity % game.framediv);

	if (ent->client->reload_attempts > 0)
	{
		if( ((ent->client->latched_buttons | ent->client->buttons) & BUTTON_ATTACK) && canFire(ent) )
			ent->client->reload_attempts = 0;
		else if( weapon_framesync )
			Cmd_Reload_f (ent);
	}

	if( (ent->client->weapon_attempts > 0) && weapon_framesync )
		Cmd_Weapon_f (ent);

	FrameEndZ( ent );

	if (!FRAMESYNC)
		return;

	if (ent->client->push_timeout > 0)
		ent->client->push_timeout--;
	/*
	else
	{
		// Really old code that would prevent kill credits from long-term bleedout.
		ent->client->attacker = NULL;
		ent->client->attacker_mod = MOD_BLEEDING;
	}
	*/

	RadioThink(ent);
}
