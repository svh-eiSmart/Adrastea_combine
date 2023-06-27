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
/*
 * unsigned char *data;  instead of  char *data;
 */
/**
 * @file altcom_coap.h
 */

#ifndef __MODULES_INCLUDE_COAP_ALTCOM_COAP_H
#define __MODULES_INCLUDE_COAP_ALTCOM_COAP_H

#define COAP_URC_TOKEN_LENGTH 9
#define COAP_URC_DATA_LENGTH 1024
#define COAP_URC_OPT_VALUE_LENGTH 512
#define COAP_URC_OPT_ID_LENGTH 128

/**
 * @defgroup coap COAP Connector APIs
 * @{
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/
/**
 * @defgroup coap_error COAP Error Code
 * @{
 */

/**
 *  @brief Definition of the error code of COAP connector API.
 */

typedef enum {
  COAP_SUCCESS, /**< Operation success */
  COAP_FAILURE  /**< Operation failure */
} Coap_err_code;

/** @} coap_error */

/**
 * @defgroup coap_configuration COAP configuration parameters
 * @{
 */

/**
 * @brief IP type used to configure preferred IP type for connection in URL use-case:
 */

typedef enum {
  COAP_IPTYPE_V4 = 1 << 0,
  COAP_IPTYPE_V6 = 1 << 1,
  COAP_IPTYPE_V4V6 = COAP_IPTYPE_V4 | COAP_IPTYPE_V6
} Coap_ip_type;

/** @brief IP configurations */

typedef struct {
  char *dest_addr; /**< Destination (server) IP address or URL */
  int *sessionId; /**< Session ID. Numeric PDN identification defined in APN table for specific PDN.
                   * A value of NULL defaults to Data PDN used unless configured differently by
                   * AT%SETROUTE */
  Coap_ip_type ip_type; /**< IP type used to configure preferred IP type for connection in URL.  */
} CoapCfgIp_t;

/**
 *  @brief Mode selection: PSK/Certificate
 */
typedef enum {
  COAP_PSK_MODE,      /**< PSK mode */
  COAP_CERT_MODE,     /**< Certificate mode */
  COAP_UNSEC_MODE = 3 /**< Unsecured mode */
} Coap_dtls_mode_e;

/**
 * @brief DTLS certificate authentication mode. Irrelevant for PSK
 */
typedef enum {
  COAP_MUTUAL_AUTH, /**< Mutual authentication */
  COAP_CLIENT_AUTH, /**< Authenticate client side only  */
  COAP_SERVER_AUTH, /**< Authenticate server side only */
  COAP_NONE_AUTH    /**< No authentication */
} Coap_dtls_auth_mode_e;

/**
 *  @brief DTLS session resumption.
 */
typedef enum {
  COAP_DTLS_RESUMP_SESSION_DISABLE, /**< Resumption session disabled */
  COAP_DTLS_RESUMP_SESSION_ENABLE   /**< Resumption session enabled */
} Coap_dtls_session_resumption_e;

/**
 *	@brief Cipher suite filtering option.
 */

typedef enum {
  COAP_CIPHER_WHITE_LIST, /**<  white list, to leave only selected cipher suites*/
  COAP_CIPHER_BLACK_LIST  /**< black list, to remove mentioned cipher suites */
} Coap_cypher_filtering_type_e;

/** @brief DTLS configuration */

typedef struct {
  unsigned char dtls_profile; /**< Predefined DTLS profile configured with  AT%CERTCFG. Profile 0 is
                                 reserved. */
  Coap_dtls_mode_e dtls_mode; /**< DTLS mode. PSK/CERTIFICATE. */
  Coap_dtls_auth_mode_e auth_mode; /**< DTLS certificate authentication mode. Irrelevant for PSK. */
  Coap_dtls_session_resumption_e session_resumption; /**< Enable DTLS session resumption. */

  Coap_cypher_filtering_type_e *CipherListFilteringType; /**< Cipher suite filtering option to be
  applied to the default list of supported ciphers for negotiation with server. NULL if not used */

  char *CipherList;
  /**< Cipher suite list (white or black) as per
  https://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml definition. All cipher suites
  in the list are encoded into single string using hexadecimal cipher suite ID separated by ;
  i.e. C02C;C0AD;C003. The list of permitted values to be inserted into string (refer to IANA site
  for exact definition). NULL if not used. Limited to 128 char */

} CoapCfgDtls_t;

