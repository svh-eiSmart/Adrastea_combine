/****************************************************************************
 *
 *  (c) copyright 2021 Altair Semiconductor, Ltd. All rights reserved.
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
 * @file altcom_http.h
 */

#ifndef __MODULES_INCLUDE_HTTP_ALTCOM_HTTP_H
#define __MODULES_INCLUDE_HTTP_ALTCOM_HTTP_H

/**
 * @defgroup http HTTP Connector APIs
 * @{
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/**
 * @defgroup httpgeneralparams  General constants
 * @{
 */

#define HTTP_MAX_DATA_LENGTH 3000

/** @} httpgeneralparams */

/****************************************************************************
 * Public Types
 ****************************************************************************/

/**
 * @defgroup httperror HTTP Error Code
 * @{
 */

/**
 *  @brief HTTP connector API error code
 */

typedef enum {
  HTTP_SUCCESS, /**< Operation success */
  HTTP_FAILURE  /**< Operation failure */
} Http_err_code_e;

/** @} httperror */

/**
 * @defgroup httpprofiles HTP available profiles
 * @{
 */

/**
 * @brief Profile ID's
 */
typedef enum {
  HTTP_PROFILE_ID1 = 1, /** Profile number 1 */
  HTTP_PROFILE_ID2,     /** Profile number 2 */
  HTTP_PROFILE_ID3,     /** Profile number 3 */
  HTTP_PROFILE_ID4,     /** Profile number 4 */
  HTTP_PROFILE_ID5,     /** Profile number 5 */
  HTTP_PROFILE_MAX
} Http_profile_id_e;

/** @} httpprofiles */

/**
 * @defgroup http_configuration HTTP configuration parameters
 * @{
 */

/**
 * @defgroup httpnode HTTP NODE configuration
 * @{
 */

/** @brief NODE configurations
 * Destination address is mandatory
 * User and password (Optional = NULL)
 */

typedef enum { PLAIN_TEXT, B64_ENCRYPT } Http_encript_format_e;

typedef struct {
  char *dest_addr; /**< URI or IP address (Mandatory) */
  char *user;      /** Authentication user identification for HTTP server (Optional) */
  char *passwd;    /** Authentication password for HTTP server (Optional) */
  Http_encript_format_e
      encrypt_format; /** User/password format - 0 - plain text , 1 - B64 (Optional) */
} Http_config_node_t;

/** @} httpnode */

/**
 * @defgroup httptls HTTP TLS configuration
 * @{
 */

/**
 * @brief TLS authentication type
 */
typedef enum {
  HTTP_MUTUAL_AUTH, /**< Mutual authentication */
  HTTP_CLIENT_AUTH, /**< Authenticate client side only  */
  HTTP_SERVER_AUTH, /**< Authenticate server side only */
  HTTP_NONE_AUTH    /**< No authentication */
} Http_config_tls_e;

/**
 *  @brief TLS session resumption.
 */
typedef enum {
  HTTP_TLS_RESUMP_SESSION_DISABLE, /**< Resumption session disabled */
  HTTP_TLS_RESUMP_SESSION_ENABLE   /**< Resumption session enabled */
} Http_tls_session_resumption_e;

/**
 *	@brief Cipher suite filtering option.
 */

typedef enum {
  HTTP_CIPHER_WHITE_LIST, /**<  white list, to leave only selected cipher suites*/
  HTTP_CIPHER_BLACK_LIST, /**< black list, to remove mentioned cipher suites */
  HTTP_CIPHER_NONE_LIST   /**< No cipher suites */
} Http_cypher_filtering_type_e;

/**
 *   @brief TLS configurations data
 */
