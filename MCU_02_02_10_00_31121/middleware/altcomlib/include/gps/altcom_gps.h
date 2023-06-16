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
#include "apicmd.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/**
 * @defgroup gpscont GPS Configuration Constants
 * @{
 */

#define ALTCOM_GPSACT_NOTOL (0)     /**< Zero tolerence */
#define NMEA_RAW_MAX_LEN 100        /**< Maximum length of GPS NMEA string */
#define URC_RAW_MAX_LEN	 256		/**< Maximum length of GPS URC string */
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

#define GPS_DELETE_EPHEMERIS (0x00010) /**< Bitmask to erase Ephemeris */
#define GPS_DELETE_ALMANAC (0x0002)    /**< Bitmask to erase Almanac */
#define GPS_DELETE_POSITION (0x0004)   /**< Bitmask to erase Position */
#define GPS_DELETE_TIME (0x0008)       /**< Bitmask to erase Time */
#define GPS_DELETE_TCXO (0x10000)      /**< Bitmask to erase TCXO */
#define GPS_DELETE_ALL (0xFFFFFFFF)    /**< Bitmask to erase all */

/** @} memerase */

/**
 * @defgroup maxsatellites Maximum of Satellites
 * Definition of maximum amount of satellites in view.
 * @{
 */
#define GSV_MAX_OF_SAT 16 /**< Maximum number of satellites in view */
#define GSV_NUM_GPS 32
#define GSV_NUM_GLN 24
#define BUF_LEN 12

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
  GPSACT_GPSOFF = 0,   /**< Deactivate GNSS */
  GPSACT_GPSON = 1,    /**< Activate GNSS */
  GPSACT_GPSON_TOL = 2 /**< Activate GNSS with tolerence*/
} gpsActFlag_e;
/** @} gpsactflag */

typedef enum {
	GPSROLLOVER_SET,
	GPSROLLOVER_GET
} roCmd_e;

typedef enum {
	GPSBLANKING_SET,
	GPSBLANKING_GET
} blankingCmd_e;

typedef enum {
	GPSTOW_SET,
	GPSTOW_GET
} towCmd_e;

typedef enum {
	GPSSATSCFG_SET,
	GPSSATSCFG_GET
} satsCmd_e;

/**
 * @defgroup cepCmd CEP Command
 * @{
 */

/**
 * @brief Definition of CEP command
 */
typedef enum {
  CEP_DLD_CMD = 1,    /**< Download operation */
  CEP_ERASE_CMD = 2,  /**< Erase operation */
  CEP_STATUS_CMD = 3, /**< Status checking operation */
  CEP_MAX_CMD         /**< Maximum value */
} cepCmd_e;

typedef enum {
  CEP_DAY_CUSTOM_URL = 0, /**< Download CEP file from customized URL */
  CEP_DAY_1 = 1,      /**< Download 1 day CEP */
  CEP_DAY_2 = 2,      /**< Download 2 days CEP */
  CEP_DAY_3 = 3,      /**< Download 3 days CEP */
  CEP_DAY_7 = 7,      /**< Download 7 days CEP */
  CEP_DAY_14 = 14,    /**< Download 14 days CEP */
  CEP_DAY_28 = 28,    /**< Download 28 days CEP */
  CEP_DAY_MAX         /**< Maximum value */
} cepDay_e;

typedef enum {
	INFO_SAT_CMD,
	INFO_EPH_CMD,
	INFO_FIX_CMD,
	INFO_TTFF_CMD
} infoCmd_e;

typedef enum {
	BLANKING_DISABLE,
	BLANKING_ENABLE
} blankingMode_e;

typedef enum {
	SKIP_TOW_VERIFICATION_OFF,
	SKIP_TOW_VERIFICATION_ON
} skipTowVerificationMode_e;

typedef enum {
	GNSS_SYSTEMS_NONE = 0,
	GNSS_SYSTEMS_GPS,
	GNSS_SYSTEMS_GLONASS,
	GNSS_SYSTEMS_BOTH
} satsConfigSystems_e;

/**
 * @brief Definition of CEP command
 */
typedef struct {
  uint8_t result;   /**< Result of CEP operation */
  uint32_t days;    /**< Days of CEP */
  uint32_t hours;   /**< Hours of CEP */
  uint32_t minutes; /**< Minutes of CEP */
} cepStatusResult_t;

