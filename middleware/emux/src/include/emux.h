/*  ---------------------------------------------------------------------------

    (c) copyright 2018 Altair Semiconductor, Ltd. All rights reserved.

    This software, in source or object form (the "Software"), is the
    property of Altair Semiconductor Ltd. (the "Company") and/or its
    licensors, which have all right, title and interest therein, You
    may use the Software only in  accordance with the terms of written
    license agreement between you and the Company (the "License").
    Except as expressly stated in the License, the Company grants no
    copy or modify the Software. You may use the source code of the
    Software only for your internal purposes and may not distribute the
    source code of the Software, any part thereof, or any derivative work
    thereof, to any third party, except pursuant to the Company's prior
    written consent.
    The Software is the confidential information of the Company.

   ------------------------------------------------------------------------- */
/**
 * @file emux.h
 */
#ifndef CORE_UTIL_EMUX_H
#define CORE_UTIL_EMUX_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "los_mux_types.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/
/**
 * @defgroup emux_library Emux
 * @{
 */
/**
 * @defgroup emux_types Emux Types
 * @{
 */

/**
 * Definitions of result code for emux.
 */
enum _emux_general_status {
  eStatus_Success, /**< Result code on success */
  eStatus_Fail     /**< Result code on generic failure */
};

/**
 * Definitions of result code for MUX_Send and MUX_Receive
 */
enum _emux_transfer_result {
  emux_Transfer_Success, /**< Result code on success */
  emux_Transfer_Fail,    /**< Result code on generic failure */
  emux_Transfer_RxBusy,  /**< Result code on rx busy */
  emux_Transfer_Ongoing, /**< Result code on rx request send complete.This is
                              returned when MUX_Receive_Nonblock is called */
  emux_Transfer_Timeout, /**< Result code on receive timeout.
                              The timeout setting is given as a
                              parameter for MUX_Receive blocking call*/
};

/**
 * @typedef emuxRxDoneFp_t
 * Definition of callback function.
 * Since MUX_Receive_Nonblock() is an asynchronous API,
 * the result is notified by this function
 * @param[in] result : The result of the RX request.
 * @param[in] param : Parameter for this callback.
 */
typedef void (*emuxRxDoneFp_t)(uint8_t result, void *param);

/**
 * @typedef emuxRxEvFp_t
 * Definition of callback function.
 * After RX event callback is registered by MUX_StartRxDataEventCallback.
   The callback function will be triggered on each RX data arrival.
 * @param[in] data : The RX data content.
 * @param[in] param: Parameter to pass into the callback
*/
typedef void (*emuxRxEvFp_t)(uint8_t data, void *param);

/**
 * @typedef emux_handle_t
 * Definition of general emux handler.
 * API user needs to use this handler to send and receive data.
 */
typedef void *emux_handle_t;
/** @} emux_types */

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* _cplusplus */

/**
 * @defgroup emux_apis Emux APIs
 * @{
 */
/**
 * MUX virtual link initialization.
 *
 * MUX_Init(). Initial and bind on the specified virtual link.
 * The configuration is stored in the handle which is used to send and recieve data.
 * This example shows how to use this function to initialize virtual link 0.
 * @code
 *  #define BUF_LEN 1024
 *  emux_handle_t handle;
 *  uint8_t backgroundBuffer[BUF_LEN]
 *  handle = MUX_Init(0, 0, backgroundBuffer, BUF_LEN);
 * @endcode
 * @param [in] muxID: Physical muxID. We only support muxID 0 currently
 * @param [in] virtualPortID: Virtaul port ID. We support 4 virtual ports on
 * a physical serail.
 * @param [in] ringBuffer: Background ring buffer to store incoming data
 * when there is no user is asking for data. This is only needed when user would like to
 * receive proactively
 * @param [in] ringBufferSize: Background ring buffer size.
 * See @ref emux_handle_t
 * @return On success, return emux handler@ref emux_handle_t.
 * On failure NULL is returned.
 */
emux_handle_t MUX_Init(int muxID, int virtualPortID, uint8_t *ringBuffer, size_t ringBufferSize);

/**
 * MUX virtual link deinitialization.
 *
 * MUX_DeInit() Unbind and deinitial the sepcified virtual link.
 *
 * @param [in] handle Pointer to emux_handle_t
 * This handle is initialzed by MUX_Init
 *
 * @return On success, handle for emux is returned. On failure,
 * NULL is returned.
 */
int MUX_DeInit(emux_handle_t handle);

/**
 * Send data with the length of bytes on a virtual link
 *
 * MUX_Send() Send data on a virtual link with the initialized handle.
 * This example shows how to use this function to send data on virtual link 0.
 * @code
 *  #define BUF_LEN 32
 *  uint8_t wBuffer[BUF_LEN] = {0};
 *  sprintf(wBuffer, "AT\r");
 *  emux_handle_t handle;
 *  handle = MUX_Init(0, 0, NULL, 0);
 *  MUX_Send(handle, wBuffer, strlen(wBuffer));
 * @endcode
 * @param [in] handle: emux handle initialized by @ref MUX_Init
 * @param [in] buffer: Buffer which stored the data to send.
 * @param [in] length: Length of bytes stored in the buffer.
 *
 * @return On success, @ref emux_Transfer_Success is returned. On failure,
 * @ref emux_Transfer_Fail is returned.
 */
int MUX_Send(emux_handle_t handle, const uint8_t *buffer, uint32_t length);

