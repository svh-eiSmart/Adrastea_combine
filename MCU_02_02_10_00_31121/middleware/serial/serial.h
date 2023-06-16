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
 * @file serial.h
 */

#ifndef _serial_H_
#define _serial_H_
#include <stdio.h>
/**
 * @defgroup serial_uart UART Driver
 * @{
 */

/**
 * @defgroup UART_const UART Constants
 * @{
 */
#define UARTI0_BAUDRATE_DEFAULT DRV_UART_BAUD_460800 /*!<  Default internal UART baudrate*/
#define UARTI0_HWFLOWCTRL_DEFAULT \
  DRV_UART_HWCONTROL_RTS_CTS /*!<  Default internal UART flow control*/
#define UARTI0_PARITY_DEFAULT DRV_UART_PARITY_MODE_DISABLE /*!<  Default internal UART parity*/
#define UARTI0_STOPBITS_DEFAULT DRV_UART_STOP_BITS_1       /*!<  Default internal UART stopbit*/
#define UARTI0_WORDLENGTH_DEFAULT                          \
  DRV_UART_WORD_LENGTH_8B /*!<  Default internal UART word \
                             lendth*/
/** @} UART_const */

/**
 * @defgroup UART_types UART Types
 * @{
 */

/**
 * @brief Define the uart port
 *
 */
typedef enum {
  ACTIVE_UARTF0, /*!< External uart 1 */
#ifdef ALT1250
  ACTIVE_UARTF1, /*!< External uart 2 */
#endif
  ACTIVE_UARTI0, /*!< Internal uart 1 */
  ACTIVE_MAX,
} eUartInstance;

/**
 * @brief Define the baud rate supported by the uart controller
 *
 */
typedef enum {
  BAUD_110 = 110,         /*!< 110 buatrate */
  BAUD_1200 = 1200,       /*!< 1200 buatrate */
  BAUD_2400 = 2400,       /*!< 2400 buatrate */
  BAUD_9600 = 9600,       /*!< 9600 buatrate */
  BAUD_14400 = 14400,     /*!< 14400 buatrate */
  BAUD_19200 = 19200,     /*!< 19200 buatrate */
  BAUD_38400 = 38400,     /*!< 38400 buatrate */
  BAUD_57600 = 57600,     /*!< 57600 buatrate */
  BAUD_76800 = 76800,     /*!< 76800 buatrate */
  BAUD_115200 = 115200,   /*!< 115200 buatrate */
  BAUD_230400 = 230400,   /*!< 230400 buatrate */
  BAUD_460800 = 460800,   /*!< 460800 buatrate */
  BAUD_921600 = 921600,   /*!< 921600 buatrate */
  BAUD_1843200 = 1843200, /*!< 1843200 buatrate */
  BAUD_3686400 = 3686400, /*!< 3686400 buatrate */
} eBaudrate;

/**
 * @brief Define the word length
 *
 */
typedef enum {
  UART_WORDLENGTH_5B = 0, /*!< 5 bits in a word */
  UART_WORDLENGTH_6B = 1, /*!< 6 bits in a word */
  UART_WORDLENGTH_7B = 2, /*!< 7 bits in a word */
  UART_WORDLENGTH_8B = 3, /*!< 8 bits in a word */
} eWordLength;

/**
 * @brief Define the length of stop bit
 *
 */
typedef enum {
  UART_STOPBITS_1 = 0, /*!< 1 stpo bit */
  UART_STOPBITS_2 = 1, /*!< 2 stop bits */
} eStopBits;

/**
 * @brief Define the parity bit
 *
 */
typedef enum {
  SERIAL_PARITY_MODE_ZERO = 0, /*!< 0 parity */
  SERIAL_PARITY_MODE_ONE = 1,  /*!< 1 parity */
  SERIAL_PARITY_MODE_ODD = 2,  /*!< Odd parity */
  SERIAL_PARITY_MODE_EVEN = 3, /*!< Even parity */
  SERIAL_PARITY_MODE_DISABLE = 4,
} eParity;

/**
 * @brief Define the flow control
 *
 */
