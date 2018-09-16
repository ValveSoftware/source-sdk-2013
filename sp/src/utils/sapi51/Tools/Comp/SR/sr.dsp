# Microsoft Developer Studio Project File - Name="SRCOMP" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=SRCOMP - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sr.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sr.mak" CFG="SRCOMP - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "SRCOMP - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "SRCOMP - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SRCOMP - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SRCOMP_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\..\..\sdk\include" /I "..\..\..\..\ddk\include" /I "..\..\source\common\include" /I "..\..\source\sr" /I "..\common\include" /I "..\..\..\include" /I "..\common\cpp" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SRCOMP_EXPORTS" /D _WIN32_WINNT=0x0500 /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kato.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:"Debug/srcomp.pdb" /machine:I386 /out:"Release/SRCOMP.dll" /implib:"Debug/srcomp.lib" /pdbtype:sept /libpath:"..\..\..\..\sdk\lib" /libpath:"..\..\..\..\sdk\lib\i386" /libpath:"..\..\source\common\lib" /libpath:"..\common\lib" /libpath:"..\..\..\lib\i386"
# SUBTRACT LINK32 /pdb:none /incremental:yes /debug

!ELSEIF  "$(CFG)" == "SRCOMP - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SRCOMP_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\common\cpp" /I "..\..\..\..\sdk\include" /I "..\..\..\..\ddk\include" /I "..\..\source\common\include" /I "..\..\source\sr" /I "..\common\include" /I "..\..\..\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SRCOMP_EXPORTS" /D _WIN32_WINNT=0x0500 /FR /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /I "..\..\..\sdk\idl" /D "_DEBUG" /win32
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kato.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"Debug/SRCOMP.dll" /pdbtype:sept /libpath:"..\..\..\..\sdk\lib\i386" /libpath:"..\..\source\common\lib" /libpath:"..\common\lib" /libpath:"..\..\..\lib\i386"
# SUBTRACT LINK32 /pdb:none /nodefaultlib

!ENDIF 

# Begin Target

# Name "SRCOMP - Win32 Release"
# Name "SRCOMP - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Test Module"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\srenginecompliance.cpp
# End Source File
# End Group
# Begin Group "Grammar"

# PROP Default_Filter "*.xml"
# Begin Source File

SOURCE=..\resources\snork.xml

!IF  "$(CFG)" == "SRCOMP - Win32 Release"

# Begin Custom Build
InputPath=..\resources\snork.xml

"..\resources\snork.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "SRCOMP - Win32 Debug"

# Begin Custom Build
InputPath=..\resources\snork.xml

"..\resources\snork.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\resources\snork_j.xml

!IF  "$(CFG)" == "SRCOMP - Win32 Release"

# Begin Custom Build
InputPath=..\resources\snork_j.xml

"..\resources\snork_j.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "SRCOMP - Win32 Debug"

# Begin Custom Build
InputPath=..\resources\snork_j.xml

"..\resources\snork_j.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\resources\tag_exprule.xml

!IF  "$(CFG)" == "SRCOMP - Win32 Release"

# Begin Custom Build
InputPath=..\resources\tag_exprule.xml

"..\resources\tag_exprule.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "SRCOMP - Win32 Debug"

# Begin Custom Build
InputPath=..\resources\tag_exprule.xml

"..\resources\tag_exprule.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\resources\tag_exprule_j.xml

!IF  "$(CFG)" == "SRCOMP - Win32 Release"

# Begin Custom Build
InputPath=..\resources\tag_exprule_j.xml

"..\resources\tag_exprule_j.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "SRCOMP - Win32 Debug"

# Begin Custom Build
InputPath=..\resources\tag_exprule_j.xml

"..\resources\tag_exprule_j.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\resources\tag_l.xml

!IF  "$(CFG)" == "SRCOMP - Win32 Release"

# Begin Custom Build
InputPath=..\resources\tag_l.xml

"..\resources\tag_l.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "SRCOMP - Win32 Debug"

# Begin Custom Build
InputPath=..\resources\tag_l.xml

"..\resources\tag_l.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\resources\tag_l_j.xml

!IF  "$(CFG)" == "SRCOMP - Win32 Release"

# Begin Custom Build
InputPath=..\resources\tag_l_j.xml

"..\resources\tag_l_j.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "SRCOMP - Win32 Debug"

# Begin Custom Build
InputPath=..\resources\tag_l_j.xml

"..\resources\tag_l_j.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\resources\tag_o.xml

!IF  "$(CFG)" == "SRCOMP - Win32 Release"

