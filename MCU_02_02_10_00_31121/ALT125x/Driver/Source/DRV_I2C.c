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
/* Standard Headers */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Platform Headers */
#include DEVICE_HEADER
#include "DRV_IF_CFG.h"

/* Driver Specific Headers */
#include "DRV_I2C.h"
#if (configUSE_ALT_SLEEP == 1)
#include "DRV_PM.h"
#include "DRV_SLEEP.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* FIFO Command */
#define I2C_CMD_FIFO_INT (1 << 6)                        /**< Enable interrupt when command done */
#define I2C_START_SEQ (1 << 4)                           /**< START sequence */
#define I2C_RESTART_SEQ ((1 << 4) | (1 << 5))            /**< RESTART sequence */
#define I2C_STOP_SEQ (1 << 3)                            /**< STOP sequence */
#define I2C_TX_SEQ (1 << 2)                              /**< TX sequence */
#define I2C_RX_SEQ (1 << 1)                              /**< RX sequence */
#define I2C_NACK_SEQ (1 << 0)                            /**< NACK sequence */
#define I2C_CMD_DATA_POS I2C0_CTRL_CMD_FIFO_ADDRESS_Pos  /**< Command data position */
#define I2C_CMD_DATA_MASK I2C0_CTRL_CMD_FIFO_ADDRESS_Msk /**< Command data mask */

/* Device ID 7/10-bit  */
#define I2C_10BITS_MASK (0x0300)
#define I2C_10BITS_PREFIX (0x7800) /**< S 1 1 1 1 0 A9 A8 ACK A7 A6 A5 A4 A3 A2 A1 A0 ACK ... */
#define I2C_10BITS_WRITE (0x0)
#define I2C_10BITS_READ (0x100)
#define I2C_7BITS_MASK (0x007F)
#define I2C_7BITS_WRITE (0x0)
#define I2C_7BITS_READ (0x1)

/* Check operation from sla_rw */
#define I2C_CHECK_0WRITE_1READ(x) ((x >> 8) ? (x & 0x0100) : (x & 0x01))

/* Default Clock Speed */
#define I2C_DEFAULT_SPEED_KHZ (100) /**< I2C Default  Speed (100kHz) */

/* CMD_FIFO checking count */
#define I2C_BUSYWAIT_CNT 1000

/* Assertion Checking */
#define I2C_ASSERT(x) \
  if ((x) == 0) {     \
    __BKPT(0);        \
  }

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* I2C Transition State */
typedef enum {
  I2C_TRANS_IDLE,        /**< Idle and wait for operation */
  I2C_TRANS_START,       /**< Start sequence */
  I2C_TRANS_RESTART,     /**< Restart sequence */
  I2C_TRANS_STOP,        /**< Stop sequence */
  I2C_TRANS_WRITE_HADDR, /**< Write slave high address */
  I2C_TRANS_WRITE_LADDR, /**< Write slave low address */
  I2C_TRANS_WRITE_DATA,  /**< Write data */
  I2C_TRANS_READ_DATA,   /**< Read data */
  I2C_TRANS_CLEAR_BUS,   /**< Force clear bus */
  I2C_TRANS_ERROR        /**< Error */
} I2C_TransState;

struct Drv_I2C_Peripheral {
  I2C_CTRL_Type *reg;           /**< Peripheral Register file */
  IRQn_Type irq_num[2];         /**< Error/Common event IRQ */
  uint32_t speed_khz;           /**< Bus speed */
  I2C_PowerMode power_mode;     /**< Power mode */
  I2C_TransState state;         /**< Transaction state */
  uint32_t pend_event;          /**< Pending event */
  uint16_t sla_rw;              /**< Slave address and RW bit */
  bool pending;                 /**< Transfer pending (no STOP) */
  bool aborting;                /**< Transfer aborting */
  int32_t cnt;                  /**< Master transfer count */
  uint8_t *data;                /**< Master data to transfer */
  uint32_t num;                 /**< Number of bytes to transfer */
  Drv_I2C_EventCallback evt_cb; /**< Event callback */
  bool is_init;                 /**< Indicator of driver initialization */
#if (configUSE_ALT_SLEEP == 1)
  bool registered_sleep_notify; /**< Registration indicator of sleep notification */
  int32_t sleep_notify_idx;     /**< Sleep notification index */
#endif
};

/****************************************************************************
 * Private Data
 ****************************************************************************/
