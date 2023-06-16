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
#include <unistd.h>
#include <stdio.h>
#include <newlibPort.h>

DRV_UART_Handle *DRV_UART_Get_Handle(DRV_UART_Port port);

static DRV_UART_Handle *std_out_uart_handle = NULL;

void newlib_set_stdout_port(DRV_UART_Port uart_inst) {
  switch (uart_inst) {
    case DRV_UART_F0:
#ifdef ALT1250
    case DRV_UART_F1:
#endif
    case DRV_UART_I0:
      std_out_uart_handle = DRV_UART_Get_Handle(uart_inst);
      break;
    default:
      break;
  }
}

void _putchar(char character) { write(STDOUT_FILENO, &character, sizeof(character)); }

int _write(int file, char *ptr, int len) {

  if (ptr == 0) {
    return 0;
  }
  if (file != STDOUT_FILENO && file != STDERR_FILENO) {
    return -1;
  }

  if (std_out_uart_handle == NULL) {
    return -1;
  }
  if( std_out_uart_handle == NULL ) {
    return 0;
  }
  return DRV_UART_Send_Block(std_out_uart_handle, (int8_t *)ptr, len);
}
