// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#include "cg_local.h"
#include "m_flash.h"

cgame_import_t cgi;
cgame_export_t cglobals;

static void *CG_GetExtension(const char *name)
{
	return nullptr;
}

void CG_InitScreen();

uint64_t cgame_init_time = 0;

static void InitCGame()
{
	CG_InitScreen();

	cgame_init_time = cgi.CL_ClientRealTime();

	pm_config.n64_physics = !!atoi(cgi.get_configstring(CONFIG_N64_PHYSICS));
	pm_config.airaccel = atoi(cgi.get_configstring(CS_AIRACCEL));
}

static void ShutdownCGame()
{
}

void CG_DrawHUD (int32_t isplit, const cg_server_data_t *data, vrect_t hud_vrect, vrect_t hud_safe, int32_t scale, int32_t playernum, const player_state_t *ps);
void CG_TouchPics();
layout_flags_t CG_LayoutFlags(const player_state_t *ps);

int32_t CG_GetActiveWeaponWheelWeapon(const player_state_t *ps)
{
	return ps->stats[STAT_ACTIVE_WHEEL_WEAPON];
}

uint32_t CG_GetOwnedWeaponWheelWeapons(const player_state_t *ps)
{
	return ((uint32_t) (uint16_t) ps->stats[STAT_WEAPONS_OWNED_1]) | ((uint32_t) (uint16_t) (ps->stats[STAT_WEAPONS_OWNED_2]) << 16);
}

int16_t CG_GetWeaponWheelAmmoCount(const player_state_t *ps, int32_t ammo_id)
{
	uint16_t ammo = G_GetAmmoStat((uint16_t *) &ps->stats[STAT_AMMO_INFO_START], ammo_id);

	if (ammo == AMMO_VALUE_INFINITE)
		return -1;

	return ammo;
}

int16_t CG_GetPowerupWheelCount(const player_state_t *ps, int32_t powerup_id)
{
	return G_GetPowerupStat((uint16_t *) &ps->stats[STAT_POWERUP_INFO_START], powerup_id);
}

int16_t CG_GetHitMarkerDamage(const player_state_t *ps)
{
	return ps->stats[STAT_HIT_MARKER];
}

static void CG_ParseConfigString(int32_t i, const char *s)
{
	if (i == CONFIG_N64_PHYSICS)
		pm_config.n64_physics = !!atoi(s);
	else if (i == CS_AIRACCEL)
		pm_config.airaccel = atoi(s);
}

void CG_ParseCenterPrint (const char *str, int isplit, bool instant);
void CG_ClearNotify(int32_t isplit);
void CG_ClearCenterprint(int32_t isplit);
void CG_NotifyMessage(int32_t isplit, const char *msg, bool is_chat);

void CG_GetMonsterFlashOffset(monster_muzzleflash_id_t id, gvec3_ref_t offset)
{
	if (id >= q_countof(monster_flash_offset))
		cgi.Com_Error("Bad muzzle flash offset");

	offset = monster_flash_offset[id];
}

/*
=================
GetCGameAPI

Returns a pointer to the structure with all entry points
and global variables
=================
*/
Q2GAME_API cgame_export_t *GetCGameAPI(cgame_import_t *import)
{
	cgi = *import;

	cglobals.apiversion = CGAME_API_VERSION;
	cglobals.Init = InitCGame;
	cglobals.Shutdown = ShutdownCGame;

	cglobals.Pmove = Pmove;
	cglobals.DrawHUD = CG_DrawHUD;
	cglobals.LayoutFlags = CG_LayoutFlags;
	cglobals.TouchPics = CG_TouchPics;
	
	cglobals.GetActiveWeaponWheelWeapon = CG_GetActiveWeaponWheelWeapon;
	cglobals.GetOwnedWeaponWheelWeapons = CG_GetOwnedWeaponWheelWeapons;
	cglobals.GetWeaponWheelAmmoCount = CG_GetWeaponWheelAmmoCount;
	cglobals.GetPowerupWheelCount = CG_GetPowerupWheelCount;
	cglobals.GetHitMarkerDamage = CG_GetHitMarkerDamage;
	cglobals.ParseConfigString = CG_ParseConfigString;
	cglobals.ParseCenterPrint = CG_ParseCenterPrint;
	cglobals.ClearNotify = CG_ClearNotify;
	cglobals.ClearCenterprint = CG_ClearCenterprint;
	cglobals.NotifyMessage = CG_NotifyMessage;
	cglobals.GetMonsterFlashOffset = CG_GetMonsterFlashOffset;

	cglobals.GetExtension = CG_GetExtension;

	return &cglobals;
}
