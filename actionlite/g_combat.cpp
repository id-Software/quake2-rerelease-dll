// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_combat.c

#include "g_local.h"

/*
============
CanDamage

Returns true if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
bool CanDamage(edict_t *targ, edict_t *inflictor)
{
	vec3_t	dest;
	trace_t trace;
	
	// bmodels need special checking because their origin is 0,0,0
	vec3_t inflictor_center;
	
	if (inflictor->linked)
		inflictor_center = (inflictor->absmin + inflictor->absmax) * 0.5f;
	else
		inflictor_center = inflictor->s.origin;
	
	if (targ->solid == SOLID_BSP)
	{
		dest = closest_point_to_box(inflictor_center, targ->absmin, targ->absmax);

		trace = gi.traceline(inflictor_center, dest, inflictor, MASK_SOLID);
		if (trace.fraction == 1.0f)
			return true;
	}

	vec3_t targ_center;
	
	if (targ->linked)
		targ_center = (targ->absmin + targ->absmax) * 0.5f;
	else
		targ_center = targ->s.origin;

	trace = gi.traceline(inflictor_center, targ_center, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0f)
		return true;

	dest = targ_center;
	dest[0] += 15.0f;
	dest[1] += 15.0f;
	trace = gi.traceline(inflictor_center, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0f)
		return true;

	dest = targ_center;
	dest[0] += 15.0f;
	dest[1] -= 15.0f;
	trace = gi.traceline(inflictor_center, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0f)
		return true;

	dest = targ_center;
	dest[0] -= 15.0f;
	dest[1] += 15.0f;
	trace = gi.traceline(inflictor_center, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0f)
		return true;

	dest = targ_center;
	dest[0] -= 15.0f;
	dest[1] -= 15.0f;
	trace = gi.traceline(inflictor_center, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0f)
		return true;

	return false;
}

/*
============
Killed
============
*/
void Killed(edict_t *targ, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, mod_t mod)
{
	if (targ->health < -999)
		targ->health = -999;

	// [Paril-KEX]
	if ((targ->svflags & SVF_MONSTER) && targ->monsterinfo.aiflags & AI_MEDIC)
	{
		if (targ->enemy && targ->enemy->inuse && (targ->enemy->svflags & SVF_MONSTER)) // god, I hope so
		{
			cleanupHealTarget(targ->enemy);
		}

		// clean up self
		targ->monsterinfo.aiflags &= ~AI_MEDIC;
	}

	targ->enemy = attacker;
	targ->lastMOD = mod;

	// [Paril-KEX] monsters call die in their damage handler
	if (targ->svflags & SVF_MONSTER)
		return;

	targ->die(targ, inflictor, attacker, damage, point, mod);

	if (targ->monsterinfo.setskin)
		targ->monsterinfo.setskin(targ);
}

