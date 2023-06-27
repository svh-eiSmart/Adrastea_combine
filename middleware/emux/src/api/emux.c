/*  ---------------------------------------------------------------------------

    (c) copyright 2018 Altair Semiconductor, Ltd. All rights reserved.

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

#include "emux.h"
#include "core/mux_util.h"
#include "core/ring_buffer.h"
#include "los/los_mux.h"
#include <assert.h>
#include <string.h>

/**
 * @typedef emux_transfer_t
 * Definition of emux RX transfer handle.
 * This is used internally to keep the status of rx transfer.
 */
typedef struct _emux_transfer {
  uint8_t *data;                    /**< Address of the buffer to hold the user data.*/
  size_t dataSize;                  /**< Remaining data count for incoming data for user.*/
  emuxRxDoneFp_t rxDoneCB;          /**< Callback of data arrival for nonblocking receive*/
  void *rxDoneParam;                /**< Parameter for rxDoneCB*/
  losMuxSemaphoreHandler_t rxMutex; /**< mutex to synchronize RX task and user task.*/
} emux_transfer_t;

/**
 * @typedef _emux_handle
 * Definition of general emux handler.
 * This is used internally and initialized by MUX_Init.
 */
typedef struct _emux_handle_definition {
  uint32_t muxID;                   /**< Physical muxID. We only support muxID 0 currently*/
  uint32_t virtualPortID;           /**< Virtaul port ID. We support 4 virtual ports on
                                         a physical serail.*/
  emux_transfer_t rxTransfer;       /**< RX transfer handle.*/
  void *rxRingBuffer;               /**< Background ring buffer to store incoming data
                                         when there is no user is asking for data.*/
  muxTxBuffFp_t muxTxFp;            /**< Function porinter to keep the address of TX function
                                         in mux core utility*/
  volatile uint8_t rxState;         /**< Bookkeeping the RX status of this virtual link.*/
  int portHandle;                   /**< Bookkeeping handler of mux core utility*/
  emuxRxEvFp_t rxEventCB;           /**< Callback function of user for RX data event.*/
  void *rxEvUserData;               /**< Parameter pass to RX data callback*/
  losMuxEventGroupHandle_t rxEvent; /**< OS event bit to synchronize RX task and user*/
} _emux_handle;

static int muxCreated[MAX_MUX_COUNT] = {0};
static _emux_handle *mh_book[MAX_MUX_COUNT][VIRTUAL_SERIAL_COUNT] = {0};
#define ERROR (-1)

#define EMUX_RX_COMPLETE 0x01

#if !defined(MIN)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

enum _emux_transfer_states { emux_RxIdle, emux_RxBusy };

static void MUX_PortReceiveCallback(const char charRecv, void *cookie) {
  _emux_handle *handle = (_emux_handle *)cookie;
  losMuxMutexTake(&handle->rxTransfer.rxMutex);
  if (handle->rxTransfer.dataSize) {
    *(handle->rxTransfer.data++) = charRecv;
    handle->rxTransfer.dataSize--;

    if (handle->rxTransfer.dataSize == 0) {
      losMuxMutexGive(&handle->rxTransfer.rxMutex);
      if (handle->rxEvent) {
        losMuxEventGroupSetBits(&handle->rxEvent, EMUX_RX_COMPLETE);
      }

      if (handle->rxTransfer.rxDoneCB) {
        handle->rxTransfer.rxDoneCB(emux_Transfer_Success, handle->rxTransfer.rxDoneParam);
        handle->rxTransfer.rxDoneCB = NULL;
      }

      handle->rxState = emux_RxIdle;
    } else {
      losMuxMutexGive(&handle->rxTransfer.rxMutex);
    }
  } else if (circBufGetSize(handle->rxRingBuffer) && circBufFree(handle->rxRingBuffer)) {
    circBufInsert(handle->rxRingBuffer, charRecv);
    losMuxMutexGive(&handle->rxTransfer.rxMutex);
  } else {
    losMuxMutexGive(&handle->rxTransfer.rxMutex);
  }

  if (handle->rxEventCB) {
    handle->rxEventCB(charRecv, handle->rxEvUserData);
  }
}

