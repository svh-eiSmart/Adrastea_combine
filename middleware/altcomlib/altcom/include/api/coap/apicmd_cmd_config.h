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

#ifndef __MODULES_ALTCOM_INCLUDE_API_COAP_APICMD_CONFIG__H
#define __MODULES_ALTCOM_INCLUDE_API_COAP_APICMD_CONFIG__H

#define MAX_CIPHER_LIST 256 + 1
#define MAX_URL_SIZE 256 + 1
#define COAP_CONFIG_REQSTR_MAXLEN (MAX_CIPHER_LIST + MAX_URL_SIZE)

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
begin_packed_struct struct apicmd_coapConfig_s {
  uint8_t profileId;
  /* IP parameters */
  int sessionId;
  int8_t ip_type;

  /* DTLS parameters */
  uint8_t dtls_profile;
  uint8_t dtls_mode;
  uint8_t auth_mode;
  uint8_t session_resumption;
  int8_t CipherListFilteringType;

  /* URI MASK */
  uint8_t uri_mask;

  /* Protocol */
  int protocol;

  /* All string variables */
  uint16_t urlLen;
  uint16_t CipherListLen;
  char strData[COAP_CONFIG_REQSTR_MAXLEN];
} end_packed_struct;

begin_packed_struct struct apicmd_coapConfig_res_s { int32_t ret_code; } end_packed_struct;

#endif /* __MODULES_ALTCOM_INCLUDE_API_COAP_APICMD_CONFIG__H */
