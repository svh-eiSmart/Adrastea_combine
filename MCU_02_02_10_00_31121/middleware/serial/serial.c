/*  ---------------------------------------------------------------------------

    (c) copyright 2021 Altair Semiconductor, Ltd. All rights reserved.

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
#include <FreeRTOS.h>

#include DEVICE_HEADER
#include "DRV_UART.h"
#include "DRV_COMMON.h"
#include "DRV_IF_CFG.h"
#include "serial.h"
#include "alt_osal.h"

extern uint32_t SystemCoreClock;
static uint32_t Get_System_Clock(void);
static uint32_t Get_Ref_Clock(void);

#define CONTAINER_ASSERT(x) \
  if ((x) == 0) {           \
    for (;;)                \
      ;                     \
  } /**< Assertion check */

#define EVT_FLAG_COMPLETE (1 << 0)
#define EVT_FLAG_ABORT (1 << 1)

#define RINGBUFFER_SIZE_UART_F0 (512)
#define RINGBUFFER_SIZE_UART_F1 (512)
#define RINGBUFFER_SIZE_UART_I0 (512)

typedef struct {
  UART_Type *base;
  UARTF_CFG_Type *cfg_base;
  IRQn_Type irq_base;
  Interface_Id interface_id;
  uint32_t (*get_clockrate)(void);
  DRV_UART_Handle *drv_handle;

  int8_t *ringbuffer;
  size_t ringbuffer_size;

  size_t received_length;
  size_t sent_length;
  uint8_t is_initialized;
  serial_callback callback;
  /* OS component*/
  alt_osal_task_handle tx_task_handle;
  alt_osal_task_handle rx_task_handle;
} UART_Resource;

static DRV_UART_Handle handle_uartf0;
static DRV_UART_Handle handle_uarti0;

int8_t uartf0_ringbuffer[RINGBUFFER_SIZE_UART_F0];
int8_t uarti0_ringbuffer[RINGBUFFER_SIZE_UART_I0];
static UART_Resource uartf0_resource = {
    .base = UARTF0,
    .cfg_base = UARTF0_CFG,
    .irq_base = UARTF0_IRQn,
    .get_clockrate = Get_System_Clock,
    .drv_handle = &handle_uartf0,
    .interface_id = IF_CFG_UARTF0,
    .ringbuffer = uartf0_ringbuffer,
    .ringbuffer_size = RINGBUFFER_SIZE_UART_F0,
    .is_initialized = 0,
    .tx_task_handle = NULL,
    .rx_task_handle = NULL,
    .callback = NULL,
};
static UART_Resource uarti0_resource = {
    .base = UARTI0,
    .cfg_base = NULL,
    .irq_base = UARTI0_IRQn,
    .get_clockrate = Get_Ref_Clock,
    .drv_handle = &handle_uarti0,
    .interface_id = IF_CFG_LAST_IF,
    .ringbuffer = uarti0_ringbuffer,
    .ringbuffer_size = RINGBUFFER_SIZE_UART_I0,
    .is_initialized = 0,
    .tx_task_handle = NULL,
    .rx_task_handle = NULL,
    .callback = NULL,
};

#ifdef ALT1250
static int8_t uartf1_ringbuffer[RINGBUFFER_SIZE_UART_F1];
static DRV_UART_Handle handle_uartf1;
static UART_Resource uartf1_resource = {
    .base = UARTF1,
    .cfg_base = UARTF1_CFG,
    .irq_base = UARTF1_IRQn,
    .get_clockrate = Get_System_Clock,
    .drv_handle = &handle_uartf1,
    .interface_id = IF_CFG_UARTF1,
    .ringbuffer = uartf1_ringbuffer,
    .ringbuffer_size = RINGBUFFER_SIZE_UART_F1,
    .is_initialized = 0,
    .tx_task_handle = NULL,
    .rx_task_handle = NULL,
    .callback = NULL,
};
#endif

#if defined(ALT1250)
#define resources \
  { &uartf0_resource, &uartf1_resource, &uarti0_resource }
#elif defined(ALT1255)
#define resources \
  { &uartf0_resource, &uarti0_resource }
#else
#error No target platform specified!!
#endif

static UART_Resource *uart_resources[] = resources;

static UART_Resource *Get_Resource(eUartInstance index) {
  if (index > ACTIVE_UARTI0) return NULL;

  return uart_resources[(int)index];
}

