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

#include "DRV_UART.h"
#include "serial.h"
#include "alt_osal.h"
#include "serial_hal/hal_mux.h"

typedef struct _muxSerialConfig {
  muxRxFp_t muxRxProcessFp;
  void *rxCookie;
  serial_handle sHandle;
  alt_osal_task_handle rxTaskHandle;
} muxSerialConfig_t;

static muxSerialConfig_t muxPhysical[MAX_MUX_COUNT] = {0};

#define MUX0_UART_INSTANCE ACTIVE_UARTI0
#define MUX_RX_STREAM_LEN (4096)
#define MAX_RX_TASK_NAME (16)
#define RX_TASK_STACK_SIZE (4096)

static int32_t muxTxFunc(halMuxHdl_t *portHandle, const uint8_t *txChar, uint16_t txCharBufLen) {
  muxSerialConfig_t *pMuxPhysical = (muxSerialConfig_t *)portHandle;
  serial_write(pMuxPhysical->sHandle, (void *)txChar, txCharBufLen);
  return 0;
}

static void emuxRxTask(void *pvParameters) {
  uint8_t data;
  muxSerialConfig_t *pMuxPhysical = (muxSerialConfig_t *)pvParameters;
  while (1) {
    if (serial_read(pMuxPhysical->sHandle, &data, 1) == 1)
      pMuxPhysical->muxRxProcessFp(data, pMuxPhysical->rxCookie);
  }
  alt_osal_delete_task(NULL);
}

halMuxHdl_t halSerialConfigure(int32_t muxID, muxRxFp_t rxProcessFp, void *appCookie,
                               muxTxBuffFp_t *txCharFp) {
  alt_osal_task_attribute task_param = {0};
  char rxTaskName[MAX_RX_TASK_NAME] = {0};
  muxSerialConfig_t *pMuxPhysical = NULL;
  if (muxID >= MAX_MUX_COUNT) {
    return NULL;
  }
  pMuxPhysical = &muxPhysical[muxID];
  snprintf(rxTaskName, MAX_RX_TASK_NAME, "EMUX RX%ld", muxID);

  switch (muxID) {
    case 0:
      if (pMuxPhysical->sHandle == NULL) {
        pMuxPhysical->sHandle = serial_open(MUX0_UART_INSTANCE);
        if (pMuxPhysical->sHandle == NULL) return NULL;
      }

      if (pMuxPhysical->rxTaskHandle == NULL) {
        task_param.function = emuxRxTask;
        task_param.name = rxTaskName;
        task_param.priority = ALT_OSAL_TASK_PRIO_HIGH;
        task_param.stack_size = RX_TASK_STACK_SIZE;
        task_param.arg = (void *)pMuxPhysical;
        if (alt_osal_create_task(&pMuxPhysical->rxTaskHandle, &task_param) != 0) return NULL;
      }

      pMuxPhysical->muxRxProcessFp = rxProcessFp;
      pMuxPhysical->rxCookie = appCookie;
      break;
    default:
      return NULL;
      break;
  }
  *txCharFp = muxTxFunc;
  return (halMuxHdl_t)pMuxPhysical;
}
