/****************************************************************************
 *
 *  (c) copyright 2021 Altair Semiconductor, Ltd. All rights reserved.
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

#ifndef __MODULES_ALTCOM_INCLUDE_API_HTTP_APICMD_CONFIGNODE_H
#define __MODULES_ALTCOM_INCLUDE_API_HTTP_APICMD_CONFIGNODE_H

#define MAX_URL_SIZE 255 + 1
#define MAX_USER_SIZE 32 + 1
#define MAX_PASSWD_SIZE 32 + 1
#define HTTP_CONFIG_NODE_MAXLEN (MAX_USER_SIZE + MAX_URL_SIZE + MAX_PASSWD_SIZE)

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
begin_packed_struct struct apicmd_httpConfigNode_s {
  uint8_t profileId;
  uint8_t encode_format;
  uint8_t url_len;
  uint8_t user_len;
  uint8_t passwd_len;
  char strData[HTTP_CONFIG_NODE_MAXLEN];
} end_packed_struct;

begin_packed_struct struct apicmd_httpConfigNode_res_s { int32_t ret_code; } end_packed_struct;

#endif /* __MODULES_ALTCOM_INCLUDE_API_HTTP_APICMD_CONFIGNODE_H */
