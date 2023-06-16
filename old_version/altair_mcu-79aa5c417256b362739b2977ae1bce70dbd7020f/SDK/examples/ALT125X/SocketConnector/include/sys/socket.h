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

#ifndef __ALTCOM_TEST_SOCKET_H
#define __ALTCOM_TEST_SOCKET_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "altcom_socket.h"

// clang-format off

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Number of sockets */

#define NSOCKET ALTCOM_NSOCKET

/* Address family */

#define AF_UNSPEC           ALTCOM_AF_UNSPEC
#define AF_INET             ALTCOM_AF_INET
#define AF_INET6            ALTCOM_AF_INET6
#define PF_INET             ALTCOM_PF_INET
#define PF_INET6            ALTCOM_PF_INET6
#define PF_UNSPEC           ALTCOM_PF_UNSPEC

/* Socket protocol type */

#define SOCK_STREAM         ALTCOM_SOCK_STREAM
#define SOCK_DGRAM          ALTCOM_SOCK_DGRAM
#define SOCK_RAW            ALTCOM_SOCK_RAW

/* Protocol */

#define IPPROTO_IP          ALTCOM_IPPROTO_IP
#define IPPROTO_ICMP        ALTCOM_IPPROTO_ICMP
#define IPPROTO_TCP         ALTCOM_IPPROTO_TCP
#define IPPROTO_UDP         ALTCOM_IPPROTO_UDP
#define IPPROTO_IPV6        ALTCOM_IPPROTO_IPV6
#define IPPROTO_ICMPV6      ALTCOM_IPPROTO_ICMPV6
#define IPPROTO_UDPLITE     ALTCOM_IPPROTO_UDPLITE
#define IPPROTO_RAW         ALTCOM_IPPROTO_RAW

/* Flags */

#define MSG_PEEK            ALTCOM_MSG_PEEK
#define MSG_WAITALL         ALTCOM_MSG_WAITALL
#define MSG_OOB             ALTCOM_MSG_OOB
#define MSG_DONTWAIT        ALTCOM_MSG_DONTWAIT
#define MSG_MORE            ALTCOM_MSG_MORE

/* How */

#define SHUT_RD             ALTCOM_SHUT_RD
#define SHUT_WR             ALTCOM_SHUT_WR
#define SHUT_RDWR           ALTCOM_SHUT_RDWR

/* Level */

#define SOL_SOCKET          ALTCOM_SOL_SOCKET

/* Option flags per-socket */

#define SO_REUSEADDR        ALTCOM_SO_REUSEADDR
#define SO_KEEPALIVE        ALTCOM_SO_KEEPALIVE
#define SO_BROADCAST        ALTCOM_SO_BROADCAST

/* Additional options, not kept in so_options */

#define SO_ACCEPTCONN       ALTCOM_SO_ACCEPTCONN
#define SO_LINGER           ALTCOM_SO_LINGER
#define SO_RCVBUF           ALTCOM_SO_RCVBUF
#define SO_SNDTIMEO         ALTCOM_SO_SNDTIMEO
#define SO_RCVTIMEO         ALTCOM_SO_RCVTIMEO
#define SO_ERROR            ALTCOM_SO_ERROR
#define SO_TYPE             ALTCOM_SO_TYPE
#define SO_NO_CHECK         ALTCOM_SO_NO_CHECK

/* Options for level IPPROTO_IP */

#define IP_TOS              ALTCOM_IP_TOS
#define IP_TTL              ALTCOM_IP_TTL

/* Options and types related to multicast membership */

#define IP_ADD_MEMBERSHIP   ALTCOM_IP_ADD_MEMBERSHIP
#define IP_DROP_MEMBERSHIP  ALTCOM_IP_DROP_MEMBERSHIP

/* Options and types for UDP multicast traffic handling */

#define IP_MULTICAST_TTL    ALTCOM_IP_MULTICAST_TTL
#define IP_MULTICAST_IF     ALTCOM_IP_MULTICAST_IF
#define IP_MULTICAST_LOOP   ALTCOM_IP_MULTICAST_LOOP

/* Options for level IPPROTO_TCP */

#define TCP_NODELAY         ALTCOM_TCP_NODELAY
#define TCP_KEEPALIVE       ALTCOM_TCP_KEEPALIVE
#define TCP_KEEPIDLE        ALTCOM_TCP_KEEPIDLE
#define TCP_KEEPINTVL       ALTCOM_TCP_KEEPINTVL
#define TCP_KEEPCNT         ALTCOM_TCP_KEEPCNT

