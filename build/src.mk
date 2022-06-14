ifneq ($(wildcard $(TARGET_DIR)/src.mk),)
include $(TARGET_DIR)/src.mk
else
TARGET := $(notdir $(TARGET_DIR))
$(info " build_src.mk " $(TARGET_DIR))

SRC_DIR := $(shell find $(TARGET_DIR) -maxdepth 5 -type d)
SRC_FILE_C := $(foreach d, $(SRC_DIR), $(wildcard $(d)/*.c))
SRC_FILE_CPP := $(foreach d, $(SRC_DIR), $(wildcard $(d)/*.cpp))

INCLUDE_H_FLAGS += $(foreach d, $(SRC_DIR), -I$(d))

LD_LIB_FLAGS :=

LD_DYNAMIC_LIBS :=
endif