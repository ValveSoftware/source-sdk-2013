PROG = lzmaDec.exe

!IFNDEF O
!IFDEF CPU
O=$(CPU)
!ELSE
O=O
!ENDIF
!ENDIF

CFLAGS = $(CFLAGS) -nologo -c -Fo$O/ -GS- 
CFLAGS_O1 = $(CFLAGS) -O1
CFLAGS_O2 = $(CFLAGS) -O2

LFLAGS = $(LFLAGS) -nologo -OPT:NOWIN98

PROGPATH = $O\$(PROG)

COMPL_O1   = $(CPP) $(CFLAGS_O1) $**
COMPL_O2   = $(CPP) $(CFLAGS_O2) $**
COMPL      = $(CPP) $(CFLAGS_O1) $**


OBJS = \
  $O\LzmaTest.obj \
  $O\LzmaDecode.obj \

all: $(PROGPATH) 

clean:
	-del /Q $(PROGPATH) $O\*.exe $O\*.dll $O\*.obj $O\*.lib $O\*.exp $O\*.res $O\*.pch 

$O:
	if not exist "$O" mkdir "$O"

$(PROGPATH): $O $(OBJS)
	link $(LFLAGS) -out:$(PROGPATH) $(OBJS) $(LIBS)


$O\LzmaTest.obj: $(*B).c
	$(COMPL)
$O\LzmaDecode.obj: ../../Compress/LZMA_C/$(*B).c
	$(COMPL_O2)