/* Options for level IPPROTO_IPV6 */

#define IPV6_CHECKSUM       ALTCOM_IPV6_CHECKSUM
#define IPV6_V6ONLY         ALTCOM_IPV6_V6ONLY

/* Options for level IPPROTO_UDPLITE */

#define UDPLITE_SEND_CSCOV  ALTCOM_UDPLITE_SEND_CSCOV
#define UDPLITE_RECV_CSCOV  ALTCOM_UDPLITE_RECV_CSCOV

/* Macros for fcntl */

#define GETFL               ALTCOM_GETFL
#define SETFL               ALTCOM_SETFL

#define O_NONBLOCK          ALTCOM_O_NONBLOCK

#if 0 /* Unimplemented in lwip */
#define SO_DEBUG            ALTCOM_SO_DEBUG
#define SO_OOBINLINE        ALTCOM_SO_OOBINLINE
#define SO_SNDBUF           ALTCOM_SO_SNDBUF
#define SO_DONTROUTE        ALTCOM_SO_DONTROUTE
#define SO_RCVLOWAT         ALTCOM_SO_RCVLOWAT
#define SO_SNDLOWAT         ALTCOM_SO_SNDLOWAT
#endif

// clang-format on

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef altcom_sa_family_t sa_family_t;
typedef altcom_socklen_t socklen_t;
typedef altcom_in_port_t in_port_t;

#define sockaddr altcom_sockaddr
#define sockaddr_storage altcom_sockaddr_storage
#define linger altcom_linger

//#define timeval altcom_timeval

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
 * Name: altcom_accept
 *
 * Description:
 *   The altcom_accept function is used with connection-based socket types
 *   (ALTCOM_SOCK_STREAM). It extracts the first
 *   connection request on the queue of pending connections, creates a new
 *   connected socket with mostly the same properties as 'sockfd', and
 *   allocates a new socket descriptor for the socket, which is returned. The
 *   newly created socket is no longer in the listening state. The original
 *   socket 'sockfd' is unaffected by this call.  Per file descriptor flags
 *   are not inherited across an accept.
 *
 *   The 'sockfd' argument is a socket descriptor that has been created with
 *   altcom_socket(), bound to a local address with altcom_bind(), and is
 *   listening for connections after a call to altcom_listen().
 *
 *   On return, the 'addr' structure is filled in with the address of the
 *   connecting entity. The 'addrlen' argument initially contains the size
 *   of the structure pointed to by 'addr'; on return it will contain the
 *   actual length of the address returned.
 *
 *   If no pending connections are present on the queue, and the socket is
 *   not marked as non-blocking, accept blocks the caller until a connection
 *   is present. If the socket is marked non-blocking and no pending
 *   connections are present on the queue, accept returns EAGAIN.
 *
 * Parameters:
 *   sockfd   The listening socket descriptor
 *   addr     Receives the address of the connecting client
 *   addrlen  Input: allocated size of 'addr', Return: returned size of 'addr'
 *
 * Returned Value:
 *  Returns -1 on error. If it succeeds, it returns a non-negative integer
 *  that is a descriptor for the accepted socket.
 *
 ****************************************************************************/

#define accept(sockfd, addr, addrlen) altcom_accept(sockfd, addr, addrlen)

/****************************************************************************
 * Name: altcom_bind
 *
 * Description:
 *   altcom_bind() gives the socket 'sockfd' the local address 'addr'.
 *   'addr' is 'addrlen' bytes long. Traditionally, this is called
 *   "assigning a name to a socket." When a socket is created with socket,
 *   it exists in a name space (address family) but has no name assigned.
 *
 * Parameters:
 *   sockfd   Socket descriptor of the socket to bind
 *   addr     Socket local address
 *   addrlen  Length of 'addr'
 *
 * Returned Value:
 *   0 on success; -1 on error with errno set appropriately
 *
 ****************************************************************************/

#define bind(sockfd, addr, addrlen) altcom_bind(sockfd, addr, addrlen)

