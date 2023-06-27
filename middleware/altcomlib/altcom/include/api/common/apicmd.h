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

#ifndef __MODULES_LTE_ALTCOM_INCLUDE_API_COMMON_APICMD_H
#define __MODULES_LTE_ALTCOM_INCLUDE_API_COMMON_APICMD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "altcom_cc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Definition of magic number field */

#define APICMD_MAGICNUMBER (0xFEEDBAC5)

/* Definition of version field */

#define APICMD_VER (0x04)

/* Enumeration of command id field */
enum apiCmdId {
#ifdef __ENABLE_LTE_API__
  /* LTE API commands */
  APICMDID_ATTACH_NET = 0x0001,
  APICMDID_DETACH_NET = 0x0002,
  APICMDID_GET_NETSTAT = 0x0003,
  APICMDID_DATAON = 0x0004,
  APICMDID_DATAOFF = 0x0005,
  APICMDID_GET_DATASTAT = 0x0006,
  APICMDID_GET_DATACONFIG = 0x0007,
  APICMDID_SET_DATACONFIG = 0x0008,
  APICMDID_GET_APNLIST = 0x0009,
  APICMDID_SET_APN = 0x000A,
  APICMDID_GET_VERSION = 0x000B,
  APICMDID_GET_PHONENO = 0x000C,
  APICMDID_GET_IMSI = 0x000D,
  APICMDID_GET_IMEI = 0x000E,
  APICMDID_GET_PINATTR = 0x000F,
  APICMDID_SET_PIN_LOCK = 0x0010,
  APICMDID_SET_PIN_CODE = 0x0011,
  APICMDID_ENTER_PIN = 0x0012,
  APICMDID_GET_LTIME = 0x0013,
  APICMDID_GET_OPERATOR = 0x0014,
  APICMDID_GET_SLPMODE = 0x0015,
  APICMDID_SET_SLPMODE = 0x0016,
  APICMDID_SET_REP_NETSTAT = 0x0017,
  APICMDID_SET_REP_EVT = 0x0018,
  APICMDID_SET_REP_QUALITY = 0x0019,
  APICMDID_SET_REP_CELLINFO = 0x001A,
  APICMDID_REPORT_NETSTAT = 0x001B,
  APICMDID_REPORT_EVT = 0x001C,
  APICMDID_REPORT_QUALITY = 0x001D,
  APICMDID_REPORT_CELLINFO = 0x001E,
  APICMDID_GET_EDRX = 0x001F,
  APICMDID_SET_EDRX = 0x0020,
  APICMDID_GET_PSM = 0x0021,
  APICMDID_SET_PSM = 0x0022,
  APICMDID_PSM_CAMP = 0x0023,
  APICMDID_GET_CELLINFO = 0x0024,
  APICMDID_GET_QUALITY = 0x0025,
  APICMDID_GET_ACTPSM = 0x0026,
  APICMDID_GET_CCID = 0x0027,
  APICMDID_GET_SIMSTAT = 0x0028,
  APICMDID_GET_TIMING_ADVANCE = 0x0029,
  APICMDID_SET_INTERCEPTED_SCAN = 0x002A,
  APICMDID_SET_CFUN = 0x002B,
  APICMDID_SCAN_SCHEME = 0x002C,
  APICMDID_SET_TIMER_EVT = 0x002D,
  APICMDID_REPORT_TIMER_EVT = 0x002E,
  APICMDID_GET_RAT = 0x00A0,
  APICMDID_SET_RAT = 0x00A1,
  APICMDID_FW_UPGRADE_CMD = 0x00A2,
  APICMDID_GET_FW_UPGRADE_STATUS = 0x00A3,
  APICMDID_SET_PDCP_DISCARD_TIMER = 0x00A4,
#endif /* __ENABLE_LTE_API__ */

#ifdef __ENABLE_ATCMD_API__
  APICMDID_ATCMDCONN_SEND = 0x30,
  APICMDID_ATCMDCONN_REGURC = 0x31,
  APICMDID_ATCMDCONN_URCEVT = 0x32,
#endif /* __ENABLE_ATCMD_API__ */

#ifdef __ENABLE_MISC_API__
  APICMDID_MISC_GETACFG = 0x0033,
  APICMDID_MISC_SETACFG = 0x0034,
  APICMDID_MISC_GETTIME = 0x0035,
  APICMDID_MISC_APPLOG = 0x0036,
  APICMDID_MISC_GETRESETINFO = 0x0037,
  APICMDID_MISC_GETMDMTEMP = 0x0038,
#endif

#if defined(__ENABLE_MQTT_API__) || defined(__ENABLE_AWS_API__)
  /* MQTT API commands */
  APICMDID_MQTT_SESSIONCREATE = 0x0040,
  APICMDID_MQTT_SESSIONDELETE = 0x0041,
  APICMDID_MQTT_CONNECT = 0x0042,
  APICMDID_MQTT_DISCONNECT = 0x0043,
  APICMDID_MQTT_SUBSCRIBE = 0x0044,
  APICMDID_MQTT_UNSUBSCRIBE = 0x0045,
  APICMDID_MQTT_PUBLISH = 0x0046,
  APICMDID_MQTT_MESSAGEEVT = 0x0047,
#endif /* defined(__ENABLE_MQTT_API__) || defined(__ENABLE_AWS_API__) */

#ifdef __ENABLE_GPS_API__
  /* GPS API commands */
  APICMDID_GPS_ACTIVATE = 0x0050,
  APICMDID_GPS_SETNMEACFG = 0x0051,
  APICMDID_GPS_IGNSSEVU = 0x0052,
  APICMDID_GPS_IGNSSVER = 0x0053,
  APICMDID_GPS_IGNSSCEP = 0x0054,
  APICMDID_GPS_SETEV = 0x0055,
  APICMDID_GPS_MEMERASE = 0x0056,
#endif /* __ENABLE_GPS_API__ */
#ifdef __ENABLE_CERTMGMT_API__
  APICMDID_READ_CREDENTIAL = 0x0060,
  APICMDID_WRITE_CREDENTIAL = 0x0061,
  APICMDID_DELETE_CREDENTIAL = 0x0062,
  APICMDID_GET_CREDLIST = 0x0063,
  APICMDID_CONFIG_CREDPROFILE = 0x0064,
#endif /* __ENABLE_CERTMGMT_API__ */

#ifdef __ENABLE_SOCKET_API__
  /* SOCKET API commands */
  APICMDID_SOCK_ACCEPT = 0x0080,
  APICMDID_SOCK_BIND = 0x0081,
  APICMDID_SOCK_CLOSE = 0x0082,
  APICMDID_SOCK_CONNECT = 0x0083,
  APICMDID_SOCK_FCNTL = 0x0084,
  APICMDID_SOCK_GETADDRINFO = 0x0085,
  APICMDID_SOCK_GETHOSTBYNAME = 0x0086,
  APICMDID_SOCK_GETHOSTBYNAMER = 0x0087,
  APICMDID_SOCK_GETSOCKNAME = 0x0088,
  APICMDID_SOCK_GETSOCKOPT = 0x0089,
  APICMDID_SOCK_LISTEN = 0x008A,
  APICMDID_SOCK_RECV = 0x008B,
  APICMDID_SOCK_RECVFROM = 0x008C,
  APICMDID_SOCK_SELECT = 0x008D,
  APICMDID_SOCK_SEND = 0x008E,
  APICMDID_SOCK_SENDTO = 0x008F,
  APICMDID_SOCK_SHUTDOWN = 0x0090,
  APICMDID_SOCK_SOCKET = 0x0091,
  APICMDID_SOCK_SETSOCKOPT = 0x0092,
#endif /* __ENABLE_SOCKET_API__ */
#ifdef __ENABLE_LWM2M_API__
  /* LWM2M API commands */
  APICMDID_LWM2M_HOST_ENABLE = 0x00120,
  APICMDID_LWM2M_ENABLE_OPEV_EVENTS = 0x00121,
  APICMDID_LWM2M_REGISTER = 0x00122,
  APICMDID_LWM2M_REGISTER_UPDATE = 0x00123,
  APICMDID_LWM2M_GET_SERVERINFO = 0x00124,
  APICMDID_LWM2M_URC = 0x00125,
  APICMDID_LWM2M_READ_RESP = 0x00126,
  APICMDID_LWM2M_SERVER_RESP = 0x00127,
  APICMDID_LWM2M_NOTIFY = 0x00128,
  APICMDID_LWM2M_BOOTSTRAP_START = 0x00129,
  APICMDID_LWM2M_BOOTSTRAP_DELETE = 0x0012A,
  APICMDID_LWM2M_BOOTSTRAP_DONE = 0x0012B,
  APICMDID_LWM2M_BOOTSTRAP_CREATE = 0x0012C,
  APICMDID_LWM2M_DEREGISTER = 0x0012D,
#endif /* CONFIG_ALTCOM_CAPI_LWM2M */
#ifdef __ENABLE_COAP_API__
  /* COAP API commands */
  APICMDID_COAP_CMDCLEAR = 0x00340,
  APICMDID_COAP_CMDABORT = 0x00341,
  APICMDID_COAP_CMDCONFIG = 0x00342,
  APICMDID_COAP_CMD = 0x00343,
  APICMDID_COAP_CMD_URC = 0x00344,
  APICMDID_COAP_TERM_URC = 0x00345,
  APICMDID_COAP_RST_URC = 0x00346,
#endif /* __ENABLE_COAP_API__ */
#ifdef __ENABLE_ATSOCKET_API__
  /* ATSOCKET API commands */
  APICMDID_ATSOCKET_TEST = 0x00400,
  APICMDID_ATSOCKET_SOCKETCMD = 0x00401,
  APICMDID_ATSOCKET_ALLOCATE = 0x00402,
  APICMDID_ATSOCKET_ACTIVATE = 0x00403,
  APICMDID_ATSOCKET_DEACTIVATE = 0x00404,
  APICMDID_ATSOCKET_DELETE = 0x00405,
  APICMDID_ATSOCKET_SETOPT = 0x00406,
  APICMDID_ATSOCKET_LASTERROR = 0x00407,
  APICMDID_ATSOCKET_FASTSEND = 0x00407,
  APICMDID_ATSOCKET_CONFSEND = 0x00409,
  APICMDID_ATSOCKET_INFO = 0x0040A,
  APICMDID_ATSOCKET_SSLALLOC = 0x0040B,
  APICMDID_ATSOCKET_SSLINFO = 0x0040C,
  APICMDID_ATSOCKET_SSLKEEP = 0x0040D,
  APICMDID_ATSOCKET_SSLDEL = 0x0040E,
  APICMDID_ATSOCKET_SEND = 0x0040F,
  APICMDID_ATSOCKET_RECEIVE = 0x00410,
  APICMDID_ATSOCKET_SOCKETEV = 0x00411,
  APICMDID_ATSOCKET_URC = 0x00412,
#endif /* __ENABLE_ATSOCKET_API__ */


