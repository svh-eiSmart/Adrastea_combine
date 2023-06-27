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
 * @file altcom_atsocket.h
 */

#ifndef __MODULES_INCLUDE_ATSOCKET_ALTCOM_ATSOCKET_H
#define __MODULES_INCLUDE_ATSOCKET_ALTCOM_ATSOCKET_H

#include <stdint.h>

#if defined(__GNUC__)
#include <sys/types.h>
#elif defined(__IAR_SYSTEMS_ICC__)
typedef int ssize_t;
#endif

/**
 * @defgroup atsocket AtSocket Connector APIs
 * @{
 */

#include "atsocket_types.h"

/**
 * @brief altcom_atsocket_open() allocate a socket for opening a connection to a remote peer.
 *
 * @param [in] pdn_session: The PDN on which the socket should be opened.
 * @param [in] socket_type: The socket type; see @ref SocketType_e.
 * @param [in] hostname: The destination to connect.
 * @param [in] dstport: The destination port number.
 * @param [in] srcport: The optional source port number. 0 means auto selection.
 * @param [in] precedence: The precednece of IP type to resolve the hostname. See @ref AddrtypePrecedence_e.
 *
 * @return socket identifier on success, @ref INVALID_SOCKET_ID on failure.
 */
SocketId_t altcom_atsocket_open(uint8_t pdn_session, SocketType_e socket_type, const char *hostname,
                                uint16_t dstport, uint16_t srcport,
                                AddrtypePrecedence_e precedence);

/**
 * @brief altcom_atsocket_listen() allocate a socket for listening to incoming connections.
 *
 * @param [in] pdn_session: The PDN on which the socket should be opened.
 * @param [in] socket_type: The socket type; see @ref SocketType_e.
 * @param [in] srcport: The source port number.
 *
 * @return socket identifier on success, INVALID_SOCKET_ID on failure.
 */
SocketId_t altcom_atsocket_listen(uint8_t pdn_session, SocketType_e socket_type, uint16_t srcport);

/**
 * @brief altcom_atsocket_activate() activate the socket.
 *
 * @param [in] sockid: The socket identifier.
 *
 * @return SOCKET_OK on success, SOCKET_ERR otherwise.
 */
SocketError_e altcom_atsocket_activate(SocketId_t sockid);

/**
 * @brief altcom_atsocket_deactivate() deactivate the socket.
 *
 * @param [in] sockid: The socket identifier.
 *
 * @return SOCKET_OK on success, SOCKET_ERR otherwise.
 */
SocketError_e altcom_atsocket_deactivate(SocketId_t sockid);

/**
 * @brief altcom_atsocket_send() send data on a socket.
 *
 * @param [in] sockid: The socket identifier.
 * @param [in] buf: The data to be sent.
 * @param [in] len: The data length.
 *
 * @return On success, it returns the number of bytes sent.  On error, @ref SocketError_e is
 * returned.
 */
ssize_t altcom_atsocket_send(SocketId_t sockid, const void *buf, size_t len);

/**
 * @brief altcom_atsocket_sendto() send a message on a socket.
 *
 * @param [in] sockid: The socket identifier.
 * @param [in] buf: The message to be sent.
 * @param [in] len: The message length.
 * @param [in] dstaddr: The destination IP address.
 * @param [in] dstport: The destination port number.
 *
 * @return On success, it returns the number of bytes sent.  Otherwise, @ref SocketError_e is
 * returned.
 */
ssize_t altcom_atsocket_sendto(SocketId_t sockid, const void *buf, size_t len,
                               const ip_addr_t *dstaddr, uint16_t dstport);

/**
 * @brief altcom_atsocket_receive() receive data from a socket.
 *
 * @param [in] sockid: The socket identifier.
 * @param [in] buf: The buffer to receive the data.
 * @param [in] len: The buffer length.
 * @param [out] remain: The number of bytes waiting to be received.
 *
 * @return On success, it returns the number of bytes received. Otherwise, @ref SocketError_e is
 * returned.
 */
ssize_t altcom_atsocket_receive(SocketId_t sockid, void *buf, size_t len, size_t *remain);

/**
 * @brief altcom_atsocket_receivefrom() receive a message from a socket.
 *
 * @param [in] sockid: The socket identifier.
 * @param [in] buf: The buffer to receive the data.
 * @param [in] len: The buffer length.
 * @param [out] remain: The number of bytes waiting to be received.
 * @param [out] srcaddr: Optinal buffer. If the underlying protocol provides the source address of
 * the message, that source IP address is placed in srcaddr.
 * @param [out] srcport: Optinal buffer. If the underlying protocol provides the source port number of the message, that source port number is placed in srcport.
 *
 * @return On success, it returns the number of bytes received. Otherwise, @ref SocketError_e is
 * returned.
 */
ssize_t altcom_atsocket_receivefrom(SocketId_t sockid, void *buf, size_t len, size_t *remain,
                                    ip_addr_t *srcaddr, uint16_t *srcport);

/**
 * @brief altcom_atsocket_delete() delete a socket. Deleting an activated socket is allowed. Don't delete a TLS-enabled socket if you want to resume the TLS session.
 *
 * @param [in] sockid: The socket identifier.
 *
 * @return SOCKET_OK on success, SOCKET_ERR otherwise.
 */
SocketError_e altcom_atsocket_delete(SocketId_t sockid);

/**
 * @brief altcom_atsocket_secure() turn a socket into TLS-protected.
 *
 * @param [in] sockid: The socket identifier.
 * @param [in] profileid: The TLS profile id.
 * @param [in] authmod: The verification mode. See @ref TlsAuthmode_e.
 * @param [in] filtering: The TLS ciphersuites filtering mode. See @ref CiphersuitesFiltering_e.
 * @param [in] ciphersuites: The ciphersuites to select or remove.
 * @param [in] ciphersuites_len: The number of ciphersuites.
 *
 * @return SOCKET_OK on success, SOCKET_ERR otherwise.
 */
SocketError_e altcom_atsocket_secure(SocketId_t sockid, uint8_t profileid, TlsAuthmode_e authmode,
                                     CiphersuitesFiltering_e filtering, uint16_t *ciphersuites,
                                     size_t ciphersuites_len);

/**
 * @brief Definition of the callback function when socket events arrived.
 *
 * @param [in] sockid: The socket identifier.
 * @param [in] event: The arrival event.
 * @param [in] user: User data.
 */
typedef void (*altcom_atsocket_callback_t)(SocketId_t sockid, SocketEvent_e event, void *user);

/**
 * @brief altcom_atsocket_set_callback() setup socket event callback.
 *
 * @param [in] callback: The callback function to be called on socket event arrival, see @ref
 * altcom_atsocket_callback_t.
 * @param [in] user: User data on callback.
 *
 * @return SOCKET_OK on success;  SOCKET_ERR on failure.
 */
SocketError_e altcom_atsocket_set_callback(altcom_atsocket_callback_t callback, void *user);

/** @} atsocket */

#endif /* __MODULES_LTE_INCLUDE_ATSOCKET_ALTCOM_ATSOCKET_H */
