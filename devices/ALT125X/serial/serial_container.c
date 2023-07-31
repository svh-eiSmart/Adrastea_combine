/*  ---------------------------------------------------------------------------

        (c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.

        This software, in source or object form (the "Software"), is the
        property of Altair Semiconductor Ltd. (the "Company") and/or its
        licensors, which have all right, title and interest therein, You
        may use the Software only in  accordance with the terms of written
        license agreement between you and the Company (the "License").
        Except as expressly stated in the License, the Company grants no
        licenses by implication, estoppel, or otherwise. If you are not
        aware of or do not agree to the License terms, you may not use,
        copy or modify the Software. You may use the source code of the
        Software only for your internal purposes and may not distribute the
        source code of the Software, any part thereof, or any derivative work
        thereof, to any third party, except pursuant to the Company's prior
        written consent.
        The Software is the confidential information of the Company.

   ------------------------------------------------------------------------- */
#include <string.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include "task.h"
#include "serial.h"
#include "serial_container.h"
#include "newlibPort.h"
#include "hifc_api.h"
#include "if_mngr.h"

static volatile unsigned short numOfSerialUartDeviceOpen = 0;
#define maxNumOfSerialUartDeviceAllowed (3)

static void *serialDriverContainer_getInstance(serial_type_id_t type,
                                               logical_hw_representation_id_t logicalId,
                                               sHuart *uartInitParam) {
  uint32_t *uart_base = NULL;
  uint32_t *uart_cfg = NULL;
  uint32_t *serialHandle;
  enum hwSer_channelNumber uartHwIndx;
  IRQn_Type interrupt_number;
  serial_uart_chan_t uartType;

  /* As of now container interact only with serial uart driver */
  if (SERIAL_TYPE_UART_ID != type) {
    printf("%s fail serial type invalid %d !\n", __FUNCTION__, type);
    return NULL;
  }
  if (numOfSerialUartDeviceOpen >= maxNumOfSerialUartDeviceAllowed) {
    printf("%s fail reached max device set %d !\n", __FUNCTION__, maxNumOfSerialUartDeviceAllowed);
    return NULL;
  }
  if (logicalId == LS_REPRESENTATION_ID_UART_A) {
    uartHwIndx = UART0;
    uart_base = (uint32_t *)BASE_ADDRESS_MCU_UARTF0;
    uart_cfg = (uint32_t *)BASE_ADDRESS_MCU_UARTF0_CFG;
    uartType = SERIAL_UART_TYPE_FAST;
    interrupt_number = UARTF0_IRQn;
    print_boot_msg("Open UART-A base = 0x%x cfg =0x%x intrp=%d", uart_base, uart_cfg,
                   interrupt_number);

#ifdef BASE_ADDRESS_MCU_UARTF1
  } else if (logicalId == LS_REPRESENTATION_ID_UART_B) {
    uartHwIndx = UART1;
    uart_base = (uint32_t *)BASE_ADDRESS_MCU_UARTF1;
    uart_cfg = (uint32_t *)BASE_ADDRESS_MCU_UARTF1_CFG;
    uartType = SERIAL_UART_TYPE_FAST;
    interrupt_number = UARTF1_IRQn;
    print_boot_msg("Open UART-B base = 0x%x cfg =0x%x intrp=%d", uart_base, uart_cfg,
                   interrupt_number);
#endif
  } else if (logicalId == LS_REPRESENTATION_ID_UART_0) {
    uartHwIndx = UARTI0;
    uart_base = (uint32_t *)BASE_ADDRESS_MCU_UARTI0;
    uartType = SERIAL_UART_TYPE_SLOW;
    interrupt_number = UARTI0_IRQn;
    print_boot_msg("Open UART-0 base = 0x%x cfg =0x%x intrp=%d", uart_base, uart_cfg,
                   interrupt_number);
  } else if (logicalId == LS_REPRESENTATION_ID_UART_1) {
    uartHwIndx = UARTI1;
    uart_base = (uint32_t *)BASE_ADDRESS_MCU_UARTI1;
    uartType = SERIAL_UART_TYPE_SLOW;
    interrupt_number = UARTI1_IRQn;
    print_boot_msg("Open UART-1 base = 0x%x cfg =0x%x intrp=%d", uart_base, uart_cfg,
                   interrupt_number);
#ifdef BASE_ADDRESS_MCU_UARTI2
  } else if (logicalId == LS_REPRESENTATION_ID_UART_2) {
    uartHwIndx = UARTI2;
    uart_base = (uint32_t *)BASE_ADDRESS_MCU_UARTI2;
    uartType = SERIAL_UART_TYPE_SLOW;
    interrupt_number = UARTI2_IRQn;
    print_boot_msg("Open UART-2 base = 0x%x cfg =0x%x intrp=%d", uart_base, uart_cfg,
                   interrupt_number);
#endif
  } else {
    printf("%s failed, called with invalid logical port  %d !\n", __FUNCTION__, logicalId);
    return NULL;
  }

  serialHandle = serialUartDriverAllocInstance(uart_base, uart_cfg, uartHwIndx, uartType,
                                               interrupt_number, uartInitParam);
  if (!serialHandle) {
    printf("%s fail in calling to serialUartDriverAllocInstance() !\n", __FUNCTION__);
    return NULL;
  }
  numOfSerialUartDeviceOpen++;

  /*  Init HIFC here */
  if (logicalId == LS_REPRESENTATION_ID_UART_0) {
    if (hifc_host_interface_init(uart_base)) {
      printf("HIFC init fail\r\n");
      while (1)
        ;
    }
  }
  return serialHandle;
}

