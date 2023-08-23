//-----------------------------------------------------------------------------
// g_combat.c
//
// $Id: g_combat.c,v 1.27 2002/09/04 11:23:09 ra Exp $
//
//-----------------------------------------------------------------------------
// $Log: g_combat.c,v $
// Revision 1.27  2002/09/04 11:23:09  ra
// Added zcam to TNG and bumped version to 3.0
//
// Revision 1.26  2002/04/01 15:47:51  freud
// Typo fixed for statistics
//
// Revision 1.25  2002/04/01 15:16:06  freud
// Stats code redone, tng_stats now much more smarter. Removed a few global
// variables regarding stats code and added kevlar hits to stats.
//
// Revision 1.24  2002/02/19 10:28:43  freud
// Added to %D hit in the kevlar vest and kevlar helmet, also body for handcannon
// and shotgun.
//
// Revision 1.23  2002/02/18 17:17:20  freud
// Fixed the CTF leaving team bug. Also made the shield more efficient,
// No falling damage.
//
// Revision 1.22  2002/02/18 13:55:35  freud
// Added last damaged players %P
//
// Revision 1.21  2002/02/01 17:49:56  freud
// Heavy changes in stats code. Removed lots of variables and replaced them
// with int arrays of MODs. This cleaned tng_stats.c up a whole lots and
// everything looks well, might need more testing.
//
// Revision 1.20  2002/01/24 02:24:56  deathwatch
// Major update to Stats code (thanks to Freud)
// new cvars:
// stats_afterround - will display the stats after a round ends or map ends
// stats_endmap - if on (1) will display the stats scoreboard when the map ends
//
// Revision 1.19  2002/01/23 14:51:35  ra
// Damn if statements from HELL (ff_afterrounds fix)
//
// Revision 1.18  2002/01/23 14:18:02  ra
// Fixed another ff_afterrounds bobo
//
// Revision 1.17  2001/12/23 21:19:41  deathwatch
// Updated stats with location and average
// cleaned it up a bit as well
//
// Revision 1.16  2001/12/23 16:30:50  ra
// 2.5 ready. New stats from Freud. HC and shotgun gibbing seperated.
//
// Revision 1.15  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.14  2001/08/18 01:28:06  deathwatch
// Fixed some stats stuff, added darkmatch + day_cycle, cleaned up several files, restructured ClientCommand
//
// Revision 1.13  2001/08/17 21:31:37  deathwatch
// Added support for stats
//
// Revision 1.12  2001/08/06 14:38:45  ra
// Adding UVtime for ctf
//
// Revision 1.11  2001/08/06 03:00:48  ra
// Added FF after rounds. Please someone look at the EVIL if statments for me :)
//
// Revision 1.10  2001/07/16 19:02:06  ra
// Fixed compilerwarnings (-g -Wall).  Only one remains.
//
// Revision 1.9  2001/06/22 18:54:38  igor_rock
// fixed the "accuracy" for killing teammates with headshots
//
// Revision 1.8  2001/06/20 07:21:21  igor_rock
// added use_warnings to enable/disable time/frags left msgs
// added use_rewards to enable/disable eimpressive, excellent and accuracy msgs
// change the configfile prefix for modes to "mode_" instead "../mode-" because
// they don't have to be in the q2 dir for doewnload protection (action dir is sufficient)
// and the "-" is bad in filenames because of linux command line parameters start with "-"
//
// Revision 1.7  2001/05/31 16:58:14  igor_rock
// conflicts resolved
//
// Revision 1.6.2.3  2001/05/25 18:59:52  igor_rock
// Added CTF Mode completly :)
// Support for .flg files is still missing, but with "real" CTF maps like
// tq2gtd1 the ctf works fine.
// (I hope that all other modes still work, just tested DM and teamplay)
//
// Revision 1.6.2.2  2001/05/20 18:54:19  igor_rock
// added original ctf code snippets from zoid. lib compilesand runs but
// doesn't function the right way.
// Jsut committing these to have a base to return to if something wents
// awfully wrong.
//
// Revision 1.6.2.1  2001/05/20 15:17:31  igor_rock
// removed the old ctf code completly
//
// Revision 1.6  2001/05/20 12:54:18  igor_rock
// Removed newlines from Centered Messages like "Impressive"
//
// Revision 1.5  2001/05/12 14:05:29  mort
// Hurting someone whilst they are "god" is fixed now
//
// Revision 1.4  2001/05/11 12:21:19  slicerdw
// Commented old Location support ( ADF ) With the ML/ETE Compatible one
//
// Revision 1.2  2001/05/07 20:06:45  igor_rock
// changed sound dir from sound/rock to sound/tng
//
// Revision 1.1.1.1  2001/05/06 17:30:50  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#include "g_local.h"
#include "cgf_sfx_glass.h"

