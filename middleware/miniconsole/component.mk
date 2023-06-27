miniconsole_DIR = $(miniconsole_ROOT)

miniconsole_SRC_DIR = $(miniconsole_DIR)

INC_DIRS += $(miniconsole_DIR)

$(eval $(call component_compile_rules,miniconsole))
