# Component makefile for OSAL

OSAL_DIR = $(osal_ROOT)
INC_DIRS += $(OSAL_DIR)/Include

osal_SRC_FILES = $(OSAL_DIR)/Source/freertos_osal.c

$(eval $(call component_compile_rules,osal))
