/*  ---------------------------------------------------------------------------

        (c) copyright 2018 Altair Semiconductor, Ltd. All rights reserved.

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

#include "FreeRTOS.h"
#include <stdio.h>
#include "task.h"
#include "portmacro.h"
#include "125X_mcu.h"
#include "gdma.h"
#include "semphr.h"

// clang-format off
#define MCU_GDMA_ADDR_DIFF (MCU_GDMA_CMD1_CNT_INC - MCU_GDMA_CMD0_CNT_INC)
#define MCU_GDMA_DMA_CH_SEL_ADDR_DIFF (MCU_GDMA_DMA_CH1_SEL-MCU_GDMA_DMA_CH0_SEL)
#define MCU_GDMA_DMA_DONE_INT_ADDR_DIFF (MCU_GDMA_DMA_CH1_DONE_INT_STAT-MCU_GDMA_DMA_CH0_DONE_INT_STAT)

#define MCU_GDMA_CNT_INC(x)				(MCU_GDMA_CMD0_CNT_INC + (x * MCU_GDMA_ADDR_DIFF))
#define MCU_GDMA_CNT_INC_DATA_MSK			MCU_GDMA_CMD0_CNT_INC_DATA_MSK
#define MCU_GDMA_CNT_INC_DATA_POS			MCU_GDMA_CMD0_CNT_INC_DATA_POS
#define MCU_GDMA_BASE_ADDR(x)				(MCU_GDMA_CMD0_BASE_ADDR + (x * MCU_GDMA_ADDR_DIFF))
#define MCU_GDMA_BASE_ADDR_DATA_MSK		        MCU_GDMA_CMD0_BASE_ADDR_DATA_MSK
#define MCU_GDMA_BASE_ADDR_DATA_POS		        MCU_GDMA_CMD0_BASE_ADDR_DATA_POS
#define MCU_GDMA_INDEX(x)				(MCU_GDMA_CMD0_INDEX + (x * MCU_GDMA_ADDR_DIFF))
#define MCU_GDMA_INDEX_DATA_MSK				MCU_GDMA_CMD0_INDEX_DATA_MSK
#define MCU_GDMA_INDEX_DATA_POS				MCU_GDMA_CMD0_INDEX_DATA_POS
#define MCU_GDMA_SIZE(x)				(MCU_GDMA_CMD0_SIZE + (x * MCU_GDMA_ADDR_DIFF))
#define MCU_GDMA_SIZE_DATA_MSK				MCU_GDMA_CMD0_SIZE_DATA_MSK
#define MCU_GDMA_SIZE_DATA_POS			        MCU_GDMA_CMD0_SIZE_DATA_POS
#define MCU_GDMA_STATUS(x)				(MCU_GDMA_CMD0_STATUS + (x * MCU_GDMA_ADDR_DIFF))
#define MCU_GDMA_STATUS_CNT_MSK			        MCU_GDMA_CMD0_STATUS_CNT_MSK
#define MCU_GDMA_STATUS_CNT_POS			        MCU_GDMA_CMD0_STATUS_CNT_POS
#define MCU_GDMA_STATUS_EMPTY_MSK			MCU_GDMA_CMD0_STATUS_EMPTY_MSK
#define MCU_GDMA_STATUS_EMPTY_POS			MCU_GDMA_CMD0_STATUS_EMPTY_POS
#define MCU_GDMA_STATUS_FULL_MSK			MCU_GDMA_CMD0_STATUS_FULL_MSK
#define MCU_GDMA_STATUS_FULL_POS			MCU_GDMA_CMD0_STATUS_FULL_POS
#define MCU_GDMA_STAT_INDEX(x)				(MCU_GDMA_STAT0_INDEX + (x * MCU_GDMA_ADDR_DIFF))
#define MCU_GDMA_STAT_INDEX_DATA_MSK			MCU_GDMA_STAT0_INDEX_DATA_MSK
#define MCU_GDMA_STAT_INDEX_DATA_POS			MCU_GDMA_STAT0_INDEX_DATA_POS
#define MCU_GDMA_DMA_CH_SEL(x)				(MCU_GDMA_DMA_CH0_SEL + (x * MCU_GDMA_DMA_CH_SEL_ADDR_DIFF))
#define MCU_GDMA_DMA_CH_SEL_DATA_MSK			MCU_GDMA_DMA_CH0_SEL_DATA_MSK
#define MCU_GDMA_DMA_CH_SEL_DATA_POS			MCU_GDMA_DMA_CH0_SEL_DATA_POS
#define MCU_GDMA_DMA_CH_DONE_INT_STAT(x)                (MCU_GDMA_DMA_CH0_DONE_INT_STAT + (x * MCU_GDMA_DMA_DONE_INT_ADDR_DIFF))
#define MCU_GDMA_DMA_CH_DONE_INT_STAT_STAT_MSK          MCU_GDMA_DMA_CH0_DONE_INT_STAT_STAT_MSK
#define MCU_GDMA_DMA_CH_DONE_INT_STAT_STAT_POS          MCU_GDMA_DMA_CH0_DONE_INT_STAT_STAT_POS
// clang-format on

typedef struct {
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
  uint32_t WAIT4READY : 1;        // for security and or external interfaces - handshake mechanism
  uint32_t DIS_RD_BYTE_EN : 1;  // default 0x1 - for sensitive slaves should be 0x0 (FIFO that pops
                                // the bytes)
  uint32_t MAX_BURST_LEN : 2;   // 0-single 1-4words 2-8words 3-16words
  uint32_t MAX_OUTSTANDING_TRANS : 2;  // 0-no outstanding 1-2 outstanding transactions 2-4
                                       // outstanding transactions - for firefly max is 2

  uint32_t INTR_MODE : 1;
  uint32_t reserve : 4;
  uint32_t STAT_INTR_EMITTED : 1;       // status - written by the GDMA - The command status that it
                                        // already emitted the interrupt
  uint32_t STAT_FIFO_SECURITY_ERR : 1;  // status - written by the GDMA - The command status is with
                                        // error indication
  uint32_t STAT_FIFO_VALIDNESS : 1;  // status - written by the GDMA - The command status is valid
} dmaCtrlBitStruc;

typedef union {
  uint32_t regVal;
  dmaCtrlBitStruc Ctrl;
} udmaCtrl;

typedef struct {
  // Word 1
  uint32_t readAddress;  // [31:0]

  // Word 2
  uint32_t writeAddress;  // [31:0]

  // Word 3
  uint32_t transactionLength : 24;  // [23:0]
  uint32_t CMD_ID : 8;              // [31:24]

  // Word 4
  udmaCtrl dmaCtrl;
} tDmaCommand;

typedef struct {
  tDmaCommand dma_command;
  uint32_t int_counter;
  gdma_callback irq_handler_user;
  volatile SemaphoreHandle_t dmaIsrSemaphore;
  char reserved4alignment[4];
} gdma_info_t;

static gdma_info_t gdma_info[GMDA_CH_MAX] __attribute__((aligned(16)));

static void gdma_HandleISR(gdma_ch_enum_t channel) {
  // Clear interrupt
  REGISTER(MCU_GDMA_DMA_CH_DONE_INT_STAT(channel));

  // Call handler
  if (gdma_info[channel].irq_handler_user) {
    gdma_info[channel].irq_handler_user(0, channel);
  }
}

void gdma_channel_0_handler(void) { gdma_HandleISR(GDMA_CH_0); }

void gdma_channel_1_handler(void) { gdma_HandleISR(GDMA_CH_1); }

void gdma_channel_2_handler(void) { gdma_HandleISR(GDMA_CH_2); }

void gdma_channel_3_handler(void) { gdma_HandleISR(GDMA_CH_3); }

void gdma_channel_4_handler(void) { gdma_HandleISR(GDMA_CH_4); }

void gdma_channel_5_handler(void) { gdma_HandleISR(GDMA_CH_5); }

void gdma_channel_6_handler(void) { gdma_HandleISR(GDMA_CH_6); }

void gdma_channel_7_handler(void) { gdma_HandleISR(GDMA_CH_7); }

void InitDma(void) {
  int n;

  for (n = 0; n < GMDA_CH_MAX; n++) {
    gdma_info[n].dmaIsrSemaphore = NULL;
    gdma_info[n].irq_handler_user = NULL;
    gdma_info[n].int_counter = 0;

    gdma_info[n].dma_command.CMD_ID = n;
    gdma_info[n].dma_command.dmaCtrl.Ctrl.RD_ADDR_MODE = 1;
    gdma_info[n].dma_command.dmaCtrl.Ctrl.RD_ADDR_INC_STEP = 3;
    gdma_info[n].dma_command.dmaCtrl.Ctrl.RD_BUS_WIDTH = 3;
    gdma_info[n].dma_command.dmaCtrl.Ctrl.RD_BYTE_ENDIAN = 1;
    gdma_info[n].dma_command.dmaCtrl.Ctrl.RD_WORD_ENDIAN = 1;

    gdma_info[n].dma_command.dmaCtrl.Ctrl.WR_ADDR_MODE = 1;
    gdma_info[n].dma_command.dmaCtrl.Ctrl.WR_ADDR_INC_STEP = 3;
    gdma_info[n].dma_command.dmaCtrl.Ctrl.WR_BUS_WIDTH = 3;
    gdma_info[n].dma_command.dmaCtrl.Ctrl.WR_BYTE_ENDIAN = 1;
    gdma_info[n].dma_command.dmaCtrl.Ctrl.WR_WORD_ENDIAN = 1;

    gdma_info[n].dma_command.dmaCtrl.Ctrl.ONGOING_MODE = 1;
    gdma_info[n].dma_command.dmaCtrl.Ctrl.RD_AFTER_WR_SYNC = 0;
    gdma_info[n].dma_command.dmaCtrl.Ctrl.WAIT4READY = 0;
    gdma_info[n].dma_command.dmaCtrl.Ctrl.DIS_RD_BYTE_EN = 0;

    gdma_info[n].dma_command.dmaCtrl.Ctrl.MAX_BURST_LEN = 3;
    gdma_info[n].dma_command.dmaCtrl.Ctrl.MAX_OUTSTANDING_TRANS = 2;

    gdma_info[n].dma_command.dmaCtrl.Ctrl.INTR_MODE = 0;

    gdma_info[n].dma_command.dmaCtrl.Ctrl.reserve = 0;

    gdma_info[n].dma_command.dmaCtrl.Ctrl.STAT_FIFO_SECURITY_ERR = 0;
    gdma_info[n].dma_command.dmaCtrl.Ctrl.STAT_INTR_EMITTED = 0;
    gdma_info[n].dma_command.dmaCtrl.Ctrl.STAT_FIFO_VALIDNESS = 0;

    REGISTER(MCU_GDMA_BASE_ADDR(n)) = (uint32_t)&gdma_info[n].dma_command;
    REGISTER(MCU_GDMA_INDEX(n)) = 0;
    REGISTER(MCU_GDMA_SIZE(n)) = 1;
    //    REGISTER(MCU_GDMA_DMA_CH_SEL(n)) = 2; // No need to set this register when WAIT4READY is
    //    set to 0
    gdma_info[n].dmaIsrSemaphore = xSemaphoreCreateBinary();

    // Enable interrupt
    NVIC_SetPriority((IRQn_Type)(GDMA_CH0_IRQn + n), 7); /* set Interrupt priority */
    NVIC_EnableIRQ((IRQn_Type)(GDMA_CH0_IRQn + n));
  }
}

