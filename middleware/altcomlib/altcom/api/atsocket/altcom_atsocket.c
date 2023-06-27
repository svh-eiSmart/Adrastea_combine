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

#include <string.h>

#include "dbg_if.h"
#include "apicmdhdlrbs.h"
#include "util/apiutil.h"
#include "altcom_atsocket.h"
#include "apicmd_atsocket.h"

static struct {
  altcom_atsocket_callback_t fn;
  void *arg;
} atsocket_callback = {NULL, NULL};

SocketError_e altcom_atsocket_set_callback(altcom_atsocket_callback_t callback, void *user) {
  if (callback) {
    atsocket_callback.fn = callback;
    atsocket_callback.arg = user;
  } else {
    atsocket_callback.fn = NULL;
    atsocket_callback.arg = NULL;
  }

  return SOCKET_OK;
}

SocketId_t altcom_atsocket_open(uint8_t pdn_session, SocketType_e socket_type, const char *hostname,
                                uint16_t dstport, uint16_t srcport,
                                AddrtypePrecedence_e precedence) {
  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return INVALID_SOCKET_ID;
  }

  struct apicmd_atsocket_allocate_s *cmd = NULL;
  struct apicmd_atsocket_allocate_resp_s *resp = NULL;

  size_t hostnamelen = strlen(hostname) + 1 /* NULL byte */;
  if (hostnamelen > sizeof(cmd->hostname)) {
    DBGIF_LOG2_ERROR("hostname too long: %d/%d\n", hostnamelen, sizeof(cmd->hostname));
    return INVALID_SOCKET_ID;
  }

  size_t sendlen = sizeof(*cmd) + hostnamelen - sizeof(cmd->hostname);

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_ATSOCKET_ALLOCATE, sendlen,
                                          (FAR void **)&resp, sizeof(*resp))) {
    DBGIF_LOG_ERROR("error allocating buffer\n");
    return INVALID_SOCKET_ID;
  }

  cmd->pdn_session = pdn_session;
  cmd->socket_type = socket_type;
  cmd->connect_type = CONNECT_TYPE_OPEN;
  cmd->precedence = precedence;
  cmd->dstport = htons(dstport);
  cmd->srcport = htons(srcport);
  memcpy(cmd->hostname, hostname, hostnamelen);

  uint16_t resplen = 0;
  int32_t err = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)resp, sizeof(*resp), &resplen,
                              ALTCOM_SYS_TIMEO_FEVR);
  if (err < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", err);
  }

  SocketId_t sockid = resp->sockid;

  altcom_generic_free_cmdandresbuff(cmd, resp);

  return sockid;
}

SocketId_t altcom_atsocket_listen(uint8_t pdn_session, SocketType_e socket_type, uint16_t srcport) {
  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return INVALID_SOCKET_ID;
  }

  struct apicmd_atsocket_allocate_s *cmd = NULL;
  struct apicmd_atsocket_allocate_resp_s *resp = NULL;

  size_t sendlen = sizeof(*cmd) + 1 /* NULL byte */ - sizeof(cmd->hostname);

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_ATSOCKET_ALLOCATE, sendlen,
                                          (FAR void **)&resp, sizeof(*resp))) {
    DBGIF_LOG_ERROR("error allocating buffer\n");
    return INVALID_SOCKET_ID;
  }

  cmd->pdn_session = pdn_session;
  cmd->socket_type = socket_type;
  cmd->connect_type = CONNECT_TYPE_LISTEN;
  cmd->dstport = 0;
  cmd->srcport = htons(srcport);
  cmd->hostname[0] = '\0';

  uint16_t resplen = 0;
  int32_t err = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)resp, sizeof(*resp), &resplen,
                              ALTCOM_SYS_TIMEO_FEVR);
  if (err < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", err);
  }

  SocketId_t sockid = resp->sockid;

  altcom_generic_free_cmdandresbuff(cmd, resp);

  return sockid;
}

SocketError_e altcom_atsocket_activate(SocketId_t sockid) {
  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return SOCKET_ERR;
  }

  struct apicmd_atsocket_activate_s *cmd = NULL;
  struct apicmd_atsocket_activate_resp_s *resp = NULL;

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_ATSOCKET_ACTIVATE,
                                          sizeof(*cmd), (FAR void **)&resp, sizeof(*resp))) {
    DBGIF_LOG_ERROR("error allocating buffer\n");
    return SOCKET_ERR;
  }

  cmd->sockid = sockid;

  uint16_t resplen = 0;
  int32_t err = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)resp, sizeof(*resp), &resplen,
                              ALTCOM_SYS_TIMEO_FEVR);
  if (err < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", err);
  }

  err = ntohl(resp->errcode);

  altcom_generic_free_cmdandresbuff(cmd, resp);

  return (SocketError_e)err;
}

