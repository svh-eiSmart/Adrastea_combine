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
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "DRV_COMMON.h"
#include "DRV_UART.h"
#include "DRV_IF_CFG.h"
#include "hifc_api.h"
#if (configUSE_ALT_SLEEP == 1)
#include "pwr_mngr.h"
#endif
#include "DRV_PM.h"
#include "DRV_SLEEP.h"

/* Assertion Checking */
#define UART_ASSERT(x)                             \
  if ((x) == 0) {                                  \
    printf("Assert at %s %d", __FILE__, __LINE__); \
    for (;;)                                       \
      ;                                            \
  } /**< Assertion check */

#if !defined(MIN)
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#if defined(ALT1250)
#define UART_BASE_PTR \
  { UARTF0, UARTF1, UARTI0 }
#elif defined(ALT1255)
#define UART_BASE_PTR \
  { UARTF0, UARTI0 }
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define HIFC_LOOP (100)
static UART_Type *uart_bases[] = UART_BASE_PTR;
static DRV_UART_Handle *uart_handles[ARRAY_SIZE(uart_bases)];

#if (configUSE_ALT_SLEEP == 1)
static uint32_t UARTF0_REG_CFG[14] = {0};
static uint32_t UARTI0_REG_CFG[14] = {0};
static uint32_t UARTI1_REG_CFG[14] = {0};
#if defined(ALT1250)
static uint32_t UARTI2_REG_CFG[14] = {0};
#endif

static uint8_t registered_sleep_notify = 0;
#endif
static uint8_t be_flag = 0;

/* Function Prototype*/
static void DRV_UART_Enable_Interrupt(UART_Type *base, uint32_t mask);
static void DRV_UART_Disable_Interrupt(UART_Type *base, uint32_t mask);

typedef enum {
  RI_INTERRUPT = UARTF0_IMSC_RIMIM_Msk,
  CTS_INTERRUPT = UARTF0_IMSC_CTSMIM_Msk,
  DCD_INTERRUPT = UARTF0_IMSC_DCDMIM_Msk,
  DSR_INTERRUPT = UARTF0_IMSC_DSRMIM_Msk,
  RX_FULL = UARTF0_IMSC_RXIM_Msk,
  TX_EMPTY = UARTF0_IMSC_TXIM_Msk,
  RX_TIMEOUT = UARTF0_IMSC_RTIM_Msk,
  FRAME_ERROR = UARTF0_IMSC_FEIM_Msk,
  PARITY_ERROR = UARTF0_IMSC_PEIM_Msk,
  BREAK_ERROR = UARTF0_IMSC_BEIM_Msk,
  OVERRUN_ERROR = UARTF0_IMSC_OEIM_Msk,
} DRV_UART_Interrupt_Type;

static void DRV_UART_Enable_Interrupt(UART_Type *base, uint32_t mask) {
  UART_ASSERT(base);
  uint32_t reg;

  do {
    reg = __LDREXW(&base->IMSC);
    reg |= mask;
  } while (__STREXW(reg, &base->IMSC));
}

static void DRV_UART_Disable_Interrupt(UART_Type *base, uint32_t mask) {
  UART_ASSERT(base);
  uint32_t reg;

  do {
    reg = __LDREXW(&base->IMSC);
    reg &= ~mask;
  } while (__STREXW(reg, &base->IMSC));
}

static size_t Availble_Byte_In_Ringbuffer(DRV_UART_Handle *handle) {
  UART_ASSERT(handle);
  UART_ASSERT(handle->rx_ringbuffer);

  size_t data_len;
  size_t head = handle->ringbuffer_head;
  size_t tail = handle->ringbuffer_tail;

  if (tail > head) {
    data_len = handle->ringbuffer_size - tail + head;
  } else {
    data_len = head - tail;
  }

  return data_len;
}

/*
  This function doens't check available data in the ringbuffer.
*/
static uint8_t Get_Byte_From_Ringbuffer(DRV_UART_Handle *handle) {
  UART_ASSERT(handle);
  UART_ASSERT(handle->rx_ringbuffer);
  UART_ASSERT(handle->ringbuffer_tail != handle->ringbuffer_head);  // make sure data is available

  uint8_t c;

  c = handle->rx_ringbuffer[handle->ringbuffer_tail];

  if (handle->ringbuffer_tail + 1 == handle->ringbuffer_size) {
    handle->ringbuffer_tail = 0;
  } else {
    handle->ringbuffer_tail++;
  }

  return c;
}

/**
 * @brief move data in the ringbuffer to the requested location.
 *
 * @param handle UART handle
 * @param buffer destination of moved data
 * @param length buffer length
 * @return size_t actually bytes that moved
 */
