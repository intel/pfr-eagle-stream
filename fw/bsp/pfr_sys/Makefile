# Enable second expansion
.SECONDEXPANSION:

# Clear all built in suffixes
.SUFFIXES:

#------------------------------------------------------------------------------
#                         The adjust-path macro
#
# If Make is launched from Windows through
# Windows Subsystem for Linux (WSL).  The adjust-path macro converts absolute windows 
# paths into unix style paths (Example: c:/dir -> /c/dir). 
# The adjust_path_mixed function converts WSL path to Windows path.
# This will ensure paths are readable by GNU Make.
#------------------------------------------------------------------------------

UNAME = $(shell uname -r)
ifeq ($(findstring Microsoft,$(UNAME)),Microsoft)
	WINDOWS_EXE = .exe
	export WSLENV=$WSLENV:PATH/p
endif

ifdef WINDOWS_EXE 
	adjust-path = $(if $1,$(shell wslpath "$1"),)
	adjust-path-mixed = $(if $1,$(shell wslpath -m "$1"),)
else # !WINDOWS_EXE
	adjust-path = $1
	adjust-path-mixed = $1
endif

##############################################################################
# Set default goal before any targets. The default goal here is "test"
##############################################################################
DEFAULT_TARGET := all

.DEFAULT_GOAL := default
.PHONY: default
default: $(DEFAULT_TARGET)

##############################################################################
# Settings
##############################################################################

##############################################################################
# Rules
##############################################################################
.PHONY : clean
clean :
	-rm -rf bsp settings.bsp.tmp

bsp :
	mkdir bsp

settings.bsp.tmp : settings.bsp
	cp -f $^ $@
	chmod u+w $@

bsp/Makefile : settings.bsp.tmp ../../../src/pfr_sys/pfr_sys.sopcinfo | bsp
	nios2-bsp-generate-files$(WINDOWS_EXE) --bsp-dir bsp --settings settings.bsp.tmp

bsp/libhal_bsp.a : bsp/Makefile
	$(info Building $@)
	$(MAKE) -C bsp

.PHONY : all
all: bsp/libhal_bsp.a