  APICMDID_ECHO = 0xFFFE,
  APICMDID_MAX,
  APICMDID_ERRIND = 0xFFFF
};

/* In the case of a response, set 15th bit of the command ID. */

#define APICMDID_CONVERT_RES(cmdid) (cmdid | 0x8000)

enum apiModuleId {
  APIMODULEID_DEFAULT = 0x0,
#ifdef __ENABLE_LTE_API__
  APIMODULEID_LTE = 0x01,
#endif /* __ENABLE_LTE_API__ */
#ifdef __ENABLE_SOCKET_API__
  APIMODULEID_SOCKET = 0x02,
#endif /* __ENABLE_SOCKET_API__ */
#ifdef __ENABLE_ATCMD_API__
  APIMODULEID_ATCMD = 0x03,
#endif /* __ENABLE_ATCMD_API__ */
#if defined(__ENABLE_MQTT_API__) || defined(__ENABLE_AWS_API__)
  APIMODULEID_MQTT = 0x04,
#endif /* __ENABLE_MQTT_API__ || __ENABLE_AWS_API__ */
#ifdef __ENABLE_GPS_API__
  APIMODULEID_GPS = 0x05,
#endif /* __ENABLE_GPS_API__ */
#ifdef __ENABLE_CERTMGMT_API__
  APIMODULEID_CERTMGMT = 0x06,
#endif /* __ENABLE_CERTMGMT_API__ */
#ifdef __ENABLE_LWM2M_API__
  APIMODULEID_LWM2M = 0x07,
#endif /* __ENABLE_LWM2M_API__ */
#ifdef __ENABLE_MISC_API__
  APIMODULEID_MISC = 0x08,
#endif /* __ENABLE_MISC_API__ */
#ifdef __ENABLE_ATSOCKET_API__
  APIMODULEID_ATSOCKET = 0x09,
#endif /* __ENABLE_MISC_API__ */
#ifdef __ENABLE_COAP_API__
  APIMODULEID_COAP = 0x10,
#endif /* __ENABLE_COAP_API__ */
};

