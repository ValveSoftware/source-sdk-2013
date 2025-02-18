#
# Base makefile for Linux and OSX
#
# !!!!! Note to future editors !!!!!
# 
# before you make changes, make sure you grok:
# 1. the difference between =, :=, +=, and ?= 
# 2. how and when this base makefile gets included in the generated makefile(s)
#  ( see http://www.gnu.org/software/make/manual/make.html#Flavors )
#
# Command line prefixes:
#  -	errors are ignored
#  @	command is not printed to stdout before being executed
#  +	command is executed even if Make is invoked in "do not exec" mode

OS := $(shell uname)
HOSTNAME := $(shell hostname)

-include $(SRCROOT)/devtools/steam_def.mak
-include $(SRCROOT)/devtools/sourcesdk_def.mak

# To build with clang, set the following in your environment:
#   CC = clang
#   CXX = clang++

ifeq ($(CFG), release)
	# With gcc 4.6.3, engine.so went from 7,383,765 to 8,429,109 when building with -O3.
	#  There also was no speed difference running at 1280x1024. May 2012, mikesart.
	#  tonyp: The size increase was likely caused by -finline-functions and -fipa-cp-clone getting switched on with -O3.
	# -fno-omit-frame-pointer: need this for stack traces with perf.
	OptimizerLevel_CompilerSpecific = -O2 -fno-strict-aliasing -ffast-math -fno-omit-frame-pointer -ftree-vectorize -fpredictive-commoning -funswitch-loops
else
	OptimizerLevel_CompilerSpecific = -O0
	#-O1 -finline-functions
endif

# CPPFLAGS == "c/c++ *preprocessor* flags" - not "cee-plus-plus flags"
ARCH_FLAGS = 
BUILDING_MULTI_ARCH = 0
CPPFLAGS = $(DEFINES) $(addprefix -I, $(abspath $(INCLUDEDIRS) ))
CFLAGS = $(ARCH_FLAGS) $(CPPFLAGS) $(WARN_FLAGS) -fvisibility=$(SymbolVisibility) $(OptimizerLevel) -pipe $(GCC_ExtraCompilerFlags) -Usprintf -Ustrncpy -UPROTECTED_THINGS_ENABLE
# In -std=gnu++0x mode we get lots of errors about "error: narrowing conversion". -fpermissive
# turns these into warnings in gcc, and -Wno-c++11-narrowing suppresses them entirely in clang 3.1+.
ifeq ($(CXX),clang++)
	CXXFLAGS = $(CFLAGS) -std=gnu++0x -Wno-c++11-narrowing -Wno-dangling-else
else
	CXXFLAGS = $(CFLAGS) -std=gnu++0x -fpermissive
endif
DEFINES += -DVPROF_LEVEL=1 -DGNUC -DNO_HOOK_MALLOC -DNO_MALLOC_OVERRIDE
LDFLAGS = $(CFLAGS) $(GCC_ExtraLinkerFlags) $(OptimizerLevel)
GENDEP_CXXFLAGS = -MD -MP -MF $(@:.o=.P) 
MAP_FLAGS =
Srv_GAMEOUTPUTFILE = 
COPY_DLL_TO_SRV = 0


ifeq ($(STEAM_BRANCH),1)
	WARN_FLAGS = -Wall -Wextra -Wshadow -Wno-invalid-offsetof
else
	WARN_FLAGS = -Wno-write-strings -Wno-multichar
endif

WARN_FLAGS += -Wno-unknown-pragmas -Wno-unused-parameter -Wno-unused-value -Wno-missing-field-initializers -Wno-sign-compare -Wno-reorder -Wno-invalid-offsetof -Wno-float-equal -Werror=return-type -fdiagnostics-show-option -Wformat -Wformat-security


