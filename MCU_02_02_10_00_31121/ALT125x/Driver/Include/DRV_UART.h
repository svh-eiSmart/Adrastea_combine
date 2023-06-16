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
/**
* @file DRV_UART.h
*/
#ifndef SERIAL_ALT125X_H_
#define SERIAL_ALT125X_H_

#include <stdio.h>
#include DEVICE_HEADER
/**
 * @defgroup uart_driver UART Low-Level Driver
 * @{
 */

/**
 * @defgroup drv_uart_types uart Types
 * @{
 */

/**
 * @brief UART status code
 *
 */
typedef enum {
  DRV_UART_ERROR_NONE = 0x0U,   /*!< Operation succeeded */
  DRV_UART_WAIT_CB,             /*!< Waiting callback when request complete */
  DRV_UART_ERROR_UNKNOWN,       /*!< Unknown/Unspecified error */
  DRV_UART_ERROR_BUSY,          /*!< Operation busy */
  DRV_UART_ERROR_TIMEOUT,       /*!< Operation timeout */
  DRV_UART_ERROR_UNSUPPORTED,   /*!< Operation not supported */
  DRV_UART_ERROR_PARAMETER,     /*!< Parameter error */
  DRV_UART_ERROR_SPECIFIC,      /*!< Specific mapping */
  DRV_UART_ERROR_UNINIT,        /*!< Driver not yet initialized */
  DRV_UART_ERROR_INIT,          /*!< Driver already initialized */
  DRV_UART_ERROR_PARITY,        /*!< Parity error */
  DRV_UART_ERROR_BREAK,         /*!< Break error */
  DRV_UART_ERROR_FRAME,         /*!< Frame error */
  DRV_UART_ERROR_OVERRUN,       /*!< Overrun error */
  DRV_UART_ERROR_TX,            /*!< Transmit error */
  DRV_UART_ERROR_RX,            /*!< Receive error */
  DRV_UART_ERROR_HIFC_TIMEOUT,  /*!< HIFC timeout error */
} DRV_UART_Status;

/**
 * @brief Define the baud rate supported by the uart controller
 *
 */
typedef enum {
  DRV_UART_BAUD_110 = 110U,         /*!< 110 buatrate */
  DRV_UART_BAUD_1200 = 1200U,       /*!< 1200 buatrate */
  DRV_UART_BAUD_2400 = 2400U,       /*!< 2400 buatrate */
  DRV_UART_BAUD_9600 = 9600U,       /*!< 9600 buatrate */
  DRV_UART_BAUD_14400 = 14400U,     /*!< 14400 buatrate */
  DRV_UART_BAUD_19200 = 19200U,     /*!< 19200 buatrate */
  DRV_UART_BAUD_38400 = 38400U,     /*!< 38400 buatrate */
  DRV_UART_BAUD_57600 = 57600U,     /*!< 57600 buatrate */
  DRV_UART_BAUD_76800 = 76800U,     /*!< 76800 buatrate */
  DRV_UART_BAUD_115200 = 115200U,   /*!< 115200 buatrate */
  DRV_UART_BAUD_230400 = 230400U,   /*!< 230400 buatrate */
  DRV_UART_BAUD_460800 = 460800U,   /*!< 460800 buatrate */
  DRV_UART_BAUD_921600 = 921600U,   /*!< 921600 buatrate */
  DRV_UART_BAUD_1843200 = 1843200U, /*!< 1843200 buatrate */
  DRV_UART_BAUD_3686400 = 3686400U, /*!< 3686400 buatrate */
} DRV_UART_Baudrate;

/**
 * @brief Define the word length
 *
 */
typedef enum {
  DRV_UART_WORD_LENGTH_5B = 0U, /*!< 5 bits in a word */
  DRV_UART_WORD_LENGTH_6B = 1U, /*!< 6 bits in a word */
  DRV_UART_WORD_LENGTH_7B = 2U, /*!< 7 bits in a word */
  DRV_UART_WORD_LENGTH_8B = 3U, /*!< 8 bits in a word */
} DRV_UART_Word_Length;

/**
 * @brief Define the length of stop bit
 *
 */
typedef enum {
  DRV_UART_STOP_BITS_1 = 0U, /*!< 1 stpo bit */
  DRV_UART_STOP_BITS_2 = 1U, /*!< 2 stop bits */
} DRV_UART_Stop_Bits;

/**
 * @brief Define the parity bit
 *
 */