/** @} cepCmd */

typedef struct {
	int32_t status;
	int32_t minsatage;
	int32_t numofsats;
	int32_t satlist[GSV_NUM_GPS + GSV_NUM_GLN];
} infoEphResult_t;

typedef struct {
	int32_t PRN;
	int32_t elevation;
	int32_t azimuth;
	int32_t SNR;
} infoSatStatus_t;

typedef struct {
	uint32_t numofsats;
	infoSatStatus_t satlist[GSV_MAX_OF_SAT];
} infoSatResult_t;

typedef struct {
	int32_t fixtype;
	char time[BUF_LEN];
	char date[BUF_LEN];
	double latitude;
	double longitude;
	double altitude;
	int64_t utc;
	float accuracy;
	float speed;
	char ephtype;
	bool accuracy_is_present;
	bool speed_is_present;
	int32_t num_of_sats_for_fix;
} infoFixResult_t;

typedef struct {
	int32_t mode;
	int32_t guardtime;
	int32_t activateupondisallow;
} blankingResult_t;

typedef struct {
	bool gps_is_config;
	bool glonass_is_config;
} satCfgResult_t;
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

/**
 * @brief Definition of GPS raw nmea message. It does not
 * include pre-defined structure - NMEA types GGA, GSV and RMC.
 * This is notified by event_report_cb_t callback function
 */
typedef struct {
  char nmea_msg[NMEA_RAW_MAX_LEN]; /**< Raw NMEA message */
} gps_nmearaw_t;

/** @} nmea */


/**
 * @brief Definition of GPS raw URC message.
 * This is notified by event_report_cb_t callback function
 */
typedef struct {
  char msg[URC_RAW_MAX_LEN]; /**< Raw URC message */
} gps_urcraw_t;

/** @} nmea */


/**
 * @defgroup gpsevent Event Information
 * @{
 */

/**
 * @brief Definition of event types
 */
typedef enum {
  EVENT_NMEA_TYPE = 0,  /**< NMEA event */
  EVENT_SESSIONST_TYPE, /**< Session status event */
  EVENT_ALLOWSTAT_TYPE, /**< Allowed status event */
  EVENT_COLDSTART_TYPE, /**< Cold-start event */
  EVENT_EPHUPD_TYPE,    /**< Ephemeris expiry event */
  EVENT_FIX_TYPE,       /**< GNSS fix event */
  EVENT_BLANKING_TYPE,  /**< Blanking toggled event */
  EVENT_TTFF_TYPE,      /**< TTFF event */
  EVENT_MAX_TYPE        /**< Maximum value */
} eventType_e;

/**
 * @brief Definition of NMEA event types
 */
typedef enum {
  EVENT_NMEA_GGA_TYPE = 1,  /**< GGA */
  EVENT_NMEA_GSV_TYPE = 2,  /**< GSV */
  EVENT_NMEA_RMC_TYPE = 3,  /**< RMC */
  EVENT_NMEA_RAW_TYPE = 255 /**< NMEA raw message, not be parsed */
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
    gps_nmearaw_t raw; /**< raw NMEA message */
    uint8_t sessionst; /**< Session status */
    uint8_t allowst; 	/**< Allowed status */
    uint8_t coldstart; /**< Cold-start status */
    uint8_t ephupd;    /**< Ephemeris expiry status */
    uint8_t fixst;     /**< GNSS fix status */
    uint8_t blanking;  /**< Blanking toggled status */
    uint8_t ttff;      /**< TTFF status */
  } u;                 /**< Union of various status */
} gps_event_t;

/**
 * @brief Since altcom_gps_setevent() registering the specific event, and the registered event will
 * notified by this function.
 *  @param[in] event : The event from map side which registered by @ref altcom_gps_setevent().
 *  @param[in] userPriv : Pointer to user's private data.
 */

typedef void (*event_report_cb_t)(gps_event_t *event, void *userPriv);

/** @} gpsevent */

