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
 * @file altcom_mqtt.h
 */

#ifndef __MODULES_INCLUDE_MQTT_ALTCOM_MQTT_H
#define __MODULES_INCLUDE_MQTT_ALTCOM_MQTT_H

/**
 * @defgroup mqtt MQTT Connector APIs
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
 * @defgroup mqttcfg MQTT Configuration Constants
 * @{
 */

#define MQTTCFG_MAX_TOPIC_LEN 256     /**< Maximum length of MQTT topic.*/
#define MQTTCFG_MAX_MSG_LEN 1024      /**< Maximum length of MQTT message.*/
#define MQTTCFG_MAX_URL_LEN 128       /**< Maximum length of the URL of MQTT portal.*/
#define MQTTCFG_MAX_CLIENTID_LEN 32   /**< Maximum length of the MQTT client ID.*/
#define MQTTCFG_MAX_WILLTOPIC_LEN 128 /**< Maximum length of the MQTT client ID.*/
#define MQTTCFG_MAX_WILLMSG_LEN 128   /**< Maximum length of the MQTT client ID.*/

/** @} mqttcfg */

/****************************************************************************
 * Public Types
 ****************************************************************************/
/**
 * @defgroup mqtterror MQTT Error Code
 * @{
 */

/**
 * @brief Definition of the error code of MQTT connector API.
 */

typedef enum {
  MQTT_SUCCESS = 0,  /**< Operation success */
  MQTT_FAILURE = -1, /**< Operation failure */
} MQTTError_e;

/** @} mqtterror */

/**
 * @defgroup mqtthandle MQTT Session Handle
 * @{
 */

/**
 * @brief Definition of the MQTT session handle
 */

typedef int MQTTSession_t;

/** @} mqtthandle */

/**
 * @defgroup mqttstruct MQTT Parameters Structure
 * @{
 */

/**
 * @brief Definition of a node to the MQTT portal.
 */

typedef struct {
  char url[MQTTCFG_MAX_URL_LEN];           /**< Unique ID of MQTT client */
  char clientId[MQTTCFG_MAX_CLIENTID_LEN]; /**< The endpoint URL */
  char username[MQTTCFG_MAX_CLIENTID_LEN]; /**< The username of node (optional)*/
  char password[MQTTCFG_MAX_CLIENTID_LEN]; /**< The password of @ref username (optional)*/
} MQTTParamNodes_t;

/**
 * @brief Enumeration of authentication method of the MQTT client.
 */

typedef enum {
  MQTTLICENT_AUTH_MUTUAL,      /**< Mutually authentication */
  MQTTLICENT_AUTH_CLIENT_ONLY, /**< Client authentication only */
  MQTTLICENT_AUTH_SERVER_ONLY, /**< Server authentication only */
  MQTTLICENT_AUTH_NOAUTH       /**< No authentication */
} MqttClientAuthMode_e;

/**
 * @brief Definition of TLS autthentication parameters of MQTT client.
 */

typedef struct {
  MqttClientAuthMode_e authMode; /**< Refers to @ref MqttClientAuthMode_e */
  unsigned char authContext; /**< Certificate profile Id, 1~10, which configured by AT%CERTCFG */
} MQTTParamTLS_t;

/**
 * @brief Definition of the IP related parameters of MQTT client.
 */

typedef struct {
  unsigned char PdnSessionId; /**< PDN session Id */
  unsigned char ipType;       /**< IPv4v6=0 IPv4=1 IPv6=2 */
  unsigned int destPort;      /**< MQTT destination port */
  unsigned int sourcePort;    /**< MQTT source port */
} MQTTParamIP_t;

/**
 * @brief Definition of the MQTT QoS level.
 */

typedef enum {
  QOS0, /**< 0: at most once */
  QOS1, /**< 1: at least once */
  QOS2  /**< 2: exactly once */
} MQTTQoS_e;

/**
 * @brief Definition of the parameters to the will message of MQTT client.
 */

