/****************************************************************************/
/*                                                                          */
/*    project    : CGF                  (c) 1999 William van der Sterren    */
/*                               parts  (c) 1998 id software                */
/*                                                                          */
/*      file     : cgf_sfx_glass.cpp  "special effects for glass entities"  */
/*      author(s): William van der Sterren                                  */
/*      version  : 0.5                                                      */
/*                                                                          */
/*      date (last revision):  Jun 12, 99                                   */
/*      date (creation)     :  Jun 04, 99                                   */
/*                                                                          */
/*                                                                          */
/*      revision history                                                    */
/*      -- date ---- | -- revision ---------------------- | -- revisor --   */
/*      Jun 12, 1999 | fixed knife slash breaks glass     | William         */
/*      Jun 08, 1999 | improved fragment limit            | William         */
/*                                                                          */
/******* http://www.botepidemic.com/aid/cgf for CGF for Action Quake2 *******/
//
// $Id: cgf_sfx_glass.c,v 1.2 2001/09/28 13:48:34 ra Exp $
//
//-----------------------------------------------------------------------------
// $Log: cgf_sfx_glass.c,v $
// Revision 1.2  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.1.1.1  2001/05/06 17:29:34  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#ifdef __cplusplus
  // VC++, for CGF
#include <cmath>		// prevent problems between C and STL
extern "C"
{
#include "g_local.h"
#include "cgf_sfx_glass.h"
}
#else
  // C, for other AQ2 variants
#include "g_local.h"
#include "cgf_sfx_glass.h"
#endif


// cvar for breaking glass
static cvar_t *breakableglass = 0;

// cvar for max glass fragment count
static cvar_t *glassfragmentlimit = 0;

static int glassfragmentcount = 0;


// additional functions - Q2 expects C calling convention
#ifdef __cplusplus
extern "C"
{
#endif

  void CGF_SFX_TouchGlass (edict_t * self, edict_t * other, cplane_t * plane,
			   csurface_t * surf);
// called whenever an entity hits the trigger spawned for the glass

  void CGF_SFX_EmitGlass (edict_t * aGlassPane, edict_t * anInflictor,
			  vec3_t aPoint);
// emits glass fragments from aPoint, to show effects of firing thru window

  void CGF_SFX_BreakGlass (edict_t * aGlassPane, edict_t * anOther,
			   edict_t * anAttacker, int aDamage, vec3_t aPoint,
			   vec_t aPaneDestructDelay);
// breaks glass

  void CGF_SFX_InstallBreakableGlass (edict_t * aGlassPane);
// when working on a glass pane for the first time, just install trigger
// when working on a glass pane again (after a game ended), move
// glass back to original location

  void CGF_SFX_HideBreakableGlass (edict_t * aGlassPane);
// after being broken, the pane cannot be removed as it is needed in
// subsequent missions/games, so hide it at about z = -1000

  void CGF_SFX_ApplyGlassFragmentLimit (const char *aClassName);
// updates glassfragmentcount and removes oldest glass fragement if
// necessary to meet limit

  void CGF_SFX_MiscGlassUse (edict_t * self, edict_t * other,
			     edict_t * activator);
// catches use from unforeseen objects (weapons, debris,
// etc. touching the window)

  void CGF_SFX_MiscGlassDie (edict_t * self, edict_t * inflictor,
			     edict_t * attacker, int damage, vec3_t point);
// catches die calls caused by unforeseen objects (weapons, debris,
// etc. damaging the window)

  void CGF_SFX_GlassThrowDebris (edict_t * self, char *modelname, float speed,
				 vec3_t origin);
// variant of id software's ThrowDebris, now numbering the entity (for later removal)

  extern			// from a_game.c
  edict_t *FindEdictByClassnum (char *classname, int classnum);

// declaration from g_misc.c
  extern			// from g_misc.c
  void debris_die (edict_t * self, edict_t * inflictor, edict_t * attacker,
		   int damage, vec3_t point);

#ifdef __cplusplus
}
#endif



