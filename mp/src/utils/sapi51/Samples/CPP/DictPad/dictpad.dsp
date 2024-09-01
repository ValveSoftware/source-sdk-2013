# Microsoft Developer Studio Project File - Name="dictpad" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=dictpad - Win32 Debug x86
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "dictpad.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "dictpad.mak" CFG="dictpad - Win32 Debug x86"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "dictpad - Win32 Debug x86" (based on "Win32 (x86) Application")
!MESSAGE "dictpad - Win32 Release x86" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Desktop"
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "dictpad - Win32 Debug x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "dictpad___Win32_Debug_x86"
# PROP BASE Intermediate_Dir "dictpad___Win32_Debug_x86"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_x86"
# PROP Intermediate_Dir "Debug_x86"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\..\include" /I "..\..\..\..\ddk\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\..\..\ddk\include" /I "..\..\..\include" /I "..\Common" /I "..\..\Common" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D _WIN32_WINNT=0x0500 /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib version.lib imm32.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"LIBCMTD" /pdbtype:sept /libpath:"..\..\..\lib\i386"

!ELSEIF  "$(CFG)" == "dictpad - Win32 Release x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "dictpad___Win32_Release_x86"
# PROP BASE Intermediate_Dir "dictpad___Win32_Release_x86"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_x86"
# PROP Intermediate_Dir "Release_x86"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /I "..\..\..\include" /I "..\..\..\..\ddk\include" /I "..\..\..\..\patch\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /W3 /GX /Zi /O2 /I "..\Common" /I "..\..\Common" /I "..\..\..\..\ddk\include" /I "..\..\..\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D _WIN32_WINNT=0x0500 /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib version.lib imm32.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"LIBCMT" /libpath:"..\..\..\lib\i386" /debugtype:cv,fixup
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "dictpad - Win32 Debug x86"
# Name "dictpad - Win32 Release x86"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\candidatelist.cpp
# End Source File
# Begin Source File

SOURCE=..\common\ComponentVersionDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\DictationRun.cpp
# End Source File
# Begin Source File

SOURCE=.\dictpad.cpp

!IF  "$(CFG)" == "dictpad - Win32 Debug x86"

# ADD CPP /I "..\common"

!ELSEIF  "$(CFG)" == "dictpad - Win32 Release x86"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dictpad.rc
# ADD BASE RSC /l 0x409
# ADD RSC /l 0x409 /i "..\..\..\..\build" /i "..\Common" /i "..\..\Common"
# End Source File
# Begin Source File

SOURCE=.\dictpad_sapi.cpp
# End Source File
# Begin Source File

SOURCE=.\phrasereplace.cpp
# End Source File
# Begin Source File

SOURCE=.\recomgr.cpp
# End Source File
# Begin Source File

SOURCE=.\resultcontainer.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\textrun.cpp
# End Source File
# Begin Source File

SOURCE=.\TextRunList.cpp
# End Source File
# Begin Source File

SOURCE=.\tom_i.c
# SUBTRACT BASE CPP /YX /Yc /Yu
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\candidatelist.h
# End Source File
# Begin Source File

SOURCE=.\DictationRun.h
# End Source File
# Begin Source File

SOURCE=.\dictpad.h
# End Source File
# Begin Source File

SOURCE=.\phrasereplace.h
# End Source File
# Begin Source File

SOURCE=.\recomgr.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\resultcontainer.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\textrun.h
# End Source File
# Begin Source File

SOURCE=.\TextRunList.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\cursor1.cur
# End Source File
# Begin Source File

SOURCE=.\dictpad.ico
# End Source File
# Begin Source File

SOURCE=.\small.ico
# End Source File
# End Group
# Begin Group "Grammar"

# PROP Default_Filter "xml"
# Begin Source File

SOURCE=.\chs_cmdmode.xml

!IF  "$(CFG)" == "dictpad - Win32 Debug x86"

# Begin Custom Build
InputDir=.
InputPath=.\chs_cmdmode.xml

"$(InputDir)\chs_cmdmode.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "dictpad - Win32 Release x86"

# Begin Custom Build
InputDir=.
InputPath=.\chs_cmdmode.xml

"$(InputDir)\chs_cmdmode.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\chs_dictmode.xml

!IF  "$(CFG)" == "dictpad - Win32 Debug x86"

# Begin Custom Build
InputDir=.
InputPath=.\chs_dictmode.xml

"$(InputDir)\chs_dictmode.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "dictpad - Win32 Release x86"

# Begin Custom Build
InputDir=.
InputPath=.\chs_dictmode.xml

"$(InputDir)\chs_dictmode.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\cmdmode.xml

!IF  "$(CFG)" == "dictpad - Win32 Debug x86"

# Begin Custom Build
ProjDir=.
InputPath=.\cmdmode.xml
InputName=cmdmode

BuildCmds= \
	..\..\..\bin\gc /h cmdmode.h $(InputName)

"$(ProjDir)\cmdmode.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\cmdmode.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "dictpad - Win32 Release x86"

# Begin Custom Build
ProjDir=.
InputPath=.\cmdmode.xml
InputName=cmdmode

BuildCmds= \
	..\..\..\bin\gc /h cmdmode.h $(InputName)

"$(ProjDir)\cmdmode.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\cmdmode.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dictmode.xml

!IF  "$(CFG)" == "dictpad - Win32 Debug x86"

# Begin Custom Build
ProjDir=.
InputPath=.\dictmode.xml
InputName=dictmode

BuildCmds= \
	..\..\..\bin\gc /h dictmode.h  $(InputName)

"$(ProjDir)\dictmode.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\dictmode.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "dictpad - Win32 Release x86"

# Begin Custom Build
ProjDir=.
InputPath=.\dictmode.xml
InputName=dictmode

BuildCmds= \
	..\..\..\bin\gc /h dictmode.h  $(InputName)

"$(ProjDir)\dictmode.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\dictmode.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpn_cmdmode.xml

!IF  "$(CFG)" == "dictpad - Win32 Debug x86"

# Begin Custom Build
InputDir=.
InputPath=.\jpn_cmdmode.xml

"$(InputDir)\jpn_cmdmode.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "dictpad - Win32 Release x86"

# Begin Custom Build
InputDir=.
InputPath=.\jpn_cmdmode.xml

"$(InputDir)\jpn_cmdmode.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jpn_dictmode.xml

!IF  "$(CFG)" == "dictpad - Win32 Debug x86"

# Begin Custom Build
InputDir=.
InputPath=.\jpn_dictmode.xml

"$(InputDir)\jpn_dictmode.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "dictpad - Win32 Release x86"

# Begin Custom Build
InputDir=.
InputPath=.\jpn_dictmode.xml

"$(InputDir)\jpn_dictmode.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\cmdmode.cfg
# End Source File
# Begin Source File

SOURCE=.\dictmode.cfg
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