typedef struct {
  char enable;                       /**< Enable will message */
  MQTTQoS_e qos;                     /**< QoS of will message; see @ref MQTTQoS_e. */
  char retain;                       /**< To retain the will message */
  char topic[MQTTCFG_MAX_TOPIC_LEN]; /**< Topic of will message */
  char msg[MQTTCFG_MAX_WILLMSG_LEN]; /**< Will message body */
} MQTTParamWillMsg_t;

/**
 * @brief Definition of the protocol parameters of MQTT client.
 */

typedef struct {
  char type;                  /**< 0=MQTT 1=MQTT-SN */
  unsigned int keepAliveTime; /**< Keepalive time of MQTT client */
  char cleanSessionFlag;      /**< Flag to clean session */
} MQTTParamProtocol_t;

/**
 * @brief Enumeration of the MQTT session type.
 *
 */
typedef enum {
  MQTT_SESSION_TYPE_MQTT, /**< Standard MQTT session */
  MQTT_SESSION_TYPE_AWS,  /**<  AWS MQTT session */
} MQTTSessionType_e;

/**
 * @brief Definition of the essential parameters to initialize a new MQTT session.
 */

typedef struct {
  MQTTSession_t *existSession;   /**< An exist session handle to update parameters */
  MQTTParamNodes_t nodes;        /**< Node parameter to the MQTT portal */
  MQTTParamTLS_t tls;            /**< TLS Authentication method of the MQTT client */
  MQTTParamIP_t ip;              /**< IP related parameters of MQTT client */
  MQTTParamWillMsg_t willMsg;    /**< Parameters for the will message of MQTT client */
  MQTTParamProtocol_t protocol;  /**< protocol parameters of MQTT client */
  MQTTSessionType_e sessionType; /**< protocol parameters of MQTT client */
} MQTTSessionParams_t;

/** @} mqttstruct */

/**
 * @defgroup mqttcallback MQTT Event & Callback
 * @{
 */

/**
 * @brief Definition of the event code of MQTT connector API.
 */

typedef enum {
  MQTT_EVT_CONCONF,  /**< Notifying event of connect API */
  MQTT_EVT_DISCONF,  /**< Notifying event of disconnect API */
  MQTT_EVT_SUBCONF,  /**< Notifying event of subscribe API */
  MQTT_EVT_UNSCONF,  /**< Notifying event of unsubscribe API */
  MQTT_EVT_PUBCONF,  /**< Notifying event of pubslish API */
  MQTT_EVT_PUBRCV,   /**< Notifying event of incoming message */
  MQTT_EVT_CONNFAIL, /**< Notifying event of connection failure */
  MQTT_EVT_MAX,
} MqttEvent_e;

/**
 * @brief Definition of the message data of MQTT event.
 */

typedef struct MessageData {
  MQTTQoS_e qos;           /**< QoS of the message */
  unsigned char retained;  /**< Is message retained? */
  unsigned char dup;       /**< Is message duplicated? */
  unsigned short id;       /**< Message Id */
  char *topic;             /**< Pointer to topic string(not includes null-terminate) */
  unsigned short topicLen; /**< Length of topic */
  char *message;           /**< Pointer to message string(not includes null-terminate) */
  unsigned short msgLen;   /**< Length of message */
} MessageData;

/**
 * @brief Definition of the event of MQTT connector API.
 */

typedef struct {
  int resultCode;          /**< result code of the event */
  int errorCode;           /**< Error code of the event */
  int messageId;           /**< Message Id of event; only if applicable */
  MessageData messageData; /**< Message data of event */
} MqttResultData_t;

/**
 * @brief Definition of the callback function when messages of the subscribed topics arrival.
 *
 * @param [in] session: The associated session of an arrival mesage.
 * @param [in] evt: The arrival event.
 * @param [in] result: The result data of event.
 * @param [in] userPriv: Pointer to user's private data.
 */

typedef void (*MQTTEvtCbFunc_t)(MQTTSession_t *session, MqttEvent_e evt, MqttResultData_t *result,
                                void *userPriv);