SocketError_e altcom_atsocket_deactivate(SocketId_t sockid) {
  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return SOCKET_ERR;
  }

  struct apicmd_atsocket_activate_s *cmd = NULL;
  struct apicmd_atsocket_deactivate_resp_s *resp = NULL;

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_ATSOCKET_DEACTIVATE,
                                          sizeof(*cmd), (FAR void **)&resp, sizeof(*resp))) {
    DBGIF_LOG_ERROR("error allocating buffer\n");
    return SOCKET_ERR;
  }

  cmd->sockid = sockid;

  uint16_t resplen = 0;
  int32_t err = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)resp, sizeof(*resp), &resplen,
                              ALTCOM_SYS_TIMEO_FEVR);
  if (err < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", err);
  }

  err = ntohl(resp->errcode);

  altcom_generic_free_cmdandresbuff(cmd, resp);

  return (SocketError_e)err;
}

ssize_t altcom_atsocket_send(SocketId_t sockid, const void *buf, size_t len) {
  return altcom_atsocket_sendto(sockid, buf, len, NULL, 0);
}

ssize_t altcom_atsocket_sendto(SocketId_t sockid, const void *buf, size_t len,
                               const ip_addr_t *dstaddr, uint16_t dstport) {
  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return SOCKET_ERR;
  }

  if (len > APICMD_ATSOCKET_MAX_DATA_LENGTH) {
    DBGIF_LOG2_WARNING("send data too big: %d/%d\n", len, APICMD_ATSOCKET_MAX_DATA_LENGTH);
    len = APICMD_ATSOCKET_MAX_DATA_LENGTH;
  }

  struct apicmd_atsocket_send_s *cmd = NULL;
  struct apicmd_atsocket_send_resp_s *resp = NULL;

  size_t sendlen = sizeof(*cmd) + len - sizeof(cmd->data);

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_ATSOCKET_SEND, sendlen,
                                          (FAR void **)&resp, sizeof(*resp))) {
    DBGIF_LOG_ERROR("error allocating buffer\n");
    return SOCKET_ERR;
  }

  cmd->sockid = sockid;
  cmd->dstaddr.type = IPADDR_TYPE_ANY;
  if (dstaddr) {
    memcpy(&cmd->dstaddr, dstaddr, sizeof(cmd->dstaddr));
  }
  cmd->dstport = htons((uint16_t)dstport);
  cmd->datalen = htons((uint16_t)len);
  memcpy(cmd->data, buf, len);

  uint16_t resplen = 0;
  int32_t err = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)resp, sizeof(*resp), &resplen,
                              ALTCOM_SYS_TIMEO_FEVR);
  if (err < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", err);
  }

  err = ntohl(resp->errcode);

  altcom_generic_free_cmdandresbuff(cmd, resp);

  return err;
}

ssize_t altcom_atsocket_receive(SocketId_t sockid, void *buf, size_t len, size_t *remain) {
  return altcom_atsocket_receivefrom(sockid, buf, len, remain, NULL, 0);
}

