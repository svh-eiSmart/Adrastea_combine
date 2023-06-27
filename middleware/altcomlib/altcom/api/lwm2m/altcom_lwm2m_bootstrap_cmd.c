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

#include "dbg_if.h"
#include "lwm2m/altcom_lwm2m.h"
#include "buffpoolwrapper.h"
#include "evthdlbs.h"
#include "apiutil.h"
#include "apicmdhdlrbs.h"
#include "altcom_osal.h"
#include "apicmd.h"
#include "lwm2m/apicmd_lwm2m_bootstrap_cmd.h"
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define LWM2MBOOTSTRAPSTART_REQ_DATALEN (sizeof(struct apicmd_lwm2BootstrapCmdStart_s))
#define LWM2MBOOTSTRAPSTART_RES_DATALEN (sizeof(struct apicmd_lwm2BootstrapCmdStartres_s))

#define LWM2MBOOTSTRAPDELETE_REQ_DATALEN (sizeof(struct apicmd_lwm2BootstrapCmdDelete_s))
#define LWM2MBOOTSTRAPDELETE_RES_DATALEN (sizeof(struct apicmd_lwm2BootstrapCmdDeleteres_s))

#define LWM2MBOOTSTRAPDONE_REQ_DATALEN (sizeof(struct apicmd_lwm2BootstrapCmdDone_s))
#define LWM2MBOOTSTRAPDONE_RES_DATALEN (sizeof(struct apicmd_lwm2BootstrapCmdDoneres_s))

#define LWM2MBOOTSTRAPCREATE_REQ_DATALEN (sizeof(struct apicmd_lwm2BootstrapCmdCreat_s))
#define LWM2MBOOTSTRAPCREATE_RES_DATALEN (sizeof(struct apicmd_lwm2BootstrapCmdCreatres_s))

/****************************************************************************
 * External Data
 ****************************************************************************/
/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lwm2mSendBootstrapCmdStart
 *
 * Description:
 *   Send APICMDID_LWM2M_BOOTSTRAP_START.
 *
 ****************************************************************************/

static LWM2MError_e lwm2mSendBootstrapCmdStart(client_inst_t client, char *FactoryBsName) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_lwm2BootstrapCmdStart_s *cmd = NULL;
  FAR struct apicmd_lwm2BootstrapCmdStartres_s *res = NULL;
  char *factoryBsNamePtr;
  unsigned int strLenFactoryBsName = 0;

  if (FactoryBsName != NULL) {
    strLenFactoryBsName = strlen(FactoryBsName);
  }
  /* Allocate send and response command buffer */
  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_LWM2M_BOOTSTRAP_START,
                                          LWM2MBOOTSTRAPSTART_REQ_DATALEN + (strLenFactoryBsName),
                                          (FAR void **)&res, LWM2MBOOTSTRAPSTART_RES_DATALEN)) {
    return LWM2M_FAILURE;
  }

  /* Fill the data */
  cmd->client = (uint8_t)client;
  cmd->factoryBsNameLen = strLenFactoryBsName;
  factoryBsNamePtr = &cmd->factoryBsName;

  if (FactoryBsName) {
    strncpy(factoryBsNamePtr, FactoryBsName, strLenFactoryBsName);
  } else {
    *factoryBsNamePtr = 0;
  }

  /* Send command and block until receive a response */
  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, LWM2MBOOTSTRAPSTART_RES_DATALEN,
                      &resLen, ALTCOM_SYS_TIMEO_FEVR);

  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\r\n", ret);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  if (resLen != LWM2MBOOTSTRAPSTART_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\r\n", resLen);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);
  if (ret != APICMD_BOOTSTARP_RES_RET_CODE_OK) {
    DBGIF_LOG1_ERROR("API command response ret :%ld.\n", ret);
    goto errout_with_cmdfree;
  }

  DBGIF_LOG1_DEBUG("[lwm2mSendBootstrapCmdStart-res]ret: %ld\n", ret);

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (LWM2MError_e)ret;
}

