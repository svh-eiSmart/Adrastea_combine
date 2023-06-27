/*  ---------------------------------------------------------------------------

        (c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.

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
 * @file led.h
 */

#ifndef CORE_OS_DRIVERS_SERIAL_SERIAL_CONTAINER_H_
#define CORE_OS_DRIVERS_SERIAL_SERIAL_CONTAINER_H_
#include "serialMngrLogPort.h"

/**
 * @defgroup serial_uart UART Driver
 * @{
 */

/**
 * @defgroup UART_const UART Constants
 * @{
 */
#define UARTI0_BAUDRATE_DEFAULT BAUD_460800              /*!<  Default internal UART baudrate*/
#define UARTI0_HWFLOWCTRL_DEFAULT UART_HWCONTROL_RTS_CTS /*!<  Default internal UART flow \
                                                            control*/
#define UARTI0_PARITY_DEFAULT SERIAL_PARITY_MODE_DISABLE /*!<  Default internal UART parity*/
#define UARTI0_STOPBITS_DEFAULT UART_STOPBITS_1          /*!<  Default internal UART stopbit*/
#define UARTI0_WORDLENGTH_DEFAULT UART_WORDLENGTH_8B     /*!<  Default internal UART word lendth*/
/** @} UART_const */

/**
 * @defgroup UART_types UART Types
 * @{
 */
/** Definition of the UART handle */
typedef uint32_t serial_handle;
/** Prototype of UART Rx task  */
typedef void (*serialRxProcessFp_t)(const char charRecv, void *recvCookie);
/** Prototype of UART Tx function */
typedef uint32_t (*serialMngrTxcharFp_t)(serial_handle *handle, const char *buf, uint32_t len);

/**
 * @brief Define the uart port
 *
 */
typedef enum {
  ACTIVE_NONE = 0,
  ACTIVE_UARTF0 = 1, /*!< External uart 1 */
#ifdef ALT1250
  ACTIVE_UARTF1 = 2, /*!< External uart 2 */
#endif
  ACTIVE_UARTI0 = 3, /*!< Internal uart 1 */
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

/** @} UART_types */

/**
 * @defgroup uart_api UART APIs
 * @{
 */

/**
 * @brief Init the serial driver. Only call once
 *
 * @return 0 on succcess. Otherwise on fail
 */
int serial_init(void);

/**
 * @brief To change parameters on the opened uart
 *
 * @param [in] handle: The uart handle
 * @param [in] serialIoctlId: The action to do
 * @param [in] param: The parameter for the action
 * @param [in] appCookie: The second paramter for IOCTAL_SERIAL_RX_CB_FUNC
 *
 * @return 0 on success. Others on fail
 */
int serial_ioctl(serial_handle *handle, serial_ioctl_id_t serialIoctlId, void *param,
                 void *appCookie);

/**
 * @brief To write data to uart port
 *
 * @param [in] handle: The uart handle
 * @param [in] buf: The data to be sent
 * @param [in] len: The data length to be sent
 *
 * @return The data length which successfully sent
 */
uint32_t serial_write(serial_handle *handle, const char *buf, uint32_t len);
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
uint32_t serial_read(serial_handle *handle, char *buf, uint32_t len);

/**
 * @brief Load default configuration generated by MCU wizard
 *
 * @param [in] uartInstance: The target uart port
 * @param [in] uartInitParam: The paramters for the uart port
 *
 * @return 0 on success. Others on fail
 */
int serial_load_defconfig(eUartInstance uartInstance, sHuart *uartInitParam);

/**
 * @brief To open a serial port
 *
 * @param [in] uartInitParam: The paramters for the uart port
 *
 * @return The handle of the uart port
 */
serial_handle *serial_open(sHuart *uartInitParam);

/**
 * @brief To close a serial port
 *
 * @param [in] handle: The handle of the uart port
 * @return 0 on success. Others on fail
 */
int serial_close(serial_handle *handle);
/** @} uart_api */
/** @} serial_uart */
#endif /* CORE_OS_DRIVERS_SERIAL_SERIAL_CONTAINER_H_ */