/**
 * Receive data synchronously on a virtual link with timeout.
 *
 * MUX_ReceiveTimeout() Receive data on the dedicated virtual port into buffer for
 * the specified length. The task will be blocked here for timeoutMS if no data received.
 * This example show how to receive data on virtual link 0 for four bytes
 * and the maximum waiting time is 200 ms.
 * @code
 *  #define BUF_LEN 1024
 *  emux_handle_t handle;
 *  uint8_t backgroundBuffer[BUF_LEN]
 *  handle = MUX_Init(0, 0, rxRingBuffer, BUF_LEN);
 *  uint8_t rBuffer[4];
 *  ret = MUX_ReceiveTimeout(handle, rBuffer, 4, 200);
 *  if (ret == emux_Transfer_Success)
 *  {
 *     ...
 *  }
 *  else if (ret == emux_Transfer_Timeout)
 *  {
 *     ...
 *  }
 * @endcode
 * @param [in] handle: emux handle initialized by @ref MUX_Init
 * @param [in] buffer: Buffer to store the incoming data.
 * @param [in] length: Length of bytes to receive.
 * @param [in] timeoutMS: Maximux waiting time when data is arrival.
 *
 * @return See @ref _emux_transfer_result.
 */
int MUX_ReceiveTimeout(emux_handle_t handle, uint8_t *buffer, uint32_t length, uint32_t timeoutMS);

/**
 * Receive data synchronously on a virtual link.
 *
 * MUX_Receive() Receive data on the dedicated virtual port into buffer for
 * the specified length. The task will be blocked here if no data received.
 * This example show how to receive data on virtual link 0 for four bytes
 * @code
 *  #define BUF_LEN 1024
 *  emux_handle_t handle;
 *  uint8_t backgroundBuffer[BUF_LEN]
 *  handle = MUX_Init(0, 0, backgroundBuffer, BUF_LEN);
 *  uint8_t rBuffer[4];
 *  ret = MUX_Receive(handle, rBuffer, 4);
 *  if (ret == emux_Transfer_Success)
 *  {
 *     ...
 *  }
 * @endcode
 * @param [in] handle: emux handle initialized by @ref MUX_Init
 * @param [in] buffer: Buffer to store the incoming data.
 * @param [in] length: Length of bytes to receive.
 *
 * @return See @ref _emux_transfer_result.
 */
static inline int MUX_Receive(emux_handle_t handle, uint8_t *buffer, uint32_t length) {
  return MUX_ReceiveTimeout(handle, buffer, length, LOS_TIMEOUT_FOREVER);
}

/**
 * Receive data asynchronously on a virtual link.
 *
 * MUX_Receive_NonBlock(). The data on the dedicated virtual port
 * will be filled into buffer with specified length.
 * This function returns immediately no matter the buffer is filled or not.
 * Caller will get the notification via rxDoneCB.
 * The following example shows how to use this function to receive four bytes of data.
 * @code
 *  void MUX_RxComplete(uint8_t result, void *param)
 *  {
 *     ...
 *  }
 *  #define BUF_LEN 1024
 *  emux_handle_t handle;
 *  uint8_t backgroundBuffer[BUF_LEN];
 *  handle = MUX_Init(0, 0, backgroundBuffer, BUF_LEN);
 *  uint8_t rBuffer[4];
 *  MUX_Receive_NonBlock(handle, data, 4, MUX_RxComplete, (void *) &rBuffer, &recv);
 * @endcode
 * @param [in] handle: emux handle initialized by @ref MUX_Init
 * @param [in] buffer: Buffer to store the incoming data.
 * @param [in] length: Length of bytes to receive.
 * @param [in] rxDoneCB: Callback function for data arrived notification.
 * @param [in] userData: Parameter for rxDoneCB.
 * @param [in] recvLen: Data length in the buffer when this function returned..
 *
 * @return See @ref _emux_transfer_result.
 */
int MUX_Receive_NonBlock(emux_handle_t handle, uint8_t *buffer, uint32_t length,
                         emuxRxDoneFp_t rxDoneCB, void *userData, int *recvLen);

/**
 * Register RX data event callback.
 *
 * MUX_StartRxDataEventCallback() Pass the callback fp and parameter to the callback,
 * and then if the virtual port has incoming data, the callback will be triggered.
 * @code
 *  static void MUX_RxReceived(uint8_t data, void *param)
 *  {
 *      emux_handle_t *handle = (emux_handle_t*) param;
 *      ...
 *  }
 *
 *  emux_handle_t handle;
 *  handle = MUX_Init(0, 0, NULL, 0);
 *  MUX_StartRxDataEventCallback(handle, MUX_RxReceived, &handle);
 * @endcode
 * @param [in] handle: emux handle initialized by @ref MUX_Init
 * @param [in] rxEventCallback: Callback function to receive the data incoming event.
 * @param [in] userData: Parameter for rxEventCallback
 *
 * @return void.
 */
void MUX_StartRxDataEventCallback(emux_handle_t handle, emuxRxEvFp_t rxEventCallback,
                                  void *userData);

/**
 * Unregister RX data event callback
 *
 * MUX_StopRxDataEventCallback(). Stop rx data event notification.
 * @param [in] handle: emux handle initialized by @ref MUX_Init
 *
 * @return void.
 */
void MUX_StopRxDataEventCallback(emux_handle_t handle);
/** @} emux_apis */
/** @} emux_library */

#if defined(__cplusplus)
}
#endif

#endif
