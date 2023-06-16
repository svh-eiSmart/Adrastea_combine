serial_SRC_DIR = $(serial_ROOT)

INC_DIRS += $(serial_SRC_DIR)

$(eval $(call component_compile_rules,serial))