void
CGF_SFX_InstallGlassSupport ()
{
  breakableglass = gi.cvar ("breakableglass", "0", 0);
  glassfragmentlimit = gi.cvar ("glassfragmentlimit", "30", 0);
}


int
CGF_SFX_IsBreakableGlassEnabled ()
{
  // returns whether breakable glass is enabled (cvar) and allowed (dm mode)
  return breakableglass->value;
}



void
CGF_SFX_TestBreakableGlassAndRemoveIfNot_Think (edict_t *
						aPossibleGlassEntity)
{
  // at level.time == 0.1 the entity has been introduced in the game,
  // and we can use gi.pointcontents and gi.trace to check the entity
  vec3_t origin;
  int breakingglass;
  trace_t trace;

  // test for cvar
  if (!CGF_SFX_IsBreakableGlassEnabled ())
    {
      G_FreeEdict (aPossibleGlassEntity);
      return;
    }

  VectorAdd (aPossibleGlassEntity->absmax, aPossibleGlassEntity->absmin,
	     origin);
  VectorScale (origin, 0.5, origin);

  // detect glass (does not work for complex shapes,
  // for example, the glass window near the satellite
  // dish at Q2 base3
  breakingglass = (gi.pointcontents (origin) & CONTENTS_TRANSLUCENT);

  if (!breakingglass)
    {
      // test for complex brushes that happen to be
      // hollow in their origin (for instance, the
      // window at Q2 base3, near the satellite dish
      trace =
	gi.trace (origin, vec3_origin, vec3_origin,
		  aPossibleGlassEntity->absmax, 0, MASK_PLAYERSOLID);
      breakingglass = ((trace.ent == aPossibleGlassEntity)
		       && (trace.contents & CONTENTS_TRANSLUCENT));
      trace =
	gi.trace (origin, vec3_origin, vec3_origin,
		  aPossibleGlassEntity->absmin, 0, MASK_PLAYERSOLID);
      breakingglass = ((breakingglass)
		       || ((trace.ent == aPossibleGlassEntity)
			   && (trace.contents & CONTENTS_TRANSLUCENT)));
    }

  if (!breakingglass)
    {
      // do remove other func_explosives
      G_FreeEdict (aPossibleGlassEntity);
      return;
    }

  // discovered some glass - now make store the origin
  // we need that after hiding the glass
  VectorCopy (aPossibleGlassEntity->s.origin, aPossibleGlassEntity->pos1);	// IMPORTANT!

  // make a backup of the health in light_level
  aPossibleGlassEntity->light_level = aPossibleGlassEntity->health;

  // install the glass
  CGF_SFX_InstallBreakableGlass (aPossibleGlassEntity);
}



void
CGF_SFX_InstallBreakableGlass (edict_t * aGlassPane)
{
  // when working on a glass pane for the first time, just install trigger
  // when working on a glass pane again (after a game ended), move
  // glass back to original location
  edict_t *trigger;
  vec3_t maxs;
  vec3_t mins;

  // reset origin based on aGlassPane->pos1
  VectorCopy (aGlassPane->pos1, aGlassPane->s.origin);

  // reset health based on aGlassPane->light_level
  aGlassPane->health = aGlassPane->light_level;

  // replace die and use functions by glass specific ones
  aGlassPane->die = CGF_SFX_MiscGlassDie;
  aGlassPane->use = CGF_SFX_MiscGlassUse;

  // reset some pane attributes
  aGlassPane->takedamage = DAMAGE_YES;
  aGlassPane->solid = SOLID_BSP;
  aGlassPane->movetype = MOVETYPE_FLYMISSILE;
  // for other movetypes, cannot move pane to hidden location and back

  // try to establish size
  VectorCopy (aGlassPane->maxs, maxs);
  VectorCopy (aGlassPane->mins, mins);

  // set up trigger, similar to triggers for doors
  // but with a smaller box
  mins[0] -= 24;
  mins[1] -= 24;
  mins[2] -= 24;
  maxs[0] += 24;
  maxs[1] += 24;
  maxs[2] += 24;

  // adjust some settings
  trigger = G_Spawn ();
  trigger->classname = "breakableglass_trigger";
  VectorCopy (mins, trigger->mins);
  VectorCopy (maxs, trigger->maxs);
  trigger->owner = aGlassPane;
  trigger->solid = SOLID_TRIGGER;
  trigger->movetype = MOVETYPE_NONE;
  trigger->touch = CGF_SFX_TouchGlass;
  gi.linkentity (trigger);
}


