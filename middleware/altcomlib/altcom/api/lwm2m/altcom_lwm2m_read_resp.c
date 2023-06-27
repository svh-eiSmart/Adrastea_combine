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
#include "lwm2m/altcom_lwm2m.h"
#include "osal/altcom_osal.h"
#include "util/apiutil.h"
#include "apicmd.h"
#include "lwm2m/apicmd_lwm2m_read_resp.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define LWM2MREAD_RESP_DATALEN (sizeof(struct apicmd_lwm2mreadresp_s))
#define LWM2MREAD_RESP_RESP_DATALEN (sizeof(struct apicmd_lwm2mreadrespres_s))

/****************************************************************************
 * Private Functions
 ****************************************************************************/
/****************************************************************************
 * Name: mqttConnect_request
 *
 * Description:
 *   Send APICMDID_LWM2M_HOST_ENABLE.
 *
 ****************************************************************************/

static LWM2MError_e lwm2mSendReadResp(client_inst_t client, int seqNum, int objectId,
                                      int instanceNum, int resourceNum, int resourceInst,
                                      bool readResult, LWM2MRsrc_type_e valType,
                                      resourceVal_t *resourceVal) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_lwm2mreadresp_s *cmd = NULL;
  FAR struct apicmd_lwm2mreadrespres_s *res = NULL;
  int tmp_readResult;

  /* Allocate send and response command buffer */
  if (valType != RSRC_VALUE_TYPE_OPAQUE) {
    if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_LWM2M_READ_RESP,
                                            LWM2MREAD_RESP_DATALEN, (FAR void **)&res,
                                            LWM2MREAD_RESP_RESP_DATALEN)) {
      return LWM2M_FAILURE;
    }
  } else {
    if (!altcom_generic_alloc_cmdandresbuff(
            (FAR void **)&cmd, APICMDID_LWM2M_READ_RESP,
            LWM2MREAD_RESP_DATALEN + resourceVal->opaqueVal.opaqueValLen, (FAR void **)&res,
            LWM2MREAD_RESP_RESP_DATALEN)) {
      return LWM2M_FAILURE;
    }
  }
  if ((valType != RSRC_VALUE_TYPE_STRING) && (valType != RSRC_VALUE_TYPE_OPAQUE)) {
    DBGIF_LOG1_DEBUG("lwm2mSendReadResp value=%f\r\n", resourceVal->value);
  }
  /* Fill the data */
  cmd->client = (uint8_t)client;
  cmd->seqNum = htonl((uint32_t)seqNum);
  cmd->objectId = htonl((uint32_t)objectId);
  cmd->instanceNum = htonl((uint32_t)instanceNum);
  cmd->resourceNum = htonl((uint32_t)resourceNum);
  tmp_readResult = (int)readResult;
  cmd->readResult = htonl(tmp_readResult);
  cmd->valType = valType;
  if (resourceInst == RSRC_INSTNC_TYPE_SINGLE) {
    cmd->resourceType = htonl((uint32_t)RSRC_INSTNC_TYPE_SINGLE);
  } else {
    cmd->resourceType = htonl((uint32_t)RSRC_INSTNC_TYPE_MULTIPLE);
    cmd->resourceInst = htonl((uint32_t)resourceInst);
  }

  if (valType == RSRC_VALUE_TYPE_STRING) {
    strncpy(cmd->resourceVal.strVal, resourceVal->strVal, (MAX_LWM2M_STRING_TYPE_LEN));
    cmd->resourceVal.strVal[MAX_LWM2M_STRING_TYPE_LEN - 1] = 0;
    DBGIF_LOG1_DEBUG("lwm2mSendReadResp value str=%s\r\n", cmd->resourceVal.strVal);
  } else if (valType == RSRC_VALUE_TYPE_OPAQUE) {
    cmd->resourceVal.opaqueVal.opaqueValLen = htons((uint16_t)resourceVal->opaqueVal.opaqueValLen);
    DBGIF_LOG1_DEBUG("lwm2mSendReadResp value Opaque len=%d\r\n",
                     resourceVal->opaqueVal.opaqueValLen);
    memcpy((char *)&cmd->resourceVal.opaqueVal.opaqueValPtr, resourceVal->opaqueVal.opaqueValPtr,
           resourceVal->opaqueVal.opaqueValLen);
  } else {  // for all other resource types including Boolean
    cmd->resourceVal.value = htond(resourceVal->value);
  }

  /* Send command and block until receive a response */
  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, LWM2MREAD_RESP_RESP_DATALEN, &resLen,
                      ALTCOM_SYS_TIMEO_FEVR);
  DBGIF_LOG_DEBUG("after sending read resp\r\n");
  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  if (resLen != LWM2MREAD_RESP_RESP_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", resLen);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);
  if (ret != APICMD_READ_RESP_RES_RET_CODE_OK) {
    DBGIF_LOG1_ERROR("API command response is ret :%ld.\n", ret);
    goto errout_with_cmdfree;
  }

  DBGIF_LOG1_DEBUG("[lwm2mSendReadResp-res]ret: %ld\n", ret);

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (LWM2MError_e)ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief altcom_lwm2mReadResp() send back to MAP lwm2m client resp with the value in respect to a
 * read request of a specific uri.
 *
 * @param [in] client: Specify the target client instance to operate.
 * @param [in] seqNum: sequence number of request to read that must be part of corresponding
 * respond.
 * @param [in] objectId: object id of uri to send read resp value for.
 * @param [in] instanceNum: instance Number of uri to send read resp value for.
 * @param [in] resourceNum: resource Number of uri to send read resp value for.
 * @param [in] resourceInst: instance Number of uri to send read resp value for.
 * @param [in] readResult: result of actual read of uri.
 * @param [in] valType: The value type of the read element uri.
 * @param [in] resourceVal: The value send in respond to read request of specific uri
 *
 * @return LWM2MError_e is returned.
 */

LWM2MError_e altcom_lwm2mReadResp(client_inst_t client, int seqNum, int objectId, int instanceNum,
                                  int resourceNum, int resourceInst, bool readResult,
                                  LWM2MRsrc_type_e valType, resourceVal_t *resourceVal) {
  LWM2MError_e ret;
  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LWM2M_FAILURE;
  }

  DBGIF_LOG2_DEBUG("call to READ RESP via C-API seqNum %d valType=%d\r\n", seqNum, valType);
  ret = lwm2mSendReadResp(client, seqNum, objectId, instanceNum, resourceNum, resourceInst,
                          readResult, valType, resourceVal);
  if (ret != LWM2M_SUCCESS) {
    DBGIF_LOG1_ERROR("lwm2mSendReadResp fail, ret = %d\n", ret);
    return LWM2M_FAILURE;
  }
  return LWM2M_SUCCESS;
}
