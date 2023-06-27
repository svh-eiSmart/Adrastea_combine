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

#ifndef SERIAL_ALT125X_H_
#define SERIAL_ALT125X_H_

//#include <pm/sleep_mngr.h>
#include "125X_mcu.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "serialMngrLogPort.h"
#include "serial_container.h"

// clang-format off
/* UART0 and uart1 bases are defined in external file for CPU */
#define OFFSET_UARTDR       0x00    /* Data register */
#define OFFSET_UARTRSR      0x04    /* Receive status register */
#define OFFSET_UARTECR      0x04    /* error clear register */
#define OFFSET_UARTFR       0x18    /* Flags register */
#define OFFSET_UARTILPR     0x20    /* IrDA low-power counter register. This is an 8-bit read/write register that stores the low-power counter divisor value used to generate the IrLPBaud16 signal by dividing down of UARTCLK*/
#define OFFSET_UARTIBRD     0x24    /* integer part of the baud rate divisor value. Baud rate divisor BAUDDIV = (FUARTCLK/ {16 * Baud rate}) where FUARTCLK is the UART reference clock frequency. The BAUDDIV is comprised of the integer value (BAUD DIVINT) and the fractional value (BAUD DIVFRAC). */
#define OFFSET_UARTFBRD     0x28    /* fractional part of the baud rate divisor value Baud rate divisor BAUDDIV = (FUARTCLK/ {16 * Baud rate}) where FUARTCLK is the UART reference clock frequency. The BAUDDIV is comprised of the integer value (BAUD DIVINT) and the fractional value (BAUD DIVFRAC).*/
#define OFFSET_UARTLCR_H    0x2c    /* Accesses bits 29 to 22 of UARTLCR. #fixme what is UARTLCR */
#define OFFSET_UARTCR       0x30    /* UART control register */
#define OFFSET_UARTIFLS     0x34    /* Interrupt FIFO levels */
#define OFFSET_UARTIMSC     0x38    /* Interrupt mask register */ /*The interrupt mask is anded with raw interrupt status and create the masked status*/
#define OFFSET_UARTRIS      0x3c    /* raw interrupt status register*/
#define OFFSET_UARTMIS      0x40    /* Masked interrupt status register */
#define OFFSET_UARTICR      0x44    /* Interrupt clear register */

// masks
// masks to read the relevant part of register
#define UARTDR_DATA_MSK             (0xff)
#define UART_RSR_ERROR_MASK         (0x000f)
// flags register mask
#define UART_FR_TX_BUSY_MASK        (0x0008)
#define UART_FR_FIFO_FULL_MASK      (0x0020)
#define UART_FR_RX_EMPTY_MASK       (0x0010)
// UARTCR (control register) masks
#define UARTCR_UARTEN_MSK           (0x01)  // UART enable mask
#define UARTCR_RTSEN_MSK            (0x04000)
#define UARTCR_RTSEN_POS            (14)
#define UARTCR_CTSEN_MSK            (0x08000)
#define UARTCR_CTSEN_POS            (15)
#define UARTCR_RTS_MSK              (0x800)
#define UARTCR_RTS_POS              (11)
// UART_LCR_H control masks
#define UART_LCR_H_FIFO_EN_MASK     (0x0010)
#define UART_LCR_H_8BIT_DATA_MASK   (0x0060)  // set 8 the length of frame to 8 bits
#define UART_LCR_H_8BIT_DATA_POS    (5)
#define UART_LCR_H_PEN_MASK         (0x0002)
#define UART_LCR_H_EPS_MASK         (0x0004)
#define UART_LCR_H_SPS_MASK         (0x0080)
#define UART_LCR_H_STP2_MASK        (0x0008)
#define UART_LCR_H_STP2_POS         (3)

// UART_LCR control masks ????????????????????????#FIXME where is this register?
#define UART_LCR_UART_EN_MASK       (0x0001)
#define UART_LCR_TX_EN_MASK         (0x0100)
#define UART_LCR_RX_EN_MASK         (0x0200)
#define UART_LCR_DTR_MASK           (0x0400)
#define UART_LCR_RTS_MASK           (0x0800)

// interrupts masks (FOR UARTIMSC, UARTRIS, UARTMIS AND UARTICR)
#define UART_RXI_MSK                (0x0010)  // Receive interrupt. Created when FIFO got data beyond the set threshold
#define UART_TXI_MSK                (0x0020)  // transmit interrupt.
#define UART_RTI_MSK                (0x0040)  // Receive timeout interrupt. Created when data received and not read for more than set time
#define UART_FEI_MSK                (0x0080)  // Framing error interrupt.
#define UART_PEI_MSK                (0x0100)  // Parity error interrupt.
#define UART_BEI_MSK                (0x0200)  // Break error interrupt.
#define UART_OEI_MSK                (0x0400)  // Overrun error interrupt.

