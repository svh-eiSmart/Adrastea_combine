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
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Device Headers */
#include DEVICE_HEADER

/* Driver Specific Headers */
#include "DRV_GDMA.h"
#if (configUSE_ALT_SLEEP == 1)
#include "DRV_PM.h"
#include "DRV_SLEEP.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* For command FIFO offset */
#define DRV_GDMA_CMD_REG_DIFF() \
  ((uint8_t *)&gdma_peripheral.reg->CMD1_CNT_INC - (uint8_t *)&gdma_peripheral.reg->CMD0_CNT_INC)
#define DRV_GDMA_CMD_BASEADDR(n)                                            \
  (*(volatile uint32_t *)((uint8_t *)&gdma_peripheral.reg->CMD0_BASE_ADDR + \
                          DRV_GDMA_CMD_REG_DIFF() * n))
#define DRV_GDMA_CMD_IDX(n)                                             \
  (*(volatile uint32_t *)((uint8_t *)&gdma_peripheral.reg->CMD0_INDEX + \
                          DRV_GDMA_CMD_REG_DIFF() * n))
#define DRV_GDMA_CMD_SIZE(n) \
  (*(volatile uint32_t *)((uint8_t *)&gdma_peripheral.reg->CMD0_SIZE + DRV_GDMA_CMD_REG_DIFF() * n))
#define DRV_GDMA_CMD_CNT_INC(n)                                           \
  (*(volatile uint32_t *)((uint8_t *)&gdma_peripheral.reg->CMD0_CNT_INC + \
                          DRV_GDMA_CMD_REG_DIFF() * n))

/* For interrupt stat offset */
#define DRV_GDMA_DONE_INT_STAT_REG_DIFF()                   \
  ((uint8_t *)&gdma_peripheral.reg->DMA_CH1_DONE_INT_STAT - \
   (uint8_t *)&gdma_peripheral.reg->DMA_CH0_DONE_INT_STAT)
#define DRV_GDMA_DONE_INT_STAT(n)                                                  \
  (*(volatile uint32_t *)((uint8_t *)&gdma_peripheral.reg->DMA_CH0_DONE_INT_STAT + \
                          DRV_GDMA_DONE_INT_STAT_REG_DIFF() * n))

/* Assertion Checking */
#define GDMA_ASSERT(x) \
  if ((x) == 0) {      \
    for (;;)           \
      ;                \
  } /**< Assertion check */

#define GDMA_BUSYWAIT_CNT 1000

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef union {
  __IOM uint32_t CMD_CNT_INC;

  struct {
    __IOM uint32_t DATA : 8; /*!< [7..0] Writing to this register increases the atomic counter
                                  value; thus causing the DMA start fetching commands from
                                  the command FIFO */
    uint32_t : 24;
  } CMD_CNT_INC_b;
} DRV_GDMA_CntInc;

typedef struct {
  // Word 1
  uint32_t read_addr;  // [31:0]
  // Word 2
  uint32_t write_addr;  // [31:0]
  // Word 3
  union {
    uint32_t cmd_translen;
    struct {
      uint32_t transaction_len : 24;  // [23:0]
      uint32_t cmd_id : 8;            // [31:24]
    } cmd_translen_b;
  };
  // Word 4
  union {
    uint32_t dma_ctrl;
    struct {
      uint32_t RD_ADDR_MODE : 2;
      uint32_t RD_ADDR_INC_STEP : 2;  // default 0x3
      uint32_t RD_BUS_WIDTH : 2;      // default 0x3
      uint32_t RD_BYTE_ENDIAN : 1;    // default 0x1
      uint32_t RD_WORD_ENDIAN : 1;    // default 0x1

      uint32_t WR_ADDR_MODE : 2;
      uint32_t WR_ADDR_INC_STEP : 2;  // default 0x3
      uint32_t WR_BUS_WIDTH : 2;      // default 0x3
      uint32_t WR_BYTE_ENDIAN : 1;    // default 0x1
      uint32_t WR_WORD_ENDIAN : 1;    // default 0x1

      uint32_t ONGOING_MODE : 1;      // default 0x0
      uint32_t RD_AFTER_WR_SYNC : 1;  // default 0x0
      uint32_t WAIT4READY : 1;      // for security and or external interfaces - handshake mechanism
      uint32_t DIS_RD_BYTE_EN : 1;  // default 0x1 - for sensitive slaves should be 0x0 (FIFO that
                                    // pops the bytes)
      uint32_t MAX_BURST_LEN : 2;   // 0-single 1-4words 2-8words 3-16words
      uint32_t MAX_OUTSTANDING_TRANS : 2;  // 0-no outstanding 1-2 outstanding transactions 2-4
                                           // outstanding transactions - for firefly max is 2

      uint32_t INTR_MODE : 1;
      uint32_t reserve : 4;
      uint32_t STAT_INTR_EMITTED : 1;  // status - written by the GDMA - The command status that it
                                       // already emitted the interrupt
      uint32_t STAT_FIFO_SECURITY_ERR : 1;  // status - written by the GDMA - The command status is
                                            // with error indication
      uint32_t
          STAT_FIFO_VALIDNESS : 1;  // status - written by the GDMA - The command status is valid
    } dma_ctrl_b;
  };
} DRV_GDMA_Command;