void Add_TeamWound (edict_t * attacker, edict_t * victim, int mod);

/*
  ============
  CanDamage
  
  Returns true if the inflictor can directly damage the target.  Used for
  explosions and melee attacks.
  ============
*/
qboolean CanDamage (edict_t * targ, edict_t * inflictor)
{
	vec3_t dest;
	trace_t trace;

	// bmodels need special checking because their origin is 0,0,0
	//GLASS FX
	if ((targ->movetype == MOVETYPE_PUSH) ||
	((targ->movetype == MOVETYPE_FLYMISSILE)
	&& (0 == Q_stricmp ("func_explosive", targ->classname))))
	//GLASS FX
	{
		VectorAdd (targ->absmin, targ->absmax, dest);
		VectorScale (dest, 0.5, dest);
		PRETRACE ();
		trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest,
			inflictor, MASK_SOLID);
		POSTTRACE ();
		if (trace.fraction == 1.0)
			return true;
		if (trace.ent == targ)
			return true;
		return false;
	}

	PRETRACE ();
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, targ->s.origin, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0) {
		POSTTRACE();
		return true;
	}

	VectorCopy (targ->s.origin, dest);
	dest[0] += 15.0;
	dest[1] += 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0) {
		POSTTRACE();
		return true;
	}

	VectorCopy (targ->s.origin, dest);
	dest[0] += 15.0;
	dest[1] -= 15.0;
	trace =	gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0) {
		POSTTRACE();
		return true;
	}

	VectorCopy (targ->s.origin, dest);
	dest[0] -= 15.0;
	dest[1] += 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0) {
		POSTTRACE();
		return true;
	}

	VectorCopy (targ->s.origin, dest);
	dest[0] -= 15.0;
	dest[1] -= 15.0;
	trace =	gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	POSTTRACE ();
	if (trace.fraction == 1.0)
		return true;

	return false;
}


/*
  ============
  Killed
  ============
*/
void Killed (edict_t * targ, edict_t * inflictor, edict_t * attacker, int damage,
	vec3_t point)
{
	if (targ->health < -999)
		targ->health = -999;

	if (targ->client)
	{
		targ->client->bleeding = 0;
		//targ->client->bleedcount = 0;
		targ->client->bleed_remain = 0;
	}

	targ->enemy = attacker;

	if (targ->movetype == MOVETYPE_PUSH || targ->movetype == MOVETYPE_STOP
	|| targ->movetype == MOVETYPE_NONE)
	{				// doors, triggers, etc
		targ->die (targ, inflictor, attacker, damage, point);
		return;
	}

	targ->die (targ, inflictor, attacker, damage, point);
}


/*
  ================
  SpawnDamage
  ================
*/
void SpawnDamage (int type, vec3_t origin, vec3_t normal, int damage)
{
	if (damage > 255)
		damage = 255;
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (type);
	//      gi.WriteByte (damage);
	gi.WritePosition (origin);
	gi.WriteDir (normal);
	gi.multicast (origin, MULTICAST_PVS);
}


