/****************************************************************************
 * modules/lte/altcom/api/socket/altcom_ioctl.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "dbg_if.h"
#include "altcom_sock.h"
#include "altcom_socket.h"
#include "apicmd_ioctl.h"
#include "altcom_errno.h"
#include "apiutil.h"

#define IOCTL_REQ_DATALEN (sizeof(struct apicmd_ioctl_s))
#define IOCTL_RES_DATALEN (sizeof(struct apicmd_ioctlres_s))

/****************************************************************************
 * Public Functions
 ****************************************************************************/

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

int altcom_ioctl(int sockfd, long req, void *argp) {
  int32_t ret;
  int32_t err;
  FAR struct altcom_socket_s *fsock;
  uint16_t reslen = 0;
  FAR struct apicmd_ioctl_s *cmd = NULL;
  FAR struct apicmd_ioctlres_s *res = NULL;

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    altcom_seterrno(ALTCOM_ENETDOWN);
    return -1;
  }

  if (!argp) {
    DBGIF_LOG_ERROR("Invalid parameter\n");
    altcom_seterrno(ALTCOM_EINVAL);
    return -1;
  }

  fsock = altcom_sockfd_socket(sockfd);
  if (!fsock) {
    altcom_seterrno(ALTCOM_EINVAL);
    return -1;
  }

  /* Allocate send and response command buffer */

  if (!altcom_sock_alloc_cmdandresbuff((FAR void **)&cmd, APICMDID_SOCK_IOCTL,
                                       IOCTL_REQ_DATALEN, (FAR void **)&res,
                                       IOCTL_RES_DATALEN)) {
    return -1;
  }

  /* Fill the data */

  cmd->sockfd = htonl(sockfd);
  cmd->req = htonl(req);

  /* Send command and block until receive a response */

  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res, IOCTL_RES_DATALEN, &reslen,
                      ALT_OSAL_TIMEO_FEVR);
  if (ret >= 0) {
    ret = ntohl(res->ret_code);
    err = ntohl(res->err_code);

    if (ret == APICMD_IOCTL_RES_RET_CODE_ERR) {
      DBGIF_LOG1_ERROR("API command response is err :%ld.\n", err);
      altcom_seterrno(err);
    } else {
      switch (req) {
        case ALTCOM_FIONREAD:
        case ALTCOM_TIOCOUTQ:
          memcpy(argp, (int[]){ ntohl(*(int*)res->arg) }, sizeof(int));
          break;
        default:
          altcom_seterrno(ALTCOM_EINVAL);
          ret = -1;
          break;
      }
    }
  } else {
    altcom_seterrno(-ret);
    ret = -1;
  }

  altcom_sock_free_cmdandresbuff(cmd, res);

  return ret;
}