ifeq ($(OS),Linux)
	# We should always specify -Wl,--build-id, as documented at:
	# http://linux.die.net/man/1/ld and http://fedoraproject.org/wiki/Releases/FeatureBuildId.http://fedoraproject.org/wiki/Releases/FeatureBuildId
	LDFLAGS += -Wl,--build-id
	# Set USE_VALVE_BINDIR to build with /Steam/tools/linux in the /valve/bin path.
	#  Dedicated server uses this.
	ifeq ($(USE_VALVE_BINDIR),1)
		# dedicated server flags
		ifeq ($(TARGET_PLATFORM),linux64)
			VALVE_BINDIR = /valve/bin64/
			MARCH_TARGET = nocona
		else
			VALVE_BINDIR = /valve/bin/
			MARCH_TARGET = pentium4
		endif
		STRIP_FLAGS =
	else
		# linux desktop client flags
		VALVE_BINDIR =
		# If the steam-runtime is available, use it. We should just default to using it when
		#  buildbot and everyone has a bit of time to get it installed.
		ifneq "$(wildcard /valve/steam-runtime/bin/)" ""
			# The steam-runtime is incompatible with clang at this point, so disable it
			# if clang is enabled.
			ifneq ($(CXX),clang++)
				VALVE_BINDIR = /valve/steam-runtime/bin/
			endif
		endif
		GCC_VER =
		MARCH_TARGET = pentium4
		# On dedicated servers, some plugins depend on global variable symbols in addition to functions.
		# So symbols like _Z16ClearMultiDamagev should show up when you do "nm server_srv.so" in TF2.
		STRIP_FLAGS = -x
	endif

	ifeq ($(CXX),clang++)
		# Clang does not support -mfpmath=sse because it uses whatever
		# instruction set extensions are available by default.
		SSE_GEN_FLAGS = -msse2
	else
		SSE_GEN_FLAGS = -msse2 -mfpmath=sse
	endif

	CCACHE := $(SRCROOT)/devtools/bin/linux/ccache

	ifeq ($(origin GCC_VER), undefined)
	GCC_VER=-4.6
	endif
	ifeq ($(origin AR), default)
		AR = $(VALVE_BINDIR)ar crs
	endif
	ifeq ($(origin CC),default)
		CC = $(CCACHE) $(VALVE_BINDIR)gcc$(GCC_VER)	
	endif
	ifeq ($(origin CXX), default)
		CXX = $(CCACHE) $(VALVE_BINDIR)g++$(GCC_VER)
	endif
	# Support ccache with clang. Add -Qunused-arguments to avoid excessive warnings due to
	# a ccache quirk. Could also upgrade ccache.
	# http://petereisentraut.blogspot.com/2011/05/ccache-and-clang.html
	ifeq ($(CC),clang)
		CC = $(CCACHE) $(VALVE_BINDIR)clang -Qunused-arguments
	endif
	ifeq ($(CXX),clang++)
		CXX = $(CCACHE) $(VALVE_BINDIR)clang++ -Qunused-arguments
	endif
	LINK ?= $(CC)

	ifeq ($(TARGET_PLATFORM),linux64)
		# nocona = pentium4 + 64bit + MMX, SSE, SSE2, SSE3 - no SSSE3 (that's three s's - added in core2)
		ARCH_FLAGS += -march=$(MARCH_TARGET) -mtune=core2
		LD_SO = ld-linux-x86_64.so.2
		LIBSTDCXX := $(shell $(CXX) -print-file-name=libstdc++.a)
		LIBSTDCXXPIC := $(shell $(CXX) -print-file-name=libstdc++-pic.a)
	else
		# pentium4 = MMX, SSE, SSE2 - no SSE3 (added in prescott) # -msse3 -mfpmath=sse
		ARCH_FLAGS += -m32 -march=$(MARCH_TARGET) -mtune=core2 $(SSE_GEN_FLAGS)
		LD_SO = ld-linux.so.2
		LIBSTDCXX := $(shell $(CXX) $(ARCH_FLAGS) -print-file-name=libstdc++.so)
		LIBSTDCXXPIC := $(shell $(CXX) $(ARCH_FLAGS) -print-file-name=libstdc++.so)
		LDFLAGS += -m32
	endif

	GEN_SYM ?= $(SRCROOT)/devtools/gendbg.sh
	ifeq ($(CFG),release)
		STRIP ?= strip $(STRIP_FLAGS) -S
	#	CFLAGS += -ffunction-sections -fdata-sections
	#	LDFLAGS += -Wl,--gc-sections -Wl,--print-gc-sections
	else
		STRIP ?= true
	endif
	VSIGN ?= true

	ifeq ($(SOURCE_SDK), 1)
		Srv_GAMEOUTPUTFILE := $(GAMEOUTPUTFILE:.so=_srv.so)
		COPY_DLL_TO_SRV := 1
	endif

	LINK_MAP_FLAGS = -Wl,-Map,$(@:.so=).map

	SHLIBLDFLAGS = -shared $(LDFLAGS) -Wl,--no-undefined

	_WRAP := -Xlinker --wrap=
	PATHWRAP = $(_WRAP)fopen $(_WRAP)freopen $(_WRAP)open    $(_WRAP)creat    $(_WRAP)access  $(_WRAP)__xstat \
		   $(_WRAP)stat  $(_WRAP)lstat   $(_WRAP)fopen64 $(_WRAP)open64   $(_WRAP)opendir $(_WRAP)__lxstat \
		   $(_WRAP)chmod $(_WRAP)chown   $(_WRAP)lchown  $(_WRAP)symlink  $(_WRAP)link    $(_WRAP)__lxstat64 \
		   $(_WRAP)mknod $(_WRAP)utimes  $(_WRAP)unlink  $(_WRAP)rename   $(_WRAP)utime   $(_WRAP)__xstat64 \
		   $(_WRAP)mount $(_WRAP)mkfifo  $(_WRAP)mkdir   $(_WRAP)rmdir    $(_WRAP)scandir $(_WRAP)realpath

	LIB_START_EXE = $(PATHWRAP) -static-libgcc -Wl,--start-group
	LIB_END_EXE = -Wl,--end-group -lm -ldl $(LIBSTDCXX) -lpthread 

	LIB_START_SHLIB = $(PATHWRAP) -static-libgcc -Wl,--start-group
	LIB_END_SHLIB = -Wl,--end-group -lm -ldl $(LIBSTDCXXPIC) -lpthread -l:$(LD_SO) -Wl,--version-script=$(SRCROOT)/devtools/version_script.linux.txt