/**
   @brief Uri options mask for COAP packet.
   Binary mask to add Uri-xxx options into COAP packet.
   Any combination from below may be selected.
 */
typedef enum {
  COAP_URI_HOST = 1,       /**< Uri-Host */
  COAP_URI_PORT = 1 << 1,  /**< Uri-Port */
  COAP_URI_PATH = 1 << 2,  /**< Uri-Path */
  COAP_URI_QUERY = 1 << 3, /**< Uri-Query */
  COAP_URI_ALL = 0xf       /**< all(default) */
} Coap_option_uri_e;

/**
 *	@brief Configuration parameters
 */

typedef struct {
  CoapCfgIp_t coapCfgIp;
  CoapCfgDtls_t coapCfgDtls;

  /** response waiting timeout for: i) Separate confirmable response */
  /** ii) Non-confirmable GET response */
  /* If equal to zero, the MAX_TRANSMIT_SPAN will be used by default. */
  /* Units: sec. Range:
     0: default value which is MAX_TRANSMIT_SPAN
     1 to MAX_TRANSMIT_SPAN  */
  int protocols;

  /**< For OPTIONS */
  Coap_option_uri_e uriMask; /**< See @ref Coap_option_uri_e */
} CoapCfgContext_t;

/**
 *	@brief Option type/value as defined in RFC 7252.
 */
typedef enum {
  COAP_PREDEF_OPTION_IF_MATCH = 1,
  COAP_PREDEF_OPTION_URI_HOST = 3,
  COAP_PREDEF_OPTION_ETAG = 4,
  COAP_PREDEF_OPTION_IF_NONE_MATCH = 5,
  COAP_PREDEF_OPTION_OBSERVE = 6,
  COAP_PREDEF_OPTION_URI_PATH = 7,
  COAP_PREDEF_OPTION_LOCATION_PATH = 8,
  COAP_PREDEF_OPTION_MAX_AGE = 14,
  COAP_PREDEF_OPTION_URI_QUERY = 15,
  COAP_PREDEF_OPTION_LOCATION_QUERY = 20,
  COAP_PREDEF_OPTION_PROXY_QUERY = 35,
  COAP_PREDEF_OPTION_PROXY_SCHEME = 39
} Coap_option_predefined_e;

/** @brief Options type/value

  Provides opportunity to define COAP options for a specified method.
  For URI-xxx options fillings there are 2 alternative mechanisms:
  - If uri (See @ref CoapCmdData_t) is present, it will be decomposed into URI-xxx options in
  accordance with RFC7252, sec 6.4 . For this procedure if some special URI-xxx option exclusive
  zero bit is set in the bitmask (See @ref Coap_option_uri_e), such URI-xxx option will be filtered
  out and not used, even if provided as part @ref CoapCmdData_t.
  - The URI-xxx options can be provided also explicitly in the list of the options type/value as
  described below (@ref CoapCmdOption_type_value_t).
 */

typedef struct {
  Coap_option_predefined_e option_type; /**< Option type */
  char *option_value;                   /**< Option value */
} CoapCmdOption_type_value_t;

/** @brief CoAP request method codes */
typedef enum {
  COAP_CMD_GET = 1, /**< Get Method */
  COAP_CMD_POST,    /**< POST Method */
  COAP_CMD_PUT,     /**< PUT Method */
  COAP_CMD_DELETE,  /**< DELETE Method */
} coap_cmd_method_t;

/** @brief confirmation mode */

