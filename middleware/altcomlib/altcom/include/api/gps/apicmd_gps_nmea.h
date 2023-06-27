/****************************************************************************
 * modules/lte/altcom/include/api/gps/apicmd_gps_gpsnmeagga.h
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

#ifndef __MODULES_LTE_ALTCOM_INCLUDE_API_GPS_APICMD_GPS_NMEA_H
#define __MODULES_LTE_ALTCOM_INCLUDE_API_GPS_APICMD_GPS_NMEA_H

#include "apicmd.h"

struct apicmd_gpsgga_repevt_s {
  double UTC;          // UTC of position fix
  double lat;          // Latitude
  char dirlat;         // Direction of latitude: N: North S: South
  double lon;          // Longitude
  char dirlon;         // Direction of longitude: E: East W: West
  int16_t quality;     // GPS Quality indicator:
  int16_t numsv;       // Number of SVs in use
  double hdop;         // HDOP
  double ortho;        // Orthometric height (MSL reference)
  char height;         // unit of measure for orthometric height is meters
  double geoid;        // Geoid separation
  char geosep;         // geoid separation measured in meters
  double dgpsupdtime;  // Age of differential GPS data record, Type 1 or Type 9. Null field when
                       // DGPS is not used.
  int16_t refstaid;    // Reference station ID
  int16_t cksum;       // The checksum data, always begins with *
};

struct apicmd_gpsgsv_sat_s {
  uint16_t PRN;
  uint16_t elevation;
  uint16_t azimuth;
  uint16_t SNR;
};

struct apicmd_gpsrmc_repevt_s {
  double UTC;      // UTC of position fix
  char status;     // Status A=active or V=valid
  double lat;      // Latitude
  char dirlat;     // Direction of latitude: N: North S: South
  double lon;      // Longitude
  char dirlon;     // Direction of longitude: E: East W: West
  double speed;    // Speed over the ground in knots
  double angle;    // Track angle in degrees True
  double date;     // Date ddmmyy
  double magnet;   // Magnetic variation
  char dirmagnet;  // Direction of magnetic variation
  int16_t cksum;   // The checksum data, always begins with *
};

#define GSV_MAX_OF_SAT 16

struct apicmd_gpsgsv_repevt_s {
  int16_t numSV;  // Number of satellites in view
  struct apicmd_gpsgsv_sat_s sat[GSV_MAX_OF_SAT];
};

struct apicmd_event_s {
  uint8_t eventType;
  uint8_t nmeaType;
  union {
    struct apicmd_gpsgga_repevt_s gga;
    struct apicmd_gpsgsv_repevt_s gsv;
    struct apicmd_gpsrmc_repevt_s rmc;
    uint8_t allowedst;
    uint8_t sessionst;
    uint8_t fixst;
  } u;
};

#endif /* __MODULES_LTE_ALTCOM_INCLUDE_API_GPS_APICMD_GPS_NMEA_H */
