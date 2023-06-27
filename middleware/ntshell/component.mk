ntshell_DIR = $(ntshell_ROOT)/src/lib/

ntshell_SRC_DIR = $(ntshell_DIR)/core $(ntshell_DIR)/util

INC_DIRS += $(ntshell_DIR)/core $(ntshell_DIR)/util

$(eval $(call component_compile_rules,ntshell))
