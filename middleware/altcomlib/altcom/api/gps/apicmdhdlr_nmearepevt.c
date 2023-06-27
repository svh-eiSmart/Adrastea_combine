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
#include "gps/altcom_gps.h"
#include "buffpoolwrapper.h"
#include "evthdlbs.h"
#include "apiutil.h"
#include "apicmd_gps_nmea.h"
#include "apicmdhdlrbs.h"
#include "altcom_osal.h"

/****************************************************************************
 * External Data
 ****************************************************************************/
extern event_report_cb_t g_event_nmea_callback;
extern void *g_event_nmea_cbpriv;
extern event_report_cb_t g_event_allowedst_callback;
extern void *g_event_allowedst_cbpriv;
extern event_report_cb_t g_event_sessionst_callback;
extern void *g_event_sessionst_cbpriv;
extern event_report_cb_t g_event_gnssfix_callback;
extern void *g_event_gnssfix_cbpriv;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void repevt_nmea_report(FAR struct apicmd_event_s *event) {
  gps_event_t evt;
  event_report_cb_t callback;
  void *cbpriv;

  evt.eventType = (eventType_e)event->eventType;
  evt.nmeaType = (nmeaType_e)event->nmeaType;

  if (evt.nmeaType == EVENT_NMEA_GGA_TYPE) {
    evt.u.gga.UTC = ntohd(event->u.gga.UTC);
    evt.u.gga.lat = ntohd(event->u.gga.lat);
    evt.u.gga.dirlat = event->u.gga.dirlat;
    evt.u.gga.lon = ntohd(event->u.gga.lon);
    evt.u.gga.dirlon = event->u.gga.dirlon;
    evt.u.gga.quality = ntohs(event->u.gga.quality);
    evt.u.gga.numsv = ntohs(event->u.gga.numsv);
    evt.u.gga.hdop = ntohd(event->u.gga.hdop);
    evt.u.gga.ortho = ntohd(event->u.gga.ortho);
    evt.u.gga.height = event->u.gga.height;
    evt.u.gga.geoid = ntohd(event->u.gga.geoid);
    evt.u.gga.geosep = event->u.gga.geosep;
    evt.u.gga.dgpsupdtime = ntohd(event->u.gga.dgpsupdtime);
    evt.u.gga.refstaid = ntohs(event->u.gga.refstaid);
    evt.u.gga.cksum = ntohs(event->u.gga.cksum);
  } else if (evt.nmeaType == EVENT_NMEA_GSV_TYPE) {
    evt.u.gsv.numSV = ntohs(event->u.gsv.numSV);
    for (int i = 0; i < GSV_MAX_OF_SAT; i++) {
      evt.u.gsv.sat[i].SNR = ntohs(event->u.gsv.sat[i].SNR);
      evt.u.gsv.sat[i].PRN = ntohs(event->u.gsv.sat[i].PRN);
      evt.u.gsv.sat[i].elevation = ntohs(event->u.gsv.sat[i].elevation);
      evt.u.gsv.sat[i].azimuth = ntohs(event->u.gsv.sat[i].azimuth);
    }
  } else if (evt.nmeaType == EVENT_NMEA_RMC_TYPE) {
    evt.u.rmc.UTC = ntohd(event->u.rmc.UTC);
    evt.u.rmc.status = event->u.rmc.status;
    evt.u.rmc.lat = ntohd(event->u.rmc.lat);
    evt.u.rmc.dirlat = event->u.rmc.dirlat;
    evt.u.rmc.lon = ntohd(event->u.rmc.lon);
    evt.u.rmc.dirlon = event->u.rmc.dirlon;
    evt.u.rmc.speed = ntohd(event->u.rmc.speed);
    evt.u.rmc.angle = ntohd(event->u.rmc.angle);
    evt.u.rmc.date = ntohd(event->u.rmc.date);
    evt.u.rmc.magnet = ntohd(event->u.rmc.magnet);
    evt.u.rmc.dirmagnet = event->u.rmc.dirmagnet;
    evt.u.rmc.cksum = ntohs(event->u.rmc.cksum);
  }

  ALTCOM_GET_CALLBACK(g_event_nmea_callback, callback, g_event_nmea_cbpriv, cbpriv);
  if (callback) {
    callback(&evt, cbpriv);
  }
}

static void repevt_allowst_report(FAR struct apicmd_event_s *event) {
  gps_event_t evt;
  event_report_cb_t callback;
  void *cbpriv;

  evt.eventType = (eventType_e)event->eventType;
  if (EVENT_ALLOWEDST_TYPE == evt.eventType) {
    evt.u.allowedst = event->u.allowedst;
    ALTCOM_GET_CALLBACK(g_event_allowedst_callback, callback, g_event_allowedst_cbpriv, cbpriv);
  } else {
    evt.u.sessionst = event->u.sessionst;
    ALTCOM_GET_CALLBACK(g_event_sessionst_callback, callback, g_event_sessionst_cbpriv, cbpriv);
  }

  if (callback) {
    callback(&evt, cbpriv);
  }
}

static void repevt_gnssfix_report(FAR struct apicmd_event_s *event) {
  gps_event_t evt;
  event_report_cb_t callback;
  void *cbpriv;

  evt.eventType = (eventType_e)event->eventType;

  evt.u.fixst = event->u.fixst;
  ALTCOM_GET_CALLBACK(g_event_gnssfix_callback, callback, g_event_gnssfix_cbpriv, cbpriv);

  if (callback) {
    callback(&evt, cbpriv);
  }
}

static void repevt_job(FAR void *arg) {
  FAR struct apicmd_event_s *evt;
  evt = (FAR struct apicmd_event_s *)arg;

  switch ((eventType_e)evt->eventType) {
    case EVENT_NMEA_TYPE:
      repevt_nmea_report(evt);
      break;

    case EVENT_SESSIONST_TYPE:
    case EVENT_ALLOWEDST_TYPE:
      repevt_allowst_report(evt);
      break;

    case EVENT_FIX_TYPE:
      repevt_gnssfix_report(evt);
      break;
    default:
      DBGIF_LOG1_ERROR("Unknown GPS event type :%d.\n", (int)evt->eventType);
      break;
  }

  altcom_free_cmd((FAR uint8_t *)arg);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
enum evthdlrc_e apicmdhdlr_nmearepevt(FAR uint8_t *evt, uint32_t evlen) {
  return apicmdhdlrbs_do_runjob(evt, APICMDID_GPS_IGNSSEVU, repevt_job);
}