endif

ifeq ($(OS),Darwin)
	CCACHE := $(SRCROOT)/devtools/bin/osx32/ccache
	MAC_SDK_VER ?= 10.6
	MAC_SDK := macosx$(MAC_SDK_VER)
	SYSROOT := $(shell xcodebuild -sdk $(MAC_SDK) -version Path)

	ifneq ($(origin MAC_SDK_VER), file)
            $(warning Attempting build with SDK version $(MAC_SDK_VER), only 10.6 is supported and recommended!)
	endif

	ifeq ($(SYSROOT),)
		FIRSTSDK := $(firstword $(sort $(shell xcodebuild -showsdks | grep macosx | sed 's/.*macosx//')))
                $(error Could not find SDK version $(MAC_SDK_VER). Install and configure Xcode 4.3, or build with: make MAC_SDK_VER=$(FIRSTSDK))
	endif

	ifeq ($(origin CC), default)
                # Test to see if you have a compiler in the right place, if you
                # don't abort with an error
		CLANG := $(shell xcrun -sdk $(MAC_SDK) -find clang)
		ifeq ($(wildcard $(CLANG)),)
                        $(error Unable to find C compiler, install and configure Xcode 4.3)
		endif

		CC := $(CCACHE) $(CLANG) -Qunused-arguments
	endif

	ifeq ($(origin CXX), default)
		CXXLANG := $(shell xcrun -sdk $(MAC_SDK) -find clang++)
		ifeq ($(wildcard $(CXXLANG)),)
                        $(error Unable to find C++ compiler, install and configure Xcode 4.3)
		endif

		CXX := $(CCACHE) $(CXXLANG) -Qunused-arguments
	endif
	LINK ?= $(CXX)

	ifeq ($(origin AR), default)
		AR := $(shell xcrun -sdk $(MAC_SDK) -find libtool) -static -o
	endif

	ifeq ($(TARGET_PLATFORM),osx64)
		ARCH_FLAGS += -arch x86_64 -m64 -march=core2
	else ifeq (,$(findstring -arch x86_64,$(GCC_ExtraCompilerFlags)))
		ARCH_FLAGS += -arch i386 -m32 -march=prescott -momit-leaf-frame-pointer -mtune=core2
	else
		# dirty hack to build a universal binary - don't specify the architecture
		ARCH_FLAGS += -arch i386 -Xarch_i386 -march=prescott -Xarch_i386 -mtune=core2 -Xarch_i386 -momit-leaf-frame-pointer -Xarch_x86_64 -march=core2
	endif

	GEN_SYM ?= $(shell xcrun -sdk $(MAC_SDK) -find dsymutil)
	ifeq ($(CFG),release)
		STRIP ?= strip -S
	else
		STRIP ?= true
	endif
	ifeq ($(SOURCE_SDK), 1)
		VSIGN ?= true
	else
		VSIGN ?= $(SRCROOT)/devtools/bin/vsign
	endif

	CPPFLAGS += -I$(SYSROOT)/usr/include/malloc
	CFLAGS += -isysroot $(SYSROOT) -mmacosx-version-min=10.5 -fasm-blocks

	LIB_START_EXE = -lm -ldl -lpthread
	LIB_END_EXE = 

	LIB_START_SHLIB = 
	LIB_END_SHLIB = 

	SHLIBLDFLAGS = $(LDFLAGS) -bundle -flat_namespace -undefined suppress -Wl,-dead_strip -Wl,-no_dead_strip_inits_and_terms 

	ifeq (lib,$(findstring lib,$(GAMEOUTPUTFILE)))
		SHLIBLDFLAGS = $(LDFLAGS) -dynamiclib -current_version 1.0 -compatibility_version 1.0 -install_name @rpath/$(basename $(notdir $(GAMEOUTPUTFILE))).dylib $(SystemLibraries) -Wl,-dead_strip -Wl,-no_dead_strip_inits_and_terms 
	endif

