/****************************************************************************
 * modules/lte/include/gps/altcom_gps.h
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
/**
 * @file altcom_gps.h
 */

#ifndef __MODULES_LTE_INCLUDE_GPS_ALTCOM_GPS_H
#define __MODULES_LTE_INCLUDE_GPS_ALTCOM_GPS_H

/**
 * @defgroup gps GPS Connector APIs
 * @{
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdbool.h>
#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/**
 * @defgroup gpscont GPS Configuration Constants
 * @{
 */

#define ALTCOM_GPSACT_NOTOL (0)     /**< Zero tolerence */
#define MAX_GPS_VERSION_DATALEN 128 /**< Maximum length of GPS version string */

/**
 * @defgroup gpsnmeaformat GPS NMEA format
 * Definitions of nmea message flag used in altcom_gps_setnmeacfg.
 * @{
 */

#define ALTCOM_GPSCFG_PARAM_GGA (1 << 0) /**< Bitmask to enable GGA */
#define ALTCOM_GPSCFG_PARAM_GLL (1 << 1) /**< Bitmask to enable GLL */
#define ALTCOM_GPSCFG_PARAM_GSA (1 << 2) /**< Bitmask to enable GSA */
#define ALTCOM_GPSCFG_PARAM_GSV (1 << 3) /**< Bitmask to enable GSV */
#define ALTCOM_GPSCFG_PARAM_GNS (1 << 4) /**< Bitmask to enable GNS */
#define ALTCOM_GPSCFG_PARAM_RMC (1 << 5) /**< Bitmask to enable RMC */
#define ALTCOM_GPSCFG_PARAM_VTG (1 << 6) /**< Bitmask to enable VTG */
#define ALTCOM_GPSCFG_PARAM_ZDA (1 << 7) /**< Bitmask to enable ZDA */
#define ALTCOM_GPSCFG_PARAM_GST (1 << 8) /**< Bitmask to enable GST */

/** @} gpsnmeaformat */

/**
 * @defgroup memerase Memory Erase Command
 * Definition of memory erase command bitmap.
 * @{
 */

#define GPS_DELETE_EPHEMERIS (0x00010   /**< Bitmask to erase Ephemeris */
#define GPS_DELETE_ALMANAC (0x0002)     /**< Bitmask to erase Almanac */
#define GPS_DELETE_POSITION (0x0004)    /**< Bitmask to erase Position */
#define GPS_DELETE_TIME (0x0008)        /**< Bitmask to erase Time */
#define GPS_DELETE_IONO (0x0010)        /**< Bitmask to erase IONO */
#define GPS_DELETE_UTC (0x0020)         /**< Bitmask to erase UTC */
#define GPS_DELETE_HEALTH (0x0040)      /**< Bitmask to erase Health */
#define GPS_DELETE_SVDIR (0x0080)       /**< Bitmask to erase SVDIR */
#define GPS_DELETE_SVSTEER (0x0100)     /**< Bitmask to erase SVSTEER */
#define GPS_DELETE_SADATA (0x0200)      /**< Bitmask to erase SADATA */
#define GPS_DELETE_RTI (0x0400)         /**< Bitmask to erase RTI */
#define GPS_DELETE_CELLDB_INFO (0x8000) /**< Bitmask to erase cell database info */
#define GPS_DELETE_TCXO (0x10000)       /**< Bitmask to erase TCXO */
#define GPS_DELETE_ALL (0xFFFFFFFF)     /**< Bitmask to erase all */

/** @} memerase */

/**
 * @defgroup maxsatellites Maximum of Satellites
 * Definition of maximum amount of satellites in view.
 * @{
 */
#define GSV_MAX_OF_SAT 16 /**< Maximum number of satellites in view */

/** @} maxsatellites */

/** @} gpscont */

/****************************************************************************
 * Public Types
 ****************************************************************************/

/**
 * @defgroup gpsactflag GPS Activate Flag
 * @{
 */

/**
 * @brief Definitions of enable flag used in altcom_gps_activate.
 */

typedef enum {
  ALTCOM_GPSACT_GPSOFF = 0,   /**< Deactivate GNSS */
  ALTCOM_GPSACT_GPSON = 1,    /**< Activate GNSS */
  ALTCOM_GPSACT_GPSON_TOL = 2 /**< Activate GNSS with tolerence*/
} gpsActFlag_e;
/** @} gpsactflag */

/**
 * @defgroup cepCmd CEP Command
 * @{
 */

/**
 * @brief Definition of CEP command
 */
typedef enum {
  CEP_DLD_CMD = 1,   /**< Dowload operation */
  CEP_ERASE_CMD = 2, /**< Erase operation */
  CEP_STATUS_CMD = 3 /**< Status checking operation */
} cepCmd_e;

/**
 * @brief Definition of CEP command
 */
typedef struct {
  uint8_t result;   /**< Result of CEP operation */
  uint32_t days;    /**< Days of CEP */
  uint32_t hours;   /**< Hours of CEP */
  uint32_t minutes; /**< Minutes of CEP */
} cepResult_t;

/** @} cepCmd */

/**
 * @defgroup nmea NMEA Information
 * @{
 */

/**
 * @brief Definition of GPS GGA nmea message.
 * This is notified by event_report_cb_t callback function
 */
