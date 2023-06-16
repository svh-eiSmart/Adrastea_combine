/****************************************************************************
 * modules/lte/include/net/altcom_gai.h
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

/**
 * @file altcom_gai.h
 */

#ifndef __MODULES_LTE_INCLUDE_NET_ALTCOM_GAI_H
#define __MODULES_LTE_INCLUDE_NET_ALTCOM_GAI_H

/**
 * @defgroup net NET Connector APIs
 * @{
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "altcom_netdb.h"

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
 * @defgroup dns_funcs DNS APIs
 * @{
 */

/**
 * Name: altcom_getaddrinfo_ext
 *
 *   altcom_getaddrinfo_ext() extends altcom_getaddrinfo() for support of PDN session selection
 *
 *   @param [in] session_id - The numeric value of the session ID defined in the APN setting.
 *              Default PDN is used for session_id = 0
 *   @param [in] nodename - Specifies either a numerical network address, or a network
 *              hostname, whose network addresses are looked up and resolved
 *   @param [in] servname - Sets the port in each returned address structure
 *   @param [in] hints - Points to an addrinfo structure that specifies criteria for
 *           selecting the socket address structures returned in the list
 *           pointed to by res
 *   @param [out] res - Pointer to the start of the list
 *
 * Returned Value:
 *  altcom_getaddrinfo_ext() returns 0 if it succeeds, or one of the following
 *  nonzero error codes:
 *
 *     ALTCOM_EAI_NONAME
 *     ALTCOM_EAI_SERVICE
 *     ALTCOM_EAI_FAIL
 *     ALTCOM_EAI_MEMORY
 *     ALTCOM_EAI_FAMILY
 *
 */

int altcom_getaddrinfo_ext(uint8_t session_id, const char *nodename, const char *servname,
                           const struct altcom_addrinfo *hints, struct altcom_addrinfo **res);

/**
 * Name: altcom_getaddrinfo_ext_cb_t
 *
 *   The definition of the callback function for altcom_getaddrinfo_ext_async()
 *
 *   @param[in] arg - User's private data.
 *   @param[in] status - Whether the query succeeded and, if not, how it failed.
 *             It may have any of the error codes defined in altcom_getaddrinfo_ext()
 *   @param[in] res - Pointer to the start of the address structure
 *
 */
typedef void (*altcom_getaddrinfo_ext_cb_t)(void *arg, int status, struct altcom_addrinfo *res);

/**
 * Name: altcom_getaddrinfo_ext_async
 *
 *   altcom_getaddrinfo_ext_async() operates altcom_getaddrinfo_ext() asynchronously.
 *
 *   @param [in] session_id - The numeric value of the session ID defined in the APN setting.
 *              Default PDN is used for session_id = 0
 *   @param [in] nodename - Specifies either a numerical network address, or a network
 *              hostname, whose network addresses are looked up and resolved
 *   @param [in] servname - Sets the port in each returned address structure
 *   @param [in] hints - Points to an addrinfo structure that specifies criteria for
 *           selecting the socket address structures returned in the list
 *           pointed to by res
 *   @param [in] callback - Callback function that informs the result of altcom_getaddrinfo_ext()
 *   @param [in] arg - For use by caller
 *
 * Returned Value:
 *  altcom_getaddrinfo_ext_async() returns 0 if the request has been successfully submitted,
 *  or any of the error codes defined in altcom_getaddrinfo_ext()
 *
 */
int altcom_getaddrinfo_ext_async(uint8_t session_id, const char *nodename, const char *servname,
                                 const struct altcom_addrinfo *hints,
                                 altcom_getaddrinfo_ext_cb_t callback, void *arg);

/** @} dns_funcs */

#undef EXTERN
#ifdef __cplusplus
}
#endif

/** @} net */

#endif /* __MODULES_LTE_INCLUDE_NET_ALTCOM_GAI_H */
