// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_actor.c

#include "g_local.h"
#include "m_actor.h"
#include "m_flash.h"

constexpr const char *actor_names[] = {
	"Hellrot",
	"Tokay",
	"Killme",
	"Disruptor",
	"Adrianator",
	"Rambear",
	"Titus",
	"Bitterman"
};

mframe_t actor_frames_stand[] = {
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
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }
};
MMOVE_T(actor_move_stand) = { FRAME_stand101, FRAME_stand140, actor_frames_stand, nullptr };

MONSTERINFO_STAND(actor_stand) (edict_t *self) -> void
{
	M_SetAnimation(self, &actor_move_stand);

	// randomize on startup
	if (level.time < 1_sec)
		self->s.frame = irandom(self->monsterinfo.active_move->firstframe, self->monsterinfo.active_move->lastframe + 1);
}

mframe_t actor_frames_walk[] = {
	{ ai_walk },
	{ ai_walk, 6 },
	{ ai_walk, 10 },
	{ ai_walk, 3 },
	{ ai_walk, 2 },
	{ ai_walk, 7 },
	{ ai_walk, 10 },
	{ ai_walk, 1 }
};
MMOVE_T(actor_move_walk) = { FRAME_walk01, FRAME_walk08, actor_frames_walk, nullptr };

MONSTERINFO_WALK(actor_walk) (edict_t *self) -> void
{
	M_SetAnimation(self, &actor_move_walk);
}

mframe_t actor_frames_run[] = {
	{ ai_run, 4 },
	{ ai_run, 15 },
	{ ai_run, 15 },
	{ ai_run, 8 },
	{ ai_run, 20 },
	{ ai_run, 15 }
};
MMOVE_T(actor_move_run) = { FRAME_run02, FRAME_run07, actor_frames_run, nullptr };

MONSTERINFO_RUN(actor_run) (edict_t *self) -> void
{
	if ((level.time < self->pain_debounce_time) && (!self->enemy))
	{
		if (self->movetarget)
			actor_walk(self);
		else
			actor_stand(self);
		return;
	}

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		actor_stand(self);
		return;
	}

	M_SetAnimation(self, &actor_move_run);
}

mframe_t actor_frames_pain1[] = {
	{ ai_move, -5 },
	{ ai_move, 4 },
	{ ai_move, 1 }
};
MMOVE_T(actor_move_pain1) = { FRAME_pain101, FRAME_pain103, actor_frames_pain1, actor_run };

mframe_t actor_frames_pain2[] = {
	{ ai_move, -4 },
	{ ai_move, 4 },
	{ ai_move }
};
MMOVE_T(actor_move_pain2) = { FRAME_pain201, FRAME_pain203, actor_frames_pain2, actor_run };

mframe_t actor_frames_pain3[] = {
	{ ai_move, -1 },
	{ ai_move, 1 },
	{ ai_move, 0 }
};
MMOVE_T(actor_move_pain3) = { FRAME_pain301, FRAME_pain303, actor_frames_pain3, actor_run };

mframe_t actor_frames_flipoff[] = {
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn }
};
MMOVE_T(actor_move_flipoff) = { FRAME_flip01, FRAME_flip14, actor_frames_flipoff, actor_run };

mframe_t actor_frames_taunt[] = {
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn },
	{ ai_turn }
};
MMOVE_T(actor_move_taunt) = { FRAME_taunt01, FRAME_taunt17, actor_frames_taunt, actor_run };

const char *messages[] = {
	"Watch it",
	"#$@*&",
	"Idiot",
	"Check your targets"
};

PAIN(actor_pain) (edict_t *self, edict_t *other, float kick, int damage, const mod_t &mod) -> void
{
	int n;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3_sec;
	//	gi.sound (self, CHAN_VOICE, actor.sound_pain, 1, ATTN_NORM, 0);

	if ((other->client) && (frandom() < 0.4f))
	{
		vec3_t		v;
		const char *name;

		v = other->s.origin - self->s.origin;
		self->ideal_yaw = vectoyaw(v);
		if (frandom() < 0.5f)
			M_SetAnimation(self, &actor_move_flipoff);
		else
			M_SetAnimation(self, &actor_move_taunt);
		name = actor_names[(self - g_edicts) % q_countof(actor_names)];
		gi.LocClient_Print(other, PRINT_CHAT, "{}: {}!\n", name, random_element(messages));
		return;
	}

	n = irandom(3);
	if (n == 0)
		M_SetAnimation(self, &actor_move_pain1);
	else if (n == 1)
		M_SetAnimation(self, &actor_move_pain2);
	else
		M_SetAnimation(self, &actor_move_pain3);
}

MONSTERINFO_SETSKIN(actor_setskin) (edict_t *self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;
	else
		self->s.skinnum = 0;
}