static size_t Get_Bytes_From_Ringbuffer(DRV_UART_Handle *handle, int8_t *buffer, size_t length) {
  UART_ASSERT(handle);
  UART_ASSERT(buffer);

  size_t data_len_rb = 0;
  size_t bytes_to_copy = 0;
  uint32_t i;

  // calculate how many bytes in ringbuffer
  data_len_rb = Availble_Byte_In_Ringbuffer(handle);

  if (data_len_rb != 0) {
    // copy data to user buffer
    bytes_to_copy = MIN(data_len_rb, length);

    for (i = 0; i < bytes_to_copy; i++) {
      buffer[i] = Get_Byte_From_Ringbuffer(handle);
    }
  }
  return bytes_to_copy;
}
static uint8_t Is_Ringbuffer_Full(DRV_UART_Handle *handle) {
  size_t data_len;

  data_len = Availble_Byte_In_Ringbuffer(handle);

  if (data_len == handle->ringbuffer_size - 1) {
    return true;
  } else {
    return false;
  }
}

static uint8_t DRV_UART_Get_Instance_From_Base(UART_Type *base) {
  uint8_t i;
  for (i = 0; i < ARRAY_SIZE(uart_bases); i++) {
    if (uart_bases[i] == base) {
      return i;
    }
  }

  UART_ASSERT(0);
}

static inline DRV_UART_Status DRV_UART_Is_Valid_Data(DRV_UART_Handle *handle, uint8_t *data) {
  UART_ASSERT(handle);
  UART_ASSERT(data);

  uint32_t uartdr;
  DRV_UART_Status ret = DRV_UART_ERROR_NONE;

  reset_inactivity_timer(handle->base);
  uartdr = handle->base->DR;
  if (uartdr & UARTF0_DR_FE_Msk) {
    ret = DRV_UART_ERROR_FRAME;
    handle->stat.fe_cnt++;
  }
  if (uartdr & UARTF0_DR_BE_Msk) {
    be_flag = 1;
    ret = DRV_UART_ERROR_BREAK;
    handle->stat.be_cnt++;
  }
  if (uartdr & UARTF0_DR_OE_Msk) {
    ret = DRV_UART_ERROR_OVERRUN;
    handle->stat.oe_cnt++;
  }
  if (uartdr & UARTF0_DR_PE_Msk) {
    ret = DRV_UART_ERROR_PARITY;
    handle->stat.pe_cnt++;
  }
  *data = uartdr & UARTF0_DR_DATA_Msk;

  return ret;
}

static void DRV_UART_IrqHandler(DRV_UART_Handle *handle) {
  UART_ASSERT(handle);
  UART_ASSERT(handle->rx_ringbuffer);

  UART_Type *base = handle->base;
  uint8_t data = 0;
  DRV_UART_Status st;

  // process tx fifo empty interrupt
  if (base->MIS_b.TXMIS) {
    while (handle->tx_data_remaining_byte) {
      if (base->FR_b.TXFF == 0)  // tx fifo has room
      {
        reset_inactivity_timer(handle->base);
        base->DR = *handle->tx_data;
        handle->tx_data++;
        handle->tx_data_remaining_byte--;
        handle->stat.tx_cnt_isr++;
      } else {
        break;  // if tx fifo is full, exit isr immediately.
      }
    }

    // if all data are pushed to fifo, notify upper layer
    if (handle->tx_data_remaining_byte == 0 && handle->tx_data && handle->callback) {
      handle->callback(handle, CB_TX_COMPLETE, handle->user_data);
      handle->tx_line_state = DRV_UART_LINE_STATE_IDLE;
      handle->tx_data = NULL;

      DRV_UART_Disable_Interrupt(handle->base, TX_EMPTY);
    }

    base->ICR_b.TXIC = 1;  // clear interrupt
  }                        // end of if( base->MIS_b.TXMIS)

  // process receive interrupt
  if (base->MIS_b.RXMIS || base->MIS_b.RTMIS) {
    // if nonblocking read is not yet completed, save data to user buffer directly
    while (handle->rx_data_remaining_byte != 0) {
#if (configUSE_ALT_SLEEP == 1)
      if (base == UARTF0) {
        pwr_mngr_refresh_uart_inactive_time();
      }
#endif
      if (base->FR_b.RXFE == 0) {
        st = DRV_UART_Is_Valid_Data(handle, &data);
        if (st == DRV_UART_ERROR_NONE || st == DRV_UART_ERROR_OVERRUN) {
          *handle->rx_data = data;
          handle->rx_data++;
          handle->rx_data_remaining_byte--;
          handle->stat.rx_cnt_isr++;
        }
      } else {
        // clear interrupt
        base->ICR_b.RXIC = 1;
        base->ICR_b.RTIC = 1;
        base->ICR_b.BEIC = 1;
        //return;
        goto end;
      }

      // If user requested length is done, send notification to upper layer
      if (handle->rx_data_remaining_byte == 0 && handle->callback) {
        handle->callback(handle, CB_RX_COMPLETE, handle->user_data);
        handle->rx_data = NULL;
        handle->rx_line_state = DRV_UART_LINE_STATE_IDLE;
      }
    }

    // If FIFO still has data and no pending user request, save data to ringbuffer
    while (base->FR_b.RXFE == 0) {
      if (Is_Ringbuffer_Full(handle) == false) {
#if (configUSE_ALT_SLEEP == 1)
        if (base == UARTF0) {
          pwr_mngr_refresh_uart_inactive_time();
        }
#endif
        st = DRV_UART_Is_Valid_Data(handle, &data);
        if (st == DRV_UART_ERROR_NONE || st == DRV_UART_ERROR_OVERRUN) {
          handle->rx_ringbuffer[handle->ringbuffer_head] = data;
          handle->stat.rx_cnt_isr++;
          if (handle->ringbuffer_head + 1 == handle->ringbuffer_size) {
            handle->ringbuffer_head = 0;
          } else {
            handle->ringbuffer_head++;
          }
        }

      } else {
        // if ringbuffer is full, do not receive data and turn off interrupt.
        // if flow control is enabled, RTS will assert. if not, new data will be lost.
        DRV_UART_Disable_Interrupt(handle->base, RX_FULL | RX_TIMEOUT);
        break;
      }
    }
    // clear interrupt
    base->ICR_b.RXIC = 1;
    base->ICR_b.RTIC = 1;
    base->ICR_b.BEIC = 1;
end:
    if (be_flag) {
      if (handle->callback) {
        handle->callback(handle, CB_BREAK_ERROR, handle->user_data);
      }
      base->ICR_b.BEIC = 1;
      be_flag = 0;
    }
  }  // end of  "if( base->MIS_b.RXMIS != 0)"
}

