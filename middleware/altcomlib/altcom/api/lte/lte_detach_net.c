/****************************************************************************
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <errno.h>

#include "lte/lte_api.h"
#include "apiutil.h"
#include "apicmd_dtchnet.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DETACH_NET_DATA_LEN (sizeof(struct apicmd_cmddat_dtchnet_s))

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern detach_net_cb_t g_detach_net_callback;
extern void *g_detach_net_cbpriv;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief lte_detach_network() detach from the LTE network.
 *
 * @param [in] radiomode: The radio mode to stay after detaching, also see @lteradiomode_e
 * @param [in] callback: Callback function to notify that
 * detach completed
 * @param[in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_detach_network(lteradiomode_e radiomode, detach_net_cb_t callback, void *userPriv) {
  int32_t ret;
  FAR struct apicmd_cmddat_dtchnet_s *cmdbuff;

  /* Return error if callback is NULL */

  if (!callback) {
    DBGIF_LOG_ERROR("Input argument is NULL.\n");
    return -EINVAL;
  }

  if (LTE_MODE_MAX <= radiomode) {
    DBGIF_LOG1_ERROR("Invalid radio mode %lu.\n", (uint32_t)radiomode);
    return LTE_RESULT_ERROR;
  }

  /* Check if the library is initialized */

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    ret = -EPERM;
  } else {
    /* Register API callback */

    ALTCOM_REG_CALLBACK(ret, g_detach_net_callback, callback, g_detach_net_cbpriv, userPriv);
    if (0 > ret) {
      DBGIF_LOG_ERROR("Currently API is busy.\n");
    }

    /* Check if callback registering only */

    if (altcom_isCbRegOnly()) {
      return ret;
    }
  }

  /* Accept the API */

  if (0 == ret) {
    /* Allocate API command buffer to send */

    cmdbuff = (FAR struct apicmd_cmddat_dtchnet_s *)apicmdgw_cmd_allocbuff(APICMDID_DETACH_NET,
                                                                           DETACH_NET_DATA_LEN);
    if (!cmdbuff) {
      DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
      ret = -ENOMEM;
    } else {
      /* Send API command to modem */
      cmdbuff->radiomode = (uint8_t)radiomode;
      ret = altcom_send_and_free((uint8_t *)cmdbuff);
    }

    /* If fail, there is no opportunity to execute the callback,
     * so clear it here. */

    if (0 > ret) {
      /* Clear registered callback */

      ALTCOM_CLR_CALLBACK(g_detach_net_callback, g_detach_net_cbpriv);
    } else {
      ret = 0;
    }
  }

  return ret;
}
