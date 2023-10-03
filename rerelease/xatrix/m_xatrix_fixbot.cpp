// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
	fixbot.c
*/

#include "../g_local.h"
#include "m_xatrix_fixbot.h"
#include "../m_flash.h"

bool infront(edict_t *self, edict_t *other);
bool FindTarget(edict_t *self);

static cached_soundindex sound_pain1;
static cached_soundindex sound_die;
static cached_soundindex sound_weld1;
static cached_soundindex sound_weld2;
static cached_soundindex sound_weld3;

void fixbot_run(edict_t *self);
void fixbot_attack(edict_t *self);
void fixbot_dead(edict_t *self);
void fixbot_fire_blaster(edict_t *self);
void fixbot_fire_welder(edict_t *self);

void use_scanner(edict_t *self);
void change_to_roam(edict_t *self);
void fly_vertical(edict_t *self);

void fixbot_stand(edict_t *self);

extern const mmove_t fixbot_move_forward;
extern const mmove_t fixbot_move_stand;
extern const mmove_t fixbot_move_stand2;
extern const mmove_t fixbot_move_roamgoal;

extern const mmove_t fixbot_move_weld_start;
extern const mmove_t fixbot_move_weld;
extern const mmove_t fixbot_move_weld_end;
extern const mmove_t fixbot_move_takeoff;
extern const mmove_t fixbot_move_landing;
extern const mmove_t fixbot_move_turn;

void roam_goal(edict_t *self);

// [Paril-KEX] clean up bot goals if we get interrupted
THINK(bot_goal_check) (edict_t *self) -> void
{
	if (!self->owner || !self->owner->inuse || self->owner->goalentity != self)
	{
		G_FreeEdict(self);
		return;
	}

	self->nextthink = level.time + 1_ms;
}

void ED_CallSpawn(edict_t *ent);

edict_t *fixbot_FindDeadMonster(edict_t *self)
{
	edict_t *ent = nullptr;
	edict_t *best = nullptr;

	while ((ent = findradius(ent, self->s.origin, 1024)) != nullptr)
	{
		if (ent == self)
			continue;
		if (!(ent->svflags & SVF_MONSTER))
			continue;
		if (ent->monsterinfo.aiflags & AI_GOOD_GUY)
			continue;
		// check to make sure we haven't bailed on this guy already
		if ((ent->monsterinfo.badMedic1 == self) || (ent->monsterinfo.badMedic2 == self))
			continue;
		if (ent->monsterinfo.healer)
			// FIXME - this is correcting a bug that is somewhere else
			// if the healer is a monster, and it's in medic mode .. continue .. otherwise
			//   we will override the healer, if it passes all the other tests
			if ((ent->monsterinfo.healer->inuse) && (ent->monsterinfo.healer->health > 0) &&
				(ent->monsterinfo.healer->svflags & SVF_MONSTER) && (ent->monsterinfo.healer->monsterinfo.aiflags & AI_MEDIC))
				continue;
		if (ent->health > 0)
			continue;
		if ((ent->nextthink) && (ent->think != monster_dead_think))
			continue;
		if (!visible(self, ent))
			continue;
		if (!best)
		{
			best = ent;
			continue;
		}
		if (ent->max_health <= best->max_health)
			continue;
		best = ent;
	}

	return best;
}

static void fixbot_set_fly_parameters(edict_t *self, bool heal, bool weld)
{
	self->monsterinfo.fly_position_time = 0_sec;
	self->monsterinfo.fly_acceleration = 5.f;
	self->monsterinfo.fly_speed = 110.f;
	self->monsterinfo.fly_buzzard = false;

	if (heal)
	{
		self->monsterinfo.fly_min_distance = 100.f;
		self->monsterinfo.fly_max_distance = 100.f;
		self->monsterinfo.fly_thrusters = true;
	}
	else if (weld)
	{
		self->monsterinfo.fly_min_distance = 24.f;
		self->monsterinfo.fly_max_distance = 24.f;
	}
	else
	{
		// timid bot
		self->monsterinfo.fly_min_distance = 300.f;
		self->monsterinfo.fly_max_distance = 500.f;
	}
}

int fixbot_search(edict_t *self)
{
	edict_t *ent;

	if (!self->enemy)
	{
		ent = fixbot_FindDeadMonster(self);
		if (ent)
		{
			self->oldenemy = self->enemy;
			self->enemy = ent;
			self->enemy->monsterinfo.healer = self;
			self->monsterinfo.aiflags |= AI_MEDIC;
			FoundTarget(self);
			fixbot_set_fly_parameters(self, true, false);
			return (1);
		}
	}
	return (0);
}

