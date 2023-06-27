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

/**
 * @file lte_api.h
 */

#ifndef __MODULES_INCLUDE_LTE_LTE_API_H
#define __MODULES_INCLUDE_LTE_LTE_API_H

/**
 * @defgroup lte LTE Connector APIs
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
 * @defgroup ltesessionid Session ID
 * @{
 */

/**
 * @brief
 * Definitions of session ID value range.
 * These are used in lte_data_on(), lte_data_off() lte_set_apn().
 */

#define LTE_SESSION_ID_MIN (1) /**< Minimum value of session ID */
#define LTE_SESSION_ID_MAX (5) /**< Maximum value of session ID */

/** @} ltesessionid */

/**
 * @defgroup lteapnlen Length of APN
 * @{
 */

/**
 * @brief
 * Definition of maximum string length of the APN name
 */

#define LTE_APN_LEN 101 /**< The maximum string length of the APN name */

/** @} lteapnlen */

/**
 * @defgroup lteapnusrnamelen Length of User Name
 * @{
 */

/**
 * @brief
 * Definition of maximum string length of the APN user name
 */

#define LTE_APN_USER_NAME_LEN          \
  64 /**< The maximum string length of \
          the APN user name */

/** @} lteapnusrnamelen */

/**
 * @defgroup lteapnpasswdlen Length of Password
 * @{
 */

/**
 * @brief
 * Definition of maximum string length of the APN password
 */

#define LTE_APN_PASSWD_LEN             \
  32 /**< The maximum string length of \
          the APN password */

/** @} lteapnpasswdlen */

/**
 * @defgroup lteccidlen Length of CCID
 * @{
 */

/**
 * @brief
 * Definition of maximum string length of the CCID
 */

#define LTE_CCID_LEN                   \
  21 /**< The maximum string length of \
          the APN password */

/** @} lteccidlen */

/**
 * @defgroup ltetimingadvna No Value From Timing Advance
 * @{
 */

/**
 * @brief
 * Definition of the number represents no value from timing advance
 */

#define LTE_TIMING_ADV_NA 65535

/** @} ltetimingadvna */

/**
 * @defgroup lteverlen Length of Version Information
 * @{
 */

/**
 * @brief
 * Definitions of length of character string for version
 */

#define LTE_VER_BB_PRODUCT_LEN            \
  (5) /**< Length of character string for \
           BB product */
#define LTE_VER_NP_PACKAGE_LEN             \
  (32) /**< Length of character string for \
            NP package */

/** @} lteverlen */

/**
 * @defgroup ltemaxphonenumlen Maximum Length of Phone Number Information
 * @{
 */

/**
 * @brief
 * Definitions of maximum length of character string for phone number
 */

#define LTE_MAX_PHONENUM_LEN (41) /**< Maximum length of character string for phone number */

/** @} ltemaxphonenumlen */

/**
 * @defgroup ltemaximsilen Maximum Length of IMSI Information
 * @{
 */

/**
 * @brief
 * Definitions of maximum length of character string for IMSI
 */

#define LTE_MAX_IMSI_LEN (16) /**< Maximum length of character string for IMSI */

/** @} ltemaximsilen */

/**
 * @defgroup ltemaximeilen Maximum Length of IMEI Information
 * @{
 */

/**
 * @brief
 * Definitions of maximum length of character string for IMEI
 */

#define LTE_MAX_IMEI_LEN (16) /**< Maximum length of character string for IMEI */

/** @} ltemaximeilen */

/**
 * @defgroup ltemaxpincodelen Maximum Length of PINCODE Information
 * @{
 */

/**
 * @brief
 * Definitions of maximum/minimum length of character string for PINCODE
 */

#define LTE_MIN_PINCODE_LEN (4) /**< Minimum length of character string for pin code */
#define LTE_MAX_PINCODE_LEN (8) /**< Maximum length of character string for pin code */

/** @} ltemaxpincodelen */

/**
 * @defgroup ltemaxoperatorlen Maximum Length of Operator Name
 * @{
 */

/**
 * @brief
 * Definitions of maximum length of character string for operator name
 */

#define LTE_MAX_OPERATOR_NAME_LEN                                \
  (33) /**< Maximum length of character string for operator name \
        */

/** @} ltemaxoperatorlen */

/**
 * @defgroup lteidentifier Various Identifier Length
 * @{
 */

/**
 * @brief
 * Definitions of MCC/MNC digits. Used in lte_cellinfo_t.
 */

#define LTE_CELLINFO_MCC_DIGIT (3)     /**< Digit number of MCC */
#define LTE_CELLINFO_MNC_DIGIT_MAX (3) /**< Max digit number of MNC */

/**
 * @brief
 * Definitions of Cell Global ID digits. Used in lte_cellinfo_t.
 */

#define LTE_CELLINFO_CGID_DIGIT_MAX (16) /**< Digit number of Cell Global ID */

/** @} lteidentifier */

/**
 * @defgroup lteneighborcell Maximum PSM Camping Attempt Timeout
 * @{
 */

/**
 * @brief
 * Definitions of maximum neighbor cell information.
 */

#define LTE_MAX_NEIGHBOR_CELL_INFO (32) /**< Max neighbor cell information */

/** @} lteneighborcell */

/**
 * @defgroup ltepsmcamptimeout Maximum PSM Camping Attempt Timeout
 * @{
 */

/**
 * @brief
 * Definitions of maximum PSM camping attempt timeout. Used in lte_psm_camp().
 */

#define LTE_MAX_PSMCAMP_ATTAMPT_TIMEOUT (12000) /**< PSM camping attempt timeout */

/** @} ltepsmcamptimeout */

/****************************************************************************
 * Public Types
 ****************************************************************************/

/**
 * @defgroup lteresult Result Code
 * @{
 */

/**
 * @brief
 * Enumerations of result code. These are notified by asynchronous
 * API callback such as attach_net_cb_t
 */

typedef enum {
  LTE_RESULT_OK = 0,     /**< Result code on success */
  LTE_RESULT_ERROR = 1,  /**< Result code on failure */
  LTE_RESULT_CANCEL = 2, /**< Result code on cancel */
} lteresult_e;

/** @} lteresult */

/**
 * @defgroup lteerrcause Error Cause
 * @{
 */

/**
 * @brief
 * Enumerations of error cause. These are notified by asynchronous
 * API callback such as attach_net_cb_t
 */

typedef enum {
  LTE_ERR_NOERR = 0,        /**< No error */
  LTE_ERR_WAITENTERPIN = 1, /**< Waiting for PIN enter */
  LTE_ERR_REJECT = 2,       /**< Rejected from the network */
  LTE_ERR_MAXRETRY = 3,     /**< No response from the network */
  LTE_ERR_BARRING = 4,      /**< Network barring */
  LTE_ERR_DETACHED = 5,     /**< Network detached */
  LTE_ERR_UNEXPECTED = 255  /**< Unexpected cause */
} lteerrcause_e;

/** @} lteerrcause */

/**
 * @defgroup lteradiomode Radio Mode
 * @{
 */

/**
 * @brief
 * Definitions of radio mode used in lte_detach_network()
 */

typedef enum {
  LTE_NONOPERATIONAL_MODE = 0, /**< Non-operational mode */
  LTE_FLIGHT_MODE,             /**< Flight mode */
  LTE_OPERATIONAL_MODE,        /**< Operational mode */
  LTE_MODE_MAX                 /**< Enumeration MAX */
} lteradiomode_e;

/** @} lteradiomode */

/**
 * @defgroup ltevalidflag Validness Flag
 * @{
 */

/**
 * @brief
 * Definitions of Validness flag used in lte_quality_t and lte_cellinfo_t
 */

typedef enum {
  LTE_VALID = true,               /**< Valid */
  LTE_TIMEDIFFIDX_VALID = 1 << 1, /**< Timdiff index is valid */
  LTE_TA_VALID = 1 << 2,          /**< Time Advance is valid */
  LTE_SFN_VALID = 1 << 3,         /**< SFN is valid */
  LTE_RSRP_VALID = 1 << 4,        /**< RSRP is valid */
  LTE_RSRQ_VALID = 1 << 5,        /**< RSRQ is valid */
  LTE_INVALID = false             /**< Invalid */
} ltevalidflag_t;

/** @} ltevalidflag */

/**
 * @defgroup lterat LTE RAT param
 * @{
 */

/**
 * @brief
 * Definitions of Validness flag used in lte_quality_t and lte_cellinfo_t
 */

typedef enum {
  LTE_RAT_DEFAULT = 1, /**< activate default RAT/configured mode */
  LTE_RAT_CATM = 2,    /**< activate CAT-M RAT */
  LTE_RAT_NBIOT = 3,   /**< activate NB-IOT RAT */
  LTE_RAT_GSM = 4,     /**< activate GSM RAT in single RAT mode */
  LTE_RAT_C2D = 5,     /**< activate CAT-M RAT in single mode with fallback to default auto mode and
                          previous RAT */
  LTE_RAT_N2D = 6, /**< activate NB-IOT RAT in single mode with fallback to default auto mode and
                      previous RAT */
  LTE_RAT_G2D =
      7, /**< activate GSM RAT in single mode with fallback to default auto mode and previous RAT */
} rat_t;

typedef enum {
  LTE_RAT_MODE_SINGLE = 0,   /**< Single RAT  */
  LTE_RAT_MODE_MULTIPLE = 1, /**< Multiple RAT  */
} rat_mode_t;

typedef enum {
  LTE_RAT_NONE = 0,  /**< RAT switch requests arbitration: none */
  LTE_RAT_HOST = 1,  /**< RAT switch requests arbitration: by host*/
  LTE_RAT_LWM2M = 2, /**< RAT switch requests arbitration: by LWM2M*/
} rat_source_t;

typedef enum {
  LTE_RAT_NOT_PERSISTENT = 0, /**< Settings is not presistent*/
  LTE_RAT_PERSISTENT = 1,     /**< Settings is persistent*/
} rat_persistency_t;

/** @} lterat */

/**
 * @defgroup fwupgrade FW upgrade command & state
 * @{
 */

/**
 * @brief
 * Definitions of the FW upgrade commands
 */

typedef enum {
  LTE_FW_UPGRADE_CMD_DLRSP = 1,     /**< Host response for pending Download event. */
  LTE_FW_UPGRADE_CMD_UPRSP = 2,     /**< Host response for pending Update event. */
  LTE_FW_UPGRADE_CMD_HOSTUPRES = 3, /**< Host result reporting for Host Update. */
  LTE_FW_UPGRADE_CMD_DLSUS = 4,     /**< Download Suspend. FOTA Manager will ask LWM2M to suspend
                                       download (applicable for HTTP/S PULL method only). */
  LTE_FW_UPGRADE_CMD_DLRES =
      5, /**< Download Resume. Resume download that previously suspended by "DLSUS" */
  LTE_FW_UPGRADE_CMD_CANCELFOTA = 6 /**< Cancel FOTA  */
} fw_upgrade_cmd_t;

