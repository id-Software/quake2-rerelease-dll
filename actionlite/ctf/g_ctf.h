// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.

#define CTF_VERSION 1.52
#define CTF_VSTRING2(x) #x
#define CTF_VSTRING(x) CTF_VSTRING2(x)
#define CTF_STRING_VERSION CTF_VSTRING(CTF_VERSION)

enum ctfteam_t
{
	CTF_NOTEAM,
	CTF_TEAM1,
	CTF_TEAM2
};

enum ctfgrapplestate_t
{
	CTF_GRAPPLE_STATE_FLY,
	CTF_GRAPPLE_STATE_PULL,
	CTF_GRAPPLE_STATE_HANG
};

struct ghost_t
{
	char netname[MAX_NETNAME];
	int	 number;

	// stats
	int deaths;
	int kills;
	int caps;
	int basedef;
	int carrierdef;

	int		 code;	// ghost code
	ctfteam_t		 team;	// team
	int		 score; // frags at time of disconnect
	edict_t *ent;
};

extern cvar_t *ctf;
extern cvar_t *g_teamplay_force_join;
extern cvar_t *teamplay;

constexpr const char *CTF_TEAM1_SKIN = "ctf_r";
constexpr const char *CTF_TEAM2_SKIN = "ctf_b";

constexpr int32_t CTF_CAPTURE_BONUS = 15;	  // what you get for capture
constexpr int32_t CTF_TEAM_BONUS = 10;		  // what your team gets for capture
constexpr int32_t CTF_RECOVERY_BONUS = 1;	  // what you get for recovery
constexpr int32_t CTF_FLAG_BONUS = 0;		  // what you get for picking up enemy flag
constexpr int32_t CTF_FRAG_CARRIER_BONUS = 2; // what you get for fragging enemy flag carrier
constexpr gtime_t CTF_FLAG_RETURN_TIME = 40_sec;  // seconds until auto return

constexpr int32_t CTF_CARRIER_DANGER_PROTECT_BONUS = 2; // bonus for fraggin someone who has recently hurt your flag carrier
constexpr int32_t CTF_CARRIER_PROTECT_BONUS = 1;		// bonus for fraggin someone while either you or your target are near your flag carrier
constexpr int32_t CTF_FLAG_DEFENSE_BONUS = 1;			// bonus for fraggin someone while either you or your target are near your flag
constexpr int32_t CTF_RETURN_FLAG_ASSIST_BONUS = 1;		// awarded for returning a flag that causes a capture to happen almost immediately
constexpr int32_t CTF_FRAG_CARRIER_ASSIST_BONUS = 2;	// award for fragging a flag carrier if a capture happens almost immediately

constexpr float CTF_TARGET_PROTECT_RADIUS = 400;   // the radius around an object being defended where a target will be worth extra frags
constexpr float CTF_ATTACKER_PROTECT_RADIUS = 400; // the radius around an object being defended where an attacker will get extra frags when making kills

constexpr gtime_t CTF_CARRIER_DANGER_PROTECT_TIMEOUT = 8_sec;
constexpr gtime_t CTF_FRAG_CARRIER_ASSIST_TIMEOUT = 10_sec;
constexpr gtime_t CTF_RETURN_FLAG_ASSIST_TIMEOUT = 10_sec;

constexpr gtime_t CTF_AUTO_FLAG_RETURN_TIMEOUT = 30_sec; // number of seconds before dropped flag auto-returns

constexpr gtime_t CTF_TECH_TIMEOUT = 60_sec; // seconds before techs spawn again

constexpr int32_t CTF_DEFAULT_GRAPPLE_SPEED = 650;		// speed of grapple in flight
constexpr float	  CTF_DEFAULT_GRAPPLE_PULL_SPEED = 650; // speed player is pulled at

void CTFInit();
void CTFSpawn();
void CTFPrecache();
bool G_TeamplayEnabled();
void G_AdjustTeamScore(ctfteam_t team, int32_t offset);

