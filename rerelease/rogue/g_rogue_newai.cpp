// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "../g_local.h"

//===============================
// BLOCKED Logic
//===============================

bool face_wall(edict_t *self);

// blocked_checkplat
//	dist: how far they are trying to walk.
bool blocked_checkplat(edict_t *self, float dist)
{
	int		 playerPosition;
	trace_t	 trace;
	vec3_t	 pt1, pt2;
	vec3_t	 forward;
	edict_t *plat;

	if (!self->enemy)
		return false;

	// check player's relative altitude
	if (self->enemy->absmin[2] >= self->absmax[2])
		playerPosition = 1;
	else if (self->enemy->absmax[2] <= self->absmin[2])
		playerPosition = -1;
	else
		playerPosition = 0;

	// if we're close to the same position, don't bother trying plats.
	if (playerPosition == 0)
		return false;

	plat = nullptr;

	// see if we're already standing on a plat.
	if (self->groundentity && self->groundentity != world)
	{
		if (!strncmp(self->groundentity->classname, "func_plat", 8))
			plat = self->groundentity;
	}

	// if we're not, check to see if we'll step onto one with this move
	if (!plat)
	{
		AngleVectors(self->s.angles, forward, nullptr, nullptr);
		pt1 = self->s.origin + (forward * dist);
		pt2 = pt1;
		pt2[2] -= 384;

		trace = gi.traceline(pt1, pt2, self, MASK_MONSTERSOLID);
		if (trace.fraction < 1 && !trace.allsolid && !trace.startsolid)
		{
			if (!strncmp(trace.ent->classname, "func_plat", 8))
			{
				plat = trace.ent;
			}
		}
	}

	// if we've found a plat, trigger it.
	if (plat && plat->use)
	{
		if (playerPosition == 1)
		{
			if ((self->groundentity == plat && plat->moveinfo.state == STATE_BOTTOM) ||
				(self->groundentity != plat && plat->moveinfo.state == STATE_TOP))
			{
				plat->use(plat, self, self);
				return true;
			}
		}
		else if (playerPosition == -1)
		{
			if ((self->groundentity == plat && plat->moveinfo.state == STATE_TOP) ||
				(self->groundentity != plat && plat->moveinfo.state == STATE_BOTTOM))
			{
				plat->use(plat, self, self);
				return true;
			}
		}
	}

	return false;
}

//*******************
// JUMPING AIDS
//*******************

inline void monster_jump_start(edict_t *self)
{
	monster_done_dodge(self);

	self->monsterinfo.jump_time = level.time + 3_sec;
}

bool monster_jump_finished(edict_t *self)
{
	// if we lost our forward velocity, give us more
	vec3_t forward;

	AngleVectors(self->s.angles, forward, nullptr, nullptr);

	vec3_t forward_velocity = self->velocity.scaled(forward);

	if (forward_velocity.length() < 150.f)
	{
		float z_velocity = self->velocity.z;
		self->velocity = forward * 150.f;
		self->velocity.z = z_velocity;
	}

	return self->monsterinfo.jump_time < level.time;
}