void
CGF_SFX_ShootBreakableGlass (edict_t * aGlassPane, edict_t * anAttacker,
			     /*trace_t* */ void *tr,
			     int mod)
{
  // process gunshots thru glass
  edict_t *trigger;
  int destruct;

  // depending on mod, destroy window or emit fragments
  switch (mod)
    {
      // break for ap, shotgun, handcannon, and kick, destory window
    case MOD_M3:
    case MOD_HC:
    case MOD_SNIPER:
    case MOD_KICK:
    case MOD_GRENADE:
    case MOD_G_SPLASH:
    case MOD_HANDGRENADE:
    case MOD_HG_SPLASH:
    case MOD_KNIFE:		// slash damage
      destruct = true;
      break;
    default:
      destruct = (rand () % 3 == 0);
      break;
    };

  if (destruct)
    {
      // break glass (and hurt if doing kick)
      CGF_SFX_BreakGlass (aGlassPane, anAttacker, 0, aGlassPane->health,
			  vec3_origin, FRAMETIME);
      if (mod == MOD_KICK)
	{
	  vec3_t bloodorigin;
	  vec3_t dir;
	  vec3_t normal;
	  VectorAdd (aGlassPane->absmax, aGlassPane->absmin, bloodorigin);
	  VectorScale (bloodorigin, 0.5, bloodorigin);
	  VectorSubtract (bloodorigin, anAttacker->s.origin, dir);
	  VectorNormalize (dir);
	  VectorMA (anAttacker->s.origin, 32.0, dir, bloodorigin);
	  VectorSet (normal, 0, 0, -1);
	  T_Damage (anAttacker, aGlassPane, anAttacker, dir, bloodorigin,
		    normal, 15.0, 0, 0, MOD_BREAKINGGLASS);
	}

      // remove corresponding trigger
      trigger = 0;
      while ((trigger = G_Find (trigger, FOFS (classname), "breakableglass_trigger")) != NULL)
	{
	  if (trigger->owner == aGlassPane)
	    {
	      // remove it
	      G_FreeEdict (trigger);
	      // only one to be found
	      break;
	    }
	}
    }
  else
    {
      // add decal (if not grenade)
      if ((mod != MOD_HANDGRENADE)
	  && (mod != MOD_HG_SPLASH)
	  && (mod != MOD_GRENADE) && (mod != MOD_G_SPLASH))
	{
	  AddDecal (anAttacker, (trace_t *) tr);
	}
      // and emit glass
      CGF_SFX_EmitGlass (aGlassPane, anAttacker, ((trace_t *) tr)->endpos);
    }
}


