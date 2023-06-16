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

/* CMSIS-Driver Headers */
#include "Driver_I2C.h"

/* Low-Level Driver Headers */
#include "DRV_I2C.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ARM_I2C_DRV_VERSION ARM_DRIVER_VERSION_MAJOR_MINOR(1, 0) /* driver version */

#define DECLARE_I2C_ADL_INSTANCE_FUNC_WRAPPER(n, inst)                                          \
  static ARM_DRIVER_VERSION I2C_ADL_##n##_GetVersion(void) { return I2C_ADL_GetVersion(inst); } \
  static ARM_I2C_CAPABILITIES I2C_ADL_##n##_GetCapabilities(void) {                             \
    return I2C_ADL_GetCapabilities(inst);                                                       \
  }                                                                                             \
  static int32_t I2C_ADL_##n##_Initialize(ARM_I2C_SignalEvent_t cb_event) {                     \
    return I2C_ADL_Initialize(inst, cb_event);                                                  \
  }                                                                                             \
  static int32_t I2C_ADL_##n##_Uninitialize(void) { return I2C_ADL_Uninitialize(inst); }        \
  static int32_t I2C_ADL_##n##_PowerControl(ARM_POWER_STATE state) {                            \
    return I2C_ADL_PowerControl(inst, state);                                                   \
  }                                                                                             \
  static int32_t I2C_ADL_##n##_MasterTransmit(uint32_t addr, const uint8_t *data, uint32_t num, \
                                              bool xfer_pending) {                              \
    return I2C_ADL_MasterTransmit(inst, addr, data, num, xfer_pending);                         \
  }                                                                                             \
  static int32_t I2C_ADL_##n##_MasterReceive(uint32_t addr, uint8_t *data, uint32_t num,        \
                                             bool xfer_pending) {                               \
    return I2C_ADL_MasterReceive(inst, addr, data, num, xfer_pending);                          \
  }                                                                                             \
  static int32_t I2C_ADL_##n##_SlaveTransmit(const uint8_t *data, uint32_t num) {               \
    return I2C_ADL_SlaveTransmit(inst, data, num);                                              \
  }                                                                                             \
  static int32_t I2C_ADL_##n##_SlaveReceive(uint8_t *data, uint32_t num) {                      \
    return I2C_ADL_SlaveReceive(inst, data, num);                                               \
  }                                                                                             \
  static int32_t I2C_ADL_##n##_GetDataCount(void) { return I2C_ADL_GetDataCount(inst); }        \
  static int32_t I2C_ADL_##n##_Control(uint32_t control, uint32_t arg) {                        \
    return I2C_ADL_Control(inst, control, arg);                                                 \
  }                                                                                             \
  static ARM_I2C_STATUS I2C_ADL_##n##_GetStatus(void) { return I2C_ADL_GetStatus(inst); }

#define DECLARE_I2C_ADL_INSTANCE_FUNC(n) DECLARE_I2C_ADL_INSTANCE_FUNC_WRAPPER(n, I2C_BUS_##n)

#define DECLARE_I2C_ADL_HOOK_WRAPPER(n)                                                       \
  {                                                                                           \
    I2C_ADL_##n##_GetVersion, I2C_ADL_##n##_GetCapabilities, I2C_ADL_##n##_Initialize,        \
        I2C_ADL_##n##_Uninitialize, I2C_ADL_##n##_PowerControl, I2C_ADL_##n##_MasterTransmit, \
        I2C_ADL_##n##_MasterReceive, I2C_ADL_##n##_SlaveTransmit, I2C_ADL_##n##_SlaveReceive, \
        I2C_ADL_##n##_GetDataCount, I2C_ADL_##n##_Control, I2C_ADL_##n##_GetStatus            \
  }

#define DECLARE_I2C_ADL_HOOK(n) DECLARE_I2C_ADL_HOOK_WRAPPER(n)

/* I2C Driver Operation flags */
#define I2C_ADL_FLAG_INIT (1 << 0)  /**< Driver initialized */
#define I2C_ADL_FLAG_POWER (1 << 1) /**< Driver power on */
#define I2C_ADL_FLAG_SETUP (1 << 2) /**< Master configured, clock set */

/* Assertion Checking */
#define I2C_ADL_ASSERT(x) \
  if ((x) == 0) {         \
    for (;;)              \
      ;                   \
  }

/****************************************************************************
 * Private Data Types
 ****************************************************************************/

/* I2C Control Information */
typedef struct {
  ARM_I2C_SignalEvent_t cb_event; /**< Event callback */
  ARM_I2C_STATUS status;          /**< Status flags */
  uint8_t flags;                  /**< Operation flags */
} I2C_ADL_CtrlInfo;

