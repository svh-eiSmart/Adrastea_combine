/*  ---------------------------------------------------------------------------

        (c) copyright 2017 Altair Semiconductor, Ltd. All rights reserved.

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

#include "FreeRTOS.h"
#include "task.h"
#include "portmacro.h"
#include "125X_mcu.h"
#include "serial.h"
#include "newlibPort.h"
#include "serial_altDbg.h"
#include "circ_buf.h"
#include "serial_container.h"
#include "hifc_api.h"
#include "if_mngr.h"

#define CONFIG_ACTIVE_UART_ADDR 0x8
#define UART_IDX_SHIFT_VAL 4

#define INPUT_QUEUE_LENGTH 2048
int uartIsrRxIndCnt[MAX_UART_NUM] = {0};
int uartIsrTxIndCnt[MAX_UART_NUM] = {0};
int uartRxDropDueToUnbindRxFunc[MAX_UART_NUM] = {0};
int uartIsrRxFifoFullCnt[MAX_UART_NUM] = {0};
#if (configUSE_ALT1250_SLEEP == 1)
volatile unsigned int last_uart_interupt = 0;
#endif
static void waitForIsrInd(void);

// static variables
static xQueueHandle xQueue_input = NULL;

// static function declarations
static void createUartIndQue(void);
// Init and options
static void hw_serial_setbrg(unsigned long int uartclk, unsigned long int baudrate,
                             serial_uart_ctxt_t *ptrCtxt);
static void hw_serial_init_helper(unsigned long int uartclk, serial_uart_ctxt_t *ptrCtxt);
static int hw_serial_putc_with_XmitBuf(const char c, volatile uint32_t *uart_base, void *xmit);
static unsigned int hw_serial_put_char(void *uart_func, const char c);
static unsigned int hw_serial_put_char_to_xmitBuf(void *uart_func, const char c);

int hw_serial_tx_busy0(void);
int hw_serial_tx_busy1(void);
#ifdef NOT_USED
static int hw_serial_getc(void *uart_base);
#endif
static int hw_serial_tstc(uint32_t *uart_base);
int hw_serial_tstc0();
int hw_serial_tstc1();

static inline void hw_uart_buffered_input_interrupt_handler(serial_uart_ctxt_t *uartCtxtPtr,
                                                            unsigned char uartIdx,
                                                            xQueueHandle queue);
typedef void (*interruptRegisterFp_t)(void);
void *uartHwIndxToCntxt[MAX_UART_NUM] = {NULL};

int uartCharRxCnt[MAX_UART_NUM] = {0};
int uartCharTxCnt[MAX_UART_NUM] = {0};
int uartCharRxframeErrCnt[MAX_UART_NUM] = {0};
int uartCharRxParityErrCnt[MAX_UART_NUM] = {0};
int uartCharRxBrkErrCnt[MAX_UART_NUM] = {0};
int uartCharRxOverrunErrCnt[MAX_UART_NUM] = {0};

enum hw_isr_indication {
  RX_UART_IND = 1,
  TX_UART_IND = 2,
};

/**
 * alt1250_uart_tx_empty - Returns whether TX FIFO is empty
 * @port: UART port instance
 */
#if 0
static unsigned int alt1250_uart_tx_empty(void *port)
{
	serial_uart_ctxt_t *ptrCtxt = (struct serial_uart_ctxt *)port;

	return (REGISTER(ptrCtxt->uart_base + OFFSET_UARTFR) & UART0_UARTFR_TXFE_MSK) ? 1 : 0;
}
#endif

#if 0
static int tty_smngr_busy_callback(void *port)
{
	printf("tty_smngr_busy_callback %s,smngr ID %d\n",(ptrCtxt->uartType == SERIAL_UART_TYPE_SLOW) ? "Slow" : "Fast",ptrCtxt->smngr_id);

	return (!alt1250_uart_tx_empty(port));
}
#endif

void hw_uart_interrupt_handler0(void) {
  hw_uart_buffered_input_interrupt_handler((uartHwIndxToCntxt[UART0]), UART0, xQueue_input);
}

#ifdef ALT1250
void hw_uart_interrupt_handler1(void) {
  hw_uart_buffered_input_interrupt_handler(
      (uartHwIndxToCntxt[UART1]), UART1,
      xQueue_input); /*Both serial uses the same queue to alert indications */
}
#endif

void hw_uart_interrupt_handleri0(void) {
  hw_uart_buffered_input_interrupt_handler((uartHwIndxToCntxt[UARTI0]), UARTI0, xQueue_input);
}

