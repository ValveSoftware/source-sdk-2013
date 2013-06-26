# Microsoft Developer Studio Generated NMAKE File, Format Version 4.20
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=smdlexp - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to smdlexp - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "smdlexp - Win32 Release" && "$(CFG)" !=\
 "smdlexp - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "smdlexp.mak" CFG="smdlexp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "smdlexp - Win32 Release" (based on\
 "Win32 (x86) Dynamic-Link Library")
!MESSAGE "smdlexp - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "smdlexp - Win32 Debug"
RSC=rc.exe
MTL=mktyplib.exe
CPP=cl.exe

!IF  "$(CFG)" == "smdlexp - Win32 Release"

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
OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\..\..\3DSMAX\STDPLUGS\SMDLEXP.DLE"

CLEAN : 
	-@erase "$(INTDIR)\smdlexp.obj"
	-@erase "$(INTDIR)\smdlexp.res"
	-@erase "$(OUTDIR)\SMDLEXP.exp"
	-@erase "$(OUTDIR)\SMDLEXP.lib"
	-@erase "..\..\..\3DSMAX\STDPLUGS\SMDLEXP.DLE"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "\3DSMAX2.5\MAXSDK\INCLUDE" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "\3DSMAX2.5\MAXSDK\INCLUDE" /D "WIN32" /D\
 "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)/smdlexp.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=.\.
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/smdlexp.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/smdlexp.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib COMCTL32.LIB /nologo /subsystem:windows /dll /machine:I386 /out:"\3DSMAX\STDPLUGS\SMDLEXP.DLE"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib COMCTL32.LIB /nologo\
 /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)/SMDLEXP.pdb"\
 /machine:I386 /def:".\smdlexp.def" /out:"\3DSMAX\STDPLUGS\SMDLEXP.DLE"\
 /implib:"$(OUTDIR)/SMDLEXP.lib" 
DEF_FILE= \
	".\smdlexp.def"
LINK32_OBJS= \
	"$(INTDIR)\smdlexp.obj" \
	"$(INTDIR)\smdlexp.res" \
	"..\..\..\quiver\src\utils\3dsmax\CORE.LIB" \
	"..\..\..\quiver\src\utils\3dsmax\GEOM.LIB" \
	"..\..\..\quiver\src\utils\3dsmax\MESH.LIB" \
	"..\..\..\quiver\src\utils\3dsmax\UTIL.LIB"

"..\..\..\3DSMAX\STDPLUGS\SMDLEXP.DLE" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "smdlexp - Win32 Debug"

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
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "..\..\..\3DSMAX\STDPLUGS\SMDLEXP.DLE" "$(OUTDIR)\smdlexp.bsc"

CLEAN : 
	-@erase "$(INTDIR)\smdlexp.obj"
	-@erase "$(INTDIR)\smdlexp.res"
	-@erase "$(INTDIR)\smdlexp.sbr"
	-@erase "$(INTDIR)\vc40.idb"
	-@erase "$(INTDIR)\vc40.pdb"
	-@erase "$(OUTDIR)\smdlexp.bsc"
	-@erase "$(OUTDIR)\SMDLEXP.exp"
	-@erase "$(OUTDIR)\SMDLEXP.lib"
	-@erase "$(OUTDIR)\SMDLEXP.pdb"
	-@erase "..\..\..\3DSMAX\STDPLUGS\SMDLEXP.DLE"
	-@erase "..\..\..\3DSMAX\STDPLUGS\SMDLEXP.ILK"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /MD /W3 /Gm /GX /Zi /Od /I "\3DSMAX2.5\MAXSDK\INCLUDE" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /YX /c
CPP_PROJ=/nologo /MD /W3 /Gm /GX /Zi /Od /I "\3DSMAX2.5\MAXSDK\INCLUDE" /D\
 "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR"$(INTDIR)/" /Fp"$(INTDIR)/smdlexp.pch"\
 /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=.\Debug/
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/smdlexp.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/smdlexp.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\smdlexp.sbr"

"$(OUTDIR)\smdlexp.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib COMCTL32.LIB /nologo /subsystem:windows /dll /debug /machine:I386 /out:"\3DSMAX\STDPLUGS\SMDLEXP.DLE"
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib COMCTL32.LIB /nologo\
 /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)/SMDLEXP.pdb" /debug\
 /machine:I386 /def:".\smdlexp.def" /out:"\3DSMAX\STDPLUGS\SMDLEXP.DLE"\
 /implib:"$(OUTDIR)/SMDLEXP.lib" 
DEF_FILE= \
	".\smdlexp.def"
LINK32_OBJS= \
	"$(INTDIR)\smdlexp.obj" \
	"$(INTDIR)\smdlexp.res" \
	"..\..\..\quiver\src\utils\3dsmax\CORE.LIB" \
	"..\..\..\quiver\src\utils\3dsmax\GEOM.LIB" \
	"..\..\..\quiver\src\utils\3dsmax\MESH.LIB" \
	"..\..\..\quiver\src\utils\3dsmax\UTIL.LIB"

"..\..\..\3DSMAX\STDPLUGS\SMDLEXP.DLE" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "smdlexp - Win32 Release"
# Name "smdlexp - Win32 Debug"

!IF  "$(CFG)" == "smdlexp - Win32 Release"

!ELSEIF  "$(CFG)" == "smdlexp - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=.\smdlexp.cpp
DEP_CPP_SMDLE=\
	".\smedefs.h"\
	
NODEP_CPP_SMDLE=\
	".\ANIMTBL.H"\
	".\DECOMP.H"\
	".\istdplug.h"\
	".\MAX.H"\
	".\STDMAT.H"\
	

!IF  "$(CFG)" == "smdlexp - Win32 Release"


"$(INTDIR)\smdlexp.obj" : $(SOURCE) $(DEP_CPP_SMDLE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "smdlexp - Win32 Debug"


"$(INTDIR)\smdlexp.obj" : $(SOURCE) $(DEP_CPP_SMDLE) "$(INTDIR)"

"$(INTDIR)\smdlexp.sbr" : $(SOURCE) $(DEP_CPP_SMDLE) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\smdlexp.def

!IF  "$(CFG)" == "smdlexp - Win32 Release"

!ELSEIF  "$(CFG)" == "smdlexp - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\smdlexp.rc

"$(INTDIR)\smdlexp.res" : $(SOURCE) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=\quiver\src\utils\3dsmax\UTIL.LIB

!IF  "$(CFG)" == "smdlexp - Win32 Release"

!ELSEIF  "$(CFG)" == "smdlexp - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=\quiver\src\utils\3dsmax\GEOM.LIB

!IF  "$(CFG)" == "smdlexp - Win32 Release"

!ELSEIF  "$(CFG)" == "smdlexp - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=\quiver\src\utils\3dsmax\MESH.LIB

!IF  "$(CFG)" == "smdlexp - Win32 Release"

!ELSEIF  "$(CFG)" == "smdlexp - Win32 Debug"

!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=\quiver\src\utils\3dsmax\CORE.LIB

!IF  "$(CFG)" == "smdlexp - Win32 Release"

!ELSEIF  "$(CFG)" == "smdlexp - Win32 Debug"

!ENDIF 

# End Source File
# End Target
# End Project
################################################################################
