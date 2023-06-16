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

/*****************************************************************************
 * Includes
 *****************************************************************************/
#include <string.h>
#include DEVICE_HEADER

/* CMSIS-Driver Headers */
#include "Driver_USART.h"

/* Low-Level Driver Headers */
#include "DRV_UART.h"
#include "DRV_COMMON.h"

#include "DRV_IF_CFG.h"

/*****************************************************************************
 * Pre-processor Definitions
 *****************************************************************************/
#define ARM_UART_DRV_VERSION ARM_DRIVER_VERSION_MAJOR_MINOR(1, 0) /* driver version */

#define DECLARE_UART_ADL_INSTANCE_FUNC_WRAPPER(n, inst)                                       \
  static ARM_DRIVER_VERSION UART_ADL_##n##_GetVersion(void) { return UART_ADL_GetVersion(); } \
  static ARM_USART_CAPABILITIES UART_ADL_##n##_GetCapabilities(void) {                        \
    return UART_ADL_GetCapabilities();                                                        \
  }                                                                                           \
  static int32_t UART_ADL_##n##_Initialize(ARM_USART_SignalEvent_t cb_event) {                \
    return UART_ADL_Initialize(inst, cb_event);                                               \
  }                                                                                           \
  static int32_t UART_ADL_##n##_Uninitialize(void) { return UART_ADL_Uninitialize(inst); }    \
  static int32_t UART_ADL_##n##_PowerControl(ARM_POWER_STATE state) {                         \
    return UART_ADL_PowerControl(inst, state);                                                \
  }                                                                                           \
  static int32_t UART_ADL_##n##_Send(const void *data, uint32_t num) {                        \
    return UART_ADL_Send(inst, data, num);                                                    \
  }                                                                                           \
  static int32_t UART_ADL_##n##_Receive(void *data, uint32_t num) {                           \
    return UART_ADL_Receive(inst, data, num);                                                 \
  }                                                                                           \
  static int32_t UART_ADL_##n##_Transfer(const void *data_out, void *data_in, uint32_t num) { \
    return UART_ADL_Transfer(inst, data_out, data_in, num);                                   \
  }                                                                                           \
  static uint32_t UART_ADL_##n##GetTxCount(void) { return UART_ADL_GetTxCount(inst); }        \
  static uint32_t UART_ADL_##n##GetRxCount(void) { return UART_ADL_GetRxCount(inst); }        \
  static int32_t UART_ADL_##n##_Control(uint32_t control, uint32_t arg) {                     \
    return UART_ADL_Control(inst, control, arg);                                              \
  }                                                                                           \
  static ARM_USART_STATUS UART_ADL_##n##_GetStatus(void) { return UART_ADL_GetStatus(inst); } \
  static int32_t UART_ADL_##n##_SetModemControl(ARM_USART_MODEM_CONTROL control) {            \
    return UART_ADL_SetModemControl(inst, control);                                           \
  }                                                                                           \
  static ARM_USART_MODEM_STATUS UART_ADL_##n##_GetModemStatus(void) {                         \
    return UART_ADL_GetModemStatus(inst);                                                     \
  }