/* I2C Resource Information */
typedef struct {
  Drv_I2C_Peripheral *peripheral;
  I2C_ADL_CtrlInfo ctrl_info;
} I2C_ADL_Resource;

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Driver Version */
static const ARM_DRIVER_VERSION driver_version = {ARM_I2C_API_VERSION, ARM_I2C_DRV_VERSION};

/* Driver Capabilities */
static const ARM_I2C_CAPABILITIES driver_capabilities = {
    .address_10_bit = 1, /* supports 10-bit addressing */
    .reserved = 0};

static I2C_ADL_Resource i2c_resource0 = {0};
#ifdef ALT1250
static I2C_ADL_Resource i2c_resource1 = {0};
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static I2C_ADL_Resource *I2C_ADL_GetResource(I2C_BusId bus_id) {
  I2C_ADL_Resource *i2c = NULL;

  switch (bus_id) {
    case I2C_BUS_0:
      i2c = &i2c_resource0;
      break;

#ifdef ALT1250
    case I2C_BUS_1:
      i2c = &i2c_resource1;
      break;

#else
    default:
      i2c = NULL;
      break;
#endif
  }

  return i2c;
}

/****************************************************************************
 * Adaptation Layer Functions
 ****************************************************************************/

static ARM_DRIVER_VERSION I2C_ADL_GetVersion(I2C_BusId bus_id) { return driver_version; }

static ARM_I2C_CAPABILITIES I2C_ADL_GetCapabilities(I2C_BusId bus_id) {
  return driver_capabilities;
}

static void I2C_ADL_EventCallback(void *user_data, uint32_t drvEvent) {
  I2C_ADL_Resource *i2c;

  i2c = I2C_ADL_GetResource((I2C_BusId)user_data);
  if (!i2c) {
    return;
  }

  uint32_t event;

  event = 0;
  if (drvEvent & I2C_EVENT_TRANSACTION_DONE) {
    event |= ARM_I2C_EVENT_TRANSFER_DONE;
  }

  if (drvEvent & I2C_EVENT_TRANSACTION_INCOMPLETE) {
    event |= ARM_I2C_EVENT_TRANSFER_INCOMPLETE | ARM_I2C_EVENT_TRANSFER_DONE;
  }

  if (drvEvent & I2C_EVENT_TRANSACTION_ADDRNACK) {
    event |= ARM_I2C_EVENT_ADDRESS_NACK | ARM_I2C_EVENT_TRANSFER_DONE;
  }

  if (drvEvent & I2C_EVENT_TRANSACTION_BUS_CLEAR) {
    event |= ARM_I2C_EVENT_BUS_CLEAR;
  }

  if (event & ARM_I2C_EVENT_TRANSFER_DONE) {
    i2c->ctrl_info.status.busy = 0;
    i2c->ctrl_info.status.direction = 0;
  }

  if (drvEvent & I2C_EVENT_TRANSACTION_DIRCHG) {
    i2c->ctrl_info.status.direction = 1;
  }

  if (event && i2c->ctrl_info.cb_event) {
    i2c->ctrl_info.cb_event(event);
  }
}