emux_handle_t MUX_Init(int muxID, int virtualPortID, uint8_t *ringBuffer, size_t ringBufferSize) {
  int retVal;
  _emux_handle *handle;
  if (!muxCreated[muxID]) {
    retVal = createMux(muxID, muxID, VIRTUAL_SERIAL_COUNT);
    if (ERROR == retVal) {
      return NULL;
    }
    muxCreated[muxID] = 1;
  }

  if (virtualPortID >= VIRTUAL_SERIAL_COUNT) {
    return NULL;
  }

  if (mh_book[muxID][virtualPortID] != NULL) {
    return NULL;
  }

  handle = losMuxMalloc(sizeof(_emux_handle));
  if (handle == NULL) {
    return NULL;
  }
  memset(handle, 0, sizeof(_emux_handle));

  handle->muxID = muxID;
  handle->virtualPortID = virtualPortID;

  if (allocCircBufUtil(&handle->rxRingBuffer, ringBuffer, ringBufferSize) == 0) {
    losMuxFree(handle);
    return NULL;
  }

  if (!losMuxCreateMutex(&handle->rxTransfer.rxMutex)) {
    freeCircBufUtil(&handle->rxRingBuffer);
    losMuxFree(handle);
    return NULL;
  }

  if (!losMuxCreateEventGroup(&handle->rxEvent)) {
    losMuxDeleteMutex(&handle->rxTransfer.rxMutex);
    freeCircBufUtil(&handle->rxRingBuffer);
    losMuxFree(handle);
    return NULL;
  }

  handle->rxTransfer.data = NULL;
  handle->rxTransfer.dataSize = 0;
  handle->rxEventCB = NULL;
  handle->rxEvUserData = NULL;

  handle->portHandle = bindToMuxVirtualPort(muxID, virtualPortID, MUX_PortReceiveCallback,
                                            (void *)handle, &handle->muxTxFp);
  mh_book[muxID][virtualPortID] = handle;
  return (emux_handle_t)handle;
}

int MUX_DeInit(emux_handle_t handle) {
  int muxID, virtualPortID;
  _emux_handle *pHandle = (_emux_handle *)handle;

  if (pHandle == NULL) {
    return eStatus_Fail;
  }

  muxID = pHandle->muxID;
  virtualPortID = pHandle->virtualPortID;

  unbindFromMuxVirtualPort(muxID, virtualPortID);

  if (pHandle->rxEvent) {
    losMuxDeleteEventGroup(&pHandle->rxEvent);
  }
  if (pHandle->rxTransfer.rxMutex) {
    losMuxDeleteMutex(&pHandle->rxTransfer.rxMutex);
  }
  if (pHandle->rxRingBuffer) {
    freeCircBufUtil(&pHandle->rxRingBuffer);
  }
  losMuxFree(pHandle);
  mh_book[muxID][virtualPortID] = NULL;
  return eStatus_Success;
}

int MUX_Send(emux_handle_t handle, const uint8_t *buffer, uint32_t length) {
  int ret;
  _emux_handle *pHandle = (_emux_handle *)handle;

  if (pHandle == NULL) {
    return emux_Transfer_Fail;
  }
  // assert(handle->muxTxFp);
  ret = pHandle->muxTxFp((void *)pHandle->portHandle, (const char *)buffer, (unsigned short)length);
  return (ret) ? emux_Transfer_Success : emux_Transfer_Fail;
}

