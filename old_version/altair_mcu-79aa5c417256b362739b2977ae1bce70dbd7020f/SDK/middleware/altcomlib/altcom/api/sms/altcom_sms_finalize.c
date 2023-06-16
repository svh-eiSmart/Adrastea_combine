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
 * Pre-processor Definitions
 ****************************************************************************/

#define SMS_FIN_RES_DATA_LEN (sizeof(struct apicmd_cmddat_sms_finres_s))

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern sms_recv_cb_t g_sms_recv_callback;
extern void *g_sms_recv_cbpriv;

/****************************************************************************
 * Name: altcom_sms_finalize
 *
 * Description:
 *   Release resources used in SMS API.
 *
 * Returned Value:
 *   On success, 0 is returned. On failure,
 *   negative value is returned according to <errno.h>.
 *
 ****************************************************************************/

smsresult_e altcom_sms_finalize(void) {
  FAR uint8_t *cmdbuff = NULL;
  FAR struct apicmd_cmddat_sms_finres_s *resbuff;
  int ret = 0;
  uint16_t reslen = 0;

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return SMS_RESULT_ERROR;
  }

  /* Check the API status. */

  ret = altcom_smsbs_check_initialize_status();
  if (0 > ret) {
    DBGIF_LOG_ERROR("LTE modem is not SMS initialized.\n");
    return SMS_RESULT_ERROR;
  }

  /* Allocate API command buffer to send. */

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmdbuff, APICMDID_SMS_FIN, 0,
                                          (FAR void **)&resbuff, SMS_FIN_RES_DATA_LEN)) {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    return SMS_RESULT_ERROR;
  }

  /* Send API command to modem. */

  ret = apicmdgw_send((FAR uint8_t *)cmdbuff, (FAR uint8_t *)resbuff, SMS_FIN_RES_DATA_LEN, &reslen,
                      ALT_OSAL_TIMEO_FEVR);
  if (0 > ret) {
    DBGIF_LOG1_ERROR("Failed to apicmdgw_send[%d].\n", ret);
    ret = SMS_RESULT_ERROR;
  } else if (0 == reslen) {
    DBGIF_LOG_ERROR("Modem does not support this API.\n");
    ret = SMS_RESULT_ERROR;
  } else {
    ret = ntohl(resbuff->result);
    DBGIF_LOG1_INFO("resbuff.result[%d].\n", ret);
    if (0 <= ret) {
      altcom_smsbs_set_status(ALTCOM_SMSBS_STATUS_UNINITIALIZED, false);

      ALTCOM_CLR_CALLBACK(g_sms_recv_callback, g_sms_recv_cbpriv);

    } else {
      ret = SMS_RESULT_ERROR;
    }
  }

  altcom_generic_free_cmdandresbuff(cmdbuff, resbuff);

  return (smsresult_e)ret;
}
