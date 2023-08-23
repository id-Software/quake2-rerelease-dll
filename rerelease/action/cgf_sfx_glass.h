/****************************************************************************/
/*                                                                          */
/*    project    : CGF                  (c) 1999 William van der Sterren    */
/*                               parts  (c) 1998 id software                */
/*                                                                          */
/*      file     : cgf_sfx_glass.h    "special effects for glass entities"  */
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
/*                                                                          */
/******* http://www.botepidemic.com/aid/cgf for CGF for Action Quake2 *******/
//
// $Id: cgf_sfx_glass.h,v 1.2 2001/09/28 13:48:34 ra Exp $
//
//-----------------------------------------------------------------------------
// $Log: cgf_sfx_glass.h,v $
// Revision 1.2  2001/09/28 13:48:34  ra
// I ran indent over the sources. All .c and .h files reindented.
//
// Revision 1.1.1.1  2001/05/06 17:29:34  igor_rock
// This is the PG Bund Edition V1.25 with all stuff laying around here...
//
//-----------------------------------------------------------------------------

#ifndef __CGF_SFX_GLASS_H_
#define __CGF_SFX_GLASS_H_


// defines (should be consistent with other weapon defs in g_local.h)
#define MOD_BREAKINGGLASS 46

/*
   // forward definitions
   typedef struct edict_s edict_t;
 */


// export a number of functions to g_func.c:
//
//

void CGF_SFX_InstallGlassSupport ();
// registers cvar breakableglass (default 0)
// registers cvar glassfragmentlimit (default 30)


void CGF_SFX_RebuildAllBrokenGlass ();
// upon starting a new team play game, reconstruct any
// broken glass because we like to break it again


int CGF_SFX_IsBreakableGlassEnabled ();
// returns whether breakable glass is enabled (cvar breakableglass)


void CGF_SFX_TestBreakableGlassAndRemoveIfNot_Think (edict_t *
						     aPossibleGlassEntity);
// initial think function for all func_explosives
// because we cannot verify the contents of the entity unless the entity
// has been spawned, we need to first introduce the entity, and remove
// it later (level.time == 0.1) if required


void CGF_SFX_ShootBreakableGlass (edict_t * aGlassPane, edict_t * anAttacker,
				  /*trace_t */ void *tr,
				  int mod);
// shoot thru glass - depending on bullet types and
// random effects, glass just emits fragments, or breaks


void CGF_SFX_AttachDecalToGlass (edict_t * aGlassPane, edict_t * aDecal);
// a glass that breaks will remove all decals (blood splashes, bullet
// holes) if they are attached


#endif


