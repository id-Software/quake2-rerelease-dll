// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_combat.c

#include "g_local.h"

void M_SetEffects (edict_t *self);

/*
ROGUE
clean up heal targets for medic
*/
void cleanupHealTarget (edict_t *ent)
{
	ent->monsterinfo.healer = NULL;
	ent->takedamage = DAMAGE_YES;
	ent->monsterinfo.aiflags &= ~AI_RESURRECTING;
	M_SetEffects (ent);
}
/*
============
CanDamage

Returns true if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
qboolean CanDamage (edict_t *targ, edict_t *inflictor)
{
	vec3_t	dest;
	trace_t	trace;

// bmodels need special checking because their origin is 0,0,0
	if (targ->movetype == MOVETYPE_PUSH)
	{
		VectorAdd (targ->absmin, targ->absmax, dest);
		VectorScale (dest, 0.5, dest);
		trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
		if (trace.fraction == 1.0)
			return true;
		if (trace.ent == targ)
			return true;
		return false;
	}
	
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, targ->s.origin, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] += 15.0;
	dest[1] += 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] += 15.0;
	dest[1] -= 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] -= 15.0;
	dest[1] += 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] -= 15.0;
	dest[1] -= 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;


	return false;
}


/*
============
Killed
============
*/
void Killed (edict_t *targ, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (targ->health < -999)
		targ->health = -999;

	if (targ->monsterinfo.aiflags & AI_MEDIC)
	{
		if (targ->enemy)  // god, I hope so
		{
			cleanupHealTarget (targ->enemy);
		}

		// clean up self
		targ->monsterinfo.aiflags &= ~AI_MEDIC;
		targ->enemy = attacker;
	}
	else
	{
		targ->enemy = attacker;
	}

	if ((targ->svflags & SVF_MONSTER) && (targ->deadflag != DEAD_DEAD))
	{
//		targ->svflags |= SVF_DEADMONSTER;	// now treat as a different content type
		//ROGUE - free up slot for spawned monster if it's spawned
		if (targ->monsterinfo.aiflags & AI_SPAWNED_CARRIER)
		{
			if (targ->monsterinfo.commander && targ->monsterinfo.commander->inuse && 
				!strcmp(targ->monsterinfo.commander->classname, "monster_carrier"))
			{
				targ->monsterinfo.commander->monsterinfo.monster_slots++;
//				if ((g_showlogic) && (g_showlogic->value))
//					gi.dprintf ("g_combat: freeing up carrier slot - %d left\n", targ->monsterinfo.commander->monsterinfo.monster_slots);
			}
		}
		if (targ->monsterinfo.aiflags & AI_SPAWNED_MEDIC_C)
		{
			if (targ->monsterinfo.commander)
			{
				if (targ->monsterinfo.commander->inuse && !strcmp(targ->monsterinfo.commander->classname, "monster_medic_commander"))
				{
					targ->monsterinfo.commander->monsterinfo.monster_slots++;
//					if ((g_showlogic) && (g_showlogic->value))
//						gi.dprintf ("g_combat: freeing up medic slot - %d left\n", targ->monsterinfo.commander->monsterinfo.monster_slots);
				}
//				else
//					if ((g_showlogic) && (g_showlogic->value))
//						gi.dprintf ("my commander is dead!  he's a %s\n", targ->monsterinfo.commander->classname);
			}
//			else if ((g_showlogic) && (g_showlogic->value))
//				gi.dprintf ("My commander is GONE\n");

		}
		if (targ->monsterinfo.aiflags & AI_SPAWNED_WIDOW)
		{
			// need to check this because we can have variable numbers of coop players
			if (targ->monsterinfo.commander && targ->monsterinfo.commander->inuse && 
				!strncmp(targ->monsterinfo.commander->classname, "monster_widow", 13))
			{
				if (targ->monsterinfo.commander->monsterinfo.monster_used > 0)
					targ->monsterinfo.commander->monsterinfo.monster_used--;
//				if ((g_showlogic) && (g_showlogic->value))
//					gi.dprintf ("g_combat: freeing up black widow slot - %d used\n", targ->monsterinfo.commander->monsterinfo.monster_used);
			}
		}
		//rogue
		if ((!(targ->monsterinfo.aiflags & AI_GOOD_GUY)) && (!(targ->monsterinfo.aiflags & AI_DO_NOT_COUNT)))
		{
			level.killed_monsters++;
			if (coop->value && attacker->client)
				attacker->client->resp.score++;
			// medics won't heal monsters that they kill themselves
			// PMM - now they will
//			if (strcmp(attacker->classname, "monster_medic") == 0)
//				targ->owner = attacker;
		}
	}

	if (targ->movetype == MOVETYPE_PUSH || targ->movetype == MOVETYPE_STOP || targ->movetype == MOVETYPE_NONE)
	{	// doors, triggers, etc
		targ->die (targ, inflictor, attacker, damage, point);
		return;
	}

	if ((targ->svflags & SVF_MONSTER) && (targ->deadflag != DEAD_DEAD))
	{
		targ->touch = NULL;
		monster_death_use (targ);
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
//	gi.WriteByte (damage);
	gi.WritePosition (origin);
	gi.WriteDir (normal);
	gi.multicast (origin, MULTICAST_PVS);
}


/*
============
T_Damage

targ		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: targ=monster, inflictor=rocket, attacker=player

dir			direction of the attack
point		point at which the damage is being inflicted
normal		normal vector from that point
damage		amount of damage being inflicted
knockback	force to be applied against targ as a result of the damage

dflags		these flags are used to control how T_Damage works
	DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
	DAMAGE_NO_ARMOR			armor does not protect from this damage
	DAMAGE_ENERGY			damage is from an energy based weapon
	DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
	DAMAGE_BULLET			damage is from a bullet (used for ricochets)
	DAMAGE_NO_PROTECTION	kills godmode, armor, everything
============
*/
static int CheckPowerArmor (edict_t *ent, vec3_t point, vec3_t normal, int damage, int dflags)
{
	gclient_t	*client;
	int			save;
	int			power_armor_type;
	int			index;
	int			damagePerCell;
	int			pa_te_type;
	int			power;
	int			power_used;

	if (!damage)
		return 0;

	client = ent->client;

	if (dflags & (DAMAGE_NO_ARMOR | DAMAGE_NO_POWER_ARMOR))		// PGM
		return 0;					

	if (client)
	{
		power_armor_type = PowerArmorType (ent);
		if (power_armor_type != POWER_ARMOR_NONE)
		{
			index = ITEM_INDEX(FindItem("Cells"));
			power = client->pers.inventory[index];
		}
	}
	else if (ent->svflags & SVF_MONSTER)
	{
		power_armor_type = ent->monsterinfo.power_armor_type;
		power = ent->monsterinfo.power_armor_power;
	}
	else
		return 0;

	if (power_armor_type == POWER_ARMOR_NONE)
		return 0;
	if (!power)
		return 0;

	if (power_armor_type == POWER_ARMOR_SCREEN)
	{
		vec3_t		vec;
		float		dot;
		vec3_t		forward;

		// only works if damage point is in front
		AngleVectors (ent->s.angles, forward, NULL, NULL);
		VectorSubtract (point, ent->s.origin, vec);
		VectorNormalize (vec);
		dot = DotProduct (vec, forward);
		if (dot <= 0.3)
			return 0;

		damagePerCell = 1;
		pa_te_type = TE_SCREEN_SPARKS;
		damage = damage / 3;
	}
	else
	{
		damagePerCell = 2;
		pa_te_type = TE_SHIELD_SPARKS;
		damage = (2 * damage) / 3;
	}

	// etf rifle
	if (dflags & DAMAGE_NO_REG_ARMOR)
		save = (power * damagePerCell) / 2;
	else
		save = power * damagePerCell;

	if (!save)
		return 0;
	if (save > damage)
		save = damage;

	SpawnDamage (pa_te_type, point, normal, save);
	ent->powerarmor_time = level.time + 0.2;

	if (dflags & DAMAGE_NO_REG_ARMOR)
		power_used = (save / damagePerCell) * 2;
	else
		power_used = save / damagePerCell;

	if (client)
		client->pers.inventory[index] -= power_used;
	else
		ent->monsterinfo.power_armor_power -= power_used;
	return save;
}

static int CheckArmor (edict_t *ent, vec3_t point, vec3_t normal, int damage, int te_sparks, int dflags)
{
	gclient_t	*client;
	int			save;
	int			index;
	gitem_t		*armor;

	if (!damage)
		return 0;

	client = ent->client;

	if (!client)
		return 0;

	// ROGUE - added DAMAGE_NO_REG_ARMOR for atf rifle
	if (dflags & (DAMAGE_NO_ARMOR | DAMAGE_NO_REG_ARMOR))
		return 0;

	index = ArmorIndex (ent);
	if (!index)
		return 0;

	armor = GetItemByIndex (index);

	if (dflags & DAMAGE_ENERGY)
		save = ceil(((gitem_armor_t *)armor->info)->energy_protection*damage);
	else
		save = ceil(((gitem_armor_t *)armor->info)->normal_protection*damage);
	if (save >= client->pers.inventory[index])
		save = client->pers.inventory[index];

	if (!save)
		return 0;

	client->pers.inventory[index] -= save;
	SpawnDamage (te_sparks, point, normal, save);

	return save;
}

void M_ReactToDamage (edict_t *targ, edict_t *attacker, edict_t *inflictor)
{
	// pmm
	qboolean new_tesla;

	if (!(attacker->client) && !(attacker->svflags & SVF_MONSTER))
		return;

//=======
//ROGUE
	// logic for tesla - if you are hit by a tesla, and can't see who you should be mad at (attacker)
	// attack the tesla
	// also, target the tesla if it's a "new" tesla
	if ((inflictor) && (!strcmp(inflictor->classname, "tesla")))
	{
		new_tesla = MarkTeslaArea(targ, inflictor);
		if (new_tesla)
			TargetTesla (targ, inflictor);
		return;
		// FIXME - just ignore teslas when you're TARGET_ANGER or MEDIC
/*		if (!(targ->enemy && (targ->monsterinfo.aiflags & (AI_TARGET_ANGER|AI_MEDIC))))
		{
			// FIXME - coop issues?
			if ((!targ->enemy) || (!visible(targ, targ->enemy)))
			{
				gi.dprintf ("can't see player, switching to tesla\n");
				TargetTesla (targ, inflictor);
				return;
			}
			gi.dprintf ("can see player, ignoring tesla\n");
		}
		else if ((g_showlogic) && (g_showlogic->value))
			gi.dprintf ("no enemy, or I'm doing other, more important things, than worrying about a damned tesla!\n");
*/
	}
//ROGUE
//=======

	if (attacker == targ || attacker == targ->enemy)
		return;

	// if we are a good guy monster and our attacker is a player
	// or another good guy, do not get mad at them
	if (targ->monsterinfo.aiflags & AI_GOOD_GUY)
	{
		if (attacker->client || (attacker->monsterinfo.aiflags & AI_GOOD_GUY))
			return;
	}

//PGM
	// if we're currently mad at something a target_anger made us mad at, ignore
	// damage
	if (targ->enemy && targ->monsterinfo.aiflags & AI_TARGET_ANGER)
	{
		float	percentHealth;

		// make sure whatever we were pissed at is still around.
		if(targ->enemy->inuse)
		{
			percentHealth = (float)(targ->health) / (float)(targ->max_health);
			if( targ->enemy->inuse && percentHealth > 0.33)
				return;
		}

		// remove the target anger flag
		targ->monsterinfo.aiflags &= ~AI_TARGET_ANGER;
	}
//PGM

// PMM
// if we're healing someone, do like above and try to stay with them
	if ((targ->enemy) && (targ->monsterinfo.aiflags & AI_MEDIC))
	{
		float	percentHealth;

		percentHealth = (float)(targ->health) / (float)(targ->max_health);
		// ignore it some of the time
		if( targ->enemy->inuse && percentHealth > 0.25)
			return;

		// remove the medic flag
		targ->monsterinfo.aiflags &= ~AI_MEDIC;
		cleanupHealTarget (targ->enemy);
	}
// PMM

	// we now know that we are not both good guys

	// if attacker is a client, get mad at them because he's good and we're not
	if (attacker->client)
	{
		targ->monsterinfo.aiflags &= ~AI_SOUND_TARGET;

		// this can only happen in coop (both new and old enemies are clients)
		// only switch if can't see the current enemy
		if (targ->enemy && targ->enemy->client)
		{
			if (visible(targ, targ->enemy))
			{
				targ->oldenemy = attacker;
				return;
			}
			targ->oldenemy = targ->enemy;
		}
		targ->enemy = attacker;
		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
			FoundTarget (targ); 
		return;
	}

	// it's the same base (walk/swim/fly) type and a different classname and it's not a tank
	// (they spray too much), get mad at them
	// PMM
	// added medics to this 
	// FIXME -
	// this really should be turned into an AI flag marking appropriate monsters as "don't shoot me"
	//   this also leads to the problem of tanks and medics being able to, at will, kill monsters with
	//   no chance of retaliation.  My vote is to make those monsters who are designed as "don't shoot me"
	//   such that they also ignore being shot by monsters as well
	/*
	if (((targ->flags & (FL_FLY|FL_SWIM)) == (attacker->flags & (FL_FLY|FL_SWIM))) &&
		 (strcmp (targ->classname, attacker->classname) != 0) &&
		 (strcmp(attacker->classname, "monster_tank") != 0) &&
		 (strcmp(attacker->classname, "monster_supertank") != 0) &&
		 (strcmp(attacker->classname, "monster_makron") != 0) &&
		 (strcmp(attacker->classname, "monster_jorg") != 0) &&
		 (strcmp(attacker->classname, "monster_carrier") != 0) && 
 		 (strncmp(attacker->classname, "monster_medic", 12) != 0) ) // this should get medics & medic_commanders
	*/
	if (((targ->flags & (FL_FLY|FL_SWIM)) == (attacker->flags & (FL_FLY|FL_SWIM))) &&
		(strcmp (targ->classname, attacker->classname) != 0) &&
		!(attacker->monsterinfo.aiflags & AI_IGNORE_SHOTS) &&
		!(targ->monsterinfo.aiflags & AI_IGNORE_SHOTS) )
	{
		if (targ->enemy && targ->enemy->client)
			targ->oldenemy = targ->enemy;
		targ->enemy = attacker;
		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
			FoundTarget (targ);
	}
	// if they *meant* to shoot us, then shoot back
	else if (attacker->enemy == targ)
	{
		if (targ->enemy && targ->enemy->client)
			targ->oldenemy = targ->enemy;
		targ->enemy = attacker;
		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
			FoundTarget (targ);
	}
	// otherwise get mad at whoever they are mad at (help our buddy) unless it is us!
	else if (attacker->enemy && attacker->enemy != targ)
	{
		if (targ->enemy && targ->enemy->client)
			targ->oldenemy = targ->enemy;
		targ->enemy = attacker->enemy;
		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
			FoundTarget (targ);
	}
}

qboolean CheckTeamDamage (edict_t *targ, edict_t *attacker)
{
		//FIXME make the next line real and uncomment this block
		// if ((ability to damage a teammate == OFF) && (targ's team == attacker's team))
	return false;
}

void T_Damage (edict_t *targ, edict_t *inflictor, edict_t *attacker, vec3_t dir, vec3_t point, vec3_t normal, int damage, int knockback, int dflags, int mod)
{
	gclient_t	*client;
	int			take;
	int			save;
	int			asave;
	int			psave;
	int			te_sparks;
	int			sphere_notified;	// PGM

	if (!targ->takedamage)
		return;

	sphere_notified = false;		// PGM

	// friendly fire avoidance
	// if enabled you can't hurt teammates (but you can hurt yourself)
	// knockback still occurs
	if ((targ != attacker) && ((deathmatch->value && ((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS))) || coop->value))
	{
		if (OnSameTeam (targ, attacker))
		{
			// PMM - nukes kill everyone
			if (((int)(dmflags->value) & DF_NO_FRIENDLY_FIRE) && (mod != MOD_NUKE))
				damage = 0;
			else
				mod |= MOD_FRIENDLY_FIRE;
		}
	}
	meansOfDeath = mod;

//ROGUE
	// allow the deathmatch game to change values
	if (deathmatch->value && gamerules && gamerules->value)
	{
		if(DMGame.ChangeDamage)
			damage = DMGame.ChangeDamage(targ, attacker, damage, mod);
		if(DMGame.ChangeKnockback)
			knockback = DMGame.ChangeKnockback(targ, attacker, knockback, mod);

		if(!damage)
			return;
	}
//ROGUE

	// easy mode takes half damage
	if (skill->value == 0 && deathmatch->value == 0 && targ->client)
	{
		damage *= 0.5;
		if (!damage)
			damage = 1;
	}

	client = targ->client;

	// PMM - defender sphere takes half damage
	if ((client) && (client->owned_sphere) && (client->owned_sphere->spawnflags == 1))
	{
		damage *= 0.5;
		if (!damage)
			damage = 1;
	}

	if (dflags & DAMAGE_BULLET)
		te_sparks = TE_BULLET_SPARKS;
	else
		te_sparks = TE_SPARKS;

	VectorNormalize(dir);

// bonus damage for suprising a monster
	if (!(dflags & DAMAGE_RADIUS) && (targ->svflags & SVF_MONSTER) && (attacker->client) && (!targ->enemy) && (targ->health > 0))
		damage *= 2;

	if (targ->flags & FL_NO_KNOCKBACK)
		knockback = 0;

// figure momentum add
	if (!(dflags & DAMAGE_NO_KNOCKBACK))
	{
		if ((knockback) && (targ->movetype != MOVETYPE_NONE) && (targ->movetype != MOVETYPE_BOUNCE) && (targ->movetype != MOVETYPE_PUSH) && (targ->movetype != MOVETYPE_STOP))
		{
			vec3_t	kvel;
			float	mass;

			if (targ->mass < 50)
				mass = 50;
			else
				mass = targ->mass;

			if (targ->client  && attacker == targ)
				VectorScale (dir, 1600.0 * (float)knockback / mass, kvel);	// the rocket jump hack...
			else
				VectorScale (dir, 500.0 * (float)knockback / mass, kvel);

			VectorAdd (targ->velocity, kvel, targ->velocity);
		}
	}

	take = damage;
	save = 0;

	// check for godmode
	if ( (targ->flags & FL_GODMODE) && !(dflags & DAMAGE_NO_PROTECTION) )
	{
		take = 0;
		save = damage;
		SpawnDamage (te_sparks, point, normal, save);
	}

	// check for invincibility
	if ((client && client->invincible_framenum > level.framenum ) && !(dflags & DAMAGE_NO_PROTECTION))
	{
		if (targ->pain_debounce_time < level.time)
		{
			gi.sound(targ, CHAN_ITEM, gi.soundindex("items/protect4.wav"), 1, ATTN_NORM, 0);
			targ->pain_debounce_time = level.time + 2;
		}
		take = 0;
		save = damage;
	}
	// ROGUE
	// check for monster invincibility	
	if (((targ->svflags & SVF_MONSTER) && targ->monsterinfo.invincible_framenum > level.framenum ) && !(dflags & DAMAGE_NO_PROTECTION))
	{
		if (targ->pain_debounce_time < level.time)
		{
			gi.sound(targ, CHAN_ITEM, gi.soundindex("items/protect4.wav"), 1, ATTN_NORM, 0);
			targ->pain_debounce_time = level.time + 2;
		}
		take = 0;
		save = damage;
	}
	// ROGUE

	psave = CheckPowerArmor (targ, point, normal, take, dflags);
	take -= psave;

	asave = CheckArmor (targ, point, normal, take, te_sparks, dflags);
	take -= asave;

	//treat cheat/powerup savings the same as armor
	asave += save;

	// team damage avoidance
	if (!(dflags & DAMAGE_NO_PROTECTION) && CheckTeamDamage (targ, attacker))
		return;

// ROGUE - this option will do damage both to the armor and person. originally for DPU rounds
	if (dflags & DAMAGE_DESTROY_ARMOR)
	{
		if(!(targ->flags & FL_GODMODE) && !(dflags & DAMAGE_NO_PROTECTION) &&
		   !(client && client->invincible_framenum > level.framenum))
		{
			take = damage;
		}
	}
// ROGUE

// do the damage
	if (take)
	{
//PGM		need more blood for chainfist.
		if(targ->flags & FL_MECHANICAL)
		{
			SpawnDamage ( TE_ELECTRIC_SPARKS, point, normal, take);
		}
		else if ((targ->svflags & SVF_MONSTER) || (client))
		{
			if(mod == MOD_CHAINFIST)
				SpawnDamage (TE_MOREBLOOD, point, normal, 255);
			else
				SpawnDamage (TE_BLOOD, point, normal, take);
		}
		else
			SpawnDamage (te_sparks, point, normal, take);
//PGM

		targ->health = targ->health - take;
			
//PGM - spheres need to know who to shoot at
		if(client && client->owned_sphere)
		{
			sphere_notified = true;
			if(client->owned_sphere->pain)
				client->owned_sphere->pain (client->owned_sphere, attacker, 0, 0);
		}
//PGM

		if (targ->health <= 0)
		{
			if ((targ->svflags & SVF_MONSTER) || (client))
				targ->flags |= FL_NO_KNOCKBACK;
			Killed (targ, inflictor, attacker, take, point);
			return;
		}
	}

//PGM - spheres need to know who to shoot at
	if (!sphere_notified)
	{
		if(client && client->owned_sphere)
		{
			sphere_notified = true;
			if(client->owned_sphere->pain)
				client->owned_sphere->pain (client->owned_sphere, attacker, 0, 0);
		}
	}
//PGM

	if (targ->svflags & SVF_MONSTER)
	{
		M_ReactToDamage (targ, attacker, inflictor);
		// PMM - fixme - if anyone else but the medic ever uses AI_MEDIC, check for it here instead
		// of in the medic's pain function
		if (!(targ->monsterinfo.aiflags & AI_DUCKED) && (take))
		{
			targ->pain (targ, attacker, knockback, take);
			// nightmare mode monsters don't go into pain frames often
			if (skill->value == 3)
				targ->pain_debounce_time = level.time + 5;
		}
	}
	else if (client)
	{
		if (!(targ->flags & FL_GODMODE) && (take))
			targ->pain (targ, attacker, knockback, take);
	}
	else if (take)
	{
		if (targ->pain)
			targ->pain (targ, attacker, knockback, take);
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
		VectorCopy (point, client->damage_from);
	}
}


/*
============
T_RadiusDamage
============
*/
void T_RadiusDamage (edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, int mod)
{
	float	points;
	edict_t	*ent = NULL;
	vec3_t	v;
	vec3_t	dir;

	while ((ent = findradius(ent, inflictor->s.origin, radius)) != NULL)
	{
		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;

		VectorAdd (ent->mins, ent->maxs, v);
		VectorMA (ent->s.origin, 0.5, v, v);
		VectorSubtract (inflictor->s.origin, v, v);
		points = damage - 0.5 * VectorLength (v);
		if (ent == attacker)
			points = points * 0.5;
		if (points > 0)
		{
			if (CanDamage (ent, inflictor))
			{
				VectorSubtract (ent->s.origin, inflictor->s.origin, dir);
				T_Damage (ent, inflictor, attacker, dir, inflictor->s.origin, vec3_origin, (int)points, (int)points, DAMAGE_RADIUS, mod);
			}
		}
	}
}

// **********************
// ROGUE

/*
============
T_RadiusNukeDamage

Like T_RadiusDamage, but ignores walls (skips CanDamage check, among others)
// up to KILLZONE radius, do 10,000 points
// after that, do damage linearly out to KILLZONE2 radius
============
*/

void T_RadiusNukeDamage (edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, int mod)
{
	float	points;
	edict_t	*ent = NULL;
	vec3_t	v;
	vec3_t	dir;
	float	len;
	float	killzone, killzone2;
	trace_t	tr;
	float	dist;

	killzone = radius;
	killzone2 = radius*2.0;

	while ((ent = findradius(ent, inflictor->s.origin, killzone2)) != NULL)
	{
// ignore nobody
		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;
		if (!ent->inuse)
			continue;
		if (!(ent->client || (ent->svflags & SVF_MONSTER) || (ent->svflags & SVF_DAMAGEABLE)))
			continue;

		VectorAdd (ent->mins, ent->maxs, v);
		VectorMA (ent->s.origin, 0.5, v, v);
		VectorSubtract (inflictor->s.origin, v, v);
		len = VectorLength(v);
		if (len <= killzone)
		{
			if (ent->client)
				ent->flags |= FL_NOGIB;
			points = 10000;
		}
		else if (len <= killzone2)
			points = (damage/killzone)*(killzone2 - len);
		else
			points = 0;
//		points = damage - 0.005 * len*len;
//		if (ent == attacker)
//			points = points * 0.5;
//		if ((g_showlogic) && (g_showlogic->value))
//		{
//			if (!(strcmp(ent->classname, "player")))
//				gi.dprintf ("dist = %2.2f doing %6.0f damage to %s\n", len, points, inflictor->teammaster->client->pers.netname);
//			else
//				gi.dprintf ("dist = %2.2f doing %6.0f damage to %s\n", len, points, ent->classname);
//		}
		if (points > 0)
		{
//			if (CanDamage (ent, inflictor))
//			{
				if (ent->client)
					ent->client->nuke_framenum = level.framenum + 20;
				VectorSubtract (ent->s.origin, inflictor->s.origin, dir);
				T_Damage (ent, inflictor, attacker, dir, inflictor->s.origin, vec3_origin, (int)points, (int)points, DAMAGE_RADIUS, mod);
//			}
		}
	}
	ent = g_edicts+1; // skip the worldspawn
	// cycle through players
	while (ent)
	{
		if ((ent->client) && (ent->client->nuke_framenum != level.framenum+20) && (ent->inuse))
		{
			tr = gi.trace (inflictor->s.origin, NULL, NULL, ent->s.origin, inflictor, MASK_SOLID);
			if (tr.fraction == 1.0)
			{
//				if ((g_showlogic) && (g_showlogic->value))
//					gi.dprintf ("Undamaged player in LOS with nuke, flashing!\n");
				ent->client->nuke_framenum = level.framenum + 20;
			}
			else
			{
				dist = realrange (ent, inflictor);
				if (dist < 2048)
					ent->client->nuke_framenum = max(ent->client->nuke_framenum,level.framenum + 15);
				else
					ent->client->nuke_framenum = max(ent->client->nuke_framenum,level.framenum + 10);
			}
			ent++;
		}
		else
			ent = NULL;
	}
}

/*
============
T_RadiusClassDamage

Like T_RadiusDamage, but ignores anything with classname=ignoreClass
============
*/
void T_RadiusClassDamage (edict_t *inflictor, edict_t *attacker, float damage, char *ignoreClass, float radius, int mod)
{
	float	points;
	edict_t	*ent = NULL;
	vec3_t	v;
	vec3_t	dir;

	while ((ent = findradius(ent, inflictor->s.origin, radius)) != NULL)
	{
		if (ent->classname && !strcmp(ent->classname, ignoreClass))
			continue;
		if (!ent->takedamage)
			continue;

		VectorAdd (ent->mins, ent->maxs, v);
		VectorMA (ent->s.origin, 0.5, v, v);
		VectorSubtract (inflictor->s.origin, v, v);
		points = damage - 0.5 * VectorLength (v);
		if (ent == attacker)
			points = points * 0.5;
		if (points > 0)
		{
			if (CanDamage (ent, inflictor))
			{
				VectorSubtract (ent->s.origin, inflictor->s.origin, dir);
				T_Damage (ent, inflictor, attacker, dir, inflictor->s.origin, vec3_origin, (int)points, (int)points, DAMAGE_RADIUS, mod);
			}
		}
	}
}

// ROGUE
// ********************