NAME=tier1
SRCROOT=..
TARGET_PLATFORM=osx32
TARGET_PLATFORM_EXT=
USE_VALVE_BINDIR=0
PWD := $(shell pwd)
# If no configuration is specified, "release" will be used.
ifeq "$(CFG)" ""
	CFG = release
endif

GCC_ExtraCompilerFlags=
GCC_ExtraLinkerFlags = 
SymbolVisibility = hidden
OptimizerLevel = -gdwarf-2 -g $(OptimizerLevel_CompilerSpecific)
SystemLibraries = 
DLL_EXT = .dylib
SYM_EXT = .dSYM
FORCEINCLUDES= 
ifeq "$(CFG)" "debug"
DEFINES += -DDEBUG -D_DEBUG -DGNUC -DPOSIX -D_OSX -DOSX -D_DARWIN_UNLIMITED_SELECT -DFD_SETSIZE=10240 -DQUICKTIME_VIDEO -DFORCE_QUICKTIME -DGL_GLEXT_PROTOTYPES -DDX_TO_GL_ABSTRACTION -DTIER1_STATIC_LIB -DVPCGAMECAPS=VALVE -DPROJECTDIR=/Users/joe/p4/ValveGames/rel/hl2/src/tier1 -D_DLL_EXT=.dylib -DVPCGAME=valve -D_POSIX=1 
else
DEFINES += -DNDEBUG -DGNUC -DPOSIX -D_OSX -DOSX -D_DARWIN_UNLIMITED_SELECT -DFD_SETSIZE=10240 -DQUICKTIME_VIDEO -DFORCE_QUICKTIME -DGL_GLEXT_PROTOTYPES -DDX_TO_GL_ABSTRACTION -DTIER1_STATIC_LIB -DVPCGAMECAPS=VALVE -DPROJECTDIR=/Users/joe/p4/ValveGames/rel/hl2/src/tier1 -D_DLL_EXT=.dylib -DVPCGAME=valve -D_POSIX=1 
endif
INCLUDEDIRS += ../common ../public ../public/tier0 ../public/tier1 
CONFTYPE = lib
OUTPUTFILE=../lib/osx32/tier1.a
THIS_MAKEFILE = $(PWD)/tier1_osx32.mak
MAKEFILE_BASE = $(SRCROOT)/devtools/makefile_base_posix.mak


POSTBUILDCOMMAND = true



CPPFILES= \
    bitbuf.cpp \
    byteswap.cpp \
    characterset.cpp \
    checksum_crc.cpp \
    checksum_md5.cpp \
    checksum_sha1.cpp \
    commandbuffer.cpp \
    convar.cpp \
    datamanager.cpp \
    diff.cpp \
    generichash.cpp \
    ilocalize.cpp \
    interface.cpp \
    KeyValues.cpp \
    kvpacker.cpp \
    mempool.cpp \
    memstack.cpp \
    NetAdr.cpp \
    newbitbuf.cpp \
    processor_detect_linux.cpp \
    rangecheckedvar.cpp \
    reliabletimer.cpp \
    snappy-sinksource.cpp \
    snappy-stubs-internal.cpp \
    snappy.cpp \
    sparsematrix.cpp \
    splitstring.cpp \
    stringpool.cpp \
    strtools.cpp \
    tier1.cpp \
    tokenreader.cpp \
    uniqueid.cpp \
    utlbuffer.cpp \
    utlbufferutil.cpp \
    utlstring.cpp \
    utlsymbol.cpp \


LIBFILES = \


LIBFILENAMES = \


# Include the base makefile now.
include $(SRCROOT)/devtools/makefile_base_posix.mak



OTHER_DEPENDENCIES = \


$(OBJ_DIR)/_other_deps.P : $(OTHER_DEPENDENCIES)
	$(GEN_OTHER_DEPS)

-include $(OBJ_DIR)/_other_deps.P



ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/bitbuf.P
endif

$(OBJ_DIR)/bitbuf.o : $(PWD)/bitbuf.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/byteswap.P
endif

$(OBJ_DIR)/byteswap.o : $(PWD)/byteswap.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/characterset.P
endif

$(OBJ_DIR)/characterset.o : $(PWD)/characterset.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/checksum_crc.P
endif

$(OBJ_DIR)/checksum_crc.o : $(PWD)/checksum_crc.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/checksum_md5.P
endif

