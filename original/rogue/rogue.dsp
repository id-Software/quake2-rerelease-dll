# Microsoft Developer Studio Project File - Name="rogue" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (ALPHA) Dynamic-Link Library" 0x0602

CFG=rogue - Win32 Debug Alpha
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "rogue.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "rogue.mak" CFG="rogue - Win32 Debug Alpha"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "rogue - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "rogue - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "rogue - Win32 Debug Alpha" (based on\
 "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE "rogue - Win32 Release Alpha" (based on\
 "Win32 (ALPHA) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
# PROP WCE_Configuration "H/PC Ver. 2.00"

!IF  "$(CFG)" == "rogue - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\release"
# PROP Intermediate_Dir ".\release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W1 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib winmm.lib /nologo /subsystem:windows /dll /machine:I386 /out:".\release\gamex86.dll"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\debug"
# PROP Intermediate_Dir ".\debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib winmm.lib /nologo /subsystem:windows /dll /incremental:no /map /debug /machine:I386 /out:".\debug\gamex86.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "rogue___"
# PROP BASE Intermediate_Dir "rogue___"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\DebugAXP"
# PROP Intermediate_Dir ".\DebugAXP"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
CPP=cl.exe
# ADD BASE CPP /nologo /Gt0 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /MTd /c
# ADD CPP /nologo /Gt0 /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "C_ONLY" /YX /FD /MTd /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /map /debug /machine:ALPHA /out:".\debug\gamex86.dll" /pdbtype:sept
# ADD LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /map /debug /machine:ALPHA /out:".\debugAXP\gameaxp.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "rogue__0"
# PROP BASE Intermediate_Dir "rogue__0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\ReleaseAXP"
# PROP Intermediate_Dir ".\ReleaseAXP"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
CPP=cl.exe
# ADD BASE CPP /nologo /MT /Gt0 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /Gt0 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "C_ONLY" /YX /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:ALPHA /out:".\release\gamex86.dll"
# ADD LINK32 winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:ALPHA /out:".\ReleaseAXP\gameaxp.dll"

!ENDIF 

# Begin Target

# Name "rogue - Win32 Release"
# Name "rogue - Win32 Debug"
# Name "rogue - Win32 Debug Alpha"
# Name "rogue - Win32 Release Alpha"
# Begin Group "Source Files"

# PROP Default_Filter "*.c"
# Begin Source File

SOURCE=.\dm_ball.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_DM_BA=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_DM_BA=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dm_tag.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_DM_TA=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_DM_TA=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_ai.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_AI_=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_AI_=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_chase.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_CHA=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_CHA=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_cmds.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_CMD=\
	".\g_local.h"\
	".\game.h"\
	".\m_player.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_CMD=\
	".\g_local.h"\
	".\game.h"\
	".\m_player.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_combat.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_COM=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_COM=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_func.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_FUN=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_FUN=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_items.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_ITE=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_ITE=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_main.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_MAI=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_MAI=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_misc.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_MIS=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_MIS=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_monster.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_MON=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_MON=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_newai.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_NEW=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_NEW=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_newdm.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_NEWD=\
	".\g_local.h"\
	".\game.h"\
	".\m_player.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_NEWD=\
	".\g_local.h"\
	".\game.h"\
	".\m_player.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_newfnc.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_NEWF=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_NEWF=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_newtarg.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_NEWT=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_NEWT=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_newtrig.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_NEWTR=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_NEWTR=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_newweap.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_NEWW=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_NEWW=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_phys.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_PHY=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_PHY=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_save.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_SAV=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_SAV=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_spawn.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_SPA=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_SPA=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_sphere.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_SPH=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_SPH=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_svcmds.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_SVC=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_SVC=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_target.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_TAR=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_TAR=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_trigger.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_TRI=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_TRI=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_turret.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_TUR=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_TUR=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_utils.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_UTI=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_UTI=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\g_weapon.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_G_WEA=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_G_WEA=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_actor.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_ACT=\
	".\g_local.h"\
	".\game.h"\
	".\m_actor.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_ACT=\
	".\g_local.h"\
	".\game.h"\
	".\m_actor.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_berserk.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_BER=\
	".\g_local.h"\
	".\game.h"\
	".\m_berserk.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_BER=\
	".\g_local.h"\
	".\game.h"\
	".\m_berserk.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_boss2.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_BOS=\
	".\g_local.h"\
	".\game.h"\
	".\m_boss2.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_BOS=\
	".\g_local.h"\
	".\game.h"\
	".\m_boss2.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_boss3.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_BOSS=\
	".\g_local.h"\
	".\game.h"\
	".\m_boss32.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_BOSS=\
	".\g_local.h"\
	".\game.h"\
	".\m_boss32.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_boss31.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_BOSS3=\
	".\g_local.h"\
	".\game.h"\
	".\m_boss31.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_BOSS3=\
	".\g_local.h"\
	".\game.h"\
	".\m_boss31.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_boss32.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_BOSS32=\
	".\g_local.h"\
	".\game.h"\
	".\m_boss32.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_BOSS32=\
	".\g_local.h"\
	".\game.h"\
	".\m_boss32.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_brain.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_BRA=\
	".\g_local.h"\
	".\game.h"\
	".\m_brain.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_BRA=\
	".\g_local.h"\
	".\game.h"\
	".\m_brain.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_carrier.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_CAR=\
	".\g_local.h"\
	".\game.h"\
	".\m_carrier.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_CAR=\
	".\g_local.h"\
	".\game.h"\
	".\m_carrier.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_chick.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_CHI=\
	".\g_local.h"\
	".\game.h"\
	".\m_chick.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_CHI=\
	".\g_local.h"\
	".\game.h"\
	".\m_chick.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_flash.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_FLA=\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_FLA=\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_flipper.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_FLI=\
	".\g_local.h"\
	".\game.h"\
	".\m_flipper.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_FLI=\
	".\g_local.h"\
	".\game.h"\
	".\m_flipper.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_float.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_FLO=\
	".\g_local.h"\
	".\game.h"\
	".\m_float.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_FLO=\
	".\g_local.h"\
	".\game.h"\
	".\m_float.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_flyer.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_FLY=\
	".\g_local.h"\
	".\game.h"\
	".\m_flyer.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_FLY=\
	".\g_local.h"\
	".\game.h"\
	".\m_flyer.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_gladiator.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_GLA=\
	".\g_local.h"\
	".\game.h"\
	".\m_gladiator.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_GLA=\
	".\g_local.h"\
	".\game.h"\
	".\m_gladiator.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_gunner.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_GUN=\
	".\g_local.h"\
	".\game.h"\
	".\m_gunner.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_GUN=\
	".\g_local.h"\
	".\game.h"\
	".\m_gunner.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_hover.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_HOV=\
	".\g_local.h"\
	".\game.h"\
	".\m_hover.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_HOV=\
	".\g_local.h"\
	".\game.h"\
	".\m_hover.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_infantry.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_INF=\
	".\g_local.h"\
	".\game.h"\
	".\m_infantry.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_INF=\
	".\g_local.h"\
	".\game.h"\
	".\m_infantry.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_insane.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_INS=\
	".\g_local.h"\
	".\game.h"\
	".\m_insane.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_INS=\
	".\g_local.h"\
	".\game.h"\
	".\m_insane.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_medic.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_MED=\
	".\g_local.h"\
	".\game.h"\
	".\m_medic.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_MED=\
	".\g_local.h"\
	".\game.h"\
	".\m_medic.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_move.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_MOV=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_MOV=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_mutant.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_MUT=\
	".\g_local.h"\
	".\game.h"\
	".\m_mutant.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_MUT=\
	".\g_local.h"\
	".\game.h"\
	".\m_mutant.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_parasite.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_PAR=\
	".\g_local.h"\
	".\game.h"\
	".\m_parasite.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_PAR=\
	".\g_local.h"\
	".\game.h"\
	".\m_parasite.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_soldier.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_SOL=\
	".\g_local.h"\
	".\game.h"\
	".\m_soldier.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_SOL=\
	".\g_local.h"\
	".\game.h"\
	".\m_soldier.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_stalker.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_STA=\
	".\g_local.h"\
	".\game.h"\
	".\m_stalker.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_STA=\
	".\g_local.h"\
	".\game.h"\
	".\m_stalker.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_supertank.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_SUP=\
	".\g_local.h"\
	".\game.h"\
	".\m_supertank.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_SUP=\
	".\g_local.h"\
	".\game.h"\
	".\m_supertank.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_tank.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_TAN=\
	".\g_local.h"\
	".\game.h"\
	".\m_tank.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_TAN=\
	".\g_local.h"\
	".\game.h"\
	".\m_tank.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_turret.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_TUR=\
	".\g_local.h"\
	".\game.h"\
	".\m_turret.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_TUR=\
	".\g_local.h"\
	".\game.h"\
	".\m_turret.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_widow.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_WID=\
	".\g_local.h"\
	".\game.h"\
	".\m_widow.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_WID=\
	".\g_local.h"\
	".\game.h"\
	".\m_widow.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\m_widow2.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_M_WIDO=\
	".\g_local.h"\
	".\game.h"\
	".\m_widow2.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_M_WIDO=\
	".\g_local.h"\
	".\game.h"\
	".\m_widow2.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\p_client.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_P_CLI=\
	".\g_local.h"\
	".\game.h"\
	".\m_player.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_P_CLI=\
	".\g_local.h"\
	".\game.h"\
	".\m_player.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\p_hud.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_P_HUD=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_P_HUD=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\p_trail.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_P_TRA=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_P_TRA=\
	".\g_local.h"\
	".\game.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\p_view.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_P_VIE=\
	".\g_local.h"\
	".\game.h"\
	".\m_player.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_P_VIE=\
	".\g_local.h"\
	".\game.h"\
	".\m_player.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\p_weapon.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_P_WEA=\
	".\g_local.h"\
	".\game.h"\
	".\m_player.h"\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_P_WEA=\
	".\g_local.h"\
	".\game.h"\
	".\m_player.h"\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\q_shared.c

!IF  "$(CFG)" == "rogue - Win32 Release"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug"

!ELSEIF  "$(CFG)" == "rogue - Win32 Debug Alpha"

DEP_CPP_Q_SHA=\
	".\q_shared.h"\
	

!ELSEIF  "$(CFG)" == "rogue - Win32 Release Alpha"

DEP_CPP_Q_SHA=\
	".\q_shared.h"\
	

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\g_local.h
# End Source File
# Begin Source File

SOURCE=.\game.h
# End Source File
# Begin Source File

SOURCE=.\m_actor.h
# End Source File
# Begin Source File

SOURCE=.\m_berserk.h
# End Source File
# Begin Source File

SOURCE=.\m_boss2.h
# End Source File
# Begin Source File

SOURCE=.\m_boss31.h
# End Source File
# Begin Source File

SOURCE=.\m_boss32.h
# End Source File
# Begin Source File

SOURCE=.\m_brain.h
# End Source File
# Begin Source File

SOURCE=.\m_chick.h
# End Source File
# Begin Source File

SOURCE=.\m_fixbot.h
# End Source File
# Begin Source File

SOURCE=.\m_flipper.h
# End Source File
# Begin Source File

SOURCE=.\m_float.h
# End Source File
# Begin Source File

SOURCE=.\m_flyer.h
# End Source File
# Begin Source File

SOURCE=.\m_gekk.h
# End Source File
# Begin Source File

SOURCE=.\m_gladiator.h
# End Source File
# Begin Source File

SOURCE=.\m_gunner.h
# End Source File
# Begin Source File

SOURCE=.\m_hover.h
# End Source File
# Begin Source File

SOURCE=.\m_infantry.h
# End Source File
# Begin Source File

SOURCE=.\m_insane.h
# End Source File
# Begin Source File

SOURCE=.\m_medic.h
# End Source File
# Begin Source File

SOURCE=.\m_mutant.h
# End Source File
# Begin Source File

SOURCE=.\m_parasite.h
# End Source File
# Begin Source File

SOURCE=.\m_player.h
# End Source File
# Begin Source File

SOURCE=.\m_rider.h
# End Source File
# Begin Source File

SOURCE=.\m_soldier.h
# End Source File
# Begin Source File

SOURCE=.\m_soldierh.h
# End Source File
# Begin Source File

SOURCE=.\m_supertank.h
# End Source File
# Begin Source File

SOURCE=.\m_tank.h
# End Source File
# Begin Source File

SOURCE=.\q_shared.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "*.def,*.res"
# Begin Source File

SOURCE=.\rogue.def
# End Source File
# End Group
# End Target
# End Project