// blocked_checkjump
//	dist: how far they are trying to walk.
//  self->monsterinfo.drop_height/self->monsterinfo.jump_height: how far they'll ok a jump for. set to 0 to disable that direction.
blocked_jump_result_t blocked_checkjump(edict_t *self, float dist)
{
	// can't jump even if we physically can
	if (!self->monsterinfo.can_jump)
		return blocked_jump_result_t::NO_JUMP;
	// no enemy to path to
	else if (!self->enemy)
		return blocked_jump_result_t::NO_JUMP;

	// we just jumped recently, don't try again
	if (self->monsterinfo.jump_time > level.time)
		return blocked_jump_result_t::NO_JUMP;

	// if we're pathing, the nodes will ensure we can reach the destination.
	if (self->monsterinfo.aiflags & AI_PATHING)
	{
		if (self->monsterinfo.nav_path.returnCode != PathReturnCode::TraversalPending)
			return blocked_jump_result_t::NO_JUMP;

		float yaw = vectoyaw((self->monsterinfo.nav_path.firstMovePoint - self->monsterinfo.nav_path.secondMovePoint).normalized());
		self->ideal_yaw = yaw + 180;
		if (self->ideal_yaw > 360)
			self->ideal_yaw -= 360;

		if (!FacingIdeal(self))
		{
			M_ChangeYaw(self);
			return blocked_jump_result_t::JUMP_TURN;
		}

		monster_jump_start(self);

		if (self->monsterinfo.nav_path.secondMovePoint.z > self->monsterinfo.nav_path.firstMovePoint.z)
			return blocked_jump_result_t::JUMP_JUMP_UP;
		else
			return blocked_jump_result_t::JUMP_JUMP_DOWN;
	}

	int		playerPosition;
	trace_t trace;
	vec3_t	pt1, pt2;
	vec3_t	forward, up;

	AngleVectors(self->s.angles, forward, nullptr, up);

	if (self->monsterinfo.aiflags & AI_PATHING)
	{
		if (self->monsterinfo.nav_path.secondMovePoint[2] > (self->absmin[2] + STEPSIZE))
			playerPosition = 1;
		else if (self->monsterinfo.nav_path.secondMovePoint[2] < (self->absmin[2] - STEPSIZE))
			playerPosition = -1;
		else
			playerPosition = 0;
	}
	else
	{
		if (self->enemy->absmin[2] > (self->absmin[2] + STEPSIZE))
			playerPosition = 1;
		else if (self->enemy->absmin[2] < (self->absmin[2] - STEPSIZE))
			playerPosition = -1;
		else
			playerPosition = 0;
	}

	if (playerPosition == -1 && self->monsterinfo.drop_height)
	{
		// check to make sure we can even get to the spot we're going to "fall" from
		pt1 = self->s.origin + (forward * 48);
		trace = gi.trace(self->s.origin, self->mins, self->maxs, pt1, self, MASK_MONSTERSOLID);
		if (trace.fraction < 1)
			return blocked_jump_result_t::NO_JUMP;

		pt2 = pt1;
		pt2[2] = self->absmin[2] - self->monsterinfo.drop_height - 1;

		trace = gi.traceline(pt1, pt2, self, MASK_MONSTERSOLID | MASK_WATER);
		if (trace.fraction < 1 && !trace.allsolid && !trace.startsolid)
		{
			// check how deep the water is
			if (trace.contents & CONTENTS_WATER) 
			{
				trace_t deep = gi.traceline(trace.endpos, pt2, self, MASK_MONSTERSOLID);

				water_level_t waterlevel;
				contents_t watertype;
				M_CatagorizePosition(self, deep.endpos, waterlevel, watertype);

				if (waterlevel > WATER_WAIST)
					return blocked_jump_result_t::NO_JUMP;
			}

			if ((self->absmin[2] - trace.endpos[2]) >= 24 && (trace.contents & (MASK_SOLID | CONTENTS_WATER)))
			{
				if (self->monsterinfo.aiflags & AI_PATHING)
				{
					if ((self->monsterinfo.nav_path.secondMovePoint[2] - trace.endpos[2]) > 32)
						return blocked_jump_result_t::NO_JUMP;
				}
				else
				{
					if ((self->enemy->absmin[2] - trace.endpos[2]) > 32)
						return blocked_jump_result_t::NO_JUMP;

					if (trace.plane.normal[2] < 0.9f)
						return blocked_jump_result_t::NO_JUMP;
				}

				monster_jump_start(self);

				return blocked_jump_result_t::JUMP_JUMP_DOWN;
			}
		}
	}
	else if (playerPosition == 1 && self->monsterinfo.jump_height)
	{
		pt1 = self->s.origin + (forward * 48);
		pt2 = pt1;
		pt1[2] = self->absmax[2] + self->monsterinfo.jump_height;

		trace = gi.traceline(pt1, pt2, self, MASK_MONSTERSOLID | MASK_WATER);
		if (trace.fraction < 1 && !trace.allsolid && !trace.startsolid)
		{
			if ((trace.endpos[2] - self->absmin[2]) <= self->monsterinfo.jump_height && (trace.contents & (MASK_SOLID | CONTENTS_WATER)))
			{
				face_wall(self);

				monster_jump_start(self);

				return blocked_jump_result_t::JUMP_JUMP_UP;
			}
		}
	}

	return blocked_jump_result_t::NO_JUMP;
}

// *************************
// HINT PATHS
// *************************

constexpr spawnflags_t SPAWNFLAG_HINT_ENDPOINT = 0x0001_spawnflag;
constexpr size_t   MAX_HINT_CHAINS = 100;

int		 hint_paths_present;
edict_t *hint_path_start[MAX_HINT_CHAINS];
int		 num_hint_paths;

//
// AI code
//

// =============
// hintpath_findstart - given any hintpath node, finds the start node
// =============
edict_t *hintpath_findstart(edict_t *ent)
{
	edict_t *e;
	edict_t *last;

	if (ent->target) // starting point
	{
		last = world;
		e = G_FindByString<&edict_t::targetname>(nullptr, ent->target);
		while (e)
		{
			last = e;
			if (!e->target)
				break;
			e = G_FindByString<&edict_t::targetname>(nullptr, e->target);
		}
	}
	else // end point
	{
		last = world;
		e = G_FindByString<&edict_t::target>(nullptr, ent->targetname);
		while (e)
		{
			last = e;
			if (!e->targetname)
				break;
			e = G_FindByString<&edict_t::target>(nullptr, e->targetname);
		}
	}

	if (!last->spawnflags.has(SPAWNFLAG_HINT_ENDPOINT))
		return nullptr;

	if (last == world)
		last = nullptr;
	return last;
}

// =============
// hintpath_other_end - given one endpoint of a hintpath, returns the other end.
// =============
edict_t *hintpath_other_end(edict_t *ent)
{
	edict_t *e;
	edict_t *last;

	if (ent->target) // starting point
	{
		last = world;
		e = G_FindByString<&edict_t::targetname>(nullptr, ent->target);
		while (e)
		{
			last = e;
			if (!e->target)
				break;
			e = G_FindByString<&edict_t::targetname>(nullptr, e->target);
		}
	}
	else // end point
	{
		last = world;
		e = G_FindByString<&edict_t::target>(nullptr, ent->targetname);
		while (e)
		{
			last = e;
			if (!e->targetname)
				break;
			e = G_FindByString<&edict_t::target>(nullptr, e->targetname);
		}
	}

	if (!(last->spawnflags & SPAWNFLAG_HINT_ENDPOINT))
		return nullptr;

	if (last == world)
		last = nullptr;
	return last;
}

// =============
// hintpath_go - starts a monster (self) moving towards the hintpath (point)
//		disables all contrary AI flags.
// =============
void hintpath_go(edict_t *self, edict_t *point)
{
	vec3_t dir;

	dir = point->s.origin - self->s.origin;

	self->ideal_yaw = vectoyaw(dir);
	self->goalentity = self->movetarget = point;
	self->monsterinfo.pausetime = 0_ms;
	self->monsterinfo.aiflags |= AI_HINT_PATH;
	self->monsterinfo.aiflags &= ~(AI_SOUND_TARGET | AI_PURSUIT_LAST_SEEN | AI_PURSUE_NEXT | AI_PURSUE_TEMP);
	// run for it
	self->monsterinfo.search_time = level.time;
	self->monsterinfo.run(self);
}

