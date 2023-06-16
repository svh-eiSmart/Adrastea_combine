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
 * @file mem_retain.h
 */

#ifndef MEM_RETAIN_H_
#define MEM_RETAIN_H_

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>

/**
 * @defgroup powermanagement PM (Power Management) Driver
 * @{
 */

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/**
 * @defgroup pm_constant PM Constants
 * @{
 */

/** @} pm_constant */

/****************************************************************************
 * Public Types
 ****************************************************************************/
/**
 * @defgroup pm_types PM Types
 * @{
 */

/**
 * @brief Definition the return status memory retention APIs.
 */

typedef enum {
  MEM_STATUS_OK = 0,        /**< Status OK */
  MEM_STATUS_INVALID_ADDR,  /**< Invalid addrees */
  MEM_STATUS_INVALID_RANGE, /**<  Invalid range of memory block */
  MEM_STATUS_IN_IRQ,        /**< Caller in IRQ */
  MEM_STATUS_NOT_AVAIL,     /**< No available entry to record */
  MEM_STATUS_UNSUPPORTED,   /**< Not yet supported */
  MEM_STATUS_MAX,           /**< Enumeration maximum value */
} mem_retain_status_t;

/** @} pm_types */

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
/*! @cond Doxygen_Suppress */
#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif
/*! @endcond */

/**
 * @defgroup mem_retain_funcs Memory Retention APIs
 * @{
 */

/**
 * @brief Marking the specified region as a "not retained" memory block while sleep.
 * Any data in this region will become unreliable after the sleep cycle,
 * it may need further initialize/cleanup process before using it.
 * Marking the memory block as "not retained" can improve the memory
 * utilization and capacity of the system.
 *
 * @param [in] start_addr: Start address of a memory region to be marked as not retained block.
 * @param [in] length: Length of the not retained memory block.
 * @param [in] description: A short description of the purpose to this memory region.
 * @return Refer to @ref mem_retain_status_t
 */

mem_retain_status_t mark_as_not_retained_mem(void *start_addr, size_t length, char *description);

/**
 * @brief Unmark the memory region which is "not retained" while sleep
 *
 * @param [in] start_addr: Start address of the memory region to be unmarked.
 * @return Refer to @ref mem_retain_status_t
 */

mem_retain_status_t unmark_not_retained_mem(void *start_addr);

/** @} mem_retain_funcs */
/*! @cond Doxygen_Suppress */
#undef EXTERN
#ifdef __cplusplus
}
#endif
/*! @endcond */
/** @} powermanagement */
#endif /* MEM_RETAIN_H_ */