typedef enum {
  UART_HWCONTROL_NONE = 0,    /*!< no flow control */
  UART_HWCONTROL_RTS = 1,     /*!< flow control on RTS*/
  UART_HWCONTROL_CTS = 2,     /*!< flow control on CTS*/
  UART_HWCONTROL_RTS_CTS = 3, /*!< flow control on both RTS and CTS*/
} eHwFlowCtl;

typedef enum{
  SERIAL_REGISTER_CALLBACK = 0,   /*!< Register callback function. Expected arg: callback function pointer with serial_callback prototype */
  SERIAL_ENABLE_BREAK_ERROR = 1,  /*!< Enable break error interrupt. Expected arg: NULL */
  SERIAL_DISABLE_BREAK_ERROR = 2, /*!< Disable break error interrupt. Expected arg: NULL */
} eIoctl;

typedef enum{
  SERIAL_STATUS_PARITY = 0,        /*!< Parity error */
  SERIAL_STATUS_BREAK = 1,         /*!< Break error */
  SERIAL_STATUS_FRAME = 2,         /*!< Frame error */
  SERIAL_STATUS_OVERRUN = 3,       /*!< Overrun error */
} eCallbackStatus;
/** @brief Definition of the UART parameter */
typedef struct {
  eBaudrate BaudRate;     /*!< The UART Baudrate */
  eWordLength WordLength; /*!< The UART word length*/
  eStopBits StopBits;     /*!< The UART stop bit */
  eParity Parity;         /*!< THe UART parity*/
  eHwFlowCtl HwFlowCtl;   /*!< THe UART hardware flow control*/
} sInit;

/** @brief Definition of the UART initialization parameter */
typedef struct {
  eUartInstance Instance; /*!<  The UART physical instance to be opened*/
  sInit Init;             /*!< The UART initialization parameter*/
} sHuart;

/** Definition of the UART handle */
typedef void *serial_handle;

typedef void (*serial_callback)(serial_handle handle, eCallbackStatus status);

/** @} UART_types */

/**
 * @defgroup uart_api UART APIs
 * @{
 */

#if defined(__cplusplus)
extern "C" {
#endif /* _cplusplus */

/**
 * @brief To send peripheral-specific request
 *
 * @param handle The uart handle
 * @param request request type to be sent
 * @param arg request-spicific argument. See the detail at @ref eIoctl
 * @return int32_t 0 on success. others, negative value are returned.
 */
int32_t serial_ioctl(serial_handle handle, eIoctl request, void *arg );

/**
 * @brief To write data to uart port
 *
 * @param [in] handle: The uart handle
 * @param [in] buf: The data to be sent
 * @param [in] len: The data length to be sent
 *
 * @return The data length which successfully sent
 */
size_t serial_write(serial_handle handle, void *buf, size_t len);
/**
 * @brief To read data from uart port
 *
 * @param [in] handle: The uart handle
 * @param [in] buf: The buffer for the read data
 * @param [in] len: The data length to be reveived
 *
 * @return The data length which successfully read
 *
 */
size_t serial_read(serial_handle handle, void *buf, size_t len);

/**
 * @brief To open a serial port
 *
 * @param [in] uartInitParam: The paramters for the uart port
 *
 * @return The handle of the uart port
 */
serial_handle serial_open(eUartInstance instance);

/**
 * @brief To close a serial port
 *
 * @param [in] handle: The handle of the uart port
 * @return int32_t 0 on success. Others on fail
 */
int32_t serial_close(serial_handle handle);

/**
 * @brief To abort a write requset
 *
 * @param [in] handle: The handle of the uart port
 * @return int32_t 0 on success. others, negative value are returned.
 */
int32_t abort_write(serial_handle handle);

/**
 * @brief To abort a read request
 *
 * @param [in] handle: The handle of the uart port
 * @return int32_t 0 on success. others, negative value are returned.
 */
int32_t abort_read(serial_handle handle);

void dump_statistics(eUartInstance ins);
#if defined(__cplusplus)
}
#endif
/** @} uart_api */
/** @} serial_uart */
#endif /* CORE_OS_DRIVERS_SERIAL_SERIAL_CONTAINER_H_ */