// =============
// hintpath_stop - bails a monster out of following hint paths
// =============
void hintpath_stop(edict_t *self)
{
	self->goalentity = nullptr;
	self->movetarget = nullptr;
	self->monsterinfo.last_hint_time = level.time;
	self->monsterinfo.goal_hint = nullptr;
	self->monsterinfo.aiflags &= ~AI_HINT_PATH;
	if (has_valid_enemy(self))
	{
		// if we can see our target, go nuts
		if (visible(self, self->enemy))
		{
			FoundTarget(self);
			return;
		}
		// otherwise, keep chasing
		HuntTarget(self);
		return;
	}
	// if our enemy is no longer valid, forget about our enemy and go into stand
	self->enemy = nullptr;
	// we need the pausetime otherwise the stand code
	// will just revert to walking with no target and
	// the monsters will wonder around aimlessly trying
	// to hunt the world entity
	self->monsterinfo.pausetime = HOLD_FOREVER;
	self->monsterinfo.stand(self);
}

// =============
// monsterlost_checkhint - the monster (self) will check around for valid hintpaths.
//		a valid hintpath is one where the two endpoints can see both the monster
//		and the monster's enemy. if only one person is visible from the endpoints,
//		it will not go for it.
// =============
bool monsterlost_checkhint(edict_t *self)
{
	edict_t *e, *monster_pathchain, *target_pathchain, *checkpoint = nullptr;
	edict_t *closest;
	float	 closest_range = 1000000;
	edict_t *start, *destination;
	int		 count5 = 0;
	float	 r;
	int		 i;
	bool	 hint_path_represented[MAX_HINT_CHAINS];

	// if there are no hint paths on this map, exit immediately.
	if (!hint_paths_present)
		return false;

	if (!self->enemy)
		return false;

	// [Paril-KEX] don't do hint paths if we're using nav nodes
	if (self->monsterinfo.aiflags & (AI_STAND_GROUND | AI_PATHING))
		return false;

	if (!strcmp(self->classname, "monster_turret"))
		return false;

	monster_pathchain = nullptr;

	// find all the hint_paths.
	// FIXME - can we not do this every time?
	for (i = 0; i < num_hint_paths; i++)
	{
		e = hint_path_start[i];
		while (e)
		{
			if (e->monster_hint_chain)
				e->monster_hint_chain = nullptr;

			if (monster_pathchain)
			{
				checkpoint->monster_hint_chain = e;
				checkpoint = e;
			}
			else
			{
				monster_pathchain = e;
				checkpoint = e;
			}
			e = e->hint_chain;
		}
	}

	// filter them by distance and visibility to the monster
	e = monster_pathchain;
	checkpoint = nullptr;
	while (e)
	{
		r = realrange(self, e);

		if (r > 512)
		{
			if (checkpoint)
			{
				checkpoint->monster_hint_chain = e->monster_hint_chain;
				e->monster_hint_chain = nullptr;
				e = checkpoint->monster_hint_chain;
				continue;
			}
			else
			{
				// use checkpoint as temp pointer
				checkpoint = e;
				e = e->monster_hint_chain;
				checkpoint->monster_hint_chain = nullptr;
				// and clear it again
				checkpoint = nullptr;
				// since we have yet to find a valid one (or else checkpoint would be set) move the
				// start of monster_pathchain
				monster_pathchain = e;
				continue;
			}
		}
		if (!visible(self, e))
		{
			if (checkpoint)
			{
				checkpoint->monster_hint_chain = e->monster_hint_chain;
				e->monster_hint_chain = nullptr;
				e = checkpoint->monster_hint_chain;
				continue;
			}
			else
			{
				// use checkpoint as temp pointer
				checkpoint = e;
				e = e->monster_hint_chain;
				checkpoint->monster_hint_chain = nullptr;
				// and clear it again
				checkpoint = nullptr;
				// since we have yet to find a valid one (or else checkpoint would be set) move the
				// start of monster_pathchain
				monster_pathchain = e;
				continue;
			}
		}

		count5++;
		checkpoint = e;
		e = e->monster_hint_chain;
	}

	// at this point, we have a list of all of the eligible hint nodes for the monster
	// we now take them, figure out what hint chains they're on, and traverse down those chains,
	// seeing whether any can see the player
	//
	// first, we figure out which hint chains we have represented in monster_pathchain
	if (count5 == 0)
		return false;

	for (i = 0; i < num_hint_paths; i++)
		hint_path_represented[i] = false;

	e = monster_pathchain;
	checkpoint = nullptr;
	while (e)
	{
		if ((e->hint_chain_id < 0) || (e->hint_chain_id > num_hint_paths))
			return false;

		hint_path_represented[e->hint_chain_id] = true;
		e = e->monster_hint_chain;
	}

	count5 = 0;

	// now, build the target_pathchain which contains all of the hint_path nodes we need to check for
	// validity (within range, visibility)
	target_pathchain = nullptr;
	checkpoint = nullptr;
	for (i = 0; i < num_hint_paths; i++)
	{
		// if this hint chain is represented in the monster_hint_chain, add all of it's nodes to the target_pathchain
		// for validity checking
		if (hint_path_represented[i])
		{
			e = hint_path_start[i];
			while (e)
			{
				if (target_pathchain)
				{
					checkpoint->target_hint_chain = e;
					checkpoint = e;
				}
				else
				{
					target_pathchain = e;
					checkpoint = e;
				}
				e = e->hint_chain;
			}
		}
	}

	// target_pathchain is a list of all of the hint_path nodes we need to check for validity relative to the target
	e = target_pathchain;
	checkpoint = nullptr;
	while (e)
	{
		r = realrange(self->enemy, e);

		if (r > 512)
		{
			if (checkpoint)
			{
				checkpoint->target_hint_chain = e->target_hint_chain;
				e->target_hint_chain = nullptr;
				e = checkpoint->target_hint_chain;
				continue;
			}
			else
			{
				// use checkpoint as temp pointer
				checkpoint = e;
				e = e->target_hint_chain;
				checkpoint->target_hint_chain = nullptr;
				// and clear it again
				checkpoint = nullptr;
				target_pathchain = e;
				continue;
			}
		}
		if (!visible(self->enemy, e))
		{
			if (checkpoint)
			{
				checkpoint->target_hint_chain = e->target_hint_chain;
				e->target_hint_chain = nullptr;
				e = checkpoint->target_hint_chain;
				continue;
			}
			else
			{
				// use checkpoint as temp pointer
				checkpoint = e;
				e = e->target_hint_chain;
				checkpoint->target_hint_chain = nullptr;
				// and clear it again
				checkpoint = nullptr;
				target_pathchain = e;
				continue;
			}
		}

		count5++;
		checkpoint = e;
		e = e->target_hint_chain;
	}

	// at this point we should have:
	// monster_pathchain - a list of "monster valid" hint_path nodes linked together by monster_hint_chain
	// target_pathcain - a list of "target valid" hint_path nodes linked together by target_hint_chain.  these
	//                   are filtered such that only nodes which are on the same chain as "monster valid" nodes
	//
	// Now, we figure out which "monster valid" node we want to use
	//
	// To do this, we first off make sure we have some target nodes.  If we don't, there are no valid hint_path nodes
	// for us to take
	//
	// If we have some, we filter all of our "monster valid" nodes by which ones have "target valid" nodes on them
	//
	// Once this filter is finished, we select the closest "monster valid" node, and go to it.

	if (count5 == 0)
		return false;

	// reuse the hint_chain_represented array, this time to see which chains are represented by the target
	for (i = 0; i < num_hint_paths; i++)
		hint_path_represented[i] = false;

	e = target_pathchain;
	checkpoint = nullptr;
	while (e)
	{
		if ((e->hint_chain_id < 0) || (e->hint_chain_id > num_hint_paths))
			return false;

		hint_path_represented[e->hint_chain_id] = true;
		e = e->target_hint_chain;
	}

	// traverse the monster_pathchain - if the hint_node isn't represented in the "target valid" chain list,
	// remove it
	// if it is on the list, check it for range from the monster.  If the range is the closest, keep it
	//
	closest = nullptr;
	e = monster_pathchain;
	while (e)
	{
		if (!(hint_path_represented[e->hint_chain_id]))
		{
			checkpoint = e->monster_hint_chain;
			e->monster_hint_chain = nullptr;
			e = checkpoint;
			continue;
		}
		r = realrange(self, e);
		if (r < closest_range)
			closest = e;
		e = e->monster_hint_chain;
	}

	if (!closest)
		return false;

	start = closest;
	// now we know which one is the closest to the monster .. this is the one the monster will go to
	// we need to finally determine what the DESTINATION node is for the monster .. walk down the hint_chain,
	// and find the closest one to the player

	closest = nullptr;
	closest_range = 10000000;
	e = target_pathchain;
	while (e)
	{
		if (start->hint_chain_id == e->hint_chain_id)
		{
			r = realrange(self, e);
			if (r < closest_range)
				closest = e;
		}
		e = e->target_hint_chain;
	}

	if (!closest)
		return false;

	destination = closest;

	self->monsterinfo.goal_hint = destination;
	hintpath_go(self, start);

	return true;
}

