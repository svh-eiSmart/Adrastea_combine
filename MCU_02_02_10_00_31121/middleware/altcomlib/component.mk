INC_DIRS += $(altcomlib_ROOT)/include \
			$(altcomlib_ROOT)/include/net \
			$(altcomlib_ROOT)/include/mbedtls \
			$(altcomlib_ROOT)/include/opt \
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
			$(altcomlib_ROOT)/include/sms \
			$(altcomlib_ROOT)/include/http \
			$(altcomlib_ROOT)/include/io \
			$(altcomlib_ROOT)/include/fs \
			$(altcomlib_ROOT)/altcom/include \
			$(altcomlib_ROOT)/altcom/include/builder \
			$(altcomlib_ROOT)/altcom/include/common \
			$(altcomlib_ROOT)/altcom/include/util \
			$(altcomlib_ROOT)/altcom/include/api \
			$(altcomlib_ROOT)/altcom/include/api/common \
			$(altcomlib_ROOT)/altcom/include/api/lte \
			$(altcomlib_ROOT)/altcom/include/api/sms \
			$(altcomlib_ROOT)/altcom/include/api/atcmd \
			$(altcomlib_ROOT)/altcom/include/api/socket \
			$(altcomlib_ROOT)/altcom/include/api/mbedtls \
			$(altcomlib_ROOT)/altcom/include/api/mqtt \
			$(altcomlib_ROOT)/altcom/include/api/gps \
			$(altcomlib_ROOT)/altcom/include/api/certmgmt \
			$(altcomlib_ROOT)/altcom/include/api/misc \
			$(altcomlib_ROOT)/altcom/include/api/lwm2m \
			$(altcomlib_ROOT)/altcom/include/api/atsocket \
			$(altcomlib_ROOT)/altcom/include/api/atcmd \
			$(altcomlib_ROOT)/altcom/include/api/coap \
			$(altcomlib_ROOT)/altcom/include/api/http \
			$(altcomlib_ROOT)/altcom/include/api/io \
			$(altcomlib_ROOT)/altcom/include/api/fs \
			$(altcomlib_ROOT)/altcom/include/evtdisp \
			$(altcomlib_ROOT)/altcom/include/gw \
			$(1250core_ROOT)

# args for passing into compile rule generation

altcomlib_MAIN = $(altcomlib_ROOT)
altcomlib_INC_DIR = # all in INC_DIRS, needed for normal operation
altcomlib_SRC_DIR = $(altcomlib_ROOT)/altcom/builder \
				  $(altcomlib_ROOT)/altcom/common \
				  $(altcomlib_ROOT)/altcom/evtdisp \
				  $(altcomlib_ROOT)/util \
				  $(altcomlib_ROOT)/opt \
				  $(altcomlib_ROOT)/altcom/api/common

ifeq ($(findstring __ENABLE_LTE_API__,$(CONFIG_H)),__ENABLE_LTE_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/lte
endif

ifeq ($(findstring __ENABLE_ATCMD_API__,$(CONFIG_H)),__ENABLE_ATCMD_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/atcmd
endif

ifeq ($(findstring __ENABLE_SOCKET_API__,$(CONFIG_H)),__ENABLE_SOCKET_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/socket
endif

ifeq ($(findstring __ENABLE_MBEDTLS_API__,$(CONFIG_H)),__ENABLE_MBEDTLS_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/mbedtls
endif

ifeq ($(findstring __ENABLE_MQTT_API__,$(CONFIG_H)),__ENABLE_MQTT_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/mqtt
else ifeq ($(findstring __ENABLE_AWS_API__,$(CONFIG_H)),__ENABLE_AWS_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/mqtt
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/aws
endif

ifeq ($(findstring __ENABLE_GPS_API__,$(CONFIG_H)),__ENABLE_GPS_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/gps
endif

ifeq ($(findstring __ENABLE_CERTMGMT_API__,$(CONFIG_H)),__ENABLE_CERTMGMT_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/certmgmt
endif

ifeq ($(findstring __ENABLE_MISC_API__,$(CONFIG_H)),__ENABLE_MISC_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/misc
endif

ifeq ($(findstring __ENABLE_LWM2M_API__,$(CONFIG_H)),__ENABLE_LWM2M_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/lwm2m
endif

ifeq ($(findstring __ENABLE_ATSOCKET_API__,$(CONFIG_H)),__ENABLE_ATSOCKET_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/atsocket
endif

ifeq ($(findstring __ENABLE_SMS_API__,$(CONFIG_H)),__ENABLE_SMS_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/sms
endif

ifeq ($(findstring __ENABLE_COAP_API__,$(CONFIG_H)),__ENABLE_COAP_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/coap
endif

ifeq ($(findstring __ENABLE_HTTP_API__,$(CONFIG_H)),__ENABLE_HTTP_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/http
endif

ifeq ($(findstring __ENABLE_IO_API__,$(CONFIG_H)),__ENABLE_IO_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/io
endif

ifeq ($(findstring __ENABLE_FS_API__,$(CONFIG_H)),__ENABLE_FS_API__)
altcomlib_SRC_DIR += $(altcomlib_ROOT)/altcom/api/fs
endif

#GW and HAL source
altcomlib_EXTRA_SRC_FILES = $(altcomlib_ROOT)/altcom/gw/apicmdgw.c

ifeq ($(findstring HAL_UART_ALT125X_MCU,$(CONFIG_H)),HAL_UART_ALT125X_MCU)
altcomlib_EXTRA_SRC_FILES += $(altcomlib_ROOT)/altcom/gw/hal_uart_alt125x.c
else ifeq ($(findstring HAL_EMUX_ALT125X,$(CONFIG_H)),HAL_EMUX_ALT125X)
altcomlib_EXTRA_SRC_FILES += $(altcomlib_ROOT)/altcom/gw/hal_emux_alt125x.c
else
echo "No HAL Implementation"
endif

$(eval $(call component_compile_rules,altcomlib))

