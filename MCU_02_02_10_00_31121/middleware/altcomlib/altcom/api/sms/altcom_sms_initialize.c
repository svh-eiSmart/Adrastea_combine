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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "altcom_smsbs.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern sms_recv_cb_t g_sms_recv_callback;
extern void *g_sms_recv_cbpriv;

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SMS_INIT_DATA_LEN (sizeof(struct apicmd_cmddat_sms_init_s))
#define SMS_INIT_RES_DATA_LEN (sizeof(struct apicmd_cmddat_sms_initres_s))

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * Initialize resources used in SMS API.
 *
 * @param [in] callback: Callback function to receiving SMS.
 *
 * @param [in] userPriv: User's private data
 *
 * @param [in] storage_use: Save received SMS to storage.
 *                          True is save a SMS to storage,
 *                          False is not save a SMS to storage.
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

smsresult_e altcom_sms_initialize(sms_recv_cb_t callback, void *userPriv, bool storage_use) {
  FAR struct apicmd_cmddat_sms_init_s *cmdbuff = NULL;
  FAR struct apicmd_cmddat_sms_initres_s resbuff;
  int ret = 0;
  uint16_t reslen = 0;
  int status = 0;
  bool temp_storage_use = 0;

  /* Check the SMS status. */

  altcom_smsbs_get_status(&status, &temp_storage_use);
  if (ALTCOM_SMSBS_STATUS_INITIALIZED == status) {
    DBGIF_LOG_ERROR("SMS has already been initialized.\n");
    return -EALREADY;
  }

  /* Check if the library is initialized */

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    ret = -EPERM;
  } else {
    /* Register API callback */

    ALTCOM_REG_CALLBACK(ret, g_sms_recv_callback, callback, g_sms_recv_cbpriv, userPriv);
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

    cmdbuff = (FAR struct apicmd_cmddat_sms_init_s *)apicmdgw_cmd_allocbuff(APICMDID_SMS_INIT,
                                                                            SMS_INIT_DATA_LEN);
    if (!cmdbuff) {
      DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
      ret = -ENOMEM;
    } else {
      /* Send API command to modem */

      if (callback) {
        cmdbuff->types = ALTCOM_SMS_MSG_TYPE_RECV | ALTCOM_SMS_MSG_TYPE_DELIVER_REPORT;
      } else {
        cmdbuff->types = 0x00;
      }

      cmdbuff->storage_use = (uint8_t)storage_use;

      /* Send API command to modem. */

      ret = apicmdgw_send((FAR uint8_t *)cmdbuff, (FAR uint8_t *)&resbuff, SMS_INIT_RES_DATA_LEN,
                          &reslen, ALT_OSAL_TIMEO_FEVR);
      if (0 > ret) {
        DBGIF_LOG1_ERROR("Failed to apicmdgw_send[%d].\n", ret);
      } else if (0 == reslen) {
        DBGIF_LOG_ERROR("Modem does not support this API.\n");
        ret = -ENOTSUP;
      } else {
        ret = ntohl(resbuff.result);
        DBGIF_LOG1_INFO("resbuff.result[%d].\n", ret);
        if (0 <= ret) {
          altcom_smsbs_set_status(ALTCOM_SMSBS_STATUS_INITIALIZED, storage_use);
        }
      }

      altcom_free_cmd((FAR uint8_t *)cmdbuff);
    }

    /* If fail, there is no opportunity to execute the callback,
     * so clear it here. */

    if (0 > ret) {
      /* Clear registered callback */

      ALTCOM_CLR_CALLBACK(g_sms_recv_callback, g_sms_recv_cbpriv);
    } else {
      ret = 0;
    }
  }

  return ret;
}
