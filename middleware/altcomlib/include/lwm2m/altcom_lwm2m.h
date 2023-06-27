/****************************************************************************
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
 * @file altcom_lwm2m.h
 */

#ifndef __MODULES_INCLUDE_LWM2M_ALTCOM_LWM2M_H
#define __MODULES_INCLUDE_LWM2M_ALTCOM_LWM2M_H

#include "altcom_cc.h"
/**
 * @defgroup lm2m LWm2m Connector APIs
 * @{
 */

/****************************************************************************
 * Public Types
 ****************************************************************************/

/**
 * @defgroup lm2m_const LWm2m consts
 * @{
 */

/**
 * @brief
 * Definitions used various LWM2M APIs
 */

#define LWM2M_OBSERVE_TOKEN_STR_LEN 16 /**< Length of token string sent within the observe */
#define MAX_LWM2M_STRING_TYPE_LEN 64   /**< Global limit of string type length */

#define MAX_LWM2M_SERVER_URI 128 /**< Maximum length of server URI*/
#define MAX_LWM2M_SERVER_NUM 5   /**< Maximum number of server */
/** @} lm2m_const */

/**
 * @defgroup LWM2MError LWM2M Error Code
 * @{
 */

/**
 * @brief Definition of the error code of LWM2M connector API
 */

typedef enum {
  LWM2M_SUCCESS = 0,  /**< Operation success */
  LWM2M_FAILURE = -1, /**< Operation failure */
} LWM2MError_e;
/** @} LWM2MError */

/**
 * @defgroup client_type LWM2M Client Insctance Type
 * @{
 */

/**
 * @brief
 * Definition of LWM2M client instance type.
 */

typedef enum {
  CLIENT_INST_OPERATOR = 1, /**< Client instance for operator */
  CLIENT_INST_ALTAIR = 2    /**<  Client instance for Altair cloud service */
} client_inst_t;
/** @} client_type */

/**
 * @defgroup Resouce_Inst_Type LWm2m resource and instance type supported
 * @{
 */

/**
 * @brief
 * Definition of LWM2M resource type supported.
 */

typedef enum {
  RSRC_VALUE_TYPE_FLOAT = 0, /**< Type resource is float */
  RSRC_VALUE_TYPE_INTEGER,   /**< Type resource is integer */
  RSRC_VALUE_TYPE_TIME,      /**< Type resource is time */
  RSRC_VALUE_TYPE_STRING,    /**< Type resource is string */
  RSRC_VALUE_TYPE_BOOLEAN,   /**< Type resource is boolean */
  RSRC_VALUE_TYPE_OPAQUE,    /**< Type resource is opaque */
} LWM2MRsrc_type_e;
/** @} LWM2MRsrc_type */

/**
 * @brief
 * Definition of LWM2M INSTANCE type supported.
 */

typedef enum {
  RSRC_INSTNC_TYPE_SINGLE = 0,  /**< Type object is of single instance */
  RSRC_INSTNC_TYPE_MULTIPLE = 1 /**< Type object is of multiple instance */
} LWM2MInstnc_type_e;
/** @} Resouce_Inst_Type */

/**
 * @defgroup URI_Resource_Data URI, Resource and Data
 * @{
 */

/**
 * @brief Definition of the LWM2M Opaque type
 */

begin_packed_struct struct opaqueVal_T {
  unsigned short opaqueValLen; /**< opaque value length */
  unsigned char *opaqueValPtr; /**< opaque value ptr */
} end_packed_struct;
typedef struct opaqueVal_T opaqueVal_t;

/**
 * @brief Definition of the LWM2M resource type
 */

begin_packed_struct union resourceVal_T {
  double value;                           /**< resource value */
  char strVal[MAX_LWM2M_STRING_TYPE_LEN]; /**< resource string value */
  opaqueVal_t opaqueVal;                  /**< resource opaque value */
} end_packed_struct;
typedef union resourceVal_T resourceVal_t;

