cmake_minimum_required(VERSION 3.23.1)
project(game)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

set(GAME_SRC 
    ${CMAKE_CURRENT_SOURCE_DIR}/action/a_game.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/action/a_team.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/action/a_balancer.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/action/cgf_sfx_glass.cpp 

    ${CMAKE_CURRENT_SOURCE_DIR}/bots/bot_debug.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/bots/bot_exports.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/bots/bot_think.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/bots/bot_utils.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/cg_main.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/cg_screen.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/ctf/g_ctf.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/ctf/p_ctf_menu.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/g_ai.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/g_chase.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/g_cmds.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/g_combat.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/g_func.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/g_items.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/g_main.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/g_misc.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/g_monster.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/g_phys.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/g_save.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/g_spawn.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/g_svcmds.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/g_target.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/g_trigger.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/g_turret.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/g_utils.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/g_weapon.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/m_actor.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/m_infantry.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/m_medic.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/m_move.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/p_client.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/p_hud.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/p_move.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/p_trail.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/p_view.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/p_weapon.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/q_std.cpp 

    ${CMAKE_CURRENT_SOURCE_DIR}/rogue/g_rogue_combat.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/rogue/g_rogue_func.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/rogue/g_rogue_items.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/rogue/g_rogue_misc.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/rogue/g_rogue_monster.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/rogue/g_rogue_newai.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/rogue/g_rogue_newdm.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/rogue/g_rogue_newfnc.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/rogue/g_rogue_newtarg.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/rogue/g_rogue_newtrig.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/rogue/g_rogue_newweap.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/rogue/g_rogue_phys.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/rogue/g_rogue_spawn.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/rogue/g_rogue_sphere.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/rogue/g_rogue_utils.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/rogue/m_rogue_carrier.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/rogue/m_rogue_stalker.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/rogue/m_rogue_turret.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/rogue/m_rogue_widow.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/rogue/m_rogue_widow2.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/rogue/p_rogue_weapon.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/rogue/rogue_dm_ball.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/rogue/rogue_dm_tag.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/xatrix/g_xatrix_func.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/xatrix/g_xatrix_items.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/xatrix/g_xatrix_misc.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/xatrix/g_xatrix_monster.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/xatrix/g_xatrix_target.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/xatrix/g_xatrix_weapon.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/xatrix/m_xatrix_fixbot.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/xatrix/m_xatrix_gekk.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/xatrix/p_xatrix_weapon.cpp 
)

set(CMAKE_CXX_FLAGS "-lm -ldl -fPIE -pie -lfmt" CACHE STRING "compile flags" FORCE)
add_compile_options(-DKEX_Q2_GAME -DKEX_Q2GAME_EXPORTS -DNO_FMT_SOURCE -DKEX_Q2GAME_DYNAMIC)
add_library(game SHARED ${GAME_SRC} )
#target_include_directories(game PUBLIC /usr/local/Cellar/jsoncpp/1.9.5/include /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX13.3.sdk/usr/include/c++/v1/)
target_include_directories(game PUBLIC /usr/include/jsoncpp)

set_target_properties(game PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
set_target_properties(game PROPERTIES PREFIX "")
set_target_properties(game PROPERTIES OUTPUT_NAME "game")

# Fetch fmt locally
find_package(fmt CONFIG REQUIRED)

# Fetch jsoncpp locally
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)
find_library(JSONCPP jsoncpp)

target_link_libraries(
    game 
    fmt::fmt 
    ${JSONCPP}
    )