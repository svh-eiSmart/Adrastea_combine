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

#include "FreeRTOS.h"
#include "serialMngrLogPort.h"
#include "serial.h"

#define STD_OUT_PORT ACTIVE_UARTF0

typedef void (*received_event)(char c);

enum streams { STDIN = 0, STDOUT, STDERR };

struct rcvEventChar {
  received_event callback;
  char c;
};

/**
 * Write to uart tx fifo from xmit circ buf till tx fifo is  full.
 *
 * @param uartDataReg- address of the uart data register
 * @return 0 - data from xmit circ buf was sent.
 *         1- No data was transmitted.
 */
int writeFromClibXmit(volatile void *uartDataReg);
/**
 * Write to stdOutput queue .
 *
 * @param c- chat to be sent
 * @return void
 *
 */
void writeToSerialRxQueu(const char c, void *appCookie);
void machine_restart(int do_sync);
xQueueHandle init_rxQueue(void);
void delete_rxQueue(xQueueHandle xQueue);
void registerCharEvent(const char c, received_event cb);
void newlib_set_stdout_port(eUartInstance uart_inst);