void landing_goal(edict_t *self)
{
	trace_t	 tr;
	vec3_t	 forward, right, up;
	vec3_t	 end;
	edict_t *ent;

	ent = G_Spawn();
	ent->classname = "bot_goal";
	ent->solid = SOLID_BBOX;
	ent->owner = self;
	ent->think = bot_goal_check;
	gi.linkentity(ent);

	ent->mins = { -32, -32, -24 };
	ent->maxs = { 32, 32, 24 };

	AngleVectors(self->s.angles, forward, right, up);
	end = self->s.origin + (forward * 32);
	end = self->s.origin + (up * -8096);

	tr = gi.trace(self->s.origin, ent->mins, ent->maxs, end, self, MASK_MONSTERSOLID);

	ent->s.origin = tr.endpos;

	self->goalentity = self->enemy = ent;
	M_SetAnimation(self, &fixbot_move_landing);
}

void takeoff_goal(edict_t *self)
{
	trace_t	 tr;
	vec3_t	 forward, right, up;
	vec3_t	 end;
	edict_t *ent;

	ent = G_Spawn();
	ent->classname = "bot_goal";
	ent->solid = SOLID_BBOX;
	ent->owner = self;
	ent->think = bot_goal_check;
	gi.linkentity(ent);

	ent->mins = { -32, -32, -24 };
	ent->maxs = { 32, 32, 24 };

	AngleVectors(self->s.angles, forward, right, up);
	end = self->s.origin + (forward * 32);
	end = self->s.origin + (up * 128);

	tr = gi.trace(self->s.origin, ent->mins, ent->maxs, end, self, MASK_MONSTERSOLID);

	ent->s.origin = tr.endpos;

	self->goalentity = self->enemy = ent;
	M_SetAnimation(self, &fixbot_move_takeoff);
}

void change_to_roam(edict_t *self)
{

	if (fixbot_search(self))
		return;

	M_SetAnimation(self, &fixbot_move_roamgoal);

	if (self->spawnflags.has(SPAWNFLAG_FIXBOT_LANDING))
	{
		landing_goal(self);
		M_SetAnimation(self, &fixbot_move_landing);
		self->spawnflags &= ~SPAWNFLAG_FIXBOT_LANDING;
		self->spawnflags = SPAWNFLAG_FIXBOT_WORKING;
	}
	if (self->spawnflags.has(SPAWNFLAG_FIXBOT_TAKEOFF))
	{
		takeoff_goal(self);
		M_SetAnimation(self, &fixbot_move_takeoff);
		self->spawnflags &= ~SPAWNFLAG_FIXBOT_TAKEOFF;
		self->spawnflags = SPAWNFLAG_FIXBOT_WORKING;
	}
	if (self->spawnflags.has(SPAWNFLAG_FIXBOT_FIXIT))
	{
		M_SetAnimation(self, &fixbot_move_roamgoal);
		self->spawnflags &= ~SPAWNFLAG_FIXBOT_FIXIT;
		self->spawnflags = SPAWNFLAG_FIXBOT_WORKING;
	}
	if (!self->spawnflags)
	{
		M_SetAnimation(self, &fixbot_move_stand2);
	}
}

void roam_goal(edict_t *self)
{

	trace_t	 tr;
	vec3_t	 forward, right, up;
	vec3_t	 end;
	edict_t *ent;
	vec3_t	 dang;
	float	 len, oldlen;
	int		 i;
	vec3_t	 vec;
	vec3_t	 whichvec {};

	ent = G_Spawn();
	ent->classname = "bot_goal";
	ent->solid = SOLID_BBOX;
	ent->owner = self;
	ent->think = bot_goal_check;
	ent->nextthink = level.time + 1_ms;
	gi.linkentity(ent);

	oldlen = 0;

	for (i = 0; i < 12; i++)
	{

		dang = self->s.angles;

		if (i < 6)
			dang[YAW] += 30 * i;
		else
			dang[YAW] -= 30 * (i - 6);

		AngleVectors(dang, forward, right, up);
		end = self->s.origin + (forward * 8192);

		tr = gi.traceline(self->s.origin, end, self, MASK_PROJECTILE);

		vec = self->s.origin - tr.endpos;
		len = vec.normalize();

		if (len > oldlen)
		{
			oldlen = len;
			whichvec = tr.endpos;
		}
	}

	ent->s.origin = whichvec;
	self->goalentity = self->enemy = ent;

	M_SetAnimation(self, &fixbot_move_turn);
}

