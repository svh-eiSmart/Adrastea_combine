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

#include "los/los_mux.h"
#include "fsl_lpuart.h"
#include "fsl_lpuart_freertos.h"
#include "serial_container.h"
typedef struct _muxSerialConfig {
  muxRxFp_t muxRxProcessFp;
  void *rxCookie;
  lpuart_rtos_config_t lpuart_config;
  serial_handle sh;
} muxSerialConfig_t;

muxSerialConfig_t muxPhysical[MAX_MUX_COUNT];

#define MUX0_UART_BASE LPUART1
#define MUX0_UART_BAUDRATE 115200
#define MUX0_UART_CLKSRC kCLOCK_Osc0ErClk
#define MUX0_UART_CLK_FREQ CLOCK_GetFreq(kCLOCK_Osc0ErClk)
#define MUX0_UART_IRQn LPUART1_IRQn

#define MUX0_UART_FLOWCONTROL false
#define MUX_RX_STREAM_LEN 4096

static uint8_t muxRXStreamBuffer[MAX_MUX_COUNT][MUX_RX_STREAM_LEN];

#define ERROR (-1)

static void uart_rx_callback(const char charRecv, void *recvCookie) {
  int muxID = (int)recvCookie;
  muxSerialConfig_t *pMuxPhysical = (muxSerialConfig_t *)&muxPhysical[muxID];
  pMuxPhysical->muxRxProcessFp(charRecv, pMuxPhysical->rxCookie);
}

static int muxTxFunc(void *portHandle, const char *txChar, unsigned short txCharLen) {
  int muxID = (int)portHandle;
  muxSerialConfig_t *pMuxPhysical = &muxPhysical[muxID];
  if (serialDriverContainerWrite(pMuxPhysical->sh, (const uint8_t *)txChar, txCharLen) !=
      SERIAL_CONTAINER_SUCCESS) {
    return ERROR;
  }
  return 0;
}

int halSerialConfigure(int muxID, muxRxFp_t rxProcessFp, void *appCookie, muxTxBuffFp_t *txCharFp) {
  muxSerialConfig_t *pMuxPhysical = &muxPhysical[muxID];
  if (muxID != MUX_0_ID) {
    return ERROR;
  }
  muxPhysical[muxID].muxRxProcessFp = rxProcessFp;
  muxPhysical[muxID].rxCookie = appCookie;
  switch (muxID) {
    case MUX_0_ID:
      pMuxPhysical->lpuart_config.base = MUX0_UART_BASE;
      pMuxPhysical->lpuart_config.baudrate = MUX0_UART_BAUDRATE;
      pMuxPhysical->lpuart_config.srcclk = MUX0_UART_CLK_FREQ;
      pMuxPhysical->lpuart_config.parity = kLPUART_ParityDisabled;
      pMuxPhysical->lpuart_config.stopbits = kLPUART_OneStopBit;
      pMuxPhysical->lpuart_config.buffer = muxRXStreamBuffer[muxID];
      pMuxPhysical->lpuart_config.buffer_size = MUX_RX_STREAM_LEN;
      NVIC_SetPriority(MUX0_UART_IRQn, 5);
      pMuxPhysical->sh = serialDriverContainerOpen(&pMuxPhysical->lpuart_config);
      serialDriverContainerIoctl(pMuxPhysical->sh, IOCTL_SERIAL_RX_CB_FUNC,
                                 (void *)uart_rx_callback, (void *)muxID);
      break;
    default:
      return ERROR;
      break;
  }

  *txCharFp = muxTxFunc;
  return 0;
}