struct Drv_GDMA_Peripheral {
  GDMA_Type *reg;                          /**< Peripheral Register file */
  IRQn_Type irq_num[DRV_GDMA_Channel_MAX]; /**< IRQ */
  DRV_GDMA_Command dma_cmd[DRV_GDMA_Channel_MAX]
      __attribute__((aligned(16))); /**< DMA command to be referred in command FIFO */
  DRV_GDMA_EventCallback evt_cb[DRV_GDMA_Channel_MAX]; /**< Event callback */
  bool is_init;
#if (configUSE_ALT_SLEEP == 1)
  bool registered_sleep_notify; /**< Registration indicator of sleep notification */
  int32_t sleep_notify_idx;     /**< Sleep notification index */
#endif
};

/****************************************************************************
 * Private Data
 ****************************************************************************/
struct Drv_GDMA_Peripheral gdma_peripheral = {
    GDMA,
    {GDMA_CH0_IRQn, GDMA_CH1_IRQn, GDMA_CH2_IRQn, GDMA_CH3_IRQn, GDMA_CH4_IRQn, GDMA_CH5_IRQn,
     GDMA_CH6_IRQn, GDMA_CH7_IRQn},
    {{0}},
    {{NULL, NULL}},
    false
#if (configUSE_ALT_SLEEP == 1)
    ,
    false,
    -1
#endif
};

/****************************************************************************
 * External Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
void DRV_GDMA_IrqHandler(DRV_GDMA_Channel channel) {
  GDMA_ASSERT(channel < DRV_GDMA_Channel_MAX);

  /* Read & clear interrupt stat */
  uint32_t int_stat;

  int_stat = DRV_GDMA_DONE_INT_STAT(channel);
  GDMA_ASSERT(int_stat & GDMA_DMA_CH0_DONE_INT_STAT_STAT_Msk);

  uint32_t event;

  event = 0;
  if (gdma_peripheral.dma_cmd[channel].dma_ctrl_b.STAT_FIFO_VALIDNESS) {
    event |= GDMA_EVENT_TRANSACTION_DONE;
  }

  if (gdma_peripheral.dma_cmd[channel].dma_ctrl_b.STAT_FIFO_SECURITY_ERR) {
    event |= GDMA_EVENT_TRANSACTION_ERROR;
  }

  /* Callback to app */
  if (gdma_peripheral.evt_cb[channel].event_callback) {
    gdma_peripheral.evt_cb[channel].event_callback(event, channel,
                                                   gdma_peripheral.evt_cb[channel].user_data);
  }
}