typedef enum {
  COAP_CMD_NON_CONFIRMED, /**< Non confirmed */
  COAP_CMD_CONFIRMED      /**< Confirmed */
} Coap_cmd_confirm_e;

/** @brief Sent content format */

typedef enum {
  COAP_CMD_FMT_TEXT_PLAIN,          /**< Text/plain */
  COAP_CMD_FMT_APP_LNK_FORMAT = 40, /**< Application/link-format */
  COAP_CMD_FMT_APP_XML = 41,        /**< Application/xml */
  COAP_CMD_FMT_APP_OCT_STREAM = 42, /**< Application/octet-stream */
  COAP_CMD_FMT_APP_OCT_EXI = 47,    /**< Application/exi */
  COAP_CMD_FMT_APP_OCT_JSON = 50,   /**< Application/json */
} Coap_cmd_sent_content_format_e;

/** @brief Block indication.
 * Indicates that the data in the message is the last block or more blocks are available */

typedef enum {
  COAP_CMD_BLK_LAST_BLOCK, /**< Last block  */
  COAP_CMD_BLK_MORE_BLOCK  /**< More block available */
} Coap_pending_block_e;

/** brief token structure */

typedef struct {
  unsigned char *token;
  unsigned char token_length;
} CoapToken_t;

/** brief Structure for passing command parameters */

typedef struct {
  coap_cmd_method_t cmd;      /**< Method - Can be GET, PUT or POST */
  char *uri;                  /**< URL address or NULL */
  Coap_cmd_confirm_e confirm; /**< Confirmation mode */
  CoapToken_t *token; /**< Token. Mandatory for GET operation with OBSERVE. Optional for others and
             may be NULL */

  Coap_cmd_sent_content_format_e *content; /**< The value of sent content format (Context-format
              (12) option) for POST/PUT or the optional value of expected content format (Accept
              (17) option) for GET as defined in RFC 7252. NULL if not used. */

  unsigned short data_len;       /**< Payload size in bytes. Should be 0 for get */
  unsigned short *blk_size;      /**< applied to blockwise data transfer. */
                                 /** Block size of transferred data(PUT/POST) */
                                 /** to be acknowledged with server(GET / PUT / POST). */
                                 /** Only power - of - two block sizes are acceptable : */
                                 /** 32 - 1024. NULL if unused */
  unsigned int *block_num;       /**< Applied to blockwise data transfer. Block sequence number:
                                    0-1048575. NULL if unused. */
  Coap_pending_block_e *block_m; /**< applied to blockwise data transfer. Indicates that the data
                                   in the message is the last block or more blocks are available.
                                   Always zero for GET:
                                   0 - last block
                                   1 - more blocks available
                                   NULL if unused. */
  CoapCmdOption_type_value_t **optionsArgV; /**< Option by type and value. NULL if unused */
  unsigned char optionsArgc;                /**< How many option_desc pairs are passed */
  unsigned char *data;                      /**< Payload binary for PUT and POST */
} CoapCmdData_t;

/** @} coap_configuration  */

/**
 * @defgroup coapcallback COAP Event & Callback
 * @{
 */

/** @brief Response code of CoAP protocol (RFC 7252) */

