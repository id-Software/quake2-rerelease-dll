project('q2-rerelease', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3',
                     'cpp_std=c++14'])

add_global_arguments('-DKEX_Q2GAME_EXPORTS -DKEX_Q2GAME_DYNAMIC -DGAME_INCLUDE -DNO_FMT_SOURCE', language : 'cpp')

common_src = [
  'rerelease/bots/bot_debug.cpp', 
  'rerelease/bots/bot_exports.cpp', 
  'rerelease/bots/bot_think.cpp', 
  'rerelease/bots/bot_utils.cpp', 
  'rerelease/cg_main.cpp', 
  'rerelease/cg_screen.cpp', 
  'rerelease/ctf/g_ctf.cpp', 
  'rerelease/ctf/p_ctf_menu.cpp', 
  'rerelease/g_ai.cpp', 
  'rerelease/g_chase.cpp', 
  'rerelease/g_cmds.cpp', 
  'rerelease/g_combat.cpp', 
  'rerelease/g_func.cpp', 
  'rerelease/g_items.cpp', 
  'rerelease/g_main.cpp', 
  'rerelease/g_misc.cpp', 
  'rerelease/g_monster.cpp', 
  'rerelease/g_phys.cpp', 
  'rerelease/g_save.cpp', 
  'rerelease/g_spawn.cpp', 
  'rerelease/g_svcmds.cpp', 
  'rerelease/g_target.cpp', 
  'rerelease/g_trigger.cpp', 
  'rerelease/g_turret.cpp', 
  'rerelease/g_utils.cpp', 
  'rerelease/g_weapon.cpp', 
  'rerelease/m_actor.cpp', 
  'rerelease/m_arachnid.cpp', 
  'rerelease/m_berserk.cpp', 
  'rerelease/m_boss2.cpp', 
  'rerelease/m_boss3.cpp', 
  'rerelease/m_boss31.cpp', 
  'rerelease/m_boss32.cpp', 
  'rerelease/m_brain.cpp', 
  'rerelease/m_chick.cpp', 
  'rerelease/m_flipper.cpp', 
  'rerelease/m_float.cpp', 
  'rerelease/m_flyer.cpp', 
  'rerelease/m_gladiator.cpp', 
  'rerelease/m_guardian.cpp', 
  'rerelease/m_guncmdr.cpp', 
  'rerelease/m_gunner.cpp', 
  'rerelease/m_hover.cpp', 
  'rerelease/m_infantry.cpp', 
  'rerelease/m_insane.cpp', 
  'rerelease/m_medic.cpp', 
  'rerelease/m_move.cpp', 
  'rerelease/m_mutant.cpp', 
  'rerelease/m_parasite.cpp', 
  'rerelease/m_shambler.cpp', 
  'rerelease/m_soldier.cpp', 
  'rerelease/m_supertank.cpp', 
  'rerelease/m_tank.cpp', 
  'rerelease/p_client.cpp', 
  'rerelease/p_hud.cpp', 
  'rerelease/p_move.cpp', 
  'rerelease/p_trail.cpp', 
  'rerelease/p_view.cpp', 
  'rerelease/p_weapon.cpp', 
  'rerelease/q_std.cpp',
]

rogue_src = [
  'rerelease/rogue/g_rogue_combat.cpp', 
  'rerelease/rogue/g_rogue_func.cpp', 
  'rerelease/rogue/g_rogue_items.cpp', 
  'rerelease/rogue/g_rogue_misc.cpp', 
  'rerelease/rogue/g_rogue_monster.cpp', 
  'rerelease/rogue/g_rogue_newai.cpp', 
  'rerelease/rogue/g_rogue_newdm.cpp', 
  'rerelease/rogue/g_rogue_newfnc.cpp', 
  'rerelease/rogue/g_rogue_newtarg.cpp', 
  'rerelease/rogue/g_rogue_newtrig.cpp', 
  'rerelease/rogue/g_rogue_newweap.cpp', 
  'rerelease/rogue/g_rogue_phys.cpp', 
  'rerelease/rogue/g_rogue_spawn.cpp', 
  'rerelease/rogue/g_rogue_sphere.cpp', 
  'rerelease/rogue/g_rogue_utils.cpp', 
  'rerelease/rogue/m_rogue_carrier.cpp', 
  'rerelease/rogue/m_rogue_stalker.cpp', 
  'rerelease/rogue/m_rogue_turret.cpp', 
  'rerelease/rogue/m_rogue_widow.cpp', 
  'rerelease/rogue/m_rogue_widow2.cpp', 
  'rerelease/rogue/p_rogue_weapon.cpp', 
  'rerelease/rogue/rogue_dm_ball.cpp', 
  'rerelease/rogue/rogue_dm_tag.cpp', 
]

xatrix_src = [
  'rerelease/xatrix/g_xatrix_func.cpp', 
  'rerelease/xatrix/g_xatrix_items.cpp', 
  'rerelease/xatrix/g_xatrix_misc.cpp', 
  'rerelease/xatrix/g_xatrix_monster.cpp', 
  'rerelease/xatrix/g_xatrix_target.cpp', 
  'rerelease/xatrix/g_xatrix_weapon.cpp', 
  'rerelease/xatrix/m_xatrix_fixbot.cpp', 
  'rerelease/xatrix/m_xatrix_gekk.cpp', 
  'rerelease/xatrix/p_xatrix_weapon.cpp',
]

win32 = host_machine.system() == 'windows'
x86 = host_machine.cpu_family() == 'x86'

cpuremap = {
  'x86': win32 ? 'x86' : 'i386',
  'aarch64': 'arm64',
}

cpu = host_machine.cpu_family()
if cpu in cpuremap
  cpu = cpuremap[cpu]
endif

shared_library('game' + cpu, common_src,
  name_prefix:           '',
  #include_directories:   'inc',
  gnu_symbol_visibility: 'hidden',
)

# exe = executable('q2_rerelease', 'q2_rerelease.cpp',
#   install : true)

# test('basic', exe)