endif

#
# Profile-directed optimizations.
# Note: Last time these were tested 3/5/08, it actually slowed down the server benchmark by 5%!
#
# First, uncomment these, build, and test. It will generate .gcda and .gcno files where the .o files are.
# PROFILE_LINKER_FLAG=-fprofile-arcs
# PROFILE_COMPILER_FLAG=-fprofile-arcs
#
# Then, comment the above flags out again and rebuild with this flag uncommented:
# PROFILE_COMPILER_FLAG=-fprofile-use
#

#############################################################################
# The compiler command lne for each src code file to compile
#############################################################################

OBJ_DIR = ./obj_$(NAME)_$(TARGET_PLATFORM)$(TARGET_PLATFORM_EXT)/$(CFG)
CPP_TO_OBJ = $(CPPFILES:.cpp=.o)
CXX_TO_OBJ = $(CPP_TO_OBJ:.cxx=.o)
CC_TO_OBJ = $(CXX_TO_OBJ:.cc=.o)
MM_TO_OBJ = $(CC_TO_OBJ:.mm=.o)
C_TO_OBJ = $(MM_TO_OBJ:.c=.o)
OBJS = $(addprefix $(OBJ_DIR)/, $(notdir $(C_TO_OBJ)))

ifeq ($(MAKE_VERBOSE),1)
	QUIET_PREFIX = 
	QUIET_ECHO_POSTFIX = 
else
	QUIET_PREFIX = @
	QUIET_ECHO_POSTFIX = > /dev/null
endif

ifeq ($(MAKE_CC_VERBOSE),1)
CC += -v
endif

ifeq ($(CONFTYPE),lib)
  LIB_File = $(OUTPUTFILE)
endif

ifeq ($(CONFTYPE),dll)
  SO_File = $(OUTPUTFILE)
endif

ifeq ($(CONFTYPE),exe)
  EXE_File = $(OUTPUTFILE)
endif

# we generate dependencies as a side-effect of compilation now
GEN_DEP_FILE=

PRE_COMPILE_FILE = 

POST_COMPILE_FILE = 

ifeq ($(BUILDING_MULTI_ARCH),1)
	SINGLE_ARCH_CXXFLAGS=$(subst -arch x86_64,,$(CXXFLAGS))
	COMPILE_FILE = \
		$(QUIET_PREFIX) \
		echo "---- $(lastword $(subst /, ,$<)) as MULTIARCH----";\
		mkdir -p $(OBJ_DIR) && \
		$(CXX) $(SINGLE_ARCH_CXXFLAGS) $(GENDEP_CXXFLAGS) -o $@ -c $< && \
		$(CXX) $(CXXFLAGS) -o $@ -c $<
