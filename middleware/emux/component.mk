# Component makefile for EMUX

EMUX_DIR = $(emux_ROOT)/src/
INC_DIRS += $(EMUX_DIR)/include $(EMUX_DIR)/include/los/FreeRTOS

# args for passing into compile rule generation
emux_INC_DIR =  # all in INC_DIRS, needed for normal operation
emux_SRC_DIR = $(EMUX_DIR)api $(EMUX_DIR)core $(EMUX_DIR)los/FreeRTOS

ifneq (,$(filter $(DEVICE),ALT1250 ALT1255))
emux_SRC_DIR += $(EMUX_DIR)serial_hal/ALT125X
else
emux_SRC_DIR += $(EMUX_DIR)serial_hal/$(DEVICE)
endif

$(eval $(call component_compile_rules,emux))
