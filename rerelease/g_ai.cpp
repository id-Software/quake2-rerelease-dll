// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
// g_ai.c

#include "g_local.h"

bool FindTarget(edict_t *self);
bool ai_checkattack(edict_t *self, float dist);

bool    enemy_vis;
bool    enemy_infront;
float   enemy_yaw;

// ROGUE
constexpr float MAX_SIDESTEP = 8.0f;
// ROGUE

//============================================================================

/*
=================
AI_GetSightClient

For a given monster, check active players to see
who we can see. We don't care who we see, as long
as it's something we can shoot.
=================
*/
edict_t *AI_GetSightClient(edict_t *self)
{
    if (level.intermissiontime)
        return nullptr;

    edict_t **visible_players = (edict_t **) alloca(sizeof(edict_t *) * game.maxclients);
    size_t num_visible = 0;

    for (auto player : active_players())
    {
        if (player->health <= 0 || player->deadflag || !player->solid)
            continue;
        else if (player->flags & (FL_NOTARGET | FL_DISGUISED))
            continue;

        // if we're touching them, allow to pass through
        if (!boxes_intersect(self->absmin, self->absmax, player->absmin, player->absmax))
        {
            if ((!(self->monsterinfo.aiflags & AI_THIRD_EYE) && !infront(self, player)) || !visible(self, player))
                continue;
        }

        visible_players[num_visible++] = player; // got one
    }

    if (!num_visible)
        return nullptr;

    return visible_players[irandom(num_visible)];
}

//============================================================================

/*
=============
ai_move

Move the specified distance at current facing.
This replaces the QC functions: ai_forward, ai_back, ai_pain, and ai_painforward
==============
*/
void ai_move(edict_t *self, float dist)
{
    M_walkmove(self, self->s.angles[YAW], dist);
}