/*
================
SpawnDamage
================
*/
void SpawnDamage(int type, const vec3_t &origin, const vec3_t &normal, int damage)
{
	if (damage > 255)
		damage = 255;
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(type);
	//	gi.WriteByte (damage);
	gi.WritePosition(origin);
	gi.WriteDir(normal);
	gi.multicast(origin, MULTICAST_PVS, false);
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
static int CheckPowerArmor(edict_t *ent, const vec3_t &point, const vec3_t &normal, int damage, damageflags_t dflags)
{
	gclient_t *client;
	int		   save;
	item_id_t  power_armor_type;
	int		   damagePerCell;
	int		   pa_te_type;
	int		*power;
	int		   power_used;

	if (ent->health <= 0)
		return 0;

	if (!damage)
		return 0;

	client = ent->client;

	if (dflags & (DAMAGE_NO_ARMOR | DAMAGE_NO_POWER_ARMOR)) // PGM
		return 0;

	if (client)
	{
		power_armor_type = PowerArmorType(ent);
		power = &client->pers.inventory[IT_AMMO_CELLS];
	}
	else if (ent->svflags & SVF_MONSTER)
	{
		power_armor_type = ent->monsterinfo.power_armor_type;
		power = &ent->monsterinfo.power_armor_power;
	}
	else
		return 0;

	if (power_armor_type == IT_NULL)
		return 0;
	if (!*power)
		return 0;

	if (power_armor_type == IT_ITEM_POWER_SCREEN)
	{
		vec3_t vec;
		float  dot;
		vec3_t forward;

		// only works if damage point is in front
		AngleVectors(ent->s.angles, forward, nullptr, nullptr);
		vec = point - ent->s.origin;
		vec.normalize();
		dot = vec.dot(forward);
		if (dot <= 0.3f)
			return 0;

		damagePerCell = 1;
		pa_te_type = TE_SCREEN_SPARKS;
		damage = damage / 3;
	}
	else
	{
		if (ctf->integer)
			damagePerCell = 1; // power armor is weaker in CTF
		else
			damagePerCell = 2;
		pa_te_type = TE_SCREEN_SPARKS;
		damage = (2 * damage) / 3;
	}

	// Paril: fix small amounts of damage not
	// being absorbed
	damage = max(1, damage);

	save = *power * damagePerCell;

	if (!save)
		return 0;

	// [Paril-KEX] energy damage should do more to power armor, not ETF Rifle shots.
	if (dflags & DAMAGE_ENERGY)
		save = max(1, save / 2);

	if (save > damage)
		save = damage;

	// [Paril-KEX] energy damage should do more to power armor, not ETF Rifle shots.
	if (dflags & DAMAGE_ENERGY)
		power_used = (save / damagePerCell) * 2;
	else
		power_used = save / damagePerCell;

	power_used = max(1, power_used);

	SpawnDamage(pa_te_type, point, normal, save);
	ent->powerarmor_time = level.time + 200_ms;

	// Paril: adjustment so that power armor
	// always uses damagePerCell even if it does
	// only a single point of damage
	*power = max(0, *power - max(damagePerCell, power_used));

	// check power armor turn-off states
	if (ent->client)
		G_CheckPowerArmor(ent);
	else if (!*power)
	{
		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/mon_power2.wav"), 1.f, ATTN_NORM, 0.f);

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_POWER_SPLASH);
		gi.WriteEntity(ent);
		gi.WriteByte((power_armor_type == IT_ITEM_POWER_SCREEN) ? 1 : 0);
		gi.multicast(ent->s.origin, MULTICAST_PHS, false);
	}

	return save;
}

static int CheckArmor(edict_t *ent, const vec3_t &point, const vec3_t &normal, int damage, int te_sparks,
					  damageflags_t dflags)
{
	gclient_t *client;
	int		   save;
	item_id_t  index;
	gitem_t	*armor;
	int *power;

	if (!damage)
		return 0;

	// ROGUE
	if (dflags & (DAMAGE_NO_ARMOR | DAMAGE_NO_REG_ARMOR))
		// ROGUE
		return 0;

	client = ent->client;
	index = ArmorIndex(ent);

	if (!index)
		return 0;

	armor = GetItemByIndex(index);

	if (dflags & DAMAGE_ENERGY)
		save = (int) ceilf(armor->armor_info->energy_protection * damage);
	else
		save = (int) ceilf(armor->armor_info->normal_protection * damage);

	if (client)
		power = &client->pers.inventory[index];
	else
		power = &ent->monsterinfo.armor_power;

	if (save >= *power)
		save = *power;

	if (!save)
		return 0;

	*power -= save;

	if (!client && !ent->monsterinfo.armor_power)
		ent->monsterinfo.armor_type = IT_NULL;

	SpawnDamage(te_sparks, point, normal, save);

	return save;
}