typedef struct {
  double UTC;         /**< UTC of position fix */
  double lat;         /**< Latitude */
  char dirlat;        /**< Direction of latitude: N: North S: South */
  double lon;         /**< Longitude */
  char dirlon;        /**< Direction of longitude: E: East W: West*/
  int16_t quality;    /**< GPS Quality indicator */
  int16_t numsv;      /**< Number of SVs in use */
  double hdop;        /**< HDOP */
  double ortho;       /**< Orthometric height (MSL reference) */
  char height;        /**< unit of measure for orthometric height is meters */
  double geoid;       /**< Geoid separation */
  char geosep;        /**< geoid separation measured in meters */
  double dgpsupdtime; /**< Age of differential GPS data record, Type 1 or Type 9. Null field when
                         DGPS is not used. */
  int16_t refstaid;   /**< Reference station ID */
  int16_t cksum;      /**< The checksum data, always begins with * */
} gps_nmeagga_t;

/**
 * @brief Definition of GSV satellite structure
 */
typedef struct {
  uint16_t PRN;       /**< Satellite ID */
  uint16_t elevation; /**< Elevation in degree */
  uint16_t azimuth;   /**< Azimuth in degree */
  uint16_t SNR;       /**< Signal to Noise Ration in dBHz */
} gps_gsvsat_t;

/**
 * @brief Definition of GPS GSV nmea message.
 * This is notified by event_report_cb_t callback function
 */
typedef struct {
  int16_t numSV;                    /**< Number of satellites in view */
  gps_gsvsat_t sat[GSV_MAX_OF_SAT]; /**< Satellites in view info */
} gps_nmeagsv_t;

/**
 * @brief Definition of GPS RMC nmea message.
 * This is notified by event_report_cb_t callback function
 */
typedef struct {
  double UTC;     /**< UTC of position fix */
  char status;    /**< Status A=active or V=valid */
  double lat;     /**< Latitude */
  char dirlat;    /**< Direction of latitude: N: North S: South */
  double lon;     /**< Longitude */
  char dirlon;    /**< Direction of longitude: E: East W: West */
  double speed;   /**< Speed over the ground in knots */
  double angle;   /**< Track angle in degrees True */
  double date;    /**< Date ddmmyy */
  double magnet;  /**< Magnetic variation */
  char dirmagnet; /**< Direction of magnetic variation */
  int16_t cksum;  /**< The checksum data, always begins with * */
} gps_nmearmc_t;

/** @} nmea */

/**
 * @defgroup gpsevent Event Information
 * @{
 */

/**
 * @brief Definition of event types
 */
typedef enum {
  EVENT_NMEA_TYPE = 0,      /**< NMEA event */
  EVENT_SESSIONST_TYPE = 1, /**< Session status event */
  EVENT_ALLOWEDST_TYPE = 2, /**< Allowed status event */
  EVENT_FIX_TYPE = 3        /**< GNSS fix event */
} eventType_e;

/**
 * @brief Definition of NMEA event types
 */
typedef enum {
  EVENT_NMEA_GGA_TYPE = 1, /**< GGA */
  EVENT_NMEA_GSV_TYPE = 2, /**< GSV */
  EVENT_NMEA_RMC_TYPE = 3  /**< RMC */
} nmeaType_e;

/**
 * @brief Definition of event structure
 */
typedef struct {
  eventType_e eventType; /**< Event type */
  nmeaType_e nmeaType;   /**< NMEA type */
  union {
    gps_nmeagga_t gga; /**< GGA data */
    gps_nmeagsv_t gsv; /**< GSV data */
    gps_nmearmc_t rmc; /**< RMC data */
    uint8_t sessionst; /**< Session status */
    uint8_t allowedst; /**< Allowed status */
    uint8_t fixst;     /**< GNSS fix status */
  } u;                 /**< Union of gga/allow/session status */
} gps_event_t;

/**
 * @brief Since altcom_gps_setevent() registering the specific event, and the registered event will
 * notified by this function.
 *  @param[in] event : The event from map side which registered by @ref altcom_gps_setevent().
 *  @param[in] userPriv : Pointer to user's private data.
 */

typedef void (*event_report_cb_t)(gps_event_t *event, void *userPriv);

/** @} gpsevent */

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif
/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * @defgroup gps_funcs GPS APIs
 * @{
 */

/**
 * @brief Activate GNSS hardware functionality.
 *
 * @param [in] mode: Activation/Deactivation mode. See @ref gpsActFlag_e.
 * @param [in] tolerence: Tolerance delay in seconds(0-99999). Only validate when mode equals to
 * @ref ALTCOM_GPSACT_GPSON_TOL.
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int altcom_gps_activate(gpsActFlag_e mode, int tolerence);

/**
 * @brief Set GNSS run-time mode configuration.
 *
 * @param [in] params: Enable specified NMEA message. See @ref gpsnmeaformat.
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int altcom_gps_setnmeacfg(int params);

/**
 * @brief Request GNSS SW version.
 *
 * @param [in] version: The version string(up to 128 Bytes).
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int altcom_gps_ver(char *version);

/**
 * @brief Request GNSS CEP.
 *
 * @param [in] cmd: 1:DLD 2:ERASE 3:STATUS.
 * @param [in] days: The duration in days.
 * @param [out] result: The result of corresponding cmd.
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int altcom_gps_cep(int cmd, int days, cepResult_t *result);

/**
 * @brief Enalbe/Disable events and callbacks hookup. The event
 * registration behavior of this API effected by @ref altcom_init_t.is_cbreg_only_until_appready.
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

int altcom_gps_setevent(eventType_e event, bool enable, event_report_cb_t callback, void *userPriv);

/**
 * @brief Erase assistance memory.
 *
 * @param [in] bitmap erase command bit mapping, see @ref memerase.
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 *
 */

int altcom_gps_memerase(int bitmap);

/** @} gps_funcs */

#undef EXTERN
#ifdef __cplusplus
}
#endif

/** @} gps */

#endif /* __MODULES_LTE_INCLUDE_GPS_ALTCOM_GPS_H */