static int32_t I2C_ADL_Initialize(I2C_BusId bus_id, ARM_I2C_SignalEvent_t cb_event) {
  I2C_ADL_Resource *i2c;

  i2c = I2C_ADL_GetResource(bus_id);
  if (!i2c) {
    printf("Unsupported bus ID %d\n", (int)bus_id);
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  if (i2c->ctrl_info.flags & I2C_ADL_FLAG_INIT) {
    return ARM_DRIVER_OK;
  }

  Drv_I2C_EventCallback evt_cb;

  evt_cb.user_data = (void *)bus_id;
  evt_cb.event_callback = I2C_ADL_EventCallback;

  if (DRV_I2C_Initialize(bus_id, &evt_cb, &i2c->peripheral) != DRV_I2C_OK) {
    return ARM_DRIVER_ERROR;
  }

  /* Reset Run-Time information structure */
  memset((void *)&i2c->ctrl_info, 0x0, sizeof(I2C_ADL_CtrlInfo));
  i2c->ctrl_info.status.mode = 1U;

  /* Hook callback */
  i2c->ctrl_info.cb_event = cb_event;

  /* Set initialized flag */
  i2c->ctrl_info.flags = I2C_ADL_FLAG_INIT;
  return ARM_DRIVER_OK;
}

static int32_t I2C_ADL_Uninitialize(I2C_BusId bus_id) {
  I2C_ADL_Resource *i2c;

  i2c = I2C_ADL_GetResource(bus_id);
  if (!i2c) {
    printf("Unsupported bus ID %d\n", (int)bus_id);
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  DRV_I2C_Status ret;

  ret = DRV_I2C_Uninitialize(bus_id);
  if (ret != DRV_I2C_OK) {
    return -ret;
  }

  /* Clear initialized flag */
  i2c->ctrl_info.flags = 0;

  /* Un-hook callback */
  i2c->ctrl_info.cb_event = NULL;

  /* Same operation as power off */
  return ARM_DRIVER_OK;
}

static int32_t I2C_ADL_PowerControl(I2C_BusId bus_id, ARM_POWER_STATE state) {
  I2C_ADL_Resource *i2c;

  i2c = I2C_ADL_GetResource(bus_id);
  if (!i2c) {
    printf("Unsupported bus ID %d\n", (int)bus_id);
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  DRV_I2C_Status ret;

  switch (state) {
    case ARM_POWER_OFF:
      if ((i2c->ctrl_info.flags & I2C_ADL_FLAG_INIT) == 0U) {
        return ARM_DRIVER_ERROR;
      }

      /* Update status */
      i2c->ctrl_info.status.busy = 0U;
      i2c->ctrl_info.status.mode = 1U;
      i2c->ctrl_info.status.direction = 0U;
      i2c->ctrl_info.status.general_call = 0U;
      i2c->ctrl_info.status.arbitration_lost = 0U;
      i2c->ctrl_info.status.bus_error = 0U;
      i2c->ctrl_info.flags &= ~I2C_ADL_FLAG_POWER;

      ret = DRV_I2C_PowerControl(i2c->peripheral, I2C_POWER_OFF);
      if (ret != DRV_I2C_OK) {
        return -ret;
      }

      break;

    case ARM_POWER_FULL:
      if ((i2c->ctrl_info.flags & I2C_ADL_FLAG_INIT) == 0U) {
        return ARM_DRIVER_ERROR;
      }

      if ((i2c->ctrl_info.flags & I2C_ADL_FLAG_POWER) != 0U) {
        return ARM_DRIVER_OK;
      }

      /* Update status */
      i2c->ctrl_info.flags |= I2C_ADL_FLAG_POWER;

      ret = DRV_I2C_PowerControl(i2c->peripheral, I2C_POWER_FULL);
      if (ret != DRV_I2C_OK) {
        return -ret;
      }

      break;

    default:
      return ARM_DRIVER_ERROR_UNSUPPORTED;
  }
  return ARM_DRIVER_OK;
}

static int32_t I2C_ADL_MasterTransmit(I2C_BusId bus_id, uint32_t addr, const uint8_t *data,
                                      uint32_t num, bool xfer_pending) {
  if (!data || !num || ((addr & ARM_I2C_ADDRESS_10BIT) && !driver_capabilities.address_10_bit)) {
    /* Invalid parameters */
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  I2C_ADL_Resource *i2c;

  i2c = I2C_ADL_GetResource(bus_id);
  if (!i2c) {
    printf("Unsupported bus ID %d\n", (int)bus_id);
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  if (!(i2c->ctrl_info.flags & I2C_ADL_FLAG_SETUP)) {
    /* Driver not yet configured */
    return ARM_DRIVER_ERROR;
  }

  if (i2c->ctrl_info.status.busy) {
    /* Transfer operation in progress, or Slave stalled */
    return ARM_DRIVER_ERROR_BUSY;
  }

  /* Set control variables */
  i2c->ctrl_info.status.busy = 1;
  i2c->ctrl_info.status.mode = 1;
  i2c->ctrl_info.status.direction = 0;
  i2c->ctrl_info.status.arbitration_lost = 0;
  i2c->ctrl_info.status.bus_error = 0;

  /* Start to transmit */
  DRV_I2C_Status ret;

  ret = DRV_I2C_MasterTransmit_Nonblock(i2c->peripheral, addr, data, num, xfer_pending);
  if (ret == DRV_I2C_OK) {
    return ARM_DRIVER_OK;
  } else {
    return -ret;
  }
}

static int32_t I2C_ADL_MasterReceive(I2C_BusId bus_id, uint32_t addr, uint8_t *data, uint32_t num,
                                     bool xfer_pending) {
  if (!data || !num || ((addr & ARM_I2C_ADDRESS_10BIT) && !driver_capabilities.address_10_bit)) {
    /* Invalid parameters */
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  I2C_ADL_Resource *i2c;

  i2c = I2C_ADL_GetResource(bus_id);
  if (!i2c) {
    printf("Unsupported bus ID %d\n", (int)bus_id);
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  if (!(i2c->ctrl_info.flags & I2C_ADL_FLAG_SETUP)) {
    /* Driver not yet configured */
    return ARM_DRIVER_ERROR;
  }

  if (i2c->ctrl_info.status.busy) {
    /* Transfer operation in progress, or Slave stalled */
    return ARM_DRIVER_ERROR_BUSY;
  }

  /* Set control variables */
  i2c->ctrl_info.status.busy = 1;
  i2c->ctrl_info.status.mode = 1;
  i2c->ctrl_info.status.direction = 0;
  i2c->ctrl_info.status.arbitration_lost = 0;
  i2c->ctrl_info.status.bus_error = 0;

  /* Start to receive */
  DRV_I2C_Status ret;

  ret = DRV_I2C_MasterReceive_Nonblock(i2c->peripheral, addr, data, num, xfer_pending);
  if (ret == DRV_I2C_OK) {
    return ARM_DRIVER_OK;
  } else {
    return -ret;
  }
}

static int32_t I2C_ADL_SlaveTransmit(I2C_BusId bus_id, const uint8_t *data, uint32_t num) {
  return ARM_DRIVER_ERROR_UNSUPPORTED;
}

static int32_t I2C_ADL_SlaveReceive(I2C_BusId bus_id, uint8_t *data, uint32_t num) {
  return ARM_DRIVER_ERROR_UNSUPPORTED;
}

static int32_t I2C_ADL_GetDataCount(I2C_BusId bus_id) {
  I2C_ADL_Resource *i2c;

  i2c = I2C_ADL_GetResource(bus_id);
  if (!i2c) {
    printf("Unsupported bus ID %d\n", (int)bus_id);
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  int32_t cnt;

  cnt = DRV_I2C_GetDataCount(i2c->peripheral);
  return cnt < 0 ? -1 : cnt;
}

static int32_t I2C_ADL_Control(I2C_BusId bus_id, uint32_t control, uint32_t arg) {
  I2C_ADL_Resource *i2c;

  i2c = I2C_ADL_GetResource(bus_id);
  if (!i2c) {
    printf("Unsupported bus ID %d\n", (int)bus_id);
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  if (!(i2c->ctrl_info.flags & I2C_ADL_FLAG_POWER)) {
    /* Driver not powered */
    return ARM_DRIVER_ERROR;
  }

  uint32_t speed;
  DRV_I2C_Status ret;

  switch (control) {
    case ARM_I2C_OWN_ADDRESS:
      return ARM_DRIVER_ERROR_UNSUPPORTED;

    case ARM_I2C_BUS_SPEED:
      switch (arg) {
        case ARM_I2C_BUS_SPEED_STANDARD:
          speed = I2C_BUS_SPEED_STANDARD;
          break;

        case ARM_I2C_BUS_SPEED_FAST:
          speed = I2C_BUS_SPEED_FAST;
          break;

        case ARM_I2C_BUS_SPEED_FAST_PLUS:
          speed = I2C_BUS_SPEED_FAST_PLUS;
          break;

        case ARM_I2C_BUS_SPEED_HIGH:
          speed = I2C_BUS_SPEED_HIGH;
          break;

        default:
          return ARM_DRIVER_ERROR_UNSUPPORTED;
      }

      ret = DRV_I2C_SetSpeed(i2c->peripheral, speed);
      if (ret != DRV_I2C_OK) {
        return -ret;
      }

      /* Speed configured, I2C Master active */
      i2c->ctrl_info.flags |= I2C_ADL_FLAG_SETUP;
      break;

    case ARM_I2C_BUS_CLEAR:
      ret = DRV_I2C_ClearBus(i2c->peripheral);
      if (ret != DRV_I2C_OK) {
        return -ret;
      }

      break;

    case ARM_I2C_ABORT_TRANSFER:
      if (i2c->ctrl_info.status.busy) {
        ret = DRV_I2C_AbortTransfer(i2c->peripheral);
        if (ret != DRV_I2C_OK) {
          return -ret;
        }
      }

      break;

    default:
      return ARM_DRIVER_ERROR_UNSUPPORTED;
  }

  return ARM_DRIVER_OK;
}

static ARM_I2C_STATUS I2C_ADL_GetStatus(I2C_BusId bus_id) {
  I2C_ADL_Resource *i2c;

  i2c = I2C_ADL_GetResource(bus_id);
  I2C_ADL_ASSERT(i2c);

  return i2c->ctrl_info.status;
}

/****************************************************************************
 * CMSIS-Driver Interface
 ****************************************************************************/

#ifdef ALT1250
DECLARE_I2C_ADL_INSTANCE_FUNC(0)
ARM_DRIVER_I2C Driver_I2C0 = DECLARE_I2C_ADL_HOOK(0);
DECLARE_I2C_ADL_INSTANCE_FUNC(1)
ARM_DRIVER_I2C Driver_I2C1 = DECLARE_I2C_ADL_HOOK(1);
#elif defined(ALT1255)
DECLARE_I2C_ADL_INSTANCE_FUNC(0)
ARM_DRIVER_I2C Driver_I2C0 = DECLARE_I2C_ADL_HOOK(0);
#else
#error No target platform specified!!
#endif