static DRV_UART_Status DRV_UART_Register_Ringbuffer(DRV_UART_Handle *handle, int8_t *buffer,
                                                    size_t buffer_size) {
  UART_ASSERT(handle);
  UART_ASSERT(buffer);

  handle->rx_ringbuffer = buffer;
  handle->ringbuffer_size = buffer_size;
  handle->ringbuffer_head = 0;
  handle->ringbuffer_tail = 0;

  return DRV_UART_ERROR_NONE;
}

DRV_UART_Handle *DRV_UART_Get_Handle(DRV_UART_Port port) { return uart_handles[port]; }

void hw_uart_interrupt_handler0(void) { DRV_UART_IrqHandler(DRV_UART_Get_Handle(DRV_UART_F0)); }

#ifdef ALT1250
void hw_uart_interrupt_handler1(void) { DRV_UART_IrqHandler(DRV_UART_Get_Handle(DRV_UART_F1)); }
#endif

void hw_uart_interrupt_handleri0(void) { DRV_UART_IrqHandler(DRV_UART_Get_Handle(DRV_UART_I0)); }

DRV_UART_Status DRV_UART_Set_Baudrate(UART_Type *base, uint32_t baudrate, uint32_t clock_rate) {
  uint32_t idiv, fdiv, fpres;

  fpres = ((clock_rate << 3) / baudrate) + 1;
  idiv = (fpres >> 7);
  fdiv = (((fpres - (idiv << 7))) >> 1);

  base->IBRD = idiv;
  base->FBRD = fdiv;

  base->LCR_H_b.BRK = base->LCR_H_b.BRK;  // in order to update baudrate

  return DRV_UART_ERROR_NONE;
}