typedef enum {
  LTE_FW_UPGRADE_CMD_CONFIRM_ACCEPT = 0, /**< accept */
  LTE_FW_UPGRADE_CMD_CONFIRM_CANCEL = 1, /**< cancel */
} fw_upgrade_cmd_confirm_t;

typedef enum {
  LTE_FW_UPGRADE_RES_INITIAL_VALUE = 0, /** < Initial value */
  LTE_FW_UPGRADE_RES_SUCCESS = 1,       /** < Success */
  LTE_FW_UPGRADE_RES_NOT_ENOUGH_FLASH_MEMORY_DURING_DOWNLOAD =
      2, /** < Not enough flash memory during download */
  LTE_FW_UPGRADE_RES_OUT_OF_RAM_DURING_DOWNLOAD = 3,      /** < Out of RAM during download */
  LTE_FW_UPGRADE_RES_CONNECTION_LOST_DURING_DOWNLOAD = 4, /** < Connection lost during download */
  LTE_FW_UPGRADE_RES_INTEGRITY_CHECK_FAILURE = 5,         /** < Integrity check failure */
  LTE_FW_UPGRADE_RES_UNSUPPORTED_PACKAGE_TYPE = 6,        /** < Unsupported package type */
  LTE_FW_UPGRADE_RES_INVALID_URI = 7,                     /** < Invalid URI */
  LTE_FW_UPGRADE_RES_IMAGE_UPDATE_FAILED = 8,             /** < Image update failed */
  LTE_FW_UPGRADE_RES_UNSUPPORTED_PROTOCOL = 9             /** < Unsupported protocol */
} fw_upgrade_result_t;

typedef enum {
  LTE_FW_UPGRADE_STATE_IDLE = 0, /** < Idle */
  LTE_FW_UPGRADE_STATE_WAITING_FOR_DOWNLOAD_CONFIRMATION =
      1,                                          /** < Waiting for download confirmation */
  LTE_FW_UPGRADE_STATE_DOWNLOAD_PRECONDITION = 2, /** < Download precondition */
  LTE_FW_UPGRADE_STATE_DURING_DOWNLOAD = 3,       /** < During download */
  LTE_FW_UPGRADE_STATE_DOWNLOAD_COMPLETED = 4,    /** < Download completed */
  LTE_FW_UPGRADE_STATE_WAITING_FOR_UPDATE_CONFIRMATION = 5, /** < Waiting for update confirmation */
  LTE_FW_UPGRADE_STATE_UPDATE_PRECONDITION = 6,             /** < Update precondition */
  LTE_FW_UPGRADE_STATE_WAITING_FOR_REBOOT = 7,              /** < Waiting for reboot */
  LTE_FW_UPGRADE_STATE_WAITING_FOR_HOST_RESULT = 8,         /** < Waiting for host result */
  LTE_FW_UPGRADE_STATE_FOTA_FINISHED = 9,                   /** < FOTA finished */
  LTE_FW_UPGRADE_STATE_DOWNLOAD_SUSPENDED = 10              /** < Download suspended */

} fw_upgrade_state_t;

/** @} fwupgrade */

/**
 * @defgroup lteinterceptscn LTE Intercepted Scan param
 * @{
 */

/**
 * @brief
 * Definitions of the mode of intercepted scan
 */

typedef enum {
  LTE_INTERCEPTED_SCAN_DISABLE = 0, /**< To disable intercepted scan */
  LTE_INTERCEPTED_SCAN_ENABLE = 1,  /**< To enable intercepted scan */
} lteintercptscanmode_t;

/** @} lteinterceptscn */

/**
 * @defgroup lteenableflag_e Enable Flag
 * @{
 */

/**
 * @brief
 * Enumerations of enable flag
 */
typedef enum {
  LTE_DISABLE = false, /**< Disable */
  LTE_ENABLE = true    /**< Enable */
} lteenableflag_e;

/** @} lteenableflag_e */

/**
 * @defgroup ltecfunoptn_e cfun options
 * @{
 */

/**
 * @brief
 * Enumerations of cfun options
 */
typedef enum {
  LTE_CFUN_0 = 0,   /**< cfun=0 */
  LTE_CFUN_1 = 1,   /**< cfun=1 */
  LTE_CFUN_4 = 4,   /**< cfun=4 */
  LTE_CFUN_10 = 10, /**< cfun=10 */
} ltecfunoptn_e;

/** @} ltecfunoptn_e */

/**
 * @defgroup lteapn APN Setting
 * @{
 */

/**
 * @brief
 * Definition of APN settings data.
 */

typedef struct {
  uint8_t session_id;      /**< Numeric value of the session ID */
  int8_t apn[LTE_APN_LEN]; /**< Access Point Name. It is terminated with '\0' */
  uint8_t ip_type;         /**< Internet Protocol type. See @ref lteapniptype */
} lte_apndata_t;

/**
 * @brief
 * Definition of APN settings list.
 */

typedef struct {
  int8_t listnum;                            /**< Number of APN data list. */
  lte_apndata_t apnlist[LTE_SESSION_ID_MAX]; /**< List of APN data. See @ref lte_apndata_t */
} lte_apnlist_t;

/** @} lteapn */

/**
 * @defgroup settimerevt Set timer event
 * @{
 */

/**
 * @brief
 * Definition of set timer event type.
 */

typedef enum {
  LTE_EVTYPE_3412 = 1, /**< Select T3412 */
  LTE_EVTYPE_3402 = 2, /**< Select T3402 */
} lte_timerevt_evttype_t;

/**
 * @brief
 * Definition of set timer event mode.
 */
typedef enum {
  LTE_MODE_DISABLE = 0, /**< Disable timer */
  LTE_MODE_ENABLE = 1,  /**< Enable timer */
} lte_timerevt_mode_t;

/**
 * @brief
 * Definition of IUICC status.
 */
typedef enum {
  LTE_IUICC_DISABLED = 0, /**< IUICC disabled */
  LTE_IUICC_ENABLED = 1,  /**< IUICC enabled */
} lte_iuicc_status_t;

/**
 * @brief
 * Definition of set timer event selector.
 */
typedef enum {
  LTE_EVSEL_STOP = 1,    /**< Timer stopped event */
  LTE_EVSEL_START = 2,   /**< Timer started event */
  LTE_EVSEL_EXPIRED = 4, /**< Timer expired event */
} lte_timerevt_evsel_t;
/**
 * @brief
 * Definition of timer set event.
 */

typedef struct {
  lte_timerevt_evttype_t ev_type;
  lte_timerevt_mode_t mode;
  lte_timerevt_evsel_t ev_sel;
  uint32_t preceding_time;
} lte_settimerevt_t;

/** @} settimerevt */

/**
 * @defgroup ltedatastate Data Connection State
 * @{
 */

/**
 * @brief
 * Enumerations of PDN data activation state
 */
typedef enum {
  LTE_NONACTIVE = false, /**< Non-Active */
  LTE_ACTIVE = true      /**< Active */
} ltepdnstate_e;

/**
 * @brief
 * Definition of data connection state.
 */

typedef struct {
  uint8_t session_id;  /**< Numeric value of the session ID */
  ltepdnstate_e state; /**< The state of data communication. See @ref ltepdnstate_e*/
} lte_datastatedata_t;

/**
 * @brief
 * Definition of data connection state list.
 */

typedef struct {
  uint8_t listnum;                                   /**< Number of APN data list. */
  lte_datastatedata_t statelist[LTE_SESSION_ID_MAX]; /**< List of data communication state. See @ref
                                                        lte_datastatedata_t */
} lte_datastatelist_t;

/** @} ltedatastate */

/**
 * @defgroup ltenetstate Network State
 * @{
 */

/**
 * @brief
 * Enumerations of network state. These are notified by netstate_report_cb_t
 */

typedef enum {
  LTE_NETSTAT_ATTACH = 0,     /**< LTE attached */
  LTE_NETSTAT_DETACH = 1,     /**< LTE detached */
  LTE_NETSTAT_CONNECT = 2,    /**< Data connected */
  LTE_NETSTAT_DISCONNECT = 3, /**< Data disconnected */
  LTE_NETSTAT_CHANGE_APN = 4  /**< Connected APN has changed */
} ltenetstate_e;

/** @} ltenetstate */

/**
 * @defgroup ltedatatype Data Type
 * @{
 */

/**
 * @brief
 * Definitions of Data type used in lte_set_dataconfig().
 */
typedef enum {
  LTE_DATA_TYPE_USER = 0, /**< Data type: user data */
  LTE_DATA_TYPE_IMS = 1   /**< Data type: IMS */
} ltedatatype_e;

/** @} ltedatatype */

/**
 * @defgroup lteapniptype APN IP Type
 * @{
 */

/**
 * @brief
 * Enumerations of internet protocol type
 */

typedef enum {
  LTE_APN_IPTYPE_IP = 0,     /**< Internet protocol type IP */
  LTE_APN_IPTYPE_IPV6 = 1,   /**< Internet protocol type IPv6 */
  LTE_APN_IPTYPE_IPV4V6 = 2, /**< Internet protocol type IPv4/v6 */
} lteapniptype_e;

/** @} lteapniptype */

/**
 * @defgroup lteapnauthtype APN PPP Authentication Type
 * @{
 */

/**
 * @brief
 * Enumerations of PPP authentication type
 */

typedef enum {
  LTE_APN_AUTHTYPE_NONE = 0, /**< PPP authentication type NONE */
  LTE_APN_AUTHTYPE_PAP = 1,  /**< PPP authentication type PAP */
  LTE_APN_AUTHTYPE_CHAP = 2  /**< PPP authentication type CHAP */
} lteapnauthtype_e;

/** @} lteapnauthtype */

/**
 * @defgroup ltever Version Information
 * @{
 */

/**
 * @brief
 * Definition of version information of the modem.
 */

typedef struct {
  int8_t bb_product[LTE_VER_BB_PRODUCT_LEN]; /**< BB product version. It is
                                                  terminated with '\0' */
  int8_t np_package[LTE_VER_NP_PACKAGE_LEN]; /**< NP package version. It is
                                                  terminated with '\0' */
} lte_version_t;

/** @} ltever */

/**
 * @defgroup ltepincode PIN Setting Information
 * @{
 */

/**
 * @brief
 * Definitions of target PIN. These are used in lte_change_pin()
 */

