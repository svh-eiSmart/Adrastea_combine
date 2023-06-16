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

#define ALTCOM_IO_REMOVE_DATA_LEN (sizeof(struct apicmd_io_remove_s))
#define ALTCOM_IO_REMOVE_RES_DATA_LEN (sizeof(struct apicmd_io_removeres_s))

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: altcom_io_remove
 *
 * Description:
 *   Remove a file on modem.
 *
 * Input Parameters:
 *   pathname - Path name to remove.
 *              Maxinum pathname length is 255 bytes.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, -1 is returned.
 *
 ****************************************************************************/

int altcom_io_remove(const char *pathname) {
  int32_t ret = 0;
  FAR struct apicmd_io_remove_s *cmdbuff = NULL;
  FAR struct apicmd_io_removeres_s *resbuff = NULL;
  uint16_t reslen = 0;

  /* Check if the library is initialized */

  if (!altcom_isinit()) {
    DBGIF_LOG_ERROR("Not intialized\n");
    return -1;
  }

  /* Check parameter */

  if (!pathname) {
    DBGIF_LOG_ERROR("pathname is NULL parameter.\n");
    return -1;
  }

  if (APICMD_IO_PATHNAME_LENGTH <= strlen(pathname)) {
    DBGIF_LOG_ERROR("Too long pathname.\n");
    return -1;
  }

  /* Allocate API command buffer to send */

  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmdbuff, APICMDID_IO_REMOVE,
                                         ALTCOM_IO_REMOVE_DATA_LEN, (FAR void **)&resbuff,
                                         ALTCOM_IO_REMOVE_RES_DATA_LEN)) {
    strncpy((FAR char *)cmdbuff->pathname, pathname, APICMD_IO_PATHNAME_LENGTH - 1);
  } else {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    return -1;
  }

  /* Send API command to modem */

  ret = apicmdgw_send((FAR uint8_t *)cmdbuff, (FAR uint8_t *)resbuff, ALTCOM_IO_REMOVE_RES_DATA_LEN,
                      &reslen, ALT_OSAL_TIMEO_FEVR);

  if (0 > ret) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %d\n", ret);
    ret = -1;
  } else if (ALTCOM_IO_REMOVE_RES_DATA_LEN != reslen) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", reslen);
    ret = -1;
  } else {
    ret = ntohl(resbuff->ret_code);
    DBGIF_LOG1_NORMAL("altcom_io_remove[%d]\n", ret);
  }

  altcom_free_cmd((FAR uint8_t *)cmdbuff);
  BUFFPOOL_FREE(resbuff);

  return ret;
}