ssize_t altcom_atsocket_receivefrom(SocketId_t sockid, void *buf, size_t len, size_t *remain,
                                    ip_addr_t *srcaddr, uint16_t *srcport) {
  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return SOCKET_ERR;
  }

  if (len > APICMD_ATSOCKET_MAX_DATA_LENGTH) {
    DBGIF_LOG2_WARNING("receive buffer too big: %d/%d\n", len, APICMD_ATSOCKET_MAX_DATA_LENGTH);
    len = APICMD_ATSOCKET_MAX_DATA_LENGTH;
  }

  struct apicmd_atsocket_receive_s *cmd = NULL;
  struct apicmd_atsocket_receive_resp_s *resp = NULL;

  uint16_t resplen = sizeof(*resp) + len - sizeof(resp->data);

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_ATSOCKET_RECEIVE,
                                          sizeof(*cmd), (FAR void **)&resp, resplen)) {
    DBGIF_LOG_ERROR("error allocating buffer\n");
    return SOCKET_ERR;
  }

  cmd->sockid = sockid;
  cmd->datalen = htons((uint16_t)len);

  int32_t err = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)resp, sizeof(*resp), &resplen,
                              ALTCOM_SYS_TIMEO_FEVR);
  if (err < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", err);
  }

  *remain = ntohs(resp->remain);

  if (srcaddr) {
    srcaddr->type = IPADDR_TYPE_ANY;
    if (resp->srcaddr.type == IPADDR_TYPE_V4 || resp->srcaddr.type == IPADDR_TYPE_V6) {
      memcpy(srcaddr, &resp->srcaddr, sizeof(*srcaddr));
    }
  }

  if (srcport) {
    *srcport = ntohs(resp->srcport);
  }

  err = ntohl(resp->errcode);
  if (err > 0) {
    memcpy(buf, resp->data, err);
  }

  altcom_generic_free_cmdandresbuff(cmd, resp);

  return err;
}

SocketError_e altcom_atsocket_delete(SocketId_t sockid) {
  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return SOCKET_ERR;
  }

  struct apicmd_atsocket_delete_s *cmd = NULL;
  struct apicmd_atsocket_delete_resp_s *resp = NULL;

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_ATSOCKET_DELETE, sizeof(*cmd),
                                          (FAR void **)&resp, sizeof(*resp))) {
    DBGIF_LOG_ERROR("error allocating buffer\n");
    return SOCKET_ERR;
  }

  cmd->sockid = sockid;

  uint16_t resplen = 0;
  int32_t err = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)resp, sizeof(*resp), &resplen,
                              ALTCOM_SYS_TIMEO_FEVR);
  if (err < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", err);
  }

  err = ntohl(resp->errcode);

  altcom_generic_free_cmdandresbuff(cmd, resp);

  return (SocketError_e)err;
}

SocketError_e altcom_atsocket_secure(SocketId_t sockid, uint8_t profileid, TlsAuthmode_e authmode,
                                     CiphersuitesFiltering_e filtering, uint16_t *ciphersuites,
                                     size_t ciphersuites_len) {
  /* Check init */
  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return SOCKET_ERR;
  }

  struct apicmd_atsocket_sslalloc_s *cmd = NULL;
  struct apicmd_atsocket_sslalloc_resp_s *resp = NULL;

  size_t sendlen =
      sizeof(*cmd) + ciphersuites_len * sizeof(*ciphersuites) - sizeof(cmd->ciphersuites);

  if (!altcom_generic_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_ATSOCKET_SSLALLOC, sendlen,
                                          (FAR void **)&resp, sizeof(*resp))) {
    DBGIF_LOG_ERROR("error allocating buffer\n");
    return SOCKET_ERR;
  }

  cmd->sockid = sockid;
  cmd->profileid = profileid;
  cmd->authmode = authmode;
  cmd->filtering = filtering;
  cmd->ciphersuites_len = htons(ciphersuites_len);

  for (size_t i = 0; i < ciphersuites_len; i++) {
    cmd->ciphersuites[i] = htons(ciphersuites[i]);
  }

  uint16_t resplen = 0;
  int32_t err = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)resp, sizeof(*resp), &resplen,
                              ALTCOM_SYS_TIMEO_FEVR);
  if (err < 0) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %ld\n", err);
  }

  err = ntohl(resp->errcode);

  altcom_generic_free_cmdandresbuff(cmd, resp);

  return (SocketError_e)err;
}

static void atsocket_callback_job(FAR void *arg) {
  FAR struct apicmd_atsocket_urc_s *evt = (FAR struct apicmd_atsocket_urc_s *)arg;

  SocketId_t sockid = evt->sockid;
  SocketEvent_e event = (SocketEvent_e)evt->event;

  if (event == SOCKET_EVENT_ACCEPTED) {
    sockid = evt->extra;
  }

  altcom_free_cmd((FAR uint8_t *)arg);

  if (atsocket_callback.fn) {
    atsocket_callback.fn(sockid, event, atsocket_callback.arg);
  }
}

enum evthdlrc_e apicmdhdlr_atsocketevt(FAR uint8_t *evt, uint32_t evlen) {
  return apicmdhdlrbs_do_runjob(evt, APICMDID_ATSOCKET_URC, atsocket_callback_job);
}