typedef enum {
  LTE_TARGET_PIN = 0, /**< Select of PIN change */
  LTE_TARGET_PIN2 = 1 /**< Select of PIN2 change */
} ltetargetpin_e;

/**
 * @brief
 * Enumerations of pin enable. Used in lte_set_pinenable()
 */

typedef enum {
  LTE_PIN_DISABLE = false, /**< Disable setting of PIN lock */
  LTE_PIN_ENABLE = true    /**< Enable setting of PIN lock */
} ltepinenable_e;

/**
 * @brief
 * Enumerations of PIN status. These are used in lte_pin_attributes_t
 */

typedef enum {
  LTE_PINSTAT_READY = 0,       /**< Not pending for any password */
  LTE_PINSTAT_SIM_PIN = 1,     /**< Waiting SIM PIN to be given */
  LTE_PINSTAT_SIM_PUK = 2,     /**< Waiting SIM PUK to be given */
  LTE_PINSTAT_PH_SIM_PIN = 3,  /**< Waiting phone to SIM card password to be given */
  LTE_PINSTAT_PH_FSIM_PIN = 4, /**< Waiting phone-to-very first SIM card password to be given */
  LTE_PINSTAT_PH_FSIM_PUK =
      5, /**< Waiting phone-to-very first SIM card unblocking password to be given */
  LTE_PINSTAT_SIM_PIN2 = 6,   /**< Waiting SIM PIN2 to be given */
  LTE_PINSTAT_SIM_PUK2 = 7,   /**< Waiting SIM PUK2 to be given */
  LTE_PINSTAT_PH_NET_PIN = 8, /**< Waiting network personalization password to be given */
  LTE_PINSTAT_PH_NET_PUK =
      9, /**< Waiting network personalization unblocking password to be given */
  LTE_PINSTAT_PH_NETSUB_PIN =
      10, /**< Waiting network subset personalization password to be given */
  LTE_PINSTAT_PH_NETSUB_PUK =
      11, /**< Waiting network subset personalization unblocking password to be given */
  LTE_PINSTAT_PH_SP_PIN = 12, /**< Waiting service provider personalization password to be given */
  LTE_PINSTAT_PH_SP_PUK =
      13, /**< Waiting service provider personalization unblocking password to be given */
  LTE_PINSTAT_PH_CORP_PIN = 14, /**< Waiting corporate personalization password to be given */
  LTE_PINSTAT_PH_CORP_PUK =
      15, /**< Waiting corporate personalization unblocking password to be given */
} ltepinstate_e;

/**
 * @brief
 * Definition of PIN setting information.
 */

typedef struct {
  ltepinenable_e enable;     /**< PIN enable. See @ref ltepinenable_e */
  ltepinstate_e status;      /**< PIN status. Refer to the this parameter only
                            when @ref enable is LTE_PIN_DISABLE.
                            See @ref ltepinstate_e */
  uint8_t pin_attemptsleft;  /**< PIN attempts left */
  uint8_t puk_attemptsleft;  /**< PUK attempts left */
  uint8_t pin2_attemptsleft; /**< PIN2 attempts left */
  uint8_t puk2_attemptsleft; /**< PUK2 attempts left */
} lte_pin_attributes_t;

/** @} ltepincode */

/**
 * @defgroup lteslpmode Sleep Mode
 * @{
 */

/**
 * @brief
 * Enumerations of sleep mode used in lte_set_sleepmode().
 */
typedef enum {
  LTE_SLPMODE_DISABLED = 0,   /**< Sleep mode: disabled */
  LTE_SLPMODE_NAP = 1,        /**< Sleep mode: nap */
  LTE_SLPMODE_LIGHTSLEEP = 2, /**< Sleep mode: light sleep */
  LTE_SLPMODE_DEEPSLEEP = 3,  /**< Sleep mode: deep sleep */
  LTE_SLPMODE_DEEPHIBER0 = 4, /**< Sleep mode: deep hibernation 0 */
  LTE_SLPMODE_DEEPHIBER1 = 5, /**< Sleep mode: deep hibernation 1 */
  LTE_SLPMODE_DEEPHIBER2 = 6  /**< Sleep mode: deep hibernation 2 */
} lteslpmode_e;

/** @} lteslpmode */

/**
 * @defgroup ltelocaltime Local Time
 * @{
 */

/**
 * @brief
 * Definition of local time. This is also notified by localtime_report_cb_t
 */

typedef struct {
  int32_t year;   /**< Years (0-99) */
  int32_t mon;    /**< Month (1-12) */
  int32_t mday;   /**< Day of the month (1-31) */
  int32_t hour;   /**< Hours (0-23) */
  int32_t min;    /**< Minutes (0-59) */
  int32_t sec;    /**< Seconds (0-59) */
  int32_t tz_sec; /**< Time zone in seconds (-86400-86400) */
} lte_localtime_t;

/** @} ltelocaltime */

/**
 * @defgroup ltegetccid Get CCID
 * @{
 */

/**
 * @brief
 * Definition of get_ccid.
 */

typedef struct {
  uint8_t ccid[LTE_CCID_LEN]; /**< CCID string */
} lte_ccid_t;

/** @} ltegetccid */

/**
 * @defgroup ltegettimingadvance Get Timing Advance
 * @{
 */

/**
 * @brief
 * Definition of get_timing_advance.
 */

typedef struct {
  uint16_t timing_advance; /**< Valid value from 0 to 1282. LTE_TIMING_ADV_NA represents failure.
                              See @ref ltetimingadvna */
} lte_timging_advance_t;

/** @} ltegettimingadvance */

/**
 * @defgroup ltegetiuiccstatus Get IUICC Status
 * @{
 */

/**
 * @brief
 * Definition of get_iuicc_status.
 */

typedef struct {
  lte_iuicc_status_t iuicc_status; /**< IUICC enablement. Refer to @ref lte_iuicc_status_t  */
} lte_get_iuicc_status_t;

/** @} ltegetiuiccstatus */

/**
 * @defgroup ltegetrat Get RAT
 * @{
 */

/**
 * @brief
 * Definition of get_rat.
 */

typedef struct {
  rat_t rat;           /**< LTE rat type. Refer to @ref rat_t*/
  rat_mode_t rat_mode; /**< RAT mode of RM state machine. Refer to @ref rat_mode_t */
  rat_source_t source; /**< RAT switch requests arbitration source. Refer to @ref rat_source_t*/
} lte_get_rat_t;

/** @} ltegetrat */

/**
 * @defgroup ltestintercptscn Set Intercepted Scan
 * @{
 */

/**
 * @brief
 * Definition of set intercepted scan.
 */

typedef struct {
  lteintercptscanmode_t
      mode;             /**< intercepted scan enablement. Refer to @ref lteintercptscanmode_t */
  uint32_t timer_value; /**<  timer value in ms*/
} lte_set_interruptedscan_t;

/** @} ltestintercptscn */

/**
 * @defgroup ltesetrat Set RAT
 * @{
 */

/**
 * @brief
 * Definition of set_rat.
 */

typedef struct {
  rat_t rat;                /**< RAT to be activated. Refer to @ref rat_t */
  rat_persistency_t persis; /**< indicates if settings are persistent over a power-cycle. Refer to
                               @ref rat_persistency_t */
} lte_set_rat_t;

/** @} ltesetrat */

/**
 * @defgroup ltefwupgradecmd FW upgrade command
 * @{
 */

/**
 * @brief
 * Definition of fw_upgrade_cmd.
 */

typedef struct {
  fw_upgrade_cmd_t cmd;
  fw_upgrade_cmd_confirm_t confirmation;
  fw_upgrade_result_t result;
} lte_fw_upgrade_cmd_t;

/** @} ltefwupgradestate */

/**
 * @defgroup ltefwupgradestate FW upgrade state
 * @{
 */

/**
 * @brief
 * Definition of fw_upgrade_state.
 */

typedef struct {
  fw_upgrade_state_t state;
  uint32_t image_downloaded_size; /**< currently downloaded size of image in bytes for download in
                                     progress  */
  uint32_t image_total_size;      /**< total size of image to download */
  fw_upgrade_result_t fota_result;
} lte_fw_upgrade_state_t;

/** @} ltefwupgradestate */

/**
 * @defgroup ltesimstate SIM State
 * @{
 */

/**
 * @brief
 * Enumerations of SIM state. These are notified by simstate_report_cb_t
 */

typedef enum {
  LTE_SIMSTAT_REMOVAL = 0,         /**< SIM removal signal detected */
  LTE_SIMSTAT_INSERTION = 1,       /**< SIM insertion signal detected */
  LTE_SIMSTAT_DEACTIVATED = 2,     /**< SIM deactivated */
  LTE_SIMSTAT_WAIT_PIN_UNLOCK = 3, /**< SIM init passed, wait for PIN unlock */
  LTE_SIMSTAT_PERSONAL_FAILED =
      4, /**< Personalization failed, wait for run-time depersonalization */
  LTE_SIMSTAT_ACTIVATE =
      5 /**< Activation completed. Event is sent always at any SIM activation completion */
} ltesimstate_e;

/** @} ltesimstate */

/**
 * @defgroup ltesigquility Signal Quility Information
 * @{
 */

/**
 * @brief
 * Definition of parameters for quality information.
 * This is reported by quality_report_cb_t
 */

typedef struct lte_quality {
  ltevalidflag_t valid; /**< Refer to the following parameters only when this flag
                   is valid. This is because valid parameters can not be
                   acquired when RF function is OFF and so on */
  int16_t rsrp;         /**< RSRP in dBm (-140-0) */
  int16_t rsrq;         /**< RSRQ in dBm (-60-0) */
  int16_t sinr;         /**< SINR in dBm (-128-40) */
  int16_t rssi;         /**< RSSI in dBm */
} lte_quality_t;

/** @} ltesigquality */

/**
 * @defgroup ltecellinfo Cell Information
 * @{
 */

/**
 * @brief
 * Definition of parameters for neighbor cell information.
 * This is reported by cellinfo_report_cb_t
 */
typedef struct {
  ltevalidflag_t valid; /**< Refer to the following
                                            parameters only when this
                                            flag is valid, plase refer to @ref ltevalidflag_t */
  uint32_t cell_id;     /**< Physical cell ID (0-503) */
  uint32_t earfcn;      /**< EARFCN (0-262143) */

  uint16_t sfn; /**< When setting sfn,
                  LTE_TIMEDIFFIDX_VALID flag has been added to "valid".
                 */

  int16_t rsrp; /**< When setting rsrp,
                  LTE_RSRP_VALID flag has been added to "valid".
                 */

  int16_t rsrq; /**< When setting rsrq,
                  LTE_RSRP_VALID flag has been added to "valid".
                 */
} lte_neighbor_cellinfo_t;