static struct Drv_I2C_Peripheral i2c_peripheral0 = {I2C0_CTRL,
                                                    {I2C0_ERR_IRQn, I2C0_IRQn},
                                                    I2C_BUS_SPEED_STANDARD,
                                                    I2C_POWER_OFF,
                                                    I2C_TRANS_IDLE,
                                                    0,
                                                    0,
                                                    false,
                                                    false,
                                                    0,
                                                    NULL,
                                                    0,
                                                    {0},
                                                    false
#if (configUSE_ALT_SLEEP == 1)
                                                    ,
                                                    false,
                                                    -1
#endif
};
#ifdef ALT1250
static struct Drv_I2C_Peripheral i2c_peripheral1 = {I2C1_CTRL,
                                                    {I2C1_ERR_IRQn, I2C1_IRQn},
                                                    I2C_BUS_SPEED_STANDARD,
                                                    I2C_POWER_OFF,
                                                    I2C_TRANS_IDLE,
                                                    0,
                                                    0,
                                                    false,
                                                    false,
                                                    0,
                                                    NULL,
                                                    0,
                                                    {0},
                                                    false
#if (configUSE_ALT_SLEEP == 1)
                                                    ,
                                                    false,
                                                    -1
#endif
};
#endif

/****************************************************************************
 * External Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static Drv_I2C_Peripheral *DRV_I2C_GetPeripheral(I2C_BusId bus_id) {
  struct Drv_I2C_Peripheral *peripheral = NULL;

  switch (bus_id) {
    case I2C_BUS_0:
      peripheral = &i2c_peripheral0;
      break;

#ifdef ALT1250
    case I2C_BUS_1:
      peripheral = &i2c_peripheral1;
      break;

#else
    default:
      peripheral = NULL;
      break;
#endif
  }

  return peripheral;
}

static void DRV_I2C_InitMainCtrl(Drv_I2C_Peripheral *i2c) {
  /* Enabled: basic mode, endian swap; Disabled: write protection */
  i2c->reg->MAIN_CTRL_CFG_b.BASIC_I2C = 1;
  i2c->reg->MAIN_CTRL_CFG_b.ENDIAN_SWAP_EN = 1;
  i2c->reg->MAIN_CTRL_CFG_b.WP = 0;
}

static DRV_I2C_Status DRV_I2C_WriteCmdFifo(Drv_I2C_Peripheral *i2c, uint32_t fifo_cmd,
                                           bool isBlocking) {
  if (!isBlocking) {
    i2c->reg->CMD_FIFO = fifo_cmd | I2C_CMD_FIFO_INT;
    return DRV_I2C_OK;
  } else {
    /* TODO: Implement blocking logic */
    /* Check write/read pointer */
    if (i2c->reg->CMD_FIFO_STATUS_b.CMD_WP != i2c->reg->CMD_FIFO_STATUS_b.CMD_RP) {
      return DRV_I2C_ERROR_BUSY;
    }

    /* Write command */
    i2c->reg->CMD_FIFO = fifo_cmd;

    int32_t wait = I2C_BUSYWAIT_CNT;

    /* Check CMD_FIFO_STATUS */
    while (--wait > 0) {
      if ((i2c->reg->CMD_FIFO_STATUS_b.CMD_WP != i2c->reg->CMD_FIFO_STATUS_b.CMD_RP) ||
          (i2c->reg->CMD_FIFO_STATUS_b.CMD_STATUS != 0)) {
        continue;
      }

      wait = I2C_BUSYWAIT_CNT;
      while (--wait > 0) {
        if (!i2c->reg->CURRENT_STATE_b.I2C_IDLE_STATE) {
          continue;
        }

        return DRV_I2C_OK;
      }
    }

    return DRV_I2C_ERROR_TIMEOUT;
  }
}

static DRV_I2C_Status DRV_I2C_ReadDataFifo(Drv_I2C_Peripheral *i2c, uint32_t *data,
                                           bool isBlocking) {
  if (!isBlocking) {
    *data = i2c->reg->RDATA_FIFO;
    return DRV_I2C_OK;
  } else {
    int32_t wait = I2C_BUSYWAIT_CNT;

    /* Check RDATA_FIFO_STATUS */
    while (--wait > 0) {
      if (i2c->reg->RDATA_FIFO_STATUS_b.RDATA_STATUS == 0) {
        continue;
      }

      *data = i2c->reg->RDATA_FIFO;
      return DRV_I2C_OK;
    }

    return DRV_I2C_ERROR_TIMEOUT;
  }
}

static void DRV_I2C_SetPeripheralIrq(Drv_I2C_Peripheral *i2c, bool enable) {
  i2c->reg->INTERRUPT_CFG_b.M_DONE = enable == true ? 0 : 1;
  i2c->reg->INTERRUPT_CFG_b.M_NACK_ERR = enable == true ? 0 : 1;
}