/****************************************************************************
 * Name: altcom_close
 *
 * Description:
 *   altcom_close() closes a file descriptor of the socket.
 *
 * Parameters:
 *   sockfd   Socket descriptor to close
 *
 * Returned Value:
 *   0 on success; -1 on error with errno set appropriately.
 *
 ****************************************************************************/

#define close(sockfd) altcom_close(sockfd)

/****************************************************************************
 * Name: altcom_connect
 *
 * Description:
 *   altcom_connect() connects the socket referred to by the file descriptor
 *   'sockfd' to the address specified by 'addr'. The addrlen argument
 *   specifies the size of 'addr'.  The format of the address in 'addr' is
 *   determined by the address space of the socket 'sockfd'.
 *
 *   If the socket 'sockfd' is of type ALTCOM_SOCK_DGRAM then 'addr' is the
 *   address to which datagrams are sent by default, and the only address
 *   from which datagrams are received. If the socket is of type
 *   ALTCOM_SOCK_STREAM this call attempts to make a connection to the socket
 *   that is bound to the address specified by 'addr'.
 *
 *   Generally, connection-based protocol sockets may successfully connect()
 *   only once; connectionless protocol sockets may use connect() multiple
 *   times to change their association.  Connectionless sockets may dissolve
 *   the association by connecting to an address with the sa_family member of
 *   sockaddr set to ALTCOM_AF_UNSPEC.
 *
 * Parameters:
 *   sockfd    Socket descriptor returned by altcom_socket()
 *   addr      Server address (form depends on type of socket)
 *   addrlen   Length of actual 'addr'
 *
 * Returned Value:
 *   0 on success; -1 on error with errno set appropriately
 *
 ****************************************************************************/

#define connect(sockfd, addr, addrlen) altcom_connect(sockfd, addr, addrlen)

/****************************************************************************
 * Name: altcom_fcntl
 *
 * Description:
 *   Performs fcntl operations on socket
 *
 * Input Parameters:
 *   sockfd - Socket descriptor of the socket to operate on
 *   cmd    - The fcntl command.
 *   val    - Command-specific arguments
 *
 * Returned Value:
 *   0 is returned on success; -1 is returned on failure and
 *   the errno value is set appropriately.
 *
 ****************************************************************************/

#define fcntl(sockfd, cmd, val) altcom_fcntl(sockfd, cmd, val)

/****************************************************************************
 * Name: altcom_getsockname
 *
 * Description:
 *   The altcom_getsockname() function retrieves the locally-bound name of the
 *   specified socket, stores this address in the sockaddr structure pointed
 *   to by the 'addr' argument, and stores the length of this address in the
 *   object pointed to by the 'addrlen' argument.
 *
 *   If the actual length of the address is greater than the length of the
 *   supplied sockaddr structure, the stored address will be truncated.
 *
 *   If the socket has not been bound to a local name, the value stored in
 *   the object pointed to by address is unspecified.
 *
 * Parameters:
 *   sockfd   Socket descriptor of socket [in]
 *   addr     sockaddr structure to receive data [out]
 *   addrlen  Length of sockaddr structure [in/out]
 *
 * Returned Value:
 *   On success, 0 is returned, the 'addr' argument points to the address
 *   of the socket, and the 'addrlen' argument points to the length of the
 *   address. Otherwise, -1 is returned and errno is set to indicate the error.
 *
 ****************************************************************************/

#define getsockname(sockfd, addr, addrlen) altcom_getsockname(sockfd, addr, addrlen)

/****************************************************************************
 * Name: altcom_getsockopt
 *
 * Description:
 *   altcom_getsockopt() retrieve thse value for the option specified by the
 *   'option' argument for the socket specified by the 'sockfd' argument. If
 *   the size of the option value is greater than 'value_len', the value
 *   stored in the object pointed to by the 'value' argument will be silently
 *   truncated. Otherwise, the length pointed to by the 'value_len' argument
 *   will be modified to indicate the actual length of the'value'.
 *
 *   The 'level' argument specifies the protocol level of the option. To
 *   retrieve options at the socket level, specify the level argument as
 *   ALTCOM_SOL_SOCKET.
 *
 * Parameters:
 *   sockfd    Socket descriptor of socket
 *   level     Protocol level to set the option
 *   option    identifies the option to get
 *   value     Points to the argument value
 *   value_len The length of the argument value
 *
 * Returned Value:
 *   0 on success; -1 on error with errno set appropriately
 *
 ****************************************************************************/