static int8_t hifc_request_to_send(hifc_sys_lpuart_t *base) {
  int8_t hifcloop = HIFC_LOOP;

  while (hifcloop--) {
    if (request_to_send(base) == hifc_req_success) {
      return 0;
    }
  }

  return -1;
}
DRV_UART_Status DRV_UART_Set_Parity(UART_Type *base, DRV_UART_Parity parity) {
  switch (parity) {
    case DRV_UART_PARITY_MODE_ZERO:
      base->LCR_H_b.PEN = 1;
      base->LCR_H_b.EPS = 1;
      base->LCR_H_b.SPS = 1;
      break;
    case DRV_UART_PARITY_MODE_ONE:
      base->LCR_H_b.PEN = 1;
      base->LCR_H_b.EPS = 0;
      base->LCR_H_b.SPS = 1;
      break;
    case DRV_UART_PARITY_MODE_ODD:
      base->LCR_H_b.PEN = 1;
      base->LCR_H_b.EPS = 0;
      base->LCR_H_b.SPS = 0;
      break;
    case DRV_UART_PARITY_MODE_EVEN:
      base->LCR_H_b.PEN = 1;
      base->LCR_H_b.EPS = 1;
      base->LCR_H_b.SPS = 0;
      break;
    case DRV_UART_PARITY_MODE_DISABLE:
      base->LCR_H_b.PEN = 0;
      base->LCR_H_b.EPS = 0;
      base->LCR_H_b.SPS = 0;
      break;
    default:
      UART_ASSERT(0);
  }

  return DRV_UART_ERROR_NONE;
}
DRV_UART_Status DRV_UART_Set_Hardware_Flow_Control(UART_Type *base,
                                                   DRV_UART_Hardware_Flow_Control ctrl) {
  switch (ctrl) {
    case DRV_UART_HWCONTROL_NONE:
      base->CR_b.RTSEN = 0;
      base->CR_b.CTSEN = 0;
      break;
    case DRV_UART_HWCONTROL_RTS:
      base->CR_b.RTSEN = 1;
      base->CR_b.CTSEN = 0;
      break;
    case DRV_UART_HWCONTROL_CTS:
      base->CR_b.RTSEN = 0;
      base->CR_b.CTSEN = 1;
      break;
    case DRV_UART_HWCONTROL_RTS_CTS:
      base->CR_b.RTSEN = 1;
      base->CR_b.CTSEN = 1;
      break;
    default:
      UART_ASSERT(0);
  }
  return DRV_UART_ERROR_NONE;
}
DRV_UART_Status DRV_UART_Set_Stopbit(UART_Type *base, DRV_UART_Stop_Bits stop_bit) {
  switch (stop_bit) {
    case DRV_UART_STOP_BITS_1:
      base->LCR_H_b.STP2 = 0;
      break;
    case DRV_UART_STOP_BITS_2:
      base->LCR_H_b.STP2 = 1;
      break;
    default:
      UART_ASSERT(0);
  }
  return DRV_UART_ERROR_NONE;
}
DRV_UART_Status DRV_UART_Set_Word_length(UART_Type *base, DRV_UART_Word_Length word_length) {
  switch (word_length) {
    case DRV_UART_WORD_LENGTH_5B:
      base->LCR_H_b.WLEN = 0;
      break;
    case DRV_UART_WORD_LENGTH_6B:
      base->LCR_H_b.WLEN = 1;
      break;
    case DRV_UART_WORD_LENGTH_7B:
      base->LCR_H_b.WLEN = 2;
      break;
    case DRV_UART_WORD_LENGTH_8B:
      base->LCR_H_b.WLEN = 3;
      break;
    default:
      UART_ASSERT(0);
  }
  return DRV_UART_ERROR_NONE;
}
static void DRV_UART_Flush_Fifo(UART_Type *base) {
  base->LCR_H_b.FEN = 0;
  base->LCR_H_b.FEN = 1;
}
void DRV_UART_Enable_Fifo(UART_Type *base) {
  /* Set uart fifo level to 1/8 full */
  base->IFLS_b.TXIFLSEL = 0;
  base->IFLS_b.RXIFLSEL = 0;
  base->IFLS = 0x20;
  base->LCR_H_b.FEN = 1;
}
void DRV_UART_Disable_Fifo(UART_Type *base) {
  /* Set uart fifo level to 1/8 full */
  base->IFLS_b.TXIFLSEL = 0;
  base->IFLS_b.RXIFLSEL = 0;
  base->LCR_H_b.FEN = 0;
}
void DRV_UART_Enable_Tx(UART_Type *base) { base->CR_b.TXE = 1; }
void DRV_UART_Disable_Tx(UART_Type *base) { base->CR_b.TXE = 0; }
void DRV_UART_Enable_Rx(UART_Type *base) { base->CR_b.RXE = 1; }
void DRV_UART_Disable_Rx(UART_Type *base) { base->CR_b.RXE = 0; }
void DRV_UART_Active_RTS(UART_Type *base) { base->CR_b.RTS = 1; }
void DRV_UART_Deactive_RTS(UART_Type *base) { base->CR_b.RTS = 0; }
void DRV_UART_Active_DTR(UART_Type *base) { base->CR_b.DTR = 1; }
void DRV_UART_Deactive_DTR(UART_Type *base) { base->CR_b.DTR = 0; }
void DRV_UART_Enable_Break_Interrupt(UART_Type *base) {
  DRV_UART_Enable_Interrupt(base, BREAK_ERROR);
}
void DRV_UART_Disable_Break_Interrupt(UART_Type *base) {
  DRV_UART_Disable_Interrupt(base, BREAK_ERROR);
}
uint32_t DRV_UART_Get_CTS(UART_Type *base) { return base->FR_b.CTS; }
uint32_t DRV_UART_Get_DSR(UART_Type *base) { return base->FR_b.DSR; }
uint32_t DRV_UART_Get_DCD(UART_Type *base) { return base->FR_b.DCD; }
uint32_t DRV_UART_Get_RI(UART_Type *base) { return base->FR_b.RI; }
void DRV_UART_Enable_Uart(UART_Type *base) {
  base->CR_b.TXE = 1;
  base->CR_b.RXE = 1;
  base->CR_b.UARTEN = 1;
}
void DRV_UART_Disable_Uart(UART_Type *base)

{
  base->CR_b.TXE = 0;
  base->CR_b.RXE = 0;
  base->CR_b.UARTEN = 0;
  DRV_UART_Flush_Fifo(base);
}

uint32_t DRV_UART_Get_Transmitted_Tx_Count(DRV_UART_Handle *handle) {
  UART_ASSERT(handle);
  return handle->tx_data_length_requested - handle->tx_data_remaining_byte;
}

