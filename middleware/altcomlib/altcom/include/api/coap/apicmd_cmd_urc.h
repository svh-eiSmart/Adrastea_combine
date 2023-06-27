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

#ifndef __MODULES_ALTCOM_INCLUDE_API_COAP_APICMD_URC_H
#define __MODULES_ALTCOM_INCLUDE_API_COAP_APICMD_URC_H

#define NULLCHAR 1

/* Commands */
#define COAP_URC_CMD_REQ_DATALEN (sizeof(struct apicmd_coapCmdUrc_s))
#define COAP_URC_CMD_REQSTR_MAXLEN 2048
#define COAP_OPTION_MAX_ID_LEN 5
#define COAP_OPTION_MAX_VAL_LEN 1034 + 1

/*Term */
#define COAP_TOKEN_MAX_SIZE 128 + 1
#define COAP_URC_TERM_REQ_DATALEN (sizeof(struct apicmd_coapTermUrc_s))

/* Reset */
#define COAP_URC_RST_REQ_DATALEN (sizeof(struct apicmd_coapRstUrc_s))

/****************************************************************************
 * Included Files
 ****************************************************************************/

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure describes the data structures of the URC API command */
begin_packed_struct struct apicmd_coapCmdUrc_s {
  uint16_t tokenLen;
  uint16_t dataLen;
  uint8_t profileId;
  uint8_t status;
  uint8_t rspCode;
  uint8_t content;
  uint8_t cmd;
  int blk_m;
  int blk_num;
  int blk_size;
  int optionsArgc;
  char strData[COAP_URC_CMD_REQSTR_MAXLEN];
} end_packed_struct;

begin_packed_struct struct apicmd_coapTermUrc_s {
  uint8_t profileId;
  uint8_t err;
  char Token[COAP_TOKEN_MAX_SIZE];
};

begin_packed_struct struct apicmd_coapRstUrc_s {
  uint8_t profileId;
  char Token[COAP_TOKEN_MAX_SIZE];
};

#endif /* __MODULES_ALTCOM_INCLUDE_API_COAP_APICMD_URC_H */