void
CGF_SFX_TouchGlass (edict_t * self, edict_t * other, cplane_t * plane,
		    csurface_t * surf)
{
  // called whenever an entity hits the trigger spawned for the glass

  vec3_t origin;
  vec3_t normal;
  vec3_t spot;
  trace_t trace;
  edict_t *glass;
  vec3_t velocity;
  vec_t speed;
  vec_t projected_speed;
  int is_hgrenade;
  int is_knife;

  is_hgrenade = is_knife = false;

  // ignore non-clients-non-grenade-non-knife
  if (!other->client)
    {
      is_knife = (0 == Q_stricmp ("weapon_knife", other->classname));
      if (!is_knife)
	{
	  is_hgrenade = (0 == Q_stricmp ("hgrenade", other->classname));
	}

      if ((!is_knife) && (!is_hgrenade))
	return;
      if (is_knife)
	goto knife_and_grenade_handling;
    }

  // test whether other really hits the glass - deal with
  // the special case that other hits some boundary close to the border of the glass pane
  //
  //
  //             ....trigger.......
  //      +++++++++              +++++++++++
  //             .+---glass------+.
  //       wall  .+--------------+.wall
  //      +++++++++              +++++++++++
  //       ----->..................
  //    wrong    ^            ^
  //             |            |
  //            wrong        ok 
  //
  glass = self->owner;
  // hack - set glass' movetype to MOVETYPE_PUSH as it is not
  // moving as long as the trigger is active
  glass->movetype = MOVETYPE_PUSH;

  VectorAdd (glass->absmax, glass->absmin, origin);
  VectorScale (origin, 0.5, origin);

  // other needs to be able to trace to glass origin
  trace = gi.trace (other->s.origin, vec3_origin, vec3_origin, origin, other,
		    MASK_PLAYERSOLID);
  if (trace.ent != glass)
    return;

  // we can reach the glass origin, so we have the normal of
  // the glass plane
  VectorCopy (trace.plane.normal, normal);

  // we need to check if client is not running into wall next
  // to the glass (the trigger stretches into the wall)
  VectorScale (normal, -1000.0, spot);
  VectorAdd (spot, other->s.origin, spot);
  // line between other->s.origin and spot (perpendicular to glass
  // surface should not hit wall but glass instead
  trace = gi.trace (other->s.origin, vec3_origin, vec3_origin, spot, other,
		    MASK_PLAYERSOLID);
  if (trace.ent != glass)
    return;

  // now, we check if the client's speed perpendicular to
  // the glass plane, exceeds the required 175
  // (speed should be < -200, as the plane's normal
  // points towards the client
  VectorCopy (other->velocity, velocity);
  speed = VectorNormalize (velocity);
  projected_speed = speed * DotProduct (velocity, normal);

  // bump projected speed for grenades - they should break
  // the window more easily
  if (is_hgrenade)
    projected_speed *= 1.5f;

  // if hitting the glass with sufficient speed (project < -175),
  // being jumpkicked (speed > 700, project < -5) break the window 
  if (!((projected_speed < -175.0) ||
	((projected_speed < -5) && (speed > 700))))
    goto knife_and_grenade_handling;

  // break glass
  CGF_SFX_BreakGlass (glass, other, other, glass->health, vec3_origin,
		      3.0f * FRAMETIME);
  // glass can take care of itself, but the trigger isn't needed anymore
  G_FreeEdict (self);

  /* not needed
     // reduce momentum of the client (he just broke the window
     // so he should lose speed. in addition, it doesn't feel
     // right if he overtakes the glass fragments
     //  VectorScale(normal, 200.0, velocity);
     //  VectorAdd(other->velocity, velocity, other->velocity);
   */

  // make sure client takes damage
  T_Damage (other, glass, other, normal, other->s.origin, normal, 15.0, 0, 0,
	    MOD_BREAKINGGLASS);
  return;

  // goto label
knife_and_grenade_handling:
  // if knife or grenade, bounce them
  if ((is_knife) || (is_hgrenade))
    {
      // change clipmask to bounce of glass
      other->clipmask = MASK_SOLID;
    }
}


