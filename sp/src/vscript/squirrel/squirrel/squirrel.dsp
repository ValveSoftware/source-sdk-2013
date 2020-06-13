# Microsoft Developer Studio Project File - Name="squirrel" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=squirrel - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "squirrel.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "squirrel.mak" CFG="squirrel - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "squirrel - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "squirrel - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_LocalPath ".."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "squirrel - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "GARBAGE_COLLECTOR" /YX /FD /c
# ADD BASE RSC /l 0x410 /d "NDEBUG"
# ADD RSC /l 0x410 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\lib\squirrel.lib"

!ELSEIF  "$(CFG)" == "squirrel - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x410 /d "_DEBUG"
# ADD RSC /l 0x410 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\lib\squirrel.lib"

!ENDIF 

# Begin Target

# Name "squirrel - Win32 Release"
# Name "squirrel - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\sqapi.cpp

!IF  "$(CFG)" == "squirrel - Win32 Release"

!ELSEIF  "$(CFG)" == "squirrel - Win32 Debug"

# ADD CPP /YX"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqbaselib.cpp

!IF  "$(CFG)" == "squirrel - Win32 Release"

!ELSEIF  "$(CFG)" == "squirrel - Win32 Debug"

# ADD CPP /YX"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqcompiler.cpp

!IF  "$(CFG)" == "squirrel - Win32 Release"

!ELSEIF  "$(CFG)" == "squirrel - Win32 Debug"

# ADD CPP /YX"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqdebug.cpp

!IF  "$(CFG)" == "squirrel - Win32 Release"

!ELSEIF  "$(CFG)" == "squirrel - Win32 Debug"

# ADD CPP /YX"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqfuncstate.cpp

!IF  "$(CFG)" == "squirrel - Win32 Release"

!ELSEIF  "$(CFG)" == "squirrel - Win32 Debug"

# ADD CPP /YX"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqlexer.cpp

!IF  "$(CFG)" == "squirrel - Win32 Release"

!ELSEIF  "$(CFG)" == "squirrel - Win32 Debug"

# ADD CPP /YX"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqmem.cpp
# End Source File
# Begin Source File

SOURCE=.\sqobject.cpp

!IF  "$(CFG)" == "squirrel - Win32 Release"

!ELSEIF  "$(CFG)" == "squirrel - Win32 Debug"

# ADD CPP /YX"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqstate.cpp

!IF  "$(CFG)" == "squirrel - Win32 Release"

!ELSEIF  "$(CFG)" == "squirrel - Win32 Debug"

# ADD CPP /YX"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqtable.cpp

!IF  "$(CFG)" == "squirrel - Win32 Release"

!ELSEIF  "$(CFG)" == "squirrel - Win32 Debug"

# ADD CPP /YX"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqclass.cpp

!IF  "$(CFG)" == "squirrel - Win32 Release"

!ELSEIF  "$(CFG)" == "squirrel - Win32 Debug"

# ADD CPP /YX"stdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\sqvm.cpp

!IF  "$(CFG)" == "squirrel - Win32 Release"

!ELSEIF  "$(CFG)" == "squirrel - Win32 Debug"

# ADD CPP /YX"stdafx.h"

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\sqarray.h
# End Source File
# Begin Source File

SOURCE=.\sqclosure.h
# End Source File
# Begin Source File

SOURCE=.\sqcompiler.h
# End Source File
# Begin Source File

SOURCE=.\sqfuncproto.h
# End Source File
# Begin Source File

SOURCE=.\sqfuncstate.h
# End Source File
# Begin Source File

SOURCE=.\sqlexer.h
# End Source File
# Begin Source File

SOURCE=.\sqobject.h
# End Source File
# Begin Source File

SOURCE=.\sqopcodes.h
# End Source File
# Begin Source File

SOURCE=.\sqpcheader.h
# End Source File
# Begin Source File

SOURCE=.\sqstate.h
# End Source File
# Begin Source File

SOURCE=.\sqstring.h
# End Source File
# Begin Source File

SOURCE=.\sqtable.h
# End Source File
# Begin Source File

SOURCE=.\squserdata.h
# End Source File
# Begin Source File

SOURCE=.\squtils.h
# End Source File
# Begin Source File

SOURCE=.\sqclass.h
# End Source File
# Begin Source File

SOURCE=.\sqvm.h
# End Source File
# End Group
# End Target
# End Project