static void DRV_I2C_ResetPeripheral(Drv_I2C_Peripheral *i2c) { i2c->reg->FLUSH_CFG_b.SM_RESET = 1; }

static void DRV_I2C_ClearFifo(Drv_I2C_Peripheral *i2c) {
  i2c->reg->FLUSH_CFG_b.FLUSH_CMD = 1;
  i2c->reg->FLUSH_CFG_b.FLUSH_WDATA = 1;
  i2c->reg->FLUSH_CFG_b.FLUSH_RDATA = 1;
}

static uint32_t DRV_I2C_DoneEventHandler(Drv_I2C_Peripheral *i2c) {
  uint32_t event = 0;

  if (i2c->aborting) {
    i2c->state = I2C_TRANS_ERROR;
  }

  uint32_t data;
  switch (i2c->state) {
    case I2C_TRANS_START:
      if (i2c->sla_rw >> 8) {
        /* 10-bit address */
        i2c->state = I2C_TRANS_WRITE_HADDR;
        DRV_I2C_WriteCmdFifo(i2c, I2C_TX_SEQ | (i2c->sla_rw & I2C_CMD_DATA_MASK), false);

      } else {
        /* 7-bit address */
        i2c->state = I2C_TRANS_WRITE_LADDR;
        DRV_I2C_WriteCmdFifo(i2c, I2C_TX_SEQ | (i2c->sla_rw << I2C_CMD_DATA_POS), false);
      }

      break;

    case I2C_TRANS_WRITE_HADDR:
      /* Low address of 10-bit */
      i2c->state = I2C_TRANS_WRITE_LADDR;
      DRV_I2C_WriteCmdFifo(i2c, I2C_TX_SEQ | (i2c->sla_rw << I2C_CMD_DATA_POS), false);
      break;

    case I2C_TRANS_WRITE_LADDR:
      i2c->cnt = 0;
      if (I2C_CHECK_0WRITE_1READ(i2c->sla_rw)) {
        /* Master Read */
        i2c->state = I2C_TRANS_READ_DATA;
        /* Receive first byte */
        DRV_I2C_WriteCmdFifo(i2c, I2C_RX_SEQ | (i2c->num != 1 ? 0 : I2C_NACK_SEQ), false);
      } else {
        /* Master Write */
        i2c->state = I2C_TRANS_WRITE_DATA;
        DRV_I2C_WriteCmdFifo(i2c, I2C_TX_SEQ | (i2c->data[i2c->cnt] << I2C_CMD_DATA_POS), false);
      }

      break;

    case I2C_TRANS_WRITE_DATA:
      i2c->cnt++;
      i2c->num--;
      if (!i2c->num) {
        if (!i2c->pending) {
          i2c->state = I2C_TRANS_STOP;
          DRV_I2C_WriteCmdFifo(i2c, I2C_STOP_SEQ, false);
        } else {
          i2c->state = I2C_TRANS_IDLE;
          event |= I2C_EVENT_TRANSACTION_DONE;
        }

        break;
      }

      /* Send next byte */
      DRV_I2C_WriteCmdFifo(i2c, I2C_TX_SEQ | (i2c->data[i2c->cnt] << I2C_CMD_DATA_POS), false);
      break;

    case I2C_TRANS_READ_DATA:
      DRV_I2C_ReadDataFifo(i2c, &data, false);
      i2c->data[i2c->cnt++] = (uint8_t)(data >> 24);
      i2c->num--;
      if (!i2c->num) {
        if (!i2c->pending) {
          i2c->state = I2C_TRANS_STOP;
          DRV_I2C_WriteCmdFifo(i2c, I2C_STOP_SEQ, false);
        } else {
          i2c->state = I2C_TRANS_IDLE;
          event |= I2C_EVENT_TRANSACTION_DONE;
        }
      } else {
        /* Receive next byte */
        DRV_I2C_WriteCmdFifo(i2c, I2C_RX_SEQ | (i2c->num != 1 ? 0 : I2C_NACK_SEQ), false);
      }

      break;

    case I2C_TRANS_STOP:
      i2c->state = I2C_TRANS_IDLE;
      event |= I2C_EVENT_TRANSACTION_DONE;
      break;

    case I2C_TRANS_ERROR:
      i2c->pending = false;
      event |= i2c->pend_event;
      i2c->pend_event = 0;
      i2c->aborting = false;
      i2c->state = I2C_TRANS_IDLE;
      break;

    default:
      event |= I2C_EVENT_TRANSACTION_INCOMPLETE;
      break;
  }

  return event;
}

