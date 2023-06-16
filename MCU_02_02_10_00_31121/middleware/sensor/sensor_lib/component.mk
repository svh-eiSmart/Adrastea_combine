SENSOR_LIB_DIR = $(sensor_lib_ROOT)/
INC_DIRS += $(SENSOR_LIB_DIR)src


# args for passing into compile rule generation
sensor_lib_INC_DIR =  # all in INC_DIRS, needed for normal operation
sensor_lib_SRC_DIR = $(SENSOR_LIB_DIR)src

$(eval $(call component_compile_rules,sensor_lib))
