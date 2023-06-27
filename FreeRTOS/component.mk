freertos_MAIN = $(freertos_ROOT)/lib
freertos_INC_DIR = $(freertos_MAIN)/include $(freertos_MAIN)/include/private

INC_DIRS += $(freertos_INC_DIR)
EXTRA_LDFLAGS += -Wl,--undefined=uxTopUsedPriority

freertos_SRC_FILES += $(freertos_MAIN)/FreeRTOS/list.c
freertos_SRC_FILES += $(freertos_MAIN)/FreeRTOS/timers.c
freertos_SRC_FILES += $(freertos_MAIN)/FreeRTOS/event_groups.c
freertos_SRC_FILES += $(freertos_MAIN)/FreeRTOS/queue.c
freertos_SRC_FILES += $(freertos_MAIN)/FreeRTOS/tasks.c
freertos_SRC_FILES += $(freertos_MAIN)/FreeRTOS/stream_buffer.c
freertos_SRC_FILES += $(freertos_MAIN)/FreeRTOS/FreeRTOS-openocd.c

ifneq (,$(filter "$(DEVICE)","ALT1250" "ALT1255"))
    freertos_INC_DIR += $(freertos_MAIN)/FreeRTOS/portable/GCC/ARM_CM3
    freertos_SRC_FILES += $(freertos_MAIN)/FreeRTOS/portable/GCC/ARM_CM3/alt125x_tickless_sleep.c
    freertos_SRC_FILES += $(freertos_MAIN)/FreeRTOS/portable/GCC/ARM_CM3/port.c
    freertos_SRC_FILES += $(freertos_MAIN)/FreeRTOS/portable/MemMang/heap_useNewlib.c
endif

ifeq ("$(DEVICE)","MK28F15")
    freertos_INC_DIR += $(freertos_MAIN)/FreeRTOS/portable/GCC/ARM_CM4F
    freertos_SRC_FILES += $(freertos_MAIN)/FreeRTOS/portable/GCC/ARM_CM4F/port.c
    freertos_SRC_FILES += $(freertos_MAIN)/FreeRTOS/portable/GCC/ARM_CM4F/fsl_tickless_systick.c
    freertos_SRC_FILES += $(freertos_MAIN)/FreeRTOS/portable/MemMang/heap_4.c
endif

$(eval $(call component_compile_rules,freertos))
