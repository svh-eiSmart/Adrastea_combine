ACCEL_LIS2DH_DIR = $(accel_lis2dh_ROOT)/
INC_DIRS += $(ACCEL_LIS2DH_DIR)lib \
            $(ACCEL_LIS2DH_DIR)external/lis2dh12_STdC/driver


# args for passing into compile rule generation
accel_lis2dh_INC_DIR =  # all in INC_DIRS, needed for normal operation
accel_lis2dh_SRC_DIR = $(ACCEL_LIS2DH_DIR)lib \
                       $(ACCEL_LIS2DH_DIR)external/lis2dh12_STdC/driver

$(eval $(call component_compile_rules,accel_lis2dh))
