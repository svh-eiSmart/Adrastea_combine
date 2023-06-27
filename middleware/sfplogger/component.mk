INC_DIRS += $(sfplogger_ROOT)/include

# args for passing into compile rule generation
sfplogger_INC_DIR =  # all in INC_DIRS, needed for normal operation
sfplogger_SRC_DIR = $(sfplogger_ROOT)

$(eval $(call component_compile_rules,sfplogger))