int gdma_register_interrupt(gdma_ch_enum_t channel, gdma_callback irq_handler) {
  if (channel >= GMDA_CH_MAX) {
    return -GDMA_INVALID_CH_NUMBER;
  }

  if (irq_handler) {
    gdma_info[channel].irq_handler_user = irq_handler;
    gdma_info[channel].dma_command.dmaCtrl.Ctrl.INTR_MODE = 1;
  } else {
    gdma_info[channel].irq_handler_user = NULL;
    gdma_info[channel].dma_command.dmaCtrl.Ctrl.INTR_MODE = 0;
  }
  return GDMA_OK;
}

int gdma_initiate_transfer(void *s1, const void *s2, size_t n, gdma_ch_enum_t channel) {
  volatile tDmaCommand *p_dma_command;

  if (channel >= GMDA_CH_MAX) {
    return -GDMA_INVALID_CH_NUMBER;
  }

  p_dma_command = &gdma_info[channel].dma_command;

  p_dma_command->readAddress = (uint32_t)s2;
  p_dma_command->writeAddress = (uint32_t)s1;
  p_dma_command->transactionLength = n;
  p_dma_command->dmaCtrl.Ctrl.STAT_FIFO_VALIDNESS = 0;

  // Start DMA action
  REGISTER(MCU_GDMA_CNT_INC(channel)) = 1;

  return GDMA_OK;
}