static uint32_t DRV_I2C_NackEventHandler(Drv_I2C_Peripheral *i2c) {
  switch (i2c->state) {
    case I2C_TRANS_WRITE_HADDR:
    case I2C_TRANS_WRITE_LADDR:
      i2c->state = I2C_TRANS_ERROR;
      i2c->pend_event |= I2C_EVENT_TRANSACTION_ADDRNACK;
      DRV_I2C_ResetPeripheral(i2c);
      DRV_I2C_ClearFifo(i2c);
      DRV_I2C_WriteCmdFifo(i2c, I2C_STOP_SEQ, false);
      break;

    default:
      i2c->state = I2C_TRANS_ERROR;
      i2c->pend_event |= I2C_EVENT_TRANSACTION_INCOMPLETE;
      DRV_I2C_ResetPeripheral(i2c);
      DRV_I2C_ClearFifo(i2c);
      DRV_I2C_WriteCmdFifo(i2c, I2C_STOP_SEQ, false);
      break;
  }

  return 0;
}

static void DRV_I2C_IrqHandler(I2C_BusId bus_id) {
  struct Drv_I2C_Peripheral *i2c;

  i2c = DRV_I2C_GetPeripheral(bus_id);
  I2C_ASSERT(i2c);

  /* Clear interrupt cause */
  uint32_t rc;

  rc = i2c->reg->INTERRUPT_STATUS_RC;

  uint32_t event;

  event = 0;
  if (rc & I2C0_CTRL_INTERRUPT_STATUS_RC_NACK_ERR_C_INT_Msk) {
    event |= DRV_I2C_NackEventHandler(i2c);
  }

  if (rc & I2C0_CTRL_INTERRUPT_STATUS_RC_DONE_C_INT_Msk) {
    event |= DRV_I2C_DoneEventHandler(i2c);
  }

  /* Callback event notification */
  if (event && i2c->evt_cb.event_callback) {
    i2c->evt_cb.event_callback(i2c->evt_cb.user_data, event);
  }
}

static DRV_I2C_Status DRV_I2C_ForceStop(Drv_I2C_Peripheral *i2c, uint32_t event) {
  DRV_I2C_SetPeripheralIrq(i2c, false);
  i2c->pend_event |= event;
  i2c->state = I2C_TRANS_ERROR;
  i2c->aborting = true;
  DRV_I2C_ResetPeripheral(i2c);
  DRV_I2C_ClearFifo(i2c);
  DRV_I2C_SetPeripheralIrq(i2c, true);
  DRV_I2C_WriteCmdFifo(i2c, I2C_STOP_SEQ, false);
  return DRV_I2C_OK;
}

static uint16_t DRV_I2C_CalculateSlaveParam(uint32_t addr, bool isWriteOp) {
  if (addr & I2C_10BIT_INDICATOR) {
    return (uint16_t)((((addr & I2C_10BITS_MASK) | I2C_10BITS_PREFIX) << 1) |
                      (isWriteOp ? I2C_10BITS_WRITE : I2C_10BITS_READ) | (addr & 0xFF));
  } else {
    return (uint16_t)(((addr & I2C_7BITS_MASK) << 1) |
                      (isWriteOp ? I2C_7BITS_WRITE : I2C_7BITS_READ));
  }
}

#if (configUSE_ALT_SLEEP == 1)
static int32_t DRV_I2C_HandleSleepNotify(DRV_SLEEP_NotifyType sleep_state, DRV_PM_PwrMode pwr_mode,
                                         void *ptr_ctx) {
  struct Drv_I2C_Peripheral *i2c = (struct Drv_I2C_Peripheral *)ptr_ctx;

  if (pwr_mode == DRV_PM_MODE_STANDBY || pwr_mode == DRV_PM_MODE_SHUTDOWN) {
    if (sleep_state == DRV_SLEEP_NTFY_SUSPENDING) {
      /* Do nothiung while suspending */
      ;
    } else if (sleep_state == DRV_SLEEP_NTFY_RESUMING) {
      /* Reconfigure main control register */
      DRV_I2C_InitMainCtrl(i2c);

      /* Reconfigure bus speed */
      DRV_I2C_SetSpeed(i2c, i2c->speed_khz);

      /* Reconfigure power mode */
      DRV_I2C_PowerControl(i2c, i2c->power_mode);
    }
  }

  return 0;
}
#endif

/****************************************************************************
 * ISR Functions
 ****************************************************************************/

void i2c0_interrupt_err_handler(void) { DRV_I2C_IrqHandler(I2C_BUS_0); }
void i2c0_interrupt_handler(void) { DRV_I2C_IrqHandler(I2C_BUS_0); }

