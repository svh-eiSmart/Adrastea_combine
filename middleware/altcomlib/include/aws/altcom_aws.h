/****************************************************************************
 * modules/include/aws/altcom_aws.h
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
 * @file altcom_aws.h
 */

#ifndef __MODULES_INCLUDE_AWS_ALTCOM_AWS_H
#define __MODULES_INCLUDE_AWS_ALTCOM_AWS_H

/**
 * @defgroup aws AWS Connector APIs
 * @{
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include "mqtt/altcom_mqtt.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/
/**
 * @defgroup awserror AWS Error Code
 * @{
 */

/**
 * @brief Definition of the error code of AWS connector API
 */

typedef enum {
  AWS_SUCCESS = MQTT_SUCCESS, /**< Operation success */
  AWS_FAILURE = MQTT_FAILURE, /**< Operation failure */
} AWSError_e;

/** @} awserror */

/**
 * @defgroup awsqos AWS QoS Level
 * @{
 */
/**
 * @brief Definition of the AWS QoS level.
 */

typedef enum {
  AWS_QOS0 = QOS0, /**< 0: at most once */
  AWS_QOS1 = QOS1, /**< 1: at least once */
} AWSQoS_e;

/** @} awsqos */

/**
 * @defgroup awscallback AWS Event Callback
 * @{
 */

/**
 * @brief Definition of the AWS message data.
 */

typedef struct AWSMessageData {
  AWSQoS_e qos;            /**< QoS of the message */
  char *topic;             /**< Pointer to topic string(not includes null-terminate) */
  unsigned short topicLen; /**< Length of topic */
  char *message;           /**< Pointer to message string(not includes null-terminate) */
  unsigned short msgLen;   /**< Length of message */
} AWSMessageData;

/**
 * @brief awsMsgCbFunc_t
 * Definition of the callback function when messages of the subscribed topics arrival
 * @param [in] msgData: -The associated topic of an arrival mesage
 * @param [in] userPriv: - Pointer to user's private data
 */

typedef void (*awsMsgCbFunc_t)(AWSMessageData *msgData, void *userPriv);

/**
 * @brief awConnEvtCbFunc_t
 * Definition of the connection failure event callback
 * @param [in] userPriv - Pointer to user's private data
 */

typedef void (*awsConnEvtCbFunc_t)(void *userPriv);

/** @} awscallback */

/**
 * @defgroup awsinit AWS Initialize Paramters
 * @{
 */

/**
 * @brief awsInitParam_t
 * Definition of the essential parameters to initialize aws service
 */

typedef struct awsInitParam_s {
  char *clientId;         /**< Unique ID of MQTT client */
  char *endpointUrl;      /**< The endpoint URL */
  uint16_t certProfileId; /**< The profile ID TLS authentication context configured by AT%CERTCFG */
  uint32_t keepAliveTime; /**< MQTT keep-alive time in seconds, default is 1200 seconds */
  char cleanSessionFlag;  /**< Flag to clean session */
  awsMsgCbFunc_t callback;        /**< Callback of subscribed event notification */
  awsConnEvtCbFunc_t evtCallback; /**< Callback of connection failure notification */
  void *userPriv;                 /**< User private data for event callback */
} awsInitParam_t;

/** @} awsinit */

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
 * @defgroup aws_funcs AWS APIs
 * @{
 */

/**
 * Name: altcom_awsInitialize
 *
 *   altcom_awsInitialize() initialize a client to deal with aws.
 *
 *   @param [in] initParam: The initial parameter to create a new client, include cert profile,
 * endpoint, client ID...etc.
 *
 *   @return AWS_SUCCESS on success;  AWS_FAILURE on fail.
 *
 */

AWSError_e altcom_awsInitialize(awsInitParam_t *initParam);

/**
 * Name: altcom_awsConnect
 *
 *   altcom_awsConnect() connect the client to aws.
 *
 *   @return AWS_SUCCESS on success;  AWS_FAILURE on fail.
 *
 */

AWSError_e altcom_awsConnect(void);

/**
 * Name: altcom_awsDisconnect
 *
 *   altcom_awsConnect() disconnect the client from aws.
 *
 *   @return AWS_SUCCESS on success;  AWS_FAILURE on fail.
 *
 */

AWSError_e altcom_awsDisconnect(void);

/**
 * Name: altcom_awsSubscribe
 *
 *   altcom_awsSubscribe() subscribe a specific topic and hookup a notification callback.
 *
 *   @param [in] qos: The qos of subscribing message; see @ref MQTTQoS_e.
 *   @param [in] topic: A specific topic to be subscribed(must be null-terminated).
 *
 *   @return AWS_SUCCESS on success;  AWS_FAILURE on fail.
 *
 */

AWSError_e altcom_awsSubscribe(AWSQoS_e qos, const char *topic);

/**
 * Name: altcom_awsUnsubscribe
 *
 *   altcom_awsUnsubscribe() unsubscribe the specific topic from aws.
 *
 *   @param [in] topic: A specific topic to be unsubscribed(must be null-terminated).
 *
 *   @return AWS_SUCCESS on success;  AWS_FAILURE on fail.
 *
 */

AWSError_e altcom_awsUnsubscribe(const char *topic);

/**
 * Name: altcom_awsPublish
 *
 *   altcom_awsPublish() publish the specific topic from aws.
 *
 *   @param [in] qos: The qos of publishing message; see @ref MQTTQoS_e.
 *   @param [in] topic: A specific topic to be subscribed(must be null-terminated).
 *   @param [in] msg: The message to be published(must be null-terminated).
 *
 *   @return AWS_SUCCESS on success;  AWS_FAILURE on fail.
 *
 */

AWSError_e altcom_awsPublish(AWSQoS_e qos, const char *topic, const char *msg);

/** @} aws_funcs */

#undef EXTERN
#ifdef __cplusplus
}
#endif

/** @} aws */

#endif /* __MODULES_INCLUDE_AWS_ALTCOM_AWS_H */
