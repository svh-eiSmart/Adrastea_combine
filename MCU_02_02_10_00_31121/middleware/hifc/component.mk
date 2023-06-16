hifc_DIR = $(hifc_ROOT)/src/lib

hifc_SRC_DIR = $(hifc_DIR)/core $(hifc_DIR)/mi

INC_DIRS += $(hifc_DIR)/core $(hifc_DIR)/mi

$(eval $(call component_compile_rules,hifc))