#define getsockopt(sockfd, level, option, value, value_len) \
  altcom_getsockopt(sockfd, level, option, value, value_len)

/****************************************************************************
 * Name: altcom_ioctl
 *
 * Description:
 *   Perform device specific operations.
 *
 * Parameters:
 *   sockfd   Socket descriptor of device
 *   req      The ioctl command
 *   argp     A third argument of type unsigned long is expected
 *
 * Return:
 *   >=0 on success (positive non-zero values are cmd-specific)
 *   -1 on failure with errno set properly.
 *
 ****************************************************************************/

#define ioctl(sockfd, req, argp) altcom_ioctl(sockfd, req, argp)

/****************************************************************************
 * Name: altcom_listen
 *
 * Description:
 *   To accept connections, a socket is first created with altcom_socket(), a
 *   willingness to accept incoming connections and a queue limit for incoming
 *   connections are specified with listen(), and then the connections are
 *   accepted with altcom_accept(). The altcom_listen() call applies only to
 *   sockets of type ALTCOM_SOCK_STREAM.
 *
 * Parameters:
 *   sockfd   Socket descriptor of the bound socket
 *   backlog  The maximum length the queue of pending connections may grow.
 *            If a connection request arrives with the queue full, the client
 *            may receive an error with an indication of ECONNREFUSED or,
 *            if the underlying protocol supports retransmission, the request
 *            may be ignored so that retries succeed.
 *
 * Returned Value:
 *   On success, zero is returned. On error, -1 is returned, and errno is set
 *   appropriately.
 *
 ****************************************************************************/

#define listen(sockfd, backlog) altcom_listen(sockfd, backlog)

/****************************************************************************
 * Name: altcom_read
 *
 * Description:
 *   The altcom_read() is the same as altcom_recv() with the flags parameter
 *   set to zero.
 *
 * Parameters:
 *   sockfd   File structure instance
 *   buf      Buffer to receive data
 *   len      Length of buffer
 *
 * Return:
 *   On success, returns the number of characters received. On error,
 *   -1 is returned, and errno is set appropriately.
 *
 ****************************************************************************/

#define read(sockfd, buf, len) altcom_read(sockfd, buf, len);

/****************************************************************************
 * Name: altcom_recv
 *
 * Description:
 *   The altcom_recv() call is identical to altcom_recvfrom() with a NULL
 *   from parameter.
 *
 * Parameters:
 *   sockfd   Socket descriptor of socket
 *   buf      Buffer to receive data
 *   len      Length of buffer
 *   flags    Receive flags
 *
 * Returned Value:
 *  (see recvfrom)
 *
 ****************************************************************************/

#define recv(sockfd, buf, len, flags) altcom_recv(sockfd, buf, len, flags)

/****************************************************************************
 * Name: altcom_recvfrom
 *
 * Description:
 *   altcom_recvfrom() receives messages from a socket, and may be used to
 *   receive data on a socket whether or not it is connection-oriented.
 *
 *   If from is not NULL, and the underlying protocol provides the source
 *   address, this source address is filled in. The argument fromlen
 *   initialized to the size of the buffer associated with from, and modified
 *   on return to indicate the actual size of the address stored there.
 *
 * Parameters:
 *   sockfd   Socket descriptor of socket
 *   buf      Buffer to receive data
 *   len      Length of buffer
 *   flags    Receive flags
 *   from     Address of source (may be NULL)
 *   fromlen  The length of the address structure
 *
 * Returned Value:
 *   On success, returns the number of characters received. On error,
 *   -1 is returned, and errno is set appropriately.
 *
 ****************************************************************************/

#define recvfrom(sockfd, buf, len, flags, from, fromlen) \
  altcom_recvfrom(sockfd, buf, len, flags, from, fromlen)

/****************************************************************************
 * Name: altcom_send
 *
 * Description:
 *   The altcom_send() call may be used only when the socket is in
 *   a connected state (so that the intended recipient is known). The only
 *   difference between altcom_send() and altcom_write() is the presence of
 *   flags. With zero flags parameter, altcom_send() is equivalent to
 *   altcom_write(). Also, send(sockfd,buf,len,flags) is equivalent to
 *   altcom_sendto(sockfd,buf,len,flags,NULL,0).
 *
 * Parameters:
 *   sockfd   Socket descriptor of socket
 *   buf      Data to send
 *   len      Length of data to send
 *   flags    Send flags
 *
 * Returned Value:
 *   On success, returns the number of characters sent. On error,
 *   -1 is returned, and errno is set appropriately.
 *
 ****************************************************************************/

