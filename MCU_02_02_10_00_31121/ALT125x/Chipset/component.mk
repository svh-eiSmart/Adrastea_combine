INC_DIRS += $(alt125x_device_ROOT)/Include $(alt125x_device_ROOT)/Include/$(DEVICE)

alt125x_device_SRC_FILES += $(alt125x_device_ROOT)/Source/startup_ALT125x.c \
			    $(alt125x_device_ROOT)/Source/system_ALT125x.c \

LINKER_SCRIPTS += $(alt125x_device_ROOT)/Source/GCC/$(DEVICE)_defs.ld
LINKER_SCRIPTS += $(alt125x_device_ROOT)/Source/GCC/ALT125x_flash.ld

EXTRA_CFLAGS += -DDEVICE_HEADER=\"$(DEVICE).h\" -DCMSIS_device_header=\"$(DEVICE).h\"

$(eval $(call component_compile_rules,alt125x_device))