void use_scanner(edict_t *self)
{
	edict_t *ent = nullptr;

	float  radius = 1024;
	vec3_t vec;

	float len;

	while ((ent = findradius(ent, self->s.origin, radius)) != nullptr)
	{
		if (ent->health >= 100)
		{
			if (strcmp(ent->classname, "object_repair") == 0)
			{
				if (visible(self, ent))
				{
					// remove the old one
					if (strcmp(self->goalentity->classname, "bot_goal") == 0)
					{
						self->goalentity->nextthink = level.time + 100_ms;
						self->goalentity->think = G_FreeEdict;
					}

					self->goalentity = self->enemy = ent;

					vec = self->s.origin - self->goalentity->s.origin;
					len = vec.normalize();

					fixbot_set_fly_parameters(self, false, true);

					if (len < 32)
					{
						M_SetAnimation(self, &fixbot_move_weld_start);
						return;
					}
					return;
				}
			}
		}
	}

	if (!self->goalentity)
	{
		M_SetAnimation(self, &fixbot_move_stand);
		return;
	}

	vec = self->s.origin - self->goalentity->s.origin;
	len = vec.length();

	if (len < 32)
	{
		if (strcmp(self->goalentity->classname, "object_repair") == 0)
		{
			M_SetAnimation(self, &fixbot_move_weld_start);
		}
		else
		{
			self->goalentity->nextthink = level.time + 100_ms;
			self->goalentity->think = G_FreeEdict;
			self->goalentity = self->enemy = nullptr;
			M_SetAnimation(self, &fixbot_move_stand);
		}
		return;
	}

	vec = self->s.origin - self->s.old_origin;
	len = vec.length();

	/*
	  bot is stuck get new goalentity
	*/
	if (len == 0)
	{
		if (strcmp(self->goalentity->classname, "object_repair") == 0)
		{
			M_SetAnimation(self, &fixbot_move_stand);
		}
		else
		{
			self->goalentity->nextthink = level.time + 100_ms;
			self->goalentity->think = G_FreeEdict;
			self->goalentity = self->enemy = nullptr;
			M_SetAnimation(self, &fixbot_move_stand);
		}
	}
}

/*
	when the bot has found a landing pad
	it will proceed to its goalentity
	just above the landing pad and
	decend translated along the z the current
	frames are at 10fps
*/
void blastoff(edict_t *self, const vec3_t &start, const vec3_t &aimdir, int damage, int kick, int te_impact, int hspread, int vspread)
{
	trace_t	   tr;
	vec3_t	   dir;
	vec3_t	   forward, right, up;
	vec3_t	   end;
	float	   r;
	float	   u;
	vec3_t	   water_start;
	bool	   water = false;
	contents_t content_mask = MASK_PROJECTILE | MASK_WATER;

	hspread += (self->s.frame - FRAME_takeoff_01);
	vspread += (self->s.frame - FRAME_takeoff_01);

	tr = gi.traceline(self->s.origin, start, self, MASK_PROJECTILE);
	if (!(tr.fraction < 1.0f))
	{
		dir = vectoangles(aimdir);
		AngleVectors(dir, forward, right, up);

		r = crandom() * hspread;
		u = crandom() * vspread;
		end = start + (forward * 8192);
		end += (right * r);
		end += (up * u);

		if (gi.pointcontents(start) & MASK_WATER)
		{
			water = true;
			water_start = start;
			content_mask &= ~MASK_WATER;
		}

		tr = gi.traceline(start, end, self, content_mask);

		// see if we hit water
		if (tr.contents & MASK_WATER)
		{
			int color;

			water = true;
			water_start = tr.endpos;

			if (start != tr.endpos)
			{
				if (tr.contents & CONTENTS_WATER)
				{
					if (strcmp(tr.surface->name, "*brwater") == 0)
						color = SPLASH_BROWN_WATER;
					else
						color = SPLASH_BLUE_WATER;
				}
				else if (tr.contents & CONTENTS_SLIME)
					color = SPLASH_SLIME;
				else if (tr.contents & CONTENTS_LAVA)
					color = SPLASH_LAVA;
				else
					color = SPLASH_UNKNOWN;

				if (color != SPLASH_UNKNOWN)
				{
					gi.WriteByte(svc_temp_entity);
					gi.WriteByte(TE_SPLASH);
					gi.WriteByte(8);
					gi.WritePosition(tr.endpos);
					gi.WriteDir(tr.plane.normal);
					gi.WriteByte(color);
					gi.multicast(tr.endpos, MULTICAST_PVS, false);
				}

				// change bullet's course when it enters water
				dir = end - start;
				dir = vectoangles(dir);
				AngleVectors(dir, forward, right, up);
				r = crandom() * hspread * 2;
				u = crandom() * vspread * 2;
				end = water_start + (forward * 8192);
				end += (right * r);
				end += (up * u);
			}

			// re-trace ignoring water this time
			tr = gi.traceline(water_start, end, self, MASK_PROJECTILE);
		}
	}

	// send gun puff / flash
	if (!((tr.surface) && (tr.surface->flags & SURF_SKY)))
	{
		if (tr.fraction < 1.0f)
		{
			if (tr.ent->takedamage)
			{
				T_Damage(tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, kick, DAMAGE_BULLET, MOD_BLASTOFF);
			}
			else
			{
				if (!(tr.surface->flags & SURF_SKY))
				{
					gi.WriteByte(svc_temp_entity);
					gi.WriteByte(te_impact);
					gi.WritePosition(tr.endpos);
					gi.WriteDir(tr.plane.normal);
					gi.multicast(tr.endpos, MULTICAST_PVS, false);

					if (self->client)
						PlayerNoise(self, tr.endpos, PNOISE_IMPACT);
				}
			}
		}
	}

	// if went through water, determine where the end and make a bubble trail
	if (water)
	{
		vec3_t pos;

		dir = tr.endpos - water_start;
		dir.normalize();
		pos = tr.endpos + (dir * -2);
		if (gi.pointcontents(pos) & MASK_WATER)
			tr.endpos = pos;
		else
			tr = gi.traceline(pos, water_start, tr.ent, MASK_WATER);

		pos = water_start + tr.endpos;
		pos *= 0.5f;

		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_BUBBLETRAIL);
		gi.WritePosition(water_start);
		gi.WritePosition(tr.endpos);
		gi.multicast(pos, MULTICAST_PVS, false);
	}
}

