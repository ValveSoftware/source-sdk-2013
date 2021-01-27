SRCROOT?=..

THISFILE:=$(SRCROOT)/devtools/makefile_base_posix.mak
MAKEFILE_BASE:=$(notdir $(THISFILE))
MAKEFILE_LINK:=$(THISFILE).link

-include $(MAKEFILE_LINK)

$(MAKEFILE_LINK): $(shell which $(CXX)) $(THISFILE)
	@ if [ "$(shell printf "$(shell $(CXX) -dumpversion)\n8" | sort -Vr | head -1)" = 8 ]; then \
		ln -sf $(MAKEFILE_BASE).default $@ ;\
	else \
		$(COMPILE.cpp) -o $(SRCROOT)/devtools/gcc9+support.o $(SRCROOT)/devtools/gcc9+support.cpp &&\
		ln -sf $(MAKEFILE_BASE).gcc8 $@ ;\
	fi
