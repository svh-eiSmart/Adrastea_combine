# Component makefile for EMUX

EMUX_DIR = $(emux_ROOT)/src/
INC_DIRS += $(EMUX_DIR)/include

# args for passing into compile rule generation
emux_INC_DIR =  # all in INC_DIRS, needed for normal operation
emux_SRC_DIR = $(EMUX_DIR)api $(EMUX_DIR)core
emux_SRC_DIR += $(EMUX_DIR)serial_hal/ALT125X

$(eval $(call component_compile_rules,emux))