/**
 * @brief
 * Definition of an object that includes LWm2M uri and its value.
 * This is used by altcom_lwm2mSendNotify()
 */

typedef struct {
  int32_t objectId;                   /**< object id part of full uri refer to */
  int32_t instanceNum;                /**< instance num part of full uri refer to */
  int32_t resourceNum;                /**< resource num part of full uri refer to */
  int32_t resourceInstanceNum;        /**< resource's instance num part of full uri refer to */
  LWM2MInstnc_type_e LWM2MInstncType; /**< resource type */
  LWM2MRsrc_type_e valType;           /**< value type according to LWM2MRsrc_type_e */
  resourceVal_t resourceVal;          /**< resource value according to resourceVal_t */
} lwm2m_uri_and_data_t;

/**
 * @brief
 * Definition of an object that includes resource number and its value.
 * This is used by altcom_lwm2mSendBootstrapCmdCreate()
 */

begin_packed_struct struct lwm2m_resource_and_data_T {
  int resourceNum; /**< resource num value refer to */
  double value;    /**< value to set resource with (if resource type is not string)*/
  char valType;    /**< value type of the resource */
  char strVal[MAX_LWM2M_STRING_TYPE_LEN]; /**< value to set resource with, if resource type is
                                             string*/
} end_packed_struct;
typedef struct lwm2m_resource_and_data_T lwm2m_resource_and_data_t;
/** @} URI_Resource_Data */

/**
 * @defgroup lwm2m_serverinfo LWM2M Server Information
 * @{
 */

/**
 * @brief
 * Definition of the server binding
 */

typedef enum {
  SERVER_BINDING_UNKNOWN,       /**< Unknown */
  SERVER_BINDING_UDP,           /**< UDP(U) */
  SERVER_BINDING_UDP_QUEUE,     /**< UDP queue mode(UQ) */
  SERVER_BINDING_SMS,           /**< SMS(S) */
  SERVER_BINDING_SMS_QUEUE,     /**< SMS queue mode(SQ) */
  SERVER_BINDING_UDP_SMS,       /**< UDP with SMS(US) */
  SERVER_BINDING_UDP_QUEUE_SMS, /**< UDP queue mode with SMS(UQS) */
} server_binding_t;

/**
 * @brief
 * Definition of the server status
 */

typedef enum {
  SERVER_STAT_NOT_REG_OR_BOOTSTRAP_NOT_STARTED, /**< Not registered or boostrap not started */
  SERVER_STAT_REG_PENDING,                      /**< Registration pending */
  SERVER_STAT_REG_SUCCESS,                      /**< Successfully registered */
  SERVER_STAT_LAST_REG_FAILED,                  /**<last registration failed */
  SERVER_STAT_REG_UPDATE_PENDING,               /**< registration update pending */
  SERVER_STAT_DEREG_PENGING,                    /**< Deregistration pending */
  SERVER_STAT_BOOTSTRAP_HOLD_OFF_TIME,          /**< Bootstrap hold off time */
  SERVER_STAT_BOOTSTRAP_REQ_SENT,               /**< bootstrap request sent */
  SERVER_STAT_BOOTSTRAP_ONGOING,                /**< bootstrap ongoing */
  SERVER_STAT_BOOTSTRAP_DONE,                   /**< bootstrap done */
  SERVER_STAT_BOOTSTRAP_FAILED,                 /**< bootstrap failed */
} server_stat_t;

/**
 * @brief
 * Definition of the single server information
 */

typedef struct {
  int valid;                             /**< Indicate the information validity of this server */
  char server_uri[MAX_LWM2M_SERVER_URI]; /**< Server URI */
  uint32_t server_id;                    /**< Server ID*/
  int32_t liftime;                       /**< The server registration period from the last
                                              registration date in seconds */
  server_binding_t binding;              /**< Binding */
  server_stat_t server_stat;             /**< Server status */
  int64_t last_regdate;                  /**< The UTC time in 10msec units counted
                                              since 00:00:00 on 1 January, 1900 */
} lwm2m_server_t;

