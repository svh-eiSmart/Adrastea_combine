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

#include <string.h>
#include "altcom_smsbs.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SMS_SEND_DATA_LEN(len) (sizeof(struct apicmd_cmddat_sms_send_s) - 1 + len)
#define SMS_SEND_RES_DATA_LEN (sizeof(struct apicmd_cmddat_sms_sendres_s))

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: altcom_sms_send
 *
 * Description:
 *   Send SMS message.
 *
 * Input Parameters:
 *   msg  The pointer of structure for send message.
 *
 * Output Parameters:
 *   mr_list  List of reference ID of sent SMS message.
 *   mr_list_len the resulting mr_list length
 *
 * Returned Value:
 *   On success, effective number of mr_list is returned. On failure,
 *   negative value is returned according to <errno.h>.
 *
 ****************************************************************************/

smsresult_e altcom_sms_send(struct altcom_sms_msg_s *msg,
                            uint8_t mr_list[ALTCOM_SMS_MSG_REF_ID_MAX_NUM],
                            uint32_t *mr_list_len) {
  FAR struct apicmd_cmddat_sms_send_s *cmdbuff = NULL;
  FAR struct apicmd_cmddat_sms_sendres_s *resbuff;
  int ret = 0;
  uint16_t reslen = 0;
  int i = 0;
  uint16_t *ucs2_str = NULL;

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

  if (!msg) {
    DBGIF_LOG_ERROR("Failed message pointer for value.\n");
    return SMS_RESULT_ERROR;
  }
  if (ALTCOM_SMS_ADDR_MAX_LEN <= strlen(msg->sc_addr.address)) {
    DBGIF_LOG1_ERROR("Invalid service center address length[%d] for message.\n",
                     strlen(msg->sc_addr.address));
    return SMS_RESULT_ERROR;
  }
  if (ALTCOM_SMS_MSG_VALID_TOA == (ALTCOM_SMS_MSG_VALID_TOA & msg->u.send.valid_indicator)) {
    if (0x80 == (0x80 & msg->u.send.dest_addr.toa)) {
      DBGIF_LOG1_ERROR("Invalid destination address type of number[%d] for message.\n",
                       msg->u.send.dest_addr.toa);
      return SMS_RESULT_ERROR;
    }
  }
  if (ALTCOM_SMS_ADDR_MAX_LEN <= strlen(msg->u.send.dest_addr.address)) {
    DBGIF_LOG1_ERROR("Invalid destination address length[%d] for message.\n",
                     strlen(msg->u.send.dest_addr.address));
    return SMS_RESULT_ERROR;
  }
  if (ALTCOM_SMS_MSG_VALID_UD == (ALTCOM_SMS_MSG_VALID_UD & msg->u.send.valid_indicator)) {
    if ((ALTCOM_SMS_MSG_CHSET_BINARY == msg->u.send.userdata.chset) ||
        (ALTCOM_SMS_MSG_CHSET_RESERVED <= msg->u.send.userdata.chset)) {
      DBGIF_LOG1_ERROR("Invalid userdata character set[%d] for message.\n",
                       msg->u.send.userdata.chset);
      return SMS_RESULT_ERROR;
    }
  }

  /* Allocate API command buffer to send */
  if (!altcom_generic_alloc_cmdandresbuff(
          (FAR void **)&cmdbuff, APICMDID_SMS_SEND,
          SMS_SEND_DATA_LEN(msg->u.send.userdata.data_len * sizeof(uint16_t)),
          (FAR void **)&resbuff, SMS_SEND_RES_DATA_LEN)) {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    return SMS_RESULT_ERROR;
  }

  if (0 < strlen(msg->sc_addr.address)) {
    ret = altcom_sms_set_apicmd_addr(&msg->sc_addr, &cmdbuff->sc_addr);
    if (0 > ret) {
      ret = SMS_RESULT_ERROR;
      goto errout;
    }
  }

  cmdbuff->valid_indicator = msg->u.send.valid_indicator;

  ret = altcom_sms_set_apicmd_addr(&msg->u.send.dest_addr, &cmdbuff->dest_addr);
  if (0 > ret) {
    ret = SMS_RESULT_ERROR;
    goto errout;
  }

  /* User data reception process. */

  if (ALTCOM_SMS_MSG_VALID_UD == (ALTCOM_SMS_MSG_VALID_UD & msg->u.send.valid_indicator)) {
    cmdbuff->userdata.chset = msg->u.send.userdata.chset;
    ret = uconv_utf8_to_ucs2(msg->u.send.userdata.data_len, msg->u.send.userdata.data,
                             msg->u.send.userdata.data_len * sizeof(uint16_t),
                             (uint16_t *)cmdbuff->user_data);
    if (0 > ret) {
      ret = SMS_RESULT_ERROR;
      goto errout;
    }

    /* Convert byte order for sending in double-byte character set. */

    ucs2_str = (uint16_t *)cmdbuff->user_data;
    for (i = 0; i < ret; i++, ucs2_str++) {
      *ucs2_str = htons(*ucs2_str);
    }
    cmdbuff->userdata.data_len = htons((uint16_t)ret * sizeof(uint16_t));
  }

  /* Send API command to modem. */

  ret = apicmdgw_send((FAR uint8_t *)cmdbuff, (FAR uint8_t *)resbuff, SMS_SEND_RES_DATA_LEN,
                      &reslen, ALT_OSAL_TIMEO_FEVR);
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
      ret = resbuff->mr_num;
      if (ALTCOM_SMS_MSG_REF_ID_MAX_NUM >= ret) {
        for (i = 0; i < ret; i++) {
          mr_list[i] = resbuff->mr_list[i];
          DBGIF_LOG1_DEBUG("[%d] ", mr_list[i]);
        }
        *mr_list_len = ret;
        ret = SMS_RESULT_OK;
      } else {
        DBGIF_LOG1_ERROR("Number of mr_list is buffer over.[%d]\n", ret);
        ret = SMS_RESULT_ERROR;
      }
    } else {
      ret = SMS_RESULT_ERROR;
    }
  }

errout:
  altcom_generic_free_cmdandresbuff(cmdbuff, resbuff);

  return (smsresult_e)ret;
}