void fly_vertical(edict_t *self)
{
	int	   i;
	vec3_t v;
	vec3_t forward, right, up;
	vec3_t start;
	vec3_t tempvec;

	v = self->goalentity->s.origin - self->s.origin;
	self->ideal_yaw = vectoyaw(v);
	M_ChangeYaw(self);

	if (self->s.frame == FRAME_landing_58 || self->s.frame == FRAME_takeoff_16)
	{
		self->goalentity->nextthink = level.time + 100_ms;
		self->goalentity->think = G_FreeEdict;
		M_SetAnimation(self, &fixbot_move_stand);
		self->goalentity = self->enemy = nullptr;
	}

	// kick up some particles
	tempvec = self->s.angles;
	tempvec[PITCH] += 90;

	AngleVectors(tempvec, forward, right, up);
	start = self->s.origin;

	for (i = 0; i < 10; i++)
		blastoff(self, start, forward, 2, 1, TE_SHOTGUN, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD);

	// needs sound
}

void fly_vertical2(edict_t *self)
{
	vec3_t v;
	float  len;

	v = self->goalentity->s.origin - self->s.origin;
	len = v.length();
	self->ideal_yaw = vectoyaw(v);
	M_ChangeYaw(self);

	if (len < 32)
	{
		self->goalentity->nextthink = level.time + 100_ms;
		self->goalentity->think = G_FreeEdict;
		M_SetAnimation(self, &fixbot_move_stand);
		self->goalentity = self->enemy = nullptr;
	}

	// needs sound
}

mframe_t fixbot_frames_landing[] = {
	{ ai_move },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },

	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },

	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },

	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },

	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },

	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 },
	{ ai_move, 0, fly_vertical2 }
};
MMOVE_T(fixbot_move_landing) = { FRAME_landing_01, FRAME_landing_58, fixbot_frames_landing, nullptr };

/*
	generic ambient stand
*/
mframe_t fixbot_frames_stand[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, change_to_roam }

};
MMOVE_T(fixbot_move_stand) = { FRAME_ambient_01, FRAME_ambient_19, fixbot_frames_stand, nullptr };

mframe_t fixbot_frames_stand2[] = {
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },

	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand, 0, change_to_roam }
};
MMOVE_T(fixbot_move_stand2) = { FRAME_ambient_01, FRAME_ambient_19, fixbot_frames_stand2, nullptr };

#if 0
/*
	will need the pickup offset for the front pincers
	object will need to stop forward of the object
	and take the object with it ( this may require a variant of liftoff and landing )
*/
mframe_t fixbot_frames_pickup[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },

	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }

};
MMOVE_T(fixbot_move_pickup) = { FRAME_pickup_01, FRAME_pickup_27, fixbot_frames_pickup, nullptr };
#endif

/*
	generic frame to move bot
*/
mframe_t fixbot_frames_roamgoal[] = {
	{ ai_move, 0, roam_goal }
};
MMOVE_T(fixbot_move_roamgoal) = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_roamgoal, nullptr };

void ai_facing(edict_t *self, float dist)
{
	if (!self->goalentity)
	{
		fixbot_stand(self);
		return;
	}

	vec3_t v;

	if (infront(self, self->goalentity))
		M_SetAnimation(self, &fixbot_move_forward);
	else
	{
		v = self->goalentity->s.origin - self->s.origin;
		self->ideal_yaw = vectoyaw(v);
		M_ChangeYaw(self);
	}
};

mframe_t fixbot_frames_turn[] = {
	{ ai_facing }
};
MMOVE_T(fixbot_move_turn) = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_turn, nullptr };