/*
=============
ai_stand

Used for standing around and looking for players
Distance is for slight position adjustments needed by the animations
==============
*/
void ai_stand(edict_t *self, float dist)
{
    vec3_t v;
    // ROGUE
    bool retval;
    // ROGUE

    if (dist || (self->monsterinfo.aiflags & AI_ALTERNATE_FLY))
        M_walkmove(self, self->s.angles[YAW], dist);

    if (self->monsterinfo.aiflags & AI_STAND_GROUND)
    {
        // [Paril-KEX] check if we've been pushed out of our point_combat
        if (!(self->monsterinfo.aiflags & AI_TEMP_STAND_GROUND) &&
            self->movetarget && self->movetarget->classname && !strcmp(self->movetarget->classname, "point_combat"))
        {
            if (!boxes_intersect(self->absmin, self->absmax, self->movetarget->absmin, self->movetarget->absmax))
            {
                self->monsterinfo.aiflags &= ~AI_STAND_GROUND;
                self->monsterinfo.aiflags |= AI_COMBAT_POINT;
                self->goalentity = self->movetarget;
                self->monsterinfo.run(self);
                return;
            }
        }

        if (self->enemy && !(self->enemy->classname && !strcmp(self->enemy->classname, "player_noise")))
        {
            v = self->enemy->s.origin - self->s.origin;
            self->ideal_yaw = vectoyaw(v);
            if (!FacingIdeal(self) && (self->monsterinfo.aiflags & AI_TEMP_STAND_GROUND))
            {
                self->monsterinfo.aiflags &= ~(AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
                self->monsterinfo.run(self);
            }
            // ROGUE
            if (!(self->monsterinfo.aiflags & AI_MANUAL_STEERING))
                // ROGUE
                M_ChangeYaw(self);
            // find out if we're going to be shooting
            retval = ai_checkattack(self, 0);
            // record sightings of player
            if ((self->enemy) && (self->enemy->inuse))
            {
                if (visible(self, self->enemy))
                {
                    self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
                    self->monsterinfo.last_sighting = self->monsterinfo.saved_goal = self->enemy->s.origin;
                    self->monsterinfo.blind_fire_target = self->monsterinfo.last_sighting + (self->enemy->velocity * -0.1f);
                    self->monsterinfo.trail_time = level.time;
                    self->monsterinfo.blind_fire_delay = 0_ms;
                }
                else
                {
                    if (FindTarget(self))
                        return;
                    
                    self->monsterinfo.aiflags |= AI_LOST_SIGHT;
                }

                // Paril: fixes rare cases of a stand ground monster being stuck
                // aiming at a sound target that they can still see
                if ((self->monsterinfo.aiflags & AI_SOUND_TARGET) && !retval)
                {
                    if (FindTarget(self))
                        return;
                }
            }
            // check retval to make sure we're not blindfiring
            else if (!retval)
            {
                FindTarget(self);
                return;
            }
            // ROGUE
        }
        else
            FindTarget(self);
        return;
    }

    // Paril: this fixes a bug somewhere else that sometimes causes
    // a monster to be given an enemy without ever calling HuntTarget.
    if (self->enemy && !(self->monsterinfo.aiflags & AI_SOUND_TARGET))
    {
        HuntTarget(self);
        return;
    }

    if (FindTarget(self))
        return;

    if (level.time > self->monsterinfo.pausetime)
    {
        self->monsterinfo.walk(self);
        return;
    }

    if (!(self->spawnflags & SPAWNFLAG_MONSTER_AMBUSH) && (self->monsterinfo.idle) &&
        (level.time > self->monsterinfo.idle_time))
    {
        if (self->monsterinfo.idle_time)
        {
            self->monsterinfo.idle(self);
            self->monsterinfo.idle_time = level.time + random_time(15_sec, 30_sec);
        }
        else
        {
            self->monsterinfo.idle_time = level.time + random_time(15_sec);
        }
    }
}

/*
=============
ai_walk

The monster is walking it's beat
=============
*/
void ai_walk(edict_t *self, float dist)
{
    edict_t *temp_goal = nullptr;

    if (!self->goalentity && (self->monsterinfo.aiflags & AI_GOOD_GUY))
    {
        vec3_t fwd;
        AngleVectors(self->s.angles, fwd, nullptr, nullptr);

        temp_goal = G_Spawn();
        temp_goal->s.origin = self->s.origin + fwd * 64;
        self->goalentity = temp_goal;
    }

    M_MoveToGoal(self, dist);

    if (temp_goal)
    {
        G_FreeEdict(temp_goal);
        self->goalentity = nullptr;
    }

    // check for noticing a player
    if (FindTarget(self))
        return;

    if ((self->monsterinfo.search) && (level.time > self->monsterinfo.idle_time))
    {
        if (self->monsterinfo.idle_time)
        {
            self->monsterinfo.search(self);
            self->monsterinfo.idle_time = level.time + random_time(15_sec, 30_sec);
        }
        else
        {
            self->monsterinfo.idle_time = level.time + random_time(15_sec);
        }
    }
}

/*
=============
ai_charge

Turns towards target and advances
Use this call with a distance of 0 to replace ai_face
==============
*/
void ai_charge(edict_t *self, float dist)
{
    vec3_t v;
    // ROGUE
    float ofs;

    // PMM - made AI_MANUAL_STEERING affect things differently here .. they turn, but
    // don't set the ideal_yaw

    // This is put in there so monsters won't move towards the origin after killing
    // a tesla. This could be problematic, so keep an eye on it.
    if (!self->enemy || !self->enemy->inuse) // PGM
        return;                              // PGM

    // PMM - save blindfire target
    if (visible(self, self->enemy))
        self->monsterinfo.blind_fire_target = self->enemy->s.origin + (self->enemy->velocity * -0.1f);
    // pmm

    if (!(self->monsterinfo.aiflags & AI_MANUAL_STEERING))
    {
        // ROGUE
        v = self->enemy->s.origin - self->s.origin;
        self->ideal_yaw = vectoyaw(v);
        // ROGUE
    }
    // ROGUE
    M_ChangeYaw(self);

    if (dist || (self->monsterinfo.aiflags & AI_ALTERNATE_FLY))
    // ROGUE
    {
        if (self->monsterinfo.aiflags & AI_CHARGING)
        {
            M_MoveToGoal(self, dist);
            return;
        }
        // circle strafe support
        if (self->monsterinfo.attack_state == AS_SLIDING)
        {
            // if we're fighting a tesla, NEVER circle strafe
            if ((self->enemy) && (self->enemy->classname) && (!strcmp(self->enemy->classname, "tesla_mine")))
                ofs = 0;
            else if (self->monsterinfo.lefty)
                ofs = 90;
            else
                ofs = -90;

            dist *= self->monsterinfo.active_move->sidestep_scale;

            if (M_walkmove(self, self->ideal_yaw + ofs, dist))
                return;

            self->monsterinfo.lefty = !self->monsterinfo.lefty;
            M_walkmove(self, self->ideal_yaw - ofs, dist);
        }
        else
            // ROGUE
            M_walkmove(self, self->s.angles[YAW], dist);
        // ROGUE
    }
    // ROGUE

    // [Paril-KEX] if our enemy is literally right next to us, give
    // us more rotational speed so we don't get circled
    if (range_to(self, self->enemy) <= RANGE_MELEE * 2.5f)
        M_ChangeYaw(self);
}

/*
=============
ai_turn

don't move, but turn towards ideal_yaw
Distance is for slight position adjustments needed by the animations
=============
*/
void ai_turn(edict_t *self, float dist)
{
    if (dist || (self->monsterinfo.aiflags & AI_ALTERNATE_FLY))
        M_walkmove(self, self->s.angles[YAW], dist);

    if (FindTarget(self))
        return;

    // ROGUE
    if (!(self->monsterinfo.aiflags & AI_MANUAL_STEERING))
        // ROGUE
        M_ChangeYaw(self);
}

/*

.enemy
Will be world if not currently angry at anyone.

.movetarget
The next path spot to walk toward.  If .enemy, ignore .movetarget.
When an enemy is killed, the monster will try to return to it's path.

.hunt_time
Set to time + something when the player is in sight, but movement straight for
him is blocked.  This causes the monster to use wall following code for
movement direction instead of sighting on the player.

.ideal_yaw
A yaw angle of the intended direction, which will be turned towards at up
to 45 deg / state.  If the enemy is in view and hunt_time is not active,
this will be the exact line towards the enemy.

.pausetime
A monster will leave it's stand state and head towards it's .movetarget when
time > .pausetime.

walkmove(angle, speed) primitive is all or nothing
*/

/*
=============
range_to

returns the distance of an entity relative to self.
in general, the results determine how an AI reacts:
melee	melee range, will become hostile even if back is turned
near	visibility and infront, or visibility and show hostile
mid	    infront and show hostile
> mid	only triggered by damage
=============
*/
float range_to(edict_t *self, edict_t *other)
{
    return distance_between_boxes(self->absmin, self->absmax, other->absmin, other->absmax);
}

/*
=============
visible

returns 1 if the entity is visible to self, even if not infront ()
=============
*/
bool visible(edict_t *self, edict_t *other, bool through_glass)
{
    // never visible
    if (other->flags & FL_NOVISIBLE)
        return false;

    // [Paril-KEX] bit of a hack, but we'll tweak monster-player visibility
    // if they have the invisibility powerup.
    if (other->client)
    {
        // always visible in rtest
        if (self->hackflags & HACKFLAG_ATTACK_PLAYER)
            return self->inuse;

        // fix intermission
        if (!other->solid)
            return false;

        if (other->client->invisible_time > level.time)
        {
            // can't see us at all after this time
            if (other->client->invisibility_fade_time <= level.time)
                return false;

            // otherwise, throw in some randomness
            if (frandom() > other->s.alpha)
                return false;
        }
    }

    vec3_t  spot1;
    vec3_t  spot2;
    trace_t trace;

    spot1 = self->s.origin;
    spot1[2] += self->viewheight;
    spot2 = other->s.origin;
    spot2[2] += other->viewheight;

    contents_t mask = MASK_OPAQUE;

    if (!through_glass)
        mask |= CONTENTS_WINDOW;

    trace = gi.traceline(spot1, spot2, self, mask);
    return trace.fraction == 1.0f || trace.ent == other; // PGM
}

/*
=============
infront

returns 1 if the entity is in front (in sight) of self
=============
*/
bool infront(edict_t *self, edict_t *other)
{
    vec3_t vec;
    float  dot;
    vec3_t forward;

    AngleVectors(self->s.angles, forward, nullptr, nullptr);
    vec = other->s.origin - self->s.origin;
    vec.normalize();
    dot = vec.dot(forward);

    // [Paril-KEX] if we're an ambush monster, reduce our cone of
    // vision to not ruin surprises, unless we already had an enemy.
    if (self->spawnflags.has(SPAWNFLAG_MONSTER_AMBUSH) && !self->monsterinfo.trail_time && !self->enemy)
        return dot > 0.15f;

    return dot > -0.30f;
}

//============================================================================

void HuntTarget(edict_t *self, bool animate_state)
{
    vec3_t vec;

    self->goalentity = self->enemy;
    if (animate_state)
    {
        if (self->monsterinfo.aiflags & AI_STAND_GROUND)
            self->monsterinfo.stand(self);
        else
            self->monsterinfo.run(self);
    }
    vec = self->enemy->s.origin - self->s.origin;
    self->ideal_yaw = vectoyaw(vec);
}

void FoundTarget(edict_t *self)
{
    // let other monsters see this monster for a while
    if (self->enemy->client)
    {
        // ROGUE
        if (self->enemy->flags & FL_DISGUISED)
            self->enemy->flags &= ~FL_DISGUISED;
        // ROGUE

        self->enemy->client->sight_entity = self;
        self->enemy->client->sight_entity_time = level.time;

        self->enemy->show_hostile = level.time + 1_sec; // wake up other monsters
    }

    // [Paril-KEX] the first time we spot something, give us a bit of a grace
    // period on firing
    if (!self->monsterinfo.trail_time)
        self->monsterinfo.attack_finished = level.time + 600_ms;

    // give easy/medium a little more reaction time
    self->monsterinfo.attack_finished += skill->integer == 0 ? 400_ms : skill->integer == 1 ? 200_ms : 0_ms;

    self->monsterinfo.last_sighting = self->monsterinfo.saved_goal = self->enemy->s.origin;
    self->monsterinfo.trail_time = level.time;
    // ROGUE
    self->monsterinfo.blind_fire_target = self->monsterinfo.last_sighting + (self->enemy->velocity * -0.1f);
    self->monsterinfo.blind_fire_delay = 0_ms;
    // ROGUE
    // [Paril-KEX] for alternate fly, pick a new position immediately
    self->monsterinfo.fly_position_time = 0_ms;

    self->monsterinfo.aiflags &= ~AI_THIRD_EYE;

    // Paril: if we're heading to a combat point/path corner, don't
    // hunt the new target yet.
    if (self->monsterinfo.aiflags & AI_COMBAT_POINT)
        return;

    if (!self->combattarget)
    {
        HuntTarget(self);
        return;
    }

    self->goalentity = self->movetarget = G_PickTarget(self->combattarget);
    if (!self->movetarget)
    {
        self->goalentity = self->movetarget = self->enemy;
        HuntTarget(self);
        gi.Com_PrintFmt("{}: combattarget {} not found\n", *self, self->combattarget);
        return;
    }

    // clear out our combattarget, these are a one shot deal
    self->combattarget = nullptr;
    self->monsterinfo.aiflags |= AI_COMBAT_POINT;

    // clear the targetname, that point is ours!
    // [Paril-KEX] not any more, we can re-use them
    //self->movetarget->targetname = nullptr;
    self->monsterinfo.pausetime = 0_ms;

    // run for it
    self->monsterinfo.run(self);
}

// [Paril-KEX] monsters that were alerted by players will
// be temporarily stored on player entities, so we can
// check them & get mad at them even around corners
static edict_t *AI_GetMonsterAlertedByPlayers(edict_t *self)
{
    for (auto player : active_players())
    {
        // dead
        if (player->health <= 0 || player->deadflag || !player->solid)
            continue;

        // we didn't alert any other monster, or it wasn't recently
        if (!player->client->sight_entity || !(player->client->sight_entity_time >= (level.time - FRAME_TIME_S)))
            continue;

        // if we can't see the monster, don't bother
        if (!visible(self, player->client->sight_entity))
            continue;

        // probably good
        return player->client->sight_entity;
    }
    
    return nullptr;
}

// [Paril-KEX] per-player sounds
static edict_t *AI_GetSoundClient(edict_t *self, bool direct)
{
    edict_t *best_sound = nullptr;
    float best_distance = std::numeric_limits<float>::max();

    for (auto player : active_players())
    {
        // dead
        if (player->health <= 0 || player->deadflag || !player->solid)
            continue;

        edict_t *sound = direct ? player->client->sound_entity : player->client->sound2_entity;

        if (!sound)
            continue;

        // too late
        gtime_t &time = direct ? player->client->sound_entity_time : player->client->sound2_entity_time;

        if (!(time >= (level.time - FRAME_TIME_S)))
            continue;

        // prefer the closest one we heard
        float dist = (self->s.origin - sound->s.origin).length();

        if (!best_sound || dist < best_distance)
        {
            best_distance = dist;
            best_sound = sound;
        }
    }

    return best_sound;
}

bool G_MonsterSourceVisible(edict_t *self, edict_t *client)
{
    // this is where we would check invisibility
    float r = range_to(self, client);

    if (r > RANGE_MID)
        return false;

    // Paril: revised so that monsters can be woken up
    // by players 'seen' and attacked at by other monsters
    // if they are close enough. they don't have to be visible.
    bool is_visible =
        ((r <= RANGE_NEAR && client->show_hostile >= level.time && !(self->spawnflags & SPAWNFLAG_MONSTER_AMBUSH)) ||
            (visible(self, client) && (r <= RANGE_MELEE || (self->monsterinfo.aiflags & AI_THIRD_EYE) || infront(self, client))));

    return is_visible;
}

/*
===========
FindTarget

Self is currently not attacking anything, so try to find a target

Returns TRUE if an enemy was sighted

When a player fires a missile, the point of impact becomes a fakeplayer so
that monsters that see the impact will respond as if they had seen the
player.

To avoid spending too much time, only a single client (or fakeclient) is
checked each frame.  This means multi player games will have slightly
slower noticing monsters.
============
*/
bool FindTarget(edict_t *self)
{
    edict_t *client = nullptr;
    bool     heardit;
    bool     ignore_sight_sound = false;

    // [Paril-KEX] if we're in a level transition, don't worry about enemies
    if (globals.server_flags & SERVER_FLAG_LOADING)
        return false;

    // N64 cutscene behavior
    if (self->hackflags & HACKFLAG_END_CUTSCENE)
        return false;

    if (self->monsterinfo.aiflags & AI_GOOD_GUY)
    {
        if (self->goalentity && self->goalentity->inuse && self->goalentity->classname)
        {
            if (strcmp(self->goalentity->classname, "target_actor") == 0)
                return false;
        }

        // FIXME look for monsters?
        return false;
    }

    // if we're going to a combat point, just proceed
    if (self->monsterinfo.aiflags & AI_COMBAT_POINT)
        return false;

    // if the first spawnflag bit is set, the monster will only wake up on
    // really seeing the player, not another monster getting angry or hearing
    // something

    // revised behavior so they will wake up if they "see" a player make a noise
    // but not weapon impact/explosion noises
    heardit = false;
    
    // Paril: revised so that monsters will first try to consider
    // the current sight client immediately if they can see it.
    // this fixes them dancing in front of you if you fire every frame.
    if ((client = AI_GetSightClient(self)))
    {
        if (client == self->enemy)
        {
            return false;
        }
    }
    // check indirect sources
    if (!client)
    {
        // check monsters that were alerted by players; we can only be alerted if we
        // can see them
        if (!(self->spawnflags & SPAWNFLAG_MONSTER_AMBUSH) && (client = AI_GetMonsterAlertedByPlayers(self)))
        {
            // KEX_FIXME: when does this happen? 
            // [Paril-KEX] adjusted to clear the client
            // so we can try other things
            if (client->enemy == self->enemy ||
                !G_MonsterSourceVisible(self, client))
                client = nullptr;
        }
        // ROGUE

        if (client == nullptr)
        {
            if (level.disguise_violation_time > level.time)
            {
                client = level.disguise_violator;
            }
            // ROGUE
            else if ((client = AI_GetSoundClient(self, true)))
            {
                heardit = true;
            }
            else if (!(self->enemy) && !(self->spawnflags & SPAWNFLAG_MONSTER_AMBUSH) &&
                (client = AI_GetSoundClient(self, false)))
            {
                heardit = true;
            }
        }
    }

    if (!client)
        return false; // no clients to get mad at

    // if the entity went away, forget it
    if (!client->inuse)
        return false;

    if (client == self->enemy)
    {
        bool skip_found = true;

        // [Paril-KEX] slight special behavior if we are currently going to a sound
        // and we hear a new one; because player noises are re-used, this can leave
        // us with the "same" enemy even though it's a different noise.
        if (heardit && (self->monsterinfo.aiflags & AI_SOUND_TARGET))
        {
            vec3_t temp = client->s.origin - self->s.origin;
            self->ideal_yaw = vectoyaw(temp);

            if (!FacingIdeal(self))
                skip_found = false;
            else if (!SV_CloseEnough(self, client, 8.f))
                skip_found = false;

            if (!skip_found && (self->monsterinfo.aiflags & AI_TEMP_STAND_GROUND))
            {
                self->monsterinfo.aiflags &= ~(AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
            }
        }

        if (skip_found)
            return true; // JDC false;
    }

    // ROGUE - hintpath coop fix
    if ((self->monsterinfo.aiflags & AI_HINT_PATH) && coop->integer)
        heardit = false;
    // ROGUE

    if (client->svflags & SVF_MONSTER)
    {
        if (!client->enemy)
            return false;
        if (client->enemy->flags & FL_NOTARGET)
            return false;
    }
    else if (heardit)
    {
        // pgm - a little more paranoia won't hurt....
        if ((client->owner) && (client->owner->flags & FL_NOTARGET))
            return false;
    }
    else if (!client->client)
        return false;

    if (!heardit)
    {
        // this is where we would check invisibility
        float r = range_to(self, client);

        if (r > RANGE_MID)
            return false;

        // Paril: revised so that monsters can be woken up
        // by players 'seen' and attacked at by other monsters
        // if they are close enough. they don't have to be visible.
        bool is_visible =
            ((r <= RANGE_NEAR && client->show_hostile >= level.time && !(self->spawnflags & SPAWNFLAG_MONSTER_AMBUSH)) ||
            (visible(self, client) && (r <= RANGE_MELEE || (self->monsterinfo.aiflags & AI_THIRD_EYE) || infront(self, client))));

        if (!is_visible)
            return false;

        self->enemy = client;

        if (strcmp(self->enemy->classname, "player_noise") != 0)
        {
            self->monsterinfo.aiflags &= ~AI_SOUND_TARGET;

            if (!self->enemy->client)
            {
                self->enemy = self->enemy->enemy;
                if (!self->enemy->client)
                {
                    self->enemy = nullptr;
                    return false;
                }
            }
        }

        if (self->enemy->client && self->enemy->client->invisible_time > level.time && self->enemy->client->invisibility_fade_time <= level.time)
        {
            self->enemy = nullptr;
            return false;
        }

        if (self->monsterinfo.close_sight_tripped)
            ignore_sight_sound = true;
        else
            self->monsterinfo.close_sight_tripped = true;
    }
    else // heardit
    {
        vec3_t temp;

        if (self->spawnflags.has(SPAWNFLAG_MONSTER_AMBUSH))
        {
            if (!visible(self, client))
                return false;
        }
        else
        {
            if (!gi.inPHS(self->s.origin, client->s.origin, true))
                return false;
        }

        temp = client->s.origin - self->s.origin;

        if (temp.length() > 1000) // too far to hear
            return false;

        // check area portals - if they are different and not connected then we can't hear it
        if (client->areanum != self->areanum)
            if (!gi.AreasConnected(self->areanum, client->areanum))
                return false;

        self->ideal_yaw = vectoyaw(temp);
        // ROGUE
        if (!(self->monsterinfo.aiflags & AI_MANUAL_STEERING))
            // ROGUE
            M_ChangeYaw(self);

        // hunt the sound for a bit; hopefully find the real player
        self->monsterinfo.aiflags |= AI_SOUND_TARGET;
        self->enemy = client;
    }

    //
    // got one
    //
    // ROGUE - if we got an enemy, we need to bail out of hint paths, so take over here
    if (self->monsterinfo.aiflags & AI_HINT_PATH)
        hintpath_stop(self);  // this calls foundtarget for us
    else
        FoundTarget(self);

    // ROGUE
    if (!(self->monsterinfo.aiflags & AI_SOUND_TARGET) && (self->monsterinfo.sight) &&
        // Paril: adjust to prevent monsters getting stuck in sight loops
        !ignore_sight_sound)
        self->monsterinfo.sight(self, self->enemy);

    return true;
}

//=============================================================================

/*
============
FacingIdeal

============
*/
bool FacingIdeal(edict_t *self)
{
    float delta = anglemod(self->s.angles[YAW] - self->ideal_yaw);

    if (self->monsterinfo.aiflags & AI_PATHING)
        return !(delta > 5 && delta < 355);

    return !(delta > 45 && delta < 315);
}

//=============================================================================

// [Paril-KEX] split this out so we can use it for the other bosses
bool M_CheckAttack_Base(edict_t *self, float stand_ground_chance, float melee_chance, float near_chance, float mid_chance, float far_chance, float strafe_scalar)
{
    vec3_t  spot1, spot2;
    float   chance;
    trace_t tr;

    if (self->enemy->flags & FL_NOVISIBLE)
        return false;

    if (self->enemy->health > 0)
    {
        if (self->enemy->client)
        {
            if (self->enemy->client->invisible_time > level.time)
            {
                // can't see us at all after this time
                if (self->enemy->client->invisibility_fade_time <= level.time)
                    return false;
            }
        }

        spot1 = self->s.origin;
        spot1[2] += self->viewheight;
        // see if any entities are in the way of the shot
        if (!self->enemy->client || self->enemy->solid)
        {
            spot2 = self->enemy->s.origin;
            spot2[2] += self->enemy->viewheight;

            tr = gi.traceline(spot1, spot2, self,
                MASK_SOLID | CONTENTS_MONSTER | CONTENTS_PLAYER | CONTENTS_SLIME | CONTENTS_LAVA);
        }
        else
        {
            tr.ent = world;
            tr.fraction = 0;
        }

        // do we have a clear shot?
        if (!(self->hackflags & HACKFLAG_ATTACK_PLAYER) && tr.ent != self->enemy && !(tr.ent->svflags & SVF_PLAYER))
        {
            // ROGUE - we want them to go ahead and shoot at info_notnulls if they can.
            if (self->enemy->solid != SOLID_NOT || tr.fraction < 1.0f) // PGM
            {
                // PMM - if we can't see our target, and we're not blocked by a monster, go into blind fire if available
                // Paril - *and* we have at least seen them once
                if (!(tr.ent->svflags & SVF_MONSTER) && !visible(self, self->enemy) && self->monsterinfo.had_visibility)
                {
                    if (self->monsterinfo.blindfire && (self->monsterinfo.blind_fire_delay <= 20_sec))
                    {
                        if (level.time < self->monsterinfo.attack_finished)
                        {
                            // ROGUE
                            return false;
                        }
                        // ROGUE
                        if (level.time < (self->monsterinfo.trail_time + self->monsterinfo.blind_fire_delay))
                        {
                            // wait for our time
                            return false;
                        }
                        else
                        {
                            // make sure we're not going to shoot a monster
                            tr = gi.traceline(spot1, self->monsterinfo.blind_fire_target, self,
                                CONTENTS_MONSTER);
                            if (tr.allsolid || tr.startsolid || ((tr.fraction < 1.0f) && (tr.ent != self->enemy)))
                                return false;

                            self->monsterinfo.attack_state = AS_BLIND;
                            return true;
                        }
                    }
                }
                // pmm
                return false;
            }
        }
    }
    // ROGUE

    float enemy_range = range_to(self, self->enemy);

    // melee attack
    if (enemy_range <= RANGE_MELEE)
    {
        if (self->monsterinfo.melee && self->monsterinfo.melee_debounce_time <= level.time)
            self->monsterinfo.attack_state = AS_MELEE;
        else
            self->monsterinfo.attack_state = AS_MISSILE;
        return true;
    }

    // if we were in melee just before this but we're too far away, get out of melee state now
    if (self->monsterinfo.attack_state == AS_MELEE && self->monsterinfo.melee_debounce_time > level.time)
        self->monsterinfo.attack_state = AS_MISSILE;

    // missile attack
    if (!self->monsterinfo.attack)
    {
        // ROGUE - fix for melee only monsters & strafing
        self->monsterinfo.attack_state = AS_STRAIGHT;
        // ROGUE
        return false;
    }

    if (level.time < self->monsterinfo.attack_finished)
        return false;

    if (self->monsterinfo.aiflags & AI_STAND_GROUND)
    {
        chance = stand_ground_chance;
    }
    else if (enemy_range <= RANGE_MELEE)
    {
        chance = melee_chance;
    }
    else if (enemy_range <= RANGE_NEAR)
    {
        chance = near_chance;
    }
    else if (enemy_range <= RANGE_MID)
    {
        chance = mid_chance;
    }
    else
    {
        chance = far_chance;
    }

    // PGM - go ahead and shoot every time if it's a info_notnull
    if ((!self->enemy->client && self->enemy->solid == SOLID_NOT) || (frandom() < chance))
    {
        self->monsterinfo.attack_state = AS_MISSILE;
        self->monsterinfo.attack_finished = level.time;
        return true;
    }

    // ROGUE -daedalus should strafe more .. this can be done here or in a customized
    // check_attack code for the hover.
    if (self->flags & FL_FLY)
    {
        if (self->monsterinfo.strafe_check_time <= level.time)
        {
            // originally, just 0.3
            float strafe_chance;

            if (!(strcmp(self->classname, "monster_daedalus")))
                strafe_chance = 0.8f;
            else
                strafe_chance = 0.6f;

            // if enemy is tesla, never strafe
            if ((self->enemy) && (self->enemy->classname) && (!strcmp(self->enemy->classname, "tesla_mine")))
                strafe_chance = 0;
            else
                strafe_chance *= strafe_scalar;

            if (strafe_chance)
            {
                monster_attack_state_t new_state = AS_STRAIGHT;

                if (frandom() < strafe_chance)
                    new_state = AS_SLIDING;

                if (new_state != self->monsterinfo.attack_state)
                {
                    self->monsterinfo.strafe_check_time = level.time + random_time(1_sec, 3_sec);
                    self->monsterinfo.attack_state = new_state;
                }
            }
        }
    }
    // do we want the monsters strafing?
    // [Paril-KEX] no, we don't
    // [Paril-KEX] if we're pathing, don't immediately reset us to
    // straight; this allows us to turn to fire and not jerk back and
    // forth.
    else if (!(self->monsterinfo.aiflags & AI_PATHING))
        self->monsterinfo.attack_state = AS_STRAIGHT;
    // ROGUE

    return false;
}

MONSTERINFO_CHECKATTACK(M_CheckAttack) (edict_t *self) -> bool
{
    return M_CheckAttack_Base(self, 0.7f, 0.4f, 0.25f, 0.06f, 0.f, 1.0f);
}

/*
=============
ai_run_melee

Turn and close until within an angle to launch a melee attack
=============
*/
void ai_run_melee(edict_t *self)
{
    self->ideal_yaw = enemy_yaw;
    // ROGUE
    if (!(self->monsterinfo.aiflags & AI_MANUAL_STEERING))
        // ROGUE
        M_ChangeYaw(self);

    if (FacingIdeal(self))
    {
        self->monsterinfo.melee(self);
        self->monsterinfo.attack_state = AS_STRAIGHT;
    }
}

/*
=============
ai_run_missile

Turn in place until within an angle to launch a missile attack
=============
*/
void ai_run_missile(edict_t *self)
{
    self->ideal_yaw = enemy_yaw;
    // ROGUE
    if (!(self->monsterinfo.aiflags & AI_MANUAL_STEERING))
        // ROGUE
        M_ChangeYaw(self);

    if (FacingIdeal(self))
    {
        if (self->monsterinfo.attack)
        {
            self->monsterinfo.attack(self);
            self->monsterinfo.attack_finished = level.time + random_time(1.0_sec, 2.0_sec);
        }

        // ROGUE
        if ((self->monsterinfo.attack_state == AS_MISSILE) || (self->monsterinfo.attack_state == AS_BLIND))
            // ROGUE
            self->monsterinfo.attack_state = AS_STRAIGHT;
    }
};

/*
=============
ai_run_slide

Strafe sideways, but stay at aproximately the same range
=============
*/
// ROGUE
void ai_run_slide(edict_t *self, float distance)
{
    float ofs;
    float angle;

    self->ideal_yaw = enemy_yaw;

    angle = 90;

    if (self->monsterinfo.lefty)
        ofs = angle;
    else
        ofs = -angle;

    if (!(self->monsterinfo.aiflags & AI_MANUAL_STEERING))
        M_ChangeYaw(self);

    // PMM - clamp maximum sideways move for non flyers to make them look less jerky
    if (!(self->flags & FL_FLY))
        distance = min(distance, MAX_SIDESTEP / (gi.frame_time_ms / 10));
    if (M_walkmove(self, self->ideal_yaw + ofs, distance))
        return;
    // PMM - if we're dodging, give up on it and go straight
    if (self->monsterinfo.aiflags & AI_DODGING)
    {
        monster_done_dodge(self);
        // by setting as_straight, caller will know to try straight move
        self->monsterinfo.attack_state = AS_STRAIGHT;
        return;
    }

    self->monsterinfo.lefty = !self->monsterinfo.lefty;
    if (M_walkmove(self, self->ideal_yaw - ofs, distance))
        return;
    // PMM - if we're dodging, give up on it and go straight
    if (self->monsterinfo.aiflags & AI_DODGING)
        monster_done_dodge(self);

    // PMM - the move failed, so signal the caller (ai_run) to try going straight
    self->monsterinfo.attack_state = AS_STRAIGHT;
}
// ROGUE

/*
=============
ai_checkattack

Decides if we're going to attack or do something else
used by ai_run and ai_stand
=============
*/
bool ai_checkattack(edict_t *self, float dist)
{
    vec3_t temp;
    bool   hesDeadJim;
    // ROGUE
    bool retval;
    // ROGUE

    if (self->monsterinfo.aiflags & AI_TEMP_STAND_GROUND)
        self->monsterinfo.aiflags &= ~(AI_STAND_GROUND | AI_TEMP_STAND_GROUND);

    // this causes monsters to run blindly to the combat point w/o firing
    if (self->goalentity)
    {
        if (self->monsterinfo.aiflags & AI_COMBAT_POINT)
        {
            if (self->enemy && range_to(self, self->enemy) > 100.f)
                return false;
        }

        if (self->monsterinfo.aiflags & AI_SOUND_TARGET)
        {
            if ((level.time - self->enemy->teleport_time) > 5_sec)
            {
                if (self->goalentity == self->enemy)
                {
                    if (self->movetarget)
                        self->goalentity = self->movetarget;
                    else
                        self->goalentity = nullptr;
                }
                self->monsterinfo.aiflags &= ~AI_SOUND_TARGET;
            }
            else
            {
                self->enemy->show_hostile = level.time + 1_sec;
                return false;
            }
        }
    }

    enemy_vis = false;

    // see if the enemy is dead
    hesDeadJim = false;
    if ((!self->enemy) || (!self->enemy->inuse))
    {
        hesDeadJim = true;
    } else if ( self->monsterinfo.aiflags & AI_FORGET_ENEMY )
    {
        self->monsterinfo.aiflags &= ~AI_FORGET_ENEMY;
        hesDeadJim = true;
    }
    else if (self->monsterinfo.aiflags & AI_MEDIC)
    {
        if (!(self->enemy->inuse) || (self->enemy->health > 0))
            hesDeadJim = true;
    }
    else
    {
        if (!(self->monsterinfo.aiflags & AI_BRUTAL))
        {
            if (self->enemy->health <= 0)
                hesDeadJim = true;
        }

        // [Paril-KEX] if our enemy was invisible, lose sight now
        if (self->enemy->client && self->enemy->client->invisible_time > level.time && self->enemy->client->invisibility_fade_time <= level.time &&
            (self->monsterinfo.aiflags & AI_PURSUE_NEXT))
        {
            hesDeadJim = true;
        }
    }

    if (hesDeadJim && !(self->hackflags & HACKFLAG_ATTACK_PLAYER))
    {
        // ROGUE
        self->monsterinfo.aiflags &= ~AI_MEDIC;
        // ROGUE
        self->enemy = self->goalentity = nullptr;
        self->monsterinfo.close_sight_tripped = false;
        // FIXME: look all around for other targets
        if (self->oldenemy && self->oldenemy->health > 0)
        {
            self->enemy = self->oldenemy;
            self->oldenemy = nullptr;
            HuntTarget(self);
        }
        // ROGUE - multiple teslas make monsters lose track of the player.
        else if (self->monsterinfo.last_player_enemy && self->monsterinfo.last_player_enemy->health > 0)
        {
            self->enemy = self->monsterinfo.last_player_enemy;
            self->oldenemy = nullptr;
            self->monsterinfo.last_player_enemy = nullptr;
            HuntTarget(self);
        }
        // ROGUE
        else
        {
            if (self->movetarget && !(self->monsterinfo.aiflags & AI_STAND_GROUND))
            {
                self->goalentity = self->movetarget;
                self->monsterinfo.walk(self);
            }
            else
            {
                // we need the pausetime otherwise the stand code
                // will just revert to walking with no target and
                // the monsters will wonder around aimlessly trying
                // to hunt the world entity
                self->monsterinfo.pausetime = HOLD_FOREVER;
                self->monsterinfo.stand(self);

                if (self->monsterinfo.aiflags & AI_TEMP_STAND_GROUND)
                    self->monsterinfo.aiflags &= ~(AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
            }
            return true;
        }
    }

    // check knowledge of enemy
    enemy_vis = visible(self, self->enemy);
    if (enemy_vis)
    {
        self->monsterinfo.had_visibility = true;
        self->enemy->show_hostile = level.time + 1_sec; // wake up other monsters
        self->monsterinfo.search_time = level.time + 5_sec;
        self->monsterinfo.last_sighting = self->monsterinfo.saved_goal = self->enemy->s.origin;
        // ROGUE
        if (self->monsterinfo.aiflags & AI_LOST_SIGHT)
        {
            self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;

            if (self->monsterinfo.move_block_change_time < level.time)
                self->monsterinfo.aiflags &= ~AI_TEMP_MELEE_COMBAT;
        }
        self->monsterinfo.trail_time = level.time;
        self->monsterinfo.blind_fire_target = self->monsterinfo.last_sighting + (self->enemy->velocity * -0.1f);
        self->monsterinfo.blind_fire_delay = 0_ms;
        // ROGUE
    }

    enemy_infront = infront(self, self->enemy);
    temp = self->enemy->s.origin - self->s.origin;
    enemy_yaw = vectoyaw(temp);

    // PMM -- reordered so the monster specific checkattack is called before the run_missle/melee/checkvis
    // stuff .. this allows for, among other things, circle strafing and attacking while in ai_run
    retval = false;

    if (self->monsterinfo.checkattack_time <= level.time)
    {
        self->monsterinfo.checkattack_time = level.time + 0.1_sec;
        retval = self->monsterinfo.checkattack(self);
    }

    if (retval || self->monsterinfo.attack_state >= AS_MISSILE)
    {
        // PMM
        if (self->monsterinfo.attack_state == AS_MISSILE)
        {
            ai_run_missile(self);
            return true;
        }
        if (self->monsterinfo.attack_state == AS_MELEE)
        {
            ai_run_melee(self);
            return true;
        }
        // PMM -- added so monsters can shoot blind
        if (self->monsterinfo.attack_state == AS_BLIND)
        {
            ai_run_missile(self);
            return true;
        }
        // pmm

        // if enemy is not currently visible, we will never attack
        if (!enemy_vis)
            return false;
        // PMM
    }

    return retval;
    // PMM
}

/*
=============
ai_run

The monster has an enemy it is trying to kill
=============
*/
void ai_run(edict_t *self, float dist)
{
    vec3_t   v;
    edict_t *tempgoal;
    edict_t *save;
    bool     newEnemy;
    edict_t *marker;
    float    d1, d2;
    trace_t  tr;
    vec3_t   v_forward, v_right;
    float    left, center, right;
    vec3_t   left_target, right_target;
    // ROGUE
    bool     retval;
    bool     alreadyMoved = false;
    bool     gotcha = false;
    edict_t *realEnemy;
    // ROGUE

    // if we're going to a combat point, just proceed
    if (self->monsterinfo.aiflags & AI_COMBAT_POINT)
    {
        ai_checkattack(self, dist);
        M_MoveToGoal(self, dist);

        if (self->movetarget)
        {
            // nb: this is done from the centroid and not viewheight on purpose;
            trace_t tr = gi.trace((self->absmax + self->absmin) * 0.5f, { -2.f, -2.f, -2.f }, { 2.f, 2.f, 2.f }, self->movetarget->s.origin, self, CONTENTS_SOLID);

            // [Paril-KEX] special case: if we're stand ground & knocked way too far away
            // from our path_corner, or we can't see it any more, assume all
            // is lost.
            if ((self->monsterinfo.aiflags & AI_REACHED_HOLD_COMBAT) && (((closest_point_to_box(self->movetarget->s.origin, self->absmin, self->absmax) - self->movetarget->s.origin).length() > 160.f)
                || (tr.fraction < 1.0f && tr.plane.normal.z <= 0.7f))) // if we hit a climbable, ignore this result
            {
                self->monsterinfo.aiflags &= ~AI_COMBAT_POINT;
                self->movetarget = nullptr;
                self->target = nullptr;
                self->goalentity = self->enemy;
            }
            else
                return;
        }
        else
            return;
    }

    // PMM
    if ((self->monsterinfo.aiflags & AI_DUCKED) && self->monsterinfo.unduck)
		self->monsterinfo.unduck(self);

    //==========
    // PGM
    // if we're currently looking for a hint path
    if (self->monsterinfo.aiflags & AI_HINT_PATH)
    {
        // determine direction to our destination hintpath.
        M_MoveToGoal(self, dist);
        if (!self->inuse)
            return;

        // first off, make sure we're looking for the player, not a noise he made
        if (self->enemy)
        {
            if (self->enemy->inuse)
            {
                if (strcmp(self->enemy->classname, "player_noise") != 0)
                    realEnemy = self->enemy;
                else if (self->enemy->owner)
                    realEnemy = self->enemy->owner;
                else // uh oh, can't figure out enemy, bail
                {
                    self->enemy = nullptr;
                    hintpath_stop(self);
                    return;
                }
            }
            else
            {
                self->enemy = nullptr;
                hintpath_stop(self);
                return;
            }
        }
        else
        {
            hintpath_stop(self);
            return;
        }

        if (coop->integer)
        {
            // if we're in coop, check my real enemy first .. if I SEE him, set gotcha to true
            if (self->enemy && visible(self, realEnemy))
                gotcha = true;
            else // otherwise, let FindTarget bump us out of hint paths, if appropriate
                FindTarget(self);
        }
        else
        {
            if (self->enemy && visible(self, realEnemy))
                gotcha = true;
        }

        // if we see the player, stop following hintpaths.
        if (gotcha)
            // disconnect from hintpaths and start looking normally for players.
            hintpath_stop(self);

        return;
    }
    // PGM
    //==========

    if (self->monsterinfo.aiflags & AI_SOUND_TARGET)
    {
        // PMM - paranoia checking
        if (self->enemy)
            v = self->s.origin - self->enemy->s.origin;

        bool touching_noise = SV_CloseEnough(self, self->enemy, dist * (gi.tick_rate / 10));

        if ((!self->enemy) || (touching_noise && FacingIdeal(self)))
        // pmm
        {
            self->monsterinfo.aiflags |= (AI_STAND_GROUND | AI_TEMP_STAND_GROUND);
            self->s.angles[YAW] = self->ideal_yaw;
            self->monsterinfo.stand(self);
            self->monsterinfo.close_sight_tripped = false;
            return;
        }

        // if we're close to the goal, just turn
        if (touching_noise)
            M_ChangeYaw(self);
        else
            M_MoveToGoal(self, dist);

        // ROGUE - prevent double moves for sound_targets
        alreadyMoved = true;

        if (!self->inuse)
            return; // PGM - g_touchtrigger free problem
        // ROGUE

        if (!FindTarget(self))
            return;
    }

    // PMM -- moved ai_checkattack up here so the monsters can attack while strafing or charging

    // PMM -- if we're dodging, make sure to keep the attack_state AS_SLIDING
    retval = ai_checkattack(self, dist);

    // PMM - don't strafe if we can't see our enemy
    if ((!enemy_vis) && (self->monsterinfo.attack_state == AS_SLIDING))
        self->monsterinfo.attack_state = AS_STRAIGHT;
    // unless we're dodging (dodging out of view looks smart)
    if (self->monsterinfo.aiflags & AI_DODGING)
        self->monsterinfo.attack_state = AS_SLIDING;
    // pmm

    if (self->monsterinfo.attack_state == AS_SLIDING)
    {
        // PMM - protect against double moves
        if (!alreadyMoved)
            ai_run_slide(self, dist);
        // PMM
        // we're using attack_state as the return value out of ai_run_slide to indicate whether or not the
        // move succeeded.  If the move succeeded, and we're still sliding, we're done in here (since we've
        // had our chance to shoot in ai_checkattack, and have moved).
        // if the move failed, our state is as_straight, and it will be taken care of below
        if ((!retval) && (self->monsterinfo.attack_state == AS_SLIDING))
            return;
    }
    else if (self->monsterinfo.aiflags & AI_CHARGING)
    {
        self->ideal_yaw = enemy_yaw;
        if (!(self->monsterinfo.aiflags & AI_MANUAL_STEERING))
            M_ChangeYaw(self);
    }
    if (retval)
    {
        // PMM - is this useful?  Monsters attacking usually call the ai_charge routine..
        // the only monster this affects should be the soldier
        if ((dist || (self->monsterinfo.aiflags & AI_ALTERNATE_FLY)) && (!alreadyMoved) && (self->monsterinfo.attack_state == AS_STRAIGHT) &&
            (!(self->monsterinfo.aiflags & AI_STAND_GROUND)))
        {
            M_MoveToGoal(self, dist);
        }
        if ((self->enemy) && (self->enemy->inuse) && (enemy_vis))
        {
            if (self->monsterinfo.aiflags & AI_LOST_SIGHT)
            {
                self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;

                if (self->monsterinfo.move_block_change_time < level.time)
                    self->monsterinfo.aiflags &= ~AI_TEMP_MELEE_COMBAT;
            }
            self->monsterinfo.last_sighting = self->monsterinfo.saved_goal = self->enemy->s.origin;
            self->monsterinfo.trail_time = level.time;
            // PMM
            self->monsterinfo.blind_fire_target = self->monsterinfo.last_sighting + (self->enemy->velocity * -0.1f);
            self->monsterinfo.blind_fire_delay = 0_ms;
            // pmm
        }
        return;
    }
    // PMM

    // PGM - added a little paranoia checking here... 9/22/98
    if ((self->enemy) && (self->enemy->inuse) && (enemy_vis))
    {
        // PMM - check for alreadyMoved
        if (!alreadyMoved)
            M_MoveToGoal(self, dist);
        if (!self->inuse)
            return; // PGM - g_touchtrigger free problem
        
        if (self->monsterinfo.aiflags & AI_LOST_SIGHT)
        {
            self->monsterinfo.aiflags &= ~AI_LOST_SIGHT;

            if (self->monsterinfo.move_block_change_time < level.time)
                self->monsterinfo.aiflags &= ~AI_TEMP_MELEE_COMBAT;
        }
        self->monsterinfo.last_sighting = self->monsterinfo.saved_goal = self->enemy->s.origin;
        self->monsterinfo.trail_time = level.time;
        // PMM
        self->monsterinfo.blind_fire_target = self->monsterinfo.last_sighting + (self->enemy->velocity * -0.1f);
        self->monsterinfo.blind_fire_delay = 0_ms;
        // pmm

        // [Paril-KEX] if our enemy is literally right next to us, give
        // us more rotational speed so we don't get circled
        if (range_to(self, self->enemy) <= RANGE_MELEE * 2.5f)
            M_ChangeYaw(self);

        return;
    }

    //=======
    // PGM
    // if we've been looking (unsuccessfully) for the player for 10 seconds
    // PMM - reduced to 5, makes them much nastier
    if ((self->monsterinfo.trail_time + 5_sec) <= level.time)
    {
        // and we haven't checked for valid hint paths in the last 10 seconds
        if ((self->monsterinfo.last_hint_time + 10_sec) <= level.time)
        {
            // check for hint_paths.
            self->monsterinfo.last_hint_time = level.time;
            if (monsterlost_checkhint(self))
                return;
        }
    }
    // PGM
    //=======

    // PMM - moved down here to allow monsters to get on hint paths
    // coop will change to another enemy if visible
    if (coop->integer)
        FindTarget(self);
    // pmm

    if ((self->monsterinfo.search_time) && (level.time > (self->monsterinfo.search_time + 20_sec)))
    {
        // PMM - double move protection
        if (!alreadyMoved)
            M_MoveToGoal(self, dist);
        self->monsterinfo.search_time = 0_ms;
        return;
    }

    save = self->goalentity;
    tempgoal = G_Spawn();
    self->goalentity = tempgoal;

    newEnemy = false;

    if (!(self->monsterinfo.aiflags & AI_LOST_SIGHT))
    {
        // just lost sight of the player, decide where to go first
        self->monsterinfo.aiflags |= (AI_LOST_SIGHT | AI_PURSUIT_LAST_SEEN);
        self->monsterinfo.aiflags &= ~(AI_PURSUE_NEXT | AI_PURSUE_TEMP);
        newEnemy = true;
        
        // immediately try paths
		self->monsterinfo.path_blocked_counter = 0_ms;
		self->monsterinfo.path_wait_time = 0_ms;
    }

    if (self->monsterinfo.aiflags & AI_PURSUE_NEXT)
    {
        self->monsterinfo.aiflags &= ~AI_PURSUE_NEXT;

        // give ourself more time since we got this far
        self->monsterinfo.search_time = level.time + 5_sec;

        if (self->monsterinfo.aiflags & AI_PURSUE_TEMP)
        {
            self->monsterinfo.aiflags &= ~AI_PURSUE_TEMP;
            marker = nullptr;
            self->monsterinfo.last_sighting = self->monsterinfo.saved_goal;
            newEnemy = true;
        }
        else if (self->monsterinfo.aiflags & AI_PURSUIT_LAST_SEEN)
        {
            self->monsterinfo.aiflags &= ~AI_PURSUIT_LAST_SEEN;
            marker = PlayerTrail_Pick(self, false);
        }
        else
        {
            marker = PlayerTrail_Pick(self, true);
        }

        if (marker)
        {
            self->monsterinfo.last_sighting = marker->s.origin;
            self->monsterinfo.trail_time = marker->timestamp;
            self->s.angles[YAW] = self->ideal_yaw = marker->s.angles[YAW];

            newEnemy = true;
        }
    }

    if (!(self->monsterinfo.aiflags & AI_PATHING) &&
        boxes_intersect(self->monsterinfo.last_sighting, self->monsterinfo.last_sighting, self->s.origin + self->mins, self->s.origin + self->maxs))
    {
        self->monsterinfo.aiflags |= AI_PURSUE_NEXT;
        dist = min(dist, (self->s.origin - self->monsterinfo.last_sighting).length());
        // [Paril-KEX] this helps them navigate corners when two next pursuits
        // are really close together
        self->monsterinfo.random_change_time = level.time + 10_hz;
    }

    self->goalentity->s.origin = self->monsterinfo.last_sighting;

    if (newEnemy)
    {
        tr =
            gi.trace(self->s.origin, self->mins, self->maxs, self->monsterinfo.last_sighting, self, MASK_PLAYERSOLID);
        if (tr.fraction < 1)
        {
            v = self->goalentity->s.origin - self->s.origin;
            d1 = v.length();
            center = tr.fraction;
            d2 = d1 * ((center + 1) / 2);
            float backup_yaw = self->s.angles.y;
            self->s.angles[YAW] = self->ideal_yaw = vectoyaw(v);
            AngleVectors(self->s.angles, v_forward, v_right, nullptr);

            v = { d2, -16, 0 };
            left_target = G_ProjectSource(self->s.origin, v, v_forward, v_right);
            tr = gi.trace(self->s.origin, self->mins, self->maxs, left_target, self, MASK_PLAYERSOLID);
            left = tr.fraction;

            v = { d2, 16, 0 };
            right_target = G_ProjectSource(self->s.origin, v, v_forward, v_right);
            tr = gi.trace(self->s.origin, self->mins, self->maxs, right_target, self, MASK_PLAYERSOLID);
            right = tr.fraction;

            center = (d1 * center) / d2;
            if (left >= center && left > right)
            {
                if (left < 1)
                {
                    v = { d2 * left * 0.5f, -16, 0 };
                    left_target = G_ProjectSource(self->s.origin, v, v_forward, v_right);
                }
                self->monsterinfo.saved_goal = self->monsterinfo.last_sighting;
                self->monsterinfo.aiflags |= AI_PURSUE_TEMP;
                self->goalentity->s.origin = left_target;
                self->monsterinfo.last_sighting = left_target;
                v = self->goalentity->s.origin - self->s.origin;
                self->ideal_yaw = vectoyaw(v);
            }
            else if (right >= center && right > left)
            {
                if (right < 1)
                {
                    v = { d2 * right * 0.5f, 16, 0 };
                    right_target = G_ProjectSource(self->s.origin, v, v_forward, v_right);
                }
                self->monsterinfo.saved_goal = self->monsterinfo.last_sighting;
                self->monsterinfo.aiflags |= AI_PURSUE_TEMP;
                self->goalentity->s.origin = right_target;
                self->monsterinfo.last_sighting = right_target;
                v = self->goalentity->s.origin - self->s.origin;
                self->ideal_yaw = vectoyaw(v);
            }
            self->s.angles[YAW] = backup_yaw;
        }
    }

    M_MoveToGoal(self, dist);

    G_FreeEdict(tempgoal);

    if (!self->inuse)
        return; // PGM - g_touchtrigger free problem

    if (self)
        self->goalentity = save;
}