#define DECLARE_UART_ADL_INSTANCE_FUNC(n) \
  DECLARE_UART_ADL_INSTANCE_FUNC_WRAPPER(n, &uart##n##_resource)

#define DECLARE_UART_ADL_HOOK_WRAPPER(n)                                                  \
  {                                                                                       \
    UART_ADL_##n##_GetVersion, UART_ADL_##n##_GetCapabilities, UART_ADL_##n##_Initialize, \
        UART_ADL_##n##_Uninitialize, UART_ADL_##n##_PowerControl, UART_ADL_##n##_Send,    \
        UART_ADL_##n##_Receive, UART_ADL_##n##_Transfer, UART_ADL_##n##GetTxCount,        \
        UART_ADL_##n##GetRxCount, UART_ADL_##n##_Control, UART_ADL_##n##_GetStatus,       \
        UART_ADL_##n##_SetModemControl, UART_ADL_##n##_GetModemStatus,                    \
  }

#define DECLARE_UART_ADL_HOOK(n) DECLARE_UART_ADL_HOOK_WRAPPER(n)

/* Assertion Checking */
#define CMSIS_UART_PAR_CHECK(x)        \
  if ((x) == 0) {                      \
    return ARM_DRIVER_ERROR_PARAMETER; \
  } /**< Assertion check */

extern uint32_t SystemCoreClock;
static uint32_t Get_System_Clock(void);
static uint32_t Get_Ref_Clock(void);

/* Current driver status flag definition */
#define UART_INITIALIZED (1 << 0)    // UART initialized
#define UART_UNINITIALIZED (1 << 1)  // UART uninitialized
#define UART_POWERED (1 << 2)        // UART powered on
#define UART_CONFIGURED (1 << 3)     // UART configured
#define UART_DATA_LOST (1 << 4)      // UART data lost occurred
#define UART_MODE_FAULT (1 << 5)     // UART mode fault occurred
#define RINGBUFFER_SIZE (64)

#define ARM_UART_API_VERSION ARM_DRIVER_VERSION_MAJOR_MINOR(2, 4) /* API version */
/****************************************************************************
 * Private Data Types
 ****************************************************************************/
static DRV_UART_Handle handle_uartf0;
static DRV_UART_Handle handle_uarti0;
#ifdef ALT1250
static DRV_UART_Handle handle_uartf1;
#endif

/* UART Resource Information */
typedef struct {
  UART_Type *base;
  UARTF_CFG_Type *cfg_base;
  IRQn_Type irq_base;
  Interface_Id interface_id;
  uint32_t (*get_clockrate)(void);
  DRV_UART_Handle *handle;
  ARM_USART_SignalEvent_t callback;
  int8_t *ringbuffer;
  size_t ringbuffer_size;
  uint32_t flag;
} UART_Resource;

/*****************************************************************************
 * Private Data
 *****************************************************************************/
/* Driver Version */
static const ARM_DRIVER_VERSION DriverVersion = {ARM_UART_API_VERSION, ARM_UART_DRV_VERSION};

/* Driver Capabilities */
static const ARM_USART_CAPABILITIES DriverCapabilities = {
    1, /* supports LPUART (Asynchronous) mode */
    0, /* supports Synchronous Master mode */
    0, /* supports Synchronous Slave mode */
    0, /* supports LPUART Single-wire mode */
    0, /* supports LPUART IrDA mode */
    0, /* supports LPUART Smart Card mode */
    0, /* Smart Card Clock generator */
    1, /* RTS Flow Control available */
    1, /* CTS Flow Control available */
    1, /* Transmit completed event: \ref ARM_USART_EVENT_TX_COMPLETE */
    0, /* Signal receive character timeout event: \ref ARM_USART_EVENT_RX_TIMEOUT */
    1, /* RTS Line: 0=not available, 1=available */
    1, /* CTS Line: 0=not available, 1=available */
    0, /* DTR Line: 0=not available, 1=available */
    0, /* DSR Line: 0=not available, 1=available */
    0, /* DCD Line: 0=not available, 1=available */
    0, /* RI Line: 0=not available, 1=available */
    0, /* Signal CTS change event: \ref ARM_USART_EVENT_CTS */
    0, /* Signal DSR change event: \ref ARM_USART_EVENT_DSR */
    0, /* Signal DCD change event: \ref ARM_USART_EVENT_DCD */
    0, /* Signal RI change event: \ref ARM_USART_EVENT_RI */
    0, /* Reserved */
};

int8_t uartf0_ringbuffer[RINGBUFFER_SIZE];
int8_t uarti0_ringbuffer[RINGBUFFER_SIZE];
static UART_Resource uartf0_resource = {
    .base = UARTF0,
    .cfg_base = UARTF0_CFG,
    .irq_base = UARTF0_IRQn,
    .get_clockrate = Get_System_Clock,
    .handle = &handle_uartf0,
    .interface_id = IF_CFG_UARTF0,
    .ringbuffer = uartf0_ringbuffer,
    .ringbuffer_size = RINGBUFFER_SIZE,
    .flag = 0,
};
static UART_Resource uarti0_resource = {
    .base = UARTI0,
    .cfg_base = NULL,
    .irq_base = UARTI0_IRQn,
    .get_clockrate = Get_Ref_Clock,
    .handle = &handle_uarti0,
    .interface_id = IF_CFG_LAST_IF,
    .ringbuffer = uarti0_ringbuffer,
    .ringbuffer_size = RINGBUFFER_SIZE,
    .flag = 0,
};

#ifdef ALT1250
static int8_t uartf1_ringbuffer[RINGBUFFER_SIZE];
static UART_Resource uartf1_resource = {
    .base = UARTF1,
    .cfg_base = UARTF1_CFG,
    .irq_base = UARTF1_IRQn,
    .get_clockrate = Get_System_Clock,
    .handle = &handle_uartf1,
    .interface_id = IF_CFG_UARTF1,
    .ringbuffer = uartf1_ringbuffer,
    .ringbuffer_size = RINGBUFFER_SIZE,
    .flag = 0,
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

/*****************************************************************************
 * Private function prototypes
 *****************************************************************************/
/* interrupts declaration */
/* Prototypes- UART callback */
// void UART_ADL_EventCallback(DRV_UART_Handle *handle, uint32_t status, void *user_data);

/*****************************************************************************
 * Private Functions
 *****************************************************************************/

static uint32_t Get_System_Clock(void) { return SystemCoreClock; }

static uint32_t Get_Ref_Clock(void) { return REF_CLK; }

void UART_ADL_EventCallback(DRV_UART_Handle *handle, uint32_t status, void *user_data) {
  uint32_t event = 0;

  switch (status) {
    case CB_TX_COMPLETE:
      event |= ARM_USART_EVENT_SEND_COMPLETE;
      break;

    case CB_RX_COMPLETE:
      event |= ARM_USART_EVENT_RECEIVE_COMPLETE;
      break;

    default:
      break;
  }

  if (event && user_data) {
    ((ARM_USART_SignalEvent_t)user_data)(event);
  }
}
/****************************************************************************
 * Driver Interface Functions
 ****************************************************************************/
static ARM_DRIVER_VERSION UART_ADL_GetVersion(void) { return DriverVersion; }

static ARM_USART_CAPABILITIES UART_ADL_GetCapabilities(void) { return DriverCapabilities; }

static int32_t UART_ADL_Initialize(UART_Resource *resource, ARM_USART_SignalEvent_t cb_event) {
  CMSIS_UART_PAR_CHECK(resource);

  resource->callback = cb_event;
  resource->flag = UART_INITIALIZED;

  if (DRV_IF_CFG_SetIO(resource->interface_id) != DRV_IF_CFG_OK) {
    return ARM_DRIVER_ERROR;
  }

  return ARM_DRIVER_OK;
}

static int32_t UART_ADL_Uninitialize(UART_Resource *resource) {
  CMSIS_UART_PAR_CHECK(resource);

  resource->flag = UART_UNINITIALIZED;
  return ARM_DRIVER_OK;
}

static int32_t UART_ADL_PowerControl(UART_Resource *resource, ARM_POWER_STATE state) {
  CMSIS_UART_PAR_CHECK(resource);

  DRV_UART_Config config;

  switch (state) {
    case ARM_POWER_OFF:
      if (resource->flag & UART_POWERED) {
        NVIC_DisableIRQ(resource->irq_base);
        DRV_UART_Uninitialize(resource->handle);
        resource->flag = UART_INITIALIZED;
      }
      break;
    case ARM_POWER_LOW:
      return ARM_DRIVER_ERROR_UNSUPPORTED;
    case ARM_POWER_FULL:
      /* Must be initialized first. */
      if (resource->flag != UART_INITIALIZED) {
        return ARM_DRIVER_ERROR;
      }

      if (resource->flag & UART_POWERED) {
        /* Driver already powered */
        break;
      }

      if (resource->base != UARTI0) {
        DRV_IF_CFG_GetDefConfig(resource->interface_id, &config);
      } else {
        config.BaudRate = DRV_UART_BAUD_460800;
        config.HwFlowCtl = DRV_UART_HWCONTROL_RTS_CTS;
        config.Parity = DRV_UART_PARITY_MODE_DISABLE;
        config.StopBits = DRV_UART_STOP_BITS_1;
        config.WordLength = DRV_UART_WORD_LENGTH_8B;
      }

      if (resource->cfg_base) {
        resource->cfg_base->EN_FLOW_INT_b.UARTINT = 1;  // enable interrupt
      }

      DRV_UART_Initialize(resource->base, &config, resource->get_clockrate());
      DRV_UART_Create_Handle(resource->base, resource->handle, UART_ADL_EventCallback,
                             (void *)resource->callback, resource->ringbuffer,
                             resource->ringbuffer_size);

      resource->flag |= (UART_POWERED | UART_CONFIGURED);

      NVIC_SetPriority(resource->irq_base, 7); /* set Interrupt priority */
      NVIC_EnableIRQ(resource->irq_base);

      break;
    default:
      return ARM_DRIVER_ERROR_UNSUPPORTED;
  }
  return ARM_DRIVER_OK;
}

static int32_t UART_ADL_Send(UART_Resource *resource, const void *data, uint32_t num) {
  CMSIS_UART_PAR_CHECK(resource);
  CMSIS_UART_PAR_CHECK(data);

  DRV_UART_Status status;
  int32_t ret = 0;

  status = DRV_UART_Send_Nonblock(resource->handle, (uint8_t *)data, num);

  switch (status) {
    case DRV_UART_ERROR_BUSY:
      ret = ARM_DRIVER_ERROR_BUSY;
      break;

    case DRV_UART_ERROR_PARAMETER:
      ret = ARM_DRIVER_ERROR_PARAMETER;
      break;

    case DRV_UART_ERROR_NONE:
      UART_ADL_EventCallback(resource->handle, CB_TX_COMPLETE, resource->callback);
      ret = ARM_DRIVER_OK;
      break;

    case DRV_UART_WAIT_CB:
      ret = ARM_DRIVER_OK;
      break;

    default:
      ret = ARM_DRIVER_ERROR;
  }
  return ret;
}

static int32_t UART_ADL_Receive(UART_Resource *resource, void *data, uint32_t num) {
  CMSIS_UART_PAR_CHECK(resource);
  CMSIS_UART_PAR_CHECK(data);

  DRV_UART_Status status;
  int32_t ret = 0;

  status = DRV_UART_Receive_Nonblock(resource->handle, (uint8_t *)data, num);

  switch (status) {
    case DRV_UART_ERROR_BUSY:
      ret = ARM_DRIVER_ERROR_BUSY;
      break;

    case DRV_UART_ERROR_PARAMETER:
      ret = ARM_DRIVER_ERROR_PARAMETER;
      break;

    case DRV_UART_ERROR_NONE:
      UART_ADL_EventCallback(resource->handle, CB_RX_COMPLETE, resource->callback);
      ret = ARM_DRIVER_OK;
      break;
    case DRV_UART_WAIT_CB:
      ret = ARM_DRIVER_OK;
      break;
    default:
      ret = ARM_DRIVER_ERROR;
  }
  return ret;
}

static int32_t UART_ADL_Transfer(UART_Resource *resource, const void *data_out, void *data_in,
                                 uint32_t num) {
  return ARM_DRIVER_ERROR_UNSUPPORTED;
}

static uint32_t UART_ADL_GetTxCount(UART_Resource *resource) {
  CMSIS_UART_PAR_CHECK(resource);

  return DRV_UART_Get_Transmitted_Tx_Count(resource->handle);
}

static uint32_t UART_ADL_GetRxCount(UART_Resource *resource) {
  CMSIS_UART_PAR_CHECK(resource);

  return DRV_UART_Get_Received_Rx_Count(resource->handle);
}

static int32_t UART_ADL_Control(UART_Resource *resource, uint32_t control, uint32_t arg) {
  CMSIS_UART_PAR_CHECK(resource);

  DRV_UART_Config config;

  if (!(resource->flag & UART_POWERED)) {
    return ARM_DRIVER_ERROR;
  }

  if (control & (ARM_USART_CPOL_Msk | ARM_USART_CPHA_Msk)) {
    return ARM_DRIVER_ERROR_UNSUPPORTED;
  }

  if (control == ARM_USART_ABORT_TRANSFER || control == ARM_USART_MODE_SYNCHRONOUS_MASTER ||
      control == ARM_USART_MODE_SYNCHRONOUS_SLAVE || control == ARM_USART_MODE_SINGLE_WIRE ||
      control == ARM_USART_MODE_IRDA || control == ARM_USART_MODE_SMART_CARD) {
    return ARM_DRIVER_ERROR_UNSUPPORTED;
  }

  switch (control & ARM_USART_CONTROL_Msk) {
    case ARM_USART_ABORT_SEND:
      return ARM_DRIVER_OK;

    case ARM_USART_ABORT_RECEIVE:
      return ARM_DRIVER_OK;

    case ARM_USART_CONTROL_TX:
      if (arg) {
        DRV_UART_Enable_Tx(resource->base);
      } else {
        DRV_UART_Disable_Tx(resource->base);
      }
      return ARM_DRIVER_OK;
    case ARM_USART_CONTROL_RX:
      if (arg) {
        DRV_UART_Enable_Rx(resource->base);
      } else {
        DRV_UART_Disable_Rx(resource->base);
      }
      return ARM_DRIVER_OK;
  }
  if (resource->base != UARTI0) {
    DRV_IF_CFG_GetDefConfig(resource->interface_id, &config);
  } else {
    config.BaudRate = DRV_UART_BAUD_460800;
    config.HwFlowCtl = DRV_UART_HWCONTROL_RTS_CTS;
    config.Parity = DRV_UART_PARITY_MODE_DISABLE;
    config.StopBits = DRV_UART_STOP_BITS_1;
    config.WordLength = DRV_UART_WORD_LENGTH_8B;
  }

  switch (control & ARM_USART_DATA_BITS_Msk) {
    case ARM_USART_DATA_BITS_5:
      config.WordLength = DRV_UART_WORD_LENGTH_5B;
      break;
    case ARM_USART_DATA_BITS_6:
      config.WordLength = DRV_UART_WORD_LENGTH_6B;
      break;
    case ARM_USART_DATA_BITS_7:
      config.WordLength = DRV_UART_WORD_LENGTH_7B;
      break;
    case ARM_USART_DATA_BITS_8:
      config.WordLength = DRV_UART_WORD_LENGTH_8B;
      break;
    default:
      return ARM_USART_ERROR_DATA_BITS;
  }

  switch (control & ARM_USART_PARITY_Msk) {
    case ARM_USART_PARITY_NONE:
      config.Parity = DRV_UART_PARITY_MODE_DISABLE;
      break;
    case ARM_USART_PARITY_EVEN:
      config.Parity = DRV_UART_PARITY_MODE_EVEN;
      break;
    case ARM_USART_PARITY_ODD:
      config.Parity = DRV_UART_PARITY_MODE_ODD;
      break;
    default:
      return ARM_USART_ERROR_PARITY;
  }

  switch (control & ARM_USART_STOP_BITS_Msk) {
    case ARM_USART_STOP_BITS_1:
      config.StopBits = DRV_UART_STOP_BITS_1;
      break;
    case ARM_USART_STOP_BITS_2:
      config.StopBits = DRV_UART_STOP_BITS_2;
      break;
    default:
      return ARM_USART_ERROR_STOP_BITS;
  }

  switch (control & ARM_USART_FLOW_CONTROL_Msk) {
    case ARM_USART_FLOW_CONTROL_NONE:
      config.HwFlowCtl = DRV_UART_HWCONTROL_NONE;
      break;
    case ARM_USART_FLOW_CONTROL_RTS:
      config.HwFlowCtl = DRV_UART_HWCONTROL_RTS;
      break;
    case ARM_USART_FLOW_CONTROL_CTS:
      config.HwFlowCtl = DRV_UART_HWCONTROL_CTS;
      break;
    case ARM_USART_FLOW_CONTROL_RTS_CTS:
      config.HwFlowCtl = DRV_UART_HWCONTROL_RTS_CTS;
      break;
    default:
      return ARM_USART_ERROR_FLOW_CONTROL;
  }
  DRV_UART_Update_Setting(resource->base, &config, resource->get_clockrate());

  return ARM_DRIVER_OK;
}

static ARM_USART_STATUS UART_ADL_GetStatus(UART_Resource *resource) {
  ARM_USART_STATUS stat = {0};

  if (!resource) return stat;

  stat.rx_busy = resource->handle->rx_line_state == DRV_UART_LINE_STATE_BUSY ? 1 : 0;
  stat.tx_busy = resource->handle->tx_line_state == DRV_UART_LINE_STATE_BUSY ? 1 : 0;
  stat.rx_break = 0;
  stat.rx_framing_error = 0;
  stat.rx_overflow = 0;
  stat.rx_parity_error = 0;
  stat.tx_busy = 0;
  stat.tx_underflow = 0;
  stat.reserved = 0;

  return stat;
}

static int32_t UART_ADL_SetModemControl(UART_Resource *resource, ARM_USART_MODEM_CONTROL control) {
  CMSIS_UART_PAR_CHECK(resource);

  switch (control) {
    case ARM_USART_RTS_CLEAR:
      DRV_UART_Deactive_RTS(resource->base);
      break;
    case ARM_USART_RTS_SET:
      DRV_UART_Active_RTS(resource->base);
      break;
    case ARM_USART_DTR_CLEAR:
      DRV_UART_Deactive_DTR(resource->base);
      break;
    case ARM_USART_DTR_SET:
      DRV_UART_Active_DTR(resource->base);
      break;
  }
  return ARM_DRIVER_OK;
}

static ARM_USART_MODEM_STATUS UART_ADL_GetModemStatus(UART_Resource *resource) {
  ARM_USART_MODEM_STATUS stat = {0};
  if (!resource) return stat;

  stat.cts = DRV_UART_Get_CTS(resource->base);
  stat.dcd = DRV_UART_Get_DCD(resource->base);
  stat.dsr = DRV_UART_Get_DSR(resource->base);
  stat.ri = DRV_UART_Get_RI(resource->base);

  return stat;
}

/*****************************************************************************
 * UART Interface Hook unctions
 *****************************************************************************/

#ifdef ALT1250
/* Master UART0, UART1; Slave UART2 */
DECLARE_UART_ADL_INSTANCE_FUNC(f0)
ARM_DRIVER_USART Driver_UARTF0 = DECLARE_UART_ADL_HOOK(f0);
DECLARE_UART_ADL_INSTANCE_FUNC(f1)
ARM_DRIVER_USART Driver_UARTF1 = DECLARE_UART_ADL_HOOK(f1);
DECLARE_UART_ADL_INSTANCE_FUNC(i0)
ARM_DRIVER_USART Driver_UARTI0 = DECLARE_UART_ADL_HOOK(i0);
#elif defined(ALT1255)
/* Master UART0; Slave UART2 */
DECLARE_UART_ADL_INSTANCE_FUNC(f0)
ARM_DRIVER_USART Driver_UARTF0 = DECLARE_UART_ADL_HOOK(f0);
DECLARE_UART_ADL_INSTANCE_FUNC(i0)
ARM_DRIVER_USART Driver_UARTI0 = DECLARE_UART_ADL_HOOK(i0);
#else
#error No target platform specified!!
#endif
