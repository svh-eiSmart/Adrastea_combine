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

#define SMS_DELETE_DATA_LEN (sizeof(struct apicmd_cmddat_sms_delete_s))
#define SMS_DELETE_RES_DATA_LEN (sizeof(struct apicmd_cmddat_sms_deleteres_s))

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: altcom_sms_delete
 *
 * Description:
 *   Delete message from reception storage.
 *
 * Input Parameters:
 *   index  Index number of the storage to delete.
 *          When message is deleted by types, set this parameter to 0.
 *   types  Types for which messages to delete.
 *          This parameter is valid only when index is 0.
 *
 * Returned Value:
 *   On success, 0 is returned. On failure,
 *   negative value is returned according to <errno.h>.
 *
 ****************************************************************************/

smsresult_e altcom_sms_delete(uint16_t index, uint8_t types) {
  FAR struct apicmd_cmddat_sms_delete_s *cmdbuff = NULL;
  FAR struct apicmd_cmddat_sms_deleteres_s *resbuff;
  int32_t ret = 0;
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

  if (index) {
    if (ALTCOM_SMS_MSG_LIST_MAX_NUM < index) {
      DBGIF_LOG1_ERROR("Invalid index number[%d].\n", index);
      return -EINVAL;
    }
  } else {
    if (0 == (types & (ALTCOM_SMS_MSG_TYPE_RECV | ALTCOM_SMS_MSG_TYPE_DELIVER_REPORT))) {
      DBGIF_LOG1_ERROR("Invalid messages type[%02X].\n", types);
      return SMS_RESULT_ERROR;
    }
  }

  /* Allocate API command buffer to send. */

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmdbuff, APICMDID_SMS_DELETE,
                                          SMS_DELETE_DATA_LEN, (FAR void **)&resbuff,
                                          SMS_DELETE_RES_DATA_LEN)) {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    return SMS_RESULT_ERROR;
  }

  DBGIF_LOG2_DEBUG("Send index[%d] and types[%d] to modem API.\n", index, types);
  cmdbuff->index = htons(index);
  cmdbuff->types = types;

  /* Send API command to modem. */

  ret = apicmdgw_send((FAR uint8_t *)cmdbuff, (FAR uint8_t *)resbuff, SMS_DELETE_RES_DATA_LEN,
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
    if (APICMD_SMS_RES_OK != ret) {
      DBGIF_LOG1_ERROR("Unexpected API result: %ld\n", ret);
      ret = SMS_RESULT_ERROR;
    }
  }

  altcom_generic_free_cmdandresbuff(cmdbuff, resbuff);

  return (smsresult_e)ret;
}