#define send(sockfd, buf, len, flags) altcom_send(sockfd, buf, len, flags)

/****************************************************************************
 * Name: altcom_sendto
 *
 * Description:
 *   If altcom_sendto() is used on a connection-mode (ALTCOM_SOCK_STREAM)
 *   socket, the parameters to and 'tolen' are ignored (and the error EISCONN
 *   may be returned when they are not NULL and 0), and the error ENOTCONN is
 *   returned when the socket was not actually connected.
 *
 * Parameters:
 *   sockfd   Socket descriptor of socket
 *   buf      Data to send
 *   len      Length of data to send
 *   flags    Send flags
 *   to       Address of recipient
 *   tolen    The length of the address structure
 *
 * Returned Value:
 *   On success, returns the number of characters sent. On error,
 *   -1 is returned, and errno is set appropriately.
 *
 ****************************************************************************/

#define sendto(sockfd, buf, len, flags, to, tolen) altcom_sendto(sockfd, buf, len, flags, to, tolen)

/****************************************************************************
 * Name: altcom_setsockopt
 *
 * Description:
 *   altcom_setsockopt() sets the option specified by the 'option' argument,
 *   at the protocol level specified by the 'level' argument, to the value
 *   pointed to by the 'value' argument for the socket associated with the
 *   file descriptor specified by the 'sockfd' argument.
 *
 *   The 'level' argument specifies the protocol level of the option. To set
 *   options at the socket level, specify the level argument
 *   as ALTCOM_SOL_SOCKET.
 *
 * Parameters:
 *   sockfd    Socket descriptor of socket
 *   level     Protocol level to set the option
 *   option    identifies the option to set
 *   value     Points to the argument value
 *   value_len The length of the argument value
 *
 * Returned Value:
 *  0 on success; -1 on failure
 *
 ****************************************************************************/

#define setsockopt(sockfd, level, option, value, len) \
  altcom_setsockopt(sockfd, level, option, value, len)

/****************************************************************************
 * Name: altcom_shutdown
 *
 * Description:
 *   The altcom_shutdown() function will cause all or part of a full-duplex
 *   connection on the socket associated with the file descriptor socket to
 *   be shut down.
 *
 *   The altcom_shutdown() function disables subsequent send and/or receive
 *   operations on a socket, depending on the value of the how argument.
 *
 * Input Paramteers:
 *   sockfd - Specifies the file descriptor of the socket.
 *   how    - Specifies the type of shutdown. The values are as follows:
 *
 *     ALTCOM_SHUT_RD   - Disables further receive operations.
 *     ALTCOM_SHUT_WR   - Disables further send operations.
 *     ALTCOM_SHUT_RDWR - Disables further send and receive operations.
 *
 * Returned Value:
 *   Upon successful completion, altcom_shutdown() will return 0; otherwise,
 *   -1 will be returned and errno set to indicate the error.
 *
 ****************************************************************************/

#define shutdown(sockfd, how) altcom_shutdown(sockfd, how)

/****************************************************************************
 * Name: altcom_socket
 *
 * Description:
 *   altcom_socket() creates an endpoint for communication and returns
 *   a descriptor.
 *
 * Parameters:
 *   domain   See Address family.
 *   type     See Socket protocol type.
 *   protocol See Protocol.
 *
 * Returned Value:
 *   A non-negative socket descriptor on success; -1 on error with errno set
 *   appropriately.
 *
 ****************************************************************************/

#define socket(domain, type, protocol) altcom_socket(domain, type, protocol)

/****************************************************************************
 * Name: altcom_write
 *
 * Description:
 *   The altcom_write() is the same as altcom_send() with the flags parameter
 *   set to zero.
 *
 * Parameters:
 *   sockfd   Socket descriptor of socket
 *   buf      Data to write
 *   len      Length of data to send
 *
 * Returned Value:
 *   On success, returns the number of characters sent. On error,
 *   -1 is returned, and errno is set appropriately.
 *
 ****************************************************************************/

#define write(sockfd, buf, len) altcom_write(sockfd, buf, len)

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __ALTCOM_TEST_SOCKET_H */
