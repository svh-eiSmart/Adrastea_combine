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

#define SMS_READ_DATA_LEN (sizeof(struct apicmd_cmddat_sms_read_s))

#define SMS_READ_RES_DATA_LEN \
  (sizeof(struct apicmd_cmddat_sms_readres_s) - 1 + ALTCOM_SMS_READ_USERDATA_MAXLEN)

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

smsresult_e altcom_sms_read(uint16_t index, struct altcom_sms_msg_s *msg) {
  FAR struct apicmd_cmddat_sms_read_s *cmdbuff = NULL;
  FAR struct apicmd_cmddat_sms_readres_s *resbuff = NULL;
  int32_t ret = 0;
  uint16_t reslen = 0;
  uint8_t *user_data = NULL;
  uint16_t *ucs2_str = NULL;
  int i = 0;

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

  if (!msg->u.recv.userdata.data) {
    DBGIF_LOG_ERROR("Failed message user data pointer for value.\n");
    return SMS_RESULT_ERROR;
  }
  /* Allocate API command buffer to send */

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmdbuff, APICMDID_SMS_READ,
                                          SMS_READ_DATA_LEN, (FAR void **)&resbuff,
                                          SMS_READ_RES_DATA_LEN)) {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    return SMS_RESULT_ERROR;
  }

  cmdbuff->index = htons(index);
  /* Send API command to modem */

  ret = apicmdgw_send((FAR uint8_t *)cmdbuff, (FAR uint8_t *)resbuff, SMS_READ_RES_DATA_LEN,
                      &reslen, ALT_OSAL_TIMEO_FEVR);

  if (0 > ret) {
    DBGIF_LOG1_ERROR("Failed to apicmdgw_send[%d].\n", ret);
    ret = SMS_RESULT_ERROR;
  } else if (0 == reslen) {
    DBGIF_LOG_ERROR("Modem does not support this API.\n");
    ret = SMS_RESULT_ERROR;
  } else {
    ret = ntohl(resbuff->result);
    DBGIF_LOG1_INFO("resbuff->result[%d].\n", ret);
    if (0 <= ret) {
      msg->type = resbuff->msg.type;
      if (ALTCOM_SMS_MSG_TYPE_RECV == msg->type) {
        msg->u.recv.valid_indicator = resbuff->msg.u.recv.valid_indicator;
        ret = altcom_sms_set_addr(&resbuff->msg.u.recv.src_addr, &msg->u.recv.src_addr);
        if (0 > ret) {
          DBGIF_LOG1_ERROR("Failed to altcom_sms_set_addr[%d].\n", ret);
          ret = SMS_RESULT_ERROR;
          goto errout;
        }
        altcom_sms_set_time(&resbuff->msg.u.recv.sc_time, &msg->u.recv.sc_time);

        /* Receive processing of concatenated messaged. */

        if (ALTCOM_SMS_MSG_VALID_CONCAT_HDR ==
            (ALTCOM_SMS_MSG_VALID_CONCAT_HDR & msg->u.recv.valid_indicator)) {
          msg->u.recv.concat_hdr.ref_num = resbuff->msg.u.recv.concat_hdr.ref_num;
          msg->u.recv.concat_hdr.max_num = resbuff->msg.u.recv.concat_hdr.max_num;
          msg->u.recv.concat_hdr.seq_num = resbuff->msg.u.recv.concat_hdr.seq_num;
          msg->u.recv.userdata.chset = resbuff->msg.u.recv.userdata.chset;
        }

        /* User data reception process. */

        if (ALTCOM_SMS_MSG_VALID_UD == (ALTCOM_SMS_MSG_VALID_UD & msg->u.recv.valid_indicator)) {
          user_data = msg->u.recv.userdata.data;

          ucs2_str = (uint16_t *)resbuff->msg.u.recv.user_data;

          /* Convert byte order to convert character code. */

          for (i = 0; i < (int)(ntohs(resbuff->msg.u.recv.userdata.data_len) / sizeof(uint16_t));
               i++, ucs2_str++) {
            *ucs2_str = ntohs(*ucs2_str);
          }
          ret = uconv_ucs2_to_utf8(ntohs(resbuff->msg.u.recv.userdata.data_len) / sizeof(uint16_t),
                                   (uint16_t *)resbuff->msg.u.recv.user_data,
                                   resbuff->msg.u.recv.userdata.data_len * sizeof(uint16_t),
                                   user_data);
          if (0 > ret) {
            DBGIF_LOG1_ERROR("Failed to uconv_ucs2_to_utf8[%d].\n", ret);
            ret = SMS_RESULT_ERROR;
            goto errout;
          }
          msg->u.recv.userdata.data = user_data;
          msg->u.recv.userdata.data_len = ret;

          ret = SMS_RESULT_OK;
        }
      } else if (ALTCOM_SMS_MSG_TYPE_DELIVER_REPORT == msg->type) {
        msg->u.delivery_report.status = resbuff->msg.u.delivery_report.status;
        altcom_sms_set_time(&resbuff->msg.u.delivery_report.sc_time,
                            &msg->u.delivery_report.sc_time);
        msg->u.delivery_report.ref_id = resbuff->msg.u.delivery_report.ref_id;
        altcom_sms_set_time(&resbuff->msg.u.delivery_report.discharge_time,
                            &msg->u.delivery_report.discharge_time);
      } else {
        DBGIF_LOG1_ERROR("Invalid message type[%02X].\n", msg->type);
        ret = SMS_RESULT_ERROR;
        goto errout;
      }
    } else {
      ret = SMS_RESULT_ERROR;
    }
  }

errout:
  altcom_generic_free_cmdandresbuff(cmdbuff, resbuff);

  return (smsresult_e)ret;
}