/****************************************************************************
 * Name: lwm2mSendBootstrapCmdDelete
 *
 * Description:
 *   Send APICMDID_LWM2M_BOOTSTRAP_DELETE.
 *
 ****************************************************************************/
static LWM2MError_e lwm2mSendBootstrapCmdDelete(client_inst_t client) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_lwm2BootstrapCmdDelete_s *cmd = NULL;
  FAR struct apicmd_lwm2BootstrapCmdDeleteres_s *res = NULL;

  /* Allocate send and response command buffer */
  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_LWM2M_BOOTSTRAP_DELETE,
                                          LWM2MBOOTSTRAPDELETE_REQ_DATALEN, (FAR void **)&res,
                                          LWM2MBOOTSTRAPDELETE_RES_DATALEN)) {
    return LWM2M_FAILURE;
  }

  cmd->client = (uint8_t)client;

  /* Send command and block until receive a response */
  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, LWM2MBOOTSTRAPDELETE_RES_DATALEN,
                      &resLen, ALTCOM_SYS_TIMEO_FEVR);

  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\r\n", ret);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  if (resLen != LWM2MBOOTSTRAPDELETE_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\r\n", resLen);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);
  if (ret != APICMD_BOOTSTARP_RES_RET_CODE_OK) {
    DBGIF_LOG1_ERROR("API command response ret :%ld.\n", ret);
    goto errout_with_cmdfree;
  }

  DBGIF_LOG1_DEBUG("[lwm2mSendBootstrapCmdDelete-res]ret: %ld\n", ret);

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (LWM2MError_e)ret;
}

/****************************************************************************
 * Name: lwm2mSendBootstrapCmdDone
 *
 * Description:
 *   Send APICMDID_LWM2M_BOOTSTRAP_DONE.
 *
 ****************************************************************************/
static LWM2MError_e lwm2mSendBootstrapCmdDone(client_inst_t client) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_lwm2BootstrapCmdDone_s *cmd = NULL;
  FAR struct apicmd_lwm2BootstrapCmdDoneres_s *res = NULL;

  /* Allocate send and response command buffer */
  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_LWM2M_BOOTSTRAP_DONE,
                                          LWM2MBOOTSTRAPDELETE_REQ_DATALEN, (FAR void **)&res,
                                          LWM2MBOOTSTRAPDONE_RES_DATALEN)) {
    return LWM2M_FAILURE;
  }

  cmd->client = (uint8_t)client;

  /* Send command and block until receive a response */
  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, LWM2MBOOTSTRAPDONE_RES_DATALEN,
                      &resLen, ALTCOM_SYS_TIMEO_FEVR);

  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\r\n", ret);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  if (resLen != LWM2MBOOTSTRAPDONE_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\r\n", resLen);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);
  if (ret != APICMD_BOOTSTARP_RES_RET_CODE_OK) {
    DBGIF_LOG1_ERROR("API command response ret :%ld.\n", ret);
    goto errout_with_cmdfree;
  }

  DBGIF_LOG1_DEBUG("[lwm2mSendBootstrapCmdDone-res]ret: %ld\n", ret);

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (LWM2MError_e)ret;
}