void actorMachineGun(edict_t *self)
{
	vec3_t start, target;
	vec3_t forward, right;

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = G_ProjectSource(self->s.origin, monster_flash_offset[MZ2_ACTOR_MACHINEGUN_1], forward, right);
	if (self->enemy)
	{
		if (self->enemy->health > 0)
		{
			target = self->enemy->s.origin + (self->enemy->velocity * -0.2f);
			target[2] += self->enemy->viewheight;
		}
		else
		{
			target = self->enemy->absmin;
			target[2] += (self->enemy->size[2] / 2) + 1;
		}
		forward = target - start;
		forward.normalize();
	}
	else
	{
		AngleVectors(self->s.angles, forward, nullptr, nullptr);
	}
	monster_fire_bullet(self, start, forward, 3, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MZ2_ACTOR_MACHINEGUN_1);
}

void actor_dead(edict_t *self)
{
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, -8 };
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0_ms;
	gi.linkentity(self);
}

mframe_t actor_frames_death1[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move, -13 },
	{ ai_move, 14 },
	{ ai_move, 3 },
	{ ai_move, -2 },
	{ ai_move, 1 }
};
MMOVE_T(actor_move_death1) = { FRAME_death101, FRAME_death107, actor_frames_death1, actor_dead };

mframe_t actor_frames_death2[] = {
	{ ai_move },
	{ ai_move, 7 },
	{ ai_move, -6 },
	{ ai_move, -5 },
	{ ai_move, 1 },
	{ ai_move },
	{ ai_move, -1 },
	{ ai_move, -2 },
	{ ai_move, -1 },
	{ ai_move, -9 },
	{ ai_move, -13 },
	{ ai_move, -13 },
	{ ai_move }
};
MMOVE_T(actor_move_death2) = { FRAME_death201, FRAME_death213, actor_frames_death2, actor_dead };

DIE(actor_die) (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, const vec3_t &point, const mod_t &mod) -> void
{
	// check for gib
	if (self->health <= -80)
	{
		//		gi.sound (self, CHAN_VOICE, actor.sound_gib, 1, ATTN_NORM, 0);
		ThrowGibs(self, damage, {
			{ 2, "models/objects/gibs/bone/tris.md2" },
			{ 4, "models/objects/gibs/sm_meat/tris.md2" },
			{ "models/objects/gibs/head2/tris.md2", GIB_HEAD }
		});
		self->deadflag = true;
		return;
	}

	if (self->deadflag)
		return;

	// regular death
	//	gi.sound (self, CHAN_VOICE, actor.sound_die, 1, ATTN_NORM, 0);
	self->deadflag = true;
	self->takedamage = true;

	if (brandom())
		M_SetAnimation(self, &actor_move_death1);
	else
		M_SetAnimation(self, &actor_move_death2);
}

