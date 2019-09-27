SRCROOT?=..

THISFILE:=$(SRCROOT)/devtools/makefile_base_posix.mak
MAKEFILE_BASE:=$(notdir $(THISFILE))
MAKEFILE_LINK:=$(THISFILE).link

-include $(MAKEFILE_LINK)

$(MAKEFILE_LINK): $(shell which $(CC)) $(THISFILE)
	if [ "$(shell printf "$(shell $(CC) -dumpversion)\n8" | sort -Vr | head -1)" = 8 ]; then \
		ln -sf $(MAKEFILE_BASE).default $@ ;\
	else \
		ln -sf $(MAKEFILE_BASE).gcc8 $@ ;\
	fi