void go_roam(edict_t *self)
{
	M_SetAnimation(self, &fixbot_move_stand);
}

/*
	takeoff
*/
mframe_t fixbot_frames_takeoff[] = {
	{ ai_move, 0.01f, fly_vertical },
	{ ai_move, 0.01f, fly_vertical },
	{ ai_move, 0.01f, fly_vertical },
	{ ai_move, 0.01f, fly_vertical },
	{ ai_move, 0.01f, fly_vertical },
	{ ai_move, 0.01f, fly_vertical },
	{ ai_move, 0.01f, fly_vertical },
	{ ai_move, 0.01f, fly_vertical },
	{ ai_move, 0.01f, fly_vertical },
	{ ai_move, 0.01f, fly_vertical },

	{ ai_move, 0.01f, fly_vertical },
	{ ai_move, 0.01f, fly_vertical },
	{ ai_move, 0.01f, fly_vertical },
	{ ai_move, 0.01f, fly_vertical },
	{ ai_move, 0.01f, fly_vertical },
	{ ai_move, 0.01f, fly_vertical }
};
MMOVE_T(fixbot_move_takeoff) = { FRAME_takeoff_01, FRAME_takeoff_16, fixbot_frames_takeoff, nullptr };

/* findout what this is */
mframe_t fixbot_frames_paina[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(fixbot_move_paina) = { FRAME_paina_01, FRAME_paina_06, fixbot_frames_paina, fixbot_run };

/* findout what this is */
mframe_t fixbot_frames_painb[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(fixbot_move_painb) = { FRAME_painb_01, FRAME_painb_08, fixbot_frames_painb, fixbot_run };

/*
	backup from pain
	call a generic painsound
	some spark effects
*/
mframe_t fixbot_frames_pain3[] = {
	{ ai_move, -1 }
};
MMOVE_T(fixbot_move_pain3) = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_pain3, fixbot_run };

#if 0
/*
	bot has compleated landing
	and is now on the grownd
	( may need second land if the bot is releasing jib into jib vat )
*/
mframe_t fixbot_frames_land[] = {
	{ ai_move }
};
MMOVE_T(fixbot_move_land) = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_land, nullptr };
#endif

void M_MoveToGoal(edict_t *ent, float dist);

void ai_movetogoal(edict_t *self, float dist)
{
	M_MoveToGoal(self, dist);
}
/*

*/
mframe_t fixbot_frames_forward[] = {
	{ ai_movetogoal, 5, use_scanner }
};
MMOVE_T(fixbot_move_forward) = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_forward, nullptr };

/*

*/
mframe_t fixbot_frames_walk[] = {
	{ ai_walk, 5 }
};
MMOVE_T(fixbot_move_walk) = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_walk, nullptr };

/*

*/
mframe_t fixbot_frames_run[] = {
	{ ai_run, 10 }
};
MMOVE_T(fixbot_move_run) = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_run, nullptr };

#if 0
/*
	raf
	note to self
	they could have a timer that will cause
	the bot to explode on countdown
*/
mframe_t fixbot_frames_death1[] = {
	{ ai_move }
};
MMOVE_T(fixbot_move_death1) = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_death1, fixbot_dead };

//
mframe_t fixbot_frames_backward[] = {
	{ ai_move }
};
MMOVE_T(fixbot_move_backward) = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_backward, nullptr };
#endif

//
mframe_t fixbot_frames_start_attack[] = {
	{ ai_charge }
};
MMOVE_T(fixbot_move_start_attack) = { FRAME_freeze_01, FRAME_freeze_01, fixbot_frames_start_attack, fixbot_attack };

#if 0
/*
	TBD:
	need to get laser attack anim
	attack with the laser blast
*/
mframe_t fixbot_frames_attack1[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, -10, fixbot_fire_blaster }
};
MMOVE_T(fixbot_move_attack1) = { FRAME_shoot_01, FRAME_shoot_06, fixbot_frames_attack1, nullptr };
#endif

void abortHeal(edict_t *self, bool change_frame, bool gib, bool mark);

PRETHINK(fixbot_laser_update) (edict_t *laser) -> void
{
	edict_t *self = laser->owner;

	vec3_t start, dir;
	AngleVectors(self->s.angles, dir, nullptr, nullptr);
	start = self->s.origin + (dir * 16);

	if (self->enemy && self->health > 0)
	{
		vec3_t point;
		point = (self->enemy->absmin + self->enemy->absmax) * 0.5f;
		if (self->monsterinfo.aiflags & AI_MEDIC)
			point[0] += sinf(level.time.seconds()) * 8;
		dir = point - self->s.origin;
		dir.normalize();
	}

	laser->s.origin = start;
	laser->movedir = dir;
	gi.linkentity(laser);
	dabeam_update(laser, true);
}