static uint32_t Get_System_Clock(void) { return SystemCoreClock; }

static uint32_t Get_Ref_Clock(void) { return REF_CLK; }
static void UART_EventCallback(DRV_UART_Handle *handle, uint32_t status, void *user_data) {
  UART_Resource *resource = (UART_Resource *)user_data;

  switch (status) {
    case CB_RX_COMPLETE:
      // send rx complete signal
      alt_osal_set_taskflag(resource->rx_task_handle, EVT_FLAG_COMPLETE);
      break;
    case CB_TX_COMPLETE:
      // send tx complete signal
      alt_osal_set_taskflag(resource->tx_task_handle, EVT_FLAG_COMPLETE);
      break;
    case CB_BREAK_ERROR:
      if( resource->callback )
      {
        resource->callback((serial_handle)resource, SERIAL_STATUS_BREAK);
      }
      break;
    default:
      break;
  }
}
serial_handle serial_open(eUartInstance instance) {
  DRV_UART_Config uart_config;
  UART_Resource *resource;

  if (instance >= ACTIVE_MAX) {
    return NULL;
  }

  resource = Get_Resource(instance);
  if (!resource) {
    return NULL;
  }
  if (resource->is_initialized) {
    return (serial_handle)resource;
  }

  if (resource->cfg_base) {
    resource->cfg_base->EN_FLOW_INT_b.UARTINT = 1;  // enable interrupt
  }

  DRV_IF_CFG_SetIO(resource->interface_id);

  if (resource->base == UARTI0) {
    uart_config.BaudRate = UARTI0_BAUDRATE_DEFAULT;
    uart_config.HwFlowCtl = UARTI0_HWFLOWCTRL_DEFAULT;
    uart_config.Parity = UARTI0_PARITY_DEFAULT;
    uart_config.StopBits = UARTI0_STOPBITS_DEFAULT;
    uart_config.WordLength = UARTI0_WORDLENGTH_DEFAULT;
  } else {
    if (DRV_IF_CFG_GetDefConfig(resource->interface_id, &uart_config) != DRV_IF_CFG_OK) {
      return NULL;
    }
  }

  DRV_UART_Initialize(resource->base, &uart_config, resource->get_clockrate());
  DRV_UART_Create_Handle(resource->base, resource->drv_handle, UART_EventCallback, (void *)resource,
                         resource->ringbuffer, resource->ringbuffer_size);

  NVIC_SetPriority(resource->irq_base, 7); /* set Interrupt priority */
  NVIC_EnableIRQ(resource->irq_base);
  resource->is_initialized = 1;
  return (serial_handle)resource;
}
int32_t serial_close(serial_handle handle) {
  UART_Resource *resource = (UART_Resource *)handle;
  resource->is_initialized = 0;

  NVIC_DisableIRQ(resource->irq_base);
  resource->tx_task_handle = NULL;
  resource->rx_task_handle = NULL;
  DRV_UART_Uninitialize(resource->drv_handle);
  return 0;
}
size_t serial_read(serial_handle handle, void *buf, size_t len) {
  UART_Resource *resource = (UART_Resource *)handle;
  DRV_UART_Handle *drv_handle = resource->drv_handle;
  size_t read_size = 0;
  uint32_t flag = 0;
  DRV_UART_Status ret;

  if (resource->rx_task_handle) {
    // this function doesn't allow to be called from mutiple task.
    CONTAINER_ASSERT(0);
  }

  resource->rx_task_handle = alt_osal_get_current_task_handle();

  ret = DRV_UART_Receive_Nonblock(drv_handle, buf, len);
  if (ret == DRV_UART_ERROR_NONE) {
    // all requested data are copied to user buffer
    read_size = len;
  } else if (ret == DRV_UART_WAIT_CB) {
    flag = alt_osal_wait_taskflag(EVT_FLAG_COMPLETE | EVT_FLAG_ABORT, ALT_OSAL_WMODE_TWF_ORW,
                                  ALT_OSAL_TIMEO_FEVR);

    if (flag & EVT_FLAG_ABORT) {
      read_size = resource->received_length;  // the information is stored in abort_read
    }
    if (flag & EVT_FLAG_COMPLETE) {
      read_size = DRV_UART_Get_Received_Rx_Count(drv_handle);
      CONTAINER_ASSERT(read_size == len);
      alt_osal_clear_taskflag(EVT_FLAG_ABORT);
    }
  } else {
    read_size = 0;
  }
  resource->rx_task_handle = NULL;
  return read_size;
}