else
	COMPILE_FILE = \
		$(QUIET_PREFIX) \
		echo "---- $(lastword $(subst /, ,$<)) ----";\
		mkdir -p $(OBJ_DIR) && \
		$(CXX) $(CXXFLAGS) $(GENDEP_CXXFLAGS) -o $@ -c $<
endif

ifneq "$(origin VALVE_NO_AUTO_P4)" "undefined"
	P4_EDIT_START = chmod -R +w
	P4_EDIT_END = || true
	P4_REVERT_START = true
	P4_REVERT_END =
else
	ifndef P4_EDIT_CHANGELIST
		# You can use an environment variable to specify what changelist to check the Linux Binaries out into. Normally the default
		# setting is best, but here is an alternate example:
		# export P4_EDIT_CHANGELIST_CMD="echo 1424335"
		# ?= means that if P4_EDIT_CHANGELIST_CMD is already set it won't be changed.
		P4_EDIT_CHANGELIST_CMD ?= p4 changes -c `p4 client -o | grep ^Client | cut -f 2` -s pending | fgrep 'POSIX Auto Checkout' | cut -d' ' -f 2 | tail -n 1
		P4_EDIT_CHANGELIST := $(shell $(P4_EDIT_CHANGELIST_CMD))
	endif
	ifeq ($(P4_EDIT_CHANGELIST),)
		# If we haven't found a changelist to check out to then create one. The name must match the one from a few
		# lines above or else a new changelist will be created each time.
		# Warning: the behavior of 'echo' is not consistent. In bash you need the "-e" option in order for \n to be
		# interpreted as a line-feed, but in dash you do not, and if "-e" is passed along then it is printed, which
		# confuses p4. So, if you run this command from the bash shell don't forget to add "-e" to the echo command.
		P4_EDIT_CHANGELIST = $(shell echo "Change: new\nDescription: POSIX Auto Checkout" | p4 change -i | cut -f 2 -d ' ')
	endif

	P4_EDIT_START := for f in
	P4_EDIT_END := ; do if [ -n $$f ]; then if [ -d $$f ]; then find $$f -type f -print | p4 -x - edit -c $(P4_EDIT_CHANGELIST); else p4 edit -c $(P4_EDIT_CHANGELIST) $$f; fi; fi; done $(QUIET_ECHO_POSTFIX)
	P4_REVERT_START := for f in  
	P4_REVERT_END := ; do if [ -n $$f ]; then if [ -d $$f ]; then find $$f -type f -print | p4 -x - revert; else p4 revert $$f; fi; fi; done $(QUIET_ECHO_POSTFIX) 
endif

ifeq ($(CONFTYPE),dll)
all: $(OTHER_DEPENDENCIES) $(OBJS) $(GAMEOUTPUTFILE)
	@echo $(GAMEOUTPUTFILE) $(QUIET_ECHO_POSTFIX)
else
all: $(OTHER_DEPENDENCIES) $(OBJS) $(OUTPUTFILE)
	@echo $(OUTPUTFILE) $(QUIET_ECHO_POSTFIX)
endif

.PHONY: clean cleantargets cleanandremove rebuild relink RemoveOutputFile SingleFile


rebuild :
	$(MAKE) -f $(firstword $(MAKEFILE_LIST)) cleanandremove
	$(MAKE) -f $(firstword $(MAKEFILE_LIST))


# Use the relink target to force to relink the project.
relink: RemoveOutputFile all

RemoveOutputFile:
	rm -f $(OUTPUTFILE)


# This rule is so you can say "make SingleFile SingleFilename=/home/myname/valve_main/src/engine/language.cpp" and have it only build that file.
# It basically just translates the full filename to create a dependency on the appropriate .o file so it'll build that.
SingleFile : RemoveSingleFile $(OBJ_DIR)/$(basename $(notdir $(SingleFilename))).o
	@echo ""

RemoveSingleFile:
	$(QUIET_PREFIX) rm -f $(OBJ_DIR)/$(basename $(notdir $(SingleFilename))).o

clean:
ifneq "$(OBJ_DIR)" ""
	$(QUIET_PREFIX) echo "rm -rf $(OBJ_DIR)"
	$(QUIET_PREFIX) rm -rf $(OBJ_DIR)