/**
 * @brief
 * Definition of the server information, this is used by altcom_lwm2mGetServerInfo()
 */

typedef struct {
  lwm2m_server_t info[MAX_LWM2M_SERVER_NUM]; /**< Server information insctance */
} lwm2m_serverinfo_t;

/** @} lwm2m_serverinfo */

/**
 * @defgroup lwm2m_event LWM2M Events And Callbacks
 * @{
 */

/**
 * @brief
 * Definition of the type of lwm2m events
 */

typedef enum {
  EVENT_OPEV = 0,      /**< OPEV recv event */
  EVENT_READ_REQ,      /**< read req event */
  EVENT_WRITE_REQ,     /**< write req event */
  EVENT_EXE_REQ,       /**< execute req event */
  EVENT_OBSERVE_START, /**< Observe start event */
  EVENT_OBSERVE_STOP,  /**< Observe start event */
} lwm2m_event_type_t;

/**
 * @brief
 * Definition of LWM2M opev types that being used.
 */

typedef enum {
  APICMDID_ENABLE_REGISTER_OPEV = 0x00000001,       /**< enable register req opev notification */
  APICMDID_ENABLE_REGISTERUPDT_OPEV = 0x00000002,   /**< enable register update opev notification */
  APICMDID_ENABLE_NOTIFY_ACK_OPEV = 0x00000004,     /**< enable notify ack opev notification */
  APICMDID_ENABLE_NOTIFY_FAIL_OPEV = 0x00000008,    /**< enable notify fail opev notification */
  APICMDID_ENABLE_NOTIFY_DISCARD_OPEV = 0x00000010, /**< enable notify discrard opev notification */
  APICMDID_ENABLE_DEREGISTER_OPEV = 0x00000020,     /**< enable de-register opev notification */
  APICMDID_ENABLE_ALL_OPEV = 0x0000003F,            /**< enable ALL opev notification */
  APICMDID_ENABLE_LAST_OPEV = 0x00000020            /**< last invalid opev bit mask */
} opev_type_t;
/** @} opev_type */

/**
 * @brief
 * Enumerations of LWM2M OPEv. These are %LWM2MEOPV URC notified by LWM2M MAP client side
 */

typedef enum {
  LWM2MOPEV_EVENT_WRITE = 0,
  LWM2MOPEV_EVENT_EXECUTE,                   // 1
  LWM2MOPEV_EVENT_CREATE,                    // 2
  LWM2MOPEV_EVENT_DELETE,                    // 3
  LWM2MOPEV_EVENT_WRITE_ATTRIBUTES,          // 4
  LWM2MOPEV_EVENT_DISCOVER,                  // 5
  LWM2MOPEV_EVENT_READ,                      // 6
  LWM2MOPEV_EVENT_OBSERVE,                   // 7
  LWM2MOPEV_EVENT_CANCEL_OBSERVATION,        // 8
  LWM2MOPEV_EVENT_OFFLINE,                   // 9
  LWM2MOPEV_EVENT_ONLINE,                    // 10
  LWM2MOPEV_EVENT_NOTIFY,                    // 11
  LWM2MOPEV_EVENT_WAKEUP_SMS,                // 12
  LWM2MOPEV_EVENT_NOTIFY_RSP,                // 13
  LWM2MOPEV_EVENT_CLIENT_ON,                 // 14
  LWM2MOPEV_EVENT_CLIENT_OFF,                // 15
  LWM2MOPEV_EVENT_CONFIRM_NOTIFY_FAIL,       // 16
  LWM2MOPEV_EVENT_SAVED_1,                   // 17
  LWM2MOPEV_EVENT_SAVED_2,                   // 18
  LWM2MOPEV_EVENT_SAVED_3,                   // 19
  LWM2MOPEV_EVENT_BOOTSTRAP_FINISHED,        // 20
  LWM2MOPEV_EVENT_REGISTER_FINISHED,         // 21
  LWM2MOPEV_EVENT_REGISTER_UPDATE_FINISHED,  // 22
  LWM2MOPEV_EVENT_DEREGISTER_FINISHED,       // 23
  LWM2MOPEV_EVENT_NOTIFY_DISCARD,            // 24
  LWM2MOPEV_EVENT_SINGLE_LAST,               // 25
  LWM2MOPEV_EVENT_ALL = 100                  // 100
} lwm2m_opev_type_t;