uint32_t DRV_UART_Get_Received_Rx_Count(DRV_UART_Handle *handle) {
  UART_ASSERT(handle);
  return handle->rx_data_length_requested - handle->rx_data_remaining_byte;
}
void DRV_UART_Update_Setting(UART_Type *base, const DRV_UART_Config *UART_config, uint32_t clock) {
  UART_ASSERT(base);
  UART_ASSERT(UART_config);

  DRV_UART_Disable_Uart(base);
  DRV_UART_Disable_Fifo(base);
  DRV_UART_Set_Baudrate(base, (uint32_t)UART_config->BaudRate, clock);
  DRV_UART_Set_Parity(base, UART_config->Parity);
  DRV_UART_Set_Stopbit(base, UART_config->StopBits);
  DRV_UART_Set_Word_length(base, UART_config->WordLength);
  DRV_UART_Set_Hardware_Flow_Control(base, UART_config->HwFlowCtl);
  DRV_UART_Enable_Fifo(base);
  DRV_UART_Enable_Uart(base);
  // A dummy write to apply the settings while MAP is in sleep mode
  base->LCR_H = base->LCR_H;
}

#if (configUSE_ALT_SLEEP == 1)
int32_t DRV_UART_HandleSleepNotify(DRV_SLEEP_NotifyType sleep_state, DRV_PM_PwrMode pwr_mode,
                                   void *ptr_ctx) {
  if (pwr_mode == DRV_PM_MODE_STANDBY || pwr_mode == DRV_PM_MODE_SHUTDOWN) {
    if (sleep_state == DRV_SLEEP_NTFY_SUSPENDING) {
      /* UARTF0 */
      UARTF0_REG_CFG[0] = REGISTER(UARTF0_BASE);
      UARTF0_REG_CFG[1] = REGISTER(UARTF0_BASE + 0x4);
      UARTF0_REG_CFG[2] = REGISTER(UARTF0_BASE + 0x18);
      UARTF0_REG_CFG[3] = REGISTER(UARTF0_BASE + 0x20);
      UARTF0_REG_CFG[4] = REGISTER(UARTF0_BASE + 0x24);
      UARTF0_REG_CFG[5] = REGISTER(UARTF0_BASE + 0x28);
      UARTF0_REG_CFG[6] = REGISTER(UARTF0_BASE + 0x2c);
      UARTF0_REG_CFG[7] = REGISTER(UARTF0_BASE + 0x30);
      UARTF0_REG_CFG[8] = REGISTER(UARTF0_BASE + 0x34);
      UARTF0_REG_CFG[9] = REGISTER(UARTF0_BASE + 0x38);
      UARTF0_REG_CFG[10] = REGISTER(UARTF0_BASE + 0x3c);
      UARTF0_REG_CFG[11] = REGISTER(UARTF0_BASE + 0x40);
      UARTF0_REG_CFG[12] = REGISTER(UARTF0_BASE + 0x44);
      UARTF0_REG_CFG[13] = REGISTER(UARTF0_CFG_BASE + 0xBC);

      /* UARTI0 */
      UARTI0_REG_CFG[0] = REGISTER(UARTI0_BASE);
      UARTI0_REG_CFG[1] = REGISTER(UARTI0_BASE + 0x4);
      UARTI0_REG_CFG[2] = REGISTER(UARTI0_BASE + 0x18);
      UARTI0_REG_CFG[3] = REGISTER(UARTI0_BASE + 0x20);
      UARTI0_REG_CFG[4] = REGISTER(UARTI0_BASE + 0x24);
      UARTI0_REG_CFG[5] = REGISTER(UARTI0_BASE + 0x28);
      UARTI0_REG_CFG[6] = REGISTER(UARTI0_BASE + 0x2c);
      UARTI0_REG_CFG[7] = REGISTER(UARTI0_BASE + 0x30);
      UARTI0_REG_CFG[8] = REGISTER(UARTI0_BASE + 0x34);
      UARTI0_REG_CFG[9] = REGISTER(UARTI0_BASE + 0x38);
      UARTI0_REG_CFG[10] = REGISTER(UARTI0_BASE + 0x3c);
      UARTI0_REG_CFG[11] = REGISTER(UARTI0_BASE + 0x40);
      UARTI0_REG_CFG[12] = REGISTER(UARTI0_BASE + 0x44);

      /* UARTI1 */
      UARTI1_REG_CFG[0] = REGISTER(UARTI1_BASE);
      UARTI1_REG_CFG[1] = REGISTER(UARTI1_BASE + 0x4);
      UARTI1_REG_CFG[2] = REGISTER(UARTI1_BASE + 0x18);
      UARTI1_REG_CFG[3] = REGISTER(UARTI1_BASE + 0x20);
      UARTI1_REG_CFG[4] = REGISTER(UARTI1_BASE + 0x24);
      UARTI1_REG_CFG[5] = REGISTER(UARTI1_BASE + 0x28);
      UARTI1_REG_CFG[6] = REGISTER(UARTI1_BASE + 0x2c);
      UARTI1_REG_CFG[7] = REGISTER(UARTI1_BASE + 0x30);
      UARTI1_REG_CFG[8] = REGISTER(UARTI1_BASE + 0x34);
      UARTI1_REG_CFG[9] = REGISTER(UARTI1_BASE + 0x38);
      UARTI1_REG_CFG[10] = REGISTER(UARTI1_BASE + 0x3c);
      UARTI1_REG_CFG[11] = REGISTER(UARTI1_BASE + 0x40);
      UARTI1_REG_CFG[12] = REGISTER(UARTI1_BASE + 0x44);
#ifdef ALT1250
      /* UARTI2 */
      UARTI2_REG_CFG[0] = REGISTER(UARTI2_BASE);
      UARTI2_REG_CFG[1] = REGISTER(UARTI2_BASE + 0x4);
      UARTI2_REG_CFG[2] = REGISTER(UARTI2_BASE + 0x18);
      UARTI2_REG_CFG[3] = REGISTER(UARTI2_BASE + 0x20);
      UARTI2_REG_CFG[4] = REGISTER(UARTI2_BASE + 0x24);
      UARTI2_REG_CFG[5] = REGISTER(UARTI2_BASE + 0x28);
      UARTI2_REG_CFG[6] = REGISTER(UARTI2_BASE + 0x2c);
      UARTI2_REG_CFG[7] = REGISTER(UARTI2_BASE + 0x30);
      UARTI2_REG_CFG[8] = REGISTER(UARTI2_BASE + 0x34);
      UARTI2_REG_CFG[9] = REGISTER(UARTI2_BASE + 0x38);
      UARTI2_REG_CFG[10] = REGISTER(UARTI2_BASE + 0x3c);
      UARTI2_REG_CFG[11] = REGISTER(UARTI2_BASE + 0x40);
      UARTI2_REG_CFG[12] = REGISTER(UARTI2_BASE + 0x44);
#endif
    } else if (sleep_state == DRV_SLEEP_NTFY_RESUMING) {
      /* UARTF0 */
      REGISTER(UARTF0_BASE + 0x24) = UARTF0_REG_CFG[4];
      REGISTER(UARTF0_BASE + 0x28) = UARTF0_REG_CFG[5];
      REGISTER(UARTF0_BASE + 0x2c) = UARTF0_REG_CFG[6];
      REGISTER(UARTF0_BASE + 0x30) = UARTF0_REG_CFG[7];
      REGISTER(UARTF0_BASE + 0x34) = UARTF0_REG_CFG[8];
      REGISTER(UARTF0_BASE + 0x38) = UARTF0_REG_CFG[9];
      REGISTER(UARTF0_CFG_BASE + 0xBC) = UARTF0_REG_CFG[13];

      /* UARTI0 */
      /* may remove below checking after addressed the restore issue */
      if ((UARTI0_REG_CFG[4] != 0) && (UARTI0_REG_CFG[5] != 0) && (UARTI0_REG_CFG[7] & 0x1)) {
        REGISTER(UARTI0_BASE + 0x24) = UARTI0_REG_CFG[4];
        REGISTER(UARTI0_BASE + 0x28) = UARTI0_REG_CFG[5];
        REGISTER(UARTI0_BASE + 0x2c) = UARTI0_REG_CFG[6];
        REGISTER(UARTI0_BASE + 0x30) = UARTI0_REG_CFG[7];
        REGISTER(UARTI0_BASE + 0x34) = UARTI0_REG_CFG[8];
        REGISTER(UARTI0_BASE + 0x38) = UARTI0_REG_CFG[9];
      }

      /* UARTI1 */
      REGISTER(UARTI1_BASE + 0x24) = UARTI1_REG_CFG[4];
      REGISTER(UARTI1_BASE + 0x28) = UARTI1_REG_CFG[5];
      REGISTER(UARTI1_BASE + 0x2c) = UARTI1_REG_CFG[6];
      REGISTER(UARTI1_BASE + 0x30) = UARTI1_REG_CFG[7];
      REGISTER(UARTI1_BASE + 0x34) = UARTI1_REG_CFG[8];
      REGISTER(UARTI1_BASE + 0x38) = UARTI1_REG_CFG[9];
#ifdef ALT1250
      /* UARTI2 */
      REGISTER(UARTI2_BASE + 0x24) = UARTI2_REG_CFG[4];
      REGISTER(UARTI2_BASE + 0x28) = UARTI2_REG_CFG[5];
      REGISTER(UARTI2_BASE + 0x2c) = UARTI2_REG_CFG[6];
      REGISTER(UARTI2_BASE + 0x30) = UARTI2_REG_CFG[7];
      REGISTER(UARTI2_BASE + 0x34) = UARTI2_REG_CFG[8];
      REGISTER(UARTI2_BASE + 0x38) = UARTI2_REG_CFG[9];
#endif
    }
  }
  return 0;
}
#endif