/**
 * @brief
 * Definition of parameters for cell information.
 * This is reported by cellinfo_report_cb_t
 */

typedef struct lte_cellinfo {
  ltevalidflag_t valid;                          /**< Refer to the following
                                            parameters only when this
                                            flag is valid, plase refer to @ref ltevalidflag_t */
  uint32_t phycell_id;                           /**< Physical cell ID (0-503) */
  uint32_t earfcn;                               /**< EARFCN (0-262143) */
  uint8_t mcc[LTE_CELLINFO_MCC_DIGIT];           /**< Mobile Country Code (000-999)
                                                      See @ref lteidentifier */
  uint8_t mnc_digit;                             /**< Digit number of mnc(2-3) */
  uint8_t mnc[LTE_CELLINFO_MNC_DIGIT_MAX];       /**< Mobile Network Code (00-999)
                                                      See @ref lteidentifier */
  uint8_t cgid[LTE_CELLINFO_CGID_DIGIT_MAX + 1]; /**< Cell Global ID LAC+CI(CGI) or eNB+CI(eCGI)
                                                See @ref lteidentifier */
  uint16_t tac;                                  /**< Tracking area code */
  uint16_t time_diffidx;                         /**< When setting time diff index,
                                         LTE_TA_VALID flag has been added to "valid".
                                        */
  uint16_t ta;                                   /**< When setting time advance,
                                                    LTE_TA_VALID flag has been added to "valid".
                                                   */
  uint16_t sfn;                                  /**< When setting sfn,
                                                   LTE_TIMEDIFFIDX_VALID flag has been added to "valid".
                                                  */
  int16_t rsrp;                                  /**< When setting rsrp,
                                                   LTE_RSRP_VALID flag has been added to "valid".
                                                  */
  int16_t rsrq;                                  /**< When setting rsrq,
                                                   LTE_RSRP_VALID flag has been added to "valid".
                                                  */
  uint8_t neighbor_num;                          /**< Valid number of neighbor information */
  lte_neighbor_cellinfo_t neighbor_cell[LTE_MAX_NEIGHBOR_CELL_INFO]; /**< Neighbor cell information
see @ref lte_neighbor_cellinfo_t */
} lte_cellinfo_t;

/** @} ltecellinfo */

/**
 * @defgroup ltecfgtype Configuration Types of PSM/eDRX
 * @{
 */

/**
 * @brief
 * Definitions of configuration type of PSM/eDRX used in lte_get_psm() and lte_get_edrx()
 */

typedef enum {
  LTE_CFGTYPE_REQUESTED = 0,   /**< Requested Configuration */
  LTE_CFGTYPE_NW_PROVIDED = 1, /**< Network-Provided Configuration */
  LTE_CFGTYPE_MAX = 2,         /**< Maximum configuration type number */
} ltecfgtype_e;
/** @} ltecfgtype */

/**
 * @defgroup lteedrx eDRX Settings
 * @{
 */

/**
 * @brief
 * Definitions of eDRX act type used in lte_edrx_setting_t
 */

typedef enum {
  LTE_EDRX_ACTTYPE_UNSUPPORTED = 0, /**< eDRX not supported */
  LTE_EDRX_ACTTYPE_WBS1 = 4,        /**< E-UTRAN (WB-S1 mode) */
  LTE_EDRX_ACTTYPE_NBS1 = 5         /**< E-UTRAN (NB-S1 mode) */
} lteedrxtype_e;

/**
 * @brief
 * Enumerations of eDRX cycle used in lte_edrx_setting_t
 */

typedef enum {
  LTE_EDRX_CYC_512 = 0,      /**< eDRX cycle     5.12 sec (WB-S1 Only) */
  LTE_EDRX_CYC_1024 = 1,     /**< eDRX cycle    10.24 sec (WB-S1 Only) */
  LTE_EDRX_CYC_2048 = 2,     /**< eDRX cycle    20.48 sec */
  LTE_EDRX_CYC_4096 = 3,     /**< eDRX cycle    40.96 sec */
  LTE_EDRX_CYC_6144 = 4,     /**< eDRX cycle    61.44 sec (WB-S1 Only) */
  LTE_EDRX_CYC_8192 = 5,     /**< eDRX cycle    81.92 sec */
  LTE_EDRX_CYC_10240 = 6,    /**< eDRX cycle   102.40 sec (WB-S1 Only) */
  LTE_EDRX_CYC_12288 = 7,    /**< eDRX cycle   122.88 sec (WB-S1 Only) */
  LTE_EDRX_CYC_14336 = 8,    /**< eDRX cycle   143.36 sec (WB-S1 Only) */
  LTE_EDRX_CYC_16384 = 9,    /**< eDRX cycle   163.84 sec */
  LTE_EDRX_CYC_32768 = 10,   /**< eDRX cycle   327.68 sec */
  LTE_EDRX_CYC_65536 = 11,   /**< eDRX cycle   655.36 sec */
  LTE_EDRX_CYC_131072 = 12,  /**< eDRX cycle  1310.72 sec */
  LTE_EDRX_CYC_262144 = 13,  /**< eDRX cycle  2621.44 sec */
  LTE_EDRX_CYC_524288 = 14,  /**< eDRX cycle  5242.88 sec (NB-S1 Only) */
  LTE_EDRX_CYC_1048576 = 15, /**< eDRX cycle  10485.76 sec (NB-S1 Only) */
} lteedrxcyc_e;

/**
 * @brief
 * Enumerations of eDRX paging time window used in lte_edrx_setting_t
 */
typedef enum {
  LTE_EDRX_PTW_128 = 0,   /**< Paging time window  1.28 sec */
  LTE_EDRX_PTW_256 = 1,   /**< Paging time window  2.56 sec */
  LTE_EDRX_PTW_384 = 2,   /**< Paging time window  3.84 sec */
  LTE_EDRX_PTW_512 = 3,   /**< Paging time window  5.12 sec */
  LTE_EDRX_PTW_640 = 4,   /**< Paging time window  6.40 sec */
  LTE_EDRX_PTW_768 = 5,   /**< Paging time window  7.68 sec */
  LTE_EDRX_PTW_896 = 6,   /**< Paging time window  8.96 sec */
  LTE_EDRX_PTW_1024 = 7,  /**< Paging time window 10.24 sec */
  LTE_EDRX_PTW_1152 = 8,  /**< Paging time window 11.52 sec */
  LTE_EDRX_PTW_1280 = 9,  /**< Paging time window 12.80 sec */
  LTE_EDRX_PTW_1408 = 10, /**< Paging time window 14.08 sec */
  LTE_EDRX_PTW_1536 = 11, /**< Paging time window 15.36 sec */
  LTE_EDRX_PTW_1664 = 12, /**< Paging time window 16.64 sec */
  LTE_EDRX_PTW_1792 = 13, /**< Paging time window 17.92 sec */
  LTE_EDRX_PTW_1920 = 14, /**< Paging time window 19.20 sec */
  LTE_EDRX_PTW_2048 = 15  /**< Paging time window 20.48 sec */
} lteedrxptw_e;

/**
 * @brief
 * Definition of eDRX settings used in lte_set_edrx().
 */

typedef struct lte_edrx_setting {
  lteedrxtype_e act_type;  /**< eDRX act type. @ref lteedrxtype_e */
  lteenableflag_e enable;  /**< eDRX enable. See @ref lteenableflag_e */
  lteedrxcyc_e edrx_cycle; /**< eDRX cycle. See @ref lteedrxcyc_e */
  lteedrxptw_e ptw_val;    /**< Paging time window. See @ref lteedrxptw_e */
} lte_edrx_setting_t;

/** @} lteedrx */

/**
 * @defgroup ltepsm PSM Timer Information
 * @{
 */

/**
 * @brief
 * Enumerations of Requested Active Time value (T3324)
 * used in lte_psm_timeval_t
 */

typedef enum {
  LTE_PSM_T3324_UNIT_2SEC = 0, /**< Unit of request active time(T3324) 2 sec */
  LTE_PSM_T3324_UNIT_1MIN = 1, /**< Unit of request active time(T3324) 1 min */
  LTE_PSM_T3324_UNIT_6MIN = 2  /**< Unit of request active time(T3324)  6 min */
} ltepsmt3324unit_e;

/**
 * @brief
 * Enumerations of Extended periodic TAU(Tracking Area Update) value (T3412)
 * used in lte_psm_timeval_t
 */

typedef enum {
  LTE_PSM_T3412_UNIT_2SEC = 0,   /**< Unit of extended periodic TAU time(T3412) 2 sec */
  LTE_PSM_T3412_UNIT_30SEC = 1,  /**< Unit of extended periodic TAU time(T3412) 30 sec */
  LTE_PSM_T3412_UNIT_1MIN = 2,   /**< Unit of extended periodic TAU time(T3412) 1 min */
  LTE_PSM_T3412_UNIT_10MIN = 3,  /**< Unit of extended periodic TAU time(T3412) 10 min */
  LTE_PSM_T3412_UNIT_1HOUR = 4,  /**< Unit of extended periodic TAU time(T3412) 1 hour */
  LTE_PSM_T3412_UNIT_10HOUR = 5, /**< Unit of extended periodic TAU time(T3412) 10 hour */
  LTE_PSM_T3412_UNIT_320HOUR = 6 /**< Unit of extended periodic TAU time(T3412) 320 hour */
} ltepsmt3412unit_e;

/**
 * @brief
 * Definition of timer information for PSM
 */

typedef struct lte_psm_timeval {
  uint8_t unit; /**< Unit of timer value. See @ref ltepsmt3324unit_e or @ref ltepsmt3412unit_e */
  uint8_t time_val; /**< Timer value (1-31) */
} lte_psm_timeval_t;

/**
 * @brief
 * Definition of PSM settings used in lte_set_psm().
 */

typedef struct lte_psm_setting {
  lteenableflag_e enable;                  /**< PSM enable. See @ref lteenableflag_e */
  lte_psm_timeval_t active_time;           /**< Active Time value (T3324) */
  lte_psm_timeval_t ext_periodic_tau_time; /**< Extended periodic TAU value (T3412) */
} lte_psm_setting_t;

/** @} ltepsm */

/**
 * @defgroup regstat Network Registration State
 * @{
 */

/**
 * @brief
 * Enumerations of network registration state. These are notified by regstate_report_cb_t
 */

