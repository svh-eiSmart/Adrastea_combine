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

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <string.h>
#include <stdbool.h>

#include "dbg_if.h"
#include "lwm2m/altcom_lwm2m.h"
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

static LWM2MError_e lwm2mSendReadResp(client_inst_t client, int seqNum, bool readResult,
                                      uint8_t numOfUriData, lwm2m_uri_and_data_t *uriAndData) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_lwm2mreadresp_s *cmd = NULL;
  FAR struct apicmd_lwm2mreadrespres_s *res = NULL;
  struct apicmd_lwm2UriData_s *lwm2UriDataPtr;
  int numOfUriDataIdx;

  uint32_t datalen = 0;

  /* Allocate send and response command buffer */
  for (numOfUriDataIdx = 0; numOfUriDataIdx < numOfUriData; numOfUriDataIdx++) {
    if (uriAndData[numOfUriDataIdx].valType == RSRC_VALUE_TYPE_OPAQUE) {
      DBGIF_LOG1_DEBUG("DEBUG Read RSRC_VALUE_TYPE_OPAQUE data length: %d\r\n",
                       uriAndData[numOfUriDataIdx].resourceVal.opaqueVal.opaqueValLen);
      if (uriAndData[numOfUriDataIdx].resourceVal.opaqueVal.opaqueValLen +
              sizeof(((opaqueVal_t *)0)->opaqueValLen) >
          MAX_LWM2M_STRING_TYPE_LEN) {
        datalen += (uriAndData[numOfUriDataIdx].resourceVal.opaqueVal.opaqueValLen +
                    sizeof(((opaqueVal_t *)0)->opaqueValLen) - MAX_LWM2M_STRING_TYPE_LEN);
      }
    }

    datalen += sizeof(struct apicmd_lwm2UriData_s);
  }

  DBGIF_LOG1_DEBUG("read datalen: %lu\n", datalen);
  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_LWM2M_READ_RESP,
                                          LWM2MREAD_RESP_DATALEN + datalen, (FAR void **)&res,
                                          LWM2MREAD_RESP_RESP_DATALEN)) {
    return LWM2M_FAILURE;
  }
  /* Fill the data */
  cmd->client = (uint8_t)client;
  cmd->seqNum = htonl((uint32_t)seqNum);
  cmd->readResult = htonl((int)readResult);
  cmd->numOfUriData = numOfUriData;
  lwm2UriDataPtr =
      (struct apicmd_lwm2UriData_s *)((char *)cmd + sizeof(struct apicmd_lwm2mreadresp_s));
  // Copy URI and data to dest cmd object
  for (numOfUriDataIdx = 0; numOfUriDataIdx < numOfUriData; numOfUriDataIdx++) {
    // Printout debug msg
    DBGIF_LOG3_DEBUG("lwm2mSendReadResp objectId=%ld instnum=%ld resource=%ld ",
                     uriAndData[numOfUriDataIdx].objectId, uriAndData[numOfUriDataIdx].instanceNum,
                     uriAndData[numOfUriDataIdx].resourceNum);
    DBGIF_LOG2_DEBUG("rsrcint=%ld instype=%lu ", uriAndData[numOfUriDataIdx].resourceInstanceNum,
                     (uint32_t)uriAndData[numOfUriDataIdx].LWM2MInstncType);
    if (uriAndData[numOfUriDataIdx].valType == RSRC_VALUE_TYPE_OPAQUE) {
      DBGIF_LOG2_DEBUG("OPAQUE len=%hu hex buff is opaqueValPtr=%p\r\n",
                       uriAndData[numOfUriDataIdx].resourceVal.opaqueVal.opaqueValLen,
                       uriAndData[numOfUriDataIdx].resourceVal.opaqueVal.opaqueValPtr);
    } else if (uriAndData[numOfUriDataIdx].valType == RSRC_VALUE_TYPE_STRING) {
      DBGIF_LOG1_DEBUG("string sent in read=\"%s\"\r\n",
                       uriAndData[numOfUriDataIdx].resourceVal.strVal);
    } else if (uriAndData[numOfUriDataIdx].valType > RSRC_VALUE_TYPE_OPAQUE) {
      DBGIF_LOG1_ERROR("Invalid value type %lu\r\n", (uint32_t)uriAndData[numOfUriDataIdx].valType);
      ret = LWM2M_FAILURE;
      goto errout_with_cmdfree;
    } else {
      DBGIF_LOG1_DEBUG("val sent in read=%f\r\n", uriAndData[numOfUriDataIdx].resourceVal.value);
    }

    lwm2UriDataPtr->objectId = htonl((uint32_t)uriAndData[numOfUriDataIdx].objectId);
    lwm2UriDataPtr->instanceNum = htonl((uint32_t)uriAndData[numOfUriDataIdx].instanceNum);
    lwm2UriDataPtr->resourceNum = htonl((uint32_t)uriAndData[numOfUriDataIdx].resourceNum);
    lwm2UriDataPtr->resourceType = htonl((uint32_t)uriAndData[numOfUriDataIdx].LWM2MInstncType);
    lwm2UriDataPtr->resourceInst = htonl((uint32_t)uriAndData[numOfUriDataIdx].resourceInstanceNum);
    lwm2UriDataPtr->valType = (char)uriAndData[numOfUriDataIdx].valType;
    if (uriAndData[numOfUriDataIdx].valType == RSRC_VALUE_TYPE_STRING) {
      strncpy(lwm2UriDataPtr->resourceVal.strVal, uriAndData[numOfUriDataIdx].resourceVal.strVal,
              MAX_LWM2M_STRING_TYPE_LEN);
      // Advance src and dest ptr
      lwm2UriDataPtr = (struct apicmd_lwm2UriData_s *)((char *)lwm2UriDataPtr +
                                                       sizeof(struct apicmd_lwm2UriData_s));

    } else if (uriAndData[numOfUriDataIdx].valType == RSRC_VALUE_TYPE_OPAQUE) {
      lwm2UriDataPtr->resourceVal.opaqueVal.opaqueValLen =
          htons(uriAndData[numOfUriDataIdx].resourceVal.opaqueVal.opaqueValLen);
      memcpy(&lwm2UriDataPtr->resourceVal.opaqueVal.opaqueValPtr,
             uriAndData[numOfUriDataIdx].resourceVal.opaqueVal.opaqueValPtr,
             uriAndData[numOfUriDataIdx].resourceVal.opaqueVal.opaqueValLen);
      // next one point to the last end of OPAQUE buffer end
      if (uriAndData[numOfUriDataIdx].resourceVal.opaqueVal.opaqueValLen +
              sizeof(((opaqueVal_t *)0)->opaqueValLen) >
          MAX_LWM2M_STRING_TYPE_LEN) {
        lwm2UriDataPtr =
            (struct apicmd_lwm2UriData_s
                 *)((unsigned char *)&lwm2UriDataPtr->resourceVal.opaqueVal.opaqueValPtr +
                    uriAndData[numOfUriDataIdx].resourceVal.opaqueVal.opaqueValLen);
      } else {
        lwm2UriDataPtr = (struct apicmd_lwm2UriData_s *)((char *)lwm2UriDataPtr +
                                                         sizeof(struct apicmd_lwm2UriData_s));
      }
    } else {
      // for all other resource types including Boolean
      lwm2UriDataPtr->resourceVal.value = htond(uriAndData[numOfUriDataIdx].resourceVal.value);
      // Advance src and dest ptr
      lwm2UriDataPtr = (struct apicmd_lwm2UriData_s *)((char *)lwm2UriDataPtr +
                                                       sizeof(struct apicmd_lwm2UriData_s));
    }
  }

  /* Send command and block until receive a response */
  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, LWM2MREAD_RESP_RESP_DATALEN, &resLen,
                      ALT_OSAL_TIMEO_FEVR);

  DBGIF_LOG1_DEBUG("ReadRsp send to c-api res=%d\r\n", ret);
  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\r\n", ret);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  if (resLen != LWM2MREAD_RESP_RESP_DATALEN) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\r\n", resLen);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  /* Check API return code*/
  ret = ntohl(res->ret_code);
  if (ret != LWM2M_SUCCESS) {
    DBGIF_LOG1_ERROR("API command response ret :%ld.\n", ret);
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

LWM2MError_e altcom_lwm2mReadResp(client_inst_t client, int seqNum, bool readResult,
                                  uint8_t numOfUriData, lwm2m_uri_and_data_t *uriAndData) {
  LWM2MError_e ret;
  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LWM2M_FAILURE;
  }

  DBGIF_LOG1_DEBUG("call to READ RESP via C-API seqNum %d\r\n", seqNum);
  ret = lwm2mSendReadResp(client, seqNum, readResult, numOfUriData, uriAndData);
  if (ret != LWM2M_SUCCESS) {
    DBGIF_LOG1_ERROR("lwm2mSendReadResp fail, ret = %d\n", ret);
    return LWM2M_FAILURE;
  }
  return LWM2M_SUCCESS;
}