/**
 * @brief
 * Definition of target observing object structure
 */

typedef struct {
  uint16_t objId;        /**< Object ID */
  uint16_t instId;       /**< Instance ID */
  uint16_t rsrcId;       /**< Resource ID */
  uint16_t rsrcInstId;   /**< Resourc Instance ID */
  uint8_t condValidMask; /**< The bitmask to indicate the following condition valid or not */
  uint32_t minPeriod;    /**< The minimum time in seconds between two notifications */
  uint32_t maxPeriod;    /**< The maximum time in seconds between two notifications */
  double gtCond;         /**< High threshold value of resource for notification */
  double ltCond;         /**< Low threshold value of resource for notification */
  double stepVal;        /**< minimum change value of resource between two notifications */
  char token[LWM2M_OBSERVE_TOKEN_STR_LEN + 1]; /**< Token of CoAP message in observation message */
} lwm2m_observe_info_t;

/**
 * @brief
 * Definition of the LWM2M event structure
 */

typedef struct {
  client_inst_t client;         /**< Source client instance to launch event */
  lwm2m_event_type_t eventType; /**< Event type */
  union {
    lwm2m_opev_type_t opev_sub_evt;   /**< Sub-event type of OPEV */
    lwm2m_observe_info_t observeInfo; /**< Observing information */
  } u;
} lwm2m_event_t;

/**
 * @brief
 * When the LWM2M,  LWM2MOPEV URC is recv it is
 * reported by this function to the application.
 *
 * @param[in] client: target client to operate.
 * @param[in] event: opev event.
 * @param[in] userPriv: User's private data.
 */

typedef void (*lwm2m_opev_report_cb_t)(client_inst_t client, lwm2m_event_t *event, void *userPriv);

/**
 * @brief
 * When the LWM2M,  %LWM2MOBJCMDU: "OBSERVE_START" or "OBSERVE_STOP" URC is recv it is
 * reported by this function to the application.
 *
 * @param[in] client: target client to operate.
 * @param[in] event: observing event.
 * @param[in] userPriv: User's private data.
 */

typedef void (*lwm2m_obsrvev_report_cb_t)(client_inst_t client, lwm2m_event_t *event,
                                          void *userPriv);
/**
 * @brief
 * When the  %LWM2MOBJCMDU: "READ" URC is recv it is
 * reported by this function to the application that return to this CB function the current last
 * value
 *
 * @param[in] client: target client to operate
 * @param[in] objectId: Obejct Id to read(OID).
 * @param[in] instanceNum: Object instance number(IID) to read.
 * @param[in] resourceNum: Resource number(RID) of object to read.
 * @param[inout] resourceInst: Pointer to store the resource instance for read response.
 * @param[out] valType: The type of resource.
 * @param[out] resourceVal: The value of requested resource to be read.
 * @param[in] userPriv: User's private data.
 *
 * @return value 0 for success
 */

typedef int (*lwm2m_object_read_req_cb_t)(client_inst_t client, int objectId, int instanceNum,
                                          int resourceNum, int *resourceInst,
                                          LWM2MRsrc_type_e *valType, resourceVal_t *resourceVal,
                                          void *userPriv);

/**
 * @brief
 *  When the  %LWM2MOBJCMDU: "WRITE" URC is recv it is
 *  reported by this function to the application that return ok OR nok
 *
 * @param[in] client: target client to operate
 * @param[in] objectId: Obejct Id(OID) to be written.
 * @param[in] instanceNum: Object instance number(IID) to be written.
 * @param[in] resourceNum: Resource number(RID) of object to be written.
 * @param[in] resourceInst: The resource instance for write response.
 * @param[in] instncType: The instance type of resource.
 * @param[in] resourceVal: The value of requested resource to be written.0
 * @param[in] userPriv: User's private data.
 *
 * @return value 0 for success
 */

