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

#ifndef DRV_SLEEP_H_
#define DRV_SLEEP_H_

#include "DRV_PM.h"

#define DRV_SLEEP_NTFY_SUCCESS (0) /**< Sleep Notification APIs return value - Success. */
#define DRV_SLEEP_NTFY_ERROR (-1)  /**< Sleep Notification APIs return value - Error. */

typedef enum {
  DRV_SLEEP_NTFY_SUSPENDING =
      0,                      /**< 0: DRV_SLEEP_NTFY_SUSPENDING before entering to sleep modes. */
  DRV_SLEEP_NTFY_RESUMING = 1 /**< 1: DRV_SLEEP_NTFY_RESUMING after exiting sleep modes. */
} DRV_SLEEP_NotifyType;

/****************************************************************************
 * Function Prototypes
 ****************************************************************************/

int32_t DRV_SLEEP_Initialize(void);

int32_t DRV_SLEEP_RegNotification(int32_t (*ptr_callback)(DRV_SLEEP_NotifyType, DRV_PM_PwrMode,
                                                          void *),
                                  int32_t *item_idx, void *ptr_context);

int32_t DRV_SLEEP_UnRegNotification(int32_t item_idx);

int32_t DRV_SLEEP_NOTIFY(DRV_SLEEP_NotifyType sleep_state, DRV_PM_PwrMode pwr_mode);

#endif /* DRV_SLEEP_H_ */
