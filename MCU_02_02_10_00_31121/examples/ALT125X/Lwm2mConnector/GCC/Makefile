PROGRAM = Lwm2mConnector

PROGRAM_INC_DIR += $(PROGRAM_DIR)/source/middleware

PROGRAM_SRC_DIR += $(PROGRAM_DIR)/source \
					$(PROGRAM_DIR)/source/middleware/osal \
					$(PROGRAM_DIR)/source/middleware/ReportManager

EXTRA_COMPONENTS = middleware/printf \
				middleware/osal \
				middleware/hifc \
				middleware/serial \
				middleware/miniconsole \
				middleware/altcomlib \
				middleware/sfplogger

include ../../../../common.mk