//Do we need also definitions for uart0? - for uart0 (i.e. slow uart) it's done by the u-boot
#ifdef CONFIG_ALT1250
#ifdef CONFIG_ALT1250B
#define BASE_ADDRESS_IO_SEL      BASE_ADDRESS_SOC_IO_FUNC_SEL
#define BASE_ADDRESS_IO_CFG      BASE_ADDRESS_GPM_IO_CFG
#define UARTS_IO_SEL_UARTF_RX   (BASE_ADDRESS_IO_SEL + 0x94)   /* uart1_rxd/uart3_rxd - UART2_RX */
#define UARTS_IO_SEL_UARTF_TX   (BASE_ADDRESS_IO_SEL + 0x98)   /* uart1_txd/uart3_txd - UART2_TX */
#endif
#if (CONFIG_FAST_UART_PORT == (0))
#ifdef CONFIG_ALT1250B
  #define UART1_IO_SEL_UARTF_RX   (BASE_ADDRESS_IO_SEL + 0x84)   /* uart0_rxd - UART0_RX */
  #define UART1_IO_SEL_UARTF_TX   (BASE_ADDRESS_IO_SEL + 0x88)   /* uart0_txd - UART0_TX */
#else
  #define UART1_IO_SEL_UARTF_RX   (BASE_ADDRESS_IO_SEL + 0x8C)   /* uart1_rxd - UART0_RX */
  #define UART1_IO_SEL_UARTF_TX   (BASE_ADDRESS_IO_SEL + 0x90)   /* uart1_txd - UART0_TX */
#endif
#else
#ifdef CONFIG_ALT1250B
  #define UART1_IO_SEL_UARTF_RX   (BASE_ADDRESS_IO_SEL + 0x94)   /* uart1_rxd - UART2_RX */
  #define UART1_IO_SEL_UARTF_TX   (BASE_ADDRESS_IO_SEL + 0x98)   /* uart1_txd - UART2_TX */
#else
  #define UART1_IO_SEL_UARTF_RX   (BASE_ADDRESS_IO_SEL + 0xBC)   /* uart1_rxd - SPIM0_EN0 */
  #define UART1_IO_SEL_UARTF_TX   (BASE_ADDRESS_IO_SEL + 0xB8)   /* uart1_txd - SPIM0_MISO */
#endif
#endif
/* added for HW flow control */
#define UART1_UARTCR_RTSEN_MSK                                         (0x04000)
#define UART1_UARTCR_CTSEN_MSK                                         (0x08000)
#ifdef CONFIG_ALT1250B
#define UART0_IO_SEL_UARTF_CTS        (BASE_ADDRESS_IO_SEL + 0x8C)
#define UART0_IO_SEL_UARTF_RTS        (BASE_ADDRESS_IO_SEL + 0x90)

#define UART0_IO_CFG_UARTF_CTS        (BASE_ADDRESS_IO_CFG + 0x8C)
#define UART0_IO_CFG_UARTF_RTS        (BASE_ADDRESS_IO_CFG + 0x90)

#define UARTS_IO_SEL_UARTF_CTS        (BASE_ADDRESS_IO_SEL + 0x9C)
#define UARTS_IO_SEL_UARTF_RTS        (BASE_ADDRESS_IO_SEL + 0xA0)

#define UARTS_IO_CFG_UARTF_CTS        (BASE_ADDRESS_IO_CFG + 0x9C)
#define UARTS_IO_CFG_UARTF_RTS        (BASE_ADDRESS_IO_CFG + 0xA0)
#else
#define UART0_IO_SEL_UARTF_CTS        (BASE_ADDRESS_IO_SEL + 0x94)
#define UART0_IO_SEL_UARTF_RTS        (BASE_ADDRESS_IO_SEL + 0x98)

#define UART0_IO_CFG_UARTF_CTS        (BASE_ADDRESS_IO_CFG + 0x94)
#define UART0_IO_CFG_UARTF_RTS        (BASE_ADDRESS_IO_CFG + 0x98)

#define UARTS_IO_SEL_UARTF_CTS        (BASE_ADDRESS_IO_SEL + 0xA4)
#define UARTS_IO_SEL_UARTF_RTS        (BASE_ADDRESS_IO_SEL + 0xA8)

#define UARTS_IO_CFG_UARTF_CTS        (BASE_ADDRESS_IO_CFG + 0xA4)
#define UARTS_IO_CFG_UARTF_RTS        (BASE_ADDRESS_IO_CFG + 0xA8)
#endif
#if (CONFIG_FAST_UART_PORT == (0))
#define UART1_CFG_FAST                (BASE_ADDRESS_UART0_CFG + 0x80)
#else
#define UART1_CFG_FAST                (BASE_ADDRESS_UART1_CFG + 0x80)
#endif
#define UART1_CFG_FAST_UARTF_HALT_MSK     (1 << 1)
#define UART1_CFG_FAST_APB_MUX_CTRL_MSK   (1 << 0)
#endif