typedef enum  {
	ACTIVATE_NO_MODE = 0,
	ACTIVATE_COLD_START = 1,
	ACTIVATE_HOT_START = 2
}gpsActMode_e;

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
 * @param [in] activation mode: Cold/Hot Start mode. See @red gpsActMode_e.
 * @param [in] tolerence: Tolerance delay in seconds(0-99999). Only validate when mode equals to
 * @ref GPSACT_GPSON_TOL.
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int altcom_gps_activate(gpsActFlag_e mode, gpsActMode_e activation_mode, unsigned int tolerence);

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
 * @brief Request GNSS CEP Download.
 *
 * @param [in] days: The duration in days. Also see @ref cepDay_e.
 * @param [in] timeout: Timeout of download request in seconds.
 * @param [out] result: The result of corresponding cmd.
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int altcom_gps_cep_download(cepDay_e days, unsigned int timeout, uint8_t *result);

/**
 * @brief Request GNSS CEP Erase.
 *
 * @param [out] result: The result of corresponding cmd.
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */
int altcom_gps_cep_erase(uint8_t *result);

/**
 * @brief Request GNSS CEP Status.
 *
 * @param [out] result: The result of corresponding cmd. Also see @ref cepStatusResult_t
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int altcom_gps_cep_status(cepStatusResult_t *result);

/**
 * @brief Request GNSS Ephemeris Status.
 *
 * @param [out] result: The result of corresponding cmd. Also see @ref infoEphResult_t
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int altcom_gps_info_get_ephemeris_status(infoEphResult_t *res);

/**
 * @brief Request GNSS Ttff Status.
 *
 * @param [out] result: The result of corresponding cmd
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int altcom_gps_info_get_ttff(int32_t *res);

/**
 * @brief Request GNSS Fix Status.
 *
 * @param [out] result: The result of corresponding cmd. Also see @ref infoFixResult_t
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int altcom_gps_info_get_fix(infoFixResult_t *res);

/**
 * @brief Request GNSS Satellite in view information.
 *
 * @param [out] result: The result of corresponding cmd. Also see @ref infoSatResult_t
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int altcom_gps_info_get_sat_in_view(infoSatResult_t *res);

/**
 * @brief Enalbe/Disable events and callbacks hookup. The event
 * registration behavior of this API effected by @ref altcom_init_t.cbreg_only.
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

/**
 * @brief Get Rollover value.
 *
 * @param [out] The result of corresponding cmd.
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 *
 */

int altcom_gps_getrollover(unsigned char *res);

/**
 * @brief Set new Rollover value.
 *
 * @param [in] The new rollover value to be set.
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 *
 */

int altcom_gps_setrollover(int val);

/**
 * @brief Set new Blanking configuration.
 *
 * @param [in] mode: The new blanking mode to be set.
 * @param [in] guardtime: The blanking guardtime in millisecond. Once guard timer expires, GNSS operation will be expired.
 * @param [in] activate_upon_disallow: When enabled, blanking will be activated once PHY wake up to prevent immediate GNSS shutdown.
 * @param [in] guard_time_enabled: When True, guard time will be modified.
 * @param [in] activate_upon_disallow+enabled: When True, active upon disallow will be modified.
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 *
 */

int altcom_gps_setblankingcfg(int mode, unsigned int guardtime, int activate_upon_disallow, bool guard_time_enabled, bool activate_upon_disallow_enabled);

/**
 * @brief Request Blanking configuration information.
 *
 * @param [out] result: The result of corresponding cmd. Also see @ref blankingResult_t
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int altcom_gps_getblankingcfg(blankingResult_t *res);

/**
 * @brief Set new TOW(time of week) algorithm configuration.
 *
 * @param [in] The new blanking mode to be set.
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 *
 */

int altcom_gps_set_time_of_week_algorithm(int val);

/**
 * @brief Get TOW(time of week) algorithm configuration.
 *
 * @param [out] The result of corresponding cmd.
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 *
 */

int altcom_gps_get_time_of_week_algorithm(int32_t *res);

/**
 * @brief Set new satellites systems in use configuration..
 *
 * @param [in] The new GNSS systems to be configure.
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 *
 */

int altcom_gps_set_satellites_cfg(satsConfigSystems_e systems_to_config);

/**
 * @brief Get satellites systems in use configuration.
 *
 * @param [out] The result of corresponding cmd.
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 *
 */

int altcom_gps_get_satellites_cfg(satsConfigSystems_e *res);

/** @} gps_funcs */

#undef EXTERN
#ifdef __cplusplus
}
#endif

/** @} gps */

#endif /* __MODULES_LTE_INCLUDE_GPS_ALTCOM_GPS_H */