typedef enum {
  LTE_REGSTAT_NOT_REGISTERED_NOT_SEARCHING =
      0, /**< not registered, MT is not currently searching an operator to register to */
  LTE_REGSTAT_REGISTERED_HOME = 1,          /**< registered, home network */
  LTE_REGSTAT_NOT_REGISTERED_SEARCHING = 2, /**< not registered, but MT is currently trying to
                                               attach or searching an operator to register to */
  LTE_REGSTAT_REGISTRATION_DENIED = 3,      /**< registration denied */
  LTE_REGSTAT_UNKNOWN = 4,                  /**< unknown (e.g. out of E-UTRAN coverage) */
  LTE_REGSTAT_REGISTERED_ROAMING = 5        /**< registered, roaming */
} lteregstate_e;

/** @} regstat */

/**
 * @defgroup psmstate PSM State
 * @{
 */

/**
 * @brief
 * Enumerations of psm state. These are notified by psmstate_report_cb_t
 */

typedef enum {
  LTE_PSMSTAT_NOT_ACTIVE = 0,        /**< PSM is not Active */
  LTE_PSMSTAT_ACTIVE = 1,            /**< PSM is Active */
  LTE_PSMSTAT_ACTIVE_AND_CAMPED = 2, /**< PSM is Active and Camped on a cell */
  LTE_PSMSTAT_CAMP_INTERRUPTED = 3   /**< PSM is Camping interrupted by timeout */

} ltepsmstate_e;

/**
 * @brief
 * Enumerations of psm activity state. These are notified by psmstate_report_cb_t
 */

typedef enum {
  LTE_PSMACTSTAT_INACTIVE = 0, /**< PSM is Inactive */
  LTE_PSMACTSTAT_ACTIVE = 1,   /**< PSM is Active */
} ltepsmactstate_e;

/** @} psmstate */

/**
 * @defgroup conphase Network Connectivity Phase
 * @{
 */

/**
 * @brief
 * Enumerations of network connectivity phase. These are notified by regstate_report_cb_t
 */

typedef enum {
  LTE_CONPHASE_START_SCAN = 0,               /**< Start Scan */
  LTE_CONPHASE_FAIL_SCAN = 1,                /**< Fail Scan */
  LTE_CONPHASE_ENTER_CAMPED = 2,             /**< Enter Camped */
  LTE_CONPHASE_CONNECTION_ESTABLISHMENT = 3, /**< Connection Establishment */
  LTE_CONPHASE_START_RESCAN = 4,             /**< Start Rescan */
  LTE_CONPHASE_CONNECTED = 5,                /**< Connected */
  LTE_CONPHASE_NO_SUITABLE_CELL = 6,         /**< No Suitable Cell Found */
  LTE_CONPHASE_REG_ATTEMPT_FAILED = 7,       /**< All registration attempts failed */
  LTE_CONPHASE_NOT_AVAIL = 99,               /**< Temporarily not available */
} lteconphase_e;

/**
 * @brief
 * Enumerations of network connectivity phase rat type. These are notified by regstate_report_cb_t
 */

typedef enum {
  LTE_CONPHASE_RAT_CATM = 0,       /**< CAT-M */
  LTE_CONPHASE_RAT_NBIOT = 1,      /**< NB-IOT */
  LTE_CONPHASE_RAT_GSM = 3,        /**< GSM */
  LTE_CONPHASE_RAT_NOT_AVAIL = 99, /**< Temporarily not available */
} lteconphase_rat_e;

/**
 * @brief
 * Enumerations of network connectivity phase - scan type. These are notified by
 * regstate_report_cb_t
 */

typedef enum {
  LTE_CONPHASE_SCAN_MRU_ONLY = 0,             /**< MRU only Scan */
  LTE_CONPHASE_SCAN_MRU_AND_FULL_SCAN = 1,    /**< MRU and Full Scan */
  LTE_CONPHASE_SCAN_MRU_AND_COUNTRY_SCAN = 2, /**< MRU and Country Scan */
  LTE_CONPHASE_SCAN_MRU_AND_LS = 3,           /**< MRU and Limited Scan*/
  LTE_CONPHASE_SCAN_NOT_AVAIL = 99,           /**< Temporarily not available */
} lteconfphase_scan_type_e;

/**
 * @brief
 * Enumerations of network connectivity phase - scan reason. These are notified by
 * regstate_report_cb_t
 */

typedef enum {
  LTE_CONPHASE_SCAN_REASON_INIT_SCAN = 0,       /**< Initial scan */
  LTE_CONPHASE_SCAN_REASON_OUT_OF_COVERAGE = 1, /**< Out of coverage */
  LTE_CONPHASE_SCAN_REASON_HIGH_PRIORITY = 2,   /**< High priority plmn search */
  LTE_CONPHASE_SCAN_REASON_LIMITED_SERVICE = 3, /**< Limited service */
  LTE_CONPHASE_SCAN_REASON_COPS = 4,            /**< COPS have been called (Manual or Automatic) */
  LTE_CONPHASE_SCAN_REASON_OTHER = 5,           /**< Other */
  LTE_CONPHASE_SCAN_REASON_NOT_AVAIL = 99,      /**< Temporarily not available */
} lteconfphase_scan_reason_e;

/** @} conphase */

/**
 * @defgroup scan_scheme Scan Scheme Configuration
 * @{
 */

/**
 * @brief
 * Definition of the configration structure of scanning scheme.
 */
typedef struct {
  uint16_t num_mru_scans;           /**< Number of MRU scans */
  uint16_t num_country_scans;       /**< Number of country scans */
  uint16_t num_region_scans;        /**< Number of region scans */
  uint8_t num_full_scans;           /**< 0/1 value, indicates if full scan exist in scheme or not */
  uint8_t fallback_full_to_country; /**< indicates if fallback from full scan to country scan will
                                       be performed if plmn found during full scan. 1: enable, 0:
                                       disable */
  uint32_t slptime_btwn_scans;      /**< delay between scans */
  uint32_t max_slptime_btwn_scans;  /**< maximum delay between scans */
  int16_t slptime_step; /**< >0: The value will increase linearly, will multiple the next
"slptime_btwn_scans" value with "slptime_step",
 0: the "slptime_btwn_scans" stays constant,
 -1: The value will increase exponentially. Step will be increased in power of 2 */
} lte_scan_scheme_t;

/**
 * @brief
 * Definition of the additional information of the last failure scanning.
 */
typedef enum {
  LTE_SCAN_INFO_SS, /**< Stored scan, use MRU table only */
  LTE_SCAN_INFO_FS, /**< Full scan */
  LTE_SCAN_INFO_CS, /**< Country scan */
  LTE_SCAN_INFO_LS, /**< Limited scan */
  LTE_SCAN_INFO_RS  /**< Region scan(TBD) */
} lte_scan_info_t;

/**
 * @brief
 * Definition of the result code of scanning.
 */
typedef enum {
  LTE_SCAN_RESULTCODE_SUCCESS, /**< Scanning succeeded */
  LTE_SCAN_RESULTCODE_FAILURE  /**< Scanning failure */
} lte_scan_resultcode_t;

/**
 * @brief
 * Definition of the result of scanning.
 */
typedef struct {
  lte_scan_resultcode_t resultcode; /**< Result code */
  lte_scan_info_t info;             /**< Additional information on scanning failure */
} lte_scan_result_t;

/** @} scan_scheme */

/**
 * @defgroup lwm2m_fw_update_event lwm2m fw upgrade event
 * @{
 */

/**
 * @brief
 * Definition of the lwm2m fw upgrade event type.
 */
typedef enum {
  LTE_LWM2M_FW_UPGRADE_EVT_PENDING_DOWNLOAD,            /**< PENDING DOWNLOAD */
  LTE_LWM2M_FW_UPGRADE_EVT_PENDING_UPDATE,              /**< PENDING UPDATE */
  LTE_LWM2M_FW_UPGRADE_EVT_DOWNLOAD_COMPLETED,          /**< DOWNLOAD COMPLETED */
  LTE_LWM2M_FW_UPGRADE_EVT_DOWNLOAD_FAILED,             /**< DOWNLOAD FAILED */
  LTE_LWM2M_FW_UPGRADE_EVT_FOTA_CANCELD_BY_LWM2M_SERVER /**< FOTA CANCELD BY LWM2M SERVER */
} lte_lwm2m_fw_upgrade_evt_type_t;

/**
 * @brief
 * Definition For DOWNLOAD FAILED - if the failure is fatal:
 */
typedef enum {
  LTE_LWM2M_FW_UPGRADE_EVT_DOWNLOAD_FAILED_NON_FATAL, /**< Download can be resumed by FOTA manager
                                                       */
  LTE_LWM2M_FW_UPGRADE_EVT_DOWNLOAD_FAILED_FATAL      /**< Download resume is not possible, FOTA
                                                         manager shall move to idle */
} lte_lwm2m_fw_upgrade_evt_error_type_t;

/**
 * @brief
 * Definition for available values to the pdcp discard timer
 */
typedef enum {
  LTE_PDCP_DISCARD_TIMER_OFF = 0,      /**< Use the value from network */
  LTE_PDCP_DISCARD_TIMER_INFINITY = 1, /**< Set timer to infinity */
  LTE_PDCP_DISCARD_TIMER_50MS = 2,     /**< Set timer to 50 ms */
  LTE_PDCP_DISCARD_TIMER_100MS = 3,    /**< Set timer to 100 ms */
  LTE_PDCP_DISCARD_TIMER_150MS = 4,    /**< Set timer to 150 ms */
  LTE_PDCP_DISCARD_TIMER_300MS = 5,    /**< Set timer to 300 ms */
  LTE_PDCP_DISCARD_TIMER_500MS = 6,    /**< Set timer to 500 ms */
  LTE_PDCP_DISCARD_TIMER_750MS = 7,    /**< Set timer to 750 ms */
  LTE_PDCP_DISCARD_TIMER_1500MS = 8,   /**< Set timer to 1500 ms */
  LTE_PDCP_DISCARD_TIMER_MAX,
} lte_set_pdcp_discard_timer_t;

/**
 * @brief
 * Definition of maximum string length of the lwm2m fw upgrade package name
 */

#define LTE_LWM2M_FW_UPGRADE_PACKAGE_NAME_LEN \
  30 /**< The maximum string length of the lwm2m fw upgrade package name */

/**
 * @brief
 * Definition of the lwm2m fw upgrade event.
 */
typedef struct {
  lte_lwm2m_fw_upgrade_evt_type_t eventtype; /**< Event Type */
  uint32_t package_size;                     /**< The package size in bytes.*/
  uint8_t package_name[LTE_LWM2M_FW_UPGRADE_PACKAGE_NAME_LEN];
  lte_lwm2m_fw_upgrade_evt_error_type_t error_type; /**< Failure cause in case of failure event */

} lte_lwm2m_fw_upgrade_evt_t;

