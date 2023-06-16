/****************************************************************************
 *
 *   Copyright 2019 Sony Semiconductor Solutions Corporation
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

#include "altcom_io.h"
#include "apicmd_io.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "apicmdgw.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ALTCOM_IO_READ_DATA_LEN (sizeof(struct apicmd_io_read_s))
#define ALTCOM_IO_READ_RES_DATA_LEN (sizeof(struct apicmd_io_readres_s))

#define ALTCOM_IO_READ_CALC_REQ_SZ(len) \
  (ALTCOM_IO_READ_RES_DATA_LEN - APICMD_IO_READ_DATA_LENGTH + (len))

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: altcom_io_read
 *
 * Description:
 *   Read data from file on modem.
 *
 * Input Parameters:
 *   fd    - File descriptor.
 *   buf   - Buffer to read.
 *   count - Read length.
 *
 * Returned Value:
 *   On success, the number of bytes read is returned.
 *   On failure, -1 is returned.
 *
 ****************************************************************************/

ssize_t altcom_io_read(int fd, void *buf, size_t count) {
  int32_t ret = 0;
  FAR struct apicmd_io_read_s *cmdbuff = NULL;
  FAR struct apicmd_io_readres_s *resbuff = NULL;
  uint16_t resbufflen = 0;
  uint16_t reslen = 0;
  int32_t read_req_len = 0;
  int32_t read_res_len = 0;

  /* Check if the library is initialized */

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return -1;
  }

  /* Check parameter */

  if (!buf) {
    DBGIF_LOG_ERROR("read buf is NULL parameter.\n");
    return -1;
  }

  read_req_len = APICMD_IO_READ_DATA_LENGTH < count ? APICMD_IO_READ_DATA_LENGTH : count;
  DBGIF_LOG1_NORMAL("altcom_io_read read_req_len:%d\n", read_req_len);
  resbufflen = ALTCOM_IO_READ_CALC_REQ_SZ(read_req_len);

  /* Allocate API command buffer to send */

  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmdbuff, APICMDID_IO_READ,
                                         ALTCOM_IO_READ_DATA_LEN, (FAR void **)&resbuff,
                                         resbufflen)) {
    cmdbuff->fd = htonl(fd);
    cmdbuff->readlen = htonl(read_req_len);
  } else {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    return -1;
  }

  /* Send API command to modem */

  ret = apicmdgw_send((FAR uint8_t *)cmdbuff, (FAR uint8_t *)resbuff, resbufflen, &reslen,
                      ALT_OSAL_TIMEO_FEVR);

  if (0 > ret) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %d\n", ret);
    ret = -1;
  } else if (resbufflen != reslen) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", reslen);
    ret = -1;
  } else {
    read_res_len = ntohl(resbuff->ret_code);

    if (read_res_len < 0 || read_req_len < read_res_len) {
      DBGIF_LOG1_ERROR("Unexpected read data length: %d\n", read_res_len);
      ret = -1;
    } else {
      memcpy(buf, resbuff->readdata, read_res_len);
      ret = read_res_len;
      DBGIF_LOG1_NORMAL("altcom_io_read[%d]\n", ret);
    }
  }

  altcom_free_cmd((FAR uint8_t *)cmdbuff);
  BUFFPOOL_FREE(resbuff);

  return ret;
}