void fixbot_fire_laser(edict_t *self)
{
	// critter dun got blown up while bein' fixed
	if (!self->enemy || !self->enemy->inuse || self->enemy->health <= self->enemy->gib_health)
	{
		M_SetAnimation(self, &fixbot_move_stand);
		self->monsterinfo.aiflags &= ~AI_MEDIC;
		return;
	}

	monster_fire_dabeam(self, -1, false, fixbot_laser_update);

	if (self->enemy->health > (self->enemy->mass / 10))
	{
		vec3_t maxs;
		self->enemy->spawnflags = SPAWNFLAG_NONE;
		self->enemy->monsterinfo.aiflags &= AI_STINKY | AI_SPAWNED_MASK;
		self->enemy->target = nullptr;
		self->enemy->targetname = nullptr;
		self->enemy->combattarget = nullptr;
		self->enemy->deathtarget = nullptr;
		self->enemy->healthtarget = nullptr;
		self->enemy->itemtarget = nullptr;
		self->enemy->monsterinfo.healer = self;

		maxs = self->enemy->maxs;
		maxs[2] += 48; // compensate for change when they die

		trace_t tr = gi.trace(self->enemy->s.origin, self->enemy->mins, maxs, self->enemy->s.origin, self->enemy, MASK_MONSTERSOLID);
		if (tr.startsolid || tr.allsolid)
		{
			abortHeal(self, false, true, false);
			return;
		}
		else if (tr.ent != world)
		{
			abortHeal(self, false, true, false);
			return;
		}
		else
		{
			self->enemy->monsterinfo.aiflags |= AI_IGNORE_SHOTS | AI_DO_NOT_COUNT;

			// backup & restore health stuff, because of multipliers
			int32_t old_max_health = self->enemy->max_health;
			item_id_t old_power_armor_type = self->enemy->monsterinfo.initial_power_armor_type;
			int32_t old_power_armor_power = self->enemy->monsterinfo.max_power_armor_power;
			int32_t old_base_health = self->enemy->monsterinfo.base_health;
			int32_t old_health_scaling = self->enemy->monsterinfo.health_scaling;
			auto reinforcements = self->enemy->monsterinfo.reinforcements;
			int32_t monster_slots = self->enemy->monsterinfo.monster_slots;
			int32_t monster_used = self->enemy->monsterinfo.monster_used;
			int32_t old_gib_health = self->enemy->gib_health;

			st = {};
			st.keys_specified.emplace("reinforcements");
			st.reinforcements = "";

			ED_CallSpawn(self->enemy);

			self->enemy->monsterinfo.reinforcements = reinforcements;
			self->enemy->monsterinfo.monster_slots = monster_slots;
			self->enemy->monsterinfo.monster_used = monster_used;

			self->enemy->gib_health = old_gib_health / 2;
			self->enemy->health = self->enemy->max_health = old_max_health;
			self->enemy->monsterinfo.power_armor_power = self->enemy->monsterinfo.max_power_armor_power = old_power_armor_power;
			self->enemy->monsterinfo.power_armor_type = self->enemy->monsterinfo.initial_power_armor_type = old_power_armor_type;
			self->enemy->monsterinfo.base_health = old_base_health;
			self->enemy->monsterinfo.health_scaling = old_health_scaling;

			if (self->enemy->monsterinfo.setskin)
				self->enemy->monsterinfo.setskin(self->enemy);

			if (self->enemy->think)
			{
				self->enemy->nextthink = level.time;
				self->enemy->think(self->enemy);
			}
			self->enemy->monsterinfo.aiflags &= ~AI_RESURRECTING;
			self->enemy->monsterinfo.aiflags |= AI_IGNORE_SHOTS | AI_DO_NOT_COUNT;
			// turn off flies
			self->enemy->s.effects &= ~EF_FLIES;
			self->enemy->monsterinfo.healer = nullptr;

			// clean up target, if we have one and it's legit
			if (self->enemy && self->enemy->inuse)
			{
				cleanupHealTarget(self->enemy);

				if ((self->oldenemy) && (self->oldenemy->inuse) && (self->oldenemy->health > 0))
				{
					self->enemy->enemy = self->oldenemy;
					FoundTarget(self->enemy);
				}
				else
				{
					self->enemy->enemy = nullptr;
					if (!FindTarget(self->enemy))
					{
						// no valid enemy, so stop acting
						self->enemy->monsterinfo.pausetime = HOLD_FOREVER;
						self->enemy->monsterinfo.stand(self->enemy);
					}
					self->enemy = nullptr;
					self->oldenemy = nullptr;
					if (!FindTarget(self))
					{
						// no valid enemy, so stop acting
						self->monsterinfo.pausetime = HOLD_FOREVER;
						self->monsterinfo.stand(self);
						return;
					}
				}
			}
		}

		M_SetAnimation(self, &fixbot_move_stand);
	}
	else
		self->enemy->monsterinfo.aiflags |= AI_RESURRECTING;
}

