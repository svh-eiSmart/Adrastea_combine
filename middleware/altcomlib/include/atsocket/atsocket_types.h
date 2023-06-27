/****************************************************************************
 *
 *  (c) copyright 2020 Altair Semiconductor, Ltd. All rights reserved.
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
 * @file atsocket_types.h
 */

#ifndef __ALTCOM_INCLUDE_API_ATSOCKET_TYPES_H
#define __ALTCOM_INCLUDE_API_ATSOCKET_TYPES_H

/**
 * @defgroup socketerror AtSocket Error Code
 * @{
 */

/**
 * @brief Definition of the error code of AtSocket Connector API.
 */
typedef enum {
  SOCKET_OK = 0, /**< Operation success */
  SOCKET_ERR = -1, /**< Operation failure */
} SocketError_e;

/** @} socketerror */

/**
 * @brief Definition of the preference order of IP type for the name resolution.
 */
typedef enum {
  ADDRESS_PRECEDENCE_IPV4V6 = 0, /**< System preference */
  ADDRESS_PRECEDENCE_IPV4 = 1,   /**< IPv4 first */
  ADDRESS_PRECEDENCE_IPV6 = 2,   /**< IPv6 first */
} AddrtypePrecedence_e;

/**
 * @brief Definition of the TLS verification mode.
 */
typedef enum {
  TLS_VERIFY_NONE = 0,     /**< Handshake continues even if verification failed */
  TLS_VERIFY_REQUIRED = 1, /**< Handshake is aborted if verification failed */
} TlsAuthmode_e;

/**
 * @brief Definition of the TLS ciphersuites filtering mode.
 */
typedef enum {
  CIPHERSUITES_FILTERING_NONE = 0,      /**< None */
  CIPHERSUITES_FILTERING_WHITELIST = 0, /**< Leave only selected ciphersuites */
  CIPHERSUITES_FILTERING_BLACKLIST = 1, /**< Remove mentioned ciphersuites */
} CiphersuitesFiltering_e;

/**
 * @brief Definition of the socket type.
 */
typedef enum {
  SOCKET_TYPE_TCP, /**< TCP */
  SOCKET_TYPE_UDP  /**< UDP */
} SocketType_e;

/**
 * @brief Definition of the socket connection type.
 */
typedef enum {
  CONNECT_TYPE_OPEN,  /**< Open */
  CONNECT_TYPE_LISTEN /**< Listen */
} ConnectType_e;

/**
 * @brief Definition of the socket event.
 */
typedef enum {
  SOCKET_EVENT_UNKNOWN = 0,
  SOCKET_EVENT_READABLE = 1, /**< Socket has data to read */
  SOCKET_EVENT_2 = 2,        /**< Reserved */
  SOCKET_EVENT_CLOSED = 3,   /**< Socket has been closed by peer */
  SOCKET_EVENT_ACCEPTED = 4, /**< Socket has been accepted */
} SocketEvent_e;

/**
 * @brief socket identifier
 */
typedef int SocketId_t;

/**
 * @brief Invalid socket identifier
 */
#define INVALID_SOCKET_ID (0)

// By defining ip_addr_t struct as lwip's, we can leverage its utility routines.

#ifndef ATSOCKET_USE_LWIP_IPADDR

#include <stdint.h>

typedef int32_t s32_t;
typedef uint32_t u32_t;
typedef uint8_t u8_t;

typedef struct ip6_addr {
  u32_t addr[4];
} ip6_addr_t;

typedef struct ip4_addr {
  u32_t addr;
} ip4_addr_t;

enum {
  /** IPv4 */
  IPADDR_TYPE_V4 = 0U,
  /** IPv6 */
  IPADDR_TYPE_V6 = 6U,
  /** IPv4+IPv6 ("dual-stack") */
  IPADDR_TYPE_ANY = 46U
};

struct ip_addr {
  union {
    ip6_addr_t ip6;
    ip4_addr_t ip4;
  } u_addr;
  u8_t type;
};

/**
 * @brief IP address
 */
typedef struct ip_addr ip_addr_t;

#else
#include "lwip/ip_addr.h"
#endif

#endif /* __ALTCOM_INCLUDE_API_ATSOCKET_TYPES_H */