typedef struct {
  Http_config_tls_e authentication_type; /**< TLS certificate authentication mode. */
  uint8_t profile_tls; /**< Predefined DTLS profile configured with  AT%CERTCFG. Profile 0 is
                          reserved. */
  Http_tls_session_resumption_e session_resumption;     /**< Enable TLS session resumption. */
  Http_cypher_filtering_type_e CipherListFilteringType; /**< Enable Cipher filtering type. */

  char *CipherList; /**< Cipher suite list (white or black) as per
                       https://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml
                       definition. All cipher suites in the list are encoded into single string
                       using hexadecimal cipher suite ID separated by ; i.e. C02C;C0AD;C003. The
                       list of permitted values to be inserted into string (refer to IANA site for
                       exact definition). NULL if not used. Limited to 256 char (Optional) */

} Http_config_tls_t;

/** @} httptls */

/**
 * @defgroup httpip HTTP IP configuration
 * @{
 */

/**
 * @brief Preferred IP type for connection to URL
 */

typedef enum {
  HTTP_IPTYPE_V4V6, /** preferred = IPV4 or IPV6 */
  HTTP_IPTYPE_V4,   /** preferred = IPV4 */
  HTTP_IPTYPE_V6,   /** preferred = IPV6 */
} Http_config_ip_e;

/** @brief IP configurations data */
typedef struct {
  int sessionId; /**< Session ID. Numeric PDN identification defined in APN table for specific PDN.
                    A value of 0 defaults to current PDN  */
  Http_config_ip_e
      ip_type; /**< IP type used to configure preferred IP type for connection in URL. */
  uint16_t Destination_port_nb; /**< Destination TCP port number. A 0 value defaults to
                                   current HTTP port number. Range: 1-65535 (Optional) */
  uint16_t Source_port_nb;      /**< Source TCP port number. A 0 value defaults to current HTTP port
                                   number. Range: 1-65535 (Optional) */
} Http_config_ip_t;

/** @} httpip */

/**
 * @defgroup httpformat HTTP FORMAT configuration
 * @{
 */

/**
 * @brief FORMAT configuration
 */

typedef enum {
  RESP_HEADER_PRESENCE_DISABLE = 0, /** Server response is not present in payload for download */
  RESP_HEADER_PRESENCE_ENABLE = 1,  /** Server response is not present in payload for download */
} respHeaderPresent_e;

typedef enum {
  REQ_HEADER_PRESENCE_DISABLE = 0, /** Headers are not part of data (POST) for upload */
  REQ_HEADER_PRESENCE_ENABLE = 1,  /** Headers are part of data (POST) for upload */
} reqHeaderPresent_e;

typedef struct {
  respHeaderPresent_e respHeaderPresent; /** For download, received headers are part of payload */
  reqHeaderPresent_e reqHeaderPresent;   /** For upload , headers are part of payload */
} Http_config_format_t;

/** @} httpformat */

/**
 * @defgroup httptimeout HTTP TIMEOUT configuration
 * @{
 */

/**
 * @brief TIMEOUT configuration
 * server response timeout. The default value is 120 sec (2 min).
 * If server response is not arrived during this time, server timeout error will be reported via
 * URC.
 */

typedef struct {
  uint16_t timeout;
} Http_config_timeout_t;

/** @} httptimeout */

/**
 * @defgroup httpcmd HTTP commands
 * @{
 */

/** @brief HTTP commands */
typedef enum {
  HTTP_CMD_GET = 1, /**< GET    Method */
  HTTP_CMD_DELETE,  /**< DELETE Method */
  HTTP_CMD_PUT,     /**< PUT    Method */
  HTTP_CMD_POST,    /**< POST   Method */
} Http_cmd_method_e;

/** @brief Commands parameters */