typedef int (*lwm2m_object_write_req_cb_t)(client_inst_t client, int objectId, int instanceNum,
                                           int resourceNum, int resourceInst,
                                           LWM2MInstnc_type_e instncType, char *resourceVal,
                                           uint16_t resourceLen, void *userPriv);
/**
 * @brief
 *  When the  %LWM2MOBJCMDU: "EXE" URC is recv it is
 *  reported by this function to the application that return ok OR nok
 *
 * @param[in] client: target client to operate
 * @param[in] objectId: Obejct Id(OID) to be written.
 * @param[in] instanceNum: Object instance number(IID) to be written.
 * @param[in] resourceNum: Resource number(RID) of object to be written.
 * @param[in] resourceInst: The resource instance for write response.
 * @param[in] param: The parameter of execution request.
 * @param[in] userPriv: User's private data.
 *
 * @return value 0 for success
 */

typedef int (*lwm2m_object_exe_req_cb_t)(client_inst_t client, int objectId, int instanceNum,
                                         int resourceNum, int resourceInst, char *param,
                                         void *userPriv);

/** @} lwm2m_event */
/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * @defgroup lwm2m_funcs LWM2M APIs
 * @{
 */

/**
 * @brief altcom_lwm2mEnableHost() ask MAP lwm2m client to send/get host indications to host/MCU.
 *
 * @param [in] client: Specify the target client instance to operate.
 *
 * @return LWM2MError_e is returned.
 */

LWM2MError_e altcom_lwm2mEnableHost(client_inst_t client);

/**
 * @brief altcom_lwm2mEnableOpev() ask MAP lwm2m client to ENABLE specific sending OPEVs to
 * host/MCU.
 *
 * @param [in] client: Specify the target client instance to operate.
 * @param [in] opevet_bitmask: bitmask of LWM2M opev event to be enable.
 *
 * @return LWM2MError_e is returned.
 */

LWM2MError_e altcom_lwm2mEnableOpev(client_inst_t client, unsigned int opevet_bitmask);

/**
 * @brief altcom_lwm2mRegisterReq() ask MAP lwm2m client to send register req to the server.
 *
 * @param [in] client: Specify the target client instance to operate.
 * @param [in] server_id: server id to register req for.
 *
 * @return LWM2MError_e is returned.
 */

LWM2MError_e altcom_lwm2mRegisterReq(client_inst_t client, unsigned int server_id);

/**
 * @brief altcom_lwm2mDeregisterReq() ask MAP lwm2m client to send de-register req to the server.
 *
 * @param [in] client: Specify the target client instance to operate.
 * @param [in] server_id: server id to register req for.
 *
 * @return LWM2MError_e is returned.
 */

LWM2MError_e altcom_lwm2mDeregisterReq(client_inst_t client, unsigned int server_id);

/**
 * @brief altcom_lwm2mRegisterUpdt() ask MAP lwm2m client to send register update req to the server.
 *
 * @param [in] client: Specify the target client instance to operate.
 * @param [in] server_id: server id to register req for.
 *
 * @return LWM2MError_e is returned.
 */

LWM2MError_e altcom_lwm2mRegisterUpdt(client_inst_t client, unsigned int server_id);

/**
 * @brief altcom_lwm2mSendNotify() ask MAP lwm2m client to send notify with data to the server.
 *
 * @param [in] client: Specify the target client instance to operate.
 * @param [in] token: the LWM2M token string which would be send with the notification/report.
 * @param [in] askForAckFromServer: Determine if notify is sent with request for ack from server.
 * @param [in] numOfUriData: numbers of reports included within the following uriAndData array.
 * @param [in] uriAndData: array of reports to be sent to server.
 *
 * @return LWM2M_SUCCESS on success; LWM2M_FAILURE on fail.
 *
 */
LWM2MError_e altcom_lwm2mSendNotify(client_inst_t client, char *token, bool askForAckFromServer,
                                    unsigned char numOfUriData, lwm2m_uri_and_data_t *uriAndData);

/**
 * @brief altcom_lwm2mSendBootstrapCmdStart() Send to  MAP lwm2m client the
 * APICMDID_LWM2M_BOOTSTRAP_START.
 *
 * @param [in] client: Specify the target client instance to operate.
 * @param [in] factoryBsName: the LWM2M Bootstrap owner name string which would be send with the
 * notification/report.
 *
 * @return LWM2M_SUCCESS on success; LWM2M_FAILURE on fail.
 *
 */
LWM2MError_e altcom_lwm2mSendBootstrapCmdStart(client_inst_t client, char *factoryBsName);

/**
 * @brief altcom_lwm2mSendBootstrapCmdDelete() Send to  MAP lwm2m client the
 * APICMDID_LWM2M_BOOTSTRAP_DELETE.
 *
 * @param [in] client: Specify the target client instance to operate.
 *
 * @return LWM2M_SUCCESS on success; LWM2M_FAILURE on fail.
 *
 */

LWM2MError_e altcom_lwm2mSendBootstrapCmdDelete(client_inst_t client);

/**
 * @brief altcom_lwm2mSendBootstrapCmdDone() Send to  MAP lwm2m client the
 * APICMDID_LWM2M_BOOTSTRAP_DONE.
 *
 * @param [in] client: Specify the target client instance to operate.
 *
 * @return LWM2M_SUCCESS on success; LWM2M_FAILURE on fail.
 *
 */

LWM2MError_e altcom_lwm2mSendBootstrapCmdDone(client_inst_t client);

/**
 * @brief altcom_lwm2mSendBootstrapCmdCreate() Send to  MAP lwm2m client the
 * APICMDID_LWM2M_BOOTSTRAP_CREATE.
 *
 * @param [in] client: Specify the target client instance to operate.
 * @param [in] objectId: object id of uri to send create values for.
 * @param [in] instanceNum: instance Number of uri send create values for.
 * @param [in] numOfResrcNumData: numbers array members that include resources and values included
 * within the following resrcAndData array.
 * @param [in] resrcAndData: array of resources and values to be set at LWM2M client configuration.
 *
 * @return LWM2M_SUCCESS on success; LWM2M_FAILURE on fail.
 *
 */

LWM2MError_e altcom_lwm2mSendBootstrapCmdCreate(client_inst_t client, int objectId,
                                                int32_t instanceNum,
                                                unsigned char numOfResrcNumData,
                                                lwm2m_resource_and_data_t *resrcAndData);

/**
 * @brief altcom_lwm2mGetServerInfo get the server information from MAP side.
 *
 * @param [in] client: Specify the target client instance to operate.
 * @param [out] serverinfo: Server information struct ptr
 *
 * @return LWM2M_SUCCESS on success; LWM2M_FAILURE on fail.
 */

LWM2MError_e altcom_lwm2mGetServerInfo(client_inst_t client, lwm2m_serverinfo_t *serverinfo);

/**
 * @brief altcom_lwm2mGetServerInfo get the server information from MAP side.
 *
 * @param [in] event_type: Callback function to handle the Specify the target client instance to
 * operate.
 * @param [in] callback: Callback function to handle the Specify the target client instance to
 * operate.
 * @param [in] userPriv: Server information struct ptr
 *
 * @return LWM2M_SUCCESS on success; LWM2M_FAILURE on fail.
 */

LWM2MError_e altcom_lwm2mSetEventCallback(lwm2m_event_type_t event_type, void *callback,
                                          void *userPriv);

/** @} lwm2m_funcs */
/** @} lwm2m */
#endif /* __MODULES_INCLUDE_LWM2M_ALTCOM_LWM2M_H */