#ifdef ALT1250
/**
 * @brief I2C bus 1 interrupt service routine
 *
 */
void i2c1_interrupt_err_handler(void) { DRV_I2C_IrqHandler(I2C_BUS_1); }
void i2c1_interrupt_handler(void) { DRV_I2C_IrqHandler(I2C_BUS_1); }
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

DRV_I2C_Status DRV_I2C_Initialize(I2C_BusId bus_id, Drv_I2C_EventCallback *evt_cb,
                                  Drv_I2C_Peripheral **peripheral) {
  struct Drv_I2C_Peripheral *i2c;

  i2c = DRV_I2C_GetPeripheral(bus_id);
  if (!i2c) {
    return DRV_I2C_ERROR_PARAMETER;
  }

  if (!peripheral) {
    return DRV_I2C_ERROR_PARAMETER;
  }

  if (evt_cb) {
    i2c->evt_cb = *evt_cb;
  }

  if (i2c->is_init) {
    return DRV_I2C_ERROR_INIT;
  }

  /* Configure IO mux */
  Interface_Id i2c_intf;
#ifdef ALT1250
  i2c_intf = (bus_id == I2C_BUS_0) ? IF_CFG_I2C0 : IF_CFG_I2C1;
#else
  i2c_intf = (bus_id == I2C_BUS_0) ? IF_CFG_I2C0 : IF_CFG_LAST_IF;
#endif
  if (DRV_IF_CFG_SetIO(i2c_intf) != DRV_IF_CFG_OK) {
    return DRV_I2C_ERROR_PARAMETER;
  }

  /* Configure main control register */
  DRV_I2C_InitMainCtrl(i2c);

  /* Force perform a STOP sequence */
  if (DRV_I2C_WriteCmdFifo(i2c, I2C_STOP_SEQ, true) != DRV_I2C_OK) {
    return DRV_I2C_ERROR_UNKNOWN;
  }

  /* Configure system interrupt */
  NVIC_SetPriority(i2c->irq_num[0], 7U); /* set Interrupt priority */
  NVIC_SetPriority(i2c->irq_num[1], 7U); /* set Interrupt priority */

#if (configUSE_ALT_SLEEP == 1)
  /* Register sleep notification */
  if (!i2c->registered_sleep_notify) {
    int32_t ret_val;

    ret_val =
        DRV_SLEEP_RegNotification(&DRV_I2C_HandleSleepNotify, &i2c->sleep_notify_idx, (void *)i2c);
    if (ret_val != 0) {
      i2c->sleep_notify_idx = -1;
      return DRV_I2C_ERROR_PWRMNGR;
    }

    i2c->registered_sleep_notify = 1;
  }
#endif

  i2c->is_init = true;
  *peripheral = (Drv_I2C_Peripheral *)i2c;

  /* Set default speed */
  DRV_I2C_SetSpeed(i2c, I2C_DEFAULT_SPEED_KHZ);

  return DRV_I2C_OK;
}

DRV_I2C_Status DRV_I2C_Uninitialize(I2C_BusId bus_id) {
  struct Drv_I2C_Peripheral *i2c;

  i2c = DRV_I2C_GetPeripheral(bus_id);
  if (!i2c) {
    return DRV_I2C_ERROR_PARAMETER;
  }

  if (!i2c->is_init) {
    return DRV_I2C_ERROR_UNINIT;
  }

  DRV_I2C_PowerControl(i2c, I2C_POWER_OFF);
#if (configUSE_ALT_SLEEP == 1)
  /* Register sleep notification */
  if (i2c->registered_sleep_notify) {
    int32_t ret_val;

    ret_val = DRV_SLEEP_UnRegNotification(i2c->sleep_notify_idx);
    if (ret_val != 0) {
      return DRV_I2C_ERROR_PWRMNGR;
    }

    i2c->sleep_notify_idx = -1;
    i2c->registered_sleep_notify = 0;
  }
#endif

  i2c->is_init = false;
  return DRV_I2C_OK;
}