void
CGF_SFX_BreakGlass (edict_t * aGlassPane, edict_t * anInflictor,
		    edict_t * anAttacker, int aDamage, vec3_t aPoint,
		    vec_t aPaneDestructDelay)
{
  // based on func_explode, but with lotsa subtle differences
  vec3_t origin;
  vec3_t old_origin;
  vec3_t chunkorigin;
  vec3_t size;
  int count;
  int mass;

  // bmodel origins are (0 0 0), we need to adjust that here
  VectorCopy (aGlassPane->s.origin, old_origin);
  VectorScale (aGlassPane->size, 0.5, size);
  VectorAdd (aGlassPane->absmin, size, origin);
  VectorCopy (origin, aGlassPane->s.origin);

  aGlassPane->takedamage = DAMAGE_NO;

  VectorSubtract (aGlassPane->s.origin, anInflictor->s.origin,
		  aGlassPane->velocity);
  VectorNormalize (aGlassPane->velocity);
  // use speed 250 instead of 150 for funkier glass spray
  VectorScale (aGlassPane->velocity, 250.0, aGlassPane->velocity);

  // start chunks towards the center
  VectorScale (size, 0.75, size);

  mass = aGlassPane->mass;
  if (!mass)
    mass = 75;

  // big chunks
  if (mass >= 100)
    {
      count = mass / 100;
      if (count > 8)
	count = 8;
      while (count--)
	{
	  CGF_SFX_ApplyGlassFragmentLimit ("debris");
	  chunkorigin[0] = origin[0] + crandom () * size[0];
	  chunkorigin[1] = origin[1] + crandom () * size[1];
	  chunkorigin[2] = origin[2] + crandom () * size[2];
	  CGF_SFX_GlassThrowDebris (aGlassPane,
				    "models/objects/debris1/tris.md2", 1,
				    chunkorigin);
	}
    }

  // small chunks
  count = mass / 25;
  if (count > 16)
    count = 16;
  while (count--)
    {
      CGF_SFX_ApplyGlassFragmentLimit ("debris");
      chunkorigin[0] = origin[0] + crandom () * size[0];
      chunkorigin[1] = origin[1] + crandom () * size[1];
      chunkorigin[2] = origin[2] + crandom () * size[2];
      CGF_SFX_GlassThrowDebris (aGlassPane, "models/objects/debris2/tris.md2",
				2, chunkorigin);
    }

  // clear velocity, reset origin (that has been abused in ThrowDebris)
  VectorClear (aGlassPane->velocity);
  VectorCopy (old_origin, aGlassPane->s.origin);

  if (anAttacker)
    {
      // jumping thru
      G_UseTargets (aGlassPane, anAttacker);
    }
  else
    {
      // firing thru - the pane has no direct attacker to hurt,
      // but G_UseTargets expects one. So make it a DIY
      G_UseTargets (aGlassPane, aGlassPane);
    }

  // have glass plane be visible for two more frames, 
  // and have it self-destruct then
  // meanwhile, make sure the player can move thru
  aGlassPane->solid = SOLID_NOT;
  aGlassPane->think = CGF_SFX_HideBreakableGlass;
  aGlassPane->nextthink = level.framenum + aPaneDestructDelay * HZ;
}



void
CGF_SFX_EmitGlass (edict_t * aGlassPane, edict_t * anInflictor, vec3_t aPoint)
{
  // based on func_explode, but with lotsa subtle differences
  vec3_t old_origin;
  vec3_t chunkorigin;
  vec3_t size;
  int count;

  // bmodel origins are (0 0 0), we need to adjust that here
  VectorCopy (aGlassPane->s.origin, old_origin);
  VectorCopy (aPoint, aGlassPane->s.origin);

  VectorSubtract (aGlassPane->s.origin, anInflictor->s.origin,
		  aGlassPane->velocity);
  VectorNormalize (aGlassPane->velocity);
  // use speed 250 instead of 150 for funkier glass spray
  VectorScale (aGlassPane->velocity, 250.0, aGlassPane->velocity);

  // start chunks towards the center
  VectorScale (aGlassPane->size, 0.25, size);

  count = 4;
  while (count--)
    {
      CGF_SFX_ApplyGlassFragmentLimit ("debris");
      chunkorigin[0] = aPoint[0] + crandom () * size[0];
      chunkorigin[1] = aPoint[1] + crandom () * size[1];
      chunkorigin[2] = aPoint[2] + crandom () * size[2];
      CGF_SFX_GlassThrowDebris (aGlassPane, "models/objects/debris2/tris.md2",
				2, chunkorigin);
    }

  // clear velocity, reset origin (that has been abused in ThrowDebris)
  VectorClear (aGlassPane->velocity);
  VectorCopy (old_origin, aGlassPane->s.origin);

  // firing thru - the pane has no direct attacker to hurt,
  // but G_UseTargets expects one. So make it a DIY
  G_UseTargets (aGlassPane, aGlassPane);
}