void M_ReactToDamage(edict_t *targ, edict_t *attacker, edict_t *inflictor)
{
	// pmm
	bool new_tesla;

	if (!(attacker->client) && !(attacker->svflags & SVF_MONSTER))
		return;

	//=======
	// ROGUE
	// logic for tesla - if you are hit by a tesla, and can't see who you should be mad at (attacker)
	// attack the tesla
	// also, target the tesla if it's a "new" tesla
	if ((inflictor) && (!strcmp(inflictor->classname, "tesla_mine")))
	{
		new_tesla = MarkTeslaArea(targ, inflictor);
		if ((new_tesla || brandom()) && (!targ->enemy || !targ->enemy->classname || strcmp(targ->enemy->classname, "tesla_mine")))
			TargetTesla(targ, inflictor);
		return;
	}
	// ROGUE
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

	// PGM
	//  if we're currently mad at something a target_anger made us mad at, ignore
	//  damage
	if (targ->enemy && targ->monsterinfo.aiflags & AI_TARGET_ANGER)
	{
		float percentHealth;

		// make sure whatever we were pissed at is still around.
		if (targ->enemy->inuse)
		{
			percentHealth = (float) (targ->health) / (float) (targ->max_health);
			if (targ->enemy->inuse && percentHealth > 0.33f)
				return;
		}

		// remove the target anger flag
		targ->monsterinfo.aiflags &= ~AI_TARGET_ANGER;
	}
	// PGM

	// we recently switched from reacting to damage, don't do it
	if (targ->monsterinfo.react_to_damage_time > level.time)
		return;

	// PMM
	// if we're healing someone, do like above and try to stay with them
	if ((targ->enemy) && (targ->monsterinfo.aiflags & AI_MEDIC))
	{
		float percentHealth;

		percentHealth = (float) (targ->health) / (float) (targ->max_health);
		// ignore it some of the time
		if (targ->enemy->inuse && percentHealth > 0.25f)
			return;

		// remove the medic flag
		cleanupHealTarget(targ->enemy);
		targ->monsterinfo.aiflags &= ~AI_MEDIC;
	}
	// PMM

	// we now know that we are not both good guys
	targ->monsterinfo.react_to_damage_time = level.time + random_time(3_sec, 5_sec);

	// if attacker is a client, get mad at them because he's good and we're not
	if (attacker->client)
	{
		targ->monsterinfo.aiflags &= ~AI_SOUND_TARGET;

		// this can only happen in coop (both new and old enemies are clients)
		// only switch if can't see the current enemy
		if (targ->enemy != attacker)
		{
			if (targ->enemy && targ->enemy->client)
			{
				if (visible(targ, targ->enemy))
				{
					targ->oldenemy = attacker;
					return;
				}
				targ->oldenemy = targ->enemy;
			}

			// [Paril-KEX]
			if ((targ->svflags & SVF_MONSTER) && targ->monsterinfo.aiflags & AI_MEDIC)
			{
				if (targ->enemy && targ->enemy->inuse && (targ->enemy->svflags & SVF_MONSTER)) // god, I hope so
				{
					cleanupHealTarget(targ->enemy);
				}

				// clean up self
				targ->monsterinfo.aiflags &= ~AI_MEDIC;
			}

			targ->enemy = attacker;
			if (!(targ->monsterinfo.aiflags & AI_DUCKED))
				FoundTarget(targ);
		}
		return;
	}

	if (attacker->enemy == targ // if they *meant* to shoot us, then shoot back
		// it's the same base (walk/swim/fly) type and both don't ignore shots,
		// get mad at them
		|| (((targ->flags & (FL_FLY | FL_SWIM)) == (attacker->flags & (FL_FLY | FL_SWIM))) &&
		(strcmp(targ->classname, attacker->classname) != 0) && !(attacker->monsterinfo.aiflags & AI_IGNORE_SHOTS) &&
		!(targ->monsterinfo.aiflags & AI_IGNORE_SHOTS)))
	{
		if (targ->enemy != attacker)
		{
			// [Paril-KEX]
			if ((targ->svflags & SVF_MONSTER) && targ->monsterinfo.aiflags & AI_MEDIC)
			{
				if (targ->enemy && targ->enemy->inuse && (targ->enemy->svflags & SVF_MONSTER)) // god, I hope so
				{
					cleanupHealTarget(targ->enemy);
				}

				// clean up self
				targ->monsterinfo.aiflags &= ~AI_MEDIC;
			}

			if (targ->enemy && targ->enemy->client)
				targ->oldenemy = targ->enemy;
			targ->enemy = attacker;
			if (!(targ->monsterinfo.aiflags & AI_DUCKED))
				FoundTarget(targ);
		}
	}
	// otherwise get mad at whoever they are mad at (help our buddy) unless it is us!
	else if (attacker->enemy && attacker->enemy != targ && targ->enemy != attacker->enemy)
	{
		if (targ->enemy != attacker->enemy)
		{
			// [Paril-KEX]
			if ((targ->svflags & SVF_MONSTER) && targ->monsterinfo.aiflags & AI_MEDIC)
			{
				if (targ->enemy && targ->enemy->inuse && (targ->enemy->svflags & SVF_MONSTER)) // god, I hope so
				{
					cleanupHealTarget(targ->enemy);
				}

				// clean up self
				targ->monsterinfo.aiflags &= ~AI_MEDIC;
			}

			if (targ->enemy && targ->enemy->client)
				targ->oldenemy = targ->enemy;
			targ->enemy = attacker->enemy;
			if (!(targ->monsterinfo.aiflags & AI_DUCKED))
				FoundTarget(targ);
		}
	}
}