#if (configUSE_ALT_SLEEP == 1)
static int32_t DRV_GDMA_HandleSleepNotify(DRV_SLEEP_NotifyType sleep_state, DRV_PM_PwrMode pwr_mode,
                                          void *ptr_ctx) {
  if (pwr_mode == DRV_PM_MODE_STANDBY || pwr_mode == DRV_PM_MODE_SHUTDOWN) {
    if (sleep_state == DRV_SLEEP_NTFY_SUSPENDING) {
      /* Do nothiung while suspending */
      ;
    } else if (sleep_state == DRV_SLEEP_NTFY_RESUMING) {
      int n;

      for (n = 0; n < DRV_GDMA_Channel_MAX; n++) {
        /* Configure DMA Command FIFO */
        gdma_peripheral.dma_cmd[n].cmd_translen_b.cmd_id = n;
        gdma_peripheral.dma_cmd[n].dma_ctrl_b.RD_ADDR_MODE = 1;
        gdma_peripheral.dma_cmd[n].dma_ctrl_b.RD_ADDR_INC_STEP = 3;
        gdma_peripheral.dma_cmd[n].dma_ctrl_b.RD_BUS_WIDTH = 3;
        gdma_peripheral.dma_cmd[n].dma_ctrl_b.RD_BYTE_ENDIAN = 1;
        gdma_peripheral.dma_cmd[n].dma_ctrl_b.RD_WORD_ENDIAN = 1;

        gdma_peripheral.dma_cmd[n].dma_ctrl_b.WR_ADDR_MODE = 1;
        gdma_peripheral.dma_cmd[n].dma_ctrl_b.WR_ADDR_INC_STEP = 3;
        gdma_peripheral.dma_cmd[n].dma_ctrl_b.WR_BUS_WIDTH = 3;
        gdma_peripheral.dma_cmd[n].dma_ctrl_b.WR_BYTE_ENDIAN = 1;
        gdma_peripheral.dma_cmd[n].dma_ctrl_b.WR_WORD_ENDIAN = 1;

        gdma_peripheral.dma_cmd[n].dma_ctrl_b.ONGOING_MODE = 1;
        gdma_peripheral.dma_cmd[n].dma_ctrl_b.RD_AFTER_WR_SYNC = 0;
        gdma_peripheral.dma_cmd[n].dma_ctrl_b.WAIT4READY = 0;
        gdma_peripheral.dma_cmd[n].dma_ctrl_b.DIS_RD_BYTE_EN = 0;
        gdma_peripheral.dma_cmd[n].dma_ctrl_b.MAX_BURST_LEN = 3;
        gdma_peripheral.dma_cmd[n].dma_ctrl_b.MAX_OUTSTANDING_TRANS = 2;

        gdma_peripheral.dma_cmd[n].dma_ctrl_b.INTR_MODE = 0;
        gdma_peripheral.dma_cmd[n].dma_ctrl_b.reserve = 0;
        gdma_peripheral.dma_cmd[n].dma_ctrl_b.STAT_INTR_EMITTED = 0;
        gdma_peripheral.dma_cmd[n].dma_ctrl_b.STAT_FIFO_SECURITY_ERR = 0;
        gdma_peripheral.dma_cmd[n].dma_ctrl_b.STAT_FIFO_VALIDNESS = 0;

        DRV_GDMA_CMD_BASEADDR(n) = (uint32_t)&gdma_peripheral.dma_cmd[n];
        DRV_GDMA_CMD_IDX(n) = 0;
        DRV_GDMA_CMD_SIZE(n) = 1;
      }
    }
  }

  return 0;
}
#endif

/****************************************************************************
 * ISR Functions
 ****************************************************************************/

void gdma_channel_0_handler(void) { DRV_GDMA_IrqHandler(DRV_GDMA_Channel_0); }

void gdma_channel_1_handler(void) { DRV_GDMA_IrqHandler(DRV_GDMA_Channel_1); }

void gdma_channel_2_handler(void) { DRV_GDMA_IrqHandler(DRV_GDMA_Channel_2); }

void gdma_channel_3_handler(void) { DRV_GDMA_IrqHandler(DRV_GDMA_Channel_3); }

void gdma_channel_4_handler(void) { DRV_GDMA_IrqHandler(DRV_GDMA_Channel_4); }

void gdma_channel_5_handler(void) { DRV_GDMA_IrqHandler(DRV_GDMA_Channel_5); }

void gdma_channel_6_handler(void) { DRV_GDMA_IrqHandler(DRV_GDMA_Channel_6); }

void gdma_channel_7_handler(void) { DRV_GDMA_IrqHandler(DRV_GDMA_Channel_7); }

/****************************************************************************
 * Public Functions
 ****************************************************************************/
