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

#include "evthdl_if.h"
#include "apicmdhdlrbs.h"

#include "altcom_smsbs.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

sms_recv_cb_t g_sms_recv_callback = NULL;
void *g_sms_recv_cbpriv = NULL;

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SMSAPIBS_EVTDISP_ID (1)
#define SMSAPIBS_EVTDISPFCTRY_GET_INSTANCE (evtdispfctry_get_instance(SMSAPIBS_EVTDISP_ID))
#define SMS_ADDRESS_DATA_LEN (sizeof(struct altcom_apicmd_sms_addr_s))
#define SMS_MESSAGE_DATA_LEN (sizeof(struct altcom_sms_msg_s))

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

// static enum evthdlrc_e apicmdhdlr_report_recv(FAR uint8_t *evt, uint32_t evlen);
static void report_recv_job(FAR void *arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_altcom_smsbs_status = ALTCOM_SMSBS_STATUS_UNINITIALIZED;
static bool g_altcom_smsbs_storage;

/****************************************************************************
 * Private Function
 ****************************************************************************/

static void report_recv_job(FAR void *arg) {
  FAR struct apicmd_cmddat_sms_reprecv_s *evt = NULL;
  sms_recv_cb_t callback;
  uint16_t index;
  int status = 0;
  bool storage_use = false;
  struct altcom_sms_msg_s *msg = NULL;
  uint8_t *user_data = NULL;
  uint16_t *ucs2_str = NULL;
  int i = 0;
  // struct apicmd_cmddat_sms_reprecvres_s *repres = NULL;
  int ret = 0;
  void *cbpriv;

  /* Argument check. */

  evt = (struct apicmd_cmddat_sms_reprecv_s *)arg;
  DBGIF_ASSERT(evt, "NULL parameter.\n");

  /* Allocate API command response buffer. */

  // repres = (struct apicmd_cmddat_sms_reprecvres_s *)apicmdgw_reply_allocbuff(
  //    (uint8_t *)evt, sizeof(struct apicmd_cmddat_sms_reprecvres_s));
  // DBGIF_ASSERT(repres, "Failed to apicmdgw_reply_allocbuff.\n");

  /* Call the registered callback. */

  ALTCOM_GET_CALLBACK(g_sms_recv_callback, callback, g_sms_recv_cbpriv, cbpriv);
  if (!callback) {
    DBGIF_LOG_ERROR("Unexpected!! callback is NULL.\n");
    goto errout;
  }

  altcom_smsbs_get_status(&status, &storage_use);
  if (false == storage_use) {
    index = 0;
  } else {
    index = ntohs(evt->index);
  }

  /* Allocate the argument of callback. */

  msg = (struct altcom_sms_msg_s *)BUFFPOOL_ALLOC(SMS_MESSAGE_DATA_LEN);
  if (!msg) {
    DBGIF_LOG_ERROR("Failed to BUFFPOOL_ALLOC.\n");
    goto errout;
  }

  ret = altcom_sms_set_addr(&evt->sc_addr, &msg->sc_addr);
  if (0 > ret) {
    DBGIF_LOG1_ERROR("Failed to altcom_sms_set_addr[%d].\n", ret);
    BUFFPOOL_FREE((uint8_t *)msg);
    goto errout;
  }
  msg->type = evt->msg.type;
  if (ALTCOM_SMS_MSG_TYPE_RECV == msg->type) {
    msg->u.recv.valid_indicator = evt->msg.u.recv.valid_indicator;
    ret = altcom_sms_set_addr(&evt->msg.u.recv.src_addr, &msg->u.recv.src_addr);
    if (0 > ret) {
      DBGIF_LOG1_ERROR("Failed to altcom_sms_set_addr[%d].\n", ret);
      BUFFPOOL_FREE((FAR uint8_t *)msg);
      goto errout;
    }
    altcom_sms_set_time(&evt->msg.u.recv.sc_time, &msg->u.recv.sc_time);

    /* Receive processing of concatenated messaged. */

    if (ALTCOM_SMS_MSG_VALID_CONCAT_HDR ==
        (ALTCOM_SMS_MSG_VALID_CONCAT_HDR & msg->u.recv.valid_indicator)) {
      msg->u.recv.concat_hdr.ref_num = evt->msg.u.recv.concat_hdr.ref_num;
      msg->u.recv.concat_hdr.max_num = evt->msg.u.recv.concat_hdr.max_num;
      msg->u.recv.concat_hdr.seq_num = evt->msg.u.recv.concat_hdr.seq_num;
      msg->u.recv.userdata.chset = evt->msg.u.recv.userdata.chset;
    }

    /* User data reception process. */

    if (ALTCOM_SMS_MSG_VALID_UD == (ALTCOM_SMS_MSG_VALID_UD & msg->u.recv.valid_indicator)) {
      user_data =
          (uint8_t *)BUFFPOOL_ALLOC(ntohs(evt->msg.u.recv.userdata.data_len) * sizeof(uint16_t));
      if (!user_data) {
        DBGIF_LOG_ERROR("Failed to BUFFPOOL_ALLOC.\n");
        BUFFPOOL_FREE((FAR uint8_t *)msg);
        goto errout;
      }
      ucs2_str = (uint16_t *)evt->msg.u.recv.user_data;

      /* Convert byte order to convert character code. */

      for (i = 0; i < (int)(ntohs(evt->msg.u.recv.userdata.data_len) / sizeof(uint16_t));
           i++, ucs2_str++) {
        *ucs2_str = ntohs(*ucs2_str);
      }
      ret = uconv_ucs2_to_utf8(ntohs(evt->msg.u.recv.userdata.data_len) / sizeof(uint16_t),
                               (uint16_t *)evt->msg.u.recv.user_data,
                               evt->msg.u.recv.userdata.data_len * sizeof(uint16_t), user_data);
      if (0 > ret) {
        DBGIF_LOG1_ERROR("Failed to uconv_ucs2_to_utf8[%d].\n", ret);
        BUFFPOOL_FREE((FAR uint8_t *)msg);
        BUFFPOOL_FREE((FAR uint8_t *)user_data);
        goto errout;
      }
      msg->u.recv.userdata.data = user_data;
      msg->u.recv.userdata.data_len = ret;
    }
  } else if (ALTCOM_SMS_MSG_TYPE_DELIVER_REPORT == msg->type) {
    msg->u.delivery_report.status = evt->msg.u.delivery_report.status;
    altcom_sms_set_time(&evt->msg.u.delivery_report.sc_time, &msg->u.delivery_report.sc_time);
    msg->u.delivery_report.ref_id = evt->msg.u.delivery_report.ref_id;
    altcom_sms_set_time(&evt->msg.u.delivery_report.discharge_time,
                        &msg->u.delivery_report.discharge_time);
  } else {
    DBGIF_LOG1_ERROR("Invalid message type[%02X].\n", msg->type);
    altcom_free_cmd((FAR uint8_t *)msg);
    goto errout;
  }

  /* Execute a callback to receive the report. */

  ret = callback(msg, index, cbpriv);
  if (0 > ret) {
    DBGIF_LOG_ERROR("Failed to callback.\n");
  }

  if (ALTCOM_SMS_MSG_VALID_UD == (ALTCOM_SMS_MSG_VALID_UD & msg->u.recv.valid_indicator)) {
    BUFFPOOL_FREE((FAR uint8_t *)user_data);
  }

  BUFFPOOL_FREE((FAR uint8_t *)msg);

errout:
  // repres->result = htonl(ret);
  // ret = APICMDGW_REPLY((uint8_t *)repres);
  // if (0 > ret) {
  //  DBGIF_LOG1_ERROR("Failed to apicmdgw_send[%d].\n", ret);
  //}
  // altcom_free_cmd((FAR void *)repres);

  /* In order to reduce the number of copies of the receive buffer,
   * bring a pointer to the receive buffer to the worker thread.
   * Therefore, the receive buffer needs to be released here. */

  altcom_free_cmd((FAR void *)evt);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: apicmdhdlr_sms_report_recv
 *
 * Description:
 *   This function is an API command handler for sms report received .
 *
 * Input Parameters:
 *  evt    Pointer to received event.
 *  evlen  Length of received event.
 *
 * Returned Value:
 *   If the API command ID matches APICMDID_SMS_REPORT_RECV,
 *   EVTHDLRC_STARTHANDLE is returned.
 *   Otherwise it returns EVTHDLRC_UNSUPPORTEDEVENT. If an internal error is
 *   detected, EVTHDLRC_INTERNALERROR is returned.
 *
 ****************************************************************************/

enum evthdlrc_e apicmdhdlr_sms_report_recv(FAR uint8_t *evt, uint32_t evlen) {
  return apicmdhdlrbs_do_runjob(evt, APICMDID_SMS_REPORT_RECV, report_recv_job);
}

/****************************************************************************
 * Name: altcom_smsbs_set_status
 *
 * Description:
 *   Initialize SMS state.
 *
 * Input Parameters:
 *   status  Save SMS Initialize status.
 *   storage_use  Save received SMS to storage.
 *                True is save a SMS to storage,
 *                False is not save a SMS to storage.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

void altcom_smsbs_set_status(int status, bool storage_use) {
  g_altcom_smsbs_status = status;
  g_altcom_smsbs_storage = storage_use;
}

/****************************************************************************
 * Name: altcom_smsbs_get_status
 *
 * Description:
 *   Referense to SMS status.
 *
 * Output Parameters:
 *   status  Save SMS Initialize status.
 *   storage_use  Save received SMS to storage.
 *                True is save a SMS to storage,
 *                False is not save a SMS to storage.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

void altcom_smsbs_get_status(int *status, bool *storage_use) {
  *status = g_altcom_smsbs_status;
  *storage_use = g_altcom_smsbs_storage;
}

/****************************************************************************
 * Name: altcom_smsbs_check_initialize_status
 *
 * Description:
 *   Check if the SMS status is initialized.
 *
 * Returned Value:
 *   On success, 0 is returned. On failure,
 *   negative value is returned according to <errno.h>.
 *
 ****************************************************************************/

int altcom_smsbs_check_initialize_status(void) {
  int ret = 0;
  int stat = 0;
  bool storage_use = false;

  altcom_smsbs_get_status(&stat, &storage_use);
  if (stat != ALTCOM_SMSBS_STATUS_INITIALIZED) {
    DBGIF_LOG1_WARNING("Invalid SMS status[%d]\n", stat);
    ret = -EPERM;
  }

  return ret;
}

/****************************************************************************
 * Name: altcom_sms_set_time
 *
 * Description:
 *   This function is an API callback for report receive.
 *
 * Input Parameters:
 *   Input  Source structure.
 *
 * Output Parameters:
 *   Output  Destination structure.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

void altcom_sms_set_time(struct altcom_apicmd_sms_time_s *input, struct altcom_sms_time_s *output) {
  output->year = input->year;
  output->mon = input->mon;
  output->mday = input->mday;
  output->hour = input->hour;
  output->min = input->min;
  output->sec = input->sec;
  output->tz = input->tz;

  DBGIF_LOG3_DEBUG("Set time[%d/%d/%d", input->year, input->mon, input->mday);
  DBGIF_LOG3_DEBUG(" %d:%d:%d", input->hour, input->min, input->sec);
  DBGIF_LOG1_DEBUG(" +(%d)].\n", input->tz);
}

/****************************************************************************
 * Name: altcom_sms_set_addr
 *
 * Description:
 *   This function is an API callback for report receive.
 *
 * Input Parameters:
 *   Input  Structure whose character code is UCS-2.
 *
 * Output Parameters:
 *   Output  Structure with character code converted to UTF-8.
 *
 * Returned Value:
 *   On success, 0 is returned. On failure,
 *   negative value is returned according to <errno.h>.
 *
 ****************************************************************************/

int altcom_sms_set_addr(struct altcom_apicmd_sms_addr_s *input, struct altcom_sms_addr_s *output) {
  int ucs2_len = input->length / sizeof(uint16_t);
  uint16_t *ucs2_str = NULL;
  int i = 0;
  int ret = 0;

  output->toa = input->toa;

  /* Convert byte order to convert character code. */

  ucs2_str = (uint16_t *)input->address;
  for (i = 0; i < ucs2_len; i++, ucs2_str++) {
    *ucs2_str = ntohs(*ucs2_str);
  }
  ret = uconv_ucs2_to_utf8(ucs2_len, (uint16_t *)input->address, ALTCOM_SMS_ADDR_MAX_LEN,
                           (uint8_t *)output->address);

  DBGIF_LOG1_DEBUG("Set type of address[%02X].\n", output->toa);
  DBGIF_LOG1_DEBUG("Set address[%02X].\n", output->address);

  return ret;
}

/****************************************************************************
 * Name: altcom_sms_set_apicmd_addr
 *
 * Description:
 *   This function is an API callback for report receive.
 *
 * Input Parameters:
 *   Input  Structure whose character code is UTF-8.
 *
 * Output Parameters:
 *   Output  Structure with character code converted to UCS-2.
 *
 * Returned Value:
 *   On success, 0 is returned. On failure,
 *   negative value is returned according to <errno.h>.
 *
 ****************************************************************************/

int altcom_sms_set_apicmd_addr(struct altcom_sms_addr_s *input,
                               struct altcom_apicmd_sms_addr_s *output) {
  uint16_t *ucs2_str = NULL;
  int i = 0;
  int ret = 0;

  output->toa = input->toa;
  ret = uconv_utf8_to_ucs2(strlen(input->address), (uint8_t *)input->address,
                           ALTCOM_SMS_ADDR_BUFF_LEN, (uint16_t *)output->address);
  if (0 > ret) {
    return ret;
  }

  /* Convert byte order for sending in double-byte character set. */

  ucs2_str = (uint16_t *)output->address;
  for (i = 0; i < ret; i++, ucs2_str++) {
    *ucs2_str = htons(*ucs2_str);
  }
  output->length = ret * sizeof(uint16_t);

  DBGIF_LOG1_DEBUG("Set type of address[%02X].\n", output->toa);
  DBGIF_LOG1_DEBUG("Set address length[%02X].\n", output->length);
  DBGIF_LOG1_DEBUG("Set address[%02X].\n", output->address);

  return ret;
}