DRV_I2C_Status DRV_I2C_SetSpeed(Drv_I2C_Peripheral *i2c, uint32_t speed_khz) {
  uint32_t count_divide;

  if (!i2c) {
    return DRV_I2C_ERROR_PARAMETER;
  }

  if (!i2c->is_init) {
    return DRV_I2C_ERROR_UNINIT;
  }

  switch (speed_khz) {
    case I2C_BUS_SPEED_STANDARD:
    case I2C_BUS_SPEED_FAST:
    case I2C_BUS_SPEED_FAST_PLUS:
    case I2C_BUS_SPEED_HIGH:
      break;

    default:
      return DRV_I2C_ERROR_UNSUPPORTED;
  }

  /* Calculate divider */
  count_divide = (uint32_t)((SystemCoreClock >> 21) * ((float)1000 / speed_khz));

  /* Set timing */
  i2c->reg->COUNTER1_CFG_b.COUNT_DIVIDE = count_divide;
  i2c->reg->COUNTER1_CFG_b.COUNT_RECOVERY = count_divide * 10;
  i2c->reg->COUNTER2_CFG_b.COUNT_T2R = 2;
  i2c->reg->COUNTER2_CFG_b.COUNT_R2T = 2;
  i2c->reg->COUNTER3_CFG_b.COUNT_START_HOLD = count_divide >> 1;
  i2c->reg->COUNTER3_CFG_b.COUNT_START_SETUP = count_divide >> 1;
  i2c->reg->COUNTER4_CFG_b.COUNT_STOP_SETUP = count_divide >> 1;
  i2c->reg->COUNTER4_CFG_b.DUTY_CYCLE = 0;
  i2c->speed_khz = speed_khz;
  return DRV_I2C_OK;
}

DRV_I2C_Status DRV_I2C_MasterTransmit_Nonblock(Drv_I2C_Peripheral *i2c, uint32_t addr,
                                               const uint8_t *data, uint32_t num,
                                               bool xfer_pending) {
  if (!i2c || !data || !num) {
    return DRV_I2C_ERROR_PARAMETER;
  }

  if (!i2c->is_init) {
    return DRV_I2C_ERROR_UNINIT;
  }

  if (i2c->power_mode != I2C_POWER_FULL) {
    return DRV_I2C_ERROR_PWRCFG;
  }

  /* Setup request */
  i2c->sla_rw = DRV_I2C_CalculateSlaveParam(addr, true);

  bool prev_pending;

  prev_pending = i2c->pending;
  i2c->pending = xfer_pending;
  i2c->data = (uint8_t *)data;
  i2c->num = num;
  i2c->cnt = -1;
  i2c->pend_event = 0;
  i2c->state = I2C_TRANS_START;
  i2c->aborting = false;

  /* Clear FIFO & enable peripheral interrupt */
  DRV_I2C_ClearFifo(i2c);
  DRV_I2C_SetPeripheralIrq(i2c, true);

  /* Start transaction */
  DRV_I2C_WriteCmdFifo(i2c, !prev_pending ? I2C_START_SEQ : I2C_RESTART_SEQ, false);

  return DRV_I2C_OK;
}

DRV_I2C_Status DRV_I2C_MasterReceive_Nonblock(Drv_I2C_Peripheral *i2c, uint32_t addr,
                                              const uint8_t *data, uint32_t num,
                                              bool xfer_pending) {
  if (!i2c || !data || !num) {
    return DRV_I2C_ERROR_PARAMETER;
  }

  if (!i2c->is_init) {
    return DRV_I2C_ERROR_UNINIT;
  }

  if (i2c->power_mode != I2C_POWER_FULL) {
    return DRV_I2C_ERROR_PWRCFG;
  }

  /* Setup request */
  i2c->sla_rw = DRV_I2C_CalculateSlaveParam(addr, false);

  bool prev_pending;

  prev_pending = i2c->pending;
  i2c->pending = xfer_pending;
  i2c->data = (uint8_t *)data;
  i2c->num = num;
  i2c->cnt = -1;
  i2c->pend_event = 0;
  i2c->state = I2C_TRANS_START;
  i2c->aborting = false;

  /* Clear FIFO & enable peripheral interrupt */
  DRV_I2C_ClearFifo(i2c);
  DRV_I2C_SetPeripheralIrq(i2c, true);

  /* Start transaction */
  DRV_I2C_WriteCmdFifo(i2c, !prev_pending ? I2C_START_SEQ : I2C_RESTART_SEQ, false);

  return DRV_I2C_OK;
}

