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

//#include <reent.h>
#if defined(__ICCARM__)
#include <yfuns.h>
#endif
#if defined(__GNUC__)
#include <unistd.h>
#endif

#include <FreeRTOS.h>
#include "queue.h"
#include "task.h"
#include "portmacro.h"
#include <time.h>
#include "errno.h"  // error types moved from arch.h to errno.h
#include "newlibPort.h"
#include <stdio.h>

#define INPUT_QUEUE_LENGTH 128

struct rcvEventChar *tRcvCharCb = NULL;
static enum hwSer_channelNumber std_out_channel = MAX_UART_NUM;

xQueueHandle init_rxQueue(void) {
  xQueueHandle xQueue = NULL;
  xQueue = xQueueCreate(INPUT_QUEUE_LENGTH, sizeof(char));
  configASSERT(xQueue);

  return xQueue;
}

void delete_rxQueue(xQueueHandle xQueue) { vQueueDelete(xQueue); }
/**
 * Write to stdOutput queue .
 *
 * @param c- chat to be sent
 * @return void
 *
 */
void writeToSerialRxQueu(const char c, void *appCookie) {
  configASSERT(appCookie);
  serial_uart_ctxt_t *sHandle = (serial_uart_ctxt_t *)appCookie;

  if (tRcvCharCb != NULL) {
    if (tRcvCharCb->c == c) {
      tRcvCharCb->callback(c);
    }
  }
  xQueueSendToBack(sHandle->xQueue_rx, &c, portMAX_DELAY);
}

void newlib_set_stdout_port(eUartInstance uart_inst) {
  switch (uart_inst) {
    case ACTIVE_UARTF0:
      std_out_channel = UART0;
      break;
#ifdef ALT1250
    case ACTIVE_UARTF1:
      std_out_channel = UART1;
      break;
#endif
    case ACTIVE_UARTI0:
      std_out_channel = UARTI0;
      break;
    default:
      break;
  }
}

#if defined(__GNUC__)

void _putchar(char character) { write(STDOUT_FILENO, &character, sizeof(character)); }

int _write(int file, char *ptr, int len) {
  /* Remove the #if #endif pair to enable the implementation */
  if (std_out_channel >= MAX_UART_NUM) return -1;

  serial_uart_ctxt_t *uart_handle = serial_uart_get_ctxt(std_out_channel);
  if (ptr == 0) {
    return 0;
  }

  if (uart_handle == NULL) {
    return -1;
  }

  if (file != STDOUT_FILENO && file != STDERR_FILENO) {
    return -1;
  }
  return serialUartDriverWrite(uart_handle, ptr, len);
}

#endif

#if defined(__ICCARM__)

#pragma module_name = "?__write"

/*
 * If the __write implementation uses internal buffering, uncomment
 * the following line to ensure that we are called with "buffer" as 0
 * (i.e. flush) when the application terminates.
 */

size_t __write(int handle, const unsigned char *buffer, size_t size) {
  /* Remove the #if #endif pair to enable the implementation */
  if (std_out_channel >= MAX_UART_NUM) return _LLIO_ERROR;

  serial_uart_ctxt_t *uart_handle = serial_uart_get_ctxt(std_out_channel);

  if (buffer == 0) {
    return 0;
  }

  if (uart_handle == NULL) {
    return _LLIO_ERROR;
  }

  if (handle != _LLIO_STDOUT && handle != _LLIO_STDERR) {
    return _LLIO_ERROR;
  }

  return serialUartDriverWrite(uart_handle, (const char *)buffer, size);
}

#endif
