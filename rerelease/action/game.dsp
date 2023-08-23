# Microsoft Developer Studio Project File - Name="game" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (ALPHA) Dynamic-Link Library" 0x0602

CFG=game - Win32 Debug Alpha
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "game.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "game.mak" CFG="game - Win32 Debug Alpha"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "game - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "game - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "game - Win32 Debug Alpha" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "game - Win32 Release Alpha" (based on "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "game - Win32 Ppro" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "game - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\release"
# PROP Intermediate_Dir "..\release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "."
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G5 /MT /W3 /GX /O2 /I "./" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winmm.lib /nologo /base:"0x20000000" /subsystem:windows /dll /machine:I386 /out:"..\release\gamex86.dll"
# SUBTRACT LINK32 /incremental:yes /debug

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\debug"
# PROP Intermediate_Dir "..\debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "."
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G5 /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "BUILDING_REF_GL" /D PATCHNAME="sl-dm" /D MODGAMEVERSION="baseq2" /FR /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib winmm.lib ws2_32.lib /nologo /base:"0x20000000" /subsystem:windows /dll /incremental:no /map:"../debug/gamex86.map" /debug /machine:I386 /out:"..\debug\gamex86.dll"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug Alpha"
# PROP BASE Intermediate_Dir "Debug Alpha"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\DebugAxp"
# PROP Intermediate_Dir ".\DebugAxp"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /MTd /c
# ADD CPP /nologo /Gt0 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "C_ONLY" /YX /FD /QA21164 /MTd /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /base:"0x20000000" /subsystem:windows /dll /debug /machine:ALPHA
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /base:"0x20000000" /subsystem:windows /dll /debug /machine:ALPHA /out:"..\DebugAxp/gameaxp.dll"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "game___W"
# PROP BASE Intermediate_Dir "game___W"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\ReleaseAXP"
# PROP Intermediate_Dir ".\ReleaseAXP"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MT /Gt0 /W3 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /Gt0 /W3 /GX /Zd /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "C_ONLY" /YX /FD /QA21164 /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /base:"0x20000000" /subsystem:windows /dll /machine:ALPHA /out:"..\Release/gamex86.dll"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib /nologo /base:"0x20000000" /subsystem:windows /dll /machine:ALPHA /out:"..\ReleaseAXP/gameaxp.dll"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "game___Win32_Ppro"
# PROP BASE Intermediate_Dir "game___Win32_Ppro"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "PPRO\Output\"
# PROP Intermediate_Dir "PPRO\Intermediate\"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /G5 /MT /W3 /GX /O2 /I "./" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /MD /W3 /O1 /Ob2 /I "./" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winmm.lib /nologo /base:"0x20000000" /subsystem:windows /dll /machine:I386 /out:"..\gamex86.dll"
# SUBTRACT BASE LINK32 /incremental:yes /debug
# ADD LINK32 kernel32.lib user32.lib /nologo /dll /pdb:none /map /machine:I386 /out:"PPRO\gamex86.dll"

!ENDIF 

# Begin Target

# Name "game - Win32 Release"
# Name "game - Win32 Debug"
# Name "game - Win32 Debug Alpha"
# Name "game - Win32 Release Alpha"
# Name "game - Win32 Ppro"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\a_ban.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\a_cmds.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\a_ctf.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\a_doorkick.c

!IF  "$(CFG)" == "game - Win32 Release"

# ADD CPP /FR

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

# ADD BASE CPP /FR
# ADD CPP /FR

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\a_game.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\a_items.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\a_match.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\a_menu.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\a_radio.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\a_team.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\a_tourney.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\a_vote.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\a_xcmds.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\a_xgame.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\a_xmenu.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\a_xvote.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cgf_sfx_glass.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_ai.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_chase.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_cmds.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_combat.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_func.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_items.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_main.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_misc.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_monster.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_phys.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_save.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_spawn.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_svcmds.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_target.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_trigger.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_turret.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_utils.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_weapon.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_xmisc.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lcchack.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_move.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\p_client.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\p_hud.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\p_trail.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\p_view.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\p_weapon.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\q_shared.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tng_flashlight.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tng_stats.c

!IF  "$(CFG)" == "game - Win32 Release"

!ELSEIF  "$(CFG)" == "game - Win32 Debug"

!ELSEIF  "$(CFG)" == "game - Win32 Debug Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Release Alpha"

!ELSEIF  "$(CFG)" == "game - Win32 Ppro"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\a_ban.h
# End Source File
# Begin Source File

SOURCE=.\a_ctf.h
# End Source File
# Begin Source File

SOURCE=.\a_game.h
# End Source File
# Begin Source File

SOURCE=.\a_match.h
# End Source File
# Begin Source File

SOURCE=.\a_menu.h
# End Source File
# Begin Source File

SOURCE=.\a_radio.h
# End Source File
# Begin Source File

SOURCE=.\a_team.h
# End Source File
# Begin Source File

SOURCE=.\a_tourney.h
# End Source File
# Begin Source File

SOURCE=.\a_vote.h
# End Source File
# Begin Source File

SOURCE=.\a_xcmds.h
# End Source File
# Begin Source File

SOURCE=.\a_xgame.h
# End Source File
# Begin Source File

SOURCE=.\a_xmenu.h
# End Source File
# Begin Source File

SOURCE=.\a_xvote.h
# End Source File
# Begin Source File

SOURCE=.\cgf_sfx_glass.h
# End Source File
# Begin Source File

SOURCE=.\g_local.h
# End Source File
# Begin Source File

SOURCE=.\g_xmisc.h
# End Source File
# Begin Source File

SOURCE=.\game.h
# End Source File
# Begin Source File

SOURCE=.\m_player.h
# End Source File
# Begin Source File

SOURCE=.\q_shared.h
# End Source File
# Begin Source File

SOURCE=.\tng_stats.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\game.def
# End Source File
# End Group
# End Target
# End Project
