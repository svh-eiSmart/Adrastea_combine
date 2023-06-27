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
#include "apicmd_dataoff.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DATAOFF_DATA_LEN (sizeof(struct apicmd_cmddat_dataoff_s))

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern data_off_cb_t g_dataoff_callback;
extern void *g_dataoff_cbpriv;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief lte_data_off() disable the data communication feature.
 *
 * @param [in] session_id: The numeric value of the session ID defined
 * in the apn setting. See @ref ltesessionid for valid range
 * @param [in] callback: Callback function to notify that
 * disabling the data communication completed
 * @param[in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_data_off(uint8_t session_id, data_off_cb_t callback, void *userPriv) {
  int32_t ret;
  FAR struct apicmd_cmddat_dataoff_s *cmdbuff;

  /* Return error if callback is NULL */

  if (!callback) {
    DBGIF_LOG_ERROR("Input argument is NULL.\n");
    return -EINVAL;
  }

  /* Check if the library is initialized */

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    ret = -EPERM;
  } else if (LTE_SESSION_ID_MIN > session_id || LTE_SESSION_ID_MAX < session_id) {
    DBGIF_LOG_ERROR("Invalid parameter.\n");
    ret = -EINVAL;
  } else {
    /* Register API callback */

    ALTCOM_REG_CALLBACK(ret, g_dataoff_callback, callback, g_dataoff_cbpriv, userPriv);
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

    cmdbuff = (FAR struct apicmd_cmddat_dataoff_s *)apicmdgw_cmd_allocbuff(APICMDID_DATAOFF,
                                                                           DATAOFF_DATA_LEN);
    if (!cmdbuff) {
      DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
      ret = -ENOMEM;
    } else {
      /* Send API command to modem */

      cmdbuff->sessionid = session_id;
      ret = altcom_send_and_free((FAR uint8_t *)cmdbuff);
    }

    /* If fail, there is no opportunity to execute the callback,
     * so clear it here. */

    if (0 > ret) {
      /* Clear registered callback */

      ALTCOM_CLR_CALLBACK(g_dataoff_callback, g_dataoff_cbpriv);
    } else {
      ret = 0;
    }
  }

  return ret;
}
