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

#ifndef __MODULES_LTE_ALTCOM_INCLUDE_API_SMS_APICMD_SMS_H
#define __MODULES_LTE_ALTCOM_INCLUDE_API_SMS_APICMD_SMS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "apicmd.h"
#include "altcom_sms.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ALTCOM_SMS_ADDR_BUFF_LEN (32)

#define APICMD_SMS_RES_OK (0)
#define APICMD_SMS_RES_ERR (1)

/****************************************************************************
 * Public Types
 ****************************************************************************/

begin_packed_struct struct altcom_apicmd_sms_time_s {
  uint8_t year; /**< Years (0-99) */
  uint8_t mon;  /**< Month (1-12) */
  uint8_t mday; /**< Day of the month (1-31) */
  uint8_t hour; /**< Hours (0-23) */
  uint8_t min;  /**< Minutes (0-59) */
  uint8_t sec;  /**< Seconds (0-59) */
  int8_t tz;    /**< Time zone in hour (-24 - +24) */
} end_packed_struct;

begin_packed_struct struct altcom_apicmd_sms_addr_s {
  /* Type of address is reserved. */

  uint8_t toa;
  uint8_t length;
  uint8_t address[ALTCOM_SMS_ADDR_BUFF_LEN];
} end_packed_struct;

begin_packed_struct struct altcom_apicmd_sms_userdata_s {
  /**
   * Set the character set used for SMS.
   * Definition is refer to @ref altcom_sms_msg_chset_e.
   */

  uint8_t chset;

  /** Buffer length of User-Data field. */

  uint16_t data_len;

  /** User data in utf-8 format. */

  uint8_t *data;
} end_packed_struct;

begin_packed_struct struct altcom_apicmd_sms_concat_hdr_s {
  /**
   * Contain a modulo 256 counter indicating the reference number
   * for a particular concatenated short message.
   */

  uint8_t ref_num;

  /**
   * Contain a value in the range 0 to 255 indicating the total number of
   * short messages within the concatenated short message.
   */

  uint8_t max_num;

  /**
   * Contain a value in the range 0 to 255 indicating the sequence number
   * of a particular short message within the concatenated short message.
   */

  uint8_t seq_num;
} end_packed_struct;

begin_packed_struct struct altcom_apicmd_sms_msg_s {
  uint8_t type;

  union {
    /* SMS-DELIVER */

    struct {
      uint8_t valid_indicator;
      struct altcom_apicmd_sms_addr_s src_addr;
      struct altcom_apicmd_sms_time_s sc_time;
      struct altcom_apicmd_sms_concat_hdr_s concat_hdr;
      struct altcom_apicmd_sms_userdata_s userdata;

      /* Variable length array */

      uint8_t user_data[1];
    } recv;

    /* SMS-STATUS-REPORT */

    struct {
      uint8_t status;
      struct altcom_apicmd_sms_time_s sc_time;
      uint8_t ref_id;
      struct altcom_apicmd_sms_time_s discharge_time;
    } delivery_report;
  } u;
} end_packed_struct;

begin_packed_struct struct altcom_apicmd_sms_msg_info_s {
  uint8_t type;
  uint8_t ref_id;
  uint16_t index;
  struct altcom_apicmd_sms_time_s sc_time;
  struct altcom_apicmd_sms_addr_s addr;
} end_packed_struct;

begin_packed_struct struct altcom_apicmd_sms_storage_info_s {
  /** Maximum number of received messages can be stored in storage. */

  uint16_t max_record;

  /** Number of received messages stored in storage. */

  uint16_t used_record;
} end_packed_struct;

/* This structure discribes the data structure of the API command */

/* APICMDID_SMS_INIT */

begin_packed_struct struct apicmd_cmddat_sms_init_s {
  uint8_t types;
  uint8_t storage_use;
} end_packed_struct;

/* APICMDID_SMS_INITRES */

begin_packed_struct struct apicmd_cmddat_sms_initres_s { int32_t result; } end_packed_struct;

/* APICMDID_SMS_FIN
 * NO DATA
 */

/* APICMDID_SMS_FINRES */

begin_packed_struct struct apicmd_cmddat_sms_finres_s { int32_t result; } end_packed_struct;

/* APICMDID_SMS_SEND */

begin_packed_struct struct apicmd_cmddat_sms_send_s {
  struct altcom_apicmd_sms_addr_s sc_addr;

  /* SMS-SUBMIT */

  uint8_t valid_indicator;
  struct altcom_apicmd_sms_addr_s dest_addr;
  struct altcom_apicmd_sms_userdata_s userdata;

  /* Variable length array */

  uint8_t user_data[1];
} end_packed_struct;

/* APICMDID_SMS_SENDRES */

begin_packed_struct struct apicmd_cmddat_sms_sendres_s {
  int32_t result;
  uint8_t mr_num;
  uint8_t mr_list[ALTCOM_SMS_MSG_REF_ID_MAX_NUM];
} end_packed_struct;

/* APICMDID_SMS_REPORT_RECV */

begin_packed_struct struct apicmd_cmddat_sms_reprecv_s {
  uint16_t index;
  struct altcom_apicmd_sms_addr_s sc_addr;
  struct altcom_apicmd_sms_msg_s msg;

} end_packed_struct;

/* APICMDID_SMS_RECVRES */

begin_packed_struct struct apicmd_cmddat_sms_reprecvres_s { int32_t result; } end_packed_struct;

/* APICMDID_SMS_DELETE */

begin_packed_struct struct apicmd_cmddat_sms_delete_s {
  uint16_t index;
  uint8_t types;
} end_packed_struct;

/* APICMDID_SMS_DELETERES */

begin_packed_struct struct apicmd_cmddat_sms_deleteres_s { int32_t result; } end_packed_struct;

/* APICMDID_SMS_GET_STGEINFO
 * NO DATA
 */

/* APICMDID_SMS_GET_STGEINFORES */

begin_packed_struct struct apicmd_cmddat_sms_getstgeinfores_s {
  int32_t result;
  struct altcom_apicmd_sms_storage_info_s info;
} end_packed_struct;

/* APICMDID_SMS_GET_LIST */

begin_packed_struct struct apicmd_cmddat_sms_getlist_s { uint8_t types; } end_packed_struct;

/* APICMDID_SMS_GET_LISTRES */

begin_packed_struct struct apicmd_cmddat_sms_getlistres_s {
  int32_t result;
  uint16_t num;
  struct altcom_apicmd_sms_msg_info_s msg[ALTCOM_SMS_MSG_LIST_MAX_NUM];
} end_packed_struct;

/* APICMDID_SMS_READ */

begin_packed_struct struct apicmd_cmddat_sms_read_s { uint16_t index; } end_packed_struct;

/* APICMDID_SMS_READRES */

begin_packed_struct struct apicmd_cmddat_sms_readres_s {
  int32_t result;
  struct altcom_apicmd_sms_addr_s sc_addr;
  struct altcom_apicmd_sms_msg_s msg;
} end_packed_struct;

#endif /* __MODULES_LTE_ALTCOM_INCLUDE_API_SMS_APICMD_SMS_H */
