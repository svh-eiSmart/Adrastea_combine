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

#ifndef HIBERNATE_H
#define HIBERNATE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdbool.h>
#include "pwr_mngr.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/**
 * Flag a static or global variable as not to be compressed when hibernating to GPM.
 * This should only be used by variables which are used as part of the hibernation process
 */
#define UNCOMPRESSED __attribute__((section("uncompressed")))

/****************************************************************************
 * Public Types
 ****************************************************************************/
/**
 * Enumeration of block type.
 */
enum block_type {
  BLOCK_HEAP = 0,
  BLOCK_TCB,
};

/**
 * List of free blocks.
 */
struct hibernate_free_block {
  uint32_t type;
  void *start;
  size_t size;
  struct hibernate_free_block *next;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/
void hibernate_dbg_dump(void);

#if (PWR_MNGR_EN_HIBERNATE_SHUTDOWN == 1)
void probe_flash_offset(void);
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * Perform compression from all of memory into GPM prior to hibernation.
 *
 * This function must be followed by hibernate_from_gpm() after resuming from hibernation.
 *
 * @return false if hibernation failed
 */
int32_t hibernate_to_gpm(PWR_MNGR_PwrMode pwr_mode);

/**
 * Restore the contents of memory from GPM
 *
 * This function must always be called after resuming from hibernation started
 * by hibernate_to_gpm().
 *
 * @return non-zero negative if hibernation failed
 */
int32_t hibernate_from_gpm(void);

#if (PWR_MNGR_EN_HIBERNATE_SHUTDOWN == 1)
/**
 * Copy compressed data from GPM to flash prior to hibernation.
 *
 * This function must be followed by hibernate_from_flash() after resuming from hibernation.
 *
 * @return error code
 */
int32_t hibernate_to_flash(void);

/**
 * Restore the contents of memory from flash to GPM.
 *
 * This function must always be called after resuming from hibernation started
 * by hibernate_to_flash().
 *
 * @return error code
 */
int32_t hibernate_from_flash(void);

/**
 * Return the flash offset using for store retention data.
 *
 * @return flash offset. error: 0.
 */
uint32_t hibernate_get_flash_offset(void);
//int32_t hibernate_prepare_flash_space(size_t retained_size);
#endif
/****************************************************************************
 * Debug Functions
 ****************************************************************************/

/**
 * DEBUG API - Get the list of free blocks in the system
 */
void hibernate_get_free_blocks(struct hibernate_free_block **freeBlockList);

/**
 * DEBUG API - Zero all free blocks in the system
 */
void hibernate_zero_free_blocks(struct hibernate_free_block *freeListPtr);

/**
 * DEBUG API - Restore guard blocks on stacks after zeroing
 * unused memory
 */
void hibernate_restore_stacks(void);

#endif /* SYSTEM_HIBERNATE_H */