/** @} lwm2m_fw_update_event */

/**
 * @defgroup fw_update_event fw upgrade event
 * @{
 */

/**
 * @brief
 * Definition of the fw upgrade event type.
 */
typedef enum {
  LTE_FW_UPGRADE_EVT_DLPENDING = 0,    /**< DLPENDING */
  LTE_FW_UPGRADE_EVT_DLDONE = 1,       /**< DLDONE */
  LTE_FW_UPGRADE_EVT_UPPENDING = 2,    /**< UPPENDING */
  LTE_FW_UPGRADE_EVT_REBOOTNEEDED = 3, /**< REBOOTNEEDED */
  LTE_FW_UPGRADE_EVT_FAILURE = 4       /**< FAILURE */
} lte_fw_upgrade_evt_type_t;

/**
 * @brief
 * Definition For image type for images to download/update
 */
typedef enum {
  LTE_FW_UPGRADE_EVT_IMAGE_TYPE_MODEM_FW = 1,                 /**< MODEM_FW */
  LTE_FW_UPGRADE_EVT_IMAGE_TYPE_HOST_SW = 2,                  /**< HOST_SW */
  LTE_FW_UPGRADE_EVT_IMAGE_TYPE_BOTH_HOST_SW_AND_MODEM_FW = 3 /**< BOTH_HOST_SW_AND_MODEM_FW */
} lte_fw_upgrade_image_type_t;

/**
 * @brief
 * Definition For failure cause for download/update failure if eventtype is
 * LTE_FW_UPGRADE_EVT_FAILURE
 */
typedef enum {
  LTE_FW_UPGRADE_EVT_FAILURE_CAUSE_RESERVED_0 = 0, /**< RESERVED_0 */
  LTE_FW_UPGRADE_EVT_FAILURE_CAUSE_RESERVED_1 = 1, /**< RESERVED_1 */
  LTE_FW_UPGRADE_EVT_FAILURE_CAUSE_NOT_ENOUGH_FLASH_MEMORY_DURING_DOWNLOAD =
      2, /**< NOT_ENOUGH_FLASH_MEMORY_DURING_DOWNLOAD */
  LTE_FW_UPGRADE_EVT_FAILURE_CAUSE_OUT_OF_RAM_DURING_DOWNLOAD =
      3, /**< OUT_OF_RAM_DURING_DOWNLOAD */
  LTE_FW_UPGRADE_EVT_FAILURE_CAUSE_CONNECTION_LOST_DURING_DOWNLOAD =
      4, /**< CONNECTION_LOST_DURING_DOWNLOAD */
  LTE_FW_UPGRADE_EVT_FAILURE_CAUSE_INTEGRITY_CHECK_FAILURE = 5,  /**< INTEGRITY_CHECK_FAILURE */
  LTE_FW_UPGRADE_EVT_FAILURE_CAUSE_UNSUPPORTED_PACKAGE_TYPE = 6, /**< UNSUPPORTED_PACKAGE_TYPE */
  LTE_FW_UPGRADE_EVT_FAILURE_CAUSE_INVALID_URI = 7,              /**< INVALID_URI */
  LTE_FW_UPGRADE_EVT_FAILURE_CAUSE_IMAGE_UPDATE_FAILED = 8,      /**< IMAGE_UPDATE_FAILED */
  LTE_FW_UPGRADE_EVT_FAILURE_CAUSE_UNSUPPORTED_PROTOCOL = 9      /**< UNSUPPORTED_PROTOCOL */
} lte_fw_upgrade_failure_cause_t;

/**
 * @brief
 * Definition of maximum string length of the lwm2m fw upgrade package name
 */

#define LTE_LWM2M_FW_UPGRADE_PACKAGE_NAME_LEN \
  30 /**< The maximum string length of the lwm2m fw upgrade package name */

/**
 * @brief
 * Definition of the fw upgrade event.
 */
typedef struct {
  lte_fw_upgrade_evt_type_t eventtype;         /**< Event Type */
  lte_fw_upgrade_image_type_t imagetype;       /**< The package size in bytes.*/
  lte_fw_upgrade_failure_cause_t failurecause; /**< The package size in bytes.*/

} lte_fw_upgrade_evt_t;

/** @} fw_update_event */

/**
 * @defgroup ltecallback Callback Functions Definition
 * @{
 */

/**
 * @brief
 *  Since lte_attach_network() is an asynchronous API,
 *  the result is notified by this function.
 * @param [in] result: The result of lte_attach_network().
 * See @ref lteresult.
 * @param [in] errcause: This parameter is valid when result
 * is an error. See @ref lteerrcause.
 * @param [in] userPriv: User's private data.
 */

typedef void (*attach_net_cb_t)(lteresult_e result, lteerrcause_e errcause, void *userPriv);

/**
 * @brief
 *  Since lte_detach_network() is an asynchronous API,
 *  the result is notified by this function.
 * @param [in] result: The result of lte_detach_network().
 * See @ref lteresult.
 * @param [in] userPriv: User's private data.
 */

typedef void (*detach_net_cb_t)(lteresult_e result, void *userPriv);

/**
 * @brief
 *  Since lte_data_on() is an asynchronous API,
 *  the result is notified by this function.
 * @param [in] result: The result of lte_data_on().
 * See @ref lteresult.
 * @param [in] errcause: This parameter is valid when result
 * is an error. See @ref lteerrcause.
 * @param [in] userPriv: User's private data.
 */

typedef void (*data_on_cb_t)(lteresult_e result, lteerrcause_e errcause, void *userPriv);

/**
 * @brief
 *  Since lte_data_off() is an asynchronous API,
 *  the result is notified by this function.
 * @param [in] result: The result of lte_data_off().
 * See @ref lteresult.
 * @param [in] errcause: This parameter is valid when result
 * is an error. See @ref lteerrcause.
 * @param [in] userPriv: User's private data.
 */

typedef void (*data_off_cb_t)(lteresult_e result, lteerrcause_e errcause, void *userPriv);

/**
 * @brief
 *  When the network state changes, the network state is
 *  reported by this function.
 * @param [in] netstate: The network state.
 * See @ref ltenetstate_e.
 * @param [in] userPriv: User's private data.
 */

typedef void (*netstate_report_cb_t)(uint8_t sessionid, ltenetstate_e netstate, void *userPriv);

/**
 * @brief
 *  When the timer event changes, the timer state is
 *  reported by this function.
 * @param [in] ev_type: Report received event type .
 * @param [in] status: Report received event status.
 *
 * @param [in] userPriv: User's private data
 */

typedef void (*timerevt_report_cb_t)(lte_timerevt_evttype_t ev_type, lte_timerevt_evsel_t status,
                                     void *userPriv);

/**
 * @brief
 *  When the SIM state changes, the SIM state is
 *  reported by this function.
 * @param [in] simstate: The SIM state.
 * See @ref ltesimstate.
 * @param [in] userPriv: User's private data.
 */

typedef void (*simstate_report_cb_t)(ltesimstate_e simstate, void *userPriv);

/**
 * @brief
 *  When the local time changes, the local time is
 *  reported by this function.
 * @param [in] localtime: Local time.
 * See @ref lte_localtime_t.
 * @param [in] userPriv: User's private data.
 */

typedef void (*localtime_report_cb_t)(lte_localtime_t *localtime, void *userPriv);

/**
 * @brief
 *  The quality information is reported by this function. It is reported
 *  at intervals of the set report period.
 * @param [in] quality: Quality information.
 * See @ref lte_quality_t.
 * @param [in] userPriv: User's private data.
 */

typedef void (*quality_report_cb_t)(lte_quality_t *quality, void *userPriv);

/**
 * @brief
 *  The cell information is reported by this function. It is reported
 *  at intervals of the set report period.
 * @param [in] cellinfo: Cell information.
 * See @ref lte_cellinfo_t.
 * @param [in] userPriv: User's private data.
 */

typedef void (*cellinfo_report_cb_t)(lte_cellinfo_t *cellinfo, void *userPriv);

/**
 * @brief
 *  When the network registration state changes, the registration state is
 *  reported by this function.
 * @param [in] regstate: The network registration state.
 * See @ref lteregstate_e.
 * @param [in] userPriv: User's private data.
 */

typedef void (*regstate_report_cb_t)(lteregstate_e regstate, void *userPriv);

/**
 * @brief
 *  When the psm state changes, the psm state is
 *  reported by this function.
 * @param [in] psmstate: The psm state.
 * See @ref ltepsmstate_e.
 * @param [in] userPriv: User's private data.
 */

typedef void (*psmstate_report_cb_t)(ltepsmstate_e psmstate, void *userPriv);

/**
 * @brief
 *  When the configured anti tamper event is triggered,
 *  it is reported by this function.
 * @param [in] data: Reserved for future use.
 * @param [in] userPriv: User's private data.
 */

typedef void (*antitamper_report_cb_t)(uint8_t data, void *userPriv);

/**
 * @brief
 *  When the network given psm setting changes, the psm setting are
 *  reported by this function.
 * @param [in] psm_settings: The psm settings given by the network.
 * See @ref lte_psm_setting_t
 * @param [in] userPriv: User's private data.
 */

typedef void (*psm_settings_report_cb_t)(lte_psm_setting_t *psm_settings, void *userPriv);

/**
 * @brief
 *  When the network given edrx setting changes, the psm setting are
 *  reported by this function.
 * @param [in] edrx_settings: The edrx settings given by the network.
 * See @ref lte_edrx_setting_t
 * @param [in] userPriv: User's private data.
 */

typedef void (*edrx_settings_report_cb_t)(lte_edrx_setting_t *edrx_settings, void *userPriv);

/**
 * @brief
 *  When the network connectivity phase changes, the phase is
 *  reported by this function.
 * @param [in] conphase: The connectivity phase.
 * See @ref lteconphase_e
 * @param [in] rat: The RAT type.
 * See @ref lteconphase_rat_e
 * @param [in] scan_type: The scanning type.
 * See @ref lteconfphase_scan_type_e
 * @param [in] scan_reason: The scanning reason.
 * See @ref lteconfphase_scan_reason_e
 * @param [in] userPriv: User's private data.
 */

typedef void (*conphase_report_cb_t)(lteconphase_e conphase, lteconphase_rat_e rat,
                                     lteconfphase_scan_type_e scan_type,
                                     lteconfphase_scan_reason_e scan_reason, void *userPriv);

/**
 * @brief
 * The reporting function of the scanning result with specified scheme.
 *
 * @param [out] scanresult: The scanning result of specified scheme. See @ref lte_scan_result_t.
 * @param [in] userPriv: User's private data.
 */

typedef void (*scanresult_report_cb_t)(lte_scan_result_t *scanresult, void *userPriv);

