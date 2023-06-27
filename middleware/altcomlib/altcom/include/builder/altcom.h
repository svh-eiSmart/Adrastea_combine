/****************************************************************************
 * modules/altcom/ include/builder/altcom.h
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

/**
 * @file altcom.h
 */

#ifndef __MODULES_ALTCOM_INCLUDE_BUILDER_ALTCOM_H
#define __MODULES_ALTCOM_INCLUDE_BUILDER_ALTCOM_H

/**
 * @defgroup altcom ALTCOM Infrastructure APIs
 * @{
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "buffpool.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

/**
 * @defgroup altcomdbgmsg ALTCOM Debug Message Level
 * @{
 */

/**
 * @brief
 * Definition of debuuging message level of ALTCOM
 */

typedef enum {
  ALTCOM_DBG_NONE = 0, /**< Output nothing */
  ALTCOM_DBG_ERROR,    /**< Output error level message */
  ALTCOM_DBG_WARNING,  /**< Output wanring level message */
  ALTCOM_DBG_NORMAL,   /**< Output normal level message */
  ALTCOM_DBG_INFO,     /**< Output inforamtion level message */
  ALTCOM_DBG_DEBUG,    /**< Output debug level message */
} dbglevel_e;

/** @} altcomdbgmsg */

/**
 * @defgroup altcombuff Buffer Pool Configuration
 * @{
 */

/**
 * @brief
 * Definition of memory block attribute
 */

typedef struct {
  uint32_t blkSize; /**< Size of a memory block */
  uint16_t blkNum;  /**< Number of memory block with specified size */
} blockset_t;

/**
 * @brief
 * Definition of user defined memory block buffer pool
 */

typedef struct {
  blockset_t *blksetCfg; /**< User defined configuration of block set, See @ref blockset_t */
  uint8_t blksetNum;     /**<  Number of block set */
} blockcfg_t;

/** @} altcombuff */

/**
 * @defgroup altcomhal ALTCOM Hardware Abstraction Layer Configuration
 * @{
 */

/**
 * @brief
 * Definition of HAL type of ALTCOM
 */

typedef enum {
  ALTCOM_HAL_INT_UART, /**< Internal UART */
  ALTCOM_HAL_INT_EMUX, /**< Internal eMUX */
  ALTCOM_HAL_EXT_UART, /**< External UART */
  ALTCOM_HAL_EXT_EMUX  /**< External eMUX */
} haltype_e;

/**
 * @brief
 * Definition of HAL configuration of ALTCOM
 */

typedef struct {
  haltype_e halType; /**< HAL type */
  int virtPortId;    /**< Virtual port ID */
} halcfg_t;

/** @} altcomhal */

/**
 * @defgroup altcominitstruct ALTCOM Initialization Structure
 * @{
 */

/**
 * @brief
 * Definition of initialization structure for ALTCOM
 */

typedef struct {
  dbglevel_e dbgLevel;                /**< Debugging level configuration */
  blockcfg_t blkCfg;                  /**< Buffer pool configuration */
  halcfg_t halCfg;                    /**< HAL configuration */
  int is_postpone_evt_until_appready; /**< Postpone incoming events before application ready */
  int is_cbreg_only_until_appready; /**< Reporting APIs registering callback only before application
                                       ready*/
  altcom_sys_mutex_t altcom_log_lock; /**< User provided shared lock for ALTCOM log system */
} altcom_init_t;

/** @} altcominitstruct */

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * @defgroup altcom_funcs ALTCOM Builder APIs
 * @{
 */

/**
 * @brief altcom_initialize() initialize the ALTCOM library resources.
 *
 * @param [in] initCfg Initialization configuration, see @ref altcom_init_t.
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t altcom_initialize(altcom_init_t *initCfg);

/**
 * Finalize the ALTCOM library
 *
 * altcom_finalize() finalize the ALTCOM library resources.
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t altcom_finalize(void);

/**
 * @brief altcom_app_ready() indicates that the application is ready to work
 * and will start to replay the postponed events.
 *
 */

void altcom_app_ready(void);

/**
 * @brief altcom_get_log_level() obtains the current ALTCOM log message level
 *
 * @return Enumeration of current log message level.
 */

dbglevel_e altcom_get_log_level(void);

/**
 * @brief altcom_set_log_level() sets the target log message level of ALTCOM at runtime.
 *
 * @param [in] level: The target log message level of ALTCOM.
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int altcom_set_log_level(dbglevel_e level);

/** @} altcom_funcs */

#undef EXTERN
#ifdef __cplusplus
}
#endif

/** @} altcom */

#endif /* __MODULES_ALTCOM_INCLUDE_BUILDER_ALTCOM_H */
