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
/**
 * @file gdma.h
 */
#ifndef GDMA_ALT125X_H_
#define GDMA_ALT125X_H_

/**
 * @defgroup gdma_driver GDMA Driver
 * @{
 */

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/**
 * @defgroup gdma_constants GDMA Constants
 * @{
 */

#define GDMA_OK 0                   /**< GDMA status OK */
#define GDMA_INVALID_CH_NUMBER 3002 /**< Invalid pin/port number */
#define GDMA_TIMEOUT 3003           /**< Timeout expired */

/** @} gdma_constants */

/****************************************************************************
 * Public Data Types
 ****************************************************************************/
/**
 * @defgroup gdma_types GDMA Types
 * @{
 */

/**
 * @brief Enumeration of GDMA channel.
 */
typedef enum gdma_enum_t {
  GDMA_CH_0 = 0, /**< GDMA Channel 0 */
  GDMA_CH_1,     /**< GDMA Channel 1 */
  GDMA_CH_2,     /**< GDMA Channel 2 */
  GDMA_CH_3,     /**< GDMA Channel 3 */
  GDMA_CH_4,     /**< GDMA Channel 4 */
  GDMA_CH_5,     /**< GDMA Channel 5 */
  GDMA_CH_6,     /**< GDMA Channel 6 */
  GDMA_CH_7,     /**< GDMA Channel 7 */
  GMDA_CH_MAX    /**< Maximum channel */
} gdma_ch_enum_t;

/**
 * @brief Definition of GDMA callback function prototype.
 */
typedef void (*gdma_callback)(int error, gdma_ch_enum_t channel);

/** @} gdma_types */

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
/**
 * @defgroup gmda_apis GDMA APIs
 * @{
 */

/**
 * @brief Initialization function for GDMA.
 *
 */
void InitDma(void);

/**
 * @brief gdma_initiate_transfer() performs a GDMA transferring with the size of n from s2 to s1
 * using channel
 *
 * @param [out] s1: Destination pointer of DMA.
 * @param [in] s2: Source pointer of DMA.
 * @param [in] n: Size of the data to be transferred by DMA.
 * @param [in] channel: Target DMA channel to be operated.
 * @return int
 * Return Code               | Reason/Cause
 * --------------------------| --------------------------
 * GDMA_OK                   | Operation succeeded
 * -GDMA_INVALID_CH_NUMBER   | Invalid channel number
 */
int gdma_initiate_transfer(void *s1, const void *s2, size_t n, gdma_ch_enum_t channel);

/**
 * @brief gdma_transfer_status() obtains the status of GDMA transferring on specified channel.
 *
 * @param [in] channel: Target channel to obtain the status.
 * @return int
 * Return Code               | Reason/Cause
 * --------------------------| --------------------------
 * 0                         | Transfer completed
 * 1                         | Transfer ongoing
 */
int gdma_transfer_status(gdma_ch_enum_t channel);

/**
 * @brief gdma_register_interrupt() Registerring the callback function for notification on
 * completed.
 *
 * @param [in] channel: Target channel to be registerred of callback.
 * @param [in] irq_handler: Handler function for GDMA interrupt
 * @return int
 * Return Code               | Reason/Cause
 * --------------------------| --------------------------
 * GDMA_OK                   | Operation succeeded
 * -GDMA_INVALID_CH_NUMBER   | Invalid channel number
 */
int gdma_register_interrupt(gdma_ch_enum_t channel, gdma_callback irq_handler);

/**
 * @brief gdma_transfer_blocking() performs blocking DMA transfer
 *
 * @param [out] s1: Destination pointer of DMA.
 * @param [in] s2: Source pointer of DMA.
 * @param [in] n: Size of the data to be transferred by DMA.
 * @param [in] channel: Target DMA channel to be operated.
 * @param [in] timeout: Time in ticks to block before completed/failed.
 * @return int
 * Return Code               | Reason/Cause
 * --------------------------| --------------------------
 * GDMA_OK                   | Operation succeeded
 * -GDMA_INVALID_CH_NUMBER   | Invalid channel number
 * -GDMA_TIMEOUT             | Tranferring timeout
 */
int gdma_transfer_blocking(void *s1, const void *s2, size_t n, gdma_ch_enum_t channel,
                           uint32_t timeout);

/** @} gdma_apis */

#undef EXTERN
#ifdef __cplusplus
}
#endif

/** @} gdma_driver */

#endif /* GDMA_ALT125X_H_ */
