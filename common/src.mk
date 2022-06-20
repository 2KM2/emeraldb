TARGET := libcommon.so

SRC_DIR := $(shell find $(TARGET_DIR) -maxdepth 5 -type d)

SRC_FILE_C := $(foreach d, $(SRC_DIR), $(wildcard $(d)/*.c))
SRC_FILE_CPP := $(foreach d, $(SRC_DIR), $(wildcard $(d)/*.cpp))

INCLUDE_H_FLAGS := $(foreach d, $(shell find $(TOP_DIR)/include -maxdepth 5 -type d), -I$(d))
INCLUDE_H_FLAGS += $(foreach d, $(shell find $(TARGET_DIR) -maxdepth 5 -type d), -I$(d))

CFLAGS += -Wno-psabi

LD_LIB_FLAGS := -L$(OPENSOURCE_DIR)/boost/stage/lib 
LD_DYNAMIC_LIBS := -lm -lboost_system -lboost_thread -lboost_program_options -lrt -lpthread
CXXFLAGS += -I $(OPENSOURCE_DIR)/boost -D_FILE_OFFSET_BITS=64 -ggdb -Wall -O0