int serial_ioctl(serial_handle *handle, serial_ioctl_id_t serialIoctlId, void *param,
                 void *appCookie) {
  return serialUartDriverIoctl(handle, serialIoctlId, param, appCookie);
}

int serial_init(void) {
  // maxNumOfSerialUartDeviceAllowed = MAX_PHYSICAL_UART_NUM;
  serialUartDriver_createRxTask();
  return 0;
}

int serial_close(serial_handle *handle) {
  if (!handle) return -1;

  if (!serialUartDriverDeallocInstance((void *)handle)) numOfSerialUartDeviceOpen--;

  return 0;
}

int serial_load_defconfig(eUartInstance uartInstance, sHuart *uartInitParam) {
  int ret = 0;
  switch (uartInstance) {
    case ACTIVE_UARTF0:
      if (if_mngr_load_defconfig(IF_MNGR_UARTF0, uartInitParam) != IF_MNGR_SUCCESS) ret = -1;

      // Use UARTF0
      break;
#ifdef ALT1250
    case ACTIVE_UARTF1:
      if (if_mngr_load_defconfig(IF_MNGR_UARTF1, uartInitParam) != IF_MNGR_SUCCESS) ret = -1;
      // Use UARTF1
      break;
#endif
    case ACTIVE_UARTI0:
      uartInitParam->Instance = ACTIVE_UARTI0;
      uartInitParam->Init.BaudRate = UARTI0_BAUDRATE_DEFAULT;
      uartInitParam->Init.WordLength = UARTI0_WORDLENGTH_DEFAULT;
      uartInitParam->Init.StopBits = UARTI0_STOPBITS_DEFAULT;
      uartInitParam->Init.Parity = UARTI0_PARITY_DEFAULT;
      uartInitParam->Init.HwFlowCtl = UARTI0_HWFLOWCTRL_DEFAULT;
      break;
    default:
      ret = -1;
      break;
  }

  return ret;
}

serial_handle *serial_open(sHuart *uartInitParam) {
#define CONFIG_ACTIVE_UART_MSK 0x3
  unsigned int ConfActUart = (int)uartInitParam->Instance;
  serial_handle *sHandle = NULL;
  logical_hw_representation_id_t uartConf = LS_REPRESENTATION_LAST;

  switch (ConfActUart & CONFIG_ACTIVE_UART_MSK) {
    case ACTIVE_UARTF0:
      // Use UARTF0
      uartConf = LS_REPRESENTATION_ID_UART_A;
      break;
#ifdef ALT1250
    case ACTIVE_UARTF1:
      // Use UARTF1
      uartConf = LS_REPRESENTATION_ID_UART_B;
      break;
#endif
    case ACTIVE_UARTI0:
      uartConf = LS_REPRESENTATION_ID_UART_0;
      break;
    default:
      return NULL;
  }

  if (uartConf != LS_REPRESENTATION_LAST) {
    sHandle = serialDriverContainer_getInstance(SERIAL_TYPE_UART_ID, uartConf, uartInitParam);
    serialUartDriverHwInit(sHandle);
    // register Rx callback. Could it be set in container open?
    // serialUartDriverIoctl(sHandle, IOCTAL_SERIAL_RX_CB_FUNC, (void*)writeToCliRxQueu, (void
    // *)sHandle);
    serial_ioctl(sHandle, IOCTAL_SERIAL_RX_CB_FUNC, (void *)writeToSerialRxQueu, (void *)sHandle);
  }
  if (uartConf == LS_REPRESENTATION_ID_UART_0) set_hifc_mode(hifc_mode_b);

  return sHandle;
}

uint32_t serial_read(serial_handle *handle, char *buf, uint32_t len) {
  if (!handle || !buf) return 0;
  char c;
  uint32_t index;
  serial_uart_ctxt_t *sHandle = (serial_uart_ctxt_t *)handle;

  /* Wait until something arrives in the queue - this task will block
  indefinitely provided INCLUDE_vTaskSuspend is set to 1 in FreeRTOSConfig.h. */
  for (index = 0; index < len; index++) {
    if (xQueueReceive(sHandle->xQueue_rx, &c, portMAX_DELAY) != pdPASS) {
      break;
    }
    buf[index] = c;
     //if(sHandle->uartHwIndx == 2)
     //printf("line 219 serial_container.c Serial read: HwIdx:%d Q:%p buf:%c buf_c: %c\r\n",sHandle->uartHwIndx,sHandle->xQueue_rx, c, &c);
    //printf("buf:%c \r\n", c);


  }

  return index;
}

uint32_t serial_write(serial_handle *handle, const char *buf, uint32_t len) {
  if (!handle || !buf) return 0;

  return serialUartDriverWrite(handle, buf, len);
}
