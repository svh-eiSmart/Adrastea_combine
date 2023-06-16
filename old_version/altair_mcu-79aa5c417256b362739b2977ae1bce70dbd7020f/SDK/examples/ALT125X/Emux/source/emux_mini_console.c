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

/* Standard includes. */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "newlibPort.h"
/* Middleware includes. */
#include "emux.h"
#include "mini_console.h"

// extern logical_hw_representation_id_t ConfigActiveUartCli;
#define CMD_NUM_OF_PARAM_MAX 12
#define CMD_PARAM_LENGTH 128

#define MUX_BACKGROUND_BUFFER_SIZE 32
#define AT_1_MUX_CH 1
#define AT_2_MUX_CH 2

static void MUX_RxReceived(uint8_t data, void *param) { console_write((char *)&data, 1); }

int do_atb(char *s) {
  emux_handle_t mux_handle;
  char c;
  uint8_t data;
  int at_solo_mux_ports[2] = {AT_1_MUX_CH, AT_2_MUX_CH};
  int virtual_port;
  int p = atoi(s) - 1;
  if (p >= 2) {
    printf("Parameter_outof_range\r\n");
    return 1;
  }

  virtual_port = at_solo_mux_ports[p];

  mux_handle = MUX_Init(0, virtual_port, NULL, 0);
  if (mux_handle == NULL) {
    printf("Cannot open AT bridge on MUX%d\r\n", virtual_port);
    return 1;
  }
  MUX_StartRxDataEventCallback(mux_handle, MUX_RxReceived, NULL);
  printf("Open AT %d (Press Ctrl+D to exit)\r\n", p + 1);

  do {
    {
      console_read(&c, 1);
      data = c & 0xff;
      if (data != 4) {
        MUX_Send(mux_handle, (const uint8_t *)&data, 1);
      }
    }
  } while (data != 4);
  MUX_StopRxDataEventCallback(mux_handle);
  MUX_DeInit(mux_handle);
  printf("AT bridge %d closed.\r\n", p + 1);
  return 0;
}
