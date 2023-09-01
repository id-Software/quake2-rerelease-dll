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