DRV_UART_Status DRV_UART_Initialize(UART_Type *base, const DRV_UART_Config *UART_config,
                                    uint32_t clock) {
  UART_ASSERT(base);
  UART_ASSERT(UART_config);

  DRV_UART_Update_Setting(base, UART_config, clock);
#if (configUSE_ALT_SLEEP == 1)
  if (!registered_sleep_notify) {
    int32_t temp_for_sleep_notify, ret_val;

    ret_val = DRV_SLEEP_RegNotification(&DRV_UART_HandleSleepNotify, &temp_for_sleep_notify, NULL);
    if (ret_val != 0) { /* failed to register sleep notify callback */
      ;
    } else {
      registered_sleep_notify = 1;
    }
  }
#endif
  return DRV_UART_ERROR_NONE;
}

DRV_UART_Status DRV_UART_Create_Handle(UART_Type *base, DRV_UART_Handle *handle,
                                       DRV_UART_EventCallback_t callback, void *userdata,
                                       int8_t *buffer, size_t buffer_size) {
  uint8_t instance;
  UART_ASSERT(base);
  UART_ASSERT(handle);

  memset(handle, 0, sizeof(DRV_UART_Handle));
  handle->tx_line_state = DRV_UART_LINE_STATE_IDLE;
  handle->rx_line_state = DRV_UART_LINE_STATE_IDLE;
  handle->base = base;
  handle->callback = callback;
  handle->user_data = userdata;

  instance = DRV_UART_Get_Instance_From_Base(base);

  uart_handles[instance] = handle;

  if (base == UARTI0) {
    // enable hifc protocol if internal uart is selected
    if (hifc_host_interface_init(base)) {
      UART_ASSERT(0);
    }
    set_hifc_mode(hifc_mode_b);
  }
  DRV_UART_Register_Ringbuffer(handle, buffer, buffer_size);

  // after registring ringbuffer, start interrupt to receive data.
  DRV_UART_Enable_Interrupt(base, RX_FULL | RX_TIMEOUT);

  return DRV_UART_ERROR_NONE;
}