DRV_I2C_Status DRV_I2C_MasterTransmit_Block(Drv_I2C_Peripheral *i2c, uint32_t addr,
                                            const uint8_t *data, uint32_t num, bool xfer_pending) {
  if (!i2c || !data || !num) {
    return DRV_I2C_ERROR_PARAMETER;
  }

  if (!i2c->is_init) {
    return DRV_I2C_ERROR_UNINIT;
  }

  if (i2c->power_mode != I2C_POWER_FULL) {
    return DRV_I2C_ERROR_PWRCFG;
  }

  /* Setup request */
  i2c->sla_rw = DRV_I2C_CalculateSlaveParam(addr, true);

  bool prev_pending;

  prev_pending = i2c->pending;
  i2c->pending = xfer_pending;
  i2c->data = (uint8_t *)data;
  i2c->num = num;
  i2c->cnt = -1;

  /* Clear FIFO & disable peripheral interrupt */
  DRV_I2C_ClearFifo(i2c);
  DRV_I2C_SetPeripheralIrq(i2c, false);

  int32_t ret = DRV_I2C_OK;

  /* Start sequence */
  if (DRV_I2C_WriteCmdFifo(i2c, !prev_pending ? I2C_START_SEQ : I2C_RESTART_SEQ, true) !=
      DRV_I2C_OK) {
    ret = DRV_I2C_ERROR_START;
    goto errout;
  }

  /* Addressing slave */
  if (addr & I2C_10BIT_INDICATOR) {
    /* Send 10-bit high addr */
    if (DRV_I2C_WriteCmdFifo(i2c, I2C_TX_SEQ | (i2c->sla_rw & I2C_CMD_DATA_MASK), true) !=
        DRV_I2C_OK) {
      ret = DRV_I2C_ERROR_HNACK;
      goto errout;
    }

    /* Send 10-bit low addr */
    if (DRV_I2C_WriteCmdFifo(i2c, I2C_TX_SEQ | (i2c->sla_rw << I2C_CMD_DATA_POS), true) !=
        DRV_I2C_OK) {
      ret = DRV_I2C_ERROR_LNACK;
      goto errout;
    }
  } else {
    /* Send 7-bit addr */
    if (DRV_I2C_WriteCmdFifo(i2c, I2C_TX_SEQ | (i2c->sla_rw << I2C_CMD_DATA_POS), true) !=
        DRV_I2C_OK) {
      ret = DRV_I2C_ERROR_LNACK;
      goto errout;
    }
  }

  /* Write data */
  i2c->cnt = 0;
  do {
    if (DRV_I2C_WriteCmdFifo(i2c, I2C_TX_SEQ | (i2c->data[i2c->cnt] << I2C_CMD_DATA_POS), true) !=
        DRV_I2C_OK) {
      ret = DRV_I2C_ERROR_TX;
      goto errout;
    }

    i2c->cnt++;
    i2c->num--;
  } while (i2c->num != 0);

  /* Stop sequence */
  if (!i2c->pending) {
    if (DRV_I2C_WriteCmdFifo(i2c, I2C_STOP_SEQ, true) != DRV_I2C_OK) {
      ret = DRV_I2C_ERROR_STOP;
      goto errout;
    }
  }

  goto out;

errout:
  i2c->pending = 0;
  DRV_I2C_ResetPeripheral(i2c);
  DRV_I2C_ClearFifo(i2c);
  I2C_ASSERT(DRV_I2C_WriteCmdFifo(i2c, I2C_STOP_SEQ, true) == DRV_I2C_OK);

out:
  return ret;
}