int MUX_Receive_NonBlock(emux_handle_t handle, uint8_t *buffer, uint32_t length,
                         emuxRxDoneFp_t rxDoneCB, void *userData, int *recvLen) {
  uint32_t bytesToCopy = 0;
  uint32_t ringBufferLen = 0;
  _emux_handle *pHandle = (_emux_handle *)handle;

  if (pHandle == NULL) {
    return emux_Transfer_Fail;
  }

  if (pHandle->rxState == emux_RxBusy) {
    return emux_Transfer_RxBusy;
  }

  losMuxMutexTake(&pHandle->rxTransfer.rxMutex);
  pHandle->rxTransfer.data = buffer;
  pHandle->rxTransfer.dataSize = length;
  pHandle->rxTransfer.rxDoneCB = rxDoneCB;
  pHandle->rxTransfer.rxDoneParam = userData;
  pHandle->rxState = emux_RxBusy;

  if (circBufGetSize(pHandle->rxRingBuffer)) {
    ringBufferLen = circBufGetUsage(pHandle->rxRingBuffer);
    bytesToCopy = MIN(pHandle->rxTransfer.dataSize, ringBufferLen);
    *recvLen = bytesToCopy;
    if (bytesToCopy) {
      pHandle->rxTransfer.dataSize -= bytesToCopy;
      while (bytesToCopy--) {
        circBufGetChar(pHandle->rxRingBuffer, pHandle->rxTransfer.data++);
      }
    }
  }

  if (pHandle->rxTransfer.dataSize == 0) {
    losMuxMutexGive(&pHandle->rxTransfer.rxMutex);
    pHandle->rxState = emux_RxIdle;
    if (pHandle->rxTransfer.rxDoneCB) {
      pHandle->rxTransfer.rxDoneCB(emux_Transfer_Success, pHandle->rxTransfer.rxDoneParam);
    }
    return emux_Transfer_Success;
  } else {
    losMuxMutexGive(&pHandle->rxTransfer.rxMutex);
    return emux_Transfer_Ongoing;
  }
}

int MUX_ReceiveTimeout(emux_handle_t handle, uint8_t *buffer, uint32_t length, uint32_t timeoutMS) {
  uint32_t bytesToCopy = 0;
  uint32_t ringBufferLen = 0;
  int ret;
  losMuxEvBits_t ev;
  _emux_handle *pHandle = (_emux_handle *)handle;

  if (pHandle == NULL) {
    return emux_Transfer_Fail;
  }

  if (pHandle->rxState == emux_RxBusy) {
    return emux_Transfer_RxBusy;
  }

  losMuxMutexTake(&pHandle->rxTransfer.rxMutex);
  pHandle->rxTransfer.data = buffer;
  pHandle->rxTransfer.dataSize = length;
  pHandle->rxState = emux_RxBusy;

  if (circBufGetSize(pHandle->rxRingBuffer)) {
    ringBufferLen = circBufGetUsage(pHandle->rxRingBuffer);

    bytesToCopy = MIN(pHandle->rxTransfer.dataSize, ringBufferLen);
    if (bytesToCopy) {
      pHandle->rxTransfer.dataSize -= bytesToCopy;
      while (bytesToCopy--) {
        circBufGetChar(pHandle->rxRingBuffer, pHandle->rxTransfer.data++);
      }
    }
  }
  if (pHandle->rxTransfer.dataSize) {
    losMuxMutexGive(&pHandle->rxTransfer.rxMutex);
    /*Start to wait the RX complete event*/
    ev = losMuxEventGroupWait(&pHandle->rxEvent, EMUX_RX_COMPLETE, timeoutMS);

    losMuxMutexTake(&pHandle->rxTransfer.rxMutex);
    if (ev & EMUX_RX_COMPLETE || pHandle->rxTransfer.dataSize == 0) {
      ret = emux_Transfer_Success;
    } else {
      ret = emux_Transfer_Timeout;
    }
    losMuxEventGroupClearBits(&pHandle->rxEvent, EMUX_RX_COMPLETE);
  } else {
    ret = emux_Transfer_Success;
  }
  pHandle->rxTransfer.data = NULL;
  pHandle->rxTransfer.dataSize = 0;
  pHandle->rxState = emux_RxIdle;
  losMuxMutexGive(&pHandle->rxTransfer.rxMutex);
  return ret;
}

void MUX_StartRxDataEventCallback(emux_handle_t handle, emuxRxEvFp_t rxEventCallback,
                                  void *userData) {
  _emux_handle *pHandle = (_emux_handle *)handle;

  if (pHandle == NULL) {
    return;
  }

  pHandle->rxEvUserData = userData;
  pHandle->rxEventCB = rxEventCallback;
}

void MUX_StopRxDataEventCallback(emux_handle_t handle) {
  _emux_handle *pHandle = (_emux_handle *)handle;

  if (pHandle == NULL) {
    return;
  }

  pHandle->rxEventCB = NULL;
  pHandle->rxEvUserData = NULL;
}