//masks for rx & tx fifo level thresholds
#define UART0_UARTIFLS_RXIFLSEL_MSK   (0x000F0)
/*interrupt when rx fifo becomes more then 1/16 full */
#define UART_RX_INTR_LEVEL (0x0 << 4)
/*interrupt when tx fifo becomes 1/16 full */
#define UART_TX_INTR_LEVEL (0x0 << 0)

#define UART0_UARTIFLS_TXIFLSEL_MSK   (0x0000F)
/*mask for possible uart errors indications */
#define UART0_UARTRSR_FE_MSK          (0x00001)
#define UART0_UARTRSR_PE_MSK          (0x00002)
#define UART0_UARTRSR_BE_MSK          (0x00004)
#define UART0_UARTRSR_OE_MSK          (0x00008)

//masks interrupt clear  register
#define UART0_UARTICR_RXIC_MSK        (0x00010)
#define UART0_UARTICR_TXIC_MSK        (0x00020)
#define UART0_UARTICR_RTIC_MSK        (0x00040)
#define UART0_UARTFR_TXFE_MSK         (0x00080)
// clang-format on

typedef enum {
  SERIAL_UART_TYPE_SLOW,
  SERIAL_UART_TYPE_FAST,
  SERIAL_UART_TYPE_MAX_ID
} serial_uart_chan_t;

typedef unsigned int (*serialTxCharFp_t)(void *uart_func, const char c);

typedef struct serial_uart_ctxt {
  unsigned char uartHwIndx;
  enum hwSer_tx_mode TxBlockingMode;
  IRQn_Type interrupt_number;
  serialTxCharFp_t serial_putc;
  unsigned char WasRegistInterrupt;
  void (*buffered_input_interrupt_register)(void);
  serialRxProcessFp_t serialRxProcess;
  void *rxProcessCookie;
  void *xmitBuffer;
  void *recvBuffer;
  uint32_t *uart_base;
  void *uart_cfg;
  serial_uart_chan_t uartType;
  uint32_t smngr_id;
  sHuart uartInitParam;
  xQueueHandle xQueue_rx;
} serial_uart_ctxt_t;

// functions list

/**
 * @brief To allocate required resources for a new serial connection.
 *
 * @param uart_base uart controller base address
 * @param uart_cfg uart controller cfg register address
 * @param uartHwIndx uart channel index
 * @param uartType uart type is slow or fast
 * @param interrupt_number The interrupt number for this uart controller
 * @param uartInitParam The UART running paramters like baudrate, stop bit and etc.
 * @return void* the handle of this uart instance
 */
void *serialUartDriverAllocInstance(void *uart_base, void *uart_cfg,
                                    enum hwSer_channelNumber uartHwIndx,
                                    serial_uart_chan_t uartType, IRQn_Type interrupt_number,
                                    sHuart *uartInitParam);
/**
 * @brief To change parameters on the opened uart
 *
 * @param handle The uart handle
 * @param ioctlId The action to do
 * @param param The parameter for the action
 * @param appCookie The second paramter for IOCTAL_SERIAL_RX_CB_FUNC
 * @return int 0 on success. Others on fail
 */

int serialUartDriverIoctl(void *handle, serial_ioctl_id_t ioctlId, uint32_t *param,
                          void *appCookie);

/**
 * @brief To write data to uart port
 *
 * @param serialHandle The uart handle
 * @param txCharBuf The data to be sent
 * @param txCharBufLen The data length to be sent
 * @return int The data length which successfully sent
 */
int serialUartDriverWrite(void *serialHandle, const char *txCharBuf, uint32_t txCharBufLen);

/**
 * @brief To allocate phsical pins for uart controller
 *
 * @param serialHandle The uart handle
 */
void serialUartDriverHwInit(void *serialHandle);

/**
 * @brief To create a task which receives data from circular buffer of uart port to the rx thread of
 * uart port
 *
 */
void serialUartDriver_createRxTask(void);

/**
 * @brief To get the uart handle of the channel
 *
 * @param channel The uart handle to be get
 * @return struct serial_uart_ctxt* The uart handle
 */
struct serial_uart_ctxt *serial_uart_get_ctxt(enum hwSer_channelNumber channel);

/**
 * @brief To delete resouces which allocated to the uart
 *
 * @param handle The uart handle
 * @return int 0 on succcess. Otherwise on fail
 */
int serialUartDriverDeallocInstance(void *handle);

/**
 * @brief To check if UART TX is busy
 *
 * @param uart_base The uart controller base address
 * @return int 0 when TX fifo is empty. return 1 if Tx fifo has data.
 */
int is_serial_tx_busy(uint32_t *uart_base);
#endif /* SERIAL_ALT125X_H_ */