// check if the two given entities are on the same team
bool OnSameTeam(edict_t *ent1, edict_t *ent2)
{
	// monsters are never on our team atm
	if (!ent1->client || !ent2->client)
		return false;
	// we're never on our own team
	else if (ent1 == ent2)
		return false;

	// [Paril-KEX] coop 'team' support
	if (coop->integer)
		return ent1->client && ent2->client;
	// ZOID
	else if (G_TeamplayEnabled() && ent1->client && ent2->client)
	{
		if (ent1->client->resp.ctf_team == ent2->client->resp.ctf_team)
			return true;
	}
	// ZOID

	return false;
}

// check if the two entities are on a team and that
// they wouldn't damage each other
bool CheckTeamDamage(edict_t *targ, edict_t *attacker)
{
	// always damage teammates if friendly fire is enabled
	if (g_friendly_fire->integer)
		return false;

	return OnSameTeam(targ, attacker);
}

void T_Damage(edict_t *targ, edict_t *inflictor, edict_t *attacker, const vec3_t &dir, const vec3_t &point,
			  const vec3_t &normal, int damage, int knockback, damageflags_t dflags, mod_t mod)
{
	gclient_t *client;
	int		   take;
	int		   save;
	int		   asave;
	int		   psave;
	int		   te_sparks;
	bool	   sphere_notified; // PGM

	if (!targ->takedamage)
		return;

	if (g_instagib->integer && attacker->client && targ->client)
	{
		// [Kex] always kill no matter what on instagib
		damage = 9999;
	}

	sphere_notified = false; // PGM

	// friendly fire avoidance
	// if enabled you can't hurt teammates (but you can hurt yourself)
	// knockback still occurs
	if ((targ != attacker) && !(dflags & DAMAGE_NO_PROTECTION))
	{
		// mark as friendly fire
		if (OnSameTeam(targ, attacker))
		{
			mod.friendly_fire = true;

			// if we're not a nuke & friendly fire is disabled, just kill the damage
			if (!g_friendly_fire->integer && (mod.id != MOD_NUKE))
				damage = 0;
		}
	}

	// ROGUE
	//  allow the deathmatch game to change values
	if (deathmatch->integer && gamerules->integer)
	{
		if (DMGame.ChangeDamage)
			damage = DMGame.ChangeDamage(targ, attacker, damage, mod);
		if (DMGame.ChangeKnockback)
			knockback = DMGame.ChangeKnockback(targ, attacker, knockback, mod);

		if (!damage)
			return;
	}
	// ROGUE

	// easy mode takes half damage
	if (skill->integer == 0 && deathmatch->integer == 0 && targ->client && damage)
	{
		damage /= 2;
		if (!damage)
			damage = 1;
	}

	if ( ( targ->svflags & SVF_MONSTER ) != 0 ) {
		damage *= ai_damage_scale->integer;
	} else {
		damage *= g_damage_scale->integer;
	} // mal: just for debugging...

	client = targ->client;

	// PMM - defender sphere takes half damage
	if (damage && (client) && (client->owned_sphere) && (client->owned_sphere->spawnflags == SPHERE_DEFENDER))
	{
		damage /= 2;
		if (!damage)
			damage = 1;
	}

	if (dflags & DAMAGE_BULLET)
		te_sparks = TE_BULLET_SPARKS;
	else
		te_sparks = TE_SPARKS;

	// bonus damage for surprising a monster
	if (!(dflags & DAMAGE_RADIUS) && (targ->svflags & SVF_MONSTER) && (attacker->client) &&
		(!targ->enemy || targ->monsterinfo.surprise_time == level.time) && (targ->health > 0))
	{
		damage *= 2;
		targ->monsterinfo.surprise_time = level.time;
	}

	// ZOID
	// strength tech
	damage = CTFApplyStrength(attacker, damage);
	// ZOID

	if ((targ->flags & FL_NO_KNOCKBACK) ||
		((targ->flags & FL_ALIVE_KNOCKBACK_ONLY) && (!targ->deadflag || targ->dead_time != level.time)))
		knockback = 0;

	// figure momentum add
	if (!(dflags & DAMAGE_NO_KNOCKBACK))
	{
		if ((knockback) && (targ->movetype != MOVETYPE_NONE) && (targ->movetype != MOVETYPE_BOUNCE) &&
			(targ->movetype != MOVETYPE_PUSH) && (targ->movetype != MOVETYPE_STOP))
		{
			vec3_t normalized = dir.normalized();
			vec3_t kvel;
			float  mass;

			if (targ->mass < 50)
				mass = 50;
			else
				mass = (float) targ->mass;

			if (targ->client && attacker == targ)
				kvel = normalized * (1600.0f * knockback / mass); // the rocket jump hack...
			else
				kvel = normalized * (500.0f * knockback / mass);

			targ->velocity += kvel;
		}
	}

	take = damage;
	save = 0;

	// check for godmode
	if ((targ->flags & FL_GODMODE) && !(dflags & DAMAGE_NO_PROTECTION))
	{
		take = 0;
		save = damage;
		SpawnDamage(te_sparks, point, normal, save);
	}

	// check for invincibility
	// ROGUE
	if (!(dflags & DAMAGE_NO_PROTECTION) &&
		(((client && client->invincible_time > level.time)) ||
		 ((targ->svflags & SVF_MONSTER) && targ->monsterinfo.invincible_time > level.time)))
	// ROGUE
	{
		if (targ->pain_debounce_time < level.time)
		{
			gi.sound(targ, CHAN_ITEM, gi.soundindex("items/protect4.wav"), 1, ATTN_NORM, 0);
			targ->pain_debounce_time = level.time + 2_sec;
		}
		take = 0;
		save = damage;
	}

	// ZOID
	// team armor protect
	if (G_TeamplayEnabled() && targ->client && attacker->client &&
		targ->client->resp.ctf_team == attacker->client->resp.ctf_team && targ != attacker &&
		g_teamplay_armor_protect->integer)
	{
		psave = asave = 0;
	}
	else
	{
		// ZOID
		psave = CheckPowerArmor(targ, point, normal, take, dflags);
		take -= psave;

		asave = CheckArmor(targ, point, normal, take, te_sparks, dflags);
		take -= asave;
	}

	// treat cheat/powerup savings the same as armor
	asave += save;

	// ZOID
	// resistance tech
	take = CTFApplyResistance(targ, take);
	// ZOID

	// ZOID
	CTFCheckHurtCarrier(targ, attacker);
	// ZOID

	// ROGUE - this option will do damage both to the armor and person. originally for DPU rounds
	if (dflags & DAMAGE_DESTROY_ARMOR)
	{
		if (!(targ->flags & FL_GODMODE) && !(dflags & DAMAGE_NO_PROTECTION) &&
			!(client && client->invincible_time > level.time))
		{
			take = damage;
		}
	}
	// ROGUE

	// [Paril-KEX] player hit markers
	if (targ != attacker && attacker->client && targ->health > 0 && !((targ->svflags & SVF_DEADMONSTER) || (targ->flags & FL_NO_DAMAGE_EFFECTS)) && mod.id != MOD_TARGET_LASER)
		attacker->client->ps.stats[STAT_HIT_MARKER] += take + psave + asave;

	// do the damage
	if (take)
	{
		if (!(targ->flags & FL_NO_DAMAGE_EFFECTS))
		{
			// ROGUE
			if (targ->flags & FL_MECHANICAL)
				SpawnDamage(TE_ELECTRIC_SPARKS, point, normal, take);
			// ROGUE
			else if ((targ->svflags & SVF_MONSTER) || (client))
			{
				// XATRIX
				if (strcmp(targ->classname, "monster_gekk") == 0)
					SpawnDamage(TE_GREENBLOOD, point, normal, take);
				// XATRIX
				// ROGUE
				else if (mod.id == MOD_CHAINFIST)
					SpawnDamage(TE_MOREBLOOD, point, normal, 255);
				// ROGUE
				else
					SpawnDamage(TE_BLOOD, point, normal, take);
			}
			else
				SpawnDamage(te_sparks, point, normal, take);
		}

		if (!CTFMatchSetup())
			targ->health = targ->health - take;

		if ((targ->flags & FL_IMMORTAL) && targ->health <= 0)
			targ->health = 1;

		// PGM - spheres need to know who to shoot at
		if (client && client->owned_sphere)
		{
			sphere_notified = true;
			if (client->owned_sphere->pain)
				client->owned_sphere->pain(client->owned_sphere, attacker, 0, 0, mod);
		}
		// PGM

		if (targ->health <= 0)
		{
			if ((targ->svflags & SVF_MONSTER) || (client))
			{
				targ->flags |= FL_ALIVE_KNOCKBACK_ONLY;
				targ->dead_time = level.time;
			}
			targ->monsterinfo.damage_blood += take;
			targ->monsterinfo.damage_attacker = attacker;
			targ->monsterinfo.damage_inflictor = inflictor;
			targ->monsterinfo.damage_from = point;
			targ->monsterinfo.damage_mod = mod;
			targ->monsterinfo.damage_knockback += knockback;
			Killed(targ, inflictor, attacker, take, point, mod);
			return;
		}
	}

	// PGM - spheres need to know who to shoot at
	if (!sphere_notified)
	{
		if (client && client->owned_sphere)
		{
			sphere_notified = true;
			if (client->owned_sphere->pain)
				client->owned_sphere->pain(client->owned_sphere, attacker, 0, 0, mod);
		}
	}
	// PGM

	if ( targ->client ) {
		targ->client->last_attacker_time = level.time;
	}

	if (targ->svflags & SVF_MONSTER)
	{
		if (damage > 0)
		{
			M_ReactToDamage(targ, attacker, inflictor);

			targ->monsterinfo.damage_attacker = attacker;
			targ->monsterinfo.damage_inflictor = inflictor;
			targ->monsterinfo.damage_blood += take;
			targ->monsterinfo.damage_from = point;
			targ->monsterinfo.damage_mod = mod;
			targ->monsterinfo.damage_knockback += knockback;
		}

		if (targ->monsterinfo.setskin)
			targ->monsterinfo.setskin(targ);
	}
	else if (take && targ->pain)
		targ->pain(targ, attacker, (float) knockback, take, mod);

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if (client)
	{
		client->damage_parmor += psave;
		client->damage_armor += asave;
		client->damage_blood += take;
		client->damage_knockback += knockback;
		client->damage_from = point;
		client->last_damage_time = level.time + COOP_DAMAGE_RESPAWN_TIME;

		if (!(dflags & DAMAGE_NO_INDICATOR) && inflictor != world && attacker != world && (take || psave || asave))
		{
			damage_indicator_t *indicator = nullptr;
			size_t i;

			for (i = 0; i < client->num_damage_indicators; i++)
			{
				if ((point - client->damage_indicators[i].from).length() < 32.f)
				{
					indicator = &client->damage_indicators[i];
					break;
				}
			}

			if (!indicator && i != MAX_DAMAGE_INDICATORS)
			{
				indicator = &client->damage_indicators[i];
				// for projectile direct hits, use the attacker; otherwise
				// use the inflictor (rocket splash should point to the rocket)
				indicator->from = (dflags & DAMAGE_RADIUS) ? inflictor->s.origin : attacker->s.origin;
				indicator->health = indicator->armor = indicator->power = 0;
				client->num_damage_indicators++;
			}

			if (indicator)
			{
				indicator->health += take;
				indicator->power += psave;
				indicator->armor += asave;
			}
		}
	}
}