endif
ifneq "$(OUTPUTFILE)" ""
	$(QUIET_PREFIX) if [ -e $(OUTPUTFILE) ]; then \
		echo "p4 revert $(OUTPUTFILE)"; \
		$(P4_REVERT_START) $(OUTPUTFILE) $(OUTPUTFILE)$(SYM_EXT) $(P4_REVERT_END); \
	fi;
endif
ifneq "$(OTHER_DEPENDENCIES)" ""
	$(QUIET_PREFIX) echo "rm -f $(OTHER_DEPENDENCIES)"
	$(QUIET_PREFIX) rm -f $(OTHER_DEPENDENCIES)
endif
ifneq "$(GAMEOUTPUTFILE)" ""
	$(QUIET_PREFIX) echo "p4 revert $(GAMEOUTPUTFILE)"
	$(QUIET_PREFIX) $(P4_REVERT_START) $(GAMEOUTPUTFILE) $(GAMEOUTPUTFILE)$(SYM_EXT) $(P4_REVERT_END)
endif


# Do the above cleaning, except with p4 edit and rm. Reason being ar crs adds and replaces obj files to the
# archive. However if you've renamed or deleted a source file, $(AR) won't remove it. This can leave
# us with archive files that have extra unused symbols, and also potentially cause compilation errors
# when you rename a file and have many duplicate symbols.
cleanandremove:
ifneq "$(OBJ_DIR)" ""
	$(QUIET_PREFIX) echo "rm -rf $(OBJ_DIR)"
	$(QUIET_PREFIX) -rm -rf $(OBJ_DIR)
endif
ifneq "$(OUTPUTFILE)" ""
	$(QUIET_PREFIX) if [ -e $(OUTPUTFILE) ]; then \
		echo "p4 edit and rm -f $(OUTPUTFILE) $(OUTPUTFILE)$(SYM_EXT)"; \
		$(P4_EDIT_START) $(OUTPUTFILE) $(OUTPUTFILE)$(SYM_EXT) $(P4_EDIT_END); \
	fi;
	$(QUIET_PREFIX) -rm -f $(OUTPUTFILE) $(OUTPUTFILE)$(SYM_EXT);
endif
ifneq "$(OTHER_DEPENDENCIES)" ""
	$(QUIET_PREFIX) echo "rm -f $(OTHER_DEPENDENCIES)"
	$(QUIET_PREFIX) -rm -f $(OTHER_DEPENDENCIES)
endif
ifneq "$(GAMEOUTPUTFILE)" ""
	$(QUIET_PREFIX) echo "p4 edit and rm -f $(GAMEOUTPUTFILE) $(GAMEOUTPUTFILE)$(SYM_EXT)"
	$(QUIET_PREFIX) $(P4_EDIT_START) $(GAMEOUTPUTFILE) $(GAMEOUTPUTFILE)$(SYM_EXT) $(P4_EDIT_END)
	$(QUIET_PREFIX) -rm -f $(GAMEOUTPUTFILE)
endif


# This just deletes the final targets so it'll do a relink next time we build.
cleantargets:
	$(QUIET_PREFIX) rm -f $(OUTPUTFILE) $(GAMEOUTPUTFILE)


$(LIB_File): $(OTHER_DEPENDENCIES) $(OBJS) 
	$(QUIET_PREFIX) -$(P4_EDIT_START) $(LIB_File) $(P4_EDIT_END); 
	$(QUIET_PREFIX) $(AR) $(LIB_File) $(OBJS) $(LIBFILES);

SO_GameOutputFile = $(GAMEOUTPUTFILE)

