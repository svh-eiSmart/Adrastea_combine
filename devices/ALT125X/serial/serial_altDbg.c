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
#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
//#include "CLI/FreeRTOS_CLI.h"
//#include "serialContainer/serial_container.h"
#include "serial.h"
#include "circ_buf.h"

extern int uartIsrRxIndCnt[MAX_UART_NUM];
extern int uartIsrTxIndCnt[MAX_UART_NUM];
extern int uartRxDropDueToUnbindRxFunc[MAX_UART_NUM];
extern int uartCharRxParityErrCnt[MAX_UART_NUM];
extern int uartCharRxBrkErrCnt[MAX_UART_NUM];
extern int uartCharRxCnt[MAX_UART_NUM];
extern int uartCharTxCnt[MAX_UART_NUM];
extern int uartCharRxframeErrCnt[MAX_UART_NUM];
extern int uartIsrRxFifoFullCnt[MAX_UART_NUM];

extern int uartCharRxOverrunErrCnt[MAX_UART_NUM];
extern int ppp_dropCnt;

/* Need to be adress usign IOCTL ????? */
int getXmitCircBufUsageUart(enum hwSer_channelNumber chNum) {
  struct serial_uart_ctxt *serialFuncIfc = NULL;

  // serialFuncIfc =hw_serial_get_funcs(UART0);
  serialFuncIfc = serial_uart_get_ctxt(chNum);
  if (serialFuncIfc)
    return serialCircBufGetUsage(serialFuncIfc->xmitBuffer);
  else
    return 0;
}

BaseType_t cliSerialStat_cmd(int argc, char *argv[]) {
  unsigned char serialNum;
  const char *serialDesStr;

  if (argc > 2) goto err;

  if (argc == 1) {
    serialNum = UARTI0;
    serialDesStr = "UARTI0";
  } else {
    if (*argv[1] == '0') {
      serialNum = UART0;
      serialDesStr = "UART0";
    } else if (*argv[1] == '1') {
#ifdef ALT1250
      serialNum = UART1;
      serialDesStr = "UART1";
#else
      goto err;
#endif
    } else if (*argv[1] == '2') {
      serialNum = UARTI0;
      serialDesStr = "UARTI0";
    } else {
      printf("error got %c\n", *argv[1]);
      goto err;
    }
  }

#ifdef ALT1250
  printf(
      "%s stats - IsrRxIndCnt=%d RxCnt=%d IsrTxIndCnt=%d IsrTxCnt=%d frameErr=%d \
					   \nparityErr=%d BrkErr=%d overrunErr=%d RxUnBindDrop=%d \
					   \nuart0Usage=%d uart1Usage=%d uartI0Usage=%d swUartRxFull=%d\n",
      serialDesStr, uartIsrRxIndCnt[serialNum], uartCharRxCnt[serialNum],
      uartIsrTxIndCnt[serialNum], uartCharTxCnt[serialNum], uartCharRxframeErrCnt[serialNum],
      uartCharRxParityErrCnt[serialNum], uartCharRxBrkErrCnt[serialNum],
      uartCharRxOverrunErrCnt[serialNum],
      /*ppp_dropCnt,*/
      uartRxDropDueToUnbindRxFunc[serialNum], getXmitCircBufUsageUart(UART0),
      getXmitCircBufUsageUart(UART1), getXmitCircBufUsageUart(UARTI0),
      uartIsrRxFifoFullCnt[serialNum]);
#else
  printf(
      "%s stats - IsrRxIndCnt=%d RxCnt=%d IsrTxIndCnt=%d IsrTxCnt=%d frameErr=%d \
					   \nparityErr=%d BrkErr=%d overrunErr=%d RxUnBindDrop=%d \
					   \nuart0Usage=%d uartI0Usage=%d swUartRxFull=%d\n",
      serialDesStr, uartIsrRxIndCnt[serialNum], uartCharRxCnt[serialNum],
      uartIsrTxIndCnt[serialNum], uartCharTxCnt[serialNum], uartCharRxframeErrCnt[serialNum],
      uartCharRxParityErrCnt[serialNum], uartCharRxBrkErrCnt[serialNum],
      uartCharRxOverrunErrCnt[serialNum],
      /*ppp_dropCnt,*/
      uartRxDropDueToUnbindRxFunc[serialNum], getXmitCircBufUsageUart(UART0),
      getXmitCircBufUsageUart(UARTI0), uartIsrRxFifoFullCnt[serialNum]);
#endif
  return pdFALSE;
err:
#ifdef ALT1250
  printf("Syntax error param given should be 0, 1 or 2(UARTI0)");
#else
  printf("Syntax error param given should be 0 or 2(UARTI0)");
#endif
  return pdFALSE;
}