/*
============
T_RadiusDamage
============
*/
void T_RadiusDamage(edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, damageflags_t dflags, mod_t mod)
{
	float	 points;
	edict_t *ent = nullptr;
	vec3_t	 v;
	vec3_t	 dir;
	vec3_t   inflictor_center;
	
	if (inflictor->linked)
		inflictor_center = (inflictor->absmax + inflictor->absmin) * 0.5f;
	else
		inflictor_center = inflictor->s.origin;

	while ((ent = findradius(ent, inflictor_center, radius)) != nullptr)
	{
		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;

		if (ent->solid == SOLID_BSP && ent->linked)
			v = closest_point_to_box(inflictor_center, ent->absmin, ent->absmax);
		else
		{
			v = ent->mins + ent->maxs;
			v = ent->s.origin + (v * 0.5f);
		}
		v = inflictor_center - v;
		points = damage - 0.5f * v.length();
		if (ent == attacker)
			points = points * 0.5f;
		if (points > 0)
		{
			if (CanDamage(ent, inflictor))
			{
				dir = (ent->s.origin - inflictor_center).normalized();
				// [Paril-KEX] use closest point on bbox to explosion position
				// to spawn damage effect

				T_Damage(ent, inflictor, attacker, dir, closest_point_to_box(inflictor_center, ent->absmin, ent->absmax), dir, (int) points, (int) points,
						 dflags | DAMAGE_RADIUS, mod);
			}
		}
	}
}