/**
 * @brief
 * The reporting function of the lwm2m fw upgrade event.
 *
 * @param [out] lwm2m_fw_upgrade_event: The LWM2M FW Upgrade event. See @ref
 * lte_lwm2m_fw_upgrade_evt_t.
 * @param [in] userPriv: User's private data.
 */

typedef void (*lwm2m_fw_upgrade_evt_report_cb_t)(lte_lwm2m_fw_upgrade_evt_t *lwm2m_fw_upgrade_event,
                                                 void *userPriv);

/**
 * @brief
 * The reporting function of the fw upgrade event.
 *
 * @param [out] fw_upgrade_event: The FW Upgrade event. See @ref
 * lte_fw_upgrade_evt_t.
 * @param [in] userPriv: User's private data.
 */

typedef void (*fw_upgrade_evt_report_cb_t)(lte_fw_upgrade_evt_t *fw_upgrade_event, void *userPriv);

/** @} ltecallback */

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
 * @defgroup lte_funcs LTE APIs
 * @{
 */

/**
 * @brief lte_attach_network() attach to the LTE network.
 *
 * @param [in] callback: Callback function to notify that
 * attach completed
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_attach_network(attach_net_cb_t callback, void *userPriv);

/**
 * @brief lte_detach_network() detach from the LTE network.
 *
 * @param [in] radiomode: The radio mode to stay after detaching, also see @ref lteradiomode_e
 * @param [in] callback: Callback function to notify that detach completed
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_detach_network(lteradiomode_e radiomode, detach_net_cb_t callback, void *userPriv);

/**
 * @brief lte_set_cfun() set modem functionality.
 *
 * @param [in] fun: "Enable" or "Disable".
 * See @ref ltecfunoptn_e

 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_set_cfun(ltecfunoptn_e fun);

/**
 * @brief lte_get_netstat() get network state of the LTE; The network state means whether it has
 * been attached or not.
 *
 * @param [inout] state: The state of network state, and valid only if LTE_RESULT_OK returned.
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_netstat(ltenetstate_e *state);

/**
 * @brief lte_data_on() enable the data communication feature.
 *
 * @param [in] session_id: The numeric value of the session ID defined in the apn setting. See
 * @ref ltesessionid for valid range
 * @param [in] callback: Callback function to notify that enabling the data communication
 * completed
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_data_on(uint8_t session_id, data_on_cb_t callback, void *userPriv);

/**
 * @brief lte_data_off() disable the data communication feature.
 *
 * @param [in] session_id: The numeric value of the session ID defined in the apn setting. See
 * @ref ltesessionid for valid range
 * @param [in] callback: Callback function to notify that disabling the data communication
 * completed
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_data_off(uint8_t session_id, data_off_cb_t callback, void *userPriv);

/**
 * @brief lte_get_datastate() get state of the data communication.
 *
 * @param [inout] state: The state of data communication, and it's valid only if LTE_RESULT_OK
 * returned;
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_datastate(lte_datastatelist_t *state);

/**
 * @brief lte_get_dataconfig() get configration of the data transfer; There are two types of data
 * that can be specified: user data or IMS.
 *
 * @param [in] data_type: Data type. See @ref ltedatatype
 * @param [inout] general: Data transfer for general, and it's valid only if LTE_RESULT_OK
 * returned. See @ref lteenableflag_e
 * @param [inout] roaming: Data transfer for roaming, and it's valid only if LTE_RESULT_OK
 * returned. See @ref lteenableflag_e
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_dataconfig(ltedatatype_e data_type, lteenableflag_e *general,
                               lteenableflag_e *roaming);

/**
 * @brief lte_set_dataconfig() change configration of the data transfer; There are two types of
 * data that can be specified: user data or IMS.
 *
 * Details are shown in the table below.
 *
 * general  | roaming  | data transfer(Home/Roaming)
 * -------- | -------- | -----------------------------
 * disable  | disable  | Not available / Not available
 * disable  | enable   | Not available / Not available
 * enable   | disable  | Available / Not available
 * enable   | enable   | Available / Available
 *
 * @param [in] data_type: Data type. See @ref ltedatatype
 * @param [in] general: Data transfer for general.
 * See @ref lteenableflag_e
 * @param [in] roaming: Data transfer for roaming.
 * See @ref lteenableflag_e
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_set_dataconfig(ltedatatype_e data_type, lteenableflag_e general,
                               lteenableflag_e roaming);

/**
 * @brief lte_get_apnlist() get access point name settings.
 *
 * @param [inout] apn: APN settings, and it's valid only if LTE_RESULT_OK returned;
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_apnlist(lte_apnlist_t *apn);

/**
 * @brief lte_set_apn() set access point name settings.
 *
 * @param [in] session_id: The numeric value of the session ID.
 * See @ref ltesessionid for valid range
 * @param [in] apn: Character string of Access Point Name. The maximum string length is
 * LTE_APN_LEN, end with '\0'. See
 * @ref lteapnlen
 * @param [in] ip_type: Internet protocol type. See @ref lteapniptype
 * @param [in] auth_type: Authentication type. See @ref lteapnauthtype
 * @param [in] user_name: Character string of user name. The maximum string length is
 * LTE_APN_USER_NAME_LEN, end with
 * '\0'. See @ref lteapnusrnamelen
 * @param [in] password: Character string of password.
 * The maximum string length is LTE_APN_PASSWD_LEN, end with '\0'.
 * See @ref lteapnusrnamelen
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_set_apn(uint8_t session_id, int8_t *apn, lteapniptype_e ip_type,
                        lteapnauthtype_e auth_type, int8_t *user_name, int8_t *password);

/**
 * @brief lte_get_version() get the version of the modem.
 *
 * @param [inout] version: The version information of the modem.
 * See @ref lte_version_t
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_version(lte_version_t *version);

/**
 * @brief lte_get_phoneno() get phone number.
 *
 * @param [inout] errcause: Error cause.
 * See @ref lteerrcause
 * It is set only if the result is not successful.
 * @param [inout] phoneno: A character string indicating phone number.
 * It is terminated with '\0'; Note that is buffer should have sufficient length not less than
 * @ref LTE_MAX_PHONENUM_LEN.
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_phoneno(lteerrcause_e *errcause, int8_t *phoneno);

/**
 * @brief lte_get_imsi() get International Mobile Subscriber Identity(IMSI).
 *
 * @param [inout] errcause: Error cause. It is set only if the result is not successful. See @ref
 * lteerrcause
 * @param [inout] imsi: A character string indicating IMSI.
 * It is terminated with '\0'; Note that is buffer should have sufficient length not less than
 * @ref LTE_MAX_IMSI_LEN.
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_imsi(lteerrcause_e *errcause, int8_t *imsi);

/**
 * @brief lte_get_imei() get International Mobile Equipment Identifier(IMEI).
 *
 * See @ref lteerrcause
 * @param [inout] imei: A character string indicating IMEI.
 * It is terminated with '\0'; Note that is buffer should have sufficient length not less than
 * @ref LTE_MAX_IMEI_LEN.
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_imei(int8_t *imei);

/**
 * @brief lte_get_pinattributes() get Personal Identification Number(PIN) settings information.
 *
 * @param [inout] pinattr: PIN settings information, and it's valid only if LTE_RESULT_OK
 * returned; See @ref lte_pin_attributes_t
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_pinattributes(lte_pin_attributes_t *pinattr);

/**
 * @brief lte_set_pinenable() set Personal Identification Number(PIN) enable.
 *
 * @param [in] enable: "Enable" or "Disable". See @ref ltepinenable_e
 * @param [in] pincode: Current PIN code. Minimum number of digits is 4.
 * Maximum number of digits is 8, end with '\0'. (i.e. Max 9 byte)
 * @param [inout] attemptsleft : Number of attempts left.
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_set_pinenable(lteenableflag_e enable, int8_t *pincode, uint8_t *attemptsleft);

/**
 * @brief lte_change_pin() change Personal Identification Number(PIN).
 *
 * @param [in] target_pin: Target of change PIN. See @ref ltetargetpin_e
 * @param [in] pincode: Current PIN code. Minimum number of digits is 4.
 * Maximum number of digits is 8, end with '\0'. (i.e. Max 9 byte)
 * @param [in] new_pincode: New PIN code. Minimum number of digits is 4.
 * Maximum number of digits is 8, end with '\0'. (i.e. Max 9 byte)
 * @param [inout] attemptsleft: Number of attempts left.
 * It is set only if the result is not successful.
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_change_pin(ltetargetpin_e target_pin, int8_t *pincode, int8_t *new_pincode,
                           uint8_t *attemptsleft);

/**
 * @brief lte_enter_pin() enter Personal Identification Number(PIN).
 *
 * @param [in] pincode: Current PIN code. Minimum number of digits is 4.
 * Maximum number of digits is 8, end with '\0'. (i.e. Max 9 byte)
 * @param [in] new_pincode: If not used, set NULL.
 * If the PIN is SIM PUK or SIM PUK2, the new_pincode is required.
 * Minimum number of digits is 4.
 * Maximum number of digits is 8, end with '\0'. (i.e. Max 9 byte)
 * @param [inout] simstate: State after PIN enter.
 * See @ref ltesimstate
 * @param [inout] attemptsleft : Number of attempts left.
 * It is set only if the result is not successful.
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_enter_pin(int8_t *pincode, int8_t *new_pincode, ltesimstate_e *simstate,
                          uint8_t *attemptsleft);

/**
 * @brief lte_get_localtime() get local time.
 *
 * @param [inout] localtime: Local time, and it's valid only if LTE_RESULT_OK returned;
 * See @ref lte_localtime_t
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_localtime(lte_localtime_t *localtime);

/**
 * @brief lte_get_operator() get network operator information.
 *
 * @param [inout] oper: A character string indicating network operator.
 * It is terminated with '\0'
 * If it is not connected, the first character is '\0'.
 * Note that is buffer should have sufficient length not less than @ref LTE_MAX_OPERATOR_NAME_LEN.
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_operator(int8_t *oper);

/**
 * @brief lte_get_sleepmode() get sleep mode of the modem.
 *
 * @param [inout] sleepmode: Sleep mode of the modem.
 * See @ref lteslpmode
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_sleepmode(lteslpmode_e *sleepmode);

/**
 * @brief lte_set_sleepmode() set sleep mode of the modem.
 *
 * @param [in] sleepmode: Sleep mode of the modem.
 * See @ref lteslpmode
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_set_sleepmode(lteslpmode_e sleepmode);

/**
 * @brief lte_get_edrx() get the requested/network provided eDRX settings.
 *
 * @param [inout] settings: eDRX settings, and it's valid only if LTE_RESULT_OK returned;
 * See @ref lte_edrx_setting_t
 * @param [in] cfgType: Get setting from Requested/Network-Provided configuration
 * See @ref ltecfgtype_e
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_edrx(lte_edrx_setting_t *settings, ltecfgtype_e cfgType);

/**
 * @brief lte_set_edrx() set eDRX settings.
 *
 * @param [in] settings: eDRX settings
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_set_edrx(lte_edrx_setting_t *settings);

/**
 * @brief lte_get_psm() get the requested/network provided PSM settings.
 *
 * @param [inout] settings: PSM settings, and it's valid only if LTE_RESULT_OK returned;
 * See @ref lte_psm_setting_t
 * @param [in] cfgType: Get setting from Requested/Network-Provided configuration
 * See @ref ltecfgtype_e
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_psm(lte_psm_setting_t *settings, ltecfgtype_e cfgType);

/**
 * @brief lte_set_psm() set PSM settings.
 *
 * @param [in] settings: PSM settings
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_set_psm(lte_psm_setting_t *settings);

/**
 * @brief lte_set_report_netstate() Change the report setting of the LTE
 * network state and data communication state.
 * The default report setting is disable.
 *
 * @param [in] netstate_callback: Callback function to notify that
 * LTE network state. If NULL is set, the report setting is disabled.
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_netstate(netstate_report_cb_t netstate_callback, void *userPriv);

/**
 * @brief lte_set_report_simstate() Change the report setting of the SIM
 * state. The default report setting is disable.
 *
 * @param [in] simstate_callback: Callback function to notify that
 * SIM state. If NULL is set, the report setting is disabled.
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_simstate(simstate_report_cb_t simstate_callback, void *userPriv);

/**
 * @brief lte_set_report_localtime() Change the report setting of the local
 * time. The default report setting is disable.
 *
 * @param [in] localtime_callback: Callback function to notify that
 * local time. If NULL is set, the report setting is disabled.
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_localtime(localtime_report_cb_t localtime_callback, void *userPriv);

/**
 * @brief lte_set_report_quality() Change the report setting of the quality
 * information. The default report setting is disable.
 *
 * @param [in] quality_callback: Callback function to notify that
 * quality information. If NULL is set, the report setting is disabled.
 * @param [in] period: Reporting cycle in sec (1-4233600)
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_quality(quality_report_cb_t quality_callback, uint32_t period,
                               void *userPriv);

/**
 * @brief lte_set_report_cellinfo() Change the report setting of the cell
 * information. The default report setting is disable.
 *
 * @param [in] cellinfo_callback: Callback function to notify that
 * cell information. If NULL is set, the report setting is disabled.
 * @param [in] period: Reporting cycle in sec (1-4233600)
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_cellinfo(cellinfo_report_cb_t cellinfo_callback, uint32_t period,
                                void *userPriv);

/**
 * @brief lte_set_report_regstate() Change the report setting of the LTE
 * registration state
 * The default report setting is disable.
 *
 * @param [in] regstate_callback: Callback function to notify that
 * LTE registration  state. If NULL is set, the report setting is disabled.
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_regstate(regstate_report_cb_t regstate_callback, void *userPriv);

/**
 * @brief lte_set_report_active_psm() Change the report setting of the LTE
 * active PSM state.
 * The default report setting is disable.
 *
 * @param [in] psmstate_callback: Callback function to notify that
 * LTE PSM state. If NULL is set, the report setting is disabled.
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_active_psm(psmstate_report_cb_t psmstate_callback, void *userPriv);

/**
 * @brief lte_set_report_dynamic_psm() Change the report setting of the LTE
 * network given dynamic psm settings.
 * The default report setting is disable.
 *
 * @param [in] psm_settings_report_callback: Callback function to notify that
 * LTE network given psm setting has changed. If NULL is set, the report setting is disabled.
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_dynamic_psm(psm_settings_report_cb_t psm_settings_report_callback,
                                   void *userPriv);

/**
 * @brief lte_set_report_dynamic_psm() Change the report setting of the LTE
 * network given dynamic edrx settings.
 * The default report setting is disable.
 *
 * @param [in] edrx_settings_report_callback: Callback function to notify that
 * LTE network given edrx setting has changed. If NULL is set, the report setting is disabled.
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_dynamic_edrx(edrx_settings_report_cb_t edrx_settings_report_callback,
                                    void *userPriv);

/**
 * @brief lte_get_cellinfo() get the current cell information.
 *
 * @param [inout] cellinfo: Cell information.
 * See @ref lte_cellinfo_t
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_cellinfo(lte_cellinfo_t *cellinfo);

/**
 * @brief lte_get_quality() get the current cell quality information.
 *
 * @param [in] quality: Quality information.
 * See @ref lte_quality_t
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_quality(lte_quality_t *quality);

/**
 * @brief lte_set_report_connectivity_phase() Change the report setting of the LTE
 * connectivity phase
 * The default report setting is disable.
 *
 * @param [in] conphase_callback: Callback function to notify that
 * LTE connectivity phase changed. If NULL is set, the report setting is disabled.
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_connectivity_phase(conphase_report_cb_t conphase_callback, void *userPriv);

/**
 * @brief lte_set_report_antitamper() Change the report setting of the LTE
 * anti tamper event
 * The default report setting is disable.
 *
 * @param [in] alert_callback: Callback function to notify that
 * ANTI TAMPER event. If NULL is set, the report setting is disabled.
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_antitamper(antitamper_report_cb_t alert_callback, void *userPriv);

/**
 * @brief lte_psm_camp() enable/disable cell camping while still staying in PSM network state.
 *
 * @param [in] enable: enable/disable camping
 * @param [in] timeout: how long to try and camp on a cell in sec. valid range is 1~1200, 0 =
 * means the system default of 2min
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_psm_camp(lteenableflag_e enable, uint32_t timeout);

/**
 * @brief lte_get_active_psm() get if we are in active PSM state.
 *
 * @param [inout] actpsm: PSM activity state, and it's valid only if LTE_RESULT_OK returned;
 * See @ref ltepsmactstate_e
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */

lteresult_e lte_get_active_psm(ltepsmactstate_e *actpsm);

/**
 * @brief lte_get_ccid() get CCID.
 *
 * @param [out] ccid: CCID
 * See @ref lte_ccid_t
 *
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */
lteresult_e lte_get_ccid(lte_ccid_t *ccid);

/**
 * @brief lte_set_scan_scheme() set the scanning scheme.
 *
 * @param [in] scheme: The scan scheme ptr
 *
 * @return On success, LTE_RESULT_OK is returned; On failure, LTE_RESULT_ERR is returned.
 */

lteresult_e lte_set_scan_scheme(lte_scan_scheme_t *scheme);

/**
 * @brief lte_set_report_scanresult() Change the report setting of the LTE
 * scanning result event
 * The default report setting is disable.
 *
 * @param [in] scanresult_callback: Callback of scanning result. If NULL is set, the report
 * setting is disabled.
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_scanresult(scanresult_report_cb_t scanresult_callback, void *userPriv);

/**
 * @brief lte_set_report_lwm2m_fw_upgrade_evt() Change the report setting of the LTE LWM2M
 * FW upgrade event
 * The default report setting is disable.
 *
 * @param [in] lwm2m_fw_upgrade_evt_callback: Callback of lwm2m fw upgrade event. If NULL is set,
 * the report setting is disabled.
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_lwm2m_fw_upgrade_evt(
    lwm2m_fw_upgrade_evt_report_cb_t lwm2m_fw_upgrade_evt_callback, void *userPriv);

/**
 * @brief lte_set_report_fw_upgrade_evt() Change the report setting of the LTE FW upgrade event
 * The default report setting is disable.
 *
 * @param [in] fw_upgrade_evt_callback: Callback of fw upgrade event. If NULL is set,
 * the report setting is disabled.
 * @param [in] userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */

int32_t lte_set_report_fw_upgrade_evt(fw_upgrade_evt_report_cb_t fw_upgrade_evt_callback,
                                      void *userPriv);

/**
 * @brief Enable notification for timer event
 *
 * @param timerevt_callback: Callback of timer event result.
 * @param tmrevt: Parameters for timer event. The report setting is disabled if
 * tmrevt->preceding_time is 0
 * @param userPriv: User's private data
 *
 * @return On success, 0 is returned. On failure,
 * negative value is returned.
 */
int32_t lte_set_report_timerevt(timerevt_report_cb_t timerevt_callback, lte_settimerevt_t *tmrevt,
                                void *userPriv);

/**
 * @brief Get RAT information
 *
 * @param [out] ratres: RAT information
 * @return On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */
lteresult_e lte_get_rat(lte_get_rat_t *ratres);

/**
 * @brief Set RAT parameter
 *
 * @param [in] rat_config: RAT parameter
 * @return  On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */
lteresult_e lte_set_rat(lte_set_rat_t *rat_config);

/**
 * @brief invoke a FW upgrade command
 *
 * @param [in] fw_upgrade_cmd: FW upgrade command
 * @return  On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */
lteresult_e lte_fw_upgrade_cmd(lte_fw_upgrade_cmd_t *fw_upgrade_cmd);

/**
 * @brief get FW upgrade state
 *
 * @param [out] fw_upgrade_state: FW upgrade state
 * @return  On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */
lteresult_e lte_get_fw_upgrade_state(lte_fw_upgrade_state_t *fw_upgrade_state);

/**
 * @brief Get timing advance information
 *
 * @param [out] timing_adv: timing advance information
 * @return lteresult_e On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */
lteresult_e lte_get_timing_advance(lte_timging_advance_t *timing_adv);

/**
 * @brief Query if IUICC is enabled
 *
 * @param [out] iuicc: IUICC information
 * @return lteresult_e On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */
lteresult_e lte_get_iuicc_status(lte_get_iuicc_status_t *iuicc);

/**
 * @brief Set the functionality of intercepted scan
 *
 * @param intercpt interepted scan parameters.
 * @return lteresult_e On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */
lteresult_e lte_set_intercepted_scan(lte_set_interruptedscan_t *intercpt);

/**
 * @brief Set the pdcp discard timer
 *
 * @param [in] discard_timer: The discard timer that would like to apply to
 * @return lteresult_e On success, LTE_RESULT_OK is returned. On failure,
 * LTE_RESULT_ERR is returned.
 */
lteresult_e lte_set_pdcp_discard_timer(lte_set_pdcp_discard_timer_t discard_timer);
/** @} lte_funcs */

#undef EXTERN
#ifdef __cplusplus
}
#endif

/** @} lte */

#endif /* __MODULES_INCLUDE_LTE_LTE_API_H */
