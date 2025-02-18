PROG = lzma.exe
CFLAGS = $(CFLAGS) -I ../../../
LIBS = $(LIBS) oleaut32.lib user32.lib

!IFNDEF O
!IFDEF CPU
O=$(CPU)
!ELSE
O=O
!ENDIF
!ENDIF

CFLAGS = $(CFLAGS) -nologo -EHsc -c -Fo$O/ -GS- 
CFLAGS_O1 = $(CFLAGS) -O1
CFLAGS_O2 = $(CFLAGS) -O2

LFLAGS = $(LFLAGS) -nologo -OPT:NOWIN98

PROGPATH = $O\$(PROG)

COMPL_O1   = $(CPP) $(CFLAGS_O1) $**
COMPL_O2   = $(CPP) $(CFLAGS_O2) $**
COMPL      = $(CPP) $(CFLAGS_O1) $**


LZMA_OBJS = \
  $O\LzmaAlone.obj \
  $O\LzmaBench.obj \
  $O\LzmaRam.obj \

LZMA_OPT_OBJS = \
  $O\LZMADecoder.obj \
  $O\LZMAEncoder.obj \

COMMON_OBJS = \
  $O\Alloc.obj \
  $O\CRC.obj \
  $O\CommandLineParser.obj \
  $O\String.obj \
  $O\StringConvert.obj \
  $O\StringToInt.obj \
  $O\Vector.obj

7ZIP_COMMON_OBJS = \
  $O\InBuffer.obj \
  $O\OutBuffer.obj \
  $O\StreamUtils.obj \

LZ_OBJS = \
  $O\LZInWindow.obj \
  $O\LZOutWindow.obj \


OBJS = \
  $(LZMA_OBJS) \
  $(LZMA_OPT_OBJS) \
  $(COMMON_OBJS) \
  $(7ZIP_COMMON_OBJS) \
  $(LZ_OBJS) \
  $O\LzmaRamDecode.obj \
  $O\LzmaDecode.obj \
  $O\FileStreams.obj \
  $O\FileIO.obj \
  $O\RangeCoderBit.obj \
  $O\BranchX86.obj \

all: $(PROGPATH) 

clean:
	-del /Q $(PROGPATH) $O\*.exe $O\*.dll $O\*.obj $O\*.lib $O\*.exp $O\*.res $O\*.pch 

$O:
	if not exist "$O" mkdir "$O"

$(PROGPATH): $O $(OBJS)
	link $(LFLAGS) -out:$(PROGPATH) $(OBJS) $(LIBS)


$(LZMA_OBJS): $(*B).cpp
	$(COMPL)
$(LZMA_OPT_OBJS): ../LZMA/$(*B).cpp
	$(COMPL_O2)
$(COMMON_OBJS): ../../../Common/$(*B).cpp
	$(COMPL)
$(7ZIP_COMMON_OBJS): ../../Common/$(*B).cpp
	$(COMPL)
$(LZ_OBJS): ../LZ/$(*B).cpp
	$(COMPL)
$O\RangeCoderBit.obj: ../RangeCoder/$(*B).cpp
	$(COMPL)
$O\LzmaRamDecode.obj: LzmaRamDecode.c
	$(COMPL_O1)
$O\LzmaDecode.obj: ../LZMA_C/LzmaDecode.c
	$(COMPL_O2)
$O\BranchX86.obj: ../Branch/BranchX86.c
	$(COMPL_O2)
$O\FileStreams.obj: ../../Common/FileStreams.cpp
	$(COMPL)
$O\FileIO.obj: ../../../Windows/FileIO.cpp
	$(COMPL)
