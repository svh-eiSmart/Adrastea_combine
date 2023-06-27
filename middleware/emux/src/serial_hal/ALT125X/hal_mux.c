/****************************************************************************
 *
 *  (c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.
 *
 *  This software, in source or object form (the "Software"), is the
 *  property of Altair Semiconductor Ltd. (the "Company") and/or its
 *  licensors, which have all right, title and interest therein, You
 *  may use the Software only in  accordance with the terms of written
 *  license agreement between you and the Company (the "License").
 *  Except as expressly stated in the License, the Company grants no
 *  licenses by implication, estoppel, or otherwise. If you are not
 *  aware of or do not agree to the License terms, you may not use,
 *  copy or modify the Software. You may use the source code of the
 *  Software only for your internal purposes and may not distribute the
 *  source code of the Software, any part thereof, or any derivative work
 *  thereof, to any third party, except pursuant to the Company's prior
 *  written consent.
 *  The Software is the confidential information of the Company.
 *
 ****************************************************************************/

#include "los/los_mux.h"
#include "serial.h"
#include "serial_container.h"

typedef struct _muxSerialConfig {
  muxRxFp_t muxRxProcessFp;
  void *rxCookie;
  sHuart uartParam;
  void *serialhdl;
} muxSerialConfig_t;

muxSerialConfig_t muxPhysical[MAX_MUX_COUNT];

#define MUX0_UART_INSTANCE ACTIVE_UARTI0
#define MUX_RX_STREAM_LEN (4096)

#define ERROR (-1)

static int muxTxFunc(void *portHandle, const char *txChar, unsigned short txCharBufLen) {
  int muxID = (int)portHandle;
  muxSerialConfig_t *pMuxPhysical = &muxPhysical[muxID];
  serial_write(pMuxPhysical->serialhdl, (char *)txChar, txCharBufLen);
  return 0;
}

static void uart_rx_callback(const char data, void *appCookie) {
  int muxID = (int)appCookie;
  muxSerialConfig_t *pMuxPhysical = (muxSerialConfig_t *)&muxPhysical[muxID];
  pMuxPhysical->muxRxProcessFp(data, pMuxPhysical->rxCookie);
  return;
}

int halSerialConfigure(int muxID, muxRxFp_t rxProcessFp, void *appCookie, muxTxBuffFp_t *txCharFp) {
  muxSerialConfig_t *pMuxPhysical = &muxPhysical[muxID];
  unsigned int rxBufferSize = MUX_RX_STREAM_LEN;
  if (muxID >= MAX_MUX_COUNT) {
    return ERROR;
  }
  muxPhysical[muxID].muxRxProcessFp = rxProcessFp;
  muxPhysical[muxID].rxCookie = appCookie;
  switch (muxID) {
    case MUX_0_ID:
      serial_load_defconfig(MUX0_UART_INSTANCE, &pMuxPhysical->uartParam);
      pMuxPhysical->serialhdl = serial_open(&pMuxPhysical->uartParam);
      serial_ioctl(pMuxPhysical->serialhdl, IOCTAL_SERIAL_SW_RX_FIFO, (void *)&rxBufferSize, NULL);

      serial_ioctl(pMuxPhysical->serialhdl, IOCTAL_SERIAL_RX_CB_FUNC, (void *)uart_rx_callback,
                   (void *)muxID);
      break;
    default:
      return ERROR;
      break;
  }
  *txCharFp = muxTxFunc;
  return 0;
}