static LWM2MError_e lwm2mSendBootstrapCmdCreate(client_inst_t client, int32_t objectId,
                                                int32_t instanceNum,
                                                unsigned char numOfResrcNumData,
                                                lwm2m_resource_and_data_t *resrcAndData) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_lwm2BootstrapCmdCreat_s *cmd = NULL;
  FAR struct apicmd_lwm2BootstrapCmdCreatres_s *res = NULL;
  lwm2m_resource_and_data_t *lwm2UriDataPtr;
  int numOfUriDataIdx;

  /* Allocate send and response command buffer */
  if (!altcom_generic_alloc_cmdandresbuff(
          (FAR void **)&cmd, APICMDID_LWM2M_BOOTSTRAP_CREATE,
          LWM2MBOOTSTRAPCREATE_REQ_DATALEN +
              (numOfResrcNumData * sizeof(lwm2m_resource_and_data_t)),
          (FAR void **)&res, LWM2MBOOTSTRAPCREATE_RES_DATALEN)) {
    return LWM2M_FAILURE;
  }
  /* Fill the data */
  cmd->client = (uint8_t)client;
  cmd->objectId = htonl((uint32_t)objectId);
  cmd->instanceNum = htonl((uint32_t)instanceNum);
  cmd->numOfResrcNumData = numOfResrcNumData;
  lwm2UriDataPtr = (lwm2m_resource_and_data_t *)((char *)&cmd->ResrcNumDataStart);
  // Copy RSRC and data to dest cmd object
  for (numOfUriDataIdx = 0; numOfUriDataIdx < numOfResrcNumData;
       numOfUriDataIdx++, lwm2UriDataPtr++) {
    DBGIF_LOG3_DEBUG("lwm2mSendBootstrapCmdCreate Add objectId=%d resource=%d valType=%d\r\n",
                     objectId, resrcAndData[numOfUriDataIdx].resourceNum,
                     resrcAndData[numOfUriDataIdx].valType);
    lwm2UriDataPtr->resourceNum = htonl((uint32_t)resrcAndData[numOfUriDataIdx].resourceNum);
    lwm2UriDataPtr->valType = resrcAndData[numOfUriDataIdx].valType;

    if (lwm2UriDataPtr->valType == RSRC_VALUE_TYPE_STRING) {
      strncpy(lwm2UriDataPtr->strVal, resrcAndData[numOfUriDataIdx].strVal,
              (MAX_LWM2M_STRING_TYPE_LEN - 1));
      lwm2UriDataPtr->strVal[MAX_LWM2M_STRING_TYPE_LEN - 1] = 0;
      DBGIF_LOG1_DEBUG("lwm2mSendBootstrapCmdCreate add value str=%s\r\n", lwm2UriDataPtr->strVal);

    } else {
      lwm2UriDataPtr->value = htond(resrcAndData[numOfUriDataIdx].value);
      DBGIF_LOG1_DEBUG("lwm2mSendBootstrapCmdCreate add value =%f\r\n",
                       resrcAndData[numOfUriDataIdx].value);
    }
  }

  /* Send command and block until receive a response */
  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, LWM2MBOOTSTRAPCREATE_RES_DATALEN,
                      &resLen, ALTCOM_SYS_TIMEO_FEVR);

  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\r\n", ret);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  if (resLen != LWM2MBOOTSTRAPCREATE_RES_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\r\n", resLen);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);
  if (ret != LWM2M_SUCCESS) {
    DBGIF_LOG1_ERROR("API command response ret :%ld.\n", ret);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  DBGIF_LOG1_DEBUG("[lwm2mSendBootstrapCmdCreate-res]ret: %ld\n", ret);

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (LWM2MError_e)ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief altcom_lwm2mSendBootstrapCmdStart() Send to  MAP lwm2m client the
 * APICMDID_LWM2M_BOOTSTRAP_START.
 *
 * @param [in] client: Specify the target client instance to operate.
 * @param [in] factoryBsName: the LWM2M Bootstrap owner name string which would be send with the
 * notification/report.
 *
 * @return LWM2M_SUCCESS on success; LWM2M_FAILURE on fail.
 *
 */
LWM2MError_e altcom_lwm2mSendBootstrapCmdStart(client_inst_t client, char *factoryBsName) {
  LWM2MError_e ret;
  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LWM2M_FAILURE;
  }
  if (factoryBsName) {
    DBGIF_LOG1_DEBUG("Call to send BOOTSTRAP  START via C-API factoryBsName=%s\r\n", factoryBsName);
  } else {
    DBGIF_LOG_DEBUG("Call to send BOOTSTRAP  START via C-API \r\n");
  }

  ret = lwm2mSendBootstrapCmdStart(client, factoryBsName);
  if (ret != LWM2M_SUCCESS) {
    DBGIF_LOG1_ERROR("lwm2mSendBootstrapCmdStart fail, ret = %d\n", ret);
    return LWM2M_FAILURE;
  }
  return LWM2M_SUCCESS;
}

