# Microsoft Developer Studio Project File - Name="TtsEng" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=TtsEng - Win32 Debug x86
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ttseng.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ttseng.mak" CFG="TtsEng - Win32 Debug x86"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "TtsEng - Win32 Debug x86" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "TtsEng - Win32 Release x86" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Desktop"
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "TtsEng - Win32 Debug x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "TtsEng___Win32_Debug_x86"
# PROP BASE Intermediate_Dir "TtsEng___Win32_Debug_x86"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_x86"
# PROP Intermediate_Dir "Debug_x86"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /ZI /Od /I "..\..\..\sdk\include" /I "..\..\..\ddk\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /ZI /Od /I "..\..\..\..\..\ddk\include" /I "..\..\..\..\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD MTL /I "..\..\..\..\..\ddk\idl" /I "..\..\..\..\idl"
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib advapi32.lib ole32.lib /nologo /subsystem:windows /dll /map /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\..\lib\i386"
# Begin Custom Build - Performing registration
OutDir=.\Debug_x86
TargetPath=.\Debug_x86\ttseng.dll
InputPath=.\Debug_x86\ttseng.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "TtsEng - Win32 Release x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "TtsEng___Win32_Release_x86"
# PROP BASE Intermediate_Dir "TtsEng___Win32_Release_x86"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_x86"
# PROP Intermediate_Dir "Release_x86"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_DLL" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /O1 /I "..\..\..\..\..\ddk\include" /I "..\..\..\..\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /FD /c
# ADD MTL /I "..\..\..\..\..\ddk\idl" /I "..\..\..\..\idl"
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib advapi32.lib ole32.lib /nologo /subsystem:windows /dll /map /machine:I386 /libpath:"..\..\..\..\lib\i386"
# Begin Custom Build - Performing registration
OutDir=.\Release_x86
TargetPath=.\Release_x86\ttseng.dll
InputPath=.\Release_x86\ttseng.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "TtsEng - Win32 Debug x86"
# Name "TtsEng - Win32 Release x86"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD BASE CPP /Yc"stdafx.h"
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\TtsEng.cpp
# End Source File
# Begin Source File

SOURCE=.\TtsEng.def
# End Source File
# Begin Source File

SOURCE=.\TtsEng.idl
# ADD BASE MTL /I "..\..\..\sdk\idl" /tlb ".\TtsEng.tlb" /h "TtsEng.h" /iid "" /Oicf
# ADD MTL /I "..\..\..\sdk\idl" /tlb ".\TtsEng.tlb" /h "TtsEng.h" /iid "TtsEng_i.c" /Oicf
# End Source File
# Begin Source File

SOURCE=.\TtsEng.rc
# End Source File
# Begin Source File

SOURCE=.\TtsEngObj.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TtsEngObj.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\TtsEngObj.rgs
# End Source File
# End Group
# End Target
# End Project