DRV_GDMA_Status DRV_GDMA_Initialize(void) {
  if (gdma_peripheral.is_init) {
    return DRV_GDMA_ERROR_INIT;
  }

  /* TODO: Add configuration for channel arbitration scheme */

  int n;

  for (n = 0; n < DRV_GDMA_Channel_MAX; n++) {
    /* Configure DMA Command FIFO */
    gdma_peripheral.dma_cmd[n].cmd_translen_b.cmd_id = n;
    gdma_peripheral.dma_cmd[n].dma_ctrl_b.RD_ADDR_MODE = 1;
    gdma_peripheral.dma_cmd[n].dma_ctrl_b.RD_ADDR_INC_STEP = 3;
    gdma_peripheral.dma_cmd[n].dma_ctrl_b.RD_BUS_WIDTH = 3;
    gdma_peripheral.dma_cmd[n].dma_ctrl_b.RD_BYTE_ENDIAN = 1;
    gdma_peripheral.dma_cmd[n].dma_ctrl_b.RD_WORD_ENDIAN = 1;

    gdma_peripheral.dma_cmd[n].dma_ctrl_b.WR_ADDR_MODE = 1;
    gdma_peripheral.dma_cmd[n].dma_ctrl_b.WR_ADDR_INC_STEP = 3;
    gdma_peripheral.dma_cmd[n].dma_ctrl_b.WR_BUS_WIDTH = 3;
    gdma_peripheral.dma_cmd[n].dma_ctrl_b.WR_BYTE_ENDIAN = 1;
    gdma_peripheral.dma_cmd[n].dma_ctrl_b.WR_WORD_ENDIAN = 1;

    gdma_peripheral.dma_cmd[n].dma_ctrl_b.ONGOING_MODE = 1;
    gdma_peripheral.dma_cmd[n].dma_ctrl_b.RD_AFTER_WR_SYNC = 0;
    gdma_peripheral.dma_cmd[n].dma_ctrl_b.WAIT4READY = 0;
    gdma_peripheral.dma_cmd[n].dma_ctrl_b.DIS_RD_BYTE_EN = 0;
    gdma_peripheral.dma_cmd[n].dma_ctrl_b.MAX_BURST_LEN = 3;
    gdma_peripheral.dma_cmd[n].dma_ctrl_b.MAX_OUTSTANDING_TRANS = 2;

    gdma_peripheral.dma_cmd[n].dma_ctrl_b.INTR_MODE = 0;
    gdma_peripheral.dma_cmd[n].dma_ctrl_b.reserve = 0;
    gdma_peripheral.dma_cmd[n].dma_ctrl_b.STAT_INTR_EMITTED = 0;
    gdma_peripheral.dma_cmd[n].dma_ctrl_b.STAT_FIFO_SECURITY_ERR = 0;
    gdma_peripheral.dma_cmd[n].dma_ctrl_b.STAT_FIFO_VALIDNESS = 0;

    DRV_GDMA_CMD_BASEADDR(n) = (uint32_t)&gdma_peripheral.dma_cmd[n];
    DRV_GDMA_CMD_IDX(n) = 0;
    DRV_GDMA_CMD_SIZE(n) = 1;

    /* Set priority & enable interrupt */
    NVIC_SetPriority(gdma_peripheral.irq_num[n], 7);
    NVIC_EnableIRQ(gdma_peripheral.irq_num[n]);
  }

#if (configUSE_ALT_SLEEP == 1)
  /* Register sleep notification */
  if (!gdma_peripheral.registered_sleep_notify) {
    int32_t ret_val;

    ret_val = DRV_SLEEP_RegNotification(&DRV_GDMA_HandleSleepNotify,
                                        &gdma_peripheral.sleep_notify_idx, NULL);
    if (ret_val != 0) {
      gdma_peripheral.sleep_notify_idx = -1;
      return DRV_GDMA_ERROR_PWRMNGR;
    }

    gdma_peripheral.registered_sleep_notify = 1;
  }
#endif

  gdma_peripheral.is_init = true;
  return DRV_GDMA_OK;
}

