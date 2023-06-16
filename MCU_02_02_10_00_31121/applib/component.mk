INC_DIRS += $(applib_ROOT)

applib_SRC_FILES += $(applib_ROOT)/timex.c \
		    $(applib_ROOT)/backtrace.c \
		    $(applib_ROOT)/fault_handler.c \
		    $(applib_ROOT)/reset.c \
		    $(applib_ROOT)/applib.c \
		    $(applib_ROOT)/newlibPort.c

EXTRA_CFLAGS += -D$(DEVICE)
EXTRA_LDFLAGS += -nostartfiles

$(eval $(call component_compile_rules,applib))

$(FW_FILE): $(PROGRAM_OUT) $(BUILD_DIR)
	$(vecho) "FW $@"
	$(Q) $(ROOT)utils/genfw.sh $@


