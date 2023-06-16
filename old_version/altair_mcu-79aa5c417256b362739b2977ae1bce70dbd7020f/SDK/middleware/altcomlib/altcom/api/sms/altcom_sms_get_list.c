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

#define SMS_GET_LIST_DATA_LEN (sizeof(struct apicmd_cmddat_sms_getlist_s))
#define SMS_GET_LIST_RES_DATA_LEN (sizeof(struct apicmd_cmddat_sms_getlistres_s))

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: altcom_sms_get_list
 *
 * Description:
 *   Get the information list of the message with the status
 *   specified by the argument.
 *
 * Input Parameters:
 *   types  Type of messages to get.
 *
 * Output Parameters:
 *   list  The pointer of list of messages stored in the reception storage.
 *
 * Returned Value:
 *   On success, 0 is returned. On failure,
 *   negative value is returned according to <errno.h>.
 *
 ****************************************************************************/

smsresult_e altcom_sms_get_list(uint8_t types, struct altcom_sms_msg_list_s *list) {
  FAR struct apicmd_cmddat_sms_getlist_s *cmdbuff = NULL;
  FAR struct apicmd_cmddat_sms_getlistres_s *resbuff = NULL;
  int32_t ret = 0;
  uint16_t reslen = 0;
  uint8_t i;
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

  if (!list) {
    DBGIF_LOG_ERROR("message list pointer is NULL.\n");
    return SMS_RESULT_ERROR;
  }

  if (0 == (types & (ALTCOM_SMS_MSG_TYPE_RECV | ALTCOM_SMS_MSG_TYPE_DELIVER_REPORT))) {
    DBGIF_LOG1_ERROR("Invalid messages type[%d].\n", types);
    return SMS_RESULT_ERROR;
  }

  /* Allocate API command buffer to send. */

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmdbuff, APICMDID_SMS_GET_LIST,
                                          SMS_GET_LIST_DATA_LEN, (FAR void **)&resbuff,
                                          SMS_GET_LIST_RES_DATA_LEN)) {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    return SMS_RESULT_ERROR;
  }

  DBGIF_LOG1_DEBUG("Send message type[%d] to modem API.\n", types);
  cmdbuff->types = types;

  /* Send API command to modem. */

  ret = apicmdgw_send((FAR uint8_t *)cmdbuff, (FAR uint8_t *)resbuff, SMS_GET_LIST_RES_DATA_LEN,
                      &reslen, ALT_OSAL_TIMEO_FEVR);
  if (0 > ret) {
    DBGIF_LOG1_ERROR("Failed to apicmdgw_send [%d].\n", ret);
    ret = SMS_RESULT_ERROR;
  } else if (!reslen) {
    DBGIF_LOG_ERROR("Modem does not support this API.\n");
    ret = SMS_RESULT_ERROR;
  } else {
    ret = ntohl(resbuff->result);
    DBGIF_LOG1_INFO("resbuff.result[%d].\n", ret);
    if (0 <= ret) {
      if (ALTCOM_SMS_MSG_LIST_MAX_NUM < ntohs(resbuff->num)) {
        DBGIF_LOG1_INFO("Invalid infomation number [%d].\n", resbuff->num);
        ret = SMS_RESULT_ERROR;
      } else {
        list->num = ntohs(resbuff->num);
        DBGIF_LOG1_DEBUG("Number of message lists[%d] to modem API.\n", list->num);
        for (i = 0; i < list->num; i++) {
          list->msg[i].type = resbuff->msg[i].type;
          list->msg[i].ref_id = resbuff->msg[i].ref_id;
          list->msg[i].index = ntohs(resbuff->msg[i].index);
          DBGIF_LOG3_DEBUG("Message lists[%d] type[%02X], reference id[%d]", i, list->msg[i].type,
                           list->msg[i].ref_id);
          DBGIF_LOG1_DEBUG(", index[%d] to modem API.\n", list->msg[i].index);
          altcom_sms_set_time(&resbuff->msg[i].sc_time, &list->msg[i].sc_time);
          ret = altcom_sms_set_addr(&resbuff->msg[i].addr, &list->msg[i].addr);
          if (0 > ret) {
            ret = SMS_RESULT_ERROR;
            goto errout;
          }
          ret = SMS_RESULT_OK;
        }
      }
    } else {
      DBGIF_LOG1_ERROR("Unexpected API result: %ld\n", ret);
      ret = SMS_RESULT_ERROR;
    }
  }

errout:
  altcom_generic_free_cmdandresbuff(cmdbuff, resbuff);

  return (smsresult_e)ret;
}