void SP_info_player_team1(edict_t *self);
void SP_info_player_team2(edict_t *self);

const char *CTFTeamName(int team);
const char *CTFOtherTeamName(int team);
void		CTFAssignSkin(edict_t *ent, const char *s);
void		CTFAssignTeam(gclient_t *who);
edict_t	   *SelectCTFSpawnPoint(edict_t *ent, bool force_spawn);
bool		CTFPickup_Flag(edict_t *ent, edict_t *other);
void		CTFDrop_Flag(edict_t *ent, gitem_t *item);
void		CTFEffects(edict_t *player);
void		CTFCalcScores();
void		CTFCalcRankings(std::array<uint32_t, MAX_CLIENTS> &player_ranks); // [Paril-KEX]
void		CheckEndTDMLevel(); // [Paril-KEX]
void		SetCTFStats(edict_t *ent);
void		CTFDeadDropFlag(edict_t *self);
void		CTFScoreboardMessage(edict_t *ent, edict_t *killer);
void		CTFTeam_f(edict_t *ent);
void		CTFID_f(edict_t *ent);
#ifndef KEX_Q2_GAME
void		CTFSay_Team(edict_t *who, const char *msg);
#endif
void		CTFFlagSetup(edict_t *ent);
void		CTFResetFlag(int ctf_team);
void		CTFFragBonuses(edict_t *targ, edict_t *inflictor, edict_t *attacker);
void		CTFCheckHurtCarrier(edict_t *targ, edict_t *attacker);
void        CTFDirtyTeamMenu();

// GRAPPLE
void CTFWeapon_Grapple(edict_t *ent);
void CTFPlayerResetGrapple(edict_t *ent);
void CTFGrapplePull(edict_t *self);
void CTFResetGrapple(edict_t *self);

// TECH
gitem_t *CTFWhat_Tech(edict_t *ent);
bool	 CTFPickup_Tech(edict_t *ent, edict_t *other);
void	 CTFDrop_Tech(edict_t *ent, gitem_t *item);
void	 CTFDeadDropTech(edict_t *ent);
void	 CTFSetupTechSpawn();
int		 CTFApplyResistance(edict_t *ent, int dmg);
int		 CTFApplyStrength(edict_t *ent, int dmg);
bool	 CTFApplyStrengthSound(edict_t *ent);
bool	 CTFApplyHaste(edict_t *ent);
void	 CTFApplyHasteSound(edict_t *ent);
void	 CTFApplyRegeneration(edict_t *ent);
bool	 CTFHasRegeneration(edict_t *ent);
void	 CTFRespawnTech(edict_t *ent);
void	 CTFResetTech();

void CTFOpenJoinMenu(edict_t *ent);
bool CTFStartClient(edict_t *ent);
void CTFVoteYes(edict_t *ent);
void CTFVoteNo(edict_t *ent);
void CTFReady(edict_t *ent);
void CTFNotReady(edict_t *ent);
bool CTFNextMap();
bool CTFMatchSetup();
bool CTFMatchOn();
void CTFGhost(edict_t *ent);
void CTFAdmin(edict_t *ent);
bool CTFInMatch();
void CTFStats(edict_t *ent);
void CTFWarp(edict_t *ent);
void CTFBoot(edict_t *ent);
void CTFPlayerList(edict_t *ent);

bool CTFCheckRules();

void SP_misc_ctf_banner(edict_t *ent);
void SP_misc_ctf_small_banner(edict_t *ent);

void UpdateChaseCam(edict_t *ent);
void ChaseNext(edict_t *ent);
void ChasePrev(edict_t *ent);

void CTFObserver(edict_t *ent);

void SP_trigger_teleport(edict_t *ent);
void SP_info_teleport_destination(edict_t *ent);

void CTFSetPowerUpEffect(edict_t *ent, effects_t def);