//
// Path code
//

// =============
// hint_path_touch - someone's touched the hint_path
// =============
TOUCH(hint_path_touch) (edict_t *self, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
	edict_t *e, *goal, *next = nullptr;
	//	int			chain;			 // direction - (-1) = upstream, (1) = downstream, (0) = done
	bool goalFound = false;

	// make sure we're the target of it's obsession
	if (other->movetarget == self)
	{
		goal = other->monsterinfo.goal_hint;

		// if the monster is where he wants to be
		if (goal == self)
		{
			hintpath_stop(other);
			return;
		}
		else
		{
			// if we aren't, figure out which way we want to go
			e = hint_path_start[self->hint_chain_id];
			while (e)
			{
				// if we get up to ourselves on the hint chain, we're going down it
				if (e == self)
				{
					next = e->hint_chain;
					break;
				}
				if (e == goal)
					goalFound = true;
				// if we get to where the next link on the chain is this hint_path and have found the goal on the way
				// we're going upstream, so remember who the previous link is
				if ((e->hint_chain == self) && goalFound)
				{
					next = e;
					break;
				}
				e = e->hint_chain;
			}
		}

		// if we couldn't find it, have the monster go back to normal hunting.
		if (!next)
		{
			hintpath_stop(other);
			return;
		}

		// send him on his way
		hintpath_go(other, next);

		// have the monster freeze if the hint path we just touched has a wait time
		// on it, for example, when riding a plat.
		if (self->wait)
			other->nextthink = level.time + gtime_t::from_sec(self->wait);
	}
}

/*QUAKED hint_path (.5 .3 0) (-8 -8 -8) (8 8 8) END
Target: next hint path

END - set this flag on the endpoints of each hintpath.

"wait" - set this if you want the monster to freeze when they touch this hintpath
*/
void SP_hint_path(edict_t *self)
{
	if (deathmatch->integer)
	{
		G_FreeEdict(self);
		return;
	}

	if (!self->targetname && !self->target)
	{
		gi.Com_PrintFmt("{}: unlinked\n", *self);
		G_FreeEdict(self);
		return;
	}

	self->solid = SOLID_TRIGGER;
	self->touch = hint_path_touch;
	self->mins = { -8, -8, -8 };
	self->maxs = { 8, 8, 8 };
	self->svflags |= SVF_NOCLIENT;
	gi.linkentity(self);
}

