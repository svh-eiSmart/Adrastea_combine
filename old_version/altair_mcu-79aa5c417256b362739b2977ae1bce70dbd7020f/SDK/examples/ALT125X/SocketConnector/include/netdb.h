/****************************************************************************
 *
 *   Copyright 2018 Sony Semiconductor Solutions Corporation
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

/* Stub header for socket test */

#ifndef __ALTCOM_TEST_NETDB_H
#define __ALTCOM_TEST_NETDB_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "altcom_netdb.h"
#include "sys/socket.h"

// clang-format off

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* input flags for struct addrinfo */

#define AI_PASSIVE     ALTCOM_AI_PASSIVE
#define AI_CANONNAME   ALTCOM_AI_CANONNAME
#define AI_NUMERICHOST ALTCOM_AI_NUMERICHOST
#define AI_NUMERICSERV ALTCOM_AI_NUMERICSERV
#define AI_V4MAPPED    ALTCOM_AI_V4MAPPED
#define AI_ALL         ALTCOM_AI_ALL
#define AI_ADDRCONFIG  ALTCOM_AI_ADDRCONFIG

/* Errors used by the DNS API functions, h_errno can be one of them */

#define EAI_NONAME     ALTCOM_EAI_NONAME
#define EAI_SERVICE    ALTCOM_EAI_SERVICE
#define EAI_FAIL       ALTCOM_EAI_FAIL
#define EAI_MEMORY     ALTCOM_EAI_MEMORY
#define EAI_FAMILY     ALTCOM_EAI_FAMILY

#define HOST_NOT_FOUND ALTCOM_HOST_NOT_FOUND
#define NO_DATA        ALTCOM_NO_DATA
#define NO_RECOVERY    ALTCOM_NO_RECOVERY
#define TRY_AGAIN      ALTCOM_TRY_AGAIN

// clang-format on

/****************************************************************************
 * Public Types
 ****************************************************************************/

#define hostent altcom_hostent
#define h_addr h_addr_list[0]
#define addrinfo altcom_addrinfo

/* application accessible error code set by the DNS API functions */

#define h_errno altcom_h_errno
// extern int altcom_h_errno;

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: altcom_freeaddrinfo
 *
 * Description:
 *   The altcom_freeaddrinfo() function frees the memory that was allocated
 *   for the dynamically allocated linked list res.
 *
 * Parameters:
 *   res - The res returned by altcom_getaddrinfo()
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

#define freeaddrinfo(res) altcom_freeaddrinfo(res)

/****************************************************************************
 * Name: altcom_getaddrinfo
 *
 * Description:
 *   Given node and service, which identify an Internet host and a service,
 *   altcom_getaddrinfo() returns one or more addrinfo structures, each of
 *   which contains an Internet address that can be specified in a call to
 *   altcom_bind() or altcom_connect(). The altcom_getaddrinfo() is reentrant
 *   and allows programs to eliminate IPv4-versus-IPv6 dependencies.
 *
 * Parameters:
 *   nodename - Specifies either a numerical network address, or a network
 *              hostname, whose network addresses are looked up and resolved
 *   servname - Sets the port in each returned address structure
 *   hints - Points to an addrinfo structure that specifies criteria for
 *           selecting the socket address structures returned in the list
 *           pointed to by res
 *   res - Pointer to the start of the list
 *
 * Returned Value:
 *  altcom_getaddrinfo() returns 0 if it succeeds, or one of the following
 *  nonzero error codes:
 *
 *     ALTCOM_EAI_NONAME
 *     ALTCOM_EAI_SERVICE
 *     ALTCOM_EAI_FAIL
 *     ALTCOM_EAI_MEMORY
 *     ALTCOM_EAI_FAMILY
 *
 ****************************************************************************/

#define getaddrinfo(nodename, servname, hints, res) \
  altcom_getaddrinfo(nodename, servname, hints, res)

/****************************************************************************
 * Name: altcom_gethostbyname
 *
 * Description:
 *   The altcom_gethostbyname() function returns a structure of type hostent
 *   for the given host name. Here name is either a hostname, or an IPv4
 *   address in standard dot notation (as for inet_addr(3)), or an IPv6
 *   address in colon (and possibly dot) notation.
 *
 *   If name is an IPv4 or IPv6 address, no lookup is performed and
 *   altcom_gethostbyname_r() simply copies name into the h_name field
 *   and its struct in_addr equivalent into the h_addr_list[0] field of the
 *   returned hostent structure.
 *
 * Input Parameters:
 *   name - The name of the host to find.
 *
 * Returned Value:
 *   Upon successful completion, this function will return a pointer to a
 *   hostent structure if the requested entry was found, and a null pointer
 *   if the end of the database was reached or the requested entry was not
 *   found.
 *
 *   Upon unsuccessful completion, altcom_gethostbyname() will set h_errno to
 *   indicate the error
 *
 ****************************************************************************/

#define gethostbyname(name) altcom_gethostbyname(name);

/****************************************************************************
 * Name: altcom_gethostbyname_r
 *
 * Description:
 *   The altcom_gethostbyname_r() function returns a structure of type
 *   hostent for the given host name. Here name is either a hostname, or an
 *   IPv4 address in standard dot notation (as for inet_addr(3)), or an IPv6
 *   address in colon (and possibly dot) notation.
 *
 *   If name is an IPv4 or IPv6 address, no lookup is performed and
 *   altcom_gethostbyname_r() simply copies name into the h_name field
 *   and its struct in_addr equivalent into the h_addr_list[0] field of the
 *   returned hostent structure.
 *
 * Input Parameters:
 *   name - The name of the host to find.
 *   ret - Caller provided location to return the host data.
 *   buf - Caller provided buffer to hold string data associated with the
 *     host data.
 *   buflen - The size of the caller-provided buffer
 *   result - Point to the result on success.
 *   h_errnop - There h_errno value returned in the event of a failure.
 *
 * Returned Value:
 *   0 is returned on success, -1 is returned on a failure
 *   with the returned h_errno value provided the reason for the failure.
 *
 ****************************************************************************/

#define gethostbyname_r(name, ret, buf, buflen, result, h_errnop) \
  altcom_gethostbyname_r(name, ret, buf, buflen, result, h_errnop)

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ALTCOM_TEST_NETDB_H */
