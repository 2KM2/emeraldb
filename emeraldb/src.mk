TARGET :=emeraldb

SRC_DIR := $(shell find $(TARGET_DIR) -maxdepth 5 -type d)

SRC_FILE_C := $(foreach d, $(SRC_DIR), $(wildcard $(d)/*.c))

SRC_FILE_CPP := $(foreach d, $(SRC_DIR), $(wildcard $(d)/*.cpp))


INCLUDE_H_FLAGS += $(foreach d, $(SRC_DIR), -I$(d))


INCLUDE_H_FLAGS +=

$(info " emeraldb " $(TOP_DIR))
LD_LIB_FLAGS := -fPIC -rdynamic -L $(OPENSOURCE_DIR)/boost/stage/lib  -pthread
LD_DYNAMIC_LIBS :=-lpthread -lm -lboost_system -lboost_thread -lboost_program_options -lrt
CXXFLAGS := -I $(OPENSOURCE_DIR)/boost -D_FILE_OFFSET_BITS=64 -ggdb -Wall -O0