/*

   further documentation:

   cgf_sfx_glass.cpp can be compiled with ordinary c compilers as
   well - just change the extension from .cpp to .c

   to install breakable glass in the ActionQuake2 1.51 code base,
   do the following:

   @ file a_game.h
   + modify
   #define ACTION_VERSION  "1.51"
   to something that tells users breakable glass is supported

   @ file a_team.c
   + add #include "cgf_sfx_glass.h"
   + in void CleanLevel()
   add 
   CGF_SFX_RebuildAllBrokenGlass();
   as the last statement (thus after CleanBodies();)

   @ file g_misc.c
   + add #include "cgf_sfx_glass.h"
   + in void SP_func_explosive (edict_t *self)
   disable the statements (put them within comments):
   if (deathmatch->value)
   {       // auto-remove for deathmatch
   G_FreeEdict (self);
   return;
   }
   + in void SP_func_explosive (edict_t *self)
   add
   self->think     = CGF_SFX_TestBreakableGlassAndRemoveIfNot_Think;
   self->nextthink = level.framenum + 1;
   as the last statement (thus after gi.linkentity (self);)

   @ file g_save.c
   + add #include "cgf_sfx_glass.h"
   + in void InitGame (void)
   add
   CGF_SFX_InstallGlassSupport();
   under (splatlimit = gi.cvar ("splatlimit", "0", 0);)

   @ file g_local.h
   + replace
   void AddDecal (edict_t *self, vec3_t point, vec3_t direct);
   void AddSplat (edict_t *self, vec3_t point, vec3_t direct);
   by    
   void AddDecal (edict_t *self, trace_t* tr);
   void AddSplat (edict_t *self, vec3_t point, trace_t* tr);

   @ file a_game.c
   + add #include "cgf_sfx_glass.h"
   + replace
   void AddDecal (edict_t *self, vec3_t point, vec3_t direct)
   by    
   void AddDecal (edict_t *self, trace_t* tr)
   + replace
   void AddSplat (edict_t *self, vec3_t point, vec3_t direct);
   by    
   void AddSplat (edict_t *self, vec3_t point, trace_t* tr);
   + in void AddDecal (edict_t *self, trace_t* tr)
   replace each occurrence of 'point' by tr->endpos
   + in void AddDecal (edict_t *self, trace_t* tr)
   replace each occurrence of 'direct' by tr->plane.normal
   + in void AddDecal (edict_t *self, trace_t* tr)
   add (as the last line, thus under gi.linkentity (decal);)
   if ((tr->ent) && (0 == Q_stricmp("func_explosive", tr->ent->classname)))
   {
   CGF_SFX_AttachDecalToGlass(tr->ent, decal);
   }
   + in void AddSplat (edict_t *self, vec3_t point, trace_t* tr)
   replace each occurrence of 'direct' by tr->plane.normal
   + in void AddSplat (edict_t *self, vec3_t point, trace_t* tr)
   add (as the last line, thus under gi.linkentity (decal);)
   if ((tr->ent) && (0 == Q_stricmp("func_explosive", tr->ent->classname)))
   {
   CGF_SFX_AttachDecalToGlass(tr->ent, splat);
   }
   @ file g_weapon.c
   + add #include "cgf_sfx_glass.h"
   + in static void fire_lead (edict_t *self, ...)
   replace 
   AddDecal (self, tr.endpos, tr.plane.normal);
   by
   AddDecal (self, &tr);
   + in void fire_lead_ap (edict_t *self, ...)
   replace 
   AddDecal (self, tr.endpos, tr.plane.normal);
   by
   AddDecal (self, &tr);
   + in static void fire_lead (edict_t *self, ...)
   add
   between
   tr = do_trace (start, NULL, NULL, end, self, content_mask);
   and
   // see if we hit water
   the following
   // catch case of firing thru one or breakable glasses
   while (   (tr.fraction < 1.0) 
   && (tr.surface->flags & (SURF_TRANS33 | SURF_TRANS66))
   && (tr.ent) 
   && (0 == Q_stricmp(tr.ent->classname, "func_explosive"))
   )
   {
   // break glass  
   CGF_SFX_ShootBreakableGlass(tr.ent, self, &tr, mod);
   // continue trace from current endpos to start
   tr = do_trace (tr.endpos, NULL, NULL, end, tr.ent, content_mask);
   }
   + in static void fire_lead_ap (edict_t *self, ...)
   add
   between
   tr = do_trace (start, NULL, NULL, end, self, content_mask);
   and
   // see if we hit water
   the following
   // catch case of firing thru one or breakable glasses
   while (   (tr.fraction < 1.0) 
   && (tr.surface->flags & (SURF_TRANS33 | SURF_TRANS66))
   && (tr.ent) 
   && (0 == Q_stricmp(tr.ent->classname, "func_explosive"))
   )
   {
   // break glass  
   CGF_SFX_ShootBreakableGlass(tr.ent, self, &tr, mod);
   // continue trace from current endpos to start
   tr = do_trace (tr.endpos, NULL, NULL, end, tr.ent, content_mask);
   }
   + in void knife_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
   replace
   if (other->takedamage)
   by 
   if (0 == Q_stricmp(other->classname, "func_explosive"))
   {
   // ignore it, so it can bounce
   return;
   }
   else
   if (other->takedamage) 
   + in void kick_attack (edict_t * ent )
   between
   // zucc stop powerful upwards kicking
   forward[2] = 0;
   and
   T_Damage (tr.ent, ent, ent, forward, tr.endpos, tr.plane.normal, damage, kick, 0, MOD_KICK );
   add
   if (0 == Q_stricmp(tr.ent->classname, "func_explosive"))
   {
   CGF_SFX_ShootBreakableGlass(tr.ent, ent, &tr, MOD_KICK);
   }
   + in int knife_attack ( edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick)
   replace
   if (tr.ent->takedamage)
   by
   if (0 == Q_stricmp(tr.ent->classname, "func_explosive"))
   {
   CGF_SFX_ShootBreakableGlass(tr.ent, self, &tr, MOD_KNIFE);
   }
   else
   if (tr.ent->takedamage) 

   @ file g_phys.c
   + add #include "cgf_sfx_glass.h"
   + in void SV_Physics_Toss (edict_t *ent)
   replace 
   AddSplat (self, tr.endpos, tr.plane.normal);
   by
   AddSplat (self, &tr);

   @ file g_combat.c
   + add #include "cgf_sfx_glass.h"
   + in void T_Damage (edict_t *targ, ...)
   replace
   if ( (mod == MOD_M3) || (mod == MOD_HC) || (mod == MOD_HELD_GRENADE) ||
   (mod == MOD_HG_SPLASH) || (mod == MOD_G_SPLASH) )
   by
   if (    (mod == MOD_M3) 
   || (mod == MOD_HC) 
   || (mod == MOD_HELD_GRENADE) 
   || (mod == MOD_HG_SPLASH) 
   || (mod == MOD_G_SPLASH) 
   || (mod == MOD_BREAKINGGLASS)
   )
   + in void T_RadiusDamage (...)
   add before if (CanDamage (ent, inflictor))
   if (0 == Q_stricmp(ent->classname, "func_explosive"))
   {
   CGF_SFX_ShootBreakableGlass(ent, inflictor, 0, mod);
   }
   else

   @ file p_client.c
   + add #include "cgf_sfx_glass.h"
   + in void ClientObituary (edict_t *self, edict_t *inflictor, edict_t *attacker)
   between
   switch (mod)
   {
   and
   case MOD_SUICIDE:
   add
   case MOD_BREAKINGGLASS:
   message = "ate too much glass";
   break;
 */
