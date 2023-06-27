hifc_DIR = $(hifc_ROOT)/src/lib

hifc_SRC_DIR = $(hifc_DIR)/core $(hifc_DIR)/osal/openrtos

INC_DIRS += $(hifc_DIR)/core $(hifc_DIR)/osal/openrtos

$(eval $(call component_compile_rules,hifc))