#if 0
void hw_uart_interrupt_handler2()
{
	hw_uart_buffered_input_interrupt_handler(((serial_uart_ctxt_t*)uartHwIndxToCntxt[UART2]), UART2,xQueue_input1
}
void hw_uart_interrupt_handler3()
{
	hw_uart_buffered_input_interrupt_handler(((serial_uart_ctxt_t*)uartHwIndxToCntxt[UART3]),UART3,xQueue_input1
}
#endif
static inline void enable_uart_interrupt_flow(uint32_t *uart_cfg) {
  REGISTER(uart_cfg + (MCU_UARTF0_CFG_EN_FLOW_INT - BASE_ADDRESS_MCU_UARTF0_CFG) /*0xBC*/) =
      MCU_UARTF0_CFG_EN_FLOW_INT_UARTINT_MSK;  // enable the interrupt flow
}

static void hw_uart_interrupt_register(serial_uart_ctxt_t *ptrCtxt, IRQn_Type interrupt_number,
                                       char rxOnly) {
  if (rxOnly) {
    print_boot_msg("Setup rx_interrupt_only for uart %d", ptrCtxt->uartHwIndx);
    // enable only rx interrupt
    REGISTER(ptrCtxt->uart_base + OFFSET_UARTIMSC) =
        /*UART_TXI_MSK|*/ UART_RXI_MSK |
        UART_RTI_MSK;  // set the mask to block all interrupts but accept receive interrupt
  } else {
    print_boot_msg("Setup rx_tx_interrupt_ for uart %d", ptrCtxt->uartHwIndx);
    REGISTER(ptrCtxt->uart_base + OFFSET_UARTIMSC) =
        UART_TXI_MSK | UART_RXI_MSK |
        UART_RTI_MSK;  // set the mask to block all interrupts but accept receive interrupt
  }
  //	register_interrupt(interrupt_number,input_interrupt_register_arr[ptrCtxt->uartHwIndx],"UART
  // buffered input interrupt");
  NVIC_SetPriority(interrupt_number, 7); /* set Interrupt priority */
  NVIC_EnableIRQ(interrupt_number);

  if (ptrCtxt->uart_cfg != NULL) {
    enable_uart_interrupt_flow(ptrCtxt->uart_cfg);
  }
}
static int isNotPowerOfTwoUtil(unsigned int x) { return ((x != 0) && !(x & (x - 1))); }

static int serialUartDriverInterruptRegister(void *handle) {
  struct serial_uart_ctxt *ptrCtxt = (struct serial_uart_ctxt *)handle;
  // Currently SLOW UART TX interrupt is not functioning
  if (ptrCtxt->TxBlockingMode == BLOCKING_MODE) {
    hw_uart_interrupt_register(ptrCtxt, ptrCtxt->interrupt_number, /*only Rx*/ 1);
  } else if (ptrCtxt->TxBlockingMode == NON_BLOCKING_MODE) {
    hw_uart_interrupt_register(ptrCtxt, ptrCtxt->interrupt_number, /*Rx & Tx*/ 0);
  } else {
    printf("%s failed, unknown blocking mode given %d\n", __FUNCTION__, ptrCtxt->TxBlockingMode);
    return 0;
  }
  return 1;
}

/* Set pin configuration + default baud rate */
void serialUartDriverHwInit(void *serialHandle) {
  eIfMngrRet ret;
  serial_uart_ctxt_t *ptrCtxt = (struct serial_uart_ctxt *)serialHandle;
  // set default Rx interrupt only (blocking Tx)
#if 0
	if (!serialUartDriverInterruptRegister(serialHandle)){
		printf("%s fail in calling to serialUartDriverInterruptRegister() !\n",__FUNCTION__);
		return ;
	}
#endif

  switch (ptrCtxt->uartHwIndx) {
    case UART0:
      ret = if_mngr_config_io(IF_MNGR_UARTF0);
      if (ret != IF_MNGR_SUCCESS) {
        /*printf("Failed to init UART0 IOs\r\n");*/
      }
      hw_serial_init_helper(SystemCoreClock, ptrCtxt);
      break;
#ifdef ALT1250
    case UART1:
      ret = if_mngr_config_io(IF_MNGR_UARTF1);
      if (ret != IF_MNGR_SUCCESS) {
        /*printf("Failed to init UART1 IOs\r\n");*/
      }
      hw_serial_init_helper(SystemCoreClock, ptrCtxt);
      break;
#endif
    case UARTI0:
    case UARTI1:
#ifdef ALT1250
    case UARTI2:
#endif
      hw_serial_init_helper(REF_CLK_26000000, ptrCtxt);
      break;
  }

  if (!serialUartDriverInterruptRegister(serialHandle)) {
    printf("%s fail in calling to serialUartDriverInterruptRegister() !\n", __FUNCTION__);
    return;
  }

#if 0
	smngr_register_dev_async(
			(ptrCtxt->uartType == SERIAL_UART_TYPE_SLOW) ? "SUART" : "FUART",
			(void *) tty_smngr_busy_callback,
			ptrCtxt,
			&ptrCtxt->smngr_id);
#endif
}

struct serial_uart_ctxt *serial_uart_get_ctxt(enum hwSer_channelNumber channel) {
  if (channel < MAX_UART_NUM) return (uartHwIndxToCntxt[channel]);
  return NULL;
}

static void createUartIndQue(void) {
  xQueue_input = xQueueCreate(INPUT_QUEUE_LENGTH, sizeof(char));
  configASSERT(xQueue_input);
}
/***********************************************************external API
 * code***************************************************************/
static void serial_rx_thread(void *arg) {
  while (1) {
    waitForIsrInd(); /*wait for recv chars*/
  }
}

/******************************************************************static functions
 * code****************************************************************************************************/

/**
 * @brief Configures UART hardware (in given address) with given baud rate
 */
static void hw_serial_setbrg(unsigned long int uartclk, unsigned long int baudrate,
                             serial_uart_ctxt_t *ptrCtxt) {
  int idiv, fdiv, fpres;
  uint32_t *uart_base = ptrCtxt->uart_base;
  fpres = ((uartclk << 3) / baudrate) + 1;
  idiv = (fpres >> 7);
  fdiv = (((fpres - (idiv << 7))) >> 1);

  REGISTER(uart_base + OFFSET_UARTIBRD) = idiv;
  REGISTER(uart_base + OFFSET_UARTFBRD) = fdiv;
}
static void hw_serial_init_helper(unsigned long int uartclk,
                                  /*unsigned long int baudrate,*/ serial_uart_ctxt_t *ptrCtxt) {
  uint32_t ifls;
  uint32_t *uart_base = ptrCtxt->uart_base;
  unsigned long int baudrate = ptrCtxt->uartInitParam.Init.BaudRate;
  sHuart *uartInitParam = &ptrCtxt->uartInitParam;
  unsigned int wordLength = (unsigned int)uartInitParam->Init.WordLength;
  unsigned int flowCtrl = (unsigned int)uartInitParam->Init.HwFlowCtl;
  eParity parity = uartInitParam->Init.Parity;
  unsigned int stopBit = (unsigned int)uartInitParam->Init.StopBits;

  // disable uart
  REGISTER(uart_base + OFFSET_UARTCR) = REGISTER(uart_base + OFFSET_UARTCR) & ~UARTCR_UARTEN_MSK;
  // disable fifo
  REGISTER(uart_base + OFFSET_UARTLCR_H) =
      REGISTER(uart_base + OFFSET_UARTLCR_H) & ~UART_LCR_H_FIFO_EN_MASK;

  // set baud rate
  hw_serial_setbrg(uartclk, baudrate, ptrCtxt);

  // set data bits
  REGISTER(uart_base + OFFSET_UARTLCR_H) =
      REGISTER(uart_base + OFFSET_UARTLCR_H) & ~(UART_LCR_H_8BIT_DATA_MASK);
  REGISTER(uart_base + OFFSET_UARTLCR_H) =
      REGISTER(uart_base + OFFSET_UARTLCR_H) |
      (UART_LCR_H_8BIT_DATA_MASK & (wordLength << UART_LCR_H_8BIT_DATA_POS));

  // set HW flow control
  REGISTER(uart_base + OFFSET_UARTCR) =
      REGISTER(uart_base + OFFSET_UARTCR) & ~(UARTCR_RTSEN_MSK | UARTCR_CTSEN_MSK);
  REGISTER(uart_base + OFFSET_UARTCR) =
      REGISTER(uart_base + OFFSET_UARTCR) |
      ((UARTCR_RTSEN_MSK | UARTCR_CTSEN_MSK) & (flowCtrl << UARTCR_RTSEN_POS));

  // set parity
  switch (parity) {
    case SERIAL_PARITY_MODE_ZERO:
      REGISTER(uart_base + OFFSET_UARTLCR_H) =
          REGISTER(uart_base + OFFSET_UARTLCR_H) |
          (UART_LCR_H_PEN_MASK | UART_LCR_H_EPS_MASK | UART_LCR_H_SPS_MASK);
      break;
    case SERIAL_PARITY_MODE_ONE:
      REGISTER(uart_base + OFFSET_UARTLCR_H) =
          REGISTER(uart_base + OFFSET_UARTLCR_H) | (UART_LCR_H_PEN_MASK | UART_LCR_H_SPS_MASK);
      REGISTER(uart_base + OFFSET_UARTLCR_H) =
          REGISTER(uart_base + OFFSET_UARTLCR_H) & ~(UART_LCR_H_EPS_MASK);
      break;
    case SERIAL_PARITY_MODE_ODD:
      REGISTER(uart_base + OFFSET_UARTLCR_H) =
          REGISTER(uart_base + OFFSET_UARTLCR_H) | (UART_LCR_H_PEN_MASK);
      REGISTER(uart_base + OFFSET_UARTLCR_H) =
          REGISTER(uart_base + OFFSET_UARTLCR_H) & ~(UART_LCR_H_EPS_MASK | UART_LCR_H_SPS_MASK);
      break;
    case SERIAL_PARITY_MODE_EVEN:
      REGISTER(uart_base + OFFSET_UARTLCR_H) =
          REGISTER(uart_base + OFFSET_UARTLCR_H) | (UART_LCR_H_PEN_MASK | UART_LCR_H_EPS_MASK);
      REGISTER(uart_base + OFFSET_UARTLCR_H) =
          REGISTER(uart_base + OFFSET_UARTLCR_H) & ~(UART_LCR_H_SPS_MASK);
      break;
    case SERIAL_PARITY_MODE_DISABLE:
      REGISTER(uart_base + OFFSET_UARTLCR_H) =
          REGISTER(uart_base + OFFSET_UARTLCR_H) & ~(UART_LCR_H_PEN_MASK);
      break;
  }

  // set stop bit
  REGISTER(uart_base + OFFSET_UARTLCR_H) =
      REGISTER(uart_base + OFFSET_UARTLCR_H) & ~(UART_LCR_H_STP2_MASK);
  REGISTER(uart_base + OFFSET_UARTLCR_H) =
      REGISTER(uart_base + OFFSET_UARTLCR_H) |
      (UART_LCR_H_STP2_MASK & (stopBit << UART_LCR_H_STP2_POS));

  /*set up rx fifo interrupt level */
  ifls = REGISTER(uart_base + OFFSET_UARTIFLS);
  ifls &= ~UART0_UARTIFLS_RXIFLSEL_MSK;
  ifls |= UART_RX_INTR_LEVEL;
  REGISTER(uart_base + OFFSET_UARTIFLS) = ifls;

  /*set up tx fifo interrupt level */
  ifls = REGISTER(uart_base + OFFSET_UARTIFLS);
  ifls &= ~UART0_UARTIFLS_TXIFLSEL_MSK;
  ifls |= UART_TX_INTR_LEVEL;
  REGISTER(uart_base + OFFSET_UARTIFLS) = ifls;

  // enable fifo
  REGISTER(uart_base + OFFSET_UARTLCR_H) =
      REGISTER(uart_base + OFFSET_UARTLCR_H) | (UART_LCR_H_FIFO_EN_MASK);
  // enable uart
  REGISTER(uart_base + OFFSET_UARTCR) =
      REGISTER(uart_base + OFFSET_UARTCR) |
      (UART_LCR_TX_EN_MASK | UART_LCR_RX_EN_MASK | UARTCR_UARTEN_MSK);
}
/**
 * @brief Function used by APP layer to Writes a character into UART
 *        It would put the character to tx circ buffer unless UART FIFO is empty.
 *        If UART FIFO is empty, it woudld write the character to HW fifo till
 *        it becomes full and then write to circ buffer.
 * @param c Character to write to UART FIFO
 */
static int hw_serial_putc_with_XmitBuf(const char c, volatile uint32_t *uart_base, void *xmit) {
  int ret = 0;
  int byte_sent = 0;
  //	int	in_task_ctx = (uxInterruptNesting == 0);

  //	if (in_task_ctx)
  {
    taskENTER_CRITICAL();
    if (serialCircBufIsdata(xmit)) {
      if (!(REGISTER(uart_base + OFFSET_UARTFR) & UART_FR_FIFO_FULL_MASK)) { /* fifo is not full */
        REGISTER(uart_base + OFFSET_UARTDR) = c;
        byte_sent = 1;
        ret = 1;
      }
    }
    if (byte_sent == 0) {
      /* queue is not empty so add to it */
      if (serialCircBufFree(xmit)) { /* check if space available*/
        serialCircBufInsert(xmit, c);
        ret = 1;
      } else {
        ret = 0;
      }
    }
    taskEXIT_CRITICAL();
  }

  if (ret == 0) {
    printf("drop it since xmit queue is fully occupied and also the uart fifo\n");
  }
  return ret;
}
static inline unsigned int hw_serial_putc_none_blocking(uint32_t *uart_base, const char c) {
  unsigned int txCnt = 0;
  while (!(REGISTER(uart_base + OFFSET_UARTFR) & UART_FR_FIFO_FULL_MASK)) {
    REGISTER(uart_base + OFFSET_UARTDR) = (c & UARTDR_DATA_MSK);
    txCnt++;
  }
  return txCnt;
}
static inline unsigned int hw_serial_putc_blocking(uint32_t *uart_base, unsigned char uartHwIndx,
                                                   const char c) {
  while (REGISTER(uart_base + OFFSET_UARTFR) & UART_FR_FIFO_FULL_MASK)
    ;
  REGISTER(uart_base + OFFSET_UARTDR) = (c & UARTDR_DATA_MSK);
  uartCharTxCnt[uartHwIndx]++;
  return 1;
}

static unsigned int hw_serial_put_char(void *uart_func, const char c) {
  void *uart_base = ((struct serial_uart_ctxt *)uart_func)->uart_base;
  unsigned char uartHwIndx = ((struct serial_uart_ctxt *)uart_func)->uartHwIndx;

  if (((struct serial_uart_ctxt *)uart_func)->TxBlockingMode == BLOCKING_MODE)
    return hw_serial_putc_blocking((void *)uart_base, uartHwIndx, c);
  else
    return hw_serial_putc_none_blocking((void *)uart_base, c);
}

static unsigned int hw_serial_put_char_to_xmitBuf(void *uart_func, const char c) {
  void *xmit = ((struct serial_uart_ctxt *)uart_func)->xmitBuffer;
  void *uart_base = ((struct serial_uart_ctxt *)uart_func)->uart_base;

  return hw_serial_putc_with_XmitBuf(c, (void *)uart_base, xmit);
}

/**
 * @brief Test whether there is any data in TX fifo
 * @return Returns whether there's data in UART TX FIFO
 * @retval 0 UART TX FIFO is empty
 * @retval 1 UART TX FIFO is not empty
 */
static int hw_serial_tx_busy(uint32_t *uart_base) {
  return ((REGISTER(uart_base + OFFSET_UARTFR) & UART_FR_TX_BUSY_MASK) != 0);
}
int hw_serial_tx_busy0(void) { return hw_serial_tx_busy((uint32_t *)BASE_ADDRESS_MCU_UARTF0); }
#ifdef BASE_ADDRESS_MCU_UARTF1
int hw_serial_tx_busy1(void) { return hw_serial_tx_busy((uint32_t *)BASE_ADDRESS_MCU_UARTF1); }
#endif
int is_serial_tx_busy(uint32_t *uart_base) { return hw_serial_tx_busy(uart_base); }

#ifdef NOTUSED
/**
 * @brief Read a character from UART RX FIFO
 * @return A single character from UART FIFO
 * If there is no input block until there is character to read.
 */
static int hw_serial_getc(void *uart_base) {
  int c;
  while (readl(uart_base + OFFSET_UARTFR) & UART_FR_RX_EMPTY_MASK)
    ;
  c = readl(uart_base + OFFSET_UARTDR) & UARTDR_DATA_MSK;
  return c;

#if 0
		//clear errors is any
		if (REGISTER(UART0_UARTRSR) & UART_RSR_ERROR_MASK) {
			REGISTER(UART0_UARTECR) = 0x0;
		}
		return c;
#endif
}
#endif
/**
 * @brief Test whether there is any data in RX fifo
 * @return Returns whether there's data in UART RX FIFO
 * @retval 0 UART RX FIFO is empty
 * @retval 1 UART RX FIFO is not empty
 */
inline int hw_serial_tstc(uint32_t *uart_base) {
  return (REGISTER(uart_base + OFFSET_UARTFR) & UART_FR_RX_EMPTY_MASK) ? 0 : 1;
}
int hw_serial_tstc0() { return hw_serial_tstc((uint32_t *)BASE_ADDRESS_MCU_UARTF0); }
#ifdef BASE_ADDRESS_MCU_UARTF1
int hw_serial_tstc1() { return hw_serial_tstc((void *)BASE_ADDRESS_MCU_UARTF1); }
#endif

/*********************************************************************************************************************/
static void throttle(serial_uart_ctxt_t *uartCtxtPtr) {
  uint32_t *uart_base = uartCtxtPtr->uart_base;
  eHwFlowCtl hwflow = uartCtxtPtr->uartInitParam.Init.HwFlowCtl;

  if (hwflow & UART_HWCONTROL_RTS) {
    // disable hw flow control and deassert RTS
    REGISTER(uart_base + OFFSET_UARTCR) =
        (REGISTER(uart_base + OFFSET_UARTCR) & ~(UARTCR_RTSEN_MSK | UARTCR_RTS_MSK));
  }
}

static void unthrottle(serial_uart_ctxt_t *uartCtxtPtr) {
  uint32_t *uart_base = uartCtxtPtr->uart_base;
  eHwFlowCtl hwflow = uartCtxtPtr->uartInitParam.Init.HwFlowCtl;

  if (hwflow & UART_HWCONTROL_RTS) {
    // enable hw flow control and assert RTS
    REGISTER(uart_base + OFFSET_UARTCR) =
        (REGISTER(uart_base + OFFSET_UARTCR) | (UARTCR_RTSEN_MSK | UARTCR_RTS_MSK));
  }
}

static int processSerialRxGetChar(serial_uart_ctxt_t *uartCtxtPtr, uint8_t *charRecv) {
  void *rcvBuff = uartCtxtPtr->recvBuffer;

  if (!serialCircBufIsdata(rcvBuff)) {
    serialCircBufGetChar(rcvBuff, charRecv);
    if (serialCircBufGetRoom(rcvBuff) > 16) {
      unthrottle(uartCtxtPtr);
    }
    return 1;
  }
  return 0;
}

static int putCharOnSwRecvBuff(serial_uart_ctxt_t *uartCtxtPtr, uint8_t charRecv) {
  void *rcvBuff = uartCtxtPtr->recvBuffer;
  if (serialCircBufFree(rcvBuff)) {
    serialCircBufInsert(rcvBuff, charRecv);
    if (serialCircBufGetRoom(rcvBuff) <= 16)  // there are 16 bytes in the hardware FIFO. Throttle
                                              // in advance in case hardware FIFO is full.
      throttle(uartCtxtPtr);

    return 1;
  }
  return 0;
}

static int processSerialRxIsrInd(serial_uart_ctxt_t *uartCtxtPtr, uint8_t *charRecv) {
  if (hw_serial_tstc(uartCtxtPtr->uart_base)) /*Rx hw fifo is not empty */
  {
    uartCharRxCnt[uartCtxtPtr->uartHwIndx]++;
    *charRecv = REGISTER(uartCtxtPtr->uart_base + OFFSET_UARTDR);
    return 1;
  }

  return 0; /*Nothing was received */
}

/**
 * Write to uart tx fifo from xmit circ buf till tx fifo is  full.
 *
 * @param uartDataReg- address of the uart data register
 * @return 0 - data from xmit circ buf was sent.
 *         1- No data was transmitted.
 */
static int writeFromXmitCircBuf(void *xmitBufPtr, volatile void *uartDataReg,
                                volatile void *uart_status_reg) {
  if (xmitBufPtr == NULL) return 1;     /*No data transmitted */
  if (serialCircBufIsdata(xmitBufPtr))  //(serialCircBufIsdata(xmitBufPtr))
    return 1;                           /*No data */

  serialCircGetCharAndCopy(xmitBufPtr, uartDataReg);

  return 0; /*data was sent */
}

static int processSerialTxIsrInd(uint32_t *uart_base, void *xmitBuffer, int maxXmitTimes) {
  void *uartDataReg = NULL;
  int writeToXmit = 0;

  void *uart_status_reg = NULL;

  /*Check circ queue and try to send all it's content to fifo tx */
  uartDataReg = (uart_base + OFFSET_UARTDR);
  uart_status_reg = (uart_base + OFFSET_UARTFR);
  writeToXmit = 0;

  while ((maxXmitTimes > writeToXmit) && (!(REGISTER(uart_status_reg) & UART_FR_FIFO_FULL_MASK))) {
    if (writeFromXmitCircBuf(xmitBuffer, uartDataReg, uart_status_reg))
      break; /*No more char in xmit queue*/
    writeToXmit++;
  }
  return writeToXmit;
}
/**
 * Wait for indications coming from serial isr. Indication can be rx or tx indication for one or the
 * UARTs
 *
 * @param dataRecvWaitFlag - bool flag indicating if to wait on recv queue for new rx/tx
 *                           indication to coming from serial isr.
 */
static void waitForIsrInd(void) {
  static uint8_t indication;
  uint8_t charRecv;
  enum hwSer_channelNumber uartIdx;

  /* Wait until something arrives in the queue - this task will block indefinitely */

  while (1) {
    if (xQueueReceive(xQueue_input, &indication, portMAX_DELAY) == pdTRUE) break;
  }

  uartIdx = (enum hwSer_channelNumber)(indication >> UART_IDX_SHIFT_VAL);
  // printf("IsrInd hwidx:%d Q:%p\r\n",uartIdx,xQueue_input);
  if (uartIdx >= MAX_UART_NUM) {
    printf("%s - recv indication with indication=0x%x uartIdx %d > MAX_UART_NUM\n", __FUNCTION__,
           indication, uartIdx);
    return;
  }

  if (indication & RX_UART_IND) {
    struct serial_uart_ctxt *serialFuncIfc;
    uartIsrRxIndCnt[uartIdx]++;

    serialFuncIfc = serial_uart_get_ctxt(uartIdx);
    // if(serialFuncIfc->uartHwIndx !=0)

    while (processSerialRxGetChar(serialFuncIfc, &charRecv)) {
      if (serialFuncIfc->serialRxProcess)
        serialFuncIfc->serialRxProcess(charRecv, serialFuncIfc->rxProcessCookie);
      else
        uartRxDropDueToUnbindRxFunc[uartIdx]++;
    }
  }
}
void debugPrintIsr(xQueueHandle queue, unsigned char indication, unsigned char idx) {
  printf("debug debugPrintIsr() uartnum=%d xQueue handle=%p indication=0x%x\n", idx, queue,
         indication);
}

static void read_uart_interrupt_status(serial_uart_ctxt_t *uartCtxtPtr, unsigned char uartIdx) {
  unsigned int status;
  uint32_t *uart_base;
  uart_base = uartCtxtPtr->uart_base;

  status = REGISTER(uart_base + OFFSET_UARTRSR) & 0xf;

  if (status) {
    if (status & UART0_UARTRSR_FE_MSK) {
      uartCharRxframeErrCnt[uartIdx]++;
    }
    if (status & UART0_UARTRSR_PE_MSK) {
      uartCharRxParityErrCnt[uartIdx]++;
    }
    if (status & UART0_UARTRSR_BE_MSK) {
      uartCharRxBrkErrCnt[uartIdx]++;
    }
    if (status & UART0_UARTRSR_OE_MSK) uartCharRxOverrunErrCnt[uartIdx]++;

    /*clear all errors */
    REGISTER(uart_base + OFFSET_UARTECR) = ~0;
  }
}

// interrupt handling- buffering input. Read from the FIFO of uart chip in address uart_base to
// queue in address queue.
static inline void hw_uart_buffered_input_interrupt_handler(serial_uart_ctxt_t *uartCtxtPtr,
                                                            unsigned char uartIdx,
                                                            xQueueHandle queue) {
  BaseType_t higherPriorityTaskWoken = pdFALSE;
  unsigned char indication = 0;
  uint32_t isr;
  uint8_t charRecv;
  // uint32_t priodicReadBytes=0;
  uint32_t *uart_base;
  uart_base = uartCtxtPtr->uart_base;

#define MAX_WRITE_CHAR_TO_TX_FIFO \
  10 /* ALL tX FIFO IS 16 BYTE LEN SO 10 IS GOOD SINCE IT HIT THE THRESHOLD */

  /* Read Masked Interrupt Status register */
  isr = REGISTER(uart_base + OFFSET_UARTMIS);
  if (isr & (UART_RXI_MSK | UART_RTI_MSK)) {
    indication |= RX_UART_IND;
  }
  read_uart_interrupt_status(uartCtxtPtr, uartIdx);
  if (isr & UART_TXI_MSK) {
    // Tx indication is dealt here in ISR context
    /* clear Tx interrupt */
    REGISTER(uart_base + OFFSET_UARTICR) = UART0_UARTICR_TXIC_MSK;

    /*Check circ queue and try to send all it's content to fifo txup to  */
    uartCharTxCnt[uartIdx] += processSerialTxIsrInd(uart_base, uartCtxtPtr->xmitBuffer,
                                                    MAX_WRITE_CHAR_TO_TX_FIFO /* maxXmitTimes */);
    uartIsrTxIndCnt[uartIdx]++;
  }

  // debugPrintIsr(queue,indication,idx);
  /* Collect possible errors for statistics */
  /*
          status = readl(uart_base + OFFSET_UARTRSR) & 0xf;

          if (status) {
                  if (status & UART0_UARTRSR_FE_MSK) {
                          uartCharRxframeErrCnt[uartIdx]++;
                  }
                  if (status & UART0_UARTRSR_PE_MSK) {
                          uartCharRxParityErrCnt[uartIdx]++;
                  }
                  if (status & UART0_UARTRSR_BE_MSK) {
                          uartCharRxBrkErrCnt[uartIdx]++;
                  }
                  if (status & UART0_UARTRSR_OE_MSK)
                          uartCharRxOverrunErrCnt[uartIdx]++;

                  writel(~0, uart_base + OFFSET_UARTECR);
          }
  */

  if (indication & RX_UART_IND) {
    reset_inactivity_timer(uart_base);
    while (processSerialRxIsrInd(uartCtxtPtr, &charRecv)) {
      /*For inactivity timer - only CLI/Console UART should use this
       * TODO: use dynamic machanizim as CLI might not be on UART0*/
#if (configUSE_ALT1250_SLEEP == 1)
      if (uartCtxtPtr->uartHwIndx == UART0) {
        volatile unsigned int now = xTaskGetTickCount() / portTICK_PERIOD_MS;
        last_uart_interupt = now;
      }
#endif
      // data will lost if buff is full
      if (putCharOnSwRecvBuff(uartCtxtPtr, charRecv) == 0) {
        uartIsrRxFifoFullCnt[uartIdx]++;
      }

      uartIsrRxIndCnt[uartIdx]++;
    }

    /*clear RX interrupt */
    REGISTER(uart_base + OFFSET_UARTICR) = (UART0_UARTICR_RXIC_MSK | UART0_UARTICR_RTIC_MSK);
  }

  if (indication) {
    // Add the uartIdx to indication sent
    indication |= uartIdx << UART_IDX_SHIFT_VAL;
    // send the indication to rx thread
    xQueueSendToBackFromISR(queue, &indication, &higherPriorityTaskWoken);
  }

  /* Actual macro used here is port specific. */
  portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

int serialUartDriverIoctl(void *handle, serial_ioctl_id_t ioctlId, uint32_t *param,
                          void *appCookie) {
  serial_uart_ctxt_t *ptrCtxt = (struct serial_uart_ctxt *)handle;
  int rc = 1;

  switch (ioctlId) {
    case IOCTAL_SERIAL_SW_RX_FIFO:
      ptrCtxt->recvBuffer = NULL;
      if (!isNotPowerOfTwoUtil(*(unsigned int *)param)) {
        printf("%s - Error given RX fifo size %d is NOT power of 2!\n", __FUNCTION__,
               *(unsigned int *)param);
        rc = 0;
        break;
      }
      // 1st deallocate the current recv buffer
      if (ptrCtxt->recvBuffer) {
        serialFreeCircBufUtil(ptrCtxt->recvBuffer);
      }
      rc = serialAllocCircBufUtil(&ptrCtxt->recvBuffer, *(unsigned int *)param);
      break;

    case IOCTAL_SERIAL_SW_TX_FIFO:
      ptrCtxt->xmitBuffer = NULL;
      if (!isNotPowerOfTwoUtil(*(unsigned int *)param)) {
        printf("%s - Error given TX fifo size %d is NOT power of 2!\n", __FUNCTION__,
               *(unsigned int *)param);
        rc = 0;
        break;
      }
      if (ptrCtxt->TxBlockingMode == BLOCKING_MODE) {
        printf("%s - ignore TX fifo settings since UART is set with tx blocking mode!\n",
               __FUNCTION__);
        rc = 0;
        break;
      }
      rc = serialAllocCircBufUtil(&ptrCtxt->xmitBuffer, *(unsigned int *)param);
      if (rc) ptrCtxt->serial_putc = hw_serial_put_char_to_xmitBuf;
      break;
    case IOCTAL_SERIAL_RX_CB_FUNC:
      ptrCtxt->serialRxProcess = (serialRxProcessFp_t)param;
      ptrCtxt->rxProcessCookie = appCookie;
      break;
    case IOCTAL_SERIAL_TX_FUNC_PTR:
      param = (void *)&ptrCtxt->serial_putc;
      break;
    case IOCTAL_SERIAL_HW_FLOW_CNTRL:
      ptrCtxt->uartInitParam.Init.HwFlowCtl = (eHwFlowCtl)param;
      if (ptrCtxt->uartHwIndx < UARTI0)
        hw_serial_init_helper(SystemCoreClock, ptrCtxt);
      else
        hw_serial_init_helper(REF_CLK_26000000, ptrCtxt);
      //		hw_serial_set_hw_flow_control(ptrCtxt,(enum hwSer_hw_flow_mode) *(unsigned
      // int *)param);
      break;
    case IOCTAL_SERIAL_HW_FLOW_QUERY:
      *(unsigned int *)param = ptrCtxt->uartInitParam.Init.HwFlowCtl;
      break;
    case IOCTAL_SERIAL_TX_MODE_CNTRL:
      if ((*(unsigned int *)param == NON_BLOCKING_MODE) ||
          (*(unsigned int *)param == BLOCKING_MODE)) {
        // disregard tx fifo settings if blocking mode is set since SW Tx fifo shouldn't be used
        if (ptrCtxt->TxBlockingMode != *(unsigned int *)param) {
          ptrCtxt->TxBlockingMode = *(enum hwSer_tx_mode *)param;
          // if blocking mode is changed need to update rx & tx interrupt registration
          if (!serialUartDriverInterruptRegister(ptrCtxt)) {
            printf("%s fail in calling to serialUartDriverInterruptRegister() !\n", __FUNCTION__);
            return 0;
          }
        }
      } else {
        printf("%s -Error TX blocking %d is not valid \n", __FUNCTION__, *(unsigned int *)param);
        rc = 0;
        break;
      }
      break;

    case IOCTAL_SERIAL_UART_BAUD_RATE:
      ptrCtxt->uartInitParam.Init.BaudRate = (eBaudrate) * (unsigned long int *)param;
      if (ptrCtxt->uartHwIndx < UARTI0)
        hw_serial_init_helper(SystemCoreClock, ptrCtxt);
      else
        hw_serial_init_helper(REF_CLK_26000000, ptrCtxt);
      break;

    default:
      break;
  }
  return rc;
}

void serialUartDriver_createRxTask(void) {
#define SERIAL_RX_TASK_PRIORITY (tskIDLE_PRIORITY + 3)
  static xTaskHandle serialRxTaskHandle = NULL;

  if (serialRxTaskHandle == NULL) {
    createUartIndQue();
    /*Serial logical mngr init */
    // serialDriverContainerInit(SERIAL_TYPE_UART_ID, MAX_PHYSICAL_UART_NUM);
    xTaskCreate(serial_rx_thread, (const char *)"serial_rx_thread", configMINIMAL_STACK_SIZE,
                (void *)NULL, SERIAL_RX_TASK_PRIORITY, &serialRxTaskHandle);
  }
}

int serialUartDriverWrite(void *serialHandle, const char *txCharBuf, uint32_t txCharBufLen) {
  serial_uart_ctxt_t *ptrCtxt = (struct serial_uart_ctxt *)serialHandle;
  uint32_t idx;
  uint32_t *uart_base = ptrCtxt->uart_base;
  uint32_t five_ms = 5 / portTICK_PERIOD_MS;

  while (request_to_send(uart_base) != hifc_req_success) vTaskDelay(five_ms);

  for (idx = 0; idx < txCharBufLen; idx++) {
    reset_inactivity_timer(uart_base);
    if (!ptrCtxt->serial_putc(ptrCtxt, txCharBuf[idx])) break;
  }
  return idx;
}

int serialUartDriverDeallocInstance(void *handle) {
  if (!handle) return -1;

  serial_uart_ctxt_t *sHandle = (serial_uart_ctxt_t *)handle;
  uint32_t idx;

  NVIC_DisableIRQ(sHandle->interrupt_number);
  idx = sHandle->uartHwIndx;
  serialFreeCircBufUtil(sHandle->recvBuffer);
  uartHwIndxToCntxt[idx] = NULL;
  delete_rxQueue(sHandle->xQueue_rx);

  vPortFree(handle);

  return 0;
}

void *serialUartDriverAllocInstance(void *uart_base, void *uart_cfg,
                                    enum hwSer_channelNumber uartHwIndx,
                                    serial_uart_chan_t uartType, IRQn_Type interrupt_number,
                                    sHuart *uartInitParam) {
#define RECV_BUFF_DEFAULT_SIZE 2048

  if (uartHwIndx >= MAX_UART_NUM) {
    return NULL;
  }
  if (uartHwIndxToCntxt[uartHwIndx] != NULL)  // had initialized
    return uartHwIndxToCntxt[uartHwIndx];

  struct serial_uart_ctxt *ptrCtxt = pvPortMalloc(sizeof(struct serial_uart_ctxt));
  if (!ptrCtxt) return NULL;

  memset(ptrCtxt, (int)NULL, sizeof(struct serial_uart_ctxt));
  ptrCtxt->interrupt_number = interrupt_number;
  ptrCtxt->uartType = uartType;
  ptrCtxt->uart_base = uart_base;
  ptrCtxt->uart_cfg = uart_cfg;
  ptrCtxt->uartHwIndx = uartHwIndx;
  ptrCtxt->TxBlockingMode = BLOCKING_MODE;
  ptrCtxt->xQueue_rx = init_rxQueue();
  memcpy(&ptrCtxt->uartInitParam, uartInitParam, sizeof(sHuart));

  ptrCtxt->serial_putc = hw_serial_put_char;

  // Alloc base minimum recv buffer
  if (!serialAllocCircBufUtil(&ptrCtxt->recvBuffer, (unsigned int)RECV_BUFF_DEFAULT_SIZE)) {
    vQueueDelete(ptrCtxt->xQueue_rx);
    vPortFree(ptrCtxt);
    ptrCtxt = NULL;
    return NULL;
  }

  uartHwIndxToCntxt[uartHwIndx] = ptrCtxt;
  return ptrCtxt;
}
