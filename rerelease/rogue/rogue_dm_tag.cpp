// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// dm_tag
// pmack
// june 1998

#include "../g_local.h"

void SP_dm_tag_token(edict_t *self);

// ***********************
// Tag Specific Stuff
// ***********************

edict_t *tag_token;
edict_t *tag_owner;
int		 tag_count;

//=================
//=================
void Tag_PlayerDeath(edict_t *targ, edict_t *inflictor, edict_t *attacker)
{
	if (tag_token && targ && (targ == tag_owner))
	{
		Tag_DropToken(targ, GetItemByIndex(IT_ITEM_TAG_TOKEN));
		tag_owner = nullptr;
		tag_count = 0;
	}
}

//=================
//=================
void Tag_KillItBonus(edict_t *self)
{
	edict_t *armor;

	// if the player is hurt, boost them up to max.
	if (self->health < self->max_health)
	{
		self->health += 200;
		if (self->health > self->max_health)
			self->health = self->max_health;
	}

	// give the player a body armor
	armor = G_Spawn();
	armor->spawnflags |= SPAWNFLAG_ITEM_DROPPED;
	armor->item = GetItemByIndex(IT_ARMOR_BODY);
	Touch_Item(armor, self, null_trace, true);
	if (armor->inuse)
		G_FreeEdict(armor);
}

//=================
//=================
void Tag_PlayerDisconnect(edict_t *self)
{
	if (tag_token && self && (self == tag_owner))
	{
		Tag_DropToken(self, GetItemByIndex(IT_ITEM_TAG_TOKEN));
		tag_owner = nullptr;
		tag_count = 0;
	}
}

//=================
//=================
void Tag_Score(edict_t *attacker, edict_t *victim, int scoreChange, const mod_t &mod)
{
	gitem_t *quad;

	if (tag_token && tag_owner)
	{
		// owner killed somone else
		if ((scoreChange > 0) && tag_owner == attacker)
		{
			scoreChange = 3;
			tag_count++;
			if (tag_count == 5)
			{
				quad = GetItemByIndex(IT_ITEM_QUAD);
				attacker->client->pers.inventory[IT_ITEM_QUAD]++;
				quad->use(attacker, quad);
				tag_count = 0;
			}
		}
		// owner got killed. 5 points and switch owners
		else if (tag_owner == victim && tag_owner != attacker)
		{
			scoreChange = 5;
			if ((mod.id == MOD_HUNTER_SPHERE) || (mod.id == MOD_DOPPLE_EXPLODE) ||
				(mod.id == MOD_DOPPLE_VENGEANCE) || (mod.id == MOD_DOPPLE_HUNTER) ||
				(attacker->health <= 0))
			{
				Tag_DropToken(tag_owner, GetItemByIndex(IT_ITEM_TAG_TOKEN));
				tag_owner = nullptr;
				tag_count = 0;
			}
			else
			{
				Tag_KillItBonus(attacker);
				tag_owner = attacker;
				tag_count = 0;
			}
		}
	}

	attacker->client->resp.score += scoreChange;
}

//=================
//=================
bool Tag_PickupToken(edict_t *ent, edict_t *other)
{
	if (gamerules->integer != RDM_TAG)
	{
		return false;
	}

	// sanity checking is good.
	if (tag_token != ent)
		tag_token = ent;

	other->client->pers.inventory[ent->item->id]++;

	tag_owner = other;
	tag_count = 0;

	Tag_KillItBonus(other);

	return true;
}

//=================
//=================
THINK(Tag_Respawn) (edict_t *ent) -> void
{
	edict_t *spot;

	spot = SelectDeathmatchSpawnPoint(true, false, true).spot;
	if (spot == nullptr)
	{
		ent->nextthink = level.time + 1_sec;
		return;
	}

	ent->s.origin = spot->s.origin;
	gi.linkentity(ent);
}

