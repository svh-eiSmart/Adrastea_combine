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
#include <stdint.h>
#include <stdbool.h>

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

/**
 * @brief
 * Definition of user defined memory management interface structure
 */

typedef struct {
  void *(*create)(blockset_t set[],
                  uint8_t setnum);  /**< Interface method to create buffer pool, return handle on
                                       succeeded or NULL on failure */
  int32_t (*destroy)(void *handle); /**< Interface method to destroy buffer pool, return 0 on
                                       succeeded or -1 on failure */
  void *(*alloc)(void *handle,
                 uint32_t size); /**< Interface method to allocate buffer from pool, return buffer
                                    address on succeeded or NULL on failure */
  int32_t (*free)(void *handle, void *buf); /**< Interface method to free buffer to pool, return 0
                                               on succeeded or -1 on failure */
  void (*show)(void *handle); /**< Interface method to show statistics of buffer pool */
} mem_if_t;

/**
 * @brief
 * Definition of memory block attribute
 */

typedef struct {
  blockcfg_t blkCfg; /**< Block configuration for buffpool */
  mem_if_t *mem_if;  /**< Memory operation interface of buffpool */
} bufmgmtcfg_t;

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
 * @defgroup altcomloglock ALTCOM Logging Lock Interface Structure
 * @{
 */

/**
 * @brief
 * Definition of ALTCOM logging lock interface
 */

typedef struct {
  void (*lock)(void);   /**< The lock method of logging */
  void (*unlock)(void); /**< The unlock method of logging */
} loglock_if_t;

/** @} altcomloglock */

/**
 * @defgroup altcominitstruct ALTCOM Initialization Structure
 * @{
 */

/**
 * @brief
 * Definition of initialization structure for ALTCOM
 */

typedef struct {
  dbglevel_e dbgLevel;      /**< Debugging level configuration */
  loglock_if_t *loglock_if; /**< User provided lock interface for ALTCOM log synchronization */
  bufmgmtcfg_t bufMgmtCfg;  /**< Buffer management configuration */
  halcfg_t halCfg;          /**< HAL configuration */
  bool postpone_evt;        /**< Postpone incoming events until application ready */
  bool cbreg_only;          /**< Reporting APIs registering callback only until application ready*/
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
