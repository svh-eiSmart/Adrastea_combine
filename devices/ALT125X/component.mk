INC_DIRS += $(alt125x_ROOT)/include $(alt125x_ROOT)/include/$(DEVICE) $(alt125x_ROOT)/serial

alt125x_SRC_FILES += $(alt125x_ROOT)/atomic_counter/atomic_counter_drv.c
alt125x_SRC_FILES += $(alt125x_ROOT)/auxadc/auxadc.c
alt125x_SRC_FILES += $(alt125x_ROOT)/gdma/gdma.c
alt125x_SRC_FILES += $(alt125x_ROOT)/gpio/gpio.c
alt125x_SRC_FILES += $(alt125x_ROOT)/gpio/gpio_wakeup.c
alt125x_SRC_FILES += $(alt125x_ROOT)/gptimer/gptimer.c
alt125x_SRC_FILES += $(alt125x_ROOT)/hifc/hifc_mi.c
alt125x_SRC_FILES += $(alt125x_ROOT)/i2c/i2c.c
alt125x_SRC_FILES += $(alt125x_ROOT)/led/led.c
alt125x_SRC_FILES += $(alt125x_ROOT)/pm/pwr_mngr.c
alt125x_SRC_FILES += $(alt125x_ROOT)/pm/sleep_mngr.c
alt125x_SRC_FILES += $(alt125x_ROOT)/pm/sleep_notify.c
alt125x_SRC_FILES += $(alt125x_ROOT)/serial/circ_buf.c
alt125x_SRC_FILES += $(alt125x_ROOT)/serial_flash/serial_flash_drv_api_1250b.c
alt125x_SRC_FILES += $(alt125x_ROOT)/serial/newlibPort.c
alt125x_SRC_FILES += $(alt125x_ROOT)/serial/serial_altDbg.c
alt125x_SRC_FILES += $(alt125x_ROOT)/serial/serial.c
alt125x_SRC_FILES += $(alt125x_ROOT)/serial/serial_container.c
alt125x_SRC_FILES += $(alt125x_ROOT)/shadow32ktimer/shadow32ktimer.c
alt125x_SRC_FILES += $(alt125x_ROOT)/spi/spim.c
alt125x_SRC_FILES += $(alt125x_ROOT)/spi/spis.c
alt125x_SRC_FILES += $(alt125x_ROOT)/time/timex.c
alt125x_SRC_FILES += $(alt125x_ROOT)/version.c
alt125x_SRC_FILES += $(alt125x_ROOT)/watchdog/wdt.c
alt125x_SRC_FILES += $(alt125x_ROOT)/src/bt.c
alt125x_SRC_FILES += $(alt125x_ROOT)/src/clk_gating.c
alt125x_SRC_FILES += $(alt125x_ROOT)/src/io_mngr.c
alt125x_SRC_FILES += $(alt125x_ROOT)/src/pll.c
alt125x_SRC_FILES += $(alt125x_ROOT)/src/pmp_agent.c
alt125x_SRC_FILES += $(alt125x_ROOT)/src/system.c

LINKER_SCRIPTS += $(alt125x_ROOT)/src/GCC/alt125x_flash.ld
EXTRA_LDFLAGS += -L$(alt125x_ROOT)/src/GCC/$(DEVICE)
EXTRA_CFLAGS += -D$(DEVICE)
EXTRA_LDFLAGS += -Wl,--wrap,main

ifeq ($(DEVICE),ALT1250)
alt125x_SRC_FILES += $(alt125x_ROOT)/pwm_dac/pwm_dac_alt1250.c
alt125x_SRC_FILES += $(alt125x_ROOT)/src/sysclk_alt1250.c
alt125x_SRC_FILES += $(alt125x_ROOT)/src/if_mngr_alt1250.c
alt125x_SRC_FILES += $(alt125x_ROOT)/src/iosel_alt1250.c
alt125x_SRC_FILES += $(alt125x_ROOT)/src/GCC/ALT1250/startup_alt1250.c
else ifeq ($(DEVICE),ALT1255)
alt125x_SRC_FILES += $(alt125x_ROOT)/ccm/ccm_alt1255.c
alt125x_SRC_FILES += $(alt125x_ROOT)/src/sysclk_alt1255.c
alt125x_SRC_FILES += $(alt125x_ROOT)/src/if_mngr_alt1255.c
alt125x_SRC_FILES += $(alt125x_ROOT)/src/iosel_alt1255.c
alt125x_SRC_FILES += $(alt125x_ROOT)/src/GCC/ALT1255/startup_alt1255.c
else
$(error Unknown device type)
endif

$(eval $(call component_compile_rules,alt125x))

$(FW_FILE): $(PROGRAM_OUT) $(BUILD_DIR)
	$(vecho) "FW $@"
	$(Q) $(OBJCOPY) -O binary $< $@
	$(Q) $(ROOT)utils/genfw.py $@


