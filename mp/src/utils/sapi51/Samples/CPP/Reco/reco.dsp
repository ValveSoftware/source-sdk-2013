# Microsoft Developer Studio Project File - Name="Reco" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Reco - Win32 Debug x86
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "reco.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "reco.mak" CFG="Reco - Win32 Debug x86"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Reco - Win32 Debug x86" (based on "Win32 (x86) Application")
!MESSAGE "Reco - Win32 Release x86" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "reco"
# PROP Scc_LocalPath "Desktop"
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Reco - Win32 Debug x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Reco___Win32_Debug_x86"
# PROP BASE Intermediate_Dir "Reco___Win32_Debug_x86"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_x86"
# PROP Intermediate_Dir "Debug_x86"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\..\include" /I "..\..\..\..\ddk\include" /I "..\..\..\..\test\engext" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D _WIN32_WINNT=0x0500 /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\lib\i386"

!ELSEIF  "$(CFG)" == "Reco - Win32 Release x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Reco___Win32_Release_x86"
# PROP BASE Intermediate_Dir "Reco___Win32_Release_x86"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_x86"
# PROP Intermediate_Dir "Release_x86"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /I "..\..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\..\..\include" /I "..\..\..\..\ddk\include" /I "..\..\..\..\test\engext" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D _WIN32_WINNT=0x0500 /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386 /libpath:"..\..\..\lib\i386"

!ENDIF 

# Begin Target

# Name "Reco - Win32 Debug x86"
# Name "Reco - Win32 Release x86"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\lmadapt.cpp
# End Source File
# Begin Source File

SOURCE=.\Reco.cpp
# End Source File
# Begin Source File

SOURCE=.\Reco.rc
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Reco.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\Reco.ico
# End Source File
# Begin Source File

SOURCE=.\small.ico
# End Source File
# Begin Source File

SOURCE=.\tv.cfg
# End Source File
# End Group
# Begin Group "Grammar"

# PROP Default_Filter "*.xml"
# Begin Source File

SOURCE=.\chs_sol.xml

!IF  "$(CFG)" == "Reco - Win32 Debug x86"

USERDEP__CHS_S="..\..\..\idl\sapi.idl"	
# Begin Custom Build
ProjDir=.
InputPath=.\chs_sol.xml

"$(ProjDir)\chs_sol.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Reco - Win32 Release x86"

# Begin Custom Build
ProjDir=.
InputPath=.\chs_sol.xml

"$(ProjDir)\chs_sol.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpn_sol.xml

!IF  "$(CFG)" == "Reco - Win32 Debug x86"

USERDEP__JPN_S="..\..\..\idl\sapi.idl"	
# Begin Custom Build
ProjDir=.
InputPath=.\jpn_sol.xml

"$(ProjDir)\jpn_sol.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Reco - Win32 Release x86"

# Begin Custom Build
ProjDir=.
InputPath=.\jpn_sol.xml

"$(ProjDir)\jpn_sol.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sol.xml

!IF  "$(CFG)" == "Reco - Win32 Debug x86"

USERDEP__SOL_X="..\..\..\idl\sapi.idl"	
# Begin Custom Build
ProjDir=.
InputPath=.\sol.xml
InputName=sol

"$(ProjDir)\sol.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputName)

# End Custom Build

!ELSEIF  "$(CFG)" == "Reco - Win32 Release x86"

USERDEP__SOL_X="..\..\..\idl\sapi.idl"	
# Begin Custom Build
ProjDir=.
InputPath=.\sol.xml
InputName=sol

"$(ProjDir)\sol.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputName)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\jpn_reco.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\sol.cfg
# End Source File
# End Target
# End Project
