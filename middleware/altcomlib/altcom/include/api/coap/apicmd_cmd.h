/****************************************************************************
 *
 *  (c) copyright 2020 Altair Semiconductor, Ltd. All rights reserved.
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

#ifndef __MODULES_ALTCOM_INCLUDE_API_COAP_APICMD_CMDPUT__H
#define __MODULES_ALTCOM_INCLUDE_API_COAP_APICMD_CMDPUT__H

#define NULLCHAR 1
#define MAX_TOKEN_SIZE 8
#define MAX_PAYLOAD_LEN 1024
#define MAX_URI_SIZE 256
#define MAX_OPTION_LEN 50
#define OPTION_TYPE_LEN 1
#define MAX_OPTIONS_NO 8
#define COAP_CMD_REQSTR_MAXLEN                                                                  \
  (((MAX_OPTION_LEN + NULLCHAR + OPTION_TYPE_LEN + NULLCHAR) * MAX_OPTIONS_NO) + MAX_URI_SIZE + \
   NULLCHAR + MAX_PAYLOAD_LEN + MAX_TOKEN_SIZE + NULLCHAR)

/****************************************************************************
 * Included Files
 ****************************************************************************/

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure describes the data structure of the API command */

begin_packed_struct struct apicmd_coapCmdput_res_s { int32_t ret_code; } end_packed_struct;

begin_packed_struct struct apicmd_coapCmdput_s {
  uint8_t profileId;
  uint8_t CoapCmd;
  uint16_t uriLen;
  uint8_t confirm;
  uint8_t token_len;
  int8_t contents;
  uint16_t DataLen;
  int16_t block_size;
  int32_t block_no;
  int8_t block_m;
  uint8_t optionsArgc;
  uint8_t strData[COAP_CMD_REQSTR_MAXLEN];
} end_packed_struct;

#endif /* __MODULES_ALTCOM_INCLUDE_API_COAP_APICMD_CMDPUT__H */