/** @brief Command data for PUT/POST/GET/DELETE */
typedef struct {
  Http_profile_id_e profile_id; /**< Up to five concurrent profiles are available */
  Http_cmd_method_e cmd;        /**< Method - Can be PUT, POST */
  uint16_t
      data_len; /**< Payload size in bytes. Maximum = HTTP_MAX_DATA_LENGTH. Apply to PUT and POST */
  uint32_t
      pending_data; /**< Size of more pending data to send chunk-by-chunk for the next transfer */
  char *headers; /**< Passed headers. All headers in the list fit as single string delimited with ;
                    the size of <data_to_send> shall be decreased in such a way that overall data
                    will not exceed 3KB */
  uint8_t *data_to_send; /**< Payload for PUT and POST */
} Http_command_data_t;

/** @} httpcmd */
/** @} http_configuration  */

/**
 * @defgroup httpcallback HTTP Event & Callback
 * @{
 */

/** @brief Event commands type */
typedef enum {
  HTTP_CMD_PUTCONF_EV,  /**< PUT procedure confirmation status */
  HTTP_CMD_POSTCONF_EV, /**< POST procedure confirmation status */
  HTTP_CMD_DELCONF_EV,  /**< DELETE procedure confirmation status */
  HTTP_CMD_GETRCV_EV,   /**< GET procedure data arrival event */
  HTTP_CMD_SESTERM_EV,  /**< Session terminated locally or remotely */
  HTTP_NOT_USED_EV,     /** Not used */
  HTTP_CMD_READCONF_EV, /**< READ procedure for retrieving chunks */

} eHttpEveEnum_e;

/** @brief Status type */
typedef enum {
  STATE_TRANSACT_OK,                      /** Transaction OK */
  STATE_TRANSACT_FAIL,                    /** Transaction failure */
  STATE_SESTERM_REMOTE_TERMINATED,        /** Remote server terminated close the transaction */
  STATE_SESTERM_LOCAL_BUFFER_OVERFLOW,    /** Buffer overflow */
  STATE_SESTERM_LOCAL_TIMEOUT,            /** http client timeout occurs */
  STATE_SESTERM_LOCAL_AUTHENTICATION_FAIL /** Certificate authentication failure */
} eHttpState_e;

/** brief URC Callback data structure for all commands except GET */

typedef enum {
  HTTP_OK = 0,                        /** HTTP Successful operation */
  HTTP_FAIL = 1,                      /** HTTP failure */
  HTTP_ALLOC_FAIL = 2,                /** Memory allocation failure */
  HTTP_CREATE_SOC_FAIL = 3,           /** Socket creation failure */
  HTTP_IP_CONVERT_FAIL = 4,           /** IP conversion failure */
  HTTP_CONNECT_FAIL = 5,              /** Connection failure */
  HTTP_SEND_MSG_FAIL = 6,             /** Message sending failure */
  HTTP_RCV_MSG_FAIL = 7,              /** Message reception failure */
  HTTP_NULL_FAIL = 8,                 /** General failure like files or certificates misses */
  HTTP_DNS_FAILED_QUY_ADDR = 10,      /** external URL not found */
  HTTP_VERSION_NO_SUPPORT = 11,       /** Protocol unknown */
  HTTP_CONTENT_LEN_NOT_FOUND = 12,    /** Content length reported as 0 */
  HTTP_ACC_RANGE_NOT_FOUND = 13,      /** Header has no offset */
  HTTP_TIMEOUT_CONNECTION_FAIL = 14,  /** Connection failure due to timeout */
  HTTPS_PARSE_X509FILE_CRT_FAIL = 16, /** Parsing certificate issue */
  HTTPS_PARSE_KEY_FAIL = 17,          /** Parsing key failure */
  HTTPS_CONNECT_FAIL = 18,            /** Connection rejected */
  HTTPS_BASIC_CONF_FAIL = 19,         /** SSL configuration rejected */
  HTTPS_SSL_CONNECTION_ERROR = 20,    /** SSL connection failure */
  HTTPS_SSL_SETUP_ERROR = 21,         /** SSL setup error */
  HTTPS_SET_HOSTNAME_FAIL = 22,       /** SSL cannot set the hostname */
  HTTPS_SEND_MSG_FAIL = 23,           /** Message sending failure */
  HTTPS_RCV_MSG_FAIL = 24,            /** Message reception failure */
  HTTPS_NOT_FINISHED_DOWNLOAD = 25,   /** Download not completed */

  HTTP_DOWNLOAD_CANCEL_BY_USER = 26, /** Canceled by user @ref altcom_http_abort_profile */
  HTTP_SERVER_RET_STATUS_ERR = 28,   /** Error returned by server */
  HTTP_IN_PROGRESS_SESSION = 29,     /** Session is pending */

  // MBEDTLS errors
  HTTPS_SSL_INTERNAL = 255,              /** Internal error */
  HTTPS_SSL_INVALID_RECORD = 256,        /** SSL invalid record */
  HTTPS_SSL_NO_SHARED_CIPHER = 257,      /** Cipher not shared */
  HTTPS_SSL_NO_CLIENT_CERTIFICATE = 258, /** Client certificate is missing */
  HTTPS_SSL_CERTIFICATE_REQUIRED = 259,  /** Certificate is required */
  HTTPS_SSL_CA_CHAIN_REQUIRED = 260,     /* Certificate chain is required */
  HTTPS_SSL_FATAL_ALERT_MESSAGE = 261,   /** SSL Fatal error */
  HTTPS_SSL_PEER_VERIFY_FAILED = 262,    /** SSL verification failed */
  HTTP_MAX_RET,
} eHttpClientRet_e;

