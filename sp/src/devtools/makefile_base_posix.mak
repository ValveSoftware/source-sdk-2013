SRCROOT?=..

THISFILE:=$(SRCROOT)/devtools/makefile_base_posix.mak
MAKEFILE_BASE:=$(notdir $(THISFILE))
MAKEFILE_LINK:=$(THISFILE).link

-include $(MAKEFILE_LINK)

# depend on CXX so the correct makefile can be selected when the system is updated
$(MAKEFILE_LINK): $(shell which $(CXX)) $(THISFILE) $(SRCROOT)/devtools/gcc9+support.cpp
	@ if [ "$(shell printf "$(shell $(CXX) -dumpversion)\n8" | sort -Vr | head -1)" = 8 ]; then \
		ln -sf $(MAKEFILE_BASE).default $@ ;\
	else \
		$(COMPILE.cpp) -m32 -o $(SRCROOT)/devtools/gcc9+support.o $(SRCROOT)/devtools/gcc9+support.cpp &&\
		ln -sf $(MAKEFILE_BASE).gcc8 $@ ;\
	fi