// ============
// InitHintPaths - Called by InitGame (g_save) to enable quick exits if valid
// ============
void InitHintPaths()
{
	edict_t *e, *current;
	int		 i;

	hint_paths_present = 0;

	// check all the hint_paths.
	e = G_FindByString<&edict_t::classname>(nullptr, "hint_path");
	if (e)
		hint_paths_present = 1;
	else
		return;

	memset(hint_path_start, 0, MAX_HINT_CHAINS * sizeof(edict_t *));
	num_hint_paths = 0;
	while (e)
	{
		if (e->spawnflags.has(SPAWNFLAG_HINT_ENDPOINT))
		{
			if (e->target) // start point
			{
				if (e->targetname) // this is a bad end, ignore it
				{
					gi.Com_PrintFmt("{}: marked as endpoint with both target ({}) and targetname ({})\n",
						*e, e->target, e->targetname);
				}
				else
				{
					if (num_hint_paths >= MAX_HINT_CHAINS)
						break;

					hint_path_start[num_hint_paths++] = e;
				}
			}
		}
		e = G_FindByString<&edict_t::classname>(e, "hint_path");
	}

	for (i = 0; i < num_hint_paths; i++)
	{
		current = hint_path_start[i];
		current->hint_chain_id = i;
		e = G_FindByString<&edict_t::targetname>(nullptr, current->target);
		if (G_FindByString<&edict_t::targetname>(e, current->target))
		{
			gi.Com_PrintFmt("{}: Forked path detected for chain {}, target {}\n",
				*current, num_hint_paths, current->target);
			hint_path_start[i]->hint_chain = nullptr;
			continue;
		}
		while (e)
		{
			if (e->hint_chain)
			{
				gi.Com_PrintFmt("{}: Circular path detected for chain {}, targetname {}\n",
					*e, num_hint_paths, e->targetname);
				hint_path_start[i]->hint_chain = nullptr;
				break;
			}
			current->hint_chain = e;
			current = e;
			current->hint_chain_id = i;
			if (!current->target)
				break;
			e = G_FindByString<&edict_t::targetname>(nullptr, current->target);
			if (G_FindByString<&edict_t::targetname>(e, current->target))
			{
				gi.Com_PrintFmt("{}: Forked path detected for chain {}, target {}\n",
					*current, num_hint_paths, current->target);
				hint_path_start[i]->hint_chain = nullptr;
				break;
			}
		}
	}
}

// *****************************
//	MISCELLANEOUS STUFF
// *****************************

// PMM - inback
// use to see if opponent is behind you (not to side)
// if it looks a lot like infront, well, there's a reason

bool inback(edict_t *self, edict_t *other)
{
	vec3_t vec;
	float  dot;
	vec3_t forward;

	AngleVectors(self->s.angles, forward, nullptr, nullptr);
	vec = other->s.origin - self->s.origin;
	vec.normalize();
	dot = vec.dot(forward);
	return dot < -0.3f;
}

float realrange(edict_t *self, edict_t *other)
{
	vec3_t dir;

	dir = self->s.origin - other->s.origin;

	return dir.length();
}

bool face_wall(edict_t *self)
{
	vec3_t	pt;
	vec3_t	forward;
	vec3_t	ang;
	trace_t tr;

	AngleVectors(self->s.angles, forward, nullptr, nullptr);
	pt = self->s.origin + (forward * 64);
	tr = gi.traceline(self->s.origin, pt, self, MASK_MONSTERSOLID);
	if (tr.fraction < 1 && !tr.allsolid && !tr.startsolid)
	{
		ang = vectoangles(tr.plane.normal);
		self->ideal_yaw = ang[YAW] + 180;
		if (self->ideal_yaw > 360)
			self->ideal_yaw -= 360;

		M_ChangeYaw(self);
		return true;
	}

	return false;
}

//
// Monster "Bad" Areas
//

TOUCH(badarea_touch) (edict_t *ent, edict_t *other, const trace_t &tr, bool other_touching_self) -> void
{
}

edict_t *SpawnBadArea(const vec3_t &mins, const vec3_t &maxs, gtime_t lifespan, edict_t *owner)
{
	edict_t *badarea;
	vec3_t	 origin;

	origin = mins + maxs;
	origin *= 0.5f;

	badarea = G_Spawn();
	badarea->s.origin = origin;

	badarea->maxs = maxs - origin;
	badarea->mins = mins - origin;
	badarea->touch = badarea_touch;
	badarea->movetype = MOVETYPE_NONE;
	badarea->solid = SOLID_TRIGGER;
	badarea->classname = "bad_area";
	gi.linkentity(badarea);

	if (lifespan)
	{
		badarea->think = G_FreeEdict;
		badarea->nextthink = level.time + lifespan;
	}
	if (owner)
	{
		badarea->owner = owner;
	}

	return badarea;
}

static BoxEdictsResult_t CheckForBadArea_BoxFilter(edict_t *hit, void *data)
{
	edict_t *&result = (edict_t *&) data;

	if (hit->touch == badarea_touch)
	{
		result = hit;
		return BoxEdictsResult_t::End;
	}

	return BoxEdictsResult_t::Skip;
}

// CheckForBadArea
//		This is a customized version of G_TouchTriggers that will check
//		for bad area triggers and return them if they're touched.
edict_t *CheckForBadArea(edict_t *ent)
{
	vec3_t	 mins, maxs;

	mins = ent->s.origin + ent->mins;
	maxs = ent->s.origin + ent->maxs;

	edict_t *hit = nullptr;

	gi.BoxEdicts(mins, maxs, nullptr, 0, AREA_TRIGGERS, CheckForBadArea_BoxFilter, &hit);

	return hit;
}

