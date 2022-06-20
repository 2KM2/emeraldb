MK_FULLNAME := $(abspath $(lastword $(MAKEFILE_LIST)))
TOP_DIR := $(patsubst %/,%,$(dir $(MK_FULLNAME)))
BUILD_DIR := $(TOP_DIR)/build
BUILD_OUTPUT_DIR := $(BUILD_DIR)/output
RELEASE_DIR := $(BUILD_DIR)/release
OPENSOURCE_DIR :=$(TOP_DIR)/opensource
export TOP_DIR BUILD_DIR BUILD_OUTPUT_DIR RELEASE_DIR  OPENSOURCE_DIR

RELEASE_LIB_DIR := $(RELEASE_DIR)/lib
RELEASE_BIN_DIR := $(RELEASE_DIR)/bin
RELEASE_TEST_DIR := $(RELEASE_DIR)/test
RELEASE_CFG_DIR := $(RELEASE_DIR)/config
export RELEASE_LIB_DIR RELEASE_BIN_DIR RELEASE_TEST_DIR RELEASE_CFG_DIR


BUILD := -f $(BUILD_DIR)/build.mk SUB_DIR


include $(BUILD_DIR)/optbase.mk
include $(BUILD_DIR)/optcompiler.mk

MAKEFLAGS += --no-print-directory

TARGETS_LIB := common
#BIN TARGET目录

TARGETS_BIN += edb
TARGETS_BIN += emeraldb



TARGETS := $(TARGETS_LIB) $(TARGETS_BIN)

all: $(TARGETS)


$(TARGETS_BIN): common
$(TARGETS):
	@$(MAKE) $(BUILD)=$@

set_lib_path:
	export LD_LIBRARY_PATH=${RELEASE_LIB_DIR}

subclean := $(foreach d,$(TARGETS),clean-$(d))
$(subclean):
	@$(MAKE) $(BUILD)=$(subst clean-,,$@) clean

clean: $(subclean)

distclean: clean
	rm -rf $(BUILD_OUTPUT_DIR)
	rm -rf $(RELEASE_DIR)

.PHONY: all clean distclean $(TARGETS)
