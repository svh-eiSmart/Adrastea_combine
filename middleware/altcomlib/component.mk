INC_DIRS += $(altcomlib_ROOT)/include \
			$(altcomlib_ROOT)/include/net \
			$(altcomlib_ROOT)/include/opt \
			$(altcomlib_ROOT)/include/osal \
			$(altcomlib_ROOT)/include/util \
			$(altcomlib_ROOT)/include/aws \
			$(altcomlib_ROOT)/include/mqtt \
			$(altcomlib_ROOT)/include/atcmd \
			$(altcomlib_ROOT)/include/gps \
			$(altcomlib_ROOT)/include/certmgmt \
			$(altcomlib_ROOT)/include/misc \
			$(altcomlib_ROOT)/include/lwm2m \
			$(altcomlib_ROOT)/include/atsocket \
			$(altcomlib_ROOT)/include/coap \
			$(altcomlib_ROOT)/altcom/include \
			$(altcomlib_ROOT)/altcom/include/builder \
			$(altcomlib_ROOT)/altcom/include/common \
			$(altcomlib_ROOT)/altcom/include/util \
			$(altcomlib_ROOT)/altcom/include/api \
			$(altcomlib_ROOT)/altcom/include/api/common \
			$(altcomlib_ROOT)/altcom/include/api/lte \
			$(altcomlib_ROOT)/altcom/include/api/atcmd \
			$(altcomlib_ROOT)/altcom/include/api/socket \
			$(altcomlib_ROOT)/altcom/include/api/mqtt \
			$(altcomlib_ROOT)/altcom/include/api/gps \
			$(altcomlib_ROOT)/altcom/include/api/certmgmt \
			$(altcomlib_ROOT)/altcom/include/api/misc \
			$(altcomlib_ROOT)/altcom/include/api/lwm2m \
			$(altcomlib_ROOT)/altcom/include/api/atsocket \
			$(altcomlib_ROOT)/altcom/include/api/atcmd \
			$(altcomlib_ROOT)/altcom/include/api/coap \
			$(altcomlib_ROOT)/altcom/include/evtdisp \
			$(altcomlib_ROOT)/altcom/include/gw \
			$(1250core_ROOT)

# args for passing into compile rule generation

altcomlib_MAIN = $(altcomlib_ROOT)
altcomlib_INC_DIR = # all in INC_DIRS, needed for normal operation
altcomlib_SRC_DIR = $(altcomlib_ROOT)/altcom/builder \
				  $(altcomlib_ROOT)/altcom/common \
				  $(altcomlib_ROOT)/altcom/evtdisp \
				  $(altcomlib_ROOT)/osal/openrtos \
				  $(altcomlib_ROOT)/util \
				  $(altcomlib_ROOT)/opt \
				  $(altcomlib_ROOT)/altcom/api/common

ifeq ($(findstring -D__ENABLE_LTE_API__,$(EXTRA_CFLAGS)),-D__ENABLE_LTE_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/lte
endif

ifeq ($(findstring -D__ENABLE_ATCMD_API__,$(EXTRA_CFLAGS)),-D__ENABLE_ATCMD_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/atcmd
endif

ifeq ($(findstring -D__ENABLE_SOCKET_API__,$(EXTRA_CFLAGS)),-D__ENABLE_SOCKET_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/socket
endif

ifeq ($(findstring -D__ENABLE_MQTT_API__,$(EXTRA_CFLAGS)),-D__ENABLE_MQTT_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/mqtt
else ifeq ($(findstring -D__ENABLE_AWS_API__,$(EXTRA_CFLAGS)),-D__ENABLE_AWS_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/mqtt
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/aws
endif

ifeq ($(findstring -D__ENABLE_GPS_API__,$(EXTRA_CFLAGS)),-D__ENABLE_GPS_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/gps
endif

ifeq ($(findstring -D__ENABLE_CERTMGMT_API__,$(EXTRA_CFLAGS)),-D__ENABLE_CERTMGMT_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/certmgmt
endif

ifeq ($(findstring -D__ENABLE_MISC_API__,$(EXTRA_CFLAGS)),-D__ENABLE_MISC_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/misc
endif

ifeq ($(findstring -D__ENABLE_LWM2M_API__,$(EXTRA_CFLAGS)),-D__ENABLE_LWM2M_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/lwm2m
endif

ifeq ($(findstring -D__ENABLE_ATSOCKET_API__,$(EXTRA_CFLAGS)),-D__ENABLE_ATSOCKET_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/atsocket
endif

ifeq ($(findstring -D__ENABLE_COAP_API__,$(EXTRA_CFLAGS)),-D__ENABLE_COAP_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/coap
endif


#GW and HAL source
altcomlib_EXTRA_SRC_FILES = $(altcomlib_ROOT)/altcom/gw/apicmdgw.c

ifeq ($(findstring -DHAL_UART_ALT125X_MCU,$(EXTRA_CFLAGS)),-DHAL_UART_ALT125X_MCU)
altcomlib_EXTRA_SRC_FILES += $(altcomlib_ROOT)/altcom/gw/hal_uart_alt125x.c
else ifeq ($(findstring -DHAL_EMUX_ALT125X,$(EXTRA_CFLAGS)),-DHAL_EMUX_ALT125X)
altcomlib_EXTRA_SRC_FILES += $(altcomlib_ROOT)/altcom/gw/hal_emux_alt125x.c
else ifeq ($(findstring -DHAL_UART_NXP,$(EXTRA_CFLAGS)),-DHAL_UART_NXP)
altcomlib_EXTRA_SRC_FILES += $(altcomlib_ROOT)/altcom/gw/hal_uart_nxp.c
else ifeq ($(findstring -DHAL_EMUX_NXP,$(EXTRA_CFLAGS)),-DHAL_EMUX_NXP)
altcomlib_EXTRA_SRC_FILES += $(altcomlib_ROOT)/altcom/gw/hal_emux_nxp.c
else
echo "No HAL Implementation"
endif

$(eval $(call component_compile_rules,altcomlib))