constexpr float TESLA_DAMAGE_RADIUS = 128;

bool MarkTeslaArea(edict_t *self, edict_t *tesla)
{
	vec3_t	 mins, maxs;
	edict_t *e;
	edict_t *tail;
	edict_t *area;

	if (!tesla || !self)
		return false;

	area = nullptr;

	// make sure this tesla doesn't have a bad area around it already...
	e = tesla->teamchain;
	tail = tesla;
	while (e)
	{
		tail = tail->teamchain;
		if (!strcmp(e->classname, "bad_area"))
			return false;

		e = e->teamchain;
	}

	// see if we can grab the trigger directly
	if (tesla->teamchain && tesla->teamchain->inuse)
	{
		edict_t *trigger;

		trigger = tesla->teamchain;

		mins = trigger->absmin;
		maxs = trigger->absmax;

		if (tesla->air_finished)
			area = SpawnBadArea(mins, maxs, tesla->air_finished, tesla);
		else
			area = SpawnBadArea(mins, maxs, tesla->nextthink, tesla);
	}
	// otherwise we just guess at how long it'll last.
	else
	{

		mins = { -TESLA_DAMAGE_RADIUS, -TESLA_DAMAGE_RADIUS, tesla->mins[2] };
		maxs = { TESLA_DAMAGE_RADIUS, TESLA_DAMAGE_RADIUS, TESLA_DAMAGE_RADIUS };

		area = SpawnBadArea(mins, maxs, 30_sec, tesla);
	}

	// if we spawned a bad area, then link it to the tesla
	if (area)
		tail->teamchain = area;

	return true;
}

// predictive calculator
// target is who you want to shoot
// start is where the shot comes from
// bolt_speed is how fast the shot is (or 0 for hitscan)
// eye_height is a boolean to say whether or not to adjust to targets eye_height
// offset is how much time to miss by
// aimdir is the resulting aim direction (pass in nullptr if you don't want it)
// aimpoint is the resulting aimpoint (pass in nullptr if don't want it)
void PredictAim(edict_t *self, edict_t *target, const vec3_t &start, float bolt_speed, bool eye_height, float offset, vec3_t *aimdir, vec3_t *aimpoint)
{
	vec3_t dir, vec;
	float  dist, time;

	if (!target || !target->inuse)
	{
		*aimdir = {};
		return;
	}

	dir = target->s.origin - start;
	if (eye_height)
		dir[2] += target->viewheight;
	dist = dir.length();

	// [Paril-KEX] if our current attempt is blocked, try the opposite one
	trace_t tr = gi.traceline(start, start + dir, self, MASK_PROJECTILE);

	if (tr.ent != target)
	{
		eye_height = !eye_height;
		dir = target->s.origin - start;
		if (eye_height)
			dir[2] += target->viewheight;
		dist = dir.length();
	}

	if (bolt_speed)
		time = dist / bolt_speed;
	else
		time = 0;

	vec = target->s.origin + (target->velocity * (time - offset));

	// went backwards...
	if (dir.normalized().dot((vec - start).normalized()) < 0)
		vec = target->s.origin;
	else
	{
		// if the shot is going to impact a nearby wall from our prediction, just fire it straight.	
		if (gi.traceline(start, vec, nullptr, MASK_SOLID).fraction < 0.9f)
			vec = target->s.origin;
	}

	if (eye_height)
		vec[2] += target->viewheight;

	if (aimdir)
		*aimdir = (vec - start).normalized();

	if (aimpoint)
		*aimpoint = vec;
}

// [Paril-KEX] find a pitch that will at some point land on or near the player.
// very approximate. aim will be adjusted to the correct aim vector.
bool M_CalculatePitchToFire(edict_t *self, const vec3_t &target, const vec3_t &start, vec3_t &aim, float speed, float time_remaining, bool mortar, bool destroy_on_touch)
{
	constexpr float pitches[] = { -80.f, -70.f, -60.f, -50.f, -40.f, -30.f, -20.f, -10.f, -5.f };
	float best_pitch = 0.f;
	float best_dist = std::numeric_limits<float>::infinity();

	constexpr float sim_time = 0.1f;
	vec3_t pitched_aim = vectoangles(aim);

	for (auto &pitch : pitches)
	{
		if (mortar && pitch >= -30.f)
			break;

		pitched_aim[PITCH] = pitch;
		vec3_t fwd = AngleVectors(pitched_aim).forward;

		vec3_t velocity = fwd * speed;
		vec3_t origin = start;

		float t = time_remaining;

		while (t > 0.f)
		{
			velocity += vec3_t{ 0, 0, -1 } * level.gravity * sim_time;

			vec3_t end = origin + (velocity * sim_time);
			trace_t tr = gi.traceline(origin, end, nullptr, MASK_SHOT);

			origin = tr.endpos;

			if (tr.fraction < 1.0f)
			{
				if (tr.surface->flags & SURF_SKY)
					break;

				origin += tr.plane.normal;
				velocity = ClipVelocity(velocity, tr.plane.normal, 1.6f);

				float dist = (origin - target).lengthSquared();

				if (tr.ent == self->enemy || tr.ent->client || (tr.plane.normal.z >= 0.7f && dist < (128.f * 128.f) && dist < best_dist))
				{
					best_pitch = pitch;
					best_dist = dist;
				}

				if (destroy_on_touch || (tr.contents & (CONTENTS_MONSTER | CONTENTS_PLAYER | CONTENTS_DEADMONSTER)))
					break;
			}

			t -= sim_time;
		}
	}

	if (!isinf(best_dist))
	{
		pitched_aim[PITCH] = best_pitch;
		aim = AngleVectors(pitched_aim).forward;
		return true;
	}

	return false;
}