void actor_fire(edict_t *self)
{
	actorMachineGun(self);

	if (level.time >= self->monsterinfo.fire_wait)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

mframe_t actor_frames_attack[] = {
	{ ai_charge, -2, actor_fire },
	{ ai_charge, -2 },
	{ ai_charge, 3 },
	{ ai_charge, 2 }
};
MMOVE_T(actor_move_attack) = { FRAME_attak01, FRAME_attak04, actor_frames_attack, actor_run };

MONSTERINFO_ATTACK(actor_attack) (edict_t *self) -> void
{
	M_SetAnimation(self, &actor_move_attack);
	self->monsterinfo.fire_wait = level.time + random_time(1_sec, 2.6_sec);
}

USE(actor_use) (edict_t *self, edict_t *other, edict_t *activator) -> void
{
	vec3_t v;

	self->goalentity = self->movetarget = G_PickTarget(self->target);
	if ((!self->movetarget) || (strcmp(self->movetarget->classname, "target_actor") != 0))
	{
		gi.Com_PrintFmt("{}: bad target {}\n", *self, self->target);
		self->target = nullptr;
		self->monsterinfo.pausetime = HOLD_FOREVER;
		self->monsterinfo.stand(self);
		return;
	}

	v = self->goalentity->s.origin - self->s.origin;
	self->ideal_yaw = self->s.angles[YAW] = vectoyaw(v);
	self->monsterinfo.walk(self);
	self->target = nullptr;
}

/*QUAKED misc_actor (1 .5 0) (-16 -16 -24) (16 16 32)
 */

void SP_misc_actor(edict_t *self)
{
	if ( !M_AllowSpawn( self ) ) {
		G_FreeEdict(self);
		return;
	}

	if (!self->targetname)
	{
		gi.Com_PrintFmt("{}: no targetname\n", *self);
		G_FreeEdict(self);
		return;
	}

	if (!self->target)
	{
		gi.Com_PrintFmt("{}: no target\n", *self);
		G_FreeEdict(self);
		return;
	}

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("players/male/tris.md2");
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, 32 };

	if (!self->health)
		self->health = 100;
	self->mass = 200;

	self->pain = actor_pain;
	self->die = actor_die;

	self->monsterinfo.stand = actor_stand;
	self->monsterinfo.walk = actor_walk;
	self->monsterinfo.run = actor_run;
	self->monsterinfo.attack = actor_attack;
	self->monsterinfo.melee = nullptr;
	self->monsterinfo.sight = nullptr;
	self->monsterinfo.setskin = actor_setskin;

	self->monsterinfo.aiflags |= AI_GOOD_GUY;

	gi.linkentity(self);

	M_SetAnimation(self, &actor_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start(self);

	// actors always start in a dormant state, they *must* be used to get going
	self->use = actor_use;
}

/*QUAKED target_actor (.5 .3 0) (-8 -8 -8) (8 8 8) JUMP SHOOT ATTACK x HOLD BRUTAL
JUMP			jump in set direction upon reaching this target
SHOOT			take a single shot at the pathtarget
ATTACK			attack pathtarget until it or actor is dead

"target"		next target_actor
"pathtarget"	target of any action to be taken at this point
"wait"			amount of time actor should pause at this point
"message"		actor will "say" this to the player

for JUMP only:
"speed"			speed thrown forward (default 200)
"height"		speed thrown upwards (default 200)
*/

constexpr spawnflags_t SPAWNFLAG_TARGET_ACTOR_JUMP = 1_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TARGET_ACTOR_SHOOT = 2_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TARGET_ACTOR_ATTACK = 4_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TARGET_ACTOR_HOLD = 16_spawnflag;
constexpr spawnflags_t SPAWNFLAG_TARGET_ACTOR_BRUTAL = 32_spawnflag;

TOUCH(target_actor_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	vec3_t v;

	if (other->movetarget != self)
		return;

	if (other->enemy)
		return;

	other->goalentity = other->movetarget = nullptr;

	if (self->message)
	{
		edict_t *ent;

		for (uint32_t n = 1; n <= game.maxclients; n++)
		{
			ent = &g_edicts[n];
			if (!ent->inuse)
				continue;
			gi.LocClient_Print(ent, PRINT_CHAT, "{}: {}\n", actor_names[(other - g_edicts) % q_countof(actor_names)], self->message);
		}
	}

	if (self->spawnflags.has(SPAWNFLAG_TARGET_ACTOR_JUMP)) // jump
	{
		other->velocity[0] = self->movedir[0] * self->speed;
		other->velocity[1] = self->movedir[1] * self->speed;

		if (other->groundentity)
		{
			other->groundentity = nullptr;
			other->velocity[2] = self->movedir[2];
			gi.sound(other, CHAN_VOICE, gi.soundindex("player/male/jump1.wav"), 1, ATTN_NORM, 0);
		}
	}

	if (self->spawnflags.has(SPAWNFLAG_TARGET_ACTOR_SHOOT)) // shoot
	{
	}
	else if (self->spawnflags.has(SPAWNFLAG_TARGET_ACTOR_ATTACK)) // attack
	{
		other->enemy = G_PickTarget(self->pathtarget);
		if (other->enemy)
		{
			other->goalentity = other->enemy;
			if (self->spawnflags.has(SPAWNFLAG_TARGET_ACTOR_BRUTAL))
				other->monsterinfo.aiflags |= AI_BRUTAL;
			if (self->spawnflags.has(SPAWNFLAG_TARGET_ACTOR_HOLD))
			{
				other->monsterinfo.aiflags |= AI_STAND_GROUND;
				actor_stand(other);
			}
			else
			{
				actor_run(other);
			}
		}
	}

	if (!self->spawnflags.has((SPAWNFLAG_TARGET_ACTOR_ATTACK | SPAWNFLAG_TARGET_ACTOR_SHOOT)) && (self->pathtarget))
	{
		const char *savetarget;

		savetarget = self->target;
		self->target = self->pathtarget;
		G_UseTargets(self, other);
		self->target = savetarget;
	}

	other->movetarget = G_PickTarget(self->target);

	if (!other->goalentity)
		other->goalentity = other->movetarget;

	if (!other->movetarget && !other->enemy)
	{
		other->monsterinfo.pausetime = HOLD_FOREVER;
		other->monsterinfo.stand(other);
	}
	else if (other->movetarget == other->goalentity)
	{
		v = other->movetarget->s.origin - other->s.origin;
		other->ideal_yaw = vectoyaw(v);
	}
}

void SP_target_actor(edict_t *self)
{
	if (!self->targetname)
		gi.Com_PrintFmt("{}: no targetname\n", *self);

	self->solid = SOLID_TRIGGER;
	self->touch = target_actor_touch;
	self->mins = { -8, -8, -8 };
	self->maxs = { 8, 8, 8 };
	self->svflags = SVF_NOCLIENT;

	if (self->spawnflags.has(SPAWNFLAG_TARGET_ACTOR_JUMP))
	{
		if (!self->speed)
			self->speed = 200;
		if (!st.height)
			st.height = 200;
		if (self->s.angles[YAW] == 0)
			self->s.angles[YAW] = 360;
		G_SetMovedir(self->s.angles, self->movedir);
		self->movedir[2] = (float) st.height;
	}

	gi.linkentity(self);
}
