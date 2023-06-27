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

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <string.h>
#include <stdbool.h>
#include "dbg_if.h"
#include "apicmd.h"
#include "altcom_coap.h"
#include "apicmd_cmd.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "altcom_cc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define COAPEV_RET_SUCCESS 0
#define COAP_CMD_RES_DATALEN (sizeof(struct apicmd_coapCmdput_res_s))
#define COAP_CMD_REQ_DATALEN (sizeof(struct apicmd_coapCmdput_s))
#define CMD_TIMEOUT 10000 /* 10 secs */
#define NULLCHAR 1
#define ISPROFILE_VALID(p) (p > 0 && p <= 5)
#define MAX_BLK_SIZE 1024
#define MIN_BLK_SIZE 32
#define MAX_BLOCK_NO 1048575
#define MIN_BLOCK_NO 0

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
  @brief altcom_coap_cmd():          Commands to communicate with COAP .

 @param [in]  profileId:            Assigned profile between 1 and 5
 @param [in]  CmdParams:            Communication parameters. See @ref CoapCmdData_t.

 @return:                           COAP_SUCCESS or COAP_FAILURE.
 */

Coap_err_code altcom_coap_cmd(unsigned char profileId, CoapCmdData_t *CmdParams) {
  int32_t ret = 0;
  uint16_t resLen = 0;
  FAR struct apicmd_coapCmdput_s *cmd = NULL;
  FAR struct apicmd_coapCmdput_res_s *res = NULL;
  int32_t sendLen = 0;
  uint16_t strLen;
  uint8_t *str, optionsArgc;
  int ptr;
  Coap_err_code err = COAP_FAILURE;

  DBGIF_LOG_DEBUG("altcom_coap_cmd()");

  if (CmdParams == NULL) {
    DBGIF_LOG_ERROR("No CmdParams structures\n");
    return err;
  }

  if (CmdParams->uri) sendLen += strlen(CmdParams->uri) + 1;
  if (CmdParams->token != NULL) sendLen += CmdParams->token->token_length + 1;

  /* Optional parameter set to 0 if not entered */
  optionsArgc = CmdParams->optionsArgc;
  ptr = 0;
  if (optionsArgc > 0) {
    while (ptr < optionsArgc) {
      /* Option value */
      if (CmdParams->optionsArgV[ptr] == NULL ||
          CmdParams->optionsArgV[ptr]->option_value == NULL) {
        DBGIF_LOG_ERROR("Invalid Option value\n");
        return err;
      }
      if (CmdParams->optionsArgV[ptr]->option_value)
        sendLen += strlen(CmdParams->optionsArgV[ptr]->option_value) + 1;
      /* Option type */
      sendLen += 1;
      ptr++;
    }
  }

  /* Add payload */
  sendLen += CmdParams->data_len + 1;
  sendLen += COAP_CMD_REQ_DATALEN - COAP_CMD_REQSTR_MAXLEN;

  if (sendLen > COAP_CMD_REQSTR_MAXLEN) {
    DBGIF_LOG1_ERROR("[coap cmd request] strData length too long: %ld\n", sendLen);
    return err;
  }

  /* Allocate send and response command buffer */
  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_COAP_CMD, sendLen,
                                          (FAR void **)&res, COAP_CMD_RES_DATALEN)) {
    return err;
  }
  /* Fill command data */

  /* Global data strings */
  str = cmd->strData;

  /* Profile id  */
  cmd->profileId = profileId;
  /* Command */
  cmd->CoapCmd = CmdParams->cmd;
  /* Confirm */
  cmd->confirm = CmdParams->confirm;
  /* Contents */
  cmd->contents = CmdParams->content ? *(CmdParams->content) : -1;
  /* Data length */
  cmd->DataLen = htons(CmdParams->data_len);
  /* Block size */
  cmd->block_size = CmdParams->blk_size ? htons(*(CmdParams->blk_size)) : htons(-1);
  /* Block Num */
  cmd->block_no = CmdParams->block_num ? htonl(*(CmdParams->block_num)) : htons(-1);
  /* Block M */
  cmd->block_m = CmdParams->block_m ? *(CmdParams->block_m) : -1;
  /* optionsArgc */
  cmd->optionsArgc = optionsArgc;

  /* Blockwise are entered together otherwise there are all together default value */
  if (CmdParams->blk_size && CmdParams->block_num && CmdParams->block_m) {
    /* All values are entered */
    int16_t block_size = (int16_t)(*(CmdParams->blk_size));
    int32_t block_no = (int32_t)(*(CmdParams->block_num));
    int8_t block_m = (int8_t)(*(CmdParams->block_m));

    int blk_check = block_size & (block_size - 1);
    if (blk_check != 0 || block_size < MIN_BLK_SIZE || block_size > MAX_BLK_SIZE) {
      DBGIF_LOG1_ERROR("Incorrect Block size %d\n", block_size);
      goto sign_out;
    }
    if (block_no > MAX_BLOCK_NO) {
      DBGIF_LOG1_ERROR("Incorrect Block nb %d\n", block_no);
      goto sign_out;
    }
    if (block_m > 1) {
      DBGIF_LOG1_ERROR("Incorrect Block M %d\n", block_m);
      goto sign_out;
    }
  }

  /* String parameters format
   *
   * URI + NULL
   * Token + NULL
   *  Option value + NULL
   *  Option type (1 byte)
   *  Payload
   */

  /* URI length */
  strLen = 0;
  cmd->uriLen = 0;
  if (CmdParams->uri) {
    strLen = strlen(CmdParams->uri);
    cmd->uriLen = htons(strLen);
    if (MAX_URI_SIZE - 1 > strLen) {
      strLen = snprintf((char *)str, MAX_URI_SIZE, "%s", CmdParams->uri);
      DBGIF_LOG2_DEBUG("uri: %s, len: %d\n", str, strLen);
      str += strLen + NULLCHAR;
    }
  }

  /* Token */
  strLen = 0;
  cmd->token_len = 0;
  if (CmdParams->token) {
    cmd->token_len = CmdParams->token->token_length;
    strLen =
        snprintf((char *)str, CmdParams->token->token_length + 1, (char *)CmdParams->token->token);
    str += strLen + NULLCHAR;
  }

  /* Format :
   *  Option value + NULL
   *  Option type (1 byte)
   */
  if (optionsArgc > 0) {
    /* Options type/value are inserted one after the other */
    ptr = 0;
    while (ptr < optionsArgc) {
      strLen = snprintf((char *)str, MAX_OPTION_LEN, CmdParams->optionsArgV[ptr]->option_value);
      str += strLen + NULLCHAR;
      *str = (char)CmdParams->optionsArgV[ptr]->option_type;
      str++;
      ptr++;
    }
  }

  if (CmdParams->data_len) {
    memcpy(str, CmdParams->data, CmdParams->data_len);
  }

  /* Sanity check */

  /* Check profile id fits between limits */
  if (!ISPROFILE_VALID(cmd->profileId)) {
    DBGIF_LOG1_ERROR("Incorrect profile#: %d\n", profileId);
    goto sign_out;
  }
  /* Coap command */
  if (CmdParams->cmd > COAP_CMD_DELETE) {
    DBGIF_LOG1_ERROR("Invalid Coap command: %d\n", CmdParams->cmd);
    goto sign_out;
  }
  /* Confirm */
  if (cmd->confirm > COAP_CMD_CONFIRMED) {
    DBGIF_LOG1_ERROR("Incorrect confirm param: %d\n", cmd->confirm);
    goto sign_out;
  }
  /* Option nb */
  if (optionsArgc > MAX_OPTIONS_NO) {
    DBGIF_LOG_ERROR("Options # is limited to  MAX_OPTIONS_NO \n");
    goto sign_out;
  }

  /* Send command and block until receive a response */
  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, COAP_CMD_RES_DATALEN, &resLen,
                      CMD_TIMEOUT);

  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    goto sign_out;
  }

  if (resLen != COAP_CMD_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected data length response: %hu\n", resLen);
    goto sign_out;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);
  DBGIF_LOG1_DEBUG("[altcom_coap_cmd-res]ret: %d\n", ret);

  if (ret == (int32_t)COAPEV_RET_SUCCESS) err = COAP_SUCCESS;

sign_out:
  altcom_generic_free_cmdandresbuff(cmd, res);

  return err;
}