typedef enum {
  EV_RESP_CREATED = 201,                    /**< Created */
  EV_RESP_DELETED = 202,                    /**< Deleted */
  EV_RESP_VALID = 203,                      /**< Valid */
  EV_RESP_CHANGED = 204,                    /**< Changed */
  EV_RESP_CONTENT = 205,                    /**< Content */
  EV_RESP_CONTINUE = 231,                   /**< Continue */
  EV_RESP_BAD_REQUEST = 400,                /**< Bad request */
  EV_RESP_UNAUTHORIZED = 401,               /**< Unauthorized */
  EV_RESP_BAD_OPTION = 402,                 /**< Bad option */
  EV_RESP_FORBIDDEN = 403,                  /**< Forbidden */
  EV_RESP_NOT_FOUND = 404,                  /**< Not found */
  EV_RESP_METHOD_NOT_ALLOWED = 405,         /**< Method not allowed */
  EV_RESP_NOT_ACCEPTABLE = 406,             /**< Not acceptable */
  EV_RESP_REQ_ENT_INCOMPLETE = 408,         /**< Request entity incomplete */
  EV_RESP_PRECOND_FAILED = 412,             /**< Precondition failed */
  EV_RESP_REQ_ENTITY_TOO_LARGE = 413,       /**< Request entity too large */
  EV_RESP_UNSUPPORTED_CONTENT_FORMAT = 415, /**< Unsupported content format */
  EV_RESP_INTERNAL_SERVER_ERROR = 500,      /**< Internal server error */
  EV_RESP_NOT_IMPLEMENTED = 501,            /**< Not implemented */
  EV_RESP_BAD_GATEWAY = 502,                /**< Bad gateway */
  EV_RESP_SERVICE_UNAVAILABLE = 503,        /**< Service unavailable */
  EV_RESP_GATEWAY_TIMEOUT = 504,            /**< Gateway timeout */
  EV_RESP_PROXYING_NOT_SUPPORTED = 505      /**< Proxying not supported */
} Coap_Ev_resp_code_e;

/** @brief Event status */

typedef enum {
  COAP_EVENT_OK,  /**< Successful event */
  COAP_EVENT_FAIL /**< failed event */
} Coap_Ev_err_e;

#define COAP_DATA_MAXLEN 2048 + 1

/** brief Structure for URC Callback reply */
typedef struct {
  uint16_t tokenLen;           /**< Token. Mandatory for GET operation with OBSERVE. */
  uint16_t dataLen;            /**< Payload size in bytes. Should be 0 for get */
  uint8_t profileId;           /** Returned profile Id */
  uint8_t status;              /**< Command execution status indication*/
  Coap_Ev_resp_code_e rspCode; /**< Response code of CoAP Protocol*/
  uint8_t content; /**< The value of received Content-format (option 12) as defined in RFC 7252 */
  uint8_t cmd;     /**< Command execution status indication*/
  int blk_m;       /**< applied to blockwise data transfer. Indicates that the data */
                   /** in the message is the last block or more blocks are available. */
                   /** Always zero for GET: 0 - last block  1 - more blocks available */
  int blk_num;     /**< Block number */
  int blk_size;    /**< Block size acknowledged by server*/
  int optionsArgc; /**< How many option_desc pairs are passed */
  char token[COAP_URC_TOKEN_LENGTH];  /**< Token. Mandatory for GET operation with OBSERVE. */
  char payload[COAP_URC_DATA_LENGTH]; /**< binary Payload received*/
  /**< Option by type.
   Format: Option id0 + null + Option id1 + null ....*/
  char opt_value[COAP_URC_OPT_VALUE_LENGTH];
  /**< Option by value.
   Format: Option val0 + null + Option val1 + null ....   */
  char opt_id[COAP_URC_OPT_ID_LENGTH];
} CoapCallBackData_t;

/** @brief Command execution terminated locally due to timeout waiting for the response to be
 * received */

typedef enum {
  EV_CMDTERM_LOCAL_LTE =
      1, /**< Command execution terminated locally due to LTE connectivity lost */
  EV_CMDTERM_LOCAL_TIMEOUT = 2, /**< Command execution terminated locally due to timeout waiting
           for the respond to be received */
  EV_CMDTERM_ICMP = 3,          /** Command execution terminated locally due to ICMP */
  EV_CMDTERM_UNSUPPORTED_OPTION =
      4, /** Command execution terminated locally due to unsupported options */

  /* 256-262 Command execution terminated locally due to DTLS authentication failure */

  EV_CMDTERM_INVALID_SSL_RECORD = 256, /**< An invalid SSL record was received */
  EV_CMDTERM_NO_CYPHERSUITES, /**< The server has no cypher suites in common with the client */
  EV_CMDTERM_CERT_REQUIRED_BY_AUTH_MODE, /**<No client certification received from the client,
                                            but required by the authentication mode. */
  EV_CMDTERM_OWN_CERT_NEEDED,       /**< The own certificate is not set, but needed by the server */
  EV_CMDTERM_NO_CA_CHAIN,           /**< No CA Chain is set, but required to operate */
  EV_CMDTERM_FATAL_ALERT_FROM_PEER, /**< A fatal alert message was received from our peer */
  EV_CMDTERM_VERIF_FAIL_FROM_PEER   /**< Verification of our peer failed */

  /**< TBD add more errors for DTLS-PSK */
} Coap_ev_cmdterm_e;

