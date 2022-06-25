CC := $(CROSS_COMPILE)gcc

CXX := $(CROSS_COMPILE)g++

STRIP := $(CROSS_COMPILE)strip

AR := $(CROSS_COMPILE)ar

#CFLAGS += -Werror -Wno-write-strings -Wno-sign-compare -fPIC
CFLAGS += -g  -O3 -fPIC
ifeq ($(DEBUG_VERSION), y)
CFLAGS += -g
endif

#CXXFLAGS += -fno-strict-aliasing -traditional -Wno-reorder -Wno-char-subscripts -std=c++11
CXXFLAGS += -g -std=c++11
export CC
export CXX
export STRIP
export AR
export CFLAGS
export CXXFLAGS