size_t serial_write(serial_handle handle, void *buf, size_t len) {
  UART_Resource *resource = (UART_Resource *)handle;
  DRV_UART_Handle *drv_handle = resource->drv_handle;
  uint32_t flag;
  size_t wrote_size = 0;
  DRV_UART_Status ret;

  if (resource->tx_task_handle) {
    // this function doesn't allow to be called from mutiple task.
    CONTAINER_ASSERT(0);
  }

  resource->tx_task_handle = alt_osal_get_current_task_handle();

  do {
    alt_osal_clear_taskflag(EVT_FLAG_COMPLETE | EVT_FLAG_ABORT);
    ret = DRV_UART_Send_Nonblock(drv_handle, buf, len);
    if (ret == DRV_UART_ERROR_NONE) {
      // wrote_size = len;
      wrote_size = DRV_UART_Get_Transmitted_Tx_Count(drv_handle);
      CONTAINER_ASSERT(wrote_size == len);  // for testing. remove it when stable
    } else if (ret == DRV_UART_WAIT_CB) {
      flag = alt_osal_wait_taskflag(EVT_FLAG_COMPLETE | EVT_FLAG_ABORT, ALT_OSAL_WMODE_TWF_ORW,
                                    ALT_OSAL_TIMEO_FEVR);

      if (flag & EVT_FLAG_ABORT) {
        wrote_size = resource->sent_length;
      }
      if (flag & EVT_FLAG_COMPLETE) {
        wrote_size = DRV_UART_Get_Transmitted_Tx_Count(drv_handle);
        CONTAINER_ASSERT(wrote_size == len);  // for testing. remove it when stable
        alt_osal_clear_taskflag(EVT_FLAG_ABORT);
      }
    } else {
      wrote_size = 0;
    }
  } while (ret == DRV_UART_ERROR_HIFC_TIMEOUT);  // retry if got hifc timeout error

  resource->tx_task_handle = NULL;
  return wrote_size;
}

int32_t serial_ioctl(serial_handle handle, eIoctl request, void *arg )
{
  UART_Resource *resource = (UART_Resource *)handle;

  switch(request)
  {
    case SERIAL_REGISTER_CALLBACK:
      if( arg )
      {
        resource->callback = (serial_callback)arg;
      }
      break;
    case SERIAL_ENABLE_BREAK_ERROR:
      DRV_UART_Enable_Break_Interrupt(resource->base);
      break;
    case SERIAL_DISABLE_BREAK_ERROR:
      DRV_UART_Disable_Break_Interrupt(resource->base);
      break;
  }

  return 0;
}

int32_t abort_write(serial_handle handle) {
  UART_Resource *resource = (UART_Resource *)handle;
  int32_t sent_length = DRV_UART_Abort_Send(resource->drv_handle);
  uint32_t ret = -1;

  if (sent_length >= 0) {
    resource->sent_length = sent_length;
    alt_osal_set_taskflag(resource->tx_task_handle, EVT_FLAG_ABORT);
    ret = 0;
  }

  return ret;
}
int32_t abort_read(serial_handle handle) {
  UART_Resource *resource = (UART_Resource *)handle;
  int32_t received_length = DRV_UART_Abort_Receive(resource->drv_handle);
  uint32_t ret = -1;

  if (received_length >= 0) {
    resource->received_length = received_length;
    alt_osal_set_taskflag(resource->rx_task_handle, EVT_FLAG_ABORT);
    ret = 0;
  }

  return ret;
}

void dump_statistics(eUartInstance ins)
{
  UART_Resource *resource = Get_Resource(ins);
  DRV_UART_Statistic stat;

  if(resource == NULL)
  {
    printf("Wrong parameter\n");
    return;
  }

  DRV_UART_Get_Statistic(resource->drv_handle, &stat);
  printf("Tx cnt: %d\n", stat.tx_cnt);
  printf("Tx cnt isr: %d\n", stat.tx_cnt_isr);
  printf("Rx cnt: %d\n", stat.rx_cnt);
  printf("Rx cnt isr: %d\n", stat.rx_cnt_isr);
  printf("Break Error: %d\n", stat.be_cnt);
  printf("Frame Error: %d\n", stat.fe_cnt);
  printf("Parity Error: %d\n", stat.pe_cnt);
  printf("Overrun Error: %d\n", stat.oe_cnt);
}