mframe_t fixbot_frames_laserattack[] = {
	{ ai_charge, 0, fixbot_fire_laser },
	{ ai_charge, 0, fixbot_fire_laser },
	{ ai_charge, 0, fixbot_fire_laser },
	{ ai_charge, 0, fixbot_fire_laser },
	{ ai_charge, 0, fixbot_fire_laser },
	{ ai_charge, 0, fixbot_fire_laser }
};
MMOVE_T(fixbot_move_laserattack) = { FRAME_shoot_01, FRAME_shoot_06, fixbot_frames_laserattack, nullptr };

/*
	need to get forward translation data
	for the charge attack
*/
mframe_t fixbot_frames_attack2[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },

	{ ai_charge, -10 },
	{ ai_charge, -10 },
	{ ai_charge, -10 },
	{ ai_charge, -10 },
	{ ai_charge, -10 },
	{ ai_charge, -10 },
	{ ai_charge, -10 },
	{ ai_charge, -10 },
	{ ai_charge, -10 },
	{ ai_charge, -10 },

	{ ai_charge, 0, fixbot_fire_blaster },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },

	{ ai_charge }
};
MMOVE_T(fixbot_move_attack2) = { FRAME_charging_01, FRAME_charging_31, fixbot_frames_attack2, fixbot_run };

void weldstate(edict_t *self)
{
	if (self->s.frame == FRAME_weldstart_10)
		M_SetAnimation(self, &fixbot_move_weld);
	else if (self->goalentity && self->s.frame == FRAME_weldmiddle_07)
	{
		if (self->goalentity->health <= 0)
		{
			self->enemy->owner = nullptr;
			M_SetAnimation(self, &fixbot_move_weld_end);
		}
		else
			self->goalentity->health -= 10;
	}
	else
	{
		self->goalentity = self->enemy = nullptr;
		M_SetAnimation(self, &fixbot_move_stand);
	}
}

void ai_move2(edict_t *self, float dist)
{
	if (!self->goalentity)
	{
		fixbot_stand(self);
		return;
	}

	vec3_t v;

	M_walkmove(self, self->s.angles[YAW], dist);

	v = self->goalentity->s.origin - self->s.origin;
	self->ideal_yaw = vectoyaw(v);
	M_ChangeYaw(self);
};

mframe_t fixbot_frames_weld_start[] = {
	{ ai_move2, 0 },
	{ ai_move2, 0 },
	{ ai_move2, 0 },
	{ ai_move2, 0 },
	{ ai_move2, 0 },
	{ ai_move2, 0 },
	{ ai_move2, 0 },
	{ ai_move2, 0 },
	{ ai_move2, 0 },
	{ ai_move2, 0, weldstate }
};
MMOVE_T(fixbot_move_weld_start) = { FRAME_weldstart_01, FRAME_weldstart_10, fixbot_frames_weld_start, nullptr };

mframe_t fixbot_frames_weld[] = {
	{ ai_move2, 0, fixbot_fire_welder },
	{ ai_move2, 0, fixbot_fire_welder },
	{ ai_move2, 0, fixbot_fire_welder },
	{ ai_move2, 0, fixbot_fire_welder },
	{ ai_move2, 0, fixbot_fire_welder },
	{ ai_move2, 0, fixbot_fire_welder },
	{ ai_move2, 0, weldstate }
};
MMOVE_T(fixbot_move_weld) = { FRAME_weldmiddle_01, FRAME_weldmiddle_07, fixbot_frames_weld, nullptr };

mframe_t fixbot_frames_weld_end[] = {
	{ ai_move2, -2 },
	{ ai_move2, -2 },
	{ ai_move2, -2 },
	{ ai_move2, -2 },
	{ ai_move2, -2 },
	{ ai_move2, -2 },
	{ ai_move2, -2, weldstate }
};
MMOVE_T(fixbot_move_weld_end) = { FRAME_weldend_01, FRAME_weldend_07, fixbot_frames_weld_end, nullptr };

void fixbot_fire_welder(edict_t *self)
{
	vec3_t start;
	vec3_t forward, right, up;
	vec3_t end;
	vec3_t dir;
	vec3_t vec;
	float  r;

	if (!self->enemy)
		return;

	vec[0] = 24.0;
	vec[1] = -0.8f;
	vec[2] = -10.0;

	AngleVectors(self->s.angles, forward, right, up);
	start = M_ProjectFlashSource(self, vec, forward, right);

	end = self->enemy->s.origin;

	dir = end - start;

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_WELDING_SPARKS);
	gi.WriteByte(10);
	gi.WritePosition(start);
	gi.WriteDir(vec3_origin);
	gi.WriteByte(irandom(0xe0, 0xe8));
	gi.multicast(self->s.origin, MULTICAST_PVS, false);

	if (frandom() > 0.8f)
	{
		r = frandom();

		if (r < 0.33f)
			gi.sound(self, CHAN_VOICE, sound_weld1, 1, ATTN_IDLE, 0);
		else if (r < 0.66f)
			gi.sound(self, CHAN_VOICE, sound_weld2, 1, ATTN_IDLE, 0);
		else
			gi.sound(self, CHAN_VOICE, sound_weld3, 1, ATTN_IDLE, 0);
	}
}

