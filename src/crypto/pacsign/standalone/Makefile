SHELL := /bin/bash
 
# Enable second expansion
.SECONDEXPANSION:
# Clear all built in suffixes
.SUFFIXES:
.DELETE_ON_ERROR:

##############################################################################
# Set default goal before any targets. The default goal here is "all"
##############################################################################
DEFAULT_TARGET := all
 
.DEFAULT_GOAL := default
.PHONY: default
default: $(DEFAULT_TARGET)

all: 
	arc shell python/3.6.5 -- bash runme.sh

clean: 
	rm *.dat *pem *qky
