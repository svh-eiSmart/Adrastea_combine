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
#include "apiutil.h"
#include "altcom_cc.h"
#include "gps/altcom_gps.h"
#include "apicmd_gps_setevent.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define SETEVENT_REQ_SUCCESS 0
#define SETEVENT_REQ_FAILURE -1

/****************************************************************************
 * Private Types
 ****************************************************************************/
struct gps_setevent_s {
  int32_t event;
  bool enable;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/
static bool g_setrepnmeastat_isproc = false;

/****************************************************************************
 * External Data
 ****************************************************************************/
extern event_report_cb_t g_gps_event_callback[EVENT_MAX_TYPE];
extern void *g_gps_event_cbpriv[EVENT_MAX_TYPE];

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static int altcom_gps_setnevent_req(struct gps_setevent_s *req) {
  FAR struct apicmd_setevent_s *cmd = NULL;
  FAR struct apicmd_seteventres_s *res = NULL;
  uint16_t reslen = 0;
  int32_t ret;

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_GPS_SETEV,
                                          sizeof(struct apicmd_setevent_s), (FAR void **)&res,
                                          sizeof(struct apicmd_seteventres_s))) {
    DBGIF_LOG_ERROR("altcom_generic_alloc_cmdandresbuff error\n");
    return SETEVENT_REQ_FAILURE;
  }

  memset(cmd, 0, sizeof(struct apicmd_setevent_s));

  cmd->event = htonl(req->event);
  cmd->value = (int32_t)req->enable;
  cmd->value = htonl(cmd->value);

  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, sizeof(struct apicmd_seteventres_s),
                      &reslen, 5000);

  if (ret < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", ret);
    goto errout_with_cmdfree;
  }

  if (reslen != sizeof(struct apicmd_seteventres_s)) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", reslen);
    goto errout_with_cmdfree;
  }

  ret = ntohl(res->result);

  DBGIF_LOG1_DEBUG("[setnevent-res]ret: %ld\n", ret);

  if (ret != APICMD_SETEVENT_RES_RET_CODE_OK) goto errout_with_cmdfree;

  altcom_generic_free_cmdandresbuff(cmd, res);

  return SETEVENT_REQ_SUCCESS;

errout_with_cmdfree:
  altcom_generic_free_cmdandresbuff(cmd, res);
  return SETEVENT_REQ_FAILURE;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief Enalbe/Disable events and callbacks hookup.
 *
 * @param [in] event: 1:NMEA 2:STATUS 3:ALLOW. See @ref eventType_e.
 * @param [in] enable: false:disable true:enable.
 * @param [in] callback: The callback function to be called on event arrival, see @ref
 * event_report_cb_t.
 * @param [in] userPriv: User's private parameter on callback.
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 *
 */

int altcom_gps_setevent(eventType_e event, bool enable, event_report_cb_t callback,
                        void *userPriv) {
  struct gps_setevent_s setevent;
  int result = 0;

  if (EVENT_MAX_TYPE <= event) {
    DBGIF_LOG1_ERROR("Invalid event type: %d\n", (int)event);
    return -EINVAL;
  }

  /* Check if the library is initialized */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return -EPERM;
  }

  /* Check this process runnning. */

  if (g_setrepnmeastat_isproc) {
    return -EBUSY;
  }

  g_setrepnmeastat_isproc = true;

  /* Check if callback registering only */
  if (altcom_isCbRegOnly()) {
    ALTCOM_CLR_CALLBACK(g_gps_event_callback[event], g_gps_event_cbpriv[event]);
    if (callback && enable) {
      ALTCOM_REG_CALLBACK(result, g_gps_event_callback[event], callback, g_gps_event_cbpriv[event],
                          userPriv);
    }

    g_setrepnmeastat_isproc = false;
    return result;
  }

  setevent.event = event;
  setevent.enable = enable;
  result = altcom_gps_setnevent_req(&setevent);
  if (result == SETEVENT_REQ_SUCCESS) {
    ALTCOM_CLR_CALLBACK(g_gps_event_callback[event], g_gps_event_cbpriv[event]);
    if (callback && setevent.enable) {
      ALTCOM_REG_CALLBACK(result, g_gps_event_callback[event], callback, g_gps_event_cbpriv[event],
                          userPriv);
    }

  } else {
    DBGIF_LOG1_ERROR("altcom_gps_setnevent_req faile, ret = %d\n", result);
  }

  g_setrepnmeastat_isproc = false;
  return result;
}
