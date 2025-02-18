PROG = lzma
CXX = g++ -O2 -Wall
CXX_C = gcc -O2 -Wall
LIB = -lm
RM = rm -f
CFLAGS = -c -I ../../../

OBJS = \
  LzmaAlone.o \
  LzmaBench.o \
  LzmaRam.o \
  LzmaRamDecode.o \
  LzmaDecode.o \
  BranchX86.o \
  LZMADecoder.o \
  LZMAEncoder.o \
  LZInWindow.o \
  LZOutWindow.o \
  RangeCoderBit.o \
  InBuffer.o \
  OutBuffer.o \
  FileStreams.o \
  StreamUtils.o \
  Alloc.o \
  C_FileIO.o \
  CommandLineParser.o \
  CRC.o \
  String.o \
  StringConvert.o \
  StringToInt.o \
  Vector.o \


all: $(PROG)

$(PROG): $(OBJS)
	$(CXX) -o $(PROG) $(LDFLAGS) $(OBJS) $(LIB)

LzmaAlone.o: LzmaAlone.cpp
	$(CXX) $(CFLAGS) LzmaAlone.cpp

LzmaBench.o: LzmaBench.cpp
	$(CXX) $(CFLAGS) LzmaBench.cpp

LzmaRam.o: LzmaRam.cpp
	$(CXX) $(CFLAGS) LzmaRam.cpp

LzmaRamDecode.o: LzmaRamDecode.c
	$(CXX_C) $(CFLAGS) LzmaRamDecode.c

LzmaDecode.o: ../LZMA_C/LzmaDecode.c
	$(CXX_C) $(CFLAGS) ../LZMA_C/LzmaDecode.c

BranchX86.o: ../Branch/BranchX86.c
	$(CXX_C) $(CFLAGS) ../Branch/BranchX86.c

LZMADecoder.o: ../LZMA/LZMADecoder.cpp
	$(CXX) $(CFLAGS) ../LZMA/LZMADecoder.cpp

LZMAEncoder.o: ../LZMA/LZMAEncoder.cpp
	$(CXX) $(CFLAGS) ../LZMA/LZMAEncoder.cpp

LZInWindow.o: ../LZ/LZInWindow.cpp
	$(CXX) $(CFLAGS) ../LZ/LZInWindow.cpp

LZOutWindow.o: ../LZ/LZOutWindow.cpp
	$(CXX) $(CFLAGS) ../LZ/LZOutWindow.cpp

RangeCoderBit.o: ../RangeCoder/RangeCoderBit.cpp
	$(CXX) $(CFLAGS) ../RangeCoder/RangeCoderBit.cpp

InBuffer.o: ../../Common/InBuffer.cpp
	$(CXX) $(CFLAGS) ../../Common/InBuffer.cpp

OutBuffer.o: ../../Common/OutBuffer.cpp
	$(CXX) $(CFLAGS) ../../Common/OutBuffer.cpp

FileStreams.o: ../../Common/FileStreams.cpp
	$(CXX) $(CFLAGS) ../../Common/FileStreams.cpp

StreamUtils.o: ../../Common/StreamUtils.cpp
	$(CXX) $(CFLAGS) ../../Common/StreamUtils.cpp

Alloc.o: ../../../Common/Alloc.cpp
	$(CXX) $(CFLAGS) ../../../Common/Alloc.cpp

C_FileIO.o: ../../../Common/C_FileIO.cpp
	$(CXX) $(CFLAGS) ../../../Common/C_FileIO.cpp

CommandLineParser.o: ../../../Common/CommandLineParser.cpp
	$(CXX) $(CFLAGS) ../../../Common/CommandLineParser.cpp

CRC.o: ../../../Common/CRC.cpp
	$(CXX) $(CFLAGS) ../../../Common/CRC.cpp

MyWindows.o: ../../../Common/MyWindows.cpp
	$(CXX) $(CFLAGS) ../../../Common/MyWindows.cpp

String.o: ../../../Common/String.cpp
	$(CXX) $(CFLAGS) ../../../Common/String.cpp

StringConvert.o: ../../../Common/StringConvert.cpp
	$(CXX) $(CFLAGS) ../../../Common/StringConvert.cpp

StringToInt.o: ../../../Common/StringToInt.cpp
	$(CXX) $(CFLAGS) ../../../Common/StringToInt.cpp

Vector.o: ../../../Common/Vector.cpp
	$(CXX) $(CFLAGS) ../../../Common/Vector.cpp

clean:
	-$(RM) $(PROG) $(OBJS)