/** @} mqttcallback */

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
 * @defgroup mqtt_funcs MQTT APIs
 * @{
 */

/**
 * @brief altcom_mqttSessionCreate() create a new client session to deal with mqtt portal.
 *
 * @param [in] sessionParam: The session initialize parameters to create a new client session
 * see @ref MQTTSessionParams_t.
 *
 * @return On success, a non-NULL session handle returned, see @ref MQTTSession_t;
 * On failure, NULL returned.
 */

MQTTSession_t *altcom_mqttSessionCreate(MQTTSessionParams_t *sessionParam);

/**
 * @brief altcom_mqttSessionDelete() delete an exist client session.
 *
 * @param [in] session: The target session to be deleted, see @ref MQTTSession_t.
 *
 * @return MQTT_SUCCESS on success;  MQTT_FAILURE on failure.
 */

MQTTError_e altcom_mqttSessionDelete(MQTTSession_t *session);

/**
 * @brief altcom_mqttConnect() connect the client to mqtt portal.
 *
 * @param [in] session: The target session to be connected, see @ref MQTTSession_t.
 * @param [in] callback: The callback function to be called on event arrival, see @ref
 * MQTTEvtCbFunc_t.
 * @param [in] cbParam: User's private parameter on callback.
 *
 * @return MQTT_SUCCESS on success;  MQTT_FAILURE on failure.
 */

MQTTError_e altcom_mqttConnect(MQTTSession_t *session, MQTTEvtCbFunc_t callback, void *cbParam);

/**
 * @brief altcom_mqttDisconnect() disconnect the client from mqtt portal.
 *
 * @param [in] session: The target session to be disconnected, see @ref MQTTSession_t.
 *
 * @return MQTT_SUCCESS on success;  MQTT_FAILURE on failure.
 */

MQTTError_e altcom_mqttDisconnect(MQTTSession_t *session);

/**
 * @brief altcom_mqttSubscribe() subscribe a specific topic to MQTT portal and hookup a notifying
 * callback.
 *
 * @param [in] session: The target session to be subscribed, see @ref MQTTSession_t.
 * @param [in] qos: The qos of subscribing message; see @ref MQTTQoS_e.
 * @param [in] topic: A specific topic to be subscribed(must be null-terminated).
 *
 * @return message Id on success, 0: not in used, 1-65535(MQTT-SN only);  MQTT_FAILURE on failure.
 */

int altcom_mqttSubscribe(MQTTSession_t *session, MQTTQoS_e qos, const char *topic);

/**
 * @brief altcom_mqttUnsubscribe() unsubscribe the specific topic from MQTT portal.
 *
 * @param [in] session: The target session to be unsubscribed, see @ref MQTTSession_t.
 * @param [in] topic: The specific topic to be unsubscribed(must be null-terminated).
 *
 * @return message Id on success, 0: not in used, 1-65535(MQTT-SN only);  MQTT_FAILURE on failure.
 *
 */

int altcom_mqttUnsubscribe(MQTTSession_t *session, const char *topic);

/**
 * @brief altcom_mqttPublish() publish a message on the specific topic to MQTT portal.
 *
 * @param [in] session: The target session to be published, see @ref MQTTSession_t.
 * @param [in] qos: The qos of publishing message; see @ref MQTTQoS_e.
 * @param [in] retain: Need to retain the publishing message.
 * @param [in] topic: The specific topic to be published(must be null-terminated).
 * @param [in] msg: The message to be published(string or binary array)
 * @param [in] msgLen: The length of message to be published
 *
 * @return message Id on success, 0: not in used, 1-65535(MQTT-SN only);  MQTT_FAILURE on failure.
 *
 */

int altcom_mqttPublish(MQTTSession_t *session, MQTTQoS_e qos, unsigned int retain,
                       const char *topic, const char *msg, unsigned short msgLen);

/** @} mqtt_funcs */

#undef EXTERN
#ifdef __cplusplus
}
#endif

/** @} mqtt */

#endif /* __MODULES_LTE_INCLUDE_MQTT_ALTCOM_MQTT_H */