DRV_I2C_Status DRV_I2C_MasterReceive_Block(Drv_I2C_Peripheral *i2c, uint32_t addr,
                                           const uint8_t *data, uint32_t num, bool xfer_pending) {
  if (!i2c || !data || !num) {
    return DRV_I2C_ERROR_PARAMETER;
  }

  if (!i2c->is_init) {
    return DRV_I2C_ERROR_UNINIT;
  }

  if (i2c->power_mode != I2C_POWER_FULL) {
    return DRV_I2C_ERROR_PWRCFG;
  }

  /* Setup request */
  i2c->sla_rw = DRV_I2C_CalculateSlaveParam(addr, false);

  bool prev_pending;

  prev_pending = i2c->pending;
  i2c->pending = xfer_pending;
  i2c->data = (uint8_t *)data;
  i2c->num = num;
  i2c->cnt = -1;

  /* Clear FIFO & disable peripheral interrupt */
  DRV_I2C_ClearFifo(i2c);
  DRV_I2C_SetPeripheralIrq(i2c, false);

  int32_t ret = DRV_I2C_OK;

  /* Start sequence */
  if (DRV_I2C_WriteCmdFifo(i2c, !prev_pending ? I2C_START_SEQ : I2C_RESTART_SEQ, true) !=
      DRV_I2C_OK) {
    ret = DRV_I2C_ERROR_START;
    goto errout;
  }

  /* Addressing slave */
  if (addr & I2C_10BIT_INDICATOR) {
    /* Send 10-bit high addr */
    if (DRV_I2C_WriteCmdFifo(i2c, I2C_TX_SEQ | (i2c->sla_rw & I2C_CMD_DATA_MASK), true) !=
        DRV_I2C_OK) {
      ret = DRV_I2C_ERROR_HNACK;
      goto errout;
    }

    /* Send 10-bit low addr */
    if (DRV_I2C_WriteCmdFifo(i2c, I2C_TX_SEQ | (i2c->sla_rw << I2C_CMD_DATA_POS), true) !=
        DRV_I2C_OK) {
      ret = DRV_I2C_ERROR_LNACK;
      goto errout;
    }
  } else {
    /* Send 7-bit addr */
    if (DRV_I2C_WriteCmdFifo(i2c, I2C_TX_SEQ | (i2c->sla_rw << I2C_CMD_DATA_POS), true) !=
        DRV_I2C_OK) {
      ret = DRV_I2C_ERROR_LNACK;
      goto errout;
    }
  }

  /* Read data */
  i2c->cnt = 0;
  do {
    if (DRV_I2C_WriteCmdFifo(i2c, I2C_RX_SEQ | (i2c->num != 1 ? 0 : I2C_NACK_SEQ), true) !=
        DRV_I2C_OK) {
      ret = DRV_I2C_ERROR_RX;
      goto errout;
    }

    uint32_t tmp;

    if (DRV_I2C_ReadDataFifo(i2c, &tmp, true) != DRV_I2C_OK) {
      ret = DRV_I2C_ERROR_RXFIFO;
      goto errout;
    }

    i2c->data[i2c->cnt++] = (uint8_t)(tmp >> 24);
    i2c->num--;
  } while (i2c->num != 0);

  /* Stop sequence */
  if (!i2c->pending) {
    if (DRV_I2C_WriteCmdFifo(i2c, I2C_STOP_SEQ, true) != DRV_I2C_OK) {
      ret = DRV_I2C_ERROR_STOP;
      goto errout;
    }
  }

  goto out;

errout:
  i2c->pending = 0;
  DRV_I2C_ResetPeripheral(i2c);
  DRV_I2C_ClearFifo(i2c);
  I2C_ASSERT(DRV_I2C_WriteCmdFifo(i2c, I2C_STOP_SEQ, true) == DRV_I2C_OK);

out:
  return ret;
}

DRV_I2C_Status DRV_I2C_PowerControl(Drv_I2C_Peripheral *i2c, I2C_PowerMode mode) {
  if (!i2c) {
    return DRV_I2C_ERROR_PARAMETER;
  }

  if (!i2c->is_init) {
    return DRV_I2C_ERROR_UNINIT;
  }

  switch (mode) {
    case I2C_POWER_OFF:
      NVIC_DisableIRQ(i2c->irq_num[0]);
      NVIC_DisableIRQ(i2c->irq_num[1]);
      DRV_I2C_ResetPeripheral(i2c);
      break;

    case I2C_POWER_FULL:
      NVIC_ClearPendingIRQ(i2c->irq_num[0]);
      NVIC_ClearPendingIRQ(i2c->irq_num[1]);
      NVIC_EnableIRQ(i2c->irq_num[0]);
      NVIC_EnableIRQ(i2c->irq_num[1]);
      DRV_I2C_ResetPeripheral(i2c);
      break;

    default:
      return DRV_I2C_ERROR_PARAMETER;
  }

  i2c->power_mode = mode;
  return DRV_I2C_OK;
}

int32_t DRV_I2C_GetDataCount(Drv_I2C_Peripheral *i2c) {
  if (!i2c) {
    return -(DRV_I2C_ERROR_PARAMETER);
  }

  if (!i2c->is_init) {
    return -(DRV_I2C_ERROR_UNINIT);
  }

  return i2c->cnt;
}

DRV_I2C_Status DRV_I2C_ClearBus(Drv_I2C_Peripheral *i2c) {
  if (!i2c) {
    return DRV_I2C_ERROR_PARAMETER;
  }

  if (!i2c->is_init) {
    return DRV_I2C_ERROR_UNINIT;
  }

  DRV_I2C_ForceStop(i2c, I2C_EVENT_TRANSACTION_BUS_CLEAR);
  return DRV_I2C_OK;
}

DRV_I2C_Status DRV_I2C_AbortTransfer(Drv_I2C_Peripheral *i2c) {
  if (!i2c) {
    return DRV_I2C_ERROR_PARAMETER;
  }

  if (!i2c->is_init) {
    return DRV_I2C_ERROR_UNINIT;
  }

  DRV_I2C_ForceStop(i2c, I2C_EVENT_TRANSACTION_INCOMPLETE);
  return DRV_I2C_OK;
}