#ifdef __ENABLE_LTE_API__
#define LTEVER_MAJOR 0
#define LTEVER_MINOR 8
#endif /* __ENABLE_LTE_API__ */
#ifdef __ENABLE_SOCKET_API__
#define SOCKETVER_MAJOR 0
#define SOCKETVER_MINOR 1
#endif /* __ENABLE_SOCKET_API__ */
#ifdef __ENABLE_ATCMD_API__
#define ATCMDVER_MAJOR 0
#define ATCMDVER_MINOR 1
#endif /* __ENABLE_ATCMD_API__ */
#if defined(__ENABLE_MQTT_API__) || defined(__ENABLE_AWS_API__)
#define MQTTVER_MAJOR 0
#define MQTTVER_MINOR 1
#endif /* __ENABLE_MQTT_API__ || __ENABLE_AWS_API__ */
#ifdef __ENABLE_COAP_API__
#define COAPVER_MAJOR 0
#define COAPVER_MINOR 1
#endif
#ifdef __ENABLE_GPS_API__
#define GPSVER_MAJOR 0
#define GPSVER_MINOR 1
#endif /* __ENABLE_GPS_API__ */
#ifdef __ENABLE_CERTMGMT_API__
#define CERTMGMTVER_MAJOR 0
#define CERTMGMTVER_MINOR 2
#endif /* __ENABLE_CERTMGMT_API__ */
#ifdef __ENABLE_LWM2M_API__
#define LWM2MVER_MAJOR 0
#define LWM2MVER_MINOR 5
#endif /* __ENABLE_LWM2M_API__ */
#ifdef __ENABLE_MISC_API__
#define MISCVER_MAJOR 0
#define MISCVER_MINOR 4
#endif /* __ENABLE_MISC_API__ */

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * API command Header Format
 * bits    0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
 *         ----------------------------------------------
 *         |            magic number (Higher)           |
 *         ----------------------------------------------
 *         |            magic number (Lower)            |
 *         ----------------------------------------------
 *         |       version       |      sequence id     |
 *         ----------------------------------------------
 *         |                 command id                 |
 *         ----------------------------------------------
 *         |               transaction id               |
 *         ----------------------------------------------
 *         |                 data length                |
 *         ----------------------------------------------
 *         |                  check sum                 |
 *         ----------------------------------------------
 *         |                data checksum               |
 *         ----------------------------------------------
 ****************************************************************************/

begin_packed_struct struct apicmd_cmdhdr_s {
  uint32_t magic;
  uint8_t ver;
  uint8_t seqid;
  uint16_t cmdid;
  uint16_t transid;
  uint16_t dtlen;
  uint16_t chksum;
  uint16_t dtchksum;
} end_packed_struct;

begin_packed_struct struct apicmd_moduleinfo_s {
  uint8_t moduleId;
  uint8_t majorVer;
  uint8_t minorVer;
} end_packed_struct;

#endif /* __MODULES_LTE_ALTCOM_INCLUDE_API_COMMON_APICMD_H */
