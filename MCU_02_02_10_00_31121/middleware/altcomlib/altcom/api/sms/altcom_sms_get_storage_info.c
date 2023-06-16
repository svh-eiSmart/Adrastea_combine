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

#define SMS_GET_STGEINFO_RES_DATA_LEN (sizeof(struct apicmd_cmddat_sms_getstgeinfores_s))

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: altcom_sms_get_storage_info
 *
 * Description:
 *   Check reception storage of SMS.
 *
 * Returned Value:
 *   info  The pointer of structure to store the reception storage information.
 *
 * Returned Value:
 *   On success, 0 is returned. On failure,
 *   negative value is returned according to <errno.h>.
 *
 ****************************************************************************/

smsresult_e altcom_sms_get_storage_info(struct altcom_sms_storage_info_s *info) {
  FAR uint8_t *cmdbuff = NULL;
  FAR struct apicmd_cmddat_sms_getstgeinfores_s *resbuff;
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

  /* Argument check. */

  if (!info) {
    DBGIF_LOG_ERROR("storage information pointer is NULL.\n");
    return SMS_RESULT_ERROR;
  }

  /* Allocate API command buffer to send. */

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmdbuff, APICMDID_SMS_GET_STGEINFO, 0,
                                          (FAR void **)&resbuff, SMS_GET_STGEINFO_RES_DATA_LEN)) {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    return SMS_RESULT_ERROR;
  }

  /* Send API command to modem. */

  ret = apicmdgw_send((FAR uint8_t *)cmdbuff, (FAR uint8_t *)resbuff, SMS_GET_STGEINFO_RES_DATA_LEN,
                      &reslen, ALT_OSAL_TIMEO_FEVR);
  if (0 > ret) {
    DBGIF_LOG1_ERROR("Failed to apicmdgw_send[%d].\n", ret);
    ret = SMS_RESULT_ERROR;
  } else if (!reslen) {
    DBGIF_LOG_ERROR("Modem does not support this API.\n");
    ret = SMS_RESULT_ERROR;
  } else {
    ret = ntohl(resbuff->result);
    DBGIF_LOG1_INFO("resbuff.result[%d].\n", ret);
    if (0 <= ret) {
      info->max_record = ntohs(resbuff->info.max_record);
      info->used_record = ntohs(resbuff->info.used_record);
      DBGIF_LOG2_DEBUG("storage information is [%d/%d] to modem API.\n", info->used_record,
                       info->max_record);
      ret = SMS_RESULT_OK;
    } else {
      DBGIF_LOG1_ERROR("Unexpected API result: %ld\n", ret);
      ret = SMS_RESULT_ERROR;
    }
  }

  altcom_generic_free_cmdandresbuff(cmdbuff, resbuff);

  return (smsresult_e)ret;
}