//=================
//=================
THINK(Tag_MakeTouchable) (edict_t *ent) -> void
{
	ent->touch = Touch_Item;

	tag_token->think = Tag_Respawn;

	// check here to see if it's in lava or slime. if so, do a respawn sooner
	if (gi.pointcontents(ent->s.origin) & (CONTENTS_LAVA | CONTENTS_SLIME))
		tag_token->nextthink = level.time + 3_sec;
	else
		tag_token->nextthink = level.time + 30_sec;
}

//=================
//=================
void Tag_DropToken(edict_t *ent, gitem_t *item)
{
	trace_t trace;
	vec3_t	forward, right;
	vec3_t	offset;

	// reset the score count for next player
	tag_count = 0;
	tag_owner = nullptr;

	tag_token = G_Spawn();

	tag_token->classname = item->classname;
	tag_token->item = item;
	tag_token->spawnflags = SPAWNFLAG_ITEM_DROPPED;
	tag_token->s.effects = EF_ROTATE | EF_TAGTRAIL;
	tag_token->s.renderfx = RF_GLOW | RF_NO_LOD;
	tag_token->mins = { -15, -15, -15 };
	tag_token->maxs = { 15, 15, 15 };
	gi.setmodel(tag_token, tag_token->item->world_model);
	tag_token->solid = SOLID_TRIGGER;
	tag_token->movetype = MOVETYPE_TOSS;
	tag_token->touch = nullptr;
	tag_token->owner = ent;

	AngleVectors(ent->client->v_angle, forward, right, nullptr);
	offset = { 24, 0, -16 };
	tag_token->s.origin = G_ProjectSource(ent->s.origin, offset, forward, right);
	trace = gi.trace(ent->s.origin, tag_token->mins, tag_token->maxs,
					 tag_token->s.origin, ent, CONTENTS_SOLID);
	tag_token->s.origin = trace.endpos;

	tag_token->velocity = forward * 100;
	tag_token->velocity[2] = 300;

	tag_token->think = Tag_MakeTouchable;
	tag_token->nextthink = level.time + 1_sec;

	gi.linkentity(tag_token);

	//	tag_token = Drop_Item (ent, item);
	ent->client->pers.inventory[item->id]--;
}

//=================
//=================
void Tag_PlayerEffects(edict_t *ent)
{
	if (ent == tag_owner)
		ent->s.effects |= EF_TAGTRAIL;
}

//=================
//=================
void Tag_DogTag(edict_t *ent, edict_t *killer, const char **pic)
{
	if (ent == tag_owner)
		(*pic) = "tag3";
}

//=================
// Tag_ChangeDamage - damage done that does not involve the tag owner
//		is at 75% original to encourage folks to go after the tag owner.
//=================
int Tag_ChangeDamage(edict_t *targ, edict_t *attacker, int damage, mod_t mod)
{
	if ((targ != tag_owner) && (attacker != tag_owner))
		return (damage * 3 / 4);

	return damage;
}

//=================
//=================
void Tag_GameInit()
{
	tag_token = nullptr;
	tag_owner = nullptr;
	tag_count = 0;
}

//=================
//=================
void Tag_PostInitSetup()
{
	edict_t *e;
	vec3_t	 origin, angles;

	// automatic spawning of tag token if one is not present on map.
	e = G_FindByString<&edict_t::classname>(nullptr, "dm_tag_token");
	if (e == nullptr)
	{
		e = G_Spawn();
		e->classname = "dm_tag_token";

		bool lm = false;
		SelectSpawnPoint(e, origin, angles, true, lm);
		e->s.origin = origin;
		e->s.old_origin = origin;
		e->s.angles = angles;
		SP_dm_tag_token(e);
	}
}

/*QUAKED dm_tag_token (.3 .3 1) (-16 -16 -16) (16 16 16)
The tag token for deathmatch tag games.
*/
void SP_dm_tag_token(edict_t *self)
{
	if (!deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	if (gamerules->integer != RDM_TAG)
	{
		G_FreeEdict(self);
		return;
	}

	// store the tag token edict pointer for later use.
	tag_token = self;
	tag_count = 0;

	self->classname = "dm_tag_token";
	self->model = "models/items/tagtoken/tris.md2";
	self->count = 1;
	SpawnItem(self, GetItemByIndex(IT_ITEM_TAG_TOKEN));
}
