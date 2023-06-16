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

#ifndef __MODULES_ALTCOM_INCLUDE_API_HTTP_APICMD_COMMANDS_H
#define __MODULES_ALTCOM_INCLUDE_API_HTTP_APICMD_COMMANDS_H

#define MAX_CHUNK_SIZE 3000
#define MAX_HEADERS_SIZE 512 + 1
#define HTTP_COMMANDS_MAXLEN (MAX_CHUNK_SIZE + MAX_HEADERS_SIZE)

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
begin_packed_struct struct apicmd_httpCommands_s {
  uint8_t profileId;
  uint8_t cmd;
  uint16_t data_len;
  uint32_t pending_data;
  uint16_t headers_len;
  uint8_t strData[HTTP_COMMANDS_MAXLEN];
} end_packed_struct;

begin_packed_struct struct apicmd_httpCommands_res_s { int32_t ret_code; } end_packed_struct;

#endif /* __MODULES_ALTCOM_INCLUDE_API_HTTP_APICMD_COMMANDS_H */