bool below(edict_t *self, edict_t *other)
{
	vec3_t vec;
	float  dot;
	vec3_t down;

	vec = other->s.origin - self->s.origin;
	vec.normalize();
	down = { 0, 0, -1 };
	dot = vec.dot(down);

	if (dot > 0.95f) // 18 degree arc below
		return true;
	return false;
}

void drawbbox(edict_t *self)
{
	int lines[4][3] = {
		{ 1, 2, 4 },
		{ 1, 2, 7 },
		{ 1, 4, 5 },
		{ 2, 4, 7 }
	};

	int starts[4] = { 0, 3, 5, 6 };

	vec3_t pt[8];
	int	   i, j, k;
	vec3_t coords[2];
	vec3_t newbox;
	vec3_t f, r, u, dir;

	coords[0] = self->absmin;
	coords[1] = self->absmax;

	for (i = 0; i <= 1; i++)
	{
		for (j = 0; j <= 1; j++)
		{
			for (k = 0; k <= 1; k++)
			{
				pt[4 * i + 2 * j + k][0] = coords[i][0];
				pt[4 * i + 2 * j + k][1] = coords[j][1];
				pt[4 * i + 2 * j + k][2] = coords[k][2];
			}
		}
	}

	for (i = 0; i <= 3; i++)
	{
		for (j = 0; j <= 2; j++)
		{
			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_DEBUGTRAIL);
			gi.WritePosition(pt[starts[i]]);
			gi.WritePosition(pt[lines[i][j]]);
			gi.multicast(pt[starts[i]], MULTICAST_ALL, false);
		}
	}

	dir = vectoangles(self->s.angles);
	AngleVectors(dir, f, r, u);

	newbox = self->s.origin + (f * 50);
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_DEBUGTRAIL);
	gi.WritePosition(self->s.origin);
	gi.WritePosition(newbox);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);

	newbox = self->s.origin + (r * 50);
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_DEBUGTRAIL);
	gi.WritePosition(self->s.origin);
	gi.WritePosition(newbox);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);

	newbox = self->s.origin + (u * 50);
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_DEBUGTRAIL);
	gi.WritePosition(self->s.origin);
	gi.WritePosition(newbox);
	gi.multicast(self->s.origin, MULTICAST_PVS, false);
}

// [Paril-KEX] returns true if the skill check passes
inline bool G_SkillCheck(const std::initializer_list<float> &skills)
{
	if (skills.size() < skill->integer)
		return true;

	const float &skill_switch = *(skills.begin() + skill->integer);
	return skill_switch == 1.0f ? true : frandom() < skill_switch;
}

//
// New dodge code
//
MONSTERINFO_DODGE(M_MonsterDodge) (edict_t *self, edict_t *attacker, gtime_t eta, trace_t *tr, bool gravity) -> void
{
	float r = frandom();
	float height;
	bool  ducker = false, dodger = false;

	// this needs to be here since this can be called after the monster has "died"
	if (self->health < 1)
		return;

	if ((self->monsterinfo.duck) && (self->monsterinfo.unduck) && !gravity)
		ducker = true;
	if ((self->monsterinfo.sidestep) && !(self->monsterinfo.aiflags & AI_STAND_GROUND))
		dodger = true;

	if ((!ducker) && (!dodger))
		return;

	if (!self->enemy)
	{
		self->enemy = attacker;
		FoundTarget(self);
	}

	// PMM - don't bother if it's going to hit anyway; fix for weird in-your-face etas (I was
	// seeing numbers like 13 and 14)
	if ((eta < FRAME_TIME_MS) || (eta > 2.5_sec))
		return;

	// skill level determination..
	if (r > 0.50f)
		return;

	if (ducker && tr)
	{
		height = self->absmax[2] - 32 - 1; // the -1 is because the absmax is s.origin + maxs + 1

		if ((!dodger) && ((tr->endpos[2] <= height) || (self->monsterinfo.aiflags & AI_DUCKED)))
			return;
	}
	else
		height = self->absmax[2];

	if (dodger)
	{
		// if we're already dodging, just finish the sequence, i.e. don't do anything else
		if (self->monsterinfo.aiflags & AI_DODGING)
			return;

		// if we're ducking already, or the shot is at our knees
		if ((!ducker || !tr || tr->endpos[2] <= height) || (self->monsterinfo.aiflags & AI_DUCKED))
		{
			// on Easy & Normal, don't sidestep as often (25% on Easy, 50% on Normal)
			if (!G_SkillCheck({ 0.25f, 0.50f, 1.0f, 1.0f }))
			{
				self->monsterinfo.dodge_time = level.time + random_time(0.8_sec, 1.4_sec);
				return;
			}
			else
			{
				if (tr)
				{
					vec3_t right, diff;

					AngleVectors(self->s.angles, nullptr, right, nullptr);
					diff = tr->endpos - self->s.origin;

					if (right.dot(diff) < 0)
						self->monsterinfo.lefty = false;
					else
						self->monsterinfo.lefty = true;
				}
				else
					self->monsterinfo.lefty = brandom();

				// call the monster specific code here
				if (self->monsterinfo.sidestep(self))
				{
					// if we are currently ducked, unduck
					if ((ducker) && (self->monsterinfo.aiflags & AI_DUCKED))
						self->monsterinfo.unduck(self);

					self->monsterinfo.aiflags |= AI_DODGING;
					self->monsterinfo.attack_state = AS_SLIDING;

					self->monsterinfo.dodge_time = level.time + random_time(0.4_sec, 2.0_sec);
				}
				return;
			}
		}
	}

	// [Paril-KEX] we don't need to duck until projectiles are going to hit us very
	// soon.
	if (ducker && tr && eta < 0.5_sec)
	{
		if (self->monsterinfo.next_duck_time > level.time)
			return;

		monster_done_dodge(self);

		if (self->monsterinfo.duck(self, eta))
		{
			// if duck didn't set us yet, do it now
			if (self->monsterinfo.duck_wait_time < level.time)
				self->monsterinfo.duck_wait_time = level.time + eta;

			monster_duck_down(self);

			// on Easy & Normal mode, duck longer
			if (skill->integer == 0)
				self->monsterinfo.duck_wait_time += random_time(500_ms, 1000_ms);
			else if (skill->integer == 1)
				self->monsterinfo.duck_wait_time += random_time(100_ms, 350_ms);
		}

		self->monsterinfo.dodge_time = level.time + random_time(0.2_sec, 0.7_sec);
	}
}

