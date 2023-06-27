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
#include "lwm2m/apicmd_lwm2m_notify.h"
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define LWM2MNOTIFY_REQ_DATALEN (sizeof(struct apicmd_lwm2mnotify_s))
#define LWM2MNOTIFY_RES_DATALEN (sizeof(struct apicmd_lwm2mnotifyres_s))

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lwm2mSendNotify
 *
 * Description:
 *   Send APICMDID_LWM2M_NOTIFY.
 *
 ****************************************************************************/

static LWM2MError_e lwm2mSendNotify(client_inst_t client, char *token, bool askForAckFromServer,
                                    unsigned char numOfUriData, lwm2m_uri_and_data_t *uriAndData) {
  int32_t ret;
  uint16_t resLen = 0;
  FAR struct apicmd_lwm2mnotify_s *cmd = NULL;
  FAR struct apicmd_lwm2mnotifyres_s *res = NULL;
  struct apicmd_lwm2UriData_s *lwm2UriDataPtr;
  int numOfUriDataIdx;
  int tmp_askForAckFromServer;

  /* Allocate send and response command buffer */
  if (uriAndData[0].valType != RSRC_VALUE_TYPE_OPAQUE) {
    if (!altcom_generic_alloc_cmdandresbuff(
            (FAR void **)&cmd, APICMDID_LWM2M_NOTIFY,
            LWM2MNOTIFY_REQ_DATALEN + (numOfUriData * sizeof(struct apicmd_lwm2UriData_s)),
            (FAR void **)&res, LWM2MNOTIFY_RES_DATALEN)) {
      return LWM2M_FAILURE;
    }
  } else {
    unsigned int totalHexBuffersLen = 0;
    for (numOfUriDataIdx = 0; numOfUriDataIdx < numOfUriData; numOfUriDataIdx++) {
      DBGIF_LOG1_DEBUG("DEBUG Notify RSRC_VALUE_TYPE_OPAQUE data length: %d\r\n",
                       uriAndData[numOfUriDataIdx].resourceVal.opaqueVal.opaqueValLen);
      totalHexBuffersLen += uriAndData[numOfUriDataIdx].resourceVal.opaqueVal.opaqueValLen;
    }
    DBGIF_LOG2_DEBUG("OPAQUE numOfUriData=%d totalHexBuffersLen=%d\r\n", numOfUriData,
                     totalHexBuffersLen);
    if (!altcom_generic_alloc_cmdandresbuff(
            (FAR void **)&cmd, APICMDID_LWM2M_NOTIFY,
            LWM2MNOTIFY_REQ_DATALEN + totalHexBuffersLen +
                (numOfUriData * (3 /*alignment spare*/ + sizeof(struct apicmd_lwm2UriData_s))),
            (FAR void **)&res, LWM2MNOTIFY_RES_DATALEN)) {
      return LWM2M_FAILURE;
    }
  }

  /* Fill the data */
  cmd->client = (uint8_t)client;
  tmp_askForAckFromServer = (int)askForAckFromServer;
  cmd->askForAckFromServer = htonl(tmp_askForAckFromServer);
  strncpy(cmd->token, token, 16);
  cmd->token[16] = 0;
  cmd->numOfUriData = numOfUriData;
  lwm2UriDataPtr =
      (struct apicmd_lwm2UriData_s *)((char *)cmd + sizeof(struct apicmd_lwm2mnotify_s));
  // Copy URI and data to dest cmd object
  for (numOfUriDataIdx = 0; numOfUriDataIdx < numOfUriData; numOfUriDataIdx++) {
    // Printout debug msg
    if (uriAndData[numOfUriDataIdx].valType == RSRC_VALUE_TYPE_OPAQUE) {
      DBGIF_LOG2_DEBUG("lwm2mSendNotify OPAQUE len=%hu hex buff is opaqueValPtr=%p\r\n",
                       uriAndData[numOfUriDataIdx].resourceVal.opaqueVal.opaqueValLen,
                       uriAndData[numOfUriDataIdx].resourceVal.opaqueVal.opaqueValPtr);
    } else if (uriAndData[numOfUriDataIdx].valType != RSRC_VALUE_TYPE_STRING) {
      DBGIF_LOG3_DEBUG("lwm2mSendNotify objectId=%ld resource=%ld val sent in Notify=%f\r\n",
                       uriAndData[numOfUriDataIdx].objectId,
                       uriAndData[numOfUriDataIdx].resourceNum,
                       uriAndData[numOfUriDataIdx].resourceVal.value);
    }

    lwm2UriDataPtr->objectId = htonl((uint32_t)uriAndData[numOfUriDataIdx].objectId);
    lwm2UriDataPtr->instanceNum = htonl((uint32_t)uriAndData[numOfUriDataIdx].instanceNum);
    lwm2UriDataPtr->resourceNum = htonl((uint32_t)uriAndData[numOfUriDataIdx].resourceNum);
    lwm2UriDataPtr->resourceType = htonl((uint32_t)uriAndData[numOfUriDataIdx].LWM2MInstncType);
    lwm2UriDataPtr->resourceInst = htonl((uint32_t)uriAndData[numOfUriDataIdx].resourceInstanceNum);
    lwm2UriDataPtr->valType = uriAndData[numOfUriDataIdx].valType;
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

      free(uriAndData[numOfUriDataIdx].resourceVal.opaqueVal.opaqueValPtr);
      uriAndData[numOfUriDataIdx].resourceVal.opaqueVal.opaqueValPtr =
          0;  // sign that is was released
      // next one point to the last end of OPAQUE buffer end
      lwm2UriDataPtr =
          (struct apicmd_lwm2UriData_s
               *)((unsigned char *)&lwm2UriDataPtr->resourceVal.opaqueVal.opaqueValPtr +
                  uriAndData[numOfUriDataIdx].resourceVal.opaqueVal.opaqueValLen);

      //"Fix" next element ptr to be aligned
      char *addrToAlignPtr = (char *)lwm2UriDataPtr;
      if (((unsigned int)addrToAlignPtr & 0x03) != 0) {
        //--> not aligned 32 bits (4 bytes)
        addrToAlignPtr = (char *)(((unsigned int)addrToAlignPtr + 3) & ~0x03);
        lwm2UriDataPtr = (struct apicmd_lwm2UriData_s *)addrToAlignPtr;
      }

    } else {  // for all other resource types including Boolean
      lwm2UriDataPtr->resourceVal.value = htond(uriAndData[numOfUriDataIdx].resourceVal.value);
      // Advance src and dest ptr
      lwm2UriDataPtr = (struct apicmd_lwm2UriData_s *)((char *)lwm2UriDataPtr +
                                                       sizeof(struct apicmd_lwm2UriData_s));
    }
  }
  /* Send command and block until receive a response */
  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, LWM2MNOTIFY_RES_DATALEN, &resLen,
                      ALTCOM_SYS_TIMEO_FEVR);

  DBGIF_LOG1_DEBUG("Notify send to c-api res=%d\r\n", ret);
  /* Check GW return */
  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\r\n", ret);
    ret = LWM2M_FAILURE;
    goto errout_with_cmdfree;
  }

  if (resLen != LWM2MNOTIFY_RES_DATALEN) {
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

  DBGIF_LOG1_DEBUG("[lwm2mSendNotify-res]ret: %ld\n", ret);

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return (LWM2MError_e)ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief altcom_lwm2mSendNotify() ask MAP lwm2m client to send notify with data to the server.
 *
 * @param [in] client: Specify the target client instance to operate.
 * @param [in] token: the LWM2M token string which would be send with the notification/report.
 * @param [in] askForAckFromServer: Determine if notify is sent with request for ack from server.
 * @param [in] numOfUriData: numbers of reports included within the following uriAndData array.
 * @param [in] uriAndData: array of reports to be sent to server.
 *
 * @return LWM2M_SUCCESS on success; LWM2M_FAILURE on fail.
 *
 */
LWM2MError_e altcom_lwm2mSendNotify(client_inst_t client, char *token, bool askForAckFromServer,
                                    unsigned char numOfUriData, lwm2m_uri_and_data_t *uriAndData) {
  LWM2MError_e ret;
  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return LWM2M_FAILURE;
  }

  DBGIF_LOG1_DEBUG("call to Notify via C-API numOfUriData=%d\r\n", (int)numOfUriData);
  ret = lwm2mSendNotify(client, token, askForAckFromServer, numOfUriData, uriAndData);
  if (ret != LWM2M_SUCCESS) {
    DBGIF_LOG1_ERROR("lwm2mSendNotify fail, ret = %d\n", ret);
    return LWM2M_FAILURE;
  }
  return LWM2M_SUCCESS;
}
