/****************************************************************************
 * modules/lte/altcom/api/mbedtls/ssl_write.c
 *
 *   Copyright 2018 Sony Corporation
 *   Copyright 2020 Sony Semiconductor Solutions Corporation
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

#include <string.h>
#include "dbg_if.h"
#include "altcom_errno.h"
#include "altcom_seterrno.h"
#include "apicmd_ssl_write.h"
#include "apicmd_ssl.h"
#include "apiutil.h"
#include "mbedtls/ssl.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SSL_WRITE_REQ_DATALEN (sizeof(struct apicmd_ssl_write_s))
#define SSL_WRITE_RES_DATALEN (sizeof(struct apicmd_ssl_writeres_s))
#define SSL_WRITE_REQ_DATALEN_V4 (APICMD_TLS_SSL_CMD_DATA_SIZE + \
                                 sizeof(struct apicmd_ssl_write_v4_s))
#define SSL_WRITE_RES_DATALEN_V4 (APICMD_TLS_SSL_CMDRES_DATA_SIZE)

#define SSL_WRITE_SUCCESS 0
#define SSL_WRITE_FAILURE -1

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ssl_write_req_s
{
  uint32_t            id;
  const unsigned char *buf;
  size_t              len;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int32_t ssl_write_request(FAR struct ssl_write_req_s *req)
{
  int32_t  ret;
  uint16_t reslen = 0;
  size_t   writelen = 0;
  FAR void *cmd = NULL;
  FAR void *res = NULL;
  int      protocolver = 0;
  uint16_t reqbuffsize = 0;
  uint16_t resbuffsize = 0;

  /* Set parameter from protocol version */

  protocolver = APICMD_VER_V4;

  if (protocolver == APICMD_VER_V1)
    {
      reqbuffsize = SSL_WRITE_REQ_DATALEN;
      resbuffsize = SSL_WRITE_RES_DATALEN;
    }
  else if (protocolver == APICMD_VER_V4)
    {
      reqbuffsize = SSL_WRITE_REQ_DATALEN_V4;
      resbuffsize = SSL_WRITE_RES_DATALEN_V4;
    }
  else
    {
      return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
    }

  /* Allocate send and response command buffer */

  if (!altcom_mbedtls_alloc_cmdandresbuff(
    (FAR void **)&cmd, APICMDID_TLS_SSL_CMD,
    reqbuffsize, (FAR void **)&res, resbuffsize))
    {
      return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
    }

  /* Fill the data */

  if (protocolver == APICMD_VER_V1)
    {
      ((FAR struct apicmd_ssl_write_s *)cmd)->ssl = htonl(req->id);
      writelen = (req->len <= APICMD_SSL_WRITE_BUF_LEN)
        ? req->len : APICMD_SSL_WRITE_BUF_LEN;
      memcpy(((FAR struct apicmd_ssl_write_s *)cmd)->buf, req->buf, writelen);
      ((FAR struct apicmd_ssl_write_s *)cmd)->len = htonl(writelen);
    }
  else if (protocolver == APICMD_VER_V4)
    {
      ((FAR struct apicmd_sslcmd_s *)cmd)->ssl = htonl(req->id);
      ((FAR struct apicmd_sslcmd_s *)cmd)->subcmd_id =
        htonl(APISUBCMDID_TLS_SSL_WRITE);
      writelen = (req->len <= APICMD_SSL_WRITE_BUF_LEN)
        ? req->len : APICMD_SSL_WRITE_BUF_LEN;
      memcpy(((FAR struct apicmd_sslcmd_s *)cmd)->u.write.buf,
        req->buf, writelen);
      ((FAR struct apicmd_sslcmd_s *)cmd)->u.write.len = htonl(writelen);
    }

  DBGIF_LOG1_DEBUG("[ssl_write]ctx id: %d\n", req->id);
  DBGIF_LOG1_DEBUG("[ssl_write]write len: %d\n", writelen);

  /* Send command and block until receive a response */

  ret = apicmdgw_send((FAR uint8_t *)cmd, (FAR uint8_t *)res,
                      resbuffsize, &reslen,
                      ALT_OSAL_TIMEO_FEVR);

  if (ret < 0)
    {
      DBGIF_LOG1_ERROR("apicmdgw_send error: %d\n", ret);
      goto errout_with_cmdfree;
    }

  if (reslen != resbuffsize)
    {
      DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", reslen);
      goto errout_with_cmdfree;
    }

  if (protocolver == APICMD_VER_V1)
    {
      ret = ntohl(((FAR struct apicmd_ssl_writeres_s *)res)->ret_code);
    }
  else if (protocolver == APICMD_VER_V4)
    {
      if (ntohl(((FAR struct apicmd_sslcmdres_s *)res)->subcmd_id) !=
          APISUBCMDID_TLS_SSL_WRITE)
        {
          DBGIF_LOG1_ERROR("Unexpected sub command id: %d\n",
            ntohl(((FAR struct apicmd_sslcmdres_s *)res)->subcmd_id));
          goto errout_with_cmdfree;
        }
      ret = ntohl(((FAR struct apicmd_sslcmdres_s *)res)->ret_code);
    }

  DBGIF_LOG1_DEBUG("[ssl_write res]ret: %d\n", ret);

  altcom_mbedtls_free_cmdandresbuff(cmd, res);

  return ret;

errout_with_cmdfree:
  altcom_mbedtls_free_cmdandresbuff(cmd, res);
  return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
}



/****************************************************************************
 * Public Functions
 ****************************************************************************/

int mbedtls_ssl_write(mbedtls_ssl_context *ssl, const unsigned char *buf, size_t len)
{
  int32_t                result;
  struct ssl_write_req_s req;

  if (!altcom_isinit())
    {
      DBGIF_LOG_ERROR("Not intialized\n");
      altcom_seterrno(ALTCOM_ENETDOWN);
      return MBEDTLS_ERR_SSL_BAD_INPUT_DATA;
    }

  req.id = ssl->id;
  req.buf = buf;
  req.len = len;

  result = ssl_write_request(&req);

  return result;
}
