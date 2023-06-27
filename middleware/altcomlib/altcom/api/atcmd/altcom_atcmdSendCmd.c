/****************************************************************************
 *
 *  (c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.
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
#include "altcom_atcmd.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "altcom_cc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define ATCMD_PREFIX "AT"
#define ATCMD_PREFIX_LEN (2)
#define ATCMD_FOOTER '\r'
#define ATCMD_CHG_FOOTER(cmd, len) \
  do {                             \
    cmd[len - 1] = '\0';           \
  } while (0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: atcmdSendCmd_request
 *
 * Description:
 *   Send APICMDID_ATCMD_SEND.
 *
 ****************************************************************************/

static int atcmdSendCmd_request(uint16_t cmdLen, const char *cmdStr, uint16_t resBufSize,
                                char *resBuf) {
  int32_t ret;
  uint8_t *cmdbuff;
  uint16_t respLen;

  /* Allocate API command buffer to send */
  cmdbuff = (uint8_t *)apicmdgw_cmd_allocbuff(APICMDID_ATCMDCONN_SEND, cmdLen);
  if (!cmdbuff) {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    return -1;
  } else {
    memcpy(cmdbuff, cmdStr, cmdLen - 1);
    ATCMD_CHG_FOOTER(cmdbuff, cmdLen);

    /* Send API command */
    ret = apicmdgw_send(cmdbuff, (uint8_t *)resBuf, resBufSize, &respLen, ALTCOM_SYS_TIMEO_FEVR);
    if (ret < 0) {
      DBGIF_LOG1_ERROR("Failed to send API command, error: %ld.\n", ret);
    } else {
      ret = respLen;
    }

    /* Free API command buffer */
    apicmdgw_freebuff((void *)cmdbuff);
  }

  return 0 > ret ? -1 : ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief altcom_atcmdSendCmd() Send AT command.
 *
 * @param [in] cmdStr: A null-terminated string contains AT command to send; Note that there is no
 * need to append "\r" or "\r\n" at the end of string.
 * @param [in] resBufSize: The size of user provided response buffer; If resBufSize less then the
 * response, -1 returned.
 * @param [out] resBuf: An user provided buffer to fill the response of AT command.
 * @return On success, the length of response includes null-terminated returned; -1 on failure.
 */

int altcom_atcmdSendCmd(const char *cmdStr, uint16_t resBufSize, char *resBuf) {
  uint8_t atOffset = 0;
  uint16_t cmdLen;

  /* Check parameters */
  if (NULL == cmdStr) {
    DBGIF_LOG_ERROR("Invalid cmdStr\n");
    return -1;
  }

  cmdLen = strlen(cmdStr);
  if (0 == cmdLen || ATCMD_MAX_CMD_LEN - 1 < cmdLen) {
    DBGIF_LOG1_ERROR("Invalid cmdStr length %hu\n", cmdLen);
    return -1;
  }

  if (0 == resBufSize || NULL == resBuf) {
    DBGIF_LOG2_ERROR("Invalid resBufSize %d or resBuf %p\n", resBufSize, resBuf);
    return -1;
  }

  /* Remove AT prefix */
  if (2 <= cmdLen && !strncasecmp("AT", cmdStr, strlen("AT"))) {
    atOffset = 2;
  }

  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return -1;
  }

  /* Send request */
  return atcmdSendCmd_request(cmdLen - atOffset + 1, cmdStr + atOffset, resBufSize, resBuf);
}
