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
 * @file DRV_GDMA.h
 */
#ifndef DRV_GDMA_H_
#define DRV_GDMA_H_

/*Stardard Header */
#include <stdint.h>

/**
 * @defgroup gdma_driver GDMA Driver
 * @{
 */

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/**
 * @defgroup drv_gdma_constants GDMA Constants
 * @{
 */

/* I2C Transaction Event */
#define GDMA_EVENT_TRANSACTION_DONE (1UL << 0)  /**< Transactiond done */
#define GDMA_EVENT_TRANSACTION_ERROR (1UL << 1) /**< Transactiond error */

/** @} drv_gdma_constants */

/****************************************************************************
 * Public Data Types
 ****************************************************************************/
/**
 * @defgroup drv_gdma_types GDMA Types
 * @{
 */

/**
 * @brief Enumeration of GDMA status.
 */
typedef enum {
  DRV_GDMA_OK = 0,           /**< Operation succeeded */
  DRV_GDMA_ERROR_UNKNOWN,    /**< Unknown/Unspecified error */
  DRV_GDMA_ERROR_INVALID_CH, /**< Invalid channel number */
  DRV_GDMA_ERROR_TIMEOUT,    /**< Operation timeout */
  DRV_GDMA_ERROR_UNINIT,     /**< Driver not yet initialized */
  DRV_GDMA_ERROR_INIT,       /**< Driver already initialized */
  DRV_GDMA_ERROR_PWRMNGR     /**< Power management operation error */
} DRV_GDMA_Status;

/**
 * @brief Enumeration of GDMA channel.
 */
typedef enum {
  DRV_GDMA_Channel_0 = 0, /**< GDMA Channel 0 */
  DRV_GDMA_Channel_1,     /**< GDMA Channel 1 */
  DRV_GDMA_Channel_2,     /**< GDMA Channel 2 */
  DRV_GDMA_Channel_3,     /**< GDMA Channel 3 */
  DRV_GDMA_Channel_4,     /**< GDMA Channel 4 */
  DRV_GDMA_Channel_5,     /**< GDMA Channel 5 */
  DRV_GDMA_Channel_6,     /**< GDMA Channel 6 */
  DRV_GDMA_Channel_7,     /**< GDMA Channel 7 */
  DRV_GDMA_Channel_MAX    /**< Maximum channel */
} DRV_GDMA_Channel;

/**
 * @brief Definition of GDMA callback function prototype.
 */
typedef struct {
  void *user_data; /**< User data on event callback */
  void (*event_callback)(uint32_t event, DRV_GDMA_Channel channel,
                         void *user_data); /**< Event callback */
} DRV_GDMA_EventCallback;

/** @} drv_gdma_types */

/*! @cond Doxygen_Suppress */
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif
/*! @endcond */

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
/**
 * @defgroup drv_gmda_apis GDMA APIs
 * @{
 */

/**
 * @brief Initialize GDMA driver
 *
 * @return DRV_GDMA_Status
 */
DRV_GDMA_Status DRV_GDMA_Initialize(void);

/**
 * @brief Uninitialize GDMA driver
 *
 * @return DRV_I2C_Status
 */
DRV_GDMA_Status DRV_GDMA_Uninitialize(void);

/**
 * @brief DRV_GDMA_RegisterIrqCallback() Registerring the callback function for notification on
 * completed.
 *
 * @param [in] channel: Target channel to be registerred of callback.
 * @param [in] callback: Callback function for GDMA interrupt
 * @return DRV_GDMA_Status
 */
DRV_GDMA_Status DRV_GDMA_RegisterIrqCallback(DRV_GDMA_Channel channel,
                                             DRV_GDMA_EventCallback *callback);

/**
 * @brief DRV_GDMA_Transfer_Block() performs a blocking GDMA transferring with the size of n from
 * src to dest using channel
 *
 * @param [in] channel: Target DMA channel to be operated.
 * @param [out] dest: Destination pointer of DMA.
 * @param [in] src: Source pointer of DMA.
 * @param [in] n: Size of the data to be transferred by DMA.
 * @return DRV_GDMA_Status
 */
DRV_GDMA_Status DRV_GDMA_Transfer_Block(DRV_GDMA_Channel channel, void *dest, const void *src,
                                        size_t n);

/**
 * @brief DRV_GDMA_Transfer_Nonblock() performs a non-blocking GDMA transferring with the size of n
 * from src to dest using channel
 *
 * @param [in] channel: Target DMA channel to be operated.
 * @param [out] dest: Destination pointer of DMA.
 * @param [in] src: Source pointer of DMA.
 * @param [in] n: Size of the data to be transferred by DMA.
 * @return DRV_GDMA_Status
 */
DRV_GDMA_Status DRV_GDMA_Transfer_Nonblock(DRV_GDMA_Channel channel, void *dest, const void *src,
                                           size_t n);

/** @} drv_gmda_apis */

/*! @cond Doxygen_Suppress */
#undef EXTERN
#ifdef __cplusplus
}
#endif
/*! @endcond */

/** @} gdma_driver */

#endif /* DRV_GDMA_H_ */
