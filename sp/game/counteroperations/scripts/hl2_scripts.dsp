# Microsoft Developer Studio Project File - Name="hl2_scripts" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 60000
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=hl2_scripts - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "hl2_scripts.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "hl2_scripts.mak" CFG="hl2_scripts - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "hl2_scripts - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "hl2_scripts - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/HL2/release/dev/hl2/scripts", ACAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "hl2_scripts - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "hl2_scripts - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ  /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ  /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "hl2_scripts - Win32 Release"
# Name "hl2_scripts - Win32 Debug"
# Begin Group "AI Schedules"

# PROP Default_Filter "*.sch"
# Begin Source File

SOURCE=.\barney.sch
# End Source File
# Begin Source File

SOURCE=.\citizen.sch
# End Source File
# Begin Source File

SOURCE=.\default.sch
# End Source File
# Begin Source File

SOURCE=.\kungfu_owen.sch
# End Source File
# Begin Source File

SOURCE=.\lead_monster.sch
# End Source File
# Begin Source File

SOURCE=.\metro_police.sch
# End Source File
# Begin Source File

SOURCE=.\npc_assassin.sch
# End Source File
# Begin Source File

SOURCE=.\npc_barnacle.sch
# End Source File
# Begin Source File

SOURCE=.\npc_barney.sch
# End Source File
# Begin Source File

SOURCE=.\npc_bullsquid.sch
# End Source File
# Begin Source File

SOURCE=.\npc_combine.sch
# End Source File
# Begin Source File

SOURCE=.\npc_conscript.sch
# End Source File
# Begin Source File

SOURCE=.\npc_headcrab.sch
# End Source File
# Begin Source File

SOURCE=.\npc_manhack.sch
# End Source File
# Begin Source File

SOURCE=.\npc_mortarsynth.sch
# End Source File
# Begin Source File

SOURCE=.\npc_odell.sch
# End Source File
# Begin Source File

SOURCE=.\npc_stalker.sch
# End Source File
# Begin Source File

SOURCE=.\npc_vortigaunt.sch
# End Source File
# Begin Source File

SOURCE=.\npc_wscanner.sch
# End Source File
# Begin Source File

SOURCE=.\npc_zombie.sch
# End Source File
# Begin Source File

SOURCE=.\odell.sch
# End Source File
# Begin Source File

SOURCE=.\proto_sniper.sch
# End Source File
# Begin Source File

SOURCE=.\sacktick.sch
# End Source File
# Begin Source File

SOURCE=.\scanner.sch
# End Source File
# Begin Source File

SOURCE=.\talk_monster.sch
# End Source File
# End Group
# Begin Group "Weapon Scripts"

# PROP Default_Filter "*.txt"
# Begin Source File

SOURCE=.\weapon_ar1.txt
# End Source File
# Begin Source File

SOURCE=.\weapon_ar2.txt
# End Source File
# Begin Source File

SOURCE=.\weapon_binoculars.txt
# End Source File
# Begin Source File

SOURCE=.\weapon_brickbat.txt
# End Source File
# Begin Source File

SOURCE=.\weapon_flaregun.txt
# End Source File
# Begin Source File

SOURCE=.\weapon_hmg1.txt
# End Source File
# Begin Source File

SOURCE=.\weapon_iceaxe.txt
# End Source File
# Begin Source File

SOURCE=.\weapon_ml.txt
# End Source File
# Begin Source File

SOURCE=.\weapon_molotov.txt
# End Source File
# Begin Source File

SOURCE=.\weapon_physgun.txt
# End Source File
# Begin Source File

SOURCE=.\weapon_shotgun.txt
# End Source File
# Begin Source File

SOURCE=.\weapon_slam.txt
# End Source File
# Begin Source File

SOURCE=.\weapon_smg1.txt
# End Source File
# Begin Source File

SOURCE=.\weapon_smg2.txt
# End Source File
# Begin Source File

SOURCE=.\weapon_sniperrifle.txt
# End Source File
# Begin Source File

SOURCE=.\weapon_stunstick.txt
# End Source File
# End Group
# Begin Group "Misc Scripts"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\640_hud.txt
# End Source File
# Begin Source File

SOURCE=.\kb_act.lst
# End Source File
# Begin Source File

SOURCE=.\kb_def.lst
# End Source File
# Begin Source File

SOURCE=.\kb_keys.lst
# End Source File
# Begin Source File

SOURCE=.\liblist.gam
# End Source File
# Begin Source File

SOURCE=.\materials.txt
# End Source File
# Begin Source File

SOURCE=.\rooms.lst
# End Source File
# Begin Source File

SOURCE=.\sentences.txt
# End Source File
# Begin Source File

SOURCE=.\settings.scr
# End Source File
# Begin Source File

SOURCE=.\titles.txt
# End Source File
# Begin Source File

SOURCE=.\woncomm.lst
# End Source File
# End Group
# End Target
# End Project