# Remove the target before installing a file over it; this prevents existing
# instances of the game from crashing due to the overwrite.
$(SO_GameOutputFile): $(SO_File)
	$(QUIET_PREFIX) \
	$(P4_EDIT_START) $(GAMEOUTPUTFILE) $(P4_EDIT_END) && \
	echo "----" $(QUIET_ECHO_POSTFIX);\
	echo "---- COPYING TO $@ [$(CFG)] ----";\
	echo "----" $(QUIET_ECHO_POSTFIX);
	$(QUIET_PREFIX) -$(P4_EDIT_START) $(GAMEOUTPUTFILE) $(P4_EDIT_END);
	$(QUIET_PREFIX) -mkdir -p `dirname $(GAMEOUTPUTFILE)` > /dev/null;
	$(QUIET_PREFIX) rm -f $(GAMEOUTPUTFILE) $(QUIET_ECHO_POSTFIX);
	$(QUIET_PREFIX) cp -v $(OUTPUTFILE) $(GAMEOUTPUTFILE) $(QUIET_ECHO_POSTFIX);
	$(QUIET_PREFIX) -$(P4_EDIT_START) $(GAMEOUTPUTFILE)$(SYM_EXT) $(P4_EDIT_END);
	$(QUIET_PREFIX) $(GEN_SYM) $(GAMEOUTPUTFILE); 
	$(QUIET_PREFIX) -$(STRIP) $(GAMEOUTPUTFILE);
	$(QUIET_PREFIX) $(VSIGN) -signvalve $(GAMEOUTPUTFILE);
	$(QUIET_PREFIX) if [ "$(COPY_DLL_TO_SRV)" = "1" ]; then\
		echo "----" $(QUIET_ECHO_POSTFIX);\
		echo "---- COPYING TO $(Srv_GAMEOUTPUTFILE) ----";\
		echo "----" $(QUIET_ECHO_POSTFIX);\
		cp -v $(GAMEOUTPUTFILE) $(Srv_GAMEOUTPUTFILE) $(QUIET_ECHO_POSTFIX);\
		cp -v $(GAMEOUTPUTFILE)$(SYM_EXT) $(Srv_GAMEOUTPUTFILE)$(SYM_EXT) $(QUIET_ECHO_POSTFIX);\
	fi;
	$(QUIET_PREFIX) if [ "$(IMPORTLIBRARY)" != "" ]; then\
		echo "----" $(QUIET_ECHO_POSTFIX);\
		echo "---- COPYING TO IMPORT LIBRARY $(IMPORTLIBRARY) ----";\
		echo "----" $(QUIET_ECHO_POSTFIX);\
		$(P4_EDIT_START) $(IMPORTLIBRARY) $(P4_EDIT_END) && \
		mkdir -p `dirname $(IMPORTLIBRARY)` > /dev/null && \
		cp -v $(OUTPUTFILE) $(IMPORTLIBRARY); \
	fi;


$(SO_File): $(OTHER_DEPENDENCIES) $(OBJS) $(LIBFILENAMES)
	$(QUIET_PREFIX) \
	echo "----" $(QUIET_ECHO_POSTFIX);\
	echo "---- LINKING $@ [$(CFG)] ----";\
	echo "----" $(QUIET_ECHO_POSTFIX);\
	\
	$(LINK) $(LINK_MAP_FLAGS) $(SHLIBLDFLAGS) $(PROFILE_LINKER_FLAG) -o $(OUTPUTFILE) $(LIB_START_SHLIB) $(OBJS) $(LIBFILES) $(SystemLibraries) $(LIB_END_SHLIB);
	$(VSIGN) -signvalve $(OUTPUTFILE);


$(EXE_File) : $(OTHER_DEPENDENCIES) $(OBJS) $(LIBFILENAMES)
	$(QUIET_PREFIX) \
	echo "----" $(QUIET_ECHO_POSTFIX);\
	echo "---- LINKING EXE $@ [$(CFG)] ----";\
	echo "----" $(QUIET_ECHO_POSTFIX);\
	\
	$(P4_EDIT_START) $(OUTPUTFILE) $(P4_EDIT_END);\
	$(LINK) $(LINK_MAP_FLAGS) $(LDFLAGS) $(PROFILE_LINKER_FLAG) -o $(OUTPUTFILE) $(LIB_START_EXE) $(OBJS) $(LIBFILES) $(SystemLibraries) $(LIB_END_EXE);
	$(QUIET_PREFIX) -$(P4_EDIT_START) $(OUTPUTFILE)$(SYM_EXT) $(P4_EDIT_END);
	$(QUIET_PREFIX) $(GEN_SYM) $(OUTPUTFILE);
	$(QUIET_PREFIX) -$(STRIP) $(OUTPUTFILE);
	$(QUIET_PREFIX) $(VSIGN) -signvalve $(OUTPUTFILE);


tags:
	etags -a -C -o $(SRCROOT)/TAGS *.cpp *.cxx *.h *.hxx