void
CGF_SFX_HideBreakableGlass (edict_t * aGlassPane)
{
  // remove all attached decals
  edict_t *decal;
  decal = 0;
  while ((decal = G_Find (decal, FOFS (classname), "decal")) != NULL)
    {
      if (decal->owner == aGlassPane)
	{
	  // make it goaway in the next frame
	  decal->think = G_FreeEdict;
	  decal->nextthink = level.framenum + 1;
	}
    }

  while ((decal = G_Find (decal, FOFS (classname), "splat")) != NULL)
    {
      if (decal->owner == aGlassPane)
	{
	  // make it goaway in the next frame
	  decal->think = G_FreeEdict;
	  decal->nextthink = level.framenum + 1;
	}
    }

  // after being broken, the pane cannot be freed as it is needed in
  // subsequent missions/games, so hide it at about z = -1000 lower
  aGlassPane->movetype = MOVETYPE_FLYMISSILE;
  VectorCopy (aGlassPane->s.origin, aGlassPane->pos1);
  aGlassPane->s.origin[2] -= 1000.0;
}



void
CGF_SFX_AttachDecalToGlass (edict_t * aGlassPane, edict_t * aDecal)
{
  // just set aDecal's owner to be the glass pane
  aDecal->owner = aGlassPane;
}



void
CGF_SFX_RebuildAllBrokenGlass ()
{
  // iterate over all func_explosives
  edict_t *glass;
  glass = 0;
  while ((glass = G_Find (glass, FOFS (classname), "func_explosive")) != NULL)
    {
      // glass is broken if solid != SOLID_BSP
      if (glass->solid != SOLID_BSP)
	{
	  CGF_SFX_InstallBreakableGlass (glass);
	}
    }
}



void
CGF_SFX_ApplyGlassFragmentLimit (const char *aClassName)
{
  edict_t *oldfragment;

  glassfragmentcount++;
  if (glassfragmentcount > glassfragmentlimit->value)
    glassfragmentcount = 1;

  // remove fragment with corresponding number if any
  oldfragment = FindEdictByClassnum ((char *) aClassName, glassfragmentcount);

  if (oldfragment)
    {
      // oldfragment->nextthink = level.framenum + 1;
      G_FreeEdict (oldfragment);
    }
}



void
CGF_SFX_MiscGlassUse (edict_t * self, edict_t * other, edict_t * activator)
{
#ifdef _DEBUG
  const char *classname;
  classname = other->classname;
#endif
}



void
CGF_SFX_MiscGlassDie (edict_t * self, edict_t * inflictor, edict_t * attacker,
		      int damage, vec3_t point)
{
#ifdef _DEBUG
  const char *classname;
  classname = inflictor->classname;
#endif
}



static vec_t previous_throw_time = 0;
static int this_throw_count = 0;

void
CGF_SFX_GlassThrowDebris (edict_t * self, char *modelname, float speed,
			  vec3_t origin)
{
  // based on ThrowDebris from id software - now returns debris created
  edict_t *chunk;
  vec3_t v;

  if (level.time != previous_throw_time)
    {
      previous_throw_time = level.time;
      this_throw_count = 0;
    }
  else
    {
      this_throw_count++;
      if (this_throw_count > glassfragmentlimit->value)
	return;
    }

  chunk = G_Spawn ();
  VectorCopy (origin, chunk->s.origin);
  gi.setmodel (chunk, modelname);
  v[0] = 100 * crandom ();
  v[1] = 100 * crandom ();
  v[2] = 100 + 100 * crandom ();
  VectorMA (self->velocity, speed, v, chunk->velocity);
  chunk->movetype = MOVETYPE_BOUNCE;
  chunk->solid = SOLID_NOT;
  chunk->avelocity[0] = random () * 600;
  chunk->avelocity[1] = random () * 600;
  chunk->avelocity[2] = random () * 600;
  chunk->think = G_FreeEdict;
  chunk->nextthink = level.framenum + (5 + random() * 5) * HZ;
  chunk->s.frame = 0;
  chunk->flags = 0;
  chunk->classname = "debris";
  chunk->takedamage = DAMAGE_YES;
  chunk->die = debris_die;
  gi.linkentity (chunk);

  // number chunk
  chunk->classnum = glassfragmentcount;
}
