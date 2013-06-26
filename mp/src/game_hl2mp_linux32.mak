# VPC MASTER MAKEFILE



SHELL:=/bin/bash
# to control parallelism, set the MAKE_JOBS environment variable
ifeq ($(strip $(MAKE_JOBS)),)
	ifeq ($(shell uname),Darwin)
		CPUS:=$(shell /usr/sbin/sysctl -n hw.ncpu)
	endif
	ifeq ($(shell uname),Linux)
		CPUS:=$(shell grep processor /proc/cpuinfo | wc -l)
	endif
	MAKE_JOBS:=$(CPUS)
endif

ifeq ($(strip $(MAKE_JOBS)),)
	MAKE_JOBS:=8
endif

# All projects (default target)
all: 
	@$(MAKE) -f $(lastword $(MAKEFILE_LIST)) -j$(MAKE_JOBS) all-targets

all-targets : client_hl2mp server_hl2mp tier1 mathlib vgui_controls fixsrvso


# Individual projects + dependencies

client_hl2mp : tier1 mathlib vgui_controls 
	@echo "Building: client_hl2mp"
	@+cd game/client && $(MAKE) -f client_linux32_hl2mp.mak $(CLEANPARAM)

server_hl2mp : tier1 mathlib 
	@echo "Building: server_hl2mp"
	@+cd game/server && $(MAKE) -f server_linux32_hl2mp.mak $(CLEANPARAM)

tier1 : 
	@echo "Building: tier1"
	@+cd tier1 && $(MAKE) -f tier1_linux32.mak $(CLEANPARAM)

mathlib : 
	@echo "Building: mathlib"
	@+cd mathlib && $(MAKE) -f mathlib_linux32.mak $(CLEANPARAM)

vgui_controls : 
	@echo "Building: vgui_controls"
	@+cd vgui2/vgui_controls && $(MAKE) -f vgui_controls_linux32.mak $(CLEANPARAM)

fixsrvso : server_hl2mp
	@echo "Copying server.so to server_srv.so"
	@+cd ../game/mod_hl2mp/bin && cp server.so server_srv.so
	@+cd ../game/mod_hl2mp/bin && cp server.so.dbg server_srv.so.dbg

# this is a bit over-inclusive, but the alternative (actually adding each referenced c/cpp/h file to
# the tags file) seems like more work than it's worth.  feel free to fix that up if it bugs you. 
TAGS:
	@rm -f TAGS
	@find game/client -name '*.cpp' -print0 | xargs -0 etags --declarations --ignore-indentation --append
	@find game/client -name '*.h' -print0 | xargs -0 etags --language=c++ --declarations --ignore-indentation --append
	@find game/client -name '*.c' -print0 | xargs -0 etags --declarations --ignore-indentation --append
	@find game/server -name '*.cpp' -print0 | xargs -0 etags --declarations --ignore-indentation --append
	@find game/server -name '*.h' -print0 | xargs -0 etags --language=c++ --declarations --ignore-indentation --append
	@find game/server -name '*.c' -print0 | xargs -0 etags --declarations --ignore-indentation --append
	@find tier1 -name '*.cpp' -print0 | xargs -0 etags --declarations --ignore-indentation --append
	@find tier1 -name '*.h' -print0 | xargs -0 etags --language=c++ --declarations --ignore-indentation --append
	@find tier1 -name '*.c' -print0 | xargs -0 etags --declarations --ignore-indentation --append
	@find mathlib -name '*.cpp' -print0 | xargs -0 etags --declarations --ignore-indentation --append
	@find mathlib -name '*.h' -print0 | xargs -0 etags --language=c++ --declarations --ignore-indentation --append
	@find mathlib -name '*.c' -print0 | xargs -0 etags --declarations --ignore-indentation --append
	@find vgui2/vgui_controls -name '*.cpp' -print0 | xargs -0 etags --declarations --ignore-indentation --append
	@find vgui2/vgui_controls -name '*.h' -print0 | xargs -0 etags --language=c++ --declarations --ignore-indentation --append
	@find vgui2/vgui_controls -name '*.c' -print0 | xargs -0 etags --declarations --ignore-indentation --append



# Mark all the projects as phony or else make will see the directories by the same name and think certain targets 

.PHONY: TAGS showtargets regen showregen clean cleantargets cleanandremove relink client_hl2mp server_hl2mp tier1 mathlib vgui_controls 



# The standard clean command to clean it all out.

clean: 
	@$(MAKE) -f $(lastword $(MAKEFILE_LIST)) -j$(MAKE_JOBS) all-targets CLEANPARAM=clean



# clean targets, so we re-link next time.

cleantargets: 
	@$(MAKE) -f $(lastword $(MAKEFILE_LIST)) -j$(MAKE_JOBS) all-targets CLEANPARAM=cleantargets



# p4 edit and remove targets, so we get an entirely clean build.

cleanandremove: 
	@$(MAKE) -f $(lastword $(MAKEFILE_LIST)) -j$(MAKE_JOBS) all-targets CLEANPARAM=cleanandremove



#relink

relink: cleantargets 
	@$(MAKE) -f $(lastword $(MAKEFILE_LIST)) -j$(MAKE_JOBS) all-targets



# Here's a command to list out all the targets


showtargets: 
	@echo '-------------------' && \
	echo '----- TARGETS -----' && \
	echo '-------------------' && \
	echo 'clean' && \
	echo 'client_hl2mp' && \
	echo 'server_hl2mp' && \
	echo 'tier1' && \
	echo 'mathlib' && \
	echo 'vgui_controls'