DRV_UART_Status DRV_UART_Uninitialize(DRV_UART_Handle *handle) {
  UART_ASSERT(handle);

  UART_Type *base = handle->base;

  DRV_UART_Disable_Interrupt(handle->base, RX_FULL | RX_TIMEOUT | TX_EMPTY);
  DRV_UART_Disable_Uart(base);
  DRV_UART_Disable_Fifo(base);

  memset(handle, 0, sizeof(DRV_UART_Handle));

  return DRV_UART_ERROR_NONE;
}

DRV_UART_Status DRV_UART_Receive_Block(DRV_UART_Handle *handle, void *buffer, size_t length) {
  UART_ASSERT(buffer);
  UART_ASSERT(handle);

  size_t i = 0;
  uint8_t data = 0;
  size_t copied_data;
  size_t rb_data_len;
  DRV_UART_Status ret = DRV_UART_ERROR_NONE;
  int8_t *buf = (int8_t *)buffer;

  UART_Type *base = handle->base;
  if (handle->rx_line_state == DRV_UART_LINE_STATE_BUSY) {
    return DRV_UART_ERROR_BUSY;
  }
  if (length == 0) {
    return DRV_UART_ERROR_PARAMETER;
  }

  handle->rx_line_state = DRV_UART_LINE_STATE_BUSY;
  rb_data_len = Availble_Byte_In_Ringbuffer(handle);

  if (rb_data_len < length) {
    DRV_UART_Disable_Interrupt(handle->base, RX_FULL | RX_TIMEOUT);
  }

  copied_data = Get_Bytes_From_Ringbuffer(handle, buf, length);
  length -= copied_data;
  buffer += copied_data;

  for (i = 0; i < length; i++) {
    // waiting for
    while (base->FR_b.RXFE == 1) {
    }
#if (configUSE_ALT_SLEEP == 1)
    if (base == UARTF0) {
      pwr_mngr_refresh_uart_inactive_time();
    }
#endif
    ret = DRV_UART_Is_Valid_Data(handle, &data);
    if (ret == DRV_UART_ERROR_NONE || ret == DRV_UART_ERROR_OVERRUN) {
      buf[i] = data;
      handle->stat.rx_cnt++;
    } else {
      i--;  // got error and doesn't receive a valid byte.
    }
  }

  handle->rx_line_state = DRV_UART_LINE_STATE_IDLE;
  DRV_UART_Enable_Interrupt(handle->base, RX_FULL | RX_TIMEOUT);

  return DRV_UART_ERROR_NONE;
}
DRV_UART_Status DRV_UART_Receive_Nonblock(DRV_UART_Handle *handle, void *buffer, size_t length) {
  UART_ASSERT(buffer);
  UART_ASSERT(handle);
  UART_ASSERT(handle->rx_ringbuffer);
  int8_t *buf = (int8_t *)buffer;

  size_t copied_data = 0;
  DRV_UART_Status ret;
  if (length == 0) {
    return DRV_UART_ERROR_PARAMETER;
  }

  if (handle->rx_line_state == DRV_UART_LINE_STATE_BUSY) {
    return DRV_UART_ERROR_BUSY;
  }
  handle->rx_line_state = DRV_UART_LINE_STATE_BUSY;
  handle->rx_data_length_requested = length;

  DRV_UART_Disable_Interrupt(handle->base, RX_FULL | RX_TIMEOUT);

  copied_data = Get_Bytes_From_Ringbuffer(handle, buf, length);
  length -= copied_data;
  buf += copied_data;

  if (length == 0) {
    // user request data are all copied. call callback to notify user
    handle->rx_data_remaining_byte = 0;
    handle->rx_data_total_byte = 0;
    handle->rx_line_state = DRV_UART_LINE_STATE_IDLE;
    ret = DRV_UART_ERROR_NONE;
  } else {
    // receive remaining data in IRQ handler
    handle->rx_data = buf;
    handle->rx_data_remaining_byte = length;
    handle->rx_data_total_byte = length;
    ret = DRV_UART_WAIT_CB;
  }

  DRV_UART_Enable_Interrupt(handle->base, RX_FULL | RX_TIMEOUT);
  return ret;
}
int32_t DRV_UART_Abort_Send(DRV_UART_Handle *handle) {
  UART_ASSERT(handle);
  size_t sent;
  DRV_UART_Disable_Interrupt(handle->base, TX_EMPTY);
  if (handle->tx_line_state == DRV_UART_LINE_STATE_IDLE) {
    sent = -1;
  } else {
    sent = DRV_UART_Get_Transmitted_Tx_Count(handle);
    handle->tx_data = NULL;
    handle->tx_data_remaining_byte = 0;
    handle->tx_data_total_byte = 0;
    handle->tx_data_length_requested = 0;
    handle->tx_line_state = DRV_UART_LINE_STATE_IDLE;
  }
  return sent;
}
int32_t DRV_UART_Abort_Receive(DRV_UART_Handle *handle) {
  UART_ASSERT(handle);

  size_t received;
  DRV_UART_Disable_Interrupt(handle->base, RX_FULL | RX_TIMEOUT);
  if (handle->rx_line_state == DRV_UART_LINE_STATE_IDLE) {
    received = -1;
  } else {
    received = DRV_UART_Get_Received_Rx_Count(handle);

    handle->rx_data = NULL;
    handle->rx_data_remaining_byte = 0;
    handle->rx_data_total_byte = 0;
    handle->rx_data_length_requested = 0;
    handle->rx_line_state = DRV_UART_LINE_STATE_IDLE;
  }

  DRV_UART_Enable_Interrupt(handle->base, RX_FULL | RX_TIMEOUT);
  return received;
}