static void gdma_smpr_irq_handler(int error, gdma_ch_enum_t channel) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  BaseType_t ret =
      xSemaphoreGiveFromISR(gdma_info[channel].dmaIsrSemaphore, &xHigherPriorityTaskWoken);

  configASSERT(ret == pdPASS);
}

int gdma_transfer_blocking(void *s1, const void *s2, size_t n, gdma_ch_enum_t channel, uint32_t timeout) {
  gdma_callback irq_handler_keeper;
  int intr_mode_keeper;
  int ret_val = GDMA_OK;

  if (channel >= GMDA_CH_MAX) {
    return -GDMA_INVALID_CH_NUMBER;
  }

  timeout = timeout / portTICK_PERIOD_MS;
  if (timeout == 0) {
    timeout = 1;
  }

  //  gdma_info[channel].dmaIsrSemaphore = xSemaphoreCreateBinary();
  irq_handler_keeper = gdma_info[channel].irq_handler_user;
  intr_mode_keeper = gdma_info[channel].dma_command.dmaCtrl.Ctrl.INTR_MODE;

  // Register irq handler
  gdma_info[channel].irq_handler_user = gdma_smpr_irq_handler;
  gdma_info[channel].dma_command.dmaCtrl.Ctrl.INTR_MODE = 1;

  gdma_initiate_transfer(s1, s2, n, channel);

  // Wait for DMA transfer to finish
  if (xSemaphoreTake(gdma_info[channel].dmaIsrSemaphore, timeout) == pdFALSE) {
    // DMA transfer failed (timeout expired)
    ret_val = -GDMA_TIMEOUT;
  }

  portDISABLE_INTERRUPTS();
  //  vSemaphoreDelete(gdma_info[channel].dmaIsrSemaphore);
  gdma_info[channel].irq_handler_user = irq_handler_keeper;
  gdma_info[channel].dma_command.dmaCtrl.Ctrl.INTR_MODE = intr_mode_keeper;
  portENABLE_INTERRUPTS();
  return ret_val;
}

int gdma_transfer_status(gdma_ch_enum_t channel) {
  if (channel >= GMDA_CH_MAX) {
    return -GDMA_INVALID_CH_NUMBER;
  }

  return gdma_info[channel].dma_command.dmaCtrl.Ctrl.STAT_FIFO_VALIDNESS;
}
