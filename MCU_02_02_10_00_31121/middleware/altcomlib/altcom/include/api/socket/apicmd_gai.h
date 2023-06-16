/****************************************************************************
 * modules/lte/altcom/include/api/socket/apicmd_gai.h
 *
 *   Copyright 2021 Sony Semiconductor Solutions Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Sony Semiconductor Solutions Corporation nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __MODULES_LTE_ALTCOM_INCLUDE_API_SOCKET_APICMD_GAI_H
#define __MODULES_LTE_ALTCOM_INCLUDE_API_SOCKET_APICMD_GAI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "apicmd_getaddrinfo.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This structure discribes the data structure of the API command */

begin_packed_struct struct apicmd_gai_s {
  int32_t callback_id;
  uint8_t session_id;
  uint32_t nodenamelen;
  int8_t nodename[APICMD_GETADDRINFO_NODENAME_MAX_LENGTH];
  uint32_t servnamelen;
  int8_t servname[APICMD_GETADDRINFO_SERVNAME_MAX_LENGTH];
  int32_t hints_flag;
  int32_t ai_flags;
  int32_t ai_family;
  int32_t ai_socktype;
  int32_t ai_protocol;
} end_packed_struct;

begin_packed_struct struct apicmd_gaires_s {
  int32_t callback_id;
  int32_t ret_code;
  uint32_t ai_num;
  struct apicmd_getaddrinfo_ai_s ai[APICMD_GETADDRINFO_RES_AI_COUNT];
} end_packed_struct;

#endif /* __MODULES_LTE_ALTCOM_INCLUDE_API_SOCKET_APICMD_GAI_H */