DRV_UART_Status DRV_UART_Send_Nonblock(DRV_UART_Handle *handle, void *data, uint32_t length) {
  UART_ASSERT(data);
  UART_ASSERT(handle);
  int8_t *buf = (int8_t *)data;

  if (length == 0) {
    return DRV_UART_ERROR_PARAMETER;
  }
  if (handle->tx_line_state == DRV_UART_LINE_STATE_BUSY) {
    return DRV_UART_ERROR_BUSY;
  }
  handle->tx_line_state = DRV_UART_LINE_STATE_BUSY;
  handle->tx_data_length_requested = length;

  if (hifc_request_to_send(handle->base)) {
    handle->tx_line_state = DRV_UART_LINE_STATE_IDLE;
    return DRV_UART_ERROR_HIFC_TIMEOUT;
  }
  // in order to generate tx_empty interupt, we need to fill up fifo first
  while (handle->base->FR_b.TXFF == 0 && length != 0) {
    reset_inactivity_timer(handle->base);
    handle->base->DR = *buf;
    buf++;
    length--;
    handle->stat.tx_cnt++;
  }

  if (length != 0) {
    handle->tx_data = buf;
    handle->tx_data_remaining_byte = length;
    handle->tx_data_total_byte = length;

    DRV_UART_Enable_Interrupt(handle->base, TX_EMPTY);
    return DRV_UART_WAIT_CB;
  } else {
    handle->tx_data_remaining_byte = 0;
    handle->tx_data_total_byte = 0;
    handle->tx_line_state = DRV_UART_LINE_STATE_IDLE;
    return DRV_UART_ERROR_NONE;
  }
}

DRV_UART_Status DRV_UART_Send_Block(DRV_UART_Handle *handle, void *data, uint32_t length) {
  UART_ASSERT(data);
  UART_ASSERT(handle);

  int8_t *buf = (int8_t *)data;
  UART_Type *base = handle->base;
  /*
   * We don't check tx_line status here because we want to ensure printf
   * could print all characters even the output will mix up across tasks
   */
  if (length == 0) {
    return DRV_UART_ERROR_PARAMETER;
  }

  while (hifc_request_to_send(handle->base)) {
  }  // block until hifc goes up

  while (length) {
    while (base->FR_b.TXFF == 1)  // tx fifo has room
    {
    }
    reset_inactivity_timer(handle->base);
    base->DR = *buf;
    buf++;
    length--;
    handle->stat.tx_cnt++;
  }

  return DRV_UART_ERROR_NONE;
}

int32_t DRV_UART_Is_Uart_Busy(UART_Type *base) { return base->FR_b.BUSY; }

DRV_UART_Status DRV_UART_Get_Statistic(DRV_UART_Handle *handle, DRV_UART_Statistic *stat) {
  UART_ASSERT(handle);
  UART_ASSERT(stat);
  memcpy(stat, &handle->stat, sizeof(DRV_UART_Statistic));

  return DRV_UART_ERROR_NONE;
}