/*
  ============
  T_Damage
  
  targ            entity that is being damaged
  inflictor       entity that is causing the damage
  attacker        entity that caused the inflictor to damage targ
  example: targ=monster, inflictor=rocket, attacker=player
  
  dir                     direction of the attack
  point           point at which the damage is being inflicted
  normal          normal vector from that point
  damage          amount of damage being inflicted
  knockback       force to be applied against targ as a result of the damage
  
  dflags          these flags are used to control how T_Damage works
  DAMAGE_RADIUS                   damage was indirect (from a nearby explosion)
  DAMAGE_NO_ARMOR                 armor does not protect from this damage
  DAMAGE_ENERGY                   damage is from an energy based weapon
  DAMAGE_NO_KNOCKBACK             do not affect velocity, just view angles
  DAMAGE_BULLET                   damage is from a bullet (used for ricochets)
  DAMAGE_NO_PROTECTION    kills godmode, armor, everything
  ============
*/

void BloodSprayThink(edict_t *self)
{
  G_FreeEdict (self);
}

void blood_spray_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if( (other == ent->owner) || other->client )  // Don't stop on players.
		return;
	ent->think = G_FreeEdict;
	ent->nextthink = level.framenum + 1;
}

void spray_blood(edict_t *self, vec3_t start, vec3_t dir, int damage, int mod)
{
	edict_t *blood;
	vec3_t	temp;
	int speed;

	switch (mod)
	{
	case MOD_MK23:
		speed = 1800;
		break;
	case MOD_MP5:
		speed = 1500;
		break;
	case MOD_M4:
		speed = 2400;
		break;
	case MOD_KNIFE:
		speed = 0;
		break;
	case MOD_KNIFE_THROWN:
		speed = 0;
		break;
	case MOD_DUAL:
		speed = 1800;
		break;
	case MOD_SNIPER:
		speed = 4000;
		break;
	default:
		speed = 1800;
		break;
	}

	blood = G_Spawn();
	VectorNormalize2(dir, temp);
	VectorCopy(start, blood->s.origin);
	VectorCopy(start, blood->old_origin);
	VectorCopy(temp, blood->movedir);
	vectoangles(temp, blood->s.angles);
	VectorScale(temp, speed, blood->velocity);
	blood->movetype = MOVETYPE_BLOOD;
	blood->clipmask = MASK_SHOT;
	blood->solid = SOLID_BBOX;
	blood->s.effects |= EF_GIB;
	VectorClear(blood->mins);
	VectorClear(blood->maxs);
	blood->s.modelindex = level.model_null;
	blood->owner = self;
	blood->nextthink = level.framenum + speed * HZ / 1000;  //3.2;
	blood->touch = blood_spray_touch;
	blood->think = BloodSprayThink;
	blood->dmg = damage;
	blood->classname = "blood_spray";

	gi.linkentity(blood);
}

// zucc based on some code in Action Quake
void spray_sniper_blood(edict_t *self, vec3_t start, vec3_t dir)
{
	vec3_t forward;
	int mod = MOD_SNIPER;

	forward[0] = dir[0];
	forward[1] = dir[1];
	forward[2] = dir[2] + 0.03f;

	spray_blood( self, start, forward, 0, mod );

	forward[2] = dir[2] - 0.03f;
	spray_blood( self, start, forward, 0, mod );
	forward[2] = dir[2];

	if (dir[0] && dir[1]) {
		vec3_t diff = { 0.0f, 0.0f, 0.0f };
		if (dir[0] > 0.0f)
		{
			if (dir[1] > 0.0f) {
				diff[0] = -0.03f;
				diff[1] = 0.03f;
			}
			else {
				diff[0] = 0.03f;
				diff[1] = 0.03f;
			}
		}
		else
		{
			if (dir[1] > 0.0f) {
				diff[0] = -0.03f;
				diff[1] = -0.03f;
			}
			else {
				diff[0] = 0.03f;
				diff[1] = -0.03f;
			}
		}

		forward[0] = dir[0] + diff[0];
		forward[1] = dir[1] + diff[1];

		spray_blood( self, start, forward, 0, mod );

		forward[0] = dir[0] - diff[0];
		forward[1] = dir[1] - diff[1];

		spray_blood( self, start, forward, 0, mod );
	}

	spray_blood( self, start, dir, 0, mod );
}

