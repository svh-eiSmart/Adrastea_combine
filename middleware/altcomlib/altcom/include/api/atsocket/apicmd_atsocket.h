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

#ifndef __MODULES_ALTCOM_INCLUDE_API_ATSOCKET_APICMD_ATSOCKET_H
#define __MODULES_ALTCOM_INCLUDE_API_ATSOCKET_APICMD_ATSOCKET_H

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>
#include "apicmd.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define APICMD_ATSOCKET_MAX_DATA_LENGTH (1500)
#define APICMD_ATSOCKET_MAX_DNS_LENGTH (256)
#define APICMD_ATSOCKET_MAX_CIPHERSUITES_LENGTH (256)

/****************************************************************************
 * Public Types
 ****************************************************************************/

begin_packed_struct struct apicmd_atsocket_allocate_s {  // AT%SOCKETCMD="ALLOCATE"
  uint8_t pdn_session;
  uint8_t socket_type;
  uint8_t connect_type;
  uint8_t precedence;
  uint16_t dstport;
  uint16_t srcport;
  uint8_t hostname[APICMD_ATSOCKET_MAX_DNS_LENGTH];
} end_packed_struct;

begin_packed_struct struct apicmd_atsocket_allocate_resp_s {  // AT%SOCKETCMD="ALLOCATE"
  uint8_t sockid;
} end_packed_struct;

begin_packed_struct struct apicmd_atsocket_activate_s {  // AT%SOCKETCMD="ACTIVATE"
  uint8_t sockid;
} end_packed_struct;

begin_packed_struct struct apicmd_atsocket_activate_resp_s {  // AT%SOCKETCMD="ACTIVATE"
  int32_t errcode;
} end_packed_struct;

begin_packed_struct struct apicmd_atsocket_deactivate_s {  // AT%SOCKETCMD="DEACTIVATE"
  uint8_t sockid;
} end_packed_struct;

begin_packed_struct struct apicmd_atsocket_deactivate_resp_s {  // AT%SOCKETCMD="DEACTIVATE"
  int32_t errcode;
} end_packed_struct;

begin_packed_struct struct apicmd_atsocket_delete_s {  // AT%SOCKETCMD="DELETE"
  uint8_t sockid;
} end_packed_struct;

begin_packed_struct struct apicmd_atsocket_delete_resp_s {  // AT%SOCKETCMD="DELETE"
  int32_t errcode;
} end_packed_struct;

begin_packed_struct struct apicmd_atsocket_send_s {  // AT%SOCKETDATA="SEND"
  uint8_t sockid;
  ip_addr_t dstaddr;
  uint16_t dstport;
  uint16_t datalen;
  uint8_t data[APICMD_ATSOCKET_MAX_DATA_LENGTH];
} end_packed_struct;

begin_packed_struct struct apicmd_atsocket_send_resp_s {  // AT%SOCKETDATA="SEND"
  int32_t errcode;
} end_packed_struct;

begin_packed_struct struct apicmd_atsocket_receive_s {  // AT%SOCKETDATA="RECEIVE"
  uint8_t sockid;
  uint16_t datalen;
} end_packed_struct;

begin_packed_struct struct apicmd_atsocket_receive_resp_s {  // AT%SOCKETDATA="RECEIVE"
  uint16_t remain;
  ip_addr_t srcaddr;
  uint16_t srcport;
  int32_t errcode;
  uint8_t data[APICMD_ATSOCKET_MAX_DATA_LENGTH];
} end_packed_struct;

begin_packed_struct struct apicmd_atsocket_sslalloc_s {  // AT%SOCKETCMD="SSLALLOC"
  uint8_t sockid;
  uint8_t profileid;
  uint8_t authmode;
  uint8_t filtering;
  uint16_t ciphersuites_len;
  uint16_t ciphersuites[APICMD_ATSOCKET_MAX_CIPHERSUITES_LENGTH];
} end_packed_struct;

begin_packed_struct struct apicmd_atsocket_sslalloc_resp_s {  // AT%SOCKETCMD="SSLALLOC"
  int32_t errcode;
} end_packed_struct;

begin_packed_struct struct apicmd_atsocket_urc_s {  // %SOCKETEV
  uint8_t sockid;
  uint8_t event;
  uint8_t extra;
} end_packed_struct;

#endif /* __MODULES_ALTCOM_INCLUDE_API_ATSOCKET_APICMD_ATSOCKET_H */