DRV_GDMA_Status DRV_GDMA_Uninitialize(void) {
  if (!gdma_peripheral.is_init) {
    return DRV_GDMA_ERROR_UNINIT;
  }

  int n;

  for (n = 0; n < DRV_GDMA_Channel_MAX; n++) {
    DRV_GDMA_CMD_BASEADDR(n) = (uint32_t)NULL;
    DRV_GDMA_CMD_IDX(n) = 0;
    DRV_GDMA_CMD_SIZE(n) = 0;
  }

#if (configUSE_ALT_SLEEP == 1)
  /* Register sleep notification */
  if (gdma_peripheral.registered_sleep_notify) {
    int32_t ret_val;

    ret_val = DRV_SLEEP_UnRegNotification(gdma_peripheral.sleep_notify_idx);
    if (ret_val != 0) {
      return DRV_GDMA_ERROR_PWRMNGR;
    }

    gdma_peripheral.sleep_notify_idx = -1;
    gdma_peripheral.registered_sleep_notify = 0;
  }
#endif

  gdma_peripheral.is_init = false;
  return DRV_GDMA_OK;
}

DRV_GDMA_Status DRV_GDMA_RegisterIrqCallback(DRV_GDMA_Channel channel,
                                             DRV_GDMA_EventCallback *callback) {
  if (!gdma_peripheral.is_init) {
    return DRV_GDMA_ERROR_UNINIT;
  }

  if (channel >= DRV_GDMA_Channel_MAX) {
    return DRV_GDMA_ERROR_INVALID_CH;
  }

  if (callback) {
    gdma_peripheral.evt_cb[channel] = *callback;
  } else {
    gdma_peripheral.evt_cb[channel].event_callback = NULL;
    gdma_peripheral.evt_cb[channel].user_data = NULL;
  }
  return DRV_GDMA_OK;
}

DRV_GDMA_Status DRV_GDMA_Transfer_Block(DRV_GDMA_Channel channel, void *dest, const void *src,
                                        size_t n) {
  if (!gdma_peripheral.is_init) {
    return DRV_GDMA_ERROR_UNINIT;
  }

  if (channel >= DRV_GDMA_Channel_MAX) {
    return DRV_GDMA_ERROR_INVALID_CH;
  }

  DRV_GDMA_Command *dma_cmd;

  dma_cmd = &gdma_peripheral.dma_cmd[channel];
  dma_cmd->read_addr = (uint32_t)src;
  dma_cmd->write_addr = (uint32_t)dest;
  dma_cmd->cmd_translen_b.transaction_len = n;
  dma_cmd->dma_ctrl_b.INTR_MODE = 0;
  dma_cmd->dma_ctrl_b.STAT_INTR_EMITTED = 0;
  dma_cmd->dma_ctrl_b.STAT_FIFO_SECURITY_ERR = 0;
  dma_cmd->dma_ctrl_b.STAT_FIFO_VALIDNESS = 0;
  // Start DMA action
  DRV_GDMA_CMD_CNT_INC(channel) = 1;

  int32_t wait;

  wait = GDMA_BUSYWAIT_CNT;
  while (--wait > 0) {
    if ((dma_cmd->dma_ctrl_b.STAT_FIFO_VALIDNESS != 1)) {
      continue;
    }

    if (dma_cmd->dma_ctrl_b.STAT_FIFO_SECURITY_ERR == 1) {
      return DRV_GDMA_ERROR_UNKNOWN;
    }

    return DRV_GDMA_OK;
  }

  return DRV_GDMA_ERROR_TIMEOUT;
}
DRV_GDMA_Status DRV_GDMA_Transfer_Nonblock(DRV_GDMA_Channel channel, void *dest, const void *src,
                                           size_t n) {
  if (!gdma_peripheral.is_init) {
    return DRV_GDMA_ERROR_UNINIT;
  }

  if (channel >= DRV_GDMA_Channel_MAX) {
    return DRV_GDMA_ERROR_INVALID_CH;
  }

  DRV_GDMA_Command *dma_cmd;

  dma_cmd = &gdma_peripheral.dma_cmd[channel];
  dma_cmd->read_addr = (uint32_t)src;
  dma_cmd->write_addr = (uint32_t)dest;
  dma_cmd->cmd_translen_b.transaction_len = n;
  dma_cmd->dma_ctrl_b.INTR_MODE = 1;
  dma_cmd->dma_ctrl_b.STAT_INTR_EMITTED = 0;
  dma_cmd->dma_ctrl_b.STAT_FIFO_SECURITY_ERR = 0;
  dma_cmd->dma_ctrl_b.STAT_FIFO_VALIDNESS = 0;
  // Start DMA action
  DRV_GDMA_CMD_CNT_INC(channel) = 1;

  return DRV_GDMA_OK;
}
