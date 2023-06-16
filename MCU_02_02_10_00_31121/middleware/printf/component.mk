printf_SRC_DIR = $(printf_ROOT)

CFLAGS += -include $(printf_ROOT)/printf.h

$(eval $(call component_compile_rules,printf))
