INC_DIRS += $(alt125x_driver_ROOT)/Include \
	    $(alt125x_driver_ROOT)/Include/$(DEVICE) \
	    $(alt125x_driver_ROOT)/Source/serial \
	    $(alt125x_driver_ROOT)/Source/Private \
	    $(alt125x_driver_ROOT)/Source/Private/$(DEVICE)

# alt125x_driver_INC_DIR += $(alt125x_driver_ROOT)/Source/Private \
#			  $(alt125x_driver_ROOT)/Source/Private/$(DEVICE)

alt125x_driver_SRC_FILES += $(alt125x_driver_ROOT)/Source/DRV_IO.c \
			    $(alt125x_driver_ROOT)/Source/DRV_AUXADC.c \
			    $(alt125x_driver_ROOT)/Source/DRV_GDMA.c \
			    $(alt125x_driver_ROOT)/Source/DRV_GPTIMER.c \
			    $(alt125x_driver_ROOT)/Source/DRV_WATCHDOG.c \
			    $(alt125x_driver_ROOT)/Source/DRV_I2C.c \
			    $(alt125x_driver_ROOT)/Source/DRV_LED.c \
			    $(alt125x_driver_ROOT)/Source/DRV_GPIO.c \
			    $(alt125x_driver_ROOT)/Source/DRV_SPI.c \
			    $(alt125x_driver_ROOT)/Source/DRV_UART.c \
			    $(alt125x_driver_ROOT)/Source/DRV_FLASH.c \
			    $(alt125x_driver_ROOT)/Source/DRV_IO.c \
				$(alt125x_driver_ROOT)/Source/DRV_PM.c \
				$(alt125x_driver_ROOT)/Source/DRV_OTP.c \
			    $(alt125x_driver_ROOT)/Source/Private/DRV_GPIO_WAKEUP.c \
			    $(alt125x_driver_ROOT)/Source/Private/$(DEVICE)/DRV_IF_CFG.c \
			    $(alt125x_driver_ROOT)/Source/Private/$(DEVICE)/DRV_IOSEL.c \
			    $(alt125x_driver_ROOT)/Source/Private/DRV_CLOCK_GATING.c \
			    $(alt125x_driver_ROOT)/Source/Private/DRV_32KTIMER.c \
			    $(alt125x_driver_ROOT)/Source/Private/DRV_ATOMIC_COUNTER.c \
			    $(alt125x_driver_ROOT)/Source/Private/DRV_PMP_AGENT.c \
			    $(alt125x_driver_ROOT)/Source/Private/DRV_SB_ENABLE.c \
			    $(alt125x_driver_ROOT)/Source/Private/sha256.c \
			    $(alt125x_driver_ROOT)/Source/Private/DRV_SLEEP.c \



ifeq ($(DEVICE),ALT1250)
alt125x_driver_SRC_FILES += $(alt125x_driver_ROOT)/Source/DRV_PWM_DAC.c
endif

ifeq ($(DEVICE),ALT1255)
alt125x_driver_SRC_FILES += $(alt125x_driver_ROOT)/Source/DRV_CCM.c
endif

$(eval $(call component_compile_rules,alt125x_driver))