typedef enum {
  DRV_UART_PARITY_MODE_ZERO = 0U, /*!< 0 parity */
  DRV_UART_PARITY_MODE_ONE = 1U,  /*!< 1 parity */
  DRV_UART_PARITY_MODE_ODD = 2U,  /*!< Odd parity */
  DRV_UART_PARITY_MODE_EVEN = 3U, /*!< Even parity */
  DRV_UART_PARITY_MODE_DISABLE = 4U, /*!< No parity */
} DRV_UART_Parity;

/**
 * @brief Define the flow control
 *
 */
typedef enum {
  DRV_UART_HWCONTROL_NONE = 0U,    /*!< no flow control */
  DRV_UART_HWCONTROL_RTS = 1U,     /*!< flow control on RTS*/
  DRV_UART_HWCONTROL_CTS = 2U,     /*!< flow control on CTS*/
  DRV_UART_HWCONTROL_RTS_CTS = 3U, /*!< flow control on both RTS and CTS*/
} DRV_UART_Hardware_Flow_Control;

/**
 * @brief UART port
 *
 */
typedef enum {
  DRV_UART_F0 = 0, /*!< External uart 1 */
#ifdef ALT1250
  DRV_UART_F1, /*!< External uart 2 */
#endif
  DRV_UART_I0, /*!< Internal uart 1 */
  DRV_UART_MAX,
} DRV_UART_Port;

/**
 * @brief UART notify code
 *
 */
enum _callback_notify_code {
  CB_RX_COMPLETE, /*!< UART receive complete */
  CB_TX_COMPLETE, /*!< UART transmit complete */
  CB_BREAK_ERROR, /*!< UART receive break error */
};

/**
 * @brief UART Tx/Rx line state
 *
 */
typedef enum {
  DRV_UART_LINE_STATE_IDLE,
  DRV_UART_LINE_STATE_BUSY,
} DRV_UART_Line_State;

/**
 * @brief UART driver handle
 *
 */
typedef struct _uart_handle DRV_UART_Handle;

/*! @brief UART transfer callback function. */
typedef void (*DRV_UART_EventCallback_t)(DRV_UART_Handle *handle, uint32_t status, void *user_data);

/*! @brief Definition of the UART parameter */
typedef struct {
  DRV_UART_Baudrate BaudRate;               /*!< The UART baudrate */
  DRV_UART_Word_Length WordLength;          /*!< The UART word length*/
  DRV_UART_Stop_Bits StopBits;              /*!< The UART stop bit */
  DRV_UART_Parity Parity;                   /*!< THe UART parity*/
  DRV_UART_Hardware_Flow_Control HwFlowCtl; /*!< THe UART hardware flow control*/
} DRV_UART_Config;

/*! @brief Definition of UART running statistic */
typedef struct {
  size_t rx_cnt;      /*!< Received character counter*/
  size_t rx_cnt_isr;  /*!< Received character from ISR counter */
  size_t tx_cnt;      /*!< Transmitted character counter */
  size_t tx_cnt_isr;  /*!< Transmitted character from ISR counter */
  size_t pe_cnt;      /*!< Parity error counter */
  size_t be_cnt;      /*!< Break error counter */
  size_t oe_cnt;      /*!< Overrun error counter */
  size_t fe_cnt;      /*!< Frame error counter */
} DRV_UART_Statistic;
/*! @brief Definition of UART handler */
struct _uart_handle {
  UART_Type *base; /*!< UART controller base address */

  /* uart receive/trasmit pointer */
  int8_t *tx_data; /*!<  Pointer of user requested data for sending*/
  size_t tx_data_remaining_byte; /*!< Amount of remaining bytes that user requested for sending */
  size_t tx_data_total_byte;        /*!< Amount of bytes that IRQ should handle */
  size_t tx_data_length_requested;  /*!< Amount of bytes that user requested for sending*/
  int8_t *rx_data;                  /*!< Pointer of user requested data for receiving */
  size_t rx_data_remaining_byte;    /*!< Amount of remaining bytes that user requested for receiving */
  size_t rx_data_total_byte;        /*!< Amount of bytes that IRQ should save to rx_data intead of ringbuffer */
  size_t rx_data_length_requested;  /*!< Amount of bytes that user requested for receiving */

  /* ringbuffer */
  int8_t *rx_ringbuffer;            /*!< Pointer of Ringbuffer */
  size_t ringbuffer_size;           /*!< Ringbffer size */
  size_t ringbuffer_head;           /*!< Ringbuffer head location */
  size_t ringbuffer_tail;           /*!< Ringbuffer tail location */

