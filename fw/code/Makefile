
# Enable second expansion
.SECONDEXPANSION:

# Clear all built in suffixes
.SUFFIXES:

##############################################################################
# Set default goal before any targets. The default goal here is "all"
##############################################################################
DEFAULT_TARGET := all

.DEFAULT_GOAL := default
.PHONY: default
default: $(DEFAULT_TARGET)

##############################################################################
# Platforms
##############################################################################

PLATFORM_HW := \
	archer_city \
	archer_city_recovery

PLATFORM_SIM := \
	archer_city

    
BUILD_PLATFORM_TARGETS := $(patsubst %,build-%,$(PLATFORM_HW))
SIM_PLATFORM_TARGETS := $(patsubst %,sim-%,$(PLATFORM_SIM))
CLEAN_PLATFORM_HW_TARGETS := $(patsubst %,clean-hw-%,$(PLATFORM_HW))
CLEAN_PLATFORM_SIM_TARGETS := $(patsubst %,clean-sim-%,$(PLATFORM_SIM))

##############################################################################
# Makefile starts here
##############################################################################

.PHONY: all
all: $(BUILD_PLATFORM_TARGETS) sim


.PHONY: $(BUILD_PLATFORM_TARGETS)
$(BUILD_PLATFORM_TARGETS): build-% : hw/%
	$(MAKE) -C $<


.PHONY: sim
sim: $(SIM_PLATFORM_TARGETS)

.PHONY: $(SIM_PLATFORM_TARGETS)
$(SIM_PLATFORM_TARGETS): sim-% : sim/%
ifneq ($(PORT),WINDOWS)
	$(MAKE) -C $<
endif


.PHONY: clean
clean: $(CLEAN_PLATFORM_HW_TARGETS) $(CLEAN_PLATFORM_SIM_TARGETS)

.PHONY: $(CLEAN_PLATFORM_HW_TARGETS)
$(CLEAN_PLATFORM_HW_TARGETS): clean-hw-% : hw/%
	-$(MAKE) -C $< clean
	
.PHONY: $(CLEAN_PLATFORM_SIM_TARGETS)
$(CLEAN_PLATFORM_SIM_TARGETS): clean-sim-% : sim/%
	-$(MAKE) -C $< clean