void fixbot_fire_blaster(edict_t *self)
{
	vec3_t start;
	vec3_t forward, right, up;
	vec3_t end;
	vec3_t dir;

	if (!visible(self, self->enemy))
	{
		M_SetAnimation(self, &fixbot_move_run);
	}

	AngleVectors(self->s.angles, forward, right, up);
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_HOVER_BLASTER_1], forward, right);

	end = self->enemy->s.origin;
	end[2] += self->enemy->viewheight;
	dir = end - start;
	dir.normalize();

	monster_fire_blaster(self, start, dir, 15, 1000, MZ2_HOVER_BLASTER_1, EF_BLASTER);
}

MONSTERINFO_STAND(fixbot_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &fixbot_move_stand);
}

MONSTERINFO_RUN(fixbot_run) (edict_t *self) -> void
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		M_SetAnimation(self, &fixbot_move_stand);
	else
		M_SetAnimation(self, &fixbot_move_run);
}

MONSTERINFO_WALK(fixbot_walk) (edict_t *self) -> void
{
	vec3_t vec;
	float  len;

	if (self->goalentity && strcmp(self->goalentity->classname, "object_repair") == 0)
	{
		vec = self->s.origin - self->goalentity->s.origin;
		len = vec.length();
		if (len < 32)
		{
			M_SetAnimation(self, &fixbot_move_weld_start);
			return;
		}
	}
	M_SetAnimation(self, &fixbot_move_walk);
}

void fixbot_start_attack(edict_t *self)
{
	M_SetAnimation(self, &fixbot_move_start_attack);
}

MONSTERINFO_ATTACK(fixbot_attack) (edict_t *self) -> void
{
	vec3_t vec;
	float  len;

	if (self->monsterinfo.aiflags & AI_MEDIC)
	{
		if (!visible(self, self->enemy))
			return;
		vec = self->s.origin - self->enemy->s.origin;
		len = vec.length();
		if (len > 128)
			return;
		else
			M_SetAnimation(self, &fixbot_move_laserattack);
	}
	else
	{
		fixbot_set_fly_parameters(self, false, false);
		M_SetAnimation(self, &fixbot_move_attack2);
	}
}

PAIN(fixbot_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	if (level.time < self->pain_debounce_time)
		return;

	fixbot_set_fly_parameters(self, false, false);
	self->pain_debounce_time = level.time + 3_sec;
	gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);

	if (damage <= 10)
		M_SetAnimation(self, &fixbot_move_pain3);
	else if (damage <= 25)
		M_SetAnimation(self, &fixbot_move_painb);
	else
		M_SetAnimation(self, &fixbot_move_paina);

	abortHeal(self, false, false, false);
}

void fixbot_dead(edict_t *self)
{
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, -8 };
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0_ms;
	gi.linkentity(self);
}

DIE(fixbot_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	BecomeExplosion1(self);

	// shards
}

/*QUAKED monster_fixbot (1 .5 0) (-32 -32 -24) (32 32 24) Ambush Trigger_Spawn Fixit Takeoff Landing
 */
void SP_monster_fixbot(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict( self );
		return;
	}

	sound_pain1.assign("flyer/flypain1.wav");
	sound_die.assign("flyer/flydeth1.wav");

	sound_weld1.assign("misc/welder1.wav");
	sound_weld2.assign("misc/welder2.wav");
	sound_weld3.assign("misc/welder3.wav");

	self->s.modelindex = gi.modelindex("models/monsters/fixbot/tris.md2");

	self->mins = { -32, -32, -24 };
	self->maxs = { 32, 32, 24 };

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->health = 150 * st.health_multiplier;
	self->mass = 150;

	self->pain = fixbot_pain;
	self->die = fixbot_die;

	self->monsterinfo.stand = fixbot_stand;
	self->monsterinfo.walk = fixbot_walk;
	self->monsterinfo.run = fixbot_run;
	self->monsterinfo.attack = fixbot_attack;

	gi.linkentity(self);

	M_SetAnimation(self, &fixbot_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;
	self->monsterinfo.aiflags |= AI_ALTERNATE_FLY;
	fixbot_set_fly_parameters(self, false, false);

	flymonster_start(self);
}