  /* driver status */
  DRV_UART_Line_State tx_line_state;  /*!< Indicate Tx is busy or not */
  DRV_UART_Line_State rx_line_state;  /*!< Indicate Rx is busy or not */

  /* callback */
  DRV_UART_EventCallback_t callback;  /*!< Callback function to notify user space */
  void *user_data;  /*!< Parameter of callback function */

  /* Statistic info */
  DRV_UART_Statistic stat; /*!< UART running statistic */
};

/** @} drv_uart_types */
#if defined(__cplusplus)
extern "C" {
#endif /* _cplusplus */

/**
 * @defgroup drv_uart_apis UART APIs
 * @{
 */
/**
 * @brief Initialize UART peripheral
 *
 * @param base UART peripheral base address.
 * @param UART_init UART parameters.
 * @param clock The clock rate in HZ that the UART peripheral is running.
 * @return DRV_UART_Status Return the status code
 */
DRV_UART_Status DRV_UART_Initialize(UART_Type *base, const DRV_UART_Config *UART_init,
                                    uint32_t clock);

/**
 * @brief Initialize the UART handle.
 *
 * @param base UART peripheral base address.
 * @param handle A pre-allocated space used to store uart handle
 * @param callback Callback function that will be called when a non-blocking function is done.
 * @param userdata A user-defined data that will pass to callback function
 * @param buffer A space that used for a receiveing buffer
 * @param buffer_size The size of the receiving buffer
 * @return DRV_UART_Status Return the status code
 */
DRV_UART_Status DRV_UART_Create_Handle(UART_Type *base, DRV_UART_Handle *handle,
                                       DRV_UART_EventCallback_t callback, void *userdata,
                                       int8_t *buffer, size_t buffer_size);

/**
 * @brief Uninitialize the UART peripheral
 *
 * @param handle UART handle
 * @return DRV_UART_Status Return the status code
 */
DRV_UART_Status DRV_UART_Uninitialize(DRV_UART_Handle *handle);

/**
 * @brief To receive data by a blocking method.
 * This function polls the UART status register to read data and
 * returns until request data length are accomplished.
 *
 * @param handle UART handle
 * @param buffer A space that data should be stored to
 * @param length Request data length
 * @return DRV_UART_Status Return the status code
 */
DRV_UART_Status DRV_UART_Receive_Block(DRV_UART_Handle *handle, void *buffer, size_t length);

/**
 * @brief Receive data by a nonblocking method.
 * This is a non-blocking function which doesn't wait for data but returns immediately nomatter
 * requested data length is completed or not. If the requested data length is available in the
 * ringbuffer, this function return DRV_UART_ERROR_NONE. Otherwise, returns DRV_UART_WAIT_CB and receive
 * data by ISR and will use callback to notify upper layer when requested data length is ready.
 *
 * @param handle UART handle
 * @param buffer A space that data should be stored to
 * @param length Requested data length
 * @return DRV_UART_Status Return the status code
 */
DRV_UART_Status DRV_UART_Receive_Nonblock(DRV_UART_Handle *handle, void *buffer, size_t length);

/**
 * @brief Send data by a nonblocking method.
 * This is a non-blocking function which doesn't wait for data to be shift out but returns
 * immediately. If the tx FIFO has enough space for requsted legnth, this function returns
 * DRV_UART_ERROR_NONE, otherwaise, returns DRV_UART_WAIT_CB and send data by ISR and will use callback to
 * notify uppder layer when requested data length are all moved to tx FIFO.
 *
 * @param handle UART handle
 * @param data A space that contains data to be shift out
 * @param length Requested data length
 * @return DRV_UART_Status Return the status code
 */
DRV_UART_Status DRV_UART_Send_Nonblock(DRV_UART_Handle *handle, void *data, uint32_t length);

/**
 * @brief send data by a blocking method.
 * This function polls the UART status register to send data and returns until all requested
 * data are moved to tx FIFO.
 *
 * @param handle c
 * @param data A space that contains data to be shift out
 * @param length Requested data length
 * @return DRV_UART_Status Return the status code
 */
DRV_UART_Status DRV_UART_Send_Block(DRV_UART_Handle *handle, void *data, uint32_t length);

/**
 * @brief Change the UART parameters
 *
 * @param base UART peripheral base address.
 * @param UART_config New UART parameters
 * @param clock The clock rate in HZ that the UART peripheral is running.
 */
void DRV_UART_Update_Setting(UART_Type *base, const DRV_UART_Config *UART_config, uint32_t clock);

/**
 * @brief Enable UART Tx function
 *
 * @param base UART peripheral base address.
 */
void DRV_UART_Enable_Tx(UART_Type *base);

/**
 * @brief Disable UART Tx function
 *
 * @param base UART peripheral base address.
 */
void DRV_UART_Disable_Tx(UART_Type *base);

/**
 * @brief Enable UART Rx function
 *
 * @param base UART peripheral base address.
 */
void DRV_UART_Enable_Rx(UART_Type *base);

/**
 * @brief Disable UART Rx function
 *
 * @param base UART peripheral base address.
 */
void DRV_UART_Disable_Rx(UART_Type *base);

/**
 * @brief Set RTS to active
 *
 * @param base UART peripheral base address.
 */
void DRV_UART_Active_RTS(UART_Type *base);

/**
 * @brief Set RTS to deactive
 *
 * @param base UART peripheral base address.
 */
void DRV_UART_Deactive_RTS(UART_Type *base);

/**
 * @brief Set DTR to active
 *
 * @param base UART peripheral base address.
 */
void DRV_UART_Active_DTR(UART_Type *base);

/**
 * @brief Set DTR to deactive
 *
 * @param base UART peripheral base address.
 */
void DRV_UART_Deactive_DTR(UART_Type *base);

/**
 * @brief Enable break error interrupt
 *
 * @param base UART peripheral base address.
 */
void DRV_UART_Enable_Break_Interrupt(UART_Type *base);

/**
 * @brief Disable break error interrupt
 *
 * @param base UART peripheral base address.
 */
void DRV_UART_Disable_Break_Interrupt(UART_Type *base);

/**
 * @brief Get CTS activeness
 *
 * @param base UART peripheral base address.
 * @return uint32_t 1 for active. 0 for deactive.
 */
uint32_t DRV_UART_Get_CTS(UART_Type *base);

/**
 * @brief Get DSR activeness
 *
 * @param base UART peripheral base address.
 * @return uint32_t 1 for active. 0 for deactive.
 */
uint32_t DRV_UART_Get_DSR(UART_Type *base);

/**
 * @brief Get DCD activeness
 *
 * @param base UART peripheral base address.
 * @return uint32_t 1 for active. 0 for deactive.
 */
uint32_t DRV_UART_Get_DCD(UART_Type *base);

/**
 * @brief Get RI activeness
 *
 * @param base UART peripheral base address.
 * @return uint32_t 1 for active. 0 for deactive.
 */
uint32_t DRV_UART_Get_RI(UART_Type *base);
/**
 * @brief Abort on non-blocking receive
 *
 * @param handle UART handle
 * @return int32_t Data length that have been received
 */
int32_t DRV_UART_Abort_Receive(DRV_UART_Handle *handle);

/**
 * @brief abort on non-blocking send
 *
 * @param handle UART handle
 * @return int32_t Data length that have been written to tx FIFO
 */
int32_t DRV_UART_Abort_Send(DRV_UART_Handle *handle);

/**
 * @brief Query the data length that have been received in the latest non-blocking receive request
 *
 * @param handle UART handle
 * @return uint32_t Data length that have been received
 */
uint32_t DRV_UART_Get_Received_Rx_Count(DRV_UART_Handle *handle);

/**
 * @brief Query the data length that have been sent in the latest non-blocking send request
 *
 * @param handle UART handle
 * @return uint32_t Data length that have been sent
 */
uint32_t DRV_UART_Get_Transmitted_Tx_Count(DRV_UART_Handle *handle);

/**
 * @brief Check if the UART peripheral is busy on transmitting data
 *
 * @param base
 * @return int32_t
 */
int32_t DRV_UART_Is_Uart_Busy(UART_Type *base);

/**
 * @brief Get the uart statistic information
 *
 * @param handle UART handle
 * @param stat Output result
 * @return DRV_UART_Status Return the status code
 */
DRV_UART_Status DRV_UART_Get_Statistic(DRV_UART_Handle *handle, DRV_UART_Statistic *stat);
/** @} drv_uart_apis */
#if defined(__cplusplus)
}
#endif
/** @} uart_driver */
#endif /* SERIAL_ALT125X_H_ */