# Begin Custom Build
InputPath=..\resources\tag_o.xml

"..\resources\tag_o.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "SRCOMP - Win32 Debug"

# Begin Custom Build
InputPath=..\resources\tag_o.xml

"..\resources\tag_o.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\resources\tag_o_j.xml

!IF  "$(CFG)" == "SRCOMP - Win32 Release"

# Begin Custom Build
InputPath=..\resources\tag_o_j.xml

"..\resources\tag_o_j.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "SRCOMP - Win32 Debug"

# Begin Custom Build
InputPath=..\resources\tag_o_j.xml

"..\resources\tag_o_j.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\resources\tag_p1.xml

!IF  "$(CFG)" == "SRCOMP - Win32 Release"

# Begin Custom Build
InputPath=..\resources\tag_p1.xml

"..\resources\tag_p1.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "SRCOMP - Win32 Debug"

# Begin Custom Build
InputPath=..\resources\tag_p1.xml

"..\resources\tag_p1.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\resources\tag_p1_j.xml

!IF  "$(CFG)" == "SRCOMP - Win32 Release"

# Begin Custom Build
InputPath=..\resources\tag_p1_j.xml

"..\resources\tag_p1_j.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "SRCOMP - Win32 Debug"

# Begin Custom Build
InputPath=..\resources\tag_p1_j.xml

"..\resources\tag_p1_j.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\resources\tag_p2.xml

!IF  "$(CFG)" == "SRCOMP - Win32 Release"

# Begin Custom Build
InputPath=..\resources\tag_p2.xml

"..\resources\tag_p2.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "SRCOMP - Win32 Debug"

# Begin Custom Build
InputPath=..\resources\tag_p2.xml

"..\resources\tag_p2.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\resources\tag_p2_j.xml

!IF  "$(CFG)" == "SRCOMP - Win32 Release"

# Begin Custom Build
InputPath=..\resources\tag_p2_j.xml

"..\resources\tag_p2_j.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "SRCOMP - Win32 Debug"

# Begin Custom Build
InputPath=..\resources\tag_p2_j.xml

"..\resources\tag_p2_j.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\resources\tag_rule.xml

!IF  "$(CFG)" == "SRCOMP - Win32 Release"

# Begin Custom Build
InputPath=..\resources\tag_rule.xml

"..\resources\tag_rule.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "SRCOMP - Win32 Debug"

# Begin Custom Build
InputPath=..\resources\tag_rule.xml

"..\resources\tag_rule.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\resources\tag_rule_j.xml

!IF  "$(CFG)" == "SRCOMP - Win32 Release"

# Begin Custom Build
InputPath=..\resources\tag_rule_j.xml

"..\resources\tag_rule_j.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "SRCOMP - Win32 Debug"

# Begin Custom Build
InputPath=..\resources\tag_rule_j.xml

"..\resources\tag_rule_j.cfg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	..\..\..\bin\gc $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\SRCOMP.cpp
# End Source File
# Begin Source File

SOURCE=..\Common\def\srcomp.def
# End Source File
# Begin Source File

SOURCE=.\SRCOMP.rc
# End Source File
# Begin Source File

SOURCE=..\Common\cpp\TUXDLL.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\srenginecompliance.h
# End Source File
# Begin Source File

SOURCE=.\srtests1.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\resources\snork.cfg
# End Source File
# Begin Source File

SOURCE=..\resources\tag_EXPRULE.cfg
# End Source File
# Begin Source File

SOURCE=..\resources\tag_exprule_j.cfg
# End Source File
# Begin Source File

SOURCE=..\resources\tag_L.cfg
# End Source File
# Begin Source File

SOURCE=..\resources\tag_l_j.cfg
# End Source File
# Begin Source File

SOURCE=..\resources\tag_O.cfg
# End Source File
# Begin Source File

SOURCE=..\resources\tag_o_j.cfg
# End Source File
# Begin Source File

SOURCE=..\resources\tag_p1.cfg
# End Source File
# Begin Source File

SOURCE=..\resources\tag_p1_j.cfg
# End Source File
# Begin Source File

SOURCE=..\resources\tag_p2.cfg
# End Source File
# Begin Source File

SOURCE=..\resources\tag_p2_j.cfg
# End Source File
# Begin Source File

SOURCE=..\resources\tag_RULE.cfg
# End Source File
# Begin Source File

SOURCE=..\resources\tag_rule_j.cfg
# End Source File
# End Group
# Begin Source File

SOURCE=..\resources\snork_j.cfg
# End Source File
# End Target
# End Project
