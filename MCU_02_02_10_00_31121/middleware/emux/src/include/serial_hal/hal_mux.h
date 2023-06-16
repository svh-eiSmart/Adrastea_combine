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
/**
 * @file hal_mux.h
 */
#ifndef CORE_CORE_UTILS_HAL_MUX_H_
#define CORE_CORE_UTILS_HAL_MUX_H_
/**
 * @defgroup emux_hal Emux Serial Hardware Abstraction Layer.
 * @{
 */
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define VIRTUAL_SERIAL_COUNT 4 /**< Definition of virtual port count of each mux port. */
#define MAX_MUX_COUNT 1        /**< Definition of muxed physical serial port count. */
/****************************************************************************
 * Public Types
 ****************************************************************************/
typedef void *halMuxHdl_t;
/**
 * @typedef muxRxFp_t
 * RX function callback of mux_util
 * @param[in] carRecv: Received RX data.
 * @param[in] recvCookie : Parameter for this callback.
 */
typedef void (*muxRxFp_t)(uint8_t charRecv, void *recvCookie);
/**
 * @typedef muxTxBuffFp_t
 * TX function for mux_util to send serial data.
 * @param[in] porHandle: Handler of tx function
 * @param[in] txCharBuf: Data buffer to send to serial.
 * @param[in] txCharBufLen: TX buffer length.
 */
typedef int32_t (*muxTxBuffFp_t)(halMuxHdl_t *portHandle, const uint8_t *txCharBuf,
                                 uint16_t txCharBufLen);

/*******************************************************************************
 * API
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif /* _cplusplus */
/**
 * Platform serial hardware configuration
 *
 * halSerialConfigure(). Make connection between emux core (mux_util) and platform serial driver.
 * This API should proved a TX function pointer for mux_util to send mux-packed data and get the RX
 * callback from mux_util and trigger it once there is any data received from serial port.
 * @param [in] muxID: Physical serial ID to distinguish serial port if mux on mutiple serial port
 * supported.
 * @param [in] rxProcessFp: RX callback provided by mux_util and the implementation should notify it
 * via this callback once there is any incoming data.
 * @param [in] appCookie: User parameter of RX callback.
 * @param [out] txCharFp: Implementation of this API should provide a TX function pointer for upper
 * layer (mux_util) to send data to serial port.
 *
 * @return Return serial hal handler if OK, otherwise return NULL
 */
halMuxHdl_t halSerialConfigure(int32_t muxID, muxRxFp_t rxProcessFp, void *appCookie,
                               muxTxBuffFp_t *txCharFp);
#if defined(__cplusplus)
}
#endif
/** @} emux_hal */
#endif
