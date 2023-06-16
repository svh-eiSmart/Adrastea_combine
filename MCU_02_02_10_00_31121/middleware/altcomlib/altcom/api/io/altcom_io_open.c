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

#define ALTCOM_IO_OPEN_SUPPORTED_FLAG \
  (O_RDONLY | O_WRONLY | O_RDWR | O_APPEND | O_CREAT | O_TRUNC | O_EXCL)

#define ALTCOM_IO_OPEN_SUPPORTED_MODE (S_IRWXU | S_IRWXG | S_IRWXO)

#define ALTCOM_IO_OPEN_DATA_LEN (sizeof(struct apicmd_io_open_s))
#define ALTCOM_IO_OPEN_RES_DATA_LEN (sizeof(struct apicmd_io_openres_s))

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: conv_flags_to_apicmd
 *
 * Description:
 *   Convert flags to api command format.
 *
 * Input Parameters:
 *   flags : Flags for opening a file(system-dependent value).
 *
 * Returned Value:
 *   Flags of api command format is returned.
 *
 ****************************************************************************/

static int32_t conv_flags_to_apicmd(int flags) {
  int32_t apicmd_flags = 0;

  if (O_RDONLY == (O_ACCMODE & flags)) {
    apicmd_flags |= APICMD_IO_O_RDONLY;
  } else if (O_WRONLY == (O_ACCMODE & flags)) {
    apicmd_flags |= APICMD_IO_O_WRONLY;
  } else if (O_RDWR == (O_ACCMODE & flags)) {
    apicmd_flags |= APICMD_IO_O_RDWR;
  } else {
    return -EINVAL;
  }

  if (O_APPEND & flags) {
    apicmd_flags |= APICMD_IO_O_APPEND;
  }
  if (O_CREAT & flags) {
    apicmd_flags |= APICMD_IO_O_CREAT;
  }
  if (O_TRUNC & flags) {
    apicmd_flags |= APICMD_IO_O_TRUNC;
  }
  if (O_EXCL & flags) {
    apicmd_flags |= APICMD_IO_O_EXCL;
  }

  return apicmd_flags;
}

/****************************************************************************
 * Name: conv_mode_to_apicmd
 *
 * Description:
 *   Convert mode to api command format.
 *
 * Input Parameters:
 *   flags : Mode for opening a file(system-dependent value).
 *
 * Returned Value:
 *   Mode of api command format is returned.
 *
 ****************************************************************************/

static int32_t conv_mode_to_apicmd(mode_t mode) {
  int32_t apicmd_mode = 0;

  if (S_IRUSR & mode) {
    apicmd_mode |= APICMD_IO_S_IRUSR;
  }
  if (S_IWUSR & mode) {
    apicmd_mode |= APICMD_IO_S_IWUSR;
  }
  if (S_IXUSR & mode) {
    apicmd_mode |= APICMD_IO_S_IXUSR;
  }
  if (S_IRGRP & mode) {
    apicmd_mode |= APICMD_IO_S_IRGRP;
  }
  if (S_IWGRP & mode) {
    apicmd_mode |= APICMD_IO_S_IWGRP;
  }
  if (S_IXGRP & mode) {
    apicmd_mode |= APICMD_IO_S_IXGRP;
  }
  if (S_IROTH & mode) {
    apicmd_mode |= APICMD_IO_S_IROTH;
  }
  if (S_IWOTH & mode) {
    apicmd_mode |= APICMD_IO_S_IWOTH;
  }
  if (S_IXOTH & mode) {
    apicmd_mode |= APICMD_IO_S_IXOTH;
  }

  return apicmd_mode;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: altcom_io_open
 *
 * Description:
 *   Open a file on modem.
 *
 * Input Parameters:
 *   pathname - Path name to open.
 *              Maxinum pathname length is 255 bytes.
 *   flags    - The file control flags and file access modes.
 *   mode     - Access permission if new file will be created.
 *
 * Returned Value:
 *   On success, file descriptor is returned.
 *   It is a non-negative integer value.
 *   On failure, -1 is returned.
 *
 ****************************************************************************/

int altcom_io_open(const char *pathname, int flags, mode_t mode) {
  int32_t ret = 0;
  FAR struct apicmd_io_open_s *cmdbuff = NULL;
  FAR struct apicmd_io_openres_s *resbuff = NULL;
  uint16_t reslen = 0;
  int32_t apicmd_flags = 0;
  int32_t apicmd_mode = 0;

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

  if (flags & ~(ALTCOM_IO_OPEN_SUPPORTED_FLAG)) {
    DBGIF_LOG1_ERROR("Unsupported flags:0x%08X\n", flags);
    return -1;
  }

  apicmd_flags = conv_flags_to_apicmd(flags);
  if (0 > apicmd_flags) {
    DBGIF_LOG1_ERROR("Failed to convert flags:0x%08X\n", flags);
    return -1;
  }

  if (mode & ~(ALTCOM_IO_OPEN_SUPPORTED_MODE)) {
    DBGIF_LOG1_ERROR("Unsupported mode:0x%08X\n", mode);
    return -1;
  }

  apicmd_mode = conv_mode_to_apicmd(mode);
  if (0 > apicmd_mode) {
    DBGIF_LOG1_ERROR("Failed to convert mode:0%o\n", mode);
    return -1;
  }

  /* Allocate API command buffer to send */

  if (altcom_generic_alloc_cmdandresbuff((FAR void **)&cmdbuff, APICMDID_IO_OPEN,
                                         ALTCOM_IO_OPEN_DATA_LEN, (FAR void **)&resbuff,
                                         ALTCOM_IO_OPEN_RES_DATA_LEN)) {
    strncpy((FAR char *)cmdbuff->pathname, pathname, APICMD_IO_PATHNAME_LENGTH - 1);
    cmdbuff->flags = htonl(apicmd_flags);
    cmdbuff->mode = htonl(apicmd_mode);
  } else {
    DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
    return -1;
  }

  /* Send API command to modem */

  ret = apicmdgw_send((FAR uint8_t *)cmdbuff, (FAR uint8_t *)resbuff, ALTCOM_IO_OPEN_RES_DATA_LEN,
                      &reslen, ALT_OSAL_TIMEO_FEVR);

  if (0 > ret) {
    DBGIF_LOG1_ERROR("apicmdgw_send error: %d\n", ret);
    ret = -1;
  } else if (ALTCOM_IO_OPEN_RES_DATA_LEN != reslen) {
    DBGIF_LOG1_ERROR("Unexpected response data length: %d\n", reslen);
    ret = -1;
  } else {
    ret = ntohl(resbuff->ret_code);
    DBGIF_LOG1_NORMAL("altcom_io_open[%d]\n", ret);
  }

  altcom_free_cmd((FAR uint8_t *)cmdbuff);
  BUFFPOOL_FREE(resbuff);

  return ret;
}