/** @brief Event selection */
typedef enum {
  COAP_CB_CMDS,    /**< Regular COAP command PUT, POST,DELETE,GET */
  COAP_CB_CMDRST,  /**< Command rejected by COAP server */
  COAP_CB_CMDTERM, /**< Command execution terminated remotely or locally */
} Coap_Ev_type_e;

/**
 @brief Definition of callback event function upon messages arrival. */
typedef void (*coapCmdEvCb)(unsigned char profile, CoapCallBackData_t *cbdata);
typedef void (*coapRstEvCb)(unsigned char profile, char *token);
typedef void (*coapTermEvCb)(unsigned char profile, Coap_ev_cmdterm_e err, char *token);

/** @} coapcallback */

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
 * @defgroup coap_funcs COAP APIs
 * @{
 */

/**
 @brief altcom_coap_clear_profile():  Clear all previous configuration for selected profile.

 @param [in] profileId:               Assigned profile between 1 and 5

 @return:                             COAP_SUCCESS or COAP_FAILURE.
 */
Coap_err_code altcom_coap_clear_profile(unsigned char profileId);

/**
 @brief altcom_coap_abort_profile():  Clear all unread data from incoming buffer for selected
                                      profile.

 @param [in] profileId:         	  Assigned profile between 1 and 5

 @return:                             COAP_SUCCESS or COAP_FAILURE.
 */
Coap_err_code altcom_coap_abort_profile(unsigned char profileId);

/************* COMMANDS *****************/
/**
 @brief altcom_coap_cmd():          Commands to communicate with COAP .

 @param [in]  profileId:            Assigned profile between 1 and 5
 @param [in]  CmdParams:            Communication parameters. See @ref CoapCmdData_t.

 @return:                           COAP_SUCCESS or COAP_FAILURE.
 */
Coap_err_code altcom_coap_cmd(unsigned char profileId, CoapCmdData_t *CmdParams);

/**
  @brief altcom_coap_get_config()	Retrieve opened configuration of a given profile.

  @param [in] profileId:            Assigned profile between 1 and 5.
  @params [out] context:			Retrieved profile configuration.

   @return:                        	COAP_SUCCESS or COAP_FAILURE.
 */
Coap_err_code altcom_coap_get_config(unsigned char profileId, CoapCfgContext_t *context);

/**
  @brief altcom_coap_set_config()	Save opened configuration of a given profile.

  @param [in] profileId:            Assigned profile between 1 and 5.
  @params [in] context:				Profile configuration to save.

   @return:                        	COAP_SUCCESS or COAP_FAILURE.
 */
Coap_err_code altcom_coap_set_config(unsigned char profileId, CoapCfgContext_t *context);

/**
   @brief coap_EventRegister() 		Callback registration

   @param [in] CB:					Callback pointer.
   @param [in] evCb: 				Selected type of callback see @ref Coap_Ev_type_e.
*/

void coap_EventRegister(void *CB, Coap_Ev_type_e evCb);

/** @} coap_funcs */

#undef EXTERN
#ifdef __cplusplus
}
#endif

/** @} coap */

#endif /* __MODULES_INCLUDE_COAP_ALTCOM_COAP_H */
