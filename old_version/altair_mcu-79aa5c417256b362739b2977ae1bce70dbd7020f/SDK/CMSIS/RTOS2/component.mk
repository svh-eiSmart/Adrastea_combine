# Component makefile for CMSIS_RTOS
INC_DIRS += $(cmsis_rtos_ROOT)/FreeRTOS/Include

cmsis_rtos_SRC_FILES += $(cmsis_rtos_ROOT)/FreeRTOS/Source/cmsis_os2.c
cmsis_rtos_SRC_FILES += $(cmsis_rtos_ROOT)/FreeRTOS/Source/cmsis_os1.c
cmsis_rtos_SRC_FILES += $(cmsis_rtos_ROOT)/FreeRTOS/Source/os_systick.c

$(eval $(call component_compile_rules,cmsis_rtos))