/** brief http_status explanation
 *
 * http_status normally reports standard HTTP response code.
 * However the response code has been expanded in order to track specific issues
 * related to the firmware.
 * In case http_status doesn't report standard HTTP error code,
 * please refer to @ref eHttpClientRet_e
 */

typedef struct {
  unsigned char profileId;   /** Associated profile */
  eHttpEveEnum_e httpEveNum; /** Event id */
  eHttpState_e err_code;     /** Error code */
  unsigned int http_status;  /** HTTP status */
  unsigned int filesize;     /** Returned file size after GET command */
} HttpUrcIe_t;

/** brief URC Callback data structure for session termination
 *
 * http_status possible errors:
 *
    HTTP code
 */

typedef struct {
  unsigned char profileId;   /** Associated profile */
  eHttpEveEnum_e httpEveNum; /** Event id */
  eHttpState_e err_code;     /** Error code */
  unsigned int http_status;  /** HTTP status */
} Http_cb_sesterm_t;

/** brief URC Callback data structure for GET command */

typedef struct {
  Http_profile_id_e profile_id; /**< Associated profile */
  uint8_t *data;                /**< Received payload */
  uint16_t chunkLen;            /**< Chunk length */
  uint32_t pending;             /**< Pending data */
} Http_cb_read_data_t;

/**
 @brief Definition of callback event function upon messages arrival. */
typedef void (*Http_cmd_event_cb)(HttpUrcIe_t *cbdata); /** General callback for HTTP commands */
typedef void (*Http_read_event_cb)(
    Http_cb_read_data_t *cbdata); /** Callback issued for GET comamnd */
typedef void (*Http_sesterm_cb)(
    Http_cb_sesterm_t *cbdata); /** Session termination or failure callback */

/** @} httpcallback */

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/*********** CONFIGURATIONS ****************/

/**
 * @defgroup http_funcs HTTP APIs
 * @{
 */

/**
 @brief altcom_http_clear_profile():
 Clear all previous configuration settings for selected profile_id.
 Stop session.
 Release all resources.

 @param [in] profileId:               Assigned profile between 1 and 5

 @return:                             HTTP_SUCCESS or HTTP_FAILURE.
 */

Http_err_code_e altcom_http_clear_profile(Http_profile_id_e profileId);

/**
 @brief altcom_http_abort_profile():
 Stop session and clear all unread data from incoming buffer for specified profile_id

 @param [in] profileId:         	  Assigned profile between 1 and 5

 @return:                             HTTP_SUCCESS or HTTP_FAILURE.
 */