$(OBJ_DIR)/checksum_md5.o : $(PWD)/checksum_md5.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/checksum_sha1.P
endif

$(OBJ_DIR)/checksum_sha1.o : $(PWD)/checksum_sha1.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/commandbuffer.P
endif

$(OBJ_DIR)/commandbuffer.o : $(PWD)/commandbuffer.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/convar.P
endif

$(OBJ_DIR)/convar.o : $(PWD)/convar.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/datamanager.P
endif

$(OBJ_DIR)/datamanager.o : $(PWD)/datamanager.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/diff.P
endif

$(OBJ_DIR)/diff.o : $(PWD)/diff.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/generichash.P
endif

$(OBJ_DIR)/generichash.o : $(PWD)/generichash.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/ilocalize.P
endif

$(OBJ_DIR)/ilocalize.o : $(PWD)/ilocalize.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/interface.P
endif

$(OBJ_DIR)/interface.o : $(PWD)/interface.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/KeyValues.P
endif

$(OBJ_DIR)/KeyValues.o : $(PWD)/KeyValues.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/kvpacker.P
endif

$(OBJ_DIR)/kvpacker.o : $(PWD)/kvpacker.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/mempool.P
endif

$(OBJ_DIR)/mempool.o : $(PWD)/mempool.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/memstack.P
endif

$(OBJ_DIR)/memstack.o : $(PWD)/memstack.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/NetAdr.P
endif

$(OBJ_DIR)/NetAdr.o : $(PWD)/NetAdr.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/newbitbuf.P
endif

$(OBJ_DIR)/newbitbuf.o : $(PWD)/newbitbuf.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/processor_detect_linux.P
endif

$(OBJ_DIR)/processor_detect_linux.o : $(PWD)/processor_detect_linux.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/rangecheckedvar.P
endif

$(OBJ_DIR)/rangecheckedvar.o : $(PWD)/rangecheckedvar.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/reliabletimer.P
endif

$(OBJ_DIR)/reliabletimer.o : $(PWD)/reliabletimer.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/snappy-sinksource.P
endif

$(OBJ_DIR)/snappy-sinksource.o : $(PWD)/snappy-sinksource.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/snappy-stubs-internal.P
endif

$(OBJ_DIR)/snappy-stubs-internal.o : $(PWD)/snappy-stubs-internal.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/snappy.P
endif

$(OBJ_DIR)/snappy.o : $(PWD)/snappy.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/sparsematrix.P
endif

$(OBJ_DIR)/sparsematrix.o : $(PWD)/sparsematrix.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/splitstring.P
endif

$(OBJ_DIR)/splitstring.o : $(PWD)/splitstring.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/stringpool.P
endif

$(OBJ_DIR)/stringpool.o : $(PWD)/stringpool.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/strtools.P
endif

$(OBJ_DIR)/strtools.o : $(PWD)/strtools.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/tier1.P
endif

$(OBJ_DIR)/tier1.o : $(PWD)/tier1.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/tokenreader.P
endif

$(OBJ_DIR)/tokenreader.o : $(PWD)/tokenreader.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/uniqueid.P
endif

$(OBJ_DIR)/uniqueid.o : $(PWD)/uniqueid.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/utlbuffer.P
endif

$(OBJ_DIR)/utlbuffer.o : $(PWD)/utlbuffer.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/utlbufferutil.P
endif

$(OBJ_DIR)/utlbufferutil.o : $(PWD)/utlbufferutil.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/utlstring.P
endif

$(OBJ_DIR)/utlstring.o : $(PWD)/utlstring.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
-include $(OBJ_DIR)/utlsymbol.P
endif

$(OBJ_DIR)/utlsymbol.o : $(PWD)/utlsymbol.cpp $(THIS_MAKEFILE) $(MAKEFILE_BASE)
	$(PRE_COMPILE_FILE)
	$(COMPILE_FILE) $(POST_COMPILE_FILE)

# Uncomment this, and set FILENAME to file you want built without optimizations enabled.
# $(OBJ_DIR)/FILENAME.o : CFLAGS := $(subst -O2,-O0,$(CFLAGS))

# Uncomment this to disable optimizations for the entire project.
# $(OBJ_DIR)/%.o : CFLAGS := $(subst -O2,-O0,$(CFLAGS))


