project('q2-rerelease', 'cpp', 'c', version : '0.1', default_options : ['warning_level=3','cpp_std=c++17'], license: 'GPLv2')

add_global_arguments('-DKEX_Q2GAME_EXPORTS -DKEX_Q2GAME_DYNAMIC -DGAME_INCLUDE -DNO_FMT_SOURCE', 
'-Wno-unused-result', 
'-lm', 
'-fno-strict-aliasing', 
'-DC_ONLY', 
'-Wno-strict-prototypes', 
language : 'cpp')

cpp = meson.get_compiler('cpp')
c = meson.get_compiler('cpp')

m_dep = c.find_library('m', required : true)

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

action_src = [
  'rerelease/action/a_cmds.c',
  'rerelease/action/a_ctf.c',
  'rerelease/action/a_ctf.h',
  'rerelease/action/a_dom.c',
  'rerelease/action/a_dom.h',
  'rerelease/action/a_doorkick.c',
  'rerelease/action/a_game.c',
  'rerelease/action/a_game.h',
  'rerelease/action/a_items.c',
  'rerelease/action/a_match.c',
  'rerelease/action/a_match.h',
  'rerelease/action/a_menu.c',
  'rerelease/action/a_menu.h',
  'rerelease/action/a_radio.c',
  'rerelease/action/a_radio.h',
  'rerelease/action/a_team.c',
  'rerelease/action/a_team.h',
  'rerelease/action/a_tourney.c',
  'rerelease/action/a_tourney.h',
  'rerelease/action/a_vote.c',
  'rerelease/action/a_vote.h',
  'rerelease/action/a_xcmds.c',
  'rerelease/action/a_xcmds.h',
  'rerelease/action/a_xgame.c',
  'rerelease/action/a_xgame.h',
  'rerelease/action/a_xmenu.c',
  'rerelease/action/a_xmenu.h',
  'rerelease/action/a_xvote.c',
  'rerelease/action/a_xvote.h',
  'rerelease/action/cgf_sfx_glass.c',
  'rerelease/action/cgf_sfx_glass.h',
  'rerelease/action/game.h',
  'rerelease/action/g_chase.c',
  'rerelease/action/g_cmds.c',
  'rerelease/action/g_combat.c',
  'rerelease/action/g_func.c',
  'rerelease/action/g_grapple.c',
  'rerelease/action/g_grapple.h',
  'rerelease/action/g_items.c',
  'rerelease/action/g_local.h',
  'rerelease/action/g_main.c',
  'rerelease/action/g_misc.c',
  'rerelease/action/g_phys.c',
  'rerelease/action/g_save.c',
  'rerelease/action/g_spawn.c',
  'rerelease/action/g_svcmds.c',
  'rerelease/action/g_target.c',
  'rerelease/action/g_trigger.c',
  'rerelease/action/g_utils.c',
  'rerelease/action/g_weapon.c',
  'rerelease/action/m_player.h',
  'rerelease/action/p_antilag.c',
  'rerelease/action/p_antilag.h',
  'rerelease/action/p_client.c',
  'rerelease/action/p_hud.c',
  'rerelease/action/p_view.c',
  'rerelease/action/p_weapon.c',
  'rerelease/action/q_ghud.h',
  'rerelease/action/q_shared.c',
  'rerelease/action/q_shared.h',
  'rerelease/action/tng_balancer.c',
  'rerelease/action/tng_balancer.h',
  'rerelease/action/tng_flashlight.c',
  'rerelease/action/tng_ini.c',
  'rerelease/action/tng_ini.h',
  'rerelease/action/tng_jump.c',
  'rerelease/action/tng_jump.h',
  'rerelease/action/tng_stats.c',
  'rerelease/action/tng_stats.h',
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

common_args = ['-DHAVE_CONFIG_H']
if not win32
  common_args += '-D_GNU_SOURCE'
endif

engine_args = []
if win32
  engine_args += '-D_WIN32_WINNT=0x0601'
endif

common_link_args = []
exe_link_args = []
dll_link_args = []

if c.get_argument_syntax() == 'gcc'
  if x86
    add_global_arguments('-msse2', '-mfpmath=sse', language: 'c')
  endif

  test_args = [
    '-fsigned-char',
    '-Wpointer-arith',
    '-Wformat-security',
    '-Werror=vla',
    '-Wstrict-prototypes',
  ]

  common_args += c.get_supported_arguments(test_args)
  engine_args += c.get_supported_arguments(['-Wmissing-prototypes'])

  if win32
    common_args += '-D__USE_MINGW_ANSI_STDIO=1'
    common_link_args += '-Wl,--nxcompat,--dynamicbase'

    if cpu == 'x86_64'
      exe_link_args += '-Wl,--high-entropy-va,--image-base=0x140000000'
      dll_link_args += '-Wl,--high-entropy-va,--image-base=0x180000000'
    endif

    dll_link_args += '-static-libgcc'
  endif
elif c.get_id() == 'msvc'
  common_args += ['/D_USE_MATH_DEFINES', '/wd4146', '/wd4244', '/wd4305']
endif


add_project_arguments(common_args, language: 'c')
add_project_link_arguments(common_link_args, language: 'c')

fallback_opt = ['default_library=static']

shared_library('game' + cpu, action_src,
  name_prefix:            '',
  gnu_symbol_visibility:  'hidden',
  dependencies:           m_dep,
)
