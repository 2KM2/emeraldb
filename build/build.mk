###
all:

USE_GCC := n
TARGET_DIR := $(TOP_DIR)/$(SUB_DIR)
BUILD_OUTPUT_SUB_DIR := $(BUILD_OUTPUT_DIR)/$(SUB_DIR)

include $(BUILD_DIR)/src.mk

FINAL_TARGET := $(BUILD_OUTPUT_SUB_DIR)/$(TARGET)

$(info $(TARGET_DIR) $(TARGET) $(SUB_DIR))

OBJ_OF_C := $(patsubst $(TARGET_DIR)/%.o,$(BUILD_OUTPUT_SUB_DIR)/%.o,$(patsubst %.c,%.o,$(SRC_FILE_C)))
OBJ_OF_CPP := $(patsubst $(TARGET_DIR)/%.o,$(BUILD_OUTPUT_SUB_DIR)/%.o,$(patsubst %.cpp,%.o,$(SRC_FILE_CPP)))

INCLUDE_H_FLAGS +=

LD_LIB_FLAGS +=

LD_LIBS := $(LD_DYNAMIC_LIBS) $(LD_STATIC_LIBS)

ifeq ($(USE_GCC), y)
BUILD_TOOL := $(CC) $(CFLAGS)
else
BUILD_TOOL := $(CXX) $(CFLAGS) $(CXXFLAGS)
endif

define check_for_mkdir
	if [ ! -d $1 ]; then \
		mkdir -p $1; \
	fi
endef

###### target judge start
ifneq ($(filter %.a, $(TARGET)),)
INSTALL_RELEASE_DIR := $(RELEASE_LIB_DIR)
define BUILD_LD_CMD
	$(AR) -rc $1 $2 $(LD_LIB_FLAGS) $(LD_LIBS)
endef

else ifneq ($(filter %.so, $(TARGET)),)
INSTALL_RELEASE_DIR := $(RELEASE_LIB_DIR)
define BUILD_LD_CMD
	$(BUILD_TOOL) -shared -fPIC -o $1 $2 $(LD_LIB_FLAGS) $(LD_LIBS)
endef

else
ifneq ($(filter test/%, $(SUB_DIR)),)
INSTALL_RELEASE_DIR := $(RELEASE_TEST_DIR)
else
INSTALL_RELEASE_DIR := $(RELEASE_BIN_DIR)
endif
define BUILD_LD_CMD
	$(BUILD_TOOL) -o $1 $2 $(LD_LIB_FLAGS) $(LD_LIBS)
endef

endif
###### target judge end
all: $(FINAL_TARGET)

$(FINAL_TARGET):$(EX_TARGET) $(OBJ_OF_C) $(OBJ_OF_CPP)
	@echo "BUILD   $(TARGET) $(EX_TARGET)"
	@$(call BUILD_LD_CMD,$@,$(filter-out $(EX_TARGET),$^))
	@install -D $@ $(INSTALL_RELEASE_DIR)/$(TARGET)

$(OBJ_OF_C):$(BUILD_OUTPUT_SUB_DIR)/%.o:$(TARGET_DIR)/%.c
	@$(call check_for_mkdir,$(dir $@))
	@echo "CC      $(subst $(BUILD_OUTPUT_DIR)/,,$@)"
	@$(CC) $(CFLAGS) $(INCLUDE_H_FLAGS) -c -o $@ $<


$(OBJ_OF_CPP):$(BUILD_OUTPUT_SUB_DIR)/%.o:$(TARGET_DIR)/%.cpp
	@$(call check_for_mkdir,$(dir $@))
	@echo "CXX     $(subst $(BUILD_OUTPUT_DIR)/,,$@)"
	@$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDE_H_FLAGS) -c -o $@ $<

clean: $(foreach d,$(EX_TARGET),clean-$(d))
	@rm -rf $(BUILD_OUTPUT_SUB_DIR)

.PHONY: clean $(FINAL_TARGET)