/**
 * @brief altcom_lwm2mSendBootstrapCmdDelete() Send to  MAP lwm2m client the
 * APICMDID_LWM2M_BOOTSTRAP_DELETE.
 *
 * @param [in] client: Specify the target client instance to operate.
 *
 * @return LWM2M_SUCCESS on success; LWM2M_FAILURE on fail.
 *
 */

LWM2MError_e altcom_lwm2mSendBootstrapCmdDelete(client_inst_t client) {
  LWM2MError_e ret;
  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LWM2M_FAILURE;
  }
  DBGIF_LOG_DEBUG("Call to send BOOTSTRAP  DELETE  via C-API \r\n");

  ret = lwm2mSendBootstrapCmdDelete(client);

  if (ret != LWM2M_SUCCESS) {
    DBGIF_LOG1_ERROR("lwm2mSendBootstrapCmdDelete fail, ret = %d\n", ret);
    return LWM2M_FAILURE;
  }
  return LWM2M_SUCCESS;
}

/**
 * @brief altcom_lwm2mSendBootstrapCmdDone() Send to  MAP lwm2m client the
 * APICMDID_LWM2M_BOOTSTRAP_DONE.
 *
 * @param [in] client: Specify the target client instance to operate.
 *
 * @return LWM2M_SUCCESS on success; LWM2M_FAILURE on fail.
 *
 */

LWM2MError_e altcom_lwm2mSendBootstrapCmdDone(client_inst_t client) {
  LWM2MError_e ret;
  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LWM2M_FAILURE;
  }
  DBGIF_LOG_DEBUG("Call to send BOOTSTRAP  DONE  via C-API \r\n");

  ret = lwm2mSendBootstrapCmdDone(client);

  if (ret != LWM2M_SUCCESS) {
    DBGIF_LOG1_ERROR("lwm2mSendBootstrapCmdDone fail, ret = %d\n", ret);
    return LWM2M_FAILURE;
  }
  return LWM2M_SUCCESS;
}

/**
 * @brief altcom_lwm2mSendBootstrapCmdCreate() Send to  MAP lwm2m client the
 * APICMDID_LWM2M_BOOTSTRAP_CREATE.
 *
 * @param [in] client: Specify the target client instance to operate.
 * @param [in] objectId: object id of uri to send create values for.
 * @param [in] instanceNum: instance Number of uri send create values for.
 * @param [in] numOfResrcNumData: numbers array members that include resources and values included
 * within the following resrcAndData array.
 * @param [in] resrcAndData: array of resources and values to be set at LWM2M client configuration.
 *
 * @return LWM2M_SUCCESS on success; LWM2M_FAILURE on fail.
 *
 */

LWM2MError_e altcom_lwm2mSendBootstrapCmdCreate(client_inst_t client, int objectId,
                                                int32_t instanceNum,
                                                unsigned char numOfResrcNumData,
                                                lwm2m_resource_and_data_t *resrcAndData) {
  LWM2MError_e ret;
  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LWM2M_FAILURE;
  }
  DBGIF_LOG_DEBUG("Call to send BOOTSTRAP  CREATE  via C-API \r\n");

  ret = lwm2mSendBootstrapCmdCreate(client, objectId, instanceNum, numOfResrcNumData, resrcAndData);

  if (ret != LWM2M_SUCCESS) {
    DBGIF_LOG1_ERROR("lwm2mSendBootstrapCmdCreate fail, ret = %d\n", ret);
    return LWM2M_FAILURE;
  }

  return LWM2M_SUCCESS;
}
