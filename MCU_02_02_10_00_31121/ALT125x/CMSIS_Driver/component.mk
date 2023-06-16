INC_DIRS += $(cmsis_driver_impl_ROOT)/
cmsis_driver_impl_SRC_FILES += $(cmsis_driver_impl_ROOT)/I2C_CMSIS_ADL.c \
                               $(cmsis_driver_impl_ROOT)/SPI_CMSIS_ADL.c \
                               $(cmsis_driver_impl_ROOT)/UART_CMSIS_ADL.c
$(eval $(call component_compile_rules,cmsis_driver_impl))
