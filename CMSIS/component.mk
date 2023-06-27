# Component makefile for CMSIS
INC_DIRS += $(cmsis_ROOT)/Include
$(eval $(call component_compile_rules,cmsis))