void monster_duck_down(edict_t *self)
{
	self->monsterinfo.aiflags |= AI_DUCKED;

	self->maxs[2] = self->monsterinfo.base_height - 32;
	self->takedamage = true;
	self->monsterinfo.next_duck_time = level.time + DUCK_INTERVAL;
	gi.linkentity(self);
}

void monster_duck_hold(edict_t *self)
{
	if (level.time >= self->monsterinfo.duck_wait_time)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

MONSTERINFO_UNDUCK(monster_duck_up) (edict_t *self) -> void
{
	if (!(self->monsterinfo.aiflags & AI_DUCKED))
		return;

	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->maxs[2] = self->monsterinfo.base_height;
	self->takedamage = true;
	// we finished a duck-up successfully, so cut the time remaining in half
	if (self->monsterinfo.next_duck_time > level.time)
		self->monsterinfo.next_duck_time = level.time + ((self->monsterinfo.next_duck_time - level.time) / 2);
	gi.linkentity(self);
}

//=========================
//=========================
bool has_valid_enemy(edict_t *self)
{
	if (!self->enemy)
		return false;

	if (!self->enemy->inuse)
		return false;

	if (self->enemy->health < 1)
		return false;

	return true;
}

void TargetTesla(edict_t *self, edict_t *tesla)
{
	if ((!self) || (!tesla))
		return;

	// PMM - medic bails on healing things
	if (self->monsterinfo.aiflags & AI_MEDIC)
	{
		if (self->enemy)
			cleanupHealTarget(self->enemy);
		self->monsterinfo.aiflags &= ~AI_MEDIC;
	}

	// store the player enemy in case we lose track of him.
	if (self->enemy && self->enemy->client)
		self->monsterinfo.last_player_enemy = self->enemy;

	if (self->enemy != tesla)
	{
		self->oldenemy = self->enemy;
		self->enemy = tesla;
		if (self->monsterinfo.attack)
		{
			if (self->health <= 0)
				return;

			self->monsterinfo.attack(self);
		}
		else
			FoundTarget(self);
	}
}

// this returns a randomly selected coop player who is visible to self
// returns nullptr if bad

edict_t *PickCoopTarget(edict_t *self)
{
	edict_t **targets;
	int		 num_targets = 0, targetID;
	edict_t *ent;

	// if we're not in coop, this is a noop
	if (!coop->integer)
		return nullptr;

	targets = (edict_t **) alloca(sizeof(edict_t *) * game.maxclients);

	for (uint32_t player = 1; player <= game.maxclients; player++)
	{
		ent = &g_edicts[player];
		if (!ent->inuse)
			continue;
		if (!ent->client)
			continue;
		if (visible(self, ent))
			targets[num_targets++] = ent;
	}

	if (!num_targets)
		return nullptr;

	// get a number from 0 to (num_targets-1)
	targetID = irandom(num_targets);

	return targets[targetID];
}

// only meant to be used in coop
int CountPlayers()
{
	edict_t *ent;
	int		 count = 0;

	// if we're not in coop, this is a noop
	if (!coop->integer)
		return 1;

	for (uint32_t player = 1; player <= game.maxclients; player++)
	{
		ent = &g_edicts[player];
		if (!ent->inuse)
			continue;
		if (!ent->client)
			continue;
		count++;
	}
	/*
		ent = g_edicts+1; // skip the worldspawn
		while (ent)
		{
			if ((ent->client) && (ent->inuse))
			{
				ent++;
				count++;
			}
			else
				ent = nullptr;
		}
	*/
	return count;
}

THINK(BossExplode_think) (edict_t *self) -> void
{
	// owner gone or changed
	if (!self->owner->inuse || self->owner->s.modelindex != self->style || self->count != self->owner->spawn_count)
	{
		G_FreeEdict(self);
		return;
	}

	vec3_t org = self->owner->s.origin + self->owner->mins;
	
	org.x += frandom() * self->owner->size.x;
	org.y += frandom() * self->owner->size.y;
	org.z += frandom() * self->owner->size.z;

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(!(self->viewheight % 3) ? TE_EXPLOSION1 : TE_EXPLOSION1_NL);
	gi.WritePosition(org);
	gi.multicast(org, MULTICAST_PVS, false);

	self->viewheight++;

	self->nextthink = level.time + random_time(50_ms, 200_ms);
}

void BossExplode(edict_t *self)
{
	// no blowy on deady
	if (self->spawnflags.has(SPAWNFLAG_MONSTER_DEAD))
		return;

	edict_t *exploder = G_Spawn();
	exploder->owner = self;
	exploder->count = self->spawn_count;
	exploder->style = self->s.modelindex;
	exploder->think = BossExplode_think;
	exploder->nextthink = level.time + random_time(75_ms, 250_ms);
	exploder->viewheight = 0;
}