void VerifyHeadShot(vec3_t point, vec3_t dir, float height, vec3_t newpoint)
{
	vec3_t normdir;

	VectorNormalize2(dir, normdir);
	VectorMA(point, height, normdir, newpoint);
}

// zucc adding location hit code
// location hit code based off ideas by pyromage and shockman
#define LEG_DAMAGE (height/2.2) - fabsf(targ->mins[2]) - 3
#define STOMACH_DAMAGE (height/1.8) - fabsf(targ->mins[2])
#define CHEST_DAMAGE (height/1.4) - fabsf(targ->mins[2])

#define HEAD_HEIGHT 12.0f

void
T_Damage (edict_t * targ, edict_t * inflictor, edict_t * attacker, vec3_t dir,
	  vec3_t point, vec3_t normal, int damage, int knockback, int dflags,
	  int mod)
{
	gclient_t *client;
	char buf[256];
	int take, save;
	int asave, psave;
	int te_sparks, do_sparks = 0;
	int damage_type = 0;		// used for MOD later
	int bleeding = 0;		// damage causes bleeding
	int head_success = 0;
	int instant_dam = 1;
	float z_rel;
	int height, friendlyFire = 0, gotArmor = 0;
	float from_top;
	vec_t dist;
	float targ_maxs2;		//FB 6/1/99

	// do this before teamplay check
	if (!targ->takedamage)
		return;

	client = targ->client;
	if (targ != attacker && OnSameTeam( targ, attacker ))
		friendlyFire = 1;

	//FIREBLADE
	if (mod != MOD_TELEFRAG)
	{
		if (lights_camera_action)
			return;

		if (client)
		{
			if (client->uvTime > 0)
				return;
			if (attacker->client && attacker->client->uvTime > 0)
				return;
		}

		// AQ2:TNG - JBravo adding FF after rounds
		if (friendlyFire && DMFLAGS(DF_NO_FRIENDLY_FIRE)) {
			if (!teamplay->value || team_round_going || !ff_afterround->value)
				return;
		}
	}

	// damage reduction for shotgun
	// if far away, reduce it to original action levels
	if (mod == MOD_M3)
	{
		dist = Distance(targ->s.origin, inflictor->s.origin);
		if (dist > 450.0)
			damage = damage - 2;
	}

	targ_maxs2 = targ->maxs[2];
	if (targ_maxs2 == 4)
		targ_maxs2 = CROUCHING_MAXS2;	//FB 6/1/99

	height = fabsf (targ->mins[2]) + targ_maxs2;

	// locational damage code
	// base damage is head shot damage, so all the scaling is downwards
	if (client)
	{

		switch (mod) {
		case MOD_MK23:
		case MOD_DUAL:
			// damage reduction for longer range pistol shots
			dist = Distance( targ->s.origin, inflictor->s.origin );
			if (dist > 1400.0)
				damage = (int)(damage * 1 / 2);
			else if (dist > 600.0)
				damage = (int)(damage * 2 / 3);
			//Fallthrough
		case MOD_MP5:
		case MOD_M4:
		case MOD_SNIPER:
		case MOD_KNIFE:
		case MOD_KNIFE_THROWN:

			z_rel = point[2] - targ->s.origin[2];
			from_top = targ_maxs2 - z_rel;
			if (from_top < 0.0)	//FB 6/1/99
				from_top = 0.0;	//Slightly negative values were being handled wrong
			bleeding = 1;
			instant_dam = 0;

			if (from_top < 2 * HEAD_HEIGHT)
			{
				vec3_t new_point;
				VerifyHeadShot(point, dir, HEAD_HEIGHT, new_point);
				VectorSubtract(new_point, targ->s.origin, new_point);
				//gi.cprintf(attacker, PRINT_HIGH, "z: %d y: %d x: %d\n", (int)(targ_maxs2 - new_point[2]),(int)(new_point[1]) , (int)(new_point[0]) );

				if ((targ_maxs2 - new_point[2]) < HEAD_HEIGHT
					&& (fabsf (new_point[1])) < HEAD_HEIGHT * .8
					&& (fabsf (new_point[0])) < HEAD_HEIGHT * .8)
				{
					head_success = 1;
				}
			}

			if (head_success)
			{
				damage_type = LOC_HDAM;
				if (mod != MOD_KNIFE && mod != MOD_KNIFE_THROWN) //Knife doesnt care about helmet
					gotArmor = INV_AMMO( targ, HELM_NUM );

				if (attacker->client)
				{
					strcpy( attacker->client->last_damaged_players, client->pers.netname );
					Stats_AddHit( attacker, mod, (gotArmor) ? LOC_KVLR_HELMET : LOC_HDAM );

					//AQ2:TNG END
					if (!friendlyFire && !in_warmup)
					{
						attacker->client->resp.streakHS++;
						if (attacker->client->resp.streakHS > attacker->client->resp.streakHSHighest)
							attacker->client->resp.streakHSHighest = attacker->client->resp.streakHS;

						if(attacker->client->resp.streakHS % 3 == 0)
						{
							if (use_rewards->value) {
								Announce_Reward(attacker, ACCURACY);
							}
						}
					}
				}

				if (!gotArmor)
				{
					damage = damage * 1.8 + 1;
					gi.cprintf(targ, PRINT_HIGH, "Head damage\n");
					if (attacker->client)
						gi.cprintf(attacker, PRINT_HIGH, "You hit %s in the head\n", client->pers.netname);

					if (mod != MOD_KNIFE && mod != MOD_KNIFE_THROWN)
						gi.sound(targ, CHAN_VOICE, level.snd_headshot, 1, ATTN_NORM, 0);
				}
				else if (mod == MOD_SNIPER)
				{
					if (attacker->client)
					{
						gi.cprintf(attacker, PRINT_HIGH,
							"%s has a Kevlar Helmet, too bad you have AP rounds...\n",
							client->pers.netname);
						gi.cprintf(targ, PRINT_HIGH,
							"Kevlar Helmet absorbed some of %s's AP sniper round\n",
							attacker->client->pers.netname);
					}
					damage = (int) (damage * 0.325);
					gi.sound(targ, CHAN_VOICE, level.snd_headshot, 1, ATTN_NORM, 0);
				}
				else
				{
					if (attacker->client)
					{
						gi.cprintf( attacker, PRINT_HIGH, "%s has a Kevlar Helmet - AIM FOR THE BODY!\n",
							client->pers.netname );
						gi.cprintf( targ, PRINT_HIGH, "Kevlar Helmet absorbed a part of %s's shot\n",
							attacker->client->pers.netname );
					}
					gi.sound(targ, CHAN_ITEM, level.snd_vesthit, 1, ATTN_NORM, 0);
					damage = (int)(damage / 2);
					bleeding = 0;
					instant_dam = 1;
					stopAP = 1;
					do_sparks = 1;
				}
			}
			else if (z_rel < LEG_DAMAGE)
			{
				damage_type = LOC_LDAM;
				damage = damage * .25;
				if (attacker->client)
				{
					strcpy( attacker->client->last_damaged_players, client->pers.netname );
					Stats_AddHit( attacker, mod, LOC_LDAM );
					gi.cprintf(attacker, PRINT_HIGH, "You hit %s in the legs\n",
						client->pers.netname);
				}

				gi.cprintf(targ, PRINT_HIGH, "Leg damage\n");
				ClientLegDamage(targ);
			}
			else if (z_rel < STOMACH_DAMAGE)
			{
				damage_type = LOC_SDAM;
				damage = damage * .4;
				gi.cprintf(targ, PRINT_HIGH, "Stomach damage\n");
				if (attacker->client)
				{
					strcpy( attacker->client->last_damaged_players, client->pers.netname );
					Stats_AddHit(attacker, mod, LOC_SDAM);
					gi.cprintf(attacker, PRINT_HIGH, "You hit %s in the stomach\n",
						client->pers.netname);
				}
					
				//TempFile bloody gibbing
				if (mod == MOD_SNIPER && sv_gib->value)
					ThrowGib(targ, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
			}
			else		//(z_rel < CHEST_DAMAGE)
			{
				damage_type = LOC_CDAM;
				if (mod != MOD_KNIFE && mod != MOD_KNIFE_THROWN) //Knife doesnt care about kevlar
					gotArmor = INV_AMMO( targ, KEV_NUM );

				if (attacker->client) {
					strcpy( attacker->client->last_damaged_players, client->pers.netname );
					Stats_AddHit(attacker, mod, (gotArmor) ? LOC_KVLR_VEST : LOC_CDAM);
				}

				if (!gotArmor)
				{
					damage = damage * .65;
					gi.cprintf(targ, PRINT_HIGH, "Chest damage\n");
					if (attacker->client)
						gi.cprintf(attacker, PRINT_HIGH, "You hit %s in the chest\n",
							client->pers.netname);

					//TempFile bloody gibbing
					if (mod == MOD_SNIPER && sv_gib->value)
						ThrowGib(targ, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
				}
				else if (mod == MOD_SNIPER)
				{
					if (attacker->client)
					{
						gi.cprintf (attacker, PRINT_HIGH, "%s has a Kevlar Vest, too bad you have AP rounds...\n",
							client->pers.netname);
						gi.cprintf (targ, PRINT_HIGH, "Kevlar Vest absorbed some of %s's AP sniper round\n",
							attacker->client->pers.netname);
					}
					damage = damage * .325;
				}
				else
				{
					if (attacker->client)
					{
						gi.cprintf(attacker, PRINT_HIGH, "%s has a Kevlar Vest - AIM FOR THE HEAD!\n",
							client->pers.netname);
						gi.cprintf(targ, PRINT_HIGH, "Kevlar Vest absorbed most of %s's shot\n",
							attacker->client->pers.netname);
					}
					gi.sound(targ, CHAN_ITEM, level.snd_vesthit, 1, ATTN_NORM, 0);
					damage = (int)(damage / 10);
					bleeding = 0;
					instant_dam = 1;
					stopAP = 1;
					do_sparks = 1;
				}
			}
			break;
		case MOD_M3:
		case MOD_HC:
		case MOD_HELD_GRENADE:
		case MOD_HG_SPLASH:
		case MOD_G_SPLASH:
		case MOD_BREAKINGGLASS:
			//shotgun damage report stuff
			if (client)
				client->took_damage++;

			bleeding = 1;
			instant_dam = 0;
			break;
		default:
			break;
		}
		if(friendlyFire && team_round_going)
		{
			Add_TeamWound(attacker, targ, mod);
		}
    }


	if (damage_type && !instant_dam)	// bullets but not vest hits
	{
		vec3_t temporig;
		VectorMA(point, 20.0f, dir, temporig);
		if (mod != MOD_SNIPER)
			spray_blood(targ, temporig, dir, damage, mod);
		else
			spray_sniper_blood(targ, temporig, dir);
	}

	if (mod == MOD_FALLING && !(targ->flags & FL_GODMODE) )
	{
		if (client && targ->health > 0)
		{
			gi.cprintf(targ, PRINT_HIGH, "Leg damage\n");
			ClientLegDamage(targ);
			//bleeding = 1; for testing
		}
	}


	// friendly fire avoidance
	// if enabled you can't hurt teammates (but you can hurt yourself)
	// knockback still occurs
	if (friendlyFire)
	{
		if (DMFLAGS(DF_NO_FRIENDLY_FIRE) && (!teamplay->value || team_round_going || !ff_afterround->value))
			damage = 0;
		else
			mod |= MOD_FRIENDLY_FIRE;
	}

	meansOfDeath = mod;
	locOfDeath = damage_type;	// location

	if (dflags & DAMAGE_BULLET)
		te_sparks = TE_BULLET_SPARKS;
	else
		te_sparks = TE_SPARKS;

  // bonus damage for suprising a monster
  //      if (!(dflags & DAMAGE_RADIUS) && (targ->svflags & SVF_MONSTER) && (attacker->client) && (!targ->enemy) && (targ->health > 0))
  //              damage *= 2;

	if (targ->flags & FL_NO_KNOCKBACK)
		knockback = 0;

	// figure momentum add
	if (knockback && !(dflags & DAMAGE_NO_KNOCKBACK))
	{
		switch (targ->movetype) {
		case MOVETYPE_NONE:
		case MOVETYPE_BOUNCE:
		case MOVETYPE_PUSH:
		case MOVETYPE_STOP:
			break;
		default:
			if( mod != MOD_FALLING )
			{
				float mass = max( targ->mass, 50 );
				vec3_t flydir = {0.f,0.f,0.f}, kvel = {0.f,0.f,0.f};
				float accel_scale = (client && (attacker == targ)) ? 1600.f : 500.f; // the rocket jump hack...

				VectorNormalize2( dir, flydir );
				flydir[2] += 0.4f;

				VectorScale( flydir, accel_scale * (float) knockback / mass, kvel );

				VectorAdd( targ->velocity, kvel, targ->velocity );

				// Raptor007: Don't consider knockback part of falling damage (instant kick death).
				if( client )
					VectorAdd( client->oldvelocity, kvel, client->oldvelocity );
			}
			break;
		}
	}

	take = damage;
	save = 0;

	// zucc don't need this stuff, but to remove it need to change how damagefeedback works with colors
	if (!(dflags & DAMAGE_NO_PROTECTION))
	{
		// check for godmode
		if ((targ->flags & FL_GODMODE))
		{
			take = 0;
			save = damage;
			SpawnDamage(te_sparks, point, normal, save);
		}
		// check for invincibility
		else if (client && client->invincible_framenum > level.framenum)
		{
			if (targ->pain_debounce_framenum < level.framenum)
			{
				gi.sound(targ, CHAN_ITEM, gi.soundindex("items/protect4.wav"), 1,
					ATTN_NORM, 0);
				targ->pain_debounce_framenum = level.framenum + 2 * HZ;
			}
			take = 0;
			save = damage;
		}
	}

	psave = 0; // CheckPowerArmor( targ, point, normal, take, dflags );
	take -= psave;

	asave = 0; // CheckArmor( targ, point, normal, take, te_sparks, dflags );
	take -= asave;

	//treat cheat/powerup savings the same as armor
	asave += save;

	if (ctf->value)
		CTFCheckHurtCarrier (targ, attacker);

	// do the damage
	if (take)
	{
		// zucc added check for stopAP, if it hit a vest we want sparks
		if (((targ->svflags & SVF_MONSTER) || client) && !do_sparks)
			SpawnDamage(TE_BLOOD, point, normal, take);
		else
			SpawnDamage(te_sparks, point, normal, take);

		// all things that have at least some instantaneous damage, i.e. bruising/falling
		if (instant_dam)
			targ->health = targ->health - take;

		if (targ->health <= 0)
		{
			if (client && attacker->client)
			{
				if (!friendlyFire && !in_warmup) {
					attacker->client->resp.damage_dealt += damage;
					if (mod > 0 && mod < MAX_GUNSTAT) {
						attacker->client->resp.gunstats[mod].damage += damage;
					}
				}
			
				client->attacker = attacker;
				client->attacker_mod = mod;
				client->attacker_loc = damage_type;
			}

			if ((targ->svflags & SVF_MONSTER) || client)
				targ->flags |= FL_NO_KNOCKBACK;
			Killed(targ, inflictor, attacker, take, point);
			return;
		}
	}

	if (client)
	{
		if (!(targ->flags & FL_GODMODE) && take)
			targ->pain(targ, attacker, knockback, take);
	}
	else if (take)
	{
		if (targ->pain)
			targ->pain(targ, attacker, knockback, take);
	}

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if (client)
	{
		client->damage_parmor += psave;
		client->damage_armor += asave;
		client->damage_blood += take;
		client->damage_knockback += knockback;
		//zucc handle adding bleeding here
		if (bleeding)
		{
			vec3_t fwd, right, up, offset;

			client->bleeding += damage * BLEED_TIME;			
			AngleVectors( targ->s.angles, fwd, right, up );
			VectorSubtract( point, targ->s.origin, offset );
			targ->client->bleedloc_offset[0] = DotProduct( offset, fwd );
			targ->client->bleedloc_offset[1] = DotProduct( offset, right );
			targ->client->bleedloc_offset[2] = DotProduct( offset, up );
			
			client->bleeddelay = level.framenum + 2 * HZ;  // 2 seconds
		}
		if (attacker->client)
		{
			if (!friendlyFire && !in_warmup) {
				attacker->client->resp.damage_dealt += damage;
				// All normal weapon damage
				if (mod > 0 && mod < MAX_GUNSTAT) {
					attacker->client->resp.gunstats[mod].damage += damage;
				}
				// Grenade splash, kicks and punch damage
				if (mod > 0 && ((mod == MOD_HG_SPLASH) || (mod == MOD_KICK) || (mod == MOD_PUNCH))) {
					attacker->client->resp.gunstats[mod].damage += damage;
				}
			}

			client->attacker = attacker;
			client->attacker_mod = mod;
			client->attacker_loc = damage_type;
			client->push_timeout = 50;
		}
		VectorCopy(point, client->damage_from);
	}
}


/*
  ============
  T_RadiusDamage
  ============
*/
void
T_RadiusDamage (edict_t * inflictor, edict_t * attacker, float damage,
		edict_t * ignore, float radius, int mod)
{
	float points;
	edict_t *ent = NULL;
	vec3_t v;
	vec3_t dir;

	while ((ent = findradius (ent, inflictor->s.origin, radius)) != NULL)
	{
		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;

		VectorAdd (ent->mins, ent->maxs, v);
		VectorMA (ent->s.origin, 0.5, v, v);
		VectorSubtract (inflictor->s.origin, v, v);
		points = damage - 0.5 * VectorLength (v);
		//zucc reduce damage for crouching, max is 32 when standing
		if (ent->maxs[2] < 20)
		{
			points = points * 0.5;	// hefty reduction in damage
		}
		//if (ent == attacker)
		//points = points * 0.5; 
		if (points > 0)
		{
#ifdef _DEBUG
			if (0 == Q_stricmp (ent->classname, "func_explosive"))
			{
				CGF_SFX_ShootBreakableGlass (ent, inflictor, 0, mod);
			}
			else
#endif
			if (CanDamage (ent, inflictor))
			{
				VectorSubtract (ent->s.origin, inflictor->s.origin, dir);
				// zucc scaled up knockback(kick) of grenades
				T_Damage (ent, inflictor, attacker, dir, ent->s.origin,
				vec3_origin, (int) (points * .75),
				(int) (points * .75), DAMAGE_RADIUS, mod);
			}
		}
	}
}
