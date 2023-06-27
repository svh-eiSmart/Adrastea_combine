# Component makefile for LWIP

PPPOS_TYPE ?= $(DEVICE)

LWIP_DIR = $(lwip_ROOT)/lwip-2.0.3/src
INC_DIRS += $(LWIP_DIR)/include $(ROOT)/lwip/include $(lwip_ROOT)/include $(LWIP_DIR)/include/compat/posix $(LWIP_DIR)/include/ipv4 $(LWIP_DIR)/include/ipv4/lwip $(LWIP_DIR)/include/netif/ppp

# args for passing into compile rule generation
lwip_INC_DIR =  # all in INC_DIRS, needed for normal operation
lwip_SRC_DIR = $(lwip_ROOT) $(LWIP_DIR)/api $(LWIP_DIR)/core $(LWIP_DIR)/core/ipv4 $(LWIP_DIR)/core/ipv6 $(LWIP_DIR)/netif/ppp
lwip_EXTRA_SRC_FILES = $(LWIP_DIR)/netif/ethernet.c
lwip_SRC_DIR += $(LWIP_DIR)/netif/ppp/polarssl $(LWIP_DIR)/apps/ping $(LWIP_DIR)/apps/ppp $(LWIP_DIR)/apps/net_debug $(LWIP_DIR)/apps/lwiperf

ifneq (,$(filter "$(PPPOS_TYPE)","ALT1250" "ALT1255"))
lwip_SRC_DIR += $(LWIP_DIR)/netif/12xx_netif
INC_DIRS += $(LWIP_DIR)/include/netif/12xx_netif
endif

ifeq ("$(PPPOS_TYPE)","EMUX")
lwip_SRC_DIR += $(LWIP_DIR)/netif/emux_netif
INC_DIRS += $(LWIP_DIR)/include/netif/emux_netif
endif

ifeq ("$(PPPOS_TYPE)","MK28F15")
lwip_SRC_DIR += $(LWIP_DIR)/netif/mk28f15_netif
INC_DIRS += $(LWIP_DIR)/include/netif/mk28f15_netif
endif
$(eval $(call component_compile_rules,lwip))