Http_err_code_e altcom_http_abort_profile(Http_profile_id_e profileId);

/************* Configurations ************/

/**
  @brief altcom_http_node_config()	URI configuration.

  @param [in] profileId:         	Assigned profile between 1 and 5
  @params [in] http_node_config     HTTP URI configuration parameters.

   @return:                        	HTTP_SUCCESS or HTTP_FAILURE.
 */

Http_err_code_e altcom_http_node_config(Http_profile_id_e profile_id,
                                        Http_config_node_t *http_node_config);

/**
  @brief altcom_http_tls_config()	TLS/SSL configuration.

  @param [in] profileId:         	Assigned profile between 1 and 5
  @params [in] http_tls_config      HTTP TLS configuration parameters.

   @return:                        	HTTP_SUCCESS or HTTP_FAILURE.
 */

Http_err_code_e altcom_http_tls_config(Http_profile_id_e profile_id,
                                       Http_config_tls_t *http_tls_config);

/**
  @brief altcom_http_ip_config()    IP configuration.

  @param [in] profileId:         	Assigned profile between 1 and 5
  @params [in] http_ip_config       HTTP IP configuration parameters.

   @return:                        	HTTP_SUCCESS or HTTP_FAILURE.
 */

Http_err_code_e altcom_http_ip_config(Http_profile_id_e profile_id,
                                      Http_config_ip_t *http_ip_config);

/**
  @brief altcom_http_format_config() FORMAT configuration.

  @param [in] profileId:         	 Assigned profile between 1 and 5
  @params [in] http_format_config    HTTP formatting configuration parameters.

   @return:                        	 HTTP_SUCCESS or HTTP_FAILURE.
 */

Http_err_code_e altcom_http_format_config(Http_profile_id_e profile_id,
                                          Http_config_format_t *http_format_config);

/**
  @brief altcom_http_timeout_config()	    TIMEOUT configuration.

  @param [in] profileId:         	        Assigned profile between 1 and 5
  @params [in] http_timeout_config          HTTP server response timeout configuration. Default is
  120 seconds

   @return:                        	        HTTP_SUCCESS or HTTP_FAILURE.
 */

Http_err_code_e altcom_http_timeout_config(Http_profile_id_e profile_id,
                                           Http_config_timeout_t *http_timeout_config);

/************* COMMANDS *****************/

/**
 @brief altcom_http_send_cmd():      Commands to communicate with HTTP .

 @param [in]  profileId:             Assigned profile between 1 and 5
 @param [in]  Http_cmd_command       Communication parameters. See @ref Http_command_data_t.

 @return:                            HTTP_SUCCESS or HTTP_FAILURE.
 */

Http_err_code_e altcom_http_send_cmd(Http_profile_id_e profileId,
                                     Http_command_data_t *Http_cmd_data);

/**
 @brief altcom_http_readData_cmd	 Command to retrieve payload after a GET command

 @param [in]  profileId:             Assigned profile between 1 and 5
 @param [in]  chunklen:              Chunk length. Maximum is HTTP_MAX_DATA_LENGTH

 */

Http_err_code_e altcom_http_readData_cmd(Http_profile_id_e profileId, int chunklen);

/**
 @brief http_EventRegister():        Commands to register URC's callbacks

 @param [in]: callback:              Callback pointer
 @param [in]  profileId:             Assigned profile between 1 and 5
 @param [in]  event_callback:        Event type callback should be registered from.

*/

void http_EventRegister(void *callback, Http_profile_id_e profile_id,
                        eHttpEveEnum_e event_callback);

/** @} http_funcs */

#undef EXTERN
#ifdef __cplusplus
}
#endif

/** @} http */

#endif /* __MODULES_INCLUDE_HTTP_ALTCOM_HTTP